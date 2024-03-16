#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/spi/spi.h>

#include <wb_spi_dev.h>

#define PROXY_NAME "wb-spi-dev-device"

#define SPI_DEVICE_MAX_NUM (64)

static int g_wb_spi_dev_device_debug = 0;
static int g_wb_spi_dev_device_error = 0;

module_param(g_wb_spi_dev_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_spi_dev_device_error, int, S_IRUGO | S_IWUSR);

#define WB_SPI_DEV_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_spi_dev_device_debug) { \
        printk(KERN_INFO "[WB_SPI_DEV_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_SPI_DEV_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_spi_dev_device_error) { \
        printk(KERN_ERR "[WB_SPI_DEV_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static spi_dev_device_t spi_dev_device_data0 = {
    .spi_dev_name = "fpga1",
    .data_bus_width = 4,
    .addr_bus_width = 2,
    .per_rd_len = 4,
    .per_wr_len = 4,
    .spi_len = 0xe000,
};

static struct spi_device *g_spi_device[SPI_DEVICE_MAX_NUM];

struct spi_board_info spi_dev_device_info[] = {
    {
        .modalias           = "wb-spi-dev",
        .max_speed_hz       = 6250000,
        .bus_num            = 1,
        .chip_select        = 0,
        .mode               = SPI_MODE_3,
        .platform_data      = &spi_dev_device_data0,
    },
};

static struct class *class_spi_master = NULL;

static int spi_controller_match(struct device *dev, const void *data)
{
    struct spi_controller *ctlr;
    const u16 *bus_num = data;

    ctlr = container_of(dev, struct spi_controller, dev);
    return ctlr->bus_num == *bus_num;
}

static int wb_spi_dev_device_probe(struct platform_device *pdev)
{
    int i;
    struct spi_master *master;
    struct spi_controller *controller;
    struct device *dev;
    struct spi_device *spi;
    int spi_dev_num;

    WB_SPI_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");

    spi_dev_num = ARRAY_SIZE(spi_dev_device_info);
    if (spi_dev_num > SPI_DEVICE_MAX_NUM) {
        dev_err(&pdev->dev, "spi_dev_num[%d] is bigger than max_num[%d].\n",
            spi_dev_num, SPI_DEVICE_MAX_NUM);
        return -EINVAL;
    }

    controller = devm_spi_alloc_master(&pdev->dev, sizeof(struct spi_board_info));
    if (!controller) {
        WB_SPI_DEV_DEVICE_DEBUG_ERROR("spi_alloc_master failed.\n");
        return -ENOMEM;
    }

    class_spi_master = controller->dev.class;
    if (!class_spi_master) {
        WB_SPI_DEV_DEVICE_DEBUG_ERROR("get class_spi_master failed.\n");
        return -ENOMEM;
    }

    for (i = 0; i < ARRAY_SIZE(spi_dev_device_info); i++) {
        dev = class_find_device(class_spi_master, NULL, &spi_dev_device_info[i].bus_num,
                spi_controller_match);
        if (dev) {
            master = container_of(dev, struct spi_master, dev);
        } else {
            dev_err(&pdev->dev, "class_find_device bus_num %u dev failed.\n",
                spi_dev_device_info[i].bus_num);
            continue;
        }
        if (!master) {
            dev_err(&pdev->dev, "get bus_num %u spi master failed.\n",
                spi_dev_device_info[i].bus_num);
            continue;
        }

        WB_SPI_DEV_DEVICE_DEBUG_VERBOSE("master->bus_num = %d.\n", master->bus_num);
        WB_SPI_DEV_DEVICE_DEBUG_VERBOSE("master->num_chipselect = %d.\n", master->num_chipselect);

        spi = spi_new_device(master, &spi_dev_device_info[i]);
        put_device(&master->dev);
        if (spi) {
            g_spi_device[i] = spi;
            dev_info(&g_spi_device[i]->dev, "spi_bus %d spi_new_device success.\n", spi_dev_device_info[i].bus_num);
        } else {
            g_spi_device[i] = NULL;
            dev_err(&pdev->dev, "Failed to register spi dev device %s at bus %d!\n",
                spi_dev_device_info[i].modalias, spi_dev_device_info[i].bus_num);
            continue;
        }

        WB_SPI_DEV_DEVICE_DEBUG_VERBOSE("g_spi_device[%d]->modalias = %s.\n", i, g_spi_device[i]->modalias);
        WB_SPI_DEV_DEVICE_DEBUG_VERBOSE("g_spi_device[%d]->chip_select = %d.\n", i, g_spi_device[i]->chip_select);
        WB_SPI_DEV_DEVICE_DEBUG_VERBOSE("g_spi_device[%d]->max_speed_hz = %d.\n", i, g_spi_device[i]->max_speed_hz);
    }

    return 0;
}

static int wb_spi_dev_device_remove(struct platform_device *pdev)
{
    int i;

    WB_SPI_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(spi_dev_device_info) - 1; i >= 0; i--) {
        if (g_spi_device[i]) {
            spi_unregister_device(g_spi_device[i]);
            g_spi_device[i] = NULL;
        }
    }

    return 0;
}

static const struct of_device_id spi_dev_device_match[] = {
    {
        .compatible = "wb_spi_dev_device",
    },
    {},
};
MODULE_DEVICE_TABLE(of, spi_dev_device_match);

static struct platform_driver wb_spi_dev_device_driver = {
    .driver = {
        .name        = PROXY_NAME,
        .of_match_table = spi_dev_device_match,
    },
    .probe      = wb_spi_dev_device_probe,
    .remove     = wb_spi_dev_device_remove,
};

module_platform_driver(wb_spi_dev_device_driver);

MODULE_DESCRIPTION("SPI DEV Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");

