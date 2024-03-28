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

#define XDPE122_PROT_VR12_5MV        (0x01) /* VR12.0 mode, 5-mV DAC */
#define XDPE122_PROT_VR12_5_10MV     (0x02) /* VR12.5 mode, 10-mV DAC */
#define XDPE122_PROT_IMVP9_10MV      (0x03) /* IMVP9 mode, 10-mV DAC */
#define XDPE122_AMD_625MV            (0x10) /* AMD mode 6.25mV */
#define XDPE122_PAGE_NUM             (2)
#define XDPE122_WRITE_PROTECT_CLOSE  (0x00)
#define XDPE122_WRITE_PROTECT_OPEN   (0x40)

static int g_wb_xdpe122_debug = 0;
static int g_wb_xdpe122_error = 0;

module_param(g_wb_xdpe122_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_xdpe122_error, int, S_IRUGO | S_IWUSR);

#define WB_XDPE122_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_xdpe122_debug) { \
        printk(KERN_INFO "[WB_XDPE122][VER][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_XDPE122_ERROR(fmt, args...) do {                                        \
    if (g_wb_xdpe122_error) { \
        printk(KERN_ERR "[WB_XDPE122][ERR][func:%s line:%d]\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static int xdpe122_data2reg_vid(struct pmbus_data *data, int page, long val)
{
    int vrm_version;

    vrm_version = data->info->vrm_version[page];
    WB_XDPE122_VERBOSE("page%d, vrm_version: %d, data_val: %ld\n",
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
        WB_XDPE122_ERROR("Unsupport vrm_version, page%d, vrm_version: %d\n",
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
    WB_XDPE122_VERBOSE("page%d, vrm_version: %d, reg_val: 0x%lx, data_val: %ld\n",
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
        WB_XDPE122_ERROR("%d-%04x: read page%d, vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd);
        mutex_unlock(&data->update_lock);
        return vout_cmd;
    }

    vout = xdpe122_reg2data_vid(data, attr->index, vout_cmd);
    vout = vout * 1000;
    WB_XDPE122_VERBOSE("%d-%04x: page%d, vout command reg_val: 0x%x, vout: %d uV\n",
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
        WB_XDPE122_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    ret = kstrtoint(buf, 0, &vout);
    if (ret) {
        WB_XDPE122_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    if (vout <= 0) {
        WB_XDPE122_ERROR("%d-%04x: invalid value: %d \n", client->adapter->nr, client->addr, vout);
        return -EINVAL;
    }

    vout_max = data->vout_max[attr->index];
    vout_min = data->vout_min[attr->index];
    if ((vout > vout_max) || (vout < vout_min)) {
        WB_XDPE122_ERROR("%d-%04x: vout value: %d, out of range [%d, %d] \n", client->adapter->nr,
            client->addr, vout, vout_min, vout_max);
        return -EINVAL;
    }

    /* calc VOUT_COMMAND set value Unit must be mV*/
    vout_mv = vout / 1000;
    vout_cmd_set = xdpe122_data2reg_vid(data, attr->index, vout_mv);
    if ((vout_cmd_set < 0) || (vout_cmd_set > 0xffff)) {
        WB_XDPE122_ERROR("%d-%04x: invalid value, vout %d uV, vout_cmd_set: %d\n",
            client->adapter->nr, client->addr, vout, vout_cmd_set);
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);

    /* close write protect */
    ret = wb_pmbus_write_byte_data(client, attr->index, PMBUS_WRITE_PROTECT, XDPE122_WRITE_PROTECT_CLOSE);
    if (ret < 0) {
        WB_XDPE122_ERROR("%d-%04x: close page%d write protect failed, ret: %d\n", client->adapter->nr,
            client->addr, attr->index, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    /* set VOUT_COMMAND */
    ret = wb_pmbus_write_word_data(client, attr->index, PMBUS_VOUT_COMMAND, vout_cmd_set);
    if (ret < 0) {
        WB_XDPE122_ERROR("%d-%04x: set page%d vout cmd reg: 0x%x, value: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd_set, ret);
        goto error;
    }

    /* read back VOUT_COMMAND */
    vout_cmd = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (vout_cmd < 0) {
        ret = vout_cmd;
        WB_XDPE122_ERROR("%d-%04x: read page%d vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, ret);
        goto error;
    }

    /* compare vout_cmd and vout_cmd_set */
    if (vout_cmd != vout_cmd_set) {
        ret = -EIO;
        WB_XDPE122_ERROR("%d-%04x: vout cmd value check error, vout cmd read: 0x%x, vout cmd set: 0x%x\n",
            client->adapter->nr, client->addr, vout_cmd, vout_cmd_set);
        goto error;
    }

    /* open write protect */
    wb_pmbus_write_byte_data(client, attr->index, PMBUS_WRITE_PROTECT, XDPE122_WRITE_PROTECT_OPEN);
    mutex_unlock(&data->update_lock);
    WB_XDPE122_VERBOSE("%d-%04x: set page%d vout cmd success, vout %d uV, vout_cmd_set: 0x%x\n",
        client->adapter->nr, client->addr, attr->index, vout, vout_cmd_set);
    return count;
error:
    wb_pmbus_write_byte_data(client, attr->index, PMBUS_WRITE_PROTECT, XDPE122_WRITE_PROTECT_OPEN);
    mutex_unlock(&data->update_lock);
    return ret;
}

static ssize_t xdpe122_avs_vout_max_store(struct device *dev,
                   struct device_attribute *devattr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int ret, vout_threshold;

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        WB_XDPE122_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    ret = kstrtoint(buf, 0, &vout_threshold);
    if (ret) {
        WB_XDPE122_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    WB_XDPE122_VERBOSE("%d-%04x: vout%d max threshold: %d", client->adapter->nr, client->addr,
        attr->index, vout_threshold);

    data->vout_max[attr->index] = vout_threshold;
    return count;
}

static ssize_t xdpe122_avs_vout_max_show(struct device *dev,
                   struct device_attribute *devattr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        WB_XDPE122_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    return snprintf(buf, PAGE_SIZE, "%d\n", data->vout_max[attr->index]);
}

static ssize_t xdpe122_avs_vout_min_store(struct device *dev,
                   struct device_attribute *devattr, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int ret, vout_threshold;

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        WB_XDPE122_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    ret = kstrtoint(buf, 0, &vout_threshold);
    if (ret) {
        WB_XDPE122_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    WB_XDPE122_VERBOSE("%d-%04x: vout%d min threshold: %d", client->adapter->nr, client->addr,
        attr->index, vout_threshold);

    data->vout_min[attr->index] = vout_threshold;
    return count;
}

static ssize_t xdpe122_avs_vout_min_show(struct device *dev,
                   struct device_attribute *devattr, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        WB_XDPE122_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    return snprintf(buf, PAGE_SIZE, "%d\n", data->vout_min[attr->index]);
}

static SENSOR_DEVICE_ATTR_RW(avs0_vout, xdpe122_avs_vout, 0);
static SENSOR_DEVICE_ATTR_RW(avs1_vout, xdpe122_avs_vout, 1);
static SENSOR_DEVICE_ATTR_RW(avs0_vout_max, xdpe122_avs_vout_max, 0);
static SENSOR_DEVICE_ATTR_RW(avs0_vout_min, xdpe122_avs_vout_min, 0);
static SENSOR_DEVICE_ATTR_RW(avs1_vout_max, xdpe122_avs_vout_max, 1);
static SENSOR_DEVICE_ATTR_RW(avs1_vout_min, xdpe122_avs_vout_min, 1);

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

static const struct attribute_group *xdpe122_attribute_groups[] = {
    &avs_ctrl_group,
    NULL,
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

	info = devm_kmemdup(&client->dev, &xdpe122_info, sizeof(*info),
			    GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	return wb_pmbus_do_probe(client, info);
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
	.remove = wb_pmbus_do_remove,
	.id_table = xdpe122_id,
};

module_i2c_driver(xdpe122_driver);

MODULE_AUTHOR("Vadim Pasternak <vadimp@mellanox.com>");
MODULE_DESCRIPTION("PMBus driver for Infineon XDPE122 family");
MODULE_LICENSE("GPL");
