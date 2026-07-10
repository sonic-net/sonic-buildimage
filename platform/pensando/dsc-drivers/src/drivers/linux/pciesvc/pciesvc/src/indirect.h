/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018,2021, Pensando Systems Inc.
 */

#ifndef __INDIRECT_H__
#define __INDIRECT_H__

#include "indirect_entry.h"

int pciehw_indirect_poll_init(const int port);
int pciehw_indirect_poll(const int port);
int pciehw_indirect_intr_init(const int port,
                              u_int64_t msgaddr, u_int32_t msgdata);
int pciehw_indirect_intr(const int port);

void pciehw_indirect_complete(indirect_entry_t *ientry);

#endif /* __INDIRECT_H__ */
