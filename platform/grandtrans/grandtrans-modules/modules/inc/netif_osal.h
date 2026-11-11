#ifndef _NETIF_OSAL_H_
#define _NETIF_OSAL_H_

#include "netif_types.h"

#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/wait.h>
#include <linux/kthread.h>

#define OSAL_PRN_BUF_SZ         256
#define OSAL_THREAD_NAME_LEN    32
#define OSAL_THREAD_DFT_NAME    "osal_thread"
#define OSAL_SEMA_NAME_LEN      32
#define OSAL_SEMA_DFT_NAME      "osal_sema"
#define OSAL_SPIN_NAME_LEN      32
#define OSAL_SPIN_DFT_NAME      "osal_spin"
#define OSAL_EVENT_NAME_LEN     32
#define OSAL_EVENT_DFT_NAME     "osal_event"
#define OSAL_USEC_PER_TICK      1000
#define GT_SEMAPHORE_BINARY     1

typedef struct _linux_thread_t {
    struct _linux_thread_t *ptr_prev;
    struct _linux_thread_t *ptr_next;
    char                    name[OSAL_THREAD_NAME_LEN + 1];
    struct task_struct     *ptr_task;
    BOOL                    is_stop;
} linux_thread_t;

typedef struct {
    char            name[OSAL_SEMA_NAME_LEN + 1];
    struct semaphore lock;
} linux_sema_t;

typedef struct {
    char                name[OSAL_EVENT_NAME_LEN + 1];
    BOOL                condition;
    wait_queue_head_t   wait_que;
} linux_event_t;

typedef struct {
    char        name[OSAL_SPIN_NAME_LEN + 1];
    spinlock_t  spinlock;
} linux_isrlock_t;

typedef struct {
    void *ptr_data;
} linux_queue_entry_t;

typedef struct {
    UI32_T               head;
    UI32_T               tail;
    UI32_T               wr_cnt;
    UI32_T               rd_cnt;
    UI32_T               capacity;
    linux_queue_entry_t *ptr_entry;
} linux_queue_t;

typedef struct {
    dma_addr_t      phy_addr;
    size_t          size;
    uint8_t         data[0];
} linux_dma_t;

/* OSAL function declarations */
void * osal_memset(void *ptr_mem, const I32_T value, const UI32_T num);
void * osal_memcpy(void *ptr_dst, const void *ptr_src, const UI32_T num);
UI32_T osal_strlen(const C8_T *ptr_str);
void   osal_printf(const C8_T *ptr_fmt, ...);
void * osal_alloc(const UI32_T size);
void   osal_free(const void *ptr_mem);

GT_ERROR_NO_T osal_init(void);
GT_ERROR_NO_T osal_deinit(void);
GT_ERROR_NO_T osal_createThread(const C8_T *ptr_thread_name, const UI32_T stack_size,
    const UI32_T priority, int (function)(void*), void *ptr_arg, GT_THREAD_ID_T *ptr_thread_id);
GT_ERROR_NO_T osal_stopThread(GT_THREAD_ID_T *ptr_thread_id);
GT_ERROR_NO_T osal_destroyThread(GT_THREAD_ID_T *ptr_thread_id);
void osal_initRunThread(void);
GT_ERROR_NO_T osal_isRunThread(void);
void osal_exitRunThread(void);

GT_ERROR_NO_T osal_createSemaphore(const C8_T *ptr_sema_name, const UI32_T sema_count,
    GT_SEMAPHORE_ID_T *ptr_semaphore_id);
GT_ERROR_NO_T osal_takeSemaphore(GT_SEMAPHORE_ID_T *ptr_semaphore_id, UI32_T time_out);
GT_ERROR_NO_T osal_giveSemaphore(GT_SEMAPHORE_ID_T *ptr_semaphore_id);
GT_ERROR_NO_T osal_destroySemaphore(GT_SEMAPHORE_ID_T *ptr_semaphore_id);

GT_ERROR_NO_T osal_createEvent(const C8_T *ptr_event_name, GT_SEMAPHORE_ID_T *ptr_event_id);
GT_ERROR_NO_T osal_waitEvent(GT_SEMAPHORE_ID_T *ptr_event_id);
GT_ERROR_NO_T osal_triggerEvent(GT_SEMAPHORE_ID_T *ptr_event_id);
GT_ERROR_NO_T osal_destroyEvent(GT_SEMAPHORE_ID_T *ptr_event_id);

GT_ERROR_NO_T osal_createIsrLock(const C8_T *ptr_isrlock_name, GT_ISRLOCK_ID_T *ptr_isrlock_id);
GT_ERROR_NO_T osal_takeIsrLock(GT_ISRLOCK_ID_T *ptr_isrlock_id, GT_IRQ_FLAGS_T *ptr_irq_flags);
GT_ERROR_NO_T osal_giveIsrLock(GT_ISRLOCK_ID_T *ptr_isrlock_id, GT_IRQ_FLAGS_T *ptr_irq_flags);
GT_ERROR_NO_T osal_destroyIsrLock(GT_ISRLOCK_ID_T *ptr_isrlock_id);

GT_ERROR_NO_T osal_sleepThread(const UI32_T usecond);
GT_ERROR_NO_T osal_getTime(GT_TIME_T *ptr_time);

GT_ERROR_NO_T osal_que_create(GT_HUGE_T *ptr_queue_id, UI32_T capacity);
GT_ERROR_NO_T osal_que_enque(GT_HUGE_T *ptr_queue_id, void *ptr_data);
GT_ERROR_NO_T osal_que_deque(GT_HUGE_T *ptr_queue_id, void **pptr_data);
GT_ERROR_NO_T osal_que_destroy(GT_HUGE_T *ptr_queue_id);
GT_ERROR_NO_T osal_que_getCount(GT_HUGE_T *ptr_queue_id, unsigned int *ptr_count);

int  osal_io_copyToUser(void *ptr_usr_buf, void *ptr_knl_buf, unsigned int size);
int  osal_io_copyFromUser(void *ptr_knl_buf, void *ptr_usr_buf, unsigned int size);

void *       osal_dma_alloc(const UI32_T size);
GT_ERROR_NO_T osal_dma_free(void *ptr_dma_mem);
dma_addr_t   osal_dma_convertVirtToPhy(void *ptr_virt_addr);
void *       osal_dma_convertPhyToVirt(const dma_addr_t phy_addr);
int          osal_dma_flushCache(void *ptr_virt_addr, const unsigned int size);
int          osal_dma_invalidateCache(void *ptr_virt_addr, const unsigned int size);

struct sk_buff * osal_skb_alloc(UI32_T size);
void             osal_skb_free(struct sk_buff *ptr_skb);
dma_addr_t       osal_skb_mapDma(struct sk_buff *ptr_skb, enum dma_data_direction dir);
void             osal_skb_unmapDma(const dma_addr_t phy_addr, UI32_T size, enum dma_data_direction dir);
void             osal_skb_send(struct sk_buff *ptr_skb);
void             osal_skb_recv(struct sk_buff *ptr_skb);

#endif
