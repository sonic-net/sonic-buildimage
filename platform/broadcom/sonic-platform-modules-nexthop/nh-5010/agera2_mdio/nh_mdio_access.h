/*
 * Copyright (C) 2026 Nexthop Systems Inc.
 * SPDX-License-Identifier: Apache-2.0
 * 
 */

#ifndef NH_MDIO_ACCESS_H
#define NH_MDIO_ACCESS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t nh_mdio_status_t;

#define NH_MDIO_STATUS_SUCCESS            0   /* SAI_STATUS_SUCCESS */
#define NH_MDIO_STATUS_FAILURE            (-1) /* SAI_STATUS_FAILURE */
#define NH_MDIO_STATUS_INVALID_PARAMETER  (-5) /* SAI_STATUS_INVALID_PARAMETER */

/*
 * MDIO register access for gearbox PHYs reached through the Nexthop FPGA's
 * MDIO masters.
 */
nh_mdio_status_t mdio_read(uint64_t ctx,
                           uint32_t mdio_addr,
                           uint32_t reg_addr,
                           uint32_t number_of_registers,
                           uint32_t *data);

nh_mdio_status_t mdio_write(uint64_t ctx,
                            uint32_t mdio_addr,
                            uint32_t reg_addr,
                            uint32_t number_of_registers,
                            const uint32_t *data);

#ifdef __cplusplus
}
#endif

#endif /* NH_MDIO_ACCESS_H */
