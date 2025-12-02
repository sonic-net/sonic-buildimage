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
#include <linux/version.h>
#include <linux/gpio.h>

#define GPIO_D1700_NUM      (237)                               /* D1700 CPU GPIO number */
#define GPIO_D1700_OFFSET   (ARCH_NR_GPIOS - GPIO_D1700_NUM)    /* Formula of gpio base number in Kernel */

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
        .tdi            = 54 + GPIO_D1700_OFFSET,
        .tck            = 55 + GPIO_D1700_OFFSET,
        .tms            = 53 + GPIO_D1700_OFFSET,
        .tdo            = 52 + GPIO_D1700_OFFSET,
    },
    .chain              = 1,
    .chip_index         = 1,
    .en_gpio[0]         = 49 + GPIO_D1700_OFFSET,
    .en_level[0]        = 1,
    .en_gpio[1]         = 56 + GPIO_D1700_OFFSET,
    .en_level[1]        = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x45,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x01,
    .en_logic_dis_val[0] = 0x00,
    .en_logic_width[0]   = 0x1,

    .en_gpio_num        = 2,
    .en_logic_num       = 1,
};

/* mac cpld a/b  */
static firmware_upgrade_device_t firmware_upgrade_device_data1 = {
    .type                = "JTAG",
    .chain               = 2,
    .upg_type.jtag = {
        .tdi             = 54 + GPIO_D1700_OFFSET,
        .tck             = 55 + GPIO_D1700_OFFSET,
        .tms             = 53 + GPIO_D1700_OFFSET,
        .tdo             = 52 + GPIO_D1700_OFFSET,
    },
    .chip_index          = 1,
    .en_gpio[0]          = 48 + GPIO_D1700_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 49 + GPIO_D1700_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x45,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x01,
    .en_logic_dis_val[0] = 0x00,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x46,
    .en_logic_mask[1]    = 0x00,
    .en_logic_en_val[1]  = 0x02,
    .en_logic_dis_val[1] = 0x00,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0x49,
    .en_logic_mask[2]    = 0xfc,
    .en_logic_en_val[2]  = 0x02,
    .en_logic_dis_val[2] = 0x00,
    .en_logic_width[2]   = 0x1,

    .en_logic_dev[3]     = "/dev/cpld1",
    .en_logic_addr[3]    = 0x4b,
    .en_logic_mask[3]    = 0x00,
    .en_logic_en_val[3]  = 0xfe,
    .en_logic_dis_val[3] = 0xff,
    .en_logic_width[3]   = 0x1,

    .en_gpio_num        = 2,
    .en_logic_num       = 4,
};

/* mac cpld c */
static firmware_upgrade_device_t firmware_upgrade_device_data2 = {
    .type                = "JTAG",
    .chain               = 3,
    .upg_type.jtag = {
        .tdi             = 54 + GPIO_D1700_OFFSET,
        .tck             = 55 + GPIO_D1700_OFFSET,
        .tms             = 53 + GPIO_D1700_OFFSET,
        .tdo             = 52 + GPIO_D1700_OFFSET,
    },
    .chip_index          = 1,
    .en_gpio[0]          = 48 + GPIO_D1700_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 49 + GPIO_D1700_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x45,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x01,
    .en_logic_dis_val[0] = 0x00,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x46,
    .en_logic_mask[1]    = 0x00,
    .en_logic_en_val[1]  = 0x02,
    .en_logic_dis_val[1] = 0x00,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0x49,
    .en_logic_mask[2]    = 0xfc,
    .en_logic_en_val[2]  = 0x01,
    .en_logic_dis_val[2] = 0x00,
    .en_logic_width[2]   = 0x1,

    .en_gpio_num        = 2,
    .en_logic_num       = 3,
};

/* fan cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data3 = {
    .type                = "JTAG",
    .chain               = 4,
    .upg_type.jtag = {
        .tdi             = 54 + GPIO_D1700_OFFSET,
        .tck             = 55 + GPIO_D1700_OFFSET,
        .tms             = 53 + GPIO_D1700_OFFSET,
        .tdo             = 52 + GPIO_D1700_OFFSET,
    },
    .chip_index          = 1,
    .en_gpio[0]          = 48 + GPIO_D1700_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 49 + GPIO_D1700_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x45,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x01,
    .en_logic_dis_val[0] = 0x00,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x46,
    .en_logic_mask[1]    = 0x00,
    .en_logic_en_val[1]  = 0x04,
    .en_logic_dis_val[1] = 0x00,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0x49,
    .en_logic_mask[2]    = 0xfc,
    .en_logic_en_val[2]  = 0x02,
    .en_logic_dis_val[2] = 0x00,
    .en_logic_width[2]   = 0x1,

    .en_logic_dev[3]     = "/dev/cpld1",
    .en_logic_addr[3]    = 0x4b,
    .en_logic_mask[3]    = 0x00,
    .en_logic_en_val[3]  = 0xfd,
    .en_logic_dis_val[3] = 0xff,
    .en_logic_width[3]   = 0x1,

    .en_gpio_num        = 2,
    .en_logic_num       = 4,
};

/* mgmt cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data4 = {
    .type                = "JTAG",
    .chain               = 5,
    .upg_type.jtag = {
        .tdi             = 54 + GPIO_D1700_OFFSET,
        .tck             = 55 + GPIO_D1700_OFFSET,
        .tms             = 53 + GPIO_D1700_OFFSET,
        .tdo             = 52 + GPIO_D1700_OFFSET,
    },
    .chip_index          = 1,
    .en_gpio[0]          = 48 + GPIO_D1700_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 49 + GPIO_D1700_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x45,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x01,
    .en_logic_dis_val[0] = 0x00,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x46,
    .en_logic_mask[1]    = 0x00,
    .en_logic_en_val[1]  = 0x03,
    .en_logic_dis_val[1] = 0x00,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0x49,
    .en_logic_mask[2]    = 0xfc,
    .en_logic_en_val[2]  = 0x02,
    .en_logic_dis_val[2] = 0x00,
    .en_logic_width[2]   = 0x1,

    .en_logic_dev[3]     = "/dev/cpld1",
    .en_logic_addr[3]    = 0x4b,
    .en_logic_mask[3]    = 0x00,
    .en_logic_en_val[3]  = 0xfb,
    .en_logic_dis_val[3] = 0xff,
    .en_logic_width[3]   = 0x1,

    .en_gpio_num        = 2,
    .en_logic_num       = 4,
};

/* cpu cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data5 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi             = 54 + GPIO_D1700_OFFSET,
        .tck             = 55 + GPIO_D1700_OFFSET,
        .tms             = 53 + GPIO_D1700_OFFSET,
        .tdo             = 52 + GPIO_D1700_OFFSET,
    },
    .chain               = 6,
    .chip_index          = 1,
    .en_gpio[0]          = 48 + GPIO_D1700_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 49 + GPIO_D1700_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x45,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x01,
    .en_logic_dis_val[0] = 0x00,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x46,
    .en_logic_mask[1]    = 0x00,
    .en_logic_en_val[1]  = 0x01,
    .en_logic_dis_val[1] = 0x00,
    .en_logic_width[1]   = 0x1,

    .en_gpio_num   = 2,
    .en_logic_num       = 2,
};

/* mac fpga */
static firmware_upgrade_device_t firmware_upgrade_device_data6 = {
    .type                = "SPI_LOGIC",
    .chain               = 1,
    .chip_index          = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga0",
        .ctrl_base    = 0xa00,
        .flash_base   = 0x2f0000,
        .test_base    = 0x7F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* bcm53134 */
static firmware_upgrade_device_t firmware_upgrade_device_data7 = {
    .type                = "SYSFS",
    .chain               = 2,
    .chip_index          = 1,
    .upg_type.sysfs = {
        .sysfs_name     = "/sys/bus/spi/devices/spi0.0/eeprom",
    },
    .en_gpio[0]          = 48 + GPIO_D1700_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 49 + GPIO_D1700_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x45,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x01,
    .en_logic_dis_val[0] = 0x00,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x46,
    .en_logic_mask[1]    = 0x00,
    .en_logic_en_val[1]  = 0x06,
    .en_logic_dis_val[1] = 0x00,
    .en_logic_width[1]   = 0x1,

    .en_gpio_num        = 2,
    .en_logic_num       = 2,
};

/* mac fpga shaopian */
static firmware_upgrade_device_t firmware_upgrade_device_data8 = {
    .type                = "SPI_LOGIC",
    .chain               = 3,
    .chip_index          = 1,
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
static firmware_upgrade_device_t firmware_upgrade_device_data9 = {
    .type               = "MTD_DEV",
    .chain              = 4,
    .chip_index         = 1,
    .upg_type.sysfs = {
        .mtd_name     = "BIOS",
        .flash_base   = 0x3000000,
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
        .name   = "firmware_cpld_ispvme",
        .id = 5,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data4,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_cpld_ispvme",
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
