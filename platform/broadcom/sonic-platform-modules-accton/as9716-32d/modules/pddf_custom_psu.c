
#include <linux/string.h>
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

ssize_t pddf_show_custom_psu_v_out(struct device *dev, struct device_attribute *da, char *buf);
extern PSU_SYSFS_ATTR_DATA access_psu_v_out;
ssize_t pddf_get_custom_psu_model_name(struct device *dev, struct device_attribute *da, char *buf);
ssize_t pddf_get_custom_psu_serial_num(struct device *dev, struct device_attribute *da, char *buf);
ssize_t pddf_get_custom_psu_fan_dir(struct device *dev, struct device_attribute *da, char *buf);
extern PSU_SYSFS_ATTR_DATA access_psu_model_name;
extern PSU_SYSFS_ATTR_DATA access_psu_serial_num;
extern PSU_SYSFS_ATTR_DATA access_psu_fan_dir;

#define MAX_FAN_DIR_LEN         3
#define MAX_MODEL_NAME          18
#define MAX_SERIAL_NUMBER       19
const char FAN_DIR_F2B[] = "F2B";
const char FAN_DIR_B2F[] = "B2F";

enum psu_type {
    PSU_TYPE_AC_ACBEL_FSF019_F2B, // FSF019-11G
    PSU_TYPE_AC_ACBEL_FSH082_F2B, // FSH082-610G
    PSU_TYPE_AC_ACBEL_FSH095_B2F, // FSH095-610G
    PSU_TYPE_3Y_YESM1300AM // YESM1300AM-2A01P10(F2B) or YESM1300AM-2R01P10(B2F)
};

struct model_name_info {
    enum psu_type type;
    u8 offset;
    u8 length;
    u8 chk_length;
    char* model_name;
    const char* fan_dir;
};

struct serial_number_info {
    enum psu_type type;
    u8 offset;
    u8 length;
    u8 chk_length;
    char* serial_number;
};
struct model_name_info models[] = {
    { PSU_TYPE_AC_ACBEL_FSF019_F2B, 0x15, 13, 6, "FSF019", FAN_DIR_F2B },
    { PSU_TYPE_AC_ACBEL_FSH082_F2B, 0x20, 13, 6, "FSH082", FAN_DIR_F2B },
    { PSU_TYPE_AC_ACBEL_FSH095_B2F, 0x20, 13, 6, "FSH095", FAN_DIR_B2F },
    { PSU_TYPE_3Y_YESM1300AM, 0x20, 19, 8, "YESM1300", NULL }
};

struct serial_number_info serials[] = {
    { PSU_TYPE_AC_ACBEL_FSF019_F2B, 0x2e, 16, 16, "FSF019" },
    { PSU_TYPE_AC_ACBEL_FSH082_F2B, 0x35, 18, 18, "FSH082" },
    { PSU_TYPE_AC_ACBEL_FSH095_B2F, 0x35, 18, 18, "FSH095" },
    { PSU_TYPE_3Y_YESM1300AM, 0x35, 19, 19, "YESM1300" }
};

struct pddf_psu_data {
    char model_name[MAX_MODEL_NAME+1];
    char serial_number[MAX_SERIAL_NUMBER+1];
    char fan_dir[MAX_FAN_DIR_LEN+1];
};


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

static int two_complement_to_int(u16 data, u8 valid_bit, int mask)
{
	u16  valid_data  = data & mask;
	bool is_negative = valid_data >> (valid_bit - 1);

	return is_negative ? (-(((~valid_data) & mask) + 1)) : valid_data;
}

static u8 psu_get_vout_mode(struct i2c_client *client)
{
	u8 status = 0, retry = 10;
	uint8_t offset = 0x20; // VOUT_MODE

	while (retry) {
		status = i2c_smbus_read_byte_data((struct i2c_client *)client, offset);
		if (unlikely(status < 0)) {
			msleep(60);
			retry--;
			continue;
		}
		break;
	}

	if (status < 0)
	{
		printk(KERN_ERR "%s: Get PSU Vout mode failed\n", __func__);
		return 0;
	}
	else
	{
		/*printk(KERN_ERR "%s: vout_mode reg value 0x%x\n", __func__, status);*/
		return status;
	}
}

static u16 psu_get_v_out(struct i2c_client *client)
{
	u16 status = 0, retry = 10;
	uint8_t offset = 0x8b; // READ_VOUT

	while (retry) {
		status = i2c_smbus_read_word_data((struct i2c_client *)client, offset);
		if (unlikely(status < 0)) {
			msleep(60);
			retry--;
			continue;
		}
		break;
	}

	if (status < 0)
	{
		printk(KERN_ERR "%s: Get PSU Vout failed\n", __func__);
		return 0;
	}
	else
	{
		/*printk(KERN_ERR "%s: vout reg value 0x%x\n", __func__, status);*/
		return status;
	}
}

ssize_t pddf_show_custom_psu_v_out(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	int exponent, mantissa;
	int multiplier = 1000;

	u16 value = psu_get_v_out(client);
	u8 vout_mode = psu_get_vout_mode(client);

	if ((vout_mode >> 5) == 0)
		exponent = two_complement_to_int(vout_mode & 0x1f, 5, 0x1f);
	else
	{
		/*printk(KERN_ERR "%s: Only support linear mode for vout mode\n", __func__);*/
		exponent = 0;
	}
	mantissa = value;
	if (exponent >= 0)
		return sprintf(buf, "%d\n", (mantissa << exponent) * multiplier);
	else
		return sprintf(buf, "%d\n", (mantissa * multiplier) / (1 << -exponent));
}

ssize_t pddf_get_custom_psu_serial_num(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pddf_psu_data data;
    int i, status;
    char buffer[32]={0};

    for (i = 0; i < ARRAY_SIZE(models); i++) {

        status = pddf_psu_read_block(client, models[i].offset,
                                           buffer, models[i].length);
        if (status < 0) {
            data.model_name[0] = '\0';
            dev_dbg(&client->dev, "unable to read model name from (0x%x) offset(0x%x)\n",
                                  client->addr, models[i].offset);
            return status;
        }
        else {
            buffer[models[i].length >= (sizeof(buffer)-1)?(sizeof(buffer)-1):models[i].length] = '\0';
        }

        /* Determine if the model name is known, if not, read next index
         */
        data.serial_number[0] = '\0';
        if (strncmp(buffer, models[i].model_name, models[i].chk_length) == 0) {
            status = pddf_psu_read_block(client, serials[i].offset,
                                           data.serial_number, serials[i].length);

            if (status < 0) {
                data.serial_number[0] = '\0';
                dev_dbg(&client->dev, "unable to read serial num from (0x%x) offset(0x%x)\n",
                                  client->addr, serials[i].offset);
                return status;
            }
            else {
                data.serial_number[serials[i].length>=(sizeof(data.serial_number)-1)?(sizeof(data.serial_number)-1):serials[i].length] = '\0';
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

ssize_t pddf_get_custom_psu_model_name(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pddf_psu_data data;
    int i, status;

    data.model_name[0]='\0';
    for (i = 0; i < ARRAY_SIZE(models); i++) {
        status = pddf_psu_read_block(client, models[i].offset,
                                           data.model_name, models[i].length);
        if (status < 0) {
            data.model_name[0] = '\0';
            dev_dbg(&client->dev, "unable to read model name from (0x%x) offset(0x%x)\n",
                                  client->addr, models[i].offset);
            return status;
        }
        else {
            data.model_name[models[i].length >= (sizeof(data.model_name)-1)?(sizeof(data.model_name)-1):models[i].length] = '\0';
        }

        /* Determine if the model name is known, if not, read next index
         */
        if (strncmp(data.model_name, models[i].model_name, models[i].chk_length) == 0) {
            if (models[i].type==PSU_TYPE_3Y_YESM1300AM)
            {
                char buf[10] = {0};
                memcpy(buf, &data.model_name[9], 10);
                memcpy(&data.model_name[8], buf, 10);
                data.model_name[MAX_MODEL_NAME] = '\0';
            }
            else if ((models[i].type == PSU_TYPE_AC_ACBEL_FSH082_F2B) ||
                     (models[i].type == PSU_TYPE_AC_ACBEL_FSH095_B2F) ||
                     (models[i].type == PSU_TYPE_AC_ACBEL_FSF019_F2B)) {
                      // Adjust model name for FSH082 / FSH095 / FSF019
                char buf[4] = {0};
                memcpy(buf, &data.model_name[9], 4);
                memcpy(&data.model_name[7], buf, 4);
                data.model_name[6] = '-';
                data.model_name[11] = '\0';
            }
            else
                data.model_name[0]='\0';

            return sprintf(buf, "%s\n", data.model_name);
        }
        else {
            data.model_name[0] = '\0';
        }


    }

    return -ENODATA;

}

ssize_t pddf_get_custom_psu_fan_dir(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pddf_psu_data data;
    int i, status;
    char buffer[32]={0};

    data.fan_dir[0]='\0';
    for (i = 0; i < ARRAY_SIZE(models); i++) {
        buffer[0]='\0';
        status = pddf_psu_read_block(client, models[i].offset,
                                           buffer, models[i].length);
        if (status < 0) {
            buffer[0] = '\0';
            dev_dbg(&client->dev, "unable to read model name from (0x%x) offset(0x%x)\n",
                                  client->addr, models[i].offset);
            return status;
        }
        else {
            buffer[models[i].length >= (sizeof(buffer)-1)?(sizeof(buffer)-1):models[i].length]='\0';
        }

        /* Determine if the model name is known, if not, read next index
         */
        if (strncmp(buffer, models[i].model_name, models[i].chk_length) == 0) {
            if (models[i].type==PSU_TYPE_3Y_YESM1300AM)
            {
               /*YESM1300AM-2A01P10(F2B) or YESM1300AM-2R01P10(B2F)*/
               if(strstr(buffer, "2A01P10"))
               {
                   snprintf(data.fan_dir, ARRAY_SIZE(data.fan_dir), "%s", "F2B");
               }
               else if(strstr(buffer, "2R01P10"))
               {
                   snprintf(data.fan_dir, ARRAY_SIZE(data.fan_dir), "%s", "B2F");
               }

            }
            else if ((models[i].type == PSU_TYPE_AC_ACBEL_FSH082_F2B) ||
                     (models[i].type == PSU_TYPE_AC_ACBEL_FSF019_F2B))
            {
                snprintf(data.fan_dir, ARRAY_SIZE(data.fan_dir), "%s", "F2B");
            }
            else if ((models[i].type == PSU_TYPE_AC_ACBEL_FSH095_B2F))
            {
                snprintf(data.fan_dir, ARRAY_SIZE(data.fan_dir), "%s", "B2F");
            }

            data.fan_dir[MAX_FAN_DIR_LEN] = '\0';

            return sprintf(buf, "%s\n", data.fan_dir);
        }
        else {
            data.fan_dir[0] = '\0';
        }
    }

    return -ENODATA;


}

static int __init pddf_custom_psu_init(void)
{
    access_psu_v_out.show = pddf_show_custom_psu_v_out;
    access_psu_v_out.do_get = NULL;

    access_psu_serial_num.show = pddf_get_custom_psu_serial_num;
    access_psu_serial_num.do_get = NULL;

    access_psu_model_name.show = pddf_get_custom_psu_model_name;
    access_psu_model_name.do_get = NULL;

    access_psu_fan_dir.show = pddf_get_custom_psu_fan_dir;
    access_psu_fan_dir.do_get = NULL;

	return 0;
}

static void __exit pddf_custom_psu_exit(void)
{
	printk(KERN_ERR "pddf_custom_psu_exit\n");
	return;
}

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("pddf custom psu api");
MODULE_LICENSE("GPL");

module_init(pddf_custom_psu_init);
module_exit(pddf_custom_psu_exit);

