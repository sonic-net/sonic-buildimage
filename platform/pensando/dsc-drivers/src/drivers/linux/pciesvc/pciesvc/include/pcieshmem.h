/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2017-2018,2021-2022, Pensando Systems Inc.
 */

#ifndef __PCIESHMEM_H__
#define __PCIESHMEM_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

#include "pciehdevice_types.h"
#include "pciehw.h"
#include "pciemgr_stats.h"
#include "pmt.h"
#include "prt.h"

enum pciehw_cfghnd_e {
    PCIEHW_CFGHND_NONE,
    PCIEHW_CFGHND_CMD,
    PCIEHW_CFGHND_DEV_BARS,
    PCIEHW_CFGHND_ROM_BAR,
    PCIEHW_CFGHND_BRIDGECTL,
    PCIEHW_CFGHND_MSIX,
    PCIEHW_CFGHND_VPD,
    PCIEHW_CFGHND_PCIE_DEVCTL,
    PCIEHW_CFGHND_SRIOV_CTRL,
    PCIEHW_CFGHND_SRIOV_BARS,
    PCIEHW_CFGHND_DBG_DELAY,
    PCIEHW_CFGHND_BRIDGE_BUS,
};
typedef enum pciehw_cfghnd_e pciehw_cfghnd_t;

typedef enum pciehwbartype_e {
    PCIEHWBARTYPE_NONE,                 /* invalid bar type */
    PCIEHWBARTYPE_MEM,                  /* 32-bit memory bar */
    PCIEHWBARTYPE_MEM64,                /* 64-bit memory bar */
    PCIEHWBARTYPE_IO,                   /* 32-bit I/O bar */
} pciehwbartype_t;

typedef enum pciehw_barhnd_e {
    PCIEHW_BARHND_NONE,
    PCIEHW_BARHND_SERIAL,
    PCIEHW_BARHND_VIRTIO,
} pciehw_barhnd_t;

typedef union pciehwbar_u {
    struct {
        u_int64_t size;                 /* total size of this bar */
        u_int32_t valid:1;              /* valid bar for this dev */
        u_int32_t loaded:1;             /* pmts loaded */
        u_int32_t ovrds:1;              /* override pmts chained on ovrd */
        pciehwbartype_t type;           /* PCIEHWBARTYPE_* */
        u_int8_t cfgidx;                /* config bars index (0-5) */
        u_int8_t hnd;                   /* indirect/notify handling */
        u_int16_t bdf;                  /* host bdf of bar owner */
        u_int32_t pmtb;                 /* pmt base  for bar */
        u_int32_t pmtc;                 /* pmt count for bar */
        u_int16_t ovrd;                 /* override pmts */
        u_int16_t _unused;
        u_int64_t addr;                 /* addr of this bar */
    };
    u_int8_t _pad[64];
} pciehwbar_t;

typedef u_int32_t pciehwdevh_t;

#define PCIEHW_ROMSKSZ  (PCIEHW_CFGSZ / sizeof (u_int32_t))
#define PCIEHW_CFGHNDSZ (PCIEHW_CFGSZ / sizeof (u_int32_t))

#define NOVRDINTR       8

/*
 * If PCIEHDEVICE_OVERRIDE_INTRGROUPS increases we'll have
 * to grow the shared memory region with special handling.
 */
#if NOVRDINTR < PCIEHDEVICE_OVERRIDE_INTRGROUPS
# error "NOVRDINTR < PCIEHDEVICE_OVERRIDE_INTRGROUPS"
#endif

typedef struct ovrdintr_s {
    u_int32_t intrb;                    /* ovrd intr base */
    u_int32_t intrc;                    /* ovrd intr count */
} ovrdintr_t;

typedef union pciehwdev_u {
    struct {
        char name[32];                  /* device name */
        int port;                       /* pcie port */
        u_int16_t pf:1;                 /* is pf */
        u_int16_t vf:1;                 /* is vf */
        u_int16_t flexvf:1;             /* is flexvf */
        u_int16_t msix_en:1;            /* msix enabled for this dev */
        u_int16_t totalvfs;             /* totalvfs provisioned */
        u_int16_t numvfs;               /* current numvfs */
        u_int16_t vfidx;                /* if is vf, vf position */
        u_int16_t bdf;                  /* bdf of this dev */
        u_int8_t type;                  /* PCIEHDEVICE_* */
        u_int8_t novrdintr;             /* number valid in ovrdintr[] */
        u_int32_t lifb;                 /* lif base  for this dev */
        u_int32_t lifc;                 /* lif count for this dev */
        u_int32_t intrb;                /* intr resource base */
        u_int32_t intrc;                /* intr resource count */
        u_int32_t intrdmask:1;          /* reset val for drvcfg.mask */
        u_int32_t cfgloaded:1;          /* cfg pmt entries loaded */
        pciehwdevh_t parenth;           /* handle to parent */
        pciehwdevh_t childh;            /* handle to child */
        pciehwdevh_t peerh;             /* handle to peer */
        u_int8_t intpin;                /* legacy int pin */
        u_int8_t romsksel[PCIEHW_ROMSKSZ]; /* cfg read-only mask selectors */
        u_int8_t cfgpmtf[PCIEHW_CFGHNDSZ]; /* cfg pmt flags */
        u_int8_t cfghnd[PCIEHW_CFGHNDSZ];  /* cfg indirect/notify handlers */
        pciehwbar_t bar[PCIEHW_NBAR];   /* bar info */
        pciehwbar_t rombar;             /* option rom bar */
        u_int16_t sriovctrl;            /* current sriov ctrl reg */
        u_int16_t enabledvfs;           /* current numvfs enabled */
        pciehwdevh_t hwdevh;            /* handle to this dev */
        u_int32_t pmtb;                 /* pmt base  for cfg */
        u_int32_t pmtc;                 /* pmt count for cfg */
        ovrdintr_t ovrdintr[NOVRDINTR]; /* override intr resources */
    };
    u_int8_t _pad[4096];
} pciehwdev_t;

typedef union pciehw_port_u {
    struct {
        u_int8_t secbus;                /* bridge secondary bus */
        pciemgr_stats_t stats;
    };
    u_int8_t _pad[1024];
} pciehw_port_t;

typedef union pciehw_sprt_u {
    struct {
        prt_t prt;                      /* shadow copy of prt */
        u_int16_t next;                 /* next link for chained prts */
    };
    u_int8_t _pad[32];
} pciehw_sprt_t;

typedef union pciehw_spmt_u {
    struct {
        u_int64_t baroff;               /* bar addr offset */
        u_int64_t swrd;                 /* reads  handled by sw (not/ind) */
        u_int64_t swwr;                 /* writes handled by sw (not/ind) */
        pciehwdevh_t owner;             /* current owner of this entry */
        u_int8_t loaded:1;              /* is loaded into hw */
        u_int8_t vf0:1;                 /* sriov vf0 apply enabledvfs limit */
        u_int8_t vf0stride:5;           /* sriov vf0 addr mask stride */
        u_int8_t chain:1;               /* chained pmts on next */
        u_int8_t cfgidx;                /* cfgidx for bar we belong to */
        pmt_t pmt;                      /* shadow copy of pmt */
        u_int64_t vf0base:52;           /* sriov vf0 resource base address */
        u_int64_t pmtstart:6;           /* sriov vf0 addr mask start */
        u_int16_t next;                 /* next link for chained pmts */
    };
    u_int8_t _pad[128];
} pciehw_spmt_t;

typedef struct pciehw_sromsk_s {
    u_int32_t entry;
    u_int32_t count;
} pciehw_sromsk_t;

#define PCIEHW_MAGIC    0x706d656d      /* 'pmem' */
#define PCIEHW_VERSION  0x1

#define PCIEHW_VPDSZ    1024
#define PCIEHW_SERIALSZ 1024

typedef struct pciehw_shmem_lo_s {
    u_int32_t magic;                    /* PCIEHW_MAGIC when initialized */
    u_int32_t version;                  /* PCIEHW_VERSION when initialized */
    u_int32_t hwinit:1;                 /* hw is initialized */
    u_int32_t notify_verbose:1;         /* notify logs all */
    u_int32_t skip_notify:1;            /* notify skips if ring full */
    u_int32_t pmtpri:1;                 /* support pmt pri */
    u_int32_t evregistered:1;           /* event handler registered flag */
    u_int32_t hi_ndev:1;                /* 2048 ndev */
    /* Hi-Lo should be in sync till here */
    u_int32_t allocdev;
    u_int32_t allocpmt_high;            /* high priority pmt free sequential */
    u_int32_t allocprt;                 /* prt free sequential */
    u_int32_t notify_ring_mask;
    pciehwdevh_t rooth[PCIEHW_NPORTS];
    pciehwdev_t dev[PCIEHW_NDEVS];
    pciehw_port_t port[PCIEHW_NPORTS];
    pciehw_sromsk_t sromsk[PCIEHW_NROMSK];
    pciehw_spmt_t spmt[PCIEHW_NPMT];
    pciehw_sprt_t sprt[PCIEHW_NPRT];
    u_int8_t cfgrst[PCIEHW_NDEVS][PCIEHW_CFGSZ];
    u_int8_t cfgmsk[PCIEHW_NDEVS][PCIEHW_CFGSZ];
    u_int8_t vpddata[PCIEHW_NDEVS][PCIEHW_VPDSZ];
    u_int8_t serial[PCIEHW_NPORTS][PCIEHW_SERIALSZ];
    u_int32_t freepmt_high;             /* high priority pmt free list */
    u_int32_t allocpmt_low;             /* low priority pmt free sequential */
    u_int32_t freepmt_low;              /* low priority pmt free list */
    u_int32_t allocpmt_vf0adj;          /* low pri vf0 adjust (never freed) */
    u_int32_t freeprt_slab;             /* prt free slab adjacent */
} pciehw_shmem_lo_t;

typedef struct pciehw_shmem_hi_s {
    u_int32_t magic;                    /* PCIEHW_MAGIC when initialized */
    u_int32_t version;                  /* PCIEHW_VERSION when initialized */
    u_int32_t hwinit:1;                 /* hw is initialized */
    u_int32_t notify_verbose:1;         /* notify logs all */
    u_int32_t skip_notify:1;            /* notify skips if ring full */
    u_int32_t pmtpri:1;                 /* support pmt pri */
    u_int32_t evregistered:1;           /* event handler registered flag */
    u_int32_t hi_ndev:1;                /* 2048 ndev */
    /* Hi-Lo should be in sync till here */
    u_int32_t allocdev;
    u_int32_t allocpmt_high;            /* high priority pmt free sequential */
    u_int32_t allocpmt_low;             /* low priority pmt free sequential */
    u_int32_t allocpmt_vf0adj;          /* low pri vf0 adjust (never freed) */
    u_int32_t allocprt;                 /* prt free sequential */
    u_int32_t freepmt_high;             /* high priority pmt free list */
    u_int32_t freepmt_low;              /* low priority pmt free list */
    u_int32_t freeprt_slab;             /* prt free slab adjacent */
    u_int32_t notify_ring_mask;
    pciehwdevh_t rooth[PCIEHW_NPORTS];
    pciehwdev_t dev[PCIEHW_NDEVS_HI];
    pciehw_port_t port[PCIEHW_NPORTS];
    pciehw_sromsk_t sromsk[PCIEHW_NROMSK];
    pciehw_spmt_t spmt[PCIEHW_NPMT];
    pciehw_sprt_t sprt[PCIEHW_NPRT];
    u_int8_t cfgrst[PCIEHW_NDEVS_HI][PCIEHW_CFGSZ];
    u_int8_t cfgmsk[PCIEHW_NDEVS_HI][PCIEHW_CFGSZ];
    u_int8_t vpddata[PCIEHW_NDEVS_HI][PCIEHW_VPDSZ];
    u_int8_t serial[PCIEHW_NPORTS][PCIEHW_SERIALSZ];
} pciehw_shmem_hi_t;

typedef struct pciehw_shmem_s {
    union {
        pciehw_shmem_lo_t lo;
        pciehw_shmem_hi_t hi;
    };
} pciehw_shmem_t;

#define PSHMEM_IS_HI_NDEV(S) (S->lo.hi_ndev)
#define PSHMEM_NDEVS(S) (S->lo.hi_ndev ? PCIEHW_NDEVS_HI : PCIEHW_NDEVS)
#define PSHMEM_DATA_FIELD(S, V) (S->lo.hi_ndev ? S->hi.V : S->lo.V)
#define PSHMEM_ADDR_FIELD(S, V) (S->lo.hi_ndev ? &S->hi.V : &S->lo.V)
#define PSHMEM_ASGN_FIELD(S, V, A) \
            (S->lo.hi_ndev ? (S->hi.V = A) : (S->lo.V = A))
#define PSHMEM_OFFSETOF(S, V) (S->lo.hi_ndev ? \
            offsetof(pciehw_shmem_hi_t, V) : offsetof(pciehw_shmem_lo_t, V))
#define PSHMEM_SIZEOF(S, V) (S->lo.hi_ndev ? \
            sizeof(((pciehw_shmem_hi_t *)0L)->V) : \
            sizeof(((pciehw_shmem_lo_t *)0L)->V))
#define PSHMEM_SIZEOF_T(S) (S->lo.hi_ndev ? sizeof(pciehw_shmem_hi_t) : \
            sizeof(pciehw_shmem_lo_t))

#ifdef __cplusplus
}
#endif

#endif /* __PCIESHMEM_H__ */
