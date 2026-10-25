/*
 * Copyright (C)  Brandon Chuang <brandon_chuang@accton.com.tw>
 *
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

#define DRVNAME "as7535_28xb_fan"
#define ACCTON_IPMI_NETFN 0x34
#define IPMI_FAN_READ_CMD 0x14
#define IPMI_FAN_WRITE_CMD 0x15
#define IPMI_FAN_READ_RPM_CMD 0x20
#define IPMI_TIMEOUT (5 * HZ)
#define IPMI_ERR_RETRY_TIMES 1
#define IPMI_FAN_REG_READ_CMD 0x20
#define IPMI_FAN_REG_WRITE_CMD 0x21
#define MAX_FAN_SPEED_RPM 33000

static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data);
static ssize_t set_fan(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count);
static ssize_t show_fan(struct device *dev, struct device_attribute *attr,
			char *buf);
static ssize_t show_version(struct device *dev, struct device_attribute *da,
			char *buf);
static ssize_t show_dir(struct device *dev, struct device_attribute *da,
			char *buf);
static ssize_t show_threshold(struct device *dev, struct device_attribute *da,
			char *buf);
static int as7535_28xb_fan_probe(struct platform_device *pdev);
static int as7535_28xb_fan_remove(struct platform_device *pdev);

enum fan_id {
	FAN_1,
	FAN_2,
	FAN_3,
	FAN_4,
	FAN_5,
	FAN_6,
	NUM_OF_FAN,
	NUM_OF_FAN_MODULE = NUM_OF_FAN
};

enum fan_data_index {
	FAN_PRESENT,
	FAN_PWM,
	FAN_SPEED0,
	FAN_SPEED1,
	FAN_DATA_COUNT,

	FAN_TARGET_SPEED0 = 0,
	FAN_TARGET_SPEED1,
	FAN_SPEED_TOLERANCE,
	FAN_SPEED_DATA_COUNT
};

struct ipmi_data {
	struct completion read_complete;
	struct ipmi_addr address;
	struct ipmi_user *user;
	int interface;

	struct kernel_ipmi_msg tx_message;
	long tx_msgid;

	void *rx_msg_data;
	unsigned short rx_msg_len;
	unsigned char  rx_result;
	int rx_recv_type;

	struct ipmi_user_hndl ipmi_hndlrs;
};

struct as7535_28xb_fan_data {
	struct platform_device *pdev;
	struct mutex update_lock;
	char valid; /* != 0 if registers are valid */
	unsigned long last_updated;	/* In jiffies */
	/* 4 bytes for each fan, the last 2 bytes is fan dir */
	unsigned char ipmi_resp[NUM_OF_FAN * FAN_DATA_COUNT + 2];
	unsigned char ipmi_resp_cpld;
	unsigned char ipmi_resp_speed[NUM_OF_FAN * FAN_SPEED_DATA_COUNT];
	struct ipmi_data ipmi;
	unsigned char ipmi_tx_data[3];  /* 0: FAN id, 1: 0x02, 2: PWM */
};

struct as7535_28xb_fan_data *data = NULL;

static struct platform_driver as7535_28xb_fan_driver = {
	.probe = as7535_28xb_fan_probe,
	.remove = as7535_28xb_fan_remove,
	.driver = {
		.name = DRVNAME,
		.owner = THIS_MODULE,
	},
};

#define FAN_PRESENT_ATTR_ID(index) FAN##index##_PRESENT
#define FAN_PWM_ATTR_ID(index) FAN##index##_PWM
#define FAN_RPM_ATTR_ID(index) FAN##index##_INPUT
#define FAN_DIR_ATTR_ID(index) FAN##index##_DIR
#define FAN_FAULT_ATTR_ID(index) FAN##index##_FAULT
#define FAN_RPM_TARGET_ATTR_ID(index) FAN##index##_TARGET
#define FAN_RPM_TOLERANCE_ATTR_ID(index) FAN##index##_TOLERANCE

#define FAN_ATTR(fan_id) \
	FAN_PRESENT_ATTR_ID(fan_id), \
	FAN_PWM_ATTR_ID(fan_id), \
	FAN_RPM_ATTR_ID(fan_id), \
	FAN_DIR_ATTR_ID(fan_id), \
	FAN_FAULT_ATTR_ID(fan_id)

#define FAN_RPM_THRESHOLD_ATTR(fan_id) \
	FAN_RPM_TARGET_ATTR_ID(fan_id), \
	FAN_RPM_TOLERANCE_ATTR_ID(fan_id)

enum as7535_28xb_fan_sysfs_attrs {
	FAN_ATTR(1),
	FAN_ATTR(2),
	FAN_ATTR(3),
	FAN_ATTR(4),
	FAN_ATTR(5),
	FAN_ATTR(6),
	NUM_OF_FAN_ATTR,
	FAN_VERSION,
	FAN_MAX_RPM,
	NUM_OF_PER_FAN_ATTR = (NUM_OF_FAN_ATTR/NUM_OF_FAN),
	FAN_RPM_THRESHOLD_ATTR(1),
	FAN_RPM_THRESHOLD_ATTR(2),
	FAN_RPM_THRESHOLD_ATTR(3),
	FAN_RPM_THRESHOLD_ATTR(4),
	FAN_RPM_THRESHOLD_ATTR(5),
	FAN_RPM_THRESHOLD_ATTR(6)
};

/* fan attributes */
#define DECLARE_FAN_VER_SENSOR_DEVICE_ATTR() \
	static SENSOR_DEVICE_ATTR(version, S_IRUGO, show_version, NULL, FAN_VERSION)
#define DECLARE_FAN_VER_ATTR() \
	&sensor_dev_attr_version.dev_attr.attr

#define DECLARE_FAN_SENSOR_DEVICE_ATTR(index) \
	static SENSOR_DEVICE_ATTR(fan##index##_present, S_IRUGO, show_fan, NULL, \
								FAN##index##_PRESENT); \
	static SENSOR_DEVICE_ATTR(fan##index##_pwm, S_IWUSR | S_IRUGO, show_fan, \
								set_fan, FAN##index##_PWM); \
	static SENSOR_DEVICE_ATTR(fan##index##_input, S_IRUGO, show_fan, NULL, \
								FAN##index##_INPUT); \
	static SENSOR_DEVICE_ATTR(fan##index##_dir, S_IRUGO, show_dir, NULL, \
								FAN##index##_DIR); \
	static SENSOR_DEVICE_ATTR(fan##index##_fault, S_IRUGO, show_fan, NULL, \
								FAN##index##_FAULT); \
	static SENSOR_DEVICE_ATTR(fan##index##_target, S_IRUGO, show_threshold, \
								NULL, FAN##index##_TARGET); \
	static SENSOR_DEVICE_ATTR(fan##index##_tolerance, S_IRUGO, show_threshold,\
								NULL, FAN##index##_TOLERANCE)

static SENSOR_DEVICE_ATTR(fan_max_speed_rpm, S_IRUGO, show_fan, NULL, \
			FAN_MAX_RPM);
#define DECLARE_FAN_MAX_RPM_ATTR(index) \
			&sensor_dev_attr_fan_max_speed_rpm.dev_attr.attr

#define DECLARE_FAN_ATTR(index) \
	&sensor_dev_attr_fan##index##_present.dev_attr.attr, \
	&sensor_dev_attr_fan##index##_pwm.dev_attr.attr, \
	&sensor_dev_attr_fan##index##_input.dev_attr.attr, \
	&sensor_dev_attr_fan##index##_dir.dev_attr.attr, \
	&sensor_dev_attr_fan##index##_fault.dev_attr.attr, \
	&sensor_dev_attr_fan##index##_target.dev_attr.attr, \
	&sensor_dev_attr_fan##index##_tolerance.dev_attr.attr

DECLARE_FAN_SENSOR_DEVICE_ATTR(1);
DECLARE_FAN_SENSOR_DEVICE_ATTR(2);
DECLARE_FAN_SENSOR_DEVICE_ATTR(3);
DECLARE_FAN_SENSOR_DEVICE_ATTR(4);
DECLARE_FAN_SENSOR_DEVICE_ATTR(5);
DECLARE_FAN_SENSOR_DEVICE_ATTR(6);
DECLARE_FAN_VER_SENSOR_DEVICE_ATTR();

static struct attribute *as7535_28xb_fan_attributes[] = {
	/* fan attributes */
	DECLARE_FAN_ATTR(1),
	DECLARE_FAN_ATTR(2),
	DECLARE_FAN_ATTR(3),
	DECLARE_FAN_ATTR(4),
	DECLARE_FAN_ATTR(5),
	DECLARE_FAN_ATTR(6),
	DECLARE_FAN_VER_ATTR(),
	DECLARE_FAN_MAX_RPM_ATTR(),
	NULL
};

static const struct attribute_group as7535_28xb_fan_group = {
	.attrs = as7535_28xb_fan_attributes,
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

	ipmi->tx_message.cmd	  = cmd;
	ipmi->tx_message.data	 = tx_data;
	ipmi->tx_message.data_len = tx_len;
	ipmi->rx_msg_data		 = rx_data;
	ipmi->rx_msg_len		  = rx_len;

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
		status = _ipmi_send_message(ipmi,cmd, tx_data, tx_len, rx_data, rx_len);
		if (unlikely(status != 0)) {
			dev_err(&data->pdev->dev, "ipmi_send_message_%d err status(%d)\r\n",
										retry, status);
			continue;
		}

		if (unlikely(ipmi->rx_result != 0)) {
			dev_err(&data->pdev->dev, "ipmi_send_message_%d err result(%d)\r\n",
										retry, ipmi->rx_result);
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

static struct as7535_28xb_fan_data *as7535_28xb_fan_update_device(void)
{
	int status = 0;

	if (time_before(jiffies, data->last_updated + HZ * 5) && data->valid)
		return data;

	data->valid = 0;
	status = ipmi_send_message(&data->ipmi, IPMI_FAN_READ_CMD, NULL, 0,
								data->ipmi_resp, sizeof(data->ipmi_resp));
	if (unlikely(status != 0))
		goto exit;

	if (unlikely(data->ipmi.rx_result != 0)) {
		status = -EIO;
		goto exit;
	}

	data->ipmi_tx_data[0] = IPMI_FAN_READ_RPM_CMD;
	status = ipmi_send_message(&data->ipmi, IPMI_FAN_READ_CMD,
								data->ipmi_tx_data, 1,
								data->ipmi_resp_speed,
								sizeof(data->ipmi_resp_speed));

	data->last_updated = jiffies;
	data->valid = 1;

exit:
	return data;
}

static ssize_t show_fan(struct device *dev, struct device_attribute *da,
							char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	unsigned char fid = attr->index / NUM_OF_PER_FAN_ATTR;
	int value = 0;
	int index = 0;
	int present = 0;
	int error = 0;

	if (attr->index == FAN_MAX_RPM)
		return sprintf(buf, "%d\n", MAX_FAN_SPEED_RPM);

	mutex_lock(&data->update_lock);

	data = as7535_28xb_fan_update_device();
	if (!data->valid) {
		error = -EIO;
		goto exit;
	}

	index = fid * FAN_DATA_COUNT; /* base index */
	present = !!data->ipmi_resp[index + FAN_PRESENT];

	switch (attr->index) {
	case FAN1_PRESENT:
	case FAN2_PRESENT:
	case FAN3_PRESENT:
	case FAN4_PRESENT:
	case FAN5_PRESENT:
	case FAN6_PRESENT:
		value = present;
		break;
	case FAN1_PWM:
	case FAN2_PWM:
	case FAN3_PWM:
	case FAN4_PWM:
	case FAN5_PWM:
	case FAN6_PWM:
		index = (fid % NUM_OF_FAN_MODULE) * FAN_DATA_COUNT;
		value = (data->ipmi_resp[index + FAN_PWM] + 1) * 625 / 100;
		break;
	case FAN1_INPUT:
	case FAN2_INPUT:
	case FAN3_INPUT:
	case FAN4_INPUT:
	case FAN5_INPUT:
	case FAN6_INPUT:
		value = (int)data->ipmi_resp[index + FAN_SPEED0] |
				(int)data->ipmi_resp[index + FAN_SPEED1] << 8;
		break;
	case FAN1_FAULT:
	case FAN2_FAULT:
	case FAN3_FAULT:
	case FAN4_FAULT:
	case FAN5_FAULT:
	case FAN6_FAULT:
		value = (int)data->ipmi_resp[index + FAN_SPEED0] |
				(int)data->ipmi_resp[index + FAN_SPEED1] << 8;
		value = !value;
		break;
	default:
		error = -EINVAL;
		goto exit;
	}

	mutex_unlock(&data->update_lock);
	return sprintf(buf, "%d\n", value);

exit:
	mutex_unlock(&data->update_lock);
	return error;
}

static ssize_t set_fan(struct device *dev, struct device_attribute *da,
			const char *buf, size_t count)
{
	long pwm;
	int status;
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	unsigned char fid = attr->index / NUM_OF_PER_FAN_ATTR;

	status = kstrtol(buf, 10, &pwm);
	if (status)
		return status;

	pwm = (pwm * 100) / 625 - 1; /* Convert pwm to register value */

	mutex_lock(&data->update_lock);

	/* Send IPMI write command */
	data->ipmi_tx_data[0] = (fid % NUM_OF_FAN_MODULE) + 1;
	data->ipmi_tx_data[1] = 0x02;
	data->ipmi_tx_data[2] = pwm;
	status = ipmi_send_message(&data->ipmi, IPMI_FAN_WRITE_CMD,
								data->ipmi_tx_data, sizeof(data->ipmi_tx_data),
								NULL, 0);
	if (unlikely(status != 0))
		goto exit;

	if (unlikely(data->ipmi.rx_result != 0)) {
		status = -EIO;
		goto exit;
	}

	/* Update pwm to ipmi_resp buffer to prevent from the impact of lazy update */
	data->ipmi_resp[fid * FAN_DATA_COUNT + FAN_PWM] = pwm;
	status = count;

exit:
	mutex_unlock(&data->update_lock);
	return status;
}

static struct as7535_28xb_fan_data *as7535_28xb_fan_update_cpld_ver(void)
{
	int status = 0;

	data->valid = 0;
	data->ipmi_tx_data[0] = 0x66;
	status = ipmi_send_message(&data->ipmi, IPMI_FAN_REG_READ_CMD,
								data->ipmi_tx_data, 1,
								&data->ipmi_resp_cpld,
								sizeof(data->ipmi_resp_cpld));
	if (unlikely(status != 0))
		goto exit;

	if (unlikely(data->ipmi.rx_result != 0)) {
		status = -EIO;
		goto exit;
	}

	data->last_updated = jiffies;
	data->valid = 1;

exit:
	return data;
}

static ssize_t show_version(struct device *dev, struct device_attribute *da,
								char *buf)
{
	unsigned char value = 0;
	int error = 0;

	mutex_lock(&data->update_lock);

	data = as7535_28xb_fan_update_cpld_ver();
	if (!data->valid) {
		error = -EIO;
		goto exit;
	}

	value = data->ipmi_resp_cpld;
	mutex_unlock(&data->update_lock);
	return sprintf(buf, "%d\n", value);

exit:
	mutex_unlock(&data->update_lock);
	return error;
}

static ssize_t show_dir(struct device *dev, struct device_attribute *da,
							char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	unsigned char fid = (attr->index / NUM_OF_PER_FAN_ATTR);
	int value = 0;
	int index = 0;
	int present = 0;
	int error = 0;

	mutex_lock(&data->update_lock);

	data = as7535_28xb_fan_update_device();
	if (!data->valid) {
		error = -EIO;
		goto exit;
	}

	index = fid * FAN_DATA_COUNT; /* base index */
	present = !!data->ipmi_resp[index + FAN_PRESENT];

	value = data->ipmi_resp[NUM_OF_FAN * FAN_DATA_COUNT] |
			(data->ipmi_resp[NUM_OF_FAN * FAN_DATA_COUNT + 1] << 8);
	mutex_unlock(&data->update_lock);

	if (!present)
		return sprintf(buf, "0\n");
	else
		return sprintf(buf, "%s\n",
						(value & BIT(fid % NUM_OF_FAN_MODULE)) ? "B2F" : "F2B");

exit:
	mutex_unlock(&data->update_lock);
	return error;
}

static ssize_t show_threshold(struct device *dev, struct device_attribute *da,
			char *buf)
{
	struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
	int value = 0;
	int error = 0;

	mutex_lock(&data->update_lock);

	data = as7535_28xb_fan_update_device();
	if (!data->valid) {
		error = -EIO;
		goto exit;
	}

	switch (attr->index) {
	case FAN1_TARGET:
	case FAN2_TARGET:
	case FAN3_TARGET:
	case FAN4_TARGET:
	case FAN5_TARGET:
	case FAN6_TARGET:
		value = (int)data->ipmi_resp_speed[FAN_TARGET_SPEED0] |
				(int)data->ipmi_resp_speed[FAN_TARGET_SPEED1] << 8;
		break;
	case FAN1_TOLERANCE:
	case FAN2_TOLERANCE:
	case FAN3_TOLERANCE:
	case FAN4_TOLERANCE:
	case FAN5_TOLERANCE:
	case FAN6_TOLERANCE:
		value = (int)data->ipmi_resp_speed[FAN_SPEED_TOLERANCE];
		break;
	default:
		error = -EINVAL;
		goto exit;
	}

	mutex_unlock(&data->update_lock);
	return sprintf(buf, "%d\n", value);

exit:
	mutex_unlock(&data->update_lock);
	return error;
}

static int as7535_28xb_fan_probe(struct platform_device *pdev)
{
	int status = -1;

	/* Register sysfs hooks */
	status = sysfs_create_group(&pdev->dev.kobj, &as7535_28xb_fan_group);
	if (status)
		return status;

	dev_info(&pdev->dev, "device created\n");

	return 0;
}

static int as7535_28xb_fan_remove(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &as7535_28xb_fan_group);

	return 0;
}

static int __init as7535_28xb_fan_init(void)
{
	int ret;

	data = kzalloc(sizeof(struct as7535_28xb_fan_data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto alloc_err;
	}

	mutex_init(&data->update_lock);
	data->valid = 0;

	ret = platform_driver_register(&as7535_28xb_fan_driver);
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
	platform_driver_unregister(&as7535_28xb_fan_driver);
dri_reg_err:
	kfree(data);
alloc_err:
	return ret;
}

static void __exit as7535_28xb_fan_exit(void)
{
	ipmi_destroy_user(data->ipmi.user);
	platform_device_unregister(data->pdev);
	platform_driver_unregister(&as7535_28xb_fan_driver);
	kfree(data);
}

MODULE_AUTHOR("Brandon Chuang <brandon_chuang@accton.com.tw>");
MODULE_DESCRIPTION("as7535_28xb_fan driver");
MODULE_LICENSE("GPL");

module_init(as7535_28xb_fan_init);
module_exit(as7535_28xb_fan_exit);
