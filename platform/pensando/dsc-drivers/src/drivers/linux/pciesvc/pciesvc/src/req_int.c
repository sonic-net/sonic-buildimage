// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018,2021-2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "req_int.h"

#define REQ_INT_NWORDS  3

typedef union req_int_u {
    struct {
        u_int64_t data:32;
        u_int64_t addrdw:34;
    } __attribute__((packed));
    u_int32_t w[REQ_INT_NWORDS];
} req_int_t;

void
req_int_set(const u_int64_t reg, const u_int64_t addr, const u_int32_t data)
{
    req_int_t in = { .data = data, .addrdw = addr >> 2 };

    pciesvc_reg_wr32w(reg, in.w, REQ_INT_NWORDS);
}

void
req_int_get(const u_int64_t reg, u_int64_t *addrp, u_int32_t *datap)
{
    req_int_t in;

    pciesvc_reg_rd32w(reg, in.w, REQ_INT_NWORDS);
    *addrp = in.addrdw << 2;
    *datap = in.data;
}

/*
 * The pcie request hardware provides a single base register
 * CFG_TGT_REQ_*_INT.addrdw to configure indirect/notify interrupts.
 * Each per-port interrupt is sent to the address
 * (CFG_TGT_REQ_*_INT.addrdw << 2) + (port * 4).
 *
 * If CFG_TGT_REQ_*_INT.data[31] == 0 then a "1" is written to
 * the destination address.  This is used to trigger an interrupt
 * through a write to INTR_ASSERT register.
 * If CFG_TGT_REQ_*_INT.data[31] == 1 then data written is
 *     data = (data & 0x7fffffff) + port.
 *
 * This routine provides the abstraction that we can configure each
 * port independently.  When the first port is configured we set the
 * base port0 values for msgaddr0/msgdata0 and configure the hw to match.
 * Subsequent ports msgaddr/data are validated to be sure they match
 * what the hw will do.
 */
int
req_int_init(const u_int64_t reg,
             const int port, u_int64_t msgaddr, u_int32_t msgdata)
{
    u_int64_t msgaddr0;
    u_int32_t msgdata0;
    int r = 0;

    /*
     * First time through set msgaddr0/data0 and hw to match.
     * Doesn't matter which port we configure first,
     * but subsequent ports must follow the pattern
     *     msgaddr = msgaddr0 + (port * 4)
     *     msgdata = msgdata0 + port
     */
    req_int_get(reg, &msgaddr0, &msgdata0);
    if (port == 0 || msgaddr0 == 0) {
        msgaddr0 = msgaddr - (port * 4);
        if (MSGDATA_HAS_ADD_PORT(msgdata)) {
            msgdata0 = (MSGDATA_DATA(msgdata) - port) | MSGDATA_ADD_PORT;
        } else {
            msgdata0 = msgdata;
        }
        req_int_set(reg, msgaddr0, msgdata0);
    }

    if (msgaddr != msgaddr0 + (port * 4)) {
        r = -1;
    } else if (MSGDATA_HAS_ADD_PORT(msgdata) !=
               MSGDATA_HAS_ADD_PORT(msgdata0)) {
        r = -2;
    } else if (MSGDATA_HAS_ADD_PORT(msgdata) &&
               MSGDATA_DATA(msgdata) !=
               MSGDATA_DATA(MSGDATA_DATA(msgdata0) + port)) {
        r = -3;
    } else if (!MSGDATA_HAS_ADD_PORT(msgdata) &&
               MSGDATA_DATA(msgdata) != MSGDATA_DATA(msgdata0)) {
        r = -4;
    }
    return r;
}
