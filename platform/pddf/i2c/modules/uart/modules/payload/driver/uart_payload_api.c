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
#include "uart_payload_defs.h"


/*#define PAYLOAD_DEBUG*/
#ifdef PAYLOAD_DEBUG
#define payload_dbg(...) printk(__VA_ARGS__)
#else
#define payload_dbg(...)
#endif

extern u8 calmode;
extern void set_bmc_data(const char *command, const char *fst_command, const char *sec_command, const u8 vnum_command);
extern void get_bmc_data(u8 command, u8 fst_command, u8 sec_command, u8 vnum_command, union i2c_smbus_data *bmc_read_data);

int payload_update_hw(struct device *dev, struct payload_attr_info *info, PAYLOAD_DATA_ATTR *udata)
{
    int status = 0;
    struct i2c_client *client = to_i2c_client(dev);
    PAYLOAD_SYSFS_ATTR_DATA *sysfs_attr_data = NULL;


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


int payload_update_attr(struct device *dev, struct payload_attr_info *data, PAYLOAD_DATA_ATTR *udata)
{
    int status = 0;
    struct i2c_client *client = to_i2c_client(dev);
    PAYLOAD_SYSFS_ATTR_DATA *sysfs_attr_data=NULL;

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


ssize_t payload_show_default(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct payload_data *data = i2c_get_clientdata(client);
    PAYLOAD_PDATA *pdata = (PAYLOAD_PDATA *)(client->dev.platform_data);
    PAYLOAD_DATA_ATTR *usr_data = NULL;
    struct payload_attr_info *sysfs_attr_info = NULL;
    int i, status=0;

    for (i=0;i<data->num_attr;i++)
    {
        if ( strncmp(attr->dev_attr.attr.name, pdata->payload_attrs[i].aname, ATTR_NAME_LEN) == 0 ) 
        {
            sysfs_attr_info = &data->attr_info[i];
            usr_data = &pdata->payload_attrs[i];
        }
    }

    if (sysfs_attr_info==NULL || usr_data==NULL)
    {
        printk(KERN_ERR "%s is not supported attribute for this client\n", attr->dev_attr.attr.name);
        goto exit;
    }

    payload_update_attr(dev, sysfs_attr_info, usr_data);

    switch(attr->index)
    {
        case PAYLOAD_GET:
            return scnprintf(buf, PAGE_SIZE, "%s\n", sysfs_attr_info->val.strval);
            break;
        default:
            printk(KERN_ERR "%s: Unable to find attribute index for %s\n", __FUNCTION__, usr_data->aname);
            goto exit;
    }

exit:
    return scnprintf(buf, PAGE_SIZE, "%d\n", status);
}


ssize_t payload_store_default(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct payload_data *data = i2c_get_clientdata(client);
    PAYLOAD_PDATA *pdata = (PAYLOAD_PDATA *)(client->dev.platform_data);
    PAYLOAD_DATA_ATTR *usr_data = NULL;
    struct payload_attr_info *sysfs_attr_info = NULL;
    int i;

    for (i=0;i<data->num_attr;i++)
    {
        if (strncmp(data->attr_info[i].name, attr->dev_attr.attr.name, ATTR_NAME_LEN) == 0 && strncmp(pdata->payload_attrs[i].aname, attr->dev_attr.attr.name, ATTR_NAME_LEN) == 0)
        {
            sysfs_attr_info = &data->attr_info[i];
            usr_data = &pdata->payload_attrs[i];
        }
    }

    if (sysfs_attr_info==NULL || usr_data==NULL) {
        printk(KERN_ERR "%s is not supported attribute for this client\n", attr->dev_attr.attr.name);
        goto exit;
    }

    switch(attr->index)
    {
        /*No write attributes for now in PAYLOAD*/
        default:
            goto exit;
    }

    payload_update_hw(dev, sysfs_attr_info, usr_data);

exit:
    return count;
}

int bmc_get_payload(struct i2c_client *client, PAYLOAD_DATA_ATTR *info, void *data)
{
    struct payload_attr_info *padata = (struct payload_attr_info *)data;
    u8 command = info->cmd;
    u8 sub_command_1 = info->subcmd1;
    u8 sub_command_2 = info->subcmd2;
    u8 byte = info->byte;
    u32 value = 0;
    union i2c_smbus_data bmc_read_data = {.block={0x00}};

    if (calmode == 1)
    {
        get_bmc_data(command, sub_command_1, sub_command_2, 3, &bmc_read_data);
        value = bmc_read_data.block[byte] * 1000 + bmc_read_data.block[byte + 1] * 100;
        scnprintf(padata->val.strval, sizeof(padata->val.strval), "%d", value);
    }
    else if (calmode == 2)
    {
        get_bmc_data(command, sub_command_1, sub_command_2, 3, &bmc_read_data);
        value = (((bmc_read_data.block[byte] << 8) & 0xff00) | (bmc_read_data.block[byte + 1] & 0xff))*100;
        scnprintf(padata->val.strval, sizeof(padata->val.strval), "%d", value);
    }
    else
    {
        scnprintf(padata->val.strval, sizeof(padata->val.strval), "wrong calmode!");
    }
    return 0;
}
