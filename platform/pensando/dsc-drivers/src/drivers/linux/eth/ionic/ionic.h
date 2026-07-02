/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2017 - 2022 Pensando Systems, Inc */

#ifndef _IONIC_H_
#define _IONIC_H_

struct ionic_lif;

#include "kcompat.h"

#include "ionic_if.h"
#include "ionic_dev.h"
#include "ionic_devlink.h"

#define IONIC_DRV_NAME		"ionic"
#define IONIC_DRV_DESCRIPTION	"Pensando Ethernet NIC Driver"
#define IONIC_DRV_VERSION	drv_ver

#define PCI_VENDOR_ID_PENSANDO			0x1dd8

#define PCI_DEVICE_ID_PENSANDO_IONIC_ETH_PF	0x1002
#define PCI_DEVICE_ID_PENSANDO_IONIC_ETH_VF	0x1003
#define PCI_DEVICE_ID_PENSANDO_IONIC_ETH_MGMT	0x1004

#define DEVCMD_TOUT_DEF 5
#define DEVCMD_TIMEOUT  devcmd_timeout
#define SHORT_TIMEOUT   1
#define IONIC_ADMINQ_TIME_SLICE	msecs_to_jiffies(100)

#define IONIC_PHC_UPDATE_NS	10000000000L	    /* 10s in nanoseconds */
#define NORMAL_PPB		1000000000	    /* one billion parts per billion */
#define SCALED_PPM		(1000000ull << 16)  /* 2^16 million parts per 2^16 million */

extern bool port_init_up;
extern unsigned int rx_copybreak;
extern unsigned int rx_fill_threshold;
extern unsigned int tx_budget;
extern unsigned int devcmd_timeout;
extern unsigned long affinity_mask_override;

struct ionic_vf {
	u16	 index;
	u8	 macaddr[6];
	__le32	 maxrate;
	__le16	 vlanid;
	u8	 spoofchk;
	u8	 trusted;
	u8	 linkstate;
	dma_addr_t       stats_pa;
	struct ionic_lif_stats stats;
};

struct ionic {
	struct pci_dev *pdev;
	struct platform_device *pfdev;
	struct device *dev;
	struct ionic_dev idev;
	struct mutex dev_cmd_lock;	/* lock for dev_cmd operations */
	struct dentry *dentry;
	struct ionic_dev_bar bars[IONIC_BARS_MAX];
	unsigned int num_bars;
	struct ionic_identity ident;
	bool is_mgmt_nic;
	struct ionic_lif *lif;
	unsigned int nnqs_per_lif;
	unsigned int nrdma_eqs_per_lif;
	unsigned int ntxqs_per_lif;
	unsigned int nrxqs_per_lif;
	unsigned int nlifs;
	unsigned int nintrs;
	DECLARE_BITMAP(intrs, IONIC_INTR_CTRL_REGS_MAX);
#ifndef HAVE_PCI_IRQ_API
	struct msix_entry *msix;
#endif
	struct work_struct nb_work;
	struct notifier_block nb;
#ifdef IONIC_DEVLINK
	struct devlink_port dl_port;
#endif
	struct rw_semaphore vf_op_lock;	/* lock for VF operations */
	struct ionic_vf *vfs;
	int num_vfs;
	struct timer_list watchdog_timer;
	int watchdog_period;
};

int ionic_adminq_post(struct ionic_lif *lif, struct ionic_admin_ctx *ctx);
int ionic_adminq_wait(struct ionic_lif *lif, struct ionic_admin_ctx *ctx,
		      const int err, const bool do_msg);
int ionic_adminq_post_wait(struct ionic_lif *lif, struct ionic_admin_ctx *ctx);
int ionic_adminq_post_wait_nomsg(struct ionic_lif *lif, struct ionic_admin_ctx *ctx);
void ionic_adminq_netdev_err_print(struct ionic_lif *lif, u8 opcode,
				   u8 status, int err);

int ionic_dev_cmd_wait(struct ionic *ionic, unsigned long max_wait);
int ionic_dev_cmd_wait_nomsg(struct ionic *ionic, unsigned long max_wait);
void ionic_dev_cmd_dev_err_print(struct ionic *ionic, u8 opcode, u8 status,
				 int err);
int ionic_set_dma_mask(struct ionic *ionic);
int ionic_setup(struct ionic *ionic);

int ionic_identify(struct ionic *ionic);
int ionic_init(struct ionic *ionic);
int ionic_reset(struct ionic *ionic);

int ionic_port_identify(struct ionic *ionic);
int ionic_port_init(struct ionic *ionic);
int ionic_port_reset(struct ionic *ionic);

#endif /* _IONIC_H_ */
