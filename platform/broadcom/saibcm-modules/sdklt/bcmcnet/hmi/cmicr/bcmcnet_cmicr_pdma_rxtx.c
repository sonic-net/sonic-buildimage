/*! \file bcmcnet_cmicr_pdma_rxtx.c
 *
 * Utility routines for BCMCNET hardware (CMICr) specific Rx/Tx.
 *
 * Here are the CMIC specific Rx/Tx routines including DCBs resource allocation
 * and clean up, DCBs configuration, Rx buffers allocation, Tx buffers release,
 * Rx/Tx packets processing, etc.
 * They are shared among all the modes (UNET, KNET, VNET, HNET) and in both of
 * user space and kernel space.
 *
 * The driver uses a ring of DCBs per DMA channel based on Continuous DMA mode.
 * The beginning is written to register pointing to the physical address of the
 * start of the ring. The ring size is maintained by the driver. A HALT DCB
 * physical address is written to DMA register timely to indicate how many DCBs
 * can be handled by HW.
 *
 * When a packet is received, an interrupt is triggered. The handler will go
 * through the Rx DCB ring to process the current completed DCB and every
 * subsequent DCBs until no one is left. The received packet is processed and
 * passed up to the high level SW. After that, a new buffer is allocated and
 * the DCB is updated for receiving a new packet. A new HALT DCB is selected
 * and its physical address is written to DMA register.
 *
 * When a packet is transmitted, the driver starts where it left off last time
 * in the Tx DCB ring, updates the DCB and writes its physical address to DMA
 * register so as to start DMA. Once the transmitting is finished, the handler
 * is informed to clean up the buffer based on the work mode. In KNET or HNET
 * mode, an interrupt will be triggered. Polling mode is used in CNET or VNET
 * mode, the buffers will be cleaned up when the number of dirty DCBs reaches
 * a pre-defined threshold.
 *
 * In VNET and HNET modes, DCB updating between virtual ring and real ring and
 * a IOCTL based notification mechanism are involved. The hypervisor in kernel
 * emulates the DMA HW behaviors to update DCBs in virtual network and inform
 * the handler something happened. Likewise, the hypervisor updates itself real
 * DCB ring from the virtual ring to start DMA for transmitting a packet once a
 * notification is received from the virtual network.
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
#include <bcmcnet/bcmcnet_buff.h>
#include <bcmcnet/bcmcnet_cmicr.h>

#define RX_DCB_CTRL_WORD    (2)
#define RX_DCB_STATUS_WORD  (3)
#define RX_DCB_STATUS_GET(_rd_)        (RX_DCB_GET((_rd_), RX_DCB_STATUS_WORD))
#define RX_DCB_STATUS_SET(_rd_, _val_) (RX_DCB_SET((_rd_), RX_DCB_STATUS_WORD, (_val_)))
#define RX_DCB_CTRL_SET(_rd_, _val_)   (RX_DCB_SET((_rd_), RX_DCB_CTRL_WORD, (_val_)))

#define TX_DCB_CTRL_WORD    (2)
#define TX_DCB_STATUS_WORD  (3)
#define TX_DCB_STATUS_GET(_td_)        (TX_DCB_GET((_td_), TX_DCB_STATUS_WORD))
#define TX_DCB_STATUS_SET(_td_, _val_) (TX_DCB_SET((_td_), TX_DCB_STATUS_WORD, (_val_)))
#define TX_DCB_CTRL_SET(_td_, _val_)   (TX_DCB_SET((_td_), TX_DCB_CTRL_WORD, (_val_)))

/*!
 * Configure Rx descriptor
 */
static inline void
cmicr_rx_desc_config(volatile RX_DCB_t *rd, uint64_t addr, uint32_t len)
{
    uint32_t remain;

    /* word 0, 1 : addr */
    RX_DCB_ADDR_LOf_SET(*rd, addr);
    RX_DCB_ADDR_HIf_SET(*rd, DMA_TO_BUS_HI(addr >> 32));

    /* word 2: ctrl */
    remain = RX_DCB_DESC_REMAINf_GET(*rd);
    RX_DCB_CTRL_SET(*rd, 0);
    RX_DCB_DESC_REMAINf_SET(*rd, remain);
    RX_DCB_DESC_CTRL_INTRf_SET(*rd, 1);
    RX_DCB_CHAINf_SET(*rd, 1);
    RX_DCB_BYTE_COUNTf_SET(*rd, len);

    /* word 3: status */
    RX_DCB_STATUS_SET(*rd, 0);

    MEMORY_BARRIER;
}

/*!
 * Configure Tx descriptor
 */
static inline void
cmicr_tx_desc_config(volatile TX_DCB_t *td, uint64_t addr, uint32_t len, uint16_t flags)
{
    uint32_t remain;

    /* word 0,1 : addr */
    TX_DCB_ADDR_LOf_SET(*td, addr);
    TX_DCB_ADDR_HIf_SET(*td, DMA_TO_BUS_HI(addr >> 32));

    /* word 2: ctrl */
    remain = TX_DCB_DESC_REMAINf_GET(*td);
    TX_DCB_CTRL_SET(*td, 0);
    TX_DCB_DESC_REMAINf_SET(*td, remain);
    TX_DCB_DESC_CTRL_INTRf_SET(*td, 1);
    TX_DCB_CHAINf_SET(*td, 1);
    TX_DCB_BYTE_COUNTf_SET(*td, len);
    if (flags & PDMA_TX_HIGIG_PKT) {
        TX_DCB_HGf_SET(*td, 1);
    }
    if (flags & PDMA_TX_PURGE_PKT) {
        TX_DCB_PURGEf_SET(*td, 1);
    }

    /* word 3: status */
    TX_DCB_STATUS_SET(*td, 0);

    MEMORY_BARRIER;
}

/*!
 * Configure Rx reload descriptor
 */
static inline void
cmicr_rx_rldesc_config(volatile RX_DCB_t *rd, uint64_t addr)
{
    /* word 0,1 : addr */
    RX_DCB_ADDR_LOf_SET(*rd, addr);
    RX_DCB_ADDR_HIf_SET(*rd, DMA_TO_BUS_HI(addr >> 32));

    /* word 2: ctrl */
    RX_DCB_CTRL_SET(*rd, 0);
    RX_DCB_DESC_CTRL_INTRf_SET(*rd, 1);
    RX_DCB_CHAINf_SET(*rd, 1);
    RX_DCB_RELOADf_SET(*rd, 1);

    /* word 3: status */
    RX_DCB_STATUS_SET(*rd, 0);

    MEMORY_BARRIER;
}

/*!
 * Configure Tx reload descriptor
 */
static inline void
cmicr_tx_rldesc_config(volatile TX_DCB_t *td, uint64_t addr)
{
    /* word 0, 1 : addr */
    TX_DCB_ADDR_LOf_SET(*td, addr);
    TX_DCB_ADDR_HIf_SET(*td, DMA_TO_BUS_HI(addr >> 32));

    /* word 2: ctrl */
    TX_DCB_CTRL_SET(*td, 0);
    TX_DCB_DESC_CTRL_INTRf_SET(*td, 1);
    TX_DCB_CHAINf_SET(*td, 1);
    TX_DCB_RELOADf_SET(*td, 1);

    /* word 3: status */
    TX_DCB_STATUS_SET(*td, 0);

    MEMORY_BARRIER;
}

/*!
 * Chain Rx descriptor
 */
static inline void
cmicr_rx_desc_chain(volatile RX_DCB_t *rd, int chain)
{
    if (chain) {
        RX_DCB_CHAINf_SET(*rd, 1);
    } else {
        RX_DCB_CHAINf_SET(*rd, 0);
    }

    MEMORY_BARRIER;
}

/*!
 * Chain Tx descriptor
 */
static inline void
cmicr_tx_desc_chain(volatile TX_DCB_t *td, int chain)
{
    if (chain) {
        TX_DCB_CHAINf_SET(*td, 1);
    } else {
        TX_DCB_CHAINf_SET(*td, 0);
    }

    MEMORY_BARRIER;
}

/*!
 * Set Rx descriptor remain
 */
static inline void
cmicr_rx_desc_remain(volatile RX_DCB_t *rd, uint32_t rm)
{
    RX_DCB_DESC_REMAINf_SET(*rd, rm);

    MEMORY_BARRIER;
}

/*!
 * Set Tx descriptor remain
 */
static inline void
cmicr_tx_desc_remain(volatile TX_DCB_t *td, uint32_t rm)
{
    TX_DCB_DESC_REMAINf_SET(*td, rm);

    MEMORY_BARRIER;
}

/*!
 * Get unused descriptors in a Rx ring
 */
static inline int
cmicr_pdma_rx_ring_unused(struct pdma_rx_queue *rxq)
{
    /* Leave one descriptor unused so as not to halt */
    return (rxq->nb_desc + rxq->curr - rxq->halt - 1) % rxq->nb_desc;
}

/*!
 * Get unused descriptors in a Tx ring
 */
static inline int
cmicr_pdma_tx_ring_unused(struct pdma_tx_queue *txq)
{
    /* Leave one descriptor unused so as not to halt */
    return (txq->nb_desc + txq->dirt - txq->curr - 1) % txq->nb_desc;
}

/*!
 * Initialize Rx descriptors
 */
static int
cmicr_pdma_rx_desc_init(struct pdma_hw *hw, struct pdma_rx_queue *rxq)
{
    struct pdma_dev *dev = hw->dev;
    struct pdma_buf_mngr *bm = (struct pdma_buf_mngr *)dev->ctrl.buf_mngr;
    volatile RX_DCB_t *ring = (volatile RX_DCB_t *)rxq->ring;
    bool refill = false;
    dma_addr_t addr;
    uint32_t di, rm;
    int rv;

    for (di = 0; di < rxq->nb_desc; di++) {
        if (!rxq->pbuf[di].dma) {
            /* Allocate pktbuf for ring entry */
            rv = bm->rx_buf_alloc(dev, rxq, &rxq->pbuf[di]);
            if (SHR_FAILURE(rv)) {
                if (rxq->state & PDMA_RX_BATCH_REFILL) {
                    refill = true;
                } else {
                    goto cleanup;
                }
            }
        }
        /* Config receive descriptor ring */
        bm->rx_buf_dma(dev, rxq, &rxq->pbuf[di], &addr);
        cmicr_rx_desc_config(&ring[di], addr, rxq->buf_size);
        rm = (rxq->nb_desc - di) >= CMICR_DESC_REMAIN_MAX ?
             CMICR_DESC_REMAIN_MAX : rxq->nb_desc - di;
        cmicr_rx_desc_remain(&ring[di], rm);
        if (hw->dev->flags & PDMA_CHAIN_MODE && di == rxq->nb_desc - 1) {
            cmicr_rx_desc_chain(&ring[di], 0);
        }
    }
    /* Config the last descriptor in the ring as reload descriptor */
    cmicr_rx_rldesc_config(&ring[di], rxq->ring_addr);

    rxq->curr = 0;
    if (!refill) {
        rxq->halt = rxq->nb_desc - 1;
    }

    rxq->halt_addr = rxq->ring_addr + sizeof(RX_DCB_t) * rxq->halt;
    hw->hdls.chan_goto(hw, rxq->chan_id, rxq->halt_addr);
    hw->hdls.chan_setup(hw, rxq->chan_id, rxq->ring_addr);

    return SHR_E_NONE;

cleanup:
    for (di = 0; di < rxq->nb_desc; di++) {
        if (rxq->pbuf[di].dma) {
            bm->rx_buf_free(dev, rxq, &rxq->pbuf[di]);
        }
        cmicr_rx_desc_config(&ring[di], 0, 0);
    }

    CNET_ERROR(hw->unit, "RX: Failed to allocate memory\n");

    return SHR_E_MEMORY;
}

/*!
 * Cleanup Rx descriptors
 */
static int
cmicr_pdma_rx_desc_clean(struct pdma_hw *hw, struct pdma_rx_queue *rxq)
{
    struct pdma_dev *dev = hw->dev;
    struct pdma_buf_mngr *bm = (struct pdma_buf_mngr *)dev->ctrl.buf_mngr;
    volatile RX_DCB_t *ring = (volatile RX_DCB_t *)rxq->ring;
    uint32_t di;

    /* Go through all the descriptors and free pktbuf */
    for (di = 0; di < rxq->nb_desc; di++) {
        if (rxq->pbuf[di].dma) {
            bm->rx_buf_free(dev, rxq, &rxq->pbuf[di]);
        }
        cmicr_rx_desc_config(&ring[di], 0, 0);
    }

    rxq->curr = 0;
    rxq->halt = 0;

    return SHR_E_NONE;
}

/*!
 * Initialize Tx descriptors
 */
static int
cmicr_pdma_tx_desc_init(struct pdma_hw *hw, struct pdma_tx_queue *txq)
{
    struct pdma_dev *dev = hw->dev;
    struct pdma_buf_mngr *bm = (struct pdma_buf_mngr *)dev->ctrl.buf_mngr;
    volatile TX_DCB_t *ring = (volatile TX_DCB_t *)txq->ring;
    uint32_t di, rm;

    for (di = 0; di < txq->nb_desc; di++) {
        if (txq->pbuf[di].dma) {
            bm->tx_buf_free(dev, txq, &txq->pbuf[di]);
        }
        /* Config transmit descriptor ring */
        cmicr_tx_desc_config(&ring[di], 0, 0, 0);
        rm = (txq->nb_desc - di) >= CMICR_DESC_REMAIN_MAX ?
             CMICR_DESC_REMAIN_MAX : txq->nb_desc - di;
        cmicr_tx_desc_remain(&ring[di], rm);
        if (hw->dev->flags & PDMA_CHAIN_MODE) {
            cmicr_tx_desc_chain(&ring[di], 0);
        }
    }
    /* Config the last descriptor in the ring as reload descriptor */
    cmicr_tx_rldesc_config(&ring[di], txq->ring_addr);

    txq->curr = 0;
    txq->dirt = 0;
    txq->halt = 0;

    txq->halt_addr = txq->ring_addr;
    hw->hdls.chan_goto(hw, txq->chan_id, txq->halt_addr);
    hw->hdls.chan_setup(hw, txq->chan_id, txq->ring_addr);

    return SHR_E_NONE;
}

/*!
 * Cleanup Tx descriptors
 */
static int
cmicr_pdma_tx_desc_clean(struct pdma_hw *hw, struct pdma_tx_queue *txq)
{
    struct pdma_dev *dev = hw->dev;
    struct pdma_buf_mngr *bm = (struct pdma_buf_mngr *)dev->ctrl.buf_mngr;
    volatile TX_DCB_t *ring = (volatile TX_DCB_t *)txq->ring;
    uint32_t di;

    /* Go through all the descriptors and free pktbuf */
    for (di = 0; di < txq->nb_desc; di++) {
        if (txq->pbuf[di].dma) {
            bm->tx_buf_free(dev, txq, &txq->pbuf[di]);
        }
        cmicr_tx_desc_config(&ring[di], 0, 0, 0);
    }

    txq->curr = 0;
    txq->dirt = 0;
    txq->halt = 0;

    return SHR_E_NONE;
}

/*!
 * Process Rx vring
 */
static int
cmicr_pdma_rx_vring_process(struct pdma_hw *hw, struct pdma_rx_queue *rxq,
                            struct pdma_rx_buf *pbuf)
{
    struct pdma_dev *dev = hw->dev;
    volatile RX_DCB_t *ring = (volatile RX_DCB_t *)rxq->ring;
    struct pdma_rx_queue *vrxq = NULL;
    volatile RX_DCB_t *vring = NULL;
    struct pkt_hdr *pkh = &pbuf->pkb->pkh;
    uint64_t buf_addr;

    vrxq = (struct pdma_rx_queue *)dev->ctrl.vnet_rxq[rxq->queue_id];
    vring = (volatile RX_DCB_t *)vrxq->ring;
    if (!vring) {
        rxq->stats.dropped++;
        return SHR_E_UNAVAIL;
    }

    if (RX_DCB_DONEf_GET(vring[vrxq->curr])) {
        dev->xnet_wake(dev);
        return SHR_E_BUSY;
    }

    /* Copy descriptor and packet to vring */
    buf_addr = BUS_TO_DMA_HI(RX_DCB_ADDR_HIf_GET(vring[vrxq->curr]));
    buf_addr = buf_addr << 32 | RX_DCB_ADDR_LOf_GET(vring[vrxq->curr]);
    sal_memcpy(dev->sys_p2v(dev, buf_addr), &pbuf->pkb->data,
               pkh->meta_len + pkh->data_len);
    RX_DCB_STATUS_SET(vring[vrxq->curr], RX_DCB_STATUS_GET(ring[rxq->curr]));

    MEMORY_BARRIER;

    /* Notify VNET to process if needed */
    if (!RX_DCB_STATUS_GET(vring[(vrxq->curr + vrxq->nb_desc - 1) % vrxq->nb_desc])) {
        dev->xnet_wake(dev);
    }
    vrxq->curr = (vrxq->curr + 1) % vrxq->nb_desc;

    return SHR_E_NONE;
}

/*!
 * Refill Rx ring
 */
static int
cmicr_pdma_rx_ring_refill(struct pdma_hw *hw, struct pdma_rx_queue *rxq)
{
    struct pdma_dev *dev = hw->dev;
    struct pdma_buf_mngr *bm = (struct pdma_buf_mngr *)dev->ctrl.buf_mngr;
    volatile RX_DCB_t *ring = (volatile RX_DCB_t *)rxq->ring;
    struct pdma_rx_buf *pbuf = NULL;
    int unused = cmicr_pdma_rx_ring_unused(rxq);
    dma_addr_t addr;
    uint32_t halt;
    int rv = SHR_E_NONE;

    for (halt = rxq->halt; halt < rxq->halt + unused; halt++) {
        if (RX_DCB_ADDR_LOf_GET(ring[halt % rxq->nb_desc])) {
            continue;
        }
        pbuf = &rxq->pbuf[halt % rxq->nb_desc];
        /* Allocate a new pktbuf */
        if (!bm->rx_buf_avail(dev, rxq, pbuf)) {
            rv = bm->rx_buf_alloc(dev, rxq, pbuf);
            if (SHR_FAILURE(rv)) {
                rxq->stats.nomems++;
                rxq->halt = halt % rxq->nb_desc;
                CNET_INFO(hw->unit, "%d DCBs not filled, will retry late\n",
                          cmicr_pdma_rx_ring_unused(rxq));
                break;
            }
        }
        /* Setup the new descriptor */
        bm->rx_buf_dma(dev, rxq, pbuf, &addr);
        cmicr_rx_desc_config(&ring[halt % rxq->nb_desc], addr, rxq->buf_size);
        if (dev->flags & PDMA_CHAIN_MODE && halt % rxq->nb_desc == rxq->nb_desc - 1) {
            cmicr_rx_desc_chain(&ring[halt % rxq->nb_desc], 0);
        }
    }
    rxq->halt = halt % rxq->nb_desc;

    /* Move forward */
    sal_spinlock_lock(rxq->lock);
    if (!(rxq->status & PDMA_RX_QUEUE_XOFF)) {
        /* Descriptor cherry pick */
        rxq->halt_addr = rxq->ring_addr + sizeof(RX_DCB_t) * rxq->halt;
        hw->hdls.chan_goto(hw, rxq->chan_id, rxq->halt_addr);
    }
    sal_spinlock_unlock(rxq->lock);

    return rv;
}

/*!
 * \brief Clean Rx ring
 *
 * \param [in] hw HW structure point.
 * \param [in] rxq Rx queue structure point.
 * \param [in] budget Polling budget.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
cmicr_pdma_rx_ring_clean(struct pdma_hw *hw, struct pdma_rx_queue *rxq, int budget)
{
    struct pdma_dev *dev = hw->dev;
    struct pdma_buf_mngr *bm = (struct pdma_buf_mngr *)dev->ctrl.buf_mngr;
    volatile RX_DCB_t *ring = (volatile RX_DCB_t *)rxq->ring;
    struct pdma_rx_buf *pbuf = NULL;
    struct pkt_hdr *pkh = NULL;
    dma_addr_t addr;
    uint32_t curr = rxq->curr;
    int len, done = 0;
    int retry, fill = SHR_E_NONE;
    int rv;

    while (done < budget && !(dev->flags & PDMA_ABORT)) {
        /* Replenish all the unused descriptors in batch mode */
        if (rxq->state & PDMA_RX_BATCH_REFILL &&
            cmicr_pdma_rx_ring_unused(rxq) >= (int)rxq->free_thresh) {
            fill = cmicr_pdma_rx_ring_refill(hw, rxq);
            /* If no one filled, return budget and keep polling */
            if (cmicr_pdma_rx_ring_unused(rxq) == (int)(rxq->nb_desc - 1)) {
                rxq->state |= PDMA_RX_QUEUE_BUSY;
                return budget;
            }
        }

        /* Check new packet */
        if (!RX_DCB_DONEf_GET(ring[curr])) {
            break;
        }

        /* Move forward */
        if (!(rxq->state & PDMA_RX_BATCH_REFILL)) {
            /* coverity[double_lock : FALSE] */
            sal_spinlock_lock(rxq->lock);
            if (!(rxq->status & PDMA_RX_QUEUE_XOFF)) {
                /* Descriptor cherry pick */
                rxq->halt_addr = rxq->ring_addr + sizeof(RX_DCB_t) * curr;
                hw->hdls.chan_goto(hw, rxq->chan_id, rxq->halt_addr);
                rxq->halt = curr;
            }
            sal_spinlock_unlock(rxq->lock);
        }

        /* Check the status */
        if (RX_DCB_ECC_ERRORf_GET(ring[curr]) ||
            RX_DCB_CELL_ERRORf_GET(ring[curr])) {
            rxq->stats.errors++;
            if (RX_DCB_ECC_ERRORf_GET(ring[curr])) {
                rxq->stats.data_errors++;
            }
            if (RX_DCB_CELL_ERRORf_GET(ring[curr])) {
                rxq->stats.cell_errors++;
            }

            CNET_ERROR(hw->unit, "Packet data corrupted, ignore it ...\n");

            /* Update the indicators (no lock required) */
            RX_DCB_STATUS_SET(ring[curr], 0);
            curr = (curr + 1) % rxq->nb_desc;
            rxq->curr = curr;
            done++;
            continue;
        }

        /* Get the current pktbuf to process */
        pbuf = &rxq->pbuf[curr];
        len = RX_DCB_BYTES_TRANSFERREDf_GET(ring[curr]);
        rv = bm->rx_buf_get(dev, rxq, pbuf, len);
        if (SHR_SUCCESS(rv)) {
            /* Setup packet header */
            pkh = &pbuf->pkb->pkh;
            pkh->data_len = len - hw->info.rx_ph_size;
            pkh->meta_len = hw->info.rx_ph_size;
            pkh->queue_id = rxq->queue_id;

            /* Send up the packet */
            rv = dev->pkt_recv(dev, rxq->queue_id, (void *)pbuf->skb);
            if (SHR_FAILURE(rv)) {
                if (dev->mode == DEV_MODE_HNET && pkh->attrs & PDMA_RX_TO_VNET) {
                    rv = cmicr_pdma_rx_vring_process(hw, rxq, pbuf);
                    if (SHR_FAILURE(rv) && rv == SHR_E_BUSY) {
                        rxq->state |= PDMA_RX_QUEUE_BUSY;
                        return done;
                    }
                } else {
                    rxq->stats.dropped++;
                }
                bm->rx_buf_put(dev, rxq, pbuf, len);
            }
        } else if (rv != SHR_E_UNAVAIL) {
            CNET_INFO(hw->unit, "RX buffer not enough, retry ...\n");
            rxq->stats.nomems++;
            /* Set busy state to retry */
            rxq->state |= PDMA_RX_QUEUE_BUSY;
            return budget;
        }

        /* Count the packets/bytes */
        rxq->stats.packets++;
        rxq->stats.bytes += len;

        /* Setup the new descriptor */
        if (!(rxq->state & PDMA_RX_BATCH_REFILL)) {
            if (!bm->rx_buf_avail(dev, rxq, pbuf)) {
                retry = 0;
                while (1) {
                    rv = bm->rx_buf_alloc(dev, rxq, pbuf);
                    if (SHR_SUCCESS(rv)) {
                        break;
                    }
                    rxq->stats.nomems++;
                    if (dev->mode == DEV_MODE_UNET || dev->mode == DEV_MODE_VNET) {
                        if (retry++ < 5000000) {
                            sal_usleep(1);
                            continue;
                        }
                        CNET_ERROR(hw->unit, "Fatal error: can not alloc RX buffer\n");
                    }
                    rxq->state |= PDMA_RX_BATCH_REFILL;
                    rxq->free_thresh = 1;
                    cmicr_rx_desc_config(&ring[curr], 0, 0);
                    CNET_ERROR(hw->unit, "RX buffer alloc failed, try batch refilling later\n");
                    break;
                }
            }
            if (pbuf->dma) {
                bm->rx_buf_dma(dev, rxq, pbuf, &addr);
                cmicr_rx_desc_config(&ring[curr], addr, rxq->buf_size);
                if (dev->flags & PDMA_CHAIN_MODE && curr == rxq->nb_desc - 1) {
                    cmicr_rx_desc_chain(&ring[curr], 0);
                }
            }
        } else {
            cmicr_rx_desc_config(&ring[curr], 0, 0);
        }

        /* Notify HNET to process if needed */
        if (dev->mode == DEV_MODE_VNET) {
            if (RX_DCB_STATUS_GET(ring[(curr + rxq->nb_desc - 1) % rxq->nb_desc]) == 0 &&
                RX_DCB_STATUS_GET(ring[(curr + rxq->nb_desc - 2) % rxq->nb_desc]) != 0) {
                dev->xnet_wake(dev);
            }
        }

        /* Update the indicators */
        curr = (curr + 1) % rxq->nb_desc;
        rxq->curr = curr;
        done++;

        /* Restart DMA if in chain mode */
        if (dev->flags & PDMA_CHAIN_MODE) {
            /* coverity[double_lock : FALSE] */
            sal_spinlock_lock(rxq->lock);
            if (curr == 0 && !(rxq->status & PDMA_RX_QUEUE_XOFF)) {
                hw->hdls.chan_stop(hw, rxq->chan_id);
                hw->hdls.chan_start(hw, rxq->chan_id);
            }
            sal_spinlock_unlock(rxq->lock);
        }
    }

    /* One more poll for chain done in chain mode */
    if (dev->flags & PDMA_CHAIN_MODE) {
        if (curr == rxq->nb_desc - 1 && done) {
            done = budget;
        }
    }

    return SHR_FAILURE(fill) ? budget : done;
}

/*!
 * Process Tx vring
 */
static int
cmicr_pdma_tx_vring_process(struct pdma_hw *hw, struct pdma_tx_queue *txq,
                            struct pdma_tx_buf *pbuf)
{
    struct pdma_dev *dev = hw->dev;
    volatile TX_DCB_t *ring = (volatile TX_DCB_t *)txq->ring;
    struct pdma_tx_queue *vtxq = NULL;
    volatile TX_DCB_t *vring = NULL;

    vtxq = (struct pdma_tx_queue *)dev->ctrl.vnet_txq[txq->queue_id];
    vring = (volatile TX_DCB_t *)vtxq->ring;
    if (!vring) {
        return SHR_E_UNAVAIL;
    }

    /* Update vring descriptor */
    TX_DCB_STATUS_SET(vring[vtxq->dirt], TX_DCB_STATUS_GET(ring[txq->dirt]));
    pbuf->dma = 0;

    MEMORY_BARRIER;

    /* Notify VNET to process if needed */
    if (!TX_DCB_STATUS_GET(vring[(vtxq->dirt + vtxq->nb_desc - 1) % vtxq->nb_desc])) {
        dev->xnet_wake(dev);
    }
    vtxq->dirt = (vtxq->dirt + 1) % vtxq->nb_desc;

    return SHR_E_NONE;
}

/*!
 * \brief Clean Tx ring
 *
 * \param [in] hw HW structure point.
 * \param [in] txq Tx queue structure point.
 * \param [in] budget Polling budget.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
cmicr_pdma_tx_ring_clean(struct pdma_hw *hw, struct pdma_tx_queue *txq, int budget)
{
    struct pdma_dev *dev = hw->dev;
    struct pdma_buf_mngr *bm = (struct pdma_buf_mngr *)dev->ctrl.buf_mngr;
    volatile TX_DCB_t *ring = (volatile TX_DCB_t *)txq->ring;
    uint32_t curr, dirt = txq->dirt;
    int done = 0;

    while (done < budget) {
        if (!txq->pbuf[dirt].dma || !TX_DCB_STATUS_GET(ring[dirt])) {
            break;
        }

        if (dev->mode == DEV_MODE_HNET && !txq->pbuf[dirt].skb) {
            cmicr_pdma_tx_vring_process(hw, txq, &txq->pbuf[dirt]);
        } else {
            /* Free the done pktbuf */
            bm->tx_buf_free(dev, txq, &txq->pbuf[dirt]);
        }

        cmicr_tx_desc_config(&ring[dirt], 0, 0, 0);

        /* Update the indicators */
        dirt = (dirt + 1) % txq->nb_desc;
        txq->dirt = dirt;
        done++;

        /* Restart DMA if in chain mode */
        if (dev->flags & PDMA_CHAIN_MODE) {
            sal_spinlock_lock(txq->lock);
            curr = txq->curr;
            if (dirt == txq->halt && dirt != curr) {
                hw->hdls.chan_stop(hw, txq->chan_id);
                cmicr_tx_desc_chain(&ring[(curr + txq->nb_desc - 1) % txq->nb_desc], 0);
                hw->hdls.chan_setup(hw, txq->chan_id,
                                    txq->ring_addr + sizeof(TX_DCB_t) * txq->halt);
                hw->hdls.chan_start(hw, txq->chan_id);
                txq->halt = curr;
            }
            sal_spinlock_unlock(txq->lock);
        }

        if (dev->flags & PDMA_ABORT) {
            break;
        }
    }

    /* One more poll for chain done in chain mode */
    if (dev->flags & PDMA_CHAIN_MODE) {
        /* coverity[double_lock : FALSE] */
        sal_spinlock_lock(txq->lock);
        if (dirt != txq->halt) {
            done = budget;
        }
        sal_spinlock_unlock(txq->lock);
    }

    /* Set busy state to avoid HW checking */
    if (done == budget) {
        txq->state |= PDMA_TX_QUEUE_BUSY;
    }

    /* Resume Tx if any */
    sal_spinlock_lock(txq->lock);
    if (txq->status & PDMA_TX_QUEUE_XOFF && cmicr_pdma_tx_ring_unused(txq)) {
        txq->status &= ~PDMA_TX_QUEUE_XOFF;
        if (dev->suspended) {
            sal_spinlock_unlock(txq->lock);
            return done;
        }
        if (dev->tx_resume) {
            dev->tx_resume(dev, txq->queue_id);
        }
        sal_spinlock_unlock(txq->lock);
        if (!dev->tx_resume && !(txq->state & PDMA_TX_QUEUE_POLL)) {
            sal_sem_give(txq->sem);
        }
        return done;
    }
    sal_spinlock_unlock(txq->lock);

    return done;
}

/*!
 * Dump Rx ring
 */
static int
cmicr_pdma_rx_ring_dump(struct pdma_hw *hw, struct pdma_rx_queue *rxq)
{
    volatile RX_DCB_t *ring = (volatile RX_DCB_t *)rxq->ring;
    volatile RX_DCB_t *rd;
    uint32_t di;

    CNET_INFO(hw->unit, "RX: queue=%d, chan=%d, curr=%d, halt=%d, halt@%p\n",
              rxq->queue_id, rxq->chan_id, rxq->curr, rxq->halt, (void *)&ring[rxq->halt]);
    CNET_INFO(hw->unit, "----------------------------------------------------------------\n");
    for (di = 0; di < rxq->nb_desc + 1; di++) {
        rd = &ring[di];
        CNET_INFO(hw->unit, "DESC[%03d]: (%p)->%08x %08x %08x %08x\n",
                  di, (void *)(unsigned long)(rxq->ring_addr + di * CMICR_PDMA_DCB_SIZE),
                  RX_DCB_GET(*rd, 0), RX_DCB_GET(*rd, 1),
                  RX_DCB_GET(*rd, 2), RX_DCB_GET(*rd, 3));
    }

    return SHR_E_NONE;
}

/*!
 * Dump Tx ring
 */
static int
cmicr_pdma_tx_ring_dump(struct pdma_hw *hw, struct pdma_tx_queue *txq)
{
    volatile TX_DCB_t *ring = (volatile TX_DCB_t *)txq->ring;
    volatile TX_DCB_t *td;
    uint32_t di;

    CNET_INFO(hw->unit, "TX: queue=%d, chan=%d, curr=%d, dirt=%d, halt@%p\n",
              txq->queue_id, txq->chan_id, txq->curr, txq->dirt, (void *)&ring[txq->curr]);
    CNET_INFO(hw->unit, "----------------------------------------------------------------\n");
    for (di = 0; di < txq->nb_desc + 1; di++) {
        td = &ring[di];
        CNET_INFO(hw->unit, "DESC[%03d]: (%p)->%08x %08x %08x %08x\n",
                  di, (void *)(unsigned long)(txq->ring_addr + di * CMICR_PDMA_DCB_SIZE),
                  TX_DCB_GET(*td, 0), TX_DCB_GET(*td, 1),
                  TX_DCB_GET(*td, 2), TX_DCB_GET(*td, 3));
    }

    return SHR_E_NONE;
}

/*!
 * Fetch Tx vring
 */
static int
cmicr_pdma_tx_vring_fetch(struct pdma_hw *hw, struct pdma_tx_queue *txq,
                          struct pdma_tx_buf *pbuf)
{
    struct pdma_dev *dev = hw->dev;
    volatile TX_DCB_t *ring = (volatile TX_DCB_t *)txq->ring;
    struct pdma_tx_queue *vtxq = NULL;
    volatile TX_DCB_t *vring = NULL;
    uint32_t rm;

    vtxq = (struct pdma_tx_queue *)dev->ctrl.vnet_txq[txq->queue_id];
    vring = (volatile TX_DCB_t *)vtxq->ring;
    if (!vring || !TX_DCB_BYTE_COUNTf_GET(vring[vtxq->curr])) {
        return SHR_E_UNAVAIL;
    }

    /* Fetch vring descriptor */
    rm = TX_DCB_DESC_REMAINf_GET(ring[txq->curr]);
    TX_DCB_SET(ring[txq->curr], 0, TX_DCB_GET(vring[vtxq->curr], 0));
    TX_DCB_SET(ring[txq->curr], 1, TX_DCB_GET(vring[vtxq->curr], 1));
    TX_DCB_SET(ring[txq->curr], 2, TX_DCB_GET(vring[vtxq->curr], 2));
    TX_DCB_SET(ring[txq->curr], 3, TX_DCB_GET(vring[vtxq->curr], 3));
    TX_DCB_DESC_REMAINf_SET(ring[txq->curr], rm);
    TX_DCB_BYTE_COUNTf_SET(vring[vtxq->curr], 0);

    MEMORY_BARRIER;

    pbuf->dma = TX_DCB_ADDR_LOf_GET(vring[vtxq->curr]);
    pbuf->len = TX_DCB_BYTE_COUNTf_GET(ring[txq->curr]);
    vtxq->curr = (vtxq->curr + 1) % vtxq->nb_desc;

    return SHR_E_NONE;
}

/*!
 * Check Tx ring
 */
static inline int
cmicr_pdma_tx_ring_check(struct pdma_hw *hw, struct pdma_tx_queue *txq)
{
    struct pdma_dev *dev = hw->dev;

    if (dev->suspended) {
        txq->stats.xoffs++;
        if (dev->tx_suspend) {
            dev->tx_suspend(dev, txq->queue_id);
            return SHR_E_BUSY;
        }
        if (!(txq->state & PDMA_TX_QUEUE_POLL)) {
            return SHR_E_BUSY;
        }
    }

    if (cmicr_pdma_tx_ring_unused(txq)) {
        return SHR_E_NONE;
    }

    sal_spinlock_lock(txq->lock);
    if (!cmicr_pdma_tx_ring_unused(txq)) {
        txq->status |= PDMA_TX_QUEUE_XOFF;
        txq->stats.xoffs++;
        if (dev->tx_suspend) {
            dev->tx_suspend(dev, txq->queue_id);
        }
        sal_spinlock_unlock(txq->lock);
        return SHR_E_BUSY;
    }
    sal_spinlock_unlock(txq->lock);

    return SHR_E_NONE;
}

/*!
 * \brief Start packet transmission
 *
 * \param [in] hw HW structure point.
 * \param [in] txq Tx queue structure point.
 * \param [in] buf Tx packet buffer.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
static int
cmicr_pdma_pkt_xmit(struct pdma_hw *hw, struct pdma_tx_queue *txq, void *buf)
{
    struct pdma_dev *dev = hw->dev;
    struct pdma_buf_mngr *bm = (struct pdma_buf_mngr *)dev->ctrl.buf_mngr;
    volatile TX_DCB_t *ring = (volatile TX_DCB_t *)txq->ring;
    struct pdma_tx_buf *pbuf = NULL;
    struct pkt_hdr *pkh = NULL;
    dma_addr_t addr;
    uint32_t curr;
    int retry = 5000000;
    int rv;

    if (dev->tx_suspend) {
        sal_spinlock_lock(txq->mutex);
    } else {
        rv = sal_sem_take(txq->sem, BCMCNET_TX_RSRC_WAIT_USEC);
        if (rv == -1) {
            CNET_ERROR(hw->unit, "Timeout waiting for Tx resources\n");
            return SHR_E_TIMEOUT;
        }
    }

    /* Check Tx resource */
    if (dev->tx_suspend) {
        /* Suspend Tx if no resource */
        rv = cmicr_pdma_tx_ring_check(hw, txq);
        if (SHR_FAILURE(rv)) {
            sal_spinlock_unlock(txq->mutex);
            return rv;
        }
    } else {
        /* Abort Tx if a fatal error happened */
        if (txq->status & PDMA_TX_QUEUE_XOFF) {
            sal_sem_give(txq->sem);
            return SHR_E_RESOURCE;
        }
    }

    /* Setup the new descriptor */
    curr = txq->curr;
    pbuf = &txq->pbuf[curr];
    if (dev->mode == DEV_MODE_HNET && !buf) {
        rv = cmicr_pdma_tx_vring_fetch(hw, txq, pbuf);
        if (SHR_FAILURE(rv)) {
            sal_spinlock_unlock(txq->mutex);
            return SHR_E_EMPTY;
        }
        txq->state |= PDMA_TX_QUEUE_BUSY;
    } else {
        rv = bm->tx_buf_get(dev, txq, pbuf, buf);
        if (SHR_FAILURE(rv)) {
            txq->stats.dropped++;
            if (dev->tx_suspend) {
                sal_spinlock_unlock(txq->mutex);
            } else {
                sal_sem_give(txq->sem);
            }
            return SHR_E_RESOURCE;
        }
        bm->tx_buf_dma(dev, txq, pbuf, &addr);
        pkh = &pbuf->pkb->pkh;
        cmicr_tx_desc_config(&ring[curr], addr, pbuf->len, pkh->attrs);
    }

    /* Notify HNET to process if needed */
    if (dev->mode == DEV_MODE_VNET) {
        if (!TX_DCB_BYTE_COUNTf_GET(ring[(curr + txq->nb_desc - 1) % txq->nb_desc])) {
            dev->xnet_wake(dev);
        }
    }

    /* Update the indicators */
    curr = (curr + 1) % txq->nb_desc;
    txq->curr = curr;

    /* Start DMA if in chain mode */
    if (dev->flags & PDMA_CHAIN_MODE) {
        if (txq->state & PDMA_TX_QUEUE_POLL) {
            do {
                rv = cmicr_pdma_tx_ring_clean(hw, txq, txq->nb_desc - 1);
                if (rv != (int)txq->nb_desc - 1) {
                    break;
                }
                sal_usleep(1);
            } while (retry--);
            if (retry < 0) {
                CNET_ERROR(hw->unit, "Last Tx could not get done in given time\n");
            }
        }
        sal_spinlock_lock(txq->lock);
        if (txq->dirt == txq->halt && txq->dirt != curr) {
            hw->hdls.chan_stop(hw, txq->chan_id);
            cmicr_tx_desc_chain(&ring[(curr + txq->nb_desc - 1) % txq->nb_desc], 0);
            hw->hdls.chan_setup(hw, txq->chan_id,
                                txq->ring_addr + sizeof(TX_DCB_t) * txq->halt);
            hw->hdls.chan_start(hw, txq->chan_id);
            txq->halt = curr;
        }
        sal_spinlock_unlock(txq->lock);
    }

    /* Kick off DMA */
    txq->halt_addr = txq->ring_addr + sizeof(TX_DCB_t) * curr;
    hw->hdls.chan_goto(hw, txq->chan_id, txq->halt_addr);

    /* Count the packets/bytes */
    txq->stats.packets++;
    txq->stats.bytes += pbuf->len;

    /* Clean up ring if in polling mode */
    if (txq->state & PDMA_TX_QUEUE_POLL &&
        cmicr_pdma_tx_ring_unused(txq) <= (int)txq->free_thresh) {
        cmicr_pdma_tx_ring_clean(hw, txq, dev->ctrl.budget);
    }

    /* Suspend Tx if no resource */
    rv = cmicr_pdma_tx_ring_check(hw, txq);
    if (SHR_FAILURE(rv)) {
        if (dev->mode == DEV_MODE_VNET) {
            dev->xnet_wake(dev);
        }

        if (txq->state & PDMA_TX_QUEUE_POLL) {
            /* In polling mode, must wait till the ring is available */
            do {
                cmicr_pdma_tx_ring_clean(hw, txq, dev->ctrl.budget);
                if (!(txq->status & PDMA_TX_QUEUE_XOFF) ||
                    !(txq->state & PDMA_TX_QUEUE_ACTIVE)) {
                    break;
                }
                sal_usleep(1);
            } while (retry--);
            if (retry < 0) {
                CNET_ERROR(hw->unit, "Fatal error: Tx ring is full, packets can not been transmitted\n");
                if (!dev->tx_suspend) {
                    sal_sem_give(txq->sem);
                    return SHR_E_RESOURCE;
                }
            }
        } else {
            /* In interrupt mode, the handle thread will wake up Tx */
            if (!dev->tx_suspend) {
                return SHR_E_NONE;
            }
        }
    }

    if (dev->tx_suspend) {
        sal_spinlock_unlock(txq->mutex);
    } else {
        sal_sem_give(txq->sem);
    }

    return SHR_E_NONE;
}

/*!
 * Suspend Rx queue
 */
static int
cmicr_pdma_rx_suspend(struct pdma_hw *hw, struct pdma_rx_queue *rxq)
{
    sal_spinlock_lock(rxq->lock);
    rxq->status |= PDMA_RX_QUEUE_XOFF;
    if (hw->dev->flags & PDMA_CHAIN_MODE) {
        hw->hdls.chan_stop(hw, rxq->chan_id);
    }
    sal_spinlock_unlock(rxq->lock);

    return SHR_E_NONE;
}

/*!
 * Resume Rx queue
 */
static int
cmicr_pdma_rx_resume(struct pdma_hw *hw, struct pdma_rx_queue *rxq)
{
    volatile RX_DCB_t *ring = (volatile RX_DCB_t *)rxq->ring;

    sal_spinlock_lock(rxq->lock);
    if (!(rxq->status & PDMA_RX_QUEUE_XOFF)) {
        sal_spinlock_unlock(rxq->lock);
        return SHR_E_NONE;
    }
    if (rxq->state & PDMA_RX_BATCH_REFILL) {
        rxq->halt_addr = rxq->ring_addr + sizeof(RX_DCB_t) * rxq->halt;
        hw->hdls.chan_goto(hw, rxq->chan_id, rxq->halt_addr);
    } else if (!RX_DCB_STATUS_GET(ring[(rxq->curr + 1) % rxq->nb_desc])) {
        rxq->halt = (rxq->curr + rxq->nb_desc - 1) % rxq->nb_desc;
        rxq->halt_addr = rxq->ring_addr + sizeof(RX_DCB_t) * rxq->halt;
        hw->hdls.chan_goto(hw, rxq->chan_id, rxq->halt_addr);
    }
    if (hw->dev->flags & PDMA_CHAIN_MODE) {
        rxq->curr = 0;
        hw->hdls.chan_start(hw, rxq->chan_id);
    }
    rxq->status &= ~PDMA_RX_QUEUE_XOFF;
    sal_spinlock_unlock(rxq->lock);

    return SHR_E_NONE;
}

/*!
 * Initialize function pointers
 */
int
bcmcnet_cmicr_pdma_desc_ops_init(struct pdma_hw *hw)
{
    if (!hw) {
        return SHR_E_PARAM;
    }

    hw->dops.rx_desc_init = cmicr_pdma_rx_desc_init;
    hw->dops.rx_desc_clean = cmicr_pdma_rx_desc_clean;
    hw->dops.rx_ring_clean = cmicr_pdma_rx_ring_clean;
    hw->dops.rx_ring_dump = cmicr_pdma_rx_ring_dump;
    hw->dops.rx_suspend = cmicr_pdma_rx_suspend;
    hw->dops.rx_resume = cmicr_pdma_rx_resume;
    hw->dops.tx_desc_init = cmicr_pdma_tx_desc_init;
    hw->dops.tx_desc_clean = cmicr_pdma_tx_desc_clean;
    hw->dops.tx_ring_clean = cmicr_pdma_tx_ring_clean;
    hw->dops.tx_ring_dump = cmicr_pdma_tx_ring_dump;
    hw->dops.pkt_xmit = cmicr_pdma_pkt_xmit;

    return SHR_E_NONE;
}

/*!
 * Attach device driver
 */
int
bcmcnet_cmicr_pdma_driver_attach(struct pdma_dev *dev)
{
    struct pdma_hw *hw = NULL;

    /* Allocate memory for HW data */
    hw = sal_alloc(sizeof(*hw), "bcmcnetPdmaHw");
    if (!hw) {
        return SHR_E_MEMORY;
    }
    sal_memset(hw, 0, sizeof(*hw));
    hw->unit = dev->unit;
    hw->dev = dev;
    dev->ctrl.hw = hw;

    bcmcnet_cmicr_pdma_hw_hdls_init(hw);
    bcmcnet_cmicr_pdma_desc_ops_init(hw);

    return SHR_E_NONE;
}

/*!
 * Detach device driver
 */
int
bcmcnet_cmicr_pdma_driver_detach(struct pdma_dev *dev)
{
    if (dev->ctrl.hw) {
        sal_free(dev->ctrl.hw);
    }
    dev->ctrl.hw = NULL;

    return SHR_E_NONE;
}
