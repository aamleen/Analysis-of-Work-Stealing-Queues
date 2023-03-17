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

    //printf("Pushing Starting\n");
    int anchor = deq->anchor;
    int tail=anchor & 0xFFFF;
    int tag=(anchor >> 16) & 0xFFFF;
    int capacity= INIT_DEQUE_CAPACITY;
    if(tail>=capacity){
        //printf("Found full deque while owner pushing with tail%d\n",tail);
        /* expand(deq);
        deque_push(deq,entry); */
        return 0;
    }
    else{
        deq->data[tail] = entry;
        tail+=1;
        tag+=1;
        anchor=tail | (tag<<16);

       deq->anchor=anchor;
    }
    //printf("Pushed in, tail: %d\n",tail+1);
    return 1;
}

/*
 * the steal protocol
 */
hclib_task_t *deque_steal(deque_t *deq) {
    int anchor = deq->anchor;
    int tail=anchor & 0xFFFF;
    int tag=(anchor >> 16) & 0xFFFF;
    
    //printf("Stealing started\n");
    if(tail == 0){
        //printf("Deque is empty while Stealing\n");
        return NULL;
    }
    hclib_task_t* task = deq->data[tail-1];

    //printf("TAKEN\n");
    int anchor_expected=(tail-1) | (tag<<16);
    if(_hclib_atomic_cas_acq_rel(&deq->anchor,anchor,anchor_expected))
        return task;
    //printf("CAS Failed\n");
    /* if( _hclib_atomic_cas_acq_rel(&deq->tail,tag,tag) && _hclib_atomic_cas_acq_rel(&deq->tail,tail,tail-1)){
        //printf("Steal successful tail: %d\n",tail-1);
            return task;
        } */
    /* if(_hclib_atomic_cas_acq_rel(&task->state,0,1)){
        
    } */
    return deque_steal(deq);
}

/*
 * pop the task out of the deque from the tail
 */
hclib_task_t *deque_pop(deque_t *deq) {
    int anchor = deq->anchor;
    int tail=anchor & 0xFFFF;
    int tag=(anchor >> 16) & 0xFFFF;
    if(tail == 0){
        //printf("Empty deque while owner taking out task\n");
        return NULL;
    }
    hclib_task_t* task = deq->data[tail-1];
    //printf("Owner got the task out tail: %d\n",tail-1);
    anchor=(tail-1) | (tag<<16);
    //_hclib_atomic_dec_relaxed(&deq->anchor);
    
    deq->anchor = anchor;
    return task;
}

