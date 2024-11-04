// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017,2021, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "hdrt.h"

#define HDRT_BASE       PXB_(DHS_ITR_PCIHDRT)
#define HDRT_COUNT      ASIC_(PXB_CSR_DHS_ITR_PCIHDRT_ENTRIES)
#define HDRT_STRIDE     ASIC_(PXB_CSR_DHS_ITR_PCIHDRT_ENTRY_BYTE_SIZE)
#define HDRT_NWORDS     3

static int
hdrt_size(void)
{
    return HDRT_COUNT;
}

static u_int64_t
hdrt_addr(const u_int32_t lif)
{
    pciesvc_assert(lif < hdrt_size());
    return HDRT_BASE + (lif * HDRT_STRIDE);
}

static void
hdrt_set(const u_int32_t lif, const hdrt_t *hdrt)
{
    pciesvc_reg_wr32w(hdrt_addr(lif), (u_int32_t *)hdrt, HDRT_NWORDS);
}

static void
hdrt_set_itr(const u_int32_t lif, const u_int16_t bdf)
{
    hdrt_t h = { 0 };

    h.valid = 1;
    h.bdf = bdf;
    h.attr2_1_rd = 0x1; /* reads get Relaxed Ordering */
    hdrt_set(lif, &h);
}

/******************************************************************
 * apis
 */

int
pciehw_hdrt_load(const u_int32_t lifb,
                 const u_int32_t lifc,
                 const u_int16_t bdf)
{
    u_int32_t lif;

    for (lif = lifb; lif < lifb + lifc; lif++) {
        hdrt_set_itr(lif, bdf);
    }
    return 0;
}

int
pciehw_hdrt_unload(const u_int32_t lifb, const u_int32_t lifc)
{
    const hdrt_t h0 = { 0 };
    u_int32_t lif;

    for (lif = lifb; lif < lifb + lifc; lif++) {
        hdrt_set(lif, &h0);
    }
    return 0;
}
