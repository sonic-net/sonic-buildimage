#ifndef __UART_PAYLOAD_DEFS_H__
#define __UART_PAYLOAD_DEFS_H__

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


#define MAX_NUM_PAYLOAD 20
#define MAX_PAYLOAD_ATTRS 32
#define ATTR_NAME_LEN 32
#define STR_ATTR_SIZE 32
#define DEV_TYPE_LEN 32
#define BUFFERSIZE	128

/* Each client has this additional data 
 */

typedef struct PAYLOAD_DATA_ATTR
{
    char aname[ATTR_NAME_LEN];      // attr name, taken from enum payload_sysfs_attributes
    uint32_t cmd;
    uint32_t subcmd1;
    uint32_t subcmd2;
    uint32_t byte;
    void *access_data;
}PAYLOAD_DATA_ATTR;

typedef struct PAYLOAD_SYSFS_ATTR_DATA
{
    int index;
    unsigned short mode;
    ssize_t (*show)(struct device *dev, struct device_attribute *da, char *buf);
    int (*pre_get)(struct i2c_client *client, PAYLOAD_DATA_ATTR *adata, void *data);
    int (*do_get)(struct i2c_client *client, PAYLOAD_DATA_ATTR *adata, void *data);
    int (*post_get)(struct i2c_client *client, PAYLOAD_DATA_ATTR *adata, void *data);
    ssize_t (*store)(struct device *dev, struct device_attribute *da, const char *buf, __kernel_size_t count);
    int (*pre_set)(struct i2c_client *client, PAYLOAD_DATA_ATTR *adata, void *data);
    int (*do_set)(struct i2c_client *client, PAYLOAD_DATA_ATTR *adata, void *data);
    int (*post_set)(struct i2c_client *client, PAYLOAD_DATA_ATTR *adata, void *data);
    void *data;
} PAYLOAD_SYSFS_ATTR_DATA;

typedef struct PAYLOAD_SYSFS_ATTR_DATA_ENTRY
{
    char name[ATTR_NAME_LEN];
    PAYLOAD_SYSFS_ATTR_DATA *a_ptr;
} PAYLOAD_SYSFS_ATTR_DATA_ENTRY;

/* PAYLOAD CLIENT DATA - PLATFORM DATA FOR PAYLOAD CLIENT */
typedef struct PAYLOAD_DATA
{   
    int idx;
    PAYLOAD_DATA_ATTR payload_attr;
    int len;             // no of valid attributes for this payload client
    PAYLOAD_DATA_ATTR payload_attrs[MAX_PAYLOAD_ATTRS]; 
}PAYLOAD_DATA;

typedef struct PAYLOAD_PDATA
{
    int idx;
    int len;             // no of valid attributes for this payload client
    PAYLOAD_DATA_ATTR *payload_attrs; 
}PAYLOAD_PDATA;


enum payload_sysfs_attributes {
    PAYLOAD_GET,

};

struct payload_attr_info {
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
struct payload_data {
	struct device			*hwmon_dev;
	u8						index;
	int						num_attr;
	struct attribute		*payload_attribute_list[MAX_PAYLOAD_ATTRS];
	struct attribute_group	payload_attribute_group;
	struct payload_attr_info	attr_info[MAX_PAYLOAD_ATTRS];
};

#endif
