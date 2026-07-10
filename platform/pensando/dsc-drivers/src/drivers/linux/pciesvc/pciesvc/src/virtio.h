/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022, Pensando Systems Inc.
 */

#ifndef __VIRTIO_H__
#define __VIRTIO_H__

u_int64_t
virtio_barrd(pciehwdev_t *phwdev, u_int64_t addr,
             const u_int64_t baroff, const size_t size,
             u_int8_t *do_notify);

void
virtio_barwr(pciehwdev_t *phwdev, u_int64_t addr,
             const u_int64_t baroff, const size_t size, const u_int64_t val,
             u_int8_t *do_notify);

#endif /* __VIRTIO_H__ */
