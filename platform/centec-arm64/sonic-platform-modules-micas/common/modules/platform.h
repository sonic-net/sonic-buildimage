#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#include<linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))

/* debug switch level */
typedef enum {
    DBG_START,
    DBG_VERBOSE,
    DBG_KEY,
    DBG_WARN,
    DBG_ERROR,
    DBG_END,
} dbg_level_t;

typedef enum dfd_cpld_id {
    BCM_CPLD0 = 0,
    BCM_CPLD1,
    CPLD0_MAC0,
    CPLD0_MAC1,
    CPLD1_MAC0,
    CPLD2_MAC1,
} dfd_cpld_id_t;

 typedef enum dfd_cpld_bus {
    SMBUS_BUS = 0 ,
    GPIO_BUS = 1,
    PCA9641_BUS = 2,
} dfd_cpld_bus_t;

 typedef struct dfd_i2c_dev_s {
     int bus;        /* bus number */
     int addr;       /* bus address */
 } dfd_i2c_dev_t;

 typedef enum dfd_cpld_addr {
    CPLD_ADDR_MIN   = 0x31,
    BCM_CPLD0_ADDR  = 0x32,   /* pca9641 up */
    CPLD0_MAC0_ADDR = 0x33,   /* SMBUS down */
    CPLD0_MAC1_ADDR = 0x34,
    CPLD1_MAC0_ADDR = 0x35,
    CPLD2_MAC1_ADDR = 0x36,
    BCM_CPLD1_ADDR  = 0x37,
    CPLD_ADDR_MAX,
} dfd_cpld_addr_t;

typedef struct dfd_dev_head_info_s {
    uint8_t   ver;                       /* The version number defined by the E2PROM file, the initial version is 0x01  */
    uint8_t   flag;                      /* The new version of E2PROM is identified as 0x7E */
    uint8_t   hw_ver;                    /* It consists of two parts: the main version number and the revised version */
    uint8_t   type;                      /* Hardware type definition information */
    int16_t   tlv_len;                   /* Effective data length (16 bits) */
} dfd_dev_head_info_t;

typedef enum dfd_intf_e{
    DFD_INTF_GET_FAN_HW_VERSION,
    DFD_INTF_GET_FAN_STATUS,
    DFD_INTF_GET_FAN_SPEED_LEVEL,
    DFD_INTF_GET_FAN_SPEED,
    DFD_INTF_GET_FAN_ATTRIBUTE,
    DFD_INTF_GET_FAN_SN,
    DFD_INTF_GET_FAN_TYPE,
    DFD_INTF_SET_FAN_SPEED_LEVEL,
    DFD_INTF_GET_FAN_SUB_NUM,
    DFD_INTF_GET_FAN_FAIL_BITMAP,
}dfd_intf_t;

typedef struct dfd_dev_tlv_info_s {
    uint8_t  type;                       /* the type of data */
    uint8_t  len;                        /* the length of data */
    uint8_t  data[0];                    /* data */
} dfd_dev_tlv_info_t;

typedef enum dfd_dev_info_type_e {
    DFD_DEV_INFO_TYPE_MAC        = 1,
    DFD_DEV_INFO_TYPE_NAME       = 2,
    DFD_DEV_INFO_TYPE_SN         = 3,
    DFD_DEV_INFO_TYPE_PWR_CONS   = 4,
    DFD_DEV_INFO_TYPE_HW_INFO    = 5,
    DFD_DEV_INFO_TYPE_DEV_TYPE   = 6,
} dfd_dev_tlv_type_t;

struct sfp_drivers_t{
    /* addr = sff present bitmap addr, index from 0 */
    void (*get_number) (unsigned int *number);
    int (*get_port_start) (void);
    int (*get_port_end) (void);
    bool (*is_qsfp_port) (const unsigned int port_num);
    bool (*get_present) (unsigned long *bitmap);
    bool (*read_sfp_eeprom) (const unsigned int port,
                const unsigned char addr,
                const unsigned char offset,
                const uint32_t count, char *buf);
    bool (*write_sfp_eeprom) (const unsigned int port,
                const unsigned char addr,
                const unsigned char offset,
                const unsigned char count,
                const char *buf);
    bool (*read_sysfs) (const unsigned int bus,
                const unsigned char addr,
                const unsigned char offset,
                const uint32_t count, char *buf);
    bool (*write_sysfs) (const unsigned int bus,
                const unsigned char addr,
                const unsigned char offset,
                const unsigned char count,
                const char *buf);
    bool (*read_block_sysfs) (const unsigned int bus,
                const unsigned char addr,
                const unsigned char offset,
                const uint32_t count, char *buf);
};

extern int debuglevel;
extern int dfd_cpld_read_chipid(int cpldid , uint32_t addr, int32_t size, unsigned char *buf);
extern int dfd_cpld_read(int32_t addr, uint8_t *val);
extern int dfd_cpld_write(int32_t addr, uint8_t val);

extern s32 platform_i2c_smbus_read_byte_data(const struct i2c_client *client, u8 command);
extern s32 platform_i2c_smbus_read_i2c_block_data(const struct i2c_client *client,
                u8 command, u8 length, u8 *values);
extern s32 platform_i2c_smbus_read_word_data(const struct i2c_client *client, u8 command);

extern int  sfp_drivers_register(struct sfp_drivers_t *drv);
extern int  sfp_drivers_unregister(void);

extern int sysfs_drivers_register(struct sfp_drivers_t *drv);
extern int sysfs_drivers_unregister(void);

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
