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

/*
 * push an entry onto the tail of the deque
 */
int deque_push(deque_t *deq, hclib_task_t *entry) {
    /* int tail = _hclib_atomic_load_relaxed(&deq->tail);
    int head = _hclib_atomic_load_relaxed(&deq->head); */
    int tail = deq->tail;
    deq->data[tail%INIT_DEQUE_CAPACITY] = entry;
    deq->tail = tail+1;
    return 1;
}

/*
 * the steal protocol
 */
hclib_task_t *deque_steal(deque_t *deq) {
    int delta = 22; //Store buffer/2
    int head;
    int tail;
    while(true){
        head = deq->head;
        tail = deq->tail;
        if (head>tail || head==tail)
            return NULL;
        if(tail-delta< head || tail-delta==head)
            return NULL;
        hclib_task_t *t = deq->data[head % INIT_DEQUE_CAPACITY];
        if (! _hclib_atomic_cas_acq_rel(&deq->head, head, head+1))
            continue;
        return t;
    }
}

/*
 * pop the task out of the deque from the tail
 */
hclib_task_t *deque_pop(deque_t *deq) {
    int tail = deq->tail - 1;
    deq->tail = tail;
    int head = deq->head;

    if(tail>head){
        //Thief will observe t and will not try to steal task t
        return deq->data[tail % INIT_DEQUE_CAPACITY];
    }
    if(tail<head){
        deq->tail = head;
        return NULL;
    }
    //tail == head
    deq->tail = head +1;
    if(! _hclib_atomic_cas_acq_rel(&deq->head, head, head+1))
        return NULL;
    else
        return deq->data[tail % INIT_DEQUE_CAPACITY];
}

