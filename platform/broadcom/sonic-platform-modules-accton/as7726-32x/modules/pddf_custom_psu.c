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

ssize_t pddf_get_custom_psu_model_name(struct device *dev, struct device_attribute *da, char *buf);
ssize_t pddf_get_custom_psu_serial_num(struct device *dev, struct device_attribute *da, char *buf);
extern PSU_SYSFS_ATTR_DATA access_psu_model_name;
extern PSU_SYSFS_ATTR_DATA access_psu_serial_num;

#define MAX_MODEL_NAME          16
#define MAX_SERIAL_NUMBER       19

enum psu_type {
    PSU_TYPE_AC_110V,
    PSU_TYPE_DC_48V,
    PSU_TYPE_DC_12V,
    PSU_TYPE_AC_ACBEL_FSF019,
    PSU_TYPE_AC_ACBEL_FSF045
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
{PSU_TYPE_AC_110V, 0x20, 8, 8,  "YM-2651Y"},
{PSU_TYPE_DC_48V,  0x20, 8, 8,  "YM-2651V"},
{PSU_TYPE_DC_12V,  0x00, 11, 11, "PSU-12V-750"},
{PSU_TYPE_AC_ACBEL_FSF019, 0x15, 10, 7, "FSF019-"},
{PSU_TYPE_AC_ACBEL_FSF045, 0x15, 10, 7, "FSF045-"}

};

struct serial_number_info serials[] = {
{PSU_TYPE_AC_110V, 0x2e, 18, 18,  "YM-2651Y"},
{PSU_TYPE_DC_48V,  0x2e, 18, 18,  "YM-2651V"},
{PSU_TYPE_DC_12V,  0x2e, 18, 18,  "PSU-12V-750"},
{PSU_TYPE_AC_ACBEL_FSF019, 0x2e, 16, 16, "FSF019-"},
{PSU_TYPE_AC_ACBEL_FSF019, 0x2e, 16, 16, "FSF045-"}

};

struct pddf_psu_data {    
    char model_name[MAX_MODEL_NAME+1];
    char serial_number[MAX_SERIAL_NUMBER+1];
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

ssize_t pddf_get_custom_psu_model_name(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pddf_psu_data data;
    int i, status;
    
    for (i = 0; i < ARRAY_SIZE(models); i++) {
        memset(data.model_name, 0, sizeof(data.model_name));

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
            return sprintf(buf, "%s\n", data.model_name);
        }
        else {
            data.model_name[0] = '\0';
        }
    }

    return -ENODATA;

}

static int __init pddf_custom_psu_init(void)
{
    access_psu_serial_num.show = pddf_get_custom_psu_serial_num;
    access_psu_serial_num.do_get = NULL;
	
    access_psu_model_name.show = pddf_get_custom_psu_model_name;
    access_psu_model_name.do_get = NULL;
	
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

