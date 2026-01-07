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


static ssize_t do_attr_operation(struct device *dev, struct device_attribute *da, const char *buf, __kernel_size_t count);
static ssize_t do_device_operation(struct device *dev, struct device_attribute *da, const char *buf, __kernel_size_t count);
extern void *get_uart_eeprom_access_data(char *);
extern void *get_device_table(char *name);
extern void delete_device_table(char *name);

EEPROM_DATA eeprom_data = {0};



/* EEPROM CLIENT DATA */
PDDF_DATA_ATTR(eeprom_idx, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&eeprom_data.idx, NULL);

PDDF_DATA_ATTR(attr_name, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_CHAR, 32, (void*)&eeprom_data.eeprom_attr.aname, NULL);
PDDF_DATA_ATTR(attr_cmd, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&eeprom_data.eeprom_attr.cmd, NULL);
PDDF_DATA_ATTR(attr_subcmd1, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&eeprom_data.eeprom_attr.subcmd1, NULL);
PDDF_DATA_ATTR(attr_subcmd2, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&eeprom_data.eeprom_attr.subcmd2, NULL);
PDDF_DATA_ATTR(attr_byte, S_IWUSR|S_IRUGO, show_pddf_data, store_pddf_data, PDDF_INT_DEC, sizeof(int), (void*)&eeprom_data.eeprom_attr.byte, NULL);
PDDF_DATA_ATTR(attr_ops, S_IWUSR, NULL, do_attr_operation, PDDF_CHAR, 8, (void*)&eeprom_data, NULL);
PDDF_DATA_ATTR(dev_ops, S_IWUSR, NULL, do_device_operation, PDDF_CHAR, 8, (void*)&eeprom_data, (void*)&pddf_data);



static struct attribute *eeprom_attributes[] = {
    &attr_eeprom_idx.dev_attr.attr,

    &attr_attr_name.dev_attr.attr,
    &attr_attr_cmd.dev_attr.attr,
    &attr_attr_subcmd1.dev_attr.attr,
    &attr_attr_subcmd2.dev_attr.attr,
    &attr_attr_byte.dev_attr.attr,
    &attr_attr_ops.dev_attr.attr,
    &attr_dev_ops.dev_attr.attr,
    NULL
};

static const struct attribute_group pddf_eeprom_client_data_group = {
    .attrs = eeprom_attributes,
};


static ssize_t do_attr_operation(struct device *dev, struct device_attribute *da, const char *buf, __kernel_size_t count)
{
    PDDF_ATTR *ptr = (PDDF_ATTR *)da;
    EEPROM_DATA *pdata = (EEPROM_DATA *)(ptr->addr);
    EEPROM_SYSFS_ATTR_DATA_ENTRY *access_ptr;

    pdata->eeprom_attrs[pdata->len] = pdata->eeprom_attr; 
    access_ptr = get_uart_eeprom_access_data(pdata->eeprom_attrs[pdata->len].aname);
    if (access_ptr != NULL && access_ptr->a_ptr != NULL)
    {
        pdata->eeprom_attrs[pdata->len].access_data = access_ptr->a_ptr ;
    }

    pdata->len++;
    memset(&pdata->eeprom_attr, 0, sizeof(pdata->eeprom_attr));


    return count;
}

struct i2c_board_info *i2c_get_eeprom_board_info(EEPROM_DATA *pdata, NEW_DEV_ATTR *cdata)
{
    int num = pdata->len; 
    int i = 0;
    static struct i2c_board_info board_info;
    EEPROM_PDATA *eeprom_platform_data;
    
    
    if (strncmp(cdata->dev_type, "eeprom_uart", GEN_NAME_SIZE)==0 || strncmp(cdata->dev_type, "eeprom_eeprom", GEN_NAME_SIZE)==0 )
    {
        /* Allocate the eeprom_platform_data */
        eeprom_platform_data = (EEPROM_PDATA *)kzalloc(sizeof(EEPROM_PDATA), GFP_KERNEL);
        eeprom_platform_data->eeprom_attrs = (EEPROM_DATA_ATTR *)kzalloc(num*sizeof(EEPROM_DATA_ATTR), GFP_KERNEL);

        eeprom_platform_data->idx = pdata->idx;  
        eeprom_platform_data->len = pdata->len; 

        for (i=0;i<num;i++)
        {
            eeprom_platform_data->eeprom_attrs[i] = pdata->eeprom_attrs[i]; 
        }

        board_info = (struct i2c_board_info) {
            .platform_data = eeprom_platform_data, 
        };

        board_info.addr = cdata->dev_addr; 
        strlcpy(board_info.type, cdata->dev_type, sizeof(board_info.type));
    }
    else
    {
        printk(KERN_ERR "%s:Unknown type of device %s. Unable to clreate I2C client for it\n",__FUNCTION__, cdata->dev_type);
    }

    return &board_info;
}


static ssize_t do_device_operation(struct device *dev, struct device_attribute *da, const char *buf, __kernel_size_t count)
{
    PDDF_ATTR *ptr = (PDDF_ATTR *)da;
    EEPROM_DATA *pdata = (EEPROM_DATA *)(ptr->addr);
    NEW_DEV_ATTR *cdata = (NEW_DEV_ATTR *)(ptr->data);
    struct i2c_adapter *adapter;
    struct i2c_board_info *board_info;
    struct i2c_client *client_ptr;


    if (strncmp(buf, "add", strlen("add"))==0)
    {
        adapter = i2c_get_adapter(cdata->parent_bus); 
        board_info = i2c_get_eeprom_board_info(pdata, cdata);

        /* Populate the platform data for eeprom */
        client_ptr = i2c_new_client_device(adapter, board_info);
        
        if(!IS_ERR(client_ptr))
        {
            i2c_put_adapter(adapter);
            pddf_dbg("UART_EEPROM", KERN_ERR "Created a %s client: 0x%p\n", cdata->i2c_name , (void *)client_ptr);
            add_device_table(cdata->i2c_name, (void*)client_ptr); 
        }
        else
        {
            i2c_put_adapter(adapter);
            goto free_data;
        }
    }
    else if (strncmp(buf, "delete", strlen("delete"))==0)
    {
        /*Get the i2c_client handle for the created client*/
        client_ptr = (struct i2c_client *)get_device_table(cdata->i2c_name);
        if (client_ptr)
        {
            pddf_dbg("UART_EEPROM", KERN_ERR "Removing %s client: 0x%p\n", cdata->i2c_name, (void *)client_ptr);
            i2c_unregister_device(client_ptr);
            delete_device_table(cdata->i2c_name);
        }
        else
        {
            printk(KERN_ERR "Unable to get the client handle for %s\n", cdata->i2c_name);
        }

    }
    else
    {
        printk(KERN_ERR "PDDF_ERROR: %s: Invalid value for dev_ops %s", __FUNCTION__, buf);
    }

    goto clear_data;

free_data:
    if (board_info->platform_data)
    {
        EEPROM_PDATA *eeprom_platform_data = board_info->platform_data;
        if (eeprom_platform_data->eeprom_attrs)
        {
            printk(KERN_ERR "%s: Unable to create i2c client. Freeing the platform subdata\n", __FUNCTION__);
            kfree(eeprom_platform_data->eeprom_attrs);
        }
        printk(KERN_ERR "%s: Unable to create i2c client. Freeing the platform data\n", __FUNCTION__);
        kfree(eeprom_platform_data);
    }

clear_data:
    memset(pdata, 0, sizeof(EEPROM_DATA));
    /*TODO: free the data cdata->data if data is dynal=mically allocated*/
    memset(cdata, 0, sizeof(NEW_DEV_ATTR));
    return count;
}


static struct kobject *eeprom_kobj;
static struct kobject *uart_eeprom_kobj;

int __init pddf_data_init(void)
{
    struct kobject *device_kobj;
    int ret = 0;


    pddf_dbg("UART_EEPROM", "PDDF_DATA MODULE.. init\n");

    device_kobj = get_device_i2c_kobj();
    if(!device_kobj) 
        return -ENOMEM;

    uart_eeprom_kobj = kobject_create_and_add("uart_eeprom", device_kobj);
    if(!uart_eeprom_kobj) 
        return -ENOMEM;
    eeprom_kobj = kobject_create_and_add("eeprom", uart_eeprom_kobj);
    if(!eeprom_kobj) 
        return -ENOMEM;
    
    ret = sysfs_create_group(eeprom_kobj, &pddf_clients_data_group);
    if (ret)
    {
        kobject_put(eeprom_kobj);
        kobject_put(uart_eeprom_kobj);
        return ret;
    }
    pddf_dbg("UART_EEPROM", "CREATED EEPROM I2C CLIENTS CREATION SYSFS GROUP\n");

    ret = sysfs_create_group(eeprom_kobj, &pddf_eeprom_client_data_group);
    if (ret)
    {
        sysfs_remove_group(eeprom_kobj, &pddf_clients_data_group);
        kobject_put(eeprom_kobj);
        kobject_put(uart_eeprom_kobj);
        return ret;
    }
    pddf_dbg("UART_EEPROM", "CREATED PDDF EEPROM DATA SYSFS GROUP\n");
    
    return ret;
}

void __exit pddf_data_exit(void)
{

    pddf_dbg("UART_EEPROM", "PDDF_DATA MODULE.. exit\n");
    sysfs_remove_group(eeprom_kobj, &pddf_eeprom_client_data_group);
    sysfs_remove_group(eeprom_kobj, &pddf_clients_data_group);
    kobject_put(eeprom_kobj);
    kobject_put(uart_eeprom_kobj);
    pddf_dbg("UART_EEPROM", KERN_ERR "%s: Removed the kobjects for 'uart' and 'eeprom'\n",__FUNCTION__);

    return;
}

module_init(pddf_data_init);
module_exit(pddf_data_exit);

MODULE_AUTHOR("lanxuhui");
MODULE_DESCRIPTION("eeprom platform data");
MODULE_LICENSE("GPL");
