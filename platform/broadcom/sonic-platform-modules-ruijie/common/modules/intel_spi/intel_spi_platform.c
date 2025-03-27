/*
 * Intel PCH/PCU SPI flash platform driver.
 *
 * Copyright (C) 2016, Intel Corporation
 * Author: Mika Westerberg <mika.westerberg@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pci.h>

#include "intel_spi.h"

#define PCI_VENDOR_ID_D1527_LPC             (0x8c54)

#define BIOS_CNTL             (0xdc)
#define BIOS_CNTL_SRC_SHIFT    2
#define BIOS_CNTL_WN          BIT(0)
#define BIOS_CNTL_BLE         BIT(1)
#define BIOS_CNTL_SMM_BMP     BIT(5)

#define RCBABASE		0xf0

int intel_spi_platform_debug = 0;
module_param(intel_spi_platform_debug, int, S_IRUGO | S_IWUSR);
int intel_spi_platform_error = 0;
module_param(intel_spi_platform_error, int, S_IRUGO | S_IWUSR);

static bool writeable;
module_param(writeable, bool, 0);
MODULE_PARM_DESC(writeable, "Enable write access to BIOS (default=0)");

#define INTEL_SPI_PLATFORM_VERBOSE(fmt, args...) do {                                        \
        if (intel_spi_platform_debug) { \
            printk(KERN_INFO "[INTEL_SPI_PLATFORM][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
        } \
    } while (0)

#define INTEL_SPI_PLATFORM_ERROR(fmt, args...) do {                                        \
        if (intel_spi_platform_error) { \
            printk(KERN_ERR "[INTEL_SPI_PLATFORM][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
        } \
    } while (0)

static void intel_spi_enable_bios_write(struct pci_dev *pci_dev, struct intel_spi_boardinfo *info)
{
    u8 bios_cntl, value, want, new;

    if (writeable) {
        pci_read_config_byte(pci_dev, BIOS_CNTL, &bios_cntl);
        want = bios_cntl;
        value = (bios_cntl >> BIOS_CNTL_SRC_SHIFT) & 0x3 ;
        if (value == 0x3) {
            INTEL_SPI_PLATFORM_VERBOSE("invalid prefetching/caching settings, ");
        } else {
            INTEL_SPI_PLATFORM_VERBOSE("prefetching %sabled, caching %sabled, ",
                (value & 0x2) ? "en" : "dis",
                (value & 0x1) ? "dis" : "en");
        }

        /* writeable regardless */
        want &= ~BIOS_CNTL_SMM_BMP;
        /* write enable */
        want |= BIOS_CNTL_WN;
        /* BIOS lock disabled */
        want &= ~BIOS_CNTL_BLE;
        INTEL_SPI_PLATFORM_VERBOSE("bios cntl is:0x%x, want is:0x%x\n", bios_cntl, want);
        pci_write_config_byte(pci_dev, BIOS_CNTL, want);
        pci_read_config_byte(pci_dev, BIOS_CNTL, &new);
        INTEL_SPI_PLATFORM_VERBOSE("\nBIOS_CNTL = 0x%02x: ", new);
        INTEL_SPI_PLATFORM_VERBOSE("BIOS Lock Enable: %sabled, ", (new & BIOS_CNTL_BLE) ? "en" : "dis");
        INTEL_SPI_PLATFORM_VERBOSE("BIOS Write Enable: %sabled\n", (new & BIOS_CNTL_WN) ? "en" : "dis");

        if (new & BIOS_CNTL_SMM_BMP) {
            INTEL_SPI_PLATFORM_VERBOSE("BIOS region SMM protection is enabled!\n");
        }

        if (new != want) {
            INTEL_SPI_PLATFORM_VERBOSE("Warning: Setting Bios Control at 0x%x from 0x%02x to 0x%02x failed.\n"
                "New value is 0x%02x.\n", BIOS_CNTL, value, want, new);
        } else {
            info->writeable = !!(new & BIOS_CNTL_WN);
        }
        INTEL_SPI_PLATFORM_VERBOSE("Bios Control is 0x%x\n", new);
    } else {
        INTEL_SPI_PLATFORM_VERBOSE("Bios don't write\n");
    }

    return ;
}

static int intel_spi_platform_probe(struct platform_device *pdev)
{
	struct intel_spi_boardinfo *info;
	struct intel_spi *ispi;
	struct resource *mem;
    struct pci_dev *pci_dev = NULL;
    u32 rcba;

	info = dev_get_platdata(&pdev->dev);
	if (!info)
		return -EINVAL;

    pci_dev = pci_get_device(PCI_VENDOR_ID_INTEL, PCI_VENDOR_ID_D1527_LPC, pci_dev);
    if (!pci_dev) {
        INTEL_SPI_PLATFORM_ERROR("pci_get_device(0x8086, 0x8c54) failed!\n");
        return -1;
    }

    switch (info->type) {
    case INTEL_SPI_LPT:
        pci_read_config_dword(pci_dev, RCBABASE, &rcba);
        if (rcba & 1) {
            intel_spi_enable_bios_write(pci_dev, info);
        }
        break;
    default:
        INTEL_SPI_PLATFORM_ERROR("info type[%d] not need set writeable.\n",info->type);
        break;
    }
    INTEL_SPI_PLATFORM_VERBOSE("intel spi boardinfo writeable is %sabled\n",
            info->writeable ? "en" : "dis");

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ispi = intel_spi_probe(&pdev->dev, mem, info);
	if (IS_ERR(ispi))
		return PTR_ERR(ispi);

	platform_set_drvdata(pdev, ispi);
	return 0;
}

static int intel_spi_platform_remove(struct platform_device *pdev)
{
	struct intel_spi *ispi = platform_get_drvdata(pdev);

	return intel_spi_remove(ispi);
}

static struct of_device_id intel_spi_match[] = {
    {
        .compatible = "spi-c224",
    },
    {},
};
MODULE_DEVICE_TABLE(of, intel_spi_match);

static struct platform_driver intel_spi_platform_driver = {
	.probe = intel_spi_platform_probe,
	.remove = intel_spi_platform_remove,
	.driver = {
		.name = "intel-spi",
		.of_match_table = intel_spi_match,
	},
};

module_platform_driver(intel_spi_platform_driver);

MODULE_DESCRIPTION("Intel PCH/PCU SPI flash platform driver");
MODULE_AUTHOR("support");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:intel-spi");
