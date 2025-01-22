// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017-2018,2021-2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "prt.h"

#define PRT_BASE        PXB_(DHS_TGT_PRT)
#define PRT_STRIDE      ASIC_(PXB_CSR_DHS_TGT_PRT_ENTRY_BYTE_SIZE)

/* the only client freeing slabs is overrides */
#define PRT_SLAB_SIZE   PCIEHDEVICE_OVERRIDE_INTRGROUPS

static int
prt_count(void)
{
    return PRT_COUNT;
}

static void
assert_prti_in_range(const int prti)
{
    pciesvc_assert(prti >= 0 && prti < prt_count());
}

static void
assert_prts_in_range(const int prtb, const int prtc)
{
    if (prtc > 0) {
        assert_prti_in_range(prtb);
        assert_prti_in_range(prtb + prtc - 1);
    }
}

int
prt_alloc(const int n)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();
    pciehw_sprt_t *sprt;
    int prti = -1;

    if (n == PRT_SLAB_SIZE && pshmem->freeprt_slab != PRT_INVALID) {
        /* alloc slab entry from slab list */
        prti = pshmem->freeprt_slab;
        sprt = pciesvc_sprt_get(prti);
        pshmem->freeprt_slab = sprt->next;
        sprt->next = PRT_INVALID;
        pciesvc_sprt_put(sprt, DIRTY);
    } else if (pshmem->allocprt + n < prt_count()) {
        prti = pshmem->allocprt;
        pshmem->allocprt += n;
    }
    return prti;
}

void
prt_free(const int prtb, const int prtc)
{
    assert_prts_in_range(prtb, prtc);

    if (prtc == PRT_SLAB_SIZE) {
        pciehw_shmem_t *pshmem = pciesvc_shmem_get();
        pciehw_sprt_t *sprt;

        sprt = pciesvc_sprt_get(prtb);
        sprt->next = pshmem->freeprt_slab;
        pciesvc_sprt_put(sprt, DIRTY);
        pshmem->freeprt_slab = prtb;
    } else {
        /* XXX */
    }
}

static u_int64_t
prt_addr(const int prti)
{
    assert_prti_in_range(prti);
    return PRT_BASE + (prti * PRT_STRIDE);
}

void
prt_get(const int prti, prt_t *prt)
{
    pciesvc_reg_rd32w(prt_addr(prti), prt->w, PRT_NWORDS);
}

void
prt_set(const int prti, const prt_t *prt)
{
    pciesvc_reg_wr32w(prt_addr(prti), prt->w, PRT_NWORDS);
}

/******************************************************************
 * apis
 */

int
pciehw_prt_load(const int prtbase, const int prtcount)
{
    const int prtend = prtbase + prtcount;
    pciehw_sprt_t *sprt;
    int prti;

    assert_prts_in_range(prtbase, prtcount);

    for (prti = prtbase; prti < prtend; prti++) {
        sprt = pciesvc_sprt_get(prti);
        prt_set(prti, &sprt->prt);
        pciesvc_sprt_put(sprt, CLEAN);
    }
    return 0;
}

void
pciehw_prt_unload(const int prtbase, const int prtcount)
{
    const int prtend = prtbase + prtcount;
    const prt_t prt0 = {{ 0 }};
    int prti;

    assert_prts_in_range(prtbase, prtcount);

    for (prti = prtbase; prti < prtend; prti++) {
        prt_set(prti, &prt0);
    }
}
