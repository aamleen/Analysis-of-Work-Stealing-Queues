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

#include <pthread.h>
#include <sys/time.h>
#include <stddef.h>

#include <hclib.h>
#include <hclib-internal.h>
#include <hclib-atomics.h>
#include <hclib-finish.h>
#include <hclib-hpt.h>
#include <hclib-perfcounter.h>
#include <hclib-timeline.h>


// new header files

#define _XOPEN_SOURCE 500
#include <unistd.h>
#include <stdio.h>  // for printf
#include <stdlib.h>  // for malloc - temp until share memory region allocated
#include <inttypes.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h> //for handling SIGINT signal
#include <string.h> //for parsing argument 2, number of samples and time span for sleep




// Energy measurement
// ---------------------------------------------------------------------------------------------------------------------------------


/* Intel Xeon Power MSR register addresses  (change according to your machine) */
// register value for different scope
#define IA32_PERF_GLOBAL_CTRL_VALUE 0x70000000F // bit {0-3} tells us the number of PMC registers in use (i'th bit implies that PMC[i] is active)
#define IA32_FIXED_CTR_CTRL_VALUE 0x2 // Control register for Fixed Counter 
// Fixed CTRL register
#define IA32_FIXED_CTR_CTRL             0x38D // Controls for fixed ctr0, 1, and 2 
#define IA32_PERF_GLOBAL_CTRL           0x38F // Enables for fixed ctr0,1,and2 here
#define IA32_FIXED_CTR0                 0x309 // (R/W) Counts Instr_Retired.Any
/* RAPL defines */
#define MSR_RAPL_POWER_UNIT             0x606
#define MSR_PKG_ENERGY_STATUS           0x611
/************************************************************************/
// Machine configuration  (change according to your machine)
#define CORESperSOCKET 10
#define SOCKETSperNODE 2
#define NNODES 1
/************************************************************************/


int64_t numOfNodes = -1;
int64_t numOfSockets = -1;
int64_t numOfCores = -1;

uint64_t *energyWrap;
uint64_t *energySave;


static int isBlockTopology = 0; //Assuming non-block (cyclic etc.) topology for the system by default

uint64_t TOTAL_PWR_PKG_ENERGY[SOCKETSperNODE];
uint64_t LAST_PWR_PKG_ENERGY[SOCKETSperNODE];
uint64_t PWR_PKG_ENERGY_Core[SOCKETSperNODE];

uint64_t TOTAL_INST_RETIRED[SOCKETSperNODE * CORESperSOCKET];
uint64_t LAST_INST_RETIRED[SOCKETSperNODE * CORESperSOCKET];
uint64_t INST_RETIRED_CORE[SOCKETSperNODE * CORESperSOCKET];

uint64_t POWER_UNIT = 0;
double JOULE_UNIT = 0.0;  // convert energy counter in JOULE


uint64_t readMSR(uint32_t core , uint32_t name){
    int fd = -1;
    char filename[256];
    sprintf(filename, "/dev/cpu/%d/msr", core);
    fd = open(filename, O_RDONLY);
    if(fd < 0){
    	fprintf (stderr, "\n%s : open failed", filename);
    	return -1;
    }
    uint64_t data;
    if (pread(fd, &data, sizeof(data), name) != sizeof(data)) {
        perror("rdmsr:pread");
        exit(127);
    }
    close(fd);
    return data;
}

int writeMSR(int cpu, uint32_t reg, uint64_t data)
{
  int fd;
  char msr_file_name[64];

  sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);
  fd = open(msr_file_name, O_WRONLY);
  if (fd < 0) {
    if (errno == ENXIO) {
      fprintf(stderr, "wrmsr: No CPU %d\n", cpu);
      exit(2);
    } else if (errno == EIO) {
      fprintf(stderr, "wrmsr: CPU %d doesn't support MSRs\n",
        cpu);
      exit(3);
    } else {
      perror("wrmsr@: open");
      exit(127);
    }
  }

    if (pwrite(fd, &data, sizeof data, reg) != sizeof data) {
        if (errno == EIO) {
            fprintf(stderr,
                "wrmsr: CPU %d cannot set MSR "
                "0x%08" PRIx32 " to 0x%016" PRIx64 "\n",
                cpu, reg, data);
            return(4);
        } else {
            perror("wrmsr: pwrite");
            return(127);
        }
    }

  close(fd);

  return(0);
}



/* Function returns the physical package id (socket number) given a cpu */
int get_physical_package_id (int cpu)
{
  char path[256];
  FILE *fileP;
  int physicalPackageId;

  sprintf (path, "/sys/devices/system/cpu/cpu%d/topology/physical_package_id", cpu);

  fileP = fopen (path, "r");
  if (!fileP)
  {
    fprintf (stderr, "\n%s : open failed", path);
    return -1;
  }

  if (fscanf (fileP, "%d", &physicalPackageId) != 1)
  {
    fprintf (stderr, "\n%s: failed to parse from file", path);
    return -1;
  }

  fclose(fileP);
  return physicalPackageId;
}


void perfcounters_dump_energy();
void perfcounters_read_energy();

void perfcounters_init_energy(){

    int sock;
    //Store local copies of the socket and core counts -- check for previous intialization
    if (numOfNodes == -1) numOfNodes = NNODES;
    if (numOfSockets == -1) numOfSockets = SOCKETSperNODE;
    if (numOfCores == -1) numOfCores = CORESperSOCKET; 

    energyWrap = (uint64_t *) malloc (sizeof (uint64_t) * numOfSockets);
    energySave = (uint64_t *) malloc (sizeof (uint64_t) * numOfSockets);
   
	for (int core = 0; core < numOfCores * numOfSockets; core++)
	{
		// set Global Counter to read instruction at user level only
	    writeMSR (core, IA32_PERF_GLOBAL_CTRL, IA32_PERF_GLOBAL_CTRL_VALUE);
      writeMSR (core, IA32_FIXED_CTR_CTRL, IA32_FIXED_CTR_CTRL_VALUE);
	}

}
void perfcounters_start_energy(){
    //compute power unit
    int correctedCoreNumber;
    int sock;
    POWER_UNIT = readMSR(0, MSR_RAPL_POWER_UNIT); // calculate once
    JOULE_UNIT = 1.0 / (1 << ((POWER_UNIT >> 8) & 0x1F));

    for (sock = 0; sock < numOfSockets; sock++)
    { 
		energyWrap[sock] = 0;
        energySave[sock] = 0;
        PWR_PKG_ENERGY_Core[sock] = 0;
        LAST_PWR_PKG_ENERGY[sock] = 0;
        TOTAL_PWR_PKG_ENERGY[sock] = 0;
        //Discover the topology of the system using the physical id and assign correct cores to sockets 
        if (sock == get_physical_package_id(sock))
        {
            correctedCoreNumber = sock;
        }
        else
        {
            correctedCoreNumber = sock * numOfCores;
            isBlockTopology = 1;
        }
        uint64_t energyStatus = readMSR(correctedCoreNumber, MSR_PKG_ENERGY_STATUS); // get energy MSR

        uint64_t energyCounter = energyStatus & 0xffffffff; // only 32 of 64 bits good 
        if (energyCounter < energySave[sock]) 
        { 
            // did I just wrap the counter?
            energyWrap[sock]++;
        }
        energySave[sock] = energyCounter;
        energyCounter = energyCounter + (energyWrap[sock]<<32);// number of wraps in upper 32 bits
        PWR_PKG_ENERGY_Core[sock] = energyCounter;
    }
	for (int core=0; core<numOfCores * numOfSockets; core++)
	{
		INST_RETIRED_CORE[core]=0;
		LAST_INST_RETIRED[core]=0;
		TOTAL_INST_RETIRED[core]=0;
		INST_RETIRED_CORE[core] = readMSR (core, IA32_FIXED_CTR0);
	}
}

void perfcounters_finalize_energy(){
  perfcounters_dump_energy();
  free(energyWrap);
  free(energySave);
}

void perfcounters_read_energy(){
	int sock;
    int correctedCoreNumber;
	for (sock = 0; sock < numOfSockets; sock++)
	{
        //Discover the topology of the system using the physical id and assign correct cores to sockets 
        if (sock == get_physical_package_id(sock))
        {
            correctedCoreNumber = sock;
        }
        else
        {
            correctedCoreNumber = sock * numOfCores;
            isBlockTopology = 1;
        }

		uint64_t energyStatus = readMSR(correctedCoreNumber, MSR_PKG_ENERGY_STATUS); // get energy MSR

		uint64_t energyCounter = energyStatus & 0xffffffff; // only 32 of 64 bits good 
		if (energyCounter < energySave[sock]) 
		{ 
		  // did I just wrap the counter?
		  energyWrap[sock]++;
		}
		energySave[sock] = energyCounter;
		energyCounter = energyCounter + (energyWrap[sock]<<32);// number of wraps in upper 32 bits
	   
	    LAST_PWR_PKG_ENERGY[sock] = energyCounter - PWR_PKG_ENERGY_Core[sock];
	    TOTAL_PWR_PKG_ENERGY[sock] += LAST_PWR_PKG_ENERGY[sock];
		PWR_PKG_ENERGY_Core[sock] = energyCounter;
	}
	for (int core=0; core<numOfCores * numOfSockets; core++)
	{
		uint64_t instruction = readMSR (core, IA32_FIXED_CTR0);
		LAST_INST_RETIRED[core] = instruction - INST_RETIRED_CORE[core];
		TOTAL_INST_RETIRED[core] += LAST_INST_RETIRED[core];
		INST_RETIRED_CORE[core] = instruction;
	}
    
}

void perfcounters_stop_energy(){
    perfcounters_read_energy();
}

void perfcounters_dump_energy(){
  int i;
  //   fprintf(stdout,"\n============================ Tabulate Statistics ============================\n");
  //   // print all events
  //   fprintf(stdout,"%s\t","PWR_PKG_ENERGY");
	// fprintf(stdout,"%s\t","INST_RETIRED");
  //   fprintf(stdout,"\n");
    
    //printf("power %f \n", LAST_PWR_PKG_ENERGY[0]*JOULE_UNIT);
    double res=0;
    for(i=0; i<numOfSockets; i++) {
      res += ((double)TOTAL_PWR_PKG_ENERGY[i])*JOULE_UNIT;
    }
    // fprintf(stdout,"%f\t",res);
	res = 0;
	for(i=0;i<numOfSockets*numOfCores;i++) {
		res += ((double)TOTAL_INST_RETIRED[i]);
	}
    // fprintf(stdout,"%f\t",res);
    // fprintf(stdout,"\n=============================================================================\n");
    // fflush(stdout);

}


// ---------------------------------------------------------------------------------------------------------------------------------
// Energy End

static double benchmark_start_time_stats = 0;
static double user_specified_timer = 0;
// TODO use __thread on Linux?
pthread_key_t ws_key;

hc_context *hclib_context = NULL;

static char *hclib_stats = NULL;
static int bind_threads = -1;

void hclib_start_finish();

void log_(const char *file, int line, hclib_worker_state *ws,
          const char *format,
          ...) {
    va_list l;
    FILE *f = stderr;
    if (ws != NULL) {
        fprintf(f, "[worker: %d (%s:%d)] ", ws->id, file, line);
    } else {
        fprintf(f, "[%s:%d] ", file, line);
    }
    va_start(l, format);
    vfprintf(f, format, l);
    fflush(f);
    va_end(l);
}

void set_current_worker(int wid) {
    if (pthread_setspecific(ws_key, hclib_context->workers[wid]) != 0) {
        log_die("Cannot set thread-local worker state");
    }

    if (bind_threads) {
	bind_thread(wid, hclib_context->nworkers);
    }
}

int get_current_worker() {
    return ((hclib_worker_state *)pthread_getspecific(ws_key))->id;
}

#ifdef HCLIB_LITECTX_STRATEGY
static void set_curr_lite_ctx(LiteCtx *ctx) {
    CURRENT_WS_INTERNAL->curr_ctx = ctx;
}

static LiteCtx *get_curr_lite_ctx() {
    return CURRENT_WS_INTERNAL->curr_ctx;
}

static __inline__ void ctx_swap(LiteCtx *current, LiteCtx *next,
                                const char *lbl) {
    // switching to new context
    set_curr_lite_ctx(next);
    LiteCtx_swap(current, next, lbl);
    // switched back to this context
    set_curr_lite_ctx(current);
}
#endif

hclib_worker_state *current_ws() {
    return CURRENT_WS_INTERNAL;
}

// FWD declaration for pthread_create
static void *worker_routine(void *args);

/*
 * Main initialization function for the hclib_context object.
 */
void hclib_global_init() {
    // Build queues
    hclib_context->hpt = read_hpt(&hclib_context->places,
                                  &hclib_context->nplaces, &hclib_context->nproc,
                                  &hclib_context->workers, &hclib_context->nworkers);

    for (int i = 0; i < hclib_context->nworkers; i++) {
        hclib_worker_state *ws = hclib_context->workers[i];
        ws->context = hclib_context;
        ws->current_finish = NULL;
	ws->total_push=0;
	ws->total_steals=0;
#ifdef HCLIB_LITECTX_STRATEGY
        ws->curr_ctx = NULL;
        ws->root_ctx = NULL;
#endif
    }
    hclib_context->done_flags = (worker_done_t *)malloc(
                                    hclib_context->nworkers * sizeof(worker_done_t));
    for (int i = 0; i < hclib_context->nworkers; i++) {
        hclib_context->done_flags[i].flag = 1;
    }

    // Sets up the deques and worker contexts for the parsed HPT
    hc_hpt_init(hclib_context);

}

void hclib_display_runtime() {
    printf("---------HCLIB_RUNTIME_INFO-----------\n");
    printf(">>> HCLIB_WORKERS\t= %s\n", getenv("HCLIB_WORKERS"));
    printf(">>> HCLIB_HPT_FILE\t= %s\n", getenv("HCLIB_HPT_FILE"));
    printf(">>> HCLIB_BIND_THREADS\t= %s\n", bind_threads ? "true" : "false");
    if (getenv("HCLIB_WORKERS") && bind_threads) {
        printf("WARNING: HCLIB_BIND_THREADS assign cores in round robin. E.g., "
               "setting HCLIB_WORKERS=12 on 2-socket node, each with 12 cores.\n");
    }
    printf(">>> HCLIB_STATS\t\t= %s\n", hclib_stats);
    printf("----------------------------------------\n");
}

void hclib_entrypoint() {
    if (hclib_stats) {
        hclib_display_runtime();
    }

    srand(0);

    hclib_context = (hc_context *)malloc(sizeof(hc_context));
    HASSERT(hclib_context);

    /*
     * Parse the platform description from the HPT configuration file and load
     * it into the hclib_context.
     */
    hclib_global_init();

    /*Initialize the datastructure for logging timeline events*/
    hclib_init_timeline();

    /* Create key to store per thread worker_state */
    if (pthread_key_create(&ws_key, NULL) != 0) {
        log_die("Cannot create ws_key for worker-specific data");
    }

    // Launch the worker threads
    if (hclib_stats) {
        printf("Using %d worker threads (including main thread)\n",
               hclib_context->nworkers);
    }

    // Start workers
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) {
        fprintf(stderr, "Error in pthread_attr_init\n");
        exit(3);
    }

    for (int i = 1; i < hclib_context->nworkers; i++) {
        if (pthread_create(&hclib_context->workers[i]->t, &attr, worker_routine,
                           &hclib_context->workers[i]->id) != 0) {
            fprintf(stderr, "Error launching thread\n");
            exit(4);
        }
    }
    set_current_worker(0);

    // allocate root finish
    hclib_start_finish();
}

void hclib_signal_join(int nb_workers) {
    int i;
    for (i = 0; i < nb_workers; i++) {
        hclib_context->done_flags[i].flag = 0;
    }
}

void hclib_join(int nb_workers) {
    // Join the workers
    LOG_DEBUG("hclib_join: nb_workers = %d\n", nb_workers);
    for (int i = 1; i < nb_workers; i++) {
        pthread_join(hclib_context->workers[i]->t, NULL);
    }
    LOG_DEBUG("hclib_join: finished\n");
}

void hclib_cleanup() {
    hc_hpt_cleanup(hclib_context); /* cleanup deques (allocated by hc mm) */
    pthread_key_delete(ws_key);

    /*Cleanup the datastructure for logging timeline events*/
    hclib_free_timeline();

    free(hclib_context);
}

static inline void check_in_finish(finish_t *finish) {
    if (finish) {
        // FIXME - does this need to be acquire, or can it be relaxed?
        _hclib_atomic_inc_acquire(&finish->counter);
    }
}

static inline void check_out_finish(finish_t *finish) {
    if (finish) {
        // was this the last async to check out?
        if (_hclib_atomic_dec_release(&finish->counter) == 0) {
#if HCLIB_LITECTX_STRATEGY
            HASSERT(!_hclib_promise_is_satisfied(finish->finish_deps[0]->owner));
            hclib_promise_put(finish->finish_deps[0]->owner, finish);
#endif /* HCLIB_LITECTX_STRATEGY */
        }
    }
}

static inline void execute_task(hclib_task_t *task) {
    finish_t *current_finish = task->current_finish;
    /*
     * Update the current finish of this worker to be inherited from the
     * currently executing task so that any asyncs spawned from the currently
     * executing task are registered on the same finish.
     */
    CURRENT_WS_INTERNAL->current_finish = current_finish;

    // task->_fp is of type 'void (*generic_frame_ptr)(void*)'
    LOG_DEBUG("execute_task: task=%p fp=%p\n", task, task->_fp);
    (task->_fp)(task->args);
    check_out_finish(current_finish);
    free(task);
}

static inline void rt_schedule_async(hclib_task_t *async_task,
                                     hclib_worker_state *ws) {
    LOG_DEBUG("rt_schedule_async: async_task=%p place=%p\n",
            async_task, async_task->place);

    ws->total_push++;
    // push on worker deq
    if (async_task->place) {
        deque_push_place(ws, async_task->place, async_task);
    } else {
        const int wid = get_current_worker();
        LOG_DEBUG("rt_schedule_async: scheduling on worker wid=%d "
                "hclib_context=%p\n", wid, hclib_context);
        if (!deque_push(&(hclib_context->workers[wid]->current->deque),
                        async_task)) {
            // TODO: deque is full, so execute in place
            printf("WARNING: deque full, local execution\n");
            execute_task(async_task);
        }
        LOG_DEBUG("rt_schedule_async: finished scheduling on worker wid=%d\n",
                wid);
    }
}

/*
 * A task which has no dependencies on prior tasks through promises is always
 * immediately ready for scheduling. A task that is registered on some prior
 * promises may be ready for scheduling if all of those promises have already been
 * satisfied. If they have not all been satisfied, the execution of this task is
 * registered on each, and it is only placed in a work deque once all promises have
 * been satisfied.
 */
static inline int is_eligible_to_schedule(hclib_task_t *async_task) {
    LOG_DEBUG("is_eligible_to_schedule: async_task=%p future_list=%p\n",
            async_task, async_task->future_list);
    if (async_task->future_list != NULL) {
        return register_on_all_promise_dependencies(async_task);
    } else {
        return 1;
    }
}

/*
 * If this async is eligible for scheduling, we insert it into the work-stealing
 * runtime. See is_eligible_to_schedule to understand when a task is or isn't
 * eligible for scheduling.
 */
void try_schedule_async(hclib_task_t *async_task, hclib_worker_state *ws) {
    MARK_BUSY(ws->id);
    if (is_eligible_to_schedule(async_task)) {
        rt_schedule_async(async_task, ws);
    }
}

void spawn_handler(hclib_task_t *task, place_t *pl, bool escaping) {

    HASSERT(task);

    hclib_worker_state *ws = CURRENT_WS_INTERNAL;
    if (!escaping) {
        check_in_finish(ws->current_finish);
        task->current_finish = ws->current_finish;
        HASSERT(task->current_finish != NULL);
    } else {
        // If escaping task, don't register with current finish
        HASSERT(task->current_finish == NULL);
    }

    LOG_DEBUG("spawn_handler: task=%p\n", task);

    try_schedule_async(task, ws);
}

void spawn_at_hpt(place_t *pl, hclib_task_t *task) {
    // get current worker
    hclib_worker_state *ws = CURRENT_WS_INTERNAL;
    check_in_finish(ws->current_finish);
    task->current_finish = ws->current_finish;
    task->place = pl;
    try_schedule_async(task, ws);
}

void spawn(hclib_task_t *task) {
    spawn_handler(task, NULL, false);
}

void spawn_escaping(hclib_task_t *task, hclib_future_t **future_list) {
    spawn_handler(task, NULL, true);
}

void spawn_escaping_at(place_t *pl, hclib_task_t *task,
                       hclib_future_t **future_list) {
    spawn_handler(task, pl, true);
}

void spawn_await_at(hclib_task_t *task, hclib_future_t **future_list,
                    place_t *pl) {
    // FIXME - the future_list member may not have been properly
    // initialized on this call path from C++ code (fix later)
    task->future_list = future_list;
    spawn_handler(task, pl, false);
}

void spawn_await(hclib_task_t *task, hclib_future_t **future_list) {
    spawn_await_at(task, future_list, NULL);
}

void find_and_run_task(hclib_worker_state *ws) {
    hclib_task_t *task = hpt_pop_task(ws);
    if (!task) {
        while (hclib_context->done_flags[ws->id].flag) {
            // try to steal
            task = hpt_steal_task(ws);
            if (task) {
		hclib_log_event(ws->id, SUCCESS_STEALS);
		ws->total_steals++;
                MARK_BUSY(ws->id);
                break;
            }
        }
    }

    if (task) {
        execute_task(task);
    }
}

#if HCLIB_LITECTX_STRATEGY
static void _hclib_finalize_ctx(LiteCtx *ctx) {
    hclib_end_finish();
    // Signal shutdown to all worker threads
    hclib_signal_join(hclib_context->nworkers);
    // Jump back to the system thread context for this worker
    ctx_swap(ctx, CURRENT_WS_INTERNAL->root_ctx, __func__);
    HASSERT(0); // Should never return here
}

static void core_work_loop(void) {
    uint64_t wid;
    do {
        hclib_worker_state *ws = CURRENT_WS_INTERNAL;
        wid = (uint64_t)ws->id;
        find_and_run_task(ws);
    } while (hclib_context->done_flags[wid].flag);

    // Jump back to the system thread context for this worker
    hclib_worker_state *ws = CURRENT_WS_INTERNAL;
    HASSERT(ws->root_ctx);
    ctx_swap(get_curr_lite_ctx(), ws->root_ctx, __func__);
    HASSERT(0); // Should never return here
}

static void crt_work_loop(LiteCtx *ctx) {
    core_work_loop(); // this function never returns
    HASSERT(0); // Should never return here
}

/*
 * With the addition of lightweight context switching, worker creation becomes a
 * bit more complicated because we need all task creation and finish scopes to
 * be performed from beneath an explicitly created context, rather than from a
 * pthread context. To do this, we start worker_routine by creating a proxy
 * context to switch from and create a lightweight context to switch to, which
 * enters crt_work_loop immediately, moving into the main work loop, eventually
 * swapping back to the proxy task
 * to clean up this worker thread when the worker thread is signaled to exit.
 */
static void *worker_routine(void *args) {
    const int wid = *((int *)args);
    set_current_worker(wid);
    hclib_worker_state *ws = CURRENT_WS_INTERNAL;

    // Create proxy original context to switch from
    LiteCtx *currentCtx = LiteCtx_proxy_create(__func__);
    ws->root_ctx = currentCtx;

    /*
     * Create the new proxy we will be switching to, which will start with
     * crt_work_loop at the top of the stack.
     */
    LiteCtx *newCtx = LiteCtx_create(crt_work_loop);
    newCtx->arg = args;

    // Swap in the newCtx lite context
    ctx_swap(currentCtx, newCtx, __func__);

    LOG_DEBUG("worker_routine: worker %d exiting, cleaning up proxy %p "
            "and lite ctx %p\n", get_current_worker(), currentCtx, newCtx);

    // free resources
    LiteCtx_destroy(currentCtx->prev);
    LiteCtx_proxy_destroy(currentCtx);
    return NULL;
}

#else /* default (broken) strategy */

static void *worker_routine(void *args) {
    const int wid = *((int *) args);
    set_current_worker(wid);

    hclib_worker_state *ws = CURRENT_WS_INTERNAL;

    while (hclib_context->done_flags[wid].flag) {
        find_and_run_task(ws);
    }

    return NULL;
}
#endif /* HCLIB_LITECTX_STRATEGY */

#if HCLIB_LITECTX_STRATEGY
static void _finish_ctx_resume(void *arg) {
    LiteCtx *currentCtx = get_curr_lite_ctx();
    LiteCtx *finishCtx = arg;
    ctx_swap(currentCtx, finishCtx, __func__);

    LOG_DEBUG("Should not have reached here, currentCtx=%p "
            "finishCtx=%p\n", currentCtx, finishCtx);
    HASSERT(0);
}

// Based on _help_finish_ctx
void _help_wait(LiteCtx *ctx) {
    hclib_future_t **continuation_deps = ctx->arg;
    LiteCtx *wait_ctx = ctx->prev;

    // reusing _finish_ctx_resume
    hclib_async(_finish_ctx_resume, wait_ctx, continuation_deps,
            NO_PHASER, ANY_PLACE, ESCAPING_ASYNC);

    core_work_loop();
    HASSERT(0);
}

void *hclib_future_wait(hclib_future_t *future) {
    if (_hclib_promise_is_satisfied(future->owner)) {
        return future->owner->datum;
    }

    // save current finish scope (in case of worker swap)
    finish_t *current_finish = CURRENT_WS_INTERNAL->current_finish;

    hclib_future_t *continuation_deps[] = { future, NULL };
    LiteCtx *currentCtx = get_curr_lite_ctx();
    HASSERT(currentCtx);
    LiteCtx *newCtx = LiteCtx_create(_help_wait);
    newCtx->arg = continuation_deps;
    ctx_swap(currentCtx, newCtx, __func__);
    LiteCtx_destroy(currentCtx->prev);

    // restore current finish scope (in case of worker swap)
    CURRENT_WS_INTERNAL->current_finish = current_finish;

    HASSERT(_hclib_promise_is_satisfied(future->owner) &&
            "promise must be satisfied before returning from wait");
    return future->owner->datum;
}

static void _help_finish_ctx(LiteCtx *ctx) {
    /*
     * Set up previous context to be stolen when the finish completes (note that
     * the async must ESCAPE, otherwise this finish scope will deadlock on
     * itself).
     */
    LOG_DEBUG("_help_finish_ctx: ctx = %p, ctx->arg = %p\n", ctx, ctx->arg);
    finish_t *finish = ctx->arg;
    LiteCtx *hclib_finish_ctx = ctx->prev;

    /*
     * Create an async to handle the continuation after the finish, whose state
     * is captured in hclib_finish_ctx and whose execution is pending on
     * finish->finish_deps.
     */
    hclib_async(_finish_ctx_resume, hclib_finish_ctx, finish->finish_deps,
            NO_PHASER, ANY_PLACE, ESCAPING_ASYNC);

    /*
     * The main thread is now exiting the finish (albeit in a separate context),
     * so check it out.
     */
    check_out_finish(finish);

    // keep work-stealing until this context gets swapped out and destroyed
    core_work_loop(); // this function never returns
    HASSERT(0); // we should never return here
}
#else /* default (broken) strategy */

static inline void slave_worker_finishHelper_routine(finish_t *finish) {
    hclib_worker_state *ws = CURRENT_WS_INTERNAL;

    while (_hclib_atomic_load_relaxed(&finish->counter) > 0) {
        // try to pop
        hclib_task_t *task = hpt_pop_task(ws);
        if (!task) {
            while (_hclib_atomic_load_relaxed(&finish->counter) > 0) {
                // try to steal
                task = hpt_steal_task(ws);
                if (task) {
                    ws->total_steals++;
                    MARK_BUSY(ws->id);
                    break;
                }
            }
        }
        if (task) {
            execute_task(task);
        }
    }
}

static void _help_finish(finish_t *finish) {
    slave_worker_finishHelper_routine(finish);
}

#endif /* HCLIB_???_STRATEGY */

void help_finish(finish_t *finish) {
    // This is called to make progress when an end_finish has been
    // reached but it hasn't completed yet.
    // Note that's also where the master worker ends up entering its work loop

#if HCLIB_THREAD_BLOCKING_STRATEGY
#error Thread-blocking strategy is not yet implemented
#elif HCLIB_LITECTX_STRATEGY
    {
        /*
         * Creating a new context to switch to is necessary here because the
         * current context needs to become the continuation for this finish
         * (which will be switched back to by _finish_ctx_resume, for which an
         * async is created inside _help_finish_ctx).
         */

        // TODO - should only switch contexts after actually finding work

        // Try to execute a sub-task of the current finish scope
        do {
            hclib_worker_state *ws = CURRENT_WS_INTERNAL;
            hclib_task_t *task = hpt_pop_task(ws);
            // Fall through if we have no local tasks.
            if (!task) {
                break;
            }
            // Since the current finish scope is not yet complete,
            // there's a good chance that the task at the top of the
            // deque is a task from the current finish scope.
            // It's safe to continue executing sub-tasks on the current
            // stack, since the finish scope blocks on them anyway.
            else if (task->current_finish == finish) {
                execute_task(task); // !!! May cause a worker-swap!!!
            }
            // For tasks in a different finish scope, we need a new context.
            // FIXME: Figure out a better way to handle this!
            // For now, just put it back in the deque and fall through.
            else {
                deque_push_place(ws, NULL, task);
                break;
            }
        } while (_hclib_atomic_load_relaxed(&finish->counter) > 1);

        // Someone stole our last task...
        // Create a new context to do other work,
        // and suspend this finish scope pending on the outstanding tasks.
        if (_hclib_atomic_load_relaxed(&finish->counter) > 1) {
            // create finish event
            hclib_promise_t *finish_promise = hclib_promise_create();
            hclib_future_t *finish_deps[] = { &finish_promise->future, NULL };
            finish->finish_deps = finish_deps;

            LiteCtx *currentCtx = get_curr_lite_ctx();
            HASSERT(currentCtx);
            LiteCtx *newCtx = LiteCtx_create(_help_finish_ctx);
            newCtx->arg = finish;

            LOG_DEBUG("help_finish: newCtx = %p, newCtx->arg = %p\n", newCtx, newCtx->arg);
            ctx_swap(currentCtx, newCtx, __func__);

            // note: the other context checks out of the current finish scope

            // destroy the context that resumed this one since it's now defunct
            // (there are no other handles to it, and it will never be resumed)
            LiteCtx_destroy(currentCtx->prev);
            hclib_promise_free(finish_promise);
        } else {
            HASSERT(_hclib_atomic_load_relaxed(&finish->counter) == 1);
            // finish->counter == 1 implies that all the tasks are done
            // (it's only waiting on itself now), so just return!
            _hclib_atomic_dec_acq_rel(&finish->counter);
        }
    }
#else /* default (broken) strategy */
    // FIXME - do I need to decrement the finish counter here?
    _help_finish(finish);
#endif /* HCLIB_???_STRATEGY */

    HASSERT(_hclib_atomic_load_relaxed(&finish->counter) == 0);

}
/*
 * =================== INTERFACE TO USER FUNCTIONS ==========================
 */

void hclib_start_finish() {
    hclib_worker_state *ws = CURRENT_WS_INTERNAL;
    finish_t *finish = malloc(sizeof(*finish));
    HASSERT(finish);
    finish->parent = ws->current_finish;
#if HCLIB_LITECTX_STRATEGY
    /*
     * Set finish counter to 1 initially to emulate the main thread inside the
     * finish being a task registered on the finish. When we reach the
     * corresponding end_finish we set up the finish_deps for the continuation
     * and then decrement the counter from the main thread. This ensures that
     * anytime the counter reaches zero, it is safe to do a promise_put on the
     * finish_deps. If we initialized counter to zero here, any async inside the
     * finish could start and finish before the main thread reaches the
     * end_finish, decrementing the finish counter to zero when it completes.
     * This would make it harder to detect when all tasks within the finish have
     * completed, or just the tasks launched so far.
     */
    finish->finish_deps = NULL;
    check_in_finish(finish->parent); // check_in_finish performs NULL check
    _hclib_atomic_store_release(&finish->counter, 1);
#else
    finish->counter = 0;
    if(finish->parent) {
        check_in_finish(finish->parent); // check_in_finish performs NULL check
    }
#endif
    ws->current_finish = finish;
}

void hclib_end_finish() {
    finish_t *current_finish = CURRENT_WS_INTERNAL->current_finish;

#if HCLIB_LITECTX_STRATEGY
    HASSERT(_hclib_atomic_load_relaxed(&current_finish->counter) > 0);
    help_finish(current_finish);
    HASSERT(_hclib_atomic_load_relaxed(&current_finish->counter) == 0);
    check_out_finish(current_finish->parent); // NULL check in check_out_finish
    // Don't reuse worker-state! (we might not be on the same worker anymore)
#else
    if(_hclib_atomic_load_relaxed(&current_finish->counter) > 0) {
        help_finish(current_finish);
    }
    HASSERT(_hclib_atomic_load_relaxed(&current_finish->counter) == 0);
    if(current_finish->parent) {
	check_out_finish(current_finish->parent);
    }
#endif
    CURRENT_WS_INTERNAL->current_finish = current_finish->parent;
    free(current_finish);
}

// Based on help_finish
void hclib_end_finish_nonblocking_helper(hclib_promise_t *event) {
    finish_t *current_finish = CURRENT_WS_INTERNAL->current_finish;

    HASSERT(_hclib_atomic_load_relaxed(&current_finish->counter) > 0);

    // NOTE: this is a nasty hack to avoid a memory leak here.
    // Previously we were allocating a two-element array of
    // futures here, but there was no good way to free it...
    // Since the promise datum is null until the promise is satisfied,
    // we can use that as the null-terminator for our future list.
#ifdef HCLIB_LITECTX_STRATEGY
    hclib_future_t **finish_deps = (hclib_future_t**)&event->future.owner;
#endif
    HASSERT_STATIC(sizeof(event->future) == sizeof(event->datum) &&
            offsetof(hclib_promise_t, future) == 0 &&
            offsetof(hclib_promise_t, datum) == sizeof(hclib_future_t*),
            "ad-hoc null terminator is correctly aligned in promise struct");
    HASSERT(event->datum == NULL && UNINITIALIZED_PROMISE_DATA_PTR == NULL &&
            "ad-hoc null terminator must have value NULL");
#ifdef HCLIB_LITECTX_STRATEGY
    current_finish->finish_deps = finish_deps;
#endif
    // Check out this "task" from the current finish
    check_out_finish(current_finish);

    // Check out the current finish from its parent
    check_out_finish(current_finish->parent);
    CURRENT_WS_INTERNAL->current_finish = current_finish->parent;
}

hclib_future_t *hclib_end_finish_nonblocking() {
    hclib_promise_t *event = hclib_promise_create();
    hclib_end_finish_nonblocking_helper(event);
    return &event->future;
}

int hclib_num_workers() {
    return hclib_context->nworkers;
}

void hclib_gather_comm_worker_stats(int *push_outd, int *push_ind,
                                    int *steal_ind) {
    assert(0);
}

double mysecond() {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return tv.tv_sec + ((double) tv.tv_usec / 1000000);
}

void runtime_statistics(double duration) {
    int asyncPush=0, steals=0;
    hclib_worker_state** workers = CURRENT_WS_INTERNAL->context->workers;
    for(int i=0; i<hclib_num_workers(); i++) {
        asyncPush += workers[i]->total_push;
        steals += workers[i]->total_steals;
    }
    double time_stats[HCLIB_NSTATES];
    hclib_get_avg_time(time_stats);

    hclib_prettyprint_timeline_events();

    perfcounters_finalize();
    printf("============================ MMTk Statistics Totals ============================\n");
    printf("time.kernel\ttotalPush\ttotalSteals");
    for(int i=0; i<HCLIB_NSTATES; i++) {
        printf("\t%s",TIME_STATS[i]);
    }
    printf("\n%.3f\t%d\t%d",user_specified_timer,asyncPush,steals);
    for(int i=0; i<HCLIB_NSTATES; i++) {
        printf("\t%.2f",time_stats[i]);
    }
    printf("\nTotal time: %.3f ms\n",duration);
    printf("------------------------------ End MMTk Statistics -----------------------------\n");
    printf("===== TEST PASSED in %.3f msec =====\n",duration);
}

static void show_stats_header() {
    printf("\n");
    printf("-----\n");
    printf("mkdir timedrun fake\n");
    printf("\n");
    printf("-----\n");
    benchmark_start_time_stats = mysecond();
}

void hclib_user_harness_timer(double dur) {
    user_specified_timer = dur;
}

void showStatsFooter() {
    double end = mysecond();
    HASSERT(benchmark_start_time_stats != 0);
    double dur = (end-benchmark_start_time_stats)*1000;
    runtime_statistics(dur);
}

void zero_initialize_counters() {
    hclib_worker_state** workers = CURRENT_WS_INTERNAL->context->workers;
    for(int i=0; i<hclib_num_workers(); i++) {
        workers[i]->total_steals = 0;
        workers[i]->total_push= 0;
    }
}

void hclib_kernel(generic_frame_ptr fct_ptr, void *arg) {
    zero_initialize_counters();
    perfcounters_start();
    perfcounters_start_energy();
    double start = mysecond();
    // init timer stats
    hclib_initStats(hclib_context->nworkers);
    hclib_start_timeline();
    (*fct_ptr)(arg);
    hclib_stop_timeline();
    hclib_stopStats();
    user_specified_timer = (mysecond() - start)*1000;
    perfcounters_read_energy();
    printf("Kernel__Energy___ -->  %f\n", (LAST_PWR_PKG_ENERGY[0]+LAST_PWR_PKG_ENERGY[1])*JOULE_UNIT);
    perfcounters_stop_energy(); 
    // perfcounters_finalize_energy();
    perfcounters_stop();
}

/*
 * Main entrypoint for runtime initialization, this function must be called by
 * the user program before any HC actions are performed.
 */
static void hclib_init() {
    HASSERT(hclib_stats == NULL);
    HASSERT(bind_threads == -1);
    hclib_stats = getenv("HCLIB_STATS");
    bind_threads = (getenv("HCLIB_BIND_THREADS") != NULL);

    if (hclib_stats) {
        show_stats_header();
    }

    const char *hpt_file = getenv("HCLIB_HPT_FILE");
    if (hpt_file == NULL) {
        //fprintf(stderr, "WARNING: Running without a provided HCLIB_HPT_FILE, "
        //        "will make a best effort to generate a default HPT.\n");
    }

    hclib_entrypoint();
}


static void hclib_finalize() {
#if HCLIB_LITECTX_STRATEGY
    LiteCtx *finalize_ctx = LiteCtx_proxy_create(__func__);
    LiteCtx *finish_ctx = LiteCtx_create(_hclib_finalize_ctx);
    CURRENT_WS_INTERNAL->root_ctx = finalize_ctx;
    ctx_swap(finalize_ctx, finish_ctx, __func__);
    // free resources
    LiteCtx_destroy(finalize_ctx->prev);
    LiteCtx_proxy_destroy(finalize_ctx);
#else /* default (broken) strategy */
    hclib_end_finish();
    hclib_signal_join(hclib_context->nworkers);
#endif /* HCLIB_LITECTX_STRATEGY */

    if (hclib_stats) {
        showStatsFooter();
    }

    hclib_join(hclib_context->nworkers);
    hclib_cleanup();
}

/**
 * @brief Initialize and launch HClib runtime.
 * Implicitly defines a global finish scope.
 * Returns once the computation has completed and the runtime has been
 * finalized.
 *
 * With fibers, using hclib_launch is a requirement for any HC program. All
 * asyncs/finishes must be performed from beneath hclib_launch. Ensuring that
 * the parent of any end finish is a fiber means that the runtime can assume
 * that the current parent is a fiber, and therefore its lifetime is already
 * managed by the runtime. If we allowed both system-managed threads (i.e. the
 * main thread) and fibers to reach end-finishes, we would have to know to
 * create a LiteCtx from the system-managed stacks and save them, but to not do
 * so when the calling context is already a LiteCtx. While this could be
 * supported, this introduces unnecessary complexity into the runtime code. It
 * is simpler to use hclib_launch to ensure that finish scopes are only ever
 * reached from a fiber context, allowing us to assume that it is safe to simply
 * swap out the current context as a continuation without having to check if we
 * need to do extra work to persist it.
 */

void hclib_launch(generic_frame_ptr fct_ptr, void *arg) {
    hclib_init();
    perfcounters_init();
    perfcounters_init_energy(); // call once
    // perfcounters_start_energy();
#ifdef HCLIB_LITECTX_STRATEGY
    hclib_async(fct_ptr, arg, NO_FUTURE, NO_PHASER, ANY_PLACE, NO_PROP);
#else
    fct_ptr(arg);
#endif
    // perfcounters_read_energy();
    // printf("__Energy___ -->  %f\n", (LAST_PWR_PKG_ENERGY[0]+LAST_PWR_PKG_ENERGY[1])*JOULE_UNIT);
    // perfcounters_stop_energy(); 
    // perfcounters_finalize_energy();
    hclib_finalize();
    perfcounters_finalize_energy();
}

