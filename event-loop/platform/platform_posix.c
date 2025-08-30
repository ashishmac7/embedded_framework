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
static int timer_fd = -1;
static pthread_t timer_thread;

static volatile int timer_running = 0;

static uint32_t due;
static void (*arm_hal_callback)(void);

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
    return timer_fd;
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

#ifdef NS_EVENTLOOP_USE_TICK_TIMER
static void (*timer_callback)(void) = NULL;

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

#endif // NS_EVENTLOOP_USE_TICK_TIMER

int platform_timer_enable(void)
{
if (timer_fd != -1) {
        return -1; // Already registered
    }
    
    timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (timer_fd == -1) {
        return -1;
    }
    return timer_fd;
}

// Actually cancels a timer, not the opposite of enable
void platform_timer_disable(void)
{
    timerfd_settime(timer_fd, 0, NULL, NULL);
}

// Not called while running, fortunately
void platform_timer_set_cb(void (*new_fp)(void))
{
    arm_hal_callback = new_fp;
}

void timer_callback(void)
{
    due = 0;
    arm_hal_callback();
}

// This is called from inside platform_enter_critical - IRQs can't happen
void platform_timer_start(uint16_t slots)
{
    // timer->reset();
    // due = slots * UINT32_C(50);
    // timeout->attach_us(timer_callback, due);
    if (timer_fd == -1 || !arm_hal_callback) {
        return -1;
    }
    due = slots * UINT32_C(50); // in microseconds
    struct itimerspec timer_spec;
    timer_spec.it_value.tv_sec = due / 1000000;
    timer_spec.it_value.tv_nsec = (due % 1000000) * 1000;
    timer_spec.it_interval.tv_sec = due / 1000000;
    timer_spec.it_interval.tv_nsec = (due % 1000000) * 1000;

    if (timerfd_settime(timer_fd, 0, &timer_spec, NULL) == -1) {
        printf("Failed to set timer: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;

}

// This is called from inside platform_enter_critical - IRQs can't happen
uint16_t platform_timer_get_remaining_slots(void)
{
    struct itimerspec current_timer;
    if (timer_fd == -1 || timerfd_gettime(timer_fd, &current_timer) == -1) {
        return 0; // Timer not active or error occurred
    }

    uint64_t remaining_ns = (current_timer.it_value.tv_sec * 1000000000ULL) + current_timer.it_value.tv_nsec;
    if (remaining_ns == 0) {
        return 0; // Timer has expired
    }

    uint32_t remaining_us = remaining_ns / 1000; // Convert to microseconds
    if (remaining_us < due) {
        return (uint16_t)((due - remaining_us) / 50);
    } else {
        return 0;
    }

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

