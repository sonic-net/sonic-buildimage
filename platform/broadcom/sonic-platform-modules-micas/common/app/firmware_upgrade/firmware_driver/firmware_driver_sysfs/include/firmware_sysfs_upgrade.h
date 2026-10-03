#ifndef __FIRMWARE_SYSFS_UPGRADE_H__
#define __FIRMWARE_SYSFS_UPGRADE_H__

#define FIRMWARE_DEV_NAME_LEN     64    /* the macro definition needs to same as app space define */
#define FIRMWARE_TYPE_LEN         10
#define FIRMWARE_EN_INFO_MAX      16

typedef struct firmware_spi_logic_info_s {
    char dev_name[FIRMWARE_DEV_NAME_LEN];        /* Logical device name */
    uint32_t flash_base;                         /* Flash Upgrade Address */
    uint32_t ctrl_base;                          /* SPI upgrade control register base address */
    uint32_t test_base;                          /* Test flash address */
    uint32_t test_size;                          /* Test flash size */
} firmware_spi_logic_info_t;

typedef struct firmware_dev_file_info_s {
    char sysfs_name[FIRMWARE_DEV_NAME_LEN];         /* sysfs name */
    uint32_t dev_base;                              /* device upgrade base address */
    uint32_t per_len;                               /* The length of bytes per operation */
    uint32_t test_base;                             /* Test flash address */
    uint32_t test_size;                             /* Test flash size */
} firmware_dev_file_info_t;

typedef struct firmware_mtd_info_s {
    char mtd_name[FIRMWARE_DEV_NAME_LEN];         /* sysfs name */
    uint32_t flash_base;                          /* Flash Upgrade Address */
    uint32_t test_base;                           /* Test flash address */
    uint32_t test_size;                           /* Test flash size */
} firmware_mtd_info_t;

typedef struct firmware_gpio_jtag_en_s {
    uint32_t en_gpio;                                               /* GPIO enable pin */
    uint32_t en_level;                                              /* GPIO enable level */
    int flag;                                                       /* init flag; 1-init 0-not init */
} firmware_gpio_jtag_en_t;

typedef struct firmware_logic_dev_en_s {
    char dev_name[FIRMWARE_DEV_NAME_LEN];        /* Logical device name */
    uint32_t addr;                               /* Enable register address */
    uint32_t mask;                               /* mask */
    uint32_t en_val;                             /* Enable value */
    uint32_t dis_val;                            /* Disable value*/
    uint32_t width;                              /* width */
    int flag;                                    /* init flag; 1-init 0-not init */
} firmware_logic_dev_en_t;

typedef struct firmware_sysfs_s {
    char devname[FIRMWARE_DEV_NAME_LEN];                        /* Device name */
    char type[FIRMWARE_TYPE_LEN];                               /* interface type */
    uint32_t chain;                                             /* chain num */
    uint32_t chip_index;                                        /* chip index */
    union {
        firmware_spi_logic_info_t spi_logic_info;                   /* SPI logic Information */
        firmware_dev_file_info_t dev_file_info;                     /* device file Information */
        firmware_mtd_info_t mtd_info;                               /* mtd device Information */
    } info;
    uint32_t gpio_en_info_num;                                       /* GPIO Enable Number */
    firmware_gpio_jtag_en_t gpio_en_info[FIRMWARE_EN_INFO_MAX]; /* GPIO Enable Information */
    uint32_t logic_dev_en_num;                                       /* Register Enable Number */
    firmware_logic_dev_en_t logic_dev_en_info[FIRMWARE_EN_INFO_MAX]; /* Register Enable Information */
} firmware_sysfs_t;

typedef struct firmware_sysfs_function_s{
    int (*init_dev)(void);                           /* upgrade initializes the operation */
    int (*finish_dev)(void);                         /* upgrade completes the operation */
}firmware_sysfs_function_t;

extern void firmware_set_sysfs_info(firmware_sysfs_t *sysfs_info);
extern int firmware_init_dev_loc(firmware_sysfs_t *sysfs_info);
extern int firmware_finish_dev_loc(firmware_sysfs_t *sysfs_info);

#endif /* __FIRMWARE_SYSFS_UPGRADE_H__ */
