// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Hardware monitoring driver for UCD90xxx Sequencer and System Health
 * Controller series
 *
 * Copyright (C) 2011 Ericsson AB.
 */

#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/pmbus.h>
#include <linux/gpio/driver.h>
#include <linux/delay.h>
#include <linux/workqueue.h>

#include "wb_pmbus.h"
#include <wb_bsp_kernel_debug.h>

enum chips { ucd9000, ucd90120, ucd90124, ucd90160, ucd90320, ucd9090,
             ucd90910
           };

#define UCD9000_MONITOR_CONFIG      (0xd5)
#define UCD9000_NUM_PAGES           (0xd6)
#define UCD9000_RUN_TIME_CLOCK      (0xd7)
#define UCD9000_FAN_CONFIG_INDEX    (0xe7)
#define UCD9000_FAN_CONFIG          (0xe8)
#define UCD9000_MFR_STATUS          (0xf3)
#define UCD9000_GPIO_SELECT         (0xfa)
#define UCD9000_GPIO_CONFIG         (0xfb)
#define UCD9000_DEVICE_ID           (0xfd)

#define UCD9000_RUN_TIME_CLOCK_COUNT      (8)

/* GPIO CONFIG bits */
#define UCD9000_GPIO_CONFIG_ENABLE        BIT(0)
#define UCD9000_GPIO_CONFIG_OUT_ENABLE    BIT(1)
#define UCD9000_GPIO_CONFIG_OUT_VALUE     BIT(2)
#define UCD9000_GPIO_CONFIG_STATUS        BIT(3)
#define UCD9000_GPIO_INPUT        (0)
#define UCD9000_GPIO_OUTPUT       (1)

#define UCD9000_MON_TYPE(x)       (((x) >> 5) & 0x07)
#define UCD9000_MON_PAGE(x)       ((x) & 0x1f)

#define UCD9000_MON_VOLTAGE       (1)
#define UCD9000_MON_TEMPERATURE   (2)
#define UCD9000_MON_CURRENT       (3)
#define UCD9000_MON_VOLTAGE_HW    (4)

#define UCD9000_NUM_FAN           (4)

#define UCD9000_GPIO_NAME_LEN     (16)
#define UCD9090_NUM_GPIOS         (23)
#define UCD901XX_NUM_GPIOS        (26)
#define UCD90320_NUM_GPIOS        (84)
#define UCD90910_NUM_GPIOS        (26)

#define UCD9000_DEBUGFS_NAME_LEN  (24)
#define UCD9000_GPI_COUNT         (8)
#define UCD90320_GPI_COUNT        (32)

#define UCD9000_RETRY_SLEEP_TIME          (20000)   /* 20ms */
#define UCD9000_RETRY_TIME                (3)
#define WB_DEV_NAME_MAX_LEN               (64)

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static int dwork_set_time_enable = 0;
module_param(dwork_set_time_enable, int, S_IRUGO | S_IWUSR);

static int dwork_set_delay = 1800;  /* half hour */
module_param(dwork_set_delay, int, S_IRUGO | S_IWUSR);

struct ucd9000_data {
    u8 fan_data[UCD9000_NUM_FAN][I2C_SMBUS_BLOCK_MAX];
    struct pmbus_driver_info info;
#ifdef CONFIG_GPIOLIB
    struct gpio_chip gpio;
#endif
    struct dentry *debugfs;
    struct i2c_client *client;
    struct delayed_work dwork_set_time;     /* delay work for set ucd90160 fault record time */
};
#define to_ucd9000_data(_info) container_of(_info, struct ucd9000_data, info)

struct ucd9000_debugfs_entry {
    struct i2c_client *client;
    u8 index;
};

/* fault record */
#define FAULT_RECORD_SIZE                   (512)

#define LOGGED_FAULT_CLEAR                  (0xEA)
#define LOGGED_FAULT_DETAIL_INDEX           (0xEB)
#define LOGGED_FAULT_INFO                   (0xEC)

#define PAGE_FAULT                          BIT(31)

#define FAULT_TYPE_OFFSET                   (27)
#define FAULT_TYPE_MASK                     (0xF)

#define FAULT_PAGE_OFFSET                   (23)
#define FAULT_PAGE_MASK                     (0xF)

#define FAULT_CLEAR_CMD                     (0xC)
#define FAULT_CLEAR_LENGTH                  (18)
#define ONE_DAY_TO_SECON                    (86400) /* one day is 24*3600 */

static ssize_t get_fault_record(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data = i2c_get_clientdata(client);
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1];
    int ret;
    int fault_num;
    int fault_index;
    int i;
    u32 tmp_data;
    int value;
    u8 fault_type;
    u8 fault_page;
    int exponent;
    int vout_mode;

    mutex_lock(&data->update_lock);
    mem_clear(buf, FAULT_RECORD_SIZE);
    mem_clear(block_buffer, sizeof(block_buffer));
    /* get fault index and number */
    ret = i2c_smbus_read_i2c_block_data(client, LOGGED_FAULT_DETAIL_INDEX, 2, block_buffer);
    if (ret <= 0) {
        dev_err(&client->dev, "Failed to read LOGGED_FAULT_DETAIL_INDEX, ret = %d\n", ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    fault_num = block_buffer[1];
    fault_index = block_buffer[0];
    DEBUG_VERBOSE("bus %d addr 0x%x, get_fault_record\n", client->adapter->nr, client->addr);
    DEBUG_VERBOSE("fault_num = 0x%x", fault_num);
    DEBUG_VERBOSE("fault_index = 0x%x", fault_index);

    if (fault_num > 0) {
        for (i = 0; i < fault_num; i++) {
            block_buffer[0] = i;
            block_buffer[1] = fault_num;
            /* set fault index */
            ret = i2c_smbus_write_i2c_block_data(client, LOGGED_FAULT_DETAIL_INDEX, 2, block_buffer);
            if (ret != 0) {
                dev_dbg(&client->dev, "Failed to write LOGGED_FAULT_DETAIL_INDEX, ret = %d\n", ret);
                mutex_unlock(&data->update_lock);
                return ret;
            }

            /* get one fault record */
            ret = i2c_smbus_read_i2c_block_data(client, LOGGED_FAULT_INFO, 0xB, block_buffer);
            if (ret <= 0) {
                dev_dbg(&client->dev, "Failed to read LOGGED_FAULT_INFO, ret = %d\n", ret);
                mutex_unlock(&data->update_lock);
                return ret;
            }

            /* fault id + days data */
            tmp_data = (u32)(block_buffer[5] << 24 | block_buffer[6] << 16 | block_buffer[7] << 8 | block_buffer[8]);
            DEBUG_VERBOSE("tmp_data = 0x%x\n", tmp_data);
            if (tmp_data & PAGE_FAULT) {
                fault_type = tmp_data >> FAULT_TYPE_OFFSET & FAULT_TYPE_MASK;
                DEBUG_VERBOSE("fault_type = 0x%x\n", fault_type);

                fault_page = tmp_data >> FAULT_PAGE_OFFSET & FAULT_PAGE_MASK;
                DEBUG_VERBOSE("fault_page = 0x%x\n", fault_page);

                /* get page vout_mode */
                ret = wb_pmbus_read_byte_data(client, fault_page, PMBUS_VOUT_MODE);
                if (ret < 0) {
                    dev_dbg(&client->dev, "Failed to read PMBUS_VOUT_MODE, ret = %d\n", ret);
                    mutex_unlock(&data->update_lock);
                    return ret;
                }
                vout_mode = ret;
                DEBUG_VERBOSE("vout_mode = 0x%x\n", vout_mode);
                exponent = ((s8)(vout_mode << 3)) >> 3;
                DEBUG_VERBOSE("exponent = 0x%x\n", exponent);
                block_buffer[11] = vout_mode;

                /* value data */
                value = (int)(block_buffer[10] << 8 | block_buffer[9]);
                value = value * 1000;
                if (exponent >= 0) {
                    value <<= exponent;
                } else {
                    value >>= -exponent;
                }

                DEBUG_VERBOSE("value = %d.%dv\n", value / 1000, value % 1000);
            } else {  /* not page fault */
                DEBUG_VERBOSE("bus %d addr 0x%x, not page fault\n", client->adapter->nr, client->addr);
            }

            memcpy(buf + i * 16, &block_buffer[0], 16);
        }

        block_buffer[0] = fault_index;
        block_buffer[1] = fault_num;
        /* set fault index to origin index */
        ret = i2c_smbus_write_i2c_block_data(client, LOGGED_FAULT_DETAIL_INDEX, 2, block_buffer);
        if (ret != 0) {
            dev_dbg(&client->dev, "Failed to write LOGGED_FAULT_DETAIL_INDEX2, ret = %d\n", ret);
            mutex_unlock(&data->update_lock);
            return ret;
        }
    }

    mutex_unlock(&data->update_lock);
    return FAULT_RECORD_SIZE;
}

static ssize_t clear_fault_record(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    char tmp_value[32];
    int err;
    int ret;
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data = i2c_get_clientdata(client);
    unsigned long val;

    val = 0;
    err = kstrtoul(buf, 16, &val);
    if (err) {
        dev_dbg(&client->dev, "kstrtoul failed, err = %d\n", err);
        return err;
    }

    if (val != 0) {
        dev_dbg(&client->dev, "please enter 0 to clear fault_record, val = %ld\n", val);
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    memset(tmp_value, 0, sizeof(tmp_value));
    tmp_value[0] = FAULT_CLEAR_LENGTH;
    /* write 18 byte 0 to clear fault record */
    ret = i2c_smbus_write_i2c_block_data(client, LOGGED_FAULT_CLEAR, FAULT_CLEAR_LENGTH + 1, tmp_value);
    if (ret != 0) {
        dev_dbg(&client->dev, "Failed to write LOGGED_FAULT_CLEAR, ret = %d\n", ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }

    DEBUG_VERBOSE("bus %d addr 0x%x, clear_fault_record success\n", client->adapter->nr, client->addr);
    mutex_unlock(&data->update_lock);

    return count;
}

static int wb_i2c_smbus_read_byte_data(const struct i2c_client *client, u8 command)
{
    int rv, i;

    for(i = 0; i < UCD9000_RETRY_TIME; i++) {
        rv = i2c_smbus_read_byte_data(client, command);
        if (rv >= 0) {
            return rv;
        }
        usleep_range(UCD9000_RETRY_SLEEP_TIME, UCD9000_RETRY_SLEEP_TIME + 1);
    }
    DEBUG_ERROR("read_byte_data failed. nr:  %d, addr: 0x%x, reg: 0x%x, rv: %d\n",
        client->adapter->nr, client->addr, command, rv);
    return rv;
}

static int wb_i2c_smbus_read_block_data(const struct i2c_client *client, u8 command, u8 *values)
{
    int rv, i;

    for(i = 0; i < UCD9000_RETRY_TIME; i++) {
        rv = i2c_smbus_read_block_data(client, command, values);
        if(rv >= 0){
            return rv;
        }
        usleep_range(UCD9000_RETRY_SLEEP_TIME, UCD9000_RETRY_SLEEP_TIME + 1);
    }
    DEBUG_ERROR("read_block_data failed. nr:  %d, addr: 0x%x, reg: 0x%x, rv: %d\n",
        client->adapter->nr, client->addr, command, rv);
    return rv;
}

static int wb_i2c_smbus_write_block_data(const struct i2c_client *client, u8 command, u8 length, char *data_buf)
{
    int rv, i;

    for (i = 0; i < UCD9000_RETRY_TIME; i++) {
        rv = i2c_smbus_write_block_data(client, command, length, data_buf);
        if (rv >= 0) {
            return rv;
        }
        usleep_range(UCD9000_RETRY_SLEEP_TIME, UCD9000_RETRY_SLEEP_TIME + 1);
    }
    DEBUG_ERROR("write_block_data failed. nr:  %d, addr: 0x%x, reg: 0x%x, rv: %d\n",
                client->adapter->nr, client->addr, command, rv);
    return rv;
}

static int ucd9000_get_fan_config(struct i2c_client *client, int fan)
{
    int fan_config = 0;
    struct ucd9000_data *data
      = to_ucd9000_data(wb_pmbus_get_driver_info(client));

    if (data->fan_data[fan][3] & 1)
        fan_config |= PB_FAN_2_INSTALLED;   /* Use lower bit position */

    /* Pulses/revolution */
    fan_config |= (data->fan_data[fan][3] & 0x06) >> 1;

    return fan_config;
}

static int ucd9000_read_byte_data(struct i2c_client *client, int page, int reg)
{
    int ret = 0;
    int fan_config;

    switch (reg) {
    case PMBUS_FAN_CONFIG_12:
        if (page > 0)
            return -ENXIO;

        ret = ucd9000_get_fan_config(client, 0);
        if (ret < 0)
            return ret;
        fan_config = ret << 4;
        ret = ucd9000_get_fan_config(client, 1);
        if (ret < 0)
            return ret;
        fan_config |= ret;
        ret = fan_config;
        break;
    case PMBUS_FAN_CONFIG_34:
        if (page > 0)
            return -ENXIO;

        ret = ucd9000_get_fan_config(client, 2);
        if (ret < 0)
            return ret;
        fan_config = ret << 4;
        ret = ucd9000_get_fan_config(client, 3);
        if (ret < 0)
            return ret;
        fan_config |= ret;
        ret = fan_config;
        break;
    default:
        ret = -ENODATA;
        break;
    }
    return ret;
}

static ssize_t ucd9000_block_data_str_show(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data = i2c_get_clientdata(client);
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1];
    u8 reg = to_sensor_dev_attr(da)->index;
    int ret;

    memset(buf, 0, PAGE_SIZE);
    memset(block_buffer, 0, sizeof(block_buffer));

    mutex_lock(&data->update_lock);
    ret = wb_pmbus_read_block_data(client, 0, reg, block_buffer);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read ucd9000 data, reg: 0x%x, ret = %d\n", reg, ret);
        mutex_unlock(&data->update_lock);
        return ret;
    }
    mutex_unlock(&data->update_lock);

    return snprintf(buf, PAGE_SIZE, "%s\n", block_buffer);
}

static int get_current_time(uint32_t *day, uint32_t *milliseconds)
{
    struct timespec64 ts;
    u32 remainder;

    memset(&ts, 0, sizeof(struct timespec64));

    ktime_get_real_ts64(&ts);

    *day = div_s64_rem(ts.tv_sec, ONE_DAY_TO_SECON, &remainder);
    *milliseconds = (remainder * 1000) + (div_s64(ts.tv_nsec, 1000000));

    return 0;
}

static int convert_to_tm_using_time64(int days, uint32_t milliseconds, struct tm *tm)
{
    time64_t seconds;

    /* the number of seconds elapsed since 00:00:00 on January 1, 1970 */
    seconds = (time64_t)div_s64(milliseconds, 1000) + (days * ONE_DAY_TO_SECON);
    /* Convert to year-month-day */
    time64_to_tm(seconds, 0, tm);

    return 0;
}

static int ucd9000_get_date_func(struct i2c_client *client, char *buf, uint32_t len)
{
    uint32_t day;
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1];
    int ret;
    u32 ms_data;
    struct tm tm;
    size_t size;

    size = 0;
    memset(&tm, 0, sizeof(tm));
    memset(block_buffer, 0, sizeof(block_buffer));
    ret = wb_i2c_smbus_read_block_data(client, UCD9000_RUN_TIME_CLOCK, block_buffer);
    if (ret <= 0) {
        dev_info(&client->dev, "Failed to get clock, ret = %d\n", ret);
        return -EIO;
    }

    ms_data = (u32)(block_buffer[0] << 24 | block_buffer[1] << 16 | block_buffer[2] << 8 | block_buffer[3]);
    day = (u32)(block_buffer[4] << 24 | block_buffer[5] << 16 | block_buffer[6] << 8 | block_buffer[7]);
    DEBUG_VERBOSE("get date:day %u, milliseconds %u\n", day, ms_data);

    convert_to_tm_using_time64(day, ms_data, &tm);
    size += snprintf(buf + size, len, "%ld-%02d-%02d %02d:%02d:%02d\n",
        tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    return size;
}

static ssize_t get_fault_date(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data = i2c_get_clientdata(client);
    int ret;

    mutex_lock(&data->update_lock);
    ret = ucd9000_get_date_func(client, buf, PAGE_SIZE);
    mutex_unlock(&data->update_lock);

    return ret;
}

static int ucd9000_set_date_func(struct i2c_client *client, uint32_t day, uint32_t milliseconds)
{
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1];
    int ret;

    memset(block_buffer, 0, sizeof(block_buffer));
    DEBUG_VERBOSE("set date:day %u, milliseconds %u\n", day, milliseconds);
    block_buffer[0] = (milliseconds >> 24) & 0xff;
    block_buffer[1] = (milliseconds >> 16) & 0xff;
    block_buffer[2] = (milliseconds >> 8) & 0xff;
    block_buffer[3] = milliseconds & 0xff;

    block_buffer[4] = (day >> 24) & 0xff;
    block_buffer[5] = (day >> 16) & 0xff;
    block_buffer[6] = (day >> 8) & 0xff;
    block_buffer[7] = day & 0xff;
    ret = wb_i2c_smbus_write_block_data(client, UCD9000_RUN_TIME_CLOCK, UCD9000_RUN_TIME_CLOCK_COUNT, block_buffer);
    if (ret != 0) {
        dev_info(&client->dev, "Failed to set clock, ret = %d\n", ret);
        return ret;
    }

    return 0;
}

static void set_date_work_function(struct work_struct *work)
{
    struct ucd9000_data *data;
    int ret;
    uint32_t day;
    uint32_t milliseconds;
    struct pmbus_data *pmbus_data;

    data = container_of(work, struct ucd9000_data, dwork_set_time.work);
    if (data == NULL) {
        pr_info("Ucd9000 timing failed\n");
        return;
    }

    if (data->client == NULL) {
        pr_info("Ucd9000 timing failed(client is NULL)\n");
        return;
    }

    pmbus_data = i2c_get_clientdata(data->client);
    if (pmbus_data == NULL) {
        pr_info("Ucd9000 timing failed(pmbus_data is NULL)\n");
        return;
    }

    day = 0;
    milliseconds = 0;
    (void)get_current_time(&day, &milliseconds);
    mutex_lock(&pmbus_data->update_lock);
    ret = ucd9000_set_date_func(data->client, day, milliseconds);
    if (ret != 0) {
        dev_info(&data->client->dev, "set_date_work_function, Failed to set ucd9000 clock, ret = %d\n", ret);
    }
    mutex_unlock(&pmbus_data->update_lock);

    /* reset delay time */
    schedule_delayed_work(&data->dwork_set_time, HZ * dwork_set_delay);
}

static ssize_t set_fault_date_and_time(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pmbus_data *data = i2c_get_clientdata(client);
    uint32_t day;
    uint32_t milliseconds;
    int ret;
    unsigned long val;

    val = 0;
    ret = kstrtoul(buf, 0, &val);
    if (ret) {
        dev_info(&client->dev, "kstrtoul failed, ret = %d\n", ret);
        return ret;
    }

    if (val != 1) {
        dev_info(&client->dev, "please enter 1 to set_fault_date_and_time, val = %ld\n", val);
        return -EINVAL;
    }

    day = 0;
    milliseconds = 0;
    (void)get_current_time(&day, &milliseconds);

    mutex_lock(&data->update_lock);

    ret = ucd9000_set_date_func(client, day, milliseconds);
    if (ret != 0) {
        dev_info(&client->dev, "Failed to set ucd9000 clock, ret = %d\n", ret);
    }

    mutex_unlock(&data->update_lock);

    return count;
}

static SENSOR_DEVICE_ATTR(version, S_IRUGO, ucd9000_block_data_str_show, NULL, PMBUS_MFR_REVISION);
static SENSOR_DEVICE_ATTR(fault_date, S_IRUGO | S_IWUSR, get_fault_date, set_fault_date_and_time, 0);
static SENSOR_DEVICE_ATTR(fault_record, S_IRUGO | S_IWUSR, get_fault_record, clear_fault_record, 0);

static struct attribute *ucd90160_attrs[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_fault_date.dev_attr.attr,
    &sensor_dev_attr_fault_record.dev_attr.attr,
    NULL
};

static const struct attribute_group ucd9000_sysfs_group = {
    .attrs = ucd90160_attrs,
};

static const struct i2c_device_id ucd9000_id[] = {
    {"wb_ucd9000", ucd9000},
    {"wb_ucd90120", ucd90120},
    {"wb_ucd90124", ucd90124},
    {"wb_ucd90160", ucd90160},
    {"wb_ucd90320", ucd90320},
    {"wb_ucd9090", ucd9090},
    {"wb_ucd90910", ucd90910},
    {}
};
MODULE_DEVICE_TABLE(i2c, ucd9000_id);

static const struct of_device_id __maybe_unused ucd9000_of_match[] = {
    {
        .compatible = "ti,wb_ucd9000",
        .data = (void *)ucd9000
    },
    {
        .compatible = "ti,wb_ucd90120",
        .data = (void *)ucd90120
    },
    {
        .compatible = "ti,wb_ucd90124",
        .data = (void *)ucd90124
    },
    {
        .compatible = "ti,wb_ucd90160",
        .data = (void *)ucd90160
    },
    {
        .compatible = "ti,wb_ucd90320",
        .data = (void *)ucd90320
    },
    {
        .compatible = "ti,wb_ucd9090",
        .data = (void *)ucd9090
    },
    {
        .compatible = "ti,wb_ucd90910",
        .data = (void *)ucd90910
    },
    { },
};
MODULE_DEVICE_TABLE(of, ucd9000_of_match);

#ifdef CONFIG_GPIOLIB
static int ucd9000_gpio_read_config(struct i2c_client *client,
                    unsigned int offset)
{
    int ret;

    /* No page set required */
    ret = i2c_smbus_write_byte_data(client, UCD9000_GPIO_SELECT, offset);
    if (ret < 0)
        return ret;

    return i2c_smbus_read_byte_data(client, UCD9000_GPIO_CONFIG);
}

static int ucd9000_gpio_get(struct gpio_chip *gc, unsigned int offset)
{
    struct i2c_client *client  = gpiochip_get_data(gc);
    int ret;

    ret = ucd9000_gpio_read_config(client, offset);
    if (ret < 0)
        return ret;

    return !!(ret & UCD9000_GPIO_CONFIG_STATUS);
}

static void ucd9000_gpio_set(struct gpio_chip *gc, unsigned int offset,
                 int value)
{
    struct i2c_client *client = gpiochip_get_data(gc);
    int ret;

    ret = ucd9000_gpio_read_config(client, offset);
    if (ret < 0) {
        dev_dbg(&client->dev, "failed to read GPIO %d config: %d\n",
            offset, ret);
        return;
    }

    if (value) {
        if (ret & UCD9000_GPIO_CONFIG_STATUS)
            return;

        ret |= UCD9000_GPIO_CONFIG_STATUS;
    } else {
        if (!(ret & UCD9000_GPIO_CONFIG_STATUS))
            return;

        ret &= ~UCD9000_GPIO_CONFIG_STATUS;
    }

    ret |= UCD9000_GPIO_CONFIG_ENABLE;

    /* Page set not required */
    ret = i2c_smbus_write_byte_data(client, UCD9000_GPIO_CONFIG, ret);
    if (ret < 0) {
        dev_dbg(&client->dev, "Failed to write GPIO %d config: %d\n",
            offset, ret);
        return;
    }

    ret &= ~UCD9000_GPIO_CONFIG_ENABLE;

    ret = i2c_smbus_write_byte_data(client, UCD9000_GPIO_CONFIG, ret);
    if (ret < 0)
        dev_dbg(&client->dev, "Failed to write GPIO %d config: %d\n",
            offset, ret);
}

static int ucd9000_gpio_get_direction(struct gpio_chip *gc,
                      unsigned int offset)
{
    struct i2c_client *client = gpiochip_get_data(gc);
    int ret;

    ret = ucd9000_gpio_read_config(client, offset);
    if (ret < 0)
        return ret;

    return !(ret & UCD9000_GPIO_CONFIG_OUT_ENABLE);
}

static int ucd9000_gpio_set_direction(struct gpio_chip *gc,
                      unsigned int offset, bool direction_out,
                      int requested_out)
{
    struct i2c_client *client = gpiochip_get_data(gc);
    int ret, config, out_val;

    ret = ucd9000_gpio_read_config(client, offset);
    if (ret < 0)
        return ret;

    if (direction_out) {
        out_val = requested_out ? UCD9000_GPIO_CONFIG_OUT_VALUE : 0;

        if (ret & UCD9000_GPIO_CONFIG_OUT_ENABLE) {
            if ((ret & UCD9000_GPIO_CONFIG_OUT_VALUE) == out_val)
                return 0;
        } else {
            ret |= UCD9000_GPIO_CONFIG_OUT_ENABLE;
        }

        if (out_val)
            ret |= UCD9000_GPIO_CONFIG_OUT_VALUE;
        else
            ret &= ~UCD9000_GPIO_CONFIG_OUT_VALUE;

    } else {
        if (!(ret & UCD9000_GPIO_CONFIG_OUT_ENABLE))
            return 0;

        ret &= ~UCD9000_GPIO_CONFIG_OUT_ENABLE;
    }

    ret |= UCD9000_GPIO_CONFIG_ENABLE;
    config = ret;

    /* Page set not required */
    ret = i2c_smbus_write_byte_data(client, UCD9000_GPIO_CONFIG, config);
    if (ret < 0)
        return ret;

    config &= ~UCD9000_GPIO_CONFIG_ENABLE;

    return i2c_smbus_write_byte_data(client, UCD9000_GPIO_CONFIG, config);
}

static int ucd9000_gpio_direction_input(struct gpio_chip *gc,
                    unsigned int offset)
{
    return ucd9000_gpio_set_direction(gc, offset, UCD9000_GPIO_INPUT, 0);
}

static int ucd9000_gpio_direction_output(struct gpio_chip *gc,
                     unsigned int offset, int val)
{
    return ucd9000_gpio_set_direction(gc, offset, UCD9000_GPIO_OUTPUT,
                      val);
}

static void ucd9000_probe_gpio(struct i2c_client *client,
                   const struct i2c_device_id *mid,
                   struct ucd9000_data *data)
{
    int rc;

    switch (mid->driver_data) {
    case ucd9090:
        data->gpio.ngpio = UCD9090_NUM_GPIOS;
        break;
    case ucd90120:
    case ucd90124:
    case ucd90160:
        data->gpio.ngpio = UCD901XX_NUM_GPIOS;
        break;
    case ucd90320:
        data->gpio.ngpio = UCD90320_NUM_GPIOS;
        break;
    case ucd90910:
        data->gpio.ngpio = UCD90910_NUM_GPIOS;
        break;
    default:
        return; /* GPIO support is optional. */
    }

    /*
     * Pinmux support has not been added to the new gpio_chip.
     * This support should be added when possible given the mux
     * behavior of these IO devices.
     */
    data->gpio.label = client->name;
    data->gpio.get_direction = ucd9000_gpio_get_direction;
    data->gpio.direction_input = ucd9000_gpio_direction_input;
    data->gpio.direction_output = ucd9000_gpio_direction_output;
    data->gpio.get = ucd9000_gpio_get;
    data->gpio.set = ucd9000_gpio_set;
    data->gpio.can_sleep = true;
    data->gpio.base = -1;
    data->gpio.parent = &client->dev;

    rc = devm_gpiochip_add_data(&client->dev, &data->gpio, client);
    if (rc)
        dev_warn(&client->dev, "Could not add gpiochip: %d\n", rc);
}
#else
static void ucd9000_probe_gpio(struct i2c_client *client,
                   const struct i2c_device_id *mid,
                   struct ucd9000_data *data)
{
}
#endif /* CONFIG_GPIOLIB */

#ifdef CONFIG_DEBUG_FS
static int ucd9000_get_mfr_status(struct i2c_client *client, u8 *buffer)
{
    int ret = wb_pmbus_set_page(client, 0, 0xff);

    if (ret < 0)
        return ret;

    return wb_i2c_smbus_read_block_data(client, UCD9000_MFR_STATUS, buffer);
}

static int ucd9000_debugfs_show_mfr_status_bit(void *data, u64 *val)
{
    struct ucd9000_debugfs_entry *entry = data;
    struct i2c_client *client = entry->client;
    u8 buffer[I2C_SMBUS_BLOCK_MAX];
    int ret, i;

    mem_clear(buffer, sizeof(buffer));
    ret = ucd9000_get_mfr_status(client, buffer);
    if (ret < 0)
        return ret;

    /*
     * GPI fault bits are in sets of 8, two bytes from end of response.
     */
    i = ret - 3 - entry->index / 8;
    if (i >= 0)
        *val = !!(buffer[i] & BIT(entry->index % 8));

    return 0;
}
DEFINE_DEBUGFS_ATTRIBUTE(ucd9000_debugfs_mfr_status_bit,
             ucd9000_debugfs_show_mfr_status_bit, NULL, "%1lld\n");

static ssize_t ucd9000_debugfs_read_mfr_status(struct file *file,
                           char __user *buf, size_t count,
                           loff_t *ppos)
{
    struct i2c_client *client = file->private_data;
    u8 buffer[I2C_SMBUS_BLOCK_MAX];
    char str[(I2C_SMBUS_BLOCK_MAX * 2) + 2];
    char *res;
    int rc;

    rc = ucd9000_get_mfr_status(client, buffer);
    if (rc < 0)
        return rc;

    res = bin2hex(str, buffer, min(rc, I2C_SMBUS_BLOCK_MAX));
    *res++ = '\n';
    *res = 0;

    return simple_read_from_buffer(buf, count, ppos, str, res - str);
}

static const struct file_operations ucd9000_debugfs_show_mfr_status_fops = {
    .llseek = noop_llseek,
    .read = ucd9000_debugfs_read_mfr_status,
    .open = simple_open,
};

static int ucd9000_init_debugfs(struct i2c_client *client,
                const struct i2c_device_id *mid,
                struct ucd9000_data *data)
{
    struct dentry *debugfs;
    struct ucd9000_debugfs_entry *entries;
    int i, gpi_count;
    char name[UCD9000_DEBUGFS_NAME_LEN];

    debugfs = wb_pmbus_get_debugfs_dir(client);
    if (!debugfs)
        return -ENOENT;

    data->debugfs = debugfs_create_dir(client->name, debugfs);
    if (!data->debugfs)
        return -ENOENT;

    /*
     * Of the chips this driver supports, only the UCD9090, UCD90160,
     * UCD90320, and UCD90910 report GPI faults in their MFR_STATUS
     * register, so only create the GPI fault debugfs attributes for those
     * chips.
     */
    if (mid->driver_data == ucd9090 || mid->driver_data == ucd90160 ||
        mid->driver_data == ucd90320 || mid->driver_data == ucd90910) {
        gpi_count = mid->driver_data == ucd90320 ? UCD90320_GPI_COUNT
                             : UCD9000_GPI_COUNT;
        entries = devm_kcalloc(&client->dev,
                       gpi_count, sizeof(*entries),
                       GFP_KERNEL);
        if (!entries)
            return -ENOMEM;

        for (i = 0; i < gpi_count; i++) {
            entries[i].client = client;
            entries[i].index = i;
            scnprintf(name, UCD9000_DEBUGFS_NAME_LEN,
                  "gpi%d_alarm", i + 1);
            debugfs_create_file(name, 0444, data->debugfs,
                        &entries[i],
                        &ucd9000_debugfs_mfr_status_bit);
        }
    }

    scnprintf(name, UCD9000_DEBUGFS_NAME_LEN, "mfr_status");
    debugfs_create_file(name, 0444, data->debugfs, client,
                &ucd9000_debugfs_show_mfr_status_fops);

    return 0;
}
#else
static int ucd9000_init_debugfs(struct i2c_client *client,
                const struct i2c_device_id *mid,
                struct ucd9000_data *data)
{
    return 0;
}
#endif /* CONFIG_DEBUG_FS */

static int ucd9000_probe(struct i2c_client *client)
{
    u8 block_buffer[I2C_SMBUS_BLOCK_MAX + 1];
    char wb_device_name[WB_DEV_NAME_MAX_LEN];
    struct ucd9000_data *data;
    struct pmbus_driver_info *info;
    const struct i2c_device_id *mid;
    enum chips chip;
    int i, ret;

    if (!i2c_check_functionality(client->adapter,
                     I2C_FUNC_SMBUS_BYTE_DATA |
                     I2C_FUNC_SMBUS_BLOCK_DATA))
        return -ENODEV;

    /* Enable PEC if the controller supports it */
    ret = wb_i2c_smbus_read_byte_data(client, PMBUS_CAPABILITY);
    if (ret >= 0 && (ret & PB_CAPABILITY_ERROR_CHECK)) {
        if (i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_PEC)) {
            client->flags |= I2C_CLIENT_PEC;
        }
    }

    ret = wb_i2c_smbus_read_block_data(client, UCD9000_DEVICE_ID,
                    block_buffer);
    if (ret < 0) {
        dev_err(&client->dev, "Failed to read device ID, ret:%d.\n", ret);
        return ret;
    }
    block_buffer[ret] = '\0';
    dev_info(&client->dev, "Device ID %s\n", block_buffer);

    mem_clear(wb_device_name, sizeof(wb_device_name));
    snprintf(wb_device_name, sizeof(wb_device_name), "wb_%s", block_buffer);

    for (mid = ucd9000_id; mid->name[0]; mid++) {
        if (!strncasecmp(mid->name, wb_device_name, strlen(mid->name)))
            break;
    }
    if (!mid->name[0]) {
        dev_err(&client->dev, "Unsupported device\n");
        return -ENODEV;
    }

    if (client->dev.of_node)
        chip = (enum chips)of_device_get_match_data(&client->dev);
    else
        chip = mid->driver_data;

    if (chip != ucd9000 && strcmp(client->name, mid->name) != 0)
        dev_notice(&client->dev,
               "Device mismatch: Configured %s, detected %s\n",
               client->name, mid->name);

    data = devm_kzalloc(&client->dev, sizeof(struct ucd9000_data),
                GFP_KERNEL);
    if (!data)
        return -ENOMEM;
    info = &data->info;
    data->client = client;

    ret = wb_i2c_smbus_read_byte_data(client, UCD9000_NUM_PAGES);
    if (ret < 0) {
        dev_err(&client->dev,
            "Failed to read number of active pages, ret:%d.\n", ret);
        return ret;
    }
    info->pages = ret;
    if (!info->pages) {
        dev_err(&client->dev, "No pages configured\n");
        return -ENODEV;
    }

    /* The internal temperature sensor is always active */
    /* ucd90160 have no temperature */
    /* info->func[0] = PMBUS_HAVE_TEMP; */

    /* Everything else is configurable */
    ret = wb_i2c_smbus_read_block_data(client, UCD9000_MONITOR_CONFIG,
                    block_buffer);
    if (ret <= 0) {
        dev_err(&client->dev, "Failed to read configuration data, ret:%d.\n", ret);
        return -ENODEV;
    }
    for (i = 0; i < ret; i++) {
        int page = UCD9000_MON_PAGE(block_buffer[i]);

        if (page >= info->pages)
            continue;

        switch (UCD9000_MON_TYPE(block_buffer[i])) {
        case UCD9000_MON_VOLTAGE:
        case UCD9000_MON_VOLTAGE_HW:
            info->func[page] |= PMBUS_HAVE_VOUT
              | PMBUS_HAVE_STATUS_VOUT;
            break;
        case UCD9000_MON_TEMPERATURE:
            info->func[page] |= PMBUS_HAVE_TEMP2
              | PMBUS_HAVE_STATUS_TEMP;
            break;
        case UCD9000_MON_CURRENT:
            info->func[page] |= PMBUS_HAVE_IOUT
              | PMBUS_HAVE_STATUS_IOUT;
            break;
        default:
            break;
        }
    }

    /* Fan configuration */
    if (mid->driver_data == ucd90124) {
        for (i = 0; i < UCD9000_NUM_FAN; i++) {
            i2c_smbus_write_byte_data(client,
                          UCD9000_FAN_CONFIG_INDEX, i);
            ret = wb_i2c_smbus_read_block_data(client,
                            UCD9000_FAN_CONFIG,
                            data->fan_data[i]);
            if (ret < 0)
                return ret;
        }
        i2c_smbus_write_byte_data(client, UCD9000_FAN_CONFIG_INDEX, 0);

        info->read_byte_data = ucd9000_read_byte_data;
        info->func[0] |= PMBUS_HAVE_FAN12 | PMBUS_HAVE_STATUS_FAN12
          | PMBUS_HAVE_FAN34 | PMBUS_HAVE_STATUS_FAN34;
    }

    ucd9000_probe_gpio(client, mid, data);

    ret = wb_pmbus_do_probe(client, info);
    if (ret) {
        dev_info(&client->dev, "ucd9000 pmbus probe error %d\n", ret);
        return ret;
    }

    ret = ucd9000_init_debugfs(client, mid, data);
    if (ret)
        dev_warn(&client->dev, "Failed to register debugfs: %d\n",
             ret);

    ret = sysfs_create_group(&client->dev.kobj, &ucd9000_sysfs_group);
    if (ret != 0) {
        wb_pmbus_do_remove(client);
        dev_err(&client->dev, "sysfs_create_group error\n");
        return ret;
    }

    INIT_DELAYED_WORK(&data->dwork_set_time, set_date_work_function);
    if (dwork_set_time_enable) {
        schedule_delayed_work(&data->dwork_set_time, HZ * 3);
    }

    return 0;
}

static void ucd9000_remove(struct i2c_client *client)
{
    struct ucd9000_data *data = to_ucd9000_data(wb_pmbus_get_driver_info(client));

    cancel_delayed_work_sync(&data->dwork_set_time);
    sysfs_remove_group(&client->dev.kobj, &ucd9000_sysfs_group);
    (void)wb_pmbus_do_remove(client);
    return;
}

/* This is the driver that will be inserted */
static struct i2c_driver ucd9000_driver = {
    .driver = {
        .name = "wb_ucd9000",
        .of_match_table = of_match_ptr(ucd9000_of_match),
    },
    .probe_new = ucd9000_probe,
    .remove = ucd9000_remove,
    .id_table = ucd9000_id,
};

module_i2c_driver(ucd9000_driver);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("PMBus driver for TI UCD90xxx");
MODULE_LICENSE("GPL");
