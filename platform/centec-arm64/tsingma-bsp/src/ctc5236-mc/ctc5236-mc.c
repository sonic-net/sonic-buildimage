/* 
 * Centec TsingMa Memory Contoller Driver
 *
 * Author: lius <lius@centecnetworks.com>
 *
 * Copyright 2002-2019, Centec Networks (Suzhou) Co., Ltd.
 *
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/ctype.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/io.h>
#include "../include/sysctl.h"
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>

struct ctc5236_mc {
	struct device *dev;
	void __iomem *base;
	int irq;
	int irq1;		/* one bit ecc error irq num, only use in TM1.1 */
	int irq2;		/* more than one bit ecc error irq num, only use in TM1.1 */
	int irq_cache_ecc;	/* cache error interrupt */
	struct regmap *regmap_base;
	unsigned int soc_ver;
};

/* DDR interrupt enable register */
#define DDR_ERR_INT_EN 	0xF0

/* DDR interrupt status register */
#define DDR_ERR_INT_STATUS 	0xF4

/* over top-bound info register*/
#define DDR_ERR_INT_OVER_TOPBOUND_L 	0xF8
#define DDR_ERR_INT_OVER_TOPBOUND_H	0xFC

#define DDR_PORT0_ERR_INT_STATUS		0x1
#define DDR_PORT1_ERR_INT_STATUS		0x2
#define DDR_PORT2_ERR_INT_STATUS		0x3
#define DDR_PORT3_ERR_INT_STATUS		0x4
#define DDR_ERR_ECC_INT_STATUS		0x10000
#define DDR_ERR_CRC_INT_STATUS		0x200000
#define DDR_ERR_WR_PORT_REC_UNDERFLOW		0x20000
#define DDR_ERR_WR_PORT_REC_OVERFLOW		0x40000
#define DDR_ERR_RD_PORT_REC_UNDERFLOW		0x80000
#define DDR_ERR_RD_PORT_REC_OVERFLOW		0x100000

#define DDR_PORT0_STATUS 	0xB0
#define DDR_PORT1_STATUS 	0xB4
#define DDR_PORT2_STATUS 	0xB8
#define DDR_PORT3_STATUS 	0xBC

#define DDR_ERR_OVER_TOPBOUND	0x20000
#define DDR_ERR_WCMDQ_OVER		0x40000
#define DDR_ERR_WCMDQ_UNDER		0x80000
#define DDR_ERR_WDATAQ_OVER		0x100000
#define DDR_ERR_WDATAQ_UNDER	0x200000
#define DDR_ERR_WESPQ_OVER		0x400000
#define DDR_ERR_WESPQ_UNDER		0x800000
#define DDR_ERR_WINFOQ_OVER		0x1000000
#define DDR_ERR_WINFOQ_UNDER		0x2000000
#define DDR_ERR_RCMDQ_OVER		0x4000000
#define DDR_ERR_RCMDQ_UNDER		0x8000000
#define DDR_ERR_RDATAQ_OVER		0x10000000
#define DDR_ERR_RDATAQ_UNDER		0x20000000
#define DDR_ERR_RESPQ_OVER		0x40000000
#define DDR_ERR_RESPQ_UNDER		0x80000000

#define DDR_PORT0_BASE 0xB0
#define DDR_PORT1_BASE 0xB4
#define DDR_PORT2_BASE 0xB8
#define DDR_PORT3_BASE 0xBc

#define DDR_PORT0 	0
#define DDR_PORT1 	1
#define DDR_PORT2 	2
#define DDR_PORT3 	3

static int port_err_status(int status, int port, void *dev_id)
{
	int id = port;
	unsigned long temp = 0;
	unsigned int addr_h = 0;
	unsigned int addr_l = 0;
	struct ctc5236_mc *mci = dev_id;

	/*the  reason of port interrupt  */
	if (status & DDR_ERR_OVER_TOPBOUND) {
		/* get low 32-bit address */
		addr_l = readl(mci->base + DDR_ERR_INT_OVER_TOPBOUND_L);
		/* get high 2-bit address */
		addr_h = readl(mci->base + DDR_ERR_INT_OVER_TOPBOUND_H);
		temp = (addr_l | (((unsigned long)((addr_h >> 12) & 0x3)) << 32)
		    );

		printk(KERN_EMERG
		       "ERROR:port%d is out of top-bound range!\n The error address is 0x%p\n",
		       id, (void *)temp);
	}

	if (status & DDR_ERR_WCMDQ_OVER) {
		printk(KERN_ERR
		       "ERROR:port%d write command queue is overflow!\n", id);
	}

	if (status & DDR_ERR_WCMDQ_UNDER) {
		printk(KERN_ERR
		       "ERROR:port%d write command queue is underflow!\n", id);
	}

	if (status & DDR_ERR_WDATAQ_OVER) {
		printk(KERN_ERR "ERROR:port%d write data queue is overflow!\n",
		       id);
	}

	if (status & DDR_ERR_WDATAQ_UNDER) {
		printk(KERN_ERR "ERROR:port%d write data queue is underflow!\n",
		       id);
	}

	if (status & DDR_ERR_WESPQ_OVER) {
		printk(KERN_ERR
		       "ERROR:port%d write response queue is overflow!\n", id);
	}

	if (status & DDR_ERR_WESPQ_UNDER) {
		printk(KERN_ERR
		       "ERROR:port%d write response queue is underflow!\n", id);
	}

	if (status & DDR_ERR_WINFOQ_OVER) {
		printk(KERN_ERR "ERROR:port%d write info queue is overflow!\n",
		       id);
	}

	if (status & DDR_ERR_WINFOQ_UNDER) {
		printk(KERN_ERR "ERROR:port%d write info queue is underflow!\n",
		       id);
	}

	if (status & DDR_ERR_RCMDQ_OVER) {
		printk(KERN_ERR
		       "ERROR:port%d read command queue is overflow!\n", id);
	}

	if (status & DDR_ERR_RCMDQ_UNDER) {
		printk(KERN_ERR
		       "ERROR:port%d read command queue is underflow!\n", id);
	}

	if (status & DDR_ERR_RDATAQ_OVER) {
		printk(KERN_ERR "ERROR:port%d read data queue is overflow!\n",
		       id);
	}

	if (status & DDR_ERR_RDATAQ_UNDER) {
		printk(KERN_ERR "ERROR:port%d read data queue is underflow!\n",
		       id);
	}

	if (status & DDR_ERR_RESPQ_OVER) {
		printk(KERN_ERR
		       "ERROR:port%d read response queue is overflow!\n", id);
	}

	if (status & DDR_ERR_RESPQ_UNDER) {
		printk(KERN_ERR
		       "ERROR:port%d read response queue is underflow!\n", id);
	}
	return 1;
}

static irqreturn_t ctc_mc_err_handler(int irq, void *dev_id)
{
	struct ctc5236_mc *mci = dev_id;
	unsigned int status;
	unsigned int ret = 0;

	/* get interrupt status */
	status = readl(mci->base + DDR_ERR_INT_STATUS);

	if (status & DDR_PORT0_ERR_INT_STATUS) {
		ret = readl(mci->base + DDR_PORT0_BASE);
		port_err_status(ret, DDR_PORT0, mci);
	}

	if (status & DDR_PORT1_ERR_INT_STATUS) {
		ret = readl(mci->base + DDR_PORT1_BASE);
		port_err_status(ret, DDR_PORT1, mci);
	}

	if (status & DDR_PORT2_ERR_INT_STATUS) {
		ret = readl(mci->base + DDR_PORT2_BASE);
		port_err_status(ret, DDR_PORT2, mci);
	}

	if (status & DDR_PORT3_ERR_INT_STATUS) {
		ret = readl(mci->base + DDR_PORT3_BASE);
		port_err_status(ret, DDR_PORT3, mci);
	}

	if (status & DDR_ERR_ECC_INT_STATUS) {
		printk(KERN_ERR "ERROR:The ecc more than 1-bit error !\n");
	}

	if (status & DDR_ERR_WR_PORT_REC_UNDERFLOW) {
		printk(KERN_ERR "ERROR:MPARB wr_port_rec FIFO is underflow!\n");
	}

	if (status & DDR_ERR_WR_PORT_REC_OVERFLOW) {
		printk(KERN_ERR "ERROR:MPARB wr_port_rec FIFO is overflow!\n");
	}

	if (status & DDR_ERR_RD_PORT_REC_UNDERFLOW) {
		printk(KERN_ERR "ERROR:MPARB rd_port_rec FIFO is underflow!\n");
	}

	if (status & DDR_ERR_RD_PORT_REC_OVERFLOW) {
		printk(KERN_ERR "ERROR:MPARB rd_port_rec FIFO is overflow!\n");
	}

	if (status & DDR_ERR_CRC_INT_STATUS) {
		printk(KERN_ERR "ERROR:The crc error from DRAM!\n");
	}

	/* disable DDR interrupt */
	writel(0x0, mci->base + DDR_ERR_INT_EN);

	return IRQ_HANDLED;
}

static irqreturn_t ctc_mc_onebit_ecc_err_handler(int irq, void *dev_id)
{
	struct ctc5236_mc *mci = dev_id;
	unsigned int val;

	printk(KERN_ERR "ERROR:One-Bit ECC Error!\n");
	regmap_read(mci->regmap_base,
		    offsetof(struct SysCtl_regs, SysDdrEccCtl), &val);
	printk(KERN_ERR "One-Bit ECC Error Count is %d\n", ((val >> 8) & 0xf));
	printk(KERN_ERR "more than One-Bit ECC Error Count is %d\n",
	       ((val >> 12) & 0xf));

	return IRQ_HANDLED;
}

static irqreturn_t ctc_mc_twobit_ecc_err_handler(int irq, void *dev_id)
{
	struct ctc5236_mc *mci = dev_id;
	unsigned int val;

	printk(KERN_ERR "ERROR:more than One-Bit ECC Error!\n");
	regmap_read(mci->regmap_base,
		    offsetof(struct SysCtl_regs, SysDdrEccCtl), &val);
	printk(KERN_ERR "One-Bit ECC Error Count is %d\n", ((val >> 8) & 0xf));
	printk(KERN_ERR "more than One-Bit ECC Error Count is %d\n",
	       ((val >> 12) & 0xf));

	return IRQ_HANDLED;
}

static irqreturn_t ctc_cache_err_handler(int irq, void *dev_id)
{

	printk(KERN_ERR "ERROR:Cache ECC Error!\n");
	return IRQ_HANDLED;
}

static const struct of_device_id ctc5236_ddr_ctrl_of_match[] = {
	{
	 .compatible = "ctc,ctc5236-ddr-ctrl",
	 },
	{},
};

MODULE_DEVICE_TABLE(of, ctc5236_ddr_ctrl_of_match);

static int ctc5236_mc_probe(struct platform_device *pdev)
{
	const struct of_device_id *id;
	struct ctc5236_mc *mci;
	int ret;
	unsigned int val;

	id = of_match_device(ctc5236_ddr_ctrl_of_match, &pdev->dev);
	if (!id)
		return -ENODEV;

	mci = kzalloc(sizeof(*mci), GFP_KERNEL);
	if (!mci)
		return -ENODEV;

	mci->base = devm_ioremap_resource(&pdev->dev, pdev->resource);
	if (IS_ERR(mci->base))
		return PTR_ERR(mci->base);

	mci->regmap_base =
	    syscon_regmap_lookup_by_phandle(pdev->dev.of_node, "ctc,sysctrl");
	if (IS_ERR(mci->regmap_base))
		return PTR_ERR(mci->regmap_base);

	regmap_read(mci->regmap_base,
		    offsetof(struct SysCtl_regs, SysCtlSysRev), &val);
	mci->soc_ver = val;

	mci->irq = platform_get_irq(pdev, 0);
	ret =
	    devm_request_irq(&pdev->dev, mci->irq, ctc_mc_err_handler, 0,
			     "DDR Ecc", mci);
	if (ret < 0) {
		dev_err(&pdev->dev, "Unable to request ddr error irq %d\n",
			mci->irq);
		goto err;
	}

	val = readl(mci->base);
	/* register ecc interrupt when use TM1.1 soc and enable ecc function */
	if ((0x0 != (val & BIT(10))) && (0x1 == mci->soc_ver)) {
		/* clean ecc status */
		regmap_write(mci->regmap_base,
			     offsetof(struct SysCtl_regs, SysDdrEccCtl), 0x1);
		regmap_write(mci->regmap_base,
			     offsetof(struct SysCtl_regs, SysDdrEccCtl), 0x0);

		mci->irq1 = platform_get_irq(pdev, 1);
		ret =
		    devm_request_irq(&pdev->dev, mci->irq1,
				     ctc_mc_twobit_ecc_err_handler, 0,
				     "DDR two-bit Ecc(TM1.1)", mci);
		if (ret < 0) {
			dev_err(&pdev->dev,
				"Unable to request ddr two-bit ecc error irq %d\n",
				mci->irq1);
			goto err;
		}

		mci->irq2 = platform_get_irq(pdev, 2);
		ret =
		    devm_request_irq(&pdev->dev, mci->irq2,
				     ctc_mc_onebit_ecc_err_handler, 0,
				     "DDR one-bit Ecc(TM1.1)", mci);
		if (ret < 0) {
			dev_err(&pdev->dev,
				"Unable to request one-bit ecc error irq %d\n",
				mci->irq2);
			goto err;
		}
	}

	mci->irq_cache_ecc = platform_get_irq(pdev, 3);
	ret =
	    devm_request_irq(&pdev->dev, mci->irq_cache_ecc,
			     ctc_cache_err_handler, 0, "Cache Ecc", mci);
	if (ret < 0) {
		dev_err(&pdev->dev,
			"Unable to request cache ecc error irq %d\n",
			mci->irq_cache_ecc);
		goto err;
	}

	return 0;

err:
	dev_err(&pdev->dev, "Probe Failed!\n");
	return ret;
}

static int ctc5236_mc_remove(struct platform_device *pdev)
{
	struct ctc5236_mc *mci = platform_get_drvdata(pdev);

	devm_iounmap(&pdev->dev, mci->base);
	devm_free_irq(&pdev->dev, mci->irq, mci);

	kfree(mci);
	return 0;
}

static struct platform_driver ctc5236_mc_driver = {
	.probe = ctc5236_mc_probe,
	.remove = ctc5236_mc_remove,
	.driver = {
		   .name = "ctc5236_mc",
		   .of_match_table = ctc5236_ddr_ctrl_of_match,
		   },
};

static int __init ctc5236_mc_init(void)
{
	return platform_driver_register(&ctc5236_mc_driver);
}

static void __exit ctc5236_mc_exit(void)
{
	platform_driver_unregister(&ctc5236_mc_driver);
}

module_init(ctc5236_mc_init);
module_exit(ctc5236_mc_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Centec Network");
MODULE_DESCRIPTION("Centec TsingMa memory contoller driver");
