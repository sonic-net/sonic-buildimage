/*! \file ngedk_ioctl.h
 *
 * NGEDK device I/O control definitions.
 *
 * This file is intended for use in both kernel mode and user mode.
 *
 * IMPORTANT!
 * All shared structures must be properly 64-bit aligned.
 *
 */
/*
 *
 * Copyright 2018-2025 Broadcom. All rights reserved.
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
 * be found in the LICENSES folder.
 */

#ifndef NGEDK_IOCTL_H
#define NGEDK_IOCTL_H

#include <linux/types.h>
#include <linux/ioctl.h>

/*! Module information */
#define NGEDK_MODULE_NAME       "linux_ngedk"
#define NGEDK_MODULE_MAJOR      62

/*! Must be updated if backward compatibility is broken. */
#define NGEDK_IOC_VERSION       1

/*! LUEDK IOCTL command magic. */
#define NGEDK_IOC_MAGIC 'L'

/*! Maximum number of mHosts supported per switch device. */
#ifndef MCS_NUM_UC
#define MCS_NUM_UC     6
#endif

/*!
 * \name IOCTL commands for the NGEDK kernel module.
 * \anchor NGEDK_IOC_xxx
 *
 * Note that we use __u64 for the IOCTL parameter size because
 * sizeof(void *) is different between 32-bit and 64-bit code, and we
 * need a 32-bit user mode application to generate the same IOCTL
 * command codes as a 64-bit kernel when using the _IOW macro.
 */

/*! \{ */

/*! Get kernel module information. */
#define NGEDK_IOC_MOD_INFO      _IOW(NGEDK_IOC_MAGIC, 0, __u64)

/*! Attach EDK instance. */
#define NGEDK_IOC_ATTACH_INST   _IOW(NGEDK_IOC_MAGIC, 1, __u64)

/*! Get EDK DMA memory information. */
#define NGEDK_IOC_GET_DMA_INFO  _IOW(NGEDK_IOC_MAGIC, 2, __u64)

/*! Enable EDK interrupts in this unit. */
#define NGEDK_IOC_INTR_ENABLE   _IOW(NGEDK_IOC_MAGIC, 3, __u64)

/*! Disable EDK interrupts in this unit. */
#define NGEDK_IOC_INTR_DISABLE  _IOW(NGEDK_IOC_MAGIC, 4, __u64)

/*! Set Interrupt registers and mask values used by the EDK. */
#define NGEDK_IOC_INTR_SET      _IOW(NGEDK_IOC_MAGIC, 5, __u64)

/*! Wait for an EDK interrupt. */
#define NGEDK_IOC_INTR_WAIT     _IOW(NGEDK_IOC_MAGIC, 6, __u64)

/*! Handle EDK software interrupt. */
#define NGEDK_IOC_SW_INTR       _IOW(NGEDK_IOC_MAGIC, 7, __u64)

/*! Handle EDK timer interrupt. */
#define NGEDK_IOC_TIMER_INTR    _IOW(NGEDK_IOC_MAGIC, 8, __u64)

/*! \} */

/*! IOCTL command return code for success. */
#define NGEDK_IOC_SUCCESS       0

/*! IOCTL command return code for failure. */
#define NGEDK_IOC_FAIL          ((__u32)-1)

/*!
 * \name EDK IOC flags.
 * \anchor NGEDK_IOC_F_xxx
 */

/*! \{ */

/*! Interrupt enable/disable registers are "write 1 to clear". */
#define NGEDK_IOC_F_W1TC         (1 << 0)

/*! \} */

/*! Kernel module information. */
struct ngedk_ioc_mod_info_s {

    /*! IOCTL version used by kernel module. */
    __u16 version;
};

/*! Attach EDK Instance */
struct ngedk_ioc_attach_inst_s {

    /*! HostRAM size for this instance. */
    __u32 size_mb;
};

/*! Get EDK DMA information */
struct ngedk_ioc_dma_info_s {

    /*! Virtual address */
    __u64 vaddr;

    /*! Physical address */
    __u64 paddr;

    /*! Bus address as maped by IOMMU */
    __u64 baddr;

    /*! DMA pool size */
    __u32 size;
};

/* Set details of interrupts handled by EDK */
struct ngedk_ioc_intr_s {

    /*! Active cores */
    __u32 active_bmp;

    /*! Timer interrupts status offset */
    __u32 timer_intrc_stat_reg;

    /*! Timer interrupts disable offset */
    __u32 timer_intrc_disable_reg;

    /*! Timer interrupts mask */
    __u32 timer_intrc_mask_val;

    /*! Bitmap of cores that triggered SW interrupt. */
    __u32 sw_intr_cores;

    /*! EDK ioctl flags (\ref NGEDK_IOC_F_xxx). */
    __u32 flags;
};

/*! IOCTL operation data. */
struct ngedk_ioc_sw_intr_s {

    /*! mHost core number corresponding to this SW interrupt */
    __u32 uc;

};

/*! IOCTL operation data. */
union ngedk_ioc_op_s {

    /*! Get kernel module information. */
    struct ngedk_ioc_mod_info_s mod_info;

    /*! Attach EDK Instance */
    struct ngedk_ioc_attach_inst_s attach_inst;

    /*! EDK DMA information */
    struct ngedk_ioc_dma_info_s dma_info;

    /*! EDK Interrupt setting */
    struct ngedk_ioc_intr_s edk_intr;

    /*! EDK software interrupt */
    struct ngedk_ioc_sw_intr_s sw_intr;
};

/*! IOCTL command message. */
typedef struct ngedk_ioc_cmd_s {

    /*! Device handle. */
    __u32 devid;

    /*! Return code (0 means success). */
    __u32 rc;

    /*! IOCTL operation. */
    union ngedk_ioc_op_s op;

} ngedk_ioc_cmd_t;

#endif /* NGEDK_IOCTL_H */
