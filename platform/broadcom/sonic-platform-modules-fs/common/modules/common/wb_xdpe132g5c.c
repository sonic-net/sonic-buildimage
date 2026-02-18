/*
 * xdpe132g5c_i2c_drv.c
 *
 * This module create sysfs to set AVS and create hwmon to get out power
 * through xdpe132g5c I2C address.
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2021-09-17                  Initial version
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/miscdevice.h>
#include <wb_bsp_kernel_debug.h>

#define WB_I2C_RETRY_SLEEP_TIME          (10000)   /* 10ms */
#define WB_I2C_RETRY_TIME                (10)
#define WB_XDPE_I2C_PAGE_ADDR            (0xff)
#define WB_XDPE_I2C_VOUT_MODE            (0x40)
#define WB_XDPE_I2C_VOUT_COMMAND         (0x42)
#define WB_XDPE_I2C_VOUT_PAGE            (0x06)
#define WB_XDPE_VOUT_MAX_THRESHOLD       ((0xFFFFLL * 1000LL * 1000LL) / (256))
#define WB_XDPE_VOUT_MIN_THRESHOLD       (0)

#define WB_XDPE_VERSION_PAGE             (0x00)
#define WB_XDPE_VERSION_REG              (0x7a)

#define WB_XDPE_PHASE_CURR_PAGE          (0)
#define WB_XDPE_PHASE_SELECT_REG         (0xe6)
#define WB_XDPE_PHASE_SELECT_VALUE       (0x30)         /* bit[6:4] is 3 */
#define WB_XDPE_PHASE_SELECT_VALUE_MASK  (0xFF8F)       /* clear bit[6:4] */

/* bit[7:0] Bit 7 is the sign bit */
#define XDPE132G5_PHASE_CURR_REG_TO_VALUE(reg_val)   (((s16)(((reg_val) & 0xff) << 8)) >> 8)

#define WB_XDPE_PHASE1_CURR_REG          (0xac)
#define WB_XDPE_PHASE2_CURR_REG          (0xae)
#define WB_XDPE_PHASE3_CURR_REG          (0xb0)
#define WB_XDPE_PHASE4_CURR_REG          (0xb2)
#define WB_XDPE_PHASE5_CURR_REG          (0xb4)
#define WB_XDPE_PHASE6_CURR_REG          (0xb6)
#define WB_XDPE_PHASE7_CURR_REG          (0xb8)
#define WB_XDPE_PHASE8_CURR_REG          (0xba)
#define WB_XDPE_PHASE9_CURR_REG          (0xbc)
#define WB_XDPE_PHASE10_CURR_REG         (0xbe)
#define WB_XDPE_PHASE11_CURR_REG         (0xc0)
#define WB_XDPE_PHASE12_CURR_REG         (0xc2)
#define WB_XDPE_PHASE13_CURR_REG         (0xc4)
#define WB_XDPE_PHASE14_CURR_REG         (0xc6)
#define WB_XDPE_PHASE15_CURR_REG         (0xc8)
#define WB_XDPE_PHASE16_CURR_REG         (0xca)

#define WB_XDPE_MAX_MISC_DEV_NUM         (256)
#define WB_XDPE_BUF_LEN_MAX              (256)

#define WIDTH_1Byte                      (1)
#define WIDTH_2Byte                      (2)

/* Read two bytes of the XDPE132 firmware CRC value from register 0xA6 */
#define WB_XDPE_CHIP_CRC_COMMAND         (0xa6)
/* Read four bytes of date information (year-month-day) from registers 0x76, 0x77, 0x78, and 0x79, representing the format xxxx-yy-zz */
#define WB_XDPE_CHIP_VER_DATE_COMMAND    (0x76)
#define WB_XDPE_CHIP_VER_DATE_LEN        (4)
/* Read the remaining NVM upgrade count (up to a maximum of 25) from registers 0xA0 and 0xA2. */
#define WB_XDPE_CHIP_NVM_HIGH_COMMAND    (0xa0)
#define WB_XDPE_CHIP_NVM_LOW_COMMAND     (0xa2)
#define WB_XDPE_CHIP_NVM_TIME_MAX        (25)
/* Device read/write enable flag */
#define WB_XDPE_DEV_AVAILABLE_FLAG       (1)
#define WB_XDPE_DEV_UNAVAILABLE_FLAG     (0)

/* Character device read/write interface, macros related to offset parsing */
/* Does the input parameter offset indicate the need for paging? Bit 16 set to 1 means paging is required. */
#define WB_XDPE_DEV_RW_SET_PAGE_MASK     (0x10000)
/* Does the input parameter offset indicate the starting address of the page to be switched? Bits 8 to 15 represent the page address information. */
#define WB_XDPE_DEV_RW_PAGE_INFO_BIT     (8)

/* Firmware upgrade command to write to the ROM register address and value, i.e., writing 0x3F42 to register 0xE8 on page 0. */
#define WB_XDPE_STORE_TO_NVM_COMMAND     (0xe8)
#define WB_XDPE_STORE_TO_NVM_VALUE       (0x3f42)

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);
struct xdpe_data {
    struct i2c_client *client;
    struct device *hwmon_dev;
    struct mutex update_lock;
    long long vout_max;
    long long vout_min;
    u32 dev_available;
    struct device *dev;
    struct miscdevice misc;
    char dev_name[WB_XDPE_BUF_LEN_MAX];
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

static struct xdpe_data* xdpe132g5c_dev_arry[WB_XDPE_MAX_MISC_DEV_NUM];

/* Determine whether the device is accessible; return true if accessible, otherwise return false. */
static bool is_xdpe_dev_available(struct device *dev)
{
    struct i2c_client *client;
    struct xdpe_data *data;

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);
    if (data == NULL) {
        DEBUG_ERROR("is_xdpe_dev_available param error\n");
        return false;
    }

    if (data->dev_available) {
        return true;
    }

    return false;
}

static s32 wb_i2c_smbus_read_byte_data(const struct i2c_client *client, u8 command)
{
    int i;
    s32 ret;

    for (i = 0; i < WB_I2C_RETRY_TIME; i++) {
        ret = i2c_smbus_read_byte_data(client, command);
        if (ret >= 0) {
            return ret;
        }
        usleep_range(WB_I2C_RETRY_SLEEP_TIME, WB_I2C_RETRY_SLEEP_TIME + 1);
    }
    return ret;
}

static s32 wb_i2c_smbus_write_byte_data(const struct i2c_client *client, u8 command, u8 value)
{
    int i;
    s32 ret;

    for (i = 0; i < WB_I2C_RETRY_TIME; i++) {
        ret = i2c_smbus_write_byte_data(client, command, value);
        if (ret >= 0) {
            return ret;
        }
        usleep_range(WB_I2C_RETRY_SLEEP_TIME, WB_I2C_RETRY_SLEEP_TIME + 1);
    }
    return ret;
}

static s32 wb_i2c_smbus_read_word_data(const struct i2c_client *client, u8 command)
{
    int i;
    s32 ret;

    for (i = 0; i < WB_I2C_RETRY_TIME; i++) {
        ret = i2c_smbus_read_word_data(client, command);
        if (ret >= 0) {
            return ret;
        }
        usleep_range(WB_I2C_RETRY_SLEEP_TIME, WB_I2C_RETRY_SLEEP_TIME + 1);
    }
    return ret;
}

static s32 wb_i2c_smbus_write_word_data(const struct i2c_client *client, u8 command,
               u16 value)
{
    int i;
    s32 ret;

    for (i = 0; i < WB_I2C_RETRY_TIME; i++) {
        ret = i2c_smbus_write_word_data(client, command, value);
        if (ret >= 0) {
            return ret;
        }
        usleep_range(WB_I2C_RETRY_SLEEP_TIME, WB_I2C_RETRY_SLEEP_TIME + 1);
    }
    return ret;
}

static long calc_power_linear11_data(int data)
{
    s16 exponent;
    s32 mantissa;
    long val;

    exponent = ((s16)data) >> 11;
    mantissa = ((s16)((data & 0x7ff) << 5)) >> 5;
    val = mantissa;
    val = val * 1000L * 1000L;

    if (exponent >= 0) {
        val <<= exponent;
    } else {
        val >>= -exponent;
    }
    return val;
}

static int read_xdpe_power_value(const struct i2c_client *client, u8 page, u8 reg, long *value)
{
    int ret, data;

    ret = wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, page);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe page%u failed, ret: %d\n", client->adapter->nr,
            client->addr, page, ret);
        return ret;
    }
    data = wb_i2c_smbus_read_word_data(client, reg);
    if (data < 0) {
        DEBUG_ERROR("%d-%04x: read xdpe page%u reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, page, reg, data);
        return data;
    }
    *value = calc_power_linear11_data(data);
    DEBUG_VERBOSE("%d-%04x: page%u reg: 0x%x rd_data: 0x%x, decode linear11 value: %ld\n",
        client->adapter->nr, client->addr, page, reg, data, *value);
    return 0;
}

static ssize_t xdpe_power_value_show(struct device *dev, struct device_attribute *da,
                   char *buf)
{
    int ret, ori_page;
    u16 sensor_h, sensor_l;
    u8 page, reg;
    struct sensor_device_attribute *attr;
    struct i2c_client *client;
    struct xdpe_data *data;
    long value1, value2;

    if (is_xdpe_dev_available(dev) == false) {
        DEBUG_VERBOSE("xdpe_power_value_show return failed due to dev unavailable\n");
        return -EINVAL;
    }

    data = dev_get_drvdata(dev);
    client = data->client;
    attr = to_sensor_dev_attr(da);
    sensor_h = ((attr->index) >> 16) & 0xffff;
    sensor_l = (attr->index) & 0xffff;

    mutex_lock(&data->update_lock);

    ori_page = wb_i2c_smbus_read_byte_data(client, WB_XDPE_I2C_PAGE_ADDR);
    if (ori_page < 0) {
        DEBUG_ERROR("%d-%04x: read xdpe origin page failed, ret: %d\n", client->adapter->nr,
            client->addr, ori_page);
        mutex_unlock(&data->update_lock);
        return ori_page;
    }
    value1 = 0;
    value2 = 0;

    if (sensor_h) {
        page = (sensor_h >> 8) & 0xff;
        reg = sensor_h & 0xff;
        ret = read_xdpe_power_value(client, page, reg, &value1);
        if (ret < 0) {
            DEBUG_ERROR("%d-%04x: read xdpe sensor high sensor page%u reg: 0x%x failed, ret: %d\n",
                client->adapter->nr, client->addr, page, reg, ret);
            goto error;
        }
        DEBUG_VERBOSE("%d-%04x: read xdpe sensor high sensor page%u reg: 0x%x success, value: %ld\n",
            client->adapter->nr, client->addr, page, reg, value1);
    }

    page = (sensor_l >> 8) & 0xff;
    reg = sensor_l & 0xff;
    ret = read_xdpe_power_value(client, page, reg, &value2);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read xdpe sensor low sensor page%u reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, page, reg, ret);
        goto error;
    }
    DEBUG_VERBOSE("%d-%04x: read xdpe sensor low sensor page%u reg: 0x%x success, value: %ld\n",
        client->adapter->nr, client->addr, page, reg, value2);

    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);
    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%ld\n", value1 + value2);
error:
    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);
    mutex_unlock(&data->update_lock);
    return ret;
}

static int xdpe_get_vout_precision(const struct i2c_client *client, int *vout_precision)
{
    int i, vout_mode, a_size;

    vout_mode = wb_i2c_smbus_read_byte_data(client, WB_XDPE_I2C_VOUT_MODE);
    if (vout_mode < 0) {
        DEBUG_ERROR("%d-%04x: read xdpe vout mode reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, WB_XDPE_I2C_VOUT_MODE, vout_mode);
        return vout_mode;
    }

    a_size = ARRAY_SIZE(g_xdpe_vout_group);
    for (i = 0; i < a_size; i++) {
        if (g_xdpe_vout_group[i].vout_mode == vout_mode) {
            *vout_precision = g_xdpe_vout_group[i].vout_precision;
            DEBUG_VERBOSE("%d-%04x: match, vout mode: 0x%x, precision: %d\n",
                client->adapter->nr, client->addr, vout_mode, *vout_precision);
            break;
        }
    }
    if (i == a_size) {
        DEBUG_ERROR("%d-%04x: invalid vout mode: 0x%x\n",client->adapter->nr, client->addr,
            vout_mode);
        return -EINVAL;
    }
    return 0;
}

static ssize_t xdpe_avs_vout_show(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret, ori_page, vout_cmd, vout_precision;
    struct i2c_client *client;
    struct xdpe_data *data;
    long long vout;

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);

    if (is_xdpe_dev_available(dev) == false) {
        DEBUG_VERBOSE("xdpe_power_value_show return failed due to dev unavailable\n");
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);

    ori_page = wb_i2c_smbus_read_byte_data(client, WB_XDPE_I2C_PAGE_ADDR);
    if (ori_page < 0) {
        DEBUG_ERROR("%d-%04x: read xdpe origin page failed, ret: %d\n", client->adapter->nr,
            client->addr, ori_page);
        mutex_unlock(&data->update_lock);
        return ori_page;
    }

    ret = wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, WB_XDPE_I2C_VOUT_PAGE);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe avs vout page%u failed, ret: %d\n", client->adapter->nr,
            client->addr, WB_XDPE_I2C_VOUT_PAGE, ret);
        goto error;
    }

    ret = xdpe_get_vout_precision(client, &vout_precision);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: get xdpe avs vout precision failed, ret: %d\n",
            client->adapter->nr, client->addr, ret);
        goto error;
    }

    vout_cmd = wb_i2c_smbus_read_word_data(client, WB_XDPE_I2C_VOUT_COMMAND);
    if (vout_cmd < 0) {
        ret = vout_cmd;
        DEBUG_ERROR("%d-%04x: read xdpe vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, WB_XDPE_I2C_VOUT_COMMAND, ret);
        goto error;
    }

    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);
    mutex_unlock(&data->update_lock);

    vout = div_s64(vout_cmd * 1000LL * 1000LL, vout_precision);
    DEBUG_VERBOSE("%d-%04x: vout: %lld, vout_cmd: 0x%x, precision: %d\n", client->adapter->nr,
        client->addr, vout, vout_cmd, vout_precision);
    return snprintf(buf, PAGE_SIZE, "%lld\n", vout);
error:
    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);
    mutex_unlock(&data->update_lock);
    return ret;
}

static ssize_t xdpe_avs_vout_store(struct device *dev, struct device_attribute *da,
                   const char *buf, size_t count)
{
    int ret, ori_page, vout_cmd, vout_cmd_set, vout_precision;
    struct i2c_client *client;
    struct xdpe_data *data;
    long long vout, vout_max, vout_min;

    if (is_xdpe_dev_available(dev) == false) {
        DEBUG_VERBOSE("xdpe_power_value_show return failed due to dev unavailable\n");
        return -EINVAL;
    }

    client = to_i2c_client(dev);
    vout = 0;
    ret = kstrtoll(buf, 0, &vout);
    if (ret) {
        DEBUG_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    if (vout <= 0) {
        DEBUG_ERROR("%d-%04x: invalid value: %lld \n", client->adapter->nr, client->addr, vout);
        return -EINVAL;
    }

    data = i2c_get_clientdata(client);
    vout_max = data->vout_max;
    vout_min = data->vout_min;
    if ((vout > vout_max) || (vout < vout_min)) {
        DEBUG_ERROR("%d-%04x: vout value: %lld, out of range [%lld, %lld] \n", client->adapter->nr,
            client->addr, vout, vout_min, vout_max);
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);

    ori_page = wb_i2c_smbus_read_byte_data(client, WB_XDPE_I2C_PAGE_ADDR);
    if (ori_page < 0) {
        DEBUG_ERROR("%d-%04x: read xdpe origin page failed, ret: %d\n", client->adapter->nr,
            client->addr, ori_page);
        mutex_unlock(&data->update_lock);
        return ori_page;
    }

    ret = wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, WB_XDPE_I2C_VOUT_PAGE);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe avs vout page%u failed, ret: %d\n", client->adapter->nr,
            client->addr, WB_XDPE_I2C_VOUT_PAGE, ret);
        goto error;
    }

    ret = xdpe_get_vout_precision(client, &vout_precision);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: get xdpe avs vout precision failed, ret: %d\n",
            client->adapter->nr, client->addr, ret);
        goto error;
    }

    vout_cmd_set = div_s64(vout * vout_precision, 1000L * 1000L);
    if (vout_cmd_set > 0xffff) {
        DEBUG_ERROR("%d-%04x: invalid value, vout %lld, vout_precision: %d, vout_cmd_set: 0x%x\n",
            client->adapter->nr, client->addr, vout, vout_precision, vout_cmd_set);
        ret = -EINVAL;
        goto error;
    }
    ret = wb_i2c_smbus_write_word_data(client, WB_XDPE_I2C_VOUT_COMMAND, vout_cmd_set);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe vout cmd reg: 0x%x,  value: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, WB_XDPE_I2C_VOUT_COMMAND, vout_cmd_set, ret);
        goto error;
    }

    vout_cmd = wb_i2c_smbus_read_word_data(client, WB_XDPE_I2C_VOUT_COMMAND);
    if (vout_cmd < 0) {
        ret = vout_cmd;
        DEBUG_ERROR("%d-%04x: read xdpe vout command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, WB_XDPE_I2C_VOUT_COMMAND, ret);
        goto error;
    }
    if (vout_cmd != vout_cmd_set) {
        ret = -EIO;
        DEBUG_ERROR("%d-%04x: vout cmd value check error, vout cmd read: 0x%x, vout cmd set: 0x%x\n",
            client->adapter->nr, client->addr, vout_cmd, vout_cmd_set);
        goto error;

    }

    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);
    mutex_unlock(&data->update_lock);
    DEBUG_VERBOSE("%d-%04x: set vout cmd success, vout %lld, vout_precision: %d, vout_cmd_set: 0x%x\n",
        client->adapter->nr, client->addr, vout, vout_precision, vout_cmd_set);
    return count;
error:
    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);
    mutex_unlock(&data->update_lock);
    return ret;
}

static ssize_t xdpe_avs_vout_max_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client;
    struct xdpe_data *data;
    long long vout_max;

    if (is_xdpe_dev_available(dev) == false) {
        DEBUG_VERBOSE("xdpe_power_value_show return failed due to dev unavailable\n");
        return -EINVAL;
    }

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);
    vout_max = data->vout_max;
    return snprintf(buf, PAGE_SIZE, "%lld\n", vout_max);
}

static ssize_t xdpe_avs_vout_max_store(struct device *dev, struct device_attribute *da,
                   const char *buf, size_t count)
{
    int ret;
    struct i2c_client *client;
    struct xdpe_data *data;
    long long vout_max;

    if (is_xdpe_dev_available(dev) == false) {
        DEBUG_VERBOSE("xdpe_power_value_show return failed due to dev unavailable\n");
        return -EINVAL;
    }

    client = to_i2c_client(dev);
    vout_max = 0;
    ret = kstrtoll(buf, 10, &vout_max);
    if (ret) {
        DEBUG_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }
    DEBUG_VERBOSE("%d-%04x: vout max threshold: %lld", client->adapter->nr, client->addr,
        vout_max);
    data = i2c_get_clientdata(client);
    data->vout_max = vout_max;
    return count;
}

static ssize_t xdpe_avs_vout_min_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client;
    struct xdpe_data *data;
    long long vout_min;

    if (is_xdpe_dev_available(dev) == false) {
        DEBUG_VERBOSE("xdpe_power_value_show return failed due to dev unavailable\n");
        return -EINVAL;
    }

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);
    vout_min = data->vout_min;
    return snprintf(buf, PAGE_SIZE, "%lld\n", vout_min);
}

static ssize_t xdpe_avs_vout_min_store(struct device *dev, struct device_attribute *da,
                   const char *buf, size_t count)
{
    int ret;
    struct i2c_client *client;
    struct xdpe_data *data;
    long long vout_min;

    if (is_xdpe_dev_available(dev) == false) {
        DEBUG_VERBOSE("xdpe_power_value_show return failed due to dev unavailable\n");
        return -EINVAL;
    }

    client = to_i2c_client(dev);
    vout_min = 0;
    ret = kstrtoll(buf, 10, &vout_min);
    if (ret) {
        DEBUG_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }
    DEBUG_VERBOSE("%d-%04x: vout min threshold: %lld", client->adapter->nr, client->addr,
        vout_min);
    data = i2c_get_clientdata(client);
    data->vout_min = vout_min;
    return count;
}

static ssize_t xdpe_word_data_show(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret, ori_page, word_data;
    struct i2c_client *client;
    struct xdpe_data *data;
    u8 page = to_sensor_dev_attr_2(da)->nr;
    u8 reg = to_sensor_dev_attr_2(da)->index;

    if (is_xdpe_dev_available(dev) == false) {
        DEBUG_VERBOSE("xdpe_power_value_show return failed due to dev unavailable\n");
        return -EINVAL;
    }

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);

    mutex_lock(&data->update_lock);

    ori_page = wb_i2c_smbus_read_byte_data(client, WB_XDPE_I2C_PAGE_ADDR);
    if (ori_page < 0) {
        DEBUG_ERROR("%d-%04x: read xdpe origin page failed, ret: %d\n", client->adapter->nr,
            client->addr, ori_page);
        mutex_unlock(&data->update_lock);
        return ori_page;
    }

    ret = wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, page);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe page%u failed, ret: %d\n", client->adapter->nr,
            client->addr, page, ret);
        goto error;
    }

    word_data = wb_i2c_smbus_read_word_data(client, reg);
    if (word_data < 0) {
        ret = word_data;
        DEBUG_ERROR("%d-%04x: read xdpe word data reg failed, page%u, reg: 0x%x, ret: %d\n",
            client->adapter->nr, client->addr, page, reg, ret);
        goto error;
    }

    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);
    mutex_unlock(&data->update_lock);

    DEBUG_VERBOSE("%d-%04x: read xdpe word data success, page%u, reg: 0x%x, value: 0x%04x\n", client->adapter->nr,
        client->addr, page, reg, word_data);
    return snprintf(buf, PAGE_SIZE, "0x%04x\n", word_data);
error:
    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);
    mutex_unlock(&data->update_lock);
    return ret;
}

static ssize_t xdpe_phase_curr_show(struct device *dev, struct device_attribute *da,
                   char *buf)
{
    int ret, ori_page, ori_reg_value, word_data;
    struct sensor_device_attribute *attr;
    struct i2c_client *client;
    struct xdpe_data *data;
    int value;

    if (is_xdpe_dev_available(dev) == false) {
        DEBUG_VERBOSE("xdpe_power_value_show return failed due to dev unavailable\n");
        return -EINVAL;
    }

    data = dev_get_drvdata(dev);
    client = data->client;
    attr = to_sensor_dev_attr(da);

    mutex_lock(&data->update_lock);

    ori_page = wb_i2c_smbus_read_byte_data(client, WB_XDPE_I2C_PAGE_ADDR);
    if (ori_page < 0) {
        ret = ori_page;
        DEBUG_ERROR("%d-%04x: read xdpe origin page failed, ret: %d\n", client->adapter->nr,
            client->addr, ori_page);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    ret = wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, WB_XDPE_PHASE_CURR_PAGE);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe page%u failed, ret: %d\n", client->adapter->nr,
            client->addr, WB_XDPE_PHASE_CURR_PAGE, ret);
        goto error1;
    }

    ori_reg_value = wb_i2c_smbus_read_word_data(client, WB_XDPE_PHASE_SELECT_REG);
    if (ori_reg_value < 0) {
        ret = ori_reg_value;
        DEBUG_ERROR("%d-%04x: read xdpe word data reg failed, reg: 0x%x, ret: %d\n",
            client->adapter->nr, client->addr, WB_XDPE_PHASE_SELECT_REG, ret);
        goto error1;
    }

    word_data = (ori_reg_value & WB_XDPE_PHASE_SELECT_VALUE_MASK) | WB_XDPE_PHASE_SELECT_VALUE;
    ret = wb_i2c_smbus_write_word_data(client, WB_XDPE_PHASE_SELECT_REG, word_data);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe phase select reg: 0x%x,  value: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, WB_XDPE_PHASE_SELECT_REG, word_data, ret);
        goto error1;
    }

    word_data = wb_i2c_smbus_read_word_data(client, attr->index);
    if (word_data < 0) {
        ret = word_data;
        DEBUG_ERROR("%d-%04x: read xdpe word data reg failed, reg: 0x%x, ret: %d\n",
            client->adapter->nr, client->addr, attr->index, ret);
        goto error2;
    }

    wb_i2c_smbus_write_word_data(client, WB_XDPE_PHASE_SELECT_REG, ori_reg_value);
    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);

    mutex_unlock(&data->update_lock);
    /* unit mA */
    value = XDPE132G5_PHASE_CURR_REG_TO_VALUE(word_data) * 1000;
    return snprintf(buf, PAGE_SIZE, "%d\n", value);

error2:
    wb_i2c_smbus_write_word_data(client, WB_XDPE_PHASE_SELECT_REG, ori_reg_value);
error1:
    wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, ori_page);
    mutex_unlock(&data->update_lock);
    return ret;
}

/* Read two bytes of the XDPE132 firmware CRC value from register 0xA6. */
static ssize_t xdpe_show_chip_crc(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret, crc_val;
    struct i2c_client *client;
    struct xdpe_data *data;

    if (is_xdpe_dev_available(dev) == false) {
        DEBUG_VERBOSE("xdpe_power_value_show return failed due to dev unavailable\n");
        return -EINVAL;
    }

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);
    if (data == NULL) {
        DEBUG_ERROR("xdpe_show_chip_crc param error\n");
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);

    /* Switch to PAGE 0. */
    ret = wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, WB_XDPE_PHASE_CURR_PAGE);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe avs vout page%u failed, ret: %d\n", client->adapter->nr,
            client->addr, WB_XDPE_PHASE_CURR_PAGE, ret);
        goto error;
    }

    /* read crc */
    crc_val = wb_i2c_smbus_read_word_data(client, WB_XDPE_CHIP_CRC_COMMAND);
    if (crc_val < 0) {
        ret = crc_val;
        DEBUG_ERROR("%d-%04x: read xdpe crc command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, WB_XDPE_I2C_VOUT_COMMAND, ret);
        goto error;
    }

    mutex_unlock(&data->update_lock);

    DEBUG_VERBOSE("%d-%04x: offset: 0x%x, crc: 0x%x\n",
        client->adapter->nr, client->addr, WB_XDPE_CHIP_CRC_COMMAND, crc_val);

    return snprintf(buf, WB_XDPE_BUF_LEN_MAX, "0x%x\n", crc_val);

error:
    mutex_unlock(&data->update_lock);
    return ret;
}

/* Read four bytes of date information (year-month-day) from registers 0x76, 0x77, 0x78, and 0x79, representing the format xxxx-yy-zz. */
static ssize_t xdpe_show_chip_ver_date(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client;
    struct xdpe_data *data;
    int date_ver[WB_XDPE_CHIP_VER_DATE_LEN];
    int ret, i, date_tmp;

    if (is_xdpe_dev_available(dev) == false) {
        DEBUG_VERBOSE("xdpe_power_value_show return failed due to dev unavailable\n");
        return -EINVAL;
    }

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);
    if (data == NULL) {
        DEBUG_ERROR("xdpe_show_chip_ver_date param error\n");
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);

    /* Switch to PAGE 0. */
    ret = wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, WB_XDPE_PHASE_CURR_PAGE);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe avs vout page%u failed, ret: %d\n", client->adapter->nr,
            client->addr, WB_XDPE_PHASE_CURR_PAGE, ret);
        goto error;
    }

    memset(date_ver, 0, sizeof(date_ver));
    for (i = 0; i < WB_XDPE_CHIP_VER_DATE_LEN; i++) {
        date_tmp = wb_i2c_smbus_read_byte_data(client, WB_XDPE_CHIP_VER_DATE_COMMAND + i);
        if (date_tmp < 0) {
            ret = date_tmp;
            DEBUG_ERROR("%d-%04x: read xdpe crc command reg: 0x%x failed, ret: %d\n",
                client->adapter->nr, client->addr, (WB_XDPE_I2C_VOUT_COMMAND + i), ret);
            goto error;
        }
        date_ver[i] = date_tmp & 0xff;
        DEBUG_VERBOSE("%d-%04x: offset: 0x%x, crc: 0x%x\n",
            client->adapter->nr, client->addr, (WB_XDPE_I2C_VOUT_COMMAND + i), date_ver[i]);
    }
    mutex_unlock(&data->update_lock);

    return snprintf(buf, WB_XDPE_BUF_LEN_MAX, "%02x%02x-%02x-%02x\n", date_ver[0], date_ver[1], date_ver[2], date_ver[3]);

error:
    mutex_unlock(&data->update_lock);
    return ret;
}

/* Read the remaining NVM upgrade count (up to a maximum of 25) from registers 0xA0 and 0xA2. */
static ssize_t xdpe_show_chip_nvm_times(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client;
    struct xdpe_data *data;
    int nvm_val_high, nvm_val_low, nvm_val, nvm_time;
    int ret, i;

    if (is_xdpe_dev_available(dev) == false) {
        DEBUG_VERBOSE("xdpe_power_value_show return failed due to dev unavailable\n");
        return -EINVAL;
    }

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);
    if (data == NULL) {
        DEBUG_ERROR("xdpe_show_chip_nvm_times param error\n");
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);

    /* Switch to PAGE 0. */
    ret = wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, WB_XDPE_PHASE_CURR_PAGE);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe avs vout page%u failed, ret: %d\n", client->adapter->nr,
            client->addr, WB_XDPE_PHASE_CURR_PAGE, ret);
        goto error;
    }

    nvm_val_high = wb_i2c_smbus_read_word_data(client, WB_XDPE_CHIP_NVM_HIGH_COMMAND);
    if (nvm_val_high < 0) {
        ret = nvm_val_high;
        DEBUG_ERROR("%d-%04x: read xdpe crc command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, WB_XDPE_CHIP_NVM_HIGH_COMMAND, nvm_val_high);
        goto error;
    }

    nvm_val_low = wb_i2c_smbus_read_word_data(client, WB_XDPE_CHIP_NVM_LOW_COMMAND);
    if (nvm_val_low < 0) {
        ret = nvm_val_low;
        DEBUG_ERROR("%d-%04x: read xdpe crc command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, WB_XDPE_CHIP_NVM_LOW_COMMAND, nvm_val_low);
        goto error;
    }

    mutex_unlock(&data->update_lock);

    /* Starting from 0, iterate through bits 0 to 24 (a total of 25 bits) of the variable tmp. */
    /* For each bit that is 0, it indicates one remaining upgrade attempt. */
    nvm_val = ((nvm_val_high & 0xffff) << 16) | (nvm_val_low & 0xffff);
    nvm_time = 0;
    for (i = 0; i < WB_XDPE_CHIP_NVM_TIME_MAX; i++) {
        if ((nvm_val & (0x1 << i)) == 0) {
            nvm_time++;
        }
    }

    DEBUG_VERBOSE("%d-%04x: high: 0x%x, low: 0x%x, nvm_val: 0x%x, nvm_time: %d\n",
        client->adapter->nr, client->addr, nvm_val_high, nvm_val_low, nvm_val, nvm_time);

    return snprintf(buf, WB_XDPE_BUF_LEN_MAX, "%d\n", nvm_time);

error:
    mutex_unlock(&data->update_lock);
    return ret;
}

static ssize_t xdpe_show_chip_version_info(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client;
    char buf_tmp[WB_XDPE_BUF_LEN_MAX];
    int ret, buf_len;

    if (is_xdpe_dev_available(dev) == false) {
        DEBUG_VERBOSE("xdpe_power_value_show return failed due to dev unavailable\n");
        return -EINVAL;
    }
    client = to_i2c_client(dev);
    buf_len = 0;

    memset(buf_tmp, 0, sizeof(buf_tmp));
    ret = xdpe_show_chip_crc(dev, da, buf_tmp);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read xdpe crc failed, ret: %d\n", client->adapter->nr, client->addr, ret);
        return ret;
    }
    buf_len += snprintf(buf + buf_len, PAGE_SIZE - buf_len, "Chip CRC8: %s", buf_tmp);

    memset(buf_tmp, 0, sizeof(buf_tmp));
    ret = xdpe_show_chip_ver_date(dev, da, buf_tmp);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read xdpe ver_date failed, ret: %d\n", client->adapter->nr, client->addr, ret);
        return ret;
    }
    buf_len += snprintf(buf + buf_len, PAGE_SIZE - buf_len, "Chip Version Date: %s", buf_tmp);

    memset(buf_tmp, 0, sizeof(buf_tmp));
    ret = xdpe_show_chip_nvm_times(dev, da, buf_tmp);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read xdpe nvm_times failed, ret: %d\n", client->adapter->nr, client->addr, ret);
        return ret;
    }
    buf_len += snprintf(buf + buf_len, PAGE_SIZE - buf_len, "Chip NVM Program times left: %s", buf_tmp);

    return buf_len;
}

/* Firmware upgrade command to write to the ROM register address and value, i.e., writing 0x3F42 to register 0xE8 on page 0. */
static ssize_t xdpe_store_remap_to_nvm(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct i2c_client *client;
    struct xdpe_data *data;
    long param;
    int ret, read_back;

    if (is_xdpe_dev_available(dev) == false) {
        DEBUG_VERBOSE("xdpe_store_remap_to_nvm return failed due to dev unavailable\n");
        return -EINVAL;
    }

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);
    if (data == NULL) {
        DEBUG_ERROR("xdpe_store_remap_to_nvm param error\n");
        return -EINVAL;
    }

    param = 0;
    ret = kstrtol(buf, 0, &param);
    if (ret) {
        DEBUG_ERROR("%d-%04x: invalid value: %s\n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    /* If the input parameter is not 0x3F42, exit. */
    if ((param & 0xffff) != WB_XDPE_STORE_TO_NVM_VALUE) {
        DEBUG_ERROR("%d-%04x: invalid param: 0x%lx\n", client->adapter->nr, client->addr, param);
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);

    /* Switch to PAGE 0 */
    ret = wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, 0);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe page 0 failed, ret: %d\n", client->adapter->nr, client->addr, ret);
        goto error;
    }

    ret = wb_i2c_smbus_write_word_data(client, WB_XDPE_STORE_TO_NVM_COMMAND, WB_XDPE_STORE_TO_NVM_VALUE);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe store_nvm cmd reg: 0x%x,  value: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, WB_XDPE_STORE_TO_NVM_COMMAND, WB_XDPE_STORE_TO_NVM_VALUE, ret);
        goto error;
    }
    /* Read-back verification */
    read_back = wb_i2c_smbus_read_word_data(client, WB_XDPE_STORE_TO_NVM_COMMAND);
    if (read_back < 0) {
        ret = read_back;
        DEBUG_ERROR("%d-%04x: read xdpe store_nvm command reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, WB_XDPE_I2C_VOUT_COMMAND, ret);
        goto error;
    }
    if (read_back != WB_XDPE_STORE_TO_NVM_VALUE) {
        ret = -EIO;
        DEBUG_ERROR("%d-%04x: xdpe store_nvm value check error, read: 0x%x, set: 0x%x\n",
            client->adapter->nr, client->addr, read_back, WB_XDPE_STORE_TO_NVM_VALUE);
        goto error;
    }

    mutex_unlock(&data->update_lock);
    DEBUG_VERBOSE("%d-%04x: store remap to nvm success\n", client->adapter->nr, client->addr);
    return count;

error:
    mutex_unlock(&data->update_lock);
    DEBUG_VERBOSE("%d-%04x: store remap to nvm failed\n", client->adapter->nr, client->addr);
    return ret;
}

/* I2C unlock */
static ssize_t xdpe_unlock_dev_i2c(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct i2c_client *client;
    struct xdpe_data *data;
    int ret, read_val_0xfd, read_val_0x90;

    if (is_xdpe_dev_available(dev) == false) {
        DEBUG_VERBOSE("xdpe_store_remap_to_nvm return failed due to dev unavailable\n");
        return -EINVAL;
    }

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);
    if (data == NULL) {
        DEBUG_ERROR("xdpe_unlock_dev_i2c param error\n");
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);

    /* Switch to PAGE 0 */
    ret = wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, 0);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe page 0 failed, ret: %d\n", client->adapter->nr, client->addr, ret);
        goto error;
    }

    /* read 0xfd */
    read_val_0xfd = wb_i2c_smbus_read_byte_data(client, 0xfd);
    if (read_val_0xfd < 0) {
        ret = read_val_0xfd;
        DEBUG_ERROR("%d-%04x: read xdpe reg: 0xfd failed, ret: %d\n", client->adapter->nr, client->addr, ret);
        goto error;
    }

    if ((read_val_0xfd & 0xff) == 0) {
        /* Read value 0 from address 0xFD, then write 0x2 to address 0xE6. */
        ret = wb_i2c_smbus_write_byte_data(client, 0xe6, 0x2);
        if (ret < 0) {
            DEBUG_ERROR("%d-%04x: write xdpe reg 0xe6 0x2 failed, ret: %d\n", client->adapter->nr, client->addr, ret);
            goto error;
        }

        /* read 0x90 */
        read_val_0x90 = wb_i2c_smbus_read_byte_data(client, 0x90);
        if (read_val_0x90 < 0) {
            ret = read_val_0x90;
            DEBUG_ERROR("%d-%04x: read xdpe reg: 0x90 failed, ret: %d\n", client->adapter->nr, client->addr, ret);
            goto error;
        }

        if (((read_val_0x90 >> 5) & 0x01) == 0) {
            ret = wb_i2c_smbus_write_byte_data(client, 0xec, 0x5a);
            if (ret < 0) {
                DEBUG_ERROR("%d-%04x: write xdpe reg 0xec 0x5a failed, ret: %d\n", client->adapter->nr, client->addr, ret);
                goto error;
            }
            ret = wb_i2c_smbus_write_byte_data(client, 0xed, 0xa5);
            if (ret < 0) {
                DEBUG_ERROR("%d-%04x: write xdpe reg 0xed 0xa5 failed, ret: %d\n", client->adapter->nr, client->addr, ret);
                goto error;
            }
        }
    } else {
        /* If the value read from 0xFD is non-zero, write 0x3 to 0xE6. */
        ret = wb_i2c_smbus_write_byte_data(client, 0xe6, 0x3);
        if (ret < 0) {
            DEBUG_ERROR("%d-%04x: write xdpe reg 0xe6 0x3 failed, ret: %d\n", client->adapter->nr, client->addr, ret);
            goto error;
        }
    }

    mutex_unlock(&data->update_lock);
    DEBUG_VERBOSE("%d-%04x: xdpe unlock i2c success\n", client->adapter->nr, client->addr);
    return count;

error:
    mutex_unlock(&data->update_lock);
    DEBUG_VERBOSE("%d-%04x: xdpe unlock i2c failed\n", client->adapter->nr, client->addr);
    return ret;
}

static ssize_t xdpe_get_dev_available_flag(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client;
    struct xdpe_data *data;

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);

    if (data == NULL) {
        DEBUG_ERROR("xdpe_get_dev_available_flag param error\n");
        return -EINVAL;
    }

    return snprintf(buf, PAGE_SIZE, "%d\n", data->dev_available);
}

static ssize_t xdpe_set_dev_available_flag(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct i2c_client *client;
    struct xdpe_data *data;
    long set_val;
    int ret;

    if (buf == NULL) {
        DEBUG_ERROR("xdpe_set_dev_available_flag param buf error\n");
        return -EINVAL;
    }

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);
    if (data == NULL) {
        DEBUG_ERROR("xdpe_set_dev_available_flag param error\n");
        return -EINVAL;
    }

    set_val = WB_XDPE_DEV_AVAILABLE_FLAG;
    ret = kstrtol(buf, 0, &set_val);
    if (ret) {
        DEBUG_ERROR("%d-%04x: invalid value: %s \n", client->adapter->nr, client->addr, buf);
        return -EINVAL;
    }

    if (set_val == WB_XDPE_DEV_UNAVAILABLE_FLAG) {
        data->dev_available = WB_XDPE_DEV_UNAVAILABLE_FLAG;
    } else {
        data->dev_available = WB_XDPE_DEV_AVAILABLE_FLAG;
    }
    DEBUG_VERBOSE("%d-%04x: set_val: %ld, dev_available: %d\n",
        client->adapter->nr, client->addr, set_val, data->dev_available);

    return count;
}

/* xdpe hwmon */
static SENSOR_DEVICE_ATTR(power1_input, S_IRUGO, xdpe_power_value_show, NULL, 0x072c);
static SENSOR_DEVICE_ATTR(power2_input, S_IRUGO, xdpe_power_value_show, NULL, 0x0b2c);
static SENSOR_DEVICE_ATTR(power3_input, S_IRUGO, xdpe_power_value_show, NULL, 0x072c0b2c);

static SENSOR_DEVICE_ATTR(phase1_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE1_CURR_REG);
static SENSOR_DEVICE_ATTR(phase2_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE2_CURR_REG);
static SENSOR_DEVICE_ATTR(phase3_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE3_CURR_REG);
static SENSOR_DEVICE_ATTR(phase4_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE4_CURR_REG);
static SENSOR_DEVICE_ATTR(phase5_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE5_CURR_REG);
static SENSOR_DEVICE_ATTR(phase6_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE6_CURR_REG);
static SENSOR_DEVICE_ATTR(phase7_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE7_CURR_REG);
static SENSOR_DEVICE_ATTR(phase8_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE8_CURR_REG);
static SENSOR_DEVICE_ATTR(phase9_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE9_CURR_REG);
static SENSOR_DEVICE_ATTR(phase10_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE10_CURR_REG);
static SENSOR_DEVICE_ATTR(phase11_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE11_CURR_REG);
static SENSOR_DEVICE_ATTR(phase12_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE12_CURR_REG);
static SENSOR_DEVICE_ATTR(phase13_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE13_CURR_REG);
static SENSOR_DEVICE_ATTR(phase14_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE14_CURR_REG);
static SENSOR_DEVICE_ATTR(phase15_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE15_CURR_REG);
static SENSOR_DEVICE_ATTR(phase16_curr, S_IRUGO, xdpe_phase_curr_show, NULL, WB_XDPE_PHASE16_CURR_REG);

static struct attribute *xdpe_hwmon_attrs[] = {
    &sensor_dev_attr_power1_input.dev_attr.attr,
    &sensor_dev_attr_power2_input.dev_attr.attr,
    &sensor_dev_attr_power3_input.dev_attr.attr,
    &sensor_dev_attr_phase1_curr.dev_attr.attr,
    &sensor_dev_attr_phase2_curr.dev_attr.attr,
    &sensor_dev_attr_phase3_curr.dev_attr.attr,
    &sensor_dev_attr_phase4_curr.dev_attr.attr,
    &sensor_dev_attr_phase5_curr.dev_attr.attr,
    &sensor_dev_attr_phase6_curr.dev_attr.attr,
    &sensor_dev_attr_phase7_curr.dev_attr.attr,
    &sensor_dev_attr_phase8_curr.dev_attr.attr,
    &sensor_dev_attr_phase9_curr.dev_attr.attr,
    &sensor_dev_attr_phase10_curr.dev_attr.attr,
    &sensor_dev_attr_phase11_curr.dev_attr.attr,
    &sensor_dev_attr_phase12_curr.dev_attr.attr,
    &sensor_dev_attr_phase13_curr.dev_attr.attr,
    &sensor_dev_attr_phase14_curr.dev_attr.attr,
    &sensor_dev_attr_phase15_curr.dev_attr.attr,
    &sensor_dev_attr_phase16_curr.dev_attr.attr,
    NULL
};
ATTRIBUTE_GROUPS(xdpe_hwmon);

/* xdpe sysfs */
static SENSOR_DEVICE_ATTR(avs_vout, S_IRUGO | S_IWUSR, xdpe_avs_vout_show, xdpe_avs_vout_store, 0);
static SENSOR_DEVICE_ATTR(avs_vout_max, S_IRUGO | S_IWUSR, xdpe_avs_vout_max_show, xdpe_avs_vout_max_store, 0);
static SENSOR_DEVICE_ATTR(avs_vout_min, S_IRUGO | S_IWUSR, xdpe_avs_vout_min_show, xdpe_avs_vout_min_store, 0);
static SENSOR_DEVICE_ATTR_2(version, S_IRUGO, xdpe_word_data_show, NULL, WB_XDPE_VERSION_PAGE, WB_XDPE_VERSION_REG);

static SENSOR_DEVICE_ATTR(dev_available, S_IRUGO | S_IWUSR, xdpe_get_dev_available_flag, xdpe_set_dev_available_flag, 0);

/* show chip ver sysfs */
static SENSOR_DEVICE_ATTR(chip_crc, S_IRUGO, xdpe_show_chip_crc, NULL, 0);
static SENSOR_DEVICE_ATTR(chip_ver_date, S_IRUGO, xdpe_show_chip_ver_date, NULL, 0);
static SENSOR_DEVICE_ATTR(chip_nvm_time, S_IRUGO, xdpe_show_chip_nvm_times, NULL, 0);
static SENSOR_DEVICE_ATTR(chip_ver_info, S_IRUGO, xdpe_show_chip_version_info, NULL, 0);

/* firmware upgrade sysfs */
static SENSOR_DEVICE_ATTR(store_to_nvm, S_IRUGO | S_IWUSR, NULL, xdpe_store_remap_to_nvm, 0);
static SENSOR_DEVICE_ATTR(unlock_i2c, S_IRUGO | S_IWUSR, NULL, xdpe_unlock_dev_i2c, 0);

static struct attribute *xdpe132g5c_sysfs_attrs[] = {
    &sensor_dev_attr_avs_vout.dev_attr.attr,
    &sensor_dev_attr_avs_vout_max.dev_attr.attr,
    &sensor_dev_attr_avs_vout_min.dev_attr.attr,
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_dev_available.dev_attr.attr,
    /* show chip ver sysfs */
    &sensor_dev_attr_chip_crc.dev_attr.attr,
    &sensor_dev_attr_chip_ver_date.dev_attr.attr,
    &sensor_dev_attr_chip_nvm_time.dev_attr.attr,
    &sensor_dev_attr_chip_ver_info.dev_attr.attr,

    /* firmware upgrade sysfs */
    &sensor_dev_attr_store_to_nvm.dev_attr.attr,
    &sensor_dev_attr_unlock_i2c.dev_attr.attr,

    NULL,
};

static const struct attribute_group xdpe132g5c_sysfs_attrs_group = {
    .attrs = xdpe132g5c_sysfs_attrs,
};

static loff_t xdpe132g5c_dev_llseek(struct file *file, loff_t offset, int origin)
{
    loff_t ret = 0;

    switch (origin) {
    case SEEK_SET:        /* Offset relative to the beginning of the file. */
        if (offset < 0) {
            DEBUG_VERBOSE("SEEK_SET, offset:%lld, invalid.\r\n", offset);
            ret = -EINVAL;
            break;
        }
        file->f_pos = offset;
        ret = file->f_pos;
        break;
    case SEEK_CUR:        /* Offset relative to the current position in the file. */
        if (file->f_pos + offset < 0) {
            DEBUG_VERBOSE("SEEK_CUR out of range, f_ops:%lld, offset:%lld.\n", file->f_pos, offset);
        }
        file->f_pos += offset;
        ret = file->f_pos;
        break;
    default:
        DEBUG_VERBOSE("unsupport llseek type:%d.\n", origin);
        ret = -EINVAL;
        break;
    }
    return ret;
}

/**
 * The xdpe132 character device read interface currently supports only 1-byte or 2-byte reads.
 * The offset supports the format: Bit16 set to 1 + Followed by 8 bits for the page number + Followed by 8 bits for the read address
 * For example, 0x10234 means reading from page 2, address 0x34.
 *              0x00034 means reading from address 0x34 of the current page without switching pages.
 */
static ssize_t xdpe132g5c_dev_read(struct file *file, char __user *buf, size_t count, loff_t *offset)
{
    struct xdpe_data *i2c_dev;
    struct i2c_client *client;
    loff_t offset_tmp;
    u8 page, addr;
    int data, ret;

    if (file == NULL || buf == NULL || offset == NULL || *offset < 0) {
        DEBUG_ERROR("param error, read failed.\n");
        return -EINVAL;
    }

    if (count != WIDTH_1Byte && count != WIDTH_2Byte) {
        DEBUG_ERROR("read conut %lu .\n", count);
        return -EINVAL;
    }

    i2c_dev = file->private_data;
    if (i2c_dev == NULL) {
        DEBUG_ERROR("can't get read private_data .\r\n");
        return -EINVAL;
    }
    client = i2c_dev->client;
    if (client == NULL) {
        DEBUG_ERROR("can't get client.\r\n");
        return -EINVAL;
    }

    offset_tmp = *offset;
    addr = offset_tmp & 0xff;
    page = (offset_tmp >> WB_XDPE_DEV_RW_PAGE_INFO_BIT) & 0xff;
    DEBUG_VERBOSE("%d-%04x: offset_tmp 0x%llx addr 0x%x page 0x%x\n",
        client->adapter->nr, client->addr, offset_tmp, addr, page);

    mutex_lock(&i2c_dev->update_lock);

    if (offset_tmp & WB_XDPE_DEV_RW_SET_PAGE_MASK) {
        /* need switch page */
        ret = wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, page);
        if (ret < 0) {
            DEBUG_ERROR("%d-%04x: set xdpe page%u failed, ret: %d\n", client->adapter->nr,
                client->addr, page, ret);
            goto error;
        }
    }

    if (count == WIDTH_1Byte) {
        data = wb_i2c_smbus_read_byte_data(client, addr);
    } else {
        data = wb_i2c_smbus_read_word_data(client, addr);
    }
    if (data < 0) {
        DEBUG_ERROR("%d-%04x: read xdpe reg: 0x%x failed, ret: %d\n",
            client->adapter->nr, client->addr, (u8)*offset, data);
        goto error;
    }

    mutex_unlock(&i2c_dev->update_lock);

    if (copy_to_user(buf, &data, count)) {
        DEBUG_ERROR("copy_to_user error \r\n");
        return -EFAULT;
    }
    /* After reading the data, the offset address is incremented by the number of bytes read (count). */
    *offset += count;

    return count;

error:
    mutex_unlock(&i2c_dev->update_lock);
    return -EFAULT;
}

/* The xdpe132 character device write interface currently supports only 1-byte writes. */
static ssize_t xdpe132g5c_dev_write(struct file *file, const char __user *buf, size_t count, loff_t *offset)
{
    struct xdpe_data *i2c_dev;
    struct i2c_client *client;
    loff_t offset_tmp;
    u8 page, addr, val;
    int ret;

    if (count != WIDTH_1Byte) {
        DEBUG_ERROR("write conut %lu\n", count);
        return -EINVAL;
    }

    i2c_dev = file->private_data;
    if (i2c_dev == NULL) {
        DEBUG_ERROR("get write private_data error.\r\n");
        return -EINVAL;
    }
    client = i2c_dev->client;
    if (client == NULL) {
        DEBUG_ERROR("can't get client.\r\n");
        return -EINVAL;
    }

    if (copy_from_user(&val, buf, count)) {
        DEBUG_ERROR("copy_from_user error.\r\n");
        return -EFAULT;
    }

    offset_tmp = *offset;
    addr = offset_tmp & 0xff;
    page = (offset_tmp >> WB_XDPE_DEV_RW_PAGE_INFO_BIT) & 0xff;
    DEBUG_VERBOSE("%d-%04x: offset_tmp 0x%llx addr 0x%x page 0x%x\n",
        client->adapter->nr, client->addr, offset_tmp, addr, page);

    mutex_lock(&i2c_dev->update_lock);

    if (offset_tmp & WB_XDPE_DEV_RW_SET_PAGE_MASK) {
        /* need switch page */
        ret = wb_i2c_smbus_write_byte_data(client, WB_XDPE_I2C_PAGE_ADDR, page);
        if (ret < 0) {
            DEBUG_ERROR("%d-%04x: set xdpe page%u failed, ret: %d\n", client->adapter->nr,
                client->addr, page, ret);
            goto error;
        }
    }

    ret = wb_i2c_smbus_write_byte_data(client, addr, val);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: set xdpe page%u failed, ret: %d\n",
            client->adapter->nr, client->addr, addr, ret);
        goto error;
    }

    mutex_unlock(&i2c_dev->update_lock);

    /* After reading the data, the offset address is incremented by the number of bytes read (count). */
    *offset += count;

    return count;

error:
    mutex_unlock(&i2c_dev->update_lock);
    return -EFAULT;
}

static long xdpe132g5c_dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    return 0;
}

static int xdpe132g5c_dev_open(struct inode *inode, struct file *file)
{
    unsigned int minor = iminor(inode);
    struct xdpe_data *i2c_dev;

    if (minor >= WB_XDPE_MAX_MISC_DEV_NUM) {
        DEBUG_ERROR("minor [%d] is greater than max i2c dev num [%d], open fail.\r\n",
            minor, WB_XDPE_MAX_MISC_DEV_NUM);
        return -EINVAL;
    }
    i2c_dev = xdpe132g5c_dev_arry[minor];
    if (i2c_dev == NULL) {
        return -ENODEV;
    }

    file->private_data = i2c_dev;

    return 0;
}

static int xdpe132g5c_dev_release(struct inode *inode, struct file *file)
{
    file->private_data = NULL;

    return 0;
}

static const struct file_operations xdpe132g5c_dev_fops = {
    .owner          = THIS_MODULE,
    .llseek         = xdpe132g5c_dev_llseek,
    .read           = xdpe132g5c_dev_read,
    .write          = xdpe132g5c_dev_write,
    .unlocked_ioctl = xdpe132g5c_dev_ioctl,
    .open           = xdpe132g5c_dev_open,
    .release        = xdpe132g5c_dev_release,
};

static int xdpe132g5c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct xdpe_data *data;
    struct miscdevice *misc;
    int ret;

    DEBUG_VERBOSE("bus: %d, addr: 0x%02x do probe.\n", client->adapter->nr, client->addr);
    data = devm_kzalloc(&client->dev, sizeof(struct xdpe_data), GFP_KERNEL);
    if (!data) {
        dev_err(&client->dev, "devm_kzalloc failed.\n");
        return -ENOMEM;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    ret = sysfs_create_group(&client->dev.kobj, &xdpe132g5c_sysfs_attrs_group);
    if (ret != 0) {
        dev_err(&client->dev, "Create xdpe132g5c sysfs failed, ret: %d\n", ret);
        return ret;
    }
    data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, client->name, data,
                          xdpe_hwmon_groups);
    if (IS_ERR(data->hwmon_dev)) {
        ret = PTR_ERR(data->hwmon_dev);
        sysfs_remove_group(&client->dev.kobj, &xdpe132g5c_sysfs_attrs_group);
        dev_err(&client->dev, "Failed to register xdpe hwmon device, ret: %d\n", ret);
        return ret;
    }
    data->vout_max = WB_XDPE_VOUT_MAX_THRESHOLD;
    data->vout_min = WB_XDPE_VOUT_MIN_THRESHOLD;

    /* dev_available The initial value is 1. 
     * When this value is exported to user space for modification, 
     * if it is set to 0, all related nodes become inaccessible. */
    data->dev_available = WB_XDPE_DEV_AVAILABLE_FLAG;

    snprintf(data->dev_name, sizeof(data->dev_name), "xdpe_%d_0x%02x", client->adapter->nr,
        client->addr);
    misc = &data->misc;
    misc->minor = MISC_DYNAMIC_MINOR;
    misc->name = data->dev_name;
    misc->fops = &xdpe132g5c_dev_fops;
    if (misc_register(misc) != 0) {
        dev_err(&client->dev, "register %s faild.\r\n", misc->name);
        hwmon_device_unregister(data->hwmon_dev);
        sysfs_remove_group(&client->dev.kobj, &xdpe132g5c_sysfs_attrs_group);
        return -ENXIO;
    }

    if (misc->minor >= WB_XDPE_MAX_MISC_DEV_NUM) {
        dev_err(&client->dev, "minor number beyond the limit! is %d.\r\n", misc->minor);
        hwmon_device_unregister(data->hwmon_dev);
        sysfs_remove_group(&client->dev.kobj, &xdpe132g5c_sysfs_attrs_group);
        misc_deregister(misc);
        return -ENXIO;
    }
    /* Add to the device list */
    xdpe132g5c_dev_arry[misc->minor] = data;
    dev_info(&client->dev, "bus: %d addr: 0x%02x register %s success.\n",
        client->adapter->nr, client->addr, misc->name);

    dev_info(&client->dev, "xdpe132g5c probe success\n");
    return 0;
}

static void xdpe132g5c_remove(struct i2c_client *client)
{
    struct xdpe_data *data;

    DEBUG_VERBOSE("bus: %d, addr: 0x%02x do remove\n", client->adapter->nr, client->addr);
    data = i2c_get_clientdata(client);
    if (data == NULL || data->hwmon_dev == NULL) {
        DEBUG_VERBOSE("get client data error\n");
        return;
    }

    hwmon_device_unregister(data->hwmon_dev);
    sysfs_remove_group(&client->dev.kobj, &xdpe132g5c_sysfs_attrs_group);

    DEBUG_VERBOSE("xdpe132g5c_remove minor %d\n", data->misc.minor);
    if ((data->misc.minor >= 0) && (data->misc.minor < WB_XDPE_MAX_MISC_DEV_NUM)) {
        if (xdpe132g5c_dev_arry[data->misc.minor] != NULL) {
            xdpe132g5c_dev_arry[data->misc.minor] = NULL;
        }
    }
    misc_deregister(&data->misc);

    return;
}

static const struct i2c_device_id xdpe132g5c_id[] = {
    {"wb_xdpe132g5c", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, xdpe132g5c_id);

static const struct of_device_id __maybe_unused xdpe132g5c_of_match[] = {
    {.compatible = "infineon,wb_xdpe132g5c"},
    {}
};
MODULE_DEVICE_TABLE(of, xdpe132g5c_of_match);

static struct i2c_driver wb_xdpe132g5c_driver = {
    .driver = {
        .name = "wb_xdpe132g5c",
        .of_match_table = of_match_ptr(xdpe132g5c_of_match),
    },
    .probe      = xdpe132g5c_probe,
    .remove     = xdpe132g5c_remove,
    .id_table   = xdpe132g5c_id,
};

module_i2c_driver(wb_xdpe132g5c_driver);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
MODULE_DESCRIPTION("I2C driver for Infineon XDPE132 family");
