/*
 * Copyright 2019 Broadcom.
 * The term “Broadcom” refers to Broadcom Inc. and/or its subsidiaries.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * Description
 *  Optics driver related api declarations
 */
#ifndef __PDDF_XCVR_API_H__
#define __PDDF_XCVR_API_H__

extern int sonic_i2c_get_mod_pres(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data);
extern int sonic_i2c_get_mod_reset(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data);
extern int sonic_i2c_get_mod_intr_status(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data);
extern int sonic_i2c_get_mod_lpmode(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data);
extern int sonic_i2c_get_mod_rxlos(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data);
extern int sonic_i2c_get_mod_txdisable(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data);
extern int sonic_i2c_get_mod_txfault(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data);
extern int sonic_i2c_set_mod_lpmode(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data);
extern int sonic_i2c_set_mod_reset(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data);
extern int sonic_i2c_set_mod_txdisable(struct i2c_client *client, XCVR_ATTR *info, struct xcvr_data *data);

extern ssize_t get_module_presence(struct device *dev, struct device_attribute *da, char *buf);
extern ssize_t get_module_reset(struct device *dev, struct device_attribute *da, char *buf);
extern ssize_t set_module_reset(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
extern ssize_t get_module_intr_status(struct device *dev, struct device_attribute *da, char *buf);
extern ssize_t get_module_lpmode(struct device *dev, struct device_attribute *da, char *buf);
extern ssize_t set_module_lpmode(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
extern ssize_t get_module_rxlos(struct device *dev, struct device_attribute *da, char *buf);
extern ssize_t get_module_txdisable(struct device *dev, struct device_attribute *da, char *buf);
extern ssize_t set_module_txdisable(struct device *dev, struct device_attribute *da, const char *buf, size_t count);
extern ssize_t get_module_txfault(struct device *dev, struct device_attribute *da, char *buf);

#endif
