#ifndef __CPLD_I2C_H__
#define __CPLD_I2C_H__

#include "xo2_dev.h"

#define CPLD_I2C_BUFFER_LEN             (64)
#define CPLD_I2C_HELP_INFO_LEN          (256)
#define CPLD_I2C_FILE_DIR_LEN           (128)
#define CPLD_I2C_PRODUCT_LEN            (128)
#define CPLD_I2C_MAX_SLOT_NUM           (8)
#define CPLD_I2C_DEVICE_FULL_NAME       "/etc/device/fullproductname"
#define CPLD_I2C_BUFF_SIZE	            (256)

typedef int (*cpld_i2c_func)(int argc, char* argv[]);

typedef enum {
    CPLD_I2C_OK = 0,
    CPLD_I2C_CMD_ERR = 1,
    CPLD_I2C_NULL_CPLD = 2,
} cpld_i2c_err_code;

typedef enum {
    CPLD_I2C_UPGRADE = 0,
    CPLD_I2C_READSTATUS,
    CPLD_I2C_UPDATE_EBR,
} cpld_i2c_func_type;

struct cpld_i2c_function {
    int type;
    char op[CPLD_I2C_BUFFER_LEN];
    cpld_i2c_func func;
    char help_info[CPLD_I2C_HELP_INFO_LEN];
};

#endif /* End of __CPLD_I2C_H__ */