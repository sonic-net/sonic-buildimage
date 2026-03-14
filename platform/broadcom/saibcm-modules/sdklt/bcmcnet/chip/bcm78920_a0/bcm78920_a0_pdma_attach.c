/*! \file bcm78920_a0_pdma_attach.c
 *
 * Chip stub for packet DMA driver.
 *
 */
/*
 *
 * Copyright 2018-2025 Broadcom. All rights reserved.
 * The term 'Broadcom' refers to Broadcom Inc. and/or its subsidiaries.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License 
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * A copy of the GNU General Public License version 2 (GPLv2) can
 * be found in the LICENSES folder.
 */

#ifdef UNFINISHED_DRIVER_CHECK
#error DRIVER UNFINISHED
#endif

#include <bcmcnet/bcmcnet_core.h>
#include <bcmcnet/bcmcnet_dev.h>
#include <bcmcnet/bcmcnet_cmicr.h>

int
bcm78920_a0_cnet_pdma_attach(struct pdma_dev *dev)
{
    return bcmcnet_cmicr_pdma_driver_attach(dev);
}

int
bcm78920_a0_cnet_pdma_detach(struct pdma_dev *dev)
{
    return bcmcnet_cmicr_pdma_driver_detach(dev);
}

