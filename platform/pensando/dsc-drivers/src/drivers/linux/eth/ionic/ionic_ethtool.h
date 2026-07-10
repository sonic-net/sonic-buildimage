/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2017 - 2022 Pensando Systems, Inc */

#ifndef _IONIC_ETHTOOL_H_
#define _IONIC_ETHTOOL_H_

int ionic_cmb_pages_in_use(struct ionic_lif *lif);
void ionic_ethtool_set_ops(struct net_device *netdev);

#endif /* _IONIC_ETHTOOL_H_ */
