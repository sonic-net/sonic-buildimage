/*
 * Centec CpuMac Ethernet Driver -- CpuMac controller implementation
 * Provides Bus interface for MIIM regs
 *
 * Author: liuht <liuht@centecnetworks.com>
 *
 * Copyright 2002-2018, Centec Networks (Suzhou) Co., Ltd.
 *
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/if_vlan.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_mdio.h>
#include <linux/of_platform.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/in.h>
#include <linux/net_tstamp.h>
#include <linux/mii.h>
#include <linux/iopoll.h>

#include <asm/io.h>

#include "ctcmac.h"
#include "ctcmac_reg.h"

struct ctc_mdio_priv {
	void __iomem *map;
	struct MdioSoc_regs *mdio_reg;
};

static int ctc_mdio_write(struct mii_bus *bus, int mii_id, int reg, u16 value)
{
	int ret = 0;
	u32 cmd = 0;
	u32 tmp = 0;
	struct ctc_mdio_priv *priv = (struct ctc_mdio_priv *)bus->priv;

	cmd = CTCMAC_MDIO_CMD_REGAD(reg) | CTCMAC_MDIO_CMD_PHYAD(mii_id)
	    | CTCMAC_MDIO_CMD_OPCODE(1) | CTCMAC_MDIO_CMD_DATA(value);

	writel(cmd, &priv->mdio_reg->MdioSocCmd0[0]);
	writel(1, &priv->mdio_reg->MdioSocCmd0[1]);

	ret = readl_poll_timeout(&priv->mdio_reg->MdioSocStatus0,
				 tmp, tmp & CTCMAC_MDIO_STAT(1), 1000, 10000);

	if (ret < 0) {
		return -1;
	}

	return 0;
}

static int ctc_mdio_read(struct mii_bus *bus, int mii_id, int reg)
{
	int ret = 0;
	u32 cmd = 0;
	u32 status;
	int value = 0;
	struct ctc_mdio_priv *priv = (struct ctc_mdio_priv *)bus->priv;

	cmd = CTCMAC_MDIO_CMD_REGAD(reg) | CTCMAC_MDIO_CMD_PHYAD(mii_id)
	    | CTCMAC_MDIO_CMD_OPCODE(2);

	writel(cmd, &priv->mdio_reg->MdioSocCmd0[0]);
	writel(1, &priv->mdio_reg->MdioSocCmd0[1]);

	ret = readl_poll_timeout(&priv->mdio_reg->MdioSocStatus0,
				 status, status & CTCMAC_MDIO_STAT(1), 1000,
				 10000);
	if (ret < 0) {
		printk(KERN_ERR "ctc_mdio_read1\n");
		return -1;
	}

	value = (readl(&priv->mdio_reg->MdioSocStatus0) & 0xffff);

	return value;
}

static int ctc_mdio_reset(struct mii_bus *bus)
{
	struct ctc_mdio_priv *priv = (struct ctc_mdio_priv *)bus->priv;

	writel(0x91f, &priv->mdio_reg->MdioSocCfg0);

	return 0;
}

static const struct of_device_id ctc_mdio_match[] = {
	{
	 .compatible = "ctc,mdio",
	 },
	{},
};

MODULE_DEVICE_TABLE(of, ctc_mdio_match);

static int ctc_mdio_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct resource res;
	struct ctc_mdio_priv *priv;
	struct mii_bus *new_bus;
	int err;

	new_bus = mdiobus_alloc_size(sizeof(*priv));
	if (!new_bus)
		return -ENOMEM;

	priv = new_bus->priv;
	new_bus->name = "CTC MII Bus", new_bus->read = &ctc_mdio_read;
	new_bus->write = &ctc_mdio_write;
	new_bus->reset = &ctc_mdio_reset;

	err = of_address_to_resource(np, 0, &res);
	if (err < 0) {
		pr_err("Of address to resource fail %d!\n", err);
		goto error;
	}

	snprintf(new_bus->id, MII_BUS_ID_SIZE, "%s@%llx", np->name,
		 (unsigned long long)res.start);

	priv->map = of_iomap(np, 0);
	if (!priv->map) {
		err = -ENOMEM;
		pr_err("of iomap fail %d!\n", err);
		goto error;
	}
	priv->mdio_reg = (struct MdioSoc_regs *)priv->map;
	new_bus->parent = &pdev->dev;
	platform_set_drvdata(pdev, new_bus);

	err = of_mdiobus_register(new_bus, np);
	if (err) {
		pr_err("register mdio bus fail %d!\n", err);
		goto error;
	}

	return 0;

error:
	if (priv->map)
		iounmap(priv->map);

	kfree(new_bus);

	return err;
}

static int ctc_mdio_remove(struct platform_device *pdev)
{
	struct device *device = &pdev->dev;
	struct mii_bus *bus = dev_get_drvdata(device);
	struct ctc_mdio_priv *priv = bus->priv;

	mdiobus_unregister(bus);

	iounmap(priv->map);
	mdiobus_free(bus);

	return 0;
}

static struct platform_driver ctc_mdio_driver = {
	.driver = {
		   .name = "ctc_mdio",
		   .of_match_table = ctc_mdio_match,
		   },
	.probe = ctc_mdio_probe,
	.remove = ctc_mdio_remove,
};

module_platform_driver(ctc_mdio_driver);

MODULE_LICENSE("GPL");
