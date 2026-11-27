#ifndef _SHM_RING_H_
#define _SHM_RING_H_

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/ioctl.h>
#else
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#endif

#define SHM_RING_SIZE   256
#define SHM_RING_MASK   (SHM_RING_SIZE - 1)
#define SHM_PKT_SIZE    9600
#define SHM_META_SIZE   32

/* ---- 32-byte metadata header ---- */
struct shm_metadata {
    uint16_t netif_id;
    uint16_t port_id;
    uint8_t  rsvd[28];
};

struct pkt_slot {
    volatile uint32_t len;           /* SHM_META_SIZE + packet length */
    volatile uint32_t ready;
    struct shm_metadata meta;
    uint8_t             data[SHM_PKT_SIZE];
};

struct shm_ring {
    volatile uint32_t head;
    uint8_t           _pad1[60];
    volatile uint32_t tail;
    uint8_t           _pad2[60];
    struct pkt_slot   slots[SHM_RING_SIZE];
};

struct shm_region {
    struct shm_ring rx;          /* user(producer) -> kernel(consumer) */
    struct shm_ring tx;          /* kernel(producers) -> user(consumer) */
};

/* ---- Lock-free SPSC ring helpers ---- */

#ifdef __KERNEL__
#define SHM_MB()    smp_mb()
#define SHM_WMB()   smp_wmb()
#define SHM_RMB()   smp_rmb()
#else
#define SHM_MB()    __sync_synchronize()
#define SHM_WMB()   __sync_synchronize()
#define SHM_RMB()   __sync_synchronize()
#endif

/*
 * enqueue: producer writes slot, then advances head.
 * Before calling, the caller must ensure single-producer safety
 * (e.g. rx ring: user-space single writer; tx ring: kernel spinlock).
 */
static inline int shm_ring_enqueue(struct shm_ring *r,
                                   const struct shm_metadata *meta,
                                   const uint8_t *data, uint32_t data_len)
{
    uint32_t h, t, next, total;

    h = r->head;
    t = r->tail;

    next = (h + 1) & SHM_RING_MASK;
    if (next == t)
        return -1;

    if (data_len > SHM_PKT_SIZE)
        return -2;

    total = SHM_META_SIZE + data_len;

    r->slots[h].meta = *meta;
#ifdef __KERNEL__
    memcpy(r->slots[h].data, data, data_len);
#else
    memcpy((void *)r->slots[h].data, data, data_len);
#endif

    SHM_WMB();
    r->slots[h].len   = total;
    r->slots[h].ready = 1;
    SHM_WMB();
    r->head = next;
    return 0;
}

static inline int shm_ring_dequeue(struct shm_ring *r,
                                   struct shm_metadata *meta,
                                   uint8_t *buf, uint32_t *total_len,
                                   uint32_t *data_len)
{
    uint32_t h, t;

    t = r->tail;
    h = r->head;

    if (t == h)
        return -1;
    if (r->slots[t].ready == 0)
        return -1;

    SHM_RMB();
    *total_len = r->slots[t].len;
    *data_len  = *total_len - SHM_META_SIZE;
    *meta      = r->slots[t].meta;

#ifdef __KERNEL__
    memcpy(buf, r->slots[t].data, *data_len);
#else
    memcpy(buf, (const void *)r->slots[t].data, *data_len);
#endif

    SHM_MB();
    r->slots[t].ready = 0;
    r->tail = (t + 1) & SHM_RING_MASK;
    return 0;
}

static inline int shm_ring_empty(struct shm_ring *r)
{
    return r->head == r->tail;
}

#endif
