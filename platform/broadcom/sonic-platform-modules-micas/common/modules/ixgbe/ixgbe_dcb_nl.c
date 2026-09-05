/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (C) 1999 - 2026 Intel Corporation */

#include "ixgbe.h"

#if IS_ENABLED(CONFIG_DCB)
#include <linux/dcbnl.h>
#include "ixgbe_dcb_82598.h"
#include "ixgbe_dcb_82599.h"

/* Callbacks for DCB netlink in the kernel */
#define BIT_DCB_MODE	0x01
#define BIT_PFC		0x02
#define BIT_PG_RX	0x04
#define BIT_PG_TX	0x08
#define BIT_APP_UPCHG	0x10
#define BIT_RESETLINK	0x40
#define BIT_LINKSPEED	0x80

/* Responses for the DCB_C_SET_ALL command */
#define DCB_HW_CHG_RST	0  /* DCB configuration changed with reset */
#define DCB_NO_HW_CHG	1  /* DCB configuration did not change */
#define DCB_HW_CHG	2  /* DCB configuration changed, no reset */

int ixgbe_copy_dcb_cfg(struct ixgbe_adapter *adapter, int tc_max)
{
	struct ixgbe_dcb_config *scfg = &adapter->temp_dcb_cfg;
	struct ixgbe_dcb_config *dcfg = &adapter->dcb_cfg;
	struct ixgbe_dcb_tc_config *src = NULL;
	struct ixgbe_dcb_tc_config *dst = NULL;
	int i, j;
	int tx = IXGBE_DCB_TX_CONFIG;
	int rx = IXGBE_DCB_RX_CONFIG;
	int changes = 0;

#if IS_ENABLED(CONFIG_FCOE)
	if (adapter->fcoe.up_set != adapter->fcoe.up)
		changes |= BIT_APP_UPCHG;
#endif /* CONFIG_FCOE */

	for (i = DCB_PG_ATTR_TC_0; i < tc_max + DCB_PG_ATTR_TC_0; i++) {
		src = &scfg->tc_config[i - DCB_PG_ATTR_TC_0];
		dst = &dcfg->tc_config[i - DCB_PG_ATTR_TC_0];

		if (dst->path[tx].tsa != src->path[tx].tsa) {
			dst->path[tx].tsa = src->path[tx].tsa;
			changes |= BIT_PG_TX;
		}

		if (dst->path[tx].bwg_id != src->path[tx].bwg_id) {
			dst->path[tx].bwg_id = src->path[tx].bwg_id;
			changes |= BIT_PG_TX;
		}

		if (dst->path[tx].bwg_percent != src->path[tx].bwg_percent) {
			dst->path[tx].bwg_percent = src->path[tx].bwg_percent;
			changes |= BIT_PG_TX;
		}

		if (dst->path[tx].up_to_tc_bitmap !=
		    src->path[tx].up_to_tc_bitmap) {
			dst->path[tx].up_to_tc_bitmap =
				src->path[tx].up_to_tc_bitmap;
			changes |= (BIT_PG_TX | BIT_PFC | BIT_APP_UPCHG);
		}

		if (dst->path[rx].tsa != src->path[rx].tsa) {
			dst->path[rx].tsa = src->path[rx].tsa;
			changes |= BIT_PG_RX;
		}

		if (dst->path[rx].bwg_id != src->path[rx].bwg_id) {
			dst->path[rx].bwg_id = src->path[rx].bwg_id;
			changes |= BIT_PG_RX;
		}

		if (dst->path[rx].bwg_percent != src->path[rx].bwg_percent) {
			dst->path[rx].bwg_percent = src->path[rx].bwg_percent;
			changes |= BIT_PG_RX;
		}

		if (dst->path[rx].up_to_tc_bitmap !=
		    src->path[rx].up_to_tc_bitmap) {
			dst->path[rx].up_to_tc_bitmap =
				src->path[rx].up_to_tc_bitmap;
			changes |= (BIT_PG_RX | BIT_PFC | BIT_APP_UPCHG);
		}
	}

	for (i = DCB_PG_ATTR_BW_ID_0; i < DCB_PG_ATTR_BW_ID_MAX; i++) {
		j = i - DCB_PG_ATTR_BW_ID_0;

		if (dcfg->bw_percentage[tx][j] != scfg->bw_percentage[tx][j]) {
			dcfg->bw_percentage[tx][j] = scfg->bw_percentage[tx][j];
			changes |= BIT_PG_TX;
		}
		if (dcfg->bw_percentage[rx][j] != scfg->bw_percentage[rx][j]) {
			dcfg->bw_percentage[rx][j] = scfg->bw_percentage[rx][j];
			changes |= BIT_PG_RX;
		}
	}

	for (i = DCB_PFC_UP_ATTR_0; i < DCB_PFC_UP_ATTR_MAX; i++) {
		j = i - DCB_PFC_UP_ATTR_0;
		if (dcfg->tc_config[j].pfc != scfg->tc_config[j].pfc) {
			dcfg->tc_config[j].pfc = scfg->tc_config[j].pfc;
			changes |= BIT_PFC;
		}
	}

	if (dcfg->pfc_mode_enable != scfg->pfc_mode_enable) {
		dcfg->pfc_mode_enable = scfg->pfc_mode_enable;
		changes |= BIT_PFC;
	}

	return changes;
}

/**
 * ixgbe_dcbnl_get_state - Retrieve the DCB state of the network device
 * @netdev: Network device structure
 *
 * This function checks whether Data Center Bridging (DCB) is enabled on the
 * specified network device. It accesses the adapter's flags to determine the
 * DCB state and returns a boolean value indicating the result. The function
 * returns `1` if DCB is enabled and `0` otherwise.
 *
 * Return: A boolean value (`1` or `0`) indicating whether DCB is enabled.
 */
static u8 ixgbe_dcbnl_get_state(struct net_device *netdev)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	return !!(adapter->flags & IXGBE_FLAG_DCB_ENABLED);
}

/**
 * ixgbe_dcbnl_set_state - Set the DCB state of the network device
 * @netdev: Network device structure
 * @state: Desired state for DCB (0 to disable, non-zero to enable)
 *
 * This function sets the Data Center Bridging (DCB) state of the specified
 * network device. It checks if the device is in CEE (Converged Enhanced Ethernet)
 * mode and proceeds to update the DCB state if applicable.
 *
 * The function verifies if there is a change in the DCB state. If the desired
 * state matches the current state, the function exits without making changes.
 * Otherwise, it calls `ixgbe_setup_tc` to configure the traffic classes based
 * on the desired state. If enabling DCB, it uses the number of priority groups
 * configured in the device; if disabling, it sets the number of traffic classes
 * to zero.
 *
 * Return: 0 on success, or 1 if the device is not in CEE mode or if an error
 *         occurs during the process.
 */
static u8 ixgbe_dcbnl_set_state(struct net_device *netdev, u8 state)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	int err = 0;

	/* Fail command if not in CEE mode */
	if (!(adapter->dcbx_cap & DCB_CAP_DCBX_VER_CEE))
		return 1;

	/* verify there is something to do, if not then exit */
	if (!state == !(adapter->flags & IXGBE_FLAG_DCB_ENABLED))
		goto out;

	err = ixgbe_setup_tc(netdev,
			     state ? adapter->dcb_cfg.num_tcs.pg_tcs : 0);
out:
	return !!err;
}

/**
 * ixgbe_dcbnl_get_perm_hw_addr - Retrieve the permanent hardware address
 * @netdev: Network device structure
 * @perm_addr: Buffer to store the permanent hardware address
 *
 * This function retrieves the permanent hardware address (MAC address) of the
 * specified network device and stores it in the provided buffer `perm_addr`.
 * The function first initializes the buffer with a default value of 0xff for
 * each byte, then copies the permanent MAC address from the adapter's hardware
 * structure.
 *
 * For certain MAC types, such as ixgbe_mac_82599EB and ixgbe_mac_X540, the
 * function also appends the SAN (Storage Area Network) address to the buffer.
 * This is conditional based on the hardware type and specific compile-time
 * flags (e.g. MAGNOLIA_PARK_HW).
 *
 * This function is useful for obtaining the original hardware address of the
 * device, which may be needed for various network configuration and management
 * tasks.
 */
static void ixgbe_dcbnl_get_perm_hw_addr(struct net_device *netdev,
					 u8 *perm_addr)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	int i, j;

	memset(perm_addr, 0xff, MAX_ADDR_LEN);

	for (i = 0; i < netdev->addr_len; i++)
		perm_addr[i] = adapter->hw.mac.perm_addr[i];

	switch (adapter->hw.mac.type) {
	case ixgbe_mac_82599EB:
	case ixgbe_mac_X540:
	case ixgbe_mac_X550:
	case ixgbe_mac_E610:
		for (j = 0; j < netdev->addr_len; j++, i++)
			perm_addr[i] = adapter->hw.mac.san_addr[j];
		break;
	default:
		break;
	}
}

/**
 * ixgbe_dcbnl_set_pg_tc_cfg_tx - Set TX traffic class configuration
 * @netdev: Network device structure
 * @tc: Traffic class identifier for which to set the configuration
 * @prio: Priority assignment for the traffic class
 * @bwg_id: Bandwidth group identifier for the traffic class
 * @bw_pct: Bandwidth percentage for the traffic class
 * @up_map: User priority to traffic class mapping
 *
 * This function sets the transmit (TX) configuration for a specified traffic
 * class (TC) on the given network device. It updates the temporary DCB
 * configuration with the provided parameters, including the priority assignment
 * (`tsa`), bandwidth group identifier (`bwg_id`), bandwidth percentage
 * (`bwg_percent`), and user priority to traffic class mapping (`up_to_tc_bitmap`).
 *
 * The function first checks if the provided traffic class identifier is within
 * the valid range. If the identifier is out of range, it logs an error message
 * and returns without making any changes.
 *
 * Each parameter is only updated if it is not equal to `DCB_ATTR_VALUE_UNDEFINED`,
 * allowing for selective updates of the traffic class configuration.
 */
static void ixgbe_dcbnl_set_pg_tc_cfg_tx(struct net_device *netdev, int tc,
					 u8 prio, u8 bwg_id, u8 bw_pct,
					 u8 up_map)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	if (tc < 0 || tc >= IXGBE_DCB_MAX_TRAFFIC_CLASS) {
		netdev_err(netdev, "Traffic class out of range.\n");
		return;
	}

	if (prio != DCB_ATTR_VALUE_UNDEFINED)
		adapter->temp_dcb_cfg.tc_config[tc].path[0].tsa = prio;
	if (bwg_id != DCB_ATTR_VALUE_UNDEFINED)
		adapter->temp_dcb_cfg.tc_config[tc].path[0].bwg_id = bwg_id;
	if (bw_pct != DCB_ATTR_VALUE_UNDEFINED)
		adapter->temp_dcb_cfg.tc_config[tc].path[0].bwg_percent =
			bw_pct;
	if (up_map != DCB_ATTR_VALUE_UNDEFINED)
		adapter->temp_dcb_cfg.tc_config[tc].path[0].up_to_tc_bitmap =
			up_map;
}

/**
 * ixgbe_dcbnl_set_pg_bwg_cfg_tx - Set TX bandwidth percentage for a BWG
 * @netdev: Network device structure
 * @bwg_id: Bandwidth group identifier for which to set the configuration
 * @bw_pct: Bandwidth percentage to be set for the specified bandwidth group
 *
 * This function sets the transmit (TX) bandwidth percentage for a specified
 * bandwidth group (BWG) on the given network device. It updates the temporary
 * DCB configuration with the provided bandwidth percentage for the specified
 * BWG identifier.
 *
 * The function first checks if the provided BWG identifier is within the valid
 * range. If the identifier is out of range, it logs an error message and
 * returns without making any changes.
 */
static void ixgbe_dcbnl_set_pg_bwg_cfg_tx(struct net_device *netdev, int bwg_id,
					  u8 bw_pct)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	if (bwg_id < 0 || bwg_id >= IXGBE_DCB_MAX_BW_GROUP) {
		netdev_err(netdev,
			   "BWG index out of range.\n");
		return;
	}

	adapter->temp_dcb_cfg.bw_percentage[0][bwg_id] = bw_pct;
}

/**
 * ixgbe_dcbnl_set_pg_tc_cfg_rx - Set RX traffic class configuration
 * @netdev: Network device structure
 * @tc: Traffic class identifier for which to set the configuration
 * @prio: Priority assignment for the traffic class
 * @bwg_id: Bandwidth group identifier for the traffic class
 * @bw_pct: Bandwidth percentage for the traffic class
 * @up_map: User priority to traffic class mapping
 *
 * This function sets the receive (RX) configuration for a specified traffic
 * class (TC) on the given network device. It updates the temporary DCB
 * configuration with the provided parameters, including the priority assignment
 * (`tsa`), bandwidth group identifier (`bwg_id`), bandwidth percentage
 * (`bwg_percent`), and user priority to traffic class mapping (`up_to_tc_bitmap`).
 *
 * The function first checks if the provided traffic class identifier is within
 * the valid range. If the identifier is out of range, it logs an error message
 * and returns without making any changes.
 *
 * Each parameter is only updated if it is not equal to `DCB_ATTR_VALUE_UNDEFINED`,
 * allowing for selective updates of the traffic class configuration.
 */
static void ixgbe_dcbnl_set_pg_tc_cfg_rx(struct net_device *netdev, int tc,
					 u8 prio, u8 bwg_id, u8 bw_pct,
					 u8 up_map)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	if (tc < 0 || tc >= IXGBE_DCB_MAX_TRAFFIC_CLASS) {
		netdev_err(netdev, "Traffic class out of range.\n");
		return;
	}

	if (prio != DCB_ATTR_VALUE_UNDEFINED)
		adapter->temp_dcb_cfg.tc_config[tc].path[1].tsa = prio;
	if (bwg_id != DCB_ATTR_VALUE_UNDEFINED)
		adapter->temp_dcb_cfg.tc_config[tc].path[1].bwg_id = bwg_id;
	if (bw_pct != DCB_ATTR_VALUE_UNDEFINED)
		adapter->temp_dcb_cfg.tc_config[tc].path[1].bwg_percent =
			bw_pct;
	if (up_map != DCB_ATTR_VALUE_UNDEFINED)
		adapter->temp_dcb_cfg.tc_config[tc].path[1].up_to_tc_bitmap =
			up_map;
}

/**
 * ixgbe_dcbnl_set_pg_bwg_cfg_rx - Set RX bandwidth percentage for a BWG
 * @netdev: Network device structure
 * @bwg_id: Bandwidth group identifier for which to set the configuration
 * @bw_pct: Bandwidth percentage to be set for the specified bandwidth group
 *
 * This function sets the receive (RX) bandwidth percentage for a specified
 * bandwidth group (BWG) on the given network device. It updates the temporary
 * DCB configuration with the provided bandwidth percentage for the specified
 * BWG identifier.
 *
 * The function first checks if the provided BWG identifier is within the valid
 * range. If the identifier is out of range, it logs an error message and
 * returns without making any changes.
 */
static void ixgbe_dcbnl_set_pg_bwg_cfg_rx(struct net_device *netdev, int bwg_id,
					  u8 bw_pct)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	if (bwg_id < 0 || bwg_id >= IXGBE_DCB_MAX_BW_GROUP) {
		netdev_err(netdev,
			   "BWG index out of range.\n");
		return;
	}

	adapter->temp_dcb_cfg.bw_percentage[1][bwg_id] = bw_pct;
}

/**
 * ixgbe_dcbnl_get_pg_tc_cfg_tx - Retrieve TX traffic class configuration
 * @netdev: Network device structure
 * @tc: Traffic class identifier for which to retrieve the configuration
 * @prio: Pointer to store the priority assignment for the traffic class
 * @bwg_id: Pointer to store the bandwidth group identifier for the traffic class
 * @bw_pct: Pointer to store the bandwidth percentage for the traffic class
 * @up_map: Pointer to store the user priority to traffic class mapping
 *
 * This function retrieves the transmit (TX) configuration for a specified traffic
 * class (TC) on the given network device. It accesses the adapter's DCB
 * configuration to obtain various parameters for the specified traffic class,
 * including the priority assignment (`tsa`), bandwidth group identifier (`bwg_id`),
 * bandwidth percentage (`bwg_percent`), and user priority to traffic class mapping
 * (`up_to_tc_bitmap`). These values are stored in the provided pointers.
 */
static void ixgbe_dcbnl_get_pg_tc_cfg_tx(struct net_device *netdev, int tc,
					 u8 *prio, u8 *bwg_id, u8 *bw_pct,
					 u8 *up_map)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	*prio = adapter->dcb_cfg.tc_config[tc].path[0].tsa;
	*bwg_id = adapter->dcb_cfg.tc_config[tc].path[0].bwg_id;
	*bw_pct = adapter->dcb_cfg.tc_config[tc].path[0].bwg_percent;
	*up_map = adapter->dcb_cfg.tc_config[tc].path[0].up_to_tc_bitmap;
}

/**
 * ixgbe_dcbnl_get_pg_bwg_cfg_tx - Retrieve TX bandwidth percentage for a BWG
 * @netdev: Network device structure
 * @bwg_id: Bandwidth group identifier for which to retrieve the configuration
 * @bw_pct: Pointer to a buffer where the bandwidth percentage will be stored
 *
 * This function retrieves the bandwidth percentage configuration for a specified
 * bandwidth group (BWG) on the transmit (TX) side of the given network device.
 * The function accesses the adapter's DCB configuration to obtain the bandwidth
 * percentage for the specified BWG identifier and stores it in the provided
 * buffer `bw_pct`. The TX side configuration is indexed by `0` in the
 * `bw_percentage` array.
 */
static void ixgbe_dcbnl_get_pg_bwg_cfg_tx(struct net_device *netdev, int bwg_id,
					  u8 *bw_pct)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	*bw_pct = adapter->dcb_cfg.bw_percentage[0][bwg_id];
}

/**
 * ixgbe_dcbnl_get_pg_tc_cfg_rx - Retrieve RX traffic class configuration
 * @netdev: Network device structure
 * @tc: Traffic class identifier for which to retrieve the configuration
 * @prio: Pointer to store the priority assignment for the traffic class
 * @bwg_id: Pointer to store the bandwidth group identifier for the traffic class
 * @bw_pct: Pointer to store the bandwidth percentage for the traffic class
 * @up_map: Pointer to store the user priority to traffic class mapping
 *
 * This function retrieves the receive (RX) configuration for a specified traffic
 * class (TC) on the given network device. It accesses the adapter's DCB
 * configuration to obtain various parameters for the specified traffic class,
 * including the priority assignment (`tsa`), bandwidth group identifier (`bwg_id`),
 * bandwidth percentage (`bwg_percent`), and user priority to traffic class mapping
 * (`up_to_tc_bitmap`). These values are stored in the provided pointers.
 */
static void ixgbe_dcbnl_get_pg_tc_cfg_rx(struct net_device *netdev, int tc,
					 u8 *prio, u8 *bwg_id, u8 *bw_pct,
					 u8 *up_map)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	*prio = adapter->dcb_cfg.tc_config[tc].path[1].tsa;
	*bwg_id = adapter->dcb_cfg.tc_config[tc].path[1].bwg_id;
	*bw_pct = adapter->dcb_cfg.tc_config[tc].path[1].bwg_percent;
	*up_map = adapter->dcb_cfg.tc_config[tc].path[1].up_to_tc_bitmap;
}

/**
 * ixgbe_dcbnl_get_pg_bwg_cfg_rx - Retrieve RX bandwidth percentage for a BWG
 * @netdev: Network device structure
 * @bwg_id: Bandwidth group identifier for which to retrieve the configuration
 * @bw_pct: Pointer to a buffer where the bandwidth percentage will be stored
 *
 * This function retrieves the bandwidth percentage configuration for a specified
 * bandwidth group (BWG) on the receive (RX) side of the given network device.
 * The function accesses the adapter's DCB configuration to obtain the bandwidth
 * percentage for the specified BWG identifier and stores it in the provided
 * buffer `bw_pct`. The RX side configuration is indexed by `1` in the
 * `bw_percentage` array.
 */
static void ixgbe_dcbnl_get_pg_bwg_cfg_rx(struct net_device *netdev, int bwg_id,
					  u8 *bw_pct)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	*bw_pct = adapter->dcb_cfg.bw_percentage[1][bwg_id];
}

/**
 * ixgbe_dcbnl_set_pfc_cfg - Set the PFC configuration for a specific user priority
 * @netdev: Network device structure
 * @up: User priority for which to set the PFC configuration
 * @pfc: PFC configuration value to be set
 *
 * This function sets the Priority Flow Control (PFC) configuration for a
 * specified user priority (UP) on the given network device. It determines the
 * traffic class (TC) associated with the specified user priority and updates
 * the temporary DCB configuration with the provided PFC value.
 *
 * If the new PFC configuration differs from the current configuration, the
 * function enables the PFC mode in the temporary DCB configuration, indicating
 * that a change has been made and may need to be applied to the hardware.
 */
static void ixgbe_dcbnl_set_pfc_cfg(struct net_device *netdev, int up, u8 pfc)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	u8 tc = ixgbe_dcb_get_tc_from_up(&adapter->temp_dcb_cfg, 0, up);

	adapter->temp_dcb_cfg.tc_config[tc].pfc = pfc;
	if (adapter->temp_dcb_cfg.tc_config[tc].pfc !=
	    adapter->dcb_cfg.tc_config[tc].pfc)
		adapter->temp_dcb_cfg.pfc_mode_enable = true;
}

/**
 * ixgbe_dcbnl_get_pfc_cfg - Retrieve the PFC configuration for a given user priority
 * @netdev: Network device structure
 * @up: User priority for which to retrieve the PFC configuration
 * @pfc: Pointer to a buffer where the PFC configuration will be stored
 *
 * This function retrieves the Priority Flow Control (PFC) configuration for a
 * specified user priority (UP) on the given network device. The PFC configuration
 * is part of the Data Center Bridging (DCB) feature, which provides enhanced
 * Ethernet capabilities for data center environments.
 *
 * The function first determines the traffic class (TC) associated with the
 * specified user priority by calling `ixgbe_dcb_get_tc_from_up`. It then
 * retrieves the PFC configuration for that traffic class from the adapter's
 * DCB configuration and stores it in the provided buffer `pfc`.
 *
 * This function is useful for network management tools and applications that
 * need to query the PFC settings of a network device to ensure proper traffic
 * prioritization and flow control in a data center network.
 */
static void ixgbe_dcbnl_get_pfc_cfg(struct net_device *netdev, int up, u8 *pfc)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	u8 tc = ixgbe_dcb_get_tc_from_up(&adapter->dcb_cfg, 0, up);
	*pfc = adapter->dcb_cfg.tc_config[tc].pfc;
}

/**
 * ixgbe_dcbnl_devreset - Reset the network device for DCB changes
 * @dev: Network device to reset
 *
 * This function resets the ixgbe network adapter to apply changes related
 * to Data Center Bridging (DCB) configurations. It ensures that the device
 * is not already in the process of resetting and stops the network
 * interface if it is running. The function clears and reinitializes the
 * interrupt scheme, then restarts the network interface if it was
 * previously running. It manages the resetting state to prevent concurrent
 * resets.
 */
static void ixgbe_dcbnl_devreset(struct net_device *dev)
{
	struct ixgbe_adapter *adapter = netdev_priv(dev);

	while (test_and_set_bit(__IXGBE_RESETTING, adapter->state))
		usleep_range(1000, 2000);

	if (netif_running(dev))
#ifdef HAVE_NET_DEVICE_OPS
		dev->netdev_ops->ndo_stop(dev);
#else
		dev->stop(dev);
#endif

	ixgbe_clear_interrupt_scheme(adapter);
	ixgbe_init_interrupt_scheme(adapter);

	if (netif_running(dev))
#ifdef HAVE_NET_DEVICE_OPS
		dev->netdev_ops->ndo_open(dev);
#else
		dev->open(dev);
#endif

	clear_bit(__IXGBE_RESETTING, adapter->state);
}

/**
 * ixgbe_dcbnl_set_all - Apply all DCB configurations to the hardware
 * @netdev: Network device structure
 *
 * This function applies all Data Center Bridging (DCB) configurations to the
 * hardware for the specified network device. It checks if the device is in CEE
 * (Converged Enhanced Ethernet) mode and proceeds to update the DCB settings
 * if applicable. The function handles Priority Group (PG) and Priority Flow
 * Control (PFC) configurations, as well as application priority mappings.
 *
 * The function calculates traffic class credits and configures the hardware
 * accordingly. It also manages the enabling and disabling of Malicious Driver
 * Detection (MDD) around updates to ensure proper operation.
 *
 * If Fibre Channel over Ethernet (FCoE) is enabled, the function reprograms
 * FCoE hardware offloads when necessary.
 *
 * Return: A status code indicating whether a hardware change was made and
 *         whether a reset is required. Possible return values are:
 *         - DCB_NO_HW_CHG: No hardware change was made.
 *         - DCB_HW_CHG: A hardware change was made.
 *         - DCB_HW_CHG_RST: A hardware change was made and a reset is required.
 */
static u8 ixgbe_dcbnl_set_all(struct net_device *netdev)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	struct ixgbe_dcb_config *dcb_cfg = &adapter->dcb_cfg;
	struct ixgbe_hw *hw = &adapter->hw;
	int ret = DCB_NO_HW_CHG;
	u8 prio_tc[IXGBE_DCB_MAX_USER_PRIORITY] = { 0 };

	/* Fail command if not in CEE mode */
	if (!(adapter->dcbx_cap & DCB_CAP_DCBX_VER_CEE))
		return ret;

	adapter->dcb_set_bitmap |= ixgbe_copy_dcb_cfg(adapter,
						      IXGBE_DCB_MAX_TRAFFIC_CLASS);
	if (!adapter->dcb_set_bitmap)
		return ret;

	ixgbe_dcb_unpack_map_cee(dcb_cfg, IXGBE_DCB_TX_CONFIG, prio_tc);

	if (adapter->dcb_set_bitmap & (BIT_PG_TX | BIT_PG_RX)) {
		/* Priority to TC mapping in CEE case default to 1:1 */
		int max_frame = adapter->netdev->mtu + ETH_HLEN + ETH_FCS_LEN;
#ifdef HAVE_MQPRIO
		int i;
#endif

#if IS_ENABLED(CONFIG_FCOE)
#ifdef HAVE_NETDEV_FCOE_MTU
		if (adapter->netdev->fcoe_mtu)
#else
		if (adapter->netdev->features & NETIF_F_FCOE_MTU)
#endif
			max_frame = max(max_frame, IXGBE_FCOE_JUMBO_FRAME_SIZE);
#endif

		ixgbe_dcb_calculate_tc_credits_cee(hw, dcb_cfg, max_frame,
						   IXGBE_DCB_TX_CONFIG);

		ixgbe_dcb_calculate_tc_credits_cee(hw, dcb_cfg, max_frame,
						   IXGBE_DCB_RX_CONFIG);

		ixgbe_dcb_hw_config_cee(hw, dcb_cfg);

#ifdef HAVE_MQPRIO
		for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++)
			netdev_set_prio_tc_map(netdev, i, prio_tc[i]);
#endif /* HAVE_MQPRIO */
		ret = DCB_HW_CHG_RST;
	}

	if (adapter->dcb_set_bitmap & BIT_PFC) {
		if (dcb_cfg->pfc_mode_enable) {
			u8 pfc_en;
			ixgbe_dcb_unpack_pfc_cee(dcb_cfg, prio_tc, &pfc_en);
			ixgbe_dcb_config_pfc(hw, pfc_en, prio_tc);
		} else {
			hw->mac.ops.fc_enable(hw);
		}
		/* This is known driver so disable MDD before updating SRRCTL */
		if (hw->mac.ops.disable_mdd &&
		    (adapter->flags & IXGBE_FLAG_MDD_ENABLED))
			hw->mac.ops.disable_mdd(hw);

		ixgbe_set_rx_drop_en(adapter);

		if (hw->mac.ops.enable_mdd &&
		    (adapter->flags & IXGBE_FLAG_MDD_ENABLED))
			hw->mac.ops.enable_mdd(hw);

		if (ret != DCB_HW_CHG_RST)
			ret = DCB_HW_CHG;
	}

#if IS_ENABLED(CONFIG_FCOE)
	/* Reprogam FCoE hardware offloads when the traffic class
	 * FCoE is using changes. This happens if the APP info
	 * changes or the up2tc mapping is updated.
	 */
	if (adapter->dcb_set_bitmap & BIT_APP_UPCHG) {
		adapter->fcoe.up_set = adapter->fcoe.up;
		ixgbe_dcbnl_devreset(netdev);
		ret = DCB_HW_CHG_RST;
	}
#endif /* CONFIG_FCOE */

	adapter->dcb_set_bitmap = 0x00;
	return ret;
}

/**
 * ixgbe_dcbnl_getcap - Retrieve DCB capability for a given attribute
 * @netdev: Network device structure
 * @capid: Capability identifier for which to retrieve the configuration
 * @cap: Pointer to a buffer where the capability value will be stored
 *
 * This function retrieves the Data Center Bridging (DCB) capability for a
 * specified attribute identified by `capid` on the given network device. The
 * function sets the capability value in the provided buffer `cap` based on the
 * attribute requested. It supports various DCB capabilities, such as Priority
 * Groups (PG), Priority Flow Control (PFC), User Priority to Traffic Class
 * mapping (UP2TC), and others.
 *
 * The function uses a switch statement to determine the capability value for
 * each supported attribute. If the attribute is not recognized, the capability
 * is set to `false`.
 *
 * Return: Always returns 0.
 */
static u8 ixgbe_dcbnl_getcap(struct net_device *netdev, int capid, u8 *cap)
{
#ifdef HAVE_DCBNL_IEEE
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
#endif

	switch (capid) {
	case DCB_CAP_ATTR_PG:
		*cap = true;
		break;
	case DCB_CAP_ATTR_PFC:
		*cap = true;
		break;
	case DCB_CAP_ATTR_UP2TC:
		*cap = false;
		break;
	case DCB_CAP_ATTR_PG_TCS:
		*cap = 0x80;
		break;
	case DCB_CAP_ATTR_PFC_TCS:
		*cap = 0x80;
		break;
	case DCB_CAP_ATTR_GSP:
		*cap = true;
		break;
	case DCB_CAP_ATTR_BCN:
		*cap = false;
		break;
#ifdef HAVE_DCBNL_IEEE
	case DCB_CAP_ATTR_DCBX:
		*cap = adapter->dcbx_cap;
		break;
#endif
	default:
		*cap = false;
		break;
	}

	return 0;
}

/**
 * ixgbe_dcbnl_getnumtcs - Retrieve the number of traffic classes for a given attribute
 * @netdev: Network device structure
 * @tcid: Traffic class identifier attribute for which to retrieve the number of TCs
 * @num: Pointer to a buffer where the number of traffic classes will be stored
 *
 * This function retrieves the number of traffic classes (TCs) for a specified
 * attribute identified by `tcid` on the given network device. It checks if Data
 * Center Bridging (DCB) is enabled and then accesses the adapter's DCB
 * configuration to obtain the number of TCs for the specified attribute. The
 * result is stored in the provided buffer `num`.
 *
 * The function supports attributes such as Priority Groups (PG) and Priority
 * Flow Control (PFC). If DCB is not enabled or the attribute is not recognized,
 * the function returns an error value.
 *
 * Return: 0 on success, or a negative error code (-EINVAL) if DCB is not enabled
 *         or the attribute is invalid. The return type is `u8` or `int` based on
 *         the `NUMTCS_RETURNS_U8` preprocessor directive.
 */
#ifdef NUMTCS_RETURNS_U8
static u8 ixgbe_dcbnl_getnumtcs(struct net_device *netdev, int tcid, u8 *num)
#else
static int ixgbe_dcbnl_getnumtcs(struct net_device *netdev, int tcid, u8 *num)
#endif
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	u8 rval = 0;

	if (adapter->flags & IXGBE_FLAG_DCB_ENABLED) {
		switch (tcid) {
		case DCB_NUMTCS_ATTR_PG:
			*num = adapter->dcb_cfg.num_tcs.pg_tcs;
			break;
		case DCB_NUMTCS_ATTR_PFC:
			*num = adapter->dcb_cfg.num_tcs.pfc_tcs;
			break;
		default:
			rval = -EINVAL;
			break;
		}
	} else {
		rval = -EINVAL;
	}

	return rval;
}

/**
 * ixgbe_dcbnl_setnumtcs - Set the number of traffic classes for a given attribute
 * @netdev: Network device structure
 * @tcid: Traffic class identifier attribute for which to set the number of TCs
 * @num: Number of traffic classes to be set for the specified attribute
 *
 * This function sets the number of traffic classes (TCs) for a specified
 * attribute identified by `tcid` on the given network device. It updates the
 * adapter's DCB configuration with the provided number of TCs for the specified
 * attribute, such as Priority Groups (PG) or Priority Flow Control (PFC).
 *
 * The function first checks if DCB is enabled on the device. If not, or if the
 * attribute is not recognized, it returns an error value.
 *
 * Return: 0 on success, or a negative error code (-EINVAL) if DCB is not enabled
 *         or the attribute is invalid. The return type is `u8` or `int` based on
 *         the `NUMTCS_RETURNS_U8` preprocessor directive.
 */
#ifdef NUMTCS_RETURNS_U8
static u8 ixgbe_dcbnl_setnumtcs(struct net_device *netdev, int tcid, u8 num)
#else
static int ixgbe_dcbnl_setnumtcs(struct net_device *netdev, int tcid, u8 num)
#endif
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);
	u8 rval = 0;

	if (adapter->flags & IXGBE_FLAG_DCB_ENABLED) {
		switch (tcid) {
		case DCB_NUMTCS_ATTR_PG:
			adapter->dcb_cfg.num_tcs.pg_tcs = num;
			break;
		case DCB_NUMTCS_ATTR_PFC:
			adapter->dcb_cfg.num_tcs.pfc_tcs = num;
			break;
		default:
			rval = -EINVAL;
			break;
		}
	} else {
		rval = -EINVAL;
	}

	return rval;
}

/**
 * ixgbe_dcbnl_getpfcstate - Retrieve the PFC state of the network device
 * @netdev: Network device structure
 *
 * This function retrieves the Priority Flow Control (PFC) state of the specified
 * network device. It accesses the adapter's DCB configuration to obtain the PFC
 * mode enable status and returns it. The PFC state indicates whether PFC is
 * enabled or disabled on the device.
 *
 * Return: The PFC mode enable status of the network device.
 */
static u8 ixgbe_dcbnl_getpfcstate(struct net_device *netdev)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	return adapter->dcb_cfg.pfc_mode_enable;
}

/**
 * ixgbe_dcbnl_setpfcstate - Set the PFC state of the network device
 * @netdev: Network device structure
 * @state: Desired state for PFC (0 to disable, non-zero to enable)
 *
 * This function sets the Priority Flow Control (PFC) state for the specified
 * network device. It updates the temporary DCB configuration to reflect the
 * desired PFC state, enabling or disabling PFC as specified by the `state`
 * parameter.
 *
 * The function directly modifies the `pfc_mode_enable` field in the temporary
 * DCB configuration, which may later be applied to the hardware.
 */
static void ixgbe_dcbnl_setpfcstate(struct net_device *netdev, u8 state)
{
	struct ixgbe_adapter *adapter = netdev_priv(netdev);

	adapter->temp_dcb_cfg.pfc_mode_enable = state;
	return;
}

#ifdef HAVE_DCBNL_OPS_GETAPP
/**
 * ixgbe_dcbnl_getapp - retrieve the DCBX application user priority
 * @netdev : the corresponding netdev
 * @idtype : identifies the id as ether type or TCP/UDP port number
 * @id: id is either ether type or TCP/UDP port number
 *
 * Returns : on success, returns a non-zero 802.1p user priority bitmap
 * otherwise returns 0 as the invalid user priority bitmap to indicate an
 * error.
 */
#ifdef HAVE_DCBNL_OPS_SETAPP_RETURN_INT
static int ixgbe_dcbnl_getapp(struct net_device *netdev, u8 idtype, u16 id)
#else
static u8 ixgbe_dcbnl_getapp(struct net_device *netdev, u8 idtype, u16 id)
#endif
{
	u8 rval = 0;
#ifdef HAVE_DCBNL_IEEE
	struct dcb_app app = {
		.selector = idtype,
		.protocol = id,
	};

	rval = dcb_getapp(netdev, &app);
#endif

	switch (idtype) {
	case DCB_APP_IDTYPE_ETHTYPE:
#if IS_ENABLED(CONFIG_FCOE)
		if (id == ETH_P_FCOE)
			rval = ixgbe_fcoe_getapp(netdev);
#endif
		break;
	case DCB_APP_IDTYPE_PORTNUM:
		break;
	default:
		break;
	}

	return rval;
}

/**
 * ixgbe_dcbnl_setapp - set the DCBX application user priority
 * @netdev: the corresponding netdev
 * @idtype: identifies the id as ether type or TCP/UDP port number
 * @id: id is either ether type or TCP/UDP port number
 * @up: the 802.1p user priority bitmap
 *
 * Returns : 0 on success or 1 on error
 */
#ifdef HAVE_DCBNL_OPS_SETAPP_RETURN_INT
static int ixgbe_dcbnl_setapp(struct net_device *netdev,
			      u8 idtype, u16 id, u8 up)
#else
static u8 ixgbe_dcbnl_setapp(struct net_device *netdev,
			     u8 idtype, u16 id, u8 up)
#endif
{
	int err = 0;
#ifdef HAVE_DCBNL_IEEE
	struct dcb_app app;

	app.selector = idtype;
	app.protocol = id;
	app.priority = up;
	err = dcb_setapp(netdev, &app);
#endif

	switch (idtype) {
	case DCB_APP_IDTYPE_ETHTYPE:
#if IS_ENABLED(CONFIG_FCOE)
		if (id == ETH_P_FCOE) {
			struct ixgbe_adapter *adapter = netdev_priv(netdev);

			adapter->fcoe.up = up ? ffs(up) - 1 : IXGBE_FCOE_DEFUP;
		}
#endif
		break;
	case DCB_APP_IDTYPE_PORTNUM:
		break;
	default:
		break;
	}

	return err;
}
#endif /* HAVE_DCBNL_OPS_GETAPP */

#ifdef HAVE_DCBNL_IEEE
/**
 * ixgbe_dcbnl_ieee_getets - Retrieve IEEE ETS configuration
 * @dev: Network device structure
 * @ets: Pointer to the IEEE ETS structure where the configuration will be stored
 *
 * This function retrieves the Enhanced Transmission Selection (ETS) configuration
 * for the specified network device according to the IEEE 802.1Qaz standard. It
 * accesses the adapter's stored ETS configuration and copies it into the provided
 * `ets` structure.
 *
 * The function sets the ETS capability (`ets_cap`) based on the number of priority
 * groups configured in the device. It also copies the Class-Based Shaping (CBS)
 * settings, transmit and receive bandwidth allocations (`tc_tx_bw` and `tc_rx_bw`),
 * traffic selection algorithms (`tc_tsa`), and priority to traffic class mappings
 * (`prio_tc`) from the adapter's ETS configuration.
 *
 * If no IEEE ETS settings are available, the function returns an error.
 *
 * Return: 0 on success, or a negative error code (-EINVAL) if no ETS settings are
 *         available.
 */
static int ixgbe_dcbnl_ieee_getets(struct net_device *dev,
				   struct ieee_ets *ets)
{
	struct ixgbe_adapter *adapter = netdev_priv(dev);
	struct ieee_ets *my_ets = adapter->ixgbe_ieee_ets;

	/* No IEEE PFC settings available */
	if (!my_ets)
		return -EINVAL;

	ets->ets_cap = adapter->dcb_cfg.num_tcs.pg_tcs;
	ets->cbs = my_ets->cbs;
	memcpy(ets->tc_tx_bw, my_ets->tc_tx_bw, sizeof(ets->tc_tx_bw));
	memcpy(ets->tc_rx_bw, my_ets->tc_rx_bw, sizeof(ets->tc_rx_bw));
	memcpy(ets->tc_tsa, my_ets->tc_tsa, sizeof(ets->tc_tsa));
	memcpy(ets->prio_tc, my_ets->prio_tc, sizeof(ets->prio_tc));
	return 0;
}

/**
 * ixgbe_dcbnl_ieee_setets - Set IEEE ETS configuration
 * @dev: Network device structure
 * @ets: Pointer to the IEEE ETS structure containing the configuration to be set
 *
 * This function sets the Enhanced Transmission Selection (ETS) configuration
 * for the specified network device according to the IEEE 802.1Qaz standard. It
 * first checks if the device supports IEEE DCBX (Data Center Bridging Exchange)
 * version. If not, it returns an error.
 *
 * The function allocates memory for the ETS configuration if it hasn't been
 * initialized yet and attempts to update the User Priority to Traffic Class
 * (UP2TC) mappings from hardware if possible. It then updates the ETS
 * configuration with the provided settings and checks for changes in the
 * traffic class mappings.
 *
 * If the number of traffic classes (`max_tc`) changes, the function calls
 * `ixgbe_setup_tc` to reconfigure the traffic classes. If only the mappings
 * change, it resets the device's DCB configuration.
 *
 * Finally, the function updates the hardware ETS settings using
 * `ixgbe_dcb_hw_ets`.
 *
 * Return: 0 on success, or a negative error code (-EINVAL, -ENOMEM) if an error
 *         occurs during the process.
 */
static int ixgbe_dcbnl_ieee_setets(struct net_device *dev,
				   struct ieee_ets *ets)
{
	struct ixgbe_adapter *adapter = netdev_priv(dev);
	int max_frame = dev->mtu + ETH_HLEN + ETH_FCS_LEN;
	int i, err = 0;
	__u8 max_tc = 0;
	__u8 map_chg = 0;

	if (!(adapter->dcbx_cap & DCB_CAP_DCBX_VER_IEEE))
		return -EINVAL;

	if (!adapter->ixgbe_ieee_ets) {
		adapter->ixgbe_ieee_ets = kzalloc(sizeof(struct ieee_ets),
						  GFP_KERNEL);
		if (!adapter->ixgbe_ieee_ets)
			return -ENOMEM;
		/* initialize UP2TC mappings to invalid value */
		for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++)
			adapter->ixgbe_ieee_ets->prio_tc[i] =
				IEEE_8021QAZ_MAX_TCS;
		/* if possible update UP2TC mappings from HW */
		if (adapter->hw.mac.ops.get_rtrup2tc)
			adapter->hw.mac.ops.get_rtrup2tc(&adapter->hw,
					adapter->ixgbe_ieee_ets->prio_tc);
	}

	for (i = 0; i < IEEE_8021QAZ_MAX_TCS; i++) {
		if (ets->prio_tc[i] > max_tc)
			max_tc = ets->prio_tc[i];
		if (ets->prio_tc[i] != adapter->ixgbe_ieee_ets->prio_tc[i])
			map_chg = 1;
	}

	memcpy(adapter->ixgbe_ieee_ets, ets, sizeof(*adapter->ixgbe_ieee_ets));

	if (max_tc)
		max_tc++;

	if (max_tc > adapter->dcb_cfg.num_tcs.pg_tcs)
		return -EINVAL;

	if (max_tc != netdev_get_num_tc(dev))
		err = ixgbe_setup_tc(dev, max_tc);
	else if (map_chg)
		ixgbe_dcbnl_devreset(dev);

	if (err)
		goto err_out;

	err = ixgbe_dcb_hw_ets(&adapter->hw, ets, max_frame);
err_out:
	return err;
}

/**
 * ixgbe_dcbnl_ieee_getpfc - Retrieve IEEE PFC configuration
 * @dev: Network device structure
 * @pfc: Pointer to the IEEE PFC structure where the configuration will be stored
 *
 * This function retrieves the Priority Flow Control (PFC) configuration for the
 * specified network device according to the IEEE 802.1Qaz standard. It accesses
 * the adapter's stored PFC configuration and copies it into the provided `pfc`
 * structure.
 *
 * The function sets the PFC capability (`pfc_cap`) based on the number of traffic
 * classes configured for PFC in the device. It also copies the PFC enable mask
 * (`pfc_en`), the MAC control frame support (`mbc`), and the delay time for PFC
 * frames (`delay`) from the adapter's PFC configuration.
 *
 * Additionally, the function populates the `requests` and `indications` arrays
 * with the number of PFC requests and indications for each traffic class, using
 * statistics from the adapter.
 *
 * If no IEEE PFC settings are available, the function returns an error.
 *
 * Return: 0 on success, or a negative error code (-EINVAL) if no PFC settings are
 *         available.
 */
static int ixgbe_dcbnl_ieee_getpfc(struct net_device *dev,
				   struct ieee_pfc *pfc)
{
	struct ixgbe_adapter *adapter = netdev_priv(dev);
	struct ieee_pfc *my_pfc = adapter->ixgbe_ieee_pfc;
	int i;

	/* No IEEE PFC settings available */
	if (!my_pfc)
		return -EINVAL;

	pfc->pfc_cap = adapter->dcb_cfg.num_tcs.pfc_tcs;
	pfc->pfc_en = my_pfc->pfc_en;
	pfc->mbc = my_pfc->mbc;
	pfc->delay = my_pfc->delay;

	for (i = 0; i < IXGBE_DCB_MAX_TRAFFIC_CLASS; i++) {
		pfc->requests[i] = adapter->stats.pxoffrxc[i];
		pfc->indications[i] = adapter->stats.pxofftxc[i];
	}

	return 0;
}

/**
 * ixgbe_dcbnl_ieee_setpfc - Set IEEE PFC configuration
 * @dev: Network device structure
 * @pfc: Pointer to the IEEE PFC structure containing the configuration to be set
 *
 * This function sets the Priority Flow Control (PFC) configuration for the
 * specified network device according to the IEEE 802.1Qaz standard. It first
 * checks if the device supports IEEE DCBX (Data Center Bridging Exchange)
 * version. If not, it returns an error.
 *
 * The function allocates memory for the PFC configuration if it hasn't been
 * initialized yet. It then updates the adapter's PFC configuration with the
 * provided settings.
 *
 * If PFC is enabled, the function configures the PFC settings using
 * `ixgbe_dcb_config_pfc`, passing the PFC enable mask and the priority to
 * traffic class mapping. If PFC is disabled, it enables standard link flow
 * control using the hardware's `fc_enable` operation.
 *
 * The function also handles the disabling and enabling of Malicious Driver
 * Detection (MDD) around the update of the SRRCTL register to ensure proper
 * operation.
 *
 * Return: 0 on success, or a negative error code (-EINVAL, -ENOMEM) if an error
 *         occurs during the process.
 */
static int ixgbe_dcbnl_ieee_setpfc(struct net_device *dev,
				   struct ieee_pfc *pfc)
{
	struct ixgbe_adapter *adapter = netdev_priv(dev);
	struct ixgbe_hw *hw = &adapter->hw;
	u8 *prio_tc;
	int err;

	if (!(adapter->dcbx_cap & DCB_CAP_DCBX_VER_IEEE))
		return -EINVAL;

	if (!adapter->ixgbe_ieee_pfc) {
		adapter->ixgbe_ieee_pfc = kzalloc(sizeof(struct ieee_pfc),
						  GFP_KERNEL);
		if (!adapter->ixgbe_ieee_pfc)
			return -ENOMEM;
	}

	prio_tc = adapter->ixgbe_ieee_ets->prio_tc;
	memcpy(adapter->ixgbe_ieee_pfc, pfc, sizeof(*adapter->ixgbe_ieee_pfc));


	/* Enable link flow control parameters if PFC is disabled */
	if (pfc->pfc_en)
		err = ixgbe_dcb_config_pfc(hw, pfc->pfc_en, prio_tc);
	else
		err = hw->mac.ops.fc_enable(hw);

	/* This is known driver so disable MDD before updating SRRCTL */
	if (hw->mac.ops.disable_mdd &&
	    (adapter->flags & IXGBE_FLAG_MDD_ENABLED))
		hw->mac.ops.disable_mdd(hw);

	ixgbe_set_rx_drop_en(adapter);

	if (hw->mac.ops.enable_mdd &&
	    (adapter->flags & IXGBE_FLAG_MDD_ENABLED))
		hw->mac.ops.enable_mdd(hw);

	return err;
}

/**
 * ixgbe_dcbnl_ieee_setapp - Set an IEEE DCB application priority mapping
 * @dev: Network device structure
 * @app: Pointer to the DCB application structure to be set
 *
 * This function sets an IEEE Data Center Bridging (DCB) application priority
 * mapping for the specified network device. It first checks if the device
 * supports IEEE DCBX (Data Center Bridging Exchange) version. If not, it returns
 * an error. The function then calls `dcb_ieee_setapp` to apply the application
 * mapping.
 *
 * If Fibre Channel over Ethernet (FCoE) is enabled in the kernel configuration
 * and the application being set is related to FCoE, the function updates the
 * FCoE user priority (UP) and resets the device's DCB configuration if necessary.
 *
 * Return: 0 on success, or a negative error code (-EINVAL) if IEEE DCBX is not
 *         supported or if the setting fails.
 */
static int ixgbe_dcbnl_ieee_setapp(struct net_device *dev,
				   struct dcb_app *app)
{
	struct ixgbe_adapter *adapter = netdev_priv(dev);
	int err = -EINVAL;

	if (!(adapter->dcbx_cap & DCB_CAP_DCBX_VER_IEEE))
		return err;

	err = dcb_ieee_setapp(dev, app);

#if IS_ENABLED(CONFIG_FCOE)
	if (!err && app->selector == IEEE_8021QAZ_APP_SEL_ETHERTYPE &&
	    app->protocol == ETH_P_FCOE) {
		u8 app_mask = dcb_ieee_getapp_mask(dev, app);

		if (app_mask & (1 << adapter->fcoe.up))
			return err;

		adapter->fcoe.up = app->priority;
		adapter->fcoe.up_set = adapter->fcoe.up;
		ixgbe_dcbnl_devreset(dev);
	}
#endif
	return 0;
}

#ifdef HAVE_DCBNL_IEEE_DELAPP
/**
 * ixgbe_dcbnl_ieee_delapp - Delete an IEEE DCB application priority mapping
 * @dev: Network device structure
 * @app: Pointer to the DCB application structure to be deleted
 *
 * This function deletes an IEEE Data Center Bridging (DCB) application priority
 * mapping from the specified network device. It first checks if the device
 * supports IEEE DCBX (Data Center Bridging Exchange) version. If not, it returns
 * an error. The function then calls `dcb_ieee_delapp` to remove the application
 * mapping.
 *
 * If Fibre Channel over Ethernet (FCoE) is enabled in the kernel configuration
 * and the application being deleted is related to FCoE, the function updates the
 * FCoE user priority (UP) if necessary and resets the device's DCB configuration.
 *
 * Return: 0 on success, or a negative error code (-EINVAL) if IEEE DCBX is not
 *         supported or if the deletion fails.
 */
static int ixgbe_dcbnl_ieee_delapp(struct net_device *dev,
				   struct dcb_app *app)
{
	struct ixgbe_adapter *adapter = netdev_priv(dev);
	int err;

	if (!(adapter->dcbx_cap & DCB_CAP_DCBX_VER_IEEE))
		return -EINVAL;

	err = dcb_ieee_delapp(dev, app);

#if IS_ENABLED(CONFIG_FCOE)
	if (!err && app->selector == IEEE_8021QAZ_APP_SEL_ETHERTYPE &&
	    app->protocol == ETH_P_FCOE) {
		u8 app_mask = dcb_ieee_getapp_mask(dev, app);

		if (app_mask & (1 << adapter->fcoe.up))
			return err;

		adapter->fcoe.up = app_mask ?
				   ffs(app_mask) - 1 : IXGBE_FCOE_DEFUP;
		ixgbe_dcbnl_devreset(dev);
	}
#endif
	return err;
}
#endif /* HAVE_DCBNL_IEEE_DELAPP */

/**
 * ixgbe_dcbnl_getdcbx - Retrieve the DCBX capability of the network device
 * @dev: Network device structure
 *
 * This function retrieves the Data Center Bridging Exchange (DCBX) capability
 * of the specified network device. It accesses the adapter's configuration to
 * obtain the DCBX capability value and returns it. The DCBX capability
 * indicates the level of DCBX support provided by the device.
 *
 * Return: The DCBX capability value of the network device.
 */
static u8 ixgbe_dcbnl_getdcbx(struct net_device *dev)
{
	struct ixgbe_adapter *adapter = netdev_priv(dev);
	return adapter->dcbx_cap;
}

/**
 * ixgbe_dcbnl_setdcbx - Set the DCBX mode of the network device
 * @dev: Network device structure
 * @mode: Desired DCBX mode to be set
 *
 * This function sets the Data Center Bridging Exchange (DCBX) mode for the
 * specified network device. It checks for unsupported configurations, such as
 * LLD_MANAGED modes or simultaneous CEE and IEEE modes, and returns an error
 * if any are detected. The function also ensures that the mode includes
 * DCB_CAP_DCBX_HOST.
 *
 * If the desired mode is already set, the function exits without making changes.
 * Otherwise, it updates the adapter's DCBX capability and applies default
 * Enhanced Transmission Selection (ETS) and Priority Flow Control (PFC)
 * configurations.
 *
 * Depending on the mode, the function configures the device for IEEE or CEE
 * DCBX. For IEEE, it sets ETS and PFC using the respective functions. For CEE,
 * it sets a bitmap for DCB configuration and applies all settings. If both CEE
 * and IEEE are disabled, the function configures the device for single traffic
 * class mode with strict priority.
 *
 * Return: 0 on success, or 1 if an unsupported configuration is detected.
 */
static u8 ixgbe_dcbnl_setdcbx(struct net_device *dev, u8 mode)
{
	struct ixgbe_adapter *adapter = netdev_priv(dev);
	struct ieee_ets ets = { .ets_cap = 0 };
	struct ieee_pfc pfc = { .pfc_en = 0 };

	/* no support for LLD_MANAGED modes or CEE+IEEE */
	if ((mode & DCB_CAP_DCBX_LLD_MANAGED) ||
	    ((mode & DCB_CAP_DCBX_VER_IEEE) && (mode & DCB_CAP_DCBX_VER_CEE)) ||
	    !(mode & DCB_CAP_DCBX_HOST))
		return 1;

	if (mode == adapter->dcbx_cap)
		return 0;

	adapter->dcbx_cap = mode;

	/* ETS and PFC defaults */
	ets.ets_cap = 8;
	pfc.pfc_cap = 8;

	if (mode & DCB_CAP_DCBX_VER_IEEE) {
		ixgbe_dcbnl_ieee_setets(dev, &ets);
		ixgbe_dcbnl_ieee_setpfc(dev, &pfc);
	} else if (mode & DCB_CAP_DCBX_VER_CEE) {
		u8 mask = (BIT_PFC | BIT_PG_TX | BIT_PG_RX | BIT_APP_UPCHG);

		adapter->dcb_set_bitmap |= mask;
		ixgbe_dcbnl_set_all(dev);
	} else {
		/* Drop into single TC mode strict priority as this
		 * indicates CEE and IEEE versions are disabled
		 */
		ixgbe_dcbnl_ieee_setets(dev, &ets);
		ixgbe_dcbnl_ieee_setpfc(dev, &pfc);
		ixgbe_setup_tc(dev, 0);
	}

	return 0;
}

#endif

struct dcbnl_rtnl_ops ixgbe_dcbnl_ops = {
#ifdef HAVE_DCBNL_IEEE
	.ieee_getets	= ixgbe_dcbnl_ieee_getets,
	.ieee_setets	= ixgbe_dcbnl_ieee_setets,
	.ieee_getpfc	= ixgbe_dcbnl_ieee_getpfc,
	.ieee_setpfc	= ixgbe_dcbnl_ieee_setpfc,
	.ieee_setapp	= ixgbe_dcbnl_ieee_setapp,
#ifdef HAVE_DCBNL_IEEE_DELAPP
	.ieee_delapp	= ixgbe_dcbnl_ieee_delapp,
#endif
#endif
	.getstate	= ixgbe_dcbnl_get_state,
	.setstate	= ixgbe_dcbnl_set_state,
	.getpermhwaddr	= ixgbe_dcbnl_get_perm_hw_addr,
	.setpgtccfgtx	= ixgbe_dcbnl_set_pg_tc_cfg_tx,
	.setpgbwgcfgtx	= ixgbe_dcbnl_set_pg_bwg_cfg_tx,
	.setpgtccfgrx	= ixgbe_dcbnl_set_pg_tc_cfg_rx,
	.setpgbwgcfgrx	= ixgbe_dcbnl_set_pg_bwg_cfg_rx,
	.getpgtccfgtx	= ixgbe_dcbnl_get_pg_tc_cfg_tx,
	.getpgbwgcfgtx	= ixgbe_dcbnl_get_pg_bwg_cfg_tx,
	.getpgtccfgrx	= ixgbe_dcbnl_get_pg_tc_cfg_rx,
	.getpgbwgcfgrx	= ixgbe_dcbnl_get_pg_bwg_cfg_rx,
	.setpfccfg	= ixgbe_dcbnl_set_pfc_cfg,
	.getpfccfg	= ixgbe_dcbnl_get_pfc_cfg,
	.setall		= ixgbe_dcbnl_set_all,
	.getcap		= ixgbe_dcbnl_getcap,
	.getnumtcs	= ixgbe_dcbnl_getnumtcs,
	.setnumtcs	= ixgbe_dcbnl_setnumtcs,
	.getpfcstate	= ixgbe_dcbnl_getpfcstate,
	.setpfcstate	= ixgbe_dcbnl_setpfcstate,
#ifdef HAVE_DCBNL_OPS_GETAPP
	.getapp		= ixgbe_dcbnl_getapp,
	.setapp		= ixgbe_dcbnl_setapp,
#endif
#ifdef HAVE_DCBNL_IEEE
	.getdcbx	= ixgbe_dcbnl_getdcbx,
	.setdcbx	= ixgbe_dcbnl_setdcbx,
#endif
};

#endif /* CONFIG_DCB */
