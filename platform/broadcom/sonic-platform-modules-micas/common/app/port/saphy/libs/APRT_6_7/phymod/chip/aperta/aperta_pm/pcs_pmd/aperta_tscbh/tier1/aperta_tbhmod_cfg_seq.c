/*----------------------------------------------------------------------
 * $Id: aperta_tbhmod_cfg_seq.c,v 1.8 Broadcom SDK $
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *---------------------------------------------------------------------
 * File       : aperta_tbhmod_cfg_seq.c
 * Description: c functions implementing Tier1s for TEFMod Serdes Driver
 *---------------------------------------------------------------------*/

#ifndef _DV_TB_
 #define _SDK_APERTA_TBHMOD_ 1
#endif

#ifdef _DV_TB_
#include <stdint.h>
#include "aperta_tbhmod_main.h"
#include "aperta_tbhmod_defines.h"
#include "aperta_tbhmod_spd_ctrl.h"
#include "aperta_tbhmod.h"
#include "tbhPCSRegEnums.h"
#include "tbhPMDRegEnums.h"
#include "phy_tsc_iblk.h"
#include "bcmi_tscbh_xgxs_defs.h"

#endif /* _DV_TB_ */

#ifdef _SDK_APERTA_TBHMOD_
#include <phymod/phymod.h>
#include <phymod/phymod_util.h>
#include <phymod/phymod_debug.h>
#include <bcmi_aperta_tscbh_xgxs_defs.h>
#include "aperta_tbhmod_enum_defines.h"
#include "aperta_tbhmod_spd_ctrl.h"
#include "aperta_tbhmod.h"
#include "aperta_tbhmod_sc_lkup_table.h"
#include "tbhPCSRegEnums.h"
#include <tier1/aperta_reg_access.h>
#endif /* _SDK_APERTA_TBHMOD_ */

#ifndef _SDK_APERTA_TBHMOD_
#define PHYMOD_ST const plp_aperta_phymod_phy_access_t
#endif

#ifdef _SDK_APERTA_TBHMOD_
  #define APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc) \
    PHYMOD_VDBG(APERTA_TBHMOD_DBG_FUNC,&pc->access,("-22%s: Adr:%08x Ln:%02d\n", __func__, pc->access.addr, pc->access.lane_mask))
  #define APERTA_TBHMOD_DBG_IN_FUNC_VIN_INFO(pc,_print_) \
    PHYMOD_VDBG(APERTA_TBHMOD_DBG_FUNCVALIN,&pc->access,_print_)
  #define APERTA_TBHMOD_DBG_IN_FUNC_VOUT_INFO(pc,_print_) \
    PHYMOD_VDBG(APERTA_TBHMOD_DBG_FUNCVALOUT,&pc->access,_print_)
#endif

#ifdef _DV_TB_
  #define TBHMOD_DBG_IN_FUNC_INFO(pc) \
    PHYMOD_VDBG(TBHMOD_DBG_FUNC, pc, \
      ("TBHMOD IN Function : %s Port Add : %d Lane No: %d\n", \
      __func__, pc->prt_ad, pc->this_lane))
  #define TBHMOD_DBG_IN_FUNC_VIN_INFO(pc,_print_) \
    PHYMOD_VDBG(TBHMOD_DBG_FUNCVALIN,pc,_print_)
  #define TBHMOD_DBG_IN_FUNC_VOUT_INFO(pc,_print_) \
    PHYMOD_VDBG(TBHMOD_DBG_FUNCVALOUT,pc,_print_)
int plp_aperta_phymod_debug_check(uint32_t flags, tbhmod_st *pc);
#endif

#define TSCBH_GEN1           0x25 /* 0x9008 Main0_serdesID - Serdes ID Register */
#define TSCBH_GEN2           0x26 /* 0x9008 Main0_serdesID - Serdes ID Register */
#define TSCBH_FLEXE          0x28 /* 0x9008 Main0_serdesID - Serdes ID Register */
#define TSCBH_FE_GEN2        0x2f /* 0x9008 Main0_serdesID - Serdes ID Register */

/* This function will return model num for the core */
int plp_aperta_tbhmod_model_num_get(PHYMOD_ST* pc, uint32_t* model_num)
{
    MAIN0_SERDESIDr_t MAIN0_SERDESIDr_reg;

    PHYMOD_IF_ERR_RETURN(READ_MAIN0_SERDESIDr(pc, &MAIN0_SERDESIDr_reg));
    *model_num = MAIN0_SERDESIDr_MODEL_NUMBERf_GET(MAIN0_SERDESIDr_reg);

    return PHYMOD_E_NONE;
}


int plp_aperta_tbhmod_pmd_reset_seq(PHYMOD_ST* pc, int pmd_touched) /* PMD_RESET_SEQ */
{
    PMD_X1_CTLr_t reg_pmd_x1_ctrl;

    PMD_X1_CTLr_CLR(reg_pmd_x1_ctrl);

    /* first read the model number */
    /* for tscbh gen1, the fclk is hardwired to PLL1 */
    /* in Gen2, flck selection needs to be programmed the same as the tvco  PLL index */
    pc->access.lane_mask = 0x1 << 0;

    if (pmd_touched == 0) {
        PHYMOD_IF_ERR_RETURN(READ_PMD_X1_CTLr(pc,&reg_pmd_x1_ctrl));
        PMD_X1_CTLr_POR_H_RSTBf_SET(reg_pmd_x1_ctrl,1);
        PMD_X1_CTLr_CORE_DP_H_RSTBf_SET(reg_pmd_x1_ctrl,1);
        PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_CTLr(pc,reg_pmd_x1_ctrl));

        /* next change the MPP index to 1 */
        pc->access.lane_mask = 0x1 << 4;
        PMD_X1_CTLr_CLR(reg_pmd_x1_ctrl);
        PHYMOD_IF_ERR_RETURN(READ_PMD_X1_CTLr(pc,&reg_pmd_x1_ctrl));
        PMD_X1_CTLr_POR_H_RSTBf_SET(reg_pmd_x1_ctrl,1);
        PMD_X1_CTLr_CORE_DP_H_RSTBf_SET(reg_pmd_x1_ctrl,1);
        PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_CTLr(pc,reg_pmd_x1_ctrl));
        
    }
    return PHYMOD_E_NONE;
}


/*!
@brief   get  port speed id configured
@param   pc handle to current TSCBH context (#aperta_tbhmod_st)
@param   speed_id Receives the resolved speed cfg in the speed_id
@returns The value PHYMOD_E_NONE upon successful completion.
@details get  port speed configured
*/
int plp_aperta_tbhmod_speed_id_get(PHYMOD_ST* pc, int *speed_id)
{
  SC_X4_RSLVD_SPDr_t sc_final_resolved_speed;

  SC_X4_RSLVD_SPDr_CLR(sc_final_resolved_speed);
  PHYMOD_IF_ERR_RETURN(
      READ_SC_X4_RSLVD_SPDr(pc,&sc_final_resolved_speed));
  *speed_id = SC_X4_RSLVD_SPDr_SPEEDf_GET(sc_final_resolved_speed);
  return PHYMOD_E_NONE;
}



/**
@brief   Get the Port status
@param   pc handle to current TSCBH context (#aperta_tbhmod_st)
@param   disabled  Receives status on port disabledness
@returns The value PHYMOD_E_NONE upon successful completion
@details Ports can be disabled in several ways. In this function we simply write
0 to the speed change which will bring the PCS down for that lane.

*/
int plp_aperta_tbhmod_enable_get(PHYMOD_ST* pc, uint32_t* disabled)
{
  SC_X4_CTLr_t reg_sc_ctrl;

  SC_X4_CTLr_CLR(reg_sc_ctrl);

  PHYMOD_IF_ERR_RETURN(READ_SC_X4_CTLr(pc,&reg_sc_ctrl));
  *disabled = SC_X4_CTLr_SW_SPEED_CHANGEf_GET(reg_sc_ctrl);

  return PHYMOD_E_NONE;
}
/**
@brief   Get info on Disable status of the Port
@param   pc handle to current TSCBH context (#aperta_tbhmod_st)
@returns The value PHYMOD_E_NONE upon successful completion
@details Disables the port by writing 0 to the speed config logic in PCS.
This makes the PCS to bring down the PCS blocks and also apply lane datapath
reset to the PMD. There is no control input to this function since it only does
one thing.
*/
int plp_aperta_tbhmod_disable_set(PHYMOD_ST* pc)
{
  SC_X4_CTLr_t reg_sc_ctrl;
  unsigned int index;
  plp_aperta_phymod_phy_access_t temp;

  PHYMOD_MEMCPY(&temp, pc, sizeof(plp_aperta_phymod_phy_access_t));
  for (index = 0; index <8; index++) {
      if (pc->access.lane_mask & (1 << index)) {
          temp.access.lane_mask = 1 << index;
          SC_X4_CTLr_CLR(reg_sc_ctrl);
          PHYMOD_IF_ERR_RETURN(READ_SC_X4_CTLr(&temp,&reg_sc_ctrl));
          SC_X4_CTLr_SW_SPEED_CHANGEf_SET(reg_sc_ctrl, 0);
          PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(&temp,reg_sc_ctrl));
      }
  }

  return PHYMOD_E_NONE;
}

/* this function needs to be called if pcs reset is set
only forced speed will be affected by this HW issue */

int plp_aperta_tbhmod_pcs_reset_sw_war(const PHYMOD_ST* pc)
{
    SC_X4_CTLr_t reg_sc_ctrl;
    TXFIR_MISC_CTL0r_t txfir_misc_ctrl;
    RX_X4_PCS_LATCH_STS1r_t link_status_reg;
    int start_lane, num_lane, i;
    PHYMOD_ST phy_copy;

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));


    /*first check live status, if link status is down,
    nothing is needed */
    PHYMOD_IF_ERR_RETURN(READ_RX_X4_PCS_LATCH_STS1r(pc, &link_status_reg));

    if (RX_X4_PCS_LATCH_STS1r_PCS_LINK_STATUS_LIVEf_GET(link_status_reg)) {
        SC_X4_CTLr_CLR(reg_sc_ctrl);
        TXFIR_MISC_CTL0r_CLR(txfir_misc_ctrl);
        PHYMOD_IF_ERR_RETURN
                (READ_TXFIR_MISC_CTL0r(&phy_copy, &txfir_misc_ctrl));

        /* first disable all lane tx */
        TXFIR_MISC_CTL0r_SDK_TX_DISABLEf_SET(txfir_misc_ctrl, 1);
        for (i = 0; i < num_lane; i++) {
            phy_copy.access.lane_mask = 1 << (start_lane + i);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_TXFIR_MISC_CTL0r(&phy_copy, txfir_misc_ctrl));
        }
        PHYMOD_IF_ERR_RETURN(READ_SC_X4_CTLr(&phy_copy, &reg_sc_ctrl));
        /* next  toggle speed control bit */
        SC_X4_CTLr_SW_SPEED_CHANGEf_SET(reg_sc_ctrl, 1);
        phy_copy.access.lane_mask = 1 << start_lane;
        PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(&phy_copy, reg_sc_ctrl));
        /* then sleep for 5 micro-second */
        PHYMOD_USLEEP(5);

        SC_X4_CTLr_SW_SPEED_CHANGEf_SET(reg_sc_ctrl, 0);
        PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(&phy_copy, reg_sc_ctrl));
        /* then sleep for 5 micro-second */
        PHYMOD_USLEEP(5);

        /* last, disable tx_disable */
        TXFIR_MISC_CTL0r_SDK_TX_DISABLEf_SET(txfir_misc_ctrl, 0);
        for (i = 0; i < num_lane; i++) {
            phy_copy.access.lane_mask = 1 << (start_lane + i);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_TXFIR_MISC_CTL0r(&phy_copy, txfir_misc_ctrl));
        }
    }

    return PHYMOD_E_NONE;
}


int plp_aperta_tbhmod_enable_set(PHYMOD_ST* pc)
{
  SC_X4_CTLr_t reg_sc_ctrl;
  unsigned int index;
  plp_aperta_phymod_phy_access_t temp;

  PHYMOD_MEMCPY(&temp, pc, sizeof(plp_aperta_phymod_phy_access_t));

  for (index = 0; index <8; index++) {
      if (pc->access.lane_mask & (1 << index)) {
          temp.access.lane_mask = 1 << index;
          SC_X4_CTLr_CLR(reg_sc_ctrl);

          PHYMOD_IF_ERR_RETURN(READ_SC_X4_CTLr(pc,&reg_sc_ctrl));
          SC_X4_CTLr_SW_SPEED_CHANGEf_SET(reg_sc_ctrl, 1);
          PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(pc,reg_sc_ctrl));
      }
  }

  return PHYMOD_E_NONE;
}

/**
@brief   rx lane reset and enable of any particular lane
@param   pc handle to current TSCBH context (#aperta_tbhmod_st)
@param   Get the configured reset lane.
@returns The value PHYMOD_E_NONE upon successful completion.
@details
          This function read back control bit
*/
int plp_aperta_tbhmod_rx_lane_control_get(PHYMOD_ST* pc, int *enable)         /* RX_LANE_CONTROL */
{
  RX_X4_PMA_CTL0r_t    reg_pma_ctrl;

  RX_X4_PMA_CTL0r_CLR(reg_pma_ctrl);
  PHYMOD_IF_ERR_RETURN (READ_RX_X4_PMA_CTL0r(pc, &reg_pma_ctrl));
  *enable =   RX_X4_PMA_CTL0r_RSTB_LANEf_GET( reg_pma_ctrl);

  return PHYMOD_E_NONE;
}

/*!
@brief Override TX lane output.
@param  pc handle to current TSCBH context (#aperta_tbhmod_st)
@param  tx control lanes to disable
@returns PHYMOD_E_NONE if no errors. PHYMOD_ERROR else.
@details This function enables/disables, via override, transmission on a specific lane. No reset is
required to restart the transmission. Lane control is done through 'ovrr' and 'value' inputs.
<B>Set ovrr=0 & value=0 : No override of tx_disable to PMD lanes 0/1/2/3</B>
<B>Set ovrr=0 & value=1 : No override of tx_disable to PMD lanes 0/1/2/3</B>
<B>Set ovrr=1 & value=0 : Override tx_disable to inactive for PMD lanes 0/1/2/3</B>
<B>Set ovrr=1 & value=1 : Override tx_disable to active for PMD lanes 0/1/2/3</B>
*/

/* FCSN */
int plp_aperta_tbhmod_tx_lane_disable (PHYMOD_ST* pc, uint16_t ovrr, uint16_t value)

{
  PMD_X4_CTLr_t  PMD_X4_PMD_X4_CONTROLr_reg;
  PMD_X4_OVRRr_t PMD_X4_PMD_X4_OVERRIDEr_reg;

  #ifdef _DV_TB_
  PHYMOD_DEBUG_ERROR(("FCSN_Info : plp_aperta_tbhmod_tx_lane_disable : port %0d : ovrr %0d : value %0d\n", pc.this_lane, ovrr, value));
  #endif

  if(ovrr) {
    PMD_X4_CTLr_CLR(PMD_X4_PMD_X4_CONTROLr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_CTLr(pc, &PMD_X4_PMD_X4_CONTROLr_reg));
    PMD_X4_CTLr_TX_DISABLEf_SET(PMD_X4_PMD_X4_CONTROLr_reg, value);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_CTLr(pc, PMD_X4_PMD_X4_CONTROLr_reg));

    PMD_X4_OVRRr_CLR(PMD_X4_PMD_X4_OVERRIDEr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_OVRRr(pc, &PMD_X4_PMD_X4_OVERRIDEr_reg));
    PMD_X4_OVRRr_TX_DISABLE_OENf_SET(PMD_X4_PMD_X4_OVERRIDEr_reg, ovrr);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_OVRRr(pc, PMD_X4_PMD_X4_OVERRIDEr_reg));
  }
  else { /* remove the override first then set the "don't care" value - prevents glitches */
    PMD_X4_OVRRr_CLR(PMD_X4_PMD_X4_OVERRIDEr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_OVRRr(pc, &PMD_X4_PMD_X4_OVERRIDEr_reg));
    PMD_X4_OVRRr_TX_DISABLE_OENf_SET(PMD_X4_PMD_X4_OVERRIDEr_reg, ovrr);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_OVRRr(pc, PMD_X4_PMD_X4_OVERRIDEr_reg));

    PMD_X4_CTLr_CLR(PMD_X4_PMD_X4_CONTROLr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_OVRRr(pc, &PMD_X4_PMD_X4_OVERRIDEr_reg));
    PMD_X4_CTLr_TX_DISABLEf_SET(PMD_X4_PMD_X4_CONTROLr_reg, value);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_CTLr(pc, PMD_X4_PMD_X4_CONTROLr_reg));
  }
  return PHYMOD_E_NONE;
}

/* ------------------------------------------------------------------------- */

/* FCSN */
int plp_aperta_tbhmod_rx_lock_ovrd(PHYMOD_ST* pc, uint16_t ovrd)
{
  PMD_X4_OVRRr_t PMD_X4_PMD_X4_OVERRIDEr_reg;


  PMD_X4_OVRRr_CLR(PMD_X4_PMD_X4_OVERRIDEr_reg);
  PHYMOD_IF_ERR_RETURN(READ_PMD_X4_OVRRr(pc, &PMD_X4_PMD_X4_OVERRIDEr_reg));
  PMD_X4_OVRRr_RX_LOCK_OVRDf_SET(PMD_X4_PMD_X4_OVERRIDEr_reg, ovrd);
  PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_OVRRr(pc, PMD_X4_PMD_X4_OVERRIDEr_reg));


  return PHYMOD_E_NONE;
}

/* ------------------------------------------------------------------------- */

/* FCSN */
int plp_aperta_tbhmod_rx_sigdet_ovrd(PHYMOD_ST* pc, uint16_t ovrd)
{
  PMD_X4_OVRRr_t PMD_X4_PMD_X4_OVERRIDEr_reg;


  PMD_X4_OVRRr_CLR(PMD_X4_PMD_X4_OVERRIDEr_reg);
  PHYMOD_IF_ERR_RETURN(READ_PMD_X4_OVRRr(pc, &PMD_X4_PMD_X4_OVERRIDEr_reg));
  PMD_X4_OVRRr_SIGNAL_DETECT_OVRDf_SET(PMD_X4_PMD_X4_OVERRIDEr_reg, ovrd);
  PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_OVRRr(pc, PMD_X4_PMD_X4_OVERRIDEr_reg));

  return PHYMOD_E_NONE;
}




int plp_aperta_tbhmod_pcs_set_ilkn_wm(PHYMOD_ST* pc,int ilkn_wm)
{
  ILKN_CTL0r_t ILKN_CONTROL0r_reg;


  ILKN_CTL0r_CLR(ILKN_CONTROL0r_reg);
  PHYMOD_IF_ERR_RETURN(READ_ILKN_CTL0r(pc, &ILKN_CONTROL0r_reg));
  ILKN_CTL0r_SOFT_RST_RXf_SET(ILKN_CONTROL0r_reg, 1);
  ILKN_CTL0r_SOFT_RST_TXf_SET(ILKN_CONTROL0r_reg, 1);
  PHYMOD_IF_ERR_RETURN(MODIFY_ILKN_CTL0r(pc, ILKN_CONTROL0r_reg));

  ILKN_CTL0r_CLR(ILKN_CONTROL0r_reg);
  PHYMOD_IF_ERR_RETURN(READ_ILKN_CTL0r(pc, &ILKN_CONTROL0r_reg));
  ILKN_CTL0r_WMf_SET(ILKN_CONTROL0r_reg, ilkn_wm);
  PHYMOD_IF_ERR_RETURN(MODIFY_ILKN_CTL0r(pc, ILKN_CONTROL0r_reg));

  return PHYMOD_E_NONE;
}

#ifdef _DV_TB_
/*!
@brief Controls PMD reset pins irrespective of PCS is in Speed Control mode or not
aram  pc handle to current TSCBH context (#aperta_tbhmod_st)
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Set the ::aperta_tbhmod_st::per_lane_control field of #aperta_tbhmod_st to <B>0 to
disable</B> PCS  <B>1 to enable</B>.
*/

/* Ensure that tier2 and phymod control exist to access this function */
int aperta_tbhmod_pmd_lane_reset_bypass (PHYMOD_ST* pc, int pmd_reset_control)     /* PMD_LANE_RESET_BYPASS */
{
  int cntl;
  PMD_X4_OVRRr_t PMD_X4_OVERRIDEr_reg;
  PMD_X4_CTLr_t PMD_X4_PMD_X4_CONTROLr_reg;


  cntl = pmd_reset_control & 0x10;

  if (cntl) {
    PMD_X4_CTLr_CLR(PMD_X4_PMD_X4_CONTROLr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_CTLr(pc, &PMD_X4_PMD_X4_CONTROLr_reg));
    PMD_X4_CTLr_LN_DP_H_RSTBf_SET(PMD_X4_PMD_X4_CONTROLr_reg,1);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_CTLr(pc, PMD_X4_PMD_X4_CONTROLr_reg));

    PMD_X4_OVRRr_CLR(PMD_X4_OVERRIDEr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_OVRRr(pc, &PMD_X4_OVERRIDEr_reg));
    PMD_X4_OVRRr_LN_DP_H_RSTB_OENf_SET(PMD_X4_OVERRIDEr_reg, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_OVRRr(pc, PMD_X4_OVERRIDEr_reg));

  } else {
    PMD_X4_OVRRr_CLR(PMD_X4_OVERRIDEr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_OVRRr(pc, &PMD_X4_OVERRIDEr_reg));
    PMD_X4_OVRRr_LN_DP_H_RSTB_OENf_SET(PMD_X4_OVERRIDEr_reg,0);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_OVRRr(pc, PMD_X4_OVERRIDEr_reg));

    PMD_X4_CTLr_CLR(PMD_X4_PMD_X4_CONTROLr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_CTLr(pc, &PMD_X4_PMD_X4_CONTROLr_reg));
    PMD_X4_CTLr_LN_DP_H_RSTBf_SET(PMD_X4_PMD_X4_CONTROLr_reg,0);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_CTLr(pc, PMD_X4_PMD_X4_CONTROLr_reg));
  }


  return PHYMOD_E_NONE;
} /* PMD_RESET_BYPASS */



#endif


int plp_aperta_tbhmod_set_sw_mgmt_glb_mask(PHYMOD_ST* pc, int mask)
{
  AN_X1_GLB_MASKr_t AN_X1_GLB_MASKr_reg;
  AN_X1_GLB_MASKr_SET(AN_X1_GLB_MASKr_reg, mask);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_GLB_MASKr(pc, AN_X1_GLB_MASKr_reg));

  return PHYMOD_E_NONE;
}


/*!
@brief Controls the setting/resetting of autoneg advertisement registers.
@param  pc handle to current TSCBH context (#aperta_tbhmod_st)
@param  cl73_adv struct of type  #aperta_tbhmod_an_adv_ability_t
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details
TBD
*/

#ifdef _DV_TB_
int aperta_tbhmod_sw_autoneg_set(PHYMOD_ST* pc, sw_autoneg_st* an_st)               /* SW_AUTONEG_SET */
{
  AN_X4_INT_ENr_t AN_X4_INT_ENr_reg;
  AN_X4_WAIT_ACK_COMPLETEr_t AN_X4_WAIT_ACK_COMPLETEr_reg;


  AN_X4_INT_ENr_CLR(AN_X4_INT_ENr_reg);
  AN_X4_INT_ENr_AN_COMPLETED_SW_ENf_SET(AN_X4_INT_ENr_reg, an_st->sw_an_completed_en);
  AN_X4_INT_ENr_LP_PAGE_RDY_ENf_SET(AN_X4_INT_ENr_reg, an_st->lp_page_rdy_en);
  PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_INT_ENr(pc, AN_X4_INT_ENr_reg));

  AN_X4_WAIT_ACK_COMPLETEr_CLR(AN_X4_WAIT_ACK_COMPLETEr_reg);
  AN_X4_WAIT_ACK_COMPLETEr_WAIT_FOR_ACK_ENf_SET(AN_X4_WAIT_ACK_COMPLETEr_reg, an_st->sw_an_wait_for_ack);
  PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_WAIT_ACK_COMPLETEr(pc, AN_X4_WAIT_ACK_COMPLETEr_reg));

  plp_aperta_tbhmod_set_sw_mgmt_glb_mask(dv_st, 0xf);

  return PHYMOD_E_NONE;
}

#endif

/*!
 * @brief   Controls the setting/resetting of autoneg advertisement registers.
 * @param   pc handle to current TSCBH context (#PHYMOD_ST)
 * @returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
 * @details
 *   Get aneg features via an_ability_st and write the pages
 *   This does not start the autoneg. That is done in plp_aperta_tbhmod_autoneg_control
*/
int plp_aperta_tbhmod_autoneg_ability_set(PHYMOD_ST* pc,
                               const phymod_autoneg_advert_abilities_t* autoneg_abilities)
{
  AN_X4_LD_BASE_ABIL1r_t AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg;
  AN_X4_LD_BASE_ABIL2r_t AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg;
  AN_X4_LD_BASE_ABIL3r_t AN_X4_ABILITIES_LD_BASE_ABILITIES_3r_reg;
  AN_X4_LD_UP1_ABIL0r_t  AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg;
  AN_X4_LD_UP1_ABIL1r_t  AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg;
  AN_X4_CL73_CFGr_t      AN_X4_CL73_CFGr_reg;
  phymod_autoneg_advert_ability_t *an_ability;
  uint32_t i;
  uint8_t fec_is_shared, no_baser_support;


  AN_X4_LD_BASE_ABIL1r_CLR(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg);
  AN_X4_LD_BASE_ABIL2r_CLR(AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg);
  AN_X4_LD_BASE_ABIL3r_CLR(AN_X4_ABILITIES_LD_BASE_ABILITIES_3r_reg);
  AN_X4_LD_UP1_ABIL0r_CLR(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg);
  AN_X4_LD_UP1_ABIL1r_CLR(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg);
  AN_X4_CL73_CFGr_CLR(AN_X4_CL73_CFGr_reg);

  PHYMOD_IF_ERR_RETURN(READ_AN_X4_CL73_CFGr(pc, &AN_X4_CL73_CFGr_reg));
  /* Write base page bit position for 50GKR_CR, 100GKR2_CR2, 200GKR4_CR4 */
  /* Bit position need revisit after IEEE SPEC finialized */
  AN_X4_LD_BASE_ABIL2r_BASE_50G_KR_CR_SELf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg, APERTA_TBHMOD_CL73_ABILITY_50G_KR1_CR1_POSITION);
  AN_X4_LD_BASE_ABIL2r_BASE_100G_KR2_CR2_SELf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg, APERTA_TBHMOD_CL73_ABILITY_100G_KR2_CR2_POSITION);
  AN_X4_LD_BASE_ABIL3r_BASE_200G_KR4_CR4_SELf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_3r_reg, APERTA_TBHMOD_CL73_ABILITY_200G_KR4_CR4_POSITION);

#ifdef APERTA_ABILITY_WO_CLEAR
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL1r(pc, &AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg));
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_UP1_ABIL1r(pc, &AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg));
  /* Set default value 
  AN_X4_CL73_CFGr_MSA_FEC_MAPPINGf_SET(AN_X4_CL73_CFGr_reg, 0);*/
  AN_X4_LD_BASE_ABIL1r_FEC_REQf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 0x1);
  /*AN_X4_LD_UP1_ABIL1r_RS_FEC_REQf_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 0x1);
  AN_X4_LD_UP1_ABIL1r_CL74_REQf_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 0x1);*/

#else
  /* Set default value*/ 
  AN_X4_CL73_CFGr_MSA_FEC_MAPPINGf_SET(AN_X4_CL73_CFGr_reg, 0);
  AN_X4_LD_BASE_ABIL1r_FEC_REQf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 0x1);
  AN_X4_LD_UP1_ABIL1r_RS_FEC_REQf_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 0x1);
  AN_X4_LD_UP1_ABIL1r_CL74_REQf_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 0x1);
#endif

  no_baser_support = 0;

  for (i = 0; i < autoneg_abilities->num_abilities; i++) {
      an_ability = &autoneg_abilities->autoneg_abilities[i];
      fec_is_shared = 0;
      switch (an_ability->speed) {
          case 10000:
              /* CL73-10G-1lane */
              AN_X4_LD_BASE_ABIL1r_BASE_10G_KR1f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
              fec_is_shared = 1;
              break;
          case 20000:
              /* CL73BAM-20G-1lane */
              if (an_ability->medium == phymodFirmwareMediaTypeCopperCable) {
                  AN_X4_LD_UP1_ABIL1r_BAM_20G_CR1f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
              } else {
                  AN_X4_LD_UP1_ABIL1r_BAM_20G_KR1f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
              }
              fec_is_shared = 1;
              break;
          case 25000:
              if (an_ability->an_mode == phymod_AN_MODE_CL73) {
                  /* CL73 */
                  if (an_ability->channel == phymod_channel_short) {
                      AN_X4_LD_BASE_ABIL1r_BASE_25G_CRS_KRSf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
                  } else {
                      AN_X4_LD_BASE_ABIL1r_BASE_25G_CR_KRf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
                  }
              } else {
                  /* MSA and CL73BAM share the same speed ability fields */
                  if (an_ability->an_mode == phymod_AN_MODE_MSA) {
                      AN_X4_CL73_CFGr_MSA_FEC_MAPPINGf_SET(AN_X4_CL73_CFGr_reg, 1);
                  }
                  if (an_ability->medium == phymodFirmwareMediaTypeCopperCable) {
                      AN_X4_LD_UP1_ABIL1r_BAM_25G_CR1f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                  } else {
                      AN_X4_LD_UP1_ABIL1r_BAM_25G_KR1f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                  }
              }
              fec_is_shared = 1;
              break;
          case 40000:
              if (an_ability->an_mode == phymod_AN_MODE_CL73) {
                  /* CL73-40G-4lanes */
                  if (an_ability->medium == phymodFirmwareMediaTypeCopperCable) {
                      AN_X4_LD_BASE_ABIL1r_BASE_40G_CR4f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
                  } else {
                      AN_X4_LD_BASE_ABIL1r_BASE_40G_KR4f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
                  }
              } else {
                  /* CL73BAM-40G-2lanes */
                  if (an_ability->medium == phymodFirmwareMediaTypeCopperCable) {
                      AN_X4_LD_UP1_ABIL0r_BAM_40G_CR2f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg, 1);
                  } else {
                      AN_X4_LD_UP1_ABIL0r_BAM_40G_KR2f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg, 1);
                  }
              }
              fec_is_shared = 1;
              break;
          case 50000:
              if (an_ability->an_mode == phymod_AN_MODE_CL73) {
                  /* CL73-50G-1lane */
                  AN_X4_LD_BASE_ABIL2r_BASE_50G_KR_CR_ENf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg, 1);
              } else if (an_ability->an_mode == phymod_AN_MODE_CL73BAM) {
                  if (an_ability->resolved_num_lanes == 1) {
                      /* CL73BAM-50G-1lane */
                      if (an_ability->fec == phymod_fec_None) {
                          AN_X4_LD_UP1_ABIL0r_BAM_50G_BRCM_NO_FEC_CR1_KR1f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg, 1);
                      } else {
                          AN_X4_LD_UP1_ABIL0r_BAM_50G_BRCM_FEC_528_CR1_KR1f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg, 1);
                      }
                  } else {
                     /* CL73BAM-50G-2lanes */
                      if (an_ability->fec == phymod_fec_RS544) {
                          AN_X4_LD_UP1_ABIL0r_BAM_50G_BRCM_FEC_544_CR2_KR2f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg, 1);
                      } else {
                     if (an_ability->medium == phymodFirmwareMediaTypeCopperCable) {
                         AN_X4_LD_UP1_ABIL0r_BAM_50G_CR2f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg, 1);
                     } else {
                         AN_X4_LD_UP1_ABIL0r_BAM_50G_KR2f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg, 1);
                     }
                     fec_is_shared = 1;
                          no_baser_support = 1;
                      }
                  }
              } else {
                  /* MSA-50G-2lanes */
                  AN_X4_CL73_CFGr_MSA_FEC_MAPPINGf_SET(AN_X4_CL73_CFGr_reg, 1);
                  if (an_ability->medium == phymodFirmwareMediaTypeCopperCable) {
                      AN_X4_LD_UP1_ABIL0r_BAM_50G_CR2f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg, 1);
                  } else {
                      AN_X4_LD_UP1_ABIL0r_BAM_50G_KR2f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg, 1);
                  }
                  fec_is_shared = 1;
                  no_baser_support = 1;
              }
              break;
          case 100000:
              if (an_ability->an_mode == phymod_AN_MODE_CL73) {
                  if (an_ability->resolved_num_lanes == 2) {
                      /* CL73-100G-2lanes */
                      AN_X4_LD_BASE_ABIL2r_BASE_100G_KR2_CR2_ENf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg,1);
                  } else {
                      /* CL73_100G-4lanes */
                      if (an_ability->medium == phymodFirmwareMediaTypeCopperCable) {
                          AN_X4_LD_BASE_ABIL1r_BASE_100G_CR4f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
                      } else {
                          AN_X4_LD_BASE_ABIL1r_BASE_100G_KR4f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
                      }
                  }
              } else {
                  if (an_ability->resolved_num_lanes == 2) {
                      /* CL73BAM-100G-2lanes */
                      if (an_ability->fec == phymod_fec_None) {
                          AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_NO_FEC_KR2_CR2f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                      } else {
                          AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_FEC_528_KR2_CR2f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                      }
                  } else {
                      /* CL73BAM-100G-4lanes */
                      if (an_ability->fec == phymod_fec_None) {
                          AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_NO_FEC_X4f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                      } else {
                          AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_KR4_CR4f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                      }
                  }
              }
              break;
          case 200000:
              if (an_ability->an_mode == phymod_AN_MODE_CL73) {
                  /* CL73-200G-4lanes */
                  AN_X4_LD_BASE_ABIL3r_BASE_200G_KR4_CR4_ENf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_3r_reg, 1);
              } else {
                  /* CL73BAM-200G-4lanes */
                  if (an_ability->fec == phymod_fec_None) {
                      AN_X4_LD_UP1_ABIL1r_BAM_200G_BRCM_NO_FEC_KR4_CR4f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                  } else {
                      AN_X4_LD_UP1_ABIL1r_BAM_200G_BRCM_KR4_CR4f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                  }
              }
              break;
          default:
              break;
      }
      if (fec_is_shared) {
          if (an_ability->an_mode == phymod_AN_MODE_CL73) {
              /****** Base FEC Settings ********/
              if (an_ability->speed == 25000) {
                  if (an_ability->fec & (1 << phymod_fec_CL91)) {
                      AN_X4_LD_BASE_ABIL1r_BASE_25G_RS_FEC_REQf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
                  } 
                  if (an_ability->fec & (1 << phymod_fec_CL74)) {
                      AN_X4_LD_BASE_ABIL1r_BASE_25G_BASE_R_FEC_REQf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
                  }
              } else {
                  if (an_ability->fec &  (1 << phymod_fec_CL74)) {
                      AN_X4_LD_BASE_ABIL1r_FEC_REQf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 3);
                  } else {
                      AN_X4_LD_BASE_ABIL1r_FEC_REQf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
                  }
              }
          } else {
              /****** BAM FEC ******/
              if (an_ability->fec & (1 << phymod_fec_CL91)) {
                  AN_X4_LD_UP1_ABIL1r_RS_FEC_REQf_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 0x3);
              } else {
                  AN_X4_LD_UP1_ABIL1r_RS_FEC_REQf_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 0x1);
              }
              if(an_ability->fec & (1 << phymod_fec_CL74)) { 
                  AN_X4_LD_UP1_ABIL1r_CL74_REQf_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 0x3);
              } else {
                  AN_X4_LD_UP1_ABIL1r_CL74_REQf_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 0x1);
              }
          }
      }

      /******* Pause Settings ********/
      if(an_ability->pause == phymod_pause_none)
        AN_X4_LD_BASE_ABIL1r_CL73_PAUSEf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 0);
      if(an_ability->pause == phymod_pause_symm)
        AN_X4_LD_BASE_ABIL1r_CL73_PAUSEf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
      if(an_ability->pause == phymod_pause_asym)
        AN_X4_LD_BASE_ABIL1r_CL73_PAUSEf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 2);
      if(an_ability->pause == phymod_pause_asym_symm)
        AN_X4_LD_BASE_ABIL1r_CL73_PAUSEf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 3);
  }
  /* MSA is not a baser*/
  if (no_baser_support) {
      AN_X4_LD_BASE_ABIL1r_FEC_REQf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 0x0);
      AN_X4_LD_UP1_ABIL1r_CL74_REQf_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 0x0);
  }

  /***** Setting AN_X4_ABILITIES_cl73_cfg 0xC1C0 *********/
  PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_CL73_CFGr(pc, AN_X4_CL73_CFGr_reg));
  /***** Setting AN_X4_ABILITIES_ld_base_abilities_1 0xC1C4 *******/
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_BASE_ABIL1r(pc, AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg));
  /***** Setting AN_X4_ABILITIES_ld_base_abilities_1 0xC1C7 *******/
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_BASE_ABIL2r(pc, AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg ));
  /***** Setting AN_X4_ABILITIES_ld_base_abilities_1 0xC1C8 *******/
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_BASE_ABIL3r(pc, AN_X4_ABILITIES_LD_BASE_ABILITIES_3r_reg ));
  /******** Setting AN_X4_ABILITIES_ld_up1_abilities_0 0xC1C1******/
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_UP1_ABIL0r(pc, AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg));
  /******** Setting AN_X4_ABILITIES_ld_up1_abilities_0 0xC1C2******/
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_UP1_ABIL1r(pc, AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg));
  (void) fec_is_shared;
  (void) no_baser_support;
  return PHYMOD_E_NONE;
}

int _plp_aperta_an_ability_construct(phymod_autoneg_advert_ability_t* an_ability,
                          uint32_t speed,
                          int num_lanes,
                          phymod_fec_type_t fec,
                          uint32_t pause,
                          int channel,
                          plp_aperta_phymod_an_mode_type_t an_mode)
{
  an_ability->speed = speed;
  an_ability->resolved_num_lanes = num_lanes;
  an_ability->fec |= (1 << fec);
  an_ability->pause = pause;
  an_ability->channel = channel;
  an_ability->an_mode = an_mode;
  /* We get medium type from FW lane config in tier 2 */
  an_ability->medium = 0;

  return PHYMOD_E_NONE;
}

/**
 * @brief   To get local autoneg advertisement registers.
 * @param   pc handle to current TSCBH context (#PHYMOD_ST)
 * @param   an_ability_st receives autoneg info. #tefmod16_an_adv_ability_t)
 * @returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
 * @details Upper layer software calls this function to get local autoneg
 *          info. This function is currently not implemented
 */
int plp_aperta_tbhmod_autoneg_ability_get(PHYMOD_ST* pc, phymod_autoneg_advert_abilities_t* autoneg_abilities)
{
  AN_X4_LD_BASE_ABIL1r_t        AN_X4_LD_BASE_ABIL1r_reg;
  AN_X4_LD_UP1_ABIL0r_t         AN_X4_LD_UP1_ABIL0r_reg;
  AN_X4_LD_UP1_ABIL1r_t         AN_X4_LD_UP1_ABIL1r_reg;
  AN_X4_LD_BASE_ABIL2r_t        AN_X4_LD_BASE_ABIL2r_reg;
  AN_X4_LD_BASE_ABIL3r_t        AN_X4_LD_BASE_ABIL3r_reg;
  AN_X4_CL73_CFGr_t             AN_X4_CL73_CFGr_reg;
  uint32_t pause, fec_cl74, fec_cl91;
  int i = 0;


  /***** AN_X4_ABILITIES_ld_base_abilities_1 0xC1C4 *******/
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL1r(pc, &AN_X4_LD_BASE_ABIL1r_reg));
  /***** AN_X4_ABILITIES_ld_base_abilities_2 0xC1C7 ******/
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL2r(pc, &AN_X4_LD_BASE_ABIL2r_reg));
  /***** AN_X4_ABILITIES_ld_base_abilities_3 0xC1C8 ******/
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL3r(pc, &AN_X4_LD_BASE_ABIL3r_reg));
  /***** AN_X4_ABILITIES_ld_up0_abilities_1 0xC1C1 *******/
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_UP1_ABIL0r(pc, &AN_X4_LD_UP1_ABIL0r_reg));
  /***** AN_X4_ABILITIES_ld_up1_abilities_1 0xC1C2 *******/
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_UP1_ABIL1r(pc, &AN_X4_LD_UP1_ABIL1r_reg));
  /***** AN_X4_ABILITIES_cl73_cfg 0xC1C0 *******/
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_CL73_CFGr(pc, &AN_X4_CL73_CFGr_reg));

  pause = AN_X4_LD_BASE_ABIL1r_CL73_PAUSEf_GET(AN_X4_LD_BASE_ABIL1r_reg);

  /* Get CL73 base abilities */
  fec_cl74 = AN_X4_LD_BASE_ABIL1r_FEC_REQf_GET(AN_X4_LD_BASE_ABIL1r_reg);

  if (AN_X4_LD_BASE_ABIL3r_BASE_200G_KR4_CR4_ENf_GET(AN_X4_LD_BASE_ABIL3r_reg)) {
      PHYMOD_IF_ERR_RETURN(
          _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                200000, 4, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73));
      
      i++;
  }
  if (AN_X4_LD_BASE_ABIL2r_BASE_100G_KR2_CR2_ENf_GET(AN_X4_LD_BASE_ABIL2r_reg)) {
      PHYMOD_IF_ERR_RETURN(
          _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                100000, 2, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73));
      i++;
  }
  if (AN_X4_LD_BASE_ABIL1r_BASE_100G_KR4f_GET(AN_X4_LD_BASE_ABIL1r_reg)
      || AN_X4_LD_BASE_ABIL1r_BASE_100G_CR4f_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
      PHYMOD_IF_ERR_RETURN(
          _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                100000, 4, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73));
      i++;
  }
  if (AN_X4_LD_BASE_ABIL2r_BASE_50G_KR_CR_ENf_GET(AN_X4_LD_BASE_ABIL2r_reg)) {
      PHYMOD_IF_ERR_RETURN(
          _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                50000, 1, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73));
      i++;
  }
  if (AN_X4_LD_BASE_ABIL1r_BASE_40G_KR4f_GET(AN_X4_LD_BASE_ABIL1r_reg)
       || AN_X4_LD_BASE_ABIL1r_BASE_40G_CR4f_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
      if (fec_cl74 == 0x3) {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   40000, 4, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73));
      } else {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   40000, 4, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73));
      }
      i++;
  }
  if (AN_X4_LD_BASE_ABIL1r_BASE_25G_CR_KRf_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
      if (AN_X4_LD_BASE_ABIL1r_BASE_25G_RS_FEC_REQf_GET(AN_X4_LD_BASE_ABIL1r_reg)
          || AN_X4_LD_BASE_ABIL1r_BASE_25G_BASE_R_FEC_REQf_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
          if (AN_X4_LD_BASE_ABIL1r_BASE_25G_RS_FEC_REQf_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        25000, 1, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73));
          }
          if (AN_X4_LD_BASE_ABIL1r_BASE_25G_BASE_R_FEC_REQf_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        25000, 1, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73));
          }
          i++;
      } else {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    25000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73));
          i++;
      }
  }
  if (AN_X4_LD_BASE_ABIL1r_BASE_25G_CRS_KRSf_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
      /* Short channel does not support RS FEC */
      if (AN_X4_LD_BASE_ABIL1r_BASE_25G_BASE_R_FEC_REQf_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    25000, 1, phymod_fec_CL74, pause, phymod_channel_short, phymod_AN_MODE_CL73));
      } else {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    25000, 1, phymod_fec_None, pause, phymod_channel_short, phymod_AN_MODE_CL73));
      }
      i++;
  }
  if (AN_X4_LD_BASE_ABIL1r_BASE_10G_KR1f_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
      if (fec_cl74 == 0x3) {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   10000, 1, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73));
      } else {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   10000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73));
      }
      i++;
  }

  /* Get UP page abilities */
  fec_cl74 = AN_X4_LD_UP1_ABIL1r_CL74_REQf_GET(AN_X4_LD_UP1_ABIL1r_reg);
  fec_cl91 = AN_X4_LD_UP1_ABIL1r_RS_FEC_REQf_GET(AN_X4_LD_UP1_ABIL1r_reg);
  if (AN_X4_CL73_CFGr_MSA_FEC_MAPPINGf_GET(AN_X4_CL73_CFGr_reg)) {
      /* UP page is MSA page */
      if (AN_X4_LD_UP1_ABIL0r_BAM_50G_CR2f_GET(AN_X4_LD_UP1_ABIL0r_reg)
          || AN_X4_LD_UP1_ABIL0r_BAM_50G_KR2f_GET(AN_X4_LD_UP1_ABIL0r_reg)) {
          if (!(fec_cl74 == 0x3 || fec_cl91 == 0x3)) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        50000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_MSA));
              i++;
          } else {
              if (fec_cl74 == 0x3) {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            50000, 2, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                  i++;
              }
              if (fec_cl91 == 0x3) {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            50000, 2, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                  i++;
              }
          }
      }
      if (AN_X4_LD_UP1_ABIL1r_BAM_25G_CR1f_GET(AN_X4_LD_UP1_ABIL1r_reg)
          || AN_X4_LD_UP1_ABIL1r_BAM_25G_KR1f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
          if (!(fec_cl74 == 0x3 || fec_cl91 == 0x3)) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        25000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_MSA));
              i++;
          } else {
              if (fec_cl74 == 0x3) {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            25000, 1, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_MSA));
              }
              if (fec_cl91 == 0x3) {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            25000, 1, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_MSA));
              }
              i++;
          }
      }
  } else {
      /* UP page is BAM page */
      if (AN_X4_LD_UP1_ABIL1r_BAM_200G_BRCM_KR4_CR4f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    200000, 4, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
          i++;
      }
      if (AN_X4_LD_UP1_ABIL1r_BAM_200G_BRCM_NO_FEC_KR4_CR4f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
          PHYMOD_IF_ERR_RETURN(
             _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   200000, 4, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
         i++;
      }
      if (AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_FEC_528_KR2_CR2f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
          PHYMOD_IF_ERR_RETURN(
             _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   100000, 2, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
          i++;
      }
      if (AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_NO_FEC_KR2_CR2f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    100000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
          i++;
      }
      if (AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_KR4_CR4f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    100000, 4, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
          i++;
      }
      if (AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_NO_FEC_X4f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    100000, 4, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
          i++;
      }
      if (AN_X4_LD_UP1_ABIL0r_BAM_50G_BRCM_FEC_528_CR1_KR1f_GET(AN_X4_LD_UP1_ABIL0r_reg)) {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    50000, 1, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
          i++;
      }
      if (AN_X4_LD_UP1_ABIL0r_BAM_50G_BRCM_NO_FEC_CR1_KR1f_GET(AN_X4_LD_UP1_ABIL0r_reg)) {
          PHYMOD_IF_ERR_RETURN(
             _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    50000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
          i++;
      }
      if (AN_X4_LD_UP1_ABIL0r_BAM_50G_BRCM_FEC_544_CR2_KR2f_GET(AN_X4_LD_UP1_ABIL0r_reg)) {
          PHYMOD_IF_ERR_RETURN(
             _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    50000, 2, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
          i++;
      }
      if (AN_X4_LD_UP1_ABIL0r_BAM_50G_CR2f_GET(AN_X4_LD_UP1_ABIL0r_reg)
          || AN_X4_LD_UP1_ABIL0r_BAM_50G_KR2f_GET(AN_X4_LD_UP1_ABIL0r_reg)) {
          if (!(fec_cl74 == 0x3 || fec_cl91 == 0x3)) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        50000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              i++;
          } else {
              if (fec_cl74 == 0x3) {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            50000, 2, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                  i++;
              }
              if (fec_cl91 == 0x3) {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            50000, 2, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                  i++;
              }
          }
      }
      if (AN_X4_LD_UP1_ABIL0r_BAM_40G_CR2f_GET(AN_X4_LD_UP1_ABIL0r_reg)
          || AN_X4_LD_UP1_ABIL0r_BAM_40G_KR2f_GET(AN_X4_LD_UP1_ABIL0r_reg)) {
          if (fec_cl74 == 0x3) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        40000, 2, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
          } else {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        40000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
          }
          i++;
      }
      if (AN_X4_LD_UP1_ABIL1r_BAM_25G_CR1f_GET(AN_X4_LD_UP1_ABIL1r_reg)
          || AN_X4_LD_UP1_ABIL1r_BAM_25G_KR1f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
          if (!(fec_cl74 == 0x3 || fec_cl91 == 0x3)) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        25000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              i++;
          } else {
              if (fec_cl74 == 0x3) {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            25000, 1, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              }
              if (fec_cl91 == 0x3) {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            25000, 1, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              }
              i++;
          }
      }
      if (AN_X4_LD_UP1_ABIL1r_BAM_20G_CR1f_GET(AN_X4_LD_UP1_ABIL1r_reg)
          || AN_X4_LD_UP1_ABIL1r_BAM_20G_KR1f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
          if (fec_cl74 == 0x3) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        20000, 1, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
          } else {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        20000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
          }
          i++;
      }
  }

  autoneg_abilities->num_abilities = i;

  return PHYMOD_E_NONE;
}



/*!
@brief Enable/disable AN
@param pc handle to current TSCBH context ($aperta_tbhmod_st)
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_ERROR else.
@details
This function programs auto-negotiation (AN) modes for the TEF. It can
enable/disable clause37/clause73/BAM autonegotiation capabilities. Call this
function once for combo mode and once per lane in independent lane mode.
The autonegotiation mode is indicated by setting an_control as required.
*/
int plp_aperta_tbhmod_autoneg_control(PHYMOD_ST* pc, aperta_tbhmod_an_control_t *an_control)
{
  plp_aperta_phymod_access_t pa_copy;
  uint16_t num_advertised_lanes;
  int start_lane, num_of_lane;
  uint16_t cl73_enable, cl73_bam_enable;
  uint16_t cl73_next_page;
  uint16_t cl73_restart;
  uint16_t cl73_bam_code;
  uint16_t msa_overrides;
   uint32_t oui; 
  AN_X4_CL73_CFGr_t      AN_X4_CL73_CFGr_reg;
  AN_X4_LD_BASE_ABIL1r_t AN_X4_LD_BASE_ABIL1r_reg;
  AN_X4_LD_BAM_ABILr_t   AN_X4_ABILITIES_LD_BAM_ABILITIESr_reg;
  AN_X1_OUI_UPRr_t   AN_X1_OUI_UPRr_reg;
  AN_X1_OUI_LWRr_t   AN_X1_OUI_LWRr_reg;

  PHYMOD_IF_ERR_RETURN
      (plp_aperta_phymod_util_lane_config_get(&pc->access, &start_lane, &num_of_lane));
  PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));

  num_advertised_lanes          = an_control->num_lane_adv;
  cl73_bam_code                 = 0x0;
  cl73_bam_enable               = 0x0;
  cl73_enable                   = 0x0;
  cl73_next_page                = 0x0;
  cl73_restart                  = 0x0;
  msa_overrides                 = 0x0;
  oui                           = BRCM_OUI;

  switch (an_control->an_type) {
    case APERTA_TBHMOD_AN_MODE_CL73:
      cl73_restart                = an_control->enable;
      cl73_enable                 = an_control->enable;
      break;
    case APERTA_TBHMOD_AN_MODE_CL73_BAM:
      cl73_restart                = an_control->enable;
      cl73_enable                 = an_control->enable;
      cl73_bam_enable             = an_control->enable;
      cl73_bam_code               = 0x3;
      cl73_next_page              = 0x1;
      break;
    case APERTA_TBHMOD_AN_MODE_CL73_MSA:
      cl73_restart                = an_control->enable;
      cl73_enable                 = an_control->enable;
      cl73_bam_enable             = an_control->enable;
      cl73_bam_code               = 0x3;
      cl73_next_page              = 0x1;
      msa_overrides               = 0x1;
       oui                         = MSA_OUI; 
      break;
    default:
      return PHYMOD_E_FAIL;
      break;
  }
  /*need to set cl73 BAM next page 0xc1c4 probably*/
  AN_X4_LD_BASE_ABIL1r_CLR(AN_X4_LD_BASE_ABIL1r_reg);
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL1r(pc, &AN_X4_LD_BASE_ABIL1r_reg));
  AN_X4_LD_BASE_ABIL1r_NEXT_PAGEf_SET(AN_X4_LD_BASE_ABIL1r_reg, cl73_next_page & 1);
  PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LD_BASE_ABIL1r(pc, AN_X4_LD_BASE_ABIL1r_reg));

  /* Writing bam_code 0xc1c5*/
  AN_X4_LD_BAM_ABILr_CLR(AN_X4_ABILITIES_LD_BAM_ABILITIESr_reg);
  AN_X4_LD_BAM_ABILr_CL73_BAM_CODEf_SET(AN_X4_ABILITIES_LD_BAM_ABILITIESr_reg, cl73_bam_code);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_BAM_ABILr(pc, AN_X4_ABILITIES_LD_BAM_ABILITIESr_reg));
 
  /* Set OUI */
    if (an_control->an_type != APERTA_TBHMOD_AN_MODE_CL73) {
        /* OUI is per MPP */
        if (start_lane < 4) {
            pa_copy.lane_mask = 0x1;
        } else {
            pa_copy.lane_mask = 0x10;
        }
        AN_X1_OUI_UPRr_CLR(AN_X1_OUI_UPRr_reg);
        AN_X1_OUI_UPRr_SET(AN_X1_OUI_UPRr_reg, (oui >> 16) & 0xff);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_OUI_UPRr(pc, AN_X1_OUI_UPRr_reg));

        AN_X1_OUI_LWRr_CLR(AN_X1_OUI_LWRr_reg);
        AN_X1_OUI_LWRr_SET(AN_X1_OUI_LWRr_reg, oui & 0xffff);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_OUI_LWRr(pc, AN_X1_OUI_LWRr_reg));
    }

    

  /* Clear AN enable bit in 0xc1c0 */
  AN_X4_CL73_CFGr_CLR(AN_X4_CL73_CFGr_reg);
  PHYMOD_IF_ERR_RETURN (READ_AN_X4_CL73_CFGr(pc, &AN_X4_CL73_CFGr_reg));
  AN_X4_CL73_CFGr_CL73_ENABLEf_SET(AN_X4_CL73_CFGr_reg,0);
  AN_X4_CL73_CFGr_CL73_AN_RESTARTf_SET(AN_X4_CL73_CFGr_reg,0);
  PHYMOD_IF_ERR_RETURN (MODIFY_AN_X4_CL73_CFGr(pc, AN_X4_CL73_CFGr_reg));
  
  /*Setting X4 abilities*/
  AN_X4_CL73_CFGr_CLR(AN_X4_CL73_CFGr_reg);
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_CL73_CFGr(pc, &AN_X4_CL73_CFGr_reg));
  AN_X4_CL73_CFGr_CL73_BAM_ENABLEf_SET(AN_X4_CL73_CFGr_reg,cl73_bam_enable);
  AN_X4_CL73_CFGr_CL73_ENABLEf_SET(AN_X4_CL73_CFGr_reg,cl73_enable);
  AN_X4_CL73_CFGr_CL73_AN_RESTARTf_SET(AN_X4_CL73_CFGr_reg,cl73_restart);
  AN_X4_CL73_CFGr_NUM_ADVERTISED_LANESf_SET(AN_X4_CL73_CFGr_reg,num_advertised_lanes);
  AN_X4_CL73_CFGr_MSA_FEC_MAPPINGf_SET(AN_X4_CL73_CFGr_reg, msa_overrides);
  PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_CL73_CFGr(pc, AN_X4_CL73_CFGr_reg));

  /* if an is enabled, the restart bit needs to be cleared */
 if (an_control->enable) {
      AN_X4_CL73_CFGr_CLR(AN_X4_CL73_CFGr_reg);
      PHYMOD_IF_ERR_RETURN (READ_AN_X4_CL73_CFGr(pc, &AN_X4_CL73_CFGr_reg));
      AN_X4_CL73_CFGr_CL73_AN_RESTARTf_SET(AN_X4_CL73_CFGr_reg,0);
      PHYMOD_IF_ERR_RETURN (MODIFY_AN_X4_CL73_CFGr(pc, AN_X4_CL73_CFGr_reg));
 }

  return PHYMOD_E_NONE;
}

/*
  @brief   To get autoneg control registers.
  @param   pc handle to current APERTA_TBHMOD context (#PHYMOD_ST)
  @param   an_control reference to struct with autoneg parms #tefmod16_an_control_t
  @param   an_complete status of AN completion
  @returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
  @details Upper layer software calls this function to get autoneg
           info.

             <B>How to call:</B><br>
             <p>Call directly </p>
 */
int plp_aperta_tbhmod_autoneg_control_get(PHYMOD_ST* pc, aperta_tbhmod_an_control_t *an_control, int *an_complete)
{
  AN_X4_CL73_CFGr_t      AN_X4_CL73_CFGr_reg;
  AN_X4_AN_MISC_STSr_t   AN_X4_AN_MISC_STSr_reg;
  AN_X4_LD_BASE_ABIL1r_t AN_X4_LD_BASE_ABIL1r_reg;
  AN_X4_LD_BASE_ABIL2r_t AN_X4_LD_BASE_ABIL2r_reg;
  AN_X4_LD_BASE_ABIL3r_t AN_X4_LD_BASE_ABIL3r_reg;
  uint32_t   base_ability1, base_ability2, base_ability3;

  APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc);

  /* CL73 AN CONFIG 0xC1C0 */
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_CL73_CFGr(pc, &AN_X4_CL73_CFGr_reg));
  if (AN_X4_CL73_CFGr_CL73_BAM_ENABLEf_GET(AN_X4_CL73_CFGr_reg) == 1) {
    if (AN_X4_CL73_CFGr_MSA_FEC_MAPPINGf_GET(AN_X4_CL73_CFGr_reg) == 1) {
        PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL1r(pc, &AN_X4_LD_BASE_ABIL1r_reg));
        PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL2r(pc, &AN_X4_LD_BASE_ABIL2r_reg));
        PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL3r(pc, &AN_X4_LD_BASE_ABIL3r_reg));
        base_ability1 =  AN_X4_LD_BASE_ABIL1r_GET(AN_X4_LD_BASE_ABIL1r_reg) & 0xc01f;
        base_ability2 = AN_X4_LD_BASE_ABIL2r_GET(AN_X4_LD_BASE_ABIL2r_reg) & 0x41;
        base_ability3 = AN_X4_LD_BASE_ABIL3r_GET(AN_X4_LD_BASE_ABIL3r_reg) & 0x1;

        if (!(base_ability1) && !(base_ability2) && !(base_ability3)) {
            an_control->an_type = APERTA_TBHMOD_AN_MODE_MSA;
            an_control->enable = 1;
        } else {
            an_control->an_type = APERTA_TBHMOD_AN_MODE_CL73_MSA;
            an_control->enable = 1;
        }
    } else {
        an_control->an_type = APERTA_TBHMOD_AN_MODE_CL73_BAM;
        an_control->enable = 1;
    }
  } else if (AN_X4_CL73_CFGr_CL73_ENABLEf_GET(AN_X4_CL73_CFGr_reg) == 1) {
     an_control->an_type = APERTA_TBHMOD_AN_MODE_CL73;
     an_control->enable = 1;
  } else {
     an_control->an_type = APERTA_TBHMOD_AN_MODE_NONE;
     an_control->enable = 0;
  }

  an_control->num_lane_adv = AN_X4_CL73_CFGr_NUM_ADVERTISED_LANESf_GET(AN_X4_CL73_CFGr_reg);

  /* an_complete status 0xC1E9 */
  AN_X4_AN_MISC_STSr_CLR(AN_X4_AN_MISC_STSr_reg);
  PHYMOD_IF_ERR_RETURN (READ_AN_X4_AN_MISC_STSr(pc, &AN_X4_AN_MISC_STSr_reg));
  *an_complete = AN_X4_AN_MISC_STSr_AN_COMPLETEf_GET(AN_X4_AN_MISC_STSr_reg);

  return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_autoneg_status_get(PHYMOD_ST* pc, int *an_en, int *an_done)
{
  AN_X4_CL73_CFGr_t  AN_X4_CL73_CFGr_reg;
  AN_X4_AN_MISC_STSr_t  AN_X4_AN_MISC_STSr_reg;

  AN_X4_CL73_CFGr_CLR(AN_X4_CL73_CFGr_reg);
  AN_X4_AN_MISC_STSr_CLR(AN_X4_AN_MISC_STSr_reg);

  PHYMOD_IF_ERR_RETURN(READ_AN_X4_CL73_CFGr(pc, &AN_X4_CL73_CFGr_reg));
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_AN_MISC_STSr(pc, &AN_X4_AN_MISC_STSr_reg));

  *an_en = AN_X4_CL73_CFGr_CL73_ENABLEf_GET(AN_X4_CL73_CFGr_reg);
  *an_done = AN_X4_AN_MISC_STSr_AN_COMPLETEf_GET(AN_X4_AN_MISC_STSr_reg);

  return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_autoneg_fec_status_get(PHYMOD_ST* pc, uint8_t *fec_status)
{
  AN_X4_AN_ABIL_RESOLUTION_STSr_t an_fec_status_reg;

  AN_X4_AN_ABIL_RESOLUTION_STSr_CLR(an_fec_status_reg);

  PHYMOD_IF_ERR_RETURN(READ_AN_X4_AN_ABIL_RESOLUTION_STSr(pc, &an_fec_status_reg));
  *fec_status = AN_X4_AN_ABIL_RESOLUTION_STSr_AN_HCD_FEC_ARCHf_GET(an_fec_status_reg);

  return PHYMOD_E_NONE;
}
int plp_aperta_tbhmod_autoneg_remote_ability_get(PHYMOD_ST* pc, phymod_autoneg_advert_abilities_t *autoneg_abilities)
{
  AN_X4_LP_BASE1r_t   AN_X4_LP_BASE1r_reg;
  AN_X4_LP_BASE2r_t   AN_X4_LP_BASE2r_reg;
  AN_X4_LP_BASE3r_t   AN_X4_LP_BASE3r_reg;
  AN_X4_LP_MP5_LWRr_t  AN_X4_LP_MP5_LWRr_reg;
  AN_X4_LP_MP5_MIDDLEr_t AN_X4_LP_MP5_MIDDLEr_reg;
  AN_X4_LP_MP5_UPRr_t AN_X4_LP_MP5_UPRr_reg;
  AN_X4_LP_UP_LWRr_t AN_X4_LP_UP_LWRr_reg;
  AN_X4_LP_UP_MIDDLEr_t AN_X4_LP_UP_MIDDLEr_reg;
  AN_X4_LP_UP_UPRr_t AN_X4_LP_UP_UPRr_reg;
  uint32_t base_0, base_1, base_2;
  uint32_t mp5_1, mp5_2;
  uint32_t up1_0, up1_1, up1_2;
  uint32_t msa_code_13_23 = 0, msa_code_2_12 = 0, msa_code_0_1 = 0;
  uint32_t bam_code = 0, i = 0;
  uint32_t pause, fec_cl74, fec_cl91;

  AN_X4_LP_BASE1r_CLR(AN_X4_LP_BASE1r_reg);
  AN_X4_LP_BASE2r_CLR(AN_X4_LP_BASE2r_reg);
  AN_X4_LP_BASE3r_CLR(AN_X4_LP_BASE3r_reg);
  AN_X4_LP_MP5_LWRr_CLR(AN_X4_LP_MP5_LWRr_reg);
  AN_X4_LP_MP5_MIDDLEr_CLR(AN_X4_LP_MP5_MIDDLEr_reg);
  AN_X4_LP_MP5_UPRr_CLR(AN_X4_LP_MP5_UPRr_reg);
  AN_X4_LP_UP_LWRr_CLR(AN_X4_LP_UP_LWRr_reg);
  AN_X4_LP_UP_MIDDLEr_CLR(AN_X4_LP_UP_MIDDLEr_reg);
  AN_X4_LP_UP_UPRr_CLR(AN_X4_LP_UP_UPRr_reg);

  /**** 0xC1D3 ****/
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_BASE1r(pc,  &AN_X4_LP_BASE1r_reg));
  /**** 0xC1D4 ****/
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_BASE2r(pc,  &AN_X4_LP_BASE2r_reg));
  /**** 0xC1D5 ****/
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_BASE3r(pc,  &AN_X4_LP_BASE3r_reg));
  /**** 0xC1D6 ****/
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_MP5_LWRr(pc, &AN_X4_LP_MP5_LWRr_reg));
  /**** 0xC1D7 ****/
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_MP5_MIDDLEr(pc, &AN_X4_LP_MP5_MIDDLEr_reg));
  /**** 0xC1D8 ****/
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_MP5_UPRr(pc, &AN_X4_LP_MP5_UPRr_reg));
  /**** 0xC1D9 ****/
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_UP_LWRr(pc, &AN_X4_LP_UP_LWRr_reg));
  /**** 0xC1DA ****/
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_UP_MIDDLEr(pc, &AN_X4_LP_UP_MIDDLEr_reg));
  /**** 0xC1DB ****/
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_UP_UPRr(pc, &AN_X4_LP_UP_UPRr_reg));

  base_0 = AN_X4_LP_BASE1r_GET(AN_X4_LP_BASE1r_reg);
  base_1 = AN_X4_LP_BASE2r_GET(AN_X4_LP_BASE2r_reg);
  base_2 = AN_X4_LP_BASE3r_GET(AN_X4_LP_BASE3r_reg);
  mp5_1  = AN_X4_LP_MP5_MIDDLEr_GET(AN_X4_LP_MP5_MIDDLEr_reg);
  mp5_2  = AN_X4_LP_MP5_UPRr_GET(AN_X4_LP_MP5_UPRr_reg);
  up1_0  = AN_X4_LP_UP_LWRr_GET(AN_X4_LP_UP_LWRr_reg);
  up1_1  = AN_X4_LP_UP_MIDDLEr_GET(AN_X4_LP_UP_MIDDLEr_reg);
  up1_2  = AN_X4_LP_UP_UPRr_GET(AN_X4_LP_UP_UPRr_reg);

  /* Get Pause Ability */
  pause = (base_0 >> AN_BASE0_PAGE_PAUSE_OFFSET) & AN_BASE0_PAGE_PAUSE_MASK;
  fec_cl74 = (base_2 >> AN_BASE2_CL74_ABILITY_REQ_SUP_OFFSET) & AN_BASE2_CL74_ABILITY_REQ_SUP_MASK;

  /* Get Base Abilities */
  if ((base_2 >> AN_BASE2_TECH_ABILITY_200G_KR4_CR4_OFFSET) & AN_BASE2_TECH_ABILITY_200G_KR4_CR4_MASK) {
      PHYMOD_IF_ERR_RETURN(
          _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                200000, 4, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_CL73));
     i++;
  }
  if ((base_2 >> AN_BASE2_TECH_ABILITY_100G_KR2_CR2_OFFSET) & AN_BASE2_TECH_ABILITY_100G_KR2_CR2_MASK) {
      PHYMOD_IF_ERR_RETURN(
          _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                100000, 2, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73));
      i++;
  }
  if (((base_1 >> AN_BASE1_TECH_ABILITY_100G_KR4_OFFSET) & AN_BASE1_TECH_ABILITY_100G_KR4_MASK)
      || ((base_1 >> AN_BASE1_TECH_ABILITY_100G_CR4_OFFSET) & AN_BASE1_TECH_ABILITY_100G_CR4_MASK)) {
      PHYMOD_IF_ERR_RETURN(
          _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                100000, 4, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73));
      if ((base_1 >> AN_BASE1_TECH_ABILITY_100G_CR4_OFFSET) & AN_BASE1_TECH_ABILITY_100G_CR4_MASK) {
          autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
      }
      i++;
  }
  if ((base_2 >> AN_BASE2_TECH_ABILITY_50G_KR1_CR1_OFFSET) & AN_BASE2_TECH_ABILITY_50G_KR1_CR1_MASK) {
      PHYMOD_IF_ERR_RETURN(
          _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                50000, 1, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73));
      i++;
  }
  if (((base_1 >> AN_BASE1_TECH_ABILITY_40G_KR4_OFFSET) & AN_BASE1_TECH_ABILITY_40G_KR4_MASK)
      || ((base_1 >> AN_BASE1_TECH_ABILITY_40G_CR4_OFFSET) & AN_BASE1_TECH_ABILITY_40G_CR4_MASK)) {
      if (fec_cl74 == 0x3) {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   40000, 4, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73));
      } else {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   40000, 4, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73));
      }
      if ((base_1 >> AN_BASE1_TECH_ABILITY_40G_CR4_OFFSET) & AN_BASE1_TECH_ABILITY_40G_CR4_MASK) {
          autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
      }
      i++;
  }
  fec_cl74 = (base_2 >> AN_BASE2_25G_BASE_R_FEC_ABILITY_REQ_OFFSET) & AN_BASE2_25G_BASE_R_FEC_ABILITY_REQ_MASK;
  fec_cl91 = (base_2 >> AN_BASE2_25G_RS_FEC_ABILITY_REQ_OFFSET) & AN_BASE2_25G_RS_FEC_ABILITY_REQ_MASK;
  if ((base_1 >> AN_BASE1_TECH_ABILITY_25G_KR1_CR1_OFFSET) & AN_BASE1_TECH_ABILITY_25G_KR1_CR1_MASK) {
      if (fec_cl74 || fec_cl91) {
          if (fec_cl74) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        25000, 1, phymod_fec_CL74, pause, 1, phymod_AN_MODE_CL73));
          }
          if (fec_cl91) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        25000, 1, phymod_fec_CL91, pause, 1, phymod_AN_MODE_CL73));
          }
          i++;
      } else {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    25000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73));
          i++;
      }
  }
  if ((base_1 >> AN_BASE1_TECH_ABILITY_25G_KRS1_CRS1_OFFSET) & AN_BASE1_TECH_ABILITY_25G_KRS1_CRS1_MASK) {
      if (fec_cl74) {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    25000, 1, phymod_fec_CL74, pause, 1, phymod_AN_MODE_CL73));
      } else {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    25000, 1, phymod_fec_None, pause, 1, phymod_AN_MODE_CL73));
      }
      i++;
  }
  fec_cl74 = (base_2 >> AN_BASE2_CL74_ABILITY_REQ_SUP_OFFSET) & AN_BASE2_CL74_ABILITY_REQ_SUP_MASK;
  if ((base_1 >> AN_BASE1_TECH_ABILITY_10G_KR1_OFFSET) & AN_BASE1_TECH_ABILITY_10G_KR1_MASK) {
      if (fec_cl74 == 0x3) {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   10000, 1, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73));
      } else {
          PHYMOD_IF_ERR_RETURN(
              _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   10000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73));
      }
      i++;
  }

  /* Check if Next_Page bit is set in base page, if yes, need to check MSA/BAM abilities */
  if ((base_0 >> AN_BASE0_PAGE_NP_OFFSET) & AN_BASE0_PAGE_NP_MASK) {
      msa_code_13_23 = (mp5_1 >> AN_MSG_PAGE1_OUI_13to23_OFFSET) & AN_MSG_PAGE1_OUI_13to23_MASK;
      msa_code_2_12 = (mp5_2 >> AN_MSG_PAGE2_OUI_2to12_OFFSET) & AN_MSG_PAGE2_OUI_2to12_MASK;
      msa_code_0_1 = (up1_0 >> AN_UF_PAGE0_OUI_OFFSET) & AN_UF_PAGE0_OUI_MASK;
      bam_code = (up1_0 >> AN_UF_PAGE0_UD_0to8_OFFSET) & AN_UF_PAGE0_UD_0to8_MASK;

      if ((msa_code_13_23 == MSA_OUI_13to23) && (msa_code_2_12 == MSA_OUI_2to12) && (msa_code_0_1 == MSA_OUI_0to1)) {
          /* UF page is MSA page */
          fec_cl74 = (up1_2 >> AN_UF_PAGE2_MSA_CL74_REQ_OFFSET) & AN_UF_PAGE2_MSA_CL74_REQ_MASK;
          fec_cl91 = (up1_2 >> AN_UF_PAGE2_MSA_CL91_REQ_OFFSET) & AN_UF_PAGE2_MSA_CL91_REQ_MASK;
          if (((up1_1 >> AN_UF_PAGE1_MSA_50G_CR2_ABILITY_OFFSET) & AN_UF_PAGE1_MSA_50G_CR2_ABILITY_MASK)
              || ((up1_1 >> AN_UF_PAGE1_MSA_50G_KR2_ABILITY_OFFSET) & AN_UF_PAGE1_MSA_50G_KR2_ABILITY_MASK)) {
              if (fec_cl74 || fec_cl91) {
                  if(fec_cl74) {
                      PHYMOD_IF_ERR_RETURN(
                           _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                 50000, 2, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                      if ((up1_1 >> AN_UF_PAGE1_MSA_50G_CR2_ABILITY_OFFSET) & AN_UF_PAGE1_MSA_50G_CR2_ABILITY_MASK) {
                          autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                      }
                      i++;
                  }
                  if (fec_cl91) {
                      PHYMOD_IF_ERR_RETURN(
                           _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                 50000, 2, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                      if ((up1_1 >> AN_UF_PAGE1_MSA_50G_CR2_ABILITY_OFFSET) & AN_UF_PAGE1_MSA_50G_CR2_ABILITY_MASK) {
                          autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                      }
                      i++;
                  }
              } else {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            50000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                  if ((up1_1 >> AN_UF_PAGE1_MSA_50G_CR2_ABILITY_OFFSET) & AN_UF_PAGE1_MSA_50G_CR2_ABILITY_MASK) {
                      autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                  }
                  i++;
              }
          }
          if (((up1_1 >> AN_UF_PAGE1_MSA_25G_KR1_ABILITY_OFFSET) & AN_UF_PAGE1_MSA_25G_KR1_ABILITY_MASK)
              || ((up1_1 >> AN_UF_PAGE1_MSA_25G_CR1_ABILITY_OFFSET) & AN_UF_PAGE1_MSA_25G_CR1_ABILITY_MASK)) {
              if (fec_cl74 || fec_cl91) {
                  if(fec_cl74) {
                      PHYMOD_IF_ERR_RETURN(
                           _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                 25000, 1, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                      if ((up1_1 >> AN_UF_PAGE1_MSA_25G_CR1_ABILITY_OFFSET) & AN_UF_PAGE1_MSA_25G_CR1_ABILITY_MASK) {
                          autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                      }
                  }
                  if (fec_cl91) {
                      PHYMOD_IF_ERR_RETURN(
                           _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                 25000, 1, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                      if ((up1_1 >> AN_UF_PAGE1_MSA_25G_CR1_ABILITY_OFFSET) & AN_UF_PAGE1_MSA_25G_CR1_ABILITY_MASK) {
                          autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                      }
                  }
                  i++;
              } else {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            25000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                  if ((up1_1 >> AN_UF_PAGE1_MSA_25G_CR1_ABILITY_OFFSET) & AN_UF_PAGE1_MSA_25G_CR1_ABILITY_MASK) {
                      autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                  }
                  i++;
              }
          }
      } else if (bam_code == BRCM_BAM_CODE) {
          /* UF page is BAM page */
          fec_cl74 = (up1_2 >> AN_UF_PAGE2_BAM_CL74_REQ_OFFSET) & AN_UF_PAGE2_BAM_CL74_REQ_MASK;
          fec_cl91 = (up1_2 >> AN_UF_PAGE2_BAM_CL91_REQ_OFFSET) & AN_UF_PAGE2_BAM_CL91_REQ_MASK;
          if ((up1_2 >> AN_UF_PAGE2_BAM_200G_BRCM_KR4_CR4_OFFSET) & AN_UF_PAGE2_BAM_200G_BRCM_KR4_CR4_MASK) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        200000, 4, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              i++;
          }
          if ((up1_2 >> AN_UF_PAGE2_BAM_200G_BRCM_NO_FEC_KR4_CR4_OFFSET) & AN_UF_PAGE2_BAM_200G_BRCM_NO_FEC_KR4_CR4_MASK) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        200000, 4, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              i++;
          }
          if ((up1_2 >> AN_UF_PAGE2_BAM_100G_BRCM_FEC_528_KR2_CR2_OFFSET) & AN_UF_PAGE2_BAM_100G_BRCM_FEC_528_KR2_CR2_MASK) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        100000, 2, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              i++;
          }
          if ((up1_2 >> AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_KR2_CR2_OFFSET) & AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_KR2_CR2_MASK) {
              PHYMOD_IF_ERR_RETURN(
                   _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                         100000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              i++;
          }
          if ((up1_2 >> AN_UF_PAGE2_BAM_100G_BRCM_KR4_CR4_OFFSET) & AN_UF_PAGE2_BAM_100G_BRCM_KR4_CR4_MASK) {
              PHYMOD_IF_ERR_RETURN(
                   _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                         100000, 4, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              i++;
          }
          if ((up1_2 >> AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_X4_OFFSET) & AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_X4_MASK) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        100000, 4, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              i++;
          }
          if ((up1_1 >> AN_UF_PAGE1_BAM_50G_BRCM_FEC_528_CR1_KR1_OFFSET) & AN_UF_PAGE1_BAM_50G_BRCM_FEC_528_CR1_KR1_MASK) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        50000, 1, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              i++;
          }
          if ((up1_1 >> AN_UF_PAGE1_BAM_50G_BRCM_NO_FEC_CR1_KR1_OFFSET) & AN_UF_PAGE1_BAM_50G_BRCM_NO_FEC_CR1_KR1_MASK) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        50000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              i++;
          }
          if ((up1_1 >> AN_UF_PAGE1_BAM_50G_BRCM_FEC_544_CR2_KR2_OFFSET) & AN_UF_PAGE1_BAM_50G_BRCM_FEC_544_CR2_KR2_MASK) {
              PHYMOD_IF_ERR_RETURN(
                  _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        50000, 2, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              i++;
          }
          if (((up1_1 >> AN_UF_PAGE1_BAM_50G_KR2_OFFSET) & AN_UF_PAGE1_BAM_50G_KR2_MASK)
              || ((up1_1 >> AN_UF_PAGE1_BAM_50G_CR2_OFFSET) & AN_UF_PAGE1_BAM_50G_CR2_MASK)) {
              if (fec_cl74 || fec_cl91) {
                  if (fec_cl74) {
                      PHYMOD_IF_ERR_RETURN(
                          _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                50000, 2, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                      if ((up1_1 >> AN_UF_PAGE1_BAM_50G_CR2_OFFSET) & AN_UF_PAGE1_BAM_50G_CR2_MASK) {
                          autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                      }
                      i++;
                  }
                  if (fec_cl91) {
                      PHYMOD_IF_ERR_RETURN(
                          _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                50000, 2, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                      if ((up1_1 >> AN_UF_PAGE1_BAM_50G_CR2_OFFSET) & AN_UF_PAGE1_BAM_50G_CR2_MASK) {
                          autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                      }
                      i++;
                  }
              } else {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            50000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                  if ((up1_1 >> AN_UF_PAGE1_BAM_50G_CR2_OFFSET) & AN_UF_PAGE1_BAM_50G_CR2_MASK) {
                      autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                  }
                  i++;
              }
          }
          if (((up1_1 >> AN_UF_PAGE1_BAM_40G_KR2_OFFSET) & AN_UF_PAGE1_BAM_40G_KR2_MASK)
              || ((up1_1 >> AN_UF_PAGE1_BAM_40G_CR2_OFFSET) & AN_UF_PAGE1_BAM_40G_CR2_MASK)) {
              if (fec_cl74) {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            40000, 2, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              } else {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            40000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              }
              if ((up1_1 >> AN_UF_PAGE1_BAM_40G_CR2_OFFSET) & AN_UF_PAGE1_BAM_40G_CR2_MASK) {
                  autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
              }
              i++;
          }
          if (((up1_1 >> AN_UF_PAGE1_BAM_25G_KR1_OFFSET) & AN_UF_PAGE1_BAM_25G_KR1_MASK)
              ||((up1_1 >> AN_UF_PAGE1_BAM_25G_CR1_OFFSET) & AN_UF_PAGE1_BAM_25G_CR1_MASK)) {
              if (fec_cl74 || fec_cl91) {
                  if (fec_cl74) {
                      PHYMOD_IF_ERR_RETURN(
                          _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                25000, 1, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                      if ((up1_1 >> AN_UF_PAGE1_BAM_25G_CR1_OFFSET) & AN_UF_PAGE1_BAM_25G_CR1_MASK) {
                          autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                      }
                  }
                  if (fec_cl91) {
                      PHYMOD_IF_ERR_RETURN(
                          _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                25000, 1, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                      if ((up1_1 >> AN_UF_PAGE1_BAM_25G_CR1_OFFSET) & AN_UF_PAGE1_BAM_25G_CR1_MASK) {
                          autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                      }
                  }
                  i++;
              } else {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            25000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                  if ((up1_1 >> AN_UF_PAGE1_BAM_25G_CR1_OFFSET) & AN_UF_PAGE1_BAM_25G_CR1_MASK) {
                      autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                  }
                  i++;
              }
          }
          if (((up1_1 >> AN_UF_PAGE1_BAM_20G_KR1_OFFSET) & AN_UF_PAGE1_BAM_20G_KR1_MASK)
              ||((up1_1 >> AN_UF_PAGE1_BAM_20G_CR1_OFFSET) & AN_UF_PAGE1_BAM_20G_CR1_MASK)) {
              if (fec_cl74) {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            20000, 1, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              } else {
                  PHYMOD_IF_ERR_RETURN(
                      _plp_aperta_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            20000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
              }
              if ((up1_1 >> AN_UF_PAGE1_BAM_20G_CR1_OFFSET) & AN_UF_PAGE1_BAM_20G_CR1_MASK) {
                  autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
              }
              i++;
          }
      }
  }

  autoneg_abilities->num_abilities = i;

  return PHYMOD_E_NONE;
}


#ifdef DV_AN_SUPPORT

/*!
@brief To get autoneg advertisement registers.
@param  pc handle to current TSCBH context (#aperta_tbhmod_st)
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details
*/
int aperta_tbhmod_autoneg_get(aperta_tbhmod_st* pc)               /* AUTONEG_GET */
{
  APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc);
  /* TBD */
  return PHYMOD_E_NONE;
}

/*!
@brief Programs the SW AN local device pages
@param  pc handle to current TSCBH context (#aperta_tbhmod_st)
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.

@details
This function programs SW AN local device pages.
per_lane_control bits [17:16] indicate
the page its trying to write, per_lane_control bits [15:0]
has the actual page contents
*/
int aperta_tbhmod_sw_an_ld_page_load(PHYMOD_ST* pc, int is_base_page, int page_descriptor)
{
  uint16_t page_num, page;
  AN_X4_LD_PAGE0r_t  AN_X4_LD_PAGE0r_reg;
  AN_X4_LD_PAGE1r_t  AN_X4_LD_PAGE1r_reg;
  AN_X4_LD_PAGE2r_t  AN_X4_LD_PAGE2r_reg;

  AN_X4_SW_AN_BASE_PAGE0r_t AN_X4_SW_AN_BASE_PAGE0r_reg;
  AN_X4_SW_AN_BASE_PAGE1r_t AN_X4_SW_AN_BASE_PAGE1r_reg;
  AN_X4_SW_AN_BASE_PAGE2r_t AN_X4_SW_AN_BASE_PAGE2r_reg;


  page_num = (page_descriptor>>16) & 0x3;
  page     = page_descriptor & 0xffff;

  if(is_base_page) {
    if(page_num == 0) {
      AN_X4_SW_AN_BASE_PAGE0r_CLR(AN_X4_SW_AN_BASE_PAGE0r_reg);
      AN_X4_SW_AN_BASE_PAGE0r_SET(AN_X4_SW_AN_BASE_PAGE0r_reg, page);
      PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_SW_AN_BASE_PAGE0r(pc, AN_X4_SW_AN_BASE_PAGE0r_reg));
    }
    else if(page_num == 1) {
      AN_X4_SW_AN_BASE_PAGE1r_CLR(AN_X4_SW_AN_BASE_PAGE1r_reg);
      AN_X4_SW_AN_BASE_PAGE1r_SET(AN_X4_SW_AN_BASE_PAGE1r_reg, page);
      PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_SW_AN_BASE_PAGE1r(pc, AN_X4_SW_AN_BASE_PAGE1r_reg));
    }
    else if(page_num == 2) {
      AN_X4_SW_AN_BASE_PAGE2r_CLR(AN_X4_SW_AN_BASE_PAGE2r_reg);
      AN_X4_SW_AN_BASE_PAGE2r_SET(AN_X4_SW_AN_BASE_PAGE2r_reg, page);
      PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_SW_AN_BASE_PAGE2r(pc, AN_X4_SW_AN_BASE_PAGE2r_reg));
    }
  }
  else {
    if(page_num == 0) {
      AN_X4_LD_PAGE0r_CLR(AN_X4_LD_PAGE0r_reg);
      AN_X4_LD_PAGE0r_LD_PAGE_0f_SET(AN_X4_LD_PAGE0r_reg, page);
      PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_PAGE0r(pc, AN_X4_LD_PAGE0r_reg));
    }
    else if(page_num == 1) {
      AN_X4_LD_PAGE1r_CLR(AN_X4_LD_PAGE1r_reg);
      AN_X4_LD_PAGE1r_LD_PAGE_1f_SET(AN_X4_LD_PAGE1r_reg, page);
      PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_PAGE1r(pc, AN_X4_LD_PAGE1r_reg));
    }
    else if(page_num == 2) {
      AN_X4_LD_PAGE2r_CLR(AN_X4_LD_PAGE2r_reg);
      AN_X4_LD_PAGE2r_LD_PAGE_2f_SET(AN_X4_LD_PAGE2r_reg, page);
      PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_PAGE2r(pc, AN_X4_LD_PAGE2r_reg));
    }
  }

  return PHYMOD_E_NONE;

} /*aperta_tbhmod_sw_an_ld_page_load(aperta_tbhmod_st* pc)*/

/*!
@brief Reads the SW AN link partner pages
@param  pc handle to current TSCBH context (#aperta_tbhmod_st)
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.

@details
This function reads SW AN link partner pages. The HW
requires you to read page0 register last.

*/
int aperta_tbhmod_sw_an_lp_page_read(aperta_tbhmod_st* pc)
{
  AN_X4_LP_PAGE0r_t AN_X4_LP_PAGE0r_reg;
  AN_X4_LP_PAGE1r_t AN_X4_LP_PAGE1r_reg;
  AN_X4_LP_PAGE2r_t AN_X4_LP_PAGE2r_reg;

  AN_X4_LP_PAGE2r_CLR(AN_X4_LP_PAGE2r_reg);
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_PAGE2r(pc, &AN_X4_LP_PAGE2r_reg));
  pc->lp_page2 = AN_X4_LP_PAGE2r_LP_PAGE_2f_GET(AN_X4_LP_PAGE2r_reg);

  AN_X4_LP_PAGE1r_CLR(AN_X4_LP_PAGE1r_reg);
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_PAGE1r(pc, &AN_X4_LP_PAGE1r_reg));
  pc->lp_page1 = AN_X4_LP_PAGE1r_LP_PAGE_1f_GET(AN_X4_LP_PAGE1r_reg);

  AN_X4_LP_PAGE0r_CLR(AN_X4_LP_PAGE0r_reg);
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LP_PAGE0r(pc, &AN_X4_LP_PAGE0r_reg));
  pc->lp_page0 = AN_X4_LP_PAGE0r_LP_PAGE_0f_GET(AN_X4_LP_PAGE0r_reg);


  return PHYMOD_E_NONE;

} /*aperta_tbhmod_sw_an_lp_page_read(aperta_tbhmod_st* pc)*/


int aperta_tbhmod_sw_an_set_ack(PHYMOD_ST* pc)
{
  AN_X4_WAIT_ACK_COMPLETEr_t AN_X4_WAIT_ACK_COMPLETEr_reg;



  AN_X4_WAIT_ACK_COMPLETEr_CLR(AN_X4_WAIT_ACK_COMPLETEr_reg);
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_WAIT_ACK_COMPLETEr(pc, &AN_X4_WAIT_ACK_COMPLETEr_reg));
  AN_X4_WAIT_ACK_COMPLETEr_SEND_ACKf_SET(AN_X4_WAIT_ACK_COMPLETEr_reg, 1);
  PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_WAIT_ACK_COMPLETEr(pc, AN_X4_WAIT_ACK_COMPLETEr_reg));

  return PHYMOD_E_NONE;
}


int aperta_tbhmod_sw_an_control(PHYMOD_ST* pc, int sw_an_en)
{
  AN_X4_LD_CTLr_t   AN_X4_LD_CTLr_reg;
  AN_X4_CL73_CFGr_t AN_X4_ABILITIES_CL73_CFGr_reg;



/** Enabling SW AN if sw_an is set **/
  AN_X4_LD_CTLr_CLR(AN_X4_LD_CTLr_reg);
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_CTLr(pc, &AN_X4_LD_CTLr_reg));
  AN_X4_LD_CTLr_AN_TYPE_SWf_SET(AN_X4_LD_CTLr_reg, sw_an_en);
  AN_X4_LD_CTLr_AN_HCD_RES_DISABLEf_SET(AN_X4_LD_CTLr_reg, 0);
  PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LD_CTLr(pc, AN_X4_LD_CTLr_reg));

  AN_X4_CL73_CFGr_CLR(AN_X4_ABILITIES_CL73_CFGr_reg);
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_CL73_CFGr(pc, &AN_X4_ABILITIES_CL73_CFGr_reg));
  AN_X4_CL73_CFGr_CL73_ENABLEf_SET(AN_X4_ABILITIES_CL73_CFGr_reg, sw_an_en);
  PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_CL73_CFGr(pc, AN_X4_ABILITIES_CL73_CFGr_reg));

  return PHYMOD_E_NONE;
}


/*!
@brief Controls the setting/resetting of autoneg ability and enabling/disabling
@param  pc handle to current TSCBH context (#aperta_tbhmod_st)
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.

@details
This function programs auto-negotiation (AN) modes for the TEF. It can
enable/disable clause37/clause73/BAM autonegotiation capabilities. Call this
function once for combo mode and once per lane in independent lane mode.

The autonegotiation mode is indicated by setting an_control as required.
*/

#ifdef _DV_TB_
int plp_aperta_tbhmod_autoneg_control(PHYMOD_ST* pc, autoneg_control_st* an_st)
{
  uint16_t cl73_bam_enable, cl73_enable;
  uint16_t cl73_nonce_match_over, cl73_nonce_match_val ;
  uint16_t num_advertised_lanes;
  AN_X4_CL73_CFGr_t AN_X4_ABILITIES_CL73_CFGr_reg;



  cl73_enable                   = an_st->cl73_enable;
  cl73_nonce_match_over         = an_st->nonce_match_override;
  cl73_nonce_match_val          = an_st->nonce_match_val;
  cl73_bam_enable               = an_st->cl73_bam_enable;

  num_advertised_lanes          = an_st->num_advertised_lanes;



  AN_X4_CL73_CFGr_CLR(AN_X4_ABILITIES_CL73_CFGr_reg);
  AN_X4_CL73_CFGr_NUM_ADVERTISED_LANESf_SET(AN_X4_ABILITIES_CL73_CFGr_reg, num_advertised_lanes);

  AN_X4_CL73_CFGr_CL73_NONCE_MATCH_VALf_SET(AN_X4_ABILITIES_CL73_CFGr_reg, cl73_nonce_match_val);
  AN_X4_CL73_CFGr_CL73_NONCE_MATCH_OVERf_SET(AN_X4_ABILITIES_CL73_CFGr_reg, cl73_nonce_match_over);

  AN_X4_CL73_CFGr_CL73_ENABLEf_SET(AN_X4_ABILITIES_CL73_CFGr_reg, cl73_enable);
  AN_X4_CL73_CFGr_MSA_FEC_MAPPINGf_SET(AN_X4_ABILITIES_CL73_CFGr_reg, an_st->an_msa_fec_mapping);
  AN_X4_CL73_CFGr_CL73_BAM_ENABLEf_SET(AN_X4_ABILITIES_CL73_CFGr_reg, cl73_bam_enable);

  /*********Setting AN_X4_ABILITIES_cl73_cfg 0xC180 ******/
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_CL73_CFGr(pc, AN_X4_ABILITIES_CL73_CFGr_reg));

  return PHYMOD_E_NONE;

} /* plp_aperta_tbhmod_autoneg_control(aperta_tbhmod_st* pc) */
#endif





int aperta_tbhmod_autoneg_restart(aperta_tbhmod_st *pc)
{
  AN_X4_CL73_CFGr_t AN_X4_ABILITIES_CL73_CFGr_reg;

  APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc);

  AN_X4_CL73_CFGr_CLR(AN_X4_ABILITIES_CL73_CFGr_reg);
  PHYMOD_IF_ERR_RETURN(READ_AN_X4_CL73_CFGr(pc, &AN_X4_ABILITIES_CL73_CFGr_reg));
  AN_X4_CL73_CFGr_CL73_AN_RESTARTf_SET(AN_X4_ABILITIES_CL73_CFGr_reg, 0);
  PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_CL73_CFGr(pc, AN_X4_ABILITIES_CL73_CFGr_reg));

  PHYMOD_IF_ERR_RETURN(READ_AN_X4_CL73_CFGr(pc, &AN_X4_ABILITIES_CL73_CFGr_reg));
  AN_X4_CL73_CFGr_CL73_AN_RESTARTf_SET(AN_X4_ABILITIES_CL73_CFGr_reg, 1);
  PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_CL73_CFGr(pc, AN_X4_ABILITIES_CL73_CFGr_reg));

  return PHYMOD_E_NONE;
}

#endif /* DV_AN_SUPPORT */

/**
@brief   getRevDetails , calls revid_read
@param   pc handle to current TSCBH context (#aperta_tbhmod_st)
@returns The value PHYMOD_E_NONE upon successful completion
@details
*/
/* internal function */
int _plp_aperta_tbhmod_getRevDetails(PHYMOD_ST* pc)
{
 /* MAIN0_SERDESIDr_t MAIN0_SERDESIDr_reg;
  MAIN0_SERDESIDr_CLR(MAIN0_SERDESIDr_reg);
  PHYMOD_IF_ERR_RETURN(READ_MAIN0_SERDESIDr(pc, &MAIN0_SERDESIDr_reg));
  if(MAIN0_SERDESIDr_GET(MAIN0_SERDESIDr_reg) != 0x42d4) {
    return PHYMOD_E_FAIL;
  } else {
    return PHYMOD_E_NONE;
  }*/
 return PHYMOD_E_NONE;
}

/*!
@brief Sets the core for autoneg
@param  pc handle to current TSCBH context (#aperta_tbhmod_st)
@returns The value PHYMOD_E_NONE upon successful completion
This function sets the tsc core for autoneg generic settings.
If per_lane_control=APERTA_TBHMOD_CTL_AN_CHIP_INIT(0x1) PLL and RX path are reset.
If per_lane_control=APERTA_TBHMOD_CTL_AN_MODE_INIT(0x2) PLL and RX path are not reset.
*/
int plp_aperta_tbhmod_set_an_port_mode(PHYMOD_ST* pc, int starting_lane)    /* SET_AN_PORT_MODE */
{
  uint16_t new_port_mode_sel = 0;
  uint16_t modify_port_mode = 0, is_octal_mode = 0;
  uint16_t port_mode_sel_reg;
  MAIN0_SETUPr_t MAIN0_SETUPr_reg;
  PHYMOD_ST phy_copy;

  PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));

  MAIN0_SETUPr_CLR(MAIN0_SETUPr_reg);

  PHYMOD_IF_ERR_RETURN (READ_MAIN0_SETUPr(pc, &MAIN0_SETUPr_reg));

  port_mode_sel_reg = MAIN0_SETUPr_PORT_MODE_SELf_GET(MAIN0_SETUPr_reg);

  if(port_mode_sel_reg == APERTA_TBHMOD_DXGXS) {
      modify_port_mode = 1;
      if((starting_lane == 0) || (starting_lane == 4)) {
          new_port_mode_sel = APERTA_TBHMOD_TRI1_PORT;
      }
      else if((starting_lane == 2)  || (starting_lane == 6)) {
          new_port_mode_sel = APERTA_TBHMOD_TRI2_PORT;
      }
  }
  if(port_mode_sel_reg == APERTA_TBHMOD_TRI1_PORT) {
      if((starting_lane == 2) || (starting_lane == 6)) {
          modify_port_mode = 1;
          new_port_mode_sel = APERTA_TBHMOD_MULTI_PORT;
      }
  }
  if(port_mode_sel_reg == APERTA_TBHMOD_TRI2_PORT) {
      if(starting_lane == 0 || (starting_lane == 4)  ) {
          modify_port_mode = 1;
          new_port_mode_sel = APERTA_TBHMOD_MULTI_PORT;
      }
  }
  if(port_mode_sel_reg == APERTA_TBHMOD_SINGLE_PORT) {
      modify_port_mode = 1;
      new_port_mode_sel = APERTA_TBHMOD_MULTI_PORT;
  }
  if(port_mode_sel_reg == APERTA_TBHMOD_SINGLE_OCTAL_PORT) {
      modify_port_mode = 1;
      is_octal_mode = 1;
      new_port_mode_sel = APERTA_TBHMOD_MULTI_PORT;
  }

  if (pc->access.lane_mask & 0xf) {
      phy_copy.access.lane_mask = 0x1;
  } else {
      phy_copy.access.lane_mask = 0x10;
  }

  if(modify_port_mode == 1) {
    MAIN0_SETUPr_PORT_MODE_SELf_SET(MAIN0_SETUPr_reg, new_port_mode_sel);
  }

  PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_SETUPr(&phy_copy, MAIN0_SETUPr_reg));

  if (is_octal_mode) {
      phy_copy.access.lane_mask = 0x10;
      PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_SETUPr(&phy_copy, MAIN0_SETUPr_reg));
  }

  return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_set_an_timers(PHYMOD_ST* pc, plp_aperta_phymod_ref_clk_t refclk, uint32_t *pam4_an_timer_value)    /* SET_AN_TIMERS */
{
    AN_X1_IGNORE_LNK_TMRr_t AN_X1_TIMERS_IGNORE_LINK_TIMERr_reg;
    RX_X4_RS_FEC_TMRr_t RX_X4_RS_FEC_TMRr_reg;

    AN_X1_CL73_ERRr_t  AN_X1_CL73_ERRr_reg;
    AN_X1_CL73_BRK_LNKr_t AN_X1_TIMERS_CL73_BREAK_LINKr_reg;
    AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg;
    AN_X1_LNK_FAIL_INHBT_TMR_CL72r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg;

    AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg;
    AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg;
    AN_X1_IGNORE_LNK_TMR_PAM4r_t AN_X1_IGNORE_LNK_TMR_PAM4r_reg;

    MAIN0_TICK_CTL0r_t MAIN0_TICK_CONTROL_0r_reg;
    MAIN0_TICK_CTL1r_t MAIN0_TICK_CONTROL_1r_reg;

    if (refclk == phymodRefClk312Mhz) {
        /* need to set the tick count from 15 us to 60us */
        MAIN0_TICK_CTL0r_CLR(MAIN0_TICK_CONTROL_0r_reg);
        MAIN0_TICK_CTL0r_TICK_DENOMINATORf_SET(MAIN0_TICK_CONTROL_0r_reg, 1);
        MAIN0_TICK_CTL0r_TICK_NUMERATOR_LOWERf_SET(MAIN0_TICK_CONTROL_0r_reg, 0xe);

        PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_TICK_CTL0r(pc, MAIN0_TICK_CONTROL_0r_reg));

        MAIN0_TICK_CTL1r_CLR(MAIN0_TICK_CONTROL_1r_reg);
        MAIN0_TICK_CTL1r_TICK_OVERRIDEf_SET(MAIN0_TICK_CONTROL_1r_reg, 1);
        MAIN0_TICK_CTL1r_TICK_NUMERATOR_UPPERf_SET(MAIN0_TICK_CONTROL_1r_reg, 0x493);
        PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_TICK_CTL1r(pc, MAIN0_TICK_CONTROL_1r_reg));
    } else {
        /* need to set the tick count from 15 us to 60us */
        MAIN0_TICK_CTL0r_CLR(MAIN0_TICK_CONTROL_0r_reg);
        MAIN0_TICK_CTL0r_TICK_DENOMINATORf_SET(MAIN0_TICK_CONTROL_0r_reg, 1);
        MAIN0_TICK_CTL0r_TICK_NUMERATOR_LOWERf_SET(MAIN0_TICK_CONTROL_0r_reg, 0xf);

        PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_TICK_CTL0r(pc, MAIN0_TICK_CONTROL_0r_reg));

        MAIN0_TICK_CTL1r_CLR(MAIN0_TICK_CONTROL_1r_reg);
        MAIN0_TICK_CTL1r_TICK_OVERRIDEf_SET(MAIN0_TICK_CONTROL_1r_reg, 1);
        MAIN0_TICK_CTL1r_TICK_NUMERATOR_UPPERf_SET(MAIN0_TICK_CONTROL_1r_reg, 0x249);
        PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_TICK_CTL1r(pc, MAIN0_TICK_CONTROL_1r_reg));
    }

    /* first program NRZ speed related timer value */
    /*0x9250  65.04 ms */
    AN_X1_CL73_BRK_LNKr_CLR(AN_X1_TIMERS_CL73_BREAK_LINKr_reg);
    AN_X1_CL73_BRK_LNKr_SET(AN_X1_TIMERS_CL73_BREAK_LINKr_reg, 0x10f0);
    PHYMOD_IF_ERR_RETURN (WRITE_AN_X1_CL73_BRK_LNKr(pc, AN_X1_TIMERS_CL73_BREAK_LINKr_reg));
    /* 0x9251  25.02 ms */
    AN_X1_CL73_ERRr_CLR(AN_X1_CL73_ERRr_reg);
    AN_X1_CL73_ERRr_SET(AN_X1_CL73_ERRr_reg, 0x0);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL73_ERRr(pc, AN_X1_CL73_ERRr_reg));
    /* 0x9254 10.02 ms */
    AN_X1_IGNORE_LNK_TMRr_CLR(AN_X1_TIMERS_IGNORE_LINK_TIMERr_reg);
    AN_X1_IGNORE_LNK_TMRr_SET(AN_X1_TIMERS_IGNORE_LINK_TIMERr_reg, 0x29c);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_IGNORE_LNK_TMRr(pc, AN_X1_TIMERS_IGNORE_LINK_TIMERr_reg));
    /* 0x9255 500.04 ms */
    AN_X1_LNK_FAIL_INHBT_TMR_CL72r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg);
    AN_X1_LNK_FAIL_INHBT_TMR_CL72r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg, 0x8236);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_CL72r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg));
    /* 0x9256 160.98 ms */
    AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg);
    AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg, 0x29ab);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg));
    /* 0xc130 60 ms */
    RX_X4_RS_FEC_TMRr_CLR(RX_X4_RS_FEC_TMRr_reg);
    RX_X4_RS_FEC_TMRr_SET(RX_X4_RS_FEC_TMRr_reg, 0xfa0);
    PHYMOD_IF_ERR_RETURN(WRITE_RX_X4_RS_FEC_TMRr(pc, RX_X4_RS_FEC_TMRr_reg));

    /*next program PAM4 related timer */
    if (pam4_an_timer_value == NULL) {
        /*then default value */
		/* 0x9259 10.08 ms */
        AN_X1_IGNORE_LNK_TMR_PAM4r_CLR(AN_X1_IGNORE_LNK_TMR_PAM4r_reg);
        /*AN_X1_IGNORE_LNK_TMR_PAM4r_SET(AN_X1_IGNORE_LNK_TMR_PAM4r_reg, 0xa8);*/
        AN_X1_IGNORE_LNK_TMR_PAM4r_SET(AN_X1_IGNORE_LNK_TMR_PAM4r_reg, 0x2a);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_IGNORE_LNK_TMR_PAM4r(pc, AN_X1_IGNORE_LNK_TMR_PAM4r_reg));
        /*0x925a 3199.9 ms */
        AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg);
        /*AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg, 0xc238);*/
        AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg, 0x3415);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg));
        /*0x925b 1000.08 ms */
        AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg);
        /*AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg, 0x41aa);*/
        AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg, 0x106a);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg));
    } else {
        /*then user passed value */
        AN_X1_IGNORE_LNK_TMR_PAM4r_CLR(AN_X1_IGNORE_LNK_TMR_PAM4r_reg);
        AN_X1_IGNORE_LNK_TMR_PAM4r_SET(AN_X1_IGNORE_LNK_TMR_PAM4r_reg, *pam4_an_timer_value);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_IGNORE_LNK_TMR_PAM4r(pc, AN_X1_IGNORE_LNK_TMR_PAM4r_reg));
        /*0x925a*/
        AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg);
        AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg, *(pam4_an_timer_value + 1));
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg));
        /*0x925b*/
        AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg);
        AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg,  *(pam4_an_timer_value + 2));
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg));
   }
    return PHYMOD_E_NONE;
}

#if 0
int plp_aperta_tbhmod_set_an_timers(PHYMOD_ST* pc)    /* SET_AN_TIMERS */
{
#ifdef DV_AN_SUPPORT
  uint16_t ignore_link_timer_period;
  uint16_t tx_disable_timer_period;
  uint16_t link_fail_inhibit_no_cl72_period ;
  uint16_t link_fail_inhibit_cl72_period ;
  uint16_t ignore_link_timer_period_pam4;
  uint16_t link_fail_inhibit_no_cl72_period_pam4 ;
  uint16_t link_fail_inhibit_cl72_period_pam4 ;


  AN_X1_IGNORE_LNK_TMRr_t AN_X1_TIMERS_IGNORE_LINK_TIMERr_reg;
  AN_X1_IGNORE_LNK_TMR_PAM4r_t AN_X1_TIMERS_IGNORE_LINK_TIMER_PAM4r_reg;
  MAIN0_TICK_CTL0r_t MAIN0_TICK_CONTROL_0r_reg;
  MAIN0_TICK_CTL1r_t MAIN0_TICK_CONTROL_1r_reg;

  AN_X1_OUI_LWRr_t AN_X1_CONTROL_OUI_LOWERr_reg;
  AN_X1_OUI_UPRr_t AN_X1_CONTROL_OUI_UPPERr_reg;
  AN_X1_CL73_BRK_LNKr_t AN_X1_TIMERS_CL73_BREAK_LINKr_reg;
  AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg;
  AN_X1_LNK_FAIL_INHBT_TMR_CL72r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg;

  AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg;
  AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg;

  APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc);

  ignore_link_timer_period         = 0xf;
  link_fail_inhibit_no_cl72_period = 0x64;
  link_fail_inhibit_cl72_period    = 0xfff;
  ignore_link_timer_period_pam4    = ignore_link_timer_period/4 +1;
  link_fail_inhibit_no_cl72_period_pam4 = link_fail_inhibit_no_cl72_period/4 +1;
  link_fail_inhibit_cl72_period_pam4    = link_fail_inhibit_cl72_period/4 +1;

  pc->port_type = APERTA_TBHMOD_MULTI_PORT;

  if(pc->ignore_link_timer_period !=0) ignore_link_timer_period = pc->ignore_link_timer_period;
  AN_X1_IGNORE_LNK_TMRr_CLR(AN_X1_TIMERS_IGNORE_LINK_TIMERr_reg);
  AN_X1_IGNORE_LNK_TMRr_IGNORE_LINK_TIMER_PERIODf_SET(AN_X1_TIMERS_IGNORE_LINK_TIMERr_reg, ignore_link_timer_period);
  PHYMOD_IF_ERR_RETURN (MODIFY_AN_X1_IGNORE_LNK_TMRr(pc, AN_X1_TIMERS_IGNORE_LINK_TIMERr_reg));

  if(pc->ignore_link_timer_period_pam4 !=0) ignore_link_timer_period_pam4 = pc->ignore_link_timer_period_pam4;
  AN_X1_IGNORE_LNK_TMR_PAM4r_CLR(AN_X1_TIMERS_IGNORE_LINK_TIMER_PAM4r_reg);
  AN_X1_IGNORE_LNK_TMR_PAM4r_IGNORE_LINK_TIMER_PERIOD_PAM4f_SET(AN_X1_TIMERS_IGNORE_LINK_TIMER_PAM4r_reg, ignore_link_timer_period_pam4);
  PHYMOD_IF_ERR_RETURN (MODIFY_AN_X1_IGNORE_LNK_TMR_PAM4r(pc, AN_X1_TIMERS_IGNORE_LINK_TIMER_PAM4r_reg));

/* 9003/9004 main0_tick_control1/0 only used from mpp0 copy */
  MAIN0_TICK_CTL0r_CLR(MAIN0_TICK_CONTROL_0r_reg);
  MAIN0_TICK_CTL0r_TICK_DENOMINATORf_SET(MAIN0_TICK_CONTROL_0r_reg, pc->tick_denominator);
  MAIN0_TICK_CTL0r_TICK_NUMERATOR_LOWERf_SET(MAIN0_TICK_CONTROL_0r_reg, pc->tick_numerator_lower);

  PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_TICK_CTL0r(pc, MAIN0_TICK_CONTROL_0r_reg));

  MAIN0_TICK_CTL1r_CLR(MAIN0_TICK_CONTROL_1r_reg);
  MAIN0_TICK_CTL1r_TICK_OVERRIDEf_SET(MAIN0_TICK_CONTROL_1r_reg, pc->tick_override);
  MAIN0_TICK_CTL1r_TICK_NUMERATOR_UPPERf_SET(MAIN0_TICK_CONTROL_1r_reg, pc->tick_numerator_upper);


  PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_TICK_CTL1r(pc, MAIN0_TICK_CONTROL_1r_reg));
  if(
      (pc->tsc_touched_mpp0 ==0 && pc->this_lane < 4) ||
      (pc->tsc_touched_mpp1 ==0 && pc->this_lane > 3 && pc->this_lane < 8)
      ){
    if(pc->tx_disable_timer_period !=0) tx_disable_timer_period = pc->tx_disable_timer_period;

    AN_X1_CL73_BRK_LNKr_CLR(AN_X1_TIMERS_CL73_BREAK_LINKr_reg);
    AN_X1_CL73_BRK_LNKr_CL73_BREAK_TIMER_PERIODf_SET(AN_X1_TIMERS_CL73_BREAK_LINKr_reg, tx_disable_timer_period);
    PHYMOD_IF_ERR_RETURN (MODIFY_AN_X1_CL73_BRK_LNKr(pc, AN_X1_TIMERS_CL73_BREAK_LINKr_reg));

    AN_X1_OUI_LWRr_CLR(AN_X1_CONTROL_OUI_LOWERr_reg);
    AN_X1_OUI_LWRr_SET(AN_X1_CONTROL_OUI_LOWERr_reg, pc->oui & 0xffff);
    /********Setting AN_X1_OUI_LWR 0x9241*****/
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_OUI_LWRr(pc, AN_X1_CONTROL_OUI_LOWERr_reg));

    AN_X1_OUI_UPRr_CLR(AN_X1_CONTROL_OUI_UPPERr_reg);
    AN_X1_OUI_UPRr_SET(AN_X1_CONTROL_OUI_UPPERr_reg, (pc->oui >> 16) & 0xffff);
    /********Setting AN_X1_OUI_UPR 0x9240*****/
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_OUI_UPRr(pc, AN_X1_CONTROL_OUI_UPPERr_reg));
  }


  /**** 0x9256*******/
  if(pc->link_fail_inhibit_no_cl72_period !=0) link_fail_inhibit_no_cl72_period = pc->link_fail_inhibit_no_cl72_period;
  AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg);
  AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_LINK_FAIL_INHIBIT_TIMER_NCL72_PERIODf_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg, link_fail_inhibit_no_cl72_period);

  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg));

  /**** 0x9255*******/
  if(pc->link_fail_inhibit_cl72_period !=0) link_fail_inhibit_cl72_period = pc->link_fail_inhibit_cl72_period;

  AN_X1_LNK_FAIL_INHBT_TMR_CL72r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg);
  AN_X1_LNK_FAIL_INHBT_TMR_CL72r_LINK_FAIL_INHIBIT_TIMER_CL72_PERIODf_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg, link_fail_inhibit_cl72_period);
  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_CL72r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg));


/**** 0x925a*******/
   if(pc->link_fail_inhibit_no_cl72_period_pam4 !=0) link_fail_inhibit_no_cl72_period_pam4 = pc->link_fail_inhibit_no_cl72_period_pam4;
  AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg);
  AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_LINK_FAIL_INHIBIT_TIMER_NCL72_PERIOD_PAM4f_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg, link_fail_inhibit_no_cl72_period_pam4);


  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg));

  /**** 0x925b*******/
   if(pc->link_fail_inhibit_cl72_period_pam4 !=0) link_fail_inhibit_cl72_period_pam4 = pc->link_fail_inhibit_cl72_period_pam4;

  AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg);
  AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_LINK_FAIL_INHIBIT_TIMER_CL72_PERIOD_PAM4f_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg, link_fail_inhibit_cl72_period_pam4);

  PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg));


#endif /* DV_AN_SUPPORT */
  return PHYMOD_E_NONE;
} /* plp_aperta_tbhmod_set_an_timers(aperta_tbhmod_st* pc) */

#endif




#ifdef _DV_TB_
/*!
@brief Local Fault/Remote Fault signalling enable for Tx and RX
@param  pc handle to current TSCBH context (#aperta_tbhmod_st)
@returns The value PHYMOD_E_NONE upon successful completion.
@details
This function is used to enable/disable Link Interrupt, Local Fault and
Remote Fault signalling of any lane .

\li #aperta_tbhmod_st::per_lane_control[3:0] represents the mode (CL49, CL48, CL82).
\li #aperta_tbhmod_st::per_lane_control[6]   is for li;
\li #aperta_tbhmod_st::per_lane_control[5]   is for lf;
\li #aperta_tbhmod_st::per_lane_control[4]   is for rf;

This function reads/modifies the following registers:

\li cl49_tx            :   0xc113
\li cl49_rx            :   0xc134
\li cl48_tx            :   0xa001
\li cl82_tx            :   0xa002
\li cl48_cl82_rx       :   0xa024
*/

int aperta_tbhmod_lf_rf_control(aperta_tbhmod_st* pc)         /* LF_RF_CONTROL */
{
  APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc);
  return PHYMOD_E_NONE;
}
#endif

#if 0

/**
@brief   Checks config valid status for the port
@param   pc handle to current TSCBH context (#aperta_tbhmod_st)
@returns The value PHYMOD_E_NONE upon successful completion
@details This register bit indicates that PCS is now programmed as required
by the HT entry for that speed
*/
int _aperta_tbhmod_wait_sc_stats_set(aperta_tbhmod_st* pc)
{
#if defined(_DV_TB_) || defined(_SDK_APERTA_TBHMOD_)
  uint16_t data;
  uint16_t i;
  /*SC_X4_STSr_t reg_sc_ctrl_sts;

  SC_X4_STSr_CLR(reg_sc_ctrl_sts);*/
  i = 0;
  data = 0;
#endif /* _DV_TB_ || _SDK_APERTA_TBHMOD_ */

#ifdef _DV_TB_
  while(1){
   i=i+1; /* added only to eliminate compile warning */
   /*PHYMOD_IF_ERR_RETURN(READ_SC_X4_STSr(pc, &reg_sc_ctrl_sts));
   data = SC_X4_STSr_SW_SPEED_CONFIG_VLDf_GET(reg_sc_ctrl_sts);*/
   if(data == 1)
     break;
  }
#endif /* _DV_TB_ */
  return PHYMOD_E_NONE;
}

#endif /* 0 */



/*!
@brief tx lane reset and enable of any particular lane
@param  pc handle to current TSCBH context (#aperta_tbhmod_st)
@param  reset gets the configured tx rstb
@param  enable gets the configured tx enable
@returns The value PHYMOD_E_NONE upon successful completion.
@details
This function reads the following registers:

\li rstb_tx_lane  :   0xc113[1]
\li enable_tx_lane:   0xc113[0]
*/
int plp_aperta_tbhmod_tx_lane_control_get(PHYMOD_ST* pc,  int *reset, int *enable)         /* TX_LANE_CONTROL */
{
  TX_X4_MISCr_t TX_X4_CONTROL0_MISCr_reg;

  TX_X4_MISCr_CLR(TX_X4_CONTROL0_MISCr_reg);

  PHYMOD_IF_ERR_RETURN (READ_TX_X4_MISCr(pc, &TX_X4_CONTROL0_MISCr_reg));

  *reset = TX_X4_MISCr_RSTB_TX_LANEf_GET(TX_X4_CONTROL0_MISCr_reg);
  *enable = TX_X4_MISCr_ENABLE_TX_LANEf_GET(TX_X4_CONTROL0_MISCr_reg);

   return PHYMOD_E_NONE;
}


/*!
@brief Read the 16 bit rev. id value etc.
@param  pc handle to current TSCBH context (#aperta_tbhmod_st)
@param  revIdV receives the revid
@returns The value of the revid.
@details
This  fucntion reads the revid register that contains core number, revision
number and returns it.
*/
int plp_aperta_tbhmod_revid_read(PHYMOD_ST* pc, uint32_t *revIdV)              /* REVID_READ */
{
#ifdef _DV_TB_
  int laneselect;
#endif
  APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc);
#ifdef _DV_TB_
  laneselect = pc->lane_select;
  pc->lane_select=APERTA_TBHMOD_LANE_0_0_0_1;

  PHYMOD_DEBUG_ERROR(("%-22s u=%0d p=%0d sel=%x ln=%0d dxgxs=%0d\n", __func__,
            pc->unit, pc->port, pc->lane_select, pc->this_lane, pc->dxgxs));
#endif
  *revIdV=_plp_aperta_tbhmod_getRevDetails(pc);
#ifdef _DV_TB_
  pc->lane_select=laneselect;
#endif
  return PHYMOD_E_NONE;
}


/* RX_X4_Fec_Control_fec_0 */
int plp_aperta_tbhmod_cl74_control_fec_0(PHYMOD_ST* pc, int cl74_good_parity_cnt, int cl74_invalid_parity_cnt, int cl74_dec_max_pm, int cl74_burst_err_status_mode, int cl74_dbg_err_mode)
{
  RX_X4_FEC0r_t     RX_X4_FEC0r_reg;

  /* APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc); */

  PHYMOD_DEBUG_ERROR(("%-22s CL74_CONTROL_FEC_0 Reg: cl74_good_parity_cnt=%0x cl74_invalid_parity_cnt=%0x cl74_dec_max_pm=%x cl74_burst_err_status_mode=%0x cl74_dbg_err_mode=%0x ln=%0x\n", __func__,
            cl74_good_parity_cnt, cl74_invalid_parity_cnt, cl74_dec_max_pm, cl74_burst_err_status_mode, cl74_dbg_err_mode, pc->access.lane_mask));


  RX_X4_FEC0r_CLR(RX_X4_FEC0r_reg);
  PHYMOD_IF_ERR_RETURN(READ_RX_X4_FEC0r(pc, &RX_X4_FEC0r_reg));
  RX_X4_FEC0r_GOOD_PARITY_CNTf_SET(RX_X4_FEC0r_reg, cl74_good_parity_cnt);
  RX_X4_FEC0r_INVALID_PARITY_CNTf_SET(RX_X4_FEC0r_reg, cl74_invalid_parity_cnt);
  RX_X4_FEC0r_DEC_MAX_PMf_SET(RX_X4_FEC0r_reg, cl74_dec_max_pm);
  RX_X4_FEC0r_BURST_ERR_STATUS_MODEf_SET(RX_X4_FEC0r_reg, cl74_burst_err_status_mode);
  RX_X4_FEC0r_DBG_ERR_MODEf_SET(RX_X4_FEC0r_reg, cl74_dbg_err_mode);
  PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_FEC0r(pc, RX_X4_FEC0r_reg));

  return PHYMOD_E_NONE;
}

/* RX_X4_Fec_Control_fec_1 */
int plp_aperta_tbhmod_cl74_control_fec_1(PHYMOD_ST* pc,int cl74_dec_pm_mode, int cl74_dec_19b_burst_gap_count, int cl74_dec_18b_burst_gap_count, int cl74_dec_17b_burst_gap_count, int cl74_dec_gap_count_mode)
{
  RX_X4_FEC1r_t     RX_X4_FEC1r_reg;


  /* APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc); */

  PHYMOD_DEBUG_ERROR(("%-22s CL74_CONTROL_FEC_1 Reg: cl74_dec_pm_mode=%0x cl74_dec_19b_burst_gap_count=%0x cl74_dec_18b_burst_gap_count=%x cl74_dec_17b_burst_gap_count=%0x cl74_dec_gap_count_mode=%0x ln=%0x\n", __func__,
            cl74_dec_pm_mode, cl74_dec_19b_burst_gap_count, cl74_dec_18b_burst_gap_count, cl74_dec_17b_burst_gap_count, cl74_dec_gap_count_mode, pc->access.lane_mask));


  RX_X4_FEC1r_CLR(RX_X4_FEC1r_reg);
  PHYMOD_IF_ERR_RETURN(READ_RX_X4_FEC1r(pc, &RX_X4_FEC1r_reg));
  RX_X4_FEC1r_DEC_PM_MODEf_SET(RX_X4_FEC1r_reg, cl74_dec_pm_mode);
  RX_X4_FEC1r_DEC_19B_BURST_GAP_COUNTf_SET(RX_X4_FEC1r_reg, cl74_dec_19b_burst_gap_count);
  RX_X4_FEC1r_DEC_18B_BURST_GAP_COUNTf_SET(RX_X4_FEC1r_reg, cl74_dec_18b_burst_gap_count);
  RX_X4_FEC1r_DEC_17B_BURST_GAP_COUNTf_SET(RX_X4_FEC1r_reg, cl74_dec_17b_burst_gap_count);
  RX_X4_FEC1r_DEC_GAP_COUNT_MODEf_SET(RX_X4_FEC1r_reg, cl74_dec_gap_count_mode);
  PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_FEC1r(pc, RX_X4_FEC1r_reg));

  return PHYMOD_E_NONE;
}

/* RX_X4_Fec_Control_fec_2 */
int plp_aperta_tbhmod_cl74_control_fec_2(PHYMOD_ST* pc, int cl74_fec_err_enable_per_stream_mpp0, int cl74_dbg_enable_per_stream_mpp0, int cl74_fec_error_code_all_per_stream_mpp0, int cl74_fec_err_enable_per_stream_mpp1, int cl74_dbg_enable_per_stream_mpp1, int cl74_fec_error_code_all_per_stream_mpp1, aperta_tbhmod_port_type_t port_type)
{
#ifdef FIXME_YZ /* mpp logic inside */
  int this_lane,lane;
  RX_X4_FEC2r_t     RX_X4_FEC2r_reg;

  /* APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc); */

  PHYMOD_DEBUG_ERROR(("%-22s CL74_CONTROL_FEC_2 Reg: cl74_fec_err_enable_per_stream_mpp0=%0x cl74_dbg_enable_per_stream_mpp0=%0x cl74_fec_error_code_all_per_stream_mpp0=%x ln=%0x, port_type = %0x\n", __func__,
            cl74_fec_err_enable_per_stream_mpp0, cl74_dbg_enable_per_stream_mpp0, cl74_fec_error_code_all_per_stream_mpp0, pc.access.lane_mask, port_type));
  PHYMOD_DEBUG_ERROR(("%-22s CL74_CONTROL_FEC_2 Reg: cl74_fec_err_enable_per_stream_mpp1=%0x cl74_dbg_enable_per_stream_mpp1=%0x cl74_fec_error_code_all_per_stream_mpp1=%x ln=%0x, port_type = %0x\n", __func__,
            cl74_fec_err_enable_per_stream_mpp1, cl74_dbg_enable_per_stream_mpp1, cl74_fec_error_code_all_per_stream_mpp1, pc.access.lane_mask, port_type));


  this_lane = pc.this_lane;
  lane      = pc.this_lane;

  RX_X4_FEC2r_CLR(RX_X4_FEC2r_reg);
  if(lane/4==0) {
    RX_X4_FEC2r_FEC_ERR_ENABLEf_SET(RX_X4_FEC2r_reg,  ((cl74_fec_err_enable_per_stream_mpp0>>(lane*5))&0x1f));
    RX_X4_FEC2r_DBG_ENABLEf_SET(RX_X4_FEC2r_reg, ((cl74_dbg_enable_per_stream_mpp0>>(lane*5))&0x1f));
    RX_X4_FEC2r_FEC_ERROR_CODE_ALLf_SET(RX_X4_FEC2r_reg, ((cl74_fec_error_code_all_per_stream_mpp0>>(lane*5))&0x1f));
  }
  else {
    RX_X4_FEC2r_FEC_ERR_ENABLEf_SET(RX_X4_FEC2r_reg,  ((cl74_fec_err_enable_per_stream_mpp1>>((lane - 4)*5))&0x1f));
    RX_X4_FEC2r_DBG_ENABLEf_SET(RX_X4_FEC2r_reg, ((cl74_dbg_enable_per_stream_mpp1>>((lane - 4)*5))&0x1f));
    RX_X4_FEC2r_FEC_ERROR_CODE_ALLf_SET(RX_X4_FEC2r_reg, ((cl74_fec_error_code_all_per_stream_mpp1>>((lane - 4)*5))&0x1f));
  }
  PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_FEC2r(pc, RX_X4_FEC2r_reg));

  if(port_type == APERTA_TBHMOD_SINGLE_PORT) {
    do {
      this_lane++;
      pc.this_lane=this_lane;
      if(lane/4==0) {
        RX_X4_FEC2r_FEC_ERR_ENABLEf_SET(RX_X4_FEC2r_reg, ((cl74_fec_err_enable_per_stream_mpp0>>(this_lane*5))&0x1f));
        RX_X4_FEC2r_DBG_ENABLEf_SET(RX_X4_FEC2r_reg, ((cl74_dbg_enable_per_stream_mpp0>>(this_lane*5))&0x1f));
        RX_X4_FEC2r_FEC_ERROR_CODE_ALLf_SET(RX_X4_FEC2r_reg, ((cl74_fec_error_code_all_per_stream_mpp0>>(this_lane*5))&0x1f));
      }
      else {
        RX_X4_FEC2r_FEC_ERR_ENABLEf_SET(RX_X4_FEC2r_reg, ((cl74_fec_err_enable_per_stream_mpp1>>((this_lane - 4)*5))&0x1f));
        RX_X4_FEC2r_DBG_ENABLEf_SET(RX_X4_FEC2r_reg, ((cl74_dbg_enable_per_stream_mpp1>>((this_lane - 4)*5))&0x1f));
        RX_X4_FEC2r_FEC_ERROR_CODE_ALLf_SET(RX_X4_FEC2r_reg, ((cl74_fec_error_code_all_per_stream_mpp1>>((this_lane - 4)*5))&0x1f));
      }
      PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_FEC2r(pc, RX_X4_FEC2r_reg));
    } while((this_lane - lane)!=3);
    pc.this_lane = 0;
    this_lane = 0;
  }
#endif
  return PHYMOD_E_NONE;
}



/* RX_X4_Control0_RS_FEC_rx_control_0 */
int plp_aperta_tbhmod_rs_fec_rx_control_0(PHYMOD_ST* pc, int cl91_bypass_indication)
{
  RX_X4_RS_FEC_RX_CTL0r_t   RX_X4_RS_FEC_RX_CTL0r_reg;

  /* APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc); */

  PHYMOD_DEBUG_ERROR(("%-22s RX_X4_Control0_RS_FEC_rx_control_0 Reg: cl91_bypass_indication=%0x ln=%0x\n", __func__,
            cl91_bypass_indication, pc->access.lane_mask));


  RX_X4_RS_FEC_RX_CTL0r_CLR(RX_X4_RS_FEC_RX_CTL0r_reg);
  PHYMOD_IF_ERR_RETURN(READ_RX_X4_RS_FEC_RX_CTL0r(pc, &RX_X4_RS_FEC_RX_CTL0r_reg));
  RX_X4_RS_FEC_RX_CTL0r_CW_BAD_ENABLEf_SET(RX_X4_RS_FEC_RX_CTL0r_reg, ~cl91_bypass_indication);
  PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_RS_FEC_RX_CTL0r(pc, RX_X4_RS_FEC_RX_CTL0r_reg));

  return PHYMOD_E_NONE;
}


/*!
@brief disables the PCS by clearing the writing to Speed control reg
@param  pc handle to current TSCBH context (#aperta_tbhmod_st)
@returns The value PHYMOD_E_NONE upon successful completion.
@details
TBD
*/

#ifdef _DV_TB_
int aperta_tbhmod_disable_pcs_blackhawk(PHYMOD_ST* pc)
{
  SC_X4_CTLr_t SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg;

  SC_X4_CTLr_CLR(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg);
  SC_X4_CTLr_SW_SPEED_CHANGEf_SET(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg, 0);

  PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(pc, SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg));

  return PHYMOD_E_NONE;
}
#endif /* _DV_TB_*/



#ifdef _DV_TB_
/*!
@brief removes resets from all the PMD lanes
@param  pc handle to current TSCBH context (#aperta_tbhmod_st)
@returns The value PHYMOD_E_NONE upon successful completion
@details
Removes resets
*/

int aperta_tbhmod_pmd_reset_remove(PHYMOD_ST* pc, int pmd_touched)
{
  PMD_X1_CTLr_t PMD_X1_PMD_X1_CONTROLr_reg;
  PMD_X4_CTLr_t PMD_X4_PMD_X4_CONTROLr_reg;


  if(pmd_touched == 0) {
    PMD_X1_CTLr_CLR(PMD_X1_PMD_X1_CONTROLr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X1_CTLr(pc, &PMD_X1_PMD_X1_CONTROLr_reg));
    /* setup the PMD reset */
    PMD_X1_CTLr_POR_H_RSTBf_SET(PMD_X1_PMD_X1_CONTROLr_reg, 1);
    PMD_X1_CTLr_CORE_DP_H_RSTBf_SET(PMD_X1_PMD_X1_CONTROLr_reg, 1);

    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_CTLr(pc, PMD_X1_PMD_X1_CONTROLr_reg));
  }


  PMD_X4_CTLr_CLR(PMD_X4_PMD_X4_CONTROLr_reg);
  PHYMOD_IF_ERR_RETURN(READ_PMD_X4_CTLr(pc, &PMD_X4_PMD_X4_CONTROLr_reg));
  PMD_X4_CTLr_LN_RX_H_PWRDNf_SET(PMD_X4_PMD_X4_CONTROLr_reg, 0);
  PMD_X4_CTLr_LN_TX_H_PWRDNf_SET(PMD_X4_PMD_X4_CONTROLr_reg, 0);

  PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_CTLr(pc, PMD_X4_PMD_X4_CONTROLr_reg));

  PMD_X4_CTLr_CLR(PMD_X4_PMD_X4_CONTROLr_reg);
  PHYMOD_IF_ERR_RETURN(READ_PMD_X4_CTLr(pc, &PMD_X4_PMD_X4_CONTROLr_reg));
  PMD_X4_CTLr_LN_H_RSTBf_SET(PMD_X4_PMD_X4_CONTROLr_reg, 1);
  PMD_X4_CTLr_LN_DP_H_RSTBf_SET(PMD_X4_PMD_X4_CONTROLr_reg, 1);

  PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_CTLr(pc, PMD_X4_PMD_X4_CONTROLr_reg));

  return PHYMOD_E_NONE;
}
#endif

int plp_aperta_tbhmod_get_pcs_latched_link_status(PHYMOD_ST* pc, uint32_t *link)
{
  RX_X4_PCS_LATCH_STS1r_t latched_sts ;
  int                     latched_val;

  RX_X4_PCS_LATCH_STS1r_CLR(latched_sts) ;

  PHYMOD_IF_ERR_RETURN (READ_RX_X4_PCS_LATCH_STS1r(pc, &latched_sts));
  latched_val = RX_X4_PCS_LATCH_STS1r_LINK_STATUS_LLf_GET(latched_sts) ;
  if(latched_val) {
      *link = 0 ;
  } else {
      *link = RX_X4_PCS_LATCH_STS1r_PCS_LINK_STATUS_LIVEf_GET(latched_sts);
  }

  return PHYMOD_E_NONE;
}




/*!
@brief plp_aperta_tbhmod_poll_for_sc_done , polls for the sc_done bit ,
@returns The value PHYMOD_E_NONE upon successful completion.
@details
Reads the SC_X4_COntrol_status reg and polls it to see if sc_done bit is set
Sets the sc_done flag=1/0, if speed control is done, after resolving to the correct speed
*/
int plp_aperta_tbhmod_poll_for_sc_done(PHYMOD_ST* pc, int mapped_speed)
{
  int done;
  int spd_match;
  int sc_done;
  int cnt;
  uint16_t data;

  uint16_t sw_spd_chg_chk;

  SC_X4_STSr_t SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg;
  SC_X4_RSLVD_SPDr_t SC_X4_FINAL_CONFIG_STATUS_RESOLVED_SPEEDr_reg;

  APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc);
  done=0;
  cnt=0;
  sc_done = 0;
  spd_match = 0;

  SC_X4_STSr_CLR(SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg);
  while (cnt <=2000) {
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_STSr(pc, &SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg));
    cnt = cnt + 1;
         sw_spd_chg_chk = SC_X4_STSr_SW_SPEED_CHANGE_DONEf_GET(SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg);

    if(sw_spd_chg_chk) {
      sc_done = 1;
      break;
    } else {
      if(cnt == 2000) {
        sc_done = 0;
        break;
      }
    }
  }

  cnt=0;
  while (cnt <=400) {
    SC_X4_RSLVD_SPDr_CLR(SC_X4_FINAL_CONFIG_STATUS_RESOLVED_SPEEDr_reg);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_RSLVD_SPDr(pc, &SC_X4_FINAL_CONFIG_STATUS_RESOLVED_SPEEDr_reg));
    data = SC_X4_RSLVD_SPDr_SPEEDf_GET(SC_X4_FINAL_CONFIG_STATUS_RESOLVED_SPEEDr_reg);
#ifdef _DV_TB_
    if(pc->verbosity)
     PHYMOD_DEBUG_ERROR(("Resolved speed is %x :: need %x :: uint :: %d :: lane :: %d\n", data, mapped_speed, pc->unit, pc->this_lane));
#endif
    cnt = cnt + 1;
    if(data == mapped_speed) {
      spd_match = 1;
      break;
    } else {
      if(cnt == 400) {
        spd_match = 0;
        break;
      }
    }
  }
  if(sc_done && spd_match) {
    done = 1;
  } else {
    done = 0;
  }
  return done;
}

/**
@brief   update the port mode
@param   pc handle to current TSCF context (#PHYMOD_ST)
@returns The value PHYMOD_E_NONE upon successful completion
*/
int plp_aperta_tbhmod_update_port_mode( PHYMOD_ST *pc)
{
  MAIN0_SETUPr_t mode_reg;
  PHYMOD_ST phy_copy;
  int port_mode_sel, port_mode_sel_reg, mpp_index = 0;
  uint32_t single_port_mode;
  int first_couple_mode = 0, second_couple_mode = 0;
  port_mode_sel = 0 ;
  single_port_mode = 0 ;

  PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));

  if (pc->access.lane_mask == 0xFF) {
      phy_copy.access.lane_mask = 0xF;
      PHYMOD_IF_ERR_RETURN(
          READ_MAIN0_SETUPr(&phy_copy, &mode_reg));
      MAIN0_SETUPr_PORT_MODE_SELf_SET(mode_reg, 0);
      PHYMOD_IF_ERR_RETURN(
          MODIFY_MAIN0_SETUPr(&phy_copy, mode_reg));
      phy_copy.access.lane_mask = 0xF0;
      PHYMOD_IF_ERR_RETURN(
          MODIFY_MAIN0_SETUPr(&phy_copy, mode_reg));

  }
  /*first check if 8 lane port */
  if(pc->access.lane_mask == 0xff) {
    single_port_mode = 1;
    mpp_index = 0;
  } else if (pc->access.lane_mask & 0xf) {
    mpp_index = 0;
  } else {
    mpp_index = 1;
  }

  /* next set the MPP index properly */
  if (mpp_index) {
    phy_copy.access.lane_mask = 0x10;
  } else {
    phy_copy.access.lane_mask = 0x1;
  }
  PHYMOD_IF_ERR_RETURN(
      READ_MAIN0_SETUPr(&phy_copy, &mode_reg));
  port_mode_sel_reg = MAIN0_SETUPr_PORT_MODE_SELf_GET(mode_reg);
  if (!single_port_mode) {
      if ((pc->access.lane_mask == 0xf) || (pc->access.lane_mask == 0xf0)) {
        port_mode_sel = cl82_port_mode_SINGLE_PORT;
      } else {
        first_couple_mode = ((port_mode_sel_reg == cl82_port_mode_TRI_2_PORT) ||
                             (port_mode_sel_reg == cl82_port_mode_DUAL_PORT) ||
                             (port_mode_sel_reg == cl82_port_mode_SINGLE_PORT));

        second_couple_mode = ((port_mode_sel_reg == cl82_port_mode_TRI_1_PORT) ||
                              (port_mode_sel_reg == cl82_port_mode_DUAL_PORT) ||
                              (port_mode_sel_reg == cl82_port_mode_SINGLE_PORT));
        /* based on the lane mask, figue out which couple mode */
        switch (pc->access.lane_mask) {
        case 0x1:
        case 0x2:
        case 0x10:
        case 0x20:
          first_couple_mode = 0;
          break;
        case 0x4:
        case 0x8:
        case 0x40:
        case 0x80:
          second_couple_mode = 0;
          break;
        case 0x3:
        case 0x30:
          first_couple_mode = 1;
          break;
        case 0xc:
        case 0xc0:
          second_couple_mode = 1;
          break;
        default:
          return PHYMOD_E_PARAM;
        }

        if (first_couple_mode) {
          port_mode_sel =(second_couple_mode)? cl82_port_mode_DUAL_PORT: cl82_port_mode_TRI_2_PORT;
        } else if (second_couple_mode) {
          port_mode_sel = cl82_port_mode_TRI_1_PORT;
        } else {
          port_mode_sel = cl82_port_mode_QUAD_PORT ;
        }
      }
  }
  if (single_port_mode) {
    port_mode_sel = cl82_port_mode_SINGLE_OCTAL_PORT;
  }
  MAIN0_SETUPr_PORT_MODE_SELf_SET(mode_reg, port_mode_sel);
  PHYMOD_IF_ERR_RETURN(
      MODIFY_MAIN0_SETUPr(&phy_copy, mode_reg));
  if (pc->access.lane_mask == 0xFF) {
      phy_copy.access.lane_mask = 0xF0;
      PHYMOD_IF_ERR_RETURN(
          MODIFY_MAIN0_SETUPr(&phy_copy, mode_reg));
  }

  return PHYMOD_E_NONE ;
}


/* ------------------------------------------------------------------------- */

/* TSCBH NEW TIER1s*/
/* to update port_mode_select value.  If the update warrants a pll reset,
   then return accData=1, otherwise return accData=0.  The update shall
   support flex port technology. Called forced speed modes */

int plp_aperta_tbhmod_update_port_mode_select(PHYMOD_ST* pc, aperta_tbhmod_port_type_t port_type)
{
  MAIN0_SETUPr_t   MAIN0_SETUPr_reg;
#ifdef _DV_TB_
  pc.port_type = port_type;
  pc.accData   = 0;
  if((dv_st->tsc_touched_mpp0 == 1 && pc.this_lane/4==0) || ( pc.this_lane/4==1 && dv_st->tsc_touched_mpp1==1)) {
    return PHYMOD_E_NONE;
  }

#endif


  if((port_type ==APERTA_TBHMOD_MULTI_PORT) ||(port_type ==APERTA_TBHMOD_DXGXS)||
     (port_type ==APERTA_TBHMOD_SINGLE_PORT)||(port_type ==APERTA_TBHMOD_TRI1_PORT)||
     (port_type ==APERTA_TBHMOD_TRI2_PORT)  ||(port_type == APERTA_TBHMOD_SINGLE_OCTAL_PORT)){
  } else {
    PHYMOD_DEBUG_ERROR(("%-22s: ERROR port_type=%0d undefined\n", __func__, port_type));
    return PHYMOD_E_FAIL;
  }


 /* Leave port mode to quad in an_en */
  MAIN0_SETUPr_CLR(MAIN0_SETUPr_reg);
  PHYMOD_IF_ERR_RETURN (READ_MAIN0_SETUPr(pc, &MAIN0_SETUPr_reg));
  MAIN0_SETUPr_PORT_MODE_SELf_SET(MAIN0_SETUPr_reg, port_type);
  PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_SETUPr(pc, MAIN0_SETUPr_reg));

  MAIN0_SETUPr_CLR(MAIN0_SETUPr_reg);

  return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_low_latency_mode_en(PHYMOD_ST* pc,int low_latency_mode_en)
{

   MAIN0_SETUPr_t   MAIN0_SETUPr_reg;

   MAIN0_SETUPr_CLR(MAIN0_SETUPr_reg);
   PHYMOD_IF_ERR_RETURN (READ_MAIN0_SETUPr(pc, &MAIN0_SETUPr_reg));
   MAIN0_SETUPr_RESOLVED_PORT_MODE_CONTROLf_SET(MAIN0_SETUPr_reg,low_latency_mode_en);
   PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_SETUPr(pc, MAIN0_SETUPr_reg));
   return PHYMOD_E_NONE;
}


#ifdef _DV_TB_ /*THIS is required for SDK also*/
/*!
@brief plp_aperta_tbhmod_set_sc_speed , sets the required speed ,
@returns The value PHYMOD_E_NONE upon successful completion.
@details
After the resets, speed_ctrl regs are programmed(as per Ankits new init seq), fills up the lookup table by
Updating the expected value
*/
/*int plp_aperta_tbhmod_set_sc_speed(aperta_tbhmod_st* pc, int set_sw_speed_change) */ /* SET_SC_SPEED */
int plp_aperta_tbhmod_set_sc_speed(PHYMOD_ST* pc, aperta_tbhmod_spd_intfc_type_t spd_intf, int set_sw_speed_change)  /* SET_SC_SPEED */
{
  int mapped_speed;
  SC_X4_CTLr_t SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg;


  /* write 0 to the speed change */
  SC_X4_CTLr_CLR(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg);

  PHYMOD_IF_ERR_RETURN(READ_SC_X4_CTLr(pc, &SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg));

  if(set_sw_speed_change) {
    SC_X4_CTLr_SW_SPEED_CHANGEf_SET(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg, 0);
    PHYMOD_IF_ERR_RETURN(WRITE_SC_X4_CTLr(pc, SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg));
  }

   /* Set speed and write 1 */
  SC_X4_CTLr_CLR(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg);
  SC_X4_CTLr_SW_SPEED_CHANGEf_SET(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg, set_sw_speed_change);
  plp_aperta_tbhmod_get_mapped_speed(spd_intf, &mapped_speed);
  SC_X4_CTLr_SW_SPEED_IDf_SET(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg, mapped_speed);
  PHYMOD_IF_ERR_RETURN(WRITE_SC_X4_CTLr(pc, SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg));

  /*  Update the speed */
  pc.speed= mapped_speed;


  return PHYMOD_E_NONE;
}
#endif

#ifdef _SDK_APERTA_TBHMOD_
int plp_aperta_tbhmod_set_sc_speed(PHYMOD_ST* pc, int mapped_speed, int set_sw_speed_change)  /* SET_SC_SPEED */
{
  SC_X4_CTLr_t SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg;


  /* write 0 to the speed change */
  SC_X4_CTLr_CLR(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg);

  PHYMOD_IF_ERR_RETURN(READ_SC_X4_CTLr(pc, &SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg));

  if(set_sw_speed_change) {
    SC_X4_CTLr_SW_SPEED_CHANGEf_SET(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg, 0);
    PHYMOD_IF_ERR_RETURN(WRITE_SC_X4_CTLr(pc, SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg));
  }

   /* Set speed and write 1 */
  SC_X4_CTLr_CLR(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg);
  SC_X4_CTLr_SW_SPEED_CHANGEf_SET(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg, set_sw_speed_change);
  SC_X4_CTLr_SW_SPEED_IDf_SET(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg, mapped_speed);
  PHYMOD_IF_ERR_RETURN(WRITE_SC_X4_CTLr(pc, SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg));

  return PHYMOD_E_NONE;
}
#endif

#ifdef DV_AN_SUPPORT
int aperta_tbhmod_pcs_set_sw_an_speed(PHYMOD_ST* pc, int speed)  /* SET_SW_AN_SPEED */
{
  AN_X4_LD_CTLr_t AN_X4_LD_CTLr_reg;


  AN_X4_LD_CTLr_CLR(AN_X4_LD_CTLr_reg);

  PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_CTLr(pc, &AN_X4_LD_CTLr_reg));


  AN_X4_LD_CTLr_SW_AN_SPEED_IDf_SET(AN_X4_LD_CTLr_reg, speed);
  AN_X4_LD_CTLr_AN_HCD_RES_DISABLEf_SET(AN_X4_LD_CTLr_reg, 1);
  AN_X4_LD_CTLr_AN_TYPE_SWf_SET(AN_X4_LD_CTLr_reg, 1);
  PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_LD_CTLr(pc, AN_X4_LD_CTLr_reg));

  /*  Update the speed */
  pc.speed= speed;


  return PHYMOD_E_NONE;
}
#endif

/**
@brief   PMD per lane reset
@param   pc handle to current TSCBH context (#aperta_tbhmod_st)
@returns The value PHYMOD_E_NONE upon successful completion
@details Per lane PMD ln_rst and ln_dp_rst by writing to PMD_X4_CONTROL in pcs space
*/
int plp_aperta_tbhmod_pmd_x4_reset(PHYMOD_ST* pc)              /* PMD_X4_RESET */
{
  PMD_X4_CTLr_t reg_pmd_x4_ctrl;


  PMD_X4_CTLr_CLR(reg_pmd_x4_ctrl);
  PHYMOD_IF_ERR_RETURN (READ_PMD_X4_CTLr(pc, &reg_pmd_x4_ctrl));
  PMD_X4_CTLr_LN_H_RSTBf_SET(reg_pmd_x4_ctrl,1);
  PMD_X4_CTLr_LN_DP_H_RSTBf_SET(reg_pmd_x4_ctrl,1);
  PHYMOD_IF_ERR_RETURN (MODIFY_PMD_X4_CTLr(pc, reg_pmd_x4_ctrl));

  return PHYMOD_E_NONE;
}


/**
@brief   RSFEC latency Bit selection
@param   pc handle to current TSCBH context (#aperta_tbhmod_st)
@returns The value PHYMOD_E_NONE upon successful completion
@details assert/deassert the cw_latency bit to support 544 and 528 fecs in same MPP
*/
int plp_aperta_tbhmod_rsfec_cw_type(PHYMOD_ST* pc, int en, int fec_bypass_correction)              /* PMD_X4_RESET */
{
    PHYMOD_ST phy_copy;
    RX_X1_RS_FEC_CFGr_t reg_rsfec_cfg;
    uint16_t current_cw_type = 0;
    uint32_t i, lane_offset, port_en, print_en = 0;

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));

    RX_X1_RS_FEC_CFGr_CLR(reg_rsfec_cfg);
    PHYMOD_IF_ERR_RETURN(READ_RX_X1_RS_FEC_CFGr(pc, &reg_rsfec_cfg));
    current_cw_type = RX_X1_RS_FEC_CFGr_RS_FEC_CW_TYPEf_GET(reg_rsfec_cfg);

    /* Check port status on a MPP */
    for (i = 0; i< 4; i++) {
        lane_offset = i + (pc->access.lane_mask & 0xf ? 0 : 4);
        phy_copy.access.lane_mask = 0x1 << lane_offset;
        PHYMOD_IF_ERR_RETURN(plp_aperta_tbhmod_port_enable_get(&phy_copy, &port_en));
        if (port_en) {
            print_en = 1;
        }
    }

    /* print out the warning message if CW type changes */
    if ((current_cw_type != en) && print_en) {
        PHYMOD_DEBUG_ERROR(("WARNING: FEC config changes will require all the ports of same MPP disabled \n"));
    }

    RX_X1_RS_FEC_CFGr_RS_FEC_CW_TYPEf_SET(reg_rsfec_cfg, en);
    if (fec_bypass_correction == 1) {
        RX_X1_RS_FEC_CFGr_RS_FEC_CORRECTION_ENABLEf_SET(reg_rsfec_cfg, ~fec_bypass_correction);
    }
    PHYMOD_IF_ERR_RETURN (MODIFY_RX_X1_RS_FEC_CFGr(pc, reg_rsfec_cfg));

    return PHYMOD_E_NONE;
}


/**
@brief   OSTS Pipeline enable Bit selection
@param   pc handle to current TSCBH context (#aperta_tbhmod_st)
@returns The value PHYMOD_E_NONE upon successful completion
@details enable/disable One Step pipeline
*/

int plp_aperta_tbhmod_osts_pipeline(PHYMOD_ST* pc,uint32_t en)
{
 TX_X4_TX_TS_CTLr_t    reg_tx_x4_ts_ctlr;

  TX_X4_TX_TS_CTLr_CLR(reg_tx_x4_ts_ctlr);
  PHYMOD_IF_ERR_RETURN (READ_TX_X4_TX_TS_CTLr(pc, &reg_tx_x4_ts_ctlr));
  TX_X4_TX_TS_CTLr_OSTS_PIPELINE_ENABLEf_SET(reg_tx_x4_ts_ctlr, en);

  PHYMOD_IF_ERR_RETURN (MODIFY_TX_X4_TX_TS_CTLr(pc, reg_tx_x4_ts_ctlr));
  return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_osts_pipeline_get(PHYMOD_ST* pc, uint32_t* en)
{
  TX_X4_TX_TS_CTLr_t reg_tx_x4_ts_ctlr;

  TX_X4_TX_TS_CTLr_CLR(reg_tx_x4_ts_ctlr);
  PHYMOD_IF_ERR_RETURN(READ_TX_X4_TX_TS_CTLr(pc, &reg_tx_x4_ts_ctlr));

  *en = TX_X4_TX_TS_CTLr_OSTS_PIPELINE_ENABLEf_GET(reg_tx_x4_ts_ctlr);

  return PHYMOD_E_NONE;
}

/*!
@brief tx lane reset and enable of any particular lane
@param  pc handle to current TSCBH context (#aperta_tbhmod_st)
@param  en controls enabling properties of  lane  control
@param  dis controls what property to control enum #aperta_tbhmod_tx_disable_enum_t
@returns The value PHYMOD_E_NONE upon successful completion.
@details
This function is used to reset tx lane and enable/disable tx lane of any
transmit lane.  Set bit 0 of per_lane_control to enable the TX_LANE
(1) or disable the TX lane (0).
When diable TX lane, two types of operations are inovked independently.
If per_lane_control bit [7:4] = 1, only traffic is disabled.
      (APERTA_TBHMOD_TX_LANE_TRAFFIC =0x10)
If bit [7:4] = 2, only reset function is invoked.
      (APERTA_TBHMOD_TX_LANE_RESET = 0x20)

This function reads/modifies the following registers:

\li rstb_tx_lane  :   0xc113[1]
\li enable_tx_lane:   0xc113[0]
*/

int plp_aperta_tbhmod_tx_lane_control(PHYMOD_ST* pc, int en, aperta_tbhmod_tx_disable_enum_t dis)         /* TX_LANE_CONTROL */
{
  TX_X4_MISCr_t TX_X4_CONTROL0_MISCr_reg;

 /* APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc); */

  #ifdef _DV_TB_

   printf("APERTA_TBHMOD_TIER1_TX_LANE_CONTROL: Enabling TXLane and removing Reset for Lane %0d unit %0d\n",pc.this_lane,pc.unit);
  #endif

   if (en) {
    TX_X4_MISCr_CLR(TX_X4_CONTROL0_MISCr_reg);
    PHYMOD_IF_ERR_RETURN(READ_TX_X4_MISCr(pc, &TX_X4_CONTROL0_MISCr_reg));
    TX_X4_MISCr_RSTB_TX_LANEf_SET(TX_X4_CONTROL0_MISCr_reg, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));

    TX_X4_MISCr_CLR(TX_X4_CONTROL0_MISCr_reg);
    PHYMOD_IF_ERR_RETURN(READ_TX_X4_MISCr(pc, &TX_X4_CONTROL0_MISCr_reg));
    TX_X4_MISCr_ENABLE_TX_LANEf_SET(TX_X4_CONTROL0_MISCr_reg, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));
  } else {
    TX_X4_MISCr_CLR(TX_X4_CONTROL0_MISCr_reg);
    PHYMOD_IF_ERR_RETURN(READ_TX_X4_MISCr(pc, &TX_X4_CONTROL0_MISCr_reg));
    if(dis == APERTA_TBHMOD_TX_LANE_TRAFFIC_DISABLE ){
      TX_X4_MISCr_ENABLE_TX_LANEf_SET(TX_X4_CONTROL0_MISCr_reg, 0);
    } else if (dis == APERTA_TBHMOD_TX_LANE_RESET) {
      TX_X4_MISCr_RSTB_TX_LANEf_SET(TX_X4_CONTROL0_MISCr_reg, 0);
    } else {
      TX_X4_MISCr_RSTB_TX_LANEf_SET(TX_X4_CONTROL0_MISCr_reg, 0);
      TX_X4_MISCr_ENABLE_TX_LANEf_SET(TX_X4_CONTROL0_MISCr_reg, 0);
    }
    PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_MISCr(pc, TX_X4_CONTROL0_MISCr_reg));
  }


  return PHYMOD_E_NONE;
}


/*!
@brief rx lane reset and enable of any particular lane
@param  pc handle to current TSCBH context (#aperta_tbhmod_st)
@param  accData TBD
@param  per_lane_control TBD
@returns The value PHYMOD_E_NONE upon successful completion.
@details
This function enables/disables rx lane (RSTB_LANE) or read back control bit for
that based on per_lane_control being 1 or 0. If per_lane_control is 0xa, only
read back RSTB_LANE.
\li select encodeMode
This function reads/modifies the following registers:
\li rstb_rx_lane  :   0xc137[0]
*/

int plp_aperta_tbhmod_rx_lane_control(PHYMOD_ST* pc, int per_lane_control)         /* RX_LANE_CONTROL */
{
  int cntl;
  RX_X4_PMA_CTL0r_t RX_X4_CONTROL0_PMA_CONTROL_0r_reg;

  cntl = per_lane_control;

  #ifdef _DV_TB_
  printf("APERTA_TBHMOD_TIER1_RX_LANE_CONTROL:Enabling Rx Lane and bringing outofreset for  lane %0d unit %0d\n",pc.this_lane,pc.unit);
  #endif
  RX_X4_PMA_CTL0r_CLR(RX_X4_CONTROL0_PMA_CONTROL_0r_reg);
  PHYMOD_IF_ERR_RETURN(READ_RX_X4_PMA_CTL0r(pc, &RX_X4_CONTROL0_PMA_CONTROL_0r_reg));

  if (cntl) {
    RX_X4_PMA_CTL0r_RSTB_LANEf_SET(RX_X4_CONTROL0_PMA_CONTROL_0r_reg, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_PMA_CTL0r(pc, RX_X4_CONTROL0_PMA_CONTROL_0r_reg));
  } else {
     /* bit set to 0 for disabling RXP */
    RX_X4_PMA_CTL0r_RSTB_LANEf_SET(RX_X4_CONTROL0_PMA_CONTROL_0r_reg, 0);
    PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_PMA_CTL0r(pc, RX_X4_CONTROL0_PMA_CONTROL_0r_reg));
  }
  return PHYMOD_E_NONE;
}


/*!
@brief Power down transmitter or receiver per lane basis.
@param  pc handle to current TSCBH context (a.k.a #aperta_tbhmod_st)
@param tx control for power of TX path
@param rx control for power of RX path
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details
Indi
*/
int plp_aperta_tbhmod_power_control(PHYMOD_ST* pc, int tx, int rx)
{
  PMD_X4_CTLr_t PMD_X4_PMD_X4_CONTROLr_reg;

    /*APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc);*/
  PMD_X4_CTLr_CLR(PMD_X4_PMD_X4_CONTROLr_reg);

  PHYMOD_IF_ERR_RETURN(READ_PMD_X4_CTLr(pc, &PMD_X4_PMD_X4_CONTROLr_reg));
  PMD_X4_CTLr_LN_RX_H_PWRDNf_SET(PMD_X4_PMD_X4_CONTROLr_reg, rx);
  PMD_X4_CTLr_LN_TX_H_PWRDNf_SET(PMD_X4_PMD_X4_CONTROLr_reg, tx);
  PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_CTLr(pc, PMD_X4_PMD_X4_CONTROLr_reg));
  return PHYMOD_E_NONE;
}




int plp_aperta_tbhmod_get_sw_mgmt_glb_int(PHYMOD_ST* pc, int *port_int)
{
    AN_X1_GLB_INTr_t  AN_X1_GLB_INTr_reg;

     AN_X1_GLB_INTr_CLR(AN_X1_GLB_INTr_reg);
    PHYMOD_IF_ERR_RETURN(READ_AN_X1_GLB_INTr(pc, &AN_X1_GLB_INTr_reg));
    *port_int = AN_X1_GLB_INTr_GET(AN_X1_GLB_INTr_reg);


    return PHYMOD_E_NONE;
}





/*!
 @brief Rx laneswap per Mpp.
@param  unit number for instance lane number for decide which lane
@param rx_lane_swap value tells phy to log lanes mapping
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Rx lane swap */
int plp_aperta_tbhmod_pcs_rx_lane_swap(PHYMOD_ST* pc, int rx_lane_swap)
{
    uint8_t rx_lane_map_physical[8], lane;
    RX_X1_RX_LN_SWPr_t    RX_X1_RX_LN_SWPr_reg;

    /*first we need to translate logical ln based on physical lane based lane ma-*/
    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE; lane++){
        rx_lane_map_physical[((rx_lane_swap >> (lane*4)) & 0xf)] = lane;
    }

    /*first for physcial lane 0 through 3, use mmp0 */
    RX_X1_RX_LN_SWPr_CLR(RX_X1_RX_LN_SWPr_reg);
    pc->access.lane_mask = 0x1 << 0;
    RX_X1_RX_LN_SWPr_PHYSICAL3_TO_LOGICAL_SELf_SET(RX_X1_RX_LN_SWPr_reg,(rx_lane_map_physical[3] & 0x7));
    RX_X1_RX_LN_SWPr_PHYSICAL2_TO_LOGICAL_SELf_SET(RX_X1_RX_LN_SWPr_reg,(rx_lane_map_physical[2] & 0x7));
    RX_X1_RX_LN_SWPr_PHYSICAL1_TO_LOGICAL_SELf_SET(RX_X1_RX_LN_SWPr_reg,(rx_lane_map_physical[1] & 0x7));
    RX_X1_RX_LN_SWPr_PHYSICAL0_TO_LOGICAL_SELf_SET(RX_X1_RX_LN_SWPr_reg,(rx_lane_map_physical[0] & 0x7));
    PHYMOD_IF_ERR_RETURN(MODIFY_RX_X1_RX_LN_SWPr(pc, RX_X1_RX_LN_SWPr_reg));

    RX_X1_RX_LN_SWPr_CLR(RX_X1_RX_LN_SWPr_reg);
    pc->access.lane_mask = 0x1 << 4;
    RX_X1_RX_LN_SWPr_PHYSICAL3_TO_LOGICAL_SELf_SET(RX_X1_RX_LN_SWPr_reg,(rx_lane_map_physical[7] & 0x7));
    RX_X1_RX_LN_SWPr_PHYSICAL2_TO_LOGICAL_SELf_SET(RX_X1_RX_LN_SWPr_reg,(rx_lane_map_physical[6] & 0x7));
    RX_X1_RX_LN_SWPr_PHYSICAL1_TO_LOGICAL_SELf_SET(RX_X1_RX_LN_SWPr_reg,(rx_lane_map_physical[5] & 0x7));
    RX_X1_RX_LN_SWPr_PHYSICAL0_TO_LOGICAL_SELf_SET(RX_X1_RX_LN_SWPr_reg,(rx_lane_map_physical[4] & 0x7));

    PHYMOD_IF_ERR_RETURN(MODIFY_RX_X1_RX_LN_SWPr(pc, RX_X1_RX_LN_SWPr_reg));

    return PHYMOD_E_NONE;
}


 /*!
 @brief Tx laneswap per Mpp.
@param  unit number for instance lane number for decide which lane
@param tx_lane_swap value tells phy to log lanes mapping
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Tx lane swap */
int plp_aperta_tbhmod_pcs_tx_lane_swap(PHYMOD_ST* pc, int tx_lane_swap)
{
    uint8_t tx_lane_map_physical[8], lane;
    TX_X1_TX_LN_SWPr_t TX_X1_TX_LN_SWPr_reg;

    /*first we need to translate logical ln based on physical lane based lane ma-*/
    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE; lane++){
        tx_lane_map_physical[((tx_lane_swap >> (lane*4)) & 0xf)] = lane;
    }

    /* next need to adjust the MPP1 logical lane offset */
    for (lane = 0; lane < APERTA_TSCBH_NOF_LANES_IN_CORE / 2; lane++){
        if (tx_lane_map_physical[4 + lane] < 4) {
            tx_lane_map_physical[4 + lane] += 4;
        } else {
            tx_lane_map_physical[4 + lane] -= 4;
        }
    }
    /*for the physical 0 through 3 using MPP0 */
    TX_X1_TX_LN_SWPr_CLR(TX_X1_TX_LN_SWPr_reg);
    pc->access.lane_mask = 0x1 << 0;
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL0_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[0] & 0x7));
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL1_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[1] & 0x7));
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL2_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[2] & 0x7));
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL3_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[3] & 0x7));
    PHYMOD_IF_ERR_RETURN(MODIFY_TX_X1_TX_LN_SWPr(pc, TX_X1_TX_LN_SWPr_reg));


    /*for the physical 4 through 7 using MPP1 */
    TX_X1_TX_LN_SWPr_CLR(TX_X1_TX_LN_SWPr_reg);
    pc->access.lane_mask = 0x1 << 4;
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL0_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[4] & 0x7));
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL1_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[5] & 0x7));
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL2_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[6] & 0x7));
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL3_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[7] & 0x7));
    PHYMOD_IF_ERR_RETURN(MODIFY_TX_X1_TX_LN_SWPr(pc, TX_X1_TX_LN_SWPr_reg));

    return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_port_start_lane_get(PHYMOD_ST *pc, int *port_starting_lane, int *port_num_lane)
{
    int start_lane, num_of_lane;
    PHYMOD_ST pc_copy;

    PHYMOD_IF_ERR_RETURN
      (plp_aperta_phymod_util_lane_config_get(&pc->access, &start_lane, &num_of_lane));

    if (num_of_lane > 1) {
        *port_starting_lane = start_lane;
        *port_num_lane = num_of_lane;
    } else {
        MAIN0_SETUPr_t mode_reg;
        int port_mode_sel_reg;

        /* need to figure out the port starting lane basedon the port mode */
        PHYMOD_MEMCPY(&pc_copy, pc, sizeof(pc_copy));
        /* first read MPP0 port mode */
        pc_copy.access.lane_mask = 0x1;
        PHYMOD_IF_ERR_RETURN(READ_MAIN0_SETUPr(&pc_copy, &mode_reg));
        port_mode_sel_reg = MAIN0_SETUPr_PORT_MODE_SELf_GET(mode_reg);

        /*first check if 8 lane port mode */
        if (port_mode_sel_reg == cl82_port_mode_SINGLE_OCTAL_PORT) {
            *port_starting_lane = 0;
            *port_num_lane = 8;
        } else {
            /* first check which MPP  based on the starting lane */
            if (start_lane  < 4) {
                switch (port_mode_sel_reg) {
                    case  cl82_port_mode_SINGLE_PORT:
                        *port_starting_lane = 0;
                        *port_num_lane = 4;
                        break;
                    case  cl82_port_mode_DUAL_PORT:
                        if (start_lane < 2) {
                            *port_starting_lane = 0;
                            *port_num_lane = 2;
                        } else {
                            *port_starting_lane = 2;
                            *port_num_lane = 2;
                        }
                        break;
                    case  cl82_port_mode_TRI_1_PORT:
                        if (start_lane < 2) {
                            *port_starting_lane = start_lane;
                            *port_num_lane = 1;
                        } else {
                            *port_starting_lane = 2;
                            *port_num_lane = 2;
                        }
                        break;
                    case  cl82_port_mode_TRI_2_PORT:
                        if (start_lane < 2) {
                            *port_starting_lane = 0;
                            *port_num_lane = 2;
                        } else {
                            *port_starting_lane = start_lane;
                            *port_num_lane = 1;
                        }
                        break;
                    default:
                        *port_starting_lane = start_lane;
                        *port_num_lane = num_of_lane;
                }
            } else {
                /* first read MPP1 port mode */
                pc_copy.access.lane_mask = 0x10;
                PHYMOD_IF_ERR_RETURN(READ_MAIN0_SETUPr(&pc_copy, &mode_reg));
                port_mode_sel_reg = MAIN0_SETUPr_PORT_MODE_SELf_GET(mode_reg);
                switch (port_mode_sel_reg) {
                    case  cl82_port_mode_SINGLE_PORT:
                        *port_starting_lane = 4;
                        *port_num_lane = 4;
                        break;
                    case  cl82_port_mode_DUAL_PORT:
                        if (start_lane < 6) {
                            *port_starting_lane = 4;
                            *port_num_lane = 2;
                        } else {
                            *port_starting_lane = 6;
                            *port_num_lane = 2;
                        }
                        break;
                    case  cl82_port_mode_TRI_1_PORT:
                        if (start_lane < 6) {
                            *port_starting_lane = start_lane;
                            *port_num_lane = 1;
                        } else {
                            *port_starting_lane = 6;
                            *port_num_lane = 2;
                        }
                        break;
                    case  cl82_port_mode_TRI_2_PORT:
                        if (start_lane < 6) {
                            *port_starting_lane = 4;
                            *port_num_lane = 2;
                        } else {
                            *port_starting_lane = start_lane;
                            *port_num_lane = 1;
                        }
                        break;
                    default:
                        *port_starting_lane = start_lane;
                        *port_num_lane = num_of_lane;
                }
            }
        }
    }

    return PHYMOD_E_NONE;
}

/*!
 *  @brief rx_scrambleidle_en per port.
@param  unit number for instance lane number for decide which lane
@param en tells enabling/disabling r_test_mode AKA scrmable_idle_en
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details  Rx scramble Idle Enable */
int plp_aperta_tbhmod_pcs_rx_scramble_idle_en(PHYMOD_ST* pc, int en)
{

  RX_X4_DEC_CTL0r_t RX_X4_CONTROL0_DECODE_CONTROL_0r_reg;

  RX_X4_DEC_CTL0r_CLR(RX_X4_CONTROL0_DECODE_CONTROL_0r_reg);
  PHYMOD_IF_ERR_RETURN(READ_RX_X4_DEC_CTL0r(pc, &RX_X4_CONTROL0_DECODE_CONTROL_0r_reg));
  RX_X4_DEC_CTL0r_R_TEST_MODE_CFGf_SET(RX_X4_CONTROL0_DECODE_CONTROL_0r_reg, en);
  PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_DEC_CTL0r(pc, RX_X4_CONTROL0_DECODE_CONTROL_0r_reg));

  return PHYMOD_E_NONE;
}

/*!
 *  @brief plp_aperta_tbhmod_pcs_rx_ts_en per port.
@param unit number for instance lane number for decide which lane
@param en tells enabling/disabling r_test_mode AKA scrmable_idle_en
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Enables ts in rx path */
int plp_aperta_tbhmod_pcs_rx_ts_en(PHYMOD_ST* pc, int en, int sfd)
{
  RX_X4_RX_TS_CTLr_t RX_X4_RX_TS_CTLr_reg;


  RX_X4_RX_TS_CTLr_CLR(RX_X4_RX_TS_CTLr_reg);

  PHYMOD_IF_ERR_RETURN(READ_RX_X4_RX_TS_CTLr(pc, &RX_X4_RX_TS_CTLr_reg));
  RX_X4_RX_TS_CTLr_RECORD_DESKEW_TS_INFOf_SET(RX_X4_RX_TS_CTLr_reg, 1);
  RX_X4_RX_TS_CTLr_RX_SFD_TS_ENf_SET(RX_X4_RX_TS_CTLr_reg, sfd);
  PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_RX_TS_CTLr(pc, RX_X4_RX_TS_CTLr_reg));

  return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_pcs_rx_ts_en_get(PHYMOD_ST* pc, uint32_t* en)
{
  RX_X4_RX_TS_CTLr_t RX_X4_RX_TS_CTLr_reg;

  RX_X4_RX_TS_CTLr_CLR(RX_X4_RX_TS_CTLr_reg);
  PHYMOD_IF_ERR_RETURN(READ_RX_X4_RX_TS_CTLr(pc, &RX_X4_RX_TS_CTLr_reg));
  *en = RX_X4_RX_TS_CTLr_TS_UPDATE_ENABLEf_GET(RX_X4_RX_TS_CTLr_reg);

  return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_pcs_1588_ts_offset_set(PHYMOD_ST *pc, uint32_t ns_offset, uint32_t sub_ns_offset)
{
    PMD_X1_PM_TMR_OFFSr_t PMD_X1_PM_TMR_OFFSr_reg;
    PHYMOD_ST phy_copy;

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));

    PMD_X1_PM_TMR_OFFSr_CLR(PMD_X1_PM_TMR_OFFSr_reg);
    PMD_X1_PM_TMR_OFFSr_PM_OFFSET_IN_NSf_SET(PMD_X1_PM_TMR_OFFSr_reg, ns_offset);
    PMD_X1_PM_TMR_OFFSr_PM_OFFSET_SUB_NSf_SET(PMD_X1_PM_TMR_OFFSr_reg, sub_ns_offset);
    /* Program both MPPs. */
    phy_copy.access.lane_mask = 0x1;
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_PM_TMR_OFFSr(&phy_copy, PMD_X1_PM_TMR_OFFSr_reg));
    phy_copy.access.lane_mask = 0x10;
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_PM_TMR_OFFSr(&phy_copy, PMD_X1_PM_TMR_OFFSr_reg));

    return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_pcs_1588_ts_offset_get(PHYMOD_ST *pc, uint32_t *ns_offset, uint32_t *sub_ns_offset)
{
    PMD_X1_PM_TMR_OFFSr_t PMD_X1_PM_TMR_OFFSr_reg;

    PMD_X1_PM_TMR_OFFSr_CLR(PMD_X1_PM_TMR_OFFSr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X1_PM_TMR_OFFSr(pc, &PMD_X1_PM_TMR_OFFSr_reg));

    *ns_offset = PMD_X1_PM_TMR_OFFSr_PM_OFFSET_IN_NSf_GET(PMD_X1_PM_TMR_OFFSr_reg);
    *sub_ns_offset = PMD_X1_PM_TMR_OFFSr_PM_OFFSET_SUB_NSf_GET(PMD_X1_PM_TMR_OFFSr_reg);

    return PHYMOD_E_NONE;
}

#ifdef SERDES_API_FLOATING_POINT
static
float _tbhmod_pcs_vco_to_clk_period(uint32_t vco, int os_mode)
{
    float clk_period = 0, vco_val = 0;

    switch (vco) {
        case APERTA_TBHMOD_VCO_20G:
            vco_val = 20.6250;
            break;
        case APERTA_TBHMOD_VCO_25G:
            vco_val = 25.8125;
            break;
        case APERTA_TBHMOD_VCO_26G:
            vco_val = 26.5625;
            break;
        default:
            return clk_period;
    }

    switch (os_mode) {
        case 0:
            /* OSX1 */
            clk_period = ((float) 20/vco_val);
            break;
        case 1:
            /* OSX2 */
            clk_period = ((float) 40/vco_val);
            break;
        default:
            break;
    }

    return clk_period;
}
#endif

int plp_aperta_tbhmod_ts_offset_rx_set(PHYMOD_ST* pc, uint32_t vco, int os_mode, int tbl_ln, uint32_t *table)
{
    PMD_X1_PM_TMR_OFFSr_t PMD_X1_PM_TMR_OFFSr_reg;
    uint16_t pm_offset_ns, pm_offset_sub_ns;
    int32_t pm_offset, vl_time, vl_update_time;
    int i;
#ifdef SERDES_API_FLOATING_POINT
    float rx_clk_period;
#endif

    PMD_X1_PM_TMR_OFFSr_CLR(PMD_X1_PM_TMR_OFFSr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X1_PM_TMR_OFFSr(pc, &PMD_X1_PM_TMR_OFFSr_reg));
    pm_offset_ns = PMD_X1_PM_TMR_OFFSr_PM_OFFSET_IN_NSf_GET(PMD_X1_PM_TMR_OFFSr_reg);
    pm_offset_sub_ns = PMD_X1_PM_TMR_OFFSr_PM_OFFSET_SUB_NSf_GET(PMD_X1_PM_TMR_OFFSr_reg);
    /* pm_offset:
     * bit 0-7: sub ns.
     * bit 8-30: ns.
     * bit 31: sign (always 0).
     */
    pm_offset = ((pm_offset_ns & 0xff) << 8) | (pm_offset_sub_ns & 0xff);

#ifdef SERDES_API_FLOATING_POINT
    rx_clk_period = _tbhmod_pcs_vco_to_clk_period(vco, os_mode);
#endif
    /* Each entry in the table is in below format:
     * bit 0-3: sub ns.
     * bit 4-14: ns.
     * bit 15-19: virtual lane number.
     */
    for (i = 0; i < tbl_ln; i++) {
        /* Get the time from table and extend the sub_ns field to 8 bit. */
        vl_time = ((table[i] << 4) & 0x7fff0);
        if ((vl_time >> 18) & 1) {
            /* Sign bit extension. */
            vl_time = (vl_time | 0xfff80000);
        }

#ifdef SERDES_API_FLOATING_POINT
        vl_update_time = vl_time + pm_offset + ((uint32_t) ((float)((rx_clk_period * 4) * 256)));
#else
        vl_update_time = vl_time + pm_offset;
#endif
        /* We do not expect overflow here so omitting the overflow check. */
        table[i] = (table[i] & 0xf8000) | ((vl_update_time >> 4) & 0x7fff);
    }

    return PHYMOD_E_NONE;
}

static
uint32_t _plp_aperta_tbhmod_pcs_vco_to_ui(uint32_t vco, int os_mode, int pam4)
{
  uint32_t ui_value;
  ui_value = 0;
  switch(vco) {
    case APERTA_TBHMOD_VCO_20G:
      if(os_mode == 1) {
        ui_value = 0x18d3018c; /* 2/20.625 = 0.09696.. make 32 bit 0.09696.. * 2^32 = 416481676 = 0x18d3018c */
      } else {
        ui_value = 0x0c6980c6; /* 1/20.625 = 0.04848.. make 32 bit 0.04848.. * 2^32 = 208240838 = 0xc6980c6 */
      }
      break;
    case APERTA_TBHMOD_VCO_25G:
      if(pam4) {
        ui_value = 0x04f7004e;
      } else {
        ui_value = 0x09ee009c;
      }
      break;
    case APERTA_TBHMOD_VCO_25P00G:
      if(os_mode == 1) {
        ui_value = 0x147ae147;
      } else {
        ui_value = 0x0a3d70a3;
      }
      break;
    case APERTA_TBHMOD_VCO_26G:
      if(pam4) {
        ui_value = 0x04d19e6b;
      } else {
        ui_value = 0x09a33cd6;
      }
      break;
      default: break;
  }
  return ui_value;
}

/*!
 *  @brief plp_aperta_tbhmod_pcs_set_1588_ui per port.
@param unit number for instance lane number for decide which lane
@param vco tells which vco is used in the lane
@param os_mode tells which os_mode is used in the lane
@param clk4sync_div the fast clk divider 0 => 8 and 1 => 6
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Enables ts in rx path */
int plp_aperta_tbhmod_pcs_set_1588_ui (PHYMOD_ST* pc, uint32_t vco, uint32_t vco1, int os_mode, int clk4sync_div, int pam4)
{
  PMD_X1_FCLK_PERIODr_t PMD_X1_FCLK_PERIODr_reg;
  PMD_X4_UI_VALUE_HIr_t PMD_X4_UI_VALUE_HIr_reg;
  PMD_X4_UI_VALUE_LOr_t PMD_X4_UI_VALUE_LOr_reg;
  int fclk_period = 0, ui_value_hi, ui_value_lo, ui_value_lo_m17_to_m23;


  ui_value_hi = _plp_aperta_tbhmod_pcs_vco_to_ui(vco, os_mode, pam4)>>16;
  ui_value_lo = _plp_aperta_tbhmod_pcs_vco_to_ui(vco, os_mode, pam4)&0xffff;

  /* ui_value_lo represent ui_value m17 to m32. For register programming,
   * we only need m17 to m23 value. So right shift 9 bits. */
  ui_value_lo_m17_to_m23 = ui_value_lo >> 9;


  switch(vco1) {
    case APERTA_TBHMOD_VCO_20G:
      if(clk4sync_div) fclk_period = 0x4a79; else fclk_period = 0x634c;
      break;
    case APERTA_TBHMOD_VCO_25G:
      fclk_period = 0x4f70;
      break;
    case APERTA_TBHMOD_VCO_26G:
      fclk_period = 0x4d19;
      break;
      default: break;
  }


  PMD_X1_FCLK_PERIODr_CLR(PMD_X1_FCLK_PERIODr_reg);
  PMD_X4_UI_VALUE_HIr_CLR(PMD_X4_UI_VALUE_HIr_reg);
  PMD_X4_UI_VALUE_LOr_CLR(PMD_X4_UI_VALUE_LOr_reg);

  PHYMOD_IF_ERR_RETURN(READ_PMD_X1_FCLK_PERIODr(pc, &PMD_X1_FCLK_PERIODr_reg));
  PHYMOD_IF_ERR_RETURN(READ_PMD_X4_UI_VALUE_HIr(pc, &PMD_X4_UI_VALUE_HIr_reg));
  PHYMOD_IF_ERR_RETURN(READ_PMD_X4_UI_VALUE_LOr(pc, &PMD_X4_UI_VALUE_LOr_reg));


  PMD_X1_FCLK_PERIODr_FCLK_FRAC_NSf_SET(PMD_X1_FCLK_PERIODr_reg, fclk_period);
  PMD_X4_UI_VALUE_HIr_UI_FRAC_M1_TO_M16f_SET( PMD_X4_UI_VALUE_HIr_reg, ui_value_hi);
  PMD_X4_UI_VALUE_LOr_UI_FRAC_M17_TO_M23f_SET( PMD_X4_UI_VALUE_LOr_reg, ui_value_lo_m17_to_m23);

  PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_FCLK_PERIODr(pc, PMD_X1_FCLK_PERIODr_reg));
  PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_UI_VALUE_HIr(pc, PMD_X4_UI_VALUE_HIr_reg));
  PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_UI_VALUE_LOr(pc, PMD_X4_UI_VALUE_LOr_reg));

  return PHYMOD_E_NONE;
}

/*!
 *  @brief plp_aperta_tbhmod_1588_pmd_latency per port.
@param unit number for instance lane number for decide which lane
@param vco tells which vco is used in the lane
@param os_mode tells which os_mode is used in the lane
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Enables ts in rx path */
int plp_aperta_tbhmod_1588_pmd_latency (PHYMOD_ST* pc, uint32_t vco, int os_mode, int pam4)
{
  PHYMOD_ST phy_copy;
  int start_lane, num_lane, i;
  uint32_t tx_latency, rx_latency;
  uint32_t ui_val;
  PMD_X4_RX_FIXED_LATENCYr_t PMD_X4_RX_FIXED_LATENCYr_reg;
  PMD_X4_TX_FIXED_LATENCYr_t PMD_X4_TX_FIXED_LATENCYr_reg;

  PHYMOD_IF_ERR_RETURN
      (plp_aperta_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));

  PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));
  /* ui_val with fractional bit 32 bit from _tbhmod_pcs_vco_to_ui we need 10 fractional
     so we need 22 bit shift first shifting 8 then 14 to improve accuracy
  */
  ui_val = _plp_aperta_tbhmod_pcs_vco_to_ui(vco, os_mode, pam4)>>8;

  /* tx_latency and rx_latency with fractional bit 10 bit */
  if(pam4) {
    tx_latency = (ui_val * (124-20)) >> 14;
    rx_latency = (ui_val * (81 +20)) >> 14;
  } else {
    tx_latency = (ui_val * (144-20)) >> 14;  /* delay number from Blackhawk_V1.0 User Spec.pdf */
    rx_latency = (ui_val * (121+20)) >> 14;
  }

  for(i=0; i < num_lane; i++) {
    
    /* pc.this_lane = lane+i; */
    phy_copy.access.lane_mask = 0x1 << (start_lane + i);
    PMD_X4_RX_FIXED_LATENCYr_CLR(PMD_X4_RX_FIXED_LATENCYr_reg);
    PMD_X4_TX_FIXED_LATENCYr_CLR(PMD_X4_TX_FIXED_LATENCYr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_RX_FIXED_LATENCYr(pc, &PMD_X4_RX_FIXED_LATENCYr_reg));
    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_TX_FIXED_LATENCYr(pc, &PMD_X4_TX_FIXED_LATENCYr_reg));

    PMD_X4_RX_FIXED_LATENCYr_RX_PMD_LATENCY_IN_NSf_SET (PMD_X4_RX_FIXED_LATENCYr_reg, (rx_latency >> 10));
    PMD_X4_RX_FIXED_LATENCYr_RX_PMD_LATENCY_IN_FRAC_NSf_SET(PMD_X4_RX_FIXED_LATENCYr_reg, (rx_latency & 0x3ff));

    PMD_X4_TX_FIXED_LATENCYr_TX_PMD_LATENCY_IN_NSf_SET (PMD_X4_TX_FIXED_LATENCYr_reg, (tx_latency >> 10));
    PMD_X4_TX_FIXED_LATENCYr_TX_PMD_LATENCY_IN_FRAC_NSf_SET(PMD_X4_TX_FIXED_LATENCYr_reg, (tx_latency & 0x3ff));

    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_RX_FIXED_LATENCYr(pc, PMD_X4_RX_FIXED_LATENCYr_reg));
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_TX_FIXED_LATENCYr(pc, PMD_X4_TX_FIXED_LATENCYr_reg));
  }

  return PHYMOD_E_NONE;
}


int plp_aperta_tbhmod_pcs_rx_ts_update_en(PHYMOD_ST* pc, int en)
{
  RX_X4_RX_TS_CTLr_t RX_X4_RX_TS_CTLr_reg;

  RX_X4_RX_TS_CTLr_CLR(RX_X4_RX_TS_CTLr_reg);

  PHYMOD_IF_ERR_RETURN(READ_RX_X4_RX_TS_CTLr(pc, &RX_X4_RX_TS_CTLr_reg));
  RX_X4_RX_TS_CTLr_TS_UPDATE_ENABLEf_SET(RX_X4_RX_TS_CTLr_reg, en);
  PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_RX_TS_CTLr(pc, RX_X4_RX_TS_CTLr_reg));

  return PHYMOD_E_NONE;
}


/*! 
 *  @brief tbhmod_pcs_rx_deskew_en per port.
@param unit number for instance lane number for decide which lane
@param en tells enabling/disabling r_test_mode AKA scrmable_idle_en
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Enables ts in rx path */
int plp_aperta_tbhmod_pcs_rx_deskew_en(PHYMOD_ST* pc, int en)
{
  RX_X4_RX_TS_CTLr_t RX_X4_RX_TS_CTLr_reg;

  RX_X4_RX_TS_CTLr_CLR(RX_X4_RX_TS_CTLr_reg);

  PHYMOD_IF_ERR_RETURN(READ_RX_X4_RX_TS_CTLr(pc, &RX_X4_RX_TS_CTLr_reg));
  RX_X4_RX_TS_CTLr_RECORD_DESKEW_TS_INFOf_SET(RX_X4_RX_TS_CTLr_reg, en);
  PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_RX_TS_CTLr(pc, RX_X4_RX_TS_CTLr_reg));

  return PHYMOD_E_NONE;
}

/*!
 *  @brief tbhmod_pcs_set_1588_xgmii per port.
@param unit number for instance lane number for decide which lane
@param sfd tells if sfd time stamp or sop time stamp
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Enables ts in rx path */
int plp_aperta_tbhmod_pcs_set_1588_xgmii (PHYMOD_ST* pc, int sfd, uint32_t vco, int os_mode)
{
  int nsT, subNsT;
  RX_X4_RX_TS_CTLr_t RX_X4_RX_TS_CTLr_reg;
  TX_X4_TX_TS_CTLr_t TX_X4_TX_TS_CTLr_reg;


  switch(vco) {
    case APERTA_TBHMOD_VCO_20G:
      if(os_mode == 1) {
        nsT = 3; subNsT = 3;
      } else {
        nsT = 1; subNsT = 9;
      }
      break;
    case APERTA_TBHMOD_VCO_25G:
      nsT = 1; subNsT = 4;
      break;
      default:
      nsT = 3; subNsT = 3;
      break;
  }

  RX_X4_RX_TS_CTLr_CLR(RX_X4_RX_TS_CTLr_reg);
  TX_X4_TX_TS_CTLr_CLR(TX_X4_TX_TS_CTLr_reg);
  PHYMOD_IF_ERR_RETURN(READ_RX_X4_RX_TS_CTLr(pc, &RX_X4_RX_TS_CTLr_reg));
  PHYMOD_IF_ERR_RETURN(READ_TX_X4_TX_TS_CTLr(pc, &TX_X4_TX_TS_CTLr_reg));

  RX_X4_RX_TS_CTLr_SIGN_XGMII_MODE_ADDERf_SET(RX_X4_RX_TS_CTLr_reg, 0);
  RX_X4_RX_TS_CTLr_NS_XGMII_MODE_ADDERf_SET(RX_X4_RX_TS_CTLr_reg, nsT);
  RX_X4_RX_TS_CTLr_SUB_NS_XGMII_MODE_ADDERf_SET(RX_X4_RX_TS_CTLr_reg, subNsT);
  RX_X4_RX_TS_CTLr_RX_SFD_TS_ENf_SET(RX_X4_RX_TS_CTLr_reg, sfd);
  PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_RX_TS_CTLr(pc, RX_X4_RX_TS_CTLr_reg));

  TX_X4_TX_TS_CTLr_XGMII_SFD_TIME_STAMP_ENABLEf_SET(TX_X4_TX_TS_CTLr_reg, sfd);
  PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_TX_TS_CTLr(pc, TX_X4_TX_TS_CTLr_reg));

  return PHYMOD_E_NONE;
}

int _plp_aperta_tbhmod_virtual_lane_count_get(int bit_mux_mode, int num_lane, int* virtual_lanes, int* num_psll_per_phyl)
{
  switch(bit_mux_mode) {
    case 0: {
                *virtual_lanes = num_lane * 1;
                *num_psll_per_phyl = 1;
                break;
            }
    case 1: {
                *virtual_lanes = num_lane * 2;
                *num_psll_per_phyl = 2;
                break;
            }
    case 2: {
                *virtual_lanes = num_lane * 4;
                *num_psll_per_phyl = 4;
                break;
             }
    case 3: {
                *virtual_lanes = num_lane * 5;
                *num_psll_per_phyl = 5;
                break;
             }
    default: {
                *virtual_lanes = num_lane * 1;
                *num_psll_per_phyl = 1;
                break;
             }
  }
  return PHYMOD_E_NONE;
}


int plp_aperta_tbhmod_pcs_mod_rx_1588_tbl_val(PHYMOD_ST* pc, int bit_mux_mode, uint32_t vco, int os_mode, int pam4, int tbl_ln, uint32_t *table) {
#ifdef SERDES_API_FLOATING_POINT
  PHYMOD_ST phy_copy;
  int i = 0, curr_psll = 0, num_psll_per_phyl = 0;
  int num_lane = 0, start_lane = 0, virtual_lanes = 0;
  int32_t  table_time = 0;

  /* per psuedo logical lane
   * the equation: {NS,sub_ns} + fclk_period * fclk_adjust + bit_offset * bit_ui_val
   * there are 4 logical lanes per mpp and 20 virtual lanes.
   */
  uint32_t psll_update[20];
  uint32_t ui_value = 0;
  float bit_ui_val = 0, bit_ui_val1 = 0, bit_ui_val2 = 0;
  uint16_t base_ts_ns = 0;
  uint16_t base_ts_subns = 0;
  uint8_t am_slip_cnt = 0;
  uint8_t am_ts_fclk_adj_val = 0;
  uint8_t deskew_ts_info_val = 0;
  uint32_t fclk_period = 0;
  uint32_t psll_val_min = 0, psll_roval_min = 0;
  uint32_t psll_val_max = 0, psll_roval_max = 0;
  int32_t  vlane_val_correction = 0;
  int32_t  psll_val_correction[20];
  int32_t  psll_to_vl_map[20];
  int8_t vl_to_psll_map[20];
  int8_t vl_to_psll_off_map[20];
  uint32_t curr_vl = 0;
  PMD_X1_FCLK_PERIODr_t PMD_X1_FCLK_PERIODr_reg;
  RX_X4_PSLL_TO_VL_MAP0r_t RX_X4_PSLL_TO_VL_MAP0r_reg;
  RX_X4_PSLL_TO_VL_MAP1r_t RX_X4_PSLL_TO_VL_MAP1r_reg;
  RX_X4_SKEW_OFFSS0r_t RX_X4_SKEW_OFFSS0r_reg;
  RX_X4_SKEW_OFFSS1r_t RX_X4_SKEW_OFFSS1r_reg;
  RX_X4_SKEW_OFFSS2r_t RX_X4_SKEW_OFFSS2r_reg;
  RX_X4_SKEW_OFFSS3r_t RX_X4_SKEW_OFFSS3r_reg;
  RX_X4_SKEW_OFFSS4r_t RX_X4_SKEW_OFFSS4r_reg;
  RX_X4_AM_TS_INFO0r_t RX_X4_AM_TS_INFO0r_reg;
  RX_X4_AM_TS_INFO1r_t RX_X4_AM_TS_INFO1r_reg;
  RX_X4_AM_TS_INFO2r_t RX_X4_AM_TS_INFO2r_reg;
  RX_X4_AM_TS_INFO3r_t RX_X4_AM_TS_INFO3r_reg;
  RX_X4_AM_TS_INFO4r_t RX_X4_AM_TS_INFO4r_reg;
  PHYMOD_MEMSET(psll_update, 0 , sizeof(uint32_t) * 20);
  PHYMOD_MEMSET(psll_val_correction, 0 , sizeof(uint32_t) * 20);
  PHYMOD_MEMSET(psll_to_vl_map, 0 , sizeof(uint32_t) * 20);
  PHYMOD_MEMSET(vl_to_psll_map, 0, sizeof(int8_t)*20);
  PHYMOD_MEMSET(vl_to_psll_off_map,0, sizeof(int8_t)*20);
  PHYMOD_MEMSET(&RX_X4_SKEW_OFFSS0r_reg, 0, sizeof(RX_X4_SKEW_OFFSS0r_reg)); 
  PHYMOD_MEMSET(&RX_X4_SKEW_OFFSS1r_reg, 0, sizeof(RX_X4_SKEW_OFFSS1r_reg)); 
  PHYMOD_MEMSET(&RX_X4_SKEW_OFFSS2r_reg, 0, sizeof(RX_X4_SKEW_OFFSS2r_reg));
  PHYMOD_MEMSET(&RX_X4_SKEW_OFFSS3r_reg, 0, sizeof(RX_X4_SKEW_OFFSS3r_reg));
  PHYMOD_MEMSET(&RX_X4_SKEW_OFFSS4r_reg, 0, sizeof(RX_X4_SKEW_OFFSS4r_reg));
  PHYMOD_MEMSET(&RX_X4_AM_TS_INFO0r_reg, 0, sizeof(RX_X4_AM_TS_INFO0r_reg));
  PHYMOD_MEMSET(&RX_X4_AM_TS_INFO1r_reg, 0, sizeof(RX_X4_AM_TS_INFO1r_reg));
  PHYMOD_MEMSET(&RX_X4_AM_TS_INFO2r_reg, 0, sizeof(RX_X4_AM_TS_INFO2r_reg));
  PHYMOD_MEMSET(&RX_X4_AM_TS_INFO3r_reg, 0, sizeof(RX_X4_AM_TS_INFO3r_reg));
  PHYMOD_MEMSET(&RX_X4_AM_TS_INFO4r_reg, 0, sizeof(RX_X4_AM_TS_INFO4r_reg));
  PHYMOD_MEMSET(&PMD_X1_FCLK_PERIODr_reg, 0 , sizeof(PMD_X1_FCLK_PERIODr_reg));
  PHYMOD_MEMSET(&RX_X4_PSLL_TO_VL_MAP0r_reg, 0, sizeof(RX_X4_PSLL_TO_VL_MAP0r_reg));
  PHYMOD_MEMSET(&RX_X4_PSLL_TO_VL_MAP1r_reg, 0, sizeof(RX_X4_PSLL_TO_VL_MAP1r_reg));

  PHYMOD_IF_ERR_RETURN
      (plp_aperta_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));

  PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));

  PHYMOD_IF_ERR_RETURN(
       _plp_aperta_tbhmod_virtual_lane_count_get(bit_mux_mode, num_lane, &virtual_lanes, &num_psll_per_phyl));

  if(virtual_lanes == 1) { return PHYMOD_E_NONE; }

  ui_value = _plp_aperta_tbhmod_pcs_vco_to_ui(vco, os_mode, pam4);

  bit_ui_val1 = (float) ui_value/1024;
  bit_ui_val2 = (float) bit_ui_val1/1024;
  bit_ui_val = (float) bit_ui_val2/4096;

  psll_val_min = 0x80000;
  psll_roval_min = 0x80000;
  psll_val_max = 0;
  psll_roval_max = 0;
  curr_psll = 0;
  for (i = 0; i < 20; i++) {
        psll_val_correction[i] = 0;
        psll_to_vl_map[i] = 0;
        psll_update[i] = 0;
        vl_to_psll_map[i] = 0;
        vl_to_psll_off_map[i] = 0;
    }
  PHYMOD_IF_ERR_RETURN(READ_PMD_X1_FCLK_PERIODr(pc,&PMD_X1_FCLK_PERIODr_reg));
  fclk_period = PMD_X1_FCLK_PERIODr_FCLK_FRAC_NSf_GET(PMD_X1_FCLK_PERIODr_reg);


  for(i=0; i < num_lane; i++) {
        /* pc.this_lane = lane + i; */
        phy_copy.access.lane_mask = 0x1 << (start_lane + i);

        PHYMOD_IF_ERR_RETURN(READ_RX_X4_PSLL_TO_VL_MAP0r(&phy_copy,&RX_X4_PSLL_TO_VL_MAP0r_reg));
        PHYMOD_IF_ERR_RETURN(READ_RX_X4_PSLL_TO_VL_MAP1r(&phy_copy,&RX_X4_PSLL_TO_VL_MAP1r_reg));
        if (num_psll_per_phyl > 0) {
            curr_vl  = RX_X4_PSLL_TO_VL_MAP0r_PSLL0_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP0r_reg);
            psll_to_vl_map[curr_vl] = curr_psll;
            vl_to_psll_map[curr_vl] = start_lane + i;
            vl_to_psll_off_map[curr_vl] = 0;
            curr_psll++;
        }
        if (num_psll_per_phyl > 1) {
            curr_vl = RX_X4_PSLL_TO_VL_MAP0r_PSLL1_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP0r_reg);
            psll_to_vl_map[curr_vl] = curr_psll;
            vl_to_psll_map[curr_vl] = start_lane + i;
            vl_to_psll_off_map[curr_vl] = 1;
            curr_psll++;
        }
        if (num_psll_per_phyl > 2) {
            curr_vl = RX_X4_PSLL_TO_VL_MAP0r_PSLL2_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP0r_reg);
            psll_to_vl_map[curr_vl] = curr_psll;
            vl_to_psll_map[curr_vl] = start_lane + i;
            vl_to_psll_off_map[curr_vl] = 2;
            curr_psll++;
        }
        if (num_psll_per_phyl > 3) {
            curr_vl = RX_X4_PSLL_TO_VL_MAP1r_PSLL3_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP1r_reg);
            psll_to_vl_map[curr_vl] = curr_psll;
            vl_to_psll_map[curr_vl] = start_lane + i;
            vl_to_psll_off_map[curr_vl] = 3;
            curr_psll++;
        }
        if (num_psll_per_phyl > 4) {
            curr_vl = RX_X4_PSLL_TO_VL_MAP1r_PSLL4_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP1r_reg);
            psll_to_vl_map[curr_vl] = curr_psll;
            vl_to_psll_map[curr_vl] = start_lane + i;
            vl_to_psll_off_map[curr_vl] = 4;
            curr_psll++;
        }

  }

  for(i=0; i<virtual_lanes; i++) {
    /* pc.this_lane = lane+(i/num_psll_per_phyl);*/
    phy_copy.access.lane_mask = 0x1 << vl_to_psll_map[i];
    if (vl_to_psll_off_map[i] == 0) {
      PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS0r(&phy_copy,&RX_X4_SKEW_OFFSS0r_reg));
      PHYMOD_IF_ERR_RETURN(READ_RX_X4_AM_TS_INFO0r(&phy_copy,&RX_X4_AM_TS_INFO0r_reg));
      base_ts_ns = RX_X4_AM_TS_INFO0r_BASE_TS_NS_0f_GET(RX_X4_AM_TS_INFO0r_reg);
      base_ts_subns = RX_X4_AM_TS_INFO0r_BASE_TS_SUB_NS_0f_GET(RX_X4_AM_TS_INFO0r_reg);
      am_slip_cnt = RX_X4_SKEW_OFFSS0r_AM_SLIP_COUNT_0f_GET(RX_X4_SKEW_OFFSS0r_reg);
      am_ts_fclk_adj_val = RX_X4_SKEW_OFFSS0r_AM_TS_FCLK_ADJUST_VALUE_0f_GET(RX_X4_SKEW_OFFSS0r_reg);
      deskew_ts_info_val = RX_X4_SKEW_OFFSS0r_DESKEW_TS_INFO_VALID_0f_GET(RX_X4_SKEW_OFFSS0r_reg);
      if (deskew_ts_info_val == 1) {
        psll_update[i] = ((base_ts_ns << 4) | (base_ts_subns & 0xf)) + ((uint32_t) (((float) fclk_period/65536)*am_ts_fclk_adj_val) << 4) + ((uint32_t) ((float)(am_slip_cnt * bit_ui_val)) << 4);
        if ( psll_update[i] < psll_val_min) psll_val_min = psll_update[i];
        if ( psll_update[i] > psll_val_max) psll_val_max = psll_update[i];
      }
    } else if (vl_to_psll_off_map[i] == 1) {
      PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS1r(&phy_copy,&RX_X4_SKEW_OFFSS1r_reg));
      PHYMOD_IF_ERR_RETURN(READ_RX_X4_AM_TS_INFO1r(&phy_copy,&RX_X4_AM_TS_INFO1r_reg));
      base_ts_ns = RX_X4_AM_TS_INFO1r_BASE_TS_NS_1f_GET(RX_X4_AM_TS_INFO1r_reg);
      base_ts_subns = RX_X4_AM_TS_INFO1r_BASE_TS_SUB_NS_1f_GET(RX_X4_AM_TS_INFO1r_reg);
      am_slip_cnt = RX_X4_SKEW_OFFSS1r_AM_SLIP_COUNT_1f_GET(RX_X4_SKEW_OFFSS1r_reg);
      am_ts_fclk_adj_val = RX_X4_SKEW_OFFSS1r_AM_TS_FCLK_ADJUST_VALUE_1f_GET(RX_X4_SKEW_OFFSS1r_reg);
      deskew_ts_info_val = RX_X4_SKEW_OFFSS1r_DESKEW_TS_INFO_VALID_1f_GET(RX_X4_SKEW_OFFSS1r_reg);

      if (deskew_ts_info_val == 1) {
        psll_update[i] = ((base_ts_ns << 4) | (base_ts_subns & 0xf)) + ((uint32_t) (((float) fclk_period/65536)*am_ts_fclk_adj_val) << 4) + ((uint32_t) ((float)(am_slip_cnt * bit_ui_val)) << 4);
        if ( psll_update[i] < psll_val_min) psll_val_min = psll_update[i];
        if ( psll_update[i] > psll_val_max) psll_val_max = psll_update[i];
      }
    } else if (vl_to_psll_off_map[i] == 2) {
      PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS2r(&phy_copy,&RX_X4_SKEW_OFFSS2r_reg));
      PHYMOD_IF_ERR_RETURN(READ_RX_X4_AM_TS_INFO2r(&phy_copy,&RX_X4_AM_TS_INFO2r_reg));
      base_ts_ns = RX_X4_AM_TS_INFO2r_BASE_TS_NS_2f_GET(RX_X4_AM_TS_INFO2r_reg);
      base_ts_subns = RX_X4_AM_TS_INFO2r_BASE_TS_SUB_NS_2f_GET(RX_X4_AM_TS_INFO2r_reg);
      am_slip_cnt = RX_X4_SKEW_OFFSS2r_AM_SLIP_COUNT_2f_GET(RX_X4_SKEW_OFFSS2r_reg);
      am_ts_fclk_adj_val = RX_X4_SKEW_OFFSS2r_AM_TS_FCLK_ADJUST_VALUE_2f_GET(RX_X4_SKEW_OFFSS2r_reg);
      deskew_ts_info_val = RX_X4_SKEW_OFFSS2r_DESKEW_TS_INFO_VALID_2f_GET(RX_X4_SKEW_OFFSS2r_reg);

      if (deskew_ts_info_val == 1) {
        psll_update[i] = ((base_ts_ns << 4) | (base_ts_subns & 0xf)) + ((uint32_t) (((float) fclk_period/65536)*am_ts_fclk_adj_val) << 4) + ((uint32_t) ((float)(am_slip_cnt * bit_ui_val)) << 4);
        if ( psll_update[i] < psll_val_min) psll_val_min = psll_update[i];
        if ( psll_update[i] > psll_val_max) psll_val_max = psll_update[i];
      }
    } else if (vl_to_psll_off_map[i] == 3) {
      PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS3r(&phy_copy,&RX_X4_SKEW_OFFSS3r_reg));
      PHYMOD_IF_ERR_RETURN(READ_RX_X4_AM_TS_INFO3r(&phy_copy,&RX_X4_AM_TS_INFO3r_reg));
      base_ts_ns = RX_X4_AM_TS_INFO3r_BASE_TS_NS_3f_GET(RX_X4_AM_TS_INFO3r_reg);
      base_ts_subns = RX_X4_AM_TS_INFO3r_BASE_TS_SUB_NS_3f_GET(RX_X4_AM_TS_INFO3r_reg);
      am_slip_cnt = RX_X4_SKEW_OFFSS3r_AM_SLIP_COUNT_3f_GET(RX_X4_SKEW_OFFSS3r_reg);
      am_ts_fclk_adj_val = RX_X4_SKEW_OFFSS3r_AM_TS_FCLK_ADJUST_VALUE_3f_GET(RX_X4_SKEW_OFFSS3r_reg);
      deskew_ts_info_val = RX_X4_SKEW_OFFSS3r_DESKEW_TS_INFO_VALID_3f_GET(RX_X4_SKEW_OFFSS3r_reg);

      if (deskew_ts_info_val == 1) {
        psll_update[i] = ((base_ts_ns << 4) | (base_ts_subns & 0xf)) + ((uint32_t) (((float) fclk_period/65536)*am_ts_fclk_adj_val) << 4) + ((uint32_t) ((float)(am_slip_cnt * bit_ui_val)) << 4);
        if ( psll_update[i] < psll_val_min) psll_val_min = psll_update[i];
        if ( psll_update[i] > psll_val_max) psll_val_max = psll_update[i];
      }
    } else if (vl_to_psll_off_map[i] == 4) {
      PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS4r(&phy_copy,&RX_X4_SKEW_OFFSS4r_reg));
      PHYMOD_IF_ERR_RETURN(READ_RX_X4_AM_TS_INFO4r(&phy_copy,&RX_X4_AM_TS_INFO4r_reg));
      base_ts_ns = RX_X4_AM_TS_INFO4r_BASE_TS_NS_4f_GET(RX_X4_AM_TS_INFO4r_reg);
      base_ts_subns = RX_X4_AM_TS_INFO4r_BASE_TS_SUB_NS_4f_GET(RX_X4_AM_TS_INFO4r_reg);
      am_slip_cnt = RX_X4_SKEW_OFFSS4r_AM_SLIP_COUNT_4f_GET(RX_X4_SKEW_OFFSS4r_reg);
      am_ts_fclk_adj_val = RX_X4_SKEW_OFFSS4r_AM_TS_FCLK_ADJUST_VALUE_4f_GET(RX_X4_SKEW_OFFSS4r_reg);
      deskew_ts_info_val = RX_X4_SKEW_OFFSS4r_DESKEW_TS_INFO_VALID_4f_GET(RX_X4_SKEW_OFFSS4r_reg);

      if (deskew_ts_info_val == 1) {
        psll_update[i] = ((base_ts_ns << 4) | (base_ts_subns & 0xf)) + ((uint32_t) (((float) fclk_period/65536)*am_ts_fclk_adj_val) << 4) + ((uint32_t) ((float)(am_slip_cnt * bit_ui_val)) << 4);
        if ( psll_update[i] < psll_val_min) psll_val_min = psll_update[i];
        if ( psll_update[i] > psll_val_max) psll_val_max = psll_update[i];
       }
     }
  }
   /* Normalize to earliest */
    if ((psll_val_max - psll_val_min) > 0x1000) {
      /* Sometimes the psll_update could rollover. In case of rollover all the values need to
       * equalized meaning account for rollover and then normalize it as usual. */
      for(i=0; i<virtual_lanes; i++) {
          /***
          printf("tbhmod_pcs_mod_rx_1588_tbl :correction: psll_update[%d] : %8x : psll_val_min : %8x : diff : %8x\n",
          i, psll_update[i], psll_val_min, psll_update[i]-psll_val_min);
          ****/
          if((psll_val_max - psll_update[i]) > 0x01000) psll_update[i] |= 0x10000 ;
          if (psll_update[i] < psll_roval_min)  psll_roval_min = psll_update[i];
          if (psll_update[i] > psll_roval_max)  psll_roval_max = psll_update[i];
      }
      for(i=0; i<virtual_lanes; i++) {
          psll_val_correction[i] = psll_update[i]-psll_roval_min;
      }
    } else {
       for(i=0; i<virtual_lanes; i++) {
          psll_val_correction[i] = psll_update[i]-psll_val_min;
       }
    }

    /* Normalize to latest */
    /****
    for(i=0; i<virtual_lanes; i++) {
        printf("tbhmod_pcs_mod_rx_1588_tbl :correction: psll_val_correction[%d] : %x : psll_val_max : %x : diff : %x\n",
    i, psll_val_correction[i], psll_val_max, psll_val_correction[i]+psll_val_max);
        psll_val_correction[i] = psll_val_correction[i]+psll_val_max;
    }
    *****/

    for(i=0; i<tbl_ln; i++) {
      curr_vl = table[i] >> 15;
      table_time = table[i] & 0x7fff;
      if((table_time >> 14)&1) {
        table_time = (table_time | 0xffff8000); /* sign ext */
      }
      curr_psll = psll_to_vl_map[curr_vl];
      vlane_val_correction = psll_val_correction[curr_psll];

      if ((vlane_val_correction >> 14)&1) {
          table_time = table_time +  (vlane_val_correction & 0x7fff);
      } else {
          table_time = table_time -  (vlane_val_correction & 0x7fff);
      }

      table[i] = (table[i] & 0xf8000) | (table_time & 0x7fff); /* bit 19-bit 15 VL bit 14-bit 0 time */
    }
#endif /* SERDES_API_FLOAT_POINT */
    return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_pcs_ts_deskew_valid(PHYMOD_ST* pc, int bit_mux_mode, int *rx_ts_deskew_valid) {
  int start_lane, num_lane;
  int i, virtual_lanes = 0, num_psll_per_phyl = 0;
  int ts_deskew_valid;
  PHYMOD_ST phy_copy;

  RX_X4_SKEW_OFFSS0r_t RX_X4_SKEW_OFFSS0r_reg;
  RX_X4_SKEW_OFFSS1r_t RX_X4_SKEW_OFFSS1r_reg;
  RX_X4_SKEW_OFFSS2r_t RX_X4_SKEW_OFFSS2r_reg;
  RX_X4_SKEW_OFFSS3r_t RX_X4_SKEW_OFFSS3r_reg;
  RX_X4_SKEW_OFFSS4r_t RX_X4_SKEW_OFFSS4r_reg;

  PHYMOD_IF_ERR_RETURN
      (plp_aperta_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));
  PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));

  PHYMOD_IF_ERR_RETURN(
      _plp_aperta_tbhmod_virtual_lane_count_get(bit_mux_mode, num_lane, &virtual_lanes, &num_psll_per_phyl));

  *rx_ts_deskew_valid = 1;
  if(virtual_lanes == 1) { return PHYMOD_E_NONE; }

  for(i=0; i<virtual_lanes; i++) {
    /*pc.this_lane = lane+(i%physical_lanes); */
    phy_copy.access.lane_mask = 0x1 << (start_lane + i % num_lane);

    if((i / num_lane) == 0) {
      PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS0r(&phy_copy,&RX_X4_SKEW_OFFSS0r_reg));
      ts_deskew_valid = RX_X4_SKEW_OFFSS0r_DESKEW_TS_INFO_VALID_0f_GET(RX_X4_SKEW_OFFSS0r_reg);
      if(ts_deskew_valid != 1) { *rx_ts_deskew_valid = 0; return PHYMOD_E_NONE; }
    } else if((i / num_lane) == 1) {
      PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS1r(&phy_copy,&RX_X4_SKEW_OFFSS1r_reg));
      ts_deskew_valid = RX_X4_SKEW_OFFSS1r_DESKEW_TS_INFO_VALID_1f_GET(RX_X4_SKEW_OFFSS1r_reg);
      if(ts_deskew_valid != 1) { *rx_ts_deskew_valid = 0; return PHYMOD_E_NONE; }
    } else if((i / num_lane) == 2) {
      PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS2r(&phy_copy,&RX_X4_SKEW_OFFSS2r_reg));
      ts_deskew_valid = RX_X4_SKEW_OFFSS2r_DESKEW_TS_INFO_VALID_2f_GET(RX_X4_SKEW_OFFSS2r_reg);
      if(ts_deskew_valid != 1) { *rx_ts_deskew_valid = 0; return PHYMOD_E_NONE; }
    } else if((i / num_lane) == 3) {
      PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS3r(&phy_copy,&RX_X4_SKEW_OFFSS3r_reg));
      ts_deskew_valid = RX_X4_SKEW_OFFSS3r_DESKEW_TS_INFO_VALID_3f_GET(RX_X4_SKEW_OFFSS3r_reg);
      if(ts_deskew_valid != 1) { *rx_ts_deskew_valid = 0; return PHYMOD_E_NONE; }
    } else if((i / num_lane) == 4) {
      PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS4r(&phy_copy,&RX_X4_SKEW_OFFSS4r_reg));
      ts_deskew_valid = RX_X4_SKEW_OFFSS4r_DESKEW_TS_INFO_VALID_4f_GET(RX_X4_SKEW_OFFSS4r_reg);
      if(ts_deskew_valid != 1) { *rx_ts_deskew_valid = 0; return PHYMOD_E_NONE; }
    }
  }

  return PHYMOD_E_NONE;
}

/*!
 *  @brief tbhmod_pack_spd_id_tbl_entry
@param entry the entry structure to be packed into a uint32_t array
@param packed_entry the uint32_t array containing the data
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Packs a spd_id_tbl_entry_t into a uint32_t array*/
int plp_aperta_tbhmod_pack_spd_id_tbl_entry (spd_id_tbl_entry_t *entry, uint32_t *packed_entry)
{
    plp_aperta_spd_ctrl_pack_spd_id_tbl_entry(entry, packed_entry);
    return PHYMOD_E_NONE;
}

/*!
 *  @brief plp_aperta_tbhmod_unpack_spd_id_tbl_entry
@param packed_entry the uint32_t array containing the data
@param entry the entry structure the data will be unpacked into
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Unpacks a spd_id_tbl_entry_t into a uint32_t array*/
int plp_aperta_tbhmod_unpack_spd_id_tbl_entry (uint32_t *packed_entry, spd_id_tbl_entry_t *entry)
{
    plp_aperta_spd_ctrl_unpack_spd_id_tbl_entry(packed_entry, entry);
    return PHYMOD_E_NONE;
}

/*!
 *  @brief plp_aperta_tbhmod_pack_am_tbl_entry
@param entry the entry structure to be packed into a uint32_t array
@param packed_entry the uint32_t array containing the data
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Packs a am_tbl_entry_t into a uint32_t array*/
int plp_aperta_tbhmod_pack_am_tbl_entry (am_tbl_entry_t *entry, uint32_t *packed_entry)
{
    plp_aperta_spd_ctrl_pack_am_tbl_entry(entry, packed_entry);
    return PHYMOD_E_NONE;
}


/*!
 *  @brief plp_aperta_tbhmod_unpack_am_tbl_entry
@param packed_entry the uint32_t array containing the data
@param entry the entry structure the data will be unpacked into
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Unpacks a am_tbl_entry_t into a uint32_t array*/
int plp_aperta_tbhmod_unpack_am_tbl_entry (uint32_t *packed_entry, am_tbl_entry_t *entry)
{
    plp_aperta_spd_ctrl_unpack_am_tbl_entry(packed_entry, entry);
    return PHYMOD_E_NONE;
}

/*!
 *  @brief plp_aperta_tbhmod_pack_um_tbl_entry
@param entry the entry structure to be packed into a uint32_t array
@param packed_entry the uint32_t array containing the data
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Packs a um_tbl_entry_t into a uint32_t array*/
int plp_aperta_tbhmod_pack_um_tbl_entry (um_tbl_entry_t *entry, uint32_t *packed_entry)
{
    plp_aperta_spd_ctrl_pack_um_tbl_entry(entry, packed_entry);
    return PHYMOD_E_NONE;
}

/*!
 *  @brief plp_aperta_tbhmod_unpack_um_tbl_entry
@param packed_entry the uint32_t array containing the data
@param entry the entry structure the data will be unpacked into
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Unpacks a um_tbl_entry_t into a uint32_t array*/
int plp_aperta_tbhmod_unpack_um_tbl_entry (uint32_t *packed_entry, um_tbl_entry_t *entry)
{
    plp_aperta_spd_ctrl_unpack_um_tbl_entry(packed_entry, entry);
    return PHYMOD_E_NONE;
}

/**
 * @brief   Select the ILKN path and bypass TSCF PCS
 * @param   pc handle to current TSCBH context (#aperta_tbhmod_st)
 * @returns The value PHYMOD_E_NONE upon successful completion
 * @details This will enable ILKN path. PCS is set to bypass to relinquish PMD
 * control. Expect PMD to be programmed elsewhere.
 * */
int plp_aperta_tbhmod_init_pcs_ilkn(PHYMOD_ST* pc)              /* INIT_PCS_ILKN */
{
  ILKN_CTL0r_t ILKN_CONTROL0r_reg;

  APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc);

  ILKN_CTL0r_CLR(ILKN_CONTROL0r_reg);
  PHYMOD_IF_ERR_RETURN(READ_ILKN_CTL0r(pc, &ILKN_CONTROL0r_reg));
  ILKN_CTL0r_SOFT_RST_RXf_SET(ILKN_CONTROL0r_reg, 0);
  ILKN_CTL0r_SOFT_RST_TXf_SET(ILKN_CONTROL0r_reg, 0);
  PHYMOD_IF_ERR_RETURN(MODIFY_ILKN_CTL0r(pc, ILKN_CONTROL0r_reg));

  
  ILKN_CTL0r_CLR(ILKN_CONTROL0r_reg);
  PHYMOD_IF_ERR_RETURN(READ_ILKN_CTL0r(pc, &ILKN_CONTROL0r_reg));
  ILKN_CTL0r_CREDIT_ENf_SET(ILKN_CONTROL0r_reg, 1);
  ILKN_CTL0r_ILKN_SELf_SET(ILKN_CONTROL0r_reg, 1);
  PHYMOD_IF_ERR_RETURN(MODIFY_ILKN_CTL0r(pc, ILKN_CONTROL0r_reg));

  return PHYMOD_E_NONE;
}

/**
 * @brief   Select the ILKN path and bypass TSCF PCS
 * @param   pc handle to current TSCBH context (#tbhmod_st)
 * @returns The value PHYMOD_E_NONE upon successful completion
 * @details This will enable ILKN path. PCS is set to bypass to relinquish PMD
 * control. Expect PMD to be programmed elsewhere.
 * */
int plp_aperta_tbhmod_pcs_ilkn_enable(PHYMOD_ST* pc, int enable)
{
  ILKN_CTL0r_t ILKN_CONTROL0r_reg;
  PMD_X4_OVRRr_t pmd_x4_override;

  APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc);

  ILKN_CTL0r_CLR(ILKN_CONTROL0r_reg);
  PHYMOD_IF_ERR_RETURN(READ_ILKN_CTL0r(pc, &ILKN_CONTROL0r_reg));
  ILKN_CTL0r_SOFT_RST_RXf_SET(ILKN_CONTROL0r_reg, 0);
  ILKN_CTL0r_SOFT_RST_TXf_SET(ILKN_CONTROL0r_reg, 0);
  PHYMOD_IF_ERR_RETURN(MODIFY_ILKN_CTL0r(pc, ILKN_CONTROL0r_reg));

  
  ILKN_CTL0r_CLR(ILKN_CONTROL0r_reg);
  PHYMOD_IF_ERR_RETURN(READ_ILKN_CTL0r(pc, &ILKN_CONTROL0r_reg));
  ILKN_CTL0r_CREDIT_ENf_SET(ILKN_CONTROL0r_reg, enable);
  ILKN_CTL0r_ILKN_SELf_SET(ILKN_CONTROL0r_reg, enable);
  PHYMOD_IF_ERR_RETURN(MODIFY_ILKN_CTL0r(pc, ILKN_CONTROL0r_reg));

  /* if ILKN enable, set the PMD ln dp reset override bit as well */
  PMD_X4_OVRRr_CLR(pmd_x4_override);
  PHYMOD_IF_ERR_RETURN(READ_PMD_X4_OVRRr(pc, &pmd_x4_override));
  PMD_X4_OVRRr_LN_DP_H_RSTB_OENf_SET(pmd_x4_override, enable);
  PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_OVRRr(pc, pmd_x4_override));

  return PHYMOD_E_NONE;
}


/**
 * @brief  Get the plldiv from lookup table
 * @param  pc handle to current TSCBH context (#PHYMOD_ST)
 * @param  spd_intf  Input of enum type #tefmod16_spd_intfc_type_t
 * @param  plldiv  Receives PLL Divider value
 * @returns The value PHYMOD_E_NONE upon successful completion
 * @details Get the plldiv from lookup table as a function of the speed.
 * */

int plp_aperta_tbhmod_plldiv_lkup_get(PHYMOD_ST* pc, int mapped_speed_id, aperta_tbhmod_refclk_t refclk,  uint32_t *plldiv)
{
  APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc);
  if (refclk == APERTA_TBHMOD_REF_CLK_312P5MHZ) {
    *plldiv = plp_aperta_tscbh_sc_pmd_entry_312M_ref[mapped_speed_id].pll_mode;
  } else {
    *plldiv = plp_aperta_tscbh_sc_pmd_entry[mapped_speed_id].pll_mode;
  }

  return PHYMOD_E_NONE;

}


/**
 * @brief   Check if ILKN is set
 * @param   pc handle to current TSCBH context (#PHYMOD_ST), ilkn_set status
 * @returns The value PHYMOD_E_NONE upon successful completion
 * @details Check if ILKN is set, ilkn_set = 1 if it is set.
 * */
int plp_aperta_tbhmod_pcs_ilkn_chk(PHYMOD_ST* pc, int *ilkn_set)              /* INIT_PCS_ILKN */
{
  ILKN_CTL0r_t ILKN_CONTROL0r_reg;

  APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc);

  PHYMOD_IF_ERR_RETURN(READ_ILKN_CTL0r(pc, &ILKN_CONTROL0r_reg));
  *ilkn_set = ILKN_CTL0r_ILKN_SELf_GET(ILKN_CONTROL0r_reg);

  return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_refclk_set(PHYMOD_ST* pc, aperta_tbhmod_refclk_t ref_clk)
{
  MAIN0_SETUPr_t MAIN0_SETUPr_reg;

  APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc);

  pc->access.lane_mask = 0x1 << 0;
  PHYMOD_IF_ERR_RETURN(READ_MAIN0_SETUPr(pc, &MAIN0_SETUPr_reg));
  MAIN0_SETUPr_REFCLK_SELf_SET(MAIN0_SETUPr_reg, ref_clk);
  PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(pc, MAIN0_SETUPr_reg));

  pc->access.lane_mask = 0x1 << 4;
  PHYMOD_IF_ERR_RETURN(READ_MAIN0_SETUPr(pc, &MAIN0_SETUPr_reg));
  MAIN0_SETUPr_REFCLK_SELf_SET(MAIN0_SETUPr_reg, ref_clk);
  PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(pc, MAIN0_SETUPr_reg));

  return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_refclk_get(PHYMOD_ST* pc, aperta_tbhmod_refclk_t* ref_clk)
{
  MAIN0_SETUPr_t MAIN0_SETUPr_reg;

  APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc);

  PHYMOD_IF_ERR_RETURN(READ_MAIN0_SETUPr(pc, &MAIN0_SETUPr_reg));
  *ref_clk = MAIN0_SETUPr_REFCLK_SELf_GET(MAIN0_SETUPr_reg);

  return PHYMOD_E_NONE;
}

/**
 * @brief   Set Per lane OS mode set in PMD
 * @param   pc handle to current TSCBH context (#PHYMOD_ST)
 * @param   spd_intf speed type #aperta_tbhmod_spd_intfc_type_t
 * @param   os_mode over sample rate.
 * @returns The value PHYMOD_E_NONE upon successful completion
 * @details Per Port PMD Init
 *
 * */
int plp_aperta_tbhmod_pmd_osmode_set(PHYMOD_ST* pc, int mapped_speed_id, aperta_tbhmod_refclk_t refclk)
{
  RXTXCOM_OSR_MODE_CTLr_t reg_osr_mode;
  int os_mode;

  APERTA_TBHMOD_DBG_IN_FUNC_INFO(pc);
  RXTXCOM_OSR_MODE_CTLr_CLR(reg_osr_mode);
  PHYMOD_IF_ERR_RETURN
     (READ_RXTXCOM_OSR_MODE_CTLr(pc, &reg_osr_mode));

  /* 0=OS_MODE_1;    1=OS_MODE_2;
   * 2=OS_MODE_4;    4=OS_MODE_21p25;
   * 8=OS MODE_16p5; 12=OS_MODE_20p625;  */

  if (refclk == APERTA_TBHMOD_REF_CLK_312P5MHZ) {
      os_mode =  plp_aperta_tscbh_sc_pmd_entry_312M_ref[mapped_speed_id].t_pma_os_mode;
  } else {
      os_mode =  plp_aperta_tscbh_sc_pmd_entry[mapped_speed_id].t_pma_os_mode;
  }

  RXTXCOM_OSR_MODE_CTLr_OSR_MODE_FRCf_SET(reg_osr_mode, 1);
  RXTXCOM_OSR_MODE_CTLr_OSR_MODE_FRC_VALf_SET(reg_osr_mode, os_mode);

  PHYMOD_IF_ERR_RETURN
     (MODIFY_RXTXCOM_OSR_MODE_CTLr(pc, reg_osr_mode));

  return PHYMOD_E_NONE;
}


int plp_aperta_tbhmod_fec_correctable_counter_get(PHYMOD_ST* pc, phymod_fec_type_t fec_type, uint32_t* count)
{
    int start_lane, num_lane;
    RX_X4_FEC_CORR_CTR0r_t lower_reg;
    RX_X4_FEC_CORR_CTR1r_t upper_reg;
    RX_X4_FEC_CORRBLKSL0r_t low_reg;
    RX_X4_FEC_CORRBLKSH0r_t high_reg;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));
    if (fec_type == phymod_fec_CL74) {
        if (num_lane != 8) {
            pc->access.lane_mask = 1 << start_lane;
            PHYMOD_IF_ERR_RETURN(
                   READ_RX_X4_FEC_CORRBLKSL0r(pc, &low_reg));
            PHYMOD_IF_ERR_RETURN(
                   READ_RX_X4_FEC_CORRBLKSH0r(pc, &high_reg));
            *count = BCMI_TSCBH_XGXS_RX_X4_FEC_CORRBLKSH0r_CORCOUNTH_STREAM0f_GET(high_reg) << 16 |
                     BCMI_TSCBH_XGXS_RX_X4_FEC_CORRBLKSL0r_CORCOUNTL_STREAM0f_GET(low_reg);
        } else {
            pc->access.lane_mask = 0x1;
            PHYMOD_IF_ERR_RETURN(
                   READ_RX_X4_FEC_CORRBLKSL0r(pc, &low_reg));
            PHYMOD_IF_ERR_RETURN(
                   READ_RX_X4_FEC_CORRBLKSH0r(pc, &high_reg));
            *count = BCMI_TSCBH_XGXS_RX_X4_FEC_CORRBLKSH0r_CORCOUNTH_STREAM0f_GET(high_reg) << 16 |
                     BCMI_TSCBH_XGXS_RX_X4_FEC_CORRBLKSL0r_CORCOUNTL_STREAM0f_GET(low_reg);

            pc->access.lane_mask = 0x10;
            PHYMOD_IF_ERR_RETURN(
                   READ_RX_X4_FEC_CORRBLKSL0r(pc, &low_reg));
            PHYMOD_IF_ERR_RETURN(
                   READ_RX_X4_FEC_CORRBLKSH0r(pc, &high_reg));
            *count |= BCMI_TSCBH_XGXS_RX_X4_FEC_CORRBLKSH0r_CORCOUNTH_STREAM0f_GET(high_reg) << 16 |
                     BCMI_TSCBH_XGXS_RX_X4_FEC_CORRBLKSL0r_CORCOUNTL_STREAM0f_GET(low_reg);
        }
    } else {
        if (num_lane != 8) {
            pc->access.lane_mask = 1 << start_lane;
            PHYMOD_IF_ERR_RETURN(
                    READ_RX_X4_FEC_CORR_CTR0r(pc, &lower_reg));
            PHYMOD_IF_ERR_RETURN(
                    READ_RX_X4_FEC_CORR_CTR1r(pc, &upper_reg));
            *count = RX_X4_FEC_CORR_CTR1r_RS_FEC_FEC_CORR_CW_CNTR_UPPERf_GET(upper_reg) << 16 |
                RX_X4_FEC_CORR_CTR0r_RS_FEC_FEC_CORR_CW_CNTR_LOWERf_GET(lower_reg);
        } else {
            pc->access.lane_mask = 0x1;
            PHYMOD_IF_ERR_RETURN(
                    READ_RX_X4_FEC_CORR_CTR0r(pc, &lower_reg));
            PHYMOD_IF_ERR_RETURN(
                    READ_RX_X4_FEC_CORR_CTR1r(pc, &upper_reg));
            *count = RX_X4_FEC_CORR_CTR1r_RS_FEC_FEC_CORR_CW_CNTR_UPPERf_GET(upper_reg) << 16 |
                RX_X4_FEC_CORR_CTR0r_RS_FEC_FEC_CORR_CW_CNTR_LOWERf_GET(lower_reg);

            pc->access.lane_mask = 0x10;
            PHYMOD_IF_ERR_RETURN(
                    READ_RX_X4_FEC_CORR_CTR0r(pc, &lower_reg));
            PHYMOD_IF_ERR_RETURN(
                    READ_RX_X4_FEC_CORR_CTR1r(pc, &upper_reg));
            *count |= RX_X4_FEC_CORR_CTR1r_RS_FEC_FEC_CORR_CW_CNTR_UPPERf_GET(upper_reg) << 16 |
                RX_X4_FEC_CORR_CTR0r_RS_FEC_FEC_CORR_CW_CNTR_LOWERf_GET(lower_reg);
        }
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_fec_uncorrectable_counter_get(PHYMOD_ST* pc, phymod_fec_type_t fec_type, uint32_t* count)
{
    int start_lane, num_lane;
    RX_X4_FEC_UNCORR_CTR0r_t lower_reg;
    RX_X4_FEC_UNCORR_CTR1r_t upper_reg;
    BCMI_TSCBH_XGXS_RX_X4_FEC_UNCORRBLKSL0r_t low_reg; 
    BCMI_TSCBH_XGXS_RX_X4_FEC_UNCORRBLKSH0r_t high_reg; 

    PHYMOD_IF_ERR_RETURN
        (plp_aperta_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));

    if (fec_type == phymod_fec_CL74) {
        if (num_lane != 8) {
            pc->access.lane_mask = 1 << start_lane;
            PHYMOD_IF_ERR_RETURN(
                      READ_RX_X4_FEC_UNCORRBLKSL0r(pc, &low_reg));
            PHYMOD_IF_ERR_RETURN(
                      READ_RX_X4_FEC_UNCORRBLKSH0r(pc, &high_reg));
            *count = BCMI_TSCBH_XGXS_RX_X4_FEC_UNCORRBLKSH0r_UNCORCOUNTH_STREAM0f_GET(high_reg) << 16 |
                     BCMI_TSCBH_XGXS_RX_X4_FEC_UNCORRBLKSL0r_UNCORCOUNTL_STREAM0f_GET(low_reg);
        } else {
            pc->access.lane_mask = 0x1;
            PHYMOD_IF_ERR_RETURN(
                      READ_RX_X4_FEC_UNCORRBLKSL0r(pc, &low_reg));
            PHYMOD_IF_ERR_RETURN(
                      READ_RX_X4_FEC_UNCORRBLKSH0r(pc, &high_reg));
            *count = BCMI_TSCBH_XGXS_RX_X4_FEC_UNCORRBLKSH0r_UNCORCOUNTH_STREAM0f_GET(high_reg) << 16 |
                     BCMI_TSCBH_XGXS_RX_X4_FEC_UNCORRBLKSL0r_UNCORCOUNTL_STREAM0f_GET(low_reg);

            pc->access.lane_mask = 0x10;
            PHYMOD_IF_ERR_RETURN(
                      READ_RX_X4_FEC_UNCORRBLKSL0r(pc, &low_reg));
            PHYMOD_IF_ERR_RETURN(
                      READ_RX_X4_FEC_UNCORRBLKSH0r(pc, &high_reg));
            *count |= BCMI_TSCBH_XGXS_RX_X4_FEC_UNCORRBLKSH0r_UNCORCOUNTH_STREAM0f_GET(high_reg) << 16 |
                     BCMI_TSCBH_XGXS_RX_X4_FEC_UNCORRBLKSL0r_UNCORCOUNTL_STREAM0f_GET(low_reg);
        }
    } else {
        if (num_lane != 8) {
            pc->access.lane_mask = 1 << start_lane;
            PHYMOD_IF_ERR_RETURN(
                    READ_RX_X4_FEC_UNCORR_CTR0r(pc, &lower_reg));
            PHYMOD_IF_ERR_RETURN(
                    READ_RX_X4_FEC_UNCORR_CTR1r(pc, &upper_reg));
            *count = RX_X4_FEC_UNCORR_CTR1r_RS_FEC_FEC_UNCORR_CW_CNTR_UPPERf_GET(upper_reg) << 16 |
                RX_X4_FEC_UNCORR_CTR0r_RS_FEC_FEC_UNCORR_CW_CNTR_LOWERf_GET(lower_reg);
        } else {
            pc->access.lane_mask = 0x1;
            PHYMOD_IF_ERR_RETURN(
                    READ_RX_X4_FEC_UNCORR_CTR0r(pc, &lower_reg));
            PHYMOD_IF_ERR_RETURN(
                    READ_RX_X4_FEC_UNCORR_CTR1r(pc, &upper_reg));
            *count = RX_X4_FEC_UNCORR_CTR1r_RS_FEC_FEC_UNCORR_CW_CNTR_UPPERf_GET(upper_reg) << 16 |
                RX_X4_FEC_UNCORR_CTR0r_RS_FEC_FEC_UNCORR_CW_CNTR_LOWERf_GET(lower_reg);

            pc->access.lane_mask = 0x10;
            PHYMOD_IF_ERR_RETURN(
                    READ_RX_X4_FEC_UNCORR_CTR0r(pc, &lower_reg));
            PHYMOD_IF_ERR_RETURN(
                    READ_RX_X4_FEC_UNCORR_CTR1r(pc, &upper_reg));
            *count += RX_X4_FEC_UNCORR_CTR1r_RS_FEC_FEC_UNCORR_CW_CNTR_UPPERf_GET(upper_reg) << 16 |
                RX_X4_FEC_UNCORR_CTR0r_RS_FEC_FEC_UNCORR_CW_CNTR_LOWERf_GET(lower_reg);
        }
    }

    return PHYMOD_E_NONE;
}

/*
 * On TSCBH, SYNCE_X4 programming is per logical lane based.
 */
int plp_aperta_tbhmod_synce_mode_set(PHYMOD_ST* pc, int stage0_mode, int stage1_mode)
{
    INTEGER_DIVr_t INTEGER_DIVr_reg;

    INTEGER_DIVr_CLR(INTEGER_DIVr_reg);
    PHYMOD_IF_ERR_RETURN(READ_INTEGER_DIVr(pc, &INTEGER_DIVr_reg));
    INTEGER_DIVr_SYNCE_MODE_STAGE0f_SET(INTEGER_DIVr_reg, stage0_mode);
    INTEGER_DIVr_SYNCE_MODE_STAGE1f_SET(INTEGER_DIVr_reg, stage1_mode);

    PHYMOD_IF_ERR_RETURN(MODIFY_INTEGER_DIVr(pc, INTEGER_DIVr_reg));

    return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_synce_mode_get(PHYMOD_ST* pc, int* stage0_mode, int* stage1_mode)
{
    INTEGER_DIVr_t INTEGER_DIVr_reg;

    INTEGER_DIVr_CLR(INTEGER_DIVr_reg);
    PHYMOD_IF_ERR_RETURN(READ_INTEGER_DIVr(pc, &INTEGER_DIVr_reg));
    *stage0_mode = INTEGER_DIVr_SYNCE_MODE_STAGE0f_GET(INTEGER_DIVr_reg);
    *stage1_mode = INTEGER_DIVr_SYNCE_MODE_STAGE1f_GET(INTEGER_DIVr_reg);

    return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_synce_clk_ctrl_set(PHYMOD_ST* pc, uint32_t val)
{
    FRACTIONAL_DIVr_t FRACTIONAL_DIVr_reg;

    FRACTIONAL_DIVr_CLR(FRACTIONAL_DIVr_reg);
    PHYMOD_IF_ERR_RETURN(READ_FRACTIONAL_DIVr(pc, &FRACTIONAL_DIVr_reg));
    FRACTIONAL_DIVr_SET(FRACTIONAL_DIVr_reg, val);
    PHYMOD_IF_ERR_RETURN(MODIFY_FRACTIONAL_DIVr(pc, FRACTIONAL_DIVr_reg));

    return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_synce_clk_ctrl_get(PHYMOD_ST* pc, uint32_t* val)
{
    FRACTIONAL_DIVr_t FRACTIONAL_DIVr_reg;

    FRACTIONAL_DIVr_CLR(FRACTIONAL_DIVr_reg);
    PHYMOD_IF_ERR_RETURN(READ_FRACTIONAL_DIVr(pc, &FRACTIONAL_DIVr_reg));

    *val = FRACTIONAL_DIVr_GET(FRACTIONAL_DIVr_reg);

    return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_1588_tx_info_get(PHYMOD_ST* pc, aperta_tbhmod_ts_tx_info_t* ts_tx_info)
{
    TX_X4_TX_1588_TIMESTAMP_STSr_t reg_tx_1588_timestamp_sts;
    TX_X4_TX_1588_TIMESTAMP_HIr_t reg_tx_1588_timestamp_hi;
    TX_X4_TX_1588_TIMESTAMP_MIDr_t reg_tx_1588_timestamp_mid;
    TX_X4_TX_1588_TIMESTAMP_LOr_t reg_tx_1588_timestamp_lo;
    TX_X4_TX_TS_SEQ_IDr_t reg_tx_ts_seq_id;
    uint16_t ts_entry_valid;

    TX_X4_TX_1588_TIMESTAMP_STSr_CLR(reg_tx_1588_timestamp_sts);
    TX_X4_TX_1588_TIMESTAMP_HIr_CLR(reg_tx_1588_timestamp_hi);
    TX_X4_TX_1588_TIMESTAMP_LOr_CLR(reg_tx_1588_timestamp_lo);
    TX_X4_TX_TS_SEQ_IDr_CLR(reg_tx_ts_seq_id);

    PHYMOD_IF_ERR_RETURN(READ_TX_X4_TX_1588_TIMESTAMP_STSr(pc, &reg_tx_1588_timestamp_sts));
    ts_entry_valid = TX_X4_TX_1588_TIMESTAMP_STSr_TS_ENTRY_VALIDf_GET(reg_tx_1588_timestamp_sts);
    if (ts_entry_valid == 0) {
        return PHYMOD_E_FAIL;
    }

    PHYMOD_IF_ERR_RETURN(READ_TX_X4_TX_1588_TIMESTAMP_HIr(pc, &reg_tx_1588_timestamp_hi));
    PHYMOD_IF_ERR_RETURN(READ_TX_X4_TX_1588_TIMESTAMP_MIDr(pc, &reg_tx_1588_timestamp_mid));
    PHYMOD_IF_ERR_RETURN(READ_TX_X4_TX_1588_TIMESTAMP_LOr(pc, &reg_tx_1588_timestamp_lo));
    PHYMOD_IF_ERR_RETURN(READ_TX_X4_TX_TS_SEQ_IDr(pc, &reg_tx_ts_seq_id));

    ts_tx_info->ts_val_hi = TX_X4_TX_1588_TIMESTAMP_HIr_TS_IN_NS_HIf_GET(reg_tx_1588_timestamp_hi);
    ts_tx_info->ts_val_mid = TX_X4_TX_1588_TIMESTAMP_MIDr_TS_IN_NS_MIDf_GET(reg_tx_1588_timestamp_mid);
    ts_tx_info->ts_val_lo = TX_X4_TX_1588_TIMESTAMP_LOr_TS_IN_NS_LOf_GET(reg_tx_1588_timestamp_lo);
    ts_tx_info->ts_seq_id = TX_X4_TX_TS_SEQ_IDr_TS_SEQIDf_GET(reg_tx_ts_seq_id);
    ts_tx_info->ts_sub_nanosec = TX_X4_TX_1588_TIMESTAMP_STSr_TS_SUB_NSf_GET(reg_tx_1588_timestamp_sts);

    return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_pmd_rx_lock_override_enable(PHYMOD_ST* pc, uint32_t enable)
{
      PMD_X4_OVRRr_t pmd_x4_override;
      PMD_X4_OVRRr_CLR(pmd_x4_override);
      PHYMOD_IF_ERR_RETURN(READ_PMD_X4_OVRRr(pc, &pmd_x4_override));
      BCMI_TSCBH_XGXS_PMD_X4_OVRRr_RX_LOCK_OVRDf_SET(pmd_x4_override, enable);
      PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_OVRRr(pc, pmd_x4_override));
    return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_polling_for_sc_done(PHYMOD_ST* pc)
{
    int cnt;
    uint16_t sw_spd_chg_chk;
    SC_X4_STSr_t SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg;

    cnt = 0;

    SC_X4_STSr_CLR(SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg);
    /* wait for 5 second for the status bit to set */
    while (cnt <= 5000) {
        PHYMOD_IF_ERR_RETURN(READ_SC_X4_STSr(pc, &SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg));
        cnt = cnt + 1;
        sw_spd_chg_chk = SC_X4_STSr_SW_SPEED_CHANGE_DONEf_GET(SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg);

        if(sw_spd_chg_chk) {
            break;
        } else {
            if(cnt == 5000) {
                PHYMOD_DEBUG_ERROR(("WARNING :: speed change done bit is NOT set \n"));
                break;
            }
        }
        PHYMOD_USLEEP(1000);
    }
    return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_read_sc_done(PHYMOD_ST* pc)
{
    SC_X4_STSr_t SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg;

    SC_X4_STSr_CLR(SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_STSr(pc, &SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg));
    return PHYMOD_E_NONE;
}


int plp_aperta_tbhmod_read_sc_fsm_status(PHYMOD_ST* pc)
{

    SC_X4_DBGr_t debug_reg;

    SC_X4_DBGr_CLR(debug_reg);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_DBGr(pc, &debug_reg));
    return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_resolved_port_mode_get(PHYMOD_ST* pc, uint32_t* port_mode)
{
    int start_lane, num_lane;
    SC_X1_STSr_t reg_val;

  PHYMOD_IF_ERR_RETURN
      (plp_aperta_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));

    if (start_lane < 4) {
        pc->access.lane_mask = 0x1 << 0;
    } else {
        pc->access.lane_mask = 0x1 << 4;
    }

    PHYMOD_IF_ERR_RETURN(READ_SC_X1_STSr(pc, &reg_val));
    *port_mode = (uint32_t) SC_X1_STSr_RESOLVED_PORT_MODEf_GET(reg_val);

    return PHYMOD_E_NONE;
}

/* This function will read ECC status registers and return 1b and 2b error status.
 * All the status registers are CR registers, so the status bit will be cleared within
 * this function.
  */

int plp_aperta_tbhmod_fec_arch_decode_get(int fec_arch, phymod_fec_type_t* fec_type)
{
    switch (fec_arch) {
        case 1:
            *fec_type = phymod_fec_CL74;
            break;
        case 2:
            *fec_type = phymod_fec_CL91;
            break;
        case 3:
            *fec_type = phymod_fec_RS272;
            break;
        case 4:
            *fec_type = phymod_fec_RS544;
            break;
        case 5:
            *fec_type = phymod_fec_RS544_2XN;
            break;
        case 6:
            *fec_type = phymod_fec_RS272_2XN;
            break;
        default:
            *fec_type = phymod_fec_None;
            break;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_fec_override_get(PHYMOD_ST* pc, uint32_t* enable)
{
    SC_X4_SW_SPARE1r_t SC_X4_SW_SPARE1r_reg;
    uint16_t temp_reg_value;

    SC_X4_SW_SPARE1r_CLR(SC_X4_SW_SPARE1r_reg);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_SW_SPARE1r(pc, &SC_X4_SW_SPARE1r_reg));
    temp_reg_value = SC_X4_SW_SPARE1r_GET(SC_X4_SW_SPARE1r_reg);
    *enable = temp_reg_value & TSCBH_FEC_OVERRIDE_MASK;

    return PHYMOD_E_NONE;
}

/* this function  is to store port enable info */
int plp_aperta_tbhmod_port_enable_set(PHYMOD_ST* pc, uint32_t enable)
{
    SC_X4_SW_SPARE1r_t SC_X4_SW_SPARE1r_reg;
    uint16_t temp_reg_value;

    SC_X4_SW_SPARE1r_CLR(SC_X4_SW_SPARE1r_reg);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_SW_SPARE1r(pc, &SC_X4_SW_SPARE1r_reg));
    temp_reg_value = SC_X4_SW_SPARE1r_GET(SC_X4_SW_SPARE1r_reg);
    /* first clear the lowest bit */
    temp_reg_value  &= ~TSCBH_PORT_ENABLE_MASK;
    temp_reg_value |= (enable & 0x1) << TSCBH_PORT_ENABLE_BIT_SHIFT;
    SC_X4_SW_SPARE1r_SET(SC_X4_SW_SPARE1r_reg, temp_reg_value);
    PHYMOD_IF_ERR_RETURN(WRITE_SC_X4_SW_SPARE1r(pc, SC_X4_SW_SPARE1r_reg));

    return PHYMOD_E_NONE;
}

/* this function is to get port enable info */
int plp_aperta_tbhmod_port_enable_get(PHYMOD_ST* pc, uint32_t* enable)
{
    SC_X4_SW_SPARE1r_t SC_X4_SW_SPARE1r_reg;
    uint16_t temp_reg_value;

    SC_X4_SW_SPARE1r_CLR(SC_X4_SW_SPARE1r_reg);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_SW_SPARE1r(pc, &SC_X4_SW_SPARE1r_reg));
    temp_reg_value = SC_X4_SW_SPARE1r_GET(SC_X4_SW_SPARE1r_reg);
    *enable = (temp_reg_value & TSCBH_PORT_ENABLE_MASK) >> TSCBH_PORT_ENABLE_BIT_SHIFT;

    return PHYMOD_E_NONE;
}

/* This function will read ECC status registers and return 1b and 2b error status.
 * All the status registers are CR registers, so the status bit will be cleared within
 * this function.
 *
 * Return intr_status.err_addr as '0xffff' if the ECC error location information
 * is not applicable to the interrupt type.
 */
int plp_aperta_tbhmod_intr_status_get(PHYMOD_ST* pc, aperta_tbhmod_intr_status_t* intr_status)
{
    RX_X1_ECC_STS_BASE_R_FECr_t         RX_X1_ECC_STS_BASE_R_FECr_reg;
    RX_X1_ECC_STS_RSFEC_RBUF_MPPr_t     RX_X1_ECC_STS_RSFEC_RBUF_MPPr_reg;
    RX_X1_ECC_STS_RSFEC_RS400G_MPPr_t   RX_X1_ECC_STS_RSFEC_RS400G_MPPr_reg;
    RX_X1_ECC_STS_DESKEWr_t             RX_X1_ECC_STS_DESKEWr_reg;
    RX_X1_ECC_STS_SPD_TBLr_t            RX_X1_ECC_STS_SPD_TBLr_reg;
    RX_X1_ECC_STS_AM_TBLr_t             RX_X1_ECC_STS_AM_TBLr_reg;
    RX_X1_ECC_STS_UM_TBLr_t             RX_X1_ECC_STS_UM_TBLr_reg;
    RX_X1_ECC_STS_TX_1588_MPPr_t        RX_X1_ECC_STS_TX_1588_MPPr_reg;
    RX_X1_ECC_STS_TX_1588_400Gr_t       RX_X1_ECC_STS_TX_1588_400Gr_reg;
    RX_X1_ECC_STS_RX_1588_MPPr_t        RX_X1_ECC_STS_RX_1588_MPPr_reg;
    RX_X1_ECC_STS_RX_1588_400Gr_t       RX_X1_ECC_STS_RX_1588_400Gr_reg;
    PHYMOD_ST phy_copy;

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1;
    switch (intr_status->type) {
        case phymodIntrEccRx1588400g:
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_RX_1588_400Gr(&phy_copy, &RX_X1_ECC_STS_RX_1588_400Gr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_RX_1588_400Gr_TWO_BIT_ERR_EVENT_RX_1588_400Gf_GET(RX_X1_ECC_STS_RX_1588_400Gr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_RX_1588_400Gr_ONE_BIT_ERR_EVENT_RX_1588_400Gf_GET(RX_X1_ECC_STS_RX_1588_400Gr_reg);
            intr_status->err_addr = RX_X1_ECC_STS_RX_1588_400Gr_ERR_EVENT_ADDRESS_RX_1588_400Gf_GET(RX_X1_ECC_STS_RX_1588_400Gr_reg);
            break;
        case phymodIntrEccRx1588Mpp1:
            phy_copy.access.lane_mask = 0x1 << 4;
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_RX_1588_MPPr(&phy_copy, &RX_X1_ECC_STS_RX_1588_MPPr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_RX_1588_MPPr_TWO_BIT_ERR_EVENT_RX_1588_MPPf_GET(RX_X1_ECC_STS_RX_1588_MPPr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_RX_1588_MPPr_ONE_BIT_ERR_EVENT_RX_1588_MPPf_GET(RX_X1_ECC_STS_RX_1588_MPPr_reg);
            intr_status->err_addr = RX_X1_ECC_STS_RX_1588_MPPr_ERR_EVENT_ADDRESS_RX_1588_MPPf_GET(RX_X1_ECC_STS_RX_1588_MPPr_reg);
            break;
        case phymodIntrEccRx1588Mpp0:
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_RX_1588_MPPr(&phy_copy, &RX_X1_ECC_STS_RX_1588_MPPr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_RX_1588_MPPr_TWO_BIT_ERR_EVENT_RX_1588_MPPf_GET(RX_X1_ECC_STS_RX_1588_MPPr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_RX_1588_MPPr_ONE_BIT_ERR_EVENT_RX_1588_MPPf_GET(RX_X1_ECC_STS_RX_1588_MPPr_reg);
            intr_status->err_addr = RX_X1_ECC_STS_RX_1588_MPPr_ERR_EVENT_ADDRESS_RX_1588_MPPf_GET(RX_X1_ECC_STS_RX_1588_MPPr_reg);
            break;
        case phymodIntrEccTx1588400g:
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_TX_1588_400Gr(&phy_copy, &RX_X1_ECC_STS_TX_1588_400Gr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_TX_1588_400Gr_TWO_BIT_ERR_EVENT_TX_1588_400Gf_GET(RX_X1_ECC_STS_TX_1588_400Gr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_TX_1588_400Gr_ONE_BIT_ERR_EVENT_TX_1588_400Gf_GET(RX_X1_ECC_STS_TX_1588_400Gr_reg);
            intr_status->err_addr = RX_X1_ECC_STS_TX_1588_400Gr_ERR_EVENT_ADDRESS_TX_1588_400Gf_GET(RX_X1_ECC_STS_TX_1588_400Gr_reg);
            break;
        case phymodIntrEccTx1588Mpp1:
            phy_copy.access.lane_mask = 0x1 << 4;
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_TX_1588_MPPr(&phy_copy, &RX_X1_ECC_STS_TX_1588_MPPr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_TX_1588_MPPr_TWO_BIT_ERR_EVENT_TX_1588_MPPf_GET(RX_X1_ECC_STS_TX_1588_MPPr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_TX_1588_MPPr_ONE_BIT_ERR_EVENT_TX_1588_MPPf_GET(RX_X1_ECC_STS_TX_1588_MPPr_reg);
            intr_status->err_addr = RX_X1_ECC_STS_TX_1588_MPPr_ERR_EVENT_ADDRESS_TX_1588_MPPf_GET(RX_X1_ECC_STS_TX_1588_MPPr_reg);
            break;
        case phymodIntrEccTx1588Mpp0:
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_TX_1588_MPPr(&phy_copy, &RX_X1_ECC_STS_TX_1588_MPPr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_TX_1588_MPPr_TWO_BIT_ERR_EVENT_TX_1588_MPPf_GET(RX_X1_ECC_STS_TX_1588_MPPr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_TX_1588_MPPr_ONE_BIT_ERR_EVENT_TX_1588_MPPf_GET(RX_X1_ECC_STS_TX_1588_MPPr_reg);
            intr_status->err_addr = RX_X1_ECC_STS_TX_1588_MPPr_ERR_EVENT_ADDRESS_TX_1588_MPPf_GET(RX_X1_ECC_STS_TX_1588_MPPr_reg);
            break;
        case phymodIntrEccUMTable:
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_UM_TBLr(&phy_copy, &RX_X1_ECC_STS_UM_TBLr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_UM_TBLr_TWO_BIT_ERR_EVENT_UM_TBLf_GET(RX_X1_ECC_STS_UM_TBLr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_UM_TBLr_ONE_BIT_ERR_EVENT_UM_TBLf_GET(RX_X1_ECC_STS_UM_TBLr_reg);
            intr_status->err_addr = RX_X1_ECC_STS_UM_TBLr_ERR_EVENT_ADDRESS_UM_TBLf_GET(RX_X1_ECC_STS_UM_TBLr_reg);
            break;
        case phymodIntrEccAMTable:
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_AM_TBLr(&phy_copy, &RX_X1_ECC_STS_AM_TBLr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_AM_TBLr_TWO_BIT_ERR_EVENT_AM_TBLf_GET(RX_X1_ECC_STS_AM_TBLr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_AM_TBLr_ONE_BIT_ERR_EVENT_AM_TBLf_GET(RX_X1_ECC_STS_AM_TBLr_reg);
            intr_status->err_addr = RX_X1_ECC_STS_AM_TBLr_ERR_EVENT_ADDRESS_AM_TBLf_GET(RX_X1_ECC_STS_AM_TBLr_reg);
            break;
        case phymodIntrEccSpeedTable:
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_SPD_TBLr(&phy_copy, &RX_X1_ECC_STS_SPD_TBLr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_SPD_TBLr_TWO_BIT_ERR_EVENT_SPD_TBLf_GET(RX_X1_ECC_STS_SPD_TBLr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_SPD_TBLr_ONE_BIT_ERR_EVENT_SPD_TBLf_GET(RX_X1_ECC_STS_SPD_TBLr_reg);
            intr_status->err_addr = RX_X1_ECC_STS_SPD_TBLr_ERR_EVENT_ADDRESS_SPD_TBLf_GET(RX_X1_ECC_STS_SPD_TBLr_reg);
            break;
        case phymodIntrEccDeskew:
            /* Deskew memory fake ECC error workaround:
             * Fake ECC error might happen in deskew memory during link transition.
             * To get the valid ECC error status, we first read the status register
             * to clear any potential fake ECC intrrupt status. Then wait for 10us
             * for a valid ECC error to be reflected in the status regsiter.
             */
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_DESKEWr(&phy_copy, &RX_X1_ECC_STS_DESKEWr_reg));
            PHYMOD_USLEEP(10);
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_DESKEWr(&phy_copy, &RX_X1_ECC_STS_DESKEWr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_DESKEWr_TWO_BIT_ERR_EVENT_DESKEWf_GET(RX_X1_ECC_STS_DESKEWr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_DESKEWr_ONE_BIT_ERR_EVENT_DESKEWf_GET(RX_X1_ECC_STS_DESKEWr_reg);
            intr_status->err_addr = 0xffff;
            break;
        case phymodIntrEccRsFECRs400gMpp1:
            phy_copy.access.lane_mask = 0x1 << 4;
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_RSFEC_RS400G_MPPr(&phy_copy, &RX_X1_ECC_STS_RSFEC_RS400G_MPPr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_RSFEC_RS400G_MPPr_TWO_BIT_ERR_EVENT_RSFEC_RS400G_MPPf_GET(RX_X1_ECC_STS_RSFEC_RS400G_MPPr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_RSFEC_RS400G_MPPr_ONE_BIT_ERR_EVENT_RSFEC_RS400G_MPPf_GET(RX_X1_ECC_STS_RSFEC_RS400G_MPPr_reg);
            intr_status->err_addr = 0xffff;
            break;
        case phymodIntrEccRsFECRs400gMpp0:
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_RSFEC_RS400G_MPPr(&phy_copy, &RX_X1_ECC_STS_RSFEC_RS400G_MPPr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_RSFEC_RS400G_MPPr_TWO_BIT_ERR_EVENT_RSFEC_RS400G_MPPf_GET(RX_X1_ECC_STS_RSFEC_RS400G_MPPr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_RSFEC_RS400G_MPPr_ONE_BIT_ERR_EVENT_RSFEC_RS400G_MPPf_GET(RX_X1_ECC_STS_RSFEC_RS400G_MPPr_reg);
            intr_status->err_addr = 0xffff;
            break;
        case phymodIntrEccRsFECRbufMpp1:
            phy_copy.access.lane_mask = 0x1 << 4;
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_RSFEC_RBUF_MPPr(&phy_copy, &RX_X1_ECC_STS_RSFEC_RBUF_MPPr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_RSFEC_RBUF_MPPr_TWO_BIT_ERR_EVENT_RSFEC_RBUF_MPPf_GET(RX_X1_ECC_STS_RSFEC_RBUF_MPPr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_RSFEC_RBUF_MPPr_ONE_BIT_ERR_EVENT_RSFEC_RBUF_MPPf_GET(RX_X1_ECC_STS_RSFEC_RBUF_MPPr_reg);
            intr_status->err_addr = 0xffff;
            break;
        case phymodIntrEccRsFECRbufMpp0:
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_RSFEC_RBUF_MPPr(&phy_copy, &RX_X1_ECC_STS_RSFEC_RBUF_MPPr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_RSFEC_RBUF_MPPr_TWO_BIT_ERR_EVENT_RSFEC_RBUF_MPPf_GET(RX_X1_ECC_STS_RSFEC_RBUF_MPPr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_RSFEC_RBUF_MPPr_ONE_BIT_ERR_EVENT_RSFEC_RBUF_MPPf_GET(RX_X1_ECC_STS_RSFEC_RBUF_MPPr_reg);
            intr_status->err_addr = 0xffff;
            break;
        case phymodIntrEccBaseRFEC:
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_BASE_R_FECr(&phy_copy, &RX_X1_ECC_STS_BASE_R_FECr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_BASE_R_FECr_TWO_BIT_ERR_EVENT_BASE_R_FECf_GET(RX_X1_ECC_STS_BASE_R_FECr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_BASE_R_FECr_ONE_BIT_ERR_EVENT_BASE_R_FECf_GET(RX_X1_ECC_STS_BASE_R_FECr_reg);
            intr_status->err_addr = 0xffff;
            break;
        default:
            return PHYMOD_E_PARAM;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta_tbhmod_400g_autoneg_timer_disable(PHYMOD_ST* pc, int timer_disable)
{
    AN_X1_LNK_FAIL_INHBT_TMR_CL72r_t link_fail_cl72_timer_reg;
    AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_t link_fail_not_cl72_timer_reg;
    AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_t link_fail_cl72_pam4_timer_reg;
    AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_t link_fail_not_cl72_pam4_timer_reg;
    PHYMOD_ST phy_copy;

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));
    phy_copy.access.lane_mask = 1;


    if (timer_disable) {
        /* 0x9255 0 */
        AN_X1_LNK_FAIL_INHBT_TMR_CL72r_CLR(link_fail_cl72_timer_reg);
        AN_X1_LNK_FAIL_INHBT_TMR_CL72r_SET(link_fail_cl72_timer_reg, 0x0);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_CL72r(&phy_copy, link_fail_cl72_timer_reg));
        /* 0x9256 0 */
        AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_CLR(link_fail_not_cl72_timer_reg);
        AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_SET(link_fail_not_cl72_timer_reg, 0x0);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r(&phy_copy, link_fail_not_cl72_timer_reg));

        /*0x925a 0 */
        AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_CLR(link_fail_cl72_pam4_timer_reg);
        AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_SET(link_fail_cl72_pam4_timer_reg, 0x0);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r(&phy_copy, link_fail_cl72_pam4_timer_reg));
        /*0x925b 0 */
        AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_CLR(link_fail_not_cl72_pam4_timer_reg);
        AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_SET(link_fail_not_cl72_pam4_timer_reg, 0x0);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r(&phy_copy, link_fail_not_cl72_pam4_timer_reg));
    } else { /* need to restore the timer value */

        /* 0x9255 500.04 ms */
        AN_X1_LNK_FAIL_INHBT_TMR_CL72r_CLR(link_fail_cl72_timer_reg);
        /*AN_X1_LNK_FAIL_INHBT_TMR_CL72r_SET(link_fail_cl72_timer_reg, 0x8236);*/
        AN_X1_LNK_FAIL_INHBT_TMR_CL72r_SET(link_fail_cl72_timer_reg, 0x208e);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_CL72r(&phy_copy, link_fail_cl72_timer_reg));
        /* 0x9256 160.98 ms */
        AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_CLR(link_fail_not_cl72_timer_reg);
        /*AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_SET(link_fail_not_cl72_timer_reg, 0x29ab);*/
        AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_SET(link_fail_not_cl72_timer_reg, 0xa6c);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r(&phy_copy, link_fail_not_cl72_timer_reg));

        /*0x925a 3199.9 ms */
        AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_CLR(link_fail_cl72_pam4_timer_reg);
        /*AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_SET(link_fail_cl72_pam4_timer_reg, 0xc238);*/
        AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_SET(link_fail_cl72_pam4_timer_reg, 0x3415);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r(&phy_copy, link_fail_cl72_pam4_timer_reg));
        /*0x925b 1000.08 ms */
        AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_CLR(link_fail_not_cl72_pam4_timer_reg);
        /*AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_SET(link_fail_not_cl72_pam4_timer_reg, 0x41aa);*/
        AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_SET(link_fail_not_cl72_pam4_timer_reg, 0x106a);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r(&phy_copy, link_fail_not_cl72_pam4_timer_reg));

    }
    return PHYMOD_E_NONE;
}

#ifdef _DV_AN_NEWCODE_
#include "aperta_tbhmod_an_newcode_onlyfordv.c"
#endif /* _DV_AN_NEWCODE_ */




