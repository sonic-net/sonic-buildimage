/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/i2c.h>
#include <linux/platform_data/i2c-gpio.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/platform_device.h>

#include <linux/platform_data/pca954x.h>

struct inv_i2c_board_info {
    int ch;
    int size;
    struct i2c_board_info *board_info;
};

/////////////////////////////////////////////////////////////////////////////////////////
static struct pca954x_platform_mode mux_modes_0[] = {
    {.adap_id = 2,},    {.adap_id = 3,},
    {.adap_id = 4,},    {.adap_id = 5,},
    {.adap_id = 6,},    {.adap_id = 7,},
    {.adap_id = 8,},    {.adap_id = 9,},
};
static struct pca954x_platform_mode mux_modes_0_0[] = {
    {.adap_id = 10,},    {.adap_id = 11,},
    {.adap_id = 12,},    {.adap_id = 13,},
    {.adap_id = 14,},    {.adap_id = 15,},
    {.adap_id = 16,},    {.adap_id = 17,},
};

static struct pca954x_platform_mode mux_modes_0_1[] = {
    {.adap_id = 18,},    {.adap_id = 19,},
    {.adap_id = 20,},    {.adap_id = 21,},
    {.adap_id = 22,},    {.adap_id = 23,},
    {.adap_id = 24,},    {.adap_id = 25,},
};

static struct pca954x_platform_mode mux_modes_0_2[] = {
    {.adap_id = 26,},    {.adap_id = 27,},
    {.adap_id = 28,},    {.adap_id = 29,},
    {.adap_id = 30,},    {.adap_id = 31,},
    {.adap_id = 32,},    {.adap_id = 33,},
};

static struct pca954x_platform_mode mux_modes_0_3[] = {
    {.adap_id = 34,},    {.adap_id = 35,},
    {.adap_id = 36,},    {.adap_id = 37,},
    {.adap_id = 38,},    {.adap_id = 39,},
    {.adap_id = 40,},    {.adap_id = 41,},
};

static struct pca954x_platform_mode mux_modes_0_4[] = {
    {.adap_id = 42,},    {.adap_id = 43,},
    {.adap_id = 44,},    {.adap_id = 45,},
    {.adap_id = 46,},    {.adap_id = 47,},
    {.adap_id = 48,},    {.adap_id = 49,},
};

static struct pca954x_platform_mode mux_modes_0_5[] = {
    {.adap_id = 50,},    {.adap_id = 51,},
    {.adap_id = 52,},    {.adap_id = 53,},
    {.adap_id = 54,},    {.adap_id = 55,},
    {.adap_id = 56,},    {.adap_id = 57,},
};

static struct pca954x_platform_mode mux_modes_0_6[] = {
    {.adap_id = 58,},    {.adap_id = 59,},
    {.adap_id = 60,},    {.adap_id = 61,},
    {.adap_id = 62,},    {.adap_id = 63,},
    {.adap_id = 64,},    {.adap_id = 65,},
};

//no i2c device driver attach to mux 7


static struct pca954x_platform_data mux_data_0 = {
        .modes          = mux_modes_0,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_0 = {
        .modes          = mux_modes_0_0,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_1 = {
        .modes          = mux_modes_0_1,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_2 = {
        .modes          = mux_modes_0_2,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_3 = {
        .modes          = mux_modes_0_3,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_4 = {
        .modes          = mux_modes_0_4,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_5 = {
        .modes          = mux_modes_0_5,
        .num_modes      = 8,
};
static struct pca954x_platform_data mux_data_0_6 = {
        .modes          = mux_modes_0_6,
        .num_modes      = 8,
};


static struct i2c_board_info xlp_i2c_device_info0[] __initdata = {
        {"inv_psoc",         0, 0x66, 0, 0, 0},//psoc
        {"inv_cpld",         0, 0x55, 0, 0, 0},//cpld
        {"pca9548",          0, 0x71, &mux_data_0, 0, 0},    
};

static struct i2c_board_info xlp_i2c_device_info1[] __initdata = {
        {"inv_psoc",         0, 0x66, 0, 0, 0},//psoc
        {"inv_cpld",         0, 0x55, 0, 0, 0},//cpld
};

static struct i2c_board_info xlp_i2c_device_info2[] __initdata = {
        {"pca9548",         0, 0x72, &mux_data_0_0, 0, 0},    
};

static struct i2c_board_info xlp_i2c_device_info3[] __initdata = {
        {"pca9548",         0, 0x72, &mux_data_0_1, 0, 0},    
};

static struct i2c_board_info xlp_i2c_device_info4[] __initdata = {
        {"pca9548",         0, 0x72, &mux_data_0_2, 0, 0},    
};

static struct i2c_board_info xlp_i2c_device_info5[] __initdata = {
        {"pca9548",         0, 0x72, &mux_data_0_3, 0, 0},    
};
static struct i2c_board_info xlp_i2c_device_info6[] __initdata = {
        {"pca9548",         0, 0x72, &mux_data_0_4, 0, 0},    
};
static struct i2c_board_info xlp_i2c_device_info7[] __initdata = {
        {"pca9548",         0, 0x72, &mux_data_0_5, 0, 0},    
};
static struct i2c_board_info xlp_i2c_device_info8[] __initdata = {
        {"pca9548",         0, 0x72, &mux_data_0_6, 0, 0},    
};


static struct inv_i2c_board_info i2cdev_list[] = {
    {0, ARRAY_SIZE(xlp_i2c_device_info0),  xlp_i2c_device_info0 },  //smbus 0
    
    {2, ARRAY_SIZE(xlp_i2c_device_info2),  xlp_i2c_device_info2 },  //mux 0
    {3, ARRAY_SIZE(xlp_i2c_device_info3),  xlp_i2c_device_info3 },  //mux 1
    {4, ARRAY_SIZE(xlp_i2c_device_info4),  xlp_i2c_device_info4 },  //mux 2
    {5, ARRAY_SIZE(xlp_i2c_device_info5),  xlp_i2c_device_info5 },  //mux 3
    {6, ARRAY_SIZE(xlp_i2c_device_info6),  xlp_i2c_device_info6 },  //mux 4
    {7, ARRAY_SIZE(xlp_i2c_device_info7),  xlp_i2c_device_info7 },  //mux 5
    {8, ARRAY_SIZE(xlp_i2c_device_info8),  xlp_i2c_device_info8 },  //mux 6
};

/////////////////////////////////////////////////////////////////////////////////////////
static struct i2c_gpio_platform_data i2c_gpio_platdata = {
    .scl_pin = 8,
    .sda_pin = 9,
    
    .udelay  = 5, //5:100kHz
    .sda_is_open_drain = 0,
    .scl_is_open_drain = 0,
    .scl_is_output_only = 0
};

static struct platform_device magnolia_device_i2c_gpio = {
    .name    = "i2c-gpio",
    .id      = 0, // adapter number
    .dev.platform_data = &i2c_gpio_platdata,
};

#define PLAT_MAX_I2C_CLIENTS 32
static struct i2c_client *plat_i2c_client[PLAT_MAX_I2C_CLIENTS];
static int num_i2c_clients = 0;
static int plat_i2c_client_add(struct i2c_client *e)
{
    if (num_i2c_clients >= PLAT_MAX_I2C_CLIENTS)
      return -1;

    plat_i2c_client[num_i2c_clients] = e;
    num_i2c_clients++;
    return num_i2c_clients;
}

static void plat_i2c_client_remove_all(void)
{
    int i;
    for (i = num_i2c_clients-1; i >= 0; i--)
       i2c_unregister_device(plat_i2c_client[i]);
}

static int __init plat_magnolia_init(void)
{
    struct i2c_adapter *adap = NULL;
    struct i2c_client *e = NULL;
    int ret = 0;
    int i,j;

    for(i=0; i<ARRAY_SIZE(i2cdev_list); i++) {
        
        adap = i2c_get_adapter( i2cdev_list[i].ch );
        if (adap == NULL) {
            printk("magnolia get channel %d adapter fail\n", i);
            continue;
            return -ENODEV;
        }
    
        i2c_put_adapter(adap);
        for(j=0; j<i2cdev_list[i].size; j++) {
            e = i2c_new_device(adap, &i2cdev_list[i].board_info[j] );

            if (plat_i2c_client_add(e)<0) {
               printk("too many i2c clients added (PLAT_MAX_I2C_CLIENTS=%d)\n", PLAT_MAX_I2C_CLIENTS);
               plat_i2c_client_remove_all();
               return -ENODEV;
            }
        }
    }

    return ret;    
}

static void __exit plat_magnolia_exit(void)
{
    plat_i2c_client_remove_all();
}

module_init(plat_magnolia_init);
module_exit(plat_magnolia_exit);

MODULE_AUTHOR("Inventec");
MODULE_DESCRIPTION("Magnolia Platform devices");
MODULE_LICENSE("GPL");
