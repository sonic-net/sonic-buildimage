/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 1999 - 2026 Intel Corporation */

#ifndef _IXGBE_DEVLINK_H_
#define _IXGBE_DEVLINK_H_

#if IS_ENABLED(CONFIG_NET_DEVLINK)

struct ixgbe_adapter **ixgbe_allocate_devlink(struct ixgbe_adapter *adapter);

void ixgbe_devlink_register(struct ixgbe_adapter *adapter);
void ixgbe_devlink_unregister(struct ixgbe_adapter *adapter);

int ixgbe_devlink_register_params(struct ixgbe_adapter *adapter);
void ixgbe_devlink_unregister_params(struct ixgbe_adapter *adapter);

int ixgbe_devlink_register_port(struct ixgbe_adapter *adapter);
void ixgbe_devlink_unregister_port(struct ixgbe_adapter *adapter);

#else /* CONFIG_NET_DEVLINK */

struct ixgbe_adapter **ixgbe_allocate_devlink(struct ixgbe_adapter *adapter)
{
	return NULL
}

static inline void ixgbe_devlink_register(struct ixgbe_adapter *adapter) { }
static inline void ixgbe_devlink_unregister(struct ixgbe_adapter *adapter) { }

static inline int ixgbe_devlink_register_params(struct ixgbe_adapter *adapter) { return 0; }
static inline void ixgbe_devlink_unregister_params(struct ixgbe_adapter *adapter) { }

static inline int ixgbe_devlink_register_port(struct ixgbe_adapter *adapter) { return 0; }
static inline void ixgbe_devlink_unregister_port(struct ixgbe_adapter *adapter) { }

#endif /* !CONFIG_NET_DEVLINK */

#if IS_ENABLED(CONFIG_NET_DEVLINK) && defined(HAVE_DEVLINK_REGIONS)

void ixgbe_devlink_init_regions(struct ixgbe_adapter *adapter);
void ixgbe_devlink_destroy_regions(struct ixgbe_adapter *adapter);

#else

static inline void ixgbe_devlink_init_regions(struct ixgbe_adapter *adapter) { }
static inline void ixgbe_devlink_destroy_regions(struct ixgbe_adapter *adapter) { }

#endif

#endif /* _IXGBE_DEVLINK_H_ */
