#ifndef __FIRMWARE_UPGRADE_SYSFS_H__
#define __FIRMWARE_UPGRADE_SYSFS_H__

#define FIRMWARE_DEV_NAME_LEN          (64) /* the macro definition needs to same as FIRMWARE_DEV_NAME_LEN in firmware_sysfs_upgrade.h */
#define FW_SYSFS_RETRY_SLEEP_TIME      (10000)   /* 10ms */
#define FW_SYSFS_RETRY_TIME            (5)  /* retry 5 times, 50ms = FW_SYSFS_RETRY_TIME *FW_SYSFS_RETRY_SLEEP_TIME;  */

typedef struct firmware_dev_file_info_s {
    char sysfs_name[FIRMWARE_DEV_NAME_LEN];         /* sysfs name */
    uint32_t dev_base;                                   /* device upgrade base address */
    uint32_t per_len;                                    /* The length of bytes per operation */
    uint32_t test_base;                                  /* Test device address */
    uint32_t test_size;                                  /* Test flash size */
} firmware_dev_file_info_t;

#endif /* End of __FIRMWARE_UPGRADE_SYSFS_H__ */
