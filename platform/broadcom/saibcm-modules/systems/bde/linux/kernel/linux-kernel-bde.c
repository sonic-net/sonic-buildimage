/*
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

/*
 * Linux Kernel BDE
 */

#include <gmodule.h>
#include <linux-bde.h>
#include <linux_dma.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <sdk_config.h>
#include <soc/devids.h>
#include <linux/version.h>

#include "linux_shbde.h"

#define MEMCPY memcpy

#ifdef CONFIG_X86_64
#if (defined(__GNUC__) && (__GNUC__ >= 8))
/*
 * Prevent gcc from using a compiler builtin memcpy even if using -fno-builtin or
 * -fno-builtin-memcpy.
 * Use __inline_memcpy or __memcpy kernel function in x86 Linux instead.
 */
#undef MEMCPY
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,0,0))
#define MEMCPY __inline_memcpy
#else
#define MEMCPY __memcpy
#endif /* (LINUX_VERSION_CODE < KERNEL_VERSION(5.0.0)) */
#endif /* (defined(__GNUC__) && (__GNUC__ >= 8)) */
#endif /* CONFIG_X86_64 */


#if defined(CMIC_SOFT_BYTE_SWAP)
#define CMIC_SWAP32(_x)   ((((_x) & 0xff000000) >> 24) \
                         | (((_x) & 0x00ff0000) >>  8) \
                         | (((_x) & 0x0000ff00) <<  8) \
                         | (((_x) & 0x000000ff) << 24))
#define CMIC_SWAP64(_x)   ((((_x) & 0xff000000) >> 24) \
                         | (((_x) & 0x00ff0000) >>  8) \
                         | (((_x) & 0x0000ff00) <<  8) \
                         | (((_x) & 0x000000ff) << 24) \
                         | (((_x) & 0xff00000000000000UL) >> 24) \
                         | (((_x) & 0x00ff000000000000UL) >>  8) \
                         | (((_x) & 0x0000ff0000000000UL) <<  8) \
                         | (((_x) & 0x000000ff00000000UL) << 24))
#endif /* defined(CMIC_SOFT_BYTE_SWAP) */

#define PCI_USE_INT_NONE    (-1)
#define PCI_USE_INT_INTX     (0)
#define PCI_USE_INT_MSI     (1)
#define PCI_USE_INT_MSIX    (2)
#ifdef CONFIG_PCI_MSI
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,110))
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,84))
#define msix_table_size(flags)  ((flags & PCI_MSIX_FLAGS_QSIZE) + 1)
#endif
#define msi_control_reg(base)         (base + PCI_MSI_FLAGS)
#endif
#endif
MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("Kernel BDE");
MODULE_LICENSE("GPL");

/*
 * PCIe max payload size in bytes.
 * The default value if not specified to the kernel module by maxpayload is historically 256.
 * The default value may be changed using the BDE_PCIE_MAXPAYLOAD_DEFAULT macro.
 */
#ifndef BDE_PCIE_MAXPAYLOAD_DEFAULT
#define BDE_PCIE_MAXPAYLOAD_DEFAULT 256
#endif
int maxpayload = BDE_PCIE_MAXPAYLOAD_DEFAULT;
LKM_MOD_PARAM(maxpayload, "i", int, 0);
MODULE_PARM_DESC(maxpayload,
"Limit maximum payload size and request size on PCIe devices");

/* Use MSI or MSIX interrupts */
int usemsi = -1;
LKM_MOD_PARAM(usemsi, "i", int, 0);
MODULE_PARM_DESC(usemsi,
"Use MSI/ MSIX interrupts if supported by kernel");

/* Check system support msi or not */
static int msi_check = 1;
LKM_MOD_PARAM(msi_check, "i", int, (S_IRUGO | S_IWUSR));
MODULE_PARM_DESC(msi_check,
"Do not check msi supporting (default 1");


/* Ignore all recognized devices (for debug purposes) */
int nodevices;
LKM_MOD_PARAM(nodevices, "i", int, 0);
MODULE_PARM_DESC(nodevices,
"Ignore all recognized devices (default no)");

int msixcnt = 1;

/*
 * This usually is defined at /usr/include/linux/pci_ids.h
 * But this ID is newer.
 */
#ifndef PCI_DEVICE_ID_PLX_9656
#define PCI_DEVICE_ID_PLX_9656 0x9656
#endif

#ifndef PCI_DEVICE_ID_PLX_9056
#define PCI_DEVICE_ID_PLX_9056 0x9056
#endif

/* For 2.4.x kernel support */
#ifndef IRQF_SHARED
#define IRQF_SHARED     SA_SHIRQ
#endif

/* Set irq affinity for PCIe interrupt handler */
int pci_irq_affinity = -1;
LKM_MOD_PARAM(pci_irq_affinity, "i", int, 0);
MODULE_PARM_DESC(pci_irq_affinity,
"Manually enforce IRQ affinity for the PCIe interrupt handler");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18)
typedef unsigned long resource_size_t;
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18) */

#ifdef BCM_ICS
#define BCM_ICS_CMIC_BASE       0x08000000
#else

/* Force interrupt line */
static int forceirq = -1;
static uint32_t forceirqubm = 0xffffffff;
LKM_MOD_PARAM(forceirq, "i", int, 0);
LKM_MOD_PARAM(forceirqubm, "i", uint, 0);
MODULE_PARM_DESC(forceirq,
"Override IRQ line assigned by boot loader");
MODULE_PARM_DESC(forceirqubm,
"Bitmap for overriding the IRQ line assigned by boot loader for given units");

/* Create SPI slave device (cannot be probed) */
static uint32_t spi_devid = 0;
LKM_MOD_PARAM(spi_devid, "i", uint, 0);
MODULE_PARM_DESC(spi_devid,
"Create SPI slave device using this device ID");

/* Select SPI device revision (cannot be probed) */
static uint32_t spi_revid = 1;
LKM_MOD_PARAM(spi_revid, "i", uint, 0);
MODULE_PARM_DESC(spi_revid,
"Select device revision for SPI slave device");

#endif /* BCM_ICS */

/* Debug output */
static int debug;
LKM_MOD_PARAM(debug, "i", int, 0);
MODULE_PARM_DESC(debug,
"Set debug level (default 0");
/* Use high memory for DMA */

/* module param for probing EB devices. */
static char *eb_bus;
LKM_MOD_PARAM(eb_bus, "s", charp, 0);
MODULE_PARM_DESC(eb_bus,
"List of EB devices on platform. Input format (BA=%x IRQ=%d RD16=%d WR16=%d");

#ifdef KEYSTONE
/* Force SPI Frequency */
static int spifreq = 0;
LKM_MOD_PARAM(spifreq, "i", int, 0);
MODULE_PARM_DESC(spifreq,
"Force SPI Frequency for Keystone CPU (0 for default frequency)");
#endif



/* Periodic timer to prevent stuck interrupt */
static int isrtickms = 1000;
LKM_MOD_PARAM(isrtickms, "i", int, (S_IRUGO | S_IWUSR));
MODULE_PARM_DESC(isrtickms,
"Periodic ISR tick in milliseconds.");

/* Debug purpose to simulate stuck interrupts */
static int dma_lock = 0;
LKM_MOD_PARAM(dma_lock, "i", int, (S_IRUGO | S_IWUSR));
MODULE_PARM_DESC(dma_lock,
"Simulation of stuck interrupts.");

/* Additional configurations for PCI bridges */
static int pci_bridge_setup = 1;
LKM_MOD_PARAM(pci_bridge_setup, "i", int, (S_IRUGO | S_IWUSR));
MODULE_PARM_DESC(pci_bridge_setup,
"Additional PCI bridge configurations in initialization (default yes).");

/* Compatibility */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,30))
#define _ISR_RET irqreturn_t
#else
#define _ISR_RET int
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19))
#define _ISR_PARAMS(_i,_d,_r) int _i, void *_d
#else
#define _ISR_PARAMS(_i,_d,_r) int _i, void *_d, struct pt_regs *_r
typedef irqreturn_t (*irq_handler_t)(int _i, void *_d, struct pt_regs *_r);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,15,0))
#define SYNC_IRQ(_i) synchronize_hardirq(_i)
#else
#define SYNC_IRQ(_i) synchronize_irq(_i)
#endif
static char * ___strtok;
static char * strtok(char * s,const char * ct)
{
    char *sbegin, *send;
    sbegin  = s ? s : ___strtok;
    if (!sbegin) {
        return NULL;
    }
    sbegin += strspn(sbegin,ct);
    if (*sbegin == '\0') {
        ___strtok = NULL;
        return( NULL );
    }
    send = strpbrk( sbegin, ct);
    if (send && *send != '\0')
        *send++ = '\0';
    ___strtok = send;
    return (sbegin);
}

/* PCIe capabilities */
#ifndef PCI_CAP_ID_EXP
#define PCI_CAP_ID_EXP          0x10
#endif
#ifndef PCI_EXP_DEVCAP
#define PCI_EXP_DEVCAP          4
#endif
#ifndef PCI_EXP_DEVCTL
#define PCI_EXP_DEVCTL          8
#endif
#ifndef PCI_EXT_CAP_START
#define PCI_EXT_CAP_START       0x100
#endif
#ifndef PCI_EXT_CAP_ID
#define PCI_EXT_CAP_ID(_hdr)    (_hdr & 0x0000ffff)
#endif
#ifndef PCI_EXT_CAP_VER
#define PCI_EXT_CAP_VER(_hdr)   ((_hdr >> 16) & 0xf)
#endif
#ifndef PCI_EXT_CAP_NEXT
#define PCI_EXT_CAP_NEXT(_hdr)  ((_hdr >> 20) & 0xffc)
#endif
#ifndef PCI_EXT_CAP_ID_VNDR
#define PCI_EXT_CAP_ID_VNDR     0x0b
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,20)
#define PCI_FIND_DEV(_d, _v, _fr)       pci_find_device(_d, _v, _fr)
#else
#define PCI_FIND_DEV(_d, _v, _fr)       pci_get_device(_d, _v, _fr)
#endif

#if defined(CONFIG_RESOURCES_64BIT) || defined(CONFIG_PHYS_ADDR_T_64BIT)
#define PHYS_ADDR_IS_64BIT
#endif

/* Structure of private SPI device */
struct spi_dev {
    uint8          cid;         /* Chip ID */
    uint32          part;        /* Part number of the chip */
    uint8          rev;         /* Revision of the chip */
    void           *robo;       /* ptr to robo info required to access SPI */
    unsigned short phyid_high;  /* PHYID HIGH in MII regs of detected chip */
    unsigned short phyid_low;   /* PHYID LOW in MII regs of detected chip */
};

struct bde_spi_device_id {
    unsigned short phyid_high;  /* PHYID HIGH in MII regs of detected chip */
    unsigned short phyid_low;   /* PHYID LOW in MII regs of detected chip */
    uint32  model_info;
    uint32  rev_info;
    uint32  spifreq;
};

/* Maximum number of I/O windows supported per device. */
#define BDE_NUM_IOWIN_MAX     3

/* I/O memory window definition. */
struct memwin_s {

    /* Physical address of I/O window. */
    resource_size_t addr;

    /* Size of I/O window (in bytes). */
    resource_size_t size;
};

/* Control Data */
typedef struct bde_ctrl_s {
    struct list_head list;

    /* Specify the type of device, pci, spi, switch, ether ... */
    uint32 dev_type;

    int domain_no;
    int bus_no;
    int be_pio;
    int use_msi;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,84))
    struct msix_entry *entries;
#endif
    int msix_cnt;
    union {
        /* Linux PCI device pointer */
        struct pci_dev* _pci_dev;

        /* SPI device pointer */
        struct spi_dev* _spi_dev;
    } dev;
#define pci_device  dev._pci_dev
#define spi_device  dev._spi_dev

#ifdef LINUX_BDE_DMA_DEVICE_SUPPORT
    struct device *dma_dev;
#endif

    struct memwin_s iowin[BDE_NUM_IOWIN_MAX];

    /* Secondary mapped base address */
    sal_vaddr_t alt_base_addr;

    /* BDE device description */
    ibde_dev_t bde_dev;

    /* Interrupt Handling */
    int iLine; /* Interrupt line */
    void (*isr)(void *);
    void *isr_data;

    /*
     * Controls to allow two drivers to share a single set of
     * hardware registers. Typically a kernel driver will handle
     * a subset of hardware interrupts and a user mode driver
     * will handle the remaining interrupts.
     */
    void (*isr2)(void *);
    void *isr2_data;
    uint32_t fmask;  /* Interrupts controlled by secondary handler */
    uint32_t imask;  /* Enabled interrupts for primary handler */
    uint32_t imask2; /* Enabled interrupts for secondary handler */
    spinlock_t lock; /* Lock for IRQ mask synchronization */

    /* Hardware abstraction for shared BDE functions */
    shbde_hal_t shbde;

    /* Device state : BDE_DEV_STATE_REMOVED/CHANGED */
    uint32 dev_state;

    /* inst_id */
    uint32 inst_id; /* The instance ID of the instance controlling the device */

    /*
     * Callback function to determine whether there are any pending
     * interrupts. This function is called periodically from a timer
     * tick function when the interrupt handler is idle.
     *
     * The timer tick function is disabled temporarily when a second
     * interrupt handler is installed.
     *
     * The timer tick function is disabled permanently if the PCI link
     * is lost or reset or if the callback function is removed.
     */
    int (*intr_pending)(void *);
    void *intr_pending_data;
    uint32 interrupts;            /* Current number of interrupts */
    uint32 prev_interrupts;       /* Number of interrupts in previous ISR tick */
    uint32 stuck_interrupts;      /* Number of stuck interrupts detected */
    uint32 no_intr_isr_ticks;     /* Number of ISR ticks without interrupt occurred */
    struct timer_list isr_tick;   /* Timer tick to prevent stuck interrupt */
    uint32 timer_active;          /* Timer was added for handling pending interrupt */

#ifdef INCLUDE_SRAM_DMA
    uint32 dev_sram_dma_start;    /* start address of device SRAM used for DMA */
    uint32 dev_sram_dma_size;     /* size in bytes of device SRAM used for DMA */
#endif /* INCLUDE_SRAM_DMA */
} bde_ctrl_t;

static bde_ctrl_t _devices[LINUX_BDE_MAX_DEVICES];

/* information stored per SDK instance, currently the devices it manages */
typedef struct {
    linux_bde_device_bitmap_t devices; /* The devices controlled by this instance */
} lkbde_inst_info_t;

/* Information for each SDK instance (array index), the device that it manages */
static lkbde_inst_info_t _instance_info[LINUX_BDE_MAX_DEVICES] = {{ .devices = {0}}};

static int _ndevices = 0;
static int _switch_ndevices = 0;
static int _ether_ndevices = 0;
static int _cpu_ndevices = 0;

#define VALID_DEVICE(_n) ((_n >= 0) && (_n < _ndevices))

#if defined(IPROC_CMICD) && defined(CONFIG_OF)
#define ICFG_CHIP_ID_REG      0x10236000
#define IHOST_CMICX_MAX_INTRS 129
static uint32 iproc_cmicx_irqs[IHOST_CMICX_MAX_INTRS];
#endif

/* CPU MMIO area used with CPU cards provided on demo boards */
#if defined(BCM_SAND_SUPPORT) && (defined(__DUNE_WRX_BCM_CPU__) || defined(__DUNE_GTO_BCM_CPU__))
static void *cpu_address = NULL;
#endif


/* Broadcom BCM4704 */
#define BCM4704_VENDOR_ID 0x14E4
#define BCM4704_DEVICE_ID 0x4704

/* SiByte PCI Host */
#define SIBYTE_PCI_VENDOR_ID 0x166D
#define SIBYTE_PCI_DEVICE_ID 0x0001

/* Intel 21150 PCI-PCI Bridge */
#define DC21150_VENDOR_ID 0x1011
#define DC21150_DEVICE_ID 0x0022

/* HiNT HB4 PCI-PCI Bridge (21150 clone) */
#define HINT_HB4_VENDOR_ID 0x3388
#define HINT_HB4_DEVICE_ID 0x0022

/* Pericom PI7C8150 PCI-PCI Bridge (21150 clone) */
#define PI7C8150_VENDOR_ID 0x12D8
#define PI7C8150_DEVICE_ID 0x8150

/* Pericom PI7C9X130 PCI-PCIE Bridge */
#define PCI_VNDID_PERICOM     0x12D8
#define PCI_DEVID_PI7C9X130   0xE130
#define DEV_CTRL_REG           0xb8

#define MAX_PAYLOAD_256B       (1 << 5)
#define MAX_PAYLOAD_512B       (2 << 5)
#define MAX_READ_REQ_256B      (1 << 12)


/* Freescale 8548 PCI-E  host Bridge */
#define FSL_VENDOR_ID                   0x1957
#define FSL8548PCIE_DEVICE_ID           0x0013
#define FSL2020EPCIE_DEVICE_ID          0x0070
#define FSL8548PCIE_DEV_CTRL_REG        0x54

/* 4716 PCI-E  host Bridge */
#define BCM4716_VENDOR_ID               0x14e4
#define BCM4716PCIE_DEVICE_ID           0x4716
#define BCM4716PCIE_DEV_CAP_REG         0xd4
#define BCM4716PCIE_DEV_CTRL_REG        0xd8
#define BCM53000_VENDOR_ID              0x14e4
#define BCM53000PCIE_DEVICE_ID          0x5300

#define BCM53000PCIE_DEV(port) ((port == 0) ? pcie0 : pcie1)
#define BCM53000PCIE_BASE(port) ((port == 0) ? 0xb8005000 : 0xb800e000)
#define BCM53000PCIE_FUNC0_COFIG_SPACE 0x400
#define BCM53000PCIE_SROM_SPACE 0x800
#define BCM53000PCIE_DEV_CAP_REG  0xd4
#define BCM53000PCIE_DEV_CTRL_REG 0xd8
#define BCM53000PCIE_MAX_PAYLOAD_MASK  0x7
#define BCM53000PCIE_CAP_MAX_PAYLOAD_256B  (1 << 0)
#define BCM53000PCIE_DEFAULT_STATUS 0x00100146

/* 16bit wide register. offset 14, 14*2 = 0x1c */
#define BCM53000PCIE_SPROM_OFFSET 0x1c
/* bit 15:13 spromData.MaxPayloadSize. 1: 256 bytes */
#define BCM53000PCIE_SPROM_MAX_PAYLOAD_MASK 0xe000
#define BCM53000PCIE_SPROM_MAX_PAYLOAD_256B (1 << 13)


/* Intel 21150, HiNT HB4 and other 21150-compatible */
#define PCI_CFG_DEC21150_SEC_CLK 0x68

#define BCM4704_ENUM_BASE     0x18000000
#define BCM4704_MEMC_BASE     (BCM4704_ENUM_BASE+0x8000)
#define BCM4704_MEMC_PRIORINV 0x18

/* PLX PCI-E Switch */
#define PLX_PEX8608_DEV_ID         0x8608
#define PLX_PEX8617_DEV_ID         0x8617
#define PLX_PEX86XX_DEV_CTRL_REG   0x70

/* Broadcom BCM58525 */
#define BCM58525_PCI_VENDOR_ID     0x14E4
#define BCM58525_PCI_DEVICE_ID     0x8025
#define BCM58522_PCI_DEVICE_ID     0x8022

/* Broadcom BCM58712 */
#define BCM58712_PCI_VENDOR_ID     0x14E4
#define BCM58712_PCI_DEVICE_ID     0x168E

/* Default gicd address if not available in DTB */
#define IHOST_GICD_REG_ADDR        0x10781100
#define IHOST_GICD_REG_REMAP_LEN   0x100

#define IHOST_GICD_REG_ADDR_VALID(d, addr) \
    (_devices[d].bde_dev.base_address1 && \
    (addr & 0xFFFFFF00) == _devices[d].iowin[1].addr)

#define IHOST_GICD_REG_ADDR_REMAP(d, addr) \
    (void *)(_devices[d].bde_dev.base_address1 + \
    (addr - ((sal_vaddr_t)_devices[d].iowin[1].addr)))

static uint32_t _read(int d, uint32_t addr);

#ifdef BCM_ICS
#else
#ifdef CONFIG_PCI
/* Used to determine overall memory limits across all devices */
static uint32_t _pci_mem_start = 0xFFFFFFFF;
static uint32_t _pci_mem_end = 0;

/* Used to control MSI interrupts */
static int  use_msi = 0;
#endif
#endif

#ifdef BCM_PLX9656_LOCAL_BUS

#define CPLD_OFFSET             0x00800000
#define CPLD_REVISION_REG       0x0000
#define CPLD_REVISION_MASK      0xffff
#define CPLD_RESET_REG          0x0004
#define CPLD_RESET_NONE         0x0000

#define PL0_OFFSET              0x00800000
#define PL0_SIZE                0x00040000
#define PL0_REVISION_REG        0x0000

/* Assume there's only one PLX PCI-to-Local bus bridge if any */
static bde_ctrl_t plx_ctrl;
static int num_plx = 0;

#endif /* BCM_PLX9656_LOCAL_BUS */

static spinlock_t bus_lock;

static int
_parse_eb_args(char *str, char * format, ...)
    __attribute__ ((format (scanf, 2, 3)));

static int
_parse_eb_args(char *str, char * format, ...)
{
    va_list args;

    va_start(args, format);
    vsscanf(str, format, args);
    va_end(args);

    return 0;
}

static void
_bde_add_device(void)
{
    int add_switch_device = 0;
    if (_ndevices >= LINUX_BDE_MAX_DEVICES) {
        gprintk("Error: added too many devices\n");
        return;
    }
    if (_devices[_ndevices].dev_type & BDE_SWITCH_DEV_TYPE) {
        _switch_ndevices++;
        add_switch_device = 1;
    } else if (_devices[_ndevices].dev_type & BDE_ETHER_DEV_TYPE) {
        _ether_ndevices++;
    } else if (_devices[_ndevices].dev_type & BDE_CPU_DEV_TYPE) {
        _cpu_ndevices++;
    } else {
        return;
    }
    _ndevices++;

    /*
     * In order to be backward compatible with the user mode BDE
     * (specifically the interrupt IOCTLs) and the CM, switch devices
     * *must* come first. If this is not the case (due to the probing
     * order), we let the non-switch device(s) drop down to the end of
     * the device array.
     */
    if (add_switch_device) {
        bde_ctrl_t tmp_dev;
        int i, s = 0;

        while (s < _switch_ndevices) {
            if (_devices[s].dev_type & BDE_SWITCH_DEV_TYPE) {
                s++;
                continue;
            }
            tmp_dev = _devices[s];
            for (i = s; i < _ndevices - 1; i++) {
                _devices[i] = _devices[i+1];
            }
            _devices[i] = tmp_dev;
        }

#ifdef INCLUDE_SRAM_DMA
        if (!use_sram_for_dma || (_devices[_switch_ndevices-1].dev_type & BDE_USER_DEV_TYPE) == 0)
#endif /* INCLUDE_SRAM_DMA */
        {
            _dma_per_device_init(_switch_ndevices-1);
        }
    }

    /* Initialize device locks */
    spin_lock_init(&_devices[_ndevices-1].lock);
}

static int
_eb_device_create(resource_size_t paddr, int irq, int rd_hw, int wr_hw)
{
    bde_ctrl_t *ctrl;
    uint32  dev_rev_id = 0x0, dev_id;

    dev_id = _ndevices;

    ctrl = _devices + _ndevices;

    ctrl->dev_type |= BDE_EB_DEV_TYPE | BDE_SWITCH_DEV_TYPE;
    ctrl->pci_device = NULL; /* No PCI bus */

    if(rd_hw) {
        ctrl->dev_type |= BDE_DEV_BUS_RD_16BIT;
    }

    if (wr_hw) {
        ctrl->dev_type |= BDE_DEV_BUS_WR_16BIT;
    }

    /* Map in the device */
    ctrl->bde_dev.base_address = (sal_vaddr_t)ioremap(paddr, 0x10000);
    ctrl->iowin[0].addr = paddr;
    ctrl->iowin[0].size = 0x10000;

    dev_rev_id = _read(dev_id, 0x178);  /* CMIC_DEV_REV_ID */

    ctrl->bde_dev.device = dev_rev_id & 0xFFFF;
    ctrl->bde_dev.rev = (dev_rev_id >> 16) & 0xFF;

    ctrl->iLine = irq;
    ctrl->isr = NULL;
    ctrl->isr_data = NULL;

    gprintk("Created EB device at BA=%x IRQ=%d RD16=%d WR16=%d device=0x%x\n",
            (unsigned int)paddr, irq, rd_hw, wr_hw, ctrl->bde_dev.device);

    _bde_add_device();

    return 0;
}

#ifdef IPROC_CMICD
static void
iproc_cmicd_get_irqres(ibde_dev_t bde_dev, struct resource *res_irq)
{
    shbde_iproc_config_t iproc_config, *icfg = &iproc_config;

    /* iProc configuration parameters */
    memset(icfg, 0, sizeof(*icfg));
    shbde_iproc_config_init(icfg, bde_dev.device, bde_dev.rev);

    if ((icfg->iproc_ver == 0) && (debug >= 1)) {
        gprintk("Unable to determine iProc version\n");
    }

    if (icfg->iproc_ver == 7) {
        res_irq->start = 221;
    } else if (icfg->iproc_ver == 10) {
        res_irq->start = 184;
    }

}

#include <linux/platform_device.h>
#include <linux/of.h>

extern int iproc_platform_driver_register(struct platform_driver *drv);
extern void iproc_platform_driver_unregister(struct platform_driver *drv);
extern int iproc_platform_device_register(struct platform_device *drv);
extern void iproc_platform_device_unregister(struct platform_device *drv);

extern struct resource *
iproc_platform_get_resource(struct platform_device *dev, unsigned int type,
                            unsigned int num);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,18,0))
extern int
iproc_platform_get_irq(struct platform_device *dev, unsigned int num);
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(5,18,0)) */

#define IPROC_CHIPCOMMONA_BASE  0x18000000
#define IPROC_CMICD_BASE        0x48000000
#define IPROC_CMICD_SIZE        0x40000
#define IPROC_CMICD_INT         194

#define IPROC_CMICD_COMPATIBLE "brcm,iproc-cmicd"
#define IPROC_CMICX_COMPATIBLE "brcm,iproc-cmicx"

static int
iproc_cmicd_probe(struct platform_device *pldev)
{
    bde_ctrl_t *ctrl;
    uint32 size, dev_rev_id;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,18,0))
    struct resource *memres;
    int irq;
#else
    struct resource *memres, *irqres;
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(5,18,0)) */

#ifdef CONFIG_OF
    if (debug >= 1) {
        gprintk("iproc_cmicd_probe %s\n", pldev->dev.of_node ? "with device node":"");
    }
#endif
    memres = iproc_platform_get_resource(pldev, IORESOURCE_MEM, 0);
    if (memres == NULL) {
        gprintk("Unable to retrieve iProc CMIC resources");
        return -1;
    }
    size = memres->end - memres->start + 1;

    ctrl = _devices + _ndevices;

    ctrl->dev_type = BDE_AXI_DEV_TYPE | BDE_SWITCH_DEV_TYPE | BDE_256K_REG_SPACE;
    ctrl->pci_device = NULL; /* No PCI bus */

    /* Map CMIC block in the AXI memory space into CPU address space */
    ctrl->bde_dev.base_address = (sal_vaddr_t)ioremap(memres->start, size);
    if (!ctrl->bde_dev.base_address) {
        gprintk("Error mapping iProc CMIC registers");
        return -1;
    }
    ctrl->iowin[0].addr = memres->start;
    ctrl->iowin[0].size = size;

#ifdef CONFIG_OF
    if (of_find_compatible_node(NULL, NULL, IPROC_CMICX_COMPATIBLE)) {
        uint32 *icfg_chip_id;
        icfg_chip_id = (uint32 *)ioremap(ICFG_CHIP_ID_REG, 2 * sizeof(uint32));
        if (icfg_chip_id == NULL) {
            gprintk("Error mapping ICFG_CHIP_ID_REG\n");
            return -1;
        }
        ctrl->bde_dev.device = readl(icfg_chip_id) & 0xffff;
        ctrl->bde_dev.rev = readl(icfg_chip_id+1) & 0xff;
        iounmap(icfg_chip_id);
        /* Map GICD block in the AXI memory space into CPU address space */
        memres = iproc_platform_get_resource(pldev, IORESOURCE_MEM, 1);
        if (memres) {
            ctrl->bde_dev.base_address1 = (sal_vaddr_t)ioremap(memres->start, memres->end - memres->start + 1);
            ctrl->iowin[1].addr = memres->start;
            ctrl->iowin[1].size = memres->end - memres->start + 1;
        } else {
            /* Use default address if not available in DTB */
            ctrl->bde_dev.base_address1 = (sal_vaddr_t)ioremap(IHOST_GICD_REG_ADDR, IHOST_GICD_REG_REMAP_LEN);
            ctrl->iowin[1].addr = IHOST_GICD_REG_ADDR;
            ctrl->iowin[1].size = IHOST_GICD_REG_REMAP_LEN;
        }
        if (ctrl->bde_dev.base_address1) {
            if (debug >= 1) {
                gprintk("base_address1:0x%lx phys_address1:0x%lx\n",
                        (unsigned long)ctrl->bde_dev.base_address1, (unsigned long)ctrl->iowin[1].addr);
            }
        } else {
            gprintk("Error mapping ihost GICD registers\n");
        }
        if (dma_set_mask_and_coherent(&pldev->dev, DMA_BIT_MASK(64))) {
            gprintk("Unable to set 64-bit dma mask\n");
        }
    } else
#endif
    {
        /* Read switch device ID from CMIC */
        dev_rev_id = *((uint32 *)(ctrl->bde_dev.base_address + 0x10224));
#if defined(BCM_CMICM_SUPPORT) && defined(BE_HOST)
        ctrl->bde_dev.device = (  (((dev_rev_id >> 16) & 0xff) << 8) |
                                   ((dev_rev_id >> 24) & 0xff));
        ctrl->bde_dev.rev    =      (dev_rev_id >>  8) & 0xff ;
#else
        ctrl->bde_dev.device = dev_rev_id & 0xffff;
        ctrl->bde_dev.rev = (dev_rev_id >> 16) & 0xff;
#endif
        ctrl->bde_dev.base_address1 = 0;
    }

#ifdef CONFIG_OF
    if (!pldev->dev.of_node)
#endif
    {
        /* Assign locally if not available from device node */
        iproc_cmicd_get_irqres(ctrl->bde_dev, &pldev->resource[0]);
    }

#ifdef CONFIG_OF
    if (of_find_compatible_node(NULL, NULL, IPROC_CMICX_COMPATIBLE)) {
        int i;
        memset(iproc_cmicx_irqs, 0, IHOST_CMICX_MAX_INTRS*sizeof(uint32_t));
        for (i = 0; i < IHOST_CMICX_MAX_INTRS; i++) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,18,0))
            irq = iproc_platform_get_irq(pldev, i);
            if (irq >= 0) {
                iproc_cmicx_irqs[i] = irq;
            }
#else
            irqres = iproc_platform_get_resource(pldev, IORESOURCE_IRQ, i);
            if (irqres) {
                iproc_cmicx_irqs[i] = irqres->start;
            }
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(5,18,0)) */
            if (debug >= 1) {
                gprintk("iproc_cmicx_irqs[%d] = %d\n", i, iproc_cmicx_irqs[i]);
            }
        }
        ctrl->iLine = iproc_cmicx_irqs[0];
    } else
#endif
    {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,18,0))
        irq = iproc_platform_get_irq(pldev, 0);
        ctrl->iLine = irq;
#else
        irqres = iproc_platform_get_resource(pldev, IORESOURCE_IRQ, 0);
        ctrl->iLine = irqres->start;
#endif /* (LINUX_VERSION_CODE >= KERNEL_VERSION(5,18,0)) */
    }

    ctrl->isr = NULL;
    ctrl ->isr_data = NULL;

#ifdef LINUX_BDE_DMA_DEVICE_SUPPORT
    ctrl->dma_dev = &pldev->dev;
#endif

    /* Let's boogie */
    _bde_add_device();
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
static int
iproc_cmicd_remove(struct platform_device *pldev)
#else
static void
iproc_cmicd_remove(struct platform_device *pldev)
#endif
{
    int i;
    uint32 mask = BDE_SWITCH_DEV_TYPE | BDE_AXI_DEV_TYPE;
    bde_ctrl_t *ctrl;

    for (i = 0; i < _ndevices; i++) {
        ctrl = _devices + i;
        if ((ctrl->dev_type & mask) == mask) {
            if (ctrl->bde_dev.base_address1) {
                iounmap((void *)ctrl->bde_dev.base_address1);
                ctrl->bde_dev.base_address1 = 0;
            }

            if (ctrl->bde_dev.base_address) {
                iounmap((void *)ctrl->bde_dev.base_address);
                ctrl->bde_dev.base_address = 0;
            }
        }
    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 11, 0)
    return 0;
#endif
}
#ifdef CONFIG_OF
static const struct of_device_id iproc_cmicd_of_match[] = {
    { .compatible = IPROC_CMICD_COMPATIBLE },
    { .compatible = IPROC_CMICX_COMPATIBLE },
    {},
};
MODULE_DEVICE_TABLE(of, iproc_cmicd_of_match);
#endif
static char iproc_cmicd_string[] = "bcmiproc-cmicd";

static struct platform_driver iproc_cmicd_driver =
{
    .probe = iproc_cmicd_probe,
    .remove = iproc_cmicd_remove,
    .driver =
    {
        .name = iproc_cmicd_string,
        .owner = THIS_MODULE,
#ifdef CONFIG_OF
        .of_match_table = iproc_cmicd_of_match,
#endif
    },
};

typedef enum {
    IPROC_CMICD_RES_INTR = 0,
    IPROC_CMICD_RES_MEM
} IPROC_CMICD_RES_E;

static struct resource iproc_cmicd_resources[] = {
    [IPROC_CMICD_RES_INTR] = {
        .flags  = IORESOURCE_IRQ,
        .start  = IPROC_CMICD_INT,
    },
    [IPROC_CMICD_RES_MEM] = {
        .flags  = IORESOURCE_MEM,
        .start  = IPROC_CMICD_BASE,
        .end    = IPROC_CMICD_BASE+IPROC_CMICD_SIZE-1,
    },
};

static void
iproc_cmicd_release(struct device *dev)
{
}

static u64 iproc_cmicd_dmamask = DMA_BIT_MASK(32);

static struct platform_device iproc_cmicd_pdev = {
    .name = iproc_cmicd_string,
    .id = 0,
    .dev =  {
        .release = iproc_cmicd_release,
        .init_name = iproc_cmicd_string,
        .dma_mask = &iproc_cmicd_dmamask,
        .coherent_dma_mask = DMA_BIT_MASK(32),
    },
    .resource = iproc_cmicd_resources,
    .num_resources = ARRAY_SIZE(iproc_cmicd_resources),
};

static int
iproc_has_cmicd(void)
{
    void *iproc_cca_base;

    /* Read ChipcommonA chip id register to identify current SOC */
    iproc_cca_base = ioremap(IPROC_CHIPCOMMONA_BASE, 0x3000);
    if (iproc_cca_base == NULL) {
        gprintk("iproc_has_cmicd: ioremap of ChipcommonA registers failed");
        return 0;
    }
    iounmap(iproc_cca_base);

    /* Device has CMIC */
    return 1;
}

#define IPROC_CHIPCOMMONA_EROM_PTR_OFFSET   (0x180000fc)
#define EROM_MAX_SIZE   (0x1000)
#define EROM_PARTNUM_CMICD   (0x14)

#define EROM_DESC_COMPIDENT  (0x1)
#define EROM_DESC_MASTER     (0x3)
#define EROM_DESC_ADDR       (0x5)
#define EROM_DESC_END        (0xF)
#define EROM_DESC_EMPTY      (0)

#define EROM_IS_DESC_COMPIDENT(x) ((x & 0x7) == EROM_DESC_COMPIDENT)
#define EROM_IS_DESC_MASTER(x)    ((x & 0x7) == EROM_DESC_MASTER)
#define EROM_IS_DESC_ADDR(x)      ((x & 0x7) == EROM_DESC_ADDR)
#define EROM_IS_DESC_END(x)       ((x & 0xF) == EROM_DESC_END)

#define EROM_GET_PARTNUM(x)       ((x >> 8) & (0xFFF))       /* Bit 8~19 */
#define EROM_GET_ADDRESS(x)       ((x >> 12) & (0xFFFFF))    /* Bit 12~31 */
#define EROM_GET_SIZETYPE(x)      ((x >> 4) & (0x3))         /* Bit 4~5 */
#define EROM_GET_AG32(x)          ((x >> 3) & (0x1))         /* Bit 3 */
#define EROM_GET_SIZE(x)          ((x >> 12) & (0xFFFFF))    /* Bit 12~31 */
#define EROM_GET_SG32(x)          ((x >> 3) & (0x1))         /* Bit 3 */

#define EROM_ADDR_SIZETYPE_4K   (0)
#define EROM_ADDR_SIZETYPE_8K   (1)
#define EROM_ADDR_SIZETYPE_16K  (2)
#define EROM_ADDR_SIZETYPE_MORE (3)

#define EROM_ADDR_FLAG_AG32        (1) /* Address space greater than 32 bit */
#define EROM_ADDR_FLAG_SIZE        (2) /* Addition size descriptor */
#define EROM_ADDR_FLAG_SG32        (4) /* Size descriptor greater than 32 bit */

static void
iproc_cmicd_get_memregion(struct resource *res_mem)
{
    void *erom_ptr_oft;
    uint32_t erom_phy_addr;
    uint32_t *erom_base;
    uint32_t i = 0;
    uint32_t word = 0;
    uint8_t more_addr_word = 0; /* bit 0: AG32; bit 1: SIZE; bit 2: SG32 */
    uint8_t found_cmicd_dev = 0;
    uint8_t size_type = 0;
    bool is_compident_a = 1; /* 1: CompidentA; o/w: CompidentB */

    erom_ptr_oft = ioremap(IPROC_CHIPCOMMONA_EROM_PTR_OFFSET, 0x100);

    erom_phy_addr = readl((uint32 *)(erom_ptr_oft));
    iounmap(erom_ptr_oft);

    erom_base = ioremap(erom_phy_addr, EROM_MAX_SIZE);

    while (1) {
        word = readl((uint32 *)(erom_base + i));

        if (EROM_IS_DESC_ADDR(word) || more_addr_word) {
            if (more_addr_word == 0) { /* Normal Addr Desc */
                if (EROM_GET_AG32(word) == 1) {
                    more_addr_word |= EROM_ADDR_FLAG_AG32;
                }

                size_type = EROM_GET_SIZETYPE(word);
                if (size_type == EROM_ADDR_SIZETYPE_MORE) {
                    more_addr_word |= EROM_ADDR_FLAG_SIZE;
                }

                if (found_cmicd_dev == 1) {
                    res_mem->start = EROM_GET_ADDRESS(word) << 12;
                    if (size_type < EROM_ADDR_SIZETYPE_MORE) {
                        res_mem->end = res_mem->start + 4096 * (1 << size_type) - 1;
                    }
                }
            }
            else if (more_addr_word & EROM_ADDR_FLAG_AG32) { /* UpperAddr Desc */
                more_addr_word &= ~EROM_ADDR_FLAG_AG32;

                if (found_cmicd_dev == 1) {
                    /* res_mem->start |= word << 32; */
                    gprintk("Expect cmicd address to be 32-bit\n");
                }
            }
            else if (more_addr_word & EROM_ADDR_FLAG_SIZE) { /* Size Desc */
                if (EROM_GET_SG32(word) == 1) {
                    more_addr_word |= EROM_ADDR_FLAG_SG32;
                }

                more_addr_word &= ~EROM_ADDR_FLAG_SIZE;

                if (found_cmicd_dev == 1) {
                    res_mem->end = res_mem->start + (EROM_GET_SIZE(word) << 12) - 1;
                }
            }
            else if (more_addr_word & EROM_ADDR_FLAG_SG32) { /* UpperSize Desc */
                more_addr_word &= ~EROM_ADDR_FLAG_SG32;

                if (found_cmicd_dev == 1) {
                    /* res_mem->end += (word) << 32; */
                    gprintk("Expect cmicd size to be 32-bit\n");
                }
            }

            if (found_cmicd_dev == 1 && more_addr_word == 0) {
                break;  /* We have gotten all necessary information, exit the loop */
            }
        }
        else if (EROM_IS_DESC_COMPIDENT(word)) {
            if (is_compident_a == 1) {
                if (EROM_GET_PARTNUM(word) == EROM_PARTNUM_CMICD) {
                    found_cmicd_dev = 1;
                }
            }

            is_compident_a = 1 - is_compident_a;
        }
        else if (EROM_IS_DESC_END(word)) {
            break;
        }

        i++;
    }
    iounmap(erom_base);

    if (debug >= 1) {
        gprintk("CMICD info by %s: cmicd_mem.start=%lx, cmicd_mem.end=%lx\n",
                found_cmicd_dev ? "EROM" : "Default",
                (unsigned long)iproc_cmicd_resources[IPROC_CMICD_RES_MEM].start,
                (unsigned long)iproc_cmicd_resources[IPROC_CMICD_RES_MEM].end);
    }
}
#endif /* IPROC_CMICD */

#ifdef BCM_ICS
static int
_ics_bde_create(void)
{
    bde_ctrl_t *ctrl;
    uint32 dev_rev_id = 0x0;
    resource_size_t paddr;

    if (_ndevices == 0) {
        ctrl = _devices + _ndevices;

        ctrl->dev_type |= BDE_ICS_DEV_TYPE | BDE_SWITCH_DEV_TYPE;
        ctrl->pci_device = NULL; /* No PCI bus */

        /* Map in the device */
        paddr = BCM_ICS_CMIC_BASE;
        ctrl->bde_dev.base_address = (sal_vaddr_t)ioremap(paddr, 0x10000);
        ctrl->iowin[0].addr = paddr;
        ctrl->iowin[0].size = 0x10000;

        dev_rev_id = *((unsigned int *)(KSEG1ADDR(paddr + 0x178)));

        ctrl->bde_dev.device = dev_rev_id & 0xFFFF;
        ctrl->bde_dev.rev = (dev_rev_id >> 16) & 0xFF;

        ctrl->iLine = 5; /* From raptor linux BSP */

        ctrl->isr = NULL;
        ctrl->isr_data = NULL;
        printk("Created ICS device ..%x\n", ctrl->bde_dev.base_address);
        _bde_add_device();
    }

    return 0;
}

#else /* !BCM_ICS */

#ifdef CONFIG_PCI
extern struct pci_bus *pci_find_bus(int domain, int busnr);

/*
 * PCI device table.
 * Populated from the include/soc/devids.h file.
 */

static const struct pci_device_id _id_table[] = {
    { BROADCOM_VENDOR_ID, BCM5675_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM5676_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56218X_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56218_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56219_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56218R_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56219R_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56214_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56215_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56214R_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56215R_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56216_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56217_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56212_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56213_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56230_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56231_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53718_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53714_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53716_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56018_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56014_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56224_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56225_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56226_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56227_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56228_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56229_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56024_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56025_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53724_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53726_DEVICE_ID,  PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56100_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56101_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56102_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56105_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56106_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56107_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56110_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56111_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56112_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56115_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56116_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56117_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56300_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56301_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56302_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56303_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56304_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56404_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56305_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56306_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56307_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56308_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56309_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56310_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56311_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56312_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56313_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56314_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56315_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56316_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56317_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56318_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56319_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
#ifndef EXCLUDE_BCM56324
    { BROADCOM_VENDOR_ID, BCM56322_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56324_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
#endif /* EXCLUDE_BCM56324 */
    { BROADCOM_VENDOR_ID, BCM53312_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53313_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53314_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53324_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53333_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53334_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53342_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53343_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53344_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53346_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53347_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53393_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53394_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53300_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53301_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53302_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56500_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56501_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56502_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56503_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56504_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56505_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56506_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56507_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56508_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56509_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56510_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56511_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56512_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56513_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56514_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56516_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56517_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56518_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56519_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56580_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56620_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56624_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56626_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56628_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56629_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56680_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56684_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56700_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56701_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56800_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56801_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56802_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56803_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56534_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56334_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56320_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56321_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56132_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56134_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56150_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56151_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56152_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56930_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56931_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56935_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56936_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56939_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56840_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56841_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56842_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56843_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56844_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56845_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56846_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56847_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56549_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56053_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56838_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56831_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56835_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56849_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56742_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56743_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56744_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56745_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56746_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56640_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56548_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56547_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56346_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56345_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56344_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56342_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56340_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56049_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56048_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56047_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56042_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56041_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56040_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56643_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56644_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56648_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56649_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56540_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56541_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56542_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56543_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56544_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56545_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56546_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56044_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56045_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56046_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM55440_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56440_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56441_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56442_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56443_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56445_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56446_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56447_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56448_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56449_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56240_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56241_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56242_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56243_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56245_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56246_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56260_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56270_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56271_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56272_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53460_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53461_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56261_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56262_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56263_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56265_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56266_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56267_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56268_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56233_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56460_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56461_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56462_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56463_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56465_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56466_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56467_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56468_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56246_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM55440_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM55441_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56060_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56062_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56063_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56064_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56065_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56066_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53401_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53411_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53402_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53412_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53403_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53413_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53404_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53414_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53405_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53415_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53406_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53416_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53408_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53418_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53454_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53455_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53456_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53457_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53422_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53424_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53426_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53365_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53369_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56960_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56961_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56962_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56963_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56930_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56968_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56970_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56971_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56972_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56974_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56975_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56168_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56169_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56670_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56671_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56672_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56675_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53650_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53651_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53652_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53653_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53654_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56670_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56160_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56162_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56163_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56164_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56166_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53440_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53443_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53442_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53434_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56965_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56969_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56966_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56967_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56170_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56172_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56174_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53570_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53575_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56070_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56071_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56072_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { PCI_VENDOR_ID_PLX, PCI_DEVICE_ID_PLX_9656, PCI_ANY_ID, PCI_ANY_ID },
    { PCI_VENDOR_ID_PLX, PCI_DEVICE_ID_PLX_9056, PCI_ANY_ID, PCI_ANY_ID },
    { BCM53000_VENDOR_ID, BCM53000PCIE_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
#ifdef BCM_PETRA_SUPPORT
    { BROADCOM_VENDOR_ID, BCM88350_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88351_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88450_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88451_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88550_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88551_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88552_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88660_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88670_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88671_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88671M_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88672_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88673_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88674_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88675_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88675M_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88676_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88676M_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88677_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88678_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88679_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88370_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88371_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88371M_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88375_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88470_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88470P_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88471_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88473_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88474_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88474H_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88476_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88477_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88479_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },


    { BROADCOM_VENDOR_ID, BCM88270_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88271_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88272_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88273_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88274_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88276_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88278_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88279_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },

    { BROADCOM_VENDOR_ID, BCM88376_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88376M_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88377_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88378_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88379_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88680_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88681_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88682_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88683_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88684_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88685_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88687_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88380_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88381_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88360_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88361_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88363_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88460_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88461_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88560_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88561_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88562_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88661_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88664_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
#endif
#ifdef BCM_DNX_SUPPORT
    { BROADCOM_VENDOR_ID, BCM88690_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88690_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88691_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88692_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88693_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88694_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88695_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88696_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88697_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88698_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88699_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8869A_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8869B_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8869C_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8869D_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8869E_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8869F_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88800_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88801_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88802_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88803_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88804_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88805_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88806_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88807_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88808_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88809_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8880A_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8880B_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8880C_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8880D_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8880E_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8880F_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88820_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88821_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88822_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88823_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88824_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88825_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88826_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88827_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88828_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88829_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8882A_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8882B_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8882C_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8882D_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8882E_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8882F_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88480_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88481_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88482_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88483_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88484_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88485_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88486_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88487_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88488_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88489_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8848A_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8848B_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8848C_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8848D_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8848E_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8848F_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88280_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88281_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88282_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88283_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88284_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88285_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88286_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88287_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88288_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88289_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8828A_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8828B_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8828C_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8828D_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8828E_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8828F_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88290_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88291_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88292_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88293_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88294_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88295_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88296_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88297_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88298_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88299_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8829A_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8829B_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8829C_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8829D_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8829E_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8829F_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88850_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88851_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88852_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88853_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88854_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88855_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88856_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88857_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88858_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88859_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8885A_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8885B_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8885C_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8885D_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8885E_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8885F_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88840_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88841_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88842_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88843_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88844_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88845_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88846_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88847_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88848_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88849_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8884A_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8884B_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8884C_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8884D_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8884E_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8884F_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88810_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88811_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88812_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88813_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88814_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88815_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88816_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88817_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88818_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88819_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8881A_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8881B_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8881C_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8881D_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8881E_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8881F_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88830_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88831_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88832_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88833_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88834_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88835_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88836_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88837_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88838_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88839_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8883A_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8883B_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8883C_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8883D_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8883E_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8883F_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
#ifdef BCM_DNX3_SUPPORT
    { BROADCOM_VENDOR_ID, JERICHO3_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO3_DEVICE_ID + 1, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO3_DEVICE_ID + 2, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO3_DEVICE_ID + 3, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO3_DEVICE_ID + 4, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO3_DEVICE_ID + 5, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO3_DEVICE_ID + 6, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3_PLUS_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO3_DEVICE_ID + 8, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO3_DEVICE_ID + 9, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3_DEVICE_ID_START, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3_DEVICE_ID_START + 1, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3_DEVICE_ID_START + 2, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3_DEVICE_ID_START + 3, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3_DEVICE_ID_START + 4, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3_DEVICE_ID_START + 5, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_DEVICE_ID + 1, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_DEVICE_ID + 2, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_DEVICE_ID + 3, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_DEVICE_ID + 4, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_DEVICE_ID + 5, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_DEVICE_ID + 6, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_PLUS_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_PLUS_DEVICE_ID + 1, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_PLUS_DEVICE_ID + 2, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_PLUS_DEVICE_ID + 3, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_PLUS_DEVICE_ID + 4, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_PLUS_DEVICE_ID + 5, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_PLUS_DEVICE_ID + 6, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_PLUS_DEVICE_ID + 7, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J3AI_PLUS_DEVICE_ID + 8, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3D_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3D_DEVICE_ID + 1, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3D_DEVICE_ID + 2, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3D_DEVICE_ID + 3, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3D_DEVICE_ID + 4, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3D_DEVICE_ID + 5, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3D_DEVICE_ID + 6, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3D_DEVICE_ID + 7, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3D_DEVICE_ID + 8, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3D_DEVICE_ID + 9, PCI_ANY_ID, PCI_ANY_ID },
#ifdef BCM_Q3A_SUPPORT
    { BROADCOM_VENDOR_ID, Q3A_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3A_DEVICE_ID + 1, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3A_DEVICE_ID + 2, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3A_DEVICE_ID + 3, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3A_DEVICE_ID + 4, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3A_DEVICE_ID + 5, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3A_DEVICE_ID + 6, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3A_DEVICE_ID + 7, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3A_DEVICE_ID + 8, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3A_DEVICE_ID + 9, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3U_ORIG_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3U_ORIG_DEVICE_ID + 1, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3U_ORIG_DEVICE_ID + 2, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3U_ORIG_DEVICE_ID + 3, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3U_ORIG_DEVICE_ID + 4, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3N_ORIG_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3N_ORIG_DEVICE_ID + 1, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3N_ORIG_DEVICE_ID + 2, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3N_ORIG_DEVICE_ID + 3, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q3N_ORIG_DEVICE_ID + 4, PCI_ANY_ID, PCI_ANY_ID },
#endif
#ifdef BCM_JERICHO_4_SUPPORT
    { BROADCOM_VENDOR_ID, JERICHO4_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO4_DEVICE_ID + 1, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO4_DEVICE_ID + 2, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO4_DEVICE_ID + 3, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO4_DEVICE_ID + 4, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO4_DEVICE_ID + 5, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO4_DEVICE_ID + 6, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO4_DEVICE_ID + 7, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO4_DEVICE_ID + 8, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, JERICHO4_DEVICE_ID + 9, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4_DEVICE_ID + 1, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4_DEVICE_ID + 2, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4_DEVICE_ID + 3, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4_DEVICE_ID + 4, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4_DEVICE_ID + 5, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4_DEVICE_ID + 6, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4_DEVICE_ID + 7, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4_DEVICE_ID + 8, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4_DEVICE_ID + 9, PCI_ANY_ID, PCI_ANY_ID },
#endif

#ifdef BCM_Q4D_SUPPORT
    { BROADCOM_VENDOR_ID, Q4D_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4D_DEVICE_ID + 1, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4D_DEVICE_ID + 2, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4D_DEVICE_ID + 3, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4D_DEVICE_ID + 4, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4D_DEVICE_ID + 5, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4D_DEVICE_ID + 6, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4D_DEVICE_ID + 7, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4D_DEVICE_ID + 8, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, Q4D_DEVICE_ID + 9, PCI_ANY_ID, PCI_ANY_ID },
#endif


#ifdef BCM_J4L_SUPPORT
    { BROADCOM_VENDOR_ID, J4L_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J4L_DEVICE_ID + 1, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J4L_DEVICE_ID + 2, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J4L_DEVICE_ID + 3, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J4L_DEVICE_ID + 4, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J4L_DEVICE_ID + 5, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J4L_DEVICE_ID + 6, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J4L_DEVICE_ID + 7, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J4L_DEVICE_ID + 8, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, J4L_DEVICE_ID + 9, PCI_ANY_ID, PCI_ANY_ID },
#endif


#endif
#endif /* BCM_DNX_SUPPORT */
#ifdef BCM_DFE_SUPPORT
    { BROADCOM_VENDOR_ID, BCM88770_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88773_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88774_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88775_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88776_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88777_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
#ifndef DNX_IGNORE_FE3200
    { BROADCOM_VENDOR_ID, BCM88950_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
#endif
    { BROADCOM_VENDOR_ID, BCM88953_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88954_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88955_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88956_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88772_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88952_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
#endif
#ifdef BCM_DNXF_SUPPORT
    { BROADCOM_VENDOR_ID, BCM88790_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88791_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88792_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88793_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88794_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88795_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88796_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88797_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88798_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM88799_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8879A_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8879B_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8879C_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8879D_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM8879F_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
#ifdef BCM_DNXF3_SUPPORT
    { BROADCOM_VENDOR_ID, RAMON2_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON2_DEVICE_ID + 1, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON2_DEVICE_ID + 2, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON2_DEVICE_ID + 3, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON2_DEVICE_ID + 4, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON2_DEVICE_ID + 5, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON2_DEVICE_ID + 6, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON2_DEVICE_ID + 7, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON2_DEVICE_ID + 8, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON2_DEVICE_ID + 9, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON3_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON3_DEVICE_ID + 1, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON3_DEVICE_ID + 2, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON3_DEVICE_ID + 3, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON3_DEVICE_ID + 4, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON3_DEVICE_ID + 5, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON3_DEVICE_ID + 6, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON3_DEVICE_ID + 7, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON3_DEVICE_ID + 8, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON3_DEVICE_ID + 9, PCI_ANY_ID, PCI_ANY_ID },
#endif
#endif
#ifdef BCM_DNXFE_SUPPORT
#ifdef BCM_RAMON_4_SUPPORT
    { BROADCOM_VENDOR_ID, RAMON4_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON4_DEVICE_ID + 1, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON4_DEVICE_ID + 2, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON4_DEVICE_ID + 3, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON4_DEVICE_ID + 4, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON4_DEVICE_ID + 5, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON4_DEVICE_ID + 6, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON4_DEVICE_ID + 7, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON4_DEVICE_ID + 8, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, RAMON4_DEVICE_ID + 9, PCI_ANY_ID, PCI_ANY_ID },
#endif
#endif
    { BROADCOM_VENDOR_ID, BCM56860_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56861_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56862_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56864_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56865_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56866_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56867_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56868_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56833_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56832_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56836_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56870_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56273_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56274_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56275_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56276_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56277_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56278_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56279_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56575_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56175_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56176_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53642_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56370_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56371_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56372_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56374_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56375_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56376_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56377_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56577_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56578_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56579_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56873_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56770_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56771_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56980_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56981_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56982_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56983_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56984_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53540_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53547_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53548_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM53549_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56470_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56471_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56472_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56475_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56474_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56390_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56391_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { BROADCOM_VENDOR_ID, BCM56392_DEVICE_ID, PCI_ANY_ID, PCI_ANY_ID },
    { 0, 0, 0, 0 }
};;

MODULE_DEVICE_TABLE(pci, _id_table);

#define pci_bus_b(n) list_entry(n, struct pci_bus, node)
#define pci_dev_b(n) list_entry(n, struct pci_dev, bus_list)
#define MAX_RC_NUM 4

static struct pci_bus *
pci_do_bus_find(struct pci_bus* bus, int rc, int vendor, int device)
{
    struct list_head *tmp;
    struct pci_dev *dev;
    struct pci_bus *sub_bus;
    int func = 0;

    if (unlikely(list_empty(&bus->children))) {
        return NULL;
    }
    list_for_each(tmp, &bus->children) {
        sub_bus = pci_bus_b(tmp);
        dev = sub_bus->self;
        func = dev->devfn & 0x7;
        if (dev->vendor == vendor && dev->device == device && rc == func) {
            if (debug >= 1) {
                gprintk("pci_do_bus_find: dev->vendor = 0x%x, dev->device = 0x%x on rc(%d)\n",
                        dev->vendor, dev->device, rc);
            }
            return sub_bus;
        }
    }
    return NULL;
}

static struct pci_dev *
_pci_do_bus_dev_find(struct pci_bus* bus, unsigned int vendor, unsigned int device)
{
    struct list_head *tmp;
    struct pci_dev *dev;

    if (unlikely(list_empty(&bus->devices))) {
        return NULL;
    }
    list_for_each(tmp, &bus->devices) {
        dev = pci_dev_b(tmp);
        if (dev->vendor == vendor && (device == PCI_ANY_ID || dev->device == device)) {
            if (debug >= 1) {
                gprintk("_pci_do_rc_dev_find: vendor = %x, device = %x\n", vendor, device);
            }
            return dev;
        }
    }
    return NULL;
}

static struct pci_dev *
pci_do_rc_dev_find(int rc)
{
    struct pci_bus *root_bus = pci_find_bus(0,0);
    struct pci_bus *bus_rc = NULL;
    struct pci_dev *dev_on_rc = NULL;
    unsigned int pci_dev_id = 0;

    if(NULL == root_bus) {
        if (debug >= 1) gprintk("Not find root bus\n");
        return NULL;
    }
    bus_rc = pci_do_bus_find(root_bus, rc, 0x184e, 0x1004);
    if (NULL == bus_rc) {
        if (debug >= 1) {
            gprintk("Not find vendor(0x184e) device(0x1004) bus\n");
        }
        return NULL;
    }
    for(pci_dev_id = 0; pci_dev_id < sizeof(_id_table)/sizeof(struct pci_device_id); pci_dev_id++) {
        dev_on_rc = _pci_do_bus_dev_find(bus_rc, _id_table[pci_dev_id].vendor, _id_table[pci_dev_id].device);
        if (NULL != dev_on_rc) {
            return dev_on_rc;
        }
    }
    if (debug >= 1) {
        gprintk("Not find device at rc(%d)\n", rc);
    }
    return NULL;
}

/*
 * Function: p2p_bridge
 *
 * Purpose:
 *    Finalize initialization secondary PCI-PCI bridge.
 * Parameters:
 *    membase - start of memory address space for secondary PCI bus
 * Returns:
 *    0
 * Notes:
 *    The membase depends on the processor architecture, and is
 *    derived from the memory space addresses set up by the kernel.
 */
static int
p2p_bridge(void)
{
    struct pci_dev *dev, *dev_on_rc;
    uint16 cmd;
    uint16 mem_base;
    uint16 mem_limit;
    uint8 bridge_ctrl;
    uint8 rc_index;

    if ((dev = PCI_FIND_DEV(DC21150_VENDOR_ID, DC21150_DEVICE_ID, NULL)) != NULL ||
        (dev = PCI_FIND_DEV(HINT_HB4_VENDOR_ID, HINT_HB4_DEVICE_ID, NULL)) != NULL ||
        (dev = PCI_FIND_DEV(PI7C8150_VENDOR_ID, PI7C8150_DEVICE_ID, NULL)) != NULL) {

        if (debug >= 1) gprintk("fixing up PCI-to-PCI bridge\n");
        /* Adjust command register */
        pci_read_config_word(dev, PCI_COMMAND, &cmd);
        cmd |= PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
        /* Disable device */
        pci_write_config_word(dev, PCI_COMMAND, 0);
        /* Initialize non-prefetchable memory window if needed */
        pci_read_config_word(dev, PCI_MEMORY_BASE, &mem_base);
        if (mem_base == 0) {
            mem_base = (uint16)((_pci_mem_start & 0xFFF00000) >> 16);
            mem_limit = (uint16)((_pci_mem_end & 0xFFF00000) >> 16);
            pci_write_config_word(dev, PCI_MEMORY_BASE, mem_base);
            pci_write_config_word(dev, PCI_MEMORY_LIMIT, mem_limit);
        }
        /* Enable PCI clocks on remote end */
        pci_write_config_word(dev, PCI_CFG_DEC21150_SEC_CLK, 0);
        /* Re-enable config space */
        pci_write_config_word(dev, PCI_COMMAND, cmd);

        /* Avoid DMA data corruption */
        if (dev->vendor == HINT_HB4_VENDOR_ID) {
            /* Fix for HiNT bridge and BCM4704 DMA problem */
            if ((dev = PCI_FIND_DEV(BCM4704_VENDOR_ID, BCM4704_DEVICE_ID, NULL)) != NULL) {
                /* Reset PrefetchEn (PE) */
                pci_write_config_dword(dev, 0x8c, 1);
                if (debug >= 1) {
                    gprintk("reset PrefetchEn on BCM4704 when HiNT bridge is present\n");
                }
            }
        }
    }
    /* Enable fast back-to-back read/write */
    if ((dev = PCI_FIND_DEV(PI7C8150_VENDOR_ID, PI7C8150_DEVICE_ID, NULL)) != NULL) {
        pci_read_config_word(dev, PCI_COMMAND, &cmd);
        cmd |= PCI_COMMAND_FAST_BACK;
        pci_read_config_byte(dev, PCI_BRIDGE_CONTROL, &bridge_ctrl);
        bridge_ctrl |= PCI_BRIDGE_CTL_FAST_BACK;
        pci_write_config_word(dev, PCI_COMMAND, 0);
        pci_write_config_byte(dev, PCI_BRIDGE_CONTROL, bridge_ctrl);
        pci_write_config_word(dev, PCI_COMMAND, cmd);
    }
    /* Netlogic XLP CPU */
    for(rc_index = 0; rc_index < MAX_RC_NUM; rc_index++) {
        dev_on_rc = pci_do_rc_dev_find(rc_index);
        if (dev_on_rc != NULL ) {
            dev = PCI_FIND_DEV(0x184e, 0x1004, NULL);
            if (dev != NULL ) {
                pci_write_config_dword(dev,0x78,MAX_PAYLOAD_256B |
                                                MAX_READ_REQ_256B);
            }
        }
    }
    if ((dev = PCI_FIND_DEV(0x14e4, 0xb634, NULL)) != NULL) {
        pci_write_config_dword(dev,0x78,MAX_PAYLOAD_256B |
                                        MAX_READ_REQ_256B);
    }

    if ((dev = PCI_FIND_DEV(PCI_VNDID_PERICOM, PCI_DEVID_PI7C9X130, NULL)) != NULL) {
        /*
         * Configure the PCIE cap: Max payload size: 256, Max Read
         * Request size: 256, disabling relax ordering.
         * Writes to the PCIE capability device control register
         */
        pci_write_config_dword(dev, DEV_CTRL_REG,
                               MAX_PAYLOAD_256B | MAX_READ_REQ_256B);
    }

    if ((dev = PCI_FIND_DEV(FSL_VENDOR_ID, FSL8548PCIE_DEVICE_ID, NULL)) != NULL ||
        (dev = PCI_FIND_DEV(FSL_VENDOR_ID, FSL2020EPCIE_DEVICE_ID, NULL)) != NULL) {
        /*
         * Configure the PCIE cap: Max payload size: 256, Max Read
         * Request size: 256, disabling relax ordering.
         * Writes to the PCIE capability device control register
         */
        pci_write_config_dword(dev, FSL8548PCIE_DEV_CTRL_REG,
                               MAX_PAYLOAD_256B | MAX_READ_REQ_256B);
    }
    if ((dev = PCI_FIND_DEV(BCM4716_VENDOR_ID, BCM4716PCIE_DEVICE_ID, NULL)) != NULL ||
        (dev = PCI_FIND_DEV(BCM53000_VENDOR_ID, BCM53000PCIE_DEVICE_ID, NULL)) != NULL) {
        uint32 tmp, maxpayld, device_bmp=0, mask;
        unsigned long addr;
        uint16 tmp16, tmp161;
        int i, bus0 = -1, bus1 = -1, port;
        struct pci_dev *pcie0, *pcie1;

        pcie0 = dev;
        bus0 = dev->bus->number;
        if ((pcie1 = PCI_FIND_DEV(BCM53000_VENDOR_ID, BCM53000PCIE_DEVICE_ID, pcie0)) != NULL) {
            bus1 = pcie1->bus->number;
        }

        for(i = 0; i < _ndevices; i++) {
            bde_ctrl_t *ctrl = _devices + i;

            mask = BDE_SWITCH_DEV_TYPE | BDE_PCI_DEV_TYPE;
            if ((ctrl->dev_type & mask) == mask) {
                if (ctrl->pci_device->bus->number == bus0) {
                    device_bmp |= 1 << 0;
                }
                if (ctrl->pci_device->bus->number == bus1) {
                    device_bmp |= 1 << 1;
                }
            }
        }

        /* configure the PCIE cap: Max payload size: 256, Max Read
         * Request size: 256, disabling relax ordering.
         * Writes to the PCIE capability device control register
         */

        i = 0;
        while(device_bmp) {
            if (device_bmp & (1 << i)){
                port = i ;
                pci_read_config_dword(BCM53000PCIE_DEV(port),
                                      BCM53000PCIE_DEV_CAP_REG, &tmp);
                maxpayld = (tmp & BCM53000PCIE_MAX_PAYLOAD_MASK);
                if (debug >= 1) {
                    gprintk("port %d\n",port);
                    gprintk("DevCap (@%x): 0x%x%c\n", BCM53000PCIE_DEV_CAP_REG, tmp,
                            (maxpayld != BCM53000PCIE_CAP_MAX_PAYLOAD_256B) ? ' ':'\n');
                }
                if (maxpayld != BCM53000PCIE_CAP_MAX_PAYLOAD_256B) {
                    addr = BCM53000PCIE_BASE(port);
                    addr |= (BCM53000PCIE_SROM_SPACE | BCM53000PCIE_SPROM_OFFSET);
                    tmp16 = *((uint16 *)addr);
                    if (debug >= 1){
                        gprintk("addr %lx spromData.MaxPayloadSize: 0x%x\n", addr, tmp16);
                    }
                    mask = BCM53000PCIE_SPROM_MAX_PAYLOAD_MASK;
                    if ((tmp16 & mask) != BCM53000PCIE_SPROM_MAX_PAYLOAD_256B) {
                        tmp161 = (tmp16 & ~mask) | BCM53000PCIE_SPROM_MAX_PAYLOAD_256B;
                        *((uint16 *)addr) = tmp161;
                        if (debug >= 1) {
                            tmp16 = 0;
                            tmp16 = *((uint16 *)addr);
                            gprintk("Enable spromData.MaxPayloadSize to 1 (256 bytes): "
                                    "0x%x (%s w/ 0x%x)\n", tmp161,
                                    ((tmp16 & mask) == BCM53000PCIE_SPROM_MAX_PAYLOAD_256B) ?
                                    "Success":"Fail",
                                    tmp16);
                        }
                    }
                    pci_read_config_dword(BCM53000PCIE_DEV(port),
                                          BCM53000PCIE_DEV_CAP_REG, &tmp);
                    if (debug >= 1){
                        gprintk("DevCap (@%x): now is 0x%x\n\n",
                                BCM53000PCIE_DEV_CAP_REG, tmp);
                    }
                }

                addr = BCM53000PCIE_BASE(port);
                addr |= (BCM53000PCIE_FUNC0_COFIG_SPACE | BCM53000PCIE_DEV_CTRL_REG);
                tmp16 = *((uint16 *)addr);
                if (debug >= 1) {
                    gprintk("DevControl (@%x): 0x%x\n", BCM53000PCIE_DEV_CTRL_REG, tmp16);
                }
                if (!(tmp16 & MAX_PAYLOAD_256B) || !(tmp16 & MAX_READ_REQ_256B)) {
                    tmp161 = tmp16 | MAX_PAYLOAD_256B | MAX_READ_REQ_256B;
                    *((uint16 *)addr) = tmp161;
                    if (debug >= 1) {
                        tmp16 = 0;
                        tmp16 = *((uint16 *)addr);
                        gprintk("addr %lx Enable DevControl MaxPayloadSize to 1 (256 bytes): "
                                "0x%x (%s w/ 0x%x)\n", addr, tmp161,
                                (tmp16 & MAX_PAYLOAD_256B) ? "Success":"Fail",
                                tmp16);
                        gprintk("Enable DevControl MaxReadRequestSize to 1 (256 bytes): "
                                "0x%x (%s w/ 0x%x)\n\n", tmp161,
                                (tmp16 & MAX_READ_REQ_256B) ? "Success":"Fail",
                                tmp16);
                    }
                }
                device_bmp &= ~(1 << i);
            }
            i++;
        }
    }

    /*
     * Configure max payload to 512 on all ports in the PLX8608/PLX8617.
     * The device supports 128, 512, and 1024 max payload sizes.
     */
    dev = NULL;
    while ((dev = PCI_FIND_DEV(PCI_VENDOR_ID_PLX, PCI_ANY_ID, dev)) != NULL) {
        if ((dev->device == PLX_PEX8608_DEV_ID) ||
            (dev->device == PLX_PEX8617_DEV_ID)) {
            uint16 ctrl_reg;
            pci_read_config_word(dev, PLX_PEX86XX_DEV_CTRL_REG, &ctrl_reg);
            ctrl_reg = (ctrl_reg & ~(7<<5)) | MAX_PAYLOAD_512B;
            pci_write_config_word(dev, PLX_PEX86XX_DEV_CTRL_REG, ctrl_reg);
        }
    }
    return 0;
}

#ifdef BCM_PLX9656_LOCAL_BUS

#define PLX_LAS0_BA         0x00000004  /* LAS0 Local Base Address Remap   */
#define PLX_LAS1_BA         0x000000f4  /* LAS1 Local Base Address Remap   */
#define PLX_LAS_EN          0x00000001  /* Space Enable bit                */

#define PLX_MMAP_PCIBAR0    0           /* Memory-Mapped Config (PCIBAR0)  */
#define PLX_LAS0_PCIBAR2    2           /* Local Address Space 0 (PCIBAR2) */
#define PLX_LAS1_PCIBAR3    3           /* Local Address Space 1 (PCIBAR3) */

STATIC int
_plx_las_bar_get(struct pci_dev *dev)
{
    void           *local_config_addr;
    int             bar = -1;

    local_config_addr = ioremap(pci_resource_start(dev, PLX_MMAP_PCIBAR0),
                                pci_resource_len(dev, PLX_MMAP_PCIBAR0));
    if (local_config_addr) {
        uint32          las_remap_reg;
        /*
         * Make sure LAS0BA or LAS1BA is enabled before returning
         * BAR that will be used to access the Local Bus
         */
        las_remap_reg = ioread32(local_config_addr + PLX_LAS0_BA);
        if (las_remap_reg & PLX_LAS_EN) {
            bar = PLX_LAS0_PCIBAR2;
        } else {
            las_remap_reg = ioread32(local_config_addr + PLX_LAS1_BA);
            if (las_remap_reg & PLX_LAS_EN) {
                bar = PLX_LAS1_PCIBAR3;
            }
        }
    }
    iounmap(local_config_addr);
    return bar;
}
#endif /* BCM_PLX9656_LOCAL_BUS */

static void
_shbde_log_func(int level, const char *str, int param)
{
    level = (level >= SHBDE_DBG) ? 1 : 0;
    if (debug >= level) {
        char *klvl = (level == 0) ? KERN_WARNING : "";
        printk("%s%s (%d): %s 0x%08x (%d)\n",
               klvl, LINUX_KERNEL_BDE_NAME, current->pid,
               str, param, param);
    }
}
/*
 * Function: _device_rescan_validate
 *
 * Purpose:
 *     Check if the device is ever probed or not.
 * Parameters:
 *    dev - Linux PCI device structure
 * Returns:
 *    >= 0 : dev is ever probed
 *           reutrn value is the index point to the _devices[]
 *    -1   : dev is not probed before.
 */
static int
_device_rescan_validate(struct pci_dev *dev)
{
    bde_ctrl_t *ctrl;
    int i;

    if (PCI_FUNC(dev->devfn) > 0) {
        return -1;
    }
    ctrl = NULL;
    for (i = 0; i < _ndevices; i ++) {
        ctrl = _devices + i;
        /* check the device id */
        if (dev->device == ctrl->bde_dev.device) {
            /* check the bus number */
            if ((dev->bus)) {
                if ((dev->bus->number == ctrl->bus_no) &&
                    (pci_domain_nr(dev->bus) == ctrl->domain_no)) {
                    return i;
                }
            }
        }
    }
    return -1;
}

#ifdef CONFIG_PCI_MSI

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,84))
/**
 * _pci_msix_table_size - return the number of device's MSI-X table entries
 * @dev: pointer to the pci_dev data structure of MSI-X device function
 */
static int
_pci_msix_table_size(struct pci_dev *dev)
{
    int  nr_entries = 0;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,110))
    u16 control;
    int pos;

    pos = pci_find_capability(dev, PCI_CAP_ID_MSIX);
    if (pos) {
        pci_read_config_word(dev, msi_control_reg(pos), &control);
        nr_entries = msix_table_size(control);
    }
#else
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,14,0))
{
    /* Pass large entry value to enable MSIX to get # of entires */
    struct msix_entry *entries;
    entries = kmalloc(sizeof(struct msix_entry) *
                       PCI_MSIX_FLAGS_QSIZE, GFP_KERNEL);
    if (entries != NULL) {
        nr_entries = pci_enable_msix(dev,
                                      entries, PCI_MSIX_FLAGS_QSIZE);
        pci_disable_msix(dev);
        kfree(entries);
    }
}
#else
    nr_entries = pci_msix_vec_count(dev);
#endif
    if (nr_entries < 0) {
        nr_entries = 0;
    }
#endif

    return nr_entries;
}
#endif

static int
_msi_connect(bde_ctrl_t *ctrl)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,84))
    int ret;
    if (ctrl->use_msi == PCI_USE_INT_MSIX) {
        int i;

        ret = _pci_msix_table_size(ctrl->pci_device);
        if (ret == 0) {
            /* MSI-X failed */
            gprintk("MSI-X not supported.\n");
            goto er_intx;
        }
        ctrl->entries = kcalloc(ret, sizeof(struct msix_entry), GFP_KERNEL);

        if (!ctrl->entries) {
            goto er_intx;
        }
        /* Only one vector is mapped by default */
        /* May be more in future, can be controlled using module param */
        ctrl->msix_cnt = msixcnt;
        if (unlikely(debug > 1))
            gprintk("MSIX Table size = %d\n", ctrl->msix_cnt);
        for (i = 0; i < ctrl->msix_cnt; i++)
                ctrl->entries[i].entry = i;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,12,0)
        ret = pci_enable_msix_range(ctrl->pci_device,
                                           ctrl->entries, ctrl->msix_cnt, ctrl->msix_cnt);
#else
        ret = pci_enable_msix(ctrl->pci_device,
                                           ctrl->entries, ctrl->msix_cnt);
        if (ret > 0) {
            /* Not enough vectors available , Retry MSI-X */
            gprintk("Retrying with MSI-X interrupts = %d\n", ret);
            ctrl->msix_cnt = ret;
            msixcnt = ret;
            ret = pci_enable_msix(ctrl->pci_device,
                                           ctrl->entries, ctrl->msix_cnt);
            if (ret != 0)
                goto er_intx_free;
        }
#endif
        if (ret < 0) {
            /* Error */
            goto er_intx_free;
        } else {
            gprintk("Enabled MSI-X interrupts = %d\n", ctrl->msix_cnt);
            return 0;
        }
    }
#endif

    if (ctrl->use_msi == PCI_USE_INT_MSI) {
        if (pci_enable_msi(ctrl->pci_device) == 0) {
            ctrl->iLine = ctrl->pci_device->irq;
        } else {
            /* MSI failed */
            gprintk("Failed to initialize MSI interrupts.\n");
            goto er_intx;
        }
    } else {
        /* failed */
        gprintk("Not MSI/MSIX interrupt.\n");
        goto er_intx;
    }
    return 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,84))
er_intx_free:
    gprintk("Failed to enable MSI-X interrupts = %d\n", ret);
    kfree(ctrl->entries);
#endif
er_intx:
    return -1;

}

static int
_msi_disconnect(bde_ctrl_t *ctrl)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,84))
    if (ctrl->use_msi == PCI_USE_INT_MSIX) {
        if (ctrl->msix_cnt) {
            pci_disable_msix(ctrl->pci_device);
            kfree(ctrl->entries);
            ctrl->entries = NULL;
            ctrl->msix_cnt = 0;
        }
    }
#endif
    if (ctrl->use_msi == PCI_USE_INT_MSI) {
        pci_disable_msi(ctrl->pci_device);
    } else {
        gprintk("MSI not used\n");
    }
    return 0;
}

#endif


static void
config_pci_intr_type(struct pci_dev *dev, bde_ctrl_t *ctrl, int iproc)
{
#ifdef CONFIG_PCI_MSI
    int ret;
    shbde_iproc_config_t icfg;

    /* Each device follows global policy by default */
    ctrl->use_msi = use_msi;

    if (unlikely(debug > 1))
        gprintk("%s: msi = %d\n", __func__, ctrl->use_msi);

    /* Check IPROC version for MSIX support */
    if (iproc && ctrl->bde_dev.base_address1 &&
                (ctrl->use_msi == PCI_USE_INT_MSIX)) {
        if (shbde_pci_iproc_version_get(&ctrl->shbde, dev, &icfg.iproc_ver,
                                         &icfg.cmic_ver, &icfg.cmic_rev)) {
            if (icfg.iproc_ver < 0xe) {
                gprintk("%s: MSI-X not supported, using MSI.\n", __func__);
                ctrl->use_msi = PCI_USE_INT_MSI;
            }
        }
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,84))
    if (ctrl->use_msi == PCI_USE_INT_MSIX) {
        /* check for support MSIX vector */
        ret = _pci_msix_table_size(ctrl->pci_device);
        if (ret == 0) {
            gprintk("%s: Zero MSIX table size, using MSI.\n", __func__);
            ctrl->use_msi = PCI_USE_INT_MSI;
        }
    }
#endif

    if (msi_check && (ctrl->use_msi == PCI_USE_INT_MSI)) {
        /* check for support MSI vector */
        ret = pci_enable_msi(ctrl->pci_device);
        if (ret < 0) {
            ctrl->use_msi = PCI_USE_INT_INTX;
            gprintk("%s: Failed to enable MSI, using INTx.\n", __func__);
        } else {
            pci_disable_msi(ctrl->pci_device);
        }
    }
#else
    ctrl->use_msi = PCI_USE_INT_INTX;
#endif
}

/*
 * Function: _pci_probe
 *
 * Purpose:
 *    Device initialization callback used by the Linux PCI
 *    subsystem. Called as a result of pci_register_driver().
 * Parameters:
 *    dev - Linux PCI device structure
 * Returns:
 *    0
 */
static int
_pci_probe(struct pci_dev *dev, const struct pci_device_id *ent)
{
    bde_ctrl_t *ctrl;
    resource_size_t paddr;
    uint16  cmd = 0;
    uint32 bar_len;
    int i, cmic_bar;
    int baroff = 0;
    int iproc = 0;
    int plx_dev = 0;
    int eth_dev = 0;
    int cpu_dev = 0;
    int update_devid = 0;
    int paxb_core = 0;
    int add_dev = 0, rescan = 0, rescan_idx = -1;
    shbde_hal_t shared_bde, *shbde = &shared_bde;

    if (debug >= 4) {gprintk("probing: vendor_id=0x%x, device_id=0x%x\n", dev->vendor, dev->device);}
    if (nodevices == 1) {
        return 0;
    }

    /* Initialize Linux hardware abstraction for shared BDE functions */
    linux_shbde_hal_init(shbde, _shbde_log_func);

    /*
     * If an AXI-based switch device has been found already, then it means
     * that the AXI bus has been probed, and that we should ignore devices
     * found on PCI bus zero.
     */

#if defined(IPROC_CMICD)
    if ((dev->bus) && (dev->bus->number == 0)) {
        int i;
        uint32 mask = BDE_SWITCH_DEV_TYPE | BDE_AXI_DEV_TYPE;

        for (i = 0; i < _ndevices; i++) {
            ctrl = _devices + i;

            if ((ctrl->dev_type & mask) == mask) {
                return 0;
            }
        }
    }
#endif /* IPROC_CMICD */

    /*
     * Note that a few supported devices have a non-Broadcom PCI vendor ID,
     * but since none of their associated PCI device IDs collide with the
     * Broadcom device IDs, the probe code below only checks the device ID.
     */

    switch (dev->device) {
    case PCI_DEVICE_ID_PLX_9056:
#ifdef DNX_TEST_BOARD
        break; /* a PCIe bridge to a non PCIe device should be treated as the device */
#endif /* DNX_TEST_BOARD */
    case PCI_DEVICE_ID_PLX_9656:
        plx_dev = 1;
        break;
    case BCM53000PCIE_DEVICE_ID:
        cpu_dev = 1;
        break;
        /*
         * The device ID for 56500, 56100 and 56300 family of devices may
         * be incorrect just after a PCI HW reset. It needs to be read
         * again after stabilization.
         */
    case BCM56102_DEVICE_ID:
    case BCM56504_DEVICE_ID:
    case BCM56304_DEVICE_ID:
    case BCM56314_DEVICE_ID:
    case BCM56112_DEVICE_ID:
        update_devid = 1;
        break;
    default:
        break;
    }

    /* Check if the device is ever probed. */
    rescan_idx = _device_rescan_validate(dev);
    if (rescan_idx != -1) {
        rescan = 1;
    }

    ctrl = NULL;
    if (plx_dev) {
#if defined(BCM_PLX9656_LOCAL_BUS)
        /* PLX chip itself won't be part of _devices[]. */
        baroff = _plx_las_bar_get(dev);
        if (baroff == -1) {
            gprintk("No Local Address Space enabled in PLX\n");
            return 0;
        }
        ctrl = &plx_ctrl;
        num_plx++;
#endif
    } else if (eth_dev) {
        if (_ether_ndevices >= LINUX_BDE_MAX_ETHER_DEVICES) {
            return 0;;
        }
        ctrl = _devices + _ndevices;
        ctrl->dev_type |= BDE_ETHER_DEV_TYPE;
        ctrl->iLine = dev->irq;
        add_dev = 1;
        if (debug >= 1)
            gprintk("Found PCI device %04x:%04x as Ethernet device\n",
                    dev->vendor, dev->device);
    } else if (cpu_dev) {
        if (_cpu_ndevices >= LINUX_BDE_MAX_CPU_DEVICES) {
            return 0;;
        }
        ctrl = _devices + _ndevices;
        ctrl->dev_type |= BDE_CPU_DEV_TYPE;
        add_dev = 1;
        if (debug >= 1)
            gprintk("Found PCI device %04x:%04x as CPU device\n",
                    dev->vendor, dev->device);
    } else {
        if (PCI_FUNC(dev->devfn) > 0) {
            return 0;
        }
        if (_switch_ndevices >= LINUX_BDE_MAX_SWITCH_DEVICES) {
            return 0;
        }

        if (rescan) {
            if (debug >= 1) {
                gprintk("PCI device %04x:%04x is re-probed \n",
                        dev->vendor, dev->device);
            }
            ctrl = _devices + rescan_idx;
            ctrl->dev_state = BDE_DEV_STATE_CHANGED;
        } else {
            ctrl = _devices + _ndevices;
            ctrl->dev_type |= BDE_SWITCH_DEV_TYPE;
            ctrl->domain_no = pci_domain_nr(dev->bus);
            ctrl->bus_no = dev->bus->number;
            ctrl->dev_state = BDE_DEV_STATE_NORMAL;
            add_dev = 1;
        }

        /* Save shared BDE HAL in device structure */
        MEMCPY(&ctrl->shbde, shbde, sizeof(ctrl->shbde));

        if (update_devid) {
            /* Re-read the device ID */
            pci_read_config_word(dev,
                                 PCI_DEVICE_ID,
                                 &ctrl->bde_dev.device);
            dev->device = ctrl->bde_dev.device;
        }

        if (debug >= 4) {gprintk("Enabling PCI device : vendor_id=0x%x, device_id=0x%x\n", dev->vendor, dev->device);}
        if (pci_enable_device(dev)) {
            gprintk("Cannot enable PCI device : vendor_id = %x, device_id = %x\n",
                    dev->vendor, dev->device);
        }


        /*
         * These are workarounds to get around some existing
         * kernel problems :(
         */

        /*
         * While probing we determine the overall limits for the PCI
         * memory windows across all devices. These limits are used
         * later on by the PCI-PCI bridge  code.
         */
        if (pci_resource_start(dev, baroff) < _pci_mem_start) {
            _pci_mem_start = pci_resource_start(dev, baroff);
        }
        if (pci_resource_end(dev, baroff) > _pci_mem_end) {
            _pci_mem_end = pci_resource_end(dev, baroff);
        }

#ifdef CONFIG_SANDPOINT
        /*
         * Something wrong with the PCI subsystem in the mousse kernel.
         * The device is programmed correctly, but the irq in the pci
         * structure is hosed. This checks for the hosed-ness and fixes it.
         */
        if (dev->irq > 100) {
            dev->irq = 2;
            gprintk("irq problem: setting irq = %d\n", dev->irq);
        }
#endif

#ifdef CONFIG_BMW
        /*
         * PCI subsystem does not always program the system correctly.
         */
        if (dev->irq < 16 && dev->irq != 2) {
            dev->irq = 2;
            gprintk("irq problem: setting irq = %d\n", dev->irq);
        }
#endif

#ifdef CONFIG_IDT_79EB334
        /*
         * IDT kernel is not currently mapping interrupts correctly
         * Hardwired to core mips interrupt, irq 3 for kernel 2.4.18
         */
        if (dev->irq != 3) {
            dev->irq = 3;
            gprintk("irq problem: setting irq = %d\n", dev->irq);
        }
#endif

#ifdef CONFIG_BCM94702_CPCI
        /*
         * Hardwired to core mips interrupt irq 6 on MBZ
         */
        if (dev->irq != 6) {
            dev->irq = 6;
            gprintk("irq problem: setting irq = %d\n", dev->irq);
        }
#endif

        if ((PCI_FIND_DEV(BCM4704_VENDOR_ID, BCM4704_DEVICE_ID, NULL)) != NULL) {
            /*
             * Decrease the PCI bus priority for the CPU for better overall
             * system performance. This change significantly reduces the
             * number of PCI retries from other devices on the PCI bus.
             */
            void * _mc_vbase = ioremap(BCM4704_MEMC_BASE, 0x1000);
            int priorinv = 0x80;
            static int done = 0;
            if (!done) {
                done = 1;
                writel(priorinv, _mc_vbase + BCM4704_MEMC_PRIORINV);
                if (debug >= 1)
                    gprintk("set BCM4704 PriorInvTim register to 0x%x\n", priorinv);
                iounmap(_mc_vbase);
            }
        }

        if ((PCI_FIND_DEV(SIBYTE_PCI_VENDOR_ID, SIBYTE_PCI_DEVICE_ID, NULL)) != NULL) {
            /*
             * The BCM91125CPCI CPU boards with a PCI-PCI bridge use the same
             * interrupt line for all switch ships behind the bridge.
             */
            if (PCI_FIND_DEV(DC21150_VENDOR_ID, DC21150_DEVICE_ID, NULL) ||
                PCI_FIND_DEV(HINT_HB4_VENDOR_ID, HINT_HB4_DEVICE_ID, NULL) ||
                PCI_FIND_DEV(PI7C8150_VENDOR_ID, PI7C8150_DEVICE_ID, NULL)) {
                /*
                 * By default we try to guess the correct IRQ based on the design.
                 * For now we only look at the bridge vendor, but it may be necessary
                 * to look at the switch chip configuration as well.
                 */
                if (forceirq == -1) {
                    if ((PCI_FIND_DEV(HINT_HB4_VENDOR_ID, HINT_HB4_DEVICE_ID, NULL)) ||
                        ((dev->device == BCM5674_DEVICE_ID) &&
                         (PCI_FIND_DEV(PI7C8150_VENDOR_ID, PI7C8150_DEVICE_ID, NULL)))) {
                        forceirq = 58;
                    } else {
                        forceirq = 56;
                    }
                }
            }
        }

        if (((PCI_FIND_DEV(BCM58525_PCI_VENDOR_ID, BCM58525_PCI_DEVICE_ID, NULL)) != NULL) ||
            ((PCI_FIND_DEV(BCM58525_PCI_VENDOR_ID, BCM58522_PCI_DEVICE_ID, NULL)) != NULL) ||
            ((PCI_FIND_DEV(BCM58712_PCI_VENDOR_ID, BCM58712_PCI_DEVICE_ID, NULL)) != NULL) ) {
            /* BCM58525/BCM58712 CPU boards support 128 Max payload size */
            if (maxpayload && maxpayload != 128) {
                maxpayload = 128;
                if (debug >= 1) gprintk("force max payload size to 128\n");
            }
        }

        if (forceirq > 0 && dev->irq != (uint32) forceirq) {
            if (forceirqubm & (1U << (_ndevices - 1))) {
                dev->irq = forceirq;
                if (debug >= 1) gprintk("force irq to %d\n", forceirq);
            }
        } else if (debug >= 1) gprintk("found irq %d\n", dev->irq);

        ctrl->iLine = dev->irq;
        if (unlikely(debug > 1))
            gprintk("%s:irq = %d\n",__func__, ctrl->iLine);

        if (shbde_pci_is_pcie(shbde, dev)) {
            /* Set PCIe max payload */
            shbde_pci_max_payload_set(shbde, dev, maxpayload);
        } else {
            /* Set PCI retry to infinite on non-PCIe switch device */
            pci_write_config_word(dev, 0x40, 0x0080);
            if (debug >= 1) gprintk("set DMA retry to infinite on switch device\n");
        }
    }

#if defined(BCM_DFE_SUPPORT)
    switch (dev->device) {
    case BCM88770_DEVICE_ID:
    case BCM88773_DEVICE_ID:
    case BCM88774_DEVICE_ID:
    case BCM88775_DEVICE_ID:
    case BCM88776_DEVICE_ID:
    case BCM88777_DEVICE_ID:
    case BCM88950_DEVICE_ID:
    case BCM88953_DEVICE_ID:
    case BCM88954_DEVICE_ID:
    case BCM88955_DEVICE_ID:
    case BCM88956_DEVICE_ID:
    case BCM88772_DEVICE_ID:
    case BCM88952_DEVICE_ID:

        /*
         * For DMA transactions - set Max_Payload_Size and
         * Max_Read_Request_Size to 128 bytes.
         */
        pci_write_config_byte(dev, 0xb5, 0x0c);
        pci_write_config_byte(dev, 0xb4, 0x0);
        break;
    default:
        break;
    }
#endif /* BCM_DFE_SUPPORT */

    /* Prevent compiler warning */
    if (ctrl == NULL) {
        return 0;
    }

    ctrl->be_pio = 0;
    ctrl->dev_type |= BDE_PCI_DEV_TYPE;
    ctrl->pci_device = dev;
    pci_set_drvdata(dev, ctrl);

    /*
     * Sample setting of unique ID, used the PCIe address of the device:
     * domain, bus, slot, function in hex digits: DDDDBBSS (SS includes the slot/device and function.
     * Tested with old kernels from 2.6 .
     * Do not use the PCI_DEVID macro which old kernel versions don't have. */
    ctrl->bde_dev.dev_unique_id = dev->bus ?
      (((uint32)pci_domain_nr(dev->bus)) << 16) ^ (((uint32)dev->bus->number) << 8) ^ dev->devfn :
      dev->devfn;

    /* Check for iProc device */
    if (shbde_pci_is_iproc(shbde, dev, &cmic_bar)) {
        iproc = 1;
        if (cmic_bar >= 0) {
            baroff = cmic_bar;
        }
    }
#ifdef DNX_TEST_BOARD
    else if (dev->device == PLX9056_DEVICE_ID && baroff == 0) {
        baroff = 2;
        ctrl->dev_type |= BDE_NO_IPROC/* | BDE_BYTE_SWAP*/;
    }
#endif /* DNX_TEST_BOARD */

    /* Get the device revision */
    pci_read_config_byte(dev, PCI_REVISION_ID, &ctrl->bde_dev.rev);

    /* Map in the device */
    ctrl->bde_dev.device = dev->device;
    paddr = pci_resource_start(dev, baroff);

    bar_len = pci_resource_len(dev, baroff);

    ctrl->bde_dev.base_address = (sal_vaddr_t)ioremap(paddr, bar_len);
    ctrl->iowin[0].addr = paddr;
    ctrl->iowin[0].size = bar_len;

    if (debug >= 3) {
        gprintk("BAR %d: kernel addr:0x%lx phys addr:0x%lx length:%lx\n",
          baroff, (unsigned long)ctrl->bde_dev.base_address, (unsigned long)paddr, (unsigned long)bar_len);
    }

    /* Map secondary address spaces */
    for (i = 1; i < BDE_NUM_IOWIN_MAX; i++) {
        ctrl->iowin[i].addr = 0;
        ctrl->iowin[i].size = 0;
    }
    ctrl->bde_dev.base_address1 = 0;

    if (iproc
#ifdef DNX_TEST_BOARD
        || (dev->device == PLX9056_DEVICE_ID && baroff == 2)
#endif /* DNX_TEST_BOARD */
        ) {
        paddr = pci_resource_start(dev, 0);
        bar_len = pci_resource_len(dev, 0);
        ctrl->bde_dev.base_address1 = (sal_vaddr_t)ioremap(paddr, bar_len);
        ctrl->iowin[1].addr = paddr;
        ctrl->iowin[1].size = PAGE_ALIGN(bar_len);
        if (debug >= 3) {
            gprintk("BAR 0: kernel addr:0x%lx phys addr:0x%lx length:%lx\n",
              (unsigned long)ctrl->bde_dev.base_address1, (unsigned long)paddr, (unsigned long)bar_len);
        }
    }

    /* configure interrupt type */
    config_pci_intr_type(dev, ctrl, iproc);

    /* Configure iProc PCI-AXI bridge */
    if (iproc && ctrl->bde_dev.base_address1) {
        void *iproc_regs;
        shbde_iproc_config_t *icfg = &shbde->icfg;

        /* Mapped iProc regs in PCI BAR 0 */
        iproc_regs = (void *)ctrl->bde_dev.base_address1;

        /* iProc configuration parameters */
        (void)shbde_pci_iproc_version_get(shbde, dev, &icfg->iproc_ver,
                                          &icfg->cmic_ver, &icfg->cmic_rev);

#ifdef BCM_Q3A_SUPPORT
        if (((ctrl->bde_dev.device & DNXC_DEVID_FAMILY_MASK) == Q3U_ORIG_DEVICE_ID) && (icfg->iproc_ver == 20)) {
            /* Workaround for conflicting between Q3u,n devices and Greyhound */
            ctrl->bde_dev.device |= Q3U_DEVICE_ID ^ Q3U_ORIG_DEVICE_ID; /* 0x840x -> 0x84ax */

            if (debug >= 2) {
                gprintk("new dev ID = 0x%x\n", ctrl->bde_dev.device);
            }
        }
#endif
        shbde_iproc_config_init(icfg, ctrl->bde_dev.device, ctrl->bde_dev.rev);

        if (debug >= 2) {
            gprintk("iproc version = 0x%x dma_hi_bits = 0x%x\n",
                    icfg->iproc_ver, icfg->dma_hi_bits);
        }

        /*
         * Do not allow iProc sub-window remapping on the fly as this may
         * corrupt iProc access from user space.
         */
        icfg->no_subwin_remap = 1;

        icfg->use_msi = ctrl->use_msi;

        /* Call shared function */
        paxb_core = shbde_iproc_paxb_init(shbde, iproc_regs, icfg);

        /* Save PCI core information for CMIC */
        if (paxb_core == 1) {
            ctrl->dev_type |= BDE_DEV_BUS_ALT;
        }

        /* Save MSI enable state information */
        if (ctrl->use_msi) {
            ctrl->dev_type |= BDE_DEV_BUS_MSI;
        }

        /* iProc PCIe preemphasis */
        shbde_iproc_pcie_preemphasis_set(shbde, iproc_regs, icfg, dev);
    }

    /* Save shared BDE HAL in device structure */
    MEMCPY(&ctrl->shbde, shbde, sizeof(ctrl->shbde));


    ctrl->isr = NULL;
    ctrl->isr_data = NULL;

    pci_read_config_word(dev, PCI_COMMAND, &cmd);
    if (!(cmd & PCI_COMMAND_MEMORY) || !(cmd & PCI_COMMAND_MASTER)) {
        cmd |= PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
        pci_write_config_word(dev, PCI_COMMAND, cmd);
        if (debug >= 1) gprintk("enable PCI resources 0x%x (PCI_COMMAND)\n", cmd);
    }

    /* Check if we need 256 KB memory window (default is 64 KB) */
    bar_len = pci_resource_len(dev, baroff);
    if (bar_len == 0x40000) {
        ctrl->dev_type |= BDE_256K_REG_SPACE;
        if (debug >= 1) gprintk("PCI resource len 256K\n");
    } else if (bar_len == 0x800000) {
        ctrl->dev_type |= BDE_256K_REG_SPACE | BDE_8MB_REG_SPACE;
        if (debug >= 1) gprintk("PCI resource len 8MB\n");
    }

#ifdef LINUX_BDE_DMA_DEVICE_SUPPORT
    ctrl->dma_dev = &dev->dev;
#endif

    if (debug >= 2) {
        gprintk("_pci_probe: configured dev:0x%x rev:0x%x with base_addresses: 0x%lx 0x%lx\n",
          (unsigned)ctrl->bde_dev.device, (unsigned)ctrl->bde_dev.rev,
          (unsigned long)ctrl->bde_dev.base_address, (unsigned long)ctrl->bde_dev.base_address1);
    }

#ifdef INCLUDE_SRAM_DMA
        /* Check if the device should use SRAM for DMA, if so configure the SRAM to be used */
        if (use_sram_for_dma) {
#ifdef BCM_DNXF3_SUPPORT
            switch (dev->device & DNXC_DEVID_FAMILY_MASK) {
              case RAMON2_DEVICE_ID: /* Mark the device as user type if SRAM mode RM2/3 */
              case RAMON3_DEVICE_ID:
                /* Check if we can use RTS using ICFG_RTS_STRAPS, has to be done after PCIe is enabled. */
                if ((shbde_iproc_pci_read(&ctrl->shbde, (void *)ctrl->bde_dev.base_address1, 0x2920034) & 2) != 0) {
                    ctrl->dev_sram_dma_start = 0x38100000;
                    ctrl->dev_sram_dma_size = 0x400000; /* 4MB */
                } else { /* Use free M0SSQ SRAM */
                    ctrl->dev_sram_dma_start = 0x2070000;
                    ctrl->dev_sram_dma_size = 0x10000; /* 64KB */
                }
                ctrl->dev_type |= BDE_USER_DEV_TYPE; /* Mark as user defined access for BDE handling access to BARs */
                if (debug >= 4) {
                    gprintk("PCI device 0x%x:0x%x using SRAM DMA at 0x%x size 0x%x dev_type=0x%x dev=%u\n", dev->vendor, dev->device,
                            ctrl->dev_sram_dma_start, ctrl->dev_sram_dma_size, ctrl->dev_type, (unsigned)(ctrl - _devices));
                }
                /** Device no PCIe connect, the BARs must be accessed by the BDE, permit sub window remapping */
                ctrl->shbde.icfg.no_subwin_remap = 0;
                    break;
                default:
                    break;
            }
#endif /* BCM_DNXF3_SUPPORT */
        }
#endif /* INCLUDE_SRAM_DMA */

    if (rescan /* map IOMMU for re-probed devices */
#ifdef INCLUDE_SRAM_DMA
        && (!use_sram_for_dma || (_devices[rescan_idx].dev_type & BDE_USER_DEV_TYPE) == 0)
#endif /* INCLUDE_SRAM_DMA */
       ) {
        _dma_per_device_init(rescan_idx);
    }

    if (add_dev) {
        _bde_add_device();
    }
    /* Let's boogie */
    return 0;
}

/*
 * Function: _pci_remove
 *
 * Purpose:
 *    Detach driver from device. Called from pci_unregister_driver().
 * Parameters:
 *    dev - Linux PCI device structure
 * Returns:
 *    0
 */
static void
_pci_remove(struct pci_dev* dev)
{
    bde_ctrl_t *ctrl;

    if (nodevices == 1) {
        return;
    }

#if defined(BCM_DFE_SUPPORT)
    if (dev->device == PCI_DEVICE_ID_PLX_9056) {
        return;
    }
#endif

    ctrl = (bde_ctrl_t *) pci_get_drvdata(dev);

    if (ctrl == NULL) {
        /* Unused device */
        return;
    }
    ctrl->dev_state = BDE_DEV_STATE_REMOVED;
    if (debug >= 1) {
        gprintk("PCI device %04x:%04x is removed. \n",
                dev->vendor, dev->device);
    }

    /* Free our interrupt handler, if we have one */
    if (ctrl->isr || ctrl->isr2) {
#ifdef CONFIG_PCI_MSI
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,84))
        if (ctrl->use_msi >= PCI_USE_INT_MSIX) {
            int i;
            for (i = 0; i < ctrl->msix_cnt; i++)
                free_irq(ctrl->entries[i].vector, ctrl);
        }
        else
#endif
#endif
        {
            free_irq(ctrl->iLine, ctrl);
        }
    }
#ifdef CONFIG_PCI_MSI
    if (ctrl->intr_pending && ctrl->use_msi >= PCI_USE_INT_MSI && ctrl->timer_active) {
        ctrl->timer_active = 0;
        del_timer_sync(&ctrl->isr_tick);
    }

    _msi_disconnect(ctrl);
#endif


    ctrl->isr = NULL;
    ctrl->isr_data = NULL;
    ctrl->isr2 = NULL;
    ctrl->isr2_data = NULL;

    if (ctrl->bde_dev.base_address1) {
        iounmap((void *)ctrl->bde_dev.base_address1);
    }
    if (ctrl->bde_dev.base_address) {
        iounmap((void *)ctrl->bde_dev.base_address);
    }
}

static struct pci_driver _device_driver = {
    .name = LINUX_KERNEL_BDE_NAME,
    .probe = _pci_probe,
    .remove = _pci_remove,
    .id_table = _id_table,
    /* The rest are dynamic */
};
#endif /* CONFIG_PCI */

static void
_spi_device_setup(void) {
    bde_ctrl_t *ctrl;
    ctrl = _devices + _ndevices;
    ctrl->dev_type |= BDE_SWITCH_DEV_TYPE;
    ctrl->iLine = 0xa3;
    ctrl->be_pio = 0;
    ctrl->dev_type |= BDE_SPI_DEV_TYPE;
    ctrl->bde_dev.device = spi_devid;
    ctrl->bde_dev.rev = spi_revid;
    if (debug >= 1) {
        gprintk("SPI Slave Mode: force ctrl->bde_dev.device=0x%x\n",ctrl->bde_dev.device);
        gprintk("SPI Slave Mode: force ctrl->bde_dev.rev=0x%x\n",ctrl->bde_dev.rev);
        gprintk("SPI Slave Mode: force ctrl->dev_type=0x%x\n",ctrl->dev_type);
    }
    _bde_add_device();
}
#endif /* BCM_ICS */




#if defined(BCM_METROCORE_LOCAL_BUS)
static bde_ctrl_t*
map_local_bus(uint64_t addr, uint32_t size)
{
    bde_ctrl_t *ctrl;

    ctrl = _devices + _ndevices;

    /*
     * For now: use EB type as `local bus'
     * (memory mapped, no DMA, no interrupts)
     * metrocore local bus supports interrupts, but we don't use them.
     */
    ctrl->dev_type |= BDE_EB_DEV_TYPE | BDE_SWITCH_DEV_TYPE
        | (size > 64 * 1024 ? BDE_128K_REG_SPACE : 0);
    ctrl->pci_device = NULL; /* No PCI bus */

    /* Map in the device */
    ctrl->bde_dev.base_address = (sal_vaddr_t)ioremap(addr, size);
    ctrl->iowin[0].addr = addr;
    ctrl->iowin[0].size = size;

    _bde_add_device();
    return(ctrl);
}

#define BME_REVISION_OFFSET     (0x0)

#endif

#ifdef BCM_PLX9656_LOCAL_BUS

#define DEV_REG_BASE_OFFSET     PL0_OFFSET /* Polaris register base */
#define DEV_REG_DEVID           0          /* Device ID is first register */

/*
 * The difference at map_local_bus2:
 *
 * The PLX9656 PCI-to-LOCAL bridge chip already has been iomapped the
 * whole address space.  So the devices off local bus don't need to be
 * mapped again.  They only need to claim their own sub-space.
 */
static bde_ctrl_t *
map_local_bus2(bde_ctrl_t *plx_ctrl, uint32_t dev_base, uint32_t size)
{
    uint32_t dev_rev_id;
    uint8_t *addr;
    bde_ctrl_t *ctrl;

    ctrl = _devices + _ndevices;

    /*
     * For now: use EB type as `local bus'
     * (memory mapped, no DMA, no interrupts)
     * metrocore local bus supports interrupts, but we don't use them.
     */
    ctrl->dev_type |= BDE_EB_DEV_TYPE |  BDE_SWITCH_DEV_TYPE;
    ctrl->dev_type |= BDE_320K_REG_SPACE; /* Polaris 18 bits address + FPGA */
    ctrl->pci_device = NULL; /* No PCI bus */

    /* Map in the device */
    ctrl->bde_dev.base_address = plx_ctrl->bde_dev.base_address + dev_base;
    ctrl->iowin[0].addr = plx_ctrl->iowin[0].addr + (resource_size_t)dev_base;
    ctrl->iowin[0].size = size;

    addr = (uint8_t *)ctrl->bde_dev.base_address + PL0_REVISION_REG;
    dev_rev_id = readl(addr);
    ctrl->bde_dev.device = dev_rev_id >> 16;
    ctrl->bde_dev.rev = (dev_rev_id & 0xFF);


    _bde_add_device();

    switch (ctrl->bde_dev.device) {
    default:
        return 0;
    }
    return(ctrl);
}

static int
probe_plx_local_bus(void)
{
    bde_ctrl_t *ctrl;
    uint32_t val;
    uint8_t *addr;
    char addr_hi_str[16];

    if (num_plx > 1) {
            printk(KERN_ERR "There's more than one PLX 9656/9056 chip\n");
        return -1;
    }
    addr_hi_str[0] = 0;
#ifdef PHYS_ADDR_IS_64BIT
    sprintf(addr_hi_str, "%08x", (uint32_t)(plx_ctrl.iowin[0].addr >> 32));
#endif
    printk(KERN_ERR "Found PLX %04x:%04x vir: 0x%08x phy: 0x%s%08x\n",
           plx_ctrl.bde_dev.device, plx_ctrl.bde_dev.rev,
           plx_ctrl.bde_dev.base_address, addr_hi_str,
           (uint32_t)(plx_ctrl.iowin[0].addr));

    addr = (uint8_t *)plx_ctrl.bde_dev.base_address + CPLD_OFFSET + CPLD_REVISION_REG;
    val = readl(addr);
    printk(KERN_ERR "plx: CPLD revision %d\n", val & CPLD_REVISION_MASK);
#if 000
    addr = (uint8_t *)plx_ctrl.bde_dev.base_address + CPLD_OFFSET + CPLD_RESET_REG;
    writel(CPLD_RESET_NONE, addr);
#endif
    ctrl = map_local_bus2(&plx_ctrl, PL0_OFFSET, PL0_SIZE);
    if (ctrl == 0)
        return -1;

    /* Uses PLX IRQ for Polaris LC */
    ctrl->iLine = 48;
    ctrl->isr = NULL;
    ctrl->isr_data = NULL;

    return 0;
}

#endif /* BCM_PLX9656_LOCAL_BUS */





/*
 * Generic module functions
 */

/*
 * Function: _init
 *
 * Purpose:
 *    Module initialization.
 *    Attaches to kernel BDE.
 * Parameters:
 *    None
 * Returns:
 *    0 on success, < 0 on error.
 */
static int
_init(void)
{
    unsigned i;

    /* allocate and init the DMA buffer pool */
    _dma_init();
#ifdef IPROC_CMICD
#ifdef CONFIG_PCI
    /*
     * Adjust the PCI driver name to prevent our device file from
     * getting removed when the module is unloaded.
     */
    _device_driver.name = LINUX_KERNEL_BDE_NAME ".iproc";
#endif
#ifdef CONFIG_OF
    if (of_find_compatible_node(NULL, NULL, IPROC_CMICX_COMPATIBLE)) {
        iproc_platform_driver_register(&iproc_cmicd_driver);
    } else
#endif
    if (iproc_has_cmicd()) {
        iproc_cmicd_get_memregion(&iproc_cmicd_resources[IPROC_CMICD_RES_MEM]);
        /* PCIe device will be added here */
        iproc_platform_driver_register(&iproc_cmicd_driver);
#ifdef CONFIG_OF
        if (!of_find_compatible_node(NULL, NULL, IPROC_CMICD_COMPATIBLE))
#endif
        {
            /* Register platform device if no device node in dtb */
            iproc_platform_device_register(&iproc_cmicd_pdev);
        }
    }
#endif /* IPROC_CMICD */

#ifdef BCM_ICS
    _ics_bde_create();
#else /* PCI */

#ifdef CONFIG_PCI
    /* Configure MSI interrupt support */
    use_msi = usemsi;

#ifdef CONFIG_PCI_MSI
    if (use_msi == PCI_USE_INT_NONE) {
        /* Compilation flag determines default value */
#ifdef BDE_LINUX_USE_MSIX_INTERRUPT
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,84))
    use_msi = PCI_USE_INT_MSIX;
#else
    use_msi = PCI_USE_INT_MSI;
#endif
#elif defined(BDE_LINUX_USE_MSI_INTERRUPT)
      use_msi = PCI_USE_INT_MSI;
#else
      use_msi = PCI_USE_INT_INTX;
#endif
    }
#else
    if (use_msi > PCI_USE_INT_INTX) {
        /* Warn if invalid configuration */
        gprintk("MSI interrupts not supported by kernel\n");
    }
    use_msi = PCI_USE_INT_INTX;
#endif /* CONFIG_PCI_MSI */

    if (unlikely(debug >= 1))
        gprintk("%s(%d):use_msi = %d\n", __func__, __LINE__, use_msi);
#endif /* CONFIG_PCI */

    if (spi_devid) {
        _spi_device_setup();
    } else {
#ifdef CONFIG_PCI
        /* PCIe devices will be probed here */
        if (pci_register_driver(&_device_driver) < 0) {
            return -ENODEV;
        }
#endif
    }

#ifdef CONFIG_PCI
    /* Note: PCI-PCI bridge uses results from pci_register_driver */
    if (pci_bridge_setup) {
        p2p_bridge();
    }
#endif

#ifdef BCM_METROCORE_LOCAL_BUS
    if (probe_metrocore_local_bus()) {
        return -1;
    }
#endif
#ifdef BCM_PLX9656_LOCAL_BUS
    if (num_plx > 0) {
        probe_plx_local_bus();
    }
#endif
#endif /* BCM_ICS */

    /*
     * Probe for EB Bus devices.
     */
    if (eb_bus) {
        char  *tok;
        uint   irq = -1, eb_rd16bit=0, eb_wr16bit =0;
        unsigned int eb_ba = 0x0;

        gprintk("EB bus info: %s\n", eb_bus);
        tok = strtok(eb_bus,",");
        while (tok) {
            _parse_eb_args(tok, "BA=%x IRQ=%d RD16=%d WR16=%d",
                    &eb_ba, &irq, &eb_rd16bit, &eb_wr16bit);
            _eb_device_create(eb_ba, irq, eb_rd16bit, eb_wr16bit);
            tok = strtok(NULL,",");
        }
    }


    for (i = 0; i < LINUX_BDE_MAX_DEVICES; ++i) {
        _devices[i].inst_id = BDE_DEV_INST_ID_INVALID;
        _devices[i].timer_active = 0;
        _devices[i].intr_pending = NULL;
    }

    return 0;
}

/*
 * Function: _cleanup
 *
 * Purpose:
 *    Module cleanup function.
 * Parameters:
 *    None
 * Returns:
 *    Always 0
 */
static int
_cleanup(void)
{
    int i;

    _dma_cleanup();

#ifdef IPROC_CMICD
#ifdef CONFIG_OF
    if (of_find_compatible_node(NULL, NULL, IPROC_CMICX_COMPATIBLE)) {
        iproc_platform_driver_unregister(&iproc_cmicd_driver);
    } else
#endif
    if (iproc_has_cmicd()) {
#ifdef CONFIG_OF
        if (!of_find_compatible_node(NULL, NULL, IPROC_CMICD_COMPATIBLE))
#endif
        {
            iproc_platform_device_unregister(&iproc_cmicd_pdev);
        }
        iproc_platform_driver_unregister(&iproc_cmicd_driver);
    }
#endif

    for (i = 0; i < _ndevices; i++) {
        bde_ctrl_t *ctrl = _devices + i;

        /* free allocated kernel space memory */
        if (ctrl->dev_type & BDE_SPI_DEV_TYPE) {
            if (ctrl->spi_device) {
                kfree(ctrl->spi_device);
            }
        }
    }
#if defined(BCM_SAND_SUPPORT) && (defined(__DUNE_WRX_BCM_CPU__) || defined(__DUNE_GTO_BCM_CPU__))
    if (cpu_address) { /* unmap CPU card MMIO */
        iounmap(cpu_address);
        cpu_address = NULL;
    }
#endif

#ifdef BCM_ICS
#else
#ifdef CONFIG_PCI
    pci_unregister_driver(&_device_driver);
#endif
#endif /* BCM_ICS */
    return 0;
}

/*
 * Function: _pprint
 *
 * Purpose:
 *    Print proc filesystem information.
 * Parameters:
 *    None
 * Returns:
 *    Always 0
 */
static int
_pprint(struct seq_file *m)
{
    int i = 0;

    pprintf(m, "Broadcom Device Enumerator (%s)\n", LINUX_KERNEL_BDE_NAME);

#ifdef LKM_BUILD_INFO
    pprintf(m, "%s\n", LKM_BUILD_INFO);
#endif

    pprintf(m, "Module parameters:\n");
    pprintf(m, "\tmaxpayload=%d\n", maxpayload);
    pprintf(m, "\tusemsi=%d\n", usemsi);
    pprintf(m, "\tisrtickms=%d\n", isrtickms);
    pprintf(m, "\tpci_bridge_setup=%d\n", pci_bridge_setup);

    _dma_pprint(m);

    if (_ndevices == 0) {
        pprintf(m, "No devices found\n");
    } else {
        pprintf(m, "Devices:\n");
    }
    for (i = 0; i < _ndevices; i++) {
        bde_ctrl_t *ctrl = _devices + i;

        if (ctrl->dev_type & BDE_SWITCH_DEV_TYPE) {
            pprintf(m, "\t%d (swi) : ", i);
        } else if (ctrl->dev_type & BDE_ETHER_DEV_TYPE) {
            pprintf(m, "\t%d (eth) : ", i);
        } else if (ctrl->dev_type & BDE_CPU_DEV_TYPE) {
            pprintf(m, "\t%d (cpu) : ", i);
        } else {
            pprintf(m, "\t%d (?)   : ", i);
        }


        if (ctrl->dev_state == BDE_DEV_STATE_REMOVED) {
            pprintf(m, "PCI device 0x%x:0x%x:%d   REMOVED\n",
                    ctrl->pci_device->vendor,
                    ctrl->pci_device->device,
                    ctrl->bde_dev.rev);
            continue;
        }
        if (ctrl->dev_type & BDE_PCI_DEV_TYPE) {
            pprintf(m, "PCI device %02x:%02x.%x 0x%x:0x%x:%d:0x%.8lx:0x%.8lx:%d%s\n",
                    (unsigned int)ctrl->pci_device->bus->number,
                    PCI_SLOT(ctrl->pci_device->devfn),
                    PCI_FUNC(ctrl->pci_device->devfn),
                    ctrl->pci_device->vendor,
                    ctrl->pci_device->device,
                    ctrl->bde_dev.rev,
                    (unsigned long)pci_resource_start(ctrl->pci_device, 0),
                    (unsigned long)pci_resource_start(ctrl->pci_device, 2),
                    ctrl->pci_device->irq,
                    ctrl->use_msi ? " (MSI)" : "");
        } else if (ctrl->dev_type & BDE_SPI_DEV_TYPE) {
            pprintf(m, "SPI Device %d:%x:%x:0x%x:0x%x:%d\n",
                    ctrl->spi_device->cid,
                    ctrl->spi_device->part,
                    ctrl->spi_device->rev,
                    ctrl->spi_device->phyid_high,
                    ctrl->spi_device->phyid_low,
                    ctrl->bde_dev.rev);
        } else if (ctrl->dev_type & BDE_ICS_DEV_TYPE) {
            pprintf(m, "ICS Device 0x%x:0x%x\n",
                    ctrl->bde_dev.device,
                    ctrl->bde_dev.rev);
        } else if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            pprintf(m, "AXI Device 0x%x:0x%x:0x%.8lx:%d\n",
                    ctrl->bde_dev.device,
                    ctrl->bde_dev.rev,
                    (unsigned long)ctrl->iowin[0].addr,
                    ctrl->iLine);
        } else if (ctrl->dev_type & BDE_EB_DEV_TYPE) {
            pprintf(m, "EB Bus Device 0x%x:0x%x\n",
                    ctrl->bde_dev.device,
                    ctrl->bde_dev.rev);
        }
        if (debug >= 1) {
            pprintf(m, "\t\timask:imask2:fmask 0x%x:0x%x:0x%x  unique_id=0x%x inst_id ",
                    ctrl->imask,
                    ctrl->imask2,
                    ctrl->fmask,
                    ctrl->bde_dev.dev_unique_id);
            if (ctrl->inst_id == BDE_DEV_INST_ID_INVALID) {
                pprintf(m, "INVALID\n");
            } else {
                pprintf(m, "%u%s\n",
                        ctrl->inst_id,
                        ctrl->inst_id < LINUX_BDE_MAX_DEVICES ? "":"(Illegal)");
            }
        }
        pprintf(m, "\t\tinterrupts:prev:stuck %d:%d:%d\n",
                    ctrl->interrupts,
                    ctrl->prev_interrupts,
                    ctrl->stuck_interrupts);
    }
    return 0;
}

/*
 * Some kernels are configured to prevent mapping of kernel RAM memory
 * into user space via the /dev/mem device.
 *
 * The function below provides a backdoor to map IO and DMA memory to
 * user space via the BDE device file.
 */
static
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32))
const
#endif
struct vm_operations_struct bde_vma_ops = {
#ifdef CONFIG_HAVE_IOREMAP_PROT
	.access = generic_access_phys,
#endif
};

static int
_bde_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long paddr = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long size = vma->vm_end - vma->vm_start;
    int i, j, pio_range_valid = 0;

    for(i = 0; i < _ndevices; i++) {
        bde_ctrl_t *ctrl = _devices + i;
        if (ctrl->dev_type & BDE_SWITCH_DEV_TYPE) {
            for (j = 0; j < BDE_NUM_IOWIN_MAX; j++) {
                if (paddr >= (unsigned long)ctrl->iowin[j].addr &&
                    (paddr + size) <= (unsigned long)(ctrl->iowin[j].addr + ctrl->iowin[j].size)) {
                    pio_range_valid = 1;
                    break;
                }
                if ((ctrl->dev_type & BDE_AXI_DEV_TYPE) && (paddr == ctrl->iowin[j].addr)) {
                    pio_range_valid = 1;
                    break;
                }
            }
        }
    }

    /* Support debug access to the mapping */
    vma->vm_ops = &bde_vma_ops;

    if (pio_range_valid) {
        vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

        if (remap_pfn_range(vma,
                        vma->vm_start,
                        vma->vm_pgoff,
                        size,
                        vma->vm_page_prot)) {
            gprintk("Failed to mmap phys range 0x%lx-0x%lx to 0x%lx-0x%lx\n",
                paddr, paddr + size, vma->vm_start,vma->vm_end);
            return -EAGAIN;
        }
        return 0;
    }

    return _dma_mmap(filp, vma);
}

/* Workaround for broken Busybox/PPC insmod */
static char _modname[] = LINUX_KERNEL_BDE_NAME;

static gmodule_t _gmodule = {
    .name = LINUX_KERNEL_BDE_NAME,
    .major = LINUX_KERNEL_BDE_MAJOR,
    .init = _init,
    .cleanup = _cleanup,
    .pprint = _pprint,
    .mmap = _bde_mmap,
};

gmodule_t *
gmodule_get(void)
{
    _gmodule.name = _modname;
    return &_gmodule;
}


/*
 * BDE Interface
 */

static const char *
_name(void)
{
    return LINUX_KERNEL_BDE_NAME;
}

static int
_num_devices(int type)
{
    switch (type) {
     case BDE_ALL_DEVICES:
        return _ndevices;
     case BDE_SWITCH_DEVICES:
        return _switch_ndevices;
     case BDE_ETHER_DEVICES:
        return _ether_ndevices;
     case BDE_CPU_DEVICES:
        return _cpu_ndevices;
    }

    return 0;
}

static const ibde_dev_t *
_get_dev(int d)
{
    if (!VALID_DEVICE(d)) {
        gprintk("_get_dev: Invalid device index %d\n", d);
        return NULL;
    }

    return &_devices[d].bde_dev;
}

static uint32
_get_dev_type(int d)
{
    if (!VALID_DEVICE(d)) {
        gprintk("_get_dev: Invalid device index %d\n", d);
        return 0;
    }

    return _devices[d].dev_type;
}

static uint32
_pci_conf_read(int d, uint32 addr)
{
#ifdef BCM_ICS
    return 0xFFFFFFFF;
#else
    uint32 rc = 0;

    if (!VALID_DEVICE(d)) {
        gprintk("_pci_conf_read: Invalid device index %d\n", d);
        return 0xFFFFFFFF;
    }

    if (!(_devices[d].dev_type & BDE_PCI_DEV_TYPE)) {
        gprintk("_pci_conf_read: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return 0xFFFFFFFF;
    }

    pci_read_config_dword(_devices[d].pci_device, addr, &rc);
    return rc;
#endif /* BCM_ICS */
}

static int
_pci_conf_write(int d, uint32 addr, uint32 data)
{
#ifdef BCM_ICS
    return -1;
#else
    if (!VALID_DEVICE(d)) {
        gprintk("_pci_conf_write: Invalid device index %d\n", d);
        return -1;
    }

    if (!(_devices[d].dev_type & BDE_PCI_DEV_TYPE)) {
        gprintk("_pci_conf_write: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return -1;
    }

    pci_write_config_dword(_devices[d].pci_device, addr, data);
    return 0;
#endif /* BCM_ICS */
}

/* Initialized when the bde is created */
static linux_bde_bus_t _bus;

static void
_pci_bus_features(int unit, int *be_pio, int *be_packet, int *be_other)
{
    if ((_devices[unit].bde_dev.device & 0xFF00) != 0x5600 &&
        (_devices[unit].bde_dev.device & 0xF000) != 0xc000 &&
        (_devices[unit].bde_dev.device & 0xF000) != 0xb000 &&
        (_devices[unit].bde_dev.device & 0xF000) != 0x8000 &&
        (_devices[unit].bde_dev.device & 0xFFF0) != 0x0230 &&
        (_devices[unit].bde_dev.device & 0xFFF0) != 0xa440) {
        if (be_pio) *be_pio = 0;
        if (be_packet) *be_packet = 0;
        if (be_other) *be_other = 0;
    } else {
        if (be_pio) *be_pio = _bus.be_pio;
        if (be_packet) *be_packet = _bus.be_packet;
        if (be_other) *be_other = _bus.be_other;
    }
#if defined(BCM_METROCORE_LOCAL_BUS)
    if (_devices[unit].dev_type & BDE_EB_DEV_TYPE && be_pio) {
        *be_pio = 1;
    }
#endif

}

static uint32_t
_read(int d, uint32_t addr)
{
    unsigned long flags;
    volatile uint16  msb, lsb;
    uint32  sl_addr, data;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        return -1;
    }

    if (_devices[d].dev_type & BDE_DEV_BUS_RD_16BIT) {
        /*
         * Adjust the address presented to Eb slave. Move A15:A0 to A16:A1.
         */
        sl_addr = (addr & 0xffff0000) | ((addr & 0xffff) << 1);
        /* Disable interrupts */
        spin_lock_irqsave(&bus_lock, flags);

        lsb = *((uint16 *)(_devices[d].bde_dev.base_address + sl_addr));
        msb = *((uint16 *)(_devices[d].bde_dev.base_address + sl_addr));
        spin_unlock_irqrestore(&bus_lock, flags);

        return (msb << 16) | lsb;
    } else {
        data = ((VOL uint32_t *)_devices[d].bde_dev.base_address)[addr / 4];
#if defined(CMIC_SOFT_BYTE_SWAP)
        data = CMIC_SWAP32(data);
#endif
        return data;
    }
}

static int
_write(int d, uint32_t addr, uint32_t data)
{
    unsigned long flags;
    uint32  sl_addr;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        return -1;
    }

    if (_devices[d].dev_type & BDE_DEV_BUS_WR_16BIT) {
        /*
         * Adjust the address presented to Eb slave. Move A15:A0 to A16:A1.
         */
        sl_addr = (addr & 0xffff0000) | ((addr & 0xffff) << 1);

        /* Disable interrupts */
        spin_lock_irqsave(&bus_lock, flags);

        *((VOL uint16 *)(_devices[d].bde_dev.base_address + sl_addr)) =
                                                             data & 0xffff;
        *((VOL uint16 *)(_devices[d].bde_dev.base_address + sl_addr)) =
                                                     (data >> 16) & 0xffff;
        spin_unlock_irqrestore(&bus_lock, flags);
    } else {
#if defined(CMIC_SOFT_BYTE_SWAP)
        data = CMIC_SWAP32(data);
#endif
        ((VOL uint32_t *)_devices[d].bde_dev.base_address)[addr / 4] = data;
#ifdef KEYSTONE
        /* Enforce PCIe transaction ordering. Commit the write transaction */
        __asm__ __volatile__("sync");
#endif
    }
    return 0;

}

static uint64
_read64(int d, uint32_t addr)
{
    uint64_t  data;
    if (!VALID_DEVICE(d)) {
        data =  (uint64_t)-1;
        return *(uint64 *)&data;
    }

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        data =  (uint64_t)-1;
        return *(uint64 *)&data;
    }

    data = ((VOL uint64_t *)_devices[d].bde_dev.base_address)[addr / 8];
#if defined(CMIC_SOFT_BYTE_SWAP)
    data = CMIC_SWAP64(data);
#endif
    return *(uint64 *)&data;
}

static void
_write64(int d, uint32_t addr, uint64 data)
{
    if (!VALID_DEVICE(d) || !(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        return;
    }

#if defined(CMIC_SOFT_BYTE_SWAP)
    data = CMIC_SWAP64(data);
#endif

    ((VOL uint64 *)_devices[d].bde_dev.base_address)[addr / 8] = data;
#ifdef KEYSTONE
    /* Enforce PCIe transaction ordering. Commit the write transaction */
    __asm__ __volatile__("sync");
#endif
}

static void
_run_isrs(bde_ctrl_t *ctrl)
{
    if (ctrl->isr) {
        ctrl->isr(ctrl->isr_data);
    }

    if (ctrl->isr2) {
        ctrl->isr2(ctrl->isr2_data);
    }
}

static _ISR_RET
_isr(_ISR_PARAMS(irq, dev_id, iregs))
{
    bde_ctrl_t *ctrl = (bde_ctrl_t *) dev_id;

    if (dma_lock) {
        return IRQ_HANDLED;
    }

    if (ctrl) {
        _run_isrs(ctrl);
        ctrl->interrupts++;
    }

    return IRQ_HANDLED;
}

#ifdef CONFIG_PCI_MSI
static void
lkbde_isrtick_func(bde_ctrl_t *ctrl)
{
    unsigned long flags;

    spin_lock_irqsave(&ctrl->lock, flags);
    if (!ctrl->timer_active) {
        /* Timer is not active, do nothing */
        spin_unlock_irqrestore(&ctrl->lock, flags);
        return;
    }

    if (ctrl->prev_interrupts == ctrl->interrupts) {
        if (ctrl->intr_pending(ctrl->intr_pending_data)) {
            if (++ctrl->no_intr_isr_ticks >= 2) {
                /* Run interrupt handlers if there are pending interrupts. */
                spin_unlock_irqrestore(&ctrl->lock, flags);
                _run_isrs(ctrl);
                spin_lock_irqsave(&ctrl->lock, flags);
                ctrl->stuck_interrupts++;
                ctrl->no_intr_isr_ticks = 0;
            }
        }
    } else {
        ctrl->no_intr_isr_ticks = 0;
        ctrl->prev_interrupts = ctrl->interrupts;
    }

    ctrl->isr_tick.expires = jiffies + msecs_to_jiffies(isrtickms);
    spin_unlock_irqrestore(&ctrl->lock, flags);
    if (ctrl->timer_active) {
        add_timer(&ctrl->isr_tick);
    }
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0))
static void
lkbde_isrtick(unsigned long context)
{
    bde_ctrl_t *ctrl = (bde_ctrl_t *)context;
    return lkbde_isrtick_func(ctrl);
}
#else
static void
lkbde_isrtick(struct timer_list *t)
{
    bde_ctrl_t *ctrl = from_timer(ctrl, t, isr_tick);
    return lkbde_isrtick_func(ctrl);
}
#endif

static void
lkbde_isrtick_add(bde_ctrl_t *ctrl)
{
    if (debug >= 1) {
        gprintk("Add isr tick, msi %d, intr pending %p, timer active %d\n",
                 ctrl->use_msi, ctrl->intr_pending, ctrl->timer_active);
    }

    if (ctrl->use_msi >= PCI_USE_INT_MSI && ctrl->intr_pending
          && !ctrl->timer_active) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0))
        init_timer(&ctrl->isr_tick);
        ctrl->isr_tick.data = (unsigned long)ctrl;
        ctrl->isr_tick.function = lkbde_isrtick;
#else
        timer_setup(&ctrl->isr_tick, lkbde_isrtick, 0);
#endif
        ctrl->isr_tick.expires = jiffies + msecs_to_jiffies(isrtickms);
        ctrl->timer_active = 1;
        add_timer(&ctrl->isr_tick);
        if (debug > 1) {
            gprintk("isr tick added\n");
        }
    }
}
#endif

static int
_interrupt_connect(int d,
                   void (*isr)(void *),
                   void *isr_data)
{
    bde_ctrl_t *ctrl;
    unsigned long irq_flags;
    int isr2_dev;
    int isr_active;
    int ret = 0;

    isr2_dev = d & LKBDE_ISR2_DEV;
    d &= ~LKBDE_ISR2_DEV;

    if (!VALID_DEVICE(d)) {
        gprintk("_interrupt_connect: Invalid device index %d\n", d);
        return -1;
    }
    if (debug >= 1) {
        gprintk("_interrupt_connect d %d\n", d);
    }
    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        gprintk("_interrupt_connect: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return -1;
    }

    ctrl = _devices + d;

    isr_active = (ctrl->isr || ctrl->isr2) ? 1 : 0;

    if (unlikely(debug > 1))
        gprintk("%s:isr_active = %d\n", __func__, isr_active);

    if (isr2_dev) {
        if (debug >= 1) {
            gprintk("connect secondary isr\n");
        }
        ctrl->isr2_data = isr_data;
        ctrl->isr2 = isr;
        if (isr_active) {
            /* Main handler (_isr) already installed */
            return 0;
        }
    } else {
        if (debug >= 1) {
            gprintk("connect primary isr\n");
        }
        ctrl->isr = isr;
        ctrl->isr_data = isr_data;
        if (isr_active) {
            if (debug >= 1) {
                gprintk("add isrtick added? %d\n", ctrl->timer_active);
            }

#ifdef CONFIG_PCI_MSI
            /* add isr tick for pending interrupt if ISR tick timer was not added */
            if (!ctrl->timer_active) {
                lkbde_isrtick_add(ctrl);
            }
#endif
            /* Main handler (_isr) already installed */
            return 0;
        }
    }

    if (ctrl->iLine != -1) {
        irq_flags = IRQF_SHARED;
#ifdef CONFIG_PCI_MSI
        if (ctrl->use_msi >= PCI_USE_INT_MSI) {
            ret = _msi_connect(ctrl);
            if(ret != 0)
                goto msi_exit;
        }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,84))
        if (ctrl->use_msi == PCI_USE_INT_MSIX) {
            int i;
            for (i = 0; i < ctrl->msix_cnt; i++) {
                 if (unlikely(debug >= 1)) {
                     gprintk("%s(%d):device# = %d, irq = %d\n",
                         __func__, __LINE__, d, ctrl->entries[i].vector);
                 }
                 ret = request_irq(ctrl->entries[i].vector, (irq_handler_t)_isr, 0,
                                   LINUX_KERNEL_BDE_NAME, ctrl);
                 if (ret < 0)
                     break;
            }
            if (ret < 0) {
                while (i >= 0)
                    free_irq(ctrl->entries[i--].vector, ctrl);

                goto err_disable_msi;
            }
        }
        else
#endif
#endif
        {
#if defined(IPROC_CMICD) && defined(CONFIG_OF)
            if (of_find_compatible_node(NULL, NULL, IPROC_CMICX_COMPATIBLE) &&
                (ctrl->pci_device == NULL)) {
                int i, j;
                for (i = 0; i < IHOST_CMICX_MAX_INTRS; i++) {
                    if (!iproc_cmicx_irqs[i]) {
                        continue;
                    }
                    if (unlikely(debug >= 1))
                        gprintk("%s(%d):device# = %d, request_irq(%d)\n",
                            __func__, __LINE__, d, iproc_cmicx_irqs[i]);
                    ret = request_irq(iproc_cmicx_irqs[i], (irq_handler_t)_isr,
                            irq_flags, LINUX_KERNEL_BDE_NAME, ctrl);
                    if (ret < 0) {
                        gprintk("request_irq(%d) failed(%d)\n", iproc_cmicx_irqs[i], ret);
                        break;
                    }
                }
                if (ret < 0) {
                    for (j = 0; j < i; j++) {
                        if (!iproc_cmicx_irqs[j]) {
                            continue;
                        }
                        free_irq(iproc_cmicx_irqs[j], ctrl);
                    }
                    goto err_disable_msi;
                }
            } else
#endif
            {
                ret = request_irq(ctrl->iLine, _isr, irq_flags,
                       LINUX_KERNEL_BDE_NAME, ctrl);
                if (ret < 0)
                    goto err_disable_msi;

                if (pci_irq_affinity >= 0) {
                    struct cpumask cmask;

                    /* Clear the cpumask and pin IRQ to pci_irq_affinity */
                    cpumask_clear(&cmask);
                    cpumask_set_cpu(pci_irq_affinity, &cmask);

                    ret = irq_set_affinity_hint(ctrl->iLine, &cmask);
                    if (ret) {
                        gprintk("Set IRQ(%d) affinity to core %d failed\n",
                                ctrl->iLine, pci_irq_affinity);
                        pci_irq_affinity = -1;
                    }
                }

                if (unlikely(debug >= 1))
                    gprintk("%s(%d):device# = %d, irq_flags = %lu, irq = %d\n",
                         __func__, __LINE__, d,
                         irq_flags, ctrl->pci_device ? ctrl->pci_device->irq : ctrl->iLine);
            }
        }
    }

#ifdef CONFIG_PCI_MSI
    /* add isr tick */
    lkbde_isrtick_add(ctrl);
#endif
    return 0;

err_disable_msi:
#ifdef CONFIG_PCI_MSI
     _msi_disconnect(ctrl);

msi_exit:
#endif
     gprintk("could not request IRQ\n");
     ctrl->isr = NULL;
     ctrl->isr_data = NULL;
     ctrl->isr2 = NULL;
     ctrl->isr2_data = NULL;

            return -1;
}

static int
_interrupt_disconnect(int d)
{
    bde_ctrl_t *ctrl;
    int isr2_dev;
    int isr_active;

    isr2_dev = d & LKBDE_ISR2_DEV;
    d &= ~LKBDE_ISR2_DEV;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (debug >= 1) {
        gprintk("_interrupt_disconnect d %d\n", d);
    }
    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        gprintk("_interrupt_disconnect: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return -1;
    }

    ctrl = _devices + d;

    isr_active = (ctrl->isr || ctrl->isr2) ? 1 : 0;

    if (unlikely(debug > 1))
        gprintk("%s: isr_active = %d\n", __func__, isr_active);

#ifdef CONFIG_PCI_MSI
    if (isr_active) {
        if (ctrl->intr_pending && (ctrl->use_msi >= PCI_USE_INT_MSI) && ctrl->timer_active) {
            ctrl->timer_active = 0;
            del_timer_sync(&ctrl->isr_tick);
        }
    }
#endif

    if (isr2_dev) {
        if (debug >= 1) {
            gprintk("disconnect secondary isr\n");
        }
        ctrl->isr2 = NULL;
        ctrl->isr2_data = NULL;
        ctrl->fmask = 0;
        if (ctrl->isr) {

#ifdef CONFIG_PCI_MSI
            /* restart timer to handling pending interrupts */
            if (ctrl->intr_pending && ctrl->use_msi >= PCI_USE_INT_MSI) {
                lkbde_isrtick_add(ctrl);
            }
#endif
            /* Primary handler still active */
            SYNC_IRQ(ctrl->iLine);

            return 0;
        }
    } else {
        if (debug >= 1) {
            gprintk("disconnect primary isr\n");
        }
        ctrl->isr = NULL;
        ctrl->isr_data = NULL;
        if (ctrl->isr2) {

#ifdef CONFIG_PCI_MSI
            /* restart timer to handling pending interrupts */
            if (ctrl->intr_pending && ctrl->use_msi >= PCI_USE_INT_MSI) {
                lkbde_isrtick_add(ctrl);
            }
#endif
            /* Secondary handler still active */
            SYNC_IRQ(ctrl->iLine);
            return 0;
        }
    }

    if (isr_active) {
#ifdef CONFIG_PCI_MSI
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,84))
        if (ctrl->use_msi >= PCI_USE_INT_MSIX) {
            int i;
            for (i = 0; i < ctrl->msix_cnt; i++) {
                if (unlikely(debug > 1)) {
                     gprintk("%s(%d):device# = %d, irq = %d\n",
                         __func__, __LINE__, d, ctrl->entries[i].vector);
                }
                free_irq(ctrl->entries[i].vector, ctrl);
            }
        }
        else
#endif
#endif
#if defined(IPROC_CMICD) && defined(CONFIG_OF)
        if (of_find_compatible_node(NULL, NULL, IPROC_CMICX_COMPATIBLE) &&
            (ctrl->pci_device == NULL)) {
            int i;
            for (i = 0; i < IHOST_CMICX_MAX_INTRS; i++) {
                if (!iproc_cmicx_irqs[i]) {
                    continue;
                }
                if (unlikely(debug > 1)) {
                    gprintk("%s(%d):device# = %d, free_irq(%d)\n",
                            __func__, __LINE__, d, iproc_cmicx_irqs[i]);
                }
                free_irq(iproc_cmicx_irqs[i], ctrl);
            }
        } else
#endif
        {
            if (pci_irq_affinity >= 0) {
                irq_set_affinity_hint(ctrl->iLine, NULL);
            }
            free_irq(ctrl->iLine, ctrl);
        }
#ifdef CONFIG_PCI_MSI
        if (ctrl->use_msi >= PCI_USE_INT_MSI) {
            _msi_disconnect(ctrl);
        }
#endif
    }

    return 0;
}

static uint32_t
_iproc_ihost_read(int d, uint32_t addr)
{
    uint32_t *mapaddr;
    uint32_t reg_val;
    mapaddr = ioremap(addr, sizeof(uint32_t));
    if (mapaddr == NULL) {
        return -1;
    }
    reg_val = readl(mapaddr);
    iounmap(mapaddr);
    return reg_val;
}

static int
_iproc_ihost_write(int d, uint32_t addr, uint32_t data)
{
    uint32_t *mapaddr;
    mapaddr = ioremap(addr, sizeof(uint32_t));
    if (mapaddr == NULL) {
        return -1;
    }
    writel(data, mapaddr);
    iounmap(mapaddr);
    return 0;
}

static uint32_t
_iproc_read(int d, uint32_t addr)
{
    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type)) && (_devices[d].dev_type & BDE_USER_DEV_TYPE) == 0) {
        return -1;
    }

    if (_devices[d].dev_type & BDE_AXI_DEV_TYPE) {
        if (IHOST_GICD_REG_ADDR_VALID(d, addr)) {
            return readl(IHOST_GICD_REG_ADDR_REMAP(d, addr));
        }
        return _iproc_ihost_read(d, addr);
    }

    return shbde_iproc_pci_read(&_devices[d].shbde,
                                (void *)_devices[d].bde_dev.base_address1,
                                addr);
}

static int
_iproc_write(int d, uint32_t addr, uint32_t data)
{
    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type)) && (_devices[d].dev_type & BDE_USER_DEV_TYPE) == 0) {
        return -1;
    }

    if (_devices[d].dev_type & BDE_AXI_DEV_TYPE) {
        if (IHOST_GICD_REG_ADDR_VALID(d, addr)) {
            writel(data, IHOST_GICD_REG_ADDR_REMAP(d, addr));
            return 0;
        }
        return _iproc_ihost_write(d, addr, data);
    }

    shbde_iproc_pci_write(&_devices[d].shbde,
                          (void *)_devices[d].bde_dev.base_address1,
                          addr, data);

    return 0;
}

static int
_get_cmic_ver(int d ,  uint32_t *ver)
{

    unsigned int cap_base;
    uint32_t rval = 0;

    if (!VALID_DEVICE(d)) {
        gprintk("%s: Invalid device index %d\n", __func__, d);
        return -1;
    }

    if (!(_devices[d].dev_type & BDE_PCI_DEV_TYPE)) {
        gprintk("%s: Not PCI device %d, type %x\n", __func__,
                d, _devices[d].dev_type);
        return -1;
    }

    /* Look for PCIe vendor-specific extended capability (VSEC) */
    cap_base = PCI_EXT_CAP_START;
    while (cap_base) {
        pci_read_config_dword(_devices[d].pci_device, cap_base, &rval);
        if (rval == 0xffffffff) {
           /* Assume PCI HW read error */
           gprintk("%s: PCI HW read error\n", __func__);
           return -1;
        }

        if (PCI_EXT_CAP_ID(rval) == PCI_EXT_CAP_ID_VNDR) {
            break;
        }
        cap_base = PCI_EXT_CAP_NEXT(rval);
    }
    if (cap_base) {
        /*
         * VSEC layout:
         *
         * 0x00: PCI Express Extended Capability Header
         * 0x04: Vendor-Specific Header
         * 0x08: Vendor-Specific Register 1
         * 0x0c: Vendor-Specific Register 2
         *     ...
         * 0x24: Vendor-Specific Register 8
         */
        pci_read_config_dword(_devices[d].pci_device, cap_base + 8, &rval);
        if (unlikely(debug > 1))
            gprintk("%s:Found VSEC = %u\n", __func__, rval);

        /* Determine if CMICX */
        rval = ((rval >> 12) & 0xf);
        *ver = rval;

        return 0;
    } else {
        gprintk("%s:VSEC not found\n", __func__);
    }

    return -1;
}

#ifdef BCM_SAND_SUPPORT
int
lkbde_cpu_write(int d, uint32 addr, uint32 *buf)
{
#if defined(__DUNE_WRX_BCM_CPU__) || defined(__DUNE_GTO_BCM_CPU__)
    *((uint32_t*)((uint8_t*)cpu_address + addr)) = *buf;
#endif

    return 0;
}

int
lkbde_cpu_read(int d, uint32 addr, uint32 *buf)
{
#if defined(__DUNE_WRX_BCM_CPU__) || defined(__DUNE_GTO_BCM_CPU__)
    *buf = *((uint32_t*)((uint8_t*)cpu_address + addr));
#else
    *buf = (uint32_t)(-1);
#endif

    return 0;
}

int
lkbde_cpu_pci_register(int d)
{
    bde_ctrl_t* ctrl;
    uint16  cmd = 0;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    ctrl = &_devices[d];

    /* enable device */
    if (pci_enable_device(ctrl->pci_device)) {
        gprintk("Cannot enable pci device : vendor_id = %x, device_id = %x\n",
                ctrl->pci_device->vendor, ctrl->pci_device->device);
    }

    /* Add PCI_COMMAND_MEMORY and PCI_COMMAND_MASTER */
    pci_read_config_word(ctrl->pci_device, PCI_COMMAND, &cmd);
    if (!(cmd & PCI_COMMAND_MEMORY) || !(cmd & PCI_COMMAND_MASTER)) {
        cmd |= PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER;
        pci_write_config_word(ctrl->pci_device, PCI_COMMAND, cmd);
    }

    switch (ctrl->bde_dev.device) {
    case BCM88770_DEVICE_ID:
    case BCM88773_DEVICE_ID:
    case BCM88774_DEVICE_ID:
    case BCM88775_DEVICE_ID:
    case BCM88776_DEVICE_ID:
    case BCM88777_DEVICE_ID:
    case BCM88950_DEVICE_ID:
    case BCM88953_DEVICE_ID:
    case BCM88954_DEVICE_ID:
    case BCM88955_DEVICE_ID:
    case BCM88956_DEVICE_ID:
    case BCM88772_DEVICE_ID:
    case BCM88952_DEVICE_ID:

    case BCM88670_DEVICE_ID:
    case BCM88671_DEVICE_ID:
    case BCM88671M_DEVICE_ID:
    case BCM88672_DEVICE_ID:
    case BCM88673_DEVICE_ID:
    case BCM88674_DEVICE_ID:
    case BCM88675_DEVICE_ID:
    case BCM88675M_DEVICE_ID:
    case BCM88676_DEVICE_ID:
    case BCM88676M_DEVICE_ID:
    case BCM88677_DEVICE_ID:
    case BCM88678_DEVICE_ID:
    case BCM88679_DEVICE_ID:

    case BCM88370_DEVICE_ID:
    case BCM88371_DEVICE_ID:
    case BCM88371M_DEVICE_ID:
    case BCM88375_DEVICE_ID:
    case BCM88376_DEVICE_ID:
    case BCM88376M_DEVICE_ID:
    case BCM88377_DEVICE_ID:
    case BCM88378_DEVICE_ID:
    case BCM88379_DEVICE_ID:
    case BCM88681_DEVICE_ID:
    case BCM88682_DEVICE_ID:
    case BCM88683_DEVICE_ID:
    case BCM88684_DEVICE_ID:
    case BCM88685_DEVICE_ID:
    case BCM88687_DEVICE_ID:
    case BCM88380_DEVICE_ID:
    case BCM88381_DEVICE_ID:
    case BCM88680_DEVICE_ID:
    case BCM88470_DEVICE_ID:
    case BCM88470P_DEVICE_ID:
    case BCM88471_DEVICE_ID:
    case BCM88473_DEVICE_ID:
    case BCM88474_DEVICE_ID:
    case BCM88474H_DEVICE_ID:
    case BCM88476_DEVICE_ID:
    case BCM88477_DEVICE_ID:
    case BCM88479_DEVICE_ID:
    case BCM88270_DEVICE_ID:
    case BCM88271_DEVICE_ID:
    case BCM88272_DEVICE_ID:
    case BCM88273_DEVICE_ID:
    case BCM88274_DEVICE_ID:
    case BCM88276_DEVICE_ID:
    case BCM88278_DEVICE_ID:
    case BCM88350_DEVICE_ID:
    case BCM88351_DEVICE_ID:
    case BCM88450_DEVICE_ID:
    case BCM88451_DEVICE_ID:
    case BCM88550_DEVICE_ID:
    case BCM88551_DEVICE_ID:
    case BCM88552_DEVICE_ID:
    case BCM88660_DEVICE_ID:
    case BCM88360_DEVICE_ID:
    case BCM88361_DEVICE_ID:
    case BCM88363_DEVICE_ID:
    case BCM88460_DEVICE_ID:
    case BCM88461_DEVICE_ID:
    case BCM88560_DEVICE_ID:
    case BCM88561_DEVICE_ID:
    case BCM88562_DEVICE_ID:
    case BCM88661_DEVICE_ID:
    case BCM88664_DEVICE_ID:
        /* Fix bar 0 address */ 
        pci_write_config_byte(ctrl->pci_device, 0x12, 0x10);
        pci_write_config_byte(ctrl->pci_device, 0x13, 0x60);

        /*
         * For DMA transactions - set Max_Payload_Size and
         * Max_Read_Request_Size to 128 bytes.
         */
        pci_write_config_byte(ctrl->pci_device, 0xb5, 0x0c);
        pci_write_config_byte(ctrl->pci_device, 0xb4, 0x0);
        break;
    default:
        break;
    }

    /* configure iproc >=14 devices by device family */
#if defined(BCM_DNXF_SUPPORT) || defined(BCM_DNX_SUPPORT)
    switch (ctrl->bde_dev.device & DNXC_DEVID_FAMILY_MASK) {
#ifdef BCM_DNX_SUPPORT
      case JERICHO2_DEVICE_ID:
      case J2C_DEVICE_ID:
      case J2C_2ND_DEVICE_ID:
      case Q2A_DEVICE_ID:
      case Q2U_DEVICE_ID:
      case Q2N_DEVICE_ID:
      case J2P_DEVICE_ID:
      case J2X_DEVICE_ID:
#ifdef BCM_DNX3_SUPPORT
      case JERICHO3_DEVICE_ID:
      case J3AI_DEVICE_ID:
      case Q3D_DEVICE_ID:
#ifdef BCM_Q3A_SUPPORT
      case Q3A_DEVICE_ID:
      case Q3U_DEVICE_ID:
#endif
#endif
#ifdef BCM_JERICHO_4_SUPPORT
      case JERICHO4_DEVICE_ID:
      case Q4_DEVICE_ID:
#endif
#ifdef BCM_Q4D_SUPPORT
      case Q4D_DEVICE_ID:
#endif
#ifdef BCM_J4L_SUPPORT
      case J4L_DEVICE_ID:
#endif

#endif
#ifdef BCM_DNXF_SUPPORT
      case  BCM88790_DEVICE_ID:
#ifdef BCM_DNXF3_SUPPORT
      case RAMON2_DEVICE_ID:
      case RAMON3_DEVICE_ID:
#endif
#endif
#ifdef BCM_DNXFE_SUPPORT
#ifdef BCM_RAMON_4_SUPPORT
      case RAMON4_DEVICE_ID:
#endif
#endif
        /*
         * For DMA transactions - set Max_Payload_Size and
         * Max_Read_Request_Size to 128 bytes.
         */
        pci_write_config_byte(ctrl->pci_device, 0xb5, 0x0c);
        pci_write_config_byte(ctrl->pci_device, 0xb4, 0x0);
        break;
    }
#endif /* defined(BCM_DNXF_SUPPORT) || defined(BCM_DNX_SUPPORT) */

    /* Redo ioremap */
    if (ctrl->bde_dev.base_address) {
        iounmap((void *)ctrl->bde_dev.base_address);
    }
    ctrl->bde_dev.base_address = (sal_vaddr_t)ioremap(ctrl->iowin[0].addr, 0x1000000);

    if (debug >= 1) {
        gprintk("%s, %s(): info:\n", __FILE__, __FUNCTION__);
        gprintk("_ndevices=%d, _switch_ndevices=%d\n",
                _ndevices, _switch_ndevices);
        gprintk("ctrl->dev_type=0x%x, ctrl->phys_address=0x%lx\n",
                ctrl->dev_type, (unsigned long)ctrl->iowin[0].addr);
        gprintk("ctrl->bde_dev.device=0x%x, ctrl->bde_dev.rev=0x%x, "
                "ctrl->bde_dev.base_address=0x%lx\n",
                ctrl->bde_dev.device, ctrl->bde_dev.rev,
                (unsigned long)ctrl->bde_dev.base_address);
    }

    return 0;
}

/*
 * Export Low level access function - currently for PCP DMA Kernel module.
 */
int
lkbde_mem_write(int d, uint32 addr, uint32 *buf)
{
    bde_ctrl_t* ctrl;
    void *full_addr;

    if (!VALID_DEVICE(d)) return -1;
    ctrl = &_devices[d];

    full_addr   = (void *)ctrl->bde_dev.base_address + addr;
   *((uint32_t*)full_addr) = *buf;
    return 0;
}
LKM_EXPORT_SYM(lkbde_mem_write);

int
lkbde_mem_read(int d, uint32 addr, uint32 *buf)
{
    bde_ctrl_t* ctrl;
    void *full_addr;

    if (!VALID_DEVICE(d)) return -1;
    ctrl = &_devices[d];

    full_addr   = (void *)ctrl->bde_dev.base_address + addr;
    *buf = *((uint32_t*)full_addr);
    return 0;
}
LKM_EXPORT_SYM(lkbde_mem_read);
#endif /* BCM_SAND_SUPPORT */

static ibde_t _ibde = {
    .name = _name,
    .num_devices = _num_devices,
    .get_dev = _get_dev,
    .get_dev_type = _get_dev_type,
    .pci_conf_read = _pci_conf_read,
    .pci_conf_write = _pci_conf_write,
    .pci_bus_features = _pci_bus_features,
    .read = _read,
    .write = _write,
    .salloc = _salloc,
    .sfree = _sfree,
    .sinval = _sinval,
    .sflush = _sflush,
    .interrupt_connect = _interrupt_connect,
    .interrupt_disconnect = _interrupt_disconnect,
    .l2p = _l2p,
    .p2l = _p2l,

    NULL,
    NULL,
    .iproc_read = _iproc_read,
    .iproc_write = _iproc_write,
    .get_cmic_ver = _get_cmic_ver,
    .read64 = _read64,
    .write64 = _write64,
};

/*
 * Function: linux_bde_create
 *
 * Purpose:
 *    Creator function for this BDE interface.
 * Parameters:
 *    bus - pointer to the bus features structure you want this
 *          bde to export. Depends on the system.
 *    ibde - pointer to a location to recieve the bde interface pointer.
 * Returns:
 *    0 on success
 *    -1 on failure.
 * Notes:
 *    This is the main BDE create function for this interface.
 *    Used by the external system initialization code.
 */
int
linux_bde_create(linux_bde_bus_t *bus, ibde_t **ibde)
{

    memset(&_bus, 0, sizeof(_bus));

    if (bus) {
        _bus = *bus;
    }
#ifdef NONCOHERENT_DMA_MEMORY
#ifdef REMAP_DMA_NONCACHED
    /*
     * If we have a non-cached DMA memory pool
     * there is no need to flush and invalidate.
     */
    if (_dma_pool_allocated()) {
        _ibde.sinval = NULL;
        _ibde.sflush = NULL;
    }
#endif
#else
    _ibde.sinval = NULL;
    _ibde.sflush = NULL;
#endif
    *ibde = &_ibde;
    return 0;
}

/*
 * Function: linux_bde_destroy
 *
 * Purpose:
 *    destroy this bde
 * Parameters:
 *    BDE interface pointer
 * Returns:
 *    0 on success, < 0 on error.
 */
int
linux_bde_destroy(ibde_t *ibde)
{

    /* timer was added in user_bde context, must delete it */
#ifdef CONFIG_PCI_MSI
    int i;
    for (i = 0; i < _ndevices; i++) {
        bde_ctrl_t *ctrl = _devices + i;

        /* free allocated kernel space memory */
        if (ctrl->intr_pending && (ctrl->use_msi >= PCI_USE_INT_MSI) && ctrl->timer_active) {
            ctrl->timer_active = 0;
            del_timer_sync(&ctrl->isr_tick);
        }
    }
#endif

    /* nothing */
     return 0;
}

/*
 *  Backdoors provided by the kernel bde
 */

uint32_t
lkbde_get_dev_phys(int d)
{
    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        gprintk("lkbde_get_dev_phys: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return 0;
    }
    return _devices[d].iowin[0].addr;
}

uint32_t
lkbde_get_dev_phys_hi(int d)
{
    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        gprintk("lkbde_get_dev_phys: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return 0;
    }
#ifdef PHYS_ADDR_IS_64BIT
    return (uint32_t)(_devices[d].iowin[0].addr >> 32);
#else
    return 0;
#endif
}

void *
lkbde_get_dev_virt(int d)
{
    if (!VALID_DEVICE(d)) {
        return NULL;
    }

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        gprintk("lkbde_get_dev_virt: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return 0;
    }

    if (_devices[d].alt_base_addr) {
        return (void *)_devices[d].alt_base_addr;
    }

    return (void *)_devices[d].bde_dev.base_address;
}

int
lkbde_get_dev_resource(int d, int rsrc, uint32_t *phys_lo,
                       uint32_t *phys_hi, uint32_t *size)
{
    if (!VALID_DEVICE(d)) {
        return -1;
    }

    *size = 0;
    *phys_lo = 0;
    *phys_hi = 0;

    if (!(BDE_DEV_MEM_MAPPED(_devices[d].dev_type))) {
        gprintk("lkbde_get_dev_phys: Not PCI device %d, type %x\n",
                d, _devices[d].dev_type);
        return 0;
    }

    switch (rsrc) {
    case 0:
        *phys_lo = (uint32_t)(_devices[d].iowin[0].addr);
#ifdef PHYS_ADDR_IS_64BIT
        *phys_hi = (uint32_t)(_devices[d].iowin[0].addr >> 32);
#endif
        *size = _devices[d].iowin[0].size;
        break;
    case 1:
        *phys_lo = (uint32_t)(_devices[d].iowin[1].addr);
#ifdef PHYS_ADDR_IS_64BIT
        *phys_hi = (uint32_t)(_devices[d].iowin[1].addr >> 32);
#endif
        *size = _devices[d].iowin[1].size;
        break;
    default:
        break;
    }

    return 0;
}

void *
lkbde_get_dma_dev(int d)
{
    if (!VALID_DEVICE(d)) {
        return NULL;
    }

#ifdef LINUX_BDE_DMA_DEVICE_SUPPORT
    return (void *)_devices[d].dma_dev;
#else
    return (void *)_devices[d].pci_device;
#endif
}

void *
lkbde_get_hw_dev(int d)
{
    if (!VALID_DEVICE(d)) {
        return NULL;
    }

    return (void *)_devices[d].pci_device;
}

int
lkbde_dev_state_set(int d, uint32 state)
{
    bde_ctrl_t *ctrl;

    if (!VALID_DEVICE(d)) {
        return -1;
    }
    ctrl = _devices + d;
    ctrl->dev_state = state;
    return 0;
}

int
lkbde_dev_state_get(int d, uint32 *state)
{
    bde_ctrl_t *ctrl;
    if (!VALID_DEVICE(d)) {
        gprintk("_get_dev: Invalid device index %d\n", d);
        return -1;
    }
    ctrl = _devices + d;

    *state = ctrl->dev_state;
    return 0;
}

int
lkbde_dev_instid_set(int d, uint32 instid)
{
    bde_ctrl_t *ctrl;

    if (!VALID_DEVICE(d)) {
        return -1;
    }
    ctrl = _devices + d;
    ctrl->inst_id = instid;

    return 0;
}

int
lkbde_dev_instid_get(int d, uint32 *instid)
{
    bde_ctrl_t *ctrl;

    if (!VALID_DEVICE(d)) {
        return -1;
    }
    ctrl = _devices + d;
    *instid = ctrl->inst_id;

    return 0;
}

/*
 * Function: lkbde_get_dev_pci_info
 *
 * Purpose:
 *     Get the PCI bus number, bus slot, and function for a PCI device.
 * Returns:
 *    0 on success, -1 on error.
 */
int
lkbde_get_dev_pci_info(int d, uint32_t *bus, uint32_t *slot, uint32_t *func)
{
    bde_ctrl_t *ctrl;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    ctrl = _devices + d;

    if (ctrl->dev_type & BDE_PCI_DEV_TYPE) {
        struct pci_dev *device = ctrl->pci_device;
        if (bus)
            *bus = device->bus->number;
        if (slot)
            *slot = PCI_SLOT(device->devfn);
        if (func)
            *func = PCI_FUNC(device->devfn);
        return 0;
    } else {
        return -1;
    }
}

/*
 * When a secondary interrupt handler is installed this function
 * is used for synchronizing hardware access to the IRQ mask
 * register. The secondary driver will supply a non-zero fmask
 * (filter mask) to indicate which interrupt bits it controls.
 * The fmask is ignored for the primary driver.
 */
int
lkbde_irq_mask_set(int d, uint32_t addr, uint32_t mask, uint32_t fmask)
{
    bde_ctrl_t *ctrl;
    int isr2_dev, iproc_reg;
    unsigned long flags;

    isr2_dev = d & LKBDE_ISR2_DEV;
    iproc_reg = d & LKBDE_IPROC_REG;
    d &= ~(LKBDE_ISR2_DEV | LKBDE_IPROC_REG);

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    ctrl = _devices + d;

    /* Lock is required to synchronize access from user space */
    spin_lock_irqsave(&ctrl->lock, flags);

    if (isr2_dev) {
        /* This is the secondary interrupt handler */
        ctrl->fmask = fmask;
        ctrl->imask2 = mask & ctrl->fmask;
    } else {
        /* This is the primary interrupt handler */
        ctrl->imask = mask & ~ctrl->fmask;
    }

    if (iproc_reg) {
        _iproc_write(d, addr, ctrl->imask | ctrl->imask2);
    } else {
        _write(d, addr, ctrl->imask | ctrl->imask2);
    }

    spin_unlock_irqrestore(&ctrl->lock, flags);

    return 0;
}

/*
 * When a secondary interrupt handler is installed, this function
 * is used to avoid activating the user mode interrupt handler
 * thread if all pending interrupts are handled in kernel space.
 *
 * The mask returned is the mask of all interrupts,  it can be used
 * to do a logical AND with fmask, the result will tell you if
 * the user mode interrupt handler needs to be invoked.
 *
 * Note that if no secondary handler is installed, the value of
 * "mask & fmask" will be zero, and hence there will be no need to read the
 * current interrupt status from hardware (from kernel space).
 */
int
lkbde_irq_mask_get(int d, uint32_t *mask, uint32_t *fmask)
{
    bde_ctrl_t *ctrl;

    d &= ~(LKBDE_ISR2_DEV | LKBDE_IPROC_REG);

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    if (mask == NULL || fmask == NULL) {
        return -1;
    }

    ctrl = _devices + d;

    *fmask = ctrl->fmask;
    *mask = ctrl->imask | ctrl->imask2;

    return 0;
}

int
lkbde_irq_status_get(int d, uint32_t addr, uint32 *status)
{
    bde_ctrl_t *ctrl;
    int iproc_reg;
    unsigned long flags;

    iproc_reg = d & LKBDE_IPROC_REG;
    d &= ~LKBDE_IPROC_REG;

    if (!VALID_DEVICE(d)) {
        return -1;
    }

    ctrl = _devices + d;

    /* Lock is required to synchronize access from user space */
    spin_lock_irqsave(&ctrl->lock, flags);

    if (iproc_reg) {
        *status = _iproc_read(d, addr);
    } else {
        *status = _read(d, addr);
    }

    spin_unlock_irqrestore(&ctrl->lock, flags);

    return 0;
}

int
lkbde_iproc_version_get(int d, uint32 *iproc_ver)
{
    bde_ctrl_t *ctrl;
    shbde_iproc_config_t icfg;

    if (!VALID_DEVICE(d)) {
        return -1;
    }
    ctrl = _devices + d;

    if (shbde_pci_iproc_version_get(&ctrl->shbde, ctrl->pci_device, &icfg.iproc_ver,
                                    &icfg.cmic_ver, &icfg.cmic_rev)) {
        *iproc_ver = icfg.iproc_ver;
        return 0;
    }

    return -1;
}

int
lkbde_get_num_devices(int type)
{
    return _num_devices(type);
}

/*
 * Return none-zero if the SDK instance with the given instance ID
 * manages the given device.
 */
int lkbde_is_dev_managed_by_instance(uint32 dev, uint32 inst_id)
{
    if (inst_id >= LINUX_BDE_MAX_DEVICES || dev >= _ndevices) {
        return 0;
    }
    return _instance_info[inst_id].devices[dev / 32] & (1 << (dev % 32)) ? 1 : 0;
}

/*
 * Return a pointer to the bitmap of the SDK instance managed devices.
 */
linux_bde_device_bitmap_t* lkbde_get_inst_devs(uint32 inst_id)
{
    if (inst_id >= LINUX_BDE_MAX_DEVICES) {
        return NULL;
    }
    return &_instance_info[inst_id].devices;
}

/*
 * Register a callback function to retrieve interrupt pending status from
 * the user BDE.
 */
int
lkbde_intr_cb_register(int d,
                       int (*intr_pending)(void *),
                       void *intr_pending_data)
{
    bde_ctrl_t *ctrl;

    if (!VALID_DEVICE(d)) {
        gprintk("lkbde_intr_cb_register: Invalid device index %d\n", d);
        return -1;
    }

    if (debug >= 1) {
        gprintk("lkbde_intr_cb_register d %d\n", d);
    }

    ctrl = _devices + d;
    ctrl->intr_pending_data = intr_pending_data;
    ctrl->intr_pending = intr_pending;

    return 0;
}

#ifdef INCLUDE_SRAM_DMA

#ifdef SRAM_DMA_NEEDS_KERNEL_APIS
/* implementation of BDE APIs for DMA SRAM mode */

static int
_sinval_sram(int d, void *ptr, int length)
{
    return 0;
}

static int
_sflush_sram(int d, void *ptr, int length)
{
    return 0;
}

/* In SRAM DMA there are no virtual addresses.
 * We assume the CPU cannot access the SRAM directly, even if it is mapped to a BAR window.
 * Therfore p2l and l2p should not be called.
 * If they are called, we will return the input value,
 * possibly except if it is not in the SRAM address range used for DMA.
 */

static void *
_p2l_sram(int d, sal_paddr_t paddr)
{
    uint32 dev_sram_dma_start = _devices[d].dev_sram_dma_start;
    uint32 dev_sram_dma_size  = _devices[d].dev_sram_dma_size;
    if (paddr < dev_sram_dma_start || paddr >= dev_sram_dma_start + dev_sram_dma_size) {
        gprintk("Error: SRAM address translation called for %p which is outside SRAM range %u-%u\n",
                (void*)paddr, dev_sram_dma_start, dev_sram_dma_start +dev_sram_dma_size - 1);
        return 0;
    }
    return (void *)paddr;
}

static sal_paddr_t
_l2p_sram(int d, void *vaddr)
{
    return (sal_paddr_t)_p2l_sram(d, (sal_paddr_t)vaddr);
}

/* upate BDE API handles for SRAM DMA */
void _update_apis_for_sram_dma()
{
    _ibde.sinval = _sinval_sram;
    _ibde.sflush = _sflush_sram;
    _ibde.l2p = _l2p_sram;
    _ibde.p2l = _p2l_sram;
};
#endif /* SRAM_DMA_NEEDS_KERNEL_APIS */

/* return the (currently single) DMA buffer pool size for SRAM DMA mode, must only be called in this mode */
void
lkbde_get_sram_dma_info(unsigned d, uint32 *sram_start, uint32 *sram_size)
{
    if (d < _ndevices) {
        *sram_start = _devices[d].dev_sram_dma_start;
        *sram_size  = _devices[d].dev_sram_dma_size;
    } else {
        *sram_start = *sram_size = 0;
    }
}
#endif /* INCLUDE_SRAM_DMA */

/*
 * Export functions
 */
LKM_EXPORT_SYM(linux_bde_create);
LKM_EXPORT_SYM(linux_bde_destroy);
LKM_EXPORT_SYM(lkbde_get_dev_phys);
LKM_EXPORT_SYM(lkbde_get_dev_virt);
LKM_EXPORT_SYM(lkbde_get_dev_resource);
LKM_EXPORT_SYM(lkbde_get_hw_dev);
LKM_EXPORT_SYM(lkbde_get_dma_dev);
LKM_EXPORT_SYM(lkbde_irq_mask_set);
LKM_EXPORT_SYM(lkbde_irq_mask_get);
LKM_EXPORT_SYM(lkbde_irq_status_get);
LKM_EXPORT_SYM(lkbde_get_dev_phys_hi);
LKM_EXPORT_SYM(lkbde_dev_state_set);
LKM_EXPORT_SYM(lkbde_dev_state_get);
LKM_EXPORT_SYM(lkbde_dev_instid_set);
LKM_EXPORT_SYM(lkbde_dev_instid_get);
#ifdef BCM_SAND_SUPPORT
LKM_EXPORT_SYM(lkbde_cpu_write);
LKM_EXPORT_SYM(lkbde_cpu_read);
LKM_EXPORT_SYM(lkbde_cpu_pci_register);
#endif
LKM_EXPORT_SYM(lkbde_is_dev_managed_by_instance);
LKM_EXPORT_SYM(lkbde_get_inst_devs);
LKM_EXPORT_SYM(lkbde_intr_cb_register);
LKM_EXPORT_SYM(lkbde_get_dev_pci_info);
LKM_EXPORT_SYM(lkbde_iproc_version_get);
#ifdef INCLUDE_SRAM_DMA
LKM_EXPORT_SYM(lkbde_get_sram_dma_info);
#endif /* INCLUDE_SRAM_DMA */
