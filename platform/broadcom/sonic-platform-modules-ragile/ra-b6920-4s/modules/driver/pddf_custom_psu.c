#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>

#include "../../../../../pddf/i2c/modules/include/pddf_psu_defs.h"
#include "../../../../../pddf/i2c/modules/include/pddf_psu_driver.h"
#include "../../../common/modules/pmbus.h"

int pddf_custom_psu_present(void *client, PSU_DATA_ATTR *adata, void *data);
long pmbus_reg2data_liner(void *client, int data, int class);
int pddf_custom_psu_power_good(void *client, PSU_DATA_ATTR *adata, void *data);
int smbus_read_byte(struct i2c_client *client, uint8_t offset);
ssize_t pddf_psu_custom_show(struct device *dev, struct device_attribute *da, char *buf);

extern void *get_device_table(char *name);
extern PSU_SYSFS_ATTR_DATA access_psu_present;
extern PSU_SYSFS_ATTR_DATA access_psu_power_good;
extern PSU_SYSFS_ATTR_DATA access_psu_v_out;

int pddf_custom_psu_present(void *client, PSU_DATA_ATTR *adata, void *data)
{
    int ret;
    struct i2c_client *client_ptr;
    struct psu_attr_info *padata;

    ret = -1;
    client_ptr = NULL;
    padata = (struct psu_attr_info *)data;

    if (strncmp(adata->devtype, "io", strlen("io")) == 0) {
        ret = inb(adata->offset);
        /* printk("%s read data %x\n", __FUNCTION__, ret); */

        if (ret < 0) {
            return ret;
        }

        padata->val.intval = ((ret & adata->mask) == adata->cmpval);
    }

    else if (strncmp(adata->devtype, "cpld", strlen("cpld")) == 0) {
        client_ptr = (struct i2c_client *)get_device_table(adata->devname);
        if (client_ptr) {
            ret = i2c_smbus_read_byte_data(client_ptr, adata->offset);
        }

        if (ret < 0) {
            return ret;
        }

        padata->val.intval = ((ret & adata->mask) == adata->cmpval);
    }

    return 0;
}

int pddf_custom_psu_power_good(void *client, PSU_DATA_ATTR *adata, void *data)
{
    int ret;
    struct i2c_client *client_ptr;
    struct psu_attr_info *padata;

    ret = -1;
    client_ptr = NULL;
    padata = (struct psu_attr_info *)data;

    if (strncmp(adata->devtype, "io", strlen("io")) == 0) {
        ret = inb(adata->offset);
        /* printk("%s read data %x\n", __FUNCTION__, ret); */

        if (ret < 0) {
            return ret;
        }

        padata->val.intval = ((ret & adata->mask) == adata->cmpval);
    }

    else if (strncmp(adata->devtype, "cpld", strlen("cpld")) == 0) {
        client_ptr = (struct i2c_client *)get_device_table(adata->devname);
        if (client_ptr) {
            ret = i2c_smbus_read_byte_data(client_ptr, adata->offset);
        }

        if (ret < 0) {
            return ret;
        }

        padata->val.intval = ((ret & adata->mask) == adata->cmpval);
    }

    return 0;
}

int smbus_read_byte(struct i2c_client *client, uint8_t offset)
{
    int retry;
    int status;

    retry = 10;
    while (retry) {
        status = i2c_smbus_read_byte_data(client, offset);
        if (unlikely(status < 0)) {
            msleep(60);
            retry--;
            continue;
        }
        break;
    }

    if (status < 0) {
        /* TODO perror*/
    }

    return status;
}

long pmbus_reg2data_liner(void *client, int data, int class)
{
    s16 exponent;
    s32 mantissa;
    long val;
    int vout_mode;

    vout_mode = smbus_read_byte((struct i2c_client *)client, PMBUS_VOUT_MODE);
    /* printk("%s vout mode %x\n", __FUNCTION__, vout_mode); */
    if (vout_mode < 0) {
        return 0;
    }

	/* LINEAR16 */
    if (class == PSC_VOLTAGE_OUT) {
        /* exponent = data->exponent[sensor->page]; */
        exponent = ((s8)(vout_mode << 3)) >> 3;
        mantissa = (u16) data;
    } else {
        /* LINEAR11 */
        exponent = ((s16)data) >> 11;
        mantissa = ((s16)((data & 0x7ff) << 5)) >> 5;
    }

    val = mantissa;
    val = val * 1000L;

    /* scale result to micro-units for power sensors */
    if (class == PSC_POWER) {
        val = val * 1000L;
    }

    if (exponent >= 0) {
        val <<= exponent;
    } else {
        val >>= -exponent;
    }

    /* printk("%s class %d ex %x ma %x val %x\n", __FUNCTION__, class, exponent, mantissa, val); */

    return val;
}

int psu_update_attr(struct device *dev, struct psu_attr_info *data, PSU_DATA_ATTR *udata)
{
    int status;
    struct i2c_client *client;
    PSU_SYSFS_ATTR_DATA *sysfs_attr_data;

    status = 0;
    client = to_i2c_client(dev);
    sysfs_attr_data = NULL;

    mutex_lock(&data->update_lock);

    if (time_after(jiffies, data->last_updated + HZ + HZ / 2) || !data->valid) {
        dev_dbg(&client->dev, "Starting update for %s\n", data->name);

        sysfs_attr_data = udata->access_data;
        if (sysfs_attr_data->pre_get != NULL) {
            status = (sysfs_attr_data->pre_get)(client, udata, data);
            if (status != 0) {
                printk(KERN_ERR "%s: pre_get function fails for %s attribute\n", __FUNCTION__, udata->aname);
            }
        }
        if (sysfs_attr_data->do_get != NULL) {
            status = (sysfs_attr_data->do_get)(client, udata, data);
            if (status != 0) {
                printk(KERN_ERR "%s: do_get function fails for %s attribute\n", __FUNCTION__, udata->aname);
            }
        }
        if (sysfs_attr_data->post_get != NULL) {
            status = (sysfs_attr_data->post_get)(client, udata, data);
            if (status != 0) {
                printk(KERN_ERR "%s: post_get function fails for %s attribute\n", __FUNCTION__, udata->aname);
            }
        }

        data->last_updated = jiffies;
        data->valid = 1;
    }

    mutex_unlock(&data->update_lock);

    return 0;
}

void get_psu_duplicate_sysfs(int idx, char *str)
{
    switch (idx) {
        case PSU_V_OUT:
            strcpy(str, "in3_input");
            break;
        case PSU_I_OUT:
            strcpy(str, "curr2_input");
            break;
        case PSU_P_OUT:
            strcpy(str, "power2_input");
            break;
        case PSU_FAN1_SPEED:
            strcpy(str, "fan1_input");
            break;
        case PSU_TEMP1_INPUT:
            strcpy(str, "temp1_input");
            break;
        default:
            break;
    }

    return;
}

ssize_t pddf_psu_custom_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct sensor_device_attribute *attr;
    struct i2c_client *client;
    struct psu_data *data;

    PSU_PDATA *pdata;
    PSU_DATA_ATTR *usr_data;
    PSU_SYSFS_ATTR_DATA *ptr;
    struct psu_attr_info *sysfs_attr_info;
    int i;
    int status;
    char new_str[ATTR_NAME_LEN];

    attr = to_sensor_dev_attr(da);
    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);

    pdata = (PSU_PDATA *)(client->dev.platform_data);
    usr_data = NULL;
    ptr = NULL;
    sysfs_attr_info = NULL;
    status = 0;
    memset(new_str, 0, ATTR_NAME_LEN);

    for (i = 0; i < data->num_attr; i++) {
        ptr = (PSU_SYSFS_ATTR_DATA *)pdata->psu_attrs[i].access_data;
        get_psu_duplicate_sysfs(ptr->index , new_str);
        if (strcmp(attr->dev_attr.attr.name, pdata->psu_attrs[i].aname) == 0 || \
                strcmp(attr->dev_attr.attr.name, new_str) == 0 ) {
            sysfs_attr_info = &data->attr_info[i];
            usr_data = &pdata->psu_attrs[i];
            /* strcpy(new_str, ""); */
        }
    }

    if (sysfs_attr_info == NULL || usr_data == NULL) {
        printk(KERN_ERR "%s is not supported attribute for this client\n", attr->dev_attr.attr.name);
        goto exit;
    }

    psu_update_attr(dev, sysfs_attr_info, usr_data);

    switch(attr->index) {
        case PSU_V_OUT:
            status = pmbus_reg2data_liner(client, sysfs_attr_info->val.shortval, PSC_VOLTAGE_OUT);
            break;
        case PSU_P_OUT:
            status = pmbus_reg2data_liner(client, sysfs_attr_info->val.shortval, PSC_POWER);
            break;
        case PSU_I_OUT:
        case PSU_V_IN:
        case PSU_I_IN:
        case PSU_TEMP1_INPUT:
            status = pmbus_reg2data_liner(client, sysfs_attr_info->val.shortval, PSC_NUM_CLASSES);
            break;
        default:
            printk(KERN_ERR "%s: Unable to find attribute index for %s\n", __FUNCTION__, usr_data->aname);
            goto exit;
    }

exit:
    return sprintf(buf, "%d\n", status);
}

int __init pddf_custom_psu_init(void)
{
    access_psu_present.do_get = pddf_custom_psu_present;
    access_psu_power_good.do_get = pddf_custom_psu_power_good;
    access_psu_v_out.show = pddf_psu_custom_show;
    printk(KERN_ERR "pddf_custom_psu_init\n");
    return 0;
}

void __exit pddf_custom_psu_exit(void)
{
    printk(KERN_ERR "pddf_custom_psu_exit\n");
    return;
}

MODULE_AUTHOR("support <support@ragile.com>");
MODULE_DESCRIPTION("pddf custom psu api");
MODULE_LICENSE("GPL");

module_init(pddf_custom_psu_init);
module_exit(pddf_custom_psu_exit);

