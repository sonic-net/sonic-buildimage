/*! \file bcmcnet_cmicr.h
 *
 * CMICr registers and descriptors definitions.
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

#ifndef BCMCNET_CMICR_H
#define BCMCNET_CMICR_H

#include <bcmcnet/bcmcnet_cmicr_acc.h>

/*!
 * \name CMICR PDMA HW definitions
 */
/*! \{ */
/*! CMICR CMC number */
#define CMICR_PDMA_CMC_MAX              2
/*! CMICR CMC PDMA channels */
#define CMICR_PDMA_CMC_CHAN             16
/*! CMICR PDMA DCB size */
#define CMICR_PDMA_DCB_SIZE             RX_DCB_SIZE
/*! \} */

/*!
 * \name CMICR PCIe device address definitions
 */
/*! \{ */
/*! CMICR PCIE offset */
#define CMICR_PCIE_SO_OFFSET            0x10000000
/*! Higher DMA address to bus address */
#define DMA_TO_BUS_HI(dma)              ((dma) | CMICR_PCIE_SO_OFFSET)
/*! Higher bus address to DMA address */
#define BUS_TO_DMA_HI(bus)              ((bus) & ~CMICR_PCIE_SO_OFFSET)
/*! \} */


/*! \} */
/*!
 * \name CMICR PDMA register address
 */
/*! \{ */
/*! Base address */
#define CMICR_GRP_BASE(g)               (0x00000000 + 0x2000 * g)
/*! Control register address */
#define CMICR_PDMA_CTRL(g, q)           (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_CTRLr_OFFSET + q * 0x80)
/*! Descriptor Address Lower register address */
#define CMICR_PDMA_DESC_LO(g, q)        (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_DESC_ADDR_LOr_OFFSET + q * 0x80)
/*! Descriptor Address Higher register address */
#define CMICR_PDMA_DESC_HI(g, q)        (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_DESC_ADDR_HIr_OFFSET + q * 0x80)
/*! Descriptor Halt Address Lower register address */
#define CMICR_PDMA_DESC_HALT_LO(g, q)   (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_DESC_HALT_ADDR_LOr_OFFSET + q * 0x80)
/*! Descriptor Halt Address Higher register address */
#define CMICR_PDMA_DESC_HALT_HI(g, q)   (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_DESC_HALT_ADDR_HIr_OFFSET + q * 0x80)
/*! Status register address */
#define CMICR_PDMA_STAT(g, q)           (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_STATr_OFFSET + q * 0x80)
/*! Interrupt status register address */
#define CMICR_PDMA_INTR_STAT(g, q)      (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_INTRr_OFFSET + q * 0x80)
/*! Interrupt enable register address */
#define CMICR_PDMA_INTR_ENAB(g, q)      (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_INTR_ENABLEr_OFFSET + q * 0x80)
/*! Interrupt clear register address */
#define CMICR_PDMA_INTR_CLR(g, q)       (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_INTR_CLRr_OFFSET + q * 0x80)
/*! COS Control Rx0 register address */
#define CMICR_PDMA_COS_CTRL_RX0(g, q)   (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_COS_CTRL_RX_0r_OFFSET + q * 0x80)
/*! COS Control Rx1 register address */
#define CMICR_PDMA_COS_CTRL_RX1(g, q)   (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_COS_CTRL_RX_1r_OFFSET + q * 0x80)
/*! Interrupt Coalesce register address */
#define CMICR_PDMA_INTR_COAL(g, q)      (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_INTR_COALr_OFFSET + q * 0x80)
/*! Current Descriptor Address Lower register address */
#define CMICR_PDMA_CURR_DESC_LO(g, q)   (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_CURR_DESC_LOr_OFFSET + q * 0x80)
/*! Current Descriptor Address Higher register address */
#define CMICR_PDMA_CURR_DESC_HI(g, q)   (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_CURR_DESC_HIr_OFFSET + q * 0x80)
/*! Rx Buffer Threshhold register address */
#define CMICR_PDMA_RBUF_THRE(g, q)      (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_RXBUF_THRESHOLD_CONFIGr_OFFSET + q * 0x80)
/*! Debug Control register address */
#define CMICR_PDMA_DEBUG_CTRL(g, q)     (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_DEBUG_CONTROLr_OFFSET + q * 0x80)
/*! Debug State Machine Status register address */
#define CMICR_PDMA_DEBUG_SM_STAT(g, q)  (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_DEBUG_SM_STATUSr_OFFSET + q * 0x80)
/*! Debug Status register address */
#define CMICR_PDMA_DEBUG_STAT(g, q)     (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_DEBUG_STATUSr_OFFSET + q * 0x80)
/*! Rx Packet Count register address */
#define CMICR_PDMA_COUNT_RX(g, q)       (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_PKT_COUNT_RXPKTr_OFFSET + q * 0x80)
/*! Tx Packet Count register address */
#define CMICR_PDMA_COUNT_TX(g, q)       (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_PKT_COUNT_TXPKTr_OFFSET + q * 0x80)
/*! Dropped Rx Packet Count register address */
#define CMICR_PDMA_COUNT_RX_DROP(g, q)  (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_PKT_COUNT_RXPKT_DROPr_OFFSET + q * 0x80)
/*! Requested Descriptor Count register address */
#define CMICR_PDMA_DESC_CNT_REQ(g, q)   (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_DESC_COUNT_REQr_OFFSET + q * 0x80)
/*! Received Descriptor Count register address */
#define CMICR_PDMA_DESC_CNT_RX(g, q)    (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_DESC_COUNT_RXr_OFFSET + q * 0x80)
/*! Updated Descriptor Count register address */
#define CMICR_PDMA_DESC_CNT_STAT(g, q)  (CMICR_GRP_BASE(g) + CMIC_CMC_PKTDMA_DESC_COUNT_STATUS_WRr_OFFSET + q * 0x80)
/*! EP_TO_CPU Header Size register address */
#define CMICR_EP_TO_CPU_HEADER_SIZE     CMIC_TOP_STATUS_EP_TO_CPU_HEADER_SIZEr_OFFSET
/*! Top config register address */
#define CMICR_TOP_CONFIG                CMIC_TOP_CONFIGr_OFFSET
/*! iProc interrupt enable set register address0 */
#define PAXB_PDMA_IRQ_ENAB_SET0         PAXB_0_INTC_SET_INTR_ENABLE_REG5r_OFFSET
/*! iProc interrupt enable set register address1 */
#define PAXB_PDMA_IRQ_ENAB_SET1         PAXB_0_INTC_SET_INTR_ENABLE_REG6r_OFFSET
/*! iProc interrupt enable clear register address0 */
#define PAXB_PDMA_IRQ_ENAB_CLR0         PAXB_0_INTC_CLEAR_INTR_ENABLE_REG5r_OFFSET
/*! iProc interrupt enable clear register address1 */
#define PAXB_PDMA_IRQ_ENAB_CLR1         PAXB_0_INTC_CLEAR_INTR_ENABLE_REG6r_OFFSET
/*! \} */

/*!
 * \name Interrupt status & clear register definitions
 */
/*! \{ */
/*! Interrupt mask shift */
#define CMICR_IRQ_MASK_SHIFT            8
/*! \} */

/*! 32-bit register read */
#define DEV_READ32(_c, _a, _p) \
    do { \
        if ((_c)->dev->mode != DEV_MODE_VNET) { \
            *(_p) = ((volatile uint32_t *)(_c)->hw_addr)[(_a) / 4]; \
        } \
    } while (0)

/*! 32-bit register write */
#define DEV_WRITE32(_c, _a, _v) \
    do { \
        if ((_c)->dev->mode != DEV_MODE_VNET) { \
            ((volatile uint32_t *)(_c)->hw_addr)[(_a) / 4] = (_v); \
        } \
    } while (0)

/*! Tx packet header size */
#define CMICR_TX_PKT_HDR_SIZE           16

/*! HW access retry times */
#define CMICR_HW_RETRY_TIMES            100000

/*! Max remaining descriptors */
#define CMICR_DESC_REMAIN_MAX           63

/*!
 * \brief Initialize HW handles.
 *
 * \param [in] hw HW structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_cmicr_pdma_hw_hdls_init(struct pdma_hw *hw);

/*!
 * \brief Initialize descriptor operations.
 *
 * \param [in] hw HW structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_cmicr_pdma_desc_ops_init(struct pdma_hw *hw);

/*!
 * \brief Attach device driver.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_cmicr_pdma_driver_attach(struct pdma_dev *dev);

/*!
 * \brief Detach device driver.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_cmicr_pdma_driver_detach(struct pdma_dev *dev);

#endif /* BCMCNET_CMICR_H */
