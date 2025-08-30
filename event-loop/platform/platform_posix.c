// platform_posix.c
// Platform abstraction for POSIX/Linux

#define _GNU_SOURCE
#include "platform/arm_hal_interrupt.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <sys/timerfd.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include <fcntl.h>
#include <string.h>
#include "platform/arm_hal_timer.h"
#include "eventOS_scheduler.h"

static pthread_mutex_t critical_mutex;
static void init_critical_mutex(void) {
    static int initialized = 0;
    if (!initialized) {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        pthread_mutex_init(&critical_mutex, &attr);
        pthread_mutexattr_destroy(&attr);
        initialized = 1;
    }
}
static int timer_fd = -1;
static pthread_t timer_thread;
static void (*timer_callback)(void) = NULL;
static volatile int timer_running = 0;


void platform_enter_critical(void) {
    init_critical_mutex();
    pthread_mutex_lock(&critical_mutex);
}


void platform_exit_critical(void) {
    pthread_mutex_unlock(&critical_mutex);
}
int cmt_timer_init()
{
    if (timer_fd != -1) {
        return -1; // Already registered
    }
    
    timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (timer_fd == -1) {
        return -1;
    }
}
void common_timer_process(int timerfd)
{
    uint64_t val;
    int ret;
    
    ret = read(timerfd, &val, sizeof(val));
    
    if(ret < 0)
    {
        fprintf(stderr, "cancelled timer?\n");
        return;
    }
    if(val != 1)
    {
        fprintf(stderr, "missing timers: %"PRIu64"\n", val - 1);
        return;
    }

    if (ret > 0)
    {
        timer_callback();
    }
    
}

int8_t platform_tick_timer_register(void (*tick_timer_cb)(void))
{
    timer_callback = tick_timer_cb;
    return 0;
}

int8_t platform_tick_timer_start(uint32_t period_ms)
{
    if (timer_fd == -1 || !timer_callback) {
        return -1;
    }
    
    struct itimerspec timer_spec;
    timer_spec.it_value.tv_sec = period_ms / 1000;
    timer_spec.it_value.tv_nsec = (period_ms % 1000) * 1000000;
    timer_spec.it_interval.tv_sec = period_ms / 1000;
    timer_spec.it_interval.tv_nsec = (period_ms % 1000) * 1000000;
    
    if (timerfd_settime(timer_fd, 0, &timer_spec, NULL) == -1) {
        printf("Failed to set timer: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

int8_t platform_tick_timer_stop(void)
{
    if (timer_fd == -1) {
        return -1;
    }
    
    timer_running = 0;
    
    // Stop the timer
    struct itimerspec timer_spec = {0};
    timerfd_settime(timer_fd, 0, &timer_spec, NULL);
    
    // Wait for thread to finish
    pthread_join(timer_thread, NULL);
    
    close(timer_fd);
    timer_fd = -1;
    timer_callback = NULL;
    
    return 0;
}
int eventfd[2];
void eventOS_scheduler_signal(void)
{
    uint64_t val = 'W';
    ssize_t ret = write(eventfd[1], &val, sizeof(val));
    if (ret < 0) {
        perror("write to eventfd failed");
    }
    else {
        // printf("Event signal sent successfully\n");
    }
}

void event_scheduler_signal_init(int *event_fd)
{
    
    if (pipe(event_fd) == -1) {
        perror("pipe failed");
    }
    else {
        // printf("Pipe created successfully\n");
    }
    
    fcntl(event_fd[1], F_SETPIPE_SZ, sizeof(uint64_t) * 2);
    fcntl(event_fd[1], F_SETFL, O_NONBLOCK);
    memcpy(eventfd, event_fd, 2 * sizeof(int));
}

// Add more platform abstraction functions as needed
