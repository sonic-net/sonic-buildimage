/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020, Pensando Systems Inc.
 */

#ifndef __PCIESVC_EVENT_H__
#define __PCIESVC_EVENT_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

typedef enum pciesvc_event_e {
    PCIESVC_EV_NONE,
    PCIESVC_EV_MEMRD_NOTIFY,
    PCIESVC_EV_MEMWR_NOTIFY,
    PCIESVC_EV_SRIOV_NUMVFS,
    PCIESVC_EV_RESET,
    PCIESVC_EV_QFULL,
    PCIESVC_EV_MGMTCHG,
    PCIESVC_EV_LOGMSG,
} pciesvc_event_t;

typedef struct pciesvc_memrw_notify_s {
    u_int64_t baraddr;          /* PCIe bar address */
    u_int64_t baroffset;        /* bar-local offset */
    u_int8_t cfgidx;            /* bar cfgidx */
    u_int32_t size;             /* i/o size */
    u_int64_t localpa;          /* local physical address */
    u_int64_t data;             /* data, if write */
} pciesvc_memrw_notify_t;

typedef struct pciesvc_sriov_numvfs_s {
    u_int16_t numvfs;           /* number of vfs enabled */
} pciesvc_sriov_numvfs_t;

typedef enum pciesvc_rsttype_e {
    PCIESVC_RSTTYPE_NONE,
    PCIESVC_RSTTYPE_BUS,        /* bus reset */
    PCIESVC_RSTTYPE_FLR,        /* function level reset */
    PCIESVC_RSTTYPE_VF,         /* vf reset from sriov ctrl vfe */
} pciesvc_rsttype_t;

typedef struct pciesvc_reset_s {
    pciesvc_rsttype_t rsttype;  /* RSTTYPE_* */
    u_int32_t lifb;             /* lif base */
    u_int32_t lifc;             /* lif count */
} pciesvc_reset_t;

typedef enum pciesvc_logpri_e {
    PCIESVC_LOGPRI_DEBUG,
    PCIESVC_LOGPRI_INFO,
    PCIESVC_LOGPRI_WARN,
    PCIESVC_LOGPRI_ERROR,
} pciesvc_logpri_t;

typedef struct pciesvc_logmsg_s {
    pciesvc_logpri_t pri;       /* log priority LOGPRI_ */
    char msg[80];               /* log string, NULL-terminated */
} pciesvc_logmsg_t;

typedef struct pciesvc_eventdata_s {
    pciesvc_event_t evtype;     /* PCIESVC_EV_* */
    u_int8_t port;              /* PCIe port */
    u_int32_t lif;              /* lif if event for lifs */
    union {
        pciesvc_memrw_notify_t memrw_notify;    /* EV_MEMRD/WR_NOTIFY */
        pciesvc_sriov_numvfs_t sriov_numvfs;    /* EV_SRIOV_NUMVFS */
        pciesvc_reset_t reset;                  /* EV_RESET */
        pciesvc_logmsg_t logmsg;                /* EV_LOGMSG */
    };
} pciesvc_eventdata_t;

#ifdef __cplusplus
}
#endif

#endif /* __PCIESVC_EVENT_H__ */
