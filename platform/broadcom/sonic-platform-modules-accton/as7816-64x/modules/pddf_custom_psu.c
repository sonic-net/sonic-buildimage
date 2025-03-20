#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/dmi.h>
#include "pddf_psu_defs.h"

extern PSU_SYSFS_ATTR_DATA access_psu_serial_num;
extern PSU_SYSFS_ATTR_DATA access_psu_v_out;

#define MAX_MODEL_NAME     10
#define MAX_SERIAL_NUMBER  19

#define PSU_REG_VOUT_MODE 0x20
#define PSU_REG_READ_VOUT 0x8B

enum psu_type {
    PSU_TYPE_AC_D850AB_5A,
    PSU_TYPE_AC_D850AB_5B,
    PSU_TYPE_AC_YM2851F,
    PSU_TYPE_DC_YM2851J
};

struct model_name_info {
    enum psu_type type;
    u8 offset;
    u8 length;
    u8 chk_length;
    char* model_name;
};

struct serial_number_info {
    enum psu_type type;
    u8 offset;
    u8 length;
    u8 chk_length;
    char* serial_number;
};

struct model_name_info models[] = {
    { PSU_TYPE_AC_D850AB_5A, 0x15, 10, 10, "D850AB-5 A" },
    { PSU_TYPE_AC_D850AB_5B, 0x15, 10, 10, "D850AB-5 B" },
    { PSU_TYPE_AC_YM2851F, 0x20, 8, 8, "YM-2851F" },
    { PSU_TYPE_DC_YM2851J, 0x20, 8, 8, "YM-2851J" }
};

struct serial_number_info serials[] = {
    { PSU_TYPE_AC_D850AB_5A, 0x2e, 11, 11, "D850AB-5 A" },
    { PSU_TYPE_AC_D850AB_5B, 0x2e, 11, 11, "D850AB-5 B" },
    { PSU_TYPE_AC_YM2851F, 0x35, 18, 18, "YM-2851F" },
    { PSU_TYPE_DC_YM2851J, 0x35, 18, 18, "YM-2851J" }
};

struct pddf_psu_data {
    char model_name[MAX_MODEL_NAME+1];
    char serial_number[MAX_SERIAL_NUMBER+1];
};

static int pddf_psu_read_byte(struct i2c_client *client, u8 reg)
{
    int result = 0;
    int retry_count = 10;

    while (retry_count) {
        retry_count--;

        result = i2c_smbus_read_byte_data(client, reg);
        if (unlikely(result < 0)) {
            msleep(10);
            continue;
        }

        break;
    }

    return result;
}

static int pddf_psu_read_word(struct i2c_client *client, u8 reg)
{
    int result = 0;
    int retry_count = 10;

    while (retry_count) {
        retry_count--;

        result = i2c_smbus_read_word_data(client, reg);
        if (unlikely(result < 0)) {
            msleep(10);
            continue;
        }

        break;
    }

    return result;
}

static int pddf_psu_read_block(struct i2c_client *client, u8 command, u8 *data,
                                     int data_len)
{
    int result = 0;
    int retry_count = 10;

    while (retry_count) {
        retry_count--;

        result = i2c_smbus_read_i2c_block_data(client, command, data_len, data);
        if (unlikely(result < 0)) {
            msleep(10);
            continue;
        }

        if (unlikely(result != data_len)) {
            result = -EIO;
            msleep(10);
            continue;
        }

        result = 0;
        break;
    }

    return result;
}

ssize_t pddf_get_custom_psu_serial_num(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pddf_psu_data data;
    int i, status;

    for (i = 0; i < ARRAY_SIZE(models); i++) {
        memset(data.serial_number, 0, sizeof(data.serial_number));

        status = pddf_psu_read_block(client, models[i].offset,
                                           data.model_name, models[i].length);
        if (status < 0) {
            data.model_name[0] = '\0';
            dev_dbg(&client->dev, "unable to read model name from (0x%x) offset(0x%x)\n",
                                  client->addr, models[i].offset);
            return status;
        }
        else {
            data.model_name[models[i].length] = '\0';
        }

        /* Determine if the model name is known, if not, read next index
         */
        if (strncmp(data.model_name, models[i].model_name, models[i].chk_length) == 0) {
            status = pddf_psu_read_block(client, serials[i].offset,
                                           data.serial_number, serials[i].length);

            if (status < 0) {
                data.serial_number[0] = '\0';
                dev_dbg(&client->dev, "unable to read serial num from (0x%x) offset(0x%x)\n",
                                  client->addr, serials[i].offset);
                return status;
            }
            else {
                data.serial_number[serials[i].length] = '\0';
	            return sprintf(buf, "%s\n", data.serial_number);
            }

            return 0;
        }
        else {
            data.serial_number[0] = '\0';
        }
    }

    return -ENODATA;
}

static int twos_complement_to_int(u16 data, u8 valid_bit, int mask)
{
    u16  valid_data  = data & mask;
    bool is_negative = valid_data >> (valid_bit - 1);

    return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static ssize_t show_vout_linear(char *buf, u8 vout_mode, u16 read_vout)
{
    int exponent = 0, mantissa = read_vout;
    int multiplier = 1000;

    exponent = twos_complement_to_int(vout_mode, 5, 0x1f);
    return (exponent > 0) ? sprintf(buf, "%d\n", (mantissa << exponent) * multiplier) :
                            sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

static ssize_t show_vout_literal(char *buf, u16 read_vout)
{
    int exponent, mantissa;
    int multiplier = 1000;

    exponent = twos_complement_to_int(read_vout >> 11, 5, 0x1F);
    mantissa = twos_complement_to_int(read_vout & 0x7FF, 11, 0x7FF);

    return (exponent >= 0) ? sprintf(buf, "%d\n", (mantissa << exponent) * multiplier) :
           sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

ssize_t pddf_get_custom_psu_v_out(struct device *dev, struct device_attribute *da, char *buf)
{
    u8 reg = 0;
    u8 vout_mode = 0xFF;
    u16 read_vout = 0;
    int status = 0;
    struct i2c_client *client = to_i2c_client(dev);

    reg = PSU_REG_VOUT_MODE; /* PMBus VOUT_MODE */
    status = pddf_psu_read_byte(client, reg);
    if (status < 0) {
        dev_dbg(&client->dev, "reg %d, err %d\n", reg, status);
        return status;
    }
    else {
        vout_mode = status;
    }

    reg = PSU_REG_READ_VOUT; /* PMBus READ_VOUT */
    status = pddf_psu_read_word(client, reg);
    if (status < 0) {
        dev_dbg(&client->dev, "reg %d, err %d\n", reg, status);
        return status;
    }
    else {
        read_vout = status;
    }

    if ((vout_mode & 0xE0) == 0x20) {
        return -ENODATA;
    }
    else if (!(vout_mode & 0xE0)) {
        /* Linear mode */
        return show_vout_linear(buf, vout_mode, read_vout);
    }
    else {
        /* Default literal format */
        return show_vout_literal(buf, read_vout);
    }
}

static int __init pddf_custom_psu_init(void)
{
    access_psu_serial_num.show = pddf_get_custom_psu_serial_num;
    access_psu_serial_num.do_get = NULL;

    access_psu_v_out.show = pddf_get_custom_psu_v_out;
    access_psu_v_out.do_get = NULL;

	return 0;
}

static void __exit pddf_custom_psu_exit(void)
{
	return;
}

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("pddf custom psu api");
MODULE_LICENSE("GPL");

module_init(pddf_custom_psu_init);
module_exit(pddf_custom_psu_exit);
