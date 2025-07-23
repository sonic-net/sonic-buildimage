// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2017 - 2022 Pensando Systems, Inc */

#include <linux/module.h>
#include <linux/netdevice.h>

#include "ionic.h"
#include "ionic_bus.h"
#include "ionic_lif.h"
#include "ionic_devlink.h"

#ifdef IONIC_DEVLINK
#ifdef HAVE_DEVLINK_UPDATE_PARAMS
static int ionic_dl_flash_update(struct devlink *dl,
				 struct devlink_flash_update_params *params,
				 struct netlink_ext_ack *extack)
{
	struct ionic *ionic = devlink_priv(dl);

#ifdef HAVE_DEVLINK_PREFETCH_FW
	return ionic_firmware_update(ionic->lif, params->fw);
#else
	return ionic_firmware_fetch_and_update(ionic->lif, params->file_name);
#endif
}
#else
static int ionic_dl_flash_update(struct devlink *dl,
				 const char *fwname,
				 const char *component,
				 struct netlink_ext_ack *extack)
{
	struct ionic *ionic = devlink_priv(dl);

	if (component)
		return -EOPNOTSUPP;

	return ionic_firmware_fetch_and_update(ionic->lif, fwname);
}
#endif /* HAVE_DEVLINK_UPDATE_PARAMS */

static int ionic_dl_info_get(struct devlink *dl, struct devlink_info_req *req,
			     struct netlink_ext_ack *extack)
{
	struct ionic *ionic = devlink_priv(dl);
	struct ionic_dev *idev = &ionic->idev;
	char buf[16];
	int err = 0;
	u32 val;

	err = devlink_info_driver_name_put(req, IONIC_DRV_NAME);
	if (err)
		return err;

	err = devlink_info_version_running_put(req,
					       DEVLINK_INFO_VERSION_GENERIC_FW,
					       idev->dev_info.fw_version);
	if (err)
		return err;

	val = ioread32(&idev->dev_info_regs->fw_heartbeat);
	snprintf(buf, sizeof(buf), "0x%x", val);
	err = devlink_info_version_running_put(req, "fw.heartbeat", buf);
	if (err)
		return err;

	val = ioread8(&idev->dev_info_regs->fw_status);
	snprintf(buf, sizeof(buf), "0x%x", val);
	err = devlink_info_version_running_put(req, "fw.status", buf);
	if (err)
		return err;

	snprintf(buf, sizeof(buf), "0x%x", idev->dev_info.asic_type);
	err = devlink_info_version_fixed_put(req,
					     DEVLINK_INFO_VERSION_GENERIC_ASIC_ID,
					     buf);
	if (err)
		return err;

	snprintf(buf, sizeof(buf), "0x%x", idev->dev_info.asic_rev);
	err = devlink_info_version_fixed_put(req,
					     DEVLINK_INFO_VERSION_GENERIC_ASIC_REV,
					     buf);
	if (err)
		return err;

	err = devlink_info_serial_number_put(req, idev->dev_info.serial_num);

	return err;
}

static const struct devlink_ops ionic_dl_ops = {
	.info_get	= ionic_dl_info_get,
	.flash_update	= ionic_dl_flash_update,
};

struct ionic *ionic_devlink_alloc(struct device *dev)
{
	struct devlink *dl;

	dl = devlink_alloc(&ionic_dl_ops, sizeof(struct ionic), dev);
	if (!dl)
		return NULL;

	return devlink_priv(dl);
}

void ionic_devlink_free(struct ionic *ionic)
{
	struct devlink *dl = priv_to_devlink(ionic);

	devlink_free(dl);
}

int ionic_devlink_register(struct ionic *ionic)
{
	struct devlink *dl = priv_to_devlink(ionic);
	int err;

#ifdef HAVE_VOID_DEVLINK_REGISTER
	err = devlink_port_register(dl, &ionic->dl_port, 0);
	if (err) {
		dev_err(ionic->dev, "devlink_port_register failed: %d\n", err);
		devlink_unregister(dl);
		return err;
	}

	SET_NETDEV_DEVLINK_PORT(ionic->lif->netdev, &ionic->dl_port);
	devlink_register(dl);
#else
	err = devlink_register(dl, ionic->dev);
	if (err) {
		dev_warn(ionic->dev, "devlink_register failed: %d\n", err);
		return err;
	}

	err = devlink_port_register(dl, &ionic->dl_port, 0);
	if (err) {
		dev_err(ionic->dev, "devlink_port_register failed: %d\n", err);
		devlink_unregister(dl);
		return err;
	}

	devlink_port_type_eth_set(&ionic->dl_port, ionic->lif->netdev);
#endif
	return 0;
}

void ionic_devlink_unregister(struct ionic *ionic)
{
	struct devlink *dl = priv_to_devlink(ionic);

	devlink_port_unregister(&ionic->dl_port);
	devlink_unregister(dl);
}
#endif /* IONIC_DEVLINK */
