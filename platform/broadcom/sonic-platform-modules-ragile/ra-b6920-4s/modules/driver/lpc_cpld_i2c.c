#include <linux/kernel.h> /* Wd're doing kernel work */  
#include <linux/module.h> /* specifically, a module */  
#include <linux/types.h>
#include <linux/init.h>   /* Need for the macros */
#include <linux/moduleparam.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/sched.h>
#include <net/sock.h>
#include <net/genetlink.h>
#include <linux/netlink.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/mfd/core.h>
#include "../../../common/modules/lpc_cpld_i2c_ocores.h"
#include "ragile.h"

int lpc_cpld_i2c_verbose = 0;
int lpc_cpld_i2c_error = 0;
module_param(lpc_cpld_i2c_verbose, int, S_IRUGO | S_IWUSR);
module_param(lpc_cpld_i2c_error, int, S_IRUGO | S_IWUSR);


#define LPC_CPLD_I2C_VERBOSE(fmt, args...) do {                                        \
    if (lpc_cpld_i2c_verbose) { \
        printk(KERN_ERR "[LPC_CPLD_I2C_DEVICE][VERBOSE][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define LPC_CPLD_I2C_ERROR(fmt, args...) do {                                        \
        if (lpc_cpld_i2c_error) { \
            printk(KERN_ERR "[LPC_CPLD_I2C_DEVICE][ERROR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
        } \
    } while (0)

#define PCI_VENDOR_ID_D1527_LPC             (0x8c54)
#define PCI_VENDOR_ID_C3000_LPC             (0x19dc)

#define LPC_CPLD_I2C_OCORE_START_BASE               (0x800)
#define MAX_LPC_CPLD_I2C_REG_SIZE               (0x8)

#define LPC_CPLD_I2C_OCORE_CTRL_START(id)           ((LPC_CPLD_I2C_OCORE_START_BASE) + (id) * (MAX_LPC_CPLD_I2C_REG_SIZE))
#define LPC_CPLD_I2C_OCORE_CTRL_END(id)             ((LPC_CPLD_I2C_OCORE_START_BASE) + (id + 1) * (MAX_LPC_CPLD_I2C_REG_SIZE) - 1)

static struct rg_ocores_cpld_i2c_platform_data rg_i2c_ocore_pdata = {
    .reg_shift = 0,
    .reg_io_width = 1,
    .clock_khz = 33000,
    .num_devices = 0,
    .i2c_irq_flag = 1,
};

#define DEFINE_LPC_CPLD_I2C_DEVICE_RESOURCES(_id) \
    static struct resource lpc_cpld_i2c_resources_##_id[] = {      \
    {     \
        .start  = LPC_CPLD_I2C_OCORE_CTRL_START(_id),     \
        .end    = LPC_CPLD_I2C_OCORE_CTRL_END(_id),       \
        .flags  = IORESOURCE_IO,    \
    },      \
}

DEFINE_LPC_CPLD_I2C_DEVICE_RESOURCES(52);
DEFINE_LPC_CPLD_I2C_DEVICE_RESOURCES(48);
DEFINE_LPC_CPLD_I2C_DEVICE_RESOURCES(49);
DEFINE_LPC_CPLD_I2C_DEVICE_RESOURCES(50);
DEFINE_LPC_CPLD_I2C_DEVICE_RESOURCES(51);

#define DEFINE_LPC_CPLD_I2C_MFD_CELL_CFG(_id)                                   \
    {                                                                       \
        .name = "rg-cpld-ocrore-i2c",                                            \
        .id            = (_id),                                                \
        .num_resources = ARRAY_SIZE(lpc_cpld_i2c_resources_##_id),   \
        .resources = lpc_cpld_i2c_resources_##_id,                   \
        .platform_data = &rg_i2c_ocore_pdata,                               \
        .pdata_size = sizeof(rg_i2c_ocore_pdata),                           \
    }

static const struct mfd_cell lpc_cpld_i2c_cells_bar0_cfg0[] = {
    DEFINE_LPC_CPLD_I2C_MFD_CELL_CFG(52),
    DEFINE_LPC_CPLD_I2C_MFD_CELL_CFG(48),
    DEFINE_LPC_CPLD_I2C_MFD_CELL_CFG(49),
    DEFINE_LPC_CPLD_I2C_MFD_CELL_CFG(50),
    DEFINE_LPC_CPLD_I2C_MFD_CELL_CFG(51),
};

static int __init lpc_cpld_i2c_init(void)
{
    struct pci_dev *pdev = NULL;
    int ret;
    
    LPC_CPLD_I2C_VERBOSE("Enter.\n");

    pdev = pci_get_device(PCI_VENDOR_ID_INTEL, PCI_VENDOR_ID_D1527_LPC, pdev);
    if (!pdev) {
        LPC_CPLD_I2C_ERROR("pci_get_device(0x8086, 0x8c54) failed!\n");
        return 0;
    }

    ret = mfd_add_devices(&pdev->dev, 0,
                lpc_cpld_i2c_cells_bar0_cfg0,
                ARRAY_SIZE(lpc_cpld_i2c_cells_bar0_cfg0),
                NULL, 0, NULL);
    if (ret) {
        LPC_CPLD_I2C_ERROR("mfd_add_devices failed: %d\n", ret);
        return -1;
    }
    LPC_CPLD_I2C_VERBOSE("Leave success\n");
    return ret;
}

static void __exit lpc_cpld_i2c_exit(void)
{
    struct pci_dev *pdev = NULL;

    pdev = pci_get_device(PCI_VENDOR_ID_INTEL, PCI_VENDOR_ID_D1527_LPC, pdev);
    if (!pdev) {
        LPC_CPLD_I2C_ERROR("pci_get_device(0x8086, 0x8c54) failed!\n");
        return ;
    }

    mfd_remove_devices(&pdev->dev);
    LPC_CPLD_I2C_VERBOSE("Leave.\n");
}

module_init(lpc_cpld_i2c_init);
module_exit(lpc_cpld_i2c_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support <support@ragile.com>");
