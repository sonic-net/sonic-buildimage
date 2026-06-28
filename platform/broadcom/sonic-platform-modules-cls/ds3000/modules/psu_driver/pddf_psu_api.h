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
 *  PSU driver related api declarations
 */

#ifndef __PDDF_PSU_API_H__
#define __PDDF_PSU_API_H__

extern void get_psu_duplicate_sysfs(int idx, char *str);
extern ssize_t psu_show_default(struct device *dev, struct device_attribute *da, char *buf);
extern ssize_t psu_store_default(struct device *dev, struct device_attribute *da, const char *buf, size_t count);

extern int sonic_i2c_get_psu_byte_default(void *client, PSU_DATA_ATTR *adata, void *data);
extern int sonic_i2c_get_psu_block_default(void *client, PSU_DATA_ATTR *adata, void *data);
extern int sonic_i2c_get_psu_word_default(void *client, PSU_DATA_ATTR *adata, void *data);

#endif
