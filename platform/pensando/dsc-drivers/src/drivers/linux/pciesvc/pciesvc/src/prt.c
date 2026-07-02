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
    u_int32_t allocprt_l, freeprt_slab_l;

    allocprt_l = PSHMEM_DATA_FIELD(pshmem, allocprt);
    freeprt_slab_l = PSHMEM_DATA_FIELD(pshmem, freeprt_slab);
    if (n == PRT_SLAB_SIZE && freeprt_slab_l != PRT_INVALID) {
        /* alloc slab entry from slab list */
        prti = freeprt_slab_l;
        sprt = pciesvc_sprt_get(prti);
        PSHMEM_ASGN_FIELD(pshmem, freeprt_slab, sprt->next);
        sprt->next = PRT_INVALID;
        pciesvc_sprt_put(sprt, DIRTY);
    } else if (allocprt_l + n < prt_count()) {
        prti = allocprt_l;
        PSHMEM_ASGN_FIELD(pshmem, allocprt, allocprt_l + n);
    }
    return prti;
}

void
prt_free(const int prtb, const int prtc)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();
    u_int32_t allocprt_l;

    assert_prts_in_range(prtb, prtc);

    allocprt_l = PSHMEM_DATA_FIELD(pshmem, allocprt);
    if ((prtb + prtc) ==  allocprt_l) {
        PSHMEM_ASGN_FIELD(pshmem, allocprt, allocprt_l - prtc);
    } else if (prtc == PRT_SLAB_SIZE) {
        pciehw_shmem_t *pshmem = pciesvc_shmem_get();
        pciehw_sprt_t *sprt;

        sprt = pciesvc_sprt_get(prtb);
        sprt->next = PSHMEM_DATA_FIELD(pshmem, freeprt_slab);
        pciesvc_sprt_put(sprt, DIRTY);
        PSHMEM_ASGN_FIELD(pshmem, freeprt_slab, prtb);
    } else {
        pciesvc_logerror("prt_free: leak prt %d (%d), allocprt %d\n",
                          prtb, prtc, allocprt_l);
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
