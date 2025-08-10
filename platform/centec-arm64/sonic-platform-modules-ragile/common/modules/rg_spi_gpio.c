/*
 * EEPROMs access control driver for display configuration EEPROMs
 * on DigsyMTC board.
 *
 * (C) 2011 DENX Software Engineering, Anatolij Gustschin <agust@denx.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/spi/eeprom.h>
#include <linux/spi/flash.h>

#define GPIO_TPM_CLK	496
#define GPIO_TPM_CS		500
#define GPIO_TPM_DI		499
#define GPIO_TPM_DO		498

#define EE_SPI_BUS_NUM	    1

static struct spi_gpio_platform_data tpm_spi_gpio_data = {
	.sck		= GPIO_TPM_CLK,
	.mosi		= GPIO_TPM_DI,
	.miso		= GPIO_TPM_DO,
	.num_chipselect	= 2,
};

static void spi_gpio_release(struct device *dev)
{
    return;
}
static struct platform_device tpm_device = {
	.name		= "spi_gpio",
	.id		= 3,
	.dev	= {
		.platform_data	= &tpm_spi_gpio_data,
		.release = spi_gpio_release,
	}
};

static struct spi_board_info tpm_info = {
		.modalias		= "tpm_tis_spi",
		.max_speed_hz		= 1000000,
		.bus_num		= 3,
		.chip_select		= 0,     /* 0 ,1 */
		.mode			= SPI_MODE_0,
		.controller_data	= (void *)GPIO_TPM_CS,
};

static int __init tpm_devices_init(void)
{
	int ret;
	struct spi_master *master;

	ret = platform_device_register(&tpm_device);
	if (ret) {
		printk("can't add spi-gpio device \n");
		return ret;
	}

	master = spi_busnum_to_master(tpm_info.bus_num);
    if( !master ) {
        return -ENODEV;
	}
    printk(KERN_INFO "enter tpm_devices_init. \n");

	spi_new_device(master, &tpm_info);
	return 0 ;
}

static void __exit tpm_devices_exit(void)
{
	platform_device_unregister(&tpm_device);
}

module_init(tpm_devices_init);
module_exit(tpm_devices_exit);

MODULE_DESCRIPTION("ragile spi gpio  device Support");
MODULE_AUTHOR("support@ragilenetworks.com");
MODULE_LICENSE("GPL");
