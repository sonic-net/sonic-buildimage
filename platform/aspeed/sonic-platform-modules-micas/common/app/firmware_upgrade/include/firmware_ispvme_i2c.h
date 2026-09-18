#ifndef __FIRMWARE_ISPVME_I2C_H__
#define __FIRMWARE_ISPVME_I2C_H__

typedef struct fw_ispvme_i2c_info_s {
    unsigned int bus_num;                         /* master i2c bus num */
    unsigned int slave_addr;                      /* i2c slave dev addr */
    unsigned int cpld_type;                       /* cpld type */
    unsigned int chain;                           /* ugrade chain num */
} fw_ispvme_i2c_info_t;

#define ISPVME_I2C_MISCDEV_NAME_PREFIX     "firmware_ispvme_i2c"
#define FIRMWARE_ISPVME_I2C_TYPE    'I'
#define FIRMWARE_ISPVME_I2C_INFO    _IOR(FIRMWARE_ISPVME_I2C_TYPE, 1, fw_ispvme_i2c_info_t)

#endif /* __FIRMWARE_ISPVME_I2C_H__ */
