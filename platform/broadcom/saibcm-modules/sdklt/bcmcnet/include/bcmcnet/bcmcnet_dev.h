/*! \file bcmcnet_dev.h
 *
 * Generic data structure and macro definitions for BCMCNET device.
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

#ifndef BCMCNET_DEV_H
#define BCMCNET_DEV_H

#include <bcmcnet/bcmcnet_rxtx.h>

/*!
 * \brief HW information.
 */
struct hw_info {
    /*! HW name */
    char *name;

    /*! HW version */
    int ver_no;

    /*! Device ID */
    uint32_t dev_id;

    /*! Revision ID */
    uint32_t rev_id;

    /*! Number of CMCs */
    uint32_t num_cmcs;

    /*! Number of CMC channels */
    uint32_t cmc_chans;

    /*! Number of channels */
    uint32_t num_chans;

    /*! Rx DCB size */
    uint32_t rx_dcb_size;

    /*! Tx DCB size */
    uint32_t tx_dcb_size;

    /*! Rx packet header size */
    uint32_t rx_ph_size;

    /*! Tx packet header size */
    uint32_t tx_ph_size;

    /*! HW structure point */
    struct pdma_hw *hw;
};

/*!
 * \brief Read 32-bit register.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] addr Register address.
 * \param [in] data Pointer to read data.
 */
typedef void (*reg_rd32_f)(struct pdma_hw *hw, uint32_t addr, uint32_t *data);

/*!
 * \brief Write 32-bit register.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] addr Register address.
 * \param [in] data Data to write.
 */
typedef void (*reg_wr32_f)(struct pdma_hw *hw, uint32_t addr, uint32_t data);

/*!
 * \brief Pre-initialize hardware.
 *
 * \param [in] hw Pointer to hardware structure.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*pre_init_f)(struct pdma_hw *hw);

/*!
 * \brief Initialize hardware.
 *
 * \param [in] hw Pointer to hardware structure.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*hw_init_f)(struct pdma_hw *hw);

/*!
 * \brief Configure hardware.
 *
 * \param [in] hw Pointer to hardware structure.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*hw_config_f)(struct pdma_hw *hw);

/*!
 * \brief Reset hardware.
 *
 * \param [in] hw Pointer to hardware structure.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*hw_reset_f)(struct pdma_hw *hw);

/*!
 * \brief Start channel.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] chan Channel number.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*chan_start_f)(struct pdma_hw *hw, int chan);

/*!
 * \brief Stop channel.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] chan Channel number.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*chan_stop_f)(struct pdma_hw *hw, int chan);

/*!
 * \brief Set up channel.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] chan Channel number.
 * \param [in] addr Start DMA address of descriptors.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*chan_setup_f)(struct pdma_hw *hw, int chan, uint64_t addr);

/*!
 * \brief Go to ohter descriptor.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] chan Channel number.
 * \param [in] addr Destination DMA address of descriptors.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*chan_goto_f)(struct pdma_hw *hw, int chan, uint64_t addr);

/*!
 * \brief Clear channel.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] chan Channel number.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*chan_clear_f)(struct pdma_hw *hw, int chan);

/*!
 * \brief Check channel.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] chan Channel number.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*chan_check_f)(struct pdma_hw *hw, int chan);

/*!
 * \brief Get interrupt number.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] chan Channel number.
 *
 * \retval Returned interrupt number, errors if negative value.
 */
typedef int (*chan_intr_num_get_f)(struct pdma_hw *hw, int chan);

/*!
 * \brief Enable interrupt.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] chan Channel number.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*chan_intr_enable_f)(struct pdma_hw *hw, int chan);

/*!
 * \brief Disable interrupt.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] chan Channel number.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*chan_intr_disable_f)(struct pdma_hw *hw, int chan);

/*!
 * \brief Query interrupt.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] chan Channel number.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*chan_intr_query_f)(struct pdma_hw *hw, int chan);

/*!
 * \brief Check interrupt.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] chan Channel number.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*chan_intr_check_f)(struct pdma_hw *hw, int chan);

/*!
 * \brief Coalesce interrupt.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] chan Channel number.
 * \param [in] count Count value to trigger interrupt.
 * \param [in] timer Timer value to triggre interrupt.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*chan_intr_coalesce_f)(struct pdma_hw *hw, int chan, int count, int timer);

/*!
 * \brief Dump registers.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] chan Channel number.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*chan_reg_dump_f)(struct pdma_hw *hw, int chan);

/*!
 * \brief HW handlers.
 */
struct hw_handlers {
    /*! 32 bits register read */
    reg_rd32_f reg_rd32;

    /*! 32 bits register write */
    reg_wr32_f reg_wr32;

    /*! HW pre-initialize */
    pre_init_f pre_init;

    /*! HW initialize */
    hw_init_f hw_init;

    /*! HW configure */
    hw_config_f hw_config;

    /*! HW reset */
    hw_reset_f hw_reset;

    /*! Channel start */
    chan_start_f chan_start;

    /*! Channel stop */
    chan_stop_f chan_stop;

    /*! Channel setup */
    chan_setup_f chan_setup;

    /*! Channel goto */
    chan_goto_f chan_goto;

    /*! Channel clear */
    chan_clear_f chan_clear;

    /*! Channel check */
    chan_check_f chan_check;

    /*! Channel interrupt number get */
    chan_intr_num_get_f chan_intr_num_get;

    /*! Channel interrupt enable */
    chan_intr_enable_f chan_intr_enable;

    /*! Channel interrupt disable */
    chan_intr_disable_f chan_intr_disable;

    /*! Channel interrupt query */
    chan_intr_query_f chan_intr_query;

    /*! Channel interrupt check */
    chan_intr_check_f chan_intr_check;

    /*! Channel interrupt coalesce */
    chan_intr_coalesce_f chan_intr_coalesce;

    /*! Channel registers dump */
    chan_reg_dump_f chan_reg_dump;
};

/*!
 * \brief Initialize Rx descriptor.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] rxq Pointer to Rx queue struture.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_MEMORY Allocation failed.
 */
typedef int (*rx_desc_init_f)(struct pdma_hw *hw, struct pdma_rx_queue *rxq);

/*!
 * \brief Clean up Rx descriptor.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] rxq Pointer to Rx queue struture.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*rx_desc_clean_f)(struct pdma_hw *hw, struct pdma_rx_queue *rxq);

/*!
 * \brief Clean up Rx ring.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] rxq Pointer to Rx queue struture.
 * \param [in] budget Budget for each operation.
 *
 * \retval Number of descriptors finished.
 */
typedef int (*rx_ring_clean_f)(struct pdma_hw *hw, struct pdma_rx_queue *rxq, int budget);

/*!
 * \brief Dump Rx ring.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] rxq Pointer to Rx queue struture.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*rx_ring_dump_f)(struct pdma_hw *hw, struct pdma_rx_queue *rxq);

/*!
 * \brief Suspend Rx queue.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] rxq Pointer to Rx queue struture.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*rx_suspend_f)(struct pdma_hw *hw, struct pdma_rx_queue *rxq);

/*!
 * \brief Resume Rx queue.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] rxq Pointer to Rx queue struture.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*rx_resume_f)(struct pdma_hw *hw, struct pdma_rx_queue *rxq);

/*!
 * \brief Initialize Tx descriptor.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] txq Pointer to Tx queue struture.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_MEMORY Allocation failed.
 */
typedef int (*tx_desc_init_f)(struct pdma_hw *hw, struct pdma_tx_queue *txq);

/*!
 * \brief Clean up Tx descriptor.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] txq Pointer to Tx queue struture.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*tx_desc_clean_f)(struct pdma_hw *hw, struct pdma_tx_queue *txq);

/*!
 * \brief Clean up Tx ring.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] txq Pointer to Tx queue struture.
 * \param [in] budget Budget for each operation.
 *
 * \retval Number of descriptors finished.
 */
typedef int (*tx_ring_clean_f)(struct pdma_hw *hw, struct pdma_tx_queue *txq, int budget);

/*!
 * \brief Dump Tx ring.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] txq Pointer to Tx queue struture.
 *
 * \retval SHR_E_NONE No errors.
 */
typedef int (*tx_ring_dump_f)(struct pdma_hw *hw, struct pdma_tx_queue *txq);

/*!
 * \brief Transmit packet.
 *
 * \param [in] hw Pointer to hardware structure.
 * \param [in] txq Pointer to Tx queue struture.
 * \param [in] buf Pointer to packet buffer struture.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
typedef int (*pkt_xmit_f)(struct pdma_hw *hw, struct pdma_tx_queue *txq, void *buf);

/*!
 * \brief Descriptor operations.
 */
struct desc_operations {
    /*! Rx descriptor initialize */
    rx_desc_init_f rx_desc_init;

    /*! Rx descriptor cleanup */
    rx_desc_clean_f rx_desc_clean;

    /*! Rx ring cleanup */
    rx_ring_clean_f rx_ring_clean;

    /*! Rx ring dump */
    rx_ring_dump_f rx_ring_dump;

    /*! Rx suspend */
    rx_suspend_f rx_suspend;

    /*! Rx resume */
    rx_resume_f rx_resume;

    /*! Tx descriptor initialize */
    tx_desc_init_f tx_desc_init;

    /*! Tx descriptor cleanup */
    tx_desc_clean_f tx_desc_clean;

    /*! Tx ring cleanup */
    tx_ring_clean_f tx_ring_clean;

    /*! Tx ring dump */
    tx_ring_dump_f tx_ring_dump;

    /*! Tx transmit */
    pkt_xmit_f pkt_xmit;
};

/*!
 * \brief HW structure.
 */
struct pdma_hw {
    /*! Device number */
    int unit;

    /*! Device structure point */
    struct pdma_dev *dev;

    /*! HW information */
    struct hw_info info;

    /*! HW handlers */
    struct hw_handlers hdls;

    /*! HW operations */
    struct desc_operations dops;
};

/*!
 * \brief Open device.
 *
 * \param [in] dev Device structure point.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_open(struct pdma_dev *dev);

/*!
 * \brief Coalesce Rx interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 * \param [in] count Interrupt threshhold.
 * \param [in] timer Timer value.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_rx_queue_int_coalesce(struct pdma_dev *dev, int queue, int count, int timer);

/*!
 * \brief Coalesce Tx interrupt.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 * \param [in] count Interrupt threshhold.
 * \param [in] timer Timer value.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_tx_queue_int_coalesce(struct pdma_dev *dev, int queue, int count, int timer);

/*!
 * \brief Dump Rx queue registers.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Rx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_rx_queue_reg_dump(struct pdma_dev *dev, int queue);

/*!
 * \brief Dump Tx queue registers.
 *
 * \param [in] dev Device structure point.
 * \param [in] queue Tx queue number.
 *
 * \retval SHR_E_NONE No errors.
 * \retval SHR_E_XXXX Operation failed.
 */
extern int
bcmcnet_pdma_tx_queue_reg_dump(struct pdma_dev *dev, int queue);

#endif /* BCMCNET_DEV_H */

