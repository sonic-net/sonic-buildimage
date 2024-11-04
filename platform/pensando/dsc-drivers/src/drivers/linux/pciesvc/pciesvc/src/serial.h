/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022, Pensando Systems Inc.
 */

#ifndef __SERIAL_H__
#define __SERIAL_H__

u_int64_t
serial_barrd(pciehwdev_t *phwdev,
             const u_int64_t baroff, const size_t size);

void
serial_barwr(pciehwdev_t *phwdev,
             const u_int64_t baroff, const size_t size, const u_int64_t val);

void
serial_reset(pciehwdev_t *phwdev, const pciesvc_rsttype_t rsttype);

#endif /* __SERIAL_H__ */
