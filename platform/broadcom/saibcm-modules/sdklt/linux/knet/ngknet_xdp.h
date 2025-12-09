/*! \file ngknet_xdp.h
 *
 * NGKNET XDP_NATIVE driver header.
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

#ifndef NGKNET_XDP_H
#define NGKNET_XDP_H

#define NGKNET_XDP_PASS         0
#define NGKNET_XDP_DROP         (1 << 0)
#define NGKNET_XDP_TX           (1 << 1)
#define NGKNET_XDP_REDIR        (1 << 2)
#define NGKNET_XDP_BUSY         (1 << 3)
#define NGKNET_XDP_EXIT         (1 << 4)

#ifdef NGKNET_XDP_NATIVE

/*!
 * \brief Set up XDP.
 *
 * \param [in] ndev Network device structure.
 * \param [in] bpf BPF structure.
 */
extern int
ngknet_xdp_setup(struct net_device *ndev, struct netdev_bpf *bpf);

/*!
 * \brief Tx for XDP.
 *
 * \param [in] ndev Network device structure.
 * \param [in] frames XDP frame structure array.
 * \param [in] flags Tx flags.
 */
extern int
ngknet_xdp_xmit(struct net_device *ndev, int n, struct xdp_frame **frames,
                uint32_t flags);

/*!
 * \brief Run XDP program.
 *
 * \param [in] ndev Network device structure.
 * \param [in] xdp XDP buffer.
 */
extern int
ngknet_run_xdp(struct net_device *ndev, struct xdp_buff *xdp);

#endif /* NGKNET_XDP_NATIVE */

#endif /* NGKNET_XDP_H */
