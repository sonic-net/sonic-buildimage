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

typedef struct pciehw_mem_lo_s {
    u_int8_t notify_area[PCIEHW_NPORTS][PCIEHW_NOTIFYSZ]
                                     __attribute__((aligned(PCIEHW_NOTIFYSZ)));
    /* page of zeros to back cfgspace */
    u_int8_t zeros[4096] __attribute__((aligned(4096)));
    u_int8_t cfgcur[PCIEHW_NDEVS][PCIEHW_CFGSZ] __attribute__((aligned(4096)));
    u_int32_t notify_intr_dest[PCIEHW_NPORTS];   /* notify   intr dest */
    u_int32_t indirect_intr_dest[PCIEHW_NPORTS]; /* indirect intr dest */
    u_int32_t magic;                    /* PCIEHW_MAGIC when initialized */
    u_int32_t version;                  /* PCIEHW_VERSION when initialized */
} pciehw_mem_lo_t;

typedef struct pciehw_mem_hi_s {
    u_int8_t notify_area[PCIEHW_NPORTS][PCIEHW_NOTIFYSZ]
                                     __attribute__((aligned(PCIEHW_NOTIFYSZ)));
    /* page of zeros to back cfgspace */
    u_int8_t zeros[4096] __attribute__((aligned(4096)));
    u_int8_t cfgcur[PCIEHW_NDEVS_HI][PCIEHW_CFGSZ]
                                     __attribute__((aligned(4096)));
    u_int32_t notify_intr_dest[PCIEHW_NPORTS];   /* notify   intr dest */
    u_int32_t indirect_intr_dest[PCIEHW_NPORTS]; /* indirect intr dest */
    u_int32_t magic;                    /* PCIEHW_MAGIC when initialized */
    u_int32_t version;                  /* PCIEHW_VERSION when initialized */
} pciehw_mem_hi_t;

typedef struct pciehw_mem_s {
    union {
        pciehw_mem_lo_t lo;
        pciehw_mem_hi_t hi;
    };
} pciehw_mem_t;

#define PHWMEM_DATA_FIELD(H, S, V) (S->lo.hi_ndev ? H->hi.V : H->lo.V)
#define PHWMEM_ADDR_FIELD(H, S, V) (S->lo.hi_ndev ? &H->hi.V : &H->lo.V)
#define PHWMEM_ASGN_FIELD(H, S, V, A) (S->lo.hi_ndev ? \
            (H->hi.V = A) : (H->lo.V = A))
#define PHWMEM_OFFSETOF(H, S, V) (S->lo.hi_ndev ? \
            offsetof(pciehw_mem_hi_t, V) : offsetof(pciehw_mem_lo_t, V))
#define PHWMEM_SIZEOF(H, S, V) (S->lo.hi_ndev ? \
            sizeof(((pciehw_mem_hi_t *)0L)->V) : \
            sizeof(((pciehw_mem_lo_t *)0L)->V))

#ifdef __cplusplus
}
#endif

#endif /* __PCIEHWMEM_H__ */
