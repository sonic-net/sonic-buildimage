/*
 * wb_ucd9081.c
 *
 * This module create sysfs to get voltage
 * through ucd9081 I2C address.
 *
 * History
 *  [Version]                [Date]                    [Description]
 *   *  v1.0                2024-01-08                  Initial version
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/of.h>
#include <wb_bsp_kernel_debug.h>

#include "wb_pmbus.h"

#define WB_UCD9081_RAIL1H               (0x00)    /* Channel 1 voltage address, high 8 bits */
#define WB_UCD9081_RAIL1L               (0x01)    /* Channel 1 voltage address, low 8 bits */
#define WB_UCD9081_RAIL2H               (0x02)    /* Channel 2 voltage address, high 8 bits */
#define WB_UCD9081_RAIL2L               (0x03)    /* Channel 2 voltage address, low 8 bits */
#define WB_UCD9081_RAIL3H               (0x04)    /* Channel 3 voltage address, high 8 bits */
#define WB_UCD9081_RAIL3L               (0x05)    /* Channel 3 voltage address, low 8 bits */
#define WB_UCD9081_RAIL4H               (0x06)    /* Channel 4 voltage address, high 8 bits */
#define WB_UCD9081_RAIL4L               (0x07)    /* Channel 4 voltage address, low 8 bits */
#define WB_UCD9081_RAIL5H               (0x08)    /* Channel 5 voltage address, high 8 bits */
#define WB_UCD9081_RAIL5L               (0x09)    /* Channel 5 voltage address, low 8 bits */
#define WB_UCD9081_RAIL6H               (0x0a)    /* Channel 6 voltage address, high 8 bits */
#define WB_UCD9081_RAIL6L               (0x0b)    /* Channel 6 voltage address, low 8 bits */
#define WB_UCD9081_RAIL7H               (0x0c)    /* Channel 7 voltage address, high 8 bits */
#define WB_UCD9081_RAIL7L               (0x0d)    /* Channel 7 voltage address, low 8 bits */
#define WB_UCD9081_WADDR1               (0x30)    /* UCD9081 Low address register write address, low 8 bits */
#define WB_UCD9081_WADDR2               (0x31)    /* UCD9081 High address register write address, high 8 bits */
#define WB_UCD9081_WDATA1               (0x32)    /* Write WADDR data,low 8 bits */
#define WB_UCD9081_WDATA2               (0x33)    /* Write WADDR data,high 8 bits */

#define WB_UCD9081_FLASHLOCK_REG        (0x2E)
#define WB_UCD9081_FLASHUNLOCK_VAL      (0x02)
#define WB_UCD9081_FLASHLOCK_VAL        (0x0)

#define WB_UCD9081_FLASHLOCK_REFERENCESELECT_REG_H        (0xE1)
#define WB_UCD9081_FLASHLOCK_REFERENCESELECT_REG_L        (0x86)

#define WB_UCD9081_VERSION_START_REG        (0x1080)
#define WB_UCD9081_VERSION_REG_LEN          (8)
#define WB_UCD9081_VERSION_MASK             (0xFF)

/* Voltage definition */
#define WB_UCD9081_SELREF_OFFSET    (13)
#define WB_UCD9081_SELREF_0         (0x0)   /* External reference selected (VCC 3.3V) */
#define WB_UCD9081_SELREF_1         (0x1)   /* Internal reference selected (VCC 2.5V) */
#define WB_UCD9081_VREF_EXTERNAL    (3300)  /* 3.3V */
#define WB_UCD9081_VREF_INTERNAL    (2500)  /* 2.5V */
#define WB_UCD9081_VOLTAGE_MASK     (0x3ff)
#define WB_UCD9081_VOLTAGE_DIVIDE   (1024)

#define WB_I2C_RETRY_TIME               (10)
#define WB_I2C_RETRY_SLEEP_TIME         (10000)   /* 10ms */

/* UCD9081 error record register; every 6 entries form a group of error records */
#define WB_UCD9081_ERROR_REG            (0x20)
/* UCD9081 rail error status register; an identifier will be present if any rail is abnormal */
#define WB_UCD9081_RAILSTATUS           (0x29)
/* UCD9081 high address register write address, low 8 bits */
#define WB_UCD9081_WADDR1               (0x30)
/* UCD9081 high address register write address, high 8 bits */
#define WB_UCD9081_WADDR2               (0x31)
/* Data written to WADDR, low 8 bits */
#define WB_UCD9081_WDATA1               (0x32)
/* Data written to WADDR, high 8 bits */
#define WB_UCD9081_WDATA2               (0x33)
/* Exception record address, low 8 bits */
#define WB_UCD9081_ERR_WADDR1           (0x00)
/* Exception record address, high 8 bits */
#define WB_UCD9081_ERR_WADDR2           (0x10)

#define WB_UCD9081_ERR_LEN              (48)
#define WB_UCD9081_ERR_ONE_LEN          (6)
#define WB_UCD9081_RAM_ERR_LEN          (24)

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

#define WB_UCD9081_CLR_ERR_WADDR1       (0x7E)    /* Exception record address, low 8 bits */
#define WB_UCD9081_CLR_ERR_WADDR2       (0x10)    /* Exception record address, high 8 bits */
#define WB_UCD9081_CLR_ERR_WDATA1       (0xDC)    /* Data written to clear ERR, low 8 bits */
#define WB_UCD9081_CLR_ERR_WDATA2       (0xBA)    /* Data written to clear ERR, high 8 bits */

#define WB_UCD9081_TX_LEN               (64)
#define WB_UCD9081_ERR_FLAG_READ        (0)
#define WB_UCD9081_ERR_FLAG_READ_CLEAR  (1)
#define WB_UCD9081_NO_ERR_RECORD_VAL    (0xff)
#define WB_UCD9081_NO_RAM_ERR_RECORD_VAL    (0x0)

#define WB_UCD9081_ERROR_CODE_MASK      (0x7)
#define WB_UCD9081_ERROR_CODE_OFF       (5)
#define WB_UCD9081_RAIL_MASK            (0x7)
#define WB_UCD9081_RAIL_OFF             (2)

#define WB_UCD9081_DATA_HIGH_MASK       (0x3)
#define WB_UCD9081_DATA_LOW_MASK        (0xff)
#define WB_UCD9081_LSB                  (322)
#define WB_UCD9081_VOLTAGE_TO_V         (100000)
#define WB_UCD9081_DATE_MS_HIGH_MASK    (0x3)
#define WB_UCD9081_DATE_SEC_MASK        (0x3f)
#define WB_UCD9081_DATE_SEC_OFF         (2)

static char *error_type_string[] = {
    "Null alarm",
    "Supply did not start",                         /* No input */
    "Sustained overvoltage detected",               /* Persistent overvoltage */
    "Sustained undervoltage detected",              /* Persistent undervoltage */
    "Overvoltage glitch detected",                  /* Overvoltage */
    "Undervoltage glitch detected",                 /* Undervoltage */
    "Reserved",
    "Reserved",
};

struct ucd9081_data {
    struct i2c_client   *client;
    struct device       *hwmon_dev;
    struct mutex        update_lock;
    u32                 vref;               /* Voltage unit */
    uint8_t err_record[WB_UCD9081_ERR_LEN];
    int err_flag;
    int ram_err_flag;                       /* The rail error flag is stored in the device RAM */
    /* Rail errors are stored in the device RAM. */
    uint8_t err_record_ram[WB_UCD9081_RAM_ERR_LEN];
};

static s32 wb_i2c_smbus_read_byte_data(const struct i2c_client *client, u8 command)
{
    int i;
    s32 ret;

    ret = 0;
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
        if (ret == 0) {
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
        if (ret == 0) {
            return ret;
        }
        usleep_range(WB_I2C_RETRY_SLEEP_TIME, WB_I2C_RETRY_SLEEP_TIME + 1);
    }
    return ret;
}

static s32 wb_i2c_smbus_read_i2c_block_data(const struct i2c_client *client, u8 command, u8 length,
               u8* values)
{
    int i;
    s32 ret;

    ret = 0;
    for (i = 0; i < WB_I2C_RETRY_TIME; i++) {
        ret = i2c_smbus_read_i2c_block_data(client, command, length, values);
        if (ret >= 0) {
            return ret;
        }
        usleep_range(WB_I2C_RETRY_SLEEP_TIME, WB_I2C_RETRY_SLEEP_TIME + 1);
    }
    return ret;
}

/* Get 9081 voltage units */
static int ucd9081_get_vref(struct i2c_client *client) 
{
    int ret;
    uint16_t wr_val;
    uint16_t ori_addr;
    uint16_t reference_select_val;
    struct ucd9081_data *data;

    data = i2c_get_clientdata(client);
    DEBUG_VERBOSE("%d-%04x: enter ucd9081_get_vref\n", client->adapter->nr,
        client->addr);
    
    mutex_lock(&data->update_lock);
    /* 0.Backup original WADDR */
    ret = wb_i2c_smbus_read_word_data(client, WB_UCD9081_WADDR1);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read ucd9081 origin addr failed, ret: %d\n", client->adapter->nr,
            client->addr, ret);
        goto error;
    }
    ori_addr = ret;
    DEBUG_VERBOSE("%d-%04x: save ucd9081 waddr success, ori_addr: 0x%x\n",
            client->adapter->nr, client->addr, ori_addr);

    /* 1.Unlock */
    ret = wb_i2c_smbus_write_byte_data(client, WB_UCD9081_FLASHLOCK_REG, WB_UCD9081_FLASHUNLOCK_VAL);
    if (ret) {
        DEBUG_ERROR("%d-%04x: ucd9081 unlock failed\n", client->adapter->nr,
            client->addr);
        goto error;
    }

    /* 2. Write Voltage configuration flash register address 0xE186 address to WADDR */
    wr_val = (WB_UCD9081_FLASHLOCK_REFERENCESELECT_REG_H << 8) | WB_UCD9081_FLASHLOCK_REFERENCESELECT_REG_L;
    ret = wb_i2c_smbus_write_word_data(client, WB_UCD9081_WADDR1, wr_val);
    if (ret) {
        DEBUG_ERROR("%d-%04x: write ucd9081 waddr failed\n", client->adapter->nr,
            client->addr);
        goto error;
    }

    /* 3. The voltage configuration flash register value is read through WDATA*/
    reference_select_val = wb_i2c_smbus_read_word_data(client, WB_UCD9081_WDATA1);
    if (reference_select_val < 0) {
        DEBUG_ERROR("%d-%04x: read ucd9081 wdata failed, ret: %d\n", client->adapter->nr,
            client->addr, ret);
        ret = reference_select_val;
        goto error;
    }

    /* 4.LOCK */
    ret = wb_i2c_smbus_write_byte_data(client, WB_UCD9081_FLASHLOCK_REG, WB_UCD9081_FLASHLOCK_VAL);
    if (ret) {
        DEBUG_ERROR("%d-%04x: ucd9081 flash lock failed\n", client->adapter->nr,
            client->addr);
        goto error;
    }

    /* 5.Restore the original WADDR address */
    ret = wb_i2c_smbus_write_word_data(client, WB_UCD9081_WADDR1, ori_addr);
    if (ret) {
        DEBUG_ERROR("%d-%04x: recover ucd9081 waddr failed\n", client->adapter->nr,
            client->addr);
        goto error;
    }

    /* 5.Calculated voltage unit*/
    DEBUG_VERBOSE("%d-%04x: ucd9081 reference_select_val: 0x%x\n",
                client->adapter->nr, client->addr, reference_select_val);
    if ((reference_select_val >> WB_UCD9081_SELREF_OFFSET) == WB_UCD9081_SELREF_0) {
        data->vref = WB_UCD9081_VREF_EXTERNAL;
    } else {
        data->vref = WB_UCD9081_VREF_INTERNAL;
    }
    
    mutex_unlock(&data->update_lock);
    DEBUG_VERBOSE("%d-%04x: ucd9081 use vref: %d\n",
                client->adapter->nr, client->addr, data->vref);
    return 0;

error:
    mutex_unlock(&data->update_lock);
    return ret;
}

static ssize_t ucd9081_voltage_show(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret;
    struct ucd9081_data *data;
    struct i2c_client *client;
    uint32_t index, channel, value;
    long voltage;

    data = dev_get_drvdata(dev);
    client = data->client;
    index = to_sensor_dev_attr_2(da)->index;
    channel = to_sensor_dev_attr_2(da)->nr;

    mutex_lock(&data->update_lock);
    ret = wb_i2c_smbus_read_word_data(client, index);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read ucd9081 channel%d voltage reg failed, reg: 0x%x ret: %d\n", client->adapter->nr,
            client->addr, channel, index, ret);
        goto error;
    }
    value = ret;
    /* Terminal conversion */
    value = ((value & 0xff00) >> 8) | ((value & 0xff) << 8);
    DEBUG_VERBOSE("%d-%04x: read ucd9081 channel%d voltage success, reg: 0x%x, value: 0x%x\n",
            client->adapter->nr, client->addr, channel, index, value);

    voltage = ((value & WB_UCD9081_VOLTAGE_MASK) * data->vref) / WB_UCD9081_VOLTAGE_DIVIDE;
    DEBUG_VERBOSE("%d-%04x: ucd9081 channel%d voltage: %ld\n", client->adapter->nr, client->addr, channel, voltage);
    mutex_unlock(&data->update_lock);
    return snprintf(buf, PAGE_SIZE, "%ld\n", voltage);
error:
    mutex_unlock(&data->update_lock);
    return ret;
}

/* Get 9081 version */
static ssize_t ucd9081_version_show(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret, i;
    struct ucd9081_data *data;
    struct i2c_client *client;
    uint32_t value;
    int offset = 0;

    data = dev_get_drvdata(dev);
    client = data->client;

    mutex_lock(&data->update_lock);
    for (i = 0; i < WB_UCD9081_VERSION_REG_LEN; i++) {
        ret = wb_i2c_smbus_write_word_data(client, WB_UCD9081_WADDR1, (WB_UCD9081_VERSION_START_REG + i));
        if (ret) {
            DEBUG_ERROR("%d-%04x: write ucd9081 waddr failed, write value : 0x%x\n", client->adapter->nr,
                client->addr, (WB_UCD9081_VERSION_START_REG + i));
            goto error;
        }
        ret = wb_i2c_smbus_read_word_data(client, WB_UCD9081_WDATA1);
        if (ret < 0) {
            DEBUG_ERROR("%d-%04x: read ucd9081 wdata failed, read reg : 0x%x, ret: %d\n", client->adapter->nr,
                client->addr, (WB_UCD9081_VERSION_START_REG + i), ret);
            goto error;
        }
        value = ret;
        DEBUG_VERBOSE("%d-%04x: read ucd9081 version reg success, reg: 0x%x, value: 0x%x\n",
            client->adapter->nr, client->addr, (WB_UCD9081_VERSION_START_REG + i), value);

        offset += scnprintf(buf + offset, PAGE_SIZE - offset, "%c", (unsigned char)(value & WB_UCD9081_VERSION_MASK));
    }
    offset += scnprintf(buf + offset, PAGE_SIZE - offset, "\n");
    mutex_unlock(&data->update_lock);
    return offset;

error:
    mutex_unlock(&data->update_lock);
    return ret;
}

static int ucd9081_fail_data_analy(uint8_t *data, int len, char *buf)
{
    int i, index;
    int size;
    uint32_t tmp;
    uint32_t vol_value;
    int count;
    uint32_t hour;
    uint32_t min;
    uint32_t sec;
    uint32_t millisec;
    int j;

    if (len % WB_UCD9081_ERR_ONE_LEN) {
        DEBUG_ERROR("param err, len %d\n", len);
        return -EINVAL;
    }

    /* The input parameter could be 48 or 24. */
    count = len / WB_UCD9081_ERR_ONE_LEN;
    size = 0;
    /* len/WB_UCD9081_ERR_ONE_LEN: Each fault consists of 6 bytes. */
    for (i = 0; i < count; i++) {
        index = i * WB_UCD9081_ERR_ONE_LEN;
        /* If the first byte is all 0 or all FF, it means there is no error in the task. */
        if (data[index] == 0xff || data[index] == 0) {
            continue;
        }
        size += snprintf(buf + size, PAGE_SIZE - size, "ucd9081 blackbox:%d\n", i + 1);
        tmp = ((data[index] >> WB_UCD9081_ERROR_CODE_OFF) & WB_UCD9081_ERROR_CODE_MASK);
        size += snprintf(buf + size, PAGE_SIZE - size, "error type:%s\n", error_type_string[tmp]);
        tmp = ((data[index] >> WB_UCD9081_RAIL_OFF) & WB_UCD9081_RAIL_MASK);
        size += snprintf(buf + size, PAGE_SIZE - size, "error rail:%u\n", tmp);
        /* Voltage calculation: data * 3.22 mV */
        tmp = ((data[index] & WB_UCD9081_DATA_HIGH_MASK) << 8) + data[index + 1];
        vol_value = tmp * WB_UCD9081_LSB;
        size += snprintf(buf + size, PAGE_SIZE - size, "error voltage:%d.%dv\n",
            vol_value / WB_UCD9081_VOLTAGE_TO_V, vol_value % WB_UCD9081_VOLTAGE_TO_V);
        /* Clock parsing: */
        min = data[index + 2];
        hour = data[index + 3];
        /* ms: Higher 2 bits + lower 8 bits */
        millisec = data[index + 4] +
            ((data[index + 5] & WB_UCD9081_DATE_MS_HIGH_MASK) << 8);
        sec = (data[index + 5] >> WB_UCD9081_DATE_SEC_OFF) & WB_UCD9081_DATE_SEC_MASK;
        size += snprintf(buf + size, PAGE_SIZE - size, "time:%02d:%02d:%02d %dms\n",
            hour, min, sec, millisec);
        size += snprintf(buf + size, PAGE_SIZE - size, "regs value:\n");
        for (j = 0; j < WB_UCD9081_ERR_ONE_LEN; j++) {
            size += snprintf(buf + size, PAGE_SIZE - size, "0x%x ", data[index + j]);
        }
        size += snprintf(buf + size, PAGE_SIZE - size, "\n");
    }

    return size;
}

static int ucd9081_clear_avs_err(struct i2c_client *client)
{
    int ret;
    u16 ori_addr;
    s32 wr_val;
    struct ucd9081_data *data;

    data = i2c_get_clientdata(client);
    DEBUG_VERBOSE("%d-%04x: enter ucd9081_clear_avs_err\n", client->adapter->nr,
        client->addr);

    /*
     * The following steps all involve writing to registers, 
     * and if any one of them fails, the rollback action will also be ineffective. 
     * Therefore, if any step fails, no rollback actions will be performed.
     */
    /* 0.Back up the original WADDR. */
    ret = wb_i2c_smbus_read_word_data(client, WB_UCD9081_WADDR1);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read ucd9081 origin addr failed, ret: %d\n", client->adapter->nr,
            client->addr, ret);
        goto clear_finish;
    }
    ori_addr = (u16)ret;
    DEBUG_VERBOSE("%d-%04x: save ucd9081 waddr success, ori_addr0: 0x%x\n",
            client->adapter->nr, client->addr, ori_addr);

    /* 1.Unlock */
    ret = wb_i2c_smbus_write_byte_data(client, WB_UCD9081_FLASHLOCK_REG, 
            WB_UCD9081_FLASHUNLOCK_VAL);
    if (ret) {
        DEBUG_ERROR("%d-%04x: ucd9081 unlock failed\n", client->adapter->nr,
            client->addr);
        goto clear_finish;
    }

    /* 2. Write 0xBADC to address 0x1000. */
    /* 2.1 Offset WADDR to 0x1000. */
    wr_val = (WB_UCD9081_ERR_WADDR2 << 8) | WB_UCD9081_ERR_WADDR1;
    ret = wb_i2c_smbus_write_word_data(client, WB_UCD9081_WADDR1, wr_val);
    if (ret) {
        DEBUG_ERROR("%d-%04x: write ucd9081 waddr failed\n", client->adapter->nr,
            client->addr);
        goto clear_finish;
    }
    /* 2.2 Write 0xBADC to WDATA. */
    wr_val = (WB_UCD9081_CLR_ERR_WDATA2 << 8) | WB_UCD9081_CLR_ERR_WDATA1;
    ret = wb_i2c_smbus_write_word_data(client, WB_UCD9081_WDATA1, wr_val);
    if (ret) {
        DEBUG_ERROR("%d-%04x: write ucd9081 wdata failed\n", client->adapter->nr,
            client->addr);
        goto clear_finish;
    }

    /* 3. Write 0xBADC to 0x107E. */
    /* 3.1 Offset WADDR to 0x107E. */
    wr_val = (WB_UCD9081_CLR_ERR_WADDR2 << 8) | WB_UCD9081_CLR_ERR_WADDR1;
    ret = wb_i2c_smbus_write_word_data(client, WB_UCD9081_WADDR1, wr_val);
    if (ret) {
        DEBUG_ERROR("%d-%04x: write ucd9081 waddr failed\n", client->adapter->nr,
            client->addr);
        goto clear_finish;
    }
    /* 3.2 Write 0xBADC to WDATA. */
    wr_val = (WB_UCD9081_CLR_ERR_WDATA2 << 8) | WB_UCD9081_CLR_ERR_WDATA1;
    ret = wb_i2c_smbus_write_word_data(client, WB_UCD9081_WDATA1, wr_val);
    if (ret) {
        DEBUG_ERROR("%d-%04x: write ucd9081 wdata failed\n", client->adapter->nr,
            client->addr);
        goto clear_finish;
    }

    /* 4.Lock. */
    ret = wb_i2c_smbus_write_byte_data(client, WB_UCD9081_FLASHLOCK_REG, WB_UCD9081_FLASHLOCK_VAL);
    if (ret) {
        DEBUG_ERROR("%d-%04x: ucd9081 flash lock failed\n", client->adapter->nr,
            client->addr);
        goto clear_finish;
    }

    /* 5.Restore the original WADDR address. */
    ret = wb_i2c_smbus_write_word_data(client, WB_UCD9081_WADDR1, ori_addr);
    if (ret) {
        DEBUG_ERROR("%d-%04x: recover ucd9081 waddr failed\n", client->adapter->nr,
            client->addr);
    } else {
        data->err_flag = 0;
        DEBUG_VERBOSE("%d-%04x: ucd9081 clear avs error record success\n",
            client->adapter->nr, client->addr);
    }
clear_finish:
    return ret;
}

static int ucd9081_read_avs_err(struct i2c_client *client)
{
    int ret;
    struct ucd9081_data *data;
    u16 ori_addr, wr_val;
    uint8_t err_record[WB_UCD9081_ERR_LEN];

    DEBUG_VERBOSE("enter ucd9081_read_avs_err\n");
    data = i2c_get_clientdata(client);

    /*
     * The following steps all involve writing to registers, 
     * and if any one of them fails, the rollback action will also be ineffective. 
     * Therefore, if any step fails, no rollback actions will be performed.
     */
    /* 1.Back up the original WADDR. */
    ret = wb_i2c_smbus_read_word_data(client, WB_UCD9081_WADDR1);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read ucd9081 origin addr1 failed, ret: %d\n", client->adapter->nr,
            client->addr, ret);
        goto show_finish;
    }
    ori_addr = (u16)ret;
    DEBUG_VERBOSE("%d-%04x: save ucd9081 waddr success, ori_addr: 0x%x\n",
            client->adapter->nr, client->addr, ori_addr);

    /* 2. Offset WADDR to address 0x1000. */
    wr_val = (WB_UCD9081_ERR_WADDR2 << 8) | WB_UCD9081_ERR_WADDR1;
    ret = wb_i2c_smbus_write_word_data(client, WB_UCD9081_WADDR1, wr_val);
    if (ret) {
        DEBUG_ERROR("%d-%04x: write ucd9081 waddr failed, ret: %d\n", client->adapter->nr,
            client->addr, ret);
        goto show_finish;
    }

    /* 3.Record 48 bytes of error information. */
    mem_clear(err_record, sizeof(err_record));
    ret = wb_i2c_smbus_read_i2c_block_data(client, WB_UCD9081_WDATA1, WB_UCD9081_ERR_LEN,
            err_record);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read ucd9081 err record failed, ret: %d\n", client->adapter->nr,
            client->addr, ret);
        goto show_finish;
    }
    DEBUG_VERBOSE("%d-%04x: record ucd9081 err, ret: %d\n", client->adapter->nr, client->addr, ret);
    memcpy(data->err_record, err_record, WB_UCD9081_ERR_LEN);

    /* 4.Restore the original WADDR address. */
    ret = wb_i2c_smbus_write_word_data(client, WB_UCD9081_WADDR1, ori_addr);
    if (ret) {
        DEBUG_ERROR("%d-%04x: write ucd9081 waddr failed\n", client->adapter->nr, client->addr);
        goto show_finish;
    }
    ret = WB_UCD9081_ERR_LEN;

show_finish:
    return ret;
}

/* 
 * Reading the error flag of the 9081 monitoring channel; 
 * this register is cleared after reading by default. 
 */
static int ucd9081_read_rail_err_flag(struct i2c_client *client)
{
    int ret;
    struct ucd9081_data *data;
    uint8_t err_flag;

    DEBUG_VERBOSE("enter ucd9081_read_rail_err_flag\n");
    data = i2c_get_clientdata(client);
    /* Reset the flag to 0. */
    data->ram_err_flag = 0;

    /* Obtain the error flag. */
    ret = wb_i2c_smbus_read_byte_data(client, WB_UCD9081_RAILSTATUS);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read ucd9081 RAILSTATUS failed, ret: %d\n", client->adapter->nr,
            client->addr, ret);
        return ret;
    }
    err_flag = (uint8_t)ret;
    if (err_flag == 0) {
        data->ram_err_flag = 0;
        DEBUG_VERBOSE("%d-%04x: ucd9081 has not rail err, ram_err_flag: %d, err_flag = 0x%x\n",
            client->adapter->nr, client->addr, data->ram_err_flag, err_flag);
    } else {
        data->ram_err_flag = 1;
        DEBUG_VERBOSE("%d-%04x: ucd9081 has rail err, ram_err_flag: %d, err_flag = 0x%x\n",
            client->adapter->nr, client->addr, data->ram_err_flag, err_flag);
    }

    return 0;
}

/* Indicates whether there are error records in the flash. */
static ssize_t ucd9081_avs_err_flag_show(struct device *dev, struct device_attribute *da, char *buf)
{
    int ret, err;
    struct i2c_client *client;
    struct ucd9081_data *data;
    struct sensor_device_attribute *attr;

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);
    attr = to_sensor_dev_attr(da);

    mem_clear(buf, PAGE_SIZE);
    mutex_lock(&data->update_lock);
    /* Clear the cached data first. */
    memset(data->err_record , WB_UCD9081_NO_ERR_RECORD_VAL, WB_UCD9081_ERR_LEN);
    /* Read the UCD9081 error records. */
    ret = ucd9081_read_avs_err(client);
    if (ret != WB_UCD9081_ERR_LEN) {
        DEBUG_ERROR("%d-%04x: ucd9081 read avs err record failed, ret: %d\n",
            client->adapter->nr, client->addr, ret);
        mutex_unlock(&data->update_lock);
        return -EIO;
    }
    /* If the first byte is 0xFF, it means there are no error records. */
    if (data->err_record[0] == WB_UCD9081_NO_ERR_RECORD_VAL) {
        err = 0;
        DEBUG_VERBOSE("%d-%04x: ucd9081 has not err record, ret: %d, err_record[0] = 0x%x\n",
            client->adapter->nr, client->addr, ret, data->err_record[0]);
    } else {
        err = 1;
        DEBUG_VERBOSE("%d-%04x: ucd9081 has err record, ret: %d, err_record[0] = 0x%x\n",
            client->adapter->nr, client->addr, ret, data->err_record[0]);
    }
    data->err_flag = err;
    /* If there are error records and the read-clear mode is enabled, then clear the errors. */
    if ((err == 1) && (attr->index == WB_UCD9081_ERR_FLAG_READ_CLEAR)) {
        DEBUG_VERBOSE("%d-%04x: ucd9081 read avs_err_flag used read clear mode, try to clear error record\n",
            client->adapter->nr, client->addr);
        ucd9081_clear_avs_err(client);
    }
    mutex_unlock(&data->update_lock);
    return scnprintf(buf, WB_UCD9081_ERR_LEN, "%d\n", err);
}

static ssize_t ucd9081_avs_err_show_func(struct device *dev, struct device_attribute *da,
                char *buf, int is_analy)
{
    struct i2c_client *client;
    struct ucd9081_data *data;
    int ret;
    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);

    DEBUG_VERBOSE("enter ucd9081_avs_err_show\n");
    mem_clear(buf, PAGE_SIZE);
    mutex_lock(&data->update_lock);
    if (is_analy == 0) {
        memcpy(buf, data->err_record, WB_UCD9081_ERR_LEN);
        ret = WB_UCD9081_ERR_LEN;
    } else {
        ret = ucd9081_fail_data_analy(data->err_record, WB_UCD9081_ERR_LEN, buf);
    }
    mutex_unlock(&data->update_lock);

    return ret;
}

static ssize_t ucd9081_avs_err_show_analy(struct device *dev, struct device_attribute *da,
                char *buf)
{
    return ucd9081_avs_err_show_func(dev, da, buf, 1);
}

/* Read error information from the 9081 monitoring channel. */
static int ucd9081_read_rail_err_data(struct i2c_client *client)
{
    int ret;
    struct ucd9081_data *data;
    uint8_t err_record[WB_UCD9081_RAM_ERR_LEN];

    DEBUG_VERBOSE("enter ucd9081_read_rail_err_data\n");
    data = i2c_get_clientdata(client);

    /* 1.Record 24 bytes of error information (which corresponds to 4 sets of error information). */
    mem_clear(err_record, sizeof(err_record));
    ret = wb_i2c_smbus_read_i2c_block_data(client, WB_UCD9081_ERROR_REG, WB_UCD9081_RAM_ERR_LEN,
            err_record);
    if (ret < 0) {
        DEBUG_ERROR("%d-%04x: read ucd9081 rail err record failed, ret: %d\n", client->adapter->nr,
            client->addr, ret);
        goto show_finish;
    }
    DEBUG_VERBOSE("%d-%04x: record ucd9081 rail err, ret: %d\n", client->adapter->nr,
            client->addr, ret);

    memcpy(data->err_record_ram, err_record, WB_UCD9081_RAM_ERR_LEN);
    ret = WB_UCD9081_RAM_ERR_LEN;

show_finish:
    return ret;
}

/* Indicates whether there are error records in the RAM. */
static ssize_t ucd9081_rail_err_flag_show(struct device *dev, struct device_attribute *da,
                char *buf)
{
    int ret, err;
    struct i2c_client *client;
    struct ucd9081_data *data;
    struct sensor_device_attribute *attr;

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);
    attr = to_sensor_dev_attr(da);

    mem_clear(buf, PAGE_SIZE);
    mutex_lock(&data->update_lock);
    /* Clear the cached data first. */
    mem_clear(data->err_record_ram, WB_UCD9081_RAM_ERR_LEN);
    /* Read the UCD9081 error records. */
    ret = ucd9081_read_rail_err_flag(client);
    if (ret != 0) {
        DEBUG_ERROR("%d-%04x: ucd9081 read rail err flag failed, ret: %d\n",
            client->adapter->nr, client->addr, ret);
        mutex_unlock(&data->update_lock);
        return -EIO;
    }

    /* If there is an error flag, collect the error information. */
    if (data->ram_err_flag) {
        err = 1;
        ret = ucd9081_read_rail_err_data(client);
        if (ret != WB_UCD9081_RAM_ERR_LEN) {
            DEBUG_ERROR("%d-%04x: ucd9081 read rail err data failed, ret: %d\n",
                client->adapter->nr, client->addr, ret);
            mutex_unlock(&data->update_lock);
            return -EIO;
        }
    } else {
        err = 0;
    }

    mutex_unlock(&data->update_lock);
    return scnprintf(buf, WB_UCD9081_RAM_ERR_LEN, "%d\n", err);
}

static ssize_t ucd9081_rail_err_show_func(struct device *dev, struct device_attribute *da,
                char *buf, int flage)
{
    struct i2c_client *client;
    struct ucd9081_data *data;
    int ret;

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);

    DEBUG_VERBOSE("enter ucd9081_avs_err_show\n");
    mem_clear(buf, PAGE_SIZE);
    mutex_lock(&data->update_lock);
    if (flage == 0) {
        memcpy(buf, data->err_record_ram, WB_UCD9081_RAM_ERR_LEN);
        ret = WB_UCD9081_RAM_ERR_LEN;
    } else {
        ret = ucd9081_fail_data_analy(data->err_record_ram, WB_UCD9081_RAM_ERR_LEN, buf);
    }
    mutex_unlock(&data->update_lock);
    return (ssize_t)ret;
}

static ssize_t ucd9081_avs_err_show(struct device *dev, struct device_attribute *da, char *buf)
{
    return ucd9081_avs_err_show_func(dev, da, buf, 0);
}

static ssize_t ucd9081_rail_err_show(struct device *dev, struct device_attribute *da, char *buf)
{
    return ucd9081_rail_err_show_func(dev, da, buf, 0);
}

static ssize_t ucd9081_rail_err_show_analy(struct device *dev, struct device_attribute *da,
                char *buf)
{
    return ucd9081_rail_err_show_func(dev, da, buf, 1);
}

static ssize_t ucd9081_avs_err_clear(struct device *dev, struct device_attribute *da,
                   const char *buf, size_t count)
{
    int ret, err;
    struct i2c_client *client;
    struct ucd9081_data *data;
    unsigned long val;

    client = to_i2c_client(dev);
    data = i2c_get_clientdata(client);

    err = kstrtoul(buf, 0, &val);
    if (err) {
        dev_info(&client->dev, "kstrtoul failed, err = %d\n", err);
        return err;
    }

    if (val != 0) {
        dev_info(&client->dev, "please enter 0 to clear fault_record, val = %ld\n", val);
        return -EINVAL;
    }

    mutex_lock(&data->update_lock);
    if (data->err_flag == 0) {
        DEBUG_VERBOSE("%d-%04x: ucd9081 has no error record, don't need to clear\n",
            client->adapter->nr, client->addr);
        mutex_unlock(&data->update_lock);
        return count;
    }
    ret = ucd9081_clear_avs_err(client);
    if (ret == 0) { /* Successfully cleared UCD9081 error records, and also cleared the cache. */
        memset(data->err_record, WB_UCD9081_NO_ERR_RECORD_VAL, WB_UCD9081_ERR_LEN);
        DEBUG_VERBOSE("%d-%04x: ucd9081 clear avs error record success\n",
            client->adapter->nr, client->addr);
        mutex_unlock(&data->update_lock);
        return count;
    }

    DEBUG_ERROR("%d-%04x: ucd9081 clear avs error record failed, ret: %d\n",
        client->adapter->nr, client->addr, ret);
    mutex_unlock(&data->update_lock);
    return -EIO;
}

static SENSOR_DEVICE_ATTR_2(in1_input, S_IRUGO, ucd9081_voltage_show, NULL, 1, WB_UCD9081_RAIL1H);
static SENSOR_DEVICE_ATTR_2(in2_input, S_IRUGO, ucd9081_voltage_show, NULL, 2, WB_UCD9081_RAIL2H);
static SENSOR_DEVICE_ATTR_2(in3_input, S_IRUGO, ucd9081_voltage_show, NULL, 3, WB_UCD9081_RAIL3H);
static SENSOR_DEVICE_ATTR_2(in4_input, S_IRUGO, ucd9081_voltage_show, NULL, 4, WB_UCD9081_RAIL4H);
static SENSOR_DEVICE_ATTR_2(in5_input, S_IRUGO, ucd9081_voltage_show, NULL, 5, WB_UCD9081_RAIL5H);
static SENSOR_DEVICE_ATTR_2(in6_input, S_IRUGO, ucd9081_voltage_show, NULL, 6, WB_UCD9081_RAIL6H);
static SENSOR_DEVICE_ATTR_2(in7_input, S_IRUGO, ucd9081_voltage_show, NULL, 7, WB_UCD9081_RAIL7H);

static struct attribute *ucd9081_hwmon_attrs[] = {
    &sensor_dev_attr_in1_input.dev_attr.attr,
    &sensor_dev_attr_in2_input.dev_attr.attr,
    &sensor_dev_attr_in3_input.dev_attr.attr,
    &sensor_dev_attr_in4_input.dev_attr.attr,
    &sensor_dev_attr_in5_input.dev_attr.attr,
    &sensor_dev_attr_in6_input.dev_attr.attr,
    &sensor_dev_attr_in7_input.dev_attr.attr,
    NULL
};
ATTRIBUTE_GROUPS(ucd9081_hwmon);

static SENSOR_DEVICE_ATTR(version, S_IRUGO, ucd9081_version_show, NULL, 0);
static SENSOR_DEVICE_ATTR(avs_err_flag, S_IRUGO , ucd9081_avs_err_flag_show, NULL, 
        WB_UCD9081_ERR_FLAG_READ);
static SENSOR_DEVICE_ATTR(avs_err_flag_rc, S_IRUGO , ucd9081_avs_err_flag_show, NULL, 
        WB_UCD9081_ERR_FLAG_READ_CLEAR);
static SENSOR_DEVICE_ATTR(avs_err, S_IRUGO , ucd9081_avs_err_show, NULL, 0);
static SENSOR_DEVICE_ATTR(avs_err_analy, S_IRUGO , ucd9081_avs_err_show_analy, NULL, 0);
static SENSOR_DEVICE_ATTR(avs_err_clear, S_IWUSR, NULL, ucd9081_avs_err_clear, 0);
static SENSOR_DEVICE_ATTR(rail_status_err_flag_rc, S_IRUGO, ucd9081_rail_err_flag_show, NULL, 0);
static SENSOR_DEVICE_ATTR(rail_err, S_IRUGO, ucd9081_rail_err_show, NULL, 0);
static SENSOR_DEVICE_ATTR(rail_err_analy, S_IRUGO, ucd9081_rail_err_show_analy, NULL, 0);

static struct attribute *ucd9081_sysfs_attrs[] = {
    &sensor_dev_attr_version.dev_attr.attr,
    &sensor_dev_attr_avs_err_flag.dev_attr.attr,
    &sensor_dev_attr_avs_err_flag_rc.dev_attr.attr,
    &sensor_dev_attr_avs_err.dev_attr.attr,
    &sensor_dev_attr_avs_err_analy.dev_attr.attr,
    &sensor_dev_attr_avs_err_clear.dev_attr.attr,
    &sensor_dev_attr_rail_status_err_flag_rc.dev_attr.attr,
    &sensor_dev_attr_rail_err.dev_attr.attr,
    &sensor_dev_attr_rail_err_analy.dev_attr.attr,
    NULL
};

static struct attribute_group  ucd9081_sysfs_group = {
    .attrs = ucd9081_sysfs_attrs,
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
static int ucd9081_probe(struct i2c_client *client, const struct i2c_device_id *id)
#else
static int ucd9081_probe(struct i2c_client *client)
#endif
{
    int ret;
    struct ucd9081_data *data;

    DEBUG_VERBOSE("bus: %d, addr: 0x%02x do probe.\n", client->adapter->nr, client->addr);
    data = devm_kzalloc(&client->dev, sizeof(struct ucd9081_data), GFP_KERNEL);
    if (!data) {
        dev_err(&client->dev, "devm_kzalloc failed.\n");
        return -ENOMEM;
    }

    data->client = client;
    i2c_set_clientdata(client, data);
    mutex_init(&data->update_lock);

    ret = ucd9081_get_vref(client);
    if (ret != 0) {
        dev_err(&client->dev, "get ucd9081 vref failed, ret: %d\n", ret);
        return ret;
    }

    data->hwmon_dev = hwmon_device_register_with_groups(&client->dev, client->name, data, ucd9081_hwmon_groups);
    if (IS_ERR(data->hwmon_dev)) {
        ret = PTR_ERR(data->hwmon_dev);
        dev_err(&client->dev, "Failed to register ucd9081 hwmon device, ret: %d\n", ret);
        return ret;
    }

    ret = sysfs_create_group(&client->dev.kobj, &ucd9081_sysfs_group);
    if (ret < 0) {
        dev_err(&client->dev, "ucd9081 sysfs_create_group failed %d\n", ret);
        hwmon_device_unregister(data->hwmon_dev);
        return ret;
    }

    dev_info(&client->dev, "ucd9081 (addr:0x%x, nr:%d) probe success\n", client->addr, client->adapter->nr);
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
static int ucd9081_remove(struct i2c_client *client)
#else
static void ucd9081_remove(struct i2c_client *client)
#endif
{
    struct ucd9081_data *data;

    data = i2c_get_clientdata(client);
    dev_info(&client->dev, "ucd9081 do remove\n");

    sysfs_remove_group(&client->dev.kobj, &ucd9081_sysfs_group);
    hwmon_device_unregister(data->hwmon_dev);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
    return 0;
#endif
}

static const struct i2c_device_id ucd9081_id[] = {
    {"wb_ucd9081", 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, ucd9081_id);

static const struct of_device_id ucd9081_dev_of_match[] = {
    { .compatible = "ti,wb_ucd9081" },
    { },
};
MODULE_DEVICE_TABLE(of, ucd9081_dev_of_match);

static struct i2c_driver ucd9081_driver = {
    .class      = I2C_CLASS_HWMON,
    .driver = {
        .name   = "wb_ucd9081",
        .of_match_table = ucd9081_dev_of_match,
    },
    .probe      = ucd9081_probe,
    .remove     = ucd9081_remove,
    .id_table = ucd9081_id,
};

module_i2c_driver(ucd9081_driver);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("ucd9081 Driver");
MODULE_LICENSE("GPL");
