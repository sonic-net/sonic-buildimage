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

#include "knet_dev.h"
#include "knet_pci.h"
#include <linux/pci.h>
#include <linux/skbuff.h>

#include <linux/netdevice.h>
#include <linux/etherdevice.h>

static void
clx_dma_rx_packet_queue_init(struct dma_rx_packet_queue *queue, uint32_t max_size)
{
    INIT_LIST_HEAD(&queue->rx_packet);
    spin_lock_init(&queue->lock);
    queue->queue_size = 0;
    queue->max_size = max_size;
}

int
clx_dma_rx_packet_queue_enqueue(struct dma_rx_packet_queue *queue, struct dma_rx_packet *rx_packet)
{
    int ret = 0;

    // Eequeue in the interrupt context.
    spin_lock(&queue->lock);
    if (queue->queue_size >= queue->max_size) {
        queue->enqueue_fail++;
        ret = -ENOMEM;
    } else {
        list_add_tail(&rx_packet->rx_packet, &queue->rx_packet);
        queue->queue_size++;
        queue->enqueue_ok++;
    }
    spin_unlock(&queue->lock);

    return ret;
}

struct dma_rx_packet *
clx_dma_rx_packet_queue_dequeue(struct dma_rx_packet_queue *queue)
{
    struct dma_rx_packet *rx_packet = NULL;
    unsigned long flags = 0;

    // Dequeue in the process context.
    spin_lock_irqsave(&queue->lock, flags);
    if (!list_empty(&queue->rx_packet)) {
        rx_packet = list_first_entry(&queue->rx_packet, struct dma_rx_packet, rx_packet);
        list_del(&rx_packet->rx_packet);
        queue->queue_size--;
        queue->dequeue_ok++;
    } else {
        queue->dequeue_fail++;
    }
    spin_unlock_irqrestore(&queue->lock, flags);

    return rx_packet;
}

/* Process alloc failed case in process context. */
static void
dma_alloc_retry_work(struct work_struct *work)
{
    clx_dma_channel_t *channel = container_of(work, clx_dma_channel_t, alloc_work);
    unsigned long flags = 0;
    uint32_t unit = channel->unit;
    uint32_t channel_id = channel->channel;
    int ret;
    dbg_print(DBG_RX, "Processing alloc retry for unit %u, channel %u, failed_indx:%d\n", unit,
              channel_id, channel->failed_index);

    channel->alloc_fail_count++;
    while (1) {
        ret = clx_dma_drv(unit)->alloc_rx_buffer(unit, channel_id, channel->failed_index);
        if (ret != 0) {
            printk(KERN_WARNING "DMA RX: alloc_rx_buffer retry failed, retrying...\n");
            msleep(10);
            channel->retry_count++;
            continue;
        }
        spin_lock_irqsave(&channel->lock, flags);
        channel->alloc_failed = false;
        channel->work_idx = channel->failed_index;
        // channel->work_idx = (channel->failed_index + 1) % clx_dma_drv(unit)->ring_size;
        clx_dma_drv(unit)->set_work_idx(unit, channel_id, channel->work_idx);
        spin_unlock_irqrestore(&channel->lock, flags);

        clx_intr_drv(unit)->clear_dma_channel_irq(unit, channel_id);
        clx_intr_drv(unit)->unmask_dma_channel_irq(unit, channel_id);
        dbg_print(
            DBG_RX,
            "DMA RX: alloc_rx_buffer succeeded, work_idx updated to %d.failed cnt:%d, retry cnt:%d\n",
            channel->work_idx, channel->alloc_fail_count, channel->retry_count);
        break;
    }
}

int
clx_dma_init(void)
{
    uint32_t unit = 0;
    int rc = 0;
    uint32_t channel = 0, desc_idx;

    for (unit = 0; unit < clx_misc_dev->pci_dev_num; unit++) {
        clx_dma_drv(unit)->dma_channel = (clx_dma_channel_t *)kmalloc_array(
            clx_dma_drv(unit)->rx_channel_num + clx_dma_drv(unit)->tx_channel_num,
            sizeof(clx_dma_channel_t), GFP_ATOMIC);

        memset(clx_dma_drv(unit)->dma_channel, 0x0,
               sizeof(clx_dma_channel_t) *
                   (clx_dma_drv(unit)->rx_channel_num + clx_dma_drv(unit)->tx_channel_num));
        // rx queue
        clx_dma_rx_packet_queue_init(&clx_dma_drv(unit)->rx_queue, 10240);
        init_waitqueue_head(&clx_dma_drv(unit)->rx_wait_queue);

        for (channel = 0;
             channel < clx_dma_drv(unit)->rx_channel_num + clx_dma_drv(unit)->tx_channel_num;
             channel++) {
            /* 1. disable the dma channel */
            clx_dma_drv(unit)->disable_channel(unit, channel);

            /* 2. mask/clear the dma interrupt */
            clx_intr_drv(unit)->mask_dma_channel_irq(unit, channel);
            clx_intr_drv(unit)->clear_dma_channel_irq(unit, channel);
            clx_intr_drv(unit)->mask_dma_channel_error_irq(unit, channel);
            clx_intr_drv(unit)->clear_dma_channel_error_irq(unit, channel);

            /* 3. init the dma channel */
            clx_dma_drv(unit)->reset_channel(unit, channel);
            clx_dma_drv(unit)->alloc_ring_base(
                unit, channel, &clx_dma_drv(unit)->dma_channel[channel].hw_ring_base);
            clx_dma_drv(unit)->set_ring_base(unit, channel,
                                             clx_dma_drv(unit)->dma_channel[channel].hw_ring_base);
            clx_dma_drv(unit)->set_ring_size(unit, channel, clx_dma_drv(unit)->ring_size);
            spin_lock_init(&clx_dma_drv(unit)->dma_channel[channel].lock);

            // alloc_work
            clx_dma_drv(unit)->dma_channel[channel].alloc_failed = false;
            clx_dma_drv(unit)->dma_channel[channel].failed_index = 0;
            INIT_WORK(&clx_dma_drv(unit)->dma_channel[channel].alloc_work, dma_alloc_retry_work);
            clx_dma_drv(unit)->dma_channel[channel].alloc_fail_count = 0;
            clx_dma_drv(unit)->dma_channel[channel].retry_count = 0;
            clx_dma_drv(unit)->dma_channel[channel].unit = unit;
            clx_dma_drv(unit)->dma_channel[channel].channel = channel;

            /* Rx channel */
            clx_dma_drv(unit)->dma_channel[channel].pptr_skb = (struct sk_buff **)kmalloc_array(
                clx_dma_drv(unit)->ring_size, sizeof(struct sk_buff *), GFP_ATOMIC);

            if (channel < clx_dma_drv(unit)->rx_channel_num) {
                for (desc_idx = 0; desc_idx < clx_dma_drv(unit)->ring_size; desc_idx++) {
                    clx_dma_drv(unit)->alloc_rx_buffer(unit, channel, desc_idx);
                }
                clx_dma_drv(unit)->dma_channel[channel].work_idx = clx_dma_drv(unit)->ring_size - 1;
                clx_dma_drv(unit)->set_work_idx(unit, channel,
                                                clx_dma_drv(unit)->dma_channel[channel].work_idx);
            }

            /* 4. unmask the dma interrupt */
            clx_intr_drv(unit)->unmask_dma_channel_irq(unit, channel);
            clx_intr_drv(unit)->unmask_dma_channel_error_irq(unit, channel);

            /* 5. enable the tx dma channel */
            if (channel >= clx_dma_drv(unit)->rx_channel_num) {
                clx_dma_drv(unit)->enable_channel(unit, channel);
                dbg_print(DBG_INFO, "channel:%d work_idx:%d, pop_idx:%d\n", channel,
                          clx_dma_drv(unit)->dma_channel[channel].work_idx,
                          clx_dma_drv(unit)->dma_channel[channel].pop_idx);
            }
        }
    }

    return rc;
}

void
clx_dma_deinit(void)
{
    uint32_t unit = 0;
    uint32_t channel = 0;
    uint32_t desc_idx = 0;
    for (unit = 0; unit < clx_misc_dev->pci_dev_num; unit++) {
        for (channel = 0;
             channel < clx_dma_drv(unit)->rx_channel_num + clx_dma_drv(unit)->tx_channel_num;
             channel++) {
            /* 1. disable the dma channel */
            clx_dma_drv(unit)->disable_channel(unit, channel);

            if (channel < clx_dma_drv(unit)->rx_channel_num) {
                for (desc_idx = 0; desc_idx < clx_dma_drv(unit)->ring_size; desc_idx++) {
                    clx_dma_drv(unit)->free_rx_buffer(unit, channel, desc_idx);
                }
            }

            /* 2. deinit the dma channel */
            kfree((void *)clx_dma_drv(unit)->dma_channel[channel].pptr_skb);
            clx_dma_drv(unit)->free_ring_base(unit, channel,
                                              clx_dma_drv(unit)->dma_channel[channel].hw_ring_base);
        }
        kfree((void *)clx_dma_drv(unit)->dma_channel);
    }
}

int
dma_enable_channel(uint32_t unit)
{
    uint32_t channel;
    for (channel = 0; channel < clx_dma_drv(unit)->rx_channel_num; channel++) {
        clx_dma_drv(unit)->enable_channel(unit, channel);
    }
    return 0;
}

int
dma_disable_channel(uint32_t unit)
{
    uint32_t channel;
    for (channel = 0; channel < clx_dma_drv(unit)->rx_channel_num; channel++) {
        clx_dma_drv(unit)->disable_channel(unit, channel);
    }
    return 0;
}

struct dma_rx_packet *
dma_alloc_rx_packet(uint32_t unit)
{
    struct dma_rx_packet *rx_packet = NULL;
    rx_packet = (struct dma_rx_packet *)kmalloc(sizeof(struct dma_rx_packet), GFP_ATOMIC);
    if (!rx_packet) {
        dbg_print(DBG_CRIT, "Failed to allocate packet context\n");
        return NULL;
    }
    memset(rx_packet, 0x0, sizeof(struct dma_rx_packet));
    INIT_LIST_HEAD(&rx_packet->rx_frag);
    return rx_packet;
}

int
dma_free_rx_packet(uint32_t unit, struct dma_rx_packet *rx_packet, bool delete_skb)
{
    struct dma_rx_frag_buffer *rx_frag, *tmp;

    if (NULL == rx_packet) {
        dbg_print(DBG_WARN, "rx_packet param error!\n");
        return -EINVAL;
    }

    list_for_each_entry_safe(rx_frag, tmp, &rx_packet->rx_frag, rx_frag)
    {
        if (delete_skb) {
            if (NULL != rx_frag->ptr_skb) {
                dev_kfree_skb_any(rx_frag->ptr_skb);
                rx_frag->ptr_skb = NULL;
            } else {
                dbg_print(DBG_CRIT, "rx_frag->ptr_skb is NULL!\n");
            }
        }
        rx_packet->list_count--;
        list_del(&rx_frag->rx_frag);
        kfree(rx_frag);
    }

    kfree(rx_packet);
    return 0;
}

irqreturn_t
dma_rx_msi_hanler(int irq, void *cookie)
{
    uint32_t unit = ((clx_dma_channel_cookie_t *)cookie)->unit;
    uint32_t channel = ((clx_dma_channel_cookie_t *)cookie)->channel;
    struct tasklet_struct *rx_tasklet = &clx_dma_drv(unit)->clx_dma_intr[channel].dma_tasklets;

    clx_intr_drv(unit)->clear_dma_channel_irq(unit, channel);
    tasklet_schedule(rx_tasklet);

    return IRQ_HANDLED;
}

irqreturn_t
dma_tx_msi_hanler(int irq, void *cookie)
{
    uint32_t unit = ((clx_dma_channel_cookie_t *)cookie)->unit;
    uint32_t channel = ((clx_dma_channel_cookie_t *)cookie)->channel;
    struct tasklet_struct *tx_tasklet = &clx_dma_drv(unit)->clx_dma_intr[channel].dma_tasklets;

    clx_intr_drv(unit)->clear_dma_channel_irq(unit, channel);
    tasklet_schedule(tx_tasklet);

    return IRQ_HANDLED;
}

void
dma_rx_tasklet_func(unsigned long data)
{
    uint32_t unit = ((clx_dma_channel_cookie_t *)data)->unit;
    uint32_t channel = ((clx_dma_channel_cookie_t *)data)->channel;
    clx_dma_channel_t *ptr_channel = &clx_dma_drv(unit)->dma_channel[channel];
    clx_rx_action_e rx_action = ACTION_DROP;
    uint32_t port_di = 0, reason, pop_idx, work_idx;
    struct dma_rx_packet *rx_packet = NULL;
    int desc_processed = 0, ret;
    struct profile_rule *rule;
    struct netlink_rx_cookie netlink_cookie = {0};
    int rc = 0;

    spin_lock(&ptr_channel->lock);
    pop_idx = ptr_channel->pop_idx;
    work_idx = ptr_channel->work_idx;
    dbg_print(DBG_RX, "sw1 pop:%d,work:%d\n", ptr_channel->pop_idx, ptr_channel->work_idx);
    clx_dma_drv(unit)->dma_channel[channel].cnt.interrupts++;

    while (desc_processed++ < clx_dma_drv(unit)->ring_size) {
        if (ptr_channel->current_packet == NULL) {
            rx_packet = dma_alloc_rx_packet(unit);
            ptr_channel->current_packet = rx_packet;
        } else {
            rx_packet = ptr_channel->current_packet;
        }

        ret = clx_dma_drv(unit)->alloc_rx_frag(unit, channel, pop_idx, rx_packet);
        if (ret == DMA_RX_NO_NEW_PACKET) {
            break;
        }

        dbg_print(DBG_RX, "rx_packet, rx_complete:%d, list_count:%d, packet_len:%d\n",
                  rx_packet->rx_complete, rx_packet->list_count, rx_packet->packet_len);
        rc = clx_dma_drv(unit)->alloc_rx_buffer(unit, channel, pop_idx);
        pop_idx++;
        pop_idx %= clx_dma_drv(unit)->ring_size;

        if (0 != rc) {
            ptr_channel->alloc_failed = true;
            ptr_channel->failed_index = (work_idx + 1) % clx_dma_drv(unit)->ring_size;
            dbg_print(DBG_RX, "alloc failed pop:%d,work:%d\n", pop_idx, work_idx);

            break;
        }
        work_idx++;
        work_idx %= clx_dma_drv(unit)->ring_size;
        if (rx_packet->rx_complete != 1) {
            continue;
        }

        /* get the packet destination */
        ret = clx_netif_drv(unit)->get_pkt_dst(unit, rx_packet, &port_di, &reason);
        if (0 != ret) {
            kfree(rx_packet);
            continue;
        }

        rule = clx_netif_match_profile(unit, port_di, reason, rx_packet);
        if (!rule) {
            rx_action = ACTION_NETDEV;
        } else {
            rx_action = rule->action;
        }

        switch (rx_action) {
            case ACTION_NETDEV:
                ret = clx_netif_netdev_receive_skb(unit, rx_packet, port_di);
                if (0 != ret) {
                    dbg_print(DBG_RX, "netdev_receive_skb failed. unit=%d, port_di=%d, ret=%d\n",
                              unit, port_di, ret);
                    dma_free_rx_packet(unit, rx_packet, true);
                }
                break;

            case ACTION_NETLINK:
                ret = clx_netif_drv(unit)->parse_netlink_info(unit, rx_packet, &netlink_cookie);
                if (0 != ret) {
                    dbg_print(DBG_RX, "parse_netlink_info failed. unit=%d, ret=%d\n", unit, ret);
                    dma_free_rx_packet(unit, rx_packet, true);
                    break;
                }
                netlink_cookie.nl = &rule->netlink;
                ret = netif_netlink_reveive_skb(unit, rx_packet, port_di, &netlink_cookie);
                if (0 != ret) {
                    dbg_print(DBG_RX, "netlink_reveive_skb failed. unit=%d, port_di=%d, ret=%d\n",
                              unit, port_di, ret);
                    dma_free_rx_packet(unit, rx_packet, true);
                    break;
                }
                break;
            case ACTION_SDK:
                dbg_print(DBG_RX, "enqueue size:%d\n", clx_dma_drv(unit)->rx_queue.queue_size);
                ret = clx_dma_rx_packet_queue_enqueue(&clx_dma_drv(unit)->rx_queue, rx_packet);
                if (ret != 0) {
                    dma_free_rx_packet(unit, rx_packet, true);
                    break;
                }
                wake_up_interruptible(&clx_dma_drv(unit)->rx_wait_queue);
                break;

            case ACTION_FAST_FWD:
                /* ifa2 packet need to modify device id and send to new egress port */
                ret = clx_netif_netdev_receive_send_ifa(unit, rx_packet, port_di);
                if (0 != ret) {
                    dbg_print(
                        DBG_RX,
                        "clx_netif_netdev_receive_send_ifa failed. unit=%d, port_di=%d, ret=%d\n",
                        unit, port_di, ret);
                    dma_free_rx_packet(unit, rx_packet, true);
                }
                break;

            default:
                break;
        }

        ptr_channel->current_packet = NULL;
    }
    ptr_channel->pop_idx = pop_idx;
    ptr_channel->work_idx = work_idx;
    clx_dma_drv(unit)->set_work_idx(unit, channel, ptr_channel->work_idx);
    dbg_print(DBG_RX, "sw2 pop:%d,work:%d\n", ptr_channel->pop_idx, ptr_channel->work_idx);
    spin_unlock(&ptr_channel->lock);
    if (ptr_channel->alloc_failed) {
        schedule_work(&ptr_channel->alloc_work); // schedule work
    } else {
        clx_intr_drv(unit)->unmask_dma_channel_irq(unit, channel);
    }
}

void
dma_tx_tasklet_func(unsigned long data)
{
    clx_dma_channel_cookie_t channel_cookie = *(clx_dma_channel_cookie_t *)data;
    uint32_t unit = channel_cookie.unit;
    uint32_t channel = channel_cookie.channel;
    clx_dma_channel_t *ptr_channel = &clx_dma_drv(unit)->dma_channel[channel];

    spin_lock(&ptr_channel->lock);
    clx_dma_drv(unit)->dma_channel[channel].cnt.interrupts++;

    clx_dma_drv(unit)->tx_callback(unit, channel);
    clx_intr_drv(unit)->unmask_dma_channel_irq(unit, channel);
    spin_unlock(&ptr_channel->lock);
}

void
dma_error_tasklet_func(unsigned long data)
{}

int
clx_ioctl_rx_start(uint32_t unit, unsigned long arg)
{
    uint32_t channel;
    for (unit = 0; unit < clx_misc_dev->pci_dev_num; unit++) {
        for (channel = 0; channel < clx_dma_drv(unit)->rx_channel_num; channel++) {
            clx_dma_drv(unit)->enable_channel(unit, channel);
            dbg_print(DBG_INFO, "channel:%d work_idx:%d, pop_idx:%d\n", channel,
                      clx_dma_drv(unit)->dma_channel[channel].work_idx,
                      clx_dma_drv(unit)->dma_channel[channel].pop_idx);
        }
    }
    return 0;
}

int
clx_ioctl_rx_stop(uint32_t unit, unsigned long arg)
{
    uint32_t channel;
    for (unit = 0; unit < clx_misc_dev->pci_dev_num; unit++) {
        for (channel = 0; channel < clx_dma_drv(unit)->rx_channel_num; channel++) {
            clx_dma_drv(unit)->disable_channel(unit, channel);
        }
    }
    return 0;
}

int
clx_ioctl_get_rx_cnt(uint32_t unit, unsigned long arg)
{
    struct clx_netif_ioctl_rx_cnt __user *user_cnt = (void __user *)arg;
    struct clx_netif_ioctl_rx_cnt k_cnt;

    if (copy_from_user(&k_cnt, user_cnt, sizeof(struct clx_netif_ioctl_rx_cnt)))
        return -EFAULT;

    if (k_cnt.channel >= clx_dma_drv(unit)->rx_channel_num) {
        dbg_print(DBG_TX, "***Error***, rx channel %u is out of valid range (0-%u)\n",
                  k_cnt.channel, clx_dma_drv(unit)->rx_channel_num - 1);
        return -EINVAL;
    }

    k_cnt.enqueue_ok = clx_dma_drv(unit)->rx_queue.enqueue_ok;
    k_cnt.enqueue_fail = clx_dma_drv(unit)->rx_queue.enqueue_fail;
    k_cnt.deque_ok = clx_dma_drv(unit)->rx_queue.dequeue_ok;
    k_cnt.deque_fail = clx_dma_drv(unit)->rx_queue.dequeue_fail;
    memcpy(&k_cnt.dma_cnt, &clx_dma_drv(unit)->dma_channel[k_cnt.channel].cnt,
           sizeof(struct clx_dma_channel_cnt));

    if (copy_to_user(user_cnt, &k_cnt, sizeof(struct clx_netif_ioctl_rx_cnt))) {
        return -EFAULT;
    }

    return 0;
}

int
clx_ioctl_get_tx_cnt(uint32_t unit, unsigned long arg)
{
    struct clx_netif_ioctl_tx_cnt __user *user_cnt = (void __user *)arg;
    struct clx_netif_ioctl_tx_cnt k_cnt;

    if (copy_from_user(&k_cnt, user_cnt, sizeof(struct clx_netif_ioctl_tx_cnt)))
        return -EFAULT;

    if (k_cnt.channel >= clx_dma_drv(unit)->tx_channel_num + clx_dma_drv(unit)->rx_channel_num) {
        dbg_print(DBG_TX, "***Error***, tx channel %u is out of valid range (4-%u)\n",
                  k_cnt.channel,
                  clx_dma_drv(unit)->tx_channel_num + clx_dma_drv(unit)->rx_channel_num - 1);
        return -EINVAL;
    }

    memcpy(&k_cnt.dma_cnt, &clx_dma_drv(unit)->dma_channel[k_cnt.channel].cnt,
           sizeof(struct clx_dma_channel_cnt));

    if (copy_to_user(user_cnt, &k_cnt, sizeof(struct clx_netif_ioctl_tx_cnt))) {
        return -EFAULT;
    }

    return 0;
}

int
clx_ioctl_clear_rx_cnt(uint32_t unit, unsigned long arg)
{
    uint32_t channel = 0;

    if (copy_from_user(&channel, (void __user *)arg, sizeof(uint32_t)))
        return -EFAULT;

    // Validate the channel index
    if (channel >= clx_dma_drv(unit)->rx_channel_num) {
        dbg_print(DBG_RX, "***Error***, rx channel %u is out of valid range (0-%u)\n", channel,
                  clx_dma_drv(unit)->rx_channel_num - 1);
        return -EINVAL;
    }

    memset(&clx_dma_drv(unit)->dma_channel[channel].cnt, 0x0, sizeof(struct clx_dma_channel_cnt));

    clx_dma_drv(unit)->rx_queue.enqueue_ok = 0;
    clx_dma_drv(unit)->rx_queue.enqueue_fail = 0;
    clx_dma_drv(unit)->rx_queue.dequeue_ok = 0;
    clx_dma_drv(unit)->rx_queue.dequeue_fail = 0;

    return 0;
}

int
clx_ioctl_clear_tx_cnt(uint32_t unit, unsigned long arg)
{
    uint32_t channel = 0;

    if (copy_from_user(&channel, (void __user *)arg, sizeof(uint32_t)))
        return -EFAULT;

    // Validate the channel index
    if (channel >= clx_dma_drv(unit)->tx_channel_num + clx_dma_drv(unit)->rx_channel_num) {
        dbg_print(DBG_TX, "***Error***, tx channel %u is out of valid range (4-%u)\n", channel,
                  clx_dma_drv(unit)->tx_channel_num + clx_dma_drv(unit)->rx_channel_num - 1);
        return -EINVAL;
    }

    memset(&clx_dma_drv(unit)->dma_channel[channel].cnt, 0x0, sizeof(struct clx_dma_channel_cnt));

    return 0;
}
