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

#if ((LINUX_VERSION_CODE > KERNEL_VERSION(3,17,0)) && (LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)))
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


#define CMICX_DEV_TYPE   ((ptp_priv->dcb_type == 38) || \
                          (ptp_priv->dcb_type == 36) || \
                          (ptp_priv->dcb_type == 39))

/* Arad Series of DNX Devices */
#define DEVICE_IS_DPP       (ptp_priv->dcb_type == 28)

/* JR2 Series of DNX Devices */
#define DEVICE_IS_DNX       (ptp_priv->dcb_type == 39)

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

#define BCMKSYNC_NUM_PORTS           128    /* NUM_PORTS where 2-step is supported. */
#define BCMKSYNC_MAX_NUM_PORTS       256    /* Max ever NUM_PORTS in the system */
#define BCMKSYNC_MAX_MTP_IDX        8    /* Max number of mtps in the system */

#define BKN_DNX_PTCH_2_SIZE         2       /* PTCH_2 */
#define BKN_DNX_ITMH_SIZE           5       /* ITMH */

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
    KSYNC_SYSINFO_UC_PORT_NUM       = (0x1),
    KSYNC_SYSINFO_UC_PORT_SYSPORT   = (0x2),
    KSYNC_SYSINFO_HOST_CPU_PORT     = (0x3),
    KSYNC_SYSINFO_HOST_CPU_SYSPORT  = (0x4),
    KSYNC_SYSINFO_UDH_LEN           = (0x5),
};

enum {
    KSYNC_BROADSYNC_BS0_CONFIG      = (0x1),
    KSYNC_BROADSYNC_BS1_CONFIG      = (0x2),
    KSYNC_BROADSYNC_BS0_STATUS_GET  = (0x3),
    KSYNC_BROADSYNC_BS1_STATUS_GET  = (0x4),
};

enum {
    KSYNC_GPIO_0                    = (0x1),
    KSYNC_GPIO_1                    = (0x2),
    KSYNC_GPIO_2                    = (0x3),
    KSYNC_GPIO_3                    = (0x4),
    KSYNC_GPIO_4                    = (0x5),
    KSYNC_GPIO_5                    = (0x6),
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

#define SKB_U16_GET(_skb, _pkt_offset) \
            ((_skb->data[_pkt_offset] << 8) | _skb->data[_pkt_offset + 1])

#define BKSYNC_PTP_EVENT_MSG(_ptp_msg_type) \
            ((_ptp_msg_type == IEEE1588_MSGTYPE_DELREQ) ||    \
             (_ptp_msg_type == IEEE1588_MSGTYPE_SYNC))


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
#define HWTSTAMP_TX_ONESTEP_SYNC 2
#else
#include <linux/net_tstamp.h>
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

/* Driver Proc Entry root */
static struct proc_dir_entry *bksync_proc_root = NULL;

/* Shared data structures with R5 */
typedef struct _bksync_tx_ts_data_s {
    u32 ts_valid;   /* Timestamp valid indication */
    u32 port_id;    /* Port number */
    u32 ts_seq_id;  /* Sequency Id */
    u32 ts_cnt;
    u64 timestamp;  /* Timestamp */
} bksync_tx_ts_data_t;

typedef struct bksync_info_s {
    u32 ksyncinit;
    u32 dev_id;
    s64 freqcorr;
    u64 portmap[BCMKSYNC_MAX_NUM_PORTS/64];  /* Two-step enabled ports */
    u64 ptptime;
    u64 reftime;
    u64 ptptime_alt;
    u64 reftime_alt;
    s64 phase_offset;
    bksync_tx_ts_data_t port_ts_data[BCMKSYNC_MAX_NUM_PORTS];
} bksync_info_t;


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

typedef struct bksync_evlog_s {
    bksync_fw_debug_event_tstamps_t event_timestamps[NUM_TS_EVENTS];
} __ATTRIBUTE_PACKED__ bksync_evlog_t;


/* Timestamps for EXTTS from Firmware */
#define NUM_EXT_TS          6       /* gpio0 = event0 ..... gpio5 = event5 */
#define NUM_EVENT_TS        128     /* Directly mapped to PTP_MAX_TIMESTAMPS from ptp_private.h */
typedef struct bksync_fw_extts_event_s {
    u32 ts_event_id;
    fw_tstamp_t tstamp;
} __ATTRIBUTE_PACKED__ bksync_fw_extts_event_t;

typedef struct bksync_extts_log_s {
    u32 head;   /* Read pointer - Updated by HOST */
    u32 tail;   /* Write pointer - Updated by FW */
    bksync_fw_extts_event_t event_ts[NUM_EVENT_TS];
    u32 overflow;
} __ATTRIBUTE_PACKED__ bksync_fw_extts_log_t;

typedef struct bksync_fw_comm_s {
    u32 cmd;
    u32 dw1[2];
    u32 dw2[2];
    u32 head;   /* Read pointer - Updated by HOST */
    u32 tail;   /* Write pointer - Updated by FW */
} __ATTRIBUTE_PACKED__ bksync_fw_comm_t;

struct bksync_extts_event {
    int enable[NUM_EXT_TS];
    int head;
};

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
} bksync_bs_info_t;

typedef struct bksync_gpio_info_s {
    u32 enable;
    u32 mode;
    u32 period;
    int64_t phaseoffset;
} bksync_gpio_info_t;

typedef struct bksync_evlog_info_s {
    u32 enable;
} bksync_evlog_info_t;

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


typedef struct  bksync_dnx_jr2_devices_system_info_s {
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
} bksync_dnx_jr2_devices_system_info_t;

typedef enum bksync_dnxjr2_system_headers_mode_e {
    bksync_dnxjr2_sys_hdr_mode_jericho = 0,
    bksync_dnxjr2_sys_hdr_mode_jericho2 = 1
} bksync_dnxjr2_system_headers_mode_t;

/* DNX JR2 FTMH Header information */
#define BKSYNC_DNXJR2_FTMH_HDR_LEN              (10)
#define BKSYNC_DNXJR2_FTMH_TM_DEST_EXT_LEN      (3)
#define BKSYNC_DNXJR2_FTMH_FLOWID_EXT_LEN       (3)
#define BKSYNC_DNXJR2_FTMH_BEIR_BFR_EXT_LEN     (3)
#define BKSYNC_DNXJR2_FTMH_APP_SPECIFIC_EXT_LEN     (6)

/* DNX FTMH PPH type */
#define BKSYNC_DNXJR2_PPH_TYPE_NO_PPH            (0)
#define BKSYNC_DNXJR2_PPH_TYPE_PPH_BASE          (1)
#define BKSYNC_DNXJR2_PPH_TYPE_TSH_ONLY          (2)
#define BKSYNC_DNXJR2_PPH_TYPE_PPH_BASE_TSH      (3)

typedef enum bksync_dnxjr2_ftmh_tm_action_type_e {
    bksync_dnxjr2_ftmh_tm_action_type_forward = 0,               /* TM action is forward */
    bksync_dnxjr2_ftmh_tm_action_type_snoop = 1,                 /* TM action is snoop */
    bksync_dnxjr2_ftmh_tm_action_type_inbound_mirror = 2,        /* TM action is inbound mirror.  */
    bksync_dnxjr2_ftmh_tm_action_type_outbound_mirror = 3,       /* TM action is outbound mirror. */
    bksync_dnxjr2_ftmh_tm_action_type_mirror = 4,                /* TM action is mirror. */
    bksync_dnxjr2_ftmh_tm_action_type_statistical_sampling = 5   /* TM action is statistical sampling. */
} bksync_dnxjr2_ftmh_tm_action_type_t;

typedef enum bksync_dnxjr2_ftmh_app_spec_ext_type_e {
    bksync_dnxjr2_ftmh_app_spec_ext_type_none             = 0,   /* FTMH ASE type is None or OAM */
    bksync_dnxjr2_ftmh_app_spec_ext_type_1588v2           = 1,   /* FTMH ASE type is 1588v2 */
    bksync_dnxjr2_ftmh_app_spec_ext_type_mirror           = 3,   /* FTMH ASE type is Mirror */
    bksync_dnxjr2_ftmh_app_spec_ext_type_trajectory_trace = 4,   /* FTMH ASE type is trajectory trace */
    bksync_dnxjr2_ftmh_app_spec_ext_type_inband_telemetry = 5,   /* FTMH ASE type is Inband telemetry */
} bksync_dnxjr2_ftmh_app_spec_ext_type_t;


typedef union bksync_dnxjr2_ftmh_base_header_s {
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
} bksync_dnxjr2_ftmh_base_header_t;

typedef union bksync_dnxjr2_ftmh_app_spec_ext_1588v2_s {
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
} bksync_dnxjr2_ftmh_app_spec_ext_1588v2_t;

/* DNX TSH Header size */
#define BKSYNC_DNXJR2_TSH_HDR_SIZE              (4)

typedef union bksync_dnxjr2_timestamp_header_s {
    struct {
        uint32_t word;
    };
    struct {
        uint32_t timestamp;
    };
}  bksync_dnxjr2_timestamp_header_t;

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

typedef enum bksync_dnxjr2_pph_fheiext_type_e {
    bksync_dnxjr2_pph_fheiext_type_vlanedit = 0,
    bksync_dnxjr2_pph_fheiext_type_pop = 1,
    bksync_dnxjr2_pph_fheiext_type_swap =  3,
    bksync_dnxjr2_pph_fheiext_type_trap_snoop_mirror =  5,
} bksync_dnxjr2_pph_fheiext_type_t;

typedef union bksync_dnxjr2_pph_base_12b_header_s {
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
} bksync_dnxjr2_pph_base_12b_header_t;

typedef union bksync_dnxjr2_pph_fheiext_vlanedit_3b_header_s {
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
} bksync_dnxjr2_pph_fheiext_vlanedit_3b_header_t;

typedef union bksync_dnxjr2_pph_fheiext_vlanedit_5b_header_s {
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
} bksync_dnxjr2_pph_fheiext_vlanedit_5b_header_t;

typedef union bksync_dnxjr2_pph_fheiext_trap_header_s {
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
} bksync_dnxjr2_pph_fheiext_trap_header_t;

#define BKSYNC_DNXJR2_UDH_BASE_HEADER_LEN       (1)
#define BKSYNC_DNXJR2_UDH_DATA_TYPE_MAX          (4)

typedef union bksync_dnxjr2_udh_base_header_s {
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
} bksync_dnxjr2_udh_base_header_t;

#define DNX_PTCH_TYPE2_HEADER_LEN   2
typedef union bksync_dnxjr2_ptch_type2_header_s {
    struct {
        uint8_t bytes[DNX_PTCH_TYPE2_HEADER_LEN];
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
} bksync_dnxjr2_ptch_type2_header_t;

#define DNX_DNXJR2_MODULE_HEADER_LEN   16
#define DNX_DNXJR2_ITMH_HEADER_LEN     5


/* Clock Private Data */
struct bksync_ptp_priv {
    struct device           dev;
    int                     dcb_type;
    struct ptp_clock        *ptp_clock;
    struct ptp_clock_info   ptp_caps;
    struct mutex            ptp_lock;
    int                     ptp_pair_lock;
    volatile void           *base_addr;   /* address for PCI register access */
    volatile bksync_info_t  *shared_addr; /* address for shared memory access */
    volatile bksync_evlog_t *evlog;       /* dma-able address for fw updates */
    dma_addr_t              dma_mem;
    int                     dma_mem_size;
    struct DMA_DEV          *dma_dev;     /* Required for DMA memory control */
    int                     num_pports;
    int                     timekeep_status;
    u32                     mirror_encap_bmp;
    struct delayed_work     time_keep;
    bksync_port_stats_t     *port_stats;
    bksync_init_info_t      bksync_init_info;
    bksync_bs_info_t        bksync_bs_info[2];
    bksync_gpio_info_t      bksync_gpio_info[6];
    bksync_evlog_info_t     bksync_evlog_info[NUM_TS_EVENTS];
    bksync_dnx_jr2_devices_system_info_t bksync_jr2devs_sys_info;
    volatile bksync_fw_extts_log_t      *extts_log; /* dma-able/virtual address for fw updates */
    struct bksync_extts_event           extts_event;
    struct delayed_work                 extts_logging;

    struct kobject                      *kobj;
#ifdef BDE_EDK_SUPPORT
    volatile bksync_fw_comm_t           *fw_comm;
#else
    int                                 extts_dma_mem_size;
    dma_addr_t                          extts_dma_mem_addr;
#endif
};

static struct bksync_ptp_priv *ptp_priv;
volatile bksync_info_t *linuxPTPMemory = (bksync_info_t*)(0);
static volatile int module_initialized;
static int num_retries = 5;   /* Retry count */

static void bksync_ptp_time_keep_init(void);
static void bksync_ptp_time_keep_deinit(void);
void bksync_dnxjr2_parse_rxpkt_system_header(uint8_t *raw_frame,  bksync_dnx_rx_pkt_parse_info_t *rx_pkt_parse_info, int isfirsthdr);
static int bksync_ptp_gettime(struct ptp_clock_info *ptp, struct timespec64 *ts);

static void bksync_ptp_extts_logging_init(void);
static void bksync_ptp_extts_logging_deinit(void);

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

static void
ptp_usleep(int usec)
{
    if (DEVICE_IS_DNX) {
        udelay(usec);
    } else {
        usleep_range(usec,usec+1);
    }
}

static void
ptp_sleep(int jiffies)
{
    wait_queue_head_t wq;
    init_waitqueue_head(&wq);

    wait_event_timeout(wq, 0, jiffies);

}

#ifdef BDE_EDK_SUPPORT
static void bksync_hostcmd_data_op(int setget, u64 *d1, u64 *d2)
{
    u32 w0, w1;
    u64 data;

    if (!d1) {
        return;
    }

    if (setget) {
        if (d1) {
            data = *d1;
            ptp_priv->fw_comm->dw1[0] = (data & 0xFFFFFFFF);
            ptp_priv->fw_comm->dw1[1] = (data >> 32);
        }

        if (d2) {
            data = *d2;
            ptp_priv->fw_comm->dw2[0] = (data & 0xFFFFFFFF);
            ptp_priv->fw_comm->dw2[1] = (data >> 32);
        }

    } else {
        if (d1) {
            w0 = ptp_priv->fw_comm->dw1[0];
            w1 = ptp_priv->fw_comm->dw1[1];
            data = (((u64)w1 << 32) | (w0));
            *d1 = data;
        }

        if (d2) {
            w0 = ptp_priv->fw_comm->dw2[0];
            w1 = ptp_priv->fw_comm->dw2[1];
            data = (((u64)w1 << 32) | (w0));
            *d2 = data;
        }
    }
}
#else
static void bksync_hostcmd_data_op(int setget, u64 *d1, u64 *d2)
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
#endif

static int bksync_cmd_go(u32 cmd, void *data0, void *data1)
{
    int ret = -1;
    int retry_cnt = (1000); /* 1ms default timeout for hostcmd response */
    u32 cmd_status;
    char cmd_str[30];
    int port = 0;
    uint32_t seq_id = 0;
    ktime_t start, now;
    u32 subcmd = 0;

    if (ptp_priv == NULL || ptp_priv->shared_addr == NULL) {
        return ret;
    }

    mutex_lock(&ptp_priv->ptp_lock);

    if (cmd == BKSYNC_GET_TSTIME || cmd == BKSYNC_ACK_TSTIME) {
        port = *((uint64_t *)data0) & 0xFFF;
        seq_id = *((uint64_t*)data0) >> 16;
    }
    start = ktime_get();

    ptp_priv->shared_addr->ksyncinit = cmd;

    /* init data */
#ifdef BDE_EDK_SUPPORT
    ptp_priv->fw_comm->dw1[0] = 0;
    ptp_priv->fw_comm->dw1[1] = 0;
    ptp_priv->fw_comm->dw2[0] = 0;
    ptp_priv->fw_comm->dw2[1] = 0;
#else
    DEV_WRITE32(ptp_priv, hostcmd_regs[1], 0x0);
    DEV_WRITE32(ptp_priv, hostcmd_regs[2], 0x0);
    DEV_WRITE32(ptp_priv, hostcmd_regs[3], 0x0);
    DEV_WRITE32(ptp_priv, hostcmd_regs[4], 0x0);
#endif

    switch (cmd) {
        case BKSYNC_INIT:
            retry_cnt = (retry_cnt * 4);
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_INIT");
            ptp_priv->shared_addr->phase_offset  = 0;
            bksync_hostcmd_data_op(1, (u64 *)&(ptp_priv->shared_addr->phase_offset), 0);
            break;
        case BKSYNC_FREQCOR:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_FREQCORR");
            ptp_priv->shared_addr->freqcorr  = *((s32 *)data0);
            bksync_hostcmd_data_op(1, (u64 *)&(ptp_priv->shared_addr->freqcorr), 0);
            break;
        case BKSYNC_ADJTIME:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_ADJTIME");
            ptp_priv->shared_addr->phase_offset  = *((s64 *)data0);
            bksync_hostcmd_data_op(1, (u64 *)&(ptp_priv->shared_addr->phase_offset), 0);
            break;
        case BKSYNC_GETTIME:
            retry_cnt = (retry_cnt * 2);
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_GETTIME");
            break;
        case BKSYNC_GET_TSTIME:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_GET_TSTIME");
            bksync_hostcmd_data_op(1, data0, data1);
            break;
         case BKSYNC_ACK_TSTIME:
            retry_cnt = (retry_cnt * 2);
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_ACK_TSTIME");
            bksync_hostcmd_data_op(1, data0, data1);
            break;
        case BKSYNC_SETTIME:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_SETTIME");
            ptp_priv->shared_addr->ptptime   = *((s64 *)data0);
            ptp_priv->shared_addr->phase_offset = 0;
            bksync_hostcmd_data_op(1, (u64 *)&(ptp_priv->shared_addr->ptptime), (u64 *)&(ptp_priv->shared_addr->phase_offset));
            break;
        case BKSYNC_MTP_TS_UPDATE_ENABLE:
            retry_cnt = (retry_cnt * 6);
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_MTP_TS_UPDATE_ENABLE");
            bksync_hostcmd_data_op(1, (u64 *)data0, 0);
            break;
        case BKSYNC_MTP_TS_UPDATE_DISABLE:
            retry_cnt = (retry_cnt * 6);
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_MTP_TS_UPDATE_DISABLE");
            bksync_hostcmd_data_op(1, (u64 *)data0, 0);
            break;
        case BKSYNC_DEINIT:
            retry_cnt = (retry_cnt * 4);
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_DEINIT");
            break;
        case BKSYNC_SYSINFO:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_SYSINFO");
            bksync_hostcmd_data_op(1, (u64 *)data0, (u64 *)data1);
            break;
        case BKSYNC_BROADSYNC:
            subcmd = *((u32 *)data0);
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_BROADSYNC");
            bksync_hostcmd_data_op(1, (u64 *)data0, (u64 *)data1);
            break;
        case BKSYNC_GPIO:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_GPIO");
            bksync_hostcmd_data_op(1, (u64 *)data0, (u64 *)data1);
            break;
        case BKSYNC_EVLOG:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_EVLOG");
            bksync_hostcmd_data_op(1, (u64 *)data0, (u64 *)data1);
            break;
        case BKSYNC_EXTTSLOG:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_EXTTSLOG");
            bksync_hostcmd_data_op(1, (u64 *)data0, (u64 *)data1);
            break;
#ifdef BDE_EDK_SUPPORT
        case BKSYNC_GET_EXTTS_BUFF:
            snprintf(cmd_str, sizeof(cmd_str), "BKSYNC_GET_EXTTS_BUFF");
            bksync_hostcmd_data_op(1, (u64 *)data0, (u64 *)data1);
            break;
#endif
        case BKSYNC_GPIO_PHASEOFFSET:
            snprintf(cmd_str, sizeof(cmd_str), "BKSYNC_GPIO_PHASEOFFSET");
            bksync_hostcmd_data_op(1, (u64 *)data0, (u64 *)data1);
            break;
        default:
            snprintf(cmd_str, sizeof(cmd_str), "KSYNC_XXX");
            break;
    }

#ifdef BDE_EDK_SUPPORT
    ptp_priv->fw_comm->cmd = cmd;
#else
    DEV_WRITE32(ptp_priv, hostcmd_regs[0], ptp_priv->shared_addr->ksyncinit);
#endif

    do {
#ifdef BDE_EDK_SUPPORT
        cmd_status = ptp_priv->fw_comm->cmd;
#else
        DEV_READ32(ptp_priv, hostcmd_regs[0], &cmd_status);
        ptp_priv->shared_addr->ksyncinit = cmd_status;
#endif

        if (cmd_status == BKSYNC_DONE) {
            ret = 0;
            switch (cmd) {
                case BKSYNC_GET_TSTIME:
                case BKSYNC_GETTIME:
#ifndef BDE_EDK_SUPPORT
                    {
                        u64 d0 = 0ULL;
                        u64 d1 = 0ULL;
                        int retry2_cnt = 3;
                        *(u64 *)data0 = 0ULL;
                        *(u64 *)data1 = 0ULL;
                        do {
                            bksync_hostcmd_data_op(0, &d0, &d1);
                            *(u64 *)data0 |= d0;
                            *(u64 *)data1 |= d1;
                            if (((*(u64 *)data0) & 0xFFFFFFFF) && ((*(u64 *)data0)>>32) &&
                                ((*(u64 *)data1) & 0xFFFFFFFF) && ((*(u64 *)data1)>>32)) {
                                break;
                            }
                            retry2_cnt--;
                            ptp_usleep(1);
                        } while (retry2_cnt);
                        if (retry2_cnt == 0)
                            ret = -1;
                    }
                    break;
#else
                    bksync_hostcmd_data_op(0, (u64 *)data0, (u64 *)data1);
                    break;
#endif
                case BKSYNC_BROADSYNC:
                    if ((subcmd == KSYNC_BROADSYNC_BS0_STATUS_GET) ||
                        (subcmd == KSYNC_BROADSYNC_BS1_STATUS_GET)) {
                        bksync_hostcmd_data_op(0, (u64 *)data0, (u64 *)data1);
                    }
                    break;
#ifdef BDE_EDK_SUPPORT
                    /* Get the host ram address from fw.*/
                case BKSYNC_GET_EXTTS_BUFF:
                    bksync_hostcmd_data_op(0, (u64 *)data0, (u64 *)data1);
                    break;
#endif
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
        DBG_ERR(("Timeout on response from R5 to cmd %s time taken %lld us\n", cmd_str, ktime_us_delta(now, start)));
        if (cmd == BKSYNC_GET_TSTIME) {
            DBG_ERR(("2step timestamp get timeout for port:%d seq_id:%d\n", port, seq_id));
        }
    }

    if (debug & DBG_LVL_CMDS) {
        if (ktime_us_delta(now, start) > 5000)
            DBG_CMDS(("R5 Command %s exceeded time expected (%lld us)\n", cmd_str, ktime_us_delta(now, start)));
    }

    DBG_CMDS(("bksync_cmd_go: cmd:%s rv:%d\n", cmd_str, ret));

    return ret;
}

/**
 * bksync_ptp_adjfreq
 *
 * @ptp: pointer to ptp_clock_info structure
 * @ppb: frequency correction value
 *
 * Description: this function will set the frequency correction
 */
static int bksync_ptp_adjfreq(struct ptp_clock_info *ptp, s32 ppb)
{
    int ret = -1;

    ret = bksync_cmd_go(BKSYNC_FREQCOR, &ppb, NULL);
    DBG_VERB(("ptp_adjfreq: applying freq correction: %x; rv:%d\n", ppb, ret));

    return ret;
}

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
    int ret = -1;

    ret = bksync_cmd_go(BKSYNC_ADJTIME, (void *)&delta, NULL);
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
    int ret = -1;
    s64 reftime = 0;
    s64 refctr = 0;
    static u64 prv_reftime = 0, prv_refctr = 0;
    u64 diff_reftime = 0, diff_refctr = 0;

    ret = bksync_cmd_go(BKSYNC_GETTIME, (void *)&reftime, (void *)&refctr);
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
            DBG_WARN(("PTP-GETTIME ptptime: 0x%llx reftime: 0x%llx prv_ptptime: 0x%llx prv_reftime: 0x%llx \n",
                     ptp_priv->shared_addr->ptptime, ptp_priv->shared_addr->reftime, diff_reftime, diff_refctr));
        }
        prv_reftime = reftime;
        prv_refctr = refctr;

        *ts = ns_to_timespec64(reftime);
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
    s64 reftime, phaseadj;
    int ret = -1;

    phaseadj = 0;
    reftime = timespec64_to_ns(ts);

    ret = bksync_cmd_go(BKSYNC_SETTIME, (void *)&reftime, (void *)&phaseadj);
    DBG_VERB(("ptp_settime: settime: 0x%llx; rv:%d\n", reftime, ret));

    return ret;
}

static int bksync_exttslog_cmd(int event, int enable)
{
    int ret;
    u64 subcmd = 0, subcmd_data = 0;

#ifdef BDE_EDK_SUPPORT
    if (NULL == ptp_priv->extts_log) {
        ret = bksync_cmd_go(BKSYNC_GET_EXTTS_BUFF,
                &subcmd, &subcmd_data);
        DBG_VERB(("bksync_exttslog_cmd: Get EXTTS buff: \
                    subcmd_data:0x%llx\n", subcmd_data));

        ptp_priv->extts_log =
            (bksync_fw_extts_log_t *)lkbde_edk_dmamem_map_p2v(subcmd_data);
        if (NULL == ptp_priv->extts_log) {
            DBG_ERR(("EXTTS failed to get virtual addr for the physical address\n"));
        }
    }
#else
    subcmd_data = ptp_priv->extts_dma_mem_addr;
#endif

    /* upper 32b -> event
     * lower 32b -> enable/disable */
    subcmd = (u64)event << 32 | enable;

    ret = bksync_cmd_go(BKSYNC_EXTTSLOG, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_evlog_cmd: subcmd: 0x%llx subcmd_data: 0x%llx rv:%d\n", subcmd, subcmd_data, ret));

    return ret;
}

static int bksync_ptp_enable(struct ptp_clock_info *ptp,
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

                bksync_exttslog_cmd(mapped_event, enable);

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


static int bksync_ptp_mirror_encap_update(struct ptp_clock_info *ptp,
                                          int mtp_idx, int start)
{
    int ret = -1;
    u64 mirror_encap_idx;
    u32 cmd_status;

    if (mtp_idx > BCMKSYNC_MAX_MTP_IDX) {
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

    ret = bksync_cmd_go(cmd_status, &mirror_encap_idx, NULL);
    DBG_VERB(("mirror_encap_update: %d, mpt_index: %d, ret:%d\n", start, mtp_idx, ret));

    return ret;

}

/* structure describing a PTP hardware clock */
static struct ptp_clock_info bksync_ptp_caps = {
    .owner = THIS_MODULE,
    .name = "bksync_ptp_clock",
    .max_adj = 200000,
    .n_alarm = 0,
    .n_ext_ts = NUM_EXT_TS,
    .n_per_out = 0, /* will be overwritten in bksync_ptp_register */
    .n_pins = 0,
    .pps = 0,
    .adjfreq = bksync_ptp_adjfreq,
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
int bksync_ptp_hw_tstamp_enable(int dev_no, int port, int tx_type)
{
    uint64_t portmap = 0;
    int map = 0;
    int ret = 0;

    if (!module_initialized) {
        ret = -1;
        goto exit;
    }

    if (tx_type == HWTSTAMP_TX_ONESTEP_SYNC) {
        DBG_VERB(("hw_tstamp_enable: Enabling 1-step(type:%d) TS on port:%d\n", tx_type, port));
        bksync_ptp_time_keep_init();
        goto exit;
    }

    DBG_VERB(("hw_tstamp_enable: Enabling 2-step(type:%d) TS on port:%d\n", tx_type, port));
    if (port <= 0) {
        DBG_ERR(("hw_tstamp_enable: Error enabling 2-step timestamp on port:%d\n", port));
        ret = -1;
        goto exit;
    }

    /* Update the shared structure member */
    if (ptp_priv->shared_addr) {
        if ((port > 0) && (port < BCMKSYNC_MAX_NUM_PORTS)) {
            port -= 1;
            map = (port / 64);
            port = (port % 64);

            portmap = ptp_priv->shared_addr->portmap[map];
            portmap |= (uint64_t)0x1 << port;
            ptp_priv->shared_addr->portmap[map] = portmap;

            /* Command to R5 for the update */
            ptp_priv->shared_addr->ksyncinit=BKSYNC_PBM_UPDATE;

        }
    }

exit:
    return ret;
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
int bksync_ptp_hw_tstamp_disable(int dev_no, int port, int tx_type)
{
    uint64_t portmap = 0;
    int map = 0;
    int ret = 0;

    if (!module_initialized) {
        ret = -1;
        goto exit;
    }

    if (tx_type == HWTSTAMP_TX_ONESTEP_SYNC) {
        DBG_VERB(("hw_tstamp_disable: Disable 1Step TS(type:%d) port = %d\n", tx_type, port));
        goto exit;
    }

    DBG_VERB(("hw_tstamp_disable: Disable 2Step TS(type:%d) port = %d\n", tx_type, port));
    if (port <= 0) {
        DBG_ERR(("hw_tstamp_disable: Error disabling timestamp on port:%d\n", port));
        ret = -1;
        goto exit;
    }

    /* Update the shared structure member */
    if (ptp_priv->shared_addr) {
        if ((port > 0) && (port < BCMKSYNC_MAX_NUM_PORTS)) {
            port -= 1;
            map = (port / 64);
            port = (port % 64);

            portmap = ptp_priv->shared_addr->portmap[map];
            portmap &= ~((uint64_t)0x1 << port);
            ptp_priv->shared_addr->portmap[map]= portmap;
    
            /* Command to R5 for the update */
            ptp_priv->shared_addr->ksyncinit = BKSYNC_PBM_UPDATE;
        }
    }

exit:
    return ret;
}

int bksync_ptp_transport_get(uint8_t *pkt)
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
bksync_txpkt_tsts_tsamp_get(int port, uint32_t pkt_seq_id, uint32_t *ts_valid, uint32_t *seq_id, uint64_t *timestamp)
{
    int ret = 0;
    uint64_t data=0ULL;
    u32 fifo_rxctr = 0;

    *ts_valid = 0;
    *timestamp = 0ULL;
    *seq_id = 0;

    data = (port & 0xFFFF) | ((pkt_seq_id & 0xFFFF) << 16);

    ret = bksync_cmd_go(BKSYNC_GET_TSTIME, &data, timestamp);
    if (ret >= 0) {
        *ts_valid = data & 0x1ULL;
        *seq_id = (data >> 16) & 0xFFFF;
        fifo_rxctr = (data >> 32) & 0xFFFFFFFF;
        if (*ts_valid) {
            data = (port & 0xFFFF) | ((pkt_seq_id & 0xFFFF) << 16);
            ret = bksync_cmd_go(BKSYNC_ACK_TSTIME, &data, 0ULL);
            if (ret >= 0) {
                if (fifo_rxctr != 0) {
                    if (fifo_rxctr != (ptp_priv->port_stats[port].fifo_rxctr + 1)) {
                        DBG_ERR(("FW reset or lost timestamp FIFO_RxCtr:"
                                    "(Prev %u : Current %u) port:%d pkt_sn:%u hw_sn:%u \n",
                                    ptp_priv->port_stats[port].fifo_rxctr,
                                    fifo_rxctr, port, pkt_seq_id, *seq_id));
                    }
                    ptp_priv->port_stats[port].fifo_rxctr = fifo_rxctr;
                }
            } else {
                DBG_ERR(("BKSYNC_ACK_TSTIME failed on port:%d sn:%d\n", port, pkt_seq_id));
            }
        } else {
            DBG_ERR(("BKSYNC_GET_TSTIME invalid on port:%d pkt_sn:%d fw_sn:%d fifo:%d prev_fifo:%d\n",
                    port, pkt_seq_id, *seq_id, fifo_rxctr,
                    ptp_priv->port_stats[port].fifo_rxctr));
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
int bksync_ptp_hw_tstamp_tx_time_get(int dev_no, int port, uint8_t *pkt, uint64_t *ts, int tx_type)
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

    start = ktime_get();

    if (!ptp_priv || !pkt || !ts || port < 1 || port > 255 || ptp_priv->shared_addr == NULL) {
        return -1;
    }

    *ts = 0;

    /* Linux 5.10.67 kernel complains about missing delay request timestamp for even if
     * configuration is for one-step ptp, hence provided ptp time in skb timestamp
     */
    if (tx_type == HWTSTAMP_TX_ONESTEP_SYNC) {
        if (ptp_priv->ptp_pair_lock == 1) {
            /* use alternate pair when main dataset is being updated */
            *ts = ptp_priv->shared_addr->ptptime_alt;
        } else {
            *ts = ptp_priv->shared_addr->ptptime;
        }
        ptp_priv->port_stats[port].pkt_txctr += 1;
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

    port -= 1;

    DBG_TXTS(("hw_tstamp_tx_time_get: port %d pktseq_id %u\n", port, pktseq_id));

    /* Fetch the TX timestamp from shadow memory */
    do {
        bksync_txpkt_tsts_tsamp_get(port, pktseq_id, &ts_valid, &seq_id, &timestamp);
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
                DBG_TXTS(("Port: %d Skb_SeqID %d FW_SeqId %d and TS:%llx FetchTime %lld retries:%d\n",
                          port, pktseq_id, seq_id, timestamp, delta,
                          (num_retries-retry_cnt)));

                if (delta < ptp_priv->port_stats[port].tsts_best_fetch_time || ptp_priv->port_stats[port].tsts_best_fetch_time == 0) {
                    ptp_priv->port_stats[port].tsts_best_fetch_time = delta;
                }
                if (delta > ptp_priv->port_stats[port].tsts_worst_fetch_time || ptp_priv->port_stats[port].tsts_worst_fetch_time == 0) {
                    ptp_priv->port_stats[port].tsts_worst_fetch_time = delta;
                }
                /* Calculate Moving Average*/
                ptp_priv->port_stats[port].tsts_avg_fetch_time = ((u32)delta + ((ptp_priv->port_stats[port].tsts_match - 1) * ptp_priv->port_stats[port].tsts_avg_fetch_time)) / ptp_priv->port_stats[port].tsts_match;
                break;
            } else {
                DBG_TXTS(("Discard timestamp on port %d Skb_SeqID %d FW_SeqId %d RetryCnt %d TimeLapsed (%lld us)\n",
                          port, pktseq_id, seq_id, (num_retries - retry_cnt), ktime_us_delta(ktime_get(),start)));

                ptp_priv->port_stats[port].tsts_discard += 1;
                continue;
            }
        }
        ptp_usleep(1000);
        retry_cnt--;
    } while(retry_cnt);


    ptp_priv->port_stats[port].pkt_txctr += 1;

    if (retry_cnt == 0) {
        ptp_priv->port_stats[port].tsts_timeout += 1;
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
void bksync_dnxjr2_parse_rxpkt_system_header(uint8_t *raw_pkt_frame,  bksync_dnx_rx_pkt_parse_info_t *rx_pkt_parse_info, int isfirsthdr)
{
    bksync_dnxjr2_ftmh_base_header_t  *ftmh_base_hdr = NULL;
    bksync_dnxjr2_timestamp_header_t  *timestamp_hdr = NULL;
    bksync_dnxjr2_udh_base_header_t   *udh_base_header = NULL;
    bksync_dnxjr2_ftmh_app_spec_ext_1588v2_t *ftmp_app_spec_ext_1588v2_hdr = NULL;
    bksync_dnxjr2_pph_fheiext_vlanedit_3b_header_t *fheiext_vlanedit_3b_hdr = NULL;
    bksync_dnxjr2_pph_fheiext_vlanedit_5b_header_t *fheiext_vlanedit_5b_hdr = NULL;
    bksync_dnxjr2_pph_base_12b_header_t *pph_base_12b_hdr = NULL;
    uint8_t     raw_frame[64];
    int tmp = 0;


    rx_pkt_parse_info->rx_frame_len = 0;
    rx_pkt_parse_info->dnx_header_offset = 0;
    rx_pkt_parse_info->pph_header_vlan = 0;
    rx_pkt_parse_info->rx_hw_timestamp = 0;
    rx_pkt_parse_info->src_sys_port = 0;

    for (tmp = 0; tmp < 64; tmp++) {
        raw_frame[tmp] = raw_pkt_frame[tmp];
    }

    /* FTMH */
    ftmh_base_hdr = (bksync_dnxjr2_ftmh_base_header_t *)(&raw_frame[rx_pkt_parse_info->dnx_header_offset]);
    ftmh_base_hdr->words[0] = ntohl(ftmh_base_hdr->words[0]);
    ftmh_base_hdr->words[1] = ntohl(ftmh_base_hdr->words[1]);

    rx_pkt_parse_info->src_sys_port = (uint16_t)(ftmh_base_hdr->src_sys_port_aggr_0 << 9 | ftmh_base_hdr->src_sys_port_aggr_1 << 1 | ftmh_base_hdr->src_sys_port_aggr_2);
    rx_pkt_parse_info->rx_frame_len = (uint16_t)(ftmh_base_hdr->packet_size_0 << 6 | ftmh_base_hdr->packet_size_1);

    rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_FTMH_HDR_LEN;

    /* FTMH LB-Key Extension */
    if ((ptp_priv->bksync_jr2devs_sys_info).ftmh_lb_key_ext_size > 0) {
        rx_pkt_parse_info->dnx_header_offset += (ptp_priv->bksync_jr2devs_sys_info).ftmh_lb_key_ext_size;
    }

    /* FTMH Stacking Extension */
    if ((ptp_priv->bksync_jr2devs_sys_info).ftmh_stacking_ext_size > 0) {
        rx_pkt_parse_info->dnx_header_offset += (ptp_priv->bksync_jr2devs_sys_info).ftmh_stacking_ext_size;
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
        ftmp_app_spec_ext_1588v2_hdr = (bksync_dnxjr2_ftmh_app_spec_ext_1588v2_t*)(&raw_frame[rx_pkt_parse_info->dnx_header_offset]);
        ftmp_app_spec_ext_1588v2_hdr->word = ntohl(ftmp_app_spec_ext_1588v2_hdr->word);

        if (ftmp_app_spec_ext_1588v2_hdr->type == bksync_dnxjr2_ftmh_app_spec_ext_type_1588v2) {
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

        timestamp_hdr = (bksync_dnxjr2_timestamp_header_t* )(&raw_frame[rx_pkt_parse_info->dnx_header_offset]);
        timestamp_hdr->word = ntohl(timestamp_hdr->word);

        rx_pkt_parse_info->rx_hw_timestamp = timestamp_hdr->timestamp;
        rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_TSH_HDR_SIZE;

    }

    /* PPH - internal header */
    if ((ftmh_base_hdr->pph_type == BKSYNC_DNXJR2_PPH_TYPE_PPH_BASE) ||
        (ftmh_base_hdr->pph_type == BKSYNC_DNXJR2_PPH_TYPE_PPH_BASE_TSH)) {

        pph_base_12b_hdr = (bksync_dnxjr2_pph_base_12b_header_t*)(&raw_frame[rx_pkt_parse_info->dnx_header_offset]);

        pph_base_12b_hdr->word[0] = ntohl(pph_base_12b_hdr->word[0]);
        pph_base_12b_hdr->word[1] = ntohl(pph_base_12b_hdr->word[1]);
        pph_base_12b_hdr->word[2] = ntohl(pph_base_12b_hdr->word[2]);

        rx_pkt_parse_info->dnx_header_offset += (ptp_priv->bksync_jr2devs_sys_info).pph_base_size;


        /* PPH fhei_size handling */
        if (pph_base_12b_hdr->fhei_size > BKSYNC_DNXJR2_PPH_FHEI_TYPE_NONE) {

            switch(pph_base_12b_hdr->fhei_size) {
                case BKSYNC_DNXJR2_PPH_FHEI_TYPE_SZ0: /* 3byte */
                    fheiext_vlanedit_3b_hdr = (bksync_dnxjr2_pph_fheiext_vlanedit_3b_header_t*)(&raw_frame[rx_pkt_parse_info->dnx_header_offset]);

                    if(fheiext_vlanedit_3b_hdr->type == bksync_dnxjr2_pph_fheiext_type_vlanedit) {
                        rx_pkt_parse_info->pph_header_vlan = fheiext_vlanedit_3b_hdr->edit_vid1_0 << 7 | fheiext_vlanedit_3b_hdr->edit_vid1_1;
                    }
                    rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_PPH_FHEI_SZ0_SIZE;
                    break;
                case BKSYNC_DNXJR2_PPH_FHEI_TYPE_SZ1: /* 5byte */
                    fheiext_vlanedit_5b_hdr = (bksync_dnxjr2_pph_fheiext_vlanedit_5b_header_t*)(&raw_frame[rx_pkt_parse_info->dnx_header_offset]);

                    if (fheiext_vlanedit_5b_hdr->type == bksync_dnxjr2_pph_fheiext_type_vlanedit) {
                        rx_pkt_parse_info->pph_header_vlan = fheiext_vlanedit_5b_hdr->edit_vid1_0 << 7 | fheiext_vlanedit_5b_hdr->edit_vid1_1;
                    } else if (fheiext_vlanedit_5b_hdr->type == bksync_dnxjr2_pph_fheiext_type_trap_snoop_mirror) {
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
            rx_pkt_parse_info->dnx_header_offset += (ptp_priv->bksync_jr2devs_sys_info).pph_lif_ext_size[pph_base_12b_hdr->lif_ext_type];
        }

        /* PPH Learn Extension */
        if (pph_base_12b_hdr->learn_ext_present) {
            rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_PPH_LEARN_EXT_SIZE;
        }
    }

    /* UDH header */
    if (!isfirsthdr) {
    if ((ptp_priv->bksync_jr2devs_sys_info).udh_enable) {
        udh_base_header = (bksync_dnxjr2_udh_base_header_t* )(&raw_frame[rx_pkt_parse_info->dnx_header_offset]);

        rx_pkt_parse_info->dnx_header_offset += BKSYNC_DNXJR2_UDH_BASE_HEADER_LEN;
        /* Need to understand more */
        rx_pkt_parse_info->dnx_header_offset += (ptp_priv->bksync_jr2devs_sys_info).udh_data_lenght_per_type[udh_base_header->udh_data_type_0];
        rx_pkt_parse_info->dnx_header_offset += (ptp_priv->bksync_jr2devs_sys_info).udh_data_lenght_per_type[udh_base_header->udh_data_type_1];
        rx_pkt_parse_info->dnx_header_offset += (ptp_priv->bksync_jr2devs_sys_info).udh_data_lenght_per_type[udh_base_header->udh_data_type_2];
        rx_pkt_parse_info->dnx_header_offset += (ptp_priv->bksync_jr2devs_sys_info).udh_data_lenght_per_type[udh_base_header->udh_data_type_3];
    }
    }

    DBG_RX(("DNX PKT PARSE: src_sys_port %u rx_hw_timestamp %llx pph_header_vlan %llx dnx_header_offset %u rx_frame_len %d\n",
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
    BKSYNC_UNPACK_U32(custom_hdr, seq_id);
    tot_len = len;

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

    return (tot_len);
}

/**
 * bksync_ptp_hw_tstamp_rx_pre_process
 *
 * @dev_no: device number
 *
 * Description:
 */
int bksync_ptp_hw_tstamp_rx_pre_process(int dev_no, uint8_t *pkt, uint32_t sspa, int *pkt_offset)
{
    int ret = -1;
    uint64_t ts;
    int custom_encap_len = 0;

    bksync_dnx_rx_pkt_parse_info_t rx_pkt_parse_info_1 =  {0}, rx_pkt_parse_info_2 = {0};

    if (sspa == (ptp_priv->bksync_init_info).uc_port_num  && pkt_offset == NULL) {
        ret = 0;
    }


    if (DEVICE_IS_DNX && pkt_offset == NULL) {
        bksync_dnxjr2_parse_rxpkt_system_header(pkt,  &rx_pkt_parse_info_1, 1);
        bksync_dnxjr2_parse_rxpkt_system_header(pkt + rx_pkt_parse_info_1.dnx_header_offset,  &rx_pkt_parse_info_2, 0);
    }


    /* parse custom encap header in pkt for ptp rxtime */
    if (DEVICE_IS_DNX) {
        custom_encap_len = bksync_pkt_custom_encap_ptprx_get(pkt + rx_pkt_parse_info_1.dnx_header_offset
                + rx_pkt_parse_info_2.dnx_header_offset, &ts);
    } else {
        custom_encap_len = bksync_pkt_custom_encap_ptprx_get(pkt, &ts);
    }
    DBG_RX(("hw_tstamp_rx_pre_process: sspa:0x%x cust_encap_len=0x%x\n", sspa, custom_encap_len));

    if ((pkt_offset) && (custom_encap_len >= 0)) {
        *pkt_offset = custom_encap_len;
        ret = 0;
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
int bksync_ptp_hw_tstamp_rx_time_upscale(int dev_no, int port, struct sk_buff *skb, uint32_t *meta, uint64_t *ts)
{
    int ret = 0;
    int custom_encap_len = 0;
    uint16_t tpid = 0;
    uint16_t msgtype_offset = 0;
    int transport = network_transport;
    int ptp_hdr_offset = 0, ptp_message_len = 0;

    if (!module_initialized || !ptp_priv || (ptp_priv->shared_addr == NULL)) {
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
        tpid = SKB_U16_GET(skb, (12));
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

        ptp_message_len = SKB_U16_GET(skb, (ptp_hdr_offset + 2));

        DBG_RX(("rxtime_upscale: custom_encap_len %d tpid 0x%x transport %d skb->len %d ptp message type %d, ptp_message_len %d\n",
                custom_encap_len, tpid, transport, skb->len, skb->data[msgtype_offset] & 0x0F, ptp_message_len));

        /* Remove padding ,CRC from from L2 packet from returning to Linux Stack */
        if (DEVICE_IS_DNX && (transport == 2) ) {
            skb_trim(skb, ptp_hdr_offset + ptp_message_len);
        }
    }

    if ((port > 0) && (port < BCMKSYNC_MAX_NUM_PORTS)) {
        port -= 1;
        ptp_priv->port_stats[port].pkt_rxctr += 1;
    }

    return ret;
}


void bksync_hton64(u8 *buf, const uint64_t *data)
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
bksync_dnx_ase1588_tsh_hdr_update_ipv6(struct sk_buff *skb, int hwts, int encap_type, int ptp_hdr_offset)
{
    /* Module Hdr [16] + PTCH [2] + ITMH [5] + ASE1588 [6] + TSH [4] + Internal Hdr [12] + UDH base [1] */

    /* For JR3 for CF update 1588v2_Offset should also have system_header length of
     * PTCH [2] + ITMH [5] + ASE1588 [6] + TSH [4] + Internal Hdr [12] + UDH base [1] = 30. */
    if ((ptp_priv->bksync_init_info).application_v2) {
        ptp_hdr_offset = 30;
    } else {
        ptp_hdr_offset = 29;
    }

    switch(encap_type)
    {
        case 2: /* IEEE 802.3 */
            ptp_hdr_offset += 18;
            break;
        case 4: /* UDP IPv4   */
            ptp_hdr_offset += 46;
            break;
        case 6: /* UDP IPv6   */
            ptp_hdr_offset += 66;
            break;
        default:
            ptp_hdr_offset += 46;
            break;
    }

    /* ITMH */
    /* App Specific Ext Present  ASE 1588*/
    skb->data [18] |= (0x1 << 3);

    /* PPH_TYPE - TSH  +  Internal Hdr */
    skb->data [18] |= (0x3 << 1);   /* TSH + PPH Only */

    /* ASE 1588 ext */
    skb->data [23] = skb->data [24] = skb->data [25] = skb->data [26] = 0x00;

    skb->data[27] = skb->data[28] = 0;

    /* OTSH.encap_type = udp vs non-udp  - 1bit (15:15) */
    /* encap type - 2 L2, 4 & 6 UDP */
    skb->data[27] |= (((encap_type == 2) ? 1 : 0) << 7);

    /* ASE1588 1588v2 command -  one step or two step  3bit (14:12) */
    /* ASE1588 1588v2 command should be zero for CF update */

    /*  offset to start of 1588v2 frame  - 8 bit (11:4) */
    skb->data [27] = skb->data [27] | ((ptp_hdr_offset) & 0xf0) >> 4;
    skb->data [28] = ((ptp_hdr_offset) & 0xf) << 4;

    /* ASE1588 type = 1588v2  - 4 bit (0:3) */
    skb->data [28] = skb->data [28] | 0x01;

    skb->data [29] = skb->data [30] = skb->data [31] = skb->data [32] = 0x00;

    /* Internal Header */
    skb->data [33] = skb->data [34] = skb->data [35] = skb->data [36] = 0x00;
    skb->data [37] = skb->data [38] = skb->data [39] = skb->data [40] = 0x00;
    skb->data [41] = skb->data [42] = skb->data [43] = skb->data [44] = 0x00;

    skb->data [44] = 0x42;
    skb->data [43] = 0x07;
    skb->data [42] = 0x10;

    /* UDH Base Hdr */
    skb->data [45] = 0;

    return;
}

static void
bksync_dnx_ase1588_tsh_hdr_update(struct sk_buff *skb, int hwts, int encap_type, int ptp_hdr_offset)
{
    /* Module Hdr [16] + PTCH [2] + ITMH [5] + ASE1588 [6] + TSH [4] */

    /* For JR3 for CF update 1588v2_Offset should also have system_header length of
     * PTCH [2] + ITMH [5] + ASE1588 [6] + TSH [4] = 17. */
    if ((ptp_priv->bksync_init_info).application_v2) {
        ptp_hdr_offset = ptp_hdr_offset + 17;
    }

    /* ITMH */
    /* App Specific Ext Present */
    skb->data [18] |= (1 << 3);

    /* PPH_TYPE - TSH */
    skb->data [18] |= (0x2 << 1);

    /* ASE 1588 ext */
    skb->data [23] = skb->data [24] = skb->data [25] = skb->data [26] = 0x00;

    skb->data[27] = skb->data[28] = 0;

    /* OTSH.encap_type = udp vs non-udp  - 1bit (15:15) */
    /* encap type - 2 L2, 4 & 6 UDP */
    skb->data[27] |= (((encap_type == 2) ? 1 : 0) << 7);

    /* ASE1588 1588v2 command -  one step or two step  3bit (14:12) */
    switch (hwts) {
        case HWTSTAMP_TX_ONESTEP_SYNC:
            skb->data[27] |= ((0x1) << 4);
            break;
        default:
            skb->data[27] |= ((0x2) << 4);
            break;
    }

    /*  offset to start of 1588v2 frame  - 8 bit (11:4) */
    skb->data [27] = skb->data [27] | ((ptp_hdr_offset) & 0xf0) >> 4;
    skb->data [28] = ((ptp_hdr_offset) & 0xf) << 4;

    /* ASE1588 type = 1588v2  - 4 bit (0:3) */
    skb->data [28] = skb->data [28] | 0x01;

    /* TSH Timestamp: 0x0 */
    skb->data [29] = skb->data [30] = skb->data [31] = skb->data [32] = 0x00;

    return;
}



int bksync_ptp_hw_tstamp_tx_meta_get(int dev_no,
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
    int64_t corrField;
    int32_t negCurTS32;
    int64_t negCurTS64;

    if (!module_initialized || !ptp_priv || (ptp_priv->shared_addr == NULL)) {
        return 0;
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

    if (CMICX_DEV_TYPE || DEVICE_IS_DPP) {
        pkt_offset = ptp_hdr_offset = hdrlen;
    }

    /* Need to check VLAN tag if packet is tagged */
    tpid = SKB_U16_GET(skb, (pkt_offset + 12));
    if (tpid == 0x8100) {
        md_offset = 4;
        ptp_hdr_offset += 4;

        if (DEVICE_IS_DNX && vnptp_l2hdr_vlan_prio != 0) {
            ethertype =  SKB_U16_GET(skb, hdrlen + 12 + 4);
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

    if (DEVICE_IS_DPP && (hdrlen > (BKN_DNX_PTCH_2_SIZE))) {
        DBG_TX_DUMP(("hw_tstamp_tx_meta_get: Before OTSH updates\n"));
        if (debug & DBG_LVL_TX_DUMP) dbg_dump_pkt(skb->data, skb->len);

        DBG_TX(("hw_tstamp_tx_meta_get: Before: ptch[0]: 0x%x ptch[1]: 0x%x itmh[0]: 0x%x "
                  "oam-ts[0]: 0x%x pkt[0]:0x%x\n", skb->data[0], skb->data[1], skb->data[2],
                  skb->data[6], skb->data[12]));

        bksync_dpp_otsh_update(skb, hwts, transport, (ptp_hdr_offset - pkt_offset));

        DBG_TX(("hw_tstamp_tx_meta_get: After : ptch[0]: 0x%x itmh[0]: 0x%x oam-ts[0]: 0x%x "
                  "pkt[0]:0x%x\n", skb->data[0], skb->data[2], skb->data[6], skb->data[12]));

        DBG_TX_DUMP(("hw_tstamp_tx_meta_get: After OTSH updates\n"));
        if (debug & DBG_LVL_TX_DUMP) dbg_dump_pkt(skb->data, skb->len);
    } else if (DEVICE_IS_DNX && (hdrlen > (BKN_DNX_PTCH_2_SIZE))) {

        switch(transport)
        {
            case 6: /* UDP IPv6   */
                bksync_dnx_ase1588_tsh_hdr_update_ipv6(skb, hwts, transport, (ptp_hdr_offset - pkt_offset));
                break;
            case 4: /* UDP IPv4   */
            case 2: /* IEEE 802.3 */
            default:
                bksync_dnx_ase1588_tsh_hdr_update(skb, hwts, transport, (ptp_hdr_offset - pkt_offset));
                break;
        }
    }

    DBG_TX(("hw_tstamp_tx_meta_get: ptptime: 0x%llx ptpcounter: 0x%llx\n", ptptime, ptpcounter));

    DBG_TX(("hw_tstamp_tx_meta_get: ptpmessage type: 0x%x hwts: %d\n", skb->data[ptp_hdr_offset] & 0x0f, hwts));


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
            *tstamp = ptptime;
        }

        DBG_TX(("hw_tstamp_tx_meta_get: ptp msg type %d packet tstamp : 0x%llx corrField: 0x%llx\n",
                 (skb->data[ptp_hdr_offset] & 0x0F), ptptime, corrField));

        port = KNET_SKB_CB(skb)->port;
        if ((port > 0) && (port < BCMKSYNC_MAX_NUM_PORTS)) {
            port -= 1;
            ptp_priv->port_stats[port].pkt_txonestep += 1;
        }
    }

    DBG_TX_DUMP(("hw_tstamp_tx_meta_get: PTP Packet\n"));
    if (debug & DBG_LVL_TX_DUMP) dbg_dump_pkt(skb->data, skb->len);

    return 0;
}


int bksync_ptp_hw_tstamp_ptp_clock_index_get(int dev_no)
{
    int phc_index = -1;

    if (!module_initialized || !ptp_priv) {
        return phc_index;
    }

    if (ptp_priv && ptp_priv->ptp_clock)
        phc_index =  ptp_clock_index(ptp_priv->ptp_clock);

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

    /* Call bcm_ptp_gettime function to keep the ref_time_64 and ref_counter_48 in sync */
    bksync_ptp_gettime(&(priv->ptp_caps), &ts);
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

    if (!module_initialized || ptp_priv->extts_log == NULL)
        goto exit;

    if (ptp_priv->extts_log->overflow) {
        DBG_VERB(("EXTTS queue overflow\n"));
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
            memset((void *)&(ptp_priv->extts_log->event_ts[head]), 0, sizeof(ptp_priv->extts_log->event_ts[head]));

            ptp_priv->extts_event.head = head;
            ptp_priv->extts_log->head = head;

            head = (head + 1) % NUM_EVENT_TS;
            continue;
        }

        event.type = PTP_CLOCK_EXTTS;
        event.index = event_id;
        event.timestamp = ((s64)ptp_priv->extts_log->event_ts[head].tstamp.sec * 1000000000) + ptp_priv->extts_log->event_ts[head].tstamp.nsec;
        ptp_clock_event(ptp_priv->ptp_clock, &event);

        ptp_priv->extts_event.head = head;
        ptp_priv->extts_log->head = head;

        head = (head + 1) % NUM_EVENT_TS;
    }
exit:
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

static int bksync_ptp_init(struct ptp_clock_info *ptp)
{
    int ret = -1;
    u64 subcmd, subcmd_data;

    ret = bksync_cmd_go(BKSYNC_INIT, NULL, NULL);
    DBG_VERB(("bksync_ptp_init: BKSYNC_INIT; rv:%d\n", ret));
    if (ret < 0) goto err_exit;
    ptp_sleep(1);

    if (!DEVICE_IS_DPP && !DEVICE_IS_DNX) {
        return 0;
    }

    subcmd = KSYNC_SYSINFO_UC_PORT_NUM;
    subcmd_data = (ptp_priv->bksync_init_info).uc_port_num;
    ret = bksync_cmd_go(BKSYNC_SYSINFO, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_ptp_init: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));
    if (ret < 0) goto err_exit;

    subcmd = KSYNC_SYSINFO_UC_PORT_SYSPORT;
    subcmd_data = (ptp_priv->bksync_init_info).uc_port_sysport;
    ret = bksync_cmd_go(BKSYNC_SYSINFO, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_ptp_init: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));
    if (ret < 0) goto err_exit;

    subcmd = KSYNC_SYSINFO_HOST_CPU_PORT;
    subcmd_data = (ptp_priv->bksync_init_info).host_cpu_port;
    ret = bksync_cmd_go(BKSYNC_SYSINFO, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_ptp_init: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));
    if (ret < 0) goto err_exit;

    subcmd = KSYNC_SYSINFO_HOST_CPU_SYSPORT;
    subcmd_data = (ptp_priv->bksync_init_info).host_cpu_sysport;
    ret = bksync_cmd_go(BKSYNC_SYSINFO, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_ptp_init: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));
    if (ret < 0) goto err_exit;

    subcmd = KSYNC_SYSINFO_UDH_LEN;
    subcmd_data = (ptp_priv->bksync_init_info).udh_len;
    ret = bksync_cmd_go(BKSYNC_SYSINFO, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_ptp_init: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));
    if (ret < 0) goto err_exit;


err_exit:
    return ret;
}

static int bksync_ptp_deinit(struct ptp_clock_info *ptp)
{
    int ret = -1;

    bksync_ptp_time_keep_deinit();

    ret = bksync_cmd_go(BKSYNC_DEINIT, NULL, NULL);
    DBG_VERB(("bksync_ptp_deinit: rv:%d\n", ret));

    return ret;
}

static int bksync_broadsync_cmd(int bs_id)
{
    int ret = -1;
    u64 subcmd, subcmd_data;

    subcmd = (bs_id == 0) ? KSYNC_BROADSYNC_BS0_CONFIG : KSYNC_BROADSYNC_BS1_CONFIG;

    subcmd_data =  ((ptp_priv->bksync_bs_info[bs_id]).enable & 0x1);
    subcmd_data |= (((ptp_priv->bksync_bs_info[bs_id]).mode & 0x1) << 8);
    subcmd_data |= ((ptp_priv->bksync_bs_info[bs_id]).hb << 16);
    subcmd_data |= (((u64)(ptp_priv->bksync_bs_info[bs_id]).bc) << 32);

    ret = bksync_cmd_go(BKSYNC_BROADSYNC, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_broadsync_cmd: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));

    return ret;
}

static int bksync_broadsync_status_cmd(int bs_id, u64 *status)
{
    int ret = -1;
    u64 subcmd;

    subcmd = (bs_id == 0) ? KSYNC_BROADSYNC_BS0_STATUS_GET : KSYNC_BROADSYNC_BS1_STATUS_GET;

    ret = bksync_cmd_go(BKSYNC_BROADSYNC, &subcmd, status);
    DBG_VERB(("bksync_broadsync_status_cmd: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, *status, ret));

    return ret;
}

static int bksync_gpio_cmd(int gpio_num)
{
    int ret = -1;
    u64 subcmd, subcmd_data;

    switch (gpio_num) {
        case 0:
            subcmd = KSYNC_GPIO_0;
            break;
        case 1:
            subcmd = KSYNC_GPIO_1;
            break;
        case 2:
            subcmd = KSYNC_GPIO_2;
            break;
        case 3:
            subcmd = KSYNC_GPIO_3;
            break;
        case 4:
            subcmd = KSYNC_GPIO_4;
            break;
        case 5:
            subcmd = KSYNC_GPIO_5;
            break;
        default:
            return ret;
    }

    subcmd_data =  ((ptp_priv->bksync_gpio_info[gpio_num]).enable & 0x1);
    subcmd_data |= (((ptp_priv->bksync_gpio_info[gpio_num]).mode & 0x1) << 8);
    subcmd_data |= ((u64)((ptp_priv->bksync_gpio_info[gpio_num]).period) << 16);

    ret = bksync_cmd_go(BKSYNC_GPIO, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_gpio_cmd: subcmd: 0x%llx subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));

    return ret;
}

static int bksync_gpio_phaseoffset_cmd(int gpio_num)
{
    int ret = -1;
    u64 subcmd, subcmd_data;

    switch (gpio_num) {
        case 0:
            subcmd = KSYNC_GPIO_0;
            break;
        case 1:
            subcmd = KSYNC_GPIO_1;
            break;
        case 2:
            subcmd = KSYNC_GPIO_2;
            break;
        case 3:
            subcmd = KSYNC_GPIO_3;
            break;
        case 4:
            subcmd = KSYNC_GPIO_4;
            break;
        case 5:
            subcmd = KSYNC_GPIO_5;
            break;
        default:
            return ret;
    }

    subcmd_data = (ptp_priv->bksync_gpio_info[gpio_num]).phaseoffset;
    ret = bksync_cmd_go(BKSYNC_GPIO_PHASEOFFSET, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_gpio_phaseoffset_cmd: subcmd: 0x%llx "
              "subcmd_data: 0x%llx; rv:%d\n", subcmd, subcmd_data, ret));

    return ret;
}



static int bksync_evlog_cmd(int event, int enable)
{
    int ret;
    int addr_offset;
    u64 subcmd = 0, subcmd_data = 0;
    bksync_evlog_t tmp;

    subcmd = event;
    addr_offset = ((u8 *)&(tmp.event_timestamps[event]) - (u8 *)&(tmp.event_timestamps[0]));

    if (enable) {
        subcmd_data = (ptp_priv->dma_mem + addr_offset);
    } else {
        subcmd_data = 0;
    }

    ret = bksync_cmd_go(BKSYNC_EVLOG, &subcmd, &subcmd_data);
    DBG_VERB(("bksync_evlog_cmd: subcmd: 0x%llx subcmd_data: 0x%llx rv:%d\n", subcmd, subcmd_data, ret));

    return ret;
}


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
   /* beginning a new sequence ? */
   if ( (int)*pos == 0 && ptp_priv->shared_addr != NULL)
   {
        seq_printf(s, "TwoStep Port Bitmap : %08llx%08llx\n",
                      (uint64_t)(ptp_priv->shared_addr->portmap[1]),
                      (uint64_t)(ptp_priv->shared_addr->portmap[0]));
        seq_printf(s,"%4s| %9s| %9s| %9s| %9s| %9s| %9s| %9s| %9s| %9s| %9s| %9s\n",
                     "Port", "RxCounter", "TxCounter", "TxOneStep", "TSTimeout", "TSRead", "TSMatch", "TSDiscard",
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
    unsigned long port = (unsigned long)v;

    if ((port > 0) && (port < BCMKSYNC_MAX_NUM_PORTS)) {

    port = port - 1;
    if (ptp_priv->port_stats[port].pkt_rxctr || ptp_priv->port_stats[port].pkt_txctr ||
        ptp_priv->port_stats[port].pkt_txonestep||
        ptp_priv->port_stats[port].tsts_discard || ptp_priv->port_stats[port].tsts_timeout ||
        ptp_priv->shared_addr->port_ts_data[port].ts_cnt || ptp_priv->port_stats[port].tsts_match) {
          seq_printf(s, "%4lu | %9d| %9d| %9d| %9d| %9d| %9d| %9d| %9lld| %9lld | %9d|%9d | %s\n", (port + 1),
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
                ptp_priv->port_stats[port].pkt_txctr != ptp_priv->port_stats[port].tsts_match ? "***":"");
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
    return 0;
}

static int
bksync_proc_cleanup(void)
{
    remove_proc_entry("stats", bksync_proc_root);
    remove_proc_entry("debug", bksync_proc_root);
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

    if (ATTRCMP(bs0)) {
        ret = sscanf(buf, "enable:%d mode:%d bc:%u hb:%u", &enable, &mode, &bc, &hb);
        DBG_VERB(("rd:%d bs0: enable:%d mode:%d bc:%d hb:%d\n", rd_iter++, enable, mode, bc, hb));
        ptp_priv->bksync_bs_info[0].enable = enable;
        ptp_priv->bksync_bs_info[0].mode = mode;
        ptp_priv->bksync_bs_info[0].bc   = bc;
        ptp_priv->bksync_bs_info[0].hb   = hb;

        (void)bksync_broadsync_cmd(0);
    } else if (ATTRCMP(bs1)) {
        ret = sscanf(buf, "enable:%d mode:%d bc:%u hb:%u", &enable, &mode, &bc, &hb);
        DBG_VERB(("rd:%d bs1: enable:%d mode:%d bc:%d hb:%d\n", rd_iter++, enable, mode, bc, hb));
        ptp_priv->bksync_bs_info[1].enable = enable;
        ptp_priv->bksync_bs_info[1].mode = mode;
        ptp_priv->bksync_bs_info[1].bc   = bc;
        ptp_priv->bksync_bs_info[1].hb   = hb;

        (void)bksync_broadsync_cmd(1);
    } else {
        ret = -ENOENT;
    }

    return (ret == -ENOENT) ? ret : bytes;
}

static ssize_t bs_attr_show(struct kobject *kobj,
                            struct kobj_attribute *attr,
                            char *buf)
{
    ssize_t bytes;
    u64 status = 0;
    u32 variance = 0;

    if (ATTRCMP(bs0)) {

        if(ptp_priv->bksync_bs_info[0].enable) {
            (void)bksync_broadsync_status_cmd(0, &status);
        }

        variance = (status >> 32);
        status = (status & 0xFFFFFFFF);
        bytes = sprintf(buf, "enable:%d mode:%d bc:%u hb:%u status:%u(%u)\n",
                        ptp_priv->bksync_bs_info[0].enable,
                        ptp_priv->bksync_bs_info[0].mode,
                        ptp_priv->bksync_bs_info[0].bc,
                        ptp_priv->bksync_bs_info[0].hb,
                        (u32)status,
                        variance);
        DBG_VERB(("wr:%d bs0: enable:%d mode:%d bc:%u hb:%u status:%u(%u)\n",
                        wr_iter++,
                        ptp_priv->bksync_bs_info[0].enable,
                        ptp_priv->bksync_bs_info[0].mode,
                        ptp_priv->bksync_bs_info[0].bc,
                        ptp_priv->bksync_bs_info[0].hb,
                        (u32)status,
                        variance));
    } else if (ATTRCMP(bs1)) {

        if(ptp_priv->bksync_bs_info[1].enable) {
            (void)bksync_broadsync_status_cmd(1, &status);
        }

        variance = (status >> 32);
        status = (status & 0xFFFFFFFF);
        bytes = sprintf(buf, "enable:%d mode:%d bc:%u hb:%u status:%u(%u)\n",
                        ptp_priv->bksync_bs_info[1].enable,
                        ptp_priv->bksync_bs_info[1].mode,
                        ptp_priv->bksync_bs_info[1].bc,
                        ptp_priv->bksync_bs_info[1].hb,
                        (u32)status,
                        variance);
        DBG_VERB(("wr:%d bs1: enable:%d mode:%d bc:%u hb:%u status:%u(%u)\n",
                        wr_iter++,
                        ptp_priv->bksync_bs_info[1].enable,
                        ptp_priv->bksync_bs_info[1].mode,
                        ptp_priv->bksync_bs_info[1].bc,
                        ptp_priv->bksync_bs_info[1].hb,
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


    ret = sscanf(buf, "enable:%d mode:%d period:%u phaseoffset:%lld", &enable, &mode, &period, &phaseoffset);
    DBG_VERB(("rd:%d gpio%d: enable:%d mode:%d period:%d phaseoffset:%lld\n", gpio_rd_iter++, gpio, enable, mode, period, phaseoffset));
    ptp_priv->bksync_gpio_info[gpio].enable = enable;
    ptp_priv->bksync_gpio_info[gpio].mode = mode;
    ptp_priv->bksync_gpio_info[gpio].period   = period;

   (void)bksync_gpio_cmd(gpio);

   if (ptp_priv->bksync_gpio_info[gpio].phaseoffset != phaseoffset) {
       ptp_priv->bksync_gpio_info[gpio].phaseoffset = phaseoffset;
       (void)bksync_gpio_phaseoffset_cmd(gpio);
   }

    return (ret == -ENOENT) ? ret : bytes;
}

static ssize_t gpio_attr_show(struct kobject *kobj,
                              struct kobj_attribute *attr,
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
                    ptp_priv->bksync_gpio_info[gpio].enable,
                    ptp_priv->bksync_gpio_info[gpio].mode,
                    ptp_priv->bksync_gpio_info[gpio].period,
                    ptp_priv->bksync_gpio_info[gpio].phaseoffset);
    DBG_VERB(("wr:%d gpio%d: enable:%d mode:%d period:%u phaseoffset:%lld\n",
                    gpio_wr_iter++, gpio,
                    ptp_priv->bksync_gpio_info[gpio].enable,
                    ptp_priv->bksync_gpio_info[gpio].mode,
                    ptp_priv->bksync_gpio_info[gpio].period,
                    ptp_priv->bksync_gpio_info[gpio].phaseoffset));

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
    ptp_priv->bksync_evlog_info[event].enable = enable;

    return (ret == -ENOENT) ? ret : bytes;
}

static ssize_t evlog_attr_show(struct kobject *kobj,
                               struct kobj_attribute *attr,
                               char *buf)
{
    ssize_t bytes;
    int event;


    if (!module_initialized || !ptp_priv || (ptp_priv->evlog == NULL)) {
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
                    ptp_priv->bksync_evlog_info[event].enable,
                    ptp_priv->evlog->event_timestamps[event].prv_tstamp.sec,
                    ptp_priv->evlog->event_timestamps[event].prv_tstamp.nsec,
                    ptp_priv->evlog->event_timestamps[event].cur_tstamp.sec,
                    ptp_priv->evlog->event_timestamps[event].cur_tstamp.nsec);
    DBG_VERB(("event%d: enable:%d Previous Time:%llu.%09u Latest Time:%llu.%09u\n",
                    event,
                    ptp_priv->bksync_evlog_info[event].enable,
                    ptp_priv->evlog->event_timestamps[event].prv_tstamp.sec,
                    ptp_priv->evlog->event_timestamps[event].prv_tstamp.nsec,
                    ptp_priv->evlog->event_timestamps[event].cur_tstamp.sec,
                    ptp_priv->evlog->event_timestamps[event].cur_tstamp.nsec));

    memset((void *)&(ptp_priv->evlog->event_timestamps[event]), 0, sizeof(ptp_priv->evlog->event_timestamps[event]));

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
bksync_sysfs_init(void)
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
bksync_sysfs_cleanup(void)
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


static void bksync_ptp_fw_data_alloc(void)
{
    dma_addr_t dma_mem = 0;

    /* Initialize the Base address for CMIC and shared Memory access */
    ptp_priv->base_addr = lkbde_get_dev_virt(0);
    ptp_priv->dma_dev = lkbde_get_dma_dev(0);

    ptp_priv->dma_mem_size = sizeof(bksync_evlog_t); /*sizeof(bksync_evlog_t);*/

    if (ptp_priv->evlog == NULL) {
        DBG_ERR(("Allocate memory for event log\n"));
        ptp_priv->evlog = DMA_ALLOC_COHERENT(ptp_priv->dma_dev,
                                                   ptp_priv->dma_mem_size,
                                                   &dma_mem);
        if (ptp_priv->evlog != NULL) {
            ptp_priv->dma_mem = dma_mem;
        }
    }

    if (ptp_priv->evlog != NULL) {
        /* Reset memory */
        memset((void *)ptp_priv->evlog, 0, ptp_priv->dma_mem_size);

        DBG_ERR(("Shared memory allocation (%d bytes) for event log successful at 0x%016lx.\n",
                ptp_priv->dma_mem_size, (long unsigned int)ptp_priv->dma_mem));
    }

#ifdef BDE_EDK_SUPPORT
    /* Do nothing */
#else
    /* Allocate dma for timestmap logging for extts */
    dma_mem = 0;
    ptp_priv->extts_dma_mem_size = sizeof(bksync_fw_extts_log_t);
    if (ptp_priv->extts_log == NULL) {
        DBG_ERR(("Allocate memory for extts log\n"));
        ptp_priv->extts_log = DMA_ALLOC_COHERENT(ptp_priv->dma_dev,
                                                   ptp_priv->extts_dma_mem_size,
                                                   &dma_mem);
        if (ptp_priv->extts_log != NULL) {
            ptp_priv->extts_dma_mem_addr = dma_mem;
        }
    }

    if (ptp_priv->extts_log != NULL) {
        /* Reset memory */
        memset((void *)ptp_priv->extts_log, 0, ptp_priv->extts_dma_mem_size);
        ptp_priv->extts_log->tail = 0;
        ptp_priv->extts_event.head = -1;
        ptp_priv->extts_log->head = -1;

        DBG_ERR(("Shared memory allocation (%d bytes) for extts log successful at 0x%016lx.\n",
                ptp_priv->extts_dma_mem_size, (long unsigned int)ptp_priv->extts_dma_mem_addr));
    }
#endif
    return;
}

static void bksync_ptp_fw_data_free(void)
{
    if (ptp_priv->evlog != NULL) {
        DMA_FREE_COHERENT(ptp_priv->dma_dev, ptp_priv->dma_mem_size,
                              (void *)ptp_priv->evlog, ptp_priv->dma_mem);
        ptp_priv->evlog = NULL;
    }

#ifdef BDE_EDK_SUPPORT
    /* Do nothing */
#else
    if (ptp_priv->extts_log != NULL) {
        DBG_ERR(("Free shared memory : extts log of %d bytes\n", ptp_priv->extts_dma_mem_size));
        DMA_FREE_COHERENT(ptp_priv->dma_dev, ptp_priv->extts_dma_mem_size,
                              (void *)ptp_priv->extts_log, ptp_priv->extts_dma_mem_addr);
        ptp_priv->extts_log = NULL;
    }
#endif
    return;
}



static void bksync_ptp_dma_init(int dcb_type)
{
    int endianess;
    int num_pports = 256;
    int mem_size = 16384; /*sizeof(bksync_info_t);*/


    ptp_priv->num_pports = num_pports;
    ptp_priv->dcb_type = dcb_type;

    if (ptp_priv->shared_addr == NULL) {
        ptp_priv->shared_addr = kzalloc(16384, GFP_KERNEL);
        ptp_priv->port_stats = kzalloc((sizeof(bksync_port_stats_t) * num_pports), GFP_KERNEL);
    }

    if (ptp_priv->shared_addr != NULL) {
        /* Reset memory. */
        memset((void *)ptp_priv->shared_addr, 0, mem_size);

#ifdef __LITTLE_ENDIAN
        endianess = 0;
#else
        endianess = 1;
#endif

#ifdef BDE_EDK_SUPPORT
        /* Do nothing */
        (void)endianess;
#else
        DEV_WRITE32(ptp_priv, CMIC_CMC_SCHAN_MESSAGE_14r(CMIC_CMC_BASE), ((pci_cos << 16) | endianess));

        DEV_WRITE32(ptp_priv, CMIC_CMC_SCHAN_MESSAGE_15r(CMIC_CMC_BASE), 1);
        DEV_WRITE32(ptp_priv, CMIC_CMC_SCHAN_MESSAGE_16r(CMIC_CMC_BASE), 1);
#endif
    }

    bksync_ptp_fw_data_alloc();

    DBG_VERB(("%s %p:%p, dcb_type: %d\n", __FUNCTION__, ptp_priv->base_addr,(void *)ptp_priv->shared_addr, dcb_type));

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
bksync_ioctl_cmd_handler(kcom_msg_clock_cmd_t *kmsg, int len, int dcb_type)
{
    u32 fw_status;
    bksync_dnx_jr2_devices_system_info_t *tmp_jr2devs_sys_info = NULL;
    int tmp = 0;
#ifdef BDE_EDK_SUPPORT
    uint64_t paddr;
#endif
    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    if (!module_initialized && kmsg->clock_info.cmd != KSYNC_M_HW_INIT) {
        kmsg->hdr.status = KCOM_E_NOT_FOUND;
        return sizeof(kcom_msg_hdr_t);
    }

    switch(kmsg->clock_info.cmd) {
        case KSYNC_M_HW_INIT:
            pci_cos = kmsg->clock_info.data[0];
            DBG_VERB(("Configuring pci_cosq:%d\n", pci_cos));
            if (kmsg->clock_info.data[1] == 0 || kmsg->clock_info.data[1] == 1) {
                /* Return success if the app is already initialized. */
                if (module_initialized) {
                    kmsg->hdr.status = KCOM_E_NONE;
                    return sizeof(kcom_msg_hdr_t);
                }

                fw_core = kmsg->clock_info.data[1];
#ifdef BDE_EDK_SUPPORT
                paddr = kmsg->clock_info.data[7] << 32;
                paddr |= kmsg->clock_info.data[8];
                ptp_priv->fw_comm = (bksync_fw_comm_t *)lkbde_edk_dmamem_map_p2v(paddr);
                if (ptp_priv->fw_comm == NULL) {
                    DBG_ERR(("Hostram address conversion to get virtual address failed\n"));
                    kmsg->hdr.status = KCOM_E_RESOURCE;
                    return sizeof(kcom_msg_hdr_t);
                }
#endif

                bksync_ptp_dma_init(dcb_type);

#ifdef BDE_EDK_SUPPORT
                /* Data from FW, hence don't memset fw_comm after address converstion */
                fw_status = ptp_priv->fw_comm->cmd;
#else
                DEV_READ32(ptp_priv, CMIC_CMC_SCHAN_MESSAGE_21r(CMIC_CMC_BASE), &fw_status);
#endif
                /* Return error if the app is not ready yet. */
                if (fw_status != 0xBADC0DE1) {
                    kmsg->hdr.status = KCOM_E_RESOURCE;
                    return sizeof(kcom_msg_hdr_t);
                }

                (ptp_priv->bksync_init_info).uc_port_num = kmsg->clock_info.data[2];
                (ptp_priv->bksync_init_info).uc_port_sysport = kmsg->clock_info.data[3];
                (ptp_priv->bksync_init_info).host_cpu_port = kmsg->clock_info.data[4];
                (ptp_priv->bksync_init_info).host_cpu_sysport = kmsg->clock_info.data[5];
                (ptp_priv->bksync_init_info).udh_len = kmsg->clock_info.data[6];
                (ptp_priv->bksync_init_info).application_v2 = kmsg->clock_info.data[9];

                DBG_VERB(("fw_core:%d uc_port:%d uc_sysport:%d pci_port:%d pci_sysport:%d application_v2:%d\n",
                        kmsg->clock_info.data[1], kmsg->clock_info.data[2], kmsg->clock_info.data[3],
                        kmsg->clock_info.data[4], kmsg->clock_info.data[5], kmsg->clock_info.data[9]));

                DBG_VERB(("uc_port:%d uc_sysport:%d pci_port:%d pci_sysport:%d application_v2:%d\n",
                        (ptp_priv->bksync_init_info).uc_port_num,
                        (ptp_priv->bksync_init_info).uc_port_sysport,
                        (ptp_priv->bksync_init_info).host_cpu_port,
                        (ptp_priv->bksync_init_info).host_cpu_sysport,
                        (ptp_priv->bksync_init_info).application_v2));

                if (bksync_ptp_init(&(ptp_priv->ptp_caps)) >= 0) {
                    module_initialized = 1;
                }

            }
            break;
        case KSYNC_M_HW_DEINIT:
#ifdef BDE_EDK_SUPPORT
            /* Do nothing */
#else
            DEV_WRITE32(ptp_priv, CMIC_CMC_SCHAN_MESSAGE_15r(CMIC_CMC_BASE), 0);
            DEV_WRITE32(ptp_priv, CMIC_CMC_SCHAN_MESSAGE_16r(CMIC_CMC_BASE), 0);
#endif
            bksync_ptp_deinit(&(ptp_priv->ptp_caps));
            module_initialized = 0;
            break;
        case KSYNC_M_HW_TS_DISABLE:
            bksync_ptp_hw_tstamp_disable(0, kmsg->clock_info.data[0], 0);
            break;
        case KSYNC_M_MTP_TS_UPDATE_ENABLE:
            bksync_ptp_mirror_encap_update(0, kmsg->clock_info.data[0], TRUE);
            break;
        case KSYNC_M_MTP_TS_UPDATE_DISABLE:
            bksync_ptp_mirror_encap_update(0, kmsg->clock_info.data[0], FALSE);
            break;
        case KSYNC_M_VERSION:
            break;
        case KSYNC_M_DNX_JR2DEVS_SYS_CONFIG:
            DBG_VERB(("bksync_ioctl_cmd_handler: KSYNC_M_DNX_JR2DEVS_SYS_CONFIG Rcvd.\n"));

            tmp_jr2devs_sys_info = (bksync_dnx_jr2_devices_system_info_t *)((char *)kmsg + sizeof(kcom_msg_clock_cmd_t));

            (ptp_priv->bksync_jr2devs_sys_info).ftmh_lb_key_ext_size = tmp_jr2devs_sys_info->ftmh_lb_key_ext_size;
            (ptp_priv->bksync_jr2devs_sys_info).ftmh_stacking_ext_size = tmp_jr2devs_sys_info->ftmh_stacking_ext_size;
            (ptp_priv->bksync_jr2devs_sys_info).pph_base_size = tmp_jr2devs_sys_info->pph_base_size;

            for (tmp = 0; tmp < BKSYNC_DNXJER2_PPH_LIF_EXT_TYPE_MAX; tmp++) {
                (ptp_priv->bksync_jr2devs_sys_info).pph_lif_ext_size[tmp] = tmp_jr2devs_sys_info->pph_lif_ext_size[tmp];
            }

            (ptp_priv->bksync_jr2devs_sys_info).system_headers_mode = tmp_jr2devs_sys_info->system_headers_mode;
            (ptp_priv->bksync_jr2devs_sys_info).udh_enable = tmp_jr2devs_sys_info->udh_enable;
            for (tmp = 0; tmp < BKSYNC_DNXJER2_UDH_DATA_TYPE_MAX; tmp++) {
                (ptp_priv->bksync_jr2devs_sys_info).udh_data_lenght_per_type[tmp] = tmp_jr2devs_sys_info->udh_data_lenght_per_type[tmp];
            }

            (ptp_priv->bksync_jr2devs_sys_info).cosq_port_cpu_channel = tmp_jr2devs_sys_info->cosq_port_cpu_channel;
            (ptp_priv->bksync_jr2devs_sys_info).cosq_port_pp_port = tmp_jr2devs_sys_info->cosq_port_pp_port;

            tmp_jr2devs_sys_info = &(ptp_priv->bksync_jr2devs_sys_info);

#if 0
            DBG_VERB(("ftmh_lb_key_ext_size %u ftmh_stacking_ext_size %u pph_base_size %u\n",
                    tmp_jr2devs_sys_info->ftmh_lb_key_ext_size, tmp_jr2devs_sys_info->ftmh_stacking_ext_size,
                    tmp_jr2devs_sys_info->pph_base_size));

            for (tmp = 0; tmp < BKSYNC_DNXJER2_PPH_LIF_EXT_TYPE_MAX ; tmp++) {
                DBG_VERB(("pph_lif_ext_size[%u] %u\n",
                        tmp, tmp_jr2devs_sys_info->pph_lif_ext_size[tmp]));
            }

            DBG_VERB(("system_headers_mode %u udh_enable %u\n",
                    tmp_jr2devs_sys_info->system_headers_mode, tmp_jr2devs_sys_info->udh_enable));

            for (tmp = 0; tmp < BKSYNC_DNXJER2_UDH_DATA_TYPE_MAX; tmp++) {
                DBG_VERB(("udh_data_lenght_per_type [%d] %u\n",
                        tmp, tmp_jr2devs_sys_info->udh_data_lenght_per_type[tmp]));
            }

            DBG_VERB(("cosq_port_cpu_channel :%u cosq_port_pp_port:%u\n",
                    tmp_jr2devs_sys_info->cosq_port_cpu_channel, tmp_jr2devs_sys_info->cosq_port_cpu_channel));
#endif
            break;
        default:
            kmsg->hdr.status = KCOM_E_NOT_FOUND;
            return sizeof(kcom_msg_hdr_t);
    }

    return sizeof(*kmsg);
}



/**
 * bksync_ptp_register
 * @priv: driver private structure
 * Description: this function will register the ptp clock driver
 * to kernel. It also does some house keeping work.
 */
static int bksync_ptp_register(void)
{
    int err = -ENODEV;

    /* Support on core-0 or core-1 */
    if (fw_core < 0 || fw_core > 1) {
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

    ptp_priv->ptp_caps = bksync_ptp_caps;

    mutex_init(&(ptp_priv->ptp_lock));

    /* Register ptp clock driver with bksync_ptp_caps */
    ptp_priv->ptp_clock = ptp_clock_register(&ptp_priv->ptp_caps, NULL);

    /* Initialize the Base address for CMIC and shared Memory access */
    ptp_priv->base_addr = lkbde_get_dev_virt(0);
    ptp_priv->dma_dev = lkbde_get_dma_dev(0);

    if (IS_ERR(ptp_priv->ptp_clock)) {
        ptp_priv->ptp_clock = NULL;
    } else if (ptp_priv->ptp_clock) {
        err = 0;

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

    }

     /* Initialize proc files */
     bksync_proc_root = proc_mkdir("bcm/ksync", NULL);
     bksync_proc_init();
     bksync_sysfs_init();
     ptp_priv->shared_addr = NULL;
     ptp_priv->port_stats = NULL;

     bksync_ptp_extts_logging_init();
exit:
    return err;
}

static int bksync_ptp_remove(void)
{
    if (!ptp_priv)
        return 0;

    module_initialized = 0;

    bksync_ptp_extts_logging_deinit();

    bksync_ptp_time_keep_deinit();

    bksync_proc_cleanup();
    bksync_sysfs_cleanup();
    remove_proc_entry("bcm/ksync", NULL);

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
 
    /* reset handshaking info */
    DEV_WRITE32(ptp_priv, CMIC_CMC_SCHAN_MESSAGE_15r(CMIC_CMC_BASE), 0);
    DEV_WRITE32(ptp_priv, CMIC_CMC_SCHAN_MESSAGE_16r(CMIC_CMC_BASE), 0);

    /* Deinitialize the PTP */
    bksync_ptp_deinit(&(ptp_priv->ptp_caps));

    bksync_ptp_fw_data_free();

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
