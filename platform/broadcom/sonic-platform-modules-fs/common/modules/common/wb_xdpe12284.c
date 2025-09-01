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

#define XDPE122_PROT_VR12_5MV        (0x01) /* VR12.0 mode, 5-mV DAC */
#define XDPE122_PROT_VR12_5_10MV     (0x02) /* VR12.5 mode, 10-mV DAC */
#define XDPE122_PROT_IMVP9_10MV      (0x03) /* IMVP9 mode, 10-mV DAC */
#define XDPE122_AMD_625MV            (0x10) /* AMD mode 6.25mV */
#define XDPE122_PAGE_NUM             (2)
#define XDPE122_WRITE_PROTECT_CLOSE  (0x00)
#define XDPE122_WRITE_PROTECT_OPEN   (0x40)
#define XDPE122_LOOPA_STATUS_PAGE    (0x60)
#define XDPE122_LOOPB_STATUS_PAGE    (0x61)
#define XDPE122_CRC_PAGE             (0x62)
#define XDPE122_I2C_READ_IOUT        (0x0E)
#define XDPE122_I2C_READ_VOUT        (0x12)
#define XDPE122_I2C_READ_TEMP        (0x18)
#define XDPE122_I2C_READ_POUT        (0x1B)
#define IOUT_REG_TO_VALUE(reg_val)   ((((s16)(((reg_val) & 0x7fff) << 1)) >> 1) * 125 / 2) /* LSB: 62.5mA */
#define VOUT_REG_TO_VALUE(reg_val)   (((reg_val) & 0xfff) * 10 / 8)                        /* LSB: 1.25mV */
#define TEMP_REG_TO_VALUE(reg_val)   ((((s16)(((reg_val) & 0xfff) << 4)) >> 4) * 125)      /* LSB: 0.125C */
#define POUT_REG_TO_VALUE(reg_val)   (((reg_val) & 0x7fff) * 31250)                        /* LSB: 31.25mW */

#define XDPE122_VERSION_PAGE         (0x22)
#define XDPE122_VERSION1_REG         (0x3f)
#define XDPE122_VERSION2_REG         (0x40)

#define XDPE122_CRC_LO_REG           (0x42)
#define XDPE122_CRC_HI_REG           (0x43)

#define XDPE122_LOOP_A               (0)
#define XDPE122_LOOP_B               (1)

#define XDPE122_PHASE_CHANGE_REG     (0x3)
#define XDPE122_PHASE_VALUE_REG      (0x10)

#define XDPE122_PHASE_REG_DEFAULT_VALUE     (0x600)
#define XDPE122_PHASE1_REG_VALUE            (0x608)
#define XDPE122_PHASE2_REG_VALUE            (0x610)
#define XDPE122_PHASE3_REG_VALUE            (0x618)
#define XDPE122_PHASE4_REG_VALUE            (0x620)
#define XDPE122_PHASE5_REG_VALUE            (0x628)
#define XDPE122_PHASE6_REG_VALUE            (0x630)
#define XDPE122_PHASE7_REG_VALUE            (0x638)
#define XDPE122_PHASE8_REG_VALUE            (0x640)

#define PHASE_CURR_REG_TO_VALUE(reg_val)   ((((s16)(((reg_val) & 0xfff) << 4)) >> 4) * 125 / 2) /* LSB: 62.5mA */

static pmbus_info_t xdpe122_dfx_infos[] = {
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_BYTE", .pmbus_reg = PMBUS_STATUS_BYTE, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_WORD", .pmbus_reg = PMBUS_STATUS_WORD, .width = WORD_DATA, .data_type = RAWDATA_WORD},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_VOUT", .pmbus_reg = PMBUS_STATUS_VOUT, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_IOUT", .pmbus_reg = PMBUS_STATUS_IOUT, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_INPUT", .pmbus_reg = PMBUS_STATUS_INPUT, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_TEMP", .pmbus_reg = PMBUS_STATUS_TEMPERATURE, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_CML", .pmbus_reg = PMBUS_STATUS_CML, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "STATUS_MFR_SPEC", .pmbus_reg = PMBUS_STATUS_MFR_SPECIFIC, .width = BYTE_DATA, .data_type = RAWDATA_BYTE},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_VIN", .pmbus_reg = PMBUS_READ_VIN, .width = WORD_DATA, .data_type = SENSOR_DATA},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_IIN", .pmbus_reg = PMBUS_READ_IIN, .width = WORD_DATA, .data_type = SENSOR_DATA},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_VOUT", .pmbus_reg = PMBUS_READ_VOUT, .width = WORD_DATA, .data_type = SENSOR_DATA},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_IOUT", .pmbus_reg = PMBUS_READ_IOUT, .width = WORD_DATA, .data_type = SENSOR_DATA},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_TEMP1", .pmbus_reg = PMBUS_READ_TEMPERATURE_1, .width = WORD_DATA, .data_type = SENSOR_DATA},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_POUT", .pmbus_reg = PMBUS_READ_POUT, .width = WORD_DATA, .data_type = SENSOR_DATA},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_PIN", .pmbus_reg = PMBUS_READ_PIN, .width = WORD_DATA, .data_type = SENSOR_DATA},
    {.page_mask = GENMASK(1,0), .pmbus_name = "READ_DUTY_CYCLE", .pmbus_reg = PMBUS_READ_DUTY_CYCLE, .width = WORD_DATA, .data_type = RAWDATA_WORD},
};

typedef struct xdpe122_phase_page_s {
    u8 loop;
    u8 phase_curr_change_page;
    u8 phase_curr_value_page;
} xdpe122_phase_page_t;

static xdpe122_phase_page_t g_xdpe122_phase_page_group[] = {
    {.loop = XDPE122_LOOP_A, .phase_curr_change_page = 0x30, .phase_curr_value_page = 0x60},
    {.loop = XDPE122_LOOP_B, .phase_curr_change_page = 0x31, .phase_curr_value_page = 0x61},
};

typedef struct xdpe122_phase_curr_reg_value_s {
    u8 phase_index;
    int phase_curr_reg_value;
} xdpe122_phase_curr_reg_value_t;

static xdpe122_phase_curr_reg_value_t g_xdpe122_phase_curr_reg_value_group[] = {
    {.phase_index = 1, .phase_curr_reg_value = XDPE122_PHASE1_REG_VALUE},
    {.phase_index = 2, .phase_curr_reg_value = XDPE122_PHASE2_REG_VALUE},
    {.phase_index = 3, .phase_curr_reg_value = XDPE122_PHASE3_REG_VALUE},
    {.phase_index = 4, .phase_curr_reg_value = XDPE122_PHASE4_REG_VALUE},
    {.phase_index = 5, .phase_curr_reg_value = XDPE122_PHASE5_REG_VALUE},
    {.phase_index = 6, .phase_curr_reg_value = XDPE122_PHASE6_REG_VALUE},
    {.phase_index = 7, .phase_curr_reg_value = XDPE122_PHASE7_REG_VALUE},
    {.phase_index = 8, .phase_curr_reg_value = XDPE122_PHASE8_REG_VALUE},
};

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static ssize_t show_cfg_reg_value(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    int reg = to_sensor_dev_attr_2(da)->index;
    int page = to_sensor_dev_attr_2(da)->nr;
    struct pmbus_data *data = i2c_get_clientdata(client);
    int ret;

    mutex_lock(&data->update_lock);
    ret = wb_pmbus_read_word_data(client, page, 0xff, reg);
    if (ret < 0) {
        DEBUG_ERROR("pmbus_read_word_data read page 0x%x 0x%x failed, errno: %d\n", page, reg, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }
    DEBUG_VERBOSE("Reading value 0x%x from page 0x%x reg 0x%x\n", ret, page, reg);
    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "0x%04x\n", ret);
}

static ssize_t set_cfg_reg_value(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    int reg = to_sensor_dev_attr_2(da)->index;
    int page = to_sensor_dev_attr_2(da)->nr;
    struct pmbus_data *data = i2c_get_clientdata(client);
    int ret, value;

    value = 0;
    ret = kstrtoint(buf, 0, &value);
    if (ret) {
        DEBUG_ERROR("kstrtoint failed %s, errno: %d\n", buf, ret);
        return ret;
    }
    mutex_lock(&data->update_lock);
    DEBUG_VERBOSE("Writting 0x%x to page 0x%x reg 0x%x\n", value, page, reg);
    ret = wb_pmbus_write_word_data(client, page, reg, value);
    if (ret < 0) {
        DEBUG_ERROR("pmbus_write_word_data write page 0x%x 0x%x value 0x%x failed, errno: %d\n", page, reg, value, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }
    mutex_unlock(&data->update_lock);
    return count;
}

static int xdpe122_data2reg_vid(struct pmbus_data *data, int page, long val)
{
    int vrm_version;

    vrm_version = data->info->vrm_version[page];
    DEBUG_VERBOSE("page%d, vrm_version: %d, data_val: %ld\n",
        page, vrm_version, val);
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
static s64 xdpe122_reg2data_vid(struct pmbus_data *data, int page, long val)
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
    DEBUG_VERBOSE("page%d, vrm_version: %d, reg_val: 0x%lx, data_val: %ld\n",
        page, vrm_version, val, rv);
    return rv;
}

static ssize_t xdpe122_avs_vout_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int vout_cmd, vout;

    mutex_lock(&data->update_lock);
    vout_cmd = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (vout_cmd < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd);
        mutex_unlock(&data->update_lock);
        return vout_cmd;
    }

    vout = xdpe122_reg2data_vid(data, attr->index, vout_cmd);
    vout = vout * 1000;
    DEBUG_VERBOSE("%d-%04x: page%d, vout command reg_val: 0x%x, vout: %d uV\n",
        client->adapter->nr, client->addr, attr->index, vout_cmd, vout);

    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%d\n", vout);
}

static ssize_t xdpe122_avs_vout_store(struct device *dev, struct device_attribute *devattr,
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

    /* calc VOUT_COMMAND set value Unit must be mV*/
    vout_mv = vout / 1000;
    vout_cmd_set = xdpe122_data2reg_vid(data, attr->index, vout_mv);
    if ((vout_cmd_set < 0) || (vout_cmd_set > 0xffff)) {
        DEBUG_ERROR("%d-%04x: invalid value, vout %d uV, vout_cmd_set: %d\n",
            client->adapter->nr, client->addr, vout, vout_cmd_set);
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);

    /* Read the VOUT_COMMAND register to confirm whether it needs to be set */
    vout_cmd = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (vout_cmd < 0) {
        DEBUG_ERROR("%d-%04x: read page%d vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd);
        mutex_unlock(&data->update_lock);
        return vout_cmd;
    }

    /* compare vout_cmd and vout_cmd_set */
    if (vout_cmd == vout_cmd_set) {
        DEBUG_VERBOSE("%d-%04x: page%d vout cmd read: 0x%x is equal to vout cmd set: 0x%x, do nothing\n",
            client->adapter->nr, client->addr, attr->index, vout_cmd, vout_cmd_set);
        mutex_unlock(&data->update_lock);
        return count;
    }

    /* close write protect */
    ret = wb_pmbus_write_byte_data(client, attr->index, PMBUS_WRITE_PROTECT, XDPE122_WRITE_PROTECT_CLOSE);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: close page%d write protect failed, ret: %d\n", client->adapter->nr,
            client->addr, attr->index, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

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

    /* open write protect */
    wb_pmbus_write_byte_data(client, attr->index, PMBUS_WRITE_PROTECT, XDPE122_WRITE_PROTECT_OPEN);
    mutex_unlock(&data->update_lock);
    DEBUG_VERBOSE("%d-%04x: set page%d vout cmd success, vout %d uV, vout_cmd_set: 0x%x\n",
        client->adapter->nr, client->addr, attr->index, vout, vout_cmd_set);
    return count;
error:
    wb_pmbus_write_byte_data(client, attr->index, PMBUS_WRITE_PROTECT, XDPE122_WRITE_PROTECT_OPEN);
    mutex_unlock(&data->update_lock);
    return ret;
}

static ssize_t xdpe122_iout_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int reg_val;
    s64 val;

    mutex_lock(&data->update_lock);
    reg_val = wb_pmbus_read_word_data(client, attr->index, 0xff, XDPE122_I2C_READ_IOUT);
    if (reg_val < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, iout reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, XDPE122_I2C_READ_IOUT, reg_val);
        mutex_unlock(&data->update_lock);
        return reg_val;
    }

    val = IOUT_REG_TO_VALUE(reg_val);

    DEBUG_VERBOSE("%d-%04x: page%d, iout_reg_val: 0x%x, iout: %lld mA\n",
        client->adapter->nr, client->addr, attr->index, reg_val, val);

    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%lld\n", val);
}

static ssize_t xdpe122_vout_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int reg_val;
    s64 val;

    mutex_lock(&data->update_lock);
    reg_val = wb_pmbus_read_word_data(client, attr->index, 0xff, XDPE122_I2C_READ_VOUT);
    if (reg_val < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, vout reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, XDPE122_I2C_READ_VOUT, reg_val);
        mutex_unlock(&data->update_lock);
        return reg_val;
    }

    val = VOUT_REG_TO_VALUE(reg_val);

    DEBUG_VERBOSE("%d-%04x: page%d, vout_reg_val: 0x%x, vout: %lld mV\n",
        client->adapter->nr, client->addr, attr->index, reg_val, val);

    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%lld\n", val);
}

static ssize_t xdpe122_temp_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int reg_val;
    s64 val;

    mutex_lock(&data->update_lock);
    reg_val = wb_pmbus_read_word_data(client, attr->index, 0xff, XDPE122_I2C_READ_TEMP);
    if (reg_val < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, temp reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, XDPE122_I2C_READ_TEMP, reg_val);
        mutex_unlock(&data->update_lock);
        return reg_val;
    }

    val = TEMP_REG_TO_VALUE(reg_val);

    DEBUG_VERBOSE("%d-%04x: page%d, temp_reg_val: 0x%x, temp: %lld mC\n",
        client->adapter->nr, client->addr, attr->index, reg_val, val);

    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%lld\n", val);
}

static ssize_t xdpe122_pout_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int reg_val;
    s64 val;

    mutex_lock(&data->update_lock);
    reg_val = wb_pmbus_read_word_data(client, attr->index, 0xff, XDPE122_I2C_READ_POUT);
    if (reg_val < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, pout reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, XDPE122_I2C_READ_POUT, reg_val);
        mutex_unlock(&data->update_lock);
        return reg_val;
    }

    val = POUT_REG_TO_VALUE(reg_val);

    DEBUG_VERBOSE("%d-%04x: page%d, pout_reg_val: 0x%x, pout: %lld uW\n",
        client->adapter->nr, client->addr, attr->index, reg_val, val);

    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%lld\n", val);
}

static ssize_t xdpe122_word_data_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    u8 page = to_sensor_dev_attr_2(devattr)->nr;
    u8 reg = to_sensor_dev_attr_2(devattr)->index;
    struct pmbus_data *data = i2c_get_clientdata(client);
    int word_data;

    mutex_lock(&data->update_lock);
    word_data = wb_pmbus_read_word_data(client, page, 0xff, reg);
    if (word_data < 0) {
        DEBUG_ERROR("%d-%04x: read page%u, vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, page, reg, word_data);
        mutex_unlock(&data->update_lock);
        return word_data;
    }

    DEBUG_VERBOSE("%d-%04x: read word data success, page%u, reg: 0x%x, value: 0x%04x\n",
        client->adapter->nr, client->addr, page, reg, word_data);
    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "0x%04x\n", word_data);
}

static ssize_t xdpe122_phase_curr_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    int ret;
    struct i2c_client *client = to_i2c_client(dev->parent);
    u8 loop_index = to_sensor_dev_attr_2(devattr)->nr;
    u8 phase_index = to_sensor_dev_attr_2(devattr)->index;
    struct pmbus_data *data = i2c_get_clientdata(client);
    int word_data;
    int a_size;
    int i;
    u8 phase_curr_change_page, phase_curr_value_page;
    s64 value;
    int phase_curr_reg_value;

    mutex_lock(&data->update_lock);

    DEBUG_VERBOSE("%d-%04x: loop%u, phase_index: %u\n",
                client->adapter->nr, client->addr, loop_index, phase_index);

    a_size = ARRAY_SIZE(g_xdpe122_phase_page_group);
    for (i = 0; i < a_size; i++) {
        if (g_xdpe122_phase_page_group[i].loop == loop_index) {
            phase_curr_change_page = g_xdpe122_phase_page_group[i].phase_curr_change_page;
            phase_curr_value_page = g_xdpe122_phase_page_group[i].phase_curr_value_page;
            DEBUG_VERBOSE("%d-%04x: loop%u, phase_curr_change_page: 0x%x, phase_curr_value_page: 0x%x\n",
                client->adapter->nr, client->addr, loop_index, phase_curr_change_page, phase_curr_value_page);
            break;
        }
    }
    if (i == a_size) {
        DEBUG_ERROR("%d-%04x: invalid, loop_index%u\n",client->adapter->nr, client->addr, loop_index);
        mutex_unlock(&data->update_lock);
        return -EINVAL;
    }

    a_size = ARRAY_SIZE(g_xdpe122_phase_curr_reg_value_group);
    for (i = 0; i < a_size; i++) {
        if (g_xdpe122_phase_curr_reg_value_group[i].phase_index == phase_index) {
            phase_curr_reg_value = g_xdpe122_phase_curr_reg_value_group[i].phase_curr_reg_value;
            DEBUG_VERBOSE("%d-%04x: loop%u, phase_index: %u, phase_curr_reg_value: 0x%x\n",
                client->adapter->nr, client->addr, loop_index, phase_index, phase_curr_reg_value);
            break;
        }
    }
    if (i == a_size) {
        DEBUG_ERROR("%d-%04x: invalid, phase_index%u\n",client->adapter->nr, client->addr, phase_index);
        mutex_unlock(&data->update_lock);
        return -EINVAL;
    }

    ret = wb_pmbus_write_word_data(client, phase_curr_change_page, XDPE122_PHASE_CHANGE_REG, phase_curr_reg_value);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set loop%u phase curr reg, phase_page: 0x%x, phase_reg: 0x%x, value: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, loop_index, phase_curr_change_page, XDPE122_PHASE_CHANGE_REG, phase_curr_reg_value, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    word_data = wb_pmbus_read_word_data(client, phase_curr_value_page, 0xff, XDPE122_PHASE_VALUE_REG);
    if (word_data < 0) {
        DEBUG_ERROR("%d-%04x: read loop%u phase curr, phase_page: 0x%x, phase_reg: 0x%x, ret: %d\n",
            client->adapter->nr, client->addr, loop_index, phase_curr_value_page, XDPE122_PHASE_VALUE_REG, word_data);
        mutex_unlock(&data->update_lock);
        return word_data;
    }

    /* set PHASE_CHANGE_REG 0x600 */
    ret = wb_pmbus_write_word_data(client, phase_curr_change_page, XDPE122_PHASE_CHANGE_REG, XDPE122_PHASE_REG_DEFAULT_VALUE);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set loop%u phase curr reg, phase_page: 0x%x, phase_reg: 0x%x, value: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, loop_index, phase_curr_change_page, XDPE122_PHASE_CHANGE_REG, XDPE122_PHASE_REG_DEFAULT_VALUE, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    /* reg * 62.5mA */
    value = PHASE_CURR_REG_TO_VALUE(word_data);

    DEBUG_VERBOSE("%d-%04x: read phase curr word data success, reg: 0x%04x, value: %lld mA\n",
        client->adapter->nr, client->addr, word_data, value);

    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%lld\n", value);
}

static ssize_t xdpe122_crc_show(struct device *dev, 
                struct device_attribute *devattr __maybe_unused, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data;
    int ret_lo, ret_hi;
    uint16_t crc_lo, crc_hi;
    uint32_t crc;

    if (client == NULL) {
        return -ENODEV;
    }
    data = i2c_get_clientdata(client);
    if (data == NULL) {
        DEBUG_ERROR("pmbus_data is NULL\n");
        return -ENODEV;
    }

    mutex_lock(&data->update_lock);
    ret_lo = wb_pmbus_read_word_data(client, XDPE122_CRC_PAGE, 0xff, XDPE122_CRC_LO_REG);
    ret_hi = wb_pmbus_read_word_data(client, XDPE122_CRC_PAGE, 0xff, XDPE122_CRC_HI_REG);
    mutex_unlock(&data->update_lock);

    if (ret_lo < 0) {
        DEBUG_ERROR("read crc lo failed: %d\n", ret_lo);
        return ret_lo;
    }

    if (ret_hi < 0) {
        DEBUG_ERROR("read crc hi failed: %d\n", ret_hi);
        return ret_hi;
    }

    crc_lo = (uint16_t)ret_lo;
    crc_hi = (uint16_t)ret_hi;

    DEBUG_VERBOSE("CRC lo: 0x%04x, hi: 0x%04x\n", crc_lo, crc_hi);

    crc = ((u32)crc_hi << 16) | crc_lo;

    return snprintf(buf, PAGE_SIZE, "0x%08x\n", crc);
}

static SENSOR_DEVICE_ATTR_RW(avs0_vout, xdpe122_avs_vout, 0);
static SENSOR_DEVICE_ATTR_RW(avs1_vout, xdpe122_avs_vout, 1);
static SENSOR_DEVICE_ATTR_RW(avs0_vout_max, pmbus_avs_vout_max, 0);
static SENSOR_DEVICE_ATTR_RW(avs0_vout_min, pmbus_avs_vout_min, 0);
static SENSOR_DEVICE_ATTR_RW(avs1_vout_max, pmbus_avs_vout_max, 1);
static SENSOR_DEVICE_ATTR_RW(avs1_vout_min, pmbus_avs_vout_min, 1);
static SENSOR_DEVICE_ATTR_RO(loopa_iout, xdpe122_iout, XDPE122_LOOPA_STATUS_PAGE);
static SENSOR_DEVICE_ATTR_RO(loopa_vout, xdpe122_vout, XDPE122_LOOPA_STATUS_PAGE);
static SENSOR_DEVICE_ATTR_RO(loopa_temp, xdpe122_temp, XDPE122_LOOPA_STATUS_PAGE);
static SENSOR_DEVICE_ATTR_RO(loopa_pout, xdpe122_pout, XDPE122_LOOPA_STATUS_PAGE);
static SENSOR_DEVICE_ATTR_RO(loopb_iout, xdpe122_iout, XDPE122_LOOPB_STATUS_PAGE);
static SENSOR_DEVICE_ATTR_RO(loopb_vout, xdpe122_vout, XDPE122_LOOPB_STATUS_PAGE);
static SENSOR_DEVICE_ATTR_RO(loopb_temp, xdpe122_temp, XDPE122_LOOPB_STATUS_PAGE);
static SENSOR_DEVICE_ATTR_RO(loopb_pout, xdpe122_pout, XDPE122_LOOPB_STATUS_PAGE);
static SENSOR_DEVICE_ATTR_2_RO(version1, xdpe122_word_data, XDPE122_VERSION_PAGE, XDPE122_VERSION1_REG);
static SENSOR_DEVICE_ATTR_2_RO(version2, xdpe122_word_data, XDPE122_VERSION_PAGE, XDPE122_VERSION2_REG);

static SENSOR_DEVICE_ATTR_2_RO(loopa_phase1_curr, xdpe122_phase_curr, XDPE122_LOOP_A, 1);
static SENSOR_DEVICE_ATTR_2_RO(loopa_phase2_curr, xdpe122_phase_curr, XDPE122_LOOP_A, 2);
static SENSOR_DEVICE_ATTR_2_RO(loopa_phase3_curr, xdpe122_phase_curr, XDPE122_LOOP_A, 3);
static SENSOR_DEVICE_ATTR_2_RO(loopa_phase4_curr, xdpe122_phase_curr, XDPE122_LOOP_A, 4);
static SENSOR_DEVICE_ATTR_2_RO(loopa_phase5_curr, xdpe122_phase_curr, XDPE122_LOOP_A, 5);
static SENSOR_DEVICE_ATTR_2_RO(loopa_phase6_curr, xdpe122_phase_curr, XDPE122_LOOP_A, 6);
static SENSOR_DEVICE_ATTR_2_RO(loopa_phase7_curr, xdpe122_phase_curr, XDPE122_LOOP_A, 7);
static SENSOR_DEVICE_ATTR_2_RO(loopa_phase8_curr, xdpe122_phase_curr, XDPE122_LOOP_A, 8);

static SENSOR_DEVICE_ATTR_2_RO(loopb_phase1_curr, xdpe122_phase_curr, XDPE122_LOOP_B, 1);
static SENSOR_DEVICE_ATTR_2_RO(loopb_phase2_curr, xdpe122_phase_curr, XDPE122_LOOP_B, 2);
static SENSOR_DEVICE_ATTR_2_RO(loopb_phase3_curr, xdpe122_phase_curr, XDPE122_LOOP_B, 3);
static SENSOR_DEVICE_ATTR_2_RO(loopb_phase4_curr, xdpe122_phase_curr, XDPE122_LOOP_B, 4);
static SENSOR_DEVICE_ATTR_2_RO(loopb_phase5_curr, xdpe122_phase_curr, XDPE122_LOOP_B, 5);
static SENSOR_DEVICE_ATTR_2_RO(loopb_phase6_curr, xdpe122_phase_curr, XDPE122_LOOP_B, 6);
static SENSOR_DEVICE_ATTR_2_RO(loopb_phase7_curr, xdpe122_phase_curr, XDPE122_LOOP_B, 7);
static SENSOR_DEVICE_ATTR_2_RO(loopb_phase8_curr, xdpe122_phase_curr, XDPE122_LOOP_B, 8);

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
    &sensor_dev_attr_loopb_phase5_curr.dev_attr.attr,
    &sensor_dev_attr_loopb_phase6_curr.dev_attr.attr,
    &sensor_dev_attr_loopb_phase7_curr.dev_attr.attr,
    &sensor_dev_attr_loopb_phase8_curr.dev_attr.attr,
    NULL,
};

static const struct attribute_group phase_curr_group = {
    .attrs = phase_curr_attrs,
};

static struct attribute *others_attrs[] = {
    &sensor_dev_attr_version1.dev_attr.attr,
    &sensor_dev_attr_version2.dev_attr.attr,
    NULL,
};

static const struct attribute_group others_group = {
    .attrs = others_attrs,
};

static const struct attribute_group *xdpe122_attribute_groups[] = {
    &avs_ctrl_group,
    &loop_status_group,
    &phase_curr_group,
    &others_group,
    NULL,
};

/******************** sysfs attr ***********************/
static SENSOR_DEVICE_ATTR(dfx_info, S_IRUGO, show_pmbus_dfx_info, NULL, -1);
static SENSOR_DEVICE_ATTR(dfx_info0, S_IRUGO, show_pmbus_dfx_info, NULL, 0);
static SENSOR_DEVICE_ATTR(dfx_info1, S_IRUGO, show_pmbus_dfx_info, NULL, 1);
static SENSOR_DEVICE_ATTR_RO(device_name, pmbus_device_name, PMBUS_IC_DEVICE_ID);
static SENSOR_DEVICE_ATTR_RO(device_id, pmbus_block_data, PMBUS_IC_DEVICE_ID);
static SENSOR_DEVICE_ATTR_RO(ic_device_rev, pmbus_block_data, PMBUS_IC_DEVICE_REV);
static SENSOR_DEVICE_ATTR_RO(status0_byte, pmbus_get_status_byte, 0);
static SENSOR_DEVICE_ATTR_RO(status1_byte, pmbus_get_status_byte, 1);
static SENSOR_DEVICE_ATTR_RO(status0_word, pmbus_get_status_word, 0);
static SENSOR_DEVICE_ATTR_RO(status1_word, pmbus_get_status_word, 1);
static SENSOR_DEVICE_ATTR_RO(crc, xdpe122_crc, 0);
/* Page: 0x22; Reg: 0x1f */
static SENSOR_DEVICE_ATTR_2(cfg_reg_0x1f, S_IRUGO | S_IWUSR, show_cfg_reg_value, set_cfg_reg_value, 0x22, 0x1f);

static struct attribute *xdpe12284_attrs[] = {
    &sensor_dev_attr_dfx_info.dev_attr.attr,
    &sensor_dev_attr_dfx_info0.dev_attr.attr,
    &sensor_dev_attr_dfx_info1.dev_attr.attr,
    &sensor_dev_attr_device_id.dev_attr.attr,
    &sensor_dev_attr_device_name.dev_attr.attr,
    &sensor_dev_attr_ic_device_rev.dev_attr.attr,
    &sensor_dev_attr_status0_byte.dev_attr.attr,
    &sensor_dev_attr_status1_byte.dev_attr.attr,
    &sensor_dev_attr_status0_word.dev_attr.attr,
    &sensor_dev_attr_status1_word.dev_attr.attr,
    &sensor_dev_attr_crc.dev_attr.attr,
    &sensor_dev_attr_cfg_reg_0x1f.dev_attr.attr,
    NULL
};

static const struct attribute_group xdpe12284_sysfs_group = {
    .attrs = xdpe12284_attrs,
};

static int xdpe122_read_word_data(struct i2c_client *client, int page,
				  int phase, int reg)
{
	const struct pmbus_driver_info *info = wb_pmbus_get_driver_info(client);
	long val;
	s16 exponent;
	s32 mantissa;
	int ret;

	switch (reg) {
	case PMBUS_VOUT_OV_FAULT_LIMIT:
	case PMBUS_VOUT_UV_FAULT_LIMIT:
		ret = wb_pmbus_read_word_data(client, page, phase, reg);
		if (ret < 0)
			return ret;

		/* Convert register value to LINEAR11 data. */
		exponent = ((s16)ret) >> 11;
		mantissa = ((s16)((ret & GENMASK(10, 0)) << 5)) >> 5;
		val = mantissa * 1000L;
		if (exponent >= 0)
			val <<= exponent;
		else
			val >>= -exponent;

		/* Convert data to VID register. */
		switch (info->vrm_version[page]) {
		case vr13:
			if (val >= 500)
				return 1 + DIV_ROUND_CLOSEST(val - 500, 10);
			return 0;
		case vr12:
			if (val >= 250)
				return 1 + DIV_ROUND_CLOSEST(val - 250, 5);
			return 0;
		case imvp9:
			if (val >= 200)
				return 1 + DIV_ROUND_CLOSEST(val - 200, 10);
			return 0;
		case amd625mv:
			if (val >= 200 && val <= 1550)
				return DIV_ROUND_CLOSEST((1550 - val) * 100,
							 625);
			return 0;
		default:
			return -EINVAL;
		}
	default:
		return -ENODATA;
	}

	return 0;
}

static int xdpe122_identify(struct i2c_client *client,
			    struct pmbus_driver_info *info)
{
	u8 vout_params;
	int i, ret;

	for (i = 0; i < XDPE122_PAGE_NUM; i++) {
		/* Read the register with VOUT scaling value.*/
		ret = wb_pmbus_read_byte_data(client, i, PMBUS_VOUT_MODE);
		if (ret < 0)
			return ret;

		vout_params = ret & GENMASK(4, 0);

		switch (vout_params) {
		case XDPE122_PROT_VR12_5_10MV:
			info->vrm_version[i] = vr13;
			break;
		case XDPE122_PROT_VR12_5MV:
			info->vrm_version[i] = vr12;
			break;
		case XDPE122_PROT_IMVP9_10MV:
			info->vrm_version[i] = imvp9;
			break;
		case XDPE122_AMD_625MV:
			info->vrm_version[i] = amd625mv;
			break;
		default:
			return -EINVAL;
		}
	}

	return 0;
}

static struct pmbus_driver_info xdpe122_info = {
	.pages = XDPE122_PAGE_NUM,
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
	.groups = xdpe122_attribute_groups,
	.identify = xdpe122_identify,
	.read_word_data = xdpe122_read_word_data,
};

static int xdpe122_probe(struct i2c_client *client)
{
    struct pmbus_driver_info *info;
    struct pmbus_data *data;
    int ret;

    info = devm_kmemdup(&client->dev, &xdpe122_info, sizeof(*info), GFP_KERNEL);
    if (!info) {
        dev_info(&client->dev, "devm_kmemdup failed\n");
        return -ENOMEM;
    }

    ret = wb_pmbus_do_probe(client, info);
    if (ret != 0) {
        dev_info(&client->dev, "wb_pmbus_do_probe failed, ret: %d.\n", ret);
        return ret;
    }

    data = i2c_get_clientdata(client);
    data->pmbus_info_array = xdpe122_dfx_infos;
    data->pmbus_info_array_size = ARRAY_SIZE(xdpe122_dfx_infos);

    ret = sysfs_create_group(&client->dev.kobj, &xdpe12284_sysfs_group);
    if (ret != 0) {
        dev_info(&client->dev, "Failed to create xdpe12284_sysfs_group, ret: %d.\n", ret);
        wb_pmbus_do_remove(client);
        return ret;
    }
    return 0;
}

static void xdpe122_remove(struct i2c_client *client)
{
    sysfs_remove_group(&client->dev.kobj, &xdpe12284_sysfs_group);
    (void)wb_pmbus_do_remove(client);
    return;
}


static const struct i2c_device_id xdpe122_id[] = {
	{"wb_xdpe12254", 0},
	{"wb_xdpe12284", 0},
	{}
};

MODULE_DEVICE_TABLE(i2c, xdpe122_id);

static const struct of_device_id __maybe_unused xdpe122_of_match[] = {
	{.compatible = "infineon,wb_xdpe12254"},
	{.compatible = "infineon,wb_xdpe12284"},
	{}
};
MODULE_DEVICE_TABLE(of, xdpe122_of_match);

static struct i2c_driver xdpe122_driver = {
	.driver = {
		.name = "wb_xdpe12284",
		.of_match_table = of_match_ptr(xdpe122_of_match),
	},
	.probe_new = xdpe122_probe,
	.remove = xdpe122_remove,
	.id_table = xdpe122_id,
};

module_i2c_driver(xdpe122_driver);

MODULE_AUTHOR("Vadim Pasternak <vadimp@mellanox.com>");
MODULE_DESCRIPTION("PMBus driver for Infineon XDPE122 family");
MODULE_LICENSE("GPL");
