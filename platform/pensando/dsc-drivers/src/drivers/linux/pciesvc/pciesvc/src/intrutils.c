// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017-2022, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "intrutils.h"

#define INTR_BASE               ASIC_(ADDR_BASE_INTR_INTR_OFFSET)
#define INTR_COUNT              ASIC_(INTR_CSR_DHS_INTR_ASSERT_ENTRY_ARRAY_COUNT)

#define INTR_MSIXCFG_OFFSET     ASIC_(INTR_CSR_DHS_INTR_MSIXCFG_BYTE_OFFSET)
#define INTR_MSIXCFG_BASE       (INTR_BASE + INTR_MSIXCFG_OFFSET)
#define INTR_MSIXCFG_STRIDE     0x10

#define INTR_FWCFG_OFFSET       ASIC_(INTR_CSR_DHS_INTR_FWCFG_BYTE_OFFSET)
#define INTR_FWCFG_BASE         (INTR_BASE + INTR_FWCFG_OFFSET)
#define INTR_FWCFG_STRIDE       0x8

#define INTR_DRVCFG_OFFSET      ASIC_(INTR_CSR_DHS_INTR_DRVCFG_BYTE_OFFSET)
#define INTR_DRVCFG_BASE        (INTR_BASE + INTR_DRVCFG_OFFSET)
#define INTR_DRVCFG_STRIDE      0x20

#define INTR_ASSERT_OFFSET      ASIC_(INTR_CSR_DHS_INTR_ASSERT_BYTE_OFFSET)
#define INTR_ASSERT_BASE        (INTR_BASE + INTR_ASSERT_OFFSET)
#define INTR_ASSERT_STRIDE      0x4
#define INTR_ASSERT_DATA        0x00000001 /* in little-endian */

#define INTR_STATE_OFFSET       ASIC_(INTR_CSR_DHS_INTR_STATE_BYTE_OFFSET)
#define INTR_STATE_BASE         (INTR_BASE + INTR_STATE_OFFSET)
#define INTR_STATE_STRIDE       0x10

#define NWORDS(a)               (sizeof(a) / sizeof(u_int32_t))

static u_int64_t
intr_msixcfg_addr(const int intrb)
{
    pciesvc_assert(intrb < INTR_COUNT);
    return INTR_MSIXCFG_BASE + (intrb * INTR_MSIXCFG_STRIDE);
}

static u_int64_t
intr_fwcfg_addr(const int intrb)
{
    pciesvc_assert(intrb < INTR_COUNT);
    return INTR_FWCFG_BASE + (intrb * INTR_FWCFG_STRIDE);
}

static u_int64_t
intr_drvcfg_addr(const int intrb)
{
    pciesvc_assert(intrb < INTR_COUNT);
    return INTR_DRVCFG_BASE + (intrb * INTR_DRVCFG_STRIDE);
}

/*
 * Set the drvcfg_mask for this interrupt resource.
 * Return the previous value of the mask so caller can
 * restore to previous value if desired.
 */
int
intr_drvcfg_mask(const int intr, const int on)
{
    const u_int64_t pa = intr_drvcfg_addr(intr);
    const int omask = pciesvc_reg_rd32(pa + offsetof(intr_drvcfg_t, mask));

    pciesvc_reg_wr32(pa + offsetof(intr_drvcfg_t, mask), on);
    return omask;
}

static void
intr_msixcfg(const int intr,
             const u_int64_t msgaddr, const u_int32_t msgdata, const int vctrl)
{
    const u_int64_t pa = intr_msixcfg_addr(intr);
#define MSG_ADDR_OFF    0
#define MSG_DATA_OFF    8
#define VECTOR_CTRL_OFF 12

    pciesvc_reg_wr64(pa + MSG_ADDR_OFF, msgaddr);
    pciesvc_reg_wr32(pa + MSG_DATA_OFF, msgdata);
    pciesvc_reg_wr32(pa + VECTOR_CTRL_OFF, vctrl);
}

static void
intr_fwcfg_set_function_mask(const int intr, const int on)
{
    const u_int64_t pa = intr_fwcfg_addr(intr);
    pciesvc_reg_wr32(pa, on);
}

static u_int64_t
intr_assert_addr(const int intr)
{
    pciesvc_assert(intr < INTR_COUNT);
    return INTR_ASSERT_BASE + (intr * INTR_ASSERT_STRIDE);
}

static u_int32_t
intr_assert_data(void)
{
    return INTR_ASSERT_DATA;
}

void
intr_assert(const int intr)
{
    const u_int64_t pa = intr_assert_addr(intr);
    const u_int32_t data = intr_assert_data();

    pciesvc_reg_wr32(pa, data);
}

/*
 * Change the mode of the interrupt between legacy and msi mode.
 *
 * Note:  We are careful to make config changes to fwcfg only with
 * the function_mask set.  Masking the interrupt will deassert the
 * interrupt if asserted in legacy mode, then we change any config,
 * then re-enable with the new config.  If necessary the interrupt
 * will re-assert with the new config.
 */
void
intr_fwcfg_mode(const int intr, const int legacy, const int fmask)
{
    const u_int64_t pa = intr_fwcfg_addr(intr);
    intr_fwcfg_t v;

    /* mask via function_mask while making changes */
    intr_fwcfg_set_function_mask(intr, 1);
    {
        pciesvc_reg_rd32w(pa, v.w, NWORDS(v.w));
        v.legacy = legacy;
        pciesvc_reg_wr32w(pa, v.w, NWORDS(v.w));
    }
    if (!fmask) {
        intr_fwcfg_set_function_mask(intr, fmask);
    }
}

/*****************************************************************
 * Reset section
 */

/*****************
 * pba
 */

/*
 * Reset this interrupt's contribution to the interrupt status
 * Pending Bit Array (PBA).  We clear the PBA bit for this interrupt
 * resource by returning all the "credits" for the interrupt.
 *
 * The driver interface to return credits is drvcfg.int_credits,
 * but that register has special semantics where the value written
 * to this register is atomically subtracted from the current value.
 * We could use this interface to read the value X then write X back
 * to the register to X - X = 0.  This works even for negative values
 * since (-X) - (-X) = 0.
 */
static void
intr_pba_clear(const int intr)
{
    const u_int64_t pa = intr_drvcfg_addr(intr);
    u_int32_t credits;

    credits = pciesvc_reg_rd32(pa + offsetof(intr_drvcfg_t, int_credits));
    if (credits) {
        pciesvc_reg_wr32(pa + offsetof(intr_drvcfg_t, int_credits), credits);
    }
}

void
intr_deassert(const int intr)
{
    intr_pba_clear(intr);
}

/*****************
 * msixcfg
 */

/*
 * Reset the msix control register group.  This group is usually
 * owned by the host OS and the behavior, including these reset values,
 * are specified by the PCIe spec.
 */
static void
reset_msixcfg(const int intr)
{
    /* clear msg addr/data, vector_ctrl mask=1 */
    intr_msixcfg(intr, 0, 0, 1);
}

static void
intr_reset_msixcfg(const int intrb, const int intrc)
{
    int intr;

    for (intr = intrb; intr < intrb + intrc; intr++) {
        reset_msixcfg(intr);
    }
}

/*****************
 * intr mode
 */

/*
 * Reset the interrupt "mode" to "legacy".
 */
static void
reset_mode(const int intr)
{
    /* reset to legacy mode, no fmask (CMD.int_disable == 0) */
    intr_fwcfg_mode(intr, 1, 0);
}

static void
intr_reset_mode(const int intrb, const int intrc)
{
    int intr;

    for (intr = intrb; intr < intrb + intrc; intr++) {
        reset_mode(intr);
    }
}

/*****************
 * external reset apis
 */

void
intr_reset_pci(const int intrb, const int intrc, const int dmask)
{
    intr_reset_msixcfg(intrb, intrc);
    intr_reset_mode(intrb, intrc);
}
