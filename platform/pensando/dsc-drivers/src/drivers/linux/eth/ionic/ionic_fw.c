// SPDX-License-Identifier: GPL-2.0
/* Copyright(c) 2020 - 2022 Pensando Systems, Inc */

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>

#include "ionic.h"
#include "ionic_dev.h"
#include "ionic_lif.h"

/* The worst case wait for the install activity is about 25 minutes when
 * installing a new CPLD, which is very seldom.  Normal is about 30-35
 * seconds.  Since the driver can't tell if a CPLD update will happen we
 * set the timeout for the ugly case.
 */
#define IONIC_FW_INSTALL_TIMEOUT	(25 * 60)
#define IONIC_FW_ACTIVATE_TIMEOUT	30

/* Number of periodic log updates during fw file download */
#define IONIC_FW_INTERVAL_FRACTION	32

static void ionic_dev_cmd_firmware_download(struct ionic_dev *idev, u64 addr,
					    u32 offset, u32 length)
{
	union ionic_dev_cmd cmd = {
		.fw_download.opcode = IONIC_CMD_FW_DOWNLOAD,
		.fw_download.offset = cpu_to_le32(offset),
		.fw_download.addr = cpu_to_le64(addr),
		.fw_download.length = cpu_to_le32(length),
	};

	ionic_dev_cmd_go(idev, &cmd);
}

static void ionic_dev_cmd_firmware_install(struct ionic_dev *idev)
{
	union ionic_dev_cmd cmd = {
		.fw_control.opcode = IONIC_CMD_FW_CONTROL,
		.fw_control.oper = IONIC_FW_INSTALL_ASYNC
	};

	ionic_dev_cmd_go(idev, &cmd);
}

static void ionic_dev_cmd_firmware_install_status(struct ionic_dev *idev)
{
	union ionic_dev_cmd cmd = {
		.fw_control.opcode = IONIC_CMD_FW_CONTROL,
		.fw_control.oper = IONIC_FW_INSTALL_STATUS
	};

	ionic_dev_cmd_go(idev, &cmd);
}

static void ionic_dev_cmd_firmware_activate(struct ionic_dev *idev, u8 slot)
{
	union ionic_dev_cmd cmd = {
		.fw_control.opcode = IONIC_CMD_FW_CONTROL,
		.fw_control.oper = IONIC_FW_ACTIVATE_ASYNC,
		.fw_control.slot = slot
	};

	ionic_dev_cmd_go(idev, &cmd);
}

static void ionic_dev_cmd_firmware_activate_status(struct ionic_dev *idev)
{
	union ionic_dev_cmd cmd = {
		.fw_control.opcode = IONIC_CMD_FW_CONTROL,
		.fw_control.oper = IONIC_FW_ACTIVATE_STATUS,
	};

	ionic_dev_cmd_go(idev, &cmd);
}

int ionic_firmware_update(struct ionic_lif *lif, const struct firmware *fw)
{
	struct ionic_dev *idev = &lif->ionic->idev;
	struct net_device *netdev = lif->netdev;
	struct ionic *ionic = lif->ionic;
	union ionic_dev_cmd_comp comp;
	u32 buf_sz, copy_sz, offset;
	struct devlink *dl;
	int next_interval;
	int err = 0;
	u8 fw_slot;

	dl = priv_to_devlink(ionic);
	devlink_flash_update_status_notify(dl, "Preparing to flash", NULL, 0, 0);

	buf_sz = sizeof(idev->dev_cmd_regs->data);

	netdev_dbg(netdev,
		   "downloading firmware - size %d part_sz %d nparts %lu\n",
		   (int)fw->size, buf_sz, DIV_ROUND_UP(fw->size, buf_sz));

	devlink_flash_update_status_notify(dl, "Downloading", NULL, 0, fw->size);
	offset = 0;
	next_interval = fw->size / IONIC_FW_INTERVAL_FRACTION;
	while (offset < fw->size) {
		copy_sz = min_t(unsigned int, buf_sz, fw->size - offset);
		mutex_lock(&ionic->dev_cmd_lock);
		memcpy_toio(&idev->dev_cmd_regs->data, fw->data + offset, copy_sz);
		ionic_dev_cmd_firmware_download(idev,
						offsetof(union ionic_dev_cmd_regs, data),
						offset, copy_sz);
		err = ionic_dev_cmd_wait(ionic, DEVCMD_TIMEOUT);
		mutex_unlock(&ionic->dev_cmd_lock);
		if (err) {
			netdev_err(netdev,
				   "download failed offset 0x%x addr 0x%lx len 0x%x\n",
				   offset, offsetof(union ionic_dev_cmd_regs, data),
				   copy_sz);
			goto err_out;
		}
		offset += copy_sz;

		if (offset > next_interval) {
			devlink_flash_update_status_notify(dl, "Downloading",
							   NULL, offset, fw->size);
			next_interval = offset + (fw->size / IONIC_FW_INTERVAL_FRACTION);
		}
	}
	devlink_flash_update_status_notify(dl, "Downloading", NULL, 1, 1);

	netdev_info(netdev, "installing firmware\n");
	devlink_flash_update_status_notify(dl, "Installing", NULL, 0, 2);

	mutex_lock(&ionic->dev_cmd_lock);
	ionic_dev_cmd_firmware_install(idev);
	err = ionic_dev_cmd_wait(ionic, DEVCMD_TIMEOUT);
	ionic_dev_cmd_comp(idev, (union ionic_dev_cmd_comp *)&comp);
	fw_slot = comp.fw_control.slot;
	mutex_unlock(&ionic->dev_cmd_lock);
	if (err) {
		netdev_err(netdev, "failed to start firmware install\n");
		goto err_out;
	}

	devlink_flash_update_status_notify(dl, "Installing", NULL, 1, 2);
	mutex_lock(&ionic->dev_cmd_lock);
	ionic_dev_cmd_firmware_install_status(idev);
	err = ionic_dev_cmd_wait(ionic, IONIC_FW_INSTALL_TIMEOUT);
	mutex_unlock(&ionic->dev_cmd_lock);
	if (err) {
		netdev_err(netdev, "firmware install failed\n");
		goto err_out;
	}
	devlink_flash_update_status_notify(dl, "Installing", NULL, 2, 2);

	netdev_info(netdev, "selecting firmware\n");
	devlink_flash_update_status_notify(dl, "Selecting", NULL, 0, 2);

	mutex_lock(&ionic->dev_cmd_lock);
	ionic_dev_cmd_firmware_activate(idev, fw_slot);
	err = ionic_dev_cmd_wait(ionic, DEVCMD_TIMEOUT);
	mutex_unlock(&ionic->dev_cmd_lock);
	if (err) {
		netdev_err(netdev, "failed to start firmware select\n");
		goto err_out;
	}

	devlink_flash_update_status_notify(dl, "Selecting", NULL, 1, 2);
	mutex_lock(&ionic->dev_cmd_lock);
	ionic_dev_cmd_firmware_activate_status(idev);
	err = ionic_dev_cmd_wait(ionic, IONIC_FW_ACTIVATE_TIMEOUT);
	mutex_unlock(&ionic->dev_cmd_lock);
	if (err) {
		netdev_err(netdev, "firmware select failed\n");
		goto err_out;
	}
	devlink_flash_update_status_notify(dl, "Selecting", NULL, 2, 2);

	netdev_info(netdev, "Firmware update completed\n");

err_out:
	if (err)
		devlink_flash_update_status_notify(dl, "Flash failed", NULL, 0, 0);
	return err;
}

int ionic_firmware_fetch_and_update(struct ionic_lif *lif, const char *fw_name)
{
	const struct firmware *fw;
	struct devlink *dl;
	int err;

	netdev_info(lif->netdev, "Installing firmware %s\n", fw_name);

	dl = priv_to_devlink(lif->ionic);
	devlink_flash_update_begin_notify(dl);

	err = request_firmware(&fw, fw_name, lif->ionic->dev);
	if (err)
		goto err_out;

	err = ionic_firmware_update(lif, fw);

err_out:
	devlink_flash_update_end_notify(dl);
	release_firmware(fw);

	return err;
}

