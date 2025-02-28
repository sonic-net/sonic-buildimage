// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2017 - 2022 Pensando Systems, Inc */

#include <linux/netdevice.h>

#include "ionic.h"
#include "ionic_bus.h"
#include "ionic_lif.h"
#include "ionic_ethtool.h"
#include "ionic_debugfs.h"
#include "kcompat.h"

#ifdef CONFIG_DEBUG_FS

static struct dentry *ionic_dir;

void ionic_debugfs_create(void)
{
	ionic_dir = debugfs_create_dir(IONIC_DRV_NAME, NULL);
}

void ionic_debugfs_destroy(void)
{
	debugfs_remove_recursive(ionic_dir);
}

void ionic_debugfs_add_dev(struct ionic *ionic)
{
	ionic->dentry = debugfs_create_dir(ionic_bus_info(ionic), ionic_dir);
}

void ionic_debugfs_del_dev(struct ionic *ionic)
{
	debugfs_remove_recursive(ionic->dentry);
	ionic->dentry = NULL;
}

static int bars_show(struct seq_file *seq, void *v)
{
	struct ionic *ionic = seq->private;
	struct ionic_dev_bar *bars = ionic->bars;
	unsigned int i;

	for (i = 0; i < IONIC_BARS_MAX; i++)
		if (bars[i].len)
			seq_printf(seq, "BAR%d: res %d len 0x%08lx vaddr %pK bus_addr 0x%016llx\n",
				   i, bars[i].res_index, bars[i].len,
				   bars[i].vaddr, bars[i].bus_addr);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(bars);

void ionic_debugfs_add_bars(struct ionic *ionic)
{
	debugfs_create_file("bars", 0400, ionic->dentry, ionic, &bars_fops);
}

static const struct debugfs_reg32 dev_cmd_regs[] = {
	{ .name = "db", .offset = 0, },
	{ .name = "done", .offset = 4, },
	{ .name = "cmd.word[0]", .offset = 8, },
	{ .name = "cmd.word[1]", .offset = 12, },
	{ .name = "cmd.word[2]", .offset = 16, },
	{ .name = "cmd.word[3]", .offset = 20, },
	{ .name = "cmd.word[4]", .offset = 24, },
	{ .name = "cmd.word[5]", .offset = 28, },
	{ .name = "cmd.word[6]", .offset = 32, },
	{ .name = "cmd.word[7]", .offset = 36, },
	{ .name = "cmd.word[8]", .offset = 40, },
	{ .name = "cmd.word[9]", .offset = 44, },
	{ .name = "cmd.word[10]", .offset = 48, },
	{ .name = "cmd.word[11]", .offset = 52, },
	{ .name = "cmd.word[12]", .offset = 56, },
	{ .name = "cmd.word[13]", .offset = 60, },
	{ .name = "cmd.word[14]", .offset = 64, },
	{ .name = "cmd.word[15]", .offset = 68, },
	{ .name = "comp.word[0]", .offset = 72, },
	{ .name = "comp.word[1]", .offset = 76, },
	{ .name = "comp.word[2]", .offset = 80, },
	{ .name = "comp.word[3]", .offset = 84, },
};

void ionic_debugfs_add_dev_cmd(struct ionic *ionic)
{
	struct debugfs_regset32 *dev_cmd_regset;
	struct device *dev = ionic->dev;

	dev_cmd_regset = devm_kzalloc(dev, sizeof(*dev_cmd_regset), GFP_KERNEL);
	if (!dev_cmd_regset)
		return;
	dev_cmd_regset->regs = dev_cmd_regs;
	dev_cmd_regset->nregs = ARRAY_SIZE(dev_cmd_regs);
	dev_cmd_regset->base = ionic->idev.dev_cmd_regs;

	debugfs_create_regset32("dev_cmd", 0400, ionic->dentry, dev_cmd_regset);
}

static void identity_show_qtype(struct seq_file *seq, const char *name,
				struct ionic_lif_logical_qtype *qtype)
{
	seq_printf(seq, "%s_qtype:\t%d\n", name, qtype->qtype);
	seq_printf(seq, "%s_count:\t%d\n", name, qtype->qid_count);
	seq_printf(seq, "%s_base:\t%d\n", name, qtype->qid_base);
}

static int identity_show(struct seq_file *seq, void *v)
{
	struct ionic *ionic = seq->private;
	struct ionic_identity *ident;
	struct ionic_dev *idev;

	ident = &ionic->ident;
	idev = &ionic->idev;

	seq_printf(seq, "asic_type:        0x%x\n", idev->dev_info.asic_type);
	seq_printf(seq, "asic_rev:         0x%x\n", idev->dev_info.asic_rev);
	seq_printf(seq, "serial_num:       %s\n", idev->dev_info.serial_num);
	seq_printf(seq, "fw_version:       %s\n", idev->dev_info.fw_version);
	seq_printf(seq, "fw_status:        0x%x\n",
		   ioread8(&idev->dev_info_regs->fw_status));
	seq_printf(seq, "fw_heartbeat:     0x%x\n",
		   ioread32(&idev->dev_info_regs->fw_heartbeat));
	seq_printf(seq, "cmb_pages:        0x%x\n", ionic_cmb_pages_in_use(ionic->lif));

	seq_printf(seq, "nlifs:            %d\n", ident->dev.nlifs);
	seq_printf(seq, "nintrs:           %d\n", ident->dev.nintrs);
	seq_printf(seq, "eth_eq_count:     %d\n", ident->dev.eq_count);
	seq_printf(seq, "ndbpgs_per_lif:   %d\n", ident->dev.ndbpgs_per_lif);
	seq_printf(seq, "intr_coal_mult:   %d\n", ident->dev.intr_coal_mult);
	seq_printf(seq, "intr_coal_div:    %d\n", ident->dev.intr_coal_div);

	seq_printf(seq, "max_ucast_filters:  %d\n", ident->lif.eth.max_ucast_filters);
	seq_printf(seq, "max_mcast_filters:  %d\n", ident->lif.eth.max_mcast_filters);

	seq_printf(seq, "rdma_qp_opcodes:  %d\n", ident->lif.rdma.qp_opcodes);
	seq_printf(seq, "rdma_admin_opcodes: %d\n", ident->lif.rdma.admin_opcodes);
	seq_printf(seq, "rdma_max_stride:    %d\n", ident->lif.rdma.max_stride);
	seq_printf(seq, "rdma_cl_stride:    %d\n", ident->lif.rdma.cl_stride);
	seq_printf(seq, "rdma_pte_stride:    %d\n", ident->lif.rdma.pte_stride);
	seq_printf(seq, "rdma_rrq_stride:    %d\n", ident->lif.rdma.rrq_stride);
	seq_printf(seq, "rdma_rsq_stride:    %d\n", ident->lif.rdma.rsq_stride);

	identity_show_qtype(seq, "rdma_aq", &ident->lif.rdma.aq_qtype);
	identity_show_qtype(seq, "rdma_sq", &ident->lif.rdma.sq_qtype);
	identity_show_qtype(seq, "rdma_rq", &ident->lif.rdma.rq_qtype);
	identity_show_qtype(seq, "rdma_cq", &ident->lif.rdma.cq_qtype);
	identity_show_qtype(seq, "rdma_eq", &ident->lif.rdma.eq_qtype);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(identity);

void ionic_debugfs_add_ident(struct ionic *ionic)
{
	debugfs_create_file("identity", 0400, ionic->dentry,
			    ionic, &identity_fops);
}

void ionic_debugfs_add_sizes(struct ionic *ionic)
{
	debugfs_create_u32("nlifs", 0400, ionic->dentry,
			   (u32 *)&ionic->ident.dev.nlifs);
	debugfs_create_u32("nintrs", 0400, ionic->dentry, &ionic->nintrs);

	debugfs_create_u32("ntxqs_per_lif", 0400, ionic->dentry,
			   (u32 *)&ionic->ident.lif.eth.config.queue_count[IONIC_QTYPE_TXQ]);
	debugfs_create_u32("nrxqs_per_lif", 0400, ionic->dentry,
			   (u32 *)&ionic->ident.lif.eth.config.queue_count[IONIC_QTYPE_RXQ]);
}

static int q_tail_show(struct seq_file *seq, void *v)
{
	struct ionic_queue *q = seq->private;

	seq_printf(seq, "%d\n", q->tail_idx);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(q_tail);

static int q_head_show(struct seq_file *seq, void *v)
{
	struct ionic_queue *q = seq->private;

	seq_printf(seq, "%d\n", q->head_idx);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(q_head);

static int cq_tail_show(struct seq_file *seq, void *v)
{
	struct ionic_cq *cq = seq->private;

	seq_printf(seq, "%d\n", cq->tail_idx);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(cq_tail);

static const struct debugfs_reg32 intr_ctrl_regs[] = {
	{ .name = "coal_init", .offset = 0, },
	{ .name = "mask", .offset = 4, },
	{ .name = "credits", .offset = 8, },
	{ .name = "mask_on_assert", .offset = 12, },
	{ .name = "coal_timer", .offset = 16, },
};

void ionic_debugfs_add_qcq(struct ionic_lif *lif, struct ionic_qcq *qcq)
{
	struct dentry *qcq_dentry, *q_dentry, *cq_dentry;
	struct dentry *intr_dentry, *stats_dentry;
	struct ionic_dev *idev = &lif->ionic->idev;
	struct debugfs_regset32 *intr_ctrl_regset;
	struct ionic_intr_info *intr = &qcq->intr;
	struct debugfs_blob_wrapper *desc_blob;
	struct device *dev = lif->ionic->dev;
	struct ionic_tx_stats *txqstats;
	struct ionic_rx_stats *rxqstats;
	struct ionic_queue *q = &qcq->q;
	struct ionic_cq *cq = &qcq->cq;

	qcq_dentry = debugfs_create_dir(q->name, lif->dentry);
	if (IS_ERR_OR_NULL(qcq_dentry))
		return;
	qcq->dentry = qcq_dentry;

	debugfs_create_x64("q_base_pa", 0400, qcq_dentry, &qcq->q_base_pa);
	debugfs_create_x32("q_size", 0400, qcq_dentry, &qcq->q_size);
	debugfs_create_x64("cq_base_pa", 0400, qcq_dentry, &qcq->cq_base_pa);
	debugfs_create_x32("cq_size", 0400, qcq_dentry, &qcq->cq_size);
	debugfs_create_x64("sg_base_pa", 0400, qcq_dentry, &qcq->sg_base_pa);
	debugfs_create_x32("sg_size", 0400, qcq_dentry, &qcq->sg_size);
	debugfs_create_x32("cmb_order", 0400, qcq_dentry, &qcq->cmb_order);
	debugfs_create_x32("cmb_pgid", 0400, qcq_dentry, &qcq->cmb_pgid);

#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_VERSION(7, 0) < RHEL_RELEASE_CODE))
	debugfs_create_u8("armed", 0400, qcq_dentry, (u8 *)&qcq->armed);
#else
	debugfs_create_bool("armed", 0400, qcq_dentry, &qcq->armed);
#endif

	q_dentry = debugfs_create_dir("q", qcq->dentry);
	if (IS_ERR_OR_NULL(q_dentry))
		return;

	debugfs_create_u32("index", 0400, q_dentry, &q->index);
	debugfs_create_u32("num_descs", 0400, q_dentry, &q->num_descs);
	debugfs_create_u32("desc_size", 0400, q_dentry, &q->desc_size);
	debugfs_create_u32("pid", 0400, q_dentry, &q->pid);
	debugfs_create_u32("qid", 0400, q_dentry, &q->hw_index);
	debugfs_create_u32("qtype", 0400, q_dentry, &q->hw_type);
	debugfs_create_u64("drop", 0400, q_dentry, &q->drop);
	debugfs_create_u64("stop", 0400, q_dentry, &q->stop);
	debugfs_create_u64("wake", 0400, q_dentry, &q->wake);

	debugfs_create_file("tail", 0400, q_dentry, q, &q_tail_fops);
	debugfs_create_file("head", 0400, q_dentry, q, &q_head_fops);

	desc_blob = devm_kzalloc(dev, sizeof(*desc_blob), GFP_KERNEL);
	if (!desc_blob)
		return;
	desc_blob->data = q->base;
	desc_blob->size = (unsigned long)q->num_descs * q->desc_size;
	debugfs_create_blob("desc_blob", 0400, q_dentry, desc_blob);

	if (qcq->flags & IONIC_QCQ_F_SG) {
		desc_blob = devm_kzalloc(dev, sizeof(*desc_blob), GFP_KERNEL);
		if (!desc_blob)
			return;
		desc_blob->data = q->sg_base;
		desc_blob->size = (unsigned long)q->num_descs * q->sg_desc_size;
		debugfs_create_blob("sg_desc_blob", 0400, q_dentry,
				    desc_blob);
	}

	if (qcq->flags & IONIC_QCQ_F_TX_STATS) {
		stats_dentry = debugfs_create_dir("tx_stats", q_dentry);
		if (IS_ERR_OR_NULL(stats_dentry))
			return;
		txqstats = &lif->txqstats[q->index];

		debugfs_create_u64("dma_map_err", 0400, stats_dentry,
				   &txqstats[q->index].dma_map_err);
		debugfs_create_u64("pkts", 0400, stats_dentry,
				   &txqstats[q->index].pkts);
		debugfs_create_u64("bytes", 0400, stats_dentry,
				   &txqstats[q->index].bytes);
		debugfs_create_u64("clean", 0400, stats_dentry,
				   &txqstats[q->index].clean);
		debugfs_create_u64("linearize", 0400, stats_dentry,
				   &txqstats[q->index].linearize);
		debugfs_create_u64("csum_none", 0400, stats_dentry,
				   &txqstats[q->index].csum_none);
		debugfs_create_u64("csum", 0400, stats_dentry,
				   &txqstats[q->index].csum);
		debugfs_create_u64("crc32_csum", 0400, stats_dentry,
				   &txqstats[q->index].crc32_csum);
		debugfs_create_u64("tso", 0400, stats_dentry,
				   &txqstats[q->index].tso);
		debugfs_create_u64("frags", 0400, stats_dentry,
				   &txqstats[q->index].frags);
	}

	if (qcq->flags & IONIC_QCQ_F_RX_STATS) {
		stats_dentry = debugfs_create_dir("rx_stats", q_dentry);
		if (IS_ERR_OR_NULL(stats_dentry))
			return;
		rxqstats = &lif->rxqstats[q->index];

		debugfs_create_u64("dma_map_err", 0400, stats_dentry,
				   &rxqstats[q->index].dma_map_err);
		debugfs_create_u64("alloc_err", 0400, stats_dentry,
				   &rxqstats[q->index].alloc_err);
		debugfs_create_u64("pkts", 0400, stats_dentry,
				   &rxqstats[q->index].pkts);
		debugfs_create_u64("bytes", 0400, stats_dentry,
				   &rxqstats[q->index].bytes);
		debugfs_create_u64("csum_none", 0400, stats_dentry,
				   &rxqstats[q->index].csum_none);
		debugfs_create_u64("csum_complete", 0400, stats_dentry,
				   &rxqstats[q->index].csum_complete);
		debugfs_create_u64("csum_error", 0400, stats_dentry,
				   &rxqstats[q->index].csum_error);
	}

	cq_dentry = debugfs_create_dir("cq", qcq->dentry);
	if (IS_ERR_OR_NULL(cq_dentry))
		return;

	debugfs_create_x64("base_pa", 0400, cq_dentry, &cq->base_pa);
	debugfs_create_u32("num_descs", 0400, cq_dentry, &cq->num_descs);
	debugfs_create_u32("desc_size", 0400, cq_dentry, &cq->desc_size);

#if (RHEL_RELEASE_CODE && (RHEL_RELEASE_VERSION(7, 0) < RHEL_RELEASE_CODE))
	debugfs_create_u8("done_color", 0400, cq_dentry, (u8 *)&cq->done_color);
#else
	debugfs_create_bool("done_color", 0400, cq_dentry, &cq->done_color);
#endif

	debugfs_create_file("tail", 0400, cq_dentry, cq, &cq_tail_fops);

	desc_blob = devm_kzalloc(dev, sizeof(*desc_blob), GFP_KERNEL);
	if (!desc_blob)
		return;
	desc_blob->data = cq->base;
	desc_blob->size = (unsigned long)cq->num_descs * cq->desc_size;
	debugfs_create_blob("desc_blob", 0400, cq_dentry, desc_blob);

	if (qcq->flags & IONIC_QCQ_F_INTR) {
		intr_dentry = debugfs_create_dir("intr", qcq->dentry);
		if (IS_ERR_OR_NULL(intr_dentry))
			return;

		debugfs_create_u32("index", 0400, intr_dentry,
				   &intr->index);
		debugfs_create_u32("vector", 0400, intr_dentry,
				   &intr->vector);
		debugfs_create_u32("dim_coal_hw", 0400, intr_dentry,
				   &intr->dim_coal_hw);
		debugfs_create_u16("dim_coal_usecs", 0400, intr_dentry,
				   &intr->dim_coal_usecs);

		intr_ctrl_regset = devm_kzalloc(dev, sizeof(*intr_ctrl_regset),
						GFP_KERNEL);
		if (!intr_ctrl_regset)
			return;
		intr_ctrl_regset->regs = intr_ctrl_regs;
		intr_ctrl_regset->nregs = ARRAY_SIZE(intr_ctrl_regs);
		intr_ctrl_regset->base = &idev->intr_ctrl[intr->index];

		debugfs_create_regset32("intr_ctrl", 0400, intr_dentry,
					intr_ctrl_regset);
	}

	if (qcq->flags & IONIC_QCQ_F_NOTIFYQ) {
		stats_dentry = debugfs_create_dir("notifyblock", qcq->dentry);
		if (IS_ERR_OR_NULL(stats_dentry))
			return;

		debugfs_create_u64("eid", 0400, stats_dentry,
				   (u64 *)&lif->info->status.eid);
		debugfs_create_u16("link_status", 0400, stats_dentry,
				   (u16 *)&lif->info->status.link_status);
		debugfs_create_u32("link_speed", 0400, stats_dentry,
				   (u32 *)&lif->info->status.link_speed);
		debugfs_create_u16("link_down_count", 0400, stats_dentry,
				   (u16 *)&lif->info->status.link_down_count);
	}
}

static int netdev_show(struct seq_file *seq, void *v)
{
	struct net_device *netdev = seq->private;

	seq_printf(seq, "%s\n", netdev->name);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(netdev);

static int lif_identity_show(struct seq_file *seq, void *v)
{
	union ionic_lif_identity *lid = seq->private;

	seq_printf(seq, "capabilities:      0x%llx\n", lid->capabilities);
	seq_printf(seq, "eth-version:       0x%x\n", lid->eth.version);
	seq_printf(seq, "max_ucast_filters: %d\n", lid->eth.max_ucast_filters);
	seq_printf(seq, "max_mcast_filters: %d\n", lid->eth.max_mcast_filters);
	seq_printf(seq, "rss_ind_tbl_sz:    %d\n", lid->eth.rss_ind_tbl_sz);
	seq_printf(seq, "min_frame_size:    %d\n", lid->eth.min_frame_size);
	seq_printf(seq, "max_frame_size:    %d\n", lid->eth.max_frame_size);

	seq_printf(seq, "state:             %d\n", lid->eth.config.state);
	seq_printf(seq, "name:              \"%s\"\n", lid->eth.config.name);
	seq_printf(seq, "mtu:               %d\n", lid->eth.config.mtu);
	seq_printf(seq, "mac:               %pM\n", lid->eth.config.mac);
	seq_printf(seq, "features:          0x%08llx\n",
		   lid->eth.config.features);
	seq_printf(seq, "adminq-count:      %d\n",
		   lid->eth.config.queue_count[IONIC_QTYPE_ADMINQ]);
	seq_printf(seq, "notifyq-count:     %d\n",
		   lid->eth.config.queue_count[IONIC_QTYPE_NOTIFYQ]);
	seq_printf(seq, "rxq-count:         %d\n",
		   lid->eth.config.queue_count[IONIC_QTYPE_RXQ]);
	seq_printf(seq, "txq-count:         %d\n",
		   lid->eth.config.queue_count[IONIC_QTYPE_TXQ]);
	seq_printf(seq, "eq-count:          %d\n",
		   lid->eth.config.queue_count[IONIC_QTYPE_EQ]);

	seq_puts(seq, "\n");

	seq_printf(seq, "rdma_version:        0x%x\n", lid->rdma.version);
	seq_printf(seq, "rdma_qp_opcodes:     %d\n", lid->rdma.qp_opcodes);
	seq_printf(seq, "rdma_admin_opcodes:  %d\n", lid->rdma.admin_opcodes);
	seq_printf(seq, "rdma_npts_per_lif:   %d\n", lid->rdma.npts_per_lif);
	seq_printf(seq, "rdma_nmrs_per_lif:   %d\n", lid->rdma.nmrs_per_lif);
	seq_printf(seq, "rdma_nahs_per_lif:   %d\n", lid->rdma.nahs_per_lif);
	seq_printf(seq, "rdma_max_stride:     %d\n", lid->rdma.max_stride);
	seq_printf(seq, "rdma_cl_stride:      %d\n", lid->rdma.cl_stride);
	seq_printf(seq, "rdma_pte_stride:     %d\n", lid->rdma.pte_stride);
	seq_printf(seq, "rdma_rrq_stride:     %d\n", lid->rdma.rrq_stride);
	seq_printf(seq, "rdma_rsq_stride:     %d\n", lid->rdma.rsq_stride);
	seq_printf(seq, "rdma_dcqcn_profiles: %d\n", lid->rdma.dcqcn_profiles);

	identity_show_qtype(seq, "rdma_aq", &lid->rdma.aq_qtype);
	identity_show_qtype(seq, "rdma_sq", &lid->rdma.sq_qtype);
	identity_show_qtype(seq, "rdma_rq", &lid->rdma.rq_qtype);
	identity_show_qtype(seq, "rdma_cq", &lid->rdma.cq_qtype);
	identity_show_qtype(seq, "rdma_eq", &lid->rdma.eq_qtype);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(lif_identity);

static int lif_state_show(struct seq_file *seq, void *v)
{
	struct ionic_lif *lif = seq->private;

	seq_printf(seq, "0x%08lx\n", lif->state[0]);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(lif_state);

static int lif_filters_show(struct seq_file *seq, void *v)
{
	struct ionic_lif *lif = seq->private;
	struct ionic_rx_filter *f;
	struct hlist_head *head;
	struct hlist_node *tmp;
	unsigned int i;

	seq_puts(seq, "id      flow        state type  filter\n");
	spin_lock_bh(&lif->rx_filters.lock);
	for (i = 0; i < IONIC_RX_FILTER_HLISTS; i++) {
		head = &lif->rx_filters.by_id[i];
		hlist_for_each_entry_safe(f, tmp, head, by_id) {
			switch (le16_to_cpu(f->cmd.match)) {
			case IONIC_RX_FILTER_MATCH_VLAN:
				seq_printf(seq, "0x%04x  0x%08x  0x%02x  vlan  0x%04x\n",
					   f->filter_id, f->flow_id, f->state,
					   le16_to_cpu(f->cmd.vlan.vlan));
				break;
			case IONIC_RX_FILTER_MATCH_MAC:
				seq_printf(seq, "0x%04x  0x%08x  0x%02x  mac   %pM\n",
					   f->filter_id, f->flow_id, f->state,
					   f->cmd.mac.addr);
				break;
			case IONIC_RX_FILTER_MATCH_MAC_VLAN:
				seq_printf(seq, "0x%04x  0x%08x  0x%02x  macvl 0x%04x %pM\n",
					   f->filter_id, f->flow_id, f->state,
					   le16_to_cpu(f->cmd.vlan.vlan),
					   f->cmd.mac.addr);
				break;
			case IONIC_RX_FILTER_STEER_PKTCLASS:
				seq_printf(seq, "0x%04x  0x%08x  0x%02x  rxstr 0x%llx\n",
					   f->filter_id, f->flow_id, f->state,
					   le64_to_cpu(f->cmd.pkt_class));
				break;
			}
		}
	}
	spin_unlock_bh(&lif->rx_filters.lock);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(lif_filters);

static int lif_n_txrx_alloc_show(struct seq_file *seq, void *v)
{
	struct ionic_lif *lif = seq->private;

	seq_printf(seq, "%llu\n", lif->n_txrx_alloc);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(lif_n_txrx_alloc);

void ionic_debugfs_add_lif(struct ionic_lif *lif)
{
	struct dentry *lif_dentry;

	lif_dentry = debugfs_create_dir(lif->name, lif->ionic->dentry);
	if (IS_ERR_OR_NULL(lif_dentry))
		return;
	lif->dentry = lif_dentry;

	debugfs_create_file("netdev", 0400, lif->dentry,
			    lif->netdev, &netdev_fops);
	debugfs_create_file("identity", 0400, lif->dentry,
			    lif->identity, &lif_identity_fops);
	debugfs_create_file("state", 0400, lif->dentry,
			    lif, &lif_state_fops);
	debugfs_create_file("filters", 0400, lif->dentry,
			    lif, &lif_filters_fops);
	debugfs_create_file("txrx_alloc", 0400, lif->dentry,
			    lif, &lif_n_txrx_alloc_fops);
}

void ionic_debugfs_del_lif(struct ionic_lif *lif)
{
	if (!lif->dentry)
		return;

	debugfs_remove_recursive(lif->dentry);
	lif->dentry = NULL;
}

void ionic_debugfs_del_qcq(struct ionic_qcq *qcq)
{
	debugfs_remove_recursive(qcq->dentry);
	qcq->dentry = NULL;
}

#endif
