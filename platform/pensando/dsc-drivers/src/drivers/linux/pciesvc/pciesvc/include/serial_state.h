/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2022, Pensando Systems Inc.
 */

#ifndef __SERIAL_STATE_H__
#define __SERIAL_STATE_H__

#include "uart.h"

#define MEMQ_BUFSZ    64

typedef struct memq {
    unsigned int pidx    __attribute__((aligned(64)));
    unsigned int cidx    __attribute__((aligned(64)));
    char buf[MEMQ_BUFSZ] __attribute__((aligned(64)));
} memq_t;

typedef struct serial_state {
    u_int32_t intrb;            /* intr resource base */
    u_int32_t intrc;            /* intr resource count */
    u_int32_t gen;              /* generation number */
    u_int32_t gen_ack;          /* generation number ack */
    u_int32_t breakreq;         /* break request */
    u_int32_t _unused[11];
    memq_t txq;                 /* txq from device thr */
    memq_t rxq;                 /* rxq to   device rbr */
} serial_state_t;

typedef struct serial_uart_state {
    union {
        serial_state_t serial_state;
        u_int8_t _pad1[960];
    };
    union {
        uart_state_t uart_state;
        u_int8_t _pad2[64];
    };
} serial_uart_state_t;

#endif /* __SERIAL_STATE_H__ */
