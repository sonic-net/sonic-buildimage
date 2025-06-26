/*! \file ngst_ioctl.h
 *
 * NGST device I/O control definitions.
 *
 * This file is intended for use in both kernel mode and user mode.
 *
 * IMPORTANT!
 * All shared structures must be properly 64-bit aligned.
 *
 */
/*
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

#ifndef NGST_IOCTL_H
#define NGST_IOCTL_H

#include <linux/types.h>
#include <linux/ioctl.h>

/*! Module information */
#define NGST_MODULE_NAME       "linux_ngst"
#define NGST_MODULE_MAJOR      61

/*! LUST IOCTL command magic. */
#define NGST_IOC_MAGIC 'x'

/*!
 * \name IOCTL commands for the NGST kernel module.
 * \anchor NGST_IOC_xxx
 */

/*! \{ */

/*! Set/Get ST DMA memory information. */
#define NGST_IOC_DMA_INFO  _IOWR(NGST_IOC_MAGIC, 0, struct ngst_ioc_dma_info_s)

/*! \} */

/*! IOCTL command return code for success. */
#define NGST_IOC_SUCCESS       0

/*! Get ST DMA information */
struct ngst_ioc_dma_info_s {

    /*! Unit */
    __u32 unit;

    /*! Virtual address */
    __u64 vaddr;

    /*! Physical address */
    __u64 paddr;

    /*! Number of DMA chunks */
    __u32 chunk_cnt;

    /*! DMA pool size */
    __u32 size;
};

#endif /* NGST_IOCTL_H */
