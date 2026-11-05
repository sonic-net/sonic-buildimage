// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021-2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "indirect.h"
#include "notify.h"

int pciesvc_version_major = PCIESVC_VERSION_MAJ;
int pciesvc_version_minor = PCIESVC_VERSION_MIN;

pciesvc_logpri_t pciesvc_log_level = PCIESVC_LOGPRI_INFO;

/* local sanitized version of our params. */
typedef struct pciesvc_lparams_s {
    int         port;                   /* port */
    uint32_t    valid:1;                /* initialized */
    uint32_t    ind_poll:1;             /* indirect poll for work */
    uint32_t    ind_intr:1;             /* indirect intr for work */
    uint32_t    not_poll:1;             /* notify   poll for work */
    uint32_t    not_intr:1;             /* notify   intr for work */
    uint32_t    mac_poll:1;             /* mac poll */
    uint32_t    mac_intr:1;             /* mac intr */
    uint64_t    ind_msgaddr;            /* ind_intr=1: intr msg addr */
    uint32_t    ind_msgdata;            /* ind_intr=1: intr msg data */
    uint64_t    not_msgaddr;            /* not_intr=1: intr msg addr */
    uint32_t    not_msgdata;            /* not_intr=1: intr msg addr */
} pciesvc_lparams_t;

static pciesvc_lparams_t lparams[PCIEHW_NPORTS];

static pciesvc_lparams_t *
params_v0_to_lparams(pciesvc_params_v0_t *p)
{
    pciesvc_lparams_t *lp;

    if (p->port < 0 || p->port >= PCIEHW_NPORTS) {
        pciesvc_loglocal("pciesvc params invalid port %d\n", p->port);
        return NULL;
    }

    lp = &lparams[p->port];
    pciesvc_memset(lp, 0, sizeof(*lp));
    lp->port = p->port;

    /* poll *or* intr */
    if (p->ind_poll && p->ind_intr) {
        pciesvc_loglocal("pciesvc params indirect poll and intr\n");
        return NULL;
    }
    if (p->not_poll && p->not_intr) {
        pciesvc_loglocal("pciesvc params notify poll and intr\n");
        return NULL;
    }
    if (p->mac_poll && p->mac_intr) {
        pciesvc_loglocal("pciesvc params mac poll and intr\n");
        return NULL;
    }

    /* XXX don't mac handle yet */
    if (p->mac_poll || p->mac_intr) {
        pciesvc_loglocal("pciesvc params mac poll/intr not implemented\n");
        return NULL;
    }

    /* intr requires msgaddr */
    if (p->ind_intr && p->ind_msgaddr == 0) {
        pciesvc_loglocal("pciesvc params no indirect msgaddr\n");
        return NULL;
    }
    if (p->not_intr && p->not_msgaddr == 0) {
        pciesvc_loglocal("pciesvc params no notify msgaddr\n");
        return NULL;
    }

    lp->ind_poll = p->ind_poll;
    lp->ind_intr = p->ind_intr;
    lp->not_poll = p->not_poll;
    lp->not_intr = p->not_intr;
    lp->mac_poll = p->mac_poll;
    lp->mac_intr = p->mac_intr;
    lp->ind_msgaddr = p->ind_msgaddr;
    lp->ind_msgdata = p->ind_msgdata;
    lp->not_msgaddr = p->not_msgaddr;
    lp->not_msgdata = p->not_msgdata;
    lp->valid = 1;
    return lp;
}

static pciesvc_lparams_t *
params_to_lparams(pciesvc_params_t *params)
{
    pciesvc_lparams_t *lp = NULL;

    switch (params->version) {
    case 0:
        lp = params_v0_to_lparams(&params->params_v0);
        break;
    default:
        lp = NULL;
        break;
    }
    return lp;
}

static int
params_port(pciesvc_params_t *params)
{
    int port = -2;

    switch (params->version) {
    case 0:
        port = params->params_v0.port;
        break;
    default:
        port = -2;
        break;
    }
    return port;
}

int
pciesvc_init(pciesvc_params_t *params)
{
    int r;
    pciesvc_lparams_t *lp;

    /* if kpcimgr active_ports is unset (0) we get -1 here */
    if (params_port(params) == -1) {
        pciesvc_loglocal("pciesvc_init: no active ports\n");
        return 0;
    }

    lp = params_to_lparams(params);
    if (lp == NULL) goto err_out;

    if (lp->ind_poll) {
        if ((r = pciesvc_indirect_poll_init(lp->port)) < 0) {
            pciesvc_loglocal("indirect_poll_init failed: %d\n", r);
            goto err_out;
        }
    } else if (lp->ind_intr) {
        if ((r = pciesvc_indirect_intr_init(lp->port,
                                            lp->ind_msgaddr,
                                            lp->ind_msgdata)) < 0) {
            pciesvc_loglocal("indirect_intr_init failed: %d\n", r);
            goto err_out;
        }
    }

    if (lp->not_poll) {
        if ((r = pciesvc_notify_poll_init(lp->port)) < 0) {
            pciesvc_loglocal("indirect_poll_init failed: %d\n", r);
            goto err_out;
        }
    } else if (lp->not_intr) {
        if ((r = pciesvc_notify_intr_init(lp->port,
                                          lp->not_msgaddr,
                                          lp->not_msgdata)) < 0) {
            pciesvc_loglocal("notify_intr_init failed: %d\n", r);
            goto err_out;
        }
    }

    return 0;

 err_out:
    if (lp) lp->valid = 0;
    return -1;
}

void
pciesvc_shut(const int port)
{
    pciesvc_lparams_t *lp;

    if (port < 0 || port >= PCIEHW_NPORTS) return;

    lp = &lparams[port];
    if (!lp->valid) return;

    /*
     * Shutdown any interrupts.
     * Hardware doesn't have an interrupt disable setting.
     * For now init for poll, then don't poll anymore.
     */
    if (lp->ind_intr) {
        pciesvc_indirect_poll_init(port);
    }
    if (lp->not_intr) {
        pciesvc_notify_poll_init(port);
    }

    lp->valid = 0;
}

int
pciesvc_poll(const int port)
{
    pciesvc_lparams_t *lp;
    int ind_poll = 0;
    int not_poll = 0;

    if (port < 0 || port >= PCIEHW_NPORTS) return -1;

    lp = &lparams[port];
    if (!lp->valid) return -1;

    ind_poll = pciesvc_indirect_poll(lp->port);
    not_poll = pciesvc_notify_poll(lp->port);

    return (ind_poll || not_poll);
}

void
pciesvc_get_version(int *maj, int *min)
{
    *maj = pciesvc_version_major;
    *min = pciesvc_version_minor;
}

/******************************************************************
 * indirect
 */

int
pciesvc_indirect_poll_init(const int port)
{
    return pciehw_indirect_poll_init(port);
}

int
pciesvc_indirect_poll(const int port)
{
    return pciehw_indirect_poll(port);
}

int
pciesvc_indirect_intr_init(const int port,
                           u_int64_t msgaddr, u_int32_t msgdata)
{
    return pciehw_indirect_intr_init(port, msgaddr, msgdata);
}

int
pciesvc_indirect_intr(const int port)
{
    return pciehw_indirect_intr(port);
}

/******************************************************************
 * notify
 */

int
pciesvc_notify_poll_init(const int port)
{
    return pciehw_notify_poll_init(port);
}

int
pciesvc_notify_poll(const int port)
{
    return pciehw_notify_poll(port);
}

int
pciesvc_notify_intr_init(const int port,
                         u_int64_t msgaddr, u_int32_t msgdata)
{
    return pciehw_notify_intr_init(port, msgaddr, msgdata);
}

int
pciesvc_notify_intr(const int port)
{
    return pciehw_notify_intr(port);
}
