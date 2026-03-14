/*! \file bcmcnet_cmicr_pdma_hw.c
 *
 * Utility routines for handling BCMCNET hardware (CMICr).
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
#include <bcmcnet/bcmcnet_cmicr.h>

/*!
 * Read 32-bit register
 */
static inline void
cmicr_pdma_reg_read32(struct pdma_hw *hw, uint32_t addr, uint32_t *data)
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
cmicr_pdma_reg_write32(struct pdma_hw *hw, uint32_t addr, uint32_t data)
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
cmicr_pdma_intr_enable(struct pdma_hw *hw, int cmc, int chan)
{
    uint32_t reg, val;

    if (hw->dev->mode == DEV_MODE_UNET || hw->dev->mode == DEV_MODE_VNET) {
        hw->dev->intr_unmask(hw->dev, cmc, chan, 0, 0);
        return;
    }

    if ((cmc == 0) || (cmc == 1 && chan < 8)) {
        reg = PAXB_PDMA_IRQ_ENAB_SET0;
    } else {
        reg = PAXB_PDMA_IRQ_ENAB_SET1;
    }

    val = 1 << chan;
    if (cmc == 0) {
        val <<= CMICR_IRQ_MASK_SHIFT;
    } else if (cmc == 1 && chan < 8) {
        val <<= CMICR_IRQ_MASK_SHIFT + CMICR_PDMA_CMC_CHAN;
    } else {
        val >>= CMICR_IRQ_MASK_SHIFT;
    }

    hw->dev->intr_unmask(hw->dev, cmc, chan, reg & 0xfff, val);
}

/*!
 * Disable interrupt for a channel
 */
static inline void
cmicr_pdma_intr_disable(struct pdma_hw *hw, int cmc, int chan)
{
    uint32_t reg, val;

    if (hw->dev->mode == DEV_MODE_UNET || hw->dev->mode == DEV_MODE_VNET) {
        hw->dev->intr_mask(hw->dev, cmc, chan, 0, 0);
        return;
    }

    if ((cmc == 0) || (cmc == 1 && chan < 8)) {
        reg = PAXB_PDMA_IRQ_ENAB_CLR0;
    } else {
        reg = PAXB_PDMA_IRQ_ENAB_CLR1;
    }

    val = 1 << chan;
    if (cmc == 0) {
        val <<= CMICR_IRQ_MASK_SHIFT;
    } else if (cmc == 1 && chan < 8) {
        val <<= CMICR_IRQ_MASK_SHIFT + CMICR_PDMA_CMC_CHAN;
    } else {
        val >>= CMICR_IRQ_MASK_SHIFT;
    }

    hw->dev->intr_mask(hw->dev, cmc, chan, reg & 0xfff, val);
}

/*!
 * Initialize HW
 */
static int
cmicr_pdma_hw_init(struct pdma_hw *hw)
{
    dev_mode_t mode = DEV_MODE_MAX;
    CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr_t val_header_size;

    /* Temporarily upgrade work mode to get HW information in VNET mode. */
    if (hw->dev->mode == DEV_MODE_VNET) {
        mode = DEV_MODE_VNET;
        hw->dev->mode = DEV_MODE_UNET;
    }

    hw->info.name = CMICR_DEV_NAME;
    hw->info.dev_id = hw->dev->dev_id;
    hw->info.num_cmcs = CMICR_PDMA_CMC_MAX;
    hw->info.cmc_chans = CMICR_PDMA_CMC_CHAN;
    hw->info.num_chans = CMICR_PDMA_CMC_MAX * CMICR_PDMA_CMC_CHAN;
    hw->info.rx_dcb_size = CMICR_PDMA_DCB_SIZE;
    hw->info.tx_dcb_size = CMICR_PDMA_DCB_SIZE;
    hw->hdls.reg_rd32(hw, CMICR_EP_TO_CPU_HEADER_SIZE,
                      &CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr_GET(val_header_size));
    hw->info.rx_ph_size = CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr_EP_TO_CPU_HEADER_SIZEf_GET(val_header_size) * 8;
    hw->info.tx_ph_size = CMICR_TX_PKT_HDR_SIZE;

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
cmicr_pdma_hw_config(struct pdma_hw *hw)
{
    struct dev_ctrl *ctrl = &hw->dev->ctrl;
    struct pdma_rx_queue *rxq = NULL;
    struct pdma_tx_queue *txq = NULL;
    uint32_t que_ctrl;
    int grp, que;
    uint32_t qi;
    int ip_if_hdr_endian = 0;
    int pipe;
    CMIC_CMC_PKTDMA_CTRLr_t pktdma_ctrl;
    CMIC_CMC_PKTDMA_INTR_ENABLEr_t pktdma_intr_enable;
    CMIC_CMC_PKTDMA_INTR_CLRr_t pktdma_intr_clr;
    CMIC_TOP_CONFIGr_t cmic_config;
    CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_t pktdma_rxbuf_thresh;

    CMIC_CMC_PKTDMA_INTR_ENABLEr_CLR(pktdma_intr_enable);
    CMIC_CMC_PKTDMA_INTR_ENABLEr_DESC_CONTROLLED_INTR_ENABLEf_SET(pktdma_intr_enable, 1);

    CMIC_CMC_PKTDMA_INTR_CLRr_CLR(pktdma_intr_clr);
    CMIC_CMC_PKTDMA_INTR_CLRr_DESC_DONE_INTR_CLRf_SET(pktdma_intr_clr, 1);
    CMIC_CMC_PKTDMA_INTR_CLRr_DESC_CONTROLLED_INTR_CLRf_SET(pktdma_intr_clr, 1);
    CMIC_CMC_PKTDMA_INTR_CLRr_INTR_COALESCING_INTR_CLRf_SET(pktdma_intr_clr, 1);
    CMIC_CMC_PKTDMA_INTR_CLRr_DYN_RCNFG_ERR_CLRf_SET(pktdma_intr_clr, 1);

    for (qi = 0; qi < ctrl->nb_rxq; qi++) {
        rxq = (struct pdma_rx_queue *)ctrl->rx_queue[qi];
        grp = rxq->group_id;
        que = rxq->chan_id % CMICR_PDMA_CMC_CHAN;
        que_ctrl = ctrl->grp[grp].que_ctrl[que];
        pipe = ctrl->grp[grp].pipe[que];

        hw->hdls.reg_rd32(hw, CMICR_PDMA_RBUF_THRE(grp, que),
                          &CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_GET(pktdma_rxbuf_thresh));
        CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_ENABLEf_SET(pktdma_rxbuf_thresh, 1);
        hw->hdls.reg_wr32(hw, CMICR_PDMA_RBUF_THRE(grp, que),
                          CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_GET(pktdma_rxbuf_thresh));
        hw->hdls.reg_wr32(hw, CMICR_PDMA_INTR_CLR(grp, que),
                          CMIC_CMC_PKTDMA_INTR_CLRr_GET(pktdma_intr_clr));
        CMIC_CMC_PKTDMA_CTRLr_CLR(pktdma_ctrl);
        if (que_ctrl & PDMA_PKT_BYTE_SWAP) {
            CMIC_CMC_PKTDMA_CTRLr_PKTDMA_ENDIANESSf_SET(pktdma_ctrl, 1);
        }
        if (que_ctrl & PDMA_OTH_BYTE_SWAP) {
            CMIC_CMC_PKTDMA_CTRLr_DESC_ENDIANESSf_SET(pktdma_ctrl, 1);
        }
        if (que_ctrl & PDMA_HDR_BYTE_SWAP) {
            CMIC_CMC_PKTDMA_CTRLr_HEADER_ENDIANESSf_SET(pktdma_ctrl, 1);
        }
        if (!(hw->dev->flags & PDMA_CHAIN_MODE)) {
            CMIC_CMC_PKTDMA_CTRLr_ENABLE_CONTINUOUS_DMAf_SET(pktdma_ctrl, 1);
        }
        CMIC_CMC_PKTDMA_CTRLr_CONTIGUOUS_DESCRIPTORSf_SET(pktdma_ctrl, 1);
        CMIC_CMC_PKTDMA_CTRLr_DESC_DONE_INTR_MODEf_SET(pktdma_ctrl, 1);
        if (pipe == 1) {
            CMIC_CMC_PKTDMA_CTRLr_PIPE_MAPf_SET(pktdma_ctrl, 1);
        } else if (pipe != 0) {
            return SHR_E_CONFIG;
        }

        hw->hdls.reg_wr32(hw, CMICR_PDMA_CTRL(grp, que),
                          CMIC_CMC_PKTDMA_CTRLr_GET(pktdma_ctrl));
        hw->hdls.reg_wr32(hw, CMICR_PDMA_INTR_ENAB(grp, que),
                          CMIC_CMC_PKTDMA_INTR_ENABLEr_GET(pktdma_intr_enable));
    }

    for (qi = 0; qi < ctrl->nb_txq; qi++) {
        txq = (struct pdma_tx_queue *)ctrl->tx_queue[qi];
        grp = txq->group_id;
        que = txq->chan_id % CMICR_PDMA_CMC_CHAN;
        que_ctrl = ctrl->grp[grp].que_ctrl[que];
        pipe = ctrl->grp[grp].pipe[que];

        hw->hdls.reg_wr32(hw, CMICR_PDMA_INTR_CLR(grp, que),
                          CMIC_CMC_PKTDMA_INTR_CLRr_GET(pktdma_intr_clr));
        CMIC_CMC_PKTDMA_CTRLr_CLR(pktdma_ctrl);
        if (que_ctrl & PDMA_PKT_BYTE_SWAP) {
            CMIC_CMC_PKTDMA_CTRLr_PKTDMA_ENDIANESSf_SET(pktdma_ctrl, 1);
            CMIC_CMC_PKTDMA_CTRLr_HEADER_ENDIANESSf_SET(pktdma_ctrl, 1);
            ip_if_hdr_endian = 1;
        }
        if (que_ctrl & PDMA_OTH_BYTE_SWAP) {
            CMIC_CMC_PKTDMA_CTRLr_DESC_ENDIANESSf_SET(pktdma_ctrl, 1);
        }
        if (que_ctrl & PDMA_HDR_BYTE_SWAP) {
            ip_if_hdr_endian = 1;
        }
        if (!(hw->dev->flags & PDMA_CHAIN_MODE)) {
            CMIC_CMC_PKTDMA_CTRLr_ENABLE_CONTINUOUS_DMAf_SET(pktdma_ctrl, 1);
        }
        CMIC_CMC_PKTDMA_CTRLr_CONTIGUOUS_DESCRIPTORSf_SET(pktdma_ctrl, 1);
        CMIC_CMC_PKTDMA_CTRLr_DESC_DONE_INTR_MODEf_SET(pktdma_ctrl, 1);
        CMIC_CMC_PKTDMA_CTRLr_DIRECTIONf_SET(pktdma_ctrl, 1);
        if (pipe == 1) {
            CMIC_CMC_PKTDMA_CTRLr_PIPE_MAPf_SET(pktdma_ctrl, 1);
        } else if (pipe != 0) {
            return SHR_E_CONFIG;
        }

        hw->hdls.reg_wr32(hw, CMICR_PDMA_CTRL(grp, que),
                          CMIC_CMC_PKTDMA_CTRLr_GET(pktdma_ctrl));
        hw->hdls.reg_wr32(hw, CMICR_PDMA_INTR_ENAB(grp, que),
                          CMIC_CMC_PKTDMA_INTR_ENABLEr_GET(pktdma_intr_enable));
    }

    hw->hdls.reg_rd32(hw,
                      CMICR_TOP_CONFIG, &CMIC_TOP_CONFIGr_GET(cmic_config));
    CMIC_TOP_CONFIGr_IP_INTERFACE_HEADER_ENDIANESSf_SET(cmic_config,
                                                        ip_if_hdr_endian);
    hw->hdls.reg_wr32(hw,
                      CMICR_TOP_CONFIG, CMIC_TOP_CONFIGr_GET(cmic_config));

    return SHR_E_NONE;
}

/*!
 * Reset HW
 */
static int
cmicr_pdma_hw_reset(struct pdma_hw *hw)
{
    int gi, qi;

    for (gi = 0; gi < hw->dev->num_groups; gi++) {
        if (!hw->dev->ctrl.grp[gi].attached) {
            continue;
        }
        for (qi = 0; qi < CMICR_PDMA_CMC_CHAN; qi++) {
            if (1 << qi & hw->dev->ctrl.grp[gi].bm_rxq ||
                1 << qi & hw->dev->ctrl.grp[gi].bm_txq) {
                hw->hdls.reg_wr32(hw, CMICR_PDMA_CTRL(gi, qi), 0);
            }
        }
    }

    return SHR_E_NONE;
}

/*!
 * Start a channel
 */
static int
cmicr_pdma_chan_start(struct pdma_hw *hw, int chan)
{
    CMIC_CMC_PKTDMA_CTRLr_t pktdma_ctrl;
    int grp, que;

    grp = chan / CMICR_PDMA_CMC_CHAN;
    que = chan % CMICR_PDMA_CMC_CHAN;

    hw->hdls.reg_rd32(hw, CMICR_PDMA_CTRL(grp, que),
                      &CMIC_CMC_PKTDMA_CTRLr_GET(pktdma_ctrl));
    CMIC_CMC_PKTDMA_CTRLr_DMA_ENf_SET(pktdma_ctrl, 1);
    hw->hdls.reg_wr32(hw, CMICR_PDMA_CTRL(grp, que),
                      CMIC_CMC_PKTDMA_CTRLr_GET(pktdma_ctrl));

    MEMORY_BARRIER;

    return SHR_E_NONE;
}

/*!
 * Stop a channel
 */
static int
cmicr_pdma_chan_stop(struct pdma_hw *hw, int chan)
{
    CMIC_CMC_PKTDMA_CTRLr_t pktdma_ctrl;
    CMIC_CMC_PKTDMA_INTR_CLRr_t pktdma_intr_clr;
    CMIC_CMC_PKTDMA_STATr_t pktdma_stat;
    int grp, que;
    int retry = CMICR_HW_RETRY_TIMES;

    grp = chan / CMICR_PDMA_CMC_CHAN;
    que = chan % CMICR_PDMA_CMC_CHAN;

    do {
        hw->hdls.reg_rd32(hw, CMICR_PDMA_STAT(grp, que),
                          &CMIC_CMC_PKTDMA_STATr_GET(pktdma_stat));
        if (CMIC_CMC_PKTDMA_STATr_CHAIN_DONEf_GET(pktdma_stat)) {
            hw->hdls.reg_rd32(hw, CMICR_PDMA_CTRL(grp, que),
                              &CMIC_CMC_PKTDMA_CTRLr_GET(pktdma_ctrl));
            CMIC_CMC_PKTDMA_CTRLr_DMA_ENf_SET(pktdma_ctrl, 0);
            hw->hdls.reg_wr32(hw, CMICR_PDMA_CTRL(grp, que),
                              CMIC_CMC_PKTDMA_CTRLr_GET(pktdma_ctrl));
            return SHR_E_NONE;
        }
        if (!(hw->dev->flags & PDMA_CHAIN_MODE)) {
            break;
        }
    } while (retry--);

    /* if chain done is 0, abort */
    hw->hdls.reg_rd32(hw, CMICR_PDMA_CTRL(grp, que),
                      &CMIC_CMC_PKTDMA_CTRLr_GET(pktdma_ctrl));
    CMIC_CMC_PKTDMA_CTRLr_DMA_ENf_SET(pktdma_ctrl, 1);
    CMIC_CMC_PKTDMA_CTRLr_ABORT_DMAf_SET(pktdma_ctrl, 1);
    hw->hdls.reg_wr32(hw, CMICR_PDMA_CTRL(grp, que),
                      CMIC_CMC_PKTDMA_CTRLr_GET(pktdma_ctrl));

    MEMORY_BARRIER;

    retry = CMICR_HW_RETRY_TIMES;
    do {
        hw->hdls.reg_rd32(hw, CMICR_PDMA_STAT(grp, que),
                          &CMIC_CMC_PKTDMA_STATr_GET(pktdma_stat));
        if (CMIC_CMC_PKTDMA_STATr_CHAIN_DONEf_GET(pktdma_stat)) {
            break;
        }
        if (!retry) {
            CNET_ERROR(hw->unit, "Timeout to wait abort done\n");
        }
    } while (retry--);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_CTRL(grp, que),
                      &CMIC_CMC_PKTDMA_CTRLr_GET(pktdma_ctrl));
    CMIC_CMC_PKTDMA_CTRLr_DMA_ENf_SET(pktdma_ctrl, 0);
    CMIC_CMC_PKTDMA_CTRLr_ABORT_DMAf_SET(pktdma_ctrl, 0);
    hw->hdls.reg_wr32(hw, CMICR_PDMA_CTRL(grp, que),
                      CMIC_CMC_PKTDMA_CTRLr_GET(pktdma_ctrl));

    MEMORY_BARRIER;

    CMIC_CMC_PKTDMA_INTR_CLRr_CLR(pktdma_intr_clr);
    CMIC_CMC_PKTDMA_INTR_CLRr_DESC_DONE_INTR_CLRf_SET(pktdma_intr_clr, 1);
    CMIC_CMC_PKTDMA_INTR_CLRr_DESC_CONTROLLED_INTR_CLRf_SET(pktdma_intr_clr, 1);
    CMIC_CMC_PKTDMA_INTR_CLRr_INTR_COALESCING_INTR_CLRf_SET(pktdma_intr_clr, 1);
    CMIC_CMC_PKTDMA_INTR_CLRr_DYN_RCNFG_ERR_CLRf_SET(pktdma_intr_clr, 1);
    hw->hdls.reg_wr32(hw, CMICR_PDMA_INTR_CLR(grp, que),
                      CMIC_CMC_PKTDMA_INTR_CLRr_GET(pktdma_intr_clr));

    MEMORY_BARRIER;

    return SHR_E_NONE;
}

/*!
 * Setup a channel
 */
static int
cmicr_pdma_chan_setup(struct pdma_hw *hw, int chan, uint64_t addr)
{
    int grp, que;

    grp = chan / CMICR_PDMA_CMC_CHAN;
    que = chan % CMICR_PDMA_CMC_CHAN;

    hw->hdls.reg_wr32(hw, CMICR_PDMA_DESC_LO(grp, que), addr);
    hw->hdls.reg_wr32(hw, CMICR_PDMA_DESC_HI(grp, que), DMA_TO_BUS_HI(addr >> 32));

    MEMORY_BARRIER;

    return SHR_E_NONE;
}

/*!
 * Set halt point for a channel
 */
static int
cmicr_pdma_chan_goto(struct pdma_hw *hw, int chan, uint64_t addr)
{
    int grp, que;

    grp = chan / CMICR_PDMA_CMC_CHAN;
    que = chan % CMICR_PDMA_CMC_CHAN;

    hw->hdls.reg_wr32(hw, CMICR_PDMA_DESC_HALT_LO(grp, que), addr);
    hw->hdls.reg_wr32(hw, CMICR_PDMA_DESC_HALT_HI(grp, que), DMA_TO_BUS_HI(addr >> 32));

    MEMORY_BARRIER;

    return SHR_E_NONE;
}

/*!
 * Clear a channel
 */
static int
cmicr_pdma_chan_clear(struct pdma_hw *hw, int chan)
{
    CMIC_CMC_PKTDMA_INTR_CLRr_t pktdma_intr_clr;
    int grp, que;

    grp = chan / CMICR_PDMA_CMC_CHAN;
    que = chan % CMICR_PDMA_CMC_CHAN;

    CMIC_CMC_PKTDMA_INTR_CLRr_CLR(pktdma_intr_clr);
    CMIC_CMC_PKTDMA_INTR_CLRr_DESC_CONTROLLED_INTR_CLRf_SET(pktdma_intr_clr, 1);
    hw->hdls.reg_wr32(hw, CMICR_PDMA_INTR_CLR(grp, que),
                      CMIC_CMC_PKTDMA_INTR_CLRr_GET(pktdma_intr_clr));

    MEMORY_BARRIER;

    return SHR_E_NONE;
}

/*!
 * Check a channel
 */
static int
cmicr_pdma_chan_check(struct pdma_hw *hw, int chan)
{
    CMIC_CMC_PKTDMA_STATr_t pktdma_stat;
    int grp, que;

    grp = chan / CMICR_PDMA_CMC_CHAN;
    que = chan % CMICR_PDMA_CMC_CHAN;

    MEMORY_BARRIER;

    hw->hdls.reg_rd32(hw, CMICR_PDMA_STAT(grp, que),
                      &CMIC_CMC_PKTDMA_STATr_GET(pktdma_stat));

    return CMIC_CMC_PKTDMA_STATr_DESC_CONTROLLEDf_GET(pktdma_stat);
}

/*!
 * Get interrupt number for a channel
 */
static int
cmicr_pdma_chan_intr_num_get(struct pdma_hw *hw, int chan)
{
    int grp, que;
    const int irq_map[CMICR_PDMA_CMC_MAX][CMICR_PDMA_CMC_CHAN] =
        {{CMICR_IRQ_CMC0_PKTDMA_CH0_INTR,  CMICR_IRQ_CMC0_PKTDMA_CH1_INTR,
          CMICR_IRQ_CMC0_PKTDMA_CH2_INTR,  CMICR_IRQ_CMC0_PKTDMA_CH3_INTR,
          CMICR_IRQ_CMC0_PKTDMA_CH4_INTR,  CMICR_IRQ_CMC0_PKTDMA_CH5_INTR,
          CMICR_IRQ_CMC0_PKTDMA_CH6_INTR,  CMICR_IRQ_CMC0_PKTDMA_CH7_INTR,
          CMICR_IRQ_CMC0_PKTDMA_CH8_INTR,  CMICR_IRQ_CMC0_PKTDMA_CH9_INTR,
          CMICR_IRQ_CMC0_PKTDMA_CH10_INTR, CMICR_IRQ_CMC0_PKTDMA_CH11_INTR,
          CMICR_IRQ_CMC0_PKTDMA_CH12_INTR, CMICR_IRQ_CMC0_PKTDMA_CH13_INTR,
          CMICR_IRQ_CMC0_PKTDMA_CH14_INTR, CMICR_IRQ_CMC0_PKTDMA_CH15_INTR},
         {CMICR_IRQ_CMC1_PKTDMA_CH0_INTR,  CMICR_IRQ_CMC1_PKTDMA_CH1_INTR,
          CMICR_IRQ_CMC1_PKTDMA_CH2_INTR,  CMICR_IRQ_CMC1_PKTDMA_CH3_INTR,
          CMICR_IRQ_CMC1_PKTDMA_CH4_INTR,  CMICR_IRQ_CMC1_PKTDMA_CH5_INTR,
          CMICR_IRQ_CMC1_PKTDMA_CH6_INTR,  CMICR_IRQ_CMC1_PKTDMA_CH7_INTR,
          CMICR_IRQ_CMC1_PKTDMA_CH8_INTR,  CMICR_IRQ_CMC1_PKTDMA_CH9_INTR,
          CMICR_IRQ_CMC1_PKTDMA_CH10_INTR, CMICR_IRQ_CMC1_PKTDMA_CH11_INTR,
          CMICR_IRQ_CMC1_PKTDMA_CH12_INTR, CMICR_IRQ_CMC1_PKTDMA_CH13_INTR,
          CMICR_IRQ_CMC1_PKTDMA_CH14_INTR, CMICR_IRQ_CMC1_PKTDMA_CH15_INTR}};

    grp = chan / CMICR_PDMA_CMC_CHAN;
    que = chan % CMICR_PDMA_CMC_CHAN;

    if (grp < 0 || grp >= CMICR_PDMA_CMC_MAX) {
        return -1;
    }

    return irq_map[grp][que];
}

/*!
 * Enable interrupt for a channel
 */
static int
cmicr_pdma_chan_intr_enable(struct pdma_hw *hw, int chan)
{
    int grp, que;

    grp = chan / CMICR_PDMA_CMC_CHAN;
    que = chan % CMICR_PDMA_CMC_CHAN;

    hw->dev->ctrl.grp[grp].irq_mask |= (1 << que);
    cmicr_pdma_intr_enable(hw, grp, que);

    return SHR_E_NONE;
}

/*!
 * Disable interrupt for a channel
 */
static int
cmicr_pdma_chan_intr_disable(struct pdma_hw *hw, int chan)
{
    int grp, que;

    grp = chan / CMICR_PDMA_CMC_CHAN;
    que = chan % CMICR_PDMA_CMC_CHAN;

    hw->dev->ctrl.grp[grp].irq_mask &= ~(1 << que);
    cmicr_pdma_intr_disable(hw, grp, que);

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
cmicr_pdma_chan_intr_query(struct pdma_hw *hw, int chan)
{
    CMIC_CMC_PKTDMA_INTRr_t pktdma_intr;
    int grp, que;

    grp = chan / CMICR_PDMA_CMC_CHAN;
    que = chan % CMICR_PDMA_CMC_CHAN;

    hw->hdls.reg_rd32(hw, CMICR_PDMA_INTR_STAT(grp, que),
                      &CMIC_CMC_PKTDMA_INTRr_GET(pktdma_intr));

    return CMIC_CMC_PKTDMA_INTRr_DESC_CONTROLLED_INTRf_GET(pktdma_intr);
}

/*!
 * Check interrupt validity for a channel
 *
 * In group mode (interrupt processing per CMC), need to check each channel's
 * interrupt validity based on its interrupt mask.
 *
 */
static int
cmicr_pdma_chan_intr_check(struct pdma_hw *hw, int chan)
{
    int grp, que;

    grp = chan / CMICR_PDMA_CMC_CHAN;
    que = chan % CMICR_PDMA_CMC_CHAN;

    if (!(hw->dev->ctrl.grp[grp].irq_mask & (1 << que))) {
        return 0;
    }

    return cmicr_pdma_chan_intr_query(hw, chan);
}

/*!
 * Coalesce interrupt for a channel
 */
static int
cmicr_pdma_chan_intr_coalesce(struct pdma_hw *hw, int chan, int count, int timer)
{
    CMIC_CMC_PKTDMA_INTR_COALr_t pktdma_intr_col;
    int grp, que;

    grp = chan / CMICR_PDMA_CMC_CHAN;
    que = chan % CMICR_PDMA_CMC_CHAN;

    CMIC_CMC_PKTDMA_INTR_COALr_CLR(pktdma_intr_col);
    CMIC_CMC_PKTDMA_INTR_COALr_ENABLEf_SET(pktdma_intr_col, 1);
    CMIC_CMC_PKTDMA_INTR_COALr_COUNTf_SET(pktdma_intr_col, count);
    CMIC_CMC_PKTDMA_INTR_COALr_TIMERf_SET(pktdma_intr_col, timer);
    hw->hdls.reg_wr32(hw, CMICR_PDMA_INTR_COAL(grp, que),
                      CMIC_CMC_PKTDMA_INTR_COALr_GET(pktdma_intr_col));

    return SHR_E_NONE;
}

/*!
 * Dump registers for a channel
 */
static int
cmicr_pdma_chan_reg_dump(struct pdma_hw *hw, int chan)
{
    uint32_t val;
    int grp, que;

    grp = chan / CMICR_PDMA_CMC_CHAN;
    que = chan % CMICR_PDMA_CMC_CHAN;

    hw->hdls.reg_rd32(hw, CMICR_PDMA_CTRL(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_CTRL: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_DESC_LO(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_DESC_LO: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_DESC_HI(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_DESC_HI: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_CURR_DESC_LO(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_CURR_DESC_LO: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_CURR_DESC_HI(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_CURR_DESC_HI: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_DESC_HALT_LO(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_DESC_HALT_ADDR_LO: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_DESC_HALT_HI(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_DESC_HALT_ADDR_HI: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_COS_CTRL_RX0(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_COS_CTRL_RX_0: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_COS_CTRL_RX1(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_COS_CTRL_RX_1: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_INTR_COAL(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_INTR_COAL: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_RBUF_THRE(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_RXBUF_THRESHOLD_CONFIG: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_STAT(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_STAT: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_COUNT_RX(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_PKT_COUNT_RXPKT: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_COUNT_TX(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_PKT_COUNT_TXPKT: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_COUNT_RX_DROP(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_DMA_CH%d_PKT_COUNT_RXPKT_DROP: 0x%08x\n", grp, que, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_INTR_ENAB(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_INTR_ENAB: 0x%08x\n", grp, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_INTR_STAT(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_INTR_STAT: 0x%08x\n", grp, val);

    hw->hdls.reg_rd32(hw, CMICR_PDMA_INTR_CLR(grp, que), &val);
    CNET_INFO(hw->unit, "CMIC_CMC%d_INTR_CLR: 0x%08x\n", grp, val);

    val = hw->dev->ctrl.grp[grp].irq_mask;
    CNET_INFO(hw->unit, "CMIC_CMC%d_IRQ_ENAB: 0x%08x\n", grp, val);

    hw->hdls.reg_rd32(hw, CMICR_EP_TO_CPU_HEADER_SIZE, &val);
    CNET_INFO(hw->unit, "CMIC_EP_TO_CPU_HEADER_SIZE: 0x%08x\n", val);

    return SHR_E_NONE;
}

/*!
 * Initialize function pointers
 */
int
bcmcnet_cmicr_pdma_hw_hdls_init(struct pdma_hw *hw)
{
    if (!hw) {
        return SHR_E_PARAM;
    }

    hw->hdls.reg_rd32 = cmicr_pdma_reg_read32;
    hw->hdls.reg_wr32 = cmicr_pdma_reg_write32;
    hw->hdls.hw_init = cmicr_pdma_hw_init;
    hw->hdls.hw_config = cmicr_pdma_hw_config;
    hw->hdls.hw_reset = cmicr_pdma_hw_reset;
    hw->hdls.chan_start = cmicr_pdma_chan_start;
    hw->hdls.chan_stop = cmicr_pdma_chan_stop;
    hw->hdls.chan_setup = cmicr_pdma_chan_setup;
    hw->hdls.chan_goto = cmicr_pdma_chan_goto;
    hw->hdls.chan_clear = cmicr_pdma_chan_clear;
    hw->hdls.chan_check = cmicr_pdma_chan_check;
    hw->hdls.chan_intr_num_get = cmicr_pdma_chan_intr_num_get;
    hw->hdls.chan_intr_enable = cmicr_pdma_chan_intr_enable;
    hw->hdls.chan_intr_disable = cmicr_pdma_chan_intr_disable;
    hw->hdls.chan_intr_query = cmicr_pdma_chan_intr_query;
    hw->hdls.chan_intr_check = cmicr_pdma_chan_intr_check;
    hw->hdls.chan_intr_coalesce = cmicr_pdma_chan_intr_coalesce;
    hw->hdls.chan_reg_dump = cmicr_pdma_chan_reg_dump;

    return SHR_E_NONE;
}
