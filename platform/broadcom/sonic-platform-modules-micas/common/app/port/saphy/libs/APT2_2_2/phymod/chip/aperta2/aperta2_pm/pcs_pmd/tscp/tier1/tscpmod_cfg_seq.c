/*----------------------------------------------------------------------
 * $Id: tscpmod_cfg_seq.c,v 1.8 2013/01/17 19:47:56 udayc Exp $
 * $Copyright: $
 *
 * $Copyright: (c) 2014 Broadcom Corporation All Rights Reserved.$
 * Broadcom Corporation
 * Proprietary and Confidential information
 * All rights reserved
 * This source file is the property of Broadcom Corporation, and
 * may not be copied or distributed in any isomorphic form without the
 * prior written consent of Broadcom Corporation.
 *---------------------------------------------------------------------
 * File       : tscpmod_cfg_seq.c
 * Description: c functions implementing Tier1s for TSCPMOD Serdes Driver
 *---------------------------------------------------------------------*/

#include <phymod/phymod.h>
#include <phymod/phymod_system.h>
#include <phymod/phymod_util.h>
#include <phymod/phymod_debug.h>
#include <bcmi_tscp_xgxs_defs.h>
#include <bcmi_peregrine5_pc_xgxs_defs.h>
#include <tscp/tier1/tscpmod.h>
#include <tscp/tier1/tscpmod_sc_lkup_table.h>

#define NOCODE 0

/*
 * Forward declarations:
*/
int _plp_aperta2_tscpmod_an_ability_construct(phymod_autoneg_advert_ability_t* an_ability,
                          uint32_t speed,
                          int num_lanes,
                          phymod_fec_type_t fec,
                          uint32_t pause,
                          int channel,
                          plp_aperta2_phymod_an_mode_type_t an_mode);
float _plp_aperta2_tscpmod_pcs_vco_to_clk_period(uint32_t vco, int os_mode, int pam4);
uint32_t _plp_aperta2_tscpmod_pcs_vco_to_ui(uint32_t vco, int os_mode, int pam4);
float _plp_aperta2_tscpmod_pcs_calc_bit_ui_value(uint32_t vco, int os_mode, int pam4);
float _plp_aperta2_tscpmod_pcs_vco_to_tx_clk_period(uint32_t vco, int os_mode, int pam4);
int _plp_aperta2_tscpmod_calc_tx_pmd_latency(uint32_t vco, uint32_t os_mode, int pam4);
int _plp_aperta2_tscpmod_update_tx_pmd_latency(PHYMOD_ST *pc, uint32_t latency_adj, int normalize_to_latest);
int _plp_aperta2_tscpmod_virtual_lane_count_get(int bit_mux_mode, int num_lane, int *virtual_lanes, int *num_psll_per_phyl);
int _plp_aperta2_tscpmod_timesync_rx_deskew_info_get (PHYMOD_ST *pc, int bit_mux_mode, uint32_t vco,
                                          int os_mode, int is_pam4,
                                          uint32_t *rx_max_skew_vl, uint32_t *rx_min_skew_vl,
                                          uint32_t *skew_per_vl, uint32_t *vl_to_pl_map,
                                          uint32_t *vl_to_pl_off_map);

/* This function will return model num for the core */
int plp_aperta2_tscpmod_model_num_get(PHYMOD_ST* pc, uint32_t* model_num)
{
    MAIN0_SERDESIDr_t MAIN0_SERDESIDr_reg;

    PHYMOD_IF_ERR_RETURN(READ_MAIN0_SERDESIDr(pc, &MAIN0_SERDESIDr_reg));
    *model_num = MAIN0_SERDESIDr_MODEL_NUMBERf_GET(MAIN0_SERDESIDr_reg);

    return PHYMOD_E_NONE;
}


/*!
@brief   get  port speed id configured
@param   pc handle to current TSCBH context (#tbhmod_st)
@param   speed_id Receives the resolved speed cfg in the speed_id
@returns The value PHYMOD_E_NONE upon successful completion.
@details get  port speed configured
*/
int plp_aperta2_tscpmod_speed_id_get(PHYMOD_ST* pc, int *speed_id)
{
    SC_X4_RSLVD_SPDr_t sc_final_resolved_speed;

    SC_X4_RSLVD_SPDr_CLR(sc_final_resolved_speed);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_RSLVD_SPDr(pc,&sc_final_resolved_speed));
    *speed_id = SC_X4_RSLVD_SPDr_SPEEDf_GET(sc_final_resolved_speed);

    return PHYMOD_E_NONE;
}


/**
@brief   Get the Port status
@param   pc handle to current TSCBH context (#tbhmod_st)
@param   disabled  Receives status on port disabledness
@returns The value PHYMOD_E_NONE upon successful completion
@details Ports can be disabled in several ways. In this function we simply write
0 to the speed change which will bring the PCS down for that lane.

*/
int plp_aperta2_tscpmod_enable_get(PHYMOD_ST* pc, uint32_t* enable)
{
    SC_X4_CTLr_t reg_sc_ctrl;

    SC_X4_CTLr_CLR(reg_sc_ctrl);

    PHYMOD_IF_ERR_RETURN(READ_SC_X4_CTLr(pc,&reg_sc_ctrl));
    *enable = SC_X4_CTLr_SW_SPEED_CHANGEf_GET(reg_sc_ctrl);

    return PHYMOD_E_NONE;
}
/**
@brief   Get info on Disable status of the Port
@param   pc handle to current TSCBH context (#tbhmod_st)
@returns The value PHYMOD_E_NONE upon successful completion
@details Disables the port by writing 0 to the speed config logic in PCS.
This makes the PCS to bring down the PCS blocks and also apply lane datapath
reset to the PMD. There is no control input to this function since it only does
one thing.
*/
int plp_aperta2_tscpmod_disable_set(PHYMOD_ST* pc)
{
    SC_X4_CTLr_t reg_sc_ctrl;

    SC_X4_CTLr_CLR(reg_sc_ctrl);

    PHYMOD_IF_ERR_RETURN(READ_SC_X4_CTLr(pc, &reg_sc_ctrl));
    SC_X4_CTLr_SW_SPEED_CHANGEf_SET(reg_sc_ctrl, 0);
    PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(pc,reg_sc_ctrl));

    return PHYMOD_E_NONE;
}


int plp_aperta2_tscpmod_enable_set(PHYMOD_ST* pc)
{
    SC_X4_CTLr_t reg_sc_ctrl;

    SC_X4_CTLr_CLR(reg_sc_ctrl);

    PHYMOD_IF_ERR_RETURN(READ_SC_X4_CTLr(pc, &reg_sc_ctrl));
    SC_X4_CTLr_SW_SPEED_CHANGEf_SET(reg_sc_ctrl, 1);
    PHYMOD_IF_ERR_RETURN(MODIFY_SC_X4_CTLr(pc,reg_sc_ctrl));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_set_an_timers(PHYMOD_ST* pc, plp_aperta2_phymod_ref_clk_t refclk, uint32_t *pam4_an_timer_value)    /* SET_AN_TIMERS */
{
    AN_X1_IGNORE_LNK_TMRr_t AN_X1_TIMERS_IGNORE_LINK_TIMERr_reg;
    RX_X4_RS_FEC_TMRr_t RX_X4_RS_FEC_TMRr_reg;
    MAIN0_TICK_CTL0r_t MAIN0_TICK_CONTROL_0r_reg;
    MAIN0_TICK_CTL1r_t MAIN0_TICK_CONTROL_1r_reg;


    AN_X1_CL73_ERRr_t  AN_X1_CL73_ERRr_reg;
    AN_X1_CL73_BRK_LNKr_t AN_X1_TIMERS_CL73_BREAK_LINKr_reg;
    AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg;
    AN_X1_LNK_FAIL_INHBT_TMR_CL72r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg;

    AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg;
    AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg;
    AN_X1_IGNORE_LNK_TMR_PAM4r_t AN_X1_IGNORE_LNK_TMR_PAM4r_reg;


    /* need to set the tick count from 15 us to 60us */
    MAIN0_TICK_CTL0r_CLR(MAIN0_TICK_CONTROL_0r_reg);
    PHYMOD_IF_ERR_RETURN (READ_MAIN0_TICK_CTL0r(pc, &MAIN0_TICK_CONTROL_0r_reg));
    MAIN0_TICK_CTL0r_TICK_DENOMINATORf_SET(MAIN0_TICK_CONTROL_0r_reg, 1);
    MAIN0_TICK_CTL0r_TICK_NUMERATOR_LOWERf_SET(MAIN0_TICK_CONTROL_0r_reg, 0xe);

    PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_TICK_CTL0r(pc, MAIN0_TICK_CONTROL_0r_reg));

    MAIN0_TICK_CTL1r_CLR(MAIN0_TICK_CONTROL_1r_reg);
    PHYMOD_IF_ERR_RETURN (READ_MAIN0_TICK_CTL1r(pc, &MAIN0_TICK_CONTROL_1r_reg));
    MAIN0_TICK_CTL1r_TICK_OVERRIDEf_SET(MAIN0_TICK_CONTROL_1r_reg, 1);
    MAIN0_TICK_CTL1r_TICK_NUMERATOR_UPPERf_SET(MAIN0_TICK_CONTROL_1r_reg, 0x493);
    PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_TICK_CTL1r(pc, MAIN0_TICK_CONTROL_1r_reg));


    /* first program NRZ speed related timer value */
    /*0x9250  65.04 ms */
    AN_X1_CL73_BRK_LNKr_CLR(AN_X1_TIMERS_CL73_BREAK_LINKr_reg);
    /* AN_X1_CL73_BRK_LNKr_SET(AN_X1_TIMERS_CL73_BREAK_LINKr_reg, 0x10f0); */
    AN_X1_CL73_BRK_LNKr_SET(AN_X1_TIMERS_CL73_BREAK_LINKr_reg, 0x43c);
    PHYMOD_IF_ERR_RETURN (WRITE_AN_X1_CL73_BRK_LNKr(pc, AN_X1_TIMERS_CL73_BREAK_LINKr_reg));
    /* 0x9251  25.02 ms */
    AN_X1_CL73_ERRr_CLR(AN_X1_CL73_ERRr_reg);
    AN_X1_CL73_ERRr_SET(AN_X1_CL73_ERRr_reg, 0x0);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_CL73_ERRr(pc, AN_X1_CL73_ERRr_reg));
    /* 0x9254 10.02 ms */
    AN_X1_IGNORE_LNK_TMRr_CLR(AN_X1_TIMERS_IGNORE_LINK_TIMERr_reg);
    /* AN_X1_IGNORE_LNK_TMRr_SET(AN_X1_TIMERS_IGNORE_LINK_TIMERr_reg, 0x29c); */
    AN_X1_IGNORE_LNK_TMRr_SET(AN_X1_TIMERS_IGNORE_LINK_TIMERr_reg, 0xa7);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_IGNORE_LNK_TMRr(pc, AN_X1_TIMERS_IGNORE_LINK_TIMERr_reg));
    /* 0x9255 609.04 ms */
    AN_X1_LNK_FAIL_INHBT_TMR_CL72r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg);
    /*AN_X1_LNK_FAIL_INHBT_TMR_CL72r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg, 0x9eda);*/
    AN_X1_LNK_FAIL_INHBT_TMR_CL72r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg, 0x27b6);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_CL72r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72r_reg));
    /* 0x9256 160.98 ms */
    AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg);
    /* AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg, 0x29ab); */
    AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg, 0xa6a);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72r_reg));
    /* 0xc130 60 ms */
    RX_X4_RS_FEC_TMRr_CLR(RX_X4_RS_FEC_TMRr_reg);
    /* RX_X2_RS_FEC_TMRr_SET(RX_X2_RS_FEC_TMRr_reg, 0xfa0); */
    RX_X4_RS_FEC_TMRr_SET(RX_X4_RS_FEC_TMRr_reg, 0x3e8);
    PHYMOD_IF_ERR_RETURN(WRITE_RX_X4_RS_FEC_TMRr(pc, RX_X4_RS_FEC_TMRr_reg));

    /*next program PAM4 related timer */
    if (pam4_an_timer_value == NULL) {
        /*then default value */
        /* 0x9259 10.08 ms */
        AN_X1_IGNORE_LNK_TMR_PAM4r_CLR(AN_X1_IGNORE_LNK_TMR_PAM4r_reg);
        /* AN_X1_IGNORE_LNK_TMR_PAM4r_SET(AN_X1_IGNORE_LNK_TMR_PAM4r_reg, 0xa8); */
        AN_X1_IGNORE_LNK_TMR_PAM4r_SET(AN_X1_IGNORE_LNK_TMR_PAM4r_reg, 0x2a);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_IGNORE_LNK_TMR_PAM4r(pc, AN_X1_IGNORE_LNK_TMR_PAM4r_reg));
        /*0x925a 3109.2 * 4 = 12436 ms */
        AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg);
        AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg, 0xca79);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg));
        /*0x925b 1109.08 ms */
        AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg);
        /* AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg, 0x4844); */
        AN_X1_LNK_FAIL_INHBT_TMR_NOT_CL72_PAM4r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_NOT_CL72_PAM4r_reg, 0x1211);
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

int plp_aperta2_tscpmod_an_link_fail_inhibit_timer_set(PHYMOD_ST* pc, tscpmod_an_timer_t timer_type, uint32_t period)
{
    AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg;
    uint16_t tick_count;

    switch (timer_type) {
        case TSCPMOD_AN_FAIL_INHIBIT_TIMER_LT_PAM4:
            /*one tick indicates 60us. The granularity is 4 ticks--  240us*/
            tick_count = period * 1000 / (60 * 4);
            AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg);
            AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_SET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg, tick_count);
            PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r(pc, AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg));
            break;
        default:
            return PHYMOD_E_PARAM;
    }

    return PHYMOD_E_NONE;
}

/*The unit of period is ms*/
int plp_aperta2_tscpmod_an_link_fail_inhibit_timer_get(PHYMOD_ST* pc, tscpmod_an_timer_t timer_type, uint32_t* period)
{
    AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_t AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg;
    uint16_t tick_count;

    switch (timer_type) {
        case TSCPMOD_AN_FAIL_INHIBIT_TIMER_LT_PAM4:
            /*one tick indicates 60us. The granularity is 4 ticks--  240us*/
            AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_CLR(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg);
            PHYMOD_IF_ERR_RETURN(READ_AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r(pc, &AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg));
            tick_count = AN_X1_LNK_FAIL_INHBT_TMR_CL72_PAM4r_GET(AN_X1_TIMERS_LINK_FAIL_INHIBIT_TIMER_CL72_PAM4r_reg);
            *period = tick_count * 4 * 60 / 1000;
            if (tick_count * 4 * 60 % 1000) {
                (*period) += 1;
            }
            break;
        default:
            return PHYMOD_E_PARAM;
    }

    return PHYMOD_E_NONE;
}


int plp_aperta2_tscpmod_get_pcs_latched_link_status(PHYMOD_ST* pc, uint32_t *link)
{
    RX_X4_PCS_LATCH_STS1r_t latched_sts ;

    RX_X4_PCS_LATCH_STS1r_CLR(latched_sts) ;

    PHYMOD_IF_ERR_RETURN (READ_RX_X4_PCS_LATCH_STS1r(pc, &latched_sts));
    *link = RX_X4_PCS_LATCH_STS1r_PCS_LINK_STATUS_LIVEf_GET(latched_sts);

    return PHYMOD_E_NONE;
}


/**
@brief   RSFEC latency Bit selection
@param   pc handle to current TSCBH context (#tbhmod_st)
@returns The value PHYMOD_E_NONE upon successful completion
@details assert/deassert the cw_latency bit to support 544 and 528 fecs in same MPP
*/
int plp_aperta2_tscpmod_rsfec_cw_type_set(PHYMOD_ST* pc, tscpmod_rs_fec_cw_type  cw_type, int fec_bypass_correction)              /* PMD_X4_RESET */
{
    RX_X1_RS_FEC_CFGr_t reg_rsfec_cfg;


    RX_X1_RS_FEC_CFGr_CLR(reg_rsfec_cfg );
    PHYMOD_IF_ERR_RETURN (READ_RX_X1_RS_FEC_CFGr(pc, &reg_rsfec_cfg));
    /* RX_X1_RS_FEC_CFGr_RS_FEC_CW_TYPEf_SET(reg_rsfec_cfg, cw_type); */
    if (fec_bypass_correction == 1) {
        RX_X1_RS_FEC_CFGr_RS_FEC_CORRECTION_ENABLEf_SET(reg_rsfec_cfg,~fec_bypass_correction);
    }
    PHYMOD_IF_ERR_RETURN (MODIFY_RX_X1_RS_FEC_CFGr(pc, reg_rsfec_cfg));

    return PHYMOD_E_NONE;
}

/**
@brief   OSTS Pipeline enable Bit selection
@param   pc handle to current TSCBH context (#tbhmod_st)
@returns The value PHYMOD_E_NONE upon successful completion
@details enable/disable One Step pipeline
*/

int plp_aperta2_tscpmod_osts_pipeline(PHYMOD_ST* pc, uint32_t en)
{
    TX_X4_TX_TS_CTLr_t    reg_tx_x4_ts_ctlr;

    TX_X4_TX_TS_CTLr_CLR(reg_tx_x4_ts_ctlr);
    PHYMOD_IF_ERR_RETURN (READ_TX_X4_TX_TS_CTLr(pc, &reg_tx_x4_ts_ctlr));
    TX_X4_TX_TS_CTLr_OSTS_PIPELINE_ENABLEf_SET(reg_tx_x4_ts_ctlr, en);

    PHYMOD_IF_ERR_RETURN (MODIFY_TX_X4_TX_TS_CTLr(pc, reg_tx_x4_ts_ctlr));
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_osts_pipeline_get(PHYMOD_ST* pc, uint32_t* en)
{
    TX_X4_TX_TS_CTLr_t reg_tx_x4_ts_ctlr;

    TX_X4_TX_TS_CTLr_CLR(reg_tx_x4_ts_ctlr);
    PHYMOD_IF_ERR_RETURN(READ_TX_X4_TX_TS_CTLr(pc, &reg_tx_x4_ts_ctlr));

    *en = TX_X4_TX_TS_CTLr_OSTS_PIPELINE_ENABLEf_GET(reg_tx_x4_ts_ctlr);

    return PHYMOD_E_NONE;
}


/*!
 @brief Rx laneswap per Mpp.
@param  unit number for instance lane number for decide which lane
@param rx_lane_swap value tells phy to log lanes mapping
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Rx lane swap */
int plp_aperta2_tscpmod_pcs_rx_lane_swap(PHYMOD_ST* pc, int rx_lane_swap)
{
    uint8_t rx_lane_map_physical[8], lane;
    RX_X1_RX_LN_SWPr_t    RX_X1_RX_LN_SWPr_reg;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    /*first we need to translate logical ln based on physical lane based lane ma-*/
    for (lane = 0; lane < TSCPMOD_NOF_LANES_IN_CORE; lane++){
        rx_lane_map_physical[((rx_lane_swap >> (lane*4)) & 0xf)] = lane;
    }

    /*first for physcial lane 0 through 3, use mmp0 */
    RX_X1_RX_LN_SWPr_CLR(RX_X1_RX_LN_SWPr_reg);
    PHYMOD_IF_ERR_RETURN(READ_RX_X1_RX_LN_SWPr(pc, &RX_X1_RX_LN_SWPr_reg));
    pc->access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);;
    RX_X1_RX_LN_SWPr_PHYSICAL3_TO_LOGICAL_SELf_SET(RX_X1_RX_LN_SWPr_reg,(rx_lane_map_physical[3] & 0x7));
    RX_X1_RX_LN_SWPr_PHYSICAL2_TO_LOGICAL_SELf_SET(RX_X1_RX_LN_SWPr_reg,(rx_lane_map_physical[2] & 0x7));
    RX_X1_RX_LN_SWPr_PHYSICAL1_TO_LOGICAL_SELf_SET(RX_X1_RX_LN_SWPr_reg,(rx_lane_map_physical[1] & 0x7));
    RX_X1_RX_LN_SWPr_PHYSICAL0_TO_LOGICAL_SELf_SET(RX_X1_RX_LN_SWPr_reg,(rx_lane_map_physical[0] & 0x7));
    PHYMOD_IF_ERR_RETURN(MODIFY_RX_X1_RX_LN_SWPr(pc, RX_X1_RX_LN_SWPr_reg));

    RX_X1_RX_LN_SWPr_CLR(RX_X1_RX_LN_SWPr_reg);
    PHYMOD_IF_ERR_RETURN(READ_RX_X1_RX_LN_SWPr(pc, &RX_X1_RX_LN_SWPr_reg));
    pc->access.lane_mask = 0x1 << (4 + PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
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
int plp_aperta2_tscpmod_pcs_tx_lane_swap(PHYMOD_ST* pc, int tx_lane_swap)
{
    uint8_t tx_lane_map_physical[8], lane;
    TX_X1_TX_LN_SWPr_t TX_X1_TX_LN_SWPr_reg;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    /*first we need to translate logical ln based on physical lane based lane ma-*/
    for (lane = 0; lane < TSCPMOD_NOF_LANES_IN_CORE; lane++){
        tx_lane_map_physical[((tx_lane_swap >> (lane*4)) & 0xf)] = lane;
    }

    /*for the physical 0 through 3 using MPP0 */
    TX_X1_TX_LN_SWPr_CLR(TX_X1_TX_LN_SWPr_reg);
    pc->access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    PHYMOD_IF_ERR_RETURN(READ_TX_X1_TX_LN_SWPr(pc, &TX_X1_TX_LN_SWPr_reg));
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL0_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[0] & 0x7));
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL1_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[1] & 0x7));
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL2_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[2] & 0x7));
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL3_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[3] & 0x7));
    PHYMOD_IF_ERR_RETURN(MODIFY_TX_X1_TX_LN_SWPr(pc, TX_X1_TX_LN_SWPr_reg));


    /*for the physical 4 through 7 using MPP1 */
    TX_X1_TX_LN_SWPr_CLR(TX_X1_TX_LN_SWPr_reg);
    pc->access.lane_mask = 0x1 << (4+ PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
    PHYMOD_IF_ERR_RETURN(READ_TX_X1_TX_LN_SWPr(pc, &TX_X1_TX_LN_SWPr_reg));
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL0_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[4] & 0x7));
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL1_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[5] & 0x7));
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL2_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[6] & 0x7));
    TX_X1_TX_LN_SWPr_LOGICAL_TO_PHYSICAL3_SELf_SET(TX_X1_TX_LN_SWPr_reg, (tx_lane_map_physical[7] & 0x7));
    PHYMOD_IF_ERR_RETURN(MODIFY_TX_X1_TX_LN_SWPr(pc, TX_X1_TX_LN_SWPr_reg));

    return PHYMOD_E_NONE;
}


/*!
 *  @brief rx_scrambleidle_en per port.
@param  unit number for instance lane number for decide which lane
@param en tells enabling/disabling r_test_mode AKA scrmable_idle_en
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details  Rx scramble Idle Enable */
int plp_aperta2_tscpmod_pcs_rx_scramble_idle_en(PHYMOD_ST* pc, int en)
{

    RX_X4_DEC_CTL0r_t RX_X4_CONTROL0_DECODE_CONTROL_0r_reg;

    RX_X4_DEC_CTL0r_CLR(RX_X4_CONTROL0_DECODE_CONTROL_0r_reg);
    PHYMOD_IF_ERR_RETURN(READ_RX_X4_DEC_CTL0r(pc, &RX_X4_CONTROL0_DECODE_CONTROL_0r_reg));
    RX_X4_DEC_CTL0r_R_TEST_MODE_CFGf_SET(RX_X4_CONTROL0_DECODE_CONTROL_0r_reg, en);
    PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_DEC_CTL0r(pc, RX_X4_CONTROL0_DECODE_CONTROL_0r_reg));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_pcs_rx_ts_en_get(PHYMOD_ST* pc, uint32_t* en)
{
    RX_X4_RX_TS_CTLr_t RX_X4_RX_TS_CTLr_reg;

    RX_X4_RX_TS_CTLr_CLR(RX_X4_RX_TS_CTLr_reg);
    PHYMOD_IF_ERR_RETURN(READ_RX_X4_RX_TS_CTLr(pc, &RX_X4_RX_TS_CTLr_reg));
    *en = RX_X4_RX_TS_CTLr_TS_UPDATE_ENABLEf_GET(RX_X4_RX_TS_CTLr_reg);

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_pcs_1588_ts_offset_set(PHYMOD_ST *pc, uint32_t ns_offset, uint32_t sub_ns_offset)
{
    PMD_X1_PM_TMR_OFFSr_t PMD_X1_PM_TMR_OFFSr_reg;
    plp_aperta2_phymod_phy_access_t pa_copy;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));

    PMD_X1_PM_TMR_OFFSr_CLR(PMD_X1_PM_TMR_OFFSr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X1_PM_TMR_OFFSr(pc, &PMD_X1_PM_TMR_OFFSr_reg));
    PMD_X1_PM_TMR_OFFSr_PM_OFFSET_IN_NSf_SET(PMD_X1_PM_TMR_OFFSr_reg, ns_offset);
    PMD_X1_PM_TMR_OFFSr_PM_OFFSET_SUB_NSf_SET(PMD_X1_PM_TMR_OFFSr_reg, sub_ns_offset);

    /* Configure MPP0 */
    pa_copy.access.lane_mask = 0x1 << (PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_PM_TMR_OFFSr(&pa_copy, PMD_X1_PM_TMR_OFFSr_reg));

    /* Configure MPP1 */
    pa_copy.access.lane_mask = 0x1 << (4+ PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_PM_TMR_OFFSr(&pa_copy, PMD_X1_PM_TMR_OFFSr_reg));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_pcs_1588_ts_offset_get(PHYMOD_ST *pc, uint32_t* ns_offset, uint32_t* sub_ns_offset)
{
    PMD_X1_PM_TMR_OFFSr_t PMD_X1_PM_TMR_OFFSr_reg;

    PMD_X1_PM_TMR_OFFSr_CLR(PMD_X1_PM_TMR_OFFSr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X1_PM_TMR_OFFSr(pc, &PMD_X1_PM_TMR_OFFSr_reg));

    *ns_offset      = PMD_X1_PM_TMR_OFFSr_PM_OFFSET_IN_NSf_GET(PMD_X1_PM_TMR_OFFSr_reg);
    *sub_ns_offset  = PMD_X1_PM_TMR_OFFSr_PM_OFFSET_SUB_NSf_GET(PMD_X1_PM_TMR_OFFSr_reg);

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_ts_offset_rx_set(PHYMOD_ST* pc, int tbl_ln, uint32_t *table)
{
    PMD_X1_PM_TMR_OFFSr_t PMD_X1_PM_TMR_OFFSr_reg;
    uint16_t pm_offset_ns, pm_offset_sub_ns;
    int32_t pm_offset, vl_time, vl_update_time;
    int i;

    PMD_X1_PM_TMR_OFFSr_CLR(PMD_X1_PM_TMR_OFFSr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X1_PM_TMR_OFFSr(pc, &PMD_X1_PM_TMR_OFFSr_reg));
    pm_offset_ns = PMD_X1_PM_TMR_OFFSr_PM_OFFSET_IN_NSf_GET(PMD_X1_PM_TMR_OFFSr_reg);
    pm_offset_sub_ns = PMD_X1_PM_TMR_OFFSr_PM_OFFSET_SUB_NSf_GET(PMD_X1_PM_TMR_OFFSr_reg);
    /* pm_offset:
     * bit 0-7: sub ns.
     * bit 8-15: ns.
     */
    pm_offset = ((pm_offset_ns & 0xff) << 8) | (pm_offset_sub_ns & 0xff);

    /* Each entry in the RX table is in below format:
     * bit 0-3: sub ns.
     * bit 4-14: ns.
     * bit 15: reserved.
     */
    for (i = 0; i < tbl_ln; i++) {
        /* Get the time from table and extend the sub_ns field to 8 bit. */
        vl_time = ((table[i] << 4) & 0x7fff0);
        if ((vl_time >> 18) & 1) {
            /* Sign bit extension. */
            vl_time = (vl_time | 0xfff80000);
        }
        vl_update_time = vl_time + pm_offset;

        /* We do not expect overflow here so omitting the overflow check. */
        table[i] = (table[i] & 0xf8000) | ((vl_update_time >> 4) & 0x7fff);
    }

    return PHYMOD_E_NONE;
}

/*
 * On TSCBH, SYNCE_X4 programming is per logical lane based.
 */
int plp_aperta2_tscpmod_synce_mode_set(PHYMOD_ST* pc, int stage0_mode, int stage1_mode)
{
    INTEGER_DIVr_t INTEGER_DIVr_reg;

    INTEGER_DIVr_CLR(INTEGER_DIVr_reg);
    PHYMOD_IF_ERR_RETURN(READ_INTEGER_DIVr(pc, &INTEGER_DIVr_reg));
    INTEGER_DIVr_SYNCE_MODE_STAGE0f_SET(INTEGER_DIVr_reg, stage0_mode);
    INTEGER_DIVr_SYNCE_MODE_STAGE1f_SET(INTEGER_DIVr_reg, stage1_mode);

    PHYMOD_IF_ERR_RETURN(MODIFY_INTEGER_DIVr(pc, INTEGER_DIVr_reg));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_synce_mode_get(PHYMOD_ST* pc, int* stage0_mode, int* stage1_mode)
{
    INTEGER_DIVr_t INTEGER_DIVr_reg;

    INTEGER_DIVr_CLR(INTEGER_DIVr_reg);
    PHYMOD_IF_ERR_RETURN(READ_INTEGER_DIVr(pc, &INTEGER_DIVr_reg));
    *stage0_mode = INTEGER_DIVr_SYNCE_MODE_STAGE0f_GET(INTEGER_DIVr_reg);
    *stage1_mode = INTEGER_DIVr_SYNCE_MODE_STAGE1f_GET(INTEGER_DIVr_reg);

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_synce_clk_ctrl_set(PHYMOD_ST* pc, uint32_t val)
{
    FRACTIONAL_DIVr_t FRACTIONAL_DIVr_reg;

    FRACTIONAL_DIVr_CLR(FRACTIONAL_DIVr_reg);
    PHYMOD_IF_ERR_RETURN(READ_FRACTIONAL_DIVr(pc, &FRACTIONAL_DIVr_reg));
    FRACTIONAL_DIVr_SET(FRACTIONAL_DIVr_reg, val);
    PHYMOD_IF_ERR_RETURN(MODIFY_FRACTIONAL_DIVr(pc, FRACTIONAL_DIVr_reg));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_synce_clk_ctrl_get(PHYMOD_ST* pc, uint32_t* val)
{
    FRACTIONAL_DIVr_t FRACTIONAL_DIVr_reg;

    FRACTIONAL_DIVr_CLR(FRACTIONAL_DIVr_reg);
    PHYMOD_IF_ERR_RETURN(READ_FRACTIONAL_DIVr(pc, &FRACTIONAL_DIVr_reg));

    *val = FRACTIONAL_DIVr_GET(FRACTIONAL_DIVr_reg);

    return PHYMOD_E_NONE;
}


int plp_aperta2_tscpmod_resolved_port_mode_get(PHYMOD_ST* pc, uint32_t* port_mode)
{
    SC_X1_STSr_t reg_val;

    PHYMOD_IF_ERR_RETURN(READ_SC_X1_STSr(pc, &reg_val));
    *port_mode = (uint32_t) SC_X1_STSr_RESOLVED_PORT_MODEf_GET(reg_val);

    return PHYMOD_E_NONE;
}


int plp_aperta2_tscpmod_revid_get(PHYMOD_ST* pc, uint32_t* rev_id)
{
    MAIN0_SERDESIDr_t MAIN0_SERDESIDr_reg;

    PHYMOD_IF_ERR_RETURN(READ_MAIN0_SERDESIDr(pc, &MAIN0_SERDESIDr_reg));
    *rev_id = MAIN0_SERDESIDr_REV_NUMBERf_GET(MAIN0_SERDESIDr_reg);

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_fec_arch_decode_get(int fec_arch, phymod_fec_type_t* fec_type)
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

int plp_aperta2_tscpmod_fec_align_status_get(PHYMOD_ST* pc, uint32_t *fec_align_live)
{
    RX_X4_RS_FEC_RXP_STSr_t fec_status_reg;

    RX_X4_RS_FEC_RXP_STSr_CLR(fec_status_reg);
    PHYMOD_IF_ERR_RETURN(READ_RX_X4_RS_FEC_RXP_STSr(pc, &fec_status_reg));

    *fec_align_live = RX_X4_RS_FEC_RXP_STSr_FEC_ALIGN_STATUS_LIVEf_GET(fec_status_reg);
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_fec_override_set(PHYMOD_ST* pc, uint32_t enable)
{
    SC_X4_SW_SPARE1r_t SC_X4_SW_SPARE1r_reg;
    uint16_t temp_reg_value;

    SC_X4_SW_SPARE1r_CLR(SC_X4_SW_SPARE1r_reg);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_SW_SPARE1r(pc, &SC_X4_SW_SPARE1r_reg));
    temp_reg_value = SC_X4_SW_SPARE1r_GET(SC_X4_SW_SPARE1r_reg);
    /* first clear the lowest bit */
    temp_reg_value &= ~TSCPMOD_FEC_OVERRIDE_MASK;
    temp_reg_value |= (enable & TSCPMOD_FEC_OVERRIDE_MASK);
    SC_X4_SW_SPARE1r_SET(SC_X4_SW_SPARE1r_reg, temp_reg_value);
    PHYMOD_IF_ERR_RETURN(WRITE_SC_X4_SW_SPARE1r(pc, SC_X4_SW_SPARE1r_reg));

    return PHYMOD_E_NONE;
}


int plp_aperta2_tscpmod_fec_override_get(PHYMOD_ST* pc, uint32_t* enable)
{
    SC_X4_SW_SPARE1r_t SC_X4_SW_SPARE1r_reg;
    uint16_t temp_reg_value;

    SC_X4_SW_SPARE1r_CLR(SC_X4_SW_SPARE1r_reg);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_SW_SPARE1r(pc, &SC_X4_SW_SPARE1r_reg));
    temp_reg_value = SC_X4_SW_SPARE1r_GET(SC_X4_SW_SPARE1r_reg);
    *enable = temp_reg_value & TSCPMOD_FEC_OVERRIDE_MASK;

    return PHYMOD_E_NONE;
}


int plp_aperta2_tscpmod_fec_correctable_counter_get(PHYMOD_ST* pc, int speed, uint32_t* count)
{
    int start_lane, num_lane;
    RX_X4_FEC_CORR_CTR0r_t lower_reg;
    RX_X4_FEC_CORR_CTR1r_t upper_reg;
    plp_aperta2_phymod_phy_access_t pa_copy;
    uint32_t count_32 = 0;
    uint64_t count_64, count_max;
    int lane = 0;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));
    COMPILER_64_ZERO(count_64);
    COMPILER_64_SET(count_max, 0, 0xffffffff);
    COMPILER_64_SET(count_64, 0, count_32);

    if (num_lane != 8) {
        /*
         * For 400G port 1, MPP0 copy0 and MPP0 copy2 register values need to be added to arrive at the final count.
         * For 400G port 4, MPP1 copy0 and MPP1 copy2 register values need to be added to arrive at the final count.
         */
        if (speed == 400000) {
            pa_copy.access.lane_mask = 0x1 << start_lane;
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_CORR_CTR0r(&pa_copy, &lower_reg));
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_CORR_CTR1r(&pa_copy, &upper_reg));
            count_32 = RX_X4_FEC_CORR_CTR1r_RS_FEC_FEC_CORR_CW_CNTR_UPPERf_GET(upper_reg) << 16 |
                 RX_X4_FEC_CORR_CTR0r_RS_FEC_FEC_CORR_CW_CNTR_LOWERf_GET(lower_reg);
            COMPILER_64_ADD_32(count_64, count_32);

            pa_copy.access.lane_mask = 0x1 << (start_lane + 2);
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_CORR_CTR0r(&pa_copy, &lower_reg));
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_CORR_CTR1r(&pa_copy, &upper_reg));
            count_32 = RX_X4_FEC_CORR_CTR1r_RS_FEC_FEC_CORR_CW_CNTR_UPPERf_GET(upper_reg) << 16 |
                       RX_X4_FEC_CORR_CTR0r_RS_FEC_FEC_CORR_CW_CNTR_LOWERf_GET(lower_reg);
            COMPILER_64_ADD_32(count_64, count_32);
        } else {
            pa_copy.access.lane_mask = 1 << start_lane;
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_CORR_CTR0r(&pa_copy, &lower_reg));
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_CORR_CTR1r(&pa_copy, &upper_reg));
            count_32 = RX_X4_FEC_CORR_CTR1r_RS_FEC_FEC_CORR_CW_CNTR_UPPERf_GET(upper_reg) << 16 |
                       RX_X4_FEC_CORR_CTR0r_RS_FEC_FEC_CORR_CW_CNTR_LOWERf_GET(lower_reg);
            COMPILER_64_ADD_32(count_64, count_32);
        }
    } else {
        /*
         * For 800G port, MPP0 copy0, MPP0 copy2, MPP1 copy0 and MPP1 copy2 register values need to be added to arrive at the final count.
         */
        for (lane = 0; lane < TSCPMOD_NOF_LANES_IN_CORE; lane += 2) {
            pa_copy.access.lane_mask = 0x1 << (lane + PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
            PHYMOD_IF_ERR_RETURN(READ_RX_X4_FEC_CORR_CTR0r(&pa_copy, &lower_reg));
            PHYMOD_IF_ERR_RETURN(READ_RX_X4_FEC_CORR_CTR1r(&pa_copy, &upper_reg));
            count_32 = RX_X4_FEC_CORR_CTR1r_RS_FEC_FEC_CORR_CW_CNTR_UPPERf_GET(upper_reg) << 16 |
                 RX_X4_FEC_CORR_CTR0r_RS_FEC_FEC_CORR_CW_CNTR_LOWERf_GET(lower_reg);
            COMPILER_64_ADD_32(count_64, count_32);
            /*
             * For 400G(8lanes), MPP0 copy0 and MPP0 copy2 register values need to be added to arrive at the final count.
             */
            if (speed == 400000 && lane == 2) {
                break;
            }
        }
    }

    /* Check overflow */
    if (COMPILER_64_GE(count_64, count_max)) {
        *count = 0xffffffff;
    } else {
        *count = COMPILER_64_LO(count_64);
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_fec_uncorrectable_counter_get(PHYMOD_ST* pc, int speed, uint32_t* count)
{
    int start_lane, num_lane;
    RX_X4_FEC_UNCORR_CTR0r_t lower_reg;
    RX_X4_FEC_UNCORR_CTR1r_t upper_reg;
    plp_aperta2_phymod_phy_access_t pa_copy;
    uint32_t count_32 = 0;
    uint64_t count_64, count_max;
    int lane = 0;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));
    COMPILER_64_ZERO(count_64);
    COMPILER_64_SET(count_max, 0, 0xffffffff);
    COMPILER_64_SET(count_64, 0, count_32);

    if (num_lane != 8) {
        /*
         * For 400G port 1, MPP0 copy0 and MPP0 copy2 register values need to be added to arrive at the final count.
         * For 400G port 4, MPP1 copy0 and MPP1 copy2 register values need to be added to arrive at the final count.
         */
        if (speed == 400000) {
            pa_copy.access.lane_mask = 0x1 << start_lane;
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_UNCORR_CTR0r(&pa_copy, &lower_reg));
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_UNCORR_CTR1r(&pa_copy, &upper_reg));
            count_32 = RX_X4_FEC_UNCORR_CTR1r_RS_FEC_FEC_UNCORR_CW_CNTR_UPPERf_GET(upper_reg) << 16 |
                       RX_X4_FEC_UNCORR_CTR0r_RS_FEC_FEC_UNCORR_CW_CNTR_LOWERf_GET(lower_reg);
            COMPILER_64_ADD_32(count_64, count_32);

            pa_copy.access.lane_mask = 0x1 << (start_lane + 2);
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_UNCORR_CTR0r(&pa_copy, &lower_reg));
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_UNCORR_CTR1r(&pa_copy, &upper_reg));
            count_32 = RX_X4_FEC_UNCORR_CTR1r_RS_FEC_FEC_UNCORR_CW_CNTR_UPPERf_GET(upper_reg) << 16 |
                       RX_X4_FEC_UNCORR_CTR0r_RS_FEC_FEC_UNCORR_CW_CNTR_LOWERf_GET(lower_reg);
            COMPILER_64_ADD_32(count_64, count_32);
        } else {
            pa_copy.access.lane_mask = 1 << start_lane;
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_UNCORR_CTR0r(&pa_copy, &lower_reg));
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_UNCORR_CTR1r(&pa_copy, &upper_reg));
            count_32 = RX_X4_FEC_UNCORR_CTR1r_RS_FEC_FEC_UNCORR_CW_CNTR_UPPERf_GET(upper_reg) << 16 |
                       RX_X4_FEC_UNCORR_CTR0r_RS_FEC_FEC_UNCORR_CW_CNTR_LOWERf_GET(lower_reg);
            COMPILER_64_ADD_32(count_64, count_32);
        }    
    } else {
        /*
         * For 800G port, MPP0 copy0, MPP0 copy2, MPP1 copy0 and MPP1 copy2 register values need to be added to arrive at the final count.
         */
        for (lane = 0; lane < TSCPMOD_NOF_LANES_IN_CORE; lane += 2) {
            pa_copy.access.lane_mask = 0x1 << (lane + PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_UNCORR_CTR0r(&pa_copy, &lower_reg));
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_UNCORR_CTR1r(&pa_copy, &upper_reg));
            count_32 = RX_X4_FEC_UNCORR_CTR1r_RS_FEC_FEC_UNCORR_CW_CNTR_UPPERf_GET(upper_reg) << 16 |
                       RX_X4_FEC_UNCORR_CTR0r_RS_FEC_FEC_UNCORR_CW_CNTR_LOWERf_GET(lower_reg);
            COMPILER_64_ADD_32(count_64, count_32);
            /*
             * For 400G(8lanes), MPP0 copy0 and MPP0 copy2 register values need to be added to arrive at the final count.
             */
            if (speed == 400000 && lane == 2) {
                break;
            }
        }
    }

    /* Check overflow */
    if (COMPILER_64_GE(count_64, count_max)) {
        *count = 0xffffffff;
    } else {
        *count = COMPILER_64_LO(count_64);
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_pmd_reset_seq(PHYMOD_ST* pc) /* PMD_RESET_SEQ */
{
    PMD_X1_CTLr_t reg_pmd_x1_ctrl;
    plp_aperta2_phymod_phy_access_t pa_copy;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));

    PMD_X1_CTLr_CLR(reg_pmd_x1_ctrl);

    /* first set the MPP index to 0 */
    pa_copy.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X1_CTLr(&pa_copy,&reg_pmd_x1_ctrl));
    PMD_X1_CTLr_POR_H_RSTBf_SET(reg_pmd_x1_ctrl,1);
    PMD_X1_CTLr_CORE_DP_H_RSTBf_SET(reg_pmd_x1_ctrl,1);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_CTLr(&pa_copy,reg_pmd_x1_ctrl));

    /* next change the MPP index to 1 */
    pa_copy.access.lane_mask = 0x1 << (4+PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
    PMD_X1_CTLr_CLR(reg_pmd_x1_ctrl);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X1_CTLr(&pa_copy,&reg_pmd_x1_ctrl));
    PMD_X1_CTLr_POR_H_RSTBf_SET(reg_pmd_x1_ctrl,1);
    PMD_X1_CTLr_CORE_DP_H_RSTBf_SET(reg_pmd_x1_ctrl,1);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_CTLr(&pa_copy,reg_pmd_x1_ctrl));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_set_an_port_mode(PHYMOD_ST* pc, int starting_lane)    /* SET_AN_PORT_MODE */
{
    uint16_t new_port_mode_sel = 0;
    uint16_t modify_port_mode = 0, is_octal_mode = 0;
    uint16_t port_mode_sel_reg;
    MAIN0_SETUPr_t MAIN0_SETUPr_reg;
    PHYMOD_ST pa_copy;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));

    if (((starting_lane >= 0) && (starting_lane < 4)) ||
         ((starting_lane >=8) && (starting_lane < 12))) {
        pa_copy.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    } else {
        pa_copy.access.lane_mask = 0x10 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    }

    MAIN0_SETUPr_CLR(MAIN0_SETUPr_reg);

    PHYMOD_IF_ERR_RETURN (READ_MAIN0_SETUPr(&pa_copy, &MAIN0_SETUPr_reg));

    port_mode_sel_reg = MAIN0_SETUPr_PORT_MODE_SELf_GET(MAIN0_SETUPr_reg);

    if (port_mode_sel_reg == TSCPMOD_DUAL_PORT) {
        modify_port_mode = 1;
        if((starting_lane == 0 || starting_lane == 8) || (starting_lane == 4 || starting_lane == 12)) {
            new_port_mode_sel = TSCPMOD_TRI1_PORT;
        }
        else if((starting_lane == 2 || starting_lane == 10)  || (starting_lane == 6 || starting_lane == 14)) {
            new_port_mode_sel = TSCPMOD_TRI2_PORT;
        }
    }
    if (port_mode_sel_reg == TSCPMOD_TRI1_PORT) {
        if((starting_lane == 2 || starting_lane == 10)  || (starting_lane == 6 || starting_lane == 14)) {
            modify_port_mode = 1;
            new_port_mode_sel = TSCPMOD_QUAD_PORT;
        }
    }
    if (port_mode_sel_reg == TSCPMOD_TRI2_PORT) {
        if((starting_lane == 0 || starting_lane == 8) || (starting_lane == 4 || starting_lane == 12)) {
            modify_port_mode = 1;
            new_port_mode_sel = TSCPMOD_QUAD_PORT;
        }
    }
    if (port_mode_sel_reg == TSCPMOD_SINGLE_PORT) {
        modify_port_mode = 1;
        new_port_mode_sel = TSCPMOD_QUAD_PORT;
    }
    if (port_mode_sel_reg == TSCPMOD_MULTI_MPP_PORT) {
        modify_port_mode = 1;
        is_octal_mode = 1;
        new_port_mode_sel = TSCPMOD_QUAD_PORT;
    }

    if (((starting_lane >= 0) && (starting_lane < 4)) ||
         ((starting_lane >=8) && (starting_lane < 12))) {
        pa_copy.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    } else {
        pa_copy.access.lane_mask = 0x10 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    }

    if(modify_port_mode == 1) {
        MAIN0_SETUPr_PORT_MODE_SELf_SET(MAIN0_SETUPr_reg, new_port_mode_sel);
    }

    PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_SETUPr(&pa_copy, MAIN0_SETUPr_reg));

    if (is_octal_mode) {
        pa_copy.access.lane_mask = 0x10;
        PHYMOD_IF_ERR_RETURN (MODIFY_MAIN0_SETUPr(&pa_copy, MAIN0_SETUPr_reg));
    }

    return PHYMOD_E_NONE;
}


/**
@brief   update the port mode
@param   pc handle to current TSCF context (#PHYMOD_ST)
@param   data_rate speed of the port.
@returns The value PHYMOD_E_NONE upon successful completion
*/
int plp_aperta2_tscpmod_update_port_mode(PHYMOD_ST *pc, uint32_t data_rate)
{
    MAIN0_SETUPr_t mode_reg;
    PHYMOD_ST phy_copy;
    int port_mode_sel, port_mode_sel_reg, mpp_index = 0;
    uint32_t single_port_mode, multi_mpp_port_mode;
    int first_couple_mode = 0, second_couple_mode = 0;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    port_mode_sel = 0;
    single_port_mode = 0;
    multi_mpp_port_mode = 0;

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));
    /*first check if 8 lane port */
    if(pc->access.lane_mask == 0xff || pc->access.lane_mask == 0xff00) {
        if (data_rate == 800000) {
            multi_mpp_port_mode = 1;
            mpp_index = 0;
            port_mode_sel = TSCPMOD_MULTI_MPP_PORT;
        } else {
            mpp_index = 0;
            single_port_mode = 1;
            port_mode_sel = TSCPMOD_SINGLE_PORT;
        }
    } else if (pc->access.lane_mask == 0xf || pc->access.lane_mask == 0xf00) {
        mpp_index = 0;
        single_port_mode = 1;
        port_mode_sel = TSCPMOD_SINGLE_PORT;
    } else if (pc->access.lane_mask == 0xf0 || pc->access.lane_mask == 0xf000) {
        mpp_index = 1;
        single_port_mode = 1;
        port_mode_sel = TSCPMOD_SINGLE_PORT;
    } else if (pc->access.lane_mask & 0xf || pc->access.lane_mask & 0xf00) {
        mpp_index = 0;
    } else {
        mpp_index = 1;
    }

    /* next set the MPP index properly */
    if (mpp_index) {
        phy_copy.access.lane_mask = 0x10 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    } else {
        phy_copy.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    }

    PHYMOD_IF_ERR_RETURN(READ_MAIN0_SETUPr(&phy_copy, &mode_reg));
    port_mode_sel_reg = MAIN0_SETUPr_PORT_MODE_SELf_GET(mode_reg);
    if ((port_mode_sel_reg == TSCPMOD_MULTI_MPP_PORT) && !multi_mpp_port_mode) {
        plp_aperta2_phymod_phy_access_t temp_access;
        PHYMOD_MEMCPY(&temp_access, pc, sizeof(temp_access));
        temp_access.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
        MAIN0_SETUPr_PORT_MODE_SELf_SET(mode_reg, 0);
        PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(&temp_access, mode_reg));
        temp_access.access.lane_mask = 0x10 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
        PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(&temp_access, mode_reg));
    }

    if (!single_port_mode && !multi_mpp_port_mode) {
        first_couple_mode = ((port_mode_sel_reg == TSCPMOD_TRI2_PORT) ||
                             (port_mode_sel_reg == TSCPMOD_DUAL_PORT) ||
                             (port_mode_sel_reg == TSCPMOD_SINGLE_PORT));

        second_couple_mode = ((port_mode_sel_reg == TSCPMOD_TRI1_PORT) ||
                              (port_mode_sel_reg == TSCPMOD_DUAL_PORT) ||
                              (port_mode_sel_reg == TSCPMOD_SINGLE_PORT));
        /* based on the lane mask, figue out which couple mode */
        switch (pc->access.lane_mask) {
        case 0x1:
        case 0x2:
        case 0x10:
        case 0x20:
        case 0x100:
        case 0x200:
        case 0x1000:
        case 0x2000:
          first_couple_mode = 0;
          break;
        case 0x4:
        case 0x8:
        case 0x40:
        case 0x80:
        case 0x400:
        case 0x800:
        case 0x4000:
        case 0x8000:
          second_couple_mode = 0;
          break;
        case 0x3:
        case 0x30:
        case 0x300:
        case 0x3000:
          first_couple_mode = 1;
          break;
        case 0xc:
        case 0xc0:
        case 0xc00:
        case 0xc000:
          second_couple_mode = 1;
          break;
        default:
          return PHYMOD_E_PARAM;
        }

        if (first_couple_mode) {
            port_mode_sel =(second_couple_mode)? TSCPMOD_DUAL_PORT: TSCPMOD_TRI2_PORT;
        } else if ((second_couple_mode) && (port_mode_sel_reg != TSCPMOD_SINGLE_PORT)) {
            port_mode_sel = TSCPMOD_TRI1_PORT;
        } else {
            port_mode_sel = TSCPMOD_QUAD_PORT;
        }
    }

    if (multi_mpp_port_mode) {
        /* both MPP copies needs to be updated */
        MAIN0_SETUPr_PORT_MODE_SELf_SET(mode_reg, port_mode_sel);
        PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(&phy_copy, mode_reg));
        phy_copy.access.lane_mask = 0x10 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
        PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(&phy_copy, mode_reg));
    } else {
        MAIN0_SETUPr_PORT_MODE_SELf_SET(mode_reg, port_mode_sel);
        PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(&phy_copy, mode_reg));
    }
    
    return PHYMOD_E_NONE ;
}

int plp_aperta2_tscpmod_refclk_set(PHYMOD_ST* pc, tscpmod_refclk_t ref_clk)
{
    MAIN0_SETUPr_t MAIN0_SETUPr_reg;
    int start_lane, num_lane;
    plp_aperta2_phymod_phy_access_t pa_copy;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));

    /* need to config both MPPs */
    pa_copy.access.lane_mask = 0x1 << (0+PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
    PHYMOD_IF_ERR_RETURN(READ_MAIN0_SETUPr(&pa_copy, &MAIN0_SETUPr_reg));
    MAIN0_SETUPr_REFCLK_SELf_SET(MAIN0_SETUPr_reg, ref_clk);
    PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(&pa_copy, MAIN0_SETUPr_reg));

    pa_copy.access.lane_mask = 0x1 << (4+PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
    PHYMOD_IF_ERR_RETURN(READ_MAIN0_SETUPr(&pa_copy, &MAIN0_SETUPr_reg));
    MAIN0_SETUPr_REFCLK_SELf_SET(MAIN0_SETUPr_reg, ref_clk);
    PHYMOD_IF_ERR_RETURN(MODIFY_MAIN0_SETUPr(&pa_copy, MAIN0_SETUPr_reg));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_refclk_get(PHYMOD_ST* pc, tscpmod_refclk_t* ref_clk)
{
    plp_aperta2_phymod_phy_access_t pa_copy;
    MAIN0_SETUPr_t MAIN0_SETUPr_reg;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
    pa_copy.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    PHYMOD_IF_ERR_RETURN(READ_MAIN0_SETUPr(&pa_copy, &MAIN0_SETUPr_reg));
    *ref_clk = MAIN0_SETUPr_REFCLK_SELf_GET(MAIN0_SETUPr_reg);

    return PHYMOD_E_NONE;
}

/**
@brief   PMD per lane reset
@param   pc handle to current TSCBH context (#tbhmod_st)
@returns The value PHYMOD_E_NONE upon successful completion
@details Per lane PMD ln_rst and ln_dp_rst by writing to PMD_X4_CONTROL in pcs space
*/
int plp_aperta2_tscpmod_pmd_x4_reset(PHYMOD_ST* pc)              /* PMD_X4_RESET */
{
    plp_aperta2_phymod_phy_access_t pa_copy;
    PMD_X4_CTLr_t reg_pmd_x4_ctrl;

    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
    PMD_X4_CTLr_CLR(reg_pmd_x4_ctrl);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_CTLr(&pa_copy, &reg_pmd_x4_ctrl));
    PMD_X4_CTLr_LN_TX_H_RSTBf_SET(reg_pmd_x4_ctrl, 1);
    PMD_X4_CTLr_LN_RX_H_RSTBf_SET(reg_pmd_x4_ctrl, 1);

    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_CTLr(&pa_copy, reg_pmd_x4_ctrl));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_set_sc_speed(PHYMOD_ST* pc, int mapped_speed, int set_sw_speed_change)  /* SET_SC_SPEED */
{
    SC_X4_CTLr_t SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg;

    /* write 0 to the speed change */
    SC_X4_CTLr_CLR(SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg);

    PHYMOD_IF_ERR_RETURN(READ_SC_X4_CTLr(pc, &SC_X4_CONTROL_SC_X4_CONTROL_CONTROLr_reg));

    if (set_sw_speed_change) {
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

/*!
@brief Enable/disable AN
@param pc handle to current TSCBH context ($tbhmod_st)
@returns PHYMOD_E_NONE if no errors. PHYMOD_E_ERROR else.
@details
This function programs auto-negotiation (AN) modes for the TEF. It can
enable/disable clause37/clause73/BAM autonegotiation capabilities. Call this
function once for combo mode and once per lane in independent lane mode.
The autonegotiation mode is indicated by setting an_control as required.
*/
int plp_aperta2_tscpmod_autoneg_control(PHYMOD_ST* pc, tscpmod_an_control_t *an_control)
{
    plp_aperta2_phymod_phy_access_t pa_copy;
    uint16_t num_advertised_lanes;
    int start_lane, num_of_lane;
    uint16_t cl73_enable, cl73_bam_enable;
    uint16_t cl73_next_page;
    uint16_t cl73_restart;
    uint16_t cl73_bam_code;
    uint16_t msa_overrides;
    uint32_t oui;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);
    AN_X4_CL73_CFGr_t      AN_X4_CL73_CFGr_reg;
    AN_X4_LD_BASE_ABIL1r_t AN_X4_LD_BASE_ABIL1r_reg;
    AN_X4_LD_BAM_ABILr_t   AN_X4_ABILITIES_LD_BAM_ABILITIESr_reg;
    AN_X1_OUI_UPRr_t       AN_X1_OUI_UPRr_reg;
    AN_X1_OUI_LWRr_t       AN_X1_OUI_LWRr_reg;

    PHYMOD_IF_ERR_RETURN
      (plp_aperta2_phymod_util_lane_config_get(&pc->access, &start_lane, &num_of_lane));
    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));

    num_advertised_lanes          = an_control->num_lane_adv;
    cl73_bam_code                 = 0x0;
    cl73_bam_enable               = 0x0;
    cl73_enable                   = 0x0;
    cl73_next_page                = 0x0;
    cl73_restart                  = 0x0;
    msa_overrides                 = 0x0;
    oui                           = TSCPMOD_BRCM_OUI;

    switch (an_control->an_type) {
    case TSCPMOD_AN_MODE_CL73:
        cl73_restart                = an_control->enable;
        cl73_enable                 = an_control->enable;
        break;
    case TSCPMOD_AN_MODE_CL73_BAM:
        cl73_restart                = an_control->enable;
        cl73_enable                 = an_control->enable;
        cl73_bam_enable             = an_control->enable;
        cl73_bam_code               = 0x3;
        cl73_next_page              = 0x1;
        break;
    case TSCPMOD_AN_MODE_CL73_MSA:
        cl73_restart                = an_control->enable;
        cl73_enable                 = an_control->enable;
        cl73_bam_enable             = an_control->enable;
        cl73_bam_code               = 0x3;
        cl73_next_page              = 0x1;
        msa_overrides               = 0x1;
        oui                         = TSCPMOD_MSA_OUI;
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
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BAM_ABILr(pc, &AN_X4_ABILITIES_LD_BAM_ABILITIESr_reg));
    AN_X4_LD_BAM_ABILr_CL73_BAM_CODEf_SET(AN_X4_ABILITIES_LD_BAM_ABILITIESr_reg, cl73_bam_code);
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_BAM_ABILr(pc, AN_X4_ABILITIES_LD_BAM_ABILITIESr_reg));

    /* Set OUI */
    if (an_control->an_type != TSCPMOD_AN_MODE_CL73) {
        if (((start_lane >= 0) && (start_lane < 4)) ||
            ((start_lane >=8) && (start_lane < 12))) {
            pa_copy.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
        } else {
            pa_copy.access.lane_mask = 0x10 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
        }
        AN_X1_OUI_UPRr_CLR(AN_X1_OUI_UPRr_reg);
        AN_X1_OUI_UPRr_SET(AN_X1_OUI_UPRr_reg, (oui >> 16) & 0xff);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_OUI_UPRr(&pa_copy, AN_X1_OUI_UPRr_reg));

        AN_X1_OUI_LWRr_CLR(AN_X1_OUI_LWRr_reg);
        AN_X1_OUI_LWRr_SET(AN_X1_OUI_LWRr_reg, oui & 0xffff);
        PHYMOD_IF_ERR_RETURN(WRITE_AN_X1_OUI_LWRr(&pa_copy, AN_X1_OUI_LWRr_reg));
    }
    /* Clear AN enable bit in 0xc1c0 */
    AN_X4_CL73_CFGr_CLR(AN_X4_CL73_CFGr_reg);
    PHYMOD_IF_ERR_RETURN (READ_AN_X4_CL73_CFGr(pc, &AN_X4_CL73_CFGr_reg));
    AN_X4_CL73_CFGr_CL73_ENABLEf_SET(AN_X4_CL73_CFGr_reg, 0);
    AN_X4_CL73_CFGr_CL73_AN_RESTARTf_SET(AN_X4_CL73_CFGr_reg, 0);
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
        AN_X4_CL73_CFGr_CL73_AN_RESTARTf_SET(AN_X4_CL73_CFGr_reg, 0);
        PHYMOD_IF_ERR_RETURN (MODIFY_AN_X4_CL73_CFGr(pc, AN_X4_CL73_CFGr_reg));
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_autoneg_control_get(PHYMOD_ST* pc, tscpmod_an_control_t *an_control, int *an_complete)
{
    AN_X4_CL73_CFGr_t      AN_X4_CL73_CFGr_reg;
    AN_X4_AN_MISC_STSr_t   AN_X4_AN_MISC_STSr_reg;
    AN_X4_LD_BASE_ABIL1r_t AN_X4_LD_BASE_ABIL1r_reg;
    AN_X4_LD_BASE_ABIL2r_t AN_X4_LD_BASE_ABIL2r_reg;
    uint32_t   base_ability1, base_ability2;

    /* CL73 AN CONFIG 0xC1C0 */
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_CL73_CFGr(pc, &AN_X4_CL73_CFGr_reg));
    if (AN_X4_CL73_CFGr_CL73_BAM_ENABLEf_GET(AN_X4_CL73_CFGr_reg) == 1) {
        if (AN_X4_CL73_CFGr_MSA_FEC_MAPPINGf_GET(AN_X4_CL73_CFGr_reg) == 1) {
            PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL1r(pc, &AN_X4_LD_BASE_ABIL1r_reg));
            PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL2r(pc, &AN_X4_LD_BASE_ABIL2r_reg));
            base_ability1 =  AN_X4_LD_BASE_ABIL1r_GET(AN_X4_LD_BASE_ABIL1r_reg) & 0xc01f;
            base_ability2 = AN_X4_LD_BASE_ABIL2r_GET(AN_X4_LD_BASE_ABIL2r_reg) & 0x3f;

            if (!(base_ability1) && !(base_ability2)) {
                an_control->an_type = TSCPMOD_AN_MODE_MSA;
                an_control->enable = 1;
            } else {
                an_control->an_type = TSCPMOD_AN_MODE_CL73_MSA;
                an_control->enable = 1;
            }
        } else {
            an_control->an_type = TSCPMOD_AN_MODE_CL73_BAM;
            an_control->enable = 1;
        }
    } else if (AN_X4_CL73_CFGr_CL73_ENABLEf_GET(AN_X4_CL73_CFGr_reg) == 1) {
        an_control->an_type = TSCPMOD_AN_MODE_CL73;
        an_control->enable = 1;
    } else {
         an_control->an_type = TSCPMOD_AN_MODE_NONE;
         an_control->enable = 0;
    }

    an_control->num_lane_adv = AN_X4_CL73_CFGr_NUM_ADVERTISED_LANESf_GET(AN_X4_CL73_CFGr_reg);

    /* an_complete status 0xC1E9 */
    AN_X4_AN_MISC_STSr_CLR(AN_X4_AN_MISC_STSr_reg);
    PHYMOD_IF_ERR_RETURN (READ_AN_X4_AN_MISC_STSr(pc, &AN_X4_AN_MISC_STSr_reg));
    *an_complete = AN_X4_AN_MISC_STSr_AN_COMPLETEf_GET(AN_X4_AN_MISC_STSr_reg);

    return PHYMOD_E_NONE;
}


int plp_aperta2_tscpmod_autoneg_status_get(PHYMOD_ST* pc, int *an_en, int *an_done)
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

int plp_aperta2_tscpmod_autoneg_fec_status_get(PHYMOD_ST* pc, uint8_t *fec_status)
{
    return PHYMOD_E_NONE;
}

/*!
 * @brief   Controls the setting/resetting of autoneg advertisement registers.
 * @param   pc handle to current TSCBH context (#PHYMOD_ST)
 * @returns PHYMOD_E_NONE if no errors. PHYMOD_E_FAIL else.
 * @details
 *   Get aneg features via an_ability_st and write the pages
 *   This does not start the autoneg. That is done in tbhmod_autoneg_control
*/
int plp_aperta2_tscpmod_autoneg_ability_set(PHYMOD_ST* pc,
                               const phymod_autoneg_advert_abilities_t* autoneg_abilities)
{
    AN_X4_LD_BASE_ABIL1r_t AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg;
    AN_X4_LD_BASE_ABIL2r_t AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg;
    AN_X4_LD_UP1_ABIL0r_t AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg;
    AN_X4_LD_UP1_ABIL1r_t AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg;
    AN_X4_CL73_CFGr_t      AN_X4_CL73_CFGr_reg;
    phymod_autoneg_advert_ability_t *an_ability;
    uint32_t i;
    uint8_t fec_is_shared;
    AN_X4_LD_CTLr_t an_x4_ctrl_reg;
    AN_X4_LD_BASE_ABIL0r_t AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg;


    AN_X4_LD_BASE_ABIL1r_CLR(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg);
    AN_X4_LD_BASE_ABIL2r_CLR(AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg);
    AN_X4_LD_UP1_ABIL0r_CLR(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg);
    AN_X4_LD_UP1_ABIL1r_CLR(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg);
    AN_X4_CL73_CFGr_CLR(AN_X4_CL73_CFGr_reg);
    AN_X4_LD_CTLr_CLR(an_x4_ctrl_reg);
    BCMI_TSCP_XGXS_AN_X4_LD_BASE_ABIL0r_CLR(AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg);
     

    /* Set default value */
    AN_X4_CL73_CFGr_MSA_FEC_MAPPINGf_SET(AN_X4_CL73_CFGr_reg, 0);
    AN_X4_LD_BASE_ABIL1r_FEC_REQf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 0x1);
    AN_X4_LD_UP1_ABIL1r_RS_FEC_REQf_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 0x1);

    /* Set default values of AN_X4_LD_BASE_ABIL0r_t */
    AN_X4_LD_BASE_ABIL0r_CL73_BASE_SELECTORf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg, 1);
    AN_X4_LD_BASE_ABIL0r_TX_NONCEf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg, 0x15);


    for (i = 0; i < autoneg_abilities->num_abilities; i++) {
        an_ability = &autoneg_abilities->autoneg_abilities[i];
        fec_is_shared = 0;
        switch (an_ability->speed) {
        case 10000:
            /* CL73-10G-1lane */
            AN_X4_LD_BASE_ABIL1r_BASE_10G_KR1f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
            break;
        case 20000:
            /* CL73BAM-20G-1lane */
            if (an_ability->medium == phymodFirmwareMediaTypeCopperCable) {
              AN_X4_LD_UP1_ABIL1r_BAM_20G_CR1f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
            } else {
              AN_X4_LD_UP1_ABIL1r_BAM_20G_KR1f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
            }
            break;
        case 25000:
            if (an_ability->an_mode == phymod_AN_MODE_CL73) {
                /* CL73 */
                if (an_ability->channel == phymod_channel_short) {
                    AN_X4_LD_BASE_ABIL1r_BASE_25G_CRS_KRSf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
                } else {
                    AN_X4_LD_BASE_ABIL1r_BASE_25G_CR_KRf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
                    if (an_ability->fec == phymod_fec_None) {
                        AN_X4_LD_BASE_ABIL1r_BASE_25G_CRS_KRSf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
                    }
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
            break;
        case 50000:
            if (an_ability->an_mode == phymod_AN_MODE_CL73) {
                /* CL73-50G-1lane */
                AN_X4_LD_BASE_ABIL2r_BASE_50G_CR1_KR1f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg, 1);
            } else if (an_ability->an_mode == phymod_AN_MODE_CL73BAM) {
                if (an_ability->resolved_num_lanes == 1) {
                    AN_X4_LD_UP1_ABIL0r_BAM_50G_BRCM_FEC_528_CR1_KR1f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg, 1);
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
                    }
                }
            } else {
                AN_X4_CL73_CFGr_MSA_FEC_MAPPINGf_SET(AN_X4_CL73_CFGr_reg, 1);
                /* first check 50G  RS272 1 lane MSA ability*/
                if ((an_ability->resolved_num_lanes == 1) && (an_ability->fec == phymod_fec_RS272)) {
                    /* first need to set the  MSA_LFR bit */
                    AN_X4_LD_UP1_ABIL1r_MSA_LFRf_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                    AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_KR4_CR4_MSA_LF1f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                    AN_X4_LD_BASE_ABIL2r_BASE_50G_CR1_KR1f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg, 1);
                } else if (an_ability->resolved_num_lanes == 2) {
                    /* MSA-50G-2lanes */
                    if (an_ability->medium == phymodFirmwareMediaTypeCopperCable) {
                        AN_X4_LD_UP1_ABIL0r_BAM_50G_CR2f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg, 1);
                    } else {
                        AN_X4_LD_UP1_ABIL0r_BAM_50G_KR2f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg, 1);
                    }
                    fec_is_shared = 1;
                }
            }
            break;
        case 100000:
            if (an_ability->an_mode == phymod_AN_MODE_CL73) {
                if (an_ability->resolved_num_lanes == 1) {
                    /* CL73 100G 1 lane */
                    AN_X4_LD_BASE_ABIL2r_BASE_100G_CR1_KR1f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg, 1);
                    AN_X4_LD_BASE_ABIL0r_BASE_100G_FEC_SELf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg, 3);
                    if (an_ability->fec == phymod_fec_RS544_2XN) {
                        /* Enables the selection of the FEC */
                        AN_X4_LD_BASE_ABIL0r_BASE_100G_FEC_SEL_ENf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg, 1);
                    } else {
                        AN_X4_LD_BASE_ABIL0r_BASE_100G_FEC_SEL_ENf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg, 0);
                    }

                } else if (an_ability->resolved_num_lanes == 2) {
                    /* CL73-100G-2lanes */
                    AN_X4_LD_BASE_ABIL2r_BASE_100G_CR2_KR2f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg,1);
                } else {
                    /* CL73_100G-4lanes */
                    if (an_ability->medium == phymodFirmwareMediaTypeCopperCable) {
                        AN_X4_LD_BASE_ABIL1r_BASE_100G_CR4f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
                    } else {
                        AN_X4_LD_BASE_ABIL1r_BASE_100G_KR4f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
                    }
                }
            } else if (an_ability->an_mode == phymod_AN_MODE_MSA) {
                if ((an_ability->resolved_num_lanes == 2) && (an_ability->fec == phymod_fec_RS272)) {
                    /* first need to set MSA FEC mapping bit */
                    AN_X4_CL73_CFGr_MSA_FEC_MAPPINGf_SET(AN_X4_CL73_CFGr_reg, 1);
                    /* then need to set the  MSA_LFR bit */
                    AN_X4_LD_UP1_ABIL1r_MSA_LFRf_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                    AN_X4_LD_UP1_ABIL0r_SPEED_SPARE_22_MSA_LF2f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg, 1);
                    AN_X4_LD_BASE_ABIL2r_BASE_100G_CR2_KR2f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg,1);
                }
            } else {
                /*below ability is for CL73BAM */
                if (an_ability->resolved_num_lanes == 2) {
                    /* CL73BAM-100G-2lanes */
                    if (an_ability->fec == phymod_fec_None) {
                        AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_NO_FEC_KR2_CR2f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                    } else {
                        AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_FEC_528_KR2_CR2_LF3f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                    }
                } else {
                    /* CL73BAM-100G-4lanes */
                    if (an_ability->fec == phymod_fec_None) {
                        AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_NO_FEC_X4f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                    }
                }
            }
            break;
        case 200000:
            if (an_ability->an_mode == phymod_AN_MODE_CL73) {
                if (an_ability->resolved_num_lanes == 2) {
                    /* 200G 2 lane CL73 */
                    AN_X4_LD_BASE_ABIL2r_BASE_200G_CR2_KR2f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg, 1);
                } else {
                    /* CL73-200G-4lanes */
                    AN_X4_LD_BASE_ABIL2r_BASE_200G_CR4_KR4f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg, 1);
                }
            } else if (an_ability->an_mode == phymod_AN_MODE_MSA) {
                if ((an_ability->resolved_num_lanes == 4) && (an_ability->fec == phymod_fec_RS272_2XN)) {
                    /* first need to set MSA FEC mapping bit */
                    AN_X4_CL73_CFGr_MSA_FEC_MAPPINGf_SET(AN_X4_CL73_CFGr_reg, 1);
                    /* then need to set the  MSA_LFR bit */
                    AN_X4_LD_UP1_ABIL1r_MSA_LFRf_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                    AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_FEC_528_KR2_CR2_LF3f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                    AN_X4_LD_BASE_ABIL2r_BASE_200G_CR4_KR4f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg, 1);
                }
            } else {
                /* CL73BAM-200G-4lanes */
                if (an_ability->fec == phymod_fec_None) {
                    AN_X4_LD_UP1_ABIL1r_BAM_200G_BRCM_NO_FEC_KR4_CR4f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                } else {
                    AN_X4_LD_UP1_ABIL1r_BAM_200G_BRCM_KR4_CR4f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 1);
                }
            }
            break;
        case 400000:
            {
                if (an_ability->resolved_num_lanes == 4) {
                    AN_X4_LD_BASE_ABIL2r_BASE_400G_CR4_KR4f_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg, 1);
                } else {
                    AN_X4_LD_UP1_ABIL0r_BAM_400G_BRCM_FEC_KR8_CR8f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg, 1);
                }
                break;
            }
        case 800000:
            {
                AN_X4_LD_UP1_ABIL0r_BAM_800G_BRCM_FEC_KR8_CR8f_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg, 1);
                break;
            }
        default:
            break;
        }

        if (fec_is_shared) {
            if (an_ability->an_mode == phymod_AN_MODE_CL73) {
                /****** Base FEC Settings ********/
                if ((an_ability->speed == 25000) && (an_ability->fec == phymod_fec_CL91)) {
                    AN_X4_LD_BASE_ABIL1r_BASE_25G_RS_FEC_REQf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
                }
            } else {
                /****** BAM FEC ******/
                if (an_ability->fec == phymod_fec_CL91) {
                    AN_X4_LD_UP1_ABIL1r_RS_FEC_REQf_SET(AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg, 0x3);
                }
            }
        }

        /******* Pause Settings ********/
        if (an_ability->pause == phymod_pause_none) {
            AN_X4_LD_BASE_ABIL1r_CL73_PAUSEf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 0);
        }
        if (an_ability->pause == phymod_pause_symm) {
            AN_X4_LD_BASE_ABIL1r_CL73_PAUSEf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 1);
        }
        if (an_ability->pause == phymod_pause_asym) {
            AN_X4_LD_BASE_ABIL1r_CL73_PAUSEf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 2);
        }
        if (an_ability->pause == phymod_pause_asym_symm) {
            AN_X4_LD_BASE_ABIL1r_CL73_PAUSEf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg, 3);
        }
    }

    /***** Setting AN_X4_ABILITIES_cl73_cfg 0xC1C0 *********/
    PHYMOD_IF_ERR_RETURN(MODIFY_AN_X4_CL73_CFGr(pc, AN_X4_CL73_CFGr_reg));
    /***** Setting AN_X4_ABILITIES_ld_base_abilities_1 0xC1C4 *******/
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_BASE_ABIL1r(pc, AN_X4_ABILITIES_LD_BASE_ABILITIES_1r_reg));
    /***** Setting AN_X4_ABILITIES_ld_base_abilities_1 0xC1C7 *******/
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_BASE_ABIL2r(pc, AN_X4_ABILITIES_LD_BASE_ABILITIES_2r_reg ));
    /******** Setting AN_X4_ABILITIES_ld_up1_abilities_0 0xC1C1******/
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_UP1_ABIL0r(pc, AN_X4_ABILITIES_LD_UP1_ABILITIES_0r_reg));
    /******** Setting AN_X4_ABILITIES_ld_up1_abilities_0 0xC1C2******/
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_UP1_ABIL1r(pc, AN_X4_ABILITIES_LD_UP1_ABILITIES_1r_reg));
    /******** Setting AN_X4_CTRL 0xC1e7******/
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_CTLr(pc, an_x4_ctrl_reg));

    /***** Setting AN_X4_ABILITIES_ld_base_abilities_0 0xC1C3 *******/
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_BASE_ABIL0r(pc, AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg));

    return PHYMOD_E_NONE;
}

/*
   enable BASE_100G_FEC_EN/SEL for 100G RS544_2xN
   this function is applicable for DPLL only
*/
int plp_aperta2_tscpmod_autoneg_ability_base_100g_fec_sel_set(PHYMOD_ST* pc,
                               const phymod_autoneg_advert_abilities_t* autoneg_abilities)
{
    AN_X4_LD_BASE_ABIL0r_t AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg;
    phymod_autoneg_advert_ability_t *an_ability;
    uint32_t i;

    AN_X4_LD_BASE_ABIL0r_CLR(AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg);

    /* Set default values of AN_X4_LD_BASE_ABIL0r_t */
    AN_X4_LD_BASE_ABIL0r_CL73_BASE_SELECTORf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg, 1);
    AN_X4_LD_BASE_ABIL0r_TX_NONCEf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg, 0x15);

    for (i = 0; i < autoneg_abilities->num_abilities; i++) {
        an_ability = &autoneg_abilities->autoneg_abilities[i];
        switch (an_ability->speed) {
        case 100000:
            if ((an_ability->an_mode == phymod_AN_MODE_CL73) &&
                (an_ability->resolved_num_lanes == 1)) {
                /* CL73 100G 1 lane */
                AN_X4_LD_BASE_ABIL0r_BASE_100G_FEC_SELf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg, 3);
                if (an_ability->fec == phymod_fec_RS544_2XN) {
                    /* Enables the selection of the FEC */
                    AN_X4_LD_BASE_ABIL0r_BASE_100G_FEC_SEL_ENf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg, 1);
                } else {
                    AN_X4_LD_BASE_ABIL0r_BASE_100G_FEC_SEL_ENf_SET(AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg, 0);
                }
            }
            break;
        default:
            break;
        }
    }

    /***** Setting AN_X4_ABILITIES_ld_base_abilities_0 0xC1C3 *******/
    PHYMOD_IF_ERR_RETURN(WRITE_AN_X4_LD_BASE_ABIL0r(pc, AN_X4_ABILITIES_LD_BASE_ABILITIES_0r_reg));

    return PHYMOD_E_NONE;
}

int _plp_aperta2_tscpmod_an_ability_construct(phymod_autoneg_advert_ability_t* an_ability,
                          uint32_t speed,
                          int num_lanes,
                          phymod_fec_type_t fec,
                          uint32_t pause,
                          int channel,
                          plp_aperta2_phymod_an_mode_type_t an_mode)
{
    an_ability->speed = speed;
    an_ability->resolved_num_lanes = num_lanes;
    an_ability->fec = fec;
    an_ability->pause = pause;
    an_ability->channel = channel;
    an_ability->an_mode = an_mode;
    /* We get medium type from FW lane config in tier 2 */
    an_ability->medium = 0;

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_autoneg_ability_400g_8lane_get(PHYMOD_ST* pc, uint32_t *enabled)
{
    AN_X4_LD_UP1_ABIL0r_t         AN_X4_LD_UP1_ABIL0r_reg;

    *enabled = 0;
    /***** first read AN_X4_ABILITIES_ld_up0_abilities_1 0xC1C1 *******/
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_UP1_ABIL0r(pc, &AN_X4_LD_UP1_ABIL0r_reg));
    if (AN_X4_LD_UP1_ABIL0r_BAM_400G_BRCM_FEC_KR8_CR8f_GET(AN_X4_LD_UP1_ABIL0r_reg)) {
        *enabled = 1;
    }
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_autoneg_ability_400g_4lane_get(PHYMOD_ST* pc, uint32_t *enabled)
{
    AN_X4_LD_BASE_ABIL2r_t        AN_X4_LD_BASE_ABIL2r_reg;

    *enabled = 0;
    /***** AN_X4_ABILITIES_ld_base_abilities_1 0xC1C4 *******/
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL2r(pc, &AN_X4_LD_BASE_ABIL2r_reg));
    if (AN_X4_LD_BASE_ABIL2r_BASE_400G_CR4_KR4f_GET(AN_X4_LD_BASE_ABIL2r_reg)) {
        *enabled = 1;
    }
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
int plp_aperta2_tscpmod_autoneg_ability_get(PHYMOD_ST* pc, phymod_autoneg_advert_abilities_t* autoneg_abilities)
{
    AN_X4_LD_BASE_ABIL1r_t        AN_X4_LD_BASE_ABIL1r_reg;
    AN_X4_LD_UP1_ABIL0r_t         AN_X4_LD_UP1_ABIL0r_reg;
    AN_X4_LD_UP1_ABIL1r_t         AN_X4_LD_UP1_ABIL1r_reg;
    AN_X4_LD_BASE_ABIL2r_t        AN_X4_LD_BASE_ABIL2r_reg;
    AN_X4_CL73_CFGr_t             AN_X4_CL73_CFGr_reg;
    AN_X4_LD_BASE_ABIL0r_t        AN_X4_LD_BASE_ABIL0r_reg;
    uint32_t pause, fec_cl91;
    uint8_t llf_req_50g = 0, llf_req_100g = 0, llf_req_200g = 0;
    int i = 0;


    /***** AN_X4_ABILITIES_ld_base_abilities_1 0xC1C4 *******/
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL1r(pc, &AN_X4_LD_BASE_ABIL1r_reg));
    /***** AN_X4_ABILITIES_ld_base_abilities_2 0xC1C7 ******/
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL2r(pc, &AN_X4_LD_BASE_ABIL2r_reg));
    /***** AN_X4_ABILITIES_ld_up0_abilities_1 0xC1C1 *******/
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_UP1_ABIL0r(pc, &AN_X4_LD_UP1_ABIL0r_reg));
    /***** AN_X4_ABILITIES_ld_up1_abilities_1 0xC1C2 *******/
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_UP1_ABIL1r(pc, &AN_X4_LD_UP1_ABIL1r_reg));
    /***** AN_X4_ABILITIES_cl73_cfg 0xC1C0 *******/
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_CL73_CFGr(pc, &AN_X4_CL73_CFGr_reg));
    /***** AN_X2_LD_BASE_ABIL0 0xc1c3 ********/
    PHYMOD_IF_ERR_RETURN(READ_AN_X4_LD_BASE_ABIL0r(pc, &AN_X4_LD_BASE_ABIL0r_reg));

    pause = AN_X4_LD_BASE_ABIL1r_CL73_PAUSEf_GET(AN_X4_LD_BASE_ABIL1r_reg);

    if (AN_X4_LD_BASE_ABIL1r_BASE_100G_KR4f_GET(AN_X4_LD_BASE_ABIL1r_reg) ||
        AN_X4_LD_BASE_ABIL1r_BASE_100G_CR4f_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
        PHYMOD_IF_ERR_RETURN(
          _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                100000, 4, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        i++;
    }
    if (AN_X4_LD_BASE_ABIL1r_BASE_40G_KR4f_GET(AN_X4_LD_BASE_ABIL1r_reg) ||
        AN_X4_LD_BASE_ABIL1r_BASE_40G_CR4f_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
        PHYMOD_IF_ERR_RETURN(
          _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                               40000, 4, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        i++;
    }
    if (AN_X4_LD_BASE_ABIL1r_BASE_25G_CR_KRf_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
        if (AN_X4_LD_BASE_ABIL1r_BASE_25G_RS_FEC_REQf_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    25000, 1, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73));
            i++;
        } else {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    25000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73));
            i++;
        }
    }
    if (AN_X4_LD_BASE_ABIL1r_BASE_25G_CRS_KRSf_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
        /* Short channel does not support RS FEC */
        /* CL74 is not supported */
        PHYMOD_IF_ERR_RETURN(
          _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                25000, 1, phymod_fec_None, pause, phymod_channel_short, phymod_AN_MODE_CL73));
        i++;
    }
    if (AN_X4_LD_BASE_ABIL1r_BASE_10G_KR1f_GET(AN_X4_LD_BASE_ABIL1r_reg)) {
        /* CL74 is not supported */
        PHYMOD_IF_ERR_RETURN(
          _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                               10000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        i++;
    }

    /* first check 400G AN MSA */
    if (AN_X4_LD_UP1_ABIL0r_BAM_400G_BRCM_FEC_KR8_CR8f_GET(AN_X4_LD_UP1_ABIL0r_reg)) {
        PHYMOD_IF_ERR_RETURN(
         _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                400000, 8, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_MSA));
        i++;
    }

    /* first check 400G AN CL73 */
    if (AN_X4_LD_BASE_ABIL2r_BASE_400G_CR4_KR4f_GET(AN_X4_LD_BASE_ABIL2r_reg)) {
        PHYMOD_IF_ERR_RETURN(
         _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                400000, 4, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        i++;
    }

    /* next check 800G MSA or BAM */
    /* UP page is BAM page */
    if (AN_X4_LD_UP1_ABIL0r_BAM_800G_BRCM_FEC_KR8_CR8f_GET(AN_X4_LD_UP1_ABIL0r_reg)) {
            /* next need to check if OUI is MSA or BAM */
            uint32_t oui = 0x0;
            AN_X1_OUI_UPRr_t       AN_X1_OUI_UPRr_reg;
            AN_X1_OUI_LWRr_t       AN_X1_OUI_LWRr_reg;

            PHYMOD_IF_ERR_RETURN(READ_AN_X1_OUI_UPRr(pc, &AN_X1_OUI_UPRr_reg));
            PHYMOD_IF_ERR_RETURN(READ_AN_X1_OUI_LWRr(pc, &AN_X1_OUI_LWRr_reg));
            oui = AN_X1_OUI_UPRr_OUI_UPPER_DATAf_GET(AN_X1_OUI_UPRr_reg) << 16;
            oui |= AN_X1_OUI_LWRr_OUI_LOWER_DATAf_GET(AN_X1_OUI_LWRr_reg);

            if (oui == TSCPMOD_MSA_OUI) {
                PHYMOD_IF_ERR_RETURN(
                 _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        800000, 8, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_MSA));
            } else {
                PHYMOD_IF_ERR_RETURN(
                 _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        800000, 8, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
            }
            i++;
    }
    /* Get UP page abilities */
    fec_cl91 = AN_X4_LD_UP1_ABIL1r_RS_FEC_REQf_GET(AN_X4_LD_UP1_ABIL1r_reg);

    if (AN_X4_CL73_CFGr_MSA_FEC_MAPPINGf_GET(AN_X4_CL73_CFGr_reg)) {
        /* UP page is MSA page */
        if (AN_X4_LD_UP1_ABIL0r_BAM_50G_CR2f_GET(AN_X4_LD_UP1_ABIL0r_reg) ||
            AN_X4_LD_UP1_ABIL0r_BAM_50G_KR2f_GET(AN_X4_LD_UP1_ABIL0r_reg)) {
            if (fec_cl91 == 0x3) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        50000, 2, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                i++;
            } else {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        50000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                i++;
            }
        }
        if (AN_X4_LD_UP1_ABIL1r_BAM_25G_CR1f_GET(AN_X4_LD_UP1_ABIL1r_reg) ||
            AN_X4_LD_UP1_ABIL1r_BAM_25G_KR1f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
            if (fec_cl91 == 0x3) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        25000, 1, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                i++;
            } else {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        25000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                i++;
            }
        }
        if (AN_X4_LD_UP1_ABIL1r_MSA_LFRf_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
            /* first check 50G 1 lane */
            if (AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_KR4_CR4_MSA_LF1f_GET(AN_X4_LD_UP1_ABIL1r_reg) &&
                AN_X4_LD_BASE_ABIL2r_BASE_50G_CR1_KR1f_GET(AN_X4_LD_BASE_ABIL2r_reg)) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        50000, 1, phymod_fec_RS272, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                i++;
                llf_req_50g = 1;
            }
            /* next check 100G 2 lane */
            if (AN_X4_LD_UP1_ABIL0r_SPEED_SPARE_22_MSA_LF2f_GET(AN_X4_LD_UP1_ABIL0r_reg) &&
                AN_X4_LD_BASE_ABIL2r_BASE_100G_CR2_KR2f_GET(AN_X4_LD_BASE_ABIL2r_reg)) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        100000, 2, phymod_fec_RS272, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                i++;
                llf_req_100g = 1;
            }
            /* first check 200G 4 lane */
            if (AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_FEC_528_KR2_CR2_LF3f_GET(AN_X4_LD_UP1_ABIL1r_reg) &&
                AN_X4_LD_BASE_ABIL2r_BASE_200G_CR4_KR4f_GET(AN_X4_LD_BASE_ABIL2r_reg)) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        200000, 4, phymod_fec_RS272_2XN, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                i++;
                llf_req_200g = 1;
            }
        }
    } else {
        /* UP page is BAM page */
        if (AN_X4_LD_UP1_ABIL0r_BAM_800G_BRCM_FEC_KR8_CR8f_GET(AN_X4_LD_UP1_ABIL0r_reg)) {
                PHYMOD_IF_ERR_RETURN(
                 _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        800000, 8, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                i++;
        }
        if (AN_X4_LD_UP1_ABIL1r_BAM_200G_BRCM_KR4_CR4f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    200000, 4, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
            i++;
        }
        if (AN_X4_LD_UP1_ABIL1r_BAM_200G_BRCM_NO_FEC_KR4_CR4f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
            PHYMOD_IF_ERR_RETURN(
             _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   200000, 4, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
            i++;
        }
        if (AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_FEC_528_KR2_CR2_LF3f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
            PHYMOD_IF_ERR_RETURN(
             _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   100000, 2, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
            i++;
        }
        if (AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_NO_FEC_KR2_CR2f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    100000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
            i++;
        }
        if (AN_X4_LD_UP1_ABIL1r_BAM_100G_BRCM_NO_FEC_X4f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    100000, 4, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
            i++;
        }
        if (AN_X4_LD_UP1_ABIL0r_BAM_50G_BRCM_FEC_528_CR1_KR1f_GET(AN_X4_LD_UP1_ABIL0r_reg)) {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    50000, 1, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
            i++;
        }
        if (AN_X4_LD_UP1_ABIL0r_BAM_50G_BRCM_FEC_544_CR2_KR2f_GET(AN_X4_LD_UP1_ABIL0r_reg)) {
            PHYMOD_IF_ERR_RETURN(
             _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    50000, 2, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
            i++;
        }
        if (AN_X4_LD_UP1_ABIL0r_BAM_50G_CR2f_GET(AN_X4_LD_UP1_ABIL0r_reg) ||
            AN_X4_LD_UP1_ABIL0r_BAM_50G_KR2f_GET(AN_X4_LD_UP1_ABIL0r_reg)) {
            if (fec_cl91 == 0x3) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        50000, 2, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                i++;
            } else {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        50000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                i++;
            }
        }
        if (AN_X4_LD_UP1_ABIL1r_BAM_25G_CR1f_GET(AN_X4_LD_UP1_ABIL1r_reg) ||
            AN_X4_LD_UP1_ABIL1r_BAM_25G_KR1f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
            if (fec_cl91 == 0x3) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        25000, 1, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                i++;
            } else {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        25000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                i++;
            }
        }
        /* For BAM ability 20G and 40G */
        if (AN_X4_LD_UP1_ABIL0r_BAM_40G_CR2f_GET(AN_X4_LD_UP1_ABIL0r_reg) ||
            AN_X4_LD_UP1_ABIL0r_BAM_40G_KR2f_GET(AN_X4_LD_UP1_ABIL0r_reg)) {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    40000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
            i++;
        }
        if (AN_X4_LD_UP1_ABIL1r_BAM_20G_CR1f_GET(AN_X4_LD_UP1_ABIL1r_reg) ||
            AN_X4_LD_UP1_ABIL1r_BAM_20G_KR1f_GET(AN_X4_LD_UP1_ABIL1r_reg)) {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    20000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
            i++;
        }
    }

    if ((!llf_req_200g) && AN_X4_LD_BASE_ABIL2r_BASE_200G_CR4_KR4f_GET(AN_X4_LD_BASE_ABIL2r_reg)) {
        PHYMOD_IF_ERR_RETURN(
            _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    200000, 4, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_CL73));
          i++;
    }
    if (AN_X4_LD_BASE_ABIL2r_BASE_200G_CR2_KR2f_GET(AN_X4_LD_BASE_ABIL2r_reg)) {
        PHYMOD_IF_ERR_RETURN(
            _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    200000, 2, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_CL73));
          i++;
    }
    if ((!llf_req_100g) && AN_X4_LD_BASE_ABIL2r_BASE_100G_CR2_KR2f_GET(AN_X4_LD_BASE_ABIL2r_reg)) {
        PHYMOD_IF_ERR_RETURN(
            _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    100000, 2, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        i++;
    }
    if (AN_X4_LD_BASE_ABIL2r_BASE_100G_CR1_KR1f_GET(AN_X4_LD_BASE_ABIL2r_reg)) {
        if (AN_X4_LD_BASE_ABIL0r_BASE_100G_FEC_SEL_ENf_GET(AN_X4_LD_BASE_ABIL0r_reg)) {
            PHYMOD_IF_ERR_RETURN(
                _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        100000, 1, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        } else {
            PHYMOD_IF_ERR_RETURN(
                _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        100000, 1, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        }
        i++;
    }
    if ((!llf_req_50g) && AN_X4_LD_BASE_ABIL2r_BASE_50G_CR1_KR1f_GET(AN_X4_LD_BASE_ABIL2r_reg)) {
        PHYMOD_IF_ERR_RETURN(
            _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    50000, 1, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        i++;
    }

    autoneg_abilities->num_abilities = i;

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_autoneg_remote_ability_get(PHYMOD_ST* pc, phymod_autoneg_advert_abilities_t *autoneg_abilities)
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
    uint8_t llf_req_50g = 0, llf_req_100g = 0, llf_req_200g = 0;

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
    pause = (base_0 >> TSCPMOD_AN_BASE0_PAGE_PAUSE_OFFSET) & TSCPMOD_AN_BASE0_PAGE_PAUSE_MASK;
    fec_cl74 = (base_2 >> TSCPMOD_AN_BASE2_CL74_ABILITY_REQ_SUP_OFFSET) & TSCPMOD_AN_BASE2_CL74_ABILITY_REQ_SUP_MASK;

    /* Get Base Abilities */
    if (((base_1 >> TSCPMOD_AN_BASE1_TECH_ABILITY_100G_KR4_OFFSET) & TSCPMOD_AN_BASE1_TECH_ABILITY_100G_KR4_MASK) ||
        ((base_1 >> TSCPMOD_AN_BASE1_TECH_ABILITY_100G_CR4_OFFSET) & TSCPMOD_AN_BASE1_TECH_ABILITY_100G_CR4_MASK)) {
        PHYMOD_IF_ERR_RETURN(
          _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                100000, 4, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        if ((base_1 >> TSCPMOD_AN_BASE1_TECH_ABILITY_100G_CR4_OFFSET) & TSCPMOD_AN_BASE1_TECH_ABILITY_100G_CR4_MASK) {
            autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
        }
        i++;
    }
    if (((base_1 >> TSCPMOD_AN_BASE1_TECH_ABILITY_40G_KR4_OFFSET) & TSCPMOD_AN_BASE1_TECH_ABILITY_40G_KR4_MASK) ||
        ((base_1 >> TSCPMOD_AN_BASE1_TECH_ABILITY_40G_CR4_OFFSET) & TSCPMOD_AN_BASE1_TECH_ABILITY_40G_CR4_MASK)) {
        if (fec_cl74 == 0x3) {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   40000, 4, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        } else {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   40000, 4, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        }
        if ((base_1 >> TSCPMOD_AN_BASE1_TECH_ABILITY_40G_CR4_OFFSET) & TSCPMOD_AN_BASE1_TECH_ABILITY_40G_CR4_MASK) {
            autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
        }
        i++;
    }
    fec_cl74 = (base_2 >> TSCPMOD_AN_BASE2_25G_BASE_R_FEC_ABILITY_REQ_OFFSET) & TSCPMOD_AN_BASE2_25G_BASE_R_FEC_ABILITY_REQ_MASK;
    fec_cl91 = (base_2 >> TSCPMOD_AN_BASE2_25G_RS_FEC_ABILITY_REQ_OFFSET) & TSCPMOD_AN_BASE2_25G_RS_FEC_ABILITY_REQ_MASK;
    if ((base_1 >> TSCPMOD_AN_BASE1_TECH_ABILITY_25G_KR1_CR1_OFFSET) & TSCPMOD_AN_BASE1_TECH_ABILITY_25G_KR1_CR1_MASK) {
        if (fec_cl74 || fec_cl91) {
            if (fec_cl74) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        25000, 1, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73));
                i++;
            }
            if (fec_cl91) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        25000, 1, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73));
                i++;
            }
        } else {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    25000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73));
            i++;
        }
    }
    if ((base_1 >> TSCPMOD_AN_BASE1_TECH_ABILITY_25G_KRS1_CRS1_OFFSET) & TSCPMOD_AN_BASE1_TECH_ABILITY_25G_KRS1_CRS1_MASK) {
        if (fec_cl74) {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    25000, 1, phymod_fec_CL74, pause, 1, phymod_AN_MODE_CL73));
        } else {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    25000, 1, phymod_fec_None, pause, 1, phymod_AN_MODE_CL73));
        }
        i++;
    }
    fec_cl74 = (base_2 >> TSCPMOD_AN_BASE2_CL74_ABILITY_REQ_SUP_OFFSET) & TSCPMOD_AN_BASE2_CL74_ABILITY_REQ_SUP_MASK;
    if ((base_1 >> TSCPMOD_AN_BASE1_TECH_ABILITY_10G_KR1_OFFSET) & TSCPMOD_AN_BASE1_TECH_ABILITY_10G_KR1_MASK) {
        if (fec_cl74 == 0x3) {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   10000, 1, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        } else {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                   10000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        }
        i++;
    }

    /* Check if Next_Page bit is set in base page, if yes, need to check MSA/BAM abilities */
    if ((base_0 >> TSCPMOD_AN_BASE0_PAGE_NP_OFFSET) & TSCPMOD_AN_BASE0_PAGE_NP_MASK) {
        msa_code_13_23 = (mp5_1 >> TSCPMOD_AN_MSG_PAGE1_OUI_13to23_OFFSET) & TSCPMOD_AN_MSG_PAGE1_OUI_13to23_MASK;
        msa_code_2_12 = (mp5_2 >> TSCPMOD_AN_MSG_PAGE2_OUI_2to12_OFFSET) & TSCPMOD_AN_MSG_PAGE2_OUI_2to12_MASK;
        msa_code_0_1 = (up1_0 >> TSCPMOD_AN_UF_PAGE0_OUI_OFFSET) & TSCPMOD_AN_UF_PAGE0_OUI_MASK;
        bam_code = (up1_0 >> TSCPMOD_AN_UF_PAGE0_UD_0to8_OFFSET) & TSCPMOD_AN_UF_PAGE0_UD_0to8_MASK;

        if ((msa_code_13_23 == TSCPMOD_MSA_OUI_13to23) && (msa_code_2_12 == TSCPMOD_MSA_OUI_2to12) && (msa_code_0_1 == TSCPMOD_MSA_OUI_0to1)) {
            /* UF page is MSA page */
            fec_cl74 = (up1_2 >> TSCPMOD_AN_UF_PAGE2_MSA_CL74_REQ_OFFSET) & TSCPMOD_AN_UF_PAGE2_MSA_CL74_REQ_MASK;
            fec_cl91 = (up1_2 >> TSCPMOD_AN_UF_PAGE2_MSA_CL91_REQ_OFFSET) & TSCPMOD_AN_UF_PAGE2_MSA_CL91_REQ_MASK;
            if (((up1_1 >> TSCPMOD_AN_UF_PAGE1_MSA_50G_CR2_ABILITY_OFFSET) & TSCPMOD_AN_UF_PAGE1_MSA_50G_CR2_ABILITY_MASK) ||
                ((up1_1 >> TSCPMOD_AN_UF_PAGE1_MSA_50G_KR2_ABILITY_OFFSET) & TSCPMOD_AN_UF_PAGE1_MSA_50G_KR2_ABILITY_MASK)) {
                if (fec_cl74 || fec_cl91) {
                    if(fec_cl74) {
                        PHYMOD_IF_ERR_RETURN(
                           _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                 50000, 2, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                        if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_MSA_50G_CR2_ABILITY_OFFSET) & TSCPMOD_AN_UF_PAGE1_MSA_50G_CR2_ABILITY_MASK) {
                            autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                        }
                        i++;
                    }
                    if (fec_cl91) {
                        PHYMOD_IF_ERR_RETURN(
                           _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                 50000, 2, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                        if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_MSA_50G_CR2_ABILITY_OFFSET) & TSCPMOD_AN_UF_PAGE1_MSA_50G_CR2_ABILITY_MASK) {
                            autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                        }
                        i++;
                    }
                } else {
                    PHYMOD_IF_ERR_RETURN(
                      _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            50000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                    if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_MSA_50G_CR2_ABILITY_OFFSET) & TSCPMOD_AN_UF_PAGE1_MSA_50G_CR2_ABILITY_MASK) {
                        autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                    }
                    i++;
                }
            }
            if (((up1_1 >> TSCPMOD_AN_UF_PAGE1_MSA_25G_KR1_ABILITY_OFFSET) & TSCPMOD_AN_UF_PAGE1_MSA_25G_KR1_ABILITY_MASK) ||
                ((up1_1 >> TSCPMOD_AN_UF_PAGE1_MSA_25G_CR1_ABILITY_OFFSET) & TSCPMOD_AN_UF_PAGE1_MSA_25G_CR1_ABILITY_MASK)) {
                if (fec_cl74 || fec_cl91) {
                    if(fec_cl74) {
                        PHYMOD_IF_ERR_RETURN(
                           _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                 25000, 1, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                        if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_MSA_25G_CR1_ABILITY_OFFSET) & TSCPMOD_AN_UF_PAGE1_MSA_25G_CR1_ABILITY_MASK) {
                            autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                        }
                        i++;
                    }
                    if (fec_cl91) {
                        PHYMOD_IF_ERR_RETURN(
                           _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                 25000, 1, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                        if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_MSA_25G_CR1_ABILITY_OFFSET) & TSCPMOD_AN_UF_PAGE1_MSA_25G_CR1_ABILITY_MASK) {
                            autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                        }
                        i++;
                    }
                } else {
                    PHYMOD_IF_ERR_RETURN(
                      _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            25000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                    if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_MSA_25G_CR1_ABILITY_OFFSET) & TSCPMOD_AN_UF_PAGE1_MSA_25G_CR1_ABILITY_MASK) {
                        autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                    }
                    i++;
                }
            }
            if ((up1_2 >> TSCPMOD_AN_UF_PAGE2_MSA_LFR_OFFSET) & TSCPMOD_AN_UF_PAGE2_MSA_LFR_MASK) {
                /* 50g-1_lane */
                if (((up1_2 >> TSCPMOD_AN_UF_PAGE2_MSA_LF1_OFFSET) & TSCPMOD_AN_UF_PAGE2_MSA_LF1_MASK) &&
                    ((base_2 >> TSCPMOD_AN_BASE2_TECH_ABILITY_50G_KR1_CR1_OFFSET) & TSCPMOD_AN_BASE2_TECH_ABILITY_50G_KR1_CR1_MASK)) {
                    PHYMOD_IF_ERR_RETURN(
                        _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                         50000, 1, phymod_fec_RS272, pause,
                                                         phymod_channel_long, phymod_AN_MODE_MSA));
                    i++;
                    llf_req_50g = 1;
                }
                /* 100g-2_lane */
                if (((up1_2 >> TSCPMOD_AN_UF_PAGE2_MSA_LF2_OFFSET) & TSCPMOD_AN_UF_PAGE2_MSA_LF2_MASK) &&
                    ((base_2 >> TSCPMOD_AN_BASE2_TECH_ABILITY_100G_KR2_CR2_OFFSET) & TSCPMOD_AN_BASE2_TECH_ABILITY_100G_KR2_CR2_MASK)) {
                    PHYMOD_IF_ERR_RETURN(
                        _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                         100000, 2, phymod_fec_RS272, pause,
                                                         phymod_channel_long, phymod_AN_MODE_MSA));
                    i++;
                    llf_req_100g = 1;
                }
                /* 200g-4_lane */
                if (((up1_2 >> TSCPMOD_AN_UF_PAGE2_MSA_LF3_OFFSET) & TSCPMOD_AN_UF_PAGE2_MSA_LF3_MASK) &&
                    ((base_2 >> TSCPMOD_AN_BASE2_TECH_ABILITY_200G_KR4_CR4_OFFSET) & TSCPMOD_AN_BASE2_TECH_ABILITY_200G_KR4_CR4_MASK)) {
                    PHYMOD_IF_ERR_RETURN(
                        _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                         200000, 4, phymod_fec_RS272_2XN, pause,
                                                         phymod_channel_long, phymod_AN_MODE_MSA));
                    i++;
                    llf_req_200g = 1;
                }
            }
            if ((up1_2 >> TSCPMOD_AN_UF_PAGE2_MSA_400G_ABILITY_OFFSET) & TSCPMOD_AN_UF_PAGE2_MSA_400G_ABILITY_MASK) {
                PHYMOD_IF_ERR_RETURN(
                    _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                   400000, 8, phymod_fec_RS544_2XN, pause,
                                                   phymod_channel_long, phymod_AN_MODE_MSA));
                i++;
            }
            if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_MSA_800G_CR8_KR8_OFFSET) & TSCPMOD_AN_UF_PAGE1_MSA_800G_CR8_KR8_MASK) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        800000, 8, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_MSA));
                i++;
            }
        } else if (bam_code == TSCPMOD_BRCM_BAM_CODE) {
            /* UF page is BAM page */
            fec_cl74 = (up1_2 >> TSCPMOD_AN_UF_PAGE2_BAM_CL74_REQ_OFFSET) & TSCPMOD_AN_UF_PAGE2_BAM_CL74_REQ_MASK;
            fec_cl91 = (up1_2 >> TSCPMOD_AN_UF_PAGE2_BAM_CL91_REQ_OFFSET) & TSCPMOD_AN_UF_PAGE2_BAM_CL91_REQ_MASK;
            if ((up1_2 >> TSCPMOD_AN_UF_PAGE2_BAM_200G_BRCM_KR4_CR4_OFFSET) & TSCPMOD_AN_UF_PAGE2_BAM_200G_BRCM_KR4_CR4_MASK) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        200000, 4, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                i++;
            }
            if ((up1_2 >> TSCPMOD_AN_UF_PAGE2_BAM_200G_BRCM_NO_FEC_KR4_CR4_OFFSET) & TSCPMOD_AN_UF_PAGE2_BAM_200G_BRCM_NO_FEC_KR4_CR4_MASK) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        200000, 4, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                i++;
            }
            if ((up1_2 >> TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_FEC_528_KR2_CR2_OFFSET) & TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_FEC_528_KR2_CR2_MASK) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        100000, 2, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                i++;
            }
            if ((up1_2 >> TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_KR2_CR2_OFFSET) & TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_KR2_CR2_MASK) {
                PHYMOD_IF_ERR_RETURN(
                   _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                         100000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                i++;
            }
            if ((up1_2 >> TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_KR4_CR4_OFFSET) & TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_KR4_CR4_MASK) {
                PHYMOD_IF_ERR_RETURN(
                   _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                         100000, 4, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                i++;
            }
            if ((up1_2 >> TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_X4_OFFSET) & TSCPMOD_AN_UF_PAGE2_BAM_100G_BRCM_NO_FEC_X4_MASK) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        100000, 4, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                i++;
            }
            if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_50G_BRCM_FEC_528_CR1_KR1_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_50G_BRCM_FEC_528_CR1_KR1_MASK) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        50000, 1, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                i++;
            }
            if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_50G_BRCM_FEC_544_CR2_KR2_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_50G_BRCM_FEC_544_CR2_KR2_MASK) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        50000, 2, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                i++;
            }
            if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_800G_BRCM_CR8_KR8_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_800G_BRCM_CR8_KR8_MASK) {
                PHYMOD_IF_ERR_RETURN(
                  _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                        800000, 8, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                i++;
            }
            if (((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_50G_KR2_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_50G_KR2_MASK) ||
                ((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_50G_CR2_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_50G_CR2_MASK)) {
                if (fec_cl74 || fec_cl91) {
                    if (fec_cl74) {
                        PHYMOD_IF_ERR_RETURN(
                          _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                50000, 2, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                        if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_50G_CR2_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_50G_CR2_MASK) {
                          autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                        }
                        i++;
                    }
                    if (fec_cl91) {
                        PHYMOD_IF_ERR_RETURN(
                          _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                50000, 2, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                        if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_50G_CR2_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_50G_CR2_MASK) {
                          autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                        }
                        i++;
                    }
                } else {
                    PHYMOD_IF_ERR_RETURN(
                      _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            50000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                    if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_50G_CR2_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_50G_CR2_MASK) {
                      autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                    }
                    i++;
                }
            }
            if (((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_40G_KR2_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_40G_KR2_MASK) ||
                ((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_40G_CR2_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_40G_CR2_MASK)) {
                if (fec_cl74) {
                    PHYMOD_IF_ERR_RETURN(
                      _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            40000, 2, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                } else {
                    PHYMOD_IF_ERR_RETURN(
                      _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            40000, 2, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                }
                if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_40G_CR2_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_40G_CR2_MASK) {
                    autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                }
                i++;
            }
            if (((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_25G_KR1_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_25G_KR1_MASK) ||
                ((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_25G_CR1_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_25G_CR1_MASK)) {
                if (fec_cl74 || fec_cl91) {
                    if (fec_cl74) {
                        PHYMOD_IF_ERR_RETURN(
                          _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                25000, 1, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                        if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_25G_CR1_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_25G_CR1_MASK) {
                            autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                        }
                        i++;
                    }
                    if (fec_cl91) {
                        PHYMOD_IF_ERR_RETURN(
                          _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                                25000, 1, phymod_fec_CL91, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                        if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_25G_CR1_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_25G_CR1_MASK) {
                            autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                        }
                        i++;
                    }
                } else {
                    PHYMOD_IF_ERR_RETURN(
                      _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            25000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                    if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_25G_CR1_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_25G_CR1_MASK) {
                        autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                    }
                    i++;
                }
            }
            if (((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_20G_KR1_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_20G_KR1_MASK) ||
                ((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_20G_CR1_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_20G_CR1_MASK)) {
                if (fec_cl74) {
                    PHYMOD_IF_ERR_RETURN(
                      _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            20000, 1, phymod_fec_CL74, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                } else {
                    PHYMOD_IF_ERR_RETURN(
                      _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                            20000, 1, phymod_fec_None, pause, phymod_channel_long, phymod_AN_MODE_CL73BAM));
                }
                if ((up1_1 >> TSCPMOD_AN_UF_PAGE1_BAM_20G_CR1_OFFSET) & TSCPMOD_AN_UF_PAGE1_BAM_20G_CR1_MASK) {
                    autoneg_abilities->autoneg_abilities[i].medium = phymodFirmwareMediaTypeCopperCable;
                }
                i++;
            }
        }
    }

    if ((base_2 >> TSCPMOD_AN_BASE2_TECH_ABILITY_400G_KR4_CR4_OFFSET) & TSCPMOD_AN_BASE2_TECH_ABILITY_400G_KR4_CR4_MASK) {
        PHYMOD_IF_ERR_RETURN(
          _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                400000, 4, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        i++;
    }

    if ((base_2 >> TSCPMOD_AN_BASE2_TECH_ABILITY_800G_KR8_CR8_OFFSET) & TSCPMOD_AN_BASE2_TECH_ABILITY_800G_KR8_CR8_MASK) {
        PHYMOD_IF_ERR_RETURN(
          _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                800000, 8, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        i++;
    }
    if ((!llf_req_200g) &&
        ((base_2 >> TSCPMOD_AN_BASE2_TECH_ABILITY_200G_KR4_CR4_OFFSET) & TSCPMOD_AN_BASE2_TECH_ABILITY_200G_KR4_CR4_MASK)) {
        PHYMOD_IF_ERR_RETURN(
          _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                200000, 4, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        i++;
    }

    if ((!llf_req_200g) &&
        ((base_2 >> TSCPMOD_AN_BASE2_TECH_ABILITY_200G_KR2_CR2_OFFSET) & TSCPMOD_AN_BASE2_TECH_ABILITY_200G_KR2_CR2_MASK)) {
        PHYMOD_IF_ERR_RETURN(
          _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                200000, 2, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        i++;
    }

    if ((!llf_req_100g) &&
        ((base_2 >> TSCPMOD_AN_BASE2_TECH_ABILITY_100G_KR1_CR1_OFFSET) & TSCPMOD_AN_BASE2_TECH_ABILITY_100G_KR1_CR1_MASK)) {
        if ((base_2 >> TSCPMOD_AN_BASE3_RS_FEC_544_2XN_OFFSET) & TSCPMOD_AN_BASE3_RS_FEC_544_2XN_MASK) {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    100000, 1, phymod_fec_RS544_2XN, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        } else {
            PHYMOD_IF_ERR_RETURN(
              _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                    100000, 1, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        }
        i++;
    }

    if ((!llf_req_100g) &&
        ((base_2 >> TSCPMOD_AN_BASE2_TECH_ABILITY_100G_KR2_CR2_OFFSET) & TSCPMOD_AN_BASE2_TECH_ABILITY_100G_KR2_CR2_MASK)) {
        PHYMOD_IF_ERR_RETURN(
          _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                100000, 2, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        i++;
    }
    if ((!llf_req_50g) &&
        ((base_2 >> TSCPMOD_AN_BASE2_TECH_ABILITY_50G_KR1_CR1_OFFSET) & TSCPMOD_AN_BASE2_TECH_ABILITY_50G_KR1_CR1_MASK)) {
        PHYMOD_IF_ERR_RETURN(
          _plp_aperta2_tscpmod_an_ability_construct(&(autoneg_abilities->autoneg_abilities[i]),
                                50000, 1, phymod_fec_RS544, pause, phymod_channel_long, phymod_AN_MODE_CL73));
        i++;
    }

    autoneg_abilities->num_abilities = i;

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_pll_to_vco_get(tscpmod_refclk_t ref_clock, uint32_t pll, uint32_t *vco)
{
    if (ref_clock == TSCPMOD_REF_CLK_312P5MHZ) {
        switch  (pll) {
            case phymod_TSCBH_PLL_DIV170:
                 *vco = TSCPMOD_VCO_53G;
                 break;
            case phymod_TSCBH_PLL_DIV165:
                 *vco = TSCPMOD_VCO_51G;
                 break;
            case phymod_TSCBH_PLL_DIV132:
                 *vco = TSCPMOD_VCO_41G;
                 break;
            case phymod_TSCBH_PLL_DIVNONE:
                 *vco = TSCPMOD_VCO_NONE;
                 break;
            default:
                 *vco = TSCPMOD_VCO_INVALID;
                 break;
        }
    } else {
        PHYMOD_DEBUG_ERROR(("Unsupported reference clock.\n"));
        return PHYMOD_E_UNAVAIL;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_plldiv_lkup_get(PHYMOD_ST* pc, int mapped_speed_id, tscpmod_refclk_t refclk,  uint32_t *plldiv)
{
    if (refclk == TSCPMOD_REF_CLK_312P5MHZ) {
        *plldiv = plp_aperta2_tscpmod_sc_pmd_entry[mapped_speed_id].pll_mode;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_pmd_rx_lock_override_enable(PHYMOD_ST* pc, uint32_t enable)
{
      PMD_X4_OVRRr_t pmd_x4_override;
      PMD_X4_OVRRr_CLR(pmd_x4_override);
      PHYMOD_IF_ERR_RETURN(READ_PMD_X4_OVRRr(pc, &pmd_x4_override));
      PMD_X4_OVRRr_RX_LOCK_OVRDf_SET(pmd_x4_override, enable);
      PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_OVRRr(pc, pmd_x4_override));
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_polling_for_sc_done(PHYMOD_ST* pc)
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

int plp_aperta2_tscpmod_read_sc_done(PHYMOD_ST* pc)
{
    SC_X4_STSr_t SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg;

    SC_X4_STSr_CLR(SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_STSr(pc, &SC_X4_CONTROL_SC_X4_CONTROL_STATUSr_reg));
    return PHYMOD_E_NONE;
}


int plp_aperta2_tscpmod_read_sc_fsm_status(PHYMOD_ST* pc)
{
    SC_X4_DBGr_t debug_reg;

    SC_X4_DBGr_CLR(debug_reg);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_DBGr(pc, &debug_reg));
    return PHYMOD_E_NONE;
}

/* Enable PCS clock block */
int plp_aperta2_tscpmod_pcs_clk_blk_en(const PHYMOD_ST* pc, uint32_t en)
{
    AMS_PLL_PLL_CTL_13r_t reg_pll_ctrl;

    AMS_PLL_PLL_CTL_13r_CLR(reg_pll_ctrl);
    PHYMOD_IF_ERR_RETURN
       (READ_AMS_PLL_PLL_CTL_13r(pc, &reg_pll_ctrl));

    AMS_PLL_PLL_CTL_13r_AMS_PLL_EN_CLK4PCSf_SET(reg_pll_ctrl, en);

    PHYMOD_IF_ERR_RETURN
       (MODIFY_AMS_PLL_PLL_CTL_13r(pc, reg_pll_ctrl));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_port_start_lane_get(PHYMOD_ST *pc, int *port_starting_lane, int *port_num_lane)
{
    int start_lane, num_of_lane;
    PHYMOD_ST pc_copy;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    PHYMOD_MEMCPY(&pc_copy, pc, sizeof(pc_copy));
    PHYMOD_IF_ERR_RETURN
      (plp_aperta2_phymod_util_lane_config_get(&pc_copy.access, &start_lane, &num_of_lane));
   

    if (num_of_lane > 1) {
        *port_starting_lane = start_lane;
        *port_num_lane = num_of_lane;
    } else {
        MAIN0_SETUPr_t mode_reg;
        int port_mode_sel_reg;

        /* need to figure out the port starting lane basedon the port mode */
        PHYMOD_MEMCPY(&pc_copy, pc, sizeof(pc_copy));
        /* first read MPP0 port mode */
        pc_copy.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
        PHYMOD_IF_ERR_RETURN(READ_MAIN0_SETUPr(&pc_copy, &mode_reg));
        port_mode_sel_reg = MAIN0_SETUPr_PORT_MODE_SELf_GET(mode_reg);
        if (octal == 1) {
            start_lane -=8;
        }
        /*first check if multi MPP port mode */
        if (port_mode_sel_reg == TSCPMOD_MULTI_MPP_PORT) {
            *port_starting_lane = 0;
            *port_num_lane = 8;
        } else {
            /* first check which MPP based on the starting lane */
            if (start_lane  < 4) {
                switch (port_mode_sel_reg) {
                    case  TSCPMOD_SINGLE_PORT:
                        *port_starting_lane = 0;
                        /* need to add a check to decide if 400G 8 lane case */
                        *port_num_lane = 4;
                        break;
                    case  TSCPMOD_DUAL_PORT:
                        if (start_lane < 2) {
                            *port_starting_lane = 0;
                            *port_num_lane = 2;
                        } else {
                            *port_starting_lane = 2;
                            *port_num_lane = 2;
                        }
                        break;
                    case  TSCPMOD_TRI1_PORT:
                        if (start_lane < 2) {
                            *port_starting_lane = start_lane;
                            *port_num_lane = 1;
                        } else {
                            *port_starting_lane = 2;
                            *port_num_lane = 2;
                        }
                        break;
                    case  TSCPMOD_TRI2_PORT:
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
                pc_copy.access.lane_mask = 0x10 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
                PHYMOD_IF_ERR_RETURN(READ_MAIN0_SETUPr(&pc_copy, &mode_reg));
                port_mode_sel_reg = MAIN0_SETUPr_PORT_MODE_SELf_GET(mode_reg);
                switch (port_mode_sel_reg) {
                    case  TSCPMOD_SINGLE_PORT:
                        *port_starting_lane = 4;
                        *port_num_lane = 4;
                        break;
                    case  TSCPMOD_DUAL_PORT:
                        if (start_lane < 6) {
                            *port_starting_lane = 4;
                            *port_num_lane = 2;
                        } else {
                            *port_starting_lane = 6;
                            *port_num_lane = 2;
                        }
                        break;
                    case  TSCPMOD_TRI1_PORT:
                        if (start_lane < 6) {
                            *port_starting_lane = start_lane;
                            *port_num_lane = 1;
                        } else {
                            *port_starting_lane = 6;
                            *port_num_lane = 2;
                        }
                        break;
                    case  TSCPMOD_TRI2_PORT:
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
        if (octal == 1) {
            *port_starting_lane += 8;
        }
    }
    

    return PHYMOD_E_NONE;
}

/* Set FEC Bypass Error Indictator */
int plp_aperta2_tscpmod_fec_bypass_indication_set(PHYMOD_ST* pc, uint32_t rsfec_bypass_indication)
{
    RX_X4_RS_FEC_RX_CTL0r_t   RX_X4_RS_FEC_RX_CTL0r_reg;

    RX_X4_RS_FEC_RX_CTL0r_CLR(RX_X4_RS_FEC_RX_CTL0r_reg);
    PHYMOD_IF_ERR_RETURN(READ_RX_X4_RS_FEC_RX_CTL0r(pc, &RX_X4_RS_FEC_RX_CTL0r_reg));
    RX_X4_RS_FEC_RX_CTL0r_CW_BAD_ENABLEf_SET(RX_X4_RS_FEC_RX_CTL0r_reg, ~rsfec_bypass_indication);
    PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_RS_FEC_RX_CTL0r(pc, RX_X4_RS_FEC_RX_CTL0r_reg));

    return PHYMOD_E_NONE;
}


/* Get FEC Bypass Error Indictator */
int plp_aperta2_tscpmod_fec_bypass_indication_get(PHYMOD_ST *pc, uint32_t *rsfec_bypass_indication)
{
    uint32_t val = 0;
    RX_X4_RS_FEC_RX_CTL0r_t   RX_X4_RS_FEC_RX_CTL0r_reg;

    RX_X4_RS_FEC_RX_CTL0r_CLR(RX_X4_RS_FEC_RX_CTL0r_reg);
    PHYMOD_IF_ERR_RETURN(READ_RX_X4_RS_FEC_RX_CTL0r(pc, &RX_X4_RS_FEC_RX_CTL0r_reg));
    val = RX_X4_RS_FEC_RX_CTL0r_CW_BAD_ENABLEf_GET(RX_X4_RS_FEC_RX_CTL0r_reg);
    *rsfec_bypass_indication = (~val) & 0x1;

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_vco_to_pll_lkup(uint32_t vco, tscpmod_refclk_t refclk, uint32_t* pll_div)
{
    switch(refclk) {
        case TSCPMOD_REF_CLK_312P5MHZ:
            if (vco == TSCPMOD_VCO_41G) {
                *pll_div = TSCPMOD_PLL_MODE_DIV_132;
            } else if (vco == TSCPMOD_VCO_51G) {
                *pll_div = TSCPMOD_PLL_MODE_DIV_165;
            } else {
                *pll_div = TSCPMOD_PLL_MODE_DIV_170;
            }
            break;
        default:
            break;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_fec_cobra_enable(PHYMOD_ST *pc, uint32_t enable)
{
    RX_X1_RS_FEC_CFGr_t fec_config_reg;

    RX_X1_RS_FEC_CFGr_CLR(fec_config_reg);
    PHYMOD_IF_ERR_RETURN
        (READ_RX_X1_RS_FEC_CFGr(pc, &fec_config_reg));

    RX_X1_RS_FEC_CFGr_COBRA_ENABLEf_SET(fec_config_reg, enable);
    PHYMOD_IF_ERR_RETURN
        (MODIFY_RX_X1_RS_FEC_CFGr(pc, fec_config_reg));

    return PHYMOD_E_NONE;
}

/*!
 *  @brief tbhmod_pcs_ts_en per port.
@param unit number for instance lane number for decide which lane
@param ts_offset        - the timestamp location offset SOP/SFD/RPM.
@param rx/tx_base_addr  - 1588 lookup table base address.
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Enables ts in rx path */
int plp_aperta2_tscpmod_pcs_ts_config( PHYMOD_ST *pc, int ts_offset,
                           int rx_ts_base_addr, int tx_ts_base_addr )
{
    RX_X4_RX_TS_CTLr_t RX_X4_RX_TS_CTLr_reg;
    TX_X4_TX_TS_CTLr_t TX_X4_TX_TS_CTLr_reg;

    RX_X4_RX_TS_CTLr_CLR(RX_X4_RX_TS_CTLr_reg);
    TX_X4_TX_TS_CTLr_CLR(TX_X4_TX_TS_CTLr_reg);
    PHYMOD_IF_ERR_RETURN(READ_RX_X4_RX_TS_CTLr(pc, &RX_X4_RX_TS_CTLr_reg));
    PHYMOD_IF_ERR_RETURN(READ_TX_X4_TX_TS_CTLr(pc, &TX_X4_TX_TS_CTLr_reg));

    RX_X4_RX_TS_CTLr_RX_SOP_BYTE_OFFSETf_SET(RX_X4_RX_TS_CTLr_reg, ts_offset);
    TX_X4_TX_TS_CTLr_TX_SOP_BYTE_OFFSETf_SET(TX_X4_TX_TS_CTLr_reg, ts_offset);

    RX_X4_RX_TS_CTLr_RX_TS_BASE_ADDRESSf_SET(RX_X4_RX_TS_CTLr_reg, rx_ts_base_addr);
    TX_X4_TX_TS_CTLr_TX_TS_BASE_ADDRESSf_SET(TX_X4_TX_TS_CTLr_reg, tx_ts_base_addr);

    PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_RX_TS_CTLr(pc, RX_X4_RX_TS_CTLr_reg));
    PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_TX_TS_CTLr(pc, TX_X4_TX_TS_CTLr_reg));

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_pcs_rx_ts_en(PHYMOD_ST* pc, uint32_t en)
{
    RX_X4_RX_TS_CTLr_t RX_X4_RX_TS_CTLr_reg;

    RX_X4_RX_TS_CTLr_CLR(RX_X4_RX_TS_CTLr_reg);
    PHYMOD_IF_ERR_RETURN(READ_RX_X4_RX_TS_CTLr(pc, &RX_X4_RX_TS_CTLr_reg));
    RX_X4_RX_TS_CTLr_TS_UPDATE_ENABLEf_SET(RX_X4_RX_TS_CTLr_reg, en);

    PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_RX_TS_CTLr(pc, RX_X4_RX_TS_CTLr_reg));

    return PHYMOD_E_NONE;
}

/*!
 *  @brief tbhmod_set_rx_deskew_enb per port.
@param unit number for instance lane number for decide which lane
@param en tells enabling/disabling r_test_mode AKA scrmable_idle_en
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Enables ts in rx path */
int plp_aperta2_tscpmod_pcs_rx_deskew_en(PHYMOD_ST *pc, int en)
{
    RX_X4_RX_TS_CTLr_t RX_X4_RX_TS_CTLr_reg;

    RX_X4_RX_TS_CTLr_CLR(RX_X4_RX_TS_CTLr_reg);
    PHYMOD_IF_ERR_RETURN(READ_RX_X4_RX_TS_CTLr(pc, &RX_X4_RX_TS_CTLr_reg));
    RX_X4_RX_TS_CTLr_RECORD_DESKEW_TS_INFOf_SET(RX_X4_RX_TS_CTLr_reg, en);

    PHYMOD_IF_ERR_RETURN(MODIFY_RX_X4_RX_TS_CTLr(pc, RX_X4_RX_TS_CTLr_reg));

    return PHYMOD_E_NONE;
}

/*******************************************************************
 * OSR Mode : Encoded value
 *    OSRx1     : 4'd0   (NRZ and PAM4)
 *    OSRx2     : 4'd1   (NRZ and PAM4)
 *    OSRx4     : 4'd2   (NRZ ONLY)
 *    OSRx2.5   : 4'd3   (NRZ ONLY)
 *    OSRx8     : 4'd5   (NRZ ONLY)
 *    OSRx16    : 4'd9   (NRZ ONLY)
 *    OSRx32    : 4'd13  (NRZ ONLY)
 *    OSRx16P5  : 4'd8   (NRZ ONLY)
 *    OSRx20P625: 4'd12  (NRZ ONLY)
 *    OSRx21P25 : 4'd4   (NRZ ONLY)
 *    IN the below function it the encoded value which is being passed.
 * *****************************************************************/
#ifdef SERDES_API_FLOATING_POINT
float _plp_aperta2_tscpmod_pcs_vco_to_clk_period(uint32_t vco, int os_mode, int pam4)
{
    float clk_period = 0, vco_val = 0;

    switch (vco) {
        case TSCPMOD_VCO_41G:
            vco_val = 41.2500;
            break;
        case TSCPMOD_VCO_51G:
            vco_val = 51.625;
            break;
        case TSCPMOD_VCO_53G:
            vco_val = 53.125;
            break;
        default:
            break;
    }

    if (pam4) {
        switch(os_mode) {
            case 0:    /* OSRx1 */
                clk_period = 20.0/vco_val;
                break;
            case 1:    /* OSRx2 */
                clk_period = 40.0/vco_val;
                break;
            default: break;
        }
    } else {
        /* NRZ mode */
        switch(os_mode) {
            case 0:    /* OSRx1 */
                clk_period = 20.0/vco_val;
                break;
            case 1:     /* OSRx2 */
                clk_period = 40.0/vco_val;
                break;
            case 2:     /* OSRx4 */
                clk_period = 80.0/vco_val;
                break;
            case 3:        /* OSRx2.5 */
                clk_period = 50.0/vco_val;
                break;
            case 5:     /* OSRx8 */
                clk_period = 40.0/vco_val;
                break;
            case 9:     /* OSRx16 */
                clk_period = 40.0/vco_val;
                break;
            case 13:     /* OSRx32 */
                clk_period = 40.0/vco_val;
                break;
            case 4:     /* OSRx21P25 */
                clk_period = 20.0/vco_val;
                break;
            case 8:     /* OSRx16P5 */
                clk_period = 20.0/vco_val;
                break;
            case 12:     /* OSRx20P625 */
                clk_period = 20.0/vco_val;
                break;
            default:
                break;
        }

    }

    return clk_period;
}
#endif

/***********************************************************************************************************************/
#ifdef DV
uint32_t tscpmod_pcs_calc_ui_value(tsc_speed_id_e speed_id, int num_pcs_lanes)
{
    uint32_t ui_value;
    float bit_ui_val;

    ui_value = 0;

    bit_ui_val = tbhmod_pcs_calc_bit_ui_value(speed_id,num_pcs_lanes);

    ui_value = ((uint32_t)(bit_ui_val * 4294967296)) & 0xfffffe00;

    return ui_value;
}
#endif

/* FIXME */
uint32_t _plp_aperta2_tscpmod_pcs_vco_to_ui(uint32_t vco, int os_mode, int pam4)
{
    uint32_t ui_value;
    ui_value = 0;

    switch (vco) {
        case TSCPMOD_VCO_41G:
            /* OSR by 4 is not supported */
                /* TSCPMOD_OS_MODE_2 */
                /* 2/41.25 = 0.04848..
                 * make 32 bit 0.04848.. * 2^32 = 208240838 = 0xc6980c6
                 */
                ui_value = TSCPMOD_UI_41G_NRZ_OSX2;
            break;
        case TSCPMOD_VCO_51G:
            if (pam4) {
                if  (os_mode == TSCPMOD_OS_MODE_2) {
                    /* 2/(2*51.5625) = 0.019393939..
                     * make 32 bit 0.019393939.. * 2^32 =  83296335 = 0x04F7004F
                     */
                    ui_value = TSCPMOD_UI_51G_PAM4_OSX2;

                } else {
                    /* 1/(2*51.5625) = 0.009696969..
                     * make 32 bit 0.009696969.. * 2^32 = 41648167 = 0x27B8027
                     */
                    ui_value = TSCPMOD_UI_51G_PAM4_OSX1;
                }
            } else {
                if (os_mode == TSCPMOD_OS_MODE_5) { 
                /* 5/51.5625 = 0.09696..
                 * make 32 bit 0.09696.. * 2^32 = 416481676 = 0x18d3018c
                 */
                    ui_value = TSCPMOD_UI_51G_NRZ_OSX5;
                } else if  (os_mode == TSCPMOD_OS_MODE_2) {
                /* 2/51.5625 = 0.038787879..
                 * make 32 bit 0.038787879.. * 2^32 = 166592670 = 0x09ee009e
                 */
                    ui_value = TSCPMOD_UI_51G_NRZ_OSX2;
                } else { /* TSCPMOD_OS_MODE_1 */
                /* 1/51.5625  = 0.019393939..
                 * make 32 bit 0.019393939.. * 2^32 = 83296335 = 0x04F7004F
                 */
                    ui_value = TSCPMOD_UI_51G_NRZ_OSX1;
                }
            }
            break;
        case TSCPMOD_VCO_53G:
            if (pam4) {
                if  (os_mode == TSCPMOD_OS_MODE_2) {
                    /* 2/(2*53.125) = 0.0188235294117647..
                    * make 32 bit 0.0188235294.. * 2^32 = 80846443 = 0x04D19E6B
                    */
                    ui_value = TSCPMOD_UI_53G_PAM4_OSX2;
                } else {
                    /* 1/(2*53.125) = 0.0094117647058824..
                    * make 32 bit 0.0094117647058824.. * 2^32 = 40423221 = 0x268CF35
                    */
                    ui_value = TSCPMOD_UI_53G_PAM4_OSX1;
                }
            } else {
               if  (os_mode == TSCPMOD_OS_MODE_2) {
                /* 2/53.125 = 0.037647059..
                 * make 32 bit 0.037647059.. * 2^32 = 161692887 = 0x09A33CD7
                 */
                    ui_value = TSCPMOD_UI_53G_NRZ_OSX2;
                } else { /* TSCPMOD_OS_MODE_1 */
                /* 1/53.125  = 0.0188235294117647..
                 * make 32 bit 0.0188235294.. * 2^32 = 80846443 = 0x04D19E6B
                 */
                    ui_value = TSCPMOD_UI_53G_NRZ_OSX1;
                }
            }
            break;
        default:
            return PHYMOD_E_PARAM;
    }

    return ui_value;
}

#ifdef SERDES_API_FLOATING_POINT
float _plp_aperta2_tscpmod_pcs_calc_bit_ui_value(uint32_t vco, int os_mode, int pam4)
{
    uint32_t ui_hi_lo;
    float bit_ui_val, bit_ui_val1, bit_ui_val2;

    ui_hi_lo = _plp_aperta2_tscpmod_pcs_vco_to_ui(vco, os_mode, pam4);
    bit_ui_val1 = ((float) ui_hi_lo / 1024);
    bit_ui_val2 = ((float) bit_ui_val1 / 1024);
    bit_ui_val = ((float) bit_ui_val2 / 4096);

    return bit_ui_val;

}

float _plp_aperta2_tscpmod_pcs_vco_to_tx_clk_period(uint32_t vco, int os_mode, int pam4)
{
    float clk_period;
    float bit_ui_val;
    clk_period = 0.0;

    bit_ui_val = _plp_aperta2_tscpmod_pcs_calc_bit_ui_value(vco, os_mode, pam4);

    clk_period = 40.0 * bit_ui_val;

    return clk_period;
}
#endif

int plp_aperta2_tscpmod_set_fclk_period(PHYMOD_ST *pc, uint32_t vco, int clk4sync_div)
{
    PMD_X1_FCLK_PERIODr_t PMD_X1_FCLK_PERIODr_reg;
    int fclk_period;
    PHYMOD_ST phy_copy;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    switch (vco) {
        case TSCPMOD_VCO_41G:
            if (clk4sync_div) {
                fclk_period = TSCPMOD_FCLK_PERIOD_41G_DIV8;
            } else {
                fclk_period = TSCPMOD_FCLK_PERIOD_41G_DIV6;
            }
            break;
        case TSCPMOD_VCO_51G:
            fclk_period = TSCPMOD_FCLK_PERIOD_51G;
            break;
        case TSCPMOD_VCO_53G:
            fclk_period = TSCPMOD_FCLK_PERIOD_53G;
            break;
        default:
            return PHYMOD_E_PARAM;
    }

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));

    PMD_X1_FCLK_PERIODr_CLR(PMD_X1_FCLK_PERIODr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X1_FCLK_PERIODr(&phy_copy, &PMD_X1_FCLK_PERIODr_reg));
    PMD_X1_FCLK_PERIODr_FCLK_FRAC_NSf_SET(PMD_X1_FCLK_PERIODr_reg, fclk_period);

    /* Configure MPP0 */
    phy_copy.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_FCLK_PERIODr(&phy_copy, PMD_X1_FCLK_PERIODr_reg));

    /* Configure MPP1 */
    phy_copy.access.lane_mask = 0x10 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X1_FCLK_PERIODr(&phy_copy, PMD_X1_FCLK_PERIODr_reg));

    return PHYMOD_E_NONE;
}

/*!
 *  @brief tbhmod_set_1588_ui per port.
@param unit number for instance lane number for decide which lane
@param vco tells which vco is used in the lane
@param os_mode tells which os_mode is used in the lane
@param clk4sync_div the fast clk divider 0 => 8 and 1 => 6
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Enables ts in rx path */
int plp_aperta2_tscpmod_pcs_set_1588_ui(PHYMOD_ST *pc, uint32_t vco, int os_mode, int pam4)
{
    PMD_X4_UI_VALUE_HIr_t PMD_X4_UI_VALUE_HIr_reg;
    PMD_X4_UI_VALUE_LOr_t PMD_X4_UI_VALUE_LOr_reg;
    int ui_value_hi, ui_value_lo;

    ui_value_hi = _plp_aperta2_tscpmod_pcs_vco_to_ui(vco, os_mode, pam4) >> 16;
    ui_value_lo = (_plp_aperta2_tscpmod_pcs_vco_to_ui(vco, os_mode, pam4) & 0xffff) >> 9;

    PMD_X4_UI_VALUE_HIr_CLR(PMD_X4_UI_VALUE_HIr_reg);
    PMD_X4_UI_VALUE_LOr_CLR(PMD_X4_UI_VALUE_LOr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_UI_VALUE_HIr(pc, &PMD_X4_UI_VALUE_HIr_reg));
    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_UI_VALUE_LOr(pc, &PMD_X4_UI_VALUE_LOr_reg));

    PMD_X4_UI_VALUE_HIr_UI_FRAC_M1_TO_M16f_SET(PMD_X4_UI_VALUE_HIr_reg, ui_value_hi);
    PMD_X4_UI_VALUE_LOr_UI_FRAC_M17_TO_M23f_SET(PMD_X4_UI_VALUE_LOr_reg, ui_value_lo);

    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_UI_VALUE_HIr(pc, PMD_X4_UI_VALUE_HIr_reg));
    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_UI_VALUE_LOr(pc, PMD_X4_UI_VALUE_LOr_reg));

    return PHYMOD_E_NONE;
}



/*!
 *  @brief tbhmod_calc_tx_pmd_latency per port.
@param vco tells which vco is used in the lane
@param os_mode tells which os_mode is used in the lane
@param is_pam4 tells pam4 or NRZ
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Calculate the Tx Latency. */
int _plp_aperta2_tscpmod_calc_tx_pmd_latency(uint32_t vco, uint32_t os_mode, int pam4)
{
    uint32_t tx_latency = 0;
#ifdef SERDES_API_FLOATING_POINT
    float bit_ui_val;

    bit_ui_val = _plp_aperta2_tscpmod_pcs_calc_bit_ui_value(vco, os_mode, pam4);

    /***
    * Please refer to the Peregrine user spec datapath latency table
    * For Pam4 each UI == 2bits hence we multiple by 2
    * The TX fixed latency register has 8 bits of subnano and hence multiplied by 2**8.
    *****/
    if (pam4) {
        /* PAM4 */
        tx_latency = ((uint32_t)((bit_ui_val * (TSCPMOD_PMD_TX_DP_LATENCY * 2)) * 256));
    } else {
        /* NRZ */
        tx_latency = ((uint32_t)(bit_ui_val * TSCPMOD_PMD_TX_DP_LATENCY * 256));
    }
#endif

    return tx_latency;
}

int plp_aperta2_tscpmod_tx_pmd_latency_get(PHYMOD_ST *pc, int *tx_latency)
{

    PMD_X4_TX_FIXED_LATENCYr_t PMD_X4_TX_FIXED_LATENCYr_reg;

    PMD_X4_TX_FIXED_LATENCYr_CLR(PMD_X4_TX_FIXED_LATENCYr_reg);

    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_TX_FIXED_LATENCYr(pc, &PMD_X4_TX_FIXED_LATENCYr_reg));

    *tx_latency = PMD_X4_TX_FIXED_LATENCYr_TX_PMD_LATENCY_IN_NSf_GET(PMD_X4_TX_FIXED_LATENCYr_reg) << 8;
    *tx_latency = *tx_latency | (PMD_X4_TX_FIXED_LATENCYr_TX_PMD_LATENCY_IN_FRAC_NSf_GET(PMD_X4_TX_FIXED_LATENCYr_reg) & 0xff);
    /* The pmd latency has 8 bits of ns and 8 bits of subns. Need to convert it to
     * the same format of timestamp which has 4bits of subns.
     * TX_PMD_LATENCY_IN_NS[15:8] TX_PMD_LATENCY_IN_FRAC_NS[7:0]
     */
    *tx_latency = *tx_latency >> 4;

    return PHYMOD_E_NONE;
}


/*!
 *  @brief tbhmod_set_tx_pmd_latency per port.
@param unit number for instance lane number for decide which lane
@param vco tells which vco is used in the lane
@param os_mode tells which os_mode is used in the lane
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Enables ts in rx path */
int plp_aperta2_tscpmod_tx_pmd_latency_set(PHYMOD_ST *pc, uint32_t vco, int os_mode, int pam4)
{
    int tx_latency;
    PMD_X4_TX_FIXED_LATENCYr_t PMD_X4_TX_FIXED_LATENCYr_reg;

    tx_latency = _plp_aperta2_tscpmod_calc_tx_pmd_latency(vco, os_mode, pam4);

    /* 
     * latency above is calculated with 8 bits of subns.
     */
    PMD_X4_TX_FIXED_LATENCYr_CLR(PMD_X4_TX_FIXED_LATENCYr_reg);
    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_TX_FIXED_LATENCYr(pc, &PMD_X4_TX_FIXED_LATENCYr_reg));
    PMD_X4_TX_FIXED_LATENCYr_TX_PMD_LATENCY_IN_NSf_SET(PMD_X4_TX_FIXED_LATENCYr_reg, (tx_latency >> 8));
    PMD_X4_TX_FIXED_LATENCYr_TX_PMD_LATENCY_IN_FRAC_NSf_SET(PMD_X4_TX_FIXED_LATENCYr_reg, (tx_latency & 0xff));

    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_TX_FIXED_LATENCYr(pc, PMD_X4_TX_FIXED_LATENCYr_reg));

    return PHYMOD_E_NONE;
}

int _plp_aperta2_tscpmod_update_tx_pmd_latency(PHYMOD_ST *pc, uint32_t latency_adj, int normalize_to_latest)
{
    uint32_t tx_latency, latency_ns, latency_frac_ns;
    PMD_X4_TX_FIXED_LATENCYr_t PMD_X4_TX_FIXED_LATENCYr_reg;

    PMD_X4_TX_FIXED_LATENCYr_CLR(PMD_X4_TX_FIXED_LATENCYr_reg);

    PHYMOD_IF_ERR_RETURN(READ_PMD_X4_TX_FIXED_LATENCYr(pc, &PMD_X4_TX_FIXED_LATENCYr_reg));
    latency_ns = PMD_X4_TX_FIXED_LATENCYr_TX_PMD_LATENCY_IN_NSf_GET(PMD_X4_TX_FIXED_LATENCYr_reg);
    latency_frac_ns = (PMD_X4_TX_FIXED_LATENCYr_TX_PMD_LATENCY_IN_FRAC_NSf_GET(PMD_X4_TX_FIXED_LATENCYr_reg)<<2);
    if (normalize_to_latest == 1) {
        tx_latency = ((latency_ns << 10) | latency_frac_ns) + latency_adj;
    } else {
        /* check for signed bit. */
        if ((latency_ns >> 7) & 1)   {
            tx_latency = ((latency_ns << 10) | latency_frac_ns) + latency_adj;
        } else {
            tx_latency = ((latency_ns << 10) | latency_frac_ns) - latency_adj;
        }
    }

    PMD_X4_TX_FIXED_LATENCYr_TX_PMD_LATENCY_IN_NSf_SET(PMD_X4_TX_FIXED_LATENCYr_reg, (tx_latency >> 10));
    PMD_X4_TX_FIXED_LATENCYr_TX_PMD_LATENCY_IN_FRAC_NSf_SET(PMD_X4_TX_FIXED_LATENCYr_reg, ((tx_latency & 0x3ff)>>2));

    PHYMOD_IF_ERR_RETURN(MODIFY_PMD_X4_TX_FIXED_LATENCYr(pc, PMD_X4_TX_FIXED_LATENCYr_reg));

    return PHYMOD_E_NONE;
}


/*!
 *  @brief tbhmod_set_tx_lane_skew_capture per port.
@param unit number for instance lane number for decide which lane
@param vco tells which vco is used in the lane
@param os_mode tells which os_mode is used in the lane
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Enables ts in rx path */
int plp_aperta2_tscpmod_pcs_set_tx_lane_skew_capture(PHYMOD_ST *pc, int tx_skew_en)
{
    TX_X1_GLAS_TPMA_CTLr_t   TX_X1_GLAS_TPMA_CTLr_reg;
    PHYMOD_ST phy_copy;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));

    TX_X1_GLAS_TPMA_CTLr_CLR(TX_X1_GLAS_TPMA_CTLr_reg);
    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_CTLr(&phy_copy, &TX_X1_GLAS_TPMA_CTLr_reg));
    TX_X1_GLAS_TPMA_CTLr_GLAS_TPMA_CAPTURE_ENf_SET(TX_X1_GLAS_TPMA_CTLr_reg, tx_skew_en);
    TX_X1_GLAS_TPMA_CTLr_GLAS_TPMA_CAPTURE_MASKf_SET(TX_X1_GLAS_TPMA_CTLr_reg, pc->access.lane_mask);
    TX_X1_GLAS_TPMA_CTLr_GLAS_TPMA_CAPTURE_LOGICALf_SET(TX_X1_GLAS_TPMA_CTLr_reg, 1);
    phy_copy.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    PHYMOD_IF_ERR_RETURN(MODIFY_TX_X1_GLAS_TPMA_CTLr(&phy_copy, TX_X1_GLAS_TPMA_CTLr_reg));

    return PHYMOD_E_NONE;
}

/*!
 *  @brief tbhmod_measure_tx_lane_skew per port.
@param unit number for instance lane number for decide which lane
@param vco tells which vco is used in the lane
@param os_mode tells which os_mode is used in the lane
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Enables ts in rx path */
int plp_aperta2_tscpmod_pcs_measure_tx_lane_skew(PHYMOD_ST *pc, uint32_t vco, int os_mode, int pam4,
                                     int pma_width_multiplier, int normalize_to_latest, int *tx_max_skew)
{
#ifdef SERDES_API_FLOATING_POINT
    int32_t latency_adj;
    float bit_ui_val;
    uint32_t fclk_frac_ns;
    uint32_t tx_lane_skew_bits[16], max_lane_skew_bits, min_lane_skew_bits;
    uint32_t tx_lane_skew_bits_ro[16], max_lane_skew_bits_ro, min_lane_skew_bits_ro;
    uint32_t max_min_diff;
    uint32_t rollover;
    uint16_t glas_tpma_capture_data[16];
    uint8_t glas_tpma_capture_adj[16];
    float fclk_period, fclk_period_tmp, no_of_bits;
    int curr_lane;
    int i, start_lane, num_lane;
    int32_t normalized_tx_lane_skew_bits[16];
    PHYMOD_ST phy_copy;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    TX_X1_GLAS_TPMA_DATA0r_t TX_X1_GLAS_TPMA_DATA0r_reg;
    TX_X1_GLAS_TPMA_DATA1r_t TX_X1_GLAS_TPMA_DATA1r_reg;
    TX_X1_GLAS_TPMA_DATA2r_t TX_X1_GLAS_TPMA_DATA2r_reg;
    TX_X1_GLAS_TPMA_DATA3r_t TX_X1_GLAS_TPMA_DATA3r_reg;
    TX_X1_GLAS_TPMA_DATA4r_t TX_X1_GLAS_TPMA_DATA4r_reg;
    TX_X1_GLAS_TPMA_DATA5r_t TX_X1_GLAS_TPMA_DATA5r_reg;
    TX_X1_GLAS_TPMA_DATA6r_t TX_X1_GLAS_TPMA_DATA6r_reg;
    TX_X1_GLAS_TPMA_DATA7r_t TX_X1_GLAS_TPMA_DATA7r_reg;

    TX_X1_GLAS_TPMA_ADJ_0_1r_t TX_X1_GLAS_TPMA_ADJ_0_1r_reg;
    TX_X1_GLAS_TPMA_ADJ_2_3r_t TX_X1_GLAS_TPMA_ADJ_2_3r_reg;
    TX_X1_GLAS_TPMA_ADJ_4_5r_t TX_X1_GLAS_TPMA_ADJ_4_5r_reg;
    TX_X1_GLAS_TPMA_ADJ_6_7r_t TX_X1_GLAS_TPMA_ADJ_6_7r_reg;

    PMD_X1_FCLK_PERIODr_t PMD_X1_FCLK_PERIODr_reg;

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));

    for (i = 0; i < 16; i++) {
        tx_lane_skew_bits[i] = 0;
        normalized_tx_lane_skew_bits[i] = 0;
        glas_tpma_capture_data[i] = 0;
        glas_tpma_capture_adj[i] = 0;
    }


    max_lane_skew_bits = 0x0;
    max_lane_skew_bits_ro = 0x0;
    no_of_bits = 0;

    if (pma_width_multiplier == 2) {
        min_lane_skew_bits = 0x8fff;
        min_lane_skew_bits_ro = 0x8fff;
        max_min_diff = 0x4000;
        rollover = 0x8000;
    } else {
        min_lane_skew_bits = 0x4fff;
        min_lane_skew_bits_ro = 0x4fff;
        max_min_diff = 0x2000;
        rollover = 0x4000;
    }

    bit_ui_val = _plp_aperta2_tscpmod_pcs_calc_bit_ui_value(vco, os_mode, pam4);

    if (((start_lane >= 0) && (start_lane < 4)) ||
         ((start_lane >=8) && (start_lane < 12))) {
        phy_copy.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    } else {
        phy_copy.access.lane_mask = 0x10 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    }
    PHYMOD_IF_ERR_RETURN(READ_PMD_X1_FCLK_PERIODr(&phy_copy, &PMD_X1_FCLK_PERIODr_reg));
    fclk_frac_ns = PMD_X1_FCLK_PERIODr_FCLK_FRAC_NSf_GET(PMD_X1_FCLK_PERIODr_reg);
    fclk_period_tmp =  fclk_frac_ns / 1024.0;
    fclk_period = fclk_period_tmp / 64.0;

    TX_X1_GLAS_TPMA_DATA0r_CLR(TX_X1_GLAS_TPMA_DATA0r_reg);
    TX_X1_GLAS_TPMA_DATA1r_CLR(TX_X1_GLAS_TPMA_DATA1r_reg);
    TX_X1_GLAS_TPMA_DATA2r_CLR(TX_X1_GLAS_TPMA_DATA2r_reg);
    TX_X1_GLAS_TPMA_DATA3r_CLR(TX_X1_GLAS_TPMA_DATA3r_reg);
    TX_X1_GLAS_TPMA_DATA4r_CLR(TX_X1_GLAS_TPMA_DATA4r_reg);
    TX_X1_GLAS_TPMA_DATA5r_CLR(TX_X1_GLAS_TPMA_DATA5r_reg);
    TX_X1_GLAS_TPMA_DATA6r_CLR(TX_X1_GLAS_TPMA_DATA6r_reg);
    TX_X1_GLAS_TPMA_DATA7r_CLR(TX_X1_GLAS_TPMA_DATA7r_reg);
    TX_X1_GLAS_TPMA_ADJ_0_1r_CLR(TX_X1_GLAS_TPMA_ADJ_0_1r_reg);
    TX_X1_GLAS_TPMA_ADJ_2_3r_CLR(TX_X1_GLAS_TPMA_ADJ_2_3r_reg);
    TX_X1_GLAS_TPMA_ADJ_4_5r_CLR(TX_X1_GLAS_TPMA_ADJ_4_5r_reg);
    TX_X1_GLAS_TPMA_ADJ_6_7r_CLR(TX_X1_GLAS_TPMA_ADJ_6_7r_reg);

    phy_copy.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA0r(&phy_copy, &TX_X1_GLAS_TPMA_DATA0r_reg));
    glas_tpma_capture_data[0] = TX_X1_GLAS_TPMA_DATA0r_GLAS_TPMA_CAPTURE_DATA_0f_GET(TX_X1_GLAS_TPMA_DATA0r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA1r(&phy_copy, &TX_X1_GLAS_TPMA_DATA1r_reg));
    glas_tpma_capture_data[1] = TX_X1_GLAS_TPMA_DATA1r_GLAS_TPMA_CAPTURE_DATA_1f_GET(TX_X1_GLAS_TPMA_DATA1r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA2r(&phy_copy, &TX_X1_GLAS_TPMA_DATA2r_reg));
    glas_tpma_capture_data[2] = TX_X1_GLAS_TPMA_DATA2r_GLAS_TPMA_CAPTURE_DATA_2f_GET(TX_X1_GLAS_TPMA_DATA2r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA3r(&phy_copy, &TX_X1_GLAS_TPMA_DATA3r_reg));
    glas_tpma_capture_data[3] = TX_X1_GLAS_TPMA_DATA3r_GLAS_TPMA_CAPTURE_DATA_3f_GET(TX_X1_GLAS_TPMA_DATA3r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA4r(&phy_copy, &TX_X1_GLAS_TPMA_DATA4r_reg));
    glas_tpma_capture_data[4] = TX_X1_GLAS_TPMA_DATA4r_GLAS_TPMA_CAPTURE_DATA_4f_GET(TX_X1_GLAS_TPMA_DATA4r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA5r(&phy_copy, &TX_X1_GLAS_TPMA_DATA5r_reg));
    glas_tpma_capture_data[5] = TX_X1_GLAS_TPMA_DATA5r_GLAS_TPMA_CAPTURE_DATA_5f_GET(TX_X1_GLAS_TPMA_DATA5r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA6r(&phy_copy, &TX_X1_GLAS_TPMA_DATA6r_reg));
    glas_tpma_capture_data[6] = TX_X1_GLAS_TPMA_DATA6r_GLAS_TPMA_CAPTURE_DATA_6f_GET(TX_X1_GLAS_TPMA_DATA6r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA7r(&phy_copy, &TX_X1_GLAS_TPMA_DATA7r_reg));
    glas_tpma_capture_data[7] = TX_X1_GLAS_TPMA_DATA7r_GLAS_TPMA_CAPTURE_DATA_7f_GET(TX_X1_GLAS_TPMA_DATA7r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_ADJ_0_1r(&phy_copy, &TX_X1_GLAS_TPMA_ADJ_0_1r_reg));
    glas_tpma_capture_adj[0] = TX_X1_GLAS_TPMA_ADJ_0_1r_GLAS_TPMA_CAPTURE_ADJ_0f_GET(TX_X1_GLAS_TPMA_ADJ_0_1r_reg);
    glas_tpma_capture_adj[1] = TX_X1_GLAS_TPMA_ADJ_0_1r_GLAS_TPMA_CAPTURE_ADJ_1f_GET(TX_X1_GLAS_TPMA_ADJ_0_1r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_ADJ_2_3r(&phy_copy, &TX_X1_GLAS_TPMA_ADJ_2_3r_reg));
    glas_tpma_capture_adj[2] = TX_X1_GLAS_TPMA_ADJ_2_3r_GLAS_TPMA_CAPTURE_ADJ_2f_GET(TX_X1_GLAS_TPMA_ADJ_2_3r_reg);
    glas_tpma_capture_adj[3] = TX_X1_GLAS_TPMA_ADJ_2_3r_GLAS_TPMA_CAPTURE_ADJ_3f_GET(TX_X1_GLAS_TPMA_ADJ_2_3r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_ADJ_4_5r(&phy_copy, &TX_X1_GLAS_TPMA_ADJ_4_5r_reg));
    glas_tpma_capture_adj[4] = TX_X1_GLAS_TPMA_ADJ_4_5r_GLAS_TPMA_CAPTURE_ADJ_4f_GET(TX_X1_GLAS_TPMA_ADJ_4_5r_reg);
    glas_tpma_capture_adj[5] = TX_X1_GLAS_TPMA_ADJ_4_5r_GLAS_TPMA_CAPTURE_ADJ_5f_GET(TX_X1_GLAS_TPMA_ADJ_4_5r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_ADJ_6_7r(&phy_copy, &TX_X1_GLAS_TPMA_ADJ_6_7r_reg));
    glas_tpma_capture_adj[6] = TX_X1_GLAS_TPMA_ADJ_6_7r_GLAS_TPMA_CAPTURE_ADJ_6f_GET(TX_X1_GLAS_TPMA_ADJ_6_7r_reg);
    glas_tpma_capture_adj[7] = TX_X1_GLAS_TPMA_ADJ_6_7r_GLAS_TPMA_CAPTURE_ADJ_7f_GET(TX_X1_GLAS_TPMA_ADJ_6_7r_reg);


    no_of_bits = fclk_period / bit_ui_val;

    for (i = 0; i < num_lane; i++) {
        tx_lane_skew_bits[start_lane + i] = (glas_tpma_capture_data[start_lane + i] * pma_width_multiplier) +
                                            (glas_tpma_capture_adj[start_lane + i] * no_of_bits);
        if (tx_lane_skew_bits[start_lane + i] > max_lane_skew_bits) {
            max_lane_skew_bits = tx_lane_skew_bits[start_lane + i];
        }
        if (tx_lane_skew_bits[start_lane + i] < min_lane_skew_bits) {
            min_lane_skew_bits = tx_lane_skew_bits[start_lane + i];
        }
    }

    if (normalize_to_latest == 1) {
        if ((max_lane_skew_bits - min_lane_skew_bits) > max_min_diff) {
            for (i = 0; i < num_lane; i++) {
                if (max_lane_skew_bits - tx_lane_skew_bits[start_lane + i] > max_min_diff) {
                    tx_lane_skew_bits_ro[start_lane + i] = rollover | tx_lane_skew_bits[start_lane + i];
                } else {
                    tx_lane_skew_bits_ro[start_lane + i] = tx_lane_skew_bits[start_lane + i];
                }
                if (tx_lane_skew_bits_ro[start_lane + i] > max_lane_skew_bits_ro) {
                    max_lane_skew_bits_ro = tx_lane_skew_bits_ro[start_lane + i];
                }
                if (tx_lane_skew_bits_ro[start_lane + i] < min_lane_skew_bits_ro) {
                    min_lane_skew_bits_ro = tx_lane_skew_bits_ro[start_lane + i];
                }
            }
            for (i = 0; i < num_lane; i++) {
                normalized_tx_lane_skew_bits[start_lane + i] = tx_lane_skew_bits_ro[start_lane + i] - min_lane_skew_bits_ro;
            }
            *tx_max_skew = ((uint32_t) (max_lane_skew_bits_ro - min_lane_skew_bits_ro) * bit_ui_val * 1024.0);
        } else {
            for (i = 0; i < num_lane; i++) {
                normalized_tx_lane_skew_bits[start_lane + i] = tx_lane_skew_bits[start_lane + i] - min_lane_skew_bits;
            }
            *tx_max_skew = ((uint32_t)(max_lane_skew_bits - min_lane_skew_bits) * bit_ui_val * 1024.0);
        }
    } else { /*Normalize to earliest */
        if ((max_lane_skew_bits - min_lane_skew_bits) > max_min_diff) {
            for (i = 0; i < num_lane; i++) {
                if (max_lane_skew_bits - tx_lane_skew_bits[start_lane + i] > max_min_diff) {
                    tx_lane_skew_bits_ro[start_lane + i] = rollover | tx_lane_skew_bits[start_lane + i];
                } else {
                    tx_lane_skew_bits_ro[start_lane + i] = tx_lane_skew_bits[start_lane + i];
                }
                if (tx_lane_skew_bits_ro[start_lane + i] > max_lane_skew_bits_ro) {
                    max_lane_skew_bits_ro = tx_lane_skew_bits_ro[start_lane + i];
                }
                if (tx_lane_skew_bits_ro[start_lane + i] < min_lane_skew_bits_ro) {
                    min_lane_skew_bits_ro = tx_lane_skew_bits_ro[start_lane + i];
                }
            }
            for (i = 0; i < num_lane; i++) {
                normalized_tx_lane_skew_bits[start_lane + i] = max_lane_skew_bits_ro - tx_lane_skew_bits_ro[start_lane + i];
            }
            *tx_max_skew = ((uint32_t) (max_lane_skew_bits_ro - min_lane_skew_bits_ro) * bit_ui_val * 1024.0);
        } else {
            for (i = 0; i < num_lane; i++) {
                normalized_tx_lane_skew_bits[start_lane + i] = max_lane_skew_bits - tx_lane_skew_bits[start_lane + i];
            }
            *tx_max_skew = ((uint32_t)(max_lane_skew_bits - min_lane_skew_bits) * bit_ui_val * 1024.0);
        }
    }

    for (i = 0; i < num_lane; i++) {
        curr_lane = start_lane + i;
        latency_adj = normalized_tx_lane_skew_bits[curr_lane] * bit_ui_val * 1024;
        phy_copy.access.lane_mask = 0x1 << curr_lane;
        PHYMOD_IF_ERR_RETURN
            (_plp_aperta2_tscpmod_update_tx_pmd_latency(&phy_copy, latency_adj, normalize_to_latest));
    }
#endif
    return PHYMOD_E_NONE;
}


/*!
 *  @brief tbhmod_renormalize_tx_lane_skew_to_lane per port.
@param unit number for instance lane number for decide which lane
@param vco tells which vco is used in the lane
@param os_mode tells which os_mode is used in the lane
@returns PHYMOD_E_NONE if no errors. PHYMOD_EERROR else.
@details Enables ts in rx path */
int plp_aperta2_tscpmod_measure_n_normalize_tx_lane_skew(PHYMOD_ST *pc, uint32_t vco, int os_mode, int is_pam4, int fixed_lane, int *tx_max_skew)
{
#ifdef SERDES_API_FLOATING_POINT
    int32_t latency_adj;
    float bit_ui_val;
    uint32_t fclk_frac_ns;
    uint32_t tx_lane_skew_bits[16], max_lane_skew_bits, min_lane_skew_bits;
    uint32_t tx_lane_skew_bits_ro[16], max_lane_skew_bits_ro, min_lane_skew_bits_ro;
    uint16_t glas_tpma_capture_data[16];
    uint8_t glas_tpma_capture_adj[16];
    float fclk_period, fclk_period_tmp, no_of_bits;
    int curr_lane;
    int i, start_lane, num_lane;
    int32_t normalized_tx_lane_skew_bits[16];
    PHYMOD_ST phy_copy;

    TX_X1_GLAS_TPMA_DATA0r_t TX_X1_GLAS_TPMA_DATA0r_reg;
    TX_X1_GLAS_TPMA_DATA1r_t TX_X1_GLAS_TPMA_DATA1r_reg;
    TX_X1_GLAS_TPMA_DATA2r_t TX_X1_GLAS_TPMA_DATA2r_reg;
    TX_X1_GLAS_TPMA_DATA3r_t TX_X1_GLAS_TPMA_DATA3r_reg;
    TX_X1_GLAS_TPMA_DATA4r_t TX_X1_GLAS_TPMA_DATA4r_reg;
    TX_X1_GLAS_TPMA_DATA5r_t TX_X1_GLAS_TPMA_DATA5r_reg;
    TX_X1_GLAS_TPMA_DATA6r_t TX_X1_GLAS_TPMA_DATA6r_reg;
    TX_X1_GLAS_TPMA_DATA7r_t TX_X1_GLAS_TPMA_DATA7r_reg;

    TX_X1_GLAS_TPMA_ADJ_0_1r_t TX_X1_GLAS_TPMA_ADJ_0_1r_reg;
    TX_X1_GLAS_TPMA_ADJ_2_3r_t TX_X1_GLAS_TPMA_ADJ_2_3r_reg;
    TX_X1_GLAS_TPMA_ADJ_4_5r_t TX_X1_GLAS_TPMA_ADJ_4_5r_reg;
    TX_X1_GLAS_TPMA_ADJ_6_7r_t TX_X1_GLAS_TPMA_ADJ_6_7r_reg;

    PMD_X1_FCLK_PERIODr_t PMD_X1_FCLK_PERIODr_reg;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));

    for (i = 0; i < 16; i++) {
        tx_lane_skew_bits[i] = 0;
        normalized_tx_lane_skew_bits[i] = 0;
        glas_tpma_capture_data[i] = 0;
        glas_tpma_capture_adj[i] = 0;
    }

    max_lane_skew_bits = 0x0;
    min_lane_skew_bits = 0x2fff;
    max_lane_skew_bits_ro = 0x0;
    min_lane_skew_bits_ro = 0x2fff;
    no_of_bits = 0;

    bit_ui_val = _plp_aperta2_tscpmod_pcs_calc_bit_ui_value(vco, os_mode, is_pam4);

    if (((start_lane >= 0) && (start_lane < 4)) ||
         ((start_lane >=8) && (start_lane < 12))) {
        phy_copy.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    } else {
        phy_copy.access.lane_mask = 0x10 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    }
    PHYMOD_IF_ERR_RETURN(READ_PMD_X1_FCLK_PERIODr(&phy_copy, &PMD_X1_FCLK_PERIODr_reg));
    fclk_frac_ns = PMD_X1_FCLK_PERIODr_FCLK_FRAC_NSf_GET(PMD_X1_FCLK_PERIODr_reg);
    fclk_period_tmp =  fclk_frac_ns / 1024.0;
    fclk_period = fclk_period_tmp / 64.0;

    TX_X1_GLAS_TPMA_DATA0r_CLR(TX_X1_GLAS_TPMA_DATA0r_reg);
    TX_X1_GLAS_TPMA_DATA1r_CLR(TX_X1_GLAS_TPMA_DATA1r_reg);
    TX_X1_GLAS_TPMA_DATA2r_CLR(TX_X1_GLAS_TPMA_DATA2r_reg);
    TX_X1_GLAS_TPMA_DATA3r_CLR(TX_X1_GLAS_TPMA_DATA3r_reg);
    TX_X1_GLAS_TPMA_DATA4r_CLR(TX_X1_GLAS_TPMA_DATA4r_reg);
    TX_X1_GLAS_TPMA_DATA5r_CLR(TX_X1_GLAS_TPMA_DATA5r_reg);
    TX_X1_GLAS_TPMA_DATA6r_CLR(TX_X1_GLAS_TPMA_DATA6r_reg);
    TX_X1_GLAS_TPMA_DATA7r_CLR(TX_X1_GLAS_TPMA_DATA7r_reg);
    TX_X1_GLAS_TPMA_ADJ_0_1r_CLR(TX_X1_GLAS_TPMA_ADJ_0_1r_reg);
    TX_X1_GLAS_TPMA_ADJ_2_3r_CLR(TX_X1_GLAS_TPMA_ADJ_2_3r_reg);
    TX_X1_GLAS_TPMA_ADJ_4_5r_CLR(TX_X1_GLAS_TPMA_ADJ_4_5r_reg);
    TX_X1_GLAS_TPMA_ADJ_6_7r_CLR(TX_X1_GLAS_TPMA_ADJ_6_7r_reg);

    phy_copy.access.lane_mask = 0x1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA0r(&phy_copy, &TX_X1_GLAS_TPMA_DATA0r_reg));
    glas_tpma_capture_data[0] = TX_X1_GLAS_TPMA_DATA0r_GLAS_TPMA_CAPTURE_DATA_0f_GET(TX_X1_GLAS_TPMA_DATA0r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA1r(&phy_copy, &TX_X1_GLAS_TPMA_DATA1r_reg));
    glas_tpma_capture_data[1] = TX_X1_GLAS_TPMA_DATA1r_GLAS_TPMA_CAPTURE_DATA_1f_GET(TX_X1_GLAS_TPMA_DATA1r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA2r(&phy_copy, &TX_X1_GLAS_TPMA_DATA2r_reg));
    glas_tpma_capture_data[2] = TX_X1_GLAS_TPMA_DATA2r_GLAS_TPMA_CAPTURE_DATA_2f_GET(TX_X1_GLAS_TPMA_DATA2r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA3r(&phy_copy, &TX_X1_GLAS_TPMA_DATA3r_reg));
    glas_tpma_capture_data[3] = TX_X1_GLAS_TPMA_DATA3r_GLAS_TPMA_CAPTURE_DATA_3f_GET(TX_X1_GLAS_TPMA_DATA3r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA4r(&phy_copy, &TX_X1_GLAS_TPMA_DATA4r_reg));
    glas_tpma_capture_data[4] = TX_X1_GLAS_TPMA_DATA4r_GLAS_TPMA_CAPTURE_DATA_4f_GET(TX_X1_GLAS_TPMA_DATA4r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA5r(&phy_copy, &TX_X1_GLAS_TPMA_DATA5r_reg));
    glas_tpma_capture_data[5] = TX_X1_GLAS_TPMA_DATA5r_GLAS_TPMA_CAPTURE_DATA_5f_GET(TX_X1_GLAS_TPMA_DATA5r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA6r(&phy_copy, &TX_X1_GLAS_TPMA_DATA6r_reg));
    glas_tpma_capture_data[6] = TX_X1_GLAS_TPMA_DATA6r_GLAS_TPMA_CAPTURE_DATA_6f_GET(TX_X1_GLAS_TPMA_DATA6r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_DATA7r(&phy_copy, &TX_X1_GLAS_TPMA_DATA7r_reg));
    glas_tpma_capture_data[7] = TX_X1_GLAS_TPMA_DATA7r_GLAS_TPMA_CAPTURE_DATA_7f_GET(TX_X1_GLAS_TPMA_DATA7r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_ADJ_0_1r(&phy_copy, &TX_X1_GLAS_TPMA_ADJ_0_1r_reg));
    glas_tpma_capture_adj[0] = TX_X1_GLAS_TPMA_ADJ_0_1r_GLAS_TPMA_CAPTURE_ADJ_0f_GET(TX_X1_GLAS_TPMA_ADJ_0_1r_reg);
    glas_tpma_capture_adj[1] = TX_X1_GLAS_TPMA_ADJ_0_1r_GLAS_TPMA_CAPTURE_ADJ_1f_GET(TX_X1_GLAS_TPMA_ADJ_0_1r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_ADJ_2_3r(&phy_copy, &TX_X1_GLAS_TPMA_ADJ_2_3r_reg));
    glas_tpma_capture_adj[2] = TX_X1_GLAS_TPMA_ADJ_2_3r_GLAS_TPMA_CAPTURE_ADJ_2f_GET(TX_X1_GLAS_TPMA_ADJ_2_3r_reg);
    glas_tpma_capture_adj[3] = TX_X1_GLAS_TPMA_ADJ_2_3r_GLAS_TPMA_CAPTURE_ADJ_3f_GET(TX_X1_GLAS_TPMA_ADJ_2_3r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_ADJ_4_5r(&phy_copy, &TX_X1_GLAS_TPMA_ADJ_4_5r_reg));
    glas_tpma_capture_adj[4] = TX_X1_GLAS_TPMA_ADJ_4_5r_GLAS_TPMA_CAPTURE_ADJ_4f_GET(TX_X1_GLAS_TPMA_ADJ_4_5r_reg);
    glas_tpma_capture_adj[5] = TX_X1_GLAS_TPMA_ADJ_4_5r_GLAS_TPMA_CAPTURE_ADJ_5f_GET(TX_X1_GLAS_TPMA_ADJ_4_5r_reg);

    PHYMOD_IF_ERR_RETURN(READ_TX_X1_GLAS_TPMA_ADJ_6_7r(&phy_copy, &TX_X1_GLAS_TPMA_ADJ_6_7r_reg));
    glas_tpma_capture_adj[6] = TX_X1_GLAS_TPMA_ADJ_6_7r_GLAS_TPMA_CAPTURE_ADJ_6f_GET(TX_X1_GLAS_TPMA_ADJ_6_7r_reg);
    glas_tpma_capture_adj[7] = TX_X1_GLAS_TPMA_ADJ_6_7r_GLAS_TPMA_CAPTURE_ADJ_7f_GET(TX_X1_GLAS_TPMA_ADJ_6_7r_reg);

    no_of_bits = fclk_period / bit_ui_val;

    for (i = 0; i < num_lane; i++) {
        tx_lane_skew_bits[start_lane + i] = glas_tpma_capture_data[start_lane + i] + (glas_tpma_capture_adj[start_lane + i] * no_of_bits);
        if (tx_lane_skew_bits[start_lane + i] > max_lane_skew_bits) {
            max_lane_skew_bits = tx_lane_skew_bits[start_lane + i];
        }
        if (tx_lane_skew_bits[start_lane + i] < min_lane_skew_bits) {
            min_lane_skew_bits = tx_lane_skew_bits[start_lane + i];
        }
    }

    if ((max_lane_skew_bits - min_lane_skew_bits) > 0x1000) {
        for (i = 0; i < num_lane; i++) {
            if (max_lane_skew_bits - tx_lane_skew_bits[start_lane + i] > 0x1000) {
                tx_lane_skew_bits_ro[start_lane + i] = 0x2000 | tx_lane_skew_bits[start_lane + i];
            } else {
                tx_lane_skew_bits_ro[start_lane + i] = tx_lane_skew_bits[start_lane + i];
            }
            if (tx_lane_skew_bits_ro[start_lane + i] > max_lane_skew_bits_ro) {
                max_lane_skew_bits_ro = tx_lane_skew_bits_ro[start_lane + i];
            }
            if (tx_lane_skew_bits_ro[start_lane + i] < min_lane_skew_bits_ro) {
                min_lane_skew_bits_ro = tx_lane_skew_bits_ro[start_lane + i];
            }
        }
        for (i = 0; i < num_lane; i++) {
            normalized_tx_lane_skew_bits[start_lane + i] = tx_lane_skew_bits_ro[fixed_lane] - tx_lane_skew_bits_ro[start_lane + i];
        }
        *tx_max_skew = ((uint32_t) (max_lane_skew_bits_ro - min_lane_skew_bits_ro) * bit_ui_val * 1024.0);
    } else {
        for (i = 0; i < num_lane; i++) {
            normalized_tx_lane_skew_bits[start_lane + i] = tx_lane_skew_bits[fixed_lane] - tx_lane_skew_bits[start_lane + i];
        }
        *tx_max_skew = ((uint32_t)(max_lane_skew_bits - min_lane_skew_bits) * bit_ui_val * 1024.0);
    }

    for (i = 0; i < num_lane; i++) {
        curr_lane = start_lane + i;
        latency_adj = normalized_tx_lane_skew_bits[curr_lane] * bit_ui_val * 1024;
        phy_copy.access.lane_mask = 0x1 << curr_lane;
        PHYMOD_IF_ERR_RETURN
            (_plp_aperta2_tscpmod_update_tx_pmd_latency(&phy_copy, latency_adj, 0));
    }
#endif
    return PHYMOD_E_NONE;
}

int _plp_aperta2_tscpmod_virtual_lane_count_get(int bit_mux_mode, int num_lane, int *virtual_lanes, int *num_psll_per_phyl)
{
    switch (bit_mux_mode) {
        case 0:
            *virtual_lanes = num_lane * 1;
            *num_psll_per_phyl = 1;
            break;
        case 1:
            *virtual_lanes = num_lane * 2;
            *num_psll_per_phyl = 2;
            break;
        case 2:
            *virtual_lanes = num_lane * 4;
            *num_psll_per_phyl = 4;
            break;
        case 3:
            *virtual_lanes = num_lane * 5;
            *num_psll_per_phyl = 5;
            break;
        default:
            *virtual_lanes = num_lane * 1;
            *num_psll_per_phyl = 1;
            break;
    }
    return PHYMOD_E_NONE;
}

int _plp_aperta2_tscpmod_timesync_rx_deskew_info_get (PHYMOD_ST *pc, int bit_mux_mode, uint32_t vco,
                                          int os_mode, int is_pam4,
                                          uint32_t *rx_max_skew_vl, uint32_t *rx_min_skew_vl,
                                          uint32_t *skew_per_vl, uint32_t *vl_to_pl_map,
                                          uint32_t *vl_to_pl_off_map)
{
#ifdef PHYMOD_CONFIG_INCLUDE_FLOATING_POINT
    int i, num_psll_per_phyl, virtual_lanes;

    /* per psuedo logical lane
     * the equation: {NS,sub_ns} + fclk_period * fclk_adjust + bit_offset * bit_ui_val
     * there are 4 logical lanes per mpp and 20 virtual lanes.
     */

    uint32_t psll_update[TSCP_TS_MAX_NUM_VIRTUAL_LANE_IN_PM];
    float bit_ui_val;
    uint16_t base_ts_ns;
    uint16_t base_ts_subns;
    uint16_t am_slip_cnt;
    uint16_t am_ts_fclk_adj_val;
    uint16_t deskew_ts_info_val;
    uint32_t fclk_period;
    uint32_t psll_val_min, psll_roval_min;
    uint32_t psll_val_max, psll_roval_max;
    uint32_t min_vl, max_vl;

    int8_t   vl_to_psll_map[TSCP_TS_MAX_NUM_VIRTUAL_LANE_IN_PM];
    int8_t   vl_to_psll_off_map[TSCP_TS_MAX_NUM_VIRTUAL_LANE_IN_PM];
    uint32_t curr_vl;
    int      start_lane, num_lane;
    PHYMOD_ST phy_copy;

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

    PHYMOD_IF_ERR_RETURN
      (plp_aperta2_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (_plp_aperta2_tscpmod_virtual_lane_count_get(bit_mux_mode, num_lane, &virtual_lanes, &num_psll_per_phyl));

    bit_ui_val = _plp_aperta2_tscpmod_pcs_calc_bit_ui_value(vco, os_mode, is_pam4);

    psll_val_min = 0x80000;
    psll_roval_min = 0x80000;
    psll_val_max = 0;
    psll_roval_max = 0;
    min_vl = 0; max_vl = 0;
    for (i = 0; i < TSCP_TS_MAX_NUM_VIRTUAL_LANE_IN_PM; i++) {
        psll_update[i] = 0;
        vl_to_psll_map[i] = 0;
        vl_to_psll_off_map[i] = 0;
    }

    if (start_lane < 4) {
        phy_copy.access.lane_mask = 0x1;
    } else {
        phy_copy.access.lane_mask = 0x10;
    }
    PHYMOD_IF_ERR_RETURN(READ_PMD_X1_FCLK_PERIODr(&phy_copy, &PMD_X1_FCLK_PERIODr_reg));
    fclk_period = PMD_X1_FCLK_PERIODr_FCLK_FRAC_NSf_GET(PMD_X1_FCLK_PERIODr_reg);

    if (virtual_lanes > 1) {
        for (i = 0; i < num_lane; i++) {
            phy_copy.access.lane_mask = 0x1 << (start_lane + i);
            PHYMOD_IF_ERR_RETURN(READ_RX_X4_PSLL_TO_VL_MAP0r(&phy_copy, &RX_X4_PSLL_TO_VL_MAP0r_reg));
            PHYMOD_IF_ERR_RETURN(READ_RX_X4_PSLL_TO_VL_MAP1r(&phy_copy, &RX_X4_PSLL_TO_VL_MAP1r_reg));

            if (num_psll_per_phyl > 0) {
                curr_vl  = RX_X4_PSLL_TO_VL_MAP0r_PSLL0_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP0r_reg);
                vl_to_psll_map[curr_vl] = start_lane + i;
                vl_to_psll_off_map[curr_vl] = 0;
            }
            if (num_psll_per_phyl > 1) {
                curr_vl = RX_X4_PSLL_TO_VL_MAP0r_PSLL1_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP0r_reg);
                vl_to_psll_map[curr_vl] = start_lane + i;
                vl_to_psll_off_map[curr_vl] = 1;
            }
            if (num_psll_per_phyl > 2) {
                curr_vl = RX_X4_PSLL_TO_VL_MAP0r_PSLL2_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP0r_reg);
                vl_to_psll_map[curr_vl] = start_lane + i;
                vl_to_psll_off_map[curr_vl] = 2;
            }
            if (num_psll_per_phyl > 3) {
                curr_vl = RX_X4_PSLL_TO_VL_MAP1r_PSLL3_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP1r_reg);
                vl_to_psll_map[curr_vl] = start_lane + i;
                vl_to_psll_off_map[curr_vl] = 3;
            }
            if (num_psll_per_phyl > 4) {
                curr_vl = RX_X4_PSLL_TO_VL_MAP1r_PSLL4_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP1r_reg);
                vl_to_psll_map[curr_vl] = start_lane + i;
                vl_to_psll_off_map[curr_vl] = 4;
            }
        }

        for (i = 0; i < virtual_lanes; i++) {
            phy_copy.access.lane_mask = 0x1 << (vl_to_psll_map[i]);
            if (vl_to_psll_off_map[i] == 0) {
                PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS0r(&phy_copy, &RX_X4_SKEW_OFFSS0r_reg));
                PHYMOD_IF_ERR_RETURN(READ_RX_X4_AM_TS_INFO0r(&phy_copy, &RX_X4_AM_TS_INFO0r_reg));
                base_ts_ns = RX_X4_AM_TS_INFO0r_BASE_TS_NS_0f_GET(RX_X4_AM_TS_INFO0r_reg);
                base_ts_subns = RX_X4_AM_TS_INFO0r_BASE_TS_SUB_NS_0f_GET(RX_X4_AM_TS_INFO0r_reg);
                am_slip_cnt = RX_X4_SKEW_OFFSS0r_AM_SLIP_COUNT_0f_GET(RX_X4_SKEW_OFFSS0r_reg) << 2 ;
                am_ts_fclk_adj_val = RX_X4_SKEW_OFFSS0r_AM_TS_FCLK_ADJUST_VALUE_0f_GET(RX_X4_SKEW_OFFSS0r_reg);
                deskew_ts_info_val = RX_X4_SKEW_OFFSS0r_DESKEW_TS_INFO_VALID_0f_GET(RX_X4_SKEW_OFFSS0r_reg);
                if (deskew_ts_info_val == 1) {
                    psll_update[i] = ((base_ts_ns << 4) | (base_ts_subns & 0xf)) + ((uint32_t)((float)(fclk_period * am_ts_fclk_adj_val * 16 / 65536))) + ((uint32_t)((float)(am_slip_cnt * bit_ui_val * 16)));
                    if (psll_update[i] < psll_val_min) {
                        psll_val_min = psll_update[i];
                        min_vl = i;
                    }
                    if (psll_update[i] > psll_val_max) {
                        psll_val_max = psll_update[i];
                        max_vl = i;
                    }
                }
            } else if (vl_to_psll_off_map[i] == 1) {
                PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS1r(&phy_copy, &RX_X4_SKEW_OFFSS1r_reg));
                PHYMOD_IF_ERR_RETURN(READ_RX_X4_AM_TS_INFO1r(&phy_copy, &RX_X4_AM_TS_INFO1r_reg));
                base_ts_ns = RX_X4_AM_TS_INFO1r_BASE_TS_NS_1f_GET(RX_X4_AM_TS_INFO1r_reg);
                base_ts_subns = RX_X4_AM_TS_INFO1r_BASE_TS_SUB_NS_1f_GET(RX_X4_AM_TS_INFO1r_reg);
                am_slip_cnt = RX_X4_SKEW_OFFSS1r_AM_SLIP_COUNT_1f_GET(RX_X4_SKEW_OFFSS1r_reg) << 2;
                am_ts_fclk_adj_val = RX_X4_SKEW_OFFSS1r_AM_TS_FCLK_ADJUST_VALUE_1f_GET(RX_X4_SKEW_OFFSS1r_reg);
                deskew_ts_info_val = RX_X4_SKEW_OFFSS1r_DESKEW_TS_INFO_VALID_1f_GET(RX_X4_SKEW_OFFSS1r_reg);

                if (deskew_ts_info_val == 1) {
                    psll_update[i] = ((base_ts_ns << 4) | (base_ts_subns & 0xf)) + ((uint32_t)((float)(fclk_period * am_ts_fclk_adj_val * 16 / 65536))) + ((uint32_t)((float)(am_slip_cnt * bit_ui_val * 16)));
                    if (psll_update[i] < psll_val_min) {
                        psll_val_min = psll_update[i];
                        min_vl = i;
                    }
                    if (psll_update[i] > psll_val_max) {
                        psll_val_max = psll_update[i];
                        max_vl = i;
                    }
                }
            } else if (vl_to_psll_off_map[i] == 2) {
                PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS2r(&phy_copy, &RX_X4_SKEW_OFFSS2r_reg));
                PHYMOD_IF_ERR_RETURN(READ_RX_X4_AM_TS_INFO2r(&phy_copy, &RX_X4_AM_TS_INFO2r_reg));
                base_ts_ns = RX_X4_AM_TS_INFO2r_BASE_TS_NS_2f_GET(RX_X4_AM_TS_INFO2r_reg);
                base_ts_subns = RX_X4_AM_TS_INFO2r_BASE_TS_SUB_NS_2f_GET(RX_X4_AM_TS_INFO2r_reg);
                am_slip_cnt = RX_X4_SKEW_OFFSS2r_AM_SLIP_COUNT_2f_GET(RX_X4_SKEW_OFFSS2r_reg) << 2;
                am_ts_fclk_adj_val = RX_X4_SKEW_OFFSS2r_AM_TS_FCLK_ADJUST_VALUE_2f_GET(RX_X4_SKEW_OFFSS2r_reg);
                deskew_ts_info_val = RX_X4_SKEW_OFFSS2r_DESKEW_TS_INFO_VALID_2f_GET(RX_X4_SKEW_OFFSS2r_reg);

                if (deskew_ts_info_val == 1) {
                    psll_update[i] = ((base_ts_ns << 4) | (base_ts_subns & 0xf)) + ((uint32_t)((float)(fclk_period * am_ts_fclk_adj_val * 16 / 65536))) + ((uint32_t)((float)(am_slip_cnt * bit_ui_val * 16)));
                    if (psll_update[i] < psll_val_min) {
                        psll_val_min = psll_update[i];
                        min_vl = i;
                    }
                    if (psll_update[i] > psll_val_max) {
                        psll_val_max = psll_update[i];
                        max_vl = i;
                    }
                }
            } else if (vl_to_psll_off_map[i] == 3) {
                PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS3r(&phy_copy, &RX_X4_SKEW_OFFSS3r_reg));
                PHYMOD_IF_ERR_RETURN(READ_RX_X4_AM_TS_INFO3r(&phy_copy, &RX_X4_AM_TS_INFO3r_reg));
                base_ts_ns = RX_X4_AM_TS_INFO3r_BASE_TS_NS_3f_GET(RX_X4_AM_TS_INFO3r_reg);
                base_ts_subns = RX_X4_AM_TS_INFO3r_BASE_TS_SUB_NS_3f_GET(RX_X4_AM_TS_INFO3r_reg);
                am_slip_cnt = RX_X4_SKEW_OFFSS3r_AM_SLIP_COUNT_3f_GET(RX_X4_SKEW_OFFSS3r_reg) << 2;
                am_ts_fclk_adj_val = RX_X4_SKEW_OFFSS3r_AM_TS_FCLK_ADJUST_VALUE_3f_GET(RX_X4_SKEW_OFFSS3r_reg);
                deskew_ts_info_val = RX_X4_SKEW_OFFSS3r_DESKEW_TS_INFO_VALID_3f_GET(RX_X4_SKEW_OFFSS3r_reg);

                if (deskew_ts_info_val == 1) {
                    psll_update[i] = ((base_ts_ns << 4) | (base_ts_subns & 0xf)) + ((uint32_t)((float)(fclk_period * am_ts_fclk_adj_val * 16 / 65536))) + ((uint32_t)((float)(am_slip_cnt * bit_ui_val * 16)));
                    if (psll_update[i] < psll_val_min) {
                        psll_val_min = psll_update[i];
                        min_vl = i;
                    }
                    if (psll_update[i] > psll_val_max) {
                        psll_val_max = psll_update[i];
                        max_vl = i;
                    }
                }
            } else if (vl_to_psll_off_map[i] == 4) {
                PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS4r(&phy_copy, &RX_X4_SKEW_OFFSS4r_reg));
                PHYMOD_IF_ERR_RETURN(READ_RX_X4_AM_TS_INFO4r(&phy_copy, &RX_X4_AM_TS_INFO4r_reg));
                base_ts_ns = RX_X4_AM_TS_INFO4r_BASE_TS_NS_4f_GET(RX_X4_AM_TS_INFO4r_reg);
                base_ts_subns = RX_X4_AM_TS_INFO4r_BASE_TS_SUB_NS_4f_GET(RX_X4_AM_TS_INFO4r_reg);
                am_slip_cnt = RX_X4_SKEW_OFFSS4r_AM_SLIP_COUNT_4f_GET(RX_X4_SKEW_OFFSS4r_reg) << 2;
                am_ts_fclk_adj_val = RX_X4_SKEW_OFFSS4r_AM_TS_FCLK_ADJUST_VALUE_4f_GET(RX_X4_SKEW_OFFSS4r_reg);
                deskew_ts_info_val = RX_X4_SKEW_OFFSS4r_DESKEW_TS_INFO_VALID_4f_GET(RX_X4_SKEW_OFFSS4r_reg);

                if (deskew_ts_info_val == 1) {
                    psll_update[i] = ((base_ts_ns << 4) | (base_ts_subns & 0xf)) + ((uint32_t)((float)(fclk_period * am_ts_fclk_adj_val * 16 / 65536))) + ((uint32_t)((float)(am_slip_cnt * bit_ui_val * 16)));
                    if (psll_update[i] < psll_val_min) {
                        psll_val_min = psll_update[i];
                        min_vl = i;
                    }
                    if (psll_update[i] > psll_val_max) {
                        psll_val_max = psll_update[i];
                        max_vl = i;
                    }
                }
            }
            skew_per_vl[i]      = psll_update[i];
            vl_to_pl_map[i]     = vl_to_psll_map[i];
            vl_to_pl_off_map[i] = vl_to_psll_off_map[i];
        }
        *rx_max_skew_vl = max_vl;
        *rx_min_skew_vl = min_vl;
        if ((psll_val_max - psll_val_min) > 0x1000) {
            for (i = 0; i < virtual_lanes; i++) {
                if ((psll_val_max - psll_update[i]) > 0x01000) {
                    psll_update[i] |= 0x10000;
                }
                if (psll_update[i] < psll_roval_min) {
                    psll_roval_min = psll_update[i];
                    min_vl = i;
                }
                if (psll_update[i] > psll_roval_max) {
                    psll_roval_max = psll_update[i];
                    max_vl = i;
                }
                skew_per_vl[i] = psll_update[i];
            }
            *rx_max_skew_vl = max_vl;
            *rx_min_skew_vl = min_vl;
        }
    }
#endif

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_timesync_rx_deskew_info_dump (PHYMOD_ST *pc,
                                          int bit_mux_mode, uint32_t vco,
                                          int os_mode, int is_pam4,
                                          int normalize_to_latest)
{
#ifdef PHYMOD_CONFIG_INCLUDE_FLOATING_POINT
    int         i, num_psll_per_phyl, virtual_lanes;
    uint32_t    vl_to_pl_map[TSCP_TS_MAX_NUM_VIRTUAL_LANE_IN_PM] = {0};
    uint32_t    vl_to_pl_off_map[TSCP_TS_MAX_NUM_VIRTUAL_LANE_IN_PM] = {0};
    uint32_t    skew_per_vl[TSCP_TS_MAX_NUM_VIRTUAL_LANE_IN_PM] = {0};
    uint32_t    max_phy_lane_skew[8]={0};
    int         start_lane, num_lane;
    uint32_t    max_vl, min_vl;
    PHYMOD_ST   phy_copy;

    PHYMOD_IF_ERR_RETURN
      (plp_aperta2_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (_plp_aperta2_tscpmod_virtual_lane_count_get(bit_mux_mode, num_lane, &virtual_lanes, &num_psll_per_phyl));

    if (virtual_lanes > 1) {

        PHYMOD_IF_ERR_RETURN
            (_plp_aperta2_tscpmod_timesync_rx_deskew_info_get(pc, bit_mux_mode, vco,
                                                  os_mode, is_pam4,
                                                  &max_vl, &min_vl, skew_per_vl,
                                                  vl_to_pl_map, vl_to_pl_off_map));

        /*
         * find the skew for the physical lanes. Reference to latest lane.
         */
       for (i = 0; i < virtual_lanes; i++) {
            /* DEBUG
            PHYMOD_DIAG_OUT((" ####  vl %2d pl %d  off %d  skew 0x%x  to max 0x%02x from min 0x%x \n", i,
                            vl_to_pl_map[i], vl_to_pl_off_map[i],
                            skew_per_vl[i], skew_per_vl[max_vl]-skew_per_vl[i],
                            skew_per_vl[i]-skew_per_vl[min_vl]));
            if(!((i+1)%4)) PHYMOD_DIAG_OUT(("\n"));
            */
            if ((max_phy_lane_skew[vl_to_pl_map[i]] < ( skew_per_vl[max_vl]-skew_per_vl[i])) &&
                (vl_to_pl_map[max_vl] != vl_to_pl_map[i])) {
                max_phy_lane_skew[vl_to_pl_map[i]] = ( skew_per_vl[max_vl]-skew_per_vl[i]);
            }
        }

        for (i = 0; i < num_lane; i++) {
            PHYMOD_DIAG_OUT((" Physical Lane %d Relative Skew 0x%x ns", i, max_phy_lane_skew[i]>>4));
            if (i == (int)vl_to_pl_map[max_vl]) {
                PHYMOD_DIAG_OUT(("  <<< latest reference lane \n"));
            } else {
                PHYMOD_DIAG_OUT(("\n"));
            }
        }
    } else {
        PHYMOD_DIAG_OUT((" Single Lane Port,  No Skew information \n"));
    }
#endif

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_mod_rx_1588_tbl_val(PHYMOD_ST *pc, int bit_mux_mode, uint32_t vco,
                                    int os_mode, int is_pam4, int normalize_to_latest,
                                    uint32_t *rx_max_skew, uint32_t *rx_min_skew, uint32_t *skew_per_vl,
                                    uint32_t *rx_dsl_sel, uint32_t *rx_psll_sel)
{
#ifdef SERDES_API_FLOATING_POINT
    int         num_psll_per_phyl, virtual_lanes;
    uint32_t    vl_to_pl_map[TSCP_TS_MAX_NUM_VIRTUAL_LANE_IN_PM];
    uint32_t    vl_to_pl_off_map[TSCP_TS_MAX_NUM_VIRTUAL_LANE_IN_PM];
    int         start_lane, num_lane;
    uint32_t    max_vl, min_vl;
    PHYMOD_ST phy_copy;

    RX_X4_TIMESTAMPINGr_t RX_X4_TIMESTAMPINGr_reg;


    PHYMOD_IF_ERR_RETURN
      (plp_aperta2_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (_plp_aperta2_tscpmod_virtual_lane_count_get(bit_mux_mode, num_lane, &virtual_lanes, &num_psll_per_phyl));


    if (virtual_lanes > 1) {

        PHYMOD_IF_ERR_RETURN
            (_plp_aperta2_tscpmod_timesync_rx_deskew_info_get(pc, bit_mux_mode, vco,
                                             os_mode, is_pam4,
                                             &max_vl, &min_vl, skew_per_vl,
                                             vl_to_pl_map, vl_to_pl_off_map));

        phy_copy.access.lane_mask = 0x1 << start_lane;
        RX_X4_TIMESTAMPINGr_CLR(RX_X4_TIMESTAMPINGr_reg);
        if (normalize_to_latest == 1) {
           RX_X4_TIMESTAMPINGr_DSL_SELf_SET(RX_X4_TIMESTAMPINGr_reg, max_vl);
           *rx_dsl_sel  = max_vl;
           *rx_psll_sel = vl_to_pl_map[max_vl];
        } else {
           RX_X4_TIMESTAMPINGr_DSL_SELf_SET(RX_X4_TIMESTAMPINGr_reg, min_vl);
           *rx_dsl_sel  = min_vl;
           *rx_psll_sel = vl_to_pl_map[min_vl];
        }
        PHYMOD_IF_ERR_RETURN(WRITE_RX_X4_TIMESTAMPINGr(&phy_copy, RX_X4_TIMESTAMPINGr_reg));
    } else {
        phy_copy.access.lane_mask = 0x1 << start_lane;
        RX_X4_TIMESTAMPINGr_CLR(RX_X4_TIMESTAMPINGr_reg);
        RX_X4_TIMESTAMPINGr_DSL_SELf_SET(RX_X4_TIMESTAMPINGr_reg, 0);
        PHYMOD_IF_ERR_RETURN(WRITE_RX_X4_TIMESTAMPINGr(&phy_copy, RX_X4_TIMESTAMPINGr_reg));
    }
#endif

    return PHYMOD_E_NONE;
}

/* FIXME needs to revisit */
int plp_aperta2_tscpmod_chk_rx_ts_deskew_valid(PHYMOD_ST *pc, int bit_mux_mode, int *rx_ts_deskew_valid)
{
    int i, start_lane, num_lane, curr_vl;
    int virtual_lanes, num_psll_per_phyl;
    uint8_t vl_to_psll_map[TSCP_TS_MAX_NUM_VIRTUAL_LANE_IN_PM], vl_to_psll_off_map[TSCP_TS_MAX_NUM_VIRTUAL_LANE_IN_PM];
    uint16_t ts_deskew_valid;
    PHYMOD_ST phy_copy;
    RX_X4_PSLL_TO_VL_MAP0r_t RX_X4_PSLL_TO_VL_MAP0r_reg;
    RX_X4_PSLL_TO_VL_MAP1r_t RX_X4_PSLL_TO_VL_MAP1r_reg;

    RX_X4_SKEW_OFFSS0r_t RX_X4_SKEW_OFFSS0r_reg;
    RX_X4_SKEW_OFFSS1r_t RX_X4_SKEW_OFFSS1r_reg;
    RX_X4_SKEW_OFFSS2r_t RX_X4_SKEW_OFFSS2r_reg;
    RX_X4_SKEW_OFFSS3r_t RX_X4_SKEW_OFFSS3r_reg;
    RX_X4_SKEW_OFFSS4r_t RX_X4_SKEW_OFFSS4r_reg;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));
    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));

    PHYMOD_IF_ERR_RETURN
        (_plp_aperta2_tscpmod_virtual_lane_count_get(bit_mux_mode, num_lane, &virtual_lanes, &num_psll_per_phyl));

    if (virtual_lanes == 1) {
        *rx_ts_deskew_valid = 1;
        return PHYMOD_E_NONE;
    }

    for (i = 0; i < TSCP_TS_MAX_NUM_VIRTUAL_LANE_IN_PM; i++) {
        vl_to_psll_map[i] = 0;
        vl_to_psll_off_map[i] = 0;
    }

    /* Get VL to PSLL mapping. */
    for (i = 0; i < num_lane; i++) {
        phy_copy.access.lane_mask = 0x1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN(READ_RX_X4_PSLL_TO_VL_MAP0r(&phy_copy, &RX_X4_PSLL_TO_VL_MAP0r_reg));
        PHYMOD_IF_ERR_RETURN(READ_RX_X4_PSLL_TO_VL_MAP1r(&phy_copy, &RX_X4_PSLL_TO_VL_MAP1r_reg));

        if (num_psll_per_phyl > 0) {
            curr_vl = RX_X4_PSLL_TO_VL_MAP0r_PSLL0_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP0r_reg);
            vl_to_psll_map[curr_vl] = start_lane + i;
            vl_to_psll_off_map[curr_vl] = 0;
        }
        if (num_psll_per_phyl > 1) {
            curr_vl = RX_X4_PSLL_TO_VL_MAP0r_PSLL1_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP0r_reg);
            vl_to_psll_map[curr_vl] = start_lane + i;
            vl_to_psll_off_map[curr_vl] = 1;
        }
        if (num_psll_per_phyl > 2) {
            curr_vl = RX_X4_PSLL_TO_VL_MAP0r_PSLL2_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP0r_reg);
            vl_to_psll_map[curr_vl] = start_lane + i;
            vl_to_psll_off_map[curr_vl] = 2;
        }
        if (num_psll_per_phyl > 3) {
            curr_vl = RX_X4_PSLL_TO_VL_MAP1r_PSLL3_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP1r_reg);
            vl_to_psll_map[curr_vl] = start_lane + i;
            vl_to_psll_off_map[curr_vl] = 3;
        }
        if (num_psll_per_phyl > 4) {
            curr_vl = RX_X4_PSLL_TO_VL_MAP1r_PSLL4_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP1r_reg);
            vl_to_psll_map[curr_vl] = start_lane + i;
            vl_to_psll_off_map[curr_vl] = 4;
        }
    }

    ts_deskew_valid = 1;

    for (i = 0; i < virtual_lanes; i++) {
        phy_copy.access.lane_mask = 0x1 << vl_to_psll_map[i];
        if (vl_to_psll_off_map[i] == 0) {
            PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS0r(&phy_copy, &RX_X4_SKEW_OFFSS0r_reg));
            ts_deskew_valid = ts_deskew_valid & RX_X4_SKEW_OFFSS0r_DESKEW_TS_INFO_VALID_0f_GET(RX_X4_SKEW_OFFSS0r_reg);
        } else if (vl_to_psll_off_map[i] == 1) {
            PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS1r(&phy_copy, &RX_X4_SKEW_OFFSS1r_reg));
            ts_deskew_valid = ts_deskew_valid & RX_X4_SKEW_OFFSS1r_DESKEW_TS_INFO_VALID_1f_GET(RX_X4_SKEW_OFFSS1r_reg);
        } else if (vl_to_psll_off_map[i] == 2) {
            PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS2r(&phy_copy, &RX_X4_SKEW_OFFSS2r_reg));
            ts_deskew_valid = ts_deskew_valid & RX_X4_SKEW_OFFSS2r_DESKEW_TS_INFO_VALID_2f_GET(RX_X4_SKEW_OFFSS2r_reg);
        } else if (vl_to_psll_off_map[i] == 3) {
            PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS3r(&phy_copy, &RX_X4_SKEW_OFFSS3r_reg));
            ts_deskew_valid = ts_deskew_valid & RX_X4_SKEW_OFFSS3r_DESKEW_TS_INFO_VALID_3f_GET(RX_X4_SKEW_OFFSS3r_reg);
        } else if (vl_to_psll_off_map[i] == 4) {
            PHYMOD_IF_ERR_RETURN(READ_RX_X4_SKEW_OFFSS4r(&phy_copy, &RX_X4_SKEW_OFFSS4r_reg));
            ts_deskew_valid = ts_deskew_valid & RX_X4_SKEW_OFFSS4r_DESKEW_TS_INFO_VALID_4f_GET(RX_X4_SKEW_OFFSS4r_reg);
        }
    }

    *rx_ts_deskew_valid = ts_deskew_valid;

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_rsfec_symbol_error_counter_get(PHYMOD_ST* pc,
                                          int bit_mux_mode,
                                          int max_count,
                                          int* actual_count,
                                          uint32_t* symb_err_cnt)
{
    PHYMOD_ST phy_copy;
    int num_lanes, start_lane, virtual_lanes, num_psll_per_phyl;
    int i, cur_vl, mem_idx = 0, start_idx = 0, mem_entry = 0;
    uint32_t sum, vl_symb_err_cnt[32];
    uint32_t rsfec_symb_err_cnt[4];
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    RX_X4_PSLL_TO_VL_MAP0r_t RX_X4_PSLL_TO_VL_MAP0r_reg;
    RX_X4_PSLL_TO_VL_MAP1r_t RX_X4_PSLL_TO_VL_MAP1r_reg;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lanes));

    if (max_count < num_lanes) {
        return PHYMOD_E_PARAM;
    } else {
        *actual_count = num_lanes;
    }

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));
    PHYMOD_MEMSET(vl_symb_err_cnt, 0x0, 32 * sizeof(uint32_t));
    PHYMOD_MEMSET(rsfec_symb_err_cnt, 0x0, 4 * sizeof(uint32_t));
    PHYMOD_IF_ERR_RETURN
        (_plp_aperta2_tscpmod_virtual_lane_count_get(bit_mux_mode, num_lanes, &virtual_lanes, &num_psll_per_phyl));

    phy_copy.access.lane_mask = 1 << PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal);
    cur_vl = 0;
    mem_idx = 0;

    switch (virtual_lanes) {
    /*
     * The mapping of the symbol error counters to entries is as followed:
     * MPP0:Entry_0-Entry_7;
     * MPP1:Entry_8-Entry_15;
     * Each entry has 4 RSFEC symbol error counters
     * There are two FECs within each MPP. One FEC supports 16 symbol lane error counters.
     *
     * virtual_lanes= 1, if port start lane is 0, read symbol lane counter 0;
     *                   if port start lane is 1, read symbol lane counter 8;
     *                   if port start lane is 2, read symbol lane counter 16;
     *                   if port start lane is 3, read symbol lane counter 24;
     *                   if port start lane is 4, read symbol lane counter 32;
     *                   if port start lane is 5, read symbol lane counter 40;
     *                   if port start lane is 6, read symbol lane counter 48;
     *                   if port start lane is 7, read symbol lane counter 56;
     * virtual_lanes= 2, if port start lane is 0, read symbol lane counter 0 and 1;
     *                   if port start lane is 1, read symbol lane counter 8 and 9;
     *                   if port start lane is 2, read symbol lane counter 16 and 17;
     *                   if port start lane is 3, read symbol lane counter 24 and 25;
     *                   if port start lane is 4, read symbol lane counter 32 and 33;
     *                   if port start lane is 5, read symbol lane counter 40 and 41;
     *                   if port start lane is 6, read symbol lane counter 48 and 49;
     *                   if port start lane is 7, read symbol lane counter 56 and 57;
     * virtual_lanes= 4, if port start lane is 0, read symbol lane counter 0 to 3;
     *                   if port start lane is 1, read symbol lane counter 8 to 11;
     *                   if port start lane is 2, read symbol lane counter 16 to 19;
     *                   if port start lane is 3, read symbol lane counter 24 to 27;
     *                   if port start lane is 4, read symbol lane counter 32 to 35;
     *                   if port start lane is 5, read symbol lane counter 40 to 43;
     *                   if port start lane is 6, read symbol lane counter 48 to 51;
     *                   if port start lane is 7, read symbol lane counter 56 to 59;
     * virtual_lanes= 8, if port start lane is 0, read symbol lane counter 0 to 7;
     *                   if port start lane is 2, read symbol lane counter 16 to 23;
     *                   if port start lane is 4, read symbol lane counter 32 to 39;
     *                   if port start lane is 6, read symbol lane counter 48 to 55;
     * virtual_lanes= 16, if port in MPP0, symbol lane counter 0-15 and 16-31;
     *                    if port in MPP1, read symbol lane counter 32-47 and 48-63;
     * virtual_lanes = 32, read FEC lanes 0-15:(symbol lane counter 0-15 and 16-31)
     *                      and FEC lanes 16-31:(symbol lane counter 32-47 and 48-63);
     */
        case 1:
            if (start_lane == 0) {
                mem_idx = 0;
            } else if (start_lane == 1) {
                mem_idx = 2;
            } else if (start_lane == 2) {
                mem_idx = 4;
            } else if (start_lane == 3) {
                mem_idx = 6;
            } else if (start_lane == 4) {
                mem_idx = 8;
            } else if (start_lane == 5) {
                mem_idx = 10;
            } else if (start_lane == 6) {
                mem_idx = 12;
            } else if (start_lane == 7) {
                mem_idx = 14;
            }
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_mem_read(&phy_copy, phymodMemRsFecSymbErrMib, mem_idx, rsfec_symb_err_cnt));
            vl_symb_err_cnt[0] = rsfec_symb_err_cnt[0];
            break;
        case 2:
            if (start_lane == 0) {
                mem_idx = 0;
            } else if (start_lane == 1) {
                mem_idx = 2;
            } else if (start_lane == 2) {
                mem_idx = 4;
            } else if (start_lane == 3) {
                mem_idx = 6;
            } else if (start_lane == 4) {
                mem_idx = 8;
            } else if (start_lane == 5) {
                mem_idx = 10;
            } else if (start_lane == 6) {
                mem_idx = 12;
            } else if (start_lane == 7) {
                mem_idx = 14;
            }
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_mem_read(&phy_copy, phymodMemRsFecSymbErrMib, mem_idx, rsfec_symb_err_cnt));
            for (i = 0; i < 2; i++) {
                if (rsfec_symb_err_cnt[i] > 0xffffffff - vl_symb_err_cnt[cur_vl]) {
                    vl_symb_err_cnt[cur_vl] = 0xffffffff;
                } else {
                    vl_symb_err_cnt[cur_vl] += rsfec_symb_err_cnt[i];
                }
                cur_vl++;
            }
            break;
        case 4:
            if (start_lane == 0) {
                mem_idx = 0;
            } else if (start_lane == 1) {
                mem_idx = 2;
            } else if (start_lane == 2) {
                mem_idx = 4;
            } else if (start_lane == 3) {
                mem_idx = 6;
            } else if (start_lane == 4) {
                mem_idx = 8;
            } else if (start_lane == 5) {
                mem_idx = 10;
            } else if (start_lane == 6) {
                mem_idx = 12;
            } else if (start_lane == 7) {
                mem_idx = 14;
            }
            PHYMOD_IF_ERR_RETURN
                (plp_aperta2_mem_read(&phy_copy, phymodMemRsFecSymbErrMib, mem_idx, rsfec_symb_err_cnt));
            for (i = 0; i < 4; i++) {
                if (rsfec_symb_err_cnt[i] > 0xffffffff - vl_symb_err_cnt[cur_vl]) {
                    vl_symb_err_cnt[cur_vl] = 0xffffffff;
                } else {
                    vl_symb_err_cnt[cur_vl] += rsfec_symb_err_cnt[i];
                }
                cur_vl++;
            }
            break;
        case 8:
            if (start_lane == 0) {
                start_idx = 0;
            } else if (start_lane == 2) {
                start_idx = 4;
            } else if (start_lane == 4) {
                start_idx = 8;
            } else if (start_lane == 6) {
                start_idx = 12;
            }
            mem_entry = start_idx + 2;
            for (mem_idx = start_idx; mem_idx < mem_entry; mem_idx++) {
                if (cur_vl == 8) {
                    cur_vl = 0;
                }
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_mem_read(&phy_copy, phymodMemRsFecSymbErrMib, mem_idx, rsfec_symb_err_cnt));
                for (i = 0; i < 4; i++) {
                    if (rsfec_symb_err_cnt[i] > 0xffffffff - vl_symb_err_cnt[cur_vl]) {
                        vl_symb_err_cnt[cur_vl] = 0xffffffff;
                    } else {
                        vl_symb_err_cnt[cur_vl] += rsfec_symb_err_cnt[i];
                    }
                    cur_vl++;
                }
            }
            break;
        case 16:
            if (start_lane == 0) {
                start_idx = 0;
            } else {
                start_idx = 8;
            }
            mem_entry = start_idx + 8;
            for (mem_idx = start_idx; mem_idx < mem_entry; mem_idx++) {
                if (cur_vl == 16) {
                    cur_vl = 0;
                }
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_mem_read(&phy_copy, phymodMemRsFecSymbErrMib, mem_idx, rsfec_symb_err_cnt));
                for (i = 0; i < 4; i++) {
                    if (rsfec_symb_err_cnt[i] > 0xffffffff - vl_symb_err_cnt[cur_vl]) {
                        vl_symb_err_cnt[cur_vl] = 0xffffffff;
                    } else {
                        vl_symb_err_cnt[cur_vl] += rsfec_symb_err_cnt[i];
                    }
                    cur_vl++;
                }
            }
            break;
        case 32:
            /*FEC lanes (0-15): symbol lane counter 0-15 and 16-31 */
            for (mem_idx = 0; mem_idx < 8; mem_idx++) {
                if (cur_vl == 16) {
                    cur_vl = 0;
                }
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_mem_read(&phy_copy, phymodMemRsFecSymbErrMib, mem_idx, rsfec_symb_err_cnt));
                for (i = 0; i < 4; i++) {
                    if (rsfec_symb_err_cnt[i] > 0xffffffff - vl_symb_err_cnt[cur_vl]) {
                        vl_symb_err_cnt[cur_vl] = 0xffffffff;
                    } else {
                        vl_symb_err_cnt[cur_vl] += rsfec_symb_err_cnt[i];
                    }
                    cur_vl++;
                }
            }
            /*FEC lanes (16-31): symbol lane counter 32-47 and 48-63 */
            for (mem_idx = 8; mem_idx < 16; mem_idx++) {
                if (cur_vl == 32) {
                    cur_vl = 16;
                }
                PHYMOD_IF_ERR_RETURN
                    (plp_aperta2_mem_read(&phy_copy, phymodMemRsFecSymbErrMib, mem_idx, rsfec_symb_err_cnt));
                for (i = 0; i < 4; i++) {
                    if (rsfec_symb_err_cnt[i] > 0xffffffff - vl_symb_err_cnt[cur_vl]) {
                        vl_symb_err_cnt[cur_vl] = 0xffffffff;
                    } else {
                        vl_symb_err_cnt[cur_vl] += rsfec_symb_err_cnt[i];
                    }
                    cur_vl++;
                }
            }
            break;
        default:
            return PHYMOD_E_PARAM;
    }

    /* Map virtual lanes (FEC lanes) to logical lanes */
    for (i = 0; i < num_lanes; i++) {
        sum = 0;
        phy_copy.access.lane_mask = 0x1 << (start_lane + i);
        PHYMOD_IF_ERR_RETURN(READ_RX_X4_PSLL_TO_VL_MAP0r(&phy_copy, &RX_X4_PSLL_TO_VL_MAP0r_reg));
        PHYMOD_IF_ERR_RETURN(READ_RX_X4_PSLL_TO_VL_MAP1r(&phy_copy, &RX_X4_PSLL_TO_VL_MAP1r_reg));
        if (num_psll_per_phyl > 0) {
            cur_vl = RX_X4_PSLL_TO_VL_MAP0r_PSLL0_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP0r_reg);
            sum += vl_symb_err_cnt[cur_vl];
        }
        if (num_psll_per_phyl > 1) {
            cur_vl = RX_X4_PSLL_TO_VL_MAP0r_PSLL1_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP0r_reg);
            /* Check overflow */
            if (vl_symb_err_cnt[cur_vl] > 0xffffffff - sum) {
                sum = 0xffffffff;
            } else {
                sum += vl_symb_err_cnt[cur_vl];
            }
        }
        if (num_psll_per_phyl > 2) {
            cur_vl = RX_X4_PSLL_TO_VL_MAP0r_PSLL2_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP0r_reg);
            /* Check overflow */
            if (vl_symb_err_cnt[cur_vl] > 0xffffffff - sum) {
                sum = 0xffffffff;
            } else {
                sum += vl_symb_err_cnt[cur_vl];
            }
        }
        if (num_psll_per_phyl > 3) {
            cur_vl = RX_X4_PSLL_TO_VL_MAP1r_PSLL3_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP1r_reg);
            /* Check overflow */
            if (vl_symb_err_cnt[cur_vl] > 0xffffffff - sum) {
                sum = 0xffffffff;
            } else {
                sum += vl_symb_err_cnt[cur_vl];
            }
        }
        if (num_psll_per_phyl > 4) {
            cur_vl = RX_X4_PSLL_TO_VL_MAP1r_PSLL4_TO_VL_MAPPINGf_GET(RX_X4_PSLL_TO_VL_MAP1r_reg);
            /* Check overflow */
            if (vl_symb_err_cnt[cur_vl] > 0xffffffff - sum) {
                sum = 0xffffffff;
            } else {
                sum += vl_symb_err_cnt[cur_vl];
            }
        }
        symb_err_cnt[i] = sum;
    }

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_rs_fec_hi_ser_get(PHYMOD_ST* pc, uint32_t* hi_ser_lh, uint32_t* hi_ser_live)
{
    RX_X4_RS_FEC_RXP_STSr_t RX_X4_RS_FEC_RXP_STSr_reg;

    PHYMOD_IF_ERR_RETURN(READ_RX_X4_RS_FEC_RXP_STSr(pc, &RX_X4_RS_FEC_RXP_STSr_reg));
    *hi_ser_lh = RX_X4_RS_FEC_RXP_STSr_HI_SER_LHf_GET(RX_X4_RS_FEC_RXP_STSr_reg);
    *hi_ser_live = RX_X4_RS_FEC_RXP_STSr_HI_SER_LIVEf_GET(RX_X4_RS_FEC_RXP_STSr_reg);

    return PHYMOD_E_NONE;
}


int plp_aperta2_tscpmod_intr_status_get(PHYMOD_ST* pc, tscpmod_intr_status_t* intr_status)
{
    int num_lanes, start_lane;
    RX_X1_ECC_STS_RSFEC_RBUF_MPPr_t     RX_X1_ECC_STS_RSFEC_RBUF_MPPr_reg;
    RX_X1_ECC_STS_RSFEC_MPPr_t          RX_X1_ECC_STS_RSFEC_MPPr_reg;
    RX_X1_ECC_STS_DESKEWr_t             RX_X1_ECC_STS_DESKEWr_reg;
    RX_X1_ECC_STS_SPD_TBLr_t            RX_X1_ECC_STS_SPD_TBLr_reg;
    RX_X1_ECC_STS_AM_TBLr_t             RX_X1_ECC_STS_AM_TBLr_reg;
    RX_X1_ECC_STS_UM_TBLr_t             RX_X1_ECC_STS_UM_TBLr_reg;
    RX_X1_ECC_STS_TX_1588r_t            RX_X1_ECC_STS_TX_1588r_reg;
    RX_X1_ECC_STS_TX_1588_2r_t          RX_X1_ECC_STS_TX_1588_2r_reg;
    RX_X4_RS_FEC_RXP_STSr_t             RX_X4_RS_FEC_RXP_STSr_reg;
    RX_X1_ECC_STS_DSL_DATA_FIFOr_t      RX_X1_ECC_STS_DSL_DATA_FIFOr_reg;
    RX_X1_ECC_ECC0_STS_RX_1588_MPPr_t   RX_X1_ECC_ECC0_STS_RX_1588_MPPr_reg;
    RX_X1_ECC_ECC1_STS_RX_1588_MPPr_t   RX_X1_ECC_ECC1_STS_RX_1588_MPPr_reg;
    PHYMOD_ST phy_copy;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lanes));

    PHYMOD_MEMCPY(&phy_copy, pc, sizeof(phy_copy));
    phy_copy.access.lane_mask = 0x1;
    switch (intr_status->type) {
        case phymodIntrEccRx1588Mpp1:
            phy_copy.access.lane_mask = 0x1 << 4;
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_ECC0_STS_RX_1588_MPPr(&phy_copy, &RX_X1_ECC_ECC0_STS_RX_1588_MPPr_reg));
            intr_status->is_2b_err = RX_X1_ECC_ECC0_STS_RX_1588_MPPr_TWO_BIT_ERR_EVENT0_RX_1588_MPPf_GET(RX_X1_ECC_ECC0_STS_RX_1588_MPPr_reg);
            intr_status->is_1b_err = RX_X1_ECC_ECC0_STS_RX_1588_MPPr_ONE_BIT_ERR_EVENT0_RX_1588_MPPf_GET(RX_X1_ECC_ECC0_STS_RX_1588_MPPr_reg);
            intr_status->err_addr = RX_X1_ECC_ECC0_STS_RX_1588_MPPr_ERR_EVENT_ADDRESS0_RX_1588_MPPf_GET(RX_X1_ECC_ECC0_STS_RX_1588_MPPr_reg);
            break;
        case phymodIntrEccRx1588Mpp0:
            /* even bank, MPP0 */
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X1_ECC_ECC0_STS_RX_1588_MPPr(&phy_copy, &RX_X1_ECC_ECC0_STS_RX_1588_MPPr_reg));
            intr_status->is_2b_err = RX_X1_ECC_ECC0_STS_RX_1588_MPPr_TWO_BIT_ERR_EVENT0_RX_1588_MPPf_GET(RX_X1_ECC_ECC0_STS_RX_1588_MPPr_reg);
            intr_status->is_1b_err = RX_X1_ECC_ECC0_STS_RX_1588_MPPr_ONE_BIT_ERR_EVENT0_RX_1588_MPPf_GET(RX_X1_ECC_ECC0_STS_RX_1588_MPPr_reg);
            intr_status->err_addr = RX_X1_ECC_ECC0_STS_RX_1588_MPPr_ERR_EVENT_ADDRESS0_RX_1588_MPPf_GET(RX_X1_ECC_ECC0_STS_RX_1588_MPPr_reg);
            break;
        case phymodIntrEccRx1588_2Mpp1:
            phy_copy.access.lane_mask = 0x1 << 4;
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_ECC1_STS_RX_1588_MPPr(&phy_copy, &RX_X1_ECC_ECC1_STS_RX_1588_MPPr_reg));
            intr_status->is_2b_err = RX_X1_ECC_ECC1_STS_RX_1588_MPPr_TWO_BIT_ERR_EVENT1_RX_1588_MPPf_GET(RX_X1_ECC_ECC1_STS_RX_1588_MPPr_reg);
            intr_status->is_1b_err = RX_X1_ECC_ECC1_STS_RX_1588_MPPr_ONE_BIT_ERR_EVENT1_RX_1588_MPPf_GET(RX_X1_ECC_ECC1_STS_RX_1588_MPPr_reg);
            intr_status->err_addr = RX_X1_ECC_ECC1_STS_RX_1588_MPPr_ERR_EVENT_ADDRESS1_RX_1588_MPPf_GET(RX_X1_ECC_ECC1_STS_RX_1588_MPPr_reg);
            break;
        case phymodIntrEccRx1588_2Mpp0:
            /* odd bank, MPP0 */
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_ECC1_STS_RX_1588_MPPr(&phy_copy, &RX_X1_ECC_ECC1_STS_RX_1588_MPPr_reg));
            intr_status->is_2b_err = RX_X1_ECC_ECC1_STS_RX_1588_MPPr_TWO_BIT_ERR_EVENT1_RX_1588_MPPf_GET(RX_X1_ECC_ECC1_STS_RX_1588_MPPr_reg);
            intr_status->is_1b_err = RX_X1_ECC_ECC1_STS_RX_1588_MPPr_ONE_BIT_ERR_EVENT1_RX_1588_MPPf_GET(RX_X1_ECC_ECC1_STS_RX_1588_MPPr_reg);
            intr_status->err_addr = RX_X1_ECC_ECC1_STS_RX_1588_MPPr_ERR_EVENT_ADDRESS1_RX_1588_MPPf_GET(RX_X1_ECC_ECC1_STS_RX_1588_MPPr_reg);
            break;
        case phymodIntrEccTx1588Mpp0:
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X1_ECC_STS_TX_1588r(&phy_copy, &RX_X1_ECC_STS_TX_1588r_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_TX_1588r_TWO_BIT_ERR_EVENT_TX_1588f_GET(RX_X1_ECC_STS_TX_1588r_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_TX_1588r_ONE_BIT_ERR_EVENT_TX_1588f_GET(RX_X1_ECC_STS_TX_1588r_reg);
            intr_status->err_addr = RX_X1_ECC_STS_TX_1588r_ERR_EVENT_ADDRESS_TX_1588f_GET(RX_X1_ECC_STS_TX_1588r_reg);
            break;
        case phymodIntrEccTx1588Mpp1:
            phy_copy.access.lane_mask = 0x1 << 4;
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_TX_1588r(&phy_copy, &RX_X1_ECC_STS_TX_1588r_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_TX_1588r_TWO_BIT_ERR_EVENT_TX_1588f_GET(RX_X1_ECC_STS_TX_1588r_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_TX_1588r_ONE_BIT_ERR_EVENT_TX_1588f_GET(RX_X1_ECC_STS_TX_1588r_reg);
            intr_status->err_addr = RX_X1_ECC_STS_TX_1588r_ERR_EVENT_ADDRESS_TX_1588f_GET(RX_X1_ECC_STS_TX_1588r_reg);
            break;
        case phymodIntrEccTx1588_2Mpp0:
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X1_ECC_STS_TX_1588_2r(&phy_copy, &RX_X1_ECC_STS_TX_1588_2r_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_TX_1588_2r_TWO_BIT_ERR_EVENT_TX_1588_2f_GET(RX_X1_ECC_STS_TX_1588_2r_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_TX_1588_2r_ONE_BIT_ERR_EVENT_TX_1588_2f_GET(RX_X1_ECC_STS_TX_1588_2r_reg);
            intr_status->err_addr = RX_X1_ECC_STS_TX_1588_2r_ERR_EVENT_ADDRESS_TX_1588_2f_GET(RX_X1_ECC_STS_TX_1588_2r_reg);
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
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_DESKEWr(&phy_copy, &RX_X1_ECC_STS_DESKEWr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_DESKEWr_TWO_BIT_ERR_EVENT_DESKEWf_GET(RX_X1_ECC_STS_DESKEWr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_DESKEWr_ONE_BIT_ERR_EVENT_DESKEWf_GET(RX_X1_ECC_STS_DESKEWr_reg);
            intr_status->err_addr = 0xffff;
            break;
        case phymodIntrEccDslDataFifo:
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_DSL_DATA_FIFOr(&phy_copy, &RX_X1_ECC_STS_DSL_DATA_FIFOr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_DSL_DATA_FIFOr_TWO_BIT_ERR_EVENT_DSL_DATA_FIFOf_GET(RX_X1_ECC_STS_DSL_DATA_FIFOr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_DSL_DATA_FIFOr_ONE_BIT_ERR_EVENT_DSL_DATA_FIFOf_GET(RX_X1_ECC_STS_DSL_DATA_FIFOr_reg);
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
        case phymodIntrEccRsFECMpp1:
            phy_copy.access.lane_mask = 0x1 << 4;
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_RSFEC_MPPr(&phy_copy, &RX_X1_ECC_STS_RSFEC_MPPr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_RSFEC_MPPr_TWO_BIT_ERR_EVENT_RSFEC_MPPf_GET(RX_X1_ECC_STS_RSFEC_MPPr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_RSFEC_MPPr_ONE_BIT_ERR_EVENT_RSFEC_MPPf_GET(RX_X1_ECC_STS_RSFEC_MPPr_reg);
            intr_status->err_addr = 0xffff;
            break;
        case phymodIntrEccRsFECMpp0:
            PHYMOD_IF_ERR_RETURN(READ_RX_X1_ECC_STS_RSFEC_MPPr(&phy_copy, &RX_X1_ECC_STS_RSFEC_MPPr_reg));
            intr_status->is_2b_err = RX_X1_ECC_STS_RSFEC_MPPr_TWO_BIT_ERR_EVENT_RSFEC_MPPf_GET(RX_X1_ECC_STS_RSFEC_MPPr_reg);
            intr_status->is_1b_err = RX_X1_ECC_STS_RSFEC_MPPr_ONE_BIT_ERR_EVENT_RSFEC_MPPf_GET(RX_X1_ECC_STS_RSFEC_MPPr_reg);
            intr_status->err_addr = 0xffff;
            break;
        case phymodIntrRsFecFdr:
            /* per port register, use start lane. */
            phy_copy.access.lane_mask = 0x1 << start_lane;
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_RS_FEC_RXP_STSr(&phy_copy,
                                            &RX_X4_RS_FEC_RXP_STSr_reg));
            intr_status->non_ecc_intr_set =
                 RX_X4_RS_FEC_RXP_STSr_FDR_INTERRUPT_STATUSf_GET
                                                    (RX_X4_RS_FEC_RXP_STSr_reg);
            break;
        default:
            return PHYMOD_E_UNAVAIL;
    }

    return PHYMOD_E_NONE;
}

/**
 * @brief   Set Per lane OS mode set in PMD
 * @param   pc handle to current TSCBH context (#PHYMOD_ST)
 * @param   mapped_speed_id: speed entry index in sw_speed_table
 * @param   os_mode over sample rate.
 * @returns The value PHYMOD_E_NONE upon successful completion
 * @details Per Port PMD Init
 *
 * */
 /* FIXME need to use PEregrine API to force OSR mode */
int plp_aperta2_tscpmod_pmd_osmode_set(PHYMOD_ST* pc, int mapped_speed_id, tscpmod_refclk_t refclk)
{
    RXTXCOM_OSR_MODE_CTLr_t reg_osr_mode;
    int os_mode = 0;

    RXTXCOM_OSR_MODE_CTLr_CLR(reg_osr_mode);

    /* 0=OS_MODE_1;    1=OS_MODE_2;
    * 2=OS_MODE_4;    3=OS_MODE_5;
    * 0x9=OS_MODE_8; 0x12=OS_MODE_16;
    * 0x11= OS_MODE_33 */
    PHYMOD_IF_ERR_RETURN
        (READ_RXTXCOM_OSR_MODE_CTLr(pc, &reg_osr_mode));

    if (refclk == TSCPMOD_REF_CLK_312P5MHZ) {
        os_mode =  plp_aperta2_tscpmod_sc_pmd_entry[mapped_speed_id].t_pma_os_mode;
    }

    RXTXCOM_OSR_MODE_CTLr_OSR_MODE_FRCf_SET(reg_osr_mode, 1);
    RXTXCOM_OSR_MODE_CTLr_OSR_MODE_FRC_VALf_SET(reg_osr_mode, os_mode);

    PHYMOD_IF_ERR_RETURN
        (MODIFY_RXTXCOM_OSR_MODE_CTLr(pc, reg_osr_mode));

  return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_tx_ts_info_unpack_tx_ts_tbl_entry(uint32_t *tx_ts_tbl_entry, tscpmod_ts_tx_info_t *tx_ts_info)
{
    int valid;

    /*
     * ts_sub_nano_field = mem_tsts_fifo_data[3:0];
     * ts_value_lo       = mem_tsts_fifo_data[19:4];
     * ts_value_mid      = mem_tsts_fifo_data[35:20];
     * ts_value_hi       = mem_tsts_fifo_data[51:36];
     * ts_sequence_id    = mem_tsts_fifo_data[67:52];
     * ts_valid          = mem_tsts_info_data[68];
     */

    valid = (tx_ts_tbl_entry[2] >> 4) & 0x1;

    if (valid) {
        tx_ts_info->ts_in_fifo_lo = ((tx_ts_tbl_entry[1] << 28) & 0xf0000000) | ((tx_ts_tbl_entry[0] >> 4) & 0xfffffff);
        tx_ts_info->ts_in_fifo_hi = (tx_ts_tbl_entry[1] >> 4) & 0xffff;
        tx_ts_info->ts_seq_id = ((tx_ts_tbl_entry[2] << 12) & 0xf000) | ((tx_ts_tbl_entry[1] >> 20) & 0xfff);
        tx_ts_info->ts_sub_nanosec = tx_ts_tbl_entry[0] & 0xf;
    } else {
        return PHYMOD_E_FAIL;
    }

    return PHYMOD_E_NONE;
}

/* this function  is to store port enable info */
int plp_aperta2_tscpmod_port_enable_set(PHYMOD_ST* pc, int enable)
{
    SC_X4_SW_SPARE1r_t SC_X4_SW_SPARE1r_reg;
    uint16_t temp_reg_value;

    SC_X4_SW_SPARE1r_CLR(SC_X4_SW_SPARE1r_reg);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_SW_SPARE1r(pc, &SC_X4_SW_SPARE1r_reg));
    temp_reg_value = SC_X4_SW_SPARE1r_GET(SC_X4_SW_SPARE1r_reg);
    /* first clear the lowest bit */
    temp_reg_value  &= ~TSCPMOD_PORT_ENABLE_MASK;
    temp_reg_value |= (enable & 0x1) << TSCPMOD_PORT_ENABLE_BIT_SHIFT;
    SC_X4_SW_SPARE1r_SET(SC_X4_SW_SPARE1r_reg, temp_reg_value);
    PHYMOD_IF_ERR_RETURN(WRITE_SC_X4_SW_SPARE1r(pc, SC_X4_SW_SPARE1r_reg));

    return PHYMOD_E_NONE;
}

/* this function is to get port enable info */
int plp_aperta2_tscpmod_port_enable_get(PHYMOD_ST* pc, int* enable)
{
    SC_X4_SW_SPARE1r_t SC_X4_SW_SPARE1r_reg;
    uint16_t temp_reg_value;

    SC_X4_SW_SPARE1r_CLR(SC_X4_SW_SPARE1r_reg);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_SW_SPARE1r(pc, &SC_X4_SW_SPARE1r_reg));
    temp_reg_value = SC_X4_SW_SPARE1r_GET(SC_X4_SW_SPARE1r_reg);
    *enable = (temp_reg_value & TSCPMOD_PORT_ENABLE_MASK) >> TSCPMOD_PORT_ENABLE_BIT_SHIFT;

    return PHYMOD_E_NONE;
}

/* this function  is to store port autoneg enable info */
int plp_aperta2_tscpmod_port_an_mode_enable_set(PHYMOD_ST* pc, int enable)
{
    SC_X4_SW_SPARE1r_t SC_X4_SW_SPARE1r_reg;
    uint16_t temp_reg_value;

    SC_X4_SW_SPARE1r_CLR(SC_X4_SW_SPARE1r_reg);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_SW_SPARE1r(pc, &SC_X4_SW_SPARE1r_reg));
    temp_reg_value = SC_X4_SW_SPARE1r_GET(SC_X4_SW_SPARE1r_reg);
    /* first clear the lowest bit */
    temp_reg_value  &= ~TSCPMOD_PORT_AN_ENABLE_MASK;
    temp_reg_value |= (enable & 0x1) << TSCPMOD_PORT_AN_ENABLE_BIT_SHIFT;
    SC_X4_SW_SPARE1r_SET(SC_X4_SW_SPARE1r_reg, temp_reg_value);
    PHYMOD_IF_ERR_RETURN(WRITE_SC_X4_SW_SPARE1r(pc, SC_X4_SW_SPARE1r_reg));

    return PHYMOD_E_NONE;
}

/* this function is to get port autoneg enable info */
int plp_aperta2_tscpmod_port_an_mode_enable_get(PHYMOD_ST* pc, int* enable)
{
    SC_X4_SW_SPARE1r_t SC_X4_SW_SPARE1r_reg;
    uint16_t temp_reg_value;

    SC_X4_SW_SPARE1r_CLR(SC_X4_SW_SPARE1r_reg);
    PHYMOD_IF_ERR_RETURN(READ_SC_X4_SW_SPARE1r(pc, &SC_X4_SW_SPARE1r_reg));
    temp_reg_value = SC_X4_SW_SPARE1r_GET(SC_X4_SW_SPARE1r_reg);
    *enable = (temp_reg_value & TSCPMOD_PORT_AN_ENABLE_MASK) >> TSCPMOD_PORT_AN_ENABLE_BIT_SHIFT ;

    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_port_cl73_enable_set(PHYMOD_ST* pc, int enable)
{
    AN_X4_CL73_CFGr_t      AN_X4_CL73_CFGr_reg;

    AN_X4_CL73_CFGr_CLR(AN_X4_CL73_CFGr_reg);
    PHYMOD_IF_ERR_RETURN (READ_AN_X4_CL73_CFGr(pc, &AN_X4_CL73_CFGr_reg));
    AN_X4_CL73_CFGr_CL73_ENABLEf_SET(AN_X4_CL73_CFGr_reg, enable);
    PHYMOD_IF_ERR_RETURN (MODIFY_AN_X4_CL73_CFGr(pc, AN_X4_CL73_CFGr_reg));
    return PHYMOD_E_NONE;
}

int plp_aperta2_tscpmod_port_cl73_enable_get(PHYMOD_ST* pc, int* enable)
{
    AN_X4_CL73_CFGr_t      AN_X4_CL73_CFGr_reg;

    PHYMOD_IF_ERR_RETURN(READ_AN_X4_CL73_CFGr(pc, &AN_X4_CL73_CFGr_reg));
    *enable = AN_X4_CL73_CFGr_CL73_ENABLEf_GET(AN_X4_CL73_CFGr_reg);
    return PHYMOD_E_NONE;
}

#ifdef  APERTA2_PM_UNSUPPORTED_API
int tscpmod_pmd_override_enable_set(PHYMOD_ST* pc,
                                        phymod_override_type_t pmd_override_type,
                                        uint32_t override_enable,
                                        uint32_t override_val)
{
    PMD_X4_OVRRr_t pmd_x4_override_reg;
    PMD_X4_CTLr_t  pmd_x4_ctrl_reg;
    PMD_X4_MODEr_t pmd_x4_mode_reg;
    plp_aperta2_phymod_phy_access_t pa_copy;

    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));

    PMD_X4_CTLr_CLR(pmd_x4_ctrl_reg);
    PMD_X4_OVRRr_CLR(pmd_x4_override_reg);
    PMD_X4_MODEr_CLR(pmd_x4_mode_reg);

    switch (pmd_override_type) {
        case phymodPMDLaneReset:
            if (override_enable) {
                PMD_X4_CTLr_LN_RX_DP_H_RSTBf_SET(pmd_x4_ctrl_reg, override_val);
                PMD_X4_CTLr_LN_TX_DP_H_RSTBf_SET(pmd_x4_ctrl_reg, override_val);
                PHYMOD_IF_ERR_RETURN
                    (MODIFY_PMD_X4_CTLr(&pa_copy, pmd_x4_ctrl_reg));
            }

            PMD_X4_OVRRr_LN_RX_DP_H_RSTB_OENf_SET(pmd_x4_override_reg, override_enable);
            PMD_X4_OVRRr_LN_TX_DP_H_RSTB_OENf_SET(pmd_x4_override_reg, override_enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_PMD_X4_OVRRr(&pa_copy, pmd_x4_override_reg));
            break;
        case phymodPMDTxDisable:
            if (override_enable) {
                PMD_X4_CTLr_TX_DISABLEf_SET(pmd_x4_ctrl_reg, override_val);
                PHYMOD_IF_ERR_RETURN
                    (MODIFY_PMD_X4_CTLr(&pa_copy, pmd_x4_ctrl_reg));
            }
            PMD_X4_OVRRr_TX_DISABLE_OENf_SET(pmd_x4_override_reg, override_enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_PMD_X4_OVRRr(&pa_copy, pmd_x4_override_reg));
            break;
        case phymodPMDRxLock:
            PMD_X4_OVRRr_RX_LOCK_OVRDf_SET(pmd_x4_override_reg, override_enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_PMD_X4_OVRRr(&pa_copy, pmd_x4_override_reg));
            break;
        case phymodPMDSignalDetect:
            PMD_X4_OVRRr_SIGNAL_DETECT_OVRDf_SET(pmd_x4_override_reg, override_enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_PMD_X4_OVRRr(&pa_copy, pmd_x4_override_reg));
            break;
        case phymodPMDRxClkValid:
            PMD_X4_OVRRr_RX_CLK_VLD_OVRDf_SET(pmd_x4_override_reg, override_enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_PMD_X4_OVRRr(&pa_copy, pmd_x4_override_reg));
            break;
        case phymodPMDTxClkValid:
            PMD_X4_OVRRr_TX_CLK_VLD_OVRDf_SET(pmd_x4_override_reg, override_enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_PMD_X4_OVRRr(&pa_copy, pmd_x4_override_reg));
            break;
        case phymodPMDLaneMode:
            if (override_enable) {
                PMD_X4_MODEr_LN_TX_LANE_MODEf_SET(pmd_x4_mode_reg, override_val);
                PMD_X4_MODEr_LN_RX_LANE_MODEf_SET(pmd_x4_mode_reg, override_val);
                PHYMOD_IF_ERR_RETURN
                    (MODIFY_PMD_X4_MODEr(&pa_copy, pmd_x4_mode_reg));
            }
            PMD_X4_OVRRr_LN_RX_LANE_MODE_OENf_SET(pmd_x4_override_reg, override_enable);
            PMD_X4_OVRRr_LN_TX_LANE_MODE_OENf_SET(pmd_x4_override_reg, override_enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_PMD_X4_OVRRr(&pa_copy, pmd_x4_override_reg));
            break;
        default:
            PHYMOD_DEBUG_ERROR(("Unsupported PMD override type\n"));
            return PHYMOD_E_PARAM;
    }
    return PHYMOD_E_NONE;
}
#endif
int plp_aperta2_tscpmod_pmd_tx_pcs_delay_cnt_set(PHYMOD_ST* pc, uint32_t delay_cnt)
{
#if NOCODE
    TLB_TX_TLB_TX_MISC_CFGr_t tx_misc_cfg_reg;

    TLB_TX_TLB_TX_MISC_CFGr_CLR(tx_misc_cfg_reg);
    PHYMOD_IF_ERR_RETURN(READ_TLB_TX_TLB_TX_MISC_CFGr(pc, &tx_misc_cfg_reg));
    TLB_TX_TLB_TX_MISC_CFGr_TX_PCS_INTF_PROG_DLY_CNTf_SET(tx_misc_cfg_reg, delay_cnt);

    PHYMOD_IF_ERR_RETURN(MODIFY_TLB_TX_TLB_TX_MISC_CFGr(pc, tx_misc_cfg_reg));
#endif
    return PHYMOD_E_NONE;
}


int
plp_aperta2_tscpmod_interrupt_enable_get(PHYMOD_ST *pc,
                             phymod_interrupt_type_t intr_type,
                             uint32_t *enable)
{
    PHYMOD_ST pc_copy;
    int num_lanes, start_lane;
    RX_X4_FDR_INTRr_t fdr_intr_reg;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lanes));

    PHYMOD_MEMCPY(&pc_copy, pc, sizeof(pc_copy));
    pc_copy.access.lane_mask = 0x1;

    switch (intr_type) {
        case phymodIntrRsFecFdr:
            /* per port register, use start lane. */
            pc_copy.access.lane_mask = 0x1 << start_lane;
            RX_X4_FDR_INTRr_CLR(fdr_intr_reg);
            PHYMOD_IF_ERR_RETURN(READ_RX_X4_FDR_INTRr(&pc_copy, &fdr_intr_reg));
            *enable = RX_X4_FDR_INTRr_SYMB_ERR_INT_ENf_GET(fdr_intr_reg);
            break;
        case phymodIntrEccBaseRFEC:
        case phymodIntrEccRx1588400g:
        case phymodIntrEccRx1588Mpp0:
        case phymodIntrEccRx1588Mpp1:
        default:
            return PHYMOD_E_UNAVAIL;
    }

    return PHYMOD_E_NONE;
}

/* FIXME this need to re-visit */
int
plp_aperta2_tscpmod_interrupt_enable_set(PHYMOD_ST *pc,
                             phymod_interrupt_type_t intr_type,
                             uint32_t enable)
{
    PHYMOD_ST pc_copy;
    int num_lanes, start_lane;
    RX_X4_FDR_INTRr_t fdr_intr_reg;
    MAIN0_ECC_1B_ERR_INTR_ENr_t reg_1b;
    MAIN0_ECC_2B_ERR_INTR_ENr_t reg_2b;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lanes));

    PHYMOD_MEMCPY(&pc_copy, pc, sizeof(pc_copy));
    pc_copy.access.lane_mask = 0x1;
    MAIN0_ECC_1B_ERR_INTR_ENr_CLR(reg_1b);
    MAIN0_ECC_2B_ERR_INTR_ENr_CLR(reg_2b);

    switch (intr_type) {
        case phymodIntrRsFecFdr:
            /* per port register, use start lane. */
            pc_copy.access.lane_mask = 0x1 << start_lane;
            RX_X4_FDR_INTRr_CLR(fdr_intr_reg);
            RX_X4_FDR_INTRr_SYMB_ERR_INT_ENf_SET(fdr_intr_reg, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_RX_X4_FDR_INTRr(&pc_copy, fdr_intr_reg));
            break;
        case phymodIntrEccRx1588Mpp0:
            MAIN0_ECC_1B_ERR_INTR_ENr_ECC_1B_ERR_INTR_EN0_RX_1588_MPPf_SET(reg_1b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_1B_ERR_INTR_ENr(&pc_copy, reg_1b));
            MAIN0_ECC_2B_ERR_INTR_ENr_ECC_2B_ERR_INTR_EN0_RX_1588_MPPf_SET(reg_2b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_2B_ERR_INTR_ENr(&pc_copy, reg_2b));
            break;

        case phymodIntrEccRx1588Mpp1:
            pc_copy.access.lane_mask = 0x10;
            MAIN0_ECC_1B_ERR_INTR_ENr_ECC_1B_ERR_INTR_EN0_RX_1588_MPPf_SET(reg_1b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_1B_ERR_INTR_ENr(&pc_copy, reg_1b));
            MAIN0_ECC_2B_ERR_INTR_ENr_ECC_2B_ERR_INTR_EN0_RX_1588_MPPf_SET(reg_2b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_2B_ERR_INTR_ENr(&pc_copy, reg_2b));
            break;

        case phymodIntrEccTx1588Mpp0:
            MAIN0_ECC_1B_ERR_INTR_ENr_ECC_1B_ERR_INTR_EN_TX_1588_MPPf_SET(reg_1b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_1B_ERR_INTR_ENr(&pc_copy, reg_1b));

            MAIN0_ECC_2B_ERR_INTR_ENr_ECC_2B_ERR_INTR_EN_TX_1588_MPPf_SET(reg_2b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_2B_ERR_INTR_ENr(&pc_copy, reg_2b));
            break;

        case phymodIntrEccTx1588Mpp1:
            pc_copy.access.lane_mask = 0x10;
            MAIN0_ECC_1B_ERR_INTR_ENr_ECC_1B_ERR_INTR_EN_TX_1588_MPPf_SET(reg_1b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_1B_ERR_INTR_ENr(&pc_copy, reg_1b));

            MAIN0_ECC_2B_ERR_INTR_ENr_ECC_2B_ERR_INTR_EN_TX_1588_MPPf_SET(reg_2b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_2B_ERR_INTR_ENr(&pc_copy, reg_2b));
            break;

        case phymodIntrEccTx1588_2Mpp0:
            MAIN0_ECC_1B_ERR_INTR_ENr_ECC_1B_ERR_INTR_EN_TX_1588_2f_SET(reg_1b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_1B_ERR_INTR_ENr(&pc_copy, reg_1b));

            MAIN0_ECC_2B_ERR_INTR_ENr_ECC_2B_ERR_INTR_EN_TX_1588_2f_SET(reg_2b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_2B_ERR_INTR_ENr(&pc_copy, reg_2b));
            break;

        case phymodIntrEccRx1588_2Mpp0:
            MAIN0_ECC_1B_ERR_INTR_ENr_ECC_1B_ERR_INTR_EN1_RX_1588_MPPf_SET(reg_1b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_1B_ERR_INTR_ENr(&pc_copy, reg_1b));
            MAIN0_ECC_2B_ERR_INTR_ENr_ECC_2B_ERR_INTR_EN1_RX_1588_MPPf_SET(reg_2b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_2B_ERR_INTR_ENr(&pc_copy, reg_2b));
            break;

        case phymodIntrEccRx1588_2Mpp1:
            pc_copy.access.lane_mask = 0x10;
            MAIN0_ECC_1B_ERR_INTR_ENr_ECC_1B_ERR_INTR_EN1_RX_1588_MPPf_SET(reg_1b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_1B_ERR_INTR_ENr(&pc_copy, reg_1b));
            MAIN0_ECC_2B_ERR_INTR_ENr_ECC_2B_ERR_INTR_EN1_RX_1588_MPPf_SET(reg_2b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_2B_ERR_INTR_ENr(&pc_copy, reg_2b));
            break;

        case phymodIntrEccUMTable:

            MAIN0_ECC_1B_ERR_INTR_ENr_ECC_1B_ERR_INTR_EN_UM_TBLf_SET(reg_1b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_1B_ERR_INTR_ENr(&pc_copy, reg_1b));
            MAIN0_ECC_2B_ERR_INTR_ENr_ECC_2B_ERR_INTR_EN_UM_TBLf_SET(reg_2b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_2B_ERR_INTR_ENr(&pc_copy, reg_2b));
            break;

        case phymodIntrEccAMTable:
            MAIN0_ECC_1B_ERR_INTR_ENr_ECC_1B_ERR_INTR_EN_AM_TBLf_SET(reg_1b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_1B_ERR_INTR_ENr(&pc_copy, reg_1b));
            MAIN0_ECC_2B_ERR_INTR_ENr_ECC_2B_ERR_INTR_EN_AM_TBLf_SET(reg_2b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_2B_ERR_INTR_ENr(&pc_copy, reg_2b));
            break;


        case phymodIntrEccSpeedTable:

            MAIN0_ECC_1B_ERR_INTR_ENr_ECC_1B_ERR_INTR_EN_SPD_TBLf_SET(reg_1b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_1B_ERR_INTR_ENr(&pc_copy, reg_1b));
            MAIN0_ECC_2B_ERR_INTR_ENr_ECC_2B_ERR_INTR_EN_SPD_TBLf_SET(reg_2b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_2B_ERR_INTR_ENr(&pc_copy, reg_2b));
            break;

        case phymodIntrEccDeskew:

            MAIN0_ECC_1B_ERR_INTR_ENr_ECC_1B_ERR_INTR_EN_DESKEWf_SET(reg_1b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_1B_ERR_INTR_ENr(&pc_copy, reg_1b));
            MAIN0_ECC_2B_ERR_INTR_ENr_ECC_2B_ERR_INTR_EN_DESKEWf_SET(reg_2b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_2B_ERR_INTR_ENr(&pc_copy, reg_2b));
            break;

        case phymodIntrEccRsFECMpp0:

            MAIN0_ECC_1B_ERR_INTR_ENr_ECC_1B_ERR_INTR_EN_RSFEC_MPPf_SET(reg_1b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_1B_ERR_INTR_ENr(&pc_copy, reg_1b));
            MAIN0_ECC_2B_ERR_INTR_ENr_ECC_2B_ERR_INTR_EN_RSFEC_MPPf_SET(reg_2b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_2B_ERR_INTR_ENr(&pc_copy, reg_2b));
            break;

        case phymodIntrEccRsFECMpp1:
            pc_copy.access.lane_mask = 0x10;
            MAIN0_ECC_1B_ERR_INTR_ENr_ECC_1B_ERR_INTR_EN_RSFEC_MPPf_SET(reg_1b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_1B_ERR_INTR_ENr(&pc_copy, reg_1b));
            MAIN0_ECC_2B_ERR_INTR_ENr_ECC_2B_ERR_INTR_EN_RSFEC_MPPf_SET(reg_2b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_2B_ERR_INTR_ENr(&pc_copy, reg_2b));
            break;

        case phymodIntrEccRsFECRbufMpp0:
            MAIN0_ECC_1B_ERR_INTR_ENr_ECC_1B_ERR_INTR_EN_RSFEC_RBUF_MPPf_SET(reg_1b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_1B_ERR_INTR_ENr(&pc_copy, reg_1b));
            MAIN0_ECC_2B_ERR_INTR_ENr_ECC_2B_ERR_INTR_EN_RSFEC_RBUF_MPPf_SET(reg_2b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_2B_ERR_INTR_ENr(&pc_copy, reg_2b));
            break;

        case phymodIntrEccRsFECRbufMpp1:
            pc_copy.access.lane_mask = 0x10;
            MAIN0_ECC_1B_ERR_INTR_ENr_ECC_1B_ERR_INTR_EN_RSFEC_RBUF_MPPf_SET(reg_1b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_1B_ERR_INTR_ENr(&pc_copy, reg_1b));
            MAIN0_ECC_2B_ERR_INTR_ENr_ECC_2B_ERR_INTR_EN_RSFEC_RBUF_MPPf_SET(reg_2b, enable);
            PHYMOD_IF_ERR_RETURN
                (MODIFY_MAIN0_ECC_2B_ERR_INTR_ENr(&pc_copy, reg_2b));
            break;

        default:
            return PHYMOD_E_UNAVAIL;
    }

    return PHYMOD_E_NONE;
}
/* get timestamp entry availablilty indicator */
int plp_aperta2_tscpmod_1588_ts_valid_get(PHYMOD_ST* pc, uint16_t* ts_valid)
{
#if NOCODE
    TX_X4_TX_1588_TIMESTAMP_STSr_t reg_tx_1588_timestamp_sts;

    TX_X4_TX_1588_TIMESTAMP_STSr_CLR(reg_tx_1588_timestamp_sts);
    PHYMOD_IF_ERR_RETURN(READ_TX_X4_TX_1588_TIMESTAMP_STSr(pc, &reg_tx_1588_timestamp_sts));
    *ts_valid = TX_X4_TX_1588_TIMESTAMP_STSr_TS_ENTRY_VALIDf_GET(reg_tx_1588_timestamp_sts);
#endif
    return PHYMOD_E_NONE;
}
#if 0
int tscpmod_fec_error_inject_config_set(PHYMOD_ST *pc,
                                        const phymod_fec_error_injection_config_t *error_injection_config)
{
    TX_X1_ERRMASK0r_t  err_mask0_reg;
    TX_X1_ERRMASK1r_t  err_mask1_reg;
    TX_X1_ERRMASK2r_t  err_mask2_reg;
    TX_X1_ERRMASK3r_t  err_mask3_reg;
    TX_X1_ERRMASK4r_t  err_mask4_reg;
    TX_X4_ERR_CTLr_t   tx_x4_err_ctrl_reg;
    TX_X4_MISCr_t      tx_x4_misc_reg;

    /* Error mask bits. */
    TX_X1_ERRMASK0r_SET(err_mask0_reg, error_injection_config->error_mask_bit_31_0 & 0xffff);
    PHYMOD_IF_ERR_RETURN(WRITE_TX_X1_ERRMASK0r(pc, err_mask0_reg));
    TX_X1_ERRMASK1r_SET(err_mask1_reg, (error_injection_config->error_mask_bit_31_0 >> 16) & 0xffff);
    PHYMOD_IF_ERR_RETURN(WRITE_TX_X1_ERRMASK1r(pc, err_mask1_reg));
    TX_X1_ERRMASK2r_SET(err_mask2_reg, error_injection_config->error_mask_bit_63_32 & 0xffff);
    PHYMOD_IF_ERR_RETURN(WRITE_TX_X1_ERRMASK2r(pc, err_mask2_reg));
    TX_X1_ERRMASK3r_SET(err_mask3_reg, (error_injection_config->error_mask_bit_63_32 >> 16) & 0xffff);
    PHYMOD_IF_ERR_RETURN(WRITE_TX_X1_ERRMASK3r(pc, err_mask3_reg));
    TX_X1_ERRMASK4r_SET(err_mask4_reg, error_injection_config->error_mask_bit_79_64 & 0xf);
    PHYMOD_IF_ERR_RETURN(WRITE_TX_X1_ERRMASK4r(pc, err_mask4_reg));

    /* Error injection control. */
    /* Contiguous block count */
    TX_X4_ERR_CTLr_CLR(tx_x4_err_ctrl_reg);
    PHYMOD_IF_ERR_RETURN(READ_TX_X4_ERR_CTLr(pc, &tx_x4_err_ctrl_reg));
    TX_X4_ERR_CTLr_OFFSET_CONTROLf_SET(tx_x4_err_ctrl_reg, error_injection_config->block_offset);
    TX_X4_ERR_CTLr_CONTIGUOUS_BLOCKS_CONTROLf_SET(tx_x4_err_ctrl_reg, error_injection_config->contiguous_blocks_count);
    PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_ERR_CTLr(pc, tx_x4_err_ctrl_reg));

    /* Error rate */
    TX_X4_MISCr_CLR(tx_x4_misc_reg);
    PHYMOD_IF_ERR_RETURN(READ_TX_X4_MISCr(pc, &tx_x4_misc_reg));
    TX_X4_MISCr_ECWRf_SET(tx_x4_misc_reg, error_injection_config->error_injection_freq);
    PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_MISCr(pc,tx_x4_misc_reg));

    return PHYMOD_E_NONE;
}

int tscpmod_fec_error_inject_config_get(PHYMOD_ST *pc,
                                        phymod_fec_error_injection_config_t *error_injection_config)
{
    TX_X1_ERRMASK0r_t  err_mask0_reg;
    TX_X1_ERRMASK1r_t  err_mask1_reg;
    TX_X1_ERRMASK2r_t  err_mask2_reg;
    TX_X1_ERRMASK3r_t  err_mask3_reg;
    TX_X1_ERRMASK4r_t  err_mask4_reg;
    TX_X4_ERR_CTLr_t   tx_x4_err_ctrl_reg;
    TX_X4_MISCr_t      tx_x4_misc_reg;

    TX_X1_ERRMASK0r_CLR(err_mask0_reg);
    TX_X1_ERRMASK1r_CLR(err_mask1_reg);
    PHYMOD_IF_ERR_RETURN(READ_TX_X1_ERRMASK0r(pc, &err_mask0_reg));
    PHYMOD_IF_ERR_RETURN(READ_TX_X1_ERRMASK1r(pc, &err_mask1_reg));
    error_injection_config->error_mask_bit_31_0 = (TX_X1_ERRMASK1r_GET(err_mask1_reg) & 0xffff) << 16 |
                                                   (TX_X1_ERRMASK0r_GET(err_mask0_reg) & 0xffff);

    TX_X1_ERRMASK2r_CLR(err_mask2_reg);
    TX_X1_ERRMASK3r_CLR(err_mask3_reg);
    PHYMOD_IF_ERR_RETURN(READ_TX_X1_ERRMASK2r(pc, &err_mask2_reg));
    PHYMOD_IF_ERR_RETURN(READ_TX_X1_ERRMASK3r(pc, &err_mask3_reg));
    error_injection_config->error_mask_bit_63_32 = (TX_X1_ERRMASK3r_GET(err_mask3_reg) & 0xffff) << 16 |
                                                    (TX_X1_ERRMASK2r_GET(err_mask2_reg) & 0xffff);

    TX_X1_ERRMASK4r_CLR(err_mask4_reg);
    PHYMOD_IF_ERR_RETURN(READ_TX_X1_ERRMASK4r(pc, &err_mask4_reg));
    error_injection_config->error_mask_bit_79_64 = TX_X1_ERRMASK4r_GET(err_mask4_reg) & 0xf;

    TX_X4_ERR_CTLr_CLR(tx_x4_err_ctrl_reg);
    PHYMOD_IF_ERR_RETURN(READ_TX_X4_ERR_CTLr(pc, &tx_x4_err_ctrl_reg));
    error_injection_config->block_offset = TX_X4_ERR_CTLr_OFFSET_CONTROLf_GET(tx_x4_err_ctrl_reg);
    error_injection_config->contiguous_blocks_count = TX_X4_ERR_CTLr_CONTIGUOUS_BLOCKS_CONTROLf_GET(tx_x4_err_ctrl_reg);

    TX_X4_MISCr_CLR(tx_x4_misc_reg);
    PHYMOD_IF_ERR_RETURN(READ_TX_X4_MISCr(pc, &tx_x4_misc_reg));
    error_injection_config->error_injection_freq = TX_X4_MISCr_ECWRf_GET(tx_x4_misc_reg);

    return PHYMOD_E_NONE;
}

int tscpmod_fec_error_inject_enable_set(PHYMOD_ST* pc, const phymod_fec_error_injection_enable_t* error_injection_enable)
{
    TX_X4_ERR_CTLr_t   tx_x4_err_ctrl_reg;

    TX_X4_ERR_CTLr_CLR(tx_x4_err_ctrl_reg);
    PHYMOD_IF_ERR_RETURN(READ_TX_X4_ERR_CTLr(pc, &tx_x4_err_ctrl_reg));
    TX_X4_ERR_CTLr_CWA_PARITY_CONTROLf_SET(tx_x4_err_ctrl_reg, error_injection_enable->codeword_1_parity_enable);
    TX_X4_ERR_CTLr_CWB_PARITY_CONTROLf_SET(tx_x4_err_ctrl_reg, error_injection_enable->codeword_2_parity_enable);
    TX_X4_ERR_CTLr_CWA_CONTROLf_SET(tx_x4_err_ctrl_reg, error_injection_enable->codeword_1_enable);
    TX_X4_ERR_CTLr_CWB_CONTROLf_SET(tx_x4_err_ctrl_reg, error_injection_enable->codeword_2_enable);
    PHYMOD_IF_ERR_RETURN(MODIFY_TX_X4_ERR_CTLr(pc, tx_x4_err_ctrl_reg));

    return PHYMOD_E_NONE;
}

int tscpmod_fec_error_inject_enable_get(PHYMOD_ST* pc, phymod_fec_error_injection_enable_t* error_injection_enable)
{
    TX_X4_ERR_CTLr_t   tx_x4_err_ctrl_reg;

    TX_X4_ERR_CTLr_CLR(tx_x4_err_ctrl_reg);
    PHYMOD_IF_ERR_RETURN(READ_TX_X4_ERR_CTLr(pc, &tx_x4_err_ctrl_reg));
    error_injection_enable->codeword_1_parity_enable = TX_X4_ERR_CTLr_CWA_PARITY_CONTROLf_GET(tx_x4_err_ctrl_reg);
    error_injection_enable->codeword_2_parity_enable = TX_X4_ERR_CTLr_CWB_PARITY_CONTROLf_GET(tx_x4_err_ctrl_reg);
    error_injection_enable->codeword_1_enable = TX_X4_ERR_CTLr_CWA_CONTROLf_GET(tx_x4_err_ctrl_reg);
    error_injection_enable->codeword_2_enable = TX_X4_ERR_CTLr_CWB_CONTROLf_GET(tx_x4_err_ctrl_reg);

    return PHYMOD_E_NONE;
}
#endif
int plp_aperta2_tscpmod_fec_error_bits_counter_get (PHYMOD_ST* pc, uint32_t speed, uint32_t* count)
{
    int start_lane, num_lane;
    uint32_t count_32 = 0;
    uint64_t count_64, count_max;
    RX_X4_FEC_BIT_ERR_CTR0r_t count_low;
    RX_X4_FEC_BIT_ERR_CTR1r_t count_high;
    plp_aperta2_phymod_phy_access_t pa_copy;
    int lane = 0;
    int octal = PHYMOD_APERTA2_TSCP_GET_OCTAL(pc->access.lane_mask);

    PHYMOD_MEMCPY(&pa_copy, pc, sizeof(pa_copy));
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(&pc->access, &start_lane, &num_lane));
    COMPILER_64_ZERO(count_64);
    COMPILER_64_SET(count_max, 0, 0xffffffff);

    COMPILER_64_SET(count_64, 0, count_32);
    if (num_lane != 8) {
        /*
         * For 400G port 1, MPP0 copy0 and MPP0 copy2 register values need to be added to arrive at the final count.
         * For 400G port 4, MPP1 copy0 and MPP1 copy2 register values need to be added to arrive at the final count.
         */
        if (speed == 400000) {
            pa_copy.access.lane_mask = 0x1 << start_lane;
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_BIT_ERR_CTR0r(&pa_copy, &count_low));
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_BIT_ERR_CTR1r(&pa_copy, &count_high));
            count_32 = (RX_X4_FEC_BIT_ERR_CTR1r_GET(count_high) << 16) |
                        (RX_X4_FEC_BIT_ERR_CTR0r_GET(count_low) & 0xFFFF);
            COMPILER_64_ADD_32(count_64, count_32);

            pa_copy.access.lane_mask = 0x1 << (start_lane + 2);
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_BIT_ERR_CTR0r(&pa_copy, &count_low));
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_BIT_ERR_CTR1r(&pa_copy, &count_high));
            count_32 = (RX_X4_FEC_BIT_ERR_CTR1r_GET(count_high) << 16) |
                        (RX_X4_FEC_BIT_ERR_CTR0r_GET(count_low) & 0xFFFF);
            COMPILER_64_ADD_32(count_64, count_32);
        } else {
            pa_copy.access.lane_mask = 1 << start_lane;
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_BIT_ERR_CTR0r(&pa_copy, &count_low));
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_BIT_ERR_CTR1r(&pa_copy, &count_high));
            count_32 = (RX_X4_FEC_BIT_ERR_CTR1r_GET(count_high) << 16) |
                        (RX_X4_FEC_BIT_ERR_CTR0r_GET(count_low) & 0xFFFF);
            COMPILER_64_ADD_32(count_64, count_32);
        }
    } else {
        /*
         * For 800G port, MPP0 copy0, MPP0 copy2, MPP1 copy0 and MPP1 copy2 register values need to be added to arrive at the final count.
         */
        for (lane = 0; lane < TSCPMOD_NOF_LANES_IN_CORE; lane += 2) {
            pa_copy.access.lane_mask = 0x1 << (lane + PHYMOD_APERTA2_TSCP_OCTAL_SHIFT(octal));
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_BIT_ERR_CTR0r(&pa_copy, &count_low));
            PHYMOD_IF_ERR_RETURN
                (READ_RX_X4_FEC_BIT_ERR_CTR1r(&pa_copy, &count_high));
            count_32 = (RX_X4_FEC_BIT_ERR_CTR1r_GET(count_high) << 16) |
                        (RX_X4_FEC_BIT_ERR_CTR0r_GET(count_low) & 0xFFFF);
            COMPILER_64_ADD_32(count_64, count_32);
            /*
             * For 400G(8lanes), MPP0 copy0 and MPP0 copy2 register values need to be added to arrive at the final count.
             */
            if (speed == 400000 && lane == 2) {
                break;
            }
        }
    }

    /* Check overflow */
    if (COMPILER_64_GE(count_64, count_max)) {
        *count = 0xffffffff;
    } else {
        *count = COMPILER_64_LO(count_64);
    }


    return PHYMOD_E_NONE;
}

/* The number of BIP error */
int plp_aperta2_tscpmod_bip_error_counter_get(PHYMOD_ST* pc, uint32_t* count)
{
    RX_X4_BIPCNT0r_t count0;
    RX_X4_BIPCNT1r_t count1;
    RX_X4_BIPCNT2r_t count2;

    RX_X4_BIPCNT0r_CLR(count0);
    RX_X4_BIPCNT1r_CLR(count1);
    RX_X4_BIPCNT2r_CLR(count2);

    PHYMOD_IF_ERR_RETURN (READ_RX_X4_BIPCNT0r(pc, &count0));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_BIPCNT1r(pc, &count1));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_BIPCNT2r(pc, &count2));
    *count =  RX_X4_BIPCNT0r_BIP_ERROR_COUNT_0f_GET(count0) +
              RX_X4_BIPCNT0r_BIP_ERROR_COUNT_1f_GET(count0);
    *count += RX_X4_BIPCNT1r_BIP_ERROR_COUNT_2f_GET(count1) +
              RX_X4_BIPCNT1r_BIP_ERROR_COUNT_3f_GET(count1);
    *count += RX_X4_BIPCNT2r_BIP_ERROR_COUNT_4f_GET(count2);

    return PHYMOD_E_NONE;
}

/* The number of times BER_BAD_SH state is entered for cl49. */
int plp_aperta2_tscpmod_cl49_ber_counter_get(PHYMOD_ST* pc, uint32_t* count)
{
    RX_X4_DEC_STS3r_t dec_sts;

    RX_X4_DEC_STS3r_CLR(dec_sts);
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_DEC_STS3r(pc, &dec_sts));
    *count = RX_X4_DEC_STS3r_CL49_BER_COUNTf_GET(dec_sts);

    return PHYMOD_E_NONE;
}

/* The number of times BER_BAD_SH state is entered for cl82. */
int plp_aperta2_tscpmod_cl82_ber_counter_get(PHYMOD_ST* pc, uint32_t* count)
{
    RX_X4_BER_HOr_t count_ho;
    RX_X4_BER_LOr_t count_lo;

    RX_X4_BER_HOr_CLR(count_ho);
    RX_X4_BER_LOr_CLR(count_lo);
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_BER_HOr(pc, &count_ho));
    PHYMOD_IF_ERR_RETURN (READ_RX_X4_BER_LOr(pc, &count_lo));
    *count = (RX_X4_BER_HOr_BER_HOf_GET(count_ho) << 8) |
             (RX_X4_BER_LOr_BER_LOf_GET(count_lo));

    return PHYMOD_E_NONE;
}
#ifdef  APERTA2_PM_UNSUPPORTED_API
int
tscpmod_rsfec_symbol_error_mem_get(PHYMOD_ST* pc,
                                   int bit_mux_mode,
                                   phymod_rsfec_symbol_err_mem_info_t* sym_err_mem_info)
{
    int num_lanes, start_lane, virtual_lanes, num_psll_per_phyl;
    int mem_idx = 0, start_idx = 0;

    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_phymod_util_lane_config_get(pc, &start_lane, &num_lanes));

    PHYMOD_IF_ERR_RETURN
        (_plp_aperta2_tscpmod_virtual_lane_count_get(bit_mux_mode, num_lanes, &virtual_lanes, &num_psll_per_phyl));

    switch (virtual_lanes) {
    /*
     * The mapping of the symbol error counters to entries is as followed:
     * MPP0:Entry_0-Entry_7;
     * MPP1:Entry_8-Entry_15;
     * Each entry has 4 RSFEC symbol error counters
     * There are two FECs within each MPP. One FEC supports 16 symbol lane error counters.
     *
     * virtual_lanes= 1, if port start lane is 0, read symbol lane counter 0;
     *                   if port start lane is 1, read symbol lane counter 8;
     *                   if port start lane is 2, read symbol lane counter 16;
     *                   if port start lane is 3, read symbol lane counter 24;
     *                   if port start lane is 4, read symbol lane counter 32;
     *                   if port start lane is 5, read symbol lane counter 40;
     *                   if port start lane is 6, read symbol lane counter 48;
     *                   if port start lane is 7, read symbol lane counter 56;
     * virtual_lanes= 2, if port start lane is 0, read symbol lane counter 0 and 1;
     *                   if port start lane is 1, read symbol lane counter 8 and 9;
     *                   if port start lane is 2, read symbol lane counter 16 and 17;
     *                   if port start lane is 3, read symbol lane counter 24 and 25;
     *                   if port start lane is 4, read symbol lane counter 32 and 33;
     *                   if port start lane is 5, read symbol lane counter 40 and 41;
     *                   if port start lane is 6, read symbol lane counter 48 and 49;
     *                   if port start lane is 7, read symbol lane counter 56 and 57;
     * virtual_lanes= 4, if port start lane is 0, read symbol lane counter 0 to 3;
     *                   if port start lane is 1, read symbol lane counter 8 to 11;
     *                   if port start lane is 2, read symbol lane counter 16 to 19;
     *                   if port start lane is 3, read symbol lane counter 24 to 27;
     *                   if port start lane is 4, read symbol lane counter 32 to 35;
     *                   if port start lane is 5, read symbol lane counter 40 to 43;
     *                   if port start lane is 6, read symbol lane counter 48 to 51;
     *                   if port start lane is 7, read symbol lane counter 56 to 59;
     * virtual_lanes= 8, if port start lane is 0, read symbol lane counter 0 to 7;
     *                   if port start lane is 2, read symbol lane counter 16 to 23;
     *                   if port start lane is 4, read symbol lane counter 32 to 39;
     *                   if port start lane is 6, read symbol lane counter 48 to 55;
     * virtual_lanes= 16, if port in MPP0, symbol lane counter 0-15 and 16-31;
     *                    if port in MPP1, read symbol lane counter 32-47 and 48-63;
     * virtual_lanes = 32, read FEC lanes 0-15:(symbol lane counter 0-15 and 16-31)
     *                      and FEC lanes 16-31:(symbol lane counter 32-47 and 48-63);
     */
        case 1:
        case 2:
        case 4:
            if (start_lane == 0) {
                mem_idx = 0;
            } else if (start_lane == 1) {
                mem_idx = 2;
            } else if (start_lane == 2) {
                mem_idx = 4;
            } else if (start_lane == 3) {
                mem_idx = 6;
            } else if (start_lane == 4) {
                mem_idx = 8;
            } else if (start_lane == 5) {
                mem_idx = 10;
            } else if (start_lane == 6) {
                mem_idx = 12;
            } else if (start_lane == 7) {
                mem_idx = 14;
            }
            sym_err_mem_info->rsfec_sym_err_mem_start_entry = mem_idx;
            sym_err_mem_info->num_rsfec_sym_err_mem_entry= 1;
            sym_err_mem_info->rsfec_sym_err_mem_counter_bitmap =
                (virtual_lanes == 1)? 0x1 : (virtual_lanes == 2)? 0x3 : 0xf;
            break;
        case 8:
            if (start_lane == 0) {
                start_idx = 0;
            } else if (start_lane == 2) {
                start_idx = 4;
            } else if (start_lane == 4) {
                start_idx = 8;
            } else if (start_lane == 6) {
                start_idx = 12;
            }
            sym_err_mem_info->rsfec_sym_err_mem_start_entry = start_idx;
            sym_err_mem_info->num_rsfec_sym_err_mem_entry = 2;
            /* Each entry has 4 symbol error counters. */
            sym_err_mem_info->rsfec_sym_err_mem_counter_bitmap = 0xf;
            break;
        case 16:
            if (start_lane == 0) {
                start_idx = 0;
            } else {
                start_idx = 8;
            }
            sym_err_mem_info->rsfec_sym_err_mem_start_entry = start_idx;
            sym_err_mem_info->num_rsfec_sym_err_mem_entry = 8;
            /* Each entry has 4 symbol error counters. */
            sym_err_mem_info->rsfec_sym_err_mem_counter_bitmap = 0xf;
            break;
        case 32:
            sym_err_mem_info->rsfec_sym_err_mem_start_entry = 0;
            sym_err_mem_info->num_rsfec_sym_err_mem_entry = 16;
            /* Each entry has 4 symbol error counters. */
            sym_err_mem_info->rsfec_sym_err_mem_counter_bitmap = 0xf;
            break;
        default:
            return PHYMOD_E_PARAM;
    }

    return PHYMOD_E_NONE;
}
#endif
