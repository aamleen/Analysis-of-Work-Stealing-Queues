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

#define delta 2
/*
 * push an entry onto the tail of the deque
 */
int deque_push(deque_t *deq, hclib_task_t *entry) {
    /* int tail = _hclib_atomic_load_relaxed(&deq->tail);
    int head = _hclib_atomic_load_relaxed(&deq->head); */
    // printf("HELLO");
    int tail = deq->tail;
    deq->data[tail%INIT_DEQUE_CAPACITY] = entry;
    deq->tail = tail+1;
    return 1;
}

hclib_task_t *hclib_deque_steal(deque_t *deq) {
    int head;
    /* Cannot read deq->data[head] here
     * Can happen that head=tail=0, then the owner of the deq pushes
     * a new task when stealer is here in the code, resulting in head=0, tail=1
     * All other checks down-below will be valid, but the old value of the buffer head
     * would be returned by the steal rather than the new pushed value.
     */
    int tail;

    head = _hclib_atomic_load_relaxed(&deq->head);
    // ATOMIC: load acquire
    // We want all the writes from the producing thread to read the task data
    // and we're using the tail as the synchronization variable
    tail = _hclib_atomic_load_acquire(&deq->tail);
    if ((tail - head) <= 0) {
        return NULL;
    }

    hclib_task_t *t = deq->data[head % INIT_DEQUE_CAPACITY];
    /* compete with other thieves and possibly the owner (if the size == 1) */
    if (_hclib_atomic_cas_acq_rel(&deq->head, head, head + 1)) { /* competing */
        return t;
    }
    return NULL;
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
    while(true){
        head = deq->head;
        tail = deq->tail;
        if (head>=tail){
            //prinntf("\nHead>Tail by %d thief",thiefid);
            return NULL;
        } 
        if(tail-delta <= head){ 
            //hclib_task_t *t = deq->data[head % INIT_DEQUE_CAPACITY];
            /* compete with other thieves and possibly the owner (if the size == 1) */
            //if (_hclib_atomic_cas_acq_rel(&deq->head, head, head + 1)) { /* competing */
            //    return t;
            //}
            //return NULL;
            return hclib_deque_steal(deq);
        }
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
    int tail = deq->tail - 1;
    deq->tail = tail;
    int head = _hclib_atomic_load_relaxed(&deq->head);

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

