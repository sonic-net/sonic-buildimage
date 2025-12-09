/*! \file bcmcnet_cmicd_pdma_hw.c
 *
 * Utility routines for handling BCMCNET hardware (CMICd).
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

#include <bcmcnet/bcmcnet_core.h>
#include <bcmcnet/bcmcnet_dev.h>
#include <bcmcnet/bcmcnet_rxtx.h>
#include <bcmcnet/bcmcnet_cmicd.h>

/*!
 * Read 32-bit register
 */
static inline void
cmicd_pdma_reg_read32(struct pdma_hw *hw, uint32_t addr, uint32_t *data)
{
    if (hw->dev->dev_read32) {
        hw->dev->dev_read32(hw->dev, addr, data);
    } else {
        DEV_READ32(&hw->dev->ctrl, addr, data);
    }
}

/*!
 * Write 32-bit register
 */
static inline void
cmicd_pdma_reg_write32(struct pdma_hw *hw, uint32_t addr, uint32_t data)
{
    if (hw->dev->dev_write32) {
        hw->dev->dev_write32(hw->dev, addr, data);
    } else {
        DEV_WRITE32(&hw->dev->ctrl, addr, data);
    }
}

/*!
 * Enable interrupt for a channel
 */
static inline void
cmicd_pdma_intr_enable(struct pdma_hw *hw, int cmc, int chan, uint32_t mask)
{
    uint32_t reg = CMICD_IRQ_STAT(cmc);

    if (hw->dev->mode == DEV_MODE_UNET || hw->dev->mode == DEV_MODE_VNET) {
        hw->dev->intr_unmask(hw->dev, cmc, chan, 0, 0);
        return;
    }

    hw->dev->ctrl.grp[cmc].irq_mask |= mask;
    hw->dev->intr_unmask(hw->dev, cmc, chan, reg, 0);
}

/*!
 * Disable interrupt for a channel
 */
static inline void
cmicd_pdma_intr_disable(struct pdma_hw *hw, int cmc, int chan, uint32_t mask)
{
    uint32_t reg = CMICD_IRQ_STAT(cmc);

    if (hw->dev->mode == DEV_MODE_UNET || hw->dev->mode == DEV_MODE_VNET) {
        hw->dev->intr_mask(hw->dev, cmc, chan, 0, 0);
        return;
    }

    hw->dev->ctrl.grp[cmc].irq_mask &= ~mask;
    hw->dev->intr_mask(hw->dev, cmc, chan, reg, 0);
}

/*!
 * Initialize HW
 */
static int
cmicd_pdma_hw_init(struct pdma_hw *hw)
{
    dev_mode_t mode = DEV_MODE_MAX;
    uint32_t val;

    /* Temporarily upgrade work mode to get HW information in VNET mode. */
    if (hw->dev->mode == DEV_MODE_VNET) {
        mode = DEV_MODE_VNET;
        hw->dev->mode = DEV_MODE_UNET;
    }












    /* Release credits to EP. Only do this once when HW is initialized. */
    hw->hdls.reg_rd32(hw, CMICD_EPINTF_RELEASE_CREDITS, &val);
    if (!val) {
        hw->hdls.reg_wr32(hw, CMICD_EPINTF_RELEASE_CREDITS, 1);
    }

    hw->info.name = CMICD_DEV_NAME;
    hw->hdls.reg_rd32(hw, CMICD_CMICM_REV_ID, &val);
    hw->info.ver_no = val;
    hw->hdls.reg_rd32(hw, CMICD_DEV_REV_ID, &val);
    hw->info.dev_id = val & 0xffff;
    hw->info.rev_id = val >> 16;
    hw->info.num_cmcs = CMICD_PDMA_CMC_MAX;
    hw->info.cmc_chans = CMICD_PDMA_CMC_CHAN;
    hw->info.num_chans = CMICD_PDMA_CMC_MAX * CMICD_PDMA_CMC_CHAN;
    hw->info.rx_dcb_size = CMICD_PDMA_DCB_SIZE;
    hw->info.tx_dcb_size = CMICD_PDMA_DCB_SIZE;
    hw->info.rx_ph_size = 0;
    hw->info.tx_ph_size = 0;

    /* Restore work mode to VNET. */
    if (mode == DEV_MODE_VNET) {
        hw->dev->mode = DEV_MODE_VNET;
    }

    return SHR_E_NONE;
}

/*!
 * Configure HW
 */
static int
cmicd_pdma_hw_config(struct pdma_hw *hw)
{
    struct dev_ctrl *ctrl = &hw->dev->ctrl;
    struct pdma_rx_queue *rxq = NULL;
    struct pdma_tx_queue *txq = NULL;
    uint32_t val, que_ctrl;
    int grp, que;
    uint32_t qi;

    for (qi = 0; qi < ctrl->nb_rxq; qi++) {
        rxq = (struct pdma_rx_queue *)ctrl->rx_queue[qi];
        grp = rxq->group_id;
        que = rxq->chan_id % CMICD_PDMA_CMC_CHAN;
        que_ctrl = ctrl->grp[grp].que_ctrl[que];

        hw->hdls.reg_wr32(hw, CMICD_PDMA_STAT_CLR(grp), CMICD_PDMA_DESC_CMPLT(que));
        hw->hdls.reg_wr32(hw, CMICD_PDMA_STAT_CLR(grp), CMICD_PDMA_DESC_CNTLD(que));
        val = 0;
        if (que_ctrl & PDMA_PKT_BYTE_SWAP) {
            val |= CMICD_PDMA_PKT_BIG_ENDIAN;
        }
        if (que_ctrl & PDMA_OTH_BYTE_SWAP) {
            val |= CMICD_PDMA_DESC_BIG_ENDIAN;
        }
        if (!(hw->dev->flags & PDMA_CHAIN_MODE)) {
            val |= CMICD_PDMA_CONTINUOUS;
        }
        val |= CMICD_PDMA_CNTLD_INTR;
        val &= ~CMICD_PDMA_DIR;
        hw->hdls.reg_wr32(hw, CMICD_PDMA_CTRL(grp, que), val);
    }

    for (qi = 0; qi < ctrl->nb_txq; qi++) {
        txq = (struct pdma_tx_queue *)ctrl->tx_queue[qi];
        grp = txq->group_id;
        que = txq->chan_id % CMICD_PDMA_CMC_CHAN;
        que_ctrl = ctrl->grp[grp].que_ctrl[que];

        hw->hdls.reg_wr32(hw, CMICD_PDMA_STAT_CLR(grp), CMICD_PDMA_DESC_CMPLT(que));
        hw->hdls.reg_wr32(hw, CMICD_PDMA_STAT_CLR(grp), CMICD_PDMA_DESC_CNTLD(que));
        val = 0;
        if (que_ctrl & PDMA_PKT_BYTE_SWAP) {
            val |= CMICD_PDMA_PKT_BIG_ENDIAN;
        }
        if (que_ctrl & PDMA_OTH_BYTE_SWAP) {
            val |= CMICD_PDMA_DESC_BIG_ENDIAN;
        }
        if (!(hw->dev->flags & PDMA_CHAIN_MODE)) {
            val |= CMICD_PDMA_CONTINUOUS;
        }
        val |= CMICD_PDMA_CNTLD_INTR | CMICD_PDMA_DIR;
        hw->hdls.reg_wr32(hw, CMICD_PDMA_CTRL(grp, que), val);
    }

    return SHR_E_NONE;
}

/*!
 * Reset HW
 */
static int
cmicd_pdma_hw_reset(struct pdma_hw *hw)
{
    int gi, qi;

    for (gi = 0; gi < hw->dev->num_groups; gi++) {
        if (!hw->dev->ctrl.grp[gi].attached) {
            continue;
        }
        for (qi = 0; qi < CMICD_PDMA_CMC_CHAN; qi++) {
            if (1 << qi & hw->dev->ctrl.grp[gi].bm_rxq ||
                1 << qi & hw->dev->ctrl.grp[gi].bm_txq) {
                hw->hdls.reg_wr32(hw, CMICD_PDMA_CTRL(gi, qi), 0);
            }
        }
    }

    return SHR_E_NONE;
}

/*!
 * Start a channel
 */
static int
cmicd_pdma_chan_start(struct pdma_hw *hw, int chan)
{
    uint32_t val;
    int grp, que;

    grp = chan / CMICD_PDMA_CMC_CHAN;
    que = chan % CMICD_PDMA_CMC_CHAN;

    hw->hdls.reg_rd32(hw, CMICD_PDMA_CTRL(grp, que), &val);
    val |= CMICD_PDMA_ENABLE;
    hw->hdls.reg_wr32(hw, CMICD_PDMA_CTRL(grp, que), val);

    MEMORY_BARRIER;

    return SHR_E_NONE;
}

/*!
 * Stop a channel
 */
static int
cmicd_pdma_chan_stop(struct pdma_hw *hw, int chan)
{
    uint32_t val;
    int grp, que;
    int retry = CMICD_HW_RETRY_TIMES;

    grp = chan / CMICD_PDMA_CMC_CHAN;
    que = chan % CMICD_PDMA_CMC_CHAN;

    hw->hdls.reg_rd32(hw, CMICD_PDMA_CTRL(grp, que), &val);
    val |= CMICD_PDMA_ENABLE | CMICD_PDMA_ABORT;
    hw->hdls.reg_wr32(hw, CMICD_PDMA_CTRL(grp, que), val);

    MEMORY_BARRIER;

    do {
        val = ~CMICD_PDMA_ACTIVE(que);
        hw->hdls.reg_rd32(hw, CMICD_PDMA_STAT(grp), &val);
    } while ((val & CMICD_PDMA_ACTIVE(que)) && (--retry > 0));

    hw->hdls.reg_rd32(hw, CMICD_PDMA_CTRL(grp, que), &val);
    val &= ~(CMICD_PDMA_ENABLE | CMICD_PDMA_ABORT);
    hw->hdls.reg_wr32(hw, CMICD_PDMA_CTRL(grp, que), val);

    hw->hdls.reg_wr32(hw, CMICD_PDMA_STAT_CLR(grp), CMICD_PDMA_DESC_CNTLD(que));

    MEMORY_BARRIER;

    return SHR_E_NONE;
}

/*!
 * Setup a channel
 */
static int
cmicd_pdma_chan_setup(struct pdma_hw *hw, int chan, uint64_t addr)
{
    int grp, que;

    grp = chan / CMICD_PDMA_CMC_CHAN;
    que = chan % CMICD_PDMA_CMC_CHAN;

    hw->hdls.reg_wr32(hw, CMICD_PDMA_DESC(grp, que), addr);

    MEMORY_BARRIER;

    return SHR_E_NONE;
}

/*!
 * Set halt point for a channel
 */
static int
cmicd_pdma_chan_goto(struct pdma_hw *hw, int chan, uint64_t addr)
{
    int grp, que;

    grp = chan / CMICD_PDMA_CMC_CHAN;
    que = chan % CMICD_PDMA_CMC_CHAN;

    hw->hdls.reg_wr32(hw, CMICD_PDMA_DESC_HALT(grp, que), addr);

    MEMORY_BARRIER;

    return SHR_E_NONE;
}

/*!
 * Clear a channel
 */
static int
cmicd_pdma_chan_clear(struct pdma_hw *hw, int chan)
{
    int grp, que;

    grp = chan / CMICD_PDMA_CMC_CHAN;
    que = chan % CMICD_PDMA_CMC_CHAN;

    hw->hdls.reg_wr32(hw, CMICD_PDMA_STAT_CLR(grp), CMICD_PDMA_DESC_CNTLD(que));

    MEMORY_BARRIER;

    return SHR_E_NONE;
}

/*!
 * Get interrupt number for a channel
 */
static int
cmicd_pdma_chan_intr_num_get(struct pdma_hw *hw, int chan)
{
    int grp, que, start_num, mask_shift;

    grp = chan / CMICD_PDMA_CMC_CHAN;
    que = chan % CMICD_PDMA_CMC_CHAN;

    mask_shift = 0;
    if (grp > 0) {
        mask_shift = CMICD_IRQ_MASK_SHIFT + grp * 32;
    }
    start_num = CMICD_IRQ_START_NUM + mask_shift;

    return start_num + (que * CMICD_IRQ_NUM_OFFSET);
}

/*!
 * Enable interrupt for a channel
 */
static int
cmicd_pdma_chan_intr_enable(struct pdma_hw *hw, int chan)
{
    int grp, que;

    grp = chan / CMICD_PDMA_CMC_CHAN;
    que = chan % CMICD_PDMA_CMC_CHAN;

    cmicd_pdma_intr_enable(hw, grp, que, CMICD_IRQ_DESC_CNTLD(que));

    return SHR_E_NONE;
}

/*!
 * Disable interrupt for a channel
 */
static int
cmicd_pdma_chan_intr_disable(struct pdma_hw *hw, int chan)
{
    int grp, que;

    grp = chan / CMICD_PDMA_CMC_CHAN;
    que = chan % CMICD_PDMA_CMC_CHAN;

    cmicd_pdma_intr_disable(hw, grp, que, CMICD_IRQ_DESC_CNTLD(que));

    return SHR_E_NONE;
}

/*!
 * Query interrupt status for a channel
 *
 * In group mode (interrupt processing per CMC), need to query each channel's
 * interrupt status.
 *
 */
static int
cmicd_pdma_chan_intr_query(struct pdma_hw *hw, int chan)
{
    uint32_t val;
    int grp, que;

    grp = chan / CMICD_PDMA_CMC_CHAN;
    que = chan % CMICD_PDMA_CMC_CHAN;

    hw->hdls.reg_rd32(hw, CMICD_IRQ_STAT(grp), &val);

    return val & CMICD_IRQ_DESC_CNTLD(que);
}

/*!
 * Check interrupt validity for a channel
 *
 * In group mode (interrupt processing per CMC), need to check each channel's
 * interrupt validity based on its interrupt mask.
 *
 */
static int
cmicd_pdma_chan_intr_check(struct pdma_hw *hw, int chan)
{
    int grp, que;

    grp = chan / CMICD_PDMA_CMC_CHAN;
    que = chan % CMICD_PDMA_CMC_CHAN;

    if (!(hw->dev->ctrl.grp[grp].irq_mask & CMICD_IRQ_DESC_CNTLD(que))) {
        return 0;
    }

    return cmicd_pdma_chan_intr_query(hw, chan);
}

/*!
 * Coalesce interrupt for a channel
 */
static int
cmicd_pdma_chan_intr_coalesce(struct pdma_hw *hw, int chan, int count, int timer)
{
    uint32_t val;
    int grp, que;

    grp = chan / CMICD_PDMA_CMC_CHAN;
    que = chan % CMICD_PDMA_CMC_CHAN;

    val = CMICD_PDMA_INTR_COAL_ENA |
          CMICD_PDMA_INTR_THRESH(count) |
          CMICD_PDMA_INTR_TIMER(timer);
    hw->hdls.reg_wr32(hw, CMICD_PDMA_INTR_COAL(grp, que), val);

    return SHR_E_NONE;
}

/*!
 * Dump registers for a channel
 */
static int
cmicd_pdma_chan_reg_dump(struct pdma_hw *hw, int chan)
{
    uint32_t val;
    int grp, que;

    grp = chan / CMICD_PDMA_CMC_CHAN;
    que = chan % CMICD_PDMA_CMC_CHAN;

    hw->hdls.reg_rd32(hw, CMICD_PDMA_CTRL(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_CH%d_DMA_CTRL: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICD_PDMA_DESC(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_DESC%d: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICD_PDMA_CURR_DESC(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_CH%d_DMA_CURR_DESC: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICD_PDMA_DESC_HALT(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_DESC_HALT_ADDR: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICD_PDMA_COS_RX0(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_CH%d_COS_CTRL_RX_0: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICD_PDMA_COS_RX1(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_CH%d_COS_CTRL_RX_1: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICD_PDMA_COS_MASK0(grp), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_PROGRAMMABLE_COS_MASK0: 0x%08x\n", grp, val);

    hw->hdls.reg_rd32(hw, CMICD_PDMA_COS_MASK1(grp), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_PROGRAMMABLE_COS_MASK1: 0x%08x\n", grp, val);

    hw->hdls.reg_rd32(hw, CMICD_PDMA_INTR_COAL(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_INTR_COAL: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICD_PDMA_RBUF_THRE(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_CH%d_RXBUF_THRESHOLD_CONFIG: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICD_PDMA_STAT(grp), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_STAT: 0x%08x\n", grp, val);

    hw->hdls.reg_rd32(hw, CMICD_PDMA_STAT_HI(grp), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_STAT_HI: 0x%08x\n", grp, val);

    hw->hdls.reg_rd32(hw, CMICD_PDMA_STAT_CLR(grp), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_STAT_CLR: 0x%08x\n", grp, val);

    hw->hdls.reg_rd32(hw, CMICD_PDMA_COUNT_RX(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_PKT_COUNT_CH%d_RXPKT: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICD_PDMA_COUNT_TX(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_PKT_COUNT_CH%d_TXPKT: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICD_IRQ_STAT(grp), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_IRQ_STAT0: 0x%08x\n", grp, val);

    hw->hdls.reg_rd32(hw, CMICD_IRQ_PCI_MASK(grp), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_PCIE_IRQ_MASK0: 0x%08x\n", grp, val);

    hw->hdls.reg_rd32(hw, CMICD_DEV_REV_ID, &val);
    CNET_INFO(hw->unit, "CMIC_DEV_REV_ID: 0x%08x\n", val);

    hw->hdls.reg_rd32(hw, CMICD_CMICM_REV_ID, &val);
    CNET_INFO(hw->unit, "CMIC_CMICM_REV_ID: 0x%08x\n", val);

    return SHR_E_NONE;
}

/*!
 * Initialize function pointers
 */
int
bcmcnet_cmicd_pdma_hw_hdls_init(struct pdma_hw *hw)
{
    if (!hw) {
        return SHR_E_PARAM;
    }

    hw->hdls.reg_rd32 = cmicd_pdma_reg_read32;
    hw->hdls.reg_wr32 = cmicd_pdma_reg_write32;
    hw->hdls.hw_init = cmicd_pdma_hw_init;
    hw->hdls.hw_config = cmicd_pdma_hw_config;
    hw->hdls.hw_reset = cmicd_pdma_hw_reset;
    hw->hdls.chan_start = cmicd_pdma_chan_start;
    hw->hdls.chan_stop = cmicd_pdma_chan_stop;
    hw->hdls.chan_setup = cmicd_pdma_chan_setup;
    hw->hdls.chan_goto = cmicd_pdma_chan_goto;
    hw->hdls.chan_clear = cmicd_pdma_chan_clear;
    hw->hdls.chan_intr_num_get = cmicd_pdma_chan_intr_num_get;
    hw->hdls.chan_intr_enable = cmicd_pdma_chan_intr_enable;
    hw->hdls.chan_intr_disable = cmicd_pdma_chan_intr_disable;
    hw->hdls.chan_intr_query = cmicd_pdma_chan_intr_query;
    hw->hdls.chan_intr_check = cmicd_pdma_chan_intr_check;
    hw->hdls.chan_intr_coalesce = cmicd_pdma_chan_intr_coalesce;
    hw->hdls.chan_reg_dump = cmicd_pdma_chan_reg_dump;

    return SHR_E_NONE;
}

