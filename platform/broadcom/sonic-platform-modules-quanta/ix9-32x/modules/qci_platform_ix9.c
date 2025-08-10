/*
 *  Quanta IX9 platform driver
 *
 *
 *  Copyright (C) 2019 Quanta Computer inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/leds.h>
#include <linux/platform_device.h>
#include <linux/uaccess.h>
#include <linux/input.h>
#include <linux/input/sparse-keymap.h>
#include <linux/input-polldev.h>
#include <linux/rfkill.h>
#include <linux/slab.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(3,12,0)
#include <linux/platform_data/pca953x.h>
#else
#include <linux/i2c/pca953x.h>
#endif

static struct pca953x_platform_data tca9539_1_data = {
	.gpio_base = 0x10,
};
//CPU Linking Board at CPU's I2C Bus
static struct pca953x_platform_data pca9555_CPU_data = {
	.gpio_base = 0x20,
};

static struct i2c_board_info ix9_i2c_devices[] = {
	{
		I2C_BOARD_INFO("pca9546", 0x72),	// 0 pca9546_1
	},
	{
		I2C_BOARD_INFO("pca9548", 0x77),	// 1 pca9548_1
	},
	{
		I2C_BOARD_INFO("tca9539", 0x74),
		.platform_data = &tca9539_1_data,	// 2 Board ID and QSFP-DD PW EN/PG
	},
	{
		I2C_BOARD_INFO("24c02", 0x54),		// 3 MB_BOARDINFO_EEPROM
	},
	{
		I2C_BOARD_INFO("pca9548", 0x73),	// 4 0x77 ch0 pca9548 #1
	},
	{
		I2C_BOARD_INFO("pca9548", 0x73),	// 5 0x77 ch1 pca9548 #2
	},
	{
		I2C_BOARD_INFO("pca9548", 0x73),	// 6 0x77 ch2 pca9548 #3
	},
	{
		I2C_BOARD_INFO("pca9548", 0x73),	// 7 0x77 ch3 pca9548 #4
	},
	{
		I2C_BOARD_INFO("CPLD-QSFPDD", 0x38),	// 8 0x72 ch0 CPLD-IO #2, #3
	},
	{
		I2C_BOARD_INFO("CPLDLED_IX9", 0x39),	// 9 0x72 ch1 CPLD-LED #4, #5
	},
	{
		I2C_BOARD_INFO("optoe1", 0x50),		// 10 eeprom for loopback module
	},
	{
		I2C_BOARD_INFO("pca9555", 0x22),	// 11 CPU Linking Board at CPU's I2C Bus
		.platform_data = &pca9555_CPU_data,
	},
};

static struct platform_driver ix9_platform_driver = {
	.driver = {
		.name = "qci-ix9",
		.owner = THIS_MODULE,
	},
};

static struct platform_device *ix9_device;

static struct i2c_client **g_client;
static struct i2c_client **g_client_port;
int numof_i2c_devices = 13;
int numof_ports = 32;

static int __init ix9_platform_init(void)
{
	struct i2c_adapter *adapter;
	int ret, i;

	ret = platform_driver_register(&ix9_platform_driver);
	if (ret < 0)
		return ret;

	/* Register platform stuff */
	ix9_device = platform_device_alloc("qci-ix9", -1);
	if (!ix9_device) {
		ret = -ENOMEM;
		goto fail_platform_driver;
	}

	ret = platform_device_add(ix9_device);
	if (ret)
		goto fail_platform_device;

	g_client = kmalloc(sizeof(*g_client) * numof_i2c_devices, GFP_KERNEL);
	for (i = 0; i < numof_i2c_devices; i++) g_client[i] = NULL;

	g_client_port = kmalloc(sizeof(*g_client_port) * numof_ports, GFP_KERNEL);
	for (i = 0; i < numof_ports; i++) g_client_port[i] = NULL;

	adapter = i2c_get_adapter(0);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[0] = i2c_new_client_device(adapter, &ix9_i2c_devices[0]);		// pca9546_1 - Address: 0x72
		g_client[1] = i2c_new_client_device(adapter, &ix9_i2c_devices[1]);		// pca9548_1 - Address: 0x77
		g_client[2] = i2c_new_client_device(adapter, &ix9_i2c_devices[11]);	// CPU Linking Board at CPU's I2C Bus - Address: 0x22
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x01);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[3] = i2c_new_client_device(adapter, &ix9_i2c_devices[8]);		// CPLD-IO #2 - Address: 0x38
		g_client[4] = i2c_new_client_device(adapter, &ix9_i2c_devices[9]);		// CPLD-LED #4 - Address: 0x39
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x02);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[5] = i2c_new_client_device(adapter, &ix9_i2c_devices[8]);		// CPLD-IO #3 - Address: 0x38
		g_client[6] = i2c_new_client_device(adapter, &ix9_i2c_devices[9]);		// CPLD-LED #5 - Address: 0x39
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x03);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[7] = i2c_new_client_device(adapter, &ix9_i2c_devices[3]);		// MB_BOARDINFO_EEPROM - Address: 0x54
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x04);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[8] = i2c_new_client_device(adapter, &ix9_i2c_devices[2]);		// tca9539_1 Board ID and QSFP-DD PW EN/PG - Address: 0x74
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x05);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[9] = i2c_new_client_device(adapter, &ix9_i2c_devices[4]);		// pca9548 #1 QSFPDD - Address: 0x73
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x06);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[10] = i2c_new_client_device(adapter, &ix9_i2c_devices[5]);		// pca9548 #2 QSFPDD - Address: 0x73
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x07);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[11] = i2c_new_client_device(adapter, &ix9_i2c_devices[6]);		// pca9548 #3 QSFPDD - Address: 0x73
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x08);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[12] = i2c_new_client_device(adapter, &ix9_i2c_devices[7]);		// pca9548 #4 QSFPDD - Address: 0x73
		i2c_put_adapter(adapter);
	}

	for(i = 13; i < 45; i++)
	{
		adapter = i2c_get_adapter(i);
		if (adapter == NULL)
		{
			printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
		}
		else
		{
			g_client_port[i - 13] = i2c_new_client_device(adapter, &ix9_i2c_devices[10]);		// eeprom for loopback module - Address: 0x50
			i2c_put_adapter(adapter);
		}
	}

	return 0;

fail_platform_device:
	platform_device_put(ix9_device);

fail_platform_driver:
	platform_driver_unregister(&ix9_platform_driver);
	return ret;
}

static void __exit ix9_platform_exit(void)
{
	int i = 0;

	for (i = numof_ports - 1; i >= 0; i--)
	{
		if (g_client_port[i])
		{
			i2c_unregister_device(g_client_port[i]);
			g_client_port[i] = NULL;
		}
	}

	for (i = numof_i2c_devices - 1; i >= 0; i--)
	{
		if (g_client[i])
		{
			i2c_unregister_device(g_client[i]);
			g_client[i] = NULL;
		}
	}

	platform_device_unregister(ix9_device);
	platform_driver_unregister(&ix9_platform_driver);
}

module_init(ix9_platform_init);
module_exit(ix9_platform_exit);

MODULE_AUTHOR("Jonathan Tsai <jonathan.tsai@quantatw.com>");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Quanta IX9 Platform Driver");
MODULE_LICENSE("GPL");
