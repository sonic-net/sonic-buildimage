/*
 *  Quanta IX8 platform driver
 *
 *
 *  Copyright (C) 2017 Quanta Computer inc.
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

#include <linux/version.h>
#include <linux/kernel.h>
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
#include <linux/delay.h>
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,16,0))
#include <linux/i2c/pca953x.h>
#else
#include <linux/platform_data/pca953x.h>
#endif

//MB Board Data
static struct pca953x_platform_data pca9555_1_data = {
	.gpio_base = 0x10,
};
//QSFP28 49-56 IO Expander
static struct pca953x_platform_data pca9698_2_data = {
	.gpio_base = 0x20,
};
//CPU Board pca9555
static struct pca953x_platform_data pca9555_CPU_data = {
	.gpio_base = 0x48,
};
static struct i2c_board_info ix8_i2c_devices[] = {
	{
		I2C_BOARD_INFO("pca9546", 0x72),		// 0
	},
	{
		I2C_BOARD_INFO("pca9548", 0x77),		// 1
	},
	{
		I2C_BOARD_INFO("24c02", 0x54),			// 2 eeprom
	},
	{
		I2C_BOARD_INFO("pca9548", 0x73),		// 3 0x77 ch0
	},
	{
		I2C_BOARD_INFO("pca9548", 0x73),		// 4 0x77 ch1
	},
	{
		I2C_BOARD_INFO("pca9548", 0x73),		// 5 0x77 ch2
	},
	{
		I2C_BOARD_INFO("pca9548", 0x73),		// 6 0x77 ch3
	},
	{
		I2C_BOARD_INFO("pca9548", 0x73),		// 7 0x77 ch4
	},
	{
		I2C_BOARD_INFO("pca9548", 0x73),		// 8 0x77 ch5
	},
	{
		I2C_BOARD_INFO("pca9548", 0x73),		// 9 0x77 ch6
	},
	{
		I2C_BOARD_INFO("CPLD-SFP28", 0x38),		// 10 0x72 ch0 CPLD1_:SFP28 1~16
	},
	{
		I2C_BOARD_INFO("CPLD-SFP28", 0x38),		// 11 0x72 ch1 CPLD2_:SFP28 17~32
	},
	{
		I2C_BOARD_INFO("CPLD-SFP28", 0x38),		// 12 0x72 ch2 CPLD_3:SFP28 33~48
	},
	{
		I2C_BOARD_INFO("pca9555", 0x23),		// 13 0x72 ch3 MB Board Data
		.platform_data = &pca9555_1_data,
	},
	{
		I2C_BOARD_INFO("pca9698", 0x21),		// 14 0x72 ch3 QSFP:49~52
		.platform_data = &pca9698_2_data,
	},
	{
		I2C_BOARD_INFO("optoe1", 0x50),          // 15 0x50 QSFP EEPROM
	},
	{
		I2C_BOARD_INFO("pca9546", 0x71),		// 16
	},
	{
		I2C_BOARD_INFO("pca9555", 0x20),		// 17 0x71 ch0 CPU Board Data
		.platform_data = &pca9555_CPU_data,
	},
	{
		I2C_BOARD_INFO("CPLDLED_IX8", 0x3a),	// 18 0x72 ch0 CPLD4 LED function of SFP28 & QSFP28 (Port27~56)
	},
	{
		I2C_BOARD_INFO("CPLDLED_IX8", 0x39),	// 19 0x72 ch0 CPLD6 LED function of SFP28 & QSFP28 (Port1~26)
	},
	{
		I2C_BOARD_INFO("optoe2", 0x50),          // 20 0x50 SFP28 EEPROM
	},
};

static struct platform_driver ix8_platform_driver = {
	.driver = {
		.name = "qci-ix8",
		.owner = THIS_MODULE,
	},
};

static struct i2c_adapter *i2c_get_adapter_wait(int nr)
{
	struct i2c_adapter *adap = NULL;
	int i = 0;

	for (i = 0; i < 300; ++i) {
		adap = i2c_get_adapter(nr);
		if (adap)
			break;
		msleep(10);
	}

	if (adap == NULL)
		printk(KERN_ERR "%s: unable to get i2c adapter for bus %d\n", __FILE__, nr);

	return adap;
}

static struct platform_device *ix8_device;

static struct i2c_client **g_client;
static struct i2c_client **g_client_port;
int numof_i2c_devices = 19; // num of ix8_i2c_devices - 2 (for optoe1, optoe2)
int numof_ports = 56;

static int __init ix8_platform_init(void)
{
	struct i2c_adapter *adapter;
	int ret, i;

	ret = platform_driver_register(&ix8_platform_driver);
	if (ret < 0)
		return ret;

	/* Register platform stuff */
	ix8_device = platform_device_alloc("qci-ix8", -1);
	if (!ix8_device) {
		ret = -ENOMEM;
		goto fail_platform_driver;
	}

	ret = platform_device_add(ix8_device);
	if (ret)
		goto fail_platform_device;

	g_client = kmalloc(sizeof(*g_client) * numof_i2c_devices, GFP_KERNEL);
	for (i = 0; i < numof_i2c_devices; i++) g_client[i] = NULL;

	g_client_port = kmalloc(sizeof(*g_client_port) * numof_ports, GFP_KERNEL);
	for (i = 0; i < numof_ports; i++) g_client_port[i] = NULL;

	adapter = i2c_get_adapter_wait(0);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[0] = i2c_new_client_device(adapter, &ix8_i2c_devices[0]);		// pca9546
		g_client[1] = i2c_new_client_device(adapter, &ix8_i2c_devices[1]);		// pca9548
		g_client[2] = i2c_new_client_device(adapter, &ix8_i2c_devices[16]);		// pca9546cpu
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter_wait(13);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[3] = i2c_new_client_device(adapter, &ix8_i2c_devices[17]);		// CPU Board Data
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter_wait(1);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[4] = i2c_new_client_device(adapter, &ix8_i2c_devices[10]);		// CPLD_1
		g_client[5] = i2c_new_client_device(adapter, &ix8_i2c_devices[18]);		// CPLD_4
		g_client[6] = i2c_new_client_device(adapter, &ix8_i2c_devices[19]);		// CPLD_6
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter_wait(2);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[7] = i2c_new_client_device(adapter, &ix8_i2c_devices[11]);		// CPLD_2
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter_wait(3);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[8] = i2c_new_client_device(adapter, &ix8_i2c_devices[12]);		// CPLD_3
		g_client[9] = i2c_new_client_device(adapter, &ix8_i2c_devices[2]);		// MB_BOARDINFO_EEPROM
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter_wait(4);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[10] = i2c_new_client_device(adapter, &ix8_i2c_devices[13]);		// MB Board Data
		g_client[11] = i2c_new_client_device(adapter, &ix8_i2c_devices[14]);		// QSFP:49~52
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter_wait(5);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[12] = i2c_new_client_device(adapter, &ix8_i2c_devices[3]);		// pca9548_1 SFP
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter_wait(6);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[13] = i2c_new_client_device(adapter, &ix8_i2c_devices[4]);		// pca9548_2 SFP
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter_wait(7);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[14] = i2c_new_client_device(adapter, &ix8_i2c_devices[5]);		// pca9548_3 SFP
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter_wait(8);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[15] = i2c_new_client_device(adapter, &ix8_i2c_devices[6]);		// pca9548_4 SFP
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter_wait(9);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[16] = i2c_new_client_device(adapter, &ix8_i2c_devices[7]);		// pca9548_5 SFP
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter_wait(10);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[17] = i2c_new_client_device(adapter, &ix8_i2c_devices[8]);		// pca9548_6 SFP
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter_wait(11);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[18] = i2c_new_client_device(adapter, &ix8_i2c_devices[9]);		// pca9548_7 QSFP
		i2c_put_adapter(adapter);
	}

	for(i = 65; i < 73; i ++){									// QSFP 49~56 EEPROM
		adapter = i2c_get_adapter_wait(i);
		if (adapter == NULL)
		{
			printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
		}
		else
		{
			g_client_port[i - 17] = i2c_new_client_device(adapter, &ix8_i2c_devices[15]);
			i2c_put_adapter(adapter);
		}
	}

	for(i = 17; i < 65; i ++){									// SFP28 1~48 EEPROM
		if (adapter == NULL)
		{
			printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
		}
		else
		{
			adapter = i2c_get_adapter_wait(i);
			g_client_port[i - 17] = i2c_new_client_device(adapter, &ix8_i2c_devices[20]);
			i2c_put_adapter(adapter);
		}
	}

	return 0;

fail_platform_device:
	platform_device_put(ix8_device);

fail_platform_driver:
	platform_driver_unregister(&ix8_platform_driver);
	return ret;
}

static void __exit ix8_platform_exit(void)
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

	platform_device_unregister(ix8_device);
	platform_driver_unregister(&ix8_platform_driver);
}

module_init(ix8_platform_init);
module_exit(ix8_platform_exit);


MODULE_AUTHOR("Jonathan Tsai <jonathan.tsai@quantatw.com>");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Quanta IX8 Platform Driver");
MODULE_LICENSE("GPL");
