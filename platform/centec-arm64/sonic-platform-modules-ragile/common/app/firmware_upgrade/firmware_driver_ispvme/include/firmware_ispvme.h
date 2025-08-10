#ifndef __FIRMWARE_H__
#define __FIRMWARE_H__

#include <linux/miscdevice.h>
#include <linux/platform_device.h>

#include <asm/ioctl.h>
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))
#define dev_debug(debug, fmt, arg...)  \
    if (debug == 1) { do{printk(KERN_ERR fmt,##arg);} while(0); }

#define FIRMWARE_NAME_LEN      48

#define FIRMWARE_FAILED              (-1)
#define FIRMWARE_SUCCESS             0

enum firmware_type_s {
    FIRMWARE_CPLD = 0,
    FIRMWARE_FPGA,
};

/* ioctl command */
#define FIRMWARE_TYPE 'F'
#define FIRMWARE_JTAG_TDI                _IOR(FIRMWARE_TYPE, 0, char)
#define FIRMWARE_JTAG_TDO                _IOR(FIRMWARE_TYPE, 1, char)
#define FIRMWARE_JTAG_TCK                _IOR(FIRMWARE_TYPE, 2, char)
#define FIRMWARE_JTAG_TMS                _IOR(FIRMWARE_TYPE, 3, char)
#define FIRMWARE_JTAG_EN                 _IOR(FIRMWARE_TYPE, 4, char)
#define FIRMWARE_SET_DEBUG_ON            _IOW(FIRMWARE_TYPE, 5, int)     /* debug on */
#define FIRMWARE_SET_DEBUG_OFF           _IOW(FIRMWARE_TYPE, 6, int)     /* debug off */
#define FIRMWARE_SET_GPIO_INFO           _IOR(FIRMWARE_TYPE, 7, int)     /* Set GPIO pin configuration */

typedef struct cmd_info_s {
    int size;
    void __user *data;
} cmd_info_t;

typedef struct firmware_device_s {
    struct list_head list;             /* device linked list */
    int type;                          /* the type of device */
    int slot;                          /* position */
    char name[FIRMWARE_NAME_LEN];      /* name */
    struct miscdevice dev;             /* device */
    void *priv;                        /* private data */
} firmware_device_t;

typedef struct firmware_driver_s {
    struct list_head list;             /* linked list */
    int type;                          /* type */
    char name[FIRMWARE_NAME_LEN];      /* name */
    struct platform_driver *drv;       /* driver */
    void *priv;                        /* private data */
} firmware_driver_t;

typedef struct gpio_group_s {
    int pin;
    int val;
    int dir;
} gpio_group_t;

typedef struct firmware_upg_gpio_info_s {
    int tdi;
    int tck;
    int tms;
    int tdo;
    int jtag_en;
	int select;
    gpio_group_t jtag_5;
    gpio_group_t jtag_4;
    gpio_group_t jtag_3;
    gpio_group_t jtag_2;
    gpio_group_t jtag_1;
} firmware_upg_gpio_info_t;

extern int firmware_debug(void);
extern void firmware_set_debug(int value);
extern firmware_device_t *firmware_get_device_by_minor(int type, int minor);
extern int firmware_get_device_num(int type);
extern int firmware_device_register(firmware_device_t *fw_dev);
extern void firmware_device_unregister(firmware_device_t *fw_dev);
extern int firmware_driver_register(firmware_driver_t *fw_drv);
extern void firmware_driver_unregister(firmware_driver_t *fw_drv);
extern void firmware_fpga_init(void);
extern void firmware_cpld_init(void);
extern void firmware_fpga_exit(void);
extern void firmware_cpld_exit(void);

#endif /* end of __FIRMWARE_H__ */