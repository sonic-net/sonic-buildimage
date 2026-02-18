// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Hardware monitoring driver for Infineon Multi-phase Digital VR Controllers
 *
 * Copyright (c) 2020 Mellanox Technologies. All rights reserved.
 */

#include <linux/err.h>
#include <linux/hwmon-sysfs.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include "wb_pmbus.h"
#include <wb_bsp_kernel_debug.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

#define PROT_VR12_5MV               (0x01) /* VR12.0 mode, 5-mV DAC */
#define PROT_VR12_5_10MV            (0x02) /* VR12.5 mode, 10-mV DAC */
#define PROT_IMVP9_10MV             (0x03) /* IMVP9 mode, 10-mV DAC */
#define AMD_625MV                   (0x10) /* AMD mode 6.25mV */

#define JWH63_PAGE_NUM              (2)

#define IOUT_REG_TO_VALUE(reg_val, reg_exp)   \
    ((((reg_val) & 0x3ff)) * (1000 >> (((reg_exp) >= 0x2) ? 0x2 : (reg_exp))))

#define VOUT_REG_TO_VALUE(reg_val)   (((reg_val) & 0x3ff) * 10 / 4)

/* LSB: 1C */
#define TEMP_REG_TO_VALUE(reg_val)   ((((s16)(((reg_val) & 0x3ff) << 4)) >> 4) * 1000)
/*
 * Select PIN and rail A POUT report resolution.
 * 2'b00: 2W/LSB
 * 2'b01: 1W/LSB
 * 2'b10: 0.5W/LSB
 * 2’b11: 0.25W/LSB
 */
#define POUT_REG_TO_VALUE(reg_val, reg_exp)   (((reg_val) & 0x3ff) * (2000 >> (reg_exp)))
/* 1.25A/LSB */
#define PHASE_REG_TO_VALUE(reg_val) (((reg_val) & 0xff) *1250)

/* PAGE */
#define JWH63_LOOP_A               (0)
#define JWH63_LOOP_B               (1)
#define JWH63_CONFIG               (2)

#define JWH63_PHASE_CHANGE_REG     (0x3)
#define JWH63_PHASE_VALUE_REG      (0x10)

#define JWH63_PRODUCT_REV_USER_REG        (0xb3)
#define JWH63_READ_CS_1_2_REG             (0x7f)
#define JWH63_READ_CS_3_4_REG             (0x80)
#define JWH63_READ_CS_5_6_REG             (0x81)
#define JWH63_READ_CS_7_8_REG             (0x82)
#define JWH63_READ_CS_1_2_RB_REG          (0x84)
#define JWH63_READ_CS_3_4_RB_REG          (0x83)

/* PAGE 2 */
#define JWH63_MFR_VR_CONFIG_RA            (0x7)
#define JWH63_MFR_VR_CONFIG_RB            (0x8)
#define VID_STEP_SEL_OFFSET               (4)


static int jwh63_data2reg_vid(struct pmbus_data *data, int page, int val)
{
    int vrm_version;

    vrm_version = data->info->vrm_version[page];
    DEBUG_INFO("page%d, vrm_version: %d, data_val: %d\n", page, vrm_version, val);
    /* Convert data to VID register. */
    switch (vrm_version) {
    case vr13:
        if (val >= 500) {
            return 1 + DIV_ROUND_CLOSEST(val - 500, 10);
        }
        return 0;
    case vr12:
        if (val >= 250) {
            return 1 + DIV_ROUND_CLOSEST(val - 250, 5);
        }
        return 0;
    case imvp9:
        if (val >= 200) {
            return 1 + DIV_ROUND_CLOSEST(val - 200, 10);
        }
        return 0;
    case amd625mv:
        if (val >= 200 && val <= 1550) {
            return DIV_ROUND_CLOSEST((1550 - val) * 100, 625);
        }
        return 0;
    default:
        DEBUG_ERROR("Unsupport vrm_version, page%d, vrm_version: %d\n",
            page, vrm_version);
        return -EINVAL;
    }
    return 0;
}

/*
 * Convert VID sensor values to milli- or micro-units
 * depending on sensor type.
 */
static int jwh63_reg2data_vid(struct pmbus_data *data, int page, int val)
{
    long rv;
    int vrm_version;

    rv = 0;
    vrm_version = data->info->vrm_version[page];
    switch (vrm_version) {
    case vr11:
        if (val >= 0x02 && val <= 0xb2)
            rv = DIV_ROUND_CLOSEST(160000 - (val - 2) * 625, 100);
        break;
    case vr12:
        if (val >= 0x01)
            rv = 250 + (val - 1) * 5;
        break;
    case vr13:
        if (val >= 0x01)
            rv = 500 + (val - 1) * 10;
        break;
    case imvp9:
        if (val >= 0x01)
            rv = 200 + (val - 1) * 10;
        break;
    case amd625mv:
        if (val >= 0x0 && val <= 0xd8)
            rv = DIV_ROUND_CLOSEST(155000 - val * 625, 100);
        break;
    }
    DEBUG_INFO("page%d, vrm_version: %d, reg_val: 0x%x, data_val: %ld\n",
        page, vrm_version, val, rv);
    return rv;
}

static ssize_t jwh63_avs_vout_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int vout_cmd, vout, vout_mv;

    mutex_lock(&data->update_lock);
    vout_cmd = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (vout_cmd < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd);
        mutex_unlock(&data->update_lock);
        return vout_cmd;
    }

    vout_mv = jwh63_reg2data_vid(data, attr->index, vout_cmd);
    vout = vout_mv * 1000;  /* trans mv to uv */
    DEBUG_INFO("%d-%04x: page%d, vout command reg_val: 0x%x, vout: %d uV\n",
        client->adapter->nr, client->addr, attr->index, vout_cmd, vout);

    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%d\n", vout);
}

static ssize_t jwh63_avs_vout_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int vout, vout_max, vout_min, vout_mv;
    int ret, vout_cmd, vout_cmd_set;

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        DEBUG_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    vout = 0;
    ret = kstrtoint(buf, 0, &vout);
    if (ret) {
        DEBUG_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    DEBUG_INFO("%d-%04x: vout = %d\n", client->adapter->nr, client->addr, vout);

    if (vout <= 0) {
        DEBUG_ERROR("%d-%04x: invalid value: %d \n", client->adapter->nr, client->addr, vout);
        return -EINVAL;
    }

    vout_max = data->vout_max[attr->index];
    vout_min = data->vout_min[attr->index];
    if ((vout > vout_max) || (vout < vout_min)) {
        DEBUG_ERROR("%d-%04x: vout value: %d, out of range [%d, %d] \n", client->adapter->nr,
            client->addr, vout, vout_min, vout_max);
        return -EINVAL;
    }

    /* calc VOUT_COMMAND set value Unit is mV*/
    vout_mv = vout / 1000;
    vout_cmd_set = jwh63_data2reg_vid(data, attr->index, vout_mv);
    if ((vout_cmd_set < 0) || (vout_cmd_set > 0xffff)) {
        DEBUG_ERROR("%d-%04x: invalid value, vout %d uV, vout_cmd_set: %d\n",
            client->adapter->nr, client->addr, vout, vout_cmd_set);
        return -EINVAL;
    }
    DEBUG_INFO("%d-%04x: vout %d uV, vout_cmd_set: %d\n",
            client->adapter->nr, client->addr, vout, vout_cmd_set);

    mutex_lock(&data->update_lock);

    /* set VOUT_COMMAND */
    ret = wb_pmbus_write_word_data(client, attr->index, PMBUS_VOUT_COMMAND, vout_cmd_set);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set page%d vout cmd reg: 0x%x, value: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd_set, ret);
        goto error;
    }

    /* read back VOUT_COMMAND */
    vout_cmd = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (vout_cmd < 0) {
        ret = vout_cmd;
        DEBUG_ERROR("%d-%04x: read page%d vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, ret);
        goto error;
    }

    /* compare vout_cmd and vout_cmd_set */
    if (vout_cmd != vout_cmd_set) {
        ret = -EIO;
        DEBUG_ERROR("%d-%04x: vout cmd value check error, vout cmd read: 0x%x, vout cmd set: 0x%x\n",
            client->adapter->nr, client->addr, vout_cmd, vout_cmd_set);
        goto error;
    }

    mutex_unlock(&data->update_lock);
    DEBUG_INFO("%d-%04x: set page%d vout cmd success, vout %d uV, vout_cmd_set: 0x%x\n",
        client->adapter->nr, client->addr, attr->index, vout, vout_cmd_set);
    return count;
error:
    mutex_unlock(&data->update_lock);
    return ret;
}

static ssize_t jwh63_avs_vout_max_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int ret, vout_threshold;

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        DEBUG_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    vout_threshold = 0;
    ret = kstrtoint(buf, 0, &vout_threshold);
    if (ret) {
        DEBUG_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    DEBUG_INFO("%d-%04x: vout%d max threshold: %d", client->adapter->nr, client->addr,
        attr->index, vout_threshold);

    data->vout_max[attr->index] = vout_threshold;
    return count;
}

static ssize_t jwh63_avs_vout_max_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        DEBUG_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    return snprintf(buf, PAGE_SIZE, "%d\n", data->vout_max[attr->index]);
}

static ssize_t jwh63_avs_vout_min_store(struct device *dev, struct device_attribute *devattr,
                const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int ret, vout_threshold;

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        DEBUG_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    vout_threshold = 0;
    ret = kstrtoint(buf, 0, &vout_threshold);
    if (ret) {
        DEBUG_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    DEBUG_INFO("%d-%04x: vout%d min threshold: %d", client->adapter->nr, client->addr,
        attr->index, vout_threshold);

    data->vout_min[attr->index] = vout_threshold;
    return count;
}

static ssize_t jwh63_avs_vout_min_show(struct device *dev, struct device_attribute *devattr,
                char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        DEBUG_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    return snprintf(buf, PAGE_SIZE, "%d\n", data->vout_min[attr->index]);
}

static ssize_t jwh63_iout_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int reg_val, reg_exp;
    s64 val;

    mutex_lock(&data->update_lock);
    reg_val = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_READ_IOUT);
    if (reg_val < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, iout reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_READ_IOUT, reg_val);
        mutex_unlock(&data->update_lock);
        return reg_val;
    }

    reg_exp = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_MFR_VIN_MAX);
    if (reg_exp < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, mfr_reso_set reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_MFR_VIN_MAX, reg_val);
        mutex_unlock(&data->update_lock);
        return reg_exp;
    }

    /* Exponent bits value decided by A1h bit[3:2] */
    reg_exp = (reg_exp & 0xc) >> 2;
    val = IOUT_REG_TO_VALUE(reg_val, reg_exp);

    DEBUG_INFO("%d-%04x: page%d, iout_reg_val: 0x%x, iout: %lld mA\n",
        client->adapter->nr, client->addr, attr->index, reg_val, val);

    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%lld\n", val);
}

static ssize_t jwh63_vout_show(struct device *dev, struct device_attribute *devattr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int reg_val;
    s64 val;

    mutex_lock(&data->update_lock);
    reg_val = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_READ_VOUT);
    if (reg_val < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, vout reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_READ_VOUT, reg_val);
        mutex_unlock(&data->update_lock);
        return reg_val;
    }

    val = VOUT_REG_TO_VALUE(reg_val);

    DEBUG_INFO("%d-%04x: page%d, vout_reg_val: 0x%x, vout: %lld mV\n",
        client->adapter->nr, client->addr, attr->index, reg_val, val);

    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%lld\n", val);
}

static ssize_t jwh63_temp_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int reg_val;
    s64 val;

    mutex_lock(&data->update_lock);
    reg_val = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_READ_TEMPERATURE_1);
    if (reg_val < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, temp reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_READ_TEMPERATURE_1, reg_val);
        mutex_unlock(&data->update_lock);
        return reg_val;
    }

    val = TEMP_REG_TO_VALUE(reg_val);

    DEBUG_INFO("%d-%04x: page%d, temp_reg_val: 0x%x, temp: %lld mC\n",
        client->adapter->nr, client->addr, attr->index, reg_val, val);

    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%lld\n", val);
}

static ssize_t jwh63_pout_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int reg_val, reg_exp;
    s64 val;

    mutex_lock(&data->update_lock);
    reg_val = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_READ_POUT);
    if (reg_val < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, pout reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_READ_POUT, reg_val);
        mutex_unlock(&data->update_lock);
        return reg_val;
    }

    reg_exp = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_MFR_VIN_MAX);
    if (reg_exp < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, mfr_reso_set reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_MFR_VIN_MAX, reg_val);
        mutex_unlock(&data->update_lock);
        return reg_exp;
    }

    reg_exp &= 0x3; /* Exponent bits value decided by A1h bit[1:0] */
    val = POUT_REG_TO_VALUE(reg_val, reg_exp);

    DEBUG_INFO("%d-%04x: page%d, pout_reg_val: 0x%x, pout: %lld uW\n",
        client->adapter->nr, client->addr, attr->index, reg_val, val);

    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%lld\n", val);
}

static ssize_t jwh63_word_data_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    u8 page = to_sensor_dev_attr_2(devattr)->nr;
    u8 reg = to_sensor_dev_attr_2(devattr)->index;
    struct pmbus_data *data = i2c_get_clientdata(client);
    int word_data;

    mutex_lock(&data->update_lock);
    word_data = wb_pmbus_read_word_data(client, page, 0xff, reg);
    if (word_data < 0) {
        DEBUG_ERROR("%d-%04x: read page%u, command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, page, reg, word_data);
        mutex_unlock(&data->update_lock);
        return word_data;
    }

    DEBUG_INFO("%d-%04x: read word data success, page%u, reg: 0x%x, value: 0x%04x\n",
        client->adapter->nr, client->addr, page, reg, word_data);
    mutex_unlock(&data->update_lock);

    return snprintf(buf, PAGE_SIZE, "0x%04x\n", word_data);
}

static ssize_t jwh63_phase_curr(struct device *dev, struct device_attribute *devattr,
                   char *buf, int mask, int offset)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    u8 page = to_sensor_dev_attr_2(devattr)->nr;
    u8 reg = to_sensor_dev_attr_2(devattr)->index;
    struct pmbus_data *data = i2c_get_clientdata(client);
    int word_data, value;

    mutex_lock(&data->update_lock);
    word_data = wb_pmbus_read_word_data(client, page, 0xff, reg);
    if (word_data < 0) {
        DEBUG_ERROR("%d-%04x: read page%u, vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, page, reg, word_data);
        mutex_unlock(&data->update_lock);
        return word_data;
    }

    DEBUG_INFO("word_data: 0x%x, mask: 0x%x, offset: %d.\r\n", word_data, mask, offset);
    value = PHASE_REG_TO_VALUE((word_data & mask) >> offset);
    DEBUG_INFO("%d-%04x: read word data success, page%u, reg: 0x%x, value: 0x%04x\n",
        client->adapter->nr, client->addr, page, reg, value);

    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%04d\n", value);
}

static ssize_t jwh63_loopx_phase1_curr_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    /* read bit 0 -- bit 7 */
    return jwh63_phase_curr(dev, devattr, buf, 0xff, 0);
}

static ssize_t jwh63_loopx_phase2_curr_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    /* read bit 8 -- bit 15 */
    return jwh63_phase_curr(dev, devattr, buf, 0xff00, 8);
}

/* param _index is page */
static SENSOR_DEVICE_ATTR_RW(avs0_vout, jwh63_avs_vout, JWH63_LOOP_A);
static SENSOR_DEVICE_ATTR_RW(avs1_vout, jwh63_avs_vout, JWH63_LOOP_B);
static SENSOR_DEVICE_ATTR_RW(avs0_vout_max, jwh63_avs_vout_max, JWH63_LOOP_A);
static SENSOR_DEVICE_ATTR_RW(avs0_vout_min, jwh63_avs_vout_min, JWH63_LOOP_A);
static SENSOR_DEVICE_ATTR_RW(avs1_vout_max, jwh63_avs_vout_max, JWH63_LOOP_B);
static SENSOR_DEVICE_ATTR_RW(avs1_vout_min, jwh63_avs_vout_min, JWH63_LOOP_B);
static SENSOR_DEVICE_ATTR_RO(loopa_iout, jwh63_iout, JWH63_LOOP_A);
static SENSOR_DEVICE_ATTR_RO(loopa_vout, jwh63_vout, JWH63_LOOP_A);
static SENSOR_DEVICE_ATTR_RO(loopa_temp, jwh63_temp, JWH63_LOOP_A);
static SENSOR_DEVICE_ATTR_RO(loopa_pout, jwh63_pout, JWH63_LOOP_A);
static SENSOR_DEVICE_ATTR_RO(loopb_iout, jwh63_iout, JWH63_LOOP_B);
static SENSOR_DEVICE_ATTR_RO(loopb_vout, jwh63_vout, JWH63_LOOP_B);
static SENSOR_DEVICE_ATTR_RO(loopb_temp, jwh63_temp, JWH63_LOOP_B);
static SENSOR_DEVICE_ATTR_RO(loopb_pout, jwh63_pout, JWH63_LOOP_B);

/* param _nr is page , param _index is reg addr */
static SENSOR_DEVICE_ATTR_2_RO(loopa_phase1_curr, jwh63_loopx_phase1_curr, JWH63_LOOP_A,
        JWH63_READ_CS_1_2_REG);
static SENSOR_DEVICE_ATTR_2_RO(loopa_phase2_curr, jwh63_loopx_phase2_curr, JWH63_LOOP_A,
        JWH63_READ_CS_1_2_REG);
static SENSOR_DEVICE_ATTR_2_RO(loopa_phase3_curr, jwh63_loopx_phase1_curr, JWH63_LOOP_A,
        JWH63_READ_CS_3_4_REG);
static SENSOR_DEVICE_ATTR_2_RO(loopa_phase4_curr, jwh63_loopx_phase2_curr, JWH63_LOOP_A,
        JWH63_READ_CS_3_4_REG);
static SENSOR_DEVICE_ATTR_2_RO(loopa_phase5_curr, jwh63_loopx_phase1_curr, JWH63_LOOP_A,
        JWH63_READ_CS_5_6_REG);
static SENSOR_DEVICE_ATTR_2_RO(loopa_phase6_curr, jwh63_loopx_phase2_curr, JWH63_LOOP_A,
        JWH63_READ_CS_5_6_REG);
static SENSOR_DEVICE_ATTR_2_RO(loopa_phase7_curr, jwh63_loopx_phase1_curr, JWH63_LOOP_A,
        JWH63_READ_CS_7_8_REG);
static SENSOR_DEVICE_ATTR_2_RO(loopa_phase8_curr, jwh63_loopx_phase2_curr, JWH63_LOOP_A,
        JWH63_READ_CS_7_8_REG);
static SENSOR_DEVICE_ATTR_2_RO(loopb_phase4_curr, jwh63_loopx_phase1_curr, JWH63_LOOP_B,
        JWH63_READ_CS_3_4_RB_REG);
static SENSOR_DEVICE_ATTR_2_RO(loopb_phase3_curr, jwh63_loopx_phase2_curr, JWH63_LOOP_B,
        JWH63_READ_CS_3_4_RB_REG);
static SENSOR_DEVICE_ATTR_2_RO(loopb_phase2_curr, jwh63_loopx_phase1_curr, JWH63_LOOP_B,
        JWH63_READ_CS_1_2_RB_REG);
static SENSOR_DEVICE_ATTR_2_RO(loopb_phase1_curr, jwh63_loopx_phase2_curr, JWH63_LOOP_B,
        JWH63_READ_CS_1_2_RB_REG);

static struct attribute *avs_ctrl_attrs[] = {
    &sensor_dev_attr_avs0_vout.dev_attr.attr,
    &sensor_dev_attr_avs1_vout.dev_attr.attr,
    &sensor_dev_attr_avs0_vout_max.dev_attr.attr,
    &sensor_dev_attr_avs0_vout_min.dev_attr.attr,
    &sensor_dev_attr_avs1_vout_max.dev_attr.attr,
    &sensor_dev_attr_avs1_vout_min.dev_attr.attr,
    NULL,
};

static const struct attribute_group avs_ctrl_group = {
    .attrs = avs_ctrl_attrs,
};

static struct attribute *loop_status_attrs[] = {
    &sensor_dev_attr_loopa_iout.dev_attr.attr,
    &sensor_dev_attr_loopa_vout.dev_attr.attr,
    &sensor_dev_attr_loopa_temp.dev_attr.attr,
    &sensor_dev_attr_loopa_pout.dev_attr.attr,
    &sensor_dev_attr_loopb_iout.dev_attr.attr,
    &sensor_dev_attr_loopb_vout.dev_attr.attr,
    &sensor_dev_attr_loopb_temp.dev_attr.attr,
    &sensor_dev_attr_loopb_pout.dev_attr.attr,
    NULL,
};

static const struct attribute_group loop_status_group = {
    .attrs = loop_status_attrs,
};

static struct attribute *phase_curr_attrs[] = {
    &sensor_dev_attr_loopa_phase1_curr.dev_attr.attr,
    &sensor_dev_attr_loopa_phase2_curr.dev_attr.attr,
    &sensor_dev_attr_loopa_phase3_curr.dev_attr.attr,
    &sensor_dev_attr_loopa_phase4_curr.dev_attr.attr,
    &sensor_dev_attr_loopa_phase5_curr.dev_attr.attr,
    &sensor_dev_attr_loopa_phase6_curr.dev_attr.attr,
    &sensor_dev_attr_loopa_phase7_curr.dev_attr.attr,
    &sensor_dev_attr_loopa_phase8_curr.dev_attr.attr,
    &sensor_dev_attr_loopb_phase1_curr.dev_attr.attr,
    &sensor_dev_attr_loopb_phase2_curr.dev_attr.attr,
    &sensor_dev_attr_loopb_phase3_curr.dev_attr.attr,
    &sensor_dev_attr_loopb_phase4_curr.dev_attr.attr,
    NULL,
};

static const struct attribute_group phase_curr_group = {
    .attrs = phase_curr_attrs,
};

static SENSOR_DEVICE_ATTR_2_RO(product_rev_user, jwh63_word_data, JWH63_LOOP_A,
        JWH63_PRODUCT_REV_USER_REG);

static struct attribute *jwh63_sysfs_attrs[] = {
    &sensor_dev_attr_product_rev_user.dev_attr.attr,
    NULL,
};

static const struct attribute_group jwh63_sysfs_group = {
    .attrs = jwh63_sysfs_attrs,
};

static const struct attribute_group *jwh63_attribute_groups[] = {
    &avs_ctrl_group,
    &loop_status_group,
    &phase_curr_group,
    NULL,
};

static int jwh63_identify(struct i2c_client *client, struct pmbus_driver_info *info)
{
    u8 vout_params;
    int i, ret;

    /* Read the register with VOUT scaling value. all page use same reg value */
    ret = wb_pmbus_read_byte_data(client, 0, PMBUS_VOUT_MODE);
    if (ret < 0) {
        return ret;
    }

    for (i = 0; i < JWH63_PAGE_NUM; i++) {
        vout_params = ret & GENMASK(4, 0);

        switch (vout_params) {
        case PROT_VR12_5_10MV:
            info->vrm_version[i] = vr13;
            break;
        case PROT_VR12_5MV:
            info->vrm_version[i] = vr12;
            break;
        case PROT_IMVP9_10MV:
            info->vrm_version[i] = imvp9;
            break;
        case AMD_625MV:
            info->vrm_version[i] = amd625mv;
            break;
        default:
            return -EINVAL;
        }
    }

    return 0;
}

static struct pmbus_driver_info jwh63_info = {
    .pages = JWH63_PAGE_NUM,
    .format[PSC_VOLTAGE_IN] = linear,
    .format[PSC_VOLTAGE_OUT] = vid,
    .format[PSC_TEMPERATURE] = linear,
    .format[PSC_CURRENT_IN] = linear,
    .format[PSC_CURRENT_OUT] = linear,
    .format[PSC_POWER] = linear,
    .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
        PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
        PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_TEMP |
        PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
    .func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
        PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
        PMBUS_HAVE_TEMP | PMBUS_HAVE_STATUS_TEMP |
        PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
    .groups = jwh63_attribute_groups,
    .identify = jwh63_identify,
};

static int jwh63_probe(struct i2c_client *client)
{
    struct pmbus_driver_info *info;
    int ret;

    info = devm_kmemdup(&client->dev, &jwh63_info, sizeof(*info), GFP_KERNEL);
    if (!info) {
        DEBUG_ERROR("no memory.\r\n");
        return -ENOMEM;
    }

    ret = wb_pmbus_do_probe(client, info);
    if (ret != 0) {
        dev_info(&client->dev, "wb_pmbus_do_probe failed, ret: %d.\n", ret);
        return ret;
    }

    ret = sysfs_create_group(&client->dev.kobj, &jwh63_sysfs_group);
    if (ret != 0) {
        dev_info(&client->dev, "Failed to create jwh63_sysfs_group, ret: %d.\n", ret);
        wb_pmbus_do_remove(client);
        return ret;
    }

    return 0;
}

static void jwh63_remove(struct i2c_client *client)
{
    sysfs_remove_group(&client->dev.kobj, &jwh63_sysfs_group);
    (void)wb_pmbus_do_remove(client);
    return;
}

static const struct i2c_device_id jwh63_id[] = {
    {"wb_jwh63", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, jwh63_id);

static const struct of_device_id __maybe_unused jwh63_of_match[] = {
    {.compatible = "JoulWatt,wb_jwh6384"},
    {}
};
MODULE_DEVICE_TABLE(of, jwh63_of_match);

static struct i2c_driver JWH63_driver = {
    .driver = {
        .name = "wb_jwh6384",
        .of_match_table = of_match_ptr(jwh63_of_match),
    },
    .probe_new = jwh63_probe,
    .remove = jwh63_remove,
    .id_table = jwh63_id,
};

module_i2c_driver(JWH63_driver);

MODULE_AUTHOR("Support");
MODULE_DESCRIPTION("PMBus driver for Infineon JWH63 family");
MODULE_LICENSE("GPL");
