#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include "fpga.h"
#include "fpga_gpio.h"


fpga_gpio_s fpga_gpio_info[] = {
    {0, "ENABLE JTAG_0", FPGA_JTAG_MUX_REG, 8},
    {1, "ENABLE JTAG_1", FPGA_JTAG_MUX_REG, 9},
    {2, "ENABLE JTAG_2", FPGA_JTAG_MUX_REG, 10},
    {3, "JTAG_0 FPGA_CPU_JTAG_TDI", FPGA_JTAG_CTRL0_REG, 3},
    {4, "JTAG_0 FPGA_CPU_JTAG_TDO", FPGA_JTAG_CTRL0_REG, 2},
    {5, "JTAG_0 FPGA_CPU_JTAG_TMS", FPGA_JTAG_CTRL0_REG, 1},
    {6, "JTAG_0 FPGA_CPU_JTAG_TCK", FPGA_JTAG_CTRL0_REG, 0},
    {7, "JTAG_1 FPGA_MB_JTAG_TDI", FPGA_JTAG_CTRL1_REG, 3},
    {8, "JTAG_1 FPGA_MB_JTAG_TDO", FPGA_JTAG_CTRL1_REG, 2},
    {9, "JTAG_1 FPGA_MB_JTAG_TMS", FPGA_JTAG_CTRL1_REG, 1},
    {10, "JTAG_1 FPGA_MB_JTAG_TCK", FPGA_JTAG_CTRL1_REG, 0},
    {11, "JTAG_2 FPGA_MB_JTAG_TRST_N", FPGA_JTAG_CTRL2_REG, 4},
    {12, "JTAG_2 FPGA_MB_JTAG_TDI", FPGA_JTAG_CTRL2_REG, 3},
    {13, "JTAG_2 FPGA_MB_JTAG_TDO", FPGA_JTAG_CTRL2_REG, 2},
    {14, "JTAG_2 FPGA_MB_JTAG_TMS", FPGA_JTAG_CTRL2_REG, 1},
    {15, "JTAG_2 FPGA_MB_JTAG_TCK", FPGA_JTAG_CTRL2_REG, 0},
};

static int fpga_gpio_get(struct gpio_chip *gc, unsigned gpio)
{
    struct fpga_gpio_chip *chip = gpiochip_get_data(gc);
    int rdata;
    int ret;
    int reg = fpga_gpio_info[gpio].reg;
    int bit = fpga_gpio_info[gpio].bit;

    mutex_lock(&chip->lock);
    //ret = (chip->buffer[bank] >> pin) & 0x1;
    rdata = ioread32(chip->bar + reg);
    ret = (rdata >> bit) & 0x1;
    //printk(KERN_INFO "gpio = 0x%x chip->bar = 0x%x value = 0x%x", gpio, chip->bar, rdata);

    mutex_unlock(&chip->lock);

    return ret;
}

static void fpga_gpio_set(struct gpio_chip *gc,
                          unsigned gpio, int val)
{
    struct fpga_gpio_chip *chip = gpiochip_get_data(gc);
    int rdata, wdata;
    int reg = fpga_gpio_info[gpio].reg;
    int bit = fpga_gpio_info[gpio].bit;

    mutex_lock(&chip->lock);

    rdata = ioread32(chip->bar + reg);

    if (val)
        wdata = rdata | (1 << bit);
    else
        wdata = rdata & ~(1 << bit);

    iowrite32(wdata, chip->bar + reg);

    mutex_unlock(&chip->lock);
}

int gpiodev_init(struct pci_dev *dev, struct fpga_dev *fpga)
{
    char name[20] = "fpga-gpio chip";
    int err;
    fpga->gpio = kzalloc(sizeof(struct fpga_gpio_chip), GFP_KERNEL);
    if (!fpga->gpio)
        return -ENOMEM;

    fpga->gpio->bar = ioremap(pci_resource_start(dev, 0), pci_resource_len(dev, 0));
    fpga->gpio->gpio_chip.base = -1;
    fpga->gpio->gpio_chip.label = name;
    fpga->gpio->gpio_chip.owner = THIS_MODULE;
    fpga->gpio->gpio_chip.ngpio = 32;
    fpga->gpio->gpio_chip.parent = &dev->dev; //(struct devices) fpga

    fpga->gpio->gpio_chip.get = fpga_gpio_get;
    fpga->gpio->gpio_chip.set = fpga_gpio_set;
    err = gpiochip_add_data(&fpga->gpio->gpio_chip, fpga->gpio);
    if (err)
    {
        printk(KERN_INFO "GPIO registering failed ");
        return err;
    }
    return 0;
}

void gpiodev_exit(struct pci_dev *dev)
{
    struct fpga_dev *fpga = pci_get_drvdata(dev);
    gpiochip_remove(&fpga->gpio->gpio_chip);
}