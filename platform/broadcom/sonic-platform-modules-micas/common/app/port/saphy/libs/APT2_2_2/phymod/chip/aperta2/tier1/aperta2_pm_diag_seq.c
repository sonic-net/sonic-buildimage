/*
*
* $Id: aperta2_pm_seq.c,  $
*
*  *
*  *
  * $Copyright: (c) 2022 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
*  *
*
*/

/*
 * Includes
 */
#include <phymod/phymod.h>
#include <phymod/phymod_diagnostics.h>
#include <aperta2_pm_diag_seq.h>
#include <aperta2_pm_seq.h>
#include <aperta2_cfg_seq.h>
#include <include/pm8x100_gen2_shared.h>
#include <include/portmod.h>

extern aperta2_pm_info_t _plp_aperta2_pm_info[APERTA2_MAX_PM_INFO];


/*!
 *  Function to configure PRBS
 *
 *  @param phy             phy configuration 
 *  @param flags           represents Tx/Rx/Both
 *  @param prbs            prbs to be configured
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_pm_prbs_config_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags , const plp_aperta2_phymod_prbs_t* prbs)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_GET_PORT_UPDATE_PM_INFO_LM(phy, port);

    PHYMOD_IF_ERR_RETURN(plp_aperta2_portmod_port_prbs_config_set(unit, port, portmodPrbsModePhy, flags, prbs));
    return PHYMOD_E_NONE;
}

/*!
 *  Function to retreive PRBS configuration
 *
 *  @param phy             phy configuration 
 *  @param flags           represents Tx/Rx/Both
 *  @param prbs            configured PRBS
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_pm_prbs_config_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags , plp_aperta2_phymod_prbs_t* prbs)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_GET_PORT_UPDATE_PM_INFO_LM(phy, port);

    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_portmod_port_prbs_config_get(unit, port, portmodPrbsModePhy, flags, prbs));

    return PHYMOD_E_NONE;
}

/*!
 *  Function to Enable Tx/RX of PRBS 
 *  
 *  @param phy             phy configuration 
 *  @param flags           represents Tx/Rx/Both
 *  @param enable          0 disable 1 enable
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_pm_prbs_enable_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags , uint32_t enable)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_GET_PORT_UPDATE_PM_INFO_LM(phy, port);

    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_portmod_port_prbs_enable_set(unit, port, portmodPrbsModePhy, flags, (int) enable));

    return PHYMOD_E_NONE;
}

/*!
 *  Function to Get Enabled status of Tx generator/RX checker
 *  
 *  @param phy             phy configuration 
 *  @param flags           represents Tx/Rx/Both
 *  @param enable          output, 0 disable 1 enable
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */

int plp_aperta2_pm_prbs_enable_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags , uint32_t* enable)
{
    int unit = 0, int_port = 0, port = 0;
    int prbs_enable = 0;

    APERTA2_GET_PORT_UPDATE_PM_INFO_LM(phy, port);

    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_portmod_port_prbs_enable_get(unit, port, portmodPrbsModePhy, flags, &prbs_enable));
    *enable = prbs_enable ? 1 : 0;

    return PHYMOD_E_NONE;
}

/*!
 *  Function to Get PRBS checker status
 *  
 *  @param phy             phy configuration 
 *  @param flags           reserved
 *  @param prbs_status     output, PRBS status
 *
 * @retval PHYMOD_E_NONE     On success and corresponding error code on failure.
 */
int plp_aperta2_pm_prbs_status_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, plp_aperta2_phymod_prbs_status_t* prbs_status)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA2_GET_PORT_UPDATE_PM_INFO_LM(phy, port);

    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_portmod_port_prbs_status_get(unit, port, portmodPrbsModePhy, flags, prbs_status));

    return PHYMOD_E_NONE;
}

