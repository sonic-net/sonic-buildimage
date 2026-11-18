/*
 * This file is part of Ciena’s Siril mod
 *
 * Copyright (C) 2022 Ciena Corporation
 * Author: Amrit Pal Singh <amrsingh@ciena.com>
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
#include "bmc8140_cic.h"

#ifdef __KERNEL__
#include <linux/stddef.h>
#else
#include <stddef.h>
#endif

/* -------------------------------------------------------------------------- */
#define QUOTE(str) #str
#define QUOTE_PIN(str) QUOTE(str)

static const char * const gpio_labels[BMC8140_INT_MAX] = {
	// ---- BMC8140_BASE_ISR_TEMP ----			Bit	Hwirq
	//
	QUOTE_PIN(OVER_TEMP_THRESHOLD),			   //	 0	  0
	QUOTE_PIN(UNDER_TEMP_THRESHOLD),		   //	 1	  1
	QUOTE_PIN(TMP0_SENSOR_FAULT),			   //	 2	  2
	QUOTE_PIN(TMP0_SENSOR_ERROR),			   //	 3	  3
	QUOTE_PIN(TMP1_SENSOR_FAULT),			   //	 4	  4
	QUOTE_PIN(TMP1_SENSOR_ERROR),			   //	 5	  5
	QUOTE_PIN(TMP2_SENSOR_FAULT),			   //	 6	  6
	QUOTE_PIN(TMP2_SENSOR_ERROR),			   //	 7	  7
	QUOTE_PIN(TMP3_SENSOR_FAULT),			   //	 8	  8
	QUOTE_PIN(TMP3_SENSOR_ERROR),			   //	 9	  9
	QUOTE_PIN(TMP4_SENSOR_FAULT),			   //	10	 10
	QUOTE_PIN(TMP4_SENSOR_ERROR),			   //	11	 11
	QUOTE_PIN(TMP5_SENSOR_FAULT),			   //	12	 12
	QUOTE_PIN(TMP5_SENSOR_ERROR),			   //	13	 13
	QUOTE_PIN(TMP6_SENSOR_FAULT),			   //	14	 14
	QUOTE_PIN(TMP6_SENSOR_ERROR),			   //	15	 15
	QUOTE_PIN(TMP7_SENSOR_FAULT),			   //	16	 16
	QUOTE_PIN(TMP7_SENSOR_ERROR),			   //	17	 17
	QUOTE_PIN(TMP8_SENSOR_FAULT),			   //	18	 18
	QUOTE_PIN(TMP8_SENSOR_ERROR),			   //	19	 19
	NULL,						   //	20	 20
	NULL,						   //	21	 21
	NULL,						   //	22	 22
	NULL,						   //	23	 23
	NULL,						   //	24	 24
	NULL,						   //	25	 25
	NULL,						   //	26	 26
	NULL,						   //	27	 27
	NULL,						   //	28	 28
	NULL,						   //	29	 29
	NULL,						   //	30	 30
	QUOTE_PIN(SYS_TEMP_WITHIN_OTR),			   //	31	 31

	//      ---- BMC8140_BASE_ISR_FAN ----		        Bit	Hwirq
	QUOTE_PIN(FAN0_FAILURE),			   //	 0	 32
	QUOTE_PIN(FAN1_FAILURE),			   //	 1	 33
	QUOTE_PIN(FAN2_FAILURE),			   //	 2	 34
	QUOTE_PIN(FAN3_FAILURE),			   //	 3	 35
	QUOTE_PIN(FAN4_FAILURE),			   //	 4	 36
	QUOTE_PIN(FAN5_FAILURE),			   //	 5	 37
	QUOTE_PIN(FAN6_FAILURE),			   //	 6	 38
	QUOTE_PIN(FAN7_FAILURE),			   //	 7	 39
	QUOTE_PIN(FAN8_FAILURE),			   //	 8	 40
	QUOTE_PIN(FAN9_FAILURE),			   //	 9	 41
	QUOTE_PIN(FAN10_FAILURE),			   //	10	 42
	QUOTE_PIN(FAN11_FAILURE),			   //	11	 43
	QUOTE_PIN(FAN0_POWERGOOD),			   //	12	 44
	QUOTE_PIN(FAN1_POWERGOOD),			   //	13	 45
	QUOTE_PIN(FAN2_POWERGOOD),			   //	14	 46
	QUOTE_PIN(FAN3_POWERGOOD),			   //	15	 47
	QUOTE_PIN(FAN4_POWERGOOD),			   //	16	 48
	QUOTE_PIN(FAN5_POWERGOOD),			   //	17	 49
	QUOTE_PIN(FAN6_POWERGOOD),			   //	18	 50
	QUOTE_PIN(FAN7_POWERGOOD),			   //	19	 51
	QUOTE_PIN(FAN8_POWERGOOD),			   //	20	 52
	QUOTE_PIN(FAN9_POWERGOOD),			   //	21	 53
	QUOTE_PIN(FAN10_POWERGOOD),			   //	22	 54
	QUOTE_PIN(FAN11_POWERGOOD),			   //	23	 55
	QUOTE_PIN(FAN0_FAN1_PRESENT),			   //	24	 56
	QUOTE_PIN(FAN2_FAN3_PRESENT),			   //	25	 57
	QUOTE_PIN(FAN4_FAN5_PRESENT),			   //	26	 58
	QUOTE_PIN(FAN6_FAN7_PRESENT),			   //	27	 59
	QUOTE_PIN(FAN8_FAN9_PRESENT),			   //	28	 60
	QUOTE_PIN(FAN10_FAN11_PRESENT),			   //	29	 61
	NULL,						   //	30	 62
	NULL,						   //	31	 63

	// ---- BMC8140_BASE_ISR_WDT ----			Bit	Hwirq
	QUOTE_PIN(HOST_WDT_TIMEOUT),			   //	 0	 64
	NULL,						   //	 1	 65
	NULL,						   //	 2	 66
	NULL,						   //	 3	 67
	NULL,						   //	 4	 68
	NULL,						   //	 5	 69
	NULL,						   //	 6	 70
	NULL,						   //	 7	 71
	NULL,						   //	 8	 72
	NULL,						   //	 9	 73
	NULL,						   //	10	 74
	NULL,						   //	11	 75
	NULL,						   //	12	 76
	NULL,						   //	13	 77
	NULL,						   //	14	 78
	NULL,						   //	15	 79
	NULL,						   //	16	 80
	NULL,						   //	17	 81
	NULL,						   //	18	 82
	NULL,						   //	19	 83
	NULL,						   //	20	 84
	NULL,						   //	21	 85
	NULL,						   //	22	 86
	NULL,						   //	23	 87
	NULL,						   //	24	 88
	NULL,						   //	25	 89
	NULL,						   //	26	 90
	NULL,						   //	27	 91
	NULL,						   //	28	 92
	NULL,						   //	29	 93
	NULL,						   //	30	 94
	NULL,						   //	31	 95
}; // end gpio_labels

const char *bmc8140_cic_pin_to_str(uint32_t pin, uint32_t level)
{
	if (pin < BMC8140_INT_MAX)
		return gpio_labels[pin];
	else
		return NULL;
}

bool bmc8140_cic_add_gpio_lkup(uint32_t pin, uint32_t level)
{
	switch (pin) {
	case BMC8140_INT_HOST_WDT_TIMEOUT:
		return true;
	default:
		break;
	}
	return false;
}
