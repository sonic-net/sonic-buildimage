/*! \file ngbde_swdev.c
 *
 * <description>
 *
 */
/*
 *
 * Copyright 2018-2025 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.
 */

#include <ngbde.h>

/* Switch devices */
static struct ngbde_dev_s swdevs[NGBDE_NUM_SWDEV_MAX];

/* Number of probed switch devices */
static unsigned int num_swdev;

int
ngbde_swdev_add(struct ngbde_dev_s *nd)
{
    unsigned int idx;

    /* Look for existing slot */
    for (idx = 0; idx < num_swdev; idx++) {
        if (swdevs[idx].bus_no == nd->bus_no &&
            swdevs[idx].slot_no == nd->slot_no) {
            if (swdevs[idx].inactive) {
                memcpy(&swdevs[idx], nd, sizeof(swdevs[0]));
                return 0;
            }
            /* Active device in this slot already? */
            printk(KERN_WARNING "%s: Device exists\n",
                   MOD_NAME);
            return -EBUSY;
        }
    }

    /* Add new device */
    if (num_swdev >= NGBDE_NUM_SWDEV_MAX) {
        return -ENOMEM;
    }
    memcpy(&swdevs[num_swdev], nd, sizeof(swdevs[0]));
    ++num_swdev;
    return 0;
}

struct ngbde_dev_s *
ngbde_swdev_get(int kdev)
{
    if ((unsigned int)kdev < num_swdev) {
        return &swdevs[kdev];
    }
    return NULL;
}

int
ngbde_swdev_get_all(struct ngbde_dev_s **nd, unsigned int *num_nd)
{
    if (nd) {
        *nd = swdevs;
    }
    if (num_nd) {
        *num_nd = num_swdev;
    }
    return 0;
}
