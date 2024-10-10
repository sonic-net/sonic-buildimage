/*
 * Copyright (C)  Roger Ho <roger530_ho@edge-core.com>
 *
 * This module provides support for accessing 
 * QSFP-DD/SFP eeprom by I2C.
 *
 * This includes the:
 *     Accton as9737_32db FPGA I2C
 *
 * Copyright (C) 2017 Finisar Corp.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/version.h>
#include <linux/pci.h>
#include <linux/platform_device.h>
#include <linux/platform_data/i2c-ocores.h>

/***********************************************
 *       variable define
 * *********************************************/
#define DRVNAME                        "as9737_32db_fpga_i2c"
#define OCORES_I2C_DRVNAME             "ocores-i2c"

#define PORT_NUM                       (32 + 2)  /* 32 QSFPDDs + 1 SFP+ + 1 SFP */

/*
 * PCIE BAR0 address
 */
#define BAR0_NUM                       0
#define FPGA_PCI_VENDOR_ID             0x1172
#define FPGA_PCI_DEVICE_ID             0x0004

#define CPLD2 0x01
#define CPLD3 0x02

/***********************************************
 *       macro define
 * *********************************************/
#define pcie_err(fmt, args...) \
        printk(KERN_ERR "["DRVNAME"]: " fmt " ", ##args)

#define pcie_info(fmt, args...) \
        printk(KERN_ERR "["DRVNAME"]: " fmt " ", ##args)

#define DEFINE_RES_REG_NAMED(_start, _size, _name)    \
    DEFINE_RES_NAMED((_start), (_size), (_name), IORESOURCE_REG)

/***********************************************
 *       structure & variable declare
 * *********************************************/
typedef struct pci_fpga_device_s {
    struct pci_dev  *pci_dev;
    struct platform_device *fpga_i2c[PORT_NUM];
} pci_fpga_device_t;

static struct platform_device *pdev = NULL;

/***********************************************
 *       enum define
 * *********************************************/


/***********************************************
 *       function declare
 * *********************************************/

#if 0
static inline unsigned int fpga_read(const void __iomem *addr)
{
    return ioread8(addr);
}

static inline void fpga_write(void __iomem *addr, u8 val)
{
    iowrite8(val, addr);
}
#endif

struct port_data {
    u16 offset;
    u16 channel;
    u16 cpld; /* 0x01 --> CPLD2, 0x02 --> CPLD3 */
};

/* ============PCIe Bar Offset to I2C Master Mapping============== */
#ifdef HAS_CHANNEL_REG
static const struct port_data port[PORT_NUM]= {
    {0x00, 0x00, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port1 */
    {0x00, 0x01, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port2 */
    {0x00, 0x02, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port3 */
    {0x00, 0x03, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port4 */
    {0x00, 0x04, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port5 */
    {0x00, 0x05, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port6 */
    {0x00, 0x06, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port7 */
    {0x00, 0x07, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port8 */
    {0x00, 0x08, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port9 */
    {0x00, 0x09, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port10 */
    {0x00, 0x0a, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port11 */
    {0x00, 0x0b, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port12 */
    {0x00, 0x0c, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port13 */
    {0x00, 0x0d, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port14 */
    {0x00, 0x0e, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port15 */
    {0x00, 0x0f, CPLD2},/* 0x00 - 0x05  CPLD2 I2C Master Port16 */
    {0x00, 0x10, CPLD3},/* 0x00 - 0x05  CPLD2 I2C Master Port17 */
    {0x00, 0x11, CPLD3},/* 0x00 - 0x05  CPLD3 I2C Master Port18 */
    {0x00, 0x12, CPLD3},/* 0x00 - 0x05  CPLD3 I2C Master Port19 */
    {0x00, 0x13, CPLD3},/* 0x00 - 0x05  CPLD3 I2C Master Port20 */
    {0x00, 0x14, CPLD3},/* 0x00 - 0x05  CPLD3 I2C Master Port21 */
    {0x00, 0x15, CPLD3},/* 0x00 - 0x05  CPLD3 I2C Master Port22 */
    {0x00, 0x16, CPLD3},/* 0x00 - 0x05  CPLD3 I2C Master Port23 */
    {0x00, 0x17, CPLD3},/* 0x00 - 0x05  CPLD3 I2C Master Port24 */
    {0x00, 0x18, CPLD3},/* 0x00 - 0x05  CPLD3 I2C Master Port25 */
    {0x00, 0x19, CPLD3},/* 0x00 - 0x05  CPLD3 I2C Master Port26 */
    {0x00, 0x1a, CPLD3},/* 0x00 - 0x05  CPLD3 I2C Master Port27 */
    {0x00, 0x1b, CPLD3},/* 0x00 - 0x05  CPLD3 I2C Master Port28 */
    {0x00, 0x1c, CPLD3},/* 0x00 - 0x05  CPLD3 I2C Master Port29 */
    {0x00, 0x1d, CPLD3},/* 0x00 - 0x05  CPLD3 I2C Master Port30 */
    {0x00, 0x1e, CPLD3},/* 0x00 - 0x05  CPLD3 I2C Master Port31 */
    {0x00, 0x1f, CPLD3},/* 0x00 - 0x05  CPLD3 I2C Master Port32 */
    {0x00, 0x20, CPLD2},/* 0x00 - 0x05  CPLD3 I2C Master Port33 */
    {0x00, 0x21, CPLD2},/* 0x00 - 0x05  CPLD3 I2C Master Port34 */
};
#else
static const struct port_data port[PORT_NUM]= {
    {0x0000, 0x00, CPLD2},/* 0x0000 - 0x0010  CPLD2 I2C Master Port1 */
    {0x0014, 0x00, CPLD2},/* 0x0014 - 0x0024  CPLD2 I2C Master Port2 */
    {0x0028, 0x00, CPLD2},/* 0x0028 - 0x0038  CPLD2 I2C Master Port3 */
    {0x003C, 0x00, CPLD2},/* 0x003c - 0x004c  CPLD2 I2C Master Port4 */
    {0x0050, 0x00, CPLD2},/* 0x0050 - 0x0060  CPLD2 I2C Master Port5 */
    {0x0064, 0x00, CPLD2},/* 0x0064 - 0x0074  CPLD2 I2C Master Port6 */
    {0x0078, 0x00, CPLD2},/* 0x0078 - 0x0088  CPLD2 I2C Master Port7 */
    {0x008C, 0x00, CPLD2},/* 0x008c - 0x009c  CPLD2 I2C Master Port8 */
    {0x00A0, 0x00, CPLD2},/* 0x00a0 - 0x00b0  CPLD2 I2C Master Port9 */
    {0x00B4, 0x00, CPLD2},/* 0x00b4 - 0x00c4  CPLD2 I2C Master Port10*/
    {0x00C8, 0x00, CPLD2},/* 0x00c8 - 0x00d8  CPLD2 I2C Master Port11 */
    {0x00DC, 0x00, CPLD2},/* 0x00dc - 0x00ec  CPLD2 I2C Master Port12 */
    {0x00F0, 0x00, CPLD2},/* 0x00f0 - 0x0100  CPLD2 I2C Master Port13 */
    {0x0104, 0x00, CPLD2},/* 0x0104 - 0x0114  CPLD2 I2C Master Port14 */
    {0x0118, 0x00, CPLD2},/* 0x0118 - 0x0128  CPLD2 I2C Master Port15 */
    {0x012C, 0x00, CPLD2},/* 0x012c - 0x013c  CPLD2 I2C Master Port16 */
    {0x0140, 0x00, CPLD3},/* 0x0140 - 0x0150  CPLD2 I2C Master Port17 */
    {0x0154, 0x00, CPLD3},/* 0x0154 - 0x0164  CPLD3 I2C Master Port18 */
    {0x0168, 0x00, CPLD3},/* 0x0168 - 0x0178  CPLD3 I2C Master Port19 */
    {0x017C, 0x00, CPLD3},/* 0x017c - 0x018c  CPLD3 I2C Master Port20 */
    {0x0190, 0x00, CPLD3},/* 0x0190 - 0x01a0  CPLD3 I2C Master Port21 */
    {0x01A4, 0x00, CPLD3},/* 0x01a4 - 0x01b4  CPLD3 I2C Master Port22 */
    {0x01B8, 0x00, CPLD3},/* 0x01b8 - 0x01c8  CPLD3 I2C Master Port23 */
    {0x01CC, 0x00, CPLD3},/* 0x01cc - 0x01dc  CPLD3 I2C Master Port24 */
    {0x01E0, 0x00, CPLD3},/* 0x01e0 - 0x01f0  CPLD3 I2C Master Port25 */
    {0x01F4, 0x00, CPLD3},/* 0x01f4 - 0x0204  CPLD3 I2C Master Port26 */
    {0x0208, 0x00, CPLD3},/* 0x0208 - 0x0218  CPLD3 I2C Master Port27 */
    {0x021C, 0x00, CPLD3},/* 0x021c - 0x022c  CPLD3 I2C Master Port28 */
    {0x0230, 0x00, CPLD3},/* 0x0230 - 0x0240  CPLD3 I2C Master Port29 */
    {0x0244, 0x00, CPLD3},/* 0x0244 - 0x0254  CPLD3 I2C Master Port30 */
    {0x0258, 0x00, CPLD3},/* 0x0258 - 0x0268  CPLD3 I2C Master Port31 */
    {0x026C, 0x00, CPLD3},/* 0x026c - 0x027c  CPLD3 I2C Master Port32 */
    {0x0280, 0x00, CPLD2},/* 0x0280 - 0x0290  CPLD3 I2C Master Port33 */
    {0x0294, 0x00, CPLD2},/* 0x0294 - 0x02a4  CPLD3 I2C Master Port34 */
};
#endif

static struct ocores_i2c_platform_data as9737_32db_platform_data = {
    .reg_io_width = 1,
    .reg_shift = 2,
    /*
     * PRER_L and PRER_H are calculated based on clock_khz and bus_khz 
     * in i2c-ocores.c:ocores_init.
     */
#if 1
    /* SCL 100KHZ in FPGA spec. => PRER_L = 0x6D, PRER_H = 0x00 */
    .clock_khz = 55000,
    .bus_khz = 100,
#else
    /* SCL 400KHZ in FPGA spec. => PRER_L = 0x6D, PRER_H = 0x00 */
    .clock_khz = 220000,
    .bus_khz = 400,
#endif
};

struct platform_device *ocore_i2c_device_add(unsigned int id, unsigned long bar_base,
                                             const struct port_data port)
{
    struct platform_device *pdev;
    int err;
    struct resource res[] = {
        DEFINE_RES_MEM_NAMED(bar_base + port.offset,  0x14, "mem_map"),
#ifdef HAS_CHANNEL_REG
        DEFINE_RES_REG_NAMED(port.channel, 0x01, "channel"),
#endif
    };

    pdev = platform_device_alloc(OCORES_I2C_DRVNAME, id);
    if (!pdev) {
        err = -ENOMEM;
        pcie_err("Port%u device allocation failed (%d)\n", (id & 0xFF) + 1, err);
        goto exit;
    }

    err = platform_device_add_resources(pdev, res, ARRAY_SIZE(res));
    if (err) {
        pcie_err("Port%u device resource addition failed (%d)\n", (id & 0xFF) + 1, err);
        goto exit_device_put;
    }

    err = platform_device_add_data(pdev, &as9737_32db_platform_data,
                       sizeof(struct ocores_i2c_platform_data));
    if (err) {
        pcie_err("Port%u platform data allocation failed (%d)\n", (id & 0xFF) + 1, err);
        goto exit_device_put;
    }

    err = platform_device_add(pdev);
    if (err) {
        pcie_err("Port%u device addition failed (%d)\n", (id & 0xFF) + 1, err);
        goto exit_device_put;
    }

    return pdev;

exit_device_put:
    platform_device_put(pdev);
exit:
    return NULL;
}

static int as9737_32db_pcie_fpga_i2c_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    pci_fpga_device_t *fpga_dev;
    struct pci_dev *pcidev;
    int i;
    int status = 0, err = 0;
    unsigned long bar_base;

    fpga_dev = devm_kzalloc(dev, sizeof(pci_fpga_device_t), GFP_KERNEL);
    if (!fpga_dev) {
        return -ENOMEM;
    }
    platform_set_drvdata(pdev, fpga_dev);

    pcidev = pci_get_device(FPGA_PCI_VENDOR_ID, FPGA_PCI_DEVICE_ID, NULL);
     if (!pcidev) {
        dev_err(dev, "Cannot found PCI device(%x:%x)\n",
                     FPGA_PCI_VENDOR_ID, FPGA_PCI_DEVICE_ID);
        return -ENODEV;
    }
    fpga_dev->pci_dev = pcidev;

    err = pci_enable_device(pcidev);
    if (err != 0) {
        dev_err(dev, "Cannot enable PCI device(%x:%x)\n",
                     FPGA_PCI_VENDOR_ID, FPGA_PCI_DEVICE_ID);
        status = -ENODEV;
        goto exit_pci_disable;
    }
    /* enable PCI bus-mastering */
    pci_set_master(pcidev);

    /* Create I2C ocore devices first, then create the FPGA sysfs.
     * To prevent the application from accessing an ocore device 
     * that has not been fully created due to the port status 
     * being present.
     */

    /*
     * Create ocore_i2c device for QSFP-DD EEPROM
     */
    bar_base = pci_resource_start(pcidev, BAR0_NUM);
    for (i = 0; i < PORT_NUM; i++) {
        fpga_dev->fpga_i2c[i] = 
            ocore_i2c_device_add(i + 1, bar_base, port[i]);
        if (IS_ERR(fpga_dev->fpga_i2c[i])) {
            status = PTR_ERR(fpga_dev->fpga_i2c[i]);
            dev_err(dev, "rc:%d, unload Port%u[0x%ux] device\n", 
                         status, i + 1, port[i].offset);
            goto exit_ocores_device;
        }
    }

    return 0;

exit_ocores_device:
    while (i > 0) {
        i--;
        platform_device_unregister(fpga_dev->fpga_i2c[i]);
    }

exit_pci_disable:
    pci_disable_device(fpga_dev->pci_dev);

    return status;
}

static int as9737_32db_pcie_fpga_i2c_remove(struct platform_device *pdev)
{
    pci_fpga_device_t *fpga_dev = platform_get_drvdata(pdev);

    if (pci_is_enabled(fpga_dev->pci_dev)) {
        int i;

        /* Unregister ocore_i2c device */
        for (i = 0; i < PORT_NUM; i++) {
            platform_device_unregister(fpga_dev->fpga_i2c[i]);
        }
        pci_disable_device(fpga_dev->pci_dev);
    }

    return 0;
}

static struct platform_driver pcie_fpga_i2c_driver = {
    .probe      = as9737_32db_pcie_fpga_i2c_probe,
    .remove     = as9737_32db_pcie_fpga_i2c_remove,
    .driver     = {
        .owner = THIS_MODULE,
        .name  = DRVNAME,
    },
};

static int __init as9737_32db_pcie_fpga_i2c_init(void)
{
    int status = 0;

    /*
     * Create FPGA I2C platform driver and device
     */
    status = platform_driver_register(&pcie_fpga_i2c_driver);
    if (status < 0) {
        return status;
    }

    pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
    if (IS_ERR(pdev)) {
        status = PTR_ERR(pdev);
        goto exit_pci;
    }

    return status;

exit_pci:
    platform_driver_unregister(&pcie_fpga_i2c_driver);

    return status;
}

static void __exit as9737_32db_pcie_fpga_i2c_exit(void)
{
    platform_device_unregister(pdev);
    platform_driver_unregister(&pcie_fpga_i2c_driver);
}


module_init(as9737_32db_pcie_fpga_i2c_init);
module_exit(as9737_32db_pcie_fpga_i2c_exit);

MODULE_AUTHOR("Roger Ho <roger530_ho@edge-core.com>");
MODULE_DESCRIPTION("AS9737-32DB FPGA I2C driver");
MODULE_LICENSE("GPL");
