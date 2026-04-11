#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/spi/spi_gpio.h>
#include <wb_spi_master.h>
#include <wb_bsp_kernel_debug.h>

/* The SPI Bus number that the device is mounted on can be specified manually when this module is loaded */
#define DEFAULT_SPI_BUS_NUM     (0)
#define DEFAULT_SPI_CS_NUM      (1)
#define DEFAULT_SPI_HZ          (100000)

int spi_bus_num = DEFAULT_SPI_BUS_NUM;
int spi_chip_select = DEFAULT_SPI_CS_NUM;
module_param(spi_bus_num, int, S_IRUGO | S_IWUSR);
module_param(spi_chip_select, int, S_IRUGO | S_IWUSR);

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static char *mtd_name = NULL;
module_param(mtd_name, charp, 0644);
MODULE_PARM_DESC(str_var, "A string variable for mte device name");

static struct flash_platform_data spi_nor_flash_data = {
};

struct spi_board_info spi_nor_device_info __initdata= {
    .modalias           = "spi-nor",
    .bus_num            = DEFAULT_SPI_BUS_NUM,
    .chip_select        = DEFAULT_SPI_CS_NUM,
    .max_speed_hz       = 10 * 1000 * 1000,
    .platform_data      = &spi_nor_flash_data
};

static struct spi_device *g_spi_device;

static int __init wb_spi_nor_dev_init(void)
{
    struct spi_master *master;

    DEBUG_VERBOSE("Enter.\n");

    spi_nor_device_info.bus_num = spi_bus_num;
    spi_nor_device_info.chip_select = spi_chip_select;
    master = wb_spi_master_busnum_to_master(spi_nor_device_info.bus_num);  /* Get the controller according to the SPI Bus number */
    if (!master) {
        DEBUG_ERROR("get bus_num %u spi master failed.\n",
            spi_nor_device_info.bus_num);
        return -EINVAL;
    }

    if (mtd_name) {
        spi_nor_flash_data.name = mtd_name;
    } else {
        spi_nor_device_info.platform_data = NULL;
    }

    g_spi_device = spi_new_device(master, &spi_nor_device_info);
    put_device(&master->dev);
    if (!g_spi_device) {
        DEBUG_ERROR("register spi new device failed.\n");
        return -EPERM;
    }

    if (debug) {
        dev_info(&g_spi_device->dev, "register %u bus_num spi nor device success\n",
            spi_nor_device_info.bus_num);
    }

    return 0;
}

static void __exit wb_spi_nor_dev_exit(void)
{
    spi_unregister_device(g_spi_device);

    if (debug) {
        dev_info(&g_spi_device->dev, "unregister spi nor device success\n");
    }

    return;
}

module_init(wb_spi_nor_dev_init);
module_exit(wb_spi_nor_dev_exit);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("create spi nor device");
MODULE_LICENSE("GPL");
