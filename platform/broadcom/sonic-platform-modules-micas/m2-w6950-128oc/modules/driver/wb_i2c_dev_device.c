#include <linux/module.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <wb_i2c_dev.h>
#include <wb_bsp_kernel_debug.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

/* CPU CPLD */
static i2c_dev_device_t i2c_dev_device_data0 = {
    .i2c_bus = 0,
    .i2c_addr = 0x0d,
    .i2c_name = "cpld0",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

/* MGMT CPLD */
static i2c_dev_device_t i2c_dev_device_data1 = {
    .i2c_bus = 52,
    .i2c_addr = 0x3d,
    .i2c_name = "cpld1",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

/* MAC CPLD A */
static i2c_dev_device_t i2c_dev_device_data2 = {
    .i2c_bus = 139,
    .i2c_addr = 0x1d,
    .i2c_name = "cpld2",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

/* MAC CPLD B */
static i2c_dev_device_t i2c_dev_device_data3 = {
    .i2c_bus = 140,
    .i2c_addr = 0x2D,
    .i2c_name = "cpld3",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

/* MAC CPLD C */
static i2c_dev_device_t i2c_dev_device_data4 = {
    .i2c_bus = 141,
    .i2c_addr = 0x3D,
    .i2c_name = "cpld4",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

/* UPORT CPLD */
static i2c_dev_device_t i2c_dev_device_data5 = {
    .i2c_bus = 147,
    .i2c_addr = 0x3d,
    .i2c_name = "cpld5",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

/* DPORT CPLD */
static i2c_dev_device_t i2c_dev_device_data6 = {
    .i2c_bus = 155,
    .i2c_addr = 0x3d,
    .i2c_name = "cpld6",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

/* UFAN CPLD */
static i2c_dev_device_t i2c_dev_device_data7 = {
    .i2c_bus = 75,
    .i2c_addr = 0x0d,
    .i2c_name = "cpld7",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

/* DFAN CPLD */
static i2c_dev_device_t i2c_dev_device_data8 = {
    .i2c_bus = 83,
    .i2c_addr = 0x0d,
    .i2c_name = "cpld8",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

/* MAC CPLD D */
static i2c_dev_device_t i2c_dev_device_data9 = {
    .i2c_bus = 142,
    .i2c_addr = 0x4D,
    .i2c_name = "cpld9",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};
#if 0
/* MGMT CPLD */
static i2c_dev_device_t i2c_dev_device_data10 = {
    .i2c_bus = 2,
    .i2c_addr = 0x3a,
    .i2c_name = "cpld10",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};
#endif
/* MAC CPLD A */
static i2c_dev_device_t i2c_dev_device_data11 = {
    .i2c_bus = 12,
    .i2c_addr = 0x1c,
    .i2c_name = "cpld11",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

/* MAC CPLD B */
static i2c_dev_device_t i2c_dev_device_data12 = {
    .i2c_bus = 13,
    .i2c_addr = 0x2c,
    .i2c_name = "cpld12",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

/* MAC CPLD C */
static i2c_dev_device_t i2c_dev_device_data13 = {
    .i2c_bus = 11,
    .i2c_addr = 0x3c,
    .i2c_name = "cpld13",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

/* MAC CPLD D */
static i2c_dev_device_t i2c_dev_device_data14 = {
    .i2c_bus = 1,
    .i2c_addr = 0x4a,
    .i2c_name = "cpld14",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

/* UPORT CPLD */
static i2c_dev_device_t i2c_dev_device_data15 = {
    .i2c_bus = 15,
    .i2c_addr = 0x3c,
    .i2c_name = "cpld15",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

/* DPORT CPLD */
static i2c_dev_device_t i2c_dev_device_data16 = {
    .i2c_bus = 17,
    .i2c_addr = 0x3c,
    .i2c_name = "cpld16",
    .data_bus_width = 1,
    .addr_bus_width = 1,
    .per_rd_len = 256,
    .per_wr_len = 256,
    .i2c_len = 256,
};

struct i2c_board_info i2c_dev_device_info[] = {
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data0,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data1,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data2,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data3,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data4,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data5,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data6,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data7,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data8,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data9,
    },
#if 0
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data10,
    },
#endif
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data11,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data12,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data13,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data14,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data15,
    },
    {
        .type = "wb-i2c-dev",
        .platform_data = &i2c_dev_device_data16,
    },
};

static int __init wb_i2c_dev_device_init(void)
{
    int i;
    struct i2c_adapter *adap;
    struct i2c_client *client;
    i2c_dev_device_t *i2c_dev_device_data;

    DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(i2c_dev_device_info); i++) {
        i2c_dev_device_data = i2c_dev_device_info[i].platform_data;
        i2c_dev_device_info[i].addr = i2c_dev_device_data->i2c_addr;
        adap = i2c_get_adapter(i2c_dev_device_data->i2c_bus);
        if (adap == NULL) {
            i2c_dev_device_data->client = NULL;
            printk(KERN_ERR "get i2c bus %d adapter fail.\n", i2c_dev_device_data->i2c_bus);
            continue;
        }
        client = i2c_new_client_device(adap, &i2c_dev_device_info[i]);
        if (IS_ERR(client)) {
            i2c_dev_device_data->client = NULL;
            printk(KERN_ERR "Failed to register i2c dev device %d at bus %d!\n",
                i2c_dev_device_data->i2c_addr, i2c_dev_device_data->i2c_bus);
        } else {
            i2c_dev_device_data->client = client;
        }
        i2c_put_adapter(adap);
    }
    return 0;
}

static void __exit wb_i2c_dev_device_exit(void)
{
    int i;
    i2c_dev_device_t *i2c_dev_device_data;

    DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(i2c_dev_device_info) - 1; i >= 0; i--) {
        i2c_dev_device_data = i2c_dev_device_info[i].platform_data;
        if (i2c_dev_device_data->client) {
            i2c_unregister_device(i2c_dev_device_data->client);
            i2c_dev_device_data->client = NULL;
        }
    }
}

module_init(wb_i2c_dev_device_init);
module_exit(wb_i2c_dev_device_exit);
MODULE_DESCRIPTION("I2C DEV Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
