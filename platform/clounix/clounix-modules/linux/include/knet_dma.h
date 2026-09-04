/*******************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software and the information contained therein are protected by
 *  copyright and other intellectual property laws and terms herein is
 *  confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Clounix (Shanghai) Technology Limited. (C) 2020-2025
 *
 *  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("CLOUNIX SOFTWARE")
 *  RECEIVED FROM CLOUNIX AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
 *  AN "AS-IS" BASIS ONLY. CLOUNIX EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 *  NEITHER DOES CLOUNIX PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 *  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 *  SUPPLIED WITH THE CLOUNIX SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
 *  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. CLOUNIX SHALL ALSO
 *  NOT BE RESPONSIBLE FOR ANY CLOUNIX SOFTWARE RELEASES MADE TO BUYER'S
 *  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *  BUYER'S SOLE AND EXCLUSIVE REMEDY AND CLOUNIX'S ENTIRE AND CUMULATIVE
 *  LIABILITY WITH RESPECT TO THE CLOUNIX SOFTWARE RELEASED HEREUNDER WILL BE,
 *  AT CLOUNIX'S OPTION, TO REVISE OR REPLACE THE CLOUNIX SOFTWARE AT ISSUE,
 *  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
 *  CLOUNIX FOR SUCH CLOUNIX SOFTWARE AT ISSUE.
 *
 *  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *  WITH THE LAWS OF THE PEOPLE'S REPUBLIC OF CHINA, EXCLUDING ITS CONFLICT OF
 *  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
 *  RELATED THERETO SHALL BE SETTLED BY LAWSUIT IN SHANGHAI,CHINA UNDER.
 *
 *******************************************************************************/

#ifndef __CLX_DMA_H__
#define __CLX_DMA_H__

#include "knet_types.h"
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/kfifo.h>

typedef struct {
    uint32_t unit;
    uint32_t channel;
} clx_dma_channel_cookie_t;

typedef struct {
    /* interrupt */
    struct tasklet_struct dma_tasklet;
    clx_dma_channel_cookie_t channel_cookie;
} clx_dma_channl_error_t;

struct rx_pph_info {
    uint32_t vlan_tag;
    uint32_t vlan_pop_num;
    uint32_t cpu_reason;
};

struct dma_rx_frag_buffer {
    struct sk_buff *ptr_skb;
    dma_addr_t dma_addr;
    struct list_head rx_frag;
};

struct dma_rx_packet {
    uint32_t rx_complete;
    uint32_t list_count;
    uint32_t packet_len;
    struct list_head rx_frag;   // for dma_rx_frag_buffer
    struct list_head rx_packet; // for dma_rx_packet
    struct rx_pph_info pph_info;
};

struct dma_rx_packet_queue {
    struct list_head rx_packet; // for dma_rx_packet
    spinlock_t lock;
    uint32_t queue_size;
    uint32_t max_size;

    uint32_t enqueue_ok;
    uint32_t enqueue_fail;
    uint32_t dequeue_ok;
    uint32_t dequeue_fail;
};

enum { DMA_RX_INCOMPLETE_PACKET = 0, DMA_RX_COMPLETE_PACKET, DMA_RX_NO_NEW_PACKET };

/* DMA DRV */
typedef int (*clx_dma_channel_op)(uint32_t unit, uint32_t channel);
typedef int (*clx_dma_channel_set)(uint32_t unit, uint32_t channel, uint32_t para);
typedef int (*clx_dma_channel_set2)(uint32_t unit, uint32_t channel, dma_addr_t dma_addr);
typedef int (*clx_dma_channel_get)(uint32_t unit, uint32_t channel, uint32_t *dma_addr);
typedef int (*clx_dma_channel_get2)(uint32_t unit, uint32_t channel, dma_addr_t *dma_addr);
typedef int (*CLX_DMA_HANDLE_ERROR)(uint32_t unit, uint32_t channel);
typedef void (*clx_tasklet_func)(unsigned long data);
typedef struct sk_buff *(*clx_dma_construct_skb)(uint32_t unit, uint32_t channel);
typedef int (*clx_dma_prepare_pph)(uint32_t unit, uint32_t port_di, uint8_t tc, void *ptr_pph);
typedef int (*clx_dma_tx_packet)(uint32_t unit, uint32_t channel, struct sk_buff *ptr_skb);
typedef int (*clx_dma_tx_callback)(uint32_t unit, uint32_t channel);
typedef int (*clx_dma_alloc_rx_frag)(uint32_t unit,
                                     uint32_t channel,
                                     uint32_t pop_idx,
                                     struct dma_rx_packet *rx_packet);
typedef struct {
    struct tasklet_struct dma_tasklets;
    clx_tasklet_func dma_handler;
    clx_dma_channel_cookie_t channel_cookie;
} clx_dma_intr_t;

typedef struct {
    void *sw_ring_base;
    dma_addr_t hw_ring_base;
    uint32_t work_idx;
    uint32_t pop_idx;

    struct sk_buff **pptr_skb;
    struct dma_rx_packet *current_packet;

    struct clx_dma_channel_cnt cnt;
    spinlock_t lock;

    /* Process alloc failed case in process context. */
    struct work_struct alloc_work; // work queue for alloc failed case
    bool alloc_failed;             // alloc failed flag
    uint32_t failed_index;         // alloc failed index
    uint32_t alloc_fail_count;     // alloc failed count
    uint32_t retry_count;          // retry count
    uint32_t unit;                 // save the unit
    uint32_t channel;              // save the channel

} clx_dma_channel_t;

typedef struct {
    uint32_t channel_num;
    uint32_t rx_channel_num;
    uint32_t tx_channel_num;
    uint32_t ring_size;
    uint32_t descriptor_size;
    uint32_t dma_hdr_size;

    clx_dma_channel_op enable_channel;
    clx_dma_channel_op disable_channel;
    clx_dma_channel_op reset_channel;
    clx_dma_channel_op restart_channel;
    clx_dma_channel_set2 set_ring_base;
    clx_dma_channel_get2 get_ring_base;
    clx_dma_channel_set set_ring_size;
    clx_dma_channel_get get_ring_size;
    clx_dma_channel_set set_work_idx;
    clx_dma_channel_get get_work_idx;
    clx_dma_channel_get get_pop_idx;

    clx_dma_channel_get2 alloc_ring_base;
    clx_dma_channel_set2 free_ring_base;
    clx_dma_channel_set alloc_rx_buffer;
    clx_dma_channel_set free_rx_buffer;

    clx_dma_alloc_rx_frag alloc_rx_frag;
    clx_dma_prepare_pph prepare_pph;
    clx_dma_tx_packet tx_packet;
    clx_dma_tx_callback tx_callback;

    CLX_DMA_HANDLE_ERROR handle_error;
    clx_dma_channel_t *dma_channel;
    /* receive to sdk */
    struct dma_rx_packet_queue rx_queue;
    wait_queue_head_t rx_wait_queue;

    /* interrupt */
    clx_dma_intr_t *clx_dma_intr;
    clx_dma_channl_error_t error_channel;

} clx_dma_drv_cb_t;

int
clx_dma_init(void);
void
clx_dma_deinit(void);
void
dma_rx_tasklet_func(unsigned long data);
void
dma_tx_tasklet_func(unsigned long data);
void
dma_error_tasklet_func(unsigned long data);
irqreturn_t
dma_rx_msi_hanler(int irq, void *cookie);
irqreturn_t
dma_tx_msi_hanler(int irq, void *cookie);

struct dma_rx_packet *
dma_alloc_rx_packet(uint32_t unit);
int
dma_free_rx_packet(uint32_t unit, struct dma_rx_packet *rx_packet, bool delete_skb);

int
clx_dma_rx_packet_queue_enqueue(struct dma_rx_packet_queue *queue, struct dma_rx_packet *pkt_list);
struct dma_rx_packet *
clx_dma_rx_packet_queue_dequeue(struct dma_rx_packet_queue *queue);

int
clx_ioctl_get_tx_cnt(uint32_t unit, unsigned long arg);
int
clx_ioctl_get_rx_cnt(uint32_t unit, unsigned long arg);
int
clx_ioctl_clear_tx_cnt(uint32_t unit, unsigned long arg);
int
clx_ioctl_clear_rx_cnt(uint32_t unit, unsigned long arg);

int
dma_enable_channel(uint32_t unit);

int
dma_disable_channel(uint32_t unit);

int
clx_ioctl_rx_start(uint32_t unit, unsigned long arg);

int
clx_ioctl_rx_stop(uint32_t unit, unsigned long arg);

int
nb_handle_ifa_pkt(const uint32_t unit, struct dma_rx_packet *rx_packet);
#endif // __CLX_DMA_H__
