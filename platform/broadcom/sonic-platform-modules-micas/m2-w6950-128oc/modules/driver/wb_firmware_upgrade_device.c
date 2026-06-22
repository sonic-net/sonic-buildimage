#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <firmware_upgrade.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <wb_bsp_kernel_debug.h>

#define GPIO_NUM      (256)                               /* GPIO number */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
#define GPIO_OFFSET   (GPIO_DYNAMIC_BASE)    /* Formula of gpio base number in Kernel */
#else
#define GPIO_OFFSET   (ARCH_NR_GPIOS - GPIO_NUM)    /* Formula of gpio base number in Kernel */
#endif

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

/* cpu cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data0 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi            = 5 + GPIO_OFFSET,
        .tck            = 7 + GPIO_OFFSET,
        .tms            = 8 + GPIO_OFFSET,
        .tdo            = 6 + GPIO_OFFSET,
    },
    .chain               = 1,
    .chip_index          = 1,
    .en_gpio[0]          = 84 + GPIO_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 85 + GPIO_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x54,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x1,
    .en_logic_dis_val[0] = 0x0,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x55,
    .en_logic_mask[1]    = 0xf8,
    .en_logic_en_val[1]  = 0x1,
    .en_logic_dis_val[1] = 0x0,
    .en_logic_width[1]   = 0x1,

    .en_gpio_num   = 2,
    .en_logic_num       = 2,
};

/* mgmt cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data1 = {
    .type               = "JTAG",
    .upg_type.jtag = {
        .tdi            = 5 + GPIO_OFFSET,
        .tck            = 7 + GPIO_OFFSET,
        .tms            = 8 + GPIO_OFFSET,
        .tdo            = 6 + GPIO_OFFSET,
    },
    .chain              = 2,
    .chip_index         = 1,
    .en_gpio[0]          = 91 + GPIO_OFFSET,
    .en_level[0]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x54,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x1,
    .en_logic_dis_val[0] = 0x0,
    .en_logic_width[0]   = 0x1,

    .en_gpio_num        = 1,
    .en_logic_num       = 0,
};

/* mac cpld a/b */
static firmware_upgrade_device_t firmware_upgrade_device_data2 = {
    .type               = "JTAG",
    .upg_type.jtag = {
        .tdi            = 5 + GPIO_OFFSET,
        .tck            = 7 + GPIO_OFFSET,
        .tms            = 8 + GPIO_OFFSET,
        .tdo            = 6 + GPIO_OFFSET,
    },
    .chain              = 3,
    .chip_index         = 1,
    .en_gpio[0]          = 84 + GPIO_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 85 + GPIO_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x54,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x1,
    .en_logic_dis_val[0] = 0x0,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x55,
    .en_logic_mask[1]    = 0xf8,
    .en_logic_en_val[1]  = 0x2,
    .en_logic_dis_val[1] = 0x0,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0x44,
    .en_logic_mask[2]    = 0,
    .en_logic_en_val[2]  = 0xba,
    .en_logic_dis_val[2] = 0xbb,
    .en_logic_width[2]   = 0x1,

    .en_logic_dev[3]     = "/dev/cpld1",
    .en_logic_addr[3]    = 0x4d,
    .en_logic_mask[3]    = 0,
    .en_logic_en_val[3]  = 0xb2,
    .en_logic_dis_val[3] = 0xb3,
    .en_logic_width[3]   = 0x1,

    .en_logic_dev[4]     = "/dev/cpld4",
    .en_logic_addr[4]    = 0x53,
    .en_logic_mask[4]    = 0,
    .en_logic_en_val[4]  = 0xad,
    .en_logic_dis_val[4] = 0xac,
    .en_logic_width[4]   = 0x1,

    .en_logic_dev[5]     = "/dev/cpld4",
    .en_logic_addr[5]    = 0x54,
    .en_logic_mask[5]    = 0,
    .en_logic_en_val[5]  = 0x1,
    .en_logic_dis_val[5] = 0x0,
    .en_logic_width[5]   = 0x1,

    .en_gpio_num        = 2,
    .en_logic_num       = 6,
};

/* mac cpld c */
static firmware_upgrade_device_t firmware_upgrade_device_data3 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi            = 5 + GPIO_OFFSET,
        .tck            = 7 + GPIO_OFFSET,
        .tms            = 8 + GPIO_OFFSET,
        .tdo            = 6 + GPIO_OFFSET,
    },
    .chain               = 4,
    .chip_index          = 1,
    .en_gpio[0]          = 84 + GPIO_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 85 + GPIO_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x54,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x1,
    .en_logic_dis_val[0] = 0x0,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x55,
    .en_logic_mask[1]    = 0xf8,
    .en_logic_en_val[1]  = 0x2,
    .en_logic_dis_val[1] = 0x0,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0x43,
    .en_logic_mask[2]    = 0,
    .en_logic_en_val[2]  = 0xbc,
    .en_logic_dis_val[2] = 0xbd,
    .en_logic_width[2]   = 0x1,

    .en_gpio_num        = 2,
    .en_logic_num       = 3,
};

/* uport cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data4 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi            = 5 + GPIO_OFFSET,
        .tck            = 7 + GPIO_OFFSET,
        .tms            = 8 + GPIO_OFFSET,
        .tdo            = 6 + GPIO_OFFSET,
    },
    .chain               = 5,
    .chip_index          = 1,
    .en_gpio[0]          = 84 + GPIO_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 85 + GPIO_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x54,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x1,
    .en_logic_dis_val[0] = 0x0,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x55,
    .en_logic_mask[1]    = 0xf8,
    .en_logic_en_val[1]  = 0x2,
    .en_logic_dis_val[1] = 0x0,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0x47,
    .en_logic_mask[2]    = 0,
    .en_logic_en_val[2]  = 0xb8,
    .en_logic_dis_val[2] = 0xb9,
    .en_logic_width[2]   = 0x1,

    .en_logic_dev[3]     = "/dev/cpld1",
    .en_logic_addr[3]    = 0x4d,
    .en_logic_mask[3]    = 0,
    .en_logic_en_val[3]  = 0xb2,
    .en_logic_dis_val[3] = 0xb3,
    .en_logic_width[3]   = 0x1,

    .en_logic_dev[4]     = "/dev/cpld4",
    .en_logic_addr[4]    = 0x53,
    .en_logic_mask[4]    = 0,
    .en_logic_en_val[4]  = 0xad,
    .en_logic_dis_val[4] = 0xac,
    .en_logic_width[4]   = 0x1,

    .en_logic_dev[5]     = "/dev/cpld4",
    .en_logic_addr[5]    = 0x54,
    .en_logic_mask[5]    = 0,
    .en_logic_en_val[5]  = 0x6,
    .en_logic_dis_val[5] = 0x0,
    .en_logic_width[5]   = 0x1,

    .en_gpio_num        = 2,
    .en_logic_num       = 6,
};

/* dport cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data5 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi            = 5 + GPIO_OFFSET,
        .tck            = 7 + GPIO_OFFSET,
        .tms            = 8 + GPIO_OFFSET,
        .tdo            = 6 + GPIO_OFFSET,
    },
    .chain               = 6,
    .chip_index          = 1,
    .en_gpio[0]          = 84 + GPIO_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 85 + GPIO_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x54,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x1,
    .en_logic_dis_val[0] = 0x0,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x55,
    .en_logic_mask[1]    = 0xf8,
    .en_logic_en_val[1]  = 0x2,
    .en_logic_dis_val[1] = 0x0,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0x49,
    .en_logic_mask[2]    = 0,
    .en_logic_en_val[2]  = 0xb6,
    .en_logic_dis_val[2] = 0xb7,
    .en_logic_width[2]   = 0x1,

    .en_logic_dev[3]     = "/dev/cpld1",
    .en_logic_addr[3]    = 0x4d,
    .en_logic_mask[3]    = 0,
    .en_logic_en_val[3]  = 0xb2,
    .en_logic_dis_val[3] = 0xb3,
    .en_logic_width[3]   = 0x1,

    .en_logic_dev[4]     = "/dev/cpld4",
    .en_logic_addr[4]    = 0x53,
    .en_logic_mask[4]    = 0,
    .en_logic_en_val[4]  = 0xad,
    .en_logic_dis_val[4] = 0xac,
    .en_logic_width[4]   = 0x1,

    .en_logic_dev[5]     = "/dev/cpld4",
    .en_logic_addr[5]    = 0x54,
    .en_logic_mask[5]    = 0,
    .en_logic_en_val[5]  = 0x8,
    .en_logic_dis_val[5] = 0x0,
    .en_logic_width[5]   = 0x1,

    .en_gpio_num        = 2,
    .en_logic_num       = 6,
};

/* ufan cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data6 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi            = 5 + GPIO_OFFSET,
        .tck            = 7 + GPIO_OFFSET,
        .tms            = 8 + GPIO_OFFSET,
        .tdo            = 6 + GPIO_OFFSET,
    },
    .chain               = 7,
    .chip_index          = 1,
    .en_gpio[0]          = 84 + GPIO_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 85 + GPIO_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x54,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x1,
    .en_logic_dis_val[0] = 0x0,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x55,
    .en_logic_mask[1]    = 0xf8,
    .en_logic_en_val[1]  = 0x2,
    .en_logic_dis_val[1] = 0x0,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0x4a,
    .en_logic_mask[2]    = 0,
    .en_logic_en_val[2]  = 0xb4,
    .en_logic_dis_val[2] = 0xb5,
    .en_logic_width[2]   = 0x1,

    .en_logic_dev[3]     = "/dev/cpld1",
    .en_logic_addr[3]    = 0x4d,
    .en_logic_mask[3]    = 0,
    .en_logic_en_val[3]  = 0xb2,
    .en_logic_dis_val[3] = 0xb3,
    .en_logic_width[3]   = 0x1,

    .en_logic_dev[4]     = "/dev/cpld4",
    .en_logic_addr[4]    = 0x53,
    .en_logic_mask[4]    = 0,
    .en_logic_en_val[4]  = 0xad,
    .en_logic_dis_val[4] = 0xac,
    .en_logic_width[4]   = 0x1,

    .en_logic_dev[5]     = "/dev/cpld4",
    .en_logic_addr[5]    = 0x54,
    .en_logic_mask[5]    = 0,
    .en_logic_en_val[5]  = 0x4,
    .en_logic_dis_val[5] = 0x0,
    .en_logic_width[5]   = 0x1,

    .en_gpio_num        = 2,
    .en_logic_num       = 6,
};

/* dfan cpld */
static firmware_upgrade_device_t firmware_upgrade_device_data7 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi            = 5 + GPIO_OFFSET,
        .tck            = 7 + GPIO_OFFSET,
        .tms            = 8 + GPIO_OFFSET,
        .tdo            = 6 + GPIO_OFFSET,
    },
    .chain               = 8,
    .chip_index          = 1,
    .en_gpio[0]          = 84 + GPIO_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 85 + GPIO_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x54,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x1,
    .en_logic_dis_val[0] = 0x0,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x55,
    .en_logic_mask[1]    = 0xf8,
    .en_logic_en_val[1]  = 0x2,
    .en_logic_dis_val[1] = 0x0,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0x4b,
    .en_logic_mask[2]    = 0,
    .en_logic_en_val[2]  = 0xb4,
    .en_logic_dis_val[2] = 0xb5,
    .en_logic_width[2]   = 0x1,

    .en_logic_dev[3]     = "/dev/cpld1",
    .en_logic_addr[3]    = 0x4d,
    .en_logic_mask[3]    = 0,
    .en_logic_en_val[3]  = 0xb2,
    .en_logic_dis_val[3] = 0xb3,
    .en_logic_width[3]   = 0x1,

    .en_logic_dev[4]     = "/dev/cpld4",
    .en_logic_addr[4]    = 0x53,
    .en_logic_mask[4]    = 0,
    .en_logic_en_val[4]  = 0xad,
    .en_logic_dis_val[4] = 0xac,
    .en_logic_width[4]   = 0x1,

    .en_logic_dev[5]     = "/dev/cpld4",
    .en_logic_addr[5]    = 0x54,
    .en_logic_mask[5]    = 0,
    .en_logic_en_val[5]  = 0x5,
    .en_logic_dis_val[5] = 0x0,
    .en_logic_width[5]   = 0x1,

    .en_gpio_num        = 2,
    .en_logic_num       = 6,
};

/* mac cpldd */
static firmware_upgrade_device_t firmware_upgrade_device_data8 = {
    .type                = "JTAG",
    .upg_type.jtag = {
        .tdi            = 5 + GPIO_OFFSET,
        .tck            = 7 + GPIO_OFFSET,
        .tms            = 8 + GPIO_OFFSET,
        .tdo            = 6 + GPIO_OFFSET,
    },
    .chain               = 9,
    .chip_index          = 1,
    .en_gpio[0]          = 84 + GPIO_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 85 + GPIO_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x54,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x1,
    .en_logic_dis_val[0] = 0x0,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x55,
    .en_logic_mask[1]    = 0xf8,
    .en_logic_en_val[1]  = 0x2,
    .en_logic_dis_val[1] = 0x0,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0x4d,
    .en_logic_mask[2]    = 0,
    .en_logic_en_val[2]  = 0xb2,
    .en_logic_dis_val[2] = 0xb3,
    .en_logic_width[2]   = 0x1,

    .en_logic_dev[3]     = "/dev/cpld4",
    .en_logic_addr[3]    = 0x53,
    .en_logic_mask[3]    = 0,
    .en_logic_en_val[3]  = 0xad,
    .en_logic_dis_val[3] = 0xac,
    .en_logic_width[3]   = 0x1,

    .en_logic_dev[4]     = "/dev/cpld4",
    .en_logic_addr[4]    = 0x54,
    .en_logic_mask[4]    = 0,
    .en_logic_en_val[4]  = 0xa,
    .en_logic_dis_val[4] = 0x0,
    .en_logic_width[4]   = 0x1,

    .en_gpio_num        = 2,
    .en_logic_num       = 5,
};

/* mac fpga */
static firmware_upgrade_device_t firmware_upgrade_device_data9 = {
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

/* mac fpga shaopian */
static firmware_upgrade_device_t firmware_upgrade_device_data10 = {
    .type                = "SPI_LOGIC",
    .chain               = 2,
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

/* uport fpga */
static firmware_upgrade_device_t firmware_upgrade_device_data11 = {
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
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* uport fpga shaopian */
static firmware_upgrade_device_t firmware_upgrade_device_data12 = {
    .type                = "SPI_LOGIC",
    .chain               = 4,
    .chip_index          = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga1",
        .ctrl_base    = 0xa00,
        .flash_base   = 0x0,
        .test_base    = 0x7F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* dport fpga */
static firmware_upgrade_device_t firmware_upgrade_device_data13 = {
    .type                = "SPI_LOGIC",
    .chain               = 5,
    .chip_index          = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga2",
        .ctrl_base    = 0xa00,
        .flash_base   = 0x2f0000,
        .test_base    = 0x7F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* dport fpga shaopian */
static firmware_upgrade_device_t firmware_upgrade_device_data14 = {
    .type                = "SPI_LOGIC",
    .chain               = 6,
    .chip_index          = 1,
    .upg_type.sysfs = {
        .dev_name     = "/dev/fpga2",
        .ctrl_base    = 0xa00,
        .flash_base   = 0x0,
        .test_base    = 0x7F0000,
        .test_size    = 0x10000,
    },
    .en_gpio_num        = 0,
    .en_logic_num       = 0,
};

/* bcm53134 */
static firmware_upgrade_device_t firmware_upgrade_device_data15 = {
    .type                = "SYSFS",
    .chain               = 8,
    .chip_index          = 1,
    .upg_type.sysfs = {
        .sysfs_name     = "/sys/bus/spi/devices/spi0.0/eeprom",
    },
    .en_gpio[0]          = 85 + GPIO_OFFSET,
    .en_level[0]         = 1,
    .en_gpio[1]          = 84 + GPIO_OFFSET,
    .en_level[1]         = 1,

    .en_logic_dev[0]     = "/dev/cpld1",
    .en_logic_addr[0]    = 0x54,
    .en_logic_mask[0]    = 0xfc,
    .en_logic_en_val[0]  = 0x1,
    .en_logic_dis_val[0] = 0x0,
    .en_logic_width[0]   = 0x1,

    .en_logic_dev[1]     = "/dev/cpld1",
    .en_logic_addr[1]    = 0x55,
    .en_logic_mask[1]    = 0xf8,
    .en_logic_en_val[1]  = 0x5,
    .en_logic_dis_val[1] = 0x0,
    .en_logic_width[1]   = 0x1,

    .en_logic_dev[2]     = "/dev/cpld1",
    .en_logic_addr[2]    = 0x4c,
    .en_logic_mask[2]    = 0x00,
    .en_logic_en_val[2]  = 0xb2,
    .en_logic_dis_val[2] = 0xb3,
    .en_logic_width[2]   = 0x1,

    .en_gpio_num        = 2,
    .en_logic_num       = 3,
};

/* MAC-PCIE */
static firmware_upgrade_device_t firmware_upgrade_device_data16 = {
    .type                = "MTD_DEV",
    .chain               = 9,
    .chip_index          = 1,
    .upg_type.sysfs = {
        .mtd_name     = "spi0.0",
        .flash_base   = 0,
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
        .name   = "firmware_cpld_ispvme",
        .id = 7,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data6,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_cpld_ispvme",
        .id = 8,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data7,
            .release = firmware_device_release,
        },
    },
    {
        .name   = "firmware_cpld_ispvme",
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
    {
        .name   = "firmware_sysfs",
        .id = 17,
        .dev    = {
            .platform_data  = &firmware_upgrade_device_data16,
            .release = firmware_device_release,
        },
    },
};

static int __init firmware_upgrade_device_init(void)
{
    int i;
    int ret = 0;
    firmware_upgrade_device_t *firmware_upgrade_device_data;

    DEBUG_VERBOSE("enter!\n");
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

    DEBUG_VERBOSE("enter!\n");
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