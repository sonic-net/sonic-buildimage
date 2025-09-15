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
#include "knet_buffer.h"
#include "nb_chip.h"
#include <linux/slab.h>
#include <linux/skbuff.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>

#define descriptor(unit, channel, desc_idx) \
    (&(((volatile nb_descriptor_t           \
             *)(clx_dma_drv(unit)->dma_channel[channel].sw_ring_base))[desc_idx]))

#define NB_PKT_RX_MOD_HW_REASON (511)
static unsigned char g_mod_default_mac[ETH_ALEN] = {0x70, 0x06, 0x92, 0x6D, 0x00, 0x01};

static int
namchabar_top_irq_mask(uint32_t unit)
{
    uint32_t intr_mask = 0xFFFFFFFF;

    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, NB_CTL_CHAIN_INTR_MSK, &intr_mask,
                                              sizeof(uint32_t));
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, NB_CFG_PDMA2PCIE_INTR_MASK_ALL, &intr_mask,
                                              sizeof(uint32_t));

    return 0;
}

static int
namchabar_top_irq_unmask(uint32_t unit)
{
    uint32_t intr_mask = 0x0;

    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, NB_CTL_CHAIN_INTR_MSK, &intr_mask,
                                              sizeof(uint32_t));
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, NB_CFG_PDMA2PCIE_INTR_MASK_ALL, &intr_mask,
                                              sizeof(uint32_t));

    return 0;
}

static int
namchabar_dma_irq_mask_all(uint32_t unit)
{
    uint32_t intr_mask = 0xFFFFFFFF;

    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, NB_CFG_PDMA2PCIE_INTR_MASK_ALL, &intr_mask,
                                              sizeof(uint32_t));

    return 0;
}

static int
namchabar_dma_irq_unmask_all(uint32_t unit)
{
    uint32_t intr_mask = 0x0;

    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, NB_CFG_PDMA2PCIE_INTR_MASK_ALL, &intr_mask,
                                              sizeof(uint32_t));

    return 0;
}

static int
namchabar_get_dma_irq_channel(uint32_t unit, uint32_t *channel_bmp)
{
    uint32_t intr_mask = 0x0;
    uint32_t channel = 0;
    clx_misc_dev->clx_pci_dev[unit]->read_cb(unit, NB_STA_PDMA_NORMAL_INTR, channel_bmp,
                                             sizeof(uint32_t));
    dbg_print(DBG_DEBUG, "dma irq reg status:0x%x\n", *channel_bmp);
    if (*channel_bmp != 0) {
        for (channel = 0; channel < clx_dma_drv(unit)->channel_num; channel++) {
            if (!NB_GET_BITMAP(*channel_bmp, 0x1 << channel)) {
                continue;
            }
            clx_misc_dev->clx_pci_dev[unit]->read_cb(unit, CFG_PDMA2PCIE_INTR_CHx_MASK(channel),
                                                     &intr_mask, sizeof(uint32_t));
            if (intr_mask != 0x0) {
                NB_CLR_BITMAP(*channel_bmp, 1 << channel);
            }
        }
    }
    dbg_print(DBG_DEBUG, "dma irq status:0x%x\n", *channel_bmp);

    return 0;
}

static int
namchabar_get_dma_error_irq_channel(uint32_t unit, uint32_t *channel_bmp)
{
    uint32_t dmachain_status = 0;
    uint32_t channel = 0;
    uint32_t intr_status = 0;

    clx_misc_dev->clx_pci_dev[unit]->read_cb(unit, CHAIN28_SLV_INTR_REG, &dmachain_status,
                                             sizeof(uint32_t));
    if (dmachain_status & PDMA_ERROR_IRQ_BIT) {
        for (channel = 0;
             channel < clx_dma_drv(unit)->rx_channel_num + clx_dma_drv(unit)->tx_channel_num;
             channel++) {
            clx_misc_dev->clx_pci_dev[unit]->read_cb(unit, IRQ_PDMA_ABNORMAL_CHx_INTR(channel),
                                                     &intr_status, sizeof(uint32_t));
            if (intr_status != 0x0) {
                *channel_bmp |= 1 << channel;
            }
        }
    }

    return 0;
}

static int
nb_mask_dma_channel_irq(uint32_t unit, uint32_t channel)
{
    uint32_t intr_mask = 0x1;
    dbg_print(DBG_DEBUG, "mask channel:%d,addr:0x%x\n", channel,
              CFG_PDMA2PCIE_INTR_CHx_MASK(channel));
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, CFG_PDMA2PCIE_INTR_CHx_MASK(channel),
                                              &intr_mask, sizeof(uint32_t));
    return 0;
}

static int
nb_unmask_dma_channel_irq(uint32_t unit, uint32_t channel)
{
    uint32_t intr_mask = 0x0;
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, CFG_PDMA2PCIE_INTR_CHx_MASK(channel),
                                              &intr_mask, sizeof(uint32_t));
    return 0;
}

static int
nb_clear_dma_channel_irq(uint32_t unit, uint32_t channel)
{
    uint32_t intr_clr = 0x1 << channel;
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, NB_CFW_PDMA2PCIE_INTR_CLR, &intr_clr,
                                              sizeof(uint32_t));
    return 0;
}

static int
nb_mask_dma_channel_error_irq(uint32_t unit, uint32_t channel)
{
    uint32_t intr_mask = 0x1;
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, IRQ_PDMA_ABNORMAL_CHx_INTR_MSK(channel),
                                              &intr_mask, sizeof(uint32_t));
    return 0;
}

static int
nb_unmask_dma_channel_error_irq(uint32_t unit, uint32_t channel)
{
    uint32_t intr_mask = 0x0;
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, IRQ_PDMA_ABNORMAL_CHx_INTR_MSK(channel),
                                              &intr_mask, sizeof(uint32_t));
    return 0;
}

static int
nb_dma_channel_error_irq_clear(uint32_t unit, uint32_t channel)
{
    uint32_t intr_clr = 0x1;
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, IRQ_PDMA_ABNORMAL_CHx_INTR(channel), &intr_clr,
                                              sizeof(uint32_t));
    return 0;
}

/* dma drv */
static int
nb_dma_channel_enable(uint32_t unit, uint32_t channel)
{
    uint32_t enable;
    dbg_print(DBG_DEBUG, "unit=%d, channel=%d, enable.\n", unit, channel);
    clx_misc_dev->clx_pci_dev[unit]->read_cb(unit, NB_CFG_PDMA_CH_ENABLE, &enable,
                                             sizeof(uint32_t));
    NB_SET_BITMAP(enable, 1 << channel);
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, NB_CFG_PDMA_CH_ENABLE, &enable,
                                              sizeof(uint32_t));
    return 0;
}

static int
nb_dma_channel_disable(uint32_t unit, uint32_t channel)
{
    uint32_t enable;
    dbg_print(DBG_DEBUG, "disable channel:%d,addr:0x%x\n", channel, NB_CFG_PDMA_CH_ENABLE);
    clx_misc_dev->clx_pci_dev[unit]->read_cb(unit, NB_CFG_PDMA_CH_ENABLE, &enable,
                                             sizeof(uint32_t));
    NB_CLR_BITMAP(enable, 1 << channel);
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, NB_CFG_PDMA_CH_ENABLE, &enable,
                                              sizeof(uint32_t));
    return 0;
}

static int
nb_dma_channel_ring_base_set(uint32_t unit, uint32_t channel, clx_addr_t ring_base)
{
    dbg_print(DBG_DEBUG, "unit=%d, channel=%d, ring base=0x%llx\n", unit, channel, ring_base);
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, NB_CFG_PDMA_CHx_RING_BASE(channel),
                                              (uint32_t *)&ring_base, sizeof(clx_addr_t));
    return 0;
}

static int
nb_dma_channel_ring_base_get(uint32_t unit, uint32_t channel, clx_addr_t *ring_base)
{
    clx_misc_dev->clx_pci_dev[unit]->read_cb(unit, NB_CFG_PDMA_CHx_RING_BASE(channel),
                                             (uint32_t *)ring_base, sizeof(clx_addr_t));
    return 0;
}

static int
nb_dma_channel_ring_size_set(uint32_t unit, uint32_t channel, uint32_t ring_size)
{
    dbg_print(DBG_DEBUG, "unit=%d, channel=%d, ring size=%d\n", unit, channel, ring_size);
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, NB_CFG_PDMA_CHx_RING_SIZE(channel), &ring_size,
                                              sizeof(uint32_t));
    return 0;
}

static int
nb_dma_channel_ring_size_get(uint32_t unit, uint32_t channel, uint32_t *ring_size)
{
    clx_misc_dev->clx_pci_dev[unit]->read_cb(unit, NB_CFG_PDMA_CHx_RING_SIZE(channel), ring_size,
                                             sizeof(uint32_t));
    return 0;
}

static int
nb_dma_channel_work_idx_set(uint32_t unit, uint32_t channel, uint32_t work_idx)
{
    dbg_print(DBG_DEBUG, "unit=%d, channel=%d, work_idx=%d\n", unit, channel, work_idx);
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, NB_CFG_PDMA_CHx_DESC_WORK_IDX(channel),
                                              &work_idx, sizeof(uint32_t));
    return 0;
}

static int
nb_dma_channel_work_idx_get(uint32_t unit, uint32_t channel, uint32_t *work_idx)
{
    clx_misc_dev->clx_pci_dev[unit]->read_cb(unit, NB_CFG_PDMA_CHx_DESC_WORK_IDX(channel), work_idx,
                                             sizeof(uint32_t));
    return 0;
}

static int
nb_dma_channel_pop_idx_get(uint32_t unit, uint32_t channel, uint32_t *pop_idx)
{
    clx_misc_dev->clx_pci_dev[unit]->read_cb(unit, NB_STA_PDMA_CHx_DESC_POP_IDX(channel), pop_idx,
                                             sizeof(uint32_t));
    return 0;
}

static int
nb_dma_channel_reset(uint32_t unit, uint32_t channel)
{
    uint32_t reset;
    clx_misc_dev->clx_pci_dev[unit]->read_cb(unit, NB_CFW_PDMA_CH_RESET, &reset, sizeof(uint32_t));
    NB_CLR_BITMAP(reset, 1 << channel);
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, NB_CFW_PDMA_CH_RESET, &reset, sizeof(uint32_t));

    NB_SET_BITMAP(reset, 1 << channel);
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, NB_CFW_PDMA_CH_RESET, &reset, sizeof(uint32_t));

    nb_dma_channel_work_idx_set(unit, channel, 0);
    return 0;
}

static int
nb_dma_channel_restart(uint32_t unit, uint32_t channel)
{
    uint32_t restart = 0;
    NB_SET_BITMAP(restart, 1 << channel);
    clx_misc_dev->clx_pci_dev[unit]->write_cb(unit, NB_CFW_PDMA_CH_RESTART, &restart,
                                              sizeof(uint32_t));

    return 0;
}

/* intrrupt exist exclude dma rx tx interrupt */
static bool
nb_chip_usr_intrrupt_exist(uint32_t unit)
{
    uint64_t intr_status = 0x0;
    uint64_t status_mask = ~(0xFFULL << NB_CHAIN_INTR_SIZE);

    clx_misc_dev->clx_pci_dev[unit]->read_cb(unit, NB_TOP_STA_TOP_LVL_INTR_RAW,
                                             (uint32_t *)&intr_status, sizeof(uint64_t));
    dbg_print(DBG_INTR, "intr_status:0x%016llx, size:%lu\n", intr_status, sizeof(uint64_t));

    intr_status &= status_mask;
    return (intr_status != 0);
}

static int
nb_dma_rx_buffer_alloc(uint32_t unit, uint32_t channel, uint32_t desc_idx)
{
    clx_dma_channel_t *ptr_channel = &clx_dma_drv(unit)->dma_channel[channel];
    volatile nb_descriptor_t *ptr_descriptor = descriptor(unit, channel, desc_idx);
    struct device *ptr_dev = &clx_misc_dev->clx_pci_dev[unit]->pci_dev->dev;
    struct sk_buff *ptr_skb = NULL;
    dma_addr_t dma_addr;

    ptr_skb = dev_alloc_skb(clx_dma_drv(unit)->descriptor_size + NET_IP_ALIGN);
    if (NULL == ptr_skb) {
        dbg_print(DBG_CRIT, "Failed to allocate ptr_skb, pop:%d\n", desc_idx);
        return -ENOMEM;
    }
    /* reserve 2-bytes to alignment Ip header */
    skb_reserve(ptr_skb, NET_IP_ALIGN);
    skb_put(ptr_skb, clx_dma_drv(unit)->descriptor_size);

    dma_addr = dma_map_single(ptr_dev, ptr_skb->data, ptr_skb->len, DMA_FROM_DEVICE);
    if (dma_mapping_error(ptr_dev, dma_addr)) {
        dev_kfree_skb_any(ptr_skb);
        clx_dma_drv(unit)->dma_channel[channel].cnt.no_memory++;
        dbg_print(DBG_CRIT, "Failed to map ptr_skb\n");
        return -ENOMEM;
    }

    dbg_print(
        DBG_DEBUG,
        "pop_idx:%d, dma_addr:0x%llx, cpu_addr:0x%lx, ptr_skb:0x%lx, ptr_descriptor:0x%lx, len:%u\n",
        desc_idx, dma_addr, (unsigned long)ptr_skb->data, (unsigned long)ptr_skb,
        (unsigned long)ptr_descriptor, ptr_skb->len);
    ptr_descriptor->d_addr_hi = clx_addr_64_hi(dma_addr);
    ptr_descriptor->d_addr_lo = clx_addr_64_low(dma_addr);
    ptr_descriptor->interrupt = 0;
    ptr_descriptor->sinc = 0;
    ptr_descriptor->dinc = 1;
    ptr_descriptor->size = clx_dma_drv(unit)->descriptor_size;

    // save the ptr_skb
    ptr_channel->pptr_skb[desc_idx] = ptr_skb;

    return 0;
}

static int
nb_dma_rx_buffer_free(uint32_t unit, uint32_t channel, uint32_t desc_idx)
{
    clx_dma_channel_t *ptr_channel = &clx_dma_drv(unit)->dma_channel[channel];
    struct sk_buff *ptr_skb = ptr_channel->pptr_skb[desc_idx];
    dev_kfree_skb_any(ptr_skb);
    ptr_channel->pptr_skb[desc_idx] = NULL;

    return 0;
}

static int
nb_dma_alloc_ringbase(uint32_t unit, uint32_t channel, dma_addr_t *dma_addr)
{
    clx_dma_channel_t *ptr_channel = &clx_dma_drv(unit)->dma_channel[channel];
    struct device *ptr_dev = &clx_misc_dev->clx_pci_dev[unit]->pci_dev->dev;

    clx_dma_drv(unit)->dma_channel[channel].sw_ring_base = dma_alloc_coherent(
        ptr_dev, clx_dma_drv(unit)->ring_size * sizeof(nb_descriptor_t), dma_addr, GFP_ATOMIC);
    if (NULL == ptr_channel->sw_ring_base) {
        dbg_print(DBG_ERR, "No mem.\n");
        return -ENOMEM;
    }
    dbg_print(DBG_TX, "channel:%d sw_ring_base:0x%llx, hw_ring_base:0x%llx\n", channel,
              (clx_addr_t)clx_dma_drv(unit)->dma_channel[channel].sw_ring_base, *dma_addr);

    return 0;
}

static int
nb_dma_free_ringbase(uint32_t unit, uint32_t channel, dma_addr_t dma_addr)
{
    struct device *ptr_dev = &clx_misc_dev->clx_pci_dev[unit]->pci_dev->dev;
    dma_free_coherent(ptr_dev, clx_dma_drv(unit)->ring_size * sizeof(nb_descriptor_t),
                      clx_dma_drv(unit)->dma_channel[channel].sw_ring_base, dma_addr);

    return 0;
}

static int
nb_dma_alloc_rx_frag(uint32_t unit,
                     uint32_t channel,
                     uint32_t pop_idx,
                     struct dma_rx_packet *rx_packet)
{
    struct device *ptr_dev = &clx_misc_dev->clx_pci_dev[unit]->pci_dev->dev;
    clx_dma_channel_t *ptr_channel = &clx_dma_drv(unit)->dma_channel[channel];
    volatile nb_descriptor_t *ptr_descriptor = descriptor(unit, channel, pop_idx);
    struct dma_rx_frag_buffer *rx_frag;

    dbg_print(
        DBG_RX,
        "channel:%d , pop_idx:%d, ptr_descriptor:0x%llx, ptr_descriptor->interrupt:%d, sop:%d, eop:%d\n",
        channel, pop_idx, (clx_addr_t)ptr_descriptor, ptr_descriptor->interrupt,
        ptr_descriptor->sop, ptr_descriptor->eop);

    if (ptr_descriptor->interrupt == 0) {
        dbg_print(DBG_RX, "[debug] rx channel:%d pop_idx:%d, descriptor->interrupt:%d\n", channel,
                  pop_idx, ptr_descriptor->interrupt);
        return DMA_RX_NO_NEW_PACKET;
    }
    rx_frag = (struct dma_rx_frag_buffer *)kmalloc(sizeof(struct dma_rx_frag_buffer), GFP_ATOMIC);
    if (!rx_frag) {
        dbg_print(DBG_CRIT, "Failed to allocate packet context\n");
        return -ENOMEM;
    }

    rx_frag->ptr_skb = ptr_channel->pptr_skb[pop_idx];
    rx_frag->dma_addr = clx_addr_32_to_64(ptr_descriptor->d_addr_hi, ptr_descriptor->d_addr_lo);
    dma_unmap_single(ptr_dev, rx_frag->dma_addr, rx_frag->ptr_skb->len, DMA_FROM_DEVICE);
    rx_frag->ptr_skb->len = ptr_descriptor->size;
    list_add_tail(&rx_frag->rx_frag, &rx_packet->rx_frag);
    rx_packet->list_count++;
    rx_packet->packet_len += ptr_descriptor->size;

    if (ptr_descriptor->eop == 1) {
        rx_packet->rx_complete = 1;
        /* update rx counter */
        clx_dma_drv(unit)->dma_channel[channel].cnt.packets++;
        clx_dma_drv(unit)->dma_channel[channel].cnt.bytes += rx_packet->packet_len;

        return DMA_RX_COMPLETE_PACKET;
    }

    return DMA_RX_INCOMPLETE_PACKET;
}

static void
nb_print_pph(uint32_t loglvl, nb_pkt_pph_l2_t *ptr_pph_l2)
{
    nb_pkt_pph_l3uc_t *ptr_pph_l3 = NULL;
    nb_pkt_pph_l25_t *ptr_pph_l25 = NULL;

    if (!(loglvl & verbosity & DBG_RX_PAYLOAD) && !(loglvl & verbosity & DBG_TX_PAYLOAD)) {
        return;
    }

    /*print common field*/
    printk(" ========================== %s PPH  ====================== \n",
           (loglvl == DBG_RX_PAYLOAD) ? "Rx" : "Tx");
    printk("fwd_op                      :%u\n", ptr_pph_l2->fwd_op);
    printk("tc                          :%u\n", ptr_pph_l2->tc);
    printk("color                       :%u\n", ptr_pph_l2->color);
    printk("hash_val                    :0x%x\n", ptr_pph_l2->hash_val);
    printk("dst_idx                     :0x%x\n", nb_pkt_pph_get_dst_idx(ptr_pph_l2));
    printk("src_idx                     :0x%x\n", ptr_pph_l2->src_idx);
    printk("skip_epp                    :%u\n", ptr_pph_l2->skip_epp);
    printk("igr_acl_label               :0x%x\n", nb_pkt_pph_get_igr_acl_lable(ptr_pph_l2));
    printk("qos_dnt_modify              :%u\n", ptr_pph_l2->qos_dnt_modify);
    printk("qos_tnl_uniform             :%u\n", ptr_pph_l2->qos_tnl_uniform);
    printk("qos_pcp_dei_val             :%u\n", ptr_pph_l2->qos_pcp_dei_val);
    printk("pkt_journal                 :%u\n", ptr_pph_l2->pkt_journal);
    printk("port_num                    :0x%x\n", ptr_pph_l2->port_num);
    printk("die_id                      :%u\n", ptr_pph_l2->die_id);
    printk("slice_id                    :%u\n", ptr_pph_l2->slice_id);
    printk("skip_ipp                    :%u\n", ptr_pph_l2->skip_ipp);
    printk("mirror_bmap                 :%u\n", ptr_pph_l2->mirror_bmap);
    printk("cpu_reason                  :%u\n", ptr_pph_l2->cpu_reason);
    printk("src_bdi                     :0x%x\n", ptr_pph_l2->src_bdi);
    printk("decap_act                   :%u\n", ptr_pph_l2->decap_act);
    printk("igr_is_fab                  :%u\n", ptr_pph_l2->igr_is_fab);

    switch (ptr_pph_l2->fwd_op) {
        case NB_PKT_PPH_TYPE_L2:
            printk("evpn_esi                    :0x%x\n", nb_pkt_pph_get_evpn_esi(ptr_pph_l2));
            printk("mpls_pwcw_vld               :%u\n", ptr_pph_l2->mpls_pwcw_vld);
            printk("tnl_idx                     :0x%x\n", ptr_pph_l2->tnl_idx);
            printk("tnl_bd                      :0x%x\n", nb_pkt_pph_get_tnl_bd(ptr_pph_l2));
            printk("mpls_ctl                    :%u\n", ptr_pph_l2->mpls_ctl);
            printk("ecn_enable                  :%u\n", ptr_pph_l2->ecn_enable);
            printk("ecn                         :%u\n", ptr_pph_l2->ecn);
            printk("igr_vid_pop_num             :%u\n", ptr_pph_l2->igr_vid_pop_num);
            printk("pvlan_port_type             :%u\n", ptr_pph_l2->pvlan_port_type);
            printk("src_vlan                    :0x%x\n", ptr_pph_l2->src_vlan);
            printk("tapping_push_o              :%u\n", ptr_pph_l2->tapping_push_o);
            printk("tapping_push_t              :%u\n", ptr_pph_l2->tapping_push_t);
            printk("mac_learn_en                :%u\n", ptr_pph_l2->mac_learn_en);
            printk("ptp_info                    :%u\n", nb_pkt_pph_get_ptp_info(ptr_pph_l2));
            printk("int_mm_mode                 :%u\n", ptr_pph_l2->int_mm_mode);
            printk("int_profile                 :%u\n", ptr_pph_l2->int_profile);
            printk("int_role                    :%u\n", ptr_pph_l2->int_role);
            printk("timestamp                   :0x%x\n", ptr_pph_l2->timestamp);
            break;
        case NB_PKT_PPH_TYPE_L3UC:
            ptr_pph_l3 = (nb_pkt_pph_l3uc_t *)ptr_pph_l2;
            printk("evpn_esi                    :%u\n", nb_pkt_pph_get_evpn_esi(ptr_pph_l3));
            printk("mpls_pwcw_vld               :%u\n", ptr_pph_l3->mpls_pwcw_vld);
            printk("tnl_idx                     :%u\n", ptr_pph_l3->tnl_idx);
            printk("tnl_bd                      :%u\n", nb_pkt_pph_get_tnl_bd(ptr_pph_l3));
            printk("mpls_ctl                    :%u\n", ptr_pph_l3->mpls_ctl);
            printk("ecn_enable                  :%u\n", ptr_pph_l3->ecn_enable);
            printk("ecn                         :%u\n", ptr_pph_l3->ecn);
            printk("decr_ttl                    :%u\n", ptr_pph_l3->decr_ttl);
            printk("decap_prop_ttl              :%u\n", ptr_pph_l3->decap_prop_ttl);
            printk("mpls_inner_l2               :%u\n", ptr_pph_l3->mpls_inner_l2);
            printk("dst_bdi                     :%u\n", ptr_pph_l3->dst_bdi);
            printk("mac_da                      :0x%llx", nb_pkt_pph_get_mac_da(ptr_pph_l3));
            printk("nxt_sid_opcode              :%u\n", ptr_pph_l3->nxt_sid_opcode);
            printk("decr_sl                     :%u\n", ptr_pph_l3->decr_sl);
            printk("usid_func_en                :%u\n", ptr_pph_l3->usid_func_en);
            printk("usid_arg_en                 :%u\n", ptr_pph_l3->usid_arg_en);
            printk("srv6_encaps_red             :%u\n", ptr_pph_l3->srv6_encaps_red);
            printk("srv6_insert_red             :%u\n", ptr_pph_l3->srv6_insert_red);
            printk("srv6_func_hit               :%u\n", ptr_pph_l3->srv6_func_hit);
            printk("mac_learn_en                :%u\n", ptr_pph_l3->mac_learn_en);
            printk("srv6_encap_end              :%u\n", ptr_pph_l3->srv6_encap_end);
            printk("ptp_info                    :%u\n", nb_pkt_pph_get_ptp_info(ptr_pph_l3));
            printk("int_mm_mode                 :%u\n", ptr_pph_l3->int_mm_mode);
            printk("int_profile                 :%u\n", ptr_pph_l3->int_profile);
            printk("int_role                    :%u\n", ptr_pph_l3->int_role);
            printk("timestamp                   :0x%x\n", ptr_pph_l3->timestamp);
            break;
        case NB_PKT_PPH_TYPE_L25:
            ptr_pph_l25 = (nb_pkt_pph_l25_t *)ptr_pph_l2;
            printk("mpls_lbl                    :%u\n", nb_pkt_pph_get_mpls_lbl(ptr_pph_l25));
            printk("is_swap                     :%u\n", ptr_pph_l25->is_swap);
            printk("tnl_idx                     :%u\n", ptr_pph_l25->tnl_idx);
            printk("tnl_bd                      :%u\n", nb_pkt_pph_get_tnl_bd(ptr_pph_l25));
            printk("decr_ttl                    :%u\n", ptr_pph_l25->decr_ttl);
            printk("decap_prop_ttl              :%u\n", ptr_pph_l25->decap_prop_ttl);
            printk("mac_learn_en                :%u\n", ptr_pph_l25->mac_learn_en);
            printk("timestamp                   :0x%x\n", ptr_pph_l25->timestamp);
            break;
        default:
            printk("pph type is not valid!!!");
    }
}

static int
nb_pph_prepare(uint32_t unit, uint32_t port_di, uint8_t tc, void *pph)
{
    nb_pkt_pph_l2_t *ptr_pph = (nb_pkt_pph_l2_t *)((uint8_t *)pph + NB_PKT_EMAC_SZ);

    /* fill up pp header */
    ptr_pph->skip_ipp = 1;
    ptr_pph->skip_epp = 1;
    ptr_pph->color = 0; /* Green */
    ptr_pph->tc = tc & 0x7;
    ptr_pph->src_idx = clx_netif_drv(unit)->cpu_port;

    nb_pkt_pph_set_dst_idx(ptr_pph, port_di);

    // TODO: fill up pph other fields

    return 0;
}

static void
nb_adjust_pph(uint32_t unit, void *ptr_pph)
{
    uint32_t i = 0;

    nb_print_pph(DBG_TX_PAYLOAD, ptr_pph);

    while (i < NB_PKT_PPH_HDR_SZ / 4) {
        *((uint32_t *)ptr_pph + i) = NB_PKT_HOST_TO_BE32(*((uint32_t *)ptr_pph + i));
        i++;
    }
}

static int
nb_tx_callback(uint32_t unit, uint32_t channel)
{
    clx_dma_channel_t *ptr_channel = &clx_dma_drv(unit)->dma_channel[channel];
    struct device *ptr_dev = &clx_misc_dev->clx_pci_dev[unit]->pci_dev->dev;
    struct sk_buff *ptr_skb;
    volatile nb_descriptor_t *ptr_descriptor;
    dma_addr_t dma_addr = 0;

    while (ptr_channel->pop_idx != ptr_channel->work_idx) {
        ptr_descriptor = descriptor(unit, channel, ptr_channel->pop_idx);
        dbg_print(DBG_TX, "channel:%d work_idx:%d, pop_idx:%d, interrupt:%d, error:%d\n", channel,
                  ptr_channel->work_idx, ptr_channel->pop_idx, ptr_descriptor->interrupt,
                  ptr_descriptor->err);
        if (ptr_descriptor->interrupt != 1) {
            break;
        }
        ptr_descriptor->interrupt = 0;
        dma_addr = clx_addr_32_to_64(ptr_descriptor->s_addr_hi, ptr_descriptor->s_addr_lo);
        ptr_skb = ptr_channel->pptr_skb[ptr_channel->pop_idx];
        dbg_print(DBG_TX, "[debug] channel:%d, dma_addr:0x%llx, len:%d, skb:0x%llx.\n", channel,
                  dma_addr, ptr_skb->len, (clx_addr_t)ptr_channel->pptr_skb[ptr_channel->pop_idx]);

        /* update tx counter */
        ptr_channel->cnt.packets++;
        ptr_channel->cnt.bytes += ptr_skb->len;

        dma_unmap_single(ptr_dev, dma_addr, ptr_skb->len, DMA_TO_DEVICE);
        dev_kfree_skb_any(ptr_skb);
        ptr_channel->pop_idx++;
        ptr_channel->pop_idx %= clx_dma_drv(unit)->ring_size;
    }

    return 0;
}

static int
nb_netdev_packet_tx(uint32_t unit, uint32_t channel, struct sk_buff *ptr_skb)
{
    clx_dma_channel_t *ptr_channel = &clx_dma_drv(unit)->dma_channel[channel];
    volatile nb_descriptor_t *ptr_descriptor;
    struct device *ptr_dev = &clx_misc_dev->clx_pci_dev[unit]->pci_dev->dev;
    dma_addr_t dma_addr = 0x0;
    unsigned long flags = 0;

    spin_lock_irqsave(&ptr_channel->lock, flags);
    if ((ptr_channel->work_idx + 1) % clx_dma_drv(unit)->ring_size == ptr_channel->pop_idx) {
        dbg_print(DBG_TX, "No available descriptor!\n");
        spin_unlock_irqrestore(&ptr_channel->lock, flags);
        return -EBUSY;
    }

    nb_adjust_pph(unit, ptr_skb->data + NB_PKT_EMAC_SZ);

    dma_addr = dma_map_single(ptr_dev, ptr_skb->data, ptr_skb->len, DMA_TO_DEVICE);
    if (dma_mapping_error(ptr_dev, dma_addr)) {
        dbg_print(DBG_ERR, "u=%u, txch=%u, skb dma map err\n", unit, channel);
        clx_dma_drv(unit)->dma_channel[channel].cnt.no_memory++;
        spin_unlock_irqrestore(&ptr_channel->lock, flags);
        return -EFAULT;
    }

    if (!IS_ALIGNED(dma_addr, 4)) {
        dbg_print(DBG_ERR, "dma_addr is not 4bytes-aligned.\n");
        dma_unmap_single(ptr_dev, dma_addr, ptr_skb->len, DMA_TO_DEVICE);
        spin_unlock_irqrestore(&ptr_channel->lock, flags);
        return -EFAULT;
    }
    dma_sync_single_for_device(ptr_dev, dma_addr, ptr_skb->len, DMA_TO_DEVICE);

    ptr_descriptor = descriptor(unit, channel, ptr_channel->work_idx);
    ptr_descriptor->s_addr_hi = clx_addr_64_hi(dma_addr);
    ptr_descriptor->s_addr_lo = clx_addr_64_low(dma_addr);
    ptr_descriptor->size = ptr_skb->len;
    ptr_descriptor->sinc = 1;
    ptr_descriptor->interrupt = 0;
    ptr_descriptor->sop = 1;
    ptr_descriptor->eop = 1;

    ptr_channel->pptr_skb[ptr_channel->work_idx] = ptr_skb;
    ptr_channel->work_idx += 1;
    ptr_channel->work_idx %= clx_dma_drv(unit)->ring_size;
    nb_dma_channel_work_idx_set(unit, channel, ptr_channel->work_idx);
    dbg_print(DBG_TX, "[debug] channel:%d work_idx:%d, pop_idx:%d,dma_addr:0x%llx, len:%d.\n",
              channel, ptr_channel->work_idx, ptr_channel->pop_idx, dma_addr, ptr_skb->len);
    spin_unlock_irqrestore(&ptr_channel->lock, flags);

    return 0;
}

static void
nb_dma_general_tasklet_func(unsigned long data)
{
    clx_dma_channel_cookie_t channel_cookie = *(clx_dma_channel_cookie_t *)data;
    dbg_print(DBG_DEBUG, "nb_dma_general_tasklet_func unit:%d, channel:%d\n", channel_cookie.unit,
              channel_cookie.channel);
}

static int
nb_pkt_dst_get(const uint32_t unit,
               struct dma_rx_packet *rx_packet,
               uint32_t *port_di,
               uint32_t *reason)
{
    struct dma_rx_frag_buffer *rx_frag;
    void *ptr_dma_buffer;
    nb_pkt_pph_l2_t *ptr_pph = NULL;
    int i = 0;
    unsigned char *ptr_pkt_dmac = NULL;

    rx_frag = list_first_entry(&rx_packet->rx_frag, struct dma_rx_frag_buffer, rx_frag);
    ptr_dma_buffer = rx_frag->ptr_skb->data;
    ptr_pph = (nb_pkt_pph_l2_t *)((uint8_t *)ptr_dma_buffer + NB_PKT_EMAC_SZ);

    while (i < NB_PKT_PPH_HDR_SZ / 4) {
        *((uint32_t *)ptr_pph + i) = NB_PKT_BE_TO_HOST32(*((uint32_t *)ptr_pph + i));
        i++;
    }

    nb_print_pph(DBG_RX_PAYLOAD, ptr_pph);

    /* mod mac*/
    ptr_pkt_dmac = (unsigned char *)(ptr_pph + 1);
    if (0 == memcmp(g_mod_default_mac, ptr_pkt_dmac, 6)) {
        /* change dmac */
        if (clx_netif_drv(unit)->enable_mod_dmac) {
            memcpy(ptr_pkt_dmac, clx_netif_drv(unit)->mod_dmac, 6);
        }

        ptr_pph->cpu_reason = NB_PKT_RX_MOD_HW_REASON;

        dbg_print(
            DBG_RX,
            "u=%u set mod reason and enable_mod_dmac:%d pkt_dmac:%02x-%02x-%02x-%02x-%02x-%02x\n",
            unit, clx_netif_drv(unit)->enable_mod_dmac, ptr_pkt_dmac[0], ptr_pkt_dmac[1],
            ptr_pkt_dmac[2], ptr_pkt_dmac[3], ptr_pkt_dmac[4], ptr_pkt_dmac[5]);
    }

    /* vlan */
    if (NB_PKT_PPH_TYPE_L2 == ptr_pph->fwd_op) {
        rx_packet->pph_info.vlan_tag = ptr_pph->src_vlan;
        rx_packet->pph_info.vlan_pop_num = ptr_pph->igr_vid_pop_num;
    } else {
        rx_packet->pph_info.vlan_tag = ptr_pph->src_bdi;
        rx_packet->pph_info.vlan_pop_num = 1;
    }

    *port_di = ptr_pph->src_idx;
    if (*port_di > CLX_NETIF_PORT_DI_MAX_NUM) {
        *port_di = CLX_NETIF_GET_PORT_DI(unit, ptr_pph->slice_id, ptr_pph->port_num);
        if (-1 == *port_di) {
            dbg_print(DBG_RX, "port_di is invlaid!!!,slice_id:%u port_num:%u\n", ptr_pph->slice_id,
                      ptr_pph->port_num);
            return -1;
        }
        dbg_print(DBG_RX, "get port_di:%u slice_id:%u port_num:%u\n", *port_di, ptr_pph->slice_id,
                  ptr_pph->port_num);
    }

    rx_packet->pph_info.cpu_reason = ptr_pph->cpu_reason;
    *reason = ptr_pph->cpu_reason;

    // update cpu reason counter
    clx_netif_drv(unit)->cpu_reason_cnt[*reason].pkt_cnts++;
    clx_netif_drv(unit)->cpu_reason_cnt[*reason].byte_cnts +=
        rx_packet->packet_len - clx_dma_drv(unit)->dma_hdr_size;

    return 0;
}

static int
nb_parse_netlink_info(const uint32_t unit, struct dma_rx_packet *rx_packet, void *ptr_cookie)
{
    struct dma_rx_frag_buffer *rx_frag;
    void *ptr_dma_buffer;
    nb_pkt_pph_l2_t *ptr_pph = NULL;
    struct netlink_rx_cookie *netlink_cookie = (struct netlink_rx_cookie *)ptr_cookie;
    struct net_device *ptr_igr_net_dev = NULL, *ptr_egr_net_dev = NULL;
    uint32_t port_di = 0;
    uint32_t netif_id = 0;

    if (!rx_packet || !ptr_cookie || list_empty(&rx_packet->rx_frag)) {
        dbg_print(DBG_ERR, "rx_packet or ptr_cookie is NULL or rx_frag is empty\n");
        return -EINVAL;
    }

    rx_frag = list_first_entry(&rx_packet->rx_frag, struct dma_rx_frag_buffer, rx_frag);
    if (!rx_frag || !rx_frag->ptr_skb || !rx_frag->ptr_skb->data) {
        dbg_print(DBG_ERR, "rx_frag or rx_frag->ptr_skb or rx_frag->ptr_skb->data is NULL\n");
        return -EINVAL;
    }

    ptr_dma_buffer = rx_frag->ptr_skb->data;
    ptr_pph = (nb_pkt_pph_l2_t *)((uint8_t *)ptr_dma_buffer + NB_PKT_EMAC_SZ);

    // obtain the igress netdev ifindex
    if (ptr_pph->src_idx >= CLX_NETIF_PORT_DI_MAX_NUM) {
        if (ptr_pph->slice_id >= clx_netif_drv(unit)->slices_per_unit ||
            ptr_pph->port_num >= clx_netif_drv(unit)->ports_per_slice) {
            dbg_print(DBG_RX, "Invalid slice_id=%u or port_num=%u (max_slice=%u, max_port=%u)\n",
                      ptr_pph->slice_id, ptr_pph->port_num,
                      clx_netif_drv(unit)->slices_per_unit - 1,
                      clx_netif_drv(unit)->ports_per_slice - 1);
            return -EFAULT;
        }

        netlink_cookie->pkt.igr_port_si =
            CLX_NETIF_GET_PORT_DI(unit, ptr_pph->slice_id, ptr_pph->port_num);
        if ((uint32_t)-1 == netlink_cookie->pkt.igr_port_si) {
            return -EFAULT;
        }
    } else {
        netlink_cookie->pkt.igr_port_si = ptr_pph->src_idx;
    }
    netif_id = clx_netif_di2id_lookup(unit, netlink_cookie->pkt.igr_port_si);
    ptr_igr_net_dev = clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev;
    netlink_cookie->pkt.iifindex = ptr_igr_net_dev->ifindex;

    // obtain the egress netdev ifindex
    port_di = nb_pkt_pph_get_dst_idx(ptr_pph);
    if (port_di >= CLX_NETIF_PORT_DI_MAX_NUM) {
        // set the egress netdev ifindex to the igress netdev ifindex when the egress netdev is not
        // found
        netlink_cookie->pkt.eifindex = netlink_cookie->pkt.iifindex;
    } else {
        netif_id = clx_netif_di2id_lookup(unit, port_di);
        if (netif_id == -1) {
            // set the egress netdev ifindex to the igress netdev ifindex when the egress netdev is
            // not found
            netlink_cookie->pkt.eifindex = netlink_cookie->pkt.iifindex;
        } else {
            ptr_egr_net_dev = clx_netif_drv(unit)->netif_db[netif_id].ptr_net_dev;
            netlink_cookie->pkt.eifindex = ptr_egr_net_dev->ifindex;
        }
    }

    if (ptr_pph->cpu_reason == NB_PKT_RX_IGR_SFLOW_SAMPLER) {
        netlink_cookie->pkt.psample_dir = NETIF_NL_PKT_PSAMPLE_INGRESS;
        netlink_cookie->netlink_type = NETLINK_RX_TYPE_SFLOW;
    } else if (ptr_pph->cpu_reason == NB_PKT_RX_EGR_SFLOW_SAMPLER) {
        netlink_cookie->pkt.psample_dir = NETIF_NL_PKT_PSAMPLE_EGRESS;
        netlink_cookie->netlink_type = NETLINK_RX_TYPE_SFLOW;
    } else if (ptr_pph->cpu_reason == NB_PKT_RX_MOD_REASON) {
        netlink_cookie->netlink_type = NETLINK_RX_TYPE_MOD;
    } else {
        netlink_cookie->netlink_type = NETLINK_RX_TYPE_OTHER;
    }
    dbg_print(DBG_RX,
              "unit:%d, netlink_type:%d, psample_dir:%d, "
              "iifindex:%d, eifindex:%d, igr_port_si:%d, port dst_idx:%d, cpu_reason:%d\n",
              unit, netlink_cookie->netlink_type, netlink_cookie->pkt.psample_dir,
              netlink_cookie->pkt.iifindex, netlink_cookie->pkt.eifindex,
              netlink_cookie->pkt.igr_port_si, port_di, ptr_pph->cpu_reason);

    return 0;
}

static int
nb_register_msi_irq(uint32_t unit, uint32_t irq)
{
    uint32_t idx = 0;
    clx_dma_channel_cookie_t *dma_cookie;
    clx_intr_info_t *usr_intr_cookie;
    int rc;

    /* fill in the msi_cookie */
    for (idx = 0; idx < clx_intr_drv(unit)->msi_cnt; idx++) {
        if (idx < clx_dma_drv(unit)->rx_channel_num + clx_dma_drv(unit)->tx_channel_num) {
            dma_cookie = kmalloc(sizeof(clx_dma_channel_cookie_t), GFP_ATOMIC);
            if (!dma_cookie) {
                dbg_print(DBG_CRIT, "Failed to allocate dma cookie\n");
                return -ENOMEM;
            }
            dma_cookie->unit = unit;
            dma_cookie->channel = idx;
            clx_intr_drv(unit)->msi_vector[idx].msi_cookie = dma_cookie;

            tasklet_init(&clx_dma_drv(unit)->clx_dma_intr[idx].dma_tasklets,
                         clx_dma_drv(unit)->clx_dma_intr[idx].dma_handler,
                         (unsigned long)&clx_dma_drv(unit)->clx_dma_intr[idx].channel_cookie);
        } else {
            usr_intr_cookie = kmalloc(sizeof(clx_intr_info_t), GFP_ATOMIC);
            if (!usr_intr_cookie) {
                dbg_print(DBG_CRIT, "Failed to allocate interrupt cookie\n");
                return -ENOMEM;
            }
            usr_intr_cookie->unit = unit;
            usr_intr_cookie->irq = idx;
            usr_intr_cookie->valid = CLX_INTR_VALID_CODE;
            clx_intr_drv(unit)->msi_vector[idx].msi_cookie = usr_intr_cookie;
        }

        dbg_print(DBG_INTR, "request_irq. unit=%d, base irq=%d irq=%d.\n", unit, irq, irq + idx);
        rc = request_irq(irq + idx, clx_intr_drv(unit)->msi_vector[idx].handler, 0, CLX_DRIVER_NAME,
                         clx_intr_drv(unit)->msi_vector[idx].msi_cookie);
        if (0 != rc) {
            dbg_print(DBG_CRIT, "request_irq failed. unit=%d, irq=%d.\n", unit, irq + idx);
            return rc;
        }
    }

    return 0;
}

clx_pci_drv_cb_t nb_pci_driver = {
    .dma_bit_mask = 48,
    .mmio_bar = 2,
};

clx_netif_drv_cb_t nb_pkt_driver = {
    .mtu = NB_MAX_PKT_SIZE,
    .cpu_port = 256,
    .unit_num = 1,
    .slices_per_unit = 8,
    .ports_per_slice = 40,
    .ports_num_unit = 320,
    .get_pkt_dst = nb_pkt_dst_get,
    .parse_netlink_info = nb_parse_netlink_info,
    .ptr_port_map_db = NULL,
};

clx_dma_drv_cb_t nb_dma_driver = {
    .channel_num = 20,
    .rx_channel_num = 4,
    .tx_channel_num = 4,
    .ring_size = CLX_PKT_DMA_RING_SIZE,
    .descriptor_size = CLX_PKT_DMA_FRAG_SIZE,
    .dma_hdr_size = NB_PKT_PDMA_HDR_SZ,

    .enable_channel = nb_dma_channel_enable,
    .disable_channel = nb_dma_channel_disable,
    .reset_channel = nb_dma_channel_reset,
    .restart_channel = nb_dma_channel_restart,
    .set_ring_base = nb_dma_channel_ring_base_set,
    .get_ring_base = nb_dma_channel_ring_base_get,
    .set_ring_size = nb_dma_channel_ring_size_set,
    .get_ring_size = nb_dma_channel_ring_size_get,
    .set_work_idx = nb_dma_channel_work_idx_set,
    .get_work_idx = nb_dma_channel_work_idx_get,
    .get_pop_idx = nb_dma_channel_pop_idx_get,
    .alloc_ring_base = nb_dma_alloc_ringbase,
    .free_ring_base = nb_dma_free_ringbase,
    .alloc_rx_buffer = nb_dma_rx_buffer_alloc,
    .free_rx_buffer = nb_dma_rx_buffer_free,
    .alloc_rx_frag = nb_dma_alloc_rx_frag,
    .prepare_pph = nb_pph_prepare,
    .tx_packet = nb_netdev_packet_tx,
    .tx_callback = nb_tx_callback,
};

static msi_isr_vector_t nb_msi_vector[NUM_MSI_IRQ] = {
    /* rx dma channels */
    {.handler = dma_rx_msi_hanler, .msi_cookie = NULL},
    {.handler = dma_rx_msi_hanler, .msi_cookie = NULL},
    {.handler = dma_rx_msi_hanler, .msi_cookie = NULL},
    {.handler = dma_rx_msi_hanler, .msi_cookie = NULL},
    /* tx dma channels */
    {.handler = dma_tx_msi_hanler, .msi_cookie = NULL},
    {.handler = dma_tx_msi_hanler, .msi_cookie = NULL},
    {.handler = dma_tx_msi_hanler, .msi_cookie = NULL},
    {.handler = dma_tx_msi_hanler, .msi_cookie = NULL},
    /* general dma channels */
    {.handler = clx_msi_usr_handler, .msi_cookie = NULL},
    {.handler = clx_msi_usr_handler, .msi_cookie = NULL},
    {.handler = clx_msi_usr_handler, .msi_cookie = NULL},
    {.handler = clx_msi_usr_handler, .msi_cookie = NULL},
    {.handler = clx_msi_usr_handler, .msi_cookie = NULL},
    {.handler = clx_msi_usr_handler, .msi_cookie = NULL},
    {.handler = clx_msi_usr_handler, .msi_cookie = NULL},
    {.handler = clx_msi_usr_handler, .msi_cookie = NULL},
    /* fifo dma channel */
    {.handler = clx_msi_usr_handler, .msi_cookie = NULL},
    {.handler = clx_msi_usr_handler, .msi_cookie = NULL},
    /* ifmon interrupt */
    {.handler = clx_msi_usr_handler, .msi_cookie = NULL},
    {.handler = clx_msi_usr_handler, .msi_cookie = NULL},
    /* the other interrupt */
    {.handler = clx_msi_usr_handler, .msi_cookie = NULL},
};

clx_intr_drv_cb_t nb_intr_driver = {
    .intr_mode = INTR_MODE_MSI,
    .msi_cnt = sizeof(nb_msi_vector) / sizeof(msi_isr_vector_t),
    .msi_vector = (msi_isr_vector_t *)nb_msi_vector,
    .top_irq_mask = namchabar_top_irq_mask,
    .top_irq_unmask = namchabar_top_irq_unmask,
    .dma_irq_mask_all = namchabar_dma_irq_mask_all,
    .dma_irq_unmask_all = namchabar_dma_irq_unmask_all,
    .read_dma_irq_status = namchabar_get_dma_irq_channel,
    .mask_dma_channel_irq = nb_mask_dma_channel_irq,
    .unmask_dma_channel_irq = nb_unmask_dma_channel_irq,
    .clear_dma_channel_irq = nb_clear_dma_channel_irq,
    .read_dma_error_irq_status = namchabar_get_dma_error_irq_channel,
    .mask_dma_channel_error_irq = nb_mask_dma_channel_error_irq,
    .unmask_dma_channel_error_irq = nb_unmask_dma_channel_error_irq,
    .clear_dma_channel_error_irq = nb_dma_channel_error_irq_clear,
    .register_msi_irq = nb_register_msi_irq,
    .usr_interrupt_exist = nb_chip_usr_intrrupt_exist,
};

int
nb_init_dma_driver(uint32_t unit)
{
    uint32_t channel = 0;
    nb_dma_driver.clx_dma_intr = (clx_dma_intr_t *)kmalloc_array(
        nb_dma_driver.channel_num, sizeof(clx_dma_intr_t), GFP_ATOMIC);
    if (!nb_dma_driver.clx_dma_intr) {
        return -ENOMEM;
    }
    for (channel = 0; channel < nb_dma_driver.channel_num; channel++) {
        if (channel < 4) {
            /* Rx channel */
            nb_dma_driver.clx_dma_intr[channel].channel_cookie.channel = channel;
            nb_dma_driver.clx_dma_intr[channel].channel_cookie.unit = unit;
            nb_dma_driver.clx_dma_intr[channel].dma_handler = dma_rx_tasklet_func;
        } else if (channel < 8) {
            /* Tx channel */
            nb_dma_driver.clx_dma_intr[channel].channel_cookie.channel = channel;
            nb_dma_driver.clx_dma_intr[channel].channel_cookie.unit = unit;
            nb_dma_driver.clx_dma_intr[channel].dma_handler = dma_tx_tasklet_func;
        } else {
            nb_dma_driver.clx_dma_intr[channel].channel_cookie.channel = channel;
            nb_dma_driver.clx_dma_intr[channel].channel_cookie.unit = unit;
            nb_dma_driver.clx_dma_intr[channel].dma_handler = nb_dma_general_tasklet_func;
        }
    }

    return 0;
}

void
nb_cleanup_dma_driver(uint32_t unit)
{
    dbg_print(DBG_DEBUG, "[debug] nb_cleanup_dma_driver\n");
    kfree(nb_dma_driver.clx_dma_intr);
    dbg_print(DBG_DEBUG, "[debug] nb_cleanup_dma_driver done.\n");
}
