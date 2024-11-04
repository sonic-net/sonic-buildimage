/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021, Pensando Systems Inc.
 */

#ifndef __PCIEHWMEM_H__
#define __PCIEHWMEM_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

#include "pciehw.h"

#define PCIEHW_NOTIFYSZ         (1 * 1024 * 1024)

typedef struct pciehw_mem_s {
    u_int8_t notify_area[PCIEHW_NPORTS][PCIEHW_NOTIFYSZ]
                                     __attribute__((aligned(PCIEHW_NOTIFYSZ)));
    /* page of zeros to back cfgspace */
    u_int8_t zeros[4096] __attribute__((aligned(4096)));
    u_int8_t cfgcur[PCIEHW_NDEVS][PCIEHW_CFGSZ] __attribute__((aligned(4096)));
    u_int32_t notify_intr_dest[PCIEHW_NPORTS];   /* notify   intr dest */
    u_int32_t indirect_intr_dest[PCIEHW_NPORTS]; /* indirect intr dest */
    u_int32_t magic;                    /* PCIEHW_MAGIC when initialized */
    u_int32_t version;                  /* PCIEHW_VERSION when initialized */
} pciehw_mem_t;

#ifdef __cplusplus
}
#endif

#endif /* __PCIEHWMEM_H__ */
