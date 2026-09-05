#ifndef __RAGILE_H__
#define __RAGILE_H__

#include<linux/kernel.h>
#include <linux/module.h>

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
    PCA9641_BUS = 1,
    GPIO_BUS = 2,
} dfd_cpld_bus_t;

typedef struct dfd_i2c_dev_s {
    int bus;
    int addr;
} dfd_i2c_dev_t;

 typedef enum dfd_cpld_addr {
    CPLD_ADDR_MIN   = 0x31,
    BCM_CPLD0_ADDR  = 0x32,
    CPLD0_MAC0_ADDR = 0x33,
    CPLD0_MAC1_ADDR = 0x34,
    CPLD1_MAC0_ADDR = 0x35,
    CPLD2_MAC1_ADDR = 0x36,
    BCM_CPLD1_ADDR  = 0x37,
    CPLD_ADDR_MAX,
} dfd_cpld_addr_t;

typedef struct  dfd_dev_head_info_s {
    uint8_t   ver;
    uint8_t   flag;
    uint8_t   hw_ver;
    uint8_t   type;
    int16_t   tlv_len;
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

typedef struct i2c_muxs_struct_flag
{
    int nr; 
    char name[48]; 
    struct mutex    update_lock; 
    int flag; 
}i2c_mux_flag;

extern int setpca9641_muxflag(i2c_mux_flag i2c);
extern i2c_mux_flag getpca9641_muxflag(void) ;

extern int debuglevel;
extern int dfd_cpld_read_chipid(int cpldid , uint32_t addr, int32_t size, unsigned char *buf);
extern int dfd_cpld_read(int32_t addr, uint8_t *val);
extern int dfd_cpld_write(int32_t addr, uint8_t val);
extern int ragile_setdebug(int val);

#define DBG_DEBUG(fmt, arg...)  do { \
    if ( debuglevel > DBG_START && debuglevel < DBG_ERROR) { \
          printk(KERN_INFO "[DEBUG]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
    } else if ( debuglevel >= DBG_ERROR ) {   \
        printk(KERN_ERR "[DEBUG]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
    } else {    } \
} while (0)

#define DBG_ERROR(fmt, arg...)  do { \
     if ( debuglevel > DBG_START) {  \
        printk(KERN_ERR "[ERROR]:<%s, %d>:"fmt, __FUNCTION__, __LINE__, ##arg); \
       } \
 } while (0)

#define COMMON_STR_LEN (256)

#endif
