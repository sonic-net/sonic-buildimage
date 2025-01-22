/*
 * wb_firmware_upgrade.c
 * Original Author: support  2021-03-17
 *
 * ko for firmware device
 * History
 *  [Version]        [Author]           [Date]            [Description]
 *    v1.0          support            2021-05-07          Initial version
 */
#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <firmware_upgrade.h>

static int g_wb_firmware_upgrade_debug = 0;
static int g_wb_firmware_upgrade_error = 0;

module_param(g_wb_firmware_upgrade_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_firmware_upgrade_error, int, S_IRUGO | S_IWUSR);

#define WB_FIRMWARE_UPGRADE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_firmware_upgrade_debug) { \
        printk(KERN_INFO "[WB_FIRMWARE_UPGRADE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_FIRMWARE_UPGRADE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_firmware_upgrade_error) { \
        printk(KERN_ERR "[WB_FIRMWARE_UPGRADE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

/* cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data0 = {
    .type           = "JTAG",
    .upg_type.jtag = {
        .tdi             = 67,
        .tck             = 65,
        .tms             = 6,
        .tdo             = 32,
    },
    .en_gpio[0]     = 50,
    .en_level[0]    = 1,
    .en_gpio[1]     = 48,
    .en_level[1]    = 1,
    .chain          = 1,
    .chip_index     = 1,

    .en_gpio_num    = 2,
    .en_logic_num   = 0,
};

/* fpga */
static firmware_upgrade_device_t firmware_upgrade_device_data1 = {
    .type               = "SPI_LOGIC",
    .chain              = 3,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0xa00,
        .flash_base   = 0x1A0000,
        .test_base    = 0x7F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

static firmware_upgrade_device_t firmware_upgrade_device_data2 = {
    .type               = "SPI_LOGIC",
    .chain              = 4,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0xa00,
        .flash_base   = 0x0,
        .test_base    = 0x7F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* bios */
static firmware_upgrade_device_t firmware_upgrade_device_data3 = {
    .type               = "MTD_DEV",
    .chain              = 2,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .mtd_name     = "BIOS",
        .flash_base   = 0x800000,
    },

    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

static void firmware_device_release(struct device *dev)
{
    return;
}

static struct platform_device firmware_upgrade_device[] = {
    {
        .name   = "firmware_cpld_ispvme",
        .id = 1,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data0,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 2,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data1,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 3,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data2,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 4,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data3,
            .release = firmware_device_release,
        },
    },
 };

 static int __init firmware_upgrade_device_init(void)
 {
     int i;
     int ret = 0;
     firmware_upgrade_device_t *firmware_upgrade_device_data;

     WB_FIRMWARE_UPGRADE_DEBUG_VERBOSE("enter!\n");
     for (i = 0; i < ARRAY_SIZE(firmware_upgrade_device); i++) {
         firmware_upgrade_device_data = firmware_upgrade_device[i].dev.platform_data;
         ret = platform_device_register(&firmware_upgrade_device[i]);
         if (ret < 0) {
             firmware_upgrade_device_data->device_flag = -1; /* device register failed, set flag -1 */
             printk(KERN_ERR "firmware_upgrade_device id%d register failed!\n", i + 1);
         } else {
             firmware_upgrade_device_data->device_flag = 0; /* device register suucess, set flag 0 */
         }
     }
     return 0;
 }

 static void __exit firmware_upgrade_device_exit(void)
 {
     int i;
     firmware_upgrade_device_t *firmware_upgrade_device_data;

     WB_FIRMWARE_UPGRADE_DEBUG_VERBOSE("enter!\n");
     for (i = ARRAY_SIZE(firmware_upgrade_device) - 1; i >= 0; i--) {
         firmware_upgrade_device_data = firmware_upgrade_device[i].dev.platform_data;
         if (firmware_upgrade_device_data->device_flag == 0) { /* device register success, need unregister */
             platform_device_unregister(&firmware_upgrade_device[i]);
         }
     }
 }

 module_init(firmware_upgrade_device_init);
 module_exit(firmware_upgrade_device_exit);
 MODULE_DESCRIPTION("FIRMWARE UPGRADE Devices");
 MODULE_LICENSE("GPL");
 MODULE_AUTHOR("support");
