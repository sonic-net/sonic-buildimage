#ifndef __FIRMWARE_UPGRADE_MTD_H__
#define __FIRMWARE_UPGRADE_MTD_H__

#include <firmware_app.h>

#define FIRMWARE_DEV_NAME_LEN     64    /* the macro definition needs to same as FIRMWARE_DEV_NAME_LEN in firmware_sysfs_upgrade.h */
#define PATH_LEN    (256)
#define FW_MTD_BLOCK_SLEEP_TIME        (10000)   /* 10ms */
#define FW_SYSFS_RETRY_SLEEP_TIME      (10000)   /* 10ms */
#define FW_SYSFS_RETRY_TIME            (5)  /* retry 5 times, 50ms = FW_SYSFS_RETRY_TIME *FW_SYSFS_RETRY_SLEEP_TIME;  */

/* Debug switch level */
typedef enum {
    FIRWMARE_MTD_SUCCESS = 0,
    FIRWMARE_MTD_PART_INFO_ERR,
    FIRWMARE_MTD_MEMERASE,
    FIRWMARE_MTD_MEMGETINFO,
    FIRWMARE_END,
} firmware_debug_level_t;

#define debug(fmt, argv...) do {  \
    dbg_print(is_debug_on, ""fmt , ##argv);\
 } while(0)

typedef struct firmware_mtd_info_s {
    char mtd_name[FIRMWARE_DEV_NAME_LEN];         /* sysfs name */
    uint32_t flash_base;                               /* Flash Upgrade Address */
    uint32_t test_base;                                /* Test flash address */
    uint32_t test_size;                                /* Test flash size */
} firmware_mtd_info_t;

#endif /* End of __FIRMWARE_UPGRADE_MTD_H__ */
