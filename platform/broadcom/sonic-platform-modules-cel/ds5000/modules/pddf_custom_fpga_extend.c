// SPDX-License-Identifier: GPL-2.0+
/*
 * fpga-cls.c - front panel port control.
 *
 * Pradchaya Phucharoen <pphuchar@celestica.com>
 * Copyright (C) 2019 Celestica Corp.
 */

#include <linux/errno.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <linux/acpi.h>
#include "../../../../pddf/i2c/modules/include/pddf_i2c_algo.h"

#define XILINX_FPGA_VERSION            0x0000
#define XILINX_FPGA_SCRATCH            0x0004
#define XILINX_FPGA_REG_SIZE           0x2000
#define TH5_MAX_TEMP_REG               0x0078
#define TH5_MIN_TEMP_REG               0x0080
#define TH5_SDK_READY_REG              0x0084

#define DRV_NAME "cls_sw_fpga"
/*
 * fpga_priv - port fpga private data
 * @dev: device for reference
 * @base: virtual base address
 * @num_ports: number of front panel ports
 * @fp_devs: list of front panel port devices
 */
struct fpga_priv {
        void __iomem *base;
        struct mutex fpga_lock;         // For FPGA internal lock
        void __iomem * fpga_read_addr;
};

extern void __iomem * fpga_ctl_addr;

static struct fpga_priv fpga_priv;
static struct fpga_priv *fpga = &fpga_priv;
/**
 * Show the value of the register set by 'set_fpga_reg_address'
 * If the address is not set by 'set_fpga_reg_address' first,
 * The version register is selected by default.
 * @param  buf     register value in hextring
 * @return         number of bytes read, or an error code
 */
static ssize_t get_fpga_reg_value(struct device *dev, struct device_attribute *devattr,
                                  char *buf)
{
    // read data from the address
    uint32_t data;

    data = ioread32(fpga->fpga_read_addr);
    return sprintf(buf, "0x%8.8x\n", data);
}
/**
 * Store the register address
 * @param  buf     address wanted to be read value of
 * @return         number of bytes stored, or an error code
 */
static ssize_t set_fpga_reg_address(struct device *dev, struct device_attribute *devattr,
                                    const char *buf, size_t count)
{
    uint32_t addr;
    char *last;

    addr = (uint32_t)strtoul(buf, &last, 16);
    if (addr == 0 && buf == last) {
        return -EINVAL;
    }
    fpga->fpga_read_addr = fpga->base + addr;
    return count;
}
/**
 * Show value of fpga scratch register
 * @param  buf     register value in hexstring
 * @return         number of bytes read, or an error code
 */
static ssize_t get_fpga_scratch(struct device *dev, struct device_attribute *devattr,
                                char *buf)
{
    return sprintf(buf, "0x%8.8x\n", ioread32(fpga->base + XILINX_FPGA_SCRATCH) & 0xffffffff);
}
/**
 * Store value of fpga scratch register
 * @param  buf     scratch register value passing from user space
 * @return         number of bytes stored, or an error code
 */
static ssize_t set_fpga_scratch(struct device *dev, struct device_attribute *devattr,
                                const char *buf, size_t count)
{
    uint32_t data;
    char *last;

    data = (uint32_t)strtoul(buf, &last, 16);
    if (data == 0 && buf == last) {
        return -EINVAL;
    }
    iowrite32(data, fpga->base + XILINX_FPGA_SCRATCH);
    return count;
}

/**
 * Show value of fpga version register
 * @param  buf     register value in hexstring
 * @return         number of bytes read, or an error code
 */
static ssize_t get_fpga_version(struct device *dev, struct device_attribute *devattr,
                                char *buf)
{
    return sprintf(buf, "0x%8.8x\n", ioread32(fpga->base + XILINX_FPGA_VERSION) & 0xffffffff);
}


/**
 * Store a value in a specific register address
 * @param  buf     the value and address in format '0xhhhh 0xhhhhhhhh'
 * @return         number of bytes sent by user space, or an error code
 */
#define ARG_SIZE 40
static ssize_t set_fpga_reg_value(struct device *dev, struct device_attribute *devattr,
                                  const char *buf, size_t count)
{
    // register are 4 bytes
    uint32_t addr;
    uint32_t value;
    uint32_t mode = 8;
    char *tok;
    char clone[ARG_SIZE];
    char *pclone = clone;
    char *last;

    if (count >= ARG_SIZE) {
        return -EOVERFLOW;
    }

    strscpy(clone, buf, ARG_SIZE);
    mutex_lock(&fpga->fpga_lock);
    tok = strsep((char**)&pclone, " ");
    if (tok == NULL) {
        mutex_unlock(&fpga->fpga_lock);
        return -EINVAL;
    }
    addr = (uint32_t)strtoul(tok, &last, 16);
    if (addr == 0 && tok == last) {
        mutex_unlock(&fpga->fpga_lock);
        return -EINVAL;
    }
    tok = strsep((char**)&pclone, " ");
    if (tok == NULL) {
        mutex_unlock(&fpga->fpga_lock);
        return -EINVAL;
    }
    value = (uint32_t)strtoul(tok, &last, 16);
    if (value == 0 && tok == last) {
        mutex_unlock(&fpga->fpga_lock);
        return -EINVAL;
    }
    tok = strsep((char**)&pclone, " ");
    if (tok == NULL) {
        mode = 32;
    } else {
        mode = (uint32_t)strtoul(tok, &last, 10);
        if (mode == 0 && tok == last) {
            mutex_unlock(&fpga->fpga_lock);
            return -EINVAL;
        }
    }
    if (mode == 32) {
        iowrite32(value, fpga->base + addr);
    } else if (mode == 8) {
        iowrite8(value, fpga->base + addr);
    } else {
        mutex_unlock(&fpga->fpga_lock);
        return -EINVAL;
    }
    mutex_unlock(&fpga->fpga_lock);
    return count;
}

/**
 * Read all FPGA  register in binary mode.
 * @param  buf   Raw transceivers port startus and control register values
 * @return       number of bytes read, or an error code
 */
static ssize_t dump_read(struct file *filp, struct kobject *kobj,
                         struct bin_attribute *attr, char *buf,
                         loff_t off, size_t count)
{
    unsigned long i = 0;
    ssize_t status;
    u8 read_reg;

    if ( off + count > XILINX_FPGA_REG_SIZE ) {
        return -EINVAL;
    }
    mutex_lock(&fpga->fpga_lock);
    while (i < count) {
        read_reg = ioread8(fpga->base + off + i);
        buf[i++] = read_reg;
    }
    status = count;
    mutex_unlock(&fpga->fpga_lock);
    return status;
}

static ssize_t TH5_max_temp_show(struct device *dev, struct device_attribute *attr,
                char *buf)
{
    u32 ready;  
    u32 data;
    s64 temp;
    ssize_t ret_val = 0;

    mutex_lock(&fpga->fpga_lock);
    data = ioread32(fpga->base + TH5_MAX_TEMP_REG);
    ready = ioread32(fpga->base + TH5_SDK_READY_REG);

    // return normal temp if invalid val, FSC will not shutdwon due to invalid val
    if (data == 0 || ((ready & 0x1) == 0x0)) {
        ret_val = strscpy(buf, "30\n", 4);
    } else {
        /* original calculation */
        /* temp = ((1000000000 / data / 40 / 2 - 1) * (-0.23734)) + 356.07; */
        temp = (((1000000000 / data / 40 / 2 - 1) * (-23734)) + 35607000) / 100;
        ret_val = sprintf(buf, "%lld\n", temp);
    }

    mutex_unlock(&fpga->fpga_lock);

    return ret_val;
}

static ssize_t TH5_max_temp_raw_show(struct device *dev, struct device_attribute *attr,
                char *buf)
{
    u32 data;

    mutex_lock(&fpga->fpga_lock);
    data = ioread32(fpga->base + TH5_MAX_TEMP_REG);
    mutex_unlock(&fpga->fpga_lock);
    return sprintf(buf, "0x%x\n", data);
}

static ssize_t TH5_min_temp_show(struct device *dev, struct device_attribute *attr,
                char *buf)
{   
    u32 data;
    s64 temp;
    ssize_t ret_val = 0;

    mutex_lock(&fpga->fpga_lock);
    data = ioread32(fpga->base + TH5_MIN_TEMP_REG);

    // protect deviding by 0
    if (data == 0) {
        ret_val = sprintf(buf, "invalid raw data\n");
    } else {
        /* original calculation */
        /* temp = ((1000000000 / data / 40 / 2 - 1) * (-0.23734)) + 356.07; */
        temp = (((1000000000 / data / 40 / 2 - 1) * (-23734)) + 35607000) / 100;
        ret_val = sprintf(buf, "%lld\n", temp);
    }

    mutex_unlock(&fpga->fpga_lock);

    return ret_val; 
}

static ssize_t TH5_min_temp_raw_show(struct device *dev, struct device_attribute *attr,
                char *buf)
{
    u32 data;

    mutex_lock(&fpga->fpga_lock);
    data = ioread32(fpga->base + TH5_MIN_TEMP_REG);
    mutex_unlock(&fpga->fpga_lock);
    return sprintf(buf, "0x%x\n", data);
}
/* FPGA attributes */
static DEVICE_ATTR( getreg, 0600, get_fpga_reg_value, set_fpga_reg_address);
static DEVICE_ATTR( setreg, 0200, NULL , set_fpga_reg_value);
static DEVICE_ATTR( scratch, 0600, get_fpga_scratch, set_fpga_scratch);
static DEVICE_ATTR( version, 0400, get_fpga_version, NULL);
static DEVICE_ATTR_RO(TH5_max_temp);
static DEVICE_ATTR_RO(TH5_max_temp_raw);
static DEVICE_ATTR_RO(TH5_min_temp);
static DEVICE_ATTR_RO(TH5_min_temp_raw);
static BIN_ATTR_RO( dump, XILINX_FPGA_REG_SIZE);

static struct bin_attribute *fpga_bin_attrs[] = {
    &bin_attr_dump,
    NULL,
};

static struct attribute *fpga_attrs[] = {
    &dev_attr_getreg.attr,
    &dev_attr_scratch.attr,
    &dev_attr_version.attr,
    &dev_attr_setreg.attr,
    &dev_attr_TH5_max_temp.attr,
    &dev_attr_TH5_max_temp_raw.attr,
    &dev_attr_TH5_min_temp.attr,
    &dev_attr_TH5_min_temp_raw.attr,
    NULL,
};

static struct attribute_group fpga_attr_grp = {
    .attrs = fpga_attrs,
    .bin_attrs = fpga_bin_attrs,
};


static int cls_fpga_probe(struct platform_device *pdev)
{
    struct kobject *fpga_obj;
    int ret = -ENOMEM;

    if (!fpga_ctl_addr){
        printk(KERN_WARNING "fpga_ctl_addr is null");
        return ret;
    }

    mutex_init(&fpga->fpga_lock);
    dev_set_drvdata(&pdev->dev, fpga);
    fpga->base = fpga_ctl_addr;

    printk(KERN_INFO "FPGA version: 0x%x\n", ioread32(fpga->base + XILINX_FPGA_VERSION));
    
    fpga_obj = kobject_create_and_add("FPGA", &pdev->dev.kobj);
    if (!fpga_obj) {
        printk(KERN_ERR "Cannot create FPGA system sysfs attributes\n");		
        ret = -ENOMEM;
        goto err_exit;
    }   

    ret = sysfs_create_group(fpga_obj, &fpga_attr_grp);
    if (ret != 0) {
        printk(KERN_ERR "Cannot create FPGA attr system sysfs attributes\n");
        goto err_remove_fpga;
    }

    return 0;

err_remove_fpga:
    sysfs_remove_group(&pdev->dev.kobj, &fpga_attr_grp);
err_exit:
    return ret;
}

static int cls_fpga_remove(struct platform_device *pdev)
{
    sysfs_remove_group(&pdev->dev.kobj, &fpga_attr_grp);
    iounmap(fpga->base);
    return 0;
}

static void fpga_dev_release( struct device * dev)
{
    return;
}
static struct resource cls_fpga_resources[] = {
    {
        .start = 0,
        .end = 0,
        .flags = IORESOURCE_IO,
    },
};

static struct platform_device cls_fpga_dev = {
    .name           = DRV_NAME,
    .id             = -1,
    .num_resources  = ARRAY_SIZE(cls_fpga_resources),
    .resource       = cls_fpga_resources,
    .dev = {
        .release = fpga_dev_release,
    }
};

static struct platform_driver cls_fpga_driver = {
    .probe = cls_fpga_probe,
    .remove = cls_fpga_remove,
    .driver = {
            .name = DRV_NAME,
    },
};

static int __init drv_init(void)
{
    int rc = 0;

    rc = platform_device_register(&cls_fpga_dev);
    rc += platform_driver_register(&cls_fpga_driver);
    printk("fpga drv_init:%d\n", rc);
    return rc;
}

static void __exit drv_exit(void)
{
    platform_driver_unregister(&cls_fpga_driver);
    platform_device_unregister(&cls_fpga_dev);
    printk("fpga drv_exit.\n");
}

module_init(drv_init);
module_exit(drv_exit);

MODULE_AUTHOR("Nicholas Wu<nicwu@celestica.com>");
MODULE_DESCRIPTION("Celestica fpga control driver");
MODULE_VERSION("2.0.2");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:cls-fpga");
