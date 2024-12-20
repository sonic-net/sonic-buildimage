/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2021, Pensando Systems Inc.
 * Copyright (c) 2022, Advanced Micro Devices, Inc.
 */

#ifndef __PCIESVC_H__
#define __PCIESVC_H__

#ifdef __cplusplus
extern "C" {
#if 0
} /* close to calm emacs autoindent */
#endif
#endif

#include "pmt.h"
#include "prt.h"
#include "pciehwmem.h"
#include "pcieshmem.h"
#include "pciesvc_event.h"
#include "pciesvc_cmd.h"

#define PCIESVC_VERSION_MAJ     3
#define PCIESVC_VERSION_MIN     5

typedef struct pciesvc_params_v0_s {
    int         port;                   /* port to config */
    uint32_t    ind_poll:1;             /* indirect trans poll */
    uint32_t    ind_intr:1;             /* indirect trans intr */
    uint32_t    not_poll:1;             /* notify trans poll */
    uint32_t    not_intr:1;             /* notify trans intr */
    uint32_t    mac_poll:1;             /* mac poll */
    uint32_t    mac_intr:1;             /* mac intr */
    uint64_t    ind_msgaddr;            /* ind_intr=1: intr msg addr */
    uint32_t    ind_msgdata;            /* ind_intr=1: intr msg data */
    uint64_t    not_msgaddr;            /* not_intr=1: intr msg addr */
    uint32_t    not_msgdata;            /* not_intr=1: intr msg addr */
} pciesvc_params_v0_t;

typedef struct pciesvc_params_s {
    int                         version;
    union {
        pciesvc_params_v0_t     params_v0;
    };
} pciesvc_params_t;

int pciesvc_init(pciesvc_params_t *params);
void pciesvc_shut(const int port);

/*
 * Return value:
 *     <0 error
 *     =0 no work done
 *     >0 work done
 */
int pciesvc_poll(const int port);

int pciesvc_indirect_poll_init(const int port);
int pciesvc_indirect_poll(const int port);
int pciesvc_indirect_intr_init(const int port,
                               u_int64_t msgaddr, u_int32_t msgdata);
int pciesvc_indirect_intr(const int port);

int pciesvc_notify_poll_init(const int port);
int pciesvc_notify_poll(const int port);
int pciesvc_notify_intr_init(const int port,
                             u_int64_t msgaddr, u_int32_t msgdata);
int pciesvc_notify_intr(const int port);

int pciesvc_cmd_read(char *buf, const long int off, const size_t count);
int pciesvc_cmd_write(const char *buf, const long int off, const size_t count);

extern int pciesvc_version_major;
extern int pciesvc_version_minor;

void pciesvc_get_version(int *maj, int *min);

extern pciesvc_logpri_t pciesvc_log_level;

#ifdef __cplusplus
}
#endif

#endif /* __PCIESVC_H__ */
