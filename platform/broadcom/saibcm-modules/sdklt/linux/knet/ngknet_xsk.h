/*! \file ngknet_xsk.h
 *
 * NGKNET AF_XDP Zero-copy driver header.
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

#ifndef NGKNET_XSK_H
#define NGKNET_XSK_H

#ifdef NGKNET_XDP_NATIVE

/*!
 * \brief XSK frame.
 */
struct xsk_frame {
    /*! Data buffer */
    void *data;

    /*! Data length */
    uint32_t len;

    /*! Original descriptor, struct xdp_desc or struct xdp_frame */
    void *desc;
};

/*!
 * \brief Set up XSK buffer pool.
 *
 * \param [in] ndev Network device structure.
 * \param [in] pool XSK buffer poll structure.
 * \param [in] queue queue bound to XSK buffer bool.
 */
extern int
ngknet_xsk_pool_setup(struct net_device *ndev, struct xsk_buff_pool *pool,
                      uint32_t queue);

/*!
 * \brief Wake up Rx/Tx on queue.
 *
 * \param [in] ndev Network device structure.
 * \param [in] queue queue bound to XSK buffer bool.
 * \param [in] flags flags for Rx or Tx.
 */
extern int
ngknet_xsk_wakeup(struct net_device *ndev, uint32_t queue, uint32_t flags);

/*!
 * \brief XSK Tx by NAPI.
 *
 * \param [in] ndev NGKNET device structure.
 * \param [in] hdl Interrupt handler.
 * \param [in] budget budget for Rx or Tx.
 */
extern int
ngknet_xsk_napi_tx(struct ngknet_dev *dev, struct intr_handle *hdl, int budget);

/*!
 * \brief Run XDP program for XSK ZC.
 *
 * \param [in] ndev Network device structure.
 * \param [in] xdp XDP buffer.
 */
extern int
ngknet_run_xdp_zc(struct net_device *ndev, struct xdp_buff *xdp);

#endif /* NGKNET_XDP_NATIVE */

#endif /* NGKNET_XSK_H */
