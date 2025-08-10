/*
 * wb_lpc_drv.c
 * ko to set lpc pcie config io addr and enable lpc
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/pci.h>
#include <linux/io.h>
#include <linux/ioport.h>

#include "wb_lpc_drv.h"

#define LPC_DRIVER_NAME                    "wb-lpc"
#define LPC_MAKE_PCI_IO_RANGE(__base)      ((0xfc0001) | ((__base) & (0xFFFC)))

int g_lpc_dev_debug = 0;
int g_lpc_dev_error = 0;

module_param(g_lpc_dev_debug, int, S_IRUGO | S_IWUSR);
module_param(g_lpc_dev_error, int, S_IRUGO | S_IWUSR);

#define LPC_DEV_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_lpc_dev_debug) { \
        printk(KERN_INFO "[LPC_DEV][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define LPC_DEV_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_lpc_dev_error) { \
        printk(KERN_ERR "[LPC_DEV][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

typedef struct wb_lpc_dev_s {
    const char *lpc_io_name;
    uint32_t domain;
    uint32_t bus;
    uint32_t slot;
    uint32_t fn;
    uint32_t lpc_io_base;
    uint32_t lpc_io_size;
    uint32_t lpc_gen_dec;
} wb_lpc_dev_t;

static int wb_lpc_probe(struct platform_device *pdev)
{
    int ret, devfn;
    wb_lpc_dev_t *wb_lpc_dev;
    struct pci_dev *pci_dev;
    lpc_drv_device_t *lpc_drv_device;

    wb_lpc_dev = devm_kzalloc(&pdev->dev, sizeof(wb_lpc_dev_t), GFP_KERNEL);
    if (!wb_lpc_dev) {
        dev_err(&pdev->dev, "devm_kzalloc failed.\n");
        ret = -ENOMEM;
        return ret;
    }

    if (pdev->dev.of_node) {
        ret = 0;
        ret += of_property_read_string(pdev->dev.of_node, "lpc_io_name", &wb_lpc_dev->lpc_io_name);
        ret += of_property_read_u32(pdev->dev.of_node, "pci_domain", &wb_lpc_dev->domain);
        ret += of_property_read_u32(pdev->dev.of_node, "pci_bus", &wb_lpc_dev->bus);
        ret += of_property_read_u32(pdev->dev.of_node, "pci_slot", &wb_lpc_dev->slot);
        ret += of_property_read_u32(pdev->dev.of_node, "pci_fn", &wb_lpc_dev->fn);
        ret += of_property_read_u32(pdev->dev.of_node, "lpc_io_base", &wb_lpc_dev->lpc_io_base);
        ret += of_property_read_u32(pdev->dev.of_node, "lpc_io_size", &wb_lpc_dev->lpc_io_size);
        ret += of_property_read_u32(pdev->dev.of_node, "lpc_gen_dec", &wb_lpc_dev->lpc_gen_dec);
        if (ret != 0) {
            dev_err(&pdev->dev, "Failed to get dts config, ret:%d.\n", ret);
            return -ENXIO;
        }
    } else {
        if (pdev->dev.platform_data == NULL) {
            dev_err(&pdev->dev, "Failed to get platform data config.\n");
            return -ENXIO;
        }
        lpc_drv_device = pdev->dev.platform_data;
        wb_lpc_dev->lpc_io_name = lpc_drv_device->lpc_io_name;
        wb_lpc_dev->domain = lpc_drv_device->pci_domain;
        wb_lpc_dev->bus = lpc_drv_device->pci_bus;
        wb_lpc_dev->slot = lpc_drv_device->pci_slot;
        wb_lpc_dev->fn = lpc_drv_device->pci_fn;
        wb_lpc_dev->lpc_io_base = lpc_drv_device->lpc_io_base;
        wb_lpc_dev->lpc_io_size = lpc_drv_device->lpc_io_size;
        wb_lpc_dev->lpc_gen_dec = lpc_drv_device->lpc_gen_dec;
    }

    LPC_DEV_DEBUG_VERBOSE("domain:0x%04x, bus:0x%02x, slot:0x%02x, fn:%u\n",
        wb_lpc_dev->domain,wb_lpc_dev->bus, wb_lpc_dev->slot, wb_lpc_dev->fn);
    LPC_DEV_DEBUG_VERBOSE("lpc_io_name:%s, lpc_io_base:0x%x, lpc_io_size:%u, lpc_gen_dec:0x%x.\n",
        wb_lpc_dev->lpc_io_name, wb_lpc_dev->lpc_io_base, wb_lpc_dev->lpc_io_size, wb_lpc_dev->lpc_gen_dec);

    devfn = PCI_DEVFN(wb_lpc_dev->slot, wb_lpc_dev->fn);
    pci_dev = pci_get_domain_bus_and_slot(wb_lpc_dev->domain, wb_lpc_dev->bus, devfn);
    if (pci_dev == NULL) {
        dev_err(&pdev->dev, "Failed to find pci_dev, domain:0x%04x, bus:0x%02x, devfn:0x%x\n",
            wb_lpc_dev->domain, wb_lpc_dev->bus, devfn);
        return -ENXIO;
    }

    pci_write_config_dword(pci_dev, wb_lpc_dev->lpc_gen_dec, LPC_MAKE_PCI_IO_RANGE(wb_lpc_dev->lpc_io_base));
    if (!request_region(wb_lpc_dev->lpc_io_base, wb_lpc_dev->lpc_io_size, wb_lpc_dev->lpc_io_name)) {
        dev_err(&pdev->dev, "Failed to request_region [0x%x][0x%x].\n", wb_lpc_dev->lpc_io_base, wb_lpc_dev->lpc_io_size);
        return -EBUSY;
    }

    platform_set_drvdata(pdev, wb_lpc_dev);

    dev_info(&pdev->dev, "lpc request_region [0x%x][0x%x] success.\n", wb_lpc_dev->lpc_io_base, wb_lpc_dev->lpc_io_size);

    return 0;
}

static int wb_lpc_remove(struct platform_device *pdev)
{
    wb_lpc_dev_t *wb_lpc_dev;

    wb_lpc_dev = platform_get_drvdata(pdev);
    if (wb_lpc_dev) {
        release_region(wb_lpc_dev->lpc_io_base , wb_lpc_dev->lpc_io_size);
        LPC_DEV_DEBUG_VERBOSE("lpc base:0x%x, len:0x%x.\n", wb_lpc_dev->lpc_io_base, wb_lpc_dev->lpc_io_size);
    }
    LPC_DEV_DEBUG_VERBOSE("lpc remove.\n");

    return 0;
}

static struct of_device_id lpc_dev_match[] = {
    {
        .compatible = "wb-lpc",
    },
    {},
};
MODULE_DEVICE_TABLE(of, lpc_dev_match);

static struct platform_driver wb_lpc_driver = {
    .probe      = wb_lpc_probe,
    .remove     = wb_lpc_remove,
    .driver     = {
        .owner  = THIS_MODULE,
        .name   = LPC_DRIVER_NAME,
        .of_match_table = lpc_dev_match,
    },
};

static int __init wb_lpc_init(void)
{
    return platform_driver_register(&wb_lpc_driver);
}

static void __exit wb_lpc_exit(void)
{
    platform_driver_unregister(&wb_lpc_driver);
}

module_init(wb_lpc_init);
module_exit(wb_lpc_exit);
MODULE_DESCRIPTION("lpc driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
