/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Description:
 *   A pddf kernel driver module for FPGA connected to the CPU by I2C bus
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/dmi.h>
#include "pddf_fpgai2c_defs.h"

extern PDDF_FPGAI2C_DATA pddf_fpgai2c_data;


static LIST_HEAD(fpgai2c_client_list);
static struct mutex	list_lock;

struct fpgai2c_client_node {
	struct i2c_client *client;
	struct list_head   list;
};

int board_i2c_fpga_read(unsigned short fpga_addr, u8 reg)
{
	struct list_head   *list_node = NULL;
	struct fpgai2c_client_node *fpga_node = NULL;
	int ret = -EPERM;


	mutex_lock(&list_lock);

	list_for_each(list_node, &fpgai2c_client_list)
	{
		fpga_node = list_entry(list_node, struct fpgai2c_client_node, list);

		if (fpga_node->client->addr == fpga_addr) {
			ret = i2c_smbus_read_byte_data(fpga_node->client, reg);
			break;
		}
	}

	mutex_unlock(&list_lock);

	return ret;
}
EXPORT_SYMBOL(board_i2c_fpga_read);

int board_i2c_fpga_write(unsigned short fpga_addr, u8 reg, u8 value)
{
	struct list_head   *list_node = NULL;
	struct fpgai2c_client_node *fpga_node = NULL;
	int ret = -EIO;


	mutex_lock(&list_lock);

	list_for_each(list_node, &fpgai2c_client_list)
	{
		fpga_node = list_entry(list_node, struct fpgai2c_client_node, list);

		if (fpga_node->client->addr == fpga_addr) {
			ret = i2c_smbus_write_byte_data(fpga_node->client, reg, value);
			break;
		}
	}

	mutex_unlock(&list_lock);

	return ret;
}
EXPORT_SYMBOL(board_i2c_fpga_write);

ssize_t regval_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int len = 0;
    struct i2c_client *client = to_i2c_client(dev);

    mutex_lock(&pddf_fpgai2c_data.fpga_lock);
    // Put code here to read the register value and print it
    if (pddf_fpgai2c_data.reg_addr!=0)
        len = sprintf(buf, "0x%2.2x\n", board_i2c_fpga_read(client->addr, pddf_fpgai2c_data.reg_addr));
    else
        len = sprintf(buf, "xx\n");

    mutex_unlock(&pddf_fpgai2c_data.fpga_lock);
    return len;
}

static DEVICE_ATTR_RO(regval);

static struct attribute *fpgai2c_attrs[] = {
    &dev_attr_regval.attr,
    NULL,
};

static struct attribute_group fpgai2c_attribute_group = {
    .attrs = fpgai2c_attrs,
};




/* Addresses scanned for board_i2c_fpga
 */
static const unsigned short normal_i2c[] = { I2C_CLIENT_END };

static void board_i2c_fpga_add_client(struct i2c_client *client)
{
	struct fpgai2c_client_node *node = kzalloc(sizeof(struct fpgai2c_client_node), GFP_KERNEL);

	if (!node) {
		dev_dbg(&client->dev, "Can't allocate fpgai2c_client_node (0x%x)\n", client->addr);
		return;
	}

	node->client = client;

	mutex_lock(&list_lock);
	list_add(&node->list, &fpgai2c_client_list);
	mutex_unlock(&list_lock);
}

static void board_i2c_fpga_remove_client(struct i2c_client *client)
{
	struct list_head		*list_node = NULL;
	struct fpgai2c_client_node *fpga_node = NULL;
	int found = 0;

	mutex_lock(&list_lock);

	list_for_each(list_node, &fpgai2c_client_list)
	{
		fpga_node = list_entry(list_node, struct fpgai2c_client_node, list);

		if (fpga_node->client == client) {
			found = 1;
			break;
		}
	}

	if (found) {
		list_del(list_node);
		kfree(fpga_node);
	}

	mutex_unlock(&list_lock);
}

static int board_i2c_fpga_probe(struct i2c_client *client)
{
	int status;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_dbg(&client->dev, "i2c_check_functionality failed (0x%x)\n", client->addr);
		status = -EIO;
		goto exit;
	}

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &fpgai2c_attribute_group);
    if (status) {
        goto exit;
    }

	dev_dbg(&client->dev, "chip found\n");
	board_i2c_fpga_add_client(client);

	return 0;

exit:
	return status;
}

static void board_i2c_fpga_remove(struct i2c_client *client)
{
    sysfs_remove_group(&client->dev.kobj, &fpgai2c_attribute_group);
    board_i2c_fpga_remove_client(client);
}

static const struct i2c_device_id board_i2c_fpga_id[] = {
	{ "i2c_fpga", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, board_i2c_fpga_id);

static struct i2c_driver board_i2c_fpga_driver = {
	.class		= I2C_CLASS_HWMON,
	.driver = {
		.name = "i2c_fpga",
	},
	.probe		= board_i2c_fpga_probe,
	.remove	   	= board_i2c_fpga_remove,
	.id_table	= board_i2c_fpga_id,
	.address_list = normal_i2c,
};

static int __init board_i2c_fpga_init(void)
{
	mutex_init(&list_lock);
	return i2c_add_driver(&board_i2c_fpga_driver);
}

static void __exit board_i2c_fpga_exit(void)
{
	i2c_del_driver(&board_i2c_fpga_driver);
}

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("board_i2c_fpga driver");
MODULE_LICENSE("GPL");

module_init(board_i2c_fpga_init);
module_exit(board_i2c_fpga_exit);
