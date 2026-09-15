#ifndef _SWITCH_H_
#define _SWITCH_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include "switch_driver.h"
#include "wb_module.h"

#define DIR_NAME_MAX_LEN        (64)
#define DEBUG_FILE_SIZE     (64)
#define DEV_PRESEN_STR          "1\n"
#define DEV_ABSENT_STR          "0\n"

#define E2_UPGRADE_FLAG                  (1)
#define E2_NON_UPGRADE_FLAG              (0)

enum LOG_LEVEL {
    INFO = 0x1,
    ERR  = 0x2,
    DBG  = 0x4,
    ALL  = 0xf
};

extern int g_switch_loglevel;

#define check_pfun(p) do { \
    if (p == NULL) { \
        if (g_switch_loglevel & ERR) { \
            printk(KERN_ERR "%s, %s is NULL.\n", __FUNCTION__, #p); \
        } \
        return -WB_SYSFS_RV_UNSUPPORT; \
    } \
} while (0)

#define check_p(p) check_pfun(p)

#define to_switch_obj(x) container_of(x, struct switch_obj, kobj)
#define to_switch_attr(x) container_of(x, struct switch_attribute, attr)
#define to_switch_device_attr(x) container_of(x, struct switch_device_attribute, switch_attr)

#define SWITCH_ATTR(_name, _mode, _show, _store, _type)    \
    { .switch_attr = __ATTR(_name, _mode, _show, _store),  \
      .type = _type }

#define SWITCH_DEVICE_ATTR(_name, _mode, _show, _store, _type) \
struct switch_device_attribute switch_dev_attr_##_name         \
        = SWITCH_ATTR(_name, _mode, _show, _store, _type)

#define SWITCH_DEVICE_ATTR_PRE(_prefix, _name, _mode, _show, _store, _type) \
struct switch_device_attribute switch_dev_attr_##_prefix##_##_name         \
        = SWITCH_ATTR(_name, _mode, _show, _store, _type)

struct switch_obj {
    struct kobject kobj;
    unsigned int index;
};

/* a custom attribute that works just for a struct switch_obj. */
struct switch_attribute {
    struct attribute attr;
    ssize_t (*show)(struct switch_obj *foo, struct switch_attribute *attr, char *buf);
    ssize_t (*store)(struct switch_obj *foo, struct switch_attribute *attr, const char *buf, size_t count);
};

struct switch_device_attribute {
    struct switch_attribute switch_attr;
    int type;
};

/* system api type */
typedef enum wb_plat_system_type_e {
    WB_SYSTEM_BMC_READY            = 0x0000,  /* bmc ready         */
    WB_SYSTEM_SOL_ACTIVE           = 0x0100,  /* sol active        */
    WB_SYSTEM_PSU_RESET            = 0x0200,  /* psu reset         */
    WB_SYSTEM_CPU_BOARD_CTRL       = 0x0300,  /* cpu board control */
    WB_SYSTEM_CPU_BOARD_STATUS     = 0x0400,  /* cpu board status  */
    WB_SYSTEM_BIOS_UPGRADE         = 0x0500,  /* bios upgrade      */
    WB_SYSTEM_BIOS_SWITCH          = 0x0600,  /* bios boot switch  */
    WB_SYSTEM_BIOS_VIEW            = 0x0700,  /* bios flash view   */
    WB_SYSTEM_BIOS_BOOT_OK         = 0x0800,  /* bios boot status  */
    WB_SYSTEM_BIOS_FAIL_RECORD     = 0x0900,  /* bios startup failure record */
    WB_SYSTEM_BMC_RESET            = 0x0a00,  /* bmc reset         */
    WB_SYSTEM_MAC_BOARD_RESET      = 0x0b00,  /* mac board reset   */
    WB_SYSTEM_MAC_PWR_CTRL         = 0x0c00,  /* mac power on/off  */
    WB_SYSTEM_EMMC_PWR_CTRL        = 0x0d00,  /* emmc power on/off */
    WB_SYSTEM_PORT_PWR_CTL         = 0x0e00,  /* power power on/off*/
    WB_SYSTEM_BMC_VIEW             = 0x0f00,  /* bmc view          */
    WB_SYSTEM_BMC_SWITCH           = 0x1000,  /* bmc switch        */
    WB_SYSTEM_IS_MAIN_MGMT_BOARD   = 0x1100,  /* is main management board */
    WB_SYSTEM_BIOS_FLASH_SWITCH    = 0x1200,  /* bios flash switch */
    WB_SYSTEM_SSD_PWR_CTRL         = 0x1300,  /* ssd power control, 0: off, 1: on */
    WB_SYSTEM_SSD_PWR_STATUS       = 0x1400,  /* ssd power status, 0: off, 1: on */
    WB_SYSTEM_MAC_PWR_STATUS       = 0x1500,  /* mac power status, 0: off, 1: on */
    WB_SYSTEM_PORT_PWR_STATUS      = 0x1600,  /* port power status, 0: off, 1: on */
    WB_SYSTEM_BMC_RESET_CNT        = 0x1700,  /* bmc reset count */
    WB_SYSTEM_CPU_RESET_CNT        = 0x1800,  /* cpu reset count */
    WB_SYSTEM_CPU_RESET_TYPE       = 0x1900,  /* cpu reset type */
    WB_SYSTEM_PANEL_COM_OWNER      = 0x1a00,  /* Panel serial port ownership, 0: CPU, 1: BMC */
    WB_SYSTEM_PANEL_COM_SWITCH     = 0x1b00,  /* Panel serial port switch, 0: CPU, 1: BMC */
    WB_SYSTEM_CPU_RTC_CLEAR        = 0x1c00,  /* clear cpu rtc*/
    WB_SYSTEM_CPU_RTC_CLEAR_STATUS = 0x1d00,  /* 0:on going 1: done */
    WB_SYSTEM_BMC_RESET_EVENT_DONE = 0x1e00,  /* bmc reset done 0: none, 1: done */
    WB_SYSTEM_CPU_PWRUP_STATUS     = 0X1f00,  /* cpu power up status */
    WB_SYSTEM_BIOS_DEBUG_MODE      = 0X2000,  /* bios debug */
    WB_SYSTEM_POWER_CYCLE_MODE     = 0X2100,  /* ipmi power cycle mode */
    WB_SYSTEM_BMC_ACCESS_PERM      = 0x2200,  /* 0:no access, 1:access */
    WB_SYSTEM_RMC_ROLE             = 0X2300,  /* 1: master rmc, 0: slave rmc, NA: bmc */
    WB_SYSTEM_CPU_TYPE             = 0X2400,  /* 0: intel, 1: amd, NA: intel */
    WB_SYSTEM_PSU_OFF              = 0X2500,  /* psu off           */
    WB_SYSTEM_DEV_RATED_POWER      = 0X2600,  /* device rated power */
    WB_SYSTEM_MGMT_LINK_STATUS     = 0X2700,  /* 1: mgmt link up, 0: mgmt link down, NA: unknown */
    WB_SYSTEM_MEM_ISOLATION        = 0X2800,  /* memory isolation switch, 0: support, 1: not support */
    /*0x51~0xff used in CPU */
    WB_SYSTEM_SW_INIT_ENABLE       = 0x5100,  /* sysfs sw_init */
} wb_plat_system_type_t;

struct switch_obj *switch_kobject_create(const char *name, struct kobject *parent);
void switch_kobject_delete(struct switch_obj **obj);
int dev_debug_file_read(char *file_name, unsigned int dev_index, char *buf, int size);

#endif /* _SWITCH_H_ */
