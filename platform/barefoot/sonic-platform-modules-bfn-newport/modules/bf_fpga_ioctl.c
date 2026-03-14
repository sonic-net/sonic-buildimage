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
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/cdev.h>
#include <linux/ioctl.h>
#include <linux/uaccess.h>
#include "bf_fpga_priv.h"
#include "bf_fpga_ioctl.h"
#include "i2c/bf_fpga_i2c.h"

int bf_fpga_ioctl(struct bf_pci_dev *bfdev,
                  unsigned int cmd,
                  unsigned long arg) {
  void *addr = (void __user *)arg;
  int ret = -1;

  if (!bfdev || !addr) {
    return -EFAULT;
  }

  switch (cmd) {
    case BF_FPGA_IOCTL_I2C_CTL: {
      bf_fpga_i2c_ctl_t i2c_ctl;
      bool busy;

      if (copy_from_user(&i2c_ctl, addr, sizeof(bf_fpga_i2c_ctl_t))) {
        return -EFAULT;
      }
      switch (i2c_ctl.control_type) {
        case BF_FPGA_I2C_START:
          ret = fpga_i2c_start(i2c_ctl.inst_hndl.bus_id);
          break;
        case BF_FPGA_I2C_STOP:
          ret = fpga_i2c_stop(i2c_ctl.inst_hndl.bus_id);
          break;
        case BF_FPGA_I2C_RESET:
          ret = fpga_i2c_reset(i2c_ctl.inst_hndl.bus_id);
          break;
        case BF_FPGA_I2C_BUSY:
          ret = fpga_i2c_is_busy(i2c_ctl.inst_hndl.bus_id, &busy);
          if (ret == 0) {
            if (copy_to_user(&(((bf_fpga_i2c_ctl_t *)addr)->is_busy),
                             &busy,
                             sizeof(busy))) {
              return -EFAULT;
            }
          }
          break;
        case BF_FPGA_I2C_INST_EN:
          ret = fpga_i2c_inst_en(
              i2c_ctl.inst_hndl.bus_id, i2c_ctl.inst_hndl.inst_id, true);
          break;
        case BF_FPGA_I2C_INST_DIS:
          ret = fpga_i2c_inst_en(
              i2c_ctl.inst_hndl.bus_id, i2c_ctl.inst_hndl.inst_id, false);
          break;
        case BF_FPGA_I2C_INST_PMT:
          break;
        case BF_FPGA_I2C_INST_STOP_ON_ERR:
          break;
        case BF_FPGA_I2C_INST_INT_EN:
          break;
        default:
          break;
      }
      break;
    }
    case BF_FPGA_IOCTL_I2C_SET_CLK: {
      bf_fpga_i2c_set_clk_t i2c_clk;
      if (copy_from_user(&i2c_clk, addr, sizeof(bf_fpga_i2c_set_clk_t))) {
        return -EFAULT;
      }
      ret = fpga_i2c_set_clk(i2c_clk.bus_id, i2c_clk.clock_div);
      break;
    }
    case BF_FPGA_IOCTL_I2C_ONETIME: {
      bf_fpga_i2c_t i2c_op;
      int i;

      if (copy_from_user(&i2c_op, addr, sizeof(bf_fpga_i2c_t))) {
        return -EFAULT;
      }
      ret = fpga_i2c_oneshot(&i2c_op);
      if (ret == 0) {
        /* copy read data to user area */
        for (i = 0; i < i2c_op.num_i2c; i++) {
          if (i2c_op.i2c_inst[i].rd_cnt) {
            if (copy_to_user(
                    &(((bf_fpga_i2c_t *)addr)->i2c_inst[i].fpga_i2c_buf.rd_buf),
                    &i2c_op.i2c_inst[i].fpga_i2c_buf.rd_buf,
                    i2c_op.i2c_inst[i].rd_cnt)) {
              return -EFAULT;
            }
          }
        }
      } else {
        printk(KERN_ERR
               "fpga i2c ioctl oneshot bus %d error %d i2c_addr 0x%hhx:0x%hhx "
               "i2c_status 0x%hhx:0x%hhx\n",
               i2c_op.inst_hndl.bus_id,
               ret,
               i2c_op.i2c_inst[0].i2c_addr,
               i2c_op.i2c_inst[1].i2c_addr,
               i2c_op.i2c_inst[0].status,
               i2c_op.i2c_inst[1].status);
      }
      break;
    }
    case BF_FPGA_IOCTL_I2C_ADD_PR: {
      bf_fpga_i2c_t i2c_op;
      if (copy_from_user(&i2c_op, addr, sizeof(bf_fpga_i2c_t))) {
        return -EFAULT;
      }
      ret = fpga_i2c_pr_add(&i2c_op);
      if (ret == 0) {
        /* copy read data to user area */
        if (copy_to_user(&((bf_fpga_i2c_t *)addr)->inst_hndl.inst_id,
                         &i2c_op.inst_hndl.inst_id,
                         sizeof(i2c_op.inst_hndl.inst_id))) {
          return -EFAULT;
        }
      } else {
        printk(KERN_ERR "fpga i2c ioctl add-pr error %d on bus %d\n",
               ret,
               i2c_op.inst_hndl.bus_id);
      }
      break;
    }
    case BF_FPGA_IOCTL_I2C_DEL_PR: {
      bf_fpga_i2c_t i2c_op;
      if (copy_from_user(&i2c_op, addr, sizeof(bf_fpga_i2c_t))) {
        return -EFAULT;
      }
      ret = fpga_i2c_del(&i2c_op);
      if (ret != 0) {
        printk(KERN_ERR "fpga i2c ioctl del-pr error %d  on bus %d\n",
               ret,
               i2c_op.inst_hndl.bus_id);
      }
      break;
    }
    case BF_FPGA_IOCTL_I2C_RD_DATA: {
      bf_fpga_i2c_rd_data_t i2c_rd_data;
      /* get user supplied offset and rd_cnt */
      if (copy_from_user(
              &i2c_rd_data, addr, offsetof(bf_fpga_i2c_rd_data_t, rd_buf))) {
        return -EFAULT;
      }
      ret = fpga_i2c_data_read(i2c_rd_data.inst_hndl.bus_id,
                               i2c_rd_data.inst_hndl.inst_id,
                               i2c_rd_data.offset,
                               i2c_rd_data.rd_cnt,
                               i2c_rd_data.rd_buf);
      if (ret == 0) {
        if (copy_to_user(&(((bf_fpga_i2c_rd_data_t *)addr)->rd_buf),
                         &i2c_rd_data.rd_buf,
                         i2c_rd_data.rd_cnt)) {
          return -EFAULT;
        }
      } else {
        printk(KERN_ERR "fpga i2c ioctl rd-data error %d on bus %d inst %d\n",
               ret,
               i2c_rd_data.inst_hndl.bus_id,
               i2c_rd_data.inst_hndl.inst_id);
      }
      break;
    }
    default:
      return -EINVAL;
  }
  return ret;
}
