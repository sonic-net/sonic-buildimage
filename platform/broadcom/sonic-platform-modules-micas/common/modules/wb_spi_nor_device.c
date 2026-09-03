#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/spi/spi_bitbang.h>
#include <linux/device.h>

#define DEFAULT_SPI_BUS_NUM     (0)
#define DEFAULT_SPI_CS_NUM      (0)
#define WB_SPI_GPIO_DEV_NAME    "wb_spi_gpio"

static int g_wb_spi_nor_dev_debug;
static int g_wb_spi_nor_dev_error;
static int spi_bus_num = DEFAULT_SPI_BUS_NUM;

module_param(g_wb_spi_nor_dev_debug, int, 0644);
module_param(g_wb_spi_nor_dev_error, int, 0644);
module_param(spi_bus_num,            int, 0444);

#define SPI_NOR_DEV_DEBUG_VERBOSE(fmt, args...) do {                \
    if (g_wb_spi_nor_dev_debug) {                                   \
        printk(KERN_INFO "[SPI_NOR_DEV][VER][%s:%d] " fmt,          \
               __func__, __LINE__, ## args);                        \
    }                                                               \
} while (0)

#define SPI_NOR_DEV_DEBUG_ERROR(fmt, args...) do {                  \
    if (g_wb_spi_nor_dev_error) {                                   \
        printk(KERN_ERR "[SPI_NOR_DEV][ERR][%s:%d] " fmt,           \
               __func__, __LINE__, ## args);                        \
    }                                                               \
} while (0)

static struct spi_board_info spi_nor_device_info = {
    .modalias       = "mx25l6405d",
    .bus_num        = DEFAULT_SPI_BUS_NUM,
    .chip_select    = DEFAULT_SPI_CS_NUM,
    .max_speed_hz   = 10 * 1000 * 1000,
    .mode           = SPI_MODE_0,
};

static struct spi_device *g_spi_device;

struct wb_spi_gpio_head {
    struct spi_bitbang bitbang;
};

static struct spi_controller *wb_spi_find_controller(int bus_num)
{
    char dev_name[64];
    struct device *pdev;
    struct wb_spi_gpio_head *gpio_head;
    struct spi_controller *ctlr;

    snprintf(dev_name, sizeof(dev_name),
             "%s.%d",
             WB_SPI_GPIO_DEV_NAME,
             bus_num);

    SPI_NOR_DEV_DEBUG_VERBOSE(
        "search controller by platform device '%s'\n",
        dev_name);

    pdev = bus_find_device_by_name(
                &platform_bus_type,
                NULL,
                dev_name);

    if (!pdev) {
        pr_err("[SPI_NOR_DEV] platform device '%s' not found\n",
               dev_name);
        return NULL;
    }

    gpio_head = dev_get_drvdata(pdev);

    if (!gpio_head) {
        pr_err("[SPI_NOR_DEV] drvdata is NULL for '%s'\n",
               dev_name);
        put_device(pdev);
        return NULL;
    }

    ctlr = gpio_head->bitbang.ctlr;

    if (!ctlr) {
        pr_err("[SPI_NOR_DEV] bitbang controller is NULL\n");
        put_device(pdev);
        return NULL;
    }

    get_device(&ctlr->dev);

    put_device(pdev);

    return ctlr;
}

struct find_spi_dev_args {
    struct spi_controller *ctlr;
    u8 cs;
    struct spi_device *found;
};

static int find_spi_dev_on_cs(struct device *dev, void *data)
{
    struct find_spi_dev_args *args = data;
    struct spi_device *spi;

    if (dev->bus != &spi_bus_type)
        return 0;

    spi = to_spi_device(dev);

    if (spi->controller == args->ctlr &&
        spi_get_chipselect(spi, 0) == args->cs) {

        get_device(dev);

        args->found = spi;

        return 1;
    }

    return 0;
}

static void wb_spi_remove_existing(struct spi_controller *ctlr,
                                   u8 cs)
{
    struct find_spi_dev_args args = {
        .ctlr  = ctlr,
        .cs    = cs,
        .found = NULL,
    };

    bus_for_each_dev(&spi_bus_type,
                     NULL,
                     &args,
                     find_spi_dev_on_cs);

    if (args.found) {

        SPI_NOR_DEV_DEBUG_VERBOSE(
            "remove existing spi device '%s' on cs=%u\n",
            dev_name(&args.found->dev),
            (unsigned)cs);

        spi_unregister_device(args.found);

        put_device(&args.found->dev);
    }
}

static int __init wb_spi_nor_dev_init(void)
{
    struct spi_controller *ctrl;

    SPI_NOR_DEV_DEBUG_VERBOSE(
        "enter, spi_bus_num=%d\n",
        spi_bus_num);

    spi_nor_device_info.bus_num = spi_bus_num;

    ctrl = wb_spi_find_controller(spi_bus_num);

    if (!ctrl) {
        SPI_NOR_DEV_DEBUG_ERROR(
            "cannot find spi controller for bus=%u\n",
            spi_bus_num);
        return -ENODEV;
    }

    SPI_NOR_DEV_DEBUG_VERBOSE(
        "found controller bus=%u num_cs=%u\n",
        ctrl->bus_num,
        ctrl->num_chipselect);

    if (ctrl->num_chipselect == 0)
        ctrl->num_chipselect = 1;

    wb_spi_remove_existing(
        ctrl,
        spi_nor_device_info.chip_select);

    g_spi_device = spi_new_device(
                        ctrl,
                        &spi_nor_device_info);

    put_device(&ctrl->dev);

    if (!g_spi_device) {
        SPI_NOR_DEV_DEBUG_ERROR(
            "spi_new_device failed\n");
        return -EPERM;
    }

    SPI_NOR_DEV_DEBUG_VERBOSE(
        "spi device create success: %s\n",
        dev_name(&g_spi_device->dev));

    return 0;
}

static void __exit wb_spi_nor_dev_exit(void)
{
    SPI_NOR_DEV_DEBUG_VERBOSE("exit\n");

    if (g_spi_device) {
        spi_unregister_device(g_spi_device);
        g_spi_device = NULL;
    }
}

module_init(wb_spi_nor_dev_init);
module_exit(wb_spi_nor_dev_exit);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("create spi nor device");
MODULE_LICENSE("GPL");
