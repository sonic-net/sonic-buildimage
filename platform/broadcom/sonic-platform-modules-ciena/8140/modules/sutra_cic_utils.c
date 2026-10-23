/*
 * This file is part of Ciena’s Siril mod
 *
 * Copyright (C) 2022 Ciena Corporation
 * Author: Marc St-Amand <mstamand@ciena.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <linux/module.h>
#include "sutra_cic.h"

#ifdef __KERNEL__
#include <linux/stddef.h>
#else
#include <stddef.h>
#endif

/* -------------------------------------------------------------------------- */
#define QUOTE(str) #str
#define QUOTE_PIN(str) QUOTE(str)

static const char * const gpio_labels[SUTRA_INT_MAX] = {

	// ---- SUTRA_GLUE_ISR_SW_I2C ----		//  bit hwirq
	QUOTE_PIN(SUTRA_SW_I2C_MB_DONE),		//    0     0
	QUOTE_PIN(SUTRA_SW_I2C_PS_DONE),		//    1     1
	QUOTE_PIN(SUTRA_SW_I2C_PCIE_DONE),		//    2     2
	QUOTE_PIN(SUTRA_SW_I2C_PMBUS_DONE),		//    3     3
	NULL,						//    4     4
	NULL,						//    5     5
	NULL,						//    6     6
	NULL,						//    7     7

	// ---- SUTRA_GLUE_ISR_MISC ----		//  bit hwirq
	NULL,						//    0     8
	QUOTE_PIN(SUTRA_MISC_USB),			//    1     9
	QUOTE_PIN(SUTRA_MISC_TPM),			//    2    10
	QUOTE_PIN(SUTRA_MISC_CPU_QUAD1),		//    3    11
	QUOTE_PIN(SUTRA_MISC_CPU_BOARD_WARM_ALERT),	//    4    12
	QUOTE_PIN(SUTRA_MISC_CPU_BOARD_HOT_ALERT),	//    5    13
	NULL,						//    6    14
	NULL,						//    7    15

	// ---- SUTRA_GLUE_ISR_PWR ----			//  bit hwirq
	QUOTE_PIN(PWR_PSA_ABS),				//    0    16
	QUOTE_PIN(PWR_PSB_ABS),				//    1    17
	QUOTE_PIN(PWR_PSA_PWR_OK),			//    2    18
	QUOTE_PIN(PWR_PSB_PWR_OK),			//    3    19
	QUOTE_PIN(PWR_PSA_IN_OK_N),			//    4    20
	QUOTE_PIN(PWR_PSB_IN_OK_N),			//    5    21
	NULL,						//    6    22
	NULL,						//    7    23

	// ---- SUTRA_GLUE_ISR_BUTTON ----		//  bit hwirq
	QUOTE_PIN(PB_RESET_PRESS_DETECT),		//    0    24
	QUOTE_PIN(PB_RESET_HELD_FOR_3_SECONDS),		//    1    25
	NULL,						//    2    26
	NULL,						//    3    27
	NULL,						//    4    28
	NULL,						//    5    29
	NULL,						//    6    30
	NULL,						//    7    31

}; // end gpio_labels

const char *sutra_cic_pin_to_str(uint32_t pin, uint32_t level)
{
	if (pin < SUTRA_INT_MAX)
		return gpio_labels[pin];
	else
		return NULL;
}

bool sutra_cic_add_gpio_lkup(uint32_t pin, uint32_t level)
{
	switch (pin) {
	case SUTRA_INT_SW_I2C_MB_DONE:
	case SUTRA_INT_SW_I2C_PS_DONE:
	case SUTRA_INT_SW_I2C_PCIE_DONE:
	case SUTRA_INT_SW_I2C_PMBUS_DONE:
	case SUTRA_INT_MISC_USB:
		return true;
	default:
		break;
	}
	return false;
}

bool sutra_cic_active_low(struct device *dev, uint32_t pin, uint32_t level)
{
	switch (pin) {
	case SUTRA_INT_PWR_PSA_ACOK_H:
	case SUTRA_INT_PWR_PSB_ACOK_H:
		return true;
	default:
		break;
	}
	return false;
}
