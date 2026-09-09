#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/msi.h>
#include <linux/mfd/core.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <linux/pci_ids.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/time.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/hwmon-sysfs.h>
#include "fpga_pcie_uart.h"

#define VERSION_VALUE(d) ((d & 0xffff) >> 8)

/* Debug Control */
int pcie_debug_error = 0;
int pcie_debug_info = 0;
int show_fpga_reg = 0;

module_param(pcie_debug_error, int, S_IRUGO | S_IWUSR);
module_param(pcie_debug_info, int, S_IRUGO | S_IWUSR);
module_param(show_fpga_reg, int, S_IRUGO | S_IWUSR);

#define DEBUG_ERROR(fmt, args...) do {                                        \
        if (pcie_debug_error) { \
            printk("[FPGA_PCIE][ERR][func:%s line:%d] "fmt, __func__,     \
                        __LINE__, ##args);                                         \
        } \
    } while (0)

#define DEBUG_INFO(fmt, args...) do {                                        \
        if (pcie_debug_info) { \
            printk("[FPGA_PCIE][INFO][func:%s line:%d] "fmt, __func__,     \
                        __LINE__, ##args);                                         \
        } \
    } while (0)

/* Define Platform Data via Micro */
#define DEFINE_FPGA_PCIE_UART_DATA(_id) \
static struct mc_uart_platform_data mc_uart_pdata_##_id = {        \
    .baud_rate = UART_BARD_RATE,                                   \
    .id = _id,                                                      \
    .div = UART_DIVISOR,                                      \
    .clk = UART_CLOCK                                 \
};

#define DEFINE_FPGA_PCIE_OCORE_DATA(_id) \
static struct mc_ocores_uart_i2c_platform_data mc_i2c_ocore_pdata_##_id = {        \
    .reg_shift = 0,                                                      \
    .reg_io_width = 4,                                                  \
    .clock_khz = 125000,                                               \
    .num_devices = 0,                                          \
    .nr = ((_id) + 1 )             \
};

DEFINE_FPGA_PCIE_UART_DATA(0);
DEFINE_FPGA_PCIE_UART_DATA(1);
DEFINE_FPGA_PCIE_UART_DATA(2);
DEFINE_FPGA_PCIE_UART_DATA(3);
DEFINE_FPGA_PCIE_UART_DATA(4);
DEFINE_FPGA_PCIE_UART_DATA(5);
DEFINE_FPGA_PCIE_UART_DATA(6);
DEFINE_FPGA_PCIE_UART_DATA(7);
DEFINE_FPGA_PCIE_UART_DATA(8);
DEFINE_FPGA_PCIE_UART_DATA(9);
DEFINE_FPGA_PCIE_UART_DATA(10);
DEFINE_FPGA_PCIE_UART_DATA(11);
DEFINE_FPGA_PCIE_UART_DATA(12);
DEFINE_FPGA_PCIE_UART_DATA(13);
DEFINE_FPGA_PCIE_UART_DATA(14);
DEFINE_FPGA_PCIE_UART_DATA(15);
DEFINE_FPGA_PCIE_UART_DATA(16);
DEFINE_FPGA_PCIE_UART_DATA(17);
DEFINE_FPGA_PCIE_UART_DATA(18);
DEFINE_FPGA_PCIE_UART_DATA(19);
DEFINE_FPGA_PCIE_UART_DATA(20);
DEFINE_FPGA_PCIE_UART_DATA(21);
DEFINE_FPGA_PCIE_UART_DATA(22);
DEFINE_FPGA_PCIE_UART_DATA(23);
DEFINE_FPGA_PCIE_UART_DATA(24);
DEFINE_FPGA_PCIE_UART_DATA(25);
DEFINE_FPGA_PCIE_UART_DATA(26);
DEFINE_FPGA_PCIE_UART_DATA(27);
DEFINE_FPGA_PCIE_UART_DATA(28);
DEFINE_FPGA_PCIE_UART_DATA(29);
DEFINE_FPGA_PCIE_UART_DATA(30);
DEFINE_FPGA_PCIE_UART_DATA(31);
DEFINE_FPGA_PCIE_UART_DATA(32);
DEFINE_FPGA_PCIE_UART_DATA(33);
DEFINE_FPGA_PCIE_UART_DATA(34);
DEFINE_FPGA_PCIE_UART_DATA(35);
DEFINE_FPGA_PCIE_UART_DATA(36);
DEFINE_FPGA_PCIE_UART_DATA(37);
DEFINE_FPGA_PCIE_UART_DATA(38);
DEFINE_FPGA_PCIE_UART_DATA(39);
DEFINE_FPGA_PCIE_UART_DATA(40);
DEFINE_FPGA_PCIE_UART_DATA(41);
DEFINE_FPGA_PCIE_UART_DATA(42);
DEFINE_FPGA_PCIE_UART_DATA(43);
DEFINE_FPGA_PCIE_UART_DATA(44);
DEFINE_FPGA_PCIE_UART_DATA(45);
DEFINE_FPGA_PCIE_UART_DATA(46);
DEFINE_FPGA_PCIE_UART_DATA(47);

DEFINE_FPGA_PCIE_OCORE_DATA(11);
DEFINE_FPGA_PCIE_OCORE_DATA(12);
DEFINE_FPGA_PCIE_OCORE_DATA(13);
DEFINE_FPGA_PCIE_OCORE_DATA(14);
DEFINE_FPGA_PCIE_OCORE_DATA(15);
DEFINE_FPGA_PCIE_OCORE_DATA(16);
DEFINE_FPGA_PCIE_OCORE_DATA(17);

/* Define Resource via Micro */
#define DEFINE_FPGA_PCIE_UART_RESOURCES(_id) \
    static const struct resource fpga_pcie_uart_resources_##_id[] = {      \
    {                                                                           \
        .start    = FPGA_UART_CTRL_START(_id),                                 \
        .end    = FPGA_UART_CTRL_END(_id),                                   \
        .flags    = IORESOURCE_MEM,                                               \
    },                                                                          \
    {                                                                           \
        .start     = FPGA_UART_CTRL_IRQ(_id),                                  \
        .end    = FPGA_UART_CTRL_IRQ(_id),                                  \
        .flags    = IORESOURCE_IRQ,                                               \
    },                                                                          \
}

#define DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(_id) \
    static const struct resource fpga_pcie_i2c_ocores_resources_##_id[] = {      \
    {                                                                           \
        .start    = FPGA_I2C_OCORE_CTRL_START(_id),                                 \
        .end    = FPGA_I2C_OCORE_CTRL_END(_id),                                   \
        .flags    = IORESOURCE_MEM,                                               \
    },                                                                          \
    {                                                                           \
        .start     = FPGA_I2C_OCORE_CTRL_IRQ(_id),                                  \
        .end    = FPGA_I2C_OCORE_CTRL_IRQ(_id),                                  \
        .flags    = IORESOURCE_IRQ,                                               \
    },                                                                          \
}

DEFINE_FPGA_PCIE_UART_RESOURCES(0);
DEFINE_FPGA_PCIE_UART_RESOURCES(1);
DEFINE_FPGA_PCIE_UART_RESOURCES(2);
DEFINE_FPGA_PCIE_UART_RESOURCES(3);
DEFINE_FPGA_PCIE_UART_RESOURCES(4);
DEFINE_FPGA_PCIE_UART_RESOURCES(5);
DEFINE_FPGA_PCIE_UART_RESOURCES(6);
DEFINE_FPGA_PCIE_UART_RESOURCES(7);
DEFINE_FPGA_PCIE_UART_RESOURCES(8);
DEFINE_FPGA_PCIE_UART_RESOURCES(9);
DEFINE_FPGA_PCIE_UART_RESOURCES(10);
DEFINE_FPGA_PCIE_UART_RESOURCES(11);
DEFINE_FPGA_PCIE_UART_RESOURCES(12);
DEFINE_FPGA_PCIE_UART_RESOURCES(13);
DEFINE_FPGA_PCIE_UART_RESOURCES(14);
DEFINE_FPGA_PCIE_UART_RESOURCES(15);
DEFINE_FPGA_PCIE_UART_RESOURCES(16);
DEFINE_FPGA_PCIE_UART_RESOURCES(17);
DEFINE_FPGA_PCIE_UART_RESOURCES(18);
DEFINE_FPGA_PCIE_UART_RESOURCES(19);
DEFINE_FPGA_PCIE_UART_RESOURCES(20);
DEFINE_FPGA_PCIE_UART_RESOURCES(21);
DEFINE_FPGA_PCIE_UART_RESOURCES(22);
DEFINE_FPGA_PCIE_UART_RESOURCES(23);
DEFINE_FPGA_PCIE_UART_RESOURCES(24);
DEFINE_FPGA_PCIE_UART_RESOURCES(25);
DEFINE_FPGA_PCIE_UART_RESOURCES(26);
DEFINE_FPGA_PCIE_UART_RESOURCES(27);
DEFINE_FPGA_PCIE_UART_RESOURCES(28);
DEFINE_FPGA_PCIE_UART_RESOURCES(29);
DEFINE_FPGA_PCIE_UART_RESOURCES(30);
DEFINE_FPGA_PCIE_UART_RESOURCES(31);
DEFINE_FPGA_PCIE_UART_RESOURCES(32);
DEFINE_FPGA_PCIE_UART_RESOURCES(33);
DEFINE_FPGA_PCIE_UART_RESOURCES(34);
DEFINE_FPGA_PCIE_UART_RESOURCES(35);
DEFINE_FPGA_PCIE_UART_RESOURCES(36);
DEFINE_FPGA_PCIE_UART_RESOURCES(37);
DEFINE_FPGA_PCIE_UART_RESOURCES(38);
DEFINE_FPGA_PCIE_UART_RESOURCES(39);
DEFINE_FPGA_PCIE_UART_RESOURCES(40);
DEFINE_FPGA_PCIE_UART_RESOURCES(41);
DEFINE_FPGA_PCIE_UART_RESOURCES(42);
DEFINE_FPGA_PCIE_UART_RESOURCES(43);
DEFINE_FPGA_PCIE_UART_RESOURCES(44);
DEFINE_FPGA_PCIE_UART_RESOURCES(45);
DEFINE_FPGA_PCIE_UART_RESOURCES(46);
DEFINE_FPGA_PCIE_UART_RESOURCES(47);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(11);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(12);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(13);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(14);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(15);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(16);
DEFINE_FPGA_PCIE_I2C_OCORE_RESOURCES(17);

/* Define MTD CELL via Micro */
#define DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(_id)                                   \
    {                                                                       \
        .name = "mc-uart",                                            \
        .id            = (_id),                                                \
        .num_resources = ARRAY_SIZE(fpga_pcie_uart_resources_##_id),   \
        .resources = fpga_pcie_uart_resources_##_id,                   \
        .platform_data = &mc_uart_pdata_##_id,                               \
        .pdata_size = sizeof(mc_uart_pdata_##_id),                           \
    }

#define DEFINE_FPGA_PCIE_MFD_CELL_CFG(_id)                                   \
    {                                                                       \
        .name = "mc-i2c-ocores",                                            \
        .id            = (_id),                                                \
        .num_resources = ARRAY_SIZE(fpga_pcie_i2c_ocores_resources_##_id),   \
        .resources = fpga_pcie_i2c_ocores_resources_##_id,                   \
        .platform_data = &mc_i2c_ocore_pdata_##_id,                               \
        .pdata_size = sizeof(mc_i2c_ocore_pdata_##_id),                           \
    }

static const struct mfd_cell fpga_pcie_cells_bar0_cfg0[] = {
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(0),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(1),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(2),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(3),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(4),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(5),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(6),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(7),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(8),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(9),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(10),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(11),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(12),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(13),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(14),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(15),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(16),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(17),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(18),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(19),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(20),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(21),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(22),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(23),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(24),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(25),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(26),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(27),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(28),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(29),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(30),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(31),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(32),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(33),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(34),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(35),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(36),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(37),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(38),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(39),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(40),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(41),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(42),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(43),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(44),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(45),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(46),
    DEFINE_FPGA_PCIE_UART_MFD_CELL_CFG(47),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(11),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(12),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(13),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(14),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(15),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(16),
    DEFINE_FPGA_PCIE_MFD_CELL_CFG(17),
};

struct mc_uart_dev {
    resource_size_t        ctl_mapbase;
    unsigned char __iomem   *ctl_membase;
    struct {
        u32 version;
        u32 date;
        u32 golden_version;
        u32 driver_version;
    } fw;
};

static void mem32_pcie_out(struct mc_uart_dev *priv, int offset, int value)
{
    iowrite32(value, priv->ctl_membase + offset);
}

static unsigned int mem32_pcie_in(struct mc_uart_dev *priv, int offset)
{
    return ioread32(priv->ctl_membase + offset);
}

static ssize_t show_uart_pre(struct device *dev, struct device_attribute *da, char *buf)
{
    struct pci_dev *pdev;
    struct mc_uart_dev *priv;
    struct sensor_device_attribute *attr;
    int offset, result;

    pdev = container_of(dev, struct pci_dev, dev);
    attr = to_sensor_dev_attr(da);
    priv = pci_get_drvdata(pdev);
    offset = (attr->index);
    result = mem32_pcie_in(priv, FPGA_UART_PRE_BASE + offset);
    return snprintf(buf, BUFF_SIZE, "%d\n", result);
}

static ssize_t reset_uart(struct device *dev, struct device_attribute *da,
    const char *buf, size_t count)
{
    struct pci_dev *pdev;
    struct mc_uart_dev *priv;
    struct sensor_device_attribute *attr;
    unsigned long control;
    int err;
    int offset;

    pdev = container_of(dev, struct pci_dev, dev);
    attr = to_sensor_dev_attr(da);
    priv = pci_get_drvdata(pdev);
    err = kstrtoul(buf, 16, &control);
    if (err){
        return err;
    }

    offset = (attr->index / 8) * FPGA_UART_FREEZE_SIZE;
    mem32_pcie_out(priv, FPGA_UART_FREEZE_BASE + offset, control);
    return count;
}

static ssize_t show_uart_led(struct device *dev, struct device_attribute *da, char *buf) {
    struct pci_dev *pdev;
    struct mc_uart_dev *priv;
    struct sensor_device_attribute *attr;
    int val;

    pdev = container_of(dev, struct pci_dev, dev);
    attr = to_sensor_dev_attr(da);
    priv = pci_get_drvdata(pdev);
    val = mem32_pcie_in(priv, FPGA_UART_LED_BASE + (attr->index) * FPGA_UART_LED_SIZE);
    return snprintf(buf, BUFF_SIZE, "0x%x\n", val);
}

static ssize_t set_uart_led(struct device *dev, struct device_attribute *da,
    const char *buf, size_t count)
{
    struct pci_dev *pdev;
    struct mc_uart_dev *priv;
    struct sensor_device_attribute *attr;
    unsigned long control;
    int err;

    pdev = container_of(dev, struct pci_dev, dev);
    attr = to_sensor_dev_attr(da);
    priv = pci_get_drvdata(pdev);
    err = kstrtoul(buf, 16, &control);
    if (err){
        return err;
    }

    mem32_pcie_out(priv, FPGA_UART_LED_BASE +
        (attr->index) * FPGA_UART_LED_SIZE, control);
    return count;
}
static ssize_t show_uart_baudrate(struct device *dev, struct device_attribute *da, char *buf) {
    struct pci_dev *pdev;
    struct mc_uart_dev *priv;
    struct sensor_device_attribute *attr;
    int val, count;

    pdev = container_of(dev, struct pci_dev, dev);
    attr = to_sensor_dev_attr(da);
    priv = pci_get_drvdata(pdev);
    val = mem32_pcie_in(priv, FPGA_UART_BAUDRATE_BASE + (attr->index) * FPGA_UART_BAUDRATE_SIZE);
    switch (((val & (BIT(16) | BIT(17))) >> 16)) {
        case 0b10:
            count = snprintf(buf, BUFF_SIZE, "%d\n", 9600);
            break;
        case 0b11:
            count = snprintf(buf, BUFF_SIZE, "%d\n", 115200);
            break;
        default:
            count = snprintf(buf, BUFF_SIZE, "%s\n",  "NA");
            break;
    }
    return count;
}
/* UART Baudrate */
static SENSOR_DEVICE_ATTR(mc_uart_baudrate0, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 0);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate1, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 1);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate2, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 2);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate3, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 3);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate4, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 4);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate5, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 5);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate6, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 6);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate7, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 7);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate8, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 8);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate9, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 9);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate10, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 10);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate11, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 11);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate12, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 12);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate13, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 13);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate14, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 14);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate15, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 15);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate16, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 16);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate17, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 17);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate18, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 18);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate19, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 19);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate20, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 20);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate21, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 21);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate22, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 22);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate23, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 23);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate24, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 24);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate25, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 25);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate26, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 26);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate27, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 27);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate28, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 28);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate29, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 29);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate30, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 30);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate31, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 31);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate32, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 32);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate33, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 33);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate34, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 34);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate35, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 35);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate36, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 36);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate37, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 37);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate38, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 38);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate39, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 39);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate40, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 40);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate41, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 41);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate42, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 42);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate43, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 43);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate44, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 44);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate45, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 45);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate46, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 46);
static SENSOR_DEVICE_ATTR(mc_uart_baudrate47, S_IRUGO | S_IWUSR, show_uart_baudrate, NULL, 47);
/* Presence*/
static SENSOR_DEVICE_ATTR(mc_uart_presence0, S_IRUGO | S_IWUSR, show_uart_pre, NULL, 0x00);
static SENSOR_DEVICE_ATTR(mc_uart_presence1, S_IRUGO | S_IWUSR, show_uart_pre, NULL, 0x04);
static SENSOR_DEVICE_ATTR(mc_uart_presence2, S_IRUGO | S_IWUSR, show_uart_pre, NULL, 0x08);
static SENSOR_DEVICE_ATTR(mc_uart_presence3, S_IRUGO | S_IWUSR, show_uart_pre, NULL, 0x0c);
static SENSOR_DEVICE_ATTR(mc_uart_presence4, S_IRUGO | S_IWUSR, show_uart_pre, NULL, 0x10);
static SENSOR_DEVICE_ATTR(mc_uart_presence5, S_IRUGO | S_IWUSR, show_uart_pre, NULL, 0x14);
/* UART Freeze Sysfs */
static SENSOR_DEVICE_ATTR(mc_uart_freeze0, S_IRUGO | S_IWUSR, NULL, reset_uart, 0);
static SENSOR_DEVICE_ATTR(mc_uart_freeze1, S_IRUGO | S_IWUSR, NULL, reset_uart, 1);
static SENSOR_DEVICE_ATTR(mc_uart_freeze2, S_IRUGO | S_IWUSR, NULL, reset_uart, 2);
static SENSOR_DEVICE_ATTR(mc_uart_freeze3, S_IRUGO | S_IWUSR, NULL, reset_uart, 3);
static SENSOR_DEVICE_ATTR(mc_uart_freeze4, S_IRUGO | S_IWUSR, NULL, reset_uart, 4);
static SENSOR_DEVICE_ATTR(mc_uart_freeze5, S_IRUGO | S_IWUSR, NULL, reset_uart, 5);
/* UART LED Sysfs */
static SENSOR_DEVICE_ATTR(mc_uart_led0, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 0);
static SENSOR_DEVICE_ATTR(mc_uart_led1, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 1);
static SENSOR_DEVICE_ATTR(mc_uart_led2, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 2);
static SENSOR_DEVICE_ATTR(mc_uart_led3, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 3);
static SENSOR_DEVICE_ATTR(mc_uart_led4, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 4);
static SENSOR_DEVICE_ATTR(mc_uart_led5, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 5);
static SENSOR_DEVICE_ATTR(mc_uart_led6, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 6);
static SENSOR_DEVICE_ATTR(mc_uart_led7, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 7);
static SENSOR_DEVICE_ATTR(mc_uart_led8, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 8);
static SENSOR_DEVICE_ATTR(mc_uart_led9, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 9);
static SENSOR_DEVICE_ATTR(mc_uart_led10, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 10);
static SENSOR_DEVICE_ATTR(mc_uart_led11, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 11);
static SENSOR_DEVICE_ATTR(mc_uart_led12, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 12);
static SENSOR_DEVICE_ATTR(mc_uart_led13, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 13);
static SENSOR_DEVICE_ATTR(mc_uart_led14, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 14);
static SENSOR_DEVICE_ATTR(mc_uart_led15, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 15);
static SENSOR_DEVICE_ATTR(mc_uart_led16, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 16);
static SENSOR_DEVICE_ATTR(mc_uart_led17, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 17);
static SENSOR_DEVICE_ATTR(mc_uart_led18, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 18);
static SENSOR_DEVICE_ATTR(mc_uart_led19, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 19);
static SENSOR_DEVICE_ATTR(mc_uart_led20, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 20);
static SENSOR_DEVICE_ATTR(mc_uart_led21, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 21);
static SENSOR_DEVICE_ATTR(mc_uart_led22, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 22);
static SENSOR_DEVICE_ATTR(mc_uart_led23, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 23);
static SENSOR_DEVICE_ATTR(mc_uart_led24, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 24);
static SENSOR_DEVICE_ATTR(mc_uart_led25, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 25);
static SENSOR_DEVICE_ATTR(mc_uart_led26, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 26);
static SENSOR_DEVICE_ATTR(mc_uart_led27, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 27);
static SENSOR_DEVICE_ATTR(mc_uart_led28, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 28);
static SENSOR_DEVICE_ATTR(mc_uart_led29, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 29);
static SENSOR_DEVICE_ATTR(mc_uart_led30, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 30);
static SENSOR_DEVICE_ATTR(mc_uart_led31, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 31);
static SENSOR_DEVICE_ATTR(mc_uart_led32, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 32);
static SENSOR_DEVICE_ATTR(mc_uart_led33, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 33);
static SENSOR_DEVICE_ATTR(mc_uart_led34, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 34);
static SENSOR_DEVICE_ATTR(mc_uart_led35, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 35);
static SENSOR_DEVICE_ATTR(mc_uart_led36, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 36);
static SENSOR_DEVICE_ATTR(mc_uart_led37, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 37);
static SENSOR_DEVICE_ATTR(mc_uart_led38, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 38);
static SENSOR_DEVICE_ATTR(mc_uart_led39, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 39);
static SENSOR_DEVICE_ATTR(mc_uart_led40, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 40);
static SENSOR_DEVICE_ATTR(mc_uart_led41, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 41);
static SENSOR_DEVICE_ATTR(mc_uart_led42, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 42);
static SENSOR_DEVICE_ATTR(mc_uart_led43, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 43);
static SENSOR_DEVICE_ATTR(mc_uart_led44, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 44);
static SENSOR_DEVICE_ATTR(mc_uart_led45, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 45);
static SENSOR_DEVICE_ATTR(mc_uart_led46, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 46);
static SENSOR_DEVICE_ATTR(mc_uart_led47, S_IRUGO | S_IWUSR, show_uart_led, set_uart_led, 47);

static struct attribute *mc_uart_sysfs_attrs[] = {
    /* Present Sysfs */
    &sensor_dev_attr_mc_uart_presence0.dev_attr.attr,
    &sensor_dev_attr_mc_uart_presence1.dev_attr.attr,
    &sensor_dev_attr_mc_uart_presence2.dev_attr.attr,
    &sensor_dev_attr_mc_uart_presence3.dev_attr.attr,
    &sensor_dev_attr_mc_uart_presence4.dev_attr.attr,
    &sensor_dev_attr_mc_uart_presence5.dev_attr.attr,
    /* Reset Sysfs */
    &sensor_dev_attr_mc_uart_freeze0.dev_attr.attr,
    &sensor_dev_attr_mc_uart_freeze1.dev_attr.attr,
    &sensor_dev_attr_mc_uart_freeze2.dev_attr.attr,
    &sensor_dev_attr_mc_uart_freeze3.dev_attr.attr,
    &sensor_dev_attr_mc_uart_freeze4.dev_attr.attr,
    &sensor_dev_attr_mc_uart_freeze5.dev_attr.attr,
    /* Led Sysfs */
    &sensor_dev_attr_mc_uart_led0.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led1.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led2.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led3.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led4.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led5.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led6.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led7.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led8.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led9.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led10.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led11.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led12.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led13.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led14.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led15.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led16.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led17.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led18.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led19.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led20.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led21.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led22.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led23.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led24.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led25.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led26.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led27.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led28.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led29.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led30.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led31.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led32.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led33.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led34.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led35.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led36.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led37.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led38.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led39.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led40.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led41.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led42.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led43.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led44.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led45.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led46.dev_attr.attr,
    &sensor_dev_attr_mc_uart_led47.dev_attr.attr,
    /* Baudrate Sysfs */
    &sensor_dev_attr_mc_uart_baudrate0.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate1.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate2.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate3.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate4.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate5.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate6.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate7.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate8.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate9.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate10.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate11.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate12.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate13.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate14.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate15.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate16.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate17.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate18.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate19.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate20.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate21.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate22.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate23.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate24.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate25.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate26.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate27.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate28.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate29.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate30.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate31.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate32.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate33.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate34.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate35.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate36.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate37.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate38.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate39.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate40.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate41.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate42.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate43.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate44.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate45.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate46.dev_attr.attr,
    &sensor_dev_attr_mc_uart_baudrate47.dev_attr.attr,
    NULL
};

static const struct attribute_group uart_sysfs_group = {
    .attrs = mc_uart_sysfs_attrs,
};

static int fpga_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    struct mc_uart_dev *priv;
    int ret;
    resource_size_t mapbase;

    ret = pci_enable_device(pdev);
    if (ret){
        DEBUG_ERROR("Enable PCI Device Failed: %d\n", ret);
        goto err_enable;
    }

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv){
        DEBUG_ERROR("Apply Memory Failed: -%d\n", ENOMEM);
        goto err_kzalloc;
    }

    pci_set_master(pdev);
    mapbase = pci_resource_start(pdev, 0);
    if (!mapbase) {
        DEBUG_ERROR("No Resources.\n");
        goto err_start;
    }
    priv->ctl_mapbase = mapbase + CTLOFFSET_START;
    if (!request_mem_region(priv->ctl_mapbase, CTLSIZE, "fpga-uart-ctl")) {
        DEBUG_ERROR("Failed to request ctl mem\n");
        goto err_start;
    }
    priv->ctl_membase = devm_ioremap(&pdev->dev, priv->ctl_mapbase, CTLSIZE);
    if (!priv->ctl_membase) {
        DEBUG_ERROR("ioremap failed for ctl mem\n");
        goto err_ioremap;
    }

    priv->fw.version = mem32_pcie_in(priv, VERSION_OFFSET);
    priv->fw.date = mem32_pcie_in(priv, DATE_OFFSET);
    priv->fw.golden_version = mem32_pcie_in(priv, GOLDON_OFFSET);
    priv->fw.driver_version = VERSION_VALUE(priv->fw.version);

    DEBUG_INFO("FPGA driver_version: 0x%x\n", priv->fw.driver_version);
    DEBUG_INFO("FPGA Version: 0x%x\n", priv->fw.version);
    DEBUG_INFO("FPGA Date: 0x%x\n", priv->fw.date);
    DEBUG_INFO("FPGA Golden Version: 0x%x\n", priv->fw.golden_version);

    DEBUG_INFO("Enable PCI MSI Interrupts.\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,11,0)
    ret = pci_enable_msi_range(pdev, UART_I2C_MSI_START , UART_I2C_MSI_NUM);
#else
    ret = pci_alloc_irq_vectors_affinity(pdev, UART_I2C_MSI_START, UART_I2C_MSI_NUM, PCI_IRQ_MSI, NULL);
#endif
    if (ret != UART_I2C_MSI_NUM) {
        DEBUG_ERROR("MSI init failed: %d, expected entries: %d\n",
            ret, FPGA_MSI_IRQ_NUM);
        goto err_msix;
    }

    pci_set_drvdata(pdev, priv);

    DEBUG_INFO("Add MFD Devices.  %ld\n", (long int)ARRAY_SIZE(fpga_pcie_cells_bar0_cfg0));
    ret = mfd_add_devices(&pdev->dev, 0, fpga_pcie_cells_bar0_cfg0,
                ARRAY_SIZE(fpga_pcie_cells_bar0_cfg0),
                &pdev->resource[0], pdev->irq, NULL);
    if (ret) {
        DEBUG_ERROR("Add MFD Device Failed:%d\n", ret);
        goto err_create_device;
    }

    ret = sysfs_create_group(&pdev->dev.kobj, &uart_sysfs_group);
    if (ret != 0) {
        DEBUG_ERROR("Create Sysfs Failed:%d\n", ret);
        goto err_create_sysfs;
    }

    return 0;

err_create_sysfs:
err_create_device:
    pci_disable_msi(pdev);
err_msix:
err_ioremap:
    release_mem_region(priv->ctl_mapbase, CTLSIZE);
err_start:
err_kzalloc:
    pci_disable_device(pdev);
err_enable:
    return -ENODEV;
}

static void fpga_pcie_remove(struct pci_dev *pdev)
{
    struct mc_uart_dev *priv;

    sysfs_remove_group(&pdev->dev.kobj, &uart_sysfs_group);
    priv = pci_get_drvdata(pdev);
    mfd_remove_devices(&pdev->dev);
    pci_disable_msi(pdev);
    release_mem_region(priv->ctl_mapbase, CTLSIZE);
    pci_disable_device(pdev);
}

static const struct pci_device_id fpga_pci_ids[] = {
        { PCI_DEVICE(PCI_VENDOR_ID_XILINX, PCI_DEVICE_ID_MASS)},
        { PCI_DEVICE(PCI_VENDOR_ID_ALIBABA, PCI_DEVICE_ID_AC51_48C2G)},
        { PCI_DEVICE(PCI_VENDOR_ID_XILINX, PCI_DEVICE_ID_W5000_48CON2SFP)},
        {0}
};
MODULE_DEVICE_TABLE(pci, fpga_pci_ids);

static struct pci_driver fpga_pcie_driver = {
    .name = "fpga_pcie_uart",
    .id_table = fpga_pci_ids,/* only dynamic id's */
    .probe = fpga_pcie_probe,
    .remove = fpga_pcie_remove,
};

static int __init fpga_pcie_init(void)
{
    return pci_register_driver(&fpga_pcie_driver);
}

static void __exit fpga_pcie_exit(void)
{
    pci_unregister_driver(&fpga_pcie_driver);
}

module_init(fpga_pcie_init);
module_exit(fpga_pcie_exit);
MODULE_AUTHOR("Micas <rd@micas.com.cn>");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");
