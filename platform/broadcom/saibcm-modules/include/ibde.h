/*
 * $Id: ibde.h,v 1.27 Broadcom SDK $
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
 */

#ifndef __IBDE_H__
#define __IBDE_H__

#include <sal/types.h>

/*
 * Represents a collection of devices 
 */

typedef struct ibde_dev_s {
    uint16 device;
    uint8  rev;
    sal_vaddr_t base_address;
    sal_vaddr_t base_address1;
    sal_vaddr_t base_address2;
    /* a unique number representing the specific device.
     * Must be different for different devices.
     * May be used to identify specific devices in the system.
     * May be implemented as a full PCIe address, a persistent configurable user value, ...
     * Possible implementation value stores in QSPI flash memory of the device. */
    uint32 dev_unique_id;
} ibde_dev_t;


typedef struct ibde_s {

    const char *(*name)(void);
  
    /* Returns the number of devices available */
    /* Each device is is accessed through a handle */
    /* Handles are assumed to index the array of devices */
  
    /* Support SWITCH or ETHERNET or CPU devices */
    int     (*num_devices)(int type);
#define BDE_ALL_DEVICES         0
#define BDE_SWITCH_DEVICES      1
#define BDE_ETHER_DEVICES       2
#define BDE_CPU_DEVICES         3

    const   ibde_dev_t *(*get_dev)(int d);

    /* 
     * Get types of underlaying devices.
     * A combination of bus type and functional type is returned.
     * In case of bus type, support PCI and SPI device types.
     * In case of functional type, specify if underlaying device is
     * a switching or ethernet device.
     */
    uint32  (*get_dev_type)(int d);
#define BDE_PCI_DEV_TYPE      SAL_PCI_DEV_TYPE    /* PCI device */
#define BDE_SPI_DEV_TYPE      SAL_SPI_DEV_TYPE    /* SPI device */
#define BDE_EB_DEV_TYPE       SAL_EB_DEV_TYPE     /* EB device */
#define BDE_ICS_DEV_TYPE      SAL_ICS_DEV_TYPE    /* ICS device */
#define BDE_MII_DEV_TYPE      SAL_MII_DEV_TYPE    /* MII device */
#define BDE_I2C_DEV_TYPE      SAL_I2C_DEV_TYPE    /* I2C device */
#define BDE_AXI_DEV_TYPE      SAL_AXI_DEV_TYPE    /* AXI device */
#define BDE_EMMI_DEV_TYPE     SAL_EMMI_DEV_TYPE   /* EMMI device */
#define BDE_COMPOSITE_DEV_TYPE SAL_COMPOSITE_DEV_TYPE /* Composite device, composed of sub-devices with buses */
#define BDE_SUB_DEV_TYPE      SAL_SUB_DEV_TYPE    /* A sub-device (with a bus) of a composite device */
#define BDE_USER_DEV_TYPE     SAL_USER_DEV_TYPE   /* The user implements his own method of access to the device */
#define BDE_DEV_BUS_ALT       SAL_DEV_BUS_ALT     /* Alternate Access */
#define BDE_DEV_BUS_MSI       SAL_DEV_BUS_MSI     /* Message-signaled interrupts */

#define BDE_DEV_BUS_TYPE_MASK SAL_DEV_BUS_TYPE_MASK
    
#define BDE_SWITCH_DEV_TYPE   SAL_SWITCH_DEV_TYPE /* Switch device */
#define BDE_ETHER_DEV_TYPE    SAL_ETHER_DEV_TYPE  /* Ethernet device */
#define BDE_CPU_DEV_TYPE      SAL_CPU_DEV_TYPE    /* CPU device */

#define BDE_BYTE_SWAP         0x01000000          /* SW byte swap */
#define BDE_NO_IPROC          0x02000000          /* Device uses two BARs, but is not iProc */

#define BDE_8MB_REG_SPACE     0x10000000          /* 8MB sized CMIC BAR */
#define BDE_256K_REG_SPACE    0x20000000          /* Map 256K (v 64K) */
#define BDE_128K_REG_SPACE    0x40000000          /* Map 128K (v 64K) */
#define BDE_320K_REG_SPACE    0x80000000          /* Map 256K+64K */
#ifdef INCLUDE_SRAM_DMA
#define BDE_TEST_DEVICES 1
#endif

/* Bus supports only 16bit reads */
#define BDE_DEV_BUS_RD_16BIT  SAL_DEV_BUS_RD_16BIT 

/* Bus supports only 16bit writes */
#define BDE_DEV_BUS_WR_16BIT  SAL_DEV_BUS_WR_16BIT

/* Backward compatibility */
#define BDE_ET_DEV_TYPE       BDE_MII_DEV_TYPE

#define BDE_DEV_MEM_MAPPED(_d) \
    ((_d) & (BDE_PCI_DEV_TYPE | BDE_ICS_DEV_TYPE | BDE_EB_DEV_TYPE |\
             BDE_EMMI_DEV_TYPE | BDE_AXI_DEV_TYPE))

    /* 
     * PCI Bus Access
     */
    uint32  (*pci_conf_read)(int d, uint32 addr);
    int     (*pci_conf_write)(int d, uint32 addr, uint32 data);
    void    (*pci_bus_features)(int d, int *be_pio, int *be_packet,
                                int *be_other);

    uint32  (*read)(int d, uint32 addr);
    int     (*write)(int d, uint32 addr, uint32 data);

    /* 
     * DMA Management
     */
    uint32* (*salloc)(int d, int size, const char *name);
    void    (*sfree)(int d, void *ptr);
    int     (*sflush)(int d, void *addr, int length);
    int     (*sinval)(int d, void *addr, int length);

    /* 
     * Interrupt Management
     */
    int     (*interrupt_connect)(int d, void (*)(void*), void *data);  
    int     (*interrupt_disconnect)(int d);

    sal_paddr_t  (*l2p)(int d, void *laddr);
    void* (*p2l)(int d, sal_paddr_t paddr);

    /* 
     * SPI Access via SMP
     */
    int  (*spi_read)(int d, uint32 addr, uint8 *buf, int len);
    int  (*spi_write)(int d, uint32 addr, uint8 *buf, int len);
    /* Special SPI access addresses */
#define BDE_DEV_OP_EMMI_INIT    SAL_DEV_OP_EMMI_INIT

    /* 
     * iProc register access
     */
    uint32  (*iproc_read)(int d, uint32 addr);
    int     (*iproc_write)(int d, uint32 addr, uint32 data);

    /* 
     * Shared memory access
     */
    uint32  (*shmem_read)(int dev, uint32 addr, uint8 *buf, uint32 len);
    void    (*shmem_write)(int dev, uint32 addr, uint8 *buf, uint32 len);
    sal_vaddr_t (*shmem_map)(int dev, uint32 addr, uint32 size);

    /*
     *  CMIC
     */
    int   (*get_cmic_ver)(int d,  uint32 *ver);

    /*
     * I2C operations on the Device, assuming it is connected by I2C to the CPU.
     */
    /* Read from the internal device Address space using I2C */
    int (*i2c_device_read)(
        int dev,       /* The device ID to access */
        uint32 addr,   /* The address to access in the internal device address space */
        uint32 *value);/* the value to be read. */
    /* Write to the internal device Address space using I2C */
    int (*i2c_device_write)(
        int dev,       /* The device ID to access */
        uint32 addr,   /* The address to access in the internal device address space */
        uint32 value); /* the value to be written. */

    /* 64 bit read/write */
    uint64  (*read64)(int d, uint32 addr);
    void    (*write64)(int d, uint32 addr, uint64 data);

    /*
     * Probe for new devices.
     *
     * This function will normally be called implicitly by the BDE
     * initialization function, but it may be called at a later time
     * by the application to detect removed or added devices.
     *
     * Existing devices are not affected by this operation.
     *
     * If a device has been hot-swapped, then it will be assigned the
     * same resources as before the hot-swap.
     *
     * Return value:
     *   0: Device probe completed successfully.
     *  -1: An error happened during device probe.
     */
    int     (*probe)(void);

    /*
     * Get DMA debug information.
     *
     * This function was placed here to retain backward compatibility
     * with BDE implementations that employ a fixed-order function
     * list.
     */
    int (*sinfo)(int d, int type, uint32 *data);

} ibde_t;


/* System BDE */
extern ibde_t *bde;


#endif /* __IBDE_H__ */
