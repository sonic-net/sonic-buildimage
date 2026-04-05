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
#include "uart_psu_defs.h"
#include "uart_psu_api.h"


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

PSU_SYSFS_ATTR_DATA access_uart_psu_presence = {PSU_PRESENCE, S_IRUGO, psu_show_default, NULL, bmc_get_psu_presence, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_presence);

PSU_SYSFS_ATTR_DATA access_uart_psu_status = {PSU_STATUS, S_IRUGO, psu_show_default, NULL, bmc_get_psu_status, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_status);

PSU_SYSFS_ATTR_DATA access_uart_psu_type = {PSU_TYPE, S_IRUGO, psu_show_default, NULL, bmc_get_psu_type, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_type);

PSU_SYSFS_ATTR_DATA access_uart_psu_v_in = {PSU_VIN, S_IRUGO, psu_show_default, NULL, bmc_get_psu_v_in, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_v_in);

PSU_SYSFS_ATTR_DATA access_uart_psu_v_out = {PSU_VOUT, S_IRUGO, psu_show_default, NULL, bmc_get_psu_v_out, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_v_out);

PSU_SYSFS_ATTR_DATA access_uart_psu_i_in = {PSU_IIN, S_IRUGO, psu_show_default, NULL, bmc_get_psu_i_in, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_i_in);

PSU_SYSFS_ATTR_DATA access_uart_psu_i_out = {PSU_IOUT, S_IRUGO, psu_show_default, NULL, bmc_get_psu_i_out, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_i_out);

PSU_SYSFS_ATTR_DATA access_uart_psu_p_in = {PSU_PIN, S_IRUGO, psu_show_default, NULL, bmc_get_psu_p_in, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_p_in);

PSU_SYSFS_ATTR_DATA access_uart_psu_p_out = {PSU_POUT, S_IRUGO, psu_show_default, NULL, bmc_get_psu_p_out, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_p_out);

PSU_SYSFS_ATTR_DATA access_uart_psu_direction = {PSU_DIRECTION, S_IRUGO, psu_show_default, NULL, bmc_get_psu_direction, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_direction);

PSU_SYSFS_ATTR_DATA access_uart_psu_warning = {PSU_WARNING, S_IRUGO, psu_show_default, NULL, bmc_get_psu_warning, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_warning);

PSU_SYSFS_ATTR_DATA access_uart_psu_direction_warning = {PSU_DIRECTION_WARNING, S_IRUGO, psu_show_default, NULL, bmc_get_psu_direction_warning, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_direction_warning);

PSU_SYSFS_ATTR_DATA access_uart_psu_model = {PSU_MODEL, S_IRUGO, psu_show_default, NULL, bmc_get_psu_model, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_model);

PSU_SYSFS_ATTR_DATA access_uart_psu_serial_number = {PSU_SERIAL_NUMBER, S_IRUGO, psu_show_default, NULL, bmc_get_psu_serial_number, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_serial_number);

PSU_SYSFS_ATTR_DATA access_uart_psu_temp = {PSU_TEMP, S_IRUGO, psu_show_default, NULL, bmc_get_psu_temp, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_temp);

PSU_SYSFS_ATTR_DATA access_uart_psu_fan_speed = {PSU_FAN_SPEED, S_IRUGO, psu_show_default, NULL, bmc_get_psu_fan_speed, NULL, NULL, NULL, NULL, NULL};
EXPORT_SYMBOL(access_uart_psu_fan_speed);

PSU_SYSFS_ATTR_DATA_ENTRY psu_sysfs_attr_data_tbl[]=
{
	{ "psu_presence", &access_uart_psu_presence},
	{ "psu_status", &access_uart_psu_status},
	{ "psu_type", &access_uart_psu_type},
	{ "psu_v_in", &access_uart_psu_v_in},
	{ "psu_v_out", &access_uart_psu_v_out},
	{ "psu_i_in", &access_uart_psu_i_in},
	{ "psu_i_out", &access_uart_psu_i_out},
	{ "psu_p_in", &access_uart_psu_p_in},
	{ "psu_p_out", &access_uart_psu_p_out},
	{ "psu_direction", &access_uart_psu_direction},
	{ "psu_warning", &access_uart_psu_warning},
    { "psu_direction_warning", &access_uart_psu_direction_warning},
    { "psu_model", &access_uart_psu_model},
    { "psu_serial_number", &access_uart_psu_serial_number},
    { "psu_temp", &access_uart_psu_temp},
    { "psu_fan_speed", &access_uart_psu_fan_speed}
};

void *get_uart_psu_access_data(char *name)
{
	int i=0;
	for(i=0; i<(sizeof(psu_sysfs_attr_data_tbl)/sizeof(psu_sysfs_attr_data_tbl[0])); i++)
	{
		if(strncmp(name, psu_sysfs_attr_data_tbl[i].name, ATTR_NAME_LEN) ==0)
		{
			return &psu_sysfs_attr_data_tbl[i];
		}
	}
	return NULL;
}
EXPORT_SYMBOL(get_uart_psu_access_data);

static int psu_probe(struct i2c_client *client,
            const struct i2c_device_id *dev_id)
{
    struct psu_data *data;
    int status = 0;
    int i,num, j = 0;
    PSU_PDATA *psu_platform_data;
    PSU_DATA_ATTR *data_attr;
    PSU_SYSFS_ATTR_DATA_ENTRY *sysfs_data_entry;

    if (client == NULL) { 
        pddf_dbg("UART_PSU", "NULL Client.. \n");
        goto exit;
    }

    if (pddf_psu_ops.pre_probe) 
    {
        status = (pddf_psu_ops.pre_probe)(client, dev_id);
        if (status != 0)
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
    data->index = psu_platform_data->idx; 
    data->num_attr = num;

    /* Add supported attr in the 'attributes' list */
    for (i=0; i<num; i++) 
    {
        struct sensor_device_attribute *dy_ptr = NULL;
        data_attr = psu_platform_data->psu_attrs + i; 
        sysfs_data_entry = get_uart_psu_access_data(data_attr->aname);
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
		
		data->psu_attribute_list[i] = &dy_ptr->dev_attr.attr;
		strlcpy(data->attr_info[i].name, data_attr->aname, sizeof(data->attr_info[i].name));
		data->attr_info[i].valid = 0;
		mutex_init(&data->attr_info[i].update_lock);

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
		pddf_dbg("UART_PSU", KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
	}
    kfree(data);
exit:
    
    return status;
}

static void psu_remove(struct i2c_client *client)
{
    int i = 0, ret = 0;
    struct psu_data *data = i2c_get_clientdata(client); 
    PSU_PDATA *platdata = (PSU_PDATA *)client->dev.platform_data; 
    PSU_DATA_ATTR *platdata_sub = platdata->psu_attrs;
    struct sensor_device_attribute *ptr = NULL;

    if (pddf_psu_ops.pre_remove)
    {
        ret = (pddf_psu_ops.pre_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "PSU pre_remove function failed\n");
    }

    hwmon_device_unregister(data->hwmon_dev);   
    sysfs_remove_group(&client->dev.kobj, &data->psu_attribute_group);
    for (i=0; data->psu_attribute_list[i]!=NULL; i++)
    {
        ptr = (struct sensor_device_attribute *)data->psu_attribute_list[i];
		kfree(ptr);
		data->psu_attribute_list[i] = NULL;
	}
    pddf_dbg("UART_PSU", KERN_ERR "%s: Freed all the memory allocated for attributes\n", __FUNCTION__);
    kfree(data); 

    if (platdata_sub) { 
        pddf_dbg("UART_PSU", KERN_DEBUG "%s: Freeing platform subdata\n", __FUNCTION__);
        kfree(platdata_sub);
    }
    if (platdata) { 
        pddf_dbg("UART_PSU", KERN_DEBUG "%s: Freeing platform data\n", __FUNCTION__);
        kfree(platdata);
    }
    
    if (pddf_psu_ops.post_remove)
    {
        ret = (pddf_psu_ops.post_remove)(client);
        if (ret!=0)
            printk(KERN_ERR "PSU post_remove function failed\n");
    }
}

enum psu_intf 
{
    PSU_CTRL_INTF,
};

static const struct i2c_device_id psu_ids[] = { 
    { "psu_uart", PSU_CTRL_INTF },
    {}
};

MODULE_DEVICE_TABLE(i2c, psu_ids); 

static struct i2c_driver psu_driver = {
    /*.class        = I2C_CLASS_HWMON,*/
    .driver = {
        .name     = "psu_uart",
        .owner    = THIS_MODULE,
    },
    .probe        = psu_probe,
    .remove       = psu_remove,
    .id_table     = psu_ids, 
};


int psu_uart_init(void)
{
    int ret = 0;

    if (pddf_psu_ops.pre_init) 
    {
        ret = (pddf_psu_ops.pre_init)();
        if (ret!=0)
            return ret;
    }

    pddf_dbg("UART_PSU", KERN_ERR "UART PSU DRIVER.. init Invoked..\n");
    ret = i2c_add_driver(&psu_driver); 
    if (ret!=0) 
        return ret;

    if (pddf_psu_ops.post_init) 
    {
        ret = (pddf_psu_ops.post_init)();
        if (ret!=0)
            return ret;
    }
    
    return ret;
}
EXPORT_SYMBOL(psu_uart_init);

void __exit psu_uart_exit(void)
{
    pddf_dbg("UART_PSU", "UART PSU DRIVER.. exit\n");
    if (pddf_psu_ops.pre_exit) (pddf_psu_ops.pre_exit)();
    i2c_del_driver(&psu_driver);
    if (pddf_psu_ops.post_exit) (pddf_psu_ops.post_exit)();

}
EXPORT_SYMBOL(psu_uart_exit);

MODULE_AUTHOR("lanxuhui");
MODULE_DESCRIPTION("Driver for psu information");
MODULE_LICENSE("GPL");

module_init(psu_uart_init); 
module_exit(psu_uart_exit);
