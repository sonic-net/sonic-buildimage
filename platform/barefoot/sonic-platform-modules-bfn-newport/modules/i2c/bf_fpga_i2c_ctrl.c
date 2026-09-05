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
#include "bf_fpga_i2c_priv_porting.h"
#include "../bf_fpga_ioctl.h"
#include "bf_fpga_i2c.h"
#include "bf_fpga_i2c_priv.h"
#include "bf_fpga_i2c_reg.h"

/* static i2c controller contents */
static fpga_i2c_controller_t fpga_i2c_ctrl[BF_I2C_FPGA_NUM_CTRL];
static bool fpga_i2c_inited = false;

/* fpga memory space access APIs */
/* 32 bit write into fpga BAR0 */
void bf_fpga_reg_write(fpga_i2c_controller_t *i2c_ctrl,
                       uint32_t offset,
                       uint32_t val) {
  uint8_t *ptr = i2c_ctrl->fpga_base_addr + offset;
  bf_fpga_write32(ptr, val);
}

/* 32 bit read into fpga BAR0 */
uint32_t bf_fpga_reg_read(fpga_i2c_controller_t *i2c_ctrl, uint32_t offset) {
  uint8_t *ptr = i2c_ctrl->fpga_base_addr + offset;
  return (bf_fpga_read32(ptr));
}

/* 32 bit write into fpga i2c space */
void bf_fpga_i2c_reg_write32(fpga_i2c_controller_t *i2c_ctrl,
                             uint32_t offset,
                             uint32_t val) {
  uint8_t *ptr = i2c_ctrl->i2c_base_addr + offset;
  bf_fpga_write32(ptr, val);
}

/* 32 bit read into fpga i2c space */
uint32_t bf_fpga_i2c_reg_read32(fpga_i2c_controller_t *i2c_ctrl,
                                uint32_t offset) {
  uint8_t *ptr = i2c_ctrl->i2c_base_addr + offset;
  return (bf_fpga_read32(ptr));
}

/* 8 bit write into fpga i2c space */
void bf_fpga_i2c_reg_write8(fpga_i2c_controller_t *i2c_ctrl,
                            uint32_t offset,
                            uint8_t val) {
  uint8_t *ptr = i2c_ctrl->i2c_base_addr + offset;
  bf_fpga_write8(ptr, val);
}

/* 8 bit read into fpga i2c space */
uint8_t bf_fpga_i2c_reg_read8(fpga_i2c_controller_t *i2c_ctrl,
                              uint32_t offset) {
  uint8_t *ptr = i2c_ctrl->i2c_base_addr + offset;
  return (bf_fpga_read8(ptr));
}

int bf_fpga_i2c_lock(fpga_i2c_controller_t *i2c_ctrl) {
  return (bf_fpga_cr_enter(&i2c_ctrl->fpga_ctrl_lock));
}

void bf_fpga_i2c_unlock(fpga_i2c_controller_t *i2c_ctrl) {
  return (bf_fpga_cr_leave(&i2c_ctrl->fpga_ctrl_lock));
}

/** FPGA  return pointer to i2c_controller struct
 *
 *  @param bus_id
 *    i2c controller id
 *  @return
 *    pointer to i2c_controller struct of <bus_id>
 */
fpga_i2c_controller_t *fpga_i2c_ctrl_get(int bus_id) {
  if (bus_id >= BF_I2C_FPGA_NUM_CTRL) {
    return NULL;
  } else {
    return &fpga_i2c_ctrl[bus_id];
  }
}

/* is fpga module is soft inited */
bool fpga_i2c_is_inited() { return fpga_i2c_inited; }

/** FPGA I2C set clock: sets clock of i2c operations
 *
 *  controller must be stopped before, if applicable.
 *
 *  @param bus_id
 *    i2c controller id
 *  @param clk_div
 *    clock divider value as per fpga specs
 *  @return
 *    0 on success and <0 on error
 */
int fpga_i2c_set_clk(int bus_id, int clk_div) {
  uint32_t val;
  fpga_i2c_controller_t *i2c_ctrl;

  if (bus_id >= BF_I2C_FPGA_NUM_CTRL) {
    return BF_FPGA_EINVAL;
  }
  i2c_ctrl = fpga_i2c_ctrl_get(bus_id);
  if (bf_fpga_i2c_lock(i2c_ctrl)) {
    return BF_FPGA_EAGAIN;
  }
  clk_div = (clk_div & I2C_CTRL_CLK_DIV_MASK) << I2C_CTRL_CLK_DIV_SHF;
  val = bf_fpga_i2c_reg_read32(i2c_ctrl, Bf_FPGA_I2C_CTRL_TOP);
  val &= ~(I2C_CTRL_CLK_DIV_MASK << I2C_CTRL_CLK_DIV_SHF);
  val |= clk_div;
  bf_fpga_i2c_reg_write32(i2c_ctrl, Bf_FPGA_I2C_CTRL_TOP, val);
  bf_fpga_i2c_unlock(i2c_ctrl);
  return BF_FPGA_OK;
}

/** FPGA I2C stop : stops ongoing i2c operations without mutex locking
 *
 * internal function
 *
 *  @param bus_id
 *    i2c controller struct
 *  @return
 *    0 on success and <0 on error
 */
int fpga_i2c_stop_locked(fpga_i2c_controller_t *i2c_ctrl) {
  int to_ms, ret;
  uint8_t val;

  val = bf_fpga_i2c_reg_read8(i2c_ctrl, Bf_FPGA_I2C_CTRL_TOP);
  val &= ~I2C_CTRL_START;
  bf_fpga_i2c_reg_write8(i2c_ctrl, Bf_FPGA_I2C_CTRL_TOP, val);

  to_ms = 100; /* 5 msec converted to multiple of 50 micro sec */
  val = bf_fpga_i2c_reg_read8(i2c_ctrl, Bf_FPGA_TOP_I2C_STATUS);
  while ((val & I2C_STS_BUSY) && to_ms) {
    bf_fpga_us_delay(50);
    to_ms--;
    val = bf_fpga_i2c_reg_read8(i2c_ctrl, Bf_FPGA_TOP_I2C_STATUS);
  }
  if (to_ms > 0) {
    ret = BF_FPGA_OK;
  } else {
    ret = BF_FPGA_EIO;
  }
  return ret;
}

/** FPGA I2C start : starts i2c operations without mutex locking
 *
 * internal function
 *
 *  @param bus_id
 *    i2c controller struct
 *  @return
 *    0 on success and <0 on error
 */
int fpga_i2c_start_locked(fpga_i2c_controller_t *i2c_ctrl) {
  uint8_t val;
  val = bf_fpga_i2c_reg_read8(i2c_ctrl, Bf_FPGA_I2C_CTRL_TOP);
  bf_fpga_i2c_reg_write8(i2c_ctrl, Bf_FPGA_I2C_CTRL_TOP, val | I2C_CTRL_START);
  return BF_FPGA_OK;
}

/** FPGA I2C stop : stops ongoing i2c operations
 *
 *  @param bus_id
 *    i2c controller id
 *  @return
 *    0 on success and <0 on error
 */
int fpga_i2c_stop(int bus_id) {
  fpga_i2c_controller_t *i2c_ctrl;
  int ret;

  if (bus_id >= BF_I2C_FPGA_NUM_CTRL || !fpga_i2c_is_inited()) {
    return BF_FPGA_EINVAL;
  }
  i2c_ctrl = fpga_i2c_ctrl_get(bus_id);
  if (bf_fpga_i2c_lock(i2c_ctrl)) {
    return BF_FPGA_EAGAIN;
  }
  ret = fpga_i2c_stop_locked(i2c_ctrl);
  bf_fpga_i2c_unlock(i2c_ctrl);
  return ret;
}

/** FPGA I2C start : starts i2c operations
 *
 *  @param bus_id
 *    i2c controller id
 *  @return
 *    0 on success and <0 on error
 */
int fpga_i2c_start(int bus_id) {
  fpga_i2c_controller_t *i2c_ctrl;
  int ret;

  if (bus_id >= BF_I2C_FPGA_NUM_CTRL || !fpga_i2c_is_inited()) {
    return BF_FPGA_EINVAL;
  }
  i2c_ctrl = fpga_i2c_ctrl_get(bus_id);
  if (bf_fpga_i2c_lock(i2c_ctrl)) {
    return BF_FPGA_EAGAIN;
  }
  ret = fpga_i2c_start_locked(i2c_ctrl);
  bf_fpga_i2c_unlock(i2c_ctrl);
  return ret;
}

/** FPGA I2C reset: reset i2c by issuing 9 clocks in a specific way
 *
 *  @param bus_id
 *    i2c controller id
 *  @return
 *    0 on success and <0 on error
 */
int fpga_i2c_reset(int bus_id) {
  fpga_i2c_controller_t *i2c_ctrl;

  if (bus_id >= BF_I2C_FPGA_NUM_CTRL || !fpga_i2c_is_inited()) {
    return BF_FPGA_EINVAL;
  }
  i2c_ctrl = fpga_i2c_ctrl_get(bus_id);
  if (bf_fpga_i2c_lock(i2c_ctrl)) {
    return BF_FPGA_EAGAIN;
  }
  bf_fpga_i2c_reg_write8(i2c_ctrl, Bf_FPGA_I2C_CTRL_TOP, I2C_CTRL_RESET);
  bf_fpga_i2c_unlock(i2c_ctrl);
  return BF_FPGA_OK;
}

/** FPGA I2C is running?
 *
 *  @param bus_id
 *    i2c controller id
 *  @return
 *    0 on success and <0 on error
 */
int fpga_i2c_is_busy(int bus_id, bool *busy) {
  uint8_t val;
  fpga_i2c_controller_t *i2c_ctrl;

  if (bus_id >= BF_I2C_FPGA_NUM_CTRL || !fpga_i2c_is_inited()) {
    return BF_FPGA_EINVAL;
  }
  i2c_ctrl = fpga_i2c_ctrl_get(bus_id);
  if (bf_fpga_i2c_lock(i2c_ctrl)) {
    return BF_FPGA_EAGAIN;
  }
  val = bf_fpga_i2c_reg_read8(i2c_ctrl, Bf_FPGA_TOP_I2C_STATUS);
  *busy = ((val & I2C_STS_BUSY) ? true : false);
  bf_fpga_i2c_unlock(i2c_ctrl);
  return BF_FPGA_OK;
}

/** FPGA I2C  instruction enable/disable
 *
 *  enable or disable a particular instruction in i2c instruction memory
 *  @param bus_id
 *    i2c controller id
 *  @param inst_id
 *    instruction id within  this controller space
 *  @param en
 *    true for enable, false for disable
 *  @return
 *    0 on success and <0 on error
 */
int fpga_i2c_inst_en(int bus_id, int inst_id, bool en) {
  uint32_t val;
  fpga_i2c_controller_t *i2c_ctrl;

  if (bus_id >= BF_I2C_FPGA_NUM_CTRL || !fpga_i2c_is_inited()) {
    return BF_FPGA_EINVAL;
  }
  if (inst_id < 0 || inst_id >= FPGA_I2C_NUM_INST) {
    return BF_FPGA_EINVAL;
  }
  i2c_ctrl = fpga_i2c_ctrl_get(bus_id);
  if (bf_fpga_i2c_lock(i2c_ctrl)) {
    return BF_FPGA_EAGAIN;
  }
  val = bf_fpga_i2c_reg_read32(i2c_ctrl, Bf_FPGA_I2C_INST_CTRL(inst_id));
  if (en) {
    val |= I2C_INST_EN;
  } else {
    val &= ~I2C_INST_EN;
  }
  bf_fpga_i2c_reg_write32(i2c_ctrl, Bf_FPGA_I2C_INST_CTRL(inst_id), val);
  fpga_i2c_ctrl[bus_id].i2c_inst[inst_id].en = en;
  bf_fpga_i2c_unlock(i2c_ctrl);
  return BF_FPGA_OK;
}

/** FPGA I2C controller initialization
 *
 *  @param bus_id
 *    i2c controller id
 *  @return
 *    0 on success and <0 on error
 */
int fpga_i2c_controller_init(int bus_id) {
  fpga_i2c_controller_t *i2c_ctrl;
  int i, ret;

  if (bus_id >= BF_I2C_FPGA_NUM_CTRL) {
    return BF_FPGA_EINVAL;
  }
  i2c_ctrl = fpga_i2c_ctrl_get(bus_id);
  if (!i2c_ctrl) {
    return BF_FPGA_EINVAL;
  }
  bf_fpga_fast_lock_init(&i2c_ctrl->spinlock, 0);
  bf_fpga_cr_init(&i2c_ctrl->fpga_ctrl_lock);
  bf_fpga_cr_enter(&i2c_ctrl->fpga_ctrl_lock);
  for (i = 0; i < FPGA_I2C_NUM_INST; i++) {
    fpga_i2c_ctrl[bus_id].i2c_inst[i].inst = (uint32_t)i;
    bf_fpga_i2c_reg_write32(i2c_ctrl, Bf_FPGA_I2C_INST_CTRL(i), 0);
  }
  bf_fpga_cr_leave(&i2c_ctrl->fpga_ctrl_lock);
  ret = fpga_i2c_set_clk(bus_id, 1); /* 400 khz default */
  ret |= fpga_i2c_stop(bus_id);      /* just in case */
  return ret;
}

/** FPGA I2C controller de initialization
 *
 *  @param bus_id
 *    i2c controller id
 *  @return
 *    0 on success and <0 on error
 */
int fpga_i2c_controller_cleanup(int bus_id) {
  int i;

  fpga_i2c_stop(bus_id);
  for (i = 0; i < FPGA_I2C_NUM_INST; i++) {
    fpga_i2c_ctrl[bus_id].i2c_inst[i].en = false;
  }
  bf_fpga_cr_destroy(&fpga_i2c_ctrl[bus_id].fpga_ctrl_lock);
  bf_fpga_fast_lock_destroy(&fpga_i2c_ctrl[bus_id].spinlock);
  return BF_FPGA_OK;
}

/** FPGA I2C global initialization
 *
 *  @param base_addr
 *     virtual address of i2c memory relative to BAR0 base
 *  @return
 *    0 on success and <0 on error
 */
int fpga_i2c_init(uint8_t *base_addr) {
  int i;

  memset(fpga_i2c_ctrl, 0, sizeof(fpga_i2c_ctrl));
  for (i = 0; i < BF_I2C_FPGA_NUM_CTRL; i++) {
    fpga_i2c_ctrl[i].i2c_base_addr = base_addr + BF_FPGA_I2C_CTRL_BASE_ADDR(i);
    fpga_i2c_ctrl[i].fpga_base_addr = base_addr;
    fpga_i2c_controller_init(i);
  }
  fpga_i2c_inited = true;
  return BF_FPGA_OK;
}

/** FPGA I2C global de initialization
 *
 */
void fpga_i2c_deinit(void) {
  int i;

  for (i = 0; i < BF_I2C_FPGA_NUM_CTRL; i++) {
    fpga_i2c_controller_cleanup(i);
  }
  fpga_i2c_inited = false;
}
