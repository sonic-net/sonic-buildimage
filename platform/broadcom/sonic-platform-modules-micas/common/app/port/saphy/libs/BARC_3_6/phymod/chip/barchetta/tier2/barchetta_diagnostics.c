/*
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 */

#include <phymod/phymod.h>
#include <phymod/phymod_diagnostics.h>
#include <phymod/phymod_diagnostics_dispatch.h>

#ifdef PHYMOD_BARCHETTA_SUPPORT

#include "../tier1/barchetta_diag_seq.h"

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_prbs_config_set(const plp_barchetta_phymod_phy_access_t* phy, uint32_t flags , const plp_barchetta_phymod_prbs_t* prbs)
{
    return _plp_barchetta_phy_prbs_config_set(phy, flags , prbs);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_prbs_config_get(const plp_barchetta_phymod_phy_access_t* phy, uint32_t flags , plp_barchetta_phymod_prbs_t* prbs)
{
    return _plp_barchetta_phy_prbs_config_get(phy, flags , prbs);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_prbs_enable_set(const plp_barchetta_phymod_phy_access_t* phy, uint32_t flags , uint32_t enable)
{
    return _plp_barchetta_phy_prbs_enable_set(phy, flags , enable);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_prbs_enable_get(const plp_barchetta_phymod_phy_access_t* phy, uint32_t flags , uint32_t* enable)
{
    return _plp_barchetta_phy_prbs_enable_get(phy, flags , enable);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_prbs_status_get(const plp_barchetta_phymod_phy_access_t* phy, uint32_t flags, plp_barchetta_phymod_prbs_status_t* prbs_status)
{
    return _plp_barchetta_phy_prbs_status_get(phy, flags, prbs_status);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_diagnostics_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_phy_diagnostics_t* diag)
{
    return _plp_barchetta_phy_diagnostics_get(phy, diag);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_eyescan_run(const plp_barchetta_phymod_phy_access_t* phy, uint32_t flags, plp_barchetta_phymod_eyescan_mode_t mode, const plp_barchetta_phymod_phy_eyescan_options_t* eyescan_options)
{
    return _plp_barchetta_phy_eyescan_run(phy, flags, mode, eyescan_options);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_link_mon_enable_set(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_link_monitor_mode_t link_mon_mode, uint32_t enable)
{
    return _plp_barchetta_phy_link_mon_enable_set(phy, link_mon_mode, enable);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_link_mon_enable_get(const plp_barchetta_phymod_phy_access_t* phy, plp_barchetta_phymod_link_monitor_mode_t link_mon_mode, uint32_t* enable)
{
    return _plp_barchetta_phy_link_mon_enable_get(phy, link_mon_mode, enable);
}

/*******************************************************************************
 PURPOSE:

 COMMENT:
 *******************************************************************************/
int plp_barchetta_phy_link_mon_status_get(const plp_barchetta_phymod_phy_access_t* phy, uint32_t* lock_status, uint32_t* lock_lost_lh, uint32_t* error_count)
{
    return _plp_barchetta_phy_link_mon_status_get(phy, lock_status, lock_lost_lh, error_count);
}

int plp_barchetta_phy_prbs_error_analyzer_proj(const plp_barchetta_phymod_phy_access_t* phy, uint16_t prbs_error_fec_size, uint8_t hist_errcnt_thresh, uint32_t timeout_s)
{
    return _plp_barchetta_phy_prbs_error_analyzer_proj(phy, prbs_error_fec_size, hist_errcnt_thresh, timeout_s);
}

int plp_barchetta_phy_pattern_enable_set(const plp_barchetta_phymod_phy_access_t* phy, uint32_t enable, const plp_barchetta_phymod_pattern_t* pattern)
{
    return _plp_barchetta_phy_pattern_enable_set(phy, enable, pattern);
}

int plp_barchetta_phy_pattern_enable_get(const plp_barchetta_phymod_phy_access_t* phy, uint32_t *enable, plp_barchetta_phymod_pattern_t* pattern)
{
    return _plp_barchetta_phy_pattern_enable_get(phy, enable, pattern);
}

int plp_barchetta_phy_fec_correctable_counter_get(const plp_barchetta_phymod_phy_access_t* phy, phymod_fec_type_t fec_type, uint32_t* count)
{
    return _plp_barchetta_phy_fec_correctable_counter_get(phy, fec_type, count);
}

int plp_barchetta_phy_fec_uncorrectable_counter_get(const plp_barchetta_phymod_phy_access_t* phy, phymod_fec_type_t fec_type, uint32_t* count)
{
    return _plp_barchetta_phy_fec_uncorrectable_counter_get(phy, fec_type, count);
}

#endif /* PHYMOD_BARCHETTA_SUPPORT */
