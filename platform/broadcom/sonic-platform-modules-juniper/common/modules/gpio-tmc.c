/*
 * Juniper Networks TMC GPIO driver
 *
 * Copyright (C) 2020 Juniper Networks
 * Author: Ashish Bhensdadia <bashish@juniper.net>
 *
 * This driver implement the GPIO set/get functionality
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/platform_device.h>
#include <linux/mfd/core.h>
#include "jnx-tmc.h"

#define TMC_GPIO_MAX_BITS_PER_REG	  16
#define TMC_GPIO_SFP_MAX_BITS_PER_REG	  2
#define TMC_GPIO_PTPCFG_MAX_BITS_PER_REG  8

#define TMC_GPIO_FIND_GROUP(gpio)	\
			((gpio) / TMC_GPIO_MAX_BITS_PER_REG)
#define TMC_GPIO_FIND_GPIO(gpio)	\
			((gpio) % TMC_GPIO_MAX_BITS_PER_REG)

#define TMC_GPIO_SFP_FIND_GROUP(gpio)	\
			((gpio) / TMC_GPIO_SFP_MAX_BITS_PER_REG)
#define TMC_GPIO_SFP_FIND_GPIO(gpio)	\
			((gpio) % TMC_GPIO_SFP_MAX_BITS_PER_REG)

#define TMC_GPIO_PTPCFG_FIND_GPIO(gpio)	\
			((gpio) % TMC_GPIO_PTPCFG_MAX_BITS_PER_REG)

#define TMC_GPIO_MAX_NGPIO_PER_GROUP		320

#define TMC_PFE_QSFP_RESET_OFFSET       	0x4
#define TMC_PFE_QSFP_PRESENT_OFFSET		0x8
#define TMC_PFE_QSFP_PHY_RESET_OFFSET		0x10
#define TMC_PFE_QSFP_LPMOD_OFFSET		0x78
#define TMC_PFE_QSFP_LED_CTRL_OFFSET		0x20

#define TMC_PFE_LANES_GREEN_LED_VALUE         0x3
#define TMC_PFE_LANE0_GREEN_LED_BIT_POSITION  0
#define TMC_PFE_LANE1_GREEN_LED_BIT_POSITION  2
#define TMC_PFE_LANE2_GREEN_LED_BIT_POSITION  4
#define TMC_PFE_LANE3_GREEN_LED_BIT_POSITION  6

#define TMC_PFE_LANES_BEACON_LED_VALUE         0x2
#define TMC_PFE_LANE0_BEACON_LED_BIT_POSITION  0
#define TMC_PFE_LANE1_BEACON_LED_BIT_POSITION  2
#define TMC_PFE_LANE2_BEACON_LED_BIT_POSITION  4
#define TMC_PFE_LANE3_BEACON_LED_BIT_POSITION  6

#define TMC_PFE_LANES_FAULT_LED_VALUE         0x1
#define TMC_PFE_LANE0_FAULT_LED_BIT_POSITION  0
#define TMC_PFE_LANE1_FAULT_LED_BIT_POSITION  2
#define TMC_PFE_LANE2_FAULT_LED_BIT_POSITION  4
#define TMC_PFE_LANE3_FAULT_LED_BIT_POSITION  6

#define TMC_PFE_SFPSB0_TX_DISABLE_OFFSET   	0x0
#define TMC_PFE_SFPSB0_LED_CTRL_OFFSET     	0xC
#define TMC_PFE_SFPSB0_LED_ACTIVITY_OFFSET 	0x14
#define TMC_PFE_SFPSB0_PRESENT_OFFSET      	0x18
#define TMC_PFE_SFPSB0_LOSS_OFFSET         	0x1C
#define TMC_PFE_SFPSB0_TX_FAULT_OFFSET     	0x20

#define TMC_PFE_SFPSB1_TX_DISABLE_OFFSET   	0x0
#define TMC_PFE_SFPSB1_LED_CTRL_OFFSET     	0x8
#define TMC_PFE_SFPSB1_LED_ACTIVITY_OFFSET 	0x10
#define TMC_PFE_SFPSB1_PRESENT_OFFSET      	0x14
#define TMC_PFE_SFPSB1_LOSS_OFFSET         	0x18
#define TMC_PFE_SFPSB1_TX_FAULT_OFFSET     	0x1C

/*
 * Index 4 to 15 is used for QSFP starting with
 * QSFP_LED_LANE0_GREEN. To keep multibit set/get common
 * starting SFP_LED_LANE0_GREEN with 16 which will avoid 
 * conflict with QSFP enums.
 */
#define SFP_LED_OP_START_INDEX   	16

/*
 * Used for off-setting SFP led op index
 */
#define SFP_LED_OP_OFFSET   	0xB

/*
 * SFP slave blocks
 */
#define SFP_SLAVE0_BLOCK   	0x1
#define SFP_SLAVE1_BLOCK   	0x2

/*
 * each group represent the 16 gpios.
 * QSFP_RST - QSFP_LPMODE
 * 	each bit represent the one gpio
 *      exemple: bits[0:15] - bit0 - gpio0
 * QSFP_LED_LANE0_GREEN - QSFP_LED_LANE3_FAULT
 *	here, number represent the one gpio
 * 	exemple: bits[0:1]
 *	00 - gpio off, 01 - gpio on [ gpio0]
 *      00 - gpio off, 10 - gpio on [ gpio1]
 *      00 - gpio off, 11 - gpio on [ gpio2]
 *
 */
enum {
	QSFP_RST,
	QSFP_PRESENT,
	QSFP_PHY_RST,
	QSFP_LPMOD,
	QSFP_LED_LANE0_GREEN,
	QSFP_LED_LANE1_GREEN,
	QSFP_LED_LANE2_GREEN,
	QSFP_LED_LANE3_GREEN,
	QSFP_LED_LANE0_BEACON,
	QSFP_LED_LANE1_BEACON,
	QSFP_LED_LANE2_BEACON,
	QSFP_LED_LANE3_BEACON,
	QSFP_LED_LANE0_FAULT,
	QSFP_LED_LANE1_FAULT,
	QSFP_LED_LANE2_FAULT,
	QSFP_LED_LANE3_FAULT,
	TMC_PFE_GPIO_GROUP_MAX
};

enum sfp_op {
	SFP_TX_DISABLE,
	SFP_LED_ACTIVITY,
	SFP_PRESENT,
	SFP_SFP_LOS,
	SFP_TX_FAULT,
	SFP_LED_LANE0_GREEN = SFP_LED_OP_START_INDEX,
	SFP_LED_LANE1_GREEN,
	SFP_LED_LANE2_GREEN,
	SFP_LED_LANE3_GREEN,
	SFP_LED_LANE0_BEACON,
	SFP_LED_LANE1_BEACON,
	SFP_LED_LANE2_BEACON,
	SFP_LED_LANE3_BEACON,
	SFP_LED_LANE0_FAULT,
	SFP_LED_LANE1_FAULT,
	SFP_LED_LANE2_FAULT,
	SFP_LED_LANE3_FAULT,
	TMC_PFE_SFP_GPIO_GROUP_MAX
};

static const u32 group_offset[TMC_PFE_GPIO_GROUP_MAX] = {
			TMC_PFE_QSFP_RESET_OFFSET,
			TMC_PFE_QSFP_PRESENT_OFFSET,
			TMC_PFE_QSFP_PHY_RESET_OFFSET,
			TMC_PFE_QSFP_LPMOD_OFFSET,
			TMC_PFE_QSFP_LED_CTRL_OFFSET, /* LANE0 GREEN */
			TMC_PFE_QSFP_LED_CTRL_OFFSET, /* LANE1 GREEN */
			TMC_PFE_QSFP_LED_CTRL_OFFSET, /* LANE2 GREEN */
			TMC_PFE_QSFP_LED_CTRL_OFFSET, /* LANE3 GREEN */
			TMC_PFE_QSFP_LED_CTRL_OFFSET, /* LANE0 BEACON */
			TMC_PFE_QSFP_LED_CTRL_OFFSET, /* LANE1 BEACON */
			TMC_PFE_QSFP_LED_CTRL_OFFSET, /* LANE2 BEACON */
			TMC_PFE_QSFP_LED_CTRL_OFFSET, /* LANE3 BEACON */
			TMC_PFE_QSFP_LED_CTRL_OFFSET, /* LANE0 FAULT */
			TMC_PFE_QSFP_LED_CTRL_OFFSET, /* LANE1 FAULT */
			TMC_PFE_QSFP_LED_CTRL_OFFSET, /* LANE2 FAULT */
			TMC_PFE_QSFP_LED_CTRL_OFFSET, /* LANE3 FAULT */
};

static const u32 sfp_slaveb0_group_offset[TMC_PFE_SFP_GPIO_GROUP_MAX] = {
			TMC_PFE_SFPSB0_TX_DISABLE_OFFSET,
			TMC_PFE_SFPSB0_LED_ACTIVITY_OFFSET,
			TMC_PFE_SFPSB0_PRESENT_OFFSET,
			TMC_PFE_SFPSB0_LOSS_OFFSET,
			TMC_PFE_SFPSB0_TX_FAULT_OFFSET,
			TMC_PFE_SFPSB0_LED_CTRL_OFFSET, /* LANE0 GREEN */
			TMC_PFE_SFPSB0_LED_CTRL_OFFSET, /* LANE1 GREEN */
			TMC_PFE_SFPSB0_LED_CTRL_OFFSET, /* LANE2 GREEN */
			TMC_PFE_SFPSB0_LED_CTRL_OFFSET, /* LANE3 GREEN */
			TMC_PFE_SFPSB0_LED_CTRL_OFFSET, /* LANE0 BEACON */
			TMC_PFE_SFPSB0_LED_CTRL_OFFSET, /* LANE1 BEACON */
			TMC_PFE_SFPSB0_LED_CTRL_OFFSET, /* LANE2 BEACON */
			TMC_PFE_SFPSB0_LED_CTRL_OFFSET, /* LANE3 BEACON */
			TMC_PFE_SFPSB0_LED_CTRL_OFFSET, /* LANE0 FAULT */
			TMC_PFE_SFPSB0_LED_CTRL_OFFSET, /* LANE1 FAULT */
			TMC_PFE_SFPSB0_LED_CTRL_OFFSET, /* LANE2 FAULT */
			TMC_PFE_SFPSB0_LED_CTRL_OFFSET, /* LANE3 FAULT */
};

static const u32 sfp_slaveb1_group_offset[TMC_PFE_SFP_GPIO_GROUP_MAX] = {
			TMC_PFE_SFPSB1_TX_DISABLE_OFFSET,
			TMC_PFE_SFPSB1_LED_ACTIVITY_OFFSET,
			TMC_PFE_SFPSB1_PRESENT_OFFSET,
			TMC_PFE_SFPSB1_LOSS_OFFSET,
			TMC_PFE_SFPSB1_TX_FAULT_OFFSET,
			TMC_PFE_SFPSB1_LED_CTRL_OFFSET, /* LANE0 GREEN */
			TMC_PFE_SFPSB1_LED_CTRL_OFFSET, /* LANE1 GREEN */
			TMC_PFE_SFPSB1_LED_CTRL_OFFSET, /* LANE2 GREEN */
			TMC_PFE_SFPSB1_LED_CTRL_OFFSET, /* LANE3 GREEN */
			TMC_PFE_SFPSB1_LED_CTRL_OFFSET, /* LANE0 BEACON */
			TMC_PFE_SFPSB1_LED_CTRL_OFFSET, /* LANE1 BEACON */
			TMC_PFE_SFPSB1_LED_CTRL_OFFSET, /* LANE2 BEACON */
			TMC_PFE_SFPSB1_LED_CTRL_OFFSET, /* LANE3 BEACON */
			TMC_PFE_SFPSB1_LED_CTRL_OFFSET, /* LANE0 FAULT */
			TMC_PFE_SFPSB1_LED_CTRL_OFFSET, /* LANE1 FAULT */
			TMC_PFE_SFPSB1_LED_CTRL_OFFSET, /* LANE2 FAULT */
			TMC_PFE_SFPSB1_LED_CTRL_OFFSET, /* LANE3 FAULT */
};

struct tmc_gpio_info {
	int (*get)(struct gpio_chip *, unsigned int);
	void (*set)(struct gpio_chip *, unsigned int, int);
	int (*dirin)(struct gpio_chip *, unsigned int);
	int (*dirout)(struct gpio_chip *, unsigned int, int);
};

struct tmc_gpio_chip {
	const struct tmc_gpio_info *info;
	void __iomem *base;
	struct device *dev;
	struct gpio_chip gpio;
	int ngpio;
	spinlock_t gpio_lock; /* gpio lock */
	int sfp_slave_block;
};

/* slave gpio max */
static int gpio_max = 320;
module_param(gpio_max, int, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
MODULE_PARM_DESC(gpio_max, "Maximum number of gpio for SLAVE TMC GPIO");

/*
 * generic bit operation functions
 */
static u32 tmc_gpio_reset_bits(u32 state, u32 val, u32 shift)
{
	state &= ~(val << shift);
	return state;
};

static u32 tmc_gpio_set_bits(u32 state, u32 val, u32 shift)
{
	state |= (val << shift);
	return state;
};

static u32 tmc_gpio_find_bits_val(u32 state, u32 shift, u32 mask)
{
	return ((state >> shift)) & mask;
};

#define to_tmc_chip(chip) \
	container_of((chip), struct tmc_gpio_chip, gpio)

/*
 * tmc_gpio_multiple_bitsop - Generic TMC GPIO multiple bits operation
 */
static void tmc_gpio_multiple_bitsop(struct tmc_gpio_chip *chip,
				unsigned int gpiono, u32 group, u32 offset, bool set)
{
	u32 gpio_state, led_val, bit_shift;
	unsigned long flags;
	void __iomem *iobase;

	iobase = chip->base + offset;

	dev_dbg(chip->dev, "TMC GPIO multiple bitop group=%u, "
		"gpiono=%u, offet:=%u, set=%u\n", group, gpiono, offset, set);

	spin_lock_irqsave(&chip->gpio_lock, flags);

	switch (group) {
	case QSFP_LED_LANE0_GREEN:
	case SFP_LED_LANE0_GREEN:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		led_val = TMC_PFE_LANES_GREEN_LED_VALUE;
		bit_shift = TMC_PFE_LANE0_GREEN_LED_BIT_POSITION;
		break;
	case QSFP_LED_LANE1_GREEN:
	case SFP_LED_LANE1_GREEN:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		led_val = TMC_PFE_LANES_GREEN_LED_VALUE;
		bit_shift = TMC_PFE_LANE1_GREEN_LED_BIT_POSITION;
		break;
	case QSFP_LED_LANE2_GREEN:
	case SFP_LED_LANE2_GREEN:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		led_val = TMC_PFE_LANES_GREEN_LED_VALUE;
		bit_shift = TMC_PFE_LANE2_GREEN_LED_BIT_POSITION;
		break;
	case QSFP_LED_LANE3_GREEN:
	case SFP_LED_LANE3_GREEN:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		led_val = TMC_PFE_LANES_GREEN_LED_VALUE;
		bit_shift = TMC_PFE_LANE3_GREEN_LED_BIT_POSITION;
		break;
	case QSFP_LED_LANE0_BEACON:
	case SFP_LED_LANE0_BEACON:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		led_val = TMC_PFE_LANES_BEACON_LED_VALUE;
		bit_shift = TMC_PFE_LANE0_BEACON_LED_BIT_POSITION;
		break;
	case QSFP_LED_LANE1_BEACON:
	case SFP_LED_LANE1_BEACON:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		led_val = TMC_PFE_LANES_BEACON_LED_VALUE;
		bit_shift = TMC_PFE_LANE1_BEACON_LED_BIT_POSITION;
		break;
	case QSFP_LED_LANE2_BEACON:
	case SFP_LED_LANE2_BEACON:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		led_val = TMC_PFE_LANES_BEACON_LED_VALUE;
		bit_shift = TMC_PFE_LANE2_BEACON_LED_BIT_POSITION;
		break;
	case QSFP_LED_LANE3_BEACON:
	case SFP_LED_LANE3_BEACON:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		led_val = TMC_PFE_LANES_BEACON_LED_VALUE;
		bit_shift = TMC_PFE_LANE3_BEACON_LED_BIT_POSITION;
		break;
	case QSFP_LED_LANE0_FAULT:
	case SFP_LED_LANE0_FAULT:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		led_val = TMC_PFE_LANES_FAULT_LED_VALUE;
		bit_shift = TMC_PFE_LANE0_FAULT_LED_BIT_POSITION;
		break;
	case QSFP_LED_LANE1_FAULT:
	case SFP_LED_LANE1_FAULT:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		led_val = TMC_PFE_LANES_FAULT_LED_VALUE;
		bit_shift = TMC_PFE_LANE1_FAULT_LED_BIT_POSITION;
		break;
	case QSFP_LED_LANE2_FAULT:
	case SFP_LED_LANE2_FAULT:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		led_val = TMC_PFE_LANES_FAULT_LED_VALUE;
		bit_shift = TMC_PFE_LANE2_FAULT_LED_BIT_POSITION;
		break;
	case QSFP_LED_LANE3_FAULT:
	case SFP_LED_LANE3_FAULT:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		led_val = TMC_PFE_LANES_FAULT_LED_VALUE;
		bit_shift = TMC_PFE_LANE3_FAULT_LED_BIT_POSITION;
		break;

	default:
		spin_unlock_irqrestore(&chip->gpio_lock, flags);
		return;
	}

	if (set) {
		gpio_state = tmc_gpio_reset_bits(gpio_state, 0x3, bit_shift);
		gpio_state = tmc_gpio_set_bits(gpio_state, led_val, bit_shift);
	} else {
		gpio_state = tmc_gpio_reset_bits(gpio_state, 0x3, bit_shift);
	}

	iowrite32(gpio_state, (iobase+(0x004*gpiono)));

	spin_unlock_irqrestore(&chip->gpio_lock, flags);

	return;
};

/*
 * tmc_gpio_one_bitop - Generic TMC GPIO single bit operation
 */
static void tmc_gpio_one_bitop(struct tmc_gpio_chip *chip,
				unsigned int bit, u32 offset, bool set)
{
	u32 gpio_state;
	unsigned long flags;
	void __iomem *iobase;

	iobase = chip->base + offset;

	dev_dbg(chip->dev, "TMC GPIO one bitop bit=%u, offset=%x, "
		"set=%u\n", bit, offset, set);

	spin_lock_irqsave(&chip->gpio_lock, flags);

	gpio_state = ioread32(iobase);
	if (set)
		gpio_state |= BIT(bit);
	else
		gpio_state &= ~BIT(bit);

	iowrite32(gpio_state, iobase);

	spin_unlock_irqrestore(&chip->gpio_lock, flags);

	return;
}

/*
 * tmc_gpio_get_multiple_bitsop - Generic TMC get GPIO multiple bits operation
 */
static int tmc_gpio_get_multiple_bitsop(struct tmc_gpio_chip *chip,
				unsigned int gpiono, u32 group, u32 offset)
{
	u32 gpio_state;
	void __iomem *iobase;

	iobase = chip->base + offset;

	dev_dbg(chip->dev, "TMC GPIO get multiple bitsop group=%u, "
		"gpiono=%u, offset=%u\n", group, gpiono, offset);

	switch (group) {
	case QSFP_LED_LANE0_GREEN:
	case SFP_LED_LANE0_GREEN:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		return (TMC_PFE_LANES_GREEN_LED_VALUE ==
			tmc_gpio_find_bits_val(gpio_state,
			TMC_PFE_LANE0_GREEN_LED_BIT_POSITION, 0x3));
	case QSFP_LED_LANE1_GREEN:
	case SFP_LED_LANE1_GREEN:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		return (TMC_PFE_LANES_GREEN_LED_VALUE ==
			tmc_gpio_find_bits_val(gpio_state,
			TMC_PFE_LANE1_GREEN_LED_BIT_POSITION, 0x3));
	case QSFP_LED_LANE2_GREEN:
	case SFP_LED_LANE2_GREEN:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		return (TMC_PFE_LANES_GREEN_LED_VALUE ==
			tmc_gpio_find_bits_val(gpio_state,
			TMC_PFE_LANE2_GREEN_LED_BIT_POSITION, 0x3));
	case QSFP_LED_LANE3_GREEN:
	case SFP_LED_LANE3_GREEN:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		return (TMC_PFE_LANES_GREEN_LED_VALUE ==
			tmc_gpio_find_bits_val(gpio_state,
			TMC_PFE_LANE3_GREEN_LED_BIT_POSITION, 0x3));
	case QSFP_LED_LANE0_BEACON:
	case SFP_LED_LANE0_BEACON:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		return (TMC_PFE_LANES_BEACON_LED_VALUE ==
			tmc_gpio_find_bits_val(gpio_state,
			TMC_PFE_LANE0_BEACON_LED_BIT_POSITION, 0x3));
	case QSFP_LED_LANE1_BEACON:
	case SFP_LED_LANE1_BEACON:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		return (TMC_PFE_LANES_BEACON_LED_VALUE ==
			tmc_gpio_find_bits_val(gpio_state,
			TMC_PFE_LANE1_BEACON_LED_BIT_POSITION, 0x3));
	case QSFP_LED_LANE2_BEACON:
	case SFP_LED_LANE2_BEACON:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		return (TMC_PFE_LANES_BEACON_LED_VALUE ==
			tmc_gpio_find_bits_val(gpio_state,
			TMC_PFE_LANE2_BEACON_LED_BIT_POSITION, 0x3));
	case QSFP_LED_LANE3_BEACON:
	case SFP_LED_LANE3_BEACON:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		return (TMC_PFE_LANES_BEACON_LED_VALUE ==
			tmc_gpio_find_bits_val(gpio_state,
			TMC_PFE_LANE3_BEACON_LED_BIT_POSITION, 0x3));
	case QSFP_LED_LANE0_FAULT:
	case SFP_LED_LANE0_FAULT:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		return (TMC_PFE_LANES_FAULT_LED_VALUE ==
			tmc_gpio_find_bits_val(gpio_state,
			TMC_PFE_LANE0_FAULT_LED_BIT_POSITION, 0x3));
	case QSFP_LED_LANE1_FAULT:
	case SFP_LED_LANE1_FAULT:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		return (TMC_PFE_LANES_FAULT_LED_VALUE ==
			tmc_gpio_find_bits_val(gpio_state,
			TMC_PFE_LANE1_FAULT_LED_BIT_POSITION, 0x3));
	case QSFP_LED_LANE2_FAULT:
	case SFP_LED_LANE2_FAULT:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		return (TMC_PFE_LANES_FAULT_LED_VALUE ==
			tmc_gpio_find_bits_val(gpio_state,
			TMC_PFE_LANE2_FAULT_LED_BIT_POSITION, 0x3));
	case QSFP_LED_LANE3_FAULT:
	case SFP_LED_LANE3_FAULT:
		gpio_state = ioread32(iobase+(0x004*gpiono));
		return (TMC_PFE_LANES_FAULT_LED_VALUE ==
			tmc_gpio_find_bits_val(gpio_state,
			TMC_PFE_LANE3_FAULT_LED_BIT_POSITION, 0x3));
	default:
		return 0;
	}
};

/*
 * tmc_gpio_get - Read the specified signal of the GPIO device.
 */
static int tmc_gpio_get(struct gpio_chip *gc, unsigned int gpio)
{
	struct tmc_gpio_chip *chip = to_tmc_chip(gc);
	unsigned int group = TMC_GPIO_FIND_GROUP(gpio);
	unsigned int bit   = TMC_GPIO_FIND_GPIO(gpio);

	if (group >= TMC_PFE_GPIO_GROUP_MAX)
		return 0;

	switch (group) {
	case QSFP_RST:
	case QSFP_PRESENT:
	case QSFP_PHY_RST:
	case QSFP_LPMOD:
		dev_dbg(chip->dev, "TMC GPIO get one bitop group=%u, gpio=%u, "
			"bit=%u\n", group, gpio, bit);
		return !!(ioread32(chip->base + group_offset[group])
				& BIT(bit));
	default:
		return tmc_gpio_get_multiple_bitsop(chip, bit, group, group_offset[group]);
	}
}

/*
 * tmc_gpio_set - Write the specified signal of the GPIO device.
 */
static void tmc_gpio_set(struct gpio_chip *gc, unsigned int gpio, int val)
{
	struct tmc_gpio_chip *chip = to_tmc_chip(gc);
	unsigned int group = TMC_GPIO_FIND_GROUP(gpio);
	unsigned int bit   = TMC_GPIO_FIND_GPIO(gpio);

	if (group >= TMC_PFE_GPIO_GROUP_MAX)
		return;

	switch (group) {
	case QSFP_RST:
	case QSFP_PRESENT:
	case QSFP_PHY_RST:
	case QSFP_LPMOD:
		dev_dbg(chip->dev, "TMC GPIO one bitop group=%d\n", group);
		tmc_gpio_one_bitop(chip, bit, group_offset[group], val);
		break;
	default:
		tmc_gpio_multiple_bitsop(chip, bit, group, group_offset[group], val);
		break;
	}
}

static struct tmc_gpio_info tmc_gpios[] = {
	{
	    .get = tmc_gpio_get,
	    .set = tmc_gpio_set,
	},
};

static void tmc_gpio_setup(struct tmc_gpio_chip *sgc, int id)
{
	struct gpio_chip *chip = &sgc->gpio;
	const struct tmc_gpio_info *info = sgc->info;

	chip->get		= info->get;
	chip->set		= info->set;
	chip->direction_input	= info->dirin;
	chip->direction_output	= info->dirout;
	chip->dbg_show		= NULL;
	chip->can_sleep		= 0;

        if (id == 0) {
            chip->base = 0;
        } else if (id == 1) {
            chip->base = (gpio_max * id);
        } else { 
	    chip->base	= -1;
        }

	chip->ngpio	= sgc->ngpio;
	chip->label	= dev_name(sgc->dev);
	chip->parent	= sgc->dev;
	chip->owner	= THIS_MODULE;
}

static int tmc_gpio_of_init(struct device *dev,
				struct tmc_gpio_chip *chip)
{
        chip->info = &tmc_gpios[0];
        chip->ngpio = gpio_max;

        return 0;
}

static int tmc_gpio_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct tmc_gpio_chip *chip;
	struct resource *res;
	int ret;

        const struct mfd_cell *cell = mfd_get_cell(pdev);

	dev_dbg(dev, "TMC GPIO probe\n");

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res)
		return -ENODEV;

	dev_info(dev, "TMC GPIO resource 0x%llx, %llu\n",
		 res->start, resource_size(res));

	chip->base = devm_ioremap(dev, res->start, resource_size(res));
	if (!chip->base)
		return -ENOMEM;

	ret = tmc_gpio_of_init(dev, chip);
	if (ret)
		return ret;

	chip->dev = dev;
	spin_lock_init(&chip->gpio_lock);

	tmc_gpio_setup(chip, cell->id);

	ret = gpiochip_add(&chip->gpio);
	if (ret) {
		dev_err(dev,
			"Failed to register TMC gpiochip : %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, chip);
	dev_info(dev, "TMC GPIO registered at 0x%lx, gpiobase: %d\n",
		 (long unsigned)chip->base, chip->gpio.base);

	return 0;
}

static int tmc_gpio_remove(struct platform_device *pdev)
{
	struct tmc_gpio_chip *chip = platform_get_drvdata(pdev);

	gpiochip_remove(&chip->gpio);

	return 0;
}

static struct platform_driver tmc_gpio_driver = {
	.driver = {
		.name = "gpioslave-tmc",
		.owner  = THIS_MODULE,
	},
	.probe = tmc_gpio_probe,
	.remove = tmc_gpio_remove,
};

module_platform_driver(tmc_gpio_driver);

MODULE_DESCRIPTION("Juniper Networks TMC FPGA GPIO driver");
MODULE_AUTHOR("Ashish Bhensdadia <bashish@juniper.net>");
MODULE_LICENSE("GPL");
