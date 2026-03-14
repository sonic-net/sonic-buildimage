/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2018,2021, Pensando Systems Inc.
 */

#ifndef __REQ_INT_H__
#define __REQ_INT_H__

/*
 * Common interface for tgt_req_notify_int and tgt_req_indirect_int.
 */

#define MSGDATA_ADD_PORT        0x80000000      /* intr: msgdata += port */
#define MSGDATA_HAS_ADD_PORT(m) (((m) & MSGDATA_ADD_PORT) != 0)
#define MSGDATA_DATA(m)         ((m) & ~MSGDATA_ADD_PORT)

void
req_int_set(const u_int64_t reg, const u_int64_t addr, const u_int32_t data);

void
req_int_get(const u_int64_t reg, u_int64_t *addrp, u_int32_t *datap);

int
req_int_init(const u_int64_t reg,
             const int port, u_int64_t msgaddr, u_int32_t msgdata);

#endif /* __REQ_INT_H__ */
