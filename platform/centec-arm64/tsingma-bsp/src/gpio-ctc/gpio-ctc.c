/*
 * Copyright (c) 2018 Liuht
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/acpi.h>
#include <linux/gpio/driver.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/irqdomain.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/spinlock.h>
#include "gpio-ctcapb.h"
#include <linux/slab.h>
#include <linux/delay.h>
#include "gpiolib.h"
#include "../include/sysctl.h"
#include <linux/mfd/syscon.h>
#include <linux/regmap.h>
#include "gpiolib-acpi.h"

#define DWAPB_MAX_PORTS		2

static DEFINE_SPINLOCK(ctcgpio_lock);

struct ctcapb_gpio;

static u32 soc_v;

struct ctcapb_gpio_port {
	bool is_registered;
	unsigned int idx;
	struct gpio_chip gc;
	struct ctcapb_gpio *gpio;
	struct GpioSoc_port_regs *regs;
	struct irq_domain *domain;
};

struct ctcapb_gpio {
	struct device *dev;
	unsigned int nr_ports;
	struct GpioSoc_regs *regs;
	struct ctcapb_gpio_port *ports;
	struct regmap *regmap_base;
};

static void clrsetbits(unsigned __iomem * addr, u32 clr, u32 set)
{
	writel((readl(addr) & ~(clr)) | (set), addr);
}

static int ctcapb_gpio_to_irq(struct gpio_chip *gc, unsigned offset)
{
	struct ctcapb_gpio_port *port = gpiochip_get_data(gc);

	return irq_find_mapping(port->domain, offset);
}

static void ctcapb_toggle_trigger(struct ctcapb_gpio_port *port,
				  unsigned int offs)
{
	u32 v = readl(&port->regs->GpioIntrPolarity);

	if (gpio_get_value(port->gc.base + offs))
		v &= ~BIT(offs);
	else
		v |= BIT(offs);

	writel(v, &port->regs->GpioIntrPolarity);
}

static u32 ctcapb_do_irq(struct ctcapb_gpio_port *port)
{
	u32 irq_status = readl_relaxed(&port->regs->GpioIntrStatus);
	u32 ret = irq_status;

	while (irq_status) {
		int hwirq = fls(irq_status) - 1;
		int gpio_irq = irq_find_mapping(port->domain, hwirq);

		generic_handle_irq(gpio_irq);
		irq_status &= ~BIT(hwirq);

		if ((irq_get_trigger_type(gpio_irq) & IRQ_TYPE_SENSE_MASK)
		    == IRQ_TYPE_EDGE_BOTH)
			ctcapb_toggle_trigger(port, hwirq);
	}

	return ret;
}

static void ctcapb_irq_handler(struct irq_desc *desc)
{
	struct ctcapb_gpio_port *port = irq_desc_get_handler_data(desc);
	struct irq_chip *chip = irq_desc_get_chip(desc);

	ctcapb_do_irq(port);

	if (chip->irq_eoi)
		chip->irq_eoi(irq_desc_get_irq_data(desc));
}

static void ctcapb_irq_enable(struct irq_data *d)
{
	struct irq_chip_generic *igc = irq_data_get_irq_chip_data(d);
	struct ctcapb_gpio_port *port = igc->private;
	struct gpio_chip *gc = &port->gc;
	unsigned long flags;

	spin_lock_irqsave(&ctcgpio_lock, flags);
	clrsetbits(&port->regs->GpioIntrEn, 0, BIT(d->hwirq));
	spin_unlock_irqrestore(&ctcgpio_lock, flags);
}

extern void irq_gc_mask_clr_bit(struct irq_data *d);

static void ctcapb_irq_unmask(struct irq_data *d)
{
	irq_gc_mask_clr_bit(d);
	ctcapb_irq_enable(d);
}

#if 0
static void ctcapb_irq_disable(struct irq_data *d)
{
	struct irq_chip_generic *igc = irq_data_get_irq_chip_data(d);
	struct ctcapb_gpio_port *port = igc->private;
	struct gpio_chip *gc = &port->gc;
	unsigned long flags;

	spin_lock_irqsave(&ctcgpio_lock, flags);
	clrsetbits(&port->regs->GpioIntrEn, ~BIT(d->hwirq), 0);
	spin_unlock_irqrestore(&ctcgpio_lock, flags);
}
#endif

static int ctcapb_irq_reqres(struct irq_data *d)
{
	struct irq_chip_generic *igc = irq_data_get_irq_chip_data(d);
	struct ctcapb_gpio_port *port = igc->private;
	struct gpio_chip *gc = &port->gc;

	if (gpiochip_lock_as_irq(gc, irqd_to_hwirq(d))) {
		dev_err(port->gpio->dev, "unable to lock HW IRQ %lu for IRQ\n",
			irqd_to_hwirq(d));
		return -EINVAL;
	}
	return 0;
}

static void ctcapb_irq_relres(struct irq_data *d)
{
	struct irq_chip_generic *igc = irq_data_get_irq_chip_data(d);
	struct ctcapb_gpio_port *port = igc->private;
	struct gpio_chip *gc = &port->gc;

	gpiochip_unlock_as_irq(gc, irqd_to_hwirq(d));
}

static int ctcapb_irq_set_type(struct irq_data *d, u32 type)
{
	struct irq_chip_generic *igc = irq_data_get_irq_chip_data(d);
	struct ctcapb_gpio_port *port = igc->private;
	struct gpio_chip *gc = &port->gc;
	int bit = d->hwirq;
	unsigned long level, polarity, flags, datactl, outctl;

	if (type & ~(IRQ_TYPE_EDGE_RISING | IRQ_TYPE_EDGE_FALLING |
		     IRQ_TYPE_LEVEL_HIGH | IRQ_TYPE_LEVEL_LOW))
		return -EINVAL;

	spin_lock_irqsave(&ctcgpio_lock, flags);
	level = readl(&port->regs->GpioIntrLevel);
	polarity = readl(&port->regs->GpioIntrPolarity);

	if (!soc_v) {
		datactl = readl(&port->regs->GpioDataCtl);
		outctl = readl(&port->regs->GpioOutCtl);

		datactl &= ~BIT(bit);
		outctl |= BIT(bit);

		writel(datactl, &port->regs->GpioDataCtl);
		writel(outctl, &port->regs->GpioOutCtl);

		udelay(10);

		outctl &= ~BIT(bit);
		writel(outctl, &port->regs->GpioOutCtl);
	}
	switch (type) {
	case IRQ_TYPE_EDGE_BOTH:
		level &= ~BIT(bit);
		ctcapb_toggle_trigger(port, bit);
		break;
	case IRQ_TYPE_EDGE_RISING:
		level &= ~BIT(bit);
		polarity |= BIT(bit);
		break;
	case IRQ_TYPE_EDGE_FALLING:
		level &= ~BIT(bit);
		polarity &= ~BIT(bit);
		break;
	case IRQ_TYPE_LEVEL_HIGH:
		level |= BIT(bit);
		polarity |= BIT(bit);
		break;
	case IRQ_TYPE_LEVEL_LOW:
		level |= BIT(bit);
		polarity &= ~BIT(bit);
		break;
	}

	irq_setup_alt_chip(d, type);

	writel(level, &port->regs->GpioIntrLevel);
	writel(polarity, &port->regs->GpioIntrPolarity);
	spin_unlock_irqrestore(&ctcgpio_lock, flags);

	return 0;
}

static int ctcapb_gpio_set_debounce(struct gpio_chip *gc,
				    unsigned offset, unsigned debounce)
{
	struct ctcapb_gpio_port *port = gpiochip_get_data(gc);
	unsigned long flags, val_deb;
	unsigned long mask = BIT(offset);

	spin_lock_irqsave(&ctcgpio_lock, flags);

	val_deb = readl(&port->regs->GpioDebCtl);
	if (debounce)
		writel(val_deb | mask, &port->regs->GpioDebCtl);
	else
		writel(val_deb & ~mask, &port->regs->GpioDebCtl);

	spin_unlock_irqrestore(&ctcgpio_lock, flags);

	return 0;
}

static int ctc_gpio_set_config(struct gpio_chip *gc, unsigned int offset,
			       unsigned long config)
{
	u32 debounce;

	if (pinconf_to_config_param(config) != PIN_CONFIG_INPUT_DEBOUNCE)
		return -ENOTSUPP;

	debounce = pinconf_to_config_argument(config);
	return ctcapb_gpio_set_debounce(gc, offset, debounce);
}

static irqreturn_t ctcapb_irq_handler_mfd(int irq, void *dev_id)
{
	u32 worked;
	struct ctcapb_gpio_port *port = dev_id;

	worked = ctcapb_do_irq(port);

	return worked ? IRQ_HANDLED : IRQ_NONE;
}

static void ctcapb_configure_irqs(struct ctcapb_gpio_port *port,
				  struct ctcapb_port_property *pp)
{
	struct gpio_chip *gc = &port->gc;
	struct fwnode_handle *fwnode = pp->fwnode;
	struct irq_chip_generic *irq_gc = NULL;
	unsigned int hwirq, ngpio = gc->ngpio;
	struct irq_chip_type *ct;
	int err, i;

	port->domain = irq_domain_create_linear(fwnode, ngpio,
						&irq_generic_chip_ops, port);
	if (!port->domain)
		return;

	err = irq_alloc_domain_generic_chips(port->domain, ngpio, 2,
					     "gpio-ctcapb", handle_level_irq,
					     IRQ_NOREQUEST, 0,
					     IRQ_GC_INIT_NESTED_LOCK);
	if (err) {
		dev_info(port->gpio->dev,
			 "irq_alloc_domain_generic_chips failed\n");
		irq_domain_remove(port->domain);
		port->domain = NULL;
		return;
	}

	irq_gc = irq_get_domain_generic_chip(port->domain, 0);
	if (!irq_gc) {
		irq_domain_remove(port->domain);
		port->domain = NULL;
		return;
	}

	irq_gc->reg_base = port->regs;
	irq_gc->private = port;

	for (i = 0; i < 2; i++) {
		ct = &irq_gc->chip_types[i];
		ct->chip.irq_ack = irq_gc_ack_set_bit;
		ct->chip.irq_mask = irq_gc_mask_set_bit;
		ct->chip.irq_unmask = ctcapb_irq_unmask;
		ct->chip.irq_set_type = ctcapb_irq_set_type;
		//ct->chip.irq_enable = ctcapb_irq_enable;
		//ct->chip.irq_disable = ctcapb_irq_disable;
		ct->chip.irq_request_resources = ctcapb_irq_reqres;
		ct->chip.irq_release_resources = ctcapb_irq_relres;
		ct->regs.ack =
		    (&port->regs->GpioEoiCtl - &port->regs->GpioDataCtl) * 4;
		ct->regs.mask =
		    (&port->regs->GpioIntrMask - &port->regs->GpioDataCtl) * 4;
		ct->type = IRQ_TYPE_LEVEL_MASK;
	}

	irq_gc->chip_types[0].type = IRQ_TYPE_LEVEL_MASK;
	irq_gc->chip_types[1].type = IRQ_TYPE_EDGE_BOTH;
	irq_gc->chip_types[1].handler = handle_edge_irq;

	if (!pp->irq_shared) {
		irq_set_chained_handler_and_data(pp->irq, ctcapb_irq_handler,
						 port);
	} else {
		/*
		 * Request a shared IRQ since where MFD would have devices
		 * using the same irq pin
		 */
		err = devm_request_irq(port->gpio->dev, pp->irq,
				       ctcapb_irq_handler_mfd,
				       IRQF_SHARED, "gpio-ctcapb-mfd", port);
		if (err) {
			dev_err(port->gpio->dev, "error requesting IRQ\n");
			irq_domain_remove(port->domain);
			port->domain = NULL;
			return;
		}
	}

	for (hwirq = 0; hwirq < ngpio; hwirq++)
		irq_create_mapping(port->domain, hwirq);

	port->gc.to_irq = ctcapb_gpio_to_irq;
}

static void ctcapb_irq_teardown(struct ctcapb_gpio_port *port)
{
	struct gpio_chip *gc = &port->gc;
	unsigned int ngpio = gc->ngpio;
	irq_hw_number_t hwirq;

	if (!port->domain)
		return;

	for (hwirq = 0; hwirq < ngpio; hwirq++)
		irq_dispose_mapping(irq_find_mapping(port->domain, hwirq));

	irq_domain_remove(port->domain);
	port->domain = NULL;
}

static int ctcapb_gpio_add_port(struct ctcapb_gpio *gpio,
				struct ctcapb_port_property *pp,
				unsigned int offs)
{
	struct ctcapb_gpio_port *port;
	void __iomem *dat, *set, *dirout;
	int err;

	port = &gpio->ports[offs];
	port->gpio = gpio;
	port->idx = pp->idx;

	if (port->idx == 0) {
		dat = &gpio->regs->GpioReadData;
		set = &gpio->regs->GpioDataCtl;
		dirout = &gpio->regs->GpioOutCtl;
		port->regs =
		    (struct GpioSoc_port_regs *)&gpio->regs->GpioDataCtl;
	} else {
		dat = &gpio->regs->GpioHsReadData;
		set = &gpio->regs->GpioHsDataCtl;
		dirout = &gpio->regs->GpioHsOutCtl;
		port->regs =
		    (struct GpioSoc_port_regs *)&gpio->regs->GpioHsDataCtl;
	}

	err = bgpio_init(&port->gc, gpio->dev, 4, dat, set, NULL, dirout,
			 NULL, false);
	if (err) {
		dev_err(gpio->dev, "failed to init gpio chip for port%d\n",
			port->idx);
		return err;
	}
#ifdef CONFIG_OF_GPIO
	port->gc.of_node = to_of_node(pp->fwnode);
#endif
	port->gc.ngpio = pp->ngpio;
	port->gc.base = pp->gpio_base;
	port->gc.set_config = ctc_gpio_set_config;

	if (pp->irq)
		ctcapb_configure_irqs(port, pp);

	err = gpiochip_add_data(&port->gc, port);
	if (err)
		dev_err(gpio->dev, "failed to register gpiochip for port%d\n",
			port->idx);
	else
		port->is_registered = true;

	/* Add GPIO-signaled ACPI event support */
	if (pp->irq)
		acpi_gpiochip_request_interrupts(&port->gc);

	return err;
}

static void ctcapb_gpio_unregister(struct ctcapb_gpio *gpio)
{
	unsigned int m;

	for (m = 0; m < gpio->nr_ports; ++m)
		if (gpio->ports[m].is_registered)
			gpiochip_remove(&gpio->ports[m].gc);
}

static struct ctcapb_platform_data *ctcapb_gpio_get_pdata(struct device *dev)
{
	struct fwnode_handle *fwnode;
	struct ctcapb_platform_data *pdata;
	struct ctcapb_port_property *pp;
	int nports;
	int i;

	nports = device_get_child_node_count(dev);
	if (nports == 0)
		return ERR_PTR(-ENODEV);

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return ERR_PTR(-ENOMEM);

	pdata->properties = devm_kcalloc(dev, nports, sizeof(*pp), GFP_KERNEL);
	if (!pdata->properties)
		return ERR_PTR(-ENOMEM);

	pdata->nports = nports;

	i = 0;
	device_for_each_child_node(dev, fwnode) {
		pp = &pdata->properties[i++];
		pp->fwnode = fwnode;

		if (fwnode_property_read_u32(fwnode, "reg", &pp->idx) ||
		    pp->idx >= DWAPB_MAX_PORTS) {
			dev_err(dev,
				"missing/invalid port index for port%d\n", i);
			fwnode_handle_put(fwnode);
			return ERR_PTR(-EINVAL);
		}

		if (fwnode_property_read_u32(fwnode, "ctc,nr-gpios",
					     &pp->ngpio)) {
			dev_info(dev,
				 "failed to get number of gpios for port%d\n",
				 i);
			pp->ngpio = 32;
		}

		if (dev->of_node && fwnode_property_read_bool(fwnode,
							      "interrupt-controller"))
		{
			pp->irq = irq_of_parse_and_map(to_of_node(fwnode), 0);
			if (!pp->irq)
				dev_warn(dev, "no irq for port%d\n", pp->idx);
		}

		if (has_acpi_companion(dev) && pp->idx == 0)
			pp->irq = platform_get_irq(to_platform_device(dev), 0);

		pp->irq_shared = false;
		pp->gpio_base = -1;
	}

	return pdata;
}

int ctc_bgpio_dir_out(struct gpio_chip *gc, unsigned int gpio, int val)
{
	unsigned long mask;

	if (gc->be_bits)
		mask = BIT(gc->bgpio_bits - 1 - gpio);
	else
		mask = BIT(gpio);

	if (soc_v) {
		return 0;
	} else {
		if (val)
			gc->bgpio_data |= mask;
		else
			gc->bgpio_data &= ~mask;

		gc->write_reg(gc->reg_set, gc->bgpio_data);

		if (gc->be_bits)
			gc->bgpio_dir |= BIT(gc->bgpio_bits - 1 - gpio);
		else
			gc->bgpio_dir |= BIT(gpio);

		gc->write_reg(gc->reg_dir_out, gc->bgpio_dir);

		return 0;
	}
}

static int ctcapb_gpio_probe(struct platform_device *pdev)
{
	unsigned int i;
	struct resource *res;
	struct ctcapb_gpio *gpio;
	int err, val;
	struct device *dev = &pdev->dev;
	struct ctcapb_platform_data *pdata = dev_get_platdata(dev);

	if (!pdata) {
		pdata = ctcapb_gpio_get_pdata(dev);
		if (IS_ERR(pdata))
			return PTR_ERR(pdata);
	}

	if (!pdata->nports)
		return -ENODEV;

	gpio = devm_kzalloc(&pdev->dev, sizeof(*gpio), GFP_KERNEL);
	if (!gpio)
		return -ENOMEM;

	gpio->dev = &pdev->dev;
	gpio->nr_ports = pdata->nports;

	gpio->ports = devm_kcalloc(&pdev->dev, gpio->nr_ports,
				   sizeof(*gpio->ports), GFP_KERNEL);
	if (!gpio->ports)
		return -ENOMEM;

	gpio->regmap_base =
	    syscon_regmap_lookup_by_phandle(pdev->dev.of_node, "ctc,sysctrl");
	if (IS_ERR(gpio->regmap_base))
		return PTR_ERR(gpio->regmap_base);

	regmap_read(gpio->regmap_base,
		    offsetof(struct SysCtl_regs, SysCtlSysRev), &val);

	soc_v = val;

	printk("soc version = 0x%x\n", soc_v);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	gpio->regs =
	    (struct GpioSoc_regs *)devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(gpio->regs))
		return PTR_ERR(gpio->regs);

	for (i = 0; i < gpio->nr_ports; i++) {
		err = ctcapb_gpio_add_port(gpio, &pdata->properties[i], i);
		if (err)
			goto out_unregister;
	}
	platform_set_drvdata(pdev, gpio);

	return 0;

out_unregister:
	ctcapb_gpio_unregister(gpio);
	for (i = 0; i < gpio->nr_ports; i++) {
		ctcapb_irq_teardown(&gpio->ports[i]);
	}

	return err;
}

static int ctcapb_gpio_remove(struct platform_device *pdev)
{
	int i;
	struct ctcapb_gpio *gpio = platform_get_drvdata(pdev);

	ctcapb_gpio_unregister(gpio);
	for (i = 0; i < gpio->nr_ports; i++) {
		ctcapb_irq_teardown(&gpio->ports[i]);
	}

	return 0;
}

static const struct of_device_id ctcapb_of_match[] = {
	{.compatible = "ctc,apb-gpio"},
	{ /* Sentinel */ }
};

MODULE_DEVICE_TABLE(of, ctcapb_of_match);

static const struct acpi_device_id ctcapb_acpi_match[] = {
	{"HISI0181", 0},
	{"APMC0D07", 0},
	{}
};

MODULE_DEVICE_TABLE(acpi, ctcapb_acpi_match);

static struct platform_driver ctcapb_gpio_driver = {
	.driver = {
		   .name = "gpio-ctcapb",
		   .of_match_table = of_match_ptr(ctcapb_of_match),
		   .acpi_match_table = ACPI_PTR(ctcapb_acpi_match),
		   },
	.probe = ctcapb_gpio_probe,
	.remove = ctcapb_gpio_remove,
};

module_platform_driver(ctcapb_gpio_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Liuht");
MODULE_DESCRIPTION("Centec APB GPIO driver");
