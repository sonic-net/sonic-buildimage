/*
 * An wb_i2c_ocores_device driver for i2c ocores device function
 *
 * Copyright (C) 2024 Micas Networks Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <wb_i2c_ocores.h>

static int g_wb_i2c_ocores_device_debug = 0;
static int g_wb_i2c_ocores_device_error = 0;

module_param(g_wb_i2c_ocores_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_i2c_ocores_device_error, int, S_IRUGO | S_IWUSR);

#define WB_I2C_OCORE_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_i2c_ocores_device_debug) { \
        printk(KERN_INFO "[WB_I2C_OCORE_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_I2C_OCORE_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_i2c_ocores_device_error) { \
        printk(KERN_ERR "[WB_I2C_OCORE_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static i2c_ocores_device_t i2c_ocores_device_data0 = {
    .adap_nr = 1,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0800,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 0,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data1 = {
    .adap_nr = 2,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0820,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 1,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data2 = {
    .adap_nr = 3,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0840,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 2,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data3 = {
    .adap_nr = 4,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0860,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 3,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data4 = {
    .adap_nr = 5,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0880,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 4,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data5 = {
    .adap_nr = 6,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x08a0,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 5,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data6 = {
    .adap_nr = 7,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x08c0,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 6,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data7 = {
    .adap_nr = 8,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x08e0,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 7,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data8 = {
    .adap_nr = 9,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0900,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 8,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data9 = {
    .adap_nr = 10,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0920,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 9,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data10 = {
    .adap_nr = 11,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0940,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 10,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data11 = {
    .adap_nr = 12,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0960,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 11,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data12 = {
    .adap_nr = 13,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0980,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 12,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data13 = {
    .adap_nr = 14,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x09a0,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 13,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data14 = {
    .adap_nr = 15,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x09c0,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 14,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data15 = {
    .adap_nr = 16,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x09e0,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 15,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data16 = {
    .adap_nr = 17,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0a00,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 16,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data17 = {
    .adap_nr = 18,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0a20,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 17,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data18 = {
    .adap_nr = 19,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0a40,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 18,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data19 = {
    .adap_nr = 20,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0a60,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 19,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data20 = {
    .adap_nr = 21,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0a80,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 20,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data21 = {
    .adap_nr = 22,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0aa0,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 21,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data22 = {
    .adap_nr = 23,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0ac0,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 22,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static i2c_ocores_device_t i2c_ocores_device_data23 = {
    .adap_nr = 24,
    .big_endian = 0,
    .dev_name = "/dev/fpga0",
    .reg_access_mode = 3,
    .dev_base = 0x0ae0,
    .reg_shift = 2,
    .reg_io_width = 4,
    .ip_clock_khz = 125000,
    .bus_clock_khz = 100,
    .irq_type =1,
    .irq_offset = 23,
    .pci_domain = 0,
    .pci_bus = 3,
    .pci_slot = 0,
    .pci_fn = 0,
};

static void wb_i2c_ocores_device_release(struct device *dev)
{
    return;
}

static struct platform_device i2c_ocores_device[] = {
    {
        .name   = "wb-ocores-i2c",
        .id = 1,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data0,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 2,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data1,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 3,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data2,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 4,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data3,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 5,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data4,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 6,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data5,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 7,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data6,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 8,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data7,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 9,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data8,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 10,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data9,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 11,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data10,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 12,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data11,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 13,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data12,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 14,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data13,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 15,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data14,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 16,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data15,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 17,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data16,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 18,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data17,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 19,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data18,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 20,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data19,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 21,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data20,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 22,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data21,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 23,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data22,
            .release = wb_i2c_ocores_device_release,
        },
    },
    {
        .name   = "wb-ocores-i2c",
        .id = 24,
        .dev    = {
            .platform_data  = &i2c_ocores_device_data23,
            .release = wb_i2c_ocores_device_release,
        },
    },
};

static int __init wb_i2c_ocores_device_init(void)
{
    int i;
    int ret = 0;
    i2c_ocores_device_t *i2c_ocores_device_data;

    WB_I2C_OCORE_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(i2c_ocores_device); i++) {
        i2c_ocores_device_data = i2c_ocores_device[i].dev.platform_data;
        ret = platform_device_register(&i2c_ocores_device[i]);
        if (ret < 0) {
            i2c_ocores_device_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "wb-ocores-i2c.%d register failed!\n", i + 1);
        } else {
            i2c_ocores_device_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit wb_i2c_ocores_device_exit(void)
{
    int i;
    i2c_ocores_device_t *i2c_ocores_device_data;

    WB_I2C_OCORE_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(i2c_ocores_device) - 1; i >= 0; i--) {
        i2c_ocores_device_data = i2c_ocores_device[i].dev.platform_data;
        if (i2c_ocores_device_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&i2c_ocores_device[i]);
        }
    }
}

module_init(wb_i2c_ocores_device_init);
module_exit(wb_i2c_ocores_device_exit);
MODULE_DESCRIPTION("I2C OCORES Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
