/*! \file ngknet_buff.h
 *
 * Generic data structure definitions for NGKNET packet buffer management.
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

#ifndef NGKNET_BUFF_H
#define NGKNET_BUFF_H

/*! Rx buffer align size */
#define PDMA_RXB_ALIGN          32
/*! Rx buffer reserved size */
#define PDMA_RXB_RESV           (PDMA_RXB_ALIGN + PKT_HDR_SIZE)
/*! Rx SKB reserved size */
#define PDMA_SKB_RESV           (PDMA_RXB_RESV + SKB_DATA_ALIGN(sizeof(struct skb_shared_info)))
/*! Rx buffer size */
#define PDMA_RXB_SIZE(len)      (SKB_DATA_ALIGN(len) + PDMA_SKB_RESV)
/*! Rx reserved meta size */
#define PDMA_RXB_META           64

/*!
 * \brief Rx buffer.
 */
struct pdma_rx_buf {
    /*! DMA address */
    dma_addr_t dma;

    /*! Buffer page */
    struct page *page;

    /*! Buffer page offset */
    unsigned int page_offset;

    /*! Rx SKB */
    struct sk_buff *skb;

    /*! Packet buffer point */
    struct pkt_buf *pkb;

    /*! Packet buffer adjustment */
    uint32_t adj;
};

/*!
 * \brief Tx buffer.
 */
struct pdma_tx_buf {
    /*! DMA address */
    dma_addr_t dma;

    /*! Tx buffer length */
    uint32_t len;

    /*! Tx SKB */
    struct sk_buff *skb;

    /*! Packet buffer point */
    struct pkt_buf *pkb;

    /*! Packet buffer adjustment */
    uint32_t adj;
};

#endif /* NGKNET_BUFF_H */

