// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017-2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "bdf.h"

u_int16_t
pciehwdev_get_hostbdf(const pciehwdev_t *phwdev)
{
    pciehw_port_t *p;
    u_int8_t secbus;
    u_int16_t bdf;

    p = pciesvc_port_get(phwdev->port);
    secbus = p->secbus;
    pciesvc_port_put(p, CLEAN);

    /*
     * If we have a parent then map our local bdf based on root secbus,
     * else
     *     No parent this is the root, so no local bdf,
     *     construct bdf based on primary bus, also known as (secbus - 1).
     *     If no secbus set yet because no bios scan, then use 0.
     */
    if (phwdev->parenth) {
        bdf = bdf_make(bdf_to_bus(phwdev->bdf) + secbus,
                       bdf_to_dev(phwdev->bdf),
                       bdf_to_fnc(phwdev->bdf));
    } else {
        const u_int8_t bus = secbus ? secbus - 1 : 0;
        bdf = bdf_make(bus, 0, 0);
    }
    return bdf;
}
