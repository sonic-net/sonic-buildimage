/*
 * Copyright (C)  Roger Ho <roger530_ho@edge-core.com>
 *
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

#define DRVNAME "as9737_32db_fan"
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
static int as9737_32db_fan_probe(struct platform_device *pdev);
static int as9737_32db_fan_remove(struct platform_device *pdev);

enum fan_id {
    FAN_1,
    FAN_2,
    FAN_3,
    FAN_4,
    FAN_5,
    FAN_6,
    FAN_7,
    FAN_8,
    FAN_9,
    FAN_10,
    FAN_11,
    FAN_12,
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

struct as9737_32db_fan_data {
    struct platform_device *pdev;
    struct device   *hwmon_dev;
    struct mutex update_lock;
    char valid; /* != 0 if registers are valid */
    unsigned long last_updated;    /* In jiffies */
    /* 4 bytes for each fan, the last 2 bytes is fan dir */
    unsigned char ipmi_resp[NUM_OF_FAN * FAN_DATA_COUNT + 2];
    unsigned char ipmi_resp_cpld[2];
    unsigned char ipmi_resp_speed[NUM_OF_FAN * FAN_SPEED_DATA_COUNT];
    struct ipmi_data ipmi;
    unsigned char ipmi_tx_data[3];  /* 0: FAN id, 1: 0x02, 2: PWM */
};

struct as9737_32db_fan_data *data = NULL;

static struct platform_driver as9737_32db_fan_driver = {
    .probe = as9737_32db_fan_probe,
    .remove = as9737_32db_fan_remove,
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

enum as9737_32db_fan_sysfs_attrs {
    FAN_ATTR(1),
    FAN_ATTR(2),
    FAN_ATTR(3),
    FAN_ATTR(4),
    FAN_ATTR(5),
    FAN_ATTR(6),
    FAN_ATTR(7),
    FAN_ATTR(8),
    FAN_ATTR(9),
    FAN_ATTR(10),
    FAN_ATTR(11),
    FAN_ATTR(12),
    NUM_OF_FAN_ATTR,
    FAN_VERSION,
    FAN_MAX_RPM,
    NUM_OF_PER_FAN_ATTR = (NUM_OF_FAN_ATTR/NUM_OF_FAN),
    FAN_RPM_THRESHOLD_ATTR(1),
    FAN_RPM_THRESHOLD_ATTR(2),
    FAN_RPM_THRESHOLD_ATTR(3),
    FAN_RPM_THRESHOLD_ATTR(4),
    FAN_RPM_THRESHOLD_ATTR(5),
    FAN_RPM_THRESHOLD_ATTR(6),
    FAN_RPM_THRESHOLD_ATTR(7),
    FAN_RPM_THRESHOLD_ATTR(8),
    FAN_RPM_THRESHOLD_ATTR(9),
    FAN_RPM_THRESHOLD_ATTR(10),
    FAN_RPM_THRESHOLD_ATTR(11),
    FAN_RPM_THRESHOLD_ATTR(12),
    NUM_OF_FAN_RPM_THRESHOLD_ATTR,
    NUM_OF_PER_FAN_RPM_THRESHOLD_ATTR = ((NUM_OF_FAN_RPM_THRESHOLD_ATTR-NUM_OF_PER_FAN_ATTR)/NUM_OF_FAN),
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
DECLARE_FAN_SENSOR_DEVICE_ATTR(7);
DECLARE_FAN_SENSOR_DEVICE_ATTR(8);
DECLARE_FAN_SENSOR_DEVICE_ATTR(9);
DECLARE_FAN_SENSOR_DEVICE_ATTR(10);
DECLARE_FAN_SENSOR_DEVICE_ATTR(11);
DECLARE_FAN_SENSOR_DEVICE_ATTR(12);
DECLARE_FAN_VER_SENSOR_DEVICE_ATTR();

static struct attribute *as9737_32db_fan_attrs[] = {
    /* fan attributes */
    DECLARE_FAN_ATTR(1),
    DECLARE_FAN_ATTR(2),
    DECLARE_FAN_ATTR(3),
    DECLARE_FAN_ATTR(4),
    DECLARE_FAN_ATTR(5),
    DECLARE_FAN_ATTR(6),
    DECLARE_FAN_ATTR(7),
    DECLARE_FAN_ATTR(8),
    DECLARE_FAN_ATTR(9),
    DECLARE_FAN_ATTR(10),
    DECLARE_FAN_ATTR(11),
    DECLARE_FAN_ATTR(12),
    DECLARE_FAN_VER_ATTR(),
    DECLARE_FAN_MAX_RPM_ATTR(),
    NULL
};
ATTRIBUTE_GROUPS(as9737_32db_fan);

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

    ipmi->tx_message.cmd      = cmd;
    ipmi->tx_message.data     = tx_data;
    ipmi->tx_message.data_len = tx_len;
    ipmi->rx_msg_data         = rx_data;
    ipmi->rx_msg_len          = rx_len;

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
        status = _ipmi_send_message(ipmi,cmd, tx_data, tx_len, rx_data, rx_len);
        if (unlikely(status != 0)) {
            dev_err(&data->pdev->dev, "ipmi_send_message_%d err status(%d)[%s] raw_cmd=[%s]\r\n",
                                        retry, status, cmdline ? cmdline : "", raw_cmd);
            continue;
        }

        if (unlikely(ipmi->rx_result != 0)) {
            dev_err(&data->pdev->dev, "ipmi_send_message_%d err result(%d)[%s] raw_cmd=[%s]\r\n",
                                        retry, ipmi->rx_result, cmdline ? cmdline : "", raw_cmd);
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

static struct as9737_32db_fan_data *as9737_32db_fan_update_device(void)
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

    data = as9737_32db_fan_update_device();
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
    case FAN7_PRESENT:
    case FAN8_PRESENT:
    case FAN9_PRESENT:
    case FAN10_PRESENT:
    case FAN11_PRESENT:
    case FAN12_PRESENT:
        value = present;
        break;
    case FAN1_PWM:
    case FAN2_PWM:
    case FAN3_PWM:
    case FAN4_PWM:
    case FAN5_PWM:
    case FAN6_PWM:
    case FAN7_PWM:
    case FAN8_PWM:
    case FAN9_PWM:
    case FAN10_PWM:
    case FAN11_PWM:
    case FAN12_PWM:
        index = (fid % NUM_OF_FAN_MODULE) * FAN_DATA_COUNT;
        value = DIV_ROUND_CLOSEST(data->ipmi_resp[index + FAN_PWM] * 100, 255);
        break;
    case FAN1_INPUT:
    case FAN2_INPUT:
    case FAN3_INPUT:
    case FAN4_INPUT:
    case FAN5_INPUT:
    case FAN6_INPUT:
    case FAN7_INPUT:
    case FAN8_INPUT:
    case FAN9_INPUT:
    case FAN10_INPUT:
    case FAN11_INPUT:
    case FAN12_INPUT:
        value = (int)data->ipmi_resp[index + FAN_SPEED0] |
                (int)data->ipmi_resp[index + FAN_SPEED1] << 8;
        break;
    case FAN1_FAULT:
    case FAN2_FAULT:
    case FAN3_FAULT:
    case FAN4_FAULT:
    case FAN5_FAULT:
    case FAN6_FAULT:
    case FAN7_FAULT:
    case FAN8_FAULT:
    case FAN9_FAULT:
    case FAN10_FAULT:
    case FAN11_FAULT:
    case FAN12_FAULT:
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

    pwm = DIV_ROUND_CLOSEST(pwm * 255, 100); /* Convert pwm to register value */

    mutex_lock(&data->update_lock);

    /*
     * Send IPMI write command :
     * BMC supports a design with four fan modules, each containing two fans.
     * Since NUM_OF_FAN_MODULE is 12 in AS9737-32DB, it needs to be divided by 2.
     * The result :
     * fan1_pwm(Front) : ipmi_tx_data[0] = 1, fan2_pwm(Front) : ipmi_tx_data[0] = 2
     * fan3_pwm(Front) : ipmi_tx_data[0] = 3, fan4_pwm(Front) : ipmi_tx_data[0] = 4
     * fan5_pwm(Rear) : ipmi_tx_data[0] = 1, fan6_pwm(Rear) : ipmi_tx_data[0] = 2
     * fan7_pwm(Rear) : ipmi_tx_data[0] = 3, fan8_pwm(Rear) : ipmi_tx_data[0] = 4
     * fan9_pwm(Rear) : ipmi_tx_data[0] = 1, fan10_pwm(Rear) : ipmi_tx_data[0] = 2
     * fan11_pwm(Rear) : ipmi_tx_data[0] = 3, fan12_pwm(Rear) : ipmi_tx_data[0] = 4
     *
     */
    data->ipmi_tx_data[0] = (fid % (NUM_OF_FAN_MODULE / 2)) + 1;
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

static struct as9737_32db_fan_data *as9737_32db_fan_update_cpld_ver(void)
{
    int status = 0;

    data->valid = 0;
    data->ipmi_tx_data[0] = 0x66;
    status = ipmi_send_message(&data->ipmi, IPMI_FAN_REG_READ_CMD,
                                data->ipmi_tx_data, 1,
                                data->ipmi_resp_cpld,
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
    unsigned char major;
    unsigned char minor;
    int error = 0;

    mutex_lock(&data->update_lock);

    data = as9737_32db_fan_update_cpld_ver();
    if (!data->valid) {
        error = -EIO;
        goto exit;
    }

    major = data->ipmi_resp_cpld[0];
    minor = data->ipmi_resp_cpld[1];
    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%d.%d\n", major, minor);

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

    data = as9737_32db_fan_update_device();
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
    unsigned char fid;
    int index = 0;
    int value = 0;
    int error = 0;

    mutex_lock(&data->update_lock);

    data = as9737_32db_fan_update_device();
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
    case FAN7_TARGET:
    case FAN8_TARGET:
    case FAN9_TARGET:
    case FAN10_TARGET:
    case FAN11_TARGET:
    case FAN12_TARGET:
        fid = (attr->index - FAN1_TARGET) / NUM_OF_PER_FAN_RPM_THRESHOLD_ATTR;
        index = fid * FAN_SPEED_DATA_COUNT;
        value = (int)data->ipmi_resp_speed[index + FAN_TARGET_SPEED0] |
                (int)data->ipmi_resp_speed[index + FAN_TARGET_SPEED1] << 8;
        break;
    case FAN1_TOLERANCE:
    case FAN2_TOLERANCE:
    case FAN3_TOLERANCE:
    case FAN4_TOLERANCE:
    case FAN5_TOLERANCE:
    case FAN6_TOLERANCE:
    case FAN7_TOLERANCE:
    case FAN8_TOLERANCE:
    case FAN9_TOLERANCE:
    case FAN10_TOLERANCE:
    case FAN11_TOLERANCE:
    case FAN12_TOLERANCE:
        fid = (attr->index - FAN1_TOLERANCE) / NUM_OF_PER_FAN_RPM_THRESHOLD_ATTR;
        index = fid * FAN_SPEED_DATA_COUNT;
        value = (int)data->ipmi_resp_speed[index + FAN_SPEED_TOLERANCE];
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

static int as9737_32db_fan_probe(struct platform_device *pdev)
{
    int status = 0;
    struct device *hwmon_dev;

    hwmon_dev = hwmon_device_register_with_groups(&pdev->dev, DRVNAME,
                    NULL, as9737_32db_fan_groups);
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

static int as9737_32db_fan_remove(struct platform_device *pdev)
{
    mutex_lock(&data->update_lock);
    if (data->hwmon_dev) {
        hwmon_device_unregister(data->hwmon_dev);
        data->hwmon_dev = NULL;
    }
    mutex_unlock(&data->update_lock);

    return 0;
}

static int __init as9737_32db_fan_init(void)
{
    int ret;

    data = kzalloc(sizeof(struct as9737_32db_fan_data), GFP_KERNEL);
    if (!data) {
        ret = -ENOMEM;
        goto alloc_err;
    }

    mutex_init(&data->update_lock);

    ret = platform_driver_register(&as9737_32db_fan_driver);
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
    platform_driver_unregister(&as9737_32db_fan_driver);
dri_reg_err:
    kfree(data);
alloc_err:
    return ret;
}

static void __exit as9737_32db_fan_exit(void)
{
    if (data) {
        ipmi_destroy_user(data->ipmi.user);
        platform_device_unregister(data->pdev);
        kfree(data);
    }
    platform_driver_unregister(&as9737_32db_fan_driver);
}

MODULE_AUTHOR("Roger Ho <roger530_ho@edge-core.com>");
MODULE_DESCRIPTION("as9737_32db_fan driver");
MODULE_LICENSE("GPL");

module_init(as9737_32db_fan_init);
module_exit(as9737_32db_fan_exit);
