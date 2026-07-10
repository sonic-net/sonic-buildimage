#include <linux/module.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <fpga_i2c.h>

static int g_rg_fpga_pca954x_device_debug = 0;
static int g_rg_fpga_pca954x_device_error = 0;

module_param(g_rg_fpga_pca954x_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_rg_fpga_pca954x_device_error, int, S_IRUGO | S_IWUSR);

#define RG_FPGA_PCA954X_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_rg_fpga_pca954x_device_debug) { \
        printk(KERN_INFO "[RG_FPGA_PCA954X_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define RG_FPGA_PCA954X_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_rg_fpga_pca954x_device_error) { \
        printk(KERN_ERR "[RG_FPGA_PCA954X_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static fpga_pca954x_device_t fpga_pca954x_device_data0 = {
    .i2c_bus = 3,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 22,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data1 = {
    .i2c_bus = 4,
    .i2c_addr = 0x71,
    .pca9548_base_nr = 30,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data2 = {
    .i2c_bus = 5,
    .i2c_addr = 0x77,
    .pca9548_base_nr = 38,
    .fpga_9548_flag = 2,
    .fpga_9548_reset_flag = 1,
};

static fpga_pca954x_device_t fpga_pca954x_device_data3 = {
    .i2c_bus = 6,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 46,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data4 = {
    .i2c_bus = 7,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 48,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data5 = {
    .i2c_bus = 8,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 50,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data6 = {
    .i2c_bus = 9,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 52,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data7 = {
    .i2c_bus = 10,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 54,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data8 = {
    .i2c_bus = 11,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 56,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data9 = {
    .i2c_bus = 12,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 58,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data10 = {
    .i2c_bus = 13,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 60,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data11 = {
    .i2c_bus = 14,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 62,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data12 = {
    .i2c_bus = 15,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 64,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data13 = {
    .i2c_bus = 16,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 66,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data14 = {
    .i2c_bus = 17,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 68,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data15 = {
    .i2c_bus = 18,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 70,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data16 = {
    .i2c_bus = 19,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 72,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data17 = {
    .i2c_bus = 20,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 74,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

static fpga_pca954x_device_t fpga_pca954x_device_data18 = {
    .i2c_bus = 21,
    .i2c_addr = 0x70,
    .pca9548_base_nr = 76,
    .fpga_9548_flag = 1,
    .fpga_9548_reset_flag = 0,
};

struct i2c_board_info fpga_pca954x_device_info[] = {
    {
        .type = "rg_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data0,
    },
    {
        .type = "rg_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data1,
    },
    {
        .type = "rg_fpga_pca9548",
        .platform_data = &fpga_pca954x_device_data2,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data3,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data4,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data5,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data6,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data7,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data8,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data9,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data10,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data11,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data12,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data13,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data14,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data15,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data16,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data17,
    },
    {
        .type = "rg_fpga_pca9542",
        .platform_data = &fpga_pca954x_device_data18,
    },
};

static int __init rg_fpga_pca954x_device_init(void)
{
    int i;
    struct i2c_adapter *adap;
    struct i2c_client *client;
    fpga_pca954x_device_t *fpga_pca954x_device_data;

    RG_FPGA_PCA954X_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(fpga_pca954x_device_info); i++) {
        fpga_pca954x_device_data = fpga_pca954x_device_info[i].platform_data;
        fpga_pca954x_device_info[i].addr = fpga_pca954x_device_data->i2c_addr;
        adap = i2c_get_adapter(fpga_pca954x_device_data->i2c_bus);
        if (adap == NULL) {
            fpga_pca954x_device_data->client = NULL;
            printk(KERN_ERR "get i2c bus %d adapter fail.\n", fpga_pca954x_device_data->i2c_bus);
            continue;
        }
        client = i2c_new_client_device(adap, &fpga_pca954x_device_info[i]);
        if (!client) {
            fpga_pca954x_device_data->client = NULL;
            printk(KERN_ERR "Failed to register fpga pca954x device %d at bus %d!\n",
                fpga_pca954x_device_data->i2c_addr, fpga_pca954x_device_data->i2c_bus);
        } else {
            fpga_pca954x_device_data->client = client;
        }
        i2c_put_adapter(adap);
    }
    return 0;
}

static void __exit rg_fpga_pca954x_device_exit(void)
{
    int i;
    fpga_pca954x_device_t *fpga_pca954x_device_data;

    RG_FPGA_PCA954X_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(fpga_pca954x_device_info) - 1; i >= 0; i--) {
        fpga_pca954x_device_data = fpga_pca954x_device_info[i].platform_data;
        if (fpga_pca954x_device_data->client) {
            i2c_unregister_device(fpga_pca954x_device_data->client);
            fpga_pca954x_device_data->client = NULL;
        }
    }
}

module_init(rg_fpga_pca954x_device_init);
module_exit(rg_fpga_pca954x_device_exit);
MODULE_DESCRIPTION("RG FPGA PCA954X Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("sonic_rd@ruijie.com.cn");
