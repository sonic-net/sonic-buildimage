/*
 * Copyright (C)  Roger Ho <roger530_ho@edge-core.com>
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

#define DRVNAME "as9737_32db_psu"
#define ACCTON_IPMI_NETFN 0x34
#define IPMI_PSU_READ_CMD 0x16
#define IPMI_PSU_MODEL_NAME_CMD 0x10
#define IPMI_PSU_SERIAL_NUM_CMD 0x11
#define IPMI_PSU_FAN_DIR_CMD 0x13
#define IPMI_PSU_INFO_CMD 0x20
#define IPMI_TIMEOUT (5 * HZ)
#define IPMI_ERR_RETRY_TIMES 1
#define IPMI_MODEL_SERIAL_LEN 32
#define IPMI_FAN_DIR_LEN 3

static void ipmi_msg_handler(struct ipmi_recv_msg *msg, void *user_msg_data);
static ssize_t show_psu(struct device *dev, struct device_attribute *attr,
                            char *buf);
static ssize_t show_psu_info(struct device *dev, struct device_attribute *attr,
                            char *buf);
static ssize_t show_string(struct device *dev, struct device_attribute *attr,
                            char *buf);
static int as9737_32db_psu_probe(struct platform_device *pdev);
static int as9737_32db_psu_remove(struct platform_device *pdev);

enum psu_id {
    PSU_1,
    PSU_2,
    NUM_OF_PSU
};

enum psu_data_index {
    PSU_PRESENT = 0,
    PSU_TEMP_FAULT,
    PSU_POWER_GOOD_CPLD,
    PSU_POWER_GOOD_PMBUS,
    PSU_OVER_VOLTAGE,
    PSU_OVER_CURRENT,
    PSU_POWER_ON,
    PSU_VIN0,
    PSU_VIN1,
    PSU_VIN2,
    PSU_VOUT0,
    PSU_VOUT1,
    PSU_VOUT2,
    PSU_IIN0,
    PSU_IIN1,
    PSU_IIN2,
    PSU_IOUT0,
    PSU_IOUT1,
    PSU_IOUT2,
    PSU_PIN0,
    PSU_PIN1,
    PSU_PIN2,
    PSU_PIN3,
    PSU_POUT0,
    PSU_POUT1,
    PSU_POUT2,
    PSU_POUT3,
    PSU_TEMP1_0,
    PSU_TEMP1_1,
    PSU_TEMP2_0,
    PSU_TEMP2_1,
    PSU_TEMP3_0,
    PSU_TEMP3_1,
    PSU_FAN0,
    PSU_FAN1,
    PSU_VOUT_MODE,
    PSU_STATUS_COUNT,
    PSU_MODEL = 0,
    PSU_SERIAL = 0,
    PSU_TEMP1_MAX0 = 2,
    PSU_TEMP1_MAX1,
    PSU_TEMP1_MIN0,
    PSU_TEMP1_MIN1,
    PSU_TEMP2_MAX0,
    PSU_TEMP2_MAX1,
    PSU_TEMP2_MIN0,
    PSU_TEMP2_MIN1,
    PSU_TEMP3_MAX0,
    PSU_TEMP3_MAX1,
    PSU_TEMP3_MIN0,
    PSU_TEMP3_MIN1,
    PSU_VIN_UPPER_CRIT0,
    PSU_VIN_UPPER_CRIT1,
    PSU_VIN_UPPER_CRIT2,
    PSU_VIN_MAX0,
    PSU_VIN_MAX1,
    PSU_VIN_MAX2,
    PSU_VIN_MIN0,
    PSU_VIN_MIN1,
    PSU_VIN_MIN2,
    PSU_VIN_LOWER_CRIT0,
    PSU_VIN_LOWER_CRIT1,
    PSU_VIN_LOWER_CRIT2,
    PSU_VOUT_MAX0,
    PSU_VOUT_MAX1,
    PSU_VOUT_MAX2,
    PSU_VOUT_MIN0,
    PSU_VOUT_MIN1,
    PSU_VOUT_MIN2,
    PSU_IIN_MAX0,
    PSU_IIN_MAX1,
    PSU_IIN_MAX2,
    PSU_IOUT_MAX0,
    PSU_IOUT_MAX1,
    PSU_IOUT_MAX2,
    PSU_PIN_MAX0,
    PSU_PIN_MAX1,
    PSU_PIN_MAX2,
    PSU_PIN_MAX3,
    PSU_POUT_MAX0,
    PSU_POUT_MAX1,
    PSU_POUT_MAX2,
    PSU_POUT_MAX3,
    PSU_INFO_COUNT
};

struct ipmi_data {
    struct completion read_complete;
    struct ipmi_addr address;
    struct ipmi_user * user;
    int interface;
    int pid;

    struct kernel_ipmi_msg tx_message;
    long tx_msgid;

    void *rx_msg_data;
    unsigned short rx_msg_len;
    unsigned char rx_result;
    int rx_recv_type;

    struct ipmi_user_hndl ipmi_hndlrs;
};

struct ipmi_psu_resp_data {
    unsigned char status[PSU_STATUS_COUNT];
    unsigned char info[PSU_INFO_COUNT];
    char serial[IPMI_MODEL_SERIAL_LEN+1];
    char model[IPMI_MODEL_SERIAL_LEN+1];
    char fandir[IPMI_FAN_DIR_LEN+1];
};

struct as9737_32db_psu_data {
    struct platform_device *pdev[2];
    struct device   *hwmon_dev[2];
    struct mutex update_lock;
    char valid[2]; /* != 0 if registers are valid, 0: PSU1, 1: PSU2 */
    unsigned long last_updated[2];    /* In jiffies, 0: PSU1, 1: PSU2 */
    struct ipmi_data ipmi;
    struct ipmi_psu_resp_data ipmi_resp[2]; /* 0: PSU1, 1: PSU2 */
    unsigned char ipmi_tx_data[2];
};

struct as9737_32db_psu_data *data = NULL;

static struct platform_driver as9737_32db_psu_driver = {
    .probe = as9737_32db_psu_probe,
    .remove = as9737_32db_psu_remove,
    .driver = {
        .name = DRVNAME,
        .owner = THIS_MODULE,
    },
};

#define PSU_PRESENT_ATTR_ID(index) PSU##index##_PRESENT
#define PSU_POWERGOOD_ATTR_ID(index) PSU##index##_POWER_GOOD
#define PSU_VIN_ATTR_ID(index) PSU##index##_VIN
#define PSU_VOUT_ATTR_ID(index) PSU##index##_VOUT
#define PSU_IIN_ATTR_ID(index) PSU##index##_IIN
#define PSU_IOUT_ATTR_ID(index) PSU##index##_IOUT
#define PSU_PIN_ATTR_ID(index) PSU##index##_PIN
#define PSU_POUT_ATTR_ID(index) PSU##index##_POUT
#define PSU_MODEL_ATTR_ID(index) PSU##index##_MODEL
#define PSU_SERIAL_ATTR_ID(index) PSU##index##_SERIAL
#define PSU_TEMP1_INPUT_ATTR_ID(index) PSU##index##_TEMP1_INPUT
#define PSU_TEMP2_INPUT_ATTR_ID(index) PSU##index##_TEMP2_INPUT
#define PSU_TEMP3_INPUT_ATTR_ID(index) PSU##index##_TEMP3_INPUT
#define PSU_FAN_INPUT_ATTR_ID(index) PSU##index##_FAN_INPUT
#define PSU_FAN_DIR_ATTR_ID(index) PSU##index##_FAN_DIR

#define PSU_TEMP1_INPUT_MAX_ATTR_ID(index) PSU##index##_TEMP1_INPUT_MAX
#define PSU_TEMP1_INPUT_MIN_ATTR_ID(index) PSU##index##_TEMP1_INPUT_MIN
#define PSU_TEMP2_INPUT_MAX_ATTR_ID(index) PSU##index##_TEMP2_INPUT_MAX
#define PSU_TEMP2_INPUT_MIN_ATTR_ID(index) PSU##index##_TEMP2_INPUT_MIN
#define PSU_TEMP3_INPUT_MAX_ATTR_ID(index) PSU##index##_TEMP3_INPUT_MAX
#define PSU_TEMP3_INPUT_MIN_ATTR_ID(index) PSU##index##_TEMP3_INPUT_MIN
#define PSU_VIN_MAX_ATTR_ID(index) PSU##index##_VIN_MAX
#define PSU_VIN_MIN_ATTR_ID(index) PSU##index##_VIN_MIN
#define PSU_VIN_UPPER_CRIT_ATTR_ID(index) PSU##index##_VIN_UPPER_CRIT
#define PSU_VIN_LOWER_CRIT_ATTR_ID(index) PSU##index##_VIN_LOWER_CRIT
#define PSU_VOUT_MAX_ATTR_ID(index) PSU##index##_VOUT_MAX
#define PSU_VOUT_MIN_ATTR_ID(index) PSU##index##_VOUT_MIN
#define PSU_IIN_MAX_ATTR_ID(index) PSU##index##_IIN_MAX
#define PSU_IOUT_MAX_ATTR_ID(index) PSU##index##_IOUT_MAX
#define PSU_PIN_MAX_ATTR_ID(index) PSU##index##_PIN_MAX
#define PSU_POUT_MAX_ATTR_ID(index) PSU##index##_POUT_MAX

#define PSU_ATTR(psu_id) \
    PSU_PRESENT_ATTR_ID(psu_id), \
    PSU_POWERGOOD_ATTR_ID(psu_id), \
    PSU_VIN_ATTR_ID(psu_id), \
    PSU_VOUT_ATTR_ID(psu_id), \
    PSU_IIN_ATTR_ID(psu_id), \
    PSU_IOUT_ATTR_ID(psu_id), \
    PSU_PIN_ATTR_ID(psu_id), \
    PSU_POUT_ATTR_ID(psu_id), \
    PSU_MODEL_ATTR_ID(psu_id), \
    PSU_SERIAL_ATTR_ID(psu_id), \
    PSU_TEMP1_INPUT_ATTR_ID(psu_id), \
    PSU_TEMP2_INPUT_ATTR_ID(psu_id), \
    PSU_TEMP3_INPUT_ATTR_ID(psu_id), \
    PSU_FAN_INPUT_ATTR_ID(psu_id), \
    PSU_FAN_DIR_ATTR_ID(psu_id), \
    PSU_TEMP1_INPUT_MAX_ATTR_ID(psu_id), \
    PSU_TEMP1_INPUT_MIN_ATTR_ID(psu_id), \
    PSU_TEMP2_INPUT_MAX_ATTR_ID(psu_id), \
    PSU_TEMP2_INPUT_MIN_ATTR_ID(psu_id), \
    PSU_TEMP3_INPUT_MAX_ATTR_ID(psu_id), \
    PSU_TEMP3_INPUT_MIN_ATTR_ID(psu_id), \
    PSU_VIN_MAX_ATTR_ID(psu_id), \
    PSU_VIN_MIN_ATTR_ID(psu_id), \
    PSU_VIN_UPPER_CRIT_ATTR_ID(psu_id), \
    PSU_VIN_LOWER_CRIT_ATTR_ID(psu_id), \
    PSU_VOUT_MAX_ATTR_ID(psu_id), \
    PSU_VOUT_MIN_ATTR_ID(psu_id), \
    PSU_IIN_MAX_ATTR_ID(psu_id), \
    PSU_IOUT_MAX_ATTR_ID(psu_id), \
    PSU_PIN_MAX_ATTR_ID(psu_id), \
    PSU_POUT_MAX_ATTR_ID(psu_id)

enum as9737_32db_psu_sysfs_attrs {
    /* psu attributes */
    PSU_ATTR(1),
    PSU_ATTR(2),
    NUM_OF_PSU_ATTR,
    NUM_OF_PER_PSU_ATTR = (NUM_OF_PSU_ATTR/NUM_OF_PSU)
};

/* psu attributes */
#define DECLARE_PSU_SENSOR_DEVICE_ATTR(index) \
    static SENSOR_DEVICE_ATTR(psu##index##_present,    S_IRUGO, show_psu, NULL, \
                                PSU##index##_PRESENT); \
    static SENSOR_DEVICE_ATTR(psu##index##_power_good, S_IRUGO, show_psu, NULL,\
                                PSU##index##_POWER_GOOD); \
    static SENSOR_DEVICE_ATTR(psu##index##_vin, S_IRUGO, show_psu, NULL, \
                                PSU##index##_VIN); \
    static SENSOR_DEVICE_ATTR(psu##index##_vout, S_IRUGO, show_psu, NULL, \
                                PSU##index##_VOUT); \
    static SENSOR_DEVICE_ATTR(psu##index##_iin, S_IRUGO, show_psu, NULL, \
                                PSU##index##_IIN); \
    static SENSOR_DEVICE_ATTR(psu##index##_iout, S_IRUGO, show_psu, NULL, \
                                PSU##index##_IOUT); \
    static SENSOR_DEVICE_ATTR(psu##index##_pin, S_IRUGO, show_psu, NULL, \
                                PSU##index##_PIN); \
    static SENSOR_DEVICE_ATTR(psu##index##_pout, S_IRUGO, show_psu, NULL, \
                                PSU##index##_POUT); \
    static SENSOR_DEVICE_ATTR(psu##index##_model, S_IRUGO, show_string, NULL, \
                                PSU##index##_MODEL); \
    static SENSOR_DEVICE_ATTR(psu##index##_serial, S_IRUGO, show_string, NULL,\
                                PSU##index##_SERIAL);\
    static SENSOR_DEVICE_ATTR(psu##index##_temp1_input, S_IRUGO, show_psu,NULL,\
                                PSU##index##_TEMP1_INPUT); \
    static SENSOR_DEVICE_ATTR(psu##index##_temp2_input, S_IRUGO, show_psu,NULL,\
                                PSU##index##_TEMP2_INPUT); \
    static SENSOR_DEVICE_ATTR(psu##index##_temp3_input, S_IRUGO, show_psu,NULL,\
                                PSU##index##_TEMP3_INPUT); \
    static SENSOR_DEVICE_ATTR(psu##index##_fan1_input, S_IRUGO, show_psu, NULL,\
                                PSU##index##_FAN_INPUT); \
    static SENSOR_DEVICE_ATTR(psu##index##_fan_dir, S_IRUGO, show_string, NULL,\
                                PSU##index##_FAN_DIR); \
    static SENSOR_DEVICE_ATTR(psu##index##_temp1_input_max, S_IRUGO, \
                        show_psu_info, NULL, PSU##index##_TEMP1_INPUT_MAX); \
    static SENSOR_DEVICE_ATTR(psu##index##_temp1_input_min, S_IRUGO, \
                        show_psu_info, NULL, PSU##index##_TEMP1_INPUT_MIN); \
    static SENSOR_DEVICE_ATTR(psu##index##_temp2_input_max, S_IRUGO, \
                        show_psu_info, NULL, PSU##index##_TEMP2_INPUT_MAX); \
    static SENSOR_DEVICE_ATTR(psu##index##_temp2_input_min, S_IRUGO, \
                        show_psu_info, NULL, PSU##index##_TEMP2_INPUT_MIN); \
    static SENSOR_DEVICE_ATTR(psu##index##_temp3_input_max, S_IRUGO, \
                        show_psu_info, NULL, PSU##index##_TEMP3_INPUT_MAX); \
    static SENSOR_DEVICE_ATTR(psu##index##_temp3_input_min, S_IRUGO, \
                        show_psu_info, NULL, PSU##index##_TEMP3_INPUT_MIN); \
    static SENSOR_DEVICE_ATTR(psu##index##_vin_max, S_IRUGO, \
                        show_psu_info, NULL,  PSU##index##_VIN_MAX); \
    static SENSOR_DEVICE_ATTR(psu##index##_vin_min, S_IRUGO, \
                        show_psu_info, NULL,  PSU##index##_VIN_MIN); \
    static SENSOR_DEVICE_ATTR(psu##index##_vin_upper_crit, S_IRUGO, \
                        show_psu_info, NULL,  PSU##index##_VIN_UPPER_CRIT); \
    static SENSOR_DEVICE_ATTR(psu##index##_vin_lower_crit, S_IRUGO, \
                        show_psu_info, NULL,  PSU##index##_VIN_LOWER_CRIT); \
    static SENSOR_DEVICE_ATTR(psu##index##_vout_max, S_IRUGO, \
                        show_psu_info, NULL,  PSU##index##_VOUT_MAX); \
    static SENSOR_DEVICE_ATTR(psu##index##_vout_min, S_IRUGO, \
                        show_psu_info, NULL,  PSU##index##_VOUT_MIN); \
    static SENSOR_DEVICE_ATTR(psu##index##_iin_max, S_IRUGO, \
                        show_psu_info, NULL,  PSU##index##_IIN_MAX); \
    static SENSOR_DEVICE_ATTR(psu##index##_iout_max, S_IRUGO, \
                        show_psu_info, NULL,  PSU##index##_IOUT_MAX); \
    static SENSOR_DEVICE_ATTR(psu##index##_pin_max, S_IRUGO, \
                        show_psu_info, NULL,  PSU##index##_PIN_MAX); \
    static SENSOR_DEVICE_ATTR(psu##index##_pout_max, S_IRUGO, \
                        show_psu_info, NULL,  PSU##index##_POUT_MAX)

#define DECLARE_PSU_ATTR(index) \
    &sensor_dev_attr_psu##index##_present.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_power_good.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_vin.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_vout.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_iin.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_iout.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_pin.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_pout.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_model.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_serial.dev_attr.attr,\
    &sensor_dev_attr_psu##index##_temp1_input.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_temp2_input.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_temp3_input.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_fan1_input.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_fan_dir.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_temp1_input_max.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_temp1_input_min.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_temp2_input_max.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_temp2_input_min.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_temp3_input_max.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_temp3_input_min.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_vin_max.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_vin_min.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_vin_upper_crit.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_vin_lower_crit.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_vout_max.dev_attr.attr,\
    &sensor_dev_attr_psu##index##_vout_min.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_iin_max.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_iout_max.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_pin_max.dev_attr.attr, \
    &sensor_dev_attr_psu##index##_pout_max.dev_attr.attr

DECLARE_PSU_SENSOR_DEVICE_ATTR(1);
/*Duplicate nodes for lm-sensors.*/
static SENSOR_DEVICE_ATTR(in0_input, S_IRUGO, show_psu, NULL, PSU1_VOUT);
static SENSOR_DEVICE_ATTR(curr1_input, S_IRUGO, show_psu, NULL, PSU1_IOUT);
static SENSOR_DEVICE_ATTR(power1_input, S_IRUGO, show_psu, NULL, PSU1_POUT);
static SENSOR_DEVICE_ATTR(temp1_input, S_IRUGO, show_psu, NULL, PSU1_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(fan1_input, S_IRUGO, show_psu, NULL, PSU1_FAN_INPUT);

static struct attribute *as9737_32db_psu1_attrs[] = {
    /* psu attributes */
    DECLARE_PSU_ATTR(1),
    &sensor_dev_attr_curr1_input.dev_attr.attr,
    &sensor_dev_attr_in0_input.dev_attr.attr,
    &sensor_dev_attr_power1_input.dev_attr.attr,
    &sensor_dev_attr_temp1_input.dev_attr.attr,
    &sensor_dev_attr_fan1_input.dev_attr.attr,
    NULL
};
static struct attribute_group as9737_32db_psu1_group = {
    .attrs = as9737_32db_psu1_attrs,
};
/* ATTRIBUTE_GROUPS(as9737_32db_psu1); */

DECLARE_PSU_SENSOR_DEVICE_ATTR(2);
/*Duplicate nodes for lm-sensors.*/
static SENSOR_DEVICE_ATTR(in1_input, S_IRUGO, show_psu, NULL, PSU2_VOUT);
static SENSOR_DEVICE_ATTR(curr2_input, S_IRUGO, show_psu, NULL, PSU2_IOUT);
static SENSOR_DEVICE_ATTR(power2_input, S_IRUGO, show_psu, NULL, PSU2_POUT);
static SENSOR_DEVICE_ATTR(temp2_input, S_IRUGO, show_psu, NULL, PSU2_TEMP1_INPUT);
static SENSOR_DEVICE_ATTR(fan2_input, S_IRUGO, show_psu, NULL, PSU2_FAN_INPUT);
static struct attribute *as9737_32db_psu2_attrs[] = {
    /* psu attributes */
    DECLARE_PSU_ATTR(2),
    &sensor_dev_attr_curr2_input.dev_attr.attr,
    &sensor_dev_attr_in1_input.dev_attr.attr,
    &sensor_dev_attr_power2_input.dev_attr.attr,
    &sensor_dev_attr_temp2_input.dev_attr.attr,
    &sensor_dev_attr_fan2_input.dev_attr.attr,
    NULL
};
static struct attribute_group as9737_32db_psu2_group = {
    .attrs = as9737_32db_psu2_attrs,
};
/* ATTRIBUTE_GROUPS(as9737_32db_psu2); */

const struct attribute_group *as9737_32db_psu_groups[][2] = {
    {&as9737_32db_psu1_group, NULL},
    {&as9737_32db_psu2_group, NULL}
};

/* Functions to talk to the IPMI layer */

/* Initialize IPMI address, message buffers and user data */
static int init_ipmi_data(struct ipmi_data *ipmi, int iface, struct device *dev, int pid)
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
    ipmi->pid = dev->id;

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
                                unsigned char *rx_data, unsigned short rx_len, unsigned char pid)
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
    dev_err(&data->pdev[pid]->dev, "request_timeout=%x\n", err);
    return err;
ipmi_req_err:
    dev_err(&data->pdev[pid]->dev, "request_settime=%x\n", err);
    return err;
addr_err:
    dev_err(&data->pdev[pid]->dev, "validate_addr=%x\n", err);
    return err;
}

/* Send an IPMI command with retry */
static int ipmi_send_message(struct ipmi_data *ipmi, unsigned char cmd,
                                unsigned char *tx_data, unsigned short tx_len,
                                unsigned char *rx_data, unsigned short rx_len)
{
    int status = 0, retry = 0;
    int pid = 0;
    struct device *dev;
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

    /* Get PSU device index */
    pid = (tx_data != NULL) ? tx_data[0] - 1 : 0;

    dev = &data->pdev[pid]->dev;

    for (retry = 0; retry <= IPMI_ERR_RETRY_TIMES; retry++) {
        status = _ipmi_send_message(ipmi, cmd, tx_data, tx_len, rx_data, rx_len, pid);
        if (unlikely(status != 0)) {
            dev_err(dev, "ipmi_send_message_%d err status(%d)[%s] raw_cmd=[%s]\r\n",
                        retry, status, cmdline ? cmdline : "", raw_cmd);
            continue;
        }

        if (unlikely(ipmi->rx_result != 0)) {
            dev_err(dev, "ipmi_send_message_%d err result(%d)[%s] raw_cmd=[%s]\r\n",
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
        dev_notice(&data->pdev[ipmi->pid]->dev, "Mismatch between received msgid "
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

static struct as9737_32db_psu_data *as9737_32db_psu_update_device(struct device_attribute *da)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    unsigned char pid = attr->index / NUM_OF_PER_PSU_ATTR;
    int status = 0;

    if (time_before(jiffies, data->last_updated[pid] + HZ * 5) && data->valid[pid])
        return data;

    data->valid[pid] = 0;
    /* To be compatible for older BMC firmware */
    data->ipmi_resp[pid].status[PSU_VOUT_MODE] = 0xff;

    /* Get status from ipmi */
    data->ipmi_tx_data[0] = pid + 1; /* PSU ID base id for ipmi start from 1 */
    status = ipmi_send_message(&data->ipmi, IPMI_PSU_READ_CMD,
                                data->ipmi_tx_data, 1,
                                data->ipmi_resp[pid].status,
                                sizeof(data->ipmi_resp[pid].status));
    if (unlikely(status != 0))
        goto exit;

    if (unlikely(data->ipmi.rx_result != 0)) {
        status = -EIO;
        goto exit;
    }

    /* Get model name from ipmi */
    data->ipmi_tx_data[1] = IPMI_PSU_MODEL_NAME_CMD;
    status = ipmi_send_message(&data->ipmi, IPMI_PSU_READ_CMD,
                                data->ipmi_tx_data, 2,
                                data->ipmi_resp[pid].model,
                                sizeof(data->ipmi_resp[pid].model) - 1);
    if (unlikely(status != 0))
        goto exit;

    if (unlikely(data->ipmi.rx_result != 0)) {
        status = -EIO;
        goto exit;
    }

    /* Get serial number from ipmi */
    data->ipmi_tx_data[1] = IPMI_PSU_SERIAL_NUM_CMD;
    status = ipmi_send_message(&data->ipmi, IPMI_PSU_READ_CMD,
                                data->ipmi_tx_data, 2,
                                data->ipmi_resp[pid].serial,
                                sizeof(data->ipmi_resp[pid].serial) - 1);
    if (unlikely(status != 0))
        goto exit;

    if (unlikely(data->ipmi.rx_result != 0)) {
        status = -EIO;
        goto exit;
    }

    /* Get fan direction from ipmi */
    data->ipmi_tx_data[1] = IPMI_PSU_FAN_DIR_CMD;
    status = ipmi_send_message(&data->ipmi, IPMI_PSU_READ_CMD,
                                data->ipmi_tx_data, 2,
                                data->ipmi_resp[pid].fandir,
                                sizeof(data->ipmi_resp[pid].fandir) - 1);
    if (unlikely(status != 0))
        goto exit;

    if (unlikely(data->ipmi.rx_result != 0)) {
        status = -EIO;
        goto exit;
    }

    /* Get capability from ipmi */
    data->ipmi_tx_data[1] = IPMI_PSU_INFO_CMD;
    status = ipmi_send_message(&data->ipmi, IPMI_PSU_READ_CMD,
                                data->ipmi_tx_data, 2,
                                data->ipmi_resp[pid].info,
                                sizeof(data->ipmi_resp[pid].info));
    if (unlikely(status != 0))
        goto exit;

    if (unlikely(data->ipmi.rx_result != 0)) {
        status = -EIO;
        goto exit;
    }

    data->last_updated[pid] = jiffies;
    data->valid[pid] = 1;

exit:
    return data;
}

#define VALIDATE_PRESENT_RETURN(id) \
do { \
    if (data->ipmi_resp[id].status[PSU_PRESENT] == 0) { \
        mutex_unlock(&data->update_lock);   \
        return -ENXIO; \
    } \
} while (0)

static ssize_t show_psu(struct device *dev, struct device_attribute *da,
                            char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    unsigned char pid = attr->index / NUM_OF_PER_PSU_ATTR;
    u32 value = 0;
    int present = 0;
    int error = 0;
    int multiplier = 1000;

    mutex_lock(&data->update_lock);

    data = as9737_32db_psu_update_device(da);
    if (!data->valid[pid]) {
        error = -EIO;
        goto exit;
    }

    present = !!(data->ipmi_resp[pid].status[PSU_PRESENT]);

    switch (attr->index) {
    case PSU1_PRESENT:
    case PSU2_PRESENT:
        value = present;
        break;
    case PSU1_POWER_GOOD:
    case PSU2_POWER_GOOD:
        VALIDATE_PRESENT_RETURN(pid);
        value = data->ipmi_resp[pid].status[PSU_POWER_GOOD_PMBUS];
        break;
    case PSU1_IIN:
    case PSU2_IIN:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].status[PSU_IIN0] |
                    (u32)data->ipmi_resp[pid].status[PSU_IIN1] << 8 |
                    (u32)data->ipmi_resp[pid].status[PSU_IIN2] << 16);
        break;
    case PSU1_IOUT:
    case PSU2_IOUT:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].status[PSU_IOUT0] |
                    (u32)data->ipmi_resp[pid].status[PSU_IOUT1] << 8 |
                    (u32)data->ipmi_resp[pid].status[PSU_IOUT2] << 16);
        break;
    case PSU1_VIN:
    case PSU2_VIN:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].status[PSU_VIN0] |
                    (u32)data->ipmi_resp[pid].status[PSU_VIN1] << 8 |
                    (u32)data->ipmi_resp[pid].status[PSU_VIN2] << 16);
        break;
    case PSU1_VOUT:
    case PSU2_VOUT:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].status[PSU_VOUT0] |
                    (u32)data->ipmi_resp[pid].status[PSU_VOUT1] << 8 |
                    (u32)data->ipmi_resp[pid].status[PSU_VOUT2] << 16);
        break;
    case PSU1_PIN:
    case PSU2_PIN:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].status[PSU_PIN0] |
                    (u32)data->ipmi_resp[pid].status[PSU_PIN1] << 8  |
                    (u32)data->ipmi_resp[pid].status[PSU_PIN2] << 16 |
                    (u32)data->ipmi_resp[pid].status[PSU_PIN3] << 24);
        value /= 1000; // Convert to milliwatt
        break;
    case PSU1_POUT:
    case PSU2_POUT:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].status[PSU_POUT0] |
                    (u32)data->ipmi_resp[pid].status[PSU_POUT1] << 8  |
                    (u32)data->ipmi_resp[pid].status[PSU_POUT2] << 16 |
                    (u32)data->ipmi_resp[pid].status[PSU_POUT3] << 24);
        value /= 1000; // Convert to milliwatt
        break;
    case PSU1_TEMP1_INPUT:
    case PSU2_TEMP1_INPUT:
        VALIDATE_PRESENT_RETURN(pid);
        value = (s16)((u16)data->ipmi_resp[pid].status[PSU_TEMP1_0] |
                      (u16)data->ipmi_resp[pid].status[PSU_TEMP1_1] << 8);
        value *= 1000; // Convert to millidegree Celsius
        break;
    case PSU1_TEMP2_INPUT:
    case PSU2_TEMP2_INPUT:
        VALIDATE_PRESENT_RETURN(pid);
        value = (s16)((u16)data->ipmi_resp[pid].status[PSU_TEMP2_0] |
                      (u16)data->ipmi_resp[pid].status[PSU_TEMP2_1] << 8);
        value *= 1000; // Convert to millidegree Celsius
        break;
    case PSU1_TEMP3_INPUT:
    case PSU2_TEMP3_INPUT:
        VALIDATE_PRESENT_RETURN(pid);
        value = (s16)((u16)data->ipmi_resp[pid].status[PSU_TEMP3_0] |
                      (u16)data->ipmi_resp[pid].status[PSU_TEMP3_1] << 8);
        value *= 1000; // Convert to millidegree Celsius
        break;
    case PSU1_FAN_INPUT:
    case PSU2_FAN_INPUT:
        VALIDATE_PRESENT_RETURN(pid);
        multiplier = 1;
        value = ((u32)data->ipmi_resp[pid].status[PSU_FAN0] |
                    (u32)data->ipmi_resp[pid].status[PSU_FAN1] << 8);
        break;
    default:
        error = -EINVAL;
        goto exit;
    }

    mutex_unlock(&data->update_lock);

    return sprintf(buf, "%d\n", present ? value : 0);

exit:
    mutex_unlock(&data->update_lock);
    return error;
}

static ssize_t show_psu_info(struct device *dev, struct device_attribute *da,
                            char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    unsigned char pid = attr->index / NUM_OF_PER_PSU_ATTR;
    s32 value = 0;
    int present = 0;
    int error = 0;

    mutex_lock(&data->update_lock);

    data = as9737_32db_psu_update_device(da);
    if (!data->valid[pid]) {
        error = -EIO;
        goto exit;
    }

    present = !!(data->ipmi_resp[pid].status[PSU_PRESENT]);

    switch (attr->index) {
    case PSU1_TEMP1_INPUT_MAX:
    case PSU2_TEMP1_INPUT_MAX:
        VALIDATE_PRESENT_RETURN(pid);
        value = (s16)((u16)data->ipmi_resp[pid].info[PSU_TEMP1_MAX0] |
                      (u16)data->ipmi_resp[pid].info[PSU_TEMP1_MAX1] << 8);
        value *= 1000; // Convert to millidegree Celsius
        break;
    case PSU1_TEMP1_INPUT_MIN:
    case PSU2_TEMP1_INPUT_MIN:
        VALIDATE_PRESENT_RETURN(pid);
        value = (s16)((u16)data->ipmi_resp[pid].info[PSU_TEMP1_MIN0] |
                      (u16)data->ipmi_resp[pid].info[PSU_TEMP1_MIN1] << 8);
        value *= 1000; // Convert to millidegree Celsius
        break;
    case PSU1_TEMP2_INPUT_MAX:
    case PSU2_TEMP2_INPUT_MAX:
        VALIDATE_PRESENT_RETURN(pid);
        value = (s16)((u16)data->ipmi_resp[pid].info[PSU_TEMP2_MAX0] |
                      (u16)data->ipmi_resp[pid].info[PSU_TEMP2_MAX1] << 8);
        value *= 1000; // Convert to millidegree Celsius
        break;
    case PSU1_TEMP2_INPUT_MIN:
    case PSU2_TEMP2_INPUT_MIN:
        VALIDATE_PRESENT_RETURN(pid);
        value = (s16)((u16)data->ipmi_resp[pid].info[PSU_TEMP2_MIN0] |
                      (u16)data->ipmi_resp[pid].info[PSU_TEMP2_MIN1] << 8);
        value *= 1000; // Convert to millidegree Celsius
        break;
    case PSU1_TEMP3_INPUT_MAX:
    case PSU2_TEMP3_INPUT_MAX:
        VALIDATE_PRESENT_RETURN(pid);
        value = (s16)((u16)data->ipmi_resp[pid].info[PSU_TEMP3_MAX0] |
                      (u16)data->ipmi_resp[pid].info[PSU_TEMP3_MAX1] << 8);
        value *= 1000; // Convert to millidegree Celsius
        break;
    case PSU1_TEMP3_INPUT_MIN:
    case PSU2_TEMP3_INPUT_MIN:
        VALIDATE_PRESENT_RETURN(pid);
        value = (s16)((u16)data->ipmi_resp[pid].info[PSU_TEMP3_MIN0] |
                      (u16)data->ipmi_resp[pid].info[PSU_TEMP3_MIN1] << 8);
        value *= 1000; // Convert to millidegree Celsius
        break;
    case PSU1_VIN_UPPER_CRIT:
    case PSU2_VIN_UPPER_CRIT:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].info[PSU_VIN_UPPER_CRIT0] |
                (u32)data->ipmi_resp[pid].info[PSU_VIN_UPPER_CRIT1] << 8 |
                (u32)data->ipmi_resp[pid].info[PSU_VIN_UPPER_CRIT2] << 16);
        break;
    case PSU1_VIN_LOWER_CRIT:
    case PSU2_VIN_LOWER_CRIT:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].info[PSU_VIN_LOWER_CRIT0] |
                (u32)data->ipmi_resp[pid].info[PSU_VIN_LOWER_CRIT1] << 8 |
                (u32)data->ipmi_resp[pid].info[PSU_VIN_LOWER_CRIT2] << 16);
        break;
    case PSU1_VIN_MAX:
    case PSU2_VIN_MAX:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].info[PSU_VIN_MAX0] |
                (u32)data->ipmi_resp[pid].info[PSU_VIN_MAX1] << 8 |
                (u32)data->ipmi_resp[pid].info[PSU_VIN_MAX2] << 16);
        break;
    case PSU1_VIN_MIN:
    case PSU2_VIN_MIN:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].info[PSU_VIN_MIN0] |
                (u32)data->ipmi_resp[pid].info[PSU_VIN_MIN1] << 8 |
                (u32)data->ipmi_resp[pid].info[PSU_VIN_MIN2] << 16);
        break;
    case PSU1_VOUT_MAX:
    case PSU2_VOUT_MAX:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].info[PSU_VOUT_MAX0] |
                (u32)data->ipmi_resp[pid].info[PSU_VOUT_MAX1] << 8 |
                (u32)data->ipmi_resp[pid].info[PSU_VOUT_MAX2] << 16);
        break;
    case PSU1_VOUT_MIN:
    case PSU2_VOUT_MIN:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].info[PSU_VOUT_MIN0] |
                (u32)data->ipmi_resp[pid].info[PSU_VOUT_MIN1] << 8 |
                (u32)data->ipmi_resp[pid].info[PSU_VOUT_MIN2] << 16);
        break;
    case PSU1_IIN_MAX:
    case PSU2_IIN_MAX:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].info[PSU_IIN_MAX0] |
                (u32)data->ipmi_resp[pid].info[PSU_IIN_MAX1] << 8 |
                (u32)data->ipmi_resp[pid].info[PSU_IIN_MAX2] << 16);
        break;
    case PSU1_IOUT_MAX:
    case PSU2_IOUT_MAX:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].info[PSU_IOUT_MAX0] |
                (u32)data->ipmi_resp[pid].info[PSU_IOUT_MAX1] << 8 |
                (u32)data->ipmi_resp[pid].info[PSU_IOUT_MAX2] << 16);
        break;
    case PSU1_PIN_MAX:
    case PSU2_PIN_MAX:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].info[PSU_PIN_MAX0] |
                (u32)data->ipmi_resp[pid].info[PSU_PIN_MAX1] << 8  |
                (u32)data->ipmi_resp[pid].info[PSU_PIN_MAX2] << 16 |
                (u32)data->ipmi_resp[pid].info[PSU_PIN_MAX3] << 24);
        value /= 1000; // Convert to milliwatt
        break;
    case PSU1_POUT_MAX:
    case PSU2_POUT_MAX:
        VALIDATE_PRESENT_RETURN(pid);
        value = ((u32)data->ipmi_resp[pid].info[PSU_POUT_MAX0] |
                (u32)data->ipmi_resp[pid].info[PSU_POUT_MAX1] << 8  |
                (u32)data->ipmi_resp[pid].info[PSU_POUT_MAX2] << 16 |
                (u32)data->ipmi_resp[pid].info[PSU_POUT_MAX3] << 24);
        value /= 1000; // Convert to milliwatt
        break;
    default:
        error = -EINVAL;
        goto exit;
    }

    mutex_unlock(&data->update_lock);

    return sprintf(buf, "%d\n", present ? value : 0);

exit:
    mutex_unlock(&data->update_lock);
    return error;
}

static ssize_t show_string(struct device *dev, struct device_attribute *da,
                                char *buf)
{
    struct sensor_device_attribute *attr = to_sensor_dev_attr(da);
    unsigned char pid = attr->index / NUM_OF_PER_PSU_ATTR;
    char *str = NULL;
    int error = 0;

    mutex_lock(&data->update_lock);

    data = as9737_32db_psu_update_device(da);
    if (!data->valid[pid]) {
        error = -EIO;
        goto exit;
    }

    switch (attr->index) {
    case PSU1_MODEL:
    case PSU2_MODEL:
        VALIDATE_PRESENT_RETURN(pid);
        str = data->ipmi_resp[pid].model;
        break;
    case PSU1_SERIAL:
    case PSU2_SERIAL:
        VALIDATE_PRESENT_RETURN(pid);
        str = data->ipmi_resp[pid].serial;
        break;
    case PSU1_FAN_DIR:
    case PSU2_FAN_DIR:
        VALIDATE_PRESENT_RETURN(pid);
        str = data->ipmi_resp[pid].fandir;
        break;
    default:
        error = -EINVAL;
        goto exit;
    }

    mutex_unlock(&data->update_lock);
    return sprintf(buf, "%s\n", str);

exit:
    mutex_unlock(&data->update_lock);
    return error;
}

static int as9737_32db_psu_probe(struct platform_device *pdev)
{
    int status = 0;
    struct device *hwmon_dev = NULL;

    hwmon_dev = hwmon_device_register_with_groups(&pdev->dev, DRVNAME, 
                    NULL, as9737_32db_psu_groups[pdev->id]);
    if (IS_ERR(hwmon_dev)) {
        status = PTR_ERR(hwmon_dev);
        return status;
    }

    mutex_lock(&data->update_lock);
    data->hwmon_dev[pdev->id] = hwmon_dev;
    mutex_unlock(&data->update_lock);

    dev_info(&pdev->dev, "PSU%d device created\n", pdev->id + 1);

    return 0;
}

static int as9737_32db_psu_remove(struct platform_device *pdev)
{
    mutex_lock(&data->update_lock);
    if (data->hwmon_dev[pdev->id]) {
        hwmon_device_unregister(data->hwmon_dev[pdev->id]);
        data->hwmon_dev[pdev->id] = NULL;
    }
    mutex_unlock(&data->update_lock);

    return 0;
}

static int __init as9737_32db_psu_init(void)
{
    int ret;
    int i;

    data = kzalloc(sizeof(struct as9737_32db_psu_data), GFP_KERNEL);
    if (!data) {
        ret = -ENOMEM;
        goto alloc_err;
    }

    mutex_init(&data->update_lock);

    ret = platform_driver_register(&as9737_32db_psu_driver);
    if (ret < 0)
        goto dri_reg_err;

    for (i = 0; i < NUM_OF_PSU; i++) {
        data->pdev[i] = platform_device_register_simple(DRVNAME, i, NULL, 0);
        if (IS_ERR(data->pdev[i])) {
            ret = PTR_ERR(data->pdev[i]);
            goto dev_reg_err;
        }

        /* Set up IPMI interface */
        ret = init_ipmi_data(&data->ipmi, 0, &data->pdev[i]->dev, i);
        if (ret) {
            goto ipmi_err;
        }
    }


    return 0;

ipmi_err:
    while (i > 0) {
        i--;
        platform_device_unregister(data->pdev[i]);
    }
dev_reg_err:
    platform_driver_unregister(&as9737_32db_psu_driver);
dri_reg_err:
    kfree(data);
alloc_err:
    return ret;
}

static void __exit as9737_32db_psu_exit(void)
{
    int i;

    ipmi_destroy_user(data->ipmi.user);
    for (i = 0; i < NUM_OF_PSU; i++) {
        platform_device_unregister(data->pdev[i]);
    }
    platform_driver_unregister(&as9737_32db_psu_driver);
    kfree(data);
}

MODULE_AUTHOR("Roger Ho <roger530_ho@edge-core.com>");
MODULE_DESCRIPTION("as9737_32db_psu driver");
MODULE_LICENSE("GPL");

module_init(as9737_32db_psu_init);
module_exit(as9737_32db_psu_exit);
