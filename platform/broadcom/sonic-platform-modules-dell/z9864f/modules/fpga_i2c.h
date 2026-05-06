#ifndef __FGPA_I2C_H__
#define __FGPA_I2C_H__

#include "fpga.h"

extern int num_i2c_adapter;
int i2c_adapter_init(struct pci_dev *dev, struct fpga_dev *fpga);

#define DELTA_I2C_WAIT_BUS_TIMEOUT 100000 /* 100000us = 100ms */
#define DELTA_I2C_OFFSET 0x1000
#define DELTA_I2C_BASE(s) ((DELTA_I2C_OFFSET) + ((0x300) * (s)))
#define DELTA_I2C_CONF(s)  ((s) + 0x0 )
#define DELTA_I2C_ADDR(s)  ((s) + 0x8 )
#define DELTA_I2C_CTRL(s)  ((s) + 0x4 )
#define DELTA_I2C_DATA(s)  ((s) + 0x100 )

#define DELTA_DPLL_I2C_BASE  0x300
#define DELTA_FPGA_I2C_BASE  0x600

#define DELTA_I2C_GRABBER_OFFSET 0x1000
#define DELTA_I2C_GRABBER_CONF(s) ((DELTA_I2C_GRABBER_OFFSET) + ((0x300) * (s)))
#define DELTA_I2C_GRABBER_ADDR(s) ((DELTA_I2C_GRABBER_OFFSET) + ((0x300) * (s)) + 0x8)
#define DELTA_I2C_GRABBER_CTRL(s) ((DELTA_I2C_GRABBER_OFFSET) + ((0x300) * (s)) + 0x4)
#define DELTA_I2C_GRABBER_DATA(s) ((DELTA_I2C_GRABBER_OFFSET) + ((0x300) * (s)) + 0x100)

#define I2C_BUS_READY 0x4
#define I2C_TRANS_FAIL 0x2
#define I2C_TRANS_ENABLE 0x1
#define DELTA_FPGA_I2C_START_OFFSET 0
#define DELTA_FPGA_I2C_RW_OFFSET 3
#define DELTA_FPGA_I2C_REG_LEN_OFFSET 8
#define DELTA_FPGA_I2C_CH_SEL_OFFSET 10
#define DELTA_FPGA_I2C_CH_EN_OFFSET 13
#define DELTA_FPGA_I2C_DATA_LEN_OFFSET 15
#define DELTA_FPGA_I2C_SLAVE_OFFSET 25


#endif /* __FGPA_I2C_H__ */