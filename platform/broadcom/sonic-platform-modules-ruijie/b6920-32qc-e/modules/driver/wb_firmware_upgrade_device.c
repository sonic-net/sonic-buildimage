/*
 * wb_firmware_upgrade.c
 *
 * ko for firmware device
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

/* base cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data0 = {
    .type               = "JTAG",
    .upg_type.jtag = {
        .tdi             = 89,
        .tck             = 139,
        .tms             = 87,
        .tdo             = 88,
    },

    .en_gpio[0]     = 99,
    .en_level[0]    = 0,
    .chain          = 1,
    .chip_index     = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0xa6,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x1,
    .en_logic_dis_val[0] = 0x3,
    .en_logic_width[0]   = 0x1,

    .en_gpio_num    = 1,
    .en_logic_num   = 1,
};

/* mac cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data1 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi             = 89,
        .tck             = 139,
        .tms             = 87,
        .tdo             = 88,
    },

    .en_gpio[0]     = 99,
    .en_level[0]    = 0,
    .chain          = 2,
    .chip_index     = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x91,
    .en_logic_mask[0]    = 0xf8,
    .en_logic_en_val[0]  = 0x2,
    .en_logic_dis_val[0] = 0x0,
    .en_logic_width[0]   = 0x1,
    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x90,
    .en_logic_mask[1]    = 0xfc,
    .en_logic_en_val[1]  = 0x1,
    .en_logic_dis_val[1] = 0x0,
    .en_logic_width[1]   = 0x1,
    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0xa6,
    .en_logic_mask[2]    = 0xfc,
    .en_logic_en_val[2]  = 0x2,
    .en_logic_dis_val[2] = 0x3,
    .en_logic_width[2]   = 0x1,
    .en_logic_dev[3]     = "/dev/cpld1",
    .en_logic_addr[3]    = 0xd8,
    .en_logic_mask[3]    = 0xf8,
    .en_logic_en_val[3]  = 0x6,
    .en_logic_dis_val[3] = 0x7,
    .en_logic_width[3]   = 0x1,

    .en_gpio_num    = 1,
    .en_logic_num   = 4,
};

/* fan cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data2 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi             = 89,
        .tck             = 139,
        .tms             = 87,
        .tdo             = 88,
    },

    .en_gpio[0]     = 99,
    .en_level[0]    = 0,
    .chain          = 3,
    .chip_index     = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x91,
    .en_logic_mask[0]    = 0xf8,
    .en_logic_en_val[0]  = 0x4,
    .en_logic_dis_val[0] = 0x0,
    .en_logic_width[0]   = 0x1,
    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x90,
    .en_logic_mask[1]    = 0xfc,
    .en_logic_en_val[1]  = 0x1,
    .en_logic_dis_val[1] = 0x0,
    .en_logic_width[1]   = 0x1,
    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0xa6,
    .en_logic_mask[2]    = 0xfc,
    .en_logic_en_val[2]  = 0x2,
    .en_logic_dis_val[2] = 0x3,
    .en_logic_width[2]   = 0x1,
    .en_logic_dev[3]     = "/dev/cpld1",
    .en_logic_addr[3]    = 0xd8,
    .en_logic_mask[3]    = 0xf8,
    .en_logic_en_val[3]  = 0x3,
    .en_logic_dis_val[3] = 0x7,
    .en_logic_width[3]   = 0x1,

    .en_gpio_num    = 1,
    .en_logic_num   = 4,
};

/* cpu cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data3 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi             = 89,
        .tck             = 139,
        .tms             = 87,
        .tdo             = 88,
    },

    .en_gpio[0]     = 99,
    .en_level[0]    = 0,
    .chain          = 4,
    .chip_index     = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x91,
    .en_logic_mask[0]    = 0xf8,
    .en_logic_en_val[0]  = 0x3,
    .en_logic_dis_val[0] = 0x0,
    .en_logic_width[0]   = 0x1,
    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x90,
    .en_logic_mask[1]    = 0xfc,
    .en_logic_en_val[1]  = 0x1,
    .en_logic_dis_val[1] = 0x0,
    .en_logic_width[1]   = 0x1,
    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0xa6,
    .en_logic_mask[2]    = 0xfc,
    .en_logic_en_val[2]  = 0x2,
    .en_logic_dis_val[2] = 0x3,
    .en_logic_width[2]   = 0x1,

    .en_gpio_num    = 1,
    .en_logic_num   = 3,
};

/* mac fpga */
static firmware_upgrade_device_t firmware_upgrade_device_data4 = {
     .type               = "MTD_DEV",
     .chain              = 1,
     .chip_index         = 1,
     .upg_type.sysfs = {
         .mtd_name       = "spi0.0",
         .flash_base     = 0x2f0000,
         .test_base      = 0x7F0000,
         .test_size      = 0x10000,
     },
     .en_gpio_num        = 0,
     .en_logic_num       = 0,
 };

/* 5387 */
static firmware_upgrade_device_t firmware_upgrade_device_data5 = {
    .type               = "SYSFS",
    .chain              = 2,
    .chip_index         = 1,
    .upg_type.sysfs = {
      .sysfs_name   = "/sys/bus/spi/devices/spi0.0/eeprom",
    },

    .en_gpio[0]     = 99,
    .en_level[0]    = 0,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x91,
    .en_logic_mask[0]    = 0xf8,
    .en_logic_en_val[0]  = 0x6,
    .en_logic_dis_val[0] = 0x0,
    .en_logic_width[0]   = 0x1,
    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x90,
    .en_logic_mask[1]    = 0xfc,
    .en_logic_en_val[1]  = 0x1,
    .en_logic_dis_val[1] = 0x0,
    .en_logic_width[1]   = 0x1,
    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0xa6,
    .en_logic_mask[2]    = 0xfc,
    .en_logic_en_val[2]  = 0x2,
    .en_logic_dis_val[2] = 0x3,
    .en_logic_width[2]   = 0x1,
    .en_logic_dev[3]     = "/dev/cpld1",
    .en_logic_addr[3]    = 0xd4,
    .en_logic_mask[3]    = 0xfe,
    .en_logic_en_val[3]  = 0x0,
    .en_logic_dis_val[3] = 0x1,
    .en_logic_width[3]   = 0x1,

    .en_gpio_num        = 1,
    .en_logic_num       = 4,
};

/* bios */
static firmware_upgrade_device_t firmware_upgrade_device_data6 = {
    .type               = "MTD_DEV",
    .chain              = 3,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .mtd_name     = "BIOS",
        .flash_base   = 0x800000,
    },

    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};


/* mac fpga */
static firmware_upgrade_device_t firmware_upgrade_device_data7 = {
    .type               = "MTD_DEV",
    .chain              = 4,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .mtd_name       = "spi0.0",
        .flash_base     = 0x0,
        .test_base      = 0x7F0000,
        .test_size      = 0x10000,
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
        .name   = "firmware_cpld_ispvme",
        .id = 2,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data1,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_cpld_ispvme",
        .id = 3,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data2,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_cpld_ispvme",
        .id = 4,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data3,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 5,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data4,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 6,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data5,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 9,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data6,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 10,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data7,
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
