/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 1999 - 2026 Intel Corporation */

#ifndef _IXGBE_PTP_E600_H_
#define _IXGBE_PTP_E600_H_

#include <linux/types.h>

unsigned int ixgbe_ptp_rx_complete_skb_e600(struct ixgbe_q_vector *q_vector,
					    int *budget);
int ixgbe_ptp_set_timestamp_mode_e600(struct ixgbe_adapter *adapter,
				      struct hwtstamp_config *config);
void ixgbe_ptp_cfg_phy_vlan_e600(struct ixgbe_adapter *adapter, bool enabled);
bool ixgbe_ptp_is_tx_ptp(struct ixgbe_adapter *adapter, struct sk_buff *skb);
void ixgbe_ptp_link_up_e600(struct ixgbe_adapter *adapter);
void ixgbe_ptp_reset_e600(struct ixgbe_adapter *adapter);
void ixgbe_ptp_init_e600(struct ixgbe_adapter *adapter);
void ixgbe_ptp_release_e600(struct ixgbe_adapter *adapter);

#endif /* _IXGBE_PTP_E600_H_ */
