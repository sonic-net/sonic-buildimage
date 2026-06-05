/*
 * $Id: $
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */

#include <stddef.h>
#include "timesync.h"

#if   defined(PHYMOD_EVORA_SUPPORT) || defined(PHYMOD_EUROPA_SUPPORT)
  #include "evora_reg_access.h"
  #define  HSIP_PHY_IEEE1588_REG_READ      evora_chip_ind_raw_read
  #define  HSIP_PHY_IEEE1588_REG_WRITE     evora_chip_ind_raw_write
  #define  HSIP_PHY_REG_READ               phymod_bcm_evora_read
  #define  HSIP_PHY_REG_WRITE              phymod_bcm_evora_write
#elif defined(PHYMOD_MIURA_SUPPORT)
  #include "miura_reg_access.h"
  #define  HSIP_PHY_IEEE1588_REG_READ      miura_chip_ind_raw_read
  #define  HSIP_PHY_IEEE1588_REG_WRITE     miura_chip_ind_raw_write
#elif defined(PHYMOD_APERTA_SUPPORT)
  #include "aperta_reg_access.h"
  #define  HSIP_PHY_IEEE1588_REG_READ      plp_aperta_reg32_read
  #define  HSIP_PHY_IEEE1588_REG_WRITE     plp_aperta_reg32_write
  #define  HSIP_PHY_REG_READ               plp_aperta_direct_reg_read
  #define  HSIP_PHY_REG_WRITE              plp_aperta_direct_reg_write
  #define  TSACC(_p)                       (_p)                /* Aperta uses plp_aperta_phymod_phy_access_t */
#elif defined(PHYMOD_QUADRA28_SUPPORT)
  #include "quadra28_reg_access.h"
  #define  HSIP_PHY_IEEE1588_REG_READ      phymod_raw_iblk_read
  #define  HSIP_PHY_IEEE1588_REG_WRITE     phymod_raw_iblk_write
#elif defined(PHYMOD_XGBASET_SUPPORT) || defined(PHYMOD_MGAUTO_SUPPORT) \
                                      || defined(PHYMOD_PHY848XX_SUPPORT)
  #define  XGP_PF_RW_REG_00   0x001e4110  /* 30.0x4110 Device R/W Access Definition 00 */
  #define  XGP_PF_RW_REG_XFI  0x2004
#else
  #error   IEEE1588 not supported !!
#endif

#if ! defined(TSACC)
#define  TSACC(_p)          (&((_p)->access))       /* pointer to plp_aperta_phymod_access_t */
#endif

int
plp_aperta_p1588_reg_read(const plp_aperta_phymod_phy_access_t *pa, uint32_t reg1588, uint32_t *data)
{
#if   defined(PHYMOD_EVORA_SUPPORT) || defined(PHYMOD_EUROPA_SUPPORT) || defined(PHYMOD_MIURA_SUPPORT) \
                                    || defined(PHYMOD_APERTA_SUPPORT) || defined(PHYMOD_QUADRA28_SUPPORT)
  #if defined(PHYMOD_EVORA_SUPPORT) || defined(PHYMOD_EUROPA_SUPPORT) || defined(PHYMOD_APERTA_SUPPORT)
    if ( reg1588 > PLP_P1588_INBAND_SOFT_COMMANDr ) {   /* 1588 related, non-1588 registers */
        return  HSIP_PHY_REG_READ(TSACC(pa), reg1588, data);
    }
  #endif
    return  HSIP_PHY_IEEE1588_REG_READ(TSACC(pa), (reg1588 | PLP_P1588_REG_BASE), data);

#elif defined(PHYMOD_XGBASET_SUPPORT) || defined(PHYMOD_MGAUTO_SUPPORT) \
                                      || defined(PHYMOD_PHY848XX_SUPPORT)
    uint32_t  xgp;

    /* set XGP table to XFI read/write for accessing P1588M registers */
    PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_READ(TSACC(pa), XGP_PF_RW_REG_00, &xgp) );
    if ( xgp != XGP_PF_RW_REG_XFI ) {
        PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_WRITE(TSACC(pa), XGP_PF_RW_REG_00, XGP_PF_RW_REG_XFI) );
    }

    /* Read P1588M register */
    PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_READ(TSACC(pa), (reg1588 | PLP_P1588_REG_BASE), data) );

    /* resume XGP table */
    if ( xgp != XGP_PF_RW_REG_XFI ) {
        PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_WRITE(TSACC(pa), XGP_PF_RW_REG_00, xgp) );
    }
#endif
    PHYMOD_DEBUG_INFO(("- - p=%d 1588Reg.0x%02x  = 0x%04x\n", pa->access.addr, reg1588, *data));
    return PHYMOD_E_NONE;
}

int
plp_aperta_p1588_reg_write(const plp_aperta_phymod_phy_access_t *pa, uint32_t reg1588, uint32_t data)
{
#if   defined(PHYMOD_EVORA_SUPPORT) || defined(PHYMOD_EUROPA_SUPPORT) || defined(PHYMOD_MIURA_SUPPORT) \
                                    || defined(PHYMOD_APERTA_SUPPORT) || defined(PHYMOD_QUADRA28_SUPPORT)
  #if defined(PHYMOD_EVORA_SUPPORT) || defined(PHYMOD_EUROPA_SUPPORT) || defined(PHYMOD_APERTA_SUPPORT)
    if ( reg1588 > PLP_P1588_INBAND_SOFT_COMMANDr ) {   /* 1588 related, non-1588 registers */
        return  HSIP_PHY_REG_WRITE(TSACC(pa), reg1588, data);
    }
  #endif
    return  HSIP_PHY_IEEE1588_REG_WRITE(TSACC(pa), (reg1588 | PLP_P1588_REG_BASE), data);

#elif defined(PHYMOD_XGBASET_SUPPORT) || defined(PHYMOD_MGAUTO_SUPPORT) \
                                      || defined(PHYMOD_PHY848XX_SUPPORT)
    uint32_t  xgp;

    /* set XGP table to XFI read/write for accessing P1588M registers */
    PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_READ(TSACC(pa), XGP_PF_RW_REG_00, &xgp) );
    if ( xgp != XGP_PF_RW_REG_XFI ) {
        PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_WRITE(TSACC(pa), XGP_PF_RW_REG_00, XGP_PF_RW_REG_XFI) );
    }

    /* Read P1588M register */
    PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_WRITE(TSACC(pa), (reg1588 | PLP_P1588_REG_BASE), data) );

    /* resume XGP table */
    if ( xgp != XGP_PF_RW_REG_XFI ) {
        PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_WRITE(TSACC(pa), XGP_PF_RW_REG_00, xgp) );
    }
#endif
    PHYMOD_DEBUG_INFO(("- - p=%d 1588Reg.0x%02x <= 0x%04x\n", pa->access.addr, reg1588,  data));
    return PHYMOD_E_NONE;
}

/* prepare for p1588 indirect register read/write */
int plp_aperta_p1588_indir_reg_ctrl(const plp_aperta_phymod_phy_access_t *pa, int addr, int rw) {
    P1588_SOPMEM_CONTROLr_t  sopctrl;
    PHYMOD_MEMSET(&sopctrl, 0, sizeof(P1588_SOPMEM_CONTROLr_t));

    /* P1588 SOP Memory Control Type Register */
    P1588_SOPMEM_CONTROLr_SOPMEM_WRITE_ACCESS_SET(   sopctrl, INDIR_REG_WR(rw));
    P1588_SOPMEM_CONTROLr_SOPMEM_READ_ACCESS_SET(    sopctrl, INDIR_REG_RD(rw));
    P1588_SOPMEM_CONTROLr_SOPMEM_SUB_INDEXf_SET(     sopctrl, addr);
    P1588_SOPMEM_CONTROLr_SOPMEM_LOAD_INDEXf_SET(    sopctrl, 0x1U);
    P1588_SOPMEM_CONTROLr_SOPMEM_AUTO_INCREMENTf_SET(sopctrl, 0x0U);
    P1588_SOPMEM_CONTROLr_SOPMEM_INDEXf_SET(         sopctrl, 0x0U);

    PHYMOD_IF_ERR_RETURN( WRITE_P1588_SOPMEM_CONTROLr(pa, sopctrl) );
    PHYMOD_USLEEP(1000);    /* wait for a msec for the setting to take effect */
    return PHYMOD_E_NONE;
}

#ifdef BCM_PLP_TIMESYNC_V2_1_SUPPORT

/* p1588 indirect register write */
int p1588_indir_reg_write(const plp_aperta_phymod_phy_access_t *pa, int addr, uint32_t data)
{
    PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_indir_reg_ctrl(pa, addr, INDIR_REG_WRITE) );
    PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_write(pa, PLP_P1588_SOPMEM_WDATAr, data) );

    PHYMOD_DEBUG_INFO(("port(%d.%x) indirect reg.%02x  <= 0x%04x\n",
                             TSACC(pa)->addr, TSACC(pa)->lane_mask, addr,  data));
    return PHYMOD_E_NONE;
}

/* p1588 indirect register read  */
int p1588_indir_reg_read(const plp_aperta_phymod_phy_access_t *pa, int addr, uint32_t *data)
{
    PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_indir_reg_ctrl(pa, addr, INDIR_REG_READ ) );
    PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_read( pa, PLP_P1588_SOPMEM_RDATAr, data) );

    PHYMOD_DEBUG_INFO(("port(%d.%x) indirect reg.%02d  =  0x%04x\n",
                             TSACC(pa)->addr, TSACC(pa)->lane_mask, addr, *data) );
    return PHYMOD_E_NONE;
}

#endif  /* BCM_PLP_TIMESYNC_V2_1_SUPPORT */

