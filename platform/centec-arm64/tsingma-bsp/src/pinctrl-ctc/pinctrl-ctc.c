/*
 * Copyright (c) 2018 Liuht
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/bitops.h>
#include <linux/gpio.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/pinctrl/machine.h>
#include <linux/pinctrl/pinconf.h>
#include <linux/pinctrl/pinctrl.h>
#include <linux/pinctrl/pinmux.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/clk.h>
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include "../include/sysctl.h"
#include "pinctrl-ctc.h"
#include "core.h"
#include "pinconf.h"

#define CTC_PIN_BANK_NUM  2

struct ctc_pin_bank {
	u32 pin_base;
	u8 nr_pins;
	char *name;
};

struct ctc_pin_ctrl {
	struct ctc_pin_bank *pin_banks;
	u32 nr_banks;
	u32 nr_pins;
	char *label;
};

struct ctc_pin_config {
	unsigned int func;
	unsigned long *configs;
	unsigned int nconfigs;
};

struct ctc_pin_group {
	const char *name;
	unsigned int npins;
	unsigned int *pins;
	struct ctc_pin_config *data;
};

struct ctc_pmx_func {
	const char *name;
	const char **groups;
	u8 ngroups;
};

struct ctc_pinctrl {
	struct regmap *regmap_base;
	struct device *dev;
	struct pinctrl_desc pctl;
	struct pinctrl_dev *pctl_dev;
	struct ctc_pin_ctrl *ctrl;
	struct ctc_pin_group *groups;
	unsigned int ngroups;
	struct ctc_pmx_func *functions;
	unsigned int nfunctions;
};

static void ctc_pinctrl_child_count(struct ctc_pinctrl *info,
				    struct device_node *np)
{
	struct device_node *child;

	for_each_child_of_node(np, child) {
		info->nfunctions++;
		info->ngroups += of_get_child_count(child);
	}
}

static struct ctc_pin_bank *ctc_bank_num_to_bank(struct ctc_pinctrl *info,
						 unsigned num)
{
	struct ctc_pin_bank *b = info->ctrl->pin_banks;

	if (num < info->ctrl->nr_banks) {
		return &b[num];
	}

	return ERR_PTR(-EINVAL);
}

static int ctc_pinctrl_parse_groups(struct device_node *np,
				    struct ctc_pin_group *grp,
				    struct ctc_pinctrl *info, u32 index)
{
	struct ctc_pin_bank *bank;
	int size;
	const __be32 *list;
	int num;
	int i, j;

	dev_dbg(info->dev, "group(%d): %s\n", index, np->name);

	/* Initialise group */
	grp->name = np->name;

	/*
	 * the binding format is ctc,pins = <bank pin mux CONFIG>,
	 * do sanity check and calculate pins number
	 */
	list = of_get_property(np, "ctc,pins", &size);
	/* we do not check return since it's safe node passed down */
	size /= sizeof(*list);
	if (!size || size % 3) {
		dev_err(info->dev,
			"wrong pins number or pins and configs should be by 4\n");
		return -EINVAL;
	}

	grp->npins = size / 3;

	grp->pins = devm_kzalloc(info->dev, grp->npins * sizeof(unsigned int),
				 GFP_KERNEL);
	grp->data = devm_kzalloc(info->dev, grp->npins *
				 sizeof(struct ctc_pin_config), GFP_KERNEL);
	if (!grp->pins || !grp->data)
		return -ENOMEM;

	for (i = 0, j = 0; i < size; i += 3, j++) {
		num = be32_to_cpu(*list++);
		bank = ctc_bank_num_to_bank(info, num);
		if (IS_ERR(bank))
			return PTR_ERR(bank);

		grp->pins[j] = bank->pin_base + be32_to_cpu(*list++);
		grp->data[j].func = be32_to_cpu(*list++);
	}

	return 0;
}

static int ctc_pinctrl_parse_functions(struct device_node *np,
				       struct ctc_pinctrl *info, u32 index)
{
	struct device_node *child;
	struct ctc_pmx_func *func;
	struct ctc_pin_group *grp;
	int ret;
	static u32 grp_index;
	u32 i = 0;

	dev_dbg(info->dev, "parse function(%d): %s\n", index, np->name);

	func = &info->functions[index];

	/* Initialise function */
	func->name = np->name;
	func->ngroups = of_get_child_count(np);
	if (func->ngroups <= 0)
		return 0;

	func->groups = devm_kzalloc(info->dev,
				    func->ngroups * sizeof(char *), GFP_KERNEL);
	if (!func->groups)
		return -ENOMEM;

	for_each_child_of_node(np, child) {
		func->groups[i] = child->name;
		grp = &info->groups[grp_index++];
		ret = ctc_pinctrl_parse_groups(child, grp, info, i++);
		if (ret) {
			of_node_put(child);
			return ret;
		}
	}

	return 0;
}

static int ctc_pinctrl_parse_dt(struct platform_device *pdev,
				struct ctc_pinctrl *info)
{
	int ret, i;
	u32 bank0_pins, bank1_pins;
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *child;

	info->ctrl = devm_kzalloc(dev, sizeof(struct ctc_pin_ctrl), GFP_KERNEL);
	if (!info->ctrl) {
		dev_err(dev, "failed to allocate memory for pin\n");
		return -EINVAL;
	}

	ret = of_property_read_u32(np, "ctc,pinctrl-bank0", &bank0_pins);
	if (ret < 0) {
		dev_err(dev, "failed to get bank0 pin information\n");
		return -EINVAL;
	}
	ret = of_property_read_u32(np, "ctc,pinctrl-bank1", &bank1_pins);
	if (ret < 0) {
		dev_err(dev, "failed to get bank1 pin information\n");
		return -EINVAL;
	}
	info->ctrl->pin_banks =
	    devm_kzalloc(dev, sizeof(struct ctc_pin_bank) * CTC_PIN_BANK_NUM,
			 GFP_KERNEL);
	if (!info->ctrl->pin_banks) {
		dev_err(dev, "failed to allocate memory for pin banks\n");
		return -EINVAL;
	}
	info->ctrl->nr_banks = CTC_PIN_BANK_NUM;
	info->ctrl->nr_pins = bank0_pins + bank1_pins;
	info->ctrl->pin_banks[0].pin_base = 0;
	info->ctrl->pin_banks[0].nr_pins = bank0_pins;
	info->ctrl->pin_banks[1].pin_base = bank0_pins + 1;
	info->ctrl->pin_banks[1].nr_pins = bank1_pins;

	ctc_pinctrl_child_count(info, np);

	dev_dbg(&pdev->dev, "nfunctions = %d\n", info->nfunctions);
	dev_dbg(&pdev->dev, "ngroups = %d\n", info->ngroups);

	info->functions = devm_kzalloc(dev, info->nfunctions *
				       sizeof(struct ctc_pmx_func), GFP_KERNEL);
	if (!info->functions) {
		dev_err(dev, "failed to allocate memory for function list\n");
		return -EINVAL;
	}

	info->groups = devm_kzalloc(dev, info->ngroups *
				    sizeof(struct ctc_pin_group), GFP_KERNEL);
	if (!info->groups) {
		dev_err(dev, "failed allocate memory for ping group list\n");
		return -EINVAL;
	}

	i = 0;
	for_each_child_of_node(np, child) {
		ret = ctc_pinctrl_parse_functions(child, info, i++);
		if (ret) {
			dev_err(&pdev->dev, "failed to parse function\n");
			of_node_put(child);
			return ret;
		}
	}

	return 0;
}

static int ctc_get_groups_count(struct pinctrl_dev *pctldev)
{
	struct ctc_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	return info->ngroups;
}

static const char *ctc_get_group_name(struct pinctrl_dev *pctldev,
				      unsigned selector)
{
	struct ctc_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	return info->groups[selector].name;
}

static int ctc_get_group_pins(struct pinctrl_dev *pctldev,
			      unsigned selector, const unsigned **pins,
			      unsigned *npins)
{
	struct ctc_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	if (selector >= info->ngroups)
		return -EINVAL;

	*pins = info->groups[selector].pins;
	*npins = info->groups[selector].npins;

	return 0;
}

static inline const struct ctc_pin_group *ctc_pinctrl_name_to_group(const struct
								    ctc_pinctrl
								    *info,
								    const char
								    *name)
{
	int i;

	for (i = 0; i < info->ngroups; i++) {
		if (!strcmp(info->groups[i].name, name))
			return &info->groups[i];
	}

	return NULL;
}

static int ctc_dt_node_to_map(struct pinctrl_dev *pctldev,
			      struct device_node *np,
			      struct pinctrl_map **map, unsigned *num_maps)
{
	struct ctc_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);
	const struct ctc_pin_group *grp;
	struct pinctrl_map *new_map;
	struct device_node *parent;
	int map_num;

	/*
	 * first find the group of this node and check if we need to create
	 * config maps for pins
	 */
	grp = ctc_pinctrl_name_to_group(info, np->name);
	if (!grp) {
		dev_err(info->dev, "unable to find group for node %s\n",
			np->name);
		return -EINVAL;
	}

	map_num = 1;
	new_map = devm_kzalloc(pctldev->dev, sizeof(*new_map) * map_num,
			       GFP_KERNEL);
	if (!new_map)
		return -ENOMEM;

	*map = new_map;
	*num_maps = map_num;

	/* create mux map */
	parent = of_get_parent(np);
	if (!parent) {
		devm_kfree(pctldev->dev, new_map);
		return -EINVAL;
	}
	new_map[0].type = PIN_MAP_TYPE_MUX_GROUP;
	new_map[0].data.mux.function = parent->name;
	new_map[0].data.mux.group = np->name;
	of_node_put(parent);

	return 0;
}

static int ctc_pmx_get_funcs_count(struct pinctrl_dev *pctldev)
{
	struct ctc_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	return info->nfunctions;
}

static const char *ctc_pmx_get_func_name(struct pinctrl_dev *pctldev,
					 unsigned selector)
{
	struct ctc_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	return info->functions[selector].name;
}

static int ctc_pmx_get_groups(struct pinctrl_dev *pctldev,
			      unsigned selector, const char *const **groups,
			      unsigned *const num_groups)
{
	struct ctc_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);

	*groups = info->functions[selector].groups;
	*num_groups = info->functions[selector].ngroups;

	return 0;
}

static struct ctc_pin_bank *ctc_pin_to_bank(struct ctc_pinctrl *info,
					    unsigned pin)
{
	struct ctc_pin_bank *b = info->ctrl->pin_banks;

	while (pin >= (b->pin_base + b->nr_pins))
		b++;

	return b;
}

static int ctc_set_pin_mux(struct ctc_pinctrl *info, struct ctc_pin_bank *bank,
			   int pin, int mux)
{

	if (!bank->pin_base) {
		regmap_update_bits(info->regmap_base,
				   offsetof(struct SysCtl_regs,
					    SysGpioMultiCtl), 3 << pin * 2,
				   mux << pin * 2);
	} else {
		regmap_update_bits(info->regmap_base,
				   offsetof(struct SysCtl_regs,
					    SysGpioHsMultiCtl), 3 << pin * 2,
				   mux << pin * 2);
	}
	return 0;
}

static int ctc_pmx_set(struct pinctrl_dev *pctldev, unsigned selector,
		       unsigned group)
{
	struct ctc_pinctrl *info = pinctrl_dev_get_drvdata(pctldev);
	const unsigned int *pins = info->groups[group].pins;
	const struct ctc_pin_config *data = info->groups[group].data;
	struct ctc_pin_bank *bank;
	int cnt, ret = 0;

	dev_dbg(info->dev, "enable function %s group %s\n",
		info->functions[selector].name, info->groups[group].name);

	/*
	 * for each pin in the pin group selected, program the correspoding pin
	 * pin function number in the config register.
	 */
	for (cnt = 0; cnt < info->groups[group].npins; cnt++) {
		bank = ctc_pin_to_bank(info, pins[cnt]);
		ret = ctc_set_pin_mux(info, bank, pins[cnt] - bank->pin_base,
				      data[cnt].func);
		if (ret)
			break;
	}

	return 0;
}

static void ctc_dt_free_map(struct pinctrl_dev *pctldev,
			    struct pinctrl_map *map, unsigned num_maps)
{
}

static const struct pinctrl_ops ctc_pctrl_ops = {
	.get_groups_count = ctc_get_groups_count,
	.get_group_name = ctc_get_group_name,
	.get_group_pins = ctc_get_group_pins,
	.dt_node_to_map = ctc_dt_node_to_map,
	.dt_free_map = ctc_dt_free_map,
};

static const struct pinmux_ops ctc_pmx_ops = {
	.get_functions_count = ctc_pmx_get_funcs_count,
	.get_function_name = ctc_pmx_get_func_name,
	.get_function_groups = ctc_pmx_get_groups,
	.set_mux = ctc_pmx_set,
};

static int ctc_pinctrl_register(struct platform_device *pdev,
				struct ctc_pinctrl *info)
{
	struct pinctrl_desc *ctrldesc = &info->pctl;
	struct pinctrl_pin_desc *pindesc, *pdesc;
	struct ctc_pin_bank *pin_bank;
	int pin, bank, ret;
	int k;

	ret = ctc_pinctrl_parse_dt(pdev, info);
	if (ret)
		return ret;

	ctrldesc->name = "ctc-pinctrl";
	ctrldesc->owner = THIS_MODULE;
	ctrldesc->pctlops = &ctc_pctrl_ops;
	ctrldesc->pmxops = &ctc_pmx_ops;
	ctrldesc->confops = NULL;

	pindesc =
	    devm_kzalloc(&pdev->dev, sizeof(*pindesc) * info->ctrl->nr_pins,
			 GFP_KERNEL);
	if (!pindesc) {
		dev_err(&pdev->dev, "mem alloc for pin descriptors failed\n");
		return -ENOMEM;
	}
	ctrldesc->pins = pindesc;
	ctrldesc->npins = info->ctrl->nr_pins;

	pdesc = pindesc;
	for (bank = 0, k = 0; bank < info->ctrl->nr_banks; bank++) {
		pin_bank = &info->ctrl->pin_banks[bank];
		for (pin = 0; pin < pin_bank->nr_pins; pin++, k++) {
			pdesc->number = k;
			pdesc->name = kasprintf(GFP_KERNEL, "gpio%d-%d",
						bank, pin);
			pdesc++;
		}
	}

	info->pctl_dev = devm_pinctrl_register(&pdev->dev, ctrldesc, info);
	if (IS_ERR(info->pctl_dev)) {
		dev_err(&pdev->dev, "could not register pinctrl driver\n");
		return PTR_ERR(info->pctl_dev);
	}

	return 0;
}

static int ctc_pinctrl_probe(struct platform_device *pdev)
{
	struct ctc_pinctrl *info;
	struct device *dev = &pdev->dev;

	info = devm_kzalloc(dev, sizeof(struct ctc_pinctrl), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	info->dev = dev;
	info->regmap_base = syscon_regmap_lookup_by_phandle(pdev->dev.of_node,
							    "ctc,sysctrl");
	if (IS_ERR(info->regmap_base))
		return PTR_ERR(info->regmap_base);

	ctc_pinctrl_register(pdev, info);

	platform_set_drvdata(pdev, info);

	return 0;
}

static const struct of_device_id ctc_pinctrl_dt_match[] = {
	{.compatible = "ctc,ctc5236-pinctrl"},
	{},
};

static struct platform_driver ctc_pinctrl_driver = {
	.probe = ctc_pinctrl_probe,
	.driver = {
		   .name = "ctc-pinctrl",
		   .of_match_table = ctc_pinctrl_dt_match,
		   },
};

//static int __init ctc_pinctrl_drv_register(void)
//{
//      return platform_driver_register(&ctc_pinctrl_driver);
//}
//
//postcore_initcall(ctc_pinctrl_drv_register);

module_platform_driver(ctc_pinctrl_driver);
MODULE_LICENSE("GPL");
