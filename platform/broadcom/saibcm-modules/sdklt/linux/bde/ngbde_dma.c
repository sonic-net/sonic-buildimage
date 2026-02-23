/*! \file ngbde_dma.c
 *
 * This module handles allocation of DMA memory pools.
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

/*! \cond */
static int dma_debug = 0;
module_param(dma_debug, int, S_IRUSR | S_IWUSR);
MODULE_PARM_DESC(dma_debug,
"DMA debug output enable (default 0).");
/*! \endcond */

/*! Default size of of DMA memory pools (in MB). */
#ifndef DMAPOOL_SIZE_DEFAULT
#define DMAPOOL_SIZE_DEFAULT    32
#endif

/*! Default number of DMA memory pools per device. */
#define NUM_DMAPOOL_DEFAULT     1

/*! \cond */
static int dma_size = DMAPOOL_SIZE_DEFAULT;
module_param(dma_size, int, S_IRUSR);
MODULE_PARM_DESC(dma_size,
"Size of of DMA memory pools in MB (default 32 MB).");
/*! \endcond */

/*! \cond */
static char *dma_alloc = "auto";
module_param(dma_alloc, charp, S_IRUSR);
MODULE_PARM_DESC(dma_alloc,
"DMA allocation method auto|kapi|pgmem (default auto)");
/*! \endcond */

/*! \cond */
static int dma_pools = NUM_DMAPOOL_DEFAULT;
module_param(dma_pools, int, S_IRUSR);
MODULE_PARM_DESC(dma_pools,
"Number of DMA memory pools to pre-allocate per device (default 1).");
/*! \endcond */

/*! \cond */
static int dma_32bit = 0;
module_param(dma_32bit, int, S_IRUSR);
MODULE_PARM_DESC(dma_32bit,
"Request DMA memory with 32-bit physical address (default 0).");
/*! \endcond */

/*!
 * \brief Allocate DMA memory via kernel API.
 *
 * \param [in] dmactrl DMA allocation control.
 * \param [out] dmamem DMA allocation result.
 *
 * \return Nothing.
 */
static void
ngbde_dmamem_kapi_alloc(ngbde_dmactrl_t *dmactrl, ngbde_dmamem_t *dmamem)
{
    void *vaddr;
    dma_addr_t baddr;

    vaddr = dma_alloc_attrs(dmactrl->dev, dmactrl->size, &baddr,
                            dmactrl->flags, DMA_FORCE_CONTIGUOUS);
    if (vaddr) {
        /* Store allocation information in dmamem structure */
        dmamem->vaddr = vaddr;
        dmamem->paddr = virt_to_phys(vaddr);
        dmamem->dev = dmactrl->dev;
        dmamem->size = dmactrl->size;
        dmamem->type = NGBDE_DMA_T_KAPI;
        dmamem->baddr = baddr;

        /* Write small signature for debug purposes */
        strscpy((char *)vaddr, "DMA_KAPI", dmactrl->size);

        if (dma_debug) {
            printk("DMA: Allocated %d KB of KAPI memory at 0x%08lx\n",
                   (int)(dmamem->size / ONE_KB),
                   (unsigned long)dmamem->paddr);
        }
    } else {
        if (dma_debug) {
            printk("DMA: Failed to allocate KAPI memory\n");
        }
    }
}

/*!
 * \brief Allocate DMA memory via page allocator.
 *
 * \param [in] dmactrl DMA allocation control.
 * \param [out] dmamem DMA allocation result.
 *
 * \return Nothing.
 */
static void
ngbde_dmamem_pgmem_alloc(ngbde_dmactrl_t *dmactrl, ngbde_dmamem_t *dmamem)
{
    void *vaddr;

    vaddr = ngbde_pgmem_alloc(dmactrl->size, dmactrl->flags);
    if (vaddr) {
        /* Store allocation information in dmamem structure */
        dmamem->vaddr = vaddr;
        dmamem->paddr = virt_to_phys(vaddr);
        dmamem->dev = dmactrl->dev;
        dmamem->size = dmactrl->size;
        dmamem->type = NGBDE_DMA_T_PGMEM;
        dmamem->baddr = dma_map_single(dmamem->dev, dmamem->vaddr,
                                       dmamem->size, DMA_BIDIRECTIONAL);
        if (dma_mapping_error(dmactrl->dev, dmamem->baddr)) {
            ngbde_pgmem_free(dmamem->vaddr);
            memset(dmamem, 0, sizeof(*dmamem));
            if (dma_debug) {
                printk("DMA: Failed to map PGMEM memory\n");
            }
            return;
        }

        /* Write small signature for debug purposes */
        strscpy((char *)vaddr, "DMA_PGMEM", dmactrl->size);

        if (dma_debug) {
            printk("DMA: Allocated %d KB of PGMEM memory at 0x%08lx\n",
                   (int)(dmamem->size / ONE_KB),
                   (unsigned long)dmamem->paddr);
        }
    } else {
        if (dma_debug) {
            printk("DMA: Failed to allocate PGMEM memory\n");
        }
    }
}

/*!
 * \brief Allocate DMA memory.
 *
 * Depending on the DMA allocation control parameters, we select one
 * of several DMA memory allocation methods.
 *
 * \param [in] dmactrl DMA allocation control.
 * \param [out] dmamem DMA allocation result.
 *
 * \return Nothing.
 */
static int
ngbde_dmamem_alloc(ngbde_dmactrl_t *dmactrl, ngbde_dmamem_t *dmamem)
{
    int kapi = 0;

    if (dmamem->vaddr) {
        /* Already allocated */
        return 0;
    }

#ifdef CONFIG_CMA
    /* Always allow KAPI when CMA is available */
    kapi = 1;
#else
    if (dmactrl->size <= (1 << (MAX_PAGE_ORDER - 1 + PAGE_SHIFT))) {
        kapi = 1;
    }
#endif

    /* Allocation via kernel DMA API (if allowed) */
    if (kapi) {
        switch (dmactrl->pref_type) {
        case NGBDE_DMA_T_AUTO:
        case NGBDE_DMA_T_KAPI:
            ngbde_dmamem_kapi_alloc(dmactrl, dmamem);
            break;
        default:
            break;
        }
    }

    /* Allocation via private page allocator */
    if (dmamem->vaddr == NULL) {
        switch (dmactrl->pref_type) {
        case NGBDE_DMA_T_AUTO:
        case NGBDE_DMA_T_PGMEM:
            ngbde_dmamem_pgmem_alloc(dmactrl, dmamem);
            break;
        default:
            break;
        }
    }

    if (dmamem->vaddr == NULL) {
        printk(KERN_WARNING "%s: Failed to allocate DMA memory\n",
               MOD_NAME);
        return -1;
    }

    return 0;
}

/*!
 * \brief Free DMA memory.
 *
 * Free DMA memory allocated via \ref ngbde_dmamem_alloc.
 *
 * \param [in] dmamem DMA allocation result from \ref ngbde_dmamem_alloc.
 *
 * \return Nothing.
 */
static int
ngbde_dmamem_free(ngbde_dmamem_t *dmamem)
{
    switch (dmamem->type) {
    case NGBDE_DMA_T_KAPI:
        if (dma_debug) {
            printk("DMA: Freeing %d KB of KAPI memory\n",
                   (int)(dmamem->size / ONE_KB));
        }
        dma_free_attrs(dmamem->dev, dmamem->size,
                       dmamem->vaddr, dmamem->baddr,
                       DMA_FORCE_CONTIGUOUS);
        memset(dmamem, 0, sizeof(*dmamem));
        break;
    case NGBDE_DMA_T_PGMEM:
        if (dma_debug) {
            printk("DMA: Freeing %d KB of PGMEM memory\n",
                   (int)(dmamem->size / ONE_KB));
        }
        if (dmamem->baddr) {
            if (dma_debug) {
                printk("DMA: Unmapping PGMEM memory at 0x%08lx\n",
                       (unsigned long)dmamem->baddr);
            }
            dma_unmap_single(dmamem->dev, dmamem->baddr,
                             dmamem->size, DMA_BIDIRECTIONAL);
        }
        ngbde_pgmem_free(dmamem->vaddr);
        memset(dmamem, 0, sizeof(*dmamem));
        break;
    case NGBDE_DMA_T_NONE:
        /* Nothing to free */
        break;
    default:
        printk(KERN_WARNING "%s: Unable to free unknown DMA memory type\n",
               MOD_NAME);
        break;
    }
    return 0;
}

/*!
 * \brief Free all DMA memory pools for all devices.
 *
 * \return Nothing.
 */
void
ngbde_dma_cleanup(void)
{
    struct ngbde_dev_s *swdev;
    unsigned int num_swdev, idx;
    unsigned int pool;

    ngbde_swdev_get_all(&swdev, &num_swdev);

    for (idx = 0; idx < num_swdev; idx++) {
        for (pool = 0; pool < NGBDE_NUM_DMAPOOL_MAX; pool++) {
            if (swdev[idx].inactive) {
                ngbde_dmamem_free(&swdev[idx].dmapool[pool].dmamem);
            }
        }
    }
}

/*!
 * \brief Allocate DMA memory pools for all devices.
 *
 * \return Nothing.
 */
int
ngbde_dma_init(void)
{
    int rv;
    struct ngbde_dev_s *swdev;
    unsigned int num_swdev, idx;
    int dma_type = NGBDE_DMA_T_AUTO;
    struct ngbde_dmapool_s *dmapool;
    unsigned int pool;

    /* Default DMA memory size per device */
    if (dma_size < 0) {
        dma_size = DMAPOOL_SIZE_DEFAULT;
    }

    /* Check for forced DMA allocation method */
    if (dma_alloc) {
        if (strcmp(dma_alloc, "auto") == 0) {
            dma_type = NGBDE_DMA_T_AUTO;
        } else if (strcmp(dma_alloc, "kapi") == 0) {
            dma_type = NGBDE_DMA_T_KAPI;
        } else if (strcmp(dma_alloc, "pgmem") == 0) {
            dma_type = NGBDE_DMA_T_PGMEM;
        } else {
            printk(KERN_WARNING "%s: Unknown DMA type: %s\n",
                   MOD_NAME, dma_alloc);
        }
    }

    /* Number of DMA memory pools per device */
    if ((unsigned int)dma_pools >= NGBDE_NUM_DMAPOOL_MAX) {
        dma_pools = NUM_DMAPOOL_DEFAULT;
    }

    ngbde_swdev_get_all(&swdev, &num_swdev);

    for (idx = 0; idx < num_swdev; idx++) {

        /* Set DMA allocation parameters */
        for (pool = 0; pool < NGBDE_NUM_DMAPOOL_MAX; pool++) {
            dmapool = &swdev[idx].dmapool[pool];
            dmapool->dmactrl.dev = swdev[idx].dma_dev;
            dmapool->dmactrl.size = dma_size * ONE_MB;
            dmapool->dmactrl.pref_type = dma_type;
            dmapool->dmactrl.flags = GFP_KERNEL;
            if (dma_32bit) {
                dmapool->dmactrl.flags |= GFP_DMA32;
            }
            if (dma_type == NGBDE_DMA_T_AUTO && dma_debug == 0) {
                /* Prevent warning if CMA is unavailable */
                dmapool->dmactrl.flags |= __GFP_NOWARN;
            }
        }

        /* Allocate DMA pools */
        for (pool = 0; pool < dma_pools; pool++) {
            dmapool = &swdev[idx].dmapool[pool];
            rv = ngbde_dmamem_alloc(&dmapool->dmactrl, &dmapool->dmamem);
            if (rv < 0) {
                printk(KERN_WARNING "%s: Unable to allocate DMA pool %d %d\n",
                       MOD_NAME, idx, pool);
            }
        }
    }
    return 0;
}
