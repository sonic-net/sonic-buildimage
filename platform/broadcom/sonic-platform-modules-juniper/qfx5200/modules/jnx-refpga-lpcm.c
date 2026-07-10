/*
 * Juniper Networks Re-Fpga lpc module
 *
 * Copyright (C) 2020 Juniper Networks
 * Author: Ciju Rajan K <crajank@juniper.net>
 *
 * This module implements:
 *  - Registering Reboot handler to reset cpu
 *  - Reset management port
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <linux/ioport.h>

#define REFPGA_LPC_BASE_ADDRESS		0xFED50000
#define REFPGA_LPC_WINDOW_SIZE		0x00000400

#define REFPGA_MRE_LPCM_RST_CTL_REG	(0x3)
#define REFPGA_MAJOR_VERSION		(0x0)
#define REFPGA_MINOR_VERSION		(0x1)

#define REFPGA_CPU_RESET		BIT(0)
#define REFPGA_MGMT1_PHY_RESET		BIT(1)

static void __iomem *fpga = NULL;

static int qfx5200_cpu_reset(struct notifier_block *nb,
				unsigned long action,
				void *data)
{
    int ret = 0;

    switch (action) {
	    case SYS_POWER_OFF:
	    case SYS_HALT:
		    printk(KERN_CRIT "System halt/power_off\n");
		    break;
	    case SYS_RESTART:
		    printk(KERN_CRIT "System restart: qfx5200_cpu_reset\n");
		    iowrite8(REFPGA_CPU_RESET, (u8 *)fpga + REFPGA_MRE_LPCM_RST_CTL_REG);
		    msleep(100);
		    break;
	    default:
		    /* Do Nothing */
		    break;
    }
    return NOTIFY_DONE;
}

static struct notifier_block qfx5200_nb = {
    .notifier_call = qfx5200_cpu_reset,
};

static int __init refpga_lpcm_init(void)
{
	u8 major_version = 0x00;
	u8 minor_version = 0x00;

	if (!request_mem_region(REFPGA_LPC_BASE_ADDRESS, REFPGA_LPC_WINDOW_SIZE, "refpga-lpc")) {
		printk(KERN_ERR "Cannot allocate Re-fpga memory region\n");
		return -ENODEV;
	}

	if ((fpga = ioremap(REFPGA_LPC_BASE_ADDRESS, REFPGA_LPC_WINDOW_SIZE)) == NULL) {
		release_mem_region(REFPGA_LPC_BASE_ADDRESS, REFPGA_LPC_WINDOW_SIZE);
		printk(KERN_ERR "Re-Fpga address mapping failed\n");
		return -1;
	}

	major_version = ioread8((u8 *)fpga + REFPGA_MAJOR_VERSION);
	minor_version = ioread8((u8 *)fpga + REFPGA_MINOR_VERSION);
	printk(KERN_INFO "Re-Fpga major version: %x minor version: %x\n", major_version, minor_version);

	/*
	 * Register the cpld soft reset handler
	 */
	if(register_reboot_notifier(&qfx5200_nb)) {
		printk(KERN_ALERT "Restart handler registration failed\n");
	}

	iowrite8(REFPGA_MGMT1_PHY_RESET, (u8 *)fpga + REFPGA_MRE_LPCM_RST_CTL_REG);

	return 0;

}

static void __exit refpga_lpcm_exit(void)
{
	/*
	 * Unregister the fpga soft reset handler
	 */
	if (unregister_reboot_notifier(&qfx5200_nb)) {
		printk(KERN_CRIT "Failed to uregister reboot notifier\n");
		return;
	}
	iounmap(fpga);
	release_mem_region(REFPGA_LPC_BASE_ADDRESS, REFPGA_LPC_WINDOW_SIZE);
	printk(KERN_INFO "Re-Fpga lpcm module removed\n");
}

module_init(refpga_lpcm_init);
module_exit(refpga_lpcm_exit);

MODULE_DESCRIPTION("Juniper Networks RE-FPGA lpc module");
MODULE_AUTHOR("Ciju Rajan K <crajank@juniper.net>");
MODULE_LICENSE("GPL");
