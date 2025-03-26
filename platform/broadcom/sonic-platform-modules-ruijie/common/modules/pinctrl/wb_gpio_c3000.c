// SPDX-License-Identifier: GPL-2.0
/*
 * Intel Denverton SoC pinctrl/GPIO driver
 *
 * Copyright (C) 2017, Intel Corporation
 * Author: Mika Westerberg <mika.westerberg@linux.intel.com>
 */

#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/pci.h>

#include "wb_pinctrl_intel.h"

static int g_c3000_gpio_debug = 0;
static int g_c3000_gpio_error = 0;
module_param(g_c3000_gpio_debug, int, S_IRUGO | S_IWUSR);
module_param(g_c3000_gpio_error, int, S_IRUGO | S_IWUSR);

#define C3000_GPIO_VERBOSE(fmt, args...) do {                                        \
    if (g_c3000_gpio_debug) { \
        printk(KERN_INFO "[GPIO_PCIE][VER][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define C3000_GPIO_ERROR(fmt, args...) do {                                        \
    if (g_c3000_gpio_error) { \
        printk(KERN_ERR "[GPIO_PCIE][ERR][func:%s line:%d]\r\n"fmt, __func__, __LINE__, ## args); \
    } \
} while (0)

#define DNV_PAD_OWN	0x020
#define DNV_PADCFGLOCK	0x090
#define DNV_HOSTSW_OWN	0x0C0
#define DNV_GPI_IS	0x100
#define DNV_GPI_IE	0x120

#define DNV_GPP(n, s, e)				\
	{						\
		.reg_num = (n),				\
		.base = (s),				\
		.size = ((e) - (s) + 1),		\
	}

#define DNV_COMMUNITY(b, s, e, g, d)			\
	{						\
		.barno = (b),				\
		.padown_offset = DNV_PAD_OWN,		\
		.padcfglock_offset = DNV_PADCFGLOCK,	\
		.hostown_offset = DNV_HOSTSW_OWN,	\
		.is_offset = DNV_GPI_IS,		\
		.ie_offset = DNV_GPI_IE,		\
		.pin_base = (s),			\
		.npins = ((e) - (s) + 1),		\
		.gpps = (g),				\
		.ngpps = ARRAY_SIZE(g),			\
		.dw_base = (d),				\
	}

/* Denverton */
static const struct pinctrl_pin_desc dnv_pins[] = {
	/* North ALL */
	PINCTRL_PIN(0, "GBE0_SDP0"),
	PINCTRL_PIN(1, "GBE1_SDP0"),
	PINCTRL_PIN(2, "GBE0_SDP1"),
	PINCTRL_PIN(3, "GBE1_SDP1"),
	PINCTRL_PIN(4, "GBE0_SDP2"),
	PINCTRL_PIN(5, "GBE1_SDP2"),
	PINCTRL_PIN(6, "GBE0_SDP3"),
	PINCTRL_PIN(7, "GBE1_SDP3"),
	PINCTRL_PIN(8, "GBE2_LED0"),
	PINCTRL_PIN(9, "GBE2_LED1"),
	PINCTRL_PIN(10, "GBE0_I2C_CLK"),
	PINCTRL_PIN(11, "GBE0_I2C_DATA"),
	PINCTRL_PIN(12, "GBE1_I2C_CLK"),
	PINCTRL_PIN(13, "GBE1_I2C_DATA"),
	PINCTRL_PIN(14, "NCSI_RXD0"),
	PINCTRL_PIN(15, "NCSI_CLK_IN"),
	PINCTRL_PIN(16, "NCSI_RXD1"),
	PINCTRL_PIN(17, "NCSI_CRS_DV"),
	PINCTRL_PIN(18, "NCSI_ARB_IN"),
	PINCTRL_PIN(19, "NCSI_TX_EN"),
	PINCTRL_PIN(20, "NCSI_TXD0"),
	PINCTRL_PIN(21, "NCSI_TXD1"),
	PINCTRL_PIN(22, "NCSI_ARB_OUT"),
	PINCTRL_PIN(23, "GBE0_LED0"),
	PINCTRL_PIN(24, "GBE0_LED1"),
	PINCTRL_PIN(25, "GBE1_LED0"),
	PINCTRL_PIN(26, "GBE1_LED1"),
	PINCTRL_PIN(27, "GPIO0"),
	PINCTRL_PIN(28, "PCIE_CLKREQ0_N"),
	PINCTRL_PIN(29, "PCIE_CLKREQ1_N"),
	PINCTRL_PIN(30, "PCIE_CLKREQ2_N"),
	PINCTRL_PIN(31, "PCIE_CLKREQ3_N"),
	PINCTRL_PIN(32, "PCIE_CLKREQ4_N"),
	PINCTRL_PIN(33, "GPIO1"),
	PINCTRL_PIN(34, "GPIO2"),
	PINCTRL_PIN(35, "SVID_ALERT_N"),
	PINCTRL_PIN(36, "SVID_DATA"),
	PINCTRL_PIN(37, "SVID_CLK"),
	PINCTRL_PIN(38, "THERMTRIP_N"),
	PINCTRL_PIN(39, "PROCHOT_N"),
	PINCTRL_PIN(40, "MEMHOT_N"),
	/* South DFX */
	PINCTRL_PIN(41, "DFX_PORT_CLK0"),
	PINCTRL_PIN(42, "DFX_PORT_CLK1"),
	PINCTRL_PIN(43, "DFX_PORT0"),
	PINCTRL_PIN(44, "DFX_PORT1"),
	PINCTRL_PIN(45, "DFX_PORT2"),
	PINCTRL_PIN(46, "DFX_PORT3"),
	PINCTRL_PIN(47, "DFX_PORT4"),
	PINCTRL_PIN(48, "DFX_PORT5"),
	PINCTRL_PIN(49, "DFX_PORT6"),
	PINCTRL_PIN(50, "DFX_PORT7"),
	PINCTRL_PIN(51, "DFX_PORT8"),
	PINCTRL_PIN(52, "DFX_PORT9"),
	PINCTRL_PIN(53, "DFX_PORT10"),
	PINCTRL_PIN(54, "DFX_PORT11"),
	PINCTRL_PIN(55, "DFX_PORT12"),
	PINCTRL_PIN(56, "DFX_PORT13"),
	PINCTRL_PIN(57, "DFX_PORT14"),
	PINCTRL_PIN(58, "DFX_PORT15"),
	/* South GPP0 */
	PINCTRL_PIN(59, "GPIO12"),
	PINCTRL_PIN(60, "SMB5_GBE_ALRT_N"),
	PINCTRL_PIN(61, "PCIE_CLKREQ5_N"),
	PINCTRL_PIN(62, "PCIE_CLKREQ6_N"),
	PINCTRL_PIN(63, "PCIE_CLKREQ7_N"),
	PINCTRL_PIN(64, "UART0_RXD"),
	PINCTRL_PIN(65, "UART0_TXD"),
	PINCTRL_PIN(66, "SMB5_GBE_CLK"),
	PINCTRL_PIN(67, "SMB5_GBE_DATA"),
	PINCTRL_PIN(68, "ERROR2_N"),
	PINCTRL_PIN(69, "ERROR1_N"),
	PINCTRL_PIN(70, "ERROR0_N"),
	PINCTRL_PIN(71, "IERR_N"),
	PINCTRL_PIN(72, "MCERR_N"),
	PINCTRL_PIN(73, "SMB0_LEG_CLK"),
	PINCTRL_PIN(74, "SMB0_LEG_DATA"),
	PINCTRL_PIN(75, "SMB0_LEG_ALRT_N"),
	PINCTRL_PIN(76, "SMB1_HOST_DATA"),
	PINCTRL_PIN(77, "SMB1_HOST_CLK"),
	PINCTRL_PIN(78, "SMB2_PECI_DATA"),
	PINCTRL_PIN(79, "SMB2_PECI_CLK"),
	PINCTRL_PIN(80, "SMB4_CSME0_DATA"),
	PINCTRL_PIN(81, "SMB4_CSME0_CLK"),
	PINCTRL_PIN(82, "SMB4_CSME0_ALRT_N"),
	PINCTRL_PIN(83, "USB_OC0_N"),
	PINCTRL_PIN(84, "FLEX_CLK_SE0"),
	PINCTRL_PIN(85, "FLEX_CLK_SE1"),
	PINCTRL_PIN(86, "GPIO4"),
	PINCTRL_PIN(87, "GPIO5"),
	PINCTRL_PIN(88, "GPIO6"),
	PINCTRL_PIN(89, "GPIO7"),
	PINCTRL_PIN(90, "SATA0_LED_N"),
	PINCTRL_PIN(91, "SATA1_LED_N"),
	PINCTRL_PIN(92, "SATA_PDETECT0"),
	PINCTRL_PIN(93, "SATA_PDETECT1"),
	PINCTRL_PIN(94, "SATA0_SDOUT"),
	PINCTRL_PIN(95, "SATA1_SDOUT"),
	PINCTRL_PIN(96, "UART1_RXD"),
	PINCTRL_PIN(97, "UART1_TXD"),
	PINCTRL_PIN(98, "GPIO8"),
	PINCTRL_PIN(99, "GPIO9"),
	PINCTRL_PIN(100, "TCK"),
	PINCTRL_PIN(101, "TRST_N"),
	PINCTRL_PIN(102, "TMS"),
	PINCTRL_PIN(103, "TDI"),
	PINCTRL_PIN(104, "TDO"),
	PINCTRL_PIN(105, "CX_PRDY_N"),
	PINCTRL_PIN(106, "CX_PREQ_N"),
	PINCTRL_PIN(107, "CTBTRIGINOUT"),
	PINCTRL_PIN(108, "CTBTRIGOUT"),
	PINCTRL_PIN(109, "DFX_SPARE2"),
	PINCTRL_PIN(110, "DFX_SPARE3"),
	PINCTRL_PIN(111, "DFX_SPARE4"),
	/* South GPP1 */
	PINCTRL_PIN(112, "SUSPWRDNACK"),
	PINCTRL_PIN(113, "PMU_SUSCLK"),
	PINCTRL_PIN(114, "ADR_TRIGGER"),
	PINCTRL_PIN(115, "PMU_SLP_S45_N"),
	PINCTRL_PIN(116, "PMU_SLP_S3_N"),
	PINCTRL_PIN(117, "PMU_WAKE_N"),
	PINCTRL_PIN(118, "PMU_PWRBTN_N"),
	PINCTRL_PIN(119, "PMU_RESETBUTTON_N"),
	PINCTRL_PIN(120, "PMU_PLTRST_N"),
	PINCTRL_PIN(121, "SUS_STAT_N"),
	PINCTRL_PIN(122, "SLP_S0IX_N"),
	PINCTRL_PIN(123, "SPI_CS0_N"),
	PINCTRL_PIN(124, "SPI_CS1_N"),
	PINCTRL_PIN(125, "SPI_MOSI_IO0"),
	PINCTRL_PIN(126, "SPI_MISO_IO1"),
	PINCTRL_PIN(127, "SPI_IO2"),
	PINCTRL_PIN(128, "SPI_IO3"),
	PINCTRL_PIN(129, "SPI_CLK"),
	PINCTRL_PIN(130, "SPI_CLK_LOOPBK"),
	PINCTRL_PIN(131, "ESPI_IO0"),
	PINCTRL_PIN(132, "ESPI_IO1"),
	PINCTRL_PIN(133, "ESPI_IO2"),
	PINCTRL_PIN(134, "ESPI_IO3"),
	PINCTRL_PIN(135, "ESPI_CS0_N"),
	PINCTRL_PIN(136, "ESPI_CLK"),
	PINCTRL_PIN(137, "ESPI_RST_N"),
	PINCTRL_PIN(138, "ESPI_ALRT0_N"),
	PINCTRL_PIN(139, "GPIO10"),
	PINCTRL_PIN(140, "GPIO11"),
	PINCTRL_PIN(141, "ESPI_CLK_LOOPBK"),
	PINCTRL_PIN(142, "EMMC_CMD"),
	PINCTRL_PIN(143, "EMMC_STROBE"),
	PINCTRL_PIN(144, "EMMC_CLK"),
	PINCTRL_PIN(145, "EMMC_D0"),
	PINCTRL_PIN(146, "EMMC_D1"),
	PINCTRL_PIN(147, "EMMC_D2"),
	PINCTRL_PIN(148, "EMMC_D3"),
	PINCTRL_PIN(149, "EMMC_D4"),
	PINCTRL_PIN(150, "EMMC_D5"),
	PINCTRL_PIN(151, "EMMC_D6"),
	PINCTRL_PIN(152, "EMMC_D7"),
	PINCTRL_PIN(153, "GPIO3"),
};

static const unsigned int dnv_uart0_pins[] = { 60, 61, 64, 65 };
static const unsigned int dnv_uart0_modes[] = { 2, 3, 1, 1 };
static const unsigned int dnv_uart1_pins[] = { 94, 95, 96, 97 };
static const unsigned int dnv_uart2_pins[] = { 60, 61, 62, 63 };
static const unsigned int dnv_uart2_modes[] = { 1, 2, 2, 2 };
static const unsigned int dnv_emmc_pins[] = {
	142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152,
};

static const struct intel_pingroup dnv_groups[] = {
	PIN_GROUP("uart0_grp", dnv_uart0_pins, dnv_uart0_modes),
	PIN_GROUP("uart1_grp", dnv_uart1_pins, 1),
	PIN_GROUP("uart2_grp", dnv_uart2_pins, dnv_uart2_modes),
	PIN_GROUP("emmc_grp", dnv_emmc_pins, 1),
};

static const char * const dnv_uart0_groups[] = { "uart0_grp" };
static const char * const dnv_uart1_groups[] = { "uart1_grp" };
static const char * const dnv_uart2_groups[] = { "uart2_grp" };
static const char * const dnv_emmc_groups[] = { "emmc_grp" };

static const struct intel_function dnv_functions[] = {
	FUNCTION("uart0", dnv_uart0_groups),
	FUNCTION("uart1", dnv_uart1_groups),
	FUNCTION("uart2", dnv_uart2_groups),
	FUNCTION("emmc", dnv_emmc_groups),
};

static const struct intel_padgroup dnv_north_gpps[] = {
	DNV_GPP(0, 0, 31),	/* North ALL_0 */
	DNV_GPP(1, 32, 40),	/* North ALL_1 */
};

static const struct intel_padgroup dnv_south_gpps[] = {
	DNV_GPP(0, 41, 58),	/* South DFX */
	DNV_GPP(1, 59, 90),	/* South GPP0_0 */
	DNV_GPP(2, 91, 111),	/* South GPP0_1 */
	DNV_GPP(3, 112, 143),	/* South GPP1_0 */
	DNV_GPP(4, 144, 153),	/* South GPP1_1 */
};

static const struct intel_community dnv_communities[] = {
	DNV_COMMUNITY(0, 0, 40, dnv_north_gpps, 0xc20000),
	DNV_COMMUNITY(1, 41, 153, dnv_south_gpps, 0xc50000),
};

static const struct intel_pinctrl_soc_data dnv_soc_data = {
	.pins = dnv_pins,
	.npins = ARRAY_SIZE(dnv_pins),
	.groups = dnv_groups,
	.ngroups = ARRAY_SIZE(dnv_groups),
	.functions = dnv_functions,
	.nfunctions = ARRAY_SIZE(dnv_functions),
	.communities = dnv_communities,
	.ncommunities = ARRAY_SIZE(dnv_communities),
};

static INTEL_PINCTRL_PM_OPS(dnv_pinctrl_pm_ops);

static int pci_dev_init(wb_gpio_data_t *wb_gpio_data, struct pci_dev *pci_dev)
{
    int err, i;
    void __iomem *base;

    C3000_GPIO_VERBOSE("Enter vendor 0x%x, device 0x%x.\n",
        pci_dev->vendor, pci_dev->device);

    C3000_GPIO_VERBOSE("start pci_enable_device!\n");
    err = pci_enable_device(pci_dev);
    if (err) {
        dev_err(&pci_dev->dev, "Failed to enable pci device, ret:%d.\n", err);
        return err;
    }

    err = pci_request_region(pci_dev, 0, "P2SB");
    if (err) {
        dev_err(&pci_dev->dev, "Requesting C3000 P2SB BAR0 region failed, ret: %d\n", err);
        goto err_disable;
    }

    C3000_GPIO_VERBOSE("start pci_set_master!\n");
    pci_set_master(pci_dev);

    base = pci_iomap(pci_dev, wb_gpio_data->pci_bar, 0);
    if (!base) {
        dev_err(&pci_dev->dev, "pci_iomap bar: %d failed\n", wb_gpio_data->pci_bar);
        err = -ENOMEM;
        goto err_release;
    }
    wb_gpio_data->pci_mem_base = base;

    for (i = 0; i < dnv_soc_data.ncommunities; i++) {
        wb_gpio_data->res[i] = base + dnv_soc_data.communities[i].dw_base;
    }
    return 0;
err_release:
    pci_release_region(pci_dev, wb_gpio_data->pci_bar);
err_disable:
    pci_disable_device(pci_dev);
    return err;
}

static void pci_dev_release(wb_gpio_data_t *wb_gpio_data)
{
    struct pci_dev *pci_dev;

    pci_dev = wb_gpio_data->pci_dev;
    if (wb_gpio_data->pci_mem_base) {
        pci_iounmap(pci_dev, wb_gpio_data->pci_mem_base);
    }
    pci_release_region(pci_dev, wb_gpio_data->pci_bar);
    pci_disable_device(pci_dev);
    return;
}

static int wb_gpio_driver_probe(struct platform_device *plat_dev)
{
    int ret, devfn;
    wb_gpio_data_t *wb_gpio_data;
    wb_gpio_data_t *c3000_gpio_device;
    struct pci_dev *pci_dev;

    if (dnv_soc_data.ncommunities > GPIO_RES_MAX) {
        dev_err(&plat_dev->dev, "GPIO ncommunities %lu is more than GPIO resource number: %d\n",
            dnv_soc_data.ncommunities, GPIO_RES_MAX);
        return -EINVAL;
    }

    wb_gpio_data = devm_kzalloc(&plat_dev->dev, sizeof(wb_gpio_data_t), GFP_KERNEL);
    if (!wb_gpio_data) {
        dev_err(&plat_dev->dev, "devm_kzalloc failed.\n");
        ret = -ENOMEM;
        return ret;
    }

    if (plat_dev->dev.of_node) {
        ret = 0;
        ret += of_property_read_u32(plat_dev->dev.of_node, "pci_domain", &wb_gpio_data->pci_domain);
        ret += of_property_read_u32(plat_dev->dev.of_node, "pci_bus", &wb_gpio_data->pci_bus);
        ret += of_property_read_u32(plat_dev->dev.of_node, "pci_slot", &wb_gpio_data->pci_slot);
        ret += of_property_read_u32(plat_dev->dev.of_node, "pci_fn", &wb_gpio_data->pci_fn);
        ret += of_property_read_u32(plat_dev->dev.of_node, "pci_bar", &wb_gpio_data->pci_bar);
        ret += of_property_read_u32(plat_dev->dev.of_node, "irq", &wb_gpio_data->irq);
        if (ret != 0) {
            dev_err(&plat_dev->dev, "Failed to get dts config, ret:%d.\n", ret);
            return -ENXIO;
        }
    } else {
        if (plat_dev->dev.platform_data == NULL) {
            dev_err(&plat_dev->dev, "Failed to get platform data config.\n");
            return -ENXIO;
        }
        c3000_gpio_device = plat_dev->dev.platform_data;
        wb_gpio_data->pci_domain = c3000_gpio_device->pci_domain;
        wb_gpio_data->pci_bus = c3000_gpio_device->pci_bus;
        wb_gpio_data->pci_slot = c3000_gpio_device->pci_slot;
        wb_gpio_data->pci_fn = c3000_gpio_device->pci_fn;
        wb_gpio_data->pci_bar = c3000_gpio_device->pci_bar;
        wb_gpio_data->irq = c3000_gpio_device->irq;
    }

    C3000_GPIO_VERBOSE("domain:0x%04x, bus:0x%02x, slot:0x%02x, fn:%u, bar:%u, irq: %d\n",
        wb_gpio_data->pci_domain, wb_gpio_data->pci_bus, wb_gpio_data->pci_slot, wb_gpio_data->pci_fn,
        wb_gpio_data->pci_bar, wb_gpio_data->irq);

    devfn = PCI_DEVFN(wb_gpio_data->pci_slot, wb_gpio_data->pci_fn);
    pci_dev = pci_get_domain_bus_and_slot(wb_gpio_data->pci_domain, wb_gpio_data->pci_bus, devfn);
    if (pci_dev == NULL) {
        dev_err(&plat_dev->dev, "Failed to find pci_dev, domain:0x%04x, bus:0x%02x, devfn:0x%x\n",
            wb_gpio_data->pci_domain, wb_gpio_data->pci_bus, devfn);
        return -ENXIO;
    }
    wb_gpio_data->pci_dev = pci_dev;
    ret = pci_dev_init(wb_gpio_data, pci_dev);
    if (ret != 0) {
        dev_err(&plat_dev->dev, "Failed to get pci bar address.\n");
        return ret;
    }
    C3000_GPIO_VERBOSE("pci_dev_init success, pci_mem_bae: 0x%pK, res0: 0x%pK, res1: 0x%pK\n",
        wb_gpio_data->pci_mem_base, wb_gpio_data->res[0], wb_gpio_data->res[1]);

    platform_set_drvdata(plat_dev, wb_gpio_data);

    ret = wb_pinctrl_probe(plat_dev, &dnv_soc_data);
    if (ret) {
        dev_err(&plat_dev->dev, "C3000 gpio pinctrl probe failed, ret:%d\n", ret);
        pci_dev_release(wb_gpio_data);
        return ret;
    }
    dev_info(&plat_dev->dev, "C3000 gpio pinctrl probe success.\n");
    return 0;
}

static int wb_gpio_driver_remove(struct platform_device *plat_dev)
{
    wb_gpio_data_t *wb_gpio_data;

    C3000_GPIO_VERBOSE("c3000_gpio_pcie_remove.\n");

    wb_gpio_data = platform_get_drvdata(plat_dev);
    pci_dev_release(wb_gpio_data);
    return 0;
}

static const struct of_device_id gpio_c3000_match[] = {
	{
		.compatible = "wb_gpio_c3000",
	},
	{},
};
MODULE_DEVICE_TABLE(of, gpio_c3000_match);

static struct platform_driver wb_gpio_c3000_driver = {
	.driver = {
		.name		= "wb_gpio_c3000",
		.of_match_table = gpio_c3000_match,
	},
	.probe = wb_gpio_driver_probe,
	.remove = wb_gpio_driver_remove,
};

module_platform_driver(wb_gpio_c3000_driver);

MODULE_DESCRIPTION("C3000 GPIO Controller driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("support");
