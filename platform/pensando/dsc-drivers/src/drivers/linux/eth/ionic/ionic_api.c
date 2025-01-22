// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2017 - 2022 Pensando Systems, Inc. All rights reserved. */

#include <linux/kernel.h>

#include "ionic.h"
#include "ionic_bus.h"
#include "ionic_dev.h"
#include "ionic_lif.h"
#include "ionic_txrx.h"

void *ionic_get_handle_from_netdev(struct net_device *netdev,
				   const char *api_version,
				   enum ionic_api_prsn prsn)
{
	struct ionic_lif *lif;

	if (strcmp(api_version, IONIC_API_VERSION))
		return ERR_PTR(-EINVAL);

	lif = ionic_netdev_lif(netdev);
	if (!lif || !lif->nrdma_eqs)
		return ERR_PTR(-ENXIO);

	/* TODO: Rework if supporting more than one child */
	if (lif->child_lif_cfg.prsn != IONIC_PRSN_NONE &&
	    lif->child_lif_cfg.prsn != prsn)
		return ERR_PTR(-EBUSY);

	return lif;
}
EXPORT_SYMBOL_GPL(ionic_get_handle_from_netdev);

bool ionic_api_stay_registered(void *handle)
{
	/* TODO: Implement when eth driver reset is implemented */
	return false;
}
EXPORT_SYMBOL_GPL(ionic_api_stay_registered);

void ionic_api_request_reset(void *handle)
{
	struct ionic_lif *lif = handle;
	struct ionic *ionic;
	int err;

	union ionic_dev_cmd cmd = {
		.cmd.opcode = IONIC_CMD_RDMA_RESET_LIF,
		.cmd.lif_index = cpu_to_le16(lif->child_lif_cfg.index),
	};

	ionic = lif->ionic;

	mutex_lock(&ionic->dev_cmd_lock);

	ionic_dev_cmd_go(&ionic->idev, &cmd);
	err = ionic_dev_cmd_wait(ionic, devcmd_timeout);

	mutex_unlock(&ionic->dev_cmd_lock);

	if (err) {
		netdev_warn(lif->netdev, "request_reset: error %d\n", err);
		return;
	}

	if (lif->child_lif_cfg.priv &&
	    lif->child_lif_cfg.reset_cb)
		(*lif->child_lif_cfg.reset_cb)(lif->child_lif_cfg.priv);
}
EXPORT_SYMBOL_GPL(ionic_api_request_reset);

void *ionic_api_get_private(void *handle, enum ionic_api_prsn prsn)
{
	struct ionic_lif *lif = handle;

	if (lif->child_lif_cfg.prsn != prsn)
		return NULL;

	return lif->child_lif_cfg.priv;
}
EXPORT_SYMBOL_GPL(ionic_api_get_private);

int ionic_api_set_private(void *handle, void *priv,
			  void (*reset_cb)(void *priv),
			  enum ionic_api_prsn prsn)
{
	struct ionic_lif *lif = handle;
	struct ionic_lif_cfg *cfg = &lif->child_lif_cfg;

	if (priv && cfg->priv)
		return -EBUSY;

	cfg->priv = priv;
	cfg->prsn = prsn;
	cfg->reset_cb = reset_cb;

	return 0;
}
EXPORT_SYMBOL_GPL(ionic_api_set_private);

struct device *ionic_api_get_device(void *handle)
{
	struct ionic_lif *lif = handle;

	return lif->netdev->dev.parent;
}
EXPORT_SYMBOL_GPL(ionic_api_get_device);

const struct ionic_devinfo *ionic_api_get_devinfo(void *handle)
{
	struct ionic_lif *lif = handle;

	return &lif->ionic->idev.dev_info;
}
EXPORT_SYMBOL_GPL(ionic_api_get_devinfo);

struct dentry *ionic_api_get_debug_ctx(void *handle)
{
	struct ionic_lif *lif = handle;

	return lif->dentry;
}
EXPORT_SYMBOL_GPL(ionic_api_get_debug_ctx);

const union ionic_lif_identity *ionic_api_get_identity(void *handle,
						       int *lif_index)
{
	struct ionic_lif *lif = handle;

	if (lif_index)
		*lif_index = lif->child_lif_cfg.index;

	/* TODO: Do all LIFs have the same ident? */
	return &lif->ionic->ident.lif;
}
EXPORT_SYMBOL_GPL(ionic_api_get_identity);

int ionic_api_get_intr(void *handle, int *irq)
{
	struct ionic_lif *lif = handle;
	struct ionic_intr_info *intr_obj;
	int err;

	if (!lif->nrdma_eqs_avail)
		return -ENOSPC;

	intr_obj = kzalloc(sizeof(*intr_obj), GFP_KERNEL);
	if (!intr_obj)
		return -ENOSPC;

	err = ionic_intr_alloc(lif->ionic, intr_obj);
	if (err)
		goto done;

	err = ionic_bus_get_irq(lif->ionic, intr_obj->index);
	if (err < 0) {
		ionic_intr_free(lif->ionic, intr_obj->index);
		goto done;
	}

	lif->nrdma_eqs_avail--;

	*irq = err;
	err = intr_obj->index;
done:
	kfree(intr_obj);
	return err;
}
EXPORT_SYMBOL_GPL(ionic_api_get_intr);

void ionic_api_put_intr(void *handle, int intr)
{
	struct ionic_lif *lif = handle;

	ionic_intr_free(lif->ionic, intr);

	lif->nrdma_eqs_avail++;
}
EXPORT_SYMBOL_GPL(ionic_api_put_intr);

int ionic_api_get_cmb(void *handle, u32 *pgid, phys_addr_t *pgaddr, int order)
{
	struct ionic_lif *lif = handle;

	return ionic_get_cmb(lif, pgid, pgaddr, order);
}
EXPORT_SYMBOL_GPL(ionic_api_get_cmb);

void ionic_api_put_cmb(void *handle, u32 pgid, int order)
{
	struct ionic_lif *lif = handle;

	ionic_put_cmb(lif, pgid, order);
}
EXPORT_SYMBOL_GPL(ionic_api_put_cmb);

void ionic_api_kernel_dbpage(void *handle,
			     struct ionic_intr __iomem **intr_ctrl,
			     u32 *dbid, u64 __iomem **dbpage)
{
	struct ionic_lif *lif = handle;

	*intr_ctrl = lif->ionic->idev.intr_ctrl;

	*dbid = lif->kern_pid;
	*dbpage = lif->kern_dbpage;
}
EXPORT_SYMBOL_GPL(ionic_api_kernel_dbpage);

int ionic_api_get_dbid(void *handle, u32 *dbid, phys_addr_t *addr)
{
	struct ionic_lif *lif = handle;
	int id, dbpage_num;


	if (ionic_bus_dbpage_per_pid(lif->ionic)) {
		mutex_lock(&lif->dbid_inuse_lock);

		if (!lif->dbid_inuse) {
			mutex_unlock(&lif->dbid_inuse_lock);
			return -EINVAL;
		}

		id = find_first_zero_bit(lif->dbid_inuse, lif->dbid_count);
		if (id == lif->dbid_count) {
			mutex_unlock(&lif->dbid_inuse_lock);
			return -ENOMEM;
		}

		set_bit(id, lif->dbid_inuse);

		mutex_unlock(&lif->dbid_inuse_lock);

		dbpage_num = ionic_db_page_num(lif, id);
	} else {
		id = 0;
		dbpage_num = 0;
	}

	*dbid = id;
	*addr = ionic_bus_phys_dbpage(lif->ionic, dbpage_num);

	return 0;
}
EXPORT_SYMBOL_GPL(ionic_api_get_dbid);

void ionic_api_put_dbid(void *handle, int dbid)
{
	struct ionic_lif *lif = handle;

	if (ionic_bus_dbpage_per_pid(lif->ionic)) {
		mutex_lock(&lif->dbid_inuse_lock);
		if (lif->dbid_inuse)
			clear_bit(dbid, lif->dbid_inuse);
		mutex_unlock(&lif->dbid_inuse_lock);
	}
}
EXPORT_SYMBOL_GPL(ionic_api_put_dbid);

int ionic_api_adminq_post(void *handle, struct ionic_admin_ctx *ctx)
{
	struct ionic_lif *lif = handle;

	return ionic_adminq_post(lif, ctx);
}
EXPORT_SYMBOL_GPL(ionic_api_adminq_post);
