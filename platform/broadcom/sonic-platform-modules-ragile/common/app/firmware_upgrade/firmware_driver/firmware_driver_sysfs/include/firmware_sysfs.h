#ifndef __FIRMWARE_SYSFS_H__
#define __FIRMWARE_SYSFS_H__

#include <linux/miscdevice.h>
#include <linux/platform_device.h>

#include <asm/ioctl.h>

/* Debug switch level */
typedef enum {
    FIRWMARE_VERBOSE,
    FIRWMARE_WARN,
    FIRWMARE_ERROR,
    FIRWMARE_END,
} firmware_debug_level_t;

#define FIRMWARE_DRIVER_DEBUG_VERBOSE(fmt, args...) do { \
    if ((g_firmware_driver_debug) & (1U << FIRWMARE_VERBOSE)) { \
        printk(KERN_INFO "[FIRMWARW_DRIVER_SYSFS][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FIRMWARE_DRIVER_DEBUG_ERROR(fmt, args...) do { \
    if ((g_firmware_driver_debug) & (1U << FIRWMARE_ERROR)) { \
        printk(KERN_ERR "[FIRMWARW_DRIVER_SYSFS][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FIRMWARE_NAME_LEN      48

#define FIRMWARE_FAILED              (-1)
#define FIRMWARE_SUCCESS             0

/* ioctl publi command, the same as "firmware_upgrade\include\firmware_app.h" */
#define FIRMWARE_COMMON_TYPE 'C'
#define FIRMWARE_GET_CHIPNAME            _IOR(FIRMWARE_COMMON_TYPE, 0, char)    /* get the chip name */
#define FIRMWARE_GET_VERSION             _IOR(FIRMWARE_COMMON_TYPE, 2, int)     /* get version */

/* firmware sysfs driver ioctl command, the same as "firmware_upgrade\include\firmware_app.h" */
#define FIRMWARE_SYSFS_TYPE 'S'
#define FIRMWARE_SYSFS_INIT               _IOR(FIRMWARE_SYSFS_TYPE, 0, char)   /* enable upgrade access */
#define FIRMWARE_SYSFS_FINISH             _IOR(FIRMWARE_SYSFS_TYPE, 1, char)   /* disable upgrade access */
#define FIRMWARE_SYSFS_SPI_INFO           _IOR(FIRMWARE_SYSFS_TYPE, 2, char)   /* spi flash upgrade */
#define FIRMWARE_SYSFS_DEV_FILE_INFO      _IOR(FIRMWARE_SYSFS_TYPE, 3, char)   /* sysfs upgrade */
#define FIRMWARE_SYSFS_MTD_INFO           _IOR(FIRMWARE_SYSFS_TYPE, 4, char)   /* sysfs mtd upgrade */

#define FIRMWARE_SYSFS_TYPE_SPI_LOGIC       "SPI_LOGIC"
#define FIRMWARE_SYSFS_TYPE_SYSFS           "SYSFS"
#define FIRMWARE_SYSFS_TYPE_MTD             "MTD_DEV"

typedef struct cmd_info_s {
    uint32_t size;
    void __user *data;
} cmd_info_t;

typedef struct firmware_device_s {
    struct list_head list;             /* device list */
    uint32_t chain;                    /* chain number */
    char name[FIRMWARE_NAME_LEN];      /* name */
    struct miscdevice dev;             /* device */
    void *priv;                        /* private data */
} firmware_device_t;

typedef struct firmware_driver_s {
    struct list_head list;             /* list */
    char name[FIRMWARE_NAME_LEN];      /* name */
    struct platform_driver *drv;       /* driver */
    void *priv;                        /* private data */
} firmware_driver_t;

extern int g_firmware_driver_debug;

/* Get device information based on minor */
extern firmware_device_t *firmware_get_device_by_minor(int minor);
/* Registere device */
extern int firmware_device_register(firmware_device_t *fw_dev);
/* Unregister device */
extern void firmware_device_unregister(firmware_device_t *fw_dev);
/* Registere driver */
extern int firmware_driver_register(firmware_driver_t *fw_drv);
/* Unregister driver */
extern void firmware_driver_unregister(firmware_driver_t *fw_drv);
/* SYSFS upgrade initialized */
extern int firmware_sysfs_init(void);
/* SYSFS unload function */
extern void firmware_sysfs_exit(void);

#endif /* end of __FIRMWARE_SYSFS_H__ */
