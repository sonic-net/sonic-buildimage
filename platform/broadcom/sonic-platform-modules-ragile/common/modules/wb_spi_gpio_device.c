#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/gpio/machine.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <linux/spi/spi_gpio.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/spi/eeprom.h>
#include <linux/spi/flash.h>
#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))

#define DEFAULT_GPIO_SCK        (67)
#define DEFAULT_GPIO_MISO       (32)
#define DEFAULT_GPIO_MOSI       (65)
#define DEFAULT_GPIO_CS         (6)
#define DEFAULT_SPI_BUS         (0)

static int sck = DEFAULT_GPIO_SCK;
module_param(sck, int, S_IRUGO | S_IWUSR);

static int miso = DEFAULT_GPIO_MISO;
module_param(miso, int, S_IRUGO | S_IWUSR);

static int mosi = DEFAULT_GPIO_MOSI;
module_param(mosi, int, S_IRUGO | S_IWUSR);

static int cs = DEFAULT_GPIO_CS;
module_param(cs, int, S_IRUGO | S_IWUSR);

static int bus = DEFAULT_SPI_BUS;
module_param(bus, int, S_IRUGO | S_IWUSR);

static int g_wb_spi_gpio_device_debug = 0;
static int g_wb_spi_gpio_device_error = 0;

module_param(g_wb_spi_gpio_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_spi_gpio_device_error, int, S_IRUGO | S_IWUSR);

static char gpiod_lookup_table_devid[64];

#define WB_SPI_GPIO_DEVICE_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_spi_gpio_device_debug) { \
        printk(KERN_INFO "[WB_SPI_GPIO_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_SPI_GPIO_DEVICE_ERROR(fmt, args...) do {                                        \
    if (g_wb_spi_gpio_device_error) { \
        printk(KERN_ERR "[WB_SPI_GPIO_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static struct gpiod_lookup_table wb_spi_gpio_table = {
    .table          = {
        GPIO_LOOKUP("wb_gpio_d1500", DEFAULT_GPIO_SCK,
                "sck", GPIO_ACTIVE_HIGH),
        GPIO_LOOKUP("wb_gpio_d1500", DEFAULT_GPIO_MOSI,
                "mosi", GPIO_ACTIVE_HIGH),
        GPIO_LOOKUP("wb_gpio_d1500", DEFAULT_GPIO_MISO,
                "miso", GPIO_ACTIVE_HIGH),
        GPIO_LOOKUP("wb_gpio_d1500", DEFAULT_GPIO_CS,
                "cs", GPIO_ACTIVE_HIGH),
        { },
    },
};

static struct spi_gpio_platform_data spi_pdata = {
    .num_chipselect = 1,
};

static void spi_gpio_release(struct device *dev)
{
    return;
}

static struct platform_device wb_spi_gpio_device = {
    .name           = "wb_spi_gpio",
    .num_resources  = 0,
    .id             = -1,

    .dev = {
        .platform_data = &spi_pdata,
        .release = spi_gpio_release,
    }
};

static void wb_spi_gpio_table_devid_name_set(void) {
    int size;

    size = sizeof(gpiod_lookup_table_devid);
    wb_spi_gpio_device.id = bus;

    mem_clear(gpiod_lookup_table_devid, size);
    switch (bus) {
    case PLATFORM_DEVID_NONE:
        snprintf(gpiod_lookup_table_devid, size, "%s", wb_spi_gpio_device.name);
        break;
    case PLATFORM_DEVID_AUTO:
        snprintf(gpiod_lookup_table_devid, size, "%s.%d.auto", wb_spi_gpio_device.name, bus);
        break;
    default:
        snprintf(gpiod_lookup_table_devid, size, "%s.%d", wb_spi_gpio_device.name, bus);
        break;
    }

    wb_spi_gpio_table.dev_id = gpiod_lookup_table_devid;
    return ;
}
static int __init wb_spi_gpio_device_init(void)
{
    int err;
    struct gpiod_lookup *p;

    WB_SPI_GPIO_DEVICE_VERBOSE("enter!\n");
    wb_spi_gpio_table.table[0].chip_hwnum = sck;
    wb_spi_gpio_table.table[1].chip_hwnum = mosi;
    wb_spi_gpio_table.table[2].chip_hwnum = miso;
    wb_spi_gpio_table.table[3].chip_hwnum = cs;
    wb_spi_gpio_table_devid_name_set();
    WB_SPI_GPIO_DEVICE_VERBOSE("spi gpi device table bus[%d] dev id[%s]\n", bus, wb_spi_gpio_table.dev_id);
    for (p = &wb_spi_gpio_table.table[0]; p->key; p++) {
        WB_SPI_GPIO_DEVICE_VERBOSE("con_id:%s gpio:%d\n", p->con_id, p->chip_hwnum);
    }

    gpiod_add_lookup_table(&wb_spi_gpio_table);
    err = platform_device_register(&wb_spi_gpio_device);
    if (err < 0) {
        printk(KERN_ERR "register spi gpio device fail(%d). \n", err);
        gpiod_remove_lookup_table(&wb_spi_gpio_table);
        return -1;
    }

    return 0;
}

static void __exit wb_spi_gpio_device_exit(void)
{
    WB_SPI_GPIO_DEVICE_VERBOSE("enter!\n");
    platform_device_unregister(&wb_spi_gpio_device);
    gpiod_remove_lookup_table(&wb_spi_gpio_table);
}

module_init(wb_spi_gpio_device_init);
module_exit(wb_spi_gpio_device_exit);
MODULE_DESCRIPTION("SPI GPIO Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
