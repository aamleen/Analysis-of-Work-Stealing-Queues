#include <hclib-internal.h>
#include <assert.h>
#include <hclib-timeline.h>
#include <sys/time.h>
#include <stdlib.h>

#ifdef HCLIB_TIMELINE
logger_t*** loginfo;
logger_t*** loginfo_head;
volatile int logger_started = 0;
volatile int logger_stopped = 0;
double start_cycles = 0;
double end_cycles = 0;

inline double wctime() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec + 1E-6 * tv.tv_usec);
}

/* Called at the start of hclib::kernel */
void hclib_start_timeline() {
    if(!logger_started) start_cycles = wctime();
    logger_started = 1;
}

/* Called at the end of hclib::kernel */
void hclib_stop_timeline() {
    if(!logger_stopped) end_cycles = wctime();
    logger_stopped = 1;
}

// Initialize the datastructures
void hclib_init_timeline() {
    int total_participants = TIMELINE_PARTICIPANTS;
    loginfo = (logger_t***) malloc(sizeof(logger_t**) * TOTAL_TIMELINE_EVENTS);
    loginfo_head = (logger_t***) malloc(sizeof(logger_t**) * TOTAL_TIMELINE_EVENTS);
    for(int i=0; i<TOTAL_TIMELINE_EVENTS; i++) {
        loginfo[i] = (logger_t**) malloc(sizeof(logger_t*) * total_participants);
        loginfo_head[i] = (logger_t**) malloc(sizeof(logger_t*) * total_participants);
        for(int j=0; j<total_participants; j++) {
            loginfo_head[i][j] = loginfo[i][j] = NULL;
        }
    }
}

void hclib_free_timeline() {
    int total_participants = TIMELINE_PARTICIPANTS;
    for(int i=0; i<TOTAL_TIMELINE_EVENTS; i++) {
        for(int j=0; j<total_participants; j++) {
            while(loginfo[i][j] != NULL) {
                logger_t* curr = loginfo[i][j];
                loginfo[i][j] = loginfo[i][j]->prev;
                free(curr);
            }
        }
        free(loginfo[i]);
        free(loginfo_head[i]);
    }
    free(loginfo);
    free(loginfo_head);
}

// can be called by threads in parallel
void hclib_log_event(int participant, TIMELINE_EVENTS event) {
    if(!logger_started || logger_stopped) return;
    logger_t* node = (logger_t*) malloc(sizeof(logger_t));
    if(loginfo[event][participant] == NULL) {
        loginfo[event][participant] = node;
        loginfo_head[event][participant] = node;
        loginfo[event][participant]->prev = NULL;
    } else {
        loginfo[event][participant]->next = node;
        logger_t* prev = loginfo[event][participant];
        loginfo[event][participant] = loginfo[event][participant]->next;
        loginfo[event][participant]->prev = prev;
    }
    loginfo[event][participant]->next = NULL;
    loginfo[event][participant]->cycles = wctime();
}

void hclib_prettyprint_timeline_events() {
    int total_participants = TIMELINE_PARTICIPANTS;
    printf("------------------------------ HClib TIMELINE ---------------------------\n");
    const double duration = end_cycles - start_cycles;
    for(int i=0; i<TOTAL_TIMELINE_EVENTS; i++) {
        printf("Event(%d)============> \n",i);
        for(int j=0; j<total_participants; j++) {
            printf("Participant(%d): ",j);
            logger_t* current_reading = loginfo_head[i][j];
            int start_percentage = 101; //beyond the boundary
            if(current_reading != NULL) {
                start_percentage = (int)(100 * (current_reading->cycles - start_cycles) / duration);
                if(start_percentage%TOTAL_TIMELINE_EVENTS!=0) {
                    start_percentage += (start_percentage%TOTAL_TIMELINE_EVENTS);
                } else {
                    start_percentage += TOTAL_TIMELINE_EVENTS;
                }
            }
            for(int percent=start_percentage; percent<=100; percent+=TIMELINE_GRANULARITY) {
                int event_count=0;
                while(current_reading != NULL && current_reading->cycles<=end_cycles) {
                    double time = current_reading->cycles - start_cycles;
                    double curr_percent = 100 * time / duration;
                    if(curr_percent > percent) {
                        break;
                    }
                    event_count++;
                    current_reading = current_reading->next;
                }
                if(event_count>0) printf("%d:%d,",percent,event_count);
            }
            printf("\n");
        }
    }
    printf("--------------------------- END TIMELINE ---------------------------\n");
}
#else
void hclib_start_timeline() { }
void hclib_stop_timeline() { }
void hclib_init_timeline() { }
void hclib_free_timeline() { }
void hclib_log_event(int participant, TIMELINE_EVENTS event) { }
void hclib_prettyprint_timeline_events() { }
#endif
