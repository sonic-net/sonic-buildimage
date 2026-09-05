/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022, Pensando Systems Inc.
 */

#ifndef __PCIEHDEVICE_TYPES_H__
#define __PCIEHDEVICE_TYPES_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

typedef enum pciehdevice_type_e {
    PCIEHDEVICE_NONE,
    PCIEHDEVICE_ETH,
    PCIEHDEVICE_MGMTETH,
    PCIEHDEVICE_ACCEL,
    PCIEHDEVICE_NVME,
    PCIEHDEVICE_VIRTIO,
    PCIEHDEVICE_PCIESTRESS,
    PCIEHDEVICE_DEBUG,
    PCIEHDEVICE_RCDEV,
    PCIEHDEVICE_CRYPT,
    PCIEHDEVICE_UPT,
    PCIEHDEVICE_SERIAL,
    PCIEHDEVICE_CORE,
} pciehdevice_type_t;

#define PCIEHDEVICE_OVERRIDE_INTRGROUPS 8

#ifdef __cplusplus
}
#endif

#endif /* __PCIEHDEVICE_TYPES_H__ */
