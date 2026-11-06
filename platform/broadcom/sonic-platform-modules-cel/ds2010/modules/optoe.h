/*
 * Header file for CPLD Optical Transceiver registers
 * (SFP, QSFP and similar I2C based devices)
 */
#ifndef __OPTOE_H
#define __OPTOE_H

/* Board ID register and SKU types */
#define BOARD_ID_REG			0xa209
#define   BOARD_MAGIK_48CP		0x00
#define   BOARD_MAGIK_48C		0x01
#define   BOARD_MAGIK_24CP		0x02
#define   BOARD_MAGIK_24C		0x03
#define   BOARD_SKU_MASK		0x03
/* QSFP module presence status */
#define QSFP28_CTRL_REG1		0xa2a1
#define   QSFP28_MODULE_PRESENT		0x00
#define   QSFP28_MODULE_NOT_PRESENT	0x01
#define   QSFP28_MODULE_PRESENT_MASK	0x3f
/* QSFP module low power status */
#define QSFP28_CTRL_REG2		0xa2a2
#define   QSFP28_MODULE_NOT_LP		0x00
#define   QSFP28_MODULE_LP		0x01
#define   QSFP28_MODULE_LP_MASK		0x3f
/* QSFP module enable status */
#define QSFP28_CTRL_REG3		0xa2a3
#define   QSFP28_MODULE_ENABLED		0x00
#define   QSFP28_MODULE_NOT_ENABLED	0x01
#define   QSFP28_MODULE_ENABLED_MASK	0x3f
/* QSFP module reset */
#define QSFP28_RESET_REG		0xa22f
#define   QSFP28_MODULE_IS_RESET	0x00
#define   QSFP28_MODULE_NOT_RESET	0x01
#define   QSFP28_MODULE_RESET_MASK	0x3f

#define QSFP_PORT_ID(i2c_nr)           ((i2c_nr) - 1)

#define QSFP28_MIN_PORT_NUM		1
#define SKU_1_2_QSFP28_MAX_PORT_NUM	2
#define SKU_3_4_QSFP28_MAX_PORT_NUM	6

#endif /* __OPTOE_H */