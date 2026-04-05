#ifndef __UART_EEPROM_DEFS_H__
#define __UART_EEPROM_DEFS_H__

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


#define MAX_NUM_EEPROM 2
#define MAX_EEPROM_ATTRS 32
#define ATTR_NAME_LEN 32
#define STR_ATTR_SIZE 32
#define DEV_TYPE_LEN 32
#define BUFFERSIZE	128

/* Each client has this additional data 
 */

typedef struct EEPROM_DATA_ATTR
{
    char aname[ATTR_NAME_LEN];      // attr name, taken from enum eeprom_sysfs_attributes
    uint32_t cmd;
    uint32_t subcmd1;
    uint32_t subcmd2;
    uint32_t byte;
    void *access_data;
}EEPROM_DATA_ATTR;

typedef struct EEPROM_SYSFS_ATTR_DATA
{
    int index;
    unsigned short mode;
    ssize_t (*show)(struct device *dev, struct device_attribute *da, char *buf);
    int (*pre_get)(struct i2c_client *client, EEPROM_DATA_ATTR *adata, void *data);
    int (*do_get)(struct i2c_client *client, EEPROM_DATA_ATTR *adata, void *data);
    int (*post_get)(struct i2c_client *client, EEPROM_DATA_ATTR *adata, void *data);
    ssize_t (*store)(struct device *dev, struct device_attribute *da, const char *buf, __kernel_size_t count);
    int (*pre_set)(struct i2c_client *client, EEPROM_DATA_ATTR *adata, void *data);
    int (*do_set)(struct i2c_client *client, EEPROM_DATA_ATTR *adata, void *data);
    int (*post_set)(struct i2c_client *client, EEPROM_DATA_ATTR *adata, void *data);
    void *data;
} EEPROM_SYSFS_ATTR_DATA;

typedef struct EEPROM_SYSFS_ATTR_DATA_ENTRY
{
    char name[ATTR_NAME_LEN];
    EEPROM_SYSFS_ATTR_DATA *a_ptr;
} EEPROM_SYSFS_ATTR_DATA_ENTRY;

/* EEPROM CLIENT DATA - PLATFORM DATA FOR EEPROM CLIENT */
typedef struct EEPROM_DATA
{   int idx;
    EEPROM_DATA_ATTR eeprom_attr;
    int len;             // no of valid attributes for this eeprom client
    EEPROM_DATA_ATTR eeprom_attrs[MAX_EEPROM_ATTRS]; 
}EEPROM_DATA;

typedef struct EEPROM_PDATA
{
    int idx;
    int len;             // no of valid attributes for this eeprom client
    EEPROM_DATA_ATTR *eeprom_attrs; 
}EEPROM_PDATA;


enum eeprom_sysfs_attributes {
    EEPROM_PRODUCT_NAME,
    EEPROM_PART_NUMBER,
    EEPROM_SERIAL_NUMBER,
    EEPROM_BASE_MAC_ADDRESS,
    EEPROM_MANUFACTURE_DATA,
    EEPROM_DEVICE_VERSION,
    EEPROM_LABLE_REVISION,
    EEPROM_PLATFORM_NAME,
    EEPROM_ONIE_VERSION,
    EEPROM_MAC_ADDRESS,
    EEPROM_MANUFACTURER,
    EEPROM_COUNTRY_CODE,
    EEPROM_VENDOR_NAME,
    EEPROM_DIAG_VERSION,
    EEPROM_SERVICE_TAG,
    EEPROM_SWITCH_VENDOR,
    EEPROM_MAIN_BOARD_VERSION,
    EEPROM_COME_VERSION,
    EEPROM_GHC0_BOARD_VERSION,
    EEPROM_GHC1_BOARD_VERSION,
    EEPROM_EEPROM_CRC32,
    EEPROM_DEFAULT,
};

struct eeprom_attr_info {
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
struct eeprom_data {
	struct device			*hwmon_dev;
	u8						index;
	int						num_attr;
	struct attribute		*eeprom_attribute_list[MAX_EEPROM_ATTRS];
	struct attribute_group	eeprom_attribute_group;
	struct eeprom_attr_info	attr_info[MAX_EEPROM_ATTRS];
};

#endif
