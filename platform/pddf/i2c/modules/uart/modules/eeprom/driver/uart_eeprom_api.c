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
#include "uart_eeprom_defs.h"


/*#define EEPROM_DEBUG*/
#ifdef EEPROM_DEBUG
#define eeprom_dbg(...) printk(__VA_ARGS__)
#else
#define eeprom_dbg(...)
#endif

extern void set_bmc_data(const char *command, const char *fst_command, const char *sec_command, const u8 vnum_command);
extern void get_bmc_data(u8 command, u8 fst_command, u8 sec_command, u8 vnum_command, union i2c_smbus_data *bmc_read_data);

int eeprom_update_hw(struct device *dev, struct eeprom_attr_info *info, EEPROM_DATA_ATTR *udata)
{
    int status = 0;
    struct i2c_client *client = to_i2c_client(dev);
    EEPROM_SYSFS_ATTR_DATA *sysfs_attr_data = NULL;


    mutex_lock(&info->update_lock);

    sysfs_attr_data = udata->access_data;
    if (sysfs_attr_data->pre_set != NULL)
    {
        status = (sysfs_attr_data->pre_set)(client, udata, info);
        if (status!=0)
            dev_warn(&client->dev, "%s: pre_set function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);
    }
    if (sysfs_attr_data->do_set != NULL)
    {
        status = (sysfs_attr_data->do_set)(client, udata, info);
        if (status!=0)
            dev_warn(&client->dev, "%s: do_set function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);

    }
    if (sysfs_attr_data->post_set != NULL)
    {
        status = (sysfs_attr_data->post_set)(client, udata, info);
        if (status!=0)
            dev_warn(&client->dev, "%s: post_set function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);
    }

    mutex_unlock(&info->update_lock);

    return 0;
}


int eeprom_update_attr(struct device *dev, struct eeprom_attr_info *data, EEPROM_DATA_ATTR *udata)
{
    int status = 0;
    struct i2c_client *client = to_i2c_client(dev);
    EEPROM_SYSFS_ATTR_DATA *sysfs_attr_data=NULL;

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ + HZ / 2) || !data->valid) 
    {
        dev_dbg(&client->dev, "Starting update for %s\n", data->name);

        sysfs_attr_data = udata->access_data;
        if (sysfs_attr_data->pre_get != NULL)
        {
            status = (sysfs_attr_data->pre_get)(client, udata, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: pre_get function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);
        }
        if (sysfs_attr_data->do_get != NULL)
        {
            status = (sysfs_attr_data->do_get)(client, udata, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: do_get function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);

        }
        if (sysfs_attr_data->post_get != NULL)
        {
            status = (sysfs_attr_data->post_get)(client, udata, data);
            if (status!=0)
                dev_warn(&client->dev, "%s: post_get function fails for %s attribute. ret %d\n", __FUNCTION__, udata->aname, status);
        }

        data->last_updated = jiffies;
        data->valid = 1;
    }

    mutex_unlock(&data->update_lock);
    return 0;
}


ssize_t eeprom_show_default(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct eeprom_data *data = i2c_get_clientdata(client);
    EEPROM_PDATA *pdata = (EEPROM_PDATA *)(client->dev.platform_data);
    EEPROM_DATA_ATTR *usr_data = NULL;
    struct eeprom_attr_info *sysfs_attr_info = NULL;
    int i, status=0;

    for (i=0;i<data->num_attr;i++)
    {
        if ( strncmp(attr->dev_attr.attr.name, pdata->eeprom_attrs[i].aname, ATTR_NAME_LEN) == 0 ) 
        {
            sysfs_attr_info = &data->attr_info[i];
            usr_data = &pdata->eeprom_attrs[i];
        }
    }

    if (sysfs_attr_info==NULL || usr_data==NULL)
    {
        printk(KERN_ERR "%s is not supported attribute for this client\n", attr->dev_attr.attr.name);
        goto exit;
    }

    eeprom_update_attr(dev, sysfs_attr_info, usr_data);

    switch(attr->index)
    {
        case EEPROM_DEFAULT:
            return scnprintf(buf, PAGE_SIZE, "%s", sysfs_attr_info->val.strval);
            break;
        default:
            printk(KERN_ERR "%s: Unable to find attribute index for %s\n", __FUNCTION__, usr_data->aname);
            goto exit;
    }

exit:
    return scnprintf(buf, PAGE_SIZE, "%d\n", status);
}


ssize_t eeprom_store_default(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct eeprom_data *data = i2c_get_clientdata(client);
    EEPROM_PDATA *pdata = (EEPROM_PDATA *)(client->dev.platform_data);
    EEPROM_DATA_ATTR *usr_data = NULL;
    struct eeprom_attr_info *sysfs_attr_info = NULL;
    int i;

    for (i=0;i<data->num_attr;i++)
    {
        if (strncmp(data->attr_info[i].name, attr->dev_attr.attr.name, ATTR_NAME_LEN) == 0 && strncmp(pdata->eeprom_attrs[i].aname, attr->dev_attr.attr.name, ATTR_NAME_LEN) == 0)
        {
            sysfs_attr_info = &data->attr_info[i];
            usr_data = &pdata->eeprom_attrs[i];
        }
    }

    if (sysfs_attr_info==NULL || usr_data==NULL) {
        printk(KERN_ERR "%s is not supported attribute for this client\n", attr->dev_attr.attr.name);
        goto exit;
    }

    switch(attr->index)
    {
        /*No write attributes for now in EEPROM*/
        default:
            goto exit;
    }

    eeprom_update_hw(dev, sysfs_attr_info, usr_data);

exit:
    return count;
}

int bmc_get_eeprom_default(struct i2c_client *client, EEPROM_DATA_ATTR *info, void *data)
{
    struct eeprom_attr_info *padata = (struct eeprom_attr_info *)data;
    u8 command = info->cmd;
    u8 sub_command_1 = info->subcmd1;
    u8 sub_command_2 = info->subcmd2;
    union i2c_smbus_data eeprom_read_tlv_data = {.block={0x00}};

    get_bmc_data(command, sub_command_1, sub_command_2, 3, &eeprom_read_tlv_data);
	
    scnprintf(padata->val.strval, sizeof(padata->val.strval), "%s", eeprom_read_tlv_data.block);
    return 0;
}