/* Copyright (C) 2020  MediaTek, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General Public
 * License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with this program.
 */

/* FILE NAME:  netif_osal.c
 * PURPOSE:
 *      It provide customer linux API.
 * NOTES:
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/signal.h>
#endif
#include <netif_osal.h>

/* ----------------------------------------------------------------------------------- macro value */
#define OSAL_US_PER_SECOND      (1000000)   /* macro second per second      */
#define OSAL_NS_PER_USECOND     (1000)      /* nano second per macro second */

/* ----------------------------------------------------------------------------------- macro function */
#define OSAL_LOG_ERR(msg, ...) \
    osal_printf("\033[31m<osal:%d>\033[0m"msg, __LINE__, ##__VA_ARGS__)

/* ----------------------------------------------------------------------------------- struct */
extern struct pci_dev       *_ptr_ext_pci_dev;

static linux_thread_t       _osal_thread_head = {{0}};

/* ----------------------------------------------------------------------------------- function */
/* general */
void *
osal_memset(
    void                    *ptr_mem,
    const I32_T             value,
    const UI32_T            num)
{
    return memset(ptr_mem, value, num);
}

void *
osal_memcpy(
    void                    *ptr_dst,
    const void              *ptr_src,
    const UI32_T            num)
{
    return memcpy(ptr_dst, ptr_src, num);
}

UI32_T
osal_strlen(
    const C8_T              *ptr_str)
{
    return strlen(ptr_str);
}

void
osal_printf(
    const C8_T              *ptr_fmt,
    ...)
{
    va_list                 ap;
    char                    buf[OSAL_PRN_BUF_SZ];

    if (NULL != ptr_fmt)
    {
        va_start(ap, ptr_fmt);
        vsnprintf(buf, OSAL_PRN_BUF_SZ, ptr_fmt, ap);
        va_end(ap);

        printk("%s", buf);
    }
}

void *
osal_alloc(
    const UI32_T            size)
{
    return kmalloc(size, GFP_ATOMIC);
}

void
osal_free(
    const void              *ptr_mem)
{
    kfree(ptr_mem);
}

/* thread */
NPS_ERROR_NO_T
osal_init(void)
{
    linux_thread_t          *ptr_thread_head = &_osal_thread_head;

    memset(ptr_thread_head, 0x0, sizeof(linux_thread_t));

    /* init */
    ptr_thread_head->ptr_prev = ptr_thread_head;
    ptr_thread_head->ptr_next = ptr_thread_head;

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_deinit(void)
{
    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_createThread(
    const C8_T              *ptr_thread_name,
    const UI32_T            stack_size,
    const UI32_T            priority,
    void                    (function)(void*),
    void                    *ptr_arg,
    NPS_THREAD_ID_T         *ptr_thread_id)
{
    char                    dft_name[OSAL_THREAD_NAME_LEN + 1] = OSAL_THREAD_DFT_NAME;
    linux_thread_t          *ptr_thread_head = &_osal_thread_head;
    linux_thread_t          *ptr_thread_node = osal_alloc(sizeof(linux_thread_t));

    /* process name */
    osal_memcpy(ptr_thread_node->name, (0 == osal_strlen(ptr_thread_name))?
        dft_name : ptr_thread_name, OSAL_THREAD_NAME_LEN);
    ptr_thread_node->name[OSAL_THREAD_NAME_LEN] = '\0';

    /* init */
    ptr_thread_node->ptr_task = kthread_create((int(*)(void *))function, ptr_arg, ptr_thread_name);
    ptr_thread_node->ptr_task->policy = SCHED_RR;
    ptr_thread_node->ptr_task->rt_priority = priority;
    ptr_thread_node->is_stop = FALSE;

    *ptr_thread_id = (NPS_THREAD_ID_T)ptr_thread_node;

    wake_up_process(ptr_thread_node->ptr_task);

    /* append the thread_node */
    ptr_thread_node->ptr_prev           = ptr_thread_head->ptr_prev;
    ptr_thread_head->ptr_prev->ptr_next = ptr_thread_node;
    ptr_thread_node->ptr_next           = ptr_thread_head;
    ptr_thread_head->ptr_prev           = ptr_thread_node;

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_stopThread(
    NPS_THREAD_ID_T         *ptr_thread_id)
{
    linux_thread_t          *ptr_thread_node = (linux_thread_t *)(*ptr_thread_id);

    ptr_thread_node->is_stop = TRUE;

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_destroyThread(
    NPS_THREAD_ID_T         *ptr_thread_id)
{
    linux_thread_t          *ptr_thread_node = (linux_thread_t *)(*ptr_thread_id);

    kthread_stop(ptr_thread_node->ptr_task);

    /* remove the thread_node */
    ptr_thread_node->ptr_next->ptr_prev = ptr_thread_node->ptr_prev;
    ptr_thread_node->ptr_prev->ptr_next = ptr_thread_node->ptr_next;

    osal_free(ptr_thread_node);
    *ptr_thread_id = 0;

    return (NPS_E_OK);
}

void
osal_initRunThread(void)
{
    /* for reboot or shutdown without stopping kthread */
    allow_signal(SIGTERM);
}

NPS_ERROR_NO_T
osal_isRunThread(void)
{
    linux_thread_t          *ptr_thread_node = _osal_thread_head.ptr_next;

    while (1)
    {
        if (ptr_thread_node == &_osal_thread_head)
        {
            OSAL_LOG_ERR("Cannot find task 0x%x.\n", current);
            break;
        }
        if (ptr_thread_node->ptr_task == current)
        {
            break;
        }
        ptr_thread_node = ptr_thread_node->ptr_next;
    }

    if ((TRUE == ptr_thread_node->is_stop) || (signal_pending(current)))
    {
        return (NPS_E_OTHERS);
    }

    return (NPS_E_OK);
}

void
osal_exitRunThread(void)
{
    while (!kthread_should_stop() && !signal_pending(current))
    {
        osal_sleepThread(OSAL_NS_PER_USECOND);
    }
}

/* semaphore */
NPS_ERROR_NO_T
osal_createSemaphore(
    const C8_T              *ptr_sema_name,
    const UI32_T            sema_count,
    NPS_SEMAPHORE_ID_T      *ptr_semaphore_id)
{
    char                    dft_name[OSAL_SEMA_NAME_LEN + 1] = OSAL_SEMA_DFT_NAME;
    linux_sema_t            *ptr_sema = osal_alloc(sizeof(linux_sema_t));

    /* process name */
    osal_memcpy(ptr_sema->name, (0 == osal_strlen(ptr_sema_name))?
        dft_name : ptr_sema_name, OSAL_SEMA_NAME_LEN);
    ptr_sema->name[OSAL_SEMA_NAME_LEN] = '\0';

    /* init */
    sema_init(&ptr_sema->lock, NPS_SEMAPHORE_BINARY);

    *ptr_semaphore_id = (NPS_SEMAPHORE_ID_T)ptr_sema;

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_takeSemaphore(
    NPS_SEMAPHORE_ID_T      *ptr_semaphore_id,
    UI32_T                  time_out)
{
    linux_sema_t            *ptr_sema = (linux_sema_t *)(*ptr_semaphore_id);

    if (in_interrupt())
    {
        return (NPS_E_OTHERS);
    }

    if (!down_interruptible(&ptr_sema->lock))
    {
        return (NPS_E_OK);
    }

    return (NPS_E_OTHERS);
}

NPS_ERROR_NO_T
osal_giveSemaphore(
    NPS_SEMAPHORE_ID_T      *ptr_semaphore_id)
{
    linux_sema_t            *ptr_sema = (linux_sema_t *)(*ptr_semaphore_id);

    up(&ptr_sema->lock);

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_destroySemaphore(
    NPS_SEMAPHORE_ID_T      *ptr_semaphore_id)
{
    linux_sema_t            *ptr_sema = (linux_sema_t *)(*ptr_semaphore_id);

    osal_free(ptr_sema);
    *ptr_semaphore_id = 0;

    return (NPS_E_OK);
}

/* event */
NPS_ERROR_NO_T
osal_createEvent(
    const C8_T              *ptr_event_name,
    NPS_SEMAPHORE_ID_T      *ptr_event_id)
{
    char                    dft_name[OSAL_EVENT_NAME_LEN + 1] = OSAL_EVENT_DFT_NAME;
    linux_event_t           *ptr_event = osal_alloc(sizeof(linux_event_t));

    /* process name */
    osal_memcpy(ptr_event->name, (0 == osal_strlen(ptr_event_name))?
        dft_name : ptr_event_name, OSAL_EVENT_NAME_LEN);
    ptr_event->name[OSAL_EVENT_NAME_LEN] = '\0';

    /* init */
    ptr_event->condition = FALSE;
    init_waitqueue_head(&ptr_event->wait_que);

    *ptr_event_id = (NPS_SEMAPHORE_ID_T)ptr_event;

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_waitEvent(
    NPS_SEMAPHORE_ID_T      *ptr_event_id)
{
    linux_event_t           *ptr_event = (linux_event_t *)(*ptr_event_id);

    if (!wait_event_interruptible(ptr_event->wait_que, ptr_event->condition))
    {
        ptr_event->condition = FALSE;

        return (NPS_E_OK);
    }

    return (NPS_E_OTHERS);
}

NPS_ERROR_NO_T
osal_triggerEvent(
    NPS_SEMAPHORE_ID_T      *ptr_event_id)
{
    linux_event_t           *ptr_event = (linux_event_t *)(*ptr_event_id);

    ptr_event->condition = TRUE;
    wake_up_interruptible(&ptr_event->wait_que);

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_destroyEvent(
    NPS_SEMAPHORE_ID_T      *ptr_event_id)
{
    linux_event_t           *ptr_event = (linux_event_t *)(*ptr_event_id);

    osal_free(ptr_event);
    *ptr_event_id = 0;

    return (NPS_E_OK);
}

/* isr_lock */
NPS_ERROR_NO_T
osal_createIsrLock(
    const C8_T              *ptr_isrlock_name,
    NPS_ISRLOCK_ID_T        *ptr_isrlock_id)
{
    char                    dft_name[OSAL_SPIN_NAME_LEN + 1] = OSAL_SPIN_DFT_NAME;
    linux_isrlock_t         *ptr_isrlock = osal_alloc(sizeof(linux_isrlock_t));

    /* process name */
    osal_memcpy(ptr_isrlock->name, (0 == osal_strlen(ptr_isrlock_name))?
        dft_name : ptr_isrlock_name, OSAL_SPIN_NAME_LEN);
    ptr_isrlock->name[OSAL_SPIN_NAME_LEN] = '\0';

    /* init */
    spin_lock_init(&ptr_isrlock->spinlock);

    *ptr_isrlock_id = (NPS_ISRLOCK_ID_T)ptr_isrlock;

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_takeIsrLock(
    NPS_ISRLOCK_ID_T        *ptr_isrlock_id,
    NPS_IRQ_FLAGS_T         *ptr_irq_flags)
{
    linux_isrlock_t         *ptr_isrlock = (linux_isrlock_t *)(*ptr_isrlock_id);
    unsigned long           flags = 0;

    spin_lock_irqsave(&ptr_isrlock->spinlock, flags);
    *ptr_irq_flags = (NPS_IRQ_FLAGS_T)flags;

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_giveIsrLock(
    NPS_ISRLOCK_ID_T        *ptr_isrlock_id,
    NPS_IRQ_FLAGS_T         *ptr_irq_flags)
{
    linux_isrlock_t         *ptr_isrlock = (linux_isrlock_t *)(*ptr_isrlock_id);
    unsigned long           flags = 0;

    flags = (unsigned long)(*ptr_irq_flags);
    spin_unlock_irqrestore(&ptr_isrlock->spinlock, flags);

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_destroyIsrLock(
    NPS_ISRLOCK_ID_T        *ptr_isrlock_id)
{
    linux_isrlock_t         *ptr_isrlock = (linux_isrlock_t *)(*ptr_isrlock_id);

    osal_free(ptr_isrlock);
    *ptr_isrlock_id = 0;

    return (NPS_E_OK);
}

/* time */
NPS_ERROR_NO_T
osal_sleepThread(
    const UI32_T            usecond)
{
    UI32_T                  tick_usec;  /* how many usec per second */
    UI32_T                  jiffies;

    if (0 != usecond)
    {
        /* HZ : times/sec, tick = 1/HZ */
        tick_usec = OSAL_TICKS_PER_SEC / HZ;
        if (in_interrupt() || (usecond < tick_usec))
        {
            return (-1);
        }
        else
        {
            DECLARE_WAIT_QUEUE_HEAD(suspend_queue);

            if (usecond > 0xFFFFFFFF - (tick_usec - 1))
            {
                jiffies = 0xFFFFFFFF / tick_usec;
            }
            else
            {
                jiffies = (usecond + (tick_usec - 1)) / tick_usec;
            }

            return wait_event_interruptible_timeout(suspend_queue, 0, jiffies);
        }
    }

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_getTime(
    NPS_TIME_T              *ptr_time)
{
    struct timeval          usec_time;

    do_gettimeofday(&usec_time);
    *(NPS_TIME_T *)ptr_time = (usec_time.tv_sec * OSAL_US_PER_SECOND) + usec_time.tv_usec;

    return (NPS_E_OK);
}

/* queue */
NPS_ERROR_NO_T
osal_que_create(
    NPS_HUGE_T              *ptr_queue_id,
    UI32_T                  capacity)
{
    linux_queue_t           *ptr_queue = osal_alloc(sizeof(linux_queue_t));

    ptr_queue->head      = 0;
    ptr_queue->tail      = 0;
    ptr_queue->wr_cnt    = 0;
    ptr_queue->rd_cnt    = 0;
    ptr_queue->capacity  = capacity;
    ptr_queue->ptr_entry = osal_alloc(sizeof(linux_queue_entry_t) * capacity);
    memset(ptr_queue->ptr_entry, 0x0, sizeof(linux_queue_entry_t) * capacity);

    *ptr_queue_id = (NPS_HUGE_T)ptr_queue;

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_que_enque(
    NPS_HUGE_T              *ptr_queue_id,
    void                    *ptr_data)
{
    linux_queue_t           *ptr_queue = (linux_queue_t *)(*ptr_queue_id);

    if (ptr_queue->wr_cnt - ptr_queue->rd_cnt >= ptr_queue->capacity)
    {
        return (NPS_E_OTHERS);
    }

    /* save data to the tail */
    ptr_queue->ptr_entry[ptr_queue->tail].ptr_data = ptr_data;

    /* calculate tail and wr_cnt */
    ptr_queue->tail++;
    if (ptr_queue->tail >= ptr_queue->capacity)
    {
        ptr_queue->tail = 0;
    }

    ptr_queue->wr_cnt++;

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_que_deque(
    NPS_HUGE_T              *ptr_queue_id,
    void                    **pptr_data)
{
    linux_queue_t           *ptr_queue = (linux_queue_t *)(*ptr_queue_id);

    if (ptr_queue->wr_cnt == ptr_queue->rd_cnt)
    {
        return (NPS_E_OTHERS);
    }

    /* get data from head */
    *pptr_data = ptr_queue->ptr_entry[ptr_queue->head].ptr_data;
    ptr_queue->ptr_entry[ptr_queue->head].ptr_data = NULL;

    /* calculate head and rd_cnt */
    ptr_queue->head++;
    if (ptr_queue->head >= ptr_queue->capacity)
    {
        ptr_queue->head = 0;
    }

    ptr_queue->rd_cnt++;

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_que_destroy(
    NPS_HUGE_T              *ptr_queue_id)
{
    linux_queue_t           *ptr_queue = (linux_queue_t *)(*ptr_queue_id);

    osal_free(ptr_queue->ptr_entry);
    osal_free(ptr_queue);
    *ptr_queue_id = 0;

    return (NPS_E_OK);
}

NPS_ERROR_NO_T
osal_que_getCount(
    NPS_HUGE_T              *ptr_queue_id,
    unsigned int            *ptr_count)
{
    linux_queue_t           *ptr_queue = (linux_queue_t *)(*ptr_queue_id);

    *ptr_count = ptr_queue->wr_cnt - ptr_queue->rd_cnt;

    return (NPS_E_OK);
}

/* IO */
int
osal_io_copyToUser(
    void                    *ptr_usr_buf,
    void                    *ptr_knl_buf,
    unsigned int            size)
{
    return copy_to_user(ptr_usr_buf, ptr_knl_buf, size);
}

int
osal_io_copyFromUser(
    void                    *ptr_knl_buf,
    void                    *ptr_usr_buf,
    unsigned int            size)
{
    return copy_from_user(ptr_knl_buf, ptr_usr_buf, size);
}

/* dma */
void *
osal_dma_alloc(
    const UI32_T            size)
{
    struct device           *ptr_dev = &_ptr_ext_pci_dev->dev;
    linux_dma_t             *ptr_dma_node = NULL;
    dma_addr_t              phy_addr = 0x0;

    ptr_dma_node = dma_alloc_coherent(ptr_dev, sizeof(linux_dma_t) + size, &phy_addr, GFP_ATOMIC);
    ptr_dma_node->size = sizeof(linux_dma_t) + size;
    ptr_dma_node->phy_addr = phy_addr;

    return (void *)ptr_dma_node->data;
}

NPS_ERROR_NO_T
osal_dma_free(
    void                    *ptr_dma_mem)
{
    struct device           *ptr_dev = &_ptr_ext_pci_dev->dev;
    linux_dma_t             *ptr_dma_node = (linux_dma_t *)(ptr_dma_mem - sizeof(linux_dma_t));

    dma_free_coherent(ptr_dev, ptr_dma_node->size, ptr_dma_node, ptr_dma_node->phy_addr);

    return (NPS_E_OK);
}

dma_addr_t
osal_dma_convertVirtToPhy(
    void                    *ptr_virt_addr)
{
    return virt_to_phys(ptr_virt_addr);
}

void *
osal_dma_convertPhyToVirt(
    const dma_addr_t        phy_addr)
{
    return phys_to_virt(phy_addr);
}

int
osal_dma_flushCache(
    void                    *ptr_virt_addr,
    const unsigned int      size)
{
#if defined(CONFIG_NOT_COHERENT_CACHE) || defined(CONFIG_DMA_NONCOHERENT)
#if defined(dma_cache_wback_inv)
    dma_cache_wback_inv((NPS_HUGE_T)ptr_virt_addr, size);
#else
    dma_cache_sync(NULL, ptr_virt_addr, size, DMA_TO_DEVICE);
#endif
#endif
    return (0);
}

int
osal_dma_invalidateCache(
    void                    *ptr_virt_addr,
    const unsigned int      size)
{
#if defined(CONFIG_NOT_COHERENT_CACHE) || defined(CONFIG_DMA_NONCOHERENT)
#if defined(dma_cache_wback_inv)
    dma_cache_wback_inv((NPS_HUGE_T)ptr_virt_addr, size);
#else
    dma_cache_sync(NULL, ptr_virt_addr, size, DMA_FROM_DEVICE);
#endif
#endif
    return (0);
}

/* skb */
struct sk_buff *
osal_skb_alloc(
    UI32_T                  size)
{
    struct sk_buff          *ptr_skb = NULL;

    /* <skbuff.h>
     * 1. alloc_skb                 (len, flag) : GFP_KERNEL
     * 2. netdev_alloc_skb          (dev, len)  : GFP_ATOMIC
     * 3. dev_alloc_skb             (len)       : GFP_ATOMIC
     * 4. netdev_alloc_skb_ip_align (dev, len)  : GFP_ATOMIC
     *
     * note: Eth header is 14-bytes, we reservd 2-bytes to alignment Ip header
     */
    ptr_skb = dev_alloc_skb(size + NET_IP_ALIGN);
    skb_reserve(ptr_skb, NET_IP_ALIGN);
    skb_put(ptr_skb, size);

    return (ptr_skb);
}

void
osal_skb_free(
    struct sk_buff          *ptr_skb)
{
    /* <skbuff.h>
     * 1. dev_kfree_skb     (*skb) : release in process context
     * 2. dev_kfree_skb_irq (*skb) : release in interrupt context
     * 3. dev_kfree_skb_any (*skb) : release in any context
     */
    dev_kfree_skb_any(ptr_skb);
}

dma_addr_t
osal_skb_mapDma(
    struct sk_buff              *ptr_skb,
    enum dma_data_direction     dir)
{
    struct device           *ptr_dev = &_ptr_ext_pci_dev->dev;
    dma_addr_t              phy_addr = 0x0;

    phy_addr = dma_map_single(ptr_dev, ptr_skb->data, ptr_skb->len, dir);
    if (dma_mapping_error(ptr_dev, phy_addr))
    {
        phy_addr = 0x0;
    }

    return (phy_addr);
}

void
osal_skb_unmapDma(
    const dma_addr_t            phy_addr,
    UI32_T                      size,
    enum dma_data_direction     dir)
{
    struct device           *ptr_dev = &_ptr_ext_pci_dev->dev;

    dma_unmap_single(ptr_dev, phy_addr, size, dir);
}

void
osal_skb_send(
    struct sk_buff          *ptr_skb)
{
    dev_queue_xmit(ptr_skb);
}

void
osal_skb_recv(
    struct sk_buff          *ptr_skb)
{
    /* 1. netif_rx()          : handle skb in process context
     * 2. netif_rx_ni()       : handle skb in interrupt context
     * 3. netif_receive_skb() : for NAPI
     */
    netif_rx(ptr_skb);
}

