// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2018,2021, Pensando Systems Inc.
 */

#include "pciesvc_impl.h"
#include "intr.h"
#include "intrutils.h"

static void
intr_config(const u_int32_t intrb,
            const u_int32_t intrc,
            const int legacy,
            const int fmask)
{
    u_int32_t intr;

    for (intr = intrb; intr < intrb + intrc; intr++) {
        intr_fwcfg_mode(intr, legacy, fmask);
    }
}

void
pciehw_intr_config(pciehwdev_t *phwdev, const int legacy, const int fmask)
{
    int i;

    if (phwdev->novrdintr) {
        for (i = 0; i < phwdev->novrdintr; i++) {
            const u_int32_t intrb = phwdev->ovrdintr[i].intrb;
            const u_int32_t intrc = phwdev->ovrdintr[i].intrc;

            intr_config(intrb, intrc, legacy, fmask);
        }
    } else {
        intr_config(phwdev->intrb, phwdev->intrc, legacy, fmask);
    }
}
