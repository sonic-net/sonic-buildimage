/*
*
* Licensed under the GNU General Public License Version 2
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
*/

/*
* pddf_fpgapci_driver.c
* Description:
*   This is a PDDF FPGAPCIe driver whic creates the PCIE device and add
*   the i2c adapters to it. It uses the adapter creation and fpgapcie
*   read/write functions defined separately in another kernel module.
*
************************************************************************/
#define __STDC_WANT_LIB_EXT1__ 1
#include <linux/string.h>
#include <linux/kobject.h>
#include <linux/kdev_t.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/uaccess.h>
#include <linux/sched.h>

#include <asm/siginfo.h>    //siginfo
#include <linux/rcupdate.h> //rcu_read_lock
#include <linux/version.h>  //kernel_version
#include <linux/slab.h>
#include <linux/irqdomain.h>
#include <linux/workqueue.h>
#include <linux/i2c.h>
#include <linux/moduleparam.h>
#include "pddf_fpgapci_defs.h"
#include "pddf_client_defs.h"
#include "pddf_i2c_algo.h"



#define DEBUG 0
int (*pddf_i2c_pci_add_numbered_bus)(struct i2c_adapter *, int) = NULL;
int (*ptr_fpgapci_read)(uint32_t) = NULL;
int (*ptr_fpgapci_write)(uint32_t, uint32_t) = NULL;
EXPORT_SYMBOL(pddf_i2c_pci_add_numbered_bus);
EXPORT_SYMBOL(ptr_fpgapci_read);
EXPORT_SYMBOL(ptr_fpgapci_write);

FPGA_OPS_DATA pddf_fpga_ops_data={0};

#define DRIVER_NAME			"pddf_fpgapci"
#define MAX_PCI_NUM_BARS	6

struct pci_driver pddf_fpgapci_driver;
struct pci_device_id *pddf_fpgapci_ids=NULL;
int total_i2c_pci_bus=0;
int FPGAPCI_BAR_INDEX = -1;

void __iomem * fpga_ctl_addr = NULL;
EXPORT_SYMBOL(fpga_ctl_addr);

static int pddf_fpgapci_probe(struct pci_dev *dev, const struct pci_device_id *id);
static void pddf_fpgapci_remove(struct pci_dev *dev);
static int map_bars(struct fpgapci_devdata *pci_privdata, struct pci_dev *dev);
static void free_bars(struct fpgapci_devdata *pci_privdata, struct pci_dev *dev);
static int pddf_pci_add_adapter(struct pci_dev *dev);

/* each i2c bus is represented in linux using struct i2c_adapter */
static struct i2c_adapter 	i2c_pci_adap[I2C_PCI_MAX_BUS];

static int pddf_pci_add_adapter(struct pci_dev *dev)
{
	int i;

    total_i2c_pci_bus = pddf_fpga_ops_data.virt_i2c_ch;
    pddf_dbg(FPGA, KERN_INFO "[%s] total_i2c_pci_bus=%d\n", __FUNCTION__, total_i2c_pci_bus);
#ifdef __STDC_LIB_EXT1__
    memset_s(&i2c_pci_adap, sizeof(i2c_pci_adap), 0, sizeof(i2c_pci_adap));
#else
	memset(&i2c_pci_adap, 0, sizeof(i2c_pci_adap));
#endif

	for (i = 0 ; i < total_i2c_pci_bus; i ++) {

		i2c_pci_adap[i].owner = THIS_MODULE;
		i2c_pci_adap[i].class = I2C_CLASS_HWMON;

		/* /dev/i2c-xxx for FPGA LOGIC I2C channel  controller 1-7  */
		i2c_pci_adap[i].nr = i + pddf_fpga_ops_data.virt_bus ;
		sprintf( i2c_pci_adap[ i ].name, "i2c-pci-%d", i );

        /* set up the sysfs linkage to our parent device */
	    i2c_pci_adap[i].dev.parent = &dev->dev;

		/* Add the bus via the algorithm code */

		if( (pddf_i2c_pci_add_numbered_bus!=NULL) && (pddf_i2c_pci_add_numbered_bus( &i2c_pci_adap[ i ], i ) != 0 ))
		{
			pddf_dbg(FPGA, KERN_ERR "Cannot add bus %d to algorithm layer\n", i );
			return( -ENODEV );
		}
		pddf_dbg(FPGA, KERN_INFO "[%s] Registered bus id: %s\n", __FUNCTION__, kobject_name(&i2c_pci_adap[ i ].dev.kobj));
	}

	return 0;
}

static void pddf_pci_del_adapter(void)
{
	int i;
	for( i = 0; i < total_i2c_pci_bus; i++ ){
		i2c_del_adapter(&i2c_pci_adap[i]);
	}
}

static int map_bars(struct fpgapci_devdata *pci_privdata, struct pci_dev *dev)
{
	unsigned long barFlags, barStart, barEnd, barLen;
    int i;

	for (i=0; i < MAX_PCI_NUM_BARS; i++) {
		if((barLen=pci_resource_len(dev, i)) !=0 && (barStart=pci_resource_start(dev, i)) !=0 ) {
			barFlags = pci_resource_flags(dev, i);
			barEnd = pci_resource_end(dev, i);
			pddf_dbg(FPGA, KERN_INFO "[%s] PCI_BASE_ADDRESS_%d 0x%08lx-0x%08lx bar_len=0x%lx"
						" flags 0x%08lx IO_mapped=%s Mem_mapped=%s\n", __FUNCTION__,
						i, barStart, barEnd, barLen, barFlags, (barFlags & IORESOURCE_IO)? "Yes": "No",
						(barFlags & IORESOURCE_MEM)? "Yes" : "No");
			FPGAPCI_BAR_INDEX=i;
			break;
		}
	}

	if (FPGAPCI_BAR_INDEX != -1) {
	    pci_privdata->bar_length = barLen;
        pci_privdata->fpga_data_base_addr = ioremap_cache (barStart + pddf_fpga_ops_data.data_base_offset,
                 pddf_fpga_ops_data.data_size);
        fpga_ctl_addr = pci_privdata->fpga_data_base_addr;

        pci_privdata->fpga_i2c_ch_base_addr = ioremap_cache (barStart + pddf_fpga_ops_data.i2c_ch_base_offset,
                 I2C_PCI_MAX_BUS * pddf_fpga_ops_data.i2c_ch_size);
        pci_privdata->max_fpga_i2c_ch = pddf_fpga_ops_data.virt_i2c_ch;
        pci_privdata->fpga_i2c_ch_size = pddf_fpga_ops_data.i2c_ch_size;
	} else {
		pddf_dbg(FPGA, KERN_INFO "[%s] Failed to find BAR\n", __FUNCTION__);
		return (-1);
	}
	pddf_dbg(FPGA, KERN_INFO "[%s] fpga_ctl_addr:0x%p fpga_data__base_addr:0x%p"
		" bar_index[%d] fpgapci_bar_len:0x%08lx fpga_i2c_ch_base_addr:0x%p supported_i2c_ch=%d",
             __FUNCTION__, fpga_ctl_addr, pci_privdata->fpga_data_base_addr, FPGAPCI_BAR_INDEX,
			pci_privdata->bar_length, pci_privdata->fpga_i2c_ch_base_addr, pci_privdata->max_fpga_i2c_ch);

	return 0;
}

static void free_bars(struct fpgapci_devdata *pci_privdata, struct pci_dev *dev)
{
    pci_iounmap(dev, pci_privdata->fpga_data_base_addr);
    pci_privdata->fpga_i2c_ch_base_addr = NULL;
}

static int pddf_pci_config_data(struct pci_dev *dev)
{
	unsigned short vendorId=0xFFFF, deviceId=0xFFFF;
	char  revisionId=0xFF, classDev=0xFF, classProg=0xFF;
	char  irqLine=0xFF, irqPin=0xFF;

	pddf_dbg(FPGA, KERN_INFO "[%s] PCI Config Data\n", __FUNCTION__);

	/* accessing the configuration region of the PCI device */
	pci_read_config_word(dev, PCI_VENDOR_ID, &vendorId);
	pci_read_config_word(dev, PCI_DEVICE_ID, &deviceId);
	pci_read_config_byte(dev, PCI_REVISION_ID, &revisionId);
	pci_read_config_byte(dev, PCI_CLASS_PROG, &classProg);
	pci_read_config_byte(dev, PCI_CLASS_DEVICE, &classDev);

	pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &irqPin);
	if(pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &irqLine)) {
		pddf_dbg(FPGA, KERN_ERR "\tPCI_INTERRUPT_LINE Error\n");
	}

	pddf_dbg(FPGA, KERN_INFO "\t[venId, devId]=[0x%x;0x%x] [group, class]=[%x;%x]\n",
			vendorId, deviceId, classProg, classDev);
	pddf_dbg(FPGA, KERN_INFO "\trevsionId=0x%x, irq_line=0x%x, irq_support=%s\n",
			revisionId, irqLine, (irqPin == 0)? "No":"Yes");

      return (0);
}


static int pddf_fpgapci_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
    struct fpgapci_devdata *pci_privdata = 0;
	int err = 0;
	pddf_dbg(FPGA, KERN_INFO "[%s]\n", __FUNCTION__);

	if ((err = pci_enable_device(dev))) {
		pddf_dbg(FPGA, KERN_ERR "[%s] pci_enable_device failed. dev:%s err:%#x\n",
			__FUNCTION__, pci_name(dev), err);
		return (err);
    }

     /* Enable DMA */
     pci_set_master(dev);

     /*  Request MMIO/IOP resources - reserve PCI I/O and memory resources
         DRIVE_NAME shows up in /proc/iomem
      */
    if ((err = pci_request_regions(dev, DRIVER_NAME)) < 0) {
		pddf_dbg(FPGA, KERN_ERR "[%s] pci_request_regions failed. dev:%s err:%#x\n",
			__FUNCTION__, pci_name(dev), err);
		goto error_pci_req;
    }

    pci_privdata = kzalloc(sizeof(struct fpgapci_devdata), GFP_KERNEL);

    if (!pci_privdata) {
        pddf_dbg(FPGA, KERN_ERR "[%s] couldn't allocate pci_privdata  memory", __FUNCTION__);
		goto error_pci_req;
     }

    pci_privdata->pci_dev = dev;
    dev_set_drvdata(&dev->dev, (void*)pci_privdata);
    pddf_pci_config_data(dev);

    if (map_bars(pci_privdata, dev)) {
        pddf_dbg(FPGA, KERN_ERR "error_map_bars\n");
        goto error_map_bars;
    }
    pddf_pci_add_adapter(dev);
	return (0);

/* ERROR HANDLING */
error_map_bars:
    pci_release_regions(dev);
error_pci_req:
    pci_disable_device(dev);
    return -ENODEV;

}

static void pddf_fpgapci_remove(struct pci_dev *dev)
{
	struct fpgapci_devdata *pci_privdata = 0;

	if (dev == 0) {
		pddf_dbg(FPGA, KERN_ERR "[%s]: dev is 0\n", __FUNCTION__);
		return;
	}

	pci_privdata = (struct fpgapci_devdata*) dev_get_drvdata(&dev->dev);

	if (pci_privdata == 0) {
		pddf_dbg(FPGA, KERN_ERR "[%s]: pci_privdata is 0\n", __FUNCTION__);
		return;
	}

	pddf_pci_del_adapter();
	free_bars (pci_privdata, dev);
	pci_disable_device(dev);
	pci_release_regions(dev);
	kfree (pci_privdata);
}


/* Initialize the driver module (but not any device) and register
 * the module with the kernel PCI subsystem. */
int pddf_fpgapci_register(FPGA_OPS_DATA* ptr_ops_data)
{

    memcpy(&pddf_fpga_ops_data, ptr_ops_data, sizeof(FPGA_OPS_DATA));
#if DEBUG
	pddf_dbg(FPGA, KERN_INFO "[%s]: pddf_fpga_ops_data vendor_id=0x%x device_id=0x%x virt_bus=0x%x "
		" data_base_offset=0x%x data_size=0x%x i2c_ch_base_offset=0x%x i2c_ch_size=0x%x virt_i2c_ch=%d",
			__FUNCTION__, pddf_fpga_ops_data.vendor_id, pddf_fpga_ops_data.device_id,
			pddf_fpga_ops_data.virt_bus, pddf_fpga_ops_data.data_base_offset, pddf_fpga_ops_data.data_size,
            pddf_fpga_ops_data.i2c_ch_base_offset, pddf_fpga_ops_data.i2c_ch_size,
            pddf_fpga_ops_data.virt_i2c_ch);
#endif
	struct pci_device_id fpgapci_ids[2] = {
		{PCI_DEVICE(pddf_fpga_ops_data.vendor_id, pddf_fpga_ops_data.device_id)},
		{0, },
	};

	int size = sizeof(struct pci_device_id) * 2;

    if ((pddf_fpgapci_ids=kmalloc(size, GFP_KERNEL)) == NULL) {
		pddf_dbg(FPGA, KERN_INFO "%s kmalloc failed\n", __FUNCTION__);
		return 0;
	}

	memcpy(pddf_fpgapci_ids, fpgapci_ids, size);

    pddf_fpgapci_driver.name=DRIVER_NAME;
    pddf_fpgapci_driver.id_table=pddf_fpgapci_ids;
    pddf_fpgapci_driver.probe=pddf_fpgapci_probe;
    pddf_fpgapci_driver.remove=pddf_fpgapci_remove;

    if (pci_register_driver(&pddf_fpgapci_driver)) {
		pddf_dbg(FPGA, KERN_INFO "%s: pci_unregister_driver\n", __FUNCTION__);
		pci_unregister_driver(&pddf_fpgapci_driver);
		return -ENODEV;
    }
	return 0;
}

EXPORT_SYMBOL(pddf_fpgapci_register);

static int __init pddf_fpgapci_driver_init(void)
{
    pddf_dbg(FPGA, KERN_INFO "[%s]\n", __FUNCTION__);

    return 0;
}

static void __exit pddf_fpgapci_driver_exit(void)
{
    pddf_dbg(FPGA, KERN_INFO "[%s]\n", __FUNCTION__);

    if (pddf_fpgapci_ids) {
        /* unregister this driver from the PCI bus driver */
        pci_unregister_driver(&pddf_fpgapci_driver);
        kfree(pddf_fpgapci_ids);
    }

}


module_init (pddf_fpgapci_driver_init);
module_exit (pddf_fpgapci_driver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Broadcom");
MODULE_DESCRIPTION ("PDDF Driver for FPGAPCI Logic I2C bus");
