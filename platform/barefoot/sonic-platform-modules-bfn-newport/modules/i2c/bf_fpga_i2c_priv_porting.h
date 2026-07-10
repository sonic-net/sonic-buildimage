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
#ifndef _BF_FPGA_I2C_PRIV_PORTING_H
#define _BF_FPGA_I2C_PRIV_PORTING_H

/* Allow the use in C++ code. */
#ifdef __cplusplus
extern "C" {
#endif

#include <linux/errno.h>
#include <linux/pci.h>
#include <linux/delay.h>

/* This file contains OS and system specific porting functions declarations.
 */
/* return status compliant with linux system calls */
#define BF_FPGA_OK 0
#define BF_FPGA_EINVAL (-EINVAL)
#define BF_FPGA_EIO (-EIO)
#define BF_FPGA_EBUSY (-EBUSY)
#define BF_FPGA_EAGAIN (-EAGAIN)

/* pci memory access functions */
static inline void bf_fpga_write32(uint8_t *addr, uint32_t val) {
  u8 __iomem *reg_addr = addr;
  writel(val, reg_addr);
}

static inline uint32_t bf_fpga_read32(uint8_t *addr) {
  u8 __iomem *reg_addr = addr;
  return (readl(reg_addr));
}

static inline void bf_fpga_write8(uint8_t *addr, uint8_t val) {
  u8 __iomem *reg_addr = addr;
  writeb(val, reg_addr);
}

static inline uint8_t bf_fpga_read8(uint8_t *addr) {
  u8 __iomem *reg_addr = addr;
  return (readb(reg_addr));
}

static inline void bf_fpga_us_delay(unsigned long usecs) {
  usleep_range(usecs, usecs + 10);
}

/* general purpose mutual exclusion lock */
typedef void *bf_fpga_mutex_t;

/* fast_lock for locking only non-blocking and quick operations */
typedef void *bf_fpga_fast_lock_t;

/* APIs to init/enter/leave critical (exclusive access) regions */
int bf_fpga_cr_init(bf_fpga_mutex_t *lock);
void bf_fpga_cr_destroy(bf_fpga_mutex_t *lock);
int bf_fpga_cr_enter(bf_fpga_mutex_t *lock);
void bf_fpga_cr_leave(bf_fpga_mutex_t *lock);

int bf_fpga_fast_lock_init(bf_fpga_fast_lock_t *sl, unsigned int initial);
void bf_fpga_fast_lock_destroy(bf_fpga_fast_lock_t *sl);
int bf_fpga_fast_lock(bf_fpga_fast_lock_t *sl);
void bf_fpga_fast_unlock(bf_fpga_fast_lock_t *sl);

#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _BF_FPGA_I2C_PRIV_PORTING_H */
