#ifndef __UART_PSU_DEFS_H__
#define __UART_PSU_DEFS_H__

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


#define MAX_NUM_PSU 5
#define MAX_PSU_ATTRS 32
#define ATTR_NAME_LEN 32
#define STR_ATTR_SIZE 32
#define DEV_TYPE_LEN 32
#define BUFFERSIZE	128

/* Each client has this additional data 
 */

typedef struct PSU_DATA_ATTR
{
    char aname[ATTR_NAME_LEN];      // attr name, taken from enum psu_sysfs_attributes
    uint32_t cmd;
    uint32_t subcmd1;
    uint32_t subcmd2;
    uint32_t byte;
    void *access_data;
}PSU_DATA_ATTR;

typedef struct PSU_SYSFS_ATTR_DATA
{
    int index;
    unsigned short mode;
    ssize_t (*show)(struct device *dev, struct device_attribute *da, char *buf);
    int (*pre_get)(struct i2c_client *client, PSU_DATA_ATTR *adata, void *data);
    int (*do_get)(struct i2c_client *client, PSU_DATA_ATTR *adata, void *data);
    int (*post_get)(struct i2c_client *client, PSU_DATA_ATTR *adata, void *data);
    ssize_t (*store)(struct device *dev, struct device_attribute *da, const char *buf, __kernel_size_t count);
    int (*pre_set)(struct i2c_client *client, PSU_DATA_ATTR *adata, void *data);
    int (*do_set)(struct i2c_client *client, PSU_DATA_ATTR *adata, void *data);
    int (*post_set)(struct i2c_client *client, PSU_DATA_ATTR *adata, void *data);
    void *data;
} PSU_SYSFS_ATTR_DATA;

typedef struct PSU_SYSFS_ATTR_DATA_ENTRY
{
    char name[ATTR_NAME_LEN];
    PSU_SYSFS_ATTR_DATA *a_ptr;
} PSU_SYSFS_ATTR_DATA_ENTRY;


/* PSU CLIENT DATA - PLATFORM DATA FOR PSU CLIENT */
typedef struct PSU_DATA
{
    int idx;    // psu index
    int num_psu_fans;
    PSU_DATA_ATTR psu_attr;
    int len;             // no of valid attributes for this psu client
    PSU_DATA_ATTR psu_attrs[MAX_PSU_ATTRS]; 
}PSU_DATA;

typedef struct PSU_PDATA
{
    int idx;                    // psu index
    int num_psu_fans;      // num of fans supported by the PSU
    int len;             // no of valid attributes for this psu client
    PSU_DATA_ATTR *psu_attrs; 
}PSU_PDATA;


enum psu_sysfs_attributes {
    PSU_PRESENCE,
    PSU_STATUS,
    PSU_TYPE,
    PSU_VIN,
    PSU_VOUT,
    PSU_IIN,
    PSU_IOUT,
    PSU_PIN,
    PSU_POUT,
    PSU_DIRECTION,
    PSU_WARNING,
	PSU_DIRECTION_WARNING,
    PSU_MODEL,
    PSU_SERIAL_NUMBER,
    PSU_TEMP,
    PSU_FAN_SPEED,
};

struct psu_attr_info {
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
struct psu_data {
	struct device			*hwmon_dev;
	u8						index;
	int						num_psu_fans;
	int						num_attr;
	struct attribute		*psu_attribute_list[MAX_PSU_ATTRS];
	struct attribute_group	psu_attribute_group;
	struct psu_attr_info	attr_info[MAX_PSU_ATTRS];
};

#endif
