/*
 * This file is part of Cienaâs i2c mux, a generic driver for FPGA based I2C mux
 *
 * Based on i2c-mux-gpio.c
 *
 * Limitations:
 *   - requires device tree
 *   - assumes a 32 bit wide device (select/deselect function)
 *
 * Copyright (C) 2021 Ciena Corporation
 * Author: Marc St-Amand <mstamand@ciena.com>
 *
 * i2c mux is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 2 of the License.
 *
 * i2c mux is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/i2c-mux.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/module.h>
#ifdef CONFIG_OF
#include <linux/of_address.h>
#endif
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/rtmutex.h>
#include <linux/slab.h>
#include <linux/types.h>
#ifdef CONFIG_CIENA_MCEE
#include <linux/ciena_mcee.h>
#endif

#include "i2c-ciena-err.h"

#include "i2c-fpga-mux.h"
#include "i2c-fpga-mux-priv.h"
#include "i2c-fpga-mux-grid.h"

#define CREATE_TRACE_POINTS
#include "i2c-fpga-mux-tp.h"

struct i2c_fpga_mux_mutex {
	struct list_head link;
	unsigned         ref_count;
	struct resource  mux_res;
	struct rt_mutex  reg_mutex;
};

/* ------------------------------------------------------------------------- */
/* Shared mutex list for FPGA registers with multiple parent adapters.       */
static LIST_HEAD(mux_mutex_list);
static DEFINE_RT_MUTEX(mux_list_mutex);

static struct rt_mutex *mux_add_shared_mutex(struct resource *res)
{
	struct list_head *entry;
	struct i2c_fpga_mux_mutex *use_mutex = 0;
	int mutex_found = 0;

	rt_mutex_lock(&mux_list_mutex);

	list_for_each(entry, &mux_mutex_list) {
		use_mutex = container_of(entry, struct i2c_fpga_mux_mutex, link);
		if (resource_contains(res, &use_mutex->mux_res) &&
		    resource_contains(&use_mutex->mux_res, res)) {
			use_mutex->ref_count++;
			pr_info("%s: sharing %pR (count=%u)\n",
				__func__, res, use_mutex->ref_count);
			mutex_found = 1;
			break;
		}
	}

	if (!mutex_found) {
		use_mutex = kzalloc(sizeof(*use_mutex), GFP_KERNEL);
		if (use_mutex) {
			use_mutex->ref_count = 1;
			use_mutex->mux_res = *res;
			rt_mutex_init(&use_mutex->reg_mutex);
			list_add(&use_mutex->link, &mux_mutex_list);
		}
	}

	rt_mutex_unlock(&mux_list_mutex);

	return use_mutex ? &use_mutex->reg_mutex : 0;
}

static void mux_del_shared_mutex(struct resource *res)
{
	struct list_head *entry;
	struct i2c_fpga_mux_mutex *use_mutex;

	rt_mutex_lock(&mux_list_mutex);

	list_for_each(entry, &mux_mutex_list) {
		use_mutex = container_of(entry, struct i2c_fpga_mux_mutex, link);
		if (resource_contains(res, &use_mutex->mux_res) &&
		    resource_contains(&use_mutex->mux_res, res)) {
			use_mutex->ref_count--;
			if (0 == use_mutex->ref_count) {
				list_del(entry);
				kfree(use_mutex);
			}
			else {
				pr_info("%s: unsharing %pR (count=%u)\n",
					__func__, res, use_mutex->ref_count);
			}
			break;
		}
	}

	rt_mutex_unlock(&mux_list_mutex);
}

/* ------------------------------------------------------------------------- */
static unsigned mux_read_16b(void *reg) { return __raw_readw(reg); }
static unsigned mux_read_32b(void *reg) { return __raw_readl(reg); }

/* ------------------------------------------------------------------------- */
static void mux_write_16b(unsigned val, void *reg) { __raw_writew((u16) val, reg); }
static void mux_write_32b(unsigned val, void *reg) { __raw_writel(val, reg); }

/* ------------------------------------------------------------------------- */
static unsigned mux_read_regmap(void *reg)
{
	struct i2c_fpga_mux_priv *priv = (struct i2c_fpga_mux_priv *) reg;
	unsigned val = ~0;
	int rc;

	rc = regmap_read(priv->parent_regmap, priv->reg_res.start, &val);

	if (rc) dev_dbg(&priv->pdev->dev, "%s failed res=%pR rc=%d\n",
			__func__, &priv->reg_res, rc);

	return val;
}

static void mux_write_regmap(unsigned val, void *reg)
{
	struct i2c_fpga_mux_priv *priv = (struct i2c_fpga_mux_priv *) reg;
	int rc;

	rc = regmap_write(priv->parent_regmap, priv->reg_res.start, val);

	if (rc) dev_dbg(&priv->pdev->dev, "%s failed res=%pR rc=%d\n",
			__func__, &priv->reg_res, rc);
}

/* ------------------------------------------------------------------------- */
static int mux_select(struct i2c_mux_core *mux_core, u32 chan)
{
	struct i2c_fpga_mux_priv *priv =
		*((struct i2c_fpga_mux_priv **) i2c_mux_priv(mux_core));
	unsigned mask = priv->reg_mask;
	bool parking;
	u32 val, changed, newchan, delay;
	int rc = 0;

	if (priv->reg_mutex) rt_mutex_lock(priv->reg_mutex);

	chan &= mask;
	val = priv->reg_read(priv->mux_reg_ptr);
	changed = ((val & mask) != chan);
	if (changed) {
		newchan = chan | (val & ~mask);
		priv->reg_write(newchan, priv->mux_reg_ptr);
	}
	trace_ciena_i2c_mux_select_chan(priv, (val & mask), chan);

	parking = (priv->park_mask && (chan == priv->parked));

	/* re-use 'val' to compute the masked/shifted mux position */
	val = (chan & priv->reg_mask) >> priv->reg_shift;
	if (!parking && (priv->parent_err) &&
	    (priv->max_chan > val) && (priv->min_chan <= val)) {
		unsigned chan_id = priv->revmap[val - priv->min_chan];
		ciena_i2c_err_set_state(priv->parent_err,
					&priv->children_err[chan_id]);
	}

	if (priv->reg_mutex) rt_mutex_unlock(priv->reg_mutex);

	if (changed) {
		if (!parking) {
			delay = sfp_mux_debounce_select(&priv->mux_debounce);
			trace_ciena_i2c_mux_select_debounce_finished(priv,
								     chan,
								     delay);
		}
	}

#ifdef CONFIG_CIENA_MCEE
	if (ciena_mcee_dev_is_stale(&priv->mceed)) rc = -ENXIO;
#endif

	return rc;
}

/* ------------------------------------------------------------------------- */
static int mux_deselect(struct i2c_mux_core *mux_core, u32 chan)
{
	struct i2c_fpga_mux_priv *priv =
		*((struct i2c_fpga_mux_priv **) i2c_mux_priv(mux_core));
	unsigned mask = priv->reg_mask;
	int rc = 0;
	u32 val, delay;

	delay = sfp_mux_debounce_deselect(&priv->mux_debounce);
	trace_ciena_i2c_mux_deselect_debounce_finished(priv,
						       chan,
						       delay);

	val = (chan & priv->reg_mask) >> priv->reg_shift;
	if ((priv->parent_err) &&
	    (priv->max_chan > val) && (priv->min_chan <= val)) {
		unsigned chan_id = priv->revmap[val - priv->min_chan];
		if (ciena_i2c_err_clr_state(priv->parent_err,
					    &priv->children_err[chan_id]))
			priv->mux_debounce.prev_xfer_failed =
				!!priv->children_err[chan_id].prev_error;
	}

	/*
	 * complain noisily if the mux value does not match what is
	 * deselected -- it implies the select read/write operations
	 * must have better mutex protection
	 */
	val = priv->reg_read(priv->mux_reg_ptr);

	/* park the mux if a channel has been given for that purpose. */
	if (priv->park_mask) {
		mux_select(mux_core, priv->parked);
		trace_ciena_i2c_mux_deselect_parked(priv, priv->parked);
	}

#ifdef CONFIG_CIENA_MCEE
	if (ciena_mcee_dev_is_stale(&priv->mceed)) rc = -ENXIO;
#endif

	if (!rc && ((val & mask) != (chan & mask))) {
		dev_warn(&priv->pdev->dev, "(%x & %x) != (%x & %x)\n",
			 val, mask, chan, mask);
		rc = -1;
	}

	return rc;
}

/* ------------------------------------------------------------------------- */
static int mux_init_regmap(const struct i2c_fpga_mux_info *info,
			   struct i2c_fpga_mux_priv *priv)
{
	struct platform_device *pdev = priv->pdev;
	struct resource *reg;

	priv->parent_regmap = info->parent_regmap;

	reg = platform_get_resource(pdev, IORESOURCE_REG, 0);
	if (!reg) {
		dev_err(&pdev->dev, "missing register resource\n");
		return -ENXIO;
	}

	priv->reg_res   = *reg;
	priv->reg_read  = mux_read_regmap;
	priv->reg_write = mux_write_regmap;

	priv->mux_reg_ptr = priv;

	return 0;
}

/* ------------------------------------------------------------------------- */
static int mux_init_mmio(const struct i2c_fpga_mux_info *info,
			 struct i2c_fpga_mux_priv *priv)
{
	struct platform_device *pdev = priv->pdev;
	struct resource        *mem;
	void                   *region;

	switch (info->reg_width) {
		case 16:
			priv->reg_read  = mux_read_16b;
			priv->reg_write = mux_write_16b;
			break;
		case 0:
		case 32:
			priv->reg_read  = mux_read_32b;
			priv->reg_write = mux_write_32b;
			break;
		default:
			dev_err(&pdev->dev, "invalid register width: %d\n",
				info->reg_width);
			return -EINVAL;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "missing memory resource\n");
		return -ENXIO;
	}

	if (!info->shared_io) {
		region = devm_request_mem_region(&pdev->dev,
						 mem->start,
						 resource_size(mem),
						 I2C_FPGA_MUX_DRIVER_NAME);
		if (!region) {
			dev_err(&pdev->dev, "failed to request region %pR\n",
				mem);
			return -ENXIO;
		}
	}

	priv->mux_reg_ptr = devm_ioremap(&pdev->dev,
					 mem->start,
					 resource_size(mem));
	if (!priv->mux_reg_ptr) {
		dev_err(&pdev->dev, "failed to ioremap registers\n");
		return -EFAULT;
	}

	return 0;
}

/* ------------------------------------------------------------------------- */
static int mux_init_error_states(const struct i2c_fpga_mux_info *info,
				 struct i2c_fpga_mux_priv *priv)
{
	struct platform_device     *pdev = priv->pdev;
	struct device              *dev  = &pdev->dev;
	struct ciena_i2c_err_state *err;
	const int                  *bus_offs = info->bus_offsets;
	unsigned                    min_chan = ~0;
	unsigned                    max_chan = 0;
	unsigned                    mux_val;
	unsigned                    revmap_size;
	int                         adap_nr;
	int                         i;

	err = devm_kzalloc(dev, priv->n_adap * sizeof(*priv->children_err),
			   GFP_KERNEL);

	if (!err) {
		dev_err(dev, "no memory for %u error states\n", priv->n_adap);
		return -ENOMEM;
	}

	for (i = 0; i < priv->n_adap; i++) {
		adap_nr = priv->base_nr + (bus_offs ? bus_offs[i] : i);
		err[i].adapter_id = adap_nr;
		mux_val = priv->values[i] & priv->reg_mask;
		mux_val >>= priv->reg_shift;
		if (max_chan < mux_val)
			max_chan = mux_val;
		if (min_chan > mux_val)
			min_chan = mux_val;
	}

	/* Existing i2c multiplexers use a byte index. Sanity check in
	 * case someone invents a crazy super-mux.
	 */
	if (255 >= (max_chan - min_chan)) {
		priv->min_chan = min_chan;
		priv->max_chan = ++max_chan;
		revmap_size    = (max_chan - min_chan) * sizeof(*priv->revmap);
		priv->revmap   = devm_kzalloc(dev, revmap_size, GFP_KERNEL);
	}

	if (!priv->revmap) {
		devm_kfree(dev, err);
		dev_err(dev, "no memory for %u reverse maps\n", max_chan);
		return -ENOMEM;
	}

	for (i = 0; i < priv->n_adap; i++) {
		mux_val = priv->values[i] & priv->reg_mask;
		mux_val >>= priv->reg_shift;
		priv->revmap[mux_val - min_chan] = i;
	}

	priv->parent_err   = info->parent_err;
	priv->children_err = err;

	return 0;
}

/* ------------------------------------------------------------------------- */
static int mux_init_private_data(const struct i2c_fpga_mux_info *info,
				 struct i2c_fpga_mux_priv *priv)
{
	struct platform_device *pdev = priv->pdev;

	if (!info) return 0;

	priv->parent_id = info->parent_adapter_id;
	priv->base_nr   = info->children_base_id;
	priv->n_adap    = info->num_children;
	priv->reg_mask  = info->reg_mask ? info->reg_mask : 0xffffffff;
	priv->reg_shift = ffs(priv->reg_mask);
	priv->values    = devm_kmemdup(&pdev->dev, info->children_muxsel,
				       sizeof(*info->children_muxsel) * priv->n_adap,
				       GFP_KERNEL);
	priv->parked    = info->parked;
	priv->park_mask = info->park_mask;

	if (priv->reg_shift) priv->reg_shift--;

	if (info->parent_err) {
		int rc = mux_init_error_states(info, priv);
		if (rc) return rc;
	}

	if (info->parent_regmap)
		return mux_init_regmap(info, priv);
	else
		return mux_init_mmio(info, priv);
}

#ifdef CONFIG_OF
/* ------------------------------------------------------------------------- */
static int mux_probe_dt(struct platform_device *pdev,
			struct i2c_fpga_mux_priv *priv)
{
	struct device_node *np = pdev->dev.of_node;
	struct device_node *adapter_np, *child;
	struct i2c_adapter *adapter;
	void __iomem       *reg_ptr;
	unsigned            reg_width;
	int                 shared_reg;
	int                 rc;
	int                 i;

	if (!np) return 0;

	adapter_np = of_parse_phandle(np, "i2c-parent", 0);
	if (!adapter_np) {
		dev_err(&pdev->dev, "Cannot parse i2c-parent\n");
		return -ENODEV;
	}
	adapter = of_find_i2c_adapter_by_node(adapter_np);
	if (!adapter) {
		dev_err(&pdev->dev, "Cannot find parent bus\n");
		return -EPROBE_DEFER;
	}
	priv->parent_id = i2c_adapter_id(adapter);
	put_device(&adapter->dev);

	if (of_property_read_u32(np, "mux-base_nr", &priv->base_nr)) {
		dev_err(&pdev->dev, "Cannot find base_nr\n");
		return -ENODEV;
	}

	priv->reg_read = mux_read_32b;
	priv->reg_write = mux_write_32b;
	if (!of_property_read_u32(np, "fpga-reg-width", &reg_width)) {
		switch (reg_width) {
		case 16:
			priv->reg_read = mux_read_16b;
			priv->reg_write = mux_write_16b;
			break;
		case 32:
			/* already set */
			break;
		default:
			dev_err(&pdev->dev, "Invalid reg-width: %u\n",
				reg_width);
			return -ENODEV;
		}
	}

	priv->n_adap = of_get_child_count(np);

	if (priv->values) {
		dev_info(&pdev->dev,
			 "overriding platform mux values\n");
		devm_kfree(&pdev->dev, priv->values);
	}
	priv->values = devm_kzalloc(&pdev->dev,
				    sizeof(*priv->values) * priv->n_adap,
				    GFP_KERNEL);
	if (!priv->values) {
		dev_err(&pdev->dev, "Cannot allocate value array");
		return -ENOMEM;
	}

	i = 0;
	for_each_child_of_node(np, child) {
		of_property_read_u32(child, "reg", &priv->values[i]);
		i++;
	}

	shared_reg = (2 == of_n_addr_cells(np));
	if (shared_reg) {
		/* this is a bus-like shared mux register */
		if (of_property_read_u32_index(np, "reg", 1, &priv->reg_mask)) {
			dev_err(&pdev->dev, "Failed to get register mask\n");
			return -EINVAL;
		}
		priv->reg_shift = ffs(priv->reg_mask);
		if (priv->reg_shift) priv->reg_shift--;

		np = of_get_parent(np);
	}
	else {
		/* this is a straight mux with one i2c controller parent */
		priv->reg_mask = 0xffffffff;
	}

	rc = of_address_to_resource(np, 0, &priv->reg_res);
	if (rc) {
		dev_err(&pdev->dev, "Missing resource (%d)\n", rc);
		return rc;
	}
	reg_ptr = devm_ioremap(&pdev->dev,
			       priv->reg_res.start,
			       resource_size(&priv->reg_res));
	if (reg_ptr) {
		if (priv->mux_reg_ptr) {
			dev_info(&pdev->dev,
				 "overriding platform mux_reg_ptr=%p "
				 "with OF mux_reg_ptr=%p\n",
				 priv->mux_reg_ptr, reg_ptr);
			devm_iounmap(&pdev->dev, priv->mux_reg_ptr);
		}
		priv->mux_reg_ptr = reg_ptr;
	}
	if (priv->mux_reg_ptr == NULL) {
		dev_err(&pdev->dev, "failed to map reg ptr\n");
		return -ENODEV;
	}

	of_property_read_u32(np, "mux-parked", &priv->parked);
	of_property_read_u32(np, "mux-park-mask", &priv->park_mask);

	return 0;
}
#endif

/* ------------------------------------------------------------------------- */
static void mux_create_children(struct i2c_fpga_mux_priv      *priv,
				const struct i2c_board_info ***mux_slaves,
				int                            num_slaves)
{
	const struct i2c_board_info **bi;
	struct i2c_client            *client;
	struct i2c_adapter           *adap;
	int                           i;

	/* Register child I2C devices under multiplexer */
	for (i = 0; i < num_slaves; i++) {
		adap = priv->mux_core->adapter[i];
		bi   = mux_slaves[i];
		while (bi && *bi) {
			request_module("%s", (*bi)->type);
			client = i2c_new_client_device(adap, *bi);
			if (!client) {
				dev_err(&priv->pdev->dev,
					"failed to create device %s @0x%02x\n",
					(*bi)->type, (*bi)->addr);
			}
			bi++;
		}
	}
}

/*----------------------------------------------------------------------------*/
static void mux_child_work(struct work_struct *work)
{
	const struct i2c_fpga_mux_info *info;
	struct i2c_fpga_mux_priv       *priv;

	priv = container_of(work, struct i2c_fpga_mux_priv, children_work);
	info = priv->pdev->dev.platform_data;

	mux_create_children(priv, info->deferred_slaves,
			    info->num_deferred_slaves);
}

/* ------------------------------------------------------------------------- */
static void mux_create_deferred_children(struct i2c_fpga_mux_priv *priv)
{
	INIT_WORK(&priv->children_work, mux_child_work);
	/* schedule_work fails only if priv->children_work is already
	 * on the scheduler work queue -- this must not happen */
	BUG_ON(!schedule_work(&priv->children_work));
}

/* ------------------------------------------------------------------------- */
static int mux_probe(struct platform_device *pdev)
{
	const struct i2c_fpga_mux_info *info = dev_get_platdata(&pdev->dev);
	struct i2c_fpga_mux_priv *priv;
	struct i2c_fpga_mux_priv **ppriv;
	const int *bus_offs;
	int adap_nr;
	int err;
	int i;
	u32 val;
	u32 mask;

	dev_dbg(&pdev->dev, "%s\n", __func__);

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&pdev->dev, "no memory\n");
		return -ENOMEM;
	}

	platform_set_drvdata(pdev, priv);
	priv->pdev = pdev;

	err = mux_init_private_data(info, priv);
	if (err)
		return err;

#ifdef CONFIG_OF
	err = mux_probe_dt(pdev, priv);
	if (err)
		return err;
#endif

	if (0xffffffff != priv->reg_mask) {
		priv->reg_mutex = mux_add_shared_mutex(&priv->reg_res);
		if (!priv->reg_mutex) {
			dev_err(&pdev->dev, "Failed to create mutex\n");
			return -ENOMEM;
		}
	}

#ifdef CONFIG_CIENA_MCEE
	ciena_mcee_dev_init(pdev, priv->mux_reg_ptr,
			    (mux_read_16b == priv->reg_read) ? 2 : 4,
			    &priv->mceed);
#endif

	priv->parent = i2c_get_adapter(priv->parent_id);
	if (!priv->parent) {
		dev_dbg(&pdev->dev, "Parent adapter (%d) not found\n",
			priv->parent_id);
		err = -EPROBE_DEFER;
		goto out_shared_mutex;
	}

	/* a non-zero park_mask or parked value means: park the mux */
	if (priv->park_mask || priv->parked)
		priv->park_mask = priv->reg_mask;

	mask = priv->park_mask;
	val = priv->reg_read(priv->mux_reg_ptr);
	if (mask && ((val & mask) != priv->parked)) {
		dev_dbg(&pdev->dev, "set mux to parked position (0x%x/0x%x)\n",
			priv->parked, mask);
		priv->reg_write(priv->parked | (val & ~mask),
				priv->mux_reg_ptr);
	}

	priv->mux_debounce.mux = &pdev->dev;
	err = sfp_mux_debounce_probe(&priv->mux_debounce);
	if (err) {
		dev_err(&pdev->dev, "Cannot probe mux debouncer [%d]\n", err);
		goto mux_debounce_failed;
	}

	priv->mux_core = i2c_mux_alloc(priv->parent,
				       &pdev->dev,
				       1 + priv->n_adap,
				       sizeof(priv),
				       0,
				       mux_select,
				       mux_deselect);
	if (!priv->mux_core) {
		dev_err(&pdev->dev, "Failed to allocate mux_core\n");
		err = -ENOMEM;
		goto mux_alloc_failed;
	}

	ppriv = (struct i2c_fpga_mux_priv **) i2c_mux_priv(priv->mux_core);
	*ppriv = priv;

	bus_offs = info ? info->bus_offsets : NULL;

	for (i = 0; i < priv->n_adap; i++) {
		adap_nr = priv->base_nr + (bus_offs ? bus_offs[i] : i);

		err = i2c_mux_add_adapter(priv->mux_core,
					  adap_nr,
					  priv->values[i]);
		if (err) {
			dev_err(&pdev->dev, "Failed to add adapter %d\n", i);
			goto add_adapter_failed;
		}
	}

	if (info) {
		priv->sda_mask = info->sda_mask;
		priv->scl_mask = info->scl_mask;

		if (priv->children_err) {
			err = mux_populate_error_grid(priv);
			if (err) goto add_adapter_failed;
		}

		if (info->mux_slaves)
			mux_create_children(priv, info->mux_slaves,
					    info->num_mux_slaves);
		if (info->deferred_slaves)
			mux_create_deferred_children(priv);

	}

	dev_info(&pdev->dev,
		 "%d port mux on %s using %pR, mask=0x%08x, park=0x%x/0x%x\n",
		 priv->n_adap, priv->parent->name, &priv->reg_res,
		 priv->reg_mask, priv->parked, priv->park_mask);

	return 0;

add_adapter_failed:
	i2c_mux_del_adapters(priv->mux_core);
mux_alloc_failed:
	sfp_mux_debounce_remove(&priv->mux_debounce);
mux_debounce_failed:
	i2c_put_adapter(priv->parent);
out_shared_mutex:
#ifdef CONFIG_CIENA_MCEE
	ciena_mcee_dev_deinit(&priv->mceed);
#endif
	if (priv->reg_mutex) mux_del_shared_mutex(&priv->reg_res);
	return err;
}

/* ------------------------------------------------------------------------- */
static void __exit mux_remove(struct platform_device *pdev)
{
	const struct i2c_fpga_mux_info *info = dev_get_platdata(&pdev->dev);
	struct i2c_fpga_mux_priv       *priv = platform_get_drvdata(pdev);

	dev_dbg(&pdev->dev, "%s\n", __func__);

	if (info && info->deferred_slaves)
		cancel_work_sync(&priv->children_work);

	mux_depopulate_error_grid(priv, priv->n_adap);

	i2c_mux_del_adapters(priv->mux_core);

	sfp_mux_debounce_remove(&priv->mux_debounce);

	i2c_put_adapter(priv->parent);

#ifdef CONFIG_CIENA_MCEE
	ciena_mcee_dev_deinit(&priv->mceed);
#endif

	if (priv->reg_mutex) mux_del_shared_mutex(&priv->reg_res);
}

/* ------------------------------------------------------------------------- */
#ifdef CONFIG_OF
static const struct of_device_id i2c_fpga_mux_of_match[] = {
	{ .compatible = "ciena,i2c_fpga_mux", },
	{},
};

MODULE_DEVICE_TABLE(of, i2c_fpga_mux_of_match);
#endif

/* ------------------------------------------------------------------------- */
static const struct platform_device_id i2c_fpga_mux_platform_ids[] = {
	{ .name = I2C_FPGA_MUX_DRIVER_NAME },
	{},
};
MODULE_DEVICE_TABLE(platform, i2c_fpga_mux_platform_ids);

/* ------------------------------------------------------------------------- */
static struct platform_driver i2c_fpga_mux_driver = {
	.probe		= mux_probe,
	.remove		= mux_remove,
	.id_table       = i2c_fpga_mux_platform_ids,
	.driver = {
		.name	=  I2C_FPGA_MUX_DRIVER_NAME,
		.owner	= THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(i2c_fpga_mux_of_match),
#endif
	},
};

module_platform_driver(i2c_fpga_mux_driver);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Ciena Corporation");
MODULE_AUTHOR("David Pelton <dpelton@ciena.com>");
MODULE_DESCRIPTION("Generic driver for FPGA based I2C mux");

/*
 * Local variables:
 *  c-indent-level: 8
 *  indent-tabs-mode: t
 *  c-basic-offset: 8
 *  tab-width: 8
 * End:
 */
