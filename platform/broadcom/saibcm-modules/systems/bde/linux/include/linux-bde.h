/***********************************************************************
 *
 * $Id: linux-bde.h,v 1.24 Broadcom SDK $
 *
 * $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 *
 * Linux Broadcom Device Enumerators
 *
 *
 * There are two Linux BDEs:
 *
 * 1. Linux Kernel BDE
 *
 *	This is a kernel module implementing a BDE
 *	for the driver running as part of the kernel. 
 * 	
 *	It manages the devices through the linux PCI interfaces, 
 *	and manages a chunk of contiguous, boot-time allocated
 *	DMA memory. This is all that is needed if the BCM driver
 *	is run as part of the kernel (in another module). 
 *
 * 2. Linux User BDE
 *
 *	This is a kernel module and userland library which implement
 * 	a complete BDE for applications running in userland. 
 *	
 *	The kernel module relies upon the real kernel bde, 
 *	and allows a user space application (through the user library)
 *	to talk directly to the devices. It also virtualized the device 
 *	interrupts, so the entire driver can be run as a userspace 
 *	application. 
 *	
 *	While this causes a significant degradation in performance, 
 *	because the system runs as a user application, the development
 *	and debugging process is about a gillion times easier. 
 *	After the core logic is debugged, it can be retargeted using
 *	only the kernel bde and run in the kernel. 
 *	
 *
 **********************************************************************/

#ifndef __LINUX_BDE_H__
#define __LINUX_BDE_H__

#include <sal/types.h>
#include <ibde.h>


/* 
 * Device Major Numbers
 * 
 * The kernel and user bdes need unique major numbers
 * on systems that do not use devfs. 
 * 
 * They are defined here, along with the module names, 
 * to document them if you need to mknod them (or open) them, 
 * and to keep them unique. 
 *
 */

#include <linux/version.h>

#ifdef __KERNEL__
#include <linux/types.h>
/* Key stone and Raptor has 2.6.21 but don't have definition */
#if defined(KEYSTONE) || defined(RAPTOR)
#if (LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,21))
    #ifdef PHYS_ADDRS_ARE_64BITS
    typedef u64 phys_addr_t;
    #else
    typedef u32 phys_addr_t;
    #endif
  #endif
#endif
#endif


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,15))
#define LINUX_BDE_DMA_DEVICE_SUPPORT
#endif

#define LINUX_KERNEL_BDE_NAME 	"linux-kernel-bde"
#define LINUX_KERNEL_BDE_MAJOR 	127

#define LINUX_USER_BDE_NAME 	"linux-user-bde"
#define LINUX_USER_BDE_MAJOR   	126


/* Max devices */
/* 16 switch chips + 2 out-of-band Ethernet + 2 CPUs */
#ifndef LINUX_BDE_MAX_SWITCH_DEVICES
#define LINUX_BDE_MAX_SWITCH_DEVICES    16
#endif
#define LINUX_BDE_MAX_ETHER_DEVICES     2
#define LINUX_BDE_MAX_CPU_DEVICES       2
#define LINUX_BDE_MAX_DEVICES           (LINUX_BDE_MAX_SWITCH_DEVICES + \
                                         LINUX_BDE_MAX_ETHER_DEVICES + \
                                         LINUX_BDE_MAX_CPU_DEVICES)
#define LINUX_BDE_NOF_DEVICE_BITMAP_WORDS ((LINUX_BDE_MAX_DEVICES+31)/32)
#define LINUX_BDE_MAX_IPROC_UC_CORES    12 /* Maximum number of R5 cores per device */
typedef uint32 linux_bde_device_bitmap_t[LINUX_BDE_NOF_DEVICE_BITMAP_WORDS];

/* 
 * PCI devices will be initialized by the Linux Kernel, 
 * regardless of architecture.
 *
 * You need only provide bus endian settings. 
 */

typedef struct linux_bde_bus_s {
    int be_pio;
    int be_packet;
    int be_other;
} linux_bde_bus_t;



/* Device state used for PCI hot swap case. */
/*
 * BDE_DEV_STATE_NORMAL : A device is probed normally. Or when the device 
 * resource has been updated after "CHANGED", the state will move back to 
 * "NORMAL".
 */
#define BDE_DEV_STATE_NORMAL       (0)
/*
 * BDE_DEV_STATE_REMOVED : A previous probed device was removed.
 * We will avoid any device access while the device is in this state.
 * The state will be moved to "CHANGED" if the device is re-inserted
 * and re-probed.
 */
#define BDE_DEV_STATE_REMOVED      (1)
/* 
 * BDE_DEV_STATE_CHANGED : The device is re-probed after having been removed.
 * The resouces assigned to the device might have been changed after
 * re-probing, so we need to re-initialize our resource database accordingly. 
 * The state will change to "NORMAL" when the resource have been updated.
 */
#define BDE_DEV_STATE_CHANGED      (2)

/*
 * BDE_DEV_INST_ID_INVALID : The invalid instance identifier.
 */
#define BDE_DEV_INST_ID_INVALID    ((uint32)-1)
#define BDE_DEV_INST_ID_CURRENT    ((uint32)-2)
#define BDE_DEV_INST_ID_GET_FREE   ((uint32)-3)

extern int get_instance_dma_size(unsigned int instance_num, unsigned int *size);
extern int dma_get_usage(void);

extern int linux_bde_create(linux_bde_bus_t* bus, ibde_t** bde);
extern int linux_bde_destroy(ibde_t* bde);
#ifdef BCM_INSTANCE_SUPPORT
extern int linux_bde_instance_attach(unsigned int dev_mask,unsigned int dma_size);
extern int linux_bde_instance_config(linux_bde_device_bitmap_t dev_mask,unsigned int dma_size);
#endif
extern int linux_bde_get_pci_info(int d, uint32 *bus, uint32 *slot, uint32 *func);

#ifdef __KERNEL__

#ifdef INCLUDE_EDK
#define BDE_EDK_SUPPORT
#endif

/*
 *  Backdoors provided by the kernel bde
 *
 */

/*
 * The user bde needs to get cpu physical address for
 * the userland code to mmap.
 * And the second address is bus address, it is either
 * identical to cpu physical address or another address
 * (IOVA) translated by IOMMU.
 */
extern int lkbde_get_dma_info(phys_addr_t *cpu_pbase, phys_addr_t *dma_pbase, ssize_t *size);
extern uint32 lkbde_get_dev_phys(int d);
extern uint32 lkbde_get_dev_phys_hi(int d);
#ifdef BDE_EDK_SUPPORT
extern int lkbde_edk_get_dma_info(int dev_id, phys_addr_t* cpu_pbase,
                                  phys_addr_t* dma_pbase, ssize_t* size);
extern void * lkbde_edk_dmamem_map_p2v(int dev_no, phys_addr_t paddr);
#endif
extern int lkbde_get_phys_to_virt(int d, phys_addr_t paddr, sal_vaddr_t *vaddr);

/*
 * Virtual device address needed by kernel space
 * interrupt handler.
 */
extern void *lkbde_get_dev_virt(int d);

/*
 * The user bde needs to get some physical addresses for
 * the userland code to mmap. The following functions
 * supports multiple resources for a single device.
 */
extern int lkbde_get_dev_resource(int d, int rsrc, uint32_t *phys_lo,
                                  uint32_t *phys_hi, uint32_t *size);

/*
 * Backdoor to retrieve OS device structure to be used for
 * DMA operations.
 */
extern void *lkbde_get_dma_dev(int d);

/*
 * Backdoor to retrieve original hardware/OS device.
 */
extern void *lkbde_get_hw_dev(int d);

/*
 * Backdoor to retrieve number of switch devices probed.
 */
extern int lkbde_get_num_devices(int type);

/*
 * Retrive the device state from Kernel BDE.
 * Used for KNET and User BDE for pci hot swap case.
 */
extern int lkbde_dev_state_get(int d, uint32 *state);
extern int lkbde_dev_state_set(int d, uint32 state);

/*
 * Retrive the mapping between emulated HW device and instance id
 */
extern int lkbde_dev_instid_get(int d, uint32 *instid);
extern int lkbde_dev_instid_set(int d, uint32 instid);

/*
 * Return none-zero if the SDK instance with the given instance ID
 * manages the given device.
 */
extern int lkbde_is_dev_managed_by_instance(uint32 dev, uint32 inst_id);

/*
 * Return a pointer to the bitmap of the SDK instance managed devices.
 */
extern linux_bde_device_bitmap_t* lkbde_get_inst_devs(uint32 inst_id);


/*
 * Functions that allow an interrupt handler in user mode to
 * coexist with interrupt handler in kernel module.
 */
extern int lkbde_irq_mask_set(int d, uint32 addr, uint32 mask, uint32 fmask);
extern int lkbde_irq_mask_get(int d, uint32 *mask, uint32 *fmask);
extern int lkbde_irq_status_get(int d, uint32_t addr, uint32 *status);

#ifdef BCM_SAND_SUPPORT
extern int lkbde_cpu_write(int d, uint32 addr, uint32 *buf);
extern int lkbde_cpu_read(int d, uint32 addr, uint32 *buf);
extern int lkbde_cpu_pci_register(int d);
#endif

extern int lkbde_intr_cb_register(int d,
                                  int (*intr_pending)(void*),
                                  void *intr_pending_data);

/*
 * Get the PCI bus number, bus slot and function for a PCI device.
 */
extern int lkbde_get_dev_pci_info(int d, uint32_t *bus,
                                  uint32_t *slot, uint32_t *func);

/*
 * Get the iproc version number for a device.
 */
extern int lkbde_iproc_version_get(int d, uint32 *iproc_ver);

#ifdef INCLUDE_SRAM_DMA
#ifdef SRAM_DMA_NEEDS_KERNEL_APIS
extern void _update_apis_for_sram_dma();
#endif
extern void lkbde_get_sram_dma_info(unsigned d, uint32 *sram_start, uint32 *sram_size);
#endif /* INCLUDE_SRAM_DMA */
/*
 * This flag must be OR'ed onto the device number when calling
 * interrupt_connect/disconnect and irq_mask_set functions from
 * a secondary device driver.
 */
#define LKBDE_ISR2_DEV  0x8000
/*
 * This flag should be OR'ed onto the device number when calling
 * irq_mask_set functions from a secondary device driver if the
 * mask register is iProc register.
 */
#define LKBDE_IPROC_REG 0x4000

#ifdef BCM_SAND_SUPPORT
#include <linux/version.h>
#if defined(__DUNE_LINUX_BCM_CPU_PCIE__) && LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26)
#ifndef _SIMPLE_MEMORY_ALLOCATION_
#define _SIMPLE_MEMORY_ALLOCATION_ 1
#endif
#endif
#endif

#if defined(IPROC_CMICD) && defined(CONFIG_CMA)
#ifndef _SIMPLE_MEMORY_ALLOCATION_
#define _SIMPLE_MEMORY_ALLOCATION_ 1
#endif
#endif

/* Allocation via dma_alloc_coherent is turned off by default */
#ifndef _SIMPLE_MEMORY_ALLOCATION_
#define _SIMPLE_MEMORY_ALLOCATION_ 9 /* compile in the allocation method, but do not use it by default */
#endif

/* By default we use our private mmap for DMA pool */
#ifndef USE_LINUX_BDE_MMAP
#define USE_LINUX_BDE_MMAP 1
#endif

#endif /* __KERNEL__ */

#endif /* __LINUX_BDE_H__ */
