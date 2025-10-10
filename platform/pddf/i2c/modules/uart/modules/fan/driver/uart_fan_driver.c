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
#include <linux/types.h>
#include <linux/list.h>
#include <linux/string.h>
#include <linux/i2c-dev.h>
#include "pddf_client_defs.h"
#include "uart_fan_defs.h"
#include "uart_fan_api.h"


struct pddf_ops_t pddf_fan_ops = {
	.pre_init = NULL,
	.post_init = NULL,

	.pre_probe = NULL,
	.post_probe = NULL,

	.pre_remove = NULL,
	.post_remove = NULL,

	.pre_exit = NULL,
	.post_exit = NULL,
};

FAN_SYSFS_ATTR_DATA access_uart_fan_board_type = {FAN_BOARD_TYPE, S_IRUGO, fan_show_default, NULL, bmc_get_fan_board_type, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_fan_board_type);

FAN_SYSFS_ATTR_DATA access_uart_fan_status = {FAN_STATUS, S_IRUGO, fan_show_default, NULL, bmc_get_fan_status, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_fan_status);

FAN_SYSFS_ATTR_DATA access_uart_fan_presence = {FAN_PRESENCE, S_IRUGO, fan_show_default, NULL, bmc_get_fan_presence, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_fan_presence);

FAN_SYSFS_ATTR_DATA access_uart_fan_front_speed_rpm = {FAN_FRONT_SPEED_RPM, S_IRUGO, fan_show_default, NULL, bmc_get_fan_front_speed_rpm, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_fan_front_speed_rpm);

FAN_SYSFS_ATTR_DATA access_uart_fan_rear_speed_rpm = {FAN_REAR_SPEED_RPM, S_IRUGO, fan_show_default, NULL, bmc_get_fan_rear_speed_rpm, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_fan_rear_speed_rpm);

FAN_SYSFS_ATTR_DATA access_uart_fan_direction = {FAN_DIRECTION, S_IRUGO, fan_show_default, NULL, bmc_get_fan_direction, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_fan_direction);

FAN_SYSFS_ATTR_DATA access_uart_fan_eeprom_model = {FAN_EEPROM_MODEL, S_IRUGO, fan_show_default, NULL, bmc_get_fan_eeprom_model, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_fan_eeprom_model);

FAN_SYSFS_ATTR_DATA access_uart_fan_eeprom_serial_number = {FAN_EEPROM_SERIAL_NUMBER, S_IRUGO, fan_show_default, NULL, bmc_get_fan_eeprom_serial_number, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_fan_eeprom_serial_number);

FAN_SYSFS_ATTR_DATA access_uart_fan_eeprom_manufacturer = {FAN_EEPROM_MANUFACTURER, S_IRUGO, fan_show_default, NULL, bmc_get_fan_eeprom_manufacturer, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_fan_eeprom_manufacturer);

FAN_SYSFS_ATTR_DATA access_uart_fan_eeprom_module_model = {FAN_EEPROM_MODULE_MODEL, S_IRUGO, fan_show_default, NULL, bmc_get_fan_eeprom_module_model, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_fan_eeprom_module_model);

FAN_SYSFS_ATTR_DATA access_uart_fan_eeprom_module_brand = {FAN_EEPROM_MODULE_BRAND, S_IRUGO, fan_show_default, NULL, bmc_get_fan_eeprom_module_brand, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_fan_eeprom_module_brand);

FAN_SYSFS_ATTR_DATA access_uart_fan_eeprom_direction = {FAN_EEPROM_DIRECTION, S_IRUGO, fan_show_default, NULL, bmc_get_fan_eeprom_direction, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_fan_eeprom_direction);

FAN_SYSFS_ATTR_DATA_ENTRY fan_sysfs_attr_data_tbl[]=
{
	{ "fan_board_type", &access_uart_fan_board_type},
	{ "fan_status", &access_uart_fan_status},
	{ "fan_presence", &access_uart_fan_presence},
	{ "fan_front_speed_rpm", &access_uart_fan_front_speed_rpm},
	{ "fan_rear_speed_rpm", &access_uart_fan_rear_speed_rpm},
	{ "fan_direction", &access_uart_fan_direction},
	{ "fan_eeprom_model", &access_uart_fan_eeprom_model},
	{ "fan_eeprom_serial_number", &access_uart_fan_eeprom_serial_number},
	{ "fan_eeprom_manufacturer", &access_uart_fan_eeprom_manufacturer},
	{ "fan_eeprom_module_model", &access_uart_fan_eeprom_module_model},
	{ "fan_eeprom_module_brand", &access_uart_fan_eeprom_module_brand},
    { "fan_eeprom_direction", &access_uart_fan_eeprom_direction},
};

void *get_uart_fan_access_data(char *name)
{
	int i=0;
	for(i=0; i<(sizeof(fan_sysfs_attr_data_tbl)/sizeof(fan_sysfs_attr_data_tbl[0])); i++)
	{
		if(strncmp(name, fan_sysfs_attr_data_tbl[i].name, ATTR_NAME_LEN) ==0)
		{
			return &fan_sysfs_attr_data_tbl[i];
		}
	}
	return NULL;
}
EXPORT_SYMBOL(get_uart_fan_access_data);

static int fan_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    struct fan_data *data;
    int status = 0;
    int i,num, j = 0;
    FAN_PDATA *fan_platform_data;
    FAN_DATA_ATTR *data_attr;
    FAN_SYSFS_ATTR_DATA_ENTRY *sysfs_data_entry;

    if (client == NULL) { 
        pddf_dbg("UART_FAN", "NULL Client.. \n");
        goto exit;
    }

    if (pddf_fan_ops.pre_probe)
    {
        status = (pddf_fan_ops.pre_probe)(client, dev_id);
        if (status != 0)
            goto exit;
    }

    data = kzalloc(sizeof(struct fan_data), GFP_KERNEL); 
    if (!data) {
        status = -ENOMEM; 
        goto exit;
    }

    i2c_set_clientdata(client, data);  

    dev_info(&client->dev, "chip found\n"); 
    
    /* Take control of the platform data */
    fan_platform_data = (FAN_PDATA *)(client->dev.platform_data);
    num = fan_platform_data->len; 
    data->index = fan_platform_data->idx;
    data->num_fantrays = fan_platform_data->num_fantrays; 
    data->num_attr = num; 

    /* Add supported attr in the 'attributes' list */
    for (i=0; i<num; i++) 
    {
        struct sensor_device_attribute *dy_ptr = NULL;
        data_attr = fan_platform_data->fan_attrs + i; 
        sysfs_data_entry = get_uart_fan_access_data(data_attr->aname);
        if (sysfs_data_entry == NULL)
		{
			printk(KERN_ERR "%s: Wrong attribute name provided by user '%s'\n", __FUNCTION__, data_attr->aname);
			continue;
		}

        dy_ptr = (struct sensor_device_attribute *)kzalloc(sizeof(struct sensor_device_attribute)+ATTR_NAME_LEN, GFP_KERNEL);
		dy_ptr->dev_attr.attr.name = (char *)&dy_ptr[1];
		strlcpy((char *)dy_ptr->dev_attr.attr.name, data_attr->aname, ATTR_NAME_LEN);
		dy_ptr->dev_attr.attr.mode = sysfs_data_entry->a_ptr->mode;
		dy_ptr->dev_attr.show = sysfs_data_entry->a_ptr->show;
		dy_ptr->dev_attr.store = sysfs_data_entry->a_ptr->store;
		dy_ptr->index = sysfs_data_entry->a_ptr->index;
		
		data->fan_attribute_list[i] = &dy_ptr->dev_attr.attr;
		strlcpy(data->attr_info[i].name, data_attr->aname, sizeof(data->attr_info[i].name));
		data->attr_info[i].valid = 0;
		mutex_init(&data->attr_info[i].update_lock);

    }
    data->fan_attribute_list[i+j] = NULL;
	data->fan_attribute_group.attrs = data->fan_attribute_list;

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &data->fan_attribute_group);
    if (status) {
        goto exit_free; 
    }

    data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, client->name, NULL, NULL); 
    if (IS_ERR(data->hwmon_dev)) { 
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove; 
    }

    dev_info(&client->dev, "%s: fan '%s'\n",
         dev_name(data->hwmon_dev), client->name);
    
    /* Add a support for post probe function */
    if (pddf_fan_ops.post_probe)
    {
        status = (pddf_fan_ops.post_probe)(client, dev_id);
        if (status != 0)
            goto exit_remove;
    }


    return 0;


exit_remove:
    sysfs_remove_group(&client->dev.kobj, &data->fan_attribute_group);
exit_free: 
    /* Free all the allocated attributes */
    for (i=0;data->fan_attribute_list[i]!=NULL;i++)
	{
		struct sensor_device_attribute *ptr = (struct sensor_device_attribute *)data->fan_attribute_list[i];
		kfree(ptr);
		data->fan_attribute_list[i] = NULL;
		pddf_dbg("UART_FAN", KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
	}
    kfree(data);
exit:
    
    return status;
}

static void fan_remove(struct i2c_client *client)
{
    int i = 0, ret = 0;
    struct fan_data *data = i2c_get_clientdata(client); 
    FAN_PDATA *platdata = (FAN_PDATA *)client->dev.platform_data; 
    FAN_DATA_ATTR *platdata_sub = platdata->fan_attrs; 
    struct sensor_device_attribute *ptr = NULL;

    if (pddf_fan_ops.pre_remove)
    {
        ret = (pddf_fan_ops.pre_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "FAN pre_remove function failed\n");
    }

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &data->fan_attribute_group);
    for (i=0; data->fan_attribute_list[i]!=NULL; i++)
    {
        ptr = (struct sensor_device_attribute *)data->fan_attribute_list[i];
		kfree(ptr);
		data->fan_attribute_list[i] = NULL;
	}
    pddf_dbg("UART_FAN", KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
    kfree(data);

    if (platdata_sub) { 
        pddf_dbg("UART_FAN", KERN_DEBUG "%s: Freeing platform subdata\n", __FUNCTION__);
        kfree(platdata_sub);
    }
    if (platdata) {
        pddf_dbg("UART_FAN", KERN_DEBUG "%s: Freeing platform data\n", __FUNCTION__);
        kfree(platdata);
    }
    
    if (pddf_fan_ops.post_remove)
    {
        ret = (pddf_fan_ops.post_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "FAN post_remove function failed\n");
    }
}

enum fan_intf
{
    FAN_CTRL_INTF,
};

static const struct i2c_device_id fan_ids[] = { 
    { "fan_uart", FAN_CTRL_INTF },
    {}
};

MODULE_DEVICE_TABLE(i2c, fan_ids); 

static struct i2c_driver fan_driver = {
    /*.class        = I2C_CLASS_HWMON,*/
    .driver = {
        .name     = "fan_uart",
        .owner    = THIS_MODULE,
    },
    .probe        = fan_probe,
    .remove       = fan_remove,
    .id_table     = fan_ids,
};


int fan_uart_init(void)
{
    int ret = 0;

    if (pddf_fan_ops.pre_init) 
    {
        ret = (pddf_fan_ops.pre_init)();
        if (ret!=0)
            return ret;
    }

    pddf_dbg("UART_FAN", KERN_ERR "UART FAN DRIVER.. init Invoked..\n");
    ret = i2c_add_driver(&fan_driver); 
    if (ret!=0) 
        return ret;

    if (pddf_fan_ops.post_init)
    {
        ret = (pddf_fan_ops.post_init)();
        if (ret!=0)
            return ret;
    }
    
    return ret;
}
EXPORT_SYMBOL(fan_uart_init);

void __exit fan_uart_exit(void)
{
    pddf_dbg("UART_FAN", "UART FAN DRIVER.. exit\n");
    if (pddf_fan_ops.pre_exit) (pddf_fan_ops.pre_exit)();
    i2c_del_driver(&fan_driver);
    if (pddf_fan_ops.post_exit) (pddf_fan_ops.post_exit)();

}
EXPORT_SYMBOL(fan_uart_exit);

MODULE_AUTHOR("lanxuhui");
MODULE_DESCRIPTION("Driver for fan information");
MODULE_LICENSE("GPL");

module_init(fan_uart_init); 
module_exit(fan_uart_exit);
