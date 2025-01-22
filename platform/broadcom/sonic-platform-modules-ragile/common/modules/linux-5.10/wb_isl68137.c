// SPDX-License-Identifier: GPL-2.0+
/*
 * Hardware monitoring driver for Renesas Digital Multiphase Voltage Regulators
 *
 * Copyright (c) 2017 Google Inc
 * Copyright (c) 2020 Renesas Electronics America
 *
 */

#include <linux/err.h>
#include <linux/hwmon-sysfs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/sysfs.h>

#include "wb_pmbus.h"

#define ISL68137_VOUT_AVS       (0x30)
#define RAA_DMPVR2_READ_VMON    (0xc8)
#define WRITE_PROTECT_CLOSE     (0x00)
#define WRITE_PROTECT_OPEN      (0x40)

static int g_wb_isl68137_debug = 0;
static int g_wb_isl68137_error = 0;

module_param(g_wb_isl68137_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_isl68137_error, int, S_IRUGO | S_IWUSR);

#define WB_ISL68137_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_isl68137_debug) { \
        printk(KERN_INFO "[WB_ISL68137][VER][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_ISL68137_ERROR(fmt, args...) do {                                        \
    if (g_wb_isl68137_error) { \
        printk(KERN_ERR "[WB_ISL68137][ERR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

enum chips {
    isl68137,
    isl68220,
    isl68221,
    isl68222,
    isl68223,
    isl68224,
    isl68225,
    isl68226,
    isl68227,
    isl68229,
    isl68233,
    isl68239,
    isl69222,
    isl69223,
    isl69224,
    isl69225,
    isl69227,
    isl69228,
    isl69234,
    isl69236,
    isl69239,
    isl69242,
    isl69243,
    isl69247,
    isl69248,
    isl69254,
    isl69255,
    isl69256,
    isl69259,
    isl69260,
    isl69268,
    isl69269,
    isl69298,
    raa228000,
    raa228004,
    raa228006,
    raa228228,
    raa229001,
    raa229004,
};

enum variants {
    raa_dmpvr1_2rail,
    raa_dmpvr2_1rail,
    raa_dmpvr2_2rail,
    raa_dmpvr2_2rail_nontc,
    raa_dmpvr2_3rail,
    raa_dmpvr2_hv,
};

static const struct i2c_device_id raa_dmpvr_id[];

static ssize_t isl68137_avs_enable_show_page(struct i2c_client *client,
                         int page,
                         char *buf)
{
    int val = wb_pmbus_read_byte_data(client, page, PMBUS_OPERATION);

    return sprintf(buf, "%d\n",
               (val & ISL68137_VOUT_AVS) == ISL68137_VOUT_AVS ? 1 : 0);
}

static ssize_t isl68137_avs_enable_store_page(struct i2c_client *client,
                          int page,
                          const char *buf, size_t count)
{
    int rc, op_val;
    bool result;

    rc = kstrtobool(buf, &result);
    if (rc)
        return rc;

    op_val = result ? ISL68137_VOUT_AVS : 0;

    /*
     * Writes to VOUT setpoint over AVSBus will persist after the VRM is
     * switched to PMBus control. Switching back to AVSBus control
     * restores this persisted setpoint rather than re-initializing to
     * PMBus VOUT_COMMAND. Writing VOUT_COMMAND first over PMBus before
     * enabling AVS control is the workaround.
     */
    if (op_val == ISL68137_VOUT_AVS) {
        rc = wb_pmbus_read_word_data(client, page, 0xff,
                      PMBUS_VOUT_COMMAND);
        if (rc < 0)
            return rc;

        rc = wb_pmbus_write_word_data(client, page, PMBUS_VOUT_COMMAND,
                       rc);
        if (rc < 0)
            return rc;
    }

    rc = wb_pmbus_update_byte_data(client, page, PMBUS_OPERATION,
                    ISL68137_VOUT_AVS, op_val);

    return (rc < 0) ? rc : count;
}

static ssize_t isl68137_avs_enable_show(struct device *dev,
                    struct device_attribute *devattr,
                    char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);

    return isl68137_avs_enable_show_page(client, attr->index, buf);
}

static ssize_t isl68137_avs_enable_store(struct device *dev,
                struct device_attribute *devattr,
                const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);

    return isl68137_avs_enable_store_page(client, attr->index, buf, count);
}

static ssize_t isl68137_avs_vout_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int ret, vout_cmd, vout;

    mutex_lock(&data->update_lock);
    vout_cmd = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (vout_cmd < 0) {
        WB_ISL68137_ERROR("%d-%04x: read page%d vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, ret);
        mutex_unlock(&data->update_lock);
        return vout_cmd;
    }
    vout = vout_cmd * 1000;
    WB_ISL68137_VERBOSE("%d-%04x: page%d, vout: %d, vout_cmd: 0x%x\n", client->adapter->nr,
        client->addr, attr->index, vout, vout_cmd);
    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%d\n", vout);
}

static ssize_t isl68137_avs_vout_store(struct device *dev, struct device_attribute *devattr,
                   const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int vout, vout_max, vout_min;
    int ret, vout_cmd, vout_cmd_set;

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        WB_ISL68137_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    ret = kstrtoint(buf, 0, &vout);
    if (ret) {
        WB_ISL68137_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    vout_max = data->vout_max[attr->index];
    vout_min = data->vout_min[attr->index];
    if ((vout > vout_max) || (vout < vout_min)) {
        WB_ISL68137_ERROR("%d-%04x: vout value: %d, out of range [%d, %d] \n", client->adapter->nr,
            client->addr, vout, vout_min, vout_max);
        return -EINVAL;
    }

    /* calc VOUT_COMMAND set value */
    vout_cmd_set = vout / 1000;
    if (vout_cmd_set > 0xffff) {
        WB_ISL68137_ERROR("%d-%04x: invalid value, vout %d, vout_cmd_set: 0x%x\n",
            client->adapter->nr, client->addr, vout, vout_cmd_set);
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);

    /* close write protect */
    ret = wb_pmbus_write_byte_data(client, attr->index, PMBUS_WRITE_PROTECT, WRITE_PROTECT_CLOSE);
    if (ret < 0) {
        WB_ISL68137_ERROR("%d-%04x: close page%d write protect failed, ret: %d\n", client->adapter->nr,
            client->addr, attr->index, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    /* set VOUT_COMMAND */
    ret = wb_pmbus_write_word_data(client, attr->index, PMBUS_VOUT_COMMAND, vout_cmd_set);
    if (ret < 0) {
        WB_ISL68137_ERROR("%d-%04x: set page%d vout cmd reg: 0x%x, value: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd_set, ret);
        goto error;
    }

    /* read back VOUT_COMMAND */
    vout_cmd = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (vout_cmd < 0) {
        ret = vout_cmd;
        WB_ISL68137_ERROR("%d-%04x: read page%d vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, ret);
        goto error;
    }

    /* compare vout_cmd and vout_cmd_set */
    if (vout_cmd != vout_cmd_set) {
        ret = -EIO;
        WB_ISL68137_ERROR("%d-%04x: vout cmd value check error, vout cmd read: 0x%x, vout cmd set: 0x%x\n",
            client->adapter->nr, client->addr, vout_cmd, vout_cmd_set);
        goto error;
    }

    /* open write protect */
    wb_pmbus_write_byte_data(client, attr->index, PMBUS_WRITE_PROTECT, WRITE_PROTECT_OPEN);
    mutex_unlock(&data->update_lock);
    WB_ISL68137_VERBOSE("%d-%04x: set page%d vout cmd success, vout %d, vout_cmd_set: 0x%x\n",
        client->adapter->nr, client->addr, attr->index, vout, vout_cmd_set);
    return count;
error:
    wb_pmbus_write_byte_data(client, attr->index, PMBUS_WRITE_PROTECT, WRITE_PROTECT_OPEN);
    mutex_unlock(&data->update_lock);
    return ret;
}

static ssize_t isl68137_avs_vout_max_store(struct device *dev,
                   struct device_attribute *devattr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int ret, vout_threshold;

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        WB_ISL68137_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    ret = kstrtoint(buf, 0, &vout_threshold);
    if (ret) {
        WB_ISL68137_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    WB_ISL68137_VERBOSE("%d-%04x: vout%d max threshold: %d", client->adapter->nr, client->addr,
        attr->index, vout_threshold);

    data->vout_max[attr->index] = vout_threshold;
    return count;
}

static ssize_t isl68137_avs_vout_max_show(struct device *dev,
                   struct device_attribute *devattr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        WB_ISL68137_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    return snprintf(buf, PAGE_SIZE, "%d\n", data->vout_max[attr->index]);
}

static ssize_t isl68137_avs_vout_min_store(struct device *dev,
                   struct device_attribute *devattr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int ret, vout_threshold;

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        WB_ISL68137_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    ret = kstrtoint(buf, 0, &vout_threshold);
    if (ret) {
        WB_ISL68137_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    WB_ISL68137_VERBOSE("%d-%04x: vout%d min threshold: %d", client->adapter->nr, client->addr,
        attr->index, vout_threshold);

    data->vout_min[attr->index] = vout_threshold;
    return count;
}

static ssize_t isl68137_avs_vout_min_show(struct device *dev,
                   struct device_attribute *devattr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        WB_ISL68137_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    return snprintf(buf, PAGE_SIZE, "%d\n", data->vout_min[attr->index]);
}

static SENSOR_DEVICE_ATTR_RW(avs0_enable, isl68137_avs_enable, 0);
static SENSOR_DEVICE_ATTR_RW(avs1_enable, isl68137_avs_enable, 1);

static SENSOR_DEVICE_ATTR_RW(avs0_vout, isl68137_avs_vout, 0);
static SENSOR_DEVICE_ATTR_RW(avs1_vout, isl68137_avs_vout, 1);
static SENSOR_DEVICE_ATTR_RW(avs0_vout_max, isl68137_avs_vout_max, 0);
static SENSOR_DEVICE_ATTR_RW(avs0_vout_min, isl68137_avs_vout_min, 0);
static SENSOR_DEVICE_ATTR_RW(avs1_vout_max, isl68137_avs_vout_max, 1);
static SENSOR_DEVICE_ATTR_RW(avs1_vout_min, isl68137_avs_vout_min, 1);

static struct attribute *enable_attrs[] = {
    &sensor_dev_attr_avs0_enable.dev_attr.attr,
    &sensor_dev_attr_avs1_enable.dev_attr.attr,
    NULL,
};

static struct attribute *avs_ctrl_attrs[] = {
    &sensor_dev_attr_avs0_vout.dev_attr.attr,
    &sensor_dev_attr_avs1_vout.dev_attr.attr,
    &sensor_dev_attr_avs0_vout_max.dev_attr.attr,
    &sensor_dev_attr_avs0_vout_min.dev_attr.attr,
    &sensor_dev_attr_avs1_vout_max.dev_attr.attr,
    &sensor_dev_attr_avs1_vout_min.dev_attr.attr,
    NULL,
};

static const struct attribute_group enable_group = {
    .attrs = enable_attrs,
};

static const struct attribute_group avs_ctrl_group = {
    .attrs = avs_ctrl_attrs,
};

static const struct attribute_group *isl68137_attribute_groups[] = {
    &enable_group,
    &avs_ctrl_group,
    NULL,
};

static int raa_dmpvr2_read_word_data(struct i2c_client *client, int page,
                     int phase, int reg)
{
    int ret;

    switch (reg) {
    case PMBUS_VIRT_READ_VMON:
        ret = wb_pmbus_read_word_data(client, page, phase,
                       RAA_DMPVR2_READ_VMON);
        break;
    default:
        ret = -ENODATA;
        break;
    }

    return ret;
}

static struct pmbus_driver_info raa_dmpvr_info = {
    .pages = 3,
    .format[PSC_VOLTAGE_IN] = direct,
    .format[PSC_VOLTAGE_OUT] = direct,
    .format[PSC_CURRENT_IN] = direct,
    .format[PSC_CURRENT_OUT] = direct,
    .format[PSC_POWER] = direct,
    .format[PSC_TEMPERATURE] = direct,
    .m[PSC_VOLTAGE_IN] = 1,
    .b[PSC_VOLTAGE_IN] = 0,
    .R[PSC_VOLTAGE_IN] = 2,
    .m[PSC_VOLTAGE_OUT] = 1,
    .b[PSC_VOLTAGE_OUT] = 0,
    .R[PSC_VOLTAGE_OUT] = 3,
    .m[PSC_CURRENT_IN] = 1,
    .b[PSC_CURRENT_IN] = 0,
    .R[PSC_CURRENT_IN] = 2,
    .m[PSC_CURRENT_OUT] = 1,
    .b[PSC_CURRENT_OUT] = 0,
    .R[PSC_CURRENT_OUT] = 1,
    .m[PSC_POWER] = 1,
    .b[PSC_POWER] = 0,
    .R[PSC_POWER] = 0,
    .m[PSC_TEMPERATURE] = 1,
    .b[PSC_TEMPERATURE] = 0,
    .R[PSC_TEMPERATURE] = 0,
    .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_IIN | PMBUS_HAVE_PIN
        | PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2
        | PMBUS_HAVE_TEMP3 | PMBUS_HAVE_STATUS_TEMP
        | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
        | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_POUT
        | PMBUS_HAVE_VMON,
    .func[1] = PMBUS_HAVE_IIN | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT
        | PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP3 | PMBUS_HAVE_STATUS_TEMP
        | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT | PMBUS_HAVE_IOUT
        | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_POUT,
    .func[2] = PMBUS_HAVE_IIN | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT
        | PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP3 | PMBUS_HAVE_STATUS_TEMP
        | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT | PMBUS_HAVE_IOUT
        | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_POUT,
};

static int isl68137_probe(struct i2c_client *client)
{
    struct pmbus_driver_info *info;

    info = devm_kzalloc(&client->dev, sizeof(*info), GFP_KERNEL);
    if (!info)
        return -ENOMEM;
    memcpy(info, &raa_dmpvr_info, sizeof(*info));

    switch (i2c_match_id(raa_dmpvr_id, client)->driver_data) {
    case raa_dmpvr1_2rail:
        info->pages = 2;
        info->R[PSC_VOLTAGE_IN] = 3;
        info->func[0] &= ~PMBUS_HAVE_VMON;
        info->func[1] = PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
            | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT
            | PMBUS_HAVE_POUT;
        info->groups = isl68137_attribute_groups;
        break;
    case raa_dmpvr2_1rail:
        info->pages = 1;
        info->read_word_data = raa_dmpvr2_read_word_data;
        break;
    case raa_dmpvr2_2rail_nontc:
        info->func[0] &= ~PMBUS_HAVE_TEMP3;
        info->func[1] &= ~PMBUS_HAVE_TEMP3;
        fallthrough;
    case raa_dmpvr2_2rail:
        info->pages = 2;
        info->read_word_data = raa_dmpvr2_read_word_data;
        break;
    case raa_dmpvr2_3rail:
        info->read_word_data = raa_dmpvr2_read_word_data;
        break;
    case raa_dmpvr2_hv:
        info->pages = 1;
        info->R[PSC_VOLTAGE_IN] = 1;
        info->m[PSC_VOLTAGE_OUT] = 2;
        info->R[PSC_VOLTAGE_OUT] = 2;
        info->m[PSC_CURRENT_IN] = 2;
        info->m[PSC_POWER] = 2;
        info->R[PSC_POWER] = -1;
        info->read_word_data = raa_dmpvr2_read_word_data;
        break;
    default:
        return -ENODEV;
    }

    return wb_pmbus_do_probe(client, info);
}

static const struct i2c_device_id raa_dmpvr_id[] = {
    {"wb_isl68127", raa_dmpvr1_2rail},
    {"wb_isl68137", raa_dmpvr1_2rail},
    {"wb_isl68220", raa_dmpvr2_2rail},
    {"wb_isl68221", raa_dmpvr2_3rail},
    {"wb_isl68222", raa_dmpvr2_2rail},
    {"wb_isl68223", raa_dmpvr2_2rail},
    {"wb_isl68224", raa_dmpvr2_3rail},
    {"wb_isl68225", raa_dmpvr2_2rail},
    {"wb_isl68226", raa_dmpvr2_3rail},
    {"wb_isl68227", raa_dmpvr2_1rail},
    {"wb_isl68229", raa_dmpvr2_3rail},
    {"wb_isl68233", raa_dmpvr2_2rail},
    {"wb_isl68239", raa_dmpvr2_3rail},

    {"wb_isl69222", raa_dmpvr2_2rail},
    {"wb_isl69223", raa_dmpvr2_3rail},
    {"wb_isl69224", raa_dmpvr2_2rail},
    {"wb_isl69225", raa_dmpvr2_2rail},
    {"wb_isl69227", raa_dmpvr2_3rail},
    {"wb_isl69228", raa_dmpvr2_3rail},
    {"wb_isl69234", raa_dmpvr2_2rail},
    {"wb_isl69236", raa_dmpvr2_2rail},
    {"wb_isl69239", raa_dmpvr2_3rail},
    {"wb_isl69242", raa_dmpvr2_2rail},
    {"wb_isl69243", raa_dmpvr2_1rail},
    {"wb_isl69247", raa_dmpvr2_2rail},
    {"wb_isl69248", raa_dmpvr2_2rail},
    {"wb_isl69254", raa_dmpvr2_2rail},
    {"wb_isl69255", raa_dmpvr2_2rail},
    {"wb_isl69256", raa_dmpvr2_2rail},
    {"wb_isl69259", raa_dmpvr2_2rail},
    {"wb_isl69260", raa_dmpvr2_2rail},
    {"wb_isl69268", raa_dmpvr2_2rail},
    {"wb_isl69269", raa_dmpvr2_3rail},
    {"wb_isl69298", raa_dmpvr2_2rail},

    {"wb_raa228000", raa_dmpvr2_hv},
    {"wb_raa228004", raa_dmpvr2_hv},
    {"wb_raa228006", raa_dmpvr2_hv},
    {"wb_raa228228", raa_dmpvr2_2rail_nontc},
    {"wb_raa229001", raa_dmpvr2_2rail},
    {"wb_raa229004", raa_dmpvr2_2rail},
    {}
};

MODULE_DEVICE_TABLE(i2c, raa_dmpvr_id);

/* This is the driver that will be inserted */
static struct i2c_driver isl68137_driver = {
    .driver = {
           .name = "wb_isl68137",
           },
    .probe_new = isl68137_probe,
    .remove = wb_pmbus_do_remove,
    .id_table = raa_dmpvr_id,
};

module_i2c_driver(isl68137_driver);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("PMBus driver for Renesas digital multiphase voltage regulators");
MODULE_LICENSE("GPL");
