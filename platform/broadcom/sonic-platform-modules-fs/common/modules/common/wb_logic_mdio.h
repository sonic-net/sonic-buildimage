#ifndef __WB_LOGIC_MDIO_H__
#define __WB_LOGIC_MDIO_H__
#include <wb_logic_dev_common.h>

#define DEV_NAME_MAX_LEN (64)

typedef struct logic_mdio_device_s {
    char dev_name[DEV_NAME_MAX_LEN];
    u32 big_endian;
    u32 reg_access_mode;
    u32 reg_width;
    u32 reg_offset;
    int device_flag;
} logic_mdio_device_t;

#endif
