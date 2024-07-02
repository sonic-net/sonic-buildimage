/*
 * Copyright (C)  Brandon Chuang <brandon_chuang@accton.com.tw>
 *
 * Based on:
 *	pca954x.c from Kumar Gala <galak@kernel.crashing.org>
 * Copyright (C) 2006
 *
 * Based on:
 *	pca954x.c from Ken Harrenstien
 * Copyright (C) 2004 Google, Inc. (Ken Harrenstien)
 *
 * Based on:
 *	i2c-virtual_cb.c from Brian Kuschak <bkuschak@yahoo.com>
 * and
 *	pca9540.c from Jean Delvare <khali@linux-fr.org>.
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/version.h>
#include <linux/stat.h>
#include <linux/sysfs.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/ipmi.h>
#include <linux/ipmi_smi.h>
#include <linux/platform_device.h>

#define DRVNAME "as7535_28xb_thermal"
#define ACCTON_IPMI_NETFN 0x34
#define IPMI_THERMAL_READ_CMD 0x12
#define THERMAL_COUNT    6
#define THERMAL_DATA_LEN 3
#define THERMAL_DATA_COUNT (THERMAL_COUNT * THERMAL_DATA_LEN)

#define IPMI_TIMEOUT (5 * HZ)
#define IPMI_ERR_RETRY_TIMES 1

static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data);
static ssize_t show_temp(struct device *dev, struct device_attribute *attr,
	char *buf);
static ssize_t show_threshold(struct device *dev, struct device_attribute *da,
	char *buf);
static int as7535_28xb_thermal_probe(struct platform_device *pdev);
static int as7535_28xb_thermal_remove(struct platform_device *pdev);

enum temp_data_index {
	TEMP_ADDR,
	TEMP_FAULT,
	TEMP_INPUT,
	TEMP_DATA_COUNT
};

struct ipmi_data {
	struct completion read_complete;
	struct ipmi_addr address;
	struct ipmi_user * user;
	int interface;

	struct kernel_ipmi_msg tx_message;
	long tx_msgid;

	void *rx_msg_data;
	unsigned short rx_msg_len;
	unsigned char rx_result;
	int rx_recv_type;

	struct ipmi_user_hndl ipmi_hndlrs;
};

struct as7535_28xb_thermal_data {
	struct platform_device *pdev;
	struct mutex update_lock;
	char valid;		   /* != 0 if registers are valid */
	unsigned long last_updated;	/* In jiffies */
	char   ipmi_resp[THERMAL_DATA_COUNT]; /* 3 bytes for each thermal */
	struct ipmi_data ipmi;
	unsigned char ipmi_tx_data[2];  /* 0: thermal id, 1: temp */
};

static s8 temp_max_alarm[THERMAL_COUNT] = { 79, 89, 102, 92, 85, 85 };
static s8 temp_max[THERMAL_COUNT] = { 74, 84, 97, 87, 80, 80 };
static s8 temp_min[THERMAL_COUNT] = { -45, -45, -45, -45, -45, -45 };
static s8 temp_min_alarm[THERMAL_COUNT] = { -50, -50, -50, -50, -50, -50 };

struct as7535_28xb_thermal_data *data = NULL;

static struct platform_driver as7535_28xb_thermal_driver = {
	.probe = as7535_28xb_thermal_probe,
	.remove = as7535_28xb_thermal_remove,
	.driver = {
		.name = DRVNAME,
		.owner = THIS_MODULE,
	},
};

enum as7535_28xb_thermal_sysfs_attrs {
	TEMP1_INPUT, // 0x4B
	TEMP2_INPUT, // 0x4C tmp43 local
	TEMP3_INPUT, // 0x4C tmp43 remote
	TEMP4_INPUT, // 0x4D lm75
	TEMP5_INPUT, // 0x4E lm75
	TEMP6_INPUT, // 0x4F lm75
	TEMP1_MAX_ALARM,
	TEMP2_MAX_ALARM,
	TEMP3_MAX_ALARM,
	TEMP4_MAX_ALARM,
	TEMP5_MAX_ALARM,
	TEMP6_MAX_ALARM,
	TEMP1_MAX,
	TEMP2_MAX,
	TEMP3_MAX,
	TEMP4_MAX,
	TEMP5_MAX,
	TEMP6_MAX,
	TEMP1_MIN,
	TEMP2_MIN,
	TEMP3_MIN,
	TEMP4_MIN,
	TEMP5_MIN,
	TEMP6_MIN,
	TEMP1_MIN_ALARM,
	TEMP2_MIN_ALARM,
	TEMP3_MIN_ALARM,
	TEMP4_MIN_ALARM,
	TEMP5_MIN_ALARM,
	TEMP6_MIN_ALARM,
};

// Read only temp_input
#define DECLARE_THERMAL_SENSOR_DEVICE_ATTR(index) \
	static SENSOR_DEVICE_ATTR(temp##index##_input, S_IRUGO, show_temp, \
					NULL, TEMP##index##_INPUT); \
	static SENSOR_DEVICE_ATTR(temp##index##_max_alarm, S_IRUGO, show_threshold,\
					NULL, TEMP##index##_MAX_ALARM); \
	static SENSOR_DEVICE_ATTR(temp##index##_max, S_IRUGO, show_threshold,\
					NULL, TEMP##index##_MAX); \
	static SENSOR_DEVICE_ATTR(temp##index##_min, S_IRUGO, show_threshold,\
					NULL, TEMP##index##_MIN); \
	static SENSOR_DEVICE_ATTR(temp##index##_min_alarm, S_IRUGO, show_threshold,\
					NULL, TEMP##index##_MIN_ALARM)

#define DECLARE_THERMAL_ATTR(index) \
	&sensor_dev_attr_temp##index##_input.dev_attr.attr, \
	&sensor_dev_attr_temp##index##_max_alarm.dev_attr.attr, \
	&sensor_dev_attr_temp##index##_max.dev_attr.attr, \
	&sensor_dev_attr_temp##index##_min.dev_attr.attr, \
	&sensor_dev_attr_temp##index##_min_alarm.dev_attr.attr

DECLARE_THERMAL_SENSOR_DEVICE_ATTR(1);
DECLARE_THERMAL_SENSOR_DEVICE_ATTR(2);
DECLARE_THERMAL_SENSOR_DEVICE_ATTR(3);
DECLARE_THERMAL_SENSOR_DEVICE_ATTR(4);
DECLARE_THERMAL_SENSOR_DEVICE_ATTR(5);
DECLARE_THERMAL_SENSOR_DEVICE_ATTR(6);

static struct attribute *as7535_28xb_thermal_attributes[] = {
	DECLARE_THERMAL_ATTR(1),
	DECLARE_THERMAL_ATTR(2),
	DECLARE_THERMAL_ATTR(3),
	DECLARE_THERMAL_ATTR(4),
	DECLARE_THERMAL_ATTR(5),
	DECLARE_THERMAL_ATTR(6),
	NULL
};

static const struct attribute_group as7535_28xb_thermal_group = {
	.attrs = as7535_28xb_thermal_attributes,
};

/* Functions to talk to the IPMI layer */

/* Initialize IPMI address, message buffers and user data */
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
	ipmi->tx_message.netfn = ACCTON_IPMI_NETFN;

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

/* Send an IPMI command */
static int _ipmi_send_message(struct ipmi_data *ipmi, unsigned char cmd,
								unsigned char *tx_data, unsigned short tx_len,
								unsigned char *rx_data, unsigned short rx_len)
{
	int err;

	ipmi->tx_message.cmd = cmd;
	ipmi->tx_message.data = tx_data;
	ipmi->tx_message.data_len = tx_len;
	ipmi->rx_msg_data = rx_data;
	ipmi->rx_msg_len = rx_len;

	err = ipmi_validate_addr(&ipmi->address, sizeof(ipmi->address));
	if (err)
		goto addr_err;

	ipmi->tx_msgid++;
	err = ipmi_request_settime(ipmi->user, &ipmi->address, ipmi->tx_msgid,
				   &ipmi->tx_message, ipmi, 0, 0, 0);
	if (err)
		goto ipmi_req_err;

	err = wait_for_completion_timeout(&ipmi->read_complete, IPMI_TIMEOUT);
	if (!err)
		goto ipmi_timeout_err;

	return 0;

ipmi_timeout_err:
	err = -ETIMEDOUT;
	dev_err(&data->pdev->dev, "request_timeout=%x\n", err);
	return err;
ipmi_req_err:
	dev_err(&data->pdev->dev, "request_settime=%x\n", err);
	return err;
addr_err:
	dev_err(&data->pdev->dev, "validate_addr=%x\n", err);
	return err;
}

/* Send an IPMI command with retry */
static int ipmi_send_message(struct ipmi_data *ipmi, unsigned char cmd,
								unsigned char *tx_data, unsigned short tx_len,
								unsigned char *rx_data, unsigned short rx_len)
{
	int status = 0, retry = 0;

	for (retry = 0; retry <= IPMI_ERR_RETRY_TIMES; retry++) {
		status = _ipmi_send_message(ipmi, cmd, tx_data, tx_len, rx_data, rx_len);
		if (unlikely(status != 0)) {
			dev_err(&data->pdev->dev, "ipmi cmd(%x) err status(%d)\r\n",
										cmd, status);
			continue;
		}

		if (unlikely(ipmi->rx_result != 0)) {
			dev_err(&data->pdev->dev, "ipmi cmd(%x) err result(%d)\r\n",
										cmd, ipmi->rx_result);
			continue;
		}

		break;
	}

	return status;
}

/* Dispatch IPMI messages to callers */
static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data)
{
	unsigned short rx_len;
	struct ipmi_data *ipmi = user_msg_data;

	if (msg->msgid != ipmi->tx_msgid) {
		dev_err(&data->pdev->dev, "Mismatch between received msgid "
			"(%02x) and transmitted msgid (%02x)!\n",
			(int)msg->msgid,
			(int)ipmi->tx_msgid);
		ipmi_free_recv_msg(msg);
		return;
	}

	ipmi->rx_recv_type = msg->recv_type;
	if (msg->msg.data_len > 0)
		ipmi->rx_result = msg->msg.data[0];
	else
		ipmi->rx_result = IPMI_UNKNOWN_ERR_COMPLETION_CODE;

	if (msg->msg.data_len > 1) {
		rx_len = msg->msg.data_len - 1;
		if (ipmi->rx_msg_len < rx_len)
			rx_len = ipmi->rx_msg_len;
		ipmi->rx_msg_len = rx_len;
		memcpy(ipmi->rx_msg_data, msg->msg.data + 1, ipmi->rx_msg_len);
	} else
		ipmi->rx_msg_len = 0;

	ipmi_free_recv_msg(msg);
	complete(&ipmi->read_complete);
}

static ssize_t show_threshold(struct device *dev, struct device_attribute *da,
							char *buf)
{
	int status = 0;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

	mutex_lock(&data->update_lock);

	switch (attr->index) {
	case TEMP1_MAX_ALARM ... TEMP6_MAX_ALARM:
		status = (int)temp_max_alarm[attr->index - TEMP1_MAX_ALARM];
		break;
	case TEMP1_MAX ... TEMP6_MAX:
		status = (int)temp_max[attr->index - TEMP1_MAX];
		break;
	case TEMP1_MIN ... TEMP6_MIN:
		status = (int)temp_min[attr->index - TEMP1_MIN];
		break;
	case TEMP1_MIN_ALARM ... TEMP6_MIN_ALARM:
		status = (int)temp_min_alarm[attr->index - TEMP1_MIN_ALARM];
		break;
	default:
		status = -EINVAL;
		goto exit;
	}

	mutex_unlock(&data->update_lock);
	return sprintf(buf, "%d\n", status * 1000);

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static ssize_t show_temp(struct device *dev, struct device_attribute *da,
							char *buf)
{
	int status = 0;
	int index  = 0;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

	mutex_lock(&data->update_lock);

	if (time_after(jiffies, data->last_updated + HZ * 5) || !data->valid) {
		data->valid = 0;

		status = ipmi_send_message(&data->ipmi, IPMI_THERMAL_READ_CMD, NULL, 0,
									data->ipmi_resp, sizeof(data->ipmi_resp));
		if (unlikely(status != 0))
			goto exit;

		if (unlikely(data->ipmi.rx_result != 0)) {
			status = -EIO;
			goto exit;
		}

		data->last_updated = jiffies;
		data->valid = 1;
	}

	/* Get temp fault status */
	index = attr->index * TEMP_DATA_COUNT + TEMP_FAULT;
	if (unlikely(data->ipmi_resp[index] == 0)) {
		status = -EIO;
		goto exit;
	}

	/* Get temperature in degree celsius */
	index = attr->index * TEMP_DATA_COUNT + TEMP_INPUT;
	status = ((s8)data->ipmi_resp[index]) * 1000;

	mutex_unlock(&data->update_lock);
	return sprintf(buf, "%d\n", status);

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static int as7535_28xb_thermal_probe(struct platform_device *pdev)
{
	int status = -1;

	/* Register sysfs hooks */
	status = sysfs_create_group(&pdev->dev.kobj, &as7535_28xb_thermal_group);
	if (status)
		return status;

	dev_info(&pdev->dev, "device created\n");

	return 0;
}

static int as7535_28xb_thermal_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &as7535_28xb_thermal_group);
	return 0;
}

static int __init as7535_28xb_thermal_init(void)
{
	int ret;

	data = kzalloc(sizeof(struct as7535_28xb_thermal_data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto alloc_err;
	}

	mutex_init(&data->update_lock);
	data->valid = 0;

	ret = platform_driver_register(&as7535_28xb_thermal_driver);
	if (ret < 0)
		goto dri_reg_err;

	data->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
	if (IS_ERR(data->pdev)) {
		ret = PTR_ERR(data->pdev);
		goto dev_reg_err;
	}

	/* Set up IPMI interface */
	ret = init_ipmi_data(&data->ipmi, 0, &data->pdev->dev);
	if (ret)
		goto ipmi_err;

	return 0;

ipmi_err:
	platform_device_unregister(data->pdev);
dev_reg_err:
	platform_driver_unregister(&as7535_28xb_thermal_driver);
dri_reg_err:
	kfree(data);
alloc_err:
	return ret;
}

static void __exit as7535_28xb_thermal_exit(void)
{
	ipmi_destroy_user(data->ipmi.user);
	platform_device_unregister(data->pdev);
	platform_driver_unregister(&as7535_28xb_thermal_driver);
	kfree(data);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as7535_28xb_thermal driver");
MODULE_LICENSE("GPL");

module_init(as7535_28xb_thermal_init);
module_exit(as7535_28xb_thermal_exit);
