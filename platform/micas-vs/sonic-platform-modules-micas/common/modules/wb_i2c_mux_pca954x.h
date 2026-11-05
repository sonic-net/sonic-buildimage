/*
 * A header definition for wb_i2c_mux_pca954x driver
 *
 * Copyright (C) 2024 Micas Networks Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __WB_I2C_MUX_PCA954X_H__
#define __WB_I2C_MUX_PCA954X_H__

#include <linux/string.h>

#define mem_clear(data, size) memset((data), 0, (size))

typedef enum pca9548_reset_type_s {
    PCA9548_RESET_NONE = 0,
    PCA9548_RESET_I2C = 1,
    PCA9548_RESET_GPIO = 2,
    PCA9548_RESET_IO = 3,
    PCA9548_RESET_FILE = 4,
} pca9548_reset_type_t;

typedef struct i2c_attr_s {
    uint32_t i2c_bus;
    uint32_t i2c_addr;
    uint32_t reg_offset;
    uint32_t mask;
    uint32_t reset_on;
    uint32_t reset_off;
} i2c_attr_t;

typedef struct io_attr_s {
    uint32_t io_addr;
    uint32_t mask;
    uint32_t reset_on;
    uint32_t reset_off;
} io_attr_t;

typedef struct file_attr_s {
    const char *dev_name;
    uint32_t offset;
    uint32_t mask;
    uint32_t reset_on;
    uint32_t reset_off;
    uint32_t width;
} file_attr_t;

typedef struct gpio_attr_s {
    int gpio_init;
    uint32_t gpio;
    uint32_t reset_on;
    uint32_t reset_off;
} gpio_attr_t;

typedef struct i2c_mux_pca954x_device_s {
    struct i2c_client *client;
    uint32_t i2c_bus;
    uint32_t i2c_addr;
    uint32_t pca9548_base_nr;
    uint32_t pca9548_reset_type;
    uint32_t rst_delay_b;                   /* delay time before reset(us) */
    uint32_t rst_delay;                     /* reset time(us) */
    uint32_t rst_delay_a;                   /* delay time after reset(us) */
    bool probe_disable;
    bool select_chan_check;
    bool close_chan_force_reset;
    union {
        i2c_attr_t i2c_attr;
        gpio_attr_t gpio_attr;
        io_attr_t io_attr;
        file_attr_t file_attr;
    } attr;
} i2c_mux_pca954x_device_t;

#endif
