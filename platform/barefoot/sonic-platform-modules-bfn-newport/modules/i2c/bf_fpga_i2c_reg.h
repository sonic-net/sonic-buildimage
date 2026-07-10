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
#ifndef _BF_FPGA_I2C_REG_H
#define _BF_FPGA_I2C_REG_H

/* Allow the use in C++ code. */
#ifdef __cplusplus
extern "C" {
#endif

/* registers outsize of all i2c controller register space */
#define BF_FPGA_I2C_BASE_ADDR 0x0
#define BF_FPGA_I2C_CTRL_BASE_ADDR(i) (BF_FPGA_I2C_BASE_ADDR + (i * 4096))

/* per i2c controller register offset relative to BF_FPGA_I2C_CTRL_BASE_ADDR */
#define Bf_FPGA_I2C_CTRL_TOP 0
#define I2C_CTRL_START (1 << 0)
#define I2C_CTRL_RESET (1 << 1)
#define I2C_CTRL_CLK_DIV_SHF (8)
#define I2C_CTRL_CLK_DIV_MASK (0x1FF)

#define Bf_FPGA_TOP_I2C_STATUS 4
#define I2C_STS_BUSY (1 << 0)
#define I2C_STS_ERR (1 << 3)

#define Bf_FPGA_I2C_INST_CTRL(i) (0x10 + (16 * i))
#define I2C_INST_EN (1 << 31)
#define I2C_INST_PMT (1 << 30)
#define I2C_TYPE_SHF (26)
#define I2C_DELAY_SHF (23)
#define I2C_STOP_ON_ERROR (22)
/* various status values */
#define I2C_STATUS_MASK 0x3F
#define I2C_STATUS_ERR_MASK 0x3C
#define I2C_STATUS_RUNNING 0x1
#define I2C_STATUS_COMPLETED 0x2
#define I2C_STATUS_NACK_ADDR 0x4
#define I2C_STATUS_NACK_CMD 0x8
#define I2C_STATUS_NACK_WR_DATA 0x10
#define I2C_STATUS_TOUT 0x20

/* i2c instruction types */
#define I2C_WR_ADDR_DATA (0 << I2C_TYPE_SHF)
#define I2C_RD_DATA (3 << I2C_TYPE_SHF)
#define I2C_WR_ADDR (2 << I2C_TYPE_SHF)
#define I2C_RD_ADDR_DATA (1 << I2C_TYPE_SHF)
#define I2C_RD_ADDR_DATA_BURST (4 << I2C_TYPE_SHF)
#define I2C_NOP (6 << I2C_TYPE_SHF)

#define Bf_FPGA_I2C_INST_PARAM(i) (0x14 + (16 * i))
#define I2C_DEV_ADDR_SHF (24)
#define I2C_CMD_OFFSET (16)
#define I2C_WR_CNT_SHF (8)
#define I2C_RD_CNT_SHF (0)

#define Bf_FPGA_I2C_INST_DATA_LO(i) (0x18 + (16 * i))
#define Bf_FPGA_I2C_INST_DATA_HI(i) (0x1C + (16 * i))
/******************
#define Bf_FPGA_I2C_PR_CTRL(i) (0x100  + (16 * i))
#define Bf_FPGA_I2C_PR_PARM(i) (0x104  + (16 * i))
#define Bf_FPGA_I2C_PR_LO(i) (0x108  + (16 * i))
#define Bf_FPGA_I2C_PR_HI(i) (0x10C  + (16 * i))
******************/

/* data area pointers */
/* the driver makes fixed static allocation of the available memory
 * on per instruction basis
 * allocate 128 bytes per one time instruction = total 0x780 bytes
 * allocate 64 bytes per one periodic instruction = total 0x400 bytes
 * FPGA_I2C_ONESHOT_NUM_INST -> comes from a header file that must be included
 * before this file.
 */
#define BF_FPGA_ONE_MAX_BURST 128
#define BF_FPGA_PR_MAX_BURST 64
#define BF_FPGA_I2C_DATA_AREA(i)               \
  ((i < FPGA_I2C_ONESHOT_NUM_INST)             \
       ? (0x200 + (i * BF_FPGA_ONE_MAX_BURST)) \
       : (0x980 + ((i - FPGA_I2C_ONESHOT_NUM_INST) * BF_FPGA_PR_MAX_BURST)))

#if BF_FPGA_I2C_DATA_AREA(FPGA_I2C_PERIODIC_NUM_INST) > 0x1000
#error erroneous allocation of FPGA memory to i2c data area. Fix it!
#endif

#define BF_FPGA_VER_REG 0x3F000
#define BF_FPGA_BUILD_DATE 0x3F004
#define BF_FPGA_RESET_CTRL_1 0x3F008
#define BF_FPGA_RESET_CTRL_2 0x3F00C

#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _BF_FPGA_I2C_REG_H */
