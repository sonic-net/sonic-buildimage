/*
 * Juniper Tmc FPGA register definitions
 *
 * Copyright (C) 2018 Juniper Networks
 * Author: Ashish Bhensdadia <bashish@juniper.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __JNX_TMC_H__
#define __JNX_TMC_H__


#define TMC_REVISION				0x00064
#define TMC_MINOR				0x00068
#define TMC_SCRATCH		                0x00098

#define TMC_OPTIC_CPLD_MAJOR                    0x00104
#define TMC_OPTIC_CPLD_MINOR                    0x00108

/*
 * I2C Master Block
 */
#define TMC_I2C_AUTOMATION_I2C_CONTROL_START            0x07000
#define TMC_I2C_AUTOMATION_I2C_CONTROL_END              0x07500

#define TMC_I2C_DPMEM_ENTRY_START                       0x10000
#define TMC_I2C_DPMEM_ENTRY_END                         0x13FFC

#define TMC_LED_CONTROL_START           0x58
#define TMC_LED_CONTROL_END             0x5B

/*
 * RE-FPGA block
 */
#define TMC_REFPGA_ACCESS_START    0x228
#define TMC_REFPGA_ACCESS_END      0x233

#define TMC_I2C_MASTER_NR_MSTRS			16
#define TMC_I2C_MSTR_MAX_GROUPS                 66


/*
 * TMC GPIO SLAVE Block
 */
#define TMC_GPIO_PTP_RESET_START	0x94
#define TMC_GPIO_PTP_RESET_END		0x97

#define TMC_GPIO_PTP_CFG_START		0xa4
#define TMC_GPIO_PTP_CFG_END		0xa7

#define TMC_GPIO_PTP_DATA_START		0xa8
#define TMC_GPIO_PTP_DATA_END		0xab

#define TMC_GPIO_SLAVE0_START		0xf0
#define TMC_GPIO_SLAVE0_END		0x16b

#define TMC_GPIO_SLAVE1_START		0x170
#define TMC_GPIO_SLAVE1_END		0x1eb

#define TMC_GPIO_SLAVE2_START		0x1f0
#define TMC_GPIO_SLAVE2_END		0x213

#define TMC_GPIO_SLAVE3_START		0x280
#define TMC_GPIO_SLAVE3_END		0x2eb

#define TMC_GPIO_SFP_SLAVE0_START		0x308
#define TMC_GPIO_SFP_SLAVE0_END		0x32b

#define TMC_GPIO_SFP_SLAVE1_START		0x32c
#define TMC_GPIO_SFP_SLAVE1_END		0x34b

/*
 * TMC PSU Block
 */
#define TMC_PSU_START			0x240
#define TMC_PSU_END			0x243

/*
 * TMC SHUTDOWN REG
 */
#define TMC_SYS_SHUTDOWN_LOCK		0x254
#define TMC_SYS_SHUTDOWN		0x250

/*
 * TMC DS100 MUX Block
 */
#define TMC_GPIO_MUX_SLAVE_START		0x26c
#define TMC_GPIO_MUX_SLAVE_END		0x26f

#endif /* __JNX_TMC_H__ */
