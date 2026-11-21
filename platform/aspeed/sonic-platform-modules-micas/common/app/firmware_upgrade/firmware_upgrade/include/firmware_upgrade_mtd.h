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
    char mtd_name[FIRMWARE_DEV_NAME_LEN];         /* mtd label/name from sysfs driver */
    uint32_t flash_base;                               /* Flash Upgrade Address */
    uint32_t test_base;                                /* Test flash address */
    uint32_t test_size;                                /* Test flash size */
} firmware_mtd_info_t;

typedef struct firmware_mtd_block_d {
    uint32_t block_num; /* upgrade mtd partition block count */
    int uconf_skip;
    uint8_t *org_data_buf; /* upgrade data pointer */
    uint32_t org_data_size; /* total size of upgrade data */

    struct firmware_upgrade_mtd
    {
        char name[FIRMWARE_DEV_NAME_LEN]; /* upgrade segment corresponding sysfs mtd device name, e.g., mtd14 */
        uint32_t offset; /* upgrade segment offset */
        uint32_t size;   /* upgrade segment size */
        uint8_t skip_flag; /* whether to skip this segment upgrade, 1: skip; 0: upgrade */
        uint8_t *data;   /* upgrade segment data pointer */
     } block[]; /* upgrade segment information, including segment offset, size, and data pointer, segment information is constructed by the upper layer according to actual situation and passed in, the lower layer executes the upgrade according to the segment information */
} firmware_mtd_block_t;

typedef struct firmware_mtd_upg_ctx_s {
    uint32_t magic;  /* additional magic number check to prevent incorrect memory access due to misuse of info->priv_data */
    int uconf_skip;
    firmware_mtd_block_t *block_info;
} firmware_mtd_upg_ctx_t;

int firmware_upgrade_mtd_topo_blk_alloc(const char *mtd_name, firmware_mtd_block_t **block_info);
void firmware_upgrade_mtd_topo_blk_free(firmware_mtd_block_t **block_info);
int firmware_upgrade_mtd_get_dev_name(const char *label_name, char *mtd_dev_name, int len);
int firmware_upgrade_mtd_ctx_alloc(void **ctx_out);
void firmware_upgrade_mtd_ctx_free(void **ctx_inout);

#endif /* End of __FIRMWARE_UPGRADE_MTD_H__ */
