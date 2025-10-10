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
#include "uart_payload_defs.h"
#include "uart_payload_api.h"


struct pddf_ops_t pddf_payload_ops = {
	.pre_init = NULL,
	.post_init = NULL,

	.pre_probe = NULL,
	.post_probe = NULL,

	.pre_remove = NULL,
	.post_remove = NULL,

	.pre_exit = NULL,
	.post_exit = NULL,
};

PAYLOAD_SYSFS_ATTR_DATA access_uart_get_payload = {PAYLOAD_GET, S_IRUGO, payload_show_default, NULL, bmc_get_payload, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_get_payload);

PAYLOAD_SYSFS_ATTR_DATA_ENTRY payload_sysfs_attr_data_tbl[]=
{
	{ "payload_12_0v", &access_uart_get_payload},
	{ "payload_3_30v", &access_uart_get_payload},
	{ "payload_5_00v", &access_uart_get_payload},
	{ "falcon_vcc_1_10v", &access_uart_get_payload},
	{ "falcon_vcc_0_80v", &access_uart_get_payload},
	{ "falcon_core_0_88v", &access_uart_get_payload},
	{ "falcon_vdda_0_90v", &access_uart_get_payload},
	{ "payload_1_80v", &access_uart_get_payload},
	{ "tl7_core_voltage_0_8v", &access_uart_get_payload},
	{ "tl7_avdd_0_9v", &access_uart_get_payload},
	{ "tl7_vddh_1_8v", &access_uart_get_payload},
	{ "tl7_avddh_1_1v", &access_uart_get_payload},
	{ "tl7_pllvdd_0_9v", &access_uart_get_payload},
	{ "payload_12v", &access_uart_get_payload},
	{ "standby_12v", &access_uart_get_payload},
	{ "mgmt_board_payload_5_0v", &access_uart_get_payload},
	{ "standby_3_3v", &access_uart_get_payload},
	{ "mgmt_board_payload_3_3v", &access_uart_get_payload},
	{ "standby_1_1v", &access_uart_get_payload},
	{ "main_board_payload_right_3_3v", &access_uart_get_payload},
	{ "main_board_payload_left_3_3v", &access_uart_get_payload},
	{ "main_board_payload_5_0v", &access_uart_get_payload},
	{ "main_board_tl10_core_0_8v", &access_uart_get_payload},
	{ "main_board_tl10_avdd_1_05v", &access_uart_get_payload},
	{ "main_board_tl10_1_8v", &access_uart_get_payload},
	{ "main_board_tl10_1_2v", &access_uart_get_payload},
	{ "main_board_tl10_0_75v", &access_uart_get_payload},
};

void *get_uart_payload_access_data(char *name)
{
	int i=0;
	for(i=0; i<(sizeof(payload_sysfs_attr_data_tbl)/sizeof(payload_sysfs_attr_data_tbl[0])); i++)
	{
		if(strncmp(name, payload_sysfs_attr_data_tbl[i].name, ATTR_NAME_LEN) ==0)
		{
			return &payload_sysfs_attr_data_tbl[i];
		}
	}
	return NULL;
}
EXPORT_SYMBOL(get_uart_payload_access_data);

static int payload_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    struct payload_data *data;
    int status = 0;
    int i,num, j = 0;
    PAYLOAD_PDATA *payload_platform_data;
    PAYLOAD_DATA_ATTR *data_attr;
    PAYLOAD_SYSFS_ATTR_DATA_ENTRY *sysfs_data_entry;

    if (client == NULL) { 
        pddf_dbg("UART_PAYLOAD", "NULL Client.. \n");
        goto exit;
    }

    if (pddf_payload_ops.pre_probe) 
    {
        status = (pddf_payload_ops.pre_probe)(client, dev_id);
        if (status != 0)
            goto exit;
    }

    data = kzalloc(sizeof(struct payload_data), GFP_KERNEL); 
    if (!data) {
        status = -ENOMEM; 
        goto exit;
    }

    i2c_set_clientdata(client, data); 

    dev_info(&client->dev, "chip found\n"); 
    
    /* Take control of the platform data */
    payload_platform_data = (PAYLOAD_PDATA *)(client->dev.platform_data);
    num = payload_platform_data->len; 
    data->index = payload_platform_data->idx; 
    data->num_attr = num;

    /* Add supported attr in the 'attributes' list */
    for (i=0; i<num; i++) 
    {
        struct sensor_device_attribute *dy_ptr = NULL;
        data_attr = payload_platform_data->payload_attrs + i; 
        sysfs_data_entry = get_uart_payload_access_data(data_attr->aname);
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
		
		data->payload_attribute_list[i] = &dy_ptr->dev_attr.attr;
		strlcpy(data->attr_info[i].name, data_attr->aname, sizeof(data->attr_info[i].name));
		data->attr_info[i].valid = 0;
		mutex_init(&data->attr_info[i].update_lock);

    }
    data->payload_attribute_list[i+j] = NULL;
	data->payload_attribute_group.attrs = data->payload_attribute_list;

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &data->payload_attribute_group); 
    if (status) {
        goto exit_free; 
    }

    data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, client->name, NULL, NULL); 
    if (IS_ERR(data->hwmon_dev)) { 
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove; 
    }

    dev_info(&client->dev, "%s: payload '%s'\n",
         dev_name(data->hwmon_dev), client->name);
    
    /* Add a support for post probe function */
    if (pddf_payload_ops.post_probe)
    {
        status = (pddf_payload_ops.post_probe)(client, dev_id);
        if (status != 0)
            goto exit_remove;
    }


    return 0;


exit_remove: 
    sysfs_remove_group(&client->dev.kobj, &data->payload_attribute_group);
exit_free:
    /* Free all the allocated attributes */
    for (i=0;data->payload_attribute_list[i]!=NULL;i++)
	{
		struct sensor_device_attribute *ptr = (struct sensor_device_attribute *)data->payload_attribute_list[i];
		kfree(ptr);
		data->payload_attribute_list[i] = NULL;
		pddf_dbg("UART_PAYLOAD", KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
	}
    kfree(data);
exit:
    
    return status;
}

static void payload_remove(struct i2c_client *client)
{
    int i = 0, ret = 0;
    struct payload_data *data = i2c_get_clientdata(client); 
    PAYLOAD_PDATA *platdata = (PAYLOAD_PDATA *)client->dev.platform_data; 
    PAYLOAD_DATA_ATTR *platdata_sub = platdata->payload_attrs;
    struct sensor_device_attribute *ptr = NULL;

    if (pddf_payload_ops.pre_remove)
    {
        ret = (pddf_payload_ops.pre_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "PAYLOAD pre_remove function failed\n");
    }

    hwmon_device_unregister(data->hwmon_dev);   
    sysfs_remove_group(&client->dev.kobj, &data->payload_attribute_group);
    for (i=0; data->payload_attribute_list[i]!=NULL; i++)
    {
        ptr = (struct sensor_device_attribute *)data->payload_attribute_list[i];
		kfree(ptr);
		data->payload_attribute_list[i] = NULL;
	}
    pddf_dbg("UART_PAYLOAD", KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
    kfree(data); 

    if (platdata_sub) { 
        pddf_dbg("UART_PAYLOAD", KERN_DEBUG "%s: Freeing platform subdata\n", __FUNCTION__);
        kfree(platdata_sub);
    }
    if (platdata) { 
        pddf_dbg("UART_PAYLOAD", KERN_DEBUG "%s: Freeing platform data\n", __FUNCTION__);
        kfree(platdata);
    }
    
    if (pddf_payload_ops.post_remove)
    {
        ret = (pddf_payload_ops.post_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "PAYLOAD post_remove function failed\n");
    }
}

enum payload_intf 
{
    PAYLOAD_CTRL_INTF,
};

static const struct i2c_device_id payload_ids[] = { 
    { "payload_uart", PAYLOAD_CTRL_INTF },
    {}
};

MODULE_DEVICE_TABLE(i2c, payload_ids); 

static struct i2c_driver payload_driver = {
    /*.class        = I2C_CLASS_HWMON,*/
    .driver = {
        .name     = "payload_uart",
        .owner    = THIS_MODULE,
    },
    .probe        = payload_probe,
    .remove       = payload_remove,
    .id_table     = payload_ids, 
};

int payload_uart_init(void)
{
    int ret = 0;

    if (pddf_payload_ops.pre_init) 
    {
        ret = (pddf_payload_ops.pre_init)();
        if (ret!=0)
            return ret;
    }

    pddf_dbg("UART_PAYLOAD", KERN_ERR "UART PAYLOAD DRIVER.. init Invoked..\n");
    ret = i2c_add_driver(&payload_driver); 
    if (ret!=0) 
        return ret;

    if (pddf_payload_ops.post_init) 
    {
        ret = (pddf_payload_ops.post_init)();
        if (ret!=0)
            return ret;
    }
    
    return ret;
}
EXPORT_SYMBOL(payload_uart_init);

void __exit payload_uart_exit(void)
{
    pddf_dbg("UART_PAYLOAD", "UART PAYLOAD DRIVER.. exit\n");
    if (pddf_payload_ops.pre_exit) (pddf_payload_ops.pre_exit)();
    i2c_del_driver(&payload_driver);
    if (pddf_payload_ops.post_exit) (pddf_payload_ops.post_exit)();

}
EXPORT_SYMBOL(payload_uart_exit);

MODULE_AUTHOR("lanxuhui");
MODULE_DESCRIPTION("Driver for payload information");
MODULE_LICENSE("GPL");

module_init(payload_uart_init); 
module_exit(payload_uart_exit);
