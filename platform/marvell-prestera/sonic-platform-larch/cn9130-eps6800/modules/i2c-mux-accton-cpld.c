/*
 * I2C multiplexer driver for accton CPLD
 *
 * Copyright (C) 2023 Accton Technology Corporation.
 * Ang Liu <ang_liu@accton.com.cn>
 *
 * Based on:
 *	i2c-mux-pca954x.c
 *  i2c-mux-pinctrl.c
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include "i2c-mux-accton-cpld.h"
#include "accton-cpld.h"

#define ACCTON_CPLD_I2C_MUX_MAX_NCHANS 8

enum cpldmux_type {
	cpldmux_2,
	cpldmux_3,
	cpldmux_4,
	cpldmux_5,
	cpldmux_6,
	cpldmux_7,
	cpldmux_8,
};

struct chip_desc {
	u8 nchans;
};

struct accton_cpld_mux {
	const struct chip_desc *chip;
	u32	mux_reg_base;
	u32	cpld_type;
	u32 base_nr;
	u8	last_chan;		/* last register value */
};

/* Provide specs for the i2c-mux types we know about */
static const struct chip_desc chips[] = {
	[cpldmux_2] = {
		.nchans = 2,
	},
	[cpldmux_3] = {
		.nchans = 3,
	},
	[cpldmux_4] = {
		.nchans = 4,
	},
	[cpldmux_5] = {
		.nchans = 5,
	},
	[cpldmux_6] = {
		.nchans = 6,
	},
	[cpldmux_7] = {
		.nchans = 7,
	},
	[cpldmux_8] = {
		.nchans = 8,
	},
};

#ifdef CONFIG_OF
static const struct of_device_id accton_cpld_mux_of_match[] = {
	{ .compatible = "accton,cpldmux2", .data = &chips[cpldmux_2] },
	{ .compatible = "accton,cpldmux3", .data = &chips[cpldmux_3] },
	{ .compatible = "accton,cpldmux4", .data = &chips[cpldmux_4] },
	{ .compatible = "accton,cpldmux5", .data = &chips[cpldmux_5] },
	{ .compatible = "accton,cpldmux6", .data = &chips[cpldmux_6] },
	{ .compatible = "accton,cpldmux7", .data = &chips[cpldmux_7] },
	{ .compatible = "accton,cpldmux8", .data = &chips[cpldmux_8] },
	{}
};
MODULE_DEVICE_TABLE(of, accton_cpld_mux_of_match);
#endif

static int accton_cpld_mux_select_chan(struct i2c_mux_core *muxc, u32 chan)
{
	struct accton_cpld_mux *data = i2c_mux_priv(muxc);
	struct device *dev = muxc->dev;
	u8 regval;
	int ret = 0;

	regval = chan;

	dev_dbg(dev, "cpld_type:0x%x, reg:0x%x, val:0x%x\n",
				data->cpld_type, data->mux_reg_base, regval);

	/* Only select the channel if its different from the last channel */
	if (data->last_chan != regval) {
		ret = accton_cpld_write(data->cpld_type, data->mux_reg_base, regval);
		if (ret < 0) {
			dev_err(dev, "failed to select mux chan, cpld_type:0x%x, reg:0x%x, val:0x%x\n",
				data->cpld_type, data->mux_reg_base, regval);
		}
		data->last_chan = ret < 0 ? 0 : regval;
	}

	return ret;
}

/*
 * I2C init/probing/exit functions
 */
static int accton_cpld_mux_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int num, force;
	struct accton_cpld_mux *data;
	const struct of_device_id *match;
	struct accton_cpld_mux_platform_data *pdata = dev_get_platdata(&pdev->dev);
	struct device_node *of_node = dev->of_node;
	struct i2c_mux_core *muxc;
	struct device_node *parent_np;
	struct i2c_adapter *parent;
	int ret;

	parent_np = of_parse_phandle(of_node, "i2c-parent", 0);
	if (!parent_np) {
		dev_err(dev, "Cannot parse i2c-parent\n");
		return -EINVAL;
	}

	parent = of_get_i2c_adapter_by_node(parent_np);
	of_node_put(parent_np);
	if (!parent) {
		return -EPROBE_DEFER;
	}

	muxc = i2c_mux_alloc(parent, dev,
			     ACCTON_CPLD_I2C_MUX_MAX_NCHANS, sizeof(*data), 0,
			     accton_cpld_mux_select_chan, NULL);
	if (!muxc) {
		ret = -ENOMEM;
		goto err_parent;
	}

	data = i2c_mux_priv(muxc);
	platform_set_drvdata(pdev, muxc);

	ret = of_property_read_u32(of_node, "reg", &data->mux_reg_base);
	if (ret < 0) {
		dev_err(dev, "no reg for node '%s'\n", of_node->name);
		goto fail_del_adapters;
	}

	ret = of_property_read_u32(of_node, "base-nr", &data->base_nr);
	if (ret < 0) {
		dev_dbg(dev, "no base-nr for node '%s'\n", of_node->name);
	}

	muxc->mux_locked = of_property_read_bool(of_node, "mux-locked");
	ret = of_property_read_u32(of_node, "cpld-type", &data->cpld_type);
	if (ret < 0) {
		dev_err(dev, "no cpld-type for node '%s'\n", of_node->name);
		goto fail_del_adapters;
	}

	match = of_match_device(of_match_ptr(accton_cpld_mux_of_match), dev);
	if (match) {
		data->chip = of_device_get_match_data(dev);
	} else if (pdata) {
		data->chip = &chips[pdata->mux_type];
	} else {
		dev_err(dev, "can not match any device\n");
		goto fail_del_adapters;
	}

	data->last_chan = 0;		   /* force the first selection */

	/* Now create an adapter for each channel */
	for (num = 0; num < data->chip->nchans; num++) {

		force = data->base_nr ? (data->base_nr + num) : 0;
		if (pdata) {
			if (num >= pdata->num_modes) {
				/* discard unconfigured channels */
				break;
			}
		}

		ret = i2c_mux_add_adapter(muxc, force, num);
		if (ret)
			goto fail_del_adapters;
	}

	dev_info(dev, "registered %d busses\n", num);

	return 0;

fail_del_adapters:
	i2c_mux_del_adapters(muxc);
err_parent:
	i2c_put_adapter(parent);

	return ret;
}

static void accton_cpld_mux_remove(struct platform_device *pdev)
{
	struct i2c_mux_core *muxc = platform_get_drvdata(pdev);

	i2c_mux_del_adapters(muxc);
	i2c_put_adapter(muxc->parent);
}

static struct platform_driver accton_cpld_mux_driver = {
	.driver		= {
		.name	= "accton-cpld-mux",
		.of_match_table = of_match_ptr(accton_cpld_mux_of_match),
	},
	.probe		= accton_cpld_mux_probe,
	.remove		= accton_cpld_mux_remove,
};

module_platform_driver(accton_cpld_mux_driver);

MODULE_AUTHOR("Ang Liu <ang_liu@accton.com.cn>");
MODULE_DESCRIPTION("Accton CPLD I2C mux driver");
MODULE_LICENSE("GPL v2");
