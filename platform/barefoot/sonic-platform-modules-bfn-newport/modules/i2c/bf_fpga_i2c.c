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
#include "bf_fpga_i2c_priv.h"
#include "bf_fpga_i2c.h"
#include "bf_fpga_i2c_reg.h"

/* allocate find physically contiguous free blocks of instructions in one time
 * or periodic area  and mark them "in-use" */
static int get_next_free_index(fpga_i2c_controller_t *i2c_ctrl,
                               int cnt,
                               bool pr) {
  int i, j, begin, end;

  if (pr) {
    begin = FPGA_I2C_PERIODIC_BEGIN_INDEX;
    end = FPGA_I2C_NUM_INST;
  } else {
    begin = FPGA_I2c_ONESHOT_BEGIN_INDEX;
    end = FPGA_I2C_ONESHOT_NUM_INST;
  }

  bf_fpga_fast_lock(&i2c_ctrl->spinlock);
  for (i = begin; i < end;) {
    /* check if there are cnt number of free slots here */
    for (j = 0; j < cnt; j++) {
      if (i2c_ctrl->i2c_inst[i + j].in_use) {
        break;
      }
    }
    if (j == cnt) {
      /* we found enough free slots, so, return i */
      break;
    } else {
      /* we did not find enough free slots, continue searching */
      i += (j + 1);
      continue;
    }
  }
  if (i < end) {
    for (j = 0; j < cnt; j++) {
      i2c_ctrl->i2c_inst[i + j].in_use = true;
    }
  } else {
    i = -1;
  }
  bf_fpga_fast_unlock(&i2c_ctrl->spinlock);
  return i;
}

/* free physically contiguous in-use blocks of instructions */
static void release_index(fpga_i2c_controller_t *i2c_ctrl,
                          int inst_id,
                          int cnt) {
  int i;

  if (inst_id < 0 || (inst_id + cnt) >= FPGA_I2C_NUM_INST) {
    return; /* invalid id */
  }
  bf_fpga_fast_lock(&i2c_ctrl->spinlock);
  for (i = inst_id; i < (inst_id + cnt); i++) {
    i2c_ctrl->i2c_inst[i].in_use = false;
  }
  bf_fpga_fast_unlock(&i2c_ctrl->spinlock);
}

/* convert miroseconds to i2c-instruction delay parameter */
static int us_to_fpga_delay(int microsec) {
  int delay;

  if (microsec < 10) {
    delay = 0;
  } else if (microsec < 100) {
    delay = 1;
  } else if (microsec < 1000) {
    delay = 2;
  } else if (microsec < 10000) {
    delay = 3;
  } else if (microsec < 100000) {
    delay = 4;
  } else if (microsec < 1000000) {
    delay = 5;
  } else {
    delay = 6;
  }
  return delay;
}

/* populate single i2c_instruction at the given instruction slot */
static int fpga_i2c_enqueue(int bus_id,
                            int inst_id,
                            bf_fpga_i2c_inst_t *i2c_inst) {
  fpga_i2c_controller_t *i2c_ctrl = fpga_i2c_ctrl_get(bus_id);
  int delay = us_to_fpga_delay(i2c_inst->delay);
  uint32_t wd0 = 0, wd1 = 0;
  uint32_t i2c_data[2];
  uint8_t i2c_addr, num_wr, num_rd;

  i2c_addr = i2c_inst->i2c_addr;
  num_wr = i2c_inst->wr_cnt;
  num_rd = i2c_inst->rd_cnt;
  if (i2c_addr > 0x7F || num_wr > 129 || num_rd > 128) {
    return BF_FPGA_EINVAL;
  }
  if (i2c_inst->preemt) {
    wd0 |= I2C_INST_PMT;
  }
  if (i2c_inst->en) {
    wd0 |= I2C_INST_EN;
  }
  i2c_data[0] = i2c_data[1] = 0; /* clear on init */
  switch (i2c_inst->i2c_type) {
    case BF_FPGA_I2C_NOP:
      /* add delay + enable */
      wd0 |= (I2C_NOP | (delay << I2C_DELAY_SHF));
      break;
    case BF_FPGA_I2C_WRITE:
      if (num_wr == 0) {
        return BF_FPGA_EINVAL;
      }
      wd0 |= (I2C_WR_ADDR_DATA | (delay << I2C_DELAY_SHF));
      wd1 |= (i2c_inst->i2c_addr << I2C_DEV_ADDR_SHF);
      /* copy the first byte into register address */
      wd1 |= ((i2c_inst->fpga_i2c_buf.wr_buf[0]) << I2C_CMD_OFFSET);
      wd1 |= ((num_wr - 1) << I2C_WR_CNT_SHF);
      if (num_wr <= 9) {
        /* copy data into instruction area */
        memcpy(i2c_data, &i2c_inst->fpga_i2c_buf.wr_buf[1], (num_wr - 1));
        bf_fpga_i2c_reg_write32(
            i2c_ctrl, Bf_FPGA_I2C_INST_DATA_LO(inst_id), i2c_data[0]);
        bf_fpga_i2c_reg_write32(
            i2c_ctrl, Bf_FPGA_I2C_INST_DATA_HI(inst_id), i2c_data[1]);
      } else {
        /* copy the data in data area */
        int len = num_wr - 1;
        uint32_t addr;
        uint8_t *val = (uint8_t *)(&i2c_inst->fpga_i2c_buf.wr_buf[1]);
        /* store the data pointer  Note the indexing required by FPGA specs */
        i2c_data[0] = BF_FPGA_I2C_DATA_AREA(inst_id);
        addr = i2c_data[0];
        bf_fpga_i2c_reg_write32(
            i2c_ctrl, Bf_FPGA_I2C_INST_DATA_LO(inst_id), i2c_data[0] / 4);
        /* do byte write to avoid endianness mismatch */
        while (len--) {
          bf_fpga_i2c_reg_write8(i2c_ctrl, addr, *val);
          addr++;
          val++;
        }
      }
      break;
    case BF_FPGA_I2C_READ:
      if (num_rd == 0) {
        return BF_FPGA_EINVAL;
      }
      wd0 |= (I2C_RD_DATA | (delay << I2C_DELAY_SHF));
      wd1 |= (i2c_inst->i2c_addr << I2C_DEV_ADDR_SHF);
      wd1 |= ((num_rd) << I2C_RD_CNT_SHF);
      if (num_rd > 8) {
        /* store the data area pointer */
        i2c_data[0] = BF_FPGA_I2C_DATA_AREA(inst_id);
        bf_fpga_i2c_reg_write32(
            i2c_ctrl, Bf_FPGA_I2C_INST_DATA_LO(inst_id), i2c_data[0] / 4);
      }
      break;
    case BF_FPGA_I2C_ADDR_READ:
      if (num_wr == 0 || num_rd == 0) {
        return BF_FPGA_EINVAL;
      }
      wd0 |= (I2C_RD_ADDR_DATA_BURST | (delay << I2C_DELAY_SHF));
      wd1 |= (i2c_inst->i2c_addr << I2C_DEV_ADDR_SHF);
      /* 1st byte of the write buf goes into "register address" field */
      wd1 |= ((num_wr - 1) << I2C_WR_CNT_SHF);
      wd1 |= ((i2c_inst->fpga_i2c_buf.wr_buf[0]) << I2C_CMD_OFFSET);
      wd1 |= ((num_rd) << I2C_RD_CNT_SHF);
      /* less than 8 bytes data goes to the instruction area */
      if ((num_wr - 1 + num_rd) <= 8) {
        memcpy(i2c_data, &i2c_inst->fpga_i2c_buf.wr_buf[1], (num_wr - 1));
        bf_fpga_i2c_reg_write32(
            i2c_ctrl, Bf_FPGA_I2C_INST_DATA_LO(inst_id), i2c_data[0]);
        bf_fpga_i2c_reg_write32(
            i2c_ctrl, Bf_FPGA_I2C_INST_DATA_HI(inst_id), i2c_data[1]);
      } else {
        int len = num_wr - 1;
        uint32_t addr;
        uint8_t *val = (uint8_t *)(&i2c_inst->fpga_i2c_buf.wr_buf[1]);
        /* store the data area pointer */
        i2c_data[0] = BF_FPGA_I2C_DATA_AREA(inst_id);
        addr = i2c_data[0];
        bf_fpga_i2c_reg_write32(
            i2c_ctrl, Bf_FPGA_I2C_INST_DATA_LO(inst_id), i2c_data[0] / 4);
        /* copy the data in data area */
        while (len--) {
          bf_fpga_i2c_reg_write8(i2c_ctrl, addr, *val);
          addr++;
          val++;
        }
      }
      break;
    default:
      return BF_FPGA_EINVAL;
  }
  bf_fpga_i2c_reg_write32(i2c_ctrl, Bf_FPGA_I2C_INST_PARAM(inst_id), wd1);
  bf_fpga_i2c_reg_write32(i2c_ctrl, Bf_FPGA_I2C_INST_CTRL(inst_id), wd0);
  return BF_FPGA_OK;
}

/* get the i2c completion status of a particular instruction */
static uint32_t fpga_i2c_get_status(int bus_id, int inst_id) {
  fpga_i2c_controller_t *i2c_ctrl = fpga_i2c_ctrl_get(bus_id);
  uint32_t addr = Bf_FPGA_I2C_INST_CTRL(inst_id);
  return (bf_fpga_i2c_reg_read32(i2c_ctrl, addr) & I2C_STATUS_MASK);
}

/** FPGA I2C data read (assumes locked by caller and no need to stop i2c)
 *
 * read the data following a read type i2c operation
 *
 *  @param bus_id
 *    i2c controller id
 *  @param inst_id
 *    instruction id within  this controller space
 *  @param offset
 *    offset in the data-area where read-data is available
 *  @param num_rd
 *    number of bytes to read
 *  @param rd_buf
 *     buffer to read into
 *  @return
 *    0 on success and <0 on error
 */
static int fpga_i2c_data_read_locked(fpga_i2c_controller_t *i2c_ctrl,
                                     int inst_id,
                                     uint8_t offset,
                                     uint8_t num_rd,
                                     uint8_t *rd_buf) {
  uint8_t i;
  uint32_t addr, data_cnt;

  if (!i2c_ctrl || !rd_buf || !num_rd || inst_id < 0 ||
      inst_id >= FPGA_I2C_NUM_INST) {
    return BF_FPGA_EINVAL;
  }
  /* find out the wr_cnt + rd_cnt from the already executed instruction field */
  data_cnt = bf_fpga_i2c_reg_read32(i2c_ctrl, Bf_FPGA_I2C_INST_PARAM(inst_id));
  /* point to data area if the (wr_cnt + rd_cnt) > 8 */
  data_cnt &= 0xffff; /* retain only the length fields */
  if (((data_cnt & 0xff) + (data_cnt >> 8)) <= 8) {
    addr = Bf_FPGA_I2C_INST_DATA_LO(inst_id) + offset;
  } else {
    addr = BF_FPGA_I2C_DATA_AREA(inst_id) + offset;
  }
  for (i = 0; i < num_rd; i++) {
    *rd_buf = bf_fpga_i2c_reg_read8(i2c_ctrl, addr);
    addr++;
    rd_buf++;
  }
  return BF_FPGA_OK;
}

/** FPGA I2C data read
 *
 * read the data following a read type i2c operation
 *
 *  @param bus_id
 *    i2c controller id
 *  @param inst_id
 *    instruction id within  this controller space
 *  @param offset
 *    offset in the data-area where read-data is available
 *  @param num_rd
 *    number of bytes to read
 *  @param rd_buf
 *     buffer to read into
 *  @return
 *    0 on success and <0 on error
 */
int fpga_i2c_data_read(
    int bus_id, int inst_id, uint8_t offset, uint8_t num_rd, uint8_t *rd_buf) {
  fpga_i2c_controller_t *i2c_ctrl = fpga_i2c_ctrl_get(bus_id);
  int ret;
  bool i2c_running;
  uint8_t val;

  if (!i2c_ctrl || !rd_buf || !num_rd || inst_id < 0 ||
      inst_id >= FPGA_I2C_NUM_INST) {
    return BF_FPGA_EINVAL;
  }

  /* aligned (upto) 4 bytes can be read without stopping the ongoing i2c,
   * this is guaranteed by FPGA design. i2c has to be stopped, in all other
   * cases, to  read a consistent set of read-data.
   */
  if ((offset % 4 == 0) && (num_rd <= 4)) {
    return (
        fpga_i2c_data_read_locked(i2c_ctrl, inst_id, offset, num_rd, rd_buf));
  }

  /* non-aligned case; stop i2c if running, read data and restart i2c */
  if (bf_fpga_i2c_lock(i2c_ctrl)) {
    return BF_FPGA_EAGAIN;
  }
  /* check if i2c_controller is running */
  val = bf_fpga_i2c_reg_read8(i2c_ctrl, Bf_FPGA_TOP_I2C_STATUS);
  i2c_running = ((val & I2C_STS_BUSY) ? true : false);
  if (i2c_running) {
    /* stop ongoing i2c operations */
    fpga_i2c_stop_locked(i2c_ctrl);
  }
  ret = fpga_i2c_data_read_locked(i2c_ctrl, inst_id, offset, num_rd, rd_buf);
  if (i2c_running) {
    /*  restart ongoing i2c operations */
    fpga_i2c_start_locked(i2c_ctrl);
  }
  bf_fpga_i2c_unlock(i2c_ctrl);
  return ret;
}

/** FPGA I2C onetime i2c operation
 *
 *  @param i2c_op
 *    bf_fpga_i2c_t parameters * <in/out>
 *  @return
 *    0 on success and <0 on error
 */
int fpga_i2c_oneshot(bf_fpga_i2c_t *i2c_op) {
  int i, ret;
  uint32_t val;
  int bus_id;
  fpga_i2c_controller_t *i2c_ctrl;

  if (!i2c_op) {
    return BF_FPGA_EINVAL;
  }

  bus_id = i2c_op->inst_hndl.bus_id;
  i2c_ctrl = fpga_i2c_ctrl_get(bus_id);

  if (i2c_op->num_i2c == 0 || i2c_op->num_i2c >= FPGA_I2C_ONESHOT_NUM_INST ||
      i2c_op->one_time == 0 || bus_id >= BF_I2C_FPGA_NUM_CTRL || !i2c_ctrl) {
    return BF_FPGA_EINVAL;
  }
  if (bf_fpga_i2c_lock(i2c_ctrl)) {
    return BF_FPGA_EAGAIN;
  }
  /* stop ongoing i2c operations */
  ret = fpga_i2c_stop_locked(i2c_ctrl);
  if (ret) {
    bf_fpga_i2c_unlock(i2c_ctrl);
    return ret;
  }
  /* populate one time i2c operation instruction(s) from offset zero */
  for (i = 0; i < i2c_op->num_i2c; i++) {
    ret = fpga_i2c_enqueue(bus_id, i, &i2c_op->i2c_inst[i]);
    if (ret) {
      goto oneshot_error_exit;
    }
  }
  /* start i2c operations */
  ret = fpga_i2c_start_locked(i2c_ctrl);
  if (ret) {
    goto oneshot_error_exit;
  }

  /* wait until complete and read the data if necessary */
  for (i = 0; i < i2c_op->num_i2c; i++) {
    int cnt;
    val = 0;
    /* cnt is roughly the number of bytes of this i2c cycle
     * overhead of 100 bytes for for worst case timeout, one
     * should not hit that in normal working case
     */
    cnt = i2c_op->i2c_inst[i].wr_cnt + i2c_op->i2c_inst[i].rd_cnt;
    /* bump up the cnt for an i2c transaction containing  some data
     * for computing worst case timeout */
    if (cnt > 0) {
      cnt = cnt + 100;
    }
    while (!(val & I2C_STATUS_COMPLETED) && (cnt-- > 0)) {
      /* 1 byte ~= 10 bits takes 25 microsec on i2c cycle at 400khz */
      bf_fpga_us_delay(50);
      val = fpga_i2c_get_status(bus_id, i);
    }
    i2c_op->i2c_inst[i].status = val; /* store the h/w status */
    if (val & I2C_STATUS_ERR_MASK) {
      ret = BF_FPGA_EIO;
      goto oneshot_error_exit;
    }
    if (i2c_op->i2c_inst[i].rd_cnt) {
      uint8_t offset = 0;
      if (i2c_op->i2c_inst[i].wr_cnt > 1) {
        offset = i2c_op->i2c_inst[i].wr_cnt - 1;
      }
      if (fpga_i2c_data_read_locked(i2c_ctrl,
                                    i,
                                    offset,
                                    i2c_op->i2c_inst[i].rd_cnt,
                                    i2c_op->i2c_inst[i].fpga_i2c_buf.rd_buf)) {
        ret = BF_FPGA_EIO;
        goto oneshot_error_exit;
      }
    }
  }
  ret = BF_FPGA_OK;

oneshot_error_exit:
  for (i = 0; i < i2c_op->num_i2c; i++) {
    /* cleanup the enable bit */
    val = bf_fpga_i2c_reg_read32(i2c_ctrl, Bf_FPGA_I2C_INST_CTRL(i));
    val &= (~I2C_INST_EN);
    bf_fpga_i2c_reg_write32(i2c_ctrl, Bf_FPGA_I2C_INST_CTRL(i), val);
  }
  bf_fpga_i2c_unlock(i2c_ctrl);
  return ret;
}

/** FPGA I2C insert periodic i2c operation
 *
 *  @param i2c_op
 *    bf_fpga_i2c_t parameters * <in/out>
 *  @return
 *    0 on success and <0 on error
 */
int fpga_i2c_pr_add(bf_fpga_i2c_t *i2c_op) {
  fpga_i2c_controller_t *i2c_ctrl;
  int i, ret, next_id;
  bool preemt;

  if (!i2c_op) {
    return BF_FPGA_EINVAL;
  }
  i2c_ctrl = fpga_i2c_ctrl_get(i2c_op->inst_hndl.bus_id);
  if (!i2c_ctrl) {
    return BF_FPGA_EINVAL;
  }
  if (bf_fpga_i2c_lock(i2c_ctrl)) {
    return BF_FPGA_EAGAIN;
  }
  /* get the next available free slot */
  next_id = get_next_free_index(i2c_ctrl, i2c_op->num_i2c, true);
  if (next_id < 0) {
    bf_fpga_i2c_unlock(i2c_ctrl);
    return BF_FPGA_EBUSY;
  }
  /* populate periodic i2c operation instruction(s) */
  for (i = 0; i < i2c_op->num_i2c; i++) {
    preemt = ((i == (i2c_op->num_i2c - 1)) ? false : true);
    i2c_op->i2c_inst[i].preemt = preemt;
    ret = fpga_i2c_enqueue(
        i2c_op->inst_hndl.bus_id, next_id + i, &i2c_op->i2c_inst[i]);

    if (ret) {
      bf_fpga_i2c_unlock(i2c_ctrl);
      return ret;
    }
  }
  bf_fpga_i2c_unlock(i2c_ctrl);
  i2c_op->inst_hndl.inst_id = next_id;
  return BF_FPGA_OK;
}

/** FPGA I2C remove periodic i2c operation(s) from instruction memory
 *
 *  @param i2c_op
 *    bf_fpga_i2c_t parameters * <in/out>
 *  @return
 *    0 on success and <0 on error
 */
int fpga_i2c_del(bf_fpga_i2c_t *i2c_op) {
  fpga_i2c_controller_t *i2c_ctrl;
  int i, inst_id;

  if (!i2c_op) {
    return BF_FPGA_EINVAL;
  }
  i2c_ctrl = fpga_i2c_ctrl_get(i2c_op->inst_hndl.bus_id);
  if (!i2c_ctrl) {
    return BF_FPGA_EINVAL;
  }
  inst_id = i2c_op->inst_hndl.inst_id;
  if (bf_fpga_i2c_lock(i2c_ctrl)) {
    return BF_FPGA_EAGAIN;
  }
  for (i = 0; i < i2c_op->num_i2c; i++) {
    /* nullify the instruction */
    bf_fpga_i2c_reg_write32(i2c_ctrl, Bf_FPGA_I2C_INST_CTRL(inst_id + i), 0);
  }
  /* reset the in_use flag */
  release_index(i2c_ctrl, inst_id, i2c_op->num_i2c);
  bf_fpga_i2c_unlock(i2c_ctrl);
  return BF_FPGA_OK;
}
