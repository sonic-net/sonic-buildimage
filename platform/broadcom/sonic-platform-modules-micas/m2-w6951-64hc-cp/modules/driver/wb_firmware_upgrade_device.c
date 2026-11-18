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

#define GPIO_AMD_NUM        (256)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
#define GPIO_OFFSET   (GPIO_DYNAMIC_BASE)    /* Formula of gpio base number in Kernel */
#else
#define GPIO_OFFSET   (ARCH_NR_GPIOS - GPIO_AMD_NUM)    /* Formula of gpio base number in Kernel */
#endif

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
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi             = 5 + GPIO_OFFSET,
        .tck             = 7 + GPIO_OFFSET,
        .tms             = 8 + GPIO_OFFSET,
        .tdo             = 6 + GPIO_OFFSET,
    },
    .chain               = 1,
    .chip_index          = 1,

    .chain              = 1,
    .chip_index         = 1,
    .en_gpio[0]         = 91 + GPIO_OFFSET,
    .en_level[0]        = 1,

    .en_gpio_num        = 1,
    .en_logic_num       = 0,
};

/* mac cpld a */
static firmware_upgrade_device_t firmware_upgrade_device_data1 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi             = 5 + GPIO_OFFSET,
        .tck             = 7 + GPIO_OFFSET,
        .tms             = 8 + GPIO_OFFSET,
        .tdo             = 6 + GPIO_OFFSET,
    },
    .chain               = 2,
    .chip_index          = 1,

    .en_gpio[0]          = 85 + GPIO_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 84 + GPIO_OFFSET,
    .en_level[1]         = 1,
    .en_gpio_num         = 2,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0xe1,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x01,
    .en_logic_dis_val[0] = 0x00,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0xe2,
    .en_logic_mask[1]    = 0xf8,
    .en_logic_en_val[1]  = 0x02,
    .en_logic_dis_val[1] = 0x00,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0xe3,
    .en_logic_mask[2]    = 0x00,
    .en_logic_en_val[2]  = 0x1c,
    .en_logic_dis_val[2] = 0x1d,
    .en_logic_width[2]   = 0x1,

    .en_logic_num        = 3,
};

/* mac cpld b */
static firmware_upgrade_device_t firmware_upgrade_device_data2 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi             = 5 + GPIO_OFFSET,
        .tck             = 7 + GPIO_OFFSET,
        .tms             = 8 + GPIO_OFFSET,
        .tdo             = 6 + GPIO_OFFSET,
    },

    .chain               = 3,
    .chip_index          = 1,

    .en_gpio[0]          = 85 + GPIO_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 84 + GPIO_OFFSET,
    .en_level[1]         = 1,
    .en_gpio_num         = 2,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0xe1,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x01,
    .en_logic_dis_val[0] = 0x00,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0xe2,
    .en_logic_mask[1]    = 0xf8,
    .en_logic_en_val[1]  = 0x02,
    .en_logic_dis_val[1] = 0x00,
    .en_logic_width[1]   = 0x1,


    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0xe4,
    .en_logic_mask[2]    = 0x00,
    .en_logic_en_val[2]  = 0x1a,
    .en_logic_dis_val[2] = 0x1b,
    .en_logic_width[2]   = 0x1,

    .en_logic_num        = 3,
};

/* io cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data3 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi             = 5 + GPIO_OFFSET,
        .tck             = 7 + GPIO_OFFSET,
        .tms             = 8 + GPIO_OFFSET,
        .tdo             = 6 + GPIO_OFFSET,
    },

    .chain               = 4,
    .chip_index          = 1,

    .en_gpio[0]          = 85 + GPIO_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 84 + GPIO_OFFSET,
    .en_level[1]         = 1,
    .en_gpio_num         = 2,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0xe1,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x01,
    .en_logic_dis_val[0] = 0x00,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0xe2,
    .en_logic_mask[1]    = 0xf8,
    .en_logic_en_val[1]  = 0x03,
    .en_logic_dis_val[1] = 0x00,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0xe6,
    .en_logic_mask[2]    = 0x00,
    .en_logic_en_val[2]  = 0x18,
    .en_logic_dis_val[2] = 0x19,
    .en_logic_width[2]   = 0x1,

    .en_logic_num        = 3,
};

/* fan cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data4 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi             = 5 + GPIO_OFFSET,
        .tck             = 7 + GPIO_OFFSET,
        .tms             = 8 + GPIO_OFFSET,
        .tdo             = 6 + GPIO_OFFSET,
    },

    .chain               = 5,
    .chip_index          = 1,

    .en_gpio[0]          = 85 + GPIO_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 84 + GPIO_OFFSET,
    .en_level[1]         = 1,
    .en_gpio_num         = 2,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0xe1,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x01,
    .en_logic_dis_val[0] = 0x00,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0xe2,
    .en_logic_mask[1]    = 0xf8,
    .en_logic_en_val[1]  = 0x04,
    .en_logic_dis_val[1] = 0x00,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0xe8,
    .en_logic_mask[2]    = 0x00,
    .en_logic_en_val[2]  = 0x16,
    .en_logic_dis_val[2] = 0x17,
    .en_logic_width[2]   = 0x1,

    .en_logic_num        = 3,
};


/* cpu cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data5 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi             = 5 + GPIO_OFFSET,
        .tck             = 7 + GPIO_OFFSET,
        .tms             = 8 + GPIO_OFFSET,
        .tdo             = 6 + GPIO_OFFSET,
    },
    .chain               = 6,
    .chip_index          = 1,

    .en_gpio[0]          = 85 + GPIO_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 84 + GPIO_OFFSET,
    .en_level[1]         = 1,
    .en_gpio_num         = 2,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0xe1,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x01,
    .en_logic_dis_val[0] = 0x00,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0xe2,
    .en_logic_mask[1]    = 0xf8,
    .en_logic_en_val[1]  = 0x01,
    .en_logic_dis_val[1] = 0x00,
    .en_logic_width[1]   = 0x1,

    .en_logic_num        = 2,
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

/* io fpga */
static firmware_upgrade_device_t firmware_upgrade_device_data7 = {
    .type                = "SPI_LOGIC",
    .chain               = 3,
    .chip_index          = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga1",
        .ctrl_base    = 0xa00,
        .flash_base   = 0x2f0000,
        .test_base    = 0x7F0000,
        .test_size    = 0x10000,
    },
    .en_logic_num       = 0,
    .en_logic_num       = 0,
};

/* bcm53134 */
static firmware_upgrade_device_t firmware_upgrade_device_data8 = {
    .type                = "SYSFS",
    .chain               = 2,
    .chip_index          = 1,
    .upg_type.sysfs = {
        .sysfs_name     = "/sys/bus/spi/devices/spi0.0/eeprom",
    },
    .en_gpio[0]          = 85 + GPIO_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 84 + GPIO_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0xe1,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x01,
    .en_logic_dis_val[0] = 0x00,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0xe2,
    .en_logic_mask[1]    = 0xf8,
    .en_logic_en_val[1]  = 0x05,
    .en_logic_dis_val[1] = 0x00,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0xe9,
    .en_logic_mask[2]    = 0x00,
    .en_logic_en_val[2]  = 0x16,
    .en_logic_dis_val[2] = 0x17,
    .en_logic_width[2]   = 0x1,

    .en_gpio_num        = 2,
    .en_logic_num       = 3,
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
             firmware_upgrade_device_data->device_flag = -1;
             printk(KERN_ERR "firmware_upgrade_device id%d register failed!\n", i + 1);
         } else {
             firmware_upgrade_device_data->device_flag = 0;
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
         if (firmware_upgrade_device_data->device_flag == 0) {
             platform_device_unregister(&firmware_upgrade_device[i]);
         }
     }
 }

 module_init(firmware_upgrade_device_init);
 module_exit(firmware_upgrade_device_exit);
 MODULE_DESCRIPTION("FIRMWARE UPGRADE Devices");
 MODULE_LICENSE("GPL");
 MODULE_AUTHOR("support");
