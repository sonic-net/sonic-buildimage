#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/hwmon-sysfs.h>
#include <linux/string.h>
#include <linux/sysfs.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include "wb_pmbus.h"
#include <wb_bsp_kernel_debug.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

#define BUF_SIZE                    (256)
#define XDPE132G5C_PAGE_NUM         (2)
#define XDPE132G5C_PROT_VR12_5MV    (0x01) /* VR12.0 mode, 5-mV DAC */
#define XDPE132G5C_PROT_VR12_5_10MV (0x02) /* VR12.5 mode, 10-mV DAC */
#define XDPE132G5C_PROT_IMVP9_10MV  (0x03) /* IMVP9 mode, 10-mV DAC */
#define XDPE132G5C_PROT_VR13_10MV   (0x04) /* VR13.0 mode, 10-mV DAC */
#define XDPE132G5C_PROT_IMVP8_5MV   (0x05) /* IMVP8 mode, 5-mV DAC */
#define XDPE132G5C_PROT_VR13_5MV    (0x07) /* VR13.0 mode, 5-mV DAC */
#define RETRY_TIME                  (15)

enum chips {
    XDPE132G5C,
    XDPE1A2G5B,
    XDPE19284C,
    XDPE192C4B,
};

struct xdpe_chip_data {
    enum chips id;
    struct pmbus_driver_info info;
    u32 vout_multiplier[2];
};

#define to_xdpe_chip_data(x) container_of(x, struct xdpe_chip_data, info)

static pmbus_info_t dfx_infos[] = {
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

typedef struct xdpe_vout_data_s {
    u8 vout_mode;
    int vout_precision;
} xdpe_vout_data_t;

static xdpe_vout_data_t g_xdpe_vout_group[] = {
    {.vout_mode = 0x18, .vout_precision = 256},
    {.vout_mode = 0x17, .vout_precision = 512},
    {.vout_mode = 0x16, .vout_precision = 1024},
    {.vout_mode = 0x15, .vout_precision = 2048},
    {.vout_mode = 0x14, .vout_precision = 4096},
};

static ssize_t set_xdpe132g5c_avs(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    int ret;
    unsigned long val;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data;

    data = i2c_get_clientdata(client);
    val = 0;
    ret = kstrtoul(buf, 0, &val);
    if (ret){
        return ret;
    }
    mutex_lock(&data->update_lock);
    /* set value */
    ret = wb_pmbus_write_word_data(client, attr->index, PMBUS_VOUT_COMMAND, (u16)val);
    if (ret < 0) {
        DEBUG_ERROR("set pmbus_vout_command fail\n");
        goto finish_set;
    }
finish_set:
    wb_pmbus_clear_faults(client);
    mutex_unlock(&data->update_lock);
    return (ret < 0) ? ret : count;

}

static ssize_t show_xdpe132g5c_avs(struct device *dev, struct device_attribute *da, char *buf)
{
    int val;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data;

    data = i2c_get_clientdata(client);
    mutex_lock(&data->update_lock);
    val = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (val < 0) {
        DEBUG_ERROR("fail val = %d\n", val);
        goto finish_show;
    }
finish_show:
    wb_pmbus_clear_faults(client);
    mutex_unlock(&data->update_lock);
    return snprintf(buf, BUF_SIZE, "0x%04x\n", val);
}

static int xdpe_get_vout_precision(struct i2c_client *client, int page, int *vout_precision)
{
    int i, vout_mode, a_size;

    vout_mode = wb_pmbus_read_byte_data(client, page, PMBUS_VOUT_MODE);
    if (vout_mode < 0) {
        DEBUG_ERROR("%d-%04x: read xdpe page%d vout mode reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, page, PMBUS_VOUT_MODE, vout_mode);
        return vout_mode;
    }

    a_size = ARRAY_SIZE(g_xdpe_vout_group);
    for (i = 0; i < a_size; i++) {
        if (g_xdpe_vout_group[i].vout_mode == vout_mode) {
            *vout_precision = g_xdpe_vout_group[i].vout_precision;
            DEBUG_VERBOSE("%d-%04x: match, page%d, vout mode: 0x%x, precision: %d\n",
                client->adapter->nr, client->addr, page, vout_mode, *vout_precision);
            break;
        }
    }
    if (i == a_size) {
        DEBUG_ERROR("%d-%04x: invalid, page%d, vout mode: 0x%x\n",client->adapter->nr, client->addr,
            page, vout_mode);
        return -EINVAL;
    }
    return 0;
}

static ssize_t xdpe132g5_avs_vout_show(struct device *dev, struct device_attribute *devattr,
                   char *buf)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int vout_cmd, ret, vout_precision;
    s64 vout;

    mutex_lock(&data->update_lock);
    ret = xdpe_get_vout_precision(client, attr->index, &vout_precision);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: get xdpe avs%d vout precision failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    vout_cmd = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (vout_cmd < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd);
        mutex_unlock(&data->update_lock);
        return vout_cmd;
    }

    mutex_unlock(&data->update_lock);
    vout = div_s64((s64)vout_cmd * 1000L * 1000L, vout_precision);
    DEBUG_VERBOSE("%d-%04x: page%d vout: %lld, vout_cmd: 0x%x, precision: %d\n", client->adapter->nr,
        client->addr, attr->index, vout, vout_cmd, vout_precision);
    return snprintf(buf, PAGE_SIZE, "%lld\n", vout);
}

static ssize_t xdpe132g5_avs_vout_store(struct device *dev, struct device_attribute *devattr,
                   const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev->parent);
    struct sensor_device_attribute *attr = to_sensor_dev_attr(devattr);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int vout_max, vout_min;
    int ret, vout_cmd, vout_cmd_set;
    int vout_precision;
    s64 vout;

    if ((attr->index < 0) || (attr->index >= PMBUS_PAGES)) {
        DEBUG_ERROR("%d-%04x: invalid index: %d \n", client->adapter->nr, client->addr,
            attr->index);
        return -EINVAL;
    }

    vout = 0;
    ret = kstrtos64(buf, 0, &vout);
    if (ret) {
        DEBUG_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    if (vout <= 0) {
        DEBUG_ERROR("%d-%04x: invalid value: %lld \n", client->adapter->nr, client->addr, vout);
        return -EINVAL;
    }

    vout_max = data->vout_max[attr->index];
    vout_min = data->vout_min[attr->index];
    if ((vout > vout_max) || (vout < vout_min)) {
        DEBUG_ERROR("%d-%04x: vout value: %lld, out of range [%d, %d] \n", client->adapter->nr,
            client->addr, vout, vout_min, vout_max);
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    ret = xdpe_get_vout_precision(client, attr->index, &vout_precision);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: get xdpe avs%d vout precision failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    vout_cmd_set = div_s64(vout * vout_precision, 1000L * 1000L);
    if (vout_cmd_set > 0xffff) {
        DEBUG_ERROR("%d-%04x: invalid value, page%d, vout: %lld, vout_precision: %d, vout_cmd_set: 0x%x\n",
            client->adapter->nr, client->addr, attr->index, vout, vout_precision, vout_cmd_set);
        mutex_unlock(&data->update_lock);
        return -EINVAL;
    }

    /* Read the VOUT_COMMAND register to confirm whether it needs to be set */
    vout_cmd = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (vout_cmd < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd);
        mutex_unlock(&data->update_lock);
        return vout_cmd;
    }
    if (vout_cmd == vout_cmd_set) {
        DEBUG_VERBOSE("%d-%04x: page%d vout cmd read: 0x%x is equal to vout cmd set: 0x%x, do nothing\n",
            client->adapter->nr, client->addr, attr->index, vout_cmd, vout_cmd_set);
        mutex_unlock(&data->update_lock);
        return count;
    }

    /* set VOUT_COMMAND */
    ret = wb_pmbus_write_word_data(client, attr->index, PMBUS_VOUT_COMMAND, (u16)vout_cmd_set);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe page%d vout cmd reg: 0x%x, value: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd_set, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    /* read back VOUT_COMMAND */
    vout_cmd = wb_pmbus_read_word_data(client, attr->index, 0xff, PMBUS_VOUT_COMMAND);
    if (vout_cmd < 0) {
        DEBUG_ERROR("%d-%04x: read page%d, vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, PMBUS_VOUT_COMMAND, vout_cmd);
        mutex_unlock(&data->update_lock);
        return vout_cmd;
    }

    if (vout_cmd != vout_cmd_set) {
        DEBUG_ERROR("%d-%04x: page%d vout cmd value check error, vout cmd read: 0x%x, vout cmd set: 0x%x\n",
            client->adapter->nr, client->addr, attr->index, vout_cmd, vout_cmd_set);
        mutex_unlock(&data->update_lock);
        return -EIO;
    }
    mutex_unlock(&data->update_lock);
    DEBUG_VERBOSE("%d-%04x: set page%d vout cmd success, vout: %lld uV, vout_cmd_set: 0x%x\n",
        client->adapter->nr, client->addr, attr->index, vout, vout_cmd_set);
    return count;
}

static SENSOR_DEVICE_ATTR_RW(avs0_vout, xdpe132g5_avs_vout, 0);
static SENSOR_DEVICE_ATTR_RW(avs1_vout, xdpe132g5_avs_vout, 1);
static SENSOR_DEVICE_ATTR_RW(avs0_vout_max, pmbus_avs_vout_max, 0);
static SENSOR_DEVICE_ATTR_RW(avs0_vout_min, pmbus_avs_vout_min, 0);
static SENSOR_DEVICE_ATTR_RW(avs1_vout_max, pmbus_avs_vout_max, 1);
static SENSOR_DEVICE_ATTR_RW(avs1_vout_min, pmbus_avs_vout_min, 1);

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

static const struct attribute_group *xdpe132g5_attribute_groups[] = {
    &avs_ctrl_group,
    NULL,
};

static SENSOR_DEVICE_ATTR(avs0_vout_command, S_IRUGO | S_IWUSR, show_xdpe132g5c_avs, set_xdpe132g5c_avs, 0);
static SENSOR_DEVICE_ATTR(avs1_vout_command, S_IRUGO | S_IWUSR, show_xdpe132g5c_avs, set_xdpe132g5c_avs, 1);
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

static struct attribute *xdpe132g5c_sysfs_attrs[] = {
    &sensor_dev_attr_avs0_vout_command.dev_attr.attr,
    &sensor_dev_attr_avs1_vout_command.dev_attr.attr,
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
    NULL,
};

static const struct attribute_group xdpe132g5c_sysfs_attrs_group = {
    .attrs = xdpe132g5c_sysfs_attrs,
};

static int xdpe132g5c_identify(struct i2c_client *client, struct pmbus_driver_info *info)
{
    u8 vout_params;
    int ret, i, retry;

    /* Read the register with VOUT scaling value.*/
    for (i = 0; i < XDPE132G5C_PAGE_NUM; i++) {
        for (retry = 0; retry < RETRY_TIME; retry++) {
            ret = wb_pmbus_read_byte_data(client, i, PMBUS_VOUT_MODE);
            if (ret < 0 || ret == 0xff) {
                msleep(5);
                continue;
            } else {
                break;
            }
        }
        if (ret < 0) {
            return ret;
        }

        switch (ret >> 5) {
        case 0: /* linear mode      */
            if (info->format[PSC_VOLTAGE_OUT] != linear) {
                return -ENODEV;
            }
            break;
        case 1: /* VID mode         */
            if (info->format[PSC_VOLTAGE_OUT] != vid) {
                return -ENODEV;
            }
            vout_params = ret & GENMASK(4, 0);
            switch (vout_params) {
            case XDPE132G5C_PROT_VR13_10MV:
            case XDPE132G5C_PROT_VR12_5_10MV:
                info->vrm_version[i] = vr13;
                break;
            case XDPE132G5C_PROT_VR13_5MV:
            case XDPE132G5C_PROT_VR12_5MV:
            case XDPE132G5C_PROT_IMVP8_5MV:
                info->vrm_version[i] = vr12;
                break;
            case XDPE132G5C_PROT_IMVP9_10MV:
                info->vrm_version[i] = imvp9;
                break;
            default:
                return -EINVAL;
            }
            break;
        case 2: /* direct mode      */
            if (info->format[PSC_VOLTAGE_OUT] != direct) {
                return -ENODEV;
            }
            break;
        default:
            return -ENODEV;
        }
    }

    return 0;
}

static int xdpe_chip_read_word_data(struct i2c_client *client, int page, int phase, int reg)
{
    const struct pmbus_driver_info *info = wb_pmbus_get_driver_info(client);
    const struct xdpe_chip_data *data = to_xdpe_chip_data(info);
    int ret = 0;

    /* Virtual PMBUS Command not supported */
    if (reg >= PMBUS_VIRT_BASE) {
        DEBUG_ERROR("reg:0x%x is invalid\n", reg);
        ret = -ENXIO;
        return ret;
    }
    switch (reg) {
    case PMBUS_READ_VOUT:
        ret = wb_pmbus_read_word_data(client, page, phase, reg);
        DEBUG_INFO("READ_VOUT Value:%d %d %d\n", ret, data->vout_multiplier[0],
                    data->vout_multiplier[1]);
        ret = ((ret * data->vout_multiplier[0])/data->vout_multiplier[1]);
        break;
    default:
        ret = wb_pmbus_read_word_data(client, page, phase, reg);
        break;
    }
    return ret;
}


static struct pmbus_driver_info xdpe1x2g5_info[] = {
    [XDPE132G5C] = {
        .pages = XDPE132G5C_PAGE_NUM,
        .format[PSC_VOLTAGE_IN] = linear,
        .format[PSC_VOLTAGE_OUT] = linear,
        .format[PSC_TEMPERATURE] = linear,
        .format[PSC_CURRENT_IN] = linear,
        .format[PSC_CURRENT_OUT] = linear,
        .format[PSC_POWER] = linear,
        .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_IIN | PMBUS_HAVE_PIN
        | PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_TEMP
        | PMBUS_HAVE_STATUS_TEMP
        | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
        | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_POUT,
        .func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_IIN | PMBUS_HAVE_PIN
        | PMBUS_HAVE_STATUS_INPUT | PMBUS_HAVE_TEMP
        | PMBUS_HAVE_STATUS_TEMP
        | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT
        | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT | PMBUS_HAVE_POUT,
        .groups = xdpe132g5_attribute_groups,
        .identify = xdpe132g5c_identify,
    },
    [XDPE1A2G5B] = {
        .pages = XDPE132G5C_PAGE_NUM,
        .read_word_data = xdpe_chip_read_word_data,
        .format[PSC_VOLTAGE_IN] = linear,
        .format[PSC_VOLTAGE_OUT] = linear,
        .format[PSC_TEMPERATURE] = linear,
        .format[PSC_CURRENT_IN] = linear,
        .format[PSC_CURRENT_OUT] = linear,
        .format[PSC_POWER] = linear,
        .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
            PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
            PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
            PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
        .func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
            PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
            PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
            PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
        .groups = xdpe132g5_attribute_groups,
    },
    [XDPE19284C] = {
        .pages = 2,
        .read_word_data = xdpe_chip_read_word_data,
        .format[PSC_VOLTAGE_IN] = linear,
        .format[PSC_VOLTAGE_OUT] = linear,
        .format[PSC_TEMPERATURE] = linear,
        .format[PSC_CURRENT_IN] = linear,
        .format[PSC_CURRENT_OUT] = linear,
        .format[PSC_POWER] = linear,
        .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
            PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
            PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
            PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
        .func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
            PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
            PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
            PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
        .groups = xdpe132g5_attribute_groups,
    },
    [XDPE192C4B] = {
        .pages = 2,
        .read_word_data = xdpe_chip_read_word_data,
        .format[PSC_VOLTAGE_IN] = linear,
        .format[PSC_VOLTAGE_OUT] = linear,
        .format[PSC_TEMPERATURE] = linear,
        .format[PSC_CURRENT_IN] = linear,
        .format[PSC_CURRENT_OUT] = linear,
        .format[PSC_POWER] = linear,
        .func[0] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
            PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
            PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
            PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
        .func[1] = PMBUS_HAVE_VIN | PMBUS_HAVE_VOUT | PMBUS_HAVE_STATUS_VOUT |
            PMBUS_HAVE_IIN | PMBUS_HAVE_IOUT | PMBUS_HAVE_STATUS_IOUT |
            PMBUS_HAVE_TEMP | PMBUS_HAVE_TEMP2 | PMBUS_HAVE_STATUS_TEMP |
            PMBUS_HAVE_POUT | PMBUS_HAVE_PIN | PMBUS_HAVE_STATUS_INPUT,
        .groups = xdpe132g5_attribute_groups,
    },
};

static int xdpe132g5c_probe(struct i2c_client *client,
             const struct i2c_device_id *id)
{
    struct xdpe_chip_data *chip_data;
    struct pmbus_data *data;
    int ret;

    /* 1. alloc mem and do init */
    chip_data = devm_kzalloc(&client->dev, sizeof(*chip_data), GFP_KERNEL);
    if (!chip_data) {
        dev_err(&client->dev, "devm_kzalloc fail\n");
        return -ENOMEM;
    }

    chip_data->id = id->driver_data;
    memcpy(&chip_data->info, &xdpe1x2g5_info[id->driver_data], sizeof(chip_data->info));

    ret = of_property_read_u32_array(client->dev.of_node, "vout_multiplier",
                                     chip_data->vout_multiplier, ARRAY_SIZE(chip_data->vout_multiplier));
    DEBUG_INFO("vout_multipplier:%d %d, ret:%d\n", 
               chip_data->vout_multiplier[0], chip_data->vout_multiplier[1], ret);
    if (ret != 0) {
        /* get fail use default val */
        chip_data->vout_multiplier[0] = 0x01;
        chip_data->vout_multiplier[1] = 0x01;
    }

    if (chip_data->vout_multiplier[0] == 0 || chip_data->vout_multiplier[1] == 0) {
        dev_err(&client->dev, "vout_multiplier have 0. its invalid\n");
        return -EINVAL;
    }

    /* 2. pmbus do probe */
    ret = wb_pmbus_do_probe(client, &chip_data->info);
    if (ret != 0) {
        dev_info(&client->dev, "wb_pmbus_do_probe failed, ret: %d.\n", ret);
        return ret;
    }

    /* 3. init clinet data */
    data = i2c_get_clientdata(client);
    data->pmbus_info_array = dfx_infos;
    data->pmbus_info_array_size = ARRAY_SIZE(dfx_infos);

    /* 4. create sysfs group */
    ret = sysfs_create_group(&client->dev.kobj, &xdpe132g5c_sysfs_attrs_group);
    if (ret != 0) {
        dev_info(&client->dev, "Failed to create xdpe132g5c_sysfs_attrs_group, ret: %d.\n", ret);
        wb_pmbus_do_remove(client);
        return ret;
    }

    return 0;
}

static void xdpe132g5c_remove(struct i2c_client *client)
{
    sysfs_remove_group(&client->dev.kobj, &xdpe132g5c_sysfs_attrs_group);
    (void)wb_pmbus_do_remove(client);
    return;
}

static const struct i2c_device_id xdpe132g5c_id[] = {
    {"wb_xdpe132g5c_pmbus", XDPE132G5C},
    {"wb_xdpe1a2g5b_pmbus", XDPE1A2G5B},
    {"wb_xdpe19284c_pmbus", XDPE19284C},
    {"wb_xdpe192c4b_pmbus", XDPE192C4B},
    {}
};

MODULE_DEVICE_TABLE(i2c, xdpe132g5c_id);

static const struct of_device_id __maybe_unused xdpe132g5c_of_match[] = {
    {.compatible = "infineon,wb_xdpe132g5c_pmbus", .data = (void *)XDPE132G5C},
    {.compatible = "infineon,wb_xdpe1a2g5b_pmbus", .data = (void *)XDPE1A2G5B},
    {.compatible = "infineon,wb_xdpe19284c_pmbus", .data = (void *)XDPE19284C},
    {.compatible = "infineon,wb_xdpe192c4b_pmbus", .data = (void *)XDPE192C4B},
    {}
};
MODULE_DEVICE_TABLE(of, xdpe132g5c_of_match);

static struct i2c_driver xdpe132g5c_driver = {
    .driver = {
        .name = "wb_xdpe132g5c_pmbus",
        .of_match_table = of_match_ptr(xdpe132g5c_of_match),
    },
    .probe = xdpe132g5c_probe,
    .remove = xdpe132g5c_remove,
    .id_table = xdpe132g5c_id,
};

module_i2c_driver(xdpe132g5c_driver);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("PMBus driver for Infineon XDPE132g5 family");
MODULE_LICENSE("GPL");
