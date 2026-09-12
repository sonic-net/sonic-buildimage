#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>
#include <linux/device.h>
#include "wb_eeprom_93xx46.h"

#define EEPROM_93XX46_QUIRK_ACTIVE_HIGH_CS		(1 << 2)

#define DEFAULT_SPI_BUS_NUM     (0)
#define DEFAULT_SPI_CS_NUM      (0)
#define DEFAULT_SPI_HZ          (100000)
#define GPIO_EEPROM_CS          (-1)

int g_wb_spi_93xx46_debug = 0;
int g_wb_spi_93xx46_error = 0;
int spi_bus_num = DEFAULT_SPI_BUS_NUM;
int spi_cs_gpio = GPIO_EEPROM_CS;

module_param(g_wb_spi_93xx46_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_spi_93xx46_error, int, S_IRUGO | S_IWUSR);
module_param(spi_bus_num, int, S_IRUGO | S_IWUSR);
module_param(spi_cs_gpio, int, S_IRUGO | S_IWUSR);

#define SPI_93xx46_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_spi_93xx46_debug) { \
        printk(KERN_INFO "[SPI-93xx46][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define SPI_93xx46_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_spi_93xx46_error) { \
        printk(KERN_ERR "[SPI-93xx46][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

struct eeprom_93xx46_platform_data eeprom_data = {
    .flags      = EE_ADDR16,
    .quirks     = EEPROM_93XX46_QUIRK_SINGLE_WORD_READ,
};

struct spi_board_info eeprom_93xx46_info __initdata = {
        .modalias           = "wb_93xx46",
        .max_speed_hz       = DEFAULT_SPI_HZ,
        .bus_num            = DEFAULT_SPI_BUS_NUM,
        .chip_select        = DEFAULT_SPI_CS_NUM,
        .mode               = SPI_MODE_0 | SPI_CS_HIGH,
        .controller_data    = (void *)GPIO_EEPROM_CS,
        .platform_data      = &eeprom_data,
};

static struct spi_device *g_spi_device;

static struct spi_controller *wb_spi_find_master(u16 bus_num)
{
    struct device *pdev;
    struct device *ctlr_dev;
    char pdev_name[32];
    char ctlr_name[16];

    snprintf(pdev_name, sizeof(pdev_name), "wb_spi_gpio.%u", (unsigned int)bus_num);
    snprintf(ctlr_name, sizeof(ctlr_name), "spi%u", (unsigned int)bus_num);

    pdev = bus_find_device_by_name(&platform_bus_type, NULL, pdev_name);
    if (!pdev)
        return NULL;

    ctlr_dev = device_find_child_by_name(pdev, ctlr_name);
    put_device(pdev);
    if (!ctlr_dev)
        return NULL;

    return container_of(ctlr_dev, struct spi_controller, dev);
}

static int __init wb_spi_93xx46_init(void)
{
    struct spi_controller *master;
    SPI_93xx46_DEBUG_VERBOSE("Enter.\n");
    eeprom_93xx46_info.bus_num = spi_bus_num;
    eeprom_93xx46_info.controller_data = (void *)(long)spi_cs_gpio;
    master = wb_spi_find_master(eeprom_93xx46_info.bus_num);
    if (!master) {
        pr_err("[SPI-93xx46] spi master not found for bus %u "
               "(check wb_spi_gpio.%u is loaded and the SPI controller is bound)\n",
               eeprom_93xx46_info.bus_num, eeprom_93xx46_info.bus_num);
        return -EINVAL;
    }
    g_spi_device = spi_new_device(master, &eeprom_93xx46_info);
    put_device(&master->dev);
    if (!g_spi_device) {
        pr_err("[SPI-93xx46] spi_new_device failed for bus %u cs %u\n",
               eeprom_93xx46_info.bus_num, eeprom_93xx46_info.chip_select);
        return -EPERM;
    }
    if (g_wb_spi_93xx46_debug) {
        dev_info(&g_spi_device->dev, "register %u bus_num spi 93xx46 eeprom success\n",
            eeprom_93xx46_info.bus_num);
    }
    return 0;
}

static void __exit wb_spi_93xx46_exit(void)
{
    spi_unregister_device(g_spi_device);
    if (g_wb_spi_93xx46_debug) {
        dev_info(&g_spi_device->dev, "unregister spi 93xx46 eeprom success\n");
    }
    return;
}

module_init(wb_spi_93xx46_init);
module_exit(wb_spi_93xx46_exit);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("create 93xx46 eeprom device");
MODULE_LICENSE("GPL");
