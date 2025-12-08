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
 * Linux User BDE Helper Module
 */
#include <gmodule.h>
#include <mpool.h>
#include <linux-bde.h>

#include <soc/devids.h>
#include <linux/jiffies.h>
#include "linux-user-bde.h"

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,12,0)
#include <linux/uaccess.h>
#endif

MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("User BDE Helper Module");
MODULE_LICENSE("GPL");

/* CMIC/CMICe defines */
#define CMIC_IRQ_STAT                   0x00000144
#define CMIC_IRQ_MASK                   0x00000148
#define CMIC_IRQ_MASK_1                 0x0000006C
#define CMIC_IRQ_MASK_2                 0x00000070

/* CMICm defines */
#define CMIC_CMCx_IRQ_STAT0_OFFSET(x)                    (0x31400 + (0x1000 * x))
#define CMIC_CMCx_IRQ_STAT1_OFFSET(x)                    (0x31404 + (0x1000 * x))
#define CMIC_CMCx_IRQ_STAT2_OFFSET(x)                    (0x31408 + (0x1000 * x))
#define CMIC_CMCx_IRQ_STAT3_OFFSET(x)                    (0x3140c + (0x1000 * x))
#define CMIC_CMCx_IRQ_STAT4_OFFSET(x)                    (0x31410 + (0x1000 * x))

#define CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(x)               (0x31414 + (0x1000 * x))
#define CMIC_CMCx_PCIE_IRQ_MASK1_OFFSET(x)               (0x31418 + (0x1000 * x))
#define CMIC_CMCx_PCIE_IRQ_MASK2_OFFSET(x)               (0x3141c + (0x1000 * x))
#define CMIC_CMCx_PCIE_IRQ_MASK3_OFFSET(x)               (0x31420 + (0x1000 * x))
#define CMIC_CMCx_PCIE_IRQ_MASK4_OFFSET(x)               (0x31424 + (0x1000 * x))

/* CMICd defines */
#define CMIC_CMCx_IRQ_STAT5_OFFSET(x)                    (0x314b0 + (0x1000 * x))
#define CMIC_CMCx_IRQ_STAT6_OFFSET(x)                    (0x314b4 + (0x1000 * x))
#define CMIC_CMCx_PCIE_IRQ_MASK5_OFFSET(x)               (0x314b8 + (0x1000 * x))
#define CMIC_CMCx_PCIE_IRQ_MASK6_OFFSET(x)               (0x314bc + (0x1000 * x))
#define CMIC_CMCx_UC0_IRQ_MASK5_OFFSET(x)                (0x314c0 + (0x1000 * x))
#define CMIC_CMCx_UC0_IRQ_MASK6_OFFSET(x)                (0x314c4 + (0x1000 * x))

#define CMIC_CMCx_UC0_IRQ_MASK0_OFFSET(x)                (0x31428 + (0x1000 * x))
#define CMIC_CMCx_UC0_IRQ_MASK1_OFFSET(x)                (0x3142c + (0x1000 * x))
#define CMIC_CMCx_UC0_IRQ_MASK2_OFFSET(x)                (0x31430 + (0x1000 * x))
#define CMIC_CMCx_UC0_IRQ_MASK3_OFFSET(x)                (0x31434 + (0x1000 * x))
#define CMIC_CMCx_UC0_IRQ_MASK4_OFFSET(x)                (0x31438 + (0x1000 * x))

/* CMICX defines */
#define INTC_INTR_REG_NUM               (8)
#define PAXB_INTRCLR_DELAY_REG_NUM      (16)







#define PAXB_0_PAXB_IC_INTRCLR_0       (0x180123a0)
#define PAXB_0_PAXB_IC_INTRCLR_1       (0x180123a4)
#define PAXB_0_PAXB_IC_INTRCLR_MODE_0  (0x180123a8)
#define PAXB_0_PAXB_IC_INTRCLR_MODE_1  (0x180123ac)
#define PAXB_0_PAXB_INTR_STATUS             (0x18012f38)
#define PAXB_0_PAXB_IC_INTR_PACING_CTRL     (0x18012398)
#define PAXB_0_PAXB_INTRCLR_DELAY_UNIT      (0x1801239c)
#define PAXB_0_PAXB_IC_INTRCLR_DELAY_REG0   (0x180123b0)
#define PAXB_0_PCIE_ERROR_STATUS            (0x18012024)

#define HX5_PAXB_0_PAXB_IC_INTRCLR_0   (0x102303a0)
#define HX5_PAXB_0_PAXB_IC_INTRCLR_1   (0x102303a4)

#define HX5_PAXB_0_PAXB_IC_INTRCLR_MODE_0   (0x102303a8)
#define HX5_PAXB_0_PAXB_IC_INTRCLR_MODE_1   (0x102303ac)
#define HX5_PAXB_0_PAXB_INTR_STATUS         (0x10230f38)
#define HX5_PAXB_0_PAXB_IC_INTR_PACING_CTRL (0x10230398)
#define HX5_PAXB_0_PAXB_INTRCLR_DELAY_UNIT  (0x1023039c)
#define HX5_PAXB_0_PAXB_IC_INTRCLR_DELAY_REG0 (0x102303b0)
#define HX5_PAXB_0_PCIE_ERROR_STATUS          (0x10230024)

#define PAXB_0_PAXB_IC_INTRCLR_DELAY_BASE     (PAXB_0_PAXB_IC_INTRCLR_DELAY_REG0)
#define HX5_PAXB_0_PAXB_IC_INTRCLR_DELAY_BASE (HX5_PAXB_0_PAXB_IC_INTRCLR_DELAY_REG0)

#define INTC_INTR_ENABLE_REG0          (0x180130f0)
#define INTC_INTR_STATUS_REG0          (0x18013190)
#define INTC_INTR_RAW_STATUS_REG0      (0x18013140)

#define INTC_INTR_ENABLE_BASE           (INTC_INTR_ENABLE_REG0)
#define INTC_INTR_STATUS_BASE           (INTC_INTR_STATUS_REG0)
#define INTC_INTR_RAW_STATUS_BASE       (INTC_INTR_RAW_STATUS_REG0)

/** CMICX Gen2 defines*/
#define CMICX_GEN2_INTC_INTR_REG_NUM               (10)
#define CMICX_GEN2_PAXB_0_PAXB_IC_INTRCLR_0       (0x0292C3A0)
#define CMICX_GEN2_PAXB_0_PAXB_IC_INTRCLR_1       (0x0292C3A4)
#define CMICX_GEN2_PAXB_0_PAXB_IC_INTRCLR_MODE_0  (0x0292C3A8)
#define CMICX_GEN2_PAXB_0_PAXB_IC_INTRCLR_MODE_1  (0x0292C3AC)
#define CMICX_GEN2_PAXB_0_PAXB_INTR_STATUS             (0x0292CF38)
#define CMICX_GEN2_PAXB_0_PAXB_IC_INTR_PACING_CTRL     (0x0292C398)
#define CMICX_GEN2_PAXB_0_PAXB_INTRCLR_DELAY_UNIT      (0x0292C39c)
#define CMICX_GEN2_PAXB_0_PAXB_IC_INTRCLR_DELAY_REG0   (0x0292C3b0)
#define CMICX_GEN2_PAXB_0_PAXB_IC_INTRCLR_DELAY_BASE     (CMICX_GEN2_PAXB_0_PAXB_IC_INTRCLR_DELAY_REG0)
#define CMICX_GEN2_PAXB_0_PCIE_ERROR_STATUS            (0x0292C024)
#define CMICX_GEN2_PAXB_0_INTC_INTR_ENABLE_REG0          (0x0292d150)
#define CMICX_GEN2_PAXB_0_INTC_INTR_STATUS_REG0          (0x0292D1A0)
#define CMICX_GEN2_PAXB_0_INTC_INTR_RAW_STATUS_REG0      (0x0292D178)
#define CMICX_GEN2_PAXB_0_INTC_SET_INTR_ENABLE_REG0      (0x0292d100)
#define CMICX_GEN2_PAXB_0_INTC_CLEAR_INTR_ENABLE_REG0    (0x0292d128)

#define CMICX_GEN2_PAXB_0_SW_PROG_INTR_ENABLE      (0x292cf8c)
#define CMICX_GEN2_PAXB_0_SW_PROG_INTR_CLR        (0x292cf98)

#define CMICX_GEN2_PAXB_0_INTC_INTR_ENABLE_BASE           (CMICX_GEN2_PAXB_0_INTC_INTR_ENABLE_REG0)
#define CMICX_GEN2_PAXB_0_INTC_INTR_STATUS_BASE           (CMICX_GEN2_PAXB_0_INTC_INTR_STATUS_REG0)
#define CMICX_GEN2_PAXB_0_INTC_INTR_RAW_STATUS_BASE       (CMICX_GEN2_PAXB_0_INTC_INTR_RAW_STATUS_REG0)
#define CMICX_GEN2_PAXB_0_INTC_INTR_SET_ENABLE_BASE       (CMICX_GEN2_PAXB_0_INTC_SET_INTR_ENABLE_REG0)
#define CMICX_GEN2_PAXB_0_INTC_INTR_CLEAR_ENABLE_BASE     (CMICX_GEN2_PAXB_0_INTC_CLEAR_INTR_ENABLE_REG0)

#define CMICX_GEN2_INTC_PDMA_INTR_REG_IND_0             5
#define CMICX_GEN2_INTC_PDMA_INTR_REG_IND_1             6



#define HX5_INTC_INTR_ENABLE_REG0          (0x102310f0)
#define HX5_INTC_INTR_STATUS_REG0          (0x10231190)
#define HX5_INTC_INTR_RAW_STATUS_REG0      (0x10231140)

#define HX5_INTC_INTR_ENABLE_BASE          (HX5_INTC_INTR_ENABLE_REG0)
#define HX5_INTC_INTR_STATUS_BASE          (HX5_INTC_INTR_STATUS_REG0)
#define HX5_INTC_INTR_RAW_STATUS_BASE      (HX5_INTC_INTR_RAW_STATUS_REG0)

#define HX5_IHOST_GICD_ISENABLERN_0        (0x10781100)
#define HX5_IHOST_GICD_ISENABLERN_1        (0x10781104)
#define HX5_IHOST_GICD_ICENABLERN_1        (0x10781184)
#define HX5_IHOST_GICD_ICENABLERN_8        (0x107811a0)
#define HX5_IHOST_GICD_ISPENDRN_8          (0x10781220)
/* Offset between ISENABLERN_1 and ICENABLERN_1 in 4-bytes */
#define HX5_IHOST_IRQ_MASK_OFFSET          0x20
/* Offset between ISENABLERN_1 and ISPENDRN_1 in 4-bytes */
#define HX5_IHOST_IRQ_PEND_OFFSET          0x40
#define HX5_IHOST_INTR_MAP_NUM             (HX5_IHOST_GICD_ISPENDRN_8 - HX5_IHOST_GICD_ISENABLERN_0)
#define HX5_IHOST_INTR_STATUS_MAP_NUM      (INTC_INTR_REG_NUM * (sizeof(uint32)))
#define IRQ_BIT(intr)                      (intr % (sizeof(uint32)*8))
#define IRQ_MASK_INDEX(intr)               (intr / (sizeof(uint32)*8))
#define P19_SW_PROG_INTR_IRQ               69
#define HX5_SW_PROG_INTR_IRQ               73
#define INTR_SW_PROG_INTR_BITPOS           (1 << IRQ_BIT(ihost_sw_prog_intr_num))
#define INTC_SW_PROG_INTR_REG_IND          IRQ_MASK_INDEX(ihost_sw_prog_intr_num)
#define HX5_CHIP_INTR_LOW_PRIORITY         119
#define INTR_LOW_PRIORITY_BITPOS           (1 << IRQ_BIT(HX5_CHIP_INTR_LOW_PRIORITY))
#define INTC_LOW_PRIORITY_INTR_REG_IND     IRQ_MASK_INDEX(HX5_CHIP_INTR_LOW_PRIORITY)
#define INTC_PDMA_INTR_REG_IND             4

#define IPROC_READ(d, reg, v) \
        (v = user_bde->iproc_read(d, reg))
#define IPROC_WRITE(d, reg, v) \
        (user_bde->iproc_write(d, reg, v))

#define IHOST_READ_INTR(d, reg, v) \
        (v = readl((reg)))
#define IHOST_WRITE_INTR(d, reg, v) \
        (writel((v), (reg)))

/* Allow override of default CMICm CMC */
#ifndef BDE_CMICM_PCIE_CMC
#define BDE_CMICM_PCIE_CMC              0
#endif

/* Allow override of default CMICm CMC */
#ifndef BDE_CMICD_PCIE_CMC
#define BDE_CMICD_PCIE_CMC              0
#endif

/* Defines used to distinguish CMICe from CMICm */
#define CMICE_DEV_REV_ID                (0x178 / sizeof(uint32))

#ifdef BCM_DNX3_SUPPORT
#ifndef NEED_CMICX_GEN2_INTERRUPT
#define NEED_CMICX_GEN2_INTERRUPT
#endif
#endif
#ifdef BCM_DNXF3_SUPPORT
#ifndef NEED_CMICX_GEN2_INTERRUPT
#define NEED_CMICX_GEN2_INTERRUPT
#endif
#endif

static uint32 *ihost_intr_status_base = NULL;
static uint32 *ihost_intr_enable_base = NULL;
static int ihost_sw_prog_intr_num = HX5_SW_PROG_INTR_IRQ;

/* Module parameter for Interruptible timeout */
static int intr_timeout = 0;
LKM_MOD_PARAM(intr_timeout, "i", int, (S_IRUGO | S_IWUSR));
MODULE_PARM_DESC(intr_timeout,
"Interruptible wait timeout in milliseconds for Interrupt to be triggered.");

static ulong intr_count = 0;
LKM_MOD_PARAM(intr_count, "intr_count", ulong, (S_IRUGO | S_IWUSR));
MODULE_PARM_DESC(intr_count,
"Interrupt count provides information about the number of times the ISR is called.");

static ulong intr_timeout_count = 0;
LKM_MOD_PARAM(intr_timeout_count, "intr_timeout_count", ulong, (S_IRUGO | S_IWUSR));
MODULE_PARM_DESC(intr_timeout_count,
"Interrupt timeout count provides information about the number of times the interrupt wait is timeed out.");

/* Debug output */
static int debug;
LKM_MOD_PARAM(debug, "i", int, (S_IRUGO | S_IWUSR));
MODULE_PARM_DESC(debug,
"Set debug level (default 0).");

#if defined(BCM_DNX3_SUPPORT) || defined(BCM_DNXF3_SUPPORT)
/* Force mask interrupts before PCIe remove */
static int force_mask_irq = 0;
LKM_MOD_PARAM(force_mask_irq, "i", int, (S_IRUGO | S_IWUSR));
MODULE_PARM_DESC(force_mask_irq, "Force mask interrupts when pcie remove (default 0)");
#endif

/* Periodically check for pending interrupts when interrupt handler is idle */
static int intr_pending_check = 1;
LKM_MOD_PARAM(intr_pending_check, "i", int, (S_IRUGO | S_IWUSR));
MODULE_PARM_DESC(intr_pending_check,
"Check for pending interrupts periodically (default 1).");


static ibde_t *user_bde = NULL;

typedef void (*isr_f)(void *);

typedef struct _intr_regs_s {
    uint32 intc_intr_status_base;
    uint32 intc_intr_enable_base;
    uint32 intc_intr_set_enable_base;
    uint32 intc_intr_clear_enable_base;
    uint32 intc_intr_raw_status_base;
    uint32 intc_intr_clear_0;
    uint32 intc_intr_clear_1;
    uint32 intc_intr_clear_mode_0;
    uint32 intc_intr_clear_mode_1;
    uint32 intc_intr_status;
    uint32 intc_intr_pacing_ctrl;
    uint32 intc_intr_clear_delay_unit;
    uint32 intc_intr_clear_delay_base;
    uint32 intc_intr_pcie_err_status;
    uint32 intc_intr_nof_regs;
} _intr_regs_t;

typedef struct bde_ctrl_s {
    uint32 dev_type;
    int irq;
    atomic_t enabled;
    int devid;
    isr_f isr;
    uint32 *ba;
    uint32 inst;   /* the resource/instance index in _bde_inst_resource[] */
    int edk_irq_enabled;
    _intr_regs_t intr_regs;
} bde_ctrl_t;

#define VALID_DEVICE(_n) (_n < LINUX_BDE_MAX_DEVICES)

static bde_ctrl_t _devices[LINUX_BDE_MAX_DEVICES];

static wait_queue_head_t _ether_interrupt_wq;
static atomic_t _ether_interrupt_has_taken_place = ATOMIC_INIT(0);

/*
 * Multiple instance resource data structure.
 * To keep the DMA resource per instance.
 * And track the DMA pool usage.
 */
static int _bde_multi_inst = 0;

#define MAX_UC_CORES LINUX_BDE_MAX_IPROC_UC_CORES

/* Structure to hold info about interrupts handled by EDK */
typedef struct {
    uint32_t timer_intrc_offset; /* Timer interrupts INTC offset */
    uint32_t timer_intrc_mask; /* Timer interrupts mask */
    uint32_t sw_intr_intrc_offset; /* SW Programmable Interrupt's offset */
    uint32_t sw_intr_intrc_mask; /* SW interrupt's mask */
    uint32_t sw_intr_src_bitmap; /* SW interrupt's bitmask to navigate ICFG registers */
    uint32_t sw_intr_icfg_reg[MAX_UC_CORES]; /* ICFG registers for each core */
} bde_edk_intr_t;

typedef struct {
    int             is_active;  /* Is the instance active */
    unsigned int    dma_offset; /* offset of the instance's DMA memory in the DMA buffer pool */
    unsigned int    dma_size; /* size of the instance's DMA memory (in the DMA buffer pool) */
    wait_queue_head_t intr_wq;
    wait_queue_head_t edk_intr_wq;
    atomic_t intr;
    atomic_t edk_intr;
    bde_edk_intr_t edk_irqs;
} bde_inst_resource_t;

/* This array contains information for SDK instance, the index in the array is the instance ID */
static bde_inst_resource_t _bde_inst_resource[LINUX_BDE_MAX_DEVICES];
/*
 * Lock used to protect changes to _bde_inst_resource
 */
static spinlock_t bde_resource_lock;


typedef struct {
    phys_addr_t  cpu_pbase; /* CPU physical base address of the DMA pool */
    phys_addr_t  dma_pbase; /* Bus base address of the DMA pool */
    uint32  total_size; /* Total size of the pool in MB */
    uint32  offset; /* Current offset of the pool in MB */
}_dma_pool_t;

static _dma_pool_t _dma_pool;

#define ONE_MB      (1024 * 1024)

#ifdef KEYSTONE
/*
 * Enforce PCIE transaction ordering. Commit the write transaction.
 */

#define SSOC_WRITEL(val, addr)                  \
            do {                                \
                writel((val), (addr));          \
                __asm__ __volatile__("sync");   \
            } while(0)

#else

#define SSOC_WRITEL(val, addr) \
            writel((val), (addr))

#endif
/*
 * Function: _interrupt
 *
 * Purpose:
 *    Interrupt Handler.
 *    Mask all interrupts on device and wake up interrupt
 *    thread. It is assumed that the interrupt thread unmasks
 *    interrupts again when interrupt handling is complete.
 * Parameters:
 *    ctrl - BDE control structure for this device.
 * Returns:
 *    Nothing
 */
static void
_cmic_interrupt(void *arg)
{
    bde_ctrl_t *ctrl = (bde_ctrl_t *)arg;
    int d;
    uint32_t mask = 0, stat, imask = 0, fmask = 0;
    bde_inst_resource_t *res;

    d = (((uint8 *)ctrl - (uint8 *)_devices) / sizeof (bde_ctrl_t));
    res = &_bde_inst_resource[ctrl->inst];

    /* Check for secondary interrupt handler */
    if (lkbde_irq_mask_get(d, &mask, &fmask) < 0) {
        fmask = 0;
    }

    if (fmask != 0) {
        imask = mask & ~fmask;
        /* Check for pending user mode interrupts */
        stat = user_bde->read(d, CMIC_IRQ_STAT);
        if ((stat & imask) == 0) {
            /* All handled in kernel mode */
            lkbde_irq_mask_set(d, CMIC_IRQ_MASK, imask, 0);
            return;
        }
    }

    lkbde_irq_mask_set(d, CMIC_IRQ_MASK, 0, 0);

    atomic_set(&res->intr, 1);

#ifdef BDE_LINUX_NON_INTERRUPTIBLE
    wake_up(&res->intr_wq);
#else
    wake_up_interruptible(&res->intr_wq);
#endif
}

static void
dump_interrupt_regs(bde_ctrl_t *ctrl , int dev)
{
    int ind;
    uint32_t val;

    if (debug >= 2) {
        gprintk("Interrupt timeout count = %lu\n", intr_timeout_count);
        gprintk("Interrupt count = %lu\n", intr_count);
        for (ind = 0; ind < ctrl->intr_regs.intc_intr_nof_regs; ind++) {
            IPROC_READ(dev, ctrl->intr_regs.intc_intr_status_base + 4 * ind, val);
            gprintk("INTC_INTR_STATUS_REG_%d = 0x%x\n", ind, val);
            IPROC_READ(dev, ctrl->intr_regs.intc_intr_raw_status_base + 4 * ind, val);
            gprintk("INTC_INTR_RAW_STATUS_REG_%d = 0x%x\n", ind, val);
            IPROC_READ(dev, ctrl->intr_regs.intc_intr_enable_base + 4 * ind, val);
            gprintk("INTC_INTR_ENABLE_REG_%d = 0x%x\n", ind, val);
        }
        /* Dump PAXB Register */
        IPROC_READ(dev, ctrl->intr_regs.intc_intr_status, val);
        gprintk("PAXB_0_PAXB_INTR_STATUS = 0x%x\n", val);
        IPROC_READ(dev, ctrl->intr_regs.intc_intr_pacing_ctrl, val);
        gprintk("PAXB_0_PAXB_IC_INTR_PACING_CTRL = 0x%x\n", val);
        IPROC_READ(dev, ctrl->intr_regs.intc_intr_clear_delay_unit, val);
        gprintk("PAXB_0_PAXB_INTRCLR_DELAY_UNIT = 0x%x\n", val);
        IPROC_READ(dev, ctrl->intr_regs.intc_intr_pcie_err_status, val);
        gprintk("PAXB_0_PCIE_ERROR_STATUS = 0x%x\n", val);

        for (ind = 0; ind < PAXB_INTRCLR_DELAY_REG_NUM; ind++) {
            IPROC_READ(dev, ctrl->intr_regs.intc_intr_clear_delay_base + 4 * ind, val);
            gprintk("PAXB_0_PAXB_IC_INTRCLR_DELAY_REG_%d = 0x%x\n", ind, val);
        }
    }
    /* Clear interrupt enable registers */
    for (ind = 0; ind < ctrl->intr_regs.intc_intr_nof_regs; ind++) {
        IPROC_WRITE(dev, ctrl->intr_regs.intc_intr_enable_base + 4 * ind, 0);
    }
}

#ifdef BDE_EDK_SUPPORT
static int
_cmicx_edk_interrupt_check(bde_ctrl_t *ctrl, int d)
{
    bde_inst_resource_t *res;
    uint32 stat, mask = 0, bitmap = 0, reg;
    int idx;

    res = &_bde_inst_resource[ctrl->inst];
    bitmap = res->edk_irqs.sw_intr_src_bitmap;

    /* Explicitly reading raw_status so as to not clear the status on read */
    IPROC_READ(d, ctrl->intr_regs.intc_intr_raw_status_base +
        (res->edk_irqs.sw_intr_intrc_offset * 4), stat);
    /* Check whether Software Programmable Interrupt is set */
    if (stat & res->edk_irqs.sw_intr_intrc_mask) {
        for (idx = 0; (bitmap && (idx < MAX_UC_CORES)); idx++) {
            if (bitmap & 1) {
                IPROC_READ(d, res->edk_irqs.sw_intr_icfg_reg[idx], stat);
                mask |= (stat & 1) << idx;
            }
            bitmap = (bitmap >> 1);
        }
        if (mask) {
            /* Explicitly reading status to clear the interrupt status on read
             * as it is clear that it is EDK's interrupt */
            IPROC_READ(d, ctrl->intr_regs.intc_intr_raw_status_base +
                (res->edk_irqs.sw_intr_intrc_offset * 4), stat);
            if (stat == res->edk_irqs.sw_intr_intrc_mask) {
                IPROC_READ(d, ctrl->intr_regs.intc_intr_status_base +
                    (res->edk_irqs.sw_intr_intrc_offset * 4), stat);
            }
            /* Disable SWI and return */
            reg = ctrl->intr_regs.intc_intr_enable_base +
                (res->edk_irqs.sw_intr_intrc_offset * 4);
            IPROC_READ(d, reg, stat);
            stat &= ~res->edk_irqs.sw_intr_intrc_mask;
            IPROC_WRITE(d, reg, stat);
            return 1;
        }
    }

    /* EDK uses timer interrupt as watchdog to indicate the
     * firmware has crashed */
    IPROC_READ(d, ctrl->intr_regs.intc_intr_raw_status_base +
        (res->edk_irqs.timer_intrc_offset * 4), stat);
    if (stat & res->edk_irqs.timer_intrc_mask) {
        /* Disable WD timer interrupt and return */
        reg = ctrl->intr_regs.intc_intr_enable_base +
            (res->edk_irqs.timer_intrc_offset * 4);
        IPROC_READ(d, reg, stat);
        stat &= ~res->edk_irqs.timer_intrc_mask;
        IPROC_WRITE(d, reg, stat);
        return 1;
    }
    return 0;
}
#endif

static int
_cmicx_interrupt_prepare(bde_ctrl_t *ctrl)
{
    int d, ind, ret = 0;
    uint32 stat, iena, mask, fmask;
    uint32 intrs = 0;

    d = (((uint8 *)ctrl - (uint8 *)_devices) / sizeof (bde_ctrl_t));

#ifdef BDE_EDK_SUPPORT
    /* Check for interrupts meant for EDK and return without wasting time */
    if (ctrl->edk_irq_enabled) {
        ret = _cmicx_edk_interrupt_check(ctrl, d);
        if (ret) {
            return ret;
        }
    }
#endif

    if (ctrl->dev_type & BDE_PCI_DEV_TYPE) {
        IPROC_READ(d, ctrl->intr_regs.intc_intr_clear_mode_0, stat);
        /* Clear MSI interrupts immediately to prevent spurious interrupts */
        if (stat == 0) {
            IPROC_WRITE(d, ctrl->intr_regs.intc_intr_clear_0, 0xFFFFFFFF);
            IPROC_WRITE(d, ctrl->intr_regs.intc_intr_clear_1, 0xFFFFFFFF);
        }
    }

    lkbde_irq_mask_get(d, &mask, &fmask);

    if (fmask) {
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            IHOST_READ_INTR(d, ihost_intr_status_base + INTC_PDMA_INTR_REG_IND, stat);
            IHOST_READ_INTR(d, ihost_intr_enable_base + INTC_PDMA_INTR_REG_IND, iena);
        } else {
            IPROC_READ(d, ctrl->intr_regs.intc_intr_status_base + 4 * INTC_PDMA_INTR_REG_IND, stat);
            IPROC_READ(d, ctrl->intr_regs.intc_intr_enable_base + 4 * INTC_PDMA_INTR_REG_IND, iena);
        }
        intrs = stat & iena;
        if (intrs && (intrs == (intrs & fmask))) {
            if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
                IHOST_WRITE_INTR(d, ihost_intr_enable_base + INTC_PDMA_INTR_REG_IND +
                    HX5_IHOST_IRQ_MASK_OFFSET, ~0);
            } else {
                IPROC_WRITE(d, ctrl->intr_regs.intc_intr_enable_base + 4 * INTC_PDMA_INTR_REG_IND, 0);
            }

            for (ind = 0; ind < INTC_INTR_REG_NUM; ind++) {
                if (ind == INTC_PDMA_INTR_REG_IND) {
                    continue;
                }
                if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
                    if (ind < INTC_SW_PROG_INTR_REG_IND) {
                        continue;
                    }
                    if (ind == INTC_SW_PROG_INTR_REG_IND) {
                        IHOST_READ_INTR(d, ihost_intr_enable_base + ind + HX5_IHOST_IRQ_PEND_OFFSET, stat);
                        stat &= INTR_SW_PROG_INTR_BITPOS;
                    } else {
                        IHOST_READ_INTR(d, ihost_intr_status_base + ind, stat);
                        if (ind == INTC_LOW_PRIORITY_INTR_REG_IND) {
                            stat &= INTR_LOW_PRIORITY_BITPOS;
                        }
                    }
                } else {
                    IPROC_READ(d, ctrl->intr_regs.intc_intr_status_base + 4 * ind, stat);
                    IPROC_READ(d, ctrl->intr_regs.intc_intr_enable_base + 4 * ind, iena);
                }
                if (stat & iena) {
                    break;
                }
            }
            /* No pending interrupts */
            if (ind >= INTC_INTR_REG_NUM) {
                return -1;
            }
        }
    }

    /* Disable all interrupts.. Re-enable unserviced interrupts later
     * So as to avoid getting new interrupts until the user level driver
     * enumerates the interrupts to be serviced
     */

    for (ind = 0; ind < INTC_INTR_REG_NUM; ind++) {
        if (fmask && (intrs == (intrs & fmask)) && ind == INTC_PDMA_INTR_REG_IND) {
            continue;
        }
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            if (ind < INTC_SW_PROG_INTR_REG_IND) {
                continue;
            }
            if (ind == INTC_SW_PROG_INTR_REG_IND) {
                IHOST_WRITE_INTR(d, ihost_intr_enable_base + INTC_SW_PROG_INTR_REG_IND +
                    HX5_IHOST_IRQ_MASK_OFFSET, INTR_SW_PROG_INTR_BITPOS);
            } else if (ind == INTC_LOW_PRIORITY_INTR_REG_IND) {
                IHOST_WRITE_INTR(d, ihost_intr_enable_base + INTC_LOW_PRIORITY_INTR_REG_IND +
                    HX5_IHOST_IRQ_MASK_OFFSET, INTR_LOW_PRIORITY_BITPOS);
            } else {
                IHOST_WRITE_INTR(d, ihost_intr_enable_base + ind +
                    HX5_IHOST_IRQ_MASK_OFFSET, ~0);
            }
        } else {
            IPROC_WRITE(d, ctrl->intr_regs.intc_intr_enable_base + (4 * ind), 0);
        }
    }

    return ret;
}

static int
_cmicx_interrupt_pending(void *data)
{
    int d, ind;
    uint32 stat, iena;
    bde_ctrl_t *ctrl = (bde_ctrl_t *)data;

    if (ctrl->dev_type & BDE_PCI_DEV_TYPE) {

        d = (((uint8 *)ctrl - (uint8 *)_devices) / sizeof (bde_ctrl_t));

        for (ind = 0; ind < ctrl->intr_regs.intc_intr_nof_regs; ind++) {
            IPROC_READ(d, ctrl->intr_regs.intc_intr_status_base + 4 * ind, stat);
            IPROC_READ(d, ctrl->intr_regs.intc_intr_enable_base + 4 * ind, iena);
            if (debug >= 2) {
                gprintk("INTC_INTR_STATUS_REG_%d = 0x%x\n", ind, stat);
                gprintk("INTC_INTR_ENABLE_REG_%d = 0x%x\n", ind, iena);
            }
            if (stat & iena) {
                return 1;
            }
        }
    }

    /* No pending interrupts */
    return 0;
}

static void
_cmicx_interrupt(void *arg)
{
    bde_ctrl_t *ctrl = (bde_ctrl_t *)arg;
    bde_inst_resource_t *res;
    int ret;

    intr_count++;

    res = &_bde_inst_resource[ctrl->inst];
    ret = _cmicx_interrupt_prepare(ctrl);
    if (ret < 0) {
        return;
    } else if (ret > 0) {
        /* Notify */
        atomic_set(&res->edk_intr, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
        wake_up(&res->edk_intr_wq);
#else
        wake_up_interruptible(&res->edk_intr_wq);
#endif
    } else {
        /* Notify */
        atomic_set(&res->intr, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
        wake_up(&res->intr_wq);
#else
        wake_up_interruptible(&res->intr_wq);
#endif
    }
}

#ifdef NEED_CMICX_GEN2_INTERRUPT
static void
_cmicx_gen2_interrupt(void *arg)
{
    bde_ctrl_t *ctrl = (bde_ctrl_t *)arg;
    int d, ind ;
    uint32 stat, iena, mask, fmask;
    int active_interrupts = 0;
    bde_inst_resource_t *res;
    uint32 intrs = 0, dev_state = 0;

    intr_count++;
    d = (((uint8 *)ctrl - (uint8 *)_devices) / sizeof (bde_ctrl_t));

    (void)lkbde_dev_state_get(d, &dev_state);
    if (dev_state == BDE_DEV_STATE_REMOVED) {
        /* Return directly if PCIe device was removed */
        return;
    }

    res = &_bde_inst_resource[ctrl->inst];

    /** Get MSI clear mode, auto clear or SW clear, must be configure same for 64 MSI/MSIx vectors */
    IPROC_READ(d, ctrl->intr_regs.intc_intr_clear_mode_0, stat);
    /* Clear MSI interrupts immediately to prevent spurious interrupts */
    if (stat == 0) {
        IPROC_WRITE(d, ctrl->intr_regs.intc_intr_clear_0, 0xFFFFFFFF);
        IPROC_WRITE(d, ctrl->intr_regs.intc_intr_clear_1, 0xFFFFFFFF);
    }

    lkbde_irq_mask_get(d, &mask, &fmask);

    if (fmask) {
        /*
         * Packet DMA occupies 8 - 31 bits on IPROC_IRQ_BASE5, KNET only uses 8 - 23 bits for CMC0 currently
         * Prioritizing IPROC_IRQ_BASE5 when ONLY Packet DMA interrupts are raised
         */
        IPROC_READ(d, ctrl->intr_regs.intc_intr_status_base + 4 * CMICX_GEN2_INTC_PDMA_INTR_REG_IND_0, stat);
        IPROC_READ(d, ctrl->intr_regs.intc_intr_enable_base + 4 * CMICX_GEN2_INTC_PDMA_INTR_REG_IND_0, iena);
        intrs = stat & iena;
        if (intrs && (intrs == (intrs & fmask))) {
            IPROC_WRITE(d, ctrl->intr_regs.intc_intr_clear_enable_base + (4 * CMICX_GEN2_INTC_PDMA_INTR_REG_IND_0), 0xFFFFFFFF);
            IPROC_WRITE(d, ctrl->intr_regs.intc_intr_set_enable_base + (4 * CMICX_GEN2_INTC_PDMA_INTR_REG_IND_0), 0x0);

            for (ind = 0; ind < CMICX_GEN2_INTC_INTR_REG_NUM; ind++) {
                if (ind == CMICX_GEN2_INTC_PDMA_INTR_REG_IND_0) {
                    continue;
                }
                IPROC_READ(d, ctrl->intr_regs.intc_intr_status_base + 4 * ind, stat);
                IPROC_READ(d, ctrl->intr_regs.intc_intr_enable_base + 4 * ind, iena);
                if (stat & iena) {
                    active_interrupts = 1;
                    break;
                }
            }
            if (!active_interrupts) {
                /** only KNET interrupt asserted */
                return;
            }
        } else if (intrs) {
            active_interrupts = 1;
        }
    }

    /** check left interrupts */
    if (!active_interrupts) {
        for (ind = 0; ind < CMICX_GEN2_INTC_INTR_REG_NUM; ind++) {
            /** skip packetDMA */
            if (fmask && (ind == CMICX_GEN2_INTC_PDMA_INTR_REG_IND_0)) {
                continue;
            }
            IPROC_READ(d, ctrl->intr_regs.intc_intr_status_base + 4 * ind, stat);
            if (stat == 0) {
                continue;
            }
            IPROC_READ(d, ctrl->intr_regs.intc_intr_enable_base + 4 * ind, iena);
            if (stat & iena) {
                active_interrupts = 1;
                break;
            }
        }
    }

    /* No active interrupts to service */
    if (!active_interrupts) {
        return;
    }

    /* Disable all interrupts.. Re-enable unserviced interrupts later
     * So as to avoid getting new interrupts until the user level driver
     * enumerates the interrupts to be serviced
     */
    for (ind = 0; ind < CMICX_GEN2_INTC_INTR_REG_NUM; ind++) {
        if (fmask && (intrs == (intrs & fmask)) && ind == CMICX_GEN2_INTC_PDMA_INTR_REG_IND_0) {
            continue;
        }
        IPROC_WRITE(d, ctrl->intr_regs.intc_intr_clear_enable_base + (4 * ind), 0xFFFFFFFF);
        IPROC_WRITE(d, ctrl->intr_regs.intc_intr_set_enable_base + (4 * ind), 0);
    }

    /* Notify */
    atomic_set(&res->intr, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
    wake_up(&res->intr_wq);
#else
    wake_up_interruptible(&res->intr_wq);
#endif
}
#endif /* NEED_CMICX_GEN2_INTERRUPT */

static void
_cmicm_interrupt(void *arg)
{
    bde_ctrl_t *ctrl = (bde_ctrl_t *)arg;
    int d;
    int cmc = BDE_CMICM_PCIE_CMC;
    uint32 stat, mask = 0, fmask = 0, imask = 0;
    bde_inst_resource_t *res;

    d = (((uint8 *)ctrl - (uint8 *)_devices) / sizeof (bde_ctrl_t));
    res = &_bde_inst_resource[ctrl->inst];

    lkbde_irq_mask_get(d, &mask, &fmask);

    while (fmask) {
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT0_OFFSET(cmc));
        imask = mask & ~fmask;
        if (stat & imask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT1_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK1_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK1_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT2_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK2_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK2_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT3_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK3_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK3_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT4_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK4_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK4_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        return;
    }

    if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
        lkbde_irq_mask_set(d, CMIC_CMCx_UC0_IRQ_MASK0_OFFSET(cmc), 0, 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK1_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK2_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK3_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK4_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK0_OFFSET(1), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK0_OFFSET(2), 0);
    }
    else {
        lkbde_irq_mask_set(d, CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(cmc), 0, 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK1_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK2_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK3_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK4_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(1), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(2), 0);
    }
    atomic_set(&res->intr, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
    wake_up(&res->intr_wq);
#else
    wake_up_interruptible(&res->intr_wq);
#endif
}

/* some device has cmc0 only */
static void
_cmicd_cmc0_interrupt(void *arg)
{
    bde_ctrl_t *ctrl = (bde_ctrl_t *)arg;
    int d;
    int cmc = 0;
    uint32 stat, mask = 0, fmask = 0, imask = 0;
    bde_inst_resource_t *res;

    d = (((uint8 *)ctrl - (uint8 *)_devices) / sizeof (bde_ctrl_t));
    res = &_bde_inst_resource[ctrl->inst];
    lkbde_irq_mask_get(d, &mask, &fmask);

    while (fmask) {
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT0_OFFSET(cmc));
        imask = mask & ~fmask;
        if (stat & imask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT1_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK1_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK1_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT2_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK2_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK2_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT3_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK3_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK3_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT4_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK4_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK4_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT5_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK5_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK5_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT6_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK6_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK6_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        return;
    }

    if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
        lkbde_irq_mask_set(d, CMIC_CMCx_UC0_IRQ_MASK0_OFFSET(cmc), 0, 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK1_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK2_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK3_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK4_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK5_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK6_OFFSET(cmc), 0);
    } else {
        lkbde_irq_mask_set(d, CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(cmc), 0, 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK1_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK2_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK3_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK4_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK5_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK6_OFFSET(cmc), 0);
    }
    atomic_set(&res->intr, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
    wake_up(&res->intr_wq);
#else
    wake_up_interruptible(&res->intr_wq);
#endif
}

static void
_cmicd_interrupt(void *arg)
{
    bde_ctrl_t *ctrl = (bde_ctrl_t *)arg;
    int d;
    int cmc = BDE_CMICD_PCIE_CMC;
    uint32 stat, mask = 0, fmask = 0, imask = 0;
    bde_inst_resource_t *res;

    d = (((uint8 *)ctrl - (uint8 *)_devices) / sizeof (bde_ctrl_t));
    res = &_bde_inst_resource[ctrl->inst];
    lkbde_irq_mask_get(d, &mask, &fmask);

    while (fmask) {
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT0_OFFSET(cmc));
        imask = mask & ~fmask;
        if (stat & imask) {
            break;
        }
        /** Check if there are interrupts other than PacketIO interrupts on CMC1 */
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT0_OFFSET(1));
        imask = mask & ~fmask;
        if (stat & imask) {
            break;
        }
        /** Check if there are interrupts other than PacketIO interrupts on CMC2 */
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT0_OFFSET(2));
        imask = mask & ~fmask;
        if (stat & imask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT1_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK1_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK1_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT2_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK2_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK2_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT3_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK3_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK3_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT4_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK4_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK4_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT5_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK5_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK5_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        stat = user_bde->read(d, CMIC_CMCx_IRQ_STAT6_OFFSET(cmc));
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            mask = user_bde->read(d, CMIC_CMCx_UC0_IRQ_MASK6_OFFSET(cmc));
        } else {
            mask = user_bde->read(d, CMIC_CMCx_PCIE_IRQ_MASK6_OFFSET(cmc));
        }
        if (stat & mask) {
            break;
        }
        return;
    }

    if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
        lkbde_irq_mask_set(d, CMIC_CMCx_UC0_IRQ_MASK0_OFFSET(cmc), 0, 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK1_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK2_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK3_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK4_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK5_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK6_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK0_OFFSET(1), 0);
        user_bde->write(d, CMIC_CMCx_UC0_IRQ_MASK0_OFFSET(2), 0);
    } else {
        lkbde_irq_mask_set(d, CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(cmc), 0, 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK1_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK2_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK3_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK4_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK5_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK6_OFFSET(cmc), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(1), 0);
        user_bde->write(d, CMIC_CMCx_PCIE_IRQ_MASK0_OFFSET(2), 0);
    }
    atomic_set(&res->intr, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
    wake_up(&res->intr_wq);
#else
    wake_up_interruptible(&res->intr_wq);
#endif
}


/* The actual interrupt handler of ethernet devices */
static void
_ether_interrupt(void *arg)
{
    bde_ctrl_t *ctrl = (bde_ctrl_t *)arg;

    SSOC_WRITEL(0, ctrl->ba + 0x024/4);

    atomic_set(&_ether_interrupt_has_taken_place, 1);
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
    wake_up(&_ether_interrupt_wq);
#else
    wake_up_interruptible(&_ether_interrupt_wq);
#endif
}


static struct _intr_mode_s {
    isr_f isr;
    const char *name;
} _intr_mode[] = {
    { _cmic_interrupt,              "CMIC/CMICe" },
    { _cmicm_interrupt,             "CMICm" },
    { _cmicd_interrupt,             "CMICd" },
    { _cmicd_cmc0_interrupt,        "CMICd CMC0" },
    { _cmicx_interrupt,             "CMICx" },
#ifdef NEED_CMICX_GEN2_INTERRUPT
    { _cmicx_gen2_interrupt,        "CMICx Gen2" },
#endif
    { NULL, NULL }
};

static const char *
_intr_mode_str(void *isr)
{
    int imx;

    imx = 0;
    while (_intr_mode[imx].isr != NULL) {
        if (isr == _intr_mode[imx].isr) {
            return _intr_mode[imx].name;
        }
        imx++;
    }
    return NULL;
}

static void
_intr_regs_init(bde_ctrl_t *ctrl, int flag)
{
    if (flag == 1) {
        ctrl->intr_regs.intc_intr_status_base = HX5_INTC_INTR_STATUS_BASE;
        ctrl->intr_regs.intc_intr_enable_base = HX5_INTC_INTR_ENABLE_BASE;
        ctrl->intr_regs.intc_intr_raw_status_base = HX5_INTC_INTR_RAW_STATUS_BASE;
        ctrl->intr_regs.intc_intr_clear_0 = HX5_PAXB_0_PAXB_IC_INTRCLR_0;
        ctrl->intr_regs.intc_intr_clear_1 = HX5_PAXB_0_PAXB_IC_INTRCLR_1;
        ctrl->intr_regs.intc_intr_clear_mode_0 = HX5_PAXB_0_PAXB_IC_INTRCLR_MODE_0;
        ctrl->intr_regs.intc_intr_clear_mode_1 = HX5_PAXB_0_PAXB_IC_INTRCLR_MODE_1;
        ctrl->intr_regs.intc_intr_status = HX5_PAXB_0_PAXB_INTR_STATUS;
        ctrl->intr_regs.intc_intr_pacing_ctrl = HX5_PAXB_0_PAXB_IC_INTR_PACING_CTRL;
        ctrl->intr_regs.intc_intr_clear_delay_unit = HX5_PAXB_0_PAXB_INTRCLR_DELAY_UNIT;
        ctrl->intr_regs.intc_intr_clear_delay_base = HX5_PAXB_0_PAXB_IC_INTRCLR_DELAY_BASE;
        ctrl->intr_regs.intc_intr_pcie_err_status = HX5_PAXB_0_PCIE_ERROR_STATUS;
        ctrl->intr_regs.intc_intr_nof_regs = INTC_INTR_REG_NUM;
        if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
            switch (ctrl->devid) {
                case BCM53650_DEVICE_ID:
                case BCM53651_DEVICE_ID:
                case BCM53652_DEVICE_ID:
                case BCM53653_DEVICE_ID:
                case BCM53654_DEVICE_ID:
                case BCM56390_DEVICE_ID:
                case BCM56391_DEVICE_ID:
                case BCM56392_DEVICE_ID:
                    ihost_sw_prog_intr_num = P19_SW_PROG_INTR_IRQ;
                    break;
                default:
                    break;
            }
        }
    } else if (flag == 2){
        ctrl->intr_regs.intc_intr_status_base = CMICX_GEN2_PAXB_0_INTC_INTR_STATUS_BASE;
        ctrl->intr_regs.intc_intr_raw_status_base = CMICX_GEN2_PAXB_0_INTC_INTR_RAW_STATUS_BASE;
        ctrl->intr_regs.intc_intr_enable_base = CMICX_GEN2_PAXB_0_INTC_INTR_ENABLE_BASE;
        ctrl->intr_regs.intc_intr_set_enable_base = CMICX_GEN2_PAXB_0_INTC_INTR_SET_ENABLE_BASE;
        ctrl->intr_regs.intc_intr_clear_enable_base = CMICX_GEN2_PAXB_0_INTC_INTR_CLEAR_ENABLE_BASE;
        ctrl->intr_regs.intc_intr_clear_0 = CMICX_GEN2_PAXB_0_PAXB_IC_INTRCLR_0;
        ctrl->intr_regs.intc_intr_clear_1 = CMICX_GEN2_PAXB_0_PAXB_IC_INTRCLR_1;
        ctrl->intr_regs.intc_intr_clear_mode_0 = CMICX_GEN2_PAXB_0_PAXB_IC_INTRCLR_MODE_0;
        ctrl->intr_regs.intc_intr_clear_mode_1 = CMICX_GEN2_PAXB_0_PAXB_IC_INTRCLR_MODE_1;
        ctrl->intr_regs.intc_intr_status = CMICX_GEN2_PAXB_0_PAXB_INTR_STATUS;
        ctrl->intr_regs.intc_intr_pacing_ctrl = CMICX_GEN2_PAXB_0_PAXB_IC_INTR_PACING_CTRL;
        ctrl->intr_regs.intc_intr_clear_delay_unit = CMICX_GEN2_PAXB_0_PAXB_INTRCLR_DELAY_UNIT;
        ctrl->intr_regs.intc_intr_clear_delay_base = CMICX_GEN2_PAXB_0_PAXB_IC_INTRCLR_DELAY_BASE;
        ctrl->intr_regs.intc_intr_pcie_err_status = CMICX_GEN2_PAXB_0_PCIE_ERROR_STATUS;
        ctrl->intr_regs.intc_intr_nof_regs = CMICX_GEN2_INTC_INTR_REG_NUM;
    } else {
        ctrl->intr_regs.intc_intr_status_base = INTC_INTR_STATUS_BASE;
        ctrl->intr_regs.intc_intr_raw_status_base = INTC_INTR_RAW_STATUS_BASE;
        ctrl->intr_regs.intc_intr_enable_base = INTC_INTR_ENABLE_BASE;
        ctrl->intr_regs.intc_intr_clear_0 = PAXB_0_PAXB_IC_INTRCLR_0;
        ctrl->intr_regs.intc_intr_clear_1 = PAXB_0_PAXB_IC_INTRCLR_1;
        ctrl->intr_regs.intc_intr_clear_mode_0 = PAXB_0_PAXB_IC_INTRCLR_MODE_0;
        ctrl->intr_regs.intc_intr_clear_mode_1 = PAXB_0_PAXB_IC_INTRCLR_MODE_1;
        ctrl->intr_regs.intc_intr_status = PAXB_0_PAXB_INTR_STATUS;
        ctrl->intr_regs.intc_intr_pacing_ctrl = PAXB_0_PAXB_IC_INTR_PACING_CTRL;
        ctrl->intr_regs.intc_intr_clear_delay_unit = PAXB_0_PAXB_INTRCLR_DELAY_UNIT;
        ctrl->intr_regs.intc_intr_clear_delay_base = PAXB_0_PAXB_IC_INTRCLR_DELAY_BASE;
        ctrl->intr_regs.intc_intr_pcie_err_status = PAXB_0_PCIE_ERROR_STATUS;
        ctrl->intr_regs.intc_intr_nof_regs = INTC_INTR_REG_NUM;
    }
}

static void
_devices_init(int d)
{
    bde_ctrl_t *ctrl;
    uint32 ver;
    uint16 device_id_mask = 0xFFF0;
    uint16 device_id;
    uint32 state = 0;

    (void)lkbde_dev_state_get(d, &state);
    if (state == BDE_DEV_STATE_REMOVED) {
        return;
    }

    ctrl = &_devices[d];
    /* Initialize our control info */
    ctrl->dev_type = user_bde->get_dev_type(d);
    ctrl->devid = user_bde->get_dev(d)->device;
    ctrl->inst = 0;

    if (BDE_DEV_MEM_MAPPED(ctrl->dev_type)) {
        atomic_set(&ctrl->enabled, 0);
        ctrl->ba = lkbde_get_dev_virt(d);
    }
    if (ctrl->dev_type & BDE_SWITCH_DEV_TYPE) {
        switch (user_bde->get_dev(d)->device) {
        case BCM53540_DEVICE_ID:
        case BCM53547_DEVICE_ID:
        case BCM53548_DEVICE_ID:
        case BCM53549_DEVICE_ID:
            ctrl->isr = _cmicd_cmc0_interrupt;
            break;
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
        case BCM88770_DEVICE_ID:
        case BCM88773_DEVICE_ID:
        case BCM88774_DEVICE_ID:
        case BCM88775_DEVICE_ID:
        case BCM88776_DEVICE_ID:
        case BCM88777_DEVICE_ID:
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
        case BCM88272_DEVICE_ID:
        case BCM88273_DEVICE_ID:
        case BCM88274_DEVICE_ID:
        case BCM88278_DEVICE_ID:
        case BCM88279_DEVICE_ID:
        case BCM88950_DEVICE_ID:
        case BCM88953_DEVICE_ID:
        case BCM88954_DEVICE_ID:
        case BCM88955_DEVICE_ID:
        case BCM88956_DEVICE_ID:
        case BCM88772_DEVICE_ID:
        case BCM88952_DEVICE_ID:
            ctrl->isr = _cmicd_interrupt;
            break;
        case BCM56370_DEVICE_ID:
        case BCM56371_DEVICE_ID:
        case BCM56372_DEVICE_ID:
        case BCM56374_DEVICE_ID:
        case BCM56375_DEVICE_ID:
        case BCM56376_DEVICE_ID:
        case BCM56377_DEVICE_ID:
        case BCM56577_DEVICE_ID:
        case BCM56578_DEVICE_ID:
        case BCM56579_DEVICE_ID:
        case BCM56273_DEVICE_ID:
        case BCM56274_DEVICE_ID:
        case BCM56275_DEVICE_ID:
        case BCM56276_DEVICE_ID:
        case BCM56277_DEVICE_ID:
        case BCM56278_DEVICE_ID:
        case BCM56279_DEVICE_ID:
        case BCM56575_DEVICE_ID:
        case BCM56175_DEVICE_ID:
        case BCM56176_DEVICE_ID:
        case BCM53642_DEVICE_ID:
        case BCM53650_DEVICE_ID:
        case BCM53651_DEVICE_ID:
        case BCM53652_DEVICE_ID:
        case BCM53653_DEVICE_ID:
        case BCM53654_DEVICE_ID:
        case BCM56390_DEVICE_ID:
        case BCM56391_DEVICE_ID:
        case BCM56392_DEVICE_ID:
            ctrl->isr = _cmicx_interrupt;
            if (ctrl->dev_type & BDE_AXI_DEV_TYPE) {
                if (!ihost_intr_enable_base) {
                    ihost_intr_enable_base = (uint32_t *)ioremap(HX5_IHOST_GICD_ISENABLERN_1,
                                                                 HX5_IHOST_INTR_MAP_NUM);
                }
                if (!ihost_intr_status_base) {
                    ihost_intr_status_base = (uint32_t *)ioremap(HX5_INTC_INTR_RAW_STATUS_REG0,
                                                                 HX5_IHOST_INTR_STATUS_MAP_NUM);
                }
            }
            _intr_regs_init(ctrl, 1);
            break;
        default:
            /* Get CMIC version */
            if (user_bde->get_cmic_ver(d, &ver) != 0) {
                ver = -1;
            }
            device_id = ctrl->devid & device_id_mask;
            /* TH/TH+/TH2 should use cmicd interrupt handler */
            if (BCM56960_DEVICE_ID == device_id ||
                BCM56930_DEVICE_ID == device_id ||
                BCM56970_DEVICE_ID == device_id) {
                ctrl->isr = _cmicd_interrupt;
            }
            /* check if version is CMICX */
            else if (ver == 0x04) {
                ctrl->isr = _cmicx_interrupt;
                _intr_regs_init(ctrl, 0);
            } else {
                ctrl->isr = _cmic_interrupt;
                if ((ctrl->dev_type & BDE_256K_REG_SPACE) &&
#ifdef BCM_PETRA_SUPPORT 
                    ctrl->devid != 0x1234 &&
#endif
                    readl(ctrl->ba + CMICE_DEV_REV_ID) == 0) {
                    ctrl->isr = _cmicm_interrupt;
                }
            }
            break;
        }

        /* configure interrupts for DNX devices using iproc >=14 */
#if defined(BCM_DNXF_SUPPORT) || defined(BCM_DNX_SUPPORT)
        switch (user_bde->get_dev(d)->device & DNXC_DEVID_FAMILY_MASK) {
#ifdef BCM_DNX_SUPPORT
          case JERICHO2_DEVICE_ID:
          case J2C_DEVICE_ID:
          case J2C_2ND_DEVICE_ID:
          case Q2A_DEVICE_ID:
          case Q2U_DEVICE_ID:
          case Q2N_DEVICE_ID:
          case J2P_DEVICE_ID:
          case J2X_DEVICE_ID:
#endif
#ifdef BCM_DNXF_SUPPORT
          case  BCM88790_DEVICE_ID:
#endif
            ctrl->isr = _cmicx_interrupt;
            _intr_regs_init(ctrl, 0);
            break;

#ifdef BCM_DNX3_SUPPORT
          case JERICHO3_DEVICE_ID:
          case J3AI_DEVICE_ID:

          case Q3D_DEVICE_ID:
#ifdef BCM_Q3A_SUPPORT
          case Q3A_DEVICE_ID:
          case Q3U_DEVICE_ID:
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
#ifdef BCM_DNXF3_SUPPORT
          case  RAMON2_DEVICE_ID:
          case  RAMON3_DEVICE_ID:
#endif
#ifdef BCM_DNXFE_SUPPORT
#ifdef BCM_RAMON_4_SUPPORT
          case RAMON4_DEVICE_ID:
#endif
#endif
#if defined(BCM_DNX3_SUPPORT) || defined(BCM_DNXF3_SUPPORT)
            ctrl->isr = _cmicx_gen2_interrupt;
            _intr_regs_init(ctrl, 2);
            break;
#endif
        }
#endif /* defined(BCM_DNXF_SUPPORT) || defined(BCM_DNX_SUPPORT) */

        if (_intr_mode_str(ctrl->isr) == NULL) {
            gprintk("Warning: Unknown interrupt mode\n");
        }
    }
}
/*
 * Function: _init
 *
 * Purpose:
 *    Module initialization.
 *    Attaches to kernel BDE.
 * Parameters:
 *    None
 * Returns:
 *    Always 0
 */
static int
_init(void)
{
    int i, nof_devices;
    phys_addr_t cpu_pbase, dma_pbase;
    ssize_t dmasize;
    bde_inst_resource_t *res;
    uint32 *bitmap_ptr;

    /* Connect to the kernel bde */
    if ((linux_bde_create(NULL, &user_bde) < 0) || user_bde == NULL) {
        return -ENODEV;
    }
    spin_lock_init(&bde_resource_lock);

    init_waitqueue_head(&_ether_interrupt_wq);

    lkbde_get_dma_info(&cpu_pbase, &dma_pbase, &dmasize);

    memset(&_dma_pool, 0, sizeof(_dma_pool));
    _dma_pool.cpu_pbase = cpu_pbase;
    _dma_pool.dma_pbase = dma_pbase;
    _dma_pool.total_size = dmasize / ONE_MB;

    memset(_devices, 0, sizeof(_devices));

    /* Use _bde_inst_resource[0] as the default resource */
    memset(_bde_inst_resource, 0, sizeof(_bde_inst_resource));
    res = &_bde_inst_resource[0];
    res->dma_offset = 0;
    res->dma_size = _dma_pool.total_size;
    init_waitqueue_head(&res->intr_wq);
    init_waitqueue_head(&res->edk_intr_wq);
    atomic_set(&res->intr, 0);
    atomic_set(&res->edk_intr, 0);

    ihost_intr_enable_base = NULL;
    ihost_intr_status_base = NULL;

    nof_devices = user_bde->num_devices(BDE_ALL_DEVICES);
    /* for no BDE instances, mark the single SDK as controlling all devices */
    bitmap_ptr = *lkbde_get_inst_devs(0);
    for (i = nof_devices; i >=32; i -= 32) {
      *(bitmap_ptr++) = 0xffffffff;
    }
    *bitmap_ptr = (((uint32)1) << i) - 1;
    res->is_active = 1;

    for (i = 0; i < nof_devices; i++) { /* init all devices */
        _devices_init(i);
    }

    if (intr_timeout > 0) {
        gprintk("Interruptible wait timeout = %d msecs\n", intr_timeout);
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

    if (user_bde) {
        for (i = 0; i < user_bde->num_devices(BDE_ALL_DEVICES); i++) {
            if (atomic_read(&_devices[i].enabled) &&
                BDE_DEV_MEM_MAPPED(_devices[i].dev_type)) {

#if defined(BCM_DNX3_SUPPORT) || defined(BCM_DNXF3_SUPPORT)
                /** before disconnect interrupt, mask it */
                if (force_mask_irq) {
                    bde_ctrl_t *ctrl;
                    int ind;
                    switch (user_bde->get_dev(i)->device & DNXC_DEVID_FAMILY_MASK) {
#ifdef BCM_DNX3_SUPPORT
                        case JERICHO3_DEVICE_ID:
                        case J3AI_DEVICE_ID:
                        case Q3D_DEVICE_ID:
#ifdef BCM_Q3A_SUPPORT
                        case Q3A_DEVICE_ID:
                        case Q3U_DEVICE_ID:
#endif
#endif
#ifdef BCM_DNXF3_SUPPORT
                        case  RAMON2_DEVICE_ID:
                        case  RAMON3_DEVICE_ID:
#endif
                            if (debug >= 3) {
                                gprintk("force mask interrupts for device %d!\n", i);
                            }
                            ctrl = &_devices[i];
                            for (ind = 0; ind < ctrl->intr_regs.intc_intr_nof_regs; ind++) {
                                if (ctrl->intr_regs.intc_intr_clear_enable_base != 0) {
                                    /** clear interrupt */
                                    IPROC_WRITE(i, ctrl->intr_regs.intc_intr_clear_enable_base + (4 * ind), 0xFFFFFFFF);
                                    IPROC_WRITE(i, ctrl->intr_regs.intc_intr_set_enable_base + (4 * ind), 0);
                                } else if (ctrl->intr_regs.intc_intr_enable_base != 0) {
                                    IPROC_WRITE(i, ctrl->intr_regs.intc_intr_enable_base + (4 * ind), 0);
                                }
                            }
                            /* clear Software Programmable interrupt (PAXB_0_SW_PROG_INTR_CLR)*/
                            IPROC_WRITE(i, CMICX_GEN2_PAXB_0_SW_PROG_INTR_CLR, 0xFFFFFFFF);
                            /* disable Software Programmable interrupt (PAXB_0_SW_PROG_INTR_ENABLE)*/
                            IPROC_WRITE(i, CMICX_GEN2_PAXB_0_SW_PROG_INTR_ENABLE, 0x0);
                            break;

                       default:
                            if (debug >= 4) {
                                gprintk("device %x do not support force interrupt mask!\n", user_bde->get_dev(i)->device);
                            }
                           break;
                    }
                }
#endif /* defined(BCM_DNXF_SUPPORT) || defined(BCM_DNX_SUPPORT) */

                user_bde->interrupt_disconnect(i);
            }
            lkbde_dev_instid_set(i, BDE_DEV_INST_ID_INVALID);
        }
        linux_bde_destroy(user_bde);
        user_bde = NULL;
    }

    if (ihost_intr_enable_base) {
        iounmap(ihost_intr_enable_base);
        ihost_intr_enable_base = NULL;
    }
    if (ihost_intr_status_base) {
        iounmap(ihost_intr_status_base);
        ihost_intr_status_base = NULL;
    }

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
    int idx;
    const char *name;
    bde_inst_resource_t *res;
    uint32 state, instid;

    pprintf(m, "Broadcom Device Enumerator (%s)\n", LINUX_USER_BDE_NAME);
#ifdef LKM_BUILD_INFO
    pprintf(m, "%s\n", LKM_BUILD_INFO);
#endif
    for (idx = 0; idx < user_bde->num_devices(BDE_ALL_DEVICES); idx++) {
        name = _intr_mode_str(_devices[idx].isr);
        if (name == NULL) {
            name = "unknown";
        }
        pprintf(m, "\t%d: Interrupt mode  %s ",idx, name);
        (void)lkbde_dev_state_get(idx, &state);
        if (state == BDE_DEV_STATE_REMOVED) {
            pprintf(m, " Device REMOVED ! \n");
        } else {
            (void)lkbde_dev_instid_get(idx, &instid);
            if (instid != BDE_DEV_INST_ID_INVALID) {
                pprintf(m, "Inst id 0x%x\n",instid);
            } else {
                pprintf(m, "\n");
            }
        }
    }
    pprintf(m, "Instance resource \n");

    for (idx = 0; idx < user_bde->num_devices(BDE_ALL_DEVICES); idx++) {
        res = &_bde_inst_resource[idx];
        if (res->is_active) {
            linux_bde_device_bitmap_t* bitmap_p = lkbde_get_inst_devs(idx);
            pprintf(m, "\t%d: DMA offset %d size %d MB Dev mask 0x",
                    idx,
                    res->dma_offset,
                    res->dma_size);
            for (state = 0; state * 32 < user_bde->num_devices(BDE_ALL_DEVICES); ++state) {
                pprintf(m,"%.8x ", (*bitmap_p)[state]);
            }
            pprintf(m,"\n");
        }
    }

    return 0;
}

#ifdef BCM_INSTANCE_SUPPORT
/*
 * Allocate the DMA resource from DMA pool
 * Parameter :
 * dma_size (IN): allocate dma_size in MB
 * dma_offset (OUT): dma offset in MB
 */
static int
_dma_resource_alloc(unsigned int dma_size, unsigned int *dma_offset)
{
    uint32 left;

    left = _dma_pool.total_size - _dma_pool.offset;
    if (dma_size > left) {
        gprintk("ERROR: Run out the dma resource!\n");
        return -1;
    }
    *dma_offset = _dma_pool.offset;
    _dma_pool.offset += dma_size;
    return 0;
}
#endif

static int
_dma_resource_get(unsigned inst_id, phys_addr_t *cpu_pbase, phys_addr_t *dma_pbase, ssize_t* size)
{
    unsigned int dma_size = 0, dma_offset = 0;
    bde_inst_resource_t *res;

    spin_lock(&bde_resource_lock);
    if (inst_id >= user_bde->num_devices(BDE_ALL_DEVICES)) {
        gprintk("ERROR: requested DMA resources for an instance number out of range: %u\n", inst_id);
        spin_unlock(&bde_resource_lock);
        return -1;
    }
    res = &_bde_inst_resource[inst_id];

    if (res->is_active == 0) {
        *cpu_pbase = 0;
        *dma_pbase = 0;
        *size = 0;
        spin_unlock(&bde_resource_lock);
        return -2;
    }

    dma_size = res->dma_size;
    dma_offset = res->dma_offset;
    spin_unlock(&bde_resource_lock);

    *cpu_pbase = _dma_pool.cpu_pbase + dma_offset * ONE_MB;
    *dma_pbase = _dma_pool.dma_pbase + dma_offset * ONE_MB;
    *size = dma_size * ONE_MB;

    return 0;
}

#ifdef BCM_INSTANCE_SUPPORT
/*
 * Checks if we have the instance in _bde_inst_resource. If not, return LUBDE_SUCCESS==0 (considered a new instance).
 * If it exists with the same dmasize, return 1 (It is considered already in use)
 * Otherwise if the device with the same index of the resource, has resource/instance index 0, return LUBDE_SUCCESS==0. (error)
 * Otherwise return LUBDE_FAIL==-1 (It is considered to exist with a different dmasize).
 */
static int
_instance_validate(unsigned int inst_id, unsigned int dmasize, linux_bde_device_bitmap_t inst_devices)
{
    unsigned i;
    uint32 bits;
    bde_inst_resource_t *res = _bde_inst_resource + inst_id;
    linux_bde_device_bitmap_t* bitmap_p;

    if (inst_id >= user_bde->num_devices(BDE_ALL_DEVICES)) {
        gprintk("ERROR: instance number out of range: %u\n", inst_id);
        return LUBDE_FAIL;
    }

    if (res->is_active == 0) {

        return LUBDE_SUCCESS;
    }

    bitmap_p = lkbde_get_inst_devs(inst_id);
    for (i = 0; i < LINUX_BDE_NOF_DEVICE_BITMAP_WORDS; ++i) {
        bits = inst_devices[i] ^ (*bitmap_p)[i];
        if (bits != 0) {
            for (i *= 32; (bits & 1) == 0; bits >>= 1, ++i);
            gprintk("ERROR: existing instance number %u does not control the same devices, see device %u\n", inst_id, i);
            return LUBDE_FAIL;
        }
    }

    if (res->dma_size == dmasize) {
        return 1; /* For an existing same instance with the same DMA size, do nothing */
    }
    /* with a different DMS size */
    gprintk("ERROR: dma_size mismatch\n");
    return LUBDE_FAIL;
}
#endif

static int
_device_reprobe(void)
{
    int i;

    for (i = 0; i < user_bde->num_devices(BDE_ALL_DEVICES); i++) {
        if (_devices[i].devid == 0) {
            _devices_init(i);
        }
    }
    return 0;
}

#ifdef BCM_INSTANCE_SUPPORT
/*
 * Attach an SDK instance:
 * _device_reprobe();
 * Check If an instance with the same bitmap exists, if yes with the same dmasize, return, if yes a different dmasize return an error, if no:
 * Allocate DMA for the instance.
 * This loop finds the first free resource in _bde_inst_resource[] and configure it for the instance.
 * Store the resource/instance index in _bde_inst_resource for every device in the instance.
 */
static int
_instance_attach(unsigned int inst_id, unsigned int dma_size, linux_bde_device_bitmap_t inst_devices)
{
    unsigned int dma_offset;
    int i, exist;
    bde_inst_resource_t *res;
    uint32 previous_inst_id;

    /* Reprobe the system for hot-plugged device */
    _device_reprobe();

    if (debug >= 2) {
        gprintk("INFO: Request to attach to instance_id %u with dma size %d!\n", inst_id, dma_size);
    }

    spin_lock(&bde_resource_lock);

    /* If not in multi instance mode, move to the mode and fix the first instance that represented all devices */
    if (_bde_multi_inst == 0) {
        _bde_multi_inst = 1;
        _bde_inst_resource->is_active = 0;
        /*_bde_inst_resource->dev will not be used when _bde_inst_resource->is_active == 0 */
    }

    /* Validate the resource with inst_devices */
    exist = _instance_validate(inst_id, dma_size, inst_devices);

    if (exist == LUBDE_FAIL) {
        spin_unlock(&bde_resource_lock);
        return LUBDE_FAIL;
    }
    if (exist > 0) {
        if (debug >= 2) {
            gprintk("INFO: Already attached to instance_id %u with dma size %d!\n", inst_id, dma_size);
        }
        spin_unlock(&bde_resource_lock);
        return LUBDE_SUCCESS;
    }
    if (_dma_resource_alloc(dma_size, &dma_offset) < 0) {
        spin_unlock(&bde_resource_lock);
        return LUBDE_FAIL;
    }

    /* configure the instance ID resources */
    res = _bde_inst_resource + inst_id;
    res->is_active = 1;
    res->dma_offset = dma_offset;
    res->dma_size = dma_size;
    /* skip instance 0, WQ for instance 0 has been initialized in user_bde init, see _init() */
    if (inst_id != 0) {
        init_waitqueue_head(&res->intr_wq);
        init_waitqueue_head(&res->edk_intr_wq);
        atomic_set(&res->intr, 0);
        atomic_set(&res->edk_intr, 0);
    }
    memcpy(*lkbde_get_inst_devs(inst_id), inst_devices, sizeof(linux_bde_device_bitmap_t));

    /* store the resource/instance index in _bde_inst_resource for every device in the instance */
    for (i = 0; i < user_bde->num_devices(BDE_ALL_DEVICES); i++) {
        if (inst_devices[i / 32] & (1 << (i % 32))) {
            _devices[i].inst = inst_id;
            /* Pass the inst_id to the kernel BDE */
            if (lkbde_dev_instid_get(i, &previous_inst_id) == 0) { /* If the linux-kernel-bde inst_id is not set for the device, set it to the instance ID */
                if (previous_inst_id == BDE_DEV_INST_ID_INVALID) {
                    lkbde_dev_instid_set(i, inst_id);
                }
            } 
        }
    }
    spin_unlock(&bde_resource_lock);
    if (debug >= 2) {
        gprintk("INFO: Attached to instance_id %lu with dma size %d! SUCCESS\n", (unsigned long)inst_devices, dma_size);
    }

    return LUBDE_SUCCESS;
}
#endif /* BCM_INSTANCE_SUPPORT */

#ifdef BDE_EDK_SUPPORT
static int
_edk_instance_attach(unsigned int inst_id, unsigned int dma_size)
{
    ssize_t size = (ssize_t)dma_size;
    if (size) {
        lkbde_edk_get_dma_info(inst_id, NULL, NULL, &size);
        if (!size) {
            gprintk("Error: EDK Attached to instance_id %lu failed\n", (unsigned long)inst_id);
            return LUBDE_FAIL;
        }
    }
    return LUBDE_SUCCESS;
}
#endif

/*
 * Function: _ioctl
 *
 * Purpose:
 *    Handle IOCTL commands from user mode.
 * Parameters:
 *    cmd - IOCTL cmd
 *    arg - IOCTL parameters
 * Returns:
 *    0 on success, <0 on error
 */
static int
_ioctl(unsigned int cmd, unsigned long arg)
{
    lubde_ioctl_t io;
    phys_addr_t cpu_pbase, dma_pbase;
    ssize_t size;
    ssize_t total_size;
    const ibde_dev_t *bde_dev;
    int inst_id, idx, inst_i;
    bde_inst_resource_t *res;
    uint32_t *mapaddr;

    if (copy_from_user(&io, (void *)arg, sizeof(io))) {
        return -EFAULT;
    }

    io.rc = LUBDE_SUCCESS;

    switch(cmd) {
    case LUBDE_VERSION:
        io.d0 = KBDE_VERSION;
        break;
    case LUBDE_GET_NUM_DEVICES:
        io.d0 = user_bde->num_devices(io.dev);
        break;
    case LUBDE_GET_DEVICE:
        if (!VALID_DEVICE(io.dev)) {
            return -EINVAL;
        }
        bde_dev = user_bde->get_dev(io.dev);
        if (bde_dev) {
            io.d0 = bde_dev->device;
            io.d1 = bde_dev->rev;
            io.dx.dw[0] = bde_dev->dev_unique_id;
            if (BDE_DEV_MEM_MAPPED(_devices[io.dev].dev_type)) {
                /* Get physical address to map */
                io.d2 = lkbde_get_dev_phys(io.dev);
                io.d3 = lkbde_get_dev_phys_hi(io.dev);
            }
        } else {
            io.rc = LUBDE_FAIL;
        }
        break;
    case LUBDE_GET_DEVICE_TYPE:
        if (!VALID_DEVICE(io.dev)) {
            return -EINVAL;
        }
        io.d0 = _devices[io.dev].dev_type;
        break;
    case LUBDE_GET_BUS_FEATURES:
        user_bde->pci_bus_features(io.dev, (int *) &io.d0, (int *) &io.d1,
                                   (int *) &io.d2);
        break;
    case LUBDE_PCI_CONFIG_PUT32:
        if (!VALID_DEVICE(io.dev)) {
            return -EINVAL;
        }
        if (_devices[io.dev].dev_type & BDE_PCI_DEV_TYPE) {
            user_bde->pci_conf_write(io.dev, io.d0, io.d1);
        } else {
            io.rc = LUBDE_FAIL;
        }
        break;
    case LUBDE_PCI_CONFIG_GET32:
        if (!VALID_DEVICE(io.dev)) {
            return -EINVAL;
        }
        if (_devices[io.dev].dev_type & BDE_PCI_DEV_TYPE) {
            io.d0 = user_bde->pci_conf_read(io.dev, io.d0);
        } else {
            io.rc = LUBDE_FAIL;
        }
        break;
    case LUBDE_GET_DMA_INFO:
        inst_id = io.dev;
        if (_bde_multi_inst && (inst_id != BDE_DEV_INST_ID_INVALID)) {
            if (inst_id == BDE_DEV_INST_ID_GET_FREE) {
                /* Get the total DMA size */
                lkbde_get_dma_info(&cpu_pbase, &dma_pbase, &total_size);
                /* Decrease the total size for each allocated SDK instance */
                for (inst_i = user_bde->num_devices(BDE_ALL_DEVICES); inst_i > 0;)
                {
                    --inst_i;
                    if (_bde_inst_resource[inst_i].is_active && (_dma_resource_get(inst_i, &cpu_pbase, &dma_pbase, &size) == 0)) {
                        total_size -= size;
                    }
                }
                size = total_size;
            } else {
                if (_dma_resource_get(inst_id, &cpu_pbase, &dma_pbase, &size)) {
                    io.rc = LUBDE_FAIL;
                }
            }
        } else {
            lkbde_get_dma_info(&cpu_pbase, &dma_pbase, &size);
        }
#ifdef BDE_EDK_SUPPORT
    case LUBDE_GET_EDK_DMA_INFO:
        if (cmd == LUBDE_GET_EDK_DMA_INFO) {
            inst_id = io.dev;
            lkbde_edk_get_dma_info(inst_id, &cpu_pbase, &dma_pbase, &size);
        }
#endif
        io.d0 = dma_pbase;
        io.d1 = size;
        /* Optionally enable DMA mmap via /dev/linux-kernel-bde */
        io.d2 = USE_LINUX_BDE_MMAP;
        /* Get physical address for mmap */
        io.dx.dw[0] = cpu_pbase;
#ifdef PHYS_ADDRS_ARE_64BITS
        io.dx.dw[1] = cpu_pbase >> 32;
        io.d3 = dma_pbase >> 32;
#else
        io.dx.dw[1] = 0;
        io.d3 = 0;
#endif
        break;

#ifdef INCLUDE_SRAM_DMA
    case LUBDE_GET_DEV_DMA_INFO:
        if (!VALID_DEVICE(io.dev)) {
            return -EINVAL;
        }
        lkbde_get_sram_dma_info(io.dev, &io.dx.dw[1], &io.dx.dw[0]);
        if (io.dx.dw[0] == 0) {
            io.rc = LUBDE_FAIL;
        }
        break;
#endif /* INCLUDE_SRAM_DMA */

    case LUBDE_ENABLE_INTERRUPTS:
        if (!VALID_DEVICE(io.dev)) {
            return -EINVAL;
        }
        /*
         * Disable interrupt in case already enabled
         * This is to handle the use case where userspace
         * application gets killed abruptly.
         */
        if (atomic_read(&_devices[io.dev].enabled)) {
            if (debug >= 1) {
                gprintk("Interrupts already enabled, disable to cleanup\n");
            }
            user_bde->interrupt_disconnect(io.dev);
            atomic_set(&_devices[io.dev].enabled, 0);
        }
        if (_devices[io.dev].dev_type & BDE_SWITCH_DEV_TYPE) {
            if (_devices[io.dev].isr && !atomic_read(&_devices[io.dev].enabled)) {
                /* PCI/CMICX Devices */
                if ((_devices[io.dev].dev_type & BDE_PCI_DEV_TYPE) &&
                    (_devices[io.dev].isr == _cmicx_interrupt) &&
                    intr_pending_check) {
                    lkbde_intr_cb_register(io.dev,
                                           _cmicx_interrupt_pending,
                                           _devices+io.dev);
                }

                user_bde->interrupt_connect(io.dev,
                                            _devices[io.dev].isr,
                                            _devices+io.dev);
                atomic_set(&_devices[io.dev].enabled, 1);
            }
        } else {
            /* Process ethernet device interrupt */

            if (!atomic_read(&_devices[io.dev].enabled)) {
                user_bde->interrupt_connect(io.dev,
                                            _ether_interrupt,
                                            _devices+io.dev);
                atomic_set(&_devices[io.dev].enabled, 1);
            }
        }
        break;
    case LUBDE_DISABLE_INTERRUPTS:
        if (!VALID_DEVICE(io.dev)) {
            return -EINVAL;
        }
        if (atomic_read(&_devices[io.dev].enabled)) {
            user_bde->interrupt_disconnect(io.dev);
            atomic_set(&_devices[io.dev].enabled, 0);
        }
        break;
    case LUBDE_SET_EDK_INTERRUPTS:
        if (!VALID_DEVICE(io.dev)) {
            return -EINVAL;
        }
        res = &_bde_inst_resource[_devices[io.dev].inst];
        res->edk_irqs.timer_intrc_offset = io.d0;
        res->edk_irqs.timer_intrc_mask =  io.d1;
        res->edk_irqs.sw_intr_intrc_offset = io.d2;
        res->edk_irqs.sw_intr_intrc_mask = io.d3;
        res->edk_irqs.sw_intr_src_bitmap = io.dx.dw[0];
        for (idx = 0; idx < MAX_UC_CORES; idx++) {
            res->edk_irqs.sw_intr_icfg_reg[idx] = io.dx.dw[idx + 1];
        }
        break;
    case LUBDE_ENABLE_EDK_INTERRUPTS:
        if (!VALID_DEVICE(io.dev)) {
            return -EINVAL;
        }
        _devices[io.dev].edk_irq_enabled = 1;
        break;
    case LUBDE_DISABLE_EDK_INTERRUPTS:
        if (!VALID_DEVICE(io.dev)) {
            return -EINVAL;
        }
        _devices[io.dev].edk_irq_enabled = 0;
        break;
    case LUBDE_WAIT_FOR_INTERRUPT:
        if (!VALID_DEVICE(io.dev)) {
            return -EINVAL;
        }
        if (_devices[io.dev].dev_type & BDE_SWITCH_DEV_TYPE) {
            res = &_bde_inst_resource[_devices[io.dev].inst];
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
            wait_event_timeout(res->intr_wq,
                               atomic_read(&res->intr) != 0, 100);

#else
            /* CMICX Devices */
            if ((_devices[io.dev].dev_type & BDE_PCI_DEV_TYPE) &&
                (_devices[io.dev].isr == _cmicx_interrupt)  &&
                (intr_timeout > 0)) {
                unsigned long t_jiffies;
                int err=0;
                t_jiffies =  msecs_to_jiffies(intr_timeout);
                err = wait_event_interruptible_timeout(res->intr_wq,
                             atomic_read(&res->intr) != 0,
                             t_jiffies);
                /* Timeout happend and condition not set */
                if (err == 0) {
                    bde_ctrl_t *ctrl;
                    ctrl = &_devices[io.dev];
                    intr_timeout_count++;
                    if (debug >= 1) {
                        gprintk("Timeout happend and condition not set\n");
                    }
                    dump_interrupt_regs(ctrl, io.dev);
                } else if (err == -ERESTARTSYS) {
                    if (debug >= 1) {
                       gprintk("Interrupted by Signal\n");
                    }
                }
            } else {
                wait_event_interruptible(res->intr_wq,
                                      atomic_read(&res->intr) != 0);
            }
#endif

            /*
             * Even if we get multiple interrupts, we
             * only run the interrupt handler once.
             */
            atomic_set(&res->intr, 0);
        } else {
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
            wait_event_timeout(_ether_interrupt_wq,
                               atomic_read(&_ether_interrupt_has_taken_place) != 0, 100);
#else
            wait_event_interruptible(_ether_interrupt_wq,
                                     atomic_read(&_ether_interrupt_has_taken_place) != 0);

#endif
            /*
             * Even if we get multiple interrupts, we
             * only run the interrupt handler once.
             */
            atomic_set(&_ether_interrupt_has_taken_place, 0);
        }
        break;
   case LUBDE_WAIT_FOR_EDK_INTERRUPT:
        if (!VALID_DEVICE(io.dev)) {
            return -EINVAL;
        }
        res = &_bde_inst_resource[_devices[io.dev].inst];
#ifdef BDE_LINUX_NON_INTERRUPTIBLE
        wait_event_timeout(res->edk_intr_wq,
                           atomic_read(&res->edk_intr) != 0, 100);

#else
        /* CMICX Devices */
        if ((_devices[io.dev].dev_type & BDE_PCI_DEV_TYPE) &&
            (_devices[io.dev].isr == _cmicx_interrupt) &&
            (intr_timeout > 0)) {
            unsigned long t_jiffies;
            int err = 0;
            t_jiffies = msecs_to_jiffies(intr_timeout);
            err = wait_event_interruptible_timeout(res->edk_intr_wq,
                         atomic_read(&res->edk_intr) != 0, t_jiffies);
            /* Timeout happend and condition not set */
            if (err == 0) {
                bde_ctrl_t *ctrl;
                ctrl = &_devices[io.dev];
                intr_timeout_count++;
                if (debug >= 1) {
                    gprintk("EDK Interrrupt: Timeout happened\n");
                }
                dump_interrupt_regs(ctrl, io.dev);
            } else if (err == -ERESTARTSYS) {
                if (debug >= 1) {
                    gprintk("EDK Interrrupt: Interrupted by Signal\n");
                }
            }
        } else {
            wait_event_interruptible(res->edk_intr_wq, atomic_read(&res->edk_intr) != 0);
        }
#endif
        /* Even if we get multiple interrupts, we
         * only run the interrupt handler once. */
        atomic_set(&res->edk_intr, 0);
        break;
    case LUBDE_USLEEP:
    case LUBDE_UDELAY:
    case LUBDE_SEM_OP:
        return -EINVAL;
    case LUBDE_WRITE_IRQ_MASK:
        /* CMICx device */
        if (_devices[io.dev].isr == _cmicx_interrupt) {
            io.rc = lkbde_irq_mask_set(io.dev + LKBDE_IPROC_REG, io.d0, io.d1, 0);
        }
#ifdef NEED_CMICX_GEN2_INTERRUPT
        else if (_devices[io.dev].isr == _cmicx_gen2_interrupt)
        {
            io.rc = lkbde_irq_mask_set(io.dev | LKBDE_IPROC_REG, io.d0, io.d1, 0);
        }
#endif
        else {
            io.rc = lkbde_irq_mask_set(io.dev, io.d0, io.d1, 0);
        }
        break;
    case LUBDE_SPI_READ_REG:
        if (user_bde->spi_read(io.dev, io.d0, io.dx.buf, io.d1) == -1) {
            io.rc = LUBDE_FAIL;
        }
        break;
    case LUBDE_SPI_WRITE_REG:
        if (user_bde->spi_write(io.dev, io.d0, io.dx.buf, io.d1) == -1) {
            io.rc = LUBDE_FAIL;
        }
        break;
    case LUBDE_READ_REG_16BIT_BUS:
        io.d1 = user_bde->read(io.dev, io.d0);
        break;
    case LUBDE_WRITE_REG_16BIT_BUS:
        io.rc = user_bde->write(io.dev, io.d0, io.d1);
        break;
#ifdef BCM_SAND_SUPPORT
    case LUBDE_CPU_WRITE_REG:
    {
        if (lkbde_cpu_write(io.dev, io.d0, (uint32*)io.dx.buf) == -1) {
            io.rc = LUBDE_FAIL;
        }
        break;
    }
    case LUBDE_CPU_READ_REG:
    {
        if (lkbde_cpu_read(io.dev, io.d0, (uint32*)io.dx.buf) == -1) {
            io.rc = LUBDE_FAIL;
        }
        break;
    }
    case LUBDE_CPU_PCI_REGISTER:
    {
        if (lkbde_cpu_pci_register(io.dev) == -1) {
            io.rc = LUBDE_FAIL;
        }
        break;
    }
#endif
    case LUBDE_DEV_RESOURCE:
        if (!VALID_DEVICE(io.dev)) {
            return -EINVAL;
        }
        bde_dev = user_bde->get_dev(io.dev);
        if (bde_dev) {
            if (BDE_DEV_MEM_MAPPED(_devices[io.dev].dev_type)) {
                /* Get physical address to map */
                io.rc = lkbde_get_dev_resource(io.dev, io.d0,
                                               &io.d2, &io.d3, &io.d1);
            }
        } else {
            io.rc = LUBDE_FAIL;
        }
        break;
    case LUBDE_IPROC_READ_REG:
        if (!VALID_DEVICE(io.dev)) {
            return -EINVAL;
        }
        if (_devices[io.dev].dev_type & BDE_AXI_DEV_TYPE) {
            mapaddr = ioremap(io.d0, sizeof(uint32_t));
            if (mapaddr == NULL) {
                io.rc = LUBDE_FAIL;
                return -1;
            }
            io.d1 = readl(mapaddr);
            iounmap(mapaddr);
        } else {
            io.d1 = user_bde->iproc_read(io.dev, io.d0);
        }
        break;
    case LUBDE_IPROC_WRITE_REG:
        if (user_bde->iproc_write(io.dev, io.d0, io.d1) == -1) {
            io.rc = LUBDE_FAIL;
        }
        break;
#ifdef BDE_EDK_SUPPORT
    case LUBDE_ATTACH_EDK_INSTANCE:
        io.rc = _edk_instance_attach(io.d0, io.d1);
        break;
#endif
#ifdef BCM_INSTANCE_SUPPORT
    case LUBDE_ATTACH_INSTANCE:
        io.rc = _instance_attach(io.d0, io.d1, io.dx.dw);
        break;
#endif
    case LUBDE_GET_DEVICE_STATE:
        io.rc = lkbde_dev_state_get(io.dev, &io.d0);
        break;
    case LUBDE_REPROBE:
        io.rc = _device_reprobe();
        break;
    case LUBDE_GET_DEVICE_PCI_INFO:
        if (_devices[io.dev].dev_type & BDE_PCI_DEV_TYPE) {
            if (lkbde_get_dev_pci_info(io.dev, &io.d0, &io.d1, &io.d2) < 0)
                io.rc = LUBDE_FAIL;
        } else {
            io.rc = LUBDE_FAIL;
        }
        break;
    case LUBDE_BAR2_READ32:
        io.d1 = user_bde->read(io.dev, io.d0);
        break;
    case LUBDE_BAR2_WRITE32:
        if (user_bde->write(io.dev, io.d0, io.d1) == -1) {
            io.rc = LUBDE_FAIL;
        }
        break;
    case LUBDE_BAR2_READ64:
        {
            uint64_t data;
            data = user_bde->read64(io.dev, io.d0);
            io.d1 = (uint32_t)data;
            io.d2 = (uint32_t)(data >> 32);
        }
        break;
    case LUBDE_BAR2_WRITE64:
        {
            uint64_t val;
            val = ((uint64_t)io.d2 << 32) |  (uint64_t)io.d1;
            user_bde->write64(io.dev, io.d0, val);
        }
        break;

    default:
        gprintk("Error: Invalid ioctl (%08x)\n", cmd);
        io.rc = LUBDE_FAIL;
        break;
    }

    if (copy_to_user((void *)arg, &io, sizeof(io))) {
        return -EFAULT;
    }

    return 0;
}

/* Workaround for broken Busybox/PPC insmod */
static char _modname[] = LINUX_USER_BDE_NAME;

static gmodule_t _gmodule =
{
    .name = LINUX_USER_BDE_NAME,
    .major = LINUX_USER_BDE_MAJOR,
    .init = _init,
    .cleanup = _cleanup,
    .pprint = _pprint,
    .ioctl = _ioctl,
};

gmodule_t*
gmodule_get(void)
{
    _gmodule.name = _modname;
    return &_gmodule;
}
