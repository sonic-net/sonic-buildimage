#ifndef __SIRILX_PLATFORM_H
#define __SIRILX_PLATFORM_H

#include <linux/i2c.h>
#include <linux/irqdomain.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/property.h>
#include <linux/spi/spi.h>
#include <linux/serial_core.h>
#include <linux/uio_driver.h>

#include "i2c-ciena.h"
#include "i2c-ciena-smb.h"
#include "i2c-fpga-mux.h"
#include "reset-fpga.h"
#include "reset-sysfs.h"
#include "regmap-sysfs.h"
#include "led-sysfs.h"

#include "sirilx_common.h"
#include "ciena_thermal.h"
#include "ciena_fan.h"

#include "spi-ciena-fpga.h"

#include "gnss/sirilx_gnss.h"
#include "ciena_fpga_watchdog.h"

/* -------------------------------------------------------------------------- */
/* Siril resource types.
 */
enum ciena_siril_devtype {
	devtype_i2c = 0,
	devtype_spi,
	devtype_cic,
	devtype_reset,
	devtype_watchdog,
	devtype_uart,
	devtype_led,
	devtype_num
};
static const unsigned long *ciena_siril_resource_type;

/* -------------------------------------------------------------------------- */
/* NULL-terminated array of hodge-podge platform devices.
 */

struct generic_cic_config;

struct siril_i2c_priv {
	struct platform_device  *i2c_master;
	struct platform_device **i2c_muxes;
};

struct siril_reset_priv {
	struct platform_device **controls;
	struct platform_device **devices;
};

struct siril_priv {
	struct uio_info           info;
	struct siril_i2c_priv    *i2c_masters;
	struct platform_device   *cic;
	struct platform_device  **spi_masters;
	struct platform_device  **uart_masters;
	struct platform_device  **regmap_sysfs;
	struct siril_reset_priv   resets;
	struct platform_device   *watchdog;
	struct platform_device  **extra_devs;
	struct platform_device  **led_devs;
	struct platform_device  **thermal_devs;
	struct platform_device  **fan_devs;
	struct regmap            *regs[MAX_UIO_MAPS];
	struct irq_domain        *irq_dom;
	int                       wdt_irq;
	char                      dev_name[40];
#ifdef CIENA_FPGA_FUNCTIONS
	/* generic chip function (bfpga, cfpga, openbmc, pfpga, etc.) */
	struct device           **chipf_dev;
#endif

};

#include SIRIL_PRIV_MOD_INC

/* The overwhelming majority of siril FPGAs have one memory area. */
#ifndef CIENA_SIRIL_NUM_BARS
#define CIENA_SIRIL_NUM_BARS 1
static const char *ciena_siril_bar_names[CIENA_SIRIL_NUM_BARS] = {
	MOD_NAME " registers",
};
#endif

#include "generic_cic_config.h"

/* -------------------------------------------------------------------------- */
/* Multiplexer speed and delay control knobs.
 */
static u32 optics_i2c_normal_speed;
module_param(optics_i2c_normal_speed, uint, S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(optics_i2c_normal_speed,
		 "override 100kHz optics i2c normal speed");

static u32 optics_i2c_high_speed;
module_param(optics_i2c_high_speed, uint, S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(optics_i2c_high_speed,
		 "override 1MHz optics i2c high speed");

static u32 optics_fru_i2c_normal_speed;
module_param(optics_fru_i2c_normal_speed, uint, S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(optics_fru_i2c_normal_speed,
		 "override 100kHz optics FRU i2c normal speed");

static u32 optics_fru_i2c_high_speed;
module_param(optics_fru_i2c_high_speed, uint, S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(optics_fru_i2c_high_speed,
		 "override 1MHz optics FRU i2c high speed");

static u32 optics_i2c_mux_select_usecs;
module_param(optics_i2c_mux_select_usecs, uint, S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(optics_i2c_mux_select_usecs,
		 "override optics i2c multiplexer selection delay");

static u32 optics_i2c_mux_deselect_usecs;
module_param(optics_i2c_mux_deselect_usecs, uint, S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(optics_i2c_mux_deselect_usecs,
		 "override optics i2c multiplexer deselection delay");

static u32 optics_fru_i2c_mux_select_usecs;
module_param(optics_fru_i2c_mux_select_usecs, uint, S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(optics_fru_i2c_mux_select_usecs,
		 "override optics FRU i2c multiplexer selection delay");

static u32 optics_fru_i2c_mux_deselect_usecs;
module_param(optics_fru_i2c_mux_deselect_usecs, uint, S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(optics_fru_i2c_mux_deselect_usecs,
		 "override optics FRU i2c multiplexer deselection delay");

static u32 optics_i2c_timeout_usecs;
module_param(optics_i2c_timeout_usecs, uint, S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(optics_i2c_timeout_usecs,
		 "override optics i2c transaction timeout");

static u32 optics_fru_i2c_timeout_usecs;
module_param(optics_fru_i2c_timeout_usecs, uint, S_IWUSR | S_IRUGO);
MODULE_PARM_DESC(optics_fru_i2c_timeout_usecs,
		 "override optics FRU i2c transaction timeout");

/* -------------------------------------------------------------------------- */
/*  Tie a device to a device tree node.
 */
static int ciena_siril_of_bind(struct device *dev, const char *of_node)
{
	struct device_node *np;

	np = of_find_node_by_name(NULL, of_node);
	if (NULL == np) {
		dev_err(dev, "could not find of_node %s\n", of_node);
		return -ENOENT;
	}

	device_set_node(dev, of_fwnode_handle(np));

	return 0;
}

/* -------------------------------------------------------------------------- */
/* Override the platform device driver name.
 */
static int ciena_siril_driver_override(struct platform_device *pdev,
				       const char *custom_name,
				       const char *use_driver)
{
	struct device *dev = &pdev->dev;
	int            rc;

	if (NULL == custom_name) return 0;

	rc = driver_set_override(dev, &pdev->driver_override, use_driver,
				 strlen(use_driver));
	if (rc) {
		dev_err(dev, "Cannot set driver override %s (%d)\n",
			use_driver, rc);
		return rc;
	}

	request_module(use_driver);

	rc = device_attach(dev);
	if (0 >= rc) {
		dev_err(dev, "cannot attach to %s (%d)\n", use_driver, rc);
		return (0 > rc) ? rc : -EINVAL;
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
/* Platform device creator helper.
 */
static struct platform_device *
create_pdev(struct device                       *parent,
	    const struct ciena_siril_extra_pdev *xdev)
{
	struct platform_device *pd;
	int                     rc;

	/* step 1: allocate storage */
	pd = platform_device_alloc(xdev->name, xdev->id);
	if (NULL == pd) {
		dev_err(parent, "no memory for platform device %s.%d\n",
			xdev->name, xdev->id);
		return pd;
	}

	/* step 2: adjust pointers
	 * this is the whole purpose of this function */
	pd->dev.parent = parent;

	if (xdev->of_node) {
		rc = ciena_siril_of_bind(&pd->dev, xdev->of_node);
		if (rc) goto out_device_del;
	}

	/* step 3: copy resources */
	if (xdev->res) {
		rc = platform_device_add_resources(pd, xdev->res, xdev->nres);
		if (rc) {
			dev_err(parent, "cannot add resources to device "
				"%s.%d (%d)\n", xdev->name, xdev->id, rc);
			goto out_device_del;
		}
	}

	/* step 4: assign platform data */
	if (xdev->pdata) {
		rc = platform_device_add_data(pd, xdev->pdata, xdev->psize);
		if (rc) {
			dev_err(parent, "cannot add data to device "
				"%s.%d (%d)\n", xdev->name, xdev->id, rc);
			goto out_device_del;
		}
	}

	/* step 5: turn the crank */
	rc = platform_device_add(pd);
	if (rc) {
		dev_err(parent, "cannot add device %s.%d (%d)\n",
			xdev->name, xdev->id, rc);
		goto out_device_del;
	}

	return pd;

out_device_del:
	platform_device_del(pd);
	return NULL;
}

/* -------------------------------------------------------------------------- */
/*  Override I2C multiplexer timing
 */
static void __override_mux_delay(struct device *dev,
				 int            base_id,
				 int            num_pos,
				 unsigned       new_val,
				 u32           *val,
				 const char    *name)
{
	dev_info(dev, "i2c-%d to i2c-%d %s delay override: %u usecs "
		 "(instead of %u)\n", base_id, base_id + num_pos - 1,
		 name, new_val, *val);
	*val = new_val;
}

static void __override_i2c_speed(struct device *dev,
				 int            base_id,
				 int            num_pos,
				 unsigned       new_val,
				 unsigned       mask,
				 unsigned      *val,
				 const char    *name)
{
	unsigned shift = ffs(mask);

	if (1 != num_pos) {
		dev_warn(dev, "cannot override %d-pos i2c-%d %s mux\n",
			 num_pos, base_id, name);
		return;
	}

	if (shift) shift--;

	dev_info(dev, "i2c-%d %s override: %u (instead of %u)\n",
		 base_id, name, new_val, (*val & mask) >> shift);

	*val = (new_val << shift) & mask;

}

static void ciena_siril_override_i2c_mux_timing(struct device            *dev,
						struct i2c_fpga_mux_info *info)
{
	int base_id = info->children_base_id;
	int num_pos = info->num_children;

	switch (base_id) {
	case siril_optics_sfp01_bus_id:
	case siril_optics_qsfp01_bus_id:
	case siril_optics_hs_qsfp01_bus_id:
		if (optics_i2c_mux_select_usecs)
			__override_mux_delay(dev, base_id, num_pos,
					     optics_i2c_mux_select_usecs,
					     &info->select_delay_usec,
					     "mux select");
		if (optics_i2c_mux_deselect_usecs)
			__override_mux_delay(dev, base_id, num_pos,
					     optics_i2c_mux_deselect_usecs,
					     &info->deselect_delay_usec,
					     "mux deselect");
		break;
	case siril_fru1m_i2c_bus_id:
	case siril_fru2m_i2c_bus_id:
	case siril_optics_fru1_hs_qsfp01_bus_id:
	case siril_optics_fru2_hs_qsfp01_bus_id:
		if (optics_fru_i2c_mux_select_usecs)
			__override_mux_delay(dev, base_id, num_pos,
					     optics_fru_i2c_mux_select_usecs,
					     &info->select_delay_usec,
					     "FRU mux select");
		if (optics_fru_i2c_mux_deselect_usecs)
			__override_mux_delay(dev, base_id, num_pos,
					     optics_fru_i2c_mux_deselect_usecs,
					     &info->deselect_delay_usec,
					     "FRU mux deselect");
		break;
	case siril_optics_hs_sel_bus_id:
		if (optics_i2c_normal_speed)
			__override_i2c_speed(dev, base_id, num_pos,
					     optics_i2c_normal_speed,
					     info->reg_mask, &info->parked,
					     "normal speed");
		if (optics_i2c_high_speed)
			__override_i2c_speed(dev, base_id, num_pos,
					     optics_i2c_high_speed,
					     info->reg_mask,
					     info->children_muxsel,
					     "high speed");
		break;
	case siril_optics_fru1_hs_sel_bus_id:
	case siril_optics_fru2_hs_sel_bus_id:
		if (optics_fru_i2c_normal_speed)
			__override_i2c_speed(dev, base_id, num_pos,
					     optics_fru_i2c_normal_speed,
					     info->reg_mask, &info->parked,
					     "FRU normal speed");
		if (optics_fru_i2c_high_speed)
			__override_i2c_speed(dev, base_id, num_pos,
					     optics_fru_i2c_high_speed,
					     info->reg_mask,
					     info->children_muxsel,
					     "FRU high speed");
		break;
	default:
		break;
	}
}

/* -------------------------------------------------------------------------- */
/*  Siril I2C mux
 */
static int ciena_siril_i2c_mux_create(struct device *dev,
				      struct siril_i2c_master *pdata,
				      struct siril_i2c_priv *priv)
{
	const struct siril_i2c_mux *sim;
	struct platform_device    **pd;
	struct resource             res;
	struct device              *mdev = &priv->i2c_master->dev;
	struct regmap              *parent_regmap = NULL;
	struct siril_priv          *ppriv = dev_get_drvdata(dev);
	int                         num_muxes, index;

	num_muxes = pdata->num_muxes;
	if (!num_muxes) return 0;

	priv->i2c_muxes = devm_kzalloc(dev,
				       (1 + num_muxes) *
				       sizeof(*(priv->i2c_muxes)),
				       GFP_KERNEL);
	if (NULL == priv->i2c_muxes) {
		dev_err(dev, "no memory for i2c muxes\n");
		return -ENOMEM;
	}

	if (IORESOURCE_REG == ciena_siril_resource_type[devtype_i2c])
		parent_regmap = ppriv->regs[0];

	for (index = 0; index < num_muxes; index++) {
		struct i2c_fpga_mux_info info;

		sim  = pdata->muxes + index;
		info = *sim->info;

		info.parent_err    = &pdata->err;
		res        = sim->res;

		if ((IORESOURCE_REG == res.flags) && (NULL == parent_regmap)) {
			/* use the parent regmap and resource as-is */
			info.parent_regmap = ppriv->regs[0];
		} else {
			info.parent_regmap  = parent_regmap;
			res.start += ciena_siril_dev_offset(dev);
			res.end   += ciena_siril_dev_offset(dev);
		}

		ciena_siril_override_i2c_mux_timing(dev, &info);

		dev_info(mdev, "%s res:%pR si:%d pai:%d cbi:%d nc:%d\n",
			 sim->name, &res, info.shared_io,
			 info.parent_adapter_id, info.children_base_id,
			 info.num_children);

		pd = priv->i2c_muxes + index;

		*pd = platform_device_register_resndata(mdev, sim->name,
							PLATFORM_DEVID_AUTO,
							&res, 1, &info,
							sizeof(info));
		if (NULL == *pd) {
			dev_err(mdev, "cannot register i2c mux %s\n",
				sim->res.name);
			return -1;
		}
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
/*  Override I2C master timeout
 */
static void ciena_siril_override_i2c_tmo(struct device           *dev,
					 struct siril_i2c_master *sim,
					 unsigned                *tmo)
{
	switch (sim->bus_number) {
	case siril_optics_i2c0_bus_id:
	case siril_optics_i2c1_bus_id:
	case siril_optics_i2c2_bus_id:
	case siril_optics_i2c3_bus_id:
	case siril_optics_i2c4_bus_id:
		if (optics_i2c_timeout_usecs) {
			dev_info(dev, "i2c-%d timeout override: %u usecs"
				 "(instead of %u)\n", sim->bus_number,
				 optics_i2c_timeout_usecs, *tmo);
			*tmo = optics_i2c_timeout_usecs;
		}
		break;
	case siril_fru1_i2c_bus_id:
	case siril_fru2_i2c_bus_id:
	case siril_fru3_i2c_bus_id:
	case siril_fru4_i2c_bus_id:
		if (optics_fru_i2c_timeout_usecs) {
			dev_info(dev, "FRU i2c-%d timeout override: %u usecs"
				 "(instead of %u)\n", sim->bus_number,
				 optics_fru_i2c_timeout_usecs, *tmo);
			*tmo = optics_fru_i2c_timeout_usecs;
		}
		break;
	default:
		break;
	}
}

/* -------------------------------------------------------------------------- */
/*  Siril I2C master
 */
static int ciena_siril_i2c_masters_create(struct device *dev)
{
	struct siril_i2c_master      *master;
	struct siril_i2c_master      *masters       = sirilx_pdata.sirilx_i2c_masters;
	size_t                        ndev          = sirilx_pdata.sirilx_i2c_master_count;
	struct platform_device      **pd;
	struct i2c_ciena_smb_config   smb_config;
	struct ciena_i2c_info         i2c_info;
	struct resource               res[2]        = {};
	struct siril_priv            *priv          = dev_get_drvdata(dev);
	struct regmap                *parent_regmap;
	int                           index, rc;
	void                         *pd_data;
	size_t                        pd_size;

	/* enable the software-controlled I2C controllers */
	if (sirilx_pdata.sirilx_sw_i2c_enable)
		sirilx_pdata.sirilx_sw_i2c_enable(priv->regs[0], true);

	priv->i2c_masters = devm_kzalloc(dev,
					 (1 + ndev) *
					 sizeof(*(priv->i2c_masters)),
					 GFP_KERNEL);
	if (NULL == priv->i2c_masters) {
		dev_err(dev, "no memory for i2c devices\n");
		return -ENOMEM;
	}

	for (index = 0; index < ndev; index++) {
		struct ciena_siril_extra_pdev  xdev     = {};
		const char                    *use_name;
		int                            dev_id   = PLATFORM_DEVID_NONE;
		int                            num_res  = 1;
		int                            i2c_irq  = 0;

		master = masters + index;
		res[0] = master->res;

		if (IORESOURCE_REG == (master->res.flags & IORESOURCE_REG))
			parent_regmap = priv->regs[0];
		else {
			parent_regmap  = NULL;
			res[0].start  += ciena_siril_dev_offset(dev);
			res[0].end    += ciena_siril_dev_offset(dev);
		}

		if (master->is_smbus) {
			memset(&smb_config, 0, sizeof(smb_config));
			smb_config.bus_number            = master->bus_number;
			smb_config.pre_start_gap         = master->pre_start_gap;
			smb_config.timeout_usecs         = master->i2c_timeout;
			smb_config.num_children          = master->num_slaves;
			smb_config.children              = master->slaves;
			smb_config.num_deferred_children = master->num_deferred;
			smb_config.deferred_children     = master->deferred;
			smb_config.deferred_children_delay_ms =
				master->deferred_children_delay_ms;
			smb_config.offsets               = master->offsets;
			smb_config.adap_name             = master->adapter_name;
			smb_config.err_state             = &master->err;

			ciena_siril_override_i2c_tmo(dev, master,
						     &smb_config.timeout_usecs);

			pd_data = &smb_config;
			pd_size = sizeof(smb_config);
		} else {
			memset(&i2c_info, 0, sizeof(i2c_info));
#ifdef __LITTLE_ENDIAN
			i2c_info.little_endian     = true;
#endif
			i2c_info.shared_io         = true;
			i2c_info.num_board_info    = master->num_slaves;
			i2c_info.board_info        = master->slaves;
			i2c_info.num_deferred_info = master->num_deferred;
			i2c_info.deferred_info     = master->deferred;
			i2c_info.reg_width         = master->reg_width;
			i2c_info.reg_offset        = master->reg_offset;
			i2c_info.reg_gap           = master->reg_gap;
			i2c_info.bus_number        = master->bus_number;
			i2c_info.name              = master->adapter_name;
			i2c_info.i2c_timeout       = master->i2c_timeout;
			i2c_info.pre_start_gap     = master->pre_start_gap;
			i2c_info.no_watch          = master->no_watch;
			i2c_info.parent_regmap     = parent_regmap;
			i2c_info.err_state         = &master->err;

			ciena_siril_override_i2c_tmo(dev, master,
						     &i2c_info.i2c_timeout);

			pd_data = &i2c_info;
			pd_size = sizeof(i2c_info);
		}

		if (priv->cic && master->pin) {
			i2c_irq = siril_resolve_cic_pin(priv->regs[0],
							&priv->cic->dev,
							master->pin);
			if (0 < i2c_irq) {
				res[1].flags = IORESOURCE_IRQ;
				res[1].start = i2c_irq;
				res[1].end   = i2c_irq;
				num_res++;
			}
		}

		dev_dbg(dev, "%s res:%pR smb:%d busn:%d regw:%d irq:%d\n",
			master->adapter_name, res, master->is_smbus,
			master->bus_number,
			master->reg_width, i2c_irq);

		use_name = master->adapter_name;
		if (NULL == use_name) {
			use_name = master->driver_name;
			dev_id   = PLATFORM_DEVID_AUTO;
		}

		xdev.name    = use_name;
		xdev.id      = dev_id;
		xdev.res     = res;
		xdev.nres    = num_res;
		xdev.pdata   = pd_data;
		xdev.psize   = pd_size;
		xdev.of_node = master->of_node;

		pd  = &priv->i2c_masters[index].i2c_master;
		*pd = create_pdev(dev, &xdev);

		if (NULL == *pd) {
			dev_err(dev, "cannot register i2c master %s\n",
				master->res.name);
			return -1;
		}

		rc = ciena_siril_driver_override(*pd, master->adapter_name,
						 master->driver_name);
		if (rc) return rc;

		/* create the multiplexers sitting beyond the controller */
		rc = ciena_siril_i2c_mux_create(dev, master,
						&priv->i2c_masters[index]);
		if (rc) return rc;
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
/*  Siril Cascading Interrupts
 */
static int ciena_siril_cic_create(struct device *dev)
{
	struct platform_device        *pd;
	struct resource                res[2] = {};
	struct siril_priv             *priv   = dev_get_drvdata(dev);
	struct generic_cic_config      config = {};
	struct ciena_siril_extra_pdev  xdev   = {};

	if (sirilx_pdata.sirilx_cic_end_offset <=
	    sirilx_pdata.sirilx_cic_start_offset) {
		dev_info(dev, "skipping %s (%pa <= %pa)\n", CIC_NAME,
			 &sirilx_pdata.sirilx_cic_end_offset,
			 &sirilx_pdata.sirilx_cic_start_offset);
		return 0;
	}

	if (sirilx_pdata.sirilx_cic_pdata)
		config = *sirilx_pdata.sirilx_cic_pdata;

	res[0].start  = ciena_siril_dev_offset(dev);
	res[0].end    = res[0].start;
	res[0].start += sirilx_pdata.sirilx_cic_start_offset;
	res[0].end   += sirilx_pdata.sirilx_cic_end_offset;
	res[0].name   = CIC_NAME"-regs";
	res[0].flags  = ciena_siril_resource_type[devtype_cic];

	if (sirilx_pdata.sirilx_irq_resource)
		res[1] = sirilx_pdata.sirilx_irq_resource(dev);
	else
		res[1] = ciena_siril_default_irq_resource(dev);

	config.start_offset  = sirilx_pdata.sirilx_cic_start_offset;
	config.shared_io     = true;
#ifdef __LITTLE_ENDIAN
	config.little_endian = true;
#endif

	if (IORESOURCE_REG == ciena_siril_resource_type[devtype_cic])
		config.parent_regmap = priv->regs[0];

	dev_dbg(dev, "%s res[0]:%pR res[1]:%pR so:%pa[p], si:%d, le:%d%s\n",
		CIC_NAME, &res[0], &res[1], &config.start_offset,
		config.shared_io, config.little_endian,
		config.clear_to_clear ? " c2c" : "");

	if (sirilx_pdata.sirilx_cic_pdata && sirilx_pdata.sirilx_cic_pdata_size)
		*sirilx_pdata.sirilx_cic_pdata = config;
	else {
		sirilx_pdata.sirilx_cic_pdata      = &config;
		sirilx_pdata.sirilx_cic_pdata_size = sizeof(config);
	}

	xdev.name    = CIC_NAME;
	xdev.id      = PLATFORM_DEVID_NONE;
	xdev.res     = res;
	xdev.nres    = ARRAY_SIZE(res);
	xdev.pdata   = sirilx_pdata.sirilx_cic_pdata;
	xdev.psize   = sirilx_pdata.sirilx_cic_pdata_size;
	xdev.of_node = sirilx_pdata.sirilx_cic_of_node;

	pd = create_pdev(dev, &xdev);

	/* clear the sirilx_pdata.sirilx_cic_pdata static, we do not
	 * want a stale stack pointer spanning a driver restart */
	sirilx_pdata.sirilx_cic_pdata      = NULL;
	sirilx_pdata.sirilx_cic_pdata_size = 0;

	if (!pd) {
		dev_err(dev, "failed to register cic device\n");
		if (sirilx_pdata.sirilx_irq_deresource)
			sirilx_pdata.sirilx_irq_deresource(dev);
		else
			ciena_siril_default_irq_deresource(dev);
		return -1;
	}

	priv->cic = pd;

	if (sirilx_pdata.sirilx_cic_fixups)
		(sirilx_pdata.sirilx_cic_fixups)(priv->regs[0], &pd->dev);

	return 0;
}

/* -------------------------------------------------------------------------- */
/*  SPI master lookup helper.
 *  Replaces deprecated spi_busnum_to_master() by finding the child device
 *  by name under the platform device that registered the SPI controller.
 */
static struct spi_controller *cfpga_spi_master(struct device *dev, unsigned busn)
{
	struct device *child;
	char           spinm[sizeof("spi4294967295")] = {};

	snprintf(spinm, sizeof(spinm) - 1, "spi%u", busn);

	child = device_find_child_by_name(dev, spinm);
	if (NULL == child) {
		dev_err(dev, "cannot find child spi master %s\n", spinm);
		return NULL;
	}

	return container_of(child, struct spi_controller, dev);
}

/* -------------------------------------------------------------------------- */
/*  Siril SPI master
 */
static int ciena_siril_spi_masters_create(struct device *dev)
{
	struct siril_priv                    *priv    = dev_get_drvdata(dev);
	const struct siril_spi_master        *master;
	const struct siril_spi_master        *masters = sirilx_pdata.sirilx_spi_masters;
	size_t                                ndev    = sirilx_pdata.sirilx_spi_master_count;
	struct platform_device              **pd;
	int                                   ncl, index;
	struct spi_ciena_fpga_platform_data   scfpd;
	struct spi_controller                    *sm;
	struct resource                       res;
	void                                 *pd_data;
	size_t                                pd_size;

	/* enable the offload FPGA eeprom first -- the spi flash
	 * driver must probe it */
	if (sirilx_pdata.sirilx_ofpga_eeprom_enable)
		sirilx_pdata.sirilx_ofpga_eeprom_enable(priv->regs[0], true);

	priv->spi_masters = devm_kzalloc(dev,
					 (1 + ndev) *
					 sizeof(*(priv->spi_masters)),
					 GFP_KERNEL);
	if (NULL == priv->spi_masters) {
		dev_err(dev, "no memory for spi devices\n");
		return -ENOMEM;
	}

	for (index = 0; index < ndev; index++) {
		struct ciena_siril_extra_pdev  xdev = {};

		master = masters + index;

		res        = master->res;
		res.start += ciena_siril_dev_offset(dev);
		res.end   += ciena_siril_dev_offset(dev);

		memset(&scfpd, 0, sizeof(scfpd));
		scfpd.num_chipselect = master->num_chipsel;
		scfpd.bus_num        = master->bus_number;
		scfpd.width          = master->reg_width;
		scfpd.gpio_chipsel   = master->gpio_chipsel;
		scfpd.fifo_depth     = siril_spi_master_fifo_depth;
#ifndef __LITTLE_ENDIAN
		scfpd.big_endian     = 1;
#endif

		dev_dbg(dev, "%s res:%pR ncsl:%u busn:%u regw:%d be:%d\n",
			master->name, &res, master->num_chipsel, master->bus_number,
			master->reg_width, scfpd.big_endian);

		pd_data = &scfpd;
		pd_size = sizeof(scfpd);

		xdev.name    = master->name;
		xdev.id      = PLATFORM_DEVID_AUTO;
		xdev.res     = &res;
		xdev.nres    = 1;
		xdev.pdata   = pd_data;
		xdev.psize   = pd_size;
		xdev.of_node = master->of_node;

		pd = priv->spi_masters + index;
		*pd = create_pdev(dev, &xdev);
		if (NULL == *pd) {
			dev_err(dev, "cannot register spi master %s\n",
				master->res.name);
			return -1;
		}

		if (0 == master->num_clients)
			continue;

		sm = cfpga_spi_master(&(*pd)->dev, master->bus_number);
		if (NULL == sm)
			return -1;

		/* create the spi devices underneath the master */
		for (ncl = 0; ncl < master->num_clients; ncl++) {
			struct spi_board_info sbi = master->clients[ncl].board_info;

			/* there is no need to save the spi_device, it
			 * gets released along with the spi_master */
			if (NULL == spi_new_device(sm, &sbi)) {
				dev_err(dev,
					"cannot add spi device %s\n",
					sbi.modalias);
				put_device(&sm->dev);
				return -1;
			}
		}

		/* release the ref obtained by cfpga_spi_master */
		put_device(&sm->dev);
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
/*  Siril UART
 */
static int ciena_siril_uart_create(struct device *dev)
{
	struct platform_device           **pd            = 0;
	struct siril_priv                 *priv          = dev_get_drvdata(dev);
	const bool                         shared_io     = true;
	const bool                         little_endian = __LITTLE_ENDIAN ? true : false;
	const struct siril_uart_master    *master;
	const struct siril_uart_master    *masters       = sirilx_pdata.sirilx_uart_masters;
	size_t                             ndev          = sirilx_pdata.sirilx_uart_master_count;
	size_t                             index         = 0;
	struct sirilx_gnss_platform_data   pdata         = {};

	if (sirilx_pdata.sirilx_uart_buffer_enable)
		sirilx_pdata.sirilx_uart_buffer_enable(priv->regs[0], true);

	priv->uart_masters =
		devm_kzalloc(dev,
			     (1 + ndev) * sizeof(*(priv->uart_masters)),
			     GFP_KERNEL);
	if(! priv->uart_masters) {
		dev_err(dev, "no memory for UART devices\n");
		return -ENOMEM;
	}

	for (index = 0; index < ndev; ++index) {
		struct resource  res[2] = { };
		const char      *name;
		int              nres = 1;
		int              devid;
		int              irq;

		master = masters + index;
		name   = master->name;
		devid  = master->devid;
		*res   = master->res;

		pdata.port.mapbase  = res->start;
		res->start         += ciena_siril_dev_offset(dev);
		res->end           += ciena_siril_dev_offset(dev);

		if (priv->cic && master->pin) {
			irq = siril_resolve_cic_pin(priv->regs[0],
							&priv->cic->dev,
							master->pin);
			if (0 < irq) {
				res[1].flags = IORESOURCE_IRQ;
				res[1].start = irq;
				res[1].end   = irq;
				nres++;
			}
		}

		dev_dbg(dev,
			"%s res:%pR so:%pa[p], si:%d, le:%d, irq:%d\n",
			name, &res, &pdata.port.mapbase, shared_io,
			little_endian, irq);

		pd = priv->uart_masters + index;
		*pd = platform_device_register_resndata(dev, name, devid,
							res, nres, &pdata,
							sizeof(pdata));

		if (! *pd) {
			dev_err(dev, "failed to register UART device\n");
			return -1;
		}
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
/*  Siril watchdog
 */
static int ciena_siril_watchdog_create(struct device *dev)
{
	struct siril_priv                *priv  = dev_get_drvdata(dev);
	const struct siril_fpga_watchdog *sfw   = sirilx_pdata.sirilx_watchdog;
	struct ciena_fpga_watchdog_pdata  pdata;
	int                               bar   = 0;

	/*  check if platform support fpga based watchdog */
	if (sfw == NULL) {
		priv->watchdog = NULL;
		return 0;
	}

	memset(&pdata, 0, sizeof(pdata));
	pdata.name        = FPGA_WATCHDOG_DRIVER_NAME;
	pdata.regmap      = priv->regs[bar];
	pdata.wdt_ctl_reg = sfw->wdt_ctl_reg;
	pdata.wdt_clr_reg = sfw->wdt_clr_reg;

	priv->watchdog = platform_device_register_data(dev,
						       pdata.name,
						       PLATFORM_DEVID_NONE,
						       &pdata,
						       sizeof(pdata));
	if (NULL == priv->watchdog) {
		dev_err(dev,
			"cannot register fpga watchdog controller %s\n",
			pdata.name);
		return -1;
	}

	return 0;
}
/* -------------------------------------------------------------------------- */
/*  Siril reset controllers
 */
static void ciena_siril_scrape_resets(struct device                 *dev,
				      struct ciena_fpga_reset_pdata *pdata)
{
	const struct ciena_sysfs_reset  *csr;
	const struct siril_reset_device *srd;
	const struct siril_reset_device *siril_resets = sirilx_pdata.sirilx_resets;
	size_t                           ndev         = sirilx_pdata.sirilx_reset_count;
	struct ciena_sysfs_reset_names   names        = {};
	unsigned                         max          = 0;
	unsigned                         pos;
	size_t                           index;

	for (index = 0; index < ndev; index++) {
		srd = siril_resets + index;

		if (strcmp(pdata->name, srd->pdata.controller_name))
			continue;

		for (csr = srd->pdata.resets; csr->reset_name; csr++) {
			pos = csr->bit_offset;

			/* We are dealing with a jumbo reset controller
			 * that supports more than 64 bit position. If this
			 * ever happens, then RESET_SYSFS_MAX_NAMES must
			 * be incremented.
			 */
			if (RESET_SYSFS_MAX_NAMES <= pos) {
				dev_warn(dev, "position %u >= %u\n", pos,
					 RESET_SYSFS_MAX_NAMES);
				continue;
			}

			names.nm[pos] = csr->reset_name;
			if (max <= pos) max = 1 + pos;
		}
	}

	if (!max) return;

	pdata->reset_names = devm_kmalloc_array(dev, max,
						sizeof(*pdata->reset_names),
						GFP_KERNEL);

	if (NULL == pdata->reset_names) {
		/* Not enough memory for reset names. This is not a
		 * big deal, sirilx resets were nameless for years,
		 */
		dev_warn(dev, "No memory for %u reset names\n", max);
		return;
	}

	for (index = 0; max > index; index++)
		pdata->reset_names[index] = names.nm[index];

	pdata->num_names = max;
}

static int ciena_siril_reset_create(struct device *dev)
{
	struct siril_priv                    *priv            = dev_get_drvdata(dev);
	const struct siril_reset_controller  *src;
	const struct siril_reset_device      *srd;
	struct ciena_fpga_reset_pdata         pdata;
	struct regmap                        *parent_regmap   = NULL;
	struct platform_device              **pd;
	const struct siril_reset_device      *siril_resets    = sirilx_pdata.sirilx_resets;
	size_t                                ndev            = sirilx_pdata.sirilx_reset_count;
	size_t                                num_controllers = sirilx_pdata.sirilx_reset_controller_count;
	size_t                                index;
	struct resource                       res;
	int                                   rc;

	/* enable the reset registers first */
	if (sirilx_pdata.sirilx_resets_init)
		sirilx_pdata.sirilx_resets_init(priv->regs[0]);

	priv->resets.controls = devm_kzalloc(dev,
					     (1 + num_controllers) *
					     sizeof(*(priv->resets.controls)),
					     GFP_KERNEL);

	priv->resets.devices = devm_kzalloc(dev,
					    (1 + ndev) *
					    sizeof(*(priv->resets.devices)),
					    GFP_KERNEL);

	if ((NULL == priv->resets.controls) ||
	    (NULL == priv->resets.devices)) {
		dev_err(dev, "no memory for reset controllers\n");
		return -ENOMEM;
	}

	if (IORESOURCE_REG == ciena_siril_resource_type[devtype_reset])
		parent_regmap = priv->regs[0];

	for (index = 0; index < num_controllers; index++) {
		src = sirilx_pdata.sirilx_reset_controllers + index;

		pdata = src->pdata;
		pdata.parent_regmap = parent_regmap;

		if (!(pdata.use_raw_value)) ciena_siril_scrape_resets(dev, &pdata);

		memset(&res, 0, sizeof(res));
		res.start = ciena_siril_dev_offset(dev) + src->reg_offset;
		res.end   = res.start + pdata.reg_size - 1;
		res.name  = pdata.name;
		res.flags = ciena_siril_resource_type[devtype_reset];

		dev_dbg(dev, "%s res:%pR rs:%u sio:%u neg:%u\n",
			pdata.name, &res, pdata.reg_size,
			pdata.shared_io, pdata.negative);

		pd = priv->resets.controls + index;
		*pd = platform_device_register_resndata(dev, pdata.name,
							PLATFORM_DEVID_NONE,
							&res, pdata.reg_size ? 1 : 0,
							&pdata, sizeof(pdata));
		if (NULL == *pd) {
			dev_err(dev,
				"cannot register reset controller %s\n",
				src->pdata.name);
			return -1;
		}

		rc = ciena_siril_driver_override(*pd, pdata.name,
						 pdata.ipmi ? RESET_IPMI_DRIVER_NAME : RESET_FPGA_DRIVER_NAME);
		if (rc) return rc;
	}

	for (index = 0; index < ndev; index++) {
		srd = siril_resets + index;

		dev_dbg(dev, "%s using %s\n", srd->dev_name,
			srd->pdata.controller_name);

		pd = priv->resets.devices + index;
		*pd = platform_device_register_data(dev,
						    srd->dev_name,
						    PLATFORM_DEVID_NONE,
						    &srd->pdata,
						    sizeof(srd->pdata));
		if (NULL == *pd) {
			dev_err(dev, "cannot register %s resets with %s\n",
				srd->dev_name, srd->pdata.controller_name);
			return -1;
		}

		rc = ciena_siril_driver_override(*pd, srd->dev_name,
						 RESET_SYSFS_DRIVER_NAME);
		if (rc) return rc;
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
/*  Siril extra devices
 */
static int ciena_siril_extra_create(struct device *dev)
{
	struct platform_device              **pd;
	struct siril_priv                    *priv       = dev_get_drvdata(dev);
	const struct ciena_siril_extra_pdev  *extra_dev  = sirilx_pdata.sirilx_extra_devs;
	const struct ciena_siril_extra_pdev  *extra_devs = sirilx_pdata.sirilx_extra_devs;
	size_t                                ndev       = sirilx_pdata.sirilx_extra_devs_count;
	size_t                                index;

	if (0 == ndev) return 0;

	priv->extra_devs = devm_kzalloc(dev,
					(ndev + 1) * sizeof(*priv->extra_devs),
					GFP_KERNEL);

	if (NULL == priv->extra_devs) {
		dev_err(dev, "no memory for extra devices\n");
		return -ENOMEM;
	}

	for (index = 0, extra_dev = extra_devs, pd = priv->extra_devs;
	     index < ndev;
	     index++, extra_dev++, pd++) {
		*pd = create_pdev(dev, extra_dev);
		if (NULL == *pd) {
			dev_err(dev, "cannot registerextra device %s/%d\n",
				extra_dev->name, extra_dev->id);
			return -1;
		}
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
/*  Siril regmap
 */
static int ciena_siril_regmap_sysfs_create(struct device *dev)
{
	const char                       *mod_name = "regmap_sysfs";
	struct siril_priv                *priv     = dev_get_drvdata(dev);
	struct platform_device          **pd;
	struct ciena_sysfs_regmap_pdata   pdata;
	int                               bar;
	int                               rc;

	priv->regmap_sysfs = devm_kzalloc(dev,
					  (1 + CIENA_SIRIL_NUM_BARS) *
					  sizeof(*(priv->regmap_sysfs)),
					  GFP_KERNEL);

	if (NULL == priv->regmap_sysfs) {
		dev_err(dev, "no memory for regmap sysfs data\n");
		return -ENOMEM;
	}

	rc = request_module(mod_name);
	if (rc) dev_warn(dev, "cannot request_module(\"%s\")[%d]\n",
			 mod_name, rc);

	for (bar = 0; CIENA_SIRIL_NUM_BARS > bar; bar++) {
		char dev_name[128];

		snprintf(dev_name, sizeof(dev_name), "regmap-%s", MOD_NAME);
		memset(&pdata, 0, sizeof(pdata));
		pdata.name   = dev_name;
		pdata.regmap = priv->regs[bar];

		dev_dbg(dev, "%s\n", pdata.name);

		pd = priv->regmap_sysfs + bar;
		*pd = platform_device_register_data(dev,
						    pdata.name,
						    bar,
						    &pdata,
						    sizeof(pdata));
		if (NULL == *pd) {
			dev_err(dev, "cannot register %s regmap_sysfs\n",
				pdata.name);
			return -1;
		}

		rc = ciena_siril_driver_override(*pd, pdata.name,
						 REGMAP_SYSFS_DRIVER_NAME);
		if (rc) return rc;
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
/*  Siril led devices
 */
static int ciena_siril_leds_create(struct device *dev)
{
	struct siril_priv                  *priv     = dev_get_drvdata(dev);
	struct platform_device            **pd;
	const struct ciena_siril_led_pdev  *led_dev;
	const struct ciena_siril_led_pdev  *led_devs = sirilx_pdata.sirilx_led_devs;
	size_t                              ndev     = sirilx_pdata.sirilx_led_dev_count;
	size_t                              index;
	struct ciena_led_sysfs_pdata        pdata;

	if (0 == ndev) return 0;

	priv->led_devs = devm_kzalloc(dev,
				      (ndev + 1) * sizeof(*priv->led_devs),
				      GFP_KERNEL);

	if (NULL == priv->led_devs) {
		dev_err(dev, "no memory for led devices\n");
		return -ENOMEM;
	}

	for (index = 0, led_dev = led_devs, pd = priv->led_devs;
	     index < ndev;
	     index++, led_dev++, pd++) {
		pdata.name   = led_dev->name;
		pdata.reg    = led_dev->reg;
		pdata.mask   = led_dev->mask;
		pdata.val    = led_dev->val;
		pdata.use_val= led_dev->use_val;
		pdata.invert = led_dev->invert;
		pdata.blnk   = led_dev->blnk;
		*pd = platform_device_register_data(dev,
						    LED_SYSFS_DRIVER_NAME,
						    PLATFORM_DEVID_AUTO,
						    &pdata,
						    sizeof(pdata));
		if (NULL == *pd) {
			dev_err(dev, "cannot register led device %s\n",
				led_dev->name);
			return -1;
		}
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
/*  Siril thermal devices
 */
static int ciena_siril_thermal_create(struct device *dev)
{
	struct platform_device                **pd;
	struct siril_priv                      *priv       = dev_get_drvdata(dev);
	const struct ciena_siril_thermal_pdev  *therm_dev;
	const struct ciena_siril_thermal_pdev  *therm_devs = sirilx_pdata.sirilx_thermal_devs;
	size_t                                  ndev       = sirilx_pdata.sirilx_thermal_dev_count;
	size_t                                  index;
	struct ciena_thermal_pdata              pdata;

	if (0 == ndev) return 0;

	priv->thermal_devs = devm_kzalloc(dev,
				      (ndev + 1) * sizeof(*priv->thermal_devs),
				      GFP_KERNEL);

	if (NULL == priv->thermal_devs) {
		dev_err(dev, "no memory for thermal devices\n");
		return -ENOMEM;
	}

	for (index = 0, therm_dev = therm_devs, pd = priv->thermal_devs;
	     index < ndev;
	     index++, therm_dev++, pd++) {
		pdata.name          = therm_dev->name;
		pdata.regmap        = priv->regs[0];
		pdata.reg           = therm_dev->reg;
		pdata.valid_mask    = therm_dev->valid_mask;
		pdata.temp_mask     = therm_dev->temp_mask;
		pdata.temp_shift    = therm_dev->temp_shift;
		pdata.temp_unsigned = therm_dev->temp_unsigned;
		pdata.threshold     = therm_dev->threshold;
		*pd = platform_device_register_data(dev,
						    CIENA_THERMAL_NAME,
						    PLATFORM_DEVID_AUTO,
						    &pdata,
						    sizeof(pdata));
		if (NULL == *pd) {
			dev_err(dev, "cannot register thermal device %s\n",
				therm_dev->name);
			return -1;
		}
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
/*  Siril fan devices
 */
static int ciena_siril_fan_create(struct device *dev)
{
	struct platform_device       **pd;
	struct siril_priv             *priv     = dev_get_drvdata(dev);
	const struct ciena_fan_pdata  *fan_dev;
	const struct ciena_fan_pdata  *fan_devs = sirilx_pdata.sirilx_fan_devs;
	size_t                         ndev     = sirilx_pdata.sirilx_fan_dev_count;
	size_t                         index;
	struct ciena_fan_pdata         pdata;

	if (0 == ndev) return 0;

	priv->fan_devs = devm_kzalloc(dev,
				      (ndev + 1) * sizeof(*priv->fan_devs),
				      GFP_KERNEL);

	if (NULL == priv->fan_devs) {
		dev_err(dev, "no memory for fan devices\n");
		return -ENOMEM;
	}

	for (index = 0, fan_dev = fan_devs, pd = priv->fan_devs;
	     index < ndev;
	     index++, fan_dev++, pd++) {
		pdata        = *fan_dev;
		pdata.regmap = priv->regs[0];
		*pd = platform_device_register_data(dev,
						    CIENA_FAN_NAME,
						    PLATFORM_DEVID_AUTO,
						    &pdata,
						    sizeof(pdata));
		if (NULL == *pd) {
			dev_err(dev, "cannot register fan device %s\n",
				fan_dev->name);
			return -1;
		}
	}

	return 0;
}

/* -------------------------------------------------------------------------- */
/*  Siril watchdog IRQ handler
 */
static irqreturn_t ciena_siril_wdt_irq_handler(int irq, void *data)
{
	struct siril_priv *priv = data;
	struct device *dev      = regmap_get_device(priv->regs[0]);

	panic("%s/%s watchdog timeout!\n",
	      dev_name(dev),
	      sirilx_pdata.sirilx_wdt_irq_pin);

	return IRQ_HANDLED;
}

static int ciena_siril_wdt_irq_create(struct device *dev)
{
	struct siril_priv *priv = dev_get_drvdata(dev);
	int                irq;
	int                rc;

	if (NULL == sirilx_pdata.sirilx_wdt_irq_pin) return 0;

	irq = siril_resolve_cic_pin(priv->regs[0], &priv->cic->dev,
				    sirilx_pdata.sirilx_wdt_irq_pin);

	if (0 >= irq) {
		dev_err(dev, "cannot resolve pin %s in cic %s (%d)\n",
			sirilx_pdata.sirilx_wdt_irq_pin,
			dev_name(&priv->cic->dev),
			irq);
		return (irq ? irq : -EINVAL);
	}

	rc = request_threaded_irq(irq, NULL, ciena_siril_wdt_irq_handler,
				  0, sirilx_pdata.sirilx_wdt_irq_pin, priv);
	if (rc) dev_err(dev, "cannot request irq#%d (%d)\n", irq, rc);
	else priv->wdt_irq = irq;

	return rc;
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_i2c_mux_destroy(struct siril_i2c_priv *priv)
{
	int index = 0;

	while (priv->i2c_muxes && priv->i2c_muxes[index])
		index++;

	while (index--)
		platform_device_del(priv->i2c_muxes[index]);
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_i2c_masters_destroy(struct device *dev)
{
	struct siril_priv *priv = dev_get_drvdata(dev);
	int index = 0;

	while (priv->i2c_masters && priv->i2c_masters[index].i2c_master) {
		ciena_siril_i2c_mux_destroy(&priv->i2c_masters[index]);
		platform_device_del(priv->i2c_masters[index].i2c_master);
		index++;
	}

	/* disable the software-controlled I2C controllers */
	if (sirilx_pdata.sirilx_sw_i2c_enable)
		sirilx_pdata.sirilx_sw_i2c_enable(priv->regs[0], false);
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_cic_destroy(struct device *dev)
{
	struct siril_priv *priv = dev_get_drvdata(dev);

	if (priv->cic) {
		platform_device_del(priv->cic);
		if (sirilx_pdata.sirilx_irq_deresource)
			sirilx_pdata.sirilx_irq_deresource(dev);
		else
			ciena_siril_default_irq_deresource(dev);
	}
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_spi_masters_destroy(struct device *dev)
{
	struct siril_priv *priv = dev_get_drvdata(dev);
	int index = 0;

	while (priv->spi_masters && priv->spi_masters[index]) {
		platform_device_del(priv->spi_masters[index]);
		index++;
	}

	/* disable the offload FPGA eeprom */
	if (sirilx_pdata.sirilx_ofpga_eeprom_enable)
		sirilx_pdata.sirilx_ofpga_eeprom_enable(priv->regs[0], false);
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_uart_destroy(struct device *dev)
{
	struct siril_priv *priv = dev_get_drvdata(dev);
	int index = 0;

	while (priv->uart_masters && priv->uart_masters[index]) {
		platform_device_del(priv->uart_masters[index]);
		index++;
	}

	if (sirilx_pdata.sirilx_uart_buffer_enable)
		sirilx_pdata.sirilx_uart_buffer_enable(priv->regs[0], false);
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_reset_destroy(struct device *dev)
{
	struct siril_priv *priv = dev_get_drvdata(dev);
	int index;

	if (priv->resets.devices) {
		index = 0;
		while (priv->resets.devices[index]) {
			platform_device_del(priv->resets.devices[index]);
			index++;
		}
	}

	if (priv->resets.controls) {
		index = 0;
		while (priv->resets.controls[index]) {
			platform_device_del(priv->resets.controls[index]);
			index++;
		}
	}
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_regmap_sysfs_destroy(struct device *dev)
{
	struct siril_priv *priv  = dev_get_drvdata(dev);
	int                index = 0;

	while (priv->regmap_sysfs && priv->regmap_sysfs[index]) {
		platform_device_del(priv->regmap_sysfs[index]);
		index++;
	}
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_watchdog_destroy(struct device *dev)
{
	struct siril_priv *priv = dev_get_drvdata(dev);

	if (priv->watchdog)
		platform_device_del(priv->watchdog);
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_extra_destroy(struct device *dev)
{
	struct siril_priv *priv = dev_get_drvdata(dev);
	int                ndev = 0;

	if (NULL == priv->extra_devs) return;

	/* delete the extra devices in the reverse order they were created */
	while (priv->extra_devs[ndev]) ndev++;

	while (ndev--) platform_device_del(priv->extra_devs[ndev]);
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_leds_destroy(struct device *dev)
{
	struct siril_priv *priv = dev_get_drvdata(dev);
	int                ndev = 0;

	if (NULL == priv->led_devs) return;

	/* delete the led devices in the reverse order they were created */
	while (priv->led_devs[ndev]) ndev++;

	while (ndev--) platform_device_del(priv->led_devs[ndev]);
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_thermal_destroy(struct device *dev)
{
	struct siril_priv *priv = dev_get_drvdata(dev);
	int                ndev = 0;

	if (NULL == priv->thermal_devs) return;

	/* delete the thermal devices in the reverse order they were created */
	while (priv->thermal_devs[ndev]) ndev++;

	while (ndev--) platform_device_del(priv->thermal_devs[ndev]);
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_fan_destroy(struct device *dev)
{
	struct siril_priv *priv = dev_get_drvdata(dev);
	int                ndev = 0;

	if (NULL == priv->fan_devs) return;

	/* delete the fan devices in the reverse order they were created */
	while (priv->fan_devs[ndev]) ndev++;

	while (ndev--) platform_device_del(priv->fan_devs[ndev]);
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_wdt_irq_destroy(struct device *dev)
{
	struct siril_priv *priv = dev_get_drvdata(dev);

	if (priv->wdt_irq) free_irq(priv->wdt_irq, priv);
}

/* -------------------------------------------------------------------------- */
static int siril_resolve_cic_pin(struct regmap *regs,
				 struct device *cic_dev,
				 const char    *pin)
{
	struct device    *dev  = regmap_get_device(regs);
	struct gpio_desc *gd;
	int               gflags = GPIOD_ASIS | GPIOD_FLAGS_BIT_NONEXCLUSIVE;
	int               irq = 0;

	request_module("platform:" GPIO_NAME);

	gd = gpiod_get(cic_dev, pin, gflags);
	if (IS_ERR_OR_NULL(gd)) {
		dev_warn(dev, "cannot resolve %s/%s (%ld)\n",
			 dev_name(cic_dev), pin, PTR_ERR(gd));
		return irq;
	}

	irq = gpiod_to_irq(gd);

	if (0 >= irq) dev_warn(dev, "cannot convert %s/%s ti irq (%d)\n",
			       dev_name(cic_dev), pin, irq);
	else dev_info(dev, "using cic irq %d (%s/%s)\n", irq,
		      dev_name(cic_dev), pin);

	/*
	 * Do NOT call gpiod_put() here.  The NONEXCLUSIVE flag causes the
	 * kernel to return an already-requested descriptor WITHOUT taking a
	 * new module reference. The descriptor is owned by the CIC export
	 * path and will be properly released by europa_gpio_unexport().
	 */

	return irq;
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_dump_fpga_info(struct siril_priv *priv,
				       struct device     *dev)
{
	struct siril_id fpga_info = {};

	CIENA_FPGA_INFO(priv->regs[0], &fpga_info);

	dev_info(dev, "%s id:%x version:%02u.%02u.%02u",
		 MOD_NAME, fpga_info.fpga_id, fpga_info.major,
		 fpga_info.minor, fpga_info.build);
}

/* -------------------------------------------------------------------------- */
static noinline int ciena_siril_create_sub_devices(struct siril_priv *priv,
						   struct device     *dev)
{
	int rc;

	ciena_siril_dump_fpga_info(priv, dev);

	/* create subordinate devices */
	rc = ciena_siril_regmap_sysfs_create(dev);
	if (rc) goto out_regmap_destroy;

	rc = ciena_siril_cic_create(dev);
	if (rc) goto out_cic_destroy;

	rc = ciena_siril_i2c_masters_create(dev);
	if (rc) goto out_i2c_master_destroy;

	rc = ciena_siril_uart_create(dev);
	if (rc) goto out_uart_destroy;

	rc = ciena_siril_reset_create(dev);
	if (rc) goto out_reset_destroy;

	rc = ciena_siril_watchdog_create(dev);
	if (rc) goto out_watchdog_destroy;

	rc = ciena_siril_extra_create(dev);
	if (rc) goto out_extra_destroy;

	rc = ciena_siril_spi_masters_create(dev);
	if (rc) goto out_spi_destroy;

	rc = ciena_siril_leds_create(dev);
	if (rc) goto out_leds_destroy;

	rc = ciena_siril_thermal_create(dev);
	if (rc) goto out_thermal_destroy;

	rc = ciena_siril_fan_create(dev);
	if (rc) goto out_fan_destroy;

	rc = ciena_siril_wdt_irq_create(dev);
	if (rc) goto out_wdt_irq_destroy;

	return 0;

out_wdt_irq_destroy:
	ciena_siril_wdt_irq_destroy(dev);
out_fan_destroy:
	ciena_siril_fan_destroy(dev);
out_thermal_destroy:
	ciena_siril_thermal_destroy(dev);
out_leds_destroy:
	ciena_siril_leds_destroy(dev);
out_spi_destroy:
	ciena_siril_spi_masters_destroy(dev);
out_extra_destroy:
	ciena_siril_extra_destroy(dev);
out_watchdog_destroy:
	ciena_siril_watchdog_destroy(dev);
out_reset_destroy:
	ciena_siril_reset_destroy(dev);
out_uart_destroy:
	ciena_siril_uart_destroy(dev);
out_i2c_master_destroy:
	ciena_siril_i2c_masters_destroy(dev);
out_cic_destroy:
	ciena_siril_cic_destroy(dev);
out_regmap_destroy:
	ciena_siril_regmap_sysfs_destroy(dev);

	return rc;
}

/* -------------------------------------------------------------------------- */
static void ciena_siril_destroy_sub_devices(struct device *dev)
{
	ciena_siril_wdt_irq_destroy(dev);
	ciena_siril_fan_destroy(dev);
	ciena_siril_thermal_destroy(dev);
	ciena_siril_leds_destroy(dev);
	ciena_siril_spi_masters_destroy(dev);
	ciena_siril_extra_destroy(dev);
	ciena_siril_watchdog_destroy(dev);
	ciena_siril_reset_destroy(dev);
	ciena_siril_uart_destroy(dev);
	ciena_siril_i2c_masters_destroy(dev);
	ciena_siril_cic_destroy(dev);
	ciena_siril_regmap_sysfs_destroy(dev);
}

#ifdef CIENA_RESTART_MASTER
static ssize_t restart_master_show(struct device *dev, struct device_attribute *attr,
			    char *buf)
{
	return sysfs_emit(buf, "%d\n", CIENA_RESTART_MASTER);
}

static DEVICE_ATTR_RO(restart_master);
#endif

/* -------------------------------------------------------------------------- */
static void ciena_siril_post_functions(struct device *dev, bool is_add)
{
#ifdef CIENA_FPGA_FUNCTIONS
#define LINK_NAME "device"
	struct siril_priv *priv        = dev_get_drvdata(dev);
	struct device    **chipf_dev   = priv->chipf_dev;
	char              *token;
	char               envs[]      = CIENA_FPGA_FUNCTIONS;
	char              *buf         = envs;
	int                chipf_count = 0;
	int                rc;

	while ((token = strchr(buf, ',')) != NULL) {
		chipf_count += (token != buf);
		buf = token + 1;
	}
	chipf_count += ('\0' != *buf);

	buf = envs;
	if (is_add && chipf_count) {
		chipf_dev = devm_kzalloc(dev, chipf_count * sizeof(*chipf_dev),
					 GFP_KERNEL);
		if (NULL == chipf_dev)
			dev_err(dev, "no memory for %d chip functions\n",
				chipf_count);
	}
	priv->chipf_dev = chipf_dev;

	while (chipf_dev && (NULL != (token = strsep(&buf, ",")))) {
		if (!*token) continue;

		if (is_add) {
			*chipf_dev = root_device_register(token);
			if (IS_ERR_OR_NULL(*chipf_dev)) {
				dev_err(dev, "cannot register root device %s\n",
					token);
				*chipf_dev = NULL;
				continue;
			}

			rc = sysfs_create_link(&(*chipf_dev)->kobj, &dev->kobj,
					       LINK_NAME);
			if (rc) {
				dev_err(dev, "cannot link /sys/devices/%s "
					"[rc=%d]\n", token, rc);
				root_device_unregister(*chipf_dev);
				*chipf_dev = NULL;
				continue;
			}
			dev_info(dev, "created /sys/devices/%s\n", token);
		} else if (*chipf_dev) {
			sysfs_remove_link(&(*chipf_dev)->kobj, LINK_NAME);
			root_device_unregister(*chipf_dev);
			*chipf_dev = NULL;
			dev_info(dev, "removed /sys/devices/%s\n", token);
		}
		chipf_dev++;
	}
#ifdef CIENA_RESTART_MASTER
	if (is_add) {
		rc = device_create_file(dev, &dev_attr_restart_master);
		if (rc)
			dev_err(dev, "cannot create restart_master:%d\n", rc);
	} else
		device_remove_file(dev, &dev_attr_restart_master);
#endif
#endif
}

#else
#error "This file must only be included once."
#endif
// vim: sw=8 noet
