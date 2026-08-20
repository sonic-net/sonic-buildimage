#ifndef _BMCX_COMMON_H
#define _BMCX_COMMON_H

#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>

/* Expose a sysfs file called restart_master which indicates that the BMC is
 * the restart master of the board */
#define CIENA_RESTART_MASTER 1

/* openBMC uses two PCI BARS */
#define CIENA_SIRIL_NUM_BARS 2

/* openBMC uses MSI vector #4 */
#undef  CIENA_SIRIL_MSI_INDEX
#define CIENA_SIRIL_MSI_INDEX 4

/* openBMC needs one-shot regmap interrupts */
#undef  CIENA_SIRIL_CIC_RESTYPE
#define CIENA_SIRIL_CIC_RESTYPE IORESOURCE_REG

/* override interrupt resources */
#define CIENA_SIRIL_IRQ_RES_OVERRIDE

/* definitions and code monkeyed from
 * https://github.com/AspeedTech-BMC/linux/blob/d3fac3133a9f105e9bb8f3ef0b7e0dc39f0db6b6/drivers/soc/aspeed/aspeed-host-bmc-dev.c
 */
#define ASPEED_PCI_BMC_BMC2HOST_STS 0x30040
#define BMC2HOST_INT_STS_DOORBELL   BIT(31)
#define BMC2HOST_ENABLE_INTB        BIT(30)

/*
 * Definitions and hard-coded constants lifted from:
 * https://bitbucket.ciena.com/projects/CEDIAG/repos/openbmc/browse/meta-ciena/meta-common/recipes-ciena/obmc-installer/ciena-obmc-installer/obmc_hostipmi.sh#83
 *
 * FPGA reset requests
 */
#define CPLD_FPGA_RECONFIG    1
#define CPLD_DP_FPGA_RECONFIG 2
#define CPLD_BOARD_RESET      3
#define CPLD_DP_BOARD_RESET   4
#define DP_FPGA_RECONFIG      5

/* OpenBMC implements an internal USB hub (0107) with an ethernet and
 * a storage device (both 0199). The USB-2 root hub (1d6b:0002) must
 * also be white-listed.
 *
 * All devices use vendor ID 1d6b (Linux Foundation).
 */
#define BMCX_USB_WHITELIST						\
	"ATTR{idVendor}==\"1d6b\", ATTR{idProduct}==\"0199\", ATTR{manufacturer}==\"Ciena Corp.\", \n" \
	"ATTR{idVendor}==\"1d6b\", ATTR{idProduct}==\"0107\", ATTR{manufacturer}==\"Aspeed\", \n" \
	"ATTR{idVendor}==\"1d6b\", ATTR{idProduct}==\"0002\",\n"

/* OpenBMC BAR names are predictable */
static const char *ciena_siril_bar_names[CIENA_SIRIL_NUM_BARS] = {
	MOD_NAME " registers",
	"aspeed registers",
};

static struct sirilx_usb_neuter bmcx_usb_neuter = {
	.whitelist = BMCX_USB_WHITELIST,
};

static int bmcx_irq_map(struct irq_domain *d, unsigned int virq,
			irq_hw_number_t hw)
{
	irq_set_chip_and_handler(virq, &dummy_irq_chip, handle_simple_irq);
	return 0;
}

static const struct irq_domain_ops bmcx_irq_domain_ops = {
	.map = bmcx_irq_map,
};

static void bmcx_handle_irq(struct irq_desc *desc)
{
	struct irq_chip   *chip    = irq_desc_get_chip(desc);
	struct siril_priv *priv    = irq_desc_get_handler_data(desc);
	void              *bmc_reg = priv->info.mem[1].internal_addr;
	uint32_t           bmc_stat;

	chained_irq_enter(chip, desc);

	bmc_stat = readl(bmc_reg + ASPEED_PCI_BMC_BMC2HOST_STS);

	if (bmc_stat & BMC2HOST_INT_STS_DOORBELL)
		writel(BMC2HOST_INT_STS_DOORBELL,
		       bmc_reg + ASPEED_PCI_BMC_BMC2HOST_STS);

	if (bmc_stat & BMC2HOST_ENABLE_INTB)
		writel(BMC2HOST_ENABLE_INTB,
		       bmc_reg + ASPEED_PCI_BMC_BMC2HOST_STS);

	generic_handle_irq(irq_find_mapping(priv->irq_dom, 0));

	chained_irq_exit(chip, desc);
}

static struct resource bmcx_irq_resource(struct device *dev)
{
	struct siril_priv *priv = dev_get_drvdata(dev);
	struct pci_dev    *pdev = to_pci_dev(dev);
	struct resource    res = {};

	irq_set_chained_handler_and_data(pdev->irq, bmcx_handle_irq, priv);

	priv->irq_dom = irq_domain_add_simple(NULL, 1, 0,
					      &bmcx_irq_domain_ops, priv);
	if (NULL == priv->irq_dom) {
		dev_warn(dev, "failed to allocate BMC interrupt domain\n");
		return res;
	}

	irq_create_mapping(priv->irq_dom, 0);

	res.start = res.end = irq_find_mapping(priv->irq_dom, 0);
	res.flags = IORESOURCE_IRQ;

	return res;
}

static void bmcx_irq_deresource(struct device *dev)
{
	struct siril_priv *priv    = dev_get_drvdata(dev);
	struct pci_dev    *pdev    = to_pci_dev(dev);

	irq_domain_remove(priv->irq_dom);

	irq_set_chained_handler(pdev->irq, NULL);
}

#else
#error "This file must only be included once."
#endif
