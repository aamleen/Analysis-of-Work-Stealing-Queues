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

#ifndef HCLIB_TIMER_H_
#define HCLIB_TIMER_H_

/*
 * Comment this if you don't want timing analysis
 */
#define _TIMER_ON_

/* Search status */
#define STATUS_HAVEWORK 0
#define STATUS_TERM     1

/* Search states */
#define HCLIB_WORK    0
#define HCLIB_SEARCH  1
/* New States can be added ONLY at this location
 * i.e., in between HCLIB_SEARCH and HCLIB_IDLE.
 * For every new state: 
 * a) increment the count in HCLIB_NSTATES below
 * b) append name of new states in string array below
 */
#define HCLIB_IDLE    2
#define HCLIB_NSTATES 3

const char static TIME_STATS[HCLIB_NSTATES][50] = {"tWork", "tSearch", "tIdle"};

void hclib_initStats  (int numWorkers);
void hclib_stopStats();
void hclib_setState   (int wid, int state);
void hclib_get_avg_time (double* time_stats);

#define MARK_BUSY(w)	hclib_setState(w, HCLIB_WORK);
#define MARK_SEARCH(w)	hclib_setState(w, HCLIB_SEARCH);
#define MARK_IDLE(w)	hclib_setState(w, HCLIB_IDLE);

#endif /* HCLIB_TIMER_H_ */
