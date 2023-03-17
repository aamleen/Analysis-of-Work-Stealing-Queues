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

#include "hclib-internal.h"
#include "hclib-atomics.h"
#include <unistd.h>

/*
 * push an entry onto the tail of the deque
 */
int deque_push(deque_t *deq, hclib_task_t *entry) {
    /* int tail = _hclib_atomic_load_relaxed(&deq->tail);
    int head = _hclib_atomic_load_relaxed(&deq->head); */
    // printf("HELLO");
    int tail = _hclib_atomic_load_relaxed(&deq->tail);
    // int tail = deq->tail;
    deq->data[tail%INIT_DEQUE_CAPACITY] = entry;
    _hclib_atomic_inc_release(&deq->tail);
    return 1;
}

/*
 * the steal protocol
 */
hclib_task_t *deque_steal(deque_t *deq) {
    /* _hclib_atomic_inc_relaxed(&deq->counter);
    if(deq->counter >19){
        deq->counter=0;
        sleep(0.00000001);
    } */
    int head;
    int tail;
    //int thiefid = rand();
    //prinntf("Stealing \n");
    // printf("%d %d\n", delta, ff);
    while(true){
        head = deq->head;
        tail = deq->tail;
        if (head>=tail){
            //printf("\nHead>Tail");
            return NULL;
        } 
        // HCLIB START HERE
        if(tail-delta <= head){  
            return NULL;
        }
        // HCLIB END HERE
        hclib_task_t *t = deq->data[head % INIT_DEQUE_CAPACITY];
        //prinntf("Task stolen by %d thief\n",thiefid);
        if (! _hclib_atomic_cas_acq_rel(&deq->head, head, head+1))
            continue;
        return t;
    }
}

/*
 * pop the task out of the deque from the tail
 */
hclib_task_t *deque_pop(deque_t *deq) {
    int tail = _hclib_atomic_dec_relaxed(&deq->tail);
    int head = _hclib_atomic_load_relaxed(&deq->head);

    int size = tail - head;
    if (size < 0) {
        _hclib_atomic_store_relaxed(&deq->tail, head);
        return NULL;
    }
    hclib_task_t *t = deq->data[tail % INIT_DEQUE_CAPACITY];

    if (size > 0) {
        return t;
    }

    /* now the deque appears empty */
    /* I need to compete with the thieves for the last task */
    //@- if (!hc_cas(&deq->head, head, head + 1)) {
    if (!_hclib_atomic_cas_acq_rel(&deq->head, head, head + 1)) {
        t = NULL;
    }

    _hclib_atomic_inc_relaxed(&deq->tail);

    return t;
}

