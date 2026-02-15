/*
 * Copyright 2019 Broadcom.
 * The term “Broadcom” refers to Broadcom Inc. and/or its subsidiaries.
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
 *
 * A pddf kernel module driver for PSU
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/kobject.h>
#include "pddf_client_defs.h"
#include "pddf_psu_defs.h"
#include "pddf_psu_driver.h"
#include "pddf_psu_api.h"


static unsigned short normal_i2c[] = { I2C_CLIENT_END };

struct pddf_ops_t pddf_psu_ops = {
	.pre_init = NULL,
	.post_init = NULL,

	.pre_probe = NULL,
	.post_probe = NULL,

	.pre_remove = NULL,
	.post_remove = NULL,

	.pre_exit = NULL,
	.post_exit = NULL,
};
EXPORT_SYMBOL(pddf_psu_ops);


PSU_SYSFS_ATTR_DATA access_psu_present = {PSU_PRESENT, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_byte_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_present);

PSU_SYSFS_ATTR_DATA access_psu_model_name = {PSU_MODEL_NAME, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_block_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_model_name);

PSU_SYSFS_ATTR_DATA access_psu_power_good = {PSU_POWER_GOOD, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_byte_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_power_good);

PSU_SYSFS_ATTR_DATA access_psu_mfr_id = {PSU_MFR_ID, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_block_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_mfr_id);

PSU_SYSFS_ATTR_DATA access_psu_serial_num = {PSU_SERIAL_NUM, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_block_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_serial_num);

PSU_SYSFS_ATTR_DATA access_psu_fan_dir = {PSU_FAN_DIR, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_block_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_fan_dir);

PSU_SYSFS_ATTR_DATA access_psu_v_out = {PSU_V_OUT, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_v_out);

PSU_SYSFS_ATTR_DATA access_psu_v_out_min = {PSU_V_OUT_MIN, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_v_out_min);

PSU_SYSFS_ATTR_DATA access_psu_v_out_max = {PSU_V_OUT_MAX, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_v_out_max);

PSU_SYSFS_ATTR_DATA access_psu_i_out = {PSU_I_OUT, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_i_out);

PSU_SYSFS_ATTR_DATA access_psu_p_out = {PSU_P_OUT, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_p_out);

PSU_SYSFS_ATTR_DATA access_psu_p_out_max = {PSU_P_OUT_MAX, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_p_out_max);

PSU_SYSFS_ATTR_DATA access_psu_fan1_speed_rpm = {PSU_FAN1_SPEED, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_fan1_speed_rpm);

PSU_SYSFS_ATTR_DATA access_psu_temp1_input = {PSU_TEMP1_INPUT, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_temp1_input);

PSU_SYSFS_ATTR_DATA access_psu_temp1_high_threshold = {PSU_TEMP1_HIGH_THRESHOLD, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_temp1_high_threshold);

PSU_SYSFS_ATTR_DATA access_psu_temp2_input = {PSU_TEMP2_INPUT, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_temp2_input);

PSU_SYSFS_ATTR_DATA access_psu_temp2_high_threshold = {PSU_TEMP2_HIGH_THRESHOLD, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_temp2_high_threshold);

PSU_SYSFS_ATTR_DATA access_psu_temp3_input = {PSU_TEMP3_INPUT, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_temp3_input);

PSU_SYSFS_ATTR_DATA access_psu_temp3_high_threshold = {PSU_TEMP3_HIGH_THRESHOLD, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_temp3_high_threshold);

PSU_SYSFS_ATTR_DATA access_psu_v_in = {PSU_V_IN, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_v_in);

PSU_SYSFS_ATTR_DATA access_psu_i_in = {PSU_I_IN, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_i_in);

PSU_SYSFS_ATTR_DATA access_psu_p_in = {PSU_P_IN, S_IRUGO, psu_show_default, NULL, sonic_i2c_get_psu_word_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_psu_p_in);

PSU_SYSFS_ATTR_DATA_ENTRY psu_sysfs_attr_data_tbl[]=
{
	{ "psu_present", &access_psu_present},
	{ "psu_model_name", &access_psu_model_name},
	{ "psu_power_good" , &access_psu_power_good},
	{ "psu_mfr_id" , &access_psu_mfr_id},
	{ "psu_serial_num" , &access_psu_serial_num},
	{ "psu_fan_dir" , &access_psu_fan_dir},
	{ "psu_v_out" , &access_psu_v_out},
	{ "psu_v_out_min" , &access_psu_v_out_min},
	{ "psu_v_out_max" , &access_psu_v_out_max},
	{ "psu_i_out" , &access_psu_i_out},
	{ "psu_p_out" , &access_psu_p_out},
	{ "psu_p_out_max" , &access_psu_p_out_max},
	{ "psu_fan1_speed_rpm" , &access_psu_fan1_speed_rpm},
	{ "psu_temp1_input" , &access_psu_temp1_input},
	{ "psu_temp1_high_threshold" , &access_psu_temp1_high_threshold},
	{ "psu_temp2_input" , &access_psu_temp2_input},
	{ "psu_temp2_high_threshold" , &access_psu_temp2_high_threshold},
	{ "psu_temp3_input" , &access_psu_temp3_input},
	{ "psu_temp3_high_threshold" , &access_psu_temp3_high_threshold},
	{ "psu_v_in" , &access_psu_v_in},
	{ "psu_i_in" , &access_psu_i_in},
	{ "psu_p_in" , &access_psu_p_in}
};

enum psu_intf
{
	eeprom_intf,
	smbus_intf
};

static const struct i2c_device_id psu_id[] = {
	{"psu_eeprom", eeprom_intf},
	{"psu_pmbus", smbus_intf},
	{}
};

MODULE_DEVICE_TABLE(i2c, psu_id);

void *get_psu_access_data(char *name)
{
	int i=0;
	for(i=0; i<(sizeof(psu_sysfs_attr_data_tbl)/sizeof(psu_sysfs_attr_data_tbl[0])); i++)
	{
		if(strcmp(name, psu_sysfs_attr_data_tbl[i].name) ==0)
		{
			return &psu_sysfs_attr_data_tbl[i];
		}
	}
	return NULL;
}
EXPORT_SYMBOL(get_psu_access_data);

static bool skip_unsupported_psu_attribute(PSU_DATA_ATTR *data_attr,
					   struct psu_data *data)
{
	/* Only process temperature sensor attributes */
	if (strncmp(data_attr->aname, "psu_temp", 8) != 0 ||
	    data_attr->aname[8] == '\0') {
		return false;
	}

	int temp_index = data_attr->aname[8] - '0';

	/* Skip if this thermal sensor is not available */
	if (temp_index > data->num_psu_thermals) {
		pddf_dbg(PSU,
			 KERN_INFO
			 "%s: Skipping %s as num_psu_thermals is %d\n",
			 __func__, data_attr->aname, data->num_psu_thermals);
		return true;
	}

	/* Skip high threshold attributes if not supported by this thermal sensor */
	if (strstr(data_attr->aname, "high_threshold") != NULL) {
		/* Check if this thermal sensor supports high threshold using the bitmap */
		if (!(data->psu_temp_high_thresh_bitmap &
		      (1 << (temp_index - 1)))) {
			pddf_dbg(
				PSU,
				KERN_INFO
				"%s: Skipping %s as high threshold is not supported for thermal sensor %d\n",
				__func__, data_attr->aname, temp_index);
			return true;
		}
	}

	return false;
}

static int psu_probe(struct i2c_client *client)
{
    struct psu_data *data;
    int status =0;
    int i,num, j=0;
    PSU_PDATA *psu_platform_data;
    PSU_DATA_ATTR *data_attr;
    PSU_SYSFS_ATTR_DATA_ENTRY *sysfs_data_entry;
    char new_str[ATTR_NAME_LEN] = "";
    struct i2c_device_id *dev_id;


	if (client == NULL) {
		printk("NULL Client.. \n");
		goto exit;
	}

	if (pddf_psu_ops.pre_probe)
    {
        dev_id = i2c_match_id(psu_id, client);
        status = (pddf_psu_ops.pre_probe)(client, dev_id);
        if (status != 0)
            goto exit;
    }

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_I2C_BLOCK)) {
        status = -EIO;
        goto exit;
    }

    data = kzalloc(sizeof(struct psu_data), GFP_KERNEL);
    if (!data) {
        status = -ENOMEM;
        goto exit;
    }

    i2c_set_clientdata(client, data);
    dev_info(&client->dev, "chip found\n");

	/* Take control of the platform data */
	psu_platform_data = (PSU_PDATA *)(client->dev.platform_data);
	num = psu_platform_data->len;
	data->index = psu_platform_data->idx - 1;
	data->num_psu_fans = psu_platform_data->num_psu_fans;
	data->num_psu_thermals = psu_platform_data->num_psu_thermals;
	data->psu_temp_high_thresh_bitmap = psu_platform_data->psu_temp_high_thresh_bitmap;
	data->num_attr = num;



	/* Create and Add supported attr in the 'attributes' list */
	for (i=0; i<num; i++)
	{
		/*struct attribute *aptr = NULL;*/
		struct sensor_device_attribute *dy_ptr = NULL;
		data_attr = psu_platform_data->psu_attrs + i;
		sysfs_data_entry = get_psu_access_data(data_attr->aname);
		if (sysfs_data_entry == NULL)
		{
			printk(KERN_ERR "%s: Wrong attribute name provided by user '%s'\n", __FUNCTION__, data_attr->aname);
			continue;
		}
		
		/* Skip unsupported PSU attributes */
		if (skip_unsupported_psu_attribute(data_attr, data)) {
			continue;
		}

		dy_ptr = (struct sensor_device_attribute *)kzalloc(sizeof(struct sensor_device_attribute)+ATTR_NAME_LEN, GFP_KERNEL);
		dy_ptr->dev_attr.attr.name = (char *)&dy_ptr[1];
		strcpy((char *)dy_ptr->dev_attr.attr.name, data_attr->aname);
		dy_ptr->dev_attr.attr.mode = sysfs_data_entry->a_ptr->mode;
		dy_ptr->dev_attr.show = sysfs_data_entry->a_ptr->show;
		dy_ptr->dev_attr.store = sysfs_data_entry->a_ptr->store;
		dy_ptr->index = sysfs_data_entry->a_ptr->index;
		
		data->psu_attribute_list[i] = &dy_ptr->dev_attr.attr;
		strcpy(data->attr_info[i].name, data_attr->aname);
		data->attr_info[i].valid = 0;
		mutex_init(&data->attr_info[i].update_lock);

		/*Create a duplicate entry*/
		get_psu_duplicate_sysfs(dy_ptr->index, new_str);
		if (strcmp(new_str,""))
		{
			dy_ptr = (struct sensor_device_attribute *)kzalloc(sizeof(struct sensor_device_attribute)+ATTR_NAME_LEN, GFP_KERNEL);
			dy_ptr->dev_attr.attr.name = (char *)&dy_ptr[1];
			strcpy((char *)dy_ptr->dev_attr.attr.name, new_str);
			dy_ptr->dev_attr.attr.mode = sysfs_data_entry->a_ptr->mode;
			dy_ptr->dev_attr.show = sysfs_data_entry->a_ptr->show;
			dy_ptr->dev_attr.store = sysfs_data_entry->a_ptr->store;
			dy_ptr->index = sysfs_data_entry->a_ptr->index;

			data->psu_attribute_list[num+j] = &dy_ptr->dev_attr.attr;
			j++;
			strcpy(new_str,"");
		}
	}
	data->psu_attribute_list[i+j] = NULL;
	data->psu_attribute_group.attrs = data->psu_attribute_list;

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &data->psu_attribute_group);
    if (status) {
        goto exit_free;
    }

	data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, client->name, NULL, NULL);
	if (IS_ERR(data->hwmon_dev)) {
		status = PTR_ERR(data->hwmon_dev);
		goto exit_remove;
	}

    dev_info(&client->dev, "%s: psu '%s'\n",
         dev_name(data->hwmon_dev), client->name);

	/* Add a support for post probe function */
    if (pddf_psu_ops.post_probe)
    {
        dev_id = i2c_match_id(psu_id, client);
        status = (pddf_psu_ops.post_probe)(client, dev_id);
        if (status != 0)
            goto exit_remove;
    }

    return 0;


exit_remove:
    sysfs_remove_group(&client->dev.kobj, &data->psu_attribute_group);
exit_free:
	/* Free all the allocated attributes */
	for (i=0;data->psu_attribute_list[i]!=NULL;i++)
	{
		struct sensor_device_attribute *ptr = (struct sensor_device_attribute *)data->psu_attribute_list[i];
		kfree(ptr);
		data->psu_attribute_list[i] = NULL;
		pddf_dbg(PSU, KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
	}
    kfree(data);
exit:
    return status;
}

static void psu_remove(struct i2c_client *client)
{
	int i=0, ret = 0;
    struct psu_data *data = i2c_get_clientdata(client);
	PSU_PDATA *platdata = (PSU_PDATA *)client->dev.platform_data; // use dev_get_platdata()
	PSU_DATA_ATTR *platdata_sub = platdata->psu_attrs;
	struct sensor_device_attribute *ptr = NULL;

	if (pddf_psu_ops.pre_remove)
    {
        ret = (pddf_psu_ops.pre_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "FAN pre_remove function failed\n");
    }

	hwmon_device_unregister(data->hwmon_dev);
	sysfs_remove_group(&client->dev.kobj, &data->psu_attribute_group);
	for (i=0; data->psu_attribute_list[i]!=NULL; i++)
    {
        ptr = (struct sensor_device_attribute *)data->psu_attribute_list[i];
		kfree(ptr);
		data->psu_attribute_list[i] = NULL;
	}
    pddf_dbg(PSU, KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
    kfree(data);
	if (platdata_sub) {
		printk(KERN_DEBUG "%s: Freeing platform subdata\n", __FUNCTION__);
		kfree(platdata_sub);
	}
	if (platdata) {
		printk(KERN_DEBUG "%s: Freeing platform data\n", __FUNCTION__);
		kfree(platdata);
	}

	if (pddf_psu_ops.post_remove)
    {
        ret = (pddf_psu_ops.post_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "FAN post_remove function failed\n");
    }
}

static struct i2c_driver psu_driver = {
    .class        = I2C_CLASS_HWMON,
    .driver = {
        .name     = "psu",
    },
    .probe        = psu_probe,
    .remove       = psu_remove,
    .id_table     = psu_id,
    .address_list = normal_i2c,
};

int example_fun(void)
{
	pddf_dbg(PSU, KERN_ERR "CALLING FUN...\n");
	return 0;
}
EXPORT_SYMBOL(example_fun);


int psu_init(void)
{
    int status = 0;

    if (pddf_psu_ops.pre_init)
    {
        status = (pddf_psu_ops.pre_init)();
        if (status!=0)
            return status;
    }

	pddf_dbg(PSU, KERN_ERR "GENERIC_PSU_DRIVER.. init Invoked..\n");
    status = i2c_add_driver(&psu_driver);
    if (status!=0)
        return status;
	
    if (pddf_psu_ops.post_init)
    {
        status = (pddf_psu_ops.post_init)();
        if (status!=0)
            return status;
    }

	return status;
}
EXPORT_SYMBOL(psu_init);

void psu_exit(void)
{
	pddf_dbg(PSU, "GENERIC_PSU_DRIVER.. exit\n");
	if (pddf_psu_ops.pre_exit) (pddf_psu_ops.pre_exit)();
    i2c_del_driver(&psu_driver);
	if (pddf_psu_ops.post_exit) (pddf_psu_ops.post_exit)();
}
EXPORT_SYMBOL(psu_exit);

module_init(psu_init);
module_exit(psu_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("psu driver");
MODULE_LICENSE("GPL");
