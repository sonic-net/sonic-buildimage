/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * pmbus.h - Common defines and structures for PMBus devices
 *
 * Copyright (c) 2010, 2011 Ericsson AB.
 * Copyright (c) 2012 Guenter Roeck
 */

#ifndef IPMI_DEF_H
#define IPMI_DEF_H

#include <linux/completion.h>

// LOG
enum log_type {
    LOG_OFF,
    LOG_ERR,
    LOG_INFO,
    LOG_DEBUG,
};

#define IPMI_I2C "IPMI_I2C"
#define IPMI_SEN "IPMI_SENSOR"

#define log_f(filter, ...) \
    pr_cont("[%s] ", filter); \
    pr_cont(__VA_ARGS__)
#define log_cont(...) \
    pr_cont(__VA_ARGS__)

#define log_dbg_cont(...) \
    if (log_level >= LOG_DEBUG) { \
        log_cont(__VA_ARGS__); \
    }
#define log_dbg(filter, ...) \
    if (log_level >= LOG_DEBUG) { \
        log_f(filter, __VA_ARGS__); \
    }
#define log_info(filter, ...) \
    if (log_level >= LOG_INFO) { \
        log_f(filter, __VA_ARGS__); \
    }
#define log_err(filter, ...) \
    if (log_level >= LOG_ERR) { \
        log_f(filter, KERN_ALERT __VA_ARGS__); \
    }

#define GEN_NAME_SIZE 32

//ipmi cmd def
#define IPMI_NETFN_OEM_I2C 0x2E
#define IPMI_OEM_I2C_CMD 0x1

#define IPMI_NETFN_SENSOR_EVENT 0x4 
#define IPMI_GET_SENSOR_READING_CMD 0x2D
#define IPMI_GET_SENSOR_THRESHOLDS_CMD 0x27

#define IPMI_NETFN_STORAGE 0xA
#define IPMI_RESERVE_SDR_REP_CMD 0x22
#define IPMI_GET_SDR_CMD 0x23

typedef struct {
	struct completion comp;
	int read_byte;
	int recv_len;
	u8 *data;
} recv_user_data;

#endif /* IPMI_DEF_H */
