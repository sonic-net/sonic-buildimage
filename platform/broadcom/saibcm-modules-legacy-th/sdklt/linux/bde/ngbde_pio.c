/*! \file ngbde_pio.c
 *
 * API for managing and accessing memory-mapped I/O for switch
 * registers.
 *
 */
/*
 * Copyright 2018-2024 Broadcom. All rights reserved.
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

#include <ngbde.h>

void *
ngbde_pio_map(void *devh, phys_addr_t addr, phys_addr_t size)
{
    struct ngbde_dev_s *sd = (struct ngbde_dev_s *)devh;

    if (sd->pio_mem) {
        if (addr == sd->pio_win.addr && size == sd->pio_win.size) {
            /* Already mapped */
            return sd->pio_mem;
        }
        ngbde_pio_unmap(devh);
    }

    sd->pio_mem = ioremap(addr, size);

    if (sd->pio_mem) {
        /* Save mapped resources */
        sd->pio_win.addr = addr;
        sd->pio_win.size = size;
    } else {
        printk(KERN_WARNING "%s: Unable to map address 0x%08lu\n",
               MOD_NAME, (unsigned long)addr);
    }

    return sd->pio_mem;
}

void
ngbde_pio_unmap(void *devh)
{
    struct ngbde_dev_s *sd = (struct ngbde_dev_s *)devh;

    if (sd->pio_mem) {
        iounmap(sd->pio_mem);
        sd->pio_mem = NULL;
    }
}

void
ngbde_pio_cleanup(void)
{
    struct ngbde_dev_s *swdev, *sd;
    unsigned int num_swdev, idx;

    ngbde_swdev_get_all(&swdev, &num_swdev);

    for (idx = 0; idx < num_swdev; idx++) {
        sd = ngbde_swdev_get(idx);
        if (sd) {
            ngbde_pio_unmap(sd);
        }
    }
}

void
ngbde_pio_write32(void *devh, uint32_t offs, uint32_t val)
{
    struct ngbde_dev_s *sd = (struct ngbde_dev_s *)devh;

    if (sd->pio_mem) {
        NGBDE_IOWRITE32(val, sd->pio_mem + offs);
    }
}

uint32_t
ngbde_pio_read32(void *devh, uint32_t offs)
{
    struct ngbde_dev_s *sd = (struct ngbde_dev_s *)devh;

    if (sd->pio_mem) {
        return NGBDE_IOREAD32(sd->pio_mem + offs);
    }
    return 0;
}
