/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2017 - 2022 Pensando Systems, Inc */

#ifndef _IONIC_DEVLINK_H_
#define _IONIC_DEVLINK_H_

#include <linux/firmware.h>

#if IS_ENABLED(CONFIG_NET_DEVLINK)
#include <net/devlink.h>
#endif

int ionic_firmware_update(struct ionic_lif *lif, const struct firmware *fw);
int ionic_firmware_fetch_and_update(struct ionic_lif *lif, const char *fw_name);

/* make sure we've got a new-enough devlink support to use dev info */
#ifdef DEVLINK_INFO_VERSION_GENERIC_BOARD_ID

#define IONIC_DEVLINK

struct ionic *ionic_devlink_alloc(struct device *dev);
void ionic_devlink_free(struct ionic *ionic);
int ionic_devlink_register(struct ionic *ionic);
void ionic_devlink_unregister(struct ionic *ionic);
#else
#define ionic_devlink_alloc(dev)  devm_kzalloc(dev, sizeof(struct ionic), GFP_KERNEL)
#define ionic_devlink_free(i)     devm_kfree(i->dev, i)

#define ionic_devlink_register(x)    0
#define ionic_devlink_unregister(x)
#endif

#if !IS_ENABLED(CONFIG_NET_DEVLINK)
#define priv_to_devlink(i)  0
#define devlink_flash_update_begin_notify(d)
#define devlink_flash_update_end_notify(d)
#define devlink_flash_update_status_notify(d, s, c, n, t)
#endif /* CONFIG_NET_DEVLINK */

#endif /* _IONIC_DEVLINK_H_ */
