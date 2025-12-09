/*! \file bcmcnet_cmicx.h
 *
 * CMICx registers and descriptors definitions.
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

#ifndef BCMCNET_CMICX_H
#define BCMCNET_CMICX_H

/*!
 * \name CMICX PDMA HW definitions
 */
/*! \{ */
/*! CMICX CMC number */
#define CMICX_PDMA_CMC_MAX              2
/*! CMICX CMC PDMA channels */
#define CMICX_PDMA_CMC_CHAN             8
/*! CMICX PDMA DCB size */
#define CMICX_PDMA_DCB_SIZE             16
/*! \} */

/*!
 * \name CMICX PCIe device address definitions
 */
/*! \{ */
/*! CMICX PCIE offset */
#define CMICX_PCIE_SO_OFFSET            0x10000000
/*! Higher DMA address to bus address */
#define DMA_TO_BUS_HI(dma)              ((dma) | CMICX_PCIE_SO_OFFSET)
/*! Higher bus address to DMA address */
#define BUS_TO_DMA_HI(bus)              ((bus) & ~CMICX_PCIE_SO_OFFSET)
/*! \} */

/*!
 * \name CMICX PDMA register definitions
 */
/*! \{ */
#define CMICX_PDMA_CTRLr                0x2100
#define CMICX_PDMA_STATr                0x2114
#define CMICX_PDMA_DESC_LOr             0x2104
#define CMICX_PDMA_DESC_HIr             0x2108
#define CMICX_PDMA_CURR_DESC_LOr        0x2124
#define CMICX_PDMA_CURR_DESC_HIr        0x2128
#define CMICX_PDMA_DESC_HALT_LOr        0x210c
#define CMICX_PDMA_DESC_HALT_HIr        0x2110
#define CMICX_PDMA_COS_CTRL_RX0r        0x2118
#define CMICX_PDMA_COS_CTRL_RX1r        0x211c
#define CMICX_PDMA_INTR_COALr           0x2120
#define CMICX_PDMA_RBUF_THREr           0x212c
#define CMICX_PDMA_DEBUG_CTRLr          0x2130
#define CMICX_PDMA_DEBUG_SM_STATr       0x2134
#define CMICX_PDMA_DEBUG_STATr          0x2138
#define CMICX_PDMA_COUNT_RXr            0x213c
#define CMICX_PDMA_COUNT_TXr            0x2140
#define CMICX_PDMA_COUNT_RX_DROPr       0x2144
#define CMICX_PDMA_DESC_CNT_REQr        0x2148
#define CMICX_PDMA_DESC_CNT_RXr         0x214c
#define CMICX_PDMA_DESC_CNT_STATr       0x2150
#define CMICX_PDMA_IRQ_STATr            0x106c
#define CMICX_PDMA_IRQ_STAT_CLRr        0x1074
/*! \} */

/*!
 * \name CMICX PDMA register address
 */
/*! \{ */
/*! Base address */
#define CMICX_GRP_BASE(g)               (0x00000000 + 0x3000 * g)
/*! Control register address */
#define CMICX_PDMA_CTRL(g, q)           (CMICX_GRP_BASE(g) + CMICX_PDMA_CTRLr + q * 0x80)
/*! Status register address */
#define CMICX_PDMA_STAT(g, q)           (CMICX_GRP_BASE(g) + CMICX_PDMA_STATr + q * 0x80)
/*! Descriptor Address Lower register address */
#define CMICX_PDMA_DESC_LO(g, q)        (CMICX_GRP_BASE(g) + CMICX_PDMA_DESC_LOr + q * 0x80)
/*! Descriptor Address Higher register address */
#define CMICX_PDMA_DESC_HI(g, q)        (CMICX_GRP_BASE(g) + CMICX_PDMA_DESC_HIr + q * 0x80)
/*! Current Descriptor Address Lower register address */
#define CMICX_PDMA_CURR_DESC_LO(g, q)   (CMICX_GRP_BASE(g) + CMICX_PDMA_CURR_DESC_LOr + q * 0x80)
/*! Current Descriptor Address Higher register address */
#define CMICX_PDMA_CURR_DESC_HI(g, q)   (CMICX_GRP_BASE(g) + CMICX_PDMA_CURR_DESC_HIr + q * 0x80)
/*! Descriptor Halt Address Lower register address */
#define CMICX_PDMA_DESC_HALT_LO(g, q)   (CMICX_GRP_BASE(g) + CMICX_PDMA_DESC_HALT_LOr + q * 0x80)
/*! Descriptor Halt Address Higher register address */
#define CMICX_PDMA_DESC_HALT_HI(g, q)   (CMICX_GRP_BASE(g) + CMICX_PDMA_DESC_HALT_HIr + q * 0x80)
/*! COS Control Rx0 register address */
#define CMICX_PDMA_COS_CTRL_RX0(g, q)   (CMICX_GRP_BASE(g) + CMICX_PDMA_COS_CTRL_RX0r + q * 0x80)
/*! COS Control Rx1 register address */
#define CMICX_PDMA_COS_CTRL_RX1(g, q)   (CMICX_GRP_BASE(g) + CMICX_PDMA_COS_CTRL_RX1r + q * 0x80)
/*! Interrupt Coalesce register address */
#define CMICX_PDMA_INTR_COAL(g, q)      (CMICX_GRP_BASE(g) + CMICX_PDMA_INTR_COALr + q * 0x80)
/*! Rx Buffer Threshhold register address */
#define CMICX_PDMA_RBUF_THRE(g, q)      (CMICX_GRP_BASE(g) + CMICX_PDMA_RBUF_THREr + q * 0x80)
/*! Debug Control register address */
#define CMICX_PDMA_DEBUG_CTRL(g, q)     (CMICX_GRP_BASE(g) + CMICX_PDMA_DEBUG_CTRLr + q * 0x80)
/*! Debug Status register address */
#define CMICX_PDMA_DEBUG_STAT(g, q)     (CMICX_GRP_BASE(g) + CMICX_PDMA_DEBUG_STATr + q * 0x80)
/*! Debug State Machine Status register address */
#define CMICX_PDMA_DEBUG_SM_STAT(g, q)  (CMICX_GRP_BASE(g) + CMICX_PDMA_DEBUG_SM_STATr + q * 0x80)
/*! Rx Packet Count register address */
#define CMICX_PDMA_COUNT_RX(g, q)       (CMICX_GRP_BASE(g) + CMICX_PDMA_COUNT_RXr + q * 0x80)
/*! Tx Packet Count register address */
#define CMICX_PDMA_COUNT_TX(g, q)       (CMICX_GRP_BASE(g) + CMICX_PDMA_COUNT_TXr + q * 0x80)
/*! Dropped Rx Packet Count register address */
#define CMICX_PDMA_COUNT_RX_DROP(g, q)  (CMICX_GRP_BASE(g) + CMICX_PDMA_COUNT_RX_DROPr + q * 0x80)
/*! Requested Descriptor Count register address */
#define CMICX_PDMA_DESC_CNT_REQ(g, q)   (CMICX_GRP_BASE(g) + CMICX_PDMA_DESC_CNT_REQr + q * 0x80)
/*! Received Descriptor Count register address */
#define CMICX_PDMA_DESC_CNT_RX(g, q)    (CMICX_GRP_BASE(g) + CMICX_PDMA_DESC_CNT_RXr + q * 0x80)
/*! Updated Descriptor Count register address */
#define CMICX_PDMA_DESC_CNT_STAT(g, q)  (CMICX_GRP_BASE(g) + CMICX_PDMA_DESC_CNT_STATr + q * 0x80)
/*! Interrupt Status register address */
#define CMICX_PDMA_IRQ_STAT(g)          (CMICX_GRP_BASE(g) + CMICX_PDMA_IRQ_STATr)
/*! Interrupt Status Clear register address */
#define CMICX_PDMA_IRQ_STAT_CLR(g)      (CMICX_GRP_BASE(g) + CMICX_PDMA_IRQ_STAT_CLRr)
/*! Interrupt Enable register address0 */
#define CMICX_PDMA_IRQ_ENAB0            0x18013100
/*! Interrupt Enable register address1 */
#define CMICX_PDMA_IRQ_ENAB1            0x18013104
/*! Interrupt Enable register address2 */
#define CMICX_PDMA_IRQ_ENAB2            0x18013108
/*! Interrupt raw status register address0 */
#define CMICX_PDMA_IRQ_RAW_STAT0        0x18013150
/*! Interrupt raw status register address1 */
#define CMICX_PDMA_IRQ_RAW_STAT1        0x18013154
/*! Interrupt raw status register address2 */
#define CMICX_PDMA_IRQ_RAW_STAT2        0x18013158
/*! EP_TO_CPU Header Size register address */
#define CMICX_EP_TO_CPU_HEADER_SIZE     0x00000004
/*! Top config register address */
#define CMICX_TOP_CONFIG                0x00000008
/*! Credits release register address */
#define CMICX_EPINTF_RELEASE_CREDITS    0x0000006c
/*! Max credits register address */
#define CMICX_EPINTF_MAX_CREDITS        0x00000070
/*! \} */

/*!
 * \name Control register definitions
 */
/*! \{ */
/*! Disable abort on error */
#define CMICX_PDMA_NO_ABORT_ON_ERR      0x00002000
/*! EP_TO_CPU header big endianess */
#define CMICX_PDMA_HDR_BIG_ENDIAN       0x00001000
/*! Continuous descriptor mode */
#define CMICX_PDMA_CONTINUOUS_DESC      0x00000200
/*! Continuous DMA mode */
#define CMICX_PDMA_CONTINUOUS           0x00000100
/*! Interrupt after descriptor */
#define CMICX_PDMA_INTR_ON_DESC         0x00000080
/*! Update status on reload */
#define CMICX_PDMA_RLD_STAT_DIS         0x00000040
/*! Dropped on chain end */
#define CMICX_PDMA_DROP_ON_END          0x00000020
/*! Descriptor big endianess */
#define CMICX_PDMA_DESC_BIG_ENDIAN      0x00000010
/*! Packet DMA big endianess */
#define CMICX_PDMA_PKT_BIG_ENDIAN       0x00000008
/*! Abort DMA */
#define CMICX_PDMA_ABORT                0x00000004
/*! Enable DMA */
#define CMICX_PDMA_ENABLE               0x00000002
/*! DMA direction */
#define CMICX_PDMA_DIR                  0x00000001
/*! EP_TO_CPU header alignment bytes */
#define CMICX_PDMA_HDR_ALMNT(bytes)     (((bytes) & 0x3) << 10)
/*! \} */

/*!
 * \name Status register definitions
 */
/*! \{ */
/*! Channel in halt */
#define CMICX_PDMA_IN_HALT              0x00000040
/*! Channel active */
#define CMICX_PDMA_IS_ACTIVE            0x00000002
/*! Chain done */
#define CMICX_PDMA_CHAIN_DONE           0x00000001
/*! \} */

/*!
 * \name Interrupt_coalesce register definitions
 */
/*! \{ */
/*! Interrupt coalesce enable */
#define CMICX_PDMA_INTR_COAL_ENA        (1 << 31)
/*! Interrupt coalesce threshhold */
#define CMICX_PDMA_INTR_THRESH(cnt)     (((cnt) & 0x7fff) << 16)
/*! Interrupt coalesce timeout */
#define CMICX_PDMA_INTR_TIMER(tmr)      ((tmr) & 0xffff)
/*! \} */

/*!
 * \name Interrupt status&clear register definitions
 */
/*! \{ */
/*! Descriptor done */
#define CMICX_PDMA_IRQ_DESC_DONE(q)     (0x00000001 << ((q) * 4))
/*! Chain done */
#define CMICX_PDMA_IRQ_CHAIN_DONE(q)    (0x00000002 << ((q) * 4))
/*! Coalescing interrupt */
#define CMICX_PDMA_IRQ_COALESCE_INTR(q) (0x00000004 << ((q) * 4))
/*! Controlled interrupt */
#define CMICX_PDMA_IRQ_CTRLD_INTR(q)    (0x00000008 << ((q) * 4))
/*! Interrupt mask */
#define CMICX_PDMA_IRQ_MASK(q)          (0xf << ((q) * 4))
/*! Interrupt start number */
#define CMICX_IRQ_START_NUM             (128 + 3)
/*! Interrupt number offset */
#define CMICX_IRQ_NUM_OFFSET            4
/*! Interrupt mask shift */
#define CMICX_IRQ_MASK_SHIFT            16
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

/*!
 * \brief Rx descriptor.
 */
struct cmicx_rx_desc {
    /*! Packet address lower */
    volatile uint32_t addr_lo;

    /*! Packet address higher */
    volatile uint32_t addr_hi;

    /*! Packet control */
    volatile uint32_t ctrl;

    /*! Packet status */
    volatile uint32_t status;
} __attribute__((packed));

/*!
 * \brief Tx descriptor.
 */
struct cmicx_tx_desc {
    /*! Packet address lower */
    volatile uint32_t addr_lo;

    /*! Packet address higher */
    volatile uint32_t addr_hi;

    /*! Packet control */
    volatile uint32_t ctrl;

    /*! Packet status */
    volatile uint32_t status;
} __attribute__((packed));

/*!
 * Flags related to descriptors.
 */
/*! Disable descriptor status write */
#define CMICX_DESC_CTRL_STAT_WR_DIS     (1 << 29)
/*! Descriptors remaining */
#define CMICX_DESC_CTRL_REMAIN(cnt)     (((cnt) & 0xf) << 25)
/*! Max remaining descriptors */
#define CMICX_DESC_REMAIN_MAX           8
/*! Controlled interrupt */
#define CMICX_DESC_CTRL_CNTLD_INTR      (1 << 24)
/*! Completed interrupt */
#define CMICX_DESC_CTRL_CMPLT_INTR      (1 << 23)
/*! Reload DCB */
#define CMICX_DESC_CTRL_RELOAD          (1 << 18)
/*! Scatter DCB */
#define CMICX_DESC_CTRL_SCATTER         (1 << 17)
/*! Chained DCB */
#define CMICX_DESC_CTRL_CHAIN           (1 << 16)
/*! Control flags */
#define CMICX_DESC_CTRL_FLAGS(f)        (((f) & 0xffff) << 16)
/*! Purge packet */
#define CMICX_DESC_TX_PURGE_PKT         (1 << 6)
/*! Higig packet */
#define CMICX_DESC_TX_HIGIG_PKT         (1 << 3)
/*! Packet length */
#define CMICX_DESC_CTRL_LEN(len)        ((len) & 0xffff)
/*! Done */
#define CMICX_DESC_STAT_RTX_DONE        (1 << 31)
/*! Ecc error */
#define CMICX_DESC_STAT_DATA_ERR        (1 << 19)
/*! Cell error */
#define CMICX_DESC_STAT_CELL_ERR        (1 << 18)
/*! Error mask */
#define CMICX_DESC_STAT_ERR_MASK        (CMICX_DESC_STAT_DATA_ERR | \
                                         CMICX_DESC_STAT_CELL_ERR)
/*! Packet start */
#define CMICX_DESC_STAT_PKT_START       (1 << 17)
/*! Packet end */
#define CMICX_DESC_STAT_PKT_END         (1 << 16)
/*! Get done state */
#define CMICX_DESC_STAT_DONE(stat)      ((stat) & CMICX_DESC_STAT_RTX_DONE)
/*! Get flags */
#define CMICX_DESC_STAT_FLAGS(stat)     (((stat) >> 16) & ~0x8003)
/*! Get packet length */
#define CMICX_DESC_STAT_LEN(stat)       ((stat) & 0xffff)

/*! Tx packet header size */
#define CMICX_TX_PKT_HDR_SIZE           16

/*! HW access retry times */
#define CMICX_HW_RETRY_TIMES            100000

/*!
 * \brief Initialize HW handles.
 *
 * \param [in] hw HW structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_cmicx_pdma_hw_hdls_init(struct pdma_hw *hw);

/*!
 * \brief Initialize descriptor operations.
 *
 * \param [in] hw HW structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_cmicx_pdma_desc_ops_init(struct pdma_hw *hw);

/*!
 * \brief Attach device driver.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_cmicx_pdma_driver_attach(struct pdma_dev *dev);

/*!
 * \brief Detach device driver.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_cmicx_pdma_driver_detach(struct pdma_dev *dev);

#endif /* BCMCNET_CMICX_H */

