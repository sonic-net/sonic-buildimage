/*
 * I2C multiplexer driver for accton CPLD
 *
 * Copyright (C) 2015 Accton Technology Corporation.
 * Ang Liu <ang_liu@accton.com.cn>
 *
 * Based on:
 *	pca954x.h
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _LINUX_I2C_MUX_ACCTON_CPLD_H
#define _LINUX_I2C_MUX_ACCTON_CPLD_H

/* Platform data for the accton CPLD I2C multiplexers */

struct accton_cpld_mux_platform_data {
	int num_modes;
	unsigned int mux_type;
};

#endif /* _LINUX_I2C_MUX_ACCTON_CPLD_H */
