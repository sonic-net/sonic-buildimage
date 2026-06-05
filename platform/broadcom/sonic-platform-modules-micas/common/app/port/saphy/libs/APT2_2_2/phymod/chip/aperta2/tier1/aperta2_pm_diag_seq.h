/*
 *
 * $Id: aperta2_diag_seq.h  $
 *
 *  *
 *  *
  * $Copyright: (c) 2022 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
 *  *
 *
 *
 *
 */

#ifndef __APERTA2_PM_DIAG_SEQ_H__
#define __APERTA2_PM_DIAG_SEQ_H__

int plp_aperta2_pm_prbs_config_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags , const plp_aperta2_phymod_prbs_t* prbs);
int plp_aperta2_pm_prbs_config_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags , plp_aperta2_phymod_prbs_t* prbs);
int plp_aperta2_pm_prbs_enable_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags , uint32_t enable);
int plp_aperta2_pm_prbs_enable_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags , uint32_t* enable) ;
int plp_aperta2_pm_prbs_status_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, plp_aperta2_phymod_prbs_status_t* prbs_status);

#endif


