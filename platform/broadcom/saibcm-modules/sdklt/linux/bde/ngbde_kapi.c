/*! \file ngbde_kapi.c
 *
 * Public BDE kernel API for use with other kernel modules.
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

#include <ngbde.h>

#include <lkm/ngbde_kapi.h>

struct pci_dev *
ngbde_kapi_pci_dev_get(int kdev)
{
    struct ngbde_dev_s *sd;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return NULL;
    }

    return sd->pci_dev;
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_pci_dev_get);
/*! \endcond */

struct device *
ngbde_kapi_dma_dev_get(int kdev)
{
    struct ngbde_dev_s *sd;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return NULL;
    }

    return sd->dma_dev;
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_dma_dev_get);
/*! \endcond */

void *
ngbde_kapi_dma_bus_to_virt(int kdev, dma_addr_t baddr)
{
    struct ngbde_dev_s *sd;
    struct ngbde_dmamem_s *dmamem;
    size_t dma_offset;
    int idx;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return NULL;
    }

    for (idx = 0; idx < NGBDE_NUM_DMAPOOL_MAX; idx++) {
        dmamem = &sd->dmapool[idx].dmamem;
        dma_offset = baddr - dmamem->baddr;
        if (dma_offset < dmamem->size) {
            return (uint8_t *)dmamem->vaddr + dma_offset;
        }
    }
    return NULL;
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_dma_bus_to_virt);
/*! \endcond */

dma_addr_t
ngbde_kapi_dma_virt_to_bus(int kdev, void *vaddr)
{
    struct ngbde_dev_s *sd;
    struct ngbde_dmamem_s *dmamem;
    size_t dma_offset;
    int idx;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return 0UL;
    }

    for (idx = 0; idx < NGBDE_NUM_DMAPOOL_MAX; idx++) {
        dmamem = &sd->dmapool[idx].dmamem;
        dma_offset = (uintptr_t)vaddr - (uintptr_t)dmamem->vaddr;
        if (dma_offset < dmamem->size) {
            return dmamem->baddr + dma_offset;
        }
    }
    return 0UL;
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_dma_virt_to_bus);
/*! \endcond */

void *
ngbde_kapi_dma_alloc(size_t size)
{
    return ngbde_pgmem_alloc(size, GFP_KERNEL | GFP_DMA32);
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_dma_alloc);
/*! \endcond */

int
ngbde_kapi_dma_free(void *ptr)
{
    return ngbde_pgmem_free(ptr);
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_dma_free);
/*! \endcond */

void
ngbde_kapi_pio_write32(int kdev, uint32_t offs, uint32_t val)
{
    struct ngbde_dev_s *sd;

    sd = ngbde_swdev_get(kdev);
    if (sd) {
        return ngbde_pio_write32(sd, offs, val);
    }
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_pio_write32);
/*! \endcond */

uint32_t
ngbde_kapi_pio_read32(int kdev, uint32_t offs)
{
    struct ngbde_dev_s *sd;

    sd = ngbde_swdev_get(kdev);
    if (sd) {
        return ngbde_pio_read32(sd, offs);
    }

    return (uint32_t)-1;
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_pio_read32);
/*! \endcond */

void *
ngbde_kapi_pio_membase(int kdev)
{
    struct ngbde_dev_s *sd;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return NULL;
    }

    return sd->pio_mem;
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_pio_membase);
/*! \endcond */

void
ngbde_kapi_iio_write32(int kdev, uint32_t offs, uint32_t val)
{
    struct ngbde_dev_s *sd;

    sd = ngbde_swdev_get(kdev);
    if (sd) {
        return ngbde_iio_write32(sd, offs, val);
    }
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_iio_write32);
/*! \endcond */

uint32_t
ngbde_kapi_iio_read32(int kdev, uint32_t offs)
{
    struct ngbde_dev_s *sd;

    sd = ngbde_swdev_get(kdev);
    if (sd) {
        return ngbde_iio_read32(sd, offs);
    }

    return (uint32_t)-1;
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_iio_read32);
/*! \endcond */

void *
ngbde_kapi_iio_membase(int kdev)
{
    struct ngbde_dev_s *sd;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return NULL;
    }

    return sd->iio_mem;
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_iio_membase);
/*! \endcond */

void
ngbde_kapi_paxb_write32(int kdev, uint32_t offs, uint32_t val)
{
    struct ngbde_dev_s *sd;

    sd = ngbde_swdev_get(kdev);
    if (sd) {
        return ngbde_paxb_write32(sd, offs, val);
    }
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_paxb_write32);
/*! \endcond */

uint32_t
ngbde_kapi_paxb_read32(int kdev, uint32_t offs)
{
    struct ngbde_dev_s *sd;

    sd = ngbde_swdev_get(kdev);
    if (sd) {
        return ngbde_paxb_read32(sd, offs);
    }

    return (uint32_t)-1;
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_paxb_read32);
/*! \endcond */

void *
ngbde_kapi_paxb_membase(int kdev)
{
    struct ngbde_dev_s *sd;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return NULL;
    }

    return sd->paxb_mem;
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_paxb_membase);
/*! \endcond */

int
ngbde_kapi_intr_connect(int kdev, unsigned int irq_num,
                        int (*isr_func)(void *), void *isr_data)
{
    struct ngbde_dev_s *sd;
    struct ngbde_intr_ctrl_s *ic;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return -1;
    }

    if (irq_num >= NGBDE_NUM_IRQS_MAX) {
        return -1;
    }

    ic = &sd->intr_ctrl[irq_num];
    ic->isr_func = isr_func;
    ic->isr_data = isr_data;

    return 0;
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_intr_connect);
/*! \endcond */

int
ngbde_kapi_intr_disconnect(int kdev, unsigned int irq_num)
{
    struct ngbde_dev_s *sd;
    struct ngbde_intr_ctrl_s *ic;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return -1;
    }

    if (irq_num >= NGBDE_NUM_IRQS_MAX) {
        return -1;
    }

    ic = &sd->intr_ctrl[irq_num];
    ic->isr_func = NULL;
    ic->isr_data = NULL;

    return 0;
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_intr_disconnect);
/*! \endcond */

int
ngbde_kapi_intr_mask_write(int kdev, unsigned int irq_num,
                           uint32_t status_reg, uint32_t mask_val)
{
    return ngbde_intr_mask_write(kdev, irq_num, 1, status_reg, mask_val);
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_intr_mask_write);
/*! \endcond */

int
ngbde_kapi_knet_connect(int kdev, knet_func_f knet_func, void *knet_data)
{
    struct ngbde_dev_s *sd;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return -1;
    }
    sd->knet_func = knet_func;
    sd->knet_data = knet_data;

    return 0;
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_knet_connect);
/*! \endcond */

int
ngbde_kapi_knet_disconnect(int kdev)
{
    struct ngbde_dev_s *sd;

    sd = ngbde_swdev_get(kdev);
    if (!sd) {
        return -1;
    }
    sd->knet_func = NULL;
    sd->knet_data = NULL;

    return 0;
}
/*! \cond */
EXPORT_SYMBOL(ngbde_kapi_knet_disconnect);
/*! \endcond */

