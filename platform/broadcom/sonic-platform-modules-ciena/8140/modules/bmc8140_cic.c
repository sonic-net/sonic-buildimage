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
#include <linux/platform_device.h> /* platform_*    */
#include "bmc8140_cic.h"             /* hwirq numbers */
#include <linux/bmc8140_regmap.h>
#include <linux/generic_cic.h>

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

#define REG_ENT( bLOCK, sOURCE, mASTER )   {			   \
	.status  = BMC8140_REG_OFFSET(bLOCK##_STATUS_##sOURCE),      \
	.disable = BMC8140_REG_OFFSET(bLOCK##_ISM_##sOURCE),         \
	.intr    = BMC8140_REG_OFFSET(bLOCK##_ISR_##sOURCE),         \
	.mask    = bLOCK##_ISR_##sOURCE##____REGMASK,              \
	.name    = EXPAND_AND_QUOTE(sOURCE),                       \
	.master  = BMC8140_BASE_ISR_MASTER_EVENT_##mASTER##___MASK   \
}

#define REG_ENT_NOSTAT( bLOCK, sOURCE, mASTER )   {		   \
	.status  = BMC8140_REG_OFFSET(bLOCK##_ISR_##sOURCE),         \
	.disable = BMC8140_REG_OFFSET(bLOCK##_ISM_##sOURCE),         \
	.intr    = BMC8140_REG_OFFSET(bLOCK##_ISR_##sOURCE),         \
	.mask    = bLOCK##_ISR_##sOURCE##____REGMASK,              \
	.name    = EXPAND_AND_QUOTE(sOURCE),                       \
	.master  = BMC8140_GLUE_ISR_MASTER_EVENT_##mASTER##___MASK   \
}

#define REG_ENT_SUM(sOURCE) {					\
	.disable = BMC8140_REG_OFFSET(BMC8140_BASE_ISM_##sOURCE),	\
	.intr    = BMC8140_REG_OFFSET(BMC8140_BASE_ISR_##sOURCE),   \
	.mask    = BMC8140_BASE_ISR_##sOURCE##____REGMASK,        \
	.name    = EXPAND_AND_QUOTE(sOURCE)		        \
}

static irq_reg_info_t irq_reg_info[] =
{
	REG_ENT( BMC8140_BASE, TEMP, ANY_TEMP ),
	REG_ENT( BMC8140_BASE, FAN,  ANY_FANS ),
	REG_ENT( BMC8140_BASE, WDT,  ANY_WDT  ),
	{ .mask = 0 }, /* end of array */
};

static irq_reg_info_t irq_reg_info_sum[] =
{
	REG_ENT_SUM( MASTER_EVENT ),
	{ .mask = 0 }, /* end of array */
};

irq_level_t irq_interrupt_table[] = /* index by level from device tree */
{
	{
		.irq_sources_max      = BMC8140_INT_MAX,
		.irq_reg_table        = irq_reg_info,
		.irq_reg_ignore_table = irq_reg_info_sum
	},
};

uint32_t bmc8140_get_master_sum(void)
{
	return (uint32_t) BMC8140_REG_OFFSET(BMC8140_BASE_ISR_MASTER_EVENT);
}

uint32_t bmc8140_get_master_mask(void)
{
	return (uint32_t) BMC8140_REG_OFFSET(BMC8140_BASE_ISM_MASTER_EVENT);
}

uint32_t bmc8140_get_msi_ctrl(void)
{
	return (uint32_t) BMC8140_REG_OFFSET(BMC8140_BASE_MSI_CTRL);
}

irq_level_t * bmc8140_get_interrupt_table(void)
{
	return irq_interrupt_table;
}
