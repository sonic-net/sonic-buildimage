/*
 * $Id: $
 *
 * $Copyright: (c) 2022 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */

#include <stddef.h>
#include "timesync.h"

#if defined(PHYMOD_APERTA2_SUPPORT)
  #include "aperta2_reg_access.h"
  #define  HSIP_PHY_IEEE1588_REG_READ      plp_aperta2_reg32_read
  #define  HSIP_PHY_IEEE1588_REG_WRITE     plp_aperta2_reg32_write
  #define  HSIP_PHY_REG_READ               plp_aperta2_direct_reg_read
  #define  HSIP_PHY_REG_WRITE              plp_aperta2_direct_reg_write
  #define  TSACC(_p)                       (_p)            /* uses plp_aperta2_phymod_phy_access_t */
#endif

#if ! defined(TSACC)
#define  TSACC(_p)                         &((_p)->access) /* uses  plp_aperta2_phymod_access_t    */
#endif

int plp_aperta2_p1588_reg_read(const plp_aperta2_phymod_phy_access_t *pa, uint32_t reg1588, uint32_t *data)
{
    return  HSIP_PHY_IEEE1588_REG_READ(TSACC(pa), (reg1588 | PLP_P1588_REG_BASE), data);
}

int plp_aperta2_p1588_reg_write(const plp_aperta2_phymod_phy_access_t *pa, uint32_t reg1588, uint32_t data)
{
    return  HSIP_PHY_IEEE1588_REG_WRITE(TSACC(pa), (reg1588 | PLP_P1588_REG_BASE), data);
}
