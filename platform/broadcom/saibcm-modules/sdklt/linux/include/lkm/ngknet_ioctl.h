/*! \file ngknet_ioctl.h
 *
 * NGKNET I/O control definitions.
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

#ifndef NGKNET_IOCTL_H
#define NGKNET_IOCTL_H

/*! Module information */
#define NGKNET_MODULE_NAME      "linux_ngknet"
#define NGKNET_MODULE_MAJOR     121

/*! Must be updated if backward compatibility is broken */
#define NGKNET_IOC_VERSION      4

/*! Max number of input arguments */
#define NGKNET_IOC_IARG_MAX     2

#define NGKNET_IOC_MAGIC        'K'

#define NGKNET_VERSION_GET      _IOR(NGKNET_IOC_MAGIC,  0xa0, unsigned int)
#define NGKNET_RX_RATE_LIMIT    _IOWR(NGKNET_IOC_MAGIC, 0xa1, unsigned int)
#define NGKNET_DEV_INIT         _IOWR(NGKNET_IOC_MAGIC, 0xb0, unsigned int)
#define NGKNET_DEV_DEINIT       _IOWR(NGKNET_IOC_MAGIC, 0xb1, unsigned int)
#define NGKNET_DEV_SUSPEND      _IOWR(NGKNET_IOC_MAGIC, 0xb2, unsigned int)
#define NGKNET_DEV_RESUME       _IOWR(NGKNET_IOC_MAGIC, 0xb3, unsigned int)
#define NGKNET_DEV_VNET_WAIT    _IOWR(NGKNET_IOC_MAGIC, 0xb4, unsigned int)
#define NGKNET_DEV_HNET_WAKE    _IOWR(NGKNET_IOC_MAGIC, 0xb5, unsigned int)
#define NGKNET_DEV_VNET_DOCK    _IOWR(NGKNET_IOC_MAGIC, 0xb6, unsigned int)
#define NGKNET_DEV_VNET_UNDOCK  _IOWR(NGKNET_IOC_MAGIC, 0xb7, unsigned int)
#define NGKNET_QUEUE_CONFIG     _IOWR(NGKNET_IOC_MAGIC, 0xc0, unsigned int)
#define NGKNET_QUEUE_QUERY      _IOR(NGKNET_IOC_MAGIC,  0xc1, unsigned int)
#define NGKNET_RCPU_CONFIG      _IOWR(NGKNET_IOC_MAGIC, 0xc2, unsigned int)
#define NGKNET_RCPU_GET         _IOR(NGKNET_IOC_MAGIC,  0xc3, unsigned int)
#define NGKNET_NETIF_CREATE     _IOWR(NGKNET_IOC_MAGIC, 0xd0, unsigned int)
#define NGKNET_NETIF_DESTROY    _IOWR(NGKNET_IOC_MAGIC, 0xd1, unsigned int)
#define NGKNET_NETIF_GET        _IOR(NGKNET_IOC_MAGIC,  0xd2, unsigned int)
#define NGKNET_NETIF_NEXT       _IOR(NGKNET_IOC_MAGIC,  0xd3, unsigned int)
#define NGKNET_NETIF_LINK_SET   _IOW(NGKNET_IOC_MAGIC,  0xd4, unsigned int)
#define NGKNET_FILT_CREATE      _IOWR(NGKNET_IOC_MAGIC, 0xe0, unsigned int)
#define NGKNET_FILT_DESTROY     _IOWR(NGKNET_IOC_MAGIC, 0xe1, unsigned int)
#define NGKNET_FILT_GET         _IOR(NGKNET_IOC_MAGIC,  0xe2, unsigned int)
#define NGKNET_FILT_NEXT        _IOR(NGKNET_IOC_MAGIC,  0xe3, unsigned int)
#define NGKNET_INFO_GET         _IOR(NGKNET_IOC_MAGIC,  0xf0, unsigned int)
#define NGKNET_STATS_GET        _IOR(NGKNET_IOC_MAGIC,  0xf1, unsigned int)
#define NGKNET_STATS_RESET      _IOWR(NGKNET_IOC_MAGIC, 0xf2, unsigned int)
#define NGKNET_PTP_DEV_CTRL     _IOWR(NGKNET_IOC_MAGIC, 0x90, unsigned int)

/*! Kernel module information. */
struct ngknet_ioc_mod_info {
    /*! IOCTL version used by kernel module */
    uint32_t version;
};

/*! Data transmission */
struct ngknet_ioc_data_xmit {
    /*! Data buffer address */
    uint64_t buf;

    /*! Data buffer length */
    uint32_t len;
};

/*! IOCTL operations */
union ngknet_ioc_op {
    /*! Get module info */
    struct ngknet_ioc_mod_info info;
    /*! Transmit data */
    struct ngknet_ioc_data_xmit data;
};

/*!
 * \brief NGKNET IOCTL command message.
 */
struct ngknet_ioctl {
    /*! Device number */
    uint32_t unit;

    /*! Return code (0 means success) */
    uint32_t rc;

    /*! Input arguments */
    int iarg[NGKNET_IOC_IARG_MAX];

    /*! IOCTL operation */
    union ngknet_ioc_op op;
};

#endif /* NGKNET_IOCTL_H */

