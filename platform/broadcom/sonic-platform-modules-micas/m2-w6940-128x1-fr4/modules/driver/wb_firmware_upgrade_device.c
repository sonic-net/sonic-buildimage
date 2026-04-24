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

/* IOB FPGA */
static firmware_upgrade_device_t firmware_upgrade_device_data0 = {
    .type               = "SPI_LOGIC",
    .chain              = 1,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0x400,
        .flash_base   = 0x200000,
        .test_base    = 0x7F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* DOM FPGA */
static firmware_upgrade_device_t firmware_upgrade_device_data1 = {
    .type               = "SPI_LOGIC",
    .chain              = 2,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0x1800,
        .flash_base   = 0x200000,
        .test_base    = 0x7F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* SCM CPLD */
static firmware_upgrade_device_t firmware_upgrade_device_data2 = {
    .type               = "SPI_LOGIC",
    .chain              = 3,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0x1200,
        .flash_base   = 0x10000,
        .test_base    = 0x1F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* MCB CPLD */
static firmware_upgrade_device_t firmware_upgrade_device_data3 = {
    .type               = "SPI_LOGIC",
    .chain              = 4,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0x1000,
        .flash_base   = 0x10000,
        .test_base    = 0x1F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* SMB CPLD */
static firmware_upgrade_device_t firmware_upgrade_device_data4 = {
    .type               = "SPI_LOGIC",
    .chain              = 5,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0x1600,
        .flash_base   = 0x10000,
        .test_base    = 0x1F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* FCB_B CPLD */
static firmware_upgrade_device_t firmware_upgrade_device_data5 = {
    .type               = "SPI_LOGIC",
    .chain              = 6,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0x1a00,
        .flash_base   = 0x10000,
        .test_base    = 0x1F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* FCB_T CPLD */
static firmware_upgrade_device_t firmware_upgrade_device_data6 = {
    .type               = "SPI_LOGIC",
    .chain              = 7,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0x1c00,
        .flash_base   = 0x10000,
        .test_base    = 0x1F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* MAC PCIe */
static firmware_upgrade_device_t firmware_upgrade_device_data7 = {
    .type               = "SPI_LOGIC",
    .chain              = 8,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0x1400,
        .flash_base   = 0,
        .test_base    = 0xFF0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* bios */
static firmware_upgrade_device_t firmware_upgrade_device_data8 = {
    .type               = "MTD_DEV",
    .chain              = 9,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .mtd_name     = "BIOS",
        .flash_base   = 0x3000000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* IOB FPGA shaopian */
static firmware_upgrade_device_t firmware_upgrade_device_data9 = {
    .type               = "SPI_LOGIC",
    .chain              = 10,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0x400,
        .flash_base   = 0x0,
        .test_base    = 0x7F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* DOM FPGA shaopian */
static firmware_upgrade_device_t firmware_upgrade_device_data10 = {
    .type               = "SPI_LOGIC",
    .chain              = 11,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0x1800,
        .flash_base   = 0x0,
        .test_base    = 0x7F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* SCM CPLD shaopian */
static firmware_upgrade_device_t firmware_upgrade_device_data11 = {
    .type               = "SPI_LOGIC",
    .chain              = 12,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0x1200,
        .flash_base   = 0x0,
        .test_base    = 0x1F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* MCB CPLD shaopian */
static firmware_upgrade_device_t firmware_upgrade_device_data12 = {
    .type               = "SPI_LOGIC",
    .chain              = 13,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0x1000,
        .flash_base   = 0x0,
        .test_base    = 0x1F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* SMB CPLD shaopian */
static firmware_upgrade_device_t firmware_upgrade_device_data13 = {
    .type               = "SPI_LOGIC",
    .chain              = 14,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0x1600,
        .flash_base   = 0x0,
        .test_base    = 0x1F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* FCB_B CPLD shaopian */
static firmware_upgrade_device_t firmware_upgrade_device_data14 = {
    .type               = "SPI_LOGIC",
    .chain              = 15,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0x1a00,
        .flash_base   = 0x0,
        .test_base    = 0x1F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* FCB_T CPLD shaopian */
static firmware_upgrade_device_t firmware_upgrade_device_data15 = {
    .type               = "SPI_LOGIC",
    .chain              = 16,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0x1c00,
        .flash_base   = 0x0,
        .test_base    = 0x1F0000,
        .test_size    = 0x10000,
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
        .name   = "firmware_sysfs",
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
        .id = 7,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data6,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 8,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data7,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 9,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data8,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 10,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data9,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 11,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data10,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 12,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data11,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 13,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data12,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 14,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data13,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 15,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data14,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_sysfs",
        .id = 16,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data15,
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
