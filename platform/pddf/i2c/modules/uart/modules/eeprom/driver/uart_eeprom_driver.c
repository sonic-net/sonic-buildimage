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
#include "uart_eeprom_defs.h"
#include "uart_eeprom_api.h"


struct pddf_ops_t pddf_eeprom_ops = {
	.pre_init = NULL,
	.post_init = NULL,

	.pre_probe = NULL,
	.post_probe = NULL,

	.pre_remove = NULL,
	.post_remove = NULL,

	.pre_exit = NULL,
	.post_exit = NULL,
};

EEPROM_SYSFS_ATTR_DATA access_uart_eeprom_default = {EEPROM_DEFAULT, S_IRUGO, eeprom_show_default, NULL, bmc_get_eeprom_default, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_eeprom_default);

EEPROM_SYSFS_ATTR_DATA_ENTRY eeprom_sysfs_attr_data_tbl[]=
{
	{ "product_name", &access_uart_eeprom_default},
	{ "part_number", &access_uart_eeprom_default},
	{ "serial_number", &access_uart_eeprom_default},
	{ "base_mac_address", &access_uart_eeprom_default},
	{ "manufacture_data", &access_uart_eeprom_default},
	{ "device_version", &access_uart_eeprom_default},
	{ "lable_revision", &access_uart_eeprom_default},
	{ "platform_name", &access_uart_eeprom_default},
	{ "onie_version", &access_uart_eeprom_default},
	{ "mac_address", &access_uart_eeprom_default},
	{ "manufacturer", &access_uart_eeprom_default},
	{ "country_code", &access_uart_eeprom_default},
	{ "vendor_name", &access_uart_eeprom_default},
	{ "diag_version", &access_uart_eeprom_default},
	{ "service_tag", &access_uart_eeprom_default},
	{ "switch_verdor", &access_uart_eeprom_default},
	{ "main_board_version", &access_uart_eeprom_default},
	{ "come_version", &access_uart_eeprom_default},
	{ "ghc0_board_version", &access_uart_eeprom_default},
	{ "ghc1_board_version", &access_uart_eeprom_default},
	{ "eeprom_crc32", &access_uart_eeprom_default},

};

void *get_uart_eeprom_access_data(char *name)
{
	int i=0;
	for(i=0; i<(sizeof(eeprom_sysfs_attr_data_tbl)/sizeof(eeprom_sysfs_attr_data_tbl[0])); i++)
	{
		if(strncmp(name, eeprom_sysfs_attr_data_tbl[i].name, ATTR_NAME_LEN) ==0)
		{
			return &eeprom_sysfs_attr_data_tbl[i];
		}
	}
	return NULL;
}
EXPORT_SYMBOL(get_uart_eeprom_access_data);

static int eeprom_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    struct eeprom_data *data;
    int status = 0;
    int i,num, j = 0;
    EEPROM_PDATA *eeprom_platform_data;
    EEPROM_DATA_ATTR *data_attr;
    EEPROM_SYSFS_ATTR_DATA_ENTRY *sysfs_data_entry;

    if (client == NULL) { 
        pddf_dbg("UART_EEPROM", "NULL Client.. \n");
        goto exit;
    }

    if (pddf_eeprom_ops.pre_probe) 
    {
        status = (pddf_eeprom_ops.pre_probe)(client, dev_id);
        if (status != 0)
            goto exit;
    }

    data = kzalloc(sizeof(struct eeprom_data), GFP_KERNEL); 
    if (!data) {
        status = -ENOMEM; 
        goto exit;
    }

    i2c_set_clientdata(client, data); 

    dev_info(&client->dev, "chip found\n"); 
    
    /* Take control of the platform data */
    eeprom_platform_data = (EEPROM_PDATA *)(client->dev.platform_data);
    num = eeprom_platform_data->len; 
    data->index = eeprom_platform_data->idx; 
    data->num_attr = num;

    /* Add supported attr in the 'attributes' list */
    for (i=0; i<num; i++) 
    {
        struct sensor_device_attribute *dy_ptr = NULL;
        data_attr = eeprom_platform_data->eeprom_attrs + i; 
        sysfs_data_entry = get_uart_eeprom_access_data(data_attr->aname);
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
		
		data->eeprom_attribute_list[i] = &dy_ptr->dev_attr.attr;
		strlcpy(data->attr_info[i].name, data_attr->aname, sizeof(data->attr_info[i].name));
		data->attr_info[i].valid = 0;
		mutex_init(&data->attr_info[i].update_lock);

    }
    data->eeprom_attribute_list[i+j] = NULL;
	data->eeprom_attribute_group.attrs = data->eeprom_attribute_list;

    /* Register sysfs hooks */
    status = sysfs_create_group(&client->dev.kobj, &data->eeprom_attribute_group); 
    if (status) {
        goto exit_free; 
    }

    data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, client->name, NULL, NULL); 
    if (IS_ERR(data->hwmon_dev)) { 
        status = PTR_ERR(data->hwmon_dev);
        goto exit_remove; 
    }

    dev_info(&client->dev, "%s: eeprom '%s'\n",
         dev_name(data->hwmon_dev), client->name);
    
    /* Add a support for post probe function */
    if (pddf_eeprom_ops.post_probe)
    {
        status = (pddf_eeprom_ops.post_probe)(client, dev_id);
        if (status != 0)
            goto exit_remove;
    }


    return 0;


exit_remove: 
    sysfs_remove_group(&client->dev.kobj, &data->eeprom_attribute_group);
exit_free:
    /* Free all the allocated attributes */
    for (i=0;data->eeprom_attribute_list[i]!=NULL;i++)
	{
		struct sensor_device_attribute *ptr = (struct sensor_device_attribute *)data->eeprom_attribute_list[i];
		kfree(ptr);
		data->eeprom_attribute_list[i] = NULL;
		pddf_dbg("UART_EEPROM", KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
	}
    kfree(data);
exit:
    
    return status;
}

static void eeprom_remove(struct i2c_client *client)
{
    int i = 0, ret = 0;
    struct eeprom_data *data = i2c_get_clientdata(client); 
    EEPROM_PDATA *platdata = (EEPROM_PDATA *)client->dev.platform_data; 
    EEPROM_DATA_ATTR *platdata_sub = platdata->eeprom_attrs;
    struct sensor_device_attribute *ptr = NULL;

    if (pddf_eeprom_ops.pre_remove)
    {
        ret = (pddf_eeprom_ops.pre_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "EEPROM pre_remove function failed\n");
    }

    hwmon_device_unregister(data->hwmon_dev);   
    sysfs_remove_group(&client->dev.kobj, &data->eeprom_attribute_group);
    for (i=0; data->eeprom_attribute_list[i]!=NULL; i++)
    {
        ptr = (struct sensor_device_attribute *)data->eeprom_attribute_list[i];
		kfree(ptr);
		data->eeprom_attribute_list[i] = NULL;
	}
    pddf_dbg("UART_EEPROM", KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
    kfree(data); 

    if (platdata_sub) { 
        pddf_dbg("UART_EEPROM", KERN_DEBUG "%s: Freeing platform subdata\n", __FUNCTION__);
        kfree(platdata_sub);
    }
    if (platdata) { 
        pddf_dbg("UART_EEPROM", KERN_DEBUG "%s: Freeing platform data\n", __FUNCTION__);
        kfree(platdata);
    }
    
    if (pddf_eeprom_ops.post_remove)
    {
        ret = (pddf_eeprom_ops.post_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "EEPROM post_remove function failed\n");
    }
}

enum eeprom_intf 
{
    EEPROM_CTRL_INTF,
};

static const struct i2c_device_id eeprom_ids[] = { 
    { "eeprom_uart", EEPROM_CTRL_INTF },
    {}
};

MODULE_DEVICE_TABLE(i2c, eeprom_ids); 

static struct i2c_driver eeprom_driver = {
    /*.class        = I2C_CLASS_HWMON,*/
    .driver = {
        .name     = "eeprom_uart",
        .owner    = THIS_MODULE,
    },
    .probe        = eeprom_probe,
    .remove       = eeprom_remove,
    .id_table     = eeprom_ids, 
};


int eeprom_uart_init(void)
{
    int ret = 0;

    if (pddf_eeprom_ops.pre_init) 
    {
        ret = (pddf_eeprom_ops.pre_init)();
        if (ret!=0)
            return ret;
    }

    pddf_dbg("UART_EEPROM", KERN_ERR "UART EEPROM DRIVER.. init Invoked..\n");
    ret = i2c_add_driver(&eeprom_driver); 
    if (ret!=0) 
        return ret;

    if (pddf_eeprom_ops.post_init) 
    {
        ret = (pddf_eeprom_ops.post_init)();
        if (ret!=0)
            return ret;
    }
    
    return ret;
}
EXPORT_SYMBOL(eeprom_uart_init);

void __exit eeprom_uart_exit(void)
{
    pddf_dbg("UART_EEPROM", "UART EEPROM DRIVER.. exit\n");
    if (pddf_eeprom_ops.pre_exit) (pddf_eeprom_ops.pre_exit)();
    i2c_del_driver(&eeprom_driver);
    if (pddf_eeprom_ops.post_exit) (pddf_eeprom_ops.post_exit)();

}
EXPORT_SYMBOL(eeprom_uart_exit);

MODULE_AUTHOR("lanxuhui");
MODULE_DESCRIPTION("Driver for eeprom information");
MODULE_LICENSE("GPL");

module_init(eeprom_uart_init); 
module_exit(eeprom_uart_exit);
