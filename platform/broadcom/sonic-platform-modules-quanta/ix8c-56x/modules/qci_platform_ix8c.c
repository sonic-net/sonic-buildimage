/*
 *  Quanta IX8C platform driver
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
//CPU Linking Board at CPU's I2C Bus
static struct pca953x_platform_data pca9555_CPU_data = {
	.gpio_base = 0x48,
};

static struct i2c_board_info ix8c_i2c_devices[] = {
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
		I2C_BOARD_INFO("pca9555", 0x22),		// 16 CPU Linking Board at CPU's I2C Bus
		.platform_data = &pca9555_CPU_data,
	},
	{
		I2C_BOARD_INFO("CPLDLED_IX8C", 0x3a),	// 17 0x72 ch0 CPLD4 LED function of SFP28 & QSFP28 (Port27~56)
	},
	{
		I2C_BOARD_INFO("CPLDLED_IX8C", 0x39),	// 18 0x72 ch0 CPLD6 LED function of SFP28 & QSFP28 (Port1~26)
	},
	{
		I2C_BOARD_INFO("optoe2", 0x50),          // 19 0x50 SFP28 EEPROM
	},
};

static struct platform_driver ix8c_platform_driver = {
	.driver = {
		.name = "qci-ix8c",
		.owner = THIS_MODULE,
	},
};

static struct platform_device *ix8c_device;

static struct i2c_client **g_client;
static struct i2c_client **g_client_port;
int numof_i2c_devices = 18;
int numof_ports = 56;

static int __init ix8c_platform_init(void)
{
	struct i2c_adapter *adapter;
	int ret, i;

	ret = platform_driver_register(&ix8c_platform_driver);
	if (ret < 0)
		return ret;

	/* Register platform stuff */
	ix8c_device = platform_device_alloc("qci-ix8c", -1);
	if (!ix8c_device) {
		ret = -ENOMEM;
		goto fail_platform_driver;
	}

	ret = platform_device_add(ix8c_device);
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
		g_client[0] = i2c_new_client_device(adapter, &ix8c_i2c_devices[0]);		// pca9546
		g_client[1] = i2c_new_client_device(adapter, &ix8c_i2c_devices[1]);		// pca9548
		g_client[2] = i2c_new_client_device(adapter, &ix8c_i2c_devices[16]);		// CPU Linking Board at CPU's I2C Bus
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x01);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[3] = i2c_new_client_device(adapter, &ix8c_i2c_devices[10]);		// CPLD_1
		g_client[4] = i2c_new_client_device(adapter, &ix8c_i2c_devices[17]);		// CPLD_4
		g_client[5] = i2c_new_client_device(adapter, &ix8c_i2c_devices[18]);		// CPLD_6
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x02);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[6] = i2c_new_client_device(adapter, &ix8c_i2c_devices[11]);		// CPLD_2
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x03);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[7] = i2c_new_client_device(adapter, &ix8c_i2c_devices[12]);		// CPLD_3
		g_client[8] = i2c_new_client_device(adapter, &ix8c_i2c_devices[2]);		// MB_BOARDINFO_EEPROM
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x04);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[9] = i2c_new_client_device(adapter, &ix8c_i2c_devices[13]);		// MB Board Data
		g_client[10] = i2c_new_client_device(adapter, &ix8c_i2c_devices[14]);		// QSFP:49~52
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x05);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[11] = i2c_new_client_device(adapter, &ix8c_i2c_devices[3]);		// pca9548_1 SFP
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x06);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[12] = i2c_new_client_device(adapter, &ix8c_i2c_devices[4]);		// pca9548_2 SFP
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x07);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[13] = i2c_new_client_device(adapter, &ix8c_i2c_devices[5]);		// pca9548_3 SFP
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x08);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[14] = i2c_new_client_device(adapter, &ix8c_i2c_devices[6]);		// pca9548_4 SFP
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x09);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[15] = i2c_new_client_device(adapter, &ix8c_i2c_devices[7]);		// pca9548_5 SFP
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x0a);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[16] = i2c_new_client_device(adapter, &ix8c_i2c_devices[8]);		// pca9548_6 SFP
		i2c_put_adapter(adapter);
	}

	adapter = i2c_get_adapter(0x0b);
	if (adapter == NULL)
	{
		printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
	}
	else
	{
		g_client[17] = i2c_new_client_device(adapter, &ix8c_i2c_devices[9]);		// pca9548_7 QSFP
		i2c_put_adapter(adapter);
	}

	for(i = 13; i < 69; i ++){
		adapter = i2c_get_adapter(i);
		if (adapter == NULL)
		{
			printk("[%s] get i2c adapter fail at line %d", __FUNCTION__, __LINE__);
		}
		else
		{
			if (i < 61)  // SFP28 1~48 EEPROM
				g_client_port[i - 13] = i2c_new_client_device(adapter, &ix8c_i2c_devices[19]);
			else  // QSFP 49~56 EEPROM
				g_client_port[i - 13] = i2c_new_client_device(adapter, &ix8c_i2c_devices[15]);
			i2c_put_adapter(adapter);
		}
	}

	return 0;

fail_platform_device:
	platform_device_put(ix8c_device);

fail_platform_driver:
	platform_driver_unregister(&ix8c_platform_driver);
	return ret;
}

static void __exit ix8c_platform_exit(void)
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

	platform_device_unregister(ix8c_device);
	platform_driver_unregister(&ix8c_platform_driver);
}

module_init(ix8c_platform_init);
module_exit(ix8c_platform_exit);


MODULE_AUTHOR("Jonathan Tsai <jonathan.tsai@quantatw.com>");
MODULE_VERSION("1.0");
MODULE_DESCRIPTION("Quanta IX8C Platform Driver");
MODULE_LICENSE("GPL");
