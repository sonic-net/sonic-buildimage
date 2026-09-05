/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2017-2018,2021, Pensando Systems Inc.
 */

#ifndef __PCIESVC_PRT_H__
#define __PCIESVC_PRT_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

/******************************************************************
 * PCIe Resource Table (PRT)
 *
 * PRT entry is the table entry by PMT to describe
 * bar match table resources and handling.
 */

#define PRT_COUNT       4096
#define PRT_NWORDS      3

/* terminator index for chained prts */
#define PRT_INVALID     ((u_int16_t)-1)

/* for PRT.type */
#define PRT_TYPE_RES    0       /* resource */
#define PRT_TYPE_DB64   1       /* 64-bit doorbells */
#define PRT_TYPE_DB32   2       /* 32-bit doorbells */
#define PRT_TYPE_DB16   3       /* 16-bit doorbells */

/* for PRT.res.wqebpsize */
#define PRT_WQEBP_SZ64  0       /*  64B aligned WQEs */
#define PRT_WQEBP_SZ128 1       /* 128B aligned WQEs */
#define PRT_WQEBP_SZ256 2       /* 256B aligned WQEs */
#define PRT_WQEBP_SZ512 3       /* 512B aligned WQEs */

/* for PRT.db.updvec */
#define PRT_UPD_SCHED_NONE      0x00 /* no scheduler request */
#define PRT_UPD_SCHED_EVAL      0x01 /* scheduler eval pi/ci */
#define PRT_UPD_SCHED_CLEAR     0x02 /* scheduler clear */
#define PRT_UPD_SCHED_SET       0x03 /* scheduler set */
#define PRT_UPD_SCHED_MASK      0x03 /* sched bit mask */
#define PRT_UPD_PICI_CISET      0x04 /* set ci */
#define PRT_UPD_PICI_PISET      0x08 /* set pi */
#define PRT_UPD_PICI_PIINC      0x0c /* increment pi */
#define PRT_UPD_PICI_MASK       0x0c /* pici bit mask */
#define PRT_UPD_PID_CHECK       0x10 /* check pid */

#define PRT_CMN_FIELDS                                      \
    u_int64_t valid     :1;     /* entry is valid */        \
    u_int64_t type      :2;     /* PRT_TYPE_* */            \
    u_int64_t indirect  :1;     /* sw handles tlp */        \
    u_int64_t notify    :1;     /* notify sw */             \
    u_int64_t vfstride  :5      /* power-of-2 stride added to addr */

/* common prt entry format */
typedef struct {
    PRT_CMN_FIELDS;
} __attribute__((packed)) prt_cmn_t;

/* resource prt entry format */
typedef struct {
    PRT_CMN_FIELDS;
    u_int64_t aspace    :1;     /* target address space, 1=external (pcie) */
    u_int64_t addrdw    :50;    /* target resource address */
    u_int64_t sizedw    :11;    /* encoded resource size */
    u_int64_t pmvdis    :1;     /* disable Programming Model Violation check */
#if defined(ASIC_CAPRI)
    u_int64_t spare     :3;     /* implemented but unused in hw */
    u_int64_t rsrv      :52;    /* unimplemented bits */
#elif defined(ASIC_ELBA)
    u_int64_t wqebpen   :1;     /* WQE bypass enable */
    u_int64_t wqebpsize :2;     /* WQE bypass entry size encoded */
    u_int64_t wqebpdben :1;     /* WQE bypass doorbell enable */
    u_int64_t spare     :8;     /* implemented but unused in hw */
    u_int64_t rsrv      :43;    /* unimplemented bits */
#else
#error "ASIC not specified"
#endif
} __attribute__((packed)) prt_res_t;

/* db64/db32/db16 prt entry format */
typedef struct {
    PRT_CMN_FIELDS;
    u_int64_t lif       :11;    /* target LIF */
    u_int64_t updvec    :40;    /* 8x5-bit UPD field, indexed by qtype */
    u_int64_t stridesel :2;     /* selects vfstride, 0=VF, 1={VF,LIF} */
    u_int64_t idxshift  :2;     /* db16/32: index location in data */
    u_int64_t idxwidth  :4;     /* db16/32: index width    in data */
    u_int64_t qidshift  :2;     /* db16/32: qid   location in data */
    u_int64_t qidwidth  :4;     /* db16/32: qid   width    in data */
    u_int64_t qidsel    :1;     /* db16/32: qid source select, 0=data 1=addr */
#if defined(ASIC_CAPRI)
    u_int64_t rsrv      :52;    /* unimplemented bits */
#elif defined(ASIC_ELBA)
    u_int64_t dbbussel  :1;     /* doorbell bus select, 0=prp, 1=express */
    u_int64_t spare     :8;     /* implemented but unused in hw */
    u_int64_t rsrv      :43;    /* unimplemented bits */
#else
#error "ASIC not specified"
#endif
} __attribute__((packed)) prt_db_t;

/* PRT entry */
typedef union prt_u {
    prt_cmn_t cmn;
    prt_res_t res;
    prt_db_t db;
    u_int32_t w[PRT_NWORDS];
} prt_t;

static inline int
prt_is_valid(const prt_t *prt)
{
    return prt->cmn.valid;
}

static inline u_int32_t
prt_type(const prt_t *prt)
{
    return prt->cmn.type;
}

#ifdef __cplusplus
}
#endif

#endif /* __PCIESVC_PRT_H__ */
