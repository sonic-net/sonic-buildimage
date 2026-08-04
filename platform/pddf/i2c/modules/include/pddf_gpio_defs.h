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
 * Description:
 *  Platform GPIO defines/structures header file
 */

#ifndef __PAL_GPIO_DEFS_H__
#define __PAL_GPIO_DEFS_H__

#include <linux/platform_data/pca953x.h>
/* GPIO CLIENT DATA*/
typedef struct GPIO_DATA
{
    int gpio_base;      // base bus number of the gpio pins
}GPIO_DATA;

#endif
