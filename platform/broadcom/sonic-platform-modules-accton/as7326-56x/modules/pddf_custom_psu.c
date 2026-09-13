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
ssize_t pddf_get_custom_psu_fan_dir(struct device *dev, struct device_attribute *da, char *buf);
extern PSU_SYSFS_ATTR_DATA access_psu_model_name;
extern PSU_SYSFS_ATTR_DATA access_psu_serial_num;
extern PSU_SYSFS_ATTR_DATA access_psu_fan_dir;

#define MAX_MODEL_NAME          13
#define MAX_SERIAL_NUMBER       18
#define FAN_DIR_LEN 3

const char FAN_DIR_F2B[] = "F2B";
const char FAN_DIR_B2F[] = "B2F";
const char FAN_DIR_UNKNOWN[] = "";

enum psu_type {
    PSU_TYPE_ACBEL_FSF019_AC_F2B, // FSF019-611G
    PSU_TYPE_ACBEL_FSF045_AC_B2F, // FSH045-611G
    PSU_TYPE_3Y_YM2651YBR_AC_F2B, // YM-2651Y-BR
    PSU_TYPE_3Y_YM2651YB01R_AC_F2B, // YM-2651Y-B01R
    PSU_TYPE_3Y_YM2651YCR_AC_B2F, // YM-2651Y-CR
    PSU_TYPE_3Y_YM2651YC01R_AC_B2F, // YM-2651Y-C01R
    PSU_TYPE_3Y_YM2651VBR_DC_F2B, // YM-2651V-BR
    PSU_TYPE_3Y_YM2651VCR_DC_B2F, // YM-2651V-CR
    PSU_TYPE_3Y_YM2651_SERIES
};

struct model_name_info {
    char* model_name;
    enum psu_type type;
    u8 offset;
    u8 length;
    u8 chk_length;
};
struct serial_number_info {
    u8 offset;
    u8 length;
};

struct fan_dir_info {
    char* model_name;
    u8 model_length;
    enum psu_type type;
    const char* fan_dir;
};
const struct model_name_info models[] = {
    { "FSF019", PSU_TYPE_ACBEL_FSF019_AC_F2B, 0x20, 13, 6 },
    { "FSF045", PSU_TYPE_ACBEL_FSF045_AC_B2F, 0x20, 13, 6 },
    { "YM-2651", PSU_TYPE_3Y_YM2651_SERIES, 0x20, 13, 7 }
};

const struct fan_dir_info fan_dirs[] = {
    { "FSF019", 6, PSU_TYPE_ACBEL_FSF019_AC_F2B, FAN_DIR_F2B },
    { "FSF045", 6, PSU_TYPE_ACBEL_FSF045_AC_B2F, FAN_DIR_B2F },
    { "YM-2651Y-BR", 11, PSU_TYPE_3Y_YM2651YBR_AC_F2B, FAN_DIR_F2B },
    { "YM-2651Y-B01R", 13, PSU_TYPE_3Y_YM2651YB01R_AC_F2B, FAN_DIR_F2B },
    { "YM-2651Y-CR", 11, PSU_TYPE_3Y_YM2651YCR_AC_B2F, FAN_DIR_B2F },
    { "YM-2651Y-C01R", 13, PSU_TYPE_3Y_YM2651YC01R_AC_B2F, FAN_DIR_B2F },
    { "YM-2651V-BR", 11, PSU_TYPE_3Y_YM2651VBR_DC_F2B, FAN_DIR_F2B },
    { "YM-2651V-CR", 11, PSU_TYPE_3Y_YM2651VCR_DC_B2F, FAN_DIR_B2F }
};

const struct serial_number_info serials[] = {
    [PSU_TYPE_ACBEL_FSF019_AC_F2B] = { 0x2e, 16 },
    [PSU_TYPE_ACBEL_FSF045_AC_B2F] = { 0x2e, 16 },
    [PSU_TYPE_3Y_YM2651YBR_AC_F2B] = { 0x2e, 18 },
    [PSU_TYPE_3Y_YM2651YB01R_AC_F2B] = { 0x2e, 18 },
    [PSU_TYPE_3Y_YM2651YCR_AC_B2F] = { 0x2e, 18 },
    [PSU_TYPE_3Y_YM2651YC01R_AC_B2F] = { 0x35, 18 },
    [PSU_TYPE_3Y_YM2651VBR_DC_F2B] = { 0x2e, 18 },
    [PSU_TYPE_3Y_YM2651VCR_DC_B2F] = { 0x2e, 18 }
};

struct pddf_psu_data {    
    char model_name[MAX_MODEL_NAME+1];
    char serial_number[MAX_SERIAL_NUMBER+1];
    char fan_dir[FAN_DIR_LEN+1];
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

/* Get type and model_name
 */
static int  pddf_get_model_name_and_type(struct i2c_client *client, enum psu_type *get_type, struct pddf_psu_data *get_data)
{
    char model_name[MAX_MODEL_NAME+1];
    int i, status;
    enum psu_type type;
    
    get_data->model_name[0]='\0';
    *get_type = PSU_TYPE_ACBEL_FSF019_AC_F2B;
    for (i = 0; i < ARRAY_SIZE(models); i++) {
        model_name[0]='\0';
        if ((models[i].length+1) > ARRAY_SIZE(model_name)) {
            dev_dbg(&client->dev,
                        "invalid models[%d].length(%d), should not exceed the size of data->model_name(%ld)\n",
                        i, models[i].length, ARRAY_SIZE(model_name));
            continue;
        }
        
        status = pddf_psu_read_block(client, models[i].offset,
                                           model_name, models[i].length);
        if (status < 0) {
            model_name[0] = '\0';
            dev_dbg(&client->dev, "unable to read model name from (0x%x) offset(0x%x)\n",
                                  client->addr, models[i].offset);
            return status;
        }
        else {
            model_name[models[i].length>=(sizeof(model_name)-1)?(sizeof(model_name)-1):models[i].length] = '\0';
        }

        /* Determine if the model name is known, if not, read next index
         */
        if (strncmp(model_name, models[i].model_name, models[i].chk_length) == 0) {
            type = models[i].type;
            break;
        }
        else {
            model_name[0] = '\0';
        }
    }

    /* Remove useless characters for PSU_TYPE_3Y_YM2651_SERIES */
    if (type == PSU_TYPE_3Y_YM2651_SERIES) {
        model_name[8] = '-';
        if (!isgraph(model_name[11]))
            model_name[11] = '\0';
        else
            model_name[MAX_MODEL_NAME] = '\0';
    }
    /* Remove useless characters for PSU_TYPE_ACBEL_FSF* */
    else if ((type == PSU_TYPE_ACBEL_FSF019_AC_F2B) || (type == PSU_TYPE_ACBEL_FSF045_AC_B2F)) {
        memmove(&model_name[7], &model_name[9], ARRAY_SIZE(model_name)-9);
        model_name[6] = '-';
        model_name[11] = '\0';
    }
    else
        return -ENODEV;
        
    /* Determine fan direction and correct the PSU type */
    for (i = 0; i < ARRAY_SIZE(fan_dirs); i++) {
        if ((fan_dirs[i].model_length+1) > ARRAY_SIZE(model_name)) {
            dev_dbg(&client->dev,
                      "invalid fan_dirs[%d].model_length(%d), should not exceed the size of data->model_name(%ld)\n",
                       i, fan_dirs[i].model_length, ARRAY_SIZE(model_name));
            continue;
        }

        if (strncmp(model_name, fan_dirs[i].model_name, fan_dirs[i].model_length) == 0) {
            type = fan_dirs[i].type;            
            break;
        }
    }    
    if (type >= PSU_TYPE_3Y_YM2651_SERIES)
    {
        return -ENODEV;
    }
        
    *get_type = type;
    memcpy(get_data->model_name, model_name, strlen(model_name)>=sizeof(get_data->model_name)?sizeof(get_data->model_name):strlen(model_name));
    get_data->model_name[strlen(model_name)>=(sizeof(get_data->model_name)-1)?(sizeof(get_data->model_name)-1):strlen(model_name)]='\0';
    return 0;
}

ssize_t pddf_get_custom_psu_serial_num(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pddf_psu_data data;
    int status;
    enum psu_type type;

    data.serial_number[0]='\0';

    status = pddf_get_model_name_and_type(client, &type, &data);
    if (status < 0)
    {
        return -ENODEV;
    }

    status = pddf_psu_read_block(client, serials[type].offset,
                                            data.serial_number, serials[type].length);
    if (status < 0) {
        data.serial_number[0] = '\0';
        dev_dbg(&client->dev, "unable to read serial num from (0x%x) offset(0x%x)\n",
                                client->addr, serials[type].offset);
        return status;
    }
    else {
        data.serial_number[serials[type].length >= (sizeof(data.serial_number)-1)?(sizeof(data.serial_number)-1):serials[type].length] = '\0';
    }

    return sprintf(buf, "%s\n", data.serial_number);
	
}

ssize_t pddf_get_custom_psu_model_name(struct device *dev, struct device_attribute *da, char *buf)
{
    struct i2c_client *client = to_i2c_client(dev);
    struct pddf_psu_data data;
    int status;
    enum psu_type type;

    status = pddf_get_model_name_and_type(client, &type, &data);
    if (status < 0)
    {
        return -ENODEV;
    }
    
    return sprintf(buf, "%s\n", data.model_name);

}

ssize_t pddf_get_custom_psu_fan_dir(struct device *dev, struct device_attribute *da, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct pddf_psu_data data;
    int status;
    enum psu_type type;

    status = pddf_get_model_name_and_type(client, &type, &data);
    if (status < 0)
    {
        return -ENODEV;
    }
    
    if (type < PSU_TYPE_3Y_YM2651_SERIES)
    {
        memcpy(data.fan_dir, fan_dirs[type].fan_dir, sizeof(data.fan_dir));
        return sprintf(buf, "%s\n", data.fan_dir);
    }
    else
        return -ENODEV;

}

static int __init pddf_custom_psu_init(void)
{
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
	return;
}

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("pddf custom psu api");
MODULE_LICENSE("GPL");

module_init(pddf_custom_psu_init);
module_exit(pddf_custom_psu_exit);

