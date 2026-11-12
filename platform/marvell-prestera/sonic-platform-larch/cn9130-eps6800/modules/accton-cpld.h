/*
 * accton_cpld.h
 *
 *  Created on: 2021年11月16日
 *      Author: rei
 */

#ifndef _LINUX_ACCTON_CPLD_H_
#define _LINUX_ACCTON_CPLD_H_

enum cpld_type {
	CTRL_CPLD,
	PORT_CPLD,
	PORT_CPLD2,
};

int accton_cpld_read(unsigned short cpld_type, u8 reg);
int accton_cpld_write(unsigned short cpld_type, u8 reg, u8 val);
int accton_cpld_write_with_mask(unsigned short cpld_type, u8 reg, u8 val, u8 mask);

int accton_cpld_i2c_read(unsigned short cpld_type, u8 reg);
int accton_cpld_i2c_write(unsigned short cpld_type, u8 reg, u8 val);
int accton_cpld_i2c_write_with_mask(unsigned short cpld_type, u8 reg, u8 val, u8 mask);

#endif /* _LINUX_ACCTON_CPLD_H_ */
