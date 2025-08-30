#include "stdio.h"
#include "stdlib.h"
#include "eventOS_scheduler.h"
#include "eventOS_event.h"
#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "eventOS_event_timer.h"


enum {
    INITIALIZATION_EVENT,
    MY_EVENT
};

enum {
    POLLFD_EVENT,
    POLLFD_TIMER,
    POLLFD_COUNT,
};


struct nmse_ctxt {
    struct pollfd fds[POLLFD_COUNT];
    int schedulerfd[2];
    int timerfd;
};
struct nmse_ctxt g_ctxt ={0, }; // rfMeshProj


static void wsbr_fds_init(struct nmse_ctxt *ctxt)
{
    ctxt->fds[POLLFD_EVENT].fd = ctxt->schedulerfd[0];
    ctxt->fds[POLLFD_EVENT].events = POLLIN;
    ctxt->fds[POLLFD_TIMER].fd = ctxt->timerfd;
    ctxt->fds[POLLFD_TIMER].events = POLLIN;
    // ctxt->fds[POLLFD_LWM2M_SERVER].fd = ctxt->lwm2m_fd; // rfMeshProj
    // ctxt->fds[POLLFD_LWM2M_SERVER].events = POLLIN;
}

void common_timer_process(int timerfd);
static void nmse_poll(struct nmse_ctxt *ctxt)
{
    uint64_t val;
    int poll_ret;
    
    // Add timeout and proper polling
    poll_ret = poll(ctxt->fds, POLLFD_COUNT, 1); // 1 millisecond timeout

    if (poll_ret < 0) {
        printf("Poll error: %s\n", strerror(errno));
        return;
    }
    
    if (poll_ret == 0) {
        // Timeout - normal, just continue
        return;
    }
    
    if (ctxt->fds[POLLFD_EVENT].revents & POLLIN) {
        read(ctxt->schedulerfd[0], &val, sizeof(val));
        // printf("Event received: %llu\n", val);
        eventOS_scheduler_run_until_idle();
    }
    

    if (ctxt->fds[POLLFD_TIMER].revents & POLLIN) {
        // tr_debug("Timer event received");
        // printf("Timer event received.\n");
        common_timer_process(ctxt->timerfd);
    }
    
} 



void my_event_handler(arm_event_s *event) {
    switch (event->event_type) {
        case INITIALIZATION_EVENT:
            // Initialize my module
            printf("Initialization event received.\n");
            break;
        case MY_EVENT:
            // Event received
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            char timebuf[64];
            struct tm tm_info;
            localtime_r(&ts.tv_sec, &tm_info);
            strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm_info);
            printf("[%s] My event received with ID: %d\n", timebuf, event->event_id);
            break;
        default:
            printf("Unknown event type: %d\n", event->event_type);
            break;
    }
}

int platform_timer_enable();
void event_scheduler_signal_init(int *schedulerfd);
int main()
{
    printf("Starting event loop...\n");
    struct nmse_ctxt *ctxt = &g_ctxt;
    event_scheduler_signal_init(ctxt->schedulerfd);

    ctxt->timerfd = platform_timer_enable();
    if (ctxt->timerfd < 0) {
        printf("Failed to initialize timer.\n");
        return -1;
    }
    
    eventOS_scheduler_init();
    int my_eventhandler_id = eventOS_event_handler_create(my_event_handler, INITIALIZATION_EVENT);
    if (my_eventhandler_id < 0) {
        printf("Failed to create event handler: %d\n", my_eventhandler_id); 
    }
    else {
        printf("Event handler created with ID: %d\n", my_eventhandler_id);
    }

    wsbr_fds_init(ctxt);

    struct arm_event_s my_event = {0};
    my_event.event_type = MY_EVENT;
    my_event.event_id = 1;
    my_event.receiver = my_eventhandler_id;
    my_event.sender = 0;

    if(eventOS_event_send_every(&my_event,eventOS_event_timer_ms_to_ticks(1500))) {
        printf("Event sent successfully.\n");
    } else {
        printf("Failed to send event.\n");
    }

    while (true) {
        nmse_poll(ctxt);
    }
    return 0;
}


