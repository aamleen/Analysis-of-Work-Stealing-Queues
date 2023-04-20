
//----------------Control the Timeline by changing setting from here------->>
//Comment the below macro to disable the logging on timeline
//#define HCLIB_TIMELINE 1

typedef enum {
    SUCCESS_STEALS=0, /* Ensure the first event is assigned zero */
    /* Add any number of comma separated timeline events at this line */
    TOTAL_TIMELINE_EVENTS /*Ensure this is the last event here*/
} TIMELINE_EVENTS;

/*
 * Denotes the percentage of total execution.
 * Feel free to increase/decrease the granularity
 */
#define TIMELINE_GRANULARITY 2

/*
 * Default is to include all the workers inside the timeline.
 * If you want to monitor only a subset of workers then
 * update this TIMELINE_PARTICIPANTS macro with that worker count
 */
#define TIMELINE_PARTICIPANTS (hclib_num_workers())
//<<----------------Control the Timeline by changing setting until here-------

// DONT CHANGE ANYTHING BELOW

#define CACHE_LINE_SIZE 64

typedef struct _logger_t {
    double cycles;
    struct _logger_t* prev;
    struct _logger_t* next;
} logger_t __attribute__ ((aligned (CACHE_LINE_SIZE)));

void hclib_start_timeline();
void hclib_stop_timeline();
void hclib_init_timeline();
void hclib_free_timeline();
void hclib_log_event(int participant, TIMELINE_EVENTS event);
void hclib_prettyprint_timeline_events();

