/*
 * Copyright (C) 2024 Ciena Corporation
 * Author: Jonas Chianu <jchianu@ciena.com>
 *
 * This driver is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, version 2 of the License.
 *
 * This code is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/reset-controller.h>
#include <linux/ipmi.h>

#include "reset-fpga.h"
#include "reset-sysfs.h"

#define IPMI_TIMEOUT			(5 * HZ)
#define OBMC_MSG_LEN 3

/*
 * Definition lifted from:
 * https://github.com/openbmc/phosphor-host-ipmid/blob/master/include/ipmid/api-types.hpp
 */
enum obmc_ipmi_netfn {
	netFnApp = 0x06,
};

/*
 * Definitions lifted from:
 * https://bitbucket.ciena.com/projects/CEDIAG/repos/openbmc/browse/meta-ciena/meta-common/recipes-phosphor/ipmi/phosphor-ipmi-host/0001-Add-ipmid-raw-application-6-commands-for-Ciena.patch
 */
enum obmc_ipmi_cmdtype {
	cmdCienaType = 0x90,
};

enum obmc_ciena_cmds {
	CIENA_CMD_BMC_UPGD = 1,
};

/*
 * Definitions and hard-coded constants lifted from:
 * https://bitbucket.ciena.com/projects/CEDIAG/repos/openbmc/browse/meta-ciena/meta-common/recipes-ciena/obmc-installer/ciena-obmc-installer/obmc_hostipmi.sh#83
 */
enum obmc_ciena_opcodes {
	ciena_op_reset_request = 6,
};

struct ipmi_data {
	struct completion       read_complete;
	struct ipmi_addr        address;
	struct ipmi_user       *user;
	int                     interface;
	struct kernel_ipmi_msg  tx_message;
	long                    tx_msgid;
	void                   *rx_msg_data;
	unsigned short          rx_msg_len;
	unsigned char           rx_result;
	int                     rx_recv_type;
	struct ipmi_user_hndl   ipmi_hndlrs;
};

struct ciena_ipmi_reset_data {
	struct mutex                  lock;
	int                           ipmi_count;
	bool                          use_raw_value;
	uint32_t                      max_names;
	const char                  **names;
	struct reset_controller_dev   rcdev;
	struct platform_device       *pdev;
	struct ipmi_data              ipmi;
	unsigned char                 ipmi_tx_data[OBMC_MSG_LEN];
};

struct ciena_ipmi_reset_data *data = NULL;

static int ipmi_send_message(struct ipmi_data *ipmi, unsigned char cmd,
			     unsigned char *tx_data, unsigned short tx_len,
			     unsigned char *rx_data, unsigned short rx_len,
			     struct reset_controller_dev *rcdev)
{
	int err;

	ipmi->tx_message.cmd      = cmd;
	ipmi->tx_message.data     = tx_data;
	ipmi->tx_message.data_len = tx_len;
	ipmi->rx_msg_data         = rx_data;
	ipmi->rx_msg_len          = rx_len;

	err = ipmi_validate_addr(&ipmi->address, sizeof(ipmi->address));
	if (err) {
		dev_err(rcdev->dev, "validate_addr=%x\n", err);
		return err;
	}

	ipmi->tx_msgid++;
	err = ipmi_request_settime(ipmi->user, &ipmi->address, ipmi->tx_msgid,
				   &ipmi->tx_message, ipmi, 0, -1, 0);
	if (err) {
		dev_err(rcdev->dev, "request_settime=%x\n", err);
		return err;
	}

	err = wait_for_completion_timeout(&ipmi->read_complete, IPMI_TIMEOUT);
	if (!err) {
		err = -ETIMEDOUT;
		dev_err(rcdev->dev, "request_timeout=%x\n", err);
		return err;
	}

	return 0;
}

static int ciena_ipmi_reset_assert(struct reset_controller_dev *rcdev,
				   unsigned long id)
{
	int status;

	data = container_of(rcdev, struct ciena_ipmi_reset_data, rcdev);

	dev_info(rcdev->dev, "assert reset request %lu\n", id);

	mutex_lock(&data->lock);

	data->ipmi_tx_data[0] = CIENA_CMD_BMC_UPGD;
	data->ipmi_tx_data[1] = ciena_op_reset_request;
	data->ipmi_tx_data[2] = id;
	status = ipmi_send_message(&data->ipmi, cmdCienaType,
				   data->ipmi_tx_data, sizeof(data->ipmi_tx_data), NULL, 0, rcdev);
	if ((status == 0) && (data->ipmi.rx_result != 0)) status = -EIO;

	mutex_unlock(&data->lock);

	return status;
}

static int ciena_ipmi_reset_deassert(struct reset_controller_dev *rcdev,
				     unsigned long id)
{
	data = container_of(rcdev, struct ciena_ipmi_reset_data, rcdev);

	dev_info(rcdev->dev, "nothing to deassert\n");

	return 0;
}

static int ciena_ipmi_reset_reset(struct reset_controller_dev *rcdev,
				  unsigned long id)
{
	ciena_ipmi_reset_assert(rcdev, id);

	udelay(500);

	ciena_ipmi_reset_deassert(rcdev, id);

	return 0;
}

static int ciena_ipmi_reset_status(struct reset_controller_dev *rcdev,
				   unsigned long id)
{
	return 0;
}

static struct reset_control_ops ciena_ipmi_reset_ops = {
	.assert		= ciena_ipmi_reset_assert,
	.deassert	= ciena_ipmi_reset_deassert,
	.status		= ciena_ipmi_reset_status,
	.reset		= ciena_ipmi_reset_reset
};

static int ciena_ipmi_reset_parse_pdata(struct platform_device *pdev)
{
	struct ciena_fpga_reset_pdata *pdata = pdev->dev.platform_data;

	if (pdata) {
		/* use the platform data if it is provided */
		data->use_raw_value = (0 != pdata->use_raw_value);
		data->max_names     = pdata->num_names;
		data->names         = pdata->reset_names;
		return 0;
	}

	return -EINVAL;
}

/* Dispatch IPMI messages to callers */
static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data)
{
	unsigned short rx_len;
	struct ipmi_data *ipmi = user_msg_data;

	if (msg->msgid != ipmi->tx_msgid) {
		dev_err(data->rcdev.dev, "Mismatch between received msgid "
			"(%02x) and transmitted msgid (%02x)!\n",
			(int)msg->msgid,
			(int)ipmi->tx_msgid);
		ipmi_free_recv_msg(msg);
		return;
	}

	ipmi->rx_recv_type = msg->recv_type;
	if (msg->msg.data_len > 0) ipmi->rx_result = msg->msg.data[0];
	else ipmi->rx_result = IPMI_UNKNOWN_ERR_COMPLETION_CODE;

	if (msg->msg.data_len > 1) {
		rx_len = msg->msg.data_len - 1;
		if (ipmi->rx_msg_len < rx_len) rx_len = ipmi->rx_msg_len;
		ipmi->rx_msg_len = rx_len;
		memcpy(ipmi->rx_msg_data, msg->msg.data + 1, ipmi->rx_msg_len);
	} else
		ipmi->rx_msg_len = 0;

	ipmi_free_recv_msg(msg);
	complete(&ipmi->read_complete);
}

static int init_ipmi_data(struct ipmi_data *ipmi, int iface,
			  struct device *dev)
{
	int err;

	init_completion(&ipmi->read_complete);

	/* Initialize IPMI address */
	ipmi->address.addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE;
	ipmi->address.channel = IPMI_BMC_CHANNEL;
	ipmi->address.data[0] = 0;
	ipmi->interface = iface;

	/* Initialize message buffers */
	ipmi->tx_msgid = 0;
	ipmi->tx_message.netfn = netFnApp;

	ipmi->ipmi_hndlrs.ipmi_recv_hndl = ipmi_msg_handler;

	/* Create IPMI messaging interface user */
	err = ipmi_create_user(ipmi->interface, &ipmi->ipmi_hndlrs,
			       ipmi, &ipmi->user);
	if (err < 0) {
		dev_err(dev, "Unable to register user with IPMI "
			"interface %d\n", ipmi->interface);
		return -EACCES;
	}

	return 0;
}

static int ciena_ipmi_reset_probe(struct platform_device *pdev)
{
	int rc;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data) return -ENOMEM;

	mutex_init(&data->lock);

	rc = ciena_ipmi_reset_parse_pdata(pdev);
	if (rc) return rc;

	data->rcdev.owner = THIS_MODULE;
	data->rcdev.ops = &ciena_ipmi_reset_ops;
	data->rcdev.dev = &pdev->dev;
	data->rcdev.nr_resets = data->ipmi_count;

	rc = reset_controller_register(&data->rcdev);
	if (rc) {
		dev_err(&pdev->dev, "%s: registration failed (%d)\n",
			__func__, rc);
		return rc;
	} else dev_set_drvdata(&pdev->dev, data);

	/* Set up IPMI interface */
	rc = init_ipmi_data(&data->ipmi, 0, data->rcdev.dev);
	if (rc) {
		reset_controller_unregister(&data->rcdev);
		dev_err(&pdev->dev, "%s: ipmi interface set up failed (%d)\n",
			__func__, rc);
		return rc;
	}

	dev_info(&pdev->dev, "%s created successfully\n", RESET_IPMI_DRIVER_NAME);
	return 0;
}

static void ciena_ipmi_reset_remove(struct platform_device *pdev)
{
	data = platform_get_drvdata(pdev);
	reset_controller_unregister(&data->rcdev);
}

static struct platform_driver ciena_ipmi_reset_driver = {
	.probe	  = ciena_ipmi_reset_probe,
	.remove	  = ciena_ipmi_reset_remove,
	.driver   = {
		.name		= RESET_IPMI_DRIVER_NAME,
		.owner		= THIS_MODULE,
	},
};
module_platform_driver(ciena_ipmi_reset_driver);

MODULE_AUTHOR("Jonas Chianu");
MODULE_DESCRIPTION("Ciena IPMI Reset Controller Driver");
MODULE_LICENSE("GPL");
