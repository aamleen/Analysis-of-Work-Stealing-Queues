/*
 * Copyright 2017 Rice University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <hclib-perfcounter.h>

#ifdef PERFCOUNTER  //default is enabled. Disable it during installation by using --disable-perfcounter
#include <hclib-internal.h>
#include <assert.h>
#include <likwid.h>
#include <stdlib.h>
#include <string.h>

/*
 * Tutorial on integrating likwid in C/C++ application can be found here:
 * https://github.com/RRZE-HPC/likwid/wiki/TutorialLikwidC
 */

// For AMD
#define NUM_EVENTS 189
#define MAX_REGIONS 50
#define MAX_COUNTERS 20

typedef struct {
    char name[100];
    double counters[MAX_COUNTERS];
} region_t;

static int region_counter = 0;
static region_t region_list[MAX_REGIONS];

//-- global variables --->
static int* cpus;
static int gid;
static char* estr;
static int* cpus_in_use;
static int total_workers;
static char *evts[NUM_EVENTS];
static int num_evts = 0;
double* results;
//<-- global variables ---

//forward declaration
void perfcounters_dump();

/*
 * This must be called on master thread after all the 
 * workers are up and running.
 *
 * Should be called by the master thread only (outside the parallel region)
 */
void perfcounters_init() {
    int err, i;
    // No meaning of using perfcounters unless threads have been pinned
    assert(getenv("HCLIB_BIND_THREADS"));
    cpus_in_use = get_thread_bind_map();
    // No meaning of using erfcounters unless HCLIB_PERFCOUNTERS are specified
    assert(getenv("HCLIB_PERFCOUNTERS"));
    setenv("LIKWID_FORCE", "1", 1);
    // Initialize the topology module and fill internal 
    // structures with the topology of the current system
    err = topology_init();
    assert(err>=0 && "Error in topology_init()");
    //Get a pointer to the topology information structure holding data 
    //like number of total/online CPUs, number of CPU sockets and a 
    //sub list for all threads with their IDs and location.
    CpuTopology_t topo = get_cpuTopology();
    // Create affinity domains. Commonly only needed when reading Uncore counters
    affinity_init();
    total_workers = CURRENT_WS_INTERNAL->context->nworkers>topo->numHWThreads?topo->numHWThreads:CURRENT_WS_INTERNAL->context->nworkers;
    cpus = (int*)malloc(total_workers * sizeof(int));
    assert(cpus!=NULL && "Malloc failed"); 
    for (i=0;i<total_workers;i++) { 
        cpus[i] = topo->threadPool[i].apicId;
    }
    // Must be called before perfmon_init() but only if you want to use another
    // access mode as the pre-configured one. For direct access (0) you have to
    // be root.
    //accessClient_setaccessmode(0);

    //Initialize the perfmon module that enables performance measurements. 
    //Depending on the access mode, it starts the access daemon or 
    //initializes direct access to the MSR and PCI devices.
    // Started on all physical cores but for reading we'll use only
    // those that have our threads
    err = perfmon_init(total_workers, cpus);
    if(err<0) {
        topology_finalize();
	assert(0 && "Error in perfmon_init");
    }
    //Add the event string eventSet to the perfmon module for measurments. 
    //gid is the ID of the event group used later to setup the 
    //counters and retrieve the measured results. 
    estr = getenv("HCLIB_PERFCOUNTERS");
    gid = perfmon_addEventSet(estr);
    if (gid < 0) {
        perfmon_finalize();
        topology_finalize();
	assert(0 && "Error in perfmon_addEventSet");
    }
    //Program the hardware counter registers to reflect the the event 
    //string idenified by gid. 
    err = perfmon_setupCounters(gid);
    if (err < 0) {
        perfmon_finalize();
        topology_finalize();
	assert(0 && "Error in perfmon_setupCounters");
    }

    char *ptr = strtok(estr, ",");
    while (ptr != NULL) {
        evts[num_evts] = (char *) malloc(sizeof(char) * (strlen(ptr) + 1));
        strcpy(evts[num_evts], ptr);
        num_evts++;
        ptr = strtok(NULL, ",");
    }
    results = (double*) malloc(sizeof(double) * num_evts);
    memset(results, 0x00, sizeof(double) * num_evts);
}

/*
 * Should be called by master thread only (outside the parallel region)
 */
void perfcounters_finalize() {
    perfcounters_dump();
    // Print the result of every thread/CPU for all events in estr.
    free(cpus);
    free(results);
    //Close the connection to the performance monitoring module. 
    perfmon_finalize();
    affinity_finalize();
    //Empty and delete the data structures of the topology module. 
    //The data structures returned by get_cpuInfo() and get_cpuTopology() 
    //are not usable afterwards
    topology_finalize();
}

/*
 * Accessible in the user code.
 * Start the counters that have been previously set up by 
 * perfmon_setupCounters(). The counter registered are 
 * zeroed before enabling the counters
 *
 * Should be called by master thread only (outside the parallel region)
 */
void perfcounters_start() {
    // Start all counters in the previously set up event set.
    int err = perfmon_startCounters();
    if (err < 0) {
        perfmon_finalize();
        topology_finalize();
        assert(0 && "Error in perfmon_startCounters");
    }
}

/*
 * Accessible in the user code.
 * Stop the counters that have been previously started by 
 * perfmon_startCounters(). All config registers get 
 * zeroed before reading the counter register.
 *
 * Should be called by master thread only (outside the parallel region)
 */
void perfcounters_stop() {
    // Stop all counters in the previously set up event set.
    int err = perfmon_stopCounters();
    if (err < 0) {
        perfmon_finalize();
        topology_finalize();
        assert(0 && "Error in perfmon_stopCounters");
    }
}

void perfcounters_dump() {
    int i,j;
    fprintf(stdout,"\n============================ Tabulate Statistics ============================\n");
    memset(results, 0x00, sizeof(double) * num_evts);
    for(i=0; i<num_evts; i++) {
        fprintf(stdout,"%s\t",evts[i]);
	for(j=0; j<total_workers; j++) {
            results[i]+=perfmon_getResult(gid, i, cpus_in_use[j]);
	}
    }
    fprintf(stdout,"\n");
    for(i=0; i<num_evts; i++) {
	fprintf(stdout,"%f\t",results[i]);
    }
    fprintf(stdout,"\n=============================================================================\n");
    if(region_counter>0) {
        fprintf(stdout,"\n============================ Tabulate Statistics ============================\n");
	for(int k=0; k<region_counter; k++) {
            for(i=0; i<num_evts; i++) {
                fprintf(stdout,"%s.%s\t",region_list[k].name,evts[i]);
            }
            fprintf(stdout,"%s.time\t",region_list[k].name);
	}
        fprintf(stdout,"\n");
	for(int k=0; k<region_counter; k++) {
            for(i=0; i<num_evts; i++) {
	        fprintf(stdout,"%f\t",region_list[k].counters[i]);
            }
	    fprintf(stdout,"%f\t",region_list[k].counters[num_evts]);
	}
        fprintf(stdout,"\n=============================================================================\n");
    }
    fflush(stdout);
}

void perfcounters_readRegion() {
#if 0
    for(int j=0; j<total_workers; j++) {
        int err = perfmon_readCountersCpu(cpus_in_use[j]);
        if (err < 0) {
            perfmon_finalize();
            topology_finalize();
            assert(0 && "Error in perfmon_stopCounters");
        }
    }
#else
    int err = perfmon_readCounters();
    if (err < 0) {
        perfmon_finalize();
        topology_finalize();
        assert(0 && "Error in perfmon_stopCounters");
    }
#endif
}

double start_region_time=0;

void perfcounters_startRegion(char* name) {
    int found=0;
    for(int i=0; i<region_counter; i++) {
        if(strcmp(region_list[i].name, name) == 0) {
            found=1;
	    break;
	}
    }
    if(!found) {
        strcpy(region_list[region_counter].name, name);
	region_counter++;
    }
    start_region_time = mysecond();
    perfcounters_readRegion();
}

void perfcounters_stopRegion(char* name) {
    perfcounters_readRegion();
    double duration = (mysecond()-start_region_time)*1000;
    int index=-1;
    for(int i=0; i<region_counter; i++) {
        if(strcmp(region_list[i].name, name) == 0) {
            index=i;
	    break;
	}
    }
    assert(index>=0 && "ERROR: PERFCOUNTER REGION NAME NOT ADDED BEFORE");
    for(int i=0; i<num_evts; i++) {
	for(int j=0; j<total_workers; j++) {
            region_list[index].counters[i]+=perfmon_getLastResult(gid, i, cpus_in_use[j]);
	}
    }
    region_list[index].counters[num_evts] += duration;
}

#else
void perfcounters_init(){ }
void perfcounters_finalize(){ }
void perfcounters_start(){ }
void perfcounters_stop(){ }
void perfcounters_dump() { }
void perfcounters_startRegion(char* name) { }
void perfcounters_stopRegion(char* name) { }
#endif
