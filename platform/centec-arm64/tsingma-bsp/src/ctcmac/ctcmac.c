/*
 * Centec CpuMac Ethernet Driver -- CpuMac controller implementation
 * Provides Bus interface for MIIM regs
 *
 * Author: liuht <liuht@centecnetworks.com>
 *
 * Copyright 2002-2018, Centec Networks (Suzhou) Co., Ltd.
 *
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/if_vlan.h>
#include <linux/spinlock.h>
#include <linux/mm.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_mdio.h>
#include <linux/of_platform.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/in.h>
#include <linux/net_tstamp.h>
#include <linux/of_net.h>

#include <asm/io.h>
#include "../pinctrl-ctc/pinctrl-ctc.h"
#include "../include/sysctl.h"
#include <linux/regmap.h>
#include <linux/mfd/syscon.h>
#include <linux/iopoll.h>
#include <linux/phy_fixed.h>
#include "../include/ctc5236_switch.h"

#include "ctcmac.h"
#include "ctcmac_reg.h"

static int ctcmac_alloc_skb_resources(struct net_device *ndev);
static int ctcmac_free_skb_resources(struct ctcmac_private *priv);
static void cpumac_start(struct ctcmac_private *priv);
static void cpumac_halt(struct ctcmac_private *priv);
static void ctcmac_hw_init(struct ctcmac_private *priv);
static int ctcmac_set_ffe(struct ctcmac_private *priv, u16 coefficient[]);
static int ctcmac_get_ffe(struct ctcmac_private *priv, u16 coefficient[]);
static spinlock_t global_reglock __aligned(SMP_CACHE_BYTES);
static int g_reglock_init_done = 0;
static int g_mac_unit_init_done = 0;
static struct regmap *regmap_base;
static struct ctcmac_pkt_stats g_pkt_stats[2];

static const char ctc_stat_gstrings[][ETH_GSTRING_LEN] = {
	"RX-bytes-good-ucast",
	"RX-frame-good-ucast",
	"RX-bytes-good-mcast",
	"RX-frame-good-mcast",
	"RX-bytes-good-bcast",
	"RX-frame-good-bcast",
	"RX-bytes-good-pause",
	"RX-frame-good-pause",
	"RX-bytes-good-pfc",
	"RX-frame-good-pfc",
	"RX-bytes-good-control",
	"RX-frame-good-control",
	"RX-bytes-fcs-error",
	"RX-frame-fcs-error",
	"RX-bytes-mac-overrun",
	"RX-frame-mac-overrun",
	"RX-bytes-good-63B",
	"RX-frame-good-63B",
	"RX-bytes-bad-63B",
	"RX-frame-bad-63B",
	"RX-bytes",
	"RX-frame",
	"RX-bytes-bad",
	"RX-frame-bad",
	"RX-bytes-good-jumbo",
	"RX-frame-good-jumbo",
	"RX-bytes-bad-jumbo",
	"RX-frame-bad-jumbo",
	"RX-bytes-64B",
	"RX-frame-64B",
	"RX-bytes-127B",
	"RX-frame-127B",
	"RX-bytes-255B",
	"RX-frame-255B",
	"RX-bytes-511B",
	"RX-frame-511B",
	"RX-bytes-1023B",
	"RX-frame-1023B",
	"RX-bytes",
	"RX-frame",
	"TX-bytes-ucast",
	"TX-frame-ucast",
	"TX-bytes-mcast",
	"TX-frame-mcast",
	"TX-bytes-bcast",
	"TX-frame-bcast",
	"TX-bytes-pause",
	"TX-frame-pause",
	"TX-bytes-control",
	"TX-frame-control",
	"TX-bytes-fcs-error",
	"TX-frame-fcs-error",
	"TX-bytes-underrun",
	"TX-frame-underrun",
	"TX-bytes-63B",
	"TX-frame-63B",
	"TX-bytes-64B",
	"TX-frame-64B",
	"TX-bytes-127B",
	"TX-frame-127B",
	"TX-bytes-255B",
	"TX-frame-255B",
	"TX-bytes-511B",
	"TX-frame-511B",
	"TX-bytes-1023B",
	"TX-frame-1023B",
	"TX-bytes-mtu1",
	"TX-frame-mtu1",
	"TX-bytes-mtu2",
	"TX-frame-mtu2",
	"TX-bytes-jumbo",
	"TX-frame-jumbo",
	"mtu1",
	"mtu2",
};

static void clrsetbits(unsigned __iomem * addr, u32 clr, u32 set)
{
	writel((readl(addr) & ~(clr)) | (set), addr);
}

static inline u32 ctcmac_regr(unsigned __iomem * addr)
{
	u32 val;
	val = readl(addr);
	return val;
}

static inline void ctcmac_regw(unsigned __iomem * addr, u32 val)
{
	writel(val, addr);
}

static inline int ctcmac_rxbd_unused(struct ctcmac_priv_rx_q *rxq)
{
	/* left one rx desc unused */
	if (rxq->next_to_clean > rxq->next_to_use)
		return rxq->next_to_clean - rxq->next_to_use - 1;

	return rxq->rx_ring_size + rxq->next_to_clean - rxq->next_to_use - 1;
}

static int ctcmac_alloc_tx_queues(struct ctcmac_private *priv)
{
	int i;

	for (i = 0; i < priv->num_tx_queues; i++) {
		priv->tx_queue[i] = kzalloc(sizeof(struct ctcmac_priv_tx_q),
					    GFP_KERNEL);
		if (!priv->tx_queue[i])
			return -ENOMEM;

		priv->tx_queue[i]->tx_skbuff = NULL;
		priv->tx_queue[i]->qindex = i;
		priv->tx_queue[i]->dev = priv->ndev;
		spin_lock_init(&(priv->tx_queue[i]->txlock));
	}
	return 0;
}

static int ctcmac_alloc_rx_queues(struct ctcmac_private *priv)
{
	int i;

	for (i = 0; i < priv->num_rx_queues; i++) {
		priv->rx_queue[i] = kzalloc(sizeof(struct ctcmac_priv_rx_q),
					    GFP_KERNEL);
		if (!priv->rx_queue[i])
			return -ENOMEM;

		priv->rx_queue[i]->qindex = i;
		priv->rx_queue[i]->ndev = priv->ndev;
	}
	return 0;
}

static void ctcmac_unmap_io_space(struct ctcmac_private *priv)
{
	if (priv->iobase)
		iounmap(priv->iobase);
}

static void ctcmac_free_tx_queues(struct ctcmac_private *priv)
{
	int i;

	for (i = 0; i < priv->num_tx_queues; i++) {
		if (priv->tx_queue[i])
			kfree(priv->tx_queue[i]);
	}
}

static void ctcmac_free_rx_queues(struct ctcmac_private *priv)
{
	int i;

	for (i = 0; i < priv->num_rx_queues; i++) {
		if (priv->rx_queue[i])
			kfree(priv->rx_queue[i]);
	}
}

static void ctcmac_free_dev(struct ctcmac_private *priv)
{
	if (priv->ndev)
		free_netdev(priv->ndev);
}

static int ctcmac_fixed_phy_link_update(struct net_device *dev,
					struct fixed_phy_status *status)
{
	u32 mon = 0;
	struct ctcmac_private *priv;

	if (!dev)
		return 0;

	priv = netdev_priv(dev);

	if (priv->interface != PHY_INTERFACE_MODE_SGMII)
		return 0;

	mon = readl(&priv->cpumac_reg->CpuMacSgmiiMon[0]);
	if (priv->autoneg_mode == CTCMAC_AUTONEG_DISABLE) {
		if ((mon & 0x100) == 0x100)
			status->link = 1;
		else
			status->link = 0;

	} else {
		if ((mon & CSM_ANST_MASK) == 6)
			status->link = 1;
		else
			status->link = 0;
	}

	return 0;
}

static int ctcmac_of_init(struct platform_device *ofdev,
			  struct net_device **pdev)
{
	u32 val;
	int err = 0, index, int_coalesce;
	const void *mac_addr;
	const char *ctype, *automode, *dfe, *int_type, *tx_inv, *rx_inv;
	struct net_device *dev = NULL;
	struct ctcmac_private *priv = NULL;
	unsigned int num_tx_qs, num_rx_qs;
	struct device_node *np = ofdev->dev.of_node;

	num_tx_qs = CTCMAC_TX_QUEUE_MAX;
	num_rx_qs = CTCMAC_RX_QUEUE_MAX;

	*pdev = alloc_etherdev_mq(sizeof(*priv), num_tx_qs);
	dev = *pdev;
	if (NULL == dev)
		return -ENOMEM;

	priv = netdev_priv(dev);
	priv->ndev = dev;
	priv->ofdev = ofdev;
	priv->dev = &ofdev->dev;
	priv->dev->coherent_dma_mask = DMA_BIT_MASK(64);
	priv->num_tx_queues = num_tx_qs;
	regmap_read(regmap_base, offsetof(struct SysCtl_regs, SysCtlSysRev),
		    &val);
	priv->version = val;
	netif_set_real_num_rx_queues(dev, num_rx_qs);
	priv->num_rx_queues = num_rx_qs;

	if (ctcmac_alloc_tx_queues(priv))
		goto fail;

	if (ctcmac_alloc_rx_queues(priv))
		goto fail;

	/* init cpumac_reg/cpumac_mem/cpumac_unit address */
	priv->iobase = of_iomap(np, 0);
	priv->cpumac_reg = priv->iobase + CPUMAC_REG_BASE;
	priv->cpumac_mem = priv->iobase + CPUMAC_MEM_BASE;
	priv->cpumacu_reg = of_iomap(np, 1) + CPUMACUNIT_REG_BASE;

	/* Get CpuMac index */
	err = of_property_read_u32(np, "index", &index);
	if ((err == 0))
		priv->index = index;
	else
		priv->index = 0;

	mac_addr = of_get_mac_address(np);

	if (!IS_ERR(mac_addr))
		memcpy(dev->dev_addr, mac_addr, ETH_ALEN);

	err = of_property_read_string(np, "int-type", &int_type);
	if ((err == 0) && !strncmp(int_type, "desc", 4)) {
		priv->int_type = CTCMAC_INT_DESC;
		if (priv->version == 0) {
			priv->rx_int_coalesce_cnt = DESC_INT_COALESCE_CNT_MIN;
			priv->tx_int_coalesce_cnt = DESC_INT_COALESCE_CNT_MIN;
		} else {
			err =
			    of_property_read_u32(np, "rx-int-coalesce-count",
						 &int_coalesce);
			if (err == 0)
				priv->rx_int_coalesce_cnt = int_coalesce;
			else
				priv->rx_int_coalesce_cnt =
				    DESC_RX_INT_COALESCE_CNT_DEFAULT;

			err =
			    of_property_read_u32(np, "tx-int-coalesce-count",
						 &int_coalesce);
			if (err == 0)
				priv->tx_int_coalesce_cnt = int_coalesce;
			else
				priv->tx_int_coalesce_cnt =
				    DESC_TX_INT_COALESCE_CNT_DEFAULT;
		}
	} else {
		priv->int_type = CTCMAC_INT_PACKET;
	}

	/* Get interface type, CTC5236 only support PHY_INTERFACE_MODE_SGMII */
	err = of_property_read_string(np, "phy-connection-type", &ctype);
	if ((err == 0) && !strncmp(ctype, "mii", 3)) {
		priv->interface = PHY_INTERFACE_MODE_MII;
		priv->supported = SUPPORTED_10baseT_Full;
	} else {
		priv->interface = PHY_INTERFACE_MODE_SGMII;
		priv->supported = CTCMAC_SUPPORTED;
	}

	err = of_property_read_string(np, "auto-nego-mode", &automode);
	if ((err == 0) && !strncmp(automode, "disable", 7)) {
		priv->autoneg_mode = CTCMAC_AUTONEG_DISABLE;
	} else if ((err == 0) && !strncmp(automode, "sgmii-mac", 9)) {
		priv->autoneg_mode = CTCMAC_AUTONEG_MAC_M;
	} else if ((err == 0) && !strncmp(automode, "sgmii-phy", 9)) {
		priv->autoneg_mode = CTCMAC_AUTONEG_PHY_M;
	} else if ((err == 0) && !strncmp(automode, "1000base-x", 10)) {
		priv->autoneg_mode = CTCMAC_AUTONEG_1000BASEX_M;
	} else {
		priv->autoneg_mode = CTCMAC_AUTONEG_MAC_M;
	}

	err = of_property_read_string(np, "dfe", &dfe);
	if ((err == 0) && !strncmp(dfe, "enable", 6)) {
		priv->dfe_enable = 1;
	} else {
		priv->dfe_enable = 0;
	}

	err = of_property_read_string(np, "tx-pol-inv", &tx_inv);
	if ((err == 0) && !strncmp(tx_inv, "enable", 6)) {
		priv->tx_pol_inv = CTCMAC_TX_POL_INV_ENABLE;
	} else {
		priv->tx_pol_inv = CTCMAC_TX_POL_INV_DISABLE;
	}

	err = of_property_read_string(np, "rx-pol-inv", &rx_inv);
	if ((err == 0) && !strncmp(rx_inv, "enable", 6)) {
		priv->rx_pol_inv = CTCMAC_RX_POL_INV_ENABLE;
	} else {
		priv->rx_pol_inv = CTCMAC_RX_POL_INV_DISABLE;
	}

	priv->phy_node = of_parse_phandle(np, "phy-handle", 0);
	/* In the case of a fixed PHY, the DT node associated
	 * to the PHY is the Ethernet MAC DT node.
	 */
	if (!priv->phy_node && of_phy_is_fixed_link(np)) {
		err = of_phy_register_fixed_link(np);
		if (err)
			return -1;

		priv->phy_node = of_node_get(np);
	}
	/* mapping from hw irq to sw irq */
	priv->irqinfo[CTCMAC_NORMAL].irq = irq_of_parse_and_map(np, 0);
	priv->irqinfo[CTCMAC_FUNC].irq = irq_of_parse_and_map(np, 1);
	if (priv->version > 0) {
		priv->irqinfo[CTCMAC_FUNC_RX0].irq =
		    irq_of_parse_and_map(np, 3);
		priv->irqinfo[CTCMAC_FUNC_RX1].irq =
		    irq_of_parse_and_map(np, 4);
	}

	return 0;

fail:
	ctcmac_unmap_io_space(priv);
	ctcmac_free_tx_queues(priv);
	ctcmac_free_rx_queues(priv);
	ctcmac_free_dev(priv);

	return -1;
}

int startup_ctcmac(struct net_device *ndev)
{
	struct ctcmac_private *priv = netdev_priv(ndev);
	int err;

	ctcmac_hw_init(priv);

	err = ctcmac_alloc_skb_resources(ndev);
	if (err)
		return err;

	smp_mb__before_atomic();
	clear_bit(CTCMAC_DOWN, &priv->state);
	smp_mb__after_atomic();

	cpumac_start(priv);
	/* force link state update after mac reset */
	priv->oldlink = 0;
	priv->oldspeed = 0;
	priv->oldduplex = -1;

	phy_start(ndev->phydev);

	napi_enable(&priv->napi_rx);
	napi_enable(&priv->napi_tx);
	if (priv->version > 0)
		napi_enable(&priv->napi_rx1);

	netif_tx_wake_all_queues(ndev);

	return 0;
}

void stop_ctcmac(struct net_device *ndev)
{
	struct ctcmac_private *priv = netdev_priv(ndev);

	/* disable ints and gracefully shut down Rx/Tx DMA */
	cpumac_halt(priv);

	netif_tx_stop_all_queues(ndev);

	smp_mb__before_atomic();
	set_bit(CTCMAC_DOWN, &priv->state);
	smp_mb__after_atomic();
	napi_disable(&priv->napi_rx);
	napi_disable(&priv->napi_tx);
	if (priv->version > 0)
		napi_disable(&priv->napi_rx1);
	phy_stop(ndev->phydev);
	ctcmac_free_skb_resources(priv);
}

static void ctcmac_reset(struct net_device *ndev)
{
	struct ctcmac_private *priv = netdev_priv(ndev);
	while (test_and_set_bit_lock(CTCMAC_RESETTING, &priv->state))
		cpu_relax();

	stop_ctcmac(ndev);
	startup_ctcmac(ndev);
	clear_bit_unlock(CTCMAC_RESETTING, &priv->state);
}

/* ctcmac_reset_task gets scheduled when a packet has not been
 * transmitted after a set amount of time.
 * For now, assume that clearing out all the structures, and
 * starting over will fix the problem.
 */
static void ctcmac_reset_task(struct work_struct *work)
{
	struct ctcmac_private *priv = container_of(work, struct ctcmac_private,
						   reset_task);
	ctcmac_reset(priv->ndev);
}

/* get the rxdesc number that was used by CpuMac but has not been handled by CPU */
static int ctcmac_rxbd_recycle(struct ctcmac_private *priv, int qidx)
{
	u32 count;

	if (qidx) {
		count = readl(&priv->cpumac_reg->CpuMacDescMon[2]);
		return count & 0xffff;
	}

	count = readl(&priv->cpumac_reg->CpuMacDescMon[1]);

	return (count >> 16) & 0xffff;
}

static int ctcmac_rxbd_usable(struct ctcmac_private *priv, int qidx)
{
	return (readl(&priv->cpumac_reg->CpuMacDescMon[0]) >> (qidx * 16)) &
	    0xffff;
}

/* get the txdesc number that was used by CpuMac but has not been handled by CPU */
static int ctcmac_txbd_used_untreated(struct ctcmac_private *priv)
{
	u32 count;

	count = readl(&priv->cpumac_reg->CpuMacDescMon[2]);

	return (count >> 16) & 0xffff;
}

/* Add rx buffer data to skb fragment */
static bool ctcmac_add_rx_frag(struct ctcmac_rx_buff *rxb, u32 lstatus,
			       struct sk_buff *skb, bool first)
{
	struct page *page = rxb->page;
	unsigned int size =
	    (lstatus & CPU_MAC_DESC_INTF_W1_DESC_SIZE_MASK) >> 8;
	int data_size;

	/* Remove the CRC from the packet length */
	if (lstatus & BIT(CPU_MAC_DESC_INTF_W1_DESC_EOP_BIT))
		data_size = size - 4;
	else
		data_size = size;

	if (likely(first)) {
		skb_put(skb, data_size);
	} else {
		if (data_size > 0)
			skb_add_rx_frag(skb, skb_shinfo(skb)->nr_frags, page,
					rxb->page_offset, data_size,
					CTCMAC_RXB_TRUESIZE);

		if (data_size < 0)
			pskb_trim(skb, skb->len + data_size);
	}

	/* try reuse page */
	if (unlikely(page_count(page) != 1))
		return false;

	/* change offset to the other half */
	rxb->page_offset ^= CTCMAC_RXB_TRUESIZE;

	if (data_size > 0)
		page_ref_inc(page);

	return true;
}

/* Reused page that has been release by CPU */
static void ctcmac_reuse_rx_page(struct ctcmac_priv_rx_q *rxq,
				 struct ctcmac_rx_buff *old_rxb)
{
	struct ctcmac_rx_buff *new_rxb;
	u16 nta = rxq->next_to_alloc;

	new_rxb = &rxq->rx_buff[nta];

	/* find next buf that can reuse a page */
	nta++;
	rxq->next_to_alloc = (nta < rxq->rx_ring_size) ? nta : 0;

	/* copy page reference */
	*new_rxb = *old_rxb;

	/* sync for use by the device */
	dma_sync_single_range_for_device(rxq->dev, old_rxb->dma,
					 old_rxb->page_offset,
					 CTCMAC_RXB_TRUESIZE, DMA_FROM_DEVICE);
}

/* Handle the rx buffer that has been used by CpuMac */
static struct sk_buff *ctcmac_get_next_rxbuff(struct ctcmac_priv_rx_q *rx_queue,
					      u32 lstatus, struct sk_buff *skb)
{
	struct ctcmac_rx_buff *rxb =
	    &rx_queue->rx_buff[rx_queue->next_to_clean];
	struct page *page = rxb->page;
	bool first = false;

	if (likely(!skb)) {
		void *buff_addr = page_address(page) + rxb->page_offset;
		skb = build_skb(buff_addr, CTCMAC_SKBFRAG_SIZE);
		if (unlikely(!skb)) {
			return NULL;
		}
		first = true;
	}

	dma_sync_single_range_for_cpu(rx_queue->dev, rxb->dma, rxb->page_offset,
				      CTCMAC_RXB_TRUESIZE, DMA_FROM_DEVICE);

	if (ctcmac_add_rx_frag(rxb, lstatus, skb, first)) {
		/* reuse the free half of the page */
		ctcmac_reuse_rx_page(rx_queue, rxb);
	} else {
		/* page cannot be reused, unmap it */
		dma_unmap_page(rx_queue->dev, rxb->dma,
			       PAGE_SIZE, DMA_FROM_DEVICE);
	}

	/* clear rxb content */
	rxb->page = NULL;

	return skb;
}

static void ctcmac_process_frame(struct net_device *ndev, struct sk_buff *skb)
{
	skb->protocol = eth_type_trans(skb, ndev);
}

int ctc_mac_hss_write(struct ctcmac_private *priv, u8 addr, u8 data,
		      u8 serdes_id)
{
	u8 accid;
	int timeout = 2000;
	u32 val = 0, mon = 0;

	if (serdes_id == 0)
		accid = 3;
	else if (serdes_id == 1)
		accid = 4;
	else
		accid = 0;
	val = addr | (data << 8) | (accid << 24) | (1 << 31);
	writel(val, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);

	while (timeout--) {
		mon = readl(&priv->cpumacu_reg->CpuMacUnitHssRegAccResult);
		if (mon & CPU_MAC_UNIT_HSS_REG_ACC_RESULT_W0_HSS_ACC_ACK_MASK) {
			break;
		}
		mdelay(1);
	}
	if (!(mon & CPU_MAC_UNIT_HSS_REG_ACC_RESULT_W0_HSS_ACC_ACK_MASK)) {
		dev_err(&priv->ndev->dev,
			"wait for write ack CpuMacUnitHssRegAccResult:0x%x addr 0x%x data 0x%x serdes %d fail!\n",
			readl(&priv->cpumacu_reg->CpuMacUnitHssRegAccResult),
			addr, data, serdes_id);
		return -1;
	}

	return 0;
}

int ctc_mac_hss_read(struct ctcmac_private *priv, u8 addr, u8 * data,
		     u8 serdes_id)
{
	u8 accid;
	u32 val = 0;
	u32 mon = 0;
	int timeout = 2000;;

	if (serdes_id == 0)
		accid = 3;
	else if (serdes_id == 1)
		accid = 4;
	else
		accid = 0;
	val = addr | (1 << 16) | (accid << 24) | (1 << 31);

	writel(val, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);

	while (timeout--) {
		mon = readl(&priv->cpumacu_reg->CpuMacUnitHssRegAccResult);
		if (mon & CPU_MAC_UNIT_HSS_REG_ACC_RESULT_W0_HSS_ACC_ACK_MASK) {
			break;
		}
		mdelay(1);
	}
	if (!(mon & CPU_MAC_UNIT_HSS_REG_ACC_RESULT_W0_HSS_ACC_ACK_MASK)) {
		dev_err(&priv->ndev->dev,
			"wait for read ack CpuMacUnitHssRegAccResult:0x%x fail!\n",
			readl(&priv->cpumacu_reg->CpuMacUnitHssRegAccResult));
		*data = 0x0;
		return -1;
	}

	val = readl(&priv->cpumacu_reg->CpuMacUnitHssRegAccResult);
	*data = val & 0xff;

	return 0;
}

static int ctcmac_maximize_margin_of_cmu_tempearture_ramp(struct ctcmac_private
							  *priv)
{
	u8 val, ctune, delta, ctune_cal;
	int tmpr = 0;

	get_switch_temperature();
	tmpr = get_switch_temperature();
	if (0xffff == tmpr) {
		printk(KERN_ERR "get temperature fail!\n");
		return -1;
	}

	/*r_pll_dlol_en  0x30[0] write 1    enable pll lol status output */
	ctc_mac_hss_read(priv, 0x30, &val, 2);
	val |= BIT(0);
	ctc_mac_hss_write(priv, 0x30, val, 2);

	if (tmpr <= -20) {
		delta = 2;
	} else if (tmpr <= 60) {
		delta = 1;
	} else {
		delta = 0;
	}
	/*read_vco_ctune     0xe0[3:0] read ctune raw value */
	ctc_mac_hss_read(priv, 0xe0, &val, 2);
	ctune = val & 0xf;
	ctune_cal = ctune - delta;
	/*cfg_vco_byp_ctune  0x07[3:0] write (ctune - delta) */
	ctc_mac_hss_read(priv, 0x7, &val, 2);
	val &= 0xf0;
	val |= ctune_cal;
	ctc_mac_hss_write(priv, 0x7, val, 2);

	/*cfg_vco_cal_byp    0x06[7]   write 1 */
	ctc_mac_hss_read(priv, 0x6, &val, 2);
	val |= BIT(7);
	ctc_mac_hss_write(priv, 0x6, val, 2);

	/*for temperature -40~-20C, try (ctune-1) if (ctune-2) causes lol */
	mdelay(10);
	/*pll_lol_udl        0xe0[4]   read 0 */
	val = ctc_mac_hss_read(priv, 0xe0, &val, 2);
	if ((0 != (val & BIT(4))) && (delta == 2)) {
		/*cfg_vco_byp_ctune  0x07[3:0] write (ctune - 1) */
		ctune_cal = ctune - 1;
		ctc_mac_hss_read(priv, 0x7, &val, 2);
		val &= 0xf0;
		val |= ctune_cal;
		ctc_mac_hss_write(priv, 0x7, val, 2);
	}
	/*check pll lol */
	mdelay(10);
	/*pll_lol_udl        0xe0[4]   read 0 */
	val = ctc_mac_hss_read(priv, 0xe0, &val, 2);
	if (0 != (val & BIT(4))) {
		printk(KERN_ERR
		       "maximize margin of cmu tempearture ramp fail!\n");
		return -1;
	}

	return 0;
}

/* tx/rx polarity invert */
static int ctc_mac_pol_inv(struct ctcmac_private *priv)
{
	u8 data = 0;

	if (priv->tx_pol_inv) {
		/* tx polarity will be inverted */
		ctc_mac_hss_read(priv, 0x83, &data, priv->index);
		ctc_mac_hss_write(priv, 0x83, (data | 0x2), priv->index);
	}

	if (priv->rx_pol_inv) {
		/* rx polarity will be inverted */
		ctc_mac_hss_read(priv, 0x83, &data, priv->index);
		ctc_mac_hss_write(priv, 0x83, (data | 0x8), priv->index);
	}
	return 0;
}

/* serdes init flow */
static int ctc_mac_serdes_init(struct ctcmac_private *priv)
{
	u8 val;
	int ret = 0;
	u32 status;
	int delay_ms = 10;

	if (priv->dfe_enable) {
		/* reset serdes */
		writel(0x4610b003, &priv->cpumacu_reg->CpuMacUnitHssCfg[5]);
		writel(0x4610b003, &priv->cpumacu_reg->CpuMacUnitHssCfg[11]);
		writel(0x83806000, &priv->cpumacu_reg->CpuMacUnitHssCfg[0]);
		writel(0x28061800, &priv->cpumacu_reg->CpuMacUnitHssCfg[2]);
		writel(0x0066c03a, &priv->cpumacu_reg->CpuMacUnitHssCfg[6]);
		writel(0x28061810, &priv->cpumacu_reg->CpuMacUnitHssCfg[8]);
		writel(0x0066c03a, &priv->cpumacu_reg->CpuMacUnitHssCfg[12]);
	} else {
		/* reset serdes */
		writel(0x4610a805, &priv->cpumacu_reg->CpuMacUnitHssCfg[5]);
		writel(0x4610a805, &priv->cpumacu_reg->CpuMacUnitHssCfg[11]);
		writel(0x83806000, &priv->cpumacu_reg->CpuMacUnitHssCfg[0]);
		writel(0x28061800, &priv->cpumacu_reg->CpuMacUnitHssCfg[2]);
		writel(0x0026c02a, &priv->cpumacu_reg->CpuMacUnitHssCfg[6]);
		writel(0x28061810, &priv->cpumacu_reg->CpuMacUnitHssCfg[8]);
		writel(0x0026c02a, &priv->cpumacu_reg->CpuMacUnitHssCfg[12]);
	}

	/* offset0 bit1 BlkRstN */
	writel(0x83806002, &priv->cpumacu_reg->CpuMacUnitHssCfg[0]);
	mdelay(delay_ms);

	writel(0x80002309, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x80000842, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x8000ea45, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);

	/* serdes 0 init */
	writel(0x83000a05, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x83002008, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x8300640f, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x83000214, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x83008015, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x83000116, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x83001817, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x83003018, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x83000e24, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x83008226, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x83001f27, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x83002028, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x83002829, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x8300302a, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x83002038, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x8300223a, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x8300523b, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x83002040, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x8300f141, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x8300014a, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x8300e693, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);

	/* serdes 1 init */
	writel(0x84000a05, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x84002008, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x8400640f, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x84000214, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x84008015, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x84000116, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x84001817, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x84003018, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x84000e24, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x84008226, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x84001f27, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x84002028, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x84002829, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x8400302a, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x84002038, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x8400223a, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x8400523b, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x84002040, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x8400f141, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x8400014a, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);
	writel(0x8400e693, &priv->cpumacu_reg->CpuMacUnitHssRegAccCtl);
	mdelay(delay_ms);

	ctc_mac_hss_write(priv, 0x0c, 0x21, 0);
	ctc_mac_hss_write(priv, 0x0f, 0x64, 0);
	ctc_mac_hss_write(priv, 0x1a, 0x06, 0);
	ctc_mac_hss_write(priv, 0x91, 0x30, 0);
	ctc_mac_hss_write(priv, 0x48, 0x20, 0);
	ctc_mac_hss_write(priv, 0x90, 0x30, 0);
	//ctc_mac_hss_write(priv, 0x9e, 0x36, 0);
	//ctc_mac_hss_write(priv, 0x93, 0x76, 0);
	ctc_mac_hss_write(priv, 0x14, 0x01, 0);
	ctc_mac_hss_write(priv, 0x26, 0x81, 0);

	ctc_mac_hss_write(priv, 0x0c, 0x21, 1);
	ctc_mac_hss_write(priv, 0x0f, 0x64, 1);
	ctc_mac_hss_write(priv, 0x1a, 0x06, 1);
	ctc_mac_hss_write(priv, 0x91, 0x30, 1);
	ctc_mac_hss_write(priv, 0x48, 0x20, 1);
	ctc_mac_hss_write(priv, 0x90, 0x30, 1);
	//ctc_mac_hss_write(priv, 0x9e, 0x36, 1);
	//ctc_mac_hss_write(priv, 0x93, 0x76, 1);
	ctc_mac_hss_write(priv, 0x14, 0x01, 1);
	ctc_mac_hss_write(priv, 0x26, 0x81, 1);

	ctc_mac_hss_read(priv, 0x1c, &val, 2);
	val &= 0xf8;
	val |= 0x4;
	ctc_mac_hss_write(priv, 0x1c, val, 2);

	/* serdes post release */
	writel(0x83806003, &priv->cpumacu_reg->CpuMacUnitHssCfg[0]);
	writel(0x83826003, &priv->cpumacu_reg->CpuMacUnitHssCfg[0]);

	writel(0x28061801, &priv->cpumacu_reg->CpuMacUnitHssCfg[2]);
	writel(0x28061c01, &priv->cpumacu_reg->CpuMacUnitHssCfg[2]);
	writel(0x28071c01, &priv->cpumacu_reg->CpuMacUnitHssCfg[2]);

	writel(0x28061811, &priv->cpumacu_reg->CpuMacUnitHssCfg[8]);
	writel(0x28061c11, &priv->cpumacu_reg->CpuMacUnitHssCfg[8]);
	writel(0x28071c11, &priv->cpumacu_reg->CpuMacUnitHssCfg[8]);

	ret =
	    readl_poll_timeout(&priv->cpumacu_reg->CpuMacUnitHssMon[1], status,
			       status &
			       BIT
			       (CPU_MAC_UNIT_HSS_MON_W1_MON_HSS_L0_DFE_RST_DONE_BIT),
			       1000, 2000000);
	if (ret) {
		netdev_dbg(priv->ndev,
			   "%s:wait for hss reset done fail with CpuMacUnitHssMon[1]:0x%x\n",
			   priv->ndev->name,
			   readl(&priv->cpumacu_reg->CpuMacUnitHssMon[1]));
	}
	mdelay(delay_ms);

	ctcmac_maximize_margin_of_cmu_tempearture_ramp(priv);

	return 0;
}

static void ctcmac_mac_filter_init(struct ctcmac_private *priv)
{
	unsigned char *dev_addr;
	u32 val, addr_h = 0, addr_l = 0;

	if (priv->version == 0)
		return;

	/*  mac filter table 0~3 are assigned to cpumac0
	   mac filter table 4~7 are assigned to cpumac1  
	   mac filter table 0/4:white list to receive pacekt with local mac address 
	   mac filter table 1/5:white list to receive pacekt with broadcast mac address
	   mac filter table 2/6:white list to receive pacekt with multicast mac address
	   mac filter table 3/7:white list to receive pacekt with multicast mac address
	 */

	/* 1. mac filter table 0~3 are assigned to cpumac0
	   mac filter table 4~7 are assigned to cpumac1  */
	if (priv->index == 0) {
		val = readl(&priv->cpumacu_reg->CpuMacUnitFilterCfg1[0]);
		val |= (0xf << 16);
		val &= ~(0xf0 << 16);
		writel(val, &priv->cpumacu_reg->CpuMacUnitFilterCfg1[0]);

		val = readl(&priv->cpumacu_reg->CpuMacUnitFilterCfg1[1]);
		val |=
		    BIT
		    (CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_CHECK_NORMAL_PKT_ENABLE0_BIT);
		val |=
		    BIT(CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_DROP_NORMAL_PKT0_BIT);
		writel(val, &priv->cpumacu_reg->CpuMacUnitFilterCfg1[1]);
	} else {
		val = readl(&priv->cpumacu_reg->CpuMacUnitFilterCfg1[0]);
		val |= (0xf0 << 24);
		val &= ~(0xf << 24);
		writel(val, &priv->cpumacu_reg->CpuMacUnitFilterCfg1[0]);

		val = readl(&priv->cpumacu_reg->CpuMacUnitFilterCfg1[1]);
		val |=
		    BIT
		    (CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_CHECK_NORMAL_PKT_ENABLE1_BIT);
		val |=
		    BIT(CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_DROP_NORMAL_PKT1_BIT);
		writel(val, &priv->cpumacu_reg->CpuMacUnitFilterCfg1[1]);
	}

	/* 2. enable mac filter function */
	val = readl(&priv->cpumacu_reg->CpuMacUnitFilterCfg[0]);
	val |= BIT(CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_FILTER_EN_BIT);
	val &= ~BIT(CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_CAM_IS_BLACK_LIST_BIT);
	val &= ~BIT(CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA0_BIT);
	val &= ~BIT(CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA1_BIT);
	val &= ~BIT(CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA2_BIT);
	val &= ~BIT(CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA3_BIT);
	val &= ~BIT(CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA4_BIT);
	val &= ~BIT(CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA5_BIT);
	val &= ~BIT(CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA6_BIT);
	val &= ~BIT(CPU_MAC_UNIT_FILTER_CFG_W0_CFG_MAC_ADDR_IS_SA7_BIT);
	writel(val, &priv->cpumacu_reg->CpuMacUnitFilterCfg[0]);

	/* 3. mac filter table 0/4:white list to receive pacekt with local mac address  */
	dev_addr = priv->ndev->dev_addr;
	addr_h = dev_addr[0] << 8 | dev_addr[1];
	addr_l =
	    dev_addr[2] << 24 | dev_addr[3] << 16 | dev_addr[4] << 8 |
	    dev_addr[5];
	if (priv->index == 0) {
		writel(addr_l, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[0]);
		writel(addr_h, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[1]);
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[16]);
		writel(0x0000ffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[17]);
	} else {
		writel(addr_l, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[8]);
		writel(addr_h, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[9]);
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[24]);
		writel(0x0000ffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[25]);
	}

	/* 4. mac filter table 1/5:white list to receive pacekt with broadcast mac address */
	if (priv->index == 0) {
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[2]);
		writel(0x0000ffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[3]);
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[18]);
		writel(0x0000ffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[19]);
	} else {
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[10]);
		writel(0x0000ffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[11]);
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[26]);
		writel(0x0000ffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[27]);
	}

	/* mac filter table 2/6/3/7 reserved */
	if (priv->index == 0) {
		writel(0x00000000, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[4]);
		writel(0x00000000, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[5]);
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[20]);
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[21]);

		writel(0x00000000, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[6]);
		writel(0x00000000, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[7]);
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[22]);
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[23]);
	} else {
		writel(0x00000000, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[12]);
		writel(0x00000000, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[13]);
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[28]);
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[29]);

		writel(0x00000000, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[14]);
		writel(0x00000000, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[15]);
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[30]);
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[31]);
	}

}

/* Hardware init flow */
static void ctcmac_hw_init(struct ctcmac_private *priv)
{
	int i;
	u32 val;
	int use_extram = 0;

	/* tx/rx polarity invert */
	ctc_mac_pol_inv(priv);

	/* two cpumac access the same cpumac unit register */
	spin_lock_irq(&global_reglock);
	if (priv->index == 0) {
		/* release CpuMac0 */
		clrsetbits(&priv->cpumacu_reg->CpuMacUnitResetCtl,
			   BIT(CPU_MAC_UNIT_RESET_CTL_W0_RESET_CORE_BASE_BIT),
			   BIT
			   (CPU_MAC_UNIT_RESET_CTL_W0_RESET_CORE_CPU_MAC0_BIT));
		clrsetbits(&priv->cpumacu_reg->CpuMacUnitResetCtl,
			   BIT
			   (CPU_MAC_UNIT_RESET_CTL_W0_RESET_CORE_CPU_MAC0_BIT),
			   0);
	} else {
		/* release CpuMac0 */
		clrsetbits(&priv->cpumacu_reg->CpuMacUnitResetCtl,
			   BIT(CPU_MAC_UNIT_RESET_CTL_W0_RESET_CORE_BASE_BIT),
			   BIT
			   (CPU_MAC_UNIT_RESET_CTL_W0_RESET_CORE_CPU_MAC1_BIT));
		clrsetbits(&priv->cpumacu_reg->CpuMacUnitResetCtl,
			   BIT
			   (CPU_MAC_UNIT_RESET_CTL_W0_RESET_CORE_CPU_MAC1_BIT),
			   0);
	}

	clrsetbits(&priv->cpumacu_reg->CpuMacUnitTsCfg,
		   0, BIT(CPU_MAC_UNIT_TS_CFG_W0_CFG_FORCE_S_AND_NS_EN_BIT));

	spin_unlock_irq(&global_reglock);
	mdelay(10);

	/* init CpuMac */
	clrsetbits(&priv->cpumac_reg->CpuMacInit, 0,
		   BIT(CPU_MAC_INIT_DONE_W0_INIT_DONE_BIT));
	udelay(1);

	if (priv->interface == PHY_INTERFACE_MODE_SGMII) {
		/* switch to sgmii and enable auto nego */
		val = readl(&priv->cpumac_reg->CpuMacSgmiiAutoNegCfg);
		val &= ~(CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_AN_ENABLE_MASK
			 | CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_AN_MODE_MASK);
		val |= (CSA_SGMII_MD_MASK | CSA_EN);
		writel(val, &priv->cpumac_reg->CpuMacSgmiiAutoNegCfg);
	}

	if (priv->autoneg_mode == CTCMAC_AUTONEG_DISABLE) {
		clrsetbits(&priv->cpumac_reg->CpuMacSgmiiAutoNegCfg,
			   BIT(CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_AN_ENABLE_BIT),
			   0);

	} else {
		val = readl(&priv->cpumac_reg->CpuMacSgmiiAutoNegCfg);
		val &= ~CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_AN_MODE_MASK;
		val |=
		    (priv->
		     autoneg_mode << 2 |
		     BIT(CPU_MAC_SGMII_AUTO_NEG_CFG_W0_CFG_AN_ENABLE_BIT));
		writel(val, &priv->cpumac_reg->CpuMacSgmiiAutoNegCfg);
	}
	/* disable rx link filter */
	clrsetbits(&priv->cpumac_reg->CpuMacSgmiiCfg[0],
		   BIT(CPU_MAC_SGMII_CFG_W0_CFG_MII_RX_LINK_FILTER_EN_BIT), 0);
	/* ignore tx event */
	clrsetbits(&priv->cpumac_reg->CpuMacSgmiiCfg[0],
		   0, BIT(CPU_MAC_SGMII_CFG_W0_CFG_TX_EVEN_IGNORE_BIT));

	clrsetbits(&priv->cpumac_reg->CpuMacAxiCfg,
		   0, BIT(CPU_MAC_AXI_CFG_W0_CFG_AXI_RD_D_WORD_SWAP_EN_BIT));
	clrsetbits(&priv->cpumac_reg->CpuMacAxiCfg,
		   0, BIT(CPU_MAC_AXI_CFG_W0_CFG_AXI_WR_D_WORD_SWAP_EN_BIT));

	/* drop over size packet */
	clrsetbits(&priv->cpumac_reg->CpuMacGmacCfg[0],
		   0, BIT(CPU_MAC_GMAC_CFG_W0_CFG_RX_OVERRUN_DROP_EN_BIT)
		   | BIT(CPU_MAC_GMAC_CFG_W0_CFG_RX_OVERSIZE_DROP_EN_BIT));

	/* not strip 4B crc when send packet  */
	clrsetbits(&priv->cpumac_reg->CpuMacGmacCfg[2],
		   BIT(CPU_MAC_GMAC_CFG_W2_CFG_TX_STRIP_CRC_EN_BIT), 0);

	/* enable cut-through mode */
	clrsetbits(&priv->cpumac_reg->CpuMacGmacCfg[2],
		   0, BIT(CPU_MAC_GMAC_CFG_W2_CFG_TX_CUT_THROUGH_EN_BIT));

	for (i = 0; i < priv->num_tx_queues; i++) {
		if (priv->tx_queue[i]->tx_ring_size > CTCMAC_INTERNAL_RING_SIZE) {
			use_extram = 1;
			break;
		}
	}

	for (i = 0; i < priv->num_rx_queues; i++) {
		if (priv->rx_queue[i]->rx_ring_size > CTCMAC_INTERNAL_RING_SIZE) {
			use_extram = 1;
			break;
		}
	}

	if (use_extram) {
		spin_lock_irq(&global_reglock);
		/* enable external SRAM to store rx/tx desc, support max 1023*3 desc */
		regmap_read(regmap_base,
			    offsetof(struct SysCtl_regs, SysMemCtl), &val);
		val |= SYS_MEM_CTL_W0_CFG_RAM_MUX_EN;
		regmap_write(regmap_base,
			     offsetof(struct SysCtl_regs, SysMemCtl), val);
		spin_unlock_irq(&global_reglock);

		if (priv->index == 0) {
			ctcmac_regw(&priv->cpumac_reg->CpuMacExtRamCfg[1],
				    CTCMAC0_EXSRAM_BASE);
		} else {
			ctcmac_regw(&priv->cpumac_reg->CpuMacExtRamCfg[1],
				    CTCMAC1_EXSRAM_BASE);
		}
		ctcmac_regw(&priv->cpumac_reg->CpuMacExtRamCfg[0],
			    CTCMAC_TX_RING_SIZE);
		clrsetbits(&priv->cpumac_reg->CpuMacExtRamCfg[0], 0,
			   BIT(CPU_MAC_EXT_RAM_CFG_W0_CFG_EXT_RAM_EN_BIT));
	} else {
		/* disable external SRAM to store rx/tx desc, support max 64*3 desc */
		clrsetbits(&priv->cpumac_reg->CpuMacExtRamCfg[0],
			   BIT(CPU_MAC_EXT_RAM_CFG_W0_CFG_EXT_RAM_EN_BIT), 0);
		spin_lock_irq(&global_reglock);

		regmap_read(regmap_base,
			    offsetof(struct SysCtl_regs, SysMemCtl), &val);
		val &= ~SYS_MEM_CTL_W0_CFG_RAM_MUX_EN;
		regmap_write(regmap_base,
			     offsetof(struct SysCtl_regs, SysMemCtl), val);
		spin_unlock_irq(&global_reglock);
	}

	if (priv->int_type == CTCMAC_INT_DESC) {
		val = BIT(CPU_MAC_DESC_CFG_W0_CFG_TX_DESC_ACK_EN_BIT)
		    | (priv->rx_int_coalesce_cnt << 16)
		    | (priv->rx_int_coalesce_cnt << 8)
		    | (priv->tx_int_coalesce_cnt << 0);
		ctcmac_regw(&priv->cpumac_reg->CpuMacDescCfg[0], val);
		if (priv->version > 0) {
			val = ctcmac_regr(&priv->cpumac_reg->CpuMacDescCfg1[0]);
			val |=
			    (1 <<
			     CPU_MAC_DESC_CFG1_W0_CFG_RX_DESC_DONE_INTR_TIMER_EN)
			    | (1 <<
			       CPU_MAC_DESC_CFG1_W0_CFG_TX_DESC_DONE_INTR_TIMER_EN);
			ctcmac_regw(&priv->cpumac_reg->CpuMacDescCfg1[0], val);
			ctcmac_regw(&priv->cpumac_reg->CpuMacDescCfg1[1],
				    CTCMAC_TIMER_THRD);
			ctcmac_regw(&priv->cpumac_reg->CpuMacDescCfg1[2],
				    CTCMAC_TIMER_THRD);
		}
	} else {
		val = BIT(CPU_MAC_DESC_CFG_W0_CFG_TX_DESC_DONE_INTR_EOP_EN_BIT)
		    | BIT(CPU_MAC_DESC_CFG_W0_CFG_RX_DESC_DONE_INTR_EOP_EN_BIT)
		    | BIT(CPU_MAC_DESC_CFG_W0_CFG_TX_DESC_ACK_EN_BIT);
		ctcmac_regw(&priv->cpumac_reg->CpuMacDescCfg[0], val);
	}

	ctcmac_mac_filter_init(priv);

	/* clear and mask all interrupt */
	ctcmac_regw(&priv->cpumac_reg->CpuMacInterruptFunc[1], 0xffffffff);
	ctcmac_regw(&priv->cpumac_reg->CpuMacInterruptNormal[1], 0xffffffff);
	ctcmac_regw(&priv->cpumac_reg->CpuMacInterruptFunc[2], 0xffffffff);
	ctcmac_regw(&priv->cpumac_reg->CpuMacInterruptNormal[2], 0xffffffff);

	if (priv->version > 0) {
		ctcmac_regw(&priv->cpumac_reg->CpuMacInterruptFunc0[1],
			    0xffffffff);
		ctcmac_regw(&priv->cpumac_reg->CpuMacInterruptFunc1[1],
			    0xffffffff);
		ctcmac_regw(&priv->cpumac_reg->CpuMacInterruptFunc0[2],
			    0xffffffff);
		ctcmac_regw(&priv->cpumac_reg->CpuMacInterruptFunc1[2],
			    0xffffffff);
	}
#if 0
	if (!(priv->ndev->flags & IFF_PROMISC)) {
		ctcmac_init_mac_filter(priv);
	}
#endif

}

static int ctcmac_wait_for_linkup(struct ctcmac_private *priv)
{
	int timeout = 3000;
	u32 mon = 0;

	if (priv->autoneg_mode == CTCMAC_AUTONEG_DISABLE) {
		/* wait for linkup */
		while (timeout--) {
			mon = readl(&priv->cpumac_reg->CpuMacSgmiiMon[0]);
			if ((mon & 0x100) == 0x100) {
				break;
			}

			mdelay(1);
		}
		if ((mon & 0x100) != 0x100) {
			printk
			    ("Error! when phy link up, link status %d is not right.\n",
			     mon);
			return -1;
		}

	} else {
		/* wait for sgmii auto nego complete */
		while (timeout--) {
			mon = readl(&priv->cpumac_reg->CpuMacSgmiiMon[0]);
			if ((mon & CSM_ANST_MASK) == 6) {
				break;
			}

			mdelay(1);
		}

		if ((mon & CSM_ANST_MASK) != 6) {
			printk
			    ("Error! when phy link up, auto-neg status %d is not right.\n",
			     mon);
			return -1;
		}
	}

	return 0;
}

static void ctcmac_cfg_flow_ctrl(struct ctcmac_private *priv, u8 tx_pause_en,
				 u8 rx_pause_en)
{
	u32 val;

	val = 1 << CPU_MAC_PAUSE_CFG_W0_CFG_RX_PAUSE_TIMER_DEC_VALUE_BIT;
	writel(val, &priv->cpumac_reg->CpuMacPauseCfg[0]);
	val = (0xffff << CPU_MAC_PAUSE_CFG_W1_CFG_TX_PAUSE_TIMER_ADJ_VALUE_BIT)
	    | (1 << CPU_MAC_PAUSE_CFG_W1_CFG_TX_PAUSE_TIMER_DEC_VALUE_BIT);
	writel(val, &priv->cpumac_reg->CpuMacPauseCfg[1]);

	if (tx_pause_en || rx_pause_en)
		val = 0xff;
	else
		val = 0x800000ff;
	writel(val, &priv->cpumacu_reg->CpuMacUnitRefPulseCfg[0]);

	if (tx_pause_en)
		clrsetbits(&priv->cpumac_reg->CpuMacPauseCfg[1], 0,
			   BIT(CPU_MAC_PAUSE_CFG_W1_CFG_TX_PAUSE_EN_BIT));
	if (rx_pause_en) {
		clrsetbits(&priv->cpumac_reg->CpuMacPauseCfg[0], 0,
			   BIT(CPU_MAC_PAUSE_CFG_W0_CFG_RX_NORM_PAUSE_EN_BIT));
		clrsetbits(&priv->cpumac_reg->CpuMacGmacCfg[2], 0,
			   BIT(CPU_MAC_GMAC_CFG_W2_CFG_TX_PAUSE_STALL_EN_BIT));
	}
}

/*                            IEEE 802.3-2000
*                              Table 28B-3
*                            Pause Resolution
*
*      Local           Remote       Local Resolution   Remote Resolution
*  PAUSE  ASM_DIR   PAUSE ASM_DIR      TX    RX          TX    RX
*  
*    0       0        x      x         0     0           0     0
*    0       1        0      x         0     0           0     0
*    0       1        1      0         0     0           0     0
*    0       1        1      1         1     0           0     1
*    1       0        0      x         0     0           0     0
*    1       x        1      x         1     1           1     1
*    1       1        0      0         0     0           0     0
*    1       1        0      1         0     1           1     0
*
*/
static void ctcmac_adjust_flow_ctrl(struct ctcmac_private *priv)
{
	struct net_device *ndev = priv->ndev;
	struct phy_device *phydev = ndev->phydev;

	if (!phydev->duplex)
		return;

	if (!priv->pause_aneg_en) {
		ctcmac_cfg_flow_ctrl(priv, priv->tx_pause_en,
				     priv->rx_pause_en);
	} else {
		u16 lcl_adv, rmt_adv;
		u8 flowctrl;
		/* get link partner capabilities */
		rmt_adv = 0;
		if (phydev->pause)
			rmt_adv = LPA_PAUSE_CAP;
		if (phydev->asym_pause)
			rmt_adv |= LPA_PAUSE_ASYM;

		lcl_adv = linkmode_adv_to_lcl_adv_t(phydev->advertising);
		flowctrl = mii_resolve_flowctrl_fdx(lcl_adv, rmt_adv);
		ctcmac_cfg_flow_ctrl(priv, flowctrl & FLOW_CTRL_TX,
				     flowctrl & FLOW_CTRL_RX);
	}
}

/* update cpumac speed when phy linkup speed changed */
static noinline void ctcmac_update_link_state(struct ctcmac_private *priv,
					      struct phy_device *phydev)
{
	u32 cfg_rep, cfg_smp;
	int speed = phydev->speed;

	if (priv->interface != PHY_INTERFACE_MODE_SGMII)
		return;

	if (netif_msg_link(priv)) {
		netdev_dbg(priv->ndev, "link up speed is %d\n", speed);
	}

	if (phydev->link) {
		cfg_rep = readl(&priv->cpumac_reg->CpuMacSgmiiCfg[0]);
		cfg_smp = readl(&priv->cpumac_reg->CpuMacSgmiiCfg[1]);
		cfg_rep &= ~CSC_REP_MASK;
		cfg_smp &= ~CSC_SMP_MASK;
		if (speed == 1000) {
			cfg_rep |= CSC_1000M;
			cfg_smp |= CSC_1000M;
		} else if (speed == 100) {
			cfg_rep |= CSC_100M;
			cfg_smp |= CSC_100M;
		} else if (speed == 10) {
			cfg_rep |= CSC_10M;
			cfg_smp |= CSC_10M;
		} else {
			return;
		}
		writel(cfg_rep, &priv->cpumac_reg->CpuMacSgmiiCfg[0]);
		writel(cfg_smp, &priv->cpumac_reg->CpuMacSgmiiCfg[1]);

		ctcmac_adjust_flow_ctrl(priv);
		ctcmac_wait_for_linkup(priv);

		if (!priv->oldlink)
			priv->oldlink = 1;

	} else {
		priv->oldlink = 0;
		priv->oldspeed = 0;
		priv->oldduplex = -1;
	}

	return;
}

static void adjust_link(struct net_device *dev)
{
	struct ctcmac_private *priv = netdev_priv(dev);
	struct phy_device *phydev = dev->phydev;

	if (unlikely(phydev->link != priv->oldlink ||
		     (phydev->link && (phydev->duplex != priv->oldduplex ||
				       phydev->speed != priv->oldspeed))))
		ctcmac_update_link_state(priv, phydev);
}

/* Initializes driver's PHY state, and attaches to the PHY.
 * Returns 0 on success.
 */
static int ctcmac_init_phy(struct net_device *dev)
{
	int err;
	__ETHTOOL_DECLARE_LINK_MODE_MASK(mask) = {
	0,};
	struct ctcmac_private *priv = netdev_priv(dev);
	phy_interface_t interface;
	struct phy_device *phydev;
	const int phy_10_features_array[2] = {
		ETHTOOL_LINK_MODE_10baseT_Half_BIT,
		ETHTOOL_LINK_MODE_10baseT_Full_BIT,
	};

	linkmode_set_bit_array(phy_10_features_array,
			       ARRAY_SIZE(phy_10_features_array), mask);
	linkmode_set_bit(ETHTOOL_LINK_MODE_Autoneg_BIT, mask);
	linkmode_set_bit(ETHTOOL_LINK_MODE_MII_BIT, mask);
	if (priv->supported & SUPPORTED_100baseT_Full) {
		linkmode_set_bit(ETHTOOL_LINK_MODE_100baseT_Half_BIT, mask);
		linkmode_set_bit(ETHTOOL_LINK_MODE_100baseT_Full_BIT, mask);
	}
	if (priv->supported & SUPPORTED_1000baseT_Full) {
		linkmode_set_bit(ETHTOOL_LINK_MODE_1000baseT_Half_BIT, mask);
		linkmode_set_bit(ETHTOOL_LINK_MODE_1000baseT_Full_BIT, mask);
	}
	priv->oldlink = 0;
	priv->oldspeed = 0;
	priv->oldduplex = -1;

	interface = priv->interface;

	phydev = of_phy_connect(dev, priv->phy_node, &adjust_link, 0,
				interface);
	if (!phydev) {
		dev_err(&dev->dev, "could not attach to PHY\n");
		return -ENODEV;
	}
	if (of_phy_is_fixed_link(priv->phy_node)) {
		err = fixed_phy_set_link_update(dev->phydev,
						ctcmac_fixed_phy_link_update);
		if (err)
			dev_err(&priv->ndev->dev, "Set link update fail!\n");
	}

	/* Remove any features not supported by the controller */
	linkmode_and(phydev->supported, phydev->supported, mask);
	linkmode_copy(phydev->advertising, phydev->supported);

	/* Add support for flow control */
	phy_support_asym_pause(phydev);

	return 0;
}

static irqreturn_t ctcmac_receive(int irq, struct ctcmac_private *priv)
{
	unsigned long flags;

	if (likely(napi_schedule_prep(&priv->napi_rx))) {
		/* disable interrupt */
		spin_lock_irqsave(&priv->reglock, flags);
		writel(CTCMAC_NOR_RX0_D | CTCMAC_NOR_RX1_D,
		       &priv->cpumac_reg->CpuMacInterruptFunc[2]);
		spin_unlock_irqrestore(&priv->reglock, flags);
		__napi_schedule(&priv->napi_rx);
	} else {
		/* clear interrupt */
		writel(CTCMAC_NOR_RX0_D | CTCMAC_NOR_RX1_D,
		       &priv->cpumac_reg->CpuMacInterruptFunc[1]);
	}

	return IRQ_HANDLED;
}

static irqreturn_t ctcmac_receive0(int irq, struct ctcmac_private *priv)
{
	if (likely(napi_schedule_prep(&priv->napi_rx))) {
		/* disable interrupt */
		writel(CTCMAC_FUNC0_RX_D,
		       &priv->cpumac_reg->CpuMacInterruptFunc0[2]);
		__napi_schedule(&priv->napi_rx);
	} else {
		/* clear interrupt */
		writel(CTCMAC_FUNC0_RX_D,
		       &priv->cpumac_reg->CpuMacInterruptFunc0[1]);
	}

	return IRQ_HANDLED;
}

static irqreturn_t ctcmac_receive1(int irq, struct ctcmac_private *priv)
{
	if (likely(napi_schedule_prep(&priv->napi_rx1))) {
		/* disable interrupt */
		writel(CTCMAC_FUNC1_RX_D,
		       &priv->cpumac_reg->CpuMacInterruptFunc1[2]);
		__napi_schedule(&priv->napi_rx1);
	} else {
		/* clear interrupt */
		writel(CTCMAC_FUNC1_RX_D,
		       &priv->cpumac_reg->CpuMacInterruptFunc1[1]);
	}

	return IRQ_HANDLED;
}

static irqreturn_t ctcmac_transmit(int irq, struct ctcmac_private *priv)
{
	unsigned long flags;

	if (likely(napi_schedule_prep(&priv->napi_tx))) {
		/* disable interrupt */
		spin_lock_irqsave(&priv->reglock, flags);
		writel(CTCMAC_NOR_TX_D,
		       &priv->cpumac_reg->CpuMacInterruptFunc[2]);
		spin_unlock_irqrestore(&priv->reglock, flags);
		__napi_schedule(&priv->napi_tx);

	} else {
		/* clear interrupt */
		writel(CTCMAC_NOR_TX_D,
		       &priv->cpumac_reg->CpuMacInterruptFunc[1]);
	}

	return IRQ_HANDLED;
}

static irqreturn_t ctcmac_func(int irq, void *data)
{
	u32 event, stat, mask;
	struct ctcmac_private *priv = (struct ctcmac_private *)data;

	stat = ctcmac_regr(&priv->cpumac_reg->CpuMacInterruptFunc[0]);
	mask = ctcmac_regr(&priv->cpumac_reg->CpuMacInterruptFunc[2]);
	event = stat & ~mask;

	if (netif_msg_intr(priv)) {
		netdev_dbg(priv->ndev,
			   "function interrupt stat 0x%x mask 0x%x\n", stat,
			   mask);
	}
	if ((event & CTCMAC_NOR_RX0_D) || (event & CTCMAC_NOR_RX1_D)) {
		ctcmac_receive(irq, priv);
	}

	if (event & CTCMAC_NOR_TX_D) {
		ctcmac_transmit(irq, priv);
	}

	return IRQ_HANDLED;
}

static irqreturn_t ctcmac_tx_isr(int irq, void *data)
{
	struct ctcmac_private *priv = (struct ctcmac_private *)data;

	ctcmac_transmit(irq, priv);

	return IRQ_HANDLED;
}

static irqreturn_t ctcmac_rx0_isr(int irq, void *data)
{
	struct ctcmac_private *priv = (struct ctcmac_private *)data;

	ctcmac_receive0(irq, priv);

	return IRQ_HANDLED;
}

static irqreturn_t ctcmac_rx1_isr(int irq, void *data)
{
	struct ctcmac_private *priv = (struct ctcmac_private *)data;

	ctcmac_receive1(irq, priv);

	return IRQ_HANDLED;
}

/* not used */
static irqreturn_t ctcmac_normal(int irq, void *data)	//TODO by liuht
{
	u32 stat, mask;
	struct ctcmac_private *priv = (struct ctcmac_private *)data;

	stat = ctcmac_regr(&priv->cpumac_reg->CpuMacInterruptFunc[0]);
	mask = ctcmac_regr(&priv->cpumac_reg->CpuMacInterruptFunc[2]);

	if (netif_msg_intr(priv)) {
		netdev_dbg(priv->ndev, "normal interrupt stat 0x%x mask 0x%x\n",
			   stat, mask);
	}

	return IRQ_HANDLED;
}

static int ctcmac_request_irq(struct ctcmac_private *priv)
{
	int err = 0;

	err = request_irq(priv->irqinfo[CTCMAC_NORMAL].irq, ctcmac_normal, 0,
			  priv->irqinfo[CTCMAC_NORMAL].name, priv);
	if (err < 0) {
		free_irq(priv->irqinfo[CTCMAC_NORMAL].irq, priv);
	}

	enable_irq_wake(priv->irqinfo[CTCMAC_NORMAL].irq);
	if (priv->version == 0) {
		err =
		    request_irq(priv->irqinfo[CTCMAC_FUNC].irq, ctcmac_func, 0,
				priv->irqinfo[CTCMAC_FUNC].name, priv);
		if (err < 0) {
			free_irq(priv->irqinfo[CTCMAC_FUNC].irq, priv);
		}
		enable_irq_wake(priv->irqinfo[CTCMAC_FUNC].irq);
	} else {
		err =
		    request_irq(priv->irqinfo[CTCMAC_FUNC].irq, ctcmac_tx_isr,
				0, priv->irqinfo[CTCMAC_FUNC].name, priv);
		if (err < 0) {
			free_irq(priv->irqinfo[CTCMAC_FUNC].irq, priv);
		}
		irq_set_affinity_hint(priv->irqinfo[CTCMAC_FUNC].irq,
				      get_cpu_mask(0));
		enable_irq_wake(priv->irqinfo[CTCMAC_FUNC].irq);
		err =
		    request_irq(priv->irqinfo[CTCMAC_FUNC_RX0].irq,
				ctcmac_rx0_isr, 0,
				priv->irqinfo[CTCMAC_FUNC_RX0].name, priv);
		if (err < 0) {
			free_irq(priv->irqinfo[CTCMAC_FUNC_RX0].irq, priv);
		}
		irq_set_affinity_hint(priv->irqinfo[CTCMAC_FUNC_RX0].irq,
				      get_cpu_mask(0));
		enable_irq_wake(priv->irqinfo[CTCMAC_FUNC_RX0].irq);
		err =
		    request_irq(priv->irqinfo[CTCMAC_FUNC_RX1].irq,
				ctcmac_rx1_isr, 0,
				priv->irqinfo[CTCMAC_FUNC_RX1].name, priv);
		if (err < 0) {
			free_irq(priv->irqinfo[CTCMAC_FUNC_RX1].irq, priv);
		}
		irq_set_affinity_hint(priv->irqinfo[CTCMAC_FUNC_RX1].irq,
				      get_cpu_mask(1));
		enable_irq_wake(priv->irqinfo[CTCMAC_FUNC_RX1].irq);
	}

	return err;
}

static void ctcmac_free_irq(struct ctcmac_private *priv)
{
	free_irq(priv->irqinfo[CTCMAC_NORMAL].irq, priv);
	if (priv->version == 0) {
		free_irq(priv->irqinfo[CTCMAC_FUNC].irq, priv);
	} else {
		irq_set_affinity_hint(priv->irqinfo[CTCMAC_FUNC].irq, NULL);
		irq_set_affinity_hint(priv->irqinfo[CTCMAC_FUNC_RX0].irq, NULL);
		irq_set_affinity_hint(priv->irqinfo[CTCMAC_FUNC_RX1].irq, NULL);
		free_irq(priv->irqinfo[CTCMAC_FUNC].irq, priv);
		free_irq(priv->irqinfo[CTCMAC_FUNC_RX0].irq, priv);
		free_irq(priv->irqinfo[CTCMAC_FUNC_RX1].irq, priv);
	}
}

static bool ctcmac_new_page(struct ctcmac_priv_rx_q *rxq,
			    struct ctcmac_rx_buff *rxb)
{
	struct page *page;
	dma_addr_t addr;

	page = dev_alloc_page();
	if (unlikely(!page))
		return false;

	addr = dma_map_page(rxq->dev, page, 0, PAGE_SIZE, DMA_FROM_DEVICE);
	if (unlikely(dma_mapping_error(rxq->dev, addr))) {
		__free_page(page);

		return false;
	}

	rxb->dma = addr;
	rxb->page = page;
	rxb->page_offset = 0;

	return true;
}

static void ctcmac_fill_rxbd(struct ctcmac_private *priv,
			     struct ctcmac_rx_buff *rxb, int qidx)
{
	u32 desc_cfg_low, desc_cfg_high;
	dma_addr_t bufaddr = rxb->dma + rxb->page_offset;

	/* DDR base address is 0 for CpuMac, but is 0x80000000 for CPU */
	desc_cfg_low =
	    (bufaddr - CTC_DDR_BASE) & CPU_MAC_DESC_INTF_W0_DESC_ADDR_31_0_MASK;
	/* CPU_MAC_DESC_INTF_W1_DESC_SIZE:bit(8) */
	desc_cfg_high = (CTCMAC_RXB_SIZE << 8) |
	    (((bufaddr -
	       CTC_DDR_BASE) >> 32) &
	     CPU_MAC_DESC_INTF_W1_DESC_ADDR_39_32_MASK);

	spin_lock_irq(&priv->reglock);
	if (qidx) {
		ctcmac_regw(&priv->cpumac_mem->CpuMacDescIntf1[0],
			    desc_cfg_low);
		smp_mb__before_atomic();
		ctcmac_regw(&priv->cpumac_mem->CpuMacDescIntf1[1],
			    desc_cfg_high);

	} else {
		ctcmac_regw(&priv->cpumac_mem->CpuMacDescIntf0[0],
			    desc_cfg_low);
		smp_mb__before_atomic();
		ctcmac_regw(&priv->cpumac_mem->CpuMacDescIntf0[1],
			    desc_cfg_high);
	}

	spin_unlock_irq(&priv->reglock);
}

static void ctcmac_fill_txbd(struct ctcmac_private *priv,
			     struct ctcmac_desc_cfg *txdesc)
{
	u32 desc_cfg_low, desc_cfg_high;

	desc_cfg_low = txdesc->addr_low;
	/* CPU_MAC_DESC_INTF_W1_DESC_SIZE:bit(8) */
	/* CPU_MAC_DESC_INTF_W1_DESC_SOP:bit(22) */
	/* CPU_MAC_DESC_INTF_W1_DESC_EOP:bit(23) */
	desc_cfg_high = txdesc->addr_high |
	    (txdesc->size << 8) | (txdesc->sop << 22) | (txdesc->eop << 23);

	spin_lock_irq(&priv->reglock);
	ctcmac_regw(&priv->cpumac_mem->CpuMacDescIntf2[0], desc_cfg_low);
	smp_mb__before_atomic();
	ctcmac_regw(&priv->cpumac_mem->CpuMacDescIntf2[1], desc_cfg_high);

	spin_unlock_irq(&priv->reglock);
}

/* reclaim tx desc */
static void ctcmac_get_txbd(struct ctcmac_private *priv)
{
	u32 lstatus;

	spin_lock_irq(&priv->reglock);
	lstatus = ctcmac_regr(&priv->cpumac_mem->CpuMacDescIntf2[0]);
	smp_mb__before_atomic();
	lstatus = ctcmac_regr(&priv->cpumac_mem->CpuMacDescIntf2[1]);
	spin_unlock_irq(&priv->reglock);
}

/* reclaim rx desc */
static void ctcmac_get_rxbd(struct ctcmac_private *priv, u32 * lstatus,
			    int qidx)
{
	spin_lock_irq(&priv->reglock);
	if (qidx) {
		ctcmac_regr(&priv->cpumac_mem->CpuMacDescIntf1[0]);
		*lstatus = ctcmac_regr(&priv->cpumac_mem->CpuMacDescIntf1[1]);
	} else {
		ctcmac_regr(&priv->cpumac_mem->CpuMacDescIntf0[0]);
		*lstatus = ctcmac_regr(&priv->cpumac_mem->CpuMacDescIntf0[1]);
	}
	smp_mb__before_atomic();

	spin_unlock_irq(&priv->reglock);
}

/* alloc rx buffer for rx desc */
static void ctcmac_alloc_rx_buffs(struct ctcmac_priv_rx_q *rx_queue,
				  int alloc_cnt)
{
	int i, j;
	int qidx = rx_queue->qindex;
	struct ctcmac_rx_buff *rxb;
	struct net_device *ndev = rx_queue->ndev;
	struct ctcmac_private *priv = netdev_priv(ndev);

	i = rx_queue->next_to_use;
	j = rx_queue->next_to_alloc;
	rxb = &rx_queue->rx_buff[i];

	while (alloc_cnt--) {
		/* if rx buffer is unmapped,  alloc new pages */
		if (unlikely(!rxb->page)) {
			if (unlikely(!ctcmac_new_page(rx_queue, rxb))) {
				break;
			}
			if (unlikely(++j == rx_queue->rx_ring_size)) {
				j = 0;
			}
		}

		/* fill rx desc */
		ctcmac_fill_rxbd(priv, rxb, qidx);
		rxb++;

		if (unlikely(++i == rx_queue->rx_ring_size)) {
			i = 0;
			rxb = rx_queue->rx_buff;
		}
	}

	rx_queue->next_to_use = i;
	rx_queue->next_to_alloc = j;
}

static void ctcmac_alloc_one_rx_buffs(struct ctcmac_priv_rx_q *rx_queue)
{
	int i, j;
	int qidx = rx_queue->qindex;
	struct ctcmac_rx_buff *rxb;
	struct net_device *ndev = rx_queue->ndev;
	struct ctcmac_private *priv = netdev_priv(ndev);

	i = rx_queue->next_to_use;
	j = rx_queue->next_to_alloc;
	rxb = &rx_queue->rx_buff[i];

	if (unlikely(!rxb->page)) {
		if (unlikely(!ctcmac_new_page(rx_queue, rxb))) {
			return;
		}
		if (unlikely(++j == rx_queue->rx_ring_size)) {
			j = 0;
		}
	}

	if (unlikely(++i == rx_queue->rx_ring_size)) {
		i = 0;
	}

	rx_queue->next_to_use = i;
	rx_queue->next_to_alloc = j;

	/* fill rx desc */
	ctcmac_fill_rxbd(priv, rxb, qidx);
}

static noinline int ctcmac_clean_rx_ring(struct ctcmac_priv_rx_q *rx_queue,
					 int rx_work_limit)
{
	struct net_device *ndev = rx_queue->ndev;
	struct ctcmac_private *priv = netdev_priv(ndev);
	int i, howmany = 0;
	struct sk_buff *skb = rx_queue->skb;
	int cleaned_cnt = ctcmac_rxbd_unused(rx_queue);
	unsigned int total_bytes = 0, total_pkts = 0;
	int qidx = rx_queue->qindex;
	u32 alloc_new;
	int budget = rx_work_limit;

	/* Get the first full descriptor */
	i = rx_queue->next_to_clean;

	while (rx_work_limit--) {
		u32 lstatus;

		if (!rx_queue->pps_limit) {
			if (cleaned_cnt >= CTCMAC_RX_BUFF_ALLOC) {
				ctcmac_alloc_rx_buffs(rx_queue, cleaned_cnt);
				cleaned_cnt = 0;
			}
		} else {
			alloc_new = rx_queue->token / CTCMAC_TOKEN_PER_PKT;
			if ((cleaned_cnt >= CTCMAC_RX_BUFF_ALLOC) && alloc_new) {
				alloc_new = min(cleaned_cnt, (int)alloc_new);
				ctcmac_alloc_rx_buffs(rx_queue, alloc_new);
				rx_queue->token -=
				    CTCMAC_TOKEN_PER_PKT * alloc_new;
				cleaned_cnt -= alloc_new;
			}
		}

		if (ctcmac_rxbd_recycle(priv, qidx) <= 0)
			break;

		ctcmac_get_rxbd(priv, &lstatus, qidx);

		/* fetch next to clean buffer from the ring */
		skb = ctcmac_get_next_rxbuff(rx_queue, lstatus, skb);
		if (unlikely(!skb))
			break;

		cleaned_cnt++;
		howmany++;

		if (unlikely(++i == rx_queue->rx_ring_size))
			i = 0;

		rx_queue->next_to_clean = i;

		if (netif_msg_rx_status(priv)) {
			netdev_dbg(priv->ndev,
				   "%s rxbuf allocted %d used %d clean %d\n",
				   ndev->name, rx_queue->next_to_alloc,
				   rx_queue->next_to_use,
				   rx_queue->next_to_clean);
		}

		/* fetch next buffer if not the last in frame */
		if (!(lstatus & BIT(CPU_MAC_DESC_INTF_W1_DESC_EOP_BIT))) {
			if (rx_queue->pps_limit)
				rx_queue->token += CTCMAC_TOKEN_PER_PKT;
			continue;
		}

		if (unlikely(lstatus & BIT(CPU_MAC_DESC_INTF_W1_DESC_ERR_BIT))) {
			/* discard faulty buffer */
			dev_kfree_skb(skb);
			skb = NULL;
			rx_queue->stats.rx_dropped++;
			if (netif_msg_rx_err(priv)) {
				netdev_dbg(priv->ndev,
					   "%s: Error with rx desc status 0x%x\n",
					   ndev->name, lstatus);
			}
			continue;
		}

		skb_record_rx_queue(skb, rx_queue->qindex);
		ctcmac_process_frame(ndev, skb);
		if ((priv->version == 0) && !(ndev->flags & IFF_PROMISC)
		    && (skb->pkt_type == PACKET_OTHERHOST)) {
			/* discard  */
			dev_kfree_skb(skb);
			skb = NULL;
			rx_queue->stats.rx_dropped++;
			continue;
		}
		/* Increment the number of packets */
		total_pkts++;
		total_bytes += skb->len + ETH_HLEN;
		/* Send the packet up the stack */
		if (qidx == 0)
			napi_gro_receive(&priv->napi_rx, skb);
		else
			napi_gro_receive(&priv->napi_rx1, skb);

		skb = NULL;
	}

	/* Store incomplete frames for completion */
	rx_queue->skb = skb;

	rx_queue->stats.rx_packets += total_pkts;
	rx_queue->stats.rx_bytes += total_bytes;

	if (!rx_queue->pps_limit && cleaned_cnt) {
		ctcmac_alloc_rx_buffs(rx_queue, cleaned_cnt);
	}

	if (rx_queue->pps_limit && rx_queue->skb) {
		return budget;
	}

	return howmany;
}

static void ctcmac_clean_tx_ring(struct ctcmac_priv_tx_q *tx_queue)
{
	u16 skb_dirty, desc_dirty;
	int tqi = tx_queue->qindex, nr_txbds, txbd_index;
	struct sk_buff *skb;
	struct netdev_queue *txq;
	struct ctcmac_tx_buff *tx_buff;
	struct net_device *dev = tx_queue->dev;
	struct ctcmac_private *priv = netdev_priv(dev);

	txq = netdev_get_tx_queue(dev, tqi);
	skb_dirty = tx_queue->skb_dirty;
	desc_dirty = tx_queue->desc_dirty;
	while ((skb = tx_queue->tx_skbuff[skb_dirty].skb)) {
		if (tx_queue->tx_skbuff[skb_dirty].frag_merge == 0) {
			nr_txbds = skb_shinfo(skb)->nr_frags + 1;
		} else {
			nr_txbds = 1;
		}

		if (ctcmac_txbd_used_untreated(priv) < nr_txbds)
			break;

		for (txbd_index = 0; txbd_index < nr_txbds; txbd_index++) {
			ctcmac_get_txbd(priv);
			tx_buff = &tx_queue->tx_buff[desc_dirty];
			dma_unmap_single(priv->dev, tx_buff->dma, tx_buff->len,
					 DMA_TO_DEVICE);
			if (tx_buff->alloc)
				kfree(tx_buff->vaddr);

			desc_dirty =
			    (desc_dirty >=
			     tx_queue->tx_ring_size - 1) ? 0 : desc_dirty + 1;
		}
		skb = tx_queue->tx_skbuff[skb_dirty].skb;
		dev_kfree_skb_any(skb);
		tx_queue->tx_skbuff[skb_dirty].skb = NULL;
		tx_queue->tx_skbuff[skb_dirty].frag_merge = 0;
		skb_dirty =
		    (skb_dirty >=
		     tx_queue->tx_ring_size - 1) ? 0 : skb_dirty + 1;
		if (netif_msg_tx_queued(priv)) {
			netdev_dbg(priv->ndev,
				   "%s: skb_cur %d skb_dirty %d desc_cur %d desc_dirty %d\n",
				   priv->ndev->name, tx_queue->skb_cur,
				   tx_queue->skb_dirty, tx_queue->desc_cur,
				   tx_queue->desc_dirty);
		}

		spin_lock(&tx_queue->txlock);
		tx_queue->num_txbdfree += nr_txbds;
		spin_unlock(&tx_queue->txlock);
	}

	/* If we freed a buffer, we can restart transmission, if necessary */
	if (tx_queue->num_txbdfree &&
	    netif_tx_queue_stopped(txq) &&
	    !(test_bit(CTCMAC_DOWN, &priv->state))) {
		netif_wake_subqueue(priv->ndev, tqi);
	}

	tx_queue->skb_dirty = skb_dirty;
	tx_queue->desc_dirty = desc_dirty;
}

static int ctcmac_poll_rx_sq(struct napi_struct *napi, int budget)
{
	int work_done = 0;
	int rx_work_limit;
	struct ctcmac_private *priv =
	    container_of(napi, struct ctcmac_private, napi_rx);
	struct ctcmac_priv_rx_q *rxq0 = NULL, *rxq1 = NULL;

	/* clear interrupt */
	writel(CTCMAC_NOR_RX0_D | CTCMAC_NOR_RX1_D,
	       &priv->cpumac_reg->CpuMacInterruptFunc[1]);

	rx_work_limit = budget;
	rxq0 = priv->rx_queue[0];
	rxq1 = priv->rx_queue[1];

	work_done += ctcmac_clean_rx_ring(rxq1, rx_work_limit);
	rx_work_limit -= work_done;
	work_done += ctcmac_clean_rx_ring(rxq0, rx_work_limit);
	rx_work_limit -= work_done;

	rxq0->rx_trigger = 0;
	rxq1->rx_trigger = 0;
	if (work_done < budget) {
		napi_complete(napi);
		if (!ctcmac_rxbd_usable(priv, 0)
		    && !ctcmac_rxbd_recycle(priv, 0))
			rxq0->rx_trigger = 1;

		if (!ctcmac_rxbd_usable(priv, 1)
		    && !ctcmac_rxbd_recycle(priv, 1))
			rxq1->rx_trigger = 1;

		spin_lock_irq(&priv->reglock);
		/* enable interrupt */
		writel(CTCMAC_NOR_RX0_D | CTCMAC_NOR_RX1_D,
		       &priv->cpumac_reg->CpuMacInterruptFunc[3]);
		spin_unlock_irq(&priv->reglock);
	}

	return work_done;
}

static int ctcmac_poll_rx0_sq(struct napi_struct *napi, int budget)
{
	int work_done = 0;
	struct ctcmac_private *priv =
	    container_of(napi, struct ctcmac_private, napi_rx);
	struct ctcmac_priv_rx_q *rx_queue = NULL;

	/* clear interrupt */
	writel(CTCMAC_FUNC0_RX_D, &priv->cpumac_reg->CpuMacInterruptFunc0[1]);
	rx_queue = priv->rx_queue[0];
	work_done = ctcmac_clean_rx_ring(rx_queue, budget);

	if (work_done < budget) {
		napi_complete(napi);
		/* enable interrupt */
		writel(CTCMAC_FUNC0_RX_D,
		       &priv->cpumac_reg->CpuMacInterruptFunc0[3]);
	}

	return work_done;
}

static int ctcmac_poll_rx1_sq(struct napi_struct *napi, int budget)
{
	int work_done = 0;
	struct ctcmac_private *priv =
	    container_of(napi, struct ctcmac_private, napi_rx1);
	struct ctcmac_priv_rx_q *rx_queue = NULL;

	/* clear interrupt */
	writel(CTCMAC_FUNC1_RX_D, &priv->cpumac_reg->CpuMacInterruptFunc1[1]);
	rx_queue = priv->rx_queue[1];
	work_done = ctcmac_clean_rx_ring(rx_queue, budget);

	if (work_done < budget) {
		napi_complete(napi);
		/* enable interrupt */
		writel(CTCMAC_FUNC1_RX_D,
		       &priv->cpumac_reg->CpuMacInterruptFunc1[3]);
	}

	return work_done;
}

static int ctcmac_poll_tx_sq(struct napi_struct *napi, int budget)	//TODO by liuht
{
	struct ctcmac_private *priv =
	    container_of(napi, struct ctcmac_private, napi_tx);
	struct ctcmac_priv_tx_q *tx_queue = priv->tx_queue[0];

	/* clear interrupt */
	writel(CTCMAC_NOR_TX_D, &priv->cpumac_reg->CpuMacInterruptFunc[1]);

	ctcmac_clean_tx_ring(tx_queue);

	napi_complete(napi);
	/* enable interrupt */
	spin_lock_irq(&priv->reglock);
	writel(CTCMAC_NOR_TX_D, &priv->cpumac_reg->CpuMacInterruptFunc[3]);
	spin_unlock_irq(&priv->reglock);

	return 0;
}

static void ctcmac_free_rx_resources(struct ctcmac_private *priv)
{
	int i, j;
	struct ctcmac_priv_rx_q *rx_queue = NULL;

	for (i = 0; i < priv->num_rx_queues; i++) {
		rx_queue = priv->rx_queue[i];
		if (rx_queue->skb)
			dev_kfree_skb(rx_queue->skb);

		for (j = 0; j < rx_queue->rx_ring_size; j++) {
			struct ctcmac_rx_buff *rxb = &rx_queue->rx_buff[j];
			if (!rxb->page)
				continue;
			dma_unmap_single(rx_queue->dev, rxb->dma,
					 PAGE_SIZE, DMA_TO_DEVICE);
			__free_page(rxb->page);
		}
		if (rx_queue->rx_buff) {
			kfree(rx_queue->rx_buff);
			rx_queue->rx_buff = NULL;
		}
	}
}

static int ctcmac_init_rx_resources(struct net_device *ndev)
{
	int i;
	struct ctcmac_private *priv = netdev_priv(ndev);
	struct device *dev = priv->dev;
	struct ctcmac_priv_rx_q *rx_queue = NULL;

	for (i = 0; i < priv->num_rx_queues; i++) {
		rx_queue = priv->rx_queue[i];
		rx_queue->ndev = ndev;
		rx_queue->dev = dev;
		rx_queue->next_to_clean = 0;
		rx_queue->next_to_use = 0;
		rx_queue->next_to_alloc = 0;
		rx_queue->skb = NULL;
		rx_queue->rx_trigger = 0;
		rx_queue->rx_buff = kcalloc(rx_queue->rx_ring_size,
					    sizeof(*rx_queue->rx_buff),
					    GFP_KERNEL);
		if (!rx_queue->rx_buff)
			goto cleanup;

		ctcmac_alloc_rx_buffs(rx_queue, ctcmac_rxbd_unused(rx_queue));
	}

	return 0;

cleanup:
	ctcmac_free_rx_resources(priv);

	return -1;
}

static void ctcmac_free_tx_resources(struct ctcmac_private *priv)
{
	int i;
	struct ctcmac_priv_tx_q *tx_queue = NULL;

	for (i = 0; i < priv->num_tx_queues; i++) {
		struct netdev_queue *txq;

		tx_queue = priv->tx_queue[i];
		txq = netdev_get_tx_queue(tx_queue->dev, tx_queue->qindex);

		if (tx_queue->tx_skbuff) {
			kfree(tx_queue->tx_skbuff);
			tx_queue->tx_skbuff = NULL;
		}
	}
}

static int ctcmac_init_tx_resources(struct net_device *ndev)
{
	int i;
	struct ctcmac_private *priv = netdev_priv(ndev);
	struct ctcmac_priv_tx_q *tx_queue = NULL;

	for (i = 0; i < priv->num_tx_queues; i++) {
		tx_queue = priv->tx_queue[i];
		tx_queue->num_txbdfree = tx_queue->tx_ring_size;
		tx_queue->skb_cur = 0;
		tx_queue->skb_dirty = 0;
		tx_queue->desc_cur = 0;
		tx_queue->desc_dirty = 0;
		tx_queue->dev = ndev;
		tx_queue->tx_skbuff =
		    kmalloc_array(tx_queue->tx_ring_size,
				  sizeof(*tx_queue->tx_skbuff),
				  GFP_KERNEL | __GFP_ZERO);

		if (!tx_queue->tx_skbuff)
			goto cleanup;
	}

	return 0;

cleanup:
	ctcmac_free_tx_resources(priv);

	return -1;
}

static int ctcmac_alloc_skb_resources(struct net_device *ndev)
{
	if (ctcmac_init_rx_resources(ndev))
		return -1;
	if (ctcmac_init_tx_resources(ndev))
		return -1;

	return 0;
}

static int ctcmac_free_skb_resources(struct ctcmac_private *priv)
{
	ctcmac_free_rx_resources(priv);
	ctcmac_free_tx_resources(priv);

	return 0;
}

static void cpumac_start(struct ctcmac_private *priv)
{
	/* 1. enable rx/tx interrupt */
	if (priv->version == 0) {
		writel(CTCMAC_NOR_TX_D | CTCMAC_NOR_RX0_D | CTCMAC_NOR_RX1_D,
		       &priv->cpumac_reg->CpuMacInterruptFunc[3]);
	} else {
		writel(CTCMAC_NOR_TX_D,
		       &priv->cpumac_reg->CpuMacInterruptFunc[3]);
		writel(CTCMAC_FUNC0_RX_D,
		       &priv->cpumac_reg->CpuMacInterruptFunc0[3]);
		writel(CTCMAC_FUNC1_RX_D,
		       &priv->cpumac_reg->CpuMacInterruptFunc1[3]);
	}
	/* 2. enable rx/tx */
	clrsetbits(&priv->cpumac_reg->CpuMacReset,
		   BIT(CPU_MAC_RESET_W0_SOFT_RST_TX_BIT), 0);
	clrsetbits(&priv->cpumac_reg->CpuMacReset,
		   BIT(CPU_MAC_RESET_W0_SOFT_RST_RX_BIT), 0);

	netif_trans_update(priv->ndev);	/* prevent tx timeout */
}

static void cpumac_halt(struct ctcmac_private *priv)
{
	/* 1. disable rx/tx interrupt */
	ctcmac_regw(&priv->cpumac_reg->CpuMacInterruptFunc[2], 0xffffffff);
	ctcmac_regw(&priv->cpumac_reg->CpuMacInterruptNormal[2], 0xffffffff);
	if (priv->version > 0) {
		ctcmac_regw(&priv->cpumac_reg->CpuMacInterruptFunc0[2],
			    0xffffffff);
		ctcmac_regw(&priv->cpumac_reg->CpuMacInterruptFunc1[2],
			    0xffffffff);
	}
	/* 2. disable rx/tx */
	clrsetbits(&priv->cpumac_reg->CpuMacReset, 0,
		   BIT(CPU_MAC_RESET_W0_SOFT_RST_TX_BIT));
	clrsetbits(&priv->cpumac_reg->CpuMacReset, 0,
		   BIT(CPU_MAC_RESET_W0_SOFT_RST_RX_BIT));
}

static void ctcmac_token_timer(struct timer_list *t)
{
	struct ctcmac_private *priv = from_timer(priv, t, token_timer);
	struct ctcmac_priv_rx_q *rxq0, *rxq1;

	mod_timer(&priv->token_timer, jiffies + HZ / 10 - 1);
	rxq0 = priv->rx_queue[0];
	rxq1 = priv->rx_queue[1];
	rxq0->token = min(rxq0->token + rxq0->pps_limit, rxq0->token_max);
	rxq1->token = min(rxq1->token + rxq1->pps_limit, rxq1->token_max);

	if ((rxq0->rx_trigger == 1) && (rxq0->token / CTCMAC_TOKEN_PER_PKT)) {
		rxq0->rx_trigger = 0;
		rxq0->token -= CTCMAC_TOKEN_PER_PKT;
		ctcmac_alloc_one_rx_buffs(rxq0);
	}

	if ((rxq1->rx_trigger == 1) && (rxq1->token / CTCMAC_TOKEN_PER_PKT)) {
		rxq1->rx_trigger = 0;
		rxq1->token -= CTCMAC_TOKEN_PER_PKT;
		ctcmac_alloc_one_rx_buffs(rxq1);
	}
}

static ssize_t ctcmac_get_rxq0_pps(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct ctcmac_private *priv =
	    (struct ctcmac_private *)dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", priv->rx_queue[0]->pps_limit);;
}

static ssize_t ctcmac_set_rxq0_pps(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 rq0_pps;
	struct ctcmac_private *priv =
	    (struct ctcmac_private *)dev_get_drvdata(dev);

	if (kstrtou32(buf, 0, &rq0_pps)) {
		dev_err(&priv->ndev->dev, "Invalid rx queue0 pps!\n");
		return -1;
	}
	priv->rx_queue[0]->pps_limit = rq0_pps;

	return count;
}

static ssize_t ctcmac_get_rxq1_pps(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct ctcmac_private *priv =
	    (struct ctcmac_private *)dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", priv->rx_queue[1]->pps_limit);;
}

static ssize_t ctcmac_set_rxq1_pps(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	u32 rq1_pps;
	struct ctcmac_private *priv =
	    (struct ctcmac_private *)dev_get_drvdata(dev);

	if (kstrtou32(buf, 0, &rq1_pps)) {
		dev_err(&priv->ndev->dev, "Invalid rx queue0 pps!\n");
		return -1;
	}
	priv->rx_queue[1]->pps_limit = rq1_pps;

	return count;
}

static DEVICE_ATTR(rxq0_pps, S_IRUGO | S_IWUSR, ctcmac_get_rxq0_pps,
		   ctcmac_set_rxq0_pps);
static DEVICE_ATTR(rxq1_pps, S_IRUGO | S_IWUSR, ctcmac_get_rxq1_pps,
		   ctcmac_set_rxq1_pps);

static void ctcmac_pps_init(struct ctcmac_private *priv)
{
	struct ctcmac_priv_rx_q *rxq0, *rxq1;

	rxq0 = priv->rx_queue[0];
	rxq1 = priv->rx_queue[1];

	if (device_create_file(&priv->ndev->dev, &dev_attr_rxq0_pps) < 0) {
		dev_err(&priv->ndev->dev, "create pps limit node fail\n");
		return;
	}
	if (device_create_file(&priv->ndev->dev, &dev_attr_rxq1_pps) < 0) {
		dev_err(&priv->ndev->dev, "create pps limit node fail\n");
		return;
	}

	rxq0->pps_limit = 0;
	rxq1->pps_limit = 0;
}

static void ctcmac_pps_disable(struct ctcmac_private *priv)
{
	priv->rx_queue[0]->pps_limit = 0;
	priv->rx_queue[1]->pps_limit = 0;
	del_timer(&priv->token_timer);
}

static void ctcmac_pps_cfg(struct ctcmac_private *priv)
{
	struct ctcmac_priv_rx_q *rxq0, *rxq1;

	rxq0 = priv->rx_queue[0];
	rxq1 = priv->rx_queue[1];

	rxq0->token = 0;
	rxq0->token_max = rxq0->pps_limit * CTCMAC_TOKEN_PER_PKT;
	rxq0->rx_trigger = 0;
	rxq1->token = 0;
	rxq1->token_max = rxq1->pps_limit * CTCMAC_TOKEN_PER_PKT;
	rxq1->rx_trigger = 0;
	if (rxq0->pps_limit || rxq1->pps_limit) {
		timer_setup(&priv->token_timer, ctcmac_token_timer, 0);
		priv->token_timer.expires = jiffies + HZ / 10;
		add_timer(&priv->token_timer);
		/* when enable pps ratelimit, must use desc done interrupt */
		priv->int_type = CTCMAC_INT_DESC;
	}
}

static int ctcmac_enet_open(struct net_device *dev)
{
	struct ctcmac_private *priv = netdev_priv(dev);
	int err;

	ctcmac_pps_cfg(priv);

	err = ctcmac_init_phy(dev);
	if (err) {
		return err;
	}

	err = ctcmac_request_irq(priv);
	if (err) {
		return err;
	}

	err = startup_ctcmac(dev);
	if (err) {
		return err;
	}

	return 0;
}

static void head_to_txbuff_direct(struct device *dev, struct sk_buff *skb,
				  struct ctcmac_tx_buff *tx_buff)
{
	tx_buff->alloc = 0;
	tx_buff->vaddr = skb->data;
	tx_buff->len = skb_headlen(skb);
	tx_buff->dma = dma_map_single(dev, skb->data, skb_headlen(skb),
				      DMA_TO_DEVICE);
	tx_buff->offset = 0;
}

static void frag_to_txbuff_direct(struct device *dev, skb_frag_t * frag,
				  struct ctcmac_tx_buff *tx_buff)
{
	tx_buff->alloc = 0;
	tx_buff->vaddr = skb_frag_address(frag);
	tx_buff->len = skb_frag_size(frag);
	tx_buff->dma =
	    dma_map_single(dev, tx_buff->vaddr, tx_buff->len, DMA_TO_DEVICE);
	tx_buff->offset = 0;
}

static void head_to_txbuff_alloc(struct device *dev, struct sk_buff *skb,
				 struct ctcmac_tx_buff *tx_buff)
{
	u64 offset;
	int alloc_size;

	alloc_size = ALIGN(skb->len, BUF_ALIGNMENT);
	tx_buff->alloc = 1;
	tx_buff->len = skb->len;
	tx_buff->vaddr = kmalloc(alloc_size, GFP_KERNEL);
	offset =
	    (BUF_ALIGNMENT - (((u64) tx_buff->vaddr) & (BUF_ALIGNMENT - 1)));
	if (offset == BUF_ALIGNMENT) {
		offset = 0;
	}
	tx_buff->offset = offset;
	memcpy(tx_buff->vaddr + offset, skb->data, skb_headlen(skb));
	tx_buff->dma = dma_map_single(dev, tx_buff->vaddr, tx_buff->len,
				      DMA_TO_DEVICE);
}

static void frag_to_txbuff_alloc(struct device *dev, skb_frag_t * frag,
				 struct ctcmac_tx_buff *tx_buff)
{
	u64 offset;
	int alloc_size;

	alloc_size = ALIGN(skb_frag_size(frag), BUF_ALIGNMENT);
	tx_buff->alloc = 1;
	tx_buff->len = skb_frag_size(frag);
	tx_buff->vaddr = kmalloc(alloc_size, GFP_KERNEL);
	offset =
	    (BUF_ALIGNMENT - (((u64) tx_buff->vaddr) & (BUF_ALIGNMENT - 1)));
	if (offset == BUF_ALIGNMENT) {
		offset = 0;
	}
	tx_buff->offset = offset;
	memcpy(tx_buff->vaddr + offset, skb_frag_address(frag),
	       skb_frag_size(frag));
	tx_buff->dma =
	    dma_map_single(dev, tx_buff->vaddr, tx_buff->len, DMA_TO_DEVICE);
}

static void skb_to_txbuff_alloc(struct device *dev, struct sk_buff *skb,
				struct ctcmac_tx_buff *tx_buff)
{
	u64 offset;
	int alloc_size, frag_index;
	skb_frag_t *frag;

	alloc_size = ALIGN(skb->len, BUF_ALIGNMENT);
	tx_buff->alloc = 1;
	tx_buff->len = skb->len;
	tx_buff->vaddr = kmalloc(alloc_size, GFP_KERNEL);
	offset =
	    (BUF_ALIGNMENT - (((u64) tx_buff->vaddr) & (BUF_ALIGNMENT - 1)));
	if (offset == BUF_ALIGNMENT) {
		offset = 0;
	}
	tx_buff->offset = offset;
	memcpy(tx_buff->vaddr + offset, skb->data, skb_headlen(skb));
	offset += skb_headlen(skb);
	for (frag_index = 0; frag_index < skb_shinfo(skb)->nr_frags;
	     frag_index++) {
		frag = &skb_shinfo(skb)->frags[frag_index];
		memcpy(tx_buff->vaddr + offset, skb_frag_address(frag),
		       skb_frag_size(frag));
		offset += skb_frag_size(frag);
	}
	tx_buff->dma =
	    dma_map_single(dev, tx_buff->vaddr, tx_buff->len, DMA_TO_DEVICE);
}

static int skb_to_txbuff(struct ctcmac_private *priv, struct sk_buff *skb)
{
	u64 addr;
	int frag_index, rq, to_use, nr_frags;
	skb_frag_t *frag;
	struct ctcmac_tx_buff *tx_buff;
	struct ctcmac_priv_tx_q *tx_queue = NULL;
	bool addr_align256, not_span_4K, len_align8 = 1;

	rq = skb->queue_mapping;
	tx_queue = priv->tx_queue[rq];
	to_use = tx_queue->desc_cur;
	tx_buff = &tx_queue->tx_buff[to_use];
	nr_frags = skb_shinfo(skb)->nr_frags;

	if (priv->version) {
		head_to_txbuff_direct(priv->dev, skb, tx_buff);
		to_use =
		    (to_use >= tx_queue->tx_ring_size - 1) ? 0 : to_use + 1;
		//printk(KERN_ERR "skb_to_txbuff0 %llx %d %d\n", (u64)skb->data, skb_headlen(skb), to_use);
		for (frag_index = 0; frag_index < nr_frags; frag_index++) {
			tx_buff = &tx_queue->tx_buff[to_use];
			frag = &skb_shinfo(skb)->frags[frag_index];
			frag_to_txbuff_direct(priv->dev, frag, tx_buff);
			to_use =
			    to_use >=
			    tx_queue->tx_ring_size - 1 ? 0 : to_use + 1;
			//printk(KERN_ERR "skb_to_txbuff1 %llx %d %d %d\n", 
			//      (u64)skb_frag_address(frag), skb_frag_size(frag), to_use, frag_index);
		}
	} else {
		if (skb_shinfo(skb)->nr_frags) {
			if (skb_headlen(skb) & 0x7) {
				len_align8 = 0;
			}
			for (frag_index = 0; (len_align8 == 1) &&
			     (frag_index < nr_frags - 1); frag_index++) {
				frag = &skb_shinfo(skb)->frags[frag_index];
				if (skb_frag_size(frag) & 0x7)
					len_align8 = 0;
			}
		}

		if (len_align8 == 0) {
			//printk(KERN_ERR "skb_to_txbuff2 %llx %d %d\n", (u64)skb->data, skb_headlen(skb), to_use);
			for (frag_index = 0; frag_index < nr_frags;
			     frag_index++) {
				frag = &skb_shinfo(skb)->frags[frag_index];
				//printk(KERN_ERR "skb_to_txbuff3 %llx %d %d %d\n", (u64)skb_frag_address(frag), skb_frag_size(frag), to_use, frag_index);
			}
			skb_to_txbuff_alloc(priv->dev, skb, tx_buff);
			to_use =
			    (to_use >=
			     tx_queue->tx_ring_size - 1) ? 0 : to_use + 1;
		} else {
			addr = (u64) skb->data;
			addr_align256 =
			    ((addr & (BUF_ALIGNMENT - 1)) == 0) ? 1 : 0;
			not_span_4K =
			    ((addr & PAGE_MASK) ==
			     ((addr + skb_headlen(skb)) & PAGE_MASK)) ? 1 : 0;
			if (addr_align256 || not_span_4K) {
				head_to_txbuff_direct(priv->dev, skb, tx_buff);
				//printk(KERN_ERR "skb_to_txbuff4 %llx %d %d\n", (u64)skb->data, skb_headlen(skb), to_use);
			} else {
				head_to_txbuff_alloc(priv->dev, skb, tx_buff);
				//printk(KERN_ERR "skb_to_txbuff5 %llx %d %d\n", (u64)skb->data, skb_headlen(skb), to_use);
			}
			to_use =
			    (to_use >=
			     tx_queue->tx_ring_size - 1) ? 0 : to_use + 1;
			for (frag_index = 0; frag_index < nr_frags;
			     frag_index++) {
				tx_buff = &tx_queue->tx_buff[to_use];
				frag = &skb_shinfo(skb)->frags[frag_index];
				addr = (u64) skb_frag_address(frag);

				addr_align256 =
				    ((addr & (BUF_ALIGNMENT - 1)) == 0) ? 1 : 0;
				not_span_4K =
				    ((addr & PAGE_MASK) ==
				     ((addr +
				       skb_frag_size(frag)) & PAGE_MASK)) ? 1 :
				    0;
				if (addr_align256 || not_span_4K) {
					frag_to_txbuff_direct(priv->dev, frag,
							      tx_buff);
					//printk(KERN_ERR "skb_to_txbuff6 %llx %d %d %d\n", (u64)skb_frag_address(frag), skb_frag_size(frag), to_use, frag_index);
				} else {
					frag_to_txbuff_alloc(priv->dev, frag,
							     tx_buff);
					//printk(KERN_ERR "skb_to_txbuff7 %llx %d %d %d\n", (u64)skb_frag_address(frag), skb_frag_size(frag), to_use, frag_index);
				}
				to_use =
				    to_use >=
				    tx_queue->tx_ring_size - 1 ? 0 : to_use + 1;
			}
		}
	}

	return len_align8 ? 0 : 1;
}

static int ctcmac_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
	int rq = 0;
	int to_use = 0, frag_merged;
	unsigned int frag_index, nr_txbds;
	unsigned int bytes_sent;
	struct netdev_queue *txq;
	struct ctcmac_desc_cfg tx_desc;
	struct ctcmac_tx_buff *tx_buff;
	struct ctcmac_priv_tx_q *tx_queue = NULL;
	struct ctcmac_private *priv = netdev_priv(dev);

	rq = skb->queue_mapping;
	tx_queue = priv->tx_queue[rq];
	txq = netdev_get_tx_queue(dev, rq);

	nr_txbds = skb_shinfo(skb)->nr_frags + 1;
	/* check if there is space to queue this packet */
	if (tx_queue->num_txbdfree < nr_txbds) {
		if (netif_msg_tx_err(priv)) {
			netdev_dbg(priv->ndev,
				   "%s: no space left before send pkt!\n",
				   priv->ndev->name);
		}
		/* no space, stop the queue */
		netif_tx_stop_queue(txq);
		dev->stats.tx_fifo_errors++;
		return NETDEV_TX_BUSY;
	}

	/* Update transmit stats */
	bytes_sent = skb->len;
	tx_queue->stats.tx_bytes += bytes_sent;
	tx_queue->stats.tx_packets++;

	frag_merged = skb_to_txbuff(priv, skb);
	tx_queue->tx_skbuff[tx_queue->skb_cur].skb = skb;
	tx_queue->tx_skbuff[tx_queue->skb_cur].frag_merge = frag_merged;
	tx_queue->skb_cur =
	    (tx_queue->skb_cur >=
	     tx_queue->tx_ring_size - 1) ? 0 : tx_queue->skb_cur + 1;
	if (frag_merged) {
		nr_txbds = 1;
	}
	to_use = tx_queue->desc_cur;
	if (nr_txbds <= 1) {
		tx_buff = &tx_queue->tx_buff[to_use];
		tx_desc.sop = 1;
		tx_desc.eop = 1;
		tx_desc.size = tx_buff->len;
		tx_desc.addr_low =
		    (tx_buff->dma + tx_buff->offset - CTC_DDR_BASE)
		    & CPU_MAC_DESC_INTF_W0_DESC_ADDR_31_0_MASK;
		tx_desc.addr_high =
		    ((tx_buff->dma + tx_buff->offset - CTC_DDR_BASE) >> 32)
		    & CPU_MAC_DESC_INTF_W1_DESC_ADDR_39_32_MASK;
		ctcmac_fill_txbd(priv, &tx_desc);
		to_use =
		    (to_use >= tx_queue->tx_ring_size - 1) ? 0 : to_use + 1;
	} else {
		for (frag_index = 0; frag_index < nr_txbds; frag_index++) {
			tx_buff = &tx_queue->tx_buff[to_use];
			tx_desc.sop = (frag_index == 0) ? 1 : 0;
			tx_desc.eop = (frag_index == nr_txbds - 1) ? 1 : 0;
			tx_desc.size = tx_buff->len;
			tx_desc.addr_low =
			    (tx_buff->dma + tx_buff->offset - CTC_DDR_BASE)
			    & CPU_MAC_DESC_INTF_W0_DESC_ADDR_31_0_MASK;
			tx_desc.addr_high =
			    ((tx_buff->dma + tx_buff->offset -
			      CTC_DDR_BASE) >> 32)
			    & CPU_MAC_DESC_INTF_W1_DESC_ADDR_39_32_MASK;
			ctcmac_fill_txbd(priv, &tx_desc);
			to_use =
			    (to_use >=
			     tx_queue->tx_ring_size - 1) ? 0 : to_use + 1;
		}
	}
	tx_queue->desc_cur = to_use;

	if (netif_msg_tx_queued(priv)) {
		netdev_dbg(priv->ndev,
			   "%s: skb_cur %d skb_dirty %d desc_cur %d desc_dirty %d\n",
			   priv->ndev->name, tx_queue->skb_cur,
			   tx_queue->skb_dirty, tx_queue->desc_cur,
			   tx_queue->desc_dirty);
	}

	/* We can work in parallel with 872(), except
	 * when modifying num_txbdfree. Note that we didn't grab the lock
	 * when we were reading the num_txbdfree and checking for available
	 * space, that's because outside of this function it can only grow.
	 */
	spin_lock_bh(&tx_queue->txlock);
	/* reduce TxBD free count */
	tx_queue->num_txbdfree -= nr_txbds;
	spin_unlock_bh(&tx_queue->txlock);

	/* If the next BD still needs to be cleaned up, then the bds
	 * are full.  We need to tell the kernel to stop sending us stuff.
	 */
	if (!tx_queue->num_txbdfree) {
		netif_tx_stop_queue(txq);
		if (netif_msg_tx_err(priv)) {
			printk(KERN_ERR
			       "%s: no space left before send pkt22!\n",
			       priv->ndev->name);
		}
		dev->stats.tx_fifo_errors++;
	}

	return NETDEV_TX_OK;
}

static int ctcmac_change_mtu(struct net_device *dev, int new_mtu)
{
	struct ctcmac_private *priv = netdev_priv(dev);
	int frame_size = new_mtu + ETH_HLEN;

	if ((frame_size < 64) || (frame_size > CTCMAC_JUMBO_FRAME_SIZE)) {
		return -EINVAL;
	}

	while (test_and_set_bit_lock(CTCMAC_RESETTING, &priv->state))
		cpu_relax();

	if (dev->flags & IFF_UP)
		stop_ctcmac(dev);

	dev->mtu = new_mtu;

	if (dev->flags & IFF_UP)
		startup_ctcmac(dev);

	clear_bit_unlock(CTCMAC_RESETTING, &priv->state);

	return 0;
}

static int ctcmac_set_features(struct net_device *dev,
			       netdev_features_t features)
{
	return 0;
}

/* Stops the kernel queue, and halts the controller */
static int ctcmac_close(struct net_device *dev)
{
	struct ctcmac_private *priv = netdev_priv(dev);

	ctcmac_pps_disable(priv);
	cancel_work_sync(&priv->reset_task);
	stop_ctcmac(dev);

	/* Disconnect from the PHY */
	phy_disconnect(dev->phydev);

	ctcmac_free_irq(priv);

	return 0;
}

static void ctcmac_set_multi(struct net_device *dev)
{
	int idx = 0;
	u32 val, addr_h = 0, addr_l = 0;
	struct netdev_hw_addr *ha;
	struct ctcmac_private *priv = netdev_priv(dev);

	if (priv->version == 0)
		return;

	/* receive all packets */
	if (dev->flags & IFF_PROMISC) {
		/* not check normal packet */
		if (priv->index == 0) {
			val =
			    readl(&priv->cpumacu_reg->CpuMacUnitFilterCfg1[1]);
			val &=
			    ~BIT
			    (CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_CHECK_NORMAL_PKT_ENABLE0_BIT);
			writel(val,
			       &priv->cpumacu_reg->CpuMacUnitFilterCfg1[1]);
		} else {
			val =
			    readl(&priv->cpumacu_reg->CpuMacUnitFilterCfg1[1]);
			val &=
			    ~BIT
			    (CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_CHECK_NORMAL_PKT_ENABLE1_BIT);
			writel(val,
			       &priv->cpumacu_reg->CpuMacUnitFilterCfg1[1]);
		}
		return;
	}

	if (priv->index == 0) {
		val = readl(&priv->cpumacu_reg->CpuMacUnitFilterCfg1[1]);
		val |=
		    BIT
		    (CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_CHECK_NORMAL_PKT_ENABLE0_BIT);
		val |=
		    BIT(CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_DROP_NORMAL_PKT0_BIT);
		writel(val, &priv->cpumacu_reg->CpuMacUnitFilterCfg1[1]);
	} else {
		val = readl(&priv->cpumacu_reg->CpuMacUnitFilterCfg1[1]);
		val |=
		    BIT
		    (CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_CHECK_NORMAL_PKT_ENABLE1_BIT);
		val |=
		    BIT(CPU_MAC_UNIT_FILTER_CFG1_W1_CFG_DROP_NORMAL_PKT1_BIT);
		writel(val, &priv->cpumacu_reg->CpuMacUnitFilterCfg1[1]);
	}

	/* receive packets with local mac address
	   receive packets with broadcast mac address
	   receive packets with multicast mac address 
	 */
	if ((dev->flags & IFF_ALLMULTI) || (netdev_mc_count(dev) > 2)) {
		/* mac filter table 2/6:white list to receive all pacekt with multicast mac address  */
		if (priv->index == 0) {
			writel(0x00000000,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[4]);
			writel(0x00000100,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[5]);
			writel(0x00000000,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[20]);
			writel(0x00000100,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[21]);
		} else {
			writel(0x00000000,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[12]);
			writel(0x00000100,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[13]);
			writel(0x00000000,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[28]);
			writel(0x00000100,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[29]);
		}
		return;
	}

	if (netdev_mc_empty(dev)) {
		return;
	}
	/* receive packet with local mac address */
	/* receive packet with broadcast mac address */
	/* receive packet with multicast mac address in mc list */
	netdev_for_each_mc_addr(ha, dev) {
		addr_h = ha->addr[0] << 8 | ha->addr[1];
		addr_l =
		    ha->addr[2] << 24 | ha->addr[3] << 16 | ha->
		    addr[4] << 8 | ha->addr[5];
		if (priv->index == 0) {
			writel(addr_l,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[4 +
								       idx *
								       2]);
			writel(addr_h,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[5 +
								       idx *
								       2]);
			writel(0xffffffff,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[20 +
								       idx *
								       2]);
			writel(0x0000ffff,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[21 +
								       idx *
								       2]);
		} else {
			writel(addr_l,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[12 +
								       idx *
								       2]);
			writel(addr_h,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[13 +
								       idx *
								       2]);
			writel(0xffffffff,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[28 +
								       idx *
								       2]);
			writel(0x0000ffff,
			       &priv->cpumacu_reg->CpuMacUnitMacCamCfg[29 +
								       idx *
								       2]);
		}
		idx++;
		if (idx >= 2)
			return;
	}
}

static void ctcmac_timeout(struct net_device *dev, unsigned int txqueue)
{
	struct ctcmac_private *priv = netdev_priv(dev);

	dev->stats.tx_errors++;
	schedule_work(&priv->reset_task);
}

static int ctcmac_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
	struct phy_device *phydev = dev->phydev;

	if (!netif_running(dev))
		return -EINVAL;

	if (!phydev)
		return -ENODEV;

	return phy_mii_ioctl(phydev, rq, cmd);
}

static struct net_device_stats *ctcmac_get_stats(struct net_device *dev)
{
	int qidx;
	unsigned long rx_packets = 0, rx_bytes = 0, rx_dropped = 0;
	unsigned long tx_packets = 0, tx_bytes = 0;
	struct ctcmac_private *priv = netdev_priv(dev);

	for (qidx = 0; qidx < priv->num_rx_queues; qidx++) {
		if (!priv->rx_queue[qidx]) {
			return &dev->stats;
		}
		rx_packets += priv->rx_queue[qidx]->stats.rx_packets;
		rx_bytes += priv->rx_queue[qidx]->stats.rx_bytes;
		rx_dropped += priv->rx_queue[qidx]->stats.rx_dropped;
	}

	if (!priv->tx_queue[0]) {
		return &dev->stats;
	}

	tx_packets = priv->tx_queue[0]->stats.tx_packets;
	tx_bytes = priv->tx_queue[0]->stats.tx_bytes;

	dev->stats.rx_packets = rx_packets;
	dev->stats.rx_bytes = rx_bytes;
	dev->stats.rx_dropped = rx_dropped;
	dev->stats.tx_bytes = tx_bytes;
	dev->stats.tx_packets = tx_packets;

	return &dev->stats;
}

static void ctcmac_set_mac_for_addr(struct ctcmac_private *priv,
				    const u8 * addr)
{
	u32 addr_h = 0, addr_l = 0;

	addr_h = addr[0] << 8 | addr[1];
	addr_l = addr[2] << 24 | addr[3] << 16 | addr[4] << 8 | addr[5];
	if (priv->index == 0) {
		writel(addr_l, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[0]);
		writel(addr_h, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[1]);
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[16]);
		writel(0x0000ffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[17]);
	} else {
		writel(addr_l, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[8]);
		writel(addr_h, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[9]);
		writel(0xffffffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[24]);
		writel(0x0000ffff, &priv->cpumacu_reg->CpuMacUnitMacCamCfg[25]);
	}
}

static void ctcmac_set_src_mac_for_flow_ctrl(struct ctcmac_private *priv,
					     const u8 * addr)
{
	u32 val;
	u32 addr_h = 0, addr_l = 0;

	addr_h = addr[0] << 8 | addr[1];
	addr_l = addr[2] << 24 | addr[3] << 16 | addr[4] << 8 | addr[5];

	writel(addr_l, &priv->cpumac_reg->CpuMacPauseCfg[2]);
	val = readl(&priv->cpumac_reg->CpuMacPauseCfg[3]);
	val &= 0xffff0000;
	addr_h &= 0xffff;
	val |= addr_h;
	writel(val, &priv->cpumac_reg->CpuMacPauseCfg[3]);
}

static int ctcmac_set_mac_addr(struct net_device *dev, void *p)
{
	struct ctcmac_private *priv = netdev_priv(dev);

	eth_mac_addr(dev, p);
	ctcmac_set_src_mac_for_flow_ctrl(priv, dev->dev_addr);
	if (priv->version > 0)
		ctcmac_set_mac_for_addr(priv, dev->dev_addr);

	return 0;
}

static void ctcmac_gdrvinfo(struct net_device *dev,
			    struct ethtool_drvinfo *drvinfo)
{
	strlcpy(drvinfo->driver, "ctcmac", sizeof(drvinfo->driver));
	strlcpy(drvinfo->version, "v1.0", sizeof(drvinfo->version));
	strlcpy(drvinfo->fw_version, "N/A", sizeof(drvinfo->fw_version));
	strlcpy(drvinfo->bus_info, "N/A", sizeof(drvinfo->bus_info));
}

/* Return the length of the register structure */
static int ctcmac_reglen(struct net_device *dev)
{
	return sizeof(struct CpuMac_regs);
}

/* Return a dump of the GFAR register space */
static void ctcmac_get_regs(struct net_device *dev, struct ethtool_regs *regs,
			    void *regbuf)
{
	int i;
	struct ctcmac_private *priv = netdev_priv(dev);
	u32 __iomem *theregs = (u32 __iomem *) priv->cpumac_reg;
	u32 *buf = (u32 *) regbuf;

	for (i = 0; i < sizeof(struct CpuMac_regs) / sizeof(u32); i++)
		buf[i] = ctcmac_regr(&theregs[i]);
}

/* Fills in rvals with the current ring parameters.  Currently,
 * rx, rx_mini, and rx_jumbo rings are the same size, as mini and
 * jumbo are ignored by the driver */
static void ctcmac_gringparam(struct net_device *dev,
			      struct ethtool_ringparam *rvals)
{
	struct ctcmac_private *priv = netdev_priv(dev);
	struct ctcmac_priv_tx_q *tx_queue = NULL;
	struct ctcmac_priv_rx_q *rx_queue = NULL;

	tx_queue = priv->tx_queue[0];
	rx_queue = priv->rx_queue[0];

	rvals->rx_max_pending = CTCMAC_MAX_RING_SIZE;
	rvals->rx_mini_max_pending = CTCMAC_MAX_RING_SIZE;
	rvals->rx_jumbo_max_pending = CTCMAC_MAX_RING_SIZE;
	rvals->tx_max_pending = CTCMAC_MAX_RING_SIZE;

	/* Values changeable by the user.  The valid values are
	 * in the range 1 to the "*_max_pending" counterpart above.
	 */
	rvals->rx_pending = rx_queue->rx_ring_size;
	rvals->rx_mini_pending = rx_queue->rx_ring_size;
	rvals->rx_jumbo_pending = rx_queue->rx_ring_size;
	rvals->tx_pending = tx_queue->tx_ring_size;
}

/* Change the current ring parameters, stopping the controller if
 * necessary so that we don't mess things up while we're in motion.
 */
static int ctcmac_sringparam(struct net_device *dev,
			     struct ethtool_ringparam *rvals)
{
	struct ctcmac_private *priv = netdev_priv(dev);
	int err = 0, i;

	if (rvals->rx_pending > CTCMAC_MAX_RING_SIZE)
		return -EINVAL;

	if (rvals->tx_pending > CTCMAC_MAX_RING_SIZE)
		return -EINVAL;

	while (test_and_set_bit_lock(CTCMAC_RESETTING, &priv->state))
		cpu_relax();

	if (dev->flags & IFF_UP)
		stop_ctcmac(dev);

	/* Change the sizes */
	for (i = 0; i < priv->num_rx_queues; i++)
		priv->rx_queue[i]->rx_ring_size = rvals->rx_pending;

	for (i = 0; i < priv->num_tx_queues; i++)
		priv->tx_queue[i]->tx_ring_size = rvals->tx_pending;

	/* Rebuild the rings with the new size */
	if (dev->flags & IFF_UP)
		err = startup_ctcmac(dev);

	clear_bit_unlock(CTCMAC_RESETTING, &priv->state);

	return err;
}

static void ctcmac_gpauseparam(struct net_device *dev,
			       struct ethtool_pauseparam *epause)
{
	struct ctcmac_private *priv = netdev_priv(dev);

	epause->autoneg = ! !priv->pause_aneg_en;
	epause->rx_pause = ! !priv->rx_pause_en;
	epause->tx_pause = ! !priv->tx_pause_en;
}

/*
*                            IEEE 802.3-2000
*                              Table 28B-2
*                             Pause Encoding
*
*                PAUSE   ASM_DIR         TX      RX
*                -----   -----           --      --
*                  0       0             0       0
*                  0       1             1       0
*                  1       0             1       1
*                  1       1             0       1
*
*/
static int ctcmac_spauseparam(struct net_device *dev,
			      struct ethtool_pauseparam *epause)
{
	struct ctcmac_private *priv = netdev_priv(dev);
	struct phy_device *phydev = dev->phydev;

	if (!phydev)
		return -ENODEV;

	if (!phy_validate_pause(phydev, epause))
		return -EINVAL;

	priv->rx_pause_en = priv->tx_pause_en = 0;
	if (epause->rx_pause) {
		priv->rx_pause_en = 1;

		if (epause->tx_pause) {
			priv->tx_pause_en = 1;
		}
	} else if (epause->tx_pause) {
		priv->tx_pause_en = 1;

	}

	if (epause->autoneg)
		priv->pause_aneg_en = 1;
	else
		priv->pause_aneg_en = 0;

	if (!epause->autoneg) {
		ctcmac_cfg_flow_ctrl(priv, priv->tx_pause_en,
				     priv->rx_pause_en);
	}

	return 0;
}

static void ctcmac_gstrings(struct net_device *dev, u32 stringset, u8 * buf)
{
	memcpy(buf, ctc_stat_gstrings, CTCMAC_STATS_LEN * ETH_GSTRING_LEN);
}

static int ctcmac_sset_count(struct net_device *dev, int sset)
{
	return CTCMAC_STATS_LEN;
}

static void ctcmac_fill_stats(struct net_device *netdev,
			      struct ethtool_stats *dummy, u64 * buf)
{
	u32 mtu;
	unsigned long flags;
	struct ctcmac_pkt_stats *stats;
	struct ctcmac_private *priv = netdev_priv(netdev);

	spin_lock_irqsave(&priv->reglock, flags);
	stats = &g_pkt_stats[priv->index];
	stats->rx_good_ucast_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam0[0]);
	stats->rx_good_ucast_pkt +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam0[2]);
	stats->rx_good_mcast_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam1[0]);
	stats->rx_good_mcast_pkt +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam1[2]);
	stats->rx_good_bcast_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam2[0]);
	stats->rx_good_bcast_pkt +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam2[2]);
	stats->rx_good_pause_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam3[0]);
	stats->rx_good_pause_pkt +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam3[2]);
	stats->rx_good_pfc_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam4[0]);
	stats->rx_good_pfc_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam4[2]);
	stats->rx_good_control_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam5[0]);
	stats->rx_good_control_pkt +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam5[2]);
	stats->rx_fcs_error_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam6[0]);
	stats->rx_fcs_error_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam6[2]);
	stats->rx_mac_overrun_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam7[0]);
	stats->rx_mac_overrun_pkt +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam7[2]);
	stats->rx_good_63B_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam8[0]);
	stats->rx_good_63B_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam8[2]);
	stats->rx_bad_63B_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam9[0]);
	stats->rx_bad_63B_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam9[2]);
	stats->rx_good_mtu2B_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam10[0]);
	stats->rx_good_mtu2B_pkt +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam10[2]);
	stats->rx_bad_mtu2B_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam11[0]);
	stats->rx_bad_mtu2B_pkt +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam11[2]);
	stats->rx_good_jumbo_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam12[0]);
	stats->rx_good_jumbo_pkt +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam12[2]);
	stats->rx_bad_jumbo_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam13[0]);
	stats->rx_bad_jumbo_pkt +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam13[2]);
	stats->rx_64B_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam14[0]);
	stats->rx_64B_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam14[2]);
	stats->rx_127B_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam15[0]);
	stats->rx_127B_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam15[2]);
	stats->rx_255B_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam16[0]);
	stats->rx_255B_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam16[2]);
	stats->rx_511B_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam17[0]);
	stats->rx_511B_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam17[2]);
	stats->rx_1023B_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam18[0]);
	stats->rx_1023B_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam18[2]);
	stats->rx_mtu1B_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam19[0]);
	stats->rx_mtu1B_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam19[2]);
	stats->tx_ucast_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam20[0]);
	stats->tx_ucast_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam20[2]);
	stats->tx_mcast_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam21[0]);
	stats->tx_mcast_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam21[2]);
	stats->tx_bcast_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam22[0]);
	stats->tx_bcast_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam22[2]);
	stats->tx_pause_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam23[0]);
	stats->tx_pause_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam23[2]);
	stats->tx_control_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam24[0]);
	stats->tx_control_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam24[2]);
	stats->tx_fcs_error_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam25[0]);
	stats->tx_fcs_error_pkt +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam25[2]);
	stats->tx_underrun_bytes +=
	    readq(&priv->cpumac_mem->CpuMacStatsRam26[0]);
	stats->tx_underrun_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam26[2]);
	stats->tx_63B_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam27[0]);
	stats->tx_63B_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam27[2]);
	stats->tx_64B_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam28[0]);
	stats->tx_64B_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam28[2]);
	stats->tx_127B_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam29[0]);
	stats->tx_127B_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam29[2]);
	stats->tx_255B_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam30[0]);
	stats->tx_255B_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam30[2]);
	stats->tx_511B_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam31[0]);
	stats->tx_511B_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam31[2]);
	stats->tx_1023B_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam32[0]);
	stats->tx_1023B_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam32[2]);
	stats->tx_mtu1_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam33[0]);
	stats->tx_mtu1_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam33[2]);
	stats->tx_mtu2_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam34[0]);
	stats->tx_mtu2_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam34[2]);
	stats->tx_jumbo_bytes += readq(&priv->cpumac_mem->CpuMacStatsRam35[0]);
	stats->tx_jumbo_pkt += readq(&priv->cpumac_mem->CpuMacStatsRam35[2]);
	mtu = readl(&priv->cpumac_reg->CpuMacStatsCfg[1]);
	stats->mtu1 = mtu & 0x3fff;
	stats->mtu2 = (mtu >> 16) & 0x3fff;
	spin_unlock_irqrestore(&priv->reglock, flags);

	memcpy(buf, (void *)stats, sizeof(struct ctcmac_pkt_stats));
}

static uint32_t ctcmac_get_msglevel(struct net_device *dev)
{
	struct ctcmac_private *priv = netdev_priv(dev);

	return priv->msg_enable;
}

static void ctcmac_set_msglevel(struct net_device *dev, uint32_t data)
{
	struct ctcmac_private *priv = netdev_priv(dev);

	priv->msg_enable = data;
}

static int ctcmac_get_ts_info(struct net_device *dev,
			      struct ethtool_ts_info *info)
{
#if 0
	struct gfar_private *priv = netdev_priv(dev);

	if (!(priv->device_flags & FSL_GIANFAR_DEV_HAS_TIMER)) {
		info->so_timestamping = SOF_TIMESTAMPING_RX_SOFTWARE |
		    SOF_TIMESTAMPING_SOFTWARE;
		info->phc_index = -1;
		return 0;
	}
	info->so_timestamping = SOF_TIMESTAMPING_TX_HARDWARE |
	    SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE;
	info->phc_index = gfar_phc_index;
	info->tx_types = (1 << HWTSTAMP_TX_OFF) | (1 << HWTSTAMP_TX_ON);
	info->rx_filters = (1 << HWTSTAMP_FILTER_NONE) |
	    (1 << HWTSTAMP_FILTER_ALL);
#endif
	return 0;
}

const struct ethtool_ops ctcmac_ethtool_ops = {
	.get_drvinfo = ctcmac_gdrvinfo,
	.get_regs_len = ctcmac_reglen,
	.get_regs = ctcmac_get_regs,
	.get_link = ethtool_op_get_link,
	.get_ringparam = ctcmac_gringparam,
	.set_ringparam = ctcmac_sringparam,
	.get_pauseparam = ctcmac_gpauseparam,
	.set_pauseparam = ctcmac_spauseparam,
	.get_strings = ctcmac_gstrings,
	.get_sset_count = ctcmac_sset_count,
	.get_ethtool_stats = ctcmac_fill_stats,
	.get_msglevel = ctcmac_get_msglevel,
	.set_msglevel = ctcmac_set_msglevel,
	.get_link_ksettings = phy_ethtool_get_link_ksettings,
	.set_link_ksettings = phy_ethtool_set_link_ksettings,
	.get_ts_info = ctcmac_get_ts_info,
};

static const struct net_device_ops ctcmac_netdev_ops = {
	.ndo_open = ctcmac_enet_open,
	.ndo_start_xmit = ctcmac_start_xmit,
	.ndo_stop = ctcmac_close,
	.ndo_change_mtu = ctcmac_change_mtu,
	.ndo_set_features = ctcmac_set_features,
	.ndo_set_rx_mode = ctcmac_set_multi,
	.ndo_tx_timeout = ctcmac_timeout,
	.ndo_do_ioctl = ctcmac_ioctl,
	.ndo_get_stats = ctcmac_get_stats,
	.ndo_set_mac_address = ctcmac_set_mac_addr,
	.ndo_validate_addr = eth_validate_addr,
};

static int ctcmac_probe(struct platform_device *ofdev)
{
	struct net_device *dev = NULL;
	struct ctcmac_private *priv = NULL;
	int err = 0, i;

	regmap_base =
	    syscon_regmap_lookup_by_phandle(ofdev->dev.of_node, "ctc,sysctrl");
	if (IS_ERR(regmap_base))
		return PTR_ERR(regmap_base);

	err = ctcmac_of_init(ofdev, &dev);
	if (err) {
		return err;
	}

	priv = netdev_priv(dev);
	SET_NETDEV_DEV(dev, &ofdev->dev);
	INIT_WORK(&priv->reset_task, ctcmac_reset_task);
	platform_set_drvdata(ofdev, priv);
	dev_set_drvdata(&dev->dev, priv);

	dev->base_addr = (unsigned long)priv->iobase;
	dev->watchdog_timeo = TX_TIMEOUT;
	dev->mtu = CTCMAC_DEFAULT_MTU;
	dev->netdev_ops = &ctcmac_netdev_ops;
	dev->ethtool_ops = &ctcmac_ethtool_ops;

	if (priv->version == 0) {
		netif_napi_add(dev, &priv->napi_rx, ctcmac_poll_rx_sq,
			       CTCMAC_NAIP_RX_WEIGHT);
		netif_napi_add(dev, &priv->napi_tx, ctcmac_poll_tx_sq,
			       CTCMAC_NAIP_TX_WEIGHT);
	} else {
		netif_napi_add(dev, &priv->napi_rx, ctcmac_poll_rx0_sq,
			       CTCMAC_NAIP_RX_WEIGHT);
		netif_napi_add(dev, &priv->napi_rx1, ctcmac_poll_rx1_sq,
			       CTCMAC_NAIP_RX_WEIGHT);
		netif_napi_add(dev, &priv->napi_tx, ctcmac_poll_tx_sq,
			       CTCMAC_NAIP_TX_WEIGHT);
	}

	/* Initializing some of the rx/tx queue level parameters */
	for (i = 0; i < priv->num_tx_queues; i++) {
		priv->tx_queue[i]->tx_ring_size = CTCMAC_TX_RING_SIZE;
		priv->tx_queue[i]->num_txbdfree = CTCMAC_TX_RING_SIZE;
	}

	for (i = 0; i < priv->num_rx_queues; i++) {
		priv->rx_queue[i]->rx_ring_size = CTCMAC_RX_RING_SIZE;
	}

	set_bit(CTCMAC_DOWN, &priv->state);

	if (!g_reglock_init_done)
		spin_lock_init(&global_reglock);

	g_reglock_init_done = 1;

	spin_lock_init(&priv->reglock);
	/* Carrier starts down, phylib will bring it up */
	netif_carrier_off(dev);
	err = register_netdev(dev);
	if (err) {
		goto register_fail;
	}

	if (!g_mac_unit_init_done) {
		writel(0x07, &priv->cpumacu_reg->CpuMacUnitResetCtl);
		writel(0x00, &priv->cpumacu_reg->CpuMacUnitResetCtl);

		clrsetbits(&priv->cpumacu_reg->CpuMacUnitTsCfg,
			   0,
			   BIT
			   (CPU_MAC_UNIT_TS_CFG_W0_CFG_FORCE_S_AND_NS_EN_BIT));
		if (priv->interface == PHY_INTERFACE_MODE_SGMII) {
			clrsetbits(&priv->cpumacu_reg->CpuMacUnitRefPulseCfg[1],
				   BIT
				   (CPU_MAC_UNIT_REF_PULSE_CFG_W1_REF_LINK_PULSE_RST_BIT),
				   0);
			writel(0x1f3f,
			       &priv->cpumacu_reg->CpuMacUnitRefPulseCfg[1]);
			ctc_mac_serdes_init(priv);
		}
		g_mac_unit_init_done = 1;
	}

	ctcmac_pps_init(priv);

	mdelay(10);

	sprintf(priv->irqinfo[CTCMAC_NORMAL].name, "%s%s",
		dev->name, "_normal");
	sprintf(priv->irqinfo[CTCMAC_FUNC].name, "%s%s", dev->name, "_func");
	if (priv->version > 0) {
		sprintf(priv->irqinfo[CTCMAC_FUNC_RX0].name, "%s%s",
			dev->name, "_func_rx0");
		sprintf(priv->irqinfo[CTCMAC_FUNC_RX1].name, "%s%s",
			dev->name, "_func_rx1");
	}
	return 0;

register_fail:
	ctcmac_unmap_io_space(priv);
	ctcmac_free_rx_queues(priv);
	ctcmac_free_tx_queues(priv);
	of_node_put(priv->phy_node);
	ctcmac_free_dev(priv);

	return err;
}

static int ctcmac_remove(struct platform_device *ofdev)
{
	struct ctcmac_private *priv = platform_get_drvdata(ofdev);

	device_remove_file(&priv->ndev->dev, &dev_attr_rxq0_pps);
	device_remove_file(&priv->ndev->dev, &dev_attr_rxq1_pps);

	of_node_put(priv->phy_node);

	unregister_netdev(priv->ndev);

	ctcmac_unmap_io_space(priv);
	ctcmac_free_rx_queues(priv);
	ctcmac_free_tx_queues(priv);
	ctcmac_free_dev(priv);

	return 0;
}

static const struct of_device_id ctcmac_match[] = {
	{
	 .type = "network",
	 .compatible = "ctc,mac",
	 },
	{},
};

MODULE_DEVICE_TABLE(of, ctcmac_match);

/* Structure for a device driver */
static struct platform_driver ctcmac_driver = {
	.driver = {
		   .name = "ctc-cpumac",
		   .of_match_table = ctcmac_match,
		   },
	.probe = ctcmac_probe,
	.remove = ctcmac_remove,
};

module_platform_driver(ctcmac_driver);
MODULE_LICENSE("GPL");

static int ctcmac_set_ffe(struct ctcmac_private *priv, u16 coefficient[])
{
	u32 val;

	if (priv->index == 0) {
		val = readl(&priv->cpumacu_reg->CpuMacUnitHssCfg[3]);
		val |= BIT(CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_EN_ADV_BIT)
		    | BIT(CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_EN_DLY_BIT);
		val &= ~CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_TAP_ADV_MASK;
		val |=
		    (coefficient[0] <<
		     CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_TAP_ADV_BIT);
		val &= ~CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_TAP_DLY_MASK;
		val |=
		    (coefficient[2] <<
		     CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_TAP_DLY_BIT);
		writel(val, &priv->cpumacu_reg->CpuMacUnitHssCfg[3]);
		val = readl(&priv->cpumacu_reg->CpuMacUnitHssCfg[6]);
		val &=
		    ~CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_TX_MARGIN_MASK;
		val |=
		    (coefficient[1] <<
		     CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_TX_MARGIN_BIT);
		writel(val, &priv->cpumacu_reg->CpuMacUnitHssCfg[6]);
	} else {
		val = readl(&priv->cpumacu_reg->CpuMacUnitHssCfg[9]);
		val |= BIT(CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_EN_ADV_BIT)
		    | BIT(CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_EN_DLY_BIT);
		val &= ~CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_TAP_ADV_MASK;
		val |=
		    (coefficient[0] <<
		     CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_TAP_ADV_BIT);
		val &= ~CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_TAP_DLY_MASK;
		val |=
		    (coefficient[2] <<
		     CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_TAP_DLY_BIT);
		writel(val, &priv->cpumacu_reg->CpuMacUnitHssCfg[9]);
		val = readl(&priv->cpumacu_reg->CpuMacUnitHssCfg[12]);
		val &=
		    ~CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_TX_MARGIN_MASK;
		val |=
		    (coefficient[1] <<
		     CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_TX_MARGIN_BIT);
		writel(val, &priv->cpumacu_reg->CpuMacUnitHssCfg[12]);
	}

	return 0;
}

static int ctcmac_get_ffe(struct ctcmac_private *priv, u16 coefficient[])
{
	u32 val;

	if (priv->index == 0) {
		val = readl(&priv->cpumacu_reg->CpuMacUnitHssCfg[3]);
		coefficient[0] =
		    (val & CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_TAP_ADV_MASK)
		    >> CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_TAP_ADV_BIT;
		coefficient[2] =
		    (val & CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_TAP_DLY_MASK)
		    >> CPU_MAC_UNIT_HSS_CFG_W3_CFG_HSS_L0_PCS_TAP_DLY_BIT;
		val = readl(&priv->cpumacu_reg->CpuMacUnitHssCfg[6]);
		coefficient[1] =
		    (val &
		     CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_TX_MARGIN_MASK)
		    >>
		    CPU_MAC_UNIT_HSS_CFG_W6_CFG_HSS_L0_PCS2_PMA_TX_MARGIN_BIT;

	} else {
		val = readl(&priv->cpumacu_reg->CpuMacUnitHssCfg[9]);
		coefficient[0] =
		    (val & CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_TAP_ADV_MASK)
		    >> CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_TAP_ADV_BIT;
		coefficient[2] =
		    (val & CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_TAP_DLY_MASK)
		    >> CPU_MAC_UNIT_HSS_CFG_W9_CFG_HSS_L1_PCS_TAP_DLY_BIT;
		val = readl(&priv->cpumacu_reg->CpuMacUnitHssCfg[12]);
		coefficient[1] =
		    (val &
		     CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_TX_MARGIN_MASK)
		    >>
		    CPU_MAC_UNIT_HSS_CFG_W12_CFG_HSS_L1_PCS2_PMA_TX_MARGIN_BIT;
	}

	return 0;
}
