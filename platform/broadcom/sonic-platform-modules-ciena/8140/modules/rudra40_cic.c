/*
 * This file is part of Ciena’s Siril mod
 *
 * Copyright (C) 2022 Ciena Corporation
 * Author: Nikhil Sahu <nsahu@ciena.com>
 *
 * This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <linux/module.h>
#include <linux/platform_device.h> /* platform_*    */
#include "rudra40_cic.h"           /* hwirq numbers */
#include <linux/rudra40_regmap.h>
#include <linux/generic_cic.h>

#define QUOTE(str) #str
#define EXPAND_AND_QUOTE(str) QUOTE(str)

#define REG_ENT( bLOCK, sOURCE, mASTER )   {				\
	.status  = RUDRA40_REG_OFFSET(bLOCK##_STATUS_##sOURCE),		\
	.disable = RUDRA40_REG_OFFSET(bLOCK##_ISM_##sOURCE),		\
	.intr    = RUDRA40_REG_OFFSET(bLOCK##_ISR_##sOURCE),		\
	.test    = RUDRA40_REG_OFFSET(bLOCK##_IST_##sOURCE),		\
	.mask    = bLOCK##_ISR_##sOURCE##____REGMASK,			\
	.name    = EXPAND_AND_QUOTE(sOURCE),				\
	.master  = RUDRA40_GLUE_ISR_MASTER_EVENT_##mASTER##___MASK	\
}

#define REG_ENT_NOSTAT( bLOCK, sOURCE, mASTER )   {			\
	.status  = RUDRA40_REG_OFFSET(bLOCK##_ISR_##sOURCE),		\
	.disable = RUDRA40_REG_OFFSET(bLOCK##_ISM_##sOURCE),		\
	.intr    = RUDRA40_REG_OFFSET(bLOCK##_ISR_##sOURCE),		\
	.test    = RUDRA40_REG_OFFSET(bLOCK##_IST_##sOURCE),		\
	.mask    = bLOCK##_ISR_##sOURCE##____REGMASK,			\
	.name    = EXPAND_AND_QUOTE(sOURCE),				\
	.master  = RUDRA40_GLUE_ISR_MASTER_EVENT_##mASTER##___MASK	\
}

#define REG_ENT_SUM(sOURCE) {						\
	.disable = RUDRA40_REG_OFFSET(RUDRA40_GLUE_ISM_##sOURCE),	\
	.intr    = RUDRA40_REG_OFFSET(RUDRA40_GLUE_ISR_##sOURCE),	\
	.mask    = RUDRA40_GLUE_ISR_##sOURCE##____REGMASK,		\
	.name    = EXPAND_AND_QUOTE(sOURCE)				\
}

static irq_reg_info_t irq_reg_info[] =
{
	REG_ENT(        RUDRA40_GLUE,   MISC,             ANY_MISC          ),
	REG_ENT_NOSTAT( RUDRA40_GLUE,   SW_I2C,           ANY_SW_I2C        ),
	REG_ENT(        RUDRA40_OPTICS, QSFP_PRESENT_0,   ANY_QSFP_PRESENT  ),
	REG_ENT(        RUDRA40_OPTICS, QSFP_LOS_0,       ANY_QSFP_LOS      ),
	REG_ENT(        RUDRA40_OPTICS, QSFP_PWR_GD_0,    ANY_QSFP_PWR_GD   ),
	REG_ENT(        RUDRA40_OPTICS, SFP_RX_LOS,       ANY_SFP_RX_LOS    ),
	REG_ENT(        RUDRA40_OPTICS, SFP_RX_LOS_2,     ANY_SFP_RX_LOS    ),
	REG_ENT(        RUDRA40_OPTICS, SFP_TX_FAULT,     ANY_SFP_TX_FAULT  ),
	REG_ENT(        RUDRA40_OPTICS, SFP_TX_FAULT_2,   ANY_SFP_TX_FAULT  ),
	REG_ENT(        RUDRA40_OPTICS, SFP_PRESENT,      ANY_SFP_PRESENT   ),
	REG_ENT(        RUDRA40_OPTICS, SFP_PRESENT_2,    ANY_SFP_PRESENT   ),
	REG_ENT(        RUDRA40_OPTICS, SFP_PWR_GD,       ANY_SFP_PWR_GD    ),
	REG_ENT(        RUDRA40_OPTICS, SFP_PWR_GD_2,     ANY_SFP_PWR_GD    ),
	REG_ENT(        RUDRA40_OPTICS, SFPDD_RX_LOS,     ANY_SFPDD_RX_LOS  ),
	REG_ENT(        RUDRA40_OPTICS, SFPDD_RX_LOS_2,   ANY_SFPDD_RX_LOS  ),
	REG_ENT(        RUDRA40_OPTICS, SFPDD_TX_FAULT,   ANY_SFPDD_TX_FAULT),
	REG_ENT(        RUDRA40_OPTICS, SFPDD_TX_FAULT_2, ANY_SFPDD_TX_FAULT),
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
		.irq_sources_max      = SIRIL_INT_MAX,
		.irq_reg_table        = irq_reg_info,
		.irq_reg_ignore_table = irq_reg_info_sum
	},
};

uint32_t rudra40_get_master_sum(void)
{
	return (uint32_t) RUDRA40_REG_OFFSET(RUDRA40_GLUE_ISR_MASTER_EVENT);
}

uint32_t rudra40_get_master_mask(void)
{
	return (uint32_t) RUDRA40_REG_OFFSET(RUDRA40_GLUE_ISM_MASTER_EVENT);
}

uint32_t rudra40_get_msi_ctrl(void)
{
	return (uint32_t) RUDRA40_REG_OFFSET(RUDRA40_GLUE_MSI_CTRL);
}

irq_level_t * rudra40_get_interrupt_table(void)
{
	return irq_interrupt_table;
}

#include "rudra40_cic_utils.c"

MODULE_LICENSE("GPL v2");
