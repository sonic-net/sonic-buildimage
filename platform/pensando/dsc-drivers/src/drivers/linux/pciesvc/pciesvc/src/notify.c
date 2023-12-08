// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018,2021, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "pcietlp.h"
#include "req_int.h"
#include "notify.h"

#define NOTIFY_EN               PXB_(CFG_TGT_NOTIFY_EN)
#define NOTIFY_RING_SIZE        PXB_(CFG_TGT_REQ_NOTIFY_RING_SIZE)

#define NOTIFY_BASE             PXB_(DHS_TGT_NOTIFY)
#define NOTIFY_STRIDE           4

static u_int64_t
notify_addr(const int port)
{
    return NOTIFY_BASE + (port * NOTIFY_STRIDE);
}

static u_int64_t
notify_int_addr(void)
{
    return PXB_(CFG_TGT_REQ_NOTIFY_INT);
}

static void
notify_get_pici(const int port, int *pip, int *cip)
{
    const u_int32_t pici = pciesvc_reg_rd32(notify_addr(port));

    *pip = pici & 0xffff;
    *cip = pici >> 16;
}

static void
notify_get_masked_pici(const int port, int *pip, int *cip,
                       const u_int32_t ring_mask)
{
    int pi, ci;

    notify_get_pici(port, &pi, &ci);
    *pip = pi & ring_mask;
    *cip = ci & ring_mask;
}

/*
 * NOTE: The hw doesn't allow sw to write to PI,
 * when we write to the NOTIFY register only the CI is updated.
 * To reset to empty ring, set CI = PI.
 */
static void
notify_set_ci(const int port, const int ci)
{
    const u_int32_t pici = (ci << 16);
    pciesvc_reg_wr32(notify_addr(port), pici);
}

static u_int32_t
notify_pici_delta(const int pi, const int ci, const u_int32_t ring_mask)
{
    if (pi > ci) {
        return pi - ci;
    } else {
        return pi + ring_mask + 1 - ci;
    }
}

static void
notify_set_enable(const u_int32_t mask)
{
    union {
        struct {
            u_int32_t msg:1;
            u_int32_t pmv:1;
            u_int32_t db_pmv:1;
            u_int32_t unsupp:1;
            u_int32_t atomic:1;
            u_int32_t pmt_miss:1;
            u_int32_t pmr_invalid:1;
            u_int32_t prt_invalid:1;
            u_int32_t rc_vfid_miss:1;
            u_int32_t prt_oor:1;
            u_int32_t vfid_oor:1;
            u_int32_t cfg_bdf_oor:1;
            u_int32_t pmr_ecc_err:1;
            u_int32_t prt_ecc_err:1;
        } __attribute__((packed));
        u_int32_t w;
    } en;

    en.w = pciesvc_reg_rd32(NOTIFY_EN);
    en.w = mask;
    pciesvc_reg_wr32(NOTIFY_EN, en.w);
}

static void
notify_enable(void)
{
    notify_set_enable(0x3fff); /* enable all sources */
}

static int
notify_ring_inc(const int idx, const int inc, const u_int32_t ring_mask)
{
    return (idx + inc) & ring_mask;
}

static void
handle_notify(const int port, pciehw_port_t *p, notify_entry_t *nentry)
{
    const tlpauxinfo_t *info = &nentry->info;

    /*
     * If info->indirect_reason == 0 means we hit an entry we installed
     * in the PMT for indirect handling.  Go process the transaction.
     *
     * If info->indirect_reason != 0 then perhaps
     * this is an exception or error.  Track reason code stats.
     */
    if (info->indirect_reason == 0) {
        const u_int32_t pmti = info->pmti;
        pciehw_spmt_t *spmt = pciesvc_spmt_get(pmti);
        const pcie_tlp_common_hdr_t *hdr = (void *)nentry->rtlp;
        const u_int8_t tlp_type = hdr->type;

        switch (tlp_type) {
        case PCIE_TLP_TYPE_CFGRD0:
        case PCIE_TLP_TYPE_CFGRD1:
            pciehw_cfgrd_notify(port, nentry);
            spmt->swrd++;
            p->stats.not_cfgrd++;
            break;
        case PCIE_TLP_TYPE_CFGWR0:
        case PCIE_TLP_TYPE_CFGWR1:
            pciehw_cfgwr_notify(port, nentry);
            spmt->swwr++;
            p->stats.not_cfgwr++;
            break;
        case PCIE_TLP_TYPE_MEMRD:
        case PCIE_TLP_TYPE_MEMRD64:
            pciehw_barrd_notify(port, nentry);
            spmt->swrd++;
            p->stats.not_memrd++;
            break;
        case PCIE_TLP_TYPE_MEMWR:
        case PCIE_TLP_TYPE_MEMWR64:
            pciehw_barwr_notify(port, nentry);
            spmt->swwr++;
            p->stats.not_memwr++;
            break;
        case PCIE_TLP_TYPE_IORD:
            pciehw_barrd_notify(port, nentry);
            spmt->swrd++;
            p->stats.not_iord++;
            break;
        case PCIE_TLP_TYPE_IOWR:
            pciehw_barwr_notify(port, nentry);
            spmt->swwr++;
            p->stats.not_iowr++;
            break;
        default:
            p->stats.not_unknown++;
            break;
        }
        pciesvc_spmt_put(spmt, DIRTY);
    } else {
        uint64_t *notify_reasons = &p->stats.notify_reason_stats;
        notify_reasons[info->indirect_reason]++;
    }
}

/******************************************************************
 * apis
 */

/*
 * CFG_TGT_REQ_NOTIFY_INT
 */
int
pciehw_notify_intr_init(const int port, u_int64_t msgaddr, u_int32_t msgdata)
{
    notify_enable();
    return req_int_init(notify_int_addr(), port,
                        msgaddr, msgdata | MSGDATA_ADD_PORT);
}

static int
pciehw_notify_handle(const int port, const int polled)
{
    pciehw_port_t *p = pciesvc_port_get(port);
    const u_int32_t ring_mask = pciesvc_notify_ring_mask(port);
    int r, pi, ci, i, endidx;
    u_int32_t pici_delta;

    p->stats.not_intr++;
    if (polled) p->stats.not_polled++;

    notify_get_masked_pici(port, &pi, &ci, ring_mask);
    if (ci == pi) {
        p->stats.not_spurious++;
        r = 0; /* not our intr */
        goto out;
    }

    pici_delta = notify_pici_delta(pi, ci, ring_mask);

    p->stats.not_cnt += pici_delta;
    if (pici_delta > p->stats.not_max) {
        p->stats.not_max = pici_delta;
    }

    endidx = notify_ring_inc(pi, 1, ring_mask);
    for (i = notify_ring_inc(ci, 1, ring_mask);
         i != endidx;
         i = notify_ring_inc(i, 1, ring_mask)) {
        notify_entry_t *nentry;

        nentry = pciesvc_notify_ring_get(port, i);
        handle_notify(port, p, nentry);
        pciesvc_notify_ring_put(nentry);

        /* return some slots occasionally while processing */
        if ((i & 0xff) == 0) {
            notify_set_ci(port, i);
        }
    }

    /* we consumed these, adjust ci */
    notify_set_ci(port, pi);
    r = 1; /* handled intr */

 out:
    pciesvc_port_put(p, DIRTY);
    return r;
}

int
pciehw_notify_intr(const int port)
{
    return pciehw_notify_handle(port, 0);
}

/*
 * Arrange to have the notify interrupt written to memory,
 * then we can poll memory locations to see if there is work to do.
 */
int
pciehw_notify_poll_init(const int port)
{
    const u_int64_t msgaddr = pciesvc_notify_intr_dest_pa(port);
    const u_int32_t msgdata = 1;

    notify_enable();
    return req_int_init(notify_int_addr(), port, msgaddr, msgdata);
}

int
pciehw_notify_poll(const int port)
{
    const u_int32_t ring_mask = pciesvc_notify_ring_mask(port);
    int pi, ci;
    int r = 0;

    notify_get_masked_pici(port, &pi, &ci, ring_mask);
    if (ci != pi) {
        r = pciehw_notify_handle(port, 1);
    }
    return r;
}
