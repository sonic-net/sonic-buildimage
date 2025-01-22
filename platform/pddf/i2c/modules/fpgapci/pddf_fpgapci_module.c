/*
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
 * A pddf kernel module to create sysfs for fpga
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/dmi.h>
#include <linux/kobject.h>
#include "pddf_client_defs.h"
#include "pddf_fpgapci_defs.h"

FPGA_OPS_DATA tmp_pddf_fpga_ops_data={0};
extern int pddf_fpgapci_register(FPGA_OPS_DATA *ptr);

/**************************************************************************
 * FPGA SYSFS Attributes
 **************************************************************************/
static ssize_t dev_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count);

PDDF_DATA_ATTR(vendor_id, S_IWUSR|S_IRUGO, show_pddf_data,
                store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&tmp_pddf_fpga_ops_data.vendor_id, NULL);
PDDF_DATA_ATTR(device_id, S_IWUSR|S_IRUGO, show_pddf_data,
                store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&tmp_pddf_fpga_ops_data.device_id, NULL);
PDDF_DATA_ATTR(virt_bus, S_IWUSR|S_IRUGO, show_pddf_data,
                store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&tmp_pddf_fpga_ops_data.virt_bus, NULL);
PDDF_DATA_ATTR(data_base_offset, S_IWUSR|S_IRUGO, show_pddf_data,
                store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&tmp_pddf_fpga_ops_data.data_base_offset, NULL);
PDDF_DATA_ATTR(data_size, S_IWUSR|S_IRUGO, show_pddf_data,
                store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&tmp_pddf_fpga_ops_data.data_size, NULL);
PDDF_DATA_ATTR(i2c_ch_base_offset, S_IWUSR|S_IRUGO, show_pddf_data,
                store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&tmp_pddf_fpga_ops_data.i2c_ch_base_offset, NULL);
PDDF_DATA_ATTR(i2c_ch_size, S_IWUSR|S_IRUGO, show_pddf_data,
                store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&tmp_pddf_fpga_ops_data.i2c_ch_size, NULL);
PDDF_DATA_ATTR(virt_i2c_ch, S_IWUSR|S_IRUGO, show_pddf_data,
                store_pddf_data, PDDF_UINT32, sizeof(uint32_t), (void*)&tmp_pddf_fpga_ops_data.virt_i2c_ch, NULL);
PDDF_DATA_ATTR(dev_ops , S_IWUSR|S_IRUGO, show_pddf_data,
                dev_operation, PDDF_CHAR, NAME_SIZE, (void*)&tmp_pddf_fpga_ops_data, NULL);

struct attribute* attrs_fpgapci[]={
	&attr_vendor_id.dev_attr.attr,
	&attr_device_id.dev_attr.attr,
	&attr_virt_bus.dev_attr.attr,
	&attr_data_base_offset.dev_attr.attr,
	&attr_data_size.dev_attr.attr,
	&attr_i2c_ch_base_offset.dev_attr.attr,
	&attr_i2c_ch_size.dev_attr.attr,
	&attr_virt_i2c_ch.dev_attr.attr,
	&attr_dev_ops.dev_attr.attr,
	NULL,
};
struct attribute_group attr_group_fpgapci={
	.attrs = attrs_fpgapci,
};

ssize_t dev_operation(struct device *dev, struct device_attribute *da, const char *buf, size_t count)
{
	if(strncmp(buf, "fpgapci_init", strlen("fpgapci_init"))==0 ) {
		struct pddf_data_attribute *_ptr = (struct pddf_data_attribute *)da;
		FPGA_OPS_DATA* pddf_fpga_ops_data=(FPGA_OPS_DATA*)_ptr->addr;
		pddf_dbg(FPGA, KERN_INFO "%s: pddf_fpga_ops_data vendor_id=0x%x device_id=0x%x virt_bus=0x%x:%d "
              " data_base_offset=0x%x data_size=0x%x i2c_ch_base_offset=0x%x i2c_ch_size=0x%x virt_i2c_ch=%d",
		      __FUNCTION__, pddf_fpga_ops_data->vendor_id, pddf_fpga_ops_data->device_id,
              pddf_fpga_ops_data->virt_bus, pddf_fpga_ops_data->virt_bus,
              pddf_fpga_ops_data->data_base_offset, pddf_fpga_ops_data->data_size,
              pddf_fpga_ops_data->i2c_ch_base_offset, pddf_fpga_ops_data->i2c_ch_size, pddf_fpga_ops_data->virt_i2c_ch);

		pddf_fpgapci_register(pddf_fpga_ops_data);
	}
	else {
		pddf_dbg(FPGA, KERN_ERR "PDDF_ERROR %s: Invalid value for dev_ops %s\n", __FUNCTION__, buf);
	}
	return(count);
}


#define KOBJ_FREE(obj) \
        if(obj) kobject_put(obj); \


int __init pddf_fpga_data_init(void)
{
	int ret = 0;
	struct kobject *device_kobj;

	pddf_dbg(FPGA, KERN_INFO "%s ..\n", __FUNCTION__);

        device_kobj = get_device_i2c_kobj();
        if(!device_kobj) {
		pddf_dbg(FPGA, KERN_ERR "%s get_device_i2c_kobj failed ..\n", __FUNCTION__);
                return -ENOMEM;
	}
	fpgapci_kobj = kobject_create_and_add("fpgapci", device_kobj);
	if(!fpgapci_kobj) {
		pddf_dbg(FPGA, KERN_ERR "%s create fpgapci kobj failed ..\n", __FUNCTION__);
		return -ENOMEM;
	}

	ret = sysfs_create_group(fpgapci_kobj, &attr_group_fpgapci);
	if (ret)
	{
		pddf_dbg(FPGA, KERN_ERR "%s create fpga sysfs attributes failed ..\n", __FUNCTION__);
		return ret;
	}


        return (0);

}

void  __exit pddf_fpga_data_exit(void)
{
	pddf_dbg(FPGA, KERN_INFO "%s ..\n", __FUNCTION__);
	KOBJ_FREE(fpgapci_kobj)
	return;
}


module_init(pddf_fpga_data_init);
module_exit(pddf_fpga_data_exit);

MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION("fpga platform data");
MODULE_LICENSE("GPL");
