/*******************************************************************************
 Barefoot Networks FPGA Linux driver
 Copyright(c) 2018 - 2019 Barefoot Networks, Inc.

 This program is free software; you can redistribute it and/or modify it
 under the terms and conditions of the GNU General Public License,
 version 2, as published by the Free Software Foundation.

 This program is distributed in the hope it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 more details.

 You should have received a copy of the GNU General Public License along with
 this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.

 The full GNU General Public License is included in this distribution in
 the file called "COPYING".

 Contact Information:
 info@barefootnetworks.com
 Barefoot Networks, 4750 Patrick Henry Drive, Santa Clara CA 95054

*******************************************************************************/
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mutex.h>
#include <linux/bitops.h>
#include "bf_fpga_i2c_priv_porting.h"
#include "../bf_fpga_ioctl.h"
#include "bf_fpga_i2c.h"
#include "bf_fpga_i2c_priv.h"

/* This file contains OS and system specific porting functions for i2c APIs.
 * Implementation in this file is for porting to linux kernel.
 */
/* mutex APIs are for mutual exclusion with capability to sleep while in
 * exclusion mode
 */

/* sleepable virtual exclusion region */
typedef struct {
  atomic_t lock_state; /* 1: in exclusion mode, 0: not in exclusion mode */
} sleepable_v_mutex_t;

int bf_fpga_cr_init(bf_fpga_mutex_t *lock) {
  sleepable_v_mutex_t *mtx;

  if (!lock) {
    return -1;
  }
  mtx = vzalloc(sizeof(sleepable_v_mutex_t));

  if (mtx) {
    atomic_set(&mtx->lock_state, 0);
    *lock = (bf_fpga_mutex_t *)mtx;
    return 0;
  } else {
    *lock = NULL;
    return -1;
  }
}

void bf_fpga_cr_destroy(bf_fpga_mutex_t *lock) {
  if (lock && *lock) {
    vfree(*lock);
    *lock = NULL;
  }
}

void bf_fpga_cr_leave(bf_fpga_mutex_t *lock) {
  sleepable_v_mutex_t *mtx;

  if (lock && *lock) {
    mtx = (sleepable_v_mutex_t *)*lock;
    atomic_xchg(&mtx->lock_state, 0);
  }
}

int bf_fpga_cr_enter(bf_fpga_mutex_t *lock) {
  sleepable_v_mutex_t *mtx;

  /* All we do here is: test and set <lock_state> */
  if (lock && *lock) {
    int cnt = 10000; /* This will provide maximum of 500-1000 ms timeout */
    mtx = (sleepable_v_mutex_t *)*lock;
    while (atomic_cmpxchg(&mtx->lock_state, 0, 1) != 0) {
      if (cnt-- <= 0) {
        return -1; /* this is a worst case timeout situation */
      }
      usleep_range(50, 100); /* 50 us = about 2 bytes at 400Kbs i2c */
    }
    return 0;
  } else {
    return -1;
  }
}

/*  **** not implemented in current mode of locking */
int bf_fpga_mutex_trylock(bf_fpga_mutex_t *lock) {
  if (lock && *lock) {
    return -1;
  } else {
    return -1;
  }
}

/* fast lock is a non-blocking busy lock, implemented with spinlock */
int bf_fpga_fast_lock_init(bf_fpga_fast_lock_t *sl, unsigned int initial) {
  spinlock_t *slock;

  (void)initial;
  if (!sl) {
    return -1;
  }
  slock = vzalloc(sizeof(spinlock_t));

  if (slock) {
    spin_lock_init(slock);
    *sl = (bf_fpga_fast_lock_t *)slock;
    return 0;
  } else {
    *sl = NULL;
    return -1;
  }
}

void bf_fpga_fast_lock_destroy(bf_fpga_fast_lock_t *sl) {
  if (sl && *sl) {
    vfree(*sl);
    *sl = NULL;
  }
}

int bf_fpga_fast_lock(bf_fpga_fast_lock_t *sl) {
  if (sl && *sl) {
    spin_lock(*sl);
    return 0;
  } else {
    return -1;
  }
}

void bf_fpga_fast_unlock(bf_fpga_fast_lock_t *sl) {
  if (sl && *sl) {
    spin_unlock(*sl);
  }
}
