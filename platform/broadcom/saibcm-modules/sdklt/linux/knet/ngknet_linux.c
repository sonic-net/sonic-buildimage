/*! \file ngknet_linux.c
 *
 * Utility routines for Linux kernel APIs abstraction.
 *
 */
/*
 *
 * Copyright 2018-2025 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.
 */

#include <linux/kconfig.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>

#include "ngknet_linux.h"

/*!
 * Time
 */

unsigned long
sal_time_usecs(void)
{
    return (unsigned long)kal_time_usecs();
}

void
sal_usleep(unsigned long usec)
{
    unsigned long then, now, hz;

    hz = usec * HZ / 1000000;
    if (hz) {
        schedule_timeout(hz);
    }
    usec = usec * HZ % 1000000 / HZ;
    if (usec) {
        then = sal_time_usecs();
        do {
            schedule();
            now = sal_time_usecs();
        } while (now > then && (now - then) < usec);
    }
}

/*!
 * Synchronization
 */

typedef struct {
    struct semaphore sem;
    char *desc;
    int binary;
} sem_ctrl_t;

sal_sem_t
sal_sem_create(char *desc, int binary, int count)
{
    sem_ctrl_t *sc = kmalloc(sizeof(*sc), GFP_KERNEL);

    if (sc != NULL) {
        sema_init(&sc->sem, count);
        sc->desc = desc;
        sc->binary = binary;
    }

    return (sal_sem_t)sc;
}

void
sal_sem_destroy(sal_sem_t sem)
{
    sem_ctrl_t *sc = (sem_ctrl_t *)sem;

    kfree(sc);
}

int
sal_sem_take(sal_sem_t sem, int usec)
{
    sem_ctrl_t *sc = (sem_ctrl_t *)sem;
    int rv;

    if (usec == SAL_SEM_FOREVER) {
        do {
            rv = down_interruptible(&sc->sem);
        } while (rv == -EINTR);
        return rv ? -1 : 0;
    }

    return -1;
}

int
sal_sem_give(sal_sem_t sem)
{
    sem_ctrl_t *sc = (sem_ctrl_t *)sem;

    up(&sc->sem);

    return 0;
}

typedef struct spinlock_ctrl_s {
    spinlock_t spinlock;
    unsigned long flags;
    char *desc;
} *spinlock_ctrl_t;

sal_spinlock_t
sal_spinlock_create(char *desc)
{
    spinlock_ctrl_t sl = kmalloc(sizeof(*sl), GFP_KERNEL);

    if (sl != NULL) {
        spin_lock_init(&sl->spinlock);
        sl->flags = 0;
        sl->desc = desc;
    }

    return (sal_spinlock_t)sl;
}

void
sal_spinlock_destroy(sal_spinlock_t lock)
{
    spinlock_ctrl_t sl = (spinlock_ctrl_t)lock;

    kfree(sl);
}

int
sal_spinlock_lock(sal_spinlock_t lock)
{
    spinlock_ctrl_t sl = (spinlock_ctrl_t)lock;

    spin_lock_irqsave(&sl->spinlock, sl->flags);

    return 0;
}

int
sal_spinlock_unlock(sal_spinlock_t lock)
{
    spinlock_ctrl_t sl = (spinlock_ctrl_t)lock;

    spin_unlock_irqrestore(&sl->spinlock, sl->flags);

    return 0;
}

