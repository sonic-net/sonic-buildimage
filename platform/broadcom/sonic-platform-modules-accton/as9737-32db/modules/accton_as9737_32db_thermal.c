/*
 * Copyright (C)  Roger Ho <roger530_ho@edge-core.com>
 *
 * Based on:
 *    pca954x.c from Kumar Gala <galak@kernel.crashing.org>
 * Copyright (C) 2006
 *
 * Based on:
 *    pca954x.c from Ken Harrenstien
 * Copyright (C) 2004 Google, Inc. (Ken Harrenstien)
 *
 * Based on:
 *    i2c-virtual_cb.c from Brian Kuschak <bkuschak@yahoo.com>
 * and
 *    pca9540.c from Jean Delvare <khali@linux-fr.org>.
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
#include <linux/string_helpers.h>

#define DRVNAME "as9737_32db_thermal"
#define ACCTON_IPMI_NETFN 0x34
#define IPMI_THERMAL_READ_CMD 0x12
#define THERMAL_COUNT    5
#define THERMAL_DATA_LEN 3
#define THERMAL_DATA_COUNT (THERMAL_COUNT * THERMAL_DATA_LEN)

#define IPMI_TIMEOUT (5 * HZ)
#define IPMI_ERR_RETRY_TIMES 1

static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data);
static ssize_t show_temp(struct device *dev, struct device_attribute *attr,
    char *buf);
#ifdef ENABLE_THRESHOLD
static ssize_t show_threshold(struct device *dev, struct device_attribute *da,
    char *buf);
#endif
static int as9737_32db_thermal_probe(struct platform_device *pdev);
static int as9737_32db_thermal_remove(struct platform_device *pdev);

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

struct as9737_32db_thermal_data {
    struct platform_device *pdev;
    struct device   *hwmon_dev;
    struct mutex update_lock;
    char valid;           /* != 0 if registers are valid */
    unsigned long last_updated;    /* In jiffies */
    char   ipmi_resp[THERMAL_DATA_COUNT]; /* 3 bytes for each thermal */
    struct ipmi_data ipmi;
    unsigned char ipmi_tx_data[2];  /* 0: thermal id, 1: temp */
};

#ifdef ENABLE_THRESHOLD
static s8 temp_max_alarm[THERMAL_COUNT] = { 85, 85, 85, 79, 92};
static s8 temp_max[THERMAL_COUNT] = { 80, 80, 80, 74, 87};
static s8 temp_min[THERMAL_COUNT] = { -45, -45, -45, -45, -45};
static s8 temp_min_alarm[THERMAL_COUNT] = { -50, -50, -50, -50, -50};
#endif

struct as9737_32db_thermal_data *data = NULL;

static struct platform_driver as9737_32db_thermal_driver = {
    .probe = as9737_32db_thermal_probe,
    .remove = as9737_32db_thermal_remove,
    .driver = {
        .name = DRVNAME,
        .owner = THIS_MODULE,
    },
};

enum as9737_32db_thermal_sysfs_attrs {
    TEMP1_INPUT, // 0x48
    TEMP2_INPUT, // 0x49
    TEMP3_INPUT, // 0x4A
    TEMP4_INPUT, // 0x4C
    TEMP5_INPUT, // 0x4F
    TEMP1_MAX_ALARM,
    TEMP2_MAX_ALARM,
    TEMP3_MAX_ALARM,
    TEMP4_MAX_ALARM,
    TEMP5_MAX_ALARM,
    TEMP1_MAX,
    TEMP2_MAX,
    TEMP3_MAX,
    TEMP4_MAX,
    TEMP5_MAX,
    TEMP1_MIN,
    TEMP2_MIN,
    TEMP3_MIN,
    TEMP4_MIN,
    TEMP5_MIN,
    TEMP1_MIN_ALARM,
    TEMP2_MIN_ALARM,
    TEMP3_MIN_ALARM,
    TEMP4_MIN_ALARM,
    TEMP5_MIN_ALARM,
};

#ifdef ENABLE_THRESHOLD
// Read only temp_input
#define DECLARE_THERMAL_SENSOR_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(temp##index##_input, S_IRUGO, show_temp, \
                    NULL, TEMP##index##_INPUT); \
    static SENSOR_DEVICE_ATTR(temp##index##_crit, S_IRUGO, show_threshold,\
                    NULL, TEMP##index##_MAX_ALARM); \
    static SENSOR_DEVICE_ATTR(temp##index##_max, S_IRUGO, show_threshold,\
                    NULL, TEMP##index##_MAX); \
    static SENSOR_DEVICE_ATTR(temp##index##_min, S_IRUGO, show_threshold,\
                    NULL, TEMP##index##_MIN); \
    static SENSOR_DEVICE_ATTR(temp##index##_lcrit, S_IRUGO, show_threshold,\
                    NULL, TEMP##index##_MIN_ALARM)

#define DECLARE_THERMAL_ATTR(index) \
    &sensor_dev_attr_temp##index##_input.dev_attr.attr, \
    &sensor_dev_attr_temp##index##_crit.dev_attr.attr, \
    &sensor_dev_attr_temp##index##_max.dev_attr.attr, \
    &sensor_dev_attr_temp##index##_min.dev_attr.attr, \
    &sensor_dev_attr_temp##index##_lcrit.dev_attr.attr
#else
#define DECLARE_THERMAL_SENSOR_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(temp##index##_input, S_IRUGO, show_temp, \
                    NULL, TEMP##index##_INPUT); 

#define DECLARE_THERMAL_ATTR(index) \
    &sensor_dev_attr_temp##index##_input.dev_attr.attr
#endif

DECLARE_THERMAL_SENSOR_DEVICE_ATTR(1);
DECLARE_THERMAL_SENSOR_DEVICE_ATTR(2);
DECLARE_THERMAL_SENSOR_DEVICE_ATTR(3);
DECLARE_THERMAL_SENSOR_DEVICE_ATTR(4);
DECLARE_THERMAL_SENSOR_DEVICE_ATTR(5);

static struct attribute *as9737_32db_thermal_attrs[] = {
    DECLARE_THERMAL_ATTR(1),
    DECLARE_THERMAL_ATTR(2),
    DECLARE_THERMAL_ATTR(3),
    DECLARE_THERMAL_ATTR(4),
    DECLARE_THERMAL_ATTR(5),
    NULL
};
ATTRIBUTE_GROUPS(as9737_32db_thermal);

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

    char *cmdline = kstrdup_quotable_cmdline(current, GFP_KERNEL);

    int i = 0;
    char raw_cmd[20] = "";
    sprintf(raw_cmd, "0x%02x", cmd);
    if(tx_len)
    {
        for( i=0; i < tx_len; i++) {
            sprintf(raw_cmd + strlen(raw_cmd), " 0x%02x", tx_data[i]);
        }
    }

    for (retry = 0; retry <= IPMI_ERR_RETRY_TIMES; retry++) {
        status = _ipmi_send_message(ipmi, cmd, tx_data, tx_len, rx_data, rx_len);
        if (unlikely(status != 0)) {
            dev_err(&data->pdev->dev, "ipmi cmd(%x) err status(%d)[%s] raw_cmd=[%s]\r\n",
                                        cmd, status, cmdline ? cmdline : "", raw_cmd);
            continue;
        }

        if (unlikely(ipmi->rx_result != 0)) {
            dev_err(&data->pdev->dev, "ipmi cmd(%x) err result(%d)[%s] raw_cmd=[%s]\r\n",
                                        cmd, ipmi->rx_result, cmdline ? cmdline : "", raw_cmd);
            continue;
        }

        break;
    }

    if (cmdline) kfree(cmdline);

    return status;
}

/* Dispatch IPMI messages to callers */
static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data)
{
    unsigned short rx_len;
    struct ipmi_data *ipmi = user_msg_data;

    if (msg->msgid != ipmi->tx_msgid) {
        dev_notice(&data->pdev->dev, "Mismatch between received msgid "
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

#ifdef ENABLE_THRESHOLD
static ssize_t show_threshold(struct device *dev, struct device_attribute *da,
                            char *buf)
{
    int status = 0;
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);

    mutex_lock(&data->update_lock);

    switch (attr->index) {
    case TEMP1_MAX_ALARM ... TEMP5_MAX_ALARM:
        status = (int)temp_max_alarm[attr->index - TEMP1_MAX_ALARM];
        break;
    case TEMP1_MAX ... TEMP5_MAX:
        status = (int)temp_max[attr->index - TEMP1_MAX];
        break;
    case TEMP1_MIN ... TEMP5_MIN:
        status = (int)temp_min[attr->index - TEMP1_MIN];
        break;
    case TEMP1_MIN_ALARM ... TEMP5_MIN_ALARM:
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
#endif

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

static int as9737_32db_thermal_probe(struct platform_device *pdev)
{
    int status = 0;
    struct device *hwmon_dev;

    hwmon_dev = hwmon_device_register_with_groups(&pdev->dev, DRVNAME, 
                    NULL, as9737_32db_thermal_groups);
    if (IS_ERR(data->hwmon_dev)) {
        status = PTR_ERR(data->hwmon_dev);
        return status;
    }

    mutex_lock(&data->update_lock);
    data->hwmon_dev = hwmon_dev;
    mutex_unlock(&data->update_lock);

    dev_info(&pdev->dev, "Device Created\n");

    return status;
}

static int as9737_32db_thermal_remove(struct platform_device *pdev)
{
    mutex_lock(&data->update_lock);
    if (data->hwmon_dev) {
        hwmon_device_unregister(data->hwmon_dev);
        data->hwmon_dev = NULL;
    }
    mutex_unlock(&data->update_lock);

    return 0;
}

static int __init as9737_32db_thermal_init(void)
{
    int ret;

    data = kzalloc(sizeof(struct as9737_32db_thermal_data), GFP_KERNEL);
    if (!data) {
        ret = -ENOMEM;
        goto alloc_err;
    }

    mutex_init(&data->update_lock);

    ret = platform_driver_register(&as9737_32db_thermal_driver);
    if (ret < 0)
        goto dri_reg_err;

    data->pdev = platform_device_register_simple(DRVNAME, -1, NULL, 0);
    if (IS_ERR(data->pdev)) {
        ret = PTR_ERR(data->pdev);
        goto dev_reg_err;
    }

    /* Set up IPMI interface */
    ret = init_ipmi_data(&data->ipmi, 0, &data->pdev->dev);
    if (ret) {
        goto ipmi_err;
    }

    return 0;

ipmi_err:
    platform_device_unregister(data->pdev);
dev_reg_err:
    platform_driver_unregister(&as9737_32db_thermal_driver);
dri_reg_err:
    kfree(data);
alloc_err:
    return ret;
}

static void __exit as9737_32db_thermal_exit(void)
{
    if (data) {
        ipmi_destroy_user(data->ipmi.user);
        platform_device_unregister(data->pdev);
        platform_driver_unregister(&as9737_32db_thermal_driver);
        kfree(data);
    }
}

MODULE_AUTHOR("Roger Ho <roger530_ho@edge-core.com>");
MODULE_DESCRIPTION("as9737_32db_thermal driver");
MODULE_LICENSE("GPL");

module_init(as9737_32db_thermal_init);
module_exit(as9737_32db_thermal_exit);
