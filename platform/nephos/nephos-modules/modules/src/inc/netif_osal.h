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

/* FILE NAME:  netif_osal.h
 * PURPOSE:
 *      It provide customer linux API.
 * NOTES:
 */

#ifndef NETIF_OSAL_H
#define NETIF_OSAL_H

/* <types.h>
 * ENOMEM         : 12 - Out of memory
 * EFAULT         : 14 - Bad address
 * EBUSY          : 16 - Device or resource busy
 * ENODEV         : 19 - No such device
 * EINVAL         : 22 - Invalid argument

 * <netdevice.h>
 * NETDEV_TX_OK   : 0x00
 * NETDEV_TX_BUSY : 0x10

 * <if_ether.h>
 * ETH_HLEN       : 14      dmac + smac + etyp
 * ETH_ZLEN       : 60      minimum ethernet frame size
 * ETH_DATA_LEN   : 1500
 * ETH_FRAME_LEN  : 1514
 * ETH_FCS_LEN    : 4
 *
 * ETH_P_IP       : 0x0800
 * ETH_P_ARP      : 0x0806
 * ETH_P_IPV6     : 0x86DD
 * ETH_P_SLOW     : 0x8809
 * ETH_P_1588     : 0x88F7

 * <skbuff.h>
 * NET_IP_ALIGN   : 2
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

#include <nps_types.h>
#include <nps_error.h>

/* ----------------------------------------------------------------------------------- macro value */
/* Thread */
#define OSAL_THREAD_NAME_LEN                (16)
#define OSAL_THREAD_DFT_NAME                ("Unknown")

/* Semaphore */
#define OSAL_SEMA_NAME_LEN                  (16)
#define OSAL_SEMA_DFT_NAME                  ("Unknown")

/* Event */
#define OSAL_EVENT_NAME_LEN                 (16)
#define OSAL_EVENT_DFT_NAME                 ("Unknown")

/* Spinlock */
#define OSAL_SPIN_NAME_LEN                  (16)
#define OSAL_SPIN_DFT_NAME                  ("Unknown")

/* Queue */
#define OSAL_QUEUE_NAME_LEN                 (16)
#define OSAL_QUEUE_DFT_NAME                 ("Unknown")

#define OSAL_PRN_BUF_SZ                     (256)
#define OSAL_TICKS_PER_SEC                  (1000000)

/* ----------------------------------------------------------------------------------- struct */
typedef struct linux_thread_s
{
    char                    name[OSAL_THREAD_NAME_LEN + 1];
    struct task_struct      *ptr_task;
    unsigned int            is_stop;
    struct linux_thread_s   *ptr_prev;
    struct linux_thread_s   *ptr_next;

} linux_thread_t;

typedef struct
{
    char                    name[OSAL_SEMA_NAME_LEN + 1];
    struct semaphore        lock;

} linux_sema_t;

typedef struct
{
    char                    name[OSAL_EVENT_NAME_LEN + 1];
    wait_queue_head_t       wait_que;
    unsigned int            condition;

} linux_event_t;

typedef struct
{
    char                    name[OSAL_SPIN_NAME_LEN + 1];
    spinlock_t              spinlock;

} linux_isrlock_t;

typedef struct
{
    void                    *ptr_data;
} linux_queue_entry_t;

typedef struct
{
    char                    name[OSAL_QUEUE_NAME_LEN + 1];
    int                     head;           /* index of the queue head entry can be read  */
    int                     tail;           /* index of the queue tail entry can be write */
    unsigned int            wr_cnt;         /* enqueue total count                        */
    unsigned int            rd_cnt;         /* dequeue total count                        */
    unsigned int            capacity;       /* the queue size                             */
    linux_queue_entry_t     *ptr_entry;     /* the queue entry buffer                     */

} linux_queue_t;

typedef struct
{
    unsigned int            size;
    dma_addr_t              phy_addr;
    char                    data[0];

} linux_dma_t;

/* ----------------------------------------------------------------------------------- function */
void *
osal_memset(
    void                    *ptr_mem,
    const I32_T             value,
    const UI32_T            num);

void *
osal_memcpy(
    void                    *ptr_dst,
    const void              *ptr_src,
    const UI32_T            num);

UI32_T
osal_strlen(
    const C8_T              *ptr_str);

void
osal_printf(
    const C8_T              *ptr_fmt,
    ...);

void *
osal_alloc(
    const UI32_T            size);

void
osal_free(
    const void              *ptr_mem);

/* thread */
NPS_ERROR_NO_T
osal_init(void);

NPS_ERROR_NO_T
osal_deinit(void);

NPS_ERROR_NO_T
osal_createThread (
    const C8_T              *ptr_thread_name,
    const UI32_T            stack_size,
    const UI32_T            priority,
    void                    (function)(void*),
    void                    *ptr_arg,
    NPS_THREAD_ID_T         *ptr_thread_id);

NPS_ERROR_NO_T
osal_stopThread(
    NPS_THREAD_ID_T         *ptr_thread_id);

NPS_ERROR_NO_T
osal_destroyThread(
    NPS_THREAD_ID_T         *ptr_thread_id);

void
osal_initRunThread(
    void);

NPS_ERROR_NO_T
osal_isRunThread(
    void);

void
osal_exitRunThread(
    void);

/* semaphore */
NPS_ERROR_NO_T
osal_createSemaphore(
    const C8_T              *ptr_sema_name,
    const UI32_T            sema_count,
    NPS_SEMAPHORE_ID_T      *ptr_semaphore_id);

NPS_ERROR_NO_T
osal_takeSemaphore(
    NPS_SEMAPHORE_ID_T      *ptr_semaphore_id,
    UI32_T                  time_out);

NPS_ERROR_NO_T
osal_giveSemaphore(
    NPS_SEMAPHORE_ID_T      *ptr_semaphore_id);

NPS_ERROR_NO_T
osal_destroySemaphore(
    NPS_SEMAPHORE_ID_T      *ptr_semaphore_id);

/* event */
NPS_ERROR_NO_T
osal_createEvent(
    const C8_T              *ptr_event_name,
    NPS_SEMAPHORE_ID_T      *ptr_event_id);

NPS_ERROR_NO_T
osal_waitEvent(
    NPS_SEMAPHORE_ID_T      *ptr_event_id);

NPS_ERROR_NO_T
osal_triggerEvent(
    NPS_SEMAPHORE_ID_T      *ptr_event_id);

NPS_ERROR_NO_T
osal_destroyEvent(
    NPS_SEMAPHORE_ID_T      *ptr_event_id);

/* isr_lock */
NPS_ERROR_NO_T
osal_createIsrLock(
    const C8_T              *ptr_isrlock_name,
    NPS_ISRLOCK_ID_T        *ptr_isrlock_id);

NPS_ERROR_NO_T
osal_takeIsrLock(
    NPS_ISRLOCK_ID_T        *ptr_isrlock_id,
    NPS_IRQ_FLAGS_T         *ptr_irq_flags);

NPS_ERROR_NO_T
osal_giveIsrLock(
    NPS_ISRLOCK_ID_T        *ptr_isrlock_id,
    NPS_IRQ_FLAGS_T         *ptr_irq_flags);

NPS_ERROR_NO_T
osal_destroyIsrLock(
    NPS_ISRLOCK_ID_T        *ptr_isrlock_id);

/* timer */
NPS_ERROR_NO_T
osal_sleepThread(
    const UI32_T            usecond);

NPS_ERROR_NO_T
osal_getTime(
    NPS_TIME_T              *ptr_time);

/* queue */
NPS_ERROR_NO_T
osal_que_create(
    NPS_HUGE_T              *ptr_queue_id,
    UI32_T                  capacity);

NPS_ERROR_NO_T
osal_que_enque(
    NPS_HUGE_T              *ptr_queue_id,
    void                    *ptr_data);

NPS_ERROR_NO_T
osal_que_deque(
    NPS_HUGE_T              *ptr_queue_id,
    void                    **pptr_data);

NPS_ERROR_NO_T
osal_que_destroy(
    NPS_HUGE_T              *ptr_queue_id);

NPS_ERROR_NO_T
osal_que_getCount(
    NPS_HUGE_T              *ptr_queue_id,
    unsigned int            *ptr_count);

/* IO */
int
osal_io_copyToUser(
    void                    *ptr_usr_buf,
    void                    *ptr_knl_buf,
    unsigned int            size);

int
osal_io_copyFromUser(
    void                    *ptr_knl_buf,
    void                    *ptr_usr_buf,
    unsigned int            size);

/* dma */
void *
osal_dma_alloc(
    const UI32_T            size);

NPS_ERROR_NO_T
osal_dma_free(
    void                    *ptr_dma_mem);

dma_addr_t
osal_dma_convertVirtToPhy(
    void                    *ptr_virt_addr);

void *
osal_dma_convertPhyToVirt(
    const dma_addr_t        phy_addr);

int
osal_dma_flushCache(
    void                    *ptr_virt_addr,
    const unsigned int      size);

int
osal_dma_invalidateCache(
    void                    *ptr_virt_addr,
    const unsigned int      size);

/* skb */
struct sk_buff *
osal_skb_alloc(
    UI32_T                  size);

void
osal_skb_free(
    struct sk_buff          *ptr_skb);

dma_addr_t
osal_skb_mapDma(
    struct sk_buff          *ptr_skb,
    enum dma_data_direction dir);

void
osal_skb_unmapDma(
    const dma_addr_t        phy_addr,
    UI32_T                  size,
    enum dma_data_direction dir);

void
osal_skb_send(
    struct sk_buff          *ptr_skb);

void
osal_skb_recv(
    struct sk_buff          *ptr_skb);

#endif /* end of NETIF_OSAL_H */
