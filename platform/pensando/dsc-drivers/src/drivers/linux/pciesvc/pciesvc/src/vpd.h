/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019,2021, Pensando Systems Inc.
 */

#ifndef __VPD_H__
#define __VPD_H__

typedef u_int32_t pciehwdevh_t;

uint32_t pciehw_vpd_read(const pciehwdevh_t hwdevh, const uint16_t addr);
void pciehw_vpd_write(const pciehwdevh_t hwdevh,
                      const uint16_t addr, const uint32_t data);

#endif /* __VPD_H__ */
