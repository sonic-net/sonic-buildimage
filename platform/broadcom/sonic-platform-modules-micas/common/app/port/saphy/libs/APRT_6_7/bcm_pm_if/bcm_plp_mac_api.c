/*
 *         
 * $Id: bcm_mac_api.c $
 * 
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$ 
 *         
 *     
 */

#ifdef BCM_PLP_MAC_SUPPORT
#include "bcm_plp_mac_api.h"
#include "bcm_pm_if.h"

#ifdef PHYMOD_EVORA_SUPPORT
#include "../phymod/chip/evora/tier1/evora_pm_seq.h"
#include "../phymod/chip/evora/tier1/evora_reg_access.h"
#include "../phymod/chip/evora/tier1/evora_cfg_seq.h"
#endif
#ifdef PHYMOD_MIURA_SUPPORT
#include "../phymod/chip/miura/tier1/miura_pm_seq.h"
#include "../phymod/chip/miura/tier1/miura_reg_access.h"
#include "../phymod/chip/miura/tier1/miura_cfg_seq.h"
#endif
#ifdef PHYMOD_APERTA_SUPPORT
#include "../phymod/chip/aperta/tier1/aperta_pm_seq.h"
#include "../phymod/chip/aperta/tier1/aperta_reg_access.h"
#include "../phymod/chip/aperta/tier1/aperta_cfg_seq.h"
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
#include "../phymod/chip/aperta2/tier1/aperta2_pm_seq.h"
#include "../phymod/chip/aperta2/tier1/aperta2_reg_access.h"
#include "../phymod/chip/aperta2/tier1/aperta2_cfg_seq.h"
#endif

#if defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT) \
                                    || defined(PHYMOD_WHITETIP_SUPPORT)
#include "../phymod/common/xlmac/portmod.h"
#include "../phymod/common/xlmac/xlmac_access.h"
  #if   defined(PHYMOD_LONGFIN_SUPPORT)
    #include "../phymod/chip/longfin/tier2/xgbaset_mac.h"
  #elif defined(PHYMOD_BROADFIN_SUPPORT)
    #include "../phymod/chip/broadfin/tier2/xgbaset_mac.h"
  #elif defined(PHYMOD_WHITETIP_SUPPORT)
    #include "../phymod/chip/whitetip/tier2/xgbaset_mac.h"
  #else
    /* nothing */
  #endif
#include "../phymod/common/xlmac/xlmac.h"
#endif

extern bcm_if_phymod_ctrl_t plp_aperta_phy_ctrl;

/*! \brief Warmboot Init
 *
 *	This API is used to restore and initialize in the case of warmboot
 *
 *  @param mac_info        Represents MAC access\n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_warmboot_init(bcm_plp_mac_access_t mac_info)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
 
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_warmboot_init(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_warmboot_init(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_warmboot_init(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_warmboot_init(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = xgbaset_mac_warmboot_init(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    return rv; 
}

/*! \brief MAC Loopback set
 *
 *	This API is used to set the MAC outer loopback
 *
 *  @param mac_info        Represents MAC access\n
 *  @param lb_type         Represents MAC LB type \n
 *                         0 - MAC loopback \n
 *  @param enable          Represents Enable/Disable\n
 *                         0 - disable\n
 *                         1 - enable
 * 
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_mac_loopback_set(bcm_plp_mac_access_t mac_info, bcm_plp_mac_lb_type_t lb_type, uint32_t enable)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS, loopback = 0;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    if (lb_type >= bcmplpMaxloopbackCnt) {
       PHYMOD_DEBUG_ERROR(("Invalid loopback type\n"))
       rv = BCM_PM_IF_PARAM; 
       goto ERR;
    }
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    
    rv = BCM_PM_IF_UNAVAIL;
    if (lb_type == bcmplpMacOuterloopback) {
        loopback = 5;
    }
#if   defined(PHYMOD_EVORA_SUPPORT)
    rv = evora_pm_loopback_set( &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, loopback, enable);
#elif defined(PHYMOD_MIURA_SUPPORT)
    rv = miura_pm_loopback_set( &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, loopback, enable);
#elif defined(PHYMOD_APERTA_SUPPORT)
    rv = plp_aperta_pm_loopback_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, loopback, enable);
#elif defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    (void ) loopback;
    rv = _xlmac_loopback_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (int) enable);
#elif defined(PHYMOD_APERTA2_SUPPORT)
    (void ) loopback;
#else
    #error "Unknown PHY !!"
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv; 
}

/*! \brief MAC Loopback get
 *
 *	This API is used to get the MAC outer loopback
 *  @param mac_info        Represents MAC access\n
 *  @param lb_type         Represents MAC LB type \n
 *                         0 - MAC loopback \n
 *  @param enable          [OUT] Represents Enable/Disable status\n
 *                         0 - disable\n
 *                         1 - enable 
 * 
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_mac_loopback_get(bcm_plp_mac_access_t mac_info, bcm_plp_mac_lb_type_t lb_type, uint32_t *enable)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS, loopback = 0;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    if (lb_type >= bcmplpMaxloopbackCnt) {
       PHYMOD_DEBUG_ERROR(("Invalid loopback type\n"))
       rv = BCM_PM_IF_PARAM; 
       goto ERR;
    }
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);

    rv = BCM_PM_IF_UNAVAIL;
    if (lb_type == bcmplpMacOuterloopback) {
        loopback = 5;
    }
#if   defined(PHYMOD_EVORA_SUPPORT)
    rv = evora_pm_loopback_get( &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, loopback, enable);
#elif defined(PHYMOD_MIURA_SUPPORT)
    rv = miura_pm_loopback_get( &plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, loopback, enable);
#elif defined(PHYMOD_APERTA_SUPPORT)
    rv = plp_aperta_pm_loopback_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, loopback, enable);
#elif defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    (void ) loopback;
    rv = _xlmac_loopback_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (int*) enable);
#elif defined(PHYMOD_APERTA2_SUPPORT)
    (void ) loopback;
#else
    #error "Unknown PHY !!"
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv; 
}

/*! \brief Fault Control
 *
 *	This API is used to set the flow control option.
 *
 *  @param mac_info <pre>        Represents MAC access</pre>\n
 *  @param fault_option  Represents the mode of Fault\n
 *                               0 : bcmFaultcontrolTerminateGenerate \n
 *                               1 : bcmFaultcontrolPassthrough  \n
 *
 *  NOTE: User has to call bcm_plp_mac_fault_option_set API before configuring the port (i.e. 
 *        bcm_plp_mode_config_set). Calling bcm_plp_mac_fault_option_set after port configuration (i.e. 
 *        bcm_plp_mode_config_set) will not update port configuration.
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_mac_fault_option_set(bcm_plp_mac_access_t mac_info, bcm_plp_mac_fault_option_t fault_option)
{
    uint32_t phy_id_idx =0 , exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_fault_option_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, fault_option);
#endif    
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_fault_option_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, fault_option);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_fault_option_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, fault_option);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_fault_option_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, fault_option);
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = xgbaset_fault_option_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                  (xgbaset_fault_option_t) fault_option);
#endif
ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}


/*! \brief Fault Control
 *
 *	This API is used to get enable or disable status of flow control.
 *
 *  @param mac_info <pre>        Represents MAC access</pre>\n
 *  @param fault_option  Represents the mode of flow control\n
 *                               0 : bcmFaultcontrolTerminateGenerate \n
 *                               1 : bcmFaultcontrolPassthrough  \n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_mac_fault_option_get(bcm_plp_mac_access_t mac_info, bcm_plp_mac_fault_option_t *fault_option)
{
    uint32_t phy_id_idx =0 , exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
#ifdef PHYMOD_EVORA_SUPPORT
    evora_pm_fault_option_t fault_optn;
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    miura_pm_fault_option_t miura_fault_optn;
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    aperta_pm_fault_option_t aperta_fault_optn;
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    aperta2_pm_fault_option_t aperta2_fault_optn;
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    xgbaset_fault_option_t  xgbaset_fault_optn;
#endif
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_fault_option_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &fault_optn);
    *fault_option = fault_optn;
#endif    
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_fault_option_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &miura_fault_optn);
    *fault_option = miura_fault_optn;
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_fault_option_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &aperta_fault_optn);
    *fault_option = aperta_fault_optn;
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_fault_option_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &aperta2_fault_optn);
    *fault_option = aperta2_fault_optn;
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = xgbaset_fault_option_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &xgbaset_fault_optn);
    *fault_option = xgbaset_fault_optn;
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief Flow Control
 *
 *	This API is used to set the flow control option.
 *
 *  @param mac_info <pre>        Represents MAC access</pre>\n
 *  @param flow_control_option   Represents the mode of flow control\n
 *                               0 : bcmFlowcontrolTerminateGenerate \n
 *                               1 : bcmFlowcontrolPassthrough  \n
 *  NOTE: User has to call bcm_plp_mac_flow_control_set API before configuring the port (i.e. 
 *        bcm_plp_mode_config_set). Calling bcm_plp_mac_flow_control_set after port configuration (i.e. 
 *        bcm_plp_mode_config_set) will not update port configuration.
 *
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_mac_flow_control_set(bcm_plp_mac_access_t mac_info, bcm_plp_mac_flow_control_t flow_control_option)
{
    uint32_t phy_id_idx =0 , exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_flow_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flow_control_option);
#endif    
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_flow_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flow_control_option);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_flow_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flow_control_option);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_flow_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flow_control_option);
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = xgbaset_flow_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flow_control_option);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}


/*! \brief Flow Control
 *
 *	This API is used to get enable or disable status of flow control.
 *
 *  @param mac_info <pre>        Represents MAC access</pre>\n
 *  @param flow_control_option   Represents the mode of flow control\n
 *                               0 : bcmFlowcontrolTerminateGenerate \n
 *                               1 : bcmFlowcontrolPassthrough  \n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_mac_flow_control_get(bcm_plp_mac_access_t mac_info, bcm_plp_mac_flow_control_t *flow_control_option)
{
    uint32_t phy_id_idx =0 , exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
#ifdef PHYMOD_EVORA_SUPPORT
    evora_pm_flow_control_t flow_control;
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    miura_pm_flow_control_t miura_flow_control;
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    aperta_pm_flow_control_t aperta_flow_control;
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    aperta2_pm_flow_control_t aperta2_flow_control;
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    xgbaset_flow_control_t  xgbaset_flow_control;
#endif

    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_flow_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &flow_control);
    *flow_control_option = flow_control;
#endif    
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_flow_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &miura_flow_control);
    *flow_control_option = miura_flow_control;
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_flow_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &aperta_flow_control);
    *flow_control_option = aperta_flow_control;
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_flow_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &aperta2_flow_control);
    *flow_control_option = aperta2_flow_control;
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = xgbaset_flow_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &xgbaset_flow_control);
    *flow_control_option = xgbaset_flow_control;
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief Store & Forward mode
 *
 *  This API is used to enable or disable store & forward mode.
 *
 *  @param mac_info <pre>  Represents MAC access</pre>\n
 *  @param enable          Enable/Disable store & forward mode\n
 *                          1 - Enable\n
 *                          0 - Disable \n
 *  NOTE: User has to call bcm_plp_mac_store_and_forward_mode_set API before configuring the port (i.e. 
 *        bcm_plp_mode_config_set). Calling bcm_plp_mac_store_and_forward_mode_set after port configuration (i.e. 
 *        bcm_plp_mode_config_set) will not update port configuration.
 *
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_mac_store_and_forward_mode_set(bcm_plp_mac_access_t mac_info, int enable)
{
    uint32_t phy_id_idx =0 , exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_store_and_forward_mode_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable);
#endif    
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_store_and_forward_mode_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_store_and_forward_mode_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_store_and_forward_mode_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief Store & Forward mode
 *
 *  This API is used to get the enable or disable status of store & forward mode.
 *
 *  @param mac_info <pre>  Represents MAC access</pre>\n
 *  @param is_enabled      [OUT]enable/Disable store & forward mode\n
 *                          1 - Enable\n
 *                          0 - Disable \n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_mac_store_and_forward_mode_get(bcm_plp_mac_access_t mac_info, int *is_enabled)
{
    uint32_t phy_id_idx =0 , exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_store_and_forward_mode_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, is_enabled);
#endif    
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_store_and_forward_mode_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, is_enabled);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_store_and_forward_mode_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, is_enabled);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_store_and_forward_mode_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, is_enabled);
#endif


ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief 64-bit Register Write helper API
 *
 *	This helper API is used to perform 64-bit register write.
 *
 * @param phy_info <pre>   Represents PHY access\n
 *  @param devaddr         device address\n 1 - PMA/PMD \n 3 - PCS \n 7 - AN \n 30 - user
 *  @param regaddr         Register address of the device
 *  @param data            64-bit Value to be written to Register    </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_reg64_value_set_helper(bcm_plp_access_t phy_info, unsigned int devaddr, unsigned int regaddr, plp_uint64_t data)
{
    uint32_t phy_id_idx =0 , exist_phy = 0;
    int rv = BCM_PM_IF_UNAVAIL;
    int base_addr = regaddr & 0xFF000000;

    BCM_PLP_MAC_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_MAC_FILL_PHY_ACCESS(phy_info, phy_id_idx);
#if   defined(PHYMOD_EVORA_SUPPORT)
    if (base_addr == EVORA_PM_CLPORT_BASEADR || base_addr == EVORA_PM_CLPORT_64B_BASEADR ||
        base_addr == EVORA_PM_TSCF_BASEADR || base_addr == EVORA_PM_TSC_FALCON_BASEADR) {
        rv =  evora_set_side (&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access, plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.port_loc);
        if (rv != 0) {
            goto ERR;
        }
    }
    rv = evora_raw_write(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access, regaddr, data);
#elif defined(PHYMOD_MIURA_SUPPORT)
    if (base_addr == MIURA_PM_XLPORT_BASEADR || base_addr == MIURA_PM_XLPORT_64B_BASEADR ||
        base_addr == MIURA_PM_TSCE_BASEADR || base_addr == MIURA_PM_TSC_EAGLE_BASEADR ||
        base_addr == MIURA_EDC_BASEADR || base_addr == MIURA_MERLIN_BASEADR) {
        rv =  miura_set_side (&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access, plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.port_loc);
        if (rv != 0) {
            goto ERR;
        }
    }
    rv = miura_raw_write(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access, regaddr, data);
#elif defined(PHYMOD_APERTA_SUPPORT)
    (void) base_addr;
    rv = plp_aperta_reg64_write(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, regaddr, data);
#elif defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    (void) devaddr;
    if ( base_addr ) {
        /* convert register address to register order */
        regaddr = (regaddr & 0xffffff80) | ((regaddr & 0x7f) >> 3);
        rv = xlmac_reg64_write(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, regaddr, data);
    }
#elif defined(PHYMOD_APERTA2_SUPPORT)
    (void ) base_addr;
    rv = aperta2_reg64_write(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, regaddr, data);
#else
    #error "Unknown PHY !!"
#endif
ERR:
    return rv;
}

/*! \brief 64-bit Register Read helper API
 *
 *	This helper API is used to read the specified register address.
 *
 * @param phy_info      <pre> Represents PHY access\n
 *  @param devaddr         device address\n 1 - PMA/PMD \n 3 - PCS \n 7 - AN \n 30 - user
 *  @param regaddr         Register address to read
 *  @param data            [OUT] Value of the specified register    </pre>
 *
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_reg64_value_get_helper(bcm_plp_access_t phy_info, unsigned int devaddr, unsigned int regaddr, plp_uint64_t *data)
{
    uint32_t phy_id_idx =0 , exist_phy = 0;
	int rv = BCM_PM_IF_UNAVAIL;
    int base_addr = regaddr & 0xFF000000;

    BCM_PLP_MAC_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PLP_MAC_FILL_PHY_ACCESS(phy_info, phy_id_idx);

#if   defined(PHYMOD_EVORA_SUPPORT)
    if ( base_addr == EVORA_PM_CLPORT_BASEADR || base_addr == EVORA_PM_CLPORT_64B_BASEADR ||
         base_addr == EVORA_PM_TSCF_BASEADR   || base_addr == EVORA_PM_TSC_FALCON_BASEADR ) {
        rv =  evora_set_side (&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access, plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.port_loc);
        if ( rv != 0 ) {
            goto ERR;
        }
    }

    rv = evora_raw_read(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access, regaddr, data);
#elif defined(PHYMOD_MIURA_SUPPORT)
    if ( base_addr == MIURA_PM_XLPORT_BASEADR || base_addr == MIURA_PM_XLPORT_64B_BASEADR ||
         base_addr == MIURA_PM_TSCE_BASEADR   || base_addr == MIURA_PM_TSC_EAGLE_BASEADR  ||
         base_addr == MIURA_EDC_BASEADR       || base_addr == MIURA_MERLIN_BASEADR ) {
        rv = miura_set_side (&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access, plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.port_loc);
        if ( rv != 0 ) {
            goto ERR;
        }
    }
    rv = miura_raw_read(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access, regaddr, data);
#elif defined(PHYMOD_APERTA_SUPPORT)
     if((regaddr >> 24) == 0x18) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.pll_idx = (*data & 0x80000000) ? 1: 0;
     }

    (void) base_addr;
    rv = plp_aperta_reg64_read(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, regaddr, data);
    if((regaddr >> 24) == 0x18) {
        plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.pll_idx = 0;
    }

#elif defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    (void) devaddr;
    if ( base_addr ) {
        /* convert register address to register order */
        regaddr = (regaddr & 0xffffff80) | ((regaddr & 0x7f) >> 3);
        rv = xlmac_reg64_read(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, regaddr, data);
    }
#elif defined(PHYMOD_APERTA2_SUPPORT)
    (void) base_addr;
    rv = aperta2_reg64_read(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, regaddr, data);
#else
    #error "Unknown PHY !!"
#endif
ERR:
    return rv;
}

/*! \brief 64-bit Register Write
 *
 *	This API is used to perform 64-bit register write.
 *
 * @param phy_info <pre>   Represents PHY access\n
 *  @param devaddr         device address\n 1 - PMA/PMD \n 3 - PCS \n 7 - AN \n 30 - user
 *  @param regaddr         Register address of the device
 *  @param data            64-bit Value to be written to Register    </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_reg64_value_set(bcm_plp_access_t phy_info, unsigned int devaddr, unsigned int regaddr, plp_uint64_t data)
{
    uint32_t phy_id_idx =0 , exist_phy = 0;
    int rv = BCM_PM_IF_UNAVAIL;

    BCM_PLP_MAC_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    rv = bcm_plp_aperta_reg64_value_set_helper(phy_info, devaddr, regaddr, data);

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    return rv;
}

/*! \brief 64-bit Register Read
 *
 *	This API is used to read the specified register address.
 *
 * @param phy_info      <pre> Represents PHY access\n
 *  @param devaddr         device address\n 1 - PMA/PMD \n 3 - PCS \n 7 - AN \n 30 - user
 *  @param regaddr         Register address to read
 *  @param data            [OUT] Value of the specified register    </pre>
 *
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_reg64_value_get(bcm_plp_access_t phy_info, unsigned int devaddr, unsigned int regaddr, plp_uint64_t *data)
{
    uint32_t phy_id_idx =0 , exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PLP_MAC_GET_P_CTXT(phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(phy_info.phy_addr, phy_info.platform_ctxt);

    rv = bcm_plp_aperta_reg64_value_get_helper(phy_info, devaddr, regaddr, data);

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(phy_info.phy_addr, phy_info.platform_ctxt);
    return rv;
}

/*! \brief Cleanup the PortMACRO.
 *
 *	This API cleanup the allocated PM SW database and this API needs to be called before bcm_plp_<chipname>_cleanup.
 *
 * @param mac_info  <pre> Represents MAC access\n
 * </pre>
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int
bcm_plp_aperta_mac_cleanup(bcm_plp_mac_access_t mac_info)
{
    int rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_remove_pm_info(mac_info.phy_info.phy_addr);
#endif    
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_remove_pm_info(mac_info.phy_info.phy_addr);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_remove_pm_info(mac_info.phy_info.phy_addr);
#endif
#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    uint32_t  phy_id_idx = 0, exist_phy = 0;
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);

    rv = xgbaset_mac_cleanup(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_remove_pm_info(mac_info.phy_info.phy_addr);
#endif

    return rv;
}

/*! \brief Configure Max packet size 
 *
 *	This API is used to configure max packet size of a port. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param pkt_size        Represents max packet size </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_max_packet_size_set(bcm_plp_mac_access_t mac_info, int pkt_size)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_max_pkt_size_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pkt_size);
#endif    
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_max_pkt_size_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pkt_size);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_max_pkt_size_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pkt_size);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_max_pkt_size_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pkt_size);
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_rx_max_size_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pkt_size);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief Get configure Max packet size 
 *
 *	This API is used to get configured max packet size of a port. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param pkt_size        output, Represents max packet size </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_max_packet_size_get(bcm_plp_mac_access_t mac_info, int *pkt_size)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_max_pkt_size_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pkt_size);
#endif    
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_max_pkt_size_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pkt_size);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_max_pkt_size_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pkt_size);
#endif
#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_rx_max_size_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pkt_size);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_max_pkt_size_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pkt_size);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief Configure RUNT threshold 
 *
 *	This API is used to configure RUNT threshold value. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param threshold       Represents RUNT threshold </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_runt_threshold_set(bcm_plp_mac_access_t mac_info, int threshold)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_runt_threshold_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, threshold);
#endif    
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_runt_threshold_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, threshold);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_runt_threshold_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, threshold);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_runt_threshold_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, threshold);
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_runt_threshold_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, threshold);
#endif
ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief Get configured RUNT threshold 
 *
 *	This API is used to get the configured RUNT threshold value. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param threshold       output, Represents RUNT threshold </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_runt_threshold_get(bcm_plp_mac_access_t mac_info, int *threshold)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_runt_threshold_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, threshold);
#endif    
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_runt_threshold_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, threshold);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_runt_threshold_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, threshold);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_runt_threshold_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, threshold);
#endif
#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_runt_threshold_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, threshold);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief Set Pad threshold
 *
 *	This API is used to configure pad threshold value. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param pad_size       Represents value of padding </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_pad_size_set(bcm_plp_mac_access_t mac_info, int pad_size)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_pad_size_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pad_size);
#endif    
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_pad_size_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pad_size);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_pad_size_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pad_size);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    /* coverity[value_overwrite:FALSE] */
    rv = PHYMOD_E_UNAVAIL;
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_pad_size_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pad_size);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief Get Pad threshold
 *
 *	This API is used to get the configured pad threshold value. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param pad_size        output, Represents padded value </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_pad_size_get(bcm_plp_mac_access_t mac_info, int *pad_size)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_pad_size_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pad_size);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_pad_size_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pad_size);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_pad_size_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pad_size);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    /* coverity[value_overwrite:FALSE] */
    rv = PHYMOD_E_UNAVAIL;
#endif
#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_pad_size_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, pad_size);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief Configure TX MAC SA
 *
 *	This API is used to configure TX MAC SA. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param mac_sa          Represents TX mac SA </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_tx_mac_sa_set(bcm_plp_mac_access_t mac_info, unsigned char mac_sa[6])
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_tx_mac_sa_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_tx_mac_sa_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_tx_mac_sa_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_tx_mac_sa_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif
#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_tx_mac_sa_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief Get TX MAC SA
 *
 *	This API is used to get configured TX MAC SA. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param mac_sa          output, Represents TX mac SA </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_tx_mac_sa_get(bcm_plp_mac_access_t mac_info, unsigned char mac_sa[6])
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_tx_mac_sa_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_tx_mac_sa_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_tx_mac_sa_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_tx_mac_sa_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_tx_mac_sa_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief Configure RX MAC SA
 *
 *	This API is used to configure RX MAC SA. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param mac_sa          Represents RX mac SA </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_rx_mac_sa_set(bcm_plp_mac_access_t mac_info, unsigned char mac_sa[6])
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_rx_mac_sa_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_rx_mac_sa_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_rx_mac_sa_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_rx_mac_sa_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif
#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_rx_mac_sa_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief Get RX MAC SA
 *
 *	This API used to get configured RX MAC SA. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param mac_sa          output, Represents RX mac SA </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_rx_mac_sa_get(bcm_plp_mac_access_t mac_info, unsigned char mac_sa[6])
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_rx_mac_sa_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_rx_mac_sa_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_rx_mac_sa_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_rx_mac_sa_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_rx_mac_sa_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, mac_sa);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief Configure average IPG
 *
 *	This API is used to configure average IPG. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param avg_ipg         Represents average IPG </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_tx_avg_ipg_set(bcm_plp_mac_access_t mac_info, int avg_ipg)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_tx_avg_ipg_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, avg_ipg);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_tx_avg_ipg_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, avg_ipg);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_tx_avg_ipg_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, avg_ipg);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_tx_avg_ipg_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, avg_ipg);
#endif
#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_tx_average_ipg_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, avg_ipg);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief Get configured average IPG
 *
 *	This API is used to get configured average IPG. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param avg_ipg         output, Represents average IPG </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_tx_avg_ipg_get(bcm_plp_mac_access_t mac_info, int *avg_ipg)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_tx_avg_ipg_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, avg_ipg);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_tx_avg_ipg_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, avg_ipg);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_tx_avg_ipg_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, avg_ipg);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_tx_avg_ipg_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, avg_ipg);
#endif
#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_tx_average_ipg_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, avg_ipg);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief configure tx preamble 
 *
 *	This API is used to configure TX preamble. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param preamble_length Represents Tx preamble length </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_tx_preamble_length_set(bcm_plp_mac_access_t mac_info, int preamble_length)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_tx_preamble_length_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, preamble_length);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_tx_preamble_length_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, preamble_length);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_tx_preamble_length_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, preamble_length);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    /* coverity[value_overwrite:FALSE] */
    rv = PHYMOD_E_UNAVAIL;
#endif
#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_tx_preamble_length_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, preamble_length);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief Get configured tx preamble 
 *
 *	This API is used to get configured TX preamble. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param preamble_length output, Represents Tx preamble length </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_tx_preamble_length_get(bcm_plp_mac_access_t mac_info, int *preamble_length)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_tx_preamble_length_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, preamble_length);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_tx_preamble_length_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, preamble_length);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_tx_preamble_length_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, preamble_length);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    /* coverity[value_overwrite:FALSE] */
    rv = PHYMOD_E_UNAVAIL;
#endif
#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_tx_preamble_length_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, preamble_length);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief configures Control/Pause/PFC Frame Drop settings
 *
 *	This API is used to configure frame drop settings.
 *
 *  @param mac_info        Represents MAC access\n
 *                         Configure data_path_dir(bcm_plp_data_path_direction_t) to select path:\n
 *                         NONE = 0,\n
 *                         Egress 1,\n
 *                         Ingress 2,\n
 *                         EgressIngress 3\n
 *  @param frame
 *         Control Frame   : bcmplpFrameControl 0\n
 *         Pause  Frame    : bcmplpFramePause 1\n
 *         PFC Frame       : bcmplpFramePFC 2\n
 *  @param enable  [IN]    Represents Enable/Disable\n
 *                         0 - disable\n
 *                         1 - enable\n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_configure_frame_drop_set(bcm_plp_mac_access_t mac_info, bcm_plp_mac_frame_select_t frame, uint32_t enable)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
#ifdef PHYMOD_MIURA_SUPPORT
    miura_pm_drop_frame_control_t drop_frame_control;
#endif
#ifdef PHYMOD_EVORA_SUPPORT
    evora_pm_drop_frame_control_t drop_frame_control;
#endif

    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_MIURA_SUPPORT
    drop_frame_control.enable=(uint8_t)enable;
    drop_frame_control.frame_select=(uint8_t)frame;
    drop_frame_control.frame_direction=(uint8_t)mac_info.phy_info.data_path_dir;
    rv = miura_pm_configure_frame_drop_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &drop_frame_control);
#endif
#ifdef PHYMOD_EVORA_SUPPORT
    drop_frame_control.enable=(uint8_t)enable;
    drop_frame_control.frame_select=(uint8_t)frame;
    drop_frame_control.frame_direction=(uint8_t)mac_info.phy_info.data_path_dir;
    rv = evora_pm_configure_frame_drop_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &drop_frame_control);
#endif
#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = xgbaset_mac_ctrl_frame_drop_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                         (portmod_mac_ctrl_frame_type_t) frame, enable);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    /* coverity[value_overwrite:FALSE] */
    rv = PHYMOD_E_UNAVAIL;
#endif
ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief get configured Control/Pause/PFC Frame Drop settings
 *
 *	This API is used to get configure frame drop settings.
 *
 *  @param mac_info        Represents MAC access\n
 *                         Configure data_path_dir(bcm_plp_data_path_direction_t) to select path:\n
 *                         NONE = 0,\n
 *                         Egress,\n
 *                         Ingress,\n
 *                         EgressIngress, \n
 *  @param frame
 *         Control Frame   : bcmplpFrameControl 0\n
 *         Pause  Frame    : bcmplpFramePause 1\n
 *         PFC Frame       : bcmplpFramePFC 2\n
 *  @param enable [OUT]    Represents Enable/Disable\n
 *                         0 - disable\n
 *                         1 - enable\n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_configure_frame_drop_get(bcm_plp_mac_access_t mac_info, bcm_plp_mac_frame_select_t frame, uint32_t * enable)
{
    int rv = BCM_PM_IF_SUCCESS;
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
#ifdef PHYMOD_MIURA_SUPPORT
    miura_pm_drop_frame_control_t drop_frame_control;
#endif
#ifdef PHYMOD_EVORA_SUPPORT
    evora_pm_drop_frame_control_t drop_frame_control;
#endif

    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;

#ifdef PHYMOD_MIURA_SUPPORT
    drop_frame_control.frame_select=(uint8_t)frame;
    drop_frame_control.frame_direction=(uint8_t)mac_info.phy_info.data_path_dir;
    rv = miura_pm_configure_frame_drop_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &drop_frame_control);
    *enable=(uint32_t)drop_frame_control.enable;
#endif
#ifdef PHYMOD_EVORA_SUPPORT
    drop_frame_control.frame_select=(uint8_t)frame;
    drop_frame_control.frame_direction=(uint8_t)mac_info.phy_info.data_path_dir;
    rv = evora_pm_configure_frame_drop_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &drop_frame_control);
    *enable=(uint32_t)drop_frame_control.enable;
#endif
#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = xgbaset_mac_ctrl_frame_drop_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy,
                                         (portmod_mac_ctrl_frame_type_t) frame, enable);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    /* coverity[value_overwrite:FALSE] */
    rv = PHYMOD_E_UNAVAIL;
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}
/*! \brief configures pause control 
 *
 *	This API is used to configure pause control. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param pause_control   
 *          rx_enable          : Enable RX pause 
 *          tx_enable          : Enable Tx pause
 *          refresh_timer      : use -1 for disable this feature; 
 *                               Threshold for pause timer to cause XOFF to be resent 
 *          xoff_timer         : Time value sent in the Timer Field for classes in XOFF state </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_pause_control_set(bcm_plp_mac_access_t mac_info, bcm_plp_mac_pause_control_t pause_control)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_pause_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pause_control_t*)&pause_control);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_pause_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pause_control_t*)&pause_control);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_pause_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pause_control_t*)&pause_control);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_pause_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pause_control_t*)&pause_control);
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_pause_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pause_control_t*) &pause_control);
#endif
ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief Get configured pause control 
 *
 *	This API is used to get configured pause control. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param pause_control   output,
 *          rx_enable          : Enable RX pause 
 *          tx_enable          : Enable Tx pause
 *          refresh_timer      : use -1 for disable this feature; 
 *                               Threshold for pause timer to cause XOFF to be resent 
 *          xoff_timer         : Time value sent in the Timer Field for classes in XOFF state </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_pause_control_get(bcm_plp_mac_access_t mac_info, bcm_plp_mac_pause_control_t *pause_control)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_pause_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pause_control_t*)pause_control);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_pause_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pause_control_t*)pause_control);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_pause_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pause_control_t*)pause_control);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_pause_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pause_control_t*)pause_control);
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_pause_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pause_control_t*) pause_control);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief configures PFC control 
 *
 *	This API is used to configure PFC control. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param pfc_ctrl   
 *          rx_enable    : Enable PFC Rx \n
 *           tx_enable   : Enable PFC Tx\n
 *           stats_en    : Enable PFC counters \n
 *           force_xon   : Instructs MAC to send Xon message to all classes of service \n
 *           refresh_timer :  use -1 for disable this feature; Threshold for pause timer to cause XOFF to be resent \n
 *           xoff_timer  : Time value sent in the Timer Field for classes in XOFF state </pre>
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */

int bcm_plp_aperta_mac_pfc_control_set(bcm_plp_mac_access_t mac_info, bcm_plp_mac_pfc_control_t pfc_ctrl)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_pfc_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pfc_control_t*)&pfc_ctrl);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_pfc_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pfc_control_t*)&pfc_ctrl);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_pfc_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pfc_control_t*)&pfc_ctrl);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_pfc_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pfc_control_t*)&pfc_ctrl);
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_pfc_control_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pfc_control_t*) &pfc_ctrl);
#endif
ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief Get configured PFC control 
 *
 *	This API is used to get configured PFC control. 
 *
 *  @param mac_info <pre>  Represents MAC access\n
 *  @param pfc_ctrl       output,
 *          rx_enable    : Enable PFC Rx\n
 *           tx_enable   : Enable PFC Tx\n
 *           stats_en    : Enable PFC counters \n
 *           force_xon   : Instructs MAC to send Xon message to all classes of service \n
 *           refresh_timer :  use -1 for disable this feature; Threshold for pause timer to cause XOFF to be resent \n
 *           xoff_timer  : Time value sent in the Timer Field for classes in XOFF state</pre> 
 *	       
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_pfc_control_get(bcm_plp_mac_access_t mac_info, bcm_plp_mac_pfc_control_t *pfc_ctrl)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_pfc_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pfc_control_t*)pfc_ctrl);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_pfc_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pfc_control_t*)pfc_ctrl);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_pfc_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pfc_control_t*)pfc_ctrl);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_pfc_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pfc_control_t*)pfc_ctrl);
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_pfc_control_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (portmod_pfc_control_t*) pfc_ctrl);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief Dump PM Diag information 
 *
 *	This API is used to dump PM diagnostic information
 *
 *  @param mac_info  Represents MAC access\n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_diagnostic_dump(bcm_plp_mac_access_t mac_info)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pm_diagnostic_dump(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = miura_pm_diagnostic_dump(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_diagnostic_dump(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_diagnostic_dump(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    (void) mac_info;
    rv = xgbaset_mac_diagnostic_dump(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}
/*! \brief  Get Enable/Disable state of MAC Tx/Rx of a port
 *
 *	This API is used to get Enable/Disable state of MAC transmitter or MAC receiver of a port 
 *	on the specified interface side.
 *
 *  @param mac_info  Represents MAC access\n
 *  @param tx_rx     Represents MAC Transmitter or Receiver
 *                   0 : Represents MAC Transmitter
 *                   1 : Represents MAC Receiver
 *  @param enable_disable  [Out] Represents enable/disable of Tx/Rx
 *                   0 : Represents Disabled of Tx/Rx
 *                   1 : Represents Enabled of Tx/Rx
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_port_enable_get(bcm_plp_mac_access_t mac_info, int tx_rx, int *enable_disable)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
 
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);

#ifdef PHYMOD_EVORA_SUPPORT
    if (tx_rx == 1) {
        rv = evora_rx_mac_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable_disable);
    }
    if (tx_rx == 0) {
        rv = evora_tx_mac_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable_disable);
    }
#endif

#ifdef PHYMOD_MIURA_SUPPORT
    if (tx_rx == 1) {
        rv = miura_rx_mac_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable_disable);
    }
    if (tx_rx == 0) {
        rv = miura_tx_mac_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable_disable);
    }

#endif
#ifdef PHYMOD_APERTA_SUPPORT
    if (tx_rx == 1) {
        rv = plp_aperta_rx_mac_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable_disable);
    }
    if (tx_rx == 0) {
        rv = plp_aperta_tx_mac_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable_disable);
    }

#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    if (tx_rx == 1) {
        rv = aperta2_rx_mac_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable_disable);
    }
    if (tx_rx == 0) {
        rv = aperta2_tx_mac_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable_disable);
    }

#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    if (tx_rx == 1) {
        rv = _xlmac_rx_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable_disable);
    }
    if (tx_rx == 0) {
        rv = _xlmac_tx_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, enable_disable);
    }
#endif
ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief  Enable/Disable MAC Tx/Rx of a port
 *
 *	This API is used to Enable/Disable MAC transmitter or MAC receiver of a port 
 *	on the specified interface side.
 *
 *  @param mac_info  Represents MAC access\n
 *  @param tx_rx     Represents MAC Transmitter or Receiver
 *                   0 : Represents MAC Transmitter
 *                   1 : Represents MAC Receiver
 *  @param enable_disable  Represents enable/disable of Tx/Rx
 *                   0 : Represents Disabled of Tx/Rx
 *                   1 : Represents Enabled of Tx/Rx
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_port_enable_set(bcm_plp_mac_access_t mac_info, int tx_rx, int enable_disable)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
 
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);

#ifdef PHYMOD_EVORA_SUPPORT
    if (tx_rx == 1) {
        rv = evora_rx_mac_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (enable_disable == 1)? 1 : 0);
    }
    if (tx_rx == 0) {
        rv = evora_tx_mac_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (enable_disable == 1)? 1 : 0);
    }
#endif

#ifdef PHYMOD_MIURA_SUPPORT
    if (tx_rx == 1) {
        rv = miura_rx_mac_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (enable_disable == 1)? 1 : 0);
    }
    if (tx_rx == 0) {
        rv = miura_tx_mac_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (enable_disable == 1)? 1 : 0);
    }
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    if (tx_rx == 1) {
        rv = plp_aperta_rx_mac_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (enable_disable == 1)? 1 : 0);
    }
    if (tx_rx == 0) {
        rv = plp_aperta_tx_mac_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (enable_disable == 1)? 1 : 0);
    }

#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    if (tx_rx == 1) {
        rv = aperta2_rx_mac_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (enable_disable == 1)? 1 : 0);
    }
    if (tx_rx == 0) {
        rv = aperta2_tx_mac_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (enable_disable == 1)? 1 : 0);
    }

#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    if (tx_rx == 1) {
        rv = _xlmac_rx_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (enable_disable == 1)? 1 : 0);
    }
    if (tx_rx == 0) {
        rv = _xlmac_tx_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (enable_disable == 1)? 1 : 0);
    }
#endif
ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief  Datapath flushing 
 *
 *	This API is used to flush datapath on ingress/egress direction
 *
 *  @param mac_info  Represents MAC access\n
 *                   NOTE: use phy_info(data_path_dir) to choose direction.
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_datapath_flush(bcm_plp_mac_access_t mac_info)
{
    uint32_t phy_id_idx, port=0;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    uint64_t data = 0;
    int wait_count = 0;
    unsigned int regaddr = 0;
#ifdef INCLUDE_PLP_MACSEC
    uint64_t ingress_data=0, egress_data=0;
    uint32_t ingress_inflight = 0, egress_inflight = 0 ;
#endif    

    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    BCM_PLP_MAC_GET_PORT(mac_info.phy_info.lane_map, port);

#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_flush_port(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_flush_port(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    /* Macsec_config_EIP_CTRL */
    regaddr = 0xF1001020;
    BCM_PLP_MAC_IF_ERR_GOTO_ERR(
        xlmac_reg64_read(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, regaddr, &data));

    /* Set the eip_flush_start */
    data |= 1;

    BCM_PLP_MAC_IF_ERR_GOTO_ERR(
        xlmac_reg64_write(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, regaddr, data));

    BCM_PLP_MAC_IF_ERR_GOTO_ERR(
        xlmac_reg64_read(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, regaddr, &data));

    /* Macsec_config_LIVE_STAT */
    regaddr = 0xF1001024;
    data = 0;
    do {
        BCM_PLP_MAC_IF_ERR_GOTO_ERR(
            xlmac_reg64_read(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, regaddr, &data));
        if ((data & 1) == 0) {
            break;
        }
        PHYMOD_USLEEP(100);
    } while (wait_count++ < 100);
    (void) port;
#else
#ifndef PHYMOD_APERTA_SUPPORT
#ifdef INCLUDE_PLP_MACSEC
    /* Ingress*/
    if (mac_info.phy_info.data_path_dir == bcmplpDataPathDirectionIngress || 
           mac_info.phy_info.data_path_dir == bcmplpDataPathDirectionEgressIngress ) {
        BCM_PLP_MAC_IF_ERR_GOTO_ERR(
                bcm_plp_aperta_reg64_value_get_helper(mac_info.phy_info, 1, 0x4400FFE0 , &ingress_data));
    }
    /* Egress*/
    if (mac_info.phy_info.data_path_dir == bcmplpDataPathDirectionEgress || 
           mac_info.phy_info.data_path_dir == bcmplpDataPathDirectionEgressIngress ) {
        BCM_PLP_MAC_IF_ERR_GOTO_ERR(
                bcm_plp_aperta_reg64_value_get_helper(mac_info.phy_info, 1, 0x4600FFE0 , &egress_data));
    }
    if (egress_data & (1 << port)) {
        egress_inflight = 1;
    } else {
        egress_inflight = 0;
    }
    if (ingress_data & (1 << port)) {
        ingress_inflight = 1;
    } else {
        ingress_inflight = 0;
    }

    if (ingress_inflight) {
#endif
        if (mac_info.phy_info.data_path_dir == bcmplpDataPathDirectionIngress || 
            mac_info.phy_info.data_path_dir == bcmplpDataPathDirectionEgressIngress ) {
            BCM_PLP_MAC_IF_ERR_GOTO_ERR(
                    bcm_plp_aperta_reg64_value_get_helper(mac_info.phy_info, 1, 0x49003001, &data));
            data &= ~(0x11 << port);
            BCM_PLP_MAC_IF_ERR_GOTO_ERR(
                    bcm_plp_aperta_reg64_value_set_helper(mac_info.phy_info, 1, 0x49003001, data));
            BCM_PLP_MAC_IF_ERR_GOTO_ERR(
                    bcm_plp_aperta_reg64_value_get_helper(mac_info.phy_info, 1, 0x49003001, &data));
            data |= (0x11 << port);
            BCM_PLP_MAC_IF_ERR_GOTO_ERR(
                    bcm_plp_aperta_reg64_value_set_helper(mac_info.phy_info, 1, 0x49003001, data));
        }
#ifdef INCLUDE_PLP_MACSEC
    }
    if (egress_inflight) {
#endif    
        if (mac_info.phy_info.data_path_dir == bcmplpDataPathDirectionEgress || 
            mac_info.phy_info.data_path_dir == bcmplpDataPathDirectionEgressIngress ) {
            BCM_PLP_MAC_IF_ERR_GOTO_ERR(
                    bcm_plp_aperta_reg64_value_get_helper(mac_info.phy_info, 1, 0x49004001, &data));
            data &= ~(0x11 << port);
            BCM_PLP_MAC_IF_ERR_GOTO_ERR(
                    bcm_plp_aperta_reg64_value_set_helper(mac_info.phy_info, 1, 0x49004001, data));
            BCM_PLP_MAC_IF_ERR_GOTO_ERR(
                    bcm_plp_aperta_reg64_value_get_helper(mac_info.phy_info, 1, 0x49004001, &data));
            data |= (0x11 << port);
            BCM_PLP_MAC_IF_ERR_GOTO_ERR(
                    bcm_plp_aperta_reg64_value_set_helper(mac_info.phy_info, 1, 0x49004001, data));
        }
#ifdef INCLUDE_PLP_MACSEC
    }
#endif
#else
#ifdef INCLUDE_PLP_MACSEC
    (void) ingress_inflight;
    (void) egress_inflight;
    (void) ingress_data;
    (void) egress_data;
#endif
#endif /* PHYMOD_APERTA_SUPPORT*/
    (void) data;
    (void) port;
    (void) wait_count;
    (void) regaddr;
#endif
ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief PCS Diag information 
 *
 *	This API is used to dump PCS diagnostic information
 *
 *  @param mac_info  Represents MAC access\n
 *
 *	@return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_pcs_diagnostic_dump(bcm_plp_mac_access_t mac_info)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;
    
    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;
#ifdef PHYMOD_EVORA_SUPPORT
    rv = evora_pcs_info_dump(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
#endif
#ifdef PHYMOD_MIURA_SUPPORT
    rv = BCM_PM_IF_UNAVAIL;
#endif
#ifdef PHYMOD_APERTA_SUPPORT
    plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags = mac_info.phy_info.flags;
    rv = plp_aperta_pcs_info_dump(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
    plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= ~(mac_info.phy_info.flags);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags = mac_info.phy_info.flags;
    rv = aperta2_pcs_info_dump(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy);
    plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy.access.flags &= ~(mac_info.phy_info.flags);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;

}

/*! \brief Set MAC reset
 *
 *  @param mac_info  Represents MAC access\n
 *  @param reset   1 = Put MAC in Soft Reset \n
 *                 0 = Take out of reset \n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_reset_set(bcm_plp_mac_access_t mac_info, int reset)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_soft_reset_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, reset);
#endif
ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief Get MAC reset status
 *
 *  @param mac_info  Represents MAC access\n
 *  @param reset   1 = MAC is in Soft Reset \n
 *                 0 = Out of reset \n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_reset_get(bcm_plp_mac_access_t mac_info, int *reset)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;

#if defined(PHYMOD_WHITETIP_SUPPORT) || defined(PHYMOD_LONGFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT)
    rv = _xlmac_soft_reset_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, reset);
#endif
ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief Enable PM timesync
 *
 *	This API is used to enable PM based timesync
 *  @param mac_info  Represents MAC access\n
 *  @param flags     Represents One step or two step process pipeline\n
 *                   0x1 : Enable Rx
 *                   0x4 : Enable One Step processing
 *                   BELOW FLAGS APPLICABLE ONLY FOR APERTA2 
 *                   0x8  : Enable -  802.3 CX mode. Disabled - Legacy Broadcom mode will be used.
 *                   0x10 : Enable SOP timestamp mode, ignore MAC_DA bit. If disable, check MAC_DA mode bit. 
 *                   0x20 : Enable MAC_DA timestamp mode. Disabled, SFD timestamp mode will be used. 
 *                   0x40 : Port is in reduced preamble mode. 
 *                   bit 31-28 : Represents bcm_plp_1588_config_t
 *  @param enable    Enable or disable PM 1588  \n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_pm_timesync_enable_set(bcm_plp_mac_access_t mac_info, uint32_t flags, uint32_t enable)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;

#ifdef PHYMOD_APERTA_SUPPORT
    rv =  plp_aperta_pm_timesync_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags, enable);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv =  aperta2_pm_timesync_enable_set(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags, enable);
#endif
ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief Enable PM timesync get
 *
 *	This API is used to enable state of PM based timesync
 *  @param mac_info  Represents MAC access\n
 *  @param flags     Represents One step or two step process pipeline\n
 *                   0x4 : Get the enabled status of One Step processing
 *
 *  @param enable    Enable or disable PM 1588  \n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_pm_timesync_enable_get(bcm_plp_mac_access_t mac_info, uint32_t flags, uint32_t *enable)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;

#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_pm_timesync_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags, enable) ;
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_timesync_enable_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, flags, enable) ;
#endif
ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief Enable PM timesync info get
 *
 *	This API is used to Tx information for PM based timesync
 *  @param mac_info      Represents MAC access\n
 *  @param ts_tx_info    Tx information  \n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_pm_timesync_tx_info_get(bcm_plp_mac_access_t mac_info, bcm_plp_pm_ts_tx_info_t* ts_tx_info)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;

#ifdef PHYMOD_APERTA_SUPPORT
    {
        aperta_pm_ts_tx_info_t tx_info;
        rv = plp_aperta_pm_timesync_tx_info_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &tx_info);
        ts_tx_info->ts_in_fifo_lo = tx_info.ts_in_fifo_lo; 
        ts_tx_info->ts_in_fifo_hi = tx_info.ts_in_fifo_hi; 
        ts_tx_info->ts_seq_id     = tx_info.ts_seq_id;
        ts_tx_info->ts_sub_nanosec = tx_info.ts_sub_nanosec;
    }
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    {
        aperta2_pm_ts_tx_info_t tx_info;
        rv = aperta2_pm_timesync_tx_info_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, &tx_info);
        ts_tx_info->ts_in_fifo_lo = tx_info.ts_in_fifo_lo;
        ts_tx_info->ts_in_fifo_hi = tx_info.ts_in_fifo_hi;
        ts_tx_info->ts_seq_id     = tx_info.ts_seq_id;
        ts_tx_info->ts_sub_nanosec = tx_info.ts_sub_nanosec;
    }
#endif
ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    return rv;
}

/*! \brief MAC MIB Statistics get
 *
 *  This API is used to get MAC MIB statistics
 *  @param mac_info  Represents MAC access\n
 *  @param stat_type input, Represents the type of MIB counter\n
 *  @param count out, counter value  \n
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_mac_mib_stat_get(bcm_plp_mac_access_t mac_info, bcm_plp_mib_stat_type_t stat_type, plp_uint64_t *count)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;

#ifdef PHYMOD_APERTA_SUPPORT
    if (mac_info.phy_info.flags & BCM_PLP_CLEAR_MAC_STAT) {
        rv = plp_aperta_pm_mac_mib_stat_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, 0xFFFF, count);
    } else {
        rv = plp_aperta_pm_mac_mib_stat_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, stat_type, count);
    }
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    rv = aperta2_pm_mac_mib_stat_get(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, stat_type, count);
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);

    return rv;
}

/*! \brief Port config update
 *
 *  This API is used to update port configuration i.e. port latency, PTP latency
 *  @param mac_info  Represents MAC access\n
 *  @param prt_cfg input, Represents the port configuration i.e. port latency, ptp latency\n
 *                        refer bcm_plp_<chip>_port_config_t
 *
 *  @return BCM_PM_IF_SUCCESS(0) for success and corresponding error code on failure
 */
int bcm_plp_aperta_update_port_config(bcm_plp_mac_access_t mac_info, void *prt_cfg)
{
    uint32_t phy_id_idx;
    uint32_t exist_phy = 0;
    int rv = BCM_PM_IF_SUCCESS;

    BCM_PLP_MAC_GET_P_CTXT(mac_info.phy_info, phy_id_idx, exist_phy);
    BCM_PM_MUTEX_LOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);
    BCM_PLP_MAC_FILL_PHY_ACCESS(mac_info.phy_info, phy_id_idx);
    rv = BCM_PM_IF_UNAVAIL;

#ifdef PHYMOD_APERTA_SUPPORT
    rv = plp_aperta_update_port_config(&plp_aperta_phy_ctrl.phy[phy_id_idx]->pm_phy, (aperta_update_port_config_t*)prt_cfg);
#endif
#ifdef PHYMOD_APERTA2_SUPPORT
    /* coverity[value_overwrite:FALSE] */
    rv = PHYMOD_E_UNAVAIL;
#endif

ERR:
    BCM_PLP_MAC_API_UNAVAIL_CHECK(rv);
    BCM_PM_MUTEX_UNLOCK(mac_info.phy_info.phy_addr, mac_info.phy_info.platform_ctxt);

    return rv;
}

#endif
