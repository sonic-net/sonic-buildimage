/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 1999 - 2026 Intel Corporation */

#ifndef _IXGBE_FW_UPDATE_H_
#define _IXGBE_FW_UPDATE_H_

int ixgbe_flash_pldm_image(struct devlink *devlink,
			   struct devlink_flash_update_params *params,
			   struct netlink_ext_ack *extack);
int ixgbe_get_pending_updates(struct ixgbe_adapter *adapter, u8 *pending,
			      struct netlink_ext_ack *extack);
int ixgbe_write_one_nvm_block(struct ixgbe_adapter *adapter,
			      u16 module, u32 offset,
			      u16 block_size, u8 *block, bool last_cmd,
			      u8 *reset_level,
			      struct netlink_ext_ack *extack);

#endif
