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
 *	Platform MULTIFPGAPCI I2C XIIC module for plumbing sysfs values
 */

#include <asm-generic/errno-base.h>
#include <linux/xarray.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include "pddf_client_defs.h"
#include "pddf_multifpgapci_defs.h"
#include "pddf_multifpgapci_i2c_xiic_defs.h"

DEFINE_XARRAY(i2c_xiic_drvdata_map);

extern unsigned long multifpgapci_get_pci_dev_index(struct pci_dev *);
extern int multifpgapci_register_protocol(const char *, struct protocol_ops *);
extern void multifpgapci_unregister_protocol(const char *);

ssize_t new_i2c_xiic_adapter(struct device *_dev, struct device_attribute *da,
							 const char *buf, size_t count) {
	int index, msi_domain, hw_irq;
	int sscanf_result = sscanf(buf, "%d %d %d", &index, &msi_domain, &hw_irq);
	if (sscanf_result != 3) {
		printk("%s: [%s] Failed to parse buf: %s\n", MULTIFPGA, __FUNCTION__,
			   buf);
		return -EINVAL;
	}
	if (index < 0 || index >= I2C_XIIC_MAX_BUS) {
		printk("%s: [%s] I2C XIIC Adapter %d out of range [0, %d)\n", I2C_XIIC,
			   __FUNCTION__, index, I2C_XIIC_MAX_BUS);
		return -EINVAL;
	}
	if (msi_domain < 0 || msi_domain >= MAX_NUM_MSI_VECTORS) {
		printk("%s: [%s] MSI domain %d out of range [0, %d)\n", I2C_XIIC,
			   __FUNCTION__, msi_domain, MAX_NUM_MSI_VECTORS);
		return -EINVAL;
	}

	struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
	struct pci_dev *pci_dev = (struct pci_dev *)_ptr->addr;
	struct pddf_multifpgapci_drvdata *pci_privdata = pci_get_drvdata(pci_dev);
	struct device *dev = &pci_dev->dev;
	struct i2c_xiic_adapter_drvdata *i2c_xiic_privdata;
	if (!pci_privdata) {
		pddf_dbg(
			I2C_XIIC,
			KERN_ERR
			"[%s] unable to retrieve pddf_multifpgapci_drvdata for device %s",
			__FUNCTION__, pci_name(pci_dev));
		return -ENODEV;
	}

	const int reg_width = pci_privdata->regmap_config.val_bits;
	if (hw_irq < 0 || hw_irq >= reg_width) {
		printk("%s: [%s] hw_irq %d out of range [0, %d)\n", I2C_XIIC,
			   __FUNCTION__, hw_irq, reg_width);
		return -EINVAL;
	}

	pddf_dbg(I2C_XIIC,
			 KERN_INFO "[%s] pci_dev %s index=%d msi_domain=%d hw_irq=%d\n",
			 __FUNCTION__, pci_name(pci_dev), index, msi_domain, hw_irq);

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	i2c_xiic_privdata = xa_load(&i2c_xiic_drvdata_map, dev_index);
	if (!i2c_xiic_privdata) {
		pddf_dbg(I2C_XIIC,
				 KERN_ERR
				 "[%s] unable to retrieve i2c_xiic_privdata for device %s",
				 __FUNCTION__, pci_name(pci_dev));
		return -ENODEV;
	}

	if (i2c_xiic_privdata->i2c_xiic_adapter_registered[index]) {
		pddf_dbg(I2C_XIIC, KERN_ERR "[%s] I2C XIIC Adapter %d already exists\n",
				 __FUNCTION__, index);
		return -EEXIST;
	}

	const int bus_num = i2c_xiic_privdata->virt_bus + index;

	int virq = regmap_irq_get_virq(
		pci_privdata->msi_domain_irq_chip_data[msi_domain], hw_irq);
	if (virq <= 0) {
		pddf_dbg(I2C_XIIC,
				 KERN_ERR "[%s] failed to get virq for msi_domain=%d hw_irq=%d\n",
				 __FUNCTION__, msi_domain, hw_irq);
		return virq;
	}
	pddf_dbg(I2C_XIIC,
			 KERN_INFO "[%s] got virq=%d for index=%d msi_domain=%d hw_irq=%d\n",
		 __FUNCTION__, virq,  index, msi_domain, hw_irq);

	const struct resource res[] = {
		DEFINE_RES_MEM((resource_size_t)(i2c_xiic_privdata->ch_base_offset_bar +
										 index * i2c_xiic_privdata->ch_size),
					   i2c_xiic_privdata->ch_size),
		DEFINE_RES_IRQ(virq),
	};

	struct platform_device *platform_device = platform_device_register_resndata(
		dev, "xiic-i2c", bus_num, res, ARRAY_SIZE(res), NULL, 0);
	if (IS_ERR(platform_device)) {
		pddf_dbg(I2C_XIIC,
				 KERN_ERR "[%s] platform_device_register_resndata failed for "
						  "i2c (index=%d)",
				 __FUNCTION__, index);
		return PTR_ERR(platform_device);
	}

	i2c_xiic_privdata->platform_devices[index] = platform_device;
	i2c_xiic_privdata->i2c_xiic_adapter_registered[index] = true;

	return count;
}

ssize_t del_i2c_xiic_adapter(struct device *dev, struct device_attribute *da,
							 const char *buf, size_t count) {
	int index, error;

	error = kstrtoint(buf, 10, &index);
	if (error != 0) {
		pddf_dbg(I2C_XIIC, KERN_ERR "Error converting string: %d\n", error);
		return -EINVAL;
	}

	if (index < 0 || index >= I2C_XIIC_MAX_BUS) {
		pddf_dbg(I2C_XIIC,
				 KERN_ERR "[%s] I2C XIIC Adapter %d out of range [0, %d)\n",
				 __FUNCTION__, index, I2C_XIIC_MAX_BUS);
		return -EINVAL;
	}

	struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
	struct pci_dev *pci_dev = (struct pci_dev *)_ptr->addr;
	struct i2c_xiic_adapter_drvdata *i2c_xiic_privdata;

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	i2c_xiic_privdata = xa_load(&i2c_xiic_drvdata_map, dev_index);
	if (!i2c_xiic_privdata) {
		pddf_dbg(I2C_XIIC,
				 KERN_ERR
				 "[%s] unable to retrieve i2c_xiic_privdata for device %s",
				 __FUNCTION__, pci_name(pci_dev));
		return -ENODEV;
	}

	pddf_dbg(I2C_XIIC,
			 KERN_INFO
			 "[%s] Attempting delete of XIIC platform device index: %d\n",
			 __FUNCTION__, index);

	if (i2c_xiic_privdata->platform_devices[index]) {
		platform_device_unregister(i2c_xiic_privdata->platform_devices[index]);
		i2c_xiic_privdata->platform_devices[index] = NULL;
	}
	i2c_xiic_privdata->i2c_xiic_adapter_registered[index] = false;

	return count;
}

static int pddf_multifpgapci_i2c_xiic_attach(struct pci_dev *pci_dev, struct kobject *kobj) {
	pddf_dbg(I2C_XIIC, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
			 pci_name(pci_dev));
	struct i2c_xiic_adapter_drvdata *i2c_xiic_privdata;
	int err;

	i2c_xiic_privdata =
		kzalloc(sizeof(struct i2c_xiic_adapter_drvdata), GFP_KERNEL);
	if (!i2c_xiic_privdata)
		return -ENOMEM;

	i2c_xiic_privdata->pci_dev = pci_dev;
	i2c_xiic_privdata->i2c_xiic_kobj = kobject_create_and_add("i2c-xiic", kobj);
	if (!i2c_xiic_privdata->i2c_xiic_kobj) {
		pddf_dbg(I2C_XIIC, KERN_ERR "[%s] create i2c-xiic kobj failed\n",
				 __FUNCTION__);
		err = -ENOMEM;
		goto free_privdata;
	}

	PDDF_DATA_ATTR(virt_bus, S_IWUSR | S_IRUGO, show_pddf_data, store_pddf_data,
				   PDDF_UINT32, sizeof(uint32_t),
				   (void *)&i2c_xiic_privdata->temp_sysfs_vals.virt_bus, NULL);
	PDDF_DATA_ATTR(ch_base_offset, S_IWUSR | S_IRUGO, show_pddf_data,
				   store_pddf_data, PDDF_UINT32, sizeof(uint32_t),
				   (void *)&i2c_xiic_privdata->temp_sysfs_vals.ch_base_offset,
				   NULL);
	PDDF_DATA_ATTR(ch_size, S_IWUSR | S_IRUGO, show_pddf_data, store_pddf_data,
				   PDDF_UINT32, sizeof(uint32_t),
				   (void *)&i2c_xiic_privdata->temp_sysfs_vals.ch_size, NULL);
	PDDF_DATA_ATTR(num_virt_ch, S_IWUSR | S_IRUGO, show_pddf_data,
				   store_pddf_data, PDDF_UINT32, sizeof(uint32_t),
				   (void *)&i2c_xiic_privdata->temp_sysfs_vals.num_virt_ch,
				   NULL);
	PDDF_DATA_ATTR(new_i2c_xiic_adapter, S_IWUSR | S_IRUGO, show_pddf_data,
				   new_i2c_xiic_adapter, PDDF_CHAR, NAME_SIZE, (void *)pci_dev,
				   NULL);
	PDDF_DATA_ATTR(del_i2c_xiic_adapter, S_IWUSR | S_IRUGO, show_pddf_data,
				   del_i2c_xiic_adapter, PDDF_CHAR, NAME_SIZE, (void *)pci_dev,
				   NULL);

	i2c_xiic_privdata->attrs.attr_virt_bus = attr_virt_bus;
	i2c_xiic_privdata->attrs.attr_ch_base_offset = attr_ch_base_offset;
	i2c_xiic_privdata->attrs.attr_ch_size = attr_ch_size;
	i2c_xiic_privdata->attrs.attr_num_virt_ch = attr_num_virt_ch;
	i2c_xiic_privdata->attrs.attr_new_i2c_xiic_adapter =
		attr_new_i2c_xiic_adapter;
	i2c_xiic_privdata->attrs.attr_del_i2c_xiic_adapter =
		attr_del_i2c_xiic_adapter;

	struct attribute *i2c_xiic_attrs[NUM_I2C_XIIC_ATTRS + 1] = {
		&i2c_xiic_privdata->attrs.attr_virt_bus.dev_attr.attr,
		&i2c_xiic_privdata->attrs.attr_ch_base_offset.dev_attr.attr,
		&i2c_xiic_privdata->attrs.attr_ch_size.dev_attr.attr,
		&i2c_xiic_privdata->attrs.attr_num_virt_ch.dev_attr.attr,
		&i2c_xiic_privdata->attrs.attr_new_i2c_xiic_adapter.dev_attr.attr,
		&i2c_xiic_privdata->attrs.attr_del_i2c_xiic_adapter.dev_attr.attr,
		NULL,
	};

	memcpy(i2c_xiic_privdata->i2c_xiic_attrs, i2c_xiic_attrs,
		   sizeof(i2c_xiic_privdata->i2c_xiic_attrs));

	i2c_xiic_privdata->i2c_xiic_attr_group.attrs =
		i2c_xiic_privdata->i2c_xiic_attrs;

	err = sysfs_create_group(i2c_xiic_privdata->i2c_xiic_kobj,
							 &i2c_xiic_privdata->i2c_xiic_attr_group);
	if (err) {
		pddf_dbg(I2C_XIIC,
				 KERN_ERR "[%s] sysfs_create_group error, status: %d\n",
				 __FUNCTION__, err);
		goto put_kobj;
	}
	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	void *old = xa_store(&i2c_xiic_drvdata_map, dev_index, i2c_xiic_privdata, GFP_KERNEL);
	if (xa_is_err(old)) {
		err = xa_err(old);
		pddf_dbg(I2C_XIIC,
				 KERN_ERR "[%s] xa_store failed, status: %d\n",
				 __FUNCTION__, err);
		goto remove_sysfs;
	}

	return 0;

remove_sysfs:
	sysfs_remove_group(i2c_xiic_privdata->i2c_xiic_kobj,
					   &i2c_xiic_privdata->i2c_xiic_attr_group);

put_kobj:
	kobject_put(i2c_xiic_privdata->i2c_xiic_kobj);

free_privdata:
	kfree(i2c_xiic_privdata);
	return err;
}

static void pddf_multifpgapci_i2c_xiic_detach(struct pci_dev *pci_dev, struct kobject *kobj) {
	struct i2c_xiic_adapter_drvdata *i2c_xiic_privdata;
	pddf_dbg(I2C_XIIC, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
			 pci_name(pci_dev));

	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	i2c_xiic_privdata = xa_load(&i2c_xiic_drvdata_map, dev_index);
	if (!i2c_xiic_privdata) {
		pddf_dbg(I2C_XIIC,
				 KERN_ERR
				 "[%s] unable to find i2c xiic module data for device %s\n",
				 __FUNCTION__, pci_name(pci_dev));
		return;
	}

	for (unsigned int i = 0; i < I2C_XIIC_MAX_BUS; i++) {
		if (i2c_xiic_privdata->platform_devices[i])
			platform_device_unregister(
				i2c_xiic_privdata->platform_devices[i]);
	}

	if (i2c_xiic_privdata->i2c_xiic_kobj) {
		sysfs_remove_group(i2c_xiic_privdata->i2c_xiic_kobj,
						   &i2c_xiic_privdata->i2c_xiic_attr_group);
		kobject_put(i2c_xiic_privdata->i2c_xiic_kobj);
		i2c_xiic_privdata->i2c_xiic_kobj = NULL;
	}
	
	xa_erase(&i2c_xiic_drvdata_map, dev_index);

	kfree(i2c_xiic_privdata);
	i2c_xiic_privdata = NULL;
}

static void pddf_multifpgapci_i2c_xiic_map_bar(struct pci_dev *pci_dev,
											   void __iomem *bar_base,
											   unsigned long bar_start,
											   unsigned long bar_len) {
	struct i2c_xiic_adapter_drvdata *i2c_xiic_privdata;
	pddf_dbg(I2C_XIIC, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
			 pci_name(pci_dev));
	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	i2c_xiic_privdata = xa_load(&i2c_xiic_drvdata_map, dev_index);
	if (!i2c_xiic_privdata) {
		pddf_dbg(I2C_XIIC,
				 KERN_ERR
				 "[%s] unable to find i2c xiic module data for device %s\n",
				 __FUNCTION__, pci_name(pci_dev));
		return;
	}
	// i2c xiic specific data store
	struct i2c_xiic_sysfs_vals *i2c_xiic_pddf_data =
		&i2c_xiic_privdata->temp_sysfs_vals;

	i2c_xiic_privdata->virt_bus = i2c_xiic_pddf_data->virt_bus;
	i2c_xiic_privdata->num_virt_ch = i2c_xiic_pddf_data->num_virt_ch;
	i2c_xiic_privdata->ch_size = i2c_xiic_pddf_data->ch_size;
	i2c_xiic_privdata->ch_base_offset_bar =
		bar_start + i2c_xiic_pddf_data->ch_base_offset;
}

static void pddf_multifpgapci_i2c_xiic_unmap_bar(struct pci_dev *pci_dev,
												 void __iomem *bar_base,
												 unsigned long bar_start,
												 unsigned long bar_len) {
	struct i2c_xiic_adapter_drvdata *i2c_xiic_privdata;
	pddf_dbg(I2C_XIIC, KERN_INFO "[%s] pci_dev %s\n", __FUNCTION__,
			 pci_name(pci_dev));
	unsigned dev_index = multifpgapci_get_pci_dev_index(pci_dev);
	i2c_xiic_privdata = xa_load(&i2c_xiic_drvdata_map, dev_index);
	if (!i2c_xiic_privdata) {
		pddf_dbg(I2C_XIIC,
				 KERN_ERR
				 "[%s] unable to find i2c xiic module data for device %s\n",
				 __FUNCTION__, pci_name(pci_dev));
		return;
	}
	i2c_xiic_privdata->ch_base_offset_bar = 0;
}

static struct protocol_ops i2c_xiic_protocol_ops = {
	.attach = pddf_multifpgapci_i2c_xiic_attach,
	.detach = pddf_multifpgapci_i2c_xiic_detach,
	.map_bar = pddf_multifpgapci_i2c_xiic_map_bar,
	.unmap_bar = pddf_multifpgapci_i2c_xiic_unmap_bar,
	.name = "i2c-xiic",
};

static int __init pddf_multifpgapci_i2c_xiic_init(void) {
	pddf_dbg(I2C_XIIC, KERN_INFO "Loading I2C XIIC protocol module\n");
	xa_init(&i2c_xiic_drvdata_map);
	return multifpgapci_register_protocol("i2c-xiic", &i2c_xiic_protocol_ops);
}

static void __exit pddf_multifpgapci_i2c_xiic_exit(void) {
	pddf_dbg(I2C_XIIC, KERN_INFO "Unloading I2C XIIC protocol module\n");
	multifpgapci_unregister_protocol("i2c-xiic");
	xa_destroy(&i2c_xiic_drvdata_map);
}

module_init(pddf_multifpgapci_i2c_xiic_init);
module_exit(pddf_multifpgapci_i2c_xiic_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nexthop Systems");
MODULE_DESCRIPTION(
	"PDDF Platform Data for Multiple PCI FPGA I2C XIIC adapters.");
