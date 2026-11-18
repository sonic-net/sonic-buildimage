#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <wb_pcie_dev.h>
#include <wb_bsp_kernel_debug.h>

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

static pci_dev_device_t pcie_dev_device_data0 = {
    .pci_dev_name = "fpga0",
    .pci_domain = 0x0000,
    .pci_bus = 0x0a,
    .pci_slot = 0x00,
    .pci_fn = 0,
    .pci_bar = 0,
    .bus_width = 4,
    .search_mode = 1,
    .bridge_bus = 0,
    .bridge_slot = 0x03,
    .bridge_fn = 3,
};

static pci_dev_device_t pcie_dev_device_data1 = {
    .pci_dev_name = "fpga1",
    .pci_domain = 0x0000,
    .pci_bus = 8,
    .pci_slot = 0x00,
    .pci_fn = 0,
    .pci_bar = 0,
    .bus_width = 4,
    .search_mode = 1,
    .bridge_bus = 0,
    .bridge_slot = 0x03,
    .bridge_fn = 1,
};

static pci_dev_device_t pcie_dev_device_data2 = {
    .pci_dev_name = "fpga2",
    .pci_domain = 0x0000,
    .pci_bus = 9,
    .pci_slot = 0x00,
    .pci_fn = 0,
    .pci_bar = 0,
    .bus_width = 4,
    .search_mode = 1,
    .bridge_bus = 0,
    .bridge_slot = 0x03,
    .bridge_fn = 2,
};


static void wb_pcie_dev_device_release(struct device *dev)
{
    return;
}

static struct platform_device pcie_dev_device[] = {
    {
        .name   = "wb-pci-dev",
        .id = 1,
        .dev    = {
            .platform_data  = &pcie_dev_device_data0,
            .release = wb_pcie_dev_device_release,
        },
    },
    {
        .name   = "wb-pci-dev",
        .id = 2,
        .dev    = {
            .platform_data  = &pcie_dev_device_data1,
            .release = wb_pcie_dev_device_release,
        },
    },
    {
        .name   = "wb-pci-dev",
        .id = 3,
        .dev    = {
            .platform_data  = &pcie_dev_device_data2,
            .release = wb_pcie_dev_device_release,
        },
    },
};

static int __init wb_pcie_dev_device_init(void)
{
    int i;
    int ret = 0;
    pci_dev_device_t *pcie_dev_device_data;

    DEBUG_VERBOSE("enter!\n");
    for (i = 0; i < ARRAY_SIZE(pcie_dev_device); i++) {
        pcie_dev_device_data = pcie_dev_device[i].dev.platform_data;
        ret = platform_device_register(&pcie_dev_device[i]);
        if (ret < 0) {
            pcie_dev_device_data->device_flag = -1; /* device register failed, set flag -1 */
            printk(KERN_ERR "wb-pci-dev.%d register failed!\n", i + 1);
        } else {
            pcie_dev_device_data->device_flag = 0; /* device register suucess, set flag 0 */
        }
    }
    return 0;
}

static void __exit wb_pcie_dev_device_exit(void)
{
    int i;
    pci_dev_device_t *pcie_dev_device_data;

    DEBUG_VERBOSE("enter!\n");
    for (i = ARRAY_SIZE(pcie_dev_device) - 1; i >= 0; i--) {
        pcie_dev_device_data = pcie_dev_device[i].dev.platform_data;
        if (pcie_dev_device_data->device_flag == 0) { /* device register success, need unregister */
            platform_device_unregister(&pcie_dev_device[i]);
        }
    }
}

module_init(wb_pcie_dev_device_init);
module_exit(wb_pcie_dev_device_exit);
MODULE_DESCRIPTION("PCIE DEV Devices");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
