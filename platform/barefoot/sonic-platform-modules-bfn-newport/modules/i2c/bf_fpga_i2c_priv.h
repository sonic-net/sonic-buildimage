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
#ifndef _BF_FPGA_I2C_PRIV_H
#define _BF_FPGA_I2C_PRIV_H

/* Allow the use in C++ code. */
#ifdef __cplusplus
extern "C" {
#endif

#define FPGA_I2C_INST_OFFSET(idx) (0x10 + (16 * idx))

typedef enum {
  FGPA_I2C_NOP = 0x0,
  FGPA_I2C_WR_ADDR_DATA = 0x1,    /* wr: reg_addr, wr: data */
  FGPA_I2C_WR_ADDR_RD_DATA = 0x2, /* wr: reg_addrss, r/s, rd: data */
  FGPA_I2C_WR_ADDR = 0x3,         /* wr: reg_addr */
  FGPA_I2C_RD_DATA = 0x4,         /* rd: data */
  FGPA_I2C_MULTI_WR_RD = 0x5      /* wr: n bytes, r/s, rd: m bytes */
} i2c_cmd_type;

/* contents of each instruction instance */
typedef struct i2c_inst_cmd_s {
  i2c_cmd_type i2c_cmd; /* i2x cycle type */
  uint32_t data_lo;     /* lower 4 bytes of data associated with this inst */
  uint32_t data_hi;     /* upper 4 bytes of data associated with this inst */
  uint32_t us_delay;    /* delay before i2c cycle */
  uint8_t i2c_addr;     /* i2c device address, in 7 bit format */
  uint8_t reg_addr;     /* 1st write byte, if present in i2c cycle */
  uint8_t num_read;     /* number of bytes to write excluding reg_addr */
  uint8_t num_write;    /* number of bytes to read */
} i2c_inst_cmd_t;

/* attributes of each instruction instance */
typedef struct i2c_inst_s {
  uint32_t inst; /* index of the instruction within the controller memory */
  bool en;       /* is instruction enabled */
  bool preemt;   /* atomically execute next instruction */
  bool int_en;   /* enable interrupt after execution */
  bool in_use;   /* is this instruction currently used */
} i2c_inst_t;

typedef struct fpga_i2c_controller_s {
  bf_fpga_mutex_t fpga_ctrl_lock;
  bf_fpga_fast_lock_t spinlock;
  uint8_t *fpga_base_addr; /* virtual address of start of fpga memory */
  uint8_t *i2c_base_addr;  /* virtual address of i2c controller memory */
  uint32_t start;          /* offset of start of i2c instruction memory */
  uint32_t len; /* number of i2c instructions belonging to this i2c engine */
  uint32_t clk_div; /* clock divider used by this i2c engine */
  bool int_en;
  i2c_inst_t i2c_inst[FPGA_I2C_NUM_INST];
} fpga_i2c_controller_t;

fpga_i2c_controller_t *fpga_i2c_ctrl_get(int bus_id);
void bf_fpga_reg_write(fpga_i2c_controller_t *i2c_ctrl,
                       uint32_t offset,
                       uint32_t val);
uint32_t bf_fpga_reg_read(fpga_i2c_controller_t *i2c_ctrl, uint32_t offset);
void bf_fpga_i2c_reg_write32(fpga_i2c_controller_t *i2c_ctrl,
                             uint32_t offset,
                             uint32_t val);
uint32_t bf_fpga_i2c_reg_read32(fpga_i2c_controller_t *i2c_ctrl,
                                uint32_t offset);
void bf_fpga_i2c_reg_write8(fpga_i2c_controller_t *i2c_ctrl,
                            uint32_t offset,
                            uint8_t val);
uint8_t bf_fpga_i2c_reg_read8(fpga_i2c_controller_t *i2c_ctrl, uint32_t offset);

int bf_fpga_i2c_lock(fpga_i2c_controller_t *i2c_ctrl);

void bf_fpga_i2c_unlock(fpga_i2c_controller_t *i2c_ctrl);

int fpga_i2c_start_locked(fpga_i2c_controller_t *i2c_ctrl);
int fpga_i2c_stop_locked(fpga_i2c_controller_t *i2c_ctrl);

#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _BF_FPGA_I2C_PRIV_H */
