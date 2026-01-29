#ifndef __UART_FAN_DEFS_H__
#define __UART_FAN_DEFS_H__

#include <linux/dmaengine.h>
#include <linux/dma/hsu.h>
#include <linux/8250_pci.h>
#include <linux/serial_8250.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>


#define MAX_NUM_FAN 6
#define MAX_FAN_ATTRS 32
#define ATTR_NAME_LEN 32
#define STR_ATTR_SIZE 32
#define DEV_TYPE_LEN 32
#define BUFFERSIZE	128

/* Each client has this additional data 
 */

typedef struct FAN_DATA_ATTR
{
    char aname[ATTR_NAME_LEN];      // attr name, taken from enum fan_sysfs_attributes
    uint32_t cmd;
    uint32_t subcmd1;
    uint32_t subcmd2;
    uint32_t byte;
    void *access_data;
}FAN_DATA_ATTR;

typedef struct FAN_SYSFS_ATTR_DATA
{
    int index;
    unsigned short mode;
    ssize_t (*show)(struct device *dev, struct device_attribute *da, char *buf);
    int (*pre_get)(struct i2c_client *client, FAN_DATA_ATTR *adata, void *data);
    int (*do_get)(struct i2c_client *client, FAN_DATA_ATTR *adata, void *data);
    int (*post_get)(struct i2c_client *client, FAN_DATA_ATTR *adata, void *data);
    ssize_t (*store)(struct device *dev, struct device_attribute *da, const char *buf, __kernel_size_t count);
    int (*pre_set)(struct i2c_client *client, FAN_DATA_ATTR *adata, void *data);
    int (*do_set)(struct i2c_client *client, FAN_DATA_ATTR *adata, void *data);
    int (*post_set)(struct i2c_client *client, FAN_DATA_ATTR *adata, void *data);
    void *data;
} FAN_SYSFS_ATTR_DATA;

typedef struct FAN_SYSFS_ATTR_DATA_ENTRY
{
    char name[ATTR_NAME_LEN];
    FAN_SYSFS_ATTR_DATA *a_ptr;
} FAN_SYSFS_ATTR_DATA_ENTRY;

/* FAN CLIENT DATA - PLATFORM DATA FOR FAN CLIENT */
typedef struct FAN_DATA
{
    int idx;
    int num_fantrays;    
    FAN_DATA_ATTR fan_attr;
    int len;             // no of valid attributes for this fan client
    FAN_DATA_ATTR fan_attrs[MAX_FAN_ATTRS]; 
}FAN_DATA;

typedef struct FAN_PDATA
{
    int idx;
    int num_fantrays;      // num of fans supported by the FAN
    int len;             // no of valid attributes for this fan client
    FAN_DATA_ATTR *fan_attrs; 
}FAN_PDATA;


enum fan_sysfs_attributes {
    FAN_BOARD_TYPE,
    FAN_STATUS,
    FAN_PRESENCE,
    FAN_FRONT_SPEED_RPM,
    FAN_REAR_SPEED_RPM,
    FAN_DIRECTION,
    FAN_EEPROM_MODEL,
    FAN_EEPROM_SERIAL_NUMBER,
    FAN_EEPROM_MANUFACTURER,
    FAN_EEPROM_MODULE_MODEL,
    FAN_EEPROM_MODULE_BRAND,
    FAN_EEPROM_DIRECTION,
};

struct fan_attr_info {
	char				name[ATTR_NAME_LEN];
	struct mutex        update_lock;
    char                valid;           /* !=0 if registers are valid */
    unsigned long       last_updated;    /* In jiffies */
	union {
		char strval[STR_ATTR_SIZE];
		int	 intval;
		u16	 shortval;
		u8   charval;
	}val;
};
struct fan_data {
	struct device			*hwmon_dev;
	u8						index;
	int						num_fantrays;
	int						num_attr;
	struct attribute		*fan_attribute_list[MAX_FAN_ATTRS];
	struct attribute_group	fan_attribute_group;
	struct fan_attr_info	attr_info[MAX_FAN_ATTRS];
};

#endif
