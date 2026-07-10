// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019,2021-2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"

uint32_t
pciehw_vpd_read(const pciehwdevh_t hwdevh, const uint16_t addr)
{
    if (addr < PCIEHW_VPDSZ) {
        const uint16_t aligned_addr = addr & ~0x3;
        const uint8_t *vpddata = pciesvc_vpd_get(hwdevh);
        const uint32_t data = (((uint32_t)vpddata[aligned_addr + 3] << 24) |
                               ((uint32_t)vpddata[aligned_addr + 2] << 16) |
                               ((uint32_t)vpddata[aligned_addr + 1] <<  8) |
                               ((uint32_t)vpddata[aligned_addr + 0] <<  0));
        pciesvc_vpd_put(vpddata, CLEAN);
        return data;
    }
    return 0;
}

void
pciehw_vpd_write(const pciehwdevh_t hwdevh,
                 const uint16_t addr, const uint32_t data)
{
    /* No writeable vpd data (yet). */
}
