/*
 * This file is part of Ciena’s Siril mod
 *
 * Copyright (C) 2021 Ciena Corporation
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
#include <linux/platform_device.h> /* platform_*    */
#include "sutra_cic.h"             /* hwirq numbers */
#include <linux/sutra_regmap.h>
#include <linux/generic_cic.h>

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

#define REG_ENT( bLOCK, sOURCE, mASTER )   {				\
	.status  = SUTRA_REG_OFFSET(bLOCK##_STATUS_##sOURCE),		\
	.disable = SUTRA_REG_OFFSET(bLOCK##_ISM_##sOURCE),		\
	.intr    = SUTRA_REG_OFFSET(bLOCK##_ISR_##sOURCE),		\
	.test    = SUTRA_REG_OFFSET(bLOCK##_IST_##sOURCE),		\
	.mask    = bLOCK##_ISR_##sOURCE##____REGMASK,			\
	.name    = EXPAND_AND_QUOTE(sOURCE),				\
	.master  = SUTRA_GLUE_ISR_MASTER_EVENT_##mASTER##___MASK	\
}

#define REG_ENT_NOSTAT( bLOCK, sOURCE, mASTER )   {			\
	.status  = SUTRA_REG_OFFSET(bLOCK##_ISR_##sOURCE),		\
	.disable = SUTRA_REG_OFFSET(bLOCK##_ISM_##sOURCE),		\
	.intr    = SUTRA_REG_OFFSET(bLOCK##_ISR_##sOURCE),		\
	.test    = SUTRA_REG_OFFSET(bLOCK##_IST_##sOURCE),		\
	.mask    = bLOCK##_ISR_##sOURCE##____REGMASK,			\
	.name    = EXPAND_AND_QUOTE(sOURCE),				\
	.master  = SUTRA_GLUE_ISR_MASTER_EVENT_##mASTER##___MASK	\
}

#define REG_ENT_SUM(sOURCE) {						\
	.disable = SUTRA_REG_OFFSET(SUTRA_GLUE_ISM_##sOURCE),		\
	.intr    = SUTRA_REG_OFFSET(SUTRA_GLUE_ISR_##sOURCE),		\
	.mask    = SUTRA_GLUE_ISR_##sOURCE##____REGMASK,		\
	.name    = EXPAND_AND_QUOTE(sOURCE)				\
}

static irq_reg_info_t irq_reg_info[] =
{
	REG_ENT_NOSTAT( SUTRA_GLUE,   SW_I2C,        ANY_I2C          ),
	REG_ENT(        SUTRA_GLUE,   MISC,          ANY_MISC         ),
	REG_ENT(        SUTRA_GLUE,   PWR,           ANY_POWER        ),
	REG_ENT(        SUTRA_GLUE,   BUTTON,        ANY_BUTTON       ),
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
		.irq_sources_max      = SUTRA_INT_MAX,
		.irq_reg_table        = irq_reg_info,
		.irq_reg_ignore_table = irq_reg_info_sum
	},
};

uint32_t sutra_get_master_sum(void)
{
	return (uint32_t) SUTRA_REG_OFFSET(SUTRA_GLUE_ISR_MASTER_EVENT);
}

uint32_t sutra_get_master_mask(void)
{
	return (uint32_t) SUTRA_REG_OFFSET(SUTRA_GLUE_ISM_MASTER_EVENT);
}

uint32_t sutra_get_msi_ctrl(void)
{
	return 0;
}

irq_level_t * sutra_get_interrupt_table(void)
{
	return irq_interrupt_table;
}
