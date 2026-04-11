/*
 * wb_fpga_pcie.c
 * ko to enable fpga pcie
 */
#include <linux/module.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/pci.h>
#include <wb_bsp_kernel_debug.h>

#define FPGA_MSI_IRQ_BEGIN          (0)
#define FPGA_MSI_IRQ_MAX            (32)
#define FPGA_MSI_IRQ_MIN            (16)
#define XILINX_FPGA_USE_MSI         (0)
#define XILINX_FPGA_NUSE_MSI        (1)

static int debug = 0;
module_param(debug, int, S_IRUGO | S_IWUSR);

typedef struct wb_fpga_pcie_s {
    struct pci_dev *pci_dev;
    int driver_data;
} wb_fpga_pcie_t;

static void fpga_pcie_recover(struct pci_dev *pdev, const struct pci_device_id *id)
{
    struct resource *mem_base;
    u32 bar0_val;
    int ret;

    mem_base = &pdev->resource[0];
    bar0_val = 0;
    ret = pci_read_config_dword(pdev, PCI_BASE_ADDRESS_0, &bar0_val);
    if (ret) {
        DEBUG_ERROR("pci_read_config_dword failed ret %d.\n", ret);
        return;
    }
    DEBUG_VERBOSE("mem_base->start[0x%llx], bar0_val[0x%x], ret %d.\n",
        mem_base->start, bar0_val, ret);

    if (bar0_val != mem_base->start) {
        ret = pci_write_config_dword(pdev, PCI_BASE_ADDRESS_0, mem_base->start);
        if (ret) {
            DEBUG_ERROR("pci_write_config_dword mem_base->start[0x%llx], failed ret %d.\n", mem_base->start, ret);
            return;
        }
        DEBUG_VERBOSE("pci_write_config_dword mem_base->start[0x%llx] success.\n", mem_base->start);
    } else {
        DEBUG_VERBOSE("mem_base->start[0x%llx], bar0_val[0x%x], do nothing.\n",
            mem_base->start, bar0_val);
    }
}

static int fpga_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
    int err;
    wb_fpga_pcie_t *wb_fpga_pcie;

    DEBUG_VERBOSE("Enter vendor 0x%x, device 0x%x.\n", pdev->vendor, pdev->device);

    wb_fpga_pcie = devm_kzalloc(&pdev->dev, sizeof(wb_fpga_pcie_t), GFP_KERNEL);
    if (!wb_fpga_pcie) {
        dev_err(&pdev->dev, "devm_kzalloc failed.\n");
        return -ENOMEM;
    }

    fpga_pcie_recover(pdev, id);

    /* enable device: ask low-level code to enable I/O and memory */
    DEBUG_VERBOSE("start pci_enable_device!\n");
    err = pci_enable_device(pdev);
    if (err) {
        dev_err(&pdev->dev, "Failed to enable pci device, ret:%d.\n", err);
        return err;
    }

    DEBUG_VERBOSE("start pci_set_master!\n");
    pci_set_master(pdev);

    wb_fpga_pcie->driver_data = id->driver_data;
    wb_fpga_pcie->pci_dev = pdev;
    pci_set_drvdata(pdev, wb_fpga_pcie);

    if (wb_fpga_pcie->driver_data == XILINX_FPGA_USE_MSI) {
        DEBUG_VERBOSE("start pci_enable_msi_range!\n");
#if LINUX_VERSION_CODE < KERNEL_VERSION(4,19,152)
        err = pci_enable_msi_range(pdev, FPGA_MSI_IRQ_BEGIN + 1, FPGA_MSI_IRQ_MAX);
#else
        err = pci_alloc_irq_vectors_affinity(pdev, FPGA_MSI_IRQ_BEGIN + 1,
                FPGA_MSI_IRQ_MAX, PCI_IRQ_MSI, NULL);
#endif
        if ((err > FPGA_MSI_IRQ_MAX) || (err < FPGA_MSI_IRQ_MIN)) {
            DEBUG_ERROR("pci_enable_msi_block err %d FPGA_MSI_IRQ_NUM %d.\n", err,
                FPGA_MSI_IRQ_MAX);
            dev_err(&pdev->dev, "Failed to enable pci msi, ret:%d.\n", err);
            return -EINVAL;
        }
        DEBUG_VERBOSE("pci_enable_msi success, ret: %d\n", err);
    }

    dev_info(&pdev->dev, "fpga pci device init success.\n");
    return 0;
}

static void fpga_pcie_remove(struct pci_dev *pdev)
{
    wb_fpga_pcie_t *wb_fpga_pcie;

    DEBUG_VERBOSE("fpga_pcie_remove.\n");

    wb_fpga_pcie = pci_get_drvdata(pdev);
    if (wb_fpga_pcie->driver_data == XILINX_FPGA_USE_MSI) {
        DEBUG_VERBOSE("start pci_disable_msi!\n");
        pci_disable_msi(pdev);
    }

    pci_disable_device(pdev);
    return;
}

static const struct pci_device_id fpga_pci_ids[] = {
        { PCI_DEVICE(0x10ee, 0x7022), .driver_data = XILINX_FPGA_USE_MSI},
        { PCI_DEVICE(0x10ee, 0x7011), .driver_data = XILINX_FPGA_USE_MSI},
        { PCI_DEVICE(0x1ded, 0x7022), .driver_data = XILINX_FPGA_USE_MSI},
        { PCI_DEVICE(0x1ded, 0x7021), .driver_data = XILINX_FPGA_USE_MSI},
        { PCI_DEVICE(0x1ded, 0x5220), .driver_data = XILINX_FPGA_USE_MSI},
        {0}
};
MODULE_DEVICE_TABLE(pci, fpga_pci_ids);

static struct pci_driver wb_fpga_pcie_driver = {
    .name = "wb_fpga_pcie",
    .id_table = fpga_pci_ids,/* only dynamic id's */
    .probe = fpga_pcie_probe,
    .remove = fpga_pcie_remove,
};

static int __init wb_fpga_pcie_init(void)
{

    DEBUG_VERBOSE("wb_fpga_pcie_init enter!\n");
    return pci_register_driver(&wb_fpga_pcie_driver);
}

static void __exit wb_fpga_pcie_exit(void)
{
    DEBUG_VERBOSE("wb_fpga_pcie_exit enter!\n");
    pci_unregister_driver(&wb_fpga_pcie_driver);
    return;
}

module_init(wb_fpga_pcie_init);
module_exit(wb_fpga_pcie_exit);
MODULE_DESCRIPTION("fpga pcie driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
