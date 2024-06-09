#ifndef __PLATFORM_COMMON_H__
#define __PLATFORM_COMMON_H__

#include<linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))

typedef enum {
    DBG_START,
    DBG_VERBOSE,
    DBG_KEY,
    DBG_WARN,
    DBG_ERROR,
    DBG_END,
} dbg_level_t;

 typedef struct dfd_i2c_dev_s {
     int bus;
     int addr;
 } dfd_i2c_dev_t;

typedef struct  dfd_dev_head_info_s {
    uint8_t   ver;
    uint8_t   flag;
    uint8_t   hw_ver;
    uint8_t   type;
    int16_t   tlv_len;
} dfd_dev_head_info_t;

typedef struct dfd_dev_tlv_info_s {
    uint8_t  type;
    uint8_t  len;
    uint8_t  data[0];
} dfd_dev_tlv_info_t;

typedef enum dfd_dev_info_type_e {
    DFD_DEV_INFO_TYPE_MAC        = 1,
    DFD_DEV_INFO_TYPE_NAME       = 2,
    DFD_DEV_INFO_TYPE_SN         = 3,
    DFD_DEV_INFO_TYPE_PWR_CONS   = 4,
    DFD_DEV_INFO_TYPE_HW_INFO    = 5,
    DFD_DEV_INFO_TYPE_DEV_TYPE   = 6,
} dfd_dev_tlv_type_t;

extern int debuglevel;
extern s32 platform_i2c_smbus_read_byte_data(const struct i2c_client *client, u8 command);
extern s32 platform_i2c_smbus_read_i2c_block_data(const struct i2c_client *client,
                u8 command, u8 length, u8 *values);
extern s32 platform_i2c_smbus_read_word_data(const struct i2c_client *client, u8 command);

#define DBG_DEBUG(fmt, arg...)  do { \
    if ( debuglevel > DBG_START && debuglevel < DBG_ERROR) { \
          printk(KERN_INFO "[DEBUG]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
    } else if ( debuglevel >= DBG_ERROR ) {   \
        printk(KERN_ERR "[DEBUG]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
    } else {    } \
} while (0)

#define DBG_INFO(fmt, arg...)  do { \
     if ( debuglevel > DBG_KEY) {  \
        printk(KERN_INFO "[INFO]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
       } \
 } while (0)

#define DBG_ERROR(fmt, arg...)  do { \
     if ( debuglevel > DBG_START) {  \
        printk(KERN_ERR "[ERROR]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
       } \
 } while (0)

#endif
