#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>

/* The SPI Bus number that the device is mounted on can be specified manually when this module is loaded */
#define DEFAULT_SPI_BUS_NUM     (0)
#define DEFAULT_SPI_CS_NUM      (0)
#define DEFAULT_SPI_HZ          (100000)

int g_wb_spi_nor_dev_debug = 0;
int g_wb_spi_nor_dev_error = 0;
int spi_bus_num = DEFAULT_SPI_BUS_NUM;

module_param(g_wb_spi_nor_dev_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_spi_nor_dev_error, int, S_IRUGO | S_IWUSR);
module_param(spi_bus_num, int, S_IRUGO | S_IWUSR);

#define SPI_NOR_DEV_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_spi_nor_dev_debug) { \
        printk(KERN_INFO "[SPI_NOR_DEV][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SPI_NOR_DEV_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_spi_nor_dev_error) { \
        printk(KERN_ERR "[SPI_NOR_DEV][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct spi_board_info spi_nor_device_info __initdata= {
    .modalias           = "mx25l6405d",
    .bus_num            = DEFAULT_SPI_BUS_NUM,
    .chip_select        = DEFAULT_SPI_CS_NUM,
    .max_speed_hz       = 10 * 1000 * 1000,
};

static struct spi_device *g_spi_device;

static int __init wb_spi_nor_dev_init(void)
{
    struct spi_master *master;

    SPI_NOR_DEV_DEBUG_VERBOSE("Enter.\n");

    spi_nor_device_info.bus_num = spi_bus_num;
    master = spi_busnum_to_master(spi_nor_device_info.bus_num);  /* Get the controller according to the SPI Bus number */
    if (!master) {
        SPI_NOR_DEV_DEBUG_ERROR("get bus_num %u spi master failed.\n",
            spi_nor_device_info.bus_num);
        return -EINVAL;
    }

    g_spi_device = spi_new_device(master, &spi_nor_device_info);
    put_device(&master->dev);
    if (!g_spi_device) {
        SPI_NOR_DEV_DEBUG_ERROR("register spi new device failed.\n");
        return -EPERM;
    }

    if (g_wb_spi_nor_dev_debug) {
        dev_info(&g_spi_device->dev, "register %u bus_num spi nor device success\n",
            spi_nor_device_info.bus_num);
    }

    return 0;
}

static void __exit wb_spi_nor_dev_exit(void)
{
    spi_unregister_device(g_spi_device);

    if (g_wb_spi_nor_dev_debug) {
        dev_info(&g_spi_device->dev, "unregister spi nor device success\n");
    }

    return;
}

module_init(wb_spi_nor_dev_init);
module_exit(wb_spi_nor_dev_exit);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("create spi nor device");
MODULE_LICENSE("GPL");
