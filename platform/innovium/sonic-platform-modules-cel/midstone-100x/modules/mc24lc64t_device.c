/*
 * mct24lc64_device.c - A driver to read and write the EEPROM 
 *
 * Copyright (C) 2020 Celestica Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Freeoftware Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 *
 **/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/types.h>

#define BUS_MIN 0       // according to the hw spec 
#define BUS_MAX 0		 // according to the hw spec
#define BUS_NUM (BUS_MAX - BUS_MIN + 1)

 /* Optical Module device info */
 static struct i2c_board_info i2c_info[] = {
	 {
		 I2C_BOARD_INFO("24lc64t", 0x56),
	 }
 };

struct i2c_adapter *adapter[BUS_NUM];
struct i2c_client *client[BUS_NUM];

static int __init mc24lc64t_init(void)
{
	int errno = 0;
	int i = 0, j = 0;
	int busnum = 0;
	
	for( j = 0, i = BUS_MIN; i <= BUS_MAX; i++, j++ ){
		adapter[j] = i2c_get_adapter(i); 
		if (IS_ERR(adapter[j])) {
			errno = PTR_ERR(adapter[j]);
			goto err_put_adapter_and_client;
		}
		
		client[j] = i2c_new_client_device(adapter[j], i2c_info); 
		if (IS_ERR(client[j])) {
			errno = PTR_ERR(client[j]);
			goto err_put_adapter_and_client;
		}
	}
	return 0;
err_put_adapter_and_client:
	for( j = 0, i = BUS_MIN; i <= BUS_MAX; i++, j++ ){
		if (client[j])
			i2c_unregister_device(client[j]);
		if (adapter[j])
			i2c_put_adapter(adapter[j]); 
	}
	return errno;
}
module_init(mc24lc64t_init);

static void __exit mc24lc64t_exit(void)
{
	int i = 0, j = 0;

	for( j = 0, i = BUS_MIN; i <= BUS_MAX; i++, j++ ){
		if (client[j])
			i2c_unregister_device(client[j]);
		if (adapter[j])
			i2c_put_adapter(adapter[j]); 
	}
	printk("mc24lc64t_exit\n");
}
module_exit(mc24lc64t_exit);

MODULE_DESCRIPTION("Device for mc24lc64t EEPROM");
MODULE_AUTHOR("Nicholas <nic@celestica.com>");
MODULE_LICENSE("GPL");
