/*
 * $Id: mpool.c,v 1.18 Broadcom SDK $
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
 */

#include <mpool.h>

#ifdef __KERNEL__

/* 
 * Abstractions used when compiling for Linux kernel mode. 
 */

#include <lkm.h>

/*
 * We cannot use the linux kernel SAL for MALLOC/FREE because 
 * the current implementation of sal_alloc() allocates memory 
 * out of an mpool created by this module...
 */
#define MALLOC(x) kmalloc(x, GFP_ATOMIC)
#define FREE(x) kfree(x)

static spinlock_t _mpool_lock;
#define MPOOL_LOCK_INIT() spin_lock_init(&_mpool_lock)
#define MPOOL_LOCK() unsigned long flags; spin_lock_irqsave(&_mpool_lock, flags)
#define MPOOL_UNLOCK() spin_unlock_irqrestore(&_mpool_lock, flags)

#else /* !__KERNEL__*/

/* 
 * Abstractions used when compiling for Linux user mode. 
 */

#include <stdlib.h>
#include <sal/core/sync.h>

#define MALLOC(x) malloc(x)
#define FREE(x) free(x)

static sal_sem_t _mpool_lock;
#define MPOOL_LOCK_INIT() _mpool_lock = sal_sem_create("mpool_lock", 1, 1)
#define MPOOL_LOCK() sal_sem_take(_mpool_lock, sal_sem_FOREVER)
#define MPOOL_UNLOCK() sal_sem_give(_mpool_lock)

#endif /* __KERNEL__ */

/* Allow external override for system cache line size */
#ifndef BCM_CACHE_LINE_BYTES
#ifdef L1_CACHE_BYTES
#define BCM_CACHE_LINE_BYTES L1_CACHE_BYTES
#else
#define BCM_CACHE_LINE_BYTES 128 /* Should be fine on most platforms */
#endif
#endif

#define MPOOL_BUF_SIZE               1024
#define MPOOL_BUF_ALLOC_COUNT_MAX     128

typedef struct mpool_mem_s {
    unsigned char *address;
    int size;
    struct mpool_mem_s *prev;
    struct mpool_mem_s *next;
} mpool_mem_t;

static int _mpool_count;
static int _buf_alloc_count;
static mpool_mem_t *mpool_buf[MPOOL_BUF_ALLOC_COUNT_MAX];
static mpool_mem_t *free_list;

static mpool_mem_t *
_mpool_buf_create(void)
{
    int i;
    mpool_mem_t *ptr;

    if (_buf_alloc_count == MPOOL_BUF_ALLOC_COUNT_MAX) {
        return NULL;
    }

    mpool_buf[_buf_alloc_count] = MALLOC((sizeof(mpool_mem_t) * MPOOL_BUF_SIZE));
    if (!mpool_buf[_buf_alloc_count]) {
        return NULL;
    }

    ptr = mpool_buf[_buf_alloc_count];
    for (i = 0; i < MPOOL_BUF_SIZE - 1; i++) {
        ptr[i].next = &ptr[i+1];
    }

    ptr[MPOOL_BUF_SIZE - 1].next = NULL;

    if (free_list) {
        free_list->next = &ptr[0];
    } else {
        free_list = &ptr[0];
    }

    _buf_alloc_count++;
    return ptr;
}

/*
 * Function: mpool_init
 *
 * Purpose:
 *    Initialize mpool lock.
 * Parameters:
 *    None
 * Returns:
 *    Always 0
 */
int 
mpool_init(void)
{
    int i;

    MPOOL_LOCK_INIT();
    _buf_alloc_count = 0;
    _mpool_count = 0;
    for (i = 0; i < MPOOL_BUF_ALLOC_COUNT_MAX; i++) {
        mpool_buf[i] = NULL;
    }
    free_list = NULL;
    return 0;
}

#ifdef TRACK_DMA_USAGE
static int _dma_mem_used = 0;
#endif

/*
 * Function: mpool_alloc
 *
 * Purpose:
 *    Allocate memory block from mpool.
 * Parameters:
 *    pool - mpool handle (from mpool_create)
 *    size - size of memory block to allocate
 * Returns:
 *    Pointer to allocated memory block or NULL if allocation fails.
 */
void *
mpool_alloc(mpool_handle_t pool, int size)
{
    mpool_mem_t *ptr = pool, *newptr = NULL;
    int mod;

    MPOOL_LOCK();

    if (size < BCM_CACHE_LINE_BYTES) {
        size = BCM_CACHE_LINE_BYTES;
    }

    mod = size & (BCM_CACHE_LINE_BYTES - 1);
    if (mod != 0) {
        size += (BCM_CACHE_LINE_BYTES - mod);
    }
    while (ptr && ptr->next) {
        if (ptr->next->address - (ptr->address + ptr->size) >= size) {
            break;
        }
        ptr = ptr->next;
    }
  
    if (!(ptr && ptr->next)) {
        MPOOL_UNLOCK();
        return NULL;
    }

    if (!free_list && !_mpool_buf_create()) {
        MPOOL_UNLOCK();
        return NULL;
    }

    newptr = free_list;
    free_list = free_list->next;
  
    newptr->address = ptr->address + ptr->size;
    newptr->size = size;
    newptr->next = ptr->next;
    newptr->prev = ptr;
    ptr->next->prev = newptr;
    ptr->next = newptr;
#ifdef TRACK_DMA_USAGE
    _dma_mem_used += size;
#endif

    MPOOL_UNLOCK();
    return newptr->address;
}


/*
 * Function: mpool_free
 *
 * Purpose:
 *    Free memory block allocated from mpool..
 * Parameters:
 *    pool - mpool handle (from mpool_create)
 *    addr - address of memory block to free
 * Returns:
 *    Nothing
 */
void 
mpool_free(mpool_handle_t pool, void *addr)
{
    unsigned char *address = (unsigned char *)addr;  
    mpool_mem_t *head = pool, *ptr = NULL;

    MPOOL_LOCK();

    if (!(head && head->prev)) {
        MPOOL_UNLOCK();
        return;
    }

    ptr = head->prev->prev;

    while (ptr && (ptr != head)) {
        if (ptr->address == address) {
#ifdef TRACK_DMA_USAGE
            _dma_mem_used -= ptr->size;
#endif
            ptr->prev->next = ptr->next;
            ptr->next->prev = ptr->prev;
            ptr->next = free_list;
            free_list = ptr;
            break;
        }
        ptr = ptr->prev;
    }

    MPOOL_UNLOCK();
}

/*
 * Function: mpool_create
 *
 * Purpose:
 *    Create and initialize mpool control structures.
 * Parameters:
 *    base_ptr - pointer to mpool memory block
 *    size - total size of mpool memory block
 * Returns:
 *    mpool handle
 * Notes
 *    The mpool handle returned must be used for subsequent
 *    memory allocations from the mpool.
 */
mpool_handle_t
mpool_create(void *base_ptr, int size)
{
    mpool_mem_t *head, *tail;
    int mod = (int)(((unsigned long)base_ptr) & (BCM_CACHE_LINE_BYTES - 1));

    MPOOL_LOCK();

    if (!free_list || !(free_list->next)) {
        if (!_mpool_buf_create()) {
            MPOOL_UNLOCK();
            return NULL;
        }
    }

    if (mod) {
        base_ptr = (char*)base_ptr + (BCM_CACHE_LINE_BYTES - mod);
        size -= (BCM_CACHE_LINE_BYTES - mod);
    }
    size &= ~(BCM_CACHE_LINE_BYTES - 1);

    head = free_list;
    free_list = free_list->next;
    tail = free_list;
    free_list = free_list->next;

    head->size = tail->size = 0;
    head->address = base_ptr;
    tail->address = head->address + size;
    head->prev = tail;
    head->next = tail;
    tail->prev = head;
    tail->next = NULL;
    _mpool_count++;

    MPOOL_UNLOCK();
    return head;
}

/*
 * Function: mpool_destroy
 *
 * Purpose:
 *    Free mpool control structures.
 * Parameters:
 *    pool - mpool handle (from mpool_create)
 * Returns:
 *    Always 0
 */
int
mpool_destroy(mpool_handle_t pool)
{
    int i;
    mpool_mem_t *head = pool;

    MPOOL_LOCK();

    if (!(head && head->prev)) {
        MPOOL_UNLOCK();
        return 0;
    }

    head->prev->next = free_list;
    free_list = head;
    _mpool_count--;

    if (_mpool_count == 0) {
        for (i = 0; i < MPOOL_BUF_ALLOC_COUNT_MAX; i++) {
            if (mpool_buf[i]) {
                FREE(mpool_buf[i]);
                mpool_buf[i] = NULL;
            }
        }
        free_list = NULL;
    }

    MPOOL_UNLOCK();

    return 0;
}

/*
 * Function: mpool_usage
 *
 * Purpose:
 *    Report total sum of allocated mpool memory.
 * Parameters:
 *    pool - mpool handle (from mpool_create)
 * Returns:
 *    Number of bytes currently allocated using mpool_alloc.
 */
int
mpool_usage(mpool_handle_t pool)
{
    int usage = 0;
    mpool_mem_t *ptr;

    MPOOL_LOCK();

    for (ptr = pool; ptr; ptr = ptr->next) {
        usage += ptr->size;
    }

    MPOOL_UNLOCK();

    return usage;
}
