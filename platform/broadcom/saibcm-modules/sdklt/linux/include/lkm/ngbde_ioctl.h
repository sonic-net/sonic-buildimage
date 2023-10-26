/*! \file ngbde_ioctl.h
 *
 * NGBDE device I/O control definitions.
 *
 * This file is intended for use in both kernel mode and user mode.
 *
 * IMPORTANT!
 * All shared structures must be properly 64-bit aligned.
 *
 */
/*
 * $Copyright: Copyright 2018-2022 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.$
 */

#ifndef NGBDE_IOCTL_H
#define NGBDE_IOCTL_H

#include <linux/types.h>
#include <linux/ioctl.h>

/*! Must be updated if backward compatibility is broken. */
#define NGBDE_IOC_VERSION       2

/*! LUBDE IOCTL command magic. */
#define NGBDE_IOC_MAGIC 'L'

/*!
 * \name IOCTL commands for the NGBDE kernel module.
 * \anchor NGBDE_IOC_xxx
 *
 * Note that we use __u64 for the IOCTL parameter size because
 * sizeof(void *) is different between 32-bit and 64-bit code, and we
 * need a 32-bit user mode application to generate the same IOCTL
 * command codes as a 64-bit kernel when using the _IOW macro.
 */

/*! \{ */

/*! Get kernel module information. */
#define NGBDE_IOC_MOD_INFO      _IOW(NGBDE_IOC_MAGIC, 0, __u64)

/*! Get information about registered devices. */
#define NGBDE_IOC_PROBE_INFO    _IOW(NGBDE_IOC_MAGIC, 1, __u64)

/*! Get detailed switch device information. */
#define NGBDE_IOC_DEV_INFO      _IOW(NGBDE_IOC_MAGIC, 2, __u64)

/*! Get a physical memory address associated with a switch device. */
#define NGBDE_IOC_PHYS_ADDR     _IOW(NGBDE_IOC_MAGIC, 3, __u64)

/*! Interrupt control command (see \ref NGBDE_ICTL_xxx). */
#define NGBDE_IOC_INTR_CTRL     _IOW(NGBDE_IOC_MAGIC, 4, __u64)

/*! Add interrupt status/mask register for kernel to control. */
#define NGBDE_IOC_IRQ_REG_ADD   _IOW(NGBDE_IOC_MAGIC, 5, __u64)

/*! Write to a shared interrupt mask register. */
#define NGBDE_IOC_IRQ_MASK_WR   _IOW(NGBDE_IOC_MAGIC, 6, __u64)

/*! Map device registers in kernel space. */
#define NGBDE_IOC_PIO_WIN_MAP   _IOW(NGBDE_IOC_MAGIC, 7, __u64)

/*! Map interrupt controller registers in kernel space. */
#define NGBDE_IOC_IIO_WIN_MAP   _IOW(NGBDE_IOC_MAGIC, 8, __u64)

/*! Map PCI bridge registers in kernel space. */
#define NGBDE_IOC_PAXB_WIN_MAP  _IOW(NGBDE_IOC_MAGIC, 9, __u64)

/*! Add interrupt ACK register for kernel to control. */
#define NGBDE_IOC_INTR_ACK_REG_ADD   _IOW(NGBDE_IOC_MAGIC, 10, __u64)

/*! \} */

/*! IOCTL command return code for success. */
#define NGBDE_IOC_SUCCESS       0

/*! IOCTL command return code for failure. */
#define NGBDE_IOC_FAIL          ((__u32)-1)

/*!
 * \name Device flags.
 * \anchor NGBDE_DEV_F_xxx
 */

/*! \{ */

/*! Message-signaled interrupts, PCI interrupts are operating in MSI mode. */
#define NGBDE_DEV_F_MSI         (1 << 0)

/*! \} */

/*!
 * \name Interrupt control commands.
 * \anchor NGBDE_ICTL_xxx
 */

/*! \{ */

/*! Connect interrupt handler. */
#define NGBDE_ICTL_INTR_CONN    0

/*! Disconnect interrupt handler. */
#define NGBDE_ICTL_INTR_DISC    1

/*! Wait for interrupt. */
#define NGBDE_ICTL_INTR_WAIT    2

/*! Force waiting thread to return. */
#define NGBDE_ICTL_INTR_STOP    3

/*! Clear list of interrupt status/mask registers. */
#define NGBDE_ICTL_REGS_CLR     4

/*! \} */

/*! Kernel module information. */
struct ngbde_ioc_mod_info_s {

    /*! IOCTL version used by kernel module. */
    __u16 version;
};

/*! Probing results. */
struct ngbde_ioc_probe_info_s {

    /*! Number of switch devices. */
    __u16 num_swdev;
};

/*! Device information. */
struct ngbde_ioc_dev_info_s {

    /*! Device type. */
    __u16 type;

    /*! Device flags (\ref NGBDE_DEV_F_xxx). */
    __u16 flags;

    /*! Vendor ID (typically the PCI vendor ID). */
    __u16 vendor_id;

    /*! Device ID (typically the PCI vendor ID). */
    __u16 device_id;

    /*! Device revision (typically the PCI device revision). */
    __u16 revision;

    /*! Device model (device-identification beyond PCI generic ID). */
    __u16 model;
};

/*!
 * \name I/O resource types.
 * \anchor NGBDE_IO_RSRC_xxx
 */

/*! \{ */

/*! Memory-mapped I/O. */
#define NGBDE_IO_RSRC_DEV_IO    0

/*! DMA memory pool. */
#define NGBDE_IO_RSRC_DMA_MEM   1

/*! DMA memory pool as mapped by IOMMU. */
#define NGBDE_IO_RSRC_DMA_BUS   2

/*! \} */

/*!
 * \brief Resource ID (IOCTL input).
 *
 * This structure is used to query a physical address resource in the
 * kernel module. The caller must provide a resource type (device I/O,
 * DMA memory, etc.) and a resource instance number (e.g. a PCI BAR
 * address will have multiple instances).
 *
 * Also see \ref ngbde_ioc_phys_addr_s.
 */
struct ngbde_ioc_rsrc_id_s {

    /*! Resource type (\ref NGBDE_IO_RSRC_xxx). */
    __u32 type;

    /*! Resource instance number. */
    __u32 inst;
};

/*!
 * \brief Physical device address.
 *
 * This structure is returned in response to the \ref
 * NGBDE_IOC_PHYS_ADDR command. The caller must identify the requested
 * physical address using the \ref ngbde_ioc_rsrc_id_s structure.
 */
struct ngbde_ioc_phys_addr_s {

    /*! Physical address. */
    __u64 addr;

    /*! Resource size (in bytes). */
    __u32 size;
};

/*! Interrupt control operation */
struct ngbde_ioc_intr_ctrl_s {

    /*! Interrupt instance for this device. */
    __u32 irq_num;

    /*! Interrupt control command. */
    __u32 cmd;
};

/*!
 * \name Interrupt register access flags.
 * \anchor NGBDE_DEV_IRQ_REG_F_xxx
 */

/*! \{ */

/*! IRQ register is of type "write 1 to clear". */
#define NGBDE_DEV_IRQ_REG_F_W1TC        (1 << 0)

/*! IRQ status register is a bitwise AND of mask and raw status. */
#define NGBDE_DEV_IRQ_REG_F_MASKED      (1 << 1)

/*!
 * Indicates that the kmask value is valid. This is mainly to
 * distinguish a mask value of zero from the mask value being
 * uninitialized, as this matters during a warm boot.
 */
#define NGBDE_DEV_IRQ_REG_F_KMASK       (1 << 2)

/*! \} */

/*! Add interrupt register information. */
struct ngbde_ioc_irq_reg_add_s {

    /*! Interrupt instance for this device. */
    __u32 irq_num;

    /*! Interrupt status register address offset. */
    __u32 status_reg;

    /*! Interrupt mask register address offset. */
    __u32 mask_reg;

    /*! Interrupt mask for interrupts handled by the kernel. */
    __u32 kmask;

    /*! Flags for special handling (\ref NGBDE_DEV_IRQ_REG_F_xxx). */
    __u32 flags;
};

/*!
 * \name Interrupt ACK register access flags.
 * \anchor NGBDE_DEV_INTR_ACK_F_xxx
 */

/*! \{ */

/*! ACK registers resides in PCI bridge I/O window. */
#define NGBDE_DEV_INTR_ACK_F_PAXB   (1 << 0)

/*! \} */

/*! Add interrupt ACK register information. */
struct ngbde_ioc_intr_ack_reg_add_s {

    /*! Interrupt instance for this device. */
    __u32 irq_num;

    /*! Interrupt ACK register address offset. */
    __u32 ack_reg;

    /*! Interrupt ACK value. */
    __u32 ack_val;

    /*! Flags to indicate ack_reg resides in PCI bridge window. */
    __u32 flags;
};

/*! Memory-mapped I/O window */
struct ngbde_ioc_pio_win_s {

    /*! Physical address */
    __u64 addr;

    /*! Resource size */
    __u32 size;
};

/*! Interrupt mask register write */
struct ngbde_ioc_irq_mask_wr_s {

    /*! Interrupt instance for this device. */
    __u32 irq_num;

    /*! Register offset. */
    __u32 offs;

    /*! Value to write. */
    __u32 val;
};

/*! IOCTL operation data. */
union ngbde_ioc_op_s {

    /*! Get kernel module information. */
    struct ngbde_ioc_mod_info_s mod_info;

    /*! Get information about registered devices. */
    struct ngbde_ioc_probe_info_s probe_info;

    /*! Get detailed switch device information. */
    struct ngbde_ioc_dev_info_s dev_info;

    /*! Resource ID (input). */
    struct ngbde_ioc_rsrc_id_s rsrc_id;

    /*! Get a physical memory address associated with a switch device. */
    struct ngbde_ioc_phys_addr_s phys_addr;

    /*! Interrupt control command (see \ref NGBDE_ICTL_xxx). */
    struct ngbde_ioc_intr_ctrl_s intr_ctrl;

    /*! Add interrupt status/mask register for kernel to control. */
    struct ngbde_ioc_irq_reg_add_s irq_reg_add;

    /*! Add interrupt ACK register for kernel to control. */
    struct ngbde_ioc_intr_ack_reg_add_s intr_ack_reg_add;

    /*! Write to a shared interrupt mask register. */
    struct ngbde_ioc_irq_mask_wr_s irq_mask_wr;

    /*! Map device registers in kernel space. */
    struct ngbde_ioc_pio_win_s pio_win;
};

/*! IOCTL command message. */
typedef struct ngbde_ioc_cmd_s {

    /*! Device handle. */
    __u32 devid;

    /*! Return code (0 means success). */
    __u32 rc;

    /*! IOCTL operation. */
    union ngbde_ioc_op_s op;
} ngbde_ioc_cmd_t;

#endif /* NGBDE_IOCTL_H */
