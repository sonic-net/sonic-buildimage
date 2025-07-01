/*
 * $Copyright: 2017-2024 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
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
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 */

/*
 * This module implements a Linux network driver for Broadcom
 * XGS switch devices. The driver simultaneously serves a
 * number of vitual Linux network devices and a Tx/Rx API
 * implemented in user space.
 *
 * Packets received from the switch device are sent to either
 * a virtual Linux network device or the user mode Rx API
 * based on a set of packet filters.susp
 *
 * Packets from the virtual Linux network devices and the user
 * mode Tx API are multiplexed with priority given to the Tx API.
 *
 * A message-based IOCTL interface is used for managing packet
 * filters and virtual Linux network interfaces.
 *
 * A virtual network interface can be configured to work in RCPU
 * mode, which means that packets from the switch device will
 * be encasulated with a RCPU header and a block of meta data
 * that basically contains the core DCB information. Likewise,
 * packets received from the Linux network stack are assumed to
 * be RCPU encapsulated when going out on an interface in RCPU
 * mode.
 *
 * The module implements basic Rx DMA rate control. The rate is
 * specified in packets per second, and different Rx DMA channels
 * can be configured to use different maximum packet rates.
 * The packet rate can be configure as a module parameter, and
 * it can also be changed dynamically through the proc file
 * system (syntax is described in function header comment).
 *
 * To support multiple instance, each instance has its event queue.
 *
 * To support pci hot-plug in this module, the resource update
 * should be handled when the PCI device is re-plugged.
 * NOTE: the KNET detach should be invoked before removing the
 * device.
 *
 * For a list of supported module parameters, please see below.
 */

#include <gmodule.h> /* Must be included first */
#include <linux-bde.h>
#include <kcom.h>
#include <bcm-knet.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/random.h>
#include <linux/seq_file.h>
#include <linux/if_vlan.h>
#include <linux/nsproxy.h>


MODULE_AUTHOR("Broadcom Corporation");
MODULE_DESCRIPTION("Network Device Driver for Broadcom BCM TxRx API");
MODULE_LICENSE("GPL");

static int debug;
LKM_MOD_PARAM(debug, "i", int, 0);
MODULE_PARM_DESC(debug,
"Debug level (default 0)");

static char *mac_addr = NULL;
LKM_MOD_PARAM(mac_addr, "s", charp, 0);
MODULE_PARM_DESC(mac_addr,
"Ethernet MAC address (default 02:10:18:xx:xx:xx)");

static int rx_buffer_size = 9216;
LKM_MOD_PARAM(rx_buffer_size, "i", int, 0);
MODULE_PARM_DESC(rx_buffer_size,
"Size of RX packet buffers (default 9216)");

static int default_mtu = 1500;
LKM_MOD_PARAM(default_mtu, "i", int, 0);
MODULE_PARM_DESC(default_mtu,
"Default MTU for KNET network interfaces (default 1500)");

static int rx_sync_retry = 1000;
LKM_MOD_PARAM(rx_sync_retry, "i", int, 0);
MODULE_PARM_DESC(rx_sync_retry,
"Retries if chain is incomplete on interrupt (default 10)");

static char *base_dev_name = NULL;
LKM_MOD_PARAM(base_dev_name, "s", charp, 0);
MODULE_PARM_DESC(base_dev_name,
"Base device name (default bcm0, bcm1, etc.)");

static int rcpu_mode = 0;
LKM_MOD_PARAM(rcpu_mode, "i", int, 0);
MODULE_PARM_DESC(rcpu_mode,
"Enable RCPU encapsulation (default 0)");

static char *rcpu_dmac = NULL;
LKM_MOD_PARAM(rcpu_dmac, "s", charp, 0);
MODULE_PARM_DESC(rcpu_dmac,
"RCPU destination MAC address (by default use L2 destination MAC address)");

static char *rcpu_smac = NULL;
LKM_MOD_PARAM(rcpu_smac, "s", charp, 0);
MODULE_PARM_DESC(rcpu_smac,
"RCPU source MAC address (by default use L2 source MAC address)");

static int rcpu_ethertype = 0xde08;
LKM_MOD_PARAM(rcpu_ethertype, "i", int, 0);
MODULE_PARM_DESC(rcpu_ethertype,
"RCPU EtherType (default DE08h)");

static int rcpu_signature = 0;
LKM_MOD_PARAM(rcpu_signature, "i", int, 0);
MODULE_PARM_DESC(rcpu_signature,
"RCPU Signature (default is PCI device ID)");

static int rcpu_vlan = 1;
LKM_MOD_PARAM(rcpu_vlan, "i", int, 0);
MODULE_PARM_DESC(rcpu_vlan,
"RCPU VLAN ID (default 1)");

static int use_rx_skb = 0;
LKM_MOD_PARAM(use_rx_skb, "i", int, 0);
MODULE_PARM_DESC(use_rx_skb,
"Use socket buffers for receive operation (default 0)");

static int num_rx_prio = 1;
LKM_MOD_PARAM(num_rx_prio, "i", int, 0);
MODULE_PARM_DESC(num_rx_prio,
"Number of filter priorities per Rx DMA channel");

static int rx_rate[16] = { 100000, 100000, 100000, 100000, 100000, 100000, 100000, 100000, 100000, 100000, 100000, 100000, 100000, 100000, 100000, 100000};
LKM_MOD_PARAM_ARRAY(rx_rate, "1-4i", int, NULL, 0);
MODULE_PARM_DESC(rx_rate,
"Rx rate in packets per second (default 100000)");

static int rx_burst[16] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
LKM_MOD_PARAM_ARRAY(rx_burst, "1-4i", int, NULL, 0);
MODULE_PARM_DESC(rx_burst,
"Rx rate burst maximum in packets (default rx_rate/10)");

static int check_rcpu_signature = 0;
LKM_MOD_PARAM(check_rcpu_signature, "i", int, 0);
MODULE_PARM_DESC(check_rcpu_signature,
"Check RCPU Signature for Tx packets from RCPU interfaces");

static int basedev_suspend = 0;
LKM_MOD_PARAM(basedev_suspend, "i", int, 0);
MODULE_PARM_DESC(basedev_suspend,
"Pause traffic till base device is up (enabled by default in NAPI mode)");

static int mirror_local = 1;
LKM_MOD_PARAM(mirror_local, "i", int, 0);
MODULE_PARM_DESC(mirror_local,
"Encapasulate packets based on mirror_to interface configuration, e.g. "
"encapsulate non-RCPU packets when mirrored to an RCPU interface "
"(default 1).");

/*
 * Force to add one layer of VLAN tag to untagged packets on Dune devices
 */
static int force_tagged = 1;
LKM_MOD_PARAM(force_tagged, "i", int, 0);
MODULE_PARM_DESC(force_tagged,
"Always tagged with VLAN tag with spceified VID or VSI(default 1)");

static int ft_tpid=0x8100;
LKM_MOD_PARAM(ft_tpid, "i", int, 0);
MODULE_PARM_DESC(ft_tpid,
"Tag Protocol Identifier (TPID) indicates the frame type (default 0x8100)");

static int ft_pri=0;
LKM_MOD_PARAM(ft_pri, "i", int, 0);
MODULE_PARM_DESC(ft_cfi,
"Priority (PRI) indicates the frame priority (default 0)");

static int ft_cfi=0;
LKM_MOD_PARAM(ft_cfi, "i", int, 0);
MODULE_PARM_DESC(ft_cfi,
"Canonical Format Indicator (CFI) indicates whether a MAC address is encapsulated in canonical format over different transmission media (default 0)");

static int ft_vid=0;
LKM_MOD_PARAM(ft_vid, "i", int, 0);
MODULE_PARM_DESC(ft_vid,
"VLAN ID (VID) indicates the VLAN to which a frame belongs (default 0)");

/* Debug levels */
#define DBG_LVL_VERB    0x1
#define DBG_LVL_DCB     0x2
#define DBG_LVL_PKT     0x4
#define DBG_LVL_SKB     0x8
#define DBG_LVL_CMD     0x10
#define DBG_LVL_EVT     0x20
#define DBG_LVL_IRQ     0x40
#define DBG_LVL_NAPI    0x80
#define DBG_LVL_PDMP    0x100
#define DBG_LVL_FLTR    0x200
#define DBG_LVL_KCOM    0x400
#define DBG_LVL_RCPU    0x800
#define DBG_LVL_WARN    0x1000
#define DBG_LVL_NDEV    0x2000
#define DBG_LVL_INST    0x4000
/* Level to output Dune internal headers Parsing */
#define DBG_LVL_DUNE    0x8000
#define DBG_LVL_DCB_TX  0x10000
#define DBG_LVL_DCB_RX  0x20000
#define DBG_LVL_PDMP_TX 0x40000
#define DBG_LVL_PDMP_RX 0x80000
#define DBG_LVL_PTP     0x100000

#define DBG_VERB(_s)    do { if (debug & DBG_LVL_VERB) gprintk _s; } while (0)
#define DBG_PKT(_s)     do { if (debug & DBG_LVL_PKT)  gprintk _s; } while (0)
#define DBG_SKB(_s)     do { if (debug & DBG_LVL_SKB)  gprintk _s; } while (0)
#define DBG_CMD(_s)     do { if (debug & DBG_LVL_CMD)  gprintk _s; } while (0)
#define DBG_EVT(_s)     do { if (debug & DBG_LVL_EVT)  gprintk _s; } while (0)
#define DBG_IRQ(_s)     do { if (debug & DBG_LVL_IRQ)  gprintk _s; } while (0)
#define DBG_NAPI(_s)    do { if (debug & DBG_LVL_NAPI) gprintk _s; } while (0)
#define DBG_PDMP(_s)    do { if (debug & DBG_LVL_PDMP) gprintk _s; } while (0)
#define DBG_FLTR(_s)    do { if (debug & DBG_LVL_FLTR) gprintk _s; } while (0)
#define DBG_KCOM(_s)    do { if (debug & DBG_LVL_KCOM) gprintk _s; } while (0)
#define DBG_RCPU(_s)    do { if (debug & DBG_LVL_RCPU) gprintk _s; } while (0)
#define DBG_WARN(_s)    do { if (debug & DBG_LVL_WARN) gprintk _s; } while (0)
#define DBG_NDEV(_s)    do { if (debug & DBG_LVL_NDEV) gprintk _s; } while (0)
#define DBG_INST(_s)    do { if (debug & DBG_LVL_INST) gprintk _s; } while (0)
#define DBG_DUNE(_s)    do { if (debug & DBG_LVL_DUNE) gprintk _s; } while (0)
#define DBG_DCB_TX(_s)  do { if (debug & (DBG_LVL_DCB|DBG_LVL_DCB_TX)) \
                                 gprintk _s; } while (0)
#define DBG_DCB_RX(_s)  do { if (debug & (DBG_LVL_DCB|DBG_LVL_DCB_RX)) \
                                 gprintk _s; } while (0)
#define DBG_DCB(_s)     do { if (debug & (DBG_LVL_DCB|DBG_LVL_DCB_TX| \
                                          DBG_LVL_DCB_RX)) \
                                 gprintk _s; } while (0)
#define DBG_PTP(_s)    do { if (debug & DBG_LVL_PTP) gprintk _s; } while (0)


/* This flag is used to indicate if debugging packet function is open or closed */
static int dbg_pkt_enable = 0;

/* Module Information */
#define MODULE_MAJOR 122
#define MODULE_NAME "linux-bcm-knet"

#ifndef NAPI_SUPPORT
#define NAPI_SUPPORT 1
#endif

#if NAPI_SUPPORT

static int use_napi = 0;
LKM_MOD_PARAM(use_napi, "i", int, 0);
MODULE_PARM_DESC(use_napi,
"Use NAPI interface (default 0)");

static int napi_weight = 64;
LKM_MOD_PARAM(napi_weight, "i", int, 0);
MODULE_PARM_DESC(napi_weight,
"Weight of NAPI interfaces (default 64)");

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
#define bkn_napi_enable(_dev, _napi) netif_poll_enable(_dev)
#define bkn_napi_disable(_dev, _napi) netif_poll_disable(_dev)
#define bkn_napi_schedule(_dev, _napi) netif_rx_schedule(_dev)
#define bkn_napi_schedule_prep(_dev, _napi) netif_rx_schedule_prep(_dev)
#define __bkn_napi_schedule(_dev, _napi) __netif_rx_schedule(_dev)
#define bkn_napi_complete(_dev, _napi) netif_rx_complete(_dev)
#else
#define bkn_napi_enable(_dev, _napi) napi_enable(_napi)
#define bkn_napi_disable(_dev, _napi) napi_disable(_napi)
#define bkn_napi_schedule(_dev, _napi) napi_schedule(_napi)
#define bkn_napi_schedule_prep(_dev, _napi) napi_schedule_prep(_napi)
#define __bkn_napi_schedule(_dev, _napi) __napi_schedule(_napi)
#define bkn_napi_complete(_dev, _napi) napi_complete(_napi)
#endif

#else

static int use_napi = 0;
static int napi_weight = 0;

#define bkn_napi_enable(_dev, _napi)
#define bkn_napi_disable(_dev, _napi)
#define bkn_napi_schedule(_dev, _napi)
#define bkn_napi_schedule_prep(_dev, _napi) (0)
#define __bkn_napi_schedule(_dev, _napi)
#define bkn_napi_complete(_dev, _napi)

#endif

/* Compatibility */

#ifndef DMA_BIT_MASK
#define DMA_BIT_MASK(n) (((n) == 64) ? ~0ULL : ((1ULL<<(n))-1))
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,7,0))
#define NETDEV_UPDATE_TRANS_START_TIME(dev) dev->trans_start = jiffies
#else
#define NETDEV_UPDATE_TRANS_START_TIME(dev) netif_trans_update(dev)
#endif

/*
 * The eth_hw_addr_set was added in Linux 5.15, but later backported
 * to various longterm releases, so we need a more advanced check with
 * the option to override the default.
 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,15,0))
#define KERNEL_HAS_ETH_HW_ADDR_SET 1
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,11,0) && \
     LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,188))
#define KERNEL_HAS_ETH_HW_ADDR_SET 1
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(5,5,0) && \
     LINUX_VERSION_CODE >= KERNEL_VERSION(5,4,251))
#define KERNEL_HAS_ETH_HW_ADDR_SET 1
#endif
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,20,0) &&     \
     LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,291))
#define KERNEL_HAS_ETH_HW_ADDR_SET 1
#endif
#ifndef KERNEL_HAS_ETH_HW_ADDR_SET
#define KERNEL_HAS_ETH_HW_ADDR_SET 0
#endif

#if (KERNEL_HAS_ETH_HW_ADDR_SET == 0)
static inline void eth_hw_addr_set(struct net_device *dev, const u8 *addr)
{
    memcpy(dev->dev_addr, addr, ETH_ALEN);
}
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
#define skb_copy_to_linear_data(_skb, _pkt, _len) \
    eth_copy_and_sum(_skb, _pkt, _len, 0)
struct napi_struct { int not_used; };
#define bkn_netif_napi_add(_dev, _napi, _poll, _weight) do { \
        (_dev)->poll = _poll; \
        (_dev)->weight = _weight; \
} while(0)
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(5,19,0))
#define bkn_netif_napi_add(_dev, _napi, _poll, _weight) \
            netif_napi_add(_dev, _napi, _poll, _weight)
#else
#define bkn_netif_napi_add(_dev, _napi, _poll, _weight) \
            netif_napi_add_weight(_dev, _napi, _poll, _weight)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,18))
#define SKB_PADTO(_skb,_len) (((_skb = skb_padto(_skb,_len)) == NULL) ? -1 : 0)
#else
#define SKB_PADTO(_skb,_len) skb_padto(_skb,_len)
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,12))
#define skb_header_cloned(_skb) \
    skb_cloned(_skb)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,27)
static inline void *netdev_priv(struct net_device *dev)
{
        return dev->priv;
}
#endif /* KERNEL_VERSION(2,4,27) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,5,0)
#define netdev_get_by_name(net, dev, tracker, gfp) \
        dev_get_by_name(net, dev)
#endif /* KERNEL_VERSION(6,5,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(6,0,0)
#define netdev_put(dev, tracker) \
        dev_put(dev)
#endif /* KERNEL_VERSION(6,0,0) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,23)
/* Special check for MontaVista 2.4.20 MIPS */
#if !(defined(MAX_USER_RT_PRIO) && defined(CONFIG_MIPS))
static inline void free_netdev(struct net_device *dev)
{
        kfree(dev);
}
#endif
static inline void netif_poll_disable(struct net_device *dev)
{
        while (test_and_set_bit(__LINK_STATE_RX_SCHED, &dev->state)) {
                /* No hurry. */
                current->state = TASK_INTERRUPTIBLE;
                schedule_timeout(1);
        }
}
static inline void netif_poll_enable(struct net_device *dev)
{
        clear_bit(__LINK_STATE_RX_SCHED, &dev->state);
}
#endif /* KERNEL_VERSION(2,4,23) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,21)
static struct sk_buff *skb_pad(struct sk_buff *skb, int pad)
{
        struct sk_buff *nskb;

        /* If the skbuff is non linear tailroom is always zero.. */
        if(skb_tailroom(skb) >= pad)
        {
                memset(skb->data+skb->len, 0, pad);
                return skb;
        }

        nskb = skb_copy_expand(skb, skb_headroom(skb), skb_tailroom(skb) + pad, GFP_ATOMIC);
        kfree_skb(skb);
        if(nskb)
                memset(nskb->data+nskb->len, 0, pad);
        return nskb;
}
static inline struct sk_buff *skb_padto(struct sk_buff *skb, unsigned int len)
{
        unsigned int size = skb->len;
        if(likely(size >= len))
                return skb;
        return skb_pad(skb, len-size);
}
#endif /* KERNEL_VERSION(2,4,21) */

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,10,0)
#define bkn_vlan_hwaccel_put_tag(_skb, _proto, _tci) \
    __vlan_hwaccel_put_tag(_skb, _tci)
#else
#define bkn_vlan_hwaccel_put_tag(_skb, _proto, _tci) \
    __vlan_hwaccel_put_tag(_skb, htons(_proto), _tci)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,1,0)
#define ETH_P_8021AD    0x88A8 /* 802.1ad Service VLAN */
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,27)
#define bkn_dma_mapping_error(d, a) \
    dma_mapping_error(a)
#define bkn_pci_dma_mapping_error(d, a) \
    pci_dma_mapping_error(a)
#else
#define bkn_dma_mapping_error(d, a) \
    dma_mapping_error(d, a)
#define bkn_pci_dma_mapping_error(d, a) \
    pci_dma_mapping_error(d, a)
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,30)
enum hwtstamp_tx_types {
    HWTSTAMP_TX_OFF,
    HWTSTAMP_TX_ON,
    HWTSTAMP_TX_ONESTEP_SYNC
};
enum {
    SKBTX_HW_TSTAMP = 1 << 0,
    SKBTX_SW_TSTAMP = 1 << 1,
    SKBTX_IN_PROGRESS = 1 << 2,
};
struct skb_shared_hwtstamps {
    ktime_t hwtstamp;
    ktime_t syststamp;
};
struct bkn_skb_shared_info {
    uint8_t tx_flags;
    struct skb_shared_hwtstamps hwtstamps;
};
#define bkn_skb_shinfo(_skb) ((struct bkn_skb_shared_info *)(unsigned char *)_skb->end)
#define bkn_skb_tx_flags(_skb) bkn_skb_shinfo(_skb)->tx_flags
static inline struct skb_shared_hwtstamps *skb_hwtstamps(struct sk_buff *skb)
{
    return &bkn_skb_shinfo(skb)->hwtstamps;
}
void skb_tstamp_tx(struct sk_buff *orig_skb, struct skb_shared_hwtstamps *hwtstamps)
{
}
static inline void bkn_skb_tx_timestamp(struct sk_buff *skb)
{
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
static inline ktime_t ns_to_ktime(u64 ns)
{
    static const ktime_t ktime;
    return ktime;
}
#endif
#elif LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
#include <linux/net_tstamp.h>
#define HWTSTAMP_TX_ONESTEP_SYNC 2
enum {
    SKBTX_HW_TSTAMP = 1 << 0,
    SKBTX_SW_TSTAMP = 1 << 1,
    SKBTX_IN_PROGRESS = 1 << 2,
};
#define bkn_skb_tx_flags(_skb) skb_shinfo(_skb)->tx_flags.flags
static inline void bkn_skb_tx_timestamp(struct sk_buff *skb)
{
}
#else
#include <linux/net_tstamp.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,2,0)
#define HWTSTAMP_TX_ONESTEP_SYNC 2
#endif

#define bkn_skb_tx_flags(_skb) skb_shinfo(_skb)->tx_flags
static inline void bkn_skb_tx_timestamp(struct sk_buff *skb)
{
    return skb_tx_timestamp(skb);
}
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#define bkn_dev_net_set(dev, net)
#else
#define bkn_dev_net_set(dev, net) dev_net_set(dev, net)
#endif

#ifdef LINUX_BDE_DMA_DEVICE_SUPPORT
#define BKN_DMA_DEV                         device
#define BKN_DMA_FROMDEV                     DMA_FROM_DEVICE
#define BKN_DMA_TODEV                       DMA_TO_DEVICE
#define BKN_DMA_MAP_SINGLE(d,p,s,r)         dma_map_single(d,p,s,r)
#define BKN_DMA_UNMAP_SINGLE(d,a,s,r)       dma_unmap_single(d,a,s,r)
#define BKN_DMA_ALLOC_COHERENT(d,s,h)       dma_alloc_coherent(d,s,h,GFP_ATOMIC|GFP_DMA32)
#define BKN_DMA_FREE_COHERENT(d,s,a,h)      dma_free_coherent(d,s,a,h)
#define BKN_DMA_MAPPING_ERROR(d,a)          bkn_dma_mapping_error(d,a)
#else
#define BKN_DMA_DEV                         pci_dev
#define BKN_DMA_FROMDEV                     PCI_DMA_FROMDEVICE
#define BKN_DMA_TODEV                       PCI_DMA_TODEVICE
#define BKN_DMA_MAP_SINGLE(d,p,s,r)         pci_map_single(d,p,s,r)
#define BKN_DMA_UNMAP_SINGLE(d,a,s,r)       pci_unmap_single(d,a,s,r)
#define BKN_DMA_ALLOC_COHERENT(d,s,h)       pci_alloc_consistent(d,s,h)
#define BKN_DMA_FREE_COHERENT(d,s,a,h)      pci_free_consistent(d,s,a,h)
#define BKN_DMA_MAPPING_ERROR(d,a)          bkn_pci_dma_mapping_error(d,a)
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29))
#define BKN_NETDEV_TX_BUSY      NETDEV_TX_BUSY
#else
#define BKN_NETDEV_TX_BUSY      1
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,15,0))
#define NDO_IOCTL       ndo_eth_ioctl
#else
#define NDO_IOCTL       ndo_do_ioctl
#endif

/*
 * Get a 16-bit value from packet offset
 * _data Pointer to packet
 * _offset Offset
 */
#define PKT_U16_GET(_data, _offset) \
    (uint16_t)(_data[_offset] << 8 | _data[_offset + 1])


/* RCPU operations */
#define RCPU_OPCODE_RX          0x10
#define RCPU_OPCODE_TX          0x20

/* RCPU flags */
#define RCPU_F_MODHDR           0x4

/* RCPU encapsulation */
#define RCPU_HDR_SIZE           32
#define RCPU_TX_META_SIZE       32
#define RCPU_TX_ENCAP_SIZE      (RCPU_HDR_SIZE + RCPU_TX_META_SIZE)
#define RCPU_RX_META_SIZE       64
#define RCPU_RX_META_SIZE_MAX   256
#define RCPU_RX_ENCAP_SIZE      (RCPU_HDR_SIZE + RCPU_RX_META_SIZE)

#define PKT_TX_HDR_SIZE         16

static volatile int module_initialized;

static int module_reload[LINUX_BDE_MAX_DEVICES];

static ibde_t *kernel_bde = NULL;

/* Descriptor info */
typedef struct bkn_desc_info_s {
    uint32_t *dcb_mem;
    uint64_t dcb_dma;
    struct sk_buff *skb;
    uint64_t skb_dma;
    uint32_t dma_size;
} bkn_desc_info_t;

/* DCB chain info */
typedef struct bkn_dcb_chain_s {
    struct list_head list;
    int dcb_cnt;
    int dcb_cur;
    uint32_t *dcb_mem;
    uint64_t dcb_dma;
} bkn_dcb_chain_t;

#define MAX_TX_DCBS 64
#define MAX_RX_DCBS 64

#define NUM_DMA_CHAN 16
#define NUM_CMICX_DMA_CHAN 8
#define NUM_CMICR_DMA_CHAN 16
#define NUM_RX_CHAN 15
#define NUM_CMICR_RX_CHAN 15
#define NUM_CMICX_RX_CHAN 7
#define NUM_CMICM_RX_CHAN 3

#define DEFAULT_RX_RATE 100000

#define FCS_SZ 4
#define TAG_SZ 4

/* Device control info */
typedef struct bkn_switch_info_s {
    struct list_head list;
    struct list_head ndev_list; /* Associated virtual Ethernet interfaces */
    struct net_device **ndevs;  /* Indexed array of ndev_list */
    int ndev_max;               /* Size of indexed array */
    struct list_head rxpf_list; /* Associated Rx packet filters */
    volatile void *base_addr;   /* Base address for PCI register access */
    struct BKN_DMA_DEV *dma_dev;    /* Required for DMA memory control */
    struct pci_dev *pdev;       /* Required for DMA memory control */
    struct net_device *dev;     /* Base network device */
    struct napi_struct napi;    /* New NAPI */
    struct timer_list timer;    /* Retry/resource timer */
    int timer_queued;           /* Flag indicating queued timer function */
    uint32_t timer_runs;        /* Timer function runs (debug only) */
    struct timer_list rxtick;   /* Rx rate control timer */
    uint32_t rxticks_per_sec;   /* Rx rate control update frequency */
    uint32_t rxtick_jiffies;    /* Time between updates (in jiffies) */
    uint32_t rxticks;           /* Rx rate control debug counter */
    uint32_t interrupts;        /* Total number of interrupts */
    spinlock_t lock;            /* Main lock for device */
    int cfg_api_locked;         /* Block configuration API when main lock is
                                   temporary released for calling kernel APIs. */
    int dev_no;                 /* Device number (from BDE) */
    int cpu_no;                 /* Cpu number. 1 for iHost(AXI),0 for others */
    int dcb_type;               /* DCB type */
    int dcb_wsize;              /* DCB size (in 32-bit words) */
    int pkt_hdr_size;           /* Packet header size */
    uint32_t ftmh_lb_key_ext_size;  /* FTMH LB-Key Extension existence/size */
    uint32_t ftmh_stacking_ext_size;/* FTMH Stacking extension existence/size */
    uint32_t pph_base_size;         /* Size of PPH base */
    uint32_t pph_lif_ext_size[8];   /* Size of PPH Lif extension header */
    uint32_t udh_length_type[4];    /* Size of UDH header per type */
    uint32_t udh_size;              /* Size of UDH header on legacy devices */
    uint32_t oamp_punt;             /* OAMP port if nonzero */
    uint32_t enet_channels;         /* Ethernet channels, No header observed.*/
    uint8_t no_skip_udh_check;      /* Indicates UDH won't be skipped */
    uint8_t oam_dm_tod_exist;       /* Indicates presence of OAM TOD MSB */
    uint8_t system_headers_mode;    /* Indicates system header mode */
    uint8_t udh_enable;             /* Indicates UDH existence */
    uint8_t oamp_port_number;       /* Indicates number of OAMP system port number */
    uint32_t oamp_ports[KCOM_HW_INFO_OAMP_PORT_MAX]; /* OAMP system port array */
    int rx_chans;               /* Number of Rx channels */
    uint32_t dma_hi;            /* DMA higher address */
    uint32_t cmic_type;         /* CMIC type (CMICe or CMICm) */
    uint32_t irq_mask;          /* Active IRQs for DMA control */
    uint32_t napi_poll_mode;    /* NAPI is in polling mode */
    uint32_t napi_not_done;     /* NAPI poll did not process all packets */
    uint32_t napi_poll_again;   /* Used if DCB chain is restarted */
    uint32_t tx_yield;          /* Tx schedule for Continuous DMA and Non-NAPI
                                   mode. */
    void *dcb_mem;              /* Logical pointer to DCB memory */
    uint64_t dcb_dma;           /* Physical bus address for DCB memory */
    int dcb_mem_size;           /* Total size of allocated DCB memory */
    uint32_t dma_events;        /* DMA events pending for BCM API */
    uint32_t rcpu_sig;          /* RCPU signature */
    uint64_t halt_addr[NUM_DMA_CHAN]; /* DMA halt address */
    uint32_t cdma_channels;     /* Active channels for Continuous DMA mode */
    uint32_t poll_channels;     /* Channels for polling */
    uint32_t unet_channels;     /* User network channels */
    uint32_t inst_id;           /* Instance id of this device */
    uint32_t base_id;           /* Device (family) ID, like 0x8675 for Jericho A0, 0x */
    uint16_t dev_id;            /* Device ID */
    uint8_t rev_id;             /* Revision ID */
    uint32_t irq_mask_reg;      /* Interrupt mask register address */
    int evt_idx;                /* Event queue index for this device*/
    int basedev_suspended;      /* Base device suspended */
    int pcie_link_status;       /* This flag is used to indicate PCIE Link status, 0 for up and 1 for down */
    struct sk_buff_head tx_ptp_queue;   /* Tx PTP skb queue */
    struct work_struct tx_ptp_work;     /* Tx PTP work */
    struct {
        bkn_desc_info_t desc[MAX_TX_DCBS+1];
        int free;               /* Number of free Tx DCBs */
        int cur;                /* Index of current Tx DCB */
        int dirty;              /* Index of next Tx DCB to complete */
        int api_active;         /* BCM Tx API is in progress */
        int suspends;           /* Calls to netif_stop_queue (debug only) */
        struct list_head api_dcb_list; /* Tx DCB chains from BCM Tx API */
        bkn_dcb_chain_t *api_dcb_chain; /* Current Tx DCB chain */
        bkn_dcb_chain_t *api_dcb_chain_end; /* Tx DCB chain end */
        uint32_t pkts;              /* Tx packet counter */
        uint32_t pkts_d_no_skb;     /* Tx drop - skb allocation failed */
        uint32_t pkts_d_rcpu_encap; /* Tx drop - bad RCPU encapsulation */
        uint32_t pkts_d_rcpu_sig;   /* Tx drop - bad RCPU signature */
        uint32_t pkts_d_rcpu_meta;  /* Tx drop - bad RCPU meta data */
        uint32_t pkts_d_pad_fail;   /* Tx drop - pad to minimum size failed */
        uint32_t pkts_d_dma_resrc;  /* Tx drop - no DMA resources */
        uint32_t pkts_d_callback;   /* Tx drop - consumed by call-back */
        uint32_t pkts_d_no_link;    /* Tx drop - software link down */
        uint32_t pkts_d_over_limit; /* Tx drop - length is out of range */
    } tx;
    struct {
        bkn_desc_info_t desc[MAX_RX_DCBS+1];
        int free;               /* Number of free Rx DCBs */
        int cur;                /* Index of current Rx DCB */
        int dirty;              /* Index of next Rx DCB to complete */
        int running;            /* Rx DMA is active */
        int api_active;         /* BCM Rx API is active */
        int chain_complete;     /* All DCBs in chain processed */
        int sync_err;           /* Chain done with incomplete DCBs (debug) */
        int sync_retry;         /* Total retry times for sync error (debug) */
        int sync_maxloop;       /* Max loop times once in recovering sync (debug) */
        int use_rx_skb;         /* Use SKBs for DMA */
        uint32_t rate_max;      /* Rx rate in packets/sec */
        uint32_t burst_max;     /* Rx burst size in number of packets */
        uint32_t tokens;        /* Tokens for Rx rate control */
        uint32_t rate;          /* Current packet rate */
        unsigned long tok_jif;  /* Jiffies at last token update */
        unsigned long rate_jif; /* Jiffies at last rate update */
        struct list_head api_dcb_list; /* Rx DCB chains from BCM Rx API */
        bkn_dcb_chain_t *api_dcb_chain; /* Current Rx DCB chain */
        bkn_dcb_chain_t *api_dcb_chain_end; /* Rx DCB chain end */
        uint32_t pkts;              /* Rx packet counter */
        uint32_t pkts_ref;          /* Rx packet count for rate calculation */
        uint32_t pkts_f_api;        /* Rx packets filtered to API */
        uint32_t pkts_f_netif;      /* Rx packets filtered to net interface */
        uint32_t pkts_m_api;        /* Rx packets mirrored to API */
        uint32_t pkts_m_netif;      /* Rx packets mirrored to net interface */
        uint32_t pkts_d_no_skb;     /* Rx drop - skb allocation failed */
        uint32_t pkts_d_no_match;   /* Rx drop - no matching filters */
        uint32_t pkts_d_unkn_netif; /* Rx drop - unknown net interface ID */
        uint32_t pkts_d_unkn_dest;  /* Rx drop - unknown destination type */
        uint32_t pkts_d_callback;   /* Rx drop - consumed by call-back */
        uint32_t pkts_d_no_link;    /* Rx drop - software link down */
        uint32_t pkts_d_no_api_buf; /* Rx drop - no API buffers */
    } rx[NUM_RX_CHAN];
} bkn_switch_info_t;

#define INVALID_INSTANCE_ID         BDE_DEV_INST_ID_INVALID

/* 0x1 - Jericho 2 mode */
#define BKN_DNX_JR2_MODE            1
/* PTCH_2 */
#define BKN_DNX_PTCH_2_SIZE         2
/* ITMH */
#define BKN_DNX_ITMH_SIZE           5
/* Modlue Header */
#define BKN_DNX_MODULE_HEADER_SIZE   16
/* FTMH */
#define BKN_DNX_FTMH_TC_MSB                            14
#define BKN_DNX_FTMH_TC_NOF_BITS                       3
#define BKN_DNX_FTMH_SRC_SYS_PORT_AGGREGATE_MSB        17
#define BKN_DNX_FTMH_SRC_SYS_PORT_AGGREGATE_NOF_BITS   16
#define BKN_DNX_FTMH_PP_DSP_MSB                        33
#define BKN_DNX_FTMH_PP_DSP_NOF_BITS                   8
#define BKN_DNX_FTMH_ACTION_TYPE_MSB                   43
#define BKN_DNX_FTMH_ACTION_TYPE_NOF_BITS              2
#define BKN_DNX_FTMH_PPH_TYPE_IS_TSH_EN_MSB            73
#define BKN_DNX_FTMH_PPH_TYPE_IS_TSH_EN_NOF_BITS       1
#define BKN_DNX_FTMH_PPH_TYPE_IS_PPH_EN_MSB            74
#define BKN_DNX_FTMH_PPH_TYPE_IS_PPH_EN_NOF_BITS       1
#define BKN_DNX_FTMH_TM_DST_EXT_PRESENT_MSB            75
#define BKN_DNX_FTMH_TM_DST_EXT_PRESENT_NOF_BITS       1
#define BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE_MSB         76
#define BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE_NOF_BITS    1
#define BKN_DNX_FTMH_FLOW_ID_EXT_SIZE_MSB              77
#define BKN_DNX_FTMH_FLOW_ID_EXT_SIZE_NOF_BITS         1
#define BKN_DNX_FTMH_BIER_BFR_EXT_SIZE_MSB             78
#define BKN_DNX_FTMH_BIER_BFR_EXT_SIZE_NOF_BITS        1
/* Fix Length for FTMH and Extension headers */
#define BKN_DNX_FTMH_BASE_SIZE                         10
#define BKN_DNX_FTMH_BIER_BFR_EXT_SIZE                 2
#define BKN_DNX_FTMH_TM_DST_EXT_SIZE                   3
#define BKN_DNX_FTMH_FLOW_ID_EXT_SIZE                  3
#define BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE             6
#define BKN_DNX_SYSPORTS_PER_DEVICE_MASK               0x3FF
/*ASE*/
#define BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_MSB              0
#define BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_NOF_BITS         4
#define BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_DM_1588          2
#define BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_DM_NTP           3
#define BKN_DNX_FTMH_ASE_TYPE_MSB                      47
#define BKN_DNX_FTMH_ASE_TYPE_NOF_BITS                 1
#define BKN_DNX_FTMH_ASE_TYPE_OAM                      0
/* TSH */
#define BKN_DNX_TSH_SIZE                               4
/* PPH */
#define BKN_DNX_INTERNAL_BASE_TYPE_9                        9
#define BKN_DNX_INTERNAL_BASE_TYPE_10                       10
#define BKN_DNX_INTERNAL_BASE_TYPE_12                       12
#define BKN_DNX_INTERNAL_9_FORWARD_DOMAIN_MSB               5
#define BKN_DNX_INTERNAL_9_FORWARD_DOMAIN_NOF_BITS          16
#define BKN_DNX_INTERNAL_9_LEARN_EXT_PRESENT_MSB            53
#define BKN_DNX_INTERNAL_9_LEARN_EXT_PRESENT_NOF_BITS       1
#define BKN_DNX_INTERNAL_9_FHEI_SIZE_MSB                    54
#define BKN_DNX_INTERNAL_9_FHEI_SIZE_NOF_BITS               2
#define BKN_DNX_INTERNAL_9_LIF_EXT_TYPE_MSB                 56
#define BKN_DNX_INTERNAL_9_LIF_EXT_TYPE_NOF_BITS            3
#define BKN_DNX_INTERNAL_10_FORWARD_DOMAIN_MSB              9
#define BKN_DNX_INTERNAL_10_FORWARD_DOMAIN_NOF_BITS         16
#define BKN_DNX_INTERNAL_10_LEARN_EXT_PRESENT_MSB           61
#define BKN_DNX_INTERNAL_10_LEARN_EXT_PRESENT_NOF_BITS      1
#define BKN_DNX_INTERNAL_10_FHEI_SIZE_MSB                   62
#define BKN_DNX_INTERNAL_10_FHEI_SIZE_NOF_BITS              2
#define BKN_DNX_INTERNAL_10_LIF_EXT_TYPE_MSB                64
#define BKN_DNX_INTERNAL_10_LIF_EXT_TYPE_NOF_BITS           3
#define BKN_DNX_INTERNAL_12_FORWARD_DOMAIN_MSB              21
#define BKN_DNX_INTERNAL_12_FORWARD_DOMAIN_NOF_BITS         18
#define BKN_DNX_INTERNAL_12_LEARN_EXT_PRESENT_MSB           77
#define BKN_DNX_INTERNAL_12_LEARN_EXT_PRESENT_NOF_BITS      1
#define BKN_DNX_INTERNAL_12_FHEI_SIZE_MSB                   78
#define BKN_DNX_INTERNAL_12_FHEI_SIZE_NOF_BITS              2
#define BKN_DNX_INTERNAL_12_LIF_EXT_TYPE_MSB                80
#define BKN_DNX_INTERNAL_12_LIF_EXT_TYPE_NOF_BITS           3
#define BKN_DNX_INTERNAL_12_PARSING_START_OFFSET_MSB                83
#define BKN_DNX_INTERNAL_12_PARSING_START_OFFSET_NOF_BITS           7
/* PPH.FHEI_TYPE */
#define BKN_DNX_INTERNAL_FHEI_TYPE_SZ0                      1
#define BKN_DNX_INTERNAL_FHEI_TYPE_SZ1                      2
#define BKN_DNX_INTERNAL_FHEI_TYPE_SZ2                      3
/* FHEI */
#define BKN_DNX_INTERNAL_FHEI_SZ0_SIZE                      3
#define BKN_DNX_INTERNAL_FHEI_SZ1_SIZE                      5
#define BKN_DNX_INTERNAL_FHEI_SZ2_SIZE                      8
#define BKN_DNX_INTERNAL_FHEI_TRAP_5B_QUALIFIER_MSB         0
#define BKN_DNX_INTERNAL_FHEI_TRAP_5B_QUALIFIER_NOF_BITS    27
#define BKN_DNX_INTERNAL_FHEI_TRAP_5B_CODE_MSB              27
#define BKN_DNX_INTERNAL_FHEI_TRAP_5B_CODE_NOF_BITS         9
#define BKN_DNX_INTERNAL_FHEI_TRAP_5B_TYPE_MSB              36
#define BKN_DNX_INTERNAL_FHEI_TRAP_5B_TYPE_NOF_BITS         4
/* PPH Extension */
#define BKN_DNX_INTERNAL_LEARN_EXT_SIZE                     19
/* UDH */
#define BKN_DNX_UDH_DATA_TYPE_0_MSB                    0
#define BKN_DNX_UDH_DATA_TYPE_0_NOF_BITS               2
#define BKN_DNX_UDH_DATA_TYPE_1_MSB                    2
#define BKN_DNX_UDH_DATA_TYPE_1_NOF_BITS               2
#define BKN_DNX_UDH_DATA_TYPE_2_MSB                    4
#define BKN_DNX_UDH_DATA_TYPE_2_NOF_BITS               2
#define BKN_DNX_UDH_DATA_TYPE_3_MSB                    6
#define BKN_DNX_UDH_DATA_TYPE_3_NOF_BITS               2
#define BKN_DNX_UDH_BASE_SIZE                          1
/* TOD SECOND header */
#define BKN_DNX_TOD_SECOND_SIZE                        4
/* OIBIH */
#define BKN_DNX_OIBIH_SIZE          14
#define BKN_DNX_OIBIH_OAM_PDU_OFFSET_MSB               104
#define BKN_DNX_OIBIH_OAM_PDU_OFFSET_NOF_BITS          8


/* PPH fwd_domain type. */
#define BKN_DNX_PPH_FWD_DOMAIN_TYPE_VSI                0
#define BKN_DNX_PPH_FWD_DOMAIN_TYPE_VRF                3

#define BKN_DNX_PPH_FWD_DOMAIN_TYPE_GET(_fwd_domain) (((_fwd_domain) >> 16) & 0x3)
#define BKN_DNX_PPH_FWD_DOMAIN_ID_GET(_fwd_domain)   ((_fwd_domain) & 0xffff)
#define BKN_DNX_PPH_FWD_DOMAIN_IS_VSI(_fwd_domain)   (BKN_DNX_PPH_FWD_DOMAIN_TYPE_GET(_fwd_domain) == BKN_DNX_PPH_FWD_DOMAIN_TYPE_VSI)
#define BKN_DNX_PPH_FWD_DOMAIN_IS_VRF(_fwd_domain)   (BKN_DNX_PPH_FWD_DOMAIN_TYPE_GET(_fwd_domain) == BKN_DNX_PPH_FWD_DOMAIN_TYPE_VRF)

#define BKN_DPP_HDR_MAX_SIZE 40
/* PTCH_2 */
#define BKN_DPP_PTCH_2_SIZE         2
/* ITMH */
#define BKN_DPP_ITMH_SIZE           4
/* FTMH */
#define BKN_DPP_FTMH_LB_EXT_EN              0x1
#define BKN_DPP_FTMH_STACKING_EXT_EN        0x2
#define BKN_DPP_FTMH_SIZE_BYTE               9
#define BKN_DPP_FTMH_LB_EXT_SIZE_BYTE        1
#define BKN_DPP_FTMH_STACKING_SIZE_BYTE      2
#define BKN_DPP_FTMH_DEST_EXT_SIZE_BYTE      2
#define BKN_DPP_FTMH_LB_EXT_SIZE_BYTE        1
#define BKN_DPP_FTMH_PKT_SIZE_MSB            0
#define BKN_DPP_FTMH_PKT_SIZE_NOF_BITS       14
#define BKN_DPP_FTMH_TC_MSB                  14
#define BKN_DPP_FTMH_TC_NOF_BITS             3
#define BKN_DPP_FTMH_SRC_SYS_PORT_MSB        17
#define BKN_DPP_FTMH_SRC_SYS_PORT_NOF_BITS   16
#define BKN_DPP_FTMH_PP_DSP_MSB              33
#define BKN_DPP_FTMH_PP_DSP_NOF_BITS         8
#define BKN_DPP_FTMH_EXT_DSP_EXIST_MSB       68
#define BKN_DPP_FTMH_EXT_DSP_EXIST_NOF_BITS  1
#define BKN_DPP_FTMH_EXT_MSB                 45
#define BKN_DPP_FTMH_EXT_NOF_BITS            2
#define BKN_DPP_FTMH_FIRST_EXT_MSB           72
#define BKN_DPP_FTMH_ACTION_TYPE_MSB         43
#define BKN_DPP_FTMH_ACTION_TYPE_NOF_BITS    2
#define BKN_DPP_FTMH_PPH_TYPE_MSB            45
#define BKN_DPP_FTMH_PPH_TYPE_NOF_BITS       2

/* OTSH */
#define BKN_DPP_OTSH_SIZE_BYTE               6
#define BKN_DPP_OTSH_TYPE_MSB                0
#define BKN_DPP_OTSH_TYPE_NOF_BITS           2
#define BKN_DPP_OTSH_OAM_SUB_TYPE_MSB        2
#define BKN_DPP_OTSH_OAM_SUB_TYPE_NOF_BITS   3

#define BKN_DPP_OTSH_TYPE_OAM                0
#define BKN_DPP_OTSH_OAM_SUB_TYPE_DM_1588    2
#define BKN_DPP_OTSH_OAM_SUB_TYPE_DM_NTP     3

#define BKN_DPP_OAM_DM_TOD_SIZE_BYTE         4

/* PPH */
#define BKN_DPP_INTERNAL_SIZE_BYTE                           7
#define BKN_DPP_INTERNAL_EEI_EXTENSION_PRESENT_MSB           0
#define BKN_DPP_INTERNAL_EEI_EXTENSION_PRESENT_NOF_BITS      1
#define BKN_DPP_INTERNAL_LEARN_EXENSION_PRESENT_MSB          1
#define BKN_DPP_INTERNAL_LEARN_EXENSION_PRESENT_NOF_BITS     1
#define BKN_DPP_INTERNAL_FHEI_SIZE_MSB                       2
#define BKN_DPP_INTERNAL_FHEI_SIZE_NOF_BITS                  2
#define BKN_DPP_INTERNAL_FORWARD_CODE_MSB                    4
#define BKN_DPP_INTERNAL_FORWARD_CODE_NOF_BITS               4
#define BKN_DPP_INTERNAL_FORWARD_CODE_CPU_TRAP               7
#define BKN_DPP_INTERNAL_FORWARDING_HEADER_OFFSET_MSB        8
#define BKN_DPP_INTERNAL_FORWARDING_HEADER_OFFSET_NOF_BITS   7
#define BKN_DPP_INTERNAL_VSI_MSB                             22
#define BKN_DPP_INTERNAL_VSI_NOF_BITS                        16

/* FHEI TRAP/SNOOP 3B */
#define BKN_DPP_INTERNAL_FHEI_3B_SIZE_BYTE                   3
#define BKN_DPP_INTERNAL_FHEI_5B_SIZE_BYTE                   5
#define BKN_DPP_INTERNAL_FHEI_8B_SIZE_BYTE                   8
#define BKN_DPP_INTERNAL_FHEI_TRAP_SNOOP_3B_CPU_TRAP_CODE_QUALIFIER_MSB      0
#define BKN_DPP_INTERNAL_FHEI_TRAP_SNOOP_3B_CPU_TRAP_CODE_QUALIFIER_NOF_BITS 16
#define BKN_DPP_INTERNAL_FHEI_TRAP_SNOOP_3B_CPU_TRAP_CODE_MSB                16
#define BKN_DPP_INTERNAL_FHEI_TRAP_SNOOP_3B_CPU_TRAP_CODE_NOF_BITS           8
/* PPH extension */
#define BKN_DPP_INTERNAL_EXPLICIT_EDITING_INFOMATION_EXTENSION_SIZE_BYTE     3
#define BKN_DPP_INTERNAL_LEARN_EXTENSION_SIZE_BYTE           5

#define BKN_SAND_SCRATCH_DATA_SIZE                           4

/* ftmh action type. */
typedef enum bkn_dpp_ftmh_action_type_e {
    BKN_DPP_FTMH_ACTION_TYPE_FORWARD = 0, /* TM action is forward */
    BKN_DPP_FTMH_ACTION_TYPE_SNOOP = 1,   /* TM action is snoop */
    BKN_DPP_FTMH_ACTION_TYPE_INBOUND_MIRROR = 2, /* TM action is inbound mirror. */
    BKN_DPP_FTMH_ACTION_TYPE_OUTBOUND_MIRROR = 3 /* TM action is outbound mirror. */
}bkn_dpp_ftmh_action_type_t;

/* ftmh dest extension. */
typedef struct bkn_dpp_ftmh_dest_extension_s {
    uint8_t valid; /* Set if the extension is present */
    uint32_t dst_sys_port; /* Destination System Port */
} bkn_dpp_ftmh_dest_extension_t;

/* dnx packet */
typedef struct bkn_dune_system_header_info_s {
    uint32_t system_header_size;
    struct {
        uint32_t action_type;                 /* Indicates if the copy is one of the Forward Snoop or Mirror packet copies */
        uint32_t source_sys_port_aggregate;   /* Source System port*/
    } ftmh;
    struct {
        uint32_t parsing_start_offset;
        uint32_t forward_domain;
        uint32_t trap_qualifier;
        uint32_t trap_id;
    } internal;
    /** Flags for RX header parser */
    /** Indicates whether 1st system header is following */
#define BKN_RX_HEADER_F_HAS_ONE_SYSTEM_HEADER      (0x1 << 0)
    /** Indicates whether 2nd system header is following */
#define BKN_RX_HEADER_F_HAS_TWO_SYSTEM_HEADER      (0x1 << 1)
    /** Indicates whether TSH is following */
#define BKN_RX_HEADER_F_HAS_TSH                    (0x1 << 2)
    /** Indicates whether OTSH is following */
#define BKN_RX_HEADER_F_HAS_OTSH                   (0x1 << 3)
    /** Indicates whether internal header is following */
#define BKN_RX_HEADER_F_HAS_INTERNAL_HEADER        (0x1 << 4)
    /** Indicates whether TOD second header is following */
#define BKN_RX_HEADER_F_HAS_OAM_DM_TOD_SECOND      (0x1 << 5)
    uint32_t flags;
} bkn_dune_system_header_info_t;

#define PREV_IDX(_cur, _max) (((_cur) == 0) ? (_max) - 1 : (_cur) - 1)

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

#define MEMORY_BARRIER smp_mb()

/* Default random MAC address has Broadcom OUI with local admin bit set */
static u8 bkn_dev_mac[6] = { 0x02, 0x10, 0x18, 0x00, 0x00, 0x00 };

static u8 bkn_rcpu_dmac[6];
static u8 bkn_rcpu_smac[6];

/* Driver Proc Entry root */
static struct proc_dir_entry *bkn_proc_root = NULL;

typedef struct bkn_priv_s {
    struct list_head list;
    struct net_device_stats stats;
    struct net_device *dev;
    bkn_switch_info_t *sinfo;
    int id;
    int type;
    int port;
    int qnum;
    uint32_t vlan;
    uint32_t flags;
    uint32_t cb_user_data;
    uint8_t system_headers[KCOM_NETIF_SYSTEM_HEADERS_SIZE_MAX];
    uint32_t system_headers_size;
    int tx_hwts;                /* HW timestamp for Tx */
    int rx_hwts;                /* HW timestamp for Rx */
    int phys_port;
    u32 ptp_stats_tx;
    u32 ptp_stats_rx;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0))
    struct ethtool_link_settings link_settings;
#endif
    int ref_count;
} bkn_priv_t;

typedef struct bkn_filter_s {
    struct list_head list;
    int dev_no;
    unsigned long hits;
    kcom_filter_t kf;
    knet_filter_cb_f cb;
} bkn_filter_t;


/*
 * Multiple instance support in KNET
 */
static int _bkn_multi_inst = 0;
typedef struct {
    wait_queue_head_t evt_wq;
    int evt_wq_put;
    int evt_wq_get;
    uint32_t inst_id;
} bkn_evt_resource_t;

static bkn_evt_resource_t _bkn_evt[LINUX_BDE_MAX_DEVICES];

static int bkn_knet_dev_init(int d);
static int bkn_knet_dev_reinit(int d);

/* IOCTL debug counters */
static int ioctl_cmd;
static int ioctl_evt;

/* Switch devices */
LIST_HEAD(_sinfo_list);

/* Reallocation chunk size for netif array */
#define NDEVS_CHUNK     64

/* User call-backs */
static knet_skb_cb_f knet_rx_cb = NULL;
static knet_skb_cb_f knet_tx_cb = NULL;
static knet_filter_cb_f knet_filter_cb = NULL;
static knet_hw_tstamp_enable_cb_f knet_hw_tstamp_enable_cb = NULL;
static knet_hw_tstamp_enable_cb_f knet_hw_tstamp_disable_cb = NULL;
static knet_hw_tstamp_tx_time_get_cb_f knet_hw_tstamp_tx_time_get_cb = NULL;
static knet_hw_tstamp_tx_meta_get_cb_f knet_hw_tstamp_tx_meta_get_cb = NULL;
static knet_hw_tstamp_ptp_clock_index_cb_f knet_hw_tstamp_ptp_clock_index_cb = NULL;
static knet_hw_tstamp_rx_time_upscale_cb_f knet_hw_tstamp_rx_time_upscale_cb = NULL;
static knet_hw_tstamp_rx_pre_process_cb_f knet_hw_tstamp_rx_pre_process_cb = NULL;
static knet_hw_tstamp_ioctl_cmd_cb_f knet_hw_tstamp_ioctl_cmd_cb = NULL;
static knet_hw_tstamp_ptp_transport_get_cb_f knet_hw_tstamp_ptp_transport_get_cb = NULL;

typedef struct bkn_netif_cb_s {
    struct list_head list;
    knet_netif_cb_f cb;
} bkn_netif_cb_t;
LIST_HEAD(netif_create_cb_list);
LIST_HEAD(netif_destroy_cb_list);

typedef struct bkn_filter_cb_s {
    struct list_head list;
    char desc[KCOM_FILTER_DESC_MAX];
    knet_filter_cb_f cb;
} bkn_filter_cb_t;
LIST_HEAD(filter_cb_list);

/*
 * Thread management
 */

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,10))
static void
bkn_sleep(int clicks)
{
    wait_queue_head_t wq;

    init_waitqueue_head(&wq);
    sleep_on_timeout(&wq, clicks);
}
#else
static void
bkn_sleep(int clicks)
{
    wait_queue_head_t wq;

    init_waitqueue_head(&wq);
    wait_event_timeout(wq, 0, clicks);
}
#endif

/*
 * On XGS devices bit 15 fo the Transferred Bytes field in
 * the DCBs is used to indicate that kernel processing is
 * complete. Using this bit reduces the theoretically maximum
 * supported packet size from 64K to 32K, but this is still
 * adequate for 16K jumbo packets.
 */
#define SOC_DCB_KNET_DONE       0x8000
#define SOC_DCB_KNET_COUNT_MASK 0x7fff
#define SOC_DCB_META_OFFSET     2

/* Default channel configuration */
#define XGS_DMA_TX_CHAN         0
#define XGS_DMA_RX_CHAN         1

/* CMIC registers */
#define CMIC_DMA_CTRLr          0x00000100
#define CMIC_DMA_STATr          0x00000104
#define CMIC_DMA_DESC0r         0x00000110
#define CMIC_IRQ_STATr          0x00000144
#define CMIC_IRQ_MASKr          0x00000148
#define CMIC_DEV_REV_IDr        0x00000178

/* CMIC interrupts reserved for kernel handler */
#define CMIC_TXRX_IRQ_MASK      0x7f80

/* CMICm registers */
#define CMICM_CMC_BASE          0x00031000
#define CMICM_DMA_CTRLr         (CMICM_CMC_BASE + 0x00000140)
#define CMICM_DMA_STATr         (CMICM_CMC_BASE + 0x00000150)
#define CMICM_DMA_STAT_CLRr     (CMICM_CMC_BASE + 0x000001a4)
#define CMICM_DMA_DESC0r        (CMICM_CMC_BASE + 0x00000158)
#define CMICM_DMA_HALT_ADDRr    (CMICM_CMC_BASE + 0x00000120)
#define CMICM_IRQ_STATr         (CMICM_CMC_BASE + 0x00000400)
#define CMICM_IRQ_PCI_MASKr     (CMICM_CMC_BASE + 0x00000414)
#define CMICM_IRQ_UC0_MASKr     (CMICM_CMC_BASE + 0x00000428)
#define CMICM_DEV_REV_IDr       0x00010224

/* CMICm interrupts reserved for kernel handler */
#define CMICM_TXRX_IRQ_MASK     0xff00

/* CMICd increased interrupts reserved for kernel handler */
#define CMICD_CTRLD_IRQ_MASK    0x78000000

/* CMICx registers */
#define CMICX_CMC_BASE          0x00000000
#define CMICX_DMA_CTRLr         (CMICX_CMC_BASE + 0x00002100)
#define CMICX_DMA_STATr         (CMICX_CMC_BASE + 0x00002114)
#define CMICX_DMA_DESC_HIr      (CMICX_CMC_BASE + 0x00002108)
#define CMICX_DMA_DESC_LOr      (CMICX_CMC_BASE + 0x00002104)
#define CMICX_DMA_HALT_HIr      (CMICX_CMC_BASE + 0x00002110)
#define CMICX_DMA_HALT_LOr      (CMICX_CMC_BASE + 0x0000210c)
#define CMICX_IRQ_STATr         (CMICX_CMC_BASE + 0x0000106c)
#define CMICX_IRQ_STAT_CLRr     (CMICX_CMC_BASE + 0x00001074)
#define CMICX_IRQ_ENABr         0x18013100
#define INTC_INTR_ENABLE_REG4r  0x10231100
#define IHOST_GIC_GIC400_GICD_ISENABLERN_5r 0x10781114
#define IHOST_GIC_GIC400_GICD_ICENABLERN_5r 0x10781194

/* CMICx interrupts reserved for kernel handler */
#define CMICX_TXRX_IRQ_MASK     0xffffffff

/* CMICR registers */
#define CMICR_CMC_BASE          0x00000000
#define CMICR_DMA_CTRLr         (CMICR_CMC_BASE + 0x00003100)
#define CMICR_DMA_STATr         (CMICR_CMC_BASE + 0x00003114)
#define CMICR_DMA_DESC_HIr      (CMICR_CMC_BASE + 0x00003108)
#define CMICR_DMA_DESC_LOr      (CMICR_CMC_BASE + 0x00003104)
#define CMICR_DMA_HALT_HIr      (CMICR_CMC_BASE + 0x00003110)
#define CMICR_DMA_HALT_LOr      (CMICR_CMC_BASE + 0x0000310c)
#define CMICR_INTR_STATr        (CMICR_CMC_BASE + 0x00003118)
#define CMICR_INTR_ENABr        (CMICR_CMC_BASE + 0x0000311C)
#define CMICR_INTR_CLRr         (CMICR_CMC_BASE + 0x00003120)

/*
 * PAXB_0_INTC_SET_INTR_ENABLE_REG5r sets interrupt enable bit for interrupts 191 down to 160
 * Packet DMA interrupt enable bit is [168 : 199], here bit [168 : 183] is considered because it is assumed that only cmc0
 */
#define PAXB_0_INTC_SET_INTR_ENABLE_REG5r   0x0292D114
#define PAXB_0_INTC_INTR_RAW_STATUS_REG5r   0x0292D18C


/* CMICR interrupts reserved for kernel handler */
#define CMICR_TXRX_IRQ_MASK     0xffff00


#define DEV_IS_CMICR(_sinfo)    ((_sinfo)->cmic_type == 'r')
#define DEV_IS_CMICX(_sinfo)    ((_sinfo)->cmic_type == 'x')
#define DEV_IS_CMICM(_sinfo)    ((_sinfo)->cmic_type == 'm')
#define DEV_IS_CMIC(_sinfo)     ((_sinfo)->cmic_type != 0)
#define CDMA_CH(_d, _ch)        ((_d)->cdma_channels & (1 << (_ch)))

/* Unet channel */
#define UNET_CH(_d, _ch)        ((_d)->unet_channels & (1 << (_ch)))

/* Ethernet channel */
#define ENET_CH(_d, _ch)        ((_d)->enet_channels & (1 << (_ch)))

/*
 * DMA_STAT: control bits
 *
 *  xxx_SET and xxx_CLR can be WRITTEN to CMIC_DMA_STAT
 *  xxx_TST can be masked against values read from CMIC_DMA_STAT.
 *  Argument required: 0 <= ch <= 3
 */
#define DS_DMA_ACTIVE(ch)               (0x00040000 << (ch))
#define DS_DMA_EN_SET(ch)               (0x80|(ch))
#define DS_DMA_EN_CLR(ch)               (0x00|(ch))
#define DS_DMA_EN_TST(ch)               (0x00000001 << (ch))

#define DS_CHAIN_DONE_SET(ch)           (0x80|(4+(ch)))
#define DS_CHAIN_DONE_CLR(ch)           (0x00|(4+(ch)))
#define DS_CHAIN_DONE_TST(ch)           (0x00000010 << (ch))

#define DS_DESC_DONE_SET(ch)            (0x80|(8+(ch)))
#define DS_DESC_DONE_CLR(ch)            (0x00|(8+(ch)))
#define DS_DESC_DONE_TST(ch)            (0x00000100 << (ch))

#define DC_ABORT_DMA(ch)                (0x04 << (8 * (ch)))

#define DS_CMC_DESCRD_CMPLT_CLR(ch)     (0x00000001 << (ch))
#define DS_CMC_CTRLD_INT_CLR(ch)        (0x00000100 << (ch))
#define DS_CMC_DMA_ACTIVE(ch)           (0x00000100 << (ch))

#define CMICX_DS_CMC_DESC_DONE(ch)      (0x00000001 << ((ch) * 4))
#define CMICX_DS_CMC_CHAIN_DONE(ch)     (0x00000002 << ((ch) * 4))
#define CMICX_DS_CMC_CTRLD_INT(ch)      (0x00000008 << ((ch) * 4))
#define CMICX_DS_CMC_DMA_CHAIN_DONE     (0x00000001)
#define CMICX_DS_CMC_DMA_ACTIVE         (0x00000002)

#define CMICR_DS_CHAIN_DONE_INTR        (0x00000001)
#define CMICR_DS_DESC_DONE_INTR         (0x00000002)
#define CMICR_DS_DESC_CONTROLLED_INTR   (0x00000004)
#define CMICR_DS_INTR_COALESCING_INTR   (0x00000008)

#define CMICR_DS_CMC_DMA_CHAIN_DONE     (0x00000001)
#define CMICR_DS_CMC_DMA_ACTIVE         (0x00000002)

#define DMA_TO_BUS_HI(dma)              ((dma) | sinfo->dma_hi)
#define BUS_TO_DMA_HI(bus)              ((bus) & ~sinfo->dma_hi)

/*
 * DMA_CTRL: control bits
 */
#define DC_CMC_DIRECTION                (0x00000001)
#define DC_CMC_ENABLE                   (0x00000002)
#define DC_CMC_ABORT                    (0x00000004)
#define DC_CMC_CTRLD_INT                (0x00000100)
#define DC_CMC_CONTINUOUS               (0x00000200)

#define CMICX_DC_CMC_DIRECTION          (0x00000001)
#define CMICX_DC_CMC_ENABLE             (0x00000002)
#define CMICX_DC_CMC_ABORT              (0x00000004)
#define CMICX_DC_CMC_CTRLD_INT          (0x00000080)
#define CMICX_DC_CMC_CONTINUOUS         (0x00000100)

#define CMICR_DC_CMC_DIRECTION          (0x00000001)
#define CMICR_DC_CMC_ENABLE             (0x00000002)
#define CMICR_DC_CMC_ABORT              (0x00000004)
#define CMICR_DC_CMC_CTRLD_INT          (0x00000080)
#define CMICR_DC_CMC_CONTINUOUS         (0x00000100)
#define CMICR_DC_CHAIN_DONE_INTR_ENABLE        (0x00000001)
#define CMICR_DC_DESC_DONE_INTR_ENABLE         (0x00000002)
#define CMICR_DC_DESC_CONTROLLED_INTR_ENABLE   (0x00000004)
#define CMICR_DC_INTR_COALESCING_INTR_ENABLE   (0x00000008)


/* Minimum packet header size for protect underflow. */
#define CMICX_PKT_HDR_SIZE_MIN          8
/* CMICX minimum packet header size for protect underflow. */
#define CMICX_DCB_SIZE_MIN              16
/* CMICR minimum packet header size for protect underflow. */
#define CMICR_DCB_SIZE_MIN              16
/* Minimum packet header size for protect underflow. */
#define DCB_SIZE_MIN                    20
/* Maximum packet raw data size for filter validation. */
#define KNET_FILTER_RAW_MAX             256

static void
dev_read32(bkn_switch_info_t *sinfo, uint32_t address, uint32_t *value)
{
    /* Devices id started with 0x8 is always from DNX devices */
    if ((sinfo->base_id & 0x8000) == 0x8000)  {
        if (sinfo->pcie_link_status == PCIE_LINK_STATUS_DOWN) {
            /* Bypass register access when PCIE Link is down */
            return;
        }
    }

    DEV_READ32(sinfo, address, value);
}

static void
dev_write32(bkn_switch_info_t *sinfo, uint32_t address, uint32_t value)
{
    /* Devices id started with 0x8 is always from DNX devices */
    if ((sinfo->base_id & 0x8000) == 0x8000)  {
        if (sinfo->pcie_link_status == PCIE_LINK_STATUS_DOWN) {
            /* Bypass register access when PCIE Link is down */
            return;
        }
    }

    DEV_WRITE32(sinfo, address, value);
}

/*
 * Per-channel operations.
 * These are the basis for the TX/RX functions
 */
static inline void
xgs_dma_chain_clear(bkn_switch_info_t *sinfo, int chan)
{
    DBG_IRQ(("Clear chain on device %d chan %d\n",
             sinfo->dev_no, chan));
    dev_write32(sinfo, CMIC_DMA_STATr, DS_DMA_EN_CLR(chan));
    dev_write32(sinfo, CMIC_DMA_STATr, DS_CHAIN_DONE_CLR(chan));

    MEMORY_BARRIER;
}

static inline void
xgs_dma_desc_clear(bkn_switch_info_t *sinfo, int chan)
{
    uint32_t val = 0;

    DBG_IRQ(("Clear desc on device %d chan %d\n",
             sinfo->dev_no, chan));
    dev_write32(sinfo, CMIC_DMA_STATr, DS_DESC_DONE_CLR(chan));

    MEMORY_BARRIER;

    /* Flush write buffer */
    dev_read32(sinfo, CMIC_DMA_STATr, &val);

    MEMORY_BARRIER;
}

static int
xgs_dma_chan_clear(bkn_switch_info_t *sinfo, int chan)
{
    xgs_dma_chain_clear(sinfo, chan);
    xgs_dma_desc_clear(sinfo, chan);

    return 0;
}

static int
xgs_dma_chan_init(bkn_switch_info_t *sinfo, int chan, int dir)
{
    uint32_t cdc = 0;

    dev_read32(sinfo, CMIC_DMA_CTRLr, &cdc);

    cdc &= ~(0x9 << (8 * chan));
    if (dir) {
        cdc |= 0x1 << (8 * chan);
    } else {
        cdc |= 0x8 << (8 * chan);
    }

    dev_write32(sinfo, CMIC_DMA_CTRLr, cdc);

    return 0;
}

static int
xgs_dma_chan_start(bkn_switch_info_t *sinfo, int chan, uint64_t dcb)
{
    /* Write the DCB address to the DESC address for this channel */
    dev_write32(sinfo, CMIC_DMA_DESC0r + 4 * chan, dcb);

    MEMORY_BARRIER;

    /* Kick it off */
    dev_write32(sinfo, CMIC_DMA_STATr, DS_DMA_EN_SET(chan));

    MEMORY_BARRIER;

    return 0;
}

static int
xgs_dma_chan_abort(bkn_switch_info_t *sinfo, int chan, int polls)
{
    uint32_t ctrl = 0;
    uint32_t dma_stat = 0;
    int p;

    /* Clear enable */
    dev_write32(sinfo, CMIC_DMA_STATr, DS_DMA_EN_CLR(chan));

    MEMORY_BARRIER;

    /* Abort the channel */
    dev_read32(sinfo, CMIC_DMA_CTRLr, &ctrl);
    dev_write32(sinfo, CMIC_DMA_CTRLr, ctrl | DC_ABORT_DMA(chan));

    MEMORY_BARRIER;

    /* Poll for abort completion */
    for (p = 0; p < polls; p++) {
        dev_read32(sinfo, CMIC_DMA_STATr, &dma_stat);
        if (!(dma_stat & DS_DMA_ACTIVE(chan))) {
            /* Restore previous control value */
            dev_write32(sinfo, CMIC_DMA_CTRLr, ctrl);

            MEMORY_BARRIER;

            /* Clear up channel */
            xgs_dma_chan_clear(sinfo, chan);

            return polls;
        }
    }
    DBG_WARN(("DMA channel %d abort failed\n", chan));

    return -1;
}

static inline void
xgs_irq_mask_set(bkn_switch_info_t *sinfo, uint32_t mask)
{
    /* Devices id started with 0x8 is always from DNX devices */
    if ((sinfo->base_id & 0x8000) == 0x8000)  {
        if (sinfo->pcie_link_status == PCIE_LINK_STATUS_DOWN) {
            /* Bypass hw access when PCIE Link is down */
            return;
        }
    }

    if (sinfo->napi_poll_mode) {
        mask = 0;
    }
    lkbde_irq_mask_set(sinfo->dev_no | LKBDE_ISR2_DEV, CMIC_IRQ_MASKr,
                       mask, CMIC_TXRX_IRQ_MASK);
}

static inline void
xgs_irq_mask_enable(bkn_switch_info_t *sinfo, int chan, int update_hw)
{
    uint32_t mask;

    if (chan == XGS_DMA_TX_CHAN) {
        mask = 0x100;
    } else {
        mask = 0x180 << (2 * chan);
    }

    sinfo->irq_mask |= mask;

    if (update_hw) {
        xgs_irq_mask_set(sinfo, sinfo->irq_mask);
    }
}

static inline void
xgs_irq_mask_disable(bkn_switch_info_t *sinfo, int chan, int update_hw)
{
    uint32_t mask;

    if (chan == XGS_DMA_TX_CHAN) {
        mask = 0x100;
    } else {
        mask = 0x180 << (2 * chan);
    }

    sinfo->irq_mask &= ~mask;

    if (update_hw) {
        xgs_irq_mask_set(sinfo, sinfo->irq_mask);
    }
}

static inline void
xgsm_dma_chain_clear(bkn_switch_info_t *sinfo, int chan)
{
    uint32_t cdc = 0;

    /* Disabing DMA clears chain done */
    dev_read32(sinfo, CMICM_DMA_CTRLr + 4 * chan, &cdc);
    cdc &= ~(DC_CMC_ENABLE | DC_CMC_ABORT);
    dev_write32(sinfo, CMICM_DMA_CTRLr + 4 * chan, cdc);

    MEMORY_BARRIER;
}

static inline void
xgsm_dma_desc_clear(bkn_switch_info_t *sinfo, int chan)
{
    uint32_t val;

    val = DS_CMC_DESCRD_CMPLT_CLR(chan);
    if (CDMA_CH(sinfo, chan)) {
        val |= DS_CMC_CTRLD_INT_CLR(chan);
    }
    dev_write32(sinfo, CMICM_DMA_STAT_CLRr, val);

    MEMORY_BARRIER;

    /* Flush write buffer */
    dev_read32(sinfo, CMICM_DMA_STAT_CLRr, &val);

    MEMORY_BARRIER;
}

static int
xgsm_dma_chan_clear(bkn_switch_info_t *sinfo, int chan)
{
    xgsm_dma_chain_clear(sinfo, chan);
    xgsm_dma_desc_clear(sinfo, chan);

    return 0;
}

static inline void
xgsm_cdma_halt_set(bkn_switch_info_t *sinfo, int chan)
{
    dev_write32(sinfo, CMICM_DMA_HALT_ADDRr + 4 * chan,
                sinfo->halt_addr[chan]);

    MEMORY_BARRIER;
}

static int
xgsm_dma_chan_init(bkn_switch_info_t *sinfo, int chan, int dir)
{
    uint32_t cdc = 0;

    dev_read32(sinfo, CMICM_DMA_CTRLr + 4 * chan, &cdc);
    cdc &= ~DC_CMC_DIRECTION;
    if (dir) {
        cdc |= DC_CMC_DIRECTION;
    }
    if (CDMA_CH(sinfo, chan)) {
        cdc |= DC_CMC_CONTINUOUS | DC_CMC_CTRLD_INT;
        xgsm_cdma_halt_set(sinfo, chan);
    }
    dev_write32(sinfo, CMICM_DMA_CTRLr + 4 * chan, cdc);

    return 0;
}

static int
xgsm_dma_chan_start(bkn_switch_info_t *sinfo, int chan, uint64_t dcb)
{
    uint32_t cdc = 0;

    /* Write the DCB address to the DESC address for this channel */
    dev_write32(sinfo, CMICM_DMA_DESC0r + 4 * chan, dcb);

    MEMORY_BARRIER;

    /* Kick it off */
    dev_read32(sinfo, CMICM_DMA_CTRLr + 4 * chan, &cdc);
    cdc |= DC_CMC_ENABLE;
    dev_write32(sinfo, CMICM_DMA_CTRLr + 4 * chan, cdc);

    MEMORY_BARRIER;

    return 0;
}

static int
xgsm_dma_chan_abort(bkn_switch_info_t *sinfo, int chan, int polls)
{
    uint32_t ctrl = 0;
    uint32_t dma_stat = 0;
    int p;

    /* Skip abort sequence if channel is not active */
    dev_read32(sinfo, CMICM_DMA_STATr, &dma_stat);
    if (!(dma_stat & DS_CMC_DMA_ACTIVE(chan))) {
        return 0;
    }

    /* Abort the channel */
    dev_read32(sinfo, CMICM_DMA_CTRLr + 4 * chan, &ctrl);
    ctrl |= (DC_CMC_ENABLE | DC_CMC_ABORT);
    dev_write32(sinfo, CMICM_DMA_CTRLr + 4 * chan, ctrl);

    MEMORY_BARRIER;

    /* Poll for abort completion */
    for (p = 0; p < polls; p++) {
        dev_read32(sinfo, CMICM_DMA_STATr, &dma_stat);
        if (!(dma_stat & DS_CMC_DMA_ACTIVE(chan))) {
            /* Clear up channel */
            xgsm_dma_chan_clear(sinfo, chan);
            return polls;
        }
    }
    DBG_WARN(("DMA channel %d abort failed\n", chan));

    return -1;
}

static inline void
xgsm_irq_mask_set(bkn_switch_info_t *sinfo, uint32_t mask)
{
    uint32_t irq_mask_reg = CMICM_IRQ_PCI_MASKr;
    uint32_t ctrld_mask = 0;

    /* Devices id started with 0x8 is always from DNX devices */
    if ((sinfo->base_id & 0x8000) == 0x8000)  {
        if (sinfo->pcie_link_status == PCIE_LINK_STATUS_DOWN) {
            /* Bypass hw access when PCIE Link is down */
            return;
        }
    }

    if (sinfo->napi_poll_mode) {
        mask = 0;
    }
    if (sinfo->cpu_no == 1) {
        irq_mask_reg = CMICM_IRQ_UC0_MASKr;
    }

    /* Get the Controlled Interrupts mask for Continuous DMA mode */
    ctrld_mask |= (sinfo->cdma_channels << 27) & CMICD_CTRLD_IRQ_MASK;

    lkbde_irq_mask_set(sinfo->dev_no | LKBDE_ISR2_DEV, irq_mask_reg,
                       mask, CMICM_TXRX_IRQ_MASK | ctrld_mask);
}

static inline void
xgsm_irq_mask_enable(bkn_switch_info_t *sinfo, int chan, int update_hw)
{
    uint32_t mask;

    if (CDMA_CH(sinfo, chan)) {
        mask = 0x08000000 << chan;
    } else {
        if (chan == XGS_DMA_TX_CHAN) {
            mask = 0x8000;
        } else {
            mask = 0xc000 >> (2 * chan);
        }
    }

    sinfo->irq_mask |= mask;

    if (update_hw) {
        xgsm_irq_mask_set(sinfo, sinfo->irq_mask);
    }
}

static inline void
xgsm_irq_mask_disable(bkn_switch_info_t *sinfo, int chan, int update_hw)
{
    uint32_t mask;

    if (CDMA_CH(sinfo, chan)) {
        mask = 0x08000000 << chan;
    } else {
        if (chan == XGS_DMA_TX_CHAN) {
            mask = 0x8000;
        } else {
            mask = 0xc000 >> (2 * chan);
        }
    }

    sinfo->irq_mask &= ~mask;

    if (update_hw) {
        xgsm_irq_mask_set(sinfo, sinfo->irq_mask);
    }
}

static inline void
xgsx_dma_chain_clear(bkn_switch_info_t *sinfo, int chan)
{
    uint32_t ctrl = 0;
    uint32_t stat = 0;

    /* Disabing DMA clears chain done */
    dev_read32(sinfo, CMICX_DMA_CTRLr + 0x80 * chan, &ctrl);
    ctrl &= ~(CMICX_DC_CMC_ENABLE | CMICX_DC_CMC_ABORT);
    dev_write32(sinfo, CMICX_DMA_CTRLr + 0x80 * chan, ctrl);

    stat = CMICX_DS_CMC_CHAIN_DONE(chan);
    dev_write32(sinfo, CMICX_IRQ_STAT_CLRr, stat);

    MEMORY_BARRIER;

    /* Flush write buffer */
    dev_read32(sinfo, CMICX_IRQ_STAT_CLRr, &stat);

    MEMORY_BARRIER;
}

static inline void
xgsx_dma_desc_clear(bkn_switch_info_t *sinfo, int chan)
{
    uint32_t stat = 0;

    if (CDMA_CH(sinfo, chan)) {
        stat = CMICX_DS_CMC_CTRLD_INT(chan);
    } else {
        stat = CMICX_DS_CMC_DESC_DONE(chan);
    }
    dev_write32(sinfo, CMICX_IRQ_STAT_CLRr, stat);

    MEMORY_BARRIER;

    /* Flush write buffer */
    dev_read32(sinfo, CMICX_IRQ_STAT_CLRr, &stat);

    MEMORY_BARRIER;
}

static int
xgsx_dma_chan_clear(bkn_switch_info_t *sinfo, int chan)
{
    xgsx_dma_chain_clear(sinfo, chan);
    xgsx_dma_desc_clear(sinfo, chan);

    return 0;
}

static inline void
xgsx_cdma_halt_set(bkn_switch_info_t *sinfo, int chan)
{
    dev_write32(sinfo, CMICX_DMA_HALT_LOr + 0x80 * chan,
                sinfo->halt_addr[chan]);
    dev_write32(sinfo, CMICX_DMA_HALT_HIr + 0x80 * chan,
                DMA_TO_BUS_HI(sinfo->halt_addr[chan] >> 32));

    MEMORY_BARRIER;
}

static int
xgsx_dma_chan_init(bkn_switch_info_t *sinfo, int chan, int dir)
{
    uint32_t ctrl = 0;

    dev_read32(sinfo, CMICX_DMA_CTRLr + 0x80 * chan, &ctrl);
    ctrl &= ~CMICX_DC_CMC_DIRECTION;
    if (dir) {
        ctrl |= CMICX_DC_CMC_DIRECTION;
    }
    if (CDMA_CH(sinfo, chan)) {
        ctrl |= CMICX_DC_CMC_CONTINUOUS | CMICX_DC_CMC_CTRLD_INT;
        xgsx_cdma_halt_set(sinfo, chan);
    }
    dev_write32(sinfo, CMICX_DMA_CTRLr + 0x80 * chan, ctrl);

    MEMORY_BARRIER;

    return 0;
}

static int
xgsx_dma_chan_start(bkn_switch_info_t *sinfo, int chan, uint64_t dcb)
{
    uint32_t ctrl = 0;

    /* Write the DCB address to the DESC address for this channel */
    dev_write32(sinfo, CMICX_DMA_DESC_LOr + 0x80 * chan, dcb);
    dev_write32(sinfo, CMICX_DMA_DESC_HIr + 0x80 * chan, DMA_TO_BUS_HI(dcb >> 32));

    MEMORY_BARRIER;

    /* Kick it off */
    dev_read32(sinfo, CMICX_DMA_CTRLr + 0x80 * chan, &ctrl);
    ctrl |= CMICX_DC_CMC_ENABLE;
    dev_write32(sinfo, CMICX_DMA_CTRLr + 0x80 * chan, ctrl);

    MEMORY_BARRIER;

    return 0;
}

static int
xgsx_dma_chan_abort(bkn_switch_info_t *sinfo, int chan, int polls)
{
    uint32_t ctrl = 0;
    uint32_t stat = 0;
    int p;

    /* Skip abort sequence if channel is not active */
    dev_read32(sinfo, CMICX_DMA_STATr + 0x80 * chan, &stat);
    if (!(stat & CMICX_DS_CMC_DMA_ACTIVE)) {
        return 0;
    }

    /* Abort the channel */
    dev_read32(sinfo, CMICX_DMA_CTRLr + 0x80 * chan, &ctrl);
    ctrl |= CMICX_DC_CMC_ENABLE | CMICX_DC_CMC_ABORT;
    dev_write32(sinfo, CMICX_DMA_CTRLr + 0x80 * chan, ctrl);

    MEMORY_BARRIER;

    /* Poll for abort completion */
    for (p = 0; p < polls; p++) {
        dev_read32(sinfo, CMICX_DMA_STATr + 0x80 * chan, &stat);
        if (!(stat & CMICX_DS_CMC_DMA_ACTIVE)) {
            /* Clear up channel */
            xgsx_dma_chan_clear(sinfo, chan);
            return polls;
        }
    }

    DBG_WARN(("DMA channel %d abort failed\n", chan));

    return -1;
}

static inline void
xgsx_irq_fmask_get(bkn_switch_info_t *sinfo, uint32_t *fmask)
{
    int chan, bits = 4, base = 0;

    if (fmask == NULL) {
        return;
    }

    *fmask = CMICX_TXRX_IRQ_MASK;

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            *fmask &= ~(((0x1 << bits) - 1) << ((XGS_DMA_RX_CHAN + chan) * bits + base));
        }
    }
    return;
}

static inline void
xgsx_irq_mask_set(bkn_switch_info_t *sinfo, uint32_t mask)
{
    uint32_t irq_mask, irq_fmask, disable_mask;
    uint32_t fmask = CMICX_TXRX_IRQ_MASK;

    /* Devices id started with 0x8 is always from DNX devices */
    if ((sinfo->base_id & 0x8000) == 0x8000)  {
        if (sinfo->pcie_link_status == PCIE_LINK_STATUS_DOWN) {
            /* Bypass hw access when PCIE Link is down */
            return;
        }
    }

    if (sinfo->napi_poll_mode) {
        mask = 0;
    }

    xgsx_irq_fmask_get(sinfo, &fmask);
    if (sinfo->cpu_no == 1) {
        lkbde_irq_mask_get(sinfo->dev_no, &irq_mask, &irq_fmask);
        disable_mask = ~mask & (irq_mask & irq_fmask);
        if (disable_mask) {
            lkbde_irq_mask_set(sinfo->dev_no | LKBDE_ISR2_DEV | LKBDE_IPROC_REG,
                       IHOST_GIC_GIC400_GICD_ICENABLERN_5r, disable_mask, fmask);
        }
    }

    lkbde_irq_mask_set(sinfo->dev_no | LKBDE_ISR2_DEV | LKBDE_IPROC_REG,
                       sinfo->irq_mask_reg, mask, fmask);
}

static inline void
xgsx_irq_mask_enable(bkn_switch_info_t *sinfo, int chan, int update_hw)
{
    uint32_t mask;

    if (CDMA_CH(sinfo, chan)) {
        mask = CMICX_DS_CMC_CTRLD_INT(chan);
    } else {
        if (chan == XGS_DMA_TX_CHAN) {
            mask = CMICX_DS_CMC_CHAIN_DONE(chan);
        } else {
            mask = CMICX_DS_CMC_DESC_DONE(chan) |
                   CMICX_DS_CMC_CHAIN_DONE(chan);
        }
    }

    sinfo->irq_mask |= mask;

    if (update_hw) {
        xgsx_irq_mask_set(sinfo, sinfo->irq_mask);
    }
}

static inline void
xgsx_irq_mask_disable(bkn_switch_info_t *sinfo, int chan, int update_hw)
{
    uint32_t mask;

    if (CDMA_CH(sinfo, chan)) {
        mask = CMICX_DS_CMC_CTRLD_INT(chan);
    } else {
        if (chan == XGS_DMA_TX_CHAN) {
            mask = CMICX_DS_CMC_CHAIN_DONE(chan);
        } else {
            mask = CMICX_DS_CMC_DESC_DONE(chan) |
                   CMICX_DS_CMC_CHAIN_DONE(chan);
        }
    }

    sinfo->irq_mask &= ~mask;

    if (update_hw) {
        xgsx_irq_mask_set(sinfo, sinfo->irq_mask);
    }
}

static inline void
xgsr_dma_chain_clear(bkn_switch_info_t *sinfo, int chan)
{
    uint32_t ctrl = 0;
    uint32_t stat = 0;

    /* Disabing DMA clears chain done */
    dev_read32(sinfo, CMICR_DMA_CTRLr + 0x80 * chan, &ctrl);
    ctrl &= ~(CMICR_DC_CMC_ENABLE | CMICR_DC_CMC_ABORT);
    dev_write32(sinfo, CMICR_DMA_CTRLr + 0x80 * chan, ctrl);

    stat = CMICR_DS_CHAIN_DONE_INTR;
    dev_write32(sinfo, CMICR_INTR_CLRr + 0x80 * chan, stat);

    MEMORY_BARRIER;

    /* Flush write buffer */
    dev_read32(sinfo, CMICR_INTR_CLRr + 0x80 * chan, &stat);

    MEMORY_BARRIER;
}

static inline void
xgsr_dma_desc_clear(bkn_switch_info_t *sinfo, int chan)
{
    uint32_t stat = 0;

    if (CDMA_CH(sinfo, chan)) {
        stat = CMICR_DS_DESC_CONTROLLED_INTR;
    } else {
        stat = CMICR_DS_DESC_DONE_INTR;
    }
    dev_write32(sinfo, CMICR_INTR_CLRr + 0x80 * chan, stat);

    MEMORY_BARRIER;

    /* Flush write buffer */
    dev_read32(sinfo, CMICR_INTR_CLRr + 0x80 * chan, &stat);

    MEMORY_BARRIER;
}

static int
xgsr_dma_chan_clear(bkn_switch_info_t *sinfo, int chan)
{
    xgsr_dma_chain_clear(sinfo, chan);
    xgsr_dma_desc_clear(sinfo, chan);

    return 0;
}

static inline void
xgsr_cdma_halt_set(bkn_switch_info_t *sinfo, int chan)
{
    dev_write32(sinfo, CMICR_DMA_HALT_LOr + 0x80 * chan,
                sinfo->halt_addr[chan]);
    dev_write32(sinfo, CMICR_DMA_HALT_HIr + 0x80 * chan,
                DMA_TO_BUS_HI(sinfo->halt_addr[chan] >> 32));

    MEMORY_BARRIER;
}

static int
xgsr_dma_chan_init(bkn_switch_info_t *sinfo, int chan, int dir)
{
    uint32_t ctrl = 0;

    dev_read32(sinfo, CMICR_DMA_CTRLr + 0x80 * chan, &ctrl);
    ctrl &= ~CMICR_DC_CMC_DIRECTION;
    if (dir) {
        ctrl |= CMICR_DC_CMC_DIRECTION;
    }
    if (CDMA_CH(sinfo, chan)) {
        ctrl |= CMICR_DC_CMC_CONTINUOUS | CMICR_DC_CMC_CTRLD_INT;
        xgsr_cdma_halt_set(sinfo, chan);
    }
    dev_write32(sinfo, CMICR_DMA_CTRLr + 0x80 * chan, ctrl);

    MEMORY_BARRIER;

    return 0;
}

static int
xgsr_dma_chan_start(bkn_switch_info_t *sinfo, int chan, uint64_t dcb)
{
    uint32_t ctrl = 0;

    /* Write the DCB address to the DESC address for this channel */
    dev_write32(sinfo, CMICR_DMA_DESC_LOr + 0x80 * chan, dcb);
    dev_write32(sinfo, CMICR_DMA_DESC_HIr + 0x80 * chan, DMA_TO_BUS_HI(dcb >> 32));

    MEMORY_BARRIER;

    /* Kick it off */
    dev_read32(sinfo, CMICR_DMA_CTRLr + 0x80 * chan, &ctrl);
    ctrl |= CMICR_DC_CMC_ENABLE;
    dev_write32(sinfo, CMICR_DMA_CTRLr + 0x80 * chan, ctrl);

    MEMORY_BARRIER;

    return 0;
}

static int
xgsr_dma_chan_abort(bkn_switch_info_t *sinfo, int chan, int polls)
{
    uint32_t ctrl = 0;
    uint32_t stat = 0;
    int p;

    /* Skip abort sequence if channel is not active */
    dev_read32(sinfo, CMICR_DMA_STATr + 0x80 * chan, &stat);
    if (!(stat & CMICR_DS_CMC_DMA_ACTIVE)) {
        return 0;
    }

    /* Abort the channel */
    dev_read32(sinfo, CMICR_DMA_CTRLr + 0x80 * chan, &ctrl);
    ctrl |= CMICR_DC_CMC_ENABLE | CMICR_DC_CMC_ABORT;
    dev_write32(sinfo, CMICR_DMA_CTRLr + 0x80 * chan, ctrl);

    MEMORY_BARRIER;

    /* Poll for abort completion */
    for (p = 0; p < polls; p++) {
        dev_read32(sinfo, CMICR_DMA_STATr + 0x80 * chan, &stat);
        if (!(stat & CMICR_DS_CMC_DMA_ACTIVE)) {
            /* Clear up channel */
            xgsr_dma_chan_clear(sinfo, chan);
            return polls;
        }
    }

    DBG_WARN(("DMA channel %d abort failed\n", chan));

    return -1;
}

static inline void
xgsr_irq_fmask_get(bkn_switch_info_t *sinfo, uint32_t *fmask)
{
    int chan, bits = 1, base = 8;

    if (fmask == NULL) {
        return;
    }

    *fmask = CMICR_TXRX_IRQ_MASK;

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            *fmask &= ~(((0x1 << bits) - 1) << ((XGS_DMA_RX_CHAN + chan) * bits + base));
        }
    }
    return;
}

static inline void
xgsr_irq_mask_set(bkn_switch_info_t *sinfo, uint32_t mask)
{
    uint32_t irq_mask_reg = PAXB_0_INTC_SET_INTR_ENABLE_REG5r;
    uint32_t fmask = CMICR_TXRX_IRQ_MASK;

    if ((sinfo->base_id & 0x8000) == 0x8000)  {
        /* Devices id started with 0x8 is always from DNX devices */
        if (sinfo->pcie_link_status == PCIE_LINK_STATUS_DOWN) {
            /* Bypass hw access when PCIE Link is down */
            return;
        }
    }

    if (sinfo->napi_poll_mode) {
        mask = 0;
    }

    xgsr_irq_fmask_get(sinfo, &fmask);
    if (sinfo->cpu_no == 1) {
        DBG_WARN(("IHost(AXI) is not supported.\n"));
    }

    lkbde_irq_mask_set(sinfo->dev_no | LKBDE_ISR2_DEV | LKBDE_IPROC_REG,
                       irq_mask_reg, mask, fmask);
}

static inline void
xgsr_irq_mask_enable(bkn_switch_info_t *sinfo, int chan, int update_hw)
{
    uint32_t enabler;
    int bits = 1, base = 8;

    if (CDMA_CH(sinfo, chan)) {
        dev_read32(sinfo, CMICR_INTR_ENABr + 0x80 * chan, &enabler);
        enabler |= CMICR_DC_DESC_CONTROLLED_INTR_ENABLE;
        dev_write32(sinfo, CMICR_INTR_ENABr + 0x80 * chan, enabler);
    } else {
        if (chan == XGS_DMA_TX_CHAN) {
            dev_read32(sinfo, CMICR_INTR_ENABr + 0x80 * chan, &enabler);
            enabler |= CMICR_DC_CHAIN_DONE_INTR_ENABLE;
            dev_write32(sinfo, CMICR_INTR_ENABr + 0x80 * chan, enabler);
        } else {
            dev_read32(sinfo, CMICR_INTR_ENABr + 0x80 * chan, &enabler);
            enabler |= CMICR_DC_DESC_DONE_INTR_ENABLE;
            enabler |= CMICR_DC_CHAIN_DONE_INTR_ENABLE;
            dev_write32(sinfo, CMICR_INTR_ENABr + 0x80 * chan, enabler);

        }
    }

    sinfo->irq_mask |= (((0x1 << bits) - 1) << (chan * bits + base));

    if (update_hw) {
        xgsr_irq_mask_set(sinfo, sinfo->irq_mask);
    }
}

static inline void
xgsr_irq_mask_disable(bkn_switch_info_t *sinfo, int chan, int update_hw)
{
    uint32_t enabler;
    int bits = 1, base = 8;

    if (CDMA_CH(sinfo, chan)) {
        dev_read32(sinfo, CMICR_INTR_ENABr + 0x80 * chan, &enabler);
        enabler &= ~CMICR_DC_DESC_CONTROLLED_INTR_ENABLE;
        dev_write32(sinfo, CMICR_INTR_ENABr + 0x80 * chan, enabler);
    } else {
        if (chan == XGS_DMA_TX_CHAN) {
            dev_read32(sinfo, CMICR_INTR_ENABr + 0x80 * chan, &enabler);
            enabler &= ~CMICR_DC_CHAIN_DONE_INTR_ENABLE;
            dev_write32(sinfo, CMICR_INTR_ENABr + 0x80 * chan, enabler);
        } else {
            dev_read32(sinfo, CMICR_INTR_ENABr + 0x80 * chan, &enabler);
            enabler &= ~CMICR_DC_DESC_DONE_INTR_ENABLE;
            enabler &= ~CMICR_DC_CHAIN_DONE_INTR_ENABLE;
            dev_write32(sinfo, CMICR_INTR_ENABr + 0x80 * chan, enabler);
        }
    }

    sinfo->irq_mask &= ~(((0x1 << bits) - 1) << (chan * bits + base));

    if (update_hw) {
        xgsr_irq_mask_set(sinfo, sinfo->irq_mask);
    }
}

static inline void
dev_dma_chain_clear(bkn_switch_info_t *sinfo, int chan)
{
    if (DEV_IS_CMICR(sinfo)) {
        xgsr_dma_chain_clear(sinfo, chan);
    } else if (DEV_IS_CMICX(sinfo)) {
        xgsx_dma_chain_clear(sinfo, chan);
    } else if (DEV_IS_CMICM(sinfo)) {
        xgsm_dma_chain_clear(sinfo, chan);
    } else {
        xgs_dma_chain_clear(sinfo, chan);
    }
}

static inline void
dev_dma_desc_clear(bkn_switch_info_t *sinfo, int chan)
{
    if (DEV_IS_CMICR(sinfo)) {
        xgsr_dma_desc_clear(sinfo, chan);
    } else if (DEV_IS_CMICX(sinfo)) {
        xgsx_dma_desc_clear(sinfo, chan);
    } else if (DEV_IS_CMICM(sinfo)) {
        xgsm_dma_desc_clear(sinfo, chan);
    } else {
        xgs_dma_desc_clear(sinfo, chan);
    }
}

static int
dev_dma_chan_clear(bkn_switch_info_t *sinfo, int chan)
{
    if (DEV_IS_CMICR(sinfo)) {
        return xgsr_dma_chan_clear(sinfo, chan);
    } else if (DEV_IS_CMICX(sinfo)) {
        return xgsx_dma_chan_clear(sinfo, chan);
    } else if (DEV_IS_CMICM(sinfo)) {
        return xgsm_dma_chan_clear(sinfo, chan);
    } else {
        return xgs_dma_chan_clear(sinfo, chan);
    }
}

static void
dev_cdma_halt_set(bkn_switch_info_t *sinfo, int chan)
{
    if (DEV_IS_CMICR(sinfo)) {
        xgsr_cdma_halt_set(sinfo, chan);
    } else if (DEV_IS_CMICX(sinfo)) {
        xgsx_cdma_halt_set(sinfo, chan);
    } else if (DEV_IS_CMICM(sinfo)) {
        xgsm_cdma_halt_set(sinfo, chan);
    }
}

static int
dev_dma_chan_init(bkn_switch_info_t *sinfo, int chan, int dir)
{
    if (DEV_IS_CMICR(sinfo)) {
        return xgsr_dma_chan_init(sinfo, chan, dir);
    } else if (DEV_IS_CMICX(sinfo)) {
        return xgsx_dma_chan_init(sinfo, chan, dir);
    } else if (DEV_IS_CMICM(sinfo)) {
        return xgsm_dma_chan_init(sinfo, chan, dir);
    } else {
        return xgs_dma_chan_init(sinfo, chan, dir);
    }
}

static int
dev_dma_chan_start(bkn_switch_info_t *sinfo, int chan, uint64_t dcb)
{
    if (DEV_IS_CMICR(sinfo)) {
        return xgsr_dma_chan_start(sinfo, chan, dcb);
    } else if (DEV_IS_CMICX(sinfo)) {
        return xgsx_dma_chan_start(sinfo, chan, dcb);
    } else if (DEV_IS_CMICM(sinfo)) {
        return xgsm_dma_chan_start(sinfo, chan, dcb);
    } else {
        return xgs_dma_chan_start(sinfo, chan, dcb);
    }
}

static int
dev_dma_chan_abort(bkn_switch_info_t *sinfo, int chan, int polls)
{
    if (DEV_IS_CMICR(sinfo)) {
        return xgsr_dma_chan_abort(sinfo, chan, polls);
    } else if (DEV_IS_CMICX(sinfo)) {
        return xgsx_dma_chan_abort(sinfo, chan, polls);
    } else if (DEV_IS_CMICM(sinfo)) {
        return xgsm_dma_chan_abort(sinfo, chan, polls);
    } else {
        return xgs_dma_chan_abort(sinfo, chan, polls);
    }
}

static inline void
dev_irq_mask_set(bkn_switch_info_t *sinfo, uint32_t mask)
{
    if (DEV_IS_CMICR(sinfo)) {
        xgsr_irq_mask_set(sinfo, mask);
    } else if (DEV_IS_CMICX(sinfo)) {
        xgsx_irq_mask_set(sinfo, mask);
    } else if (DEV_IS_CMICM(sinfo)) {
        xgsm_irq_mask_set(sinfo, mask);
    } else {
        xgs_irq_mask_set(sinfo, mask);
    }
}

static inline void
dev_irq_mask_enable(bkn_switch_info_t *sinfo, int chan, int update_hw)
{
    if (DEV_IS_CMICR(sinfo)) {
        xgsr_irq_mask_enable(sinfo, chan, update_hw);
    } else if (DEV_IS_CMICX(sinfo)) {
        xgsx_irq_mask_enable(sinfo, chan, update_hw);
    } else if (DEV_IS_CMICM(sinfo)) {
        xgsm_irq_mask_enable(sinfo, chan, update_hw);
    } else {
        xgs_irq_mask_enable(sinfo, chan, update_hw);
    }
}

static void
dev_irq_mask_disable(bkn_switch_info_t *sinfo, int chan, int update_hw)
{
    if (DEV_IS_CMICR(sinfo)) {
        xgsr_irq_mask_disable(sinfo, chan, update_hw);
    } else if (DEV_IS_CMICX(sinfo)) {
        xgsx_irq_mask_disable(sinfo, chan, update_hw);
    } else if (DEV_IS_CMICM(sinfo)) {
        xgsm_irq_mask_disable(sinfo, chan, update_hw);
    } else {
        xgs_irq_mask_disable(sinfo, chan, update_hw);
    }
}

static int
bkn_alloc_dcbs(bkn_switch_info_t *sinfo)
{
    int dcb_size;
    int tx_ring_size, rx_ring_size;
    dma_addr_t dcb_dma = 0;

    dcb_size = sinfo->dcb_wsize * sizeof(uint32_t);
    tx_ring_size = dcb_size * (MAX_TX_DCBS + 1);
    rx_ring_size = dcb_size * (MAX_RX_DCBS + 1);
    sinfo->dcb_mem_size = tx_ring_size + rx_ring_size * sinfo->rx_chans;

    sinfo->dcb_mem = BKN_DMA_ALLOC_COHERENT(sinfo->dma_dev,
                                        sinfo->dcb_mem_size,
                                        &dcb_dma);
    if (sinfo->dcb_mem == NULL) {
        gprintk("DCB memory allocation (%d bytes) failed.\n",
                sinfo->dcb_mem_size);
        return -ENOMEM;
    }
    sinfo->dcb_dma = (uint64_t)dcb_dma;

    return 0;
}

static void
bkn_free_dcbs(bkn_switch_info_t *sinfo)
{
    if (sinfo->dcb_mem != NULL) {
        BKN_DMA_FREE_COHERENT(sinfo->dma_dev, sinfo->dcb_mem_size,
                          sinfo->dcb_mem, (dma_addr_t)sinfo->dcb_dma);
        sinfo->dcb_mem = NULL;
    }
}

static void
bkn_clean_tx_dcbs(bkn_switch_info_t *sinfo)
{
    bkn_desc_info_t *desc;

    DBG_DCB_TX(("Cleaning Tx DCBs (%d %d).\n",
                sinfo->tx.cur, sinfo->tx.dirty));
    while (sinfo->tx.free < MAX_TX_DCBS) {
        desc = &sinfo->tx.desc[sinfo->tx.dirty];
        if (desc->skb != NULL) {
            DBG_SKB(("Cleaning Tx SKB from DCB %d.\n",
                     sinfo->tx.dirty));
            BKN_DMA_UNMAP_SINGLE(sinfo->dma_dev,
                             desc->skb_dma, desc->dma_size,
                             BKN_DMA_TODEV);
            desc->skb_dma = 0;
            dev_kfree_skb_any(desc->skb);
            desc->skb = NULL;
        }
        if (++sinfo->tx.dirty >= MAX_TX_DCBS) {
            sinfo->tx.dirty = 0;
        }
        sinfo->tx.free++;
    }
    sinfo->tx.api_active = 0;
    DBG_DCB_TX(("Cleaned Tx DCBs (%d %d).\n",
                sinfo->tx.cur, sinfo->tx.dirty));
}

static void
bkn_clean_rx_dcbs(bkn_switch_info_t *sinfo, int chan)
{
    bkn_desc_info_t *desc;

    DBG_DCB_RX(("Cleaning Rx%d DCBs (%d %d).\n",
                chan, sinfo->rx[chan].cur, sinfo->rx[chan].dirty));
    while (sinfo->rx[chan].free) {
        desc = &sinfo->rx[chan].desc[sinfo->rx[chan].dirty];
        if (desc->skb != NULL) {
            DBG_SKB(("Cleaning Rx%d SKB from DCB %d.\n",
                     chan, sinfo->rx[chan].dirty));
            BKN_DMA_UNMAP_SINGLE(sinfo->dma_dev,
                             desc->skb_dma, desc->dma_size,
                             BKN_DMA_FROMDEV);
            desc->skb_dma = 0;
            dev_kfree_skb_any(desc->skb);
            desc->skb = NULL;
        }
        if (++sinfo->rx[chan].dirty >= MAX_RX_DCBS) {
            sinfo->rx[chan].dirty = 0;
        }
        sinfo->rx[chan].free--;
    }
    sinfo->rx[chan].running = 0;
    sinfo->rx[chan].api_active = 0;
    DBG_DCB_RX(("Cleaned Rx%d DCBs (%d %d).\n",
                chan, sinfo->rx[chan].cur, sinfo->rx[chan].dirty));
}

static void
bkn_clean_dcbs(bkn_switch_info_t *sinfo)
{
    int chan;

    bkn_clean_tx_dcbs(sinfo);

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        bkn_clean_rx_dcbs(sinfo, chan);
    }
}

static void
bkn_init_dcbs(bkn_switch_info_t *sinfo)
{
    int dcb_size;
    uint32_t *dcb_mem;
    uint64_t dcb_dma;
    bkn_desc_info_t *desc;
    int idx;
    int chan;

    memset(sinfo->dcb_mem, 0, sinfo->dcb_mem_size);

    dcb_size = sinfo->dcb_wsize * sizeof(uint32_t);
    dcb_mem = sinfo->dcb_mem;
    dcb_dma = sinfo->dcb_dma;

    for (idx = 0; idx < (MAX_TX_DCBS + 1); idx++) {
        if (CDMA_CH(sinfo, XGS_DMA_TX_CHAN)) {
            if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                dcb_mem[2] |= 1 << 24 | 1 << 16;
            } else {
                dcb_mem[1] |= 1 << 24 | 1 << 16;
            }
            if (idx == MAX_TX_DCBS) {
                if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                    dcb_mem[0] = sinfo->tx.desc[0].dcb_dma;
                    dcb_mem[1] = DMA_TO_BUS_HI(sinfo->tx.desc[0].dcb_dma >> 32);
                    dcb_mem[2] |= 1 << 18;
                } else {
                    dcb_mem[0] = sinfo->tx.desc[0].dcb_dma;
                    dcb_mem[1] |= 1 << 18;
                }
            }
        }
        desc = &sinfo->tx.desc[idx];
        desc->dcb_mem = dcb_mem;
        desc->dcb_dma = dcb_dma;
        dcb_mem += sinfo->dcb_wsize;
        dcb_dma += dcb_size;
    }
    sinfo->halt_addr[XGS_DMA_TX_CHAN] = sinfo->tx.desc[0].dcb_dma;
    sinfo->tx.free = MAX_TX_DCBS;
    sinfo->tx.cur = 0;
    sinfo->tx.dirty = 0;

    DBG_DCB_TX(("Tx DCBs @ 0x%08x.\n",
                (uint32_t)sinfo->tx.desc[0].dcb_dma));

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        for (idx = 0; idx < (MAX_RX_DCBS + 1); idx++) {
            if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
                if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                    dcb_mem[2] |= 1 << 24 | 1 << 16;
                } else {
                    dcb_mem[1] |= 1 << 24 | 1 << 16;
                }
                if (idx == MAX_RX_DCBS) {
                    if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                        dcb_mem[0] = sinfo->rx[chan].desc[0].dcb_dma;
                        dcb_mem[1] = DMA_TO_BUS_HI(sinfo->rx[chan].desc[0].dcb_dma >> 32);
                        dcb_mem[2] |= 1 << 18;
                    } else {
                        dcb_mem[0] = sinfo->rx[chan].desc[0].dcb_dma;
                        dcb_mem[1] |= 1 << 18;
                    }
                }
            }
            desc = &sinfo->rx[chan].desc[idx];
            desc->dcb_mem = dcb_mem;
            desc->dcb_dma = dcb_dma;
            dcb_mem += sinfo->dcb_wsize;
            dcb_dma += dcb_size;
        }
        sinfo->halt_addr[XGS_DMA_RX_CHAN + chan] = sinfo->rx[chan].desc[MAX_RX_DCBS].dcb_dma;
        sinfo->rx[chan].free = 0;
        sinfo->rx[chan].cur = 0;
        sinfo->rx[chan].dirty = 0;

        DBG_DCB_RX(("Rx%d DCBs @ 0x%08x.\n",
                    chan, (uint32_t)sinfo->rx[chan].desc[0].dcb_dma));
    }
}

static void
bkn_dump_dcb(char *prefix, uint32_t *dcb, int wsize, int txrx)
{
    if (XGS_DMA_TX_CHAN == txrx) {
        if (wsize > 4) {
            DBG_DCB_TX(("%s: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x ... 0x%08x\n",
                        prefix, dcb[0], dcb[1], dcb[2], dcb[3], dcb[4], dcb[5],
                        dcb[wsize - 1]));
        } else {
            DBG_DCB_TX(("%s: 0x%08x 0x%08x 0x%08x 0x%08x\n",
                        prefix, dcb[0], dcb[1], dcb[2], dcb[3]));
        }
    } else {
        if (wsize > 4) {
            DBG_DCB_RX(("%s: 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x ... 0x%08x\n",
                        prefix, dcb[0], dcb[1], dcb[2], dcb[3], dcb[4], dcb[5],
                        dcb[wsize - 1]));
        } else {
            DBG_DCB_RX(("%s: 0x%08x 0x%08x 0x%08x 0x%08x\n",
                        prefix, dcb[0], dcb[1], dcb[2], dcb[3]));
        }
    }
}

static void
bkn_dump_pkt(uint8_t *data, int size, int txrx)
{
    int idx;
    char str[128];

    if (!(debug & DBG_LVL_PDMP) &&
        !(txrx == XGS_DMA_TX_CHAN && debug & DBG_LVL_PDMP_TX) &&
        !(txrx == XGS_DMA_RX_CHAN && debug & DBG_LVL_PDMP_RX)) {
        return;
    }

    for (idx = 0; idx < size; idx++) {
        if ((idx & 0xf) == 0) {
            sprintf(str, "%04x: ", idx);
        }
        if ((idx & 0xf) == 8) {
            sprintf(&str[strlen(str)], "- ");
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

static bkn_switch_info_t *
bkn_sinfo_from_unit(int unit)
{
    struct list_head *list;
    bkn_switch_info_t *sinfo;

    list_for_each(list, &_sinfo_list) {
        sinfo = (bkn_switch_info_t *)list;
        if (sinfo->dev_no == unit || unit == -1) {
            return sinfo;
        }
    }
    return NULL;
}

static void
bkn_cdma_goto(bkn_switch_info_t *sinfo, int chan, uint64_t dcb)
{
    if (sinfo->basedev_suspended) {
        return;
    }

    /* Set the new halt location */
    sinfo->halt_addr[chan] = dcb;
    dev_cdma_halt_set(sinfo, chan);
}

static void
bkn_api_rx_restart(bkn_switch_info_t *sinfo, int chan)
{
    bkn_dcb_chain_t *dcb_chain;
    int start_dma;

    if (sinfo->basedev_suspended) {
        return;
    }

    /* If resume from basedev suspended, there could be a suspended chain */
    if (sinfo->rx[chan].api_dcb_chain) {
        return;
    }

    /* Assume that driver lock is held */
    if (!list_empty(&sinfo->rx[chan].api_dcb_list)) {
        dcb_chain = list_entry(sinfo->rx[chan].api_dcb_list.next,
                               bkn_dcb_chain_t, list);
        start_dma = 0;
        if (sinfo->rx[chan].use_rx_skb == 0) {
            sinfo->rx[chan].chain_complete = 0;
            start_dma = 1;
            if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan) &&
                sinfo->rx[chan].api_active) {
                /* HW is running already, so we just move to the next chain */
                start_dma = 0;
            }
        }
        sinfo->rx[chan].api_dcb_chain = dcb_chain;
        sinfo->rx[chan].api_active = 1;
        if (start_dma) {
            DBG_DCB_RX(("Start API Rx DMA, first DCB @ 0x%08x (%d DCBs).\n",
                        (uint32_t)dcb_chain->dcb_dma, dcb_chain->dcb_cnt));
            dev_dma_chan_clear(sinfo, XGS_DMA_RX_CHAN + chan);
            dev_irq_mask_enable(sinfo, XGS_DMA_RX_CHAN + chan, 1);
            dev_dma_chan_start(sinfo, XGS_DMA_RX_CHAN + chan, dcb_chain->dcb_dma);
        }

        list_del(&dcb_chain->list);
    }
}

static void
bkn_api_rx_chain_done(bkn_switch_info_t *sinfo, int chan)
{
    DBG_DCB_RX(("API Rx DMA chain done\n"));

    if (!CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
        sinfo->rx[chan].api_active = 0;
    }
    if (sinfo->rx[chan].api_dcb_chain) {
        kfree(sinfo->rx[chan].api_dcb_chain);
        sinfo->rx[chan].api_dcb_chain = NULL;
    }
    bkn_api_rx_restart(sinfo, chan);
}

static int
bkn_api_rx_copy_from_skb(bkn_switch_info_t *sinfo,
                         int chan, bkn_desc_info_t *desc, int rx_hwts)
{
    bkn_dcb_chain_t *dcb_chain;
    uint32_t *dcb;
    uint32_t dcb_stat;
    uint8_t *pkt;
    uint8_t *skb_pkt;
    uint64_t pkt_dma;
    int pktlen;
    int i;
    bkn_evt_resource_t *evt;

    dcb_stat = desc->dcb_mem[sinfo->dcb_wsize-1];
    pktlen = dcb_stat & SOC_DCB_KNET_COUNT_MASK;

    dcb_chain = sinfo->rx[chan].api_dcb_chain;
    if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan) && dcb_chain == NULL) {
        /* Last chain done, try to get a new one */
        bkn_api_rx_chain_done(sinfo, chan);
        dcb_chain = sinfo->rx[chan].api_dcb_chain;
    }
    if (dcb_chain == NULL) {
        DBG_WARN(("No Rx API buffers\n"));
        sinfo->rx[chan].pkts_d_no_api_buf++;
        return -1;
    }
    dcb = &dcb_chain->dcb_mem[dcb_chain->dcb_cur * sinfo->dcb_wsize];
    if ((((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) && (dcb[2] & 0xffff) < pktlen) ||
        (((sinfo->cmic_type != 'x') && (sinfo->cmic_type != 'r')) && (dcb[1] & 0xffff) < pktlen)) {
        DBG_WARN(("Rx API buffer too small\n"));
        return -1;
    }
    if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
        pkt_dma = BUS_TO_DMA_HI(dcb[1]);
        pkt_dma = pkt_dma << 32 | dcb[0];
    } else {
        pkt_dma = dcb[0];
    }
    pkt = (uint8_t *)kernel_bde->p2l(sinfo->dev_no, (sal_paddr_t)pkt_dma);
    if (pkt == NULL) {
        DBG_WARN(("Invalid Rx API buffer\n"));
        return -1;
    }

    skb_pkt = desc->skb->data;

    /* Strip custom header from KNETSync packets.  */
    if ((rx_hwts) &&
        ((skb_pkt[0] == 'B') && (skb_pkt[1] == 'C') &&
         (skb_pkt[2] == 'M') && (skb_pkt[3] == 'C'))) {

        skb_pkt = skb_pkt + skb_pkt[4];
    }

    /* Copy packet data */
    memcpy(pkt, skb_pkt, pktlen);

    /* Copy packet metadata and mark as done */
    if ((sinfo->cmic_type != 'x') && (sinfo->cmic_type != 'r')) {
        for (i = SOC_DCB_META_OFFSET; i < sinfo->dcb_wsize; i++) {
            dcb[i] = desc->dcb_mem[i];
        }
    }
    dcb[sinfo->dcb_wsize-1] = dcb_stat | SOC_DCB_KNET_DONE;
    MEMORY_BARRIER;

    dcb_chain->dcb_cur++;

    if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
        dcb = &dcb_chain->dcb_mem[dcb_chain->dcb_cur * sinfo->dcb_wsize];
        if ((((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) && dcb[2] & (1 << 18)) ||
            (((sinfo->cmic_type != 'x') && (sinfo->cmic_type != 'r')) && dcb[1] & (1 << 18))) {
            /* Get the next chain if reload done */
            dcb[sinfo->dcb_wsize-1] |= 1 << 31 | SOC_DCB_KNET_DONE;
            MEMORY_BARRIER;
            bkn_api_rx_chain_done(sinfo, chan);
            dcb_chain = sinfo->rx[chan].api_dcb_chain;
            if (dcb_chain == NULL) {
                sinfo->rx[chan].api_dcb_chain_end = NULL;
            }
        }
    } else {
        if ((((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) && (dcb[2] & (1 << 16)) == 0) ||
            (((sinfo->cmic_type != 'x') && (sinfo->cmic_type != 'r')) && (dcb[1] & (1 << 16)) == 0)) {
            bkn_api_rx_chain_done(sinfo, chan);
        }
    }

    sinfo->dma_events |= KCOM_DMA_INFO_F_RX_DONE;

    evt = &_bkn_evt[sinfo->evt_idx];
    evt->evt_wq_put++;
    wake_up_interruptible(&evt->evt_wq);

    return 0;
}

static void
bkn_rx_refill(bkn_switch_info_t *sinfo, int chan)
{
    struct sk_buff *skb;
    bkn_desc_info_t *desc;
    uint32_t *dcb;
    uint32_t resv_size = ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) ? RCPU_HDR_SIZE : RCPU_RX_ENCAP_SIZE;
    uint32_t meta_size = ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) ? RCPU_RX_META_SIZE : 0;
    int prev;

    if (sinfo->rx[chan].use_rx_skb == 0) {
        /* Rx buffers are provided by BCM Rx API */
        return;
    }

    if (!CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan) &&
        sinfo->rx[chan].tokens < MAX_RX_DCBS) {
        /* Pause DMA for now */
        return;
    }

    /* Add meta data length */
    resv_size += sinfo->pkt_hdr_size;

    while (sinfo->rx[chan].free < MAX_RX_DCBS) {
        desc = &sinfo->rx[chan].desc[sinfo->rx[chan].cur];
        if (desc->skb == NULL) {
            skb = dev_alloc_skb(rx_buffer_size + SKB_DATA_ALIGN(resv_size));
            if (skb == NULL) {
                break;
            }
            /* Reserve buffer space for RCPU encapsulation if needed */
            skb_reserve(skb, SKB_DATA_ALIGN(resv_size));
            desc->skb = skb;
        } else {
            DBG_DCB_RX(("Refill Rx%d SKB in DCB %d recycled.\n",
                        chan, sinfo->rx[chan].cur));
        }
        skb = desc->skb;
        desc->dma_size = rx_buffer_size + meta_size;
#ifdef KNET_NO_AXI_DMA_INVAL





        /*
         * Cache invalidate may corrupt DMA memory on some iProc-based devices
         * if the kernel is mapped to ACP memory.
         */
        if (sinfo->pdev == NULL) {
            desc->dma_size = 0;
        }
#endif
        desc->skb_dma = BKN_DMA_MAP_SINGLE(sinfo->dma_dev,
                                       skb->data, desc->dma_size,
                                       BKN_DMA_FROMDEV);
        if (BKN_DMA_MAPPING_ERROR(sinfo->dma_dev, desc->skb_dma)) {
            dev_kfree_skb_any(skb);
            desc->skb = NULL;
            break;
        }
        DBG_DCB_RX(("Refill Rx%d DCB %d (0x%08x).\n",
                    chan, sinfo->rx[chan].cur, (uint32_t)desc->skb_dma));
        dcb = desc->dcb_mem;
        dcb[0] = desc->skb_dma;
        if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                dcb[2] |= 1 << 24 | 1 << 16;
            } else {
                dcb[1] |= 1 << 24 | 1 << 16;
            }
        } else {
            prev = PREV_IDX(sinfo->rx[chan].cur, MAX_RX_DCBS);
            if (prev < (MAX_RX_DCBS - 1)) {
                if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                    sinfo->rx[chan].desc[prev].dcb_mem[2] |= 1 << 16;
                } else {
                    sinfo->rx[chan].desc[prev].dcb_mem[1] |= 1 << 16;
                }
            }
        }
        if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
            dcb[1] = DMA_TO_BUS_HI(desc->skb_dma >> 32);
            dcb[2] |= rx_buffer_size + meta_size;
        } else {
            dcb[1] |= rx_buffer_size;
        }

        if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan) &&
            sinfo->rx[chan].tokens > MAX_RX_DCBS) {
            /* DMA run to the new halt location */
            bkn_cdma_goto(sinfo, XGS_DMA_RX_CHAN + chan, desc->dcb_dma);
        }

        if (++sinfo->rx[chan].cur >= MAX_RX_DCBS) {
            sinfo->rx[chan].cur = 0;
        }
        sinfo->rx[chan].free++;
        sinfo->rx[chan].tokens--;
    }
}

static int
bkn_rx_restart(bkn_switch_info_t *sinfo, int chan)
{
    bkn_desc_info_t *desc;

    if (sinfo->basedev_suspended) {
        return 0;
    }

    if (sinfo->rx[chan].running) {
        return 0;
    }

    if (sinfo->rx[chan].free < MAX_RX_DCBS) {
        return 1;
    }

    desc = &sinfo->rx[chan].desc[sinfo->rx[chan].dirty];
    sinfo->rx[chan].chain_complete = 0;
    DBG_DCB_RX(("Restart Rx%d DMA, DCB @ 0x%08x (%d).\n",
                chan, (uint32_t)desc->dcb_dma, sinfo->rx[chan].dirty));
    dev_dma_chan_clear(sinfo, XGS_DMA_RX_CHAN + chan);
    dev_irq_mask_enable(sinfo, XGS_DMA_RX_CHAN + chan, 1);
    dev_dma_chan_start(sinfo, XGS_DMA_RX_CHAN + chan, desc->dcb_dma);
    sinfo->rx[chan].running = 1;

    /* Request one extra poll if chain was restarted during poll */
    if (sinfo->napi_poll_mode) {
        sinfo->napi_poll_again = 1;
    }

    return 0;
}

static int
bkn_tx_dma_start(bkn_switch_info_t *sinfo)
{
    bkn_desc_info_t *desc;

    desc = &sinfo->tx.desc[sinfo->tx.cur];
    if (sinfo->tx.free == MAX_TX_DCBS) {
        if (!sinfo->tx.api_active) {
            DBG_DCB_TX(("Start Tx DMA, DCB @ 0x%08x (%d).\n",
                        (uint32_t)desc->dcb_dma, sinfo->tx.cur));
            dev_dma_chan_clear(sinfo, XGS_DMA_TX_CHAN);
            dev_irq_mask_enable(sinfo, XGS_DMA_TX_CHAN, 1);
            dev_dma_chan_start(sinfo, XGS_DMA_TX_CHAN, desc->dcb_dma);
        }
    }

    return 0;
}

static int
bkn_dma_init(bkn_switch_info_t *sinfo)
{
    int chan;

    dev_dma_chan_init(sinfo, XGS_DMA_TX_CHAN, 1);
    if (CDMA_CH(sinfo, XGS_DMA_TX_CHAN)) {
        bkn_tx_dma_start(sinfo);
    }

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            continue;
        }
        dev_dma_chan_init(sinfo, XGS_DMA_RX_CHAN + chan, 0);
        bkn_rx_refill(sinfo, chan);
        bkn_rx_restart(sinfo, chan);
    }

    return 0;
}

static int
bkn_dma_abort_tx(bkn_switch_info_t *sinfo)
{
    bkn_dcb_chain_t *dcb_chain;

    DBG_VERB(("Aborting Tx DMA.\n"));

    dev_irq_mask_disable(sinfo, XGS_DMA_TX_CHAN, 1);

    dev_dma_chan_abort(sinfo, XGS_DMA_TX_CHAN, 10000);

    if (sinfo->tx.api_dcb_chain) {
        DBG_DCB_TX(("Freeing active Tx DCB chain.\n"));
        kfree(sinfo->tx.api_dcb_chain);
        sinfo->tx.api_dcb_chain = NULL;
    }
    while (!list_empty(&sinfo->tx.api_dcb_list)) {
        dcb_chain = list_entry(sinfo->tx.api_dcb_list.next,
                               bkn_dcb_chain_t, list);
        list_del(&dcb_chain->list);
        DBG_DCB_TX(("Freeing Tx DCB chain.\n"));
        kfree(dcb_chain);
    }
    sinfo->tx.api_dcb_chain_end = NULL;

    return 0;
}

static int
bkn_dma_abort_rx(bkn_switch_info_t *sinfo, int chan)
{
    bkn_dcb_chain_t *dcb_chain;

    DBG_VERB(("Aborting Rx%d DMA.\n", chan));

    dev_irq_mask_disable(sinfo, XGS_DMA_RX_CHAN + chan, 1);

    dev_dma_chan_abort(sinfo, XGS_DMA_RX_CHAN + chan, 10000);

    if (sinfo->rx[chan].api_dcb_chain) {
        DBG_DCB_RX(("Freeing active Rx%d DCB chain.\n", chan));
        kfree(sinfo->rx[chan].api_dcb_chain);
        sinfo->rx[chan].api_dcb_chain = NULL;
    }
    while (!list_empty(&sinfo->rx[chan].api_dcb_list)) {
        dcb_chain = list_entry(sinfo->rx[chan].api_dcb_list.next,
                               bkn_dcb_chain_t, list);
        list_del(&dcb_chain->list);
        DBG_DCB_RX(("Freeing Rx%d DCB chain.\n", chan));
        kfree(dcb_chain);
    }
    sinfo->rx[chan].api_dcb_chain_end = NULL;

    return 0;
}

static int
bkn_dma_abort(bkn_switch_info_t *sinfo)
{
    int chan;

    bkn_dma_abort_tx(sinfo);

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            continue;
        }
        bkn_dma_abort_rx(sinfo, chan);
    }

    return 0;
}

static inline int
device_is_dpp(bkn_switch_info_t *sinfo)
{
    int is_dpp = 0;

    if (sinfo)
    {
        is_dpp = (sinfo->dcb_type == 28) ? 1 : 0;
    }
    return is_dpp;
}

static inline int
device_is_dnx(bkn_switch_info_t *sinfo)
{
    int is_dnx = 0;

    if (sinfo)
    {
        is_dnx = (sinfo->dcb_type == 39) ? 1 : 0;
    }
    return is_dnx;
}

/* is DPP or is DNX*/
static inline int
device_is_sand(bkn_switch_info_t *sinfo)
{
    int is_dpp = 0;
    int is_dnx = 0;

    if (sinfo)
    {
        is_dpp = (sinfo->dcb_type == 28) ? 1 : 0;
        is_dnx = (sinfo->dcb_type == 39) ? 1 : 0;
    }
    return (is_dpp | is_dnx);
}

static bkn_filter_t *
bkn_match_rx_pkt(bkn_switch_info_t *sinfo, uint8_t *pkt, int pktlen,
                 void *meta, int chan, bkn_filter_t *cbf)
{
    struct list_head *list;
    bkn_filter_t *filter;
    kcom_filter_t scratch, *kf;
    uint8_t *oob = (uint8_t *)meta;
    int size, wsize;
    int idx, match;
    knet_filter_cb_f filter_cb;

    list_for_each(list, &sinfo->rxpf_list) {
        filter = (bkn_filter_t *)list;
        kf = &filter->kf;
        if (kf->pkt_data_offset + kf->pkt_data_size > pktlen) {
            continue;
        }
        memcpy(&scratch.data.b[0],
               &oob[kf->oob_data_offset], kf->oob_data_size);
        memcpy(&scratch.data.b[kf->oob_data_size],
               &pkt[kf->pkt_data_offset], kf->pkt_data_size);
        size = kf->oob_data_size + kf->pkt_data_size;
        wsize = BYTES2WORDS(size);
        DBG_VERB(("Filter: size = %d (%d), data = 0x%08x, mask = 0x%08x\n",
                  size, wsize, kf->data.w[0], kf->mask.w[0]));

        if (device_is_sand(sinfo)) {
            DBG_DUNE(("Filter: size = %d (wsize %d)\n", size, wsize));
            for (idx = 0; idx < wsize; idx++)
            {
                DBG_DUNE(("OOB[%d]: 0x%08x [0x%08x]\n", idx, kf->data.w[idx], kf->mask.w[idx]));
            }
            DBG_DUNE(("Meta Data [+ Selected Raw packet data]\n"));
            for (idx = 0; idx < wsize; idx++)
            {
                DBG_DUNE(("Scratch[%d]: 0x%08x\n", idx, scratch.data.w[idx]));
            }
        }

        match = 1;
        if (match) {
            if (device_is_dnx(sinfo))
            {
                /*
                 * Mutliple RX channels are enabled on JR2 and above devices
                 * Bind between priority 0 and RX channel 0 is not checked, then all enabled RX channels can receive packets.
                 */
                if (kf->priority && (kf->priority < (num_rx_prio * sinfo->rx_chans))) {
                    if (kf->priority < (num_rx_prio * chan) ||
                        kf->priority >= (num_rx_prio * (chan + 1))) {
                        match = 0;
                    }
                }
            }
            else {
                if (kf->priority < (num_rx_prio * sinfo->rx_chans)) {
                    if (kf->priority < (num_rx_prio * chan) ||
                        kf->priority >= (num_rx_prio * (chan + 1))) {
                        match = 0;
                    }
                }
            }
        }
        if (match) {
            for (idx = 0; idx < wsize; idx++) {
                scratch.data.w[idx] &= kf->mask.w[idx];
                if (scratch.data.w[idx] != kf->data.w[idx]) {
                    match = 0;
                    break;
                }
            }
        }
        if (match) {
            if (kf->dest_type == KCOM_DEST_T_CB) {
                /* Check for custom filters */
                filter_cb = filter->cb ? filter->cb : knet_filter_cb;
                if (filter_cb != NULL && cbf != NULL) {
                    memset(cbf, 0, sizeof(*cbf));
                    memcpy(&cbf->kf, kf, sizeof(cbf->kf));
                    if (filter_cb(pkt, pktlen, sinfo->dev_no,
                                  meta, chan, &cbf->kf)) {
                        filter->hits++;
                        return cbf;
                    }
                } else {
                    DBG_FLTR(("Match, but not filter callback\n"));
                }
            } else {
                filter->hits++;
                return filter;
            }
        }
    }

    return NULL;
}

static bool
bkn_netif_ok(struct net_device *dev)
{
    return (netif_carrier_ok(dev) && netif_running(dev));
}

static bkn_priv_t *
bkn_netif_lookup(bkn_switch_info_t *sinfo, int id)
{
    struct list_head *list;
    bkn_priv_t *priv;
    int found;

    /* Fast path */
    if (id < sinfo->ndev_max) {
        if (sinfo->ndevs[id] != NULL) {
            DBG_NDEV(("Look up netif ID %d successful\n", id));
            return netdev_priv(sinfo->ndevs[id]);
        }
    }

    /* Slow path - should normally not get here */
    found = 0;
    priv = NULL;
    list_for_each(list, &sinfo->ndev_list) {
        priv = (bkn_priv_t *)list;
        if (priv->id == id) {
            found = 1;
            break;
        }
    }
    if (found && priv != NULL) {
        return priv;
    }
    return NULL;
}

static int
bkn_hw_tstamp_rx_set(bkn_switch_info_t *sinfo, int phys_port, struct sk_buff *skb, uint32 *meta)
{
    int ret = -1;
    uint64_t ts = 0;
    struct skb_shared_hwtstamps *shhwtstamps = skb_hwtstamps(skb);


    if (knet_hw_tstamp_rx_time_upscale_cb) {
        if (knet_hw_tstamp_rx_time_upscale_cb(sinfo->dev_no, phys_port, skb, meta, &ts) >= 0) {
          ret = 0;
        }
    }

    memset(shhwtstamps, 0, sizeof(*shhwtstamps));
    shhwtstamps->hwtstamp = ns_to_ktime(ts);

    return ret;
}

static int
bkn_add_rcpu_encap(bkn_switch_info_t *sinfo, struct sk_buff *skb, void *meta, int len)
{
    int pktlen = skb->len;
    uint32_t *dmeta, *smeta, wsize, psize;
    int idx;

    /* Add and clear RCPU encapsulation */
    if (device_is_sand(sinfo)) {
        /*
         * len is the length of Dune system headers
         */
        psize = RCPU_HDR_SIZE + len;
        skb_push(skb, psize);
        memset(skb->data, 0, RCPU_HDR_SIZE);
    } else if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
        psize = RCPU_HDR_SIZE + sinfo->pkt_hdr_size;
        skb_push(skb, psize);
        memset(skb->data, 0, RCPU_HDR_SIZE);
    } else {
        psize = RCPU_RX_ENCAP_SIZE;
        skb_push(skb, psize);
        memset(skb->data, 0, RCPU_RX_ENCAP_SIZE);
    }

    /* RCPU Header */
    memcpy(skb->data, &skb->data[psize], 12);
    if (rcpu_dmac != NULL) {
        memcpy(skb->data, bkn_rcpu_dmac, 6);
    }
    if (rcpu_smac != NULL) {
        memcpy(&skb->data[6], bkn_rcpu_smac, 6);
    }
    skb->data[12] = 0x81;
    skb->data[14] = rcpu_vlan >> 8;
    skb->data[15] = rcpu_vlan & 0xff;
    skb->data[16] = rcpu_ethertype >> 8;
    skb->data[17] = rcpu_ethertype & 0xff;
    skb->data[18] = sinfo->rcpu_sig >> 8;
    skb->data[19] = sinfo->rcpu_sig & 0xff;
    skb->data[20] = RCPU_OPCODE_RX;
    skb->data[21] = RCPU_F_MODHDR;
    skb->data[24] = pktlen >> 8;
    skb->data[25] = pktlen & 0xff;
    if (device_is_sand(sinfo)) {
        /* RCPU.meta_len records length of Dune system headers */
        skb->data[28] = len & 0xff;
    }

    /* Meta data */
    dmeta = (uint32_t *)&skb->data[RCPU_HDR_SIZE];

    if (device_is_sand(sinfo)) {
        /* Copy at most 256 bytes system headers */
        memcpy(&skb->data[RCPU_HDR_SIZE], (uint8_t *)meta, len);
    } else {
        smeta = ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) ? (uint32_t *)meta : (uint32_t *)meta + 2;
        wsize = ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) ? sinfo->pkt_hdr_size / 4 : sinfo->dcb_wsize - 3;
        for (idx = 0; idx < wsize; idx++) {
            dmeta[idx] = htonl(smeta[idx]);
        }
    }


    return 0;
}

static void
bkn_eth_type_update(struct sk_buff *skb, int ethertype)
{
#if defined(PM_ETH_TYPE) && defined(PM_FC_TYPE)
    /* Optionally override standard protocol */
    skb->protocol = PM_ETH_TYPE;
    if (ethertype == ETH_P_FCOE) {
        skb->protocol = PM_FC_TYPE;
    }
#endif
}

#define BKN_DNX_BIT(x) (1<<(x))
#define BKN_DNX_RBIT(x) (~(1<<(x)))
#ifdef __LITTLE_ENDIAN
#define BKN_DNX_BYTE_SWAP(x) (x)
#else
#define BKN_DNX_BYTE_SWAP(x) ((((x) << 24)) | (((x) & 0xff00) << 8) | (((x) & 0xff0000) >> 8) | (((x) >> 24)))
#endif

static int
packet_is_untagged(uint16_t tpid)
{
    int is_untagged = 0;

    /* 0x8100 is used in 802.1Q */
    /* 0x8848 is used in 802.11ad, the dtag tpid can be set to anything besides 0x8848, 0x9100 is a typical value, but couldn't cover all scenarios. */
    is_untagged = ((tpid != 0x8100) && (tpid != 0x8848) && (tpid != 0x9100));
    return is_untagged;
}

static void
bkn_bitstream_set_field(uint32_t *input_buffer, uint32_t start_bit, uint32_t  nof_bits, uint32_t field)
{
    uint32_t place;
    uint32_t field_bit_i;
    uint32_t bit_indicator;

    if( nof_bits > 32)
    {
        return;
    }

    for( place=start_bit, field_bit_i = 0; field_bit_i< nof_bits; ++place, ++field_bit_i)
    {
        bit_indicator = field & BKN_DNX_BIT(nof_bits-field_bit_i-1);
        if(bit_indicator)
        {
            input_buffer[place>>5] |= (0x80000000 >> (place & 0x0000001F));
        }
        else
        {
            input_buffer[place>>5] &= ~(0x80000000 >> (place & 0x0000001F));
        }
    }
    return;
}

static void
bkn_bitstream_get_field(uint8_t  *input_buffer, uint32_t start_bit, uint32_t  nof_bits, uint32_t *output_value)
{
    uint32_t idx;
    uint32_t buf_sizes=0;
    uint32_t tmp_output_value[2]={0};
    uint32_t first_byte_ndx;
    uint32_t last_byte_ndx;
    uint32_t place;
    uint32_t field_bit_i;
    uint8_t *tmp_output_value_u8_ptr = (uint8_t*)&tmp_output_value;
    uint32_t bit_indicator;

    if (nof_bits > 32)
    {
         return;
    }

    first_byte_ndx = start_bit / 8;
    last_byte_ndx = ((start_bit + nof_bits - 1) / 8);
    *output_value=0;

    /* get 32 bit value, MSB */
    for (idx = first_byte_ndx; idx <= last_byte_ndx; ++idx)
    {
        tmp_output_value_u8_ptr[last_byte_ndx - idx] = input_buffer[idx];
        buf_sizes += 8;
    }
    tmp_output_value[0] = BKN_DNX_BYTE_SWAP(tmp_output_value[0]);
    if (last_byte_ndx > 4)
    {
       tmp_output_value[1] = BKN_DNX_BYTE_SWAP(tmp_output_value[1]);
    }

    place = buf_sizes - (start_bit % 8 + nof_bits);
    for (field_bit_i = 0; field_bit_i< nof_bits; ++place, ++field_bit_i)
    {
        uint32_t result;
        result = tmp_output_value[place>>5] & BKN_DNX_BIT(place & 0x0000001F);
        if (result)
        {
           bit_indicator = 1;
        } else {
           bit_indicator = 0;
        }
        *output_value |=  bit_indicator << field_bit_i;
    }
    return;
}

static void
bkn_dpp_packet_parse_ftmh(
    bkn_switch_info_t *sinfo,
    uint8_t *buf,
    uint32_t buf_len,
    bkn_dune_system_header_info_t *packet_info)
{
    uint32_t pkt_offset = packet_info->system_header_size;
    uint32_t dsp_ext_exist = 0;
    uint32_t fld_val;

    /* FTMH: Source-system-port-aggregate */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DPP_FTMH_SRC_SYS_PORT_MSB,
            BKN_DPP_FTMH_SRC_SYS_PORT_NOF_BITS,
            &packet_info->ftmh.source_sys_port_aggregate);
    /* FTMH: TM-action-type */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DPP_FTMH_ACTION_TYPE_MSB,
            BKN_DPP_FTMH_ACTION_TYPE_NOF_BITS,
            &packet_info->ftmh.action_type);
    /* FTMH: Internal-type */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DPP_FTMH_PPH_TYPE_MSB,
            BKN_DPP_FTMH_PPH_TYPE_NOF_BITS,
            &fld_val);

    /* FTMH: DSP Extension */
    bkn_bitstream_get_field(
        &buf[pkt_offset],
        BKN_DPP_FTMH_EXT_DSP_EXIST_MSB,
        BKN_DPP_FTMH_EXT_DSP_EXIST_NOF_BITS,
        &dsp_ext_exist);

    if (fld_val & 0x1)
    {
        packet_info->flags |= BKN_RX_HEADER_F_HAS_INTERNAL_HEADER;
    }
    if (fld_val & 0x2)
    {
        packet_info->flags |= BKN_RX_HEADER_F_HAS_OTSH;
    }

    pkt_offset += BKN_DPP_FTMH_SIZE_BYTE;
    DBG_DUNE(("FTMH(9-%u): Action-type %d Source-system-port 0x%x\n", pkt_offset,
                packet_info->ftmh.action_type,
                packet_info->ftmh.source_sys_port_aggregate));

    /* FTMH LB-Key Extension */
    if (sinfo->ftmh_lb_key_ext_size)
    {
        pkt_offset += sinfo->ftmh_lb_key_ext_size;
        DBG_DUNE(("FTMH LB-Key Extension(1-%u) is present\n", pkt_offset));
    }

    if (dsp_ext_exist)
    {
        pkt_offset += BKN_DPP_FTMH_DEST_EXT_SIZE_BYTE;
        DBG_DUNE(("FTMH DSP Extension(2-%u) is present\n", pkt_offset));
    }

    /* FTMH Stacking Extension */
    if (sinfo->ftmh_stacking_ext_size)
    {
        pkt_offset += sinfo->ftmh_stacking_ext_size;
        DBG_DUNE(("FTMH Stacking Extension(2-%u) is present\n", pkt_offset));
    }

    packet_info->system_header_size = pkt_offset;
    return;
}

static void
bkn_dpp_packet_parse_otsh(
    bkn_switch_info_t *sinfo,
    uint8_t *buf,
    uint32_t buf_len,
    bkn_dune_system_header_info_t *packet_info,
    uint8_t *is_skip_udh)
{
    uint32_t pkt_offset = packet_info->system_header_size;
    uint32_t type = 0;
    uint32_t oam_sub_type = 0;

    /* OTSH: TYPE */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DPP_OTSH_TYPE_MSB,
            BKN_DPP_OTSH_TYPE_NOF_BITS,
            &type);
    if (type == BKN_DPP_OTSH_TYPE_OAM) {
        /* OTSH: OAM_SUB_TYPE */
        bkn_bitstream_get_field(
                &buf[pkt_offset],
                BKN_DPP_OTSH_OAM_SUB_TYPE_MSB,
                BKN_DPP_OTSH_OAM_SUB_TYPE_NOF_BITS,
                &oam_sub_type);
        if ((oam_sub_type == BKN_DPP_OTSH_OAM_SUB_TYPE_DM_1588) || (oam_sub_type == BKN_DPP_OTSH_OAM_SUB_TYPE_DM_NTP)) {
            if (sinfo->oam_dm_tod_exist) {
                packet_info->flags |= BKN_RX_HEADER_F_HAS_OAM_DM_TOD_SECOND;
            }
            /* Down MEP DM trapped packets will not have UDH present (even if configured), except for QAX when custom_feature_oam_dm_tod_msb_add_enable=0 */
            if (!sinfo->no_skip_udh_check) {
                *is_skip_udh =  TRUE;
            }
        }
    }
    packet_info->system_header_size += BKN_DPP_OTSH_SIZE_BYTE;

    DBG_DUNE(("OTSH(%d): Type 0x%x OAM-Sub-Type 0x%x\n", BKN_DPP_OTSH_SIZE_BYTE, type, oam_sub_type));
    return;
}

static void
bkn_dpp_packet_parse_internal(
    bkn_switch_info_t *sinfo,
    uint8_t *buf,
    uint32_t buf_len,
    bkn_dune_system_header_info_t *packet_info,
    uint8_t is_oamp_punted,
    uint8_t *is_trapped)
{
    uint32_t pkt_offset = packet_info->system_header_size;
    uint32_t fld_val = 0;
    uint32_t eei_extension_present = 0;
    uint32_t learn_extension_present = 0;
    uint32_t fhei_size = 0;

    /* Internal: EEI EXT */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DPP_INTERNAL_EEI_EXTENSION_PRESENT_MSB,
            BKN_DPP_INTERNAL_EEI_EXTENSION_PRESENT_NOF_BITS,
            &eei_extension_present);
    /* Internal: LERAN EXT */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DPP_INTERNAL_LEARN_EXENSION_PRESENT_MSB,
            BKN_DPP_INTERNAL_LEARN_EXENSION_PRESENT_NOF_BITS,
            &learn_extension_present);
    /* Internal: FHEI EXT */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DPP_INTERNAL_FHEI_SIZE_MSB,
            BKN_DPP_INTERNAL_FHEI_SIZE_NOF_BITS,
            &fhei_size);
    /* Internal: FORWARD_CODE */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DPP_INTERNAL_FORWARD_CODE_MSB,
            BKN_DPP_INTERNAL_FORWARD_CODE_NOF_BITS,
            &fld_val);
    *is_trapped = (uint8_t)(fld_val == BKN_DPP_INTERNAL_FORWARD_CODE_CPU_TRAP);
    /* Internal: VSI */
    bkn_bitstream_get_field(
             &buf[pkt_offset],
             BKN_DPP_INTERNAL_VSI_MSB,
             BKN_DPP_INTERNAL_VSI_NOF_BITS,
             &fld_val);
    packet_info->internal.forward_domain = fld_val;

    if (is_oamp_punted && *is_trapped) {
        if(fhei_size == 3) {
            /* Force to FHEI size 1 when packets are punted by OAMP */
            fhei_size = 1;
        }
    }

    /* Move forward to end of Internal header */
    pkt_offset += BKN_DPP_INTERNAL_SIZE_BYTE;

    DBG_DUNE(("PPH(7-%u): EEI-Extension %d Learn-Extension %d VSI %d FHEI-size %d\n",
        pkt_offset, eei_extension_present,learn_extension_present, packet_info->internal.forward_domain, fhei_size));

    /* Advance header according to FHEI Trap extension */
    switch(fhei_size) {
        case 1:
            /* 3B FHEI Extension: do nothing, header poniter is in the right location */
            break;
        case 2:
            /* 5B FHEI Extension: adavance header pointer in 2B */
            pkt_offset += 2;
            break;
        case 3:
            /* 8B FHEI Extension: adavance header pointer in 5B */
            pkt_offset += 5;
            break;
        default:
            break;
    }

    /* Internal extension */
    if (*is_trapped && fhei_size)
    {
        /* CPU trap code qualifier */
        bkn_bitstream_get_field(
                &buf[pkt_offset],
                BKN_DPP_INTERNAL_FHEI_TRAP_SNOOP_3B_CPU_TRAP_CODE_QUALIFIER_MSB,
                BKN_DPP_INTERNAL_FHEI_TRAP_SNOOP_3B_CPU_TRAP_CODE_QUALIFIER_NOF_BITS,
                &packet_info->internal.trap_qualifier);
       /* CPU trap code */
        bkn_bitstream_get_field(
          &buf[pkt_offset],
          BKN_DPP_INTERNAL_FHEI_TRAP_SNOOP_3B_CPU_TRAP_CODE_MSB,
          BKN_DPP_INTERNAL_FHEI_TRAP_SNOOP_3B_CPU_TRAP_CODE_NOF_BITS,
          &packet_info->internal.trap_id);

        DBG_DUNE(("FHEI: trap_qualifier 0x%x trap_id 0x%x\n",
            packet_info->internal.trap_qualifier,
            packet_info->internal.trap_id));
    }

    /* Move forward to end of FHEI Trap extension */
    if (fhei_size) {
        pkt_offset += 3;
    }

    /* EEI extension */
    if (eei_extension_present) {
        pkt_offset += BKN_DPP_INTERNAL_EXPLICIT_EDITING_INFOMATION_EXTENSION_SIZE_BYTE;
    }
    /* Learn extension */
    if (learn_extension_present) {
        pkt_offset += BKN_DPP_INTERNAL_LEARN_EXTENSION_SIZE_BYTE;
    }

    packet_info->system_header_size = pkt_offset;
    return;
}

static int
bkn_dpp_packet_header_parse(
    bkn_switch_info_t *sinfo,
    uint8_t *buff,
    uint32_t buff_len,
    bkn_dune_system_header_info_t *packet_info)
{
    uint8_t  is_oamp_punted = FALSE;
    uint8_t  is_trapped = FALSE;
    uint8_t  is_skip_udh = FALSE;

    if ((sinfo == NULL) || (buff == NULL) || (packet_info == NULL)) {
        return -1;
    }

    packet_info->flags = BKN_RX_HEADER_F_HAS_ONE_SYSTEM_HEADER;
    /* FTMH */
    bkn_dpp_packet_parse_ftmh(sinfo, buff, buff_len, packet_info);

    /* Check if packet was punted to CPU by OAMP */
    if (device_is_dpp(sinfo))
    {
        if (sinfo->oamp_punt && (packet_info->ftmh.source_sys_port_aggregate == sinfo->oamp_punt)) {
            is_oamp_punted = TRUE;
        }
    }
    else if (device_is_dnx(sinfo))
    {
        uint8_t  idx = 0;
        for (idx = 0; idx < sinfo->oamp_port_number; idx++)
        {
            if (packet_info->ftmh.source_sys_port_aggregate == sinfo->oamp_ports[idx])
            {
                is_oamp_punted = TRUE;
                break;
            }
        }
    }


    /* OTSH */
    if (packet_info->flags & BKN_RX_HEADER_F_HAS_OTSH)
    {
        bkn_dpp_packet_parse_otsh(sinfo, buff, buff_len, packet_info, &is_skip_udh);
    }
    /* Internal header is forced to be present if packet was punted to CPU by OAMP */
    if (is_oamp_punted)
    {
        packet_info->flags |= BKN_RX_HEADER_F_HAS_INTERNAL_HEADER;
    }
    /* Internal */
    if (packet_info->flags & BKN_RX_HEADER_F_HAS_INTERNAL_HEADER)
    {
        bkn_dpp_packet_parse_internal(sinfo, buff, buff_len, packet_info, is_oamp_punted, &is_trapped);
    }
    /* Skip UDH for the outer layer when packet is punted by OAM for JR2 in JR1 mode */
    if (device_is_dnx(sinfo) && is_oamp_punted) {
        is_skip_udh = TRUE;
    }
    /* UDH */
    if (sinfo->udh_size && !is_skip_udh) {
        packet_info->system_header_size += sinfo->udh_size;
    }
    /* OAM DM TOD header */
    if (packet_info->flags & BKN_RX_HEADER_F_HAS_OAM_DM_TOD_SECOND) {
        packet_info->system_header_size += BKN_DPP_OAM_DM_TOD_SIZE_BYTE;
    }

    /* Additional layer of system headers */
    if (is_oamp_punted && is_trapped)
    {
        is_oamp_punted = FALSE;
        is_trapped = FALSE;
        is_skip_udh = FALSE;

        packet_info->flags = BKN_RX_HEADER_F_HAS_TWO_SYSTEM_HEADER;

        /* FTMH */
        bkn_dpp_packet_parse_ftmh(sinfo, buff, buff_len, packet_info);
        /* OTSH */
        if (packet_info->flags & BKN_RX_HEADER_F_HAS_OTSH)
        {
            bkn_dpp_packet_parse_otsh(sinfo, buff, buff_len, packet_info, &is_skip_udh);
        }
        /* Internal */
        if (packet_info->flags & BKN_RX_HEADER_F_HAS_INTERNAL_HEADER)
        {
            bkn_dpp_packet_parse_internal(sinfo, buff, buff_len, packet_info, is_oamp_punted, &is_trapped);
        }
        /* OAMP Punted packets do not have UDH in the inner header for both JR1 and JR2 in JR1 mode */
        /* OAM DM TOD header */
        if (packet_info->flags & BKN_RX_HEADER_F_HAS_OAM_DM_TOD_SECOND) {
            packet_info->system_header_size += BKN_DPP_OAM_DM_TOD_SIZE_BYTE;
        }
    }

    DBG_DUNE(("Total length of headers is %u\n", packet_info->system_header_size));

    return 0;
}


static int
bkn_dnx_packet_parse_ftmh(
    bkn_switch_info_t *sinfo,
    uint8_t *buf,
    uint32_t buf_len,
    bkn_dune_system_header_info_t *packet_info)
{
    uint32_t fld_val;
    uint32_t pkt_offset = packet_info->system_header_size;
    uint8_t  tm_dst_ext_present = 0;
    uint8_t  app_specific_ext_size = 0;
    uint8_t  flow_id_ext_size = 0;
    uint8_t  bier_bfr_ext_size = 0;

    if ((sinfo == NULL) || (buf == NULL) || (packet_info == NULL)) {
        return -1;
    }

    /* FTMH: Source-System-Port-Aggregate */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_SRC_SYS_PORT_AGGREGATE_MSB,
            BKN_DNX_FTMH_SRC_SYS_PORT_AGGREGATE_NOF_BITS,
            &fld_val);
    packet_info->ftmh.source_sys_port_aggregate = fld_val;
    /* FTMH: Action-Type */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_ACTION_TYPE_MSB,
            BKN_DNX_FTMH_ACTION_TYPE_NOF_BITS,
            &fld_val);
    packet_info->ftmh.action_type = fld_val;
    /* FTMH: PPH-Type TSH */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_PPH_TYPE_IS_TSH_EN_MSB,
            BKN_DNX_FTMH_PPH_TYPE_IS_TSH_EN_NOF_BITS,
            &fld_val);
    packet_info->flags |= fld_val ? BKN_RX_HEADER_F_HAS_TSH : 0;
    /* FTMH: PPH-Type PPH base */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_PPH_TYPE_IS_PPH_EN_MSB,
            BKN_DNX_FTMH_PPH_TYPE_IS_PPH_EN_NOF_BITS,
            &fld_val);
    packet_info->flags |= fld_val ? BKN_RX_HEADER_F_HAS_INTERNAL_HEADER : 0;
    /* FTMH: TM-Destination-Extension-Present */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_TM_DST_EXT_PRESENT_MSB,
            BKN_DNX_FTMH_TM_DST_EXT_PRESENT_NOF_BITS,
            &fld_val);
    tm_dst_ext_present = fld_val;
    /* FTMH: Application-Specific-Extension-Size */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE_MSB,
            BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE_NOF_BITS,
            &fld_val);
    app_specific_ext_size = fld_val;
    /* FTMH: Flow-ID-Extension-Size */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_FLOW_ID_EXT_SIZE_MSB,
            BKN_DNX_FTMH_FLOW_ID_EXT_SIZE_NOF_BITS,
            &fld_val);
    flow_id_ext_size = fld_val;
    /* FTMH: BIER-BFR-Extension-Size */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_FTMH_BIER_BFR_EXT_SIZE_MSB,
            BKN_DNX_FTMH_BIER_BFR_EXT_SIZE_NOF_BITS,
            &fld_val);
    bier_bfr_ext_size = fld_val;

    pkt_offset += BKN_DNX_FTMH_BASE_SIZE;

    DBG_DUNE(("FTMH(10-%u): source-system-port 0x%x action_type %u\n",
              pkt_offset, packet_info->ftmh.source_sys_port_aggregate,
              packet_info->ftmh.action_type));

    /* FTMH LB-Key Extension */
    if (sinfo->ftmh_lb_key_ext_size > 0)
    {
        pkt_offset += sinfo->ftmh_lb_key_ext_size;
        DBG_DUNE(("FTMH LB-Key Extension(%u-%u) is present\n", sinfo->ftmh_lb_key_ext_size, pkt_offset));
    }
    /* FTMH Stacking Extension */
    if (sinfo->ftmh_stacking_ext_size > 0)
    {
        pkt_offset += sinfo->ftmh_stacking_ext_size;
        DBG_DUNE(("FTMH Stacking Extension(%u-%u) is present\n", sinfo->ftmh_stacking_ext_size, pkt_offset));
    }
    /* FTMH BIER BFR Extension */
    if (bier_bfr_ext_size > 0)
    {
        pkt_offset += BKN_DNX_FTMH_BIER_BFR_EXT_SIZE;
        DBG_DUNE(("FTMH BIER BFR Extension(2-%u) is present\n", pkt_offset));
    }
    /* FTMH TM Destination Extension */
    if (tm_dst_ext_present > 0)
    {
        pkt_offset += BKN_DNX_FTMH_TM_DST_EXT_SIZE;
        DBG_DUNE(("FTMH TM Destination Extension(3-%u) is present\n", pkt_offset));
    }
    /* FTMH Application Specific Extension */
    if (app_specific_ext_size > 0)
    {
        bkn_bitstream_get_field(
                &buf[pkt_offset],
                BKN_DNX_FTMH_ASE_TYPE_MSB,
                BKN_DNX_FTMH_ASE_TYPE_NOF_BITS,
                &fld_val);
        if (fld_val == BKN_DNX_FTMH_ASE_TYPE_OAM) {
            /* ASE: OAM_SUB_TYPE */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_MSB,
                    BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_NOF_BITS,
                    &fld_val);
            if ((fld_val == BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_DM_1588) ||
                (fld_val == BKN_DNX_FTMH_ASE_OAM_SUB_TYPE_DM_NTP)) {
                packet_info->flags |= BKN_RX_HEADER_F_HAS_OAM_DM_TOD_SECOND;
            }
        }
        pkt_offset += BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE;
        DBG_DUNE(("FTMH Application Specific Extension(6-%u) is present\n", pkt_offset));
    }
    /* FTMH Flow-ID Extension */
    if (flow_id_ext_size > 0)
    {
        pkt_offset += BKN_DNX_FTMH_FLOW_ID_EXT_SIZE;
        DBG_DUNE(("FTMH Flow-ID Extension(3-%u) is present\n", pkt_offset));
    }
    DBG_DUNE(("FTMH flags = 0x%08x\n", packet_info->flags));

    packet_info->system_header_size = pkt_offset;

    return 0;
}


static int
bkn_dnx_packet_parse_internal(
    bkn_switch_info_t *sinfo,
    uint8_t *buf,
    uint32_t buf_len,
    bkn_dune_system_header_info_t *packet_info,
    uint8_t is_oamp_punted,
    uint8_t *is_trapped)
{
    uint32_t fld_val;
    uint32_t pkt_offset = packet_info->system_header_size;
    uint8_t  learn_ext_present;
    uint8_t  fhei_size;
    uint8_t  lif_ext_type;
    uint8_t  udh_en = sinfo->udh_enable;

    if ((sinfo == NULL) || (buf == NULL) || (packet_info == NULL)) {
        return -1;
    }

    /* Internal: Forward-Domain */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_INTERNAL_12_FORWARD_DOMAIN_MSB,
            BKN_DNX_INTERNAL_12_FORWARD_DOMAIN_NOF_BITS,
            &fld_val);
    packet_info->internal.forward_domain = fld_val;
    /* Internal: Learn-Extension-Present */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_INTERNAL_12_LEARN_EXT_PRESENT_MSB,
            BKN_DNX_INTERNAL_12_LEARN_EXT_PRESENT_NOF_BITS,
            &fld_val);
    learn_ext_present = fld_val;
    /* Internal: FHEI-Size */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_INTERNAL_12_FHEI_SIZE_MSB,
            BKN_DNX_INTERNAL_12_FHEI_SIZE_NOF_BITS,
            &fld_val);
    fhei_size = fld_val;
    /* Internal: LIF-Extension-Type */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_INTERNAL_12_LIF_EXT_TYPE_MSB,
            BKN_DNX_INTERNAL_12_LIF_EXT_TYPE_NOF_BITS,
            &fld_val);
    lif_ext_type = fld_val;

    /* Internal: Parsing-Start-Offset */
    bkn_bitstream_get_field(
            &buf[pkt_offset],
            BKN_DNX_INTERNAL_12_PARSING_START_OFFSET_MSB,
            BKN_DNX_INTERNAL_12_PARSING_START_OFFSET_NOF_BITS,
            &fld_val);
    packet_info->internal.parsing_start_offset = fld_val;

    pkt_offset += BKN_DNX_INTERNAL_BASE_TYPE_12;
    DBG_DUNE(("Internal(12-%u): FWD_DOMAIN 0x%x(%d,%d), LEARN_EXT %d, FHEI_SIZE %d, LIF_EXT %d \n",
                pkt_offset, packet_info->internal.forward_domain,
                BKN_DNX_PPH_FWD_DOMAIN_TYPE_GET(packet_info->internal.forward_domain),
                BKN_DNX_PPH_FWD_DOMAIN_ID_GET(packet_info->internal.forward_domain),
                learn_ext_present, fhei_size, lif_ext_type));

    if (fhei_size)
    {
        switch (fhei_size)
        {
            case BKN_DNX_INTERNAL_FHEI_TYPE_SZ0:
                pkt_offset += BKN_DNX_INTERNAL_FHEI_SZ0_SIZE;
                DBG_DUNE(("FHEI(3-%u) is present\n", pkt_offset));
                break;
            case BKN_DNX_INTERNAL_FHEI_TYPE_SZ1:
                /* FHEI: Type */
                bkn_bitstream_get_field(
                        &buf[pkt_offset],
                        BKN_DNX_INTERNAL_FHEI_TRAP_5B_TYPE_MSB,
                        BKN_DNX_INTERNAL_FHEI_TRAP_5B_TYPE_NOF_BITS,
                        &fld_val);
                /* FHEI-Size == 5B, FHEI-Type == Trap/Sniff */
                if (fld_val == 0x5)
                {
                    *is_trapped = TRUE;
                    /* FHEI: Qualifier */
                    bkn_bitstream_get_field(
                            &buf[pkt_offset],
                            BKN_DNX_INTERNAL_FHEI_TRAP_5B_QUALIFIER_MSB,
                            BKN_DNX_INTERNAL_FHEI_TRAP_5B_QUALIFIER_NOF_BITS,
                            &fld_val);
                    packet_info->internal.trap_qualifier = fld_val;
                    /* FHEI: Code */
                    bkn_bitstream_get_field(
                            &buf[pkt_offset],
                            BKN_DNX_INTERNAL_FHEI_TRAP_5B_CODE_MSB,
                            BKN_DNX_INTERNAL_FHEI_TRAP_5B_CODE_NOF_BITS,
                            &fld_val);
                    packet_info->internal.trap_id= fld_val;
                }
                pkt_offset += BKN_DNX_INTERNAL_FHEI_SZ1_SIZE;
                DBG_DUNE(("FHEI(5-%u): code 0x%x qualifier 0x%x\n", pkt_offset, packet_info->internal.trap_id, packet_info->internal.trap_qualifier));
                break;
            case BKN_DNX_INTERNAL_FHEI_TYPE_SZ2:
                pkt_offset += BKN_DNX_INTERNAL_FHEI_SZ1_SIZE;
                DBG_DUNE(("FHEI(8-%u) is present\n", pkt_offset));
                break;
        }
    }

    /* PPH LIF Extension */
    if (lif_ext_type)
    {
        pkt_offset += sinfo->pph_lif_ext_size[lif_ext_type];
        DBG_DUNE(("PPH LIF Extension(%d-%u) is present\n", sinfo->pph_lif_ext_size[lif_ext_type], pkt_offset));
    }

    /* PPH Learn Extension */
    if (learn_ext_present)
    {
        pkt_offset += BKN_DNX_INTERNAL_LEARN_EXT_SIZE;
        DBG_DUNE(("PPH Learn Extension(19-%u) is present\n", pkt_offset));
    }

    /** Skip UDH If packet is punted to CPU by OAMP */
    if (is_oamp_punted) {
        udh_en = FALSE;
    }

    /* OAM DMM/DMR TOD second header: PPH+TOD+UDH */
    if (sinfo->cmic_type == 'x') {
        if (packet_info->flags & BKN_RX_HEADER_F_HAS_OAM_DM_TOD_SECOND) {
            pkt_offset += BKN_DNX_TOD_SECOND_SIZE;
            DBG_DUNE(("TOD second Header(4-%u) is present\n", pkt_offset));
        }
    }

    /* UDH Header */
    if (udh_en)
    {
        uint8_t data_type_0;
        uint8_t data_type_1;
        uint8_t data_type_2;
        uint8_t data_type_3;

        if (sinfo->cmic_type == 'r')
        {
            /* UDH: UDH-Data-Type[3] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_0_MSB,
                    BKN_DNX_UDH_DATA_TYPE_0_NOF_BITS,
                    &fld_val);
            data_type_3 = fld_val;
            /* UDH: UDH-Data-Type[2] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_1_MSB,
                    BKN_DNX_UDH_DATA_TYPE_1_NOF_BITS,
                    &fld_val);
            data_type_2 = fld_val;
            /* UDH: UDH-Data-Type[1] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_2_MSB,
                    BKN_DNX_UDH_DATA_TYPE_2_NOF_BITS,
                    &fld_val);
            data_type_1 = fld_val;
            /* UDH: UDH-Data-Type[0] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_3_MSB,
                    BKN_DNX_UDH_DATA_TYPE_3_NOF_BITS,
                    &fld_val);
            data_type_0 = fld_val;
            pkt_offset += BKN_DNX_UDH_BASE_SIZE;

            if (data_type_0)
            {
                pkt_offset += sinfo->udh_length_type[0];
            }
            if (data_type_1)
            {
                pkt_offset += sinfo->udh_length_type[1];
            }
            if (data_type_2)
            {
                pkt_offset += sinfo->udh_length_type[2];
            }
            if (data_type_3)
            {
                pkt_offset += sinfo->udh_length_type[3];
            }
        }
        else
        {
            /* UDH: UDH-Data-Type[0] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_0_MSB,
                    BKN_DNX_UDH_DATA_TYPE_0_NOF_BITS,
                    &fld_val);
            data_type_0 = fld_val;
            /* UDH: UDH-Data-Type[1] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_1_MSB,
                    BKN_DNX_UDH_DATA_TYPE_1_NOF_BITS,
                    &fld_val);
            data_type_1 = fld_val;
            /* UDH: UDH-Data-Type[2] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_2_MSB,
                    BKN_DNX_UDH_DATA_TYPE_2_NOF_BITS,
                    &fld_val);
            data_type_2 = fld_val;
            /* UDH: UDH-Data-Type[3] */
            bkn_bitstream_get_field(
                    &buf[pkt_offset],
                    BKN_DNX_UDH_DATA_TYPE_3_MSB,
                    BKN_DNX_UDH_DATA_TYPE_3_NOF_BITS,
                    &fld_val);
            data_type_3 = fld_val;
            pkt_offset += BKN_DNX_UDH_BASE_SIZE;

            pkt_offset += sinfo->udh_length_type[data_type_0];
            pkt_offset += sinfo->udh_length_type[data_type_1];
            pkt_offset += sinfo->udh_length_type[data_type_2];
            pkt_offset += sinfo->udh_length_type[data_type_3];
        }
        DBG_DUNE(("UDH base(1-%u) is present\n", pkt_offset));
    }

    /* OAM DMM/DMR TOD second header: PPH+UDH+TOD */
    if (sinfo->cmic_type == 'r') {
        if (packet_info->flags & BKN_RX_HEADER_F_HAS_OAM_DM_TOD_SECOND) {
            pkt_offset += BKN_DNX_TOD_SECOND_SIZE;
            DBG_DUNE(("TOD second Header(4-%u) is present\n", pkt_offset));
        }
    }

    packet_info->system_header_size = pkt_offset;

    return 0;
}

static int
bkn_dnx_packet_header_parse(
    bkn_switch_info_t *sinfo,
    uint8_t *buff,
    uint32_t buff_len,
    bkn_dune_system_header_info_t *packet_info)
{
    uint8_t  is_oamp_punted = FALSE;
    uint8_t  is_trapped = FALSE;
    uint8_t  idx = 0;

    if ((sinfo == NULL) || (buff == NULL) || (packet_info == NULL)) {
        return -1;
    }

    packet_info->flags = BKN_RX_HEADER_F_HAS_ONE_SYSTEM_HEADER;
    /* FTMH */
    bkn_dnx_packet_parse_ftmh(sinfo, buff, buff_len, packet_info);

    /* Time-Stamp */
    if (packet_info->flags & BKN_RX_HEADER_F_HAS_TSH)
    {
        packet_info->system_header_size += BKN_DNX_TSH_SIZE;
        DBG_DUNE(("Time-Stamp Header(4-%u) is present\n", packet_info->system_header_size));
    }

    /* Check if packet was punted to CPU by OAMP */
    for (idx = 0; idx < sinfo->oamp_port_number; idx++)
    {
        if (packet_info->ftmh.source_sys_port_aggregate == sinfo->oamp_ports[idx])
        {
            is_oamp_punted = TRUE;
            break;
        }
    }

    /* Internal */
    if (packet_info->flags & BKN_RX_HEADER_F_HAS_INTERNAL_HEADER)
    {
        bkn_dnx_packet_parse_internal(sinfo, buff, buff_len, packet_info, is_oamp_punted, &is_trapped);
    }

    if (is_oamp_punted)
    {
        uint32_t oibih_oam_pdu_offset = 0;
        is_oamp_punted = FALSE;
        is_trapped = FALSE;

        packet_info->flags = BKN_RX_HEADER_F_HAS_TWO_SYSTEM_HEADER;
        if (sinfo->cmic_type == 'r')
        {
            /* OIBIH: OAM_PDU_Offset */
            bkn_bitstream_get_field(
                    &buff[packet_info->system_header_size],
                    BKN_DNX_OIBIH_OAM_PDU_OFFSET_MSB,
                    BKN_DNX_OIBIH_OAM_PDU_OFFSET_NOF_BITS,
                    &oibih_oam_pdu_offset);
            packet_info->system_header_size += BKN_DNX_OIBIH_SIZE;
            DBG_DUNE(("OIBIH Header(14-%u) is present\n", packet_info->system_header_size));
        }
        /* FTMH */
        bkn_dnx_packet_parse_ftmh(sinfo, buff, buff_len, packet_info);
        /* Time-Stamp */
        if (packet_info->flags & BKN_RX_HEADER_F_HAS_TSH)
        {
            packet_info->system_header_size += BKN_DNX_TSH_SIZE;
            DBG_DUNE(("Time-Stamp Header(4-%u) is present\n", packet_info->system_header_size));
        }
        /* Internal */
        if (packet_info->flags & BKN_RX_HEADER_F_HAS_INTERNAL_HEADER)
        {
            bkn_dnx_packet_parse_internal(sinfo, buff, buff_len, packet_info, is_oamp_punted, &is_trapped);
        }
        if (oibih_oam_pdu_offset)
        {
            /*
             * parsing_start_offset indicates the bytes before OAM PDU including PTCH, etc. For example, it's the length of PTCH + ETH1
             * oibih_oam_pdu_offset indicates the bytes from the end of system headers to OAM PDU. For example, it's the length of ETH1
             */
            if (packet_info->internal.parsing_start_offset > oibih_oam_pdu_offset)
            {
                packet_info->system_header_size += (packet_info->internal.parsing_start_offset - oibih_oam_pdu_offset);
                DBG_DUNE(("Offset after system headers %u\n", (packet_info->internal.parsing_start_offset - oibih_oam_pdu_offset)));
            }
        }
    }
    else
    {
        if (packet_info->internal.parsing_start_offset && (sinfo->cmic_type == 'r'))
        {
            packet_info->system_header_size += packet_info->internal.parsing_start_offset;
            DBG_DUNE(("Offset after system headers %u\n", packet_info->internal.parsing_start_offset));
        }
    }

    DBG_DUNE(("Total length of headers is %u\n", packet_info->system_header_size));

    return 0;
}

static int
bkn_packet_header_parse(bkn_switch_info_t *sinfo, uint8_t *buf, uint32_t buf_len, bkn_dune_system_header_info_t *packet_info)
{
    if (device_is_dpp(sinfo))
    {
        bkn_dpp_packet_header_parse(sinfo, buf, buf_len, packet_info);
    }
    else if (device_is_dnx(sinfo))
    {
        if (sinfo->system_headers_mode == BKN_DNX_JR2_MODE)
        {
            /* Jericho 2 mode */
            bkn_dnx_packet_header_parse(sinfo, buf, buf_len, packet_info);
        }
        else
        {
            /* Jericho/QMX/QAX mode */
            bkn_dpp_packet_header_parse(sinfo, buf, buf_len, packet_info);
        }
    }
    return 0;
}

static int
bkn_do_api_rx(bkn_switch_info_t *sinfo, int chan, int budget)
{
    bkn_priv_t *priv;
    bkn_dcb_chain_t *dcb_chain;
    struct sk_buff *skb;
    bkn_filter_t cbf;
    bkn_filter_t *filter = NULL;
    uint32_t err_woff;
    uint32_t *dcb, *meta, *match_data;
    uint8_t *pkt;
    uint64_t pkt_dma;
    int drop_api;
    int ethertype;
    int pktlen, pkt_hdr_size = 0;
    int idx;
    int dcbs_done = 0;
    bkn_dune_system_header_info_t packet_info;
    uint32_t sand_scratch_data[BKN_SAND_SCRATCH_DATA_SIZE] = {0};

    dcb_chain = sinfo->rx[chan].api_dcb_chain;
    if (dcb_chain == NULL) {
        /* No active chains */
        return 0;
    }

    while (dcb_chain->dcb_cur < dcb_chain->dcb_cnt) {
        dcb = &dcb_chain->dcb_mem[dcb_chain->dcb_cur * sinfo->dcb_wsize];
        DBG_VERB(("DCB %2d: 0x%08x\n",
                  dcb_chain->dcb_cur, dcb[sinfo->dcb_wsize-1]));
        if ((dcb[sinfo->dcb_wsize-1] & (1 << 31)) == 0) {
            break;
        }
        if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            /* Handle for Continuous DMA mode */
            if (dcbs_done >= budget) {
                break;
            }
            if (((sinfo->cmic_type == 'x' || sinfo->cmic_type == 'r') && dcb[2] & (1 << 18)) ||
                ((sinfo->cmic_type != 'x' && sinfo->cmic_type != 'r') && dcb[1] & (1 << 18))) {
                dcb[sinfo->dcb_wsize-1] |= SOC_DCB_KNET_DONE;
                bkn_api_rx_chain_done(sinfo, chan);
                dcb_chain = sinfo->rx[chan].api_dcb_chain;
                if (dcb_chain == NULL) {
                    break;
                }
                continue;
            }
        }
        if (((sinfo->cmic_type == 'x' || sinfo->cmic_type == 'r') && (dcb[2] & (1 << 16)) == 0) ||
            ((sinfo->cmic_type != 'x' && sinfo->cmic_type != 'r') && (dcb[1] & (1 << 16)) == 0)) {
            sinfo->rx[chan].chain_complete = 1;
        }
        sinfo->rx[chan].pkts++;
        if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
            pkt_dma = BUS_TO_DMA_HI(dcb[1]);
            pkt_dma = pkt_dma << 32 | dcb[0];
        } else {
            pkt_dma = dcb[0];
        }
        pkt = (uint8_t *)kernel_bde->p2l(sinfo->dev_no, (sal_paddr_t)pkt_dma);
        pktlen = dcb[sinfo->dcb_wsize-1] & SOC_DCB_KNET_COUNT_MASK;
        bkn_dump_pkt(pkt, pktlen, XGS_DMA_RX_CHAN);

        if (device_is_sand(sinfo)) {
            err_woff = BKN_SAND_SCRATCH_DATA_SIZE - 1;
            sand_scratch_data[err_woff] = dcb[sinfo->dcb_wsize-1];
            meta = (uint32_t *)pkt;
            memset(&packet_info, 0, sizeof(bkn_dune_system_header_info_t));
            if (ENET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
                pkt_hdr_size = 0;
            } else {
                /* Decode system headers and fill sratch data */
                bkn_packet_header_parse(sinfo, pkt, (uint32_t)pktlen, &packet_info);
                pkt_hdr_size = packet_info.system_header_size;
            }
        } else {
            if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                meta = (uint32_t *)pkt;
                err_woff = sinfo->pkt_hdr_size / sizeof(uint32_t) - 1;
                meta[err_woff] = dcb[sinfo->dcb_wsize-1];
            } else {
                meta = dcb;
                err_woff = sinfo->dcb_wsize - 1;
            }
            pkt_hdr_size = sinfo->pkt_hdr_size;
        }

        /* Minimun size: header_size + MACs + VLAN + ETH_TYPE */
        if (pktlen > pkt_hdr_size + 18) {
            if (device_is_sand(sinfo)) {
                bkn_bitstream_set_field(sand_scratch_data, 0,  16,
                                        packet_info.internal.trap_id);
                bkn_bitstream_set_field(sand_scratch_data, 16, 16,
                                        packet_info.internal.trap_qualifier);
                bkn_bitstream_set_field(sand_scratch_data, 32, 17,
                                        packet_info.ftmh.source_sys_port_aggregate);
                bkn_bitstream_set_field(sand_scratch_data, 64, 2,
                                        packet_info.ftmh.action_type);
                bkn_bitstream_set_field(sand_scratch_data, 66, 18,
                                        packet_info.internal.forward_domain);

                if (force_tagged) {
                    uint8_t *eth_hdr = pkt + pkt_hdr_size;
                    uint16_t tpid = 0;

                    tpid = PKT_U16_GET(eth_hdr, 12);
                    if (packet_is_untagged(tpid)) {
                        int raw_packet_len = pktlen - pkt_hdr_size;
                        uint32_t vid = 0;
                        uint32_t is_vsi = BKN_DNX_PPH_FWD_DOMAIN_IS_VSI(packet_info.internal.forward_domain);
                        uint32_t fwd_domain = BKN_DNX_PPH_FWD_DOMAIN_ID_GET(packet_info.internal.forward_domain);

                        if ((pktlen + 4) < rx_buffer_size) {
                            for (idx = (raw_packet_len - 1); idx >= 12; idx--) {
                                eth_hdr[idx+4] = eth_hdr[idx];
                            }
                            if (ft_vid) {
                                vid = ft_vid;
                            }
                            else if (is_vsi && fwd_domain) {
                                vid = fwd_domain & 0xfff;
                            }
                            else {
                                vid = 1;
                            }
                            DBG_DUNE(("add vlan tag (%d) to untagged packets\n",
                                      vid));

                            eth_hdr[12] = (ft_tpid >> 8) & 0xff;
                            eth_hdr[13] = ft_tpid & 0xff;
                            eth_hdr[14] = (((ft_pri & 0x7) << 5) |
                                           ((ft_cfi & 0x1) << 4) |
                                           ((vid >> 8) & 0xf)) & 0xff;
                            eth_hdr[15] = vid & 0xff;
                            /* Reset packet length in DCB */
                            pktlen += 4;
                            bkn_dump_pkt(pkt, pktlen, XGS_DMA_RX_CHAN);
                            dcb[sinfo->dcb_wsize-1] &= ~SOC_DCB_KNET_COUNT_MASK;
                            dcb[sinfo->dcb_wsize-1] |= pktlen &
                                                       SOC_DCB_KNET_COUNT_MASK;
                        }
                    }
                }
            }

            if (device_is_sand(sinfo)) {
                match_data = sand_scratch_data;
            } else {
                match_data = meta;
            }
            filter = bkn_match_rx_pkt(sinfo, pkt + pkt_hdr_size,
                                      pktlen - pkt_hdr_size, match_data,
                                      chan, &cbf);

            if ((dcb[sinfo->dcb_wsize-1] & 0xf0000) != 0x30000) {
                /* Fragment or error */
                if (filter && filter->kf.mask.w[err_woff] == 0) {
                    /* Drop unless DCB status is part of filter */
                    filter = NULL;
                }
            }
        }
        drop_api = 1;
        if (filter) {
            DBG_FLTR(("Match filter ID %d\n", filter->kf.id));
            switch (filter->kf.dest_type) {
            case KCOM_DEST_T_API:
                DBG_FLTR(("Send to Rx API\n"));
                sinfo->rx[chan].pkts_f_api++;
                drop_api = 0;
                break;
            case KCOM_DEST_T_NETIF:
                priv = bkn_netif_lookup(sinfo, filter->kf.dest_id);
                if (priv) {
                    /* Check that software link is up */
                    if (!bkn_netif_ok(priv->dev)) {
                        sinfo->rx[chan].pkts_d_no_link++;
                        break;
                    }

                    pkt += pkt_hdr_size;
                    pktlen -= pkt_hdr_size;

                    /* Add 2 bytes for IP header alignment (see below) */
                    if (device_is_sand(sinfo)) {
                        skb = dev_alloc_skb(pktlen + RCPU_HDR_SIZE + pkt_hdr_size + 2);
                        if (skb == NULL) {
                            sinfo->rx[chan].pkts_d_no_skb++;
                            break;
                        }
                        skb_reserve(skb, RCPU_HDR_SIZE + pkt_hdr_size);
                    } else {
                        skb = dev_alloc_skb(pktlen + RCPU_RX_ENCAP_SIZE + 2);
                        if (skb == NULL) {
                            sinfo->rx[chan].pkts_d_no_skb++;
                            break;
                        }
                        skb_reserve(skb, RCPU_RX_ENCAP_SIZE);
                    }

                    DBG_FLTR(("Send to netif %d (%s)\n",
                              priv->id, priv->dev->name));
                    sinfo->rx[chan].pkts_f_netif++;
                    skb->dev = priv->dev;
                    skb_reserve(skb, 2);    /* 16 byte align the IP fields. */

                    /* Save for RCPU before stripping tag */
                    ethertype = PKT_U16_GET(pkt, 16);

                    skb_copy_to_linear_data(skb, pkt, pktlen);
                    if (device_is_sand(sinfo)) {
                        /* CRC has been stripped */
                        skb_put(skb, pktlen);
                    } else {
                        skb_put(skb, pktlen - 4); /* Strip CRC */
                    }

                    if ((priv->flags & KCOM_NETIF_F_KEEP_RX_TAG) == 0) {
                        uint16_t vlan_proto = PKT_U16_GET(pkt, 12);

                        if (filter->kf.flags & KCOM_FILTER_F_STRIP_TAG) {
                            /* Strip VLAN tag */
                            if (vlan_proto == ETH_P_8021Q ||
                                vlan_proto == ETH_P_8021AD) {
                                DBG_FLTR(("Strip VLAN tag\n"));
                                ((u32*)skb->data)[3] = ((u32*)skb->data)[2];
                                ((u32*)skb->data)[2] = ((u32*)skb->data)[1];
                                ((u32*)skb->data)[1] = ((u32*)skb->data)[0];
                                skb_pull(skb, 4);
                            }
                        } else {
                            /*
                             * Mark packet as VLAN-tagged, otherwise newer
                             * kernels will strip the tag.
                             */
                            uint16_t tci = PKT_U16_GET(pkt, 14);

                            if (priv->flags & KCOM_NETIF_F_RCPU_ENCAP) {
                                bkn_vlan_hwaccel_put_tag(skb, ETH_P_8021Q, tci);
                            } else {
                                if (vlan_proto == ETH_P_8021AD) {
                                    bkn_vlan_hwaccel_put_tag
                                        (skb, ETH_P_8021AD, tci);
                                } else {
                                    bkn_vlan_hwaccel_put_tag
                                        (skb, ETH_P_8021Q, tci);
                                }
                            }
                        }
                    }

                    priv->stats.rx_packets++;
                    priv->stats.rx_bytes += skb->len;

                    /* Optional SKB updates */
                    KNET_SKB_CB(skb)->dcb_type = sinfo->dcb_type & 0xFFFF;
                    if (knet_rx_cb != NULL) {
                        KNET_SKB_CB(skb)->netif_user_data = priv->cb_user_data;
                        KNET_SKB_CB(skb)->filter_user_data = filter->kf.cb_user_data;
                        if (device_is_sand(sinfo)) {
                            skb = knet_rx_cb(skb, sinfo->dev_no,
                                             sand_scratch_data);
                        }
                        else {
                            skb = knet_rx_cb(skb, sinfo->dev_no, meta);
                        }
                        if (skb == NULL) {
                            /* Consumed by call-back */
                            sinfo->rx[chan].pkts_d_callback++;
                            break;
                        }
                    }

                    /* Do Rx timestamping */
                    if (priv->rx_hwts) {
                        bkn_hw_tstamp_rx_set(sinfo, priv->phys_port, skb, meta);
                    }

                    if (priv->flags & KCOM_NETIF_F_RCPU_ENCAP) {
                        bkn_add_rcpu_encap(sinfo, skb, meta, pkt_hdr_size);
                        DBG_PDMP(("After add RCPU ENCAP\n"));
                        bkn_dump_pkt(skb->data, pktlen + RCPU_RX_ENCAP_SIZE,
                                     XGS_DMA_RX_CHAN);
                    }
                    skb->protocol = eth_type_trans(skb, skb->dev);
                    if (filter->kf.dest_proto) {
                        skb->protocol = filter->kf.dest_proto;
                    }
                    if (priv->flags & KCOM_NETIF_F_RCPU_ENCAP) {
                        bkn_eth_type_update(skb, ethertype);
                    }
                    DBG_DUNE(("skb protocol 0x%04x\n", skb->protocol));

                    /*
                     * Disable configuration API while the spinlock is released.
                     */
                    sinfo->cfg_api_locked = 1;
                    /* Unlock while calling up network stack */
                    spin_unlock(&sinfo->lock);
                    if (use_napi) {
                        netif_receive_skb(skb);
                    } else {
                        netif_rx(skb);
                    }
                    spin_lock(&sinfo->lock);
                    /* Re-enable configuration API once spinlock is regained. */
                    sinfo->cfg_api_locked = 0;

                    if (filter->kf.mirror_type == KCOM_DEST_T_API ||
                        dbg_pkt_enable) {
                        DBG_FLTR(("Mirror to Rx API\n"));
                        sinfo->rx[chan].pkts_m_api++;
                        drop_api = 0;
                    }
                } else {
                    DBG_FLTR(("Unknown netif %d\n",
                              filter->kf.dest_id));
                    sinfo->rx[chan].pkts_d_unkn_netif++;
                }
                break;
            default:
                /* Drop packet */
                DBG_FLTR(("Unknown dest type %d\n",
                          filter->kf.dest_type));
                sinfo->rx[chan].pkts_d_unkn_dest++;
                break;
            }
        } else {
            DBG_PKT(("Rx packet dropped.\n"));
            sinfo->rx[chan].pkts_d_no_match++;
        }
        if (drop_api) {
            /* If count is zero, the DCB will just be recycled */
            dcb[sinfo->dcb_wsize-1] &= ~SOC_DCB_KNET_COUNT_MASK;
        }
        dcb[sinfo->dcb_wsize-1] |= SOC_DCB_KNET_DONE;
        dcb_chain->dcb_cur++;
        dcbs_done++;
    }

    return dcbs_done;
}

/*
 * Process skb to netif.
 * Caller need to make sure dest_id is the valid net interface.
 */
static int
bkn_skb_rx_netif_process(bkn_switch_info_t *sinfo, int dest_id, int chan,
                         struct sk_buff *skb, bkn_filter_t *filter,
                         uint32 *rx_cb_meta, uint32 *meta,
                         int pkt_hdr_size, int pktlen, int ethertype)
{
    bkn_priv_t *priv;

    DBG_VERB(("Process SKB to netif %d\n", dest_id));
    priv = bkn_netif_lookup(sinfo, dest_id);
    if (priv == NULL) {
        DBG_VERB(("Private data is unavailable for netif %d\n", dest_id));
        /* Handle skb on error return */
        dev_kfree_skb_any(skb);
        return -1;
    }

    if ((priv->flags & KCOM_NETIF_F_KEEP_RX_TAG) == 0) {
        uint16_t vlan_proto;

        vlan_proto = PKT_U16_GET(skb->data, 12);
        if ((filter->kf.flags & KCOM_FILTER_F_STRIP_TAG) == 0) {
            /*
             * Mark packet as VLAN-tagged, otherwise newer
             * kernels will strip the tag.
             */
            uint16_t tci = PKT_U16_GET(skb->data, 14);

            if (priv->flags & KCOM_NETIF_F_RCPU_ENCAP) {
                bkn_vlan_hwaccel_put_tag(skb, ETH_P_8021Q, tci);
            } else {
                if (vlan_proto == ETH_P_8021AD) {
                    bkn_vlan_hwaccel_put_tag
                        (skb, ETH_P_8021AD, tci);
                } else {
                    bkn_vlan_hwaccel_put_tag
                        (skb, ETH_P_8021Q, tci);
                }
            }
        }
    }

    priv->stats.rx_packets++;
    priv->stats.rx_bytes += skb->len;
    skb->dev = priv->dev;

    if (knet_rx_cb != NULL) {
        KNET_SKB_CB(skb)->netif_user_data = priv->cb_user_data;
        KNET_SKB_CB(skb)->filter_user_data = filter->kf.cb_user_data;
        skb = knet_rx_cb(skb, sinfo->dev_no, rx_cb_meta);
        if (skb == NULL) {
            /* Consumed by call-back */
            sinfo->rx[chan].pkts_d_callback++;
            priv->stats.rx_dropped++;
            return -1;
        }
    }

    if (priv->flags & KCOM_NETIF_F_RCPU_ENCAP) {
        bkn_add_rcpu_encap(sinfo, skb, meta, pkt_hdr_size);
        DBG_PDMP(("After add RCPU ENCAP\n"));
        bkn_dump_pkt(skb->data, pktlen + RCPU_RX_ENCAP_SIZE,
                     XGS_DMA_RX_CHAN);
    }
    skb->protocol = eth_type_trans(skb, skb->dev);
    if (filter->kf.dest_proto) {
        skb->protocol = filter->kf.dest_proto;
    }
    if (priv->flags & KCOM_NETIF_F_RCPU_ENCAP) {
        bkn_eth_type_update(skb, ethertype);
    }
    DBG_DUNE(("skb protocol 0x%04x\n",skb->protocol));
    if (filter->kf.mirror_type == KCOM_DEST_T_NETIF &&
        filter->kf.mirror_proto) {
        skb->protocol = filter->kf.mirror_proto;
    }

    return 0;
}

static int
bkn_do_skb_rx(bkn_switch_info_t *sinfo, int chan, int budget)
{
    bkn_priv_t *priv;
    bkn_desc_info_t *desc;
    struct sk_buff *skb;
    bkn_filter_t cbf;
    bkn_filter_t *filter = NULL;
    uint32_t err_woff;
    uint32_t *dcb, *meta, *match_data;
    int pktlen, pkt_hdr_size = 0;
    uint8_t skip_hdrlen = 0;
    uint8_t eth_offset = 0;
    int idx;
    int dcbs_done = 0;
    bkn_dune_system_header_info_t packet_info = {0};
    uint32_t sand_scratch_data[BKN_SAND_SCRATCH_DATA_SIZE] = {0};
    uint8_t *pkt = NULL;
    bkn_priv_t *mpriv;
    struct sk_buff *mskb = NULL;
    uint32_t *rx_cb_meta;
    int metalen;

    if (!sinfo->rx[chan].running) {
        /* Rx not ready */
        return 0;
    }

    while (dcbs_done < budget) {
        char str[32];

        if (!sinfo->rx[chan].running) {
            /* DCBs might be cleaned up when bkn_knet_hw_reset is triggered. */
            return 0;
        }
        sprintf(str, "Rx DCB (%d)", sinfo->rx[chan].dirty);
        desc = &sinfo->rx[chan].desc[sinfo->rx[chan].dirty];
        dcb = desc->dcb_mem;
        bkn_dump_dcb(str, dcb, sinfo->dcb_wsize, XGS_DMA_RX_CHAN);
        if ((dcb[sinfo->dcb_wsize-1] & (1 << 31)) == 0) {
            break;
        }
        if ((((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) && (dcb[2] & (1 << 16)) == 0) ||
            (((sinfo->cmic_type != 'x') && (sinfo->cmic_type != 'r')) && (dcb[1] & (1 << 16)) == 0)) {
            sinfo->rx[chan].chain_complete = 1;
            /* Request one extra poll to check for chain done interrupt */
            if (sinfo->napi_poll_mode) {
                sinfo->napi_poll_again = 1;
            }
        }
        sinfo->rx[chan].pkts++;
        skb = desc->skb;

        DBG_DCB_RX(("Rx%d SKB DMA done (%d).\n", chan, sinfo->rx[chan].dirty));
        BKN_DMA_UNMAP_SINGLE(sinfo->dma_dev,
                             desc->skb_dma, desc->dma_size,
                             BKN_DMA_FROMDEV);
        desc->skb_dma = 0;

        pktlen = dcb[sinfo->dcb_wsize-1] & 0xffff;
        priv = netdev_priv(sinfo->dev);
        bkn_dump_pkt(skb->data, pktlen, XGS_DMA_RX_CHAN);

        if (device_is_sand(sinfo)) {
            err_woff = BKN_SAND_SCRATCH_DATA_SIZE - 1;
            /* Last DCB word with error bit */
            sand_scratch_data[err_woff] = dcb[sinfo->dcb_wsize-1];
            /* Data starts with DNX system headers and the length is specified by pkt_hdr_size */
            meta = (uint32_t *)skb->data;
            pkt = skb->data;
            memset(&packet_info, 0, sizeof(bkn_dune_system_header_info_t));
            if (ENET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
                pkt_hdr_size = 0;
            } else {
                /* Decode system headers and fill scratch data */
                bkn_packet_header_parse(sinfo, pkt, (uint32_t)pktlen, &packet_info);
                pkt_hdr_size = packet_info.system_header_size;
            }

            if (knet_hw_tstamp_rx_pre_process_cb) {
                if ((knet_hw_tstamp_rx_pre_process_cb(sinfo->dev_no, pkt, packet_info.ftmh.source_sys_port_aggregate, NULL)) >= 0) {
                    skip_hdrlen = pkt_hdr_size;
                    sand_scratch_data[err_woff] = (pktlen - skip_hdrlen);

                    meta = (uint32_t *)(skb->data + skip_hdrlen);
                    pkt = (skb->data + skip_hdrlen);
                    memset(&packet_info, 0, sizeof(bkn_dune_system_header_info_t));
                    if (ENET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
                        pkt_hdr_size = 0;
                    } else {
                        /* Decode system headers and fill scratch data */
                        bkn_packet_header_parse(sinfo, pkt, (uint32_t)(pktlen - skip_hdrlen), &packet_info);
                        pkt_hdr_size = packet_info.system_header_size;
                    }
                    knet_hw_tstamp_rx_pre_process_cb(sinfo->dev_no, pkt + pkt_hdr_size,
                                                     packet_info.ftmh.source_sys_port_aggregate, (int *)&eth_offset);
                }
            }
        } else {
            if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                meta = (uint32_t *)skb->data;
                err_woff = sinfo->pkt_hdr_size / sizeof(uint32_t) - 1;
                meta[err_woff] = dcb[sinfo->dcb_wsize-1];
            } else {
                /* Packet sent from R5(KNETSYNC) has EP_TO_CPU_HDR in skb->data
                 * and not in dcb header. Information within the EP_TO_CPU_HDR
                 * is considered as metadata to find the KNET filter.
                 *
                 * Below WAR implementation is to copy the EP_TO_CPU_HDR from
                 * sdk->data to dcb header(with EP_TO_CPU as 0's) for further
                 * packet processing.
                 */
                if (sinfo->dcb_type == 37 && dcb[5] == 0) {
                    metalen = (sinfo->dcb_wsize - 3) * sizeof(uint32_t);
                    meta = (uint32_t *)skb->data;
                    for (idx = 0; idx < BYTES2WORDS(metalen); idx++) {
                        dcb[idx + 2] = ntohl(meta[idx]);
                    }
                    skip_hdrlen = metalen;
                }
                meta = dcb;
                err_woff = sinfo->dcb_wsize - 1;
            }
            pkt_hdr_size = sinfo->pkt_hdr_size;
        }

        /* Minimun size: header_size + MACs + VLAN + ETH_TYPE */
        if (pktlen > pkt_hdr_size + 18) {
            if (device_is_sand(sinfo)) {
                bkn_bitstream_set_field(sand_scratch_data, 0,  16,
                                        packet_info.internal.trap_id);
                bkn_bitstream_set_field(sand_scratch_data, 16, 16,
                                        packet_info.internal.trap_qualifier);
                bkn_bitstream_set_field(sand_scratch_data, 32, 17,
                                        packet_info.ftmh.source_sys_port_aggregate);
                bkn_bitstream_set_field(sand_scratch_data, 64, 2,
                                        packet_info.ftmh.action_type);
                bkn_bitstream_set_field(sand_scratch_data, 66, 18,
                                        packet_info.internal.forward_domain);
                if (force_tagged) {
                    uint8_t *eth_hdr = pkt + pkt_hdr_size;
                    uint16_t tpid = 0;

                    if (skip_hdrlen > 0) {
                        eth_hdr += eth_offset;
                    }
                    tpid = PKT_U16_GET(eth_hdr, 12);
                    if (packet_is_untagged(tpid)) {
                        int raw_packet_len = pktlen - pkt_hdr_size;
                        uint32_t vid = 0;
                        uint32_t is_vsi = BKN_DNX_PPH_FWD_DOMAIN_IS_VSI(packet_info.internal.forward_domain);
                        uint32_t fwd_domain = BKN_DNX_PPH_FWD_DOMAIN_ID_GET(packet_info.internal.forward_domain);

                        if ((pktlen + 4) < rx_buffer_size) {
                            for (idx = (raw_packet_len - 1); idx >= 12; idx--) {
                                eth_hdr[idx+4] = eth_hdr[idx];
                            }
                            if (ft_vid) {
                                vid = ft_vid;
                            }
                            else if (is_vsi && fwd_domain) {
                                vid = fwd_domain & 0xfff;
                            }
                            else {
                                vid = 1;
                            }
                            DBG_DUNE(("add vlan tag (%d) to untagged packets\n",
                                      vid));
                            eth_hdr[12] = (ft_tpid >> 8) & 0xff;
                            eth_hdr[13] = ft_tpid & 0xff;
                            eth_hdr[14] = (((ft_pri & 0x7) << 5) |
                                           ((ft_cfi & 0x1) << 4) |
                                           ((vid >> 8) & 0xf)) & 0xff;
                            eth_hdr[15] = vid & 0xff;
                            /* reset packet length in DCB */
                            pktlen += 4;
                            bkn_dump_pkt(pkt, pktlen, XGS_DMA_RX_CHAN);
                            dcb[sinfo->dcb_wsize-1] &= ~SOC_DCB_KNET_COUNT_MASK;
                            dcb[sinfo->dcb_wsize-1] |= pktlen &
                                                       SOC_DCB_KNET_COUNT_MASK;
                        }
                    }
                }
            }
            if (device_is_sand(sinfo)) {
                match_data = sand_scratch_data;
            } else {
                match_data = meta;
            }
            filter = bkn_match_rx_pkt(sinfo, skb->data + pkt_hdr_size + skip_hdrlen,
                                      pktlen - pkt_hdr_size, match_data,
                                      chan, &cbf);

            if ((dcb[sinfo->dcb_wsize-1] & 0xf0000) != 0x30000) {
                /* Fragment or error */
                priv->stats.rx_errors++;
                if (filter && filter->kf.mask.w[err_woff] == 0) {
                    /* Drop unless DCB status is part of filter */
                    filter = NULL;
                }
            }
        }
        DBG_PKT(("Rx packet (%d bytes).\n", pktlen));
        if (filter) {
            DBG_FLTR(("Match filter ID %d\n", filter->kf.id));
            switch (filter->kf.dest_type) {
            case KCOM_DEST_T_API:
                DBG_FLTR(("Send to Rx API\n"));
                sinfo->rx[chan].pkts_f_api++;
                bkn_api_rx_copy_from_skb(sinfo, chan, desc, 0);
                break;
            case KCOM_DEST_T_NETIF:
                priv = bkn_netif_lookup(sinfo, filter->kf.dest_id);
                if (priv) {
                    int ethertype;

                    /* Check that software link is up */
                    if (!bkn_netif_ok(priv->dev)) {
                        sinfo->rx[chan].pkts_d_no_link++;
                        break;
                    }
                    DBG_FLTR(("Send to netif %d (%s)\n",
                              priv->id, priv->dev->name));
                    sinfo->rx[chan].pkts_f_netif++;

                    if ((filter->kf.mirror_type == KCOM_DEST_T_API) ||
                        dbg_pkt_enable) {
                        sinfo->rx[chan].pkts_m_api++;
                        bkn_api_rx_copy_from_skb(sinfo, chan, desc,
                                                 priv->rx_hwts);
                    }

                    if (device_is_sand(sinfo)) {
                        /* CRC has been stripped on Dune*/
                        skb_put(skb, pktlen);
                    } else {
                        skb_put(skb, pktlen - 4); /* Strip CRC */
                    }

                    skb_pull(skb, (pkt_hdr_size + skip_hdrlen));

                    /* Optional SKB updates */
                    KNET_SKB_CB(skb)->dcb_type = sinfo->dcb_type & 0xFFFF;
                    /* Do Rx timestamping */
                    if (priv->rx_hwts) {
                        if ((bkn_hw_tstamp_rx_set(sinfo, priv->phys_port, skb,
                                                  meta)) >= 0) {

                            /* Increment ptp rx counters. */
                            priv->ptp_stats_rx++;
                        }
                    }

                    /* Save for RCPU before stripping tag */
                    ethertype = PKT_U16_GET(skb->data, 16);
                    if ((priv->flags & KCOM_NETIF_F_KEEP_RX_TAG) == 0) {
                        uint16_t vlan_proto;

                        vlan_proto = PKT_U16_GET(skb->data, 12);
                        if (filter->kf.flags & KCOM_FILTER_F_STRIP_TAG) {
                            /* Strip VLAN tag */
                            if (vlan_proto == ETH_P_8021Q ||
                                vlan_proto == ETH_P_8021AD) {
                                DBG_FLTR(("Strip VLAN tag\n"));
                                ((u32*)skb->data)[3] = ((u32*)skb->data)[2];
                                ((u32*)skb->data)[2] = ((u32*)skb->data)[1];
                                ((u32*)skb->data)[1] = ((u32*)skb->data)[0];
                                skb_pull(skb, 4);
                                if (device_is_sand(sinfo)) {
                                    /* Headers shifted left 4 bytes*/
                                    for (idx = pkt_hdr_size; idx >= 0; idx--) {
                                        pkt[idx + 4] = pkt[idx];
                                    }
                                    pkt += 4;
                                    meta++;
                                } else if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                                    for (idx = pkt_hdr_size / sizeof(uint32_t);
                                         idx; idx--) {
                                        meta[idx] = meta[idx - 1];
                                    }
                                    meta++;
                                }
                            }
                        }
                    }
                    if (device_is_sand(sinfo)) {
                        rx_cb_meta = sand_scratch_data;
                    } else {
                        rx_cb_meta = meta;
                    }

                    if (mirror_local) {
                        /* Clone skb before processing netif specific config. */
                        if (filter->kf.mirror_type == KCOM_DEST_T_NETIF) {
                            mpriv = bkn_netif_lookup(sinfo, filter->kf.mirror_id);
                            /* Clone skb for mirror_to netinf */
                            if (mpriv && bkn_netif_ok(mpriv->dev)) {
                                mskb = skb_clone(skb, GFP_ATOMIC);
                                if (mskb == NULL) {
                                    sinfo->rx[chan].pkts_d_no_skb++;
                                }
                            }
                        }
                    }

                    /* Process original netif specific config. */
                    if (bkn_skb_rx_netif_process(sinfo, filter->kf.dest_id,
                                                 chan, skb, filter, rx_cb_meta,
                                                 meta, pkt_hdr_size, pktlen,
                                                 ethertype)) {
                        desc->skb = NULL;
                        break;
                    }

                    if (mirror_local) {
                        if (mskb) {
                            /* Process mirorr_to netif specific config. */
                            if (bkn_skb_rx_netif_process(sinfo,
                                                         filter->kf.mirror_id,
                                                         chan, mskb, filter,
                                                         rx_cb_meta, meta,
                                                         pkt_hdr_size,
                                                         pktlen, ethertype)){
                                desc->skb = NULL;
                                break;
                            }
                        }
                    } else {
                        /*
                         * Clone skb processed with original netif specific
                         * config.
                         */
                        /* Clone skb for mirror_to netinf */
                        if (filter->kf.mirror_type == KCOM_DEST_T_NETIF) {
                            mpriv = bkn_netif_lookup(sinfo, filter->kf.mirror_id);
                            if (mpriv && bkn_netif_ok(mpriv->dev)) {
                                mskb = skb_clone(skb, GFP_ATOMIC);
                                if (mskb == NULL) {
                                    sinfo->rx[chan].pkts_d_no_skb++;
                                } else {
                                    mpriv->stats.rx_packets++;
                                    mpriv->stats.rx_bytes += mskb->len;
                                    mskb->dev = mpriv->dev;
                                    if (filter->kf.mirror_proto) {
                                        mskb->protocol = filter->kf.mirror_proto;
                                    }
                                }
                            }
                        }
                    }
                    if (mskb) {
                        /* Send up to mirror_to netif */
                        sinfo->rx[chan].pkts_m_netif++;
                        /*
                        * Disable configuration API while the spinlock
                        * is released.
                        */
                        sinfo->cfg_api_locked = 1;

                        /* Unlock while calling up network stack */
                        spin_unlock(&sinfo->lock);
                        if (use_napi) {
                            netif_receive_skb(mskb);
                        } else {
                            netif_rx(mskb);
                        }
                        spin_lock(&sinfo->lock);
                        /*
                        * Re-enable configuration API once the spinlock
                        * is regained.
                        */
                        sinfo->cfg_api_locked = 0;
                    }

                    /* Ensure that we reallocate SKB for this DCB */
                    desc->skb = NULL;
                    /*
                     * Disable configuration API while the spinlock
                     * is released.
                     */
                    sinfo->cfg_api_locked = 1;

                    /* Unlock while calling up network stack */
                    spin_unlock(&sinfo->lock);
                    if (use_napi) {
                        netif_receive_skb(skb);
                    } else {
                        netif_rx(skb);
                    }
                    spin_lock(&sinfo->lock);
                    /*
                     * Re-enable configuration API once the spinlock
                     * is regained.
                     */
                    sinfo->cfg_api_locked = 0;
                } else {
                    DBG_FLTR(("Unknown netif %d\n",
                              filter->kf.dest_id));
                    sinfo->rx[chan].pkts_d_unkn_netif++;
                }
                break;
            default:
                /* Drop packet */
                DBG_FLTR(("Unknown dest type %d\n",
                          filter->kf.dest_type));
                sinfo->rx[chan].pkts_d_unkn_dest++;
                break;
            }
        } else {
            DBG_PKT(("Rx packet dropped.\n"));
            sinfo->rx[chan].pkts_d_no_match++;
            priv->stats.rx_dropped++;
        }
        dcb[sinfo->dcb_wsize-1] &= ~(1 << 31);
        if (++sinfo->rx[chan].dirty >= MAX_RX_DCBS) {
            sinfo->rx[chan].dirty = 0;
        }
        sinfo->rx[chan].free--;
        dcbs_done++;
        if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            /* Right now refill for Continuous DMA mode */
            bkn_rx_refill(sinfo, chan);
        }
    }

    return dcbs_done;
}

static int
bkn_do_rx(bkn_switch_info_t *sinfo, int chan, int budget)
{
    if (sinfo->rx[chan].use_rx_skb == 0) {
        /* Rx buffers are provided by BCM Rx API */
        return bkn_do_api_rx(sinfo, chan, budget);
    } else {
        /* Rx buffers are provided by Linux kernel */
        return bkn_do_skb_rx(sinfo, chan, budget);
    }
}

static void
bkn_rx_debug_dump(bkn_switch_info_t *sinfo, int chan)
{
    uint32_t *dcb = NULL;
    bkn_dcb_chain_t *dcb_chain = NULL;
    struct list_head *curr, *next;
    int cnt;
    uint32_t irq_stat, dma_stat, dma_ctrl;

    gprintk("Rx%d DCB info (dev_no %d):\n"
            "  api:   %d\n"
            "  dirty: %d\n"
            "  cur:   %d\n"
            "  free:  %d\n"
            "  run:   %d\n",
            chan, sinfo->dev_no,
            sinfo->rx[chan].api_active,
            sinfo->rx[chan].dirty,
            sinfo->rx[chan].cur,
            sinfo->rx[chan].free,
            sinfo->rx[chan].running);
    if (sinfo->rx[chan].use_rx_skb) {
        gprintk("  [0x%08lx]--->\n",
                (unsigned long)sinfo->rx[chan].desc[0].dcb_dma);
        for (cnt = 0; cnt < MAX_RX_DCBS; cnt++) {
            dcb = sinfo->rx[chan].desc[cnt].dcb_mem;
            if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                gprintk("  DCB %2d: 0x%08x 0x%08x 0x%08x 0x%08x\n", cnt,
                        dcb[0], dcb[1], dcb[2], dcb[sinfo->dcb_wsize-1]);
            } else {
                gprintk("  DCB %2d: 0x%08x 0x%08x ... 0x%08x\n", cnt,
                        dcb[0], dcb[1], dcb[sinfo->dcb_wsize-1]);
            }
        }
    } else {
        curr = &sinfo->rx[chan].api_dcb_list;
        dcb_chain = sinfo->rx[chan].api_dcb_chain;
        while (dcb_chain) {
            gprintk("  [0x%08lx]--->\n", (unsigned long)dcb_chain->dcb_dma);
            for (cnt = 0; cnt < dcb_chain->dcb_cnt; cnt++) {
                dcb = &dcb_chain->dcb_mem[sinfo->dcb_wsize * cnt];
                if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                    gprintk("  DCB %2d: 0x%08x 0x%08x 0x%08x 0x%08x\n", cnt,
                            dcb[0], dcb[1], dcb[2], dcb[sinfo->dcb_wsize-1]);
                } else {
                    gprintk("  DCB %2d: 0x%08x 0x%08x ... 0x%08x\n", cnt,
                            dcb[0], dcb[1], dcb[sinfo->dcb_wsize-1]);
                }
            }
            next = curr->next;
            if (next != &sinfo->rx[chan].api_dcb_list) {
                dcb_chain = list_entry(next, bkn_dcb_chain_t, list);
                curr = next;
            } else {
                dcb_chain = NULL;
            }
        }
    }

    irq_stat = dma_stat = dma_ctrl = 0;
    if (DEV_IS_CMICR(sinfo)) {
        dev_read32(sinfo,
                   CMICR_INTR_STATr,
                   &irq_stat);
        dev_read32(sinfo,
                   CMICR_DMA_STATr,
                   &dma_stat);
        dev_read32(sinfo,
                   CMICR_DMA_CTRLr + 0x80 * (XGS_DMA_RX_CHAN + chan),
                   &dma_ctrl);
    } else if (DEV_IS_CMICX(sinfo)) {
        dev_read32(sinfo,
                   CMICX_IRQ_STATr,
                   &irq_stat);
        dev_read32(sinfo,
                   CMICX_DMA_STATr,
                   &dma_stat);
        dev_read32(sinfo,
                   CMICX_DMA_CTRLr + 0x80 * (XGS_DMA_RX_CHAN + chan),
                   &dma_ctrl);
    } else if (DEV_IS_CMICM(sinfo)) {
        dev_read32(sinfo,
                   CMICM_IRQ_STATr,
                   &irq_stat);
        dev_read32(sinfo,
                   CMICM_DMA_STATr,
                   &dma_stat);
        dev_read32(sinfo,
                   CMICM_DMA_CTRLr + 0x80 * (XGS_DMA_RX_CHAN + chan),
                   &dma_ctrl);
    } else {
        dev_read32(sinfo,
                   CMIC_IRQ_STATr,
                   &irq_stat);
        dev_read32(sinfo,
                   CMIC_DMA_STATr,
                   &dma_stat);
        dev_read32(sinfo,
                   CMIC_DMA_CTRLr + 0x80 * (XGS_DMA_RX_CHAN + chan),
                   &dma_ctrl);
    }
    gprintk("CMIC irq_stat:0x%08x dma_stat:0x%08x dma_ctrl(RX%d):0x%08x \n",
            irq_stat, dma_stat, chan, dma_ctrl);
}

static void
bkn_rx_desc_done(bkn_switch_info_t *sinfo, int chan)
{
    bkn_evt_resource_t *evt;
    evt = &_bkn_evt[sinfo->evt_idx];
    DBG_IRQ(("Rx%d desc done\n", chan));

    if (sinfo->rx[chan].use_rx_skb == 0) {
        sinfo->dma_events |= KCOM_DMA_INFO_F_RX_DONE;
        evt->evt_wq_put++;
        wake_up_interruptible(&evt->evt_wq);
    }
}

static void
bkn_rx_chain_done(bkn_switch_info_t *sinfo, int chan)
{
    DBG_IRQ(("Rx%d chain done\n", chan));

    if (sinfo->rx[chan].chain_complete == 0) {
        /*
         * In certain environments the DCB memory is updated after
         * the corresponding interrupt has been received.
         * The following code will ensure that this situation is
         * handled properly.
         */
        int maxloop = 0;
        while (sinfo->rx[chan].chain_complete == 0) {
            sinfo->rx[chan].sync_retry++;
            if (maxloop == 0) {
                sinfo->rx[chan].sync_err++;
            }
            if (maxloop > sinfo->rx[chan].sync_maxloop) {
                sinfo->rx[chan].sync_maxloop = maxloop;
            }
            if (bkn_do_rx(sinfo, chan, MAX_RX_DCBS) > 0) {
                bkn_rx_desc_done(sinfo, chan);
            }
            if (++maxloop > rx_sync_retry) {
                gprintk("Fatal error: Incomplete chain\n");
                bkn_rx_debug_dump(sinfo, chan);
                sinfo->rx[chan].chain_complete = 1;
                break;
            }
        }
    }

    sinfo->rx[chan].running = 0;

    if (sinfo->rx[chan].use_rx_skb == 0) {
        bkn_api_rx_chain_done(sinfo, chan);
    } else {
        bkn_rx_refill(sinfo, chan);

        if (bkn_rx_restart(sinfo, chan) != 0) {
            /* Presumably out of resources */
            sinfo->timer.expires = jiffies + 1;
            if (!sinfo->timer_queued) {
                sinfo->timer_queued = 1;
                add_timer(&sinfo->timer);
            }
        }
    }
}

static void
bkn_suspend_tx(bkn_switch_info_t *sinfo)
{
    struct list_head *list;
    bkn_priv_t *priv = netdev_priv(sinfo->dev);

    /* Stop main device */
    netif_stop_queue(priv->dev);
    sinfo->tx.suspends++;
    /* Stop associated virtual devices */
    list_for_each(list, &sinfo->ndev_list) {
        priv = (bkn_priv_t *)list;
        if (priv->dev) {
            netif_stop_queue(priv->dev);
        }
    }
}

static void
bkn_resume_tx(bkn_switch_info_t *sinfo)
{
    struct list_head *list;
    bkn_priv_t *priv = netdev_priv(sinfo->dev);

    /* Check main device */
    if (netif_queue_stopped(priv->dev) && sinfo->tx.free > 1) {
        netif_wake_queue(priv->dev);
    }
    /* Check associated virtual devices */
    list_for_each(list, &sinfo->ndev_list) {
        priv = (bkn_priv_t *)list;
        if (priv->dev) {
            if (netif_queue_stopped(priv->dev) && sinfo->tx.free > 1) {
                netif_wake_queue(priv->dev);
            }
        }
    }
}

static void
bkn_skb_tstamp_copy(struct sk_buff *new_skb, struct sk_buff *skb)
{
    bkn_skb_tx_flags(new_skb) = bkn_skb_tx_flags(skb);
    new_skb->sk = skb->sk;

    return;
}

static int
bkn_hw_tstamp_tx_set(bkn_switch_info_t *sinfo, struct sk_buff *skb)
{
    int hwts;
    int port;
    uint64_t ts = 0;
    uint32_t hdrlen = ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) ? PKT_TX_HDR_SIZE : 0;
    struct skb_shared_hwtstamps shhwtstamps;

    if (!knet_hw_tstamp_tx_time_get_cb) {
        return -1;
    }

    port = KNET_SKB_CB(skb)->port;
    hwts = KNET_SKB_CB(skb)->hwts;
    ts = KNET_SKB_CB(skb)->ts;

    if (device_is_dnx(sinfo)) {
        port =  port + 1;
        if (knet_hw_tstamp_tx_time_get_cb(sinfo->dev_no, port, skb->data + hdrlen, &ts, hwts) < 0) {
            DBG_VERB(("Error in setting tx tstsamp \n"));
            return -1;
        }
    } else {
        if (hwts == HWTSTAMP_TX_ONESTEP_SYNC) {
            if (ts == 0) {
                return 1;
            }
        } else if (hwts == HWTSTAMP_TX_ON) {
            if (knet_hw_tstamp_tx_time_get_cb(sinfo->dev_no, port, skb->data + hdrlen, &ts, hwts) < 0) {
                DBG_VERB(("Error in setting tx tstsamp \n"));
                return -1;
            }
        }
    }

    memset(&shhwtstamps, 0, sizeof(shhwtstamps));
    shhwtstamps.hwtstamp = ns_to_ktime(ts);
    skb_tstamp_tx(skb, &shhwtstamps);

    return 0;
}

static void
bkn_hw_tstamp_tx_work(struct work_struct *work)
{
    bkn_switch_info_t *sinfo = container_of(work, bkn_switch_info_t, tx_ptp_work);
    struct sk_buff *skb;
    int ret;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
    ktime_t tstamp;
#endif

    while (skb_queue_len(&sinfo->tx_ptp_queue)) {
        skb = skb_dequeue(&sinfo->tx_ptp_queue);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
        tstamp = skb->tstamp;
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,13,0))
        /*
         * For Linux versions with simultaneous SW and HW TS support.
         * If the driver sends out SW TS included in skb->tstamp, then the
         * Kernel will no longer send the HW TS to the application,
         * so set the populated skb->tstamp to 0.
         */
        skb->tstamp = 0;
#endif

        ret = bkn_hw_tstamp_tx_set(sinfo, skb);
        if (ret < 0) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
            ktime_t now;
            now = ktime_get();
            DBG_PTP(("2Step TX Timestamp has not been taken for the current skb (%lld us)\n",
                                    ktime_us_delta(now, tstamp)));
        } else {
            ktime_t now;
            now = ktime_get();
            /* Timeout 20 should be same as configured by PTP4L */
            if (ktime_us_delta(now, tstamp) >= 20000) {
                DBG_PTP(("2Step TX Timestamp fetch took long time %lld us\n",
                            ktime_us_delta(now, tstamp)));
            }
#else
            DBG_PTP(("2Step TX Timestamp has not been taken for the current skb\n"));
#endif
        }
        dev_kfree_skb_any(skb);
    }
}

static int
bkn_do_tx(bkn_switch_info_t *sinfo)
{
    bkn_desc_info_t *desc;
    int dcbs_done = 0;

    if (!CDMA_CH(sinfo, XGS_DMA_TX_CHAN) && sinfo->tx.api_active) {
        return dcbs_done;
    }

    while (dcbs_done < MAX_TX_DCBS) {
        char str[32];
        if (sinfo->tx.free == MAX_TX_DCBS) {
            break;
        }
        sprintf(str, "Tx DCB (%d)", sinfo->tx.dirty);
        desc = &sinfo->tx.desc[sinfo->tx.dirty];
        bkn_dump_dcb(str, desc->dcb_mem, sinfo->dcb_wsize, XGS_DMA_TX_CHAN);
        if ((desc->dcb_mem[sinfo->dcb_wsize-1] & (1 << 31)) == 0) {
            break;
        }
        if (desc->skb) {
            DBG_DCB_TX(("Tx SKB DMA done (%d).\n", sinfo->tx.dirty));
            BKN_DMA_UNMAP_SINGLE(sinfo->dma_dev,
                             desc->skb_dma, desc->dma_size,
                             BKN_DMA_TODEV);

            if ((KNET_SKB_CB(desc->skb)->hwts == HWTSTAMP_TX_ONESTEP_SYNC) &&
                (bkn_skb_tx_flags(desc->skb) & SKBTX_IN_PROGRESS)) {

                if (bkn_hw_tstamp_tx_set(sinfo, desc->skb) < 0) {
                    DBG_PTP(("1Step timestamp has not been taken for the current skb.\n"));
                }
                bkn_skb_tx_flags(desc->skb) &= ~SKBTX_IN_PROGRESS;
            }

            if (bkn_skb_tx_flags(desc->skb) & SKBTX_IN_PROGRESS) {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
                desc->skb->tstamp = ktime_get();
#endif
                skb_queue_tail(&sinfo->tx_ptp_queue, desc->skb);
                schedule_work(&sinfo->tx_ptp_work);
            } else {
                dev_kfree_skb_any(desc->skb);
            }
            desc->skb = NULL;
            desc->skb_dma = 0;
        }
        desc->dcb_mem[sinfo->dcb_wsize-1] &= ~(1 << 31);
        if (++sinfo->tx.dirty >= MAX_TX_DCBS) {
            sinfo->tx.dirty = 0;
        }
        if (++sinfo->tx.free > MAX_TX_DCBS) {
            gprintk("Too many free Tx DCBs(%d).\n", sinfo->tx.free);
        }
        dcbs_done++;
    }

    return dcbs_done;
}

static void
bkn_tx_cdma_chain_switch(bkn_switch_info_t *sinfo)
{
    bkn_dcb_chain_t *dcb_chain = sinfo->tx.api_dcb_chain;
    uint32_t *dcb_mem;
    uint64_t dcb_dma;
    int woffset;

    /* Switch between SKB Tx and API Tx for Continuous DMA mode */
    if (!sinfo->tx.api_active) {
        /*
         * Set the current SKB DCB as reload DCB and the last DCB of
         * the pending API chain as the new halt location.
         */
        sinfo->tx.api_active = 1;
        dcb_mem = sinfo->tx.desc[sinfo->tx.cur].dcb_mem;
        memset(dcb_mem, 0, sinfo->dcb_wsize * sizeof(uint32_t));
        dcb_mem[0] = dcb_chain->dcb_dma;
        if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
            dcb_mem[1] = DMA_TO_BUS_HI(dcb_chain->dcb_dma >> 32);
            dcb_mem[2] |= 1 << 24 | 1 << 18 | 1 << 16;
        } else {
            dcb_mem[1] |= 1 << 24 | 1 << 18 | 1 << 16;
        }
        if (++sinfo->tx.cur >= MAX_TX_DCBS) {
            sinfo->tx.cur = 0;
        }
        sinfo->tx.free--;
        woffset = (dcb_chain->dcb_cnt - 1) * sinfo->dcb_wsize;
        dcb_dma = dcb_chain->dcb_dma + woffset * sizeof(uint32_t);
        /* DMA run to the new halt location */
        bkn_cdma_goto(sinfo, XGS_DMA_TX_CHAN, dcb_dma);
    } else {
        /* Only need to set the current SKB DCB as the new halt location */
        sinfo->tx.api_active = 0;
        woffset = (dcb_chain->dcb_cnt - 1) * sinfo->dcb_wsize;
        dcb_mem = &dcb_chain->dcb_mem[woffset];
        dcb_mem[0] = sinfo->tx.desc[sinfo->tx.dirty].dcb_dma;
        if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
            dcb_mem[1] = DMA_TO_BUS_HI(sinfo->tx.desc[sinfo->tx.dirty].dcb_dma >> 32);
        }
        dcb_dma = sinfo->tx.desc[sinfo->tx.cur].dcb_dma;
        /* DMA run to the new halt location */
        bkn_cdma_goto(sinfo, XGS_DMA_TX_CHAN, dcb_dma);
    }
}

static void
bkn_skb_tx(bkn_switch_info_t *sinfo)
{
    if (sinfo->tx.api_active) {
        /* Switch from API Tx to SKB Tx */
        bkn_tx_cdma_chain_switch(sinfo);
    }
}

static void
bkn_api_tx(bkn_switch_info_t *sinfo)
{
    bkn_dcb_chain_t *dcb_chain;
    uint64_t pkt_dma;
    unsigned char *pktdata;
    int pktlen;
    int i;

    /* Assume that driver lock is held */
    if (list_empty(&sinfo->tx.api_dcb_list)) {
        sinfo->tx.api_active = 0;
    } else {
        sinfo->tx.pkts++;
        dcb_chain = list_entry(sinfo->tx.api_dcb_list.next,
                               bkn_dcb_chain_t, list);
        DBG_DCB_TX(("Start API Tx DMA, first DCB @ 0x%08x (%d DCBs).\n",
                    (uint32_t)dcb_chain->dcb_dma, dcb_chain->dcb_cnt));
        for (i = 0; i < dcb_chain->dcb_cnt && debug & DBG_LVL_PDMP; i++) {
            if (CDMA_CH(sinfo, XGS_DMA_TX_CHAN) && i == dcb_chain->dcb_cnt - 1) {
                break;
            }
            if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                pkt_dma = BUS_TO_DMA_HI(dcb_chain->dcb_mem[sinfo->dcb_wsize * i + 1]);
                pkt_dma = pkt_dma << 32 | dcb_chain->dcb_mem[sinfo->dcb_wsize * i];
            } else {
                pkt_dma = dcb_chain->dcb_mem[sinfo->dcb_wsize * i];
            }
            pktdata = kernel_bde->p2l(sinfo->dev_no, pkt_dma);
            if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                pktlen = dcb_chain->dcb_mem[sinfo->dcb_wsize * i + 2] & 0xffff;
            } else {
                pktlen = dcb_chain->dcb_mem[sinfo->dcb_wsize * i + 1] & 0xffff;
            }
            bkn_dump_pkt(pktdata, pktlen, XGS_DMA_TX_CHAN);
        }

        if (CDMA_CH(sinfo, XGS_DMA_TX_CHAN)) {
            sinfo->tx.api_dcb_chain = dcb_chain;
            if (!sinfo->tx.api_active) {
                /* Switch from SKB Tx to API Tx */
                bkn_tx_cdma_chain_switch(sinfo);
            }
            list_del(&dcb_chain->list);
        } else {
            sinfo->tx.api_active = 1;
            dev_dma_chan_clear(sinfo, XGS_DMA_TX_CHAN);
            dev_irq_mask_enable(sinfo, XGS_DMA_TX_CHAN, 1);
            dev_dma_chan_start(sinfo, XGS_DMA_TX_CHAN, dcb_chain->dcb_dma);
            list_del(&dcb_chain->list);
            kfree(dcb_chain);
        }
    }
}

static void
bkn_tx_cdma_chain_done(bkn_switch_info_t *sinfo, int done)
{
    int woffset;
    int dcbs_done = 0;
    bkn_evt_resource_t *evt;

    DBG_IRQ(("Tx CDMA chain done \n"));

    evt = &_bkn_evt[sinfo->evt_idx];

    if (sinfo->tx.api_active) {
        /* Drain API Tx chains */
        while (sinfo->tx.api_dcb_chain != sinfo->tx.api_dcb_chain_end) {
            woffset = sinfo->tx.api_dcb_chain->dcb_cnt * sinfo->dcb_wsize - 1;
            if (!(sinfo->tx.api_dcb_chain->dcb_mem[woffset] & (1 << 31))) {
                return;
            }
            sinfo->tx.api_dcb_chain->dcb_mem[woffset - sinfo->dcb_wsize] |= SOC_DCB_KNET_DONE;
            sinfo->dma_events |= KCOM_DMA_INFO_F_TX_DONE;
            evt->evt_wq_put++;
            wake_up_interruptible(&evt->evt_wq);
            kfree(sinfo->tx.api_dcb_chain);
            sinfo->tx.api_dcb_chain = NULL;
            bkn_api_tx(sinfo);
            if ((++dcbs_done + done) >= MAX_TX_DCBS) {
                if (sinfo->napi_poll_mode) {
                    /* Request one extra poll to reschedule Tx */
                    sinfo->napi_poll_again = 1;
                } else {
                    /* Request to yield for Continuous DMA mode */
                    sinfo->tx_yield = 1;
                }
                return;
            }
        }
        woffset = (sinfo->tx.api_dcb_chain->dcb_cnt - 1) * sinfo->dcb_wsize - 1;
        if (!(sinfo->tx.api_dcb_chain->dcb_mem[woffset] & (1 << 31))) {
            return;
        }
        sinfo->tx.api_dcb_chain->dcb_mem[woffset] |= SOC_DCB_KNET_DONE;
        /* Try and park at SKB Tx if API Tx done */
        bkn_skb_tx(sinfo);
        sinfo->dma_events |= KCOM_DMA_INFO_F_TX_DONE;
        evt->evt_wq_put++;
        wake_up_interruptible(&evt->evt_wq);
        kfree(sinfo->tx.api_dcb_chain);
        sinfo->tx.api_dcb_chain = NULL;
        sinfo->tx.api_dcb_chain_end = NULL;
        if (!sinfo->napi_poll_mode) {
            /* Not need to yield for Continuous DMA mode */
            sinfo->tx_yield = 0;
        }
    } else {
        if (sinfo->tx.free == MAX_TX_DCBS) {
            /* Try API Tx if SKB Tx done */
            bkn_api_tx(sinfo);
            if (sinfo->tx.api_active) {
                return;
            }
        }
    }

    /* Resume if netif Tx resources available and API Tx not active */
    bkn_resume_tx(sinfo);
}

static void
bkn_tx_chain_done(bkn_switch_info_t *sinfo, int done)
{
    bkn_desc_info_t *desc;
    int idx, pending;
    bkn_evt_resource_t *evt;

    if (CDMA_CH(sinfo, XGS_DMA_TX_CHAN)) {
        return bkn_tx_cdma_chain_done(sinfo, done);
    }

    DBG_IRQ(("Tx chain done (%d/%d)\n", sinfo->tx.cur, sinfo->tx.dirty));

    dev_irq_mask_disable(sinfo, XGS_DMA_TX_CHAN, 0);

    evt = &_bkn_evt[sinfo->evt_idx];

    if (sinfo->tx.api_active) {
        sinfo->dma_events |= KCOM_DMA_INFO_F_TX_DONE;
        evt->evt_wq_put++;
        wake_up_interruptible(&evt->evt_wq);
        /* Check if BCM API has more to send */
        bkn_api_tx(sinfo);
        if (sinfo->tx.api_active) {
            return;
        }
    }

    if (sinfo->tx.free == MAX_TX_DCBS) {
        /* If netif Tx is idle then allow BCM API to send */
        bkn_api_tx(sinfo);
        if (sinfo->tx.api_active) {
            return;
        }
    } else {
        /* If two or more DCBs are pending, chain them */
        pending = MAX_TX_DCBS - sinfo->tx.free;
        idx = sinfo->tx.dirty;
        while (--pending && idx < (MAX_TX_DCBS - 1)) {
            if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                sinfo->tx.desc[idx++].dcb_mem[2] |= 1 << 16;
            } else {
                sinfo->tx.desc[idx++].dcb_mem[1] |= 1 << 16;
            }
            DBG_DCB_TX(("Chain Tx DCB %d (%d)\n", idx, pending));
        }
        /* Restart DMA from where we stopped */
        desc = &sinfo->tx.desc[sinfo->tx.dirty];
        DBG_DCB_TX(("Restart Tx DMA, DCB @ 0x%08x (%d).\n",
                    (uint32_t)desc->dcb_dma, sinfo->tx.dirty));
        dev_dma_chan_clear(sinfo, XGS_DMA_TX_CHAN);
        dev_irq_mask_enable(sinfo, XGS_DMA_TX_CHAN, 0);
        dev_dma_chan_start(sinfo, XGS_DMA_TX_CHAN, desc->dcb_dma);
    }

    /* Resume if netif Tx resources available and API Tx not active */
    bkn_resume_tx(sinfo);
}

static void
bkn_schedule_napi_poll(bkn_switch_info_t *sinfo)
{
    /* Schedule NAPI poll */
    DBG_NAPI(("Schedule NAPI poll on %s.\n", sinfo->dev->name));
    /* Disable interrupts until poll job is complete */
    sinfo->napi_poll_mode = 1;

    /* Disable configuration API while the spinlock is released. */
    sinfo->cfg_api_locked = 1;
    /* Unlock while calling up network stack */
    spin_unlock(&sinfo->lock);
    if (bkn_napi_schedule_prep(sinfo->dev, &sinfo->napi)) {
        __bkn_napi_schedule(sinfo->dev, &sinfo->napi);
        DBG_NAPI(("Schedule prep OK on %s.\n", sinfo->dev->name));
    } else {
        /* Most likely the base device is has not been opened */
        gprintk("Warning: Unable to schedule NAPI - base device not up?\n");
    }
    spin_lock(&sinfo->lock);
    /* Re-enable configuration API once spinlock is regained. */
    sinfo->cfg_api_locked = 0;
}

static void
bkn_napi_poll_complete(bkn_switch_info_t *sinfo)
{
    /* Disable configuration API while the spinlock is released. */
    sinfo->cfg_api_locked = 1;
    /* Unlock while calling up network stack */
    spin_unlock(&sinfo->lock);
    bkn_napi_complete(sinfo->dev, &sinfo->napi);
    spin_lock(&sinfo->lock);
    /* Re-enable configuration API once spinlock is regained. */
    sinfo->cfg_api_locked = 0;
    /* Re-enable interrupts */
    sinfo->napi_poll_mode = 0;
    dev_irq_mask_set(sinfo, sinfo->irq_mask);
}

static int
xgs_do_dma(bkn_switch_info_t *sinfo, int budget)
{
    int rx_dcbs_done = 0, tx_dcbs_done = 0;
    int chan_done, budget_chans = 0;
    uint32_t dma_stat = 0;
    int chan;
    int unet_chans = 0;

    dev_read32(sinfo, CMIC_DMA_STATr, &dma_stat);

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            unet_chans++;
            continue;
        }
        if (dma_stat & DS_DESC_DONE_TST(XGS_DMA_RX_CHAN + chan)) {
            xgs_dma_desc_clear(sinfo, XGS_DMA_RX_CHAN + chan);
            sinfo->poll_channels |= 1 << chan;
        }
    }
    if (!sinfo->poll_channels) {
        sinfo->poll_channels = (uint32_t)(1 << sinfo->rx_chans) - 1;
        sinfo->poll_channels &= ~(sinfo->unet_channels >> 1);
        if (sinfo->rx_chans > unet_chans) {
            budget_chans = budget / (sinfo->rx_chans - unet_chans);
        }
    } else {
        for (chan = 0; chan < sinfo->rx_chans; chan++) {
            if (1 << chan & sinfo->poll_channels) {
                budget_chans++;
            }
        }
        if (budget_chans) {
            budget_chans = budget / budget_chans;
        }
    }

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (1 << chan & sinfo->poll_channels) {
            chan_done = bkn_do_rx(sinfo, chan, budget_chans);
            rx_dcbs_done += chan_done;
            if (chan_done < budget_chans) {
                sinfo->poll_channels &= ~(1 << chan);
            }
            bkn_rx_desc_done(sinfo, chan);
        }

        if (dma_stat & DS_CHAIN_DONE_TST(XGS_DMA_RX_CHAN + chan)) {
            if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
                continue;
            }
            xgs_dma_chain_clear(sinfo, XGS_DMA_RX_CHAN + chan);
            bkn_rx_chain_done(sinfo, chan);
        }
    }

    if (dma_stat & DS_CHAIN_DONE_TST(XGS_DMA_TX_CHAN)) {
        xgs_dma_chain_clear(sinfo, XGS_DMA_TX_CHAN);
        tx_dcbs_done = bkn_do_tx(sinfo);
        bkn_tx_chain_done(sinfo, tx_dcbs_done);
    }

    return sinfo->poll_channels ? budget : rx_dcbs_done;
}

static int
xgsm_do_dma(bkn_switch_info_t *sinfo, int budget)
{
    int rx_dcbs_done = 0, tx_dcbs_done = 0;
    int chan_done, budget_chans = 0;
    uint32_t dma_stat =0, irq_stat = 0;
    int chan;
    int unet_chans = 0;

    /* Get Controlled interrupt states for Continuous DMA mode */
    if (sinfo->cdma_channels) {
        dev_read32(sinfo, CMICM_IRQ_STATr, &irq_stat);
    }

    dev_read32(sinfo, CMICM_DMA_STATr, &dma_stat);

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            unet_chans++;
            continue;
        }
        if (dma_stat & (0x10 << (XGS_DMA_RX_CHAN + chan)) ||
            irq_stat & (0x08000000 << (XGS_DMA_RX_CHAN + chan))) {
            xgsm_dma_desc_clear(sinfo, XGS_DMA_RX_CHAN + chan);
            sinfo->poll_channels |= 1 << chan;
        }
    }
    if (!sinfo->poll_channels) {
        sinfo->poll_channels = (uint32_t)(1 << sinfo->rx_chans) - 1;
        sinfo->poll_channels &= ~(sinfo->unet_channels >> 1);
        if (sinfo->rx_chans > unet_chans) {
            budget_chans = budget / (sinfo->rx_chans - unet_chans);
        }
    } else {
        for (chan = 0; chan < sinfo->rx_chans; chan++) {
            if (1 << chan & sinfo->poll_channels) {
                budget_chans++;
            }
        }
        if (budget_chans) {
            budget_chans = budget / budget_chans;
        }
    }

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (1 << chan & sinfo->poll_channels) {
            chan_done = bkn_do_rx(sinfo, chan, budget_chans);
            rx_dcbs_done += chan_done;
            if (chan_done < budget_chans) {
                sinfo->poll_channels &= ~(1 << chan);
            }
            bkn_rx_desc_done(sinfo, chan);
        }

        if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            continue;
        }

        if (dma_stat & (0x1 << (XGS_DMA_RX_CHAN + chan))) {
            if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
                continue;
            }
            xgsm_dma_chain_clear(sinfo, XGS_DMA_RX_CHAN + chan);
            bkn_rx_chain_done(sinfo, chan);
        }
    }

    if (dma_stat & (0x1 << XGS_DMA_TX_CHAN) ||
        irq_stat & (0x08000000 << XGS_DMA_TX_CHAN)) {
        if (CDMA_CH(sinfo, XGS_DMA_TX_CHAN)) {
            xgsm_dma_desc_clear(sinfo, XGS_DMA_TX_CHAN);
        } else {
            xgsm_dma_chain_clear(sinfo, XGS_DMA_TX_CHAN);
        }
        tx_dcbs_done = bkn_do_tx(sinfo);
        bkn_tx_chain_done(sinfo, tx_dcbs_done);
    }

    return sinfo->poll_channels ? budget : rx_dcbs_done;
}

static int
xgsx_do_dma(bkn_switch_info_t *sinfo, int budget)
{
    int rx_dcbs_done = 0, tx_dcbs_done = 0;
    int chan_done, budget_chans = 0;
    uint32_t irq_stat = 0;
    uint32_t tx_dma_stat = 0;
    uint32_t rx_dma_stat[NUM_CMICX_RX_CHAN] = {0};
    int chan;
    int unet_chans = 0;

    dev_read32(sinfo, CMICX_IRQ_STATr, &irq_stat);
    dev_read32(sinfo, CMICX_DMA_STATr + 0x80 * XGS_DMA_TX_CHAN, &tx_dma_stat);

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            unet_chans++;
            continue;
        }
        dev_read32(sinfo,
                   CMICX_DMA_STATr + 0x80 * (XGS_DMA_RX_CHAN + chan),
                   &rx_dma_stat[chan]);
    }

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            continue;
        }
        if ((irq_stat & CMICX_DS_CMC_CTRLD_INT(XGS_DMA_RX_CHAN + chan)) ||
            (irq_stat & CMICX_DS_CMC_DESC_DONE(XGS_DMA_RX_CHAN + chan))) {
            xgsx_dma_desc_clear(sinfo, XGS_DMA_RX_CHAN + chan);
            sinfo->poll_channels |= 1 << chan;
        }
    }
    if (!sinfo->poll_channels) {
        sinfo->poll_channels = (uint32_t)(1 << sinfo->rx_chans) - 1;
        sinfo->poll_channels &= ~(sinfo->unet_channels >> 1);
        if (sinfo->rx_chans > unet_chans) {
            budget_chans = budget / (sinfo->rx_chans - unet_chans);
        }
    } else {
        for (chan = 0; chan < sinfo->rx_chans; chan++) {
            if (1 << chan & sinfo->poll_channels) {
                budget_chans++;
            }
        }
        if (budget_chans) {
            budget_chans = budget / budget_chans;
        }
    }

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (1 << chan & sinfo->poll_channels) {
            chan_done = bkn_do_rx(sinfo, chan, budget_chans);
            rx_dcbs_done += chan_done;
            if (chan_done < budget_chans) {
                sinfo->poll_channels &= ~(1 << chan);
            }
            bkn_rx_desc_done(sinfo, chan);
        }

        if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            continue;
        }

        if (rx_dma_stat[chan] & CMICX_DS_CMC_DMA_CHAIN_DONE) {
            if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
                continue;
            }
            xgsx_dma_chain_clear(sinfo, XGS_DMA_RX_CHAN + chan);
            bkn_rx_chain_done(sinfo, chan);
        }
    }

    if ((irq_stat & CMICX_DS_CMC_CTRLD_INT(XGS_DMA_TX_CHAN)) ||
        (tx_dma_stat & CMICX_DS_CMC_DMA_CHAIN_DONE)) {
        if (CDMA_CH(sinfo, XGS_DMA_TX_CHAN)) {
            xgsx_dma_desc_clear(sinfo, XGS_DMA_TX_CHAN);
        } else {
            xgsx_dma_chain_clear(sinfo, XGS_DMA_TX_CHAN);
        }
        tx_dcbs_done = bkn_do_tx(sinfo);
        bkn_tx_chain_done(sinfo, tx_dcbs_done);
    }

    return sinfo->poll_channels ? budget : rx_dcbs_done;
}

static int
xgsr_do_dma(bkn_switch_info_t *sinfo, int budget)
{
    int rx_dcbs_done = 0, tx_dcbs_done = 0;
    int chan_done, budget_chans = 0;
    uint32_t tx_dma_stat = 0;
    uint32_t rx_dma_stat[NUM_CMICR_RX_CHAN] = {0};
    int chan;
    int unet_chans = 0;

    dev_read32(sinfo, CMICR_INTR_STATr + 0x80 * XGS_DMA_TX_CHAN, &tx_dma_stat);
    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            unet_chans++;
            continue;
        }
        dev_read32(sinfo,
                   CMICR_INTR_STATr + 0x80 * (XGS_DMA_RX_CHAN + chan),
                   &rx_dma_stat[chan]);
    }

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            continue;
        }
        if ((rx_dma_stat[chan] & CMICR_DS_DESC_CONTROLLED_INTR) ||
            (rx_dma_stat[chan] & CMICR_DS_DESC_DONE_INTR)) {
            xgsr_dma_desc_clear(sinfo, XGS_DMA_RX_CHAN + chan);
            sinfo->poll_channels |= 1 << chan;
        }
    }
    if (!sinfo->poll_channels) {
        sinfo->poll_channels = (uint32_t)(1 << sinfo->rx_chans) - 1;
        sinfo->poll_channels &= ~(sinfo->unet_channels >> 1);
        if (sinfo->rx_chans > unet_chans) {
            budget_chans = budget / (sinfo->rx_chans - unet_chans);
        }
    } else {
        for (chan = 0; chan < sinfo->rx_chans; chan++) {
            if (1 << chan & sinfo->poll_channels) {
                budget_chans++;
            }
        }
        if (budget_chans) {
            budget_chans = budget / budget_chans;
        }
    }

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (1 << chan & sinfo->poll_channels) {
            chan_done = bkn_do_rx(sinfo, chan, budget_chans);
            rx_dcbs_done += chan_done;
            if (chan_done < budget_chans) {
                sinfo->poll_channels &= ~(1 << chan);
            }
            bkn_rx_desc_done(sinfo, chan);
        }

        if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            continue;
        }

        if (rx_dma_stat[chan] & CMICR_DS_CMC_DMA_CHAIN_DONE) {
            if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
                continue;
            }
            xgsr_dma_chain_clear(sinfo, XGS_DMA_RX_CHAN + chan);
            bkn_rx_chain_done(sinfo, chan);
        }
    }

    if ((tx_dma_stat & CMICR_DS_DESC_CONTROLLED_INTR) ||
        (tx_dma_stat & CMICR_DS_DESC_DONE_INTR)) {
        if (CDMA_CH(sinfo, XGS_DMA_TX_CHAN)) {
            xgsr_dma_desc_clear(sinfo, XGS_DMA_TX_CHAN);
        } else {
            xgsr_dma_chain_clear(sinfo, XGS_DMA_TX_CHAN);
        }
        tx_dcbs_done = bkn_do_tx(sinfo);
        bkn_tx_chain_done(sinfo, tx_dcbs_done);
    }

    return sinfo->poll_channels ? budget : rx_dcbs_done;
}

static int
dev_do_dma(bkn_switch_info_t *sinfo, int budget)
{
    if (DEV_IS_CMICR(sinfo)) {
        return xgsr_do_dma(sinfo, budget);
    } else if (DEV_IS_CMICX(sinfo)) {
        return xgsx_do_dma(sinfo, budget);
    } else if (DEV_IS_CMICM(sinfo)) {
        return xgsm_do_dma(sinfo, budget);
    } else {
        return xgs_do_dma(sinfo, budget);
    }
}

static void
xgs_isr(bkn_switch_info_t *sinfo)
{
    uint32_t irq_stat = 0;
    int rx_dcbs_done;

    dev_read32(sinfo, CMIC_IRQ_STATr, &irq_stat);
    if ((irq_stat & sinfo->irq_mask) == 0) {
        /* Not ours */
        return;
    }
    sinfo->interrupts++;

    DBG_IRQ(("Got interrupt on device %d (0x%08x)\n",
             sinfo->dev_no, irq_stat));

    if (use_napi) {
        bkn_schedule_napi_poll(sinfo);
    } else {
        xgs_irq_mask_set(sinfo, 0);
        do {
            rx_dcbs_done = xgs_do_dma(sinfo, MAX_RX_DCBS);
        } while (rx_dcbs_done);
    }

    xgs_irq_mask_set(sinfo, sinfo->irq_mask);
}

static void
xgsm_isr(bkn_switch_info_t *sinfo)
{
    uint32_t irq_stat = 0;
    int rx_dcbs_done;

    dev_read32(sinfo, CMICM_IRQ_STATr, &irq_stat);
    if ((irq_stat & sinfo->irq_mask) == 0) {
        /* Not ours */
        return;
    }
    sinfo->interrupts++;

    DBG_IRQ(("Got interrupt on device %d (0x%08x)\n",
             sinfo->dev_no, irq_stat));

    if (use_napi) {
        bkn_schedule_napi_poll(sinfo);
    } else {
        xgsm_irq_mask_set(sinfo, 0);
        do {
            rx_dcbs_done = xgsm_do_dma(sinfo, MAX_RX_DCBS);
            if (sinfo->cdma_channels) {
                if (rx_dcbs_done >= MAX_RX_DCBS || sinfo->tx_yield) {
                    /* Continuous DMA mode requires to yield timely */
                    break;
                }
            }
        } while (rx_dcbs_done);
    }

    xgsm_irq_mask_set(sinfo, sinfo->irq_mask);
}

static void
xgsx_isr(bkn_switch_info_t *sinfo)
{
    uint32_t irq_stat = 0;
    int rx_dcbs_done;
    uint32_t ctrl = 0;
    int chan = 0;

    dev_read32(sinfo, CMICX_IRQ_STATr, &irq_stat);
    if ((irq_stat & sinfo->irq_mask) == 0) {
        /* Not ours */
        return;
    }

    /* Bypass chain_done from Abort */
    for (chan = 0; chan < NUM_CMICX_DMA_CHAN; chan++) {
        if (irq_stat & CMICX_DS_CMC_CHAIN_DONE(chan)) {
            dev_read32(sinfo, CMICX_DMA_CTRLr + 0x80 * chan, &ctrl);
            if (ctrl & CMICX_DC_CMC_ABORT) {
                DBG_IRQ(("chain %d: chain done for Abort\n", chan));
                return;
            }
        }
    }

    sinfo->interrupts++;

    DBG_IRQ(("Got interrupt on device %d (0x%08x)\n",
             sinfo->dev_no, irq_stat));

    if (use_napi) {
        bkn_schedule_napi_poll(sinfo);
    } else {
        xgsx_irq_mask_set(sinfo, 0);
        do {
            rx_dcbs_done = xgsx_do_dma(sinfo, MAX_RX_DCBS);
            if (sinfo->cdma_channels) {
                if (rx_dcbs_done >= MAX_RX_DCBS || sinfo->tx_yield) {
                    /* Continuous DMA mode requires to yield timely */
                    break;
                }
            }
        } while (rx_dcbs_done);
    }

    xgsx_irq_mask_set(sinfo, sinfo->irq_mask);
}

static void
xgsr_isr(bkn_switch_info_t *sinfo)
{
    uint32_t irq_stat = 0;
    int rx_dcbs_done;

    lkbde_irq_status_get(sinfo->dev_no | LKBDE_IPROC_REG, PAXB_0_INTC_INTR_RAW_STATUS_REG5r, &irq_stat);

    if ((irq_stat & sinfo->irq_mask) == 0) {
        /* Not ours */
        return;
    }

    /* Bypass chain_done from Abort */
    if (device_is_dnx(sinfo)) {
        uint32_t ctrl = 0;
        uint32_t intr_stat = 0;
        int chan = 0;
        for (chan = 0; chan < NUM_CMICR_DMA_CHAN; chan++) {
            dev_read32(sinfo, CMICR_INTR_STATr + 0x80 * chan, &intr_stat);
            if (intr_stat & CMICR_DS_CHAIN_DONE_INTR) {
                dev_read32(sinfo, CMICR_DMA_CTRLr + 0x80 * chan, &ctrl);
                if (ctrl & CMICR_DC_CMC_ABORT) {
                    DBG_IRQ(("chain %d: chain done for Abort\n", chan));
                    return;
                }
            }
        }
    }

    sinfo->interrupts++;

    DBG_IRQ(("Got interrupt on device %d (0x%08x)\n",
             sinfo->dev_no, irq_stat));

    if (use_napi) {
        bkn_schedule_napi_poll(sinfo);
    } else {
        xgsr_irq_mask_set(sinfo, 0);
        do {
            rx_dcbs_done = xgsr_do_dma(sinfo, MAX_RX_DCBS);
            if (sinfo->cdma_channels) {
                if (rx_dcbs_done >= MAX_RX_DCBS || sinfo->tx_yield) {
                    /* Continuous DMA mode requires to yield timely */
                    break;
                }
            }
        } while (rx_dcbs_done);
    }

    xgsr_irq_mask_set(sinfo, sinfo->irq_mask);
}

static void
bkn_isr(void *isr_data)
{
    bkn_switch_info_t *sinfo = isr_data;

    /* Safe exit on SMP systems */
    if (!module_initialized) {
        return;
    }

    /* Ensure that we do not touch registers during device reset */
    if (sinfo->irq_mask == 0) {
        /* Not ours */
        return;
    }

    spin_lock(&sinfo->lock);

    if (sinfo->napi_poll_mode) {
        /* Not ours */
        spin_unlock(&sinfo->lock);
        return;
    }

    if (DEV_IS_CMICR(sinfo)) {
        xgsr_isr(sinfo);
    } else if (DEV_IS_CMICX(sinfo)) {
        xgsx_isr(sinfo);
    } else if (DEV_IS_CMICM(sinfo)) {
        xgsm_isr(sinfo);
    } else {
        xgs_isr(sinfo);
    }

    spin_unlock(&sinfo->lock);
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void
bkn_poll_controller(struct net_device *dev)
{
    bkn_priv_t *priv = netdev_priv(dev);
    struct list_head *list;
    bkn_switch_info_t *sinfo;

    disable_irq(dev->irq);
    list_for_each(list, &_sinfo_list) {
        sinfo = (bkn_switch_info_t *)list;
        if (sinfo->ndevs && sinfo->ndev_max > 0 && sinfo->ndev_max > priv->id) {
            if (sinfo->ndevs[priv->id] && sinfo->ndevs[priv->id] == dev) {
                bkn_isr(sinfo);
            }
        }
    }
    enable_irq(dev->irq);
}
#endif

static void
bkn_resume_rx(bkn_switch_info_t *sinfo)
{
    bkn_desc_info_t *desc;
    bkn_dcb_chain_t *dcb_chain;
    uint64_t cur_halt, last_dcb, dcb_dma;
    int woffset, chan, cdma_running;

    /* Resume Rx DMA on all channels */
    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            continue;
        }
        if (sinfo->rx[chan].use_rx_skb) {
            cdma_running = 0;
            bkn_api_rx_restart(sinfo, chan);
            if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
                cur_halt = sinfo->halt_addr[XGS_DMA_RX_CHAN + chan];
                last_dcb = sinfo->rx[chan].desc[MAX_RX_DCBS].dcb_dma;
                if (cur_halt != last_dcb) {
                    desc = &sinfo->rx[chan].desc[sinfo->rx[chan].dirty + 1];
                    bkn_cdma_goto(sinfo, XGS_DMA_RX_CHAN + chan, desc->dcb_dma);
                    cdma_running = 1;
                }
            }
            if (!cdma_running) {
                bkn_rx_restart(sinfo, chan);
            }
        } else {
            cdma_running = 0;
            if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
                if (sinfo->rx[chan].api_active) {
                    dcb_chain = sinfo->rx[chan].api_dcb_chain_end;
                    woffset = (dcb_chain->dcb_cnt - 1) * sinfo->dcb_wsize;
                    dcb_dma = dcb_chain->dcb_dma + woffset * sizeof(uint32_t);
                    bkn_cdma_goto(sinfo, XGS_DMA_RX_CHAN + chan, dcb_dma);
                    cdma_running = 1;
                }
            }
            if (!cdma_running) {
                bkn_api_rx_restart(sinfo, chan);
            }
        }
    }
}

static int
bkn_open(struct net_device *dev)
{
    bkn_priv_t *priv = netdev_priv(dev);
    bkn_switch_info_t *sinfo = priv->sinfo;
    unsigned long flags;

    /* Check if base device */
    if (priv->id <= 0) {
        /* NAPI used only on base device */
        if (use_napi) {
            bkn_napi_enable(dev, &sinfo->napi);
        }

        /* Start DMA when base device is started */
        if (sinfo->basedev_suspended) {
            spin_lock_irqsave(&sinfo->lock, flags);
            dev_do_dma(sinfo, MAX_RX_DCBS);
            sinfo->basedev_suspended = 0;
            bkn_api_tx(sinfo);
            if (!sinfo->tx.api_active) {
                bkn_resume_tx(sinfo);
            }
            bkn_resume_rx(sinfo);
            spin_unlock_irqrestore(&sinfo->lock, flags);
        }
    }

    if (!sinfo->basedev_suspended) {
        netif_start_queue(dev);
    }

    return 0;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29))
static int
bkn_set_mac_address(struct net_device *dev, void *addr)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,18,12))
    if (!is_valid_ether_addr((const u8*)(((struct sockaddr *)addr)->sa_data))) {
#else
    if (!is_valid_ether_addr(((struct sockaddr *)addr)->sa_data)) {
#endif
        return -EINVAL;
    }
    eth_hw_addr_set(dev, ((struct sockaddr *)addr)->sa_data);
    return 0;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29))
static int
bkn_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
    bkn_priv_t *priv;
    bkn_switch_info_t *sinfo;
    struct hwtstamp_config config;

    DBG_PTP(("bkn_ioctl: module_initialized:%d\n", module_initialized));

    if (!module_initialized) {
        return -EINVAL;
    }

    if (!dev)
        return -EINVAL;

    priv = netdev_priv(dev);

    if (!priv)
        return -EINVAL;

    sinfo = priv->sinfo;

    if (cmd == SIOCSHWTSTAMP) {
        if (copy_from_user(&config, ifr->ifr_data, sizeof(config))) {
            return -EFAULT;
        }

        if (!knet_hw_tstamp_enable_cb || !knet_hw_tstamp_disable_cb) {
            return -ENOSYS;
        }

        switch (config.tx_type) {
        case HWTSTAMP_TX_OFF:
            knet_hw_tstamp_disable_cb(sinfo->dev_no, priv->phys_port, config.tx_type);
            priv->tx_hwts = (config.tx_type);
            break;
        case HWTSTAMP_TX_ON:
            knet_hw_tstamp_enable_cb(sinfo->dev_no, priv->phys_port, config.tx_type);
            priv->tx_hwts = (config.tx_type);
            break;
        case HWTSTAMP_TX_ONESTEP_SYNC:
            knet_hw_tstamp_enable_cb(sinfo->dev_no, priv->phys_port, config.tx_type);
            priv->tx_hwts = (config.tx_type);
            break;
        default:
            return -ERANGE;
        }

        switch (config.rx_filter) {
        case HWTSTAMP_FILTER_NONE:
            if (priv->rx_hwts) {
                knet_hw_tstamp_disable_cb(sinfo->dev_no, priv->phys_port, config.tx_type);
                priv->rx_hwts = 0;
            }
            break;
        default:
            if (!priv->rx_hwts) {
                knet_hw_tstamp_enable_cb(sinfo->dev_no, priv->phys_port, config.tx_type);
                priv->rx_hwts = 1;
            }
            config.rx_filter = HWTSTAMP_FILTER_ALL;
            break;
        }

        return copy_to_user(ifr->ifr_data, &config, sizeof(config)) ? -EFAULT : 0;
    }

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,14,0))
    if (cmd == SIOCGHWTSTAMP) {
        config.flags = 0;
        config.tx_type = priv->tx_hwts;
        config.rx_filter = priv->rx_hwts ? HWTSTAMP_FILTER_ALL : HWTSTAMP_FILTER_NONE;

        return copy_to_user(ifr->ifr_data, &config, sizeof(config)) ? -EFAULT : 0;
    }
#endif

    return -EINVAL;
}
#endif

static int
bkn_change_mtu(struct net_device *dev, int new_mtu)
{
    int max_size = new_mtu + ETH_HLEN + VLAN_HLEN + 4;

    if (new_mtu < 68 || max_size > rx_buffer_size) {
        return -EINVAL;
    }
    dev->mtu = new_mtu;
    return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24)
static int
bkn_poll(struct net_device *dev, int *budget)
{
    bkn_priv_t *priv = netdev_priv(dev);
    bkn_switch_info_t *sinfo = priv->sinfo;
    int cur_budget = *budget;
    int poll_again = 0;
    int rx_dcbs_done;
    unsigned long flags;

    spin_lock_irqsave(&sinfo->lock, flags);

    DBG_NAPI(("NAPI poll on %s.\n", dev->name));

    sinfo->napi_poll_again = 0;

    if (cur_budget > dev->quota) {
        cur_budget = dev->quota;
    }

    rx_dcbs_done = dev_do_dma(sinfo, cur_budget);

    *budget -= rx_dcbs_done;
    cur_budget -= rx_dcbs_done;
    dev->quota -= rx_dcbs_done;

    if (sinfo->napi_poll_again || cur_budget <= 0) {
        poll_again = 1;
        sinfo->napi_not_done++;
    } else {
        bkn_napi_poll_complete(sinfo);
    }

    spin_unlock_irqrestore(&sinfo->lock, flags);

    return poll_again;
}
#else
static int
bkn_poll(struct napi_struct *napi, int budget)
{
    bkn_switch_info_t *sinfo = container_of(napi, bkn_switch_info_t, napi);
    int rx_dcbs_done;
    unsigned long flags;

    spin_lock_irqsave(&sinfo->lock, flags);

    DBG_NAPI(("NAPI poll on %s.\n", sinfo->dev->name));

    sinfo->napi_poll_again = 0;

    rx_dcbs_done = dev_do_dma(sinfo, budget);

    if (sinfo->napi_poll_again || rx_dcbs_done >= budget) {
        /* Force poll again */
        rx_dcbs_done = budget;
        sinfo->napi_not_done++;
    } else {
        bkn_napi_poll_complete(sinfo);
    }

    spin_unlock_irqrestore(&sinfo->lock, flags);

    return rx_dcbs_done;
}
#endif

static int
bkn_stop(struct net_device *dev)
{
    bkn_priv_t *priv = netdev_priv(dev);
    bkn_switch_info_t *sinfo = priv->sinfo;
    unsigned long flags;

    netif_stop_queue(dev);

    /* Check if base device */
    if (priv->id <= 0) {
        /* NAPI used only on base device */
        if (use_napi) {
            bkn_napi_disable(dev, &sinfo->napi);
        }
        /* Suspend all devices if base device is stopped */
        if (basedev_suspend) {
            spin_lock_irqsave(&sinfo->lock, flags);
            bkn_suspend_tx(sinfo);
            sinfo->basedev_suspended = 1;
            spin_unlock_irqrestore(&sinfo->lock, flags);
        }
    }

    return 0;
}

/*
 * Network Device Statistics.
 * Cleared at init time.
 */
static struct net_device_stats *
bkn_get_stats(struct net_device *dev)
{
    bkn_priv_t *priv = netdev_priv(dev);

    return &priv->stats;
}

/* Fake multicast ability */
static void
bkn_set_multicast_list(struct net_device *dev)
{
}

static int
bkn_hw_tstamp_tx_config(bkn_switch_info_t *sinfo,
                        int hwts, int hdrlen,
                        struct sk_buff *skb, u32 *meta)
{
    uint32_t *md = NULL;

    if (!knet_hw_tstamp_tx_meta_get_cb) {
        return -1;
    }

    KNET_SKB_CB(skb)->dcb_type = sinfo->dcb_type & 0xFFFF;
    knet_hw_tstamp_tx_meta_get_cb(sinfo->dev_no, hwts, hdrlen, skb,
                                  &(KNET_SKB_CB(skb)->ts), (meta ? &md : NULL));

    if (!meta) {
        return 0;
    }

    if (!md && !device_is_dnx(sinfo)) {
        return -1;
    }

    switch (sinfo->dcb_type) {
    case 28: /* dpp */
    case 39: /* DNX - Q2A, J2C */
        break;
    case 26:
    case 32:
    case 33:
    case 35:
    case 37:
        meta[2] |= md[0];
        meta[3] |= md[1];
        meta[4] |= md[2];
        meta[5] |= md[3];
        break;
    case 36:
    case 38:
        meta[0] |= htonl(md[0]);
        meta[1] |= htonl(md[1]);
        meta[2] |= htonl(md[2]);
        meta[3] |= htonl(md[3]);
        break;
    default:
        return -1;
    }

    return 0;
}

static int
bkn_tx(struct sk_buff *skb, struct net_device *dev)
{
    bkn_priv_t *priv = netdev_priv(dev);
    bkn_switch_info_t *sinfo = priv->sinfo;
    struct sk_buff *new_skb = NULL;
    unsigned char *pktdata;
    int pktlen, hdrlen, taglen, rcpulen, metalen;
    int sop, idx;
    uint16_t tpid;
    uint32_t *metadata;
    unsigned long flags;
    uint8_t cpu_channel = 0;
    int headroom, tailroom;
    int ptp_transport = 0;

    DBG_VERB(("Netif Tx(%s): Len=%d priv->id=%d\n", dev->name, skb->len, priv->id));

    if (priv->id <= 0) {
        /* Do not transmit on base device */
        priv->stats.tx_dropped++;
        dev_kfree_skb_any(skb);
        return 0;
    }

    if (device_is_dnx(sinfo) && (skb->len == 0)) {
        priv->stats.tx_dropped++;
        dev_kfree_skb_any(skb);
        return 0;
    }

    if (!bkn_netif_ok(dev)) {
        DBG_WARN(("Tx drop: Netif link is down.\n"));
        priv->stats.tx_dropped++;
        sinfo->tx.pkts_d_no_link++;
        dev_kfree_skb_any(skb);
        return 0;
    }

    spin_lock_irqsave(&sinfo->lock, flags);

    if (sinfo->tx.free > 1) {
        bkn_desc_info_t *desc = &sinfo->tx.desc[sinfo->tx.cur];
        uint32_t *dcb, *meta;

        pktdata = skb->data;
        pktlen = skb->len;

        if (device_is_sand(sinfo)) {
            hdrlen = priv->system_headers_size;

            /* Account for extra OAM-TS header. */
            if ((bkn_skb_tx_flags(skb) & SKBTX_HW_TSTAMP) &&
                (hdrlen > (BKN_DNX_PTCH_2_SIZE))) {
                if (device_is_dnx(sinfo)) {
                    if (knet_hw_tstamp_ptp_transport_get_cb != NULL) {
                        ptp_transport = knet_hw_tstamp_ptp_transport_get_cb(skb->data);
                        switch(ptp_transport) {
                            case 6: /* UDP IPv6   */
                                /* T_LOCAL_PORT intf will use
                                    Module Hdr + PTCH + ITMH + ASE1588 + TSH + INTERNAL_HDR + UDH base
                                */
                                hdrlen += (BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE + BKN_DNX_TSH_SIZE +
                                            BKN_DNX_INTERNAL_BASE_TYPE_12 + BKN_DNX_UDH_BASE_SIZE);
                                break;
                            case 4: /* UDP IPv4   */
                            case 2: /* IEEE 802.3 */
                            default:
                                /* T_LOCAL_PORT intf will use Module Hdr + PTCH + ITMH + ASE1588 + TSH */
                                hdrlen += (BKN_DNX_FTMH_APP_SPECIFIC_EXT_SIZE + BKN_DNX_TSH_SIZE);
                                break;
                        }
                    }
                } else {
                    hdrlen += BKN_DPP_OTSH_SIZE_BYTE;
                }
            }

        }
        else {
            hdrlen = ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) ? PKT_TX_HDR_SIZE : 0;
        }
        rcpulen = 0;
        sop = 0;

        if (priv->flags & KCOM_NETIF_F_RCPU_ENCAP) {
            rcpulen = RCPU_HDR_SIZE;
            if (skb->len < (rcpulen + 14)) {
                DBG_WARN(("Tx drop: Invalid RCPU encapsulation\n"));
                priv->stats.tx_dropped++;
                sinfo->tx.pkts_d_rcpu_encap++;
                dev_kfree_skb_any(skb);
                spin_unlock_irqrestore(&sinfo->lock, flags);
                return 0;
            }
            if (check_rcpu_signature &&
                PKT_U16_GET(skb->data, 18) != sinfo->rcpu_sig) {
                DBG_WARN(("Tx drop: Invalid RCPU signature\n"));
                priv->stats.tx_dropped++;
                sinfo->tx.pkts_d_rcpu_sig++;
                dev_kfree_skb_any(skb);
                spin_unlock_irqrestore(&sinfo->lock, flags);
                return 0;
            }

            if (device_is_sand(sinfo)) {
                /* Dune devices don't use meta data */
                sop = 0;
                /* Get CPU channel from RCPU.cpu_channel */
                cpu_channel =  skb->data[29];
                /* System headers are supposed to be set by users in RCPU mode. */
                hdrlen = 0;
            } else if (skb->data[21] & RCPU_F_MODHDR) {
                sop = skb->data[RCPU_HDR_SIZE];
                switch (sop) {
                case 0xff:
                case 0x81:
                case 0xfb:
                case 0xfc:
                    break;
                default:
                    DBG_WARN(("Tx drop: Invalid RCPU meta data\n"));
                    priv->stats.tx_dropped++;
                    sinfo->tx.pkts_d_rcpu_meta++;
                    dev_kfree_skb_any(skb);
                    spin_unlock_irqrestore(&sinfo->lock, flags);
                    return 0;
                }
                if (sinfo->cmic_type != 'x') {
                    if (skb->len < (rcpulen + RCPU_TX_META_SIZE + 14)) {
                        DBG_WARN(("Tx drop: Invalid RCPU encapsulation\n"));
                        priv->stats.tx_dropped++;
                        sinfo->tx.pkts_d_rcpu_encap++;
                        dev_kfree_skb_any(skb);
                        spin_unlock_irqrestore(&sinfo->lock, flags);
                        return 0;
                    }
                    rcpulen += RCPU_TX_META_SIZE;
                }
            }
            /* Skip over RCPU encapsulation */
            pktdata = &skb->data[rcpulen];
            pktlen -= rcpulen;

            /* CPU packets require tag */
            if (sop == 0) {
                if (device_is_sand(sinfo)) {
                    /*
                     * There should be Module Header + PTCH_2 + [ITMH] on JR2,
                     * PTCH_2 +[ITMH] on JR1
                     */
                } else {
                    hdrlen = 0;
                    tpid = PKT_U16_GET(pktdata, 12);
                    if (tpid != 0x8100) {
                        if (skb_header_cloned(skb)) {
                            /* Current SKB cannot be modified */
                            DBG_SKB(("Realloc Tx SKB\n"));
                            /*
                             * New SKB needs extra TAG_SZ for VLAN tag
                             * and extra FCS_SZ for Ethernet FCS.
                             */
                            headroom = TAG_SZ;
                            tailroom = FCS_SZ;
                            new_skb = skb_copy_expand(skb,
                                                      headroom + skb_headroom(skb),
                                                      tailroom,
                                                      GFP_ATOMIC);
                            if (new_skb == NULL) {
                                DBG_WARN(("Tx drop: No SKB memory\n"));
                                priv->stats.tx_dropped++;
                                sinfo->tx.pkts_d_no_skb++;
                                dev_kfree_skb_any(skb);
                                spin_unlock_irqrestore(&sinfo->lock, flags);
                                return 0;
                            }
                            /* Remove rcpulen from buffer. */
                            skb_pull(new_skb, rcpulen);
                            /* Extended by TAG_SZ at the start of buffer. */
                            skb_push(new_skb, TAG_SZ);
                            /* Restore the data before the tag. */
                            memcpy(new_skb->data, pktdata, 12);
                            bkn_skb_tstamp_copy(new_skb, skb);
                            dev_kfree_skb_any(skb);
                            skb = new_skb;
                            pktdata = skb->data;
                            rcpulen = 0;
                        } else {
                            /* Add tag to RCPU header space */
                            DBG_SKB(("Expand into unused RCPU header\n"));
                            rcpulen -= TAG_SZ;
                            pktdata = &skb->data[rcpulen];
                            for (idx = 0; idx < 12; idx++) {
                                pktdata[idx] = pktdata[idx + TAG_SZ];
                            }
                        }
                        pktdata[12] = 0x81;
                        pktdata[13] = 0x00;
                        pktdata[14] = (priv->vlan >> 8) & 0xf;
                        pktdata[15] = priv->vlan & 0xff;
                        pktlen += TAG_SZ;
                    }
                }
            }
        } else {
            if ((((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) && (priv->port >= 0))
                    || device_is_sand(sinfo)) {
                if (skb_header_cloned(skb) || skb_headroom(skb) < hdrlen + 4) {
                    /* Current SKB cannot be modified */
                    DBG_SKB(("Realloc Tx SKB\n"));
                    if (device_is_sand(sinfo)) {
                        headroom = hdrlen;
                    } else {
                        headroom = hdrlen + 4;
                    }
                    tailroom = FCS_SZ;
                    new_skb = skb_copy_expand(skb,
                                              headroom + skb_headroom(skb),
                                              tailroom,
                                              GFP_ATOMIC);
                    if (new_skb == NULL) {
                        DBG_WARN(("Tx drop: No SKB memory\n"));
                        priv->stats.tx_dropped++;
                        sinfo->tx.pkts_d_no_skb++;
                        dev_kfree_skb_any(skb);
                        spin_unlock_irqrestore(&sinfo->lock, flags);
                        return 0;
                    }
                    skb_push(new_skb, hdrlen);
                    bkn_skb_tstamp_copy(new_skb, skb);
                    dev_kfree_skb_any(skb);
                    skb = new_skb;
                } else {
                    DBG_SKB(("Expand Tx SKB\n"));
                    skb_push(skb, hdrlen);
                }
                memset(skb->data, 0, hdrlen);
                pktdata = skb->data;
                pktlen += hdrlen;
            } else {
                hdrlen = 0;
            }

            if (priv->port < 0 || (priv->flags & KCOM_NETIF_F_ADD_TAG)) {
                DBG_DUNE(("ADD VLAN TAG\n"));
                /* Need to add VLAN tag if packet is untagged */
                tpid = PKT_U16_GET(skb->data, hdrlen + 12);
                if (tpid != 0x8100) {
                    if (skb_header_cloned(skb) || skb_headroom(skb) < 4) {
                        /* Current SKB cannot be modified */
                        DBG_SKB(("Realloc Tx SKB\n"));
                        headroom = TAG_SZ;
                        tailroom = FCS_SZ;
                        new_skb = skb_copy_expand(skb,
                                                  headroom + skb_headroom(skb),
                                                  tailroom,
                                                  GFP_ATOMIC);
                        if (new_skb == NULL) {
                            DBG_WARN(("Tx drop: No SKB memory\n"));
                            priv->stats.tx_dropped++;
                            sinfo->tx.pkts_d_no_skb++;
                            dev_kfree_skb_any(skb);
                            spin_unlock_irqrestore(&sinfo->lock, flags);
                            return 0;
                        }
                        skb_push(new_skb, TAG_SZ);
                        memcpy(new_skb->data, pktdata, hdrlen + 12);
                        bkn_skb_tstamp_copy(new_skb, skb);
                        dev_kfree_skb_any(skb);
                        skb = new_skb;
                    } else {
                        /* Add tag to existing buffer */
                        DBG_SKB(("Expand Tx SKB\n"));
                        skb_push(skb, TAG_SZ);
                        for (idx = 0; idx < hdrlen + 12; idx++) {
                            skb->data[idx] = skb->data[idx + TAG_SZ];
                        }
                    }
                    pktdata = skb->data;
                    pktdata[hdrlen + 12] = 0x81;
                    pktdata[hdrlen + 13] = 0x00;
                    pktdata[hdrlen + 14] = (priv->vlan >> 8) & 0xf;
                    pktdata[hdrlen + 15] = priv->vlan & 0xff;
                    pktlen += TAG_SZ;
                }
            }
        }

        /* Pad packet if needed */
        taglen = 0;
        tpid = PKT_U16_GET(pktdata, hdrlen + 12);
        if (tpid == 0x8100) {
            taglen = 4;
        }
        if (pktlen < (60 + taglen + hdrlen)) {
            pktlen = (60 + taglen + hdrlen);
            if (SKB_PADTO(skb, pktlen) != 0) {
                DBG_WARN(("Tx drop: skb_padto failed\n"));
                priv->stats.tx_dropped++;
                sinfo->tx.pkts_d_pad_fail++;
                dev_kfree_skb_any(skb);
                spin_unlock_irqrestore(&sinfo->lock, flags);
                return 0;
            }
            /* skb_padto may update the skb->data pointer */
            pktdata = &skb->data[rcpulen];
        }

        if ((pktlen + FCS_SZ) > SOC_DCB_KNET_COUNT_MASK) {
            DBG_WARN(("Tx drop: size of pkt (%d) is out of range(%d)\n",
                     (pktlen + FCS_SZ), SOC_DCB_KNET_COUNT_MASK));
            sinfo->tx.pkts_d_over_limit++;
            priv->stats.tx_dropped++;
            dev_kfree_skb_any(skb);
            spin_unlock_irqrestore(&sinfo->lock, flags);
            return 0;
        }

        dcb = desc->dcb_mem;
        meta = ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) ? (uint32_t *)pktdata : dcb;
        memset(dcb, 0, sinfo->dcb_wsize * sizeof(uint32_t));
        if (priv->flags & KCOM_NETIF_F_RCPU_ENCAP) {
            if (device_is_sand(sinfo)) {
                if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                    dcb[2] |= 1 << 19;
                    /* Given Module Header exists and set first byte to be CPU channel  */
                    pktdata[0] = cpu_channel;
                } else {
                    dcb[1] |= 1 << 19;
                    /* Set CPU channel */
                    dcb[2] = (cpu_channel & 0xff) << 24;
                }

            } else if (sop != 0) {
                /* If module header SOP is non-zero, use RCPU meta data */
                if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                    dcb[2] |= 1 << 19;
                } else {
                    metalen = (sinfo->dcb_wsize - 3) * sizeof(uint32_t);
                    if (metalen > RCPU_TX_META_SIZE) {
                        metalen = RCPU_TX_META_SIZE;
                    }
                    metadata = (uint32_t *)&skb->data[RCPU_HDR_SIZE];
                    for (idx = 0; idx < BYTES2WORDS(metalen); idx++) {
                        dcb[idx + 2] = ntohl(metadata[idx]);
                    }
                    dcb[1] |= 1 << 19;
                }
            }
        } else if (priv->port >= 0) {
            /* Send to physical port */
            if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                dcb[2] |= 1 << 19;
            } else {
                dcb[1] |= 1 << 19;
            }
            switch (sinfo->dcb_type) {
            case 23:
            case 26:
                dcb[2] = 0x81000000;
                dcb[3] = priv->port;
                dcb[3] |= (priv->qnum & 0xc00) << 20;
                dcb[4] = 0x00040000;
                dcb[4] |= (priv->qnum & 0x3ff) << 8;
                break;
            case 30:
            case 31:
            case 34:
            case 37:
            case 40:
                if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                    meta[0] = htonl(0x81000000);
                    meta[1] = htonl(priv->port);
                    meta[2] = htonl(0x00040000 | (priv->qnum & 0x7) << 24);
                } else {
                    dcb[2] = 0x81000000;
                    dcb[3] = priv->port;
                    dcb[4] = 0x00040000;
                    dcb[4] |= (priv->qnum & 0x7) << 24;
                }
                break;
            case 24:
                dcb[2] = 0xff000000;
                dcb[3] = 0x00000100;
                dcb[4] = priv->port;
                dcb[4] |= (priv->qnum & 0xfff) << 14;
                break;
            case 28:
                /*
                 * If KCOM_NETIF_T_PORT, add PTCH+ITMH header
                 * If KCOM_NETIF_T_VLAN, add PTCH header
                 */
                pktdata = skb->data;
                if (priv->system_headers_size + BKN_DNX_MODULE_HEADER_SIZE <= KCOM_NETIF_SYSTEM_HEADERS_SIZE_MAX) {
                    memcpy(&pktdata[0], priv->system_headers + BKN_DNX_MODULE_HEADER_SIZE, priv->system_headers_size);
                }
                /* Set CPU channel */
                dcb[2] = ((priv->system_headers[0] & 0xff) << 24);
                break;
            case 29:
                dcb[2] = 0x81000000;
                dcb[3] = priv->port;
                dcb[4] = 0x00100000;
                dcb[4] |= (priv->qnum & 0xfff) << 8;
                break;
            case 32:
                dcb[2] = 0x81000000;
                dcb[3] = priv->port;
                dcb[4] = 0x00004000;
                dcb[4] |= (priv->qnum & 0x3f) << 8;
                break;
            case 33:
                dcb[2] = 0x81000000;
                dcb[3] = (priv->port) << 2;
                dcb[4] = 0x00100000;
                dcb[4] |= (priv->qnum & 0xfff) << 8;
                break;
            case 35:
                dcb[2] = 0x81000000;
                dcb[3] = (priv->port) << 4;
                dcb[4] = 0x00400000;
                dcb[4] |= (priv->qnum & 0x3fff) << 8;
                break;
            case 36:
                if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                    meta[0] = htonl(0x81000000);
                    meta[1] = htonl(priv->port);
                    meta[2] = htonl(0x00008000 | (priv->qnum & 0x3f) << 9);
                } else {
                    dcb[2] = 0x81000000;
                    dcb[3] = priv->port;
                    dcb[4] = 0x00008000;
                    dcb[4] |= (priv->qnum & 0x3f) << 9;
                }
                break;
            case 38:
                if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                    meta[0] = htonl(0x81000000);
                    meta[1] = htonl(priv->port);
                    meta[2] = htonl(0x00004000 | (priv->qnum & 0x3f) << 8);
                } else {
                    dcb[2] = 0x81000000;
                    dcb[3] = priv->port;
                    dcb[4] = 0x00004000;
                    dcb[4] |= (priv->qnum & 0x3f) << 8;
                }
                break;
            case 39:
                if (device_is_dnx(sinfo)) {
                    /*
                     * if KCOM_NETIF_T_PORT, add MH+PTCH+ITMH header
                     * if KCOM_NETIF_T_VLAN, add MH+PTCH+header
                     */
                    pktdata = skb->data;
                    if (priv->system_headers_size <= KCOM_NETIF_SYSTEM_HEADERS_SIZE_MAX) {
                        memcpy(&pktdata[0], priv->system_headers, priv->system_headers_size);
                    }
                }
                break;
            case 41:
                if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                    meta[0] = htonl(0x81000000);
                    meta[1] = htonl((priv->port) << 4);
                    meta[2] = htonl(0x00400000 | (priv->qnum & 0x3fff) << 8);
                } else {
                    dcb[2] = 0x81000000;
                    dcb[3] = (priv->port) << 4;
                    dcb[4] = 0x00400000;
                    dcb[4] |= (priv->qnum & 0x3fff) << 8;
                }
                break;
            default:
                dcb[2] = 0xff000000;
                dcb[3] = 0x00000100;
                dcb[4] = priv->port;
                break;
            }
        }

        /* Optional SKB updates */
        if (knet_tx_cb != NULL) {
            KNET_SKB_CB(skb)->netif_user_data = priv->cb_user_data;
            KNET_SKB_CB(skb)->dcb_type = sinfo->dcb_type & 0xFFFF;
            skb = knet_tx_cb(skb, sinfo->dev_no, meta);
            if (skb == NULL) {
                /* Consumed by call-back */
                DBG_WARN(("Tx drop: Consumed by call-back\n"));
                priv->stats.tx_dropped++;
                sinfo->tx.pkts_d_callback++;
                spin_unlock_irqrestore(&sinfo->lock, flags);
                return 0;
            }
            /* Restore (possibly) altered packet variables
             * bit0 -bit15 of dcb[1] is used to save requested byte count
             */
            if ((skb->len + FCS_SZ) <= SOC_DCB_KNET_COUNT_MASK) {
                pktlen = skb->len;
                if (pktlen < (60 + taglen + hdrlen)) {
                    pktlen = (60 + taglen + hdrlen);
                    if (SKB_PADTO(skb, pktlen) != 0) {
                        DBG_WARN(("Tx drop: skb_padto failed\n"));
                        priv->stats.tx_dropped++;
                        sinfo->tx.pkts_d_pad_fail++;
                        dev_kfree_skb_any(skb);
                        spin_unlock_irqrestore(&sinfo->lock, flags);
                        return 0;
                    }
                    DBG_SKB(("Packet padded to %d bytes after tx callback\n", pktlen));
                }
                pktdata = skb->data;
                if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                    meta = (uint32_t *)pktdata;
                }
            } else {
                DBG_WARN(("Tx drop: size of pkt (%d) is out of range(%d)\n",
                         (pktlen + FCS_SZ), SOC_DCB_KNET_COUNT_MASK));
                sinfo->tx.pkts_d_over_limit++;
                priv->stats.tx_dropped++;
                sinfo->tx.pkts_d_callback++;
                dev_kfree_skb_any(skb);
                spin_unlock_irqrestore(&sinfo->lock, flags);
                return 0;
            }
        }

        /* Do Tx timestamping */
        if (bkn_skb_tx_flags(skb) & SKBTX_HW_TSTAMP) {
            KNET_SKB_CB(skb)->hwts = priv->tx_hwts;
            if ((priv->port >= 0) && (priv->tx_hwts & HWTSTAMP_TX_ON)) {
                /* TwoStep Processing of ptp-packets */
                KNET_SKB_CB(skb)->port = priv->phys_port;
                bkn_hw_tstamp_tx_config(sinfo, priv->tx_hwts, hdrlen, skb, meta);

                bkn_skb_tx_flags(skb) |= SKBTX_IN_PROGRESS;
                bkn_skb_tx_timestamp(skb);

            } else if (priv->tx_hwts & HWTSTAMP_TX_ONESTEP_SYNC) {

                /* OneStep Processing of ptp-packets */
                KNET_SKB_CB(skb)->port = priv->phys_port;
                KNET_SKB_CB(skb)->ts = 0;
                bkn_hw_tstamp_tx_config(sinfo, priv->tx_hwts, hdrlen, skb,
                                        ((priv->port >= 0) ? meta : NULL));

                if (KNET_SKB_CB(skb)->ts != 0) {
                    bkn_skb_tx_flags(skb) |= SKBTX_IN_PROGRESS;
                    bkn_skb_tx_timestamp(skb);
                }

            }

            /* Increment ptp tx counters. */
            priv->ptp_stats_tx++;
        }

        /* Prepare for DMA */
        desc->skb = skb;
        /*
         * Add FCS bytes
         * FCS bytes are always appended to packet by MAC on Dune devices
         */
        if (!device_is_sand(sinfo)) {
            pktlen = pktlen + FCS_SZ;
        }
        desc->dma_size = pktlen;
        desc->skb_dma = BKN_DMA_MAP_SINGLE(sinfo->dma_dev,
                                       pktdata, desc->dma_size,
                                       BKN_DMA_TODEV);
        if (BKN_DMA_MAPPING_ERROR(sinfo->dma_dev, desc->skb_dma)) {
            priv->stats.tx_dropped++;
            dev_kfree_skb_any(skb);
            spin_unlock_irqrestore(&sinfo->lock, flags);
            return 0;
        }
        dcb[0] = desc->skb_dma;
        if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
            dcb[1] = DMA_TO_BUS_HI(desc->skb_dma >> 32);
            dcb[2] &= ~SOC_DCB_KNET_COUNT_MASK;
            dcb[2] |= pktlen;
        } else {
            dcb[1] &= ~SOC_DCB_KNET_COUNT_MASK;
            dcb[1] |= pktlen;
        }

        bkn_dump_dcb("Tx RCPU", dcb, sinfo->dcb_wsize, XGS_DMA_TX_CHAN);
        DBG_DCB_TX(("Add Tx DCB @ 0x%08x (%d) [%d free] (%d bytes).\n",
                    (uint32_t)desc->dcb_dma, sinfo->tx.cur,
                    sinfo->tx.free, pktlen));
        bkn_dump_pkt(pktdata, pktlen, XGS_DMA_TX_CHAN);

        if (CDMA_CH(sinfo, XGS_DMA_TX_CHAN)) {
            if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                dcb[2] |= 1 << 24 | 1 << 16;
            } else {
                dcb[1] |= 1 << 24 | 1 << 16;
            }
        } else {
            bkn_tx_dma_start(sinfo);
        }
        if (++sinfo->tx.cur >= MAX_TX_DCBS) {
            sinfo->tx.cur = 0;
        }
        sinfo->tx.free--;

        if (CDMA_CH(sinfo, XGS_DMA_TX_CHAN) && !sinfo->tx.api_active) {
            /* DMA run to the new halt location */
            bkn_cdma_goto(sinfo, XGS_DMA_TX_CHAN,
                          sinfo->tx.desc[sinfo->tx.cur].dcb_dma);
        }

        priv->stats.tx_packets++;
        priv->stats.tx_bytes += pktlen;
        sinfo->tx.pkts++;
    } else {
        DBG_VERB(("Tx busy: No DMA resources\n"));
        sinfo->tx.pkts_d_dma_resrc++;
        bkn_suspend_tx(sinfo);
        spin_unlock_irqrestore(&sinfo->lock, flags);
        return BKN_NETDEV_TX_BUSY;
    }

    NETDEV_UPDATE_TRANS_START_TIME(dev);

    spin_unlock_irqrestore(&sinfo->lock, flags);

    return 0;
}

static void
bkn_timer_func(bkn_switch_info_t *sinfo)
{
    unsigned long flags;
    int chan;
    int restart_timer;

    spin_lock_irqsave(&sinfo->lock, flags);

    sinfo->timer_runs++;

    restart_timer = 0;
    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            continue;
        }
        /* Restart channel if not running */
        if (sinfo->rx[chan].running == 0) {
            bkn_rx_refill(sinfo, chan);
            if (bkn_rx_restart(sinfo, chan) != 0) {
                restart_timer = 1;
            }
        }
    }

    if (restart_timer) {
        /* Presumably still out of memory */
        sinfo->timer.expires = jiffies + 1;
        add_timer(&sinfo->timer);
    } else {
        sinfo->timer_queued = 0;
    }

    spin_unlock_irqrestore(&sinfo->lock, flags);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0))
static void
bkn_timer(unsigned long context)
{
    bkn_switch_info_t *sinfo = (bkn_switch_info_t *)context;
    return bkn_timer_func(sinfo);
}
#else
static void
bkn_timer(struct timer_list *t)
{
    bkn_switch_info_t *sinfo = from_timer(sinfo, t, timer);
    return bkn_timer_func(sinfo);
}
#endif

static void
bkn_rx_add_tokens(bkn_switch_info_t *sinfo, int chan)
{
    unsigned long cur_jif, ticks;
    uint32_t tokens_per_tick;
    bkn_desc_info_t *desc;

    tokens_per_tick = sinfo->rx[chan].rate_max / HZ;
    cur_jif = jiffies;
    ticks = cur_jif - sinfo->rx[chan].tok_jif;
    sinfo->rx[chan].tokens += ticks * tokens_per_tick;
    sinfo->rx[chan].tok_jif = cur_jif;
    if (sinfo->rx[chan].tokens > sinfo->rx[chan].burst_max) {
        sinfo->rx[chan].tokens = sinfo->rx[chan].burst_max;
    }

    /* Restart channel if Rx is suppressed */
    if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
        bkn_rx_refill(sinfo, chan);
        desc = &sinfo->rx[chan].desc[sinfo->rx[chan].dirty];
        if (desc->dcb_dma == sinfo->halt_addr[XGS_DMA_RX_CHAN + chan]) {
            desc = &sinfo->rx[chan].desc[MAX_RX_DCBS];
            bkn_cdma_goto(sinfo, XGS_DMA_RX_CHAN + chan, desc->dcb_dma);
        }
    } else {
        if (sinfo->rx[chan].running == 0) {
            bkn_rx_refill(sinfo, chan);
            bkn_rx_restart(sinfo, chan);
        }
    }
}

static void
bkn_rxtick_func(bkn_switch_info_t *sinfo)
{
    unsigned long flags;
    unsigned long cur_jif, ticks;
    uint32_t pkt_diff;
    int chan;

    spin_lock_irqsave(&sinfo->lock, flags);

    sinfo->rxtick.expires = jiffies + sinfo->rxtick_jiffies;

    /* For debug purposes we maintain a rough actual packet rate */
    if (++sinfo->rxticks >= sinfo->rxticks_per_sec) {
        for (chan = 0; chan < sinfo->rx_chans; chan++) {
            if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
                continue;
            }
            pkt_diff = sinfo->rx[chan].pkts - sinfo->rx[chan].pkts_ref;
            cur_jif = jiffies;
            ticks = cur_jif - sinfo->rx[chan].rate_jif;
            sinfo->rx[chan].rate = (pkt_diff * HZ) / ticks;
            sinfo->rx[chan].rate_jif = cur_jif;
            sinfo->rx[chan].pkts_ref = sinfo->rx[chan].pkts;
        }
        sinfo->rxticks = 0;
    }

    /* Update tokens for Rx rate control */
    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            continue;
        }
        if (sinfo->rx[chan].tokens < sinfo->rx[chan].burst_max) {
            bkn_rx_add_tokens(sinfo, chan);
        }
    }

    spin_unlock_irqrestore(&sinfo->lock, flags);

    add_timer(&sinfo->rxtick);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0))
static void
bkn_rxtick(unsigned long context)
{
    bkn_switch_info_t *sinfo = (bkn_switch_info_t *)context;
    return bkn_rxtick_func(sinfo);
}
#else
static void
bkn_rxtick(struct timer_list *t)
{
    bkn_switch_info_t *sinfo = from_timer(sinfo, t, rxtick);
    return bkn_rxtick_func(sinfo);
}
#endif

static void
bkn_rx_rate_config(bkn_switch_info_t *sinfo)
{
    unsigned long flags;
    int chan;
    uint32_t rxticks_per_sec, rps;
    uint32_t jiffies_per_rxtick;
    uint32_t tokens_per_rxtick;

    spin_lock_irqsave(&sinfo->lock, flags);

    /* Calculate the minimum update frequency across all channels */
    rxticks_per_sec = 1;
    for (chan = 0; chan < NUM_RX_CHAN; chan++) {
        if (sinfo->rx[chan].rate_max == 0) {
            /*
             * Giving default value if rx_rate parameter is 0 to avoid dividing
             * 0 in the following calculation.
             */
            sinfo->rx[chan].rate_max = DEFAULT_RX_RATE;
        }
        if (sinfo->rx[chan].burst_max == 0) {
            sinfo->rx[chan].burst_max = sinfo->rx[chan].rate_max / 10;
        }
        if (sinfo->rx[chan].burst_max == 0) {
            DBG_WARN(("Improper rx_rate %d and rx_burst %d parameter are "
                      "specified for chan %d\n", sinfo->rx[chan].rate_max,
                      sinfo->rx[chan].rate_max,chan));
            sinfo->rx[chan].burst_max = 1;
        }
        rps = sinfo->rx[chan].rate_max / sinfo->rx[chan].burst_max;
        if (rxticks_per_sec < rps) {
            rxticks_per_sec = rps;
        }
    }

    /* Convert update frequency to system ticks */
    jiffies_per_rxtick = HZ / rxticks_per_sec;
    if (jiffies_per_rxtick == 0) {
        jiffies_per_rxtick = 1;
    }
    rxticks_per_sec = HZ / jiffies_per_rxtick;

    for (chan = 0; chan < NUM_RX_CHAN; chan++) {
        /* Ensure that burst size satifies overall rate */
        tokens_per_rxtick = sinfo->rx[chan].rate_max / rxticks_per_sec;
        if (sinfo->rx[chan].burst_max < tokens_per_rxtick) {
            sinfo->rx[chan].burst_max = tokens_per_rxtick;
        }
        /* Ensure that rate has a sane value */
        if (sinfo->rx[chan].rate_max != 0) {
            if (sinfo->rx[chan].rate_max < rxticks_per_sec) {
                sinfo->rx[chan].rate_max = rxticks_per_sec;
            }
        }
        sinfo->rx[chan].tokens = sinfo->rx[chan].burst_max;
    }

    /* Update timer controls */
    sinfo->rxticks_per_sec = rxticks_per_sec;
    sinfo->rxtick_jiffies = jiffies_per_rxtick;

    spin_unlock_irqrestore(&sinfo->lock, flags);
}

static void
bkn_destroy_sinfo(bkn_switch_info_t *sinfo)
{
    list_del(&sinfo->list);
    bkn_free_dcbs(sinfo);
    kfree(sinfo);
}

static bkn_switch_info_t *
bkn_create_sinfo(int dev_no)
{
    bkn_switch_info_t *sinfo;
    int chan;

    if ((sinfo = kmalloc(sizeof(*sinfo), GFP_KERNEL)) == NULL) {
        return NULL;
    }
    memset(sinfo, 0, sizeof(*sinfo));
    INIT_LIST_HEAD(&sinfo->ndev_list);
    INIT_LIST_HEAD(&sinfo->rxpf_list);
    sinfo->base_addr = lkbde_get_dev_virt(dev_no);
    sinfo->dma_dev = lkbde_get_dma_dev(dev_no);
    sinfo->pdev = lkbde_get_hw_dev(dev_no);
    sinfo->dev_no = dev_no;
    sinfo->inst_id = INVALID_INSTANCE_ID;
    sinfo->evt_idx = -1;

    /* Allow configuration process to get lock. */
    sinfo->cfg_api_locked = 0;
    spin_lock_init(&sinfo->lock);
    skb_queue_head_init(&sinfo->tx_ptp_queue);
    INIT_WORK(&sinfo->tx_ptp_work, bkn_hw_tstamp_tx_work);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0))
    init_timer(&sinfo->timer);
    sinfo->timer.data = (unsigned long)sinfo;
    sinfo->timer.function = bkn_timer;
#else
    timer_setup(&sinfo->timer, bkn_timer, 0);
#endif
    sinfo->timer.expires = jiffies + 1;

    INIT_LIST_HEAD(&sinfo->tx.api_dcb_list);

    for (chan = 0; chan < NUM_RX_CHAN; chan++) {
        INIT_LIST_HEAD(&sinfo->rx[chan].api_dcb_list);
        sinfo->rx[chan].use_rx_skb = use_rx_skb;
    }

    /*
     * Check for dual DMA mode where Rx DMA channel 0 uses DMA buffers
     * provided by the BCM API, and the remaining Rx DMA channel(s)
     * use socket buffers (SKB) provided by the Linux kernel.
     */
    if (use_rx_skb == 2) {
        sinfo->rx[0].use_rx_skb = 0;
    }

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,15,0))
    init_timer(&sinfo->rxtick);
    sinfo->rxtick.data = (unsigned long)sinfo;
    sinfo->rxtick.function = bkn_rxtick;
#else
    timer_setup(&sinfo->rxtick, bkn_rxtick, 0);
#endif
    sinfo->rxtick.expires = jiffies + 1;

    for (chan = 0; chan < NUM_RX_CHAN; chan++) {
        sinfo->rx[chan].rate_max = rx_rate[chan];
        sinfo->rx[chan].burst_max = rx_burst[chan];
    }
    bkn_rx_rate_config(sinfo);

    add_timer(&sinfo->rxtick);

    list_add_tail(&sinfo->list, &_sinfo_list);

    return sinfo;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29))
static const struct net_device_ops bkn_netdev_ops = {
    .ndo_open            = bkn_open,
    .ndo_stop            = bkn_stop,
    .ndo_start_xmit      = bkn_tx,
    .ndo_get_stats       = bkn_get_stats,
    .ndo_validate_addr   = eth_validate_addr,
    .ndo_set_rx_mode     = bkn_set_multicast_list,
    .ndo_set_mac_address = bkn_set_mac_address,
    .NDO_IOCTL           = bkn_ioctl,
    .ndo_tx_timeout      = NULL,
    .ndo_change_mtu      = bkn_change_mtu,
#ifdef CONFIG_NET_POLL_CONTROLLER
    .ndo_poll_controller = bkn_poll_controller,
#endif
};
#endif

static void
bkn_get_drvinfo(struct net_device *dev, struct ethtool_drvinfo *drvinfo)
{
    DBG_PTP(("bkn_get_drv_info: module_initialized:%d\n", module_initialized));

    if (!module_initialized) {
        return;
    }

    strlcpy(drvinfo->driver, "bcm-knet", sizeof(drvinfo->driver));
    snprintf(drvinfo->version, sizeof(drvinfo->version), "%d", KCOM_VERSION);
    strlcpy(drvinfo->fw_version, "N/A", sizeof(drvinfo->fw_version));
    strlcpy(drvinfo->bus_info, "N/A", sizeof(drvinfo->bus_info));
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0))
static int
bkn_get_ts_info(struct net_device *dev, struct ethtool_ts_info *info)
{
    bkn_priv_t *priv;
    bkn_switch_info_t *sinfo;

    DBG_PTP(("bkn_get_ts_info: module_initialized:%d\n", module_initialized));

    if (!module_initialized) {
        return -EINVAL;
    }

    if (!dev)
        return -EINVAL;

    priv = netdev_priv(dev);

    if (!priv)
        return -EINVAL;

    sinfo = priv->sinfo;

    if (!sinfo)
        return -EINVAL;

    switch (sinfo->dcb_type) {
    case 28: /* dpp */
    case 26:
    case 32:
    case 33:
    case 35:
    case 36:
    case 38:
    case 40:
    case 41:
    case 37:
        info->so_timestamping = SOF_TIMESTAMPING_TX_HARDWARE |
                                SOF_TIMESTAMPING_TX_SOFTWARE |
                                SOF_TIMESTAMPING_RX_HARDWARE |
                                SOF_TIMESTAMPING_RX_SOFTWARE |
                                SOF_TIMESTAMPING_SOFTWARE |
                                SOF_TIMESTAMPING_RAW_HARDWARE;
        info->tx_types = 1 << HWTSTAMP_TX_OFF | 1 << HWTSTAMP_TX_ON;
        info->rx_filters = 1 << HWTSTAMP_FILTER_NONE | 1 << HWTSTAMP_FILTER_ALL;
        if (knet_hw_tstamp_ptp_clock_index_cb) {
            info->phc_index = knet_hw_tstamp_ptp_clock_index_cb(sinfo->dev_no);
        } else {
            info->phc_index = -1;
        }
        break;
    case 39: /* dnx - q2a, j2c */
        info->so_timestamping = SOF_TIMESTAMPING_TX_HARDWARE |
                                SOF_TIMESTAMPING_TX_SOFTWARE |
                                SOF_TIMESTAMPING_RX_HARDWARE |
                                SOF_TIMESTAMPING_RX_SOFTWARE |
                                SOF_TIMESTAMPING_SOFTWARE |
                                SOF_TIMESTAMPING_RAW_HARDWARE;
        info->tx_types = 1 << HWTSTAMP_TX_OFF | 1 << HWTSTAMP_TX_ON | 1 << HWTSTAMP_TX_ONESTEP_SYNC;
        info->rx_filters = 1 << HWTSTAMP_FILTER_NONE | 1 << HWTSTAMP_FILTER_ALL;
        if (knet_hw_tstamp_ptp_clock_index_cb) {
            info->phc_index = knet_hw_tstamp_ptp_clock_index_cb(sinfo->dev_no);
        } else {
            info->phc_index = -1;
        }
        break;
    default:
        info->so_timestamping = SOF_TIMESTAMPING_TX_SOFTWARE |
                                SOF_TIMESTAMPING_RX_SOFTWARE |
                                SOF_TIMESTAMPING_SOFTWARE;
        info->phc_index = -1;
        break;
    }

    return 0;
}
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0))
static int
bkn_get_link_ksettings(struct net_device *netdev,
                       struct ethtool_link_ksettings *cmd)
{
    bkn_priv_t *priv = netdev_priv(netdev);

    cmd->base.speed = priv->link_settings.speed;
    cmd->base.duplex = priv->link_settings.duplex;

    return 0;
}

static int
bkn_set_link_ksettings(struct net_device *netdev,
                       const struct ethtool_link_ksettings *cmd)
{
    bkn_priv_t *priv = netdev_priv(netdev);

    priv->link_settings.speed = cmd->base.speed;
    priv->link_settings.duplex = cmd->base.speed ? DUPLEX_FULL : 0;

    return 0;
}
#endif

static const struct ethtool_ops bkn_ethtool_ops = {
    .get_drvinfo        = bkn_get_drvinfo,
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,5,0))
    .get_ts_info        = bkn_get_ts_info,
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,6,0))
    .get_link_ksettings = bkn_get_link_ksettings,
    .set_link_ksettings = bkn_set_link_ksettings,
#endif
};

static struct net_device *
bkn_init_ndev(u8 *mac, char *name)
{
    struct net_device *dev;

    /* Create Ethernet device */
    dev = alloc_etherdev(sizeof(bkn_priv_t));

    if (dev == NULL) {
        DBG_WARN(("Error allocating Ethernet device.\n"));
        return NULL;
    }
#ifdef SET_MODULE_OWNER
    SET_MODULE_OWNER(dev);
#endif

    /* Set the device MAC address */
    eth_hw_addr_set(dev, mac);

    /* Device information -- not available right now */
    dev->irq = 0;
    dev->base_addr = 0;

    /* Default MTU should not exceed MTU of switch front-panel ports */
    dev->mtu = default_mtu;
    if (dev->mtu == 0) {
        dev->mtu = rx_buffer_size;
    }
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))
    dev->max_mtu = rx_buffer_size;
#endif

    /* Device vectors */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29))
    dev->netdev_ops = &bkn_netdev_ops;
#else
    dev->open = bkn_open;
    dev->hard_start_xmit = bkn_tx;
    dev->stop = bkn_stop;
    dev->set_multicast_list = bkn_set_multicast_list;
    dev->do_ioctl = NULL;
    dev->get_stats = bkn_get_stats;
    dev->change_mtu = bkn_change_mtu;
#ifdef CONFIG_NET_POLL_CONTROLLER
    dev->poll_controller = bkn_poll_controller;
#endif
#endif
    dev->ethtool_ops = &bkn_ethtool_ops;
    if (name && *name) {
        strncpy(dev->name, name, IFNAMSIZ-1);
    }
    DBG_VERB(("Created Ethernet device %s.\n", dev->name));

    bkn_dev_net_set(dev, current->nsproxy->net_ns);

    /* Register the kernel Ethernet device */
    if (register_netdev(dev)) {
        DBG_WARN(("Error registering Ethernet device.\n"));
        free_netdev(dev);
        return NULL;
    }


    return dev;
}

/*
 * Lookup Ethernet device by name.
 */
static struct net_device *
bkn_lookup_ndev(u8 *mac, char *name)
{
    struct net_device *dev = NULL;

    if (mac == NULL || name == NULL ||  *name == '\0') {
        DBG_WARN(("Invalid input parameters.\n"));
        return NULL;
    }
    /* get Ethernet device */
    dev = netdev_get_by_name(current->nsproxy->net_ns, name, NULL, GFP_ATOMIC);
    if (dev == NULL) {
        DBG_WARN(("Error to get Ethernet device by name.\n"));
        return NULL;
    }
    if (dev->dev_addr[0] != mac[0] || dev->dev_addr[1] != mac[1] || dev->dev_addr[2] != mac[2] ||
        dev->dev_addr[3] != mac[3] || dev->dev_addr[4] != mac[4] || dev->dev_addr[5] != mac[5]) {
        DBG_WARN(("The Mac address doesn't match the Ethernet device that found by name.\n"));
        netdev_put(dev, NULL);
        return NULL;
    }
    return dev;
}

/*
 * Device Link Control Proc Read Entry
 */
static int
bkn_proc_link_show(struct seq_file *m, void *v)
{
    struct list_head *slist, *dlist;
    struct net_device *dev;
    bkn_priv_t *priv;
    bkn_switch_info_t *sinfo;
    unsigned long flags;

    seq_printf(m, "Software link status:\n");
    list_for_each(slist, &_sinfo_list) {
        sinfo = (bkn_switch_info_t *)slist;
        spin_lock_irqsave(&sinfo->lock, flags);
        list_for_each(dlist, &sinfo->ndev_list) {
            priv = (bkn_priv_t *)dlist;
            dev = priv->dev;
            if (dev) {
                seq_printf(m, "  %-14s %s\n", dev->name,
                           netif_carrier_ok(dev) ? "up" : "down");
            }
        }
        spin_unlock_irqrestore(&sinfo->lock, flags);
    }
    return 0;
}

static int
bkn_proc_link_open(struct inode * inode, struct file * file)
{
    return single_open(file, bkn_proc_link_show, NULL);
}

/*
 * Device Link Control Proc Write Entry
 *
 *   Syntax:
 *   <netif>=up|down
 *
 *   Where <netif> is a virtual network interface name.
 *
 *   Examples:
 *   eth4=up
 *   eth4=down
 */
static ssize_t
bkn_proc_link_write(struct file *file, const char *buf,
                    size_t count, loff_t *loff)
{
    struct list_head *slist, *dlist;
    struct net_device *dev;
    bkn_priv_t *priv;
    bkn_switch_info_t *sinfo;
    unsigned long flags;
    char link_str[40];
    char *ptr;
    char *newline;

    if (count > sizeof(link_str)) {
        count = sizeof(link_str) - 1;
        link_str[count] = '\0';
    }
    if (copy_from_user(link_str, buf, count)) {
        return -EFAULT;
    }
    link_str[count] = 0;
    newline = strchr(link_str, '\n');
    if (newline) {
        /* Chop off the trailing newline */
        *newline = '\0';
    }

    if ((ptr = strchr(link_str, '=')) == NULL &&
        (ptr = strchr(link_str, ':')) == NULL) {
        gprintk("Error: link syntax not recognized: '%s'\n", link_str);
        return count;
    }
    *ptr++ = 0;

    dev = NULL;
    list_for_each(slist, &_sinfo_list) {
        sinfo = (bkn_switch_info_t *)slist;
        spin_lock_irqsave(&sinfo->lock, flags);
        list_for_each(dlist, &sinfo->ndev_list) {
            priv = (bkn_priv_t *)dlist;
            if (priv->dev) {
                if (strcmp(priv->dev->name, link_str) == 0) {
                    dev = priv->dev;
                    break;
                }
            }
        }
        if (dev) {
            if (strcmp(ptr, "up") == 0) {
                netif_carrier_on(dev);
            } else if (strcmp(ptr, "down") == 0) {
                netif_carrier_off(dev);
            } else {
                gprintk("Warning: unknown link state setting: '%s'\n", ptr);
            }
            spin_unlock_irqrestore(&sinfo->lock, flags);
            return count;
        }
        spin_unlock_irqrestore(&sinfo->lock, flags);
    }

    gprintk("Warning: unknown network interface: '%s'\n", link_str);

    return count;
}

struct proc_ops bkn_proc_link_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        bkn_proc_link_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_write =       bkn_proc_link_write,
    .proc_release =     single_release,
};

/*
 * Device Rate Control Proc Read Entry
 */
static int
bkn_proc_rate_show(struct seq_file *m, void *v)
{
    int unit = 0;
    struct list_head *list;
    bkn_switch_info_t *sinfo;
    int chan;

    list_for_each(list, &_sinfo_list) {
        sinfo = (bkn_switch_info_t *)list;

        seq_printf(m, "Rate control (unit %d):\n", unit);
        for (chan = 0; chan < sinfo->rx_chans; chan++) {
            if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
                continue;
            }
            seq_printf(m, "  Rx%d max rate  %8u\n",
                            chan, sinfo->rx[chan].rate_max);
            seq_printf(m, "  Rx%d max burst %8u\n",
                            chan, sinfo->rx[chan].burst_max);
            seq_printf(m, "  Rx%d rate      %8u\n",
                            chan, sinfo->rx[chan].rate);
            seq_printf(m, "  Rx%d tokens    %8u\n",
                            chan, sinfo->rx[chan].tokens);
        }

        unit++;
    }
    return 0;
}

static int
bkn_proc_rate_open(struct inode * inode, struct file * file)
{
    return single_open(file, bkn_proc_rate_show, NULL);
}

/*
 * Device Rate Control Proc Write Entry
 *
 *   Syntax:
 *   [<unit>:]rx_rate=<rate0>[,<rate1>[,<rate2]]
 *
 *   Where <rate0> is packets/sec for the first Rx DMA channel,
 *   <rate1> is packets/sec for the second Rx DMA channel, etc.
 *
 *   Examples:
 *   rx_rate=5000
 *   0:rx_rate=10000,10000
 *   1:rx_rate=10000,5000
 */
static ssize_t
bkn_proc_rate_write(struct file *file, const char *buf,
                    size_t count, loff_t *loff)
{
    bkn_switch_info_t *sinfo;
    char rate_str[80];
    char *ptr;
    int unit, chan;

    if (count > sizeof(rate_str)) {
        count = sizeof(rate_str) - 1;
        rate_str[count] = '\0';
    }
    if (copy_from_user(rate_str, buf, count)) {
        return -EFAULT;
    }

    unit = simple_strtol(rate_str, NULL, 10);
    sinfo = bkn_sinfo_from_unit(unit);
    if (sinfo == NULL) {
        gprintk("Warning: unknown unit: %d\n", unit);
        return count;
    }

    if ((ptr = strstr(rate_str, "rx_rate=")) != NULL) {
        ptr += 7;
        chan = 0;
        do {
            ptr++;
            sinfo->rx[chan].rate_max = simple_strtol(ptr, NULL, 10);
        } while ((ptr = strchr(ptr, ',')) != NULL && ++chan < sinfo->rx_chans);
        bkn_rx_rate_config(sinfo);
    } else if ((ptr = strstr(rate_str, "rx_burst=")) != NULL) {
        ptr += 8;
        chan = 0;
        do {
            ptr++;
            sinfo->rx[chan].burst_max = simple_strtol(ptr, NULL, 10);
        } while ((ptr = strchr(ptr, ',')) != NULL && ++chan < sinfo->rx_chans);
        bkn_rx_rate_config(sinfo);
    } else {
        gprintk("Warning: unknown configuration setting\n");
    }

    return count;
}

struct proc_ops bkn_proc_rate_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        bkn_proc_rate_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_write =       bkn_proc_rate_write,
    .proc_release =     single_release,
};

/*
 * Driver DMA Proc Entry
 *
 * This output can be rather large (> PAGE_SIZE) so we use the
 * seq_file interface to do the output. Special header records
 * are indicated by a negative DCB index.
 */
typedef struct {
    int dev_no;         /* Current unit */
    int rx_dma;         /* 0: Tx DMA, 1: Rx DMA*/
    int ch_no;          /* DMA channel no. (Rx only) */
    int idx;            /* DCB index */
} bkn_seq_dma_iter_t;

/* From current record, move forward 'pos' records */
static int
bkn_seq_dma_next_pos(bkn_seq_dma_iter_t *iter, loff_t pos)
{
    bkn_switch_info_t *sinfo;

    if ((sinfo = bkn_sinfo_from_unit(iter->dev_no)) == NULL) {
        return -1;
    }
    while (pos) {
        if (iter->rx_dma) {
            if (++iter->idx >= MAX_RX_DCBS + 1) {
                iter->idx = -1;
                if (++iter->ch_no >= sinfo->rx_chans) {
                    iter->rx_dma = 0;
                    iter->ch_no = 0;
                    iter->dev_no++;
                    if ((sinfo = bkn_sinfo_from_unit(iter->dev_no)) == NULL) {
                        return -1;
                    }
                }
            }
        } else {
            if (++iter->idx >= MAX_TX_DCBS + 1) {
                iter->idx = -1;
                iter->rx_dma = 1;
            }
        }
        pos--;
    }
    return 0;
}

/* Initialize private data and move to requested start record */
static void *
bkn_seq_dma_start(struct seq_file *s, loff_t *pos)
{
    bkn_seq_dma_iter_t *iter;

    iter = kmalloc(sizeof(bkn_seq_dma_iter_t), GFP_KERNEL);
    if (!iter) {
        return NULL;
    }
    memset(iter, 0, sizeof(*iter));
    iter->idx = -2;
    if (bkn_seq_dma_next_pos(iter, *pos) < 0) {
        kfree(iter);
        return NULL;
    }
    return iter;
}

/* Move to next record */
static void *
bkn_seq_dma_next(struct seq_file *s, void *v, loff_t *pos)
{
    bkn_seq_dma_iter_t *iter = (bkn_seq_dma_iter_t *)v;
    void *rv = iter;

    if (bkn_seq_dma_next_pos(iter, 1) < 0) {
        return NULL;
    }
    (*pos)++;
    return rv;
}

/* Release private data */
static void
bkn_seq_dma_stop(struct seq_file *s, void *v)
{
    if (v) {
        kfree(v);
    }
}

/* Print current record */
static int
bkn_seq_dma_show(struct seq_file *s, void *v)
{
    bkn_seq_dma_iter_t *iter = (bkn_seq_dma_iter_t *)v;
    bkn_switch_info_t *sinfo;
    uint32_t *dcb = NULL;
    bkn_dcb_chain_t *dcb_chain = NULL;
    struct list_head *curr, *next;
    unsigned long flags;
    int chan, cnt;

    sinfo = bkn_sinfo_from_unit(iter->dev_no);
    if (sinfo == NULL) {
        /* Should not happen */
        return 0;
    }

    if (iter->rx_dma == 0) {
        if (iter->idx == -2) {
            seq_printf(s, "Pending events: 0x%x\n", sinfo->dma_events);
        } else if (iter->idx == -1) {
            spin_lock_irqsave(&sinfo->lock, flags);
            curr = &sinfo->tx.api_dcb_list;
            dcb_chain = sinfo->tx.api_dcb_chain;
            while (dcb_chain) {
                seq_printf(s, "  [0x%08lx]--->\n", (unsigned long)dcb_chain->dcb_dma);
                for (cnt = 0; cnt < dcb_chain->dcb_cnt; cnt++) {
                    dcb = &dcb_chain->dcb_mem[sinfo->dcb_wsize * cnt];
                    if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                        seq_printf(s, "  DCB %2d: 0x%08x 0x%08x 0x%08x 0x%08x\n", cnt,
                                   dcb[0], dcb[1], dcb[2], dcb[sinfo->dcb_wsize-1]);
                    } else {
                        seq_printf(s, "  DCB %2d: 0x%08x 0x%08x ... 0x%08x\n", cnt,
                                   dcb[0], dcb[1], dcb[sinfo->dcb_wsize-1]);
                    }
                }
                next = curr->next;
                if (next != &sinfo->tx.api_dcb_list) {
                    dcb_chain = list_entry(next, bkn_dcb_chain_t, list);
                    curr = next;
                } else {
                    dcb_chain = NULL;
                }
            }
            dcb = NULL;
            spin_unlock_irqrestore(&sinfo->lock, flags);
            seq_printf(s,
                       "Tx DCB info (unit %d):\n"
                       "  api:   %d\n"
                       "  dirty: %d\n"
                       "  cur:   %d\n"
                       "  free:  %d\n"
                       "  pause: %s\n",
                       iter->dev_no,
                       sinfo->tx.api_active,
                       sinfo->tx.dirty,
                       sinfo->tx.cur,
                       sinfo->tx.free,
                       netif_queue_stopped(sinfo->dev) ? "yes" : "no");
            if (CDMA_CH(sinfo, XGS_DMA_TX_CHAN)) {
                bkn_desc_info_t *desc = &sinfo->tx.desc[0];
                uint64_t halt = sinfo->halt_addr[XGS_DMA_TX_CHAN];
                seq_printf(s,
                           "  halt:  %d\n",
                           (int)((uint32_t)(halt - desc->dcb_dma) / (sinfo->dcb_wsize * sizeof(uint32_t))));
            }
        } else {
            dcb = sinfo->tx.desc[iter->idx].dcb_mem;
            if (iter->idx == 0) {
                seq_printf(s, "  [0x%08lx]--->\n", (unsigned long)sinfo->tx.desc[0].dcb_dma);
            }
        }
    } else {
        if (iter->idx == -1) {
            chan = iter->ch_no;
            seq_printf(s,
                       "Rx%d DCB info (unit %d):\n"
                       "  api:   %d\n"
                       "  dirty: %d\n"
                       "  cur:   %d\n"
                       "  free:  %d\n"
                       "  run:   %d\n",
                       chan, iter->dev_no,
                       sinfo->rx[chan].api_active,
                       sinfo->rx[chan].dirty,
                       sinfo->rx[chan].cur,
                       sinfo->rx[chan].free,
                       sinfo->rx[chan].running);
            if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan) && sinfo->rx[chan].use_rx_skb) {
                bkn_desc_info_t *desc = &sinfo->rx[chan].desc[0];
                uint64_t halt = sinfo->halt_addr[XGS_DMA_RX_CHAN + chan];
                seq_printf(s,
                           "  halt:  %d\n",
                           (int)((uint32_t)(halt - desc->dcb_dma) / (sinfo->dcb_wsize * sizeof(uint32_t))));
            }
        } else if (sinfo->rx[iter->ch_no].use_rx_skb) {
            dcb = sinfo->rx[iter->ch_no].desc[iter->idx].dcb_mem;
            if (iter->idx == 0) {
                seq_printf(s, "  [0x%08lx]--->\n", (unsigned long)sinfo->rx[iter->ch_no].desc[0].dcb_dma);
            }
        } else if (!sinfo->rx[iter->ch_no].use_rx_skb && iter->idx == 0) {
            spin_lock_irqsave(&sinfo->lock, flags);
            curr = &sinfo->rx[iter->ch_no].api_dcb_list;
            dcb_chain = sinfo->rx[iter->ch_no].api_dcb_chain;
            while (dcb_chain) {
                seq_printf(s, "  [0x%08lx]--->\n", (unsigned long)dcb_chain->dcb_dma);
                for (cnt = 0; cnt < dcb_chain->dcb_cnt; cnt++) {
                    dcb = &dcb_chain->dcb_mem[sinfo->dcb_wsize * cnt];
                    if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                        seq_printf(s, "  DCB %2d: 0x%08x 0x%08x 0x%08x 0x%08x\n", cnt,
                                   dcb[0], dcb[1], dcb[2], dcb[sinfo->dcb_wsize-1]);
                    } else {
                        seq_printf(s, "  DCB %2d: 0x%08x 0x%08x ... 0x%08x\n", cnt,
                                   dcb[0], dcb[1], dcb[sinfo->dcb_wsize-1]);
                    }
                }
                next = curr->next;
                if (next != &sinfo->rx[iter->ch_no].api_dcb_list) {
                    dcb_chain = list_entry(next, bkn_dcb_chain_t, list);
                    curr = next;
                } else {
                    dcb_chain = NULL;
                }
            }
            dcb = NULL;
            spin_unlock_irqrestore(&sinfo->lock, flags);
        }
    }
    if (dcb) {
        if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
            seq_printf(s, "  DCB %2d: 0x%08x 0x%08x 0x%08x 0x%08x\n", iter->idx,
                       dcb[0], dcb[1], dcb[2], dcb[sinfo->dcb_wsize-1]);
        } else {
            seq_printf(s, "  DCB %2d: 0x%08x 0x%08x ... 0x%08x\n", iter->idx,
                       dcb[0], dcb[1], dcb[sinfo->dcb_wsize-1]);
        }
    }
    return 0;
}

static struct seq_operations bkn_seq_dma_ops = {
    .start = bkn_seq_dma_start,
    .next  = bkn_seq_dma_next,
    .stop  = bkn_seq_dma_stop,
    .show  = bkn_seq_dma_show
};

static int
bkn_seq_dma_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &bkn_seq_dma_ops);
};

static struct proc_ops bkn_seq_dma_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        bkn_seq_dma_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_release =     seq_release
};

/*
 * Device Debug Control Proc Write Entry
 *
 *   Syntax:
 *   [<unit>:]debug=<mask>
 *
 *   Where <mask> corresponds to the debug module parameter.
 *
 *   Examples:
 *   debug=0xffff
 *   0:debug-0x2000
 */
static ssize_t
bkn_proc_debug_write(struct file *file, const char *buf,
                     size_t count, loff_t *loff)
{
    bkn_switch_info_t *sinfo;
    char debug_str[40];
    char *ptr;
    int unit;

    if (count > sizeof(debug_str)) {
        count = sizeof(debug_str) - 1;
        debug_str[count] = '\0';
    }
    if (copy_from_user(debug_str, buf, count)) {
        return -EFAULT;
    }

    unit = simple_strtol(debug_str, NULL, 10);
    sinfo = bkn_sinfo_from_unit(unit);
    if (sinfo == NULL) {
        gprintk("Warning: unknown unit: %d\n", unit);
        return count;
    }

    if ((ptr = strstr(debug_str, "debug=")) != NULL) {
        ptr += 6;
        debug = simple_strtol(ptr, NULL, 0);
    } else {
        gprintk("Warning: unknown configuration setting\n");
    }

    return count;
}

/*
 * Driver Debug Proc Entry
 */
static int
bkn_proc_debug_show(struct seq_file *m, void *v)
{
    int unit = 0;
    struct list_head *list;
    bkn_switch_info_t *sinfo;

    seq_printf(m, "Configuration:\n");
    seq_printf(m, "  debug:          0x%x\n", debug);
    seq_printf(m, "  mac_addr:       %02x:%02x:%02x:%02x:%02x:%02x\n",
                    bkn_dev_mac[0], bkn_dev_mac[1], bkn_dev_mac[2],
                    bkn_dev_mac[3], bkn_dev_mac[4], bkn_dev_mac[5]);
    seq_printf(m, "  rx_buffer_size: %d (0x%x)\n",
                    rx_buffer_size, rx_buffer_size);
    seq_printf(m, "  rcpu_mode:      %d\n", rcpu_mode);
    seq_printf(m, "  rcpu_dmac:      %02x:%02x:%02x:%02x:%02x:%02x\n",
                    bkn_rcpu_dmac[0], bkn_rcpu_dmac[1], bkn_rcpu_dmac[2],
                    bkn_rcpu_dmac[3], bkn_rcpu_dmac[4], bkn_rcpu_dmac[5]);
    seq_printf(m, "  rcpu_smac:      %02x:%02x:%02x:%02x:%02x:%02x\n",
                    bkn_rcpu_smac[0], bkn_rcpu_smac[1], bkn_rcpu_smac[2],
                    bkn_rcpu_smac[3], bkn_rcpu_smac[4], bkn_rcpu_smac[5]);
    seq_printf(m, "  rcpu_ethertype: 0x%x\n", rcpu_ethertype);
    seq_printf(m, "  rcpu_signature: 0x%x\n", rcpu_signature);
    seq_printf(m, "  rcpu_vlan:      %d\n", rcpu_vlan);
    seq_printf(m, "  use_rx_skb:     %d\n", use_rx_skb);
    seq_printf(m, "  num_rx_prio:    %d\n", num_rx_prio);
    seq_printf(m, "  check_rcpu_sig: %d\n", check_rcpu_signature);
    seq_printf(m, "  default_mtu:    %d\n", default_mtu);
    seq_printf(m, "  rx_sync_retry:  %d\n", rx_sync_retry);
    seq_printf(m, "  use_napi:       %d\n", use_napi);
    seq_printf(m, "  napi_weight:    %d\n", napi_weight);
    seq_printf(m, "  basedev_susp:   %d\n", basedev_suspend);
    seq_printf(m, "  force_tagged:   %d\n", force_tagged);
    seq_printf(m, "  ft_tpid:        %d\n", ft_tpid);
    seq_printf(m, "  ft_pri:         %d\n", ft_pri);
    seq_printf(m, "  ft_cfi:         %d\n", ft_cfi);
    seq_printf(m, "  ft_vid:         %d\n", ft_vid);
    seq_printf(m, "Active IOCTLs:\n");
    seq_printf(m, "  Command:        %d\n", ioctl_cmd);
    seq_printf(m, "  Event:          %d\n", ioctl_evt);

    list_for_each(list, &_sinfo_list) {
        sinfo = (bkn_switch_info_t *)list;

        seq_printf(m, "Device %d:\n", unit);
        seq_printf(m, "  base_addr:      0x%p\n", sinfo->base_addr);
        seq_printf(m, "  dev_no:         %d\n", sinfo->dev_no);
        seq_printf(m, "  cmic_type:      %c\n", sinfo->cmic_type);
        seq_printf(m, "  dcb_type:       %d\n", sinfo->dcb_type);
        seq_printf(m, "  dcb_wsize:      %d\n", sinfo->dcb_wsize);
        seq_printf(m, "  pkt_hdr_size:   %d\n", sinfo->pkt_hdr_size);
        seq_printf(m, "  rx_chans:       %d\n", sinfo->rx_chans);
        seq_printf(m, "  cdma_chans:     0x%x\n", sinfo->cdma_channels);
        seq_printf(m, "  unet_chans:     0x%x\n", sinfo->unet_channels);
        seq_printf(m, "  enet_chans:     0x%x\n", sinfo->enet_channels);
        seq_printf(m, "  irq_mask:       0x%x\n", sinfo->irq_mask);
        seq_printf(m, "  dma_events:     0x%x\n", sinfo->dma_events);
        seq_printf(m, "  dcb_dma:        0x%p\n", (void *)(sal_paddr_t)sinfo->dcb_dma);
        seq_printf(m, "  dcb_mem_size:   0x%x\n", sinfo->dcb_mem_size);
        seq_printf(m, "  rcpu_sig:       0x%x\n", sinfo->rcpu_sig);
        seq_printf(m, "  napi_poll_mode: %d\n", sinfo->napi_poll_mode);
        seq_printf(m, "  inst_id:        0x%x\n", sinfo->inst_id);
        seq_printf(m, "  evt_queue:      %d\n", sinfo->evt_idx);
        seq_printf(m, "  oamp_port_num:  %d {%d, %d, %d, %d}\n",
                      sinfo->oamp_port_number,
                      sinfo->oamp_ports[0],
                      sinfo->oamp_ports[1],
                      sinfo->oamp_ports[2],
                      sinfo->oamp_ports[3]);
        seq_printf(m, "  device_id:      0x%x\n", sinfo->base_id);
        seq_printf(m, "  pcie_status:    %d\n", sinfo->pcie_link_status);
        unit++;
    }

    return 0;
}

static int bkn_proc_debug_open(struct inode * inode, struct file * file)
{
    return single_open(file, bkn_proc_debug_show, NULL);
}

struct proc_ops bkn_proc_debug_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        bkn_proc_debug_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_write =       bkn_proc_debug_write,
    .proc_release =     single_release,
};

/*
 * Device Statistics Proc Entry
 */
static int
bkn_proc_stats_show(struct seq_file *m, void *v)
{
    int unit = 0;
    struct list_head *list, *flist;
    bkn_switch_info_t *sinfo;
    bkn_filter_t *filter;
    int chan;
    unsigned long flags;

    list_for_each(list, &_sinfo_list) {
        sinfo = (bkn_switch_info_t *)list;
        spin_lock_irqsave(&sinfo->lock, flags);

        seq_printf(m, "Device stats (unit %d):\n", unit);
        seq_printf(m, "  Interrupts  %10u\n", sinfo->interrupts);
        seq_printf(m, "  Tx packets  %10u\n", sinfo->tx.pkts);
        for (chan = 0; chan < sinfo->rx_chans; chan++) {
            seq_printf(m, "  Rx%d packets %10u\n", chan, sinfo->rx[chan].pkts);
        }
        for (chan = 0; chan < sinfo->rx_chans; chan++) {
            if (sinfo->interrupts == 0) {
                /* Avoid divide-by-zero */
                seq_printf(m, "  Rx%d pkts/intr        -\n", chan);
            } else {
                seq_printf(m, "  Rx%d pkts/intr %8u\n",
                           chan, sinfo->rx[chan].pkts / sinfo->interrupts);
            }
        }
        seq_printf(m, "  Timer runs  %10u\n", sinfo->timer_runs);
        seq_printf(m, "  NAPI reruns %10u\n", sinfo->napi_not_done);

        list_for_each(flist, &sinfo->rxpf_list) {
            filter = (bkn_filter_t *)flist;

            seq_printf(m, "  Filter %d stats:\n", filter->kf.id);
            seq_printf(m, "    Hits      %10lu\n", filter->hits);
        }

        unit++;
        spin_unlock_irqrestore(&sinfo->lock, flags);
    }
    return 0;
}

static int bkn_proc_stats_open(struct inode * inode, struct file * file)
{
    return single_open(file, bkn_proc_stats_show, NULL);
}

/*
 * Device Statistics Proc Write Entry
 *
 *   Syntax:
 *   [<unit>:]clear[=all]
 *
 *   Where <mask> corresponds to the debug module parameter.
 *
 *   Examples:
 *   clear
 *   0:clear=all
 */
static ssize_t
bkn_proc_stats_write(struct file *file, const char *buf,
                     size_t count, loff_t *loff)
{
    bkn_switch_info_t *sinfo;
    struct list_head *flist;
    bkn_filter_t *filter;
    char debug_str[40];
    char *ptr;
    int unit;
    int clear_mask;
    int chan;

    if (count > sizeof(debug_str)) {
        count = sizeof(debug_str) - 1;
        debug_str[count] = '\0';
    }
    if (copy_from_user(debug_str, buf, count)) {
        return -EFAULT;
    }

    unit = simple_strtol(debug_str, NULL, 10);
    sinfo = bkn_sinfo_from_unit(unit);
    if (sinfo == NULL) {
        gprintk("Warning: unknown unit: %d\n", unit);
        return count;
    }

    clear_mask = 0;
    if ((ptr = strstr(debug_str, "clear=")) != NULL) {
        ptr += 6;
        if (strncmp(ptr, "all", 3) == 0) {
            clear_mask = ~0;
        }
    } else if ((ptr = strstr(debug_str, "clear")) != NULL) {
        clear_mask = ~0;
    } else {
        gprintk("Warning: unknown configuration setting\n");
    }

    if (clear_mask) {
        sinfo->tx.pkts = 0;
        for (chan = 0; chan < sinfo->rx_chans; chan++) {
            sinfo->rx[chan].pkts = 0;
        }
        sinfo->interrupts = 0;
        sinfo->timer_runs = 0;
        sinfo->napi_not_done = 0;
        list_for_each(flist, &sinfo->rxpf_list) {
            filter = (bkn_filter_t *)flist;
            filter->hits = 0;
        }
    }

    return count;
}

struct proc_ops bkn_proc_stats_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        bkn_proc_stats_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_write =       bkn_proc_stats_write,
    .proc_release =     single_release,
};


/*
 * Device Debug Statistics Proc Entry
 */
static int
bkn_proc_dstats_show(struct seq_file *m, void *v)
{
    int unit = 0;
    struct list_head *list;
    bkn_switch_info_t *sinfo;
    int chan;
    unsigned long flags;

    list_for_each(list, &_sinfo_list) {
        sinfo = (bkn_switch_info_t *)list;
        spin_lock_irqsave(&sinfo->lock, flags);

        seq_printf(m, "Device debug stats (unit %d):\n", unit);
        seq_printf(m, "  Tx drop no skb      %10u\n",
                        sinfo->tx.pkts_d_no_skb);
        seq_printf(m, "  Tx drop rcpu encap  %10u\n",
                        sinfo->tx.pkts_d_rcpu_encap);
        seq_printf(m, "  Tx drop rcpu sig    %10u\n",
                        sinfo->tx.pkts_d_rcpu_sig);
        seq_printf(m, "  Tx drop rcpu meta   %10u\n",
                        sinfo->tx.pkts_d_rcpu_meta);
        seq_printf(m, "  Tx drop pad failed  %10u\n",
                        sinfo->tx.pkts_d_pad_fail);
        seq_printf(m, "  Tx drop no resource %10u\n",
                        sinfo->tx.pkts_d_dma_resrc);
        seq_printf(m, "  Tx drop callback    %10u\n",
                        sinfo->tx.pkts_d_callback);
        seq_printf(m, "  Tx drop no link     %10u\n",
                        sinfo->tx.pkts_d_no_link);
        seq_printf(m, "  Tx drop oversized   %10u\n",
                        sinfo->tx.pkts_d_over_limit);
        seq_printf(m, "  Tx suspends         %10u\n",
                        sinfo->tx.suspends);
        for (chan = 0; chan < sinfo->rx_chans; chan++) {
            seq_printf(m, "  Rx%d filter to api   %10u\n",
                            chan, sinfo->rx[chan].pkts_f_api);
            seq_printf(m, "  Rx%d filter to netif %10u\n",
                            chan, sinfo->rx[chan].pkts_f_netif);
            seq_printf(m, "  Rx%d mirror to api   %10u\n",
                            chan, sinfo->rx[chan].pkts_m_api);
            seq_printf(m, "  Rx%d mirror to netif %10u\n",
                            chan, sinfo->rx[chan].pkts_m_netif);
            seq_printf(m, "  Rx%d drop no skb     %10u\n",
                            chan, sinfo->rx[chan].pkts_d_no_skb);
            seq_printf(m, "  Rx%d drop no match   %10u\n",
                            chan, sinfo->rx[chan].pkts_d_no_match);
            seq_printf(m, "  Rx%d drop unkn netif %10u\n",
                            chan, sinfo->rx[chan].pkts_d_unkn_netif);
            seq_printf(m, "  Rx%d drop unkn dest  %10u\n",
                            chan, sinfo->rx[chan].pkts_d_unkn_dest);
            seq_printf(m, "  Rx%d drop callback   %10u\n",
                            chan, sinfo->rx[chan].pkts_d_callback);
            seq_printf(m, "  Rx%d drop no link    %10u\n",
                            chan, sinfo->rx[chan].pkts_d_no_link);
            seq_printf(m, "  Rx%d sync error      %10u\n",
                            chan, sinfo->rx[chan].sync_err);
            seq_printf(m, "  Rx%d sync retry      %10u\n",
                            chan, sinfo->rx[chan].sync_retry);
            seq_printf(m, "  Rx%d sync maxloop    %10u\n",
                            chan, sinfo->rx[chan].sync_maxloop);
            seq_printf(m, "  Rx%d drop no buffer  %10u\n",
                            chan, sinfo->rx[chan].pkts_d_no_api_buf);
        }
        unit++;
        spin_unlock_irqrestore(&sinfo->lock, flags);
    }
    return 0;
}

static int bkn_proc_dstats_open(struct inode * inode, struct file * file)
{
    return single_open(file, bkn_proc_dstats_show, NULL);
}

/*
 * Device Debug Statistics Proc Write Entry
 *
 *   Syntax:
 *   [<unit>:]clear[=all|tx|rx[<chan>]]
 *
 *   Where <mask> corresponds to the debug module parameter.
 *
 *   Examples:
 *   clear
 *   0:clear=rx1
 */
static ssize_t
bkn_proc_dstats_write(struct file *file, const char *buf,
                      size_t count, loff_t *loff)
{
    bkn_switch_info_t *sinfo;
    char debug_str[40];
    char *ptr;
    int unit;
    int clear_mask;
    int chan;

    if (count > sizeof(debug_str)) {
        count = sizeof(debug_str) - 1;
        debug_str[count] = '\0';
    }
    if (copy_from_user(debug_str, buf, count)) {
        return -EFAULT;
    }

    unit = simple_strtol(debug_str, NULL, 10);
    sinfo = bkn_sinfo_from_unit(unit);
    if (sinfo == NULL) {
        gprintk("Warning: unknown unit: %d\n", unit);
        return count;
    }

    clear_mask = 0;
    if ((ptr = strstr(debug_str, "clear=")) != NULL) {
        ptr += 6;
        if (strncmp(ptr, "all", 3) == 0) {
            clear_mask = ~0;
        } else if (strncmp(ptr, "dev", 3) == 0) {
            clear_mask = 0x20;
        } else if (strncmp(ptr, "tx", 2) == 0) {
            clear_mask = 0x10;
        } else if (strncmp(ptr, "rx0", 3) == 0) {
            clear_mask = (1 << 0);
        } else if (strncmp(ptr, "rx1", 3) == 0) {
            clear_mask = (1 << 1);
        } else if (strncmp(ptr, "rx", 2) == 0) {
            clear_mask = 0xf;
        }
    } else if ((ptr = strstr(debug_str, "clear")) != NULL) {
        clear_mask = ~0;
    } else {
        gprintk("Warning: unknown configuration setting\n");
    }

    /* Tx counters */
    if (clear_mask & 0x10) {
        sinfo->tx.pkts_d_no_skb = 0;
        sinfo->tx.pkts_d_rcpu_encap = 0;
        sinfo->tx.pkts_d_rcpu_sig = 0;
        sinfo->tx.pkts_d_rcpu_meta = 0;
        sinfo->tx.pkts_d_pad_fail = 0;
        sinfo->tx.pkts_d_over_limit = 0;
        sinfo->tx.pkts_d_dma_resrc = 0;
        sinfo->tx.suspends = 0;
    }
    /* Rx counters */
    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (clear_mask & (1 << chan)) {
            sinfo->rx[chan].pkts_f_api = 0;
            sinfo->rx[chan].pkts_f_netif = 0;
            sinfo->rx[chan].pkts_m_api = 0;
            sinfo->rx[chan].pkts_m_netif = 0;
            sinfo->rx[chan].pkts_d_no_skb = 0;
            sinfo->rx[chan].pkts_d_no_match = 0;
            sinfo->rx[chan].pkts_d_unkn_netif = 0;
            sinfo->rx[chan].pkts_d_unkn_dest = 0;
            sinfo->rx[chan].pkts_d_no_api_buf = 0;
            sinfo->rx[chan].sync_err = 0;
            sinfo->rx[chan].sync_retry = 0;
            sinfo->rx[chan].sync_maxloop = 0;
        }
    }

    return count;
}

struct proc_ops bkn_proc_dstats_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        bkn_proc_dstats_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_write =       bkn_proc_dstats_write,
    .proc_release =     single_release,
};

/*
 * PTP Statistics Proc Entry
 */
static int
bkn_proc_ptp_stats_show(struct seq_file *m, void *v)
{
    int unit = 0;
    struct list_head *list, *dlist;
    bkn_switch_info_t *sinfo;
    bkn_priv_t *priv;
    struct net_device *dev;
    int print_hdr_done = 0;


    list_for_each(list, &_sinfo_list) {
        sinfo = (bkn_switch_info_t *)list;
        list_for_each(dlist, &sinfo->ndev_list) {
            priv = (bkn_priv_t *)dlist;
            dev = priv->dev;
            if (dev) {
                if (priv->tx_hwts || priv->rx_hwts) {
                    if (!print_hdr_done) {
                        seq_printf(m, "PTP message stats (unit %d):\n", unit);
                        seq_printf(m, "  %-10s| %8s| %12s| %12s| %8s| %8s\n",
                                      "intf", "phc_type", "tx_pkts", "rx_pkts", "tx_hwts", "rx_hwts");
                        seq_printf(m, "======================================================================\n");
                        print_hdr_done = 1;
                    }
                    seq_printf(m, "  %-10s| %8s| %12d| %12d| %8d| %8d\n",
                                  dev->name, (priv->tx_hwts & HWTSTAMP_TX_ONESTEP_SYNC ? "1-step": "2-step"),
                                  priv->ptp_stats_tx, priv->ptp_stats_rx,
                                  priv->tx_hwts, priv->rx_hwts);
                }
            }
        }

        unit++;
    }
    return 0;
}

static int bkn_proc_ptp_stats_open(struct inode * inode, struct file * file)
{
    return single_open(file, bkn_proc_ptp_stats_show, NULL);
}

/*
 * PTP Statistics Proc Write Entry
 *
 *   Where <mask> corresponds to the debug module parameter.
 *
 *   Examples:
 *   clear
 */
static ssize_t
bkn_proc_ptp_stats_write(struct file *file, const char *buf, size_t count, loff_t *loff)
{
    struct list_head *list, *dlist;
    bkn_switch_info_t *sinfo;
    bkn_priv_t *priv;
    struct net_device *dev;
    char debug_str[40];
    char *ptr;
    int clear_mask;

    if (count > sizeof(debug_str)) {
        count = sizeof(debug_str) - 1;
        debug_str[count] = '\0';
    }
    if (copy_from_user(debug_str, buf, count)) {
        return -EFAULT;
    }

    clear_mask = 0;
    if ((ptr = strstr(debug_str, "clear")) != NULL) {
        clear_mask = ~0;
    } else {
        gprintk("Warning: unknown configuration setting\n");
    }

    if (clear_mask) {

        list_for_each(list, &_sinfo_list) {
            sinfo = (bkn_switch_info_t *)list;
            list_for_each(dlist, &sinfo->ndev_list) {
                priv = (bkn_priv_t *)dlist;
                dev = priv->dev;
                if (dev) {
                    priv->ptp_stats_rx = 0;
                    priv->ptp_stats_tx = 0;
                }
            }

        }
    }

    return count;
}

struct proc_ops bkn_proc_ptp_stats_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        bkn_proc_ptp_stats_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_write =       bkn_proc_ptp_stats_write,
    .proc_release =     single_release,
};

/*
 * Device Associated virtual Ethernet interfaces Proc Entry
 */
static int
bkn_proc_ndev_show(struct seq_file *m, void *v)
{
    int unit = 0;
    struct list_head *list, *plist;
    bkn_switch_info_t *sinfo;
    struct net_device *dev;
    bkn_priv_t *priv, *lpriv;
    int id, idx;
    unsigned long flags;

    list_for_each(list, &_sinfo_list) {
        sinfo = (bkn_switch_info_t *)list;
        spin_lock_irqsave(&sinfo->lock, flags);
        seq_printf(m, "Device virtual Ethernet interfaces (dev_no %d):\n", unit);
        if (sinfo->ndevs && sinfo->ndev_max > 0) {
            for (id = 0; id < sinfo->ndev_max; id++) {
                if (sinfo->ndevs[id] == NULL) {
                    continue;
                }
                dev = sinfo->ndevs[id];
                priv = netdev_priv(dev);
                seq_printf(m, "  Dev Id: %d max. %d [sinfo @%p]\n", id, sinfo->ndev_max, sinfo);
                seq_printf(m, "      My Station: %s (dev @%p)\n", dev->name, dev);
                seq_printf(m, "          MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                              dev->dev_addr[0], dev->dev_addr[1], dev->dev_addr[2],
                              dev->dev_addr[3], dev->dev_addr[4], dev->dev_addr[5]);
                if (sinfo->dev_no == priv->sinfo->dev_no) {
                    seq_printf(m, "      Private: (priv @%p)\n", priv);
                    seq_printf(m, "          id        = %d\n", priv->id);
                    seq_printf(m, "          dev       = @%p\n", priv->dev);
                    seq_printf(m, "          sinfo     = @%p\n", priv->sinfo);
                    seq_printf(m, "          type      = %d\n", priv->type);
                    seq_printf(m, "          vlan      = %d\n", priv->vlan);
                    seq_printf(m, "          port      = %d\n", priv->port);
                    seq_printf(m, "          phys_port = %d\n", priv->phys_port);
                    seq_printf(m, "          qnum      = %d\n", priv->qnum);
                    seq_printf(m, "          flags     = 0x%x %s\n", priv->flags,
                                                                     priv->flags & KCOM_NETIF_F_USE_SHARED_NDEV ? "(shared)" : "");
                    seq_printf(m, "          ref_count = %d\n", priv->ref_count);
                    seq_printf(m, "          System Header (%dB): 0x", priv->system_headers_size);
                    for (idx = 0; idx < priv->system_headers_size; idx++) {
                        seq_printf(m, "%02x", priv->system_headers[idx]);
                    }
                    seq_printf(m, "\n");
                }
            }
            if (!list_empty(&sinfo->ndev_list)) {
                seq_printf(m, "      sinfo->ndev_list: [priv->id @priv]\n");
                list_for_each(plist, &sinfo->ndev_list) {
                    lpriv = (bkn_priv_t *)plist;
                    seq_printf(m, "           %3d @%p\n", lpriv->id, lpriv);
                }
            }
        }
        unit++;
        spin_unlock_irqrestore(&sinfo->lock, flags);
    }

    return 0;
}

static int bkn_proc_ndev_open(struct inode * inode, struct file * file)
{
    return single_open(file, bkn_proc_ndev_show, NULL);
}

struct proc_ops bkn_proc_ndev_file_ops = {
    PROC_OWNER(THIS_MODULE)
    .proc_open =        bkn_proc_ndev_open,
    .proc_read =        seq_read,
    .proc_lseek =       seq_lseek,
    .proc_release =     single_release,
};


static int
bkn_proc_init(void)
{
    struct proc_dir_entry *entry;

    PROC_CREATE(entry, "link", 0666, bkn_proc_root, &bkn_proc_link_file_ops);
    if (entry == NULL) {
        return -1;
    }
    PROC_CREATE(entry, "rate", 0666, bkn_proc_root, &bkn_proc_rate_file_ops);
    if (entry == NULL) {
        return -1;
    }
    PROC_CREATE(entry, "dma", 0, bkn_proc_root, &bkn_seq_dma_file_ops);
    if (entry == NULL) {
        return -1;
    }
    PROC_CREATE(entry, "debug", 0666, bkn_proc_root, &bkn_proc_debug_file_ops);
    if (entry == NULL) {
        return -1;
    }
    PROC_CREATE(entry, "stats", 0666, bkn_proc_root, &bkn_proc_stats_file_ops);
    if (entry == NULL) {
        return -1;
    }
    PROC_CREATE(entry, "dstats", 0666, bkn_proc_root, &bkn_proc_dstats_file_ops);
    if (entry == NULL) {
        return -1;
    }
    PROC_CREATE(entry, "ptp_stats", 0666, bkn_proc_root, &bkn_proc_ptp_stats_file_ops);
    if (entry == NULL) {
        return -1;
    }
    PROC_CREATE(entry, "ndev", 0666, bkn_proc_root, &bkn_proc_ndev_file_ops);
    if (entry == NULL) {
        return -1;
    }

    return 0;
}

static int
bkn_proc_cleanup(void)
{
    remove_proc_entry("link", bkn_proc_root);
    remove_proc_entry("rate", bkn_proc_root);
    remove_proc_entry("dma", bkn_proc_root);
    remove_proc_entry("debug", bkn_proc_root);
    remove_proc_entry("stats", bkn_proc_root);
    remove_proc_entry("dstats", bkn_proc_root);
    remove_proc_entry("ptp_stats", bkn_proc_root);
    remove_proc_entry("ndev", bkn_proc_root);
    return 0;
}

/*
 * Generic module functions
 */

static int
_pprint(struct seq_file *m)
{
    pprintf(m, "Broadcom BCM KNET Linux Network Driver\n");

    return 0;
}

static int
bkn_knet_dma_info(kcom_msg_dma_info_t *kmsg, int len)
{
    bkn_switch_info_t *sinfo;
    bkn_dcb_chain_t *dcb_chain, *dcb_chain_end;
    unsigned long flags;
    int chan;
    uint64_t dcb_dma;
    int woffset;

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
    if (sinfo == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    if (kmsg->dma_info.cnt < 1 || kmsg->dma_info.data.dcb_start == 0) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    dcb_chain = kmalloc(sizeof(*dcb_chain), GFP_KERNEL);
    if (dcb_chain == NULL) {
        gprintk("Fatal error: No memory for dcb_chain\n");
        kmsg->hdr.status = KCOM_E_RESOURCE;
        return sizeof(kcom_msg_hdr_t);
    }
    memset(dcb_chain, 0, sizeof(*dcb_chain));
    dcb_chain->dcb_cnt = kmsg->dma_info.cnt;
    dcb_chain->dcb_dma = kmsg->dma_info.data.dcb_start;
    dcb_chain->dcb_mem = kernel_bde->p2l(sinfo->dev_no,
                                         (sal_paddr_t)dcb_chain->dcb_dma);

    if (kmsg->dma_info.type == KCOM_DMA_INFO_T_TX_DCB) {
        spin_lock_irqsave(&sinfo->lock, flags);

        /* Hold back packets from kernel */
        bkn_suspend_tx(sinfo);

        list_add_tail(&dcb_chain->list, &sinfo->tx.api_dcb_list);

        /* Handle for Continuous DMA mode */
        if (CDMA_CH(sinfo, XGS_DMA_TX_CHAN)) {
            woffset = (dcb_chain->dcb_cnt - 1) * sinfo->dcb_wsize + 1;
            if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                woffset += 1;
            }
            if ((dcb_chain->dcb_mem[woffset] & ((1 << 18) | (1 << 16))) != 0x50000) {
                gprintk("Not suitable API DCB chain for Continuous DMA mode\n");
                list_del(&dcb_chain->list);
                kfree(dcb_chain);
                kmsg->hdr.status = KCOM_E_PARAM;
                spin_unlock_irqrestore(&sinfo->lock, flags);
                return sizeof(kcom_msg_hdr_t);
            }
            dcb_chain_end = sinfo->tx.api_dcb_chain_end;
            if (dcb_chain_end != NULL) {
                /* Stitch this chain */
                woffset = (dcb_chain_end->dcb_cnt - 1) * sinfo->dcb_wsize;
                dcb_chain_end->dcb_mem[woffset] = dcb_chain->dcb_dma;
                if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                    dcb_chain_end->dcb_mem[woffset + 1] = DMA_TO_BUS_HI(dcb_chain->dcb_dma >> 32);
                }
                MEMORY_BARRIER;
            }
            sinfo->tx.api_dcb_chain_end = dcb_chain;
            if (sinfo->tx.api_active) {
                /* Set new halt location */
                woffset = (dcb_chain->dcb_cnt - 1) * sinfo->dcb_wsize;
                dcb_dma = dcb_chain->dcb_dma + woffset * sizeof(uint32_t);
                /* DMA run to the new halt location */
                bkn_cdma_goto(sinfo, XGS_DMA_TX_CHAN, dcb_dma);
            }
        }

        if (sinfo->tx.free == MAX_TX_DCBS &&
            !sinfo->tx.api_active &&
            !sinfo->basedev_suspended) {
            bkn_api_tx(sinfo);
        }

        spin_unlock_irqrestore(&sinfo->lock, flags);
    } else if (kmsg->dma_info.type == KCOM_DMA_INFO_T_RX_DCB) {
        spin_lock_irqsave(&sinfo->lock, flags);

        chan = kmsg->dma_info.chan - 1;
        if ((chan < 0) || (chan > sinfo->rx_chans) || UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            gprintk("Invalid RX DMA channel specified: %d\n",
                    kmsg->dma_info.chan);
            kmsg->hdr.status = KCOM_E_PARAM;
            spin_unlock_irqrestore(&sinfo->lock, flags);
            return sizeof(kcom_msg_hdr_t);
        }

        list_add_tail(&dcb_chain->list, &sinfo->rx[chan].api_dcb_list);

        /* Handle for Continuous DMA mode */
        if (CDMA_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
            woffset = (dcb_chain->dcb_cnt - 1) * sinfo->dcb_wsize + 1;
            if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                woffset += 1;
            }
            if ((dcb_chain->dcb_mem[woffset] & ((1 << 18) | (1 << 16))) != 0x50000) {
                gprintk("Not suitable API DCB chain for Continuous DMA mode\n");
                list_del(&dcb_chain->list);
                kfree(dcb_chain);
                kmsg->hdr.status = KCOM_E_PARAM;
                spin_unlock_irqrestore(&sinfo->lock, flags);
                return sizeof(kcom_msg_hdr_t);
            }
            dcb_chain_end = sinfo->rx[chan].api_dcb_chain_end;
            if (dcb_chain_end != NULL) {
                /* Stitch this chain */
                woffset = (dcb_chain_end->dcb_cnt - 1) * sinfo->dcb_wsize;
                dcb_chain_end->dcb_mem[woffset] = dcb_chain->dcb_dma;
                if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
                    dcb_chain_end->dcb_mem[woffset + 1] = DMA_TO_BUS_HI(dcb_chain->dcb_dma >> 32);
                }
                MEMORY_BARRIER;
            }
            sinfo->rx[chan].api_dcb_chain_end = dcb_chain;
            if (!sinfo->rx[chan].use_rx_skb) {
                /* Set new halt location */
                woffset = (dcb_chain->dcb_cnt - 1) * sinfo->dcb_wsize;
                dcb_dma = dcb_chain->dcb_dma + woffset * sizeof(uint32_t);
                /* DMA run to the new halt location */
                bkn_cdma_goto(sinfo, XGS_DMA_RX_CHAN + chan, dcb_dma);
            }
        }

        if (sinfo->rx[chan].api_active == 0) {
            bkn_api_rx_restart(sinfo, chan);
        }

        spin_unlock_irqrestore(&sinfo->lock, flags);
    } else {
        DBG_DCB(("Unknown DCB_INFO type (%d).\n", kmsg->dma_info.type));
        kfree(dcb_chain);
        kmsg->hdr.status = KCOM_E_PARAM;
    }

    return sizeof(kcom_msg_hdr_t);
}

static int
bkn_create_inst(uint32 inst_id)
{
    bkn_switch_info_t *sinfo;
    bkn_evt_resource_t *evt;
    unsigned long flags;
    int i, evt_idx = -1;

    /* multiple instance mode */
    for (i = 0; i < kernel_bde->num_devices(BDE_ALL_DEVICES); i++) {
        evt = &_bkn_evt[i];
        if (evt->inst_id == inst_id) {
            evt_idx = i;
            DBG_INST(("%s evt_idx %d inst_id 0x%x\n",__FUNCTION__, i, inst_id));
            break;
        }
        if ((_bkn_multi_inst == 0) || (evt->inst_id == INVALID_INSTANCE_ID)) {
            _bkn_multi_inst ++;
            evt_idx = i;
            /* avoid repeated initialization */
            if (!waitqueue_active(&evt->evt_wq)) {
                init_waitqueue_head(&evt->evt_wq);
            }
            evt->inst_id = inst_id;
            DBG_INST(("%s evt_idx %d inst_id 0x%x\n",__FUNCTION__, i, inst_id));
            break;
        }
    }

    if (evt_idx == -1) {
        DBG_WARN(("Run out the event queue resource !\n"));
        return -1;
    }
    for (i = 0; i < kernel_bde->num_devices(BDE_ALL_DEVICES); i++) {
        if (lkbde_is_dev_managed_by_instance(i, inst_id)) {
            sinfo = bkn_sinfo_from_unit(i);
            if (sinfo) {
                spin_lock_irqsave(&sinfo->lock, flags);
                sinfo->evt_idx = evt_idx;
                spin_unlock_irqrestore(&sinfo->lock, flags);
                DBG_INST(("%s d(%d) evt_idx %d \n",__FUNCTION__, i, evt_idx));
            } else {
                DBG_INST(("%s d(%d) evt_idx %d sinfo is NULL\n",
                          __FUNCTION__, i, evt_idx));
            }
        }
    }
    return 0;
}

/*
 * Device reprobe driven by application to check if new device is probed or
 * existed device is changed after inserting KNET module.
 */
static int
bkn_knet_dev_reprobe(void)
{
    bkn_switch_info_t *sinfo;
    int i;

    for (i = 0; i < kernel_bde->num_devices(BDE_ALL_DEVICES); i++) {
        sinfo = bkn_sinfo_from_unit(i);
        if (sinfo == NULL) {
            /* New device found after re-probe. */
            if (bkn_knet_dev_init(i) < 0) {
                return -1;
            }
        } else {
            /* Existed device reinit after re-probe. */
            if (bkn_knet_dev_reinit(i) < 0) {
                return -1;
            }
        }
    }
    return 0;
}

/* Assign the inst_id and evt_idx */
static int
bkn_knet_dev_inst_set(kcom_msg_reprobe_t *kmsg)
{
    bkn_switch_info_t *sinfo;
    int d = kmsg->hdr.unit;
    uint32 inst = INVALID_INSTANCE_ID;
    unsigned long flags;
    struct list_head *list;

    sinfo = bkn_sinfo_from_unit(d);
    if (sinfo == NULL) {
        return -1;
    }
#ifdef BCM_INSTANCE_SUPPORT
    lkbde_dev_instid_get(d, &inst);
#else
    inst = INVALID_INSTANCE_ID;
#endif
    DBG_INST(("%s sinfo->inst_id %d d %d inst %d\n",__FUNCTION__,sinfo->inst_id, d, inst));

    spin_lock_irqsave(&sinfo->lock, flags);
    if (sinfo->inst_id != inst) {
        /* Instance database changed, reinit the inst_id */
        sinfo->inst_id = INVALID_INSTANCE_ID;
        sinfo->evt_idx = -1;
    }
    spin_unlock_irqrestore(&sinfo->lock, flags);

    if (inst != INVALID_INSTANCE_ID) {
        if (sinfo->inst_id == INVALID_INSTANCE_ID) {
            if (bkn_create_inst(inst) != 0) {
                return -1;
            }
        }
        spin_lock_irqsave(&sinfo->lock, flags);
        sinfo->inst_id = inst;
        spin_unlock_irqrestore(&sinfo->lock, flags);
    } else {
        /* legacy mode */
        list_for_each(list, &_sinfo_list) {
            sinfo = (bkn_switch_info_t *)list;
            spin_lock_irqsave(&sinfo->lock, flags);
            sinfo->evt_idx = 0;
            sinfo->inst_id = INVALID_INSTANCE_ID;
            spin_unlock_irqrestore(&sinfo->lock, flags);
        }
    }
    return 0;
}

static int
bkn_knet_version(kcom_msg_version_t *kmsg, int len, int unit)
{
    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;
    kmsg->version = KCOM_VERSION;
    kmsg->netif_max = KCOM_NETIF_MAX;
    kmsg->filter_max = KCOM_FILTER_MAX;
    kmsg->module_reload = module_reload[unit];

    /*
     * The module_reoad idicator set while module inserted.
     * Unset the indicatore when SDK has checked KNET version to idicate
     * the KNET module has been initialized by SDK.
     */
    if (module_reload[unit]) {
        module_reload[unit] = 0;
    }
    return sizeof(kcom_msg_version_t);
}

/*
 * Get lock when sinfo->cfg_api_locked is not set.
 * This is used for protecting the resource which should not be freed/destroyed
 * when KNET functions temporarily release lock for calling outside functions.
 */
static inline void
cfg_api_lock(bkn_switch_info_t *sinfo, unsigned long *flags)
{
    spin_lock_irqsave(&sinfo->lock, *flags);
    while (sinfo->cfg_api_locked) {
        spin_unlock_irqrestore(&sinfo->lock, *flags);
        while (sinfo->cfg_api_locked) {
            schedule();
        }
        spin_lock_irqsave(&sinfo->lock, *flags);
    }
}

static inline void
cfg_api_unlock(bkn_switch_info_t *sinfo, unsigned long *flags)
{
    spin_unlock_irqrestore(&sinfo->lock, *flags);
}

static int
bkn_knet_hw_reset(kcom_msg_hw_reset_t *kmsg, int len)
{
    bkn_switch_info_t *sinfo;
    unsigned long flags;
    int chan;

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
    if (sinfo == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    if (sinfo->cmic_type == 0) {
        DBG_KCOM(("HW reset: Do nothing when CMIC architecture is undecided\n"));
        return sizeof(kcom_msg_hdr_t);
    }

    cfg_api_lock(sinfo, &flags);

    if (kmsg->channels == 0) {
        /* Clean all if no channels specified */
        bkn_dma_abort(sinfo);
        bkn_clean_dcbs(sinfo);
        skb_queue_purge(&sinfo->tx_ptp_queue);
    } else {
        if (kmsg->channels & (1 << XGS_DMA_TX_CHAN)) {
            bkn_dma_abort_tx(sinfo);
            bkn_clean_tx_dcbs(sinfo);
            skb_queue_purge(&sinfo->tx_ptp_queue);
        }
        for (chan = 0; chan < sinfo->rx_chans; chan++) {
            if (UNET_CH(sinfo, XGS_DMA_RX_CHAN + chan)) {
                continue;
            }
            if (kmsg->channels & (1 << (XGS_DMA_RX_CHAN + chan))) {
                bkn_dma_abort_rx(sinfo, chan);
                bkn_clean_rx_dcbs(sinfo, chan);
            }
        }
    }

    cfg_api_unlock(sinfo, &flags);

    return sizeof(kcom_msg_hdr_t);
}

static int
bkn_knet_hw_init(kcom_msg_hw_init_t *kmsg, int len)
{
    bkn_switch_info_t *sinfo;
    uint32_t dev_type, iproc_ver = 0;
    unsigned long flags;

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    DBG_DCB(("DCB size %d, type %d\n", kmsg->dcb_size, kmsg->dcb_type));

    sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
    if (sinfo == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    if ((kmsg->cmic_type == 'r' && kmsg->dcb_size < CMICR_DCB_SIZE_MIN) ||
        (kmsg->cmic_type == 'x' && kmsg->dcb_size < CMICX_DCB_SIZE_MIN) ||
        ((kmsg->cmic_type != 'x' && kmsg->cmic_type != 'r') && kmsg->dcb_size < DCB_SIZE_MIN) ||
        (kmsg->dcb_type != 39 && kmsg->cmic_type == 'x' && kmsg->pkt_hdr_size < CMICX_PKT_HDR_SIZE_MIN)) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    cfg_api_lock(sinfo, &flags);

    sinfo->cmic_type = kmsg->cmic_type;
    sinfo->dcb_type = kmsg->dcb_type;
    sinfo->dcb_wsize = BYTES2WORDS(kmsg->dcb_size);
    sinfo->pkt_hdr_size = kmsg->pkt_hdr_size;
    sinfo->dma_hi = kmsg->dma_hi;
    sinfo->rx_chans = sinfo->cmic_type == 'r' ? NUM_CMICR_RX_CHAN : (sinfo->cmic_type == 'x' ? NUM_CMICX_RX_CHAN : NUM_CMICM_RX_CHAN);
    if (sinfo->rx_chans > NUM_RX_CHAN) {
        sinfo->rx_chans = NUM_RX_CHAN;
    }

    DBG_DUNE(("CMIC:%c DCB:%d WSIZE:%d DMA HI: 0x%08x HDR size: %d\n",
        sinfo->cmic_type, sinfo->dcb_type, sinfo->dcb_wsize,
        sinfo->dma_hi, sinfo->pkt_hdr_size));

    /* Config user network channels */
    sinfo->unet_channels = kmsg->unet_channels & ~(~0 << (sinfo->rx_chans + 1));

    /* Config Continuous DMA mode */
    sinfo->cdma_channels = kmsg->cdma_channels & ~(~0 << (sinfo->rx_chans + 1));
    sinfo->cdma_channels = sinfo->cdma_channels & ~(sinfo->unet_channels);

    /* Ensure 32-bit PCI DMA is mapped properly on 64-bit platforms */
    dev_type = kernel_bde->get_dev_type(sinfo->dev_no);
    if (dev_type & BDE_PCI_DEV_TYPE && (sinfo->cmic_type != 'x' && sinfo->cmic_type != 'r')) {
        if (dma_set_mask(&sinfo->pdev->dev, DMA_BIT_MASK(32))) {
            cfg_api_unlock(sinfo, &flags);
            gprintk("No suitable DMA available for SKBs\n");
            kmsg->hdr.status = KCOM_E_RESOURCE;
            return sizeof(kcom_msg_hdr_t);
        }
    }

    /* First time called we need to allocate DCBs */
    if (sinfo->dcb_mem == NULL) {
        if (bkn_alloc_dcbs(sinfo) < 0) {
            cfg_api_unlock(sinfo, &flags);
            kmsg->hdr.status = KCOM_E_RESOURCE;
            return sizeof(kcom_msg_hdr_t);
        }
    }

    if (DEV_IS_CMICX(sinfo)) {
        if (dev_type & BDE_PCI_DEV_TYPE) {
            lkbde_iproc_version_get(sinfo->dev_no, &iproc_ver);
            if (iproc_ver == 16 || iproc_ver == 18 || iproc_ver == 19) {
                sinfo->irq_mask_reg = INTC_INTR_ENABLE_REG4r;
            } else {
                sinfo->irq_mask_reg = CMICX_IRQ_ENABr;
            }
        } else if (dev_type & BDE_AXI_DEV_TYPE) {
            sinfo->irq_mask_reg = IHOST_GIC_GIC400_GICD_ISENABLERN_5r;
        }
    }

    /* Ensure that we restart properly */
    bkn_dma_abort(sinfo);
    bkn_clean_dcbs(sinfo);

    if (basedev_suspend) {
        if (!netif_running(sinfo->dev)) {
            sinfo->basedev_suspended = 1;
        }
    }

    /* Ensure all interrupts are disabled, e.g. if warmbooting */
    dev_irq_mask_set(sinfo, 0);

    /* Register interrupt handler */
    kernel_bde->interrupt_connect(sinfo->dev_no | LKBDE_ISR2_DEV,
                                  bkn_isr, sinfo);

    /* Init DCBs */
    bkn_init_dcbs(sinfo);

    bkn_dma_init(sinfo);

    cfg_api_unlock(sinfo, &flags);

    return sizeof(kcom_msg_hdr_t);
}

static int
bkn_knet_hw_info(kcom_msg_hw_info_t *kmsg, int len)
{
    bkn_switch_info_t *sinfo;
    unsigned long flags;

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    DBG_KCOM(("HW INFO:-OAMP: %d {%d, %d, %d, %d}\n",
              kmsg->oamp_info.oamp_port_number,
              kmsg->oamp_info.oamp_ports[0],
              kmsg->oamp_info.oamp_ports[1],
              kmsg->oamp_info.oamp_ports[2],
              kmsg->oamp_info.oamp_ports[3]));

    sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
    if (sinfo == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    cfg_api_lock(sinfo, &flags);

    if (device_is_sand(sinfo)) {
        int idx = 0;

        /* Information to parser Dune system headers */
        sinfo->ftmh_lb_key_ext_size = kmsg->ftmh_lb_key_ext_size;
        sinfo->ftmh_stacking_ext_size = kmsg->ftmh_stacking_ext_size;
        sinfo->pph_base_size = kmsg->pph_base_size;
        for (idx = 0; idx < 8; idx++)
        {
            sinfo->pph_lif_ext_size[idx] = kmsg->pph_lif_ext_size[idx];
        }
        for (idx = 0; idx < 4; idx++)
        {
            sinfo->udh_length_type[idx] = kmsg->udh_length_type[idx];
        }
        sinfo->udh_size = kmsg->udh_size;
        sinfo->oamp_punt = kmsg->oamp_punted;
        sinfo->enet_channels = kmsg->enet_channels;
        sinfo->no_skip_udh_check = kmsg->no_skip_udh_check;
        sinfo->oam_dm_tod_exist = kmsg->oam_dm_tod_exist;
        sinfo->system_headers_mode = kmsg->system_headers_mode;
        sinfo->udh_enable = kmsg->udh_enable;

        if (kmsg->oamp_info.oamp_port_number > KCOM_HW_INFO_OAMP_PORT_MAX)
        {
            cfg_api_unlock(sinfo, &flags);
            gprintk("Number of OAMP ports(%d) is greater than the maximum value allowed(%d)\n",
                        kmsg->oamp_info.oamp_port_number,
                        KCOM_HW_INFO_OAMP_PORT_MAX);
            kmsg->hdr.status = KCOM_E_PARAM;
            return sizeof(kcom_msg_hdr_t);
        }
        sinfo->oamp_port_number = kmsg->oamp_info.oamp_port_number;
        for (idx = 0; idx < sinfo->oamp_port_number; idx++) {
            sinfo->oamp_ports[idx] = kmsg->oamp_info.oamp_ports[idx];
        }

        for (idx = 0; idx < sinfo->oamp_port_number; idx++) {
            DBG_DUNE(("DNX OAMP ports [%d/%d]: %d\n",
                      idx, sinfo->oamp_port_number, sinfo->oamp_ports[idx]));
        }
    }

    cfg_api_unlock(sinfo, &flags);

    return sizeof(kcom_msg_hdr_t);
}

static int
bkn_knet_detach(kcom_msg_detach_t *kmsg, int len)
{
    bkn_switch_info_t *sinfo;
    unsigned long flags;
    bkn_evt_resource_t *evt;

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;
    sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
    if (sinfo == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    cfg_api_lock(sinfo, &flags);
    if (sinfo->evt_idx != -1) {
        /* Create dummy event to unblock pending IOCTL */
        sinfo->dma_events |= KCOM_DMA_INFO_F_TX_DONE;
        evt = &_bkn_evt[sinfo->evt_idx];
        evt->evt_wq_put++;
        wake_up_interruptible(&evt->evt_wq);
    }
    cfg_api_unlock(sinfo, &flags);

    /* Ensure that we return a valid unit number */
    kmsg->hdr.unit = sinfo->dev_no;

    return sizeof(kcom_msg_detach_t);
}

static int
bkn_knet_reprobe(kcom_msg_reprobe_t *kmsg, int len)
{
    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    /* Support pci hot plug and multiple instance */
    if ((bkn_knet_dev_reprobe() < 0) ||
        (bkn_knet_dev_inst_set(kmsg) < 0)) {
        kmsg->hdr.status = KCOM_E_RESOURCE;
        return sizeof(kcom_msg_reprobe_t);
    }

    return sizeof(kcom_msg_reprobe_t);
}

static void
bkn_knet_netif_from_priv(kcom_netif_t *netif, bkn_priv_t *priv)
{
    memcpy(netif->macaddr, priv->dev->dev_addr, 6);
    memcpy(netif->name, priv->dev->name, KCOM_NETIF_NAME_MAX - 1);
    netif->vlan = priv->vlan;
    netif->type = priv->type;
    netif->id = priv->id;
    netif->flags = priv->flags;
    netif->cb_user_data = priv->cb_user_data;

    if (priv->port < 0) {
        netif->port = 0;
    } else {
        netif->port = priv->port;
    }
    netif->qnum = priv->qnum;
}

static int
bkn_knet_netif_create(kcom_msg_netif_create_t *kmsg, int len)
{
    bkn_switch_info_t *sinfo;
    struct net_device *dev;
    struct list_head *list;
    bkn_priv_t *priv, *lpriv;
    unsigned long flags;
    int found;
    uint8_t *ma;
    uint16_t id;
    int realloc;
    int rv = KCOM_E_NONE;
    bkn_netif_cb_t *netif_create_cb;

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    switch (kmsg->netif.type) {
    case KCOM_NETIF_T_VLAN:
    case KCOM_NETIF_T_PORT:
    case KCOM_NETIF_T_META:
        break;
    default:
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }
    sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
    if (sinfo == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }
    if (device_is_sand(sinfo) &&
        kmsg->netif.system_headers_size > KCOM_NETIF_SYSTEM_HEADERS_SIZE_MAX) {
        DBG_WARN(("The system_headers_size %d is beyond the maximum size %d. n",
                  kmsg->netif.system_headers_size,
                  KCOM_NETIF_SYSTEM_HEADERS_SIZE_MAX));
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    ma = kmsg->netif.macaddr;
    if ((ma[0] | ma[1] | ma[2] | ma[3] | ma[4] | ma[5]) == 0) {
        bkn_dev_mac[5]++;
        ma = bkn_dev_mac;
    }
    if ((dev = bkn_lookup_ndev(ma, kmsg->netif.name)) == NULL) {
        if ((dev = bkn_init_ndev(ma, kmsg->netif.name)) == NULL) {
            kmsg->hdr.status = KCOM_E_RESOURCE;
            return sizeof(kcom_msg_hdr_t);
        }
        priv = netdev_priv(dev);
        priv->dev = dev;
        priv->sinfo = sinfo;
        priv->type = kmsg->netif.type;
        priv->vlan = kmsg->netif.vlan;
        /* System headers are prepared at BCM API for Dune headers */
        if (device_is_sand(sinfo)) {
            int idx = 0;
            priv->system_headers_size = kmsg->netif.system_headers_size;
            for (idx = 0; idx < KCOM_NETIF_SYSTEM_HEADERS_SIZE_MAX; idx++)
            {
                priv->system_headers[idx] = kmsg->netif.system_headers[idx];
            }
        }
        if (priv->type == KCOM_NETIF_T_PORT) {
            priv->port = kmsg->netif.port;
            priv->phys_port = kmsg->netif.phys_port;
            priv->qnum = kmsg->netif.qnum;
            memset(&(priv->link_settings), 0, sizeof(struct ethtool_link_settings));
        } else {
            if (device_is_sand(sinfo) && (priv->type == KCOM_NETIF_T_VLAN)) {
                /* PTCH.SSPA */
                priv->port = kmsg->netif.port;
                priv->qnum = kmsg->netif.qnum;
            } else {
                priv->port = -1;
            }
        }
        priv->flags = kmsg->netif.flags;
        priv->cb_user_data = kmsg->netif.cb_user_data;

        /* Force RCPU encapsulation if rcpu_mode */
        if (rcpu_mode) {
            priv->flags |= KCOM_NETIF_F_RCPU_ENCAP;
            DBG_RCPU(("RCPU auto-enabled\n"));
        }
        priv->ref_count=1;
    } else {
        if ((kmsg->netif.flags & KCOM_NETIF_F_USE_SHARED_NDEV) == 0) {
            netdev_put(dev, NULL);
            kmsg->hdr.status = KCOM_E_RESOURCE;
            return sizeof(kcom_msg_hdr_t);
        }
        priv = netdev_priv(dev);
        priv->flags |= KCOM_NETIF_F_USE_SHARED_NDEV;
        priv->ref_count++;
    }

    id = kmsg->netif.id;
    /* Prevent (incorrect) compiler warning */
    lpriv = NULL;
    realloc = 0;

    spin_lock_irqsave(&sinfo->lock, flags);

    if (id > KCOM_NETIF_MAX) {
        DBG_NDEV(("ID %d exceeds maximum network interface ID (max: %d)\n",
                  id, KCOM_NETIF_MAX));
        rv = KCOM_E_PARAM;
    } else if (id == 0) {
        for (id = 1; id < sinfo->ndev_max; id++) {
            if (!sinfo->ndevs[id]) {
                break;
            }
        }
        if (id >= sinfo->ndev_max) {
            realloc = 1;
        }
    } else {
        if (id >= sinfo->ndev_max) {
            realloc = 1;
        } else {
            if (sinfo->ndevs[id]) {
                DBG_NDEV(("ID %d is already in use\n", id));
                rv = KCOM_E_RESOURCE;
            }
        }
    }

    if (realloc) {
        int ndev_max;
        int size;
        void *ndevs;

        ndev_max = sinfo->ndev_max;
        do {
            ndev_max += NDEVS_CHUNK;
        } while ((id + 1) > ndev_max);

        if (ndev_max >= KCOM_NETIF_MAX) {
            ndev_max = KCOM_NETIF_MAX + 1;
        }
        size = ndev_max * sizeof(struct net_device *);
        ndevs = kmalloc(size, GFP_ATOMIC);
        if (ndevs != NULL) {
            DBG_NDEV(("Reallocate netif table for ID %d\n", id));
            memset(ndevs, 0, size);
            if (sinfo->ndevs != NULL) {
                size = sinfo->ndev_max * sizeof(struct net_device *);
                memcpy(ndevs, sinfo->ndevs, size);
                kfree(sinfo->ndevs);
            }
            sinfo->ndevs = ndevs;
            sinfo->ndev_max = ndev_max;
        } else {
            rv = KCOM_E_RESOURCE;
        }
    }
    if (rv != KCOM_E_NONE) {
        spin_unlock_irqrestore(&sinfo->lock, flags);
        priv->ref_count--;
        if (priv->ref_count <= 0) {
            unregister_netdev(dev);
            free_netdev(dev);
        } else {
            netdev_put(dev, NULL);
        }
        kmsg->hdr.status = rv;
        return sizeof(kcom_msg_hdr_t);
    }
    sinfo->ndevs[id] = dev;

    if (priv->ref_count == 1) {
        priv->id = id;
        DBG_NDEV(("Add netif ID %d to table\n", id));

        found = 0;
        list_for_each(list, &sinfo->ndev_list) {
            lpriv = (bkn_priv_t *)list;
            if (id < lpriv->id) {
                found = 1;
                break;
            }
        }

        if (found) {
            /* Replace previously removed interface */
            list_add_tail(&priv->list, &lpriv->list);
        } else {
            /* No holes - add to end of list */
            list_add_tail(&priv->list, &sinfo->ndev_list);
        }
    } else {
        DBG_NDEV(("Use Shared Netif ID %d\n", id));
    }
    spin_unlock_irqrestore(&sinfo->lock, flags);

    if (priv->ref_count == 1) {
        DBG_VERB(("Assigned ID %d to Ethernet device %s\n",
                  priv->id, dev->name));

        kmsg->netif.id = priv->id;
        memcpy(kmsg->netif.macaddr, dev->dev_addr, 6);
        memcpy(kmsg->netif.name, dev->name, KCOM_NETIF_NAME_MAX - 1);
    }
    list_for_each(list, &netif_create_cb_list) {
        netif_create_cb = list_entry(list, bkn_netif_cb_t, list);
        if (netif_create_cb->cb(dev, sinfo->dev_no, &kmsg->netif) != 0) {
            DBG_WARN(("Network interface create callback failed for '%s'\n",
                      dev->name));
        }
    }

    return sizeof(*kmsg);
}

static int
bkn_knet_netif_destroy(kcom_msg_netif_destroy_t *kmsg, int len)
{
    bkn_switch_info_t *sinfo;
    struct net_device *dev;
    bkn_priv_t *priv;
    struct list_head *list;
    unsigned long flags;
    bkn_netif_cb_t *netif_destroy_cb;
    kcom_netif_t kcom_netif;

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
    if (sinfo == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    cfg_api_lock(sinfo, &flags);
    priv = bkn_netif_lookup(sinfo, kmsg->hdr.id);
    /*
     * If not found.
     */
    if (priv == NULL) {
        cfg_api_unlock(sinfo, &flags);
        kmsg->hdr.status = KCOM_E_NOT_FOUND;
        return sizeof(kcom_msg_hdr_t);
    }

    priv->ref_count--;

    if (priv->ref_count <= 0) {
        list_del(&priv->list);
    }

    if (priv->id < sinfo->ndev_max) {
        sinfo->ndevs[priv->id] = NULL;
    }

    cfg_api_unlock(sinfo, &flags);

    dev = priv->dev;

    if (!list_empty(&netif_destroy_cb_list)) {
        memset(&kcom_netif, 0, sizeof(kcom_netif));
        bkn_knet_netif_from_priv(&kcom_netif, priv);
        list_for_each(list, &netif_destroy_cb_list) {
            netif_destroy_cb = list_entry(list, bkn_netif_cb_t, list);
            if (netif_destroy_cb->cb(dev, sinfo->dev_no, &kcom_netif) != 0) {
                DBG_WARN(("Network interface destroy callback failed for '%s'\n",
                          dev->name));
            }
        }
    }

    if (priv->ref_count <= 0) {
        DBG_VERB(("Removing virtual Ethernet device %s (%d).\n",
                  dev->name, priv->id));

        unregister_netdev(dev);
        free_netdev(dev);
    } else {
        netdev_put(dev, NULL);
    }

    return sizeof(kcom_msg_hdr_t);
}

static int
bkn_knet_netif_list(kcom_msg_netif_list_t *kmsg, int len)
{
    bkn_switch_info_t *sinfo;
    bkn_priv_t *priv;
    struct list_head *list;
    unsigned long flags;
    int id, idx;

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
    if (sinfo == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    spin_lock_irqsave(&sinfo->lock, flags);

    idx = 0;
    if (sinfo->ndevs && sinfo->ndev_max > 0) {
        for (id = 0; id < sinfo->ndev_max; id++) {
            if (sinfo->ndevs[id] == NULL) {
                continue;
            }
            kmsg->id[idx] = id;
            idx++;
        }
    } else {
        list_for_each(list, &sinfo->ndev_list) {
            if (idx >= KCOM_NETIF_MAX) {
                DBG_WARN(("Too many network interfaces to list (max %d).\n",
                          KCOM_NETIF_MAX));
                break;
            }
            priv = (bkn_priv_t *)list;
            kmsg->id[idx] = priv->id;
            idx++;
        }
    }

    kmsg->ifcnt = idx;

    spin_unlock_irqrestore(&sinfo->lock, flags);

    return sizeof(*kmsg) - sizeof(kmsg->id) + (idx * sizeof(kmsg->id[0]));
}

static int
bkn_knet_netif_get(kcom_msg_netif_get_t *kmsg, int len)
{
    bkn_switch_info_t *sinfo;
    bkn_priv_t *priv;
    unsigned long flags;

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
    if (sinfo == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    spin_lock_irqsave(&sinfo->lock, flags);

    priv = bkn_netif_lookup(sinfo, kmsg->hdr.id);

    if (priv == NULL) {
        spin_unlock_irqrestore(&sinfo->lock, flags);
        kmsg->hdr.status = KCOM_E_NOT_FOUND;
        return sizeof(kcom_msg_hdr_t);
    }

    bkn_knet_netif_from_priv(&kmsg->netif, priv);

    spin_unlock_irqrestore(&sinfo->lock, flags);

    return sizeof(*kmsg);
}

static int
bkn_knet_filter_create(kcom_msg_filter_create_t *kmsg, int len)
{
    bkn_switch_info_t *sinfo;
    struct list_head *list;
    bkn_filter_t *filter, *lfilter;
    bkn_filter_cb_t *filter_cb;
    unsigned long flags;
    int found, id;
    int oob_offset_max;

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
    if (sinfo == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    switch (kmsg->filter.type) {
    case KCOM_FILTER_T_RX_PKT:
        break;
    default:
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    if (device_is_sand(sinfo)) {
        oob_offset_max = BKN_SAND_SCRATCH_DATA_SIZE * 4;
    } else if ((sinfo->cmic_type == 'x') || (sinfo->cmic_type == 'r')) {
        oob_offset_max = sinfo->pkt_hdr_size;
    } else {
        oob_offset_max = sinfo->dcb_wsize * 4;
    }

    /* Validate filter data offsets and sizes. */
    if (kmsg->filter.oob_data_size +
        kmsg->filter.pkt_data_size > KCOM_FILTER_BYTES_MAX ||
        kmsg->filter.oob_data_size +
        kmsg->filter.oob_data_offset > oob_offset_max ||
        kmsg->filter.pkt_data_size +
        kmsg->filter.pkt_data_offset > KNET_FILTER_RAW_MAX) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    spin_lock_irqsave(&sinfo->lock, flags);

    /*
     * Find available ID
     */
    found = 1;
    id = 0;
    while (found && ++id < KCOM_FILTER_MAX) {
        found = 0;
        list_for_each(list, &sinfo->rxpf_list) {
            lfilter = (bkn_filter_t *)list;
            if (id == lfilter->kf.id) {
                found = 1;
                break;
            }
        }
    }

    spin_unlock_irqrestore(&sinfo->lock, flags);

    if (found) {
        /* Too many filters */
        kmsg->hdr.status = KCOM_E_RESOURCE;
        return sizeof(kcom_msg_hdr_t);
    }
    filter = kmalloc(sizeof(*filter), GFP_KERNEL);
    if (filter == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }
    memset(filter, 0, sizeof(*filter));
    memcpy(&filter->kf, &kmsg->filter, sizeof(filter->kf));
    filter->kf.id = id;
    /* Check for filter-specific callback */
    if (filter->kf.dest_type == KCOM_DEST_T_CB && filter->kf.desc[0] != '\0') {
        list_for_each(list, &filter_cb_list) {
            filter_cb = list_entry(list, bkn_filter_cb_t, list);
            if (strcmp(filter->kf.desc, filter_cb->desc) == 0) {
                filter->cb = filter_cb->cb;
                break;
            }
        }
    }

    spin_lock_irqsave(&sinfo->lock, flags);

    /* Add according to priority */
    found = 0;
    list_for_each(list, &sinfo->rxpf_list) {
        lfilter = (bkn_filter_t *)list;
        if (filter->kf.priority < lfilter->kf.priority) {
            list_add_tail(&filter->list, &lfilter->list);
            found = 1;
            break;
        }
    }
    if (!found) {
        list_add_tail(&filter->list, &sinfo->rxpf_list);
    }

    kmsg->filter.id = filter->kf.id;

    spin_unlock_irqrestore(&sinfo->lock, flags);

    DBG_VERB(("Created filter ID %d (%s).\n",
              filter->kf.id, filter->kf.desc));
    if (device_is_sand(sinfo)) {
        int idx, wsize;
        wsize = BYTES2WORDS(filter->kf.oob_data_size + filter->kf.pkt_data_size);
        DBG_DUNE(("Filter: oob_data_size = %d pkt_data_size=%d wsize %d\n",
                  filter->kf.oob_data_size, filter->kf.pkt_data_size, wsize));
        for (idx = 0; idx < wsize; idx++)
        {
            DBG_DUNE(("OOB[%d]: 0x%08x [0x%08x]\n", idx, filter->kf.data.w[idx],
                      filter->kf.mask.w[idx]));
        }
        DBG_DUNE(("DNX system headers parameters:LB_KEY_EXT %d, STK_EXT %d, PPH_BASE %d, LIF_EXT %d %d %d, UDH_ENA %d, %d %d %d %d\n",
                  sinfo->ftmh_lb_key_ext_size, sinfo->ftmh_stacking_ext_size,
                  sinfo->pph_base_size, sinfo->pph_lif_ext_size[1],
                  sinfo->pph_lif_ext_size[2], sinfo->pph_lif_ext_size[3],
                  sinfo->udh_enable, sinfo->udh_length_type[0],
                  sinfo->udh_length_type[1], sinfo->udh_length_type[2],
                  sinfo->udh_length_type[3]));
    }
    return len;
}

static int
bkn_knet_filter_destroy(kcom_msg_filter_destroy_t *kmsg, int len)
{
    bkn_switch_info_t *sinfo;
    bkn_filter_t *filter;
    struct list_head *list;
    unsigned long flags;
    int found;

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
    if (sinfo == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    cfg_api_lock(sinfo, &flags);

    found = 0;
    list_for_each(list, &sinfo->rxpf_list) {
        filter = (bkn_filter_t *)list;
        if (kmsg->hdr.id == filter->kf.id) {
            found = 1;
            break;
        }
    }

    if (!found) {
        cfg_api_unlock(sinfo, &flags);
        kmsg->hdr.status = KCOM_E_NOT_FOUND;
        return sizeof(kcom_msg_hdr_t);
    }

    list_del(&filter->list);

    cfg_api_unlock(sinfo, &flags);

    DBG_VERB(("Removing filter ID %d.\n", filter->kf.id));
    kfree(filter);

    return sizeof(kcom_msg_hdr_t);
}

static int
bkn_knet_filter_list(kcom_msg_filter_list_t *kmsg, int len)
{
    bkn_switch_info_t *sinfo;
    bkn_filter_t *filter;
    struct list_head *list;
    unsigned long flags;
    int idx;

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
    if (sinfo == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    spin_lock_irqsave(&sinfo->lock, flags);

    idx = 0;
    list_for_each(list, &sinfo->rxpf_list) {
        if (idx >= KCOM_FILTER_MAX) {
            DBG_WARN(("Too many filters to list (max %d).\n",
                      KCOM_FILTER_MAX));
            break;
        }
        filter = (bkn_filter_t *)list;
        kmsg->id[idx] = filter->kf.id;
        idx++;
    }
    kmsg->fcnt = idx;

    spin_unlock_irqrestore(&sinfo->lock, flags);
    return sizeof(*kmsg) - sizeof(kmsg->id) + (idx * sizeof(kmsg->id[0]));
}

static int
bkn_knet_filter_get(kcom_msg_filter_get_t *kmsg, int len)
{
    bkn_switch_info_t *sinfo;
    bkn_filter_t *filter;
    struct list_head *list;
    unsigned long flags;
    int found;

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
    if (sinfo == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    spin_lock_irqsave(&sinfo->lock, flags);

    found = 0;
    list_for_each(list, &sinfo->rxpf_list) {
        filter = (bkn_filter_t *)list;
        if (kmsg->hdr.id == filter->kf.id) {
            found = 1;
            break;
        }
    }

    if (!found) {
        spin_unlock_irqrestore(&sinfo->lock, flags);
        kmsg->hdr.status = KCOM_E_NOT_FOUND;
        return sizeof(kcom_msg_hdr_t);
    }

    memcpy(&kmsg->filter, &filter->kf, sizeof(kmsg->filter));

    spin_unlock_irqrestore(&sinfo->lock, flags);

    return sizeof(*kmsg);
}

static int
bkn_knet_dbg_pkt_set(kcom_msg_dbg_pkt_set_t *kmsg, int len)
{
    dbg_pkt_enable = kmsg->enable;
    return sizeof(kcom_msg_dbg_pkt_set_t);
}

static int
bkn_knet_dbg_pkt_get(kcom_msg_dbg_pkt_get_t *kmsg, int len)
{
    kmsg->value = dbg_pkt_enable;
    return sizeof(kcom_msg_dbg_pkt_get_t);
}

static int
bkn_knet_wb_cleanup(kcom_msg_wb_cleanup_t *kmsg, int len)
{
    bkn_switch_info_t *sinfo;
    bkn_dcb_chain_t *dcb_chain;
    unsigned long flags;
    int chan;

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
    if (sinfo == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    cfg_api_lock(sinfo, &flags);

    for (chan = 0; chan < sinfo->rx_chans; chan++) {
        if (sinfo->rx[chan].api_dcb_chain) {
            DBG_DCB_RX(("Freeing active Rx%d DCB chain.\n", chan));
            kfree(sinfo->rx[chan].api_dcb_chain);
            sinfo->rx[chan].api_dcb_chain = NULL;
        }
        while (!list_empty(&sinfo->rx[chan].api_dcb_list)) {
            dcb_chain = list_entry(sinfo->rx[chan].api_dcb_list.next,
                                   bkn_dcb_chain_t, list);
            list_del(&dcb_chain->list);
            DBG_DCB_RX(("Freeing Rx%d DCB chain.\n", chan));
            kfree(dcb_chain);
        }
        sinfo->rx[chan].api_dcb_chain_end = NULL;
        sinfo->rx[chan].api_active = 0;
    }

    cfg_api_unlock(sinfo, &flags);

    return sizeof(kcom_msg_hdr_t);
}

static int
bkn_knet_pcie_link_status_set(kcom_msg_pcie_link_status_t*kmsg, int len)
{
    bkn_switch_info_t *sinfo;
    unsigned long flags;

    kmsg->hdr.type = KCOM_MSG_TYPE_RSP;

    sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
    if (sinfo == NULL) {
        kmsg->hdr.status = KCOM_E_PARAM;
        return sizeof(kcom_msg_hdr_t);
    }

    cfg_api_lock(sinfo, &flags);
    sinfo->pcie_link_status = kmsg->pcie_link_status;
    cfg_api_unlock(sinfo, &flags);

    return sizeof(kcom_msg_hdr_t);
}

static int
bkn_handle_cmd_req(kcom_msg_t *kmsg, int len)
{
    /* Silently drop events and unrecognized message types */
    if (kmsg->hdr.type != KCOM_MSG_TYPE_CMD) {
        if (kmsg->hdr.opcode == KCOM_M_STRING) {
            DBG_VERB(("Debug string: '%s'\n", kmsg->string.val));
            return 0;
        }
        DBG_WARN(("Unsupported message (type=%d, opcode=%d)\n",
                  kmsg->hdr.type, kmsg->hdr.opcode));
        return 0;
    }

    switch (kmsg->hdr.opcode) {
    case KCOM_M_DMA_INFO:
        DBG_CMD(("KCOM_M_DMA_INFO\n"));
        /* Packet buffer */
        len = bkn_knet_dma_info(&kmsg->dma_info, len);
        break;
    case KCOM_M_VERSION:
        DBG_CMD(("KCOM_M_VERSION\n"));
        /* Return procotol version */
        len = bkn_knet_version(&kmsg->version, len, kmsg->hdr.unit);
        break;
    case KCOM_M_HW_RESET:
        DBG_CMD(("KCOM_M_HW_RESET\n"));
        /* Shut down DMA and release buffers */
        len = bkn_knet_hw_reset(&kmsg->hw_reset, len);
        break;
    case KCOM_M_HW_INIT:
        DBG_CMD(("KCOM_M_HW_INIT\n"));
        /* Initialize DMA */
        len = bkn_knet_hw_init(&kmsg->hw_init, len);
        break;
    case KCOM_M_HW_INFO:
        DBG_CMD(("KCOM_M_HW_INFO\n"));
        /* Initialize HW info DB */
        len = bkn_knet_hw_info(&kmsg->hw_info, len);
        break;
    case KCOM_M_DETACH:
        DBG_CMD(("KCOM_M_DETACH\n"));
        /* Detach kernel module */
        len = bkn_knet_detach(&kmsg->detach, len);
        break;
    case KCOM_M_REPROBE:
        DBG_CMD(("KCOM_M_REPROBE\n"));
        /* Reprobe device */
        len = bkn_knet_reprobe(&kmsg->reprobe, len);
        break;
    case KCOM_M_NETIF_CREATE:
        DBG_CMD(("KCOM_M_NETIF_CREATE\n"));
        /* Create network interface */
        len = bkn_knet_netif_create(&kmsg->netif_create, len);
        break;
    case KCOM_M_NETIF_DESTROY:
        DBG_CMD(("KCOM_M_NETIF_DESTROY\n"));
        /* Destroy network interface */
        len = bkn_knet_netif_destroy(&kmsg->netif_destroy, len);
        break;
    case KCOM_M_NETIF_LIST:
        DBG_CMD(("KCOM_M_NETIF_LIST\n"));
        /* Return list of IDs of installed network interfaces */
        len = bkn_knet_netif_list(&kmsg->netif_list, len);
        break;
    case KCOM_M_NETIF_GET:
        DBG_CMD(("KCOM_M_NETIF_GET\n"));
        /* Return network interface info */
        len = bkn_knet_netif_get(&kmsg->netif_get, len);
        break;
    case KCOM_M_FILTER_CREATE:
        DBG_CMD(("KCOM_M_FILTER_CREATE\n"));
        /* Create packet filter */
        len = bkn_knet_filter_create(&kmsg->filter_create, len);
        break;
    case KCOM_M_FILTER_DESTROY:
        DBG_CMD(("KCOM_M_FILTER_DESTROY\n"));
        /* Destroy packet filter */
        len = bkn_knet_filter_destroy(&kmsg->filter_destroy, len);
        break;
    case KCOM_M_FILTER_LIST:
        DBG_CMD(("KCOM_M_FILTER_LIST\n"));
        /* Return list of IDs of installed packet filters */
        len = bkn_knet_filter_list(&kmsg->filter_list, len);
        break;
    case KCOM_M_FILTER_GET:
        DBG_CMD(("KCOM_M_FILTER_GET\n"));
        /* Return packet filter info */
        len = bkn_knet_filter_get(&kmsg->filter_get, len);
        break;
    case KCOM_M_DBGPKT_SET:
        DBG_CMD(("KCOM_M_DBGPKT_SET\n"));
        /* Set debugging packet function */
        len = bkn_knet_dbg_pkt_set(&kmsg->dbg_pkt_set, len);
        break;
    case KCOM_M_DBGPKT_GET:
        DBG_CMD(("KCOM_M_DBGPKT_GET\n"));
        /* Get debugging packet function info */
        len = bkn_knet_dbg_pkt_get(&kmsg->dbg_pkt_get, len);
        break;
    case KCOM_M_WB_CLEANUP:
        DBG_CMD(("KCOM_M_WB_CLEANUP\n"));
        /* Clean up for warmbooting */
        len = bkn_knet_wb_cleanup(&kmsg->wb_cleanup, len);
        break;
    case KCOM_M_CLOCK_CMD:
        /* PHC clock control*/
        if (knet_hw_tstamp_ioctl_cmd_cb) {
            bkn_switch_info_t *sinfo;
            sinfo = bkn_sinfo_from_unit(kmsg->hdr.unit);
            if (sinfo == NULL) {
                /* The device is not probed or initialized yet.*/
                return 0;
            }
            DBG_CMD(("KCOM_M_CLOCK_CMD\n"));
            len = knet_hw_tstamp_ioctl_cmd_cb(&kmsg->clock_cmd, len, sinfo->dcb_type);
        } else {
            DBG_WARN(("Unsupported command (type=%d, opcode=%d)\n",
                      kmsg->hdr.type, kmsg->hdr.opcode));
            kmsg->hdr.opcode = 0;
            len = sizeof(kcom_msg_hdr_t);
        }
        break;
    case KCOM_M_PCIE_LINK_STATUS:
        DBG_CMD(("KCOM_M_PCIE_LINK_STATUS\n"));
        /* Set PCIE link status */
        len = bkn_knet_pcie_link_status_set(&kmsg->pcie_link_status, len);
        break;
    default:
        DBG_WARN(("Unsupported command (type=%d, opcode=%d)\n",
                  kmsg->hdr.type, kmsg->hdr.opcode));
        kmsg->hdr.opcode = 0;
        len = sizeof(kcom_msg_hdr_t);
        break;
    }
    return len;
}

static int
bkn_get_next_dma_event(kcom_msg_dma_info_t *kmsg)
{
    static int last_dev_no = 0;
    bkn_switch_info_t *sinfo;
    unsigned long flags;
    int dev_no, dev_evt;
    bkn_evt_resource_t *evt;

    dev_evt = kmsg->hdr.unit;
    sinfo = bkn_sinfo_from_unit(dev_evt);
    if (sinfo == NULL) {
        /* The device is not probed or initialized yet.*/
        return 0;
    }
    if (sinfo->evt_idx == -1) {
        /* Event queue is not ready yet */
        return 0;
    }

    DBG_INST(("%s dev %d evt_idx %d\n",__FUNCTION__, dev_evt, sinfo->evt_idx));
    dev_no = last_dev_no;
    while (1) {
        dev_no++;
        sinfo = bkn_sinfo_from_unit(dev_no);
        if (sinfo == NULL) {
            dev_no = 0;
            sinfo = bkn_sinfo_from_unit(dev_no);
        }

        if (sinfo && (sinfo->inst_id != INVALID_INSTANCE_ID) &&
           (!lkbde_is_dev_managed_by_instance(dev_evt, sinfo->inst_id))) {
            DBG_INST((" %s skip dev(%d)\n",__FUNCTION__,dev_no));
            continue;
        }

        if (sinfo && sinfo->evt_idx == -1) {
            /* Event queue is not ready yet */
            continue;
        }
        if (sinfo && sinfo->dma_events) {
            DBG_EVT(("Next DMA events (0x%08x)\n", sinfo->dma_events));
            kmsg->hdr.unit = sinfo->dev_no;

            spin_lock_irqsave(&sinfo->lock, flags);
            kmsg->dma_info.flags = sinfo->dma_events;
            sinfo->dma_events = 0;
            spin_unlock_irqrestore(&sinfo->lock, flags);

            last_dev_no = dev_no;
            break;
        }

        if ((dev_no == last_dev_no) ||
            (_bkn_multi_inst && (dev_no == dev_evt))) {
            evt = &_bkn_evt[sinfo->evt_idx];
            DBG_INST(("dev_no %d dev_evt %d wait queue index %d\n",
                      dev_no, dev_evt, sinfo->evt_idx));
            wait_event_interruptible(evt->evt_wq,
                                     evt->evt_wq_get != evt->evt_wq_put);
            DBG_VERB(("Event thread wakeup\n"));

            /* Thread interrupted */
            if (signal_pending(current)) {
               return 0;
            }

            evt->evt_wq_get = evt->evt_wq_put;
        }
    }
    return sizeof(*kmsg);
}

static int
_cleanup(void)
{
    struct list_head *list;
    struct net_device *dev;
    bkn_filter_t *filter;
    bkn_priv_t *priv;
    bkn_switch_info_t *sinfo;
    unsigned long flags;
    bkn_netif_cb_t *netif_cb;
    bkn_filter_cb_t *filter_cb;

    /* Inidicate that we are shutting down */
    module_initialized = 0;

    bkn_proc_cleanup();
    remove_proc_entry("bcm/knet", NULL);
    remove_proc_entry("bcm", NULL);

    list_for_each(list, &_sinfo_list) {
        sinfo = (bkn_switch_info_t *)list;

        del_timer_sync(&sinfo->timer);
        del_timer_sync(&sinfo->rxtick);

        cfg_api_lock(sinfo, &flags);
        if (DEV_IS_CMIC(sinfo)) {
            bkn_dma_abort(sinfo);
            dev_irq_mask_set(sinfo, 0);
        }
        cfg_api_unlock(sinfo, &flags);

        DBG_IRQ(("Unregister ISR.\n"));
        kernel_bde->interrupt_disconnect(sinfo->dev_no | LKBDE_ISR2_DEV);

        if (use_napi) {
            while (sinfo->napi_poll_mode) {
                bkn_sleep(1);
            }
        }

        cfg_api_lock(sinfo, &flags);
        bkn_clean_dcbs(sinfo);
        skb_queue_purge(&sinfo->tx_ptp_queue);
        cfg_api_unlock(sinfo, &flags);
    }

    /* Destroy all switch devices */
    while (!list_empty(&_sinfo_list)) {
        sinfo = list_entry(_sinfo_list.next, bkn_switch_info_t, list);

        /* Destroy all associated Rx packet filters */
        while (!list_empty(&sinfo->rxpf_list)) {
            filter = list_entry(sinfo->rxpf_list.next, bkn_filter_t, list);
            list_del(&filter->list);
            DBG_VERB(("Removing filter ID %d.\n", filter->kf.id));
            kfree(filter);
        }

        /* Destroy all associated virtual net devices */
        while (!list_empty(&sinfo->ndev_list)) {
            priv = list_entry(sinfo->ndev_list.next, bkn_priv_t, list);
            list_del(&priv->list);
            dev = priv->dev;
            while(--priv->ref_count > 0) {
                netdev_put(dev, NULL);
            }
            DBG_VERB(("Removing virtual Ethernet device %s.\n", dev->name));
            unregister_netdev(dev);
            free_netdev(dev);
        }
        if (sinfo->ndevs != NULL) {
            kfree(sinfo->ndevs);
        }

        /* Destroy base net device */
        if (sinfo->dev) {
            DBG_VERB(("Removing Ethernet device %s.\n", sinfo->dev->name));
            unregister_netdev(sinfo->dev);
            free_netdev(sinfo->dev);
        }

        DBG_VERB(("Removing switch device.\n"));
        bkn_destroy_sinfo(sinfo);
    }

    /* Destroy any callback list which is not unregistered */
    while (!list_empty(&netif_create_cb_list)) {
        netif_cb = list_entry(netif_create_cb_list.next,
                              bkn_netif_cb_t, list);
        list_del(&netif_cb->list);
        kfree(netif_cb);
    }
    while (!list_empty(&netif_destroy_cb_list)) {
        netif_cb = list_entry(netif_destroy_cb_list.next,
                              bkn_netif_cb_t, list);
        list_del(&netif_cb->list);
        kfree(netif_cb);
    }
    while (!list_empty(&filter_cb_list)) {
        filter_cb = list_entry(filter_cb_list.next,
                               bkn_filter_cb_t, list);
        list_del(&filter_cb->list);
        kfree(filter_cb);
    }

    return 0;
}

static int
bkn_knet_dev_reinit(int d)
{
    bkn_switch_info_t *sinfo;
    uint32 dev_state;
    unsigned long flags;

    if (lkbde_dev_state_get(d, &dev_state) < 0) {
        return -1;
    }
    DBG_VERB(("%s dev %d dev_state %d\n",__FUNCTION__, d, dev_state));
    if (dev_state == BDE_DEV_STATE_CHANGED) {
        sinfo = bkn_sinfo_from_unit(d);
        if (sinfo) {
            cfg_api_lock(sinfo, &flags);
            sinfo->base_addr = lkbde_get_dev_virt(d);
            sinfo->dma_dev = lkbde_get_dma_dev(d);
            sinfo->pdev = lkbde_get_hw_dev(d);
            cfg_api_unlock(sinfo, &flags);
        } else {
            DBG_VERB(("%s dev %d sinfo is NULL\n",__FUNCTION__, d));
        }

        dev_state = 0;
        lkbde_dev_state_set(d, dev_state);
    }
    return 0;
}

static int
bkn_knet_dev_init(int d)
{
    uint32_t dev_type;
    struct net_device *dev;
    bkn_switch_info_t *sinfo;
    bkn_priv_t *priv;
    char *bdev_name;
    const ibde_dev_t *bde_dev;

    DBG_VERB(("%s dev %d\n",__FUNCTION__, d));
    /* Base network device name */
    bdev_name = "bcm%d";
    if (base_dev_name) {
        if (strlen(base_dev_name) < IFNAMSIZ) {
            bdev_name = base_dev_name;
        } else {
            DBG_WARN(("Base device name too long\n"));
        }
    }

    dev_type = kernel_bde->get_dev_type(d);
    DBG_VERB(("Found device type 0x%x\n", dev_type));
    if ((dev_type & BDE_SWITCH_DEV_TYPE) == 0) {
        DBG_WARN(("Not switch device - skipping\n"));
        return 0;
    }
    switch (dev_type & BDE_DEV_BUS_TYPE_MASK) {
    case BDE_PCI_DEV_TYPE:
    case BDE_ICS_DEV_TYPE:
    case BDE_AXI_DEV_TYPE:
        break;
    default:
        DBG_WARN(("Not PCI/ICS/AXI device - skipping\n"));
        return 0;
    }

    if ((sinfo = bkn_create_sinfo(d)) == NULL) {
        _cleanup();
        return -ENOMEM;
    }

    /* Initialize the cpu_no.*/
    if (dev_type & BDE_AXI_DEV_TYPE) {
        /* AXI device type implies the activated iProc iHost */
        sinfo->cpu_no = 1;
    }
    /* Initialize default RCPU signature */
    if ((bde_dev = kernel_bde->get_dev(d)) != NULL) {
        sinfo->rcpu_sig = bde_dev->device & ~0xf;
        sinfo->base_id = bde_dev->device & ~0xf;
        sinfo->dev_id = bde_dev->device;
        sinfo->rev_id = bde_dev->rev;
    }
    /* Check for override */
    if (rcpu_signature) {
        sinfo->rcpu_sig = rcpu_signature;
    }

    /* Create base virtual net device */
    bkn_dev_mac[5]++;
    if ((dev = bkn_init_ndev(bkn_dev_mac, bdev_name)) == NULL) {
        _cleanup();
        return -ENOMEM;
    } else {
        sinfo->dev = dev;
        priv = netdev_priv(dev);
        priv->dev = dev;
        priv->sinfo = sinfo;
        priv->vlan = 1;
        priv->port = -1;
        priv->id = -1;
        priv->ref_count = 1;
    }

    if (use_napi) {
        bkn_netif_napi_add(dev, &sinfo->napi, bkn_poll, napi_weight);
    }
    return 0;
}

static int
_init(void)
{
    int idx;
    int num_dev;
    int rv;
    bkn_evt_resource_t *evt;

    /* Connect to the kernel bde */
    if ((linux_bde_create(NULL, &kernel_bde) < 0) || kernel_bde == NULL) {
        return -ENODEV;
    }

    /* Randomize Lower 3 bytes of the MAC address (TESTING ONLY) */
    get_random_bytes(&bkn_dev_mac[3], 3);

    /* Check for user-supplied MAC address (recommended) */
    if (mac_addr != NULL && strlen(mac_addr) == 17) {
        for (idx = 0; idx < 6; idx++) {
            bkn_dev_mac[idx] = simple_strtoul(&mac_addr[idx*3], NULL, 16);
        }
        /* Do not allow multicast address */
        bkn_dev_mac[0] &= ~0x01;
    }

    /* Optional RCPU MAC addresses */
    if (rcpu_dmac != NULL && strlen(rcpu_dmac) == 17) {
        for (idx = 0; idx < 6; idx++) {
            bkn_rcpu_dmac[idx] = simple_strtoul(&rcpu_dmac[idx*3], NULL, 16);
        }
    }
    if (rcpu_smac != NULL && strlen(rcpu_smac) == 17) {
        for (idx = 0; idx < 6; idx++) {
            bkn_rcpu_smac[idx] = simple_strtoul(&rcpu_smac[idx*3], NULL, 16);
        }
    }

    /* NAPI implies that base device must be up before we can pass traffic */
    if (use_napi) {
        basedev_suspend = 1;
    }

    num_dev = kernel_bde->num_devices(BDE_ALL_DEVICES);
    for (idx = 0; idx < num_dev; idx++) {
        rv = bkn_knet_dev_init(idx);
        if (rv) {
            return rv;
        }
    }

    /* Initialize proc files */
    proc_mkdir("bcm", NULL);
    bkn_proc_root = proc_mkdir("bcm/knet", NULL);

    bkn_proc_init();

    /* Initialize event queue */
    for (idx = 0; idx < LINUX_BDE_MAX_DEVICES; idx++) {
        evt = &_bkn_evt[idx];
        memset(evt, 0, sizeof(bkn_evt_resource_t));
        evt->inst_id = INVALID_INSTANCE_ID;
        init_waitqueue_head(&evt->evt_wq);
    }

    module_initialized = 1;

    for (idx = 0; idx < LINUX_BDE_MAX_DEVICES; idx++) {
        module_reload[idx] = 1;
    }

    return 0;
}

static int
_ioctl(unsigned int cmd, unsigned long arg)
{
    bkn_ioctl_t io;
    kcom_msg_t kmsg;

    if (!module_initialized) {
        return -EFAULT;
    }

    if (copy_from_user(&io, (void*)arg, sizeof(io))) {
        return -EFAULT;
    }

    if (io.len > sizeof(kmsg)) {
        return -EINVAL;
    }

    io.rc = 0;

    switch(cmd) {
    case 0:
        if (io.len > 0) {
            if (copy_from_user(&kmsg, (void *)(unsigned long)io.buf, io.len)) {
                return -EFAULT;
            }
            ioctl_cmd++;
            io.len = bkn_handle_cmd_req(&kmsg, io.len);
            ioctl_cmd--;
        } else {
            memset(&kmsg, 0, sizeof(kcom_msg_t));
            /*
             * Retrive the kmsg.hdr.unit from user space. The dma event queue
             * selection is based the instance derived from unit.
             */
            if (copy_from_user(&kmsg, (void *)(unsigned long)io.buf, sizeof(kmsg))) {
                return -EFAULT;
            }
            kmsg.hdr.type = KCOM_MSG_TYPE_EVT;
            kmsg.hdr.opcode = KCOM_M_DMA_INFO;
            ioctl_evt++;
            io.len = bkn_get_next_dma_event((kcom_msg_dma_info_t *)&kmsg);
            ioctl_evt--;
        }
        if (io.len > 0) {
            if (copy_to_user((void *)(unsigned long)io.buf, &kmsg, io.len)) {
                return -EFAULT;
            }
        }
        break;
    default:
        gprintk("Invalid IOCTL");
        io.rc = -1;
        break;
    }

    if (copy_to_user((void*)arg, &io, sizeof(io))) {
        return -EFAULT;
    }

    return 0;
}

static gmodule_t _gmodule = {
    .name = MODULE_NAME,
    .major = MODULE_MAJOR,
    .init = _init,
    .cleanup = _cleanup,
    .pprint = _pprint,
    .ioctl = _ioctl,
    .open = NULL,
    .close = NULL,
};

gmodule_t *
gmodule_get(void)
{
    EXPORT_NO_SYMBOLS;
    return &_gmodule;
}

/*
 * Call-back interfaces for other Linux kernel drivers.
 *
 * The Rx call-back allows an external module to modify SKB contents
 * before it is handed off to the Linux network stack.
 *
 * The Tx call-back allows an external module to modify SKB contents
 * before it is injected inot the switch.
 *
 * The HW timestamp callbacks invoke an external module to enable, disable
 * HW timestamp on a specific port which is indicated while the netif is
 * created through KNET API. KNET can also get device-specific SOBMH and
 * timestamp for a Tx PTP packet through these callbacks.
 */

int
bkn_rx_skb_cb_register(knet_skb_cb_f rx_cb)
{
    if (knet_rx_cb != NULL) {
        return -1;
    }
    knet_rx_cb = rx_cb;
    return 0;
}

int
bkn_rx_skb_cb_unregister(knet_skb_cb_f rx_cb)
{
    if (rx_cb != NULL && knet_rx_cb != rx_cb) {
        return -1;
    }
    knet_rx_cb = NULL;
    return 0;
}

int
bkn_tx_skb_cb_register(knet_skb_cb_f tx_cb)
{
    if (knet_tx_cb != NULL) {
        return -1;
    }
    knet_tx_cb = tx_cb;
    return 0;
}

int
bkn_tx_skb_cb_unregister(knet_skb_cb_f tx_cb)
{
    if (tx_cb != NULL && knet_tx_cb != tx_cb) {
        return -1;
    }
    knet_tx_cb = NULL;
    return 0;
}

int
bkn_netif_create_cb_register(knet_netif_cb_f netif_cb)
{
    struct list_head *list;
    bkn_netif_cb_t *netif_create_cb;

    if (netif_cb == NULL) {
        return -1;
    }
    list_for_each(list, &netif_create_cb_list) {
        netif_create_cb = list_entry(list, bkn_netif_cb_t, list);
        if (netif_create_cb->cb == netif_cb) {
            return -1;
        }
    }
    netif_create_cb = kmalloc(sizeof(*netif_create_cb), GFP_KERNEL);
    if (netif_create_cb == NULL) {
        return -1;
    }
    netif_create_cb->cb = netif_cb;
    list_add_tail(&netif_create_cb->list, &netif_create_cb_list);
    return 0;
}

int
bkn_netif_create_cb_unregister(knet_netif_cb_f netif_cb)
{
    struct list_head *list, *list_next;
    bkn_netif_cb_t *netif_create_cb;
    int found = 0;

    if (netif_cb == NULL) {
        return -1;
    }
    list_for_each_safe(list, list_next, &netif_create_cb_list) {
        netif_create_cb = list_entry(list, bkn_netif_cb_t, list);
        if (netif_create_cb->cb == netif_cb) {
            found = 1;
            list_del(list);
            break;
        }
    }
    if (!found) {
        return -1;
    }
    kfree(netif_create_cb);
    return 0;
}

int
bkn_netif_destroy_cb_register(knet_netif_cb_f netif_cb)
{
    struct list_head *list;
    bkn_netif_cb_t *netif_destroy_cb;

    if (netif_cb == NULL) {
        return -1;
    }
    list_for_each(list, &netif_destroy_cb_list) {
        netif_destroy_cb = list_entry(list, bkn_netif_cb_t, list);
        if (netif_destroy_cb->cb == netif_cb) {
            return -1;
        }
    }
    netif_destroy_cb = kmalloc(sizeof(*netif_destroy_cb), GFP_KERNEL);
    if (netif_destroy_cb == NULL) {
        return -1;
    }
    netif_destroy_cb->cb = netif_cb;
    list_add_tail(&netif_destroy_cb->list, &netif_destroy_cb_list);
    return 0;
}

int
bkn_netif_destroy_cb_unregister(knet_netif_cb_f netif_cb)
{
    struct list_head *list, *list_next;
    bkn_netif_cb_t *netif_destroy_cb;
    int found = 0;

    if (netif_cb == NULL) {
        return -1;
    }
    list_for_each_safe(list, list_next, &netif_destroy_cb_list) {
        netif_destroy_cb = list_entry(list, bkn_netif_cb_t, list);
        if (netif_destroy_cb->cb == netif_cb) {
            found = 1;
            list_del(list);
            break;
        }
    }
    if (!found) {
        return -1;
    }
    kfree(netif_destroy_cb);
    return 0;
}

int
bkn_filter_cb_register(knet_filter_cb_f filter_cb)
{
    if (knet_filter_cb != NULL) {
        return -1;
    }
    knet_filter_cb = filter_cb;
    return 0;
}

int
bkn_filter_cb_register_by_name(knet_filter_cb_f filter_cb, char *filter_name)
{
    struct list_head *list, *list2;
    bkn_switch_info_t *sinfo;
    bkn_filter_t *filter;
    bkn_filter_cb_t *fcb;
    unsigned long flags;

    if (filter_cb == NULL || filter_name == NULL) {
        return -1;
    }
    if (filter_name[0] == '\0' || strlen(filter_name) >= KCOM_FILTER_DESC_MAX) {
        return -1;
    }

    list_for_each(list, &filter_cb_list) {
        fcb = list_entry(list, bkn_filter_cb_t, list);
        if (strcmp(fcb->desc, filter_name) == 0) {
            return -1;
        }
    }
    fcb = kmalloc(sizeof(*fcb), GFP_KERNEL);
    if (fcb == NULL) {
        return -1;
    }
    fcb->cb = filter_cb;
    strlcpy(fcb->desc, filter_name, sizeof(fcb->desc));
    list_add_tail(&fcb->list, &filter_cb_list);

    /* Check if any existing filter matches the registered name */
    list_for_each(list, &_sinfo_list) {
        sinfo = list_entry(list, bkn_switch_info_t, list);
        spin_lock_irqsave(&sinfo->lock, flags);

        list_for_each(list2, &sinfo->rxpf_list) {
            filter = list_entry(list2, bkn_filter_t, list);
            if (filter->kf.dest_type != KCOM_DEST_T_CB) {
                continue;
            }
            if (filter->kf.desc[0] != '\0') {
                if (strcmp(filter->kf.desc, filter_name) == 0) {
                    filter->cb = filter_cb;
                }
            }
        }

        spin_unlock_irqrestore(&sinfo->lock, flags);
    }
    return 0;
}

int
bkn_filter_cb_unregister(knet_filter_cb_f filter_cb)
{
    struct list_head *list, *list2;
    bkn_switch_info_t *sinfo;
    bkn_filter_t *filter;
    bkn_filter_cb_t *fcb;
    unsigned long flags;
    int found = 0;

    /* Check if the any existing filter-specific callback matches */
    if (filter_cb) {
        /* Remove from list */
        list_for_each_safe(list, list2, &filter_cb_list) {
            fcb = list_entry(list, bkn_filter_cb_t, list);
            if (fcb->cb == filter_cb) {
                found = 1;
                list_del(&fcb->list);
                kfree(fcb);
                break;
            }
        }
        /* Check if the callback is set to filters */
        if (found) {
            list_for_each(list, &_sinfo_list) {
                sinfo = list_entry(list, bkn_switch_info_t, list);
                spin_lock_irqsave(&sinfo->lock, flags);

                list_for_each(list2, &sinfo->rxpf_list) {
                    filter = list_entry(list2, bkn_filter_t, list);
                    if (filter->kf.dest_type != KCOM_DEST_T_CB) {
                        continue;
                    }
                    if (filter->cb == filter_cb) {
                        filter->cb = NULL;
                    }
                }

                spin_unlock_irqrestore(&sinfo->lock, flags);
            }
        }
    }

    if (!found && filter_cb != NULL && knet_filter_cb != filter_cb) {
        return -1;
    }
    if (!found || filter_cb == knet_filter_cb) {
        knet_filter_cb = NULL;
    }
    return 0;
}

int
bkn_hw_tstamp_enable_cb_register(knet_hw_tstamp_enable_cb_f hw_tstamp_enable_cb)
{
    if (knet_hw_tstamp_enable_cb != NULL) {
        return -1;
    }
    knet_hw_tstamp_enable_cb = hw_tstamp_enable_cb;
    return 0;
}

int
bkn_hw_tstamp_enable_cb_unregister(knet_hw_tstamp_enable_cb_f hw_tstamp_enable_cb)
{
    if (hw_tstamp_enable_cb == NULL ||
        knet_hw_tstamp_enable_cb != hw_tstamp_enable_cb) {
        return -1;
    }
    knet_hw_tstamp_enable_cb = NULL;
    return 0;
}

int
bkn_hw_tstamp_disable_cb_register(knet_hw_tstamp_enable_cb_f hw_tstamp_disable_cb)
{
    if (knet_hw_tstamp_disable_cb != NULL) {
        return -1;
    }
    knet_hw_tstamp_disable_cb = hw_tstamp_disable_cb;
    return 0;
}

int
bkn_hw_tstamp_disable_cb_unregister(knet_hw_tstamp_enable_cb_f hw_tstamp_disable_cb)
{
    if (hw_tstamp_disable_cb == NULL ||
        knet_hw_tstamp_disable_cb != hw_tstamp_disable_cb) {
        return -1;
    }
    knet_hw_tstamp_disable_cb = NULL;
    return 0;
}

int
bkn_hw_tstamp_tx_time_get_cb_register(knet_hw_tstamp_tx_time_get_cb_f hw_tstamp_tx_time_get_cb)
{
    if (knet_hw_tstamp_tx_time_get_cb != NULL) {
        return -1;
    }
    knet_hw_tstamp_tx_time_get_cb = hw_tstamp_tx_time_get_cb;
    return 0;
}

int
bkn_hw_tstamp_tx_time_get_cb_unregister(knet_hw_tstamp_tx_time_get_cb_f hw_tstamp_tx_time_get_cb)
{
    if (hw_tstamp_tx_time_get_cb == NULL ||
        knet_hw_tstamp_tx_time_get_cb != hw_tstamp_tx_time_get_cb) {
        return -1;
    }
    knet_hw_tstamp_tx_time_get_cb = NULL;
    return 0;
}

int
bkn_hw_tstamp_tx_meta_get_cb_register(knet_hw_tstamp_tx_meta_get_cb_f hw_tstamp_tx_meta_get_cb)
{
    if (knet_hw_tstamp_tx_meta_get_cb != NULL) {
        return -1;
    }
    knet_hw_tstamp_tx_meta_get_cb = hw_tstamp_tx_meta_get_cb;
    return 0;
}

int
bkn_hw_tstamp_tx_meta_get_cb_unregister(knet_hw_tstamp_tx_meta_get_cb_f hw_tstamp_tx_meta_get_cb)
{
    if (hw_tstamp_tx_meta_get_cb == NULL ||
        knet_hw_tstamp_tx_meta_get_cb != hw_tstamp_tx_meta_get_cb) {
        return -1;
    }
    knet_hw_tstamp_tx_meta_get_cb = NULL;
    return 0;
}

int
bkn_hw_tstamp_ptp_clock_index_cb_register(knet_hw_tstamp_ptp_clock_index_cb_f hw_tstamp_ptp_clock_index_cb)
{
    if (knet_hw_tstamp_ptp_clock_index_cb != NULL) {
        return -1;
    }
    knet_hw_tstamp_ptp_clock_index_cb = hw_tstamp_ptp_clock_index_cb;
    return 0;
}

int
bkn_hw_tstamp_ptp_clock_index_cb_unregister(knet_hw_tstamp_ptp_clock_index_cb_f hw_tstamp_ptp_clock_index_cb)
{
    if (hw_tstamp_ptp_clock_index_cb == NULL ||
        knet_hw_tstamp_ptp_clock_index_cb != hw_tstamp_ptp_clock_index_cb) {
        return -1;
    }
    knet_hw_tstamp_ptp_clock_index_cb = NULL;
    return 0;
}

int
bkn_hw_tstamp_rx_time_upscale_cb_register(knet_hw_tstamp_rx_time_upscale_cb_f hw_tstamp_rx_time_upscale_cb)
{
    if (knet_hw_tstamp_rx_time_upscale_cb != NULL) {
        return -1;
    }
    knet_hw_tstamp_rx_time_upscale_cb = hw_tstamp_rx_time_upscale_cb;
    return 0;
}

int
bkn_hw_tstamp_rx_time_upscale_cb_unregister(knet_hw_tstamp_rx_time_upscale_cb_f hw_tstamp_rx_time_upscale_cb)
{
    if (hw_tstamp_rx_time_upscale_cb == NULL ||
        knet_hw_tstamp_rx_time_upscale_cb != hw_tstamp_rx_time_upscale_cb) {
        return -1;
    }
    knet_hw_tstamp_rx_time_upscale_cb = NULL;
    return 0;
}

int
bkn_hw_tstamp_rx_pre_process_cb_register(knet_hw_tstamp_rx_pre_process_cb_f hw_tstamp_rx_pre_process_cb)
{
    if (knet_hw_tstamp_rx_pre_process_cb != NULL) {
        return -1;
    }
    knet_hw_tstamp_rx_pre_process_cb = hw_tstamp_rx_pre_process_cb;
    return 0;
}

int
bkn_hw_tstamp_rx_pre_process_cb_unregister(knet_hw_tstamp_rx_pre_process_cb_f hw_tstamp_rx_pre_process_cb)
{
    if (hw_tstamp_rx_pre_process_cb == NULL ||
        knet_hw_tstamp_rx_pre_process_cb != hw_tstamp_rx_pre_process_cb) {
        return -1;
    }
    knet_hw_tstamp_rx_pre_process_cb = NULL;
    return 0;
}


int
bkn_hw_tstamp_ioctl_cmd_cb_register(knet_hw_tstamp_ioctl_cmd_cb_f hw_tstamp_ioctl_cmd_cb)
{
    if (knet_hw_tstamp_ioctl_cmd_cb != NULL) {
        return -1;
    }
    knet_hw_tstamp_ioctl_cmd_cb = hw_tstamp_ioctl_cmd_cb;
    return 0;
}

int
bkn_hw_tstamp_ioctl_cmd_cb_unregister(knet_hw_tstamp_ioctl_cmd_cb_f hw_tstamp_ioctl_cmd_cb)
{
    if (knet_hw_tstamp_ioctl_cmd_cb == NULL ||
        knet_hw_tstamp_ioctl_cmd_cb != hw_tstamp_ioctl_cmd_cb) {
        return -1;
    }
    knet_hw_tstamp_ioctl_cmd_cb = NULL;
    return 0;
}

int
bkn_hw_tstamp_ptp_transport_get_cb_register(knet_hw_tstamp_ptp_transport_get_cb_f hw_tstamp_ptp_transport_get_cb)
{
    if (knet_hw_tstamp_ptp_transport_get_cb != NULL) {
        return -1;
    }
    knet_hw_tstamp_ptp_transport_get_cb = hw_tstamp_ptp_transport_get_cb;
    return 0;
}

int
bkn_hw_tstamp_ptp_transport_get_cb_unregister(knet_hw_tstamp_ptp_transport_get_cb_f hw_tstamp_ptp_transport_get_cb)
{
    if (knet_hw_tstamp_ptp_transport_get_cb == NULL ||
        knet_hw_tstamp_ptp_transport_get_cb != hw_tstamp_ptp_transport_get_cb) {
        return -1;
    }
    knet_hw_tstamp_ptp_transport_get_cb = NULL;
    return 0;
}

/*
 * Get HW device info
 */
int
bkn_hw_device_get(int dev_no, uint16_t *dev_id, uint8_t *rev_id)
{
    bkn_switch_info_t *sinfo;

    sinfo = bkn_sinfo_from_unit(dev_no);
    if (sinfo == NULL) {
        DBG_WARN(("Warning: unknown unit: %d\n", dev_no));
        return -1;
    }

    if (dev_id) {
        *dev_id = sinfo->dev_id;
    }
    if (rev_id) {
        *rev_id = sinfo->rev_id;
    }

    return 0;
}

LKM_EXPORT_SYM(bkn_rx_skb_cb_register);
LKM_EXPORT_SYM(bkn_rx_skb_cb_unregister);
LKM_EXPORT_SYM(bkn_tx_skb_cb_register);
LKM_EXPORT_SYM(bkn_tx_skb_cb_unregister);
LKM_EXPORT_SYM(bkn_netif_create_cb_register);
LKM_EXPORT_SYM(bkn_netif_create_cb_unregister);
LKM_EXPORT_SYM(bkn_netif_destroy_cb_register);
LKM_EXPORT_SYM(bkn_netif_destroy_cb_unregister);
LKM_EXPORT_SYM(bkn_filter_cb_register);
LKM_EXPORT_SYM(bkn_filter_cb_register_by_name);
LKM_EXPORT_SYM(bkn_filter_cb_unregister);
LKM_EXPORT_SYM(bkn_hw_tstamp_enable_cb_register);
LKM_EXPORT_SYM(bkn_hw_tstamp_enable_cb_unregister);
LKM_EXPORT_SYM(bkn_hw_tstamp_disable_cb_register);
LKM_EXPORT_SYM(bkn_hw_tstamp_disable_cb_unregister);
LKM_EXPORT_SYM(bkn_hw_tstamp_tx_time_get_cb_register);
LKM_EXPORT_SYM(bkn_hw_tstamp_tx_time_get_cb_unregister);
LKM_EXPORT_SYM(bkn_hw_tstamp_tx_meta_get_cb_register);
LKM_EXPORT_SYM(bkn_hw_tstamp_tx_meta_get_cb_unregister);
LKM_EXPORT_SYM(bkn_hw_tstamp_ptp_clock_index_cb_register);
LKM_EXPORT_SYM(bkn_hw_tstamp_ptp_clock_index_cb_unregister);
LKM_EXPORT_SYM(bkn_hw_tstamp_rx_time_upscale_cb_register);
LKM_EXPORT_SYM(bkn_hw_tstamp_rx_time_upscale_cb_unregister);
LKM_EXPORT_SYM(bkn_hw_tstamp_rx_pre_process_cb_register);
LKM_EXPORT_SYM(bkn_hw_tstamp_rx_pre_process_cb_unregister);
LKM_EXPORT_SYM(bkn_hw_tstamp_ioctl_cmd_cb_register);
LKM_EXPORT_SYM(bkn_hw_tstamp_ioctl_cmd_cb_unregister);
LKM_EXPORT_SYM(bkn_hw_tstamp_ptp_transport_get_cb_register);
LKM_EXPORT_SYM(bkn_hw_tstamp_ptp_transport_get_cb_unregister);
LKM_EXPORT_SYM(bkn_hw_device_get);
