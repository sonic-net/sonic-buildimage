/*
 * Centec CpuMac Ethernet Driver -- CpuMac controller implementation
 * Provides Bus interface for MIIM regs
 *
 * Author: liuht <liuht@centecnetworks.com>
 *
 * Copyright 2002-2017, Centec Networks (Suzhou) Co., Ltd.
 *
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#ifndef __CTCMAC_H
#define __CTCMAC_H

#define TX_TIMEOUT      (5*HZ)

#define CTCMAC_DEFAULT_MTU 1500
#define CTCMAC_MIN_PKT_LEN   64

#define CTCMAC_TX_QUEUE_MAX   1
#define CTCMAC_RX_QUEUE_MAX   2

#define CTCMAC0_EXSRAM_BASE         0x0
#define CTCMAC1_EXSRAM_BASE         0x1800
#define CTCMAC_MAX_RING_SIZE        1023
#define CTCMAC_TX_RING_SIZE   1023
#define CTCMAC_RX_RING_SIZE   1023
#define CTCMAC_RX_BUFF_ALLOC  16
#define CTCMAC_INTERNAL_RING_SIZE 64

/* The maximum number of packets to be handled in one call of gfar_poll */
#define CTCMAC_NAIP_RX_WEIGHT 16
#define CTCMAC_NAIP_TX_WEIGHT 16

#define CTCMAC_RXB_SIZE 1024
#define CTCMAC_SKBFRAG_SIZE (CTCMAC_RXB_SIZE \
			  + SKB_DATA_ALIGN(sizeof(struct skb_shared_info)))
#define CTCMAC_RXB_TRUESIZE 2048
#define BUF_ALIGNMENT 256
#define CTCMAC_JUMBO_FRAME_SIZE 9600

#define CTCMAC_TOKEN_PER_PKT  10
#define CTCMAC_TIMER_COMPENSATE 1

#define CTCMAC_NOR_RX1_R    (1<<7)
#define CTCMAC_NOR_RX0_R    (1<<6)
#define CTCMAC_NOR_RX1_D    (1<<5)
#define CTCMAC_NOR_RX0_D    (1<<4)
#define CTCMAC_NOR_TX_D     (1<<3)
#define CTCMAC_NOR_AN_D     (1<<2)
#define CTCMAC_NOR_LINK_DOWN     (1<<1)
#define CTCMAC_NOR_LINK_UP       (1<<0)
#define CTCMAC_FUNC0_RX_D   (1<<0)
#define CTCMAC_FUNC0_RX_R   (1<<1)
#define CTCMAC_FUNC1_RX_D   (1<<0)
#define CTCMAC_FUNC1_RX_R   (1<<1)

#define CTC_DDR_BASE 0x80000000

#define CSA_SGMII_MD_MASK 0x00000008
#define CSA_EN  0x00000001

/*Mask*/
/*CTCMAC_SGMII_CFG*/
#define CSC_REP_MASK   0x1fc00000
#define CSC_SMP_MASK   0x1fc00000
/*CTCMAC_SGMII_MON*/
#define CSM_ANST_MASK 0x00000007

#define CSC_1000M 0x00000000
#define CSC_100M  0x02400000
#define CSC_10M    0x18c00000

#define DESC_INT_COALESCE_CNT_MIN 1
#define DESC_TX_INT_COALESCE_CNT_DEFAULT 16
#define DESC_RX_INT_COALESCE_CNT_DEFAULT 16

/* emu 100us */
//#define CTCMAC_TIMER_THRD     0x4B0
/* board 100us */
#define CTCMAC_TIMER_THRD     0xc350

#define CTCMAC_SUPPORTED (SUPPORTED_10baseT_Full \
		| SUPPORTED_100baseT_Full \
		| SUPPORTED_1000baseT_Full \
		| SUPPORTED_Autoneg)

#define CTCMAC_STATS_LEN  (sizeof(struct ctcmac_pkt_stats)/sizeof(u64))

struct ctcmac_skb_cb {
	unsigned int bytes_sent;	/* bytes-on-wire (i.e. no FCB) */
};
#define CTCMAC_CB(skb) ((struct ctcmac_skb_cb *)((skb)->cb))

enum ctcmac_irqinfo_id {
	CTCMAC_NORMAL = 0,
	CTCMAC_FUNC,
	CTCMAC_UNIT,
	CTCMAC_FUNC_RX0,
	CTCMAC_FUNC_RX1,
	CTCMAC_NUM_IRQS
};

enum ctcmac_dev_state {
	CTCMAC_DOWN = 1,
	CTCMAC_RESETTING
};

enum ctcmac_int_type {
	CTCMAC_INT_PACKET = 1,
	CTCMAC_INT_DESC,
	CTCMAC_INT_MAX
};

enum ctcmac_tx_pol_inv {
	CTCMAC_TX_POL_INV_DISABLE,
	CTCMAC_TX_POL_INV_ENABLE,
};

enum ctcmac_rx_pol_inv {
	CTCMAC_RX_POL_INV_DISABLE,
	CTCMAC_RX_POL_INV_ENABLE,
};

enum ctcmac_autoneg {
	CTCMAC_AUTONEG_1000BASEX_M,
	CTCMAC_AUTONEG_PHY_M,
	CTCMAC_AUTONEG_MAC_M,
	CTCMAC_AUTONEG_DISABLE,
	CTCMAC_AUTONEG_MAX
};

/*
 * Per TX queue stats
 */
struct txq_stats {
	unsigned long tx_packets;
	unsigned long tx_bytes;
};

struct tx_skb {
	struct sk_buff *skb;
	int frag_merge;
};

struct ctcmac_tx_buff {
	void *vaddr;
	dma_addr_t dma;
	u32 len;
	u32 offset;
	bool alloc;
};

struct ctcmac_priv_tx_q {
	spinlock_t txlock __attribute__ ((aligned(SMP_CACHE_BYTES)));
	struct ctcmac_tx_buff tx_buff[CTCMAC_MAX_RING_SIZE + 1];
	unsigned int num_txbdfree;
	u16 tx_ring_size;
	u16 qindex;
	u16 skb_cur;
	u16 skb_dirty;
	u16 desc_cur;
	u16 desc_dirty;
	struct txq_stats stats;
	struct net_device *dev;
	struct tx_skb *tx_skbuff;
	struct napi_struct napi_tx;
};

/*
 * Per RX queue stats
 */
struct rxq_stats {
	unsigned long rx_packets;
	unsigned long rx_bytes;
	unsigned long rx_dropped;
};

struct ctcmac_rx_buff {
	dma_addr_t dma;
	struct page *page;
	unsigned int page_offset;
};

struct ctcmac_priv_rx_q {
	struct ctcmac_rx_buff *rx_buff __aligned(SMP_CACHE_BYTES);
	struct net_device *ndev;
	struct device *dev;
	u16 rx_ring_size;
	u16 qindex;
	u16 next_to_clean;
	u16 next_to_use;
	u16 next_to_alloc;
	struct sk_buff *skb;
	struct rxq_stats stats;
	u32 pps_limit;
	u32 token, token_max;
	u32 rx_trigger;
	struct napi_struct napi_rx;
};

struct ctcmac_irqinfo {
	unsigned int irq;
	char name[32];
};

struct ctcmac_desc_cfg {
	u8 err_type;		/*used when err == 1 */
	u8 err;
	u8 eop;
	u8 sop;
	u32 size;
	u32 addr_high;
	u32 addr_low;
};

struct ctcmac_private {
	spinlock_t reglock __aligned(SMP_CACHE_BYTES);
	struct device *dev;
	struct net_device *ndev;
	void __iomem *iobase;
	struct CpuMac_regs __iomem *cpumac_reg;
	struct CpuMac_mems __iomem *cpumac_mem;
	struct CpuMacUnit_regs *cpumacu_reg;
	u32 device_flags;
	int irq_num;
	int index;

	struct ctcmac_priv_tx_q *tx_queue[CTCMAC_TX_QUEUE_MAX];
	struct ctcmac_priv_rx_q *rx_queue[CTCMAC_RX_QUEUE_MAX];

	unsigned long state;
	unsigned int num_tx_queues;
	unsigned int num_rx_queues;

	/* PHY stuff */
	phy_interface_t interface;
	struct device_node *phy_node;
	struct mii_bus *mii_bus;
	int oldspeed;
	int oldlink;
	int oldduplex;

	struct work_struct reset_task;
	struct platform_device *ofdev;
	struct ctcmac_irqinfo irqinfo[CTCMAC_NUM_IRQS];
	struct napi_struct napi_tx;
	struct napi_struct napi_rx;
	struct napi_struct napi_rx1;

	int hwts_rx_en;
	int hwts_tx_en;
	u32 autoneg_mode;
	u32 supported;
	u32 msg_enable;
	u32 int_type;
	u32 rx_int_coalesce_cnt;
	u32 tx_int_coalesce_cnt;
	u8 dfe_enable;
	u8 tx_pol_inv;
	u8 rx_pol_inv;
	struct timer_list token_timer;
	u8 version;
	u8 pause_aneg_en;
	u8 tx_pause_en;
	u8 rx_pause_en;
};

struct ctcmac_pkt_stats {
	u64 rx_good_ucast_bytes;
	u64 rx_good_ucast_pkt;
	u64 rx_good_mcast_bytes;
	u64 rx_good_mcast_pkt;
	u64 rx_good_bcast_bytes;
	u64 rx_good_bcast_pkt;
	u64 rx_good_pause_bytes;
	u64 rx_good_pause_pkt;
	u64 rx_good_pfc_bytes;
	u64 rx_good_pfc_pkt;
	u64 rx_good_control_bytes;
	u64 rx_good_control_pkt;
	u64 rx_fcs_error_bytes;
	u64 rx_fcs_error_pkt;
	u64 rx_mac_overrun_bytes;
	u64 rx_mac_overrun_pkt;
	u64 rx_good_63B_bytes;
	u64 rx_good_63B_pkt;
	u64 rx_bad_63B_bytes;
	u64 rx_bad_63B_pkt;
	u64 rx_good_mtu2B_bytes;
	u64 rx_good_mtu2B_pkt;
	u64 rx_bad_mtu2B_bytes;
	u64 rx_bad_mtu2B_pkt;
	u64 rx_good_jumbo_bytes;
	u64 rx_good_jumbo_pkt;
	u64 rx_bad_jumbo_bytes;
	u64 rx_bad_jumbo_pkt;
	u64 rx_64B_bytes;
	u64 rx_64B_pkt;
	u64 rx_127B_bytes;
	u64 rx_127B_pkt;
	u64 rx_255B_bytes;
	u64 rx_255B_pkt;
	u64 rx_511B_bytes;
	u64 rx_511B_pkt;
	u64 rx_1023B_bytes;
	u64 rx_1023B_pkt;
	u64 rx_mtu1B_bytes;
	u64 rx_mtu1B_pkt;
	u64 tx_ucast_bytes;
	u64 tx_ucast_pkt;
	u64 tx_mcast_bytes;
	u64 tx_mcast_pkt;
	u64 tx_bcast_bytes;
	u64 tx_bcast_pkt;
	u64 tx_pause_bytes;
	u64 tx_pause_pkt;
	u64 tx_control_bytes;
	u64 tx_control_pkt;
	u64 tx_fcs_error_bytes;
	u64 tx_fcs_error_pkt;
	u64 tx_underrun_bytes;
	u64 tx_underrun_pkt;
	u64 tx_63B_bytes;
	u64 tx_63B_pkt;
	u64 tx_64B_bytes;
	u64 tx_64B_pkt;
	u64 tx_127B_bytes;
	u64 tx_127B_pkt;
	u64 tx_255B_bytes;
	u64 tx_255B_pkt;
	u64 tx_511B_bytes;
	u64 tx_511B_pkt;
	u64 tx_1023B_bytes;
	u64 tx_1023B_pkt;
	u64 tx_mtu1_bytes;
	u64 tx_mtu1_pkt;
	u64 tx_mtu2_bytes;
	u64 tx_mtu2_pkt;
	u64 tx_jumbo_bytes;
	u64 tx_jumbo_pkt;
	u64 mtu1;
	u64 mtu2;
};

#endif
