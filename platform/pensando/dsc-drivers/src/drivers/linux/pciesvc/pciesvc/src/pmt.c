// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017-2018,2021-2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "bdf.h"
#include "prt.h"
#include "pmt.h"

#define PMT_BASE        PXB_(DHS_TGT_PMT)
#define PMT_STRIDE      \
    (ASIC_(PXB_CSR_DHS_TGT_PMT_ENTRY_ARRAY_ELEMENT_SIZE) * 4 * 8)
#define PMT_GRST        PXB_(CFG_TGT_PMT_GRST)
#define PMR_BASE        PXB_(DHS_TGT_PMR)
#define PMR_STRIDE      ASIC_(PXB_CSR_DHS_TGT_PMR_ENTRY_BYTE_SIZE)

static int
pmt_count(void)
{
    return PMT_COUNT;
}

static void
assert_pmti_in_range(const int pmti)
{
    pciesvc_assert(pmti >= 0 && pmti < pmt_count());
}

static void
assert_pmts_in_range(const int pmtb, const int pmtc)
{
    if (pmtc > 0) {
        assert_pmti_in_range(pmtb);
        assert_pmti_in_range(pmtb + pmtc - 1);
    }
}

static u_int64_t
pmt_addr(const int pmti)
{
    assert_pmti_in_range(pmti);
    return PMT_BASE + (pmti * PMT_STRIDE);
}

static u_int64_t
pmr_addr(const int pmti)
{
    assert_pmti_in_range(pmti);
    return PMR_BASE + (pmti * PMR_STRIDE);
}

static int
pmt_alloc_high(const int n)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();
    pciehw_spmt_t *spmt;
    int pmti = -1;
    u_int32_t freepmt_high_l, allocpmt_high_l, allocpmt_low_l;

    freepmt_high_l = PSHMEM_DATA_FIELD(pshmem, freepmt_high);
    allocpmt_high_l = PSHMEM_DATA_FIELD(pshmem, allocpmt_high);
    allocpmt_low_l = PSHMEM_DATA_FIELD(pshmem, allocpmt_low);

    if (n == 1 && freepmt_high_l != PMT_INVALID) {
        /* alloc a single entry from free list */
        pmti = freepmt_high_l;
        spmt = pciesvc_spmt_get(pmti);
        PSHMEM_ASGN_FIELD(pshmem, freepmt_high, spmt->next);
        spmt->next = PMT_INVALID;
        pciesvc_spmt_put(spmt, DIRTY);
    } else if (allocpmt_high_l + n <= allocpmt_low_l) {
        /* alloc multiple entries from sequential block */
        pmti = allocpmt_high_l;
        PSHMEM_ASGN_FIELD(pshmem, allocpmt_high, allocpmt_high_l + n);
    }
    return pmti;
}

static int
pmt_alloc_low(const int n)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();
    pciehw_spmt_t *spmt;
    int pmti = -1;
    u_int32_t freepmt_low_l, allocpmt_high_l, allocpmt_low_l;

    freepmt_low_l = PSHMEM_DATA_FIELD(pshmem, freepmt_low);
    allocpmt_high_l = PSHMEM_DATA_FIELD(pshmem, allocpmt_high);
    allocpmt_low_l = PSHMEM_DATA_FIELD(pshmem, allocpmt_low);

    if (n == 1 && freepmt_low_l != PMT_INVALID) {
        /* alloc a single entry from free list */
        pmti = freepmt_low_l;
        spmt = pciesvc_spmt_get(pmti);
        PSHMEM_ASGN_FIELD(pshmem, freepmt_low, spmt->next);
        spmt->next = PMT_INVALID;
        pciesvc_spmt_put(spmt, DIRTY);
    } else if (allocpmt_low_l - n >= allocpmt_high_l) {
        /* alloc multiple entries from sequential block */
        PSHMEM_ASGN_FIELD(pshmem, allocpmt_low, allocpmt_low_l - n);
        pmti = PSHMEM_DATA_FIELD(pshmem, allocpmt_low);
    }
    return pmti;
}

static int
pmt_alloc_vf0adj(const int n)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();
    int pmti = -1;
    u_int32_t allocpmt_vf0adj_l = PSHMEM_DATA_FIELD(pshmem, allocpmt_vf0adj);

    /* if no reserved vf0adj region alloc from high pri */
    if (allocpmt_vf0adj_l == -1) {
        pmti = pmt_alloc_high(n);
    } else if (allocpmt_vf0adj_l + n <= pmt_count()) {
        pmti = allocpmt_vf0adj_l;
        PSHMEM_ASGN_FIELD(pshmem, allocpmt_vf0adj, allocpmt_vf0adj_l + n);
    }
    return pmti;
}

/*
 * pmt_alloc - supports multiple priority regions in the tcam.
 * We want both PMTPRI_HIGH and PMTPRI_LOW regions to
 * be able to grow, LOW to support adding flexvfs and
 * HIGH to support adding overrides.
 *
 * Once the base config is configured and all pmts allocated
 * then only HIGH grows down to allow the max number of overrides.
 *
 *     +----------------+ <== 0
 *     |  PMTPRI_HIGH   | <== pshmem->allocpmt_high
 *     |  grows down    |
 *     +----------------+
 *     |  PMTPRI_LOW    |
 *     |  grows up      | <== pshmem->allocpmt_low
 *     +----------------+
 *     |  PMTPRI_VF0ADJ | <== pshmem->allocpmt_vf0adj
 *     |  grows down    |
 *     +----------------+
 *                        <== pmt_count()
 *
 * PMTPRI_HIGH   - config, PF bars, VF flexvf overrides
 *
 * PMTPRI_LOW    - VF flexvf base entries in this region
 *
 * PMTPRI_VF0ADJ - if we have flexvf then adjust_vf0 entries come
 *                 from this region.  We want the priority to be lower
 *                 that LOW so entries are lower than the flexvf base entries.
 *                 This region grows down to meet the expectations of the
 *                 user but is low priority so entries can be overridden
 *                 by flexvf overrides in the HIGH region.
 */
int
pmt_alloc(const int n, const int pri)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();
    int pmti = -1;

    if (n <= 0 || n > pmt_count()) {
        pciesvc_logerror("pmt_alloc: Invalid count value %d\n", n);
        return -1;
    }

    if (!PSHMEM_DATA_FIELD(pshmem, pmtpri)) {
        PSHMEM_ASGN_FIELD(pshmem, allocpmt_low, pmt_count());
        PSHMEM_ASGN_FIELD(pshmem, freepmt_high, PMT_INVALID);
        PSHMEM_ASGN_FIELD(pshmem, freepmt_low, PMT_INVALID);
        PSHMEM_ASGN_FIELD(pshmem, allocpmt_vf0adj, -1);
        PSHMEM_ASGN_FIELD(pshmem, freeprt_slab, PRT_INVALID);
        PSHMEM_ASGN_FIELD(pshmem, pmtpri, 1);
    }

    switch (pri) {
    case PMTPRI_HIGH:
        pmti = pmt_alloc_high(n);
        break;
    case PMTPRI_LOW:
        pmti = pmt_alloc_low(n);
        break;
    case PMTPRI_VF0ADJ:
        pmti = pmt_alloc_vf0adj(n);
        break;
    default:
        pciesvc_logerror("pmt_alloc: unknown pri %d\n", pri);
        pciesvc_assert(0);
        break;
    }

    return pmti;
}

/*
 * Reserve a contiguous range from PMTPRI_LOW to be used for
 * the vf0adjust range.  PMTPRI_LOW grows up but we want
 * PMTPRI_VF0ADJ to be lower than LOW priority and grow down.
 */
int
pmt_reserve_vf0adj(const int n)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();
    int ret = 0;

    ret = pmt_alloc(n, PMTPRI_LOW);
    if (ret < 0) return ret;
    PSHMEM_ASGN_FIELD(pshmem, allocpmt_vf0adj, ret);
    return PSHMEM_DATA_FIELD(pshmem, allocpmt_vf0adj);
}

static int
pmt_to_pri(const int pmtb, const int pmtc)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();
    int pmtpri = -1;
    int pmti = pmtb + pmtc;
    u_int32_t allocpmt_high_l, allocpmt_low_l;

    allocpmt_high_l = PSHMEM_DATA_FIELD(pshmem, allocpmt_high);
    allocpmt_low_l = PSHMEM_DATA_FIELD(pshmem, allocpmt_low);
    if (pmtb >= 0 && pmti <= allocpmt_high_l) {
        pmtpri = PMTPRI_HIGH;
    } else if (pmtb >= allocpmt_low_l && pmti <= pmt_count()) {
        pmtpri = PMTPRI_LOW;
    }
    return pmtpri;
}

static int
spmt_to_pmti(const pciehw_spmt_t *spmt)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();
    return spmt - PSHMEM_DATA_FIELD(pshmem, spmt);
}

void
pmt_free(const int pmtb, const int pmtc)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();
    pciehw_spmt_t *spmt;
    int pmti, pmtpri;
    u_int32_t allocpmt_high_l, freepmt_high_l, allocpmt_low_l, freepmt_low_l;

    assert_pmts_in_range(pmtb, pmtc);

    allocpmt_high_l = PSHMEM_DATA_FIELD(pshmem, allocpmt_high);
    allocpmt_low_l = PSHMEM_DATA_FIELD(pshmem, allocpmt_low);
    freepmt_high_l = PSHMEM_DATA_FIELD(pshmem, freepmt_high);
    freepmt_low_l = PSHMEM_DATA_FIELD(pshmem, freepmt_low);
    pmtpri = pmt_to_pri(pmtb, pmtc);
    if (pmtpri == PMTPRI_HIGH) {
        /* free high pri */
        if (allocpmt_high_l == (pmtb + pmtc)) {
            PSHMEM_ASGN_FIELD(pshmem, allocpmt_high, allocpmt_high_l - pmtc);
            return;
        }
        for (pmti = pmtb; pmti < pmtb + pmtc; pmti++) {
            spmt = pciesvc_spmt_get(pmti);
            spmt->next = freepmt_high_l;
            pciesvc_spmt_put(spmt, DIRTY);
            PSHMEM_ASGN_FIELD(pshmem, freepmt_high, pmti);
        }
    } else if (pmtpri == PMTPRI_LOW) {
        /* free low pri */
        if (allocpmt_low_l == pmtb) {
            PSHMEM_ASGN_FIELD(pshmem, allocpmt_low, allocpmt_low_l + pmtc);
            return;
        }
        for (pmti = pmtb; pmti < pmtb + pmtc; pmti++) {
            spmt = pciesvc_spmt_get(pmti);
            spmt->next = freepmt_low_l;
            pciesvc_spmt_put(spmt, DIRTY);
            PSHMEM_ASGN_FIELD(pshmem, freepmt_low, pmti);
        }
    } else {
        /* outside of both alloc ranges? */
        pciesvc_logerror("pmt_free: leak pmt %d (%d), "
                         "allocpmt_low %u allocpmt_high %u\n",
                         pmtb, pmtc,
                         allocpmt_low_l,
                         allocpmt_high_l);
    }
}

static void
pmt_get_entry(const int pmti, pmt_entry_t *pmte)
{
    pciesvc_reg_rd32w(pmt_addr(pmti), pmte->w, PMT_NWORDS);
}

static void
pmt_set_entry(const int pmti, const pmt_entry_t *pmte)
{
    pciesvc_reg_wr32w(pmt_addr(pmti), pmte->w, PMT_NWORDS);
}

static void
pmr_get_entry(const int pmti, pmr_entry_t *pmre)
{
    pciesvc_reg_rd32w(pmr_addr(pmti), pmre->w, PMR_NWORDS);
}

static void
pmr_set_entry(const int pmti, const pmr_entry_t *pmre)
{
    pciesvc_reg_wr32w(pmr_addr(pmti), pmre->w, PMR_NWORDS);
}

/*
 * Retrieve an entry from hardware.
 */
void
pmt_get(const int pmti, pmt_t *pmt)
{
    pmt_get_entry(pmti, &pmt->pmte);
    pmr_get_entry(pmti, &pmt->pmre);
}

/*
 * Install an entry in hardware at the specified index.
 */
void
pmt_set(const int pmti, const pmt_t *pmt)
{
    /*
     * Set PMR entry first, then TCAM, so by the time a tcam search
     * can hit an entry the corresponding ram entry is valid too.
     */
    pmr_set_entry(pmti, &pmt->pmre);
    pmt_set_entry(pmti, &pmt->pmte);
}

static void
pmt_clr_tcam(const int pmti)
{
    pmt_entry_t pmte0;

    pciesvc_memset(&pmte0, 0, sizeof(pmte0));
    pmt_set_entry(pmti, &pmte0);
}

/*
 * Clear a pmt slot.  For now we just invalidate the tcam entry
 * so searches don't hit, but we don't write anything to PMR.
 */
static void
pmt_clr(const int pmti)
{
    pmt_clr_tcam(pmti);
}

/*
 * dm->data is the entry data values
 * dm->mask is the entry mask bits,
 *     1's for bits we want to match in "data",
 *     0's for bits in "data" we want to ignore.
 *
 * {X Y} result
 * -------
 * {0 0} (always match)
 * {0 1} match if 0
 * {1 0} match if 1
 * {1 1} (never match)
 */
void
pmt_entry_enc(pmt_entry_t *pmte, const pmt_datamask_t *dm)
{
    const u_int64_t data = dm->data.all;
    const u_int64_t mask = dm->mask.all;

    pmte->tcam.x =  data & mask;
    pmte->tcam.y = ~data & mask;
    pmte->tcam.v = 1;
}

/*
 * Fetch the dm->data/mask values from this pmt_entry.
 */
void
pmt_entry_dec(const pmt_entry_t *pmte, pmt_datamask_t *dm)
{
    dm->data.all = pmte->tcam.x;
    dm->mask.all = pmte->tcam.x ^ pmte->tcam.y;
}

void
pmt_bar_set_bdf(pmt_t *pmt, const u_int16_t bdf)
{
    pmr_bar_entry_t *pmr = &pmt->pmre.bar;
    pmr->bdf = bdf;
}

void
pmt_bar_setaddr(pmt_t *pmt, const u_int64_t addr)
{
    pmt_datamask_t dm;

    pmt_entry_dec(&pmt->pmte, &dm);
    dm.data.bar.addrdw = addr >> 2;
    pmt_entry_enc(&pmt->pmte, &dm);
}

void
pmt_bar_setaddrm(pmt_t *pmt, const u_int64_t addr, const u_int64_t mask)
{
    pmt_datamask_t dm;

    pmt_entry_dec(&pmt->pmte, &dm);
    dm.data.bar.addrdw = addr >> 2;
    dm.mask.bar.addrdw = mask >> 2;
    pmt_entry_enc(&pmt->pmte, &dm);
}

u_int64_t
pmt_bar_getaddr(const pmt_t *pmt)
{
    pmt_datamask_t dm;

    pmt_entry_dec(&pmt->pmte, &dm);
    return dm.data.bar.addrdw << 2;
}

u_int64_t
pmt_bar_getaddrmask(const pmt_t *pmt)
{
    pmt_datamask_t dm;

    pmt_entry_dec(&pmt->pmte, &dm);
    return dm.mask.bar.addrdw << 2;
}

void
pmt_cfg_set_bus(pmt_t *pmt, const u_int8_t bus)
{
    pmr_cfg_entry_t *pmr = &pmt->pmre.cfg;
    pmt_datamask_t dm;
    u_int8_t obus, busdelta;

#define bdf_replace_bus(bdf, bus) \
    bdf = ((bus << 8) | ((bdf) & 0x00ff))

    pmt_entry_dec(&pmt->pmte, &dm);
    /* assume no bus wildcards */
    pciesvc_assert((dm.mask.cfg.bdf & 0xff00) == 0xff00);
    obus = bdf_to_bus(dm.data.cfg.bdf);
    bdf_replace_bus(dm.data.cfg.bdf, bus);
    pmt_entry_enc(&pmt->pmte, &dm);

    /* replicate the bus delta between start/limit with new bus */
    busdelta = pmr->bstart - obus;
    pmr->bstart = bus + busdelta;

    busdelta = pmr->blimit - obus;
    pmr->blimit = bus + busdelta;
}

/******************************************************************
 * apis
 */

void
pciehw_pmt_load_cfg(pciehwdev_t *phwdev)
{
    if (!phwdev->cfgloaded) {
        u_int32_t pmti;

        for (pmti = phwdev->pmtb; pmti < phwdev->pmtb + phwdev->pmtc; pmti++) {
            pciehw_spmt_t *spmt = pciesvc_spmt_get(pmti);
            int state = CLEAN;

            if (!spmt->loaded) {
                pmt_set(pmti, &spmt->pmt);
                spmt->loaded = 1;
                state = DIRTY; /* loaded */
            }
            pciesvc_spmt_put(spmt, state);
        }
        phwdev->cfgloaded = 1;
    }
}

void
pciehw_pmt_unload_cfg(pciehwdev_t *phwdev)
{
    if (phwdev->cfgloaded) {
        u_int32_t pmti;

        for (pmti = phwdev->pmtb; pmti < phwdev->pmtb + phwdev->pmtc; pmti++) {
            pciehw_spmt_t *spmt = pciesvc_spmt_get(pmti);
            int state = CLEAN;

            if (spmt->loaded) {
                pmt_clr(pmti);
                spmt->loaded = 0;
                state = DIRTY; /* loaded */
            }
            pciesvc_spmt_put(spmt, state);
        }
        phwdev->cfgloaded = 0;
    }
}

static void
pciehw_bar_foreach_pmt(pciehwbar_t *phwbar,
                       void (*cb)(int pmti, pciehw_spmt_t *spmt, void *arg),
                       void *arg)
{
    pciehw_spmt_t *spmt;
    u_int32_t pmti;
    int next;

    /* process the base pmts */
    for (pmti = phwbar->pmtb; pmti < phwbar->pmtb + phwbar->pmtc; pmti++) {
        int chain;

        spmt = pciesvc_spmt_get(pmti);
        chain = spmt->chain;
        next = spmt->next;

        cb(pmti, spmt, arg);

        pciesvc_spmt_put(spmt, DIRTY); /* spmt.pmt.pmr.bdf, loaded */

        /* if chained pmts, process them */
        if (chain) {
            while (next != PMT_INVALID) {
                spmt = pciesvc_spmt_get(next);

                cb(next, spmt, arg);

                next = spmt->next;
                pciesvc_spmt_put(spmt, DIRTY); /* loaded */
            }
        }
    }

    /* if ovrds, process them */
    if (phwbar->ovrds) {
        next = phwbar->ovrd;
        while (next != PMT_INVALID) {
            spmt = pciesvc_spmt_get(next);

            cb(next, spmt, arg);

            next = spmt->next;
            pciesvc_spmt_put(spmt, DIRTY); /* loaded */
        }
    }
}

static void
pmt_load(const int pmti, pciehw_spmt_t *spmt, const u_int16_t bdf)
{
    /*
     * Load PRT first, then load PMT so PMT tcam search hit
     * will find valid PRT entries.
     */
    pciehw_prt_load(spmt->pmt.pmre.bar.prtb, spmt->pmt.pmre.bar.prtc);

    /* vf0 bdf was adjusted already in adjust_vf0 */
    if (!spmt->vf0) {
        /* place bus-adjusted bdf in pmt, then load in hw */
        pmt_bar_set_bdf(&spmt->pmt, bdf);
    }
    pmt_set(pmti, &spmt->pmt);

    if (!spmt->loaded) {
        spmt->loaded = 1;
    }
}

static void
pmt_load_cb(int pmti, pciehw_spmt_t *spmt, void *arg)
{
    const u_int16_t bdf = *(u_int16_t *)arg;

    pmt_load(pmti, spmt, bdf);
}

void
pciehw_bar_load_pmts(pciehwbar_t *phwbar)
{
    u_int16_t bdf = phwbar->bdf;

    if (!phwbar->valid) return;
    pciehw_bar_foreach_pmt(phwbar, pmt_load_cb, &bdf);
}

static void
pmt_unload(const int pmti, pciehw_spmt_t *spmt)
{
    /*
     * Unload PMT first THEN PRT, so PMT tcam search will not hit
     * and PRT is unreferenced.  Then safe to unload PRT.
     */
    if (spmt->loaded) {
        pmt_clr(pmti);
        pciehw_prt_unload(spmt->pmt.pmre.bar.prtb,
                          spmt->pmt.pmre.bar.prtc);
        spmt->loaded = 0;
    }
}

static void
pmt_unload_cb(int pmti, pciehw_spmt_t *spmt, void *arg)
{
    pmt_unload(pmti, spmt);
}

void
pciehw_bar_unload_pmts(pciehwbar_t *phwbar)
{
    pciesvc_assert(phwbar->valid);
    pciehw_bar_foreach_pmt(phwbar, pmt_unload_cb, NULL);
}

void
pciehw_bar_load_ovrds(pciehwbar_t *phwbar)
{
    pciesvc_assert(phwbar->valid);
    if (phwbar->ovrds) {
        u_int16_t pmti = phwbar->ovrd;
        while (pmti != PMT_INVALID) {
            pciehw_spmt_t *spmt = pciesvc_spmt_get(pmti);

            pmt_load(pmti, spmt, phwbar->bdf);

            pmti = spmt->next;
            pciesvc_spmt_put(spmt, DIRTY); /* loaded */
        }
    }
}

void
pciehw_bar_unload_ovrds(pciehwbar_t *phwbar)
{
    pciesvc_assert(phwbar->valid);
    if (phwbar->ovrds) {
        u_int16_t pmti = phwbar->ovrd;
        while (pmti != PMT_INVALID) {
            pciehw_spmt_t *spmt = pciesvc_spmt_get(pmti);

            pmt_unload(pmti, spmt);

            pmti = spmt->next;
            pciesvc_spmt_put(spmt, DIRTY); /* loaded */
        }
    }
}

static int
spmt_dup_prts(const pciehw_spmt_t *ospmt, pciehw_spmt_t *nspmt)
{
    pciehw_shmem_t *pshmem = pciesvc_shmem_get();
    pciehw_sprt_t *osprt, *nsprt;
    pmr_bar_entry_t *pmr;
    pmt_t *pmt;
    int prti;

    pmt = &nspmt->pmt;
    pmr = &pmt->pmre.bar;
    prti = prt_alloc(pmr->prtc);
    if (prti < 0) {
        pciesvc_logerror("spmt_dup: prt_alloc %d failed\n", pmr->prtc);
        return -1;
    }
    osprt = PSHMEM_ADDR_FIELD(pshmem, sprt[pmr->prtb]);
    nsprt = PSHMEM_ADDR_FIELD(pshmem, sprt[prti]);
    pmr->prtb = prti;
    for (prti = pmr->prtb; prti < pmr->prtb + pmr->prtc; prti++) {
        pciesvc_memcpy(nsprt++, osprt++, sizeof(*nsprt));
    }
    return 0;
}

static pciehw_spmt_t *
spmt_get_dup(const pciehw_spmt_t *ospmt)
{
    pciehw_spmt_t *nspmt;
    int pmti;

    pmti = pmt_alloc(1, PMTPRI_VF0ADJ);
    if (pmti < 0) {
        pciesvc_logerror("spmt_dup: pmt_alloc failed\n");
        return NULL;
    }

    nspmt = pciesvc_spmt_get(pmti);
    pciesvc_memcpy(nspmt, ospmt, sizeof(*nspmt));
    nspmt->next = PMT_INVALID;

    if (spmt_dup_prts(ospmt, nspmt) < 0) {
        pmt_free(pmti, 1);
        return NULL;
    }

    return nspmt;
}

static pciehw_spmt_t *
spmt_get_next(pciehw_spmt_t *spmt)
{
    pciehw_spmt_t *nspmt;
    int pmti;

    pmti = spmt->next;
    if (pmti != PMT_INVALID) {
        return pciesvc_spmt_get(pmti);
    }
    nspmt = spmt_get_dup(spmt);
    if (nspmt) {
        spmt->next = spmt_to_pmti(nspmt);
        spmt->chain = 1;
    }
    return nspmt;
}

/*
 * Stub out any remaining chain pmt entries
 * by assigning 0 address.
 */
static void
pmt_adjust_nullify_chain(int pmti)
{
    const u_int64_t addr = 0ULL;
    const u_int64_t mask = ~0ULL;

    while (pmti != PMT_INVALID) {
        pciehw_spmt_t *spmt = pciesvc_spmt_get(pmti);
        pmt_t *pmt = &spmt->pmt;

        pmt_bar_setaddrm(pmt, addr, mask);

        pmti = spmt->next;
        pciesvc_spmt_put(spmt, DIRTY); /* pmt.addr/mask */
    }
}

static int
pmt_adjust_prt(pmt_t *pmt, prt_t *prt, const u_int64_t newval)
{
    int r = 0;

    switch (prt_type(prt)) {
    case PRT_TYPE_RES: {
        prt_res_t *res = &prt->res;
        res->addrdw = newval >> 2;
        break;
    }
    case PRT_TYPE_DB16:
    case PRT_TYPE_DB32:
    case PRT_TYPE_DB64: {
        prt_db_t *db = &prt->db;
        db->lif = newval;
        break;
    }
    default:
        break;
    }
    return r;
}

static int
pmt_adjust_prts(pmt_t *pmt, const u_int64_t newval)
{
    pmr_bar_entry_t *pmr = &pmt->pmre.bar;
    const int prtend = pmr->prtb + pmr->prtc;
    int prti, r;

    r = 0;
    for (prti = pmr->prtb; prti < prtend; prti++) {
        pciehw_sprt_t *sprt = pciesvc_sprt_get(pmr->prtb);
        r = pmt_adjust_prt(pmt, &sprt->prt, newval);
        pciesvc_sprt_put(sprt, DIRTY); /* addrdw/lif */
        if (r < 0) break;
    }
    return r;
}

/*
 * We want to add the "numvfs" entries in the pmt.
 * We want to match the address starting at "addr" and we need to
 * be careful about bits already set in "addr".  We can put in pmt
 * tcam "wildcard" masks to match bits in "addr" for contiguous power-of-2
 * numvfs, but if some bits are set in "addr" we will install a pmt
 * entry with exact match on the prefix up to that bit so we don't
 * claim additional address space that is not allocated to this sriov
 * vf group bar.
 */
static int
pmt_adjust_pmt(const pciehwdev_t *phwdev,
               pciehw_spmt_t *spmt,
               const u_int64_t addr,
               const int vfoffset,
               const int numvfs,
               const int do_log)
{
    pmt_t *pmt = &spmt->pmt;
    pmr_bar_entry_t *pmr = &pmt->pmre.bar;
    const u_int32_t vfstart = pmr->vfstart;
    u_int32_t numvfs2, numvfs2bitc, numvfs2end;
    u_int32_t addrvfs, nvfs, vfbitc, nvfend;
    u_int32_t ovfend, ovflimit, nvflimit;
    u_int64_t nvfmask, omask, nmask, nprtval;
    u_int16_t obdf, nbdf;
    int r;

    /*
     * Save pmtstart the first time through before any adjustments.
     * We'll keep try of pmtstart to create the original addr mask
     * for this entry.  We could arrange this to be set a init time
     * but we do this here at "runtime" when we start to configure sriov
     * to handle the case where we upgraded from an older fw that didn't
     * set "pmtstart" at init time and *then* the OS enables sriov.
     */
    if (spmt->pmtstart == 0) {
        const u_int64_t addrmask = pmt_bar_getaddrmask(pmt);
        spmt->pmtstart = pciesvc_ffsll(addrmask) - 1;
    }

    /*
     * Figure out how many bits for vf index to address "numvfs".
     * "numvfs2end" contains the upper bit of the vf index mask.
     */
    numvfs2 = roundup_power2(numvfs);
    numvfs2bitc = pciesvc_ffs(numvfs2) - 1;
    numvfs2end = vfstart + numvfs2bitc;
    /*
     * "addrvfs" contains the bits set in "addr" for the vf range.
     * We want to match on the existing bits in the address to
     * avoid any wildcard matches claiming other (incorrect) addresses.
     */
    addrvfs = (addr & ((1ULL << numvfs2end) - 1)) >> vfstart;
    /*
     * "nvfs" is the number of vfs covered by this entry.
     */
    nvfs = addrvfs ? 1 << (pciesvc_ffs(addrvfs) - 1) : rounddn_power2(numvfs);
    /*
     * Now that we have the real vf count in "nvfs" calculate the
     * new "nvfmask" that will select the vf index part of the "addr"
     * for this pmt.
     */
    vfbitc = pciesvc_ffs(nvfs) - 1;
    nvfend = vfstart + vfbitc;
    nvfmask = ((1ULL << vfbitc) - 1) << vfstart;
    /*
     * Compute the "nmask" new address mask for this pmt.
     * Start with the first bit of the old mask to
     * match the remaining upper bits of "addr".  Then AND off
     * the bits in "nvfmask" so the tcam entry will wildcard match
     * on the vf index field.  "nmask" will become the new pmt
     * mask used below in pmt_bar_setaddrm().
     */
    omask = ~((1ULL << spmt->pmtstart) - 1);
    nmask = omask & ~nvfmask;

    /* Compute the "nprtval" to be used for this "vfoffset". */
    nprtval = spmt->vf0base + (vfoffset << spmt->vf0stride);
    /* "nvflimit" is new vf limit for pmt */
    nvflimit = nvfs - 1;
    /* "nbdf" is adjusted completer bdf based on "vfoffset" */
    obdf = pciehwdev_get_hostbdf(phwdev);
    nbdf = obdf + vfoffset;
    /* save these original values for logging below */
    ovfend = pmr->vfend;
    ovflimit = pmr->vflimit;

    /* update pmt with adjusted values based on {addr, nvfs} */
    pmr->vfend = nvfend;
    pmr->vflimit = nvflimit;
    pmr->bdf = nbdf;
    pmt_bar_setaddrm(pmt, addr, nmask);

    r = pmt_adjust_prts(pmt, nprtval);
    if (do_log) {
        pciesvc_loginfo("%s: adjust bar %u addr 0x%" PRIx64 " numvfs %d/%d\n",
                        pciehwdev_get_name(phwdev), spmt->cfgidx, addr,
                        nvfs, numvfs);
        pciesvc_loginfo("  addr       0x%016" PRIx64 "\n", addr);
        pciesvc_loginfo("  omask      0x%016" PRIx64 "\n", omask);
        pciesvc_loginfo("  nmask      0x%016" PRIx64 "\n", nmask);
        pciesvc_loginfo("  nvfmask    0x%016" PRIx64 "\n", nvfmask);
        pciesvc_loginfo("  addrval    0x%016" PRIx64 " (0x%" PRIx64 ")\n",
                        addr & nvfmask,
                        (addr & nvfmask) >> vfstart);
        pciesvc_loginfo("  nvfs %u addrvfs %u\n", nvfs, addrvfs);
        pciesvc_loginfo("  vfstart %u vfend %u->%u vflimit %u->%u "
                        "bdf 0x%04x->0x%04x\n",
                        pmr->vfstart, ovfend, nvfend, ovflimit, nvflimit,
                        obdf, nbdf);
        pciesvc_loginfo("  vf0base 0x%" PRIx64
                        " nprtval 0x%" PRIx64 " r %d\n",
                        (u_int64_t)spmt->vf0base, nprtval, r);
    }
    return nvfs;
}

/*
 * We've set up this PMT entry to match *all* the VFs that belong
 * to this PF, taking advantage of the fact that the VF bars are
 * "virtual", guaranteed to be configured with a bar address that
 * is at a constant stride based on bar size.
 */
int
pciehw_pmt_adjust_vf0(pciehw_spmt_t *spmt,
                      u_int64_t addr,
                      const int numvfs,
                      const int do_log)
{
    const pciehwdev_t *phwdev = pciehwdev_get(spmt->owner);
    pmt_t *pmt = &spmt->pmt;
    pmr_bar_entry_t *pmr = &pmt->pmre.bar;
    const u_int32_t vfstart = pmr->vfstart;
    int vfoffset, nvfs_left, r;

    vfoffset = 0;
    nvfs_left = numvfs;
    r = numvfs;
    while (nvfs_left) {

        const int nvfs =
            pmt_adjust_pmt(phwdev, spmt, addr, vfoffset, nvfs_left, do_log);
        if (nvfs <= 0) {
            r = -1;
            break;
        }

        nvfs_left -= nvfs;
        vfoffset += nvfs;
        addr += nvfs << vfstart;

        /*
         * We didn't cover all the nvfs with the entry above.
         * Go fetch the next spmt on the list (or allocate a new
         * one if necessary) to use for the remainder.
         */
        if (nvfs_left) {
            pciehw_spmt_t *nspmt = spmt_get_next(spmt);
            if (nspmt == NULL) {
                r = -1;
                break;
            }
            /* set owner to adjusted vf dev */
            nspmt->owner = spmt->owner + nvfs;
            spmt = nspmt;
        }
    }

    /*
     * Deactivate remaining entries in the list, not needed for this config.
     * Host might have enabled more/fewer sriov vfs last time so the previous
     * config might have required more entries than we need to use right now.
     * We keep the entries chained on the spmt list because they might be
     * needed again if host enables more sriov vfs again in the future.
     * Enabling with more/fewer sriov vfs is possible in theory, but uncommon
     * in practice.  Normal case is max sriov vfs is always enabled and then
     * keep that config.
     */
    pmt_adjust_nullify_chain(spmt->next);

    pciehwdev_put(phwdev, CLEAN);
    return r;
}

static void
pmt_setaddr_cb(int pmti, pciehw_spmt_t *spmt, void *arg)
{
    const u_int64_t addr = *(u_int64_t *)arg;

    pmt_bar_setaddr(&spmt->pmt, addr + spmt->baroff);

    /* if loaded, update hw too */
    if (spmt->loaded) {
        pmt_set(pmti, &spmt->pmt);
    }
}

void
pciehw_pmt_setaddr(pciehwbar_t *phwbar, u_int64_t addr)
{
    pciehw_bar_foreach_pmt(phwbar, pmt_setaddr_cb, &addr);
}
