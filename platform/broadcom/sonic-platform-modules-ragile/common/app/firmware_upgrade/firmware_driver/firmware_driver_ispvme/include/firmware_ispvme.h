#ifndef __FIRMWARE_H__
#define __FIRMWARE_H__

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
        printk(KERN_INFO "[FIRMWARW_DRIVER_ISPVME][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FIRMWARE_DRIVER_DEBUG_ERROR(fmt, args...) do { \
    if ((g_firmware_driver_debug) & (1U << FIRWMARE_ERROR)) { \
        printk(KERN_ERR "[FIRMWARW_DRIVER_ISPVME][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define FIRMWARE_NAME_LEN      48

#define FIRMWARE_FAILED              (-1)
#define FIRMWARE_SUCCESS             0

/* ioctl publi command, the same as "firmware_upgrade\include\firmware_app.h" */
#define FIRMWARE_COMMON_TYPE 'C'
#define FIRMWARE_GET_CHIPNAME            _IOR(FIRMWARE_COMMON_TYPE, 0, char)    /* get the chip name */
#define FIRMWARE_GET_VERSION             _IOR(FIRMWARE_COMMON_TYPE, 2, int)     /* get version */

/* firmware cpld ispvme driver ioctl command, the same as "firmware_upgrade\include\firmware_app.h" */
#define FIRMWARE_VME_TYPE 'V'
#define FIRMWARE_JTAG_TDI                _IOR(FIRMWARE_VME_TYPE, 0, char)
#define FIRMWARE_JTAG_TDO                _IOR(FIRMWARE_VME_TYPE, 1, char)
#define FIRMWARE_JTAG_TCK                _IOR(FIRMWARE_VME_TYPE, 2, char)
#define FIRMWARE_JTAG_TMS                _IOR(FIRMWARE_VME_TYPE, 3, char)
#define FIRMWARE_JTAG_EN                 _IOR(FIRMWARE_VME_TYPE, 4, char)
#define FIRMWARE_JTAG_INIT               _IOR(FIRMWARE_VME_TYPE, 7, char)   /* enable upgrade access */
#define FIRMWARE_JTAG_FINISH             _IOR(FIRMWARE_VME_TYPE, 8, char)   /* disable upgrade access */

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
/* CPLD upgrade initialized */
extern int firmware_cpld_init(void);
/* CPLD unload function */
extern void firmware_cpld_exit(void);

#endif /* end of __FIRMWARE_H__ */
