#include <linux/module.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <wb_i2c_mux_pca954x.h>

static int g_wb_i2c_mux_pca954x_device_debug = 0;
static int g_wb_i2c_mux_pca954x_device_error = 0;

module_param(g_wb_i2c_mux_pca954x_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_i2c_mux_pca954x_device_error, int, S_IRUGO | S_IWUSR);

#define WB_I2C_MUX_PCA954X_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_i2c_mux_pca954x_device_debug) { \
        printk(KERN_INFO "[WB_I2C_MUX_PCA954X_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_I2C_MUX_PCA954X_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_i2c_mux_pca954x_device_error) { \
        printk(KERN_ERR "[WB_I2C_MUX_PCA954X_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data0 = {
    .i2c_bus                        = 2,
    .i2c_addr                       = 0x76,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 1,
    .pca9548_base_nr                = 7,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0x936,
        .io_attr.mask               = 0x10,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x10,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data1 = {
    .i2c_bus                        = 7,
    .i2c_addr                       = 0x77,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 1,
    .pca9548_base_nr                = 15,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0x917,
        .io_attr.mask               = 0x08,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x08,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data2 = {
    .i2c_bus                        = 8,
    .i2c_addr                       = 0x77,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 23,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0x917,
        .io_attr.mask               = 0x10,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x10,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data3 = {
    .i2c_bus                        = 9,
    .i2c_addr                       = 0x77,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 1,
    .pca9548_base_nr                = 31,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0x917,
        .io_attr.mask               = 0x01,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x01,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data4 = {
    .i2c_bus                        = 11,
    .i2c_addr                       = 0x77,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 1,
    .pca9548_base_nr                = 39,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0x917,
        .io_attr.mask               = 0x04,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x04,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data5 = {
    .i2c_bus                        = 12,
    .i2c_addr                       = 0x77,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 1,
    .pca9548_base_nr                = 47,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0x917,
        .io_attr.mask               = 0x02,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x02,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data6 = {
    .i2c_bus                        = 13,
    .i2c_addr                       = 0x77,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 1,
    .pca9548_base_nr                = 55,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb10,
        .io_attr.mask               = 0x80,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x80,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data7 = {
    .i2c_bus                        = 14,
    .i2c_addr                       = 0x77,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 1,
    .pca9548_base_nr                = 63,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb10,
        .io_attr.mask               = 0x20,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x20,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data8 = {
    .i2c_bus                        = 3,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 71,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x01,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x01,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data9 = {
    .i2c_bus                        = 3,
    .i2c_addr                       = 0x71,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 79,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x01,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x01,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data10 = {
    .i2c_bus                        = 3,
    .i2c_addr                       = 0x72,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 87,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x01,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x01,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data11 = {
    .i2c_bus                        = 3,
    .i2c_addr                       = 0x73,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 95,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x01,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x01,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data12 = {
    .i2c_bus                        = 4,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 103,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x02,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x02,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data13 = {
    .i2c_bus                        = 4,
    .i2c_addr                       = 0x71,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 111,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x02,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x02,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data14 = {
    .i2c_bus                        = 4,
    .i2c_addr                       = 0x72,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 119,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x02,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x02,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data15 = {
    .i2c_bus                        = 4,
    .i2c_addr                       = 0x73,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 127,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x02,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x02,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data16 = {
    .i2c_bus                        = 5,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 135,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x04,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x04,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data17 = {
    .i2c_bus                        = 5,
    .i2c_addr                       = 0x71,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 143,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x04,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x04,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data18 = {
    .i2c_bus                        = 5,
    .i2c_addr                       = 0x72,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 151,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x04,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x04,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data19 = {
    .i2c_bus                        = 5,
    .i2c_addr                       = 0x73,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 159,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x04,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x04,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data20 = {
    .i2c_bus                        = 6,
    .i2c_addr                       = 0x70,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 167,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x08,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x08,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data21 = {
    .i2c_bus                        = 6,
    .i2c_addr                       = 0x71,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 175,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x08,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x08,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data22 = {
    .i2c_bus                        = 6,
    .i2c_addr                       = 0x72,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 183,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x08,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x08,
    },
};

static i2c_mux_pca954x_device_t i2c_mux_pca954x_device_data23 = {
    .i2c_bus                        = 6,
    .i2c_addr                       = 0x73,
    .probe_disable                  = 1,
    .select_chan_check              = 0,
    .close_chan_force_reset         = 0,
    .pca9548_base_nr                = 191,
    .pca9548_reset_type             = PCA9548_RESET_IO,
    .rst_delay_b                    = 0,
    .rst_delay                      = 1000,
    .rst_delay_a                    = 1000,
    .attr = {
        .io_attr.io_addr            = 0xb17,
        .io_attr.mask               = 0x08,
        .io_attr.reset_on           = 0,
        .io_attr.reset_off          = 0x08,
    },
};

struct i2c_board_info i2c_mux_pca954x_device_info[] = {
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data0,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data1,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data2,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data3,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data4,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data5,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data6,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data7,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data8,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data9,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data10,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data11,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data12,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data13,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data14,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data15,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data16,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data17,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data18,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data19,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data20,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data21,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data22,
    },
    {
        .type = "wb_pca9548",
        .platform_data = &i2c_mux_pca954x_device_data23,
    },
};

static int __init wb_i2c_mux_pca954x_device_init(void)
{
    int i;
    struct i2c_adapter *adap;
    struct i2c_client *client;
    i2c_mux_pca954x_device_t *i2c_mux_pca954x_device_data;

    WB_I2C_MUX_PCA954X_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(i2c_mux_pca954x_device_info); i++) {
        i2c_mux_pca954x_device_data = i2c_mux_pca954x_device_info[i].platform_data;
        i2c_mux_pca954x_device_info[i].addr = i2c_mux_pca954x_device_data->i2c_addr;
        adap = i2c_get_adapter(i2c_mux_pca954x_device_data->i2c_bus);
        if (adap == NULL) {
            i2c_mux_pca954x_device_data->client = NULL;
            printk(KERN_ERR "get i2c bus %d adapter fail.\n", i2c_mux_pca954x_device_data->i2c_bus);
            continue;
        }
        client = i2c_new_client_device(adap, &i2c_mux_pca954x_device_info[i]);
        if (!client) {
            i2c_mux_pca954x_device_data->client = NULL;
            printk(KERN_ERR "Failed to register pca954x device %d at bus %d!\n",
                i2c_mux_pca954x_device_data->i2c_addr, i2c_mux_pca954x_device_data->i2c_bus);
        } else {
            i2c_mux_pca954x_device_data->client = client;
        }
        i2c_put_adapter(adap);
    }
    return 0;
}

static void __exit wb_i2c_mux_pca954x_device_exit(void)
{
    int i;
    i2c_mux_pca954x_device_t *i2c_mux_pca954x_device_data;

    WB_I2C_MUX_PCA954X_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(i2c_mux_pca954x_device_info) - 1; i >= 0; i--) {
        i2c_mux_pca954x_device_data = i2c_mux_pca954x_device_info[i].platform_data;
        if (i2c_mux_pca954x_device_data->client) {
            i2c_unregister_device(i2c_mux_pca954x_device_data->client);
            i2c_mux_pca954x_device_data->client = NULL;
        }
    }
}

module_init(wb_i2c_mux_pca954x_device_init);
module_exit(wb_i2c_mux_pca954x_device_exit);
MODULE_DESCRIPTION("I2C MUX PCA954X Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
