/*
 * Copyright (C) 2025 Nexthop Systems Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 */

#define __STDC_WANT_LIB_EXT1__ 1

#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/jiffies.h>
#include <linux/kernel.h>
#include <linux/mdio.h>
#include <linux/mii.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/phy.h>
#include <linux/string.h>
#include "../../../../pddf/i2c/modules/include/pddf_multifpgapci_defs.h"
#include "../../../../pddf/i2c/modules/include/pddf_multifpgapci_mdio_defs.h"

// MDIO Control Register
#define FPGA_MDIO_CTRL_REG_OFFSET        0x0000

// Bit definitions for MDIO Control Register
#define FPGA_MDIO_CTRL_WRITE_CMD_BIT_POS 31
#define FPGA_MDIO_CTRL_READ_CMD_BIT_POS  30

#define FPGA_MDIO_CTRL_OP_SHIFT          26
#define FPGA_MDIO_CTRL_OP_MASK           0x3

#define FPGA_MDIO_OP_ADDRESS_VAL         0x0
#define FPGA_MDIO_OP_WRITE_VAL           0x1
#define FPGA_MDIO_OP_READ_VAL            0x3
#define FPGA_MDIO_OP_POSTREAD_VAL        0x2

#define FPGA_MDIO_CTRL_PHY_ADDR_SHIFT    21
#define FPGA_MDIO_CTRL_DEV_ADDR_SHIFT    16
#define FPGA_MDIO_CTRL_DATA_SHIFT        0
#define FPGA_MDIO_CTRL_DATA_MASK         0xFFFF

// MDIO Read Data Register
#define FPGA_MDIO_READ_DATA_REG_OFFSET   0x0004

// Bit definitions for MDIO Read Data Register
#define FPGA_MDIO_READ_DATA_VALID_BIT    BIT(31)
#define FPGA_MDIO_READ_DATA_BUSY_BIT     BIT(30)
#define FPGA_MDIO_READ_DATA_MASK         0xFFFF

#define MDIO_DEVAD_SHIFT                 16

static int fpga_mdio_wait_for_idle(struct fpga_mdio_priv *priv)
{
	// Timeout after 100ms if busy bit has not cleared
	unsigned long timeout = jiffies + msecs_to_jiffies(100);
	uint32_t read_data_reg;

	do {
		read_data_reg = ioread32(priv->reg_base +
					 FPGA_MDIO_READ_DATA_REG_OFFSET);
		if (!(read_data_reg & FPGA_MDIO_READ_DATA_BUSY_BIT)) {
			// Busy bit is cleared, transaction is complete
			return 0;
		}
		usleep_range(5, 10);
	} while (time_before(jiffies, timeout));

	pddf_dbg(
		MULTIFPGA,
		KERN_ERR
		"[%s]: MDIO transaction timed out waiting for busy bit to clear\n",
		__FUNCTION__);
	return -ETIMEDOUT;
}

static int fpga_mdio_do_transaction(struct fpga_mdio_priv *priv, int phy_addr,
				    int dev_addr, uint32_t op_val,
				    uint16_t data_val, uint32_t cmd_bit)
{
	uint32_t cmd_val =
		(BIT(cmd_bit) | (op_val << FPGA_MDIO_CTRL_OP_SHIFT) |
		 ((phy_addr & 0x1F) << FPGA_MDIO_CTRL_PHY_ADDR_SHIFT) |
		 ((dev_addr & 0x1F) << FPGA_MDIO_CTRL_DEV_ADDR_SHIFT) |
		 ((data_val & FPGA_MDIO_CTRL_DATA_MASK)
		  << FPGA_MDIO_CTRL_DATA_SHIFT));

	iowrite32(cmd_val, priv->reg_base + FPGA_MDIO_CTRL_REG_OFFSET);
	int ret = fpga_mdio_wait_for_idle(priv);
	if (ret < 0)
		return ret;

	iowrite32(0x0, priv->reg_base + FPGA_MDIO_CTRL_REG_OFFSET);
	return fpga_mdio_wait_for_idle(priv);
}

int fpga_mdio_read(struct mii_bus *bus, int phy_addr, int reg_num)
{
	struct fpga_mdio_priv *priv = bus->priv;
	int ret = -EIO;

	if (!priv || !priv->reg_base) {
		pddf_dbg(
			MULTIFPGA,
			KERN_ERR
			"[%s]: Bus private data (reg_base) not properly set for bus %s\n",
			__FUNCTION__, dev_name(&bus->dev));
		return -EINVAL;
	}

	// Extract the device and reg addresses from reg_num
	int dev_addr = (reg_num >> MDIO_DEVAD_SHIFT) & 0x1F;
	int c45_reg_addr = reg_num & 0xFFFF;

	mutex_lock(&priv->lock);

	// Clause 45 Handling

	// Step 1: Write the register address
	ret = fpga_mdio_do_transaction(priv, phy_addr, dev_addr,
				       FPGA_MDIO_OP_ADDRESS_VAL, c45_reg_addr,
				       FPGA_MDIO_CTRL_WRITE_CMD_BIT_POS);
	if (ret < 0)
		goto out;

	// Step 2: Read the data from the register
	ret = fpga_mdio_do_transaction(priv, phy_addr, dev_addr,
				       FPGA_MDIO_OP_READ_VAL, 0,
				       FPGA_MDIO_CTRL_READ_CMD_BIT_POS);
	if (ret < 0)
		goto out;

	// Poll for Read Data Valid and read data
	unsigned long timeout = jiffies + msecs_to_jiffies(100);
	do {
		uint32_t read_data_reg = ioread32(
			priv->reg_base + FPGA_MDIO_READ_DATA_REG_OFFSET);
		if (read_data_reg & FPGA_MDIO_READ_DATA_VALID_BIT) {
			ret = (read_data_reg >> FPGA_MDIO_CTRL_DATA_SHIFT) &
			      FPGA_MDIO_READ_DATA_MASK;
			goto out;
		}
		usleep_range(10, 20);
	} while (time_before(jiffies, timeout));

	pddf_dbg(
		MULTIFPGA,
		KERN_ERR
		"[%s]: C45 READ data not valid within timeout for bus %s, PHY 0x%x, MMD 0x%x\n",
		__FUNCTION__, dev_name(&bus->dev), phy_addr, dev_addr);
	ret = -ETIMEDOUT;

out:
	mutex_unlock(&priv->lock);
	return ret;
}
EXPORT_SYMBOL(fpga_mdio_read);

int fpga_mdio_write(struct mii_bus *bus, int phy_addr, int reg_num,
		    uint16_t val)
{
	struct fpga_mdio_priv *priv = bus->priv;
	int ret;

	if (!priv || !priv->reg_base) {
		pddf_dbg(
			MULTIFPGA,
			KERN_ERR
			"[%s]: Bus private data (reg_base) not properly set for bus %s\n",
			__FUNCTION__, dev_name(&bus->dev));
		return -EINVAL;
	}

	// Extract the device and reg addresses from reg_num
	int dev_addr = (reg_num >> MDIO_DEVAD_SHIFT) & 0x1F;
	int c45_reg_addr = reg_num & 0xFFFF;

	mutex_lock(&priv->lock);

	// Clause 45 Handling

	// Step 1: Write the register address
	ret = fpga_mdio_do_transaction(priv, phy_addr, dev_addr,
				 FPGA_MDIO_OP_ADDRESS_VAL, c45_reg_addr,
				 FPGA_MDIO_CTRL_WRITE_CMD_BIT_POS);
	if (ret < 0)
		goto out;

	// Step 2: Write the data to the register
	ret = fpga_mdio_do_transaction(priv, phy_addr, dev_addr,
				 FPGA_MDIO_OP_WRITE_VAL, val,
				 FPGA_MDIO_CTRL_WRITE_CMD_BIT_POS);
	if (ret < 0)
		goto out;

out:
	mutex_unlock(&priv->lock);
	return 0;
}
EXPORT_SYMBOL(fpga_mdio_write);

static struct mdio_fpga_ops fpga_algo_ops_instance = {
	.read = fpga_mdio_read,
	.write = fpga_mdio_write,
};

static int __init pddf_custom_mdio_algo_init(void)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s]\n", __FUNCTION__);
	mdio_fpga_algo_ops = &fpga_algo_ops_instance;
	return 0;
}

static void __exit pddf_custom_mdio_algo_exit(void)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s]\n", __FUNCTION__);
	mdio_fpga_algo_ops = NULL;
	return;
}

module_init(pddf_custom_mdio_algo_init);
module_exit(pddf_custom_mdio_algo_exit);

MODULE_DESCRIPTION("Custom algorithm for Nexthop FPGAPCIe MDIO implementation");
MODULE_VERSION("1.0.0");
MODULE_LICENSE("GPL");
