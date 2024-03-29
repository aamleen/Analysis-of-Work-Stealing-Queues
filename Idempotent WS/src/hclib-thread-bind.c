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

#include <stdio.h>
#include <locale.h>
#include <time.h>
#define _GNU_SOURCE
#define __USE_GNU
// #include <xlocale.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
/** Platform specific thread binding implementations -- > ONLY FOR LINUX **/

typedef int clockid_t;
#include "hclib-rt.h"

#ifdef __linux
#include <pthread.h>
static int round_robin = -1;
static int* bind_map = NULL;
static pthread_mutex_t _lock = PTHREAD_MUTEX_INITIALIZER;

int get_nb_cpus() {
    int numCPU = sysconf(_SC_NPROCESSORS_ONLN);
    return numCPU;
}

void bind_thread_with_mask(int *mask, int lg) {
    cpu_set_t cpuset;
    if (mask != NULL) {
        CPU_ZERO(&cpuset);

        /* Copy the mask from the int array to the cpuset */
        int i;
        for (i = 0; i < lg; i++) {
            CPU_SET(mask[i], &cpuset);
        }

        /* Set affinity */
        int res = sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
        if (res != 0) {
            fprintf(stdout,"ERROR: ");
            if (errno == ESRCH) {
                HASSERT("THREADBINDING ERROR: ESRCH: Process not found!\n");
            }
            if (errno == EINVAL) {
                HASSERT("THREADBINDING ERROR: EINVAL: CPU mask does not contain any actual physical processor\n");
            }
            if (errno == EFAULT) {
                HASSERT("THREADBINDING ERROR: EFAULT: memory address was invalid\n");
            }
            if (errno == EPERM) {
                HASSERT("THREADBINDING ERROR: EPERM: process does not have appropriate privileges\n");
            }
        }
    }
}

/* Bind threads in a round-robin fashion */
void bind_thread_rr(int worker_id) {
    /*bind worker_id to cpu_id round-robin fashion*/
    int nbCPU = get_nb_cpus();
    int mask = worker_id % nbCPU;
    bind_thread_with_mask(&mask, 1);
}

/* Bind threads according to bind map */
void bind_thread_map(int worker_id, int bind_map_size) {
    int mask = bind_map[worker_id % bind_map_size];
    fprintf(stdout,"HCLIB_INFO: Binding worker %d to cpu_id %d\n", worker_id, mask);
    fflush(stdout);
    bind_thread_with_mask(&mask, 1);
}

/** Thread binding api to bind a worker thread using a particular binding strategy **/
void bind_thread(int worker_id, int nworkers) {
    assert(pthread_mutex_lock(&_lock) == 0);
    if(round_robin == -1) {
        char* map = getenv("HCLIB_BIND_THREADS");
	assert(map);
	bind_map = (int*) malloc(sizeof(int) * nworkers);
	assert(bind_map);
        int index=0;
	char *token = strtok(map, ",");
        while(token) {
            bind_map[index++]=atoi(token);
	    token = strtok(NULL, ",");
	}	
	if(nworkers>1 && index==nworkers) {
            fprintf(stdout,"HCLIB_INFO: Thread Binding as per Bind Map\n");
            fflush(stdout);
            round_robin=0;
	}
	else {
            fprintf(stdout,"HCLIB_INFO: Round Robin Thread Binding\n");
            fflush(stdout);
            round_robin=1;
	    for(index=0; index<nworkers; index++) bind_map[index] = index;
	}
    }
    assert(pthread_mutex_unlock(&_lock) == 0);

    if (round_robin == 1) {
        /* Round robin binding */
        bind_thread_rr(worker_id);
    } else { /*Bind map provided */
        bind_thread_map(worker_id, nworkers);
    }
}

int* get_thread_bind_map() {
    assert(bind_map);
    return bind_map;
}
#else
void bind_thread(int worker_id, int nworkers) {

}
int* get_thread_bind_map() { 
    assert(0 && "This API should be called only with HCLIB_BIND_THREADS");
    return NULL;
}
#endif


