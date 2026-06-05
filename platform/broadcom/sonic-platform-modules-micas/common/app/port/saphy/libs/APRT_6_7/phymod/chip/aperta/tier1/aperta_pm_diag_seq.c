/*
*
* $Id: aperta_pm_seq.c, 2016/03/21 $
*
*  *
*  *
  * $Copyright: (c) 2020 Broadcom.
  * Broadcom Proprietary and Confidential. All rights reserved.$
*  *
*
*/

/*
 * Includes
 */
#include <phymod/phymod.h>
#include <phymod/phymod_diagnostics.h>
#include "aperta_pm_diag_seq.h"
#include "aperta_pm_seq.h"
#include "aperta_cfg_seq.h"
#include <include/pm8x50_shared.h>
#include <include/portmod.h>

extern aperta_pm_info_t _plp_aperta_pm_info[APERTA_MAX_PM_INFO];
int plp_aperta_pm_prbs_config_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , const plp_aperta_phymod_prbs_t* prbs)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_GET_PORT_UPDATE_PM_INFO_LM(phy, port);

    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_port_prbs_config_set(unit, port, portmodPrbsModePhy, flags, prbs));
    return PHYMOD_E_NONE;
}

int plp_aperta_pm_prbs_config_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , plp_aperta_phymod_prbs_t* prbs)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_GET_PORT_UPDATE_PM_INFO_LM(phy, port);

    PHYMOD_IF_ERR_RETURN(
        plp_aperta_portmod_port_prbs_config_get(unit, port, portmodPrbsModePhy, flags, prbs));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_prbs_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , uint32_t enable)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_GET_PORT_UPDATE_PM_INFO_LM(phy, port);

    PHYMOD_IF_ERR_RETURN(
        plp_aperta_portmod_port_prbs_enable_set(unit, port, portmodPrbsModePhy, flags, (int) enable));

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_prbs_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags , uint32_t* enable)
{
    int unit = 0, int_port = 0, port = 0;
    int prbs_enable = 0;

    APERTA_GET_PORT_UPDATE_PM_INFO_LM(phy, port);

    PHYMOD_IF_ERR_RETURN(
        plp_aperta_portmod_port_prbs_enable_get(unit, port, portmodPrbsModePhy, flags, &prbs_enable));
    *enable = prbs_enable ? 1 : 0;

    return PHYMOD_E_NONE;
}

int plp_aperta_pm_prbs_status_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, plp_aperta_phymod_prbs_status_t* prbs_status)
{
    int unit = 0, int_port = 0, port = 0;

    APERTA_GET_PORT_UPDATE_PM_INFO_LM(phy, port);

    PHYMOD_IF_ERR_RETURN(
        plp_aperta_portmod_port_prbs_status_get(unit, port, portmodPrbsModePhy, flags, prbs_status));

    return PHYMOD_E_NONE;
}

