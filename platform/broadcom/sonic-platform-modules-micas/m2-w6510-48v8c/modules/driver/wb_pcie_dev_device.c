#include <linux/module.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>

#include <wb_pcie_dev.h>

static int g_wb_pcie_dev_device_debug = 0;
static int g_wb_pcie_dev_device_error = 0;

module_param(g_wb_pcie_dev_device_debug, int, S_IRUGO | S_IWUSR);
module_param(g_wb_pcie_dev_device_error, int, S_IRUGO | S_IWUSR);

#define WB_PCIE_DEV_DEVICE_DEBUG_VERBOSE(fmt, args...) do {                                        \
    if (g_wb_pcie_dev_device_debug) { \
        printk(KERN_INFO "[WB_PCIE_DEV_DEVICE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define WB_PCIE_DEV_DEVICE_DEBUG_ERROR(fmt, args...) do {                                        \
    if (g_wb_pcie_dev_device_error) { \
        printk(KERN_ERR "[WB_PCIE_DEV_DEVICE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

static pci_dev_device_t pcie_dev_device_data0 = {
    .pci_dev_name = "fpga0",
    .pci_domain = 0x0000,
    .pci_bus = 0x08,
    .pci_slot = 0x00,
    .pci_fn = 0,
    .pci_bar = 0,
    .bus_width = 4,
    .upg_ctrl_base = 0xa00,
    .upg_flash_base = 0x1a0000,
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
};

static int __init wb_pcie_dev_device_init(void)
{
    int i;
    int ret = 0;
    pci_dev_device_t *pcie_dev_device_data;

    WB_PCIE_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");
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

    WB_PCIE_DEV_DEVICE_DEBUG_VERBOSE("enter!\n");
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
