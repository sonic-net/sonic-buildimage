/*! \file bcmgenl.h
 *
 * BCMGENL module entry.
 *
 */
/*
 *
 * Copyright 2018-2025 Broadcom. All rights reserved.
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

#ifndef BCMGENL_H
#define BCMGENL_H

#include <lkm/lkm.h>
#include <linux/netdevice.h>
#include <linux/time.h>

/*! Max length of proc path */
#define PROCFS_MAX_PATH         1024

/*! Module information */
#define BCMGENL_MODULE_NAME     "linux_bcmgenl"

#define BCM_PROCFS_NAME        "bcm"
#define BCMGENL_PROCFS_NAME    "genl"
#define BCMGENL_PROCFS_PATH    (BCM_PROCFS_NAME "/" BCMGENL_PROCFS_NAME)
/*! set GENL_DEBUG for debug info */
#define GENL_DEBUG
#define GENL_DBG_LVL_VERB       0x0001
#define GENL_DBG_LVL_PDMP       0x0002
#define GENL_DBG_LVL_WARN       0x0004

#ifdef GENL_DEBUG
#define GENL_DBG_VERB(...) if (debug & GENL_DBG_LVL_VERB) printk (__VA_ARGS__);
#define GENL_DBG_PDMP(...) if (debug & GENL_DBG_LVL_PDMP) printk (__VA_ARGS__);
#define GENL_DBG_WARN(...) if (debug & GENL_DBG_LVL_WARN) printk (__VA_ARGS__);
#else
#define GENL_DBG_VERB(...)
#define GENL_DBG_PDMP(...)
#define GENL_DBG_WARN(...)
#endif /* GENL_DEBUG */

typedef struct {
    uint8_t cmic_type;
    uint8_t dcb_type;
    uint8_t dcb_size;
    uint8_t pkt_hdr_size;
    uint32_t cdma_channels;
} knet_hw_info_t;

/*! generic netlink data per interface */
typedef struct {
    struct list_head list;
    struct net_device *dev;
    uint16_t id;
    uint32_t port;
    uint16_t vlan;
    uint16_t qnum;
    uint32_t sample_rate; /* sFlow sample rate */
    uint32_t sample_size; /* sFlow sample size */
} bcmgenl_netif_t;

/*! generic netlink interface info */
typedef struct {
    struct list_head netif_list;
    int netif_count;
    knet_hw_info_t hw;
    struct net *netns;
    spinlock_t lock;
} bcmgenl_info_t;

/*! Destination port type */
#define DSTPORT_TYPE_NONE    0
#define DSTPORT_TYPE_DISCARD 1
#define DSTPORT_TYPE_MC      2

/*! Sampling type */
#define SAMPLE_TYPE_NONE     0
/*! Ingress */
#define SAMPLE_TYPE_INGRESS  1
/*! Egress */
#define SAMPLE_TYPE_EGRESS   2
/*! Ingress or Egress */
#define SAMPLE_TYPE_INGEGR   3

/*! generic netlink packet metadata */
typedef struct bcmgenl_packet_meta_s {
    int ing_pp_port;
    int src_port;
    int dst_port;
    int dst_port_type; /* Destination port type */
    uint32_t trunk_id;
    uint64_t timestamp;
    /*
     * Tag status
     * 0x0(Untagged)
     * 0x1(Single inner-tag)
     * 0x2(Single outer-tag)
     * 0x3(Double tagged)
     */
    int tag_status;
    uint16_t proto;
    uint16_t vlan;
    int sample_type; /* Sampling type */
} bcmgenl_packet_meta_t;

/*! generic netlink packet info */
typedef struct bcmgenl_pkt_s {
    struct net *netns; /* net namespace */
    bcmgenl_packet_meta_t meta;
} bcmgenl_pkt_t;

/*!
 * \brief Dump skb buffer.
 *
 * \param [in] skb socket buffer.
 */
void dump_skb(struct sk_buff *skb);

/*!
 * \brief Dump generic netlink packet.
 *
 * \param [in] bcmgenl_pkt generic netlink packet.
 */
void dump_bcmgenl_pkt(bcmgenl_pkt_t *bcmgenl_pkt);

/*!
 * \brief Package packet to Generic Netlink packet format.
 *
 * \param [in] dev NGKNET device structure point.
 * \param [in] skb socket buffer.
 * \param [in] pkt packet data buffer.
 * \param [in] pkt_meta packet metadata buffer.
 * \param [in] bcmgenl_info Generic Netlink interface information
 *             structure point.
 * \param [out] bcmgenl_pkt Generic Netlink packet information
 *              structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmgenl_pkt_package(
    int dev,
    struct sk_buff *skb,
    bcmgenl_info_t *bcmgenl_info,
    bcmgenl_pkt_t *bcmgenl_pkt);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,17,0)
/* last should be static or global */
#define bcmgenl_limited_gprintk(last, ...) { \
  struct timeval tv; \
  do_gettimeofday(&tv); \
  if (tv.tv_sec != last) { \
    printk(__VA_ARGS__); \
    last = tv.tv_sec; \
  } \
}
#else
/* last should be static or global */
#define bcmgenl_limited_gprintk(last, ...) { \
  struct timespec64 ts; \
  ktime_get_real_ts64(&ts); \
  if (ts.tv_sec != last) { \
    printk(__VA_ARGS__); \
    last = ts.tv_sec; \
  } \
}
#endif /* KERNEL_VERSION(3,17,0) */

#endif /* BCMGENL_H */
