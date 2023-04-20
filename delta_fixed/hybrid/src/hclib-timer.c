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
 *
 *
 * The source code in this file is based on similar code
 * provided as part of the UTS (Unbalanced Tree Search) benchmark:
 *
 * https://sourceforge.net/p/uts-benchmark/code/ci/722e89/tree/stats.c
 *
 * Please see the accompanying NOTICE file for license details.
 */

#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include "hclib-timer.h"

#ifdef _TIMER_ON_
typedef struct stats_t {
    double time[HCLIB_NSTATES];	/* Time spent in each state */
    double timeLast;
    int    entries[HCLIB_NSTATES]; /* Num sessions of each state */
    int    curState;
} stats_t;

static stats_t *status;
static int numWorkers = -1;
#endif
double avgtime_nstates[HCLIB_NSTATES];

volatile static int timer_started = 0;

inline double wctime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec + 1E-6 * tv.tv_usec);
}

void hclib_initStats(int nw) {
#ifdef _TIMER_ON_
    numWorkers = nw;
    status = (stats_t*) malloc(sizeof(stats_t) * numWorkers);
    for(int i=0; i<numWorkers; i++) {
        status[i].timeLast = wctime();
        status[i].curState = HCLIB_IDLE;
        for (int j = 0; j < HCLIB_NSTATES; j++) {
            status[i].time[j] = 0.0;
            status[i].entries[j] = 0;
        }
    }
    timer_started = 1;
    __sync_synchronize();
#endif
}

/* Change states */
void hclib_setState(int wid, int state) {
#ifdef _TIMER_ON_
    if(!timer_started) return;
    double time;
    if (state < 0 || state >= HCLIB_NSTATES) {
        printf("ERROR: hclib_setState: thread state out of range");
        exit(-1);
    }
    if (state == status[wid].curState)
        return;

    time = wctime();
    status[wid].time[status[wid].curState] +=  time - status[wid].timeLast;
    status[wid].entries[state]++;
    status[wid].timeLast = time;
    status[wid].curState = state;
#endif
}

void hclib_stopStats() {
    timer_started = 0;
    __sync_synchronize();
}

void hclib_get_avg_time(double* stats) {
#ifdef _TIMER_ON_
    int start = 0;
    int total = numWorkers;
    for(int j=0; j<HCLIB_NSTATES; j++) {
        avgtime_nstates[j] = 0;
        for(int i = start; i<numWorkers; i++) {
            avgtime_nstates[j] += status[i].time[j];
        }
        avgtime_nstates[j] = avgtime_nstates[j] / total;
    }
    double ttotal=0.0;
    for(int j=0; j<HCLIB_NSTATES; j++) {
        ttotal += avgtime_nstates[j];
    }
    for(int j=0; j<HCLIB_NSTATES; j++) {
        stats[j] = avgtime_nstates[j] * 100 / ttotal;
    }
#else
    for(int j=0; j<HCLIB_NSTATES; j++) {
        stats[j] = 0;
    }
#endif
}


