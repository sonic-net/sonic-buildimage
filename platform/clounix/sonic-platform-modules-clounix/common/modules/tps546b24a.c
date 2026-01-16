// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Hardware monitoring driver for MPS Multi-phase Digital VR Controllers
 *
 * Copyright (C) 2020 Nvidia Technologies Ltd.
 */

#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pmbus.h>
#include <linux/sysfs.h>
#include <linux/stat.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include "pmbus.h"

struct tps_data {
    struct pmbus_driver_info info;
    struct pmbus_sensor *sensor;
    int total_curr_resolution;
    int phase_curr_resolution;
    int curr_sense_gain;
};

static struct pmbus_platform_data tps_pdata = {0};
static struct pmbus_driver_info tps_info = {0};

ssize_t mfr_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char tmp_buf[I2C_SMBUS_BLOCK_MAX+2] = {0};
    int len;
    struct sensor_device_attribute *s_attr = to_sensor_dev_attr(attr);
    struct i2c_client *client = to_i2c_client(dev->parent);
    
    len = i2c_smbus_read_block_data(client, s_attr->index, tmp_buf);

    if (len > 0)
        return sprintf(buf, "%s\n", tmp_buf);
    
    return 0;
}

SENSOR_DEVICE_ATTR(revision, S_IRUGO, mfr_info_show, NULL, PMBUS_REVISION);
SENSOR_DEVICE_ATTR(mfr_id, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_ID);
SENSOR_DEVICE_ATTR(mfr_model, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_MODEL);
SENSOR_DEVICE_ATTR(mfr_revision, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_REVISION);
SENSOR_DEVICE_ATTR(mfr_serial, S_IRUGO, mfr_info_show, NULL, PMBUS_MFR_SERIAL);

static struct attribute *tps_psu_attrs[] = {
    &sensor_dev_attr_revision.dev_attr.attr,
    &sensor_dev_attr_mfr_id.dev_attr.attr,
    &sensor_dev_attr_mfr_model.dev_attr.attr,
    &sensor_dev_attr_mfr_revision.dev_attr.attr,
    &sensor_dev_attr_mfr_serial.dev_attr.attr,
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = tps_psu_attrs,
};

static const struct attribute_group *attr_groups[] = {
    &attr_group,
    NULL,
};

static int priv_read_byte(struct i2c_client *client, int page, int reg)
{
    int rv = 0;
    switch(reg) {
        case PMBUS_VOUT_MODE:
            rv = pmbus_read_byte_data(client, page, reg);
            return (rv&0x7F);

        default:
            break;
    }

    return -ENODATA;
}
static int tps_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    struct pmbus_driver_info *info;
    struct tps_data *data;

    data = devm_kzalloc(&client->dev, sizeof(struct tps_data), GFP_KERNEL);
    if (!data)
        return -ENOMEM;

    client->dev.platform_data = &tps_pdata;
    tps_pdata.flags = PMBUS_SKIP_STATUS_CHECK;

    memcpy(&data->info, &tps_info, sizeof(*info));
    info = &data->info;

    /* start init param */
    info->pages = 1;

    info->func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_IOUT | PMBUS_HAVE_TEMP |
                    PMBUS_HAVE_STATUS_VOUT | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_STATUS_TEMP;
    
    info->read_byte_data = priv_read_byte;

    info->groups = attr_groups;
    return pmbus_do_probe(client, info);
}
static const struct i2c_device_id tps_id[] = {
    {"tps546b24a", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, tps_id);

static const struct of_device_id __maybe_unused tps_of_match[] = {
    {.compatible = "TI,tps546b24a"},
    {}
};
MODULE_DEVICE_TABLE(of, tps_of_match);

static struct i2c_driver tps_driver = {
    .driver = {
        .name = "TI tps",
        .of_match_table = of_match_ptr(tps_of_match),
    },
    .probe = tps_probe,
    .id_table = tps_id,
};

module_i2c_driver(tps_driver);

MODULE_AUTHOR("baohx@clounix.com");
MODULE_DESCRIPTION("PMBus driver for TPS546B24A");
MODULE_LICENSE("GPL");
