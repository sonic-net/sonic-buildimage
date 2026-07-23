// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2017 - 2022 Pensando Systems, Inc */

#include <linux/module.h>
#include <linux/netdevice.h>

/* Normally we would #include <linux/sfp.h> here, but some of the
 * older distros don't have that file, and some that do have an
 * older version that doesn't include these definitions.
 */
enum {
	SFF8024_ID_UNK			= 0x00,
	SFF8024_ID_SFF_8472		= 0x02,
	SFF8024_ID_SFP			= 0x03,
	SFF8024_ID_DWDM_SFP		= 0x0b,
	SFF8024_ID_QSFP_8438		= 0x0c,
	SFF8024_ID_QSFP_8436_8636	= 0x0d,
	SFF8024_ID_QSFP28_8636		= 0x11,
};

#include "ionic.h"
#include "ionic_bus.h"
#include "ionic_lif.h"
#include "ionic_ethtool.h"
#include "ionic_stats.h"

static const char ionic_priv_flags_strings[][ETH_GSTRING_LEN] = {
#define IONIC_PRIV_F_RDMA_SNIFFER	BIT(0)
	"rdma-sniffer",
#define IONIC_PRIV_F_DEVICE_RESET	BIT(1)
	"device-reset",
#define IONIC_PRIV_F_CMB_RINGS		BIT(2)
	"cmb-rings",

#define IONIC_PRIV_F_SW_DBG_STATS	BIT(3)
#ifdef IONIC_DEBUG_STATS
	"sw-dbg-stats",
#endif
};

#define IONIC_PRIV_FLAGS_COUNT ARRAY_SIZE(ionic_priv_flags_strings)

static void ionic_get_stats_strings(struct ionic_lif *lif, u8 *buf)
{
	u32 i;

	for (i = 0; i < ionic_num_stats_grps; i++)
		ionic_stats_groups[i].get_strings(lif, &buf);
}

static void ionic_get_stats(struct net_device *netdev,
			    struct ethtool_stats *stats, u64 *buf)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	u32 i;

	if (test_bit(IONIC_LIF_F_FW_RESET, lif->state))
		return;

	memset(buf, 0, stats->n_stats * sizeof(*buf));
	for (i = 0; i < ionic_num_stats_grps; i++)
		ionic_stats_groups[i].get_values(lif, &buf);
}

static int ionic_get_stats_count(struct ionic_lif *lif)
{
	int i, num_stats = 0;

	for (i = 0; i < ionic_num_stats_grps; i++)
		num_stats += ionic_stats_groups[i].get_count(lif);

	return num_stats;
}

static int ionic_get_sset_count(struct net_device *netdev, int sset)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	int count = 0;

	switch (sset) {
	case ETH_SS_STATS:
		count = ionic_get_stats_count(lif);
		break;
	case ETH_SS_PRIV_FLAGS:
		count = IONIC_PRIV_FLAGS_COUNT;
		break;
	}
	return count;
}

static void ionic_get_strings(struct net_device *netdev,
			      u32 sset, u8 *buf)
{
	struct ionic_lif *lif = netdev_priv(netdev);

	switch (sset) {
	case ETH_SS_STATS:
		ionic_get_stats_strings(lif, buf);
		break;
	case ETH_SS_PRIV_FLAGS:
		memcpy(buf, ionic_priv_flags_strings,
		       IONIC_PRIV_FLAGS_COUNT * ETH_GSTRING_LEN);
		break;
	}
}

static void ionic_get_drvinfo(struct net_device *netdev,
			      struct ethtool_drvinfo *drvinfo)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	struct ionic *ionic = lif->ionic;

	strscpy(drvinfo->driver, IONIC_DRV_NAME, sizeof(drvinfo->driver));
	strscpy(drvinfo->version, IONIC_DRV_VERSION, sizeof(drvinfo->version));
	strscpy(drvinfo->fw_version, ionic->idev.dev_info.fw_version,
		sizeof(drvinfo->fw_version));
	strscpy(drvinfo->bus_info, ionic_bus_info(ionic),
		sizeof(drvinfo->bus_info));
}

static int ionic_get_regs_len(struct net_device *netdev)
{
	return (IONIC_DEV_INFO_REG_COUNT + IONIC_DEV_CMD_REG_COUNT) * sizeof(u32);
}

static void ionic_get_regs(struct net_device *netdev, struct ethtool_regs *regs,
			   void *p)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	unsigned int offset;
	unsigned int size;

	regs->version = IONIC_DEV_CMD_REG_VERSION;

	offset = 0;
	size = IONIC_DEV_INFO_REG_COUNT * sizeof(u32);
	memcpy_fromio(p + offset, lif->ionic->idev.dev_info_regs->words, size);

	offset += size;
	size = IONIC_DEV_CMD_REG_COUNT * sizeof(u32);
	memcpy_fromio(p + offset, lif->ionic->idev.dev_cmd_regs->words, size);
}

#if (KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE)
static void ionic_get_link_ext_stats(struct net_device *netdev,
				     struct ethtool_link_ext_stats *stats)
{
	struct ionic_lif *lif = netdev_priv(netdev);

	if (!lif->ionic->pdev || lif->ionic->pdev->is_physfn)
		stats->link_down_events = lif->link_down_count;
}
#endif

static int ionic_get_link_ksettings(struct net_device *netdev,
				    struct ethtool_link_ksettings *ks)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	struct ionic_dev *idev = &lif->ionic->idev;
	int copper_seen = 0;

	ethtool_link_ksettings_zero_link_mode(ks, supported);

	if (!idev->port_info) {
		netdev_err(netdev, "port_info not initialized\n");
		return -EOPNOTSUPP;
	}

	/* The port_info data is found in a DMA space that the NIC keeps
	 * up-to-date, so there's no need to request the data from the
	 * NIC, we already have it in our memory space.
	 */

	switch (le16_to_cpu(idev->port_info->status.xcvr.pid)) {
		/* Copper */
#ifdef HAVE_ETHTOOL_100G_BITS
	case IONIC_XCVR_PID_QSFP_100G_CR4:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     100000baseCR4_Full);
		copper_seen++;
		break;
#endif
	case IONIC_XCVR_PID_QSFP_40GBASE_CR4:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     40000baseCR4_Full);
		copper_seen++;
		break;
#ifdef HAVE_ETHTOOL_25G_BITS
	case IONIC_XCVR_PID_SFP_25GBASE_CR_S:
	case IONIC_XCVR_PID_SFP_25GBASE_CR_L:
	case IONIC_XCVR_PID_SFP_25GBASE_CR_N:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     25000baseCR_Full);
		copper_seen++;
		break;
#endif
	case IONIC_XCVR_PID_SFP_10GBASE_AOC:
	case IONIC_XCVR_PID_SFP_10GBASE_CU:
#ifdef HAVE_ETHTOOL_NEW_10G_BITS
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseCR_Full);
#else
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseT_Full);
#endif
		copper_seen++;
		break;

		/* Fibre */
#ifdef HAVE_ETHTOOL_100G_BITS
	case IONIC_XCVR_PID_QSFP_100G_SR4:
	case IONIC_XCVR_PID_QSFP_100G_AOC:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     100000baseSR4_Full);
		break;
	case IONIC_XCVR_PID_QSFP_100G_CWDM4:
	case IONIC_XCVR_PID_QSFP_100G_PSM4:
	case IONIC_XCVR_PID_QSFP_100G_LR4:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     100000baseLR4_ER4_Full);
		break;
	case IONIC_XCVR_PID_QSFP_100G_ER4:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     100000baseLR4_ER4_Full);
		break;
#endif
	case IONIC_XCVR_PID_QSFP_40GBASE_SR4:
	case IONIC_XCVR_PID_QSFP_40GBASE_AOC:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     40000baseSR4_Full);
		break;
	case IONIC_XCVR_PID_QSFP_40GBASE_LR4:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     40000baseLR4_Full);
		break;
#ifdef HAVE_ETHTOOL_25G_BITS
	case IONIC_XCVR_PID_SFP_25GBASE_SR:
	case IONIC_XCVR_PID_SFP_25GBASE_AOC:
	case IONIC_XCVR_PID_SFP_25GBASE_ACC:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     25000baseSR_Full);
		break;
#endif
#ifdef HAVE_ETHTOOL_NEW_10G_BITS
	case IONIC_XCVR_PID_SFP_10GBASE_SR:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseSR_Full);
		break;
	case IONIC_XCVR_PID_SFP_10GBASE_LR:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseLR_Full);
		break;
	case IONIC_XCVR_PID_SFP_10GBASE_LRM:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseLRM_Full);
		break;
	case IONIC_XCVR_PID_SFP_10GBASE_ER:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseER_Full);
		break;
	case IONIC_XCVR_PID_SFP_10GBASE_T:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseT_Full);
		break;
#else
	case IONIC_XCVR_PID_SFP_10GBASE_SR:
	case IONIC_XCVR_PID_SFP_10GBASE_LR:
	case IONIC_XCVR_PID_SFP_10GBASE_LRM:
	case IONIC_XCVR_PID_SFP_10GBASE_ER:
	case IONIC_XCVR_PID_SFP_10GBASE_T:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     10000baseT_Full);
		break;
#endif
	case IONIC_XCVR_PID_SFP_1000BASE_T:
		ethtool_link_ksettings_add_link_mode(ks, supported,
						     1000baseT_Full);
		break;
	case IONIC_XCVR_PID_QSFP_100G_ACC:
	case IONIC_XCVR_PID_QSFP_40GBASE_ER4:
	case IONIC_XCVR_PID_SFP_25GBASE_LR:
	case IONIC_XCVR_PID_SFP_25GBASE_ER:
		dev_info(lif->ionic->dev, "no decode bits for xcvr type pid=%d / 0x%x\n",
			 idev->port_info->status.xcvr.pid,
			 idev->port_info->status.xcvr.pid);
		break;
	case IONIC_XCVR_PID_UNKNOWN:
		/* This means there's no module plugged in */
		if (lif->ionic->is_mgmt_nic)
			ethtool_link_ksettings_add_link_mode(ks, supported,
							     1000baseT_Full);
		break;
	default:
		dev_dbg(lif->ionic->dev, "unknown xcvr type pid=%d / 0x%x\n",
			idev->port_info->status.xcvr.pid,
			idev->port_info->status.xcvr.pid);
		break;
	}

	bitmap_copy(ks->link_modes.advertising, ks->link_modes.supported,
		    __ETHTOOL_LINK_MODE_MASK_NBITS);

#ifdef ETHTOOL_FEC_NONE
	if (idev->port_info->status.fec_type == IONIC_PORT_FEC_TYPE_FC)
		ethtool_link_ksettings_add_link_mode(ks, advertising, FEC_BASER);
	else if (idev->port_info->status.fec_type == IONIC_PORT_FEC_TYPE_RS)
		ethtool_link_ksettings_add_link_mode(ks, advertising, FEC_RS);
#endif

	if (lif->ionic->is_mgmt_nic)
		ethtool_link_ksettings_add_link_mode(ks, supported, Backplane);
	else
		ethtool_link_ksettings_add_link_mode(ks, supported, FIBRE);

	ethtool_link_ksettings_add_link_mode(ks, supported, Pause);

	if (idev->port_info->status.xcvr.phy == IONIC_PHY_TYPE_COPPER ||
	    copper_seen)
		ks->base.port = PORT_DA;
	else if (idev->port_info->status.xcvr.phy == IONIC_PHY_TYPE_FIBER)
		ks->base.port = PORT_FIBRE;
	else if (lif->ionic->is_mgmt_nic)
		ks->base.port = PORT_OTHER;
	else
		ks->base.port = PORT_NONE;

	if (ks->base.port != PORT_NONE) {
		ks->base.speed = le32_to_cpu(lif->info->status.link_speed);

		if (le16_to_cpu(lif->info->status.link_status))
			ks->base.duplex = DUPLEX_FULL;
		else
			ks->base.duplex = DUPLEX_UNKNOWN;

		if (ionic_is_pf(lif->ionic) && !lif->ionic->is_mgmt_nic) {
			ethtool_link_ksettings_add_link_mode(ks, supported,
							     Autoneg);

			if (idev->port_info->config.an_enable) {
				ethtool_link_ksettings_add_link_mode(ks,
								     advertising,
								     Autoneg);
				ks->base.autoneg = AUTONEG_ENABLE;
			}
		}
	}

	return 0;
}

static int ionic_set_link_ksettings(struct net_device *netdev,
				    const struct ethtool_link_ksettings *ks)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	struct ionic_dev *idev = &lif->ionic->idev;
	struct ionic *ionic = lif->ionic;
	int err = 0;

	if (test_bit(IONIC_LIF_F_FW_RESET, lif->state))
		return -EBUSY;

	/* set autoneg */
	if (ks->base.autoneg != idev->port_info->config.an_enable) {
		mutex_lock(&ionic->dev_cmd_lock);
		ionic_dev_cmd_port_autoneg(idev, ks->base.autoneg);
		err = ionic_dev_cmd_wait(ionic, DEVCMD_TIMEOUT);
		mutex_unlock(&ionic->dev_cmd_lock);
		if (err)
			return err;
	}

	/* set speed */
	if (ks->base.speed != le32_to_cpu(idev->port_info->config.speed)) {
		mutex_lock(&ionic->dev_cmd_lock);
		ionic_dev_cmd_port_speed(idev, ks->base.speed);
		err = ionic_dev_cmd_wait(ionic, DEVCMD_TIMEOUT);
		mutex_unlock(&ionic->dev_cmd_lock);
		if (err)
			return err;
	}

	return 0;
}

static void ionic_get_pauseparam(struct net_device *netdev,
				 struct ethtool_pauseparam *pause)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	u8 pause_type;

	pause->autoneg = 0;

	pause_type = lif->ionic->idev.port_info->config.pause_type;
	if (pause_type) {
		pause->rx_pause = (pause_type & IONIC_PAUSE_F_RX) ? 1 : 0;
		pause->tx_pause = (pause_type & IONIC_PAUSE_F_TX) ? 1 : 0;
	}
}

static int ionic_set_pauseparam(struct net_device *netdev,
				struct ethtool_pauseparam *pause)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	struct ionic *ionic = lif->ionic;
	u32 requested_pause;
	int err;

	if (test_bit(IONIC_LIF_F_FW_RESET, lif->state))
		return -EBUSY;

	if (pause->autoneg)
		return -EOPNOTSUPP;

	/* change both at the same time */
	requested_pause = IONIC_PORT_PAUSE_TYPE_LINK;
	if (pause->rx_pause)
		requested_pause |= IONIC_PAUSE_F_RX;
	if (pause->tx_pause)
		requested_pause |= IONIC_PAUSE_F_TX;

	if (requested_pause == lif->ionic->idev.port_info->config.pause_type)
		return 0;

	mutex_lock(&ionic->dev_cmd_lock);
	ionic_dev_cmd_port_pause(&lif->ionic->idev, requested_pause);
	err = ionic_dev_cmd_wait(ionic, DEVCMD_TIMEOUT);
	mutex_unlock(&ionic->dev_cmd_lock);
	if (err)
		return err;

	return 0;
}

#ifdef ETHTOOL_FEC_NONE
static int ionic_get_fecparam(struct net_device *netdev,
			      struct ethtool_fecparam *fec)
{
	struct ionic_lif *lif = netdev_priv(netdev);

	switch (lif->ionic->idev.port_info->status.fec_type) {
	case IONIC_PORT_FEC_TYPE_NONE:
		fec->active_fec = ETHTOOL_FEC_OFF;
		break;
	case IONIC_PORT_FEC_TYPE_RS:
		fec->active_fec = ETHTOOL_FEC_RS;
		break;
	case IONIC_PORT_FEC_TYPE_FC:
		fec->active_fec = ETHTOOL_FEC_BASER;
		break;
	default:
		fec->active_fec = ETHTOOL_FEC_NONE;
		break;
	}

	switch (lif->ionic->idev.port_info->config.fec_type) {
	case IONIC_PORT_FEC_TYPE_NONE:
		fec->fec = ETHTOOL_FEC_OFF;
		break;
	case IONIC_PORT_FEC_TYPE_RS:
		fec->fec = ETHTOOL_FEC_RS;
		break;
	case IONIC_PORT_FEC_TYPE_FC:
		fec->fec = ETHTOOL_FEC_BASER;
		break;
	default:
		fec->fec = ETHTOOL_FEC_NONE;
		break;
	}

	return 0;
}

static int ionic_set_fecparam(struct net_device *netdev,
			      struct ethtool_fecparam *fec)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	u8 fec_type;
	int ret = 0;

	if (test_bit(IONIC_LIF_F_FW_RESET, lif->state))
		return -EBUSY;

	if (lif->ionic->idev.port_info->config.an_enable) {
		netdev_err(netdev, "FEC request not allowed while autoneg is enabled\n");
		return -EINVAL;
	}

	switch (fec->fec) {
	case ETHTOOL_FEC_NONE:
		fec_type = IONIC_PORT_FEC_TYPE_NONE;
		break;
	case ETHTOOL_FEC_OFF:
		fec_type = IONIC_PORT_FEC_TYPE_NONE;
		break;
	case ETHTOOL_FEC_RS:
		fec_type = IONIC_PORT_FEC_TYPE_RS;
		break;
	case ETHTOOL_FEC_BASER:
		fec_type = IONIC_PORT_FEC_TYPE_FC;
		break;
	case ETHTOOL_FEC_AUTO:
	default:
		netdev_dbg(netdev, "FEC request 0x%04x not supported\n",
			   fec->fec);
		return -EOPNOTSUPP;
	}

	if (fec_type != lif->ionic->idev.port_info->config.fec_type) {
		mutex_lock(&lif->ionic->dev_cmd_lock);
		ionic_dev_cmd_port_fec(&lif->ionic->idev, fec_type);
		ret = ionic_dev_cmd_wait(lif->ionic, DEVCMD_TIMEOUT);
		mutex_unlock(&lif->ionic->dev_cmd_lock);
	}

	return ret;
}

#endif /* ETHTOOL_FEC_NONE */
#ifdef HAVE_COALESCE_EXTACK
static int ionic_get_coalesce(struct net_device *netdev,
			      struct ethtool_coalesce *coalesce,
			      struct kernel_ethtool_coalesce *kernel_coal,
			      struct netlink_ext_ack *extack)
#else
static int ionic_get_coalesce(struct net_device *netdev,
			      struct ethtool_coalesce *coalesce)
#endif
{
	struct ionic_lif *lif = netdev_priv(netdev);

	coalesce->tx_coalesce_usecs = lif->tx_coalesce_usecs;
	coalesce->rx_coalesce_usecs = lif->rx_coalesce_usecs;

	if (test_bit(IONIC_LIF_F_SPLIT_INTR, lif->state))
		coalesce->use_adaptive_tx_coalesce = test_bit(IONIC_LIF_F_TX_DIM_INTR, lif->state);
	else
		coalesce->use_adaptive_tx_coalesce = 0;

	coalesce->use_adaptive_rx_coalesce = test_bit(IONIC_LIF_F_RX_DIM_INTR, lif->state);

	return 0;
}

#ifdef HAVE_COALESCE_EXTACK
static int ionic_set_coalesce(struct net_device *netdev,
			      struct ethtool_coalesce *coalesce,
			      struct kernel_ethtool_coalesce *kernel_coal,
			      struct netlink_ext_ack *extack)
#else
static int ionic_set_coalesce(struct net_device *netdev,
			      struct ethtool_coalesce *coalesce)
#endif
{
	struct ionic_lif *lif = netdev_priv(netdev);
	struct ionic_identity *ident;
	u32 rx_coal, rx_dim;
	u32 tx_coal, tx_dim;
	unsigned int i;

	if (coalesce->rx_max_coalesced_frames ||
	    coalesce->rx_coalesce_usecs_irq ||
	    coalesce->rx_max_coalesced_frames_irq ||
	    coalesce->tx_max_coalesced_frames ||
	    coalesce->tx_coalesce_usecs_irq ||
	    coalesce->tx_max_coalesced_frames_irq ||
	    coalesce->stats_block_coalesce_usecs ||
	    coalesce->pkt_rate_low ||
	    coalesce->rx_coalesce_usecs_low ||
	    coalesce->rx_max_coalesced_frames_low ||
	    coalesce->tx_coalesce_usecs_low ||
	    coalesce->tx_max_coalesced_frames_low ||
	    coalesce->pkt_rate_high ||
	    coalesce->rx_coalesce_usecs_high ||
	    coalesce->rx_max_coalesced_frames_high ||
	    coalesce->tx_coalesce_usecs_high ||
	    coalesce->tx_max_coalesced_frames_high ||
	    coalesce->rate_sample_interval)
		return -EINVAL;

	ident = &lif->ionic->ident;
	if (ident->dev.intr_coal_div == 0) {
		netdev_warn(netdev, "bad HW value in dev.intr_coal_div = %d\n",
			    ident->dev.intr_coal_div);
		return -EIO;
	}

	/* Tx normally shares Rx interrupt, so only change Rx if not split */
	if (!test_bit(IONIC_LIF_F_SPLIT_INTR, lif->state) &&
	    (coalesce->tx_coalesce_usecs != lif->rx_coalesce_usecs ||
	     coalesce->use_adaptive_tx_coalesce)) {
		netdev_warn(netdev, "only rx parameters can be changed\n");
		return -EINVAL;
	}

	/* Convert the usec request to a HW usable value.  If they asked
	 * for non-zero and it resolved to zero, bump it up
	 */
	rx_coal = ionic_coal_usec_to_hw(lif->ionic, coalesce->rx_coalesce_usecs);
	if (!rx_coal && coalesce->rx_coalesce_usecs)
		rx_coal = 1;
	tx_coal = ionic_coal_usec_to_hw(lif->ionic, coalesce->tx_coalesce_usecs);
	if (!tx_coal && coalesce->tx_coalesce_usecs)
		tx_coal = 1;

	if (rx_coal > IONIC_INTR_CTRL_COAL_MAX ||
	    tx_coal > IONIC_INTR_CTRL_COAL_MAX)
		return -ERANGE;

	/* Save the new values */
	lif->rx_coalesce_usecs = coalesce->rx_coalesce_usecs;
	lif->rx_coalesce_hw = rx_coal;

	if (test_bit(IONIC_LIF_F_SPLIT_INTR, lif->state))
		lif->tx_coalesce_usecs = coalesce->tx_coalesce_usecs;
	else
		lif->tx_coalesce_usecs = coalesce->rx_coalesce_usecs;
	lif->tx_coalesce_hw = tx_coal;

	if (coalesce->use_adaptive_rx_coalesce) {
		set_bit(IONIC_LIF_F_RX_DIM_INTR, lif->state);
		rx_dim = rx_coal;
	} else {
		clear_bit(IONIC_LIF_F_RX_DIM_INTR, lif->state);
		rx_dim = 0;
	}

	if (coalesce->use_adaptive_tx_coalesce) {
		set_bit(IONIC_LIF_F_TX_DIM_INTR, lif->state);
		tx_dim = tx_coal;
	} else {
		clear_bit(IONIC_LIF_F_TX_DIM_INTR, lif->state);
		tx_dim = 0;
	}

	if (test_bit(IONIC_LIF_F_UP, lif->state)) {
		for (i = 0; i < lif->nxqs; i++) {
			if (lif->rxqcqs[i]->flags & IONIC_QCQ_F_INTR) {
				ionic_intr_coal_init(lif->ionic->idev.intr_ctrl,
						     lif->rxqcqs[i]->intr.index,
						     lif->rx_coalesce_hw);
				lif->rxqcqs[i]->intr.dim_coal_hw = rx_dim;
				lif->rxqcqs[i]->intr.dim_coal_usecs =
							lif->rx_coalesce_usecs;
			}

			if (lif->txqcqs[i]->flags & IONIC_QCQ_F_INTR) {
				ionic_intr_coal_init(lif->ionic->idev.intr_ctrl,
						     lif->txqcqs[i]->intr.index,
						     lif->tx_coalesce_hw);
				lif->txqcqs[i]->intr.dim_coal_hw = tx_dim;
				lif->txqcqs[i]->intr.dim_coal_usecs =
							lif->tx_coalesce_usecs;
			}
		}
	}

	return 0;
}

static int ionic_validate_cmb_config(struct ionic_lif *lif,
				     struct ionic_queue_params *qparam)
{
	int pages_have, pages_required = 0;
	unsigned long sz;

	if (!lif->ionic->idev.cmb_inuse &&
	    (qparam->cmb_tx || qparam->cmb_rx)) {
		netdev_info(lif->netdev, "CMB rings are not supported on this device\n");
		return -EOPNOTSUPP;
	}

	if (qparam->cmb_tx) {
		if (!(lif->qtype_info[IONIC_QTYPE_TXQ].features & IONIC_QIDENT_F_CMB)) {
			netdev_info(lif->netdev,
				    "CMB rings for tx-push are not supported on this device\n");
			return -EOPNOTSUPP;
		}

		sz = sizeof(struct ionic_txq_desc) * qparam->ntxq_descs * qparam->nxqs;
		pages_required += ALIGN(sz, PAGE_SIZE) / PAGE_SIZE;
	}

	if (qparam->cmb_rx) {
		if (!(lif->qtype_info[IONIC_QTYPE_RXQ].features & IONIC_QIDENT_F_CMB)) {
			netdev_info(lif->netdev,
				    "CMB rings for rx-push are not supported on this device\n");
			return -EOPNOTSUPP;
		}

		sz = sizeof(struct ionic_rxq_desc) * qparam->nrxq_descs * qparam->nxqs;
		pages_required += ALIGN(sz, PAGE_SIZE) / PAGE_SIZE;
	}

	pages_have = lif->ionic->bars[IONIC_PCI_BAR_CMB].len / PAGE_SIZE;
	if (pages_required > pages_have) {
		netdev_info(lif->netdev,
			    "Not enough CMB pages for number of queues and size of descriptor rings, need %d have %d",
			    pages_required, pages_have);
		return -ENOMEM;
	}

	return pages_required;
}

int ionic_cmb_pages_in_use(struct ionic_lif *lif)
{
	struct ionic_queue_params qparam;

	ionic_init_queue_params(lif, &qparam);
	return ionic_validate_cmb_config(lif, &qparam);
}

static int ionic_cmb_rings_toggle(struct ionic_lif *lif, bool cmb_tx, bool cmb_rx)
{
	struct ionic_queue_params qparam;
	int pages_used;

	if (netif_running(lif->netdev)) {
		netdev_info(lif->netdev, "Please stop device to toggle CMB for tx/rx-push\n");
		return -EBUSY;
	}

	ionic_init_queue_params(lif, &qparam);
	qparam.cmb_tx = cmb_tx;
	qparam.cmb_rx = cmb_rx;
	pages_used = ionic_validate_cmb_config(lif, &qparam);
	if (pages_used < 0)
		return pages_used;

	if (cmb_tx)
		set_bit(IONIC_LIF_F_CMB_TX_RINGS, lif->state);
	else
		clear_bit(IONIC_LIF_F_CMB_TX_RINGS, lif->state);

	if (cmb_rx)
		set_bit(IONIC_LIF_F_CMB_RX_RINGS, lif->state);
	else
		clear_bit(IONIC_LIF_F_CMB_RX_RINGS, lif->state);

	if (cmb_tx || cmb_rx)
		netdev_info(lif->netdev, "Enabling CMB %s %s rings - %d pages\n",
			    cmb_tx ? "TX" : "", cmb_rx ? "RX" : "", pages_used);
	else
		netdev_info(lif->netdev, "Disabling CMB rings\n");

	return 0;
}

#ifdef HAVE_RINGPARAM_EXTACK
static void ionic_get_ringparam(struct net_device *netdev,
				struct ethtool_ringparam *ring,
				struct kernel_ethtool_ringparam *kernel_ring,
				struct netlink_ext_ack *extack)
#else
static void ionic_get_ringparam(struct net_device *netdev,
				struct ethtool_ringparam *ring)
#endif
{
	struct ionic_lif *lif = netdev_priv(netdev);

	ring->tx_max_pending = IONIC_MAX_TX_DESC;
	ring->tx_pending = lif->ntxq_descs;
	ring->rx_max_pending = IONIC_MAX_RX_DESC;
	ring->rx_pending = lif->nrxq_descs;
#ifdef HAVE_RX_PUSH
	kernel_ring->tx_push = test_bit(IONIC_LIF_F_CMB_TX_RINGS, lif->state);
	kernel_ring->rx_push = test_bit(IONIC_LIF_F_CMB_RX_RINGS, lif->state);
#endif
}

#ifdef HAVE_RINGPARAM_EXTACK
static int ionic_set_ringparam(struct net_device *netdev,
			       struct ethtool_ringparam *ring,
			       struct kernel_ethtool_ringparam *kernel_ring,
			       struct netlink_ext_ack *extack)
#else
static int ionic_set_ringparam(struct net_device *netdev,
			       struct ethtool_ringparam *ring)
#endif
{
	struct ionic_lif *lif = netdev_priv(netdev);
	struct ionic_queue_params qparam;
	int err;

	if (test_bit(IONIC_LIF_F_FW_RESET, lif->state))
		return -EBUSY;

	ionic_init_queue_params(lif, &qparam);

	if (ring->rx_mini_pending || ring->rx_jumbo_pending) {
		netdev_info(netdev, "Changing jumbo or mini descriptors not supported\n");
		return -EINVAL;
	}

	if (!is_power_of_2(ring->tx_pending) ||
	    !is_power_of_2(ring->rx_pending)) {
		netdev_info(netdev, "Descriptor count must be a power of 2\n");
		return -EINVAL;
	}

	if (ring->tx_pending > IONIC_MAX_TX_DESC ||
	    ring->tx_pending < IONIC_MIN_TXRX_DESC) {
		netdev_info(netdev, "Tx descriptor count must be in the range [%d-%d]\n",
			    IONIC_MIN_TXRX_DESC, IONIC_MAX_TX_DESC);
		return -EINVAL;
	}

	if (ring->rx_pending > IONIC_MAX_RX_DESC ||
	    ring->rx_pending < IONIC_MIN_TXRX_DESC) {
		netdev_info(netdev, "Rx descriptor count must be in the range [%d-%d]\n",
			    IONIC_MIN_TXRX_DESC, IONIC_MAX_RX_DESC);
		return -EINVAL;
	}

	/* if nothing to do return success */
	if (ring->tx_pending == lif->ntxq_descs &&
	    ring->rx_pending == lif->nrxq_descs
#ifdef HAVE_RX_PUSH
	    &&
	    kernel_ring->tx_push == test_bit(IONIC_LIF_F_CMB_TX_RINGS, lif->state) &&
	    kernel_ring->rx_push == test_bit(IONIC_LIF_F_CMB_RX_RINGS, lif->state)
#endif
	    )
		return 0;

	qparam.ntxq_descs = ring->tx_pending;
	qparam.nrxq_descs = ring->rx_pending;
#ifdef HAVE_RX_PUSH
	qparam.cmb_tx = kernel_ring->tx_push;
	qparam.cmb_rx = kernel_ring->rx_push;
#endif

	err = ionic_validate_cmb_config(lif, &qparam);
	if (err < 0)
		return err;

#ifdef HAVE_RX_PUSH
	if (kernel_ring->tx_push != test_bit(IONIC_LIF_F_CMB_TX_RINGS, lif->state) ||
	    kernel_ring->rx_push != test_bit(IONIC_LIF_F_CMB_RX_RINGS, lif->state)) {
		err = ionic_cmb_rings_toggle(lif, kernel_ring->tx_push,
					     kernel_ring->rx_push);
		if (err < 0)
			return err;
	}

#endif
	if (ring->tx_pending != lif->ntxq_descs)
		netdev_info(netdev, "Changing Tx ring size from %d to %d\n",
			    lif->ntxq_descs, ring->tx_pending);

	if (ring->rx_pending != lif->nrxq_descs)
		netdev_info(netdev, "Changing Rx ring size from %d to %d\n",
			    lif->nrxq_descs, ring->rx_pending);

	/* if we're not running, just set the values and return */
	if (!netif_running(lif->netdev)) {
		lif->ntxq_descs = ring->tx_pending;
		lif->nrxq_descs = ring->rx_pending;
		return 0;
	}

	mutex_lock(&lif->queue_lock);
	err = ionic_reconfigure_queues(lif, &qparam);
	mutex_unlock(&lif->queue_lock);
	if (err)
		netdev_info(netdev, "Ring reconfiguration failed, changes canceled: %d\n", err);

	return err;
}

static void ionic_get_channels(struct net_device *netdev,
			       struct ethtool_channels *ch)
{
	struct ionic_lif *lif = netdev_priv(netdev);

	/* report maximum channels */
	ch->max_combined = lif->ionic->ntxqs_per_lif;
	ch->max_rx = lif->ionic->ntxqs_per_lif / 2;
	ch->max_tx = lif->ionic->ntxqs_per_lif / 2;

	/* report current channels */
	if (test_bit(IONIC_LIF_F_SPLIT_INTR, lif->state)) {
		ch->rx_count = lif->nxqs;
		ch->tx_count = lif->nxqs;
	} else {
		ch->combined_count = lif->nxqs;
	}
}

static int ionic_set_channels(struct net_device *netdev,
			      struct ethtool_channels *ch)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	struct ionic_queue_params qparam;
	int max_cnt;
	int err;

	if (test_bit(IONIC_LIF_F_FW_RESET, lif->state))
		return -EBUSY;

	ionic_init_queue_params(lif, &qparam);

	/* Valid cases
	 *  Combined (default):
	 *    rx_count == tx_count: 0
	 *    combined_count: 1..lif->ionic->ntxqs_per_lif
	 *    other_count: 0
	 *  Split:
	 *    rx_count == tx_count: 1..lif->ionic->ntxqs_per_lif / 2
	 *    combined_count: 0
	 *    other_count: 0
	 */
	if (ch->other_count) {
		netdev_info(netdev, "We don't use other queues\n");
		return -EINVAL;
	}

	if (ch->rx_count != ch->tx_count) {
		netdev_info(netdev, "The rx and tx count must be equal\n");
		return -EINVAL;
	}

	if (ch->combined_count && ch->rx_count) {
		netdev_info(netdev, "Use either combined or rx and tx, not both\n");
		return -EINVAL;
	}

	max_cnt = lif->ionic->ntxqs_per_lif;
	if (ch->combined_count) {
		if (ch->combined_count > max_cnt)
			return -EINVAL;

		if (test_bit(IONIC_LIF_F_SPLIT_INTR, lif->state))
			netdev_info(lif->netdev, "Sharing queue interrupts\n");
		else if (ch->combined_count == lif->nxqs)
			return 0;

		if (lif->nxqs != ch->combined_count)
			netdev_info(netdev, "Changing queue count from %d to %d\n",
				    lif->nxqs, ch->combined_count);

		qparam.nxqs = ch->combined_count;
		qparam.intr_split = false;
	} else {
		max_cnt /= 2;
		if (ch->rx_count > max_cnt)
			return -EINVAL;

		if (!test_bit(IONIC_LIF_F_SPLIT_INTR, lif->state))
			netdev_info(lif->netdev, "Splitting queue interrupts\n");
		else if (ch->rx_count == lif->nxqs)
			return 0;

		if (lif->nxqs != ch->rx_count)
			netdev_info(netdev, "Changing queue count from %d to %d\n",
				    lif->nxqs, ch->rx_count);

		qparam.nxqs = ch->rx_count;
		qparam.intr_split = true;
	}

	err = ionic_validate_cmb_config(lif, &qparam);
	if (err < 0)
		return err;

	/* if we're not running, just set the values and return */
	if (!netif_running(lif->netdev)) {
		lif->nxqs = qparam.nxqs;

		if (qparam.intr_split) {
			set_bit(IONIC_LIF_F_SPLIT_INTR, lif->state);
		} else {
			clear_bit(IONIC_LIF_F_SPLIT_INTR, lif->state);
			lif->tx_coalesce_usecs = lif->rx_coalesce_usecs;
			lif->tx_coalesce_hw = lif->rx_coalesce_hw;
		}
		return 0;
	}

	mutex_lock(&lif->queue_lock);
	err = ionic_reconfigure_queues(lif, &qparam);
	mutex_unlock(&lif->queue_lock);
	if (err)
		netdev_info(netdev, "Queue reconfiguration failed, changes canceled: %d\n", err);

	return err;
}

static int ionic_get_rxnfc(struct net_device *netdev,
			   struct ethtool_rxnfc *info, u32 *rules)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	int err = 0;

	switch (info->cmd) {
	case ETHTOOL_GRXRINGS:
		info->data = lif->nxqs;
		break;
	default:
		netdev_dbg(netdev, "Command parameter %d is not supported\n",
			   info->cmd);
		err = -EOPNOTSUPP;
	}

	return err;
}

static u32 ionic_get_priv_flags(struct net_device *netdev)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	u32 priv_flags = 0;

	if (test_bit(IONIC_LIF_F_SW_DEBUG_STATS, lif->state))
		priv_flags |= IONIC_PRIV_F_SW_DBG_STATS;

	if (test_bit(IONIC_LIF_F_RDMA_SNIFFER, lif->state))
		priv_flags |= IONIC_PRIV_F_RDMA_SNIFFER;

	if (test_bit(IONIC_LIF_F_CMB_TX_RINGS, lif->state) ||
	    test_bit(IONIC_LIF_F_CMB_RX_RINGS, lif->state))
		priv_flags |= IONIC_PRIV_F_CMB_RINGS;

	return priv_flags;
}

static int ionic_set_priv_flags(struct net_device *netdev, u32 priv_flags)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	bool cmb_req;
	int rdma;
	int ret;

	if (priv_flags & IONIC_PRIV_F_DEVICE_RESET) {
		ionic_reset_prepare(lif->ionic->pdev);
		ionic_reset_done(lif->ionic->pdev);
	}

	clear_bit(IONIC_LIF_F_SW_DEBUG_STATS, lif->state);
	if (priv_flags & IONIC_PRIV_F_SW_DBG_STATS)
		set_bit(IONIC_LIF_F_SW_DEBUG_STATS, lif->state);

	rdma = test_bit(IONIC_LIF_F_RDMA_SNIFFER, lif->state);
	clear_bit(IONIC_LIF_F_RDMA_SNIFFER, lif->state);
	if (priv_flags & IONIC_PRIV_F_RDMA_SNIFFER)
		set_bit(IONIC_LIF_F_RDMA_SNIFFER, lif->state);

	if (rdma != test_bit(IONIC_LIF_F_RDMA_SNIFFER, lif->state))
		ionic_lif_rx_mode(lif);

	cmb_req = !!(priv_flags & IONIC_PRIV_F_CMB_RINGS);
	if ((cmb_req && !(test_bit(IONIC_LIF_F_CMB_TX_RINGS, lif->state) &&
			  test_bit(IONIC_LIF_F_CMB_RX_RINGS, lif->state))) ||
	    (!cmb_req && (test_bit(IONIC_LIF_F_CMB_TX_RINGS, lif->state) ||
			  test_bit(IONIC_LIF_F_CMB_RX_RINGS, lif->state)))) {
		ret = ionic_cmb_rings_toggle(lif, cmb_req, cmb_req);
		if (ret < 0)
			return ret;
	}

	return 0;
}

static u32 ionic_get_rxfh_indir_size(struct net_device *netdev)
{
	struct ionic_lif *lif = netdev_priv(netdev);

	return le16_to_cpu(lif->ionic->ident.lif.eth.rss_ind_tbl_sz);
}

static u32 ionic_get_rxfh_key_size(struct net_device *netdev)
{
	return IONIC_RSS_HASH_KEY_SIZE;
}

#ifdef HAVE_RXFH_HASHFUNC
static int ionic_get_rxfh_legacy(struct net_device *netdev, u32 *indir, u8 *key,
			  u8 *hfunc)
#else
static int ionic_get_rxfh_legacy(struct net_device *netdev, u32 *indir, u8 *key)
#endif
{
	struct ionic_lif *lif = netdev_priv(netdev);
	unsigned int i, tbl_sz;

	if (indir) {
		tbl_sz = le16_to_cpu(lif->ionic->ident.lif.eth.rss_ind_tbl_sz);
		for (i = 0; i < tbl_sz; i++)
			indir[i] = lif->rss_ind_tbl[i];
	}

	if (key)
		memcpy(key, lif->rss_hash_key, IONIC_RSS_HASH_KEY_SIZE);

#ifdef HAVE_RXFH_HASHFUNC
	if (hfunc)
		*hfunc = ETH_RSS_HASH_TOP;
#endif

	return 0;
}

static int ionic_get_rxfh(struct net_device *netdev,
			  struct ethtool_rxfh_param *rxfh)
{
#ifdef HAVE_RXFH_HASHFUNC
	u8 hfunc;
	int ret;

	ret = ionic_get_rxfh_legacy(netdev,
				    rxfh->indir,
				    rxfh->key,
				    &hfunc);
	if (!ret)
		rxfh->hfunc = hfunc;

	return ret;
#else
	return ionic_get_rxfh_legacy(netdev,
				     rxfh->indir,
				     rxfh->key);
#endif
}

#ifdef HAVE_RXFH_HASHFUNC
static int ionic_set_rxfh_legacy(struct net_device *netdev, const u32 *indir,
			  const u8 *key, const u8 hfunc)
#else
static int ionic_set_rxfh_legacy(struct net_device *netdev, const u32 *indir,
			  const u8 *key)
#endif
{
	struct ionic_lif *lif = netdev_priv(netdev);

#ifdef HAVE_RXFH_HASHFUNC
	if (hfunc != ETH_RSS_HASH_NO_CHANGE && hfunc != ETH_RSS_HASH_TOP)
		return -EOPNOTSUPP;
#endif

	return ionic_lif_rss_config(lif, lif->rss_types, key, indir);
}

static int ionic_set_rxfh(struct net_device *netdev,
			  struct ethtool_rxfh_param *rxfh,
			  struct netlink_ext_ack *extack)
{
#ifdef HAVE_RXFH_HASHFUNC
	return ionic_set_rxfh_legacy(netdev,
				     rxfh->indir,
				     rxfh->key,
				     rxfh->hfunc);
#else
	return ionic_set_rxfh_legacy(netdev,
				     rxfh->indir,
				     rxfh->key);
#endif
}

static int ionic_set_tunable(struct net_device *dev,
			     const struct ethtool_tunable *tuna,
			     const void *data)
{
	struct ionic_lif *lif = netdev_priv(dev);

	switch (tuna->id) {
	case ETHTOOL_RX_COPYBREAK:
		lif->rx_copybreak = *(u32 *)data;
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static int ionic_get_tunable(struct net_device *netdev,
			     const struct ethtool_tunable *tuna, void *data)
{
	struct ionic_lif *lif = netdev_priv(netdev);

	switch (tuna->id) {
	case ETHTOOL_RX_COPYBREAK:
		*(u32 *)data = lif->rx_copybreak;
		break;
	default:
		return -EOPNOTSUPP;
	}

	return 0;
}

static int ionic_get_module_info(struct net_device *netdev,
				 struct ethtool_modinfo *modinfo)

{
	struct ionic_lif *lif = netdev_priv(netdev);
	struct ionic_dev *idev = &lif->ionic->idev;
	struct ionic_xcvr_status *xcvr;

	xcvr = &idev->port_info->status.xcvr;

	/* report the module data type and length */
	switch (xcvr->sprom[0]) {
	case SFF8024_ID_SFP:
		modinfo->type = ETH_MODULE_SFF_8079;
		modinfo->eeprom_len = ETH_MODULE_SFF_8079_LEN;
		break;
	case SFF8024_ID_QSFP_8436_8636:
	case SFF8024_ID_QSFP28_8636:
		modinfo->type = ETH_MODULE_SFF_8436;
		modinfo->eeprom_len = ETH_MODULE_SFF_8436_LEN;
		break;
	case SFF8024_ID_UNK:
		if (lif->ionic->is_mgmt_nic)
			netdev_info(netdev, "no xcvr on mgmt nic\n");
		else
			netdev_info(netdev, "no xcvr connected? type 0x%02x\n",
				    xcvr->sprom[0]);
		return -EINVAL;
	default:
		netdev_info(netdev, "unknown xcvr type 0x%02x\n",
			    xcvr->sprom[0]);
		modinfo->type = 0;
		modinfo->eeprom_len = ETH_MODULE_SFF_8079_LEN;
		break;
	}

	return 0;
}

static int ionic_get_module_eeprom(struct net_device *netdev,
				   struct ethtool_eeprom *ee,
				   u8 *data)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	struct ionic_dev *idev = &lif->ionic->idev;
	struct ionic_xcvr_status *xcvr;
	char tbuf[sizeof(xcvr->sprom)];
	int count = 10;
	u32 len;

	/* The NIC keeps the module prom up-to-date in the DMA space
	 * so we can simply copy the module bytes into the data buffer.
	 */
	xcvr = &idev->port_info->status.xcvr;
	len = min_t(u32, sizeof(xcvr->sprom), ee->len);

	do {
		memcpy(data, xcvr->sprom, len);
		memcpy(tbuf, xcvr->sprom, len);

		/* Let's make sure we got a consistent copy */
		if (!memcmp(data, tbuf, len))
			break;

	} while (--count);

	if (!count)
		return -ETIMEDOUT;

	return 0;
}

#if IS_ENABLED(CONFIG_PTP_1588_CLOCK)
static int ionic_get_ts_info(struct net_device *netdev,
			     struct kernel_ethtool_ts_info *info)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	struct ionic *ionic = lif->ionic;
	__le64 mask;

	/* Fallback to generic ethtool handler if no PHC */
	if (!lif->phc || !lif->phc->ptp)
		return ethtool_op_get_ts_info(netdev, info);

	info->phc_index = ptp_clock_index(lif->phc->ptp);

	info->so_timestamping =
		SOF_TIMESTAMPING_TX_SOFTWARE |
		SOF_TIMESTAMPING_RX_SOFTWARE |
		SOF_TIMESTAMPING_SOFTWARE |
		SOF_TIMESTAMPING_TX_HARDWARE |
		SOF_TIMESTAMPING_RX_HARDWARE |
		SOF_TIMESTAMPING_RAW_HARDWARE;

	/* TX timestamp modes */
	info->tx_types = BIT(HWTSTAMP_TX_OFF) |
			 BIT(HWTSTAMP_TX_ON);

	mask = cpu_to_le64(BIT_ULL(IONIC_TXSTAMP_ONESTEP_SYNC));
	if (ionic->ident.lif.eth.hwstamp_tx_modes & mask)
		info->tx_types |= BIT(HWTSTAMP_TX_ONESTEP_SYNC);

#ifdef HAVE_HWSTAMP_TX_ONESTEP_P2P
	mask = cpu_to_le64(BIT_ULL(IONIC_TXSTAMP_ONESTEP_P2P));
	if (ionic->ident.lif.eth.hwstamp_tx_modes & mask)
		info->tx_types |= BIT(HWTSTAMP_TX_ONESTEP_P2P);
#endif

	/* RX timestamp filters */
	info->rx_filters = BIT(HWTSTAMP_FILTER_NONE) |
			   BIT(HWTSTAMP_FILTER_ALL);

#ifdef HAVE_HWTSTAMP_FILTER_NTP_ALL
	mask = cpu_to_le64(IONIC_PKT_CLS_NTP_ALL);
	if ((ionic->ident.lif.eth.hwstamp_rx_filters & mask) == mask)
		info->rx_filters |= BIT(HWTSTAMP_FILTER_NTP_ALL);
#endif

	mask = cpu_to_le64(IONIC_PKT_CLS_PTP1_SYNC);
	if ((ionic->ident.lif.eth.hwstamp_rx_filters & mask) == mask)
		info->rx_filters |= BIT(HWTSTAMP_FILTER_PTP_V1_L4_SYNC);

	mask = cpu_to_le64(IONIC_PKT_CLS_PTP1_DREQ);
	if ((ionic->ident.lif.eth.hwstamp_rx_filters & mask) == mask)
		info->rx_filters |= BIT(HWTSTAMP_FILTER_PTP_V1_L4_DELAY_REQ);

	mask = cpu_to_le64(IONIC_PKT_CLS_PTP1_ALL);
	if ((ionic->ident.lif.eth.hwstamp_rx_filters & mask) == mask)
		info->rx_filters |= BIT(HWTSTAMP_FILTER_PTP_V1_L4_EVENT);

	mask = cpu_to_le64(IONIC_PKT_CLS_PTP2_L4_SYNC);
	if ((ionic->ident.lif.eth.hwstamp_rx_filters & mask) == mask)
		info->rx_filters |= BIT(HWTSTAMP_FILTER_PTP_V2_L4_SYNC);

	mask = cpu_to_le64(IONIC_PKT_CLS_PTP2_L4_DREQ);
	if ((ionic->ident.lif.eth.hwstamp_rx_filters & mask) == mask)
		info->rx_filters |= BIT(HWTSTAMP_FILTER_PTP_V2_L4_DELAY_REQ);

	mask = cpu_to_le64(IONIC_PKT_CLS_PTP2_L4_ALL);
	if ((ionic->ident.lif.eth.hwstamp_rx_filters & mask) == mask)
		info->rx_filters |= BIT(HWTSTAMP_FILTER_PTP_V2_L4_EVENT);

	mask = cpu_to_le64(IONIC_PKT_CLS_PTP2_L2_SYNC);
	if ((ionic->ident.lif.eth.hwstamp_rx_filters & mask) == mask)
		info->rx_filters |= BIT(HWTSTAMP_FILTER_PTP_V2_L2_SYNC);

	mask = cpu_to_le64(IONIC_PKT_CLS_PTP2_L2_DREQ);
	if ((ionic->ident.lif.eth.hwstamp_rx_filters & mask) == mask)
		info->rx_filters |= BIT(HWTSTAMP_FILTER_PTP_V2_L2_DELAY_REQ);

	mask = cpu_to_le64(IONIC_PKT_CLS_PTP2_L2_ALL);
	if ((ionic->ident.lif.eth.hwstamp_rx_filters & mask) == mask)
		info->rx_filters |= BIT(HWTSTAMP_FILTER_PTP_V2_L2_EVENT);

	mask = cpu_to_le64(IONIC_PKT_CLS_PTP2_SYNC);
	if ((ionic->ident.lif.eth.hwstamp_rx_filters & mask) == mask)
		info->rx_filters |= BIT(HWTSTAMP_FILTER_PTP_V2_SYNC);

	mask = cpu_to_le64(IONIC_PKT_CLS_PTP2_DREQ);
	if ((ionic->ident.lif.eth.hwstamp_rx_filters & mask) == mask)
		info->rx_filters |= BIT(HWTSTAMP_FILTER_PTP_V2_DELAY_REQ);

	mask = cpu_to_le64(IONIC_PKT_CLS_PTP2_ALL);
	if ((ionic->ident.lif.eth.hwstamp_rx_filters & mask) == mask)
		info->rx_filters |= BIT(HWTSTAMP_FILTER_PTP_V2_EVENT);

	return 0;
}
#endif /* CONFIG_PTP_1588_CLOCK */

static int ionic_nway_reset(struct net_device *netdev)
{
	struct ionic_lif *lif = netdev_priv(netdev);
	struct ionic *ionic = lif->ionic;
	int err = 0;

	if (test_bit(IONIC_LIF_F_FW_RESET, lif->state))
		return -EBUSY;

	/* flap the link to force auto-negotiation */

	mutex_lock(&ionic->dev_cmd_lock);

	ionic_dev_cmd_port_state(&ionic->idev, IONIC_PORT_ADMIN_STATE_DOWN);
	err = ionic_dev_cmd_wait(ionic, DEVCMD_TIMEOUT);

	if (!err) {
		ionic_dev_cmd_port_state(&ionic->idev, IONIC_PORT_ADMIN_STATE_UP);
		err = ionic_dev_cmd_wait(ionic, DEVCMD_TIMEOUT);
	}

	mutex_unlock(&ionic->dev_cmd_lock);

	return err;
}

static int ionic_flash_device(struct net_device *netdev,
			      struct ethtool_flash *eflash)
{
	struct ionic_lif *lif = netdev_priv(netdev);

	if (eflash->region)
		return -EOPNOTSUPP;

	return ionic_firmware_fetch_and_update(lif, eflash->data);
}

static const struct ethtool_ops ionic_ethtool_ops = {
#ifdef ETHTOOL_COALESCE_USECS
	.supported_coalesce_params = ETHTOOL_COALESCE_USECS |
				     ETHTOOL_COALESCE_USE_ADAPTIVE_RX |
				     ETHTOOL_COALESCE_USE_ADAPTIVE_TX,
#endif
#ifdef HAVE_RX_PUSH
	.supported_ring_params = ETHTOOL_RING_USE_TX_PUSH |
				 ETHTOOL_RING_USE_RX_PUSH,
#endif
	.get_drvinfo		= ionic_get_drvinfo,
	.get_regs_len		= ionic_get_regs_len,
	.get_regs		= ionic_get_regs,
	.get_link		= ethtool_op_get_link,
#if (KERNEL_VERSION(6, 2, 0) <= LINUX_VERSION_CODE)
	.get_link_ext_stats	= ionic_get_link_ext_stats,
#endif
	.get_link_ksettings	= ionic_get_link_ksettings,
	.set_link_ksettings	= ionic_set_link_ksettings,
	.get_coalesce		= ionic_get_coalesce,
	.set_coalesce		= ionic_set_coalesce,
	.get_ringparam		= ionic_get_ringparam,
	.set_ringparam		= ionic_set_ringparam,
	.get_channels		= ionic_get_channels,
	.set_channels		= ionic_set_channels,
	.get_strings		= ionic_get_strings,
	.get_ethtool_stats	= ionic_get_stats,
	.get_sset_count		= ionic_get_sset_count,
	.get_priv_flags		= ionic_get_priv_flags,
	.set_priv_flags		= ionic_set_priv_flags,
	.get_rxnfc		= ionic_get_rxnfc,
	.get_rxfh_indir_size	= ionic_get_rxfh_indir_size,
	.get_rxfh_key_size	= ionic_get_rxfh_key_size,
	.get_rxfh		= ionic_get_rxfh,
	.set_rxfh		= ionic_set_rxfh,
	.get_tunable		= ionic_get_tunable,
	.set_tunable		= ionic_set_tunable,
	.get_module_info	= ionic_get_module_info,
	.get_module_eeprom	= ionic_get_module_eeprom,
	.get_pauseparam		= ionic_get_pauseparam,
	.set_pauseparam		= ionic_set_pauseparam,
#ifdef ETHTOOL_FEC_NONE
	.get_fecparam		= ionic_get_fecparam,
	.set_fecparam		= ionic_set_fecparam,
#endif
#if IS_ENABLED(CONFIG_PTP_1588_CLOCK)
	.get_ts_info		= ionic_get_ts_info,
#endif
	.nway_reset		= ionic_nway_reset,
	.flash_device	= ionic_flash_device,
};

void ionic_ethtool_set_ops(struct net_device *netdev)
{
	netdev->ethtool_ops = &ionic_ethtool_ops;
}

