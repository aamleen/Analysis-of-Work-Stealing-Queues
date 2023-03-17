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
#include <stdio.h>
#include <unistd.h>
/*
 * push an entry onto the tail of the deque
 */

/* void expand(deque_t *deq){
    int capacity=INIT_DEQUE_CAPACITY;
    hclib_task_t* temp[2*capacity];
    for(int i=0;i<capacity;i++){
        temp[i]=deq->data[i];
    }
    *(deq->data)=temp;
    INIT_DEQUE_CAPACITY=capacity*2;
} */

int deque_push(deque_t *deq, hclib_task_t *entry) {
    long unsigned anchor ;
    //anchor = _hclib_atomic_load_relaxed(&deq->anchor);
    anchor = deq->anchor;
    long int head=anchor & 0xFFFF;
    long int size=(anchor >> 16) & 0xFFFF;
    long int tag=(anchor >> 32) & 0xFFFF;
    int capacity= INIT_DEQUE_CAPACITY;
    if(size==capacity){
        return 0;
    }
    else{
        deq->data[(head+size)%capacity] = entry;
        size+=1;
        tag+=1;
        anchor=head | (size<<16) | (tag<<32);
        //_hclib_atomic_store_release(&deq->anchor,anchor);
        deq->anchor =anchor;
    }
    return 1;
}

/*
 * the steal protocol
 */
hclib_task_t *deque_steal(deque_t *deq) {
    long unsigned anchor;
    //anchor = _hclib_atomic_load_acquire(&deq->anchor);
    anchor = deq->anchor;
    long int head=anchor & 0xFFFF;
    long int size=(anchor >> 16) & 0xFFFF;
    long int tag=(anchor >> 32) & 0xFFFF;
    int capacity= INIT_DEQUE_CAPACITY;
    
    if(size == 0){
        //printf("Deque is empty while Stealing\n");
        return NULL;
    }
    hclib_task_t* task = deq->data[head%capacity];
    long int head2 = (head+1)%capacity;
    long unsigned anchor_desired= (head2) | ((size-1)<<16) | (tag<<32);

    if(_hclib_atomic_cas_acq_rel(&task->state,0,1)){
        if(_hclib_atomic_cas_acq_rel(&deq->anchor,anchor,anchor_desired)){
            return task;
        }
        else
            task->state = 0;
    }
    return NULL;
}

/*
 * pop the task out of the deque from the tail
 */
hclib_task_t *deque_pop(deque_t *deq) {
    long unsigned anchor;
    //anchor = _hclib_atomic_load_acquire(&deq->anchor);
    anchor = deq->anchor;
    long int head=anchor & 0xFFFF;
    long int size=(anchor >> 16) & 0xFFFF;
    long int tag=(anchor >> 32) & 0xFFFF;
    int capacity= INIT_DEQUE_CAPACITY;
    if(size == 0){
        return NULL;
    }
    hclib_task_t* task = deq->data[(head+size-1)%capacity];
    anchor=(head) | ((size-1)<<16) | (tag<<32);
        
    if(_hclib_atomic_cas_acq_rel(&task->state,0,1)){
        _hclib_atomic_store_release(&deq->anchor,anchor);
        deq->anchor = anchor;
	    return task;
    }
    return NULL; 
}