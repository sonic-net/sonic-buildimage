#ifndef __FIRMWARE_UPGRADE_SYSFS_H__
#define __FIRMWARE_UPGRADE_SYSFS_H__

#define FIRMWARE_DEV_NAME_LEN          (64) /* the macro definition needs to same as FIRMWARE_DEV_NAME_LEN in firmware_sysfs_upgrade.h */
#define FW_SYSFS_RETRY_SLEEP_TIME      (10000)   /* 10ms */
#define FW_SYSFS_RETRY_TIME            (5)  /* retry 5 times, 50ms = FW_SYSFS_RETRY_TIME *FW_SYSFS_RETRY_SLEEP_TIME;  */

/* sysfs_xdpe Single read/write length */
#define FW_SYSFS_XDPE_RW_LENGTH                 (1)
/* xdpe The upgrade file is parsed line by line, with a maximum length per line. */
#define FW_SYSFS_XDPE_ANALYSE_LINE_LEN_MAX      (256)
/* xdpe The upgrade file is parsed line by line, with a minimum length for valid lines. */
#define FW_SYSFS_XDPE_ANALYSE_LINE_LEN_MIN      (10)
/* xdpe The upgrade file is parsed line by line, with the length of each information string in every line. */
#define FW_SYSFS_XDPE_ANALYSE_LINE_INFO_LEN     (4)
/* sysfs_xdpe In the read/write interface, setting bit 16 of the offset to 1 indicates that page switching is required. */
#define FW_SYSFS_XDPE_RW_SET_PAGE_MASK          (0x10000)

typedef struct firmware_dev_file_info_s {
    char sysfs_name[FIRMWARE_DEV_NAME_LEN];              /* sysfs name */
    uint32_t dev_base;                                   /* device upgrade base address */
    uint32_t per_len;                                    /* The length of bytes per operation */
    uint32_t test_base;                                  /* Test device address */
    uint32_t test_size;                                  /* Test flash size */
} firmware_dev_file_info_t;

typedef struct firmware_xdpe_file_info_s {
    uint8_t page;                               /* xdpe The upgrade file is parsed line by line, the page information in each line. */
    uint8_t addr;                               /* xdpe The upgrade file is parsed line by line, the offset information in each line. */
    uint8_t val;                                /* xdpe The upgrade file is parsed line by line, the value information in each line. */
    uint8_t mask;                               /* xdpe The upgrade file is parsed line by line, the mask information in each line. */
} firmware_xdpe_file_info_t;

#endif /* End of __FIRMWARE_UPGRADE_SYSFS_H__ */
