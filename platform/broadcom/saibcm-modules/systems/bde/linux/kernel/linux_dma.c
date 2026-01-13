/*
 * $Id: linux_dma.c,v 1.414 Broadcom SDK $
 *
 * $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 *
 * Linux Kernel BDE DMA memory allocation
 *
 *
 * DMA memory allocation modes
 * ===========================
 *
 * 1. Using private pool in kernel memory
 * --------------------------------------
 * In this mode the BDE module will try to assemble a physically contiguous
 * of memory using the kernel page allocator. This memory block is then
 * administered by the mpool allocation functions. Note that once a system
 * has been running for a while, the memory fragmentation may prevent the
 * allocator from assembling a contiguous memory block, however, if the
 * module is loaded shortly after system startup, it is very unlikely to
 * fail.
 *
 * This allocation method is used by default.
 *
 * 2. Using private pool in high memory
 * ------------------------------------
 * In this mode the BDE module will assume that unused physical memory is
 * present at the high_memory address, i.e. memory not managed by the Linux
 * memory manager. This memory block is mapped into kernel space and
 * administered by the mpool allocation functions. High memory must be
 * reserved using either the mem=xxx kernel parameter (recommended), or by
 * hardcoding the memory limit in the kernel image.
 *
 * The module parameter himem=1 enables this allocation mode.
 *
 * 3. Using kernel allocators (kmalloc, __get_free_pages)
 * ------------------------------------------------------
 * In this mode all DMA memory is allocated from the kernel on the fly, i.e.
 * no private DMA memory pool will be created. If large memory blocks are
 * only allocated at system startup (or not at all), this allocation method
 * is the most flexible and memory-efficient, however, it is not recommended
 * for non-coherent memory platforms due to an overall system performance
 * degradation arising from the use of cache flush/invalidate instructions.
 *
 * The module parameter dmasize=0M enables this allocation mode, however if
 * DMA memory is requested from a user mode application, a private memory
 * pool will be created and used irrespectively.
 * dmasize=sram may be used with some device, with no direct PCIe connection
 * between the CPU and device, for using iproc SRAM for DMA.
 */

#include <gmodule.h>
#include <linux-bde.h>
#include <linux_dma.h>
#include <mpool.h>
#include <sdk_config.h>

#if defined(IPROC_CMICD) && defined(CONFIG_OF)
#include <linux/of.h>
#endif

#ifdef BCM_PLX9656_LOCAL_BUS
#include <asm/cacheflush.h>
#endif

/* allocation types/methods for the DMA memory pool */
#define ALLOC_TYPE_CHUNK 0 /* use small allocations and join them */
#define ALLOC_TYPE_API 1 /* use one allocation */
#define ALLOC_TYPE_HIMEM 2 /* use high memory */

#if _SIMPLE_MEMORY_ALLOCATION_
#include <linux/dma-mapping.h>
#ifndef CONFIG_CMA
#define DMA_MAX_ALLOC_SIZE (1 << (MAX_PAGE_ORDER - 1 + PAGE_SHIFT)) /* Maximum size the kernel can allocate in one allocation */
#endif /* !CONFIG_CMA */
#endif /* _SIMPLE_MEMORY_ALLOCATION_ */

#if _SIMPLE_MEMORY_ALLOCATION_ == 1
/* Use Linux DMA API to allocate contiguous memory */
#define ALLOC_METHOD_DEFAULT ALLOC_TYPE_API
#if defined(__arm__)
#define USE_DMA_MMAP_COHERENT
#define _PGPROT_NONCACHED(x) x = pgprot_noncached((x))
#elif defined(__aarch64__ )
#define USE_DMA_MMAP_COHERENT
#define _PGPROT_NONCACHED(x) x = pgprot_writecombine((x))
#endif
#else
#define ALLOC_METHOD_DEFAULT ALLOC_TYPE_CHUNK
#endif

#ifndef _PGPROT_NONCACHED
#ifdef REMAP_DMA_NONCACHED
#define _PGPROT_NONCACHED(x) x = pgprot_noncached((x))
#else
#define _PGPROT_NONCACHED(x)
#endif
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,0))
#include <linux/slab.h>
#define virt_to_bus virt_to_phys
#define bus_to_virt phys_to_virt
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,21))
#define VIRT_TO_PAGE(p)     virt_to_page((void*)(p))
#else
#define VIRT_TO_PAGE(p)     virt_to_page((p))
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27))
#define BDE_DMA_MAPPING_ERROR(d, p)     dma_mapping_error((d),(p))
#else
#define BDE_DMA_MAPPING_ERROR(d, p)     dma_mapping_error((p))
#endif

#ifndef KMALLOC_MAX_SIZE
#define KMALLOC_MAX_SIZE (1UL << (MAX_PAGE_ORDER - 1 + PAGE_SHIFT))
#endif

/* Compatibility */
#ifdef LKM_2_4
#define MEM_MAP_RESERVE mem_map_reserve
#define MEM_MAP_UNRESERVE mem_map_unreserve
#else /* LKM_2_6 */
#define MEM_MAP_RESERVE SetPageReserved
#define MEM_MAP_UNRESERVE ClearPageReserved
#endif /* LKM_2_x */

#ifndef GFP_DMA32
#define GFP_DMA32 0
#endif

/* Flags for memory allocations */
#ifdef SAL_BDE_XLP
static int mem_flags = GFP_ATOMIC | GFP_KERNEL | GFP_DMA;
#else
#if defined(CONFIG_ZONE_DMA32)
static int mem_flags = GFP_ATOMIC | GFP_DMA32;
#else
static int mem_flags = GFP_ATOMIC | GFP_DMA;
#endif
#endif

#ifdef IPROC_CMICD
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
#ifndef COHERENT_ALLOC_USE_DMA_TO_PHYS
#define COHERENT_ALLOC_USE_DMA_TO_PHYS 1
#endif
#else
#ifndef COHERENT_ALLOC_PHYS_IS_DMA_ADDR
#define COHERENT_ALLOC_PHYS_IS_DMA_ADDR 1
#endif
#endif
#endif /* IPROC_CMICD */

/*
 * Macros that can be used by the build to control the technique of
 * setting the physical address for dma_alloc_coherent()
 */
#ifndef COHERENT_ALLOC_USE_DMA_TO_PHYS
#define COHERENT_ALLOC_USE_DMA_TO_PHYS 0
#endif
#ifndef COHERENT_ALLOC_PHYS_IS_DMA_ADDR
#define COHERENT_ALLOC_PHYS_IS_DMA_ADDR 0
#endif

#if COHERENT_ALLOC_USE_DMA_TO_PHYS
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,16,0))
#include <linux/dma-direct.h>
#else
#include <asm/dma-mapping.h>
#endif
#endif

/* Macro to get the host physical address when using dma_alloc_coherent() */
#if COHERENT_ALLOC_USE_DMA_TO_PHYS
#define HOST_PHYS_ADDR(_dev, _dma_a, _kvirt_a) \
    ((_dev) ? dma_to_phys((_dev), (_dma_a)) : virt_to_phys(_kvirt_a))
#elif COHERENT_ALLOC_PHYS_IS_DMA_ADDR
#define HOST_PHYS_ADDR(_dev, _dma_a, _kvirt_a) (_dma_a)
#else
#define HOST_PHYS_ADDR(_dev, _dma_a, _kvirt_a) (virt_to_phys(_kvirt_a))
#endif

/* Debug output */
static int dma_debug = 0;
module_param(dma_debug, int, 0);
MODULE_PARM_DESC(dma_debug,
"DMA debug output enable (default 0).");

/* DMA memory pool size */
static char *dmasize;
LKM_MOD_PARAM(dmasize, "s", charp, 0);
MODULE_PARM_DESC(dmasize,
"Specify DMA memory size (default 4MB)");

/* Select DMA memory pool allocation method */
static int dmaalloc = ALLOC_METHOD_DEFAULT;
LKM_MOD_PARAM(dmaalloc, "i", int, 0);
MODULE_PARM_DESC(dmaalloc, "Select DMA memory allocation method");

/* Use high memory for DMA */
static char *himem;
LKM_MOD_PARAM(himem, "s", charp, 0);
MODULE_PARM_DESC(himem,
"Use high memory for DMA (default no)");

/* Physical high memory address to use for DMA */
static char *himemaddr = 0;
LKM_MOD_PARAM(himemaddr, "s", charp, 0);
MODULE_PARM_DESC(himemaddr,
"Physical address to use for high memory DMA");

/* DMA memory allocation */

#define ONE_KB 1024
#define ONE_MB (1024*1024)
#define ONE_GB (1024*1024*1024)

/* Default DMA memory size */
#ifdef SAL_BDE_DMA_MEM_DEFAULT
#define DMA_MEM_DEFAULT (SAL_BDE_DMA_MEM_DEFAULT * ONE_MB)
#else
#define DMA_MEM_DEFAULT (8 * ONE_MB)
#endif

#ifdef BDE_EDK_SUPPORT
typedef struct {
    phys_addr_t  cpu_pbase; /* CPU physical base address of the DMA pool */
    phys_addr_t  dma_pbase; /* Bus base address of the DMA pool */
    void __iomem *dma_vbase;
    uint32  size; /* Total size of the pool */
}_edk_dma_pool_t;
static _edk_dma_pool_t _edk_dma_pool[LINUX_BDE_MAX_DEVICES];
static int _edk_use_dma_mapping = 0;
#endif

/* We try to assemble a contiguous segment from chunks of this size */
#define DMA_BLOCK_SIZE (512 * ONE_KB)

typedef struct _dma_segment {
    struct list_head list;
    unsigned long req_size;     /* Requested DMA segment size */
    unsigned long blk_size;     /* DMA block size */
    unsigned long blk_order;    /* DMA block size in alternate format */
    unsigned long seg_size;     /* Current DMA segment size */
    unsigned long seg_begin;    /* Logical address of segment */
    unsigned long seg_end;      /* Logical end address of segment */
    unsigned long *blk_ptr;     /* Array of logical DMA block addresses */
    int blk_cnt_max;            /* Maximum number of block to allocate */
    int blk_cnt;                /* Current number of blocks allocated */
} dma_segment_t;

static unsigned int _dma_mem_size = DMA_MEM_DEFAULT;
static mpool_handle_t _dma_pool = NULL;
/* kernel virtual address of the DMA buffer pool */
static void __iomem *_dma_vbase = NULL;
/* CPU physical address of the DMA buffer pool, used for mmap */
static phys_addr_t _cpu_pbase = 0;
/*
 * DMA buffer pool PCIe bus address, it is either identical to the CPU
 * physical address or another address(IOVA) translated by IOMMU.
 */
static phys_addr_t _dma_pbase = 0;

#ifdef INCLUDE_SRAM_DMA
/* If the value is none-zero, device types that support it will use SRAM for DMA.
 * This is for systems where the CPU has no PCIe connection to the device,
 * and where customers implement some proxy the connects to the device */
int use_sram_for_dma = 0;
#endif /* INCLUDE_SRAM_DMA */

/* states of the DMA pool: */

/* not initialized */
#define DNA_POOL_INVALID 0
/* initialized and not yet mapped */
#define DMA_POOL_INITIALIZED 1
/* initialized and mapped */
#define DMA_POOL_MAPPED 2
/* initialization failed */
#define DMA_POOL_FAILED 3

/* Was the DMA buffer pool allocation attempted? */
static int _dma_pool_alloc_state = DNA_POOL_INVALID;

static int _use_himem = 0;
static unsigned long _himemaddr = 0;
/* None-zero if the DMA buffer pool is not mapped to the bus address by the
   allocation, and needs to be mapped per device. */
static int _use_dma_mapping = 0;
static LIST_HEAD(_dma_seg);

extern int nodevices;
#if _SIMPLE_MEMORY_ALLOCATION_
static struct device *_dma_alloc_coherent_device = NULL;
#endif /* _SIMPLE_MEMORY_ALLOCATION_ */

#define DMA_DEV_INDEX      0    /* Device index to allocate memory pool */
#define DMA_DEV(n)         lkbde_get_dma_dev(n)
#define BDE_NUM_DEVICES(t) lkbde_get_num_devices(t)

/*
 * Function: _find_largest_segment
 *
 * Purpose:
 *    Find largest contiguous segment from a pool of DMA blocks.
 * Parameters:
 *    dseg - DMA segment descriptor
 * Returns:
 *    0 on success, < 0 on error.
 * Notes:
 *    Assembly stops if a segment of the requested segment size
 *    has been obtained.
 *
 *    Lower address bits of the DMA blocks are used as follows:
 *       0: Untagged
 *       1: Discarded block
 *       2: Part of largest contiguous segment
 *       3: Part of current contiguous segment
 */
static int
_find_largest_segment(dma_segment_t *dseg)
{
    int i, j, blks, found;
    unsigned long b, e, a;

    blks = dseg->blk_cnt;
    /* Clear all block tags */
    for (i = 0; i < blks; i++) {
        dseg->blk_ptr[i] &= ~3;
    }
    for (i = 0; i < blks && dseg->seg_size < dseg->req_size; i++) {
        /* First block must be an untagged block */
        if ((dseg->blk_ptr[i] & 3) == 0) {
            /* Initial segment size is the block size */
            b = dseg->blk_ptr[i];
            e = b + dseg->blk_size;
            dseg->blk_ptr[i] |= 3;
            /* Loop looking for adjacent blocks */
            do {
                found = 0;
                for (j = i + 1; j < blks && (e - b) < dseg->req_size; j++) {
                    a = dseg->blk_ptr[j];
                    /* Check untagged blocks only */
                    if ((a & 3) == 0) {
                        if (a == (b - dseg->blk_size)) {
                            /* Found adjacent block below current segment */
                            dseg->blk_ptr[j] |= 3;
                            b = a;
                            found = 1;
                        } else if (a == e) {
                            /* Found adjacent block above current segment */
                            dseg->blk_ptr[j] |= 3;
                            e += dseg->blk_size;
                            found = 1;
                        }
                    }
                }
            } while (found);
            if ((e - b) > dseg->seg_size) {
                /* The current block is largest so far */
                dseg->seg_begin = b;
                dseg->seg_end = e;
                dseg->seg_size = e - b;
                /* Re-tag current and previous largest segment */
                for (j = 0; j < blks; j++) {
                    if ((dseg->blk_ptr[j] & 3) == 3) {
                        /* Tag current segment as the largest */
                        dseg->blk_ptr[j] &= ~1;
                    } else if ((dseg->blk_ptr[j] & 3) == 2) {
                        /* Discard previous largest segment */
                        dseg->blk_ptr[j] ^= 3;
                    }
                }
            } else {
                /* Discard all blocks in current segment */
                for (j = 0; j < blks; j++) {
                    if ((dseg->blk_ptr[j] & 3) == 3) {
                        dseg->blk_ptr[j] &= ~2;
                    }
                }
            }
        }
    }
    return 0;
}

/*
 * Function: _alloc_dma_blocks
 *
 * Purpose:
 *    Allocate DMA blocks and add them to the pool.
 * Parameters:
 *    dseg - DMA segment descriptor
 *    blks - number of DMA blocks to allocate
 * Returns:
 *    0 on success, < 0 on error.
 * Notes:
 *    DMA blocks are allocated using the page allocator.
 */
static int
_alloc_dma_blocks(dma_segment_t *dseg, int blks)
{
    int i, start;
    unsigned long addr;

    if (dseg->blk_cnt + blks > dseg->blk_cnt_max) {
        gprintk("No more DMA blocks\n");
        return -1;
    }
    start = dseg->blk_cnt;
    for (i = 0; i < blks; i++) {
        /*
         * Note that we cannot use pci_alloc_consistent when we
         * want to be able to map DMA memory to user space.
         *
         * The GFP_DMA flag is omitted as this imposes the ISA
         * addressing limitations on x86 platforms. As long as
         * we have less than 1GB of memory, we can do PCI DMA
         * to all physical RAM locations.
         */
        addr = __get_free_pages(mem_flags, dseg->blk_order);
        if (addr) {
            dseg->blk_ptr[start + i] = addr;
            ++dseg->blk_cnt;
        } else {
            gprintk("DMA allocation failed: allocated %d of %d "
                    "requested blocks\n", i, blks);
            return -1;
        }
    }
    return 0;
}

/*
 * Function: _dma_segment_alloc
 *
 * Purpose:
 *    Allocate large physically contiguous DMA segment.
 * Parameters:
 *    size - requested DMA segment size
 *    blk_size - assemble segment from blocks of this size
 * Returns:
 *    DMA segment descriptor.
 * Notes:
 *    Since we cannot allocate large blocks of contiguous
 *    memory from the kernel, we simply keep allocating
 *    smaller chunks until we can assemble a contiguous
 *    block of the desired size.
 *
 *    When system allowed maximum bytes of memory has been allocated
 *    without a successful assembly of a contiguous DMA
 *    segment, the allocation function will return the
 *    largest contiguous segment found so far. It is up
 *    to the calling function to decide whether this
 *    amount is sufficient to proceed.
 */
static dma_segment_t *
_dma_segment_alloc(size_t size, size_t blk_size)
{
    dma_segment_t *dseg;
    int i, blk_ptr_size;
    unsigned long page_addr;
    struct sysinfo si;

    /* Sanity check */
    if (size == 0 || blk_size == 0) {
        return NULL;
    }
    /* Allocate an initialize DMA segment descriptor */
    if ((dseg = kmalloc(sizeof(dma_segment_t), GFP_KERNEL)) == NULL) {
        return NULL;
    }
    memset(dseg, 0, sizeof(dma_segment_t));
    dseg->req_size = size;
    dseg->blk_size = PAGE_ALIGN(blk_size);
    while ((PAGE_SIZE << dseg->blk_order) < dseg->blk_size) {
        dseg->blk_order++;
    }

    si_meminfo(&si);
    dseg->blk_cnt_max = (si.totalram << PAGE_SHIFT) / dseg->blk_size;
    blk_ptr_size = dseg->blk_cnt_max * sizeof(unsigned long);
    if (blk_ptr_size > KMALLOC_MAX_SIZE) {
        blk_ptr_size = KMALLOC_MAX_SIZE;
        dseg->blk_cnt_max = KMALLOC_MAX_SIZE / sizeof(unsigned long);
    }
    /* Allocate an initialize DMA block pool */
    dseg->blk_ptr = KMALLOC(blk_ptr_size, GFP_KERNEL);
    if (dseg->blk_ptr == NULL) {
        kfree(dseg);
        return NULL;
    }
    memset(dseg->blk_ptr, 0, blk_ptr_size);
    /* Allocate minimum number of blocks */
    if (_alloc_dma_blocks(dseg, dseg->req_size / dseg->blk_size) != 0) {
        gprintk("Failed to allocate minimum number of DMA blocks\n");
        /*
         * _alloc_dma_blocks() returns -1 if it fails to allocate the requested
         * number of blocks, but it may still have allocated something.  Fall
         * through and return dseg filled in with as much memory as we could
         * allocate.
         */
    }
    /* Allocate more blocks until we have a complete segment */
    do {
        _find_largest_segment(dseg);
        if (dseg->seg_size >= dseg->req_size) {
            break;
        }
    } while (_alloc_dma_blocks(dseg, 8) == 0);
    /* Reserve all pages in the DMA segment and free unused blocks */
    for (i = 0; i < dseg->blk_cnt; i++) {
        if ((dseg->blk_ptr[i] & 3) == 2) {
            dseg->blk_ptr[i] &= ~3;
            for (page_addr = dseg->blk_ptr[i];
                 page_addr < dseg->blk_ptr[i] + dseg->blk_size;
                 page_addr += PAGE_SIZE) {
                MEM_MAP_RESERVE(VIRT_TO_PAGE(page_addr));
            }
        } else if (dseg->blk_ptr[i]) {
            dseg->blk_ptr[i] &= ~3;
            free_pages(dseg->blk_ptr[i], dseg->blk_order);
            dseg->blk_ptr[i] = 0;
        }
    }
    return dseg;
}

/*
 * Function: _dma_segment_free
 *
 * Purpose:
 *    Release resources used by DMA segment.
 * Parameters:
 *    dseg - DMA segment descriptor
 * Returns:
 *    Nothing.
 */
static void
_dma_segment_free(dma_segment_t *dseg)
{
    int i;
    unsigned long page_addr;

    if (dseg->blk_ptr) {
        for (i = 0; i < dseg->blk_cnt; i++) {
            if (dseg->blk_ptr[i]) {
                for (page_addr = dseg->blk_ptr[i];
                     page_addr < dseg->blk_ptr[i] + dseg->blk_size;
                     page_addr += PAGE_SIZE) {
                    MEM_MAP_UNRESERVE(VIRT_TO_PAGE(page_addr));
                }
                free_pages(dseg->blk_ptr[i], dseg->blk_order);
            }
        }
        kfree(dseg->blk_ptr);
        kfree(dseg);
    }
}

/*
 * Function: _pgalloc
 *
 * Purpose:
 *    Allocate DMA memory using page allocator
 * Parameters:
 *    size - number of bytes to allocate
 * Returns:
 *    Pointer to allocated DMA memory or NULL if failure.
 * Notes:
 *    For any sizes less than DMA_BLOCK_SIZE, we ask the page
 *    allocator for the entire memory block, otherwise we try
 *    to assemble a contiguous segment ourselves.
 */
static void *
_pgalloc(size_t size)
{
    dma_segment_t *dseg;
    size_t blk_size;

    blk_size = (size < DMA_BLOCK_SIZE) ? size : DMA_BLOCK_SIZE;
    if ((dseg = _dma_segment_alloc(size, blk_size)) == NULL) {
        return NULL;
    }
    if (dseg->seg_size < size) {
        /* If we didn't get the full size then forget it */
        gprintk("_pgalloc() failed to get requested size %zu: "
                "only got %lu contiguous across %d blocks\n",
                size, dseg->seg_size, dseg->blk_cnt);
        _dma_segment_free(dseg);
        return NULL;
    }
    list_add(&dseg->list, &_dma_seg);
    return (void *)dseg->seg_begin;
}

/*
 * Function: _pgfree
 *
 * Purpose:
 *    Free memory allocated by _pgalloc
 * Parameters:
 *    ptr - pointer returned by _pgalloc
 * Returns:
 *    0 if succesfully freed, otherwise -1.
 */
static int
_pgfree(void *ptr)
{
    struct list_head *pos;
    list_for_each(pos, &_dma_seg) {
        dma_segment_t *dseg = list_entry(pos, dma_segment_t, list);
        if (ptr == (void *)dseg->seg_begin) {
            list_del(&dseg->list);
            _dma_segment_free(dseg);
            return 0;
        }
    }
    return -1;
}

#ifdef BDE_EDK_SUPPORT
/*
 * Function: _edk_mpool_free
 *
 * Purpose:
 *    Free all memory allocated by _adk_mpool_alloc
 * Parameters:
 *    None
 * Returns:
 *    Nothing.
 */
static void
_edk_mpool_free(void)
{
    int i, ndevices;

    ndevices = BDE_NUM_DEVICES(BDE_SWITCH_DEVICES);
    for (i = 0; i < ndevices && DMA_DEV(i); i ++) {
        if (_edk_dma_pool[i].dma_vbase) {
            if (_edk_use_dma_mapping) {
                dma_unmap_single(DMA_DEV(i), (dma_addr_t)_edk_dma_pool[i].dma_pbase,
                    _edk_dma_pool[i].size, DMA_BIDIRECTIONAL);
            }
            _pgfree(_edk_dma_pool[i].dma_vbase);
            _edk_dma_pool[i].dma_vbase = NULL;
        }
    }
    _edk_use_dma_mapping = 0;
}

/*
 * Function: edk_mpool_alloc
 *
 * Purpose:
 *    Allocate DMA memory pool for EDK
 * Parameters:
 *    size - size of DMA memory pool
 * Returns:
 *    Nothing.
 */
static void
_edk_mpool_alloc(int dev_id, size_t size)
{
    static void __iomem *dma_vbase = NULL;
    static phys_addr_t cpu_pbase = 0;
    static phys_addr_t dma_pbase = 0;

    struct device *dev = DMA_DEV(DMA_DEV_INDEX);
    unsigned long pbase = 0;

    dma_vbase = _pgalloc(size);
    if (!dma_vbase) {
        gprintk("Failed to allocate memory pool of size 0x%lx for EDK\n",
                (unsigned long)size);
        return;
    }
    cpu_pbase = virt_to_bus(dma_vbase);

    /* Use dma_map_single to obtain DMA bus address or IOVA if IOMMU is present. */
    if (dev) {
        pbase = dma_map_single(dev, dma_vbase, size, DMA_BIDIRECTIONAL);
        if (BDE_DMA_MAPPING_ERROR(dev, pbase)) {
            gprintk("Failed to map memory at %p for EDK\n", dma_vbase);
            _pgfree(dma_vbase);
            dma_vbase = NULL;
            return;
        }
        _edk_use_dma_mapping = 1;
    } else {
        pbase = cpu_pbase;
    }

    dma_pbase = pbase;

#ifdef REMAP_DMA_NONCACHED
    _dma_vbase = ioremap(dma_pbase, size);
#endif
    _edk_dma_pool[dev_id].cpu_pbase = cpu_pbase;
    _edk_dma_pool[dev_id].dma_pbase = dma_pbase;
    _edk_dma_pool[dev_id].dma_vbase = dma_vbase;
    _edk_dma_pool[dev_id].size = size;
}

int
lkbde_edk_get_dma_info(int dev_id, phys_addr_t* cpu_pbase, phys_addr_t* dma_pbase, ssize_t* size)
{
    if (_edk_dma_pool[dev_id].dma_vbase == NULL) {
        _edk_mpool_alloc(dev_id, *size * ONE_MB);
    }

    if (cpu_pbase) {
        *cpu_pbase = _edk_dma_pool[dev_id].cpu_pbase;
    }

    if (dma_pbase) {
        *dma_pbase = _edk_dma_pool[dev_id].dma_pbase;
    }

    *size = (_edk_dma_pool[dev_id].dma_vbase) ? _edk_dma_pool[dev_id].size : 0;
    return 0;
}

void *
lkbde_edk_dmamem_map_p2v(int dev_no, dma_addr_t paddr)
{
    if ((paddr >= _edk_dma_pool[dev_no].dma_pbase) &&
            (paddr < (_edk_dma_pool[dev_no].dma_pbase + _edk_dma_pool[dev_no].size))) {
        return (_edk_dma_pool[dev_no].dma_vbase + (paddr - _edk_dma_pool[dev_no].dma_pbase));
    } else {
        return NULL;
    }
}

/*
 * The below function validates the memory to the EDK allocated DMA pool,
 * required to user space via the BDE device file.
 */
static int
_edk_vm_is_valid(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long phys_addr = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long size = vma->vm_end - vma->vm_start;
    int i, ndevices;

    ndevices = LINUX_BDE_MAX_DEVICES;
    for (i = 0; i < ndevices; i++) {
        if (_edk_dma_pool[i].dma_vbase) {
            if (phys_addr < (unsigned long )_edk_dma_pool[i].cpu_pbase ||
                (phys_addr + size) > ((unsigned long )_edk_dma_pool[i].cpu_pbase +
                                   _edk_dma_pool[i].size)) {
                continue;
            }
        }
        return 1;
    }

    return 0;
}
#endif

/*
 * Function: _mpool_free
 *
 * Purpose:
 *    Free the DMA buffer pool and stop using it.
 * Parameters:
 *    None
 * Returns:
 *    Nothing.
 */
static void
_mpool_free(void)
{
    if (dma_debug >= 1) {
        gprintk("Freeing DMA buffer pool kernel_virt:0x%lx size:0x%x dmaalloc:%d ndevices=%d\n",
                 (unsigned long)_dma_vbase, _dma_mem_size, dmaalloc, BDE_NUM_DEVICES(BDE_SWITCH_DEVICES));
    }

    /* unmap bus address for all devices */

    if (_use_dma_mapping) {
        int i, ndevices;
        ndevices = BDE_NUM_DEVICES(BDE_SWITCH_DEVICES);
        for (i = 0; i < ndevices && DMA_DEV(i); i ++) {
            dma_unmap_single(DMA_DEV(i), (dma_addr_t)_dma_pbase, _dma_mem_size, DMA_BIDIRECTIONAL);
        }
        _use_dma_mapping = 0;
    }

#ifdef REMAP_DMA_NONCACHED
    iounmap(_dma_vbase);
#endif

    switch (dmaalloc) {
#if _SIMPLE_MEMORY_ALLOCATION_
      case ALLOC_TYPE_API:
        if (_dma_vbase) {
            if (dma_debug >= 1) gprintk("freeing v=0x%lx p=0x%lx size=0x%lx\n", (unsigned long)_dma_vbase,(unsigned long) _dma_pbase, (unsigned long)_dma_mem_size);
            if (_dma_alloc_coherent_device != NULL) {
                dma_free_attrs(_dma_alloc_coherent_device, _dma_mem_size, _dma_vbase, _dma_pbase, DMA_FORCE_CONTIGUOUS);
                _dma_alloc_coherent_device = NULL;
            }
        }
        break;
#endif /* _SIMPLE_MEMORY_ALLOCATION_ */

      case ALLOC_TYPE_CHUNK: {
        struct list_head *pos, *tmp;

        list_for_each_safe(pos, tmp, &_dma_seg) {
            dma_segment_t *dseg = list_entry(pos, dma_segment_t, list);
            list_del(&dseg->list);
            _dma_segment_free(dseg);
        }
        break;
      }

      case ALLOC_TYPE_HIMEM:
        break;

      default:
        gprintk("_mpool_free:DMA memory allocation method dmaalloc=%d is not supported\n", dmaalloc);
    }
    _dma_vbase = NULL;
    _cpu_pbase = 0;
    _dma_pbase = 0;
}

/*
 * Function: _mpool_alloc
 *
 * Purpose:
 *    Allocate DMA memory pool
 * Parameters:
 *    size - size of DMA memory pool
 * Returns:
 *    Nothing.
 * Notes:
 *    The DMA buffer pool is allocated if the selected memory allocation
 *    method does not also map it to PCIe bus addresses.
 *    If set up to use high memory, we simply map the memory into
 *    kernel space.
 *    It is assumed there is only one pool.
 */
static void
_mpool_alloc(size_t size)
{
    unsigned long pbase = 0;

    if (dma_debug >= 1) {
        gprintk("Allocating DMA memory using method dmaalloc=%d\n", dmaalloc);
    }
#if defined(__arm__) && !defined(CONFIG_HIGHMEM)
    if (_use_himem) {
        gprintk("DMA in high memory requires CONFIG_HIGHMEM on ARM CPUs.\n");
        return;
    }
#endif

    if (size == 0) return;

    if (_use_himem) {
        /* Use high memory for DMA */
        if (_himemaddr) {
            pbase = _himemaddr;
        } else {
            pbase = virt_to_phys(high_memory);
        }
        _cpu_pbase = pbase;
        _dma_vbase = phys_to_virt(pbase);
        _use_dma_mapping = 1;
    } else {
        switch (dmaalloc) {
#if _SIMPLE_MEMORY_ALLOCATION_
          case ALLOC_TYPE_API: {
#ifndef CONFIG_CMA
            /* The allocation will be performed together with the mapping to the first device */
            if (size > DMA_MAX_ALLOC_SIZE) {
                gprintk("Will allocate 0x%lx bytes instead of 0x%lx bytes.\n",
                        (unsigned long)DMA_MAX_ALLOC_SIZE, (unsigned long)size);
                _dma_mem_size = DMA_MAX_ALLOC_SIZE;
            }
#endif /* !CONFIG_CMA */
            if (nodevices == 1) {
                 /* With no devices, allocate immediately mapping to the null device */
                _dma_pool_alloc_state = DMA_POOL_INITIALIZED;
                _dma_per_device_init(0);
                return;
            }
            break;
          }
#endif /* _SIMPLE_MEMORY_ALLOCATION_ */

          case ALLOC_TYPE_CHUNK:
            _dma_vbase = _pgalloc(size);
            if (!_dma_vbase) {
                gprintk("Failed to allocate memory pool of size 0x%lx\n", (unsigned long)size);
                return;
            }
            _cpu_pbase = virt_to_phys(_dma_vbase);
            _use_dma_mapping = 1;
            break;
          default:
            _dma_vbase = NULL;
            gprintk("DMA memory allocation method dmaalloc=%d is not supported\n", dmaalloc);
            return;
        }

    }

    _dma_pool_alloc_state = DMA_POOL_INITIALIZED;
    if (_use_dma_mapping && dma_debug >= 1) {
        gprintk("DMA buffer pool before mapping kernel_virt:0x%lx physical:0x%lx size:0x%lx dmaalloc:%d\n",
                (unsigned long)_dma_vbase, (unsigned long)_cpu_pbase, (unsigned long)size, dmaalloc);
    }
}

/*
 * Function: _dma_cleanup
 *
 * Purpose:
 *    DMA cleanup function.
 * Parameters:
 *    None
 * Returns:
 *    Always 0
 */
int
_dma_cleanup(void)
{
#ifdef BDE_EDK_SUPPORT
    _edk_mpool_free();
#endif
    if (_dma_vbase) {
        mpool_destroy(_dma_pool);
        _mpool_free();
    }
    /* In case support will be added to re-allocate the DMA buffer pool */
    _dma_pool_alloc_state = DNA_POOL_INVALID;
    return 0;
}

/*
 * DMA initialization for a device. Called later than _dma_init().
 * Perform the IOMMU mapping for the specified device.
 */
void _dma_per_device_init(int dev_index)
{
    dma_addr_t dma_addr;
    struct device *dev;
    int performed_initial_pool_mapping = 0;
    int dma64_support = 0;

    if (dma_debug >= 2) {
        gprintk("_dma_per_device_init dev_index:%d _use_dma_mapping:%d kernel_virt:0x%lx physical:0x%lx size:0x%x dmaalloc:%d\n",
                dev_index, _use_dma_mapping, (unsigned long)_dma_vbase,
                (unsigned long)_cpu_pbase, _dma_mem_size, dmaalloc);
    }
#if _SIMPLE_MEMORY_ALLOCATION_
    if (nodevices == 1 && _dma_pool_alloc_state == DMA_POOL_INITIALIZED && dmaalloc == ALLOC_TYPE_API) {
        dev = NULL; /* Map to a null device */
    } else
#endif /* _SIMPLE_MEMORY_ALLOCATION_ */
    if ( dev_index < 0 || (dev = DMA_DEV(dev_index)) == NULL) {
        return;
    }

#if _SIMPLE_MEMORY_ALLOCATION_
    if (_dma_pool_alloc_state == DMA_POOL_INITIALIZED && dmaalloc == ALLOC_TYPE_API) {
        /* allocate the DMA buffer pool and map it to the device, uses CMA */
        _dma_vbase = dma_alloc_attrs(dev, _dma_mem_size, &dma_addr, GFP_KERNEL, DMA_FORCE_CONTIGUOUS);
        if (!_dma_vbase) {
            _dma_pool_alloc_state = DMA_POOL_FAILED;
            gprintk("Failed to allocate coherent memory pool of size 0x%x\n", _dma_mem_size);
            return;
        }
        _dma_alloc_coherent_device = dev;
        performed_initial_pool_mapping = 1;
        /* Set the host physical address of the DMA buffer pool */
        _cpu_pbase = HOST_PHYS_ADDR(dev, dma_addr, _dma_vbase);

    } else
#endif /* _SIMPLE_MEMORY_ALLOCATION_ */

    /* IF the DMA buffer pool required mapping to the device, perform it */
    if (_use_dma_mapping && _dma_vbase && (_dma_pool_alloc_state == DMA_POOL_INITIALIZED ||
        _dma_pool_alloc_state == DMA_POOL_MAPPED)) {
            /* Map RAM virtual address space for DMA usage and obtain DMA bus address or IOVA if iommu is present. */
            dma_addr = dma_map_single(dev, _dma_vbase, _dma_mem_size, DMA_BIDIRECTIONAL);
            if (BDE_DMA_MAPPING_ERROR(dev, dma_addr)) {
                gprintk("Failed to map DMA buffer pool for device %d at kernel_virt:0x%lx\n", dev_index, (unsigned long)_dma_vbase);
                if (_dma_pool_alloc_state == DMA_POOL_INITIALIZED) {
                    _mpool_free();
                    _dma_pool_alloc_state = DMA_POOL_FAILED;
                }
                return;
            }
            if (_dma_pool_alloc_state == DMA_POOL_INITIALIZED) {
                performed_initial_pool_mapping = 1;
            } else {
                if (dma_addr != (dma_addr_t)_dma_pbase) {
                    /* Bus address/IOVA must be identical for all devices. */
                    gprintk("Device %d has different PCIe bus address:0x%lx (should be 0x%lx)\n",
                            dev_index, (unsigned long)dma_addr, (unsigned long)_dma_pbase);
                }
                return;
            }
    }


    /* On the first mapping of the DMA buffer pool, check the mapped address and mark it as mapped */
    if (performed_initial_pool_mapping) {
#if defined(IPROC_CMICD) && defined(CONFIG_OF)
        if (of_find_compatible_node(NULL, NULL, "brcm,iproc-cmicx")) {
            dma64_support = 1;
        }
#endif

        if (!dma64_support && ((dma_addr + (_dma_mem_size - 1)) >> 16) > DMA_BIT_MASK(16)) {
            gprintk("DMA memory allocated at dma_bus:0x%lx size 0x%x is beyond the 4GB limit and not supported.\n", (unsigned long)dma_addr, _dma_mem_size);
            _mpool_free();
            _dma_pool_alloc_state = DMA_POOL_FAILED;
            return;
        }
        _dma_pbase = dma_addr;
        _dma_pool_alloc_state = DMA_POOL_MAPPED;

#ifdef REMAP_DMA_NONCACHED
        if (dma_debug >= 2) {
            gprintk("remapping DMA buffer pool from physical:0x%lx original kernel_virt:0x%lx\n",
                (unsigned long)_dma_pbase, (unsigned long)_dma_vbase);
        }
        _dma_vbase = ioremap(_dma_pbase, _dma_mem_size);
#endif

        if (dma_debug >= 1) {
            gprintk("Mapped DMA buffer pool _use_dma_mapping:%d kernel_virt:0x%lx dma_bus:0x%lx physical:0x%lx size:0x%x dmaalloc:%d, dma64_support:%d\n",
                    _use_dma_mapping, (unsigned long)_dma_vbase, (unsigned long)_dma_pbase,
                    (unsigned long)_cpu_pbase, _dma_mem_size, dmaalloc, dma64_support);
        }

        /* DMA buffer pool initialization */
        mpool_init();
        _dma_pool = mpool_create(_dma_vbase, _dma_mem_size);
    }
    return;
}

/*
 * If this is the first time the function is called, * allocate the DMA
 * buffer pool.
 * Use kernel module arguments for pool size and allocation methods.
 */
void _dma_init(void)
{
    /*
     * If DMA buffer pool allocation was already attempted and failed,
     * do not attempt again.
     */
    if (_dma_pool_alloc_state != DNA_POOL_INVALID || _dma_vbase != NULL) {
        return;
    }
    _dma_pool_alloc_state = DMA_POOL_FAILED;

    /* dmasize, himem and himemaddr kernel module argument parsing */
    if (dmasize) {
#ifdef INCLUDE_SRAM_DMA
        if (!strncasecmp(dmasize, "sram", 4) && (dmasize[4] == '\0' || dmasize[4] == ',')) {
            use_sram_for_dma = 1; /* set SRAM DMA mode */
            if (dma_debug >= 1) {
                gprintk("SRAM DMA mode\n");
            }
#ifdef SRAM_DMA_NEEDS_KERNEL_APIS
            _update_apis_for_sram_dma();
#endif
            if (dmasize[4] == '\0') {
                dmasize +=4;
            } else {
                dmasize +=5;
            }

        }
#endif /* INCLUDE_SRAM_DMA */
        if (*dmasize != '\0') {
            if ((dmasize[strlen(dmasize)-1] & ~0x20) == 'M') {
                _dma_mem_size = simple_strtoul(dmasize, NULL, 0);
                _dma_mem_size *= ONE_MB;
            } else {
                gprintk("DMA memory size must be specified as e.g. dmasize=8M\n");
            }
        }
    }

    if (himem) {
        if ((himem[0] & ~0x20) == 'Y' || himem[0] == '1') {
            _use_himem = 1;
            dmaalloc = ALLOC_TYPE_HIMEM;
        } else if ((himem[0] & ~0x20) == 'N' || himem[0] == '0') {
            _use_himem = 0;
        }
    }

    if (himemaddr && strlen(himemaddr) > 0) {
        char suffix = (himemaddr[strlen(himemaddr)-1] & ~0x20);
        _himemaddr = simple_strtoul(himemaddr, NULL, 0);
        if (suffix == 'M') {
            _himemaddr *= ONE_MB;
        } else if (suffix == 'G') {
            _himemaddr *= ONE_GB;
        } else {
            gprintk("DMA high memory address must be specified as e.g. himemaddr=8[MG]\n");
        }
    }

    /* DMA buffer pool allocation and initialization that can be done without mapping to DMA */
    if (_dma_mem_size) {
        /* Allocate the DMA buffer pool */
        _mpool_alloc(_dma_mem_size);
        if (_dma_pool_alloc_state != DMA_POOL_INITIALIZED) {
            gprintk("no DMA memory available\n");
        }
    }
}

/*
 * Some kernels are configured to prevent mapping of kernel RAM memory
 * into user space via the /dev/mem device.
 *
 * The function below provides a backdoor to mapping the DMA pool to
 * user space via the BDE device file.
 */
int _dma_mmap(struct file *filp, struct vm_area_struct *vma)
{
    unsigned long phys_addr = vma->vm_pgoff << PAGE_SHIFT;
    unsigned long size = vma->vm_end - vma->vm_start;

    if (phys_addr < (unsigned long )_cpu_pbase ||
        (phys_addr + size) > ((unsigned long )_cpu_pbase + _dma_mem_size)) {
#ifdef BDE_EDK_SUPPORT
        if(!_edk_vm_is_valid(filp, vma)) {
            gprintk("range 0x%lx-0x%lx outside DMA pool\n", phys_addr, phys_addr + size);
            return -EINVAL;
        }
#else
        gprintk("range 0x%lx-0x%lx outside DMA pool 0x%lx-0x%lx\n",
                phys_addr, phys_addr + size, (unsigned long )_cpu_pbase,
                (unsigned long )_cpu_pbase + _dma_mem_size);
        return -EINVAL;
#endif
    }

#ifdef USE_DMA_MMAP_COHERENT
    if (dmaalloc == ALLOC_TYPE_API) {
        vma->vm_pgoff = 0;
        return dma_mmap_coherent(_dma_alloc_coherent_device, vma, (void *)_dma_vbase, phys_addr, size);
    }
#endif

    _PGPROT_NONCACHED(vma->vm_page_prot);

    if (remap_pfn_range(vma,
                        vma->vm_start,
                        vma->vm_pgoff,
                        size,
                        vma->vm_page_prot)) {
        gprintk("Failed to mmap phys range 0x%lx-0x%lx to 0x%lx-0x%lx\n",
                phys_addr, phys_addr + size, vma->vm_start,vma->vm_end);
        return -EAGAIN;
    }
    return 0;
}

/*
 * Function: _dma_pool_allocated
 *
 * Purpose:
 *    Check if DMA pool has been allocated.
 * Parameters:
 *    None
 * Returns:
 *    0 : not allocated
 *    1 : allocated
 */
int
_dma_pool_allocated(void)
{
    return (_dma_vbase) ? 1 : 0;
}

sal_paddr_t
_l2p(int d, void *vaddr)
{
    if (_dma_mem_size) {
        /* dma memory is a contiguous block */
        if (vaddr) {
            return _dma_pbase + (PTR_TO_UINTPTR(vaddr) - PTR_TO_UINTPTR(_dma_vbase));
        }
        return 0;
    }

    return ((sal_paddr_t)virt_to_bus(vaddr));
}

void *
_p2l(int d, sal_paddr_t paddr)
{
    sal_vaddr_t vaddr = (sal_vaddr_t)_dma_vbase;

    if (_dma_mem_size) {
        /* DMA memory is a contiguous block */
        if (paddr == 0) {
            return NULL;
        }
        return (void *)(vaddr + (sal_vaddr_t)(paddr - _dma_pbase));
    }

    return bus_to_virt(paddr);
}

uint32_t *
_salloc(int d, int size, const char *name)
{
    void *ptr;

    if (_dma_mem_size) {
        return mpool_alloc(_dma_pool, size);
    }
    if ((ptr = kmalloc(size, mem_flags)) == NULL) {
        ptr = _pgalloc(size);
    }
    return ptr;
}

void
_sfree(int d, void *ptr)
{
    if (_dma_mem_size) {
        return mpool_free(_dma_pool, ptr);
    }
    if (_pgfree(ptr) < 0) {
        kfree(ptr);
    }
}

int
_sinval(int d, void *ptr, int length)
{
#if defined(dma_cache_wback_inv)
     dma_cache_wback_inv((unsigned long)ptr, length);
#else
    dma_sync_single_for_cpu(NULL, (unsigned long)ptr, length, DMA_BIDIRECTIONAL);
#endif
    return 0;
}

int
_sflush(int d, void *ptr, int length)
{
#if defined(dma_cache_wback_inv)
    dma_cache_wback_inv((unsigned long)ptr, length);
#else
    dma_sync_single_for_cpu(NULL, (unsigned long)ptr, length, DMA_BIDIRECTIONAL);
#endif

    return 0;
}

int
lkbde_get_dma_info(phys_addr_t* cpu_pbase, phys_addr_t* dma_pbase, ssize_t* size)
{
    if (_dma_vbase == NULL) {
        if (_dma_mem_size == 0) {
            _dma_mem_size = DMA_MEM_DEFAULT;
        }
    }
    *cpu_pbase = _cpu_pbase;
    *dma_pbase = _dma_pbase;
    *size = (_dma_vbase) ? _dma_mem_size : 0;
    return 0;
}

void
_dma_pprint(struct seq_file *m)
{
    pprintf(m, "\tdmasize=%s\n", dmasize);
    pprintf(m, "\thimem=%s\n", himem);
    pprintf(m, "\thimemaddr=%s\n", himemaddr);
    pprintf(m, "DMA Memory (%s): %d bytes allocated%s\n",
            (_use_himem) ? "high" : dmaalloc ? "kernel-api" : "kernel-chunk",
            (_dma_vbase) ? _dma_mem_size : 0,
            USE_LINUX_BDE_MMAP ? ", local mmap" : "");
}

int
lkbde_get_phys_to_virt(int d, phys_addr_t paddr, sal_vaddr_t *vaddr)
{
    sal_vaddr_t vaddr_base = (sal_vaddr_t)_dma_vbase;
    sal_vaddr_t vaddr_new;

    if (_dma_mem_size) {
        /* DMA memory is a contiguous block */
        if (paddr == 0) {
            return -1;
        }

        vaddr_new = (vaddr_base + (paddr - _dma_pbase));
        *vaddr = vaddr_new;
        return 0;
    }

    *vaddr = 0;
    return -1;
}

/*
 * Export functions
 */

#ifdef BDE_EDK_SUPPORT
LKM_EXPORT_SYM(lkbde_edk_get_dma_info);
LKM_EXPORT_SYM(lkbde_edk_dmamem_map_p2v);
#endif
LKM_EXPORT_SYM(lkbde_get_dma_info);
LKM_EXPORT_SYM(lkbde_get_phys_to_virt);
