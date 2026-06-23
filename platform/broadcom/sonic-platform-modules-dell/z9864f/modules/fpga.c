#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/ioport.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include "fpga.h"
#include "fpga_attr.h"
#include "fpga_gpio.h"
#include "fpga_i2c.h"
#include "fpga_reg.h"

#define FPGA_PCA9548
#define FPGA_GPIO
#define FPGA_ATTR

#if 0
struct file_operations fpga_fileops = {
    .owner = THIS_MODULE,
    .read = fpga_read,
    .write = fpga_write,
    .mmap = fpga_mmap,
    /*.ioctl = fpga_ioctl,*/
    //	.open = fpga_open,
    .release = fpga_close,
};
#endif

static int dni_fpga_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
    struct fpga_dev *fpga;
    int error = -1;
    int ret;

    dev_info(&dev->dev, "probe");

    ret = pci_request_regions(dev, "delta-fpga");
    if (ret)
    {
        printk(KERN_ERR "Failed to request PCI region.\n");
        return ret;
    }

    // enabling the device
    ret = pci_enable_device(dev);
    if (ret)
    {
        printk(KERN_ERR "Failed to enable PCI device.\n");
        return ret;
    }

    if (!(pci_resource_flags(dev, 0) & IORESOURCE_MEM))
    {
        printk(KERN_ERR "Incorrect BAR configuration.\n");
        ret = -ENODEV;
        goto fail_map_bars;
    }

    fpga = kzalloc(sizeof(struct fpga_dev), GFP_KERNEL);
    if (!fpga)
    {
        dev_warn(&dev->dev, "Couldn't allocate memory for fpga!\n");
        return -ENOMEM;
    }
    fpga->dev = dev;
    fpga->buffer = kmalloc(BUF_SIZE * sizeof(char), GFP_KERNEL);
    if (!fpga->buffer)
    {
        dev_warn(&dev->dev, "Couldn't allocate memory for buffer!\n");
        return -ENOMEM;
    }
#ifdef FPGA_GPIO
    /* init gpiodev */
    ret = gpiodev_init(dev, fpga);
    if (ret)
    {
        dev_err(&dev->dev, "Couldn't create gpiodev!\n");
        return ret;
    }
#endif

    ret = i2c_adapter_init(dev, fpga);
    if (ret)
    {
        dev_err(&dev->dev, "Couldn't create i2c_adapter!\n");
        goto out_release_region;
    }
#ifdef FPGA_ATTR
    /* init fpga attr */
    ret = fpga_attr_init(dev, fpga);
    if (ret)
    {
        dev_err(&dev->dev, "Couldn't init fpga attr!\n");
        return ret;
    }
#endif
    return 0;

fail_map_bars:
    pci_disable_device(dev);
out_release_region:
    release_region(fpga->pci_base, fpga->pci_size);

    kfree((fpga));
    return error;
}

static void dni_fpga_remove(struct pci_dev *dev)
{
    /* release fpga-i2c */
    int i = 0;
    struct fpga_dev *fpga = pci_get_drvdata(dev);
    printk(KERN_INFO "fpga = %p\n", fpga);

    for (i = 0; i < num_i2c_adapter; i++)
    {
        i2c_del_adapter(&(fpga->i2c + i)->adapter);
        printk(KERN_INFO "remove - FPGA-I2C-%d\n", i);
    }
#ifdef FPGA_GPIO
    /* remove gpio */
    gpiodev_exit(dev);
#endif
    /* release pci */
    pci_disable_device(dev);
    pci_release_regions(dev);
    /*release attribute */
#ifdef FPGA_ATTR    
    fpga_attr_exit();
#endif    
    kfree(fpga);
    ////printk(KERN_INFO "Goodbye\n");
}

enum chiptype
{
    LATTICE
};

static const struct of_device_id dni_fpga_of_match[] = {
    {.compatible = "dni,fpga-i2c"},
    {/* sentinel */}};

static const struct pci_device_id dni_fpga_ids[] = {
    {PCI_DEVICE(0x1204, 0x9c1d)},
    {0},
};

MODULE_DEVICE_TABLE(pci, dni_fpga_ids);

static struct pci_driver dni_fpga_driver = {
    .name = "delta-fpga",
    .id_table = dni_fpga_ids,
    .probe = dni_fpga_probe,
    .remove = dni_fpga_remove,
};

static int dni_fpga_init(void)
{
    printk(KERN_INFO "delta-fpga-init\n");
    return pci_register_driver(&dni_fpga_driver);
}

static void dni_fpga_exit(void)
{
    pci_unregister_driver(&dni_fpga_driver);
    printk(KERN_INFO "delta-fpga-exit\n");
}

MODULE_AUTHOR("amos.lin@deltaww.com");
MODULE_DESCRIPTION("Delta FPGA Driver");
MODULE_LICENSE("GPL");

module_init(dni_fpga_init);
module_exit(dni_fpga_exit);
