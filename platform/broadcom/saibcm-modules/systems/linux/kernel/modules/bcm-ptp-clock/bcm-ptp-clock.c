/*
 * Copyright 2017 Broadcom
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 */

/*
 * This module implements a Linux PTP Clock driver for Broadcom
 * XGS switch devices.
 *
 * For a list of supported module parameters, please see below.
 *   debug: Debug level (default 0)
 *   network_transport : Transport Type (default 0 - Raw)
 *   base_dev_name: Base device name (default ptp0, ptp1, etc.)
 *
 * - All the data structures and functions work on the physical port.
 *   For array indexing purposes, we use (phy_port - 1).
 */

#include <gmodule.h> /* Must be included first */
/* Module Information */
#define MODULE_MAJOR 125
#define MODULE_NAME "linux-bcm-ptp-clock"

MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("PTP Clock Driver for Broadcom XGS/DNX Switch");
MODULE_LICENSE("GPL");

#if (LINUX_VERSION_CODE > KERNEL_VERSION(3,17,0))
#define PTPCLOCK_SUPPORTED
#endif

#ifdef PTPCLOCK_SUPPORTED
#include <linux-bde.h>
#include <kcom.h>
#include <bcm-knet.h>
#include <linux/time64.h>
#include <linux/delay.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/sysfs.h>
#include <linux-bde.h>

#include <linux/random.h>
#include <linux/seq_file.h>
#include <linux/if_vlan.h>
#include <linux/ptp_clock_kernel.h>

/* Configuration Parameters */
static int debug;
LKM_MOD_PARAM(debug, "i", int, 0);
MODULE_PARM_DESC(debug,
        "Debug level (default 0)");

static int pci_cos;

static int network_transport;
LKM_MOD_PARAM(network_transport, "i", int, 0);
MODULE_PARM_DESC(network_transport,
        "Transport Type (default - Detect from packet)");

static char *base_dev_name = "ptp0";
LKM_MOD_PARAM(base_dev_name, "s", charp, 0);
MODULE_PARM_DESC(base_dev_name,
        "Base device name (default ptp0, ptp1, etc.)");

static int fw_core;
LKM_MOD_PARAM(fw_core, "i", int, 0);
MODULE_PARM_DESC(fw_core,
        "Firmware core (default 0)");

static int vnptp_l2hdr_vlan_prio;
LKM_MOD_PARAM(vnptp_l2hdr_vlan_prio, "i", int, 0);
MODULE_PARM_DESC(vnptp_l2hdr_vlan_prio,
        "L2 Hdr Vlan priority");

static int phc_update_intv_msec = 1000;
LKM_MOD_PARAM(phc_update_intv_msec, "i", int, 0);
MODULE_PARM_DESC(phc_update_intv_msec,
        "PHC update interval in msec (default 1000)");

static int master_core = 0;
LKM_MOD_PARAM(master_core, "i", int, 0);
MODULE_PARM_DESC(master_core,
        "Master Core ID, this is specific to Q3D (default - 0)");

static int shared_phc = 0;
LKM_MOD_PARAM(shared_phc, "i", int, 0);
MODULE_PARM_DESC(shared_phc,
        "Single PHC instance of master_unit will shared with all units (default - 0)");

/* Debug levels */
#define DBG_LVL_VERB    0x1
#define DBG_LVL_WARN    0x2
#define DBG_LVL_TXTS    0x4
#define DBG_LVL_CMDS    0x8
#define DBG_LVL_TX      0x10
#define DBG_LVL_RX      0x20
#define DBG_LVL_TX_DUMP 0x40
#define DBG_LVL_RX_DUMP 0x80

#define DBG_VERB(_s)    do { if (debug & DBG_LVL_VERB) gprintk _s; } while (0)
#define DBG_WARN(_s)    do { if (debug & DBG_LVL_WARN) gprintk _s; } while (0)
#define DBG_TXTS(_s)    do { if (debug & DBG_LVL_TXTS) gprintk _s; } while (0)
#define DBG_CMDS(_s)    do { if (debug & DBG_LVL_CMDS) gprintk _s; } while (0)
#define DBG_TX(_s)      do { if (debug & DBG_LVL_TX) gprintk _s; } while (0)
#define DBG_RX(_s)      do { if (debug & DBG_LVL_RX) gprintk _s; } while (0)
#define DBG_TX_DUMP(_s) do { if (debug & DBG_LVL_TX_DUMP) gprintk _s; } while (0)
#define DBG_RX_DUMP(_s) do { if (debug & DBG_LVL_RX_DUMP) gprintk _s; } while (0)
#define DBG_ERR(_s)     do { if (1) gprintk _s; } while (0)


#ifdef LINUX_BDE_DMA_DEVICE_SUPPORT
#define DMA_DEV         device
#define DMA_ALLOC_COHERENT(d,s,h)       dma_alloc_coherent(d,s,h,GFP_ATOMIC|GFP_DMA32)
#define DMA_FREE_COHERENT(d,s,a,h)      dma_free_coherent(d,s,a,h)
#else
#define DMA_DEV         pci_dev
#define DMA_ALLOC_COHERENT(d,s,h)       pci_alloc_consistent(d,s,h)
#define DMA_FREE_COHERENT(d,s,a,h)      pci_free_consistent(d,s,a,h)
#endif

/* Type length in bytes */
#define BKSYNC_PACKLEN_U8     1
#define BKSYNC_PACKLEN_U16    2
#define BKSYNC_PACKLEN_U24    3
#define BKSYNC_PACKLEN_U32    4

#define BKSYNC_UNPACK_U8(_buf, _var)        \
    _var = *_buf++

#define BKSYNC_UNPACK_U16(_buf, _var)       \
    do {                                    \
        (_var) = (((_buf)[0] << 8) |        \
                  (_buf)[1]);               \
        (_buf) += BKSYNC_PACKLEN_U16;       \
    } while (0)

#define BKSYNC_UNPACK_U24(_buf, _var)       \
    do {                                    \
        (_var) = (((_buf)[0] << 16) |       \
                  ((_buf)[1] << 8)  |       \
                  (_buf)[2]);               \
        (_buf) += BKSYNC_PACKLEN_U24;       \
    } while (0)

#define BKSYNC_UNPACK_U32(_buf, _var)       \
    do {                                    \
        (_var) = (((_buf)[0] << 24) |       \
                  ((_buf)[1] << 16) |       \
                  ((_buf)[2] << 8)  |       \
                  (_buf)[3]);               \
        (_buf) += BKSYNC_PACKLEN_U32;       \
    } while (0)


#define CMICX_DEV_TYPE  \
            ((dev_info->dcb_type == 38) ||      \
            (dev_info->dcb_type == 36) ||       \
            (dev_info->dcb_type == 39))         \

/* Arad Series of DNX Devices */
#define DEVICE_IS_DPP       (dev_info->dcb_type == 28)

/* JR2 and JR3 Series of DNX Devices */
#define DEVICE_IS_DNX       (dev_info->dcb_type == 39)

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

u32 hostcmd_regs[5] = { 0 };

#define BKSYNC_NUM_PORTS            128    /* NUM_PORTS where 2-step is supported. */
#define BKSYNC_MAX_NUM_PORTS        512    /* Max ever NUM_PORTS in the system */
#define BKSYNC_MAX_MTP_IDX          8    /* Max number of mtps in the system */

#define BKSYNC_DNX_PTCH_1_SIZE         3       /* PTCH_1 */
#define BKSYNC_DNX_PTCH_2_SIZE         2       /* PTCH_2 */
#define BKSYNC_DNX_ITMH_SIZE           5       /* ITMH */

/* Service request commands to Firmware. */
enum {
#ifdef BDE_EDK_SUPPORT
    BKSYNC_DONE                     = (0x0),
    BKSYNC_INIT                     = (0x1),
    BKSYNC_DEINIT                   = (0x2),
    BKSYNC_GETTIME                  = (0x3),
    BKSYNC_SETTIME                  = (0x4),
    BKSYNC_FREQCOR                  = (0x5),
    BKSYNC_PBM_UPDATE               = (0x6),
    BKSYNC_ADJTIME                  = (0x7),
    BKSYNC_GET_TSTIME               = (0x8),
    BKSYNC_MTP_TS_UPDATE_ENABLE     = (0x9),
    BKSYNC_MTP_TS_UPDATE_DISABLE    = (0xa),
    BKSYNC_ACK_TSTIME               = (0xb),
    BKSYNC_SYSINFO                  = (0xc),
    BKSYNC_BROADSYNC                = (0xd),
    BKSYNC_GPIO                     = (0xe),
    BKSYNC_EVLOG                    = (0xf),
    BKSYNC_EXTTSLOG                 = (0x10),
    BKSYNC_GET_EXTTS_BUFF           = (0x11),
    BKSYNC_GPIO_PHASEOFFSET         = (0x12),
    BKSYNC_PTP_TOD                  = (0x13),
    BKSYNC_NTP_TOD                  = (0x14),
    BKSYNC_PTP_TOD_GET              = (0x15),
    BKSYNC_NTP_TOD_GET              = (0x16),
#else
    BKSYNC_DONE                     = (0x1),
    BKSYNC_INIT                     = (0x2),
    BKSYNC_DEINIT                   = (0x3),
    BKSYNC_GETTIME                  = (0x4),
    BKSYNC_SETTIME                  = (0x5),
    BKSYNC_FREQCOR                  = (0x6),
    BKSYNC_PBM_UPDATE               = (0x7),
    BKSYNC_ADJTIME                  = (0x8),
    BKSYNC_GET_TSTIME               = (0x9),
    BKSYNC_MTP_TS_UPDATE_ENABLE     = (0xa),
    BKSYNC_MTP_TS_UPDATE_DISABLE    = (0xb),
    BKSYNC_ACK_TSTIME               = (0xc),
    BKSYNC_SYSINFO                  = (0xd),
    BKSYNC_BROADSYNC                = (0xe),
    BKSYNC_GPIO                     = (0xf),
    BKSYNC_EVLOG                    = (0x10),
    BKSYNC_EXTTSLOG                 = (0x11),
    BKSYNC_GPIO_PHASEOFFSET         = (0x12),
#endif
};

enum {
    BKSYNC_SYSINFO_UC_PORT_NUM       = (0x1),
    BKSYNC_SYSINFO_UC_PORT_SYSPORT   = (0x2),
    BKSYNC_SYSINFO_HOST_CPU_PORT     = (0x3),
    BKSYNC_SYSINFO_HOST_CPU_SYSPORT  = (0x4),
    BKSYNC_SYSINFO_UDH_LEN           = (0x5),
};

enum {
    BKSYNC_BROADSYNC_BS0_CONFIG           = (0x1),
    BKSYNC_BROADSYNC_BS1_CONFIG           = (0x2),
    BKSYNC_BROADSYNC_BS0_STATUS_GET       = (0x3),
    BKSYNC_BROADSYNC_BS1_STATUS_GET       = (0x4),
    BKSYNC_BROADSYNC_BS0_PHASE_OFFSET_SET = (0x5),
    BKSYNC_BROADSYNC_BS1_PHASE_OFFSET_SET = (0x6),
};

enum {
    BKSYNC_GPIO_0                    = (0x1),
    BKSYNC_GPIO_1                    = (0x2),
    BKSYNC_GPIO_2                    = (0x3),
    BKSYNC_GPIO_3                    = (0x4),
    BKSYNC_GPIO_4                    = (0x5),
    BKSYNC_GPIO_5                    = (0x6),
};

/* 1588 message types. */
enum
{
    IEEE1588_MSGTYPE_SYNC           = (0x0),
    IEEE1588_MSGTYPE_DELREQ         = (0x1),
    IEEE1588_MSGTYPE_PDELREQ        = (0x2),
    IEEE1588_MSGTYPE_PDELRESP       = (0x3),
    /* reserved                       (0x4) */
    /* reserved                       (0x5) */
    /* reserved                       (0x6) */
    /* reserved                       (0x7) */
    IEEE1588_MSGTYPE_GENERALMASK    = (0x8),    /* all non-event messages have this bit set */
    IEEE1588_MSGTYPE_FLWUP          = (0x8),
    IEEE1588_MSGTYPE_DELRESP        = (0x9),
    IEEE1588_MSGTYPE_PDELRES_FLWUP  = (0xA),
    IEEE1588_MSGTYPE_ANNOUNCE       = (0xB),
    IEEE1588_MSGTYPE_SGNLNG         = (0xC),
    IEEE1588_MSGTYPE_MNGMNT         = (0xD)
    /* reserved                       (0xE) */
    /* reserved                       (0xF) */
};

/* Usage macros */
#define ONE_BILLION (1000000000)

#define BKSYNC_SKB_U16_GET(_skb, _pkt_offset) \
            ((_skb->data[_pkt_offset] << 8) | _skb->data[_pkt_offset + 1])

#define BKSYNC_PTP_EVENT_MSG(_ptp_msg_type) \
            ((_ptp_msg_type == IEEE1588_MSGTYPE_DELREQ) ||    \
             (_ptp_msg_type == IEEE1588_MSGTYPE_SYNC))

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#define HWTSTAMP_TX_ONESTEP_SYNC 2
#else
#include <linux/net_tstamp.h>
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,3,0))
#define FREQ_CORR       adjfine
#else
#define FREQ_CORR       adjfreq
#endif

/*
 *  Hardware specific information.
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
 *      IEEE1588_INGRESS_TIMESTAMP_SIGN -   ITS sign
 *      TX_TS                           -   TwoStep
 *      IEEE1588_TIMESTAMP_HDR_OFFSET   -   1588 header offset
 *      }
 *
 */
uint32_t sobmhrawpkts_dcb26[24] = {0x00000000, 0x00020E00, 0x00000000, 0x00000000, 0x00000000, 0x00021200, 0x00000000, 0x00000000,
                                   0x00000000, 0x00100E00, 0x00000000, 0x00000000, 0x00000000, 0x00101200, 0x00000000, 0x00000000,
                                   0x00000000, 0x00140E00, 0x00000000, 0x00000000, 0x00000000, 0x00141200, 0x00000000, 0x00000000};

uint32_t sobmhudpipv4_dcb26[24] = {0x00000000, 0x00022A00, 0x00000000, 0x00000000, 0x00000000, 0x00022E00, 0x00000000, 0x00000000,
                                   0x00000000, 0x00182A00, 0x00000000, 0x00000000, 0x00000000, 0x00182E00, 0x00000000, 0x00000000,
                                   0x00000000, 0x001C2A00, 0x00000000, 0x00000000, 0x00000000, 0x001C2E00, 0x00000000, 0x00000000};

uint32_t sobmhudpipv6_dcb26[24] = {0x00000000, 0x00023E00, 0x00000000, 0x00000000, 0x00000000, 0x00024200, 0x00000000, 0x00000000,
                                   0x00000000, 0x00183E00, 0x00000000, 0x00000000, 0x00000000, 0x00184200, 0x00000000, 0x00000000,
                                   0x00000000, 0x001C3E00, 0x00000000, 0x00000000, 0x00000000, 0x001C4200, 0x00000000, 0x00000000};

uint32_t sobmhrawpkts_dcb32[24] = {0x00000000, 0x00010E00, 0x00000000, 0x00000000, 0x00000000, 0x00011200, 0x00000000, 0x00000000,
                                   0x00000000, 0x00080E00, 0x00000000, 0x00000000, 0x00000000, 0x00081200, 0x00000000, 0x00000000,
                                   0x00000000, 0x00080E00, 0x00000000, 0x00000000, 0x00000000, 0x00081200, 0x00000000, 0x00000000};

uint32_t sobmhudpipv4_dcb32[24] = {0x00000000, 0x00012A00, 0x00000000, 0x00000000, 0x00000000, 0x00012E00, 0x00000000, 0x00000000,
                                   0x00000000, 0x000C2A00, 0x00000000, 0x00000000, 0x00000000, 0x000C2E00, 0x00000000, 0x00000000,
                                   0x00000000, 0x000C2A00, 0x00000000, 0x00000000, 0x00000000, 0x000C2E00, 0x00000000, 0x00000000};

uint32_t sobmhudpipv6_dcb32[24] = {0x00000000, 0x00013E00, 0x00000000, 0x00000000, 0x00000000, 0x00014200, 0x00000000, 0x00000000,
                                   0x00000000, 0x000C3E00, 0x00000000, 0x00000000, 0x00000000, 0x000C4200, 0x00000000, 0x00000000,
                                   0x00000000, 0x000C3E00, 0x00000000, 0x00000000, 0x00000000, 0x000C4200, 0x00000000, 0x00000000};

uint32_t sobmhrawpkts_dcb35[24] = {0x00000000, 0x0020E000, 0x00000000, 0x00000000, 0x00000000, 0x00212000, 0x00000000, 0x00000000,
                                   0x00000000, 0x0100E000, 0x00000000, 0x00000000, 0x00000000, 0x01012000, 0x00000000, 0x00000000,
                                   0x00000000, 0x0140E000, 0x00000000, 0x00000000, 0x00000000, 0x01412000, 0x00000000, 0x00000000};

uint32_t sobmhudpipv4_dcb35[24] = {0x00000000, 0x0022A000, 0x00000000, 0x00000000, 0x00000000, 0x0022E000, 0x00000000, 0x00000000,
                                   0x00000000, 0x0182A000, 0x00000000, 0x00000000, 0x00000000, 0x0182E000, 0x00000000, 0x00000000,
                                   0x00000000, 0x01C2A000, 0x00000000, 0x00000000, 0x00000000, 0x01C2E000, 0x00000000, 0x00000000};

uint32_t sobmhudpipv6_dcb35[24] = {0x00000000, 0x0023E000, 0x00000000, 0x00000000, 0x00000000, 0x00242000, 0x00000000, 0x00000000,
                                   0x00000000, 0x0183E000, 0x00000000, 0x00000000, 0x00000000, 0x01842000, 0x00000000, 0x00000000,
                                   0x00000000, 0x01C3E000, 0x00000000, 0x00000000, 0x00000000, 0x01C42000, 0x00000000, 0x00000000};


uint32_t sobmhrawpkts_dcb36[24] = {0x00000000, 0x00010E00, 0x00000000, 0x00000000, 0x00000000, 0x00011200, 0x00000000, 0x00000000,
                                   0x00000000, 0x00080E00, 0x00000000, 0x00000000, 0x00000000, 0x00081200, 0x00000000, 0x00000000,
                                   0x00000000, 0x00080E00, 0x00000000, 0x00000000, 0x00000000, 0x00081200, 0x00000000, 0x00000000};

uint32_t sobmhudpipv4_dcb36[24] = {0x00000000, 0x00012A00, 0x00000000, 0x00000000, 0x00000000, 0x00012E00, 0x00000000, 0x00000000,
                                   0x00000000, 0x000C2A00, 0x00000000, 0x00000000, 0x00000000, 0x000C2E00, 0x00000000, 0x00000000,
                                   0x00000000, 0x000C2A00, 0x00000000, 0x00000000, 0x00000000, 0x000C2E00, 0x00000000, 0x00000000};

uint32_t sobmhudpipv6_dcb36[24] = {0x00000000, 0x00013E00, 0x00000000, 0x00000000, 0x00000000, 0x00014200, 0x00000000, 0x00000000,
                                   0x00000000, 0x000C3E00, 0x00000000, 0x00000000, 0x00000000, 0x000C4200, 0x00000000, 0x00000000,
                                   0x00000000, 0x000C3E00, 0x00000000, 0x00000000, 0x00000000, 0x000C4200, 0x00000000, 0x00000000};
/* th3: onestep only */
uint32_t sobmhrawpkts_dcb38[24] = {0x00000000, 0x00080E00, 0x00000000, 0x00000000, 0x00000000, 0x00081200, 0x00000000, 0x00000000,
                                   0x00000000, 0x00080E00, 0x00000000, 0x00000000, 0x00000000, 0x00081200, 0x00000000, 0x00000000,
                                   0x00000000, 0x00080E00, 0x00000000, 0x00000000, 0x00000000, 0x00081200, 0x00000000, 0x00000000};

uint32_t sobmhudpipv4_dcb38[24] = {0x00000000, 0x00082A00, 0x00000000, 0x00000000, 0x00000000, 0x00082E00, 0x00000000, 0x00000000,
                                   0x00000000, 0x000C2A00, 0x00000000, 0x00000000, 0x00000000, 0x000C2E00, 0x00000000, 0x00000000,
                                   0x00000000, 0x000C2A00, 0x00000000, 0x00000000, 0x00000000, 0x000C2E00, 0x00000000, 0x00000000};

uint32_t sobmhudpipv6_dcb38[24] = {0x00000000, 0x00083E00, 0x00000000, 0x00000000, 0x00000000, 0x00084200, 0x00000000, 0x00000000,
                                   0x00000000, 0x000C3E00, 0x00000000, 0x00000000, 0x00000000, 0x000C4200, 0x00000000, 0x00000000,
                                   0x00000000, 0x000C3E00, 0x00000000, 0x00000000, 0x00000000, 0x000C4200, 0x00000000, 0x00000000};

/* HR3-MG/GH2 metadata */
uint32_t sobmhrawpkts_dcb37[24] = {0x00000000, 0x00020E00, 0x00000000, 0x00000000, 0x00000000, 0x00021200, 0x00000000, 0x00000000,
                                   0x00000000, 0x00100E00, 0x00000000, 0x00000000, 0x00000000, 0x00101200, 0x00000000, 0x00000000,
                                   0x00000000, 0x00140E00, 0x00000000, 0x00000000, 0x00000000, 0x00141200, 0x00000000, 0x00000000};

uint32_t sobmhudpipv4_dcb37[24] = {0x00000000, 0x00022A00, 0x00000000, 0x00000000, 0x00000000, 0x00022E00, 0x00000000, 0x00000000,
                                   0x00000000, 0x00182A00, 0x00000000, 0x00000000, 0x00000000, 0x00182E00, 0x00000000, 0x00000000,
                                   0x00000000, 0x001C2A00, 0x00000000, 0x00000000, 0x00000000, 0x001C2E00, 0x00000000, 0x00000000};

uint32_t sobmhudpipv6_dcb37[24] = {0x00000000, 0x00023E00, 0x00000000, 0x00000000, 0x00000000, 0x00024200, 0x00000000, 0x00000000,
                                   0x00000000, 0x00183E00, 0x00000000, 0x00000000, 0x00000000, 0x00184200, 0x00000000, 0x00000000,
                                   0x00000000, 0x001C3E00, 0x00000000, 0x00000000, 0x00000000, 0x001C4200, 0x00000000, 0x00000000};

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

#define __ATTRIBUTE_PACKED__  __attribute__ ((packed))

/* FW timestamps.
 *     This declaration has to match with HFT_t_TmStmp
 *     defined in the firmware. Otherwise, dma will fail. 
 */
typedef struct fw_tstamp_s {
    u64 sec;
    u32 nsec;
} __ATTRIBUTE_PACKED__ fw_tstamp_t;

typedef struct bksync_fw_debug_event_tstamps_s {
    fw_tstamp_t prv_tstamp;
    fw_tstamp_t cur_tstamp;
} __ATTRIBUTE_PACKED__ bksync_fw_debug_event_tstamps_t;

#ifndef BDE_EDK_SUPPORT
typedef struct bksync_evlog_s {
    bksync_fw_debug_event_tstamps_t event_timestamps[NUM_TS_EVENTS];
} __ATTRIBUTE_PACKED__ bksync_evlog_t;
#endif

/* Timestamps for EXTTS from Firmware */
#define BKSYNC_NUM_GPIO_EVENTS     6       /* gpio0 = event0 ..... gpio5 = event5 on single device */
#define BKSYNC_NUM_EVENT_TS        128     /* Directly mapped to PTP_MAX_TIMESTAMPS from ptp_private.h */
typedef struct bksync_fw_extts_event_s {
    u32 ts_event_id;
    fw_tstamp_t tstamp;
} __ATTRIBUTE_PACKED__ bksync_fw_extts_event_t;

typedef struct bksync_extts_log_s {
    u32 head;   /* Read pointer - Updated by HOST */
    u32 tail;   /* Write pointer - Updated by FW */
    bksync_fw_extts_event_t event_ts[BKSYNC_NUM_EVENT_TS];
    u32 overflow;
} __ATTRIBUTE_PACKED__ bksync_fw_extts_log_t;

struct bksync_extts_event {
    int enable[BKSYNC_NUM_GPIO_EVENTS];
    int head;
};

typedef struct bksync_time_spec_s {
    int sign; /* 0: positive, 1:negative */
    uint64_t sec; /* 47bit of secs */
    uint32_t nsec; /* 30bit of nsecs */
} bksync_time_spec_t;

/* DS for FW communication */
typedef struct bksync_fw_comm_s {
    u32 cmd;
    u32 dw1[2];
    u32 dw2[2];
    u32 head;   /* Read pointer - Updated by HOST */
    u32 tail;   /* Write pointer - Updated by FW */
} __ATTRIBUTE_PACKED__ bksync_fw_comm_t;

typedef struct bksync_port_stats_s {
    u32 pkt_rxctr;             /* All ingress packets */
    u32 pkt_txctr;             /* All egress packets  */
    u32 pkt_txonestep;         /* 1-step Tx packet counter */
    u32 tsts_match;            /* 2-Step tstamp req match */
    u32 tsts_timeout;          /* 2-Step tstamp req timeouts */
    u32 tsts_discard;          /* 2-Step tstamp req discards */
    u32 osts_event_pkts;       /* 1-step event packet counter */
    u32 osts_tstamp_reqs;      /* 1-step events with tstamp request */
    u32 fifo_rxctr;            /* 2-Step tstamp req match */
    u64 tsts_best_fetch_time;  /* 1-step events with tstamp request */
    u64 tsts_worst_fetch_time; /* 1-step events with tstamp request */
    u32 tsts_avg_fetch_time;   /* 1-step events with tstamp request */
} bksync_port_stats_t;

typedef struct bksync_init_info_s {
    u32 pci_knetsync_cos;
    u32 uc_port_num;
    u32 uc_port_sysport;
    u32 host_cpu_port;
    u32 host_cpu_sysport;
    u32 udh_len;
    u8  application_v2;
} bksync_init_info_t;

typedef struct bksync_bs_info_s {
    u32 enable;
    u32 mode;
    u32 bc;
    u32 hb;
    bksync_time_spec_t offset;
} bksync_bs_info_t;

typedef struct bksync_gpio_info_s {
    u32 enable;
    u32 mode;
    u32 period;
    int64_t phaseoffset;
} bksync_gpio_info_t;

typedef struct bksync_ptp_tod_info_s {
    bksync_time_spec_t offset;
} bksync_ptp_tod_info_t;

typedef struct bksync_ntp_tod_info_s {
    uint8_t leap_sec_ctrl_en; /* 1:enable, 0:disable */
    uint8_t leap_sec_op; /* 0:insert 1sec leap sec, 1:delete 1sec leap sec */
    uint64_t epoch_offset; /* 48bit epoch offset */
} bksync_ntp_tod_info_t;

#ifndef BDE_EDK_SUPPORT
typedef struct bksync_evlog_info_s {
    u32 enable;
} bksync_evlog_info_t;
#endif

typedef struct bksync_ptp_time_s {
    int ptp_pair_lock;
    u64 ptptime;
    u64 reftime;
    u64 ptptime_alt;
    u64 reftime_alt;
} bksync_ptp_time_t;

typedef struct bksync_2step_info_s {
    u64 portmap[BKSYNC_MAX_NUM_PORTS/64];  /* Two-step enabled ports */
} bksync_2step_info_t;

/************************ DNX Header Information ************************/
/* Contains information about parsed fields of RX packet header information */
typedef struct bksync_dnx_rx_pkt_parse_info_s {
    uint16_t     src_sys_port;
    uint64_t     rx_hw_timestamp;
    uint64_t     pph_header_vlan;
    uint8_t      dnx_header_offset;
    int          rx_frame_len;
} bksync_dnx_rx_pkt_parse_info_t;

/* DNX UDH DATA TYPE MAX */
#define BKSYNC_DNXJER2_UDH_DATA_TYPE_MAX     (4)

/* PPH LIF Ext. 3 bit type */
#define BKSYNC_DNXJER2_PPH_LIF_EXT_TYPE_MAX  (8)

typedef struct  bksync_dnx_jr2_header_info_s {
    /* dnx JR2 system header info */
    uint32_t     ftmh_lb_key_ext_size;
    uint32_t     ftmh_stacking_ext_size;
    uint32_t     pph_base_size;
    uint32_t     pph_lif_ext_size[BKSYNC_DNXJER2_PPH_LIF_EXT_TYPE_MAX];
    uint32_t     system_headers_mode;
    uint32_t     udh_enable;
    uint32_t     udh_data_lenght_per_type[BKSYNC_DNXJER2_UDH_DATA_TYPE_MAX];

    /* CPU port information */
    uint32_t     cosq_port_cpu_channel;
    uint32_t     cosq_port_pp_port;
} bksync_dnx_jr2_header_info_t;

typedef enum bksync_dnx_jr2_system_headers_mode_e {
    bksync_dnx_jr2_sys_hdr_mode_jericho = 0,
    bksync_dnx_jr2_sys_hdr_mode_jericho2 = 1
} bksync_dnx_jr2_system_headers_mode_t;

/* DNX JR2 FTMH Header information */
#define BKSYNC_DNXJR2_FTMH_HDR_LEN              (10)
#define BKSYNC_DNXJR2_FTMH_TM_DEST_EXT_LEN      (3)
#define BKSYNC_DNXJR2_FTMH_FLOWID_EXT_LEN       (3)
#define BKSYNC_DNXJR2_FTMH_BEIR_BFR_EXT_LEN     (3)
#define BKSYNC_DNXJR2_FTMH_APP_SPECIFIC_EXT_LEN     (6)

/* DNX FTMH PPH type */
#define BKSYNC_DNXJR2_PPH_HEADER_LEN             (12)
#define BKSYNC_DNXJR2_PPH_TYPE_NO_PPH            (0)
#define BKSYNC_DNXJR2_PPH_TYPE_PPH_BASE          (1)
#define BKSYNC_DNXJR2_PPH_TYPE_TSH_ONLY          (2)
#define BKSYNC_DNXJR2_PPH_TYPE_PPH_BASE_TSH      (3)

typedef enum bksync_dnx_jr2_ftmh_tm_action_type_e {
    bksync_dnx_jr2_ftmh_tm_action_type_forward = 0,               /* TM action is forward */
    bksync_dnx_jr2_ftmh_tm_action_type_snoop = 1,                 /* TM action is snoop */
    bksync_dnx_jr2_ftmh_tm_action_type_inbound_mirror = 2,        /* TM action is inbound mirror.  */
    bksync_dnx_jr2_ftmh_tm_action_type_outbound_mirror = 3,       /* TM action is outbound mirror. */
    bksync_dnx_jr2_ftmh_tm_action_type_mirror = 4,                /* TM action is mirror. */
    bksync_dnx_jr2_ftmh_tm_action_type_statistical_sampling = 5   /* TM action is statistical sampling. */
} bksync_dnx_jr2_ftmh_tm_action_type_t;

typedef enum bksync_dnx_jr2_ftmh_app_spec_ext_type_e {
    bksync_dnx_jr2_ftmh_app_spec_ext_type_none             = 0,   /* FTMH ASE type is None or OAM */
    bksync_dnx_jr2_ftmh_app_spec_ext_type_1588v2           = 1,   /* FTMH ASE type is 1588v2 */
    bksync_dnx_jr2_ftmh_app_spec_ext_type_mirror           = 3,   /* FTMH ASE type is Mirror */
    bksync_dnx_jr2_ftmh_app_spec_ext_type_trajectory_trace = 4,   /* FTMH ASE type is trajectory trace */
    bksync_dnx_jr2_ftmh_app_spec_ext_type_inband_telemetry = 5,   /* FTMH ASE type is Inband telemetry */
} bksync_dnx_jr2_ftmh_app_spec_ext_type_t;

typedef union bksync_dnx_jr2_ftmh_base_header_s {
    struct {
            uint32_t words[2];
            uint8_t bytes[2];
    };
    struct {
        uint32_t
            src_sys_port_aggr_1:8,
            src_sys_port_aggr_0:7,
            traffic_class_1:1,
            traffic_class_0:2,
            packet_size_1:6,
            packet_size_0:8;
        uint32_t
            unused_0:31,
            src_sys_port_aggr_2:1;
        uint8_t
            unused_1:8;
        uint8_t
            reserved:1,
            bier_bfr_ext_size:1,
            flow_id_ext_size:1,
            app_specific_ext_size:1,
            tm_dest_ext_repsent:1,
            pph_type:2,
            visibility:1;
    };
} bksync_dnx_jr2_ftmh_base_header_t;

typedef union bksync_dnx_jr2_ftmh_app_spec_ext_1588v2_s {
    struct {
        uint32_t word;
        uint8_t bytes[2];
    };
    struct {
        uint32_t
            use_ingress_time_stamp:1,
            use_ingress_time_compensation:1,
            ingress_time_compensation:28,
            time_stamp_lsbs:2;
        uint8_t
            offset_0:4,
            ts_command:3,
            ts_encapsulation:1;
        uint8_t
            offset_1:4,
            type:4;
    };
} bksync_dnx_jr2_ftmh_app_spec_ext_1588v2_t;

/* DNX TSH Header size */
#define BKSYNC_DNXJR2_TSH_HDR_SIZE              (4)

typedef union bksync_dnx_jr2_timestamp_header_s {
    struct {
        uint32_t word;
    };
    struct {
        uint32_t timestamp;
    };
}  bksync_dnx_jr2_timestamp_header_t;

/* DNX PPH FHEI_TYPE */
#define BKSYNC_DNXJR2_PPH_FHEI_TYPE_NONE           (0) /*  NO FHE1 */
#define BKSYNC_DNXJR2_PPH_FHEI_TYPE_SZ0            (1) /* 3 byte */
#define BKSYNC_DNXJR2_PPH_FHEI_TYPE_SZ1            (2) /* 5 byte */
#define BKSYNC_DNXJR2_PPH_FHEI_TYPE_SZ2            (3) /* 8 byte */

#define BKSYNC_DNXJR2_PPH_FHEI_SZ0_SIZE            (3) /*  3 byte */
#define BKSYNC_DNXJR2_PPH_FHEI_SZ1_SIZE            (5) /*  5 byte */
#define BKSYNC_DNXJR2_PPH_FHEI_SZ2_SIZE            (8) /* 8 byte */

/* PPH Learn Extension - PPH EXT3 */
#define BKSYNC_DNXJR2_PPH_LEARN_EXT_SIZE         (19)

/* PPH LIF Ext. 3 bit type */
#define BKSYNC_DNXJR2_PPH_LIF_EXT_TYPE_MAX       (8)

typedef enum bksync_dnx_jr2_pph_fheiext_type_e {
    bksync_dnx_jr2_pph_fheiext_type_vlanedit = 0,
    bksync_dnx_jr2_pph_fheiext_type_pop = 1,
    bksync_dnx_jr2_pph_fheiext_type_swap =  3,
    bksync_dnx_jr2_pph_fheiext_type_trap_snoop_mirror =  5,
} bksync_dnx_jr2_pph_fheiext_type_t;

typedef union bksync_dnx_jr2_pph_base_12b_header_s {
    struct {
        uint32_t word[3];
    };
    struct {
        uint32_t     unused_1;
        uint32_t     unused_2;
        uint32_t
            forwarding_strenght:1,
            parsing_start_type:5,
            parsing_start_offset_1:2,
            parsing_start_offset_0:5,
            lif_ext_type:3,
            fhei_size:2,
            learn_ext_present:1,
            ttl_1:5,
            ttl_0:3,
            netwrok_qos_0:5;
    };
} bksync_dnx_jr2_pph_base_12b_header_t;

typedef union bksync_dnx_jr2_pph_fheiext_vlanedit_3b_header_s {
    struct {
        uint8_t byte[3];
    };
    struct {
        uint8_t
            edit_pcp1_0:1,
            ingress_vlan_edit_cmd:7;
        uint8_t
            edit_vid1_0:5,
            edit_dei1:1,
            edit_pcp1_1:2;
        uint8_t
            type:1,
            edit_vid1_1:7;
    };
} bksync_dnx_jr2_pph_fheiext_vlanedit_3b_header_t;

typedef union bksync_dnx_jr2_pph_fheiext_vlanedit_5b_header_s {
    struct {
        uint8_t byte[5];
    };
    struct {
        uint8_t
            edit_vid2_0:4,
            edit_dei2:1,
            edit_pcp2:3;
        uint8_t
            edit_vid2_1;
        uint8_t
            edit_pcp1_0:1,
            ingress_vlan_edit_cmd:7;
        uint8_t
            edit_vid1_0:5,
            edit_dei1:1,
            edit_pcp1_1:2;
        uint8_t
            type:1,
            edit_vid1_1:7;
    };
} bksync_dnx_jr2_pph_fheiext_vlanedit_5b_header_t;

typedef union bksync_dnx_jr2_pph_fheiext_trap_header_s {
    struct {
        uint8_t byte[5];
    };
    struct {
        uint32_t
            code_0:5,
            qualifier:27;
        uint8_t
            type:4,
            code_1:4;
    };
} bksync_dnx_jr2_pph_fheiext_trap_header_t;

#define BKSYNC_DNXJR2_UDH_BASE_HEADER_LEN       (1)
#define BKSYNC_DNXJR2_UDH_DATA_TYPE_MAX          (4)

typedef union bksync_dnx_jr2_udh_base_header_s {
    struct {
        uint8_t byte;
    };
    struct {
        uint8_t
            udh_data_type_3:2,
            udh_data_type_2:2,
            udh_data_type_1:2,
            udh_data_type_0:2;
    };
} bksync_dnx_jr2_udh_base_header_t;

#define BKSYNC_DNXJR2_PTCH_TYPE2_HEADER_LEN   2

typedef union bksync_dnx_jr2_ptch_type2_header_s {
    struct {
        uint8_t bytes[BKSYNC_DNXJR2_PTCH_TYPE2_HEADER_LEN];
    };
    struct {
        uint8_t
            in_pp_port_0:2,
            reserved:2,
            opaque_pt_attributes:3,
            parser_program_control:1;
        uint8_t
            in_pp_port_1:8;
    };
} bksync_dnx_jr2_ptch_type2_header_t;

#define BKSYNC_DNXJR2_MODULE_HEADER_LEN   16
#define BKSYNC_DNXJR2_ITMH_HEADER_LEN     5

/* Device specific data */
typedef struct bksync_dev {
    int dcb_type;
    int dev_no;
    uint16_t dev_id;
    uint8_t max_core;      /* FW cores */
    volatile int dev_init; /* Indicates if the associated core is initialized */
    volatile void *base_addr;   /* address for PCI register access */
    struct DMA_DEV *dma_dev;     /* Required for DMA memory control */
    dma_addr_t dma_mem;
#ifdef BDE_EDK_SUPPORT
    volatile bksync_fw_comm_t *fw_comm;
#else
    int evlog_dma_mem_size;
    volatile bksync_evlog_t *evlog;       /* dma-able address for fw updates */
    bksync_evlog_info_t evlog_info[NUM_TS_EVENTS];
    int extts_dma_mem_size;
    dma_addr_t extts_dma_mem_addr;
#endif
    bksync_gpio_info_t bksync_gpio_info[6];
    bksync_bs_info_t bksync_bs_info[2];
    volatile bksync_fw_extts_log_t *extts_log; /* dma-able/virtual address for fw updates */
    bksync_ptp_tod_info_t ptp_tod; /* PTP ToD configuration */
    bksync_ntp_tod_info_t ntp_tod; /* NTP ToD configuration */
    struct bksync_extts_event extts_event;
    struct ptp_clock *ptp_clock;
    struct ptp_clock_info ptp_info;
    struct mutex ptp_lock;
    int num_phys_ports;
    bksync_ptp_time_t ptp_time;
    bksync_2step_info_t two_step;
    bksync_port_stats_t *port_stats;
    bksync_init_info_t init_data;
    bksync_dnx_jr2_header_info_t jr2_header_data;
}bksync_dev_t;

/* Clock Private Data */
struct bksync_ptp_priv {
    int                     timekeep_status;
    u32                     mirror_encap_bmp;
    struct delayed_work     time_keep;
    struct kobject          *kobj;
    int                      max_dev;
    struct delayed_work     extts_logging;
    bksync_dev_t            *dev_info;
    bksync_dev_t            *master_dev_info;
};

/************************ Local Variables ************************/
static ibde_t *kernel_bde = NULL;
static struct bksync_ptp_priv *ptp_priv;
static int num_retries = 5;   /* Retry count */
/* Driver Proc Entry root */
static struct proc_dir_entry *bksync_proc_root = NULL;

/************************ Local Functions ************************/
static void bksync_ptp_time_keep_init(void);
static void bksync_ptp_time_keep_deinit(void);

static void bksync_dnx_jr2_parse_rxpkt_system_header(bksync_dev_t *dev_info, uint8_t *raw_frame, 
                        bksync_dnx_rx_pkt_parse_info_t *rx_pkt_parse_info, int isfirsthdr);

static int bksync_ptp_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts);

static void bksync_ptp_extts_logging_init(void);
static void bksync_ptp_extts_logging_deinit(void);

static int bksync_ptp_remove(void);

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
    } while(0)

#define DEV_WRITE32(_d, _a, _v) \
    do { \
        uint32_t _data = CMIC_SWAP32(_v); \
        ((volatile uint32_t *)(_d)->base_addr)[(_a)/4] = (_data); \
    } while(0)

#else

#define DEV_READ32(_d, _a, _p) \
    do { \
        *(_p) = (((volatile uint32_t *)(_d)->base_addr)[(_a)/4]); \
    } while(0)

#define DEV_WRITE32(_d, _a, _v) \
    do { \
        ((volatile uint32_t *)(_d)->base_addr)[(_a)/4] = (_v); \
    } while(0)
#endif  /* defined(CMIC_SOFT_BYTE_SWAP) */

#define BKSYNC_U_SLEEP(usec) \
    do { \
        if (DEVICE_IS_DNX) { \
            udelay(usec); \
        } else { \
            usleep_range(usec,usec+1); \
        } \
    } while (0)

static void
ptp_sleep(int jiffies)
{
    wait_queue_head_t wq;
    init_waitqueue_head(&wq);

    wait_event_timeout(wq, 0, jiffies);

}

#ifdef BDE_EDK_SUPPORT
static void bksync_hostcmd_data_op(int dev_no, int setget, u64 *d1, u64 *d2)
{
    bksync_dev_t *dev_info = &ptp_priv->dev_info[dev_no];
    u32 w0, w1;
    u64 data;

    if (!d1) {
        return;
    }

    if (setget) {
        if (d1) {
            data = *d1;
            dev_info->fw_comm->dw1[0] = (data & 0xFFFFFFFF);
            dev_info->fw_comm->dw1[1] = (data >> 32);
        }

        if (d2) {
            data = *d2;
            dev_info->fw_comm->dw2[0] = (data & 0xFFFFFFFF);
            dev_info->fw_comm->dw2[1] = (data >> 32);
        }

    } else {
        if (d1) {
            w0 = dev_info->fw_comm->dw1[0];
            w1 = dev_info->fw_comm->dw1[1];
            data = (((u64)w1 << 32) | (w0));
            *d1 = data;
        }

        if (d2) {
            w0 = dev_info->fw_comm->dw2[0];
            w1 = dev_info->fw_comm->dw2[1];
            data = (((u64)w1 << 32) | (w0));
            *d2 = data;
        }
    }
}
#else
static void bksync_hostcmd_data_op(int dev_no, int setget, u64 *d1, u64 *d2)
{
    bksync_dev_t *dev_info = &ptp_priv->dev_info[dev_no];
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
            DEV_WRITE32(dev_info, hostcmd_regs[1], w0);
            DEV_WRITE32(dev_info, hostcmd_regs[2], w1);
        }

        if (d2) {
            data = *d2;

            w0 = (data & 0xFFFFFFFF);
            w1 = (data >> 32);
            DEV_WRITE32(dev_info, hostcmd_regs[3], w0);
            DEV_WRITE32(dev_info, hostcmd_regs[4], w1);
        }
    } else {
        if (d1) {
            DEV_READ32(dev_info, hostcmd_regs[1], &w0);
            DEV_READ32(dev_info, hostcmd_regs[2], &w1);
            data = (((u64)w1 << 32) | (w0));
            *d1 = data;
        }

        if (d2) {
            DEV_READ32(dev_info, hostcmd_regs[3], &w0);
            DEV_READ32(dev_info, hostcmd_regs[4], &w1);
            data = (((u64)w1 << 32) | (w0));
            *d2 = data;
        }
    }
}
#endif

static int bksync_cmd_go(bksync_dev_t *dev_info, u32 cmd, void *data0, void *data1)
{
    int ret = -1;
    int retry_cnt = (1000); /* 1ms default timeout for hostcmd response */
    int port = 0;
    char cmd_str[30];
    uint32_t cmd_status;
    uint32_t seq_id = 0;
    uint32_t subcmd = 0;
    uint64_t freqcorr = 0;
    uint64_t phase_offset = 0;
    int dev_no = dev_info->dev_no;
    ktime_t start, now;

    mutex_lock(&dev_info->ptp_lock);

    start = ktime_get();

    /* init data */
#ifdef BDE_EDK_SUPPORT
    if (dev_info->fw_comm == NULL) {
        DBG_ERR(("Device is not initialized\n"));
        return -1;
    }

    dev_info->fw_comm->dw1[0] = 0;
    dev_info->fw_comm->dw1[1] = 0;
    dev_info->fw_comm->dw2[0] = 0;
    dev_info->fw_comm->dw2[1] = 0;
#else
    DEV_WRITE32(dev_info, hostcmd_regs[1], 0x0);
    DEV_WRITE32(dev_info, hostcmd_regs[2], 0x0);
    DEV_WRITE32(dev_info, hostcmd_regs[3], 0x0);
    DEV_WRITE32(dev_info, hostcmd_regs[4], 0x0);
#endif

    switch (cmd) {
        case BKSYNC_INIT:
            retry_cnt = (retry_cnt * 4);
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_INIT");
            phase_offset  = 0;
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)&(phase_offset), 0);
            break;
        case BKSYNC_FREQCOR:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_FREQCORR");
            freqcorr  = *((s32 *)data0);
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)&(freqcorr), 0);
            break;
        case BKSYNC_ADJTIME:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_ADJTIME");
            phase_offset  = *((s64 *)data0);
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)&(phase_offset), 0);
            break;
        case BKSYNC_GETTIME:
            retry_cnt = (retry_cnt * 2);
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_GETTIME");
            break;
        case BKSYNC_GET_TSTIME:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_GET_TSTIME");
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)data0, (u64 *)data1);
            break;
         case BKSYNC_ACK_TSTIME:
            retry_cnt = (retry_cnt * 2);
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_ACK_TSTIME");
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)data0, (u64 *)data1);
            break;
        case BKSYNC_SETTIME:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_SETTIME");
            dev_info->ptp_time.ptptime = *((s64 *)data0);
            phase_offset = 0;
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)&(dev_info->ptp_time.ptptime), (u64 *)&(phase_offset));
            break;
        case BKSYNC_MTP_TS_UPDATE_ENABLE:
            retry_cnt = (retry_cnt * 6);
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_MTP_TS_UPDATE_ENABLE");
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)data0, 0);
            break;
        case BKSYNC_MTP_TS_UPDATE_DISABLE:
            retry_cnt = (retry_cnt * 6);
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_MTP_TS_UPDATE_DISABLE");
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)data0, 0);
            break;
        case BKSYNC_DEINIT:
            retry_cnt = (retry_cnt * 4);
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_DEINIT");
            break;
        case BKSYNC_SYSINFO:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_SYSINFO");
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)data0, (u64 *)data1);
            break;
        case BKSYNC_BROADSYNC:
            subcmd = *((u32 *)data0);
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_BROADSYNC");
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)data0, (u64 *)data1);
            break;
        case BKSYNC_GPIO:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_GPIO");
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)data0, (u64 *)data1);
            break;
        case BKSYNC_EVLOG:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_EVLOG");
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)data0, (u64 *)data1);
            break;
        case BKSYNC_EXTTSLOG:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_EXTTSLOG");
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)data0, (u64 *)data1);
            break;
#ifdef BDE_EDK_SUPPORT
        case BKSYNC_GET_EXTTS_BUFF:
            snprintf(cmd_str, sizeof(cmd_str), "BKSYNC_GET_EXTTS_BUFF");
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)data0, (u64 *)data1);
            break;
#endif
        case BKSYNC_GPIO_PHASEOFFSET:
            snprintf(cmd_str, sizeof(cmd_str), "BKSYNC_GPIO_PHASEOFFSET");
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)data0, (u64 *)data1);
            break;
#ifdef BDE_EDK_SUPPORT
        case BKSYNC_PTP_TOD:
            snprintf(cmd_str, sizeof(cmd_str), "BKSYNC_PTP_TOD");
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)data0, (u64 *)data1);
            break;
        case BKSYNC_NTP_TOD:
            snprintf(cmd_str, sizeof(cmd_str), "BKSYNC_NTP_TOD");
            bksync_hostcmd_data_op(dev_no, 1, (u64 *)data0, (u64 *)data1);
            break;
        case BKSYNC_PTP_TOD_GET:
            retry_cnt = (retry_cnt * 4);
            snprintf(cmd_str, sizeof(cmd_str), "BKSYNC_PTP_TOD_GET");
            break;
        case BKSYNC_NTP_TOD_GET:
            retry_cnt = (retry_cnt * 4);
            snprintf(cmd_str, sizeof(cmd_str), "BKSYNC_NTP_TOD_GET");
            break;
#endif
        default:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_XXX");
            break;
    }

#ifdef BDE_EDK_SUPPORT
    dev_info->fw_comm->cmd = cmd;
#else
    DEV_WRITE32(dev_info, hostcmd_regs[0], cmd);
#endif

    do {
#ifdef BDE_EDK_SUPPORT
        cmd_status = dev_info->fw_comm->cmd;
#else
        DEV_READ32(dev_info, hostcmd_regs[0], &cmd_status);
#endif

        if (cmd_status == BKSYNC_DONE) {
            ret = 0;
            switch (cmd) {
                case BKSYNC_GET_TSTIME:
                case BKSYNC_GETTIME:
#ifdef BDE_EDK_SUPPORT
                case BKSYNC_PTP_TOD_GET:
                case BKSYNC_NTP_TOD_GET:
#endif
#ifndef BDE_EDK_SUPPORT
                    {
                        u64 d0 = 0ULL;
                        u64 d1 = 0ULL;
                        int retry2_cnt = 3;
                        *(u64 *)data0 = 0ULL;
                        *(u64 *)data1 = 0ULL;
                        do {
                            bksync_hostcmd_data_op(dev_no, 0, &d0, &d1);
                            *(u64 *)data0 |= d0;
                            *(u64 *)data1 |= d1;
                            if (((*(u64 *)data0) & 0xFFFFFFFF) && ((*(u64 *)data0)>>32) &&
                                ((*(u64 *)data1) & 0xFFFFFFFF) && ((*(u64 *)data1)>>32)) {
                                break;
                            }
                            retry2_cnt--;
                            BKSYNC_U_SLEEP(1);
                        } while (retry2_cnt);
                        if (retry2_cnt == 0)
                            ret = -1;
                    }
                    break;
#else
                    bksync_hostcmd_data_op(dev_no, 0, (u64 *)data0, (u64 *)data1);
                    break;
#endif
                case BKSYNC_BROADSYNC:
                    if ((subcmd == BKSYNC_BROADSYNC_BS0_STATUS_GET) ||
                        (subcmd == BKSYNC_BROADSYNC_BS1_STATUS_GET)) {
                        bksync_hostcmd_data_op(dev_no, 0, (u64 *)data0, (u64 *)data1);
                    }
                    break;
#ifdef BDE_EDK_SUPPORT
                    /* Get the host ram address from fw.*/
                case BKSYNC_GET_EXTTS_BUFF:
                    bksync_hostcmd_data_op(dev_no, 0, (u64 *)data0, (u64 *)data1);
                    break;
#endif
                default:
                    break;
            }
            break;
        }
        BKSYNC_U_SLEEP(100);
        retry_cnt--;
    } while (retry_cnt);

    now = ktime_get();
    mutex_unlock(&dev_info->ptp_lock);

    if (retry_cnt == 0) {
        DBG_ERR(("bksync_cmd_go(dev_no:%d) Timeout on response from R5 to cmd %s time taken %lld us\n", dev_no, cmd_str, ktime_us_delta(now, start)));

        if (cmd == BKSYNC_GET_TSTIME || cmd == BKSYNC_ACK_TSTIME) {
            port = *((uint64_t *)data0) & 0xFFF;
            seq_id = *((uint64_t*)data0) >> 16;
            DBG_ERR(("bksync_cmd_go(dev_no:%d) 2step timestamp get timeout for port:%d seq_id:%d\n", dev_no, port, seq_id));
        }
    }

    if (debug & DBG_LVL_CMDS) {
        if (ktime_us_delta(now, start) > 5000)
            DBG_CMDS(("bksync_cmd_go(dev_no:%d) R5 Command %s exceeded time expected (%lld us)\n", dev_no, cmd_str, ktime_us_delta(now, start)));
    }

    DBG_CMDS(("bksync_cmd_go(dev_no:%d): cmd:%s rv:%d\n", dev_no, cmd_str, ret));

    return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,3,0))

#if (LINUX_VERSION_CODE <= KERNEL_VERSION(5,13,0))
#if (!(IS_REACHABLE(CONFIG_PTP_1588_CLOCK)))
static long scaled_ppm_to_ppb(long ppm)
{
    s64 ppb = 1 + ppm;

    ppb *= 125;
    ppb >>= 13;

    return (long)ppb;
}
#endif
#endif

/**
 * bksync_ptp_freqcorr
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ppm: frequency correction value in ppm with 16bit binary
 *       fractional field.
 *
 * Description: this function will set the frequency correction
 */
static int bksync_ptp_freqcorr(struct ptp_clock_info *ptp, long ppm)
{
    bksync_dev_t *dev_info = container_of(ptp, bksync_dev_t, ptp_info);
    int ret = -1;
    int64_t ppb = 0;

    if (!dev_info->dev_init) {
        return ret;
    }

    ppb = scaled_ppm_to_ppb(ppm);

    ret = bksync_cmd_go(dev_info, BKSYNC_FREQCOR, &ppb, NULL);
    DBG_VERB(("bksync_ptp_freqcorr: applying freq correction: ppm:0x%llx ppb:0x%llx; rv:%d\n", (int64_t)ppm, ppb, ret));

    return ret;
}
#else
/**
 * bksync_ptp_freqcorr
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ppb: frequency correction value in ppb
 *
 * Description: this function will set the frequency correction
 */
static int bksync_ptp_freqcorr(struct ptp_clock_info *ptp, s32 ppb)
{
    bksync_dev_t *dev_info = container_of(ptp, bksync_dev_t, ptp_info);
    int ret = -1;

    if (!dev_info->dev_init) {
        return ret;
    }

    ret = bksync_cmd_go(dev_info, BKSYNC_FREQCOR, &ppb, NULL);
    DBG_VERB(("bksync_ptp_freqcorr: applying freq correction: ppb:0%x; rv:%d\n", ppb, ret));

    return ret;
}
#endif

/**
 * bksync_ptp_adjtime
 *
 * @ptp: pointer to ptp_clock_info structure
 * @delta: desired change in nanoseconds
 *
 * Description: this function will shift/adjust the hardware clock time.
 */
static int bksync_ptp_adjtime(struct ptp_clock_info *ptp, s64 delta)
{
    bksync_dev_t *dev_info = container_of(ptp, bksync_dev_t, ptp_info);
    int ret = -1;

    if (!dev_info->dev_init) {
        return ret;
    }

    ret = bksync_cmd_go(dev_info, BKSYNC_ADJTIME, (void *)&delta, NULL);
    DBG_VERB(("ptp_adjtime: adjtime: 0x%llx; rv:%d\n", delta, ret));

    return ret;
}

/**
 * bksync_ptp_gettime
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ts: pointer to hold time/result
 *
 * Description: this function will read the current time from the
 * hardware clock and store it in @ts.
 */
static int bksync_ptp_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts)
{
    bksync_dev_t *dev_info = container_of(ptp, bksync_dev_t, ptp_info);
    bksync_dev_t *master_dev_info = NULL;
    int ret = -1;
    s64 reftime = 0;
    s64 refctr = 0;
    static u64 prv_reftime = 0, prv_refctr = 0;
    u64 diff_reftime = 0, diff_refctr = 0;

    if (!dev_info->dev_init) {
        return ret;
    }

    if ((shared_phc == 1) && (dev_info->dev_no != master_core)) {
        master_dev_info = &ptp_priv->dev_info[master_core];

        if (master_dev_info) {
            dev_info->ptp_time.ptptime_alt = dev_info->ptp_time.ptptime;
            dev_info->ptp_time.reftime_alt = dev_info->ptp_time.reftime;

            dev_info->ptp_time.ptp_pair_lock = 1;
            dev_info->ptp_time.ptptime = master_dev_info->ptp_time.ptptime;
            dev_info->ptp_time.reftime = master_dev_info->ptp_time.reftime;
            dev_info->ptp_time.ptp_pair_lock = 0;

            *ts = ns_to_timespec64(dev_info->ptp_time.ptptime); 
        }
    } else {
        ret = bksync_cmd_go(dev_info, BKSYNC_GETTIME, (void *)&reftime, (void *)&refctr);
        if (ret == 0) {
            DBG_VERB(("ptp_gettime: gettime: 0x%llx refctr:0x%llx\n", reftime, refctr));

            dev_info->ptp_time.ptptime_alt = dev_info->ptp_time.ptptime;
            dev_info->ptp_time.reftime_alt = dev_info->ptp_time.reftime;

            dev_info->ptp_time.ptp_pair_lock = 1;
            dev_info->ptp_time.ptptime = reftime;
            dev_info->ptp_time.reftime = refctr;
            dev_info->ptp_time.ptp_pair_lock = 0;

            diff_reftime = reftime - prv_reftime;
            diff_refctr = refctr - prv_refctr;

            if (diff_reftime != diff_refctr) {
                DBG_WARN(("ptp_gettime ptptime: 0x%llx reftime: 0x%llx prv_ptptime: 0x%llx prv_reftime: 0x%llx \n",
                            dev_info->ptp_time.ptptime, dev_info->ptp_time.reftime, diff_reftime, diff_refctr));
            }
            prv_reftime = reftime;
            prv_refctr = refctr;

            *ts = ns_to_timespec64(reftime);
        }
    }
    return ret;
}


/**
 * bksync_ptp_settime
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ts: time value to set
 *
 * Description: this function will set the current time on the
 * hardware clock.
 */
static int bksync_ptp_settime(struct ptp_clock_info *ptp,
                              const struct timespec64 *ts)
{
    bksync_dev_t *dev_info = container_of(ptp, bksync_dev_t, ptp_info);
    s64 reftime, phaseadj;
    int ret = -1;

    if (!dev_info->dev_init) {
        return ret;
    }

    phaseadj = 0;
    reftime = timespec64_to_ns(ts);

    ret = bksync_cmd_go(dev_info, BKSYNC_SETTIME, (void *)&reftime, (void *)&phaseadj);
    DBG_VERB(("ptp_settime: settime: 0x%llx; rv:%d\n", reftime, ret));

    return ret;
}

static int bksync_exttslog_cmd(bksync_dev_t *dev_info, int event, int enable)
{
    int ret = 0;
    u64 subcmd = 0, subcmd_data = 0;

#ifdef BDE_EDK_SUPPORT
    sal_vaddr_t vaddr = 0;

    if (NULL == dev_info->extts_log) {
        ret = bksync_cmd_go(dev_info, BKSYNC_GET_EXTTS_BUFF,
                &subcmd, &subcmd_data);
        DBG_VERB((" EXTTS: phy_addr:0x%llx\n", subcmd_data));

        ret = lkbde_get_phys_to_virt(dev_info->dev_no, (phys_addr_t)subcmd_data, &vaddr);
        if ((ret != 0) || (vaddr == 0)) {
            DBG_ERR(("EXTTS: failed to get virt_addr for the phy_addr\n"));
            return ret;
        }
        dev_info->extts_log = (bksync_fw_extts_log_t *)vaddr;
        DBG_VERB((" EXTTS: virt_addr:%p:0x%llx\n", dev_info->extts_log, (uint64_t)vaddr));
        subcmd_data = 0;
    }
#else
    subcmd_data = dev_info->extts_dma_mem_addr;
#endif

    /* upper 32b -> event
     * lower 32b -> enable/disable */
    subcmd = (u64)event << 32 | enable;

    ret = bksync_cmd_go(dev_info, BKSYNC_EXTTSLOG, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_extts_cmd: subcmd: 0x%llx subcmd_data: 0x%llx rv:%d\n", subcmd, subcmd_data, ret));

    return ret;
}

static int bksync_ptp_enable(struct ptp_clock_info *ptp,
                             struct ptp_clock_request *rq, int on)
{
    bksync_dev_t *dev_info = container_of(ptp, bksync_dev_t, ptp_info);
    int mapped_event = -1;
    int enable = on ? 1 : 0;
    int max_event_id = -1;
    int event_id = -1;
    int dev_no = -1;

    switch (rq->type) {
        case PTP_CLK_REQ_EXTTS:
            event_id = rq->extts.index;
            max_event_id = ptp->n_ext_ts;

            if (event_id > (max_event_id - 1)) {
                DBG_ERR(("bksync_ptp_enable: Event id %d not supported\n", event_id));
                return -EINVAL;
            }

            /* Determine dev_no based on the user input */
            dev_no = event_id / BKSYNC_NUM_GPIO_EVENTS;

            if (dev_no != dev_info->dev_no) {
                dev_info = &ptp_priv->dev_info[dev_no];
            }

            /* Determine actual event id as per device */
            event_id = (max_event_id + event_id) % BKSYNC_NUM_GPIO_EVENTS;

            switch (event_id) {
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

            dev_info->extts_event.enable[event_id] = enable;

            bksync_exttslog_cmd(dev_info, mapped_event, enable);

            DBG_VERB(("bksync_ptp_enable: Event state change req_index:%u (dev_n:%d event_id:%d) state:%d\n",
                        rq->extts.index, dev_no, event_id, enable));
            break;
        default:
            return -EOPNOTSUPP;
    }

    return 0;
}


static int bksync_ptp_mirror_encap_update(bksync_dev_t *dev_info, struct ptp_clock_info *ptp,
                                          int mtp_idx, int start)
{
    int ret = -1;
    u64 mirror_encap_idx;
    u32 cmd_status;

    if (mtp_idx > BKSYNC_MAX_MTP_IDX) {
        return ret;
    }

    mirror_encap_idx = mtp_idx;
    if (start) {
        cmd_status = BKSYNC_MTP_TS_UPDATE_ENABLE;
        ptp_priv->mirror_encap_bmp |= (1 << mtp_idx);
    } else {
        if (!(ptp_priv->mirror_encap_bmp & mtp_idx)) {
            /* Not running */
            return ret;
        }
        cmd_status = BKSYNC_MTP_TS_UPDATE_DISABLE;
        ptp_priv->mirror_encap_bmp &= ~mtp_idx;
    }

    ret = bksync_cmd_go(dev_info, cmd_status, &mirror_encap_idx, NULL);
    DBG_VERB(("mirror_encap_update: %d, mpt_index: %d, ret:%d\n", start, mtp_idx, ret));

    return ret;

}

/* structure describing a PTP hardware clock */
static struct ptp_clock_info bksync_ptp_info = {
    .owner = THIS_MODULE,
    .name = "bksync_ptp_clock",
    .max_adj = 200000,
    .n_alarm = 0,
    .n_ext_ts = 0, /* Determined during module init. */
    .n_per_out = 0, /* will be overwritten in bksync_ptp_register */
    .n_pins = 0,
    .pps = 0,
    .FREQ_CORR = bksync_ptp_freqcorr,
    .adjtime = bksync_ptp_adjtime,
    .gettime64 = bksync_ptp_gettime,
    .settime64 = bksync_ptp_settime,
    .enable = bksync_ptp_enable,
};

/**
 * bksync_ptp_hw_tstamp_enable
 *
 * @dev_no: device number
 * @port: port number
 *
 * Description: this is a callback function to enable the timestamping on
 * a given port
 */
static int bksync_ptp_hw_tstamp_enable(int dev_no, int port, int tx_type)
{
    uint64_t portmap = 0;
    int map = 0;
    bksync_dev_t *dev_info = &ptp_priv->dev_info[dev_no];

    if (!dev_info->dev_init) {
        return -1;
    }

    if (tx_type == HWTSTAMP_TX_ONESTEP_SYNC) {
        DBG_VERB(("hw_tstamp_enable: Enabling 1-step(type:%d) TS on port:%d\n", tx_type, port));
        bksync_ptp_time_keep_init();
        return 0;
    }

    DBG_VERB(("hw_tstamp_enable: Enabling 2-step(type:%d) TS on port:%d\n", tx_type, port));

    if ((port > 0) && (port < dev_info->num_phys_ports)) {
        port -= 1;
        map = (port / 64);
        port = (port % 64);

        portmap = dev_info->two_step.portmap[map];
        portmap |= (uint64_t)0x1 << port;
        dev_info->two_step.portmap[map] = portmap;
    } else {
        DBG_ERR(("hw_tstamp_enable: Error enabling 2-step timestamp on port:%d\n", port));
        return -1;
    }

    return 0;
}

/**
 * bksync_ptp_hw_tstamp_disable
 *
 * @dev_no: device number
 * @port: port number
 *
 * Description: this is a callback function to disable the timestamping on
 * a given port
 */
static int bksync_ptp_hw_tstamp_disable(int dev_no, int port, int tx_type)
{
    uint64_t portmap = 0;
    int map = 0;
    bksync_dev_t *dev_info = &ptp_priv->dev_info[dev_no];

    if (!dev_info->dev_init) {
        return -1;
    }

    if (tx_type == HWTSTAMP_TX_ONESTEP_SYNC) {
        DBG_VERB(("hw_tstamp_disable: Disable 1Step TS(type:%d) port = %d\n", tx_type, port));
        return 0;
    }

    DBG_VERB(("hw_tstamp_disable: Disable 2Step TS(type:%d) port = %d\n", tx_type, port));

    if ((port > 0) && (port < dev_info->num_phys_ports)) {
        port -= 1;
        map = (port / 64);
        port = (port % 64);

        portmap = dev_info->two_step.portmap[map];
        portmap &= ~((uint64_t)0x1 << port);
        dev_info->two_step.portmap[map]= portmap;
    } else {
        DBG_ERR(("hw_tstamp_disable: Error disabling timestamp on port:%d\n", port));
        return -1;
    }

    return 0;
}

static int bksync_ptp_transport_get(uint8_t *pkt)
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

static int
bksync_txpkt_tsts_tsamp_get(bksync_dev_t *dev_info, int port, uint32_t pkt_seq_id, uint32_t *ts_valid, uint32_t *seq_id, uint64_t *timestamp)
{
    int ret = 0;
    uint64_t data=0ULL;
    u32 fifo_rxctr = 0;

    *ts_valid = 0;
    *timestamp = 0ULL;
    *seq_id = 0;

    data = (port & 0xFFFF) | ((pkt_seq_id & 0xFFFF) << 16);

    ret = bksync_cmd_go(dev_info, BKSYNC_GET_TSTIME, &data, timestamp);
    if (ret >= 0) {
        *ts_valid = data & 0x1ULL;
        *seq_id = (data >> 16) & 0xFFFF;
        fifo_rxctr = (data >> 32) & 0xFFFFFFFF;
        if (*ts_valid) {
            data = (port & 0xFFFF) | ((pkt_seq_id & 0xFFFF) << 16);
            ret = bksync_cmd_go(dev_info, BKSYNC_ACK_TSTIME, &data, 0ULL);
            if (ret >= 0) {
                if (fifo_rxctr != 0) {
                    if (fifo_rxctr != (dev_info->port_stats[port].fifo_rxctr + 1)) {
                        DBG_ERR(("FW reset or lost timestamp FIFO_RxCtr:"
                                    "(Prev %u : Current %u) port:%d pkt_sn:%u hw_sn:%u \n",
                                    dev_info->port_stats[port].fifo_rxctr,
                                    fifo_rxctr, port, pkt_seq_id, *seq_id));
                    }
                    dev_info->port_stats[port].fifo_rxctr = fifo_rxctr;
                }
            } else {
                DBG_ERR(("BKSYNC_ACK_TSTIME failed on port:%d sn:%d\n", port, pkt_seq_id));
            }
        } else {
            DBG_ERR(("BKSYNC_GET_TSTIME invalid on port:%d pkt_sn:%d fw_sn:%d fifo:%d prev_fifo:%d\n",
                    port, pkt_seq_id, *seq_id, fifo_rxctr,
                    dev_info->port_stats[port].fifo_rxctr));
        }
    } else {
        DBG_ERR(("BKSYNC_GET_TSTIME failed on port:%d sn:%d\n", port, pkt_seq_id));
    }
    return ret;
}

/**
 * bksync_ptp_hw_tstamp_tx_time_get
 *
 * @dev_no: device number
 * @port: port number
 * @pkt: packet address
 * @ts: timestamp to be retrieved
 *
 * Description: this is a callback function to retrieve the timestamp on
 * a given port
 * NOTE:
 * Two-step related - fetching the timestamp from portmacro, not needed for one-step
 */
static int bksync_ptp_hw_tstamp_tx_time_get(int dev_no, int port, uint8_t *pkt, uint64_t *ts, int tx_type)
{
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
    bksync_dev_t *dev_info = &ptp_priv->dev_info[dev_no];

    if (!dev_info->dev_init) {
        return -1;
    }

    if (!pkt || !ts || port < 1 || port > 255) {
        return -1;
    }

    *ts = 0;
    port -= 1;

    start = ktime_get();
    /* Linux 5.10.67 kernel complains about missing delay request timestamp for even if
     * configuration is for one-step ptp, hence provided ptp time in skb timestamp
     */
    if (tx_type == HWTSTAMP_TX_ONESTEP_SYNC) {
        DBG_TXTS(("hw_tstamp_tx_time_get: ONESTEP port %d\n", port));
        if (dev_info->ptp_time.ptp_pair_lock == 1) {
            /* use alternate pair when main dataset is being updated */
            *ts = dev_info->ptp_time.ptptime_alt;
        } else {
            *ts = dev_info->ptp_time.ptptime;
        }
        dev_info->port_stats[port].pkt_txctr += 1;
        goto exit;
    }

    tpid_offset = 12;

    /* Parse for nw transport */
    if (transport == 0) {
        transport = bksync_ptp_transport_get(pkt);
    }

    switch(transport)
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

    DBG_TXTS(("hw_tstamp_tx_time_get: port %d pktseq_id %u\n", port, pktseq_id));

    /* Fetch the TX timestamp from shadow memory */
    do {
        bksync_txpkt_tsts_tsamp_get(dev_info, port, pktseq_id, &ts_valid, &seq_id, &timestamp);
        if (ts_valid) {

            if (seq_id == pktseq_id) {
                *ts = timestamp;
                dev_info->port_stats[port].tsts_match += 1;

                delta = ktime_us_delta(ktime_get(), start);
                DBG_TXTS(("Port: %d Skb_SeqID %d FW_SeqId %d and TS:%llx FetchTime %lld retries:%d\n",
                          port, pktseq_id, seq_id, timestamp, delta,
                          (num_retries-retry_cnt)));

                if (delta < dev_info->port_stats[port].tsts_best_fetch_time || dev_info->port_stats[port].tsts_best_fetch_time == 0) {
                    dev_info->port_stats[port].tsts_best_fetch_time = delta;
                }
                if (delta > dev_info->port_stats[port].tsts_worst_fetch_time || dev_info->port_stats[port].tsts_worst_fetch_time == 0) {
                    dev_info->port_stats[port].tsts_worst_fetch_time = delta;
                }
                /* Calculate Moving Average*/
                dev_info->port_stats[port].tsts_avg_fetch_time = ((u32)delta + ((dev_info->port_stats[port].tsts_match - 1) * dev_info->port_stats[port].tsts_avg_fetch_time)) / dev_info->port_stats[port].tsts_match;
                break;
            } else {
                DBG_TXTS(("Discard timestamp on port %d Skb_SeqID %d FW_SeqId %d RetryCnt %d TimeLapsed (%lld us)\n",
                          port, pktseq_id, seq_id, (num_retries - retry_cnt), ktime_us_delta(ktime_get(),start)));

                dev_info->port_stats[port].tsts_discard += 1;
                continue;
            }
        }
        BKSYNC_U_SLEEP(1000);
        retry_cnt--;
    } while(retry_cnt);


    dev_info->port_stats[port].pkt_txctr += 1;

    if (retry_cnt == 0) {
        dev_info->port_stats[port].tsts_timeout += 1;
        DBG_ERR(("FW Response timeout: Tx TS on phy port:%d Skb_SeqID: %d TimeLapsed (%lld us)\n", 
                        port, pktseq_id, ktime_us_delta(ktime_get(), start)));
    }

exit:
    return 0;
}


enum {
    bxconCustomEncapVersionInvalid = 0,
    bxconCustomEncapVersionOne  = 1,

    bxconCustomEncapVersionCurrent = bxconCustomEncapVersionOne,
    bxconCustomEncapVersionReserved = 255 /* last */
} bxconCustomEncapVersion;

enum {
    bxconCustomEncapOpcodeInvalid = 0,
    bxconCustomEncapOpcodePtpRx = 1,
    bxconCustomEncapOpcodeReserved = 255 /* last */
} bxconCustomEncapOpcode;

enum {
    bxconCustomEncapPtpRxTlvInvalid = 0,
    bxconCustomEncapPtpRxTlvPtpRxTime = 1,
    bxconCustomEncapPtpRxTlvReserved = 255 /* last */
} bxconCustomEncapPtpRxTlvType;

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
            gprintk("%s", str);
        }
    }
    if ((idx & 0xf) != 0) {
        sprintf(&str[strlen(str)], "\n");
        gprintk("%s", str);
    }
}

/* onesync_dnx_jr2_parse_rxpkt_system_header : This function parses DNX system headers based
 * on JR2 system headers format
 */
static void bksync_dnx_jr2_parse_rxpkt_system_header(bksync_dev_t *dev_info, uint8_t *raw_pkt_frame, 
                    bksync_dnx_rx_pkt_parse_info_t *rx_pkt_parse_info, int isfirsthdr)
{
    bksync_dnx_jr2_ftmh_base_header_t  *ftmh_base_hdr = NULL;
    bksync_dnx_jr2_timestamp_header_t  *timestamp_hdr = NULL;
    bksync_dnx_jr2_udh_base_header_t   *udh_base_header = NULL;
    bksync_dnx_jr2_ftmh_app_spec_ext_1588v2_t *ftmp_app_spec_ext_1588v2_hdr = NULL;
    bksync_dnx_jr2_pph_fheiext_vlanedit_3b_header_t *fheiext_vlanedit_3b_hdr = NULL;
    bksync_dnx_jr2_pph_fheiext_vlanedit_5b_header_t *fheiext_vlanedit_5b_hdr = NULL;
    bksync_dnx_jr2_pph_base_12b_header_t *pph_base_12b_hdr = NULL;
    uint8_t     raw_frame[64];
    int tmp = 0;

    if ((raw_pkt_frame == NULL) || (rx_pkt_parse_info == NULL)) {
        return;
    }

    rx_pkt_parse_info->rx_frame_len = 0;
    rx_pkt_parse_info->dnx_header_offset = 0;
    rx_pkt_parse_info->pph_header_vlan = 0;
    rx_pkt_parse_info->rx_hw_timestamp = 0;
    rx_pkt_parse_info->src_sys_port = 0;

    for (tmp = 0; tmp < 64; tmp++) {
        raw_frame[tmp] = raw_pkt_frame[tmp];
    }

    /* FTMH */
    ftmh_base_hdr = (bksync_dnx_jr2_ftmh_base_header_t *)(&raw_frame[rx_pkt_parse_info->dnx_header_offset]);
    ftmh_base_hdr->words[0] = ntohl(ftmh_base_hdr->words[0]);
    ftmh_base_hdr->words[1] = ntohl(ftmh_base_hdr->words[1]);

    rx_pkt_parse_info->src_sys_port = (uint16_t)(ftmh_base_hdr->src_sys_port_aggr_0 << 9 | ftmh_base_hdr->src_sys_port_aggr_1 << 1 | ftmh_base_hdr->src_sys_port_aggr_2);
    rx_pkt_parse_info->rx_frame_len = (uint16_t)(ftmh_base_hdr->packet_size_0 << 6 | ftmh_base_hdr->packet_size_1);

    rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_FTMH_HDR_LEN;

    /* FTMH LB-Key Extension */
    if ((dev_info->jr2_header_data).ftmh_lb_key_ext_size > 0) {
        rx_pkt_parse_info->dnx_header_offset += (dev_info->jr2_header_data).ftmh_lb_key_ext_size;
    }

    /* FTMH Stacking Extension */
    if ((dev_info->jr2_header_data).ftmh_stacking_ext_size > 0) {
        rx_pkt_parse_info->dnx_header_offset += (dev_info->jr2_header_data).ftmh_stacking_ext_size;
    }

    /* FTMH BIER BFR Extension */
    if (ftmh_base_hdr->bier_bfr_ext_size > 0) {
        rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_FTMH_BEIR_BFR_EXT_LEN;

    }

    /* FTMH TM Destination Extension */
    if (ftmh_base_hdr->tm_dest_ext_repsent > 0) {
        rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_FTMH_TM_DEST_EXT_LEN;

    }

    /* FTMH Application Specific Extension */
    if (ftmh_base_hdr->app_specific_ext_size > 0) {
        ftmp_app_spec_ext_1588v2_hdr = (bksync_dnx_jr2_ftmh_app_spec_ext_1588v2_t*)(&raw_frame[rx_pkt_parse_info->dnx_header_offset]);
        ftmp_app_spec_ext_1588v2_hdr->word = ntohl(ftmp_app_spec_ext_1588v2_hdr->word);

        if (ftmp_app_spec_ext_1588v2_hdr->type == bksync_dnx_jr2_ftmh_app_spec_ext_type_1588v2) {
        }

        rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_FTMH_APP_SPECIFIC_EXT_LEN;

    }

    /* FTMH Latency-Flow-ID Extension */
    if (ftmh_base_hdr->flow_id_ext_size > 0) {
        rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_FTMH_FLOWID_EXT_LEN ;

    }

    /* Time-stamp Header */
    if ((ftmh_base_hdr->pph_type == BKSYNC_DNXJR2_PPH_TYPE_TSH_ONLY) ||
        (ftmh_base_hdr->pph_type == BKSYNC_DNXJR2_PPH_TYPE_PPH_BASE_TSH) ) {

        timestamp_hdr = (bksync_dnx_jr2_timestamp_header_t* )(&raw_frame[rx_pkt_parse_info->dnx_header_offset]);
        timestamp_hdr->word = ntohl(timestamp_hdr->word);

        rx_pkt_parse_info->rx_hw_timestamp = timestamp_hdr->timestamp;
        rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_TSH_HDR_SIZE;

    }

    /* PPH - internal header */
    if ((ftmh_base_hdr->pph_type == BKSYNC_DNXJR2_PPH_TYPE_PPH_BASE) ||
        (ftmh_base_hdr->pph_type == BKSYNC_DNXJR2_PPH_TYPE_PPH_BASE_TSH)) {

        pph_base_12b_hdr = (bksync_dnx_jr2_pph_base_12b_header_t*)(&raw_frame[rx_pkt_parse_info->dnx_header_offset]);

        pph_base_12b_hdr->word[0] = ntohl(pph_base_12b_hdr->word[0]);
        pph_base_12b_hdr->word[1] = ntohl(pph_base_12b_hdr->word[1]);
        pph_base_12b_hdr->word[2] = ntohl(pph_base_12b_hdr->word[2]);

        rx_pkt_parse_info->dnx_header_offset += (dev_info->jr2_header_data).pph_base_size;


        /* PPH fhei_size handling */
        if (pph_base_12b_hdr->fhei_size > BKSYNC_DNXJR2_PPH_FHEI_TYPE_NONE) {

            switch(pph_base_12b_hdr->fhei_size) {
                case BKSYNC_DNXJR2_PPH_FHEI_TYPE_SZ0: /* 3byte */
                    fheiext_vlanedit_3b_hdr = (bksync_dnx_jr2_pph_fheiext_vlanedit_3b_header_t*)(&raw_frame[rx_pkt_parse_info->dnx_header_offset]);

                    if(fheiext_vlanedit_3b_hdr->type == bksync_dnx_jr2_pph_fheiext_type_vlanedit) {
                        rx_pkt_parse_info->pph_header_vlan = fheiext_vlanedit_3b_hdr->edit_vid1_0 << 7 | fheiext_vlanedit_3b_hdr->edit_vid1_1;
                    }
                    rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_PPH_FHEI_SZ0_SIZE;
                    break;
                case BKSYNC_DNXJR2_PPH_FHEI_TYPE_SZ1: /* 5byte */
                    fheiext_vlanedit_5b_hdr = (bksync_dnx_jr2_pph_fheiext_vlanedit_5b_header_t*)(&raw_frame[rx_pkt_parse_info->dnx_header_offset]);

                    if (fheiext_vlanedit_5b_hdr->type == bksync_dnx_jr2_pph_fheiext_type_vlanedit) {
                        rx_pkt_parse_info->pph_header_vlan = fheiext_vlanedit_5b_hdr->edit_vid1_0 << 7 | fheiext_vlanedit_5b_hdr->edit_vid1_1;
                    } else if (fheiext_vlanedit_5b_hdr->type == bksync_dnx_jr2_pph_fheiext_type_trap_snoop_mirror) {
                    }
                    rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_PPH_FHEI_SZ1_SIZE;
                    break;
                case BKSYNC_DNXJR2_PPH_FHEI_TYPE_SZ2: /* 8byte */
                    rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_PPH_FHEI_SZ2_SIZE;
                    break;
                default:
                    break;
            }
        }

        /* PPH LIF Extension */
        if ((pph_base_12b_hdr->lif_ext_type > 0) && (pph_base_12b_hdr->lif_ext_type < BKSYNC_DNXJER2_PPH_LIF_EXT_TYPE_MAX)) {
            rx_pkt_parse_info->dnx_header_offset += (dev_info->jr2_header_data).pph_lif_ext_size[pph_base_12b_hdr->lif_ext_type];
        }

        /* PPH Learn Extension */
        if (pph_base_12b_hdr->learn_ext_present) {
            rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_PPH_LEARN_EXT_SIZE;
        }
    }

    /* UDH header */
    if (!isfirsthdr) {
    if ((dev_info->jr2_header_data).udh_enable) {
        udh_base_header = (bksync_dnx_jr2_udh_base_header_t* )(&raw_frame[rx_pkt_parse_info->dnx_header_offset]);

        rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_UDH_BASE_HEADER_LEN;
        /* Need to understand more */
        rx_pkt_parse_info->dnx_header_offset += (dev_info->jr2_header_data).udh_data_lenght_per_type[udh_base_header->udh_data_type_0];
        rx_pkt_parse_info->dnx_header_offset += (dev_info->jr2_header_data).udh_data_lenght_per_type[udh_base_header->udh_data_type_1];
        rx_pkt_parse_info->dnx_header_offset += (dev_info->jr2_header_data).udh_data_lenght_per_type[udh_base_header->udh_data_type_2];
        rx_pkt_parse_info->dnx_header_offset += (dev_info->jr2_header_data).udh_data_lenght_per_type[udh_base_header->udh_data_type_3];
    }
    }

    DBG_RX(("DNX PKT PARSE(dev_no:%d): src_sys_port %x rx_hw_timestamp %llx pph_header_vlan %llx dnx_header_offset %u rx_frame_len %d\n",
            dev_info->dev_no,
            rx_pkt_parse_info->src_sys_port, rx_pkt_parse_info->rx_hw_timestamp, rx_pkt_parse_info->pph_header_vlan, 
            rx_pkt_parse_info->dnx_header_offset, rx_pkt_parse_info->rx_frame_len));

}


static inline int
bksync_pkt_custom_encap_ptprx_get(uint8_t *pkt, uint64_t *ing_ptptime)
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

    BKSYNC_UNPACK_U8(custom_hdr, id[0]);
    BKSYNC_UNPACK_U8(custom_hdr, id[1]);
    BKSYNC_UNPACK_U8(custom_hdr, id[2]);
    BKSYNC_UNPACK_U8(custom_hdr, id[3]);
    if (!((id[0] == 'B') && (id[1] == 'C') && (id[2] == 'M') && (id[3] == 'C'))) {
        /* invalid signature */
        return -1;
    }

    BKSYNC_UNPACK_U8(custom_hdr, ver);
    switch (ver) {
        case bxconCustomEncapVersionCurrent:
            break;
        default:
            DBG_ERR(("custom_encap_ptprx_get: Invalid ver\n"));
            return -1;
    }

    BKSYNC_UNPACK_U8(custom_hdr, opc);
    switch (opc) {
        case bxconCustomEncapOpcodePtpRx:
            break;
        default:
            DBG_ERR(("custom_encap_ptprx_get: Invalid opcode\n"));
            return -1;
    }

    BKSYNC_UNPACK_U16(custom_hdr, len);
    tot_len = len;

    if (ing_ptptime != NULL) {
        BKSYNC_UNPACK_U32(custom_hdr, seq_id);

        /* remaining length of custom encap */
        len = len - (custom_hdr - pkt);

        /* process tlv */
        while (len > 0) {
            BKSYNC_UNPACK_U8(custom_hdr, nh_type);
            BKSYNC_UNPACK_U8(custom_hdr, nh_rsvd);
            BKSYNC_UNPACK_U16(custom_hdr, nh_len);
            len = len - (nh_len);
            if (nh_rsvd != 0x0) {
                continue; /* invalid tlv */
            }

            switch (nh_type) {
                case bxconCustomEncapPtpRxTlvPtpRxTime:
                    BKSYNC_UNPACK_U32(custom_hdr, ptp_rx_time[0]);
                    BKSYNC_UNPACK_U32(custom_hdr, ptp_rx_time[1]);
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

        DBG_RX(("custom_encap_ptprx_get: ver=%d opcode=%d tot_len=%d seq_id=0x%x\n", ver, opc, tot_len, seq_id));
    }

    return (tot_len);
}

/**
 * bksync_ptp_hw_tstamp_rx_pre_process
 *
 * @dev_no: device number
 *
 * Description:
 */
static int bksync_ptp_hw_tstamp_rx_pre_process(int dev_no, uint8_t *pkt, uint32_t sspa, uint8_t *pkt_offset)
{
    int ret = -1;
    int custom_encap_len = 0;
    bksync_dev_t *dev_info = &ptp_priv->dev_info[dev_no];

    DBG_RX(("hw_tstamp_rx_pre_process(dev_no:%d): configured_sspa:0x%x recevied_sspa:0x%x\n", dev_no, (dev_info->init_data).uc_port_sysport, sspa));

    if (sspa == (dev_info->init_data).uc_port_sysport) {
        /* Packet is originating from uc, process next system header in KNET */
        ret = 0;
    } else if (pkt_offset != NULL) {
        /* Check for custom encap header */
        custom_encap_len = (int)bksync_pkt_custom_encap_ptprx_get(pkt, NULL);

        DBG_RX(("hw_tstamp_rx_pre_process(dev_no:%d): cust_encap_len=0x%x\n", dev_no, custom_encap_len));

        if (custom_encap_len >= 0) {
            *pkt_offset = (uint8_t)custom_encap_len;
            ret = 0;
        }
    } else {

        bksync_dnx_jr2_parse_rxpkt_system_header(dev_info, NULL,  NULL, 0);
    }

    return ret;
}



/**
 * bksync_ptp_hw_tstamp_rx_time_upscale
 *
 * @dev_no: device number
 * @ts: timestamp to be retrieved
 *
 * Description: this is a callback function to retrieve 64b equivalent of
 *   rx timestamp
 */
static int bksync_ptp_hw_tstamp_rx_time_upscale(int dev_no, int port, struct sk_buff *skb, uint32_t *meta, uint64_t *ts)
{
    int ret = 0;
    int custom_encap_len = 0;
    uint16_t tpid = 0;
    uint16_t msgtype_offset = 0;
    int transport = network_transport;
    int ptp_hdr_offset = 0, ptp_message_len = 0;
    bksync_dev_t *dev_info = &ptp_priv->dev_info[dev_no];

    if (!dev_info->dev_init) {
        return -1;
    }

    DBG_RX_DUMP(("rxtime_upscale: Incoming packet: \n"));
    if (debug & DBG_LVL_RX_DUMP) dbg_dump_pkt(skb->data, skb->len);

    switch (KNET_SKB_CB(skb)->dcb_type) {
        case 28: /* dpp */
        case 39: /* DNX - Q2A, J2C */
            break;
        case 26:
        case 32:
        case 35:
        case 37:
            if (pci_cos != (meta[4] & 0x3F)) {
                return -1;
            }
            break;
        case 38:
            if (pci_cos != ((meta[12] >> 22) & 0x2F)) {
                return -1;
            }
            break;
        case 36:
            if (pci_cos != ((meta[6] >> 22) & 0x2F)) {
                return -1;
            }
            break;
        default:
            DBG_ERR(("rxtime_upscale: Invalid dcb type\n"));
            return -1;
    }

    /* parse custom encap header in pkt for ptp rxtime */
    custom_encap_len = bksync_pkt_custom_encap_ptprx_get((skb->data), ts);

    /* Remove the custom encap header from pkt */
    if (custom_encap_len > 0) {

        skb_pull(skb, custom_encap_len);

        DBG_RX_DUMP(("rxtime_upscale: After removing custom encap: \n"));
        if (debug & DBG_LVL_RX_DUMP) dbg_dump_pkt(skb->data, skb->len);

        msgtype_offset = ptp_hdr_offset = 0;
        tpid = BKSYNC_SKB_U16_GET(skb, (12));
        if (tpid == 0x8100) {
            msgtype_offset += 4;
            ptp_hdr_offset += 4;
        }

        /* Parse for nw transport */
        transport = bksync_ptp_transport_get(skb->data);

        switch(transport)
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

        ptp_message_len = BKSYNC_SKB_U16_GET(skb, (ptp_hdr_offset + 2));

        DBG_RX(("rxtime_upscale: custom_encap_len %d tpid 0x%x transport %d skb->len %d ptp message type %d, ptp_message_len %d\n",
                custom_encap_len, tpid, transport, skb->len, skb->data[msgtype_offset] & 0x0F, ptp_message_len));

        /* Remove padding ,CRC from from L2 packet from returning to Linux Stack */
        if (DEVICE_IS_DNX && (transport == 2) ) {
            skb_trim(skb, ptp_hdr_offset + ptp_message_len);
        }
    }

    if ((port > 0) && (port < dev_info->num_phys_ports)) {
        port -= 1;
        dev_info->port_stats[port].pkt_rxctr += 1;
    }

    return ret;
}


static void bksync_hton64(u8 *buf, const uint64_t *data)
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

static void
bksync_dpp_otsh_update(struct sk_buff *skb, int hwts, int encap_type, int ptp_hdr_offset)
{

    /*
     * Type                 [47:46] type of OAM-TS extension.
     *     0x0: OAM
     *     0x1: 1588v2
     *     0x2: Latency-measurement
     *     0x3: Reserved
     *
     * TP-Command           [45:43] 1588v2 command
     *     0x0: None
     *     0x1: Stamp
     *     0x2: Record (2 step, record Tx-TS in a FIFO)
     *     0x3-0x7: Reserved
     *
     * TS-Encapsulation     [42]    1588v2 Encapsulation
     *     0x0: UDP
     *     0x1: Non UDP
     *
     * OAM-TS-Data          [33:32] OAM-TS-Data
     *     0x1: In-PP-Port.External-BRCM-MAC
     *
     * OAM-TS-Data          [31:0]
     *      Transparent or trapped 1588 events
     *
     * Rx-Time-Stamp
     *      Injected 1588v2 event from ARM/CPU: 0x0
     *
     * Offset   [7:0]   ptp_hdr_offset
     *      Offset from end of System Headers to the start of the 1588v2 frame
     *
     */

    /* PPH_TYPE = OAM-TS */
    skb->data[2] |= 0x80;

    /* OTSH.type = 1588v2 */
    skb->data[6] = 0x40;

    /* OTSH.tp_command = 1-step */
    switch (hwts) {
        case HWTSTAMP_TX_ONESTEP_SYNC:
            skb->data[6] |= ((0x1) << 3);
            break;
        default:
            skb->data[6] |= ((0x2) << 3);
            break;
    }

    /* OTSH.encap_type = udp vs non-udp */
    skb->data[6] |= (((encap_type == 2) ? 1 : 0) << 2);

    /* In-PP-Port.External-BRCM-MAC = 1 */
    skb->data[6] |= (0x1 << 0);

    /* Timestamp: 0x0 */
    skb->data[7] = skb->data[8] = skb->data[9] = skb->data[10] = 0x0;

    skb->data[11] = ptp_hdr_offset;

    return;
}
/* IPv6 WAR to avoid H/W limitation of JR2x series devices */
static void
bksync_dnx_ase1588_tsh_hdr_update_ipv6(bksync_dev_t *dev_info, struct sk_buff *skb, int hwts, int encap_type, int ptp_hdr_offset)
{
    int itmh_offset = 0, ftmh_ase_offset = 0, tse_offset = 0;
    int pph_udh_present = 0;
    /* Module Hdr [16] + PTCH [2] + ITMH [5] + ASE1588 [6] + TSH [4] + Internal Hdr [12] + UDH base [1] */

    /* For DNX3 for CF update 1588v2_Offset should also have system_header length of except Module HDR [16] */
    if ((dev_info->init_data).application_v2) {
        ptp_hdr_offset -= BKSYNC_DNXJR2_MODULE_HEADER_LEN;
    } else {
        ptp_hdr_offset -= BKSYNC_DNXJR2_MODULE_HEADER_LEN - 1;
    }

    if (ptp_hdr_offset == 93) {
        /* PTCH [3] + ITMH [5] + ASE1588 [6] + TSH [4] + Internal Hdr [12] + UDH base [1] = 31 + IPv6 [62] + VLAN [0] = 93 */
        /* Inserting TSH and ASE before PPH and UDH - shifted PPH and UDH by 13 bytes in skb->data */
        uint8_t pph_start[BKSYNC_DNXJR2_PPH_HEADER_LEN];
        uint8_t udh_start;
        itmh_offset = BKSYNC_DNXJR2_MODULE_HEADER_LEN + BKSYNC_DNX_PTCH_1_SIZE;

        pph_udh_present = 1;
        memcpy(pph_start, &skb->data[itmh_offset + BKSYNC_DNXJR2_ITMH_HEADER_LEN], BKSYNC_DNXJR2_PPH_HEADER_LEN);
        udh_start = skb->data[itmh_offset + BKSYNC_DNXJR2_ITMH_HEADER_LEN + BKSYNC_DNXJR2_PPH_HEADER_LEN];
        /* copying pph after ase + tsh 34 = module + ptch + itmh + ase + tsh */
        memcpy(&skb->data[34], pph_start, BKSYNC_DNXJR2_PPH_HEADER_LEN);
        /* copying udh after pph 46 = module + ptch + itmh + ase + tsh + pph */
        skb->data[46] = udh_start;
    }
    else {
        /* PTCH [2] + ITMH [5] + ASE1588 [6] + TSH [4] + Internal Hdr [12] + UDH base [1] = 30 + IPv6 [62] + VLAN [4] = 96 */
        /* PTCH [2] + ITMH [5] + ASE1588 [6] + TSH [4] + Internal Hdr [12] + UDH base [1] = 30 + IPv6 [62] + VLAN [0] = 92 */
        itmh_offset = BKSYNC_DNXJR2_MODULE_HEADER_LEN + BKSYNC_DNX_PTCH_2_SIZE;
    }
    /* ITMH */
    /* App Specific Ext Present  ASE 1588*/
    skb->data [itmh_offset] |= (0x1 << 3);

    /* PPH_TYPE - TSH  +  Internal Hdr */
    skb->data [itmh_offset] |= (0x3 << 1);   /* TSH + PPH Only */

    ftmh_ase_offset = itmh_offset + BKSYNC_DNXJR2_ITMH_HEADER_LEN;
    /* ASE 1588 ext */
    memset(&skb->data[ftmh_ase_offset], 0x0, BKSYNC_DNXJR2_FTMH_APP_SPECIFIC_EXT_LEN);

    /* OTSH.encap_type = udp vs non-udp  - 1bit (15:15) */
    /* encap type - 2 L2, 4 & 6 UDP */
    skb->data[ftmh_ase_offset + 4] |= (((encap_type == 2) ? 1 : 0) << 7);

    /* ASE1588 1588v2 command -  one step or two step  3bit (14:12) */
    /* ASE1588 1588v2 command should be zero for CF update */

        /*  offset to start of 1588v2 frame  - 8 bit (11:4) */
    skb->data [ftmh_ase_offset + 4] = skb->data [ftmh_ase_offset + 4] | ((ptp_hdr_offset) & 0xf0) >> 4;
    skb->data [ftmh_ase_offset + 5] = ((ptp_hdr_offset) & 0xf) << 4;

    /* ASE1588 type = 1588v2  - 4 bit (0:3) */
    skb->data [ftmh_ase_offset + 5] = skb->data [ftmh_ase_offset + 5] | 0x01;

    tse_offset = ftmh_ase_offset + BKSYNC_DNXJR2_FTMH_APP_SPECIFIC_EXT_LEN;
    memset(&skb->data[tse_offset], 0x0, BKSYNC_DNXJR2_TSH_HDR_SIZE);

    if(!pph_udh_present) {
        /* Internal Header */
        skb->data [33] = skb->data [34] = skb->data [35] = skb->data [36] = 0x00;
        skb->data [37] = skb->data [38] = skb->data [39] = skb->data [40] = 0x00;
        skb->data [41] = skb->data [42] = skb->data [43] = skb->data [44] = 0x00;

        skb->data [44] = 0x42;
        skb->data [43] = 0x07;
        skb->data [42] = 0x10;

        /* UDH Base Hdr */
        skb->data [45] = 0;
    }
    return;
}

static void
bksync_dnx_ase1588_tsh_hdr_update(bksync_dev_t *dev_info, struct sk_buff *skb, int hwts, int encap_type, int ptp_hdr_offset)
{
    int itmh_offset = 0, ftmh_ase_offset = 0, tse_offset = 0;
        /* Module Hdr [16] + PTCH [2] + ITMH [5] + ASE1588 [6] + TSH [4] */

        /* For JR3 for CF update 1588v2_Offset should also have system_header length of
         * PTCH [2] + ITMH [5] + ASE1588 [6] + TSH [4] = 17. */
        if ((dev_info->init_data).application_v2) {
            ptp_hdr_offset -= BKSYNC_DNXJR2_MODULE_HEADER_LEN;
        }
        else {
            ptp_hdr_offset -= (BKSYNC_DNXJR2_MODULE_HEADER_LEN + BKSYNC_DNX_PTCH_2_SIZE + BKSYNC_DNXJR2_ITMH_HEADER_LEN);
        }

    /* Inserting TSH and ASE before PPH and UDH - shifted PPH and UDH by 13 bytes in skb->data */
    if (ptp_hdr_offset >= 73) {   /*PTCH1 + ITMH + ASE1588 + TSH + PPH + UDH + Upto start of PTP = 73*/
        uint8_t pph_start[BKSYNC_DNXJR2_PPH_HEADER_LEN];
        uint8_t udh_start;
        itmh_offset = BKSYNC_DNXJR2_MODULE_HEADER_LEN + BKSYNC_DNX_PTCH_1_SIZE;

        memcpy(&pph_start[0], &skb->data[itmh_offset + BKSYNC_DNXJR2_ITMH_HEADER_LEN], BKSYNC_DNXJR2_PPH_HEADER_LEN);
        udh_start = skb->data[BKSYNC_DNXJR2_ITMH_HEADER_LEN + BKSYNC_DNXJR2_PPH_HEADER_LEN];
        /* copying pph after ase + tsh 34 = module + ptch + itmh + ase + tsh */
        memcpy(&skb->data[34], pph_start, BKSYNC_DNXJR2_PPH_HEADER_LEN);
        /* copying udh after pph 46 = module + ptch + itmh + ase + tsh + pph */
        skb->data[46] = udh_start;
    }
    else {
        itmh_offset = BKSYNC_DNXJR2_MODULE_HEADER_LEN + BKSYNC_DNX_PTCH_2_SIZE;
    }
    /* ITMH */
    /* App Specific Ext Present */
    skb->data [itmh_offset] |= (1 << 3);

    /* PPH_TYPE - TSH */
    skb->data [itmh_offset] |= (0x2 << 1);

    ftmh_ase_offset = itmh_offset + BKSYNC_DNXJR2_ITMH_HEADER_LEN;
    /* ASE 1588 ext */
    memset(&skb->data[ftmh_ase_offset], 0x0, BKSYNC_DNXJR2_FTMH_APP_SPECIFIC_EXT_LEN);

    /* OTSH.encap_type = udp vs non-udp  - 1bit (15:15) */
    /* encap type - 2 L2, 4 & 6 UDP */
    skb->data[ftmh_ase_offset + 4] |= (((encap_type == 2) ? 1 : 0) << 7);

    /* ASE1588 1588v2 command -  one step or two step  3bit (14:12) */
    switch (hwts) {
        case HWTSTAMP_TX_ONESTEP_SYNC:
            skb->data[ftmh_ase_offset + 4] |= ((0x1) << 4);
            break;
        default:
            skb->data[ftmh_ase_offset + 4] |= ((0x2) << 4);
            break;
    }

    /*  offset to start of 1588v2 frame  - 8 bit (11:4) */
    skb->data [ftmh_ase_offset + 4] = skb->data [ftmh_ase_offset + 4] | ((ptp_hdr_offset) & 0xf0) >> 4;
    skb->data [ftmh_ase_offset + 5] = ((ptp_hdr_offset) & 0xf) << 4;

    /* ASE1588 type = 1588v2  - 4 bit (0:3) */
    skb->data [ftmh_ase_offset + 5] = skb->data [ftmh_ase_offset + 5] | 0x01;

    tse_offset = ftmh_ase_offset + BKSYNC_DNXJR2_FTMH_APP_SPECIFIC_EXT_LEN;
    /* TSH Timestamp: 0x0 */
    memset(&skb->data[tse_offset], 0x0, BKSYNC_DNXJR2_TSH_HDR_SIZE);

    return;
}



static int bksync_ptp_hw_tstamp_tx_meta_get(int dev_no,
                                            int hwts, int hdrlen,
                                            struct sk_buff *skb,
                                            uint64_t *tstamp,
                                            u32 **md)
{
    uint16_t tpid = 0, ethertype;
    int md_offset = 0;
    int pkt_offset = 0;
    int ptp_hdr_offset = 0;
    int transport = network_transport;
    s64 ptptime  = 0;
    s64 ptpcounter = 0;
    int64_t corrField = 0;
    int32_t negCurTS32;
    int64_t negCurTS64;
    bksync_dev_t *dev_info = &ptp_priv->dev_info[dev_no];

    if (!dev_info->dev_init) {
        return -1;
    }

    if (dev_info->ptp_time.ptp_pair_lock == 1) {
        /* use alternate pair when main dataset is being updated */
        ptptime = dev_info->ptp_time.ptptime_alt;
        ptpcounter = dev_info->ptp_time.reftime_alt;
    } else {
        ptptime = dev_info->ptp_time.ptptime;
        ptpcounter = dev_info->ptp_time.reftime;
    }

    negCurTS32 = - (int32_t) ptpcounter;
    negCurTS64 = - (int64_t)(ptpcounter);

    if (CMICX_DEV_TYPE || DEVICE_IS_DPP) {
        pkt_offset = ptp_hdr_offset = hdrlen;
    }

    /* Need to check VLAN tag if packet is tagged */
    tpid = BKSYNC_SKB_U16_GET(skb, (pkt_offset + 12));
    if (tpid == 0x8100) {
        md_offset = 4;
        ptp_hdr_offset += 4;

        if (DEVICE_IS_DNX && vnptp_l2hdr_vlan_prio != 0) {
            ethertype =  BKSYNC_SKB_U16_GET(skb, hdrlen + 12 + 4);
            if (ethertype == 0x88F7 || ethertype == 0x0800 || ethertype == 0x86DD) {
                if (skb->data[hdrlen + 14] == 0x00) {
                    skb->data[hdrlen + 14] |= (vnptp_l2hdr_vlan_prio << 5);
                }
            }
        }
    }

    /* One Step Meta Data */
    if (hwts == HWTSTAMP_TX_ONESTEP_SYNC) {
        md_offset += 8;
        switch (KNET_SKB_CB(skb)->dcb_type) {
            case 26:
                corrField = (((int64_t)negCurTS32) << 16);
                if (negCurTS32 >= 0) {
                    md_offset += 8;
                }
                break;
            default:
                corrField = (((int64_t)negCurTS64) << 16);
                break;
        }
    }


    /* Parse for nw transport */
    if (transport == 0) {
        transport = bksync_ptp_transport_get(skb->data + pkt_offset);
    }

    switch(transport)
    {
        case 2: /* IEEE 802.3 */
            ptp_hdr_offset += 14;
            if (KNET_SKB_CB(skb)->dcb_type == 32) {
                if (md) *md = &sobmhrawpkts_dcb32[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 26) {
                if (md) *md = &sobmhrawpkts_dcb26[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 35) {
                if (md) *md = &sobmhrawpkts_dcb35[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 36) {
                if (md) *md = &sobmhrawpkts_dcb36[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 38) {
                if (md) *md = &sobmhrawpkts_dcb38[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 37) {
                if (md) *md = &sobmhrawpkts_dcb37[md_offset];
            }
            break;
        case 4: /* UDP IPv4   */
            ptp_hdr_offset += 42;
            if (KNET_SKB_CB(skb)->dcb_type == 32) {
                if (md) *md = &sobmhudpipv4_dcb32[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 26) {
                if (md) *md = &sobmhudpipv4_dcb26[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 35) {
                if (md) *md = &sobmhudpipv4_dcb35[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 36) {
                if (md) *md = &sobmhudpipv4_dcb36[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 38) {
                if (md) *md = &sobmhudpipv4_dcb38[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 37) {
                if (md) *md = &sobmhudpipv4_dcb37[md_offset];
            }
            break;
        case 6: /* UDP IPv6   */
            ptp_hdr_offset += 62;
            if (KNET_SKB_CB(skb)->dcb_type == 32) {
                if (md) *md = &sobmhudpipv6_dcb32[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 26) {
                if (md) *md = &sobmhudpipv6_dcb26[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 35) {
                if (md) *md = &sobmhudpipv6_dcb35[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 36) {
                if (md) *md = &sobmhudpipv6_dcb36[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 38) {
                if (md) *md = &sobmhudpipv6_dcb38[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 37) {
                if (md) *md = &sobmhudpipv6_dcb37[md_offset];
            }
            break;
        default:
            ptp_hdr_offset += 42;
            if (KNET_SKB_CB(skb)->dcb_type == 32) {
                if (md) *md = &sobmhudpipv4_dcb32[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 26) {
                if (md) *md = &sobmhudpipv4_dcb26[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 35) {
                if (md) *md = &sobmhudpipv4_dcb35[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 36) {
                if (md) *md = &sobmhudpipv4_dcb36[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 38) {
                if (md) *md = &sobmhudpipv4_dcb38[md_offset];
            } else if(KNET_SKB_CB(skb)->dcb_type == 37) {
                if (md) *md = &sobmhudpipv4_dcb37[md_offset];
            }
            break;
    }

    if (DEVICE_IS_DPP && (hdrlen > (BKSYNC_DNX_PTCH_2_SIZE))) {
        DBG_TX_DUMP(("hw_tstamp_tx_meta_get(dev_no:%d): Before OTSH updates\n", dev_no));
        if (debug & DBG_LVL_TX_DUMP) dbg_dump_pkt(skb->data, skb->len);

        DBG_TX(("hw_tstamp_tx_meta_get(dev_no:%d): Before: ptch[0]: 0x%x ptch[1]: 0x%x itmh[0]: 0x%x "
                  "oam-ts[0]: 0x%x pkt[0]:0x%x\n", dev_no, skb->data[0], skb->data[1], skb->data[2],
                  skb->data[6], skb->data[12]));

        bksync_dpp_otsh_update(skb, hwts, transport, (ptp_hdr_offset - pkt_offset));

        DBG_TX(("hw_tstamp_tx_meta_get(dev_no:%d): After : ptch[0]: 0x%x itmh[0]: 0x%x oam-ts[0]: 0x%x "
                  "pkt[0]:0x%x\n", dev_no, skb->data[0], skb->data[2], skb->data[6], skb->data[12]));

        DBG_TX_DUMP(("hw_tstamp_tx_meta_get(dev_no:%d): After OTSH updates\n", dev_no));
        if (debug & DBG_LVL_TX_DUMP) dbg_dump_pkt(skb->data, skb->len);
    } else if (DEVICE_IS_DNX && (hdrlen > (BKSYNC_DNX_PTCH_2_SIZE))) {

        switch(transport)
        {
            case 6: /* UDP IPv6   */
                bksync_dnx_ase1588_tsh_hdr_update_ipv6(dev_info, skb, hwts, transport, ptp_hdr_offset);
                break;
            case 4: /* UDP IPv4   */
            case 2: /* IEEE 802.3 */
            default:
                bksync_dnx_ase1588_tsh_hdr_update(dev_info, skb, hwts, transport, ptp_hdr_offset);
                break;
        }
    }

    DBG_TX(("hw_tstamp_tx_meta_get(dev_no:%d): ptptime: 0x%llx ptpcounter: 0x%llx\n", dev_no, ptptime, ptpcounter));

    DBG_TX(("hw_tstamp_tx_meta_get(dev_no:%d): ptpmessage type: 0x%x hwts: %d\n", dev_no, skb->data[ptp_hdr_offset] & 0x0f, hwts));


    if ((hwts == HWTSTAMP_TX_ONESTEP_SYNC) &&
        (BKSYNC_PTP_EVENT_MSG((skb->data[ptp_hdr_offset] & 0x0F)))) {
        /* One Step Timestamp Field updation */
        int port;
        int corr_offset = ptp_hdr_offset + 8;
        int origin_ts_offset = ptp_hdr_offset + 34;
        u32 tmp;
        struct timespec64 ts = {0};
        int udp_csum_regen;
        u32 udp_csum20;
        u16 udp_csum;

        udp_csum = BKSYNC_SKB_U16_GET(skb, (ptp_hdr_offset - 2));

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
        bksync_hton64(&(skb->data[corr_offset]), (const u64 *)&corrField);

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

            udp_csum20 += BKSYNC_SKB_U16_GET(skb, (corr_offset + 0));
            udp_csum20 += BKSYNC_SKB_U16_GET(skb, (corr_offset + 2));
            udp_csum20 += BKSYNC_SKB_U16_GET(skb, (corr_offset + 4));
            udp_csum20 += BKSYNC_SKB_U16_GET(skb, (corr_offset + 6));

            udp_csum20 += BKSYNC_SKB_U16_GET(skb, (origin_ts_offset + 0));
            udp_csum20 += BKSYNC_SKB_U16_GET(skb, (origin_ts_offset + 2));
            udp_csum20 += BKSYNC_SKB_U16_GET(skb, (origin_ts_offset + 4));
            udp_csum20 += BKSYNC_SKB_U16_GET(skb, (origin_ts_offset + 6));
            udp_csum20 += BKSYNC_SKB_U16_GET(skb, (origin_ts_offset + 8));

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
            *tstamp = ptptime;
        }

        port = KNET_SKB_CB(skb)->port;
        DBG_TX(("hw_tstamp_tx_meta_get(dev_no:%d): ptp msg type %d packet tstamp : 0x%llx corrField: 0x%llx port:%d\n",
                 dev_no, (skb->data[ptp_hdr_offset] & 0x0F), ptptime, corrField, port));

        if ((port > 0) && (port < dev_info->num_phys_ports)) {
            port -= 1;
            dev_info->port_stats[port].pkt_txonestep += 1;
        }
    }

    DBG_TX_DUMP(("hw_tstamp_tx_meta_get(dev_no:%d): PTP Packet\n", dev_no));
    if (debug & DBG_LVL_TX_DUMP) dbg_dump_pkt(skb->data, skb->len);

    return 0;
}


static int bksync_ptp_hw_tstamp_ptp_clock_index_get(int dev_no)
{
    int phc_index = -1;
    bksync_dev_t *dev_info = &ptp_priv->dev_info[dev_no];

    if (dev_info->ptp_clock) {
        phc_index =  ptp_clock_index(dev_info->ptp_clock);
    }

    return phc_index;
}


/**
* bcm_ptp_time_keep - call timecounter_read every second to avoid timer overrun
*                 because  a 32bit counter, will timeout in 4s
*/
static void bksync_ptp_time_keep(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    struct bksync_ptp_priv *priv =
                        container_of(dwork, struct bksync_ptp_priv, time_keep);
    struct timespec64 ts;
    bksync_dev_t *dev_info;
    int dev_no = 0;

    for (dev_no = 0; dev_no < priv->max_dev; dev_no++) {
        dev_info = &priv->dev_info[dev_no];
        /* Call bcm_ptp_gettime function to keep the ref_time_64 and ref_counter_48 in sync */
        bksync_ptp_gettime(&(dev_info->ptp_info), &ts);
    }
    schedule_delayed_work(&priv->time_keep, __msecs_to_jiffies(phc_update_intv_msec));
}

static void bksync_ptp_time_keep_init(void)
{
    if (!ptp_priv->timekeep_status) {
        INIT_DELAYED_WORK(&(ptp_priv->time_keep), bksync_ptp_time_keep);
        schedule_delayed_work(&ptp_priv->time_keep, __msecs_to_jiffies(phc_update_intv_msec));

        ptp_priv->timekeep_status = 1;
    }

    return;
}

static void bksync_ptp_time_keep_deinit(void)
{
    if (ptp_priv->timekeep_status) {
        /* Cancel delayed work */
        cancel_delayed_work_sync(&(ptp_priv->time_keep));

        ptp_priv->timekeep_status = 0;
    }

    return;
}

/* PTP_EXTTS logging */
static void bksync_ptp_extts_logging(struct work_struct *work)
{
    struct delayed_work *dwork = to_delayed_work(work);
    struct bksync_ptp_priv *priv = container_of(dwork, struct bksync_ptp_priv, extts_logging);
    struct ptp_clock_event event;
    int event_id = -1;
    int head = -1, tail = -1;
    int dev_no = 0;
    bksync_dev_t *dev_info;

    for (dev_no = 0; dev_no < ptp_priv->max_dev; dev_no++) {
        dev_info = &ptp_priv->dev_info[dev_no];

        if (!dev_info->dev_init) {
            continue;
        }

        if (dev_info->extts_log == NULL) {
            continue;
        }

        if (dev_info->extts_log->overflow) {
            DBG_VERB(("EXTTS queue overflow\n"));
        }

        tail = (int)dev_info->extts_log->tail;
        head = dev_info->extts_event.head;

        head = (head + 1) % BKSYNC_NUM_EVENT_TS;
        while (tail != head) {
            switch (dev_info->extts_log->event_ts[head].ts_event_id) {
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

            if (event_id < 0 || dev_info->extts_event.enable[event_id] != 1) {
                memset((void *)&(dev_info->extts_log->event_ts[head]), 0, sizeof(dev_info->extts_log->event_ts[head]));

                dev_info->extts_event.head = head;
                dev_info->extts_log->head = head;

                head = (head + 1) % BKSYNC_NUM_EVENT_TS;
                continue;
            }

            event.type = PTP_CLOCK_EXTTS;
            /* Determine the user event_id for the multi core devices */
            event.index = event_id + (dev_info->dev_no * BKSYNC_NUM_GPIO_EVENTS);;
            event.timestamp = ((s64)dev_info->extts_log->event_ts[head].tstamp.sec * 1000000000) + dev_info->extts_log->event_ts[head].tstamp.nsec;
            ptp_clock_event(dev_info->ptp_clock, &event);

            dev_info->extts_event.head = head;
            dev_info->extts_log->head = head;

            head = (head + 1) % BKSYNC_NUM_EVENT_TS;
        }
    }

    schedule_delayed_work(&priv->extts_logging, __msecs_to_jiffies(100));
}

static void bksync_ptp_extts_logging_init(void)
{
    INIT_DELAYED_WORK(&(ptp_priv->extts_logging), bksync_ptp_extts_logging);
    schedule_delayed_work(&ptp_priv->extts_logging, __msecs_to_jiffies(100));
}

static void bksync_ptp_extts_logging_deinit(void)
{
    cancel_delayed_work_sync(&(ptp_priv->extts_logging));
}

static int bksync_ptp_init(bksync_dev_t *dev_info, struct ptp_clock_info *ptp)
{
    int ret = -1;
    u64 subcmd, subcmd_data;

    ret = bksync_cmd_go(dev_info, BKSYNC_INIT, NULL, NULL);
    DBG_VERB(("bksync_ptp_init: BKSYNC_INIT; rv:%d\n", ret));
    if (ret < 0) goto err_exit;
    ptp_sleep(1);

    if (!DEVICE_IS_DPP && !DEVICE_IS_DNX) {
        return 0;
    }

    subcmd = BKSYNC_SYSINFO_UC_PORT_NUM;
    subcmd_data = (dev_info->init_data).uc_port_num;
    ret = bksync_cmd_go(dev_info, BKSYNC_SYSINFO, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_ptp_init: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));
    if (ret < 0) goto err_exit;

    subcmd = BKSYNC_SYSINFO_UC_PORT_SYSPORT;
    subcmd_data = (dev_info->init_data).uc_port_sysport;
    ret = bksync_cmd_go(dev_info, BKSYNC_SYSINFO, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_ptp_init: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));
    if (ret < 0) goto err_exit;

    subcmd = BKSYNC_SYSINFO_HOST_CPU_PORT;
    subcmd_data = (dev_info->init_data).host_cpu_port;
    ret = bksync_cmd_go(dev_info, BKSYNC_SYSINFO, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_ptp_init: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));
    if (ret < 0) goto err_exit;

    subcmd = BKSYNC_SYSINFO_HOST_CPU_SYSPORT;
    subcmd_data = (dev_info->init_data).host_cpu_sysport;
    ret = bksync_cmd_go(dev_info, BKSYNC_SYSINFO, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_ptp_init: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));
    if (ret < 0) goto err_exit;

    subcmd = BKSYNC_SYSINFO_UDH_LEN;
    subcmd_data = (dev_info->init_data).udh_len;
    ret = bksync_cmd_go(dev_info, BKSYNC_SYSINFO, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_ptp_init: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));
    if (ret < 0) goto err_exit;


err_exit:
    return ret;
}

static int bksync_ptp_deinit(bksync_dev_t *dev_info)
{
    int ret = -1;

    ret = bksync_cmd_go(dev_info, BKSYNC_DEINIT, NULL, NULL);
    DBG_VERB(("bksync_ptp_deinit: rv:%d\n", ret));

    return ret;
}

static int bksync_broadsync_cmd(bksync_dev_t *dev_info, int bs_id)
{
    int ret = -1;
    u64 subcmd, subcmd_data;

    if (!dev_info->dev_init) {
        return -1;
    }

    subcmd = (bs_id == 0) ? BKSYNC_BROADSYNC_BS0_CONFIG : BKSYNC_BROADSYNC_BS1_CONFIG;

    subcmd_data =  ((dev_info->bksync_bs_info[bs_id]).enable & 0x1);
    subcmd_data |= (((dev_info->bksync_bs_info[bs_id]).mode & 0x1) << 8);
    subcmd_data |= ((dev_info->bksync_bs_info[bs_id]).hb << 16);
    subcmd_data |= (((u64)(dev_info->bksync_bs_info[bs_id]).bc) << 32);

    ret = bksync_cmd_go(dev_info, BKSYNC_BROADSYNC, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_broadsync_cmd: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));

    return ret;
}

static int bksync_broadsync_status_cmd(bksync_dev_t *dev_info, int bs_id, u64 *status)
{
    int ret = -1;
    u64 subcmd;

    if (!dev_info->dev_init) {
        return -1;
    }

    subcmd = (bs_id == 0) ? BKSYNC_BROADSYNC_BS0_STATUS_GET : BKSYNC_BROADSYNC_BS1_STATUS_GET;

    ret = bksync_cmd_go(dev_info, BKSYNC_BROADSYNC, &subcmd, status);
    DBG_VERB(("bksync_broadsync_status_cmd: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, *status, ret));

    return ret;
}

static int bksync_broadsync_phase_offset_cmd(bksync_dev_t *dev_info, int bs_id, bksync_time_spec_t offset)
{
    int ret = -1;
    u64 data0, data1;
    int64_t phase_offset = 0;

    if (!dev_info->dev_init) {
        return -1;
    }

    /* Only in input mode */
    if (dev_info->bksync_bs_info[bs_id].mode == 0) {
        dev_info->bksync_bs_info[bs_id].offset = offset;
    } else {
        memset(&dev_info->bksync_bs_info[bs_id].offset, 0, sizeof(bksync_time_spec_t));
    }

    data0 = (bs_id == 0) ? BKSYNC_BROADSYNC_BS0_PHASE_OFFSET_SET : BKSYNC_BROADSYNC_BS1_PHASE_OFFSET_SET;

    phase_offset = dev_info->bksync_bs_info[bs_id].offset.sec * 1000000000 + dev_info->bksync_bs_info[bs_id].offset.nsec;
    phase_offset *= (dev_info->bksync_bs_info[bs_id].offset.sign) ? -1 : 1;

    data1 = (uint64_t)phase_offset;

    ret = bksync_cmd_go(dev_info, BKSYNC_BROADSYNC, &data0, &data1);
    DBG_VERB(("bksync_broadsync_phase_offset_cmd: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", data0, data1, ret));

    return ret;
}

static int bksync_gpio_cmd(bksync_dev_t *dev_info, int gpio_num)
{
    int ret = -1;
    u64 subcmd, subcmd_data;

    if (!dev_info->dev_init) {
        return -1;
    }

    switch (gpio_num) {
        case 0:
            subcmd = BKSYNC_GPIO_0;
            break;
        case 1:
            subcmd = BKSYNC_GPIO_1;
            break;
        case 2:
            subcmd = BKSYNC_GPIO_2;
            break;
        case 3:
            subcmd = BKSYNC_GPIO_3;
            break;
        case 4:
            subcmd = BKSYNC_GPIO_4;
            break;
        case 5:
            subcmd = BKSYNC_GPIO_5;
            break;
        default:
            return ret;
    }

    subcmd_data =  ((dev_info->bksync_gpio_info[gpio_num]).enable & 0x1);
    subcmd_data |= (((dev_info->bksync_gpio_info[gpio_num]).mode & 0x1) << 8);
    subcmd_data |= ((u64)((dev_info->bksync_gpio_info[gpio_num]).period) << 16);

    ret = bksync_cmd_go(dev_info, BKSYNC_GPIO, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_gpio_cmd: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));

    return ret;
}

static int bksync_gpio_phaseoffset_cmd(bksync_dev_t *dev_info, int gpio_num)
{
    int ret = -1;
    u64 subcmd, subcmd_data;

    if (!dev_info->dev_init) {
        return -1;
    }

    switch (gpio_num) {
        case 0:
            subcmd = BKSYNC_GPIO_0;
            break;
        case 1:
            subcmd = BKSYNC_GPIO_1;
            break;
        case 2:
            subcmd = BKSYNC_GPIO_2;
            break;
        case 3:
            subcmd = BKSYNC_GPIO_3;
            break;
        case 4:
            subcmd = BKSYNC_GPIO_4;
            break;
        case 5:
            subcmd = BKSYNC_GPIO_5;
            break;
        default:
            return ret;
    }

    subcmd_data = (dev_info->bksync_gpio_info[gpio_num]).phaseoffset;
    ret = bksync_cmd_go(dev_info, BKSYNC_GPIO_PHASEOFFSET, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_gpio_phaseoffset_cmd: subcmd: 0x%llx "
              "subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));

    return ret;
}

#ifdef BDE_EDK_SUPPORT
static int bksync_ptp_tod_cmd(bksync_dev_t *dev_info, int sign, uint64_t offset_sec, uint32_t offset_nsec)
{
    int ret = -1;
    u64 data0 = 0, data1 = 0;

    if (!dev_info->dev_init) {
        return -1;
    }

    data0 = ((uint64_t)(sign & 0x1)) << 47;
    data0 |= (offset_sec & 0x7FFFFFFFFFFF);

    data1 = offset_nsec;

    ret = bksync_cmd_go(dev_info, BKSYNC_PTP_TOD, &data0, &data1);
    DBG_VERB(("bksync_ptp_tod_cmd: data0: 0x%llx data1: 0x%llx; rv:%d\n", data0, data1, ret));

    return ret;
}

static int bksync_ptp_tod_get_cmd(bksync_dev_t *dev_info, fw_tstamp_t *tod_time)
{
    int ret = -1;
    u64 data0 = 0, data1 = 0;

    if (!dev_info->dev_init) {
        return -1;
    }

    ret = bksync_cmd_go(dev_info, BKSYNC_PTP_TOD_GET, &data0, &data1);

    tod_time->sec = data0;
    tod_time->nsec = data1;
    DBG_VERB(("bksync_ptp_tod_get_cmd: data0: 0x%llx data1: 0x%llx; rv:%d\n", data0, data1, ret));

    return ret;
}

static int bksync_ntp_tod_cmd(bksync_dev_t *dev_info, uint8_t leap_sec_ctrl_en, uint8_t leap_sec_op, uint64_t epoch_offset)
{
    int ret = -1;
    u64 data0,data1;

    if (!dev_info->dev_init) {
        return -1;
    }

    data0 = ((uint64_t)(leap_sec_ctrl_en  & 0x1) << 1);
    data0 |= (leap_sec_op & 0x1);

    data1 = epoch_offset;

    ret = bksync_cmd_go(dev_info, BKSYNC_NTP_TOD, &data0, &data1);
    DBG_VERB(("bksync_ntp_tod_cmd: data0: 0x%llx data1: 0x%llx; rv:%d\n", data0, data1, ret));

    return ret;
}

static int bksync_ntp_tod_get_cmd(bksync_dev_t *dev_info, fw_tstamp_t *tod_time)
{
    int ret = -1;
    u64 data0, data1;

    if (!dev_info->dev_init) {
        return -1;
    }

    ret = bksync_cmd_go(dev_info, BKSYNC_NTP_TOD_GET, &data0, &data1);

    tod_time->sec = data0;
    tod_time->nsec = data1;
    DBG_VERB(("bksync_ntp_tod_get_cmd: data0: 0x%llx data1: 0x%llx; rv:%d\n", data0, data1, ret));

    return ret;
}
#endif

#ifndef BDE_EDK_SUPPORT
static int bksync_evlog_cmd(int event, int enable)
{
    int ret = -1;
    int addr_offset;
    u64 subcmd = 0, subcmd_data = 0;
    bksync_evlog_t tmp;
    int dev_no = master_core;
    bksync_dev_t *dev_info = &ptp_priv->dev_info[dev_no];

    if (!dev_info->dev_init) {
        return ret;
    }

    subcmd = event;
    addr_offset = ((u8 *)&(tmp.event_timestamps[event]) - (u8 *)&(tmp.event_timestamps[0]));

    if (enable) {
        subcmd_data = (dev_info->dma_mem + addr_offset);
    } else {
        subcmd_data = 0;
    }

    ret = bksync_cmd_go(dev_info, BKSYNC_EVLOG, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_evlog_cmd: subcmd: 0x%llx subcmd_data: 0x%llx rv:%d\n", subcmd, subcmd_data, ret));

    return ret;
}
#endif

/*
 * Device Debug Statistics Proc Entry
 */
/**
* This function is called at the beginning of a sequence.
* ie, when:
*    - the /proc/bcm/ksync/stats file is read (first time)
*   - after the function stop (end of sequence)
*
*/
static void *bksync_proc_seq_start(struct seq_file *s, loff_t *pos)
{
    int dev_no = (int)*pos;
    bksync_dev_t *dev_info = NULL;

    if (dev_no < ptp_priv->max_dev) {
        dev_info = &ptp_priv->dev_info[dev_no];
    } else {
        /* End of sequence */
        return NULL;
    }

    if (dev_info == NULL) {
        /* Init not done */
        return NULL;
    }

    /* beginning a new sequence */
    if (dev_info->dev_no == 0) {
        seq_printf(s, "Port PTP statistics\n");
    }

    seq_printf(s, "dev_no : %d\n", dev_info->dev_no);
    seq_printf(s, "     TwoStep Port Bitmap : %08llx%08llx\n",
            (uint64_t)(dev_info->two_step.portmap[1]),
            (uint64_t)(dev_info->two_step.portmap[0]));
    seq_printf(s,"      %4s| %9s| %9s| %9s| %9s| %9s| %9s| %9s| %9s| %9s| %9s\n",
            "Port", "RxCounter", "TxCounter", "TxOneStep", "TSRead", "TSMatch", "TSDiscard",
            "TimeHi" , "TimeLo", "TimeAvg", "FIFORx");

    return (void *) dev_info;
}

/**
* This function is called after the beginning of a sequence.
* It's called untill the return is NULL (this ends the sequence).
*
*/
static void *bksync_proc_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    (*pos)++;
    return bksync_proc_seq_start(s, pos);
}
/**
* This function is called at the end of a sequence
*
*/
static void bksync_proc_seq_stop(struct seq_file *s, void *v)
{
    /* nothing to do, we use a static value in bksync_proc_seq_start() */
}

/**
* This function is called for each "step" of a sequence
*
*/
static int bksync_proc_seq_show(struct seq_file *s, void *v)
{
    bksync_dev_t *dev_info = (bksync_dev_t *)v;
    unsigned long port = 0;

    if (dev_info != NULL) {
        for (port = 0; port < dev_info->num_phys_ports; port++) {
            if (dev_info->port_stats[port].pkt_rxctr || dev_info->port_stats[port].pkt_txctr ||
                    dev_info->port_stats[port].pkt_txonestep||
                    dev_info->port_stats[port].tsts_discard || dev_info->port_stats[port].tsts_timeout ||
                    dev_info->port_stats[port].tsts_match) {

                seq_printf(s, "    %4lu | %9d| %9d| %9d| %9d| %9d| %9d| %9lld| %9lld | %9d|%9d | %s\n", (port + 1),
                        dev_info->port_stats[port].pkt_rxctr,
                        dev_info->port_stats[port].pkt_txctr,
                        dev_info->port_stats[port].pkt_txonestep,
                        dev_info->port_stats[port].tsts_timeout,
                        dev_info->port_stats[port].tsts_match,
                        dev_info->port_stats[port].tsts_discard,
                        dev_info->port_stats[port].tsts_worst_fetch_time,
                        dev_info->port_stats[port].tsts_best_fetch_time,
                        dev_info->port_stats[port].tsts_avg_fetch_time,
                        dev_info->port_stats[port].fifo_rxctr,
                        dev_info->port_stats[port].pkt_txctr != dev_info->port_stats[port].tsts_match ? "***":"");
            }
        }
    }
    return 0;
}

/**
* seq_operations for bsync_proc_*** entries
*
*/
static struct seq_operations bksync_proc_seq_ops = {
    .start = bksync_proc_seq_start,
    .next  = bksync_proc_seq_next,
    .stop  = bksync_proc_seq_stop,
    .show  = bksync_proc_seq_show
};

static int bksync_proc_txts_open(struct inode * inode, struct file * file)
{
    return seq_open(file, &bksync_proc_seq_ops);
}

static ssize_t
bksync_proc_txts_write(struct file *file, const char *buf,
                      size_t count, loff_t *loff)
{
    char debug_str[40];
    char *ptr;
    int port;
    int dev_no;
    bksync_dev_t *dev_info;

    if (copy_from_user(debug_str, buf, count)) {
        return -EFAULT;
    }

    if ((ptr = strstr(debug_str, "clear")) != NULL) {

        for (dev_no = 0; dev_no < ptp_priv->max_dev; dev_no++) {
            dev_info = &ptp_priv->dev_info[dev_no];

            for (port = 0; port < dev_info->num_phys_ports; port++) {
                dev_info->port_stats[port].pkt_rxctr = 0;
                dev_info->port_stats[port].pkt_txctr = 0;
                dev_info->port_stats[port].pkt_txonestep = 0;
                dev_info->port_stats[port].tsts_timeout = 0;
                dev_info->port_stats[port].tsts_match = 0;
                dev_info->port_stats[port].tsts_discard = 0;
            }
        }
    } else {
        DBG_ERR(("Warning: unknown input\n"));
    }

    return count;
}

struct proc_ops bksync_proc_txts_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =       bksync_proc_txts_open,
    .proc_read =       seq_read,
    .proc_lseek =      seq_lseek,
    .proc_write =      bksync_proc_txts_write,
    .proc_release =    seq_release,
};

/*
 * Driver Debug Proc Entry
 */
static int
bksync_proc_debug_show(struct seq_file *m, void *v)
{
    seq_printf(m, "Configuration:\n");
    seq_printf(m, "  debug:          0x%x\n", debug);
    return 0;
}

static ssize_t
bksync_proc_debug_write(struct file *file, const char *buf,
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

static int bksync_proc_debug_open(struct inode * inode, struct file * file)
{
    return single_open(file, bksync_proc_debug_show, NULL);
}

struct proc_ops bksync_proc_debug_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =       bksync_proc_debug_open,
    .proc_read =       seq_read,
    .proc_lseek =      seq_lseek,
    .proc_write =      bksync_proc_debug_write,
    .proc_release =    single_release,
};

/*
 * Device information Proc Entry
 */
/**
* This function is called at the beginning of a sequence.
* ie, when:
*    - the /proc/bcm/ksync/dev_info file is read (first time)
*   - after the function stop (end of sequence)
*
*/
static void *bksync_proc_dev_info_seq_start(struct seq_file *s, loff_t *pos)
{
    int dev_no = (int)*pos;
    bksync_dev_t *dev_info = NULL;

    if (dev_no < ptp_priv->max_dev) {
        dev_info = &ptp_priv->dev_info[dev_no];
    } else {
        /* End of sequence */
        return NULL;
    }

    /* Beginning a new sequence */
    if (dev_info->dev_no == 0) {
        seq_printf(s, "Device information:\n");
    }

    return (void *) dev_info;
}

/**
* This function is called after the beginning of a sequence.
* It's called untill the return is NULL (this ends the sequence).
*
*/
static void *bksync_proc_dev_info_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
    (*pos)++;
    return bksync_proc_dev_info_seq_start(s, pos);
}
/**
* This function is called at the end of a sequence
*
*/
static void bksync_proc_dev_info_seq_stop(struct seq_file *s, void *v)
{
    /* nothing to do, we use a static value in bksync_proc_seq_start() */
    seq_printf(s, "\nShared PHC:      %s\n", shared_phc ? "Yes" : "No");
    seq_printf(s, "Master Dev:      %d\n", master_core);
}

/**
* This function is called for each "step" of a sequence
*
*/
static int bksync_proc_dev_info_seq_show(struct seq_file *s, void *v)
{
    bksync_dev_t *dev_info = (bksync_dev_t *)v;

    if (dev_info != NULL) {
        seq_printf(s, "  dev_no:          %d\n", dev_info->dev_no);
        seq_printf(s, "     dev_id:          0x%x\n", dev_info->dev_id);
        seq_printf(s, "     dev_init:        %d\n", dev_info->dev_init);
        seq_printf(s, "     dev_core:        %d\n", dev_info->max_core);
        seq_printf(s, "     phc_index:       /dev/ptp%d\n", bksync_ptp_hw_tstamp_ptp_clock_index_get(dev_info->dev_no));
    }
    return 0;
}

/**
* seq_operations for bsync_proc_*** entries
*
*/
static struct seq_operations bksync_proc_dev_info_seq_ops = {
    .start = bksync_proc_dev_info_seq_start,
    .next  = bksync_proc_dev_info_seq_next,
    .stop  = bksync_proc_dev_info_seq_stop,
    .show  = bksync_proc_dev_info_seq_show
};

static int bksync_proc_dev_info_open(struct inode * inode, struct file * file)
{
    return seq_open(file, &bksync_proc_dev_info_seq_ops);
}

static ssize_t
bksync_proc_dev_info_write(struct file *file, const char *buf,
                      size_t count, loff_t *loff)
{
    return 0;
}

struct proc_ops bksync_proc_dev_info_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =       bksync_proc_dev_info_open,
    .proc_read =       seq_read,
    .proc_lseek =      seq_lseek,
    .proc_write =      bksync_proc_dev_info_write,
    .proc_release =    seq_release,
};

static int
bksync_proc_init(void)
{
    struct proc_dir_entry *entry;

    PROC_CREATE(entry, "stats", 0666, bksync_proc_root, &bksync_proc_txts_file_ops);
    if (entry == NULL) {
        return -1;
    }
    PROC_CREATE(entry, "debug", 0666, bksync_proc_root, &bksync_proc_debug_file_ops);
    if (entry == NULL) {
        return -1;
    }
    PROC_CREATE(entry, "dev_info", 0666, bksync_proc_root, &bksync_proc_dev_info_file_ops);
    if (entry == NULL) {
        return -1;
    }
    return 0;
}

static int
bksync_proc_cleanup(void)
{
    remove_proc_entry("stats", bksync_proc_root);
    remove_proc_entry("debug", bksync_proc_root);
    remove_proc_entry("dev_info", bksync_proc_root);
    remove_proc_entry("bcm/ksync", NULL);
    return 0;
}


#define ATTRCMP(x) (0 == strcmp(attr->attr.name, #x))

static int rd_iter=0, wr_iter=0;
static ssize_t bs_attr_store(struct kobject *kobj,
                             struct kobj_attribute *attr,
                             const char *buf,
                             size_t bytes)
{
    ssize_t ret;
    u32 enable, mode;
    u32 bc, hb;
    int bs_id = -1;
    int dev_no = -1;
    bksync_dev_t *dev_info = NULL;
    bksync_time_spec_t offset = {0};

    if (ATTRCMP(bs0)) {
        bs_id = 0;
        dev_no = 0;
    } else if (ATTRCMP(bs1)) {
        bs_id = 1;
        dev_no = 0;
    } else if (ATTRCMP(bs2)) {
        bs_id = 0;
        dev_no = 1;
    } else if (ATTRCMP(bs3)) {
        bs_id = 1;
        dev_no = 1;
    } else {
        return -ENOENT;
    }

    dev_info = &ptp_priv->dev_info[dev_no];

    ret = sscanf(buf, "enable:%d mode:%d bc:%u hb:%u sign:%d offset:%llu.%u", &enable, &mode, &bc, &hb, &offset.sign, &offset.sec, &offset.nsec);
    DBG_VERB(("rd:%d bs0: enable:%d mode:%d bc:%d hb:%d sign:%d offset:%llu.%u\n", rd_iter++, enable, mode, bc, hb, offset.sign, offset.sec, offset.nsec));

    dev_info->bksync_bs_info[bs_id].enable = enable;
    dev_info->bksync_bs_info[bs_id].mode = mode;
    dev_info->bksync_bs_info[bs_id].bc = bc;
    dev_info->bksync_bs_info[bs_id].hb = hb;

    (void)bksync_broadsync_cmd(dev_info, bs_id);

    (void)bksync_broadsync_phase_offset_cmd(dev_info, bs_id, offset);

    return (ret == -ENOENT) ? ret : bytes;
}

static ssize_t bs_attr_show(struct kobject *kobj,
                            struct kobj_attribute *attr,
                            char *buf)
{
    ssize_t bytes;
    u64 status = 0;
    u32 variance = 0;
    int bs_id = -1;
    int dev_no = -1;
    bksync_dev_t *dev_info = NULL;

    if (ATTRCMP(bs0)) {
        bs_id = 0;
        dev_no = 0;
    } else if (ATTRCMP(bs1)) {
        bs_id = 1;
        dev_no = 0;
    } else if (ATTRCMP(bs2)) {
        bs_id = 0;
        dev_no = 1;
    } else if (ATTRCMP(bs3)) {
        bs_id = 1;
        dev_no = 1;
    } else {
        return -ENOENT;
    }

    dev_info = &ptp_priv->dev_info[dev_no];

    if(dev_info->bksync_bs_info[bs_id].enable) {
        (void)bksync_broadsync_status_cmd(dev_info, bs_id, &status);
    }

    variance = (status >> 32);
    status = (status & 0xFFFFFFFF);
    bytes = sprintf(buf, "enable:%d mode:%d bc:%u hb:%u sign:%d offset:%llu.%u status:%u(%u)\n",
            dev_info->bksync_bs_info[bs_id].enable,
            dev_info->bksync_bs_info[bs_id].mode,
            dev_info->bksync_bs_info[bs_id].bc,
            dev_info->bksync_bs_info[bs_id].hb,
            dev_info->bksync_bs_info[bs_id].offset.sign,
            dev_info->bksync_bs_info[bs_id].offset.sec,
            dev_info->bksync_bs_info[bs_id].offset.nsec,
            (u32)status,
            variance);
    DBG_VERB(("wr:%d bs1: enable:%d mode:%d bc:%u hb:%u sign:%d offset:%llu.%u status:%u(%u)\n",
                wr_iter++,
                dev_info->bksync_bs_info[bs_id].enable,
                dev_info->bksync_bs_info[bs_id].mode,
                dev_info->bksync_bs_info[bs_id].bc,
                dev_info->bksync_bs_info[bs_id].hb,
                dev_info->bksync_bs_info[bs_id].offset.sign,
                dev_info->bksync_bs_info[bs_id].offset.sec,
                dev_info->bksync_bs_info[bs_id].offset.nsec,
                (u32)status,
                variance));

    return bytes;
}

#define BS_ATTR(x)                         \
    static struct kobj_attribute x##_attribute =        \
        __ATTR(x, 0664, bs_attr_show, bs_attr_store);

BS_ATTR(bs0)
BS_ATTR(bs1)
BS_ATTR(bs2)
BS_ATTR(bs3)

#define BS_ATTR_LIST(x)    & x ## _attribute.attr
static struct attribute *bs_attrs[] = {
    BS_ATTR_LIST(bs0),
    BS_ATTR_LIST(bs1),
    BS_ATTR_LIST(bs2),
    BS_ATTR_LIST(bs3),
    NULL,       /* terminator */
};

static struct attribute_group bs_attr_group = {
    .name   = "broadsync",
    .attrs  = bs_attrs,
};


static int gpio_rd_iter=0, gpio_wr_iter=0;
static ssize_t gpio_attr_store(struct kobject *kobj,
                               struct kobj_attribute *attr,
                               const char *buf,
                               size_t bytes)
{
    ssize_t ret;
    int gpio;
    u32 enable, mode;
    u32 period;
    int64_t phaseoffset;
    int dev_no = -1;
    bksync_dev_t *dev_info = NULL;

    if (ATTRCMP(gpio0)) {
        gpio = 0;
        dev_no = 0;
    } else if (ATTRCMP(gpio1)) {
        gpio = 1;
        dev_no = 0;
    } else if (ATTRCMP(gpio2)) {
        gpio = 2;
        dev_no = 0;
    } else if (ATTRCMP(gpio3)) {
        gpio = 3;
        dev_no = 0;
    } else if (ATTRCMP(gpio4)) {
        gpio = 4;
        dev_no = 0;
    } else if (ATTRCMP(gpio5)) {
        gpio = 5;
        dev_no = 0;
    } else if (ATTRCMP(gpio6)) {
        gpio = 0;
        dev_no = 1;
    } else if (ATTRCMP(gpio7)) {
        gpio = 1;
        dev_no = 1;
    } else if (ATTRCMP(gpio8)) {
        gpio = 2;
        dev_no = 1;
    } else if (ATTRCMP(gpio9)) {
        gpio = 3;
        dev_no = 1;
    } else if (ATTRCMP(gpio10)) {
        gpio = 4;
        dev_no = 1;
    } else if (ATTRCMP(gpio11)) {
        gpio = 5;
        dev_no = 1;
    } else {
        return -ENOENT;
    }


    ret = sscanf(buf, "enable:%d mode:%d period:%u phaseoffset:%lld", &enable, &mode, &period, &phaseoffset);
    DBG_VERB(("rd:%d gpio%d: enable:%d mode:%d period:%d phaseoffset:%lld\n", gpio_rd_iter++, gpio, enable, mode, period, phaseoffset));

    dev_info = &ptp_priv->dev_info[dev_no];
    dev_info->bksync_gpio_info[gpio].enable = enable;
    dev_info->bksync_gpio_info[gpio].mode = mode;
    dev_info->bksync_gpio_info[gpio].period   = period;

   (void)bksync_gpio_cmd(dev_info, gpio);

   if (dev_info->bksync_gpio_info[gpio].phaseoffset != phaseoffset) {
       dev_info->bksync_gpio_info[gpio].phaseoffset = phaseoffset;
       (void)bksync_gpio_phaseoffset_cmd(dev_info, gpio);
   }

    return (ret == -ENOENT) ? ret : bytes;
}

static ssize_t gpio_attr_show(struct kobject *kobj,
                              struct kobj_attribute *attr,
                              char *buf)
{
    ssize_t bytes;
    int gpio;
    int dev_no;
    bksync_dev_t *dev_info = NULL;

    if (ATTRCMP(gpio0)) {
        gpio = 0;
        dev_no = 0;
    } else if (ATTRCMP(gpio1)) {
        gpio = 1;
        dev_no = 0;
    } else if (ATTRCMP(gpio2)) {
        gpio = 2;
        dev_no = 0;
    } else if (ATTRCMP(gpio3)) {
        gpio = 3;
        dev_no = 0;
    } else if (ATTRCMP(gpio4)) {
        gpio = 4;
        dev_no = 0;
    } else if (ATTRCMP(gpio5)) {
        gpio = 5;
        dev_no = 0;
    } else if (ATTRCMP(gpio6)) {
        gpio = 0;
        dev_no = 1;
    } else if (ATTRCMP(gpio7)) {
        gpio = 1;
        dev_no = 1;
    } else if (ATTRCMP(gpio8)) {
        gpio = 2;
        dev_no = 1;
    } else if (ATTRCMP(gpio9)) {
        gpio = 3;
        dev_no = 1;
    } else if (ATTRCMP(gpio10)) {
        gpio = 4;
        dev_no = 1;
    } else if (ATTRCMP(gpio11)) {
        gpio = 5;
        dev_no = 1;
    } else {
        return -ENOENT;
    }

    dev_info = &ptp_priv->dev_info[dev_no];
    bytes = sprintf(buf, "enable:%d mode:%d period:%u phaseoffset:%lld\n",
                    dev_info->bksync_gpio_info[gpio].enable,
                    dev_info->bksync_gpio_info[gpio].mode,
                    dev_info->bksync_gpio_info[gpio].period,
                    dev_info->bksync_gpio_info[gpio].phaseoffset);
    DBG_VERB(("wr:%d gpio%d: enable:%d mode:%d period:%u phaseoffset:%lld\n",
                    gpio_wr_iter++, gpio,
                    dev_info->bksync_gpio_info[gpio].enable,
                    dev_info->bksync_gpio_info[gpio].mode,
                    dev_info->bksync_gpio_info[gpio].period,
                    dev_info->bksync_gpio_info[gpio].phaseoffset));

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
GPIO_ATTR(gpio6)
GPIO_ATTR(gpio7)
GPIO_ATTR(gpio8)
GPIO_ATTR(gpio9)
GPIO_ATTR(gpio10)
GPIO_ATTR(gpio11)

#define GPIO_ATTR_LIST(x)    & x ## _attribute.attr
static struct attribute *gpio_attrs[] = {
    GPIO_ATTR_LIST(gpio0),
    GPIO_ATTR_LIST(gpio1),
    GPIO_ATTR_LIST(gpio2),
    GPIO_ATTR_LIST(gpio3),
    GPIO_ATTR_LIST(gpio4),
    GPIO_ATTR_LIST(gpio5),
    GPIO_ATTR_LIST(gpio6),
    GPIO_ATTR_LIST(gpio7),
    GPIO_ATTR_LIST(gpio8),
    GPIO_ATTR_LIST(gpio9),
    GPIO_ATTR_LIST(gpio10),
    GPIO_ATTR_LIST(gpio11),
    NULL,       /* terminator */
};

static struct attribute_group gpio_attr_group = {
    .name   = "gpio",
    .attrs  = gpio_attrs,
};

#ifdef BDE_EDK_SUPPORT
static ssize_t ptp_tod_attr_store(struct kobject *kobj,
                                   struct kobj_attribute *attr,
                                   const char *buf,
                                   size_t bytes)
{
    ssize_t ret;
    int dev_no = -1;
    bksync_dev_t *dev_info = NULL;
    uint64_t offset_sec = 0;
    uint32_t offset_nsec = 0;
    int sign = 0;

    ret = sscanf(buf, "sign:%d offset_sec:%llu offset_ns:%u", &sign, &offset_sec, &offset_nsec);

    offset_sec = (offset_sec & 0x7FFFFFFFFFFF);
    offset_nsec = (offset_nsec & 0x3FFFFFFF);

    dev_no = master_core; /* All ToD control should be sent to master_core in case of multidie device */
    dev_info = &ptp_priv->dev_info[dev_no];

    if (!dev_info->dev_init) {
        return -ENOENT;
    }

    dev_info->ptp_tod.offset.sign = sign;
    dev_info->ptp_tod.offset.sec = offset_sec;
    dev_info->ptp_tod.offset.nsec = offset_nsec;

    (void)bksync_ptp_tod_cmd(dev_info, sign, offset_sec, offset_nsec);

    DBG_VERB(("sign:%d offset_sec:%llu offset_nsec:%u\n", sign, offset_sec, offset_nsec));

    return (ret == -ENOENT) ? ret : bytes;
}

static ssize_t ptp_tod_attr_show(struct kobject *kobj,
                              struct kobj_attribute *attr,
                              char *buf)
{
    ssize_t bytes;
    int dev_no = -1;
    bksync_dev_t *dev_info = NULL;
    fw_tstamp_t ptp_tod_time;

    dev_no = master_core; /* All ToD control should be sent to master_core in case of multidie device */
    dev_info = &ptp_priv->dev_info[dev_no];

    if (!dev_info->dev_init) {
        return -ENOENT;
    }

    (void)bksync_ptp_tod_get_cmd(dev_info, &ptp_tod_time);

    bytes = sprintf(buf, "sign:%d offset_sec:%llu offset_nsec:%u ptp_tod:%llusec:%unsec\n",
                    dev_info->ptp_tod.offset.sign,
                    dev_info->ptp_tod.offset.sec,
                    dev_info->ptp_tod.offset.nsec,
                    ptp_tod_time.sec, ptp_tod_time.nsec);

    DBG_VERB(("sign:%d offset_sec:%llu offset_nsec:%u ptp_tod:%llusec:%unsec\n",
                    dev_info->ptp_tod.offset.sign,
                    dev_info->ptp_tod.offset.sec,
                    dev_info->ptp_tod.offset.nsec,
                    ptp_tod_time.sec, ptp_tod_time.nsec));

    return bytes;
}

static struct kobj_attribute ptp_tod_attr =
                    __ATTR(ptp_tod, 0664, ptp_tod_attr_show, ptp_tod_attr_store);

static ssize_t ntp_tod_attr_store(struct kobject *kobj,
                                   struct kobj_attribute *attr,
                                   const char *buf,
                                   size_t bytes)
{
    ssize_t ret;
    int dev_no = -1;
    bksync_dev_t *dev_info = NULL;
    uint64_t epoch_offset = 0;
    uint32_t leap_sec_ctrl_en = 0;
    uint32_t leap_sec_op = 0;

    ret = sscanf(buf, "leap_sec_ctrl_en:%u leap_sec_op:%u epoch_offset:%llu", &leap_sec_ctrl_en, &leap_sec_op, &epoch_offset);

    dev_no = master_core; /* All ToD control should be sent to master_core in case of multidie device */
    dev_info = &ptp_priv->dev_info[dev_no];

    if (!dev_info->dev_init) {
        return -ENOENT;
    }

    dev_info->ntp_tod.leap_sec_ctrl_en = (uint8_t)leap_sec_ctrl_en;
    dev_info->ntp_tod.leap_sec_op = (uint8_t)leap_sec_op;

    if (!leap_sec_ctrl_en) {
        /* Either leap sec operation or offset can be set */
        dev_info->ntp_tod.epoch_offset = epoch_offset;
    }

    (void)bksync_ntp_tod_cmd(dev_info, (uint8_t)leap_sec_ctrl_en, (uint8_t)leap_sec_op, epoch_offset);
    DBG_VERB(("leap_sec_ctrl_en:%u leap_sec_op:%u epoch_offset:%llu\n", leap_sec_ctrl_en, leap_sec_op, epoch_offset));

    return (ret == -ENOENT) ? ret : bytes;
}

static ssize_t ntp_tod_attr_show(struct kobject *kobj,
                              struct kobj_attribute *attr,
                              char *buf)
{
    ssize_t bytes;
    int dev_no = 0;
    bksync_dev_t *dev_info = NULL;
    fw_tstamp_t ntp_tod_time;

    dev_no = master_core; /* All ToD control should be sent to master_core in case of multidie device */
    dev_info = &ptp_priv->dev_info[dev_no];

    if (!dev_info->dev_init) {
        return -ENOENT;
    }

    (void)bksync_ntp_tod_get_cmd(dev_info, &ntp_tod_time);

    bytes = sprintf(buf, "leap_sec_ctrl_en:%u leap_sec_op:%u epoch_offset:%llu ntp_tod:%llusec:%unsec\n",
                    (uint32_t)dev_info->ntp_tod.leap_sec_ctrl_en,
                    (uint32_t)dev_info->ntp_tod.leap_sec_op,
                    dev_info->ntp_tod.epoch_offset,
                    ntp_tod_time.sec, ntp_tod_time.nsec);

    DBG_VERB(("leap_sec_ctrl_en:%u leap_sec_op:%u epoch_offset:%llu ntp_tod:%llusec:%unsec\n",
                    (uint32_t)dev_info->ntp_tod.leap_sec_ctrl_en,
                    (uint32_t)dev_info->ntp_tod.leap_sec_op,
                    dev_info->ntp_tod.epoch_offset,
                    ntp_tod_time.sec, ntp_tod_time.nsec));

    return bytes;
}

static struct kobj_attribute ntp_tod_attr =
                    __ATTR(ntp_tod, 0664, ntp_tod_attr_show, ntp_tod_attr_store);
#endif

#ifndef BDE_EDK_SUPPORT
/* Event logging is replaced with EXTTS logging */
static ssize_t evlog_attr_store(struct kobject *kobj,
                                struct kobj_attribute *attr,
                                const char *buf,
                                size_t bytes)
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

    (void)bksync_evlog_cmd(event, enable);
    ptp_priv->dev_info[0].evlog_info[event].enable = enable;

    return (ret == -ENOENT) ? ret : bytes;
}

static ssize_t evlog_attr_show(struct kobject *kobj,
                               struct kobj_attribute *attr,
                               char *buf)
{
    ssize_t bytes;
    int event;
    int dev_no = master_core;
    bksync_dev_t *dev_info = &ptp_priv->dev_info[dev_no];

    if ((!dev_info->dev_init) || (dev_info->evlog == NULL)) {
        return -ENOENT;
    }

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
                    dev_info->evlog_info[event].enable,
                    dev_info->evlog->event_timestamps[event].prv_tstamp.sec,
                    dev_info->evlog->event_timestamps[event].prv_tstamp.nsec,
                    dev_info->evlog->event_timestamps[event].cur_tstamp.sec,
                    dev_info->evlog->event_timestamps[event].cur_tstamp.nsec);
    DBG_VERB(("event%d: enable:%d Previous Time:%llu.%09u Latest Time:%llu.%09u\n",
                    event,
                    dev_info->evlog_info[event].enable,
                    dev_info->evlog->event_timestamps[event].prv_tstamp.sec,
                    dev_info->evlog->event_timestamps[event].prv_tstamp.nsec,
                    dev_info->evlog->event_timestamps[event].cur_tstamp.sec,
                    dev_info->evlog->event_timestamps[event].cur_tstamp.nsec));

    memset((void *)&(dev_info->evlog->event_timestamps[event]), 0, sizeof(dev_info->evlog->event_timestamps[event]));

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
#endif

static int
bksync_sysfs_init(void)
{
    int ret = 0;
    struct kobject *parent;
    struct kobject *root = &((((struct module *)(THIS_MODULE))->mkobj).kobj);

    parent = root;
    ptp_priv->kobj = kobject_create_and_add("io", parent);

    ret = sysfs_create_group(ptp_priv->kobj, &bs_attr_group);

    ret |= sysfs_create_group(ptp_priv->kobj, &gpio_attr_group);

#ifdef BDE_EDK_SUPPORT
    ret |= sysfs_create_file(ptp_priv->kobj, &ptp_tod_attr.attr);

    ret |= sysfs_create_file(ptp_priv->kobj, &ntp_tod_attr.attr);
#endif

#ifndef BDE_EDK_SUPPORT
    ret |= sysfs_create_group(ptp_priv->kobj, &evlog_attr_group);
#endif

    return ret;
}

static int
bksync_sysfs_cleanup(void)
{
    int ret = 0;
    struct kobject *parent;

    parent = ptp_priv->kobj;

    sysfs_remove_group(parent, &bs_attr_group);
    sysfs_remove_group(parent, &gpio_attr_group);

#ifdef BDE_EDK_SUPPORT
    sysfs_remove_file(parent, &ptp_tod_attr.attr);
    sysfs_remove_file(parent, &ntp_tod_attr.attr);
#endif

#ifndef BDE_EDK_SUPPORT
    sysfs_remove_group(parent, &evlog_attr_group);
#endif

    kobject_put(ptp_priv->kobj);

    return ret;
}

static void bksync_ptp_fw_data_alloc(int dev_no)
{
    bksync_dev_t *dev_info = &ptp_priv->dev_info[dev_no];
#ifndef BDE_EDK_SUPPORT
    dma_addr_t dma_mem = 0;
#endif

    /* Initialize the Base address for CMIC and shared Memory access */
    dev_info->base_addr = lkbde_get_dev_virt(dev_no);
    dev_info->dma_dev = lkbde_get_dma_dev(dev_no);

#ifndef BDE_EDK_SUPPORT
    dev_info->evlog_dma_mem_size = sizeof(bksync_evlog_t); /*sizeof(bksync_evlog_t);*/

    if (dev_info->evlog == NULL) {
        DBG_ERR(("Allocate memory for event log\n"));
        dev_info->evlog = DMA_ALLOC_COHERENT(dev_info->dma_dev,
                                                   dev_info->evlog_dma_mem_size,
                                                   &dma_mem);
        if (dev_info->evlog != NULL) {
            dev_info->dma_mem = dma_mem;
        }
    }

    if (dev_info->evlog != NULL) {
        /* Reset memory */
        memset((void *)dev_info->evlog, 0, dev_info->evlog_dma_mem_size);

        DBG_ERR(("Shared memory allocation (%d bytes) for event log successful at 0x%016lx.\n",
                dev_info->evlog_dma_mem_size, (long unsigned int)dev_info->dma_mem));
    }

    /* Allocate dma for timestmap logging for extts */
    dma_mem = 0;
    dev_info->extts_dma_mem_size = sizeof(bksync_fw_extts_log_t);
    if (dev_info->extts_log == NULL) {
        DBG_ERR(("Allocate memory for extts log\n"));
        dev_info->extts_log = DMA_ALLOC_COHERENT(dev_info->dma_dev,
                                                   dev_info->extts_dma_mem_size,
                                                   &dma_mem);
        if (dev_info->extts_log != NULL) {
            dev_info->extts_dma_mem_addr = dma_mem;
        }
    }

    if (dev_info->extts_log != NULL) {
        /* Reset memory */
        memset((void *)dev_info->extts_log, 0, dev_info->extts_dma_mem_size);
        dev_info->extts_log->tail = 0;
        dev_info->extts_event.head = -1;
        dev_info->extts_log->head = -1;

        DBG_ERR(("Shared memory allocation (%d bytes) for extts log successful at 0x%016lx.\n",
                dev_info->extts_dma_mem_size, (long unsigned int)dev_info->extts_dma_mem_addr));
    }
#endif
    return;
}

static void bksync_ptp_fw_data_free(void)
{
    int dev_no = 0;
    bksync_dev_t *dev_info = NULL;

    for (dev_no = 0; dev_no < ptp_priv->max_dev; dev_no++) { 
        dev_info = &ptp_priv->dev_info[dev_no];

        if (dev_info == NULL) {
            continue;
        }

#ifdef BDE_EDK_SUPPORT
        /* Do nothing */
#else
        if (dev_info->evlog != NULL) {
            DMA_FREE_COHERENT(dev_info->dma_dev, dev_info->evlog_dma_mem_size,
                    (void *)dev_info->evlog, dev_info->dma_mem);
            dev_info->evlog = NULL;
        }

        if (dev_info->extts_log != NULL) {
            DBG_ERR(("Free shared memory : extts log of %d bytes\n", dev_info->extts_dma_mem_size));
            DMA_FREE_COHERENT(dev_info->dma_dev, dev_info->extts_dma_mem_size,
                    (void *)dev_info->extts_log, dev_info->extts_dma_mem_addr);
            dev_info->extts_log = NULL;
        }
#endif
    }
    return;
}

static void bksync_ptp_dma_init(bksync_dev_t *dev_info, int dcb_type)
{
    int endianess;

    dev_info->num_phys_ports = BKSYNC_MAX_NUM_PORTS;

    dev_info->port_stats = kzalloc((sizeof(bksync_port_stats_t) * (dev_info->num_phys_ports)), GFP_KERNEL);
    if (dev_info->port_stats == NULL) {
        DBG_ERR(("bksync_ptp_dma_init: port_stats memory allocation failed\n"));
    }

#ifdef __LITTLE_ENDIAN
        endianess = 0;
#else
        endianess = 1;
#endif

#ifdef BDE_EDK_SUPPORT
        /* Do nothing */
        (void)endianess;
#else
        DEV_WRITE32(dev_info, CMIC_CMC_SCHAN_MESSAGE_14r(CMIC_CMC_BASE), ((pci_cos << 16) | endianess));

        DEV_WRITE32(dev_info, CMIC_CMC_SCHAN_MESSAGE_15r(CMIC_CMC_BASE), 1);
        DEV_WRITE32(dev_info, CMIC_CMC_SCHAN_MESSAGE_16r(CMIC_CMC_BASE), 1);
#endif

    bksync_ptp_fw_data_alloc(dev_info->dev_no);

    DBG_VERB(("%s %p dcb_type: %d\n", __FUNCTION__, dev_info->base_addr, dcb_type));

    ptp_priv->mirror_encap_bmp = 0x0;

#ifdef BDE_EDK_SUPPORT
    /* Do Nothing */
#else
    hostcmd_regs[0] = CMIC_CMC_SCHAN_MESSAGE_21r(CMIC_CMC_BASE);
    hostcmd_regs[1] = CMIC_CMC_SCHAN_MESSAGE_20r(CMIC_CMC_BASE);
    hostcmd_regs[2] = CMIC_CMC_SCHAN_MESSAGE_19r(CMIC_CMC_BASE);
    hostcmd_regs[3] = CMIC_CMC_SCHAN_MESSAGE_18r(CMIC_CMC_BASE);
    hostcmd_regs[4] = CMIC_CMC_SCHAN_MESSAGE_17r(CMIC_CMC_BASE);
#endif

    return;
}


/**
 * bksync_ioctl_cmd_handler
 * @kmsg: kcom message - ptp clock ioctl command.
 * Description: This function will handle ioctl commands
 * from user mode.
 */
static int
bksync_ioctl_cmd_handler(kcom_msg_clock_cmd_t *kmsg, int len, int dcb_type, int dev_no)
{
    u32 fw_status;
    bksync_dnx_jr2_header_info_t *header_data = NULL;
    bksync_dev_t *dev_info = NULL;
    bksync_time_spec_t bs_offset;
    int tmp = 0;
#ifdef BDE_EDK_SUPPORT
    uint64_t paddr = 0;
    fw_tstamp_t tod_time;
    int rv = 0;
    sal_vaddr_t vaddr = 0;
#endif
    int bs_id = -1;
    int gpio;
    u64 status = 0;

    dev_info = &ptp_priv->dev_info[dev_no];

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    if (!dev_info->dev_init && kmsg->clock_info.cmd != KSYNC_M_HW_INIT) {
        kmsg->hdr.status = KCOM_E_NOT_FOUND;
        return sizeof(kcom_msg_hdr_t);
    }

    if (!dev_info) {
        kmsg->hdr.status = KCOM_E_NOT_FOUND;
        DBG_ERR(("Device not found %d\n", dev_no));
        return sizeof(kcom_msg_hdr_t);
    }

    switch(kmsg->clock_info.cmd) {
        case KSYNC_M_HW_INIT:
            pci_cos = kmsg->clock_info.data[0];
            fw_core = kmsg->clock_info.data[1];
            DBG_VERB(("Configuring pci_cosq:%d fw_core:%d\n", pci_cos, fw_core));
            if (fw_core >= 0 || fw_core <= dev_info->max_core) {

                /* Return success if the app is already initialized. */
                if (dev_info->dev_init) {
                    kmsg->hdr.status = KCOM_E_NONE;
                    return sizeof(kcom_msg_hdr_t);
                }

#ifdef BDE_EDK_SUPPORT
                dev_info->fw_comm = NULL;
                paddr = ((uint64_t)((uint32_t)kmsg->clock_info.data[7])) << 32;
                paddr |= ((uint32_t)kmsg->clock_info.data[8]);
                DBG_VERB((" HW_init: phy_addr:0x%llx \n", paddr));
                rv = lkbde_get_phys_to_virt(dev_no, (phys_addr_t)paddr, &vaddr);
                if ((rv != 0) || (vaddr == 0)) {
                    DBG_ERR((" Address conversion failed. rv=%d\n", rv));
                    kmsg->hdr.status = KCOM_E_RESOURCE;
                    return sizeof(kcom_msg_hdr_t);
                }
                dev_info->fw_comm = (bksync_fw_comm_t *)vaddr;
                DBG_VERB((" HW_init: virt_addr:%p:0x%llx\n", dev_info->fw_comm, (uint64_t)vaddr));
#endif
                dev_info->dcb_type = dcb_type;
                bksync_ptp_dma_init(dev_info, dcb_type);

#ifdef BDE_EDK_SUPPORT
                /* Data from FW, hence don't memset fw_comm after address converstion */
                fw_status = dev_info->fw_comm->cmd;
#else
                DEV_READ32(dev_info, CMIC_CMC_SCHAN_MESSAGE_21r(CMIC_CMC_BASE), &fw_status);
#endif
                /* Return error if the app is not ready yet. */
                if (fw_status != 0xBADC0DE1) {
                    kmsg->hdr.status = KCOM_E_RESOURCE;
                    return sizeof(kcom_msg_hdr_t);
                }

                (dev_info->init_data).uc_port_num = kmsg->clock_info.data[2];
                (dev_info->init_data).uc_port_sysport = kmsg->clock_info.data[3];
                (dev_info->init_data).host_cpu_port = kmsg->clock_info.data[4];
                (dev_info->init_data).host_cpu_sysport = kmsg->clock_info.data[5];
                (dev_info->init_data).udh_len = kmsg->clock_info.data[6];
                (dev_info->init_data).application_v2 = kmsg->clock_info.data[9];

                DBG_VERB(("fw_core:%d uc_port:%d uc_sysport:%d pci_port:%d pci_sysport:%d application_v2:%d\n",
                        kmsg->clock_info.data[1], kmsg->clock_info.data[2], kmsg->clock_info.data[3],
                        kmsg->clock_info.data[4], kmsg->clock_info.data[5], kmsg->clock_info.data[9]));

                DBG_VERB(("uc_port:%d uc_sysport:%d pci_port:%d pci_sysport:%d application_v2:%d\n",
                        (dev_info->init_data).uc_port_num,
                        (dev_info->init_data).uc_port_sysport,
                        (dev_info->init_data).host_cpu_port,
                        (dev_info->init_data).host_cpu_sysport,
                        (dev_info->init_data).application_v2));

                if (bksync_ptp_init(dev_info, &(dev_info->ptp_info)) >= 0) {
                    dev_info->dev_init = 1;
                }
            } else {
                DBG_ERR(("Invalid core number %d\n", fw_core));
                kmsg->hdr.status = KCOM_E_PARAM;
                return sizeof(kcom_msg_hdr_t);
            }
            break;
        case KSYNC_M_HW_DEINIT:

            /* If module is not init then don't call DEINIT */
            if (dev_info->dev_init) {
#ifndef BDE_EDK_SUPPORT
                DEV_WRITE32(dev_info, CMIC_CMC_SCHAN_MESSAGE_15r(CMIC_CMC_BASE), 0);
                DEV_WRITE32(dev_info, CMIC_CMC_SCHAN_MESSAGE_16r(CMIC_CMC_BASE), 0);
#endif
                bksync_ptp_deinit(dev_info);

                dev_info->dev_init = 0;
            }
            break;
        case KSYNC_M_HW_TS_DISABLE:
            bksync_ptp_hw_tstamp_disable(0, kmsg->clock_info.data[0], 0);
            break;
        case KSYNC_M_MTP_TS_UPDATE_ENABLE:
            bksync_ptp_mirror_encap_update(dev_info, NULL, kmsg->clock_info.data[0], TRUE);
            break;
        case KSYNC_M_MTP_TS_UPDATE_DISABLE:
            bksync_ptp_mirror_encap_update(dev_info, NULL, kmsg->clock_info.data[0], FALSE);
            break;
        case KSYNC_M_VERSION:
            break;
        case KSYNC_M_DNX_JR2DEVS_SYS_CONFIG:
            DBG_VERB(("bksync_ioctl_cmd_handler: KSYNC_M_DNX_JR2DEVS_SYS_CONFIG Rcvd.\n"));

            header_data = (bksync_dnx_jr2_header_info_t *)((char *)kmsg + sizeof(kcom_msg_clock_cmd_t));

            (dev_info->jr2_header_data).ftmh_lb_key_ext_size = header_data->ftmh_lb_key_ext_size;
            (dev_info->jr2_header_data).ftmh_stacking_ext_size = header_data->ftmh_stacking_ext_size;
            (dev_info->jr2_header_data).pph_base_size = header_data->pph_base_size;

            for (tmp = 0; tmp < BKSYNC_DNXJER2_PPH_LIF_EXT_TYPE_MAX; tmp++) {
                (dev_info->jr2_header_data).pph_lif_ext_size[tmp] = header_data->pph_lif_ext_size[tmp];
            }

            (dev_info->jr2_header_data).system_headers_mode = header_data->system_headers_mode;
            (dev_info->jr2_header_data).udh_enable = header_data->udh_enable;
            for (tmp = 0; tmp < BKSYNC_DNXJER2_UDH_DATA_TYPE_MAX; tmp++) {
                (dev_info->jr2_header_data).udh_data_lenght_per_type[tmp] = header_data->udh_data_lenght_per_type[tmp];
            }

            (dev_info->jr2_header_data).cosq_port_cpu_channel = header_data->cosq_port_cpu_channel;
            (dev_info->jr2_header_data).cosq_port_pp_port = header_data->cosq_port_pp_port;

            header_data = &(dev_info->jr2_header_data);

#if 0
            DBG_VERB(("ftmh_lb_key_ext_size %u ftmh_stacking_ext_size %u pph_base_size %u\n",
                    header_data->ftmh_lb_key_ext_size, header_data->ftmh_stacking_ext_size,
                    header_data->pph_base_size));

            for (tmp = 0; tmp < BKSYNC_DNXJER2_PPH_LIF_EXT_TYPE_MAX ; tmp++) {
                DBG_VERB(("pph_lif_ext_size[%u] %u\n",
                        tmp, header_data->pph_lif_ext_size[tmp]));
            }

            DBG_VERB(("system_headers_mode %u udh_enable %u\n",
                    header_data->system_headers_mode, header_data->udh_enable));

            for (tmp = 0; tmp < BKSYNC_DNXJER2_UDH_DATA_TYPE_MAX; tmp++) {
                DBG_VERB(("udh_data_lenght_per_type [%d] %u\n",
                        tmp, header_data->udh_data_lenght_per_type[tmp]));
            }

            DBG_VERB(("cosq_port_cpu_channel :%u cosq_port_pp_port:%u\n",
                    header_data->cosq_port_cpu_channel, header_data->cosq_port_cpu_channel));
#endif
            break;
        case KSYNC_M_BS_CONFIG_SET:
            bs_id = kmsg->clock_info.data[0];

            dev_info->bksync_bs_info[bs_id].enable = 1;
            dev_info->bksync_bs_info[bs_id].mode = kmsg->clock_info.data[1];
            dev_info->bksync_bs_info[bs_id].bc = kmsg->clock_info.data[2];
            dev_info->bksync_bs_info[bs_id].hb = kmsg->clock_info.data[3];

            (void)bksync_broadsync_cmd(dev_info, bs_id);
            break;

        case KSYNC_M_BS_CONFIG_CLEAR:
            bs_id = kmsg->clock_info.data[0];

            dev_info->bksync_bs_info[bs_id].enable = 0;

            (void)bksync_broadsync_cmd(dev_info, bs_id);
            break;

        case KSYNC_M_BS_STATUS:
            bs_id = kmsg->clock_info.data[0];

            kmsg->hdr.type = KCOM_MSG_TYPE_RSP;
            (void)bksync_broadsync_status_cmd(dev_info, bs_id, &status);

            kmsg->clock_info.data[1] = (int)(status >> 32); /* Variance */
            kmsg->clock_info.data[2] = (int)(status & 0xFFFFFFFF); /* Status */
            break;

#ifdef BDE_EDK_SUPPORT
        case KSYNC_M_PTP_TOD_OFFSET_SET:
            dev_info->ptp_tod.offset.sign = (int)kmsg->clock_info.data[0];
            dev_info->ptp_tod.offset.sec = (uint64_t)(((uint64_t)kmsg->clock_info.data[1]) << 32 | (uint32_t)kmsg->clock_info.data[2]);
            dev_info->ptp_tod.offset.nsec = (uint32_t)kmsg->clock_info.data[3];

            (void)bksync_ptp_tod_cmd(dev_info, dev_info->ptp_tod.offset.sign, dev_info->ptp_tod.offset.sec,
                                        dev_info->ptp_tod.offset.nsec);
            break;

        case KSYNC_M_NTP_TOD_OFFSET_SET:
            dev_info->ntp_tod.epoch_offset = (uint64_t)(((uint64_t)kmsg->clock_info.data[0]) << 32 | (uint32_t)kmsg->clock_info.data[1]);

            (void)bksync_ntp_tod_cmd(dev_info, 0, 0 , dev_info->ntp_tod.epoch_offset);
            break;

        case KSYNC_M_PTP_TOD_OFFSET_GET:
            kmsg->hdr.type = KCOM_MSG_TYPE_RSP;
            kmsg->clock_info.data[0] = dev_info->ptp_tod.offset.sign;
            kmsg->clock_info.data[1] = (int32_t)(dev_info->ptp_tod.offset.sec >> 32);
            kmsg->clock_info.data[2] = (int32_t)dev_info->ptp_tod.offset.sec;
            kmsg->clock_info.data[3] = (int32_t)(dev_info->ptp_tod.offset.nsec);
            break;

        case KSYNC_M_NTP_TOD_OFFSET_GET:
            kmsg->hdr.type = KCOM_MSG_TYPE_RSP;
            kmsg->clock_info.data[0] = (int32_t)(dev_info->ntp_tod.epoch_offset >> 32);
            kmsg->clock_info.data[1] = (int32_t)dev_info->ntp_tod.epoch_offset;
            break;

        case KSYNC_M_PTP_TOD_GET:
            (void)bksync_ptp_tod_get_cmd(dev_info, &tod_time);
            kmsg->hdr.type = KCOM_MSG_TYPE_RSP;
            kmsg->clock_info.data[0] = (int32_t)(tod_time.sec >> 32);
            kmsg->clock_info.data[1] = (int32_t)(tod_time.sec);
            kmsg->clock_info.data[2] = (int32_t)(tod_time.nsec);
            break;

        case KSYNC_M_NTP_TOD_GET:
            (void)bksync_ntp_tod_get_cmd(dev_info, &tod_time);
            kmsg->hdr.type = KCOM_MSG_TYPE_RSP;
            kmsg->clock_info.data[0] = (int32_t)(tod_time.sec >> 32);
            kmsg->clock_info.data[1] = (int32_t)(tod_time.sec);
            kmsg->clock_info.data[2] = (int32_t)(tod_time.nsec);
            break;

        case KSYNC_M_LEAP_SEC_SET:
            dev_info->ntp_tod.leap_sec_ctrl_en = kmsg->clock_info.data[0];
            dev_info->ntp_tod.leap_sec_op = kmsg->clock_info.data[1];

            (void)bksync_ntp_tod_cmd(dev_info, dev_info->ntp_tod.leap_sec_ctrl_en,
                        dev_info->ntp_tod.leap_sec_op, 0);
            break;

        case KSYNC_M_LEAP_SEC_GET:
            kmsg->hdr.type = KCOM_MSG_TYPE_RSP;
            kmsg->clock_info.data[0] = dev_info->ntp_tod.leap_sec_ctrl_en;
            kmsg->clock_info.data[1] = dev_info->ntp_tod.leap_sec_op;
            break;

#endif

        case KSYNC_M_GPIO_CONFIG_SET:
            gpio = kmsg->clock_info.data[0];

            dev_info->bksync_gpio_info[gpio].enable = (uint32_t)kmsg->clock_info.data[1];
            dev_info->bksync_gpio_info[gpio].mode = (uint32_t)kmsg->clock_info.data[2];
            dev_info->bksync_gpio_info[gpio].period = (uint32_t)kmsg->clock_info.data[3];

            (void)bksync_gpio_cmd(dev_info, gpio);

            if (dev_info->bksync_gpio_info[gpio].phaseoffset != kmsg->clock_info.data[4]) {
                dev_info->bksync_gpio_info[gpio].phaseoffset = (int64_t)kmsg->clock_info.data[4];
                (void)bksync_gpio_phaseoffset_cmd(dev_info, gpio);
            }
            break;

        case KSYNC_M_GPIO_CONFIG_GET:
            dev_info = &ptp_priv->dev_info[dev_no];
            gpio = kmsg->clock_info.data[0];

            kmsg->hdr.type = KCOM_MSG_TYPE_RSP;
            kmsg->clock_info.data[1] = (int32_t)dev_info->bksync_gpio_info[gpio].mode;
            kmsg->clock_info.data[2] = (int32_t)dev_info->bksync_gpio_info[gpio].period;
            kmsg->clock_info.data[3] = (int32_t)dev_info->bksync_gpio_info[gpio].phaseoffset;
            break;

        case KSYNC_M_BS_PHASE_OFFSET_SET:
            bs_id = kmsg->clock_info.data[0];

            bs_offset.sign = kmsg->clock_info.data[1];
            bs_offset.sec = (uint64_t)(((uint64_t)kmsg->clock_info.data[2] << 32) | 
                                                                (uint32_t)kmsg->clock_info.data[3]);
            bs_offset.nsec = (uint32_t)kmsg->clock_info.data[4];

            (void)bksync_broadsync_phase_offset_cmd(dev_info, bs_id, bs_offset);
            break;

        default:
            kmsg->hdr.status = KCOM_E_NOT_FOUND;
            return sizeof(kcom_msg_hdr_t);
    }

    return sizeof(*kmsg);
}

static int bksync_phc_create(int dev_no)
{
    bksync_dev_t *dev_info = &ptp_priv->dev_info[dev_no];
    int err = 0;
    int no_ext_ts = 0;

    memset(dev_info, 0, sizeof(bksync_dev_t));

    dev_info->port_stats = NULL;

    dev_info->dev_no = dev_no;
    err = bkn_hw_device_get(dev_no, &dev_info->dev_id, NULL);
    if (err) {
        return -ENODEV;
    }

    switch(dev_info->dev_id) {
        case 0x8870:    /* Q3D */
        case 0x8860:    /* JR3 */
        case 0x8890:    /* JRAI */
        case 0x8490:    /* Q3A */
            dev_info->max_core = 6;
            break;
        default:
            dev_info->max_core = 2;
            break;
    }

    switch(dev_info->dev_id) {
        case 0x8870:    /* Q3D */
            no_ext_ts = BKSYNC_NUM_GPIO_EVENTS * 2;
            break;
        default:
            no_ext_ts = BKSYNC_NUM_GPIO_EVENTS;
            break;
    }

    /* Initialize the Base address for CMIC and shared Memory access */
    dev_info->base_addr = lkbde_get_dev_virt(dev_no);
    dev_info->dma_dev = lkbde_get_dma_dev(dev_no);

    bksync_ptp_info.n_ext_ts = no_ext_ts;

    dev_info->ptp_info = bksync_ptp_info;

    mutex_init(&(dev_info->ptp_lock));

    if ((shared_phc == 1) && (dev_no != master_core)) {
        return 0;
    }

    /* Register ptp clock driver with bksync_ptp_info */
    dev_info->ptp_clock = ptp_clock_register(&dev_info->ptp_info, NULL);
    if (IS_ERR(dev_info->ptp_clock)) {
        return -ENODEV;
    }

    return 0;
}


/**
 * bksync_ptp_register
 * @priv: driver private structure
 * Description: this function will register the ptp clock driver
 * to kernel. It also does some house keeping work.
 */
static int bksync_ptp_register(void)
{
    int dev_no = 0, max_dev = 0;
    int err = -ENODEV;

    /* Connect to the kernel bde */
    if ((linux_bde_create(NULL, &kernel_bde) < 0) || kernel_bde == NULL) {
        return -ENODEV;
    }

    max_dev = kernel_bde->num_devices(BDE_SWITCH_DEVICES);

    DBG_VERB(("Number of devices attached %d\n", max_dev));
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
    memset(ptp_priv, 0, sizeof(*ptp_priv));

    ptp_priv->max_dev = max_dev;

    ptp_priv->dev_info = kzalloc((sizeof(bksync_dev_t) * max_dev), GFP_KERNEL);
    if (!ptp_priv->dev_info) {
        err = -ENOMEM;
        goto exit;
    }

    for (dev_no = 0; dev_no < max_dev; dev_no++) {
        err = bksync_phc_create(dev_no);
        if (err) {
            goto exit;
        }
    }

    if (shared_phc == 1) {
        for (dev_no = 0; dev_no < max_dev; dev_no++) {
            if (dev_no == master_core) {
                continue;
            }
            ptp_priv->dev_info[dev_no].ptp_clock = ptp_priv->dev_info[master_core].ptp_clock;
        }
    }


    /* Register BCM-KNET HW Timestamp Callback Functions */
    bkn_hw_tstamp_enable_cb_register(bksync_ptp_hw_tstamp_enable);
    bkn_hw_tstamp_disable_cb_register(bksync_ptp_hw_tstamp_disable);
    bkn_hw_tstamp_tx_time_get_cb_register(bksync_ptp_hw_tstamp_tx_time_get);
    bkn_hw_tstamp_tx_meta_get_cb_register(bksync_ptp_hw_tstamp_tx_meta_get);
    bkn_hw_tstamp_rx_pre_process_cb_register(bksync_ptp_hw_tstamp_rx_pre_process);
    bkn_hw_tstamp_rx_time_upscale_cb_register(bksync_ptp_hw_tstamp_rx_time_upscale);
    bkn_hw_tstamp_ptp_clock_index_cb_register(bksync_ptp_hw_tstamp_ptp_clock_index_get);
    bkn_hw_tstamp_ioctl_cmd_cb_register(bksync_ioctl_cmd_handler);
    bkn_hw_tstamp_ptp_transport_get_cb_register(bksync_ptp_transport_get);

     /* Initialize proc files */
     bksync_proc_root = proc_mkdir("bcm/ksync", NULL);

     err = bksync_proc_init();
     if (err) {
        err = -ENODEV;
        DBG_ERR(("Failed to init proc files\n"));
        goto exit;
     }

     err = bksync_sysfs_init();
     if (err) {
        err = -ENODEV;
        DBG_ERR(("Failed to init sysfs files\n"));
        goto exit;
     }

     bksync_ptp_extts_logging_init();
exit:
    if (err < 0) {
        bksync_ptp_remove();
    }
    return err;
}

static int bksync_ptp_remove(void)
{
    int dev_no;
    bksync_dev_t *dev_info = NULL;

    if (!ptp_priv)
        return 0;

    bksync_ptp_extts_logging_deinit();

    bksync_ptp_time_keep_deinit();

    bksync_proc_cleanup();
    bksync_sysfs_cleanup();

    /* UnRegister BCM-KNET HW Timestamp Callback Functions */
    bkn_hw_tstamp_enable_cb_unregister(bksync_ptp_hw_tstamp_enable);
    bkn_hw_tstamp_disable_cb_unregister(bksync_ptp_hw_tstamp_disable);
    bkn_hw_tstamp_tx_time_get_cb_unregister(bksync_ptp_hw_tstamp_tx_time_get);
    bkn_hw_tstamp_tx_meta_get_cb_unregister(bksync_ptp_hw_tstamp_tx_meta_get);
    bkn_hw_tstamp_rx_pre_process_cb_unregister(bksync_ptp_hw_tstamp_rx_pre_process);
    bkn_hw_tstamp_rx_time_upscale_cb_unregister(bksync_ptp_hw_tstamp_rx_time_upscale);
    bkn_hw_tstamp_ptp_clock_index_cb_unregister(bksync_ptp_hw_tstamp_ptp_clock_index_get);
    bkn_hw_tstamp_ioctl_cmd_cb_unregister(bksync_ioctl_cmd_handler);
    bkn_hw_tstamp_ptp_transport_get_cb_unregister(bksync_ptp_transport_get);

    for (dev_no = 0; dev_no < ptp_priv->max_dev; dev_no++) {
        dev_info = &ptp_priv->dev_info[dev_no];

        if (dev_info->dev_init) {
#ifndef BDE_EDK_SUPPORT
            /* reset handshaking info */
            DEV_WRITE32(dev_info, CMIC_CMC_SCHAN_MESSAGE_15r(CMIC_CMC_BASE), 0);
            DEV_WRITE32(dev_info, CMIC_CMC_SCHAN_MESSAGE_16r(CMIC_CMC_BASE), 0);
#endif
            /* Deinitialize */
            bksync_ptp_deinit(dev_info);

            dev_info->dev_init = 0;
        }

        mutex_destroy(&dev_info->ptp_lock);

        if (dev_info->ptp_clock) {
            /* Unregister the bcm ptp clock driver */
            if (shared_phc == 1) {
                if (dev_no == master_core) {
                    ptp_clock_unregister(dev_info->ptp_clock);
                }
            } else {
                ptp_clock_unregister(dev_info->ptp_clock);
            }
            dev_info->ptp_clock = NULL;
        }
    }

    bksync_ptp_fw_data_free();

    for (dev_no = 0; dev_no < ptp_priv->max_dev; dev_no++) {
        dev_info = &ptp_priv->dev_info[dev_no];

        if (dev_info->port_stats != NULL) {
            kfree((void *)dev_info->port_stats);
            dev_info->port_stats = NULL;
        }
    }

    /* Free Memory */
    if (ptp_priv->dev_info) {
        kfree(ptp_priv->dev_info);
    }
    kfree(ptp_priv);

    return 0;
}
#endif


/*
 * Generic module functions
 */

/*
 * Function: _pprint
 *
 * Purpose:
 *    Print proc filesystem information.
 * Parameters:
 *    None
 * Returns:
 *    Always 0
 */
    static int
_pprint(struct seq_file *m)
{
#ifdef PTPCLOCK_SUPPORTED
    /* put some goodies here */
    pprintf(m, "Broadcom BCM PTP Hardware Clock Module\n");
#else
    pprintf(m, "Broadcom BCM PTP Hardware Clock Module not supported\n");
#endif
    return 0;
}

/*
 * Function: _init
 *
 * Purpose:
 *    Module initialization.
 *    Attached SOC all devices and optionally initializes these.
 * Parameters:
 *    None
 * Returns:
 *    0 on success, otherwise -1
 */
    static int
_init(void)
{
#ifdef PTPCLOCK_SUPPORTED
    bksync_ptp_register();
    return 0;
#else
    return -1;
#endif
}

/*
 * Function: _cleanup
 *
 * Purpose:
 *    Module cleanup function
 * Parameters:
 *    None
 * Returns:
 *    Always 0
 */
    static int
_cleanup(void)
{
#ifdef PTPCLOCK_SUPPORTED
    bksync_ptp_remove();
    return 0;
#else
    return -1;
#endif
}

static gmodule_t _gmodule = {
    .name = MODULE_NAME,
    .major = MODULE_MAJOR,
    .init = _init,
    .cleanup = _cleanup,
    .pprint = _pprint,
    .ioctl = NULL,
    .open = NULL,
    .close = NULL,
};

    gmodule_t*
gmodule_get(void)
{
    EXPORT_NO_SYMBOLS;
    return &_gmodule;
}
