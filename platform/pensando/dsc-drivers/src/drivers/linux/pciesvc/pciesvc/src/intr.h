/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018,2021, Pensando Systems Inc.
 */

#ifndef __INTR_H__
#define __INTR_H__

union pciehwdev_u; typedef union pciehwdev_u pciehwdev_t;

void pciehw_intr_config(pciehwdev_t *phwdev,
                        const int legacy, const int fmask);

#endif /* __INTR_H__ */
