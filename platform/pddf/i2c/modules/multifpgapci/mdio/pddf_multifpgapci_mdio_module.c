/*
 * Copyright 2025 Nexthop Systems Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Description:
 *	PDDF MULTIFPGAPCI kernel module for registering MDIO buses.
 */

#include <asm-generic/errno-base.h>
#include <linux/xarray.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/phy.h>
#include <linux/string.h>

#include "pddf_multifpgapci_defs.h"
#include "pddf_multifpgapci_mdio_defs.h"

DEFINE_XARRAY(mdio_drvdata_map);

struct mdio_fpga_ops *mdio_fpga_algo_ops = NULL;
EXPORT_SYMBOL_GPL(mdio_fpga_algo_ops);

static ssize_t mdio_access_show(struct device *dev,
				struct device_attribute *attr, char *buf);
static ssize_t mdio_access_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count);

static DEVICE_ATTR_RW(mdio_access);

static ssize_t mdio_access_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	struct mii_bus *bus = to_mii_bus(dev);
	struct fpga_mdio_priv *priv = bus->priv;
	char op[10];
	int phy_addr, reg_num, val;
	int ret = -EINVAL;

	int num_args =
		sscanf(buf, "%9s %i %i %i", op, &phy_addr, &reg_num, &val);
	if (num_args < 1) {
		pddf_dbg(MULTIFPGA, KERN_ERR
			 "Invalid MDIO access format. No command provided.\n");
		return -EINVAL;
	}

	if (strcmp(op, "write") == 0) {
		// MDIO write
		if (num_args < 4) {
			pddf_dbg(
				MULTIFPGA, KERN_ERR
				"Invalid MDIO write access format. Expected 'write <phy_addr> <reg_num> <value>'\n");
			return -EINVAL;
		}
		ret = mdio_fpga_algo_ops->write(bus, phy_addr, reg_num, val);
		if (ret == 0) {
			return count;
		} else {
			pddf_dbg(MULTIFPGA,
				 KERN_ERR
				 "MDIO write failed for phy=%d, reg=0x%x\n",
				 phy_addr, reg_num);
			return ret;
		}
	} else if (strcmp(op, "read") == 0) {
		// MDIO read - the result is stored in priv->last_read_value.
		if (num_args < 3) {
			pddf_dbg(
				MULTIFPGA, KERN_ERR
				"Invalid MDIO read access format. Expected 'read <phy_addr> <reg_num>'\n");
			return -EINVAL;
		}
		ret = mdio_fpga_algo_ops->read(bus, phy_addr, reg_num);
		if (ret >= 0) {
			priv->last_read_value = ret;
			return count;
		} else {
			pddf_dbg(MULTIFPGA,
				 KERN_ERR
				 "MDIO read failed for phy=%d, reg=0x%x\n",
				 phy_addr, reg_num);
			return ret;
		}
	} else {
		pddf_dbg(MULTIFPGA, KERN_ERR "Invalid MDIO operation: %s\n",
			 op);
		return -EINVAL;
	}

	return ret;
}

// The show function returns the value of the last read operation to user space.
// This is triggered by a cat command on the mdio_access file.
static ssize_t mdio_access_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct mii_bus *bus = to_mii_bus(dev);
	struct fpga_mdio_priv *priv = bus->priv;

	return scnprintf(buf, PAGE_SIZE, "0x%x\n", priv->last_read_value);
}

ssize_t new_mdio_bus(struct device *dev, struct device_attribute *da,
		     const char *buf, size_t count)
{
	int index, err;
	struct mii_bus *new_bus = NULL;
	struct fpga_mdio_priv *algo_priv;

	err = kstrtoint(buf, 10, &index);
	if (err != 0) {
		pddf_dbg(MULTIFPGA, KERN_ERR "Error converting string: %d\n",
			 err);
		return -EINVAL;
	}

	if (index < 0 || index >= MDIO_MAX_BUS) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] MDIO bus %d out of range [0, %d)\n",
			 __FUNCTION__, index, MDIO_MAX_BUS);
		return -ENODEV;
	}

	struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
	struct pci_dev *pci_dev = (struct pci_dev *)_ptr->addr;
	struct mdio_bus_drvdata *mdio_privdata;

	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	mdio_privdata = xa_load(&mdio_drvdata_map, dev_index);
	if (!mdio_privdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to retrieve mdio_privdata for device %s",
			 __FUNCTION__, pci_name(pci_dev));
		return -ENODEV;
	}

	if (mdio_privdata->mdio_bus_registered[index]) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] MDIO bus %d already registered\n",
			 __FUNCTION__, index);
		return -ENODEV;
	}

	new_bus = mdiobus_alloc();
	if (!new_bus) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] Failed to allocate MDIO bus %d\n",
			 __FUNCTION__, index);
		return -ENOMEM;
	}

	// Allocate the private data for the MDIO algorithm
	algo_priv = kzalloc(sizeof(*algo_priv), GFP_KERNEL);
	if (!algo_priv) {
		pddf_dbg(
			MULTIFPGA,
			KERN_ERR
			"[%s] Failed to allocate FPGA MDIO algo private data\n",
			__FUNCTION__);
		mdiobus_free(new_bus);
		return -ENOMEM;
	}
	algo_priv->reg_base =
		mdio_privdata->ch_base_addr + index * mdio_privdata->ch_size;
	mutex_init(&algo_priv->lock);
	new_bus->priv = algo_priv;

	mdio_privdata->mdio_buses[index] = new_bus;

	new_bus->name = "pci-mdio-bus";
	snprintf(new_bus->id, MII_BUS_ID_SIZE, "pci-mdio-%d", index);

	if (!mdio_fpga_algo_ops || !mdio_fpga_algo_ops->read ||
	    !mdio_fpga_algo_ops->write) {
		pddf_dbg(
			MULTIFPGA,
			KERN_ERR
			"[%s] MDIO FPGA algorithm module not loaded or incomplete!\n",
			__FUNCTION__);
		goto err_cleanup;
	}

	new_bus->read = mdio_fpga_algo_ops->read;
	new_bus->write = mdio_fpga_algo_ops->write;
	new_bus->owner = THIS_MODULE;
	new_bus->parent = &pci_dev->dev;

	err = mdiobus_register(new_bus);
	if (err != 0) {
		pddf_dbg(MULTIFPGA, KERN_ERR "Could not register MDIO bus %d\n",
			 index);
		goto err_cleanup;
	}
	mdio_privdata->mdio_bus_registered[index] = true;

	// Attach the new sysfs path for user-space access
	err = device_create_file(&new_bus->dev, &dev_attr_mdio_access);
	if (err) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "Failed to create sysfs file for MDIO bus %s: %d\n",
			 dev_name(&new_bus->dev), err);
		goto err_cleanup_registered;
	}

	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] Registered MDIO bus id: %s\n",
		 __FUNCTION__, dev_name(&new_bus->dev));

	return count;

err_cleanup_registered:
	mdiobus_unregister(new_bus);

err_cleanup:
	if (new_bus) {
		mdiobus_free(new_bus);
		mdio_privdata->mdio_buses[index] = NULL;
	}
	if (algo_priv) {
		kfree(algo_priv);
	}
	mdio_privdata->mdio_bus_registered[index] = false;
	return err;
}

ssize_t del_mdio_bus(struct device *dev, struct device_attribute *da,
		     const char *buf, size_t count)
{
	int index, error;

	error = kstrtoint(buf, 10, &index);
	if (error != 0) {
		pddf_dbg(MULTIFPGA, KERN_ERR "Error converting string: %d\n",
			 error);
		return -EINVAL;
	}

	if (index < 0 || index >= MDIO_MAX_BUS) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] MDIO bus %d out of range [0, %d)\n",
			 __FUNCTION__, index, MDIO_MAX_BUS);
		return -ENODEV;
	}

	struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
	struct pci_dev *pci_dev = (struct pci_dev *)_ptr->addr;
	struct mdio_bus_drvdata *mdio_privdata;

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	mdio_privdata = xa_load(&mdio_drvdata_map, dev_index);
	if (!mdio_privdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to retrieve mdio_privdata for device %s",
			 __FUNCTION__, pci_name(pci_dev));
		return -ENODEV;
	}

	struct mii_bus *bus_to_unregister = mdio_privdata->mdio_buses[index];

	if (!bus_to_unregister) {
		pddf_dbg(
			MULTIFPGA,
			KERN_ERR
			"%s: MDIO bus %d marked registered but poiner is NULL\n",
			__FUNCTION__, index);
		return -ENODEV;
	}

	pddf_dbg(MULTIFPGA,
		 KERN_INFO "[%s] Attempting to unregister MDIO bus: %d\n",
		 __FUNCTION__, index);

	// Remove the custom sysfs file
	device_remove_file(&bus_to_unregister->dev, &dev_attr_mdio_access);

	mdiobus_unregister(bus_to_unregister);
	mdiobus_free(bus_to_unregister);

	mdio_privdata->mdio_bus_registered[index] = false;
	mdio_privdata->mdio_buses[index] = NULL;

	return count;
}

int pddf_multifpgapci_mdio_attach(struct pci_dev *pci_dev, struct kobject *kobj)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));
	struct mdio_bus_drvdata *mdio_privdata;
	int err;

	mdio_privdata = kzalloc(sizeof(struct mdio_bus_drvdata), GFP_KERNEL);
	if (!mdio_privdata) {
		return -ENOMEM;
	}
	mdio_privdata->pci_dev = pci_dev;

	mdio_privdata->mdio_kobj = kobject_create_and_add("mdio", kobj);
	if (!mdio_privdata->mdio_kobj) {
		pddf_dbg(MULTIFPGA, KERN_ERR "[%s] create mdio kobj failed\n",
			 __FUNCTION__);
		err = -ENOMEM;
		goto free_privdata;
	}

	memset(mdio_privdata->mdio_bus_registered, 0,
	       sizeof(mdio_privdata->mdio_bus_registered));

	PDDF_DATA_ATTR(ch_base_offset, S_IWUSR | S_IRUGO, show_pddf_data,
		       store_pddf_data, PDDF_UINT32, sizeof(uint32_t),
		       (void *)&mdio_privdata->temp_sysfs_vals.ch_base_offset,
		       NULL);

	PDDF_DATA_ATTR(ch_size, S_IWUSR | S_IRUGO, show_pddf_data,
		       store_pddf_data, PDDF_UINT32, sizeof(uint32_t),
		       (void *)&mdio_privdata->temp_sysfs_vals.ch_size, NULL);

	PDDF_DATA_ATTR(num_virt_ch, S_IWUSR | S_IRUGO, show_pddf_data,
		       store_pddf_data, PDDF_UINT32, sizeof(uint32_t),
		       (void *)&mdio_privdata->temp_sysfs_vals.num_virt_ch,
		       NULL);

	PDDF_DATA_ATTR(new_mdio_bus, S_IWUSR | S_IRUGO, show_pddf_data,
		       new_mdio_bus, PDDF_CHAR, NAME_SIZE, (void *)pci_dev,
		       NULL);

	PDDF_DATA_ATTR(del_mdio_bus, S_IWUSR | S_IRUGO, show_pddf_data,
		       del_mdio_bus, PDDF_CHAR, NAME_SIZE, (void *)pci_dev,
		       NULL);

	mdio_privdata->attrs.attr_ch_base_offset = attr_ch_base_offset;
	mdio_privdata->attrs.attr_ch_size = attr_ch_size;
	mdio_privdata->attrs.attr_num_virt_ch = attr_num_virt_ch;
	mdio_privdata->attrs.attr_new_mdio_bus = attr_new_mdio_bus;
	mdio_privdata->attrs.attr_del_mdio_bus = attr_del_mdio_bus;

	struct attribute *mdio_bus_attrs[NUM_MDIO_BUS_ATTRS + 1] = {
		&mdio_privdata->attrs.attr_ch_base_offset.dev_attr.attr,
		&mdio_privdata->attrs.attr_ch_size.dev_attr.attr,
		&mdio_privdata->attrs.attr_num_virt_ch.dev_attr.attr,
		&mdio_privdata->attrs.attr_new_mdio_bus.dev_attr.attr,
		&mdio_privdata->attrs.attr_del_mdio_bus.dev_attr.attr,
		NULL,
	};

	memcpy(mdio_privdata->mdio_bus_attrs, mdio_bus_attrs,
	       sizeof(mdio_privdata->mdio_bus_attrs));

	mdio_privdata->mdio_bus_attr_group.attrs =
		mdio_privdata->mdio_bus_attrs;

	err = sysfs_create_group(mdio_privdata->mdio_kobj,
				 &mdio_privdata->mdio_bus_attr_group);
	if (err) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR "[%s] sysfs_create_group error, status: %d\n",
			 __FUNCTION__, err);
		goto free_kobj;
	}
	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	xa_store(&mdio_drvdata_map, dev_index, mdio_privdata, GFP_KERNEL);

	return 0;

free_kobj:
	kobject_put(mdio_privdata->mdio_kobj);
free_privdata:
	kfree(mdio_privdata);
	return err;
}

static void pddf_multifpgapci_mdio_detach(struct pci_dev *pci_dev,
					  struct kobject *kobj)
{
	struct mdio_bus_drvdata *mdio_privdata;
	int i;
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	mdio_privdata = xa_load(&mdio_drvdata_map, dev_index);
	if (!mdio_privdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to find mdio module data for device %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return;
	}

	for (i = 0; i < MDIO_MAX_BUS; i++) {
		if (mdio_privdata->mdio_bus_registered[i]) {
			pddf_dbg(MULTIFPGA,
				 KERN_INFO "[%s] unregistering MDIO bus: %s\n",
				 __FUNCTION__,
				 mdio_privdata->mdio_buses[i]->name);
			mdiobus_unregister(mdio_privdata->mdio_buses[i]);
		}
	}
	if (mdio_privdata->mdio_kobj) {
		sysfs_remove_group(mdio_privdata->mdio_kobj,
				   &mdio_privdata->mdio_bus_attr_group);
		kobject_put(mdio_privdata->mdio_kobj);
		mdio_privdata->mdio_kobj = NULL;
	}
	xa_erase(&mdio_drvdata_map, (unsigned long)pci_dev);
}

static void pddf_multifpgapci_mdio_map_bar(struct pci_dev *pci_dev,
					   void __iomem *bar_base,
					   unsigned long bar_start,
					   unsigned long bar_len)
{
	struct mdio_bus_drvdata *mdio_privdata;
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));
	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	mdio_privdata = xa_load(&mdio_drvdata_map, dev_index);
	if (!mdio_privdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to find mdio module data for device %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return;
	}
	struct mdio_bus_sysfs_vals *mdio_pddf_data =
		&mdio_privdata->temp_sysfs_vals;

	mdio_privdata->ch_base_addr = bar_base + mdio_pddf_data->ch_base_offset;
	mdio_privdata->num_virt_ch = mdio_pddf_data->num_virt_ch;
	mdio_privdata->ch_size = mdio_pddf_data->ch_size;
}

static void pddf_multifpgapci_mdio_unmap_bar(struct pci_dev *pci_dev,
					     void __iomem *bar_base,
					     unsigned long bar_start,
					     unsigned long bar_len)
{
	struct mdio_bus_drvdata *mdio_privdata;
	pddf_dbg(MULTIFPGA, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
		 pci_name(pci_dev));
	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	mdio_privdata = xa_load(&mdio_drvdata_map, dev_index);
	if (!mdio_privdata) {
		pddf_dbg(MULTIFPGA,
			 KERN_ERR
			 "[%s] unable to find mdio module data for device %s\n",
			 __FUNCTION__, pci_name(pci_dev));
		return;
	}
	mdio_privdata->ch_base_addr = NULL;
}

static struct protocol_ops mdio_protocol_ops = {
	.attach = pddf_multifpgapci_mdio_attach,
	.detach = pddf_multifpgapci_mdio_detach,
	.map_bar = pddf_multifpgapci_mdio_map_bar,
	.unmap_bar = pddf_multifpgapci_mdio_unmap_bar,
	.name = "mdio",
};

static int __init pddf_multifpgapci_mdio_init(void)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "Loading MDIO protocol module\n");
	xa_init(&mdio_drvdata_map);
	return multifpgapci_register_protocol("mdio", &mdio_protocol_ops);
}

static void __exit pddf_multifpgapci_mdio_exit(void)
{
	pddf_dbg(MULTIFPGA, KERN_INFO "Unloading MDIO protocol module\n");
	multifpgapci_unregister_protocol("mdio");
	xa_destroy(&mdio_drvdata_map);
}

module_init(pddf_multifpgapci_mdio_init);
module_exit(pddf_multifpgapci_mdio_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nexthop Systems");
MODULE_DESCRIPTION(
	"PDDF MULTIFPGAPCI kernel module for registering MDIO buses.");
