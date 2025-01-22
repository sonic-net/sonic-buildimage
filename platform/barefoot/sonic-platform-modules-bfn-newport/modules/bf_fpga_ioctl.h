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
#ifndef _BF_FPGA_IOCTL_H_
#define _BF_FPGA_IOCTL_H_

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else
#include <sys/ioctl.h>

#endif /* __KERNEL__ */

#define BF_FPGA_IOC_MAGIC 'f'
#define BF_I2C_FPGA_NUM_CTRL 34
#define BF_FPGA_MAX_I2C_RD_DATA 128
#define BF_FPGA_MAX_I2C_WR_DATA 129

/* i2c control types */
#define BF_FPGA_I2C_START 1
#define BF_FPGA_I2C_STOP 2
#define BF_FPGA_I2C_BUSY 3
#define BF_FPGA_I2C_INST_EN 4
#define BF_FPGA_I2C_INST_DIS 5
#define BF_FPGA_I2C_INST_PMT 6
#define BF_FPGA_I2C_INST_STOP_ON_ERR 7
#define BF_FPGA_I2C_INST_INT_EN 8
#define BF_FPGA_I2C_RESET 9

/* i2c operation types */
#define BF_FPGA_I2C_NOP 0
#define BF_FPGA_I2C_WRITE \
  1 /* <i2c-addr-wr> / <wr-d0> <wr-d1>..[max 129 bytes] */
#define BF_FPGA_I2C_READ \
  2 /* <i2c-addr-rd> / <rd-d0> <rd-d1>..[max 128 bytes] */
#define BF_FPGA_I2C_ADDR_READ                                     \
  3 /* <i2c-addr-wr> / <upto 4 bytes of register offset / <R/S> / \
       <i2c-addr-rd> <upto 4 bytes of read data> */

#define FPGA_I2c_ONESHOT_BEGIN_INDEX 0
#define FPGA_I2C_ONESHOT_NUM_INST 15
#define FPGA_I2C_PERIODIC_BEGIN_INDEX (FPGA_I2C_ONESHOT_NUM_INST)
#define FPGA_I2C_PERIODIC_NUM_INST 16
#define FPGA_I2C_NUM_INST \
  (FPGA_I2C_ONESHOT_NUM_INST + FPGA_I2C_PERIODIC_NUM_INST)
/* maximum i2c instructions that can be handled in one system call */
#define BF_FPGA_I2C_MAX_NUM_INST 3

typedef struct bf_fpga_i2c_set_clk_s {
  /* 0:100k, 1:400k, 2:1M, or: 125e6/<desired freq>/3 */
  int clock_div;        /* clock  divider */
  unsigned char bus_id; /* controller index */
} bf_fpga_i2c_set_clk_t;

typedef struct bf_fpga_inst_hndl_s {
  int inst_id; /* instruction index within the controller memory space */
  unsigned char bus_id; /* controller index */
  unsigned char status;
} bf_fpga_inst_hndl_t;

typedef struct bf_fpga_i2c_inst_s {
  bool preemt;
  bool en;
  unsigned char i2c_addr; /* i2c device address in 7 bit format */
  unsigned char i2c_type; /* type of i2c cycle */
  unsigned char delay; /* delay in TBD - microseconds before i2c transaction */
  unsigned char ctrl;
  unsigned char wr_cnt; /* number of bytes to write (after i2c address) */
  unsigned char rd_cnt; /* number of bytes to read */
  union {
    unsigned char
        wr_buf[BF_FPGA_MAX_I2C_WR_DATA]; /* write data source buffer */
    unsigned char rd_buf[BF_FPGA_MAX_I2C_RD_DATA]; /* read data dest buffer */
  } fpga_i2c_buf;
  unsigned char status;
  unsigned char retry_cnt; /* if fpga maintains retry count */
  unsigned char mux;       /* if fpga maintains internal MUX */
} bf_fpga_i2c_inst_t;

typedef struct bf_fpga_i2c_s {
  unsigned char num_i2c;  /* number of i2c operations */
  unsigned char one_time; /* one time or periodic */
  bf_fpga_inst_hndl_t inst_hndl;
  bf_fpga_i2c_inst_t i2c_inst[BF_FPGA_I2C_MAX_NUM_INST];
} bf_fpga_i2c_t;

typedef struct bf_fpga_i2c_rd_data_s {
  bf_fpga_inst_hndl_t inst_hndl;
  unsigned char offset;
  unsigned char rd_cnt;
  unsigned char rd_buf[BF_FPGA_MAX_I2C_RD_DATA];
} bf_fpga_i2c_rd_data_t;

typedef struct bf_fpga_i2c_ctl_s {
  bf_fpga_inst_hndl_t inst_hndl;
  unsigned char control_type; /* start, stop, reset, enable, disable or busy */
  bool is_busy;
} bf_fpga_i2c_ctl_t;

#define BF_FPGA_IOCTL_I2C_CTL _IOWR(BF_FPGA_IOC_MAGIC, 0, bf_fpga_i2c_ctl_t)
#define BF_FPGA_IOCTL_I2C_ONETIME _IOWR(BF_FPGA_IOC_MAGIC, 1, bf_fpga_i2c_t)
#define BF_FPGA_IOCTL_I2C_ADD_PR _IOWR(BF_FPGA_IOC_MAGIC, 2, bf_fpga_i2c_t)
#define BF_FPGA_IOCTL_I2C_DEL_PR _IOWR(BF_FPGA_IOC_MAGIC, 3, bf_fpga_i2c_t)
#define BF_FPGA_IOCTL_I2C_RD_DATA \
  _IOWR(BF_FPGA_IOC_MAGIC, 4, bf_fpga_i2c_rd_data_t)
#define BF_FPGA_IOCTL_I2C_SET_CLK \
  _IOW(BF_FPGA_IOC_MAGIC, 5, bf_fpga_i2c_set_clk_t)

#endif /* _BF_FPGA_IOCTL_H_ */
