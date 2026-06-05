#ifndef UTILITY_H
#define UTILITY_H

#include "sdk.h"

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COUNT_OF(array) ((sizeof(array) / sizeof(0 [array])) / ((size_t)(!(sizeof(array) % sizeof(0 [array])))))

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static inline unsigned reverse_bits(unsigned num, unsigned num_of_bits) {
    unsigned reverse_num = 0;

    for (unsigned i = 0; i < num_of_bits; i++) {
        if ((num & (1 << i))) reverse_num |= 1 << ((num_of_bits - 1) - i);
    }
    return reverse_num;
}

static inline int int_to_twos(int value, unsigned width) {
    return ((1 << width) + value) & ((1 << width) - 1);
}

static inline int twos_to_int(int twos, unsigned width) {
    int mask = 1 << (width - 1);
    return (twos & (~mask)) - (twos & mask);
}

static inline unsigned bin_gray(unsigned bb) {
    unsigned gg = 0;
    gg = bb ^ (bb >> 1);
    return gg;
}

static inline unsigned gray_bin(unsigned gg) {
    gg = gg ^ (gg >> 1);
    gg = gg ^ (gg >> 2);
    gg = gg ^ (gg >> 4);
    gg = gg ^ (gg >> 8);
    gg = gg ^ (gg >> 16);
    return gg;
}

/* CredoTime_t: Platform independent timer */
static inline int get_time(CredoTime_t* time) {
    return clock_gettime(CLOCK_MONOTONIC, time);
}

static inline uint64_t us_passed(CredoTime_t* start_time) {
    CredoTime_t now;
    get_time(&now);
    /* Calculate time difference */
    uint64_t diff_nsec = difftime(now.tv_sec, start_time->tv_sec) * 1000000000;
    diff_nsec += (now.tv_nsec - start_time->tv_nsec);
    return diff_nsec / 1000;
}

static inline uint64_t ms_passed(CredoTime_t* start_time) {
    return us_passed(start_time) / 1000;
}

static inline int is_timeout(CredoTime_t* start_time, unsigned long usec) {
    return us_passed(start_time) > usec;
}

static inline void sleep_us(unsigned us) {
    struct timespec delay;
    delay.tv_sec = us / 1000000;
    delay.tv_nsec = (us % 1000000) * 1000;
    nanosleep(&delay, NULL);
}
static inline void sleep_ms(unsigned ms) {
    struct timespec delay;
    delay.tv_sec = ms / 1000;
    delay.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&delay, NULL);
}

static inline double get_walltime(void) {
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1.0e9;
}

#ifdef __cplusplus
}
#endif

#endif
