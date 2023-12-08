/*
 * Copyright 2007-2020 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa
 */
/*
 * $Id: ksal.c,v 1.1 Broadcom SDK $
 * $Copyright: (c) 2005 Broadcom Corp.
 * All Rights Reserved.$
 */

#include <sal/core/sync.h>
#include <sal/core/thread.h>

#include "lkm.h"
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif
#include <linux/interrupt.h>
#include <linux/sched.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0)
#include <linux/sched/rt.h>
#endif
#include <linux/time.h>

#if defined(MAX_USER_RT_PRIO) || defined(MAX_RT_PRIO)
/* Assume 2.6 scheduler */
#define SAL_YIELD(task) \
    yield()
#else
/* Assume 2.4 scheduler */
#define SAL_YIELD(task) \
do { \
    task->policy |= SCHED_YIELD; \
    schedule(); \
} while (0)
#endif

#define SECOND_USEC (1000000)
#define USECS_PER_JIFFY (SECOND_USEC / HZ)
#define USEC_TO_JIFFIES(usec) ((usec + (USECS_PER_JIFFY - 1)) / USECS_PER_JIFFY)

#define sal_alloc(size, desc)   kmalloc(size, GFP_KERNEL)
#define sal_free(ptr)           kfree(ptr)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,12)
#define WQ_SLEEP(a, b)          wait_event_interruptible_timeout(a, NULL, b)
#else
#define WQ_SLEEP(a, b)          interruptible_sleep_on_timeout(&(a), b)
#endif
/*
 * sem_ctrl_t
 *
 *   The semaphore control type uses the binary property to implement
 *   timed semaphores with improved performance using wait queues.
 */

typedef struct sem_ctrl_s {
    struct semaphore    sem;
    int                 binary;
    int                 cnt;
    wait_queue_head_t   wq;
} sem_ctrl_t;

sal_sem_t
sal_sem_create(char *desc, int binary, int initial_count)
{
    sem_ctrl_t *s;

    if ((s = sal_alloc(sizeof(*s), desc)) != 0) {
	sema_init(&s->sem, initial_count);
        s->binary = binary;
        if (s->binary) {
            init_waitqueue_head(&s->wq);
        }
    }

    return (sal_sem_t) s;
}

void
sal_sem_destroy(sal_sem_t b)
{
    sem_ctrl_t *s = (sem_ctrl_t *) b;

    if (s == NULL) {
	return;
    }

    /*
     * the linux kernel does not have a sema_destroy(s)
     */
    sal_free(s);
}

int
sal_sem_take(sal_sem_t b, int usec)
{
    sem_ctrl_t *s = (sem_ctrl_t *) b;
    int			err;

    if (usec == sal_sem_FOREVER && !in_interrupt()) {
	err = down_interruptible(&s->sem);
    } else {
	int		time_wait = 1;
        int             cnt = s->cnt;

	for (;;) {
	    if (down_trylock(&s->sem) == 0) {
		err = 0;
		break;
	    }

            if (s->binary) {

                /* Wait for event or timeout */

                if (time_wait > 1) {
                    err = 1;
                    break;
                }
                err = wait_event_interruptible_timeout(s->wq, cnt != s->cnt, 
                                                       USEC_TO_JIFFIES(usec));
                if (err < 0) {
                    break;
                }
                time_wait++;

            } else {

                /* Retry algorithm with exponential backoff */

                if (time_wait > usec) {
                    time_wait = usec;
                }

                sal_usleep(time_wait);
                
                usec -= time_wait;
            
                if (usec == 0) {
                    err = ETIMEDOUT;
                    break;
                }

                if ((time_wait *= 2) > 100000) {
                    time_wait = 100000;
                }
	    }
	}
    }
    return err ? -1 : 0;
}

int
sal_sem_give(sal_sem_t b)
{
    sem_ctrl_t *s = (sem_ctrl_t *) b;

    up(&s->sem);
    if (s->binary) {
        s->cnt++;
        wake_up_interruptible(&s->wq);
    }
    return 0;
}

uint32
sal_time_usecs(void)
{
#if !defined(SAI_FIXUP)
     struct timeval ltv;
     do_gettimeofday(&ltv);
     return (ltv.tv_sec * SECOND_USEC + ltv.tv_usec);
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
    /* ktime_to_us and ktime_get_real_ns return 64-bit integets, but this */
    /* function is returning a 32-bit integer. This should be fine until 2038. */
    return ktime_to_us(ktime_get_real_ns());
#else
    struct timeval ltv;
    do_gettimeofday(&ltv);
    return (ltv.tv_sec * SECOND_USEC + ltv.tv_usec);
#endif
#endif
}
    
void
sal_usleep(uint32 usec)
{
    uint32 start_usec;
    wait_queue_head_t queue;

    if (usec <= SECOND_USEC / HZ) {
        start_usec = sal_time_usecs();
        do {
            SAL_YIELD(current);
        } while ((sal_time_usecs() - start_usec) < usec);
    } else {
        init_waitqueue_head(&queue);
        WQ_SLEEP(queue, USEC_TO_JIFFIES(usec));
    }
}

void
sal_udelay(uint32 usec)
{
    static volatile int _sal_udelay_counter;
    static int loops = 0;
    int ix, iy;

    if (loops == 0 || usec == 0) {      /* Need calibration? */
        int max_loops;
        int start = 0, stop = 0;
        int mpt = USECS_PER_JIFFY;      /* usec/tick */

        for (loops = 1; loops < 0x1000 && stop == start; loops <<= 1) {
            /* Wait for clock turn over */
            for (stop = start = jiffies; start == stop; start = jiffies) {
                /* Empty */
            }
            sal_udelay(mpt);    /* Single recursion */
            stop = jiffies;
        }

        max_loops = loops / 2;  /* Loop above overshoots */

        start = stop = 0;

        if (loops < 4) {
            loops = 4;
        }

        for (loops /= 4; loops < max_loops && stop == start; loops++) {
            /* Wait for clock turn over */
            for (stop = start = jiffies; start == stop; start = jiffies) {
                /* Empty */
            }
            sal_udelay(mpt);    /* Single recursion */
            stop = jiffies;
        }
    }
   
    for (iy = 0; iy < usec; iy++) {
        for (ix = 0; ix < loops; ix++) {
            _sal_udelay_counter++;      /* Prevent optimizations */
        }
    }
}
