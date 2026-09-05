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
#ifndef _BF_FPGA_I2C_H
#define _BF_FPGA_I2C_H

/* Allow the use in C++ code. */
#ifdef __cplusplus
extern "C" {
#endif

int fpga_i2c_start(int bus_id);
int fpga_i2c_stop(int bus_id);
int fpga_i2c_reset(int bus_id);
int fpga_i2c_is_busy(int bus_id, bool *is_busy);
int fpga_i2c_inst_en(int bus_id, int inst_id, bool en);
int fpga_i2c_set_clk(int bus_id, int clock_div);
int fpga_i2c_controller_init(int bus_id);
int fpga_i2c_controller_cleanup(int bus_id);
int fpga_i2c_init(uint8_t *base_addr);
void fpga_i2c_deinit(void);
int fpga_i2c_oneshot(bf_fpga_i2c_t *i2c_op);
int fpga_i2c_pr_add(bf_fpga_i2c_t *i2c_op);
int fpga_i2c_del(bf_fpga_i2c_t *i2c_op);
int fpga_i2c_data_read(
    int bus_id, int inst_id, uint8_t offset, uint8_t len, uint8_t *buf);
bool fpga_i2c_is_inited(void);

#ifdef __cplusplus
}
#endif /* C++ */

#endif /* _BF_FPGA_I2C_H */
