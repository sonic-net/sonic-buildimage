#ifndef __FIRMWARE_UPGRADE_H__
#define __FIRMWARE_UPGRADE_H__

#include <linux/string.h>

#define TYPE_LEN         (10)
#define DEV_NAME_LEN     (64)
#define ENABLE_NUM       (16)

#define mem_clear(data, size) memset((data), 0, (size))

typedef struct firmware_jtag_device_s {
    uint32_t tdi;
    uint32_t tck;
    uint32_t tms;
    uint32_t tdo;
    uint32_t tck_delay;
} firmware_jtag_device_t;

typedef struct firmware_sysfs_device_s {
    uint32_t test_base;
    uint32_t test_size;
    char dev_name[DEV_NAME_LEN];
    uint32_t flash_base;
    uint32_t ctrl_base;
    char sysfs_name[DEV_NAME_LEN];
    uint32_t dev_base;
    uint32_t per_len;
    char mtd_name[DEV_NAME_LEN];
} firmware_sysfs_device_t;

typedef struct firmware_upgrade_device_s {
    char type[TYPE_LEN];
    uint32_t chain;
    uint32_t chip_index;

    uint32_t en_gpio_num;        /* the number of en_gpio */
    uint32_t en_gpio[ENABLE_NUM];
    uint32_t en_level[ENABLE_NUM];

    uint32_t en_logic_num;       /* the number of en_logic */
    char en_logic_dev[ENABLE_NUM][DEV_NAME_LEN];
    uint32_t en_logic_addr[ENABLE_NUM];
    uint32_t en_logic_mask[ENABLE_NUM];
    uint32_t en_logic_en_val[ENABLE_NUM];
    uint32_t en_logic_dis_val[ENABLE_NUM];
    uint32_t en_logic_width[ENABLE_NUM];

    int device_flag;
    union {
        firmware_jtag_device_t jtag;
        firmware_sysfs_device_t sysfs;
    } upg_type;

} firmware_upgrade_device_t;

#endif
