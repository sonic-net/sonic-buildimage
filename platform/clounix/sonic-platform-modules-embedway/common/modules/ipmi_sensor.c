// SPDX-License-Identifier: GPL-2.0+
/*
 * Hardware monitoring driver for Renesas Digital Multiphase Voltage Regulators
 *
 * Copyright (c) 2017 Google Inc
 * Copyright (c) 2020 Renesas Electronics America
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ipmi.h>
#include <linux/ipmi_smi.h>
#include <linux/slab.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/i2c.h>
#include "ipmi_def.h"

#define MULT_VALUE 1000
#define I2C_BUS_RAA 25
#define I2C_ADDR_RAA 0x60
#define I2C_BUS_XDPE1 26
#define I2C_ADDR_XDPE 0x5e
#define I2C_BUS_XDPE2 27

static int log_level = LOG_INFO;
module_param_named(loglevel, log_level, int, 0640);
MODULE_PARM_DESC(loglevel, "driver log level, 0:off 1:err 2:info 3:debug.");

typedef struct {
    u8 id;
    int m;
    int b;
    int k1;
    int k2;
    char name[GEN_NAME_SIZE];
} SENSOR_TABLE;

typedef struct {
    u8 *w_data;
    u8 w_data_len;
    u8 *r_data;
    u8 *r_data_len;
    u8 read_byte;
    u8 netF;
    u8 cmd;
} common_req_data;

static SENSOR_TABLE g_sensor_table[] = {
    {.id=51, .m=6,   .b=0,   .k1=0,  .k2=-3,  .name = "raa_vin1"},
    {.id=52, .m=3,   .b=0,   .k1=0,  .k2=-3,  .name = "raa_vout1"},
    {.id=53, .m=10,  .b=0,   .k1=0,  .k2=-2,  .name = "raa_iin1"},
    {.id=54, .m=1,   .b=0,   .k1=0,  .k2=-1,  .name = "raa_iout1"},
    {.id=55, .m=1,   .b=0,   .k1=0,  .k2=0,   .name = "raa_temp1"},
    {.id=56, .m=1,   .b=0,   .k1=0,  .k2=0,   .name = "raa_temp2"},
    {.id=57, .m=60,  .b=0,   .k1=0,  .k2=-3,  .name = "xdpe1_vin1"},
    {.id=58, .m=60,  .b=0,   .k1=0,  .k2=-3,  .name = "xdpe1_vin2"},
    {.id=59, .m=16,  .b=0,   .k1=0,  .k2=-3,  .name = "xdpe1_vout1"},
    {.id=60, .m=3,   .b=0,   .k1=0,  .k2=-3,  .name = "xdpe1_vout2"},
    {.id=61, .m=20,  .b=0,   .k1=0,  .k2=-2,  .name = "xdpe1_iin1"},
    {.id=62, .m=20,  .b=0,   .k1=0,  .k2=-2,  .name = "xdpe1_iin2"},
    {.id=63, .m=10,  .b=0,   .k1=0,  .k2=-2,  .name = "xdpe1_iout1"},
    {.id=64, .m=3,   .b=0,   .k1=0,  .k2=-2,  .name = "xdpe1_iout2"},
    {.id=65, .m=1,   .b=0,   .k1=0,  .k2=0,   .name = "xdpe1_temp1"},
    {.id=66, .m=1,   .b=0,   .k1=0,  .k2=0,   .name = "xdpe1_temp2"},
    {.id=67, .m=60,  .b=0,   .k1=0,  .k2=-3,  .name = "xdpe2_vin1"},
    {.id=68, .m=60,  .b=0,   .k1=0,  .k2=-3,  .name = "xdpe2_vin2"},
    {.id=69, .m=16,  .b=0,   .k1=0,  .k2=-3,  .name = "xdpe2_vout1"},
    {.id=70, .m=6,   .b=0,   .k1=0,  .k2=-3,  .name = "xdpe2_vout2"},
    {.id=71, .m=20,  .b=0,   .k1=0,  .k2=-2,  .name = "xdpe2_iin1"},
    {.id=72, .m=20,  .b=0,   .k1=0,  .k2=-2,  .name = "xdpe2_iin2"},
    {.id=73, .m=10,  .b=0,   .k1=0,  .k2=-2,  .name = "xdpe2_iout1"},
    {.id=74, .m=3,   .b=0,   .k1=0,  .k2=-2,  .name = "xdpe2_iout2"},
    {.id=75, .m=1,   .b=0,   .k1=0,  .k2=0,   .name = "xdpe2_temp1"},
    {.id=76, .m=1,   .b=0,   .k1=0,  .k2=0,   .name = "xdpe2_temp2"}
};

typedef enum {
    idx_raa_in1=0,
    idx_raa_in3,
    idx_raa_curr1,
    idx_raa_curr3,
    idx_raa_temp1,
    idx_raa_temp2,
    idx_xdpe1_in1,
    idx_xdpe1_in2,
    idx_xdpe1_in3,
    idx_xdpe1_in4,
    idx_xdpe1_curr1,
    idx_xdpe1_curr2,
    idx_xdpe1_curr3,
    idx_xdpe1_curr4,
    idx_xdpe1_temp1,
    idx_xdpe1_temp2,
    idx_xdpe2_in1,
    idx_xdpe2_in2,
    idx_xdpe2_in3,
    idx_xdpe2_in4,
    idx_xdpe2_curr1,
    idx_xdpe2_curr2,
    idx_xdpe2_curr3,
    idx_xdpe2_curr4,
    idx_xdpe2_temp1,
    idx_xdpe2_temp2
} sensor_type_idx;

static struct ipmi_user *ipmi_i2c_user = NULL;
static int mbk_updated = 0;
#if 0 //remove for confit with math.h
static long int_pow(int base, int exp)
{
	long result = 1;

	while (exp) {
		if (exp & 1)
			result *= base;
		exp >>= 1;
		base *= base;
	}

	return result;
}
#endif

/************************* IPMI 通信部分 **************************/
static int ipmi_common_req_wait(common_req_data *req)
{
	recv_user_data user_data;
	int ret;
	int i = 0;

    struct ipmi_system_interface_addr addr = {
        .addr_type = IPMI_SYSTEM_INTERFACE_ADDR_TYPE,
        .channel = IPMI_BMC_CHANNEL,
        .lun = 0
    };
    
    struct kernel_ipmi_msg msg = {
        .netfn = req->netF,
        .cmd = req->cmd,
        .data = req->w_data,
        .data_len = req->w_data_len
    };

    if (req->w_data == NULL || req->r_data == NULL || req->r_data_len == NULL) {
        log_err(IPMI_SEN, "ipmi_sensor_req_wait invalid param\n");
		return -EINVAL;
    }

	log_dbg(IPMI_SEN, "ipmi TX:");
    if (log_level >= LOG_DEBUG) {
        for (i=0;i< req->w_data_len;i++) {
            log_dbg_cont("%02x ",req->w_data[i]);
        }
    }
	log_dbg_cont("\n");

    user_data.data = req->r_data;
    user_data.read_byte = req->read_byte;
    init_completion(&user_data.comp);
    ret = ipmi_request_supply_msgs(ipmi_i2c_user,
                                  (struct ipmi_addr *)&addr,
                                  0,
                                  &msg,
                                  &user_data,
                                  NULL,
                                  NULL,
                                  0);
	if (ret) {
		return ret;
	}
	wait_for_completion(&user_data.comp);
	if (user_data.recv_len < 0) {
		return -EIO;
	} else {
		*(req->r_data_len) = (u8)user_data.recv_len;
	}

	log_dbg(IPMI_SEN, "ipmi RX:");
    if (log_level >= LOG_DEBUG) {
        for (i=0;i< *req->r_data_len;i++) {
            log_dbg_cont("%02x ",req->r_data[i]);
        }
    }
	log_dbg_cont("\n\n");
	
	return ret;
}

static int ipmi_sensor_req_wait(u8 *w_data, u8 w_data_len, u8 *r_data, u8 *r_data_len)
{
	common_req_data req = {
        .w_data = w_data,
        .w_data_len = w_data_len,
        .r_data = r_data,
        .r_data_len = r_data_len,
        .netF = IPMI_NETFN_SENSOR_EVENT,
        .cmd = IPMI_GET_SENSOR_READING_CMD,
        .read_byte = 1
    };
    return ipmi_common_req_wait(&req);
}

static int ipmi_threshold_req_wait(u8 *w_data, u8 w_data_len, u8 *r_data, u8 *r_data_len)
{
	common_req_data req = {
        .w_data = w_data,
        .w_data_len = w_data_len,
        .r_data = r_data,
        .r_data_len = r_data_len,
        .netF = IPMI_NETFN_SENSOR_EVENT,
        .cmd = IPMI_GET_SENSOR_THRESHOLDS_CMD,
        .read_byte = 7
    };
    return ipmi_common_req_wait(&req);
}

static int ipmi_reserve_sdr_req_wait(u8 *w_data, u8 w_data_len, u8 *r_data, u8 *r_data_len)
{
	common_req_data req = {
        .w_data = w_data,
        .w_data_len = w_data_len,
        .r_data = r_data,
        .r_data_len = r_data_len,
        .netF = IPMI_NETFN_STORAGE,
        .cmd = IPMI_RESERVE_SDR_REP_CMD,
        .read_byte = 2
    };
    return ipmi_common_req_wait(&req);
}

static int ipmi_get_sdr_req_wait(u8 *w_data, u8 w_data_len, u8 *r_data, u8 *r_data_len)
{
	common_req_data req = {
        .w_data = w_data,
        .w_data_len = w_data_len,
        .r_data = r_data,
        .r_data_len = r_data_len,
        .netF = IPMI_NETFN_STORAGE,
        .cmd = IPMI_GET_SDR_CMD,
        .read_byte = 8
    };
    return ipmi_common_req_wait(&req);
}

static void ipmi_raw_convert(SENSOR_TABLE *item, u8 raw, long *data)
{
    long result = 0,offset;
    long multiplier = MULT_VALUE;
    int abs_exp;

    /*
    result = (double) (    ((m * val) + (b * pow(10, k1)))      * pow(10, k2)    );
    */
    result = (long)item->m * (long)raw * multiplier;

    offset = item->b * multiplier;
    if (offset != 0) {
        if (item->k1 >= 0) {
            abs_exp = item->k1;
            offset *= int_pow(10, abs_exp);
        } else {
            abs_exp = -item->k1;
            offset /= int_pow(10, abs_exp);
        }
    }
    result += offset;

    if (item->k2 >= 0) {
        abs_exp = item->k2;
        result *= int_pow(10, abs_exp);
    } else {
        abs_exp = -item->k2;
        result /= int_pow(10, abs_exp);
    }

    *data = result;

    log_dbg(IPMI_SEN, "sensor_id:%d,raw:%d,m:%d,b:%d,k1:%d,k2:%d,result:%ld\n",\
        item->id, raw, item->m,item->b,item->k1,item->k2,result);

    return;
}

static ssize_t _sensor_reading(char *buf, int tab_idx)
{
    int ret;
	SENSOR_TABLE *item = &g_sensor_table[tab_idx];
	u8 r_len,raw;
    long sensor_data;

    ret = ipmi_sensor_req_wait(&item->id, 1, &raw, &r_len);
    if (ret) {
        log_err(IPMI_SEN, "ipmi_sensor_req_wait failed\n");
        return -1;
    }
    if (r_len == 1) {
        ipmi_raw_convert(item, raw, &sensor_data);
    }
    if (tab_idx == idx_xdpe1_in3 || tab_idx == idx_xdpe2_in3) {
        //revert to 1/2 voltage divider to keep the same range with Non-BMC platform
        sensor_data = (sensor_data >> 1);
    }

    return sprintf(buf, "%ld\n", sensor_data);
}

#define U_CRIT_IDX 5
#define U_CRIT_BIT_VAL 0x10
#define U_NON_CRIT_IDX 4
#define U_NON_CRIT_BIT_VAL 0x8
static ssize_t _sensor_crit(char *buf, int tab_idx)
{
    int ret;
	SENSOR_TABLE *item = &g_sensor_table[tab_idx];
	u8 r_len, raw[7];
    long sensor_data;

    ret = ipmi_threshold_req_wait(&item->id, 1, raw, &r_len);
    if (ret) {
        log_err(IPMI_SEN, "ipmi_sensor_req_wait failed\n");
        return -1;
    }
    if (r_len == 7) {
        if (raw[0] & U_CRIT_BIT_VAL) {
            ipmi_raw_convert(item, raw[U_CRIT_IDX], &sensor_data);
            return sprintf(buf, "%ld\n", sensor_data);
        }
    }

    return sprintf(buf, "%s\n", "NA");
}

static ssize_t _sensor_non_crit(char *buf, int tab_idx)
{
    int ret;
	SENSOR_TABLE *item = &g_sensor_table[tab_idx];
	u8 r_len, raw[7];
    long sensor_data;

    ret = ipmi_threshold_req_wait(&item->id, 1, raw, &r_len);
    if (ret) {
        log_err(IPMI_SEN, "ipmi_sensor_req_wait failed\n");
        return -1;
    }
    if (r_len == 7) {
        if (raw[0] & U_NON_CRIT_BIT_VAL) {
            ipmi_raw_convert(item, raw[U_NON_CRIT_IDX], &sensor_data);
            return sprintf(buf, "%ld\n", sensor_data);
        }
    }

    return sprintf(buf, "%s\n", "NA");
}

/************************* sysfs 接口部分 **************************/
#define INPUT_SHOW(dev, chan) \
static ssize_t dev##_##chan##_##input_show(struct kobject *kobj, \
                        struct kobj_attribute *attr,\
                        char *buf)\
{ \
	return _sensor_reading(buf, idx##_##dev##_##chan); \
} \
static struct kobj_attribute dev##_##chan##_##input_attr = __ATTR(chan##_##input, 0444, dev##_##chan##_##input_show, NULL);

#define LABLE_SHOW(dev, chan) \
static ssize_t dev##_##chan##_##lable_show(struct kobject *kobj, \
                        struct kobj_attribute *attr,\
                        char *buf)\
{ \
	return sprintf(buf, "%s\n", g_sensor_table[idx##_##dev##_##chan].name); \
} \
static struct kobj_attribute dev##_##chan##_##lable_attr = __ATTR(chan##_##lable, 0444, dev##_##chan##_##lable_show, NULL);

#define CRIT_SHOW(dev, chan) \
static ssize_t dev##_##chan##_##crit_show(struct kobject *kobj, \
                        struct kobj_attribute *attr,\
                        char *buf)\
{ \
	return _sensor_crit(buf, idx##_##dev##_##chan); \
} \
static struct kobj_attribute dev##_##chan##_##crit_attr = __ATTR(chan##_##crit, 0444, dev##_##chan##_##crit_show, NULL);

#define MAX_SHOW(dev, chan) \
static ssize_t dev##_##chan##_##max_show(struct kobject *kobj, \
                        struct kobj_attribute *attr,\
                        char *buf)\
{ \
	return _sensor_non_crit(buf, idx##_##dev##_##chan); \
} \
static struct kobj_attribute dev##_##chan##_##max_attr = __ATTR(chan##_##max, 0444, dev##_##chan##_##max_show, NULL);


//raa start
INPUT_SHOW(raa,in1)
LABLE_SHOW(raa,in1)
CRIT_SHOW(raa,in1)
MAX_SHOW(raa,in1)
INPUT_SHOW(raa,in3)
LABLE_SHOW(raa,in3)
CRIT_SHOW(raa,in3)
MAX_SHOW(raa,in3)
INPUT_SHOW(raa,curr1)
LABLE_SHOW(raa,curr1)
CRIT_SHOW(raa,curr1)
MAX_SHOW(raa,curr1)
INPUT_SHOW(raa,curr3)
LABLE_SHOW(raa,curr3)
CRIT_SHOW(raa,curr3)
MAX_SHOW(raa,curr3)
INPUT_SHOW(raa,temp1)
CRIT_SHOW(raa,temp1)
MAX_SHOW(raa,temp1)
INPUT_SHOW(raa,temp2)
CRIT_SHOW(raa,temp2)
MAX_SHOW(raa,temp2)
static struct attribute *raa_node_attrs[] = {
    &raa_in1_input_attr.attr,
    &raa_in1_lable_attr.attr,
    &raa_in1_crit_attr.attr,
    &raa_in1_max_attr.attr,
    &raa_in3_input_attr.attr,
    &raa_in3_lable_attr.attr,
    &raa_in3_crit_attr.attr,
    &raa_in3_max_attr.attr,
    &raa_curr1_input_attr.attr,
    &raa_curr1_lable_attr.attr,
    &raa_curr1_crit_attr.attr,
    &raa_curr1_max_attr.attr,
    &raa_curr3_input_attr.attr,
    &raa_curr3_lable_attr.attr,
    &raa_curr3_crit_attr.attr,
    &raa_curr3_max_attr.attr,
    &raa_temp1_input_attr.attr,
    &raa_temp1_crit_attr.attr,
    &raa_temp1_max_attr.attr,
    &raa_temp2_input_attr.attr,
    &raa_temp2_crit_attr.attr,
    &raa_temp2_max_attr.attr,
    NULL,
};
static struct attribute_group raa_node_attr_group = {
    .attrs = raa_node_attrs,
};
static struct kobject *raa_hwmon,*raa_hwmonN;
//raa end

//xdpe1 start
INPUT_SHOW(xdpe1,in1)
LABLE_SHOW(xdpe1,in1)
CRIT_SHOW(xdpe1,in1)
MAX_SHOW(xdpe1,in1)
INPUT_SHOW(xdpe1,in2)
LABLE_SHOW(xdpe1,in2)
CRIT_SHOW(xdpe1,in2)
MAX_SHOW(xdpe1,in2)
INPUT_SHOW(xdpe1,in3)
LABLE_SHOW(xdpe1,in3)
CRIT_SHOW(xdpe1,in3)
MAX_SHOW(xdpe1,in3)
INPUT_SHOW(xdpe1,in4)
LABLE_SHOW(xdpe1,in4)
CRIT_SHOW(xdpe1,in4)
MAX_SHOW(xdpe1,in4)
INPUT_SHOW(xdpe1,curr1)
LABLE_SHOW(xdpe1,curr1)
CRIT_SHOW(xdpe1,curr1)
MAX_SHOW(xdpe1,curr1)
INPUT_SHOW(xdpe1,curr2)
LABLE_SHOW(xdpe1,curr2)
CRIT_SHOW(xdpe1,curr2)
MAX_SHOW(xdpe1,curr2)
INPUT_SHOW(xdpe1,curr3)
LABLE_SHOW(xdpe1,curr3)
CRIT_SHOW(xdpe1,curr3)
MAX_SHOW(xdpe1,curr3)
INPUT_SHOW(xdpe1,curr4)
LABLE_SHOW(xdpe1,curr4)
CRIT_SHOW(xdpe1,curr4)
MAX_SHOW(xdpe1,curr4)
INPUT_SHOW(xdpe1,temp1)
CRIT_SHOW(xdpe1,temp1)
MAX_SHOW(xdpe1,temp1)
INPUT_SHOW(xdpe1,temp2)
CRIT_SHOW(xdpe1,temp2)
MAX_SHOW(xdpe1,temp2)
static struct attribute *xdpe1_node_attrs[] = {
    &xdpe1_in1_input_attr.attr,
    &xdpe1_in1_lable_attr.attr,
    &xdpe1_in1_crit_attr.attr,
    &xdpe1_in1_max_attr.attr,
    &xdpe1_in2_input_attr.attr,
    &xdpe1_in2_lable_attr.attr,
    &xdpe1_in2_crit_attr.attr,
    &xdpe1_in2_max_attr.attr,
    &xdpe1_in3_input_attr.attr,
    &xdpe1_in3_lable_attr.attr,
    &xdpe1_in3_crit_attr.attr,
    &xdpe1_in3_max_attr.attr,
    &xdpe1_in4_input_attr.attr,
    &xdpe1_in4_lable_attr.attr,
    &xdpe1_in4_crit_attr.attr,
    &xdpe1_in4_max_attr.attr,
    &xdpe1_curr1_input_attr.attr,
    &xdpe1_curr1_lable_attr.attr,
    &xdpe1_curr1_crit_attr.attr,
    &xdpe1_curr1_max_attr.attr,
    &xdpe1_curr2_input_attr.attr,
    &xdpe1_curr2_lable_attr.attr,
    &xdpe1_curr2_crit_attr.attr,
    &xdpe1_curr2_max_attr.attr,
    &xdpe1_curr3_input_attr.attr,
    &xdpe1_curr3_lable_attr.attr,
    &xdpe1_curr3_crit_attr.attr,
    &xdpe1_curr3_max_attr.attr,
    &xdpe1_curr4_input_attr.attr,
    &xdpe1_curr4_lable_attr.attr,
    &xdpe1_curr4_crit_attr.attr,
    &xdpe1_curr4_max_attr.attr,
    &xdpe1_temp1_input_attr.attr,
    &xdpe1_temp1_crit_attr.attr,
    &xdpe1_temp1_max_attr.attr,
    &xdpe1_temp2_input_attr.attr,
    &xdpe1_temp2_crit_attr.attr,
    &xdpe1_temp2_max_attr.attr,
    NULL,
};
static struct attribute_group xdpe1_node_attr_group = {
    .attrs = xdpe1_node_attrs,
};
static struct kobject *xdpe1_hwmon,*xdpe1_hwmonN;
//xdpe1 end

//xdpe2 start
INPUT_SHOW(xdpe2,in1)
LABLE_SHOW(xdpe2,in1)
CRIT_SHOW(xdpe2,in1)
MAX_SHOW(xdpe2,in1)
INPUT_SHOW(xdpe2,in2)
LABLE_SHOW(xdpe2,in2)
CRIT_SHOW(xdpe2,in2)
MAX_SHOW(xdpe2,in2)
INPUT_SHOW(xdpe2,in3)
LABLE_SHOW(xdpe2,in3)
CRIT_SHOW(xdpe2,in3)
MAX_SHOW(xdpe2,in3)
INPUT_SHOW(xdpe2,in4)
LABLE_SHOW(xdpe2,in4)
CRIT_SHOW(xdpe2,in4)
MAX_SHOW(xdpe2,in4)
INPUT_SHOW(xdpe2,curr1)
LABLE_SHOW(xdpe2,curr1)
CRIT_SHOW(xdpe2,curr1)
MAX_SHOW(xdpe2,curr1)
INPUT_SHOW(xdpe2,curr2)
LABLE_SHOW(xdpe2,curr2)
CRIT_SHOW(xdpe2,curr2)
MAX_SHOW(xdpe2,curr2)
INPUT_SHOW(xdpe2,curr3)
LABLE_SHOW(xdpe2,curr3)
CRIT_SHOW(xdpe2,curr3)
MAX_SHOW(xdpe2,curr3)
INPUT_SHOW(xdpe2,curr4)
LABLE_SHOW(xdpe2,curr4)
CRIT_SHOW(xdpe2,curr4)
MAX_SHOW(xdpe2,curr4)
INPUT_SHOW(xdpe2,temp1)
CRIT_SHOW(xdpe2,temp1)
MAX_SHOW(xdpe2,temp1)
INPUT_SHOW(xdpe2,temp2)
CRIT_SHOW(xdpe2,temp2)
MAX_SHOW(xdpe2,temp2)
static struct attribute *xdpe2_node_attrs[] = {
    &xdpe2_in1_input_attr.attr,
    &xdpe2_in1_lable_attr.attr,
    &xdpe2_in1_crit_attr.attr,
    &xdpe2_in1_max_attr.attr,
    &xdpe2_in2_input_attr.attr,
    &xdpe2_in2_lable_attr.attr,
    &xdpe2_in2_crit_attr.attr,
    &xdpe2_in2_max_attr.attr,
    &xdpe2_in3_input_attr.attr,
    &xdpe2_in3_lable_attr.attr,
    &xdpe2_in3_crit_attr.attr,
    &xdpe2_in3_max_attr.attr,
    &xdpe2_in4_input_attr.attr,
    &xdpe2_in4_lable_attr.attr,
    &xdpe2_in4_crit_attr.attr,
    &xdpe2_in4_max_attr.attr,
    &xdpe2_curr1_input_attr.attr,
    &xdpe2_curr1_lable_attr.attr,
    &xdpe2_curr1_crit_attr.attr,
    &xdpe2_curr1_max_attr.attr,
    &xdpe2_curr2_input_attr.attr,
    &xdpe2_curr2_lable_attr.attr,
    &xdpe2_curr2_crit_attr.attr,
    &xdpe2_curr2_max_attr.attr,
    &xdpe2_curr3_input_attr.attr,
    &xdpe2_curr3_lable_attr.attr,
    &xdpe2_curr3_crit_attr.attr,
    &xdpe2_curr3_max_attr.attr,
    &xdpe2_curr4_input_attr.attr,
    &xdpe2_curr4_lable_attr.attr,
    &xdpe2_curr4_crit_attr.attr,
    &xdpe2_curr4_max_attr.attr,
    &xdpe2_temp1_input_attr.attr,
    &xdpe2_temp1_crit_attr.attr,
    &xdpe2_temp1_max_attr.attr,
    &xdpe2_temp2_input_attr.attr,
    &xdpe2_temp2_crit_attr.attr,
    &xdpe2_temp2_max_attr.attr,
    NULL,
};
static struct attribute_group xdpe2_node_attr_group = {
    .attrs = xdpe2_node_attrs,
};
static struct kobject *xdpe2_hwmon,*xdpe2_hwmonN;
//xdpe2 end

# define tos32(val, bits)    ((val & ((1<<((bits)-1)))) ? (-((val) & (1<<((bits)-1))) | (val)) : (val))

# define BSWAP_16(x) ((((x) & 0xff00) >> 8) | (((x) & 0x00ff) << 8))
# define BSWAP_32(x) ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >> 8) |\
                     (((x) & 0x0000ff00) << 8) | (((x) & 0x000000ff) << 24))

# define __TO_M(mtol)       (int16_t)(tos32((((BSWAP_16(mtol) & 0xff00) >> 8) | ((BSWAP_16(mtol) & 0xc0) << 2)), 10))
# define __TO_B(bacc)       (int32_t)(tos32((((BSWAP_32(bacc) & 0xff000000) >> 24) | \
                            ((BSWAP_32(bacc) & 0xc00000) >> 14)), 10))
# define __TO_R_EXP(bacc)   (int32_t)(tos32(((BSWAP_32(bacc) & 0xf0) >> 4), 4))
# define __TO_B_EXP(bacc)   (int32_t)(tos32((BSWAP_32(bacc) & 0xf), 4))
static int update_mbk_param(void)
{
    int ret,i;
	u8 r_len, reserve_id[2], sdr_req[6], sdr_rsp[8];
    SENSOR_TABLE *item;
    u16 *mtol;
    u32 *bacc;

    ret = ipmi_reserve_sdr_req_wait(reserve_id, 0, reserve_id, &r_len);
    if (ret) {
        log_err(IPMI_SEN, "ipmi_reserve_sdr_req_wait failed\n");
        return -1;
    }
    log_info(IPMI_SEN, "reserve ID:%02x %02x\n", reserve_id[0], reserve_id[1]);

    memset(sdr_req, 0 , sizeof(sdr_req));
    memset(sdr_rsp, 0 , sizeof(sdr_rsp));
    sdr_req[0] = reserve_id[0];
    sdr_req[1] = reserve_id[1];
    sdr_req[4] = 24;
    sdr_req[5] = 6;
    for (i = 0; i < ARRAY_SIZE(g_sensor_table); i++) {
        item = &g_sensor_table[i];
        sdr_req[2] = item->id;
        ret= ipmi_get_sdr_req_wait(sdr_req, 6, sdr_rsp, &r_len);
        if (r_len == 8) {

            mtol = (u16 *)(&sdr_rsp[2]);
            bacc = (u32 *)(&sdr_rsp[4]);
            log_dbg(IPMI_SEN, "mtol=0x%x,bacc=0x%x\n",*mtol,*bacc);

            item->m =  __TO_M(*mtol);
            item->b =  __TO_B(*bacc);
            item->k1 =  __TO_B_EXP(*bacc);
            item->k2 =  __TO_R_EXP(*bacc);

            log_info(IPMI_SEN, ".id=%d,\t.m=%d,\t.b=%d,\t.k1=%d,\t.k2=%d,\n", \
                item->id, item->m, item->b, item->k1, item->k2);
        }
    }

    return ret;
}

static int ipmi_sensor_probe(struct i2c_client *client)
{
    int ret;
    struct i2c_adapter *adapter = client->adapter;
    int i2c_bus = adapter->nr;

    if (ipmi_i2c_user == NULL) {
        ipmi_i2c_user = (struct ipmi_user *)adapter->algo_data;
        if (ipmi_i2c_user == NULL) {
            log_err(IPMI_SEN, "ipmi_i2c_user acquire failed,check if ipmi_i2c_driver loaded\n");
            return -ENODEV;
        }
    }
   
    // 创建sysfs接口
    switch (i2c_bus) {
    case I2C_BUS_RAA:
        if (client->addr != I2C_ADDR_RAA) {
            log_err(IPMI_SEN, "raa228 i2c addr:0x%x invalid\n", client->addr);
            return -ENODEV;
        }
        raa_hwmon = kobject_create_and_add("hwmon", &client->dev.kobj);
        if(!raa_hwmon) {
            return -ENOMEM;
        }
        raa_hwmonN = kobject_create_and_add("hwmonN", raa_hwmon);
        if(!raa_hwmonN) {
            return -ENOMEM;
        }
        ret = sysfs_create_group(raa_hwmonN, &raa_node_attr_group);
        if (ret) {
            log_err(IPMI_SEN, "Failed to create sysfs attributes at bus %d\n", i2c_bus);
            return ret;
        }
    break;
    case I2C_BUS_XDPE1:
        if (client->addr != I2C_ADDR_XDPE) {
            log_err(IPMI_SEN, "xdpe1 i2c addr:0x%x invalid\n", client->addr);
            return -ENODEV;
        }
        xdpe1_hwmon = kobject_create_and_add("hwmon", &client->dev.kobj);
        if(!raa_hwmon) {
            return -ENOMEM;
        }
        xdpe1_hwmonN = kobject_create_and_add("hwmonN", xdpe1_hwmon);
        if(!xdpe1_hwmonN) {
            return -ENOMEM;
        }
        ret = sysfs_create_group(xdpe1_hwmonN, &xdpe1_node_attr_group);
        if (ret) {
            log_err(IPMI_SEN, "Failed to create sysfs attributes at bus %d\n", i2c_bus);
            return ret;
        }
    break;
    case I2C_BUS_XDPE2:
        if (client->addr != I2C_ADDR_XDPE) {
            log_err(IPMI_SEN, "xdpe2 i2c addr:0x%x invalid\n", client->addr);
            return -ENODEV;
        }
        xdpe2_hwmon = kobject_create_and_add("hwmon", &client->dev.kobj);
        if(!raa_hwmon) {
            return -ENOMEM;
        }
        xdpe2_hwmonN = kobject_create_and_add("hwmonN", xdpe2_hwmon);
        if(!xdpe2_hwmonN) {
            return -ENOMEM;
        }
        ret = sysfs_create_group(xdpe2_hwmonN, &xdpe2_node_attr_group);
        if (ret) {
            log_err(IPMI_SEN, "Failed to create sysfs attributes at bus %d\n", i2c_bus);
            return ret;
        }
    break;
    default:
        return -ENODEV;
    break;
    }
    
    //get MBK param
    if (mbk_updated == 0) {
        ret = update_mbk_param();
        if (ret) {
            log_err(IPMI_SEN, "Failed to update_mbk_param\n");
        }
        mbk_updated = 1;
    }

    log_info(IPMI_SEN, "ipmi_sensor probe at bus:%d addr:0x%x\n", i2c_bus, client->addr);

    return 0;
}

static void ipmi_sensor_remove(struct i2c_client *client)
{
    switch (client->adapter->nr) {
    case I2C_BUS_RAA:
        sysfs_remove_group(raa_hwmonN, &raa_node_attr_group);
        kobject_put(raa_hwmonN);
        kobject_put(raa_hwmon);
    break;
    case I2C_BUS_XDPE1:
        sysfs_remove_group(xdpe1_hwmonN, &xdpe1_node_attr_group);
        kobject_put(xdpe1_hwmonN);
        kobject_put(xdpe1_hwmon);
    break;
    case I2C_BUS_XDPE2:
        sysfs_remove_group(xdpe2_hwmonN, &xdpe2_node_attr_group);
        kobject_put(xdpe2_hwmonN);
        kobject_put(xdpe2_hwmon);
    break;
    default:
        return;
    break;
    }

    log_info(IPMI_SEN, "ipmi_sensor removed from bus:%d addr:0x%x\n", client->adapter->nr, client->addr);

    return;
}

static const struct i2c_device_id ipmi_sensor_id[] = {
    {"ipmi_sensor", 0},
    {}
};

MODULE_DEVICE_TABLE(i2c, ipmi_sensor_id);

/* This is the driver that will be inserted */
static struct i2c_driver ipmi_sensor_driver = {
    .driver = {
           .name = "ipmi_sensor",
           },
    .probe_new = ipmi_sensor_probe,
    .remove = ipmi_sensor_remove,
    .id_table = ipmi_sensor_id,
};

module_i2c_driver(ipmi_sensor_driver);

MODULE_AUTHOR("support");
MODULE_DESCRIPTION("IPMI SENSOR Driver");
MODULE_LICENSE("GPL");
