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
#include "uart_thermal_defs.h"
#include "uart_thermal_api.h"


struct pddf_ops_t pddf_thermal_ops = {
	.pre_init = NULL,
	.post_init = NULL,

	.pre_probe = NULL,
	.post_probe = NULL,

	.pre_remove = NULL,
	.post_remove = NULL,

	.pre_exit = NULL,
	.post_exit = NULL,
};

THERMAL_SYSFS_ATTR_DATA access_uart_thermal_temp_get = {THERMAL_TEMP_GET, S_IRUGO, thermal_show_default, NULL, bmc_get_temp, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_thermal_temp_get);

THERMAL_SYSFS_ATTR_DATA access_uart_thermal_temp_set = {THERMAL_TEMP_SET, S_IRUGO | S_IWUSR, NULL, NULL, NULL, NULL, thermal_store_default, NULL, bmc_set_temp, NULL};
EXPORT_SYMBOL(access_uart_thermal_temp_set);

THERMAL_SYSFS_ATTR_DATA_ENTRY thermal_sysfs_attr_data_tbl[]=
{
	{ "lm_sensor_number", &access_uart_thermal_temp_get},
	{ "mainboard_left", &access_uart_thermal_temp_get},
	{ "mainboard_right", &access_uart_thermal_temp_get},
	{ "fan_board_left", &access_uart_thermal_temp_get},
	{ "fan_board_right", &access_uart_thermal_temp_get},
	{ "falcon_lm75", &access_uart_thermal_temp_get},
	{ "invm_ambient", &access_uart_thermal_temp_get},
	{ "invm_junction", &access_uart_thermal_temp_get},
	{ "tl10_lm75", &access_uart_thermal_temp_get},
	{ "switch_core_temp_set", &access_uart_thermal_temp_set},
	{ "sfp_temp_set", &access_uart_thermal_temp_set},
};

void *get_uart_thermal_access_data(char *name)
{
	int i=0;
	for(i=0; i<(sizeof(thermal_sysfs_attr_data_tbl)/sizeof(thermal_sysfs_attr_data_tbl[0])); i++)
	{
		if(strncmp(name, thermal_sysfs_attr_data_tbl[i].name, ATTR_NAME_LEN) ==0)
		{
			return &thermal_sysfs_attr_data_tbl[i];
		}
	}
	return NULL;
}
EXPORT_SYMBOL(get_uart_thermal_access_data);

static int thermal_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    struct thermal_data *data;
    int status = 0;
    int i,num, j = 0;
    THERMAL_PDATA *thermal_platform_data;
    THERMAL_DATA_ATTR *data_attr;
    THERMAL_SYSFS_ATTR_DATA_ENTRY *sysfs_data_entry;

    if (client == NULL) { 
        pddf_dbg("UART_THERMAL", "NULL Client.. \n");
        goto exit;
    }

    if (pddf_thermal_ops.pre_probe) 
    {
        status = (pddf_thermal_ops.pre_probe)(client, dev_id);
        if (status != 0)
            goto exit;
    }

    data = kzalloc(sizeof(struct thermal_data), GFP_KERNEL); 
    if (!data) {
        status = -ENOMEM; 
        goto exit;
    }

    i2c_set_clientdata(client, data); 

    dev_info(&client->dev, "chip found\n"); 
    
    /* Take control of the platform data */
    thermal_platform_data = (THERMAL_PDATA *)(client->dev.platform_data);
    num = thermal_platform_data->len; 
    data->index = thermal_platform_data->idx; 
    data->num_attr = num;

    /* Add supported attr in the 'attributes' list */
    for (i=0; i<num; i++) 
    {
        struct sensor_device_attribute *dy_ptr = NULL;
        data_attr = thermal_platform_data->thermal_attrs + i; 
        sysfs_data_entry = get_uart_thermal_access_data(data_attr->aname);
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
		
		data->thermal_attribute_list[i] = &dy_ptr->dev_attr.attr;
		strlcpy(data->attr_info[i].name, data_attr->aname, sizeof(data->attr_info[i].name));
		data->attr_info[i].valid = 0;
		mutex_init(&data->attr_info[i].update_lock);

    }
    data->thermal_attribute_list[i+j] = NULL;
	data->thermal_attribute_group.attrs = data->thermal_attribute_list;

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &data->thermal_attribute_group); 
    if (status) {
        goto exit_free; 
    }

    data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, client->name, NULL, NULL); 
    if (IS_ERR(data->hwmon_dev)) { 
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove; 
    }

    dev_info(&client->dev, "%s: thermal '%s'\n",
         dev_name(data->hwmon_dev), client->name);
    
    /* Add a support for post probe function */
    if (pddf_thermal_ops.post_probe)
    {
        status = (pddf_thermal_ops.post_probe)(client, dev_id);
        if (status != 0)
            goto exit_remove;
    }


    return 0;


exit_remove: 
    sysfs_remove_group(&client->dev.kobj, &data->thermal_attribute_group);
exit_free:
    /* Free all the allocated attributes */
    for (i=0;data->thermal_attribute_list[i]!=NULL;i++)
	{
		struct sensor_device_attribute *ptr = (struct sensor_device_attribute *)data->thermal_attribute_list[i];
		kfree(ptr);
		data->thermal_attribute_list[i] = NULL;
		pddf_dbg("UART_THERMAL", KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
	}
    kfree(data);
exit:
    
    return status;
}

static void thermal_remove(struct i2c_client *client)
{
    int i = 0, ret = 0;
    struct thermal_data *data = i2c_get_clientdata(client); 
    THERMAL_PDATA *platdata = (THERMAL_PDATA *)client->dev.platform_data; 
    THERMAL_DATA_ATTR *platdata_sub = platdata->thermal_attrs;
    struct sensor_device_attribute *ptr = NULL;

    if (pddf_thermal_ops.pre_remove)
    {
        ret = (pddf_thermal_ops.pre_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "THERMAL pre_remove function failed\n");
    }

    hwmon_device_unregister(data->hwmon_dev);   
    sysfs_remove_group(&client->dev.kobj, &data->thermal_attribute_group);
    for (i=0; data->thermal_attribute_list[i]!=NULL; i++)
    {
        ptr = (struct sensor_device_attribute *)data->thermal_attribute_list[i];
		kfree(ptr);
		data->thermal_attribute_list[i] = NULL;
	}
    pddf_dbg("UART_THERMAL", KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
    kfree(data); 

    if (platdata_sub) { 
        pddf_dbg("UART_THERMAL", KERN_DEBUG "%s: Freeing platform subdata\n", __FUNCTION__);
        kfree(platdata_sub);
    }
    if (platdata) { 
        pddf_dbg("UART_THERMAL", KERN_DEBUG "%s: Freeing platform data\n", __FUNCTION__);
        kfree(platdata);
    }
    
    if (pddf_thermal_ops.post_remove)
    {
        ret = (pddf_thermal_ops.post_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "THERMAL post_remove function failed\n");
    }
}

enum thermal_intf 
{
    THERMAL_CTRL_INTF,
};

static const struct i2c_device_id thermal_ids[] = { 
    { "thermal_uart", THERMAL_CTRL_INTF },
    {}
};

MODULE_DEVICE_TABLE(i2c, thermal_ids); 

static struct i2c_driver thermal_driver = {
    /*.class        = I2C_CLASS_HWMON,*/
    .driver = {
        .name     = "thermal_uart",
        .owner    = THIS_MODULE,
    },
    .probe        = thermal_probe,
    .remove       = thermal_remove,
    .id_table     = thermal_ids, 
};


int thermal_uart_init(void)
{
    int ret = 0;

    if (pddf_thermal_ops.pre_init) 
    {
        ret = (pddf_thermal_ops.pre_init)();
        if (ret!=0)
            return ret;
    }

    pddf_dbg("UART_THERMAL", KERN_ERR "UART THERMAL DRIVER.. init Invoked..\n");
    ret = i2c_add_driver(&thermal_driver); 
    if (ret!=0) 
        return ret;

    if (pddf_thermal_ops.post_init) 
    {
        ret = (pddf_thermal_ops.post_init)();
        if (ret!=0)
            return ret;
    }
    
    return ret;
}
EXPORT_SYMBOL(thermal_uart_init);

void __exit thermal_uart_exit(void)
{
    pddf_dbg("UART_THERMAL", "UART THERMAL DRIVER.. exit\n");
    if (pddf_thermal_ops.pre_exit) (pddf_thermal_ops.pre_exit)();
    i2c_del_driver(&thermal_driver);
    if (pddf_thermal_ops.post_exit) (pddf_thermal_ops.post_exit)();

}
EXPORT_SYMBOL(thermal_uart_exit);

MODULE_AUTHOR("lanxuhui");
MODULE_DESCRIPTION("Driver for thermal information");
MODULE_LICENSE("GPL");

module_init(thermal_uart_init); 
module_exit(thermal_uart_exit);
