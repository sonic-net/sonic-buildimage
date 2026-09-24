/*! \file ngptpclock_main.c
 *
 * NGPTPCLOCK module.
 *
 */
/*
 * Copyright 2018-2024 Broadcom. All rights reserved.
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

/*
 * This module implements a Linux PTP Clock driver for Broadcom
 * XGS switch devices.
 *
 * - All the data structures and functions work on the physical port.
 *   For array indexing purposes, we use (phy_port - 1).
 */

#include <linux/module.h>
#include <linux/version.h>
#include <lkm/ngptpclock_ioctl.h>

MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("PTP Clock Driver for Broadcom XGS Switch");
MODULE_LICENSE("GPL");

#if ((LINUX_VERSION_CODE >= KERNEL_VERSION(4,4,0)) && \
            (LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)))
#define NGPTPCLOCK_SUPPORT
#endif

#ifdef NGPTPCLOCK_SUPPORT
#include <lkm/lkm.h>
#include <lkm/ngknet_kapi.h>
#include <lkm/ngedk_kapi.h>
#include <linux/time64.h>
#include <linux/delay.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/sysfs.h>
#include <linux/net_tstamp.h>

#include <linux/random.h>
#include <linux/seq_file.h>
#include <linux/if_vlan.h>
#include <linux/ptp_clock_kernel.h>
#include <shr/shr_error.h>

/* Configuration Parameters */
static int debug;
module_param(debug, int, 0);
MODULE_PARM_DESC(debug,
        "Debug level (default 0)");

static int network_transport;
module_param(network_transport, int, 0);
MODULE_PARM_DESC(network_transport,
        "Transport Type (default - Detect from packet)");

static char *base_dev_name = "ptp0";
module_param(base_dev_name, charp, 0);
MODULE_PARM_DESC(base_dev_name,
        "Base device name like ptp0, ptp1, etc.(default ptp0)");

static int fw_core;
module_param(fw_core, int, 0);
MODULE_PARM_DESC(fw_core,
        "Firmware core (default 0)");

static int pci_cos;

/* Debug levels */
#define DBG_LVL_VERB    0x1
#define DBG_LVL_WARN    0x2
#define DBG_LVL_TXTS    0x4
#define DBG_LVL_CMDS    0x8
#define DBG_LVL_TX      0x10
#define DBG_LVL_RX      0x20
#define DBG_LVL_TX_DUMP 0x40
#define DBG_LVL_RX_DUMP 0x80

#define DBG_VERB(_s)    do { if (debug & DBG_LVL_VERB) printk _s; } while (0)
#define DBG_WARN(_s)    do { if (debug & DBG_LVL_WARN) printk _s; } while (0)
#define DBG_TXTS(_s)    do { if (debug & DBG_LVL_TXTS) printk _s; } while (0)
#define DBG_CMDS(_s)    do { if (debug & DBG_LVL_CMDS) printk _s; } while (0)
#define DBG_TX(_s)      do { if (debug & DBG_LVL_TX) printk _s; } while (0)
#define DBG_RX(_s)      do { if (debug & DBG_LVL_RX) printk _s; } while (0)
#define DBG_TX_DUMP(_s) do { if (debug & DBG_LVL_TX_DUMP) printk _s; } while (0)
#define DBG_RX_DUMP(_s) do { if (debug & DBG_LVL_RX_DUMP) printk _s; } while (0)
#define DBG_ERR(_s)     do { if (1) printk _s; } while (0)

#define DMA_DEV                         device
#define DMA_ALLOC_COHERENT(d,s,h)       dma_alloc_coherent(d,s,h,GFP_ATOMIC|GFP_DMA32)
#define DMA_FREE_COHERENT(d,s,a,h)      dma_free_coherent(d,s,a,h)

/* Type length in bytes */
#define NGPTPCLOCK_PACKLEN_U8     1
#define NGPTPCLOCK_PACKLEN_U16    2
#define NGPTPCLOCK_PACKLEN_U24    3
#define NGPTPCLOCK_PACKLEN_U32    4

#define NGPTPCLOCK_UNPACK_U8(_buf, _var) \
    _var = *_buf++

#define NGPTPCLOCK_UNPACK_U16(_buf, _var) \
    do { \
        (_var) = (((_buf)[0] << 8) | \
                (_buf)[1]); \
        (_buf) += NGPTPCLOCK_PACKLEN_U16; \
    } while (0)

#define NGPTPCLOCK_UNPACK_U24(_buf, _var) \
    do { \
        (_var) = (((_buf)[0] << 16) | \
                ((_buf)[1] << 8)  | \
                (_buf)[2]); \
        (_buf) += NGPTPCLOCK_PACKLEN_U24; \
    } while (0)

#define NGPTPCLOCK_UNPACK_U32(_buf, _var) \
    do { \
        (_var) = (((_buf)[0] << 24) | \
                ((_buf)[1] << 16) | \
                ((_buf)[2] << 8)  | \
                (_buf)[3]); \
        (_buf) += NGPTPCLOCK_PACKLEN_U32; \
    } while (0)





#define CMICX_DEV_TYPE              1

/* CMIC MCS-0 SCHAN Messaging registers */
/* Core0:CMC1 Core1:CMC2 */
#define CMIC_CMC_BASE \
            (CMICX_DEV_TYPE ? (fw_core ? 0x10400 : 0x10300) : \
                              (fw_core ? 0x33000 : 0x32000))

#define CMIC_CMC_SCHAN_MESSAGE_10r(BASE) (BASE + 0x00000034)
#define CMIC_CMC_SCHAN_MESSAGE_11r(BASE) (BASE + 0x00000038)
#define CMIC_CMC_SCHAN_MESSAGE_12r(BASE) (BASE + 0x0000003c)
#define CMIC_CMC_SCHAN_MESSAGE_13r(BASE) (BASE + 0x00000040)
#define CMIC_CMC_SCHAN_MESSAGE_14r(BASE) (BASE + 0x00000044)
#define CMIC_CMC_SCHAN_MESSAGE_15r(BASE) (BASE + 0x00000048)
#define CMIC_CMC_SCHAN_MESSAGE_16r(BASE) (BASE + 0x0000004c)
#define CMIC_CMC_SCHAN_MESSAGE_17r(BASE) (BASE + 0x00000050)
#define CMIC_CMC_SCHAN_MESSAGE_18r(BASE) (BASE + 0x00000054)
#define CMIC_CMC_SCHAN_MESSAGE_19r(BASE) (BASE + 0x00000058)
#define CMIC_CMC_SCHAN_MESSAGE_20r(BASE) (BASE + 0x0000005c)
#define CMIC_CMC_SCHAN_MESSAGE_21r(BASE) (BASE + 0x00000060)

static u32 hostcmd_regs[5] = { 0 };

#define NGPTPCLOCK_NUM_PORTS           128     /* NUM_PORTS where 2-step is supported. */
#define NGPTPCLOCK_MAX_NUM_PORTS       256     /* Max ever NUM_PORTS in the system. */
#define NGPTPCLOCK_MAX_MTP_IDX         8       /* Max number of mtps in the system. */

/* Service request commands to Firmware. */
enum {
    NGPTPCLOCK_DONE                     = 0x0,
    NGPTPCLOCK_INIT                     = 0x1,
    NGPTPCLOCK_CLEANUP                  = 0x2,
    NGPTPCLOCK_GETTIME                  = 0x3,
    NGPTPCLOCK_SETTIME                  = 0x4,
    NGPTPCLOCK_FREQCOR                  = 0x5,
    NGPTPCLOCK_PBM_UPDATE               = 0x6,
    NGPTPCLOCK_ADJTIME                  = 0x7,
    NGPTPCLOCK_GET_TSTIME               = 0x8,
    NGPTPCLOCK_MTP_TS_UPDATE_ENABLE     = 0x9,
    NGPTPCLOCK_MTP_TS_UPDATE_DISABLE    = 0xa,
    NGPTPCLOCK_ACK_TSTIME               = 0xb,
    NGPTPCLOCK_SYSINFO                  = 0xc,
    NGPTPCLOCK_BROADSYNC                = 0xd,
    NGPTPCLOCK_GPIO                     = 0xe,
    NGPTPCLOCK_EVLOG                    = 0xf,
    NGPTPCLOCK_EXTTSLOG                 = 0x10,
    NGPTPCLOCK_GET_EXTTS_BUFF           = 0x11,
    NGPTPCLOCK_GPIO_PHASEOFFSET         = 0x12,
};

enum {
    NGPTPCLOCK_SYSINFO_UC_PORT_NUM       = 0x1,
    NGPTPCLOCK_SYSINFO_UC_PORT_SYSPORT   = 0x2,
    NGPTPCLOCK_SYSINFO_HOST_CPU_PORT     = 0x3,
    NGPTPCLOCK_SYSINFO_HOST_CPU_SYSPORT  = 0x4,
    NGPTPCLOCK_SYSINFO_UDH_LEN           = 0x5,
};

enum {
    NGPTPCLOCK_BROADSYNC_BS0_CONFIG      = 0x1,
    NGPTPCLOCK_BROADSYNC_BS1_CONFIG      = 0x2,
    NGPTPCLOCK_BROADSYNC_BS0_STATUS_GET  = 0x3,
    NGPTPCLOCK_BROADSYNC_BS1_STATUS_GET  = 0x4,
};

enum {
    NGPTPCLOCK_GPIO_0       = 0x1,
    NGPTPCLOCK_GPIO_1       = 0x2,
    NGPTPCLOCK_GPIO_2       = 0x3,
    NGPTPCLOCK_GPIO_3       = 0x4,
    NGPTPCLOCK_GPIO_4       = 0x5,
    NGPTPCLOCK_GPIO_5       = 0x6,
};

/* 1588 message types. */
enum
{
    IEEE1588_MSGTYPE_SYNC           = 0x0,
    IEEE1588_MSGTYPE_DELREQ         = 0x1,
    IEEE1588_MSGTYPE_PDELREQ        = 0x2,
    IEEE1588_MSGTYPE_PDELRESP       = 0x3,
    /* reserved                       0x4 */
    /* reserved                       0x5 */
    /* reserved                       0x6 */
    /* reserved                       0x7 */
    IEEE1588_MSGTYPE_FLWUP          = 0x8,
    IEEE1588_MSGTYPE_DELRESP        = 0x9,
    IEEE1588_MSGTYPE_PDELRES_FLWUP  = 0xA,
    IEEE1588_MSGTYPE_ANNOUNCE       = 0xB,
    IEEE1588_MSGTYPE_SGNLNG         = 0xC,
    IEEE1588_MSGTYPE_MNGMNT         = 0xD
    /* reserved                       0xE */
    /* reserved                       0xF */
};

/* Usage macros */
#define ONE_BILLION (1000000000)

#define SKB_U16_GET(_skb, _pkt_offset) \
            ((_skb->data[_pkt_offset] << 8) | _skb->data[_pkt_offset + 1])

#define NGPTPCLOCK_PTP_EVENT_MSG(_ptp_msg_type) \
            ((_ptp_msg_type == IEEE1588_MSGTYPE_DELREQ) || \
             (_ptp_msg_type == IEEE1588_MSGTYPE_SYNC))

/*
 *  IEEE1588 packet hardware specific information.
 *  4 words of information used from this data set.
 *       0 -  3: 2-step untagged.
 *       4 -  7: 2-step tagged.
 *       8 - 11: 1-step untagged.
 *      12 - 15: 1-step tagged.
 *      16 - 19: 1-step untagged with ITS-set.
 *      20 - 23: 1-step tagged with ITS-set.
 *
 *      Refer to device specific reg file for SOBMH header information.
 *      Below fields are considered:
 *      SOBMH => {
 *      IEEE1588_ONE_STEP_ENABLE        -   OneStep
 *      IEEE1588_REGEN_UDP_CHECKSUM     -   Regen UDP Checksum
 *      IEEE1588_INGRESS_TIMESTAMP_SIGN -   ITS
 *      TX_TS                           -   TwoStep
 *      IEEE1588_TIMESTAMP_HDR_OFFSET   -   1588 header offset
 *      }
 *
 */
static uint32_t ieee1588_l2pkt_md[24] = {0};
static uint32_t ieee1588_ipv4pkt_md[24] = {0};
static uint32_t ieee1588_ipv6pkt_md[24] = {0};

/* Driver Proc Entry root */
static struct proc_dir_entry *ngptpclock_proc_root = NULL;

/* Shared data structures with R5 */
typedef struct ngptpclock_tx_ts_data_s {
    u32 ts_valid;   /* Timestamp valid indication */
    u32 port_id;    /* Port number */
    u32 ts_seq_id;  /* Sequency Id */
    u32 ts_cnt;
    u64 timestamp;  /* Timestamp */
} ngptpclock_tx_ts_data_t;

typedef struct ngptpclock_info_s {
    u32 ksyncinit;
    u32 dev_id;
    s64 freqcorr;
    u64 portmap[NGPTPCLOCK_MAX_NUM_PORTS/64];  /* Two-step enabled ports */
    u64 ptptime;
    u64 reftime;
    u64 ptptime_alt;
    u64 reftime_alt;
    s64 phase_offset;
    ngptpclock_tx_ts_data_t port_ts_data[NGPTPCLOCK_MAX_NUM_PORTS];
} ngptpclock_info_t;


enum {
    TS_EVENT_CPU       = 0,
    TS_EVENT_BSHB_0    = 1,
    TS_EVENT_BSHB_1    = 2,
    TS_EVENT_GPIO_1    = 3,
    TS_EVENT_GPIO_2    = 4,
    TS_EVENT_GPIO_3    = 5,
    TS_EVENT_GPIO_4    = 6,
    TS_EVENT_GPIO_5    = 7,
    TS_EVENT_GPIO_6    = 8,
};

#define NUM_TS_EVENTS 14

/* FW timestamps.
 *     This declaration has to match with HFT_t_TmStmp
 *     defined in the firmware. Otherwise, DMA will fail.
 */
typedef struct fw_tstamp_s {
    u64 sec;
    u32 nsec;
} __attribute__ ((packed)) fw_tstamp_t;

typedef struct ngptpclock_fw_debug_event_tstamps_s {
    fw_tstamp_t prv_tstamp;
    fw_tstamp_t cur_tstamp;
} __attribute__ ((packed)) ngptpclock_fw_debug_event_tstamps_t;

typedef struct ngptpclock_evlog_s {
    ngptpclock_fw_debug_event_tstamps_t event_timestamps[NUM_TS_EVENTS];
} __attribute__ ((packed)) ngptpclock_evlog_t;


/* Timestamps for EXTTS from Firmware */
/* gpio0 = event0 ..... gpio5 = event5 */
#define NUM_EXT_TS          6
/* Directly mapped to PTP_MAX_TIMESTAMPS from ptp_private.h */
#define NUM_EVENT_TS        128
typedef struct ngptpclock_fw_extts_event_s {
    u32         ts_event_id;
    fw_tstamp_t tstamp;
} __attribute__ ((packed)) ngptpclock_fw_extts_event_t;

typedef struct ngptpclock_extts_log_s {
    u32                     head;   /* Read pointer - Updated by HOST */
    u32                     tail;   /* Write pointer - Updated by FW */
    ngptpclock_fw_extts_event_t event_ts[NUM_EVENT_TS];
    u32                     overflow;
} __attribute__ ((packed)) ngptpclock_fw_extts_log_t;

struct ngptpclock_extts_event {
    int enable[NUM_EXT_TS];
    int head;
};

typedef struct ngptpclock_port_stats_s {
    u32 pkt_rxctr;             /* All ingress packets */
    u32 pkt_txctr;             /* All egress packets  */
    u32 pkt_txonestep;         /* 1-step Tx packets counter */
    u32 tsts_match;            /* 2-Step tstamp req matches */
    u32 tsts_timeout;          /* 2-Step tstamp req timeouts */
    u32 tsts_discard;          /* 2-Step tstamp req discards */
    u32 osts_event_pkts;       /* 1-step event packet counter */
    u32 osts_tstamp_reqs;      /* 1-step events with tstamp request */
    u32 fifo_rxctr;            /* 2-Step tstamp req matches */
    u64 tsts_best_fetch_time;  /* 1-step events with tstamp request */
    u64 tsts_worst_fetch_time; /* 1-step events with tstamp request */
    u32 tsts_avg_fetch_time;   /* 1-step events with tstamp request */
} ngptpclock_port_stats_t;

typedef struct ngptpclock_init_info_s {
    u32 pci_knetsync_cos;
    u32 uc_port_num;
    u32 uc_port_sysport;
    u32 host_cpu_port;
    u32 host_cpu_sysport;
    u32 udh_len;
} ngptpclock_init_info_t;

typedef struct ngptpclock_bs_info_s {
    u32 enable;
    u32 mode;
    u32 bc;
    u32 hb;
} ngptpclock_bs_info_t;

typedef struct ngptpclock_gpio_info_s {
    u32 enable;
    u32 mode;
    u32 period;
    int64_t phase_offset;
} ngptpclock_gpio_info_t;

typedef struct ngptpclock_evlog_info_s {
    u32 enable;
} ngptpclock_evlog_info_t;

/* Clock Private Data */
struct ngptpclock_ptp_priv {
    struct device dev;
    int dcb_type;
    struct ptp_clock *ptp_clock;
    struct ptp_clock_info ptp_caps;
    struct mutex ptp_lock;
    int ptp_pair_lock;
    volatile void *base_addr;   /* Address for PCI register access. */
    volatile ngptpclock_info_t *shared_addr; /* address for shared memory access. */
    volatile ngptpclock_evlog_t *evlog; /* dma-able address for fw updates. */
    dma_addr_t dma_mem;
    int dma_mem_size;
    struct DMA_DEV *dma_dev; /* Required for DMA memory control. */
    int num_pports;
    int timekeep_status;
    u32 mirror_encap_bmp;
    struct delayed_work time_keep;
    ngptpclock_port_stats_t *port_stats;
    ngptpclock_init_info_t ngptpclock_init_info;
    ngptpclock_bs_info_t ngptpclock_bs_info[2];
    ngptpclock_gpio_info_t ngptpclock_gpio_info[6];
    ngptpclock_evlog_info_t ngptpclock_evlog_info[NUM_TS_EVENTS];
    volatile ngptpclock_fw_extts_log_t *extts_log;
    struct ngptpclock_extts_event extts_event;
    struct delayed_work extts_logging;
    struct kobject *kobj;
};

static struct ngptpclock_ptp_priv *ptp_priv;
static volatile int module_initialized;
#if defined(TWO_STEP_SUPPORT)
static int num_retries = 10;   /* Retry count */
#endif

static void ngptpclock_ptp_time_keep_init(void);
static void ngptpclock_ptp_time_keep_cleanup(void);
static int ngptpclock_ptp_gettime(struct ptp_clock_info *ptp,
                                  struct timespec64 *ts);

static void ngptpclock_ptp_extts_logging_init(void);
static void ngptpclock_ptp_extts_logging_cleanup(void);

#if defined(CMIC_SOFT_BYTE_SWAP)

#define CMIC_SWAP32(_x)   ((((_x) & 0xff000000) >> 24) \
        | (((_x) & 0x00ff0000) >>  8) \
        | (((_x) & 0x0000ff00) <<  8) \
        | (((_x) & 0x000000ff) << 24))

#define DEV_READ32(_d, _a, _p) \
    do { \
        uint32_t _data; \
        _data = (((volatile uint32_t *)(_d)->base_addr)[(_a)/4]); \
        *(_p) = CMIC_SWAP32(_data); \
    } while (0)

#define DEV_WRITE32(_d, _a, _v) \
    do { \
        uint32_t _data = CMIC_SWAP32(_v); \
        ((volatile uint32_t *)(_d)->base_addr)[(_a)/4] = (_data); \
    } while (0)

#else

#define DEV_READ32(_d, _a, _p) \
    do { \
        *(_p) = (((volatile uint32_t *)(_d)->base_addr)[(_a)/4]); \
    } while (0)

#define DEV_WRITE32(_d, _a, _v) \
    do { \
        ((volatile uint32_t *)(_d)->base_addr)[(_a)/4] = (_v); \
    } while (0)
#endif  /* defined(CMIC_SOFT_BYTE_SWAP) */

static void
ptp_usleep(int usec)
{
    usleep_range(usec, usec+1);
}

static void
ptp_sleep(int jiffies)
{
    wait_queue_head_t wq;
    init_waitqueue_head(&wq);

    wait_event_timeout(wq, 0, jiffies);
}

/**
 * ngptpclock_hostcmd_data_op
 *
 * @setget: If valid then set and get the data.
 * @d1: data pointer one.
 * @d2: data pointer two.
 *
 * Description: This function is used send and receive the
 * data from the FW.
 */
static void
ngptpclock_hostcmd_data_op(int setget, u64 *d1, u64 *d2)
{
    u32 w0, w1;
    u64 data;

    if (!d1) {
        return;
    }

    if (setget) {
        if (d1) {
            data = *d1;
            w0 = (data & 0xFFFFFFFF);
            w1 = (data >> 32);
            DEV_WRITE32(ptp_priv, hostcmd_regs[1], w0);
            DEV_WRITE32(ptp_priv, hostcmd_regs[2], w1);
        }

        if (d2) {
            data = *d2;

            w0 = (data & 0xFFFFFFFF);
            w1 = (data >> 32);
            DEV_WRITE32(ptp_priv, hostcmd_regs[3], w0);
            DEV_WRITE32(ptp_priv, hostcmd_regs[4], w1);
        }
    } else {
        if (d1) {
            DEV_READ32(ptp_priv, hostcmd_regs[1], &w0);
            DEV_READ32(ptp_priv, hostcmd_regs[2], &w1);
            data = (((u64)w1 << 32) | (w0));
            *d1 = data;
        }

        if (d2) {
            DEV_READ32(ptp_priv, hostcmd_regs[3], &w0);
            DEV_READ32(ptp_priv, hostcmd_regs[4], &w1);
            data = (((u64)w1 << 32) | (w0));
            *d2 = data;
        }
    }
}

static int
ngptpclock_cmd_go(u32 cmd, void *data0, void *data1)
{
    int ret = -1;
    int retry_cnt = 1000;
    u32 cmd_status;
    char cmd_str[48];
    int port = 0;
    uint32_t seq_id = 0;
    ktime_t start, now;
    u32 subcmd = 0;

    if (ptp_priv == NULL || ptp_priv->shared_addr == NULL) {
        return ret;
    }

    mutex_lock(&ptp_priv->ptp_lock);

    if (cmd == NGPTPCLOCK_GET_TSTIME || cmd == NGPTPCLOCK_ACK_TSTIME) {
        port = *((uint64_t *)data0) & 0xFFF;
        seq_id = *((uint64_t*)data0) >> 16;
    }
    start = ktime_get();

    ptp_priv->shared_addr->ksyncinit = cmd;

    /* init data */
    DEV_WRITE32(ptp_priv, hostcmd_regs[1], 0x0);
    DEV_WRITE32(ptp_priv, hostcmd_regs[2], 0x0);
    DEV_WRITE32(ptp_priv, hostcmd_regs[3], 0x0);
    DEV_WRITE32(ptp_priv, hostcmd_regs[4], 0x0);

    switch (cmd) {
        case NGPTPCLOCK_INIT:
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_INIT");
            ptp_priv->shared_addr->phase_offset  = 0;
            ngptpclock_hostcmd_data_op(1,
                                       (u64 *)&(ptp_priv->shared_addr->phase_offset),
                                       0);
            break;
        case NGPTPCLOCK_FREQCOR:
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_FREQCORR");
            ptp_priv->shared_addr->freqcorr  = *((s32 *)data0);
            ngptpclock_hostcmd_data_op(1,
                                       (u64 *)&(ptp_priv->shared_addr->freqcorr),
                                       0);
            break;
        case NGPTPCLOCK_ADJTIME:
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_ADJTIME");
            ptp_priv->shared_addr->phase_offset  = *((s64 *)data0);
            ngptpclock_hostcmd_data_op(1,
                                       (u64 *)&(ptp_priv->shared_addr->phase_offset),
                                       0);
            break;
        case NGPTPCLOCK_GETTIME:
            retry_cnt = (retry_cnt * 2);
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_GETTIME");
            break;
        case NGPTPCLOCK_GET_TSTIME:
            retry_cnt = (retry_cnt * 2);
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_GET_TSTIME");
            ngptpclock_hostcmd_data_op(1, data0, data1);
            break;
         case NGPTPCLOCK_ACK_TSTIME:
            retry_cnt = (retry_cnt * 2);
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_ACK_TSTIME");
            ngptpclock_hostcmd_data_op(1, data0, data1);
            break;
        case NGPTPCLOCK_SETTIME:
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_SETTIME");
            ptp_priv->shared_addr->ptptime   = *((s64 *)data0);
            ptp_priv->shared_addr->phase_offset = 0;
            ngptpclock_hostcmd_data_op(1,
                                       (u64 *)&(ptp_priv->shared_addr->ptptime),
                                       (u64 *)&(ptp_priv->shared_addr->phase_offset));
            break;
        case NGPTPCLOCK_MTP_TS_UPDATE_ENABLE:
            retry_cnt = (retry_cnt * 6);
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_MTP_TS_UPDATE_ENABLE");
            ngptpclock_hostcmd_data_op(1, (u64 *)data0, 0);
            break;
        case NGPTPCLOCK_MTP_TS_UPDATE_DISABLE:
            retry_cnt = (retry_cnt * 6);
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_MTP_TS_UPDATE_DISABLE");
            ngptpclock_hostcmd_data_op(1, (u64 *)data0, 0);
            break;
        case NGPTPCLOCK_CLEANUP:
            retry_cnt = (retry_cnt * 4);
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_CLEANUP");
            break;
        case NGPTPCLOCK_SYSINFO:
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_SYSINFO");
            ngptpclock_hostcmd_data_op(1, (u64 *)data0, (u64 *)data1);
            break;
        case NGPTPCLOCK_BROADSYNC:
            subcmd = *((u32 *)data0);
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_BROADSYNC");
            ngptpclock_hostcmd_data_op(1, (u64 *)data0, (u64 *)data1);
            break;
        case NGPTPCLOCK_GPIO:
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_GPIO");
            ngptpclock_hostcmd_data_op(1, (u64 *)data0, (u64 *)data1);
            break;
        case NGPTPCLOCK_EVLOG:
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_EVLOG");
            ngptpclock_hostcmd_data_op(1, (u64 *)data0, (u64 *)data1);
            break;
        case NGPTPCLOCK_EXTTSLOG:
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_EXTTSLOG");
            ngptpclock_hostcmd_data_op(1, (u64 *)data0, (u64 *)data1);
            break;
        case NGPTPCLOCK_GET_EXTTS_BUFF:
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_GET_EXTTS_BUFF");
            ngptpclock_hostcmd_data_op(1, (u64 *)data0, (u64 *)data1);
            break;
        case NGPTPCLOCK_GPIO_PHASEOFFSET:
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_GPIO_PHASEOFFSET");
            ngptpclock_hostcmd_data_op(1, (u64 *)data0, (u64 *)data1);
            break;
        default:
            snprintf(cmd_str, sizeof(cmd_str), "NGPTPCLOCK_XXX");
            break;
    }

    DEV_WRITE32(ptp_priv, hostcmd_regs[0], ptp_priv->shared_addr->ksyncinit);

    do {
        DEV_READ32(ptp_priv, hostcmd_regs[0], &cmd_status);
        ptp_priv->shared_addr->ksyncinit = cmd_status;

        if (cmd_status == NGPTPCLOCK_DONE) {
            ret = 0;
            switch (cmd) {
                case NGPTPCLOCK_GET_TSTIME:
                case NGPTPCLOCK_GETTIME:
                    ngptpclock_hostcmd_data_op(0, (u64 *)data0, (u64 *)data1);
                    break;
                /* Get the host ram address from fw.*/
                case NGPTPCLOCK_GET_EXTTS_BUFF:
                    ngptpclock_hostcmd_data_op(0, (u64 *)data0, (u64 *)data1);
                    break;
                case NGPTPCLOCK_BROADSYNC:
                    if ((subcmd == NGPTPCLOCK_BROADSYNC_BS0_STATUS_GET) ||
                        (subcmd == NGPTPCLOCK_BROADSYNC_BS1_STATUS_GET)) {
                        ngptpclock_hostcmd_data_op(0, (u64 *)data0, (u64 *)data1);
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        ptp_usleep(100);
        retry_cnt--;
    } while (retry_cnt);

    now = ktime_get();
    mutex_unlock(&ptp_priv->ptp_lock);

    if (retry_cnt == 0) {
        DBG_ERR(("Timeout on response from R5 to cmd %s time taken %lld us\n",
                    cmd_str, ktime_us_delta(now, start)));
        if (cmd == NGPTPCLOCK_GET_TSTIME) {
            DBG_TXTS(("Timeout Port %d SeqId %d\n", port, seq_id));
        }
    }

    if (debug & DBG_LVL_CMDS) {
        if (ktime_us_delta(now, start) > 5000)
            DBG_CMDS(("R5 Command %s exceeded time expected (%lld us)\n",
                        cmd_str, ktime_us_delta(now, start)));
    }

    DBG_CMDS(("ngptpclock_cmd_go: cmd:%s rv:%d\n", cmd_str, ret));

    return ret;
}


/**
 * ngptpclock_ptp_adjfreq
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ppb: frequency correction value
 *
 * Description: this function will set the frequency correction
 */
static int ngptpclock_ptp_adjfreq(struct ptp_clock_info *ptp, s32 ppb)
{
    int ret = -1;

    ret = ngptpclock_cmd_go(NGPTPCLOCK_FREQCOR, &ppb, NULL);
    DBG_VERB(("ptp_adjfreq: applying freq correction: %x; rv:%d\n", ppb, ret));

    return ret;
}

/**
 * ngptpclock_ptp_adjtime
 *
 * @ptp: pointer to ptp_clock_info structure
 * @delta: desired change in nanoseconds
 *
 * Description: this function will shift/adjust the hardware clock time.
 */
static int ngptpclock_ptp_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
    int ret = -1;

    ret = ngptpclock_cmd_go(NGPTPCLOCK_ADJTIME, (void *)&delta, NULL);
    DBG_VERB(("ptp_adjtime: adjtime: 0x%llx; rv:%d\n", delta, ret));

    return ret;
}

/**
 * ngptpclock_ptp_gettime
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ts: pointer to hold time/result
 *
 * Description: this function will read the current time from the
 * hardware clock and store it in @ts.
 */
static int ngptpclock_ptp_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
    int ret = -1;
    s64 reftime = 0;
    s64 refctr = 0;
    static u64 prv_reftime = 0, prv_refctr = 0;
    u64 diff_reftime = 0, diff_refctr = 0;

    ret = ngptpclock_cmd_go(NGPTPCLOCK_GETTIME, (void *)&reftime, (void *)&refctr);
    if (ret == 0) {
        DBG_VERB(("ptp_gettime: gettime: 0x%llx refctr:0x%llx\n", reftime, refctr));

        ptp_priv->shared_addr->ptptime_alt = ptp_priv->shared_addr->ptptime;
        ptp_priv->shared_addr->reftime_alt = ptp_priv->shared_addr->reftime;

        ptp_priv->ptp_pair_lock = 1;
        ptp_priv->shared_addr->ptptime = reftime;
        ptp_priv->shared_addr->reftime = refctr;
        ptp_priv->ptp_pair_lock = 0;

        diff_reftime = reftime - prv_reftime;
        diff_refctr = refctr - prv_refctr;

        if (diff_reftime != diff_refctr) {
            DBG_WARN(("PTP-GETTIME ptptime: 0x%llx reftime: 0x%llx "
                        "prv_ptptime: 0x%llx prv_reftime: 0x%llx \n",
                        ptp_priv->shared_addr->ptptime,
                        ptp_priv->shared_addr->reftime,
                        diff_reftime, diff_refctr));
        }
        prv_reftime = reftime;
        prv_refctr = refctr;

        *ts = ns_to_timespec64(reftime);
    }
    return ret;
}


/**
 * ngptpclock_ptp_settime
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ts: time value to set
 *
 * Description: this function will set the current time on the
 * hardware clock.
 */
static int ngptpclock_ptp_settime(struct ptp_clock_info *ptp,
                              const struct timespec64 *ts)
{
    s64 reftime, phaseadj;
    int ret = -1;

    phaseadj = 0;
    reftime = timespec64_to_ns(ts);

    ret = ngptpclock_cmd_go(NGPTPCLOCK_SETTIME, (void *)&reftime, (void *)&phaseadj);
    DBG_VERB(("ptp_settime: settime: 0x%llx; rv:%d\n", reftime, ret));

    return ret;
}

static int ngptpclock_exttslog_cmd(int event, int enable)
{
    int ret;
    u64 subcmd = 0, subcmd_data = 0;

    if (NULL == ptp_priv->extts_log) {
        ret = ngptpclock_cmd_go(NGPTPCLOCK_GET_EXTTS_BUFF,
                                &subcmd, &subcmd_data);
        DBG_VERB(("ngptpclock_exttslog_cmd: Get EXTTS buff: \
                   subcmd_data:0x%llx\n", subcmd_data));

        ptp_priv->extts_log =
            (ngptpclock_fw_extts_log_t *)ngedk_dmamem_map_p2v(subcmd_data);
        if (NULL == ptp_priv->extts_log) {
            DBG_ERR(("Failed to get virtual addr for the physical address\n"));
        }
    }

    /* upper 32b -> event
     * lower 32b -> enable/disable */
    subcmd = (u64)event << 32 | enable;

    ret = ngptpclock_cmd_go(NGPTPCLOCK_EXTTSLOG, &subcmd, &subcmd_data);
    DBG_VERB(("exttslog_cmd: subcmd: 0x%llx subcmd_data: 0x%llx rv:%d\n",
                subcmd, subcmd_data, ret));

    return ret;
}

static int ngptpclock_ptp_enable(struct ptp_clock_info *ptp,
                             struct ptp_clock_request *rq, int on)
{
    int mapped_event = -1;
    int enable = on ? 1 : 0;

    switch (rq->type) {
        case PTP_CLK_REQ_EXTTS:
            if (rq->extts.index < NUM_EXT_TS) {
                switch (rq->extts.index) {
                    /* Map EXTTS event_id to FW event_id */
                    case 0:
                        mapped_event = TS_EVENT_GPIO_1;
                        break;
                    case 1:
                        mapped_event = TS_EVENT_GPIO_2;
                        break;
                    case 2:
                        mapped_event = TS_EVENT_GPIO_3;
                        break;
                    case 3:
                        mapped_event = TS_EVENT_GPIO_4;
                        break;
                    case 4:
                        mapped_event = TS_EVENT_GPIO_5;
                        break;
                    case 5:
                        mapped_event = TS_EVENT_GPIO_6;
                        break;
                    default:
                        return -EINVAL;
                }

                /* Reject request for unsupported flags */
                if (rq->extts.flags & ~(PTP_ENABLE_FEATURE | PTP_RISING_EDGE)) {
                        return -EOPNOTSUPP;
                }

                ptp_priv->extts_event.enable[rq->extts.index] = enable;

                ngptpclock_exttslog_cmd(mapped_event, enable);

                DBG_VERB(("Event state change req_index:%u state:%d\n",
                            rq->extts.index, enable));
            } else {
                return -EINVAL;
            }
            break;
        default:
            return -EOPNOTSUPP;
    }

    return 0;
}

#if defined(MIRROR_ENCAP_SUPPORT)
static int ngptpclock_ptp_mirror_encap_update(struct ptp_clock_info *ptp,
                                          int mtp_idx, int start)
{
    int ret = -1;
    u64 mirror_encap_idx;
    u32 cmd_status;

    if (mtp_idx > NGPTPCLOCK_MAX_MTP_IDX) {
        return ret;
    }

    mirror_encap_idx = mtp_idx;
    if (start) {
        cmd_status = NGPTPCLOCK_MTP_TS_UPDATE_ENABLE;
        ptp_priv->mirror_encap_bmp |= (1 << mtp_idx);
    } else {
        if (!(ptp_priv->mirror_encap_bmp & mtp_idx)) {
            return ret;
        }
        cmd_status = NGPTPCLOCK_MTP_TS_UPDATE_DISABLE;
        ptp_priv->mirror_encap_bmp &= ~mtp_idx;
    }

    ret = ngptpclock_cmd_go(cmd_status, &mirror_encap_idx, NULL);
    DBG_VERB(("mirror_encap_update: %d, mpt_index: %d, ret:%d\n",
              start, mtp_idx, ret));

    return ret;

}
#endif

/* structure describing a PTP hardware clock */
static struct ptp_clock_info ngptpclock_ptp_caps = {
    .owner = THIS_MODULE,
    .name = "ptp_clock",
    .max_adj = 200000,
    .n_alarm = 0,
    .n_ext_ts = NUM_EXT_TS,
    .n_per_out = 0, /* will be overwritten in ngptpclock_ptp_register */
    .n_pins = 0,
    .pps = 0,
    .adjfreq = ngptpclock_ptp_adjfreq,
    .adjtime = ngptpclock_ptp_adjtime,
    .gettime64 = ngptpclock_ptp_gettime,
    .settime64 = ngptpclock_ptp_settime,
    .enable = ngptpclock_ptp_enable,
};

/**
 * ngptpclock_ptp_hw_tx_tstamp_config
 *
 * @dinfo: device information
 * @netif: netif information
 * @hwts_tx_type: TX Timestamp type
 *
 * Description: This is a callback function to enable/disable the TX timestamping port
 * based.
 */
int ngptpclock_ptp_hw_tx_tstamp_config(ngknet_dev_info_t *dinfo,
        ngknet_netif_t *netif,
        int *hwts_tx_type)
{
#if defined(TWO_STEP_SUPPORT)
    uint64_t portmap = 0;
    int map = 0;
#endif
    int ret = SHR_E_CONFIG;

    if (!module_initialized) {
        ret = SHR_E_DISABLED;
        goto exit;
    }

    DBG_VERB(("hw_tx_tstamp_config: Tx type %d\n", *hwts_tx_type));
    if (*hwts_tx_type == HWTSTAMP_TX_ONESTEP_SYNC) {
        DBG_VERB(("hw_tx_tstamp_config: Enabling 1-step\n"));
        ngptpclock_ptp_time_keep_init();
        ret = SHR_E_NONE;
        goto exit;
    } else if (*hwts_tx_type == HWTSTAMP_TX_OFF) {
        DBG_VERB(("hw_tx_tstamp_config: Diabling 1-step\n"));
        ret = SHR_E_NONE;
        goto exit;
    } else if (*hwts_tx_type == HWTSTAMP_TX_ON) {
#if !defined(TWO_STEP_SUPPORT)
        DBG_VERB(("hw_tx_tstamp_config: 2Step not supported\n"));
        ret = SHR_E_UNAVAIL;
        goto exit;
#endif
#if defined(TWO_STEP_SUPPORT)
        DBG_VERB(("hw_tstamp_enable: Enabling 2-step(type:%d) TS on port:%d\n",
                  tx_type, port));
        if (port <= 0) {
            ret = SHR_E_PARAM;
            goto exit;
        }

        /* Update the shared structure member */
        if (ptp_priv->shared_addr) {
            if ((port > 0) && (port < NGPTPCLOCK_MAX_NUM_PORTS)) {
                port -= 1;
                map = (port / 64);
                port = (port % 64);

                portmap = ptp_priv->shared_addr->portmap[map];
                portmap |= (uint64_t)0x1 << port;
                ptp_priv->shared_addr->portmap[map] = portmap;

                /* Command to R5 for the update */
                ptp_priv->shared_addr->ksyncinit=NGPTPCLOCK_PBM_UPDATE;

            }
        }
#endif
    }
exit:
    return ret;
}

/**
 * ngptpclock_ptp_hw_rx_tstamp_config
 *
 * @dinfo: device information
 * @netif: netif information
 * @hwts_tx_type: TX Timestamp type
 *
 * Description: This is a callback function to enable/disable the RX timestamping port
 * based.
 */
int ngptpclock_ptp_hw_rx_tstamp_config(ngknet_dev_info_t *dinfo, ngknet_netif_t *netif,
        int *hwts_rx_filter)
{
#if defined(TWO_STEP_SUPPORT)
    uint64_t portmap = 0;
    int map = 0;
#endif

    if (!module_initialized) {
        return SHR_E_DISABLED;
    }

    DBG_VERB(("hw_rx_tstamp_config: Rx filter %d\n", *hwts_rx_filter));
    if (*hwts_rx_filter == HWTSTAMP_FILTER_NONE) {
        /* disable */
    } else {
        /* enable */
    }

#if defined(TWO_STEP_SUPPORT)
    DBG_VERB(("hw_tstamp_disable: Disable 2Step TS(type:%d) port = %d\n", tx_type, port));
    if (port <= 0) {
        DBG_ERR(("hw_tstamp_disable: Error disabling timestamp on port:%d\n", port));
        ret = -1;
        goto exit;
    }

    /* Update the shared structure member - Disable 2step on port */
    if (ptp_priv->shared_addr) {
        if ((port > 0) && (port < NGPTPCLOCK_MAX_NUM_PORTS)) {
            port -= 1;
            map = (port / 64);
            port = (port % 64);

            portmap = ptp_priv->shared_addr->portmap[map];
            portmap &= ~((uint64_t)0x1 << port);
            ptp_priv->shared_addr->portmap[map]= portmap;

            /* Command to R5 for the update */
            ptp_priv->shared_addr->ksyncinit = NGPTPCLOCK_PBM_UPDATE;
        }
    }
#endif

    return SHR_E_NONE;
}

int ngptpclock_ptp_transport_get(uint8_t *pkt)
{
    int         transport = 0;
    uint16_t    ethertype;
    uint16_t    tpid;
    int         tpid_offset, ethype_offset;

    /* Need to check VLAN tag if packet is tagged */
    tpid_offset = 12;
    tpid = pkt[tpid_offset] << 8 | pkt[tpid_offset + 1];
    if (tpid == 0x8100) {
        ethype_offset = tpid_offset + 4;
    } else {
        ethype_offset = tpid_offset;
    }

    ethertype = pkt[ethype_offset] << 8 | pkt[ethype_offset+1];

    switch (ethertype) {
        case 0x88f7:    /* ETHERTYPE_PTPV2 */
            transport = 2;
            break;

        case 0x0800:    /* ETHERTYPE_IPV4 */
            transport = 4;
            break;

        case 0x86DD:    /* ETHERTYPE_IPV6 */
            transport = 6;
            break;

        default:
            transport = 0;
    }

    return transport;
}

#if defined(TWO_STEP_SUPPORT)
static int
ngptpclock_txpkt_tsts_tsamp_get(int port, uint32_t pkt_seq_id, uint32_t *ts_valid,
        uint32_t *seq_id, uint64_t *timestamp)
{
    int ret = 0;
    uint64_t tmp;
    u32 fifo_rxctr = 0;

    tmp = (port & 0xFFFF) | (pkt_seq_id << 16);

    ret = ngptpclock_cmd_go(NGPTPCLOCK_GET_TSTIME, &tmp, timestamp);
    if (ret >= 0) {
        fifo_rxctr = (tmp >> 32) & 0xFFFF;
        *seq_id = ((tmp >> 16) & 0xFFFF);
        *ts_valid = (tmp & 0x1);
         if (*ts_valid) {
            tmp = (port & 0xFFFF) | (pkt_seq_id << 16);
            ngptpclock_cmd_go(NGPTPCLOCK_ACK_TSTIME, &tmp, 0);
            if (fifo_rxctr != 0) {
                if (fifo_rxctr != ptp_priv->port_stats[port].fifo_rxctr + 1) {
                    DBG_ERR(("FW Reset or Lost Timestamp RxSeq:(Prev %d : Current %d)\n",
                                ptp_priv->port_stats[port].fifo_rxctr, fifo_rxctr));
                }
                ptp_priv->port_stats[port].fifo_rxctr = fifo_rxctr;
            }
        }
    }


    return ret;
}
#endif


/**
 * ngptpclock_ptp_hw_tstamp_tx_time_get
 *
 * @skb: Linux socket buffer
 * @ts: timestamp to be retrieved
 *
 * Description: This is a callback function to retrieve the timestamp on
 * a given port
 * NOTE:
 * Two-step related - fetching the timestamp from portmacro, not needed for one-step
 */
int ngptpclock_ptp_hw_tstamp_tx_time_get(struct sk_buff *skb, uint64_t *ts)
{
#if defined(TWO_STEP_SUPPORT)
    /* Get Timestamp from R5 or CLMAC */
    uint32_t ts_valid = 0;
    uint32_t seq_id = 0;
    uint32_t pktseq_id = 0;
    uint64_t timestamp = 0;
    uint16_t tpid = 0;
    ktime_t start;
    u64 delta;
    int retry_cnt = num_retries;
    int seq_id_offset, tpid_offset;
    int transport = network_transport;
#endif

    struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);
    struct ngknet_ptp_data *ptpd = (struct ngknet_ptp_data *)cbd->netif->user_data;
    int hwts_tx_type = ptpd->hwts_tx_type;
    /* The first byte from the userdata is the phy_port number */
    int port = ptpd->phy_port;

#if defined(TWO_STEP_SUPPORT)
    int dev_no = cbd->dinfo->dev_no;
#endif

    if (!ptp_priv || !module_initialized) {
        return SHR_E_DISABLED;
    }

#if defined(TWO_STEP_SUPPORT)
    start = ktime_get();
#endif

    if (!ts || port < 1 || port > 255 || ptp_priv->shared_addr == NULL) {
        return SHR_E_DISABLED;
    }

    /* Reset */
    *ts = 0;

    if (hwts_tx_type == HWTSTAMP_TX_ONESTEP_SYNC) {
        *ts = *(uint64_t *)skb->cb;

        port -= 1;
        ptp_priv->port_stats[port].pkt_txctr += 1;
    } else {
#if defined(TWO_STEP_SUPPORT)
        tpid_offset = 12;

        /* Parse for nw transport */
        if (transport == 0) {
            transport = ngptpclock_ptp_transport_get(pkt);
        }

        switch (transport)
        {
            case 2:
                seq_id_offset = 0x2c;
                break;
            case 4:
                seq_id_offset = 0x48;
                break;
            case 6:
                seq_id_offset = 0x5c;
                break;
            default:
                seq_id_offset = 0x2c;
                break;
        }

        /* Need to check VLAN tag if packet is tagged */
        tpid = pkt[tpid_offset] << 8 | pkt[tpid_offset + 1];
        if (tpid == 0x8100) {
            seq_id_offset += 4;
        }

        pktseq_id = pkt[seq_id_offset] << 8 | pkt[seq_id_offset + 1];

        port -= 1;

        DBG_TXTS(("hw_tstamp_tx_time_get: port %d pktseq_id %u\n", port, pktseq_id));

        /* Fetch the TX timestamp from shadow memory */
        do {
            ngptpclock_txpkt_tsts_tsamp_get(port, pktseq_id, &ts_valid, &seq_id, &timestamp);
            if (ts_valid) {

                /* Clear the shadow memory to get next entry */
                ptp_priv->shared_addr->port_ts_data[port].timestamp = 0;
                ptp_priv->shared_addr->port_ts_data[port].port_id = 0;
                ptp_priv->shared_addr->port_ts_data[port].ts_seq_id = 0;
                ptp_priv->shared_addr->port_ts_data[port].ts_valid = 0;

                if (seq_id == pktseq_id) {
                    *ts = timestamp;
                    ptp_priv->port_stats[port].tsts_match += 1;

                    delta = ktime_us_delta(ktime_get(), start);
                    DBG_TXTS(("Port: %d Skb_SeqID %d FW_SeqId %d and TS:%llx FetchTime %lld\n",
                                port, pktseq_id, seq_id, timestamp, delta));

                    if ((delta < ptp_priv->port_stats[port].tsts_best_fetch_time) ||
                            (ptp_priv->port_stats[port].tsts_best_fetch_time == 0)) {
                        ptp_priv->port_stats[port].tsts_best_fetch_time = delta;
                    }
                    if ((delta > ptp_priv->port_stats[port].tsts_worst_fetch_time) ||
                            (ptp_priv->port_stats[port].tsts_worst_fetch_time == 0)) {
                        ptp_priv->port_stats[port].tsts_worst_fetch_time = delta;
                    }
                    /* Calculate Moving Average*/
                    ptp_priv->port_stats[port].tsts_avg_fetch_time = ((u32)delta +
                            ((ptp_priv->port_stats[port].tsts_match - 1) *
                            ptp_priv->port_stats[port].tsts_avg_fetch_time)) /
                            ptp_priv->port_stats[port].tsts_match;
                    break;
                } else {
                    DBG_TXTS(("Discard timestamp on port %d Skb_SeqID %d FW_SeqId %d "
                                "RetryCnt %d TimeLapsed (%lld us)\n",
                                port, pktseq_id, seq_id, (num_retries - retry_cnt),
                                ktime_us_delta(ktime_get(),start)));

                    ptp_priv->port_stats[port].tsts_discard += 1;
                    continue;
                }
            }
            ptp_sleep(1);
            retry_cnt--;
        } while (retry_cnt);


        ptp_priv->port_stats[port].pkt_txctr += 1;

        if (retry_cnt == 0) {
            ptp_priv->port_stats[port].tsts_timeout += 1;
            DBG_ERR(("FW Response timeout: Tx TS on phy port:%d Skb_SeqID: %d "
                        "TimeLapsed (%lld us)\n", port, pktseq_id,
                        ktime_us_delta(ktime_get(), start)));
        }
#endif
    }

    return SHR_E_NONE;
}

enum {
    CUSTOM_ENCAP_VERSION_INVALID = 0,
    CUSTOM_ENCAP_VERSION_ONE = 1,

    CUSTOM_ENCAP_VERSION_CURRENT = CUSTOM_ENCAP_VERSION_ONE,
    CUSTOM_ENCAP_VERSION_RSVD = 255
};

enum {
    CUSTOM_ENCAP_OPCODE_INVALID = 0,
    CUSTOM_ENCAP_OPCODE_PTP_RX = 1,
    CUSTOM_ENCAP_OPCODE_RSVD = 255
};

enum {
    CUSTOM_ENCAP_PTP_RX_TLV_INVALID = 0,
    CUSTOM_ENCAP_PTP_RX_TLV_PTP_RX_TIME = 1,
    CUSTOM_ENCAP_PTP_RX_TLV_RSVD = 255
};

static void
dbg_dump_pkt(uint8_t *data, int size)
{
    int idx;
    char str[128];

    for (idx = 0; idx < size; idx++) {
        if ((idx & 0xf) == 0) {
            sprintf(str, "%04x: ", idx);
        }
        sprintf(&str[strlen(str)], "%02x ", data[idx]);
        if ((idx & 0xf) == 0xf) {
            sprintf(&str[strlen(str)], "\n");
            printk(str);
        }
    }
    if ((idx & 0xf) != 0) {
        sprintf(&str[strlen(str)], "\n");
        printk(str);
    }
}

static inline int
ngptpclock_pkt_custom_encap_ptprx_get(uint8_t *pkt, uint64_t *ing_ptptime)
{
    uint8_t  *custom_hdr;
    uint8_t   id[4];
    uint8_t   ver, opc;
    uint8_t   nh_type, nh_rsvd;
    uint16_t  len, tot_len;
    uint16_t  nh_len;
    uint32_t  seq_id = 0;
    uint32_t  ptp_rx_time[2];
    uint64_t  u64_ptp_rx_time = 0;

    custom_hdr = pkt;

    NGPTPCLOCK_UNPACK_U8(custom_hdr, id[0]);
    NGPTPCLOCK_UNPACK_U8(custom_hdr, id[1]);
    NGPTPCLOCK_UNPACK_U8(custom_hdr, id[2]);
    NGPTPCLOCK_UNPACK_U8(custom_hdr, id[3]);
    if (!((id[0] == 'B') && (id[1] == 'C') && (id[2] == 'M') && (id[3] == 'C'))) {
        /* invalid signature */
        return -1;
    }

    NGPTPCLOCK_UNPACK_U8(custom_hdr, ver);
    switch (ver) {
        case CUSTOM_ENCAP_VERSION_CURRENT:
            break;
        default:
            DBG_ERR(("custom_encap_ptprx_get: Invalid ver\n"));
            return -1;
    }

    NGPTPCLOCK_UNPACK_U8(custom_hdr, opc);
    switch (opc) {
        case CUSTOM_ENCAP_OPCODE_PTP_RX:
            break;
        default:
            DBG_ERR(("custom_encap_ptprx_get: Invalid opcode\n"));
            return -1;
    }

    NGPTPCLOCK_UNPACK_U16(custom_hdr, len);
    NGPTPCLOCK_UNPACK_U32(custom_hdr, seq_id);
    tot_len = len;

    /* remaining length of custom encap */
    len = len - (custom_hdr - pkt);

    /* process tlv */
    while (len > 0) {
        NGPTPCLOCK_UNPACK_U8(custom_hdr, nh_type);
        NGPTPCLOCK_UNPACK_U8(custom_hdr, nh_rsvd);
        NGPTPCLOCK_UNPACK_U16(custom_hdr, nh_len);
        len = len - (nh_len);
        if (nh_rsvd != 0x0) {
            continue; /* invalid tlv */
        }

        switch (nh_type) {
            case CUSTOM_ENCAP_PTP_RX_TLV_PTP_RX_TIME:
                NGPTPCLOCK_UNPACK_U32(custom_hdr, ptp_rx_time[0]);
                NGPTPCLOCK_UNPACK_U32(custom_hdr, ptp_rx_time[1]);
                u64_ptp_rx_time = ((uint64_t)ptp_rx_time[1] << 32) | (uint64_t)ptp_rx_time[0];
                *ing_ptptime = u64_ptp_rx_time;
                break;
            default:
                custom_hdr += nh_len;
                break;
        }
    }

    DBG_RX_DUMP(("custom_encap_ptprx_get: Custom Encap header:\n"));
    if (debug & DBG_LVL_RX_DUMP) dbg_dump_pkt(pkt, tot_len);

    DBG_RX(("custom_encap_ptprx_get: ver=%d opcode=%d tot_len=%d seq_id=0x%x\n",
                ver, opc, tot_len, seq_id));

    return (tot_len);
}

/**
 * ngptpclock_ptp_hw_rx_pre_process
 *
 * @skb: Linux socket buffer
 * @cust_hdr_len: Custom header length
 *
 * Description: Parse the packet to check if customer is present and return the header length.
 */
int ngptpclock_ptp_hw_rx_pre_process(struct sk_buff *skb, uint32_t *cust_hdr_len)
{
    uint64_t ts;
    int custom_encap_len = 0;
    struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);
    int pkt_hdr_len = 32 + cbd->pmd_len;
    uint8_t *data = skb->data + pkt_hdr_len;

    custom_encap_len = ngptpclock_pkt_custom_encap_ptprx_get(data, &ts);

    DBG_RX(("hw_rx_pre_process: cust_encap_len=0x%x\n", custom_encap_len));

    if ((cust_hdr_len) && (custom_encap_len >= 0)) {
        *cust_hdr_len = custom_encap_len;
    } else if (cust_hdr_len) {
        *cust_hdr_len = 0;
    }
    return SHR_E_NONE;
}

/**
 * ngptpclock_ptp_hw_tstamp_rx_time_upscale
 *
 * @skb: Linxu socket buffer
 * @ts: timestamp to be retrieved
 *
 * Description: This is a callback function to retrieve 64b equivalent of
 *   rx timestamp
 */
int ngptpclock_ptp_hw_tstamp_rx_time_upscale(struct sk_buff *skb, uint64_t *ts)
{
    int ret = SHR_E_NONE;
    int custom_encap_len = 0;
    uint16_t tpid = 0;
    uint16_t msgtype_offset = 0;
    int transport = network_transport;
    int ptp_hdr_offset = 0, ptp_message_len = 0;

    struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);
    /* The first byte from the userdata is the phy_port number */
    int port = (int)cbd->netif->user_data[0];

    if (!module_initialized || !ptp_priv || (ptp_priv->shared_addr == NULL)) {
        return SHR_E_DISABLED;
    }

    DBG_RX_DUMP(("rxtime_upscale: Incoming packet: \n"));
    if (debug & DBG_LVL_RX_DUMP) dbg_dump_pkt(skb->data, skb->len);






    /* parse custom encap header in pkt for ptp rxtime */
    custom_encap_len = ngptpclock_pkt_custom_encap_ptprx_get((skb->data), ts);

    /* Remove the custom encap header from pkt */
    if (custom_encap_len > 0) {

        skb_pull(skb, custom_encap_len);

        DBG_RX_DUMP(("rxtime_upscale: After removing custom encap: \n"));
        if (debug & DBG_LVL_RX_DUMP) dbg_dump_pkt(skb->data, skb->len);

        msgtype_offset = ptp_hdr_offset = 0;
        tpid = SKB_U16_GET(skb, (12));
        if (tpid == 0x8100) {
            msgtype_offset += 4;
            ptp_hdr_offset += 4;
        }

        /* Parse for nw transport */
        transport = ngptpclock_ptp_transport_get(skb->data);

        switch (transport)
        {
            case 2: /* IEEE 802.3 */
                ptp_hdr_offset += 14;
                break;
            case 4: /* UDP IPv4   */
                ptp_hdr_offset += 42;
                break;
            case 6: /* UDP IPv6   */
                ptp_hdr_offset += 62;
                break;
            default:
                ptp_hdr_offset += 42;
                break;
        }

        ptp_message_len = SKB_U16_GET(skb, (ptp_hdr_offset + 2));

        DBG_RX(("rxtime_upscale: custom_encap_len %d tpid 0x%x transport %d skb->len %d "
                    "ptp message type %d, ptp_message_len %d\n",
                    custom_encap_len, tpid, transport, skb->len,
                    skb->data[msgtype_offset] & 0x0F, ptp_message_len));
    }

    if ((port > 0) && (port < NGPTPCLOCK_MAX_NUM_PORTS)) {
        port -= 1;
        ptp_priv->port_stats[port].pkt_rxctr += 1;
    }

    return ret;
}

void ngptpclock_hton64(u8 *buf, const uint64_t *data)
{
#ifdef __LITTLE_ENDIAN
  /* LITTLE ENDIAN */
  buf[0] = (*(((uint8_t*)(data)) + 7u));
  buf[1] = (*(((uint8_t*)(data)) + 6u));
  buf[2] = (*(((uint8_t*)(data)) + 5u));
  buf[3] = (*(((uint8_t*)(data)) + 4u));
  buf[4] = (*(((uint8_t*)(data)) + 3u));
  buf[5] = (*(((uint8_t*)(data)) + 2u));
  buf[6] = (*(((uint8_t*)(data)) + 1u));
  buf[7] = (*(((uint8_t*)(data)) + 0u));
#else
  memcpy(buf, data, 8);
#endif
}

int ngptpclock_ptp_hw_tstamp_tx_meta_set(struct sk_buff *skb)
{
    uint16_t tpid = 0;
    int md_offset = 0;
    int pkt_offset = 0;
    int ptp_hdr_offset = 0;
    int transport = network_transport;
    s64 ptptime  = 0;
    s64 ptpcounter = 0;
    int64_t corrField;
    int32_t negCurTS32;
    int64_t negCurTS64;

    struct ngknet_callback_desc *cbd = NGKNET_SKB_CB(skb);
    struct ngknet_ptp_data *ptpd = (struct ngknet_ptp_data *)cbd->netif->user_data;
    int hwts_tx_type = ptpd->hwts_tx_type;
    int hdrlen = cbd->pmd_len;
    u32 md[4];
    /* The first byte from the userdata is the phy_port number */
    int port = ptpd->phy_port;

    if (!module_initialized || !ptp_priv || (ptp_priv->shared_addr == NULL)) {
        return SHR_E_DISABLED;
    }

    if (ptp_priv->ptp_pair_lock == 1) {
        /* use alternate pair when main dataset is being updated */
        ptptime = ptp_priv->shared_addr->ptptime_alt;
        ptpcounter = ptp_priv->shared_addr->reftime_alt;
    } else {
        ptptime = ptp_priv->shared_addr->ptptime;
        ptpcounter = ptp_priv->shared_addr->reftime;
    }

    negCurTS32 = - (int32_t) ptpcounter;
    negCurTS64 = - (int64_t)(ptpcounter);




    if (CMICX_DEV_TYPE) {
        pkt_offset = ptp_hdr_offset = hdrlen + 32;
    }

    /* Need to check VLAN tag if packet is tagged */
    tpid = SKB_U16_GET(skb, (pkt_offset + 12));
    if (tpid == 0x8100) {
        md_offset = 4;
        ptp_hdr_offset += 4;
    }

    /* One Step Meta Data */
    if (hwts_tx_type == HWTSTAMP_TX_ONESTEP_SYNC) {
        md_offset += 8;
        corrField = (((int64_t)negCurTS64) << 16);
    }

    /* Parse for nw transport */
    if (transport == 0) {
        transport = ngptpclock_ptp_transport_get(skb->data + pkt_offset);
    }

    memcpy(md, cbd->pmd, sizeof(md));
    switch (transport)
    {
        case 2: /* IEEE 802.3 */
            ptp_hdr_offset += 14;
            if (cbd->pmd) {
                md[0] |= (ieee1588_l2pkt_md[md_offset]);
                md[1] |= (ieee1588_l2pkt_md[md_offset + 1]);
                md[2] |= (ieee1588_l2pkt_md[md_offset + 2]);
                md[3] |= (ieee1588_l2pkt_md[md_offset + 3]);
            }
            break;
        case 4: /* UDP IPv4   */
            ptp_hdr_offset += 42;
            if (cbd->pmd) {
                md[0] |= (ieee1588_ipv4pkt_md[md_offset]);
                md[1] |= (ieee1588_ipv4pkt_md[md_offset + 1]);
                md[2] |= (ieee1588_ipv4pkt_md[md_offset + 2]);
                md[3] |= (ieee1588_ipv4pkt_md[md_offset + 3]);
            }
            break;
        case 6: /* UDP IPv6   */
            ptp_hdr_offset += 62;
            if (cbd->pmd) {
                md[0] |= (ieee1588_ipv6pkt_md[md_offset]);
                md[1] |= (ieee1588_ipv6pkt_md[md_offset + 1]);
                md[2] |= (ieee1588_ipv6pkt_md[md_offset + 2]);
                md[3] |= (ieee1588_ipv6pkt_md[md_offset + 3]);
            }
            break;
        default:
            ptp_hdr_offset += 42;
            if (cbd->pmd) {
                md[0] |= (ieee1588_ipv4pkt_md[md_offset]);
                md[1] |= (ieee1588_ipv4pkt_md[md_offset + 1]);
                md[2] |= (ieee1588_ipv4pkt_md[md_offset + 2]);
                md[3] |= (ieee1588_ipv4pkt_md[md_offset + 3]);
            }
            break;
    }
    memcpy(cbd->pmd, md, sizeof(md));

    DBG_TX(("hw_tstamp_tx_meta_get: ptptime: 0x%llx ptpcounter: 0x%llx\n", ptptime, ptpcounter));
    DBG_TX(("hw_tstamp_tx_meta_get: ptpmessage offset:%u type: 0x%x hwts_tx_type: %d\n",
                ptp_hdr_offset, skb->data[ptp_hdr_offset] & 0x0f, hwts_tx_type));

    if ((hwts_tx_type == HWTSTAMP_TX_ONESTEP_SYNC) &&
            (NGPTPCLOCK_PTP_EVENT_MSG((skb->data[ptp_hdr_offset] & 0x0F)))) {
        /* One Step Timestamp Field updation */
        int corr_offset = ptp_hdr_offset + 8;
        int origin_ts_offset = ptp_hdr_offset + 34;
        u32 tmp;
        struct timespec64 ts = {0};
        int udp_csum_regen;
        u32 udp_csum20;
        u16 udp_csum;

        udp_csum = SKB_U16_GET(skb, (ptp_hdr_offset - 2));

        switch (transport) {
            case 2:
                udp_csum_regen = 0;
                break;
            case 6:
                udp_csum_regen = 1;
                break;
            default:
                udp_csum_regen = (udp_csum != 0x0);
                break;
        }

        /* Fill the correction field */
        ngptpclock_hton64(&(skb->data[corr_offset]), (const u64 *)&corrField);

        /* Fill the Origin Timestamp Field */
        ts = ns_to_timespec64(ptptime);

        tmp = (ts.tv_sec >> 32);
        skb->data[origin_ts_offset + 0] = ((tmp >>  8) & 0xFF);
        skb->data[origin_ts_offset + 1] = ((tmp      ) & 0xFF);

        tmp = (ts.tv_sec & 0xFFFFFFFFLL);
        skb->data[origin_ts_offset + 2] = ((tmp >> 24) & 0xFF);
        skb->data[origin_ts_offset + 3] = ((tmp >> 16) & 0xFF);
        skb->data[origin_ts_offset + 4] = ((tmp >>  8) & 0xFF);
        skb->data[origin_ts_offset + 5] = ((tmp      ) & 0xFF);

        tmp = (ts.tv_nsec & 0xFFFFFFFFLL);
        skb->data[origin_ts_offset + 6] = ((tmp >> 24) & 0xFF);
        skb->data[origin_ts_offset + 7] = ((tmp >> 16) & 0xFF);
        skb->data[origin_ts_offset + 8] = ((tmp >>  8) & 0xFF);
        skb->data[origin_ts_offset + 9] = ((tmp      ) & 0xFF);

        if (udp_csum_regen) {
            udp_csum20 = (~udp_csum) & 0xFFFF;

            udp_csum20 += SKB_U16_GET(skb, (corr_offset + 0));
            udp_csum20 += SKB_U16_GET(skb, (corr_offset + 2));
            udp_csum20 += SKB_U16_GET(skb, (corr_offset + 4));
            udp_csum20 += SKB_U16_GET(skb, (corr_offset + 6));

            udp_csum20 += SKB_U16_GET(skb, (origin_ts_offset + 0));
            udp_csum20 += SKB_U16_GET(skb, (origin_ts_offset + 2));
            udp_csum20 += SKB_U16_GET(skb, (origin_ts_offset + 4));
            udp_csum20 += SKB_U16_GET(skb, (origin_ts_offset + 6));
            udp_csum20 += SKB_U16_GET(skb, (origin_ts_offset + 8));

            /* Fold 20bit checksum into 16bit udp checksum */
            udp_csum20 = ((udp_csum20 & 0xFFFF) + (udp_csum20 >> 16));
            udp_csum = ((udp_csum20 & 0xFFFF) + (udp_csum20 >> 16));

            /* invert again to get final checksum. */
            udp_csum = ~udp_csum;
            if (udp_csum == 0) {
                udp_csum = 0xFFFF;
            }

            skb->data[ptp_hdr_offset - 2] = ((udp_csum >>  8) & 0xFF);
            skb->data[ptp_hdr_offset - 1] = ((udp_csum      ) & 0xFF);
        }

        if ((skb->data[ptp_hdr_offset] & 0x0F) == IEEE1588_MSGTYPE_DELREQ) {
            *(uint64_t *)skb->cb = ptptime;
        } else {
            *(uint64_t *)skb->cb = 0;
        }

        DBG_TX(("hw_tstamp_tx_meta_get: ptp msg type %d packet tstamp : 0x%llx corrField: 0x%llx\n",
                    (skb->data[ptp_hdr_offset] & 0x0F), ptptime, corrField));

        if ((port > 0) && (port < NGPTPCLOCK_MAX_NUM_PORTS)) {
            port -= 1;
            ptp_priv->port_stats[port].pkt_txonestep += 1;
        }
    }

    DBG_TX_DUMP(("hw_tstamp_tx_meta_get: PTP Packet\n"));
    if (debug & DBG_LVL_TX_DUMP) dbg_dump_pkt(skb->data, skb->len);

    return 0;
}

int ngptpclock_ptp_hw_tstamp_ptp_clock_index_get(ngknet_dev_info_t *dinfo,
        ngknet_netif_t *netif, int *index)
{
    if (!module_initialized || !ptp_priv) {
        return SHR_E_DISABLED;
    }

    if (ptp_priv && ptp_priv->ptp_clock)
        *index =  ptp_clock_index(ptp_priv->ptp_clock);

    return SHR_E_NONE;
}

/**
* bcm_ptp_time_keep - call timecounter_read every second to avoid timer overrun
*                 because  a 32bit counter, will timeout in 4s
*/
static void
ngptpclock_ptp_time_keep(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    struct ngptpclock_ptp_priv *priv =
                        container_of(dwork, struct ngptpclock_ptp_priv, time_keep);
    struct timespec64 ts;

    /* Call bcm_ptp_gettime function to keep the ref_time_64 and ref_counter_48 in sync */
    ngptpclock_ptp_gettime(&(priv->ptp_caps), &ts);
    schedule_delayed_work(&priv->time_keep, HZ);
}

static void
ngptpclock_ptp_time_keep_init(void)
{
    if (!ptp_priv->timekeep_status) {
        INIT_DELAYED_WORK(&(ptp_priv->time_keep), ngptpclock_ptp_time_keep);
        schedule_delayed_work(&ptp_priv->time_keep, HZ);

        ptp_priv->timekeep_status = 1;
    }

    return;
}

static void
ngptpclock_ptp_time_keep_cleanup(void)
{
    if (ptp_priv->timekeep_status) {
        /* Cancel delayed work */
        cancel_delayed_work_sync(&(ptp_priv->time_keep));

        ptp_priv->timekeep_status = 0;
    }

    return;
}

/* PTP_EXTTS logging */
static void
ngptpclock_ptp_extts_logging(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    struct ngptpclock_ptp_priv *priv = container_of(dwork, struct ngptpclock_ptp_priv, extts_logging);
    struct ptp_clock_event event;
    int event_id = -1;
    int head = -1, tail = -1;

    if (!module_initialized || ptp_priv->extts_log == NULL)
        goto exit;

    if (ptp_priv->extts_log->overflow) {
        DBG_VERB(("Queue overflow state:%u\n", ptp_priv->extts_log->overflow));
    }

    tail = (int)ptp_priv->extts_log->tail;
    head = ptp_priv->extts_event.head;

    head = (head + 1) % NUM_EVENT_TS;
    while (tail != head) {
        switch (ptp_priv->extts_log->event_ts[head].ts_event_id) {
            /* Map FW event_id to EXTTS event_id */
            case TS_EVENT_GPIO_1:
                event_id = 0;
                break;
            case TS_EVENT_GPIO_2:
                event_id = 1;
                break;
            case TS_EVENT_GPIO_3:
                event_id = 2;
                break;
            case TS_EVENT_GPIO_4:
                event_id = 3;
                break;
            case TS_EVENT_GPIO_5:
                event_id = 4;
                break;
            case TS_EVENT_GPIO_6:
                event_id = 5;
                break;
        }

        if (event_id < 0 || ptp_priv->extts_event.enable[event_id] != 1) {
            memset((void *)&(ptp_priv->extts_log->event_ts[head]), 0,
                    sizeof(ptp_priv->extts_log->event_ts[head]));

            ptp_priv->extts_event.head = head;
            ptp_priv->extts_log->head = head;

            head = (head + 1) % NUM_EVENT_TS;
            continue;
        }

        event.type = PTP_CLOCK_EXTTS;
        event.index = event_id;
        event.timestamp = ((s64)ptp_priv->extts_log->event_ts[head].tstamp.sec * 1000000000) +
            ptp_priv->extts_log->event_ts[head].tstamp.nsec;
        ptp_clock_event(ptp_priv->ptp_clock, &event);

        ptp_priv->extts_event.head = head;
        ptp_priv->extts_log->head = head;

        head = (head + 1) % NUM_EVENT_TS;
    }
exit:
    schedule_delayed_work(&priv->extts_logging, __msecs_to_jiffies(100));
}

static void
ngptpclock_ptp_extts_logging_init(void)
{
    INIT_DELAYED_WORK(&(ptp_priv->extts_logging), ngptpclock_ptp_extts_logging);
    schedule_delayed_work(&ptp_priv->extts_logging, __msecs_to_jiffies(100));
}

static void
ngptpclock_ptp_extts_logging_cleanup(void)
{
    cancel_delayed_work_sync(&(ptp_priv->extts_logging));
}

static int
ngptpclock_ptp_init(struct ptp_clock_info *ptp)
{
    int ret = -1;

    ret = ngptpclock_cmd_go(NGPTPCLOCK_INIT, NULL, NULL);
    DBG_VERB(("ptp_init: NGPTPCLOCK_INIT; rv:%d\n", ret));
    if (ret < 0) goto err_exit;
    ptp_sleep(1);

err_exit:
    return ret;
}

static int
ngptpclock_ptp_cleanup(struct ptp_clock_info *ptp)
{
    int ret = -1;

    ngptpclock_ptp_time_keep_cleanup();

    ret = ngptpclock_cmd_go(NGPTPCLOCK_CLEANUP, NULL, NULL);
    DBG_VERB(("ptp_cleanup: rv:%d\n", ret));

    return ret;
}

static int
ngptpclock_broadsync_cmd(int bs_id)
{
    int ret = -1;
    u64 subcmd, subcmd_data;

    subcmd = (bs_id == 0) ? NGPTPCLOCK_BROADSYNC_BS0_CONFIG : NGPTPCLOCK_BROADSYNC_BS1_CONFIG;

    subcmd_data =  ((ptp_priv->ngptpclock_bs_info[bs_id]).enable & 0x1);
    subcmd_data |= (((ptp_priv->ngptpclock_bs_info[bs_id]).mode & 0x1) << 8);
    subcmd_data |= ((ptp_priv->ngptpclock_bs_info[bs_id]).hb << 16);
    subcmd_data |= (((u64)(ptp_priv->ngptpclock_bs_info[bs_id]).bc) << 32);

    ret = ngptpclock_cmd_go(NGPTPCLOCK_BROADSYNC, &subcmd, &subcmd_data);
    DBG_VERB(("ngptpclock_broadsync_cmd: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n",
                subcmd, subcmd_data, ret));

    return ret;
}

static int
ngptpclock_broadsync_status_cmd(int bs_id, u64 *status)
{
    int ret = -1;
    u64 subcmd;

    subcmd = (bs_id == 0) ? NGPTPCLOCK_BROADSYNC_BS0_STATUS_GET : NGPTPCLOCK_BROADSYNC_BS1_STATUS_GET;

    ret = ngptpclock_cmd_go(NGPTPCLOCK_BROADSYNC, &subcmd, status);
    DBG_VERB(("ngptpclock_broadsync_status_cmd: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n",
                subcmd, *status, ret));

    return ret;
}

static int
ngptpclock_gpio_cmd(int gpio_num)
{
    int ret = -1;
    u64 subcmd, subcmd_data;

    switch (gpio_num) {
        case 0:
            subcmd = NGPTPCLOCK_GPIO_0;
            break;
        case 1:
            subcmd = NGPTPCLOCK_GPIO_1;
            break;
        case 2:
            subcmd = NGPTPCLOCK_GPIO_2;
            break;
        case 3:
            subcmd = NGPTPCLOCK_GPIO_3;
            break;
        case 4:
            subcmd = NGPTPCLOCK_GPIO_4;
            break;
        case 5:
            subcmd = NGPTPCLOCK_GPIO_5;
            break;
        default:
            return ret;
    }

    subcmd_data =  ((ptp_priv->ngptpclock_gpio_info[gpio_num]).enable & 0x1);
    subcmd_data |= (((ptp_priv->ngptpclock_gpio_info[gpio_num]).mode & 0x1) << 8);
    subcmd_data |= ((u64)((ptp_priv->ngptpclock_gpio_info[gpio_num]).period) << 16);

    ret = ngptpclock_cmd_go(NGPTPCLOCK_GPIO, &subcmd, &subcmd_data);
    DBG_VERB(("ngptpclock_gpio_cmd: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n",
                subcmd, subcmd_data, ret));

    return ret;
}

static int
ngptpclock_gpio_phaseoffset_cmd(int gpio_num)
{
    int ret = -1;
    u64 subcmd, subcmd_data;

    switch (gpio_num) {
        case 0:
            subcmd = NGPTPCLOCK_GPIO_0;
            break;
        case 1:
            subcmd = NGPTPCLOCK_GPIO_1;
            break;
        case 2:
            subcmd = NGPTPCLOCK_GPIO_2;
            break;
        case 3:
            subcmd = NGPTPCLOCK_GPIO_3;
            break;
        case 4:
            subcmd = NGPTPCLOCK_GPIO_4;
            break;
        case 5:
            subcmd = NGPTPCLOCK_GPIO_5;
            break;
        default:
            return ret;
    }

    subcmd_data = (ptp_priv->ngptpclock_gpio_info[gpio_num]).phase_offset;
    ret = ngptpclock_cmd_go(NGPTPCLOCK_GPIO_PHASEOFFSET, &subcmd, &subcmd_data);
    DBG_VERB(("ngptpclock_cmd_go: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));

    return ret;
}

static int
ngptpclock_evlog_cmd(int event, int enable)
{
    int ret;
    int addr_offset;
    u64 subcmd = 0, subcmd_data = 0;
    ngptpclock_evlog_t tmp;

    subcmd = event;
    addr_offset = ((u8 *)&(tmp.event_timestamps[event]) - (u8 *)&(tmp.event_timestamps[0]));

    if (enable) {
        subcmd_data = (ptp_priv->dma_mem + addr_offset);
    } else {
        subcmd_data = 0;
    }

    ret = ngptpclock_cmd_go(NGPTPCLOCK_EVLOG, &subcmd, &subcmd_data);
    DBG_VERB(("ngptpclock_evlog_cmd: subcmd: 0x%llx subcmd_data: 0x%llx rv:%d\n",
                subcmd, subcmd_data, ret));

    return ret;
}


/*
 * Device Debug Statistics Proc Entry
 */
/**
* This function is called at the beginning of a sequence.
* ie, when:
*    - the /proc/linux_ngptpclock/stats file is read (first time)
*   - after the function stop (end of sequence)
*
*/
static void *ngptpclock_proc_seq_start(struct seq_file *s, loff_t *pos)
{
   /* beginning a new sequence ? */
   if ( (int)*pos == 0 && ptp_priv->shared_addr != NULL)
   {
       seq_printf(s, "TwoStep Port Bitmap : %08llx%08llx\n",
               (uint64_t)(ptp_priv->shared_addr->portmap[1]),
               (uint64_t)(ptp_priv->shared_addr->portmap[0]));
        seq_printf(s,"%4s| %9s| %9s| %9s| %9s| %9s| %9s| %9s| %9s| %9s| %9s| %9s\n",
                "Port", "RxCounter", "TxCounter", "TxOneStep", "TSTimeout",
                "TSRead", "TSMatch", "TSDiscard",
                "TimeHi" , "TimeLo", "TimeAvg", "FIFORx");
   }

   if ((int)*pos < (ptp_priv->num_pports))
        return (void *)(unsigned long)(*pos + 1);
   /* End of the sequence, return NULL */
   return NULL;
 }

/**
* This function is called after the beginning of a sequence.
* It's called untill the return is NULL (this ends the sequence).
*
*/
static void *ngptpclock_proc_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    (*pos)++;
    return ngptpclock_proc_seq_start(s, pos);
}
/**
* This function is called at the end of a sequence
*
*/
static void
ngptpclock_proc_seq_stop(struct seq_file *s, void *v)
{
    /* nothing to do, we use a static value in ngptpclock_proc_seq_start() */
}

/**
* This function is called for each "step" of a sequence
*
*/
static int
ngptpclock_proc_seq_show(struct seq_file *s, void *v)
{
    unsigned long port = (unsigned long)v;

    if ((port > 0) && (port < NGPTPCLOCK_MAX_NUM_PORTS)) {
        port = port - 1;
        if (ptp_priv->port_stats[port].pkt_rxctr || ptp_priv->port_stats[port].pkt_txctr ||
                ptp_priv->port_stats[port].pkt_txonestep||
                ptp_priv->port_stats[port].tsts_discard ||
                ptp_priv->port_stats[port].tsts_timeout ||
                ptp_priv->shared_addr->port_ts_data[port].ts_cnt ||
                ptp_priv->port_stats[port].tsts_match) {
            seq_printf(s, "%4lu | %9d| %9d| %9d| %9d| %9d| %9d| %9d| %9lld| %9lld | %9d|%9d | %s\n",
                    (port + 1),
                    ptp_priv->port_stats[port].pkt_rxctr,
                    ptp_priv->port_stats[port].pkt_txctr,
                    ptp_priv->port_stats[port].pkt_txonestep,
                    ptp_priv->port_stats[port].tsts_timeout,
                    ptp_priv->shared_addr->port_ts_data[port].ts_cnt,
                    ptp_priv->port_stats[port].tsts_match,
                    ptp_priv->port_stats[port].tsts_discard,
                    ptp_priv->port_stats[port].tsts_worst_fetch_time,
                    ptp_priv->port_stats[port].tsts_best_fetch_time,
                    ptp_priv->port_stats[port].tsts_avg_fetch_time,
                    ptp_priv->port_stats[port].fifo_rxctr,
                    ((ptp_priv->port_stats[port].pkt_txctr != ptp_priv->port_stats[port].tsts_match) ?
                    "***":""));
        }
    }
    return 0;
}

/**
* seq_operations for bsync_proc_*** entries
*
*/
static struct seq_operations ngptpclock_proc_seq_ops = {
    .start = ngptpclock_proc_seq_start,
    .next  = ngptpclock_proc_seq_next,
    .stop  = ngptpclock_proc_seq_stop,
    .show  = ngptpclock_proc_seq_show
};

static int
ngptpclock_proc_txts_open(struct inode * inode, struct file * file)
{
    return seq_open(file, &ngptpclock_proc_seq_ops);
}

static ssize_t
ngptpclock_proc_txts_write(struct file *file, const char *buf,
                      size_t count, loff_t *loff)
{
    char debug_str[40];
    char *ptr;
    int port;

    if (copy_from_user(debug_str, buf, count)) {
        return -EFAULT;
    }

    if ((ptr = strstr(debug_str, "clear")) != NULL) {
        for (port = 0; port < ptp_priv->num_pports; port++) {
            ptp_priv->port_stats[port].pkt_rxctr = 0;
            ptp_priv->port_stats[port].pkt_txctr = 0;
            ptp_priv->port_stats[port].pkt_txonestep = 0;
            ptp_priv->port_stats[port].tsts_timeout = 0;
            ptp_priv->port_stats[port].tsts_match = 0;
            ptp_priv->port_stats[port].tsts_discard = 0;
            if (ptp_priv->shared_addr)
                ptp_priv->shared_addr->port_ts_data[port].ts_cnt = 0;
        }
    } else {
        DBG_ERR(("Warning: unknown input\n"));
    }

    return count;
}

struct proc_ops ngptpclock_proc_txts_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =       ngptpclock_proc_txts_open,
    .proc_read =       seq_read,
    .proc_lseek =      seq_lseek,
    .proc_write =      ngptpclock_proc_txts_write,
    .proc_release =    seq_release,
};

/*
 * Driver Debug Proc Entry
 */
static int
ngptpclock_proc_debug_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Configuration:\n");
    seq_printf(m, "  debug:          0x%x\n", debug);
    return 0;
}

static ssize_t
ngptpclock_proc_debug_write(struct file *file, const char *buf,
                      size_t count, loff_t *loff)
{
    char debug_str[40];
    char *ptr;

    if (copy_from_user(debug_str, buf, count)) {
        return -EFAULT;
    }

    if ((ptr = strstr(debug_str, "debug=")) != NULL) {
        ptr += 6;
        debug = simple_strtol(ptr, NULL, 0);
    } else {
        DBG_ERR(("Warning: unknown configuration\n"));
    }

    return count;
}

static int
ngptpclock_proc_debug_open(struct inode * inode, struct file * file)
{
    return single_open(file, ngptpclock_proc_debug_show, NULL);
}

struct proc_ops ngptpclock_proc_debug_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =       ngptpclock_proc_debug_open,
    .proc_read =       seq_read,
    .proc_lseek =      seq_lseek,
    .proc_write =      ngptpclock_proc_debug_write,
    .proc_release =    single_release,
};

static int
ngptpclock_proc_init(void)
{
    struct proc_dir_entry *entry;

    PROC_CREATE(entry, "stats", 0666, ngptpclock_proc_root,
                &ngptpclock_proc_txts_file_ops);
    if (entry == NULL) {
        return -1;
    }
    PROC_CREATE(entry, "debug", 0666, ngptpclock_proc_root,
                &ngptpclock_proc_debug_file_ops);
    if (entry == NULL) {
        return -1;
    }
    return 0;
}

static int
ngptpclock_proc_cleanup(void)
{
    remove_proc_entry("stats", ngptpclock_proc_root);
    remove_proc_entry("debug", ngptpclock_proc_root);
    return 0;
}


#define ATTRCMP(x) (0 == strcmp(attr->attr.name, #x))

static int rd_iter=0, wr_iter=0;
static ssize_t
bs_attr_store(struct kobject *kobj, struct kobj_attribute *attr,
              const char *buf, size_t bytes)
{
    ssize_t ret;
    u32 enable, mode;
    u32 bc, hb;

    if (ATTRCMP(bs0)) {
        ret = sscanf(buf, "enable:%d mode:%d bc:%u hb:%u",
                     &enable, &mode, &bc, &hb);
        DBG_VERB(("rd:%d bs0: enable:%d mode:%d bc:%d hb:%d\n",
                  rd_iter++, enable, mode, bc, hb));
        ptp_priv->ngptpclock_bs_info[0].enable = enable;
        ptp_priv->ngptpclock_bs_info[0].mode = mode;
        ptp_priv->ngptpclock_bs_info[0].bc   = bc;
        ptp_priv->ngptpclock_bs_info[0].hb   = hb;

        (void)ngptpclock_broadsync_cmd(0);
    } else if (ATTRCMP(bs1)) {
        ret = sscanf(buf, "enable:%d mode:%d bc:%u hb:%u",
                     &enable, &mode, &bc, &hb);
        DBG_VERB(("rd:%d bs1: enable:%d mode:%d bc:%d hb:%d\n",
                   rd_iter++, enable, mode, bc, hb));
        ptp_priv->ngptpclock_bs_info[1].enable = enable;
        ptp_priv->ngptpclock_bs_info[1].mode = mode;
        ptp_priv->ngptpclock_bs_info[1].bc   = bc;
        ptp_priv->ngptpclock_bs_info[1].hb   = hb;

        (void)ngptpclock_broadsync_cmd(1);
    } else {
        ret = -ENOENT;
    }

    return (ret == -ENOENT) ? ret : bytes;
}

static ssize_t
bs_attr_show(struct kobject *kobj, struct kobj_attribute *attr,
             char *buf)
{
    ssize_t bytes;
    u64 status = 0;
    u32 variance = 0;

    if (ATTRCMP(bs0)) {

        if (ptp_priv->ngptpclock_bs_info[0].enable) {
            (void)ngptpclock_broadsync_status_cmd(0, &status);
        }

        variance = (status >> 32);
        status = (status & 0xFFFFFFFF);
        bytes = sprintf(buf, "enable:%d mode:%d bc:%u hb:%u status:%u(%u)\n",
                        ptp_priv->ngptpclock_bs_info[0].enable,
                        ptp_priv->ngptpclock_bs_info[0].mode,
                        ptp_priv->ngptpclock_bs_info[0].bc,
                        ptp_priv->ngptpclock_bs_info[0].hb,
                        (u32)status,
                        variance);
        DBG_VERB(("wr:%d bs0: enable:%d mode:%d bc:%u hb:%u status:%u(%u)\n",
                        wr_iter++,
                        ptp_priv->ngptpclock_bs_info[0].enable,
                        ptp_priv->ngptpclock_bs_info[0].mode,
                        ptp_priv->ngptpclock_bs_info[0].bc,
                        ptp_priv->ngptpclock_bs_info[0].hb,
                        (u32)status,
                        variance));
    } else if (ATTRCMP(bs1)) {

        if (ptp_priv->ngptpclock_bs_info[1].enable) {
            (void)ngptpclock_broadsync_status_cmd(1, &status);
        }

        variance = (status >> 32);
        status = (status & 0xFFFFFFFF);
        bytes = sprintf(buf, "enable:%d mode:%d bc:%u hb:%u status:%u(%u)\n",
                        ptp_priv->ngptpclock_bs_info[1].enable,
                        ptp_priv->ngptpclock_bs_info[1].mode,
                        ptp_priv->ngptpclock_bs_info[1].bc,
                        ptp_priv->ngptpclock_bs_info[1].hb,
                        (u32)status,
                        variance);
        DBG_VERB(("wr:%d bs1: enable:%d mode:%d bc:%u hb:%u status:%u(%u)\n",
                        wr_iter++,
                        ptp_priv->ngptpclock_bs_info[1].enable,
                        ptp_priv->ngptpclock_bs_info[1].mode,
                        ptp_priv->ngptpclock_bs_info[1].bc,
                        ptp_priv->ngptpclock_bs_info[1].hb,
                        (u32)status,
                        variance));
    } else {
        bytes = -ENOENT;
    }

    return bytes;
}

#define BS_ATTR(x)                         \
    static struct kobj_attribute x##_attribute =        \
        __ATTR(x, 0664, bs_attr_show, bs_attr_store);

BS_ATTR(bs0)
BS_ATTR(bs1)

#define BS_ATTR_LIST(x)    & x ## _attribute.attr
static struct attribute *bs_attrs[] = {
    BS_ATTR_LIST(bs0),
    BS_ATTR_LIST(bs1),
    NULL,       /* terminator */
};

static struct attribute_group bs_attr_group = {
    .name   = "broadsync",
    .attrs  = bs_attrs,
};


static int gpio_rd_iter=0, gpio_wr_iter=0;
static ssize_t
gpio_attr_store(struct kobject *kobj, struct kobj_attribute *attr,
                const char *buf, size_t bytes)
{
    ssize_t ret;
    int gpio;
    u32 enable, mode;
    u32 period;
    int64_t phase_offset;

    if (ATTRCMP(gpio0)) {
        gpio = 0;
    } else if (ATTRCMP(gpio1)) {
        gpio = 1;
    } else if (ATTRCMP(gpio2)) {
        gpio = 2;
    } else if (ATTRCMP(gpio3)) {
        gpio = 3;
    } else if (ATTRCMP(gpio4)) {
        gpio = 4;
    } else if (ATTRCMP(gpio5)) {
        gpio = 5;
    } else {
        return -ENOENT;
    }


    ret = sscanf(buf, "enable:%d mode:%d period:%u phaseoffset:%lld",
                       &enable, &mode, &period, &phase_offset);
    DBG_VERB(("rd:%d gpio%d: enable:%d mode:%d period:%d phaseoffset:%lld\n",
               gpio_rd_iter++, gpio, enable, mode, period, phase_offset));
    ptp_priv->ngptpclock_gpio_info[gpio].enable = enable;
    ptp_priv->ngptpclock_gpio_info[gpio].mode = mode;
    ptp_priv->ngptpclock_gpio_info[gpio].period = period;

    if (phase_offset != ptp_priv->ngptpclock_gpio_info[gpio].phase_offset) {
        ptp_priv->ngptpclock_gpio_info[gpio].phase_offset = phase_offset;
        (void)ngptpclock_gpio_phaseoffset_cmd(gpio);
    }

    (void)ngptpclock_gpio_cmd(gpio);

    return (ret == -ENOENT) ? ret : bytes;
}

static ssize_t
gpio_attr_show(struct kobject *kobj, struct kobj_attribute *attr,
               char *buf)
{
    ssize_t bytes;
    int gpio;

    if (ATTRCMP(gpio0)) {
        gpio = 0;
    } else if (ATTRCMP(gpio1)) {
        gpio = 1;
    } else if (ATTRCMP(gpio2)) {
        gpio = 2;
    } else if (ATTRCMP(gpio3)) {
        gpio = 3;
    } else if (ATTRCMP(gpio4)) {
        gpio = 4;
    } else if (ATTRCMP(gpio5)) {
        gpio = 5;
    } else {
        return -ENOENT;
    }

    bytes = sprintf(buf, "enable:%d mode:%d period:%u phaseoffset:%lld\n",
                    ptp_priv->ngptpclock_gpio_info[gpio].enable,
                    ptp_priv->ngptpclock_gpio_info[gpio].mode,
                    ptp_priv->ngptpclock_gpio_info[gpio].period,
                    ptp_priv->ngptpclock_gpio_info[gpio].phase_offset);
    DBG_VERB(("wr:%d gpio%d: enable:%d mode:%d period:%u phaseoffset:%lld\n",
                    gpio_wr_iter++, gpio,
                    ptp_priv->ngptpclock_gpio_info[gpio].enable,
                    ptp_priv->ngptpclock_gpio_info[gpio].mode,
                    ptp_priv->ngptpclock_gpio_info[gpio].period,
                    ptp_priv->ngptpclock_gpio_info[gpio].phase_offset));

    return bytes;
}

#define GPIO_ATTR(x)                         \
    static struct kobj_attribute x##_attribute =        \
        __ATTR(x, 0664, gpio_attr_show, gpio_attr_store);

GPIO_ATTR(gpio0)
GPIO_ATTR(gpio1)
GPIO_ATTR(gpio2)
GPIO_ATTR(gpio3)
GPIO_ATTR(gpio4)
GPIO_ATTR(gpio5)

#define GPIO_ATTR_LIST(x)    & x ## _attribute.attr
static struct attribute *gpio_attrs[] = {
    GPIO_ATTR_LIST(gpio0),
    GPIO_ATTR_LIST(gpio1),
    GPIO_ATTR_LIST(gpio2),
    GPIO_ATTR_LIST(gpio3),
    GPIO_ATTR_LIST(gpio4),
    GPIO_ATTR_LIST(gpio5),
    NULL,       /* terminator */
};

static struct attribute_group gpio_attr_group = {
    .name   = "gpio",
    .attrs  = gpio_attrs,
};



static ssize_t
evlog_attr_store(struct kobject *kobj, struct kobj_attribute *attr,
                 const char *buf, size_t bytes)
{
    ssize_t ret;
    int event, enable;

    if (ATTRCMP(cpu)) {
        event = 0;
    } else if (ATTRCMP(bs0)) {
        event = 1;
    } else if (ATTRCMP(bs1)) {
        event = 2;
    } else if (ATTRCMP(gpio0)) {
        event = 3;
    } else if (ATTRCMP(gpio1)) {
        event = 4;
    } else if (ATTRCMP(gpio2)) {
        event = 5;
    } else if (ATTRCMP(gpio3)) {
        event = 6;
    } else if (ATTRCMP(gpio4)) {
        event = 7;
    } else if (ATTRCMP(gpio5)) {
        event = 8;
    } else {
        return -ENOENT;
    }


    ret = sscanf(buf, "enable:%d", &enable);
    DBG_VERB(("event:%d: enable:%d\n", event, enable));

    (void)ngptpclock_evlog_cmd(event, enable);
    ptp_priv->ngptpclock_evlog_info[event].enable = enable;

    return (ret == -ENOENT) ? ret : bytes;
}

static ssize_t
evlog_attr_show(struct kobject *kobj, struct kobj_attribute *attr,
                char *buf)
{
    ssize_t bytes;
    int event;

    if (ATTRCMP(cpu)) {
        event = 0;
    } else if (ATTRCMP(bs0)) {
        event = 1;
    } else if (ATTRCMP(bs1)) {
        event = 2;
    } else if (ATTRCMP(gpio0)) {
        event = 3;
    } else if (ATTRCMP(gpio1)) {
        event = 4;
    } else if (ATTRCMP(gpio2)) {
        event = 5;
    } else if (ATTRCMP(gpio3)) {
        event = 6;
    } else if (ATTRCMP(gpio4)) {
        event = 7;
    } else if (ATTRCMP(gpio5)) {
        event = 8;
    } else {
        return -ENOENT;
    }


    bytes = sprintf(buf, "enable:%d Previous Time:%llu.%09u Latest Time:%llu.%09u\n",
                    ptp_priv->ngptpclock_evlog_info[event].enable,
                    ptp_priv->evlog->event_timestamps[event].prv_tstamp.sec,
                    ptp_priv->evlog->event_timestamps[event].prv_tstamp.nsec,
                    ptp_priv->evlog->event_timestamps[event].cur_tstamp.sec,
                    ptp_priv->evlog->event_timestamps[event].cur_tstamp.nsec);
    DBG_VERB(("event%d: enable:%d Previous Time:%llu.%09u Latest Time:%llu.%09u\n",
                    event,
                    ptp_priv->ngptpclock_evlog_info[event].enable,
                    ptp_priv->evlog->event_timestamps[event].prv_tstamp.sec,
                    ptp_priv->evlog->event_timestamps[event].prv_tstamp.nsec,
                    ptp_priv->evlog->event_timestamps[event].cur_tstamp.sec,
                    ptp_priv->evlog->event_timestamps[event].cur_tstamp.nsec));

    memset((void *)&(ptp_priv->evlog->event_timestamps[event]), 0,
            sizeof(ptp_priv->evlog->event_timestamps[event]));

    return bytes;
}

#define EVLOG_ATTR(x)                         \
    static struct kobj_attribute evlog_ ## x ##_attribute =        \
        __ATTR(x, 0664, evlog_attr_show, evlog_attr_store);

EVLOG_ATTR(bs0)
EVLOG_ATTR(bs1)
EVLOG_ATTR(gpio0)
EVLOG_ATTR(gpio1)
EVLOG_ATTR(gpio2)
EVLOG_ATTR(gpio3)
EVLOG_ATTR(gpio4)
EVLOG_ATTR(gpio5)

#define EVLOG_ATTR_LIST(x)    & evlog_ ## x ## _attribute.attr
static struct attribute *evlog_attrs[] = {
    EVLOG_ATTR_LIST(bs0),
    EVLOG_ATTR_LIST(bs1),
    EVLOG_ATTR_LIST(gpio0),
    EVLOG_ATTR_LIST(gpio1),
    EVLOG_ATTR_LIST(gpio2),
    EVLOG_ATTR_LIST(gpio3),
    EVLOG_ATTR_LIST(gpio4),
    EVLOG_ATTR_LIST(gpio5),
    NULL,       /* terminator */
};

static struct attribute_group evlog_attr_group = {
    .name   = "evlog",
    .attrs  = evlog_attrs,
};

static int
ngptpclock_sysfs_init(void)
{
    int ret = 0;
    struct kobject *parent;
    struct kobject *root = &((((struct module *)(THIS_MODULE))->mkobj).kobj);

    parent = root;
    ptp_priv->kobj = kobject_create_and_add("io", parent);

    ret = sysfs_create_group(ptp_priv->kobj, &bs_attr_group);

    ret = sysfs_create_group(ptp_priv->kobj, &gpio_attr_group);

    ret = sysfs_create_group(ptp_priv->kobj, &evlog_attr_group);

    return ret;
}

static int
ngptpclock_sysfs_cleanup(void)
{
    int ret = 0;
    struct kobject *parent;

    parent = ptp_priv->kobj;

    sysfs_remove_group(parent, &bs_attr_group);
    sysfs_remove_group(parent, &gpio_attr_group);
    sysfs_remove_group(parent, &evlog_attr_group);

    kobject_put(ptp_priv->kobj);


    return ret;
}


static void
ngptpclock_ptp_fw_data_alloc(int dev_no)
{
    dma_addr_t dma_mem = 0;

    /* Initialize the Base address for CMIC and shared Memory access */
    ptp_priv->base_addr = ngbde_kapi_pio_membase(dev_no);
    ptp_priv->dma_dev = ngbde_kapi_dma_dev_get(dev_no);

    ptp_priv->dma_mem_size = sizeof(ngptpclock_evlog_t); /*sizeof(ngptpclock_evlog_t);*/

    if (ptp_priv->evlog == NULL) {
        DBG_ERR(("Allocate memory for event log\n"));
        ptp_priv->evlog = DMA_ALLOC_COHERENT(ptp_priv->dma_dev,
                                                   ptp_priv->dma_mem_size,
                                                   &dma_mem);
    }

    if (ptp_priv->evlog != NULL) {
        /* Reset memory */
        memset((void *)ptp_priv->evlog, 0, ptp_priv->dma_mem_size);

        ptp_priv->dma_mem = dma_mem;
        DBG_ERR(("Shared memory allocation (%d bytes) for event log successful at 0x%016lx.\n",
                ptp_priv->dma_mem_size, (long unsigned int)ptp_priv->dma_mem));
    }

    ptp_priv->extts_log = NULL;
    ptp_priv->extts_event.head = -1;
    return;
}

static void
ngptpclock_ptp_fw_data_free(void)
{
    if (ptp_priv->evlog != NULL) {
        DBG_ERR(("Free shared memory : extts log of %d bytes\n",
                 ptp_priv->dma_mem_size));
        DMA_FREE_COHERENT(ptp_priv->dma_dev, ptp_priv->dma_mem_size,
                              (void *)ptp_priv->evlog, ptp_priv->dma_mem);
        ptp_priv->evlog = NULL;
    }

    return;
}

static void
ngptpclock_ptp_dma_init(int dcb_type, int dev_no)
{
    int endianess;
    int num_pports = 256;
    int mem_size = 16384; /*sizeof(ngptpclock_info_t);*/

    ptp_priv->num_pports = num_pports;
    ptp_priv->dcb_type = dcb_type;

    ngptpclock_ptp_fw_data_alloc(dev_no);

    if (ptp_priv->shared_addr == NULL) {
        ptp_priv->shared_addr = kzalloc(16384, GFP_KERNEL);
        ptp_priv->port_stats = kzalloc((sizeof(ngptpclock_port_stats_t) * num_pports), GFP_KERNEL);
    }

    if (ptp_priv->shared_addr != NULL) {
        /* Reset memory. */
        memset((void *)ptp_priv->shared_addr, 0, mem_size);

#ifdef __LITTLE_ENDIAN
        endianess = 0;
#else
        endianess = 1;
#endif
        DEV_WRITE32(ptp_priv, CMIC_CMC_SCHAN_MESSAGE_14r(CMIC_CMC_BASE),
                ((pci_cos << 16) | endianess));

        DEV_WRITE32(ptp_priv, CMIC_CMC_SCHAN_MESSAGE_15r(CMIC_CMC_BASE), 1);
        DEV_WRITE32(ptp_priv, CMIC_CMC_SCHAN_MESSAGE_16r(CMIC_CMC_BASE), 1);

    }

    DBG_VERB(("%s %p:%p, dcb_type: %d\n", __FUNCTION__, ptp_priv->base_addr,
                (void *)ptp_priv->shared_addr, dcb_type));

    ptp_priv->mirror_encap_bmp = 0x0;

    hostcmd_regs[0] = CMIC_CMC_SCHAN_MESSAGE_21r(CMIC_CMC_BASE);
    hostcmd_regs[1] = CMIC_CMC_SCHAN_MESSAGE_20r(CMIC_CMC_BASE);
    hostcmd_regs[2] = CMIC_CMC_SCHAN_MESSAGE_19r(CMIC_CMC_BASE);
    hostcmd_regs[3] = CMIC_CMC_SCHAN_MESSAGE_18r(CMIC_CMC_BASE);
    hostcmd_regs[4] = CMIC_CMC_SCHAN_MESSAGE_17r(CMIC_CMC_BASE);

    return;
}


/**
 * ngptpclock_ioctl_cmd_handler
 * @dev_info: Device information
 * @cmd: sub command
 * @data: sub command data
 * @len: Sub command length
 * Description: This function will handle ioctl commands from user mode.
 */
static int
ngptpclock_ioctl_cmd_handler(ngknet_dev_info_t *dev_info, int cmd, char *data, int len)
{
    u32 fw_status;
    int32_t *cfg_data  = (int32_t *)data;

    if (!module_initialized && cmd != NGPTPCLOCK_HW_INIT) {
        return SHR_E_CONFIG;
    }

    switch (cmd) {
        case NGPTPCLOCK_HW_INIT:
            pci_cos = cfg_data[0];
            fw_core = cfg_data[1];
            DBG_VERB(("Configuring pci_cosq:%d dev_no:%d fw_core:%d\n",
                        pci_cos, dev_info->dev_no, fw_core));
            if ((CMICX_DEV_TYPE && (fw_core >= 0 && fw_core <= 3)) ||
                (fw_core == 0 || fw_core == 1)) {
                memcpy(ieee1588_l2pkt_md, &cfg_data[12], sizeof(ieee1588_l2pkt_md));
                memcpy(ieee1588_ipv4pkt_md, &cfg_data[36], sizeof(ieee1588_ipv4pkt_md));
                memcpy(ieee1588_ipv6pkt_md, &cfg_data[60], sizeof(ieee1588_ipv6pkt_md));




                ngptpclock_ptp_dma_init(1, dev_info->dev_no);

                fw_status = 0;
                DEV_READ32(ptp_priv, CMIC_CMC_SCHAN_MESSAGE_21r(CMIC_CMC_BASE), &fw_status);

                /* Return success if the app is already initialized. */
                if (module_initialized) {
                    return SHR_E_NONE;
                }

                /* Return error if the app is not ready yet. */
                if (fw_status != 0xBADC0DE1) {
                    return SHR_E_RESOURCE;
                }

                (ptp_priv->ngptpclock_init_info).uc_port_num = cfg_data[2];
                (ptp_priv->ngptpclock_init_info).uc_port_sysport = cfg_data[3];
                (ptp_priv->ngptpclock_init_info).host_cpu_port = cfg_data[4];
                (ptp_priv->ngptpclock_init_info).host_cpu_sysport = cfg_data[5];
                (ptp_priv->ngptpclock_init_info).udh_len = cfg_data[6];

                DBG_VERB(("fw_core:%d uc_port:%d uc_sysport:%d pci_port:%d pci_sysport:%d\n",
                        fw_core,
                        (ptp_priv->ngptpclock_init_info).uc_port_num,
                        (ptp_priv->ngptpclock_init_info).uc_port_sysport,
                        (ptp_priv->ngptpclock_init_info).host_cpu_port,
                        (ptp_priv->ngptpclock_init_info).host_cpu_sysport));

                if (ngptpclock_ptp_init(&(ptp_priv->ptp_caps)) >= 0) {
                    module_initialized = 1;
                }
            }
            break;
        case NGPTPCLOCK_HW_CLEANUP:
            module_initialized = 0;

            DEV_WRITE32(ptp_priv, CMIC_CMC_SCHAN_MESSAGE_15r(CMIC_CMC_BASE), 0);
            DEV_WRITE32(ptp_priv, CMIC_CMC_SCHAN_MESSAGE_16r(CMIC_CMC_BASE), 0);

            ngptpclock_ptp_cleanup(&(ptp_priv->ptp_caps));
            break;
#if defined(HW_TS_DISABLE)
        case NGPTPCLOCK_M_HW_TS_DISABLE:
            ngptpclock_ptp_hw_tx_tstamp_config(NULL, NULL, 0);
            break;
#endif
#if defined(MIRROR_ENCAP_SUPPORT)
        case NGPTPCLOCK_M_MTP_TS_UPDATE_ENABLE:
            ngptpclock_ptp_mirror_encap_update(0, kmsg->clock_info.data[0], TRUE);
            break;
        case NGPTPCLOCK_M_MTP_TS_UPDATE_DISABLE:
            ngptpclock_ptp_mirror_encap_update(0, kmsg->clock_info.data[0], FALSE);
            break;
#endif
        default:
            return SHR_E_NOT_FOUND;
    }

    return SHR_E_NONE;
}

/**
 * ngptpclock_ptp_register
 * @priv: driver private structure
 * Description: this function will register the ptp clock driver
 * to kernel. It also does some house keeping work.
 */
static int
ngptpclock_ptp_register(void)
{
    int err = -ENODEV;

    if (CMICX_DEV_TYPE) {
        if (fw_core < 0 || fw_core > 3) {
            goto exit;
        }
    } else if (fw_core < 0 || fw_core > 1) {
        /* Support on core-0 or core-1 */
        goto exit;
    }

    /* default transport is raw, ieee 802.3 */
    switch (network_transport) {
        case 2: /* IEEE 802.3 */
        case 4: /* UDP IPv4   */
        case 6: /* UDP IPv6   */
            break;
        default:
            network_transport = 0;
    }

    ptp_priv = kzalloc(sizeof(*ptp_priv), GFP_KERNEL);
    if (!ptp_priv) {
        err = -ENOMEM;
        goto exit;
    }

    /* Reset memory */
    memset(ptp_priv, 0, sizeof(*ptp_priv));

    err = -ENODEV;

    ptp_priv->ptp_caps = ngptpclock_ptp_caps;

    mutex_init(&(ptp_priv->ptp_lock));

    /* Register ptp clock driver with ngptpclock_ptp_caps */
    ptp_priv->ptp_clock = ptp_clock_register(&ptp_priv->ptp_caps, NULL);

    if (IS_ERR(ptp_priv->ptp_clock)) {
        ptp_priv->ptp_clock = NULL;
    } else if (ptp_priv->ptp_clock) {
        err = 0;

        /* Register NGKNET HW Timestamp Callback Functions */
        ngknet_ptp_dev_ctrl_cb_register(ngptpclock_ioctl_cmd_handler);
        ngknet_ptp_tx_config_set_cb_register(ngptpclock_ptp_hw_tx_tstamp_config);
        ngknet_ptp_phc_index_get_cb_register(ngptpclock_ptp_hw_tstamp_ptp_clock_index_get);
        ngknet_ptp_rx_config_set_cb_register(ngptpclock_ptp_hw_rx_tstamp_config);
        ngknet_ptp_tx_meta_set_cb_register(ngptpclock_ptp_hw_tstamp_tx_meta_set);
        ngknet_ptp_tx_hwts_get_cb_register(ngptpclock_ptp_hw_tstamp_tx_time_get);
        ngknet_ptp_rx_hwts_get_cb_register(ngptpclock_ptp_hw_tstamp_rx_time_upscale);
        ngknet_ptp_rx_pre_process_cb_register(ngptpclock_ptp_hw_rx_pre_process);
    }

     /* Initialize proc files */
     ngptpclock_proc_root = proc_mkdir(NGPTPCLOCK_MODULE_NAME, NULL);;
     ngptpclock_proc_init();
     ngptpclock_sysfs_init();
     ptp_priv->shared_addr = NULL;
     ptp_priv->port_stats = NULL;

     ngptpclock_ptp_extts_logging_init();
exit:
    return err;
}

static int
ngptpclock_ptp_remove(void)
{
    if (!ptp_priv)
        return 0;

    module_initialized = 0;

    ngptpclock_ptp_extts_logging_cleanup();

    ngptpclock_ptp_time_keep_cleanup();

    ngptpclock_proc_cleanup();
    ngptpclock_sysfs_cleanup();
    remove_proc_entry(NGPTPCLOCK_MODULE_NAME, NULL);

    /* UnRegister NGKNET HW Timestamp Callback Functions */
    ngknet_ptp_dev_ctrl_cb_unregister(ngptpclock_ioctl_cmd_handler);
    ngknet_ptp_tx_config_set_cb_unregister(ngptpclock_ptp_hw_tx_tstamp_config);
    ngknet_ptp_phc_index_get_cb_unregister(ngptpclock_ptp_hw_tstamp_ptp_clock_index_get);
    ngknet_ptp_rx_config_set_cb_unregister(ngptpclock_ptp_hw_rx_tstamp_config);
    ngknet_ptp_tx_meta_set_cb_unregister(ngptpclock_ptp_hw_tstamp_tx_meta_set);
    ngknet_ptp_tx_hwts_get_cb_register(ngptpclock_ptp_hw_tstamp_tx_time_get);
    ngknet_ptp_rx_hwts_get_cb_unregister(ngptpclock_ptp_hw_tstamp_rx_time_upscale);
    ngknet_ptp_rx_pre_process_cb_unregister(ngptpclock_ptp_hw_rx_pre_process);

    /* Cleanup the PTP */
    ngptpclock_ptp_cleanup(&(ptp_priv->ptp_caps));

    ngptpclock_ptp_fw_data_free();

    if (ptp_priv->port_stats != NULL) {
        kfree((void *)ptp_priv->port_stats);
        ptp_priv->port_stats = NULL;
    }
    if (ptp_priv->shared_addr != NULL) {
        kfree((void *)ptp_priv->shared_addr);
        ptp_priv->shared_addr = NULL;
        DBG_ERR(("Free R5 memory\n"));
    }

    /* Unregister the bcm ptp clock driver */
    ptp_clock_unregister(ptp_priv->ptp_clock);

    /* Free Memory */
    kfree(ptp_priv);

    return 0;
}
#endif /* NGPTPCLOCK_SUPPORT */

/*
 * Function: ngptpclock_init_module
 *
 * Purpose:
 *    Module initialization.
 *    Attached SOC all devices and optionally initializes these.
 * Parameters:
 *    None
 * Returns:
 *    0 on success, otherwise -1
 */
static int __init
ngptpclock_init_module(void)
{
#ifdef NGPTPCLOCK_SUPPORT
    ngptpclock_ptp_register();
    return 0;
#else
    printk(KERN_WARNING "%s: PTP not supported by this kernel.\n",
           NGPTPCLOCK_MODULE_NAME);
    return -ENOSYS;
#endif /* NGPTPCLOCK_SUPPORT */
}

/*
 * Function: ngptpclock_exit_module
 *
 * Purpose:
 *    Module cleanup function
 * Parameters:
 *    None
 * Returns:
 *    Always 0
 */
static void __exit
ngptpclock_exit_module(void)
{
#ifdef NGPTPCLOCK_SUPPORT
    ngptpclock_ptp_remove();
#endif /* NGPTPCLOCK_SUPPORT */
}

module_init(ngptpclock_init_module);
module_exit(ngptpclock_exit_module);
