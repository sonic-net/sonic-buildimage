/*
For DS5001(ICE-LAKE)
Read the CPU register through MMIO to obtain DIMM temperature information
*/

#include <linux/module.h>
#include <linux/io.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/errno.h>

// MMIO address. From BIOS EDS(V2)
#define MMIO_BASE 0xFB900000
#define DIMM1_OFFSET 0x224d4
#define DIMM2_OFFSET 0x264d4
#define MMIO_SIZE 0x30000

#define MODULE_NAME "dimm_temp"


struct dimm_temp_dev {
    void __iomem *mmio_base;
    struct device *hwdev;
};

static struct dimm_temp_dev *g_dimm_dev;


static ssize_t temp1_input_show(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
    struct dimm_temp_dev *dimm_dev = dev_get_drvdata(dev);
    uint8_t temp;

    temp = ioread8(dimm_dev->mmio_base + DIMM1_OFFSET);

    if (temp == 0xFF || temp == 0x00) {
        return -ENODATA;
    }

    return sprintf(buf, "%d\n", temp * 1000);
}
static DEVICE_ATTR_RO(temp1_input);


static ssize_t temp2_input_show(struct device *dev,
                                struct device_attribute *attr, char *buf)
{
    struct dimm_temp_dev *dimm_dev = dev_get_drvdata(dev);
    uint8_t temp;

    temp = ioread8(dimm_dev->mmio_base + DIMM2_OFFSET);

    if (temp == 0xFF || temp == 0x00) {
        return -ENODATA;
    }

    return sprintf(buf, "%d\n", temp * 1000);
}
static DEVICE_ATTR_RO(temp2_input);


static struct attribute *dimm_attrs[] = {
    &dev_attr_temp1_input.attr,
    &dev_attr_temp2_input.attr,
    NULL
};

static const struct attribute_group dimm_group = {
    .attrs = dimm_attrs,
};

static const struct attribute_group *dimm_groups[] = {
    &dimm_group,
    NULL
};

static int __init dimm_temp_init(void)
{
    struct dimm_temp_dev *dimm_dev;
    int ret = 0;

    dimm_dev = kzalloc(sizeof(*dimm_dev), GFP_KERNEL);
    if (!dimm_dev) {
        return -ENOMEM;
    }

    dimm_dev->mmio_base = ioremap(MMIO_BASE, MMIO_SIZE);
    if (!dimm_dev->mmio_base) {
        ret = -ENOMEM;
        goto free_dev;
    }

    dimm_dev->hwdev = hwmon_device_register_with_groups(
        NULL,
        MODULE_NAME,
        dimm_dev,
        dimm_groups
    );
    if (IS_ERR(dimm_dev->hwdev)) {
        ret = PTR_ERR(dimm_dev->hwdev);
        pr_err("Hwmon device registration failed, error code %d\n", ret);
        goto unmap_io;
    }

    g_dimm_dev = dimm_dev;
    pr_info(MODULE_NAME ": MMIO DIMM temperature driver loaded.\n");
    return 0;

unmap_io:
    iounmap(dimm_dev->mmio_base);
free_dev:
    kfree(dimm_dev);
    return ret;
}

static void __exit dimm_temp_exit(void)
{
    if (g_dimm_dev) {
        hwmon_device_unregister(g_dimm_dev->hwdev);
        iounmap(g_dimm_dev->mmio_base);
        kfree(g_dimm_dev);
    }
    pr_info("DIMM temperature driver unloaded\n");
}

module_init(dimm_temp_init);
module_exit(dimm_temp_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Yagami Jiang <yajiang@celestica.com>");
MODULE_DESCRIPTION("DIMM temperature driver");