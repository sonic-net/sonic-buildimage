/*
 *
 * $Id: barchetta_diag_seq.h Exp $
 *
 *
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 *
 *
 *
 *
 */

#ifndef __BARCHETTA_DIAG_SEQ_H__
#define __BARCHETTA_DIAG_SEQ_H__
#include <phymod/phymod.h>
#include <phymod/phymod_diagnostics.h>
#include "barchetta_cfg_seq.h"

/***************************************************************************//**
 \brief    To set the PRBS configuration
 \param    phy         [In]  Pointer to phymod phy access structure
 \param    flags       [In]  flags
 \param    prbs        [In]  Pointer to prbs configuration
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_prbs_config_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t flags,
    const plp_barchetta_phymod_prbs_t* prbs
);

/***************************************************************************//**
 \brief    To get the PRBS configuration
 \param    phy         [In]  Pointer to phymod phy access structure
 \param    flags       [In]  flags
 \param    prbs        [In]  Pointer to prbs configuration
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_prbs_config_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t flags,
    plp_barchetta_phymod_prbs_t* prbs
);

/***************************************************************************//**
 \brief    To enable PRBS
 \param    phy         [In]  Pointer to phymod phy access structure
 \param    flags       [In]  flags
 \param    enable      [In]  prbs enable
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_prbs_enable_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t flags,
    uint32_t enable
);

/***************************************************************************//**
 \brief    To get prbs enable status
 \param    phy         [In]  Pointer to phymod phy access structure
 \param    flags       [In]  flags
 \param    enable      [Out] Pointer to prbs enable status
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_prbs_enable_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t flags,
    uint32_t* enable
);

/***************************************************************************//**
 \brief    To get the PRBS status
 \param    phy         [In]  Pointer to phymod phy access structure
 \param    flags       [In]  flags
 \param    prbs_status [Out] Pointer to prbs status
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_prbs_status_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t flags,
    plp_barchetta_phymod_prbs_status_t* prbs_status
);

/***************************************************************************//**
 \brief    To get phy diagonistics information
 \param    phy    [In]  Pointer to phymod phy access structure
 \param    diag   [Out] Pointer to phy diagonistics structure
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_diagnostics_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_phy_diagnostics_t* diag
);

/***************************************************************************//**
 \brief    To perform eyescan
 \param    phy              [In]  Pointer to phymod phy access structure
 \param    flags            [In]  flags
 \param    mode             [In]  eyescan mode
 \param    eyescan_options  [In]  Pointer to eyescan options
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_eyescan_run(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t flags,
    plp_barchetta_phymod_eyescan_mode_t mode,
    const plp_barchetta_phymod_phy_eyescan_options_t* eyescan_options
);

/***************************************************************************//**
 \brief    To enable PCS link monitor
 \param    phy              [In]  Pointer to phymod phy access structure
 \param    link_mon_mode    [In]  link monitor mode
 \param    enable           [In]  enable
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_PARAM(-4)
 *******************************************************************************/
int _plp_barchetta_phy_link_mon_enable_set(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_link_monitor_mode_t link_mon_mode,
    uint32_t enable
);

/***************************************************************************//**
 \brief    To get the enable status of PCS link monitor
 \param    phy              [In]  Pointer to phymod phy access structure
 \param    link_mon_mode    [In]  link monitor mode
 \param    enable           [In]  Pointer to enable status
 \retval   PHYMOD_E_NONE(0), PHYMOD_E_PARAM(-4)
 *******************************************************************************/
int _plp_barchetta_phy_link_mon_enable_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    plp_barchetta_phymod_link_monitor_mode_t link_mon_mode,
    uint32_t* enable
);

/***************************************************************************//**
 \brief    To get the status of PCS link monitor
 \param    phy            [In]  Pointer to phymod phy access structure
 \param    lock_status    [In]  lock status
 \param    lock_lost_lh   [In]  lock lost
 \param    error_count    [In]  error count
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_link_mon_status_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    uint32_t* lock_status,
    uint32_t* lock_lost_lh,
    uint32_t* error_count
);

/***************************************************************************//**
 \brief    To display PRBS Error Analyzer Projection
 \param    phy                 [In]  Pointer to phymod phy access structure
 \param    prbs_error_fec_size [In]  prbs error fec size
 \param    hist_errcnt_thresh  [In]  hist err count threshold
 \param    timeout_s           [In]  Timeout
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_prbs_error_analyzer_proj(
    const plp_barchetta_phymod_phy_access_t* phy, 
    uint16_t prbs_error_fec_size, 
    uint8_t hist_errcnt_thresh, uint32_t timeout_s
);

/***************************************************************************//**
 \brief    To retrive FEC corrected code word counter
 \param    phy      [In]  Pointer to phymod phy access structure
 \param    fec_type [In]  Represents FEC type (CL91/CL74)
 \param    count    [Out] FEC corrected word counter
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_fec_correctable_counter_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    phymod_fec_type_t fec_type,
    uint32_t* count
);

/***************************************************************************//**
 \brief    To retrive FEC uncorrected code word counter.
 \param    phy      [In]  Pointer to phymod phy access structure
 \param    fec_type [In]  Represents FEC type (CL91/CL74)
 \param    count    [Out] FEC uncorrected word counter
 \retval   PHYMOD_E_NONE(0)
 *******************************************************************************/
int _plp_barchetta_phy_fec_uncorrectable_counter_get(
    const plp_barchetta_phymod_phy_access_t* phy,
    phymod_fec_type_t fec_type,
    uint32_t* count
);

int _plp_barchetta_phy_pattern_enable_set(const plp_barchetta_phymod_phy_access_t* phy, uint32_t enable, 
                                      const plp_barchetta_phymod_pattern_t* pattern);
int _plp_barchetta_phy_pattern_enable_get(const plp_barchetta_phymod_phy_access_t* phy, uint32_t *enable, 
                                      plp_barchetta_phymod_pattern_t* pattern);

#endif
