/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018-2019, Pensando Systems Inc.
 */

#ifndef __RESET_H__
#define __RESET_H__

union pciehwdev_u;
typedef union pciehwdev_u pciehwdev_t;

void pciehw_reset_bus(pciehwdev_t *phwdev, const u_int8_t bus);
void pciehw_reset_flr(pciehwdev_t *phwdev);
void pciehw_reset_vfs(pciehwdev_t *phwdev, const int vfb, const int vfc);

#endif /* __RESET_H__ */
