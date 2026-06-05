/*
 * $Id: $
 *
 * $Copyright: (c) 2022 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */

#include "timesync.h"
#ifdef PHYMOD_APERTA2_SUPPORT
#include <aperta2_pm_seq.h>
#endif

int _plp_aperta2_timesync_config_set(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_timesync_config_t* config)
{
    PLP_ING_MISC_0_NSE_TS_BLK_ENr_t igr_nse_ts_blk_en ;
    P1588_EGR_TS_CTRLr_t  egr_ts_ctrl;
    P1588_ING_TS_CTRLr_t  igr_ts_ctrl;
    P1588_EGR_TS_CTRL2r_t egr_ts_ctrl2;
    P1588_ING_TS_CTRL2r_t igr_ts_ctrl2;

    P1588_EGR_PARSER_GEN_CTRLr_t   egr_ps_gen_ctrl;
    P1588_EGR_PARSER_IVLAN_PIDr_t  egr_itpid ;
    P1588_EGR_PARSER_OVLAN_PID0r_t egr_otpid ;
    P1588_EGR_PARSER_OVLAN_PID1r_t egr_otpid1;
    P1588_EGR_PARSER_OVLAN_PID2r_t egr_otpid2;

    P1588_ING_PARSER_GEN_CTRLr_t   igr_ps_gen_ctrl;
    P1588_ING_PARSER_IVLAN_PIDr_t  igr_itpid ;
    P1588_ING_PARSER_OVLAN_PID0r_t igr_otpid ;
    P1588_ING_PARSER_OVLAN_PID1r_t igr_otpid1;
    P1588_ING_PARSER_OVLAN_PID2r_t igr_otpid2;
    plp_aperta2_phymod_phy_access_t   igr_access, egr_access;
    uint32_t encr_seqid_mem_egr_ctrl = 0;
    uint32_t encr_seqid_mem_blk_en   = 0;
    uint32_t port  = 0;

    PHYMOD_MEMCPY(&igr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&egr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));

    PHYMOD_MEMSET(&igr_nse_ts_blk_en, 0, sizeof(PLP_ING_MISC_0_NSE_TS_BLK_ENr_t));

    PHYMOD_MEMSET(&egr_ts_ctrl,     0, sizeof(P1588_EGR_TS_CTRLr_t));
    PHYMOD_MEMSET(&igr_ts_ctrl,     0, sizeof(P1588_ING_TS_CTRLr_t));

    PHYMOD_MEMSET(&egr_ts_ctrl2,    0, sizeof(P1588_EGR_TS_CTRL2r_t));
    PHYMOD_MEMSET(&igr_ts_ctrl2,    0, sizeof(P1588_ING_TS_CTRL2r_t));

    PHYMOD_MEMSET(&egr_ps_gen_ctrl, 0, sizeof(P1588_EGR_PARSER_GEN_CTRLr_t))  ;
    PHYMOD_MEMSET(&egr_itpid,       0, sizeof(P1588_EGR_PARSER_IVLAN_PIDr_t)) ;
    PHYMOD_MEMSET(&egr_otpid,       0, sizeof(P1588_EGR_PARSER_OVLAN_PID0r_t));
    PHYMOD_MEMSET(&egr_otpid1,      0, sizeof(P1588_EGR_PARSER_OVLAN_PID1r_t));
    PHYMOD_MEMSET(&egr_otpid2,      0, sizeof(P1588_EGR_PARSER_OVLAN_PID2r_t));

    PHYMOD_MEMSET(&igr_ps_gen_ctrl, 0, sizeof(P1588_ING_PARSER_GEN_CTRLr_t))  ;
    PHYMOD_MEMSET(&igr_itpid,       0, sizeof(P1588_ING_PARSER_IVLAN_PIDr_t)) ;
    PHYMOD_MEMSET(&igr_otpid,       0, sizeof(P1588_ING_PARSER_OVLAN_PID0r_t));
    PHYMOD_MEMSET(&igr_otpid1,      0, sizeof(P1588_ING_PARSER_OVLAN_PID1r_t));
    PHYMOD_MEMSET(&igr_otpid2,      0, sizeof(P1588_ING_PARSER_OVLAN_PID2r_t));

#ifdef PHYMOD_APERTA2_SUPPORT
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_get_igr_egr_access(phy, &igr_access, &egr_access));
#endif

    PHYMOD_IF_ERR_RETURN(READ_ING_MISC_0_NSE_TS_BLK_ENr(&igr_access, &igr_nse_ts_blk_en));

    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_TS_CTRLr(&egr_access, &egr_ts_ctrl));
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_TS_CTRLr(&igr_access, &igr_ts_ctrl));

    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_TS_CTRL2r(&egr_access, &egr_ts_ctrl2));
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_TS_CTRL2r(&igr_access, &igr_ts_ctrl2));

    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_PARSER_GEN_CTRLr(&egr_access, &egr_ps_gen_ctrl));
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_PARSER_GEN_CTRLr(&igr_access, &igr_ps_gen_ctrl));

    P1588_EGR_TS_CTRLr_TC_ENf_SET(egr_ts_ctrl, config->tx_mode_tc);          /* 0 : Boundary Clock, 1: Transparent clock           */
    P1588_EGR_TS_CTRLr_TS_MODEf_SET(egr_ts_ctrl, config->tx_partial_tc);     /* 0 : IEEE TS Mode,   1 : BRCM TS mode               */
    P1588_EGR_TS_CTRLr_RSV2_CLEARf_SET(egr_ts_ctrl, config->tx_inband_clear);/* Enable port to clear reserved2 field in PTP packet */

    P1588_ING_TS_CTRLr_TC_ENf_SET(igr_ts_ctrl, config->rx_mode_tc);          /* 0 : Boundary Clock, 1: Transparent clock           */
    P1588_ING_TS_CTRLr_TS_MODEf_SET(igr_ts_ctrl, config->rx_partial_tc);     /* 0 : IEEE TS Mode,   1 : BRCM TS mode               */

    APERTA2_GET_PORT_FROM_LM(egr_access.access.lane_mask, port);
    if (PHYMOD_TS_F_1588_ENCRYPTED_MODE_GET(config->flags)) {
        P1588_EGR_TS_CTRLr_ENCRYPTION_ENf_SET(egr_ts_ctrl, 1);
        PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_read(&egr_access,  (TS_ENCR_SEQID_MEM_EGR_CTRL_ADDR + (((port >= 8) ? (port - 8) : port) << 4)) , &encr_seqid_mem_egr_ctrl));
        encr_seqid_mem_egr_ctrl |= 0x1 ;
        PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_write(&egr_access, (TS_ENCR_SEQID_MEM_EGR_CTRL_ADDR + (((port >= 8) ? (port - 8) : port) << 4)) , encr_seqid_mem_egr_ctrl));

        PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_read(&egr_access,   TS_ENCR_SEQID_MEM_EGR_BLK_CTRL_ADDR, &encr_seqid_mem_blk_en));
        encr_seqid_mem_blk_en |= 0x1 ;
        PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_write(&egr_access,  TS_ENCR_SEQID_MEM_EGR_BLK_CTRL_ADDR, encr_seqid_mem_blk_en));
    } else {
        P1588_EGR_TS_CTRLr_ENCRYPTION_ENf_SET(egr_ts_ctrl, 0);
        PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_read(&egr_access,  (TS_ENCR_SEQID_MEM_EGR_CTRL_ADDR + (((port >= 8) ? (port - 8) : port) << 4)) , &encr_seqid_mem_egr_ctrl));
        encr_seqid_mem_egr_ctrl &= 0xFFFE ;
        PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_write(&egr_access, (TS_ENCR_SEQID_MEM_EGR_CTRL_ADDR + (((port >= 8) ? (port - 8) : port) << 4)) , encr_seqid_mem_egr_ctrl));

        PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_read(&egr_access,   TS_ENCR_SEQID_MEM_EGR_BLK_CTRL_ADDR, &encr_seqid_mem_blk_en));
        encr_seqid_mem_blk_en &= 0xFFFE ;
        PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_write(&egr_access,  TS_ENCR_SEQID_MEM_EGR_BLK_CTRL_ADDR, encr_seqid_mem_blk_en));
    }

    if(PHYMOD_TS_F_NSE_ENABLED_GET(config->flags)) {           /* NSE TS for egress and Ingress            (packets encrypted)     */
        P1588_EGR_TS_CTRLr_DP_1588_ENf_SET(egr_ts_ctrl, 1);
        P1588_EGR_TS_CTRLr_PM_1588_ENf_SET(egr_ts_ctrl, 0);
        P1588_ING_TS_CTRLr_DP_1588_ENf_SET(igr_ts_ctrl, 1);
        P1588_ING_TS_CTRLr_PM_1588_ENf_SET(igr_ts_ctrl, 0);
        ING_MISC_0_NSE_TS_BLK_ENr_NSE_TS_DP_BLK_ENf_SET(igr_nse_ts_blk_en, 1);

    } else if(PHYMOD_TS_F_NSE_PM_ENABLED_GET(config->flags)) { /* NSE TS for egress and PM-TS  for Ingress (packets encrypted)     */
        P1588_EGR_TS_CTRLr_DP_1588_ENf_SET(egr_ts_ctrl, 1);
        P1588_EGR_TS_CTRLr_PM_1588_ENf_SET(egr_ts_ctrl, 0);
        P1588_ING_TS_CTRLr_DP_1588_ENf_SET(igr_ts_ctrl, 0);
        P1588_ING_TS_CTRLr_PM_1588_ENf_SET(igr_ts_ctrl, 1);
        ING_MISC_0_NSE_TS_BLK_ENr_NSE_TS_DP_BLK_ENf_SET(igr_nse_ts_blk_en, 0);

    } else if(PHYMOD_TS_F_PM_ENABLED_GET(config->flags)) {     /* PM TS  for egress and PM-TS  for Ingress (packets not encrypted) */
        P1588_EGR_TS_CTRLr_DP_1588_ENf_SET(egr_ts_ctrl, 0);
        P1588_EGR_TS_CTRLr_PM_1588_ENf_SET(egr_ts_ctrl, 1);
        P1588_ING_TS_CTRLr_DP_1588_ENf_SET(igr_ts_ctrl, 0);
        P1588_ING_TS_CTRLr_PM_1588_ENf_SET(igr_ts_ctrl, 1);
        ING_MISC_0_NSE_TS_BLK_ENr_NSE_TS_DP_BLK_ENf_SET(igr_nse_ts_blk_en, 0);

    } else if(PHYMOD_TS_F_PM_NSE_ENABLED_GET(config->flags)) { /* PM TS  for egress and NSE-TS for Ingress (packets not encrypted) */
        P1588_EGR_TS_CTRLr_DP_1588_ENf_SET(egr_ts_ctrl, 0);
        P1588_EGR_TS_CTRLr_PM_1588_ENf_SET(egr_ts_ctrl, 1);
        P1588_ING_TS_CTRLr_DP_1588_ENf_SET(igr_ts_ctrl, 1);
        P1588_ING_TS_CTRLr_PM_1588_ENf_SET(igr_ts_ctrl, 0);
        ING_MISC_0_NSE_TS_BLK_ENr_NSE_TS_DP_BLK_ENf_SET(igr_nse_ts_blk_en, 1);

    } else if(PHYMOD_TS_F_HYB_NSE_ENABLED_GET(config->flags)) {/* Hybrid for egress and NSE-TS for Ingress (packets not encrypted) */
        P1588_EGR_TS_CTRLr_DP_1588_ENf_SET(egr_ts_ctrl, 1);
        P1588_EGR_TS_CTRLr_PM_1588_ENf_SET(egr_ts_ctrl, 1);
        P1588_ING_TS_CTRLr_DP_1588_ENf_SET(igr_ts_ctrl, 1);
        P1588_ING_TS_CTRLr_PM_1588_ENf_SET(igr_ts_ctrl, 0);
        ING_MISC_0_NSE_TS_BLK_ENr_NSE_TS_DP_BLK_ENf_SET(igr_nse_ts_blk_en, 1);

    } else if(PHYMOD_TS_F_HYB_PM_ENABLED_GET(config->flags)) { /* Hybrid for egress and PM-TS  for Ingress (packets encrypted)     */
        P1588_EGR_TS_CTRLr_DP_1588_ENf_SET(egr_ts_ctrl, 1);
        P1588_EGR_TS_CTRLr_PM_1588_ENf_SET(egr_ts_ctrl, 1);
        P1588_ING_TS_CTRLr_DP_1588_ENf_SET(igr_ts_ctrl, 0);
        P1588_ING_TS_CTRLr_PM_1588_ENf_SET(igr_ts_ctrl, 1);
        ING_MISC_0_NSE_TS_BLK_ENr_NSE_TS_DP_BLK_ENf_SET(igr_nse_ts_blk_en, 0);

    } else {
        P1588_EGR_TS_CTRLr_DP_1588_ENf_SET(egr_ts_ctrl, 0);
        P1588_EGR_TS_CTRLr_PM_1588_ENf_SET(egr_ts_ctrl, 0);
        P1588_ING_TS_CTRLr_DP_1588_ENf_SET(igr_ts_ctrl, 0);
        P1588_ING_TS_CTRLr_PM_1588_ENf_SET(igr_ts_ctrl, 0);
        ING_MISC_0_NSE_TS_BLK_ENr_NSE_TS_DP_BLK_ENf_SET(igr_nse_ts_blk_en, 0);
    }

    P1588_EGR_TS_CTRLr_INT_MASK_ENf_SET(egr_ts_ctrl, config->sopmem_intr_enable);
    P1588_ING_TS_CTRLr_INT_MASK_ENf_SET(igr_ts_ctrl, config->sopmem_intr_enable);

    P1588_EGR_TS_CTRLr_WR_SOPMEM_ON_ERR_ENf_SET(egr_ts_ctrl, config->sopmem_wr_on_err);
    P1588_ING_TS_CTRLr_WR_SOPMEM_ON_ERR_ENf_SET(igr_ts_ctrl, config->sopmem_wr_on_err);

    P1588_EGR_TS_CTRLr_IP4UDP_CHKSUM_DISABLEf_SET(egr_ts_ctrl, config->ip4udp_chksum_disable);
    P1588_ING_TS_CTRLr_IP4UDP_CHKSUM_DISABLEf_SET(igr_ts_ctrl, config->ip4udp_chksum_disable);

    P1588_EGR_TS_CTRL2r_VXLAN_OUTER_CHKSUM_FORCE0f_SET(egr_ts_ctrl2, config->vxlan_outer_chksum_disable);
    P1588_ING_TS_CTRL2r_VXLAN_OUTER_CHKSUM_FORCE0f_SET(igr_ts_ctrl2, config->vxlan_outer_chksum_disable);
    if(config->rx_mode_tc) {
        P1588_ING_TS_CTRL2r_P1588_RX_CFG_TS80_ENf_SET(igr_ts_ctrl2, 0);
    } else {
        P1588_ING_TS_CTRL2r_P1588_RX_CFG_TS80_ENf_SET(igr_ts_ctrl2, 1);
    }

    if (PHYMOD_TS_F_L2_ENABLE_GET(config->flags)) {
        P1588_EGR_PARSER_GEN_CTRLr_L2_ENf_SET(egr_ps_gen_ctrl, 1);
        P1588_ING_PARSER_GEN_CTRLr_L2_ENf_SET(igr_ps_gen_ctrl, 1);
    } else {
        P1588_EGR_PARSER_GEN_CTRLr_L2_ENf_SET(egr_ps_gen_ctrl, 0);
        P1588_ING_PARSER_GEN_CTRLr_L2_ENf_SET(igr_ps_gen_ctrl, 0);
    }

    if (PHYMOD_TS_F_8021AS_ENABLE_GET(config->flags)) {
        P1588_EGR_PARSER_GEN_CTRLr_AS_ENf_SET(egr_ps_gen_ctrl, 1);
        P1588_ING_PARSER_GEN_CTRLr_AS_ENf_SET(igr_ps_gen_ctrl, 1);
    } else {
        P1588_EGR_PARSER_GEN_CTRLr_AS_ENf_SET(egr_ps_gen_ctrl, 0);
        P1588_ING_PARSER_GEN_CTRLr_AS_ENf_SET(igr_ps_gen_ctrl, 0);
    }

    if (PHYMOD_TS_F_IP6_ENABLE_GET(config->flags)) {
        P1588_EGR_PARSER_GEN_CTRLr_L4_IPV6_UDP_ENf_SET(egr_ps_gen_ctrl, 1);
        P1588_ING_PARSER_GEN_CTRLr_L4_IPV6_UDP_ENf_SET(igr_ps_gen_ctrl, 1);
    } else {
        P1588_EGR_PARSER_GEN_CTRLr_L4_IPV6_UDP_ENf_SET(egr_ps_gen_ctrl, 0);
        P1588_ING_PARSER_GEN_CTRLr_L4_IPV6_UDP_ENf_SET(igr_ps_gen_ctrl, 0);
    }

    if (PHYMOD_TS_F_IP4_ENABLE_GET(config->flags)) {
        P1588_EGR_PARSER_GEN_CTRLr_L4_IPV4_UDP_ENf_SET(egr_ps_gen_ctrl, 1);
        P1588_ING_PARSER_GEN_CTRLr_L4_IPV4_UDP_ENf_SET(igr_ps_gen_ctrl, 1);
    } else {
        P1588_EGR_PARSER_GEN_CTRLr_L4_IPV4_UDP_ENf_SET(egr_ps_gen_ctrl, 0);
        P1588_ING_PARSER_GEN_CTRLr_L4_IPV4_UDP_ENf_SET(igr_ps_gen_ctrl, 0);
    }

    P1588_EGR_PARSER_IVLAN_PIDr_SET(egr_itpid, config->itpid);
    P1588_ING_PARSER_IVLAN_PIDr_SET(igr_itpid, config->itpid_igr);

    P1588_EGR_PARSER_OVLAN_PID0r_SET(egr_otpid, config->otpid);
    P1588_ING_PARSER_OVLAN_PID0r_SET(igr_otpid, config->otpid_igr);

    P1588_EGR_PARSER_OVLAN_PID1r_SET(egr_otpid1, config->otpid1);
    P1588_ING_PARSER_OVLAN_PID1r_SET(igr_otpid1, config->otpid1_igr);

    P1588_EGR_PARSER_OVLAN_PID2r_SET(egr_otpid2, config->otpid2);
    P1588_ING_PARSER_OVLAN_PID2r_SET(igr_otpid2, config->otpid2_igr);

    PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_TS_CTRLr(&egr_access, egr_ts_ctrl));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_TS_CTRLr(&igr_access, igr_ts_ctrl));

    PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_TS_CTRL2r(&egr_access, egr_ts_ctrl2));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_TS_CTRL2r(&igr_access, igr_ts_ctrl2));

    PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_PARSER_GEN_CTRLr( &egr_access, egr_ps_gen_ctrl)) ;
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_PARSER_GEN_CTRLr( &igr_access, igr_ps_gen_ctrl)) ;

    PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_PARSER_IVLAN_PIDr (&egr_access, egr_itpid)) ;
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_PARSER_OVLAN_PID0r(&egr_access, egr_otpid)) ;
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_PARSER_OVLAN_PID1r(&egr_access, egr_otpid1));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_PARSER_OVLAN_PID2r(&egr_access, egr_otpid2));

    PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_PARSER_IVLAN_PIDr (&igr_access, igr_itpid)) ;
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_PARSER_OVLAN_PID0r(&igr_access, igr_otpid)) ;
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_PARSER_OVLAN_PID1r(&igr_access, igr_otpid1));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_PARSER_OVLAN_PID2r(&igr_access, igr_otpid2));

    PHYMOD_IF_ERR_RETURN(WRITE_ING_MISC_0_NSE_TS_BLK_ENr(&igr_access, igr_nse_ts_blk_en));

    return PHYMOD_E_NONE;
}

int _plp_aperta2_timesync_config_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_timesync_config_t* config)
{
    P1588_EGR_TS_CTRLr_t  egr_ts_ctrl;
    P1588_ING_TS_CTRLr_t  igr_ts_ctrl;
    P1588_EGR_TS_CTRL2r_t egr_ts_ctrl2;
    P1588_ING_TS_CTRL2r_t igr_ts_ctrl2;

    P1588_EGR_PARSER_GEN_CTRLr_t   egr_ps_gen_ctrl;
    P1588_EGR_PARSER_IVLAN_PIDr_t  egr_itpid ;
    P1588_EGR_PARSER_OVLAN_PID0r_t egr_otpid ;
    P1588_EGR_PARSER_OVLAN_PID1r_t egr_otpid1;
    P1588_EGR_PARSER_OVLAN_PID2r_t egr_otpid2;

    P1588_ING_PARSER_GEN_CTRLr_t   igr_ps_gen_ctrl;
    P1588_ING_PARSER_IVLAN_PIDr_t  igr_itpid ;
    P1588_ING_PARSER_OVLAN_PID0r_t igr_otpid ;
    P1588_ING_PARSER_OVLAN_PID1r_t igr_otpid1;
    P1588_ING_PARSER_OVLAN_PID2r_t igr_otpid2;
    plp_aperta2_phymod_phy_access_t   igr_access, egr_access;

    PHYMOD_MEMCPY(&igr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&egr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));

    PHYMOD_MEMSET(&egr_ts_ctrl,     0, sizeof(P1588_EGR_TS_CTRLr_t));
    PHYMOD_MEMSET(&igr_ts_ctrl,     0, sizeof(P1588_ING_TS_CTRLr_t));

    PHYMOD_MEMSET(&egr_ts_ctrl2,    0, sizeof(P1588_EGR_TS_CTRL2r_t));
    PHYMOD_MEMSET(&igr_ts_ctrl2,    0, sizeof(P1588_ING_TS_CTRL2r_t));

    PHYMOD_MEMSET(&egr_ps_gen_ctrl, 0, sizeof(P1588_EGR_PARSER_GEN_CTRLr_t))  ;
    PHYMOD_MEMSET(&egr_itpid,       0, sizeof(P1588_EGR_PARSER_IVLAN_PIDr_t)) ;
    PHYMOD_MEMSET(&egr_otpid,       0, sizeof(P1588_EGR_PARSER_OVLAN_PID0r_t));
    PHYMOD_MEMSET(&egr_otpid1,      0, sizeof(P1588_EGR_PARSER_OVLAN_PID1r_t));
    PHYMOD_MEMSET(&egr_otpid2,      0, sizeof(P1588_EGR_PARSER_OVLAN_PID2r_t));

    PHYMOD_MEMSET(&igr_ps_gen_ctrl, 0, sizeof(P1588_ING_PARSER_GEN_CTRLr_t))  ;
    PHYMOD_MEMSET(&igr_itpid,       0, sizeof(P1588_ING_PARSER_IVLAN_PIDr_t)) ;
    PHYMOD_MEMSET(&igr_otpid,       0, sizeof(P1588_ING_PARSER_OVLAN_PID0r_t));
    PHYMOD_MEMSET(&igr_otpid1,      0, sizeof(P1588_ING_PARSER_OVLAN_PID1r_t));
    PHYMOD_MEMSET(&igr_otpid2,      0, sizeof(P1588_ING_PARSER_OVLAN_PID2r_t));

#ifdef PHYMOD_APERTA2_SUPPORT
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_get_igr_egr_access(phy, &igr_access, &egr_access));
#endif

    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_TS_CTRLr(&egr_access, &egr_ts_ctrl));
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_TS_CTRLr(&igr_access, &igr_ts_ctrl));

    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_TS_CTRL2r(&egr_access, &egr_ts_ctrl2));
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_TS_CTRL2r(&igr_access, &igr_ts_ctrl2));

    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_PARSER_GEN_CTRLr  (&egr_access, &egr_ps_gen_ctrl));
    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_PARSER_IVLAN_PIDr (&egr_access, &egr_itpid)) ;
    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_PARSER_OVLAN_PID0r(&egr_access, &egr_otpid)) ;
    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_PARSER_OVLAN_PID1r(&egr_access, &egr_otpid1));
    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_PARSER_OVLAN_PID2r(&egr_access, &egr_otpid2));

    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_PARSER_GEN_CTRLr  (&igr_access, &igr_ps_gen_ctrl));
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_PARSER_IVLAN_PIDr (&igr_access, &igr_itpid)) ;
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_PARSER_OVLAN_PID0r(&igr_access, &igr_otpid)) ;
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_PARSER_OVLAN_PID1r(&igr_access, &igr_otpid1));
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_PARSER_OVLAN_PID2r(&igr_access, &igr_otpid2));

    config->tx_mode_tc      = P1588_EGR_TS_CTRLr_TC_ENf_GET(egr_ts_ctrl);
    config->tx_partial_tc   = P1588_EGR_TS_CTRLr_TS_MODEf_GET(egr_ts_ctrl);
    config->tx_inband_clear = P1588_EGR_TS_CTRLr_RSV2_CLEARf_GET(egr_ts_ctrl);

    config->rx_mode_tc      = P1588_ING_TS_CTRLr_TC_ENf_GET(igr_ts_ctrl);
    config->rx_partial_tc   = P1588_ING_TS_CTRLr_TS_MODEf_GET(igr_ts_ctrl);

    if(P1588_EGR_TS_CTRLr_ENCRYPTION_ENf_GET(egr_ts_ctrl)) {
        PHYMOD_TS_F_1588_ENCRYPTED_MODE_SET(config->flags);
    }

    if(P1588_EGR_PARSER_GEN_CTRLr_L2_ENf_GET(egr_ps_gen_ctrl)) {
        PHYMOD_TS_F_L2_ENABLE_SET(config->flags);
    }

    if(P1588_EGR_PARSER_GEN_CTRLr_AS_ENf_GET(egr_ps_gen_ctrl)) {
        PHYMOD_TS_F_8021AS_ENABLE_SET(config->flags);
    }

    if(P1588_EGR_PARSER_GEN_CTRLr_L4_IPV4_UDP_ENf_GET(egr_ps_gen_ctrl)) {
        PHYMOD_TS_F_IP4_ENABLE_SET(config->flags);
    }

    if(P1588_EGR_PARSER_GEN_CTRLr_L4_IPV6_UDP_ENf_GET(egr_ps_gen_ctrl)) {
        PHYMOD_TS_F_IP6_ENABLE_SET(config->flags);
    }

    config->sopmem_intr_enable         = P1588_EGR_TS_CTRLr_INT_MASK_ENf_GET(egr_ts_ctrl);
    config->sopmem_wr_on_err           = P1588_EGR_TS_CTRLr_WR_SOPMEM_ON_ERR_ENf_GET(egr_ts_ctrl);
    config->ip4udp_chksum_disable      = P1588_EGR_TS_CTRLr_IP4UDP_CHKSUM_DISABLEf_GET(egr_ts_ctrl);
    config->vxlan_outer_chksum_disable = P1588_EGR_TS_CTRL2r_VXLAN_OUTER_CHKSUM_FORCE0f_GET(egr_ts_ctrl2);

    config->itpid      = P1588_EGR_PARSER_IVLAN_PIDr_ITPIDf_GET(egr_itpid);
    config->otpid      = P1588_EGR_PARSER_OVLAN_PID0r_OTPIDf_GET(egr_otpid);
    config->otpid1     = P1588_EGR_PARSER_OVLAN_PID1r_OTPID1f_GET(egr_otpid1);
    config->otpid2     = P1588_EGR_PARSER_OVLAN_PID2r_OTPID2f_GET(egr_otpid2);

    config->itpid_igr  = P1588_ING_PARSER_IVLAN_PIDr_ITPIDf_GET(igr_itpid);
    config->otpid_igr  = P1588_ING_PARSER_OVLAN_PID0r_OTPIDf_GET(igr_otpid);
    config->otpid1_igr = P1588_ING_PARSER_OVLAN_PID1r_OTPID1f_GET(igr_otpid1);
    config->otpid2_igr = P1588_ING_PARSER_OVLAN_PID2r_OTPID2f_GET(igr_otpid2);

    return PHYMOD_E_NONE;
}

int _plp_aperta2_timesync_enable_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flag, uint32_t enable)
{
    P1588_EGR_PARSER_RESET_CTRLr_t    egr_ps_reset_ctrl;
    P1588_ING_PARSER_RESET_CTRLr_t    igr_ps_reset_ctrl;
    P1588_EGR_TS_CTRLr_t    egr_ts_ctrl;
    P1588_ING_TS_CTRLr_t    igr_ts_ctrl;
    P1588_EGR_TS_CTRL2r_t   egr_ts_ctrl_2;
    P1588_ING_TS_CTRL2r_t   igr_ts_ctrl_2;
    P1588_COMMON0r_t        nse_ctrl;
    plp_aperta2_phymod_phy_access_t   igr_access, egr_access;

    PHYMOD_MEMCPY(&igr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&egr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMSET(&egr_ps_reset_ctrl, 0, sizeof(P1588_EGR_PARSER_RESET_CTRLr_t));
    PHYMOD_MEMSET(&igr_ps_reset_ctrl, 0, sizeof(P1588_ING_PARSER_RESET_CTRLr_t));
    PHYMOD_MEMSET(&egr_ts_ctrl,       0, sizeof(P1588_EGR_TS_CTRLr_t));
    PHYMOD_MEMSET(&igr_ts_ctrl,       0, sizeof(P1588_ING_TS_CTRLr_t));
    PHYMOD_MEMSET(&egr_ts_ctrl_2,     0, sizeof(P1588_EGR_TS_CTRL2r_t));
    PHYMOD_MEMSET(&igr_ts_ctrl_2,     0, sizeof(P1588_ING_TS_CTRL2r_t));
    PHYMOD_MEMSET(&nse_ctrl,          0, sizeof(P1588_COMMON0r_t));
#ifdef PHYMOD_APERTA2_SUPPORT
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_get_igr_egr_access(phy, &igr_access, &egr_access));
#endif

    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_PARSER_RESET_CTRLr(&egr_access, &egr_ps_reset_ctrl));
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_PARSER_RESET_CTRLr(&igr_access, &igr_ps_reset_ctrl));
    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_TS_CTRLr(&egr_access, &egr_ts_ctrl));
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_TS_CTRLr(&igr_access, &igr_ts_ctrl));

    /* Egress PTP Parser and Time Stamper enable */
    if (PHYMOD_TS_DIRECTION_TX_GET(flag) || (flag == 0)) {
        P1588_EGR_PARSER_RESET_CTRLr_PTP_ENf_SET(egr_ps_reset_ctrl, enable);
        P1588_EGR_TS_CTRLr_PTP_ENf_SET(egr_ts_ctrl, enable);
    }

    /* Ingress PTP Parser and Time Stamper enable */
    if (PHYMOD_TS_DIRECTION_RX_GET(flag) || (flag == 0)) {
        P1588_ING_PARSER_RESET_CTRLr_PTP_ENf_SET(igr_ps_reset_ctrl, enable);
        P1588_ING_TS_CTRLr_PTP_ENf_SET(igr_ts_ctrl, enable);
    }

    /* Soft reset for 1588 timer block only */
    if (PHYMOD_TS_ENABLE_CTRL_SW_RSTB_NSE_GET(flag)) {
        PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON0r(phy, &nse_ctrl));
        P1588_COMMON0r_REGS_RSTBf_SET(nse_ctrl, !enable);
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON0r(phy, nse_ctrl));
    }

    /* Soft reset for 1588 register block   */
    if (PHYMOD_TS_ENABLE_CTRL_SW_RSTB_REG_GET(flag)) {
        P1588_EGR_PARSER_RESET_CTRLr_REG_RSTBf_SET(egr_ps_reset_ctrl, !enable);
        P1588_ING_PARSER_RESET_CTRLr_REG_RSTBf_SET(igr_ps_reset_ctrl, !enable);

        P1588_EGR_TS_CTRLr_SOFT_RSTBf_SET(egr_ts_ctrl, !enable);
        P1588_ING_TS_CTRLr_SOFT_RSTBf_SET(igr_ts_ctrl, !enable);
    }

    /* Soft reset for 1588 block, datapath */
    if (PHYMOD_TS_ENABLE_CTRL_SW_RSTB_GET(flag)) {
        PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_TS_CTRL2r(&egr_access, &egr_ts_ctrl_2));
        PHYMOD_IF_ERR_RETURN(READ_P1588_ING_TS_CTRL2r(&igr_access, &igr_ts_ctrl_2));

        P1588_EGR_PARSER_RESET_CTRLr_SW_RSTBf_SET(egr_ps_reset_ctrl, !enable);
        P1588_ING_PARSER_RESET_CTRLr_SW_RSTBf_SET(igr_ps_reset_ctrl, !enable);

        P1588_EGR_TS_CTRL2r_SW_RSTBf_SET(egr_ts_ctrl_2, !enable);
        P1588_ING_TS_CTRL2r_SW_RSTBf_SET(igr_ts_ctrl_2, !enable);

        PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_TS_CTRL2r(&egr_access, egr_ts_ctrl_2));
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_TS_CTRL2r(&igr_access, igr_ts_ctrl_2));
    }

    PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_PARSER_RESET_CTRLr(&egr_access, egr_ps_reset_ctrl));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_PARSER_RESET_CTRLr(&igr_access, igr_ps_reset_ctrl));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_TS_CTRLr(&egr_access, egr_ts_ctrl));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_TS_CTRLr(&igr_access, igr_ts_ctrl));

    return PHYMOD_E_NONE;
}

int _plp_aperta2_timesync_enable_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flag, uint32_t* enable)
{
    P1588_EGR_TS_CTRLr_t  egr_ts_ctrl;
    P1588_ING_TS_CTRLr_t  igr_ts_ctrl;
    plp_aperta2_phymod_phy_access_t   igr_access, egr_access;

    *enable = 0 ;
    PHYMOD_MEMCPY(&igr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&egr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMSET(&egr_ts_ctrl, 0, sizeof(P1588_EGR_TS_CTRLr_t));
    PHYMOD_MEMSET(&igr_ts_ctrl, 0, sizeof(P1588_ING_TS_CTRLr_t));

#ifdef PHYMOD_APERTA2_SUPPORT
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_get_igr_egr_access(phy, &igr_access, &egr_access));
#endif

    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_TS_CTRLr(&egr_access, &egr_ts_ctrl));
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_TS_CTRLr(&igr_access, &igr_ts_ctrl));

    if (PHYMOD_TS_DIRECTION_TX_GET(flag) || (flag == 0)) {
        *enable = P1588_EGR_TS_CTRLr_PTP_ENf_GET(egr_ts_ctrl);
    } else {
        *enable = P1588_ING_TS_CTRLr_PTP_ENf_GET(igr_ts_ctrl);
    }

    return PHYMOD_E_NONE;
}

int _plp_aperta2_timesync_framesync_mode_set(const plp_aperta2_phymod_phy_access_t* phy, const plp_aperta2_phymod_timesync_framesync_t* framesync)
{
    P1588_COMMON0r_t      nse_ctrl;

    PHYMOD_MEMSET(&nse_ctrl, 0, sizeof(P1588_COMMON0r_t));

    PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON0r(phy, &nse_ctrl));

    /* Enable SYNCREF to compensate PPM error for the phy timer, when sref_mode == 0 (ie, In case of Asynchronous mode) */
    P1588_COMMON0r_SREF_ENAf_SET(nse_ctrl, !P1588_COMMON0r_SREF_MODEf_GET(nse_ctrl));

    /* Enable PPS to load TOD into the phy timer */
    P1588_COMMON0r_PPS_ENAf_SET(nse_ctrl, ((framesync->flags >> 14) & 0x1));

    /* Clear pps_ena after TOD load into the phy timer */
    P1588_COMMON0r_PPS_AUTOOFFf_SET(nse_ctrl, ((framesync->flags >> 15) & 0x1));

    PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON0r(phy, nse_ctrl));

    return PHYMOD_E_NONE;
}

int _plp_aperta2_timesync_framesync_mode_get(const plp_aperta2_phymod_phy_access_t* phy, plp_aperta2_phymod_timesync_framesync_t* framesync)
{
    P1588_COMMON0r_t      nse_ctrl;

    PHYMOD_MEMSET(&nse_ctrl, 0, sizeof(P1588_COMMON0r_t));

    PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON0r(phy, &nse_ctrl));

    framesync->flags |= (P1588_COMMON0r_PPS_ENAf_GET(nse_ctrl) << 14);
    framesync->flags |= (P1588_COMMON0r_PPS_AUTOOFFf_GET(nse_ctrl) << 15);

    return PHYMOD_E_NONE;
}

int _plp_aperta2_timesync_timestamp_offset_set(const plp_aperta2_phymod_phy_access_t* phy, int txrx, uint32_t op, uint32_t msg_type, uint32_t ts_offset)
{
    P1588_EGR_TS_LINK_ASYNMETRY_DELAY_LOWr_t  egr_asym_delay_lo;
    P1588_EGR_TS_LINK_ASYNMETRY_DELAY_HIGHr_t egr_asym_delay_hi;
    P1588_ING_TS_LINK_ASYNMETRY_DELAY_LOWr_t  igr_asym_delay_lo;
    P1588_ING_TS_LINK_ASYNMETRY_DELAY_HIGHr_t igr_asym_delay_hi;
    P1588_EGR_TS_CTRLr_t  egr_ts_ctrl;
    P1588_ING_TS_CTRLr_t  igr_ts_ctrl;
    plp_aperta2_phymod_phy_access_t   igr_access, egr_access;

    PHYMOD_MEMCPY(&igr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&egr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMSET(&egr_asym_delay_lo, 0, sizeof(P1588_EGR_TS_LINK_ASYNMETRY_DELAY_LOWr_t));
    PHYMOD_MEMSET(&egr_asym_delay_hi, 0, sizeof(P1588_EGR_TS_LINK_ASYNMETRY_DELAY_HIGHr_t));
    PHYMOD_MEMSET(&igr_asym_delay_lo, 0, sizeof(P1588_ING_TS_LINK_ASYNMETRY_DELAY_LOWr_t));
    PHYMOD_MEMSET(&igr_asym_delay_hi, 0, sizeof(P1588_ING_TS_LINK_ASYNMETRY_DELAY_HIGHr_t));
    PHYMOD_MEMSET(&egr_ts_ctrl,       0, sizeof(P1588_EGR_TS_CTRLr_t));
    PHYMOD_MEMSET(&igr_ts_ctrl,       0, sizeof(P1588_ING_TS_CTRLr_t));

#ifdef PHYMOD_APERTA2_SUPPORT
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_get_igr_egr_access(phy, &igr_access, &egr_access));
#endif

    if((op == TIMESYNC_OPERATOR_SUB) && (ts_offset > 0x80000000)) {
        PHYMOD_DEBUG_ERROR(("ERROR: Negative ts_offset range exceeded. It should be less than or equal 0x80000000"));
        return PHYMOD_E_PARAM;
    }
    if((op != TIMESYNC_OPERATOR_SUB) && (ts_offset > 0x7FFFFFFF)) {
        PHYMOD_DEBUG_ERROR(("ERROR: Positive ts_offset range exceeded. It should be less than or equal 0x7FFFFFFF"));
        return PHYMOD_E_PARAM;
    }

    /* Making sure ts_offset is in 2s complement format */
    if(op == TIMESYNC_OPERATOR_SUB) {
        ts_offset = -ts_offset ;
    }
    /* Be aligned with TS_offset bits in register */
    msg_type >>= TS_OFFSET_CTRL_SHIFT;

    /* Egress */
    if ( PHYMOD_TS_DIRECTION_TX_GET(txrx) || (txrx == 0) ) {

        PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_TS_CTRLr(&egr_access, &egr_ts_ctrl));
        PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_TS_LINK_ASYNMETRY_DELAY_LOWr(&egr_access,  &egr_asym_delay_lo));
        PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_TS_LINK_ASYNMETRY_DELAY_HIGHr(&egr_access, &egr_asym_delay_hi));

        egr_ts_ctrl.v[0] &=  (~TS_OFFSET_MSG_TYPE_MASK);
        egr_ts_ctrl.v[0] |=  ( TS_OFFSET_MSG_TYPE_MASK & msg_type);

        P1588_EGR_TS_LINK_ASYNMETRY_DELAY_LOWr_SET(egr_asym_delay_lo, (ts_offset & BIT_15_00_MASK));
        P1588_EGR_TS_LINK_ASYNMETRY_DELAY_HIGHr_SET(egr_asym_delay_hi, ((ts_offset >> 16) & BIT_15_00_MASK));

        PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_TS_CTRLr(&egr_access, egr_ts_ctrl));
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_TS_LINK_ASYNMETRY_DELAY_LOWr(&egr_access,  egr_asym_delay_lo));
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_TS_LINK_ASYNMETRY_DELAY_HIGHr(&egr_access, egr_asym_delay_hi));
    }

    /* Ingress */
    if ( PHYMOD_TS_DIRECTION_RX_GET(txrx) || (txrx == 0) ) {

        PHYMOD_IF_ERR_RETURN(READ_P1588_ING_TS_CTRLr(&igr_access, &igr_ts_ctrl));
        PHYMOD_IF_ERR_RETURN(READ_P1588_ING_TS_LINK_ASYNMETRY_DELAY_LOWr(&igr_access,  &igr_asym_delay_lo));
        PHYMOD_IF_ERR_RETURN(READ_P1588_ING_TS_LINK_ASYNMETRY_DELAY_HIGHr(&igr_access, &igr_asym_delay_hi));

        igr_ts_ctrl.v[0] &=  (~TS_OFFSET_MSG_TYPE_MASK);
        igr_ts_ctrl.v[0] |=  ( TS_OFFSET_MSG_TYPE_MASK & msg_type);

        P1588_ING_TS_LINK_ASYNMETRY_DELAY_LOWr_SET(igr_asym_delay_lo, (ts_offset & BIT_15_00_MASK));
        P1588_ING_TS_LINK_ASYNMETRY_DELAY_HIGHr_SET(igr_asym_delay_hi, ((ts_offset >> 16) & BIT_15_00_MASK));

        PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_TS_CTRLr(&igr_access, igr_ts_ctrl));
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_TS_LINK_ASYNMETRY_DELAY_LOWr(&igr_access, igr_asym_delay_lo));
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_TS_LINK_ASYNMETRY_DELAY_HIGHr(&igr_access, igr_asym_delay_hi));
    }
    return PHYMOD_E_NONE;
}

int _plp_aperta2_timesync_timestamp_offset_get(const plp_aperta2_phymod_phy_access_t* phy, int txrx, uint32_t *op, uint32_t *msg_type, uint32_t *ts_offset)
{
    P1588_EGR_TS_LINK_ASYNMETRY_DELAY_LOWr_t  egr_asym_delay_lo;
    P1588_EGR_TS_LINK_ASYNMETRY_DELAY_HIGHr_t egr_asym_delay_hi;
    P1588_ING_TS_LINK_ASYNMETRY_DELAY_LOWr_t  igr_asym_delay_lo;
    P1588_ING_TS_LINK_ASYNMETRY_DELAY_HIGHr_t igr_asym_delay_hi;
    P1588_EGR_TS_CTRLr_t  egr_ts_ctrl;
    P1588_ING_TS_CTRLr_t  igr_ts_ctrl;
    plp_aperta2_phymod_phy_access_t   igr_access, egr_access;

    PHYMOD_MEMCPY(&igr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&egr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));

    PHYMOD_MEMSET(&egr_asym_delay_lo, 0, sizeof(P1588_EGR_TS_LINK_ASYNMETRY_DELAY_LOWr_t));
    PHYMOD_MEMSET(&egr_asym_delay_hi, 0, sizeof(P1588_EGR_TS_LINK_ASYNMETRY_DELAY_HIGHr_t));
    PHYMOD_MEMSET(&igr_asym_delay_lo, 0, sizeof(P1588_ING_TS_LINK_ASYNMETRY_DELAY_LOWr_t));
    PHYMOD_MEMSET(&igr_asym_delay_hi, 0, sizeof(P1588_ING_TS_LINK_ASYNMETRY_DELAY_HIGHr_t));
    PHYMOD_MEMSET(&egr_ts_ctrl,       0, sizeof(P1588_EGR_TS_CTRLr_t));
    PHYMOD_MEMSET(&igr_ts_ctrl,       0, sizeof(P1588_ING_TS_CTRLr_t));

    *op = 0;
#ifdef PHYMOD_APERTA2_SUPPORT
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_get_igr_egr_access(phy, &igr_access, &egr_access));
#endif

    /* Egress */
    if (PHYMOD_TS_DIRECTION_TX_GET(txrx) || (txrx == 0) ) {
        PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_TS_CTRLr(&egr_access, &egr_ts_ctrl));
        PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_TS_LINK_ASYNMETRY_DELAY_LOWr(&egr_access,  &egr_asym_delay_lo));
        PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_TS_LINK_ASYNMETRY_DELAY_HIGHr(&egr_access, &egr_asym_delay_hi));
       *msg_type  = ((egr_ts_ctrl.v[0] & TS_OFFSET_MSG_TYPE_MASK) << TS_OFFSET_CTRL_SHIFT);
       *ts_offset = (((egr_asym_delay_hi.v[0] & BIT_15_00_MASK) << 16 ) | (egr_asym_delay_lo.v[0] & BIT_15_00_MASK));

        /* Incase of negative number perform 2's complement */
        if(*ts_offset & (1<<31)) {
            *op = TIMESYNC_OPERATOR_SUB ;
            *ts_offset = -(*ts_offset);
        }
    }

    /* Ingress */
    if (PHYMOD_TS_DIRECTION_RX_GET(txrx) || (txrx == 0) ) {
        PHYMOD_IF_ERR_RETURN(READ_P1588_ING_TS_CTRLr(&igr_access, &igr_ts_ctrl));
        PHYMOD_IF_ERR_RETURN(READ_P1588_ING_TS_LINK_ASYNMETRY_DELAY_LOWr(&igr_access,  &igr_asym_delay_lo));
        PHYMOD_IF_ERR_RETURN(READ_P1588_ING_TS_LINK_ASYNMETRY_DELAY_HIGHr(&igr_access, &igr_asym_delay_hi));
        *msg_type  = ((igr_ts_ctrl.v[0] & TS_OFFSET_MSG_TYPE_MASK) << TS_OFFSET_CTRL_SHIFT);
        *ts_offset = (((igr_asym_delay_hi.v[0] & BIT_15_00_MASK) << 16 ) | (igr_asym_delay_lo.v[0] & BIT_15_00_MASK));

        /* Incase of negative number perform 2's complement */
        if(*ts_offset & (1<<31)) {
            *op = TIMESYNC_OPERATOR_SUB ;
            *ts_offset = -(*ts_offset);
        }
    }
    return PHYMOD_E_NONE;
}

int _plp_aperta2_timesync_time_code_set(const plp_aperta2_phymod_phy_access_t  *phy, uint32_t flags, const plp_aperta2_phymod_timesync_timespec_t *timecode)
{
    P1588_COMMON0r_t  nse_ctrl;

    P1588_COMMON1r_t pps_t48_15to0_ns  ;
    P1588_COMMON2r_t pps_t48_31to16_ns ;
    P1588_COMMON3r_t pps_t48_47to32_ns ;

    P1588_COMMON4r_t pps_t80_15to0_ieee_ns   ;
    P1588_COMMON5r_t pps_t80_31to16_ieee_ns  ;
    P1588_COMMON6r_t pps_t80_47to32_ieee_sec ;
    P1588_COMMON7r_t pps_t80_63to48_ieee_sec ;
    P1588_COMMON8r_t pps_t80_79to64_ieee_sec ;

    P1588_COMMON9r_t  sref_t48_15to0_ns  ;
    P1588_COMMON10r_t sref_t48_31to16_ns ;
    P1588_COMMON11r_t sref_t48_47to32_ns ;

    uint64_t ns_64  = 0;
    uint32_t ns_32  = 0;
    uint32_t sec_32 = 0;
    uint64_t sref_t = 0;
    uint32_t sref_t_32 = 0;

    (void)flags ;

    PHYMOD_MEMSET(&nse_ctrl,                0, sizeof(P1588_COMMON0r_t));

    PHYMOD_MEMSET(&pps_t48_15to0_ns,        0, sizeof(P1588_COMMON1r_t));
    PHYMOD_MEMSET(&pps_t48_31to16_ns,       0, sizeof(P1588_COMMON2r_t));
    PHYMOD_MEMSET(&pps_t48_47to32_ns,       0, sizeof(P1588_COMMON3r_t));

    PHYMOD_MEMSET(&pps_t80_15to0_ieee_ns,   0, sizeof(P1588_COMMON4r_t));
    PHYMOD_MEMSET(&pps_t80_31to16_ieee_ns,  0, sizeof(P1588_COMMON5r_t));
    PHYMOD_MEMSET(&pps_t80_47to32_ieee_sec, 0, sizeof(P1588_COMMON6r_t));
    PHYMOD_MEMSET(&pps_t80_63to48_ieee_sec, 0, sizeof(P1588_COMMON7r_t));
    PHYMOD_MEMSET(&pps_t80_79to64_ieee_sec, 0, sizeof(P1588_COMMON8r_t));

    PHYMOD_MEMSET(&sref_t48_15to0_ns,       0, sizeof(P1588_COMMON9r_t));
    PHYMOD_MEMSET(&sref_t48_31to16_ns,      0, sizeof(P1588_COMMON10r_t));
    PHYMOD_MEMSET(&sref_t48_47to32_ns,      0, sizeof(P1588_COMMON11r_t));

    /* Make sure 48bit seconds max range is 0xFFFFFFFFFFFF and 32bit nanosecond max range is 999999999 */
    if((timecode->seconds > 0xFFFFFFFFFFFF) || (timecode->nanoseconds > 999999999)) {
        PHYMOD_DEBUG_ERROR(("ERROR: Provided 48bit ns or 48bit second input is more than 48bit."));
        return PHYMOD_E_PARAM;
    }
    /* 64-bit nanosecond conversion from 48bit Second+32bit nanosecond */
    ns_64 = timecode->seconds * (uint64_t)1000000000 + (uint64_t)timecode->nanoseconds ;

    /* Set 48-bit nanosecond TOD */
    ns_32 = COMPILER_64_LO(ns_64);
    P1588_COMMON1r_PPS_T48_15TO0f_SET(pps_t48_15to0_ns, (ns_32 & BIT_15_00_MASK));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON1r(phy, pps_t48_15to0_ns));
    P1588_COMMON2r_PPS_T48_31TO16f_SET(pps_t48_31to16_ns, ((ns_32 & BIT_31_16_MASK) >> 16));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON2r(phy, pps_t48_31to16_ns));

    ns_32 = COMPILER_64_HI(ns_64);
    P1588_COMMON3r_PPS_T48_47TO32f_SET(pps_t48_47to32_ns, (ns_32 & BIT_15_00_MASK));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON3r(phy, pps_t48_47to32_ns));

    /* Set 48bit Second + 32-bit nanosecond TOD */
    P1588_COMMON4r_PPS_T80_15TO0f_SET(pps_t80_15to0_ieee_ns, (timecode->nanoseconds & BIT_15_00_MASK));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON4r(phy, pps_t80_15to0_ieee_ns) );
    P1588_COMMON5r_PPS_T80_31TO16f_SET(pps_t80_31to16_ieee_ns, ((timecode->nanoseconds >> 16) & BIT_15_00_MASK));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON5r(phy, pps_t80_31to16_ieee_ns));

    sec_32 = COMPILER_64_LO(timecode->seconds);
    P1588_COMMON6r_PPS_T80_47TO32f_SET(pps_t80_47to32_ieee_sec, (sec_32 & BIT_15_00_MASK));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON6r(phy, pps_t80_47to32_ieee_sec));
    P1588_COMMON7r_PPS_T80_63TO48f_SET(pps_t80_63to48_ieee_sec, ((sec_32 & BIT_31_16_MASK) >> 16));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON7r(phy, pps_t80_63to48_ieee_sec));

    sec_32 = COMPILER_64_HI(timecode->seconds);
    P1588_COMMON8r_PPS_T80_79TO64f_SET(pps_t80_79to64_ieee_sec, (sec_32 & BIT_15_00_MASK));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON8r(phy, pps_t80_79to64_ieee_sec));

    /* SREF settings in case of Asynchronous mode */
    PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON0r(phy, &nse_ctrl));

    if(! P1588_COMMON0r_SREF_MODEf_GET(nse_ctrl)) {
        if(timecode->syncref_delay.enable) {
            sref_t = (timecode->syncref_delay.value > 2) ? (ns_64 + timecode->syncref_delay.value) : (ns_64 + 1000);
        } else {
            sref_t = ns_64 ;
        }
    }

    sref_t_32 = COMPILER_64_LO(sref_t);
    P1588_COMMON9r_SREF_T48_15TO0f_SET(sref_t48_15to0_ns, (sref_t_32 & BIT_15_00_MASK));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON9r(phy, sref_t48_15to0_ns));
    P1588_COMMON10r_SREF_T48_31TO16f_SET(sref_t48_31to16_ns, ((sref_t_32 & BIT_31_16_MASK) >> 16));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON10r(phy, sref_t48_31to16_ns));

    sref_t_32 = COMPILER_64_HI(sref_t);
    P1588_COMMON11r_SREF_T48_47TO32f_SET(sref_t48_47to32_ns, (sref_t_32 & BIT_15_00_MASK));
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON11r(phy, sref_t48_47to32_ns));

    return PHYMOD_E_NONE;
}

int _plp_aperta2_timesync_time_code_get(const plp_aperta2_phymod_phy_access_t  *phy, uint32_t flags, plp_aperta2_phymod_timesync_timespec_t *timecode)
{
    P1588_COMMON0r_t  nse_ctrl;
    P1588_COMMON15r_t snap_tphy_15to0;
    P1588_COMMON16r_t snap_tphy_31to16;
    P1588_COMMON17r_t snap_tphy_47to32;
    P1588_COMMON18r_t  snap_tphy_plsb;

    P1588_COMMON1r_t pps_t48_15to0_ns  ;
    P1588_COMMON2r_t pps_t48_31to16_ns ;
    P1588_COMMON3r_t pps_t48_47to32_ns ;

    P1588_COMMON4r_t pps_t80_15to0_ieee_ns   ;
    P1588_COMMON5r_t pps_t80_31to16_ieee_ns  ;
    P1588_COMMON6r_t pps_t80_47to32_ieee_sec ;
    P1588_COMMON7r_t pps_t80_63to48_ieee_sec ;
    P1588_COMMON8r_t pps_t80_79to64_ieee_sec ;

    P1588_COMMON9r_t  sref_t48_15to0_ns  ;
    P1588_COMMON10r_t sref_t48_31to16_ns ;
    P1588_COMMON11r_t sref_t48_47to32_ns ;

    PHYMOD_MEMSET(&nse_ctrl,         0, sizeof(P1588_COMMON0r_t));

    PHYMOD_MEMSET(&snap_tphy_15to0,  0, sizeof(P1588_COMMON15r_t));
    PHYMOD_MEMSET(&snap_tphy_31to16, 0, sizeof(P1588_COMMON16r_t));
    PHYMOD_MEMSET(&snap_tphy_47to32, 0, sizeof(P1588_COMMON17r_t));
    PHYMOD_MEMSET(&snap_tphy_plsb,   0, sizeof(P1588_COMMON18r_t));

    PHYMOD_MEMSET(&pps_t48_15to0_ns,        0, sizeof(P1588_COMMON1r_t));
    PHYMOD_MEMSET(&pps_t48_31to16_ns,       0, sizeof(P1588_COMMON2r_t));
    PHYMOD_MEMSET(&pps_t48_47to32_ns,       0, sizeof(P1588_COMMON3r_t));

    PHYMOD_MEMSET(&pps_t80_15to0_ieee_ns,   0, sizeof(P1588_COMMON4r_t));
    PHYMOD_MEMSET(&pps_t80_31to16_ieee_ns,  0, sizeof(P1588_COMMON5r_t));
    PHYMOD_MEMSET(&pps_t80_47to32_ieee_sec, 0, sizeof(P1588_COMMON6r_t));
    PHYMOD_MEMSET(&pps_t80_63to48_ieee_sec, 0, sizeof(P1588_COMMON7r_t));
    PHYMOD_MEMSET(&pps_t80_79to64_ieee_sec, 0, sizeof(P1588_COMMON8r_t));

    PHYMOD_MEMSET(&sref_t48_15to0_ns,       0, sizeof(P1588_COMMON9r_t));
    PHYMOD_MEMSET(&sref_t48_31to16_ns,      0, sizeof(P1588_COMMON10r_t));
    PHYMOD_MEMSET(&sref_t48_47to32_ns,      0, sizeof(P1588_COMMON11r_t));

    PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON15r(phy, &snap_tphy_15to0));
    PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON16r(phy, &snap_tphy_31to16));
    PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON17r(phy, &snap_tphy_47to32));
    PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON18r(phy, &snap_tphy_plsb));

    if(flags == phymodTimesyncTimeCodeHeartBeat48bit) {
        PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON0r(phy, &nse_ctrl));
        P1588_COMMON0r_PPS_SNAPf_SET(nse_ctrl, 1);
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON0r(phy, nse_ctrl));
        COMPILER_64_SET( timecode->nanoseconds64,
                        ( P1588_COMMON17r_SNAP_TPHY_47TO32f_GET(snap_tphy_47to32)),
                        ((P1588_COMMON16r_SNAP_TPHY_31TO16f_GET(snap_tphy_31to16) << 16) |
                         (P1588_COMMON15r_SNAP_TPHY_15TO0f_GET(snap_tphy_15to0))
                        )
                       );
        /* precision LSB Handling*/
        COMPILER_64_ADD_32(timecode->nanoseconds64, PLP_P1588_COMMON18r_SNAP_TPHY_PLSBf_GET(snap_tphy_plsb));

    } else if(flags == phymodTimesyncTimeCodeHeartBeat48bitIeee) {
        PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON0r(phy, &nse_ctrl));
        P1588_COMMON0r_PPS_SNAPf_SET(nse_ctrl, 2);
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON0r(phy, nse_ctrl));
        timecode->nanoseconds = ((P1588_COMMON16r_SNAP_TPHY_31TO16f_GET(snap_tphy_31to16) << 16) |
                                  P1588_COMMON15r_SNAP_TPHY_15TO0f_GET(snap_tphy_15to0));
        COMPILER_64_SET(timecode->seconds, 0, P1588_COMMON17r_SNAP_TPHY_47TO32f_GET(snap_tphy_47to32));

        /* precision LSB Handling*/
        if ((timecode->nanoseconds + PLP_P1588_COMMON18r_SNAP_TPHY_PLSBf_GET(snap_tphy_plsb)) > TS_ONE_SEC_IN_NS) {
            COMPILER_64_ADD_32(timecode->seconds, 1);
            timecode->nanoseconds = ((timecode->nanoseconds + PLP_P1588_COMMON18r_SNAP_TPHY_PLSBf_GET(snap_tphy_plsb)) - TS_ONE_SEC_IN_NS) ;
        } else {
            timecode->nanoseconds += PLP_P1588_COMMON18r_SNAP_TPHY_PLSBf_GET(snap_tphy_plsb);
        }

    } else if((flags == phymodTimesyncTimeCode48bit) || (flags == phymodTimesyncTimeCode80bit)) {
        /* Get 48bit Second + 32bit nanosecond value */
        PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON4r(phy, &pps_t80_15to0_ieee_ns));
        PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON5r(phy, &pps_t80_31to16_ieee_ns));
        timecode->nanoseconds = ((P1588_COMMON5r_PPS_T80_31TO16f_GET(pps_t80_31to16_ieee_ns) << 16) |
                                  P1588_COMMON4r_PPS_T80_15TO0f_GET(pps_t80_15to0_ieee_ns));

        PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON6r(phy, &pps_t80_47to32_ieee_sec));
        PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON7r(phy, &pps_t80_63to48_ieee_sec));
        PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON8r(phy, &pps_t80_79to64_ieee_sec));
        COMPILER_64_SET(timecode->seconds,
                        ( P1588_COMMON8r_PPS_T80_79TO64f_GET(pps_t80_79to64_ieee_sec)),
                        ((P1588_COMMON7r_PPS_T80_63TO48f_GET(pps_t80_63to48_ieee_sec) << 16) |
                         (P1588_COMMON6r_PPS_T80_47TO32f_GET(pps_t80_47to32_ieee_sec))
                        )
                       );

        /* Get 48bit nanosecond value */
        PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON1r(phy, &pps_t48_15to0_ns) );
        PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON2r(phy, &pps_t48_31to16_ns) );
        PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON3r(phy, &pps_t48_47to32_ns) );
        COMPILER_64_SET( timecode->nanoseconds64,
                         ( P1588_COMMON3r_PPS_T48_47TO32f_GET(pps_t48_47to32_ns)),
                         ((P1588_COMMON2r_PPS_T48_31TO16f_GET(pps_t48_31to16_ns) << 16) |
                          (P1588_COMMON1r_PPS_T48_15TO0f_GET(pps_t48_15to0_ns))
                         )
                       );

        /* Get sref settings */
        PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON9r(phy,  &sref_t48_15to0_ns ) );
        PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON10r(phy, &sref_t48_31to16_ns) );
        PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON11r(phy, &sref_t48_47to32_ns) );
        COMPILER_64_SET( timecode->syncref_delay.value,
                             ( P1588_COMMON11r_SREF_T48_47TO32f_GET(sref_t48_47to32_ns)),
                             ((P1588_COMMON10r_SREF_T48_31TO16f_GET(sref_t48_31to16_ns) << 16) |
                              (P1588_COMMON9r_SREF_T48_15TO0f_GET(sref_t48_15to0_ns))
                             )
                           );
    } else {
        PHYMOD_IF_ERR_RETURN(READ_P1588_COMMON0r(phy, &nse_ctrl));
        P1588_COMMON0r_PPS_SNAPf_SET(nse_ctrl, 0);
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_COMMON0r(phy, nse_ctrl));
    }
    return PHYMOD_E_NONE;
}

int _plp_aperta2_timesync_mpls_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t direction, const plp_aperta2_phymod_timesync_mpls_ctrl_t *config)
{
    P1588_EGR_PARSER_MPLS_LABEL_ENr_t egr_ps_mpls_label_en;
    P1588_ING_PARSER_MPLS_LABEL_ENr_t igr_ps_mpls_label_en;
    P1588_EGR_PARSER_GEN_CTRLr_t      egr_ps_gen_ctrl;
    P1588_ING_PARSER_GEN_CTRLr_t      igr_ps_gen_ctrl;
    uint32_t egr_mpls_label_en     = 0 ;
    uint32_t igr_mpls_label_en     = 0 ;
    uint64_t egr_mpls_label_msb    = 0 ;
    uint64_t igr_mpls_label_msb    = 0 ;
    uint64_t egr_mpls_mask_msb     = 0 ;
    uint64_t igr_mpls_mask_msb     = 0 ;
    uint32_t egr_mpls_mask_msb_u32 = 0 ;
    uint32_t igr_mpls_mask_msb_u32 = 0 ;
    uint32_t egr_mpls_label_mask_msb[5] = {0};
    uint32_t igr_mpls_label_mask_msb[5] = {0};
    uint32_t mpls_en = TRUE;
    int idx = 0;
    plp_aperta2_phymod_phy_access_t   igr_access, egr_access;

    PHYMOD_MEMCPY(&igr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&egr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMSET(&egr_ps_mpls_label_en, 0, sizeof(P1588_EGR_PARSER_MPLS_LABEL_ENr_t)) ;
    PHYMOD_MEMSET(&igr_ps_mpls_label_en, 0, sizeof(P1588_ING_PARSER_MPLS_LABEL_ENr_t)) ;
    PHYMOD_MEMSET(&egr_ps_gen_ctrl,      0, sizeof(P1588_EGR_PARSER_GEN_CTRLr_t)) ;
    PHYMOD_MEMSET(&igr_ps_gen_ctrl,      0, sizeof(P1588_ING_PARSER_GEN_CTRLr_t)) ;

    if ( (config->size < 0) || (config->size > TS_MPLS_LABEL_COUNT) ) {
        return PHYMOD_E_PARAM;
    }
#ifdef PHYMOD_APERTA2_SUPPORT
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_get_igr_egr_access(phy, &igr_access, &egr_access));
#endif

    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_PARSER_MPLS_LABEL_ENr(&egr_access, &egr_ps_mpls_label_en));
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_PARSER_MPLS_LABEL_ENr(&igr_access, &igr_ps_mpls_label_en));
    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_PARSER_GEN_CTRLr(&egr_access, &egr_ps_gen_ctrl)) ;
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_PARSER_GEN_CTRLr(&igr_access, &igr_ps_gen_ctrl)) ;

    mpls_en = PHYMOD_TS_MPLS_F_ENABLE_GET(config->flags);

    /* MPLS control */
    if ( IS_TS_MPLS_DIRECTION_RX(direction) ) {
        P1588_ING_PARSER_GEN_CTRLr_MPLS_ENf_SET(igr_ps_gen_ctrl, mpls_en);
    }
    if ( IS_TS_MPLS_DIRECTION_TX(direction) ) {
        P1588_EGR_PARSER_GEN_CTRLr_MPLS_ENf_SET(egr_ps_gen_ctrl, mpls_en);
    }

    if ( PHYMOD_TS_MPLS_F_CONTROL_WORD_ENABLE_GET(config->flags) ) {
        if ( IS_TS_MPLS_DIRECTION_RX(direction) ) {
            P1588_ING_PARSER_GEN_CTRLr_MPLS_CONTROL_WORD_ENf_SET(igr_ps_gen_ctrl, mpls_en);
        }
        if ( IS_TS_MPLS_DIRECTION_TX(direction) ) {
            P1588_EGR_PARSER_GEN_CTRLr_MPLS_CONTROL_WORD_ENf_SET(egr_ps_gen_ctrl, mpls_en);
        }
    }
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_PARSER_GEN_CTRLr( &igr_access, igr_ps_gen_ctrl)) ;
    PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_PARSER_GEN_CTRLr( &egr_access, egr_ps_gen_ctrl)) ;

    for(idx = 0; idx < config->size; idx++) {
        if ( IS_TS_MPLS_DIRECTION_RX(direction) && mpls_en ) {
            igr_mpls_label_en |= (config->labels[idx].flags << idx);
            PHYMOD_IF_ERR_RETURN( plp_aperta2_p1588_reg_write(&igr_access, (TS_MPLS_IGR_LABEL0_ADDR + idx), (config->labels[idx].value & 0xFFFF)) );
            PHYMOD_IF_ERR_RETURN( plp_aperta2_p1588_reg_write(&igr_access, (TS_MPLS_IGR_MASK0_ADDR  + idx), (config->labels[idx].mask  & 0xFFFF)) );
            igr_mpls_label_msb |= ((uint64_t)((config->labels[idx].value >> 16) & 0xF) << (idx << 2));
            igr_mpls_mask_msb  |= ((uint64_t)((config->labels[idx].mask  >> 16) & 0xF) << (idx << 2));
        }
        if ( IS_TS_MPLS_DIRECTION_TX(direction) && mpls_en ) {
            egr_mpls_label_en |= (config->labels[idx].flags << idx);
            PHYMOD_IF_ERR_RETURN( plp_aperta2_p1588_reg_write(&egr_access, (TS_MPLS_EGR_LABEL0_ADDR + idx), (config->labels[idx].value & 0xFFFF)) );
            PHYMOD_IF_ERR_RETURN( plp_aperta2_p1588_reg_write(&egr_access, (TS_MPLS_EGR_MASK0_ADDR  + idx), (config->labels[idx].mask  & 0xFFFF)) );
            egr_mpls_label_msb |= ((uint64_t)((config->labels[idx].value >> 16) & 0xF) << (idx << 2));
            egr_mpls_mask_msb  |= ((uint64_t)((config->labels[idx].mask  >> 16) & 0xF) << (idx << 2));
        }
    }

    if ( IS_TS_MPLS_DIRECTION_RX(direction) && mpls_en ) {
        igr_mpls_mask_msb_u32      = COMPILER_64_LO(igr_mpls_mask_msb);
        igr_mpls_mask_msb          = ((igr_mpls_mask_msb >> 32) << 40);
        igr_mpls_label_msb        |= igr_mpls_mask_msb ;
        igr_mpls_label_mask_msb[0] = ( igr_mpls_label_msb          & 0xFFFF);
        igr_mpls_label_mask_msb[1] = ((igr_mpls_label_msb >>16)    & 0xFFFF);
        igr_mpls_label_mask_msb[2] = ((igr_mpls_label_msb >>32)    & 0xFFFF);
        igr_mpls_label_mask_msb[3] = ( igr_mpls_mask_msb_u32       & 0xFFFF);
        igr_mpls_label_mask_msb[4] = ((igr_mpls_mask_msb_u32 >>16) & 0xFFFF);

        for(idx = 0; idx < 5; idx++) {
            PHYMOD_IF_ERR_RETURN( plp_aperta2_p1588_reg_write(&igr_access, (TS_MPLS_IGR_LABEL_MASK_MSB_ADDR + idx), igr_mpls_label_mask_msb[idx]));
        }
        P1588_ING_PARSER_MPLS_LABEL_ENr_MPLS_PTP_LABEL_ENf_SET(igr_ps_mpls_label_en, igr_mpls_label_en);
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_PARSER_MPLS_LABEL_ENr(&igr_access, igr_ps_mpls_label_en));
    }

    if ( IS_TS_MPLS_DIRECTION_TX(direction) && mpls_en ) {
        egr_mpls_mask_msb_u32      = COMPILER_64_LO(egr_mpls_mask_msb);
        egr_mpls_mask_msb          = ((egr_mpls_mask_msb >> 32) << 40);
        egr_mpls_label_msb        |= egr_mpls_mask_msb ;
        egr_mpls_label_mask_msb[0] = ( egr_mpls_label_msb          & 0xFFFF);
        egr_mpls_label_mask_msb[1] = ((egr_mpls_label_msb >>16)    & 0xFFFF);
        egr_mpls_label_mask_msb[2] = ((egr_mpls_label_msb >>32)    & 0xFFFF);
        egr_mpls_label_mask_msb[3] = ( egr_mpls_mask_msb_u32       & 0xFFFF);
        egr_mpls_label_mask_msb[4] = ((egr_mpls_mask_msb_u32 >>16) & 0xFFFF);

        for(idx = 0; idx < 5; idx++) {
            PHYMOD_IF_ERR_RETURN( plp_aperta2_p1588_reg_write(&egr_access, (TS_MPLS_EGR_LABEL_MASK_MSB_ADDR + idx), egr_mpls_label_mask_msb[idx]));
        }
        P1588_EGR_PARSER_MPLS_LABEL_ENr_MPLS_PTP_LABEL_ENf_SET(egr_ps_mpls_label_en, egr_mpls_label_en);
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_PARSER_MPLS_LABEL_ENr(&egr_access, egr_ps_mpls_label_en));
    }

    return PHYMOD_E_NONE;
}

int _plp_aperta2_timesync_mpls_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t direction, plp_aperta2_phymod_timesync_mpls_ctrl_t *config)
{
    P1588_EGR_PARSER_MPLS_LABEL_ENr_t egr_ps_mpls_label_en;
    P1588_ING_PARSER_MPLS_LABEL_ENr_t igr_ps_mpls_label_en;
    P1588_EGR_PARSER_GEN_CTRLr_t egr_ps_gen_ctrl;
    P1588_ING_PARSER_GEN_CTRLr_t igr_ps_gen_ctrl;
    uint32_t egr_mpls_label_en   = 0 ;
    uint32_t igr_mpls_label_en   = 0 ;
    uint64_t egr_mpls_label_msb  = 0 ;
    uint64_t igr_mpls_label_msb  = 0 ;
    uint64_t egr_mpls_mask_msb   = 0 ;
    uint64_t igr_mpls_mask_msb   = 0 ;
    uint32_t egr_mpls_label_mask_msb[5] = {0};
    uint32_t igr_mpls_label_mask_msb[5] = {0};
    int idx = 0;
    plp_aperta2_phymod_phy_access_t   igr_access, egr_access;

    PHYMOD_MEMCPY(&igr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&egr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMSET(&egr_ps_mpls_label_en, 0, sizeof(P1588_EGR_PARSER_MPLS_LABEL_ENr_t)) ;
    PHYMOD_MEMSET(&igr_ps_mpls_label_en, 0, sizeof(P1588_ING_PARSER_MPLS_LABEL_ENr_t)) ;
    PHYMOD_MEMSET(&egr_ps_gen_ctrl,      0, sizeof(P1588_EGR_PARSER_GEN_CTRLr_t)) ;
    PHYMOD_MEMSET(&igr_ps_gen_ctrl,      0, sizeof(P1588_ING_PARSER_GEN_CTRLr_t)) ;

    if ( (config->size < 0) || (config->size > TS_MPLS_LABEL_COUNT) ) {
        return PHYMOD_E_PARAM;
    }
#ifdef PHYMOD_APERTA2_SUPPORT
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_get_igr_egr_access(phy, &igr_access, &egr_access));
#endif

    /* MPLS control */
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_PARSER_MPLS_LABEL_ENr(&igr_access, &igr_ps_mpls_label_en));
    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_PARSER_MPLS_LABEL_ENr(&egr_access, &egr_ps_mpls_label_en));
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_PARSER_GEN_CTRLr(&igr_access, &igr_ps_gen_ctrl)) ;
    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_PARSER_GEN_CTRLr(&egr_access, &egr_ps_gen_ctrl)) ;

    igr_mpls_label_en = P1588_ING_PARSER_MPLS_LABEL_ENr_MPLS_PTP_LABEL_ENf_GET(igr_ps_mpls_label_en);
    egr_mpls_label_en = P1588_EGR_PARSER_MPLS_LABEL_ENr_MPLS_PTP_LABEL_ENf_GET(egr_ps_mpls_label_en);

    if ( ( IS_TS_MPLS_DIRECTION_RX(direction) && P1588_ING_PARSER_GEN_CTRLr_MPLS_ENf_GET(igr_ps_gen_ctrl) ) ||
         ( IS_TS_MPLS_DIRECTION_TX(direction) && P1588_EGR_PARSER_GEN_CTRLr_MPLS_ENf_GET(egr_ps_gen_ctrl) ) ) {
        PHYMOD_TS_MPLS_F_ENABLE_SET(config->flags);
    }

    if ( ( IS_TS_MPLS_DIRECTION_RX(direction) && P1588_ING_PARSER_GEN_CTRLr_MPLS_CONTROL_WORD_ENf_GET(igr_ps_gen_ctrl) ) ||
         ( IS_TS_MPLS_DIRECTION_TX(direction) && P1588_EGR_PARSER_GEN_CTRLr_MPLS_CONTROL_WORD_ENf_GET(egr_ps_gen_ctrl) ) ) {
        PHYMOD_TS_MPLS_F_CONTROL_WORD_ENABLE_SET(config->flags);
    }

    if (IS_TS_MPLS_DIRECTION_RX(direction)) {
        for(idx = 0; idx < 5; idx++) {
            PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_read(&igr_access, (TS_MPLS_IGR_LABEL_MASK_MSB_ADDR + idx), &igr_mpls_label_mask_msb[idx]));
        }
        igr_mpls_label_msb = (((uint64_t)(igr_mpls_label_mask_msb[2] & 0xFFFF) << 32) |
                              ((uint64_t)(igr_mpls_label_mask_msb[1] & 0xFFFF) << 16) |
                              ((uint64_t)(igr_mpls_label_mask_msb[0] & 0xFFFF)      ) );

        igr_mpls_mask_msb  = (((uint64_t)((igr_mpls_label_mask_msb[2] >> 8) & 0xFFFF) << 32) |
                              ((uint64_t)( igr_mpls_label_mask_msb[3] & 0xFFFF) << 16) |
                              ((uint64_t)( igr_mpls_label_mask_msb[4] & 0xFFFF)      ) );
    }

    if ( IS_TS_MPLS_DIRECTION_TX(direction)) {
        for(idx = 0; idx < 5; idx++) {
            PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_read(&egr_access, (TS_MPLS_EGR_LABEL_MASK_MSB_ADDR + idx), &egr_mpls_label_mask_msb[idx]));
        }
        egr_mpls_label_msb = (((uint64_t)(egr_mpls_label_mask_msb[2] & 0xFFFF) << 32) |
                              ((uint64_t)(egr_mpls_label_mask_msb[1] & 0xFFFF) << 16) |
                              ((uint64_t)(egr_mpls_label_mask_msb[0] & 0xFFFF)      ) );

        egr_mpls_mask_msb  = (((uint64_t)((egr_mpls_label_mask_msb[2] >> 8) & 0xFFFF) << 32) |
                              ((uint64_t)( egr_mpls_label_mask_msb[3] & 0xFFFF) << 16) |
                              ((uint64_t)( egr_mpls_label_mask_msb[4] & 0xFFFF)      ) );
    }

    for(idx = 0; idx < TS_MPLS_LABEL_COUNT; idx++) {
        if (IS_TS_MPLS_DIRECTION_RX(direction)) {
            PHYMOD_IF_ERR_RETURN( plp_aperta2_p1588_reg_read(&igr_access, (TS_MPLS_IGR_LABEL0_ADDR + idx), &config->labels[idx].value));
            PHYMOD_IF_ERR_RETURN( plp_aperta2_p1588_reg_read(&igr_access, (TS_MPLS_IGR_MASK0_ADDR  + idx), &config->labels[idx].mask ));
            config->labels[idx].flags  = ((igr_mpls_label_en >> idx ) & 0x1);
            config->labels[idx].value  = (config->labels[idx].value & 0xFFFF) ;
            config->labels[idx].value |= (((igr_mpls_label_msb >> (idx << 2)) & 0xF) << 16);
            config->labels[idx].mask   = (config->labels[idx].mask  & 0xFFFF) ;
            config->labels[idx].mask  |= (((igr_mpls_mask_msb >> (idx << 2))  & 0xF) << 16);
        }
        if (IS_TS_MPLS_DIRECTION_TX(direction)) {
            PHYMOD_IF_ERR_RETURN( plp_aperta2_p1588_reg_read(&egr_access, (TS_MPLS_EGR_LABEL0_ADDR + idx), &config->labels[idx].value));
            PHYMOD_IF_ERR_RETURN( plp_aperta2_p1588_reg_read(&egr_access, (TS_MPLS_EGR_MASK0_ADDR  + idx), &config->labels[idx].mask ));
            config->labels[idx].flags  = ((egr_mpls_label_en >> idx) & 0x1);
            config->labels[idx].value  = (config->labels[idx].value & 0xFFFF) ;
            config->labels[idx].value |= (((egr_mpls_label_msb >> (idx << 2)) & 0xF) << 16);
            config->labels[idx].mask   = (config->labels[idx].mask  & 0xFFFF) ;
            config->labels[idx].mask  |= (((egr_mpls_mask_msb >> (idx << 2))  & 0xF) << 16);
        }
    }
    return PHYMOD_E_NONE;
}

int _plp_aperta2_timesync_inband_filter_set(const plp_aperta2_phymod_phy_access_t* phy, uint32_t direction, uint32_t index, const phymod_timesync_inband_filter_ctrl_t *config)
{
    P1588_EGR_PARSER_CLASSIFIER_CTRLr_t egr_ps_classifier_ctrl ;
    P1588_EGR_PARSER_CLASSIFIER_DATAr_t egr_ps_classifier_data ;
    P1588_ING_PARSER_CLASSIFIER_CTRLr_t igr_ps_classifier_ctrl ;
    P1588_ING_PARSER_CLASSIFIER_DATAr_t igr_ps_classifier_data ;

    uint32_t num_of_bytes = 0;
    uint32_t field_index  = 0;
    uint32_t idx  = 0;
    uint16_t word = 0;
    plp_aperta2_phymod_phy_access_t   igr_access, egr_access;

    PHYMOD_MEMCPY(&igr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&egr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));

    PHYMOD_MEMSET(&egr_ps_classifier_ctrl, 0, sizeof(P1588_EGR_PARSER_CLASSIFIER_CTRLr_t)) ;
    PHYMOD_MEMSET(&egr_ps_classifier_data, 0, sizeof(P1588_EGR_PARSER_CLASSIFIER_DATAr_t)) ;
    PHYMOD_MEMSET(&igr_ps_classifier_ctrl, 0, sizeof(P1588_ING_PARSER_CLASSIFIER_CTRLr_t)) ;
    PHYMOD_MEMSET(&igr_ps_classifier_data, 0, sizeof(P1588_ING_PARSER_CLASSIFIER_DATAr_t)) ;

#ifdef PHYMOD_APERTA2_SUPPORT
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_get_igr_egr_access(phy, &igr_access, &egr_access));
#endif

    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_PARSER_CLASSIFIER_CTRLr(&egr_access, &egr_ps_classifier_ctrl));
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_PARSER_CLASSIFIER_CTRLr(&igr_access, &igr_ps_classifier_ctrl));

    if(config->flags & TS_FILTER_FOR_MAC) {
        num_of_bytes = 6;
    } else if(config->flags & TS_FILTER_FOR_IPV6) {
        num_of_bytes = 16;
    } else {
        num_of_bytes = 4;
    }
    field_index = index * 9 ;
    for (idx = 0; idx < num_of_bytes; idx+=2, field_index++) {
        if(config->flags & TS_FILTER_FOR_MAC) {
            word = ((config->match_addr.mac_addr[idx+1] << 8) | (config->match_addr.mac_addr[idx]));
        } else if(config->flags & TS_FILTER_FOR_IPV6) {
            word = ((config->match_addr.ip6_addr[idx+1] << 8) | (config->match_addr.ip6_addr[idx]));
        } else {
            word = ((config->match_addr.ip_addr[idx+1]  << 8) | (config->match_addr.ip_addr[idx]));
        }
        /* Egress */
        if (PHYMOD_TS_DIRECTION_TX_GET(direction) || (direction == 0) ) {
            P1588_EGR_PARSER_CLASSIFIER_CTRLr_LOAD_INDEXf_SET(egr_ps_classifier_ctrl, 1);
            P1588_EGR_PARSER_CLASSIFIER_CTRLr_INDEXf_SET(egr_ps_classifier_ctrl, field_index);
            P1588_EGR_PARSER_CLASSIFIER_DATAr_SET(egr_ps_classifier_data, word);
            PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_PARSER_CLASSIFIER_CTRLr(&egr_access, egr_ps_classifier_ctrl));
            PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_PARSER_CLASSIFIER_DATAr(&egr_access, egr_ps_classifier_data));
        }
        /* Ingress */
        if (PHYMOD_TS_DIRECTION_RX_GET(direction) || (direction == 0) ) {
            P1588_ING_PARSER_CLASSIFIER_CTRLr_LOAD_INDEXf_SET(igr_ps_classifier_ctrl, 1);
            P1588_ING_PARSER_CLASSIFIER_CTRLr_INDEXf_SET(igr_ps_classifier_ctrl, field_index);
            P1588_ING_PARSER_CLASSIFIER_DATAr_SET(igr_ps_classifier_data, word);
            PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_PARSER_CLASSIFIER_CTRLr(&igr_access, igr_ps_classifier_ctrl));
            PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_PARSER_CLASSIFIER_DATAr(&igr_access, igr_ps_classifier_data));
        }
    }

    word = ((config->valid & 0x1) << 15) | (((config->action & 0x7) != 0) << 14) |
           (((config->flags >> 4) & 0x1) << 3)|(((config->flags >> 3) & 0x1) << 2) | (config->flags & 0x3);

    field_index = index * 9 + 8;
    /* Egress */
    if (PHYMOD_TS_DIRECTION_TX_GET(direction) || (direction == 0) ) {
        P1588_EGR_PARSER_CLASSIFIER_CTRLr_LOAD_INDEXf_SET(egr_ps_classifier_ctrl, 1);
        P1588_EGR_PARSER_CLASSIFIER_CTRLr_INDEXf_SET(egr_ps_classifier_ctrl, field_index);
        P1588_EGR_PARSER_CLASSIFIER_DATAr_SET(egr_ps_classifier_data, word);
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_PARSER_CLASSIFIER_CTRLr(&egr_access, egr_ps_classifier_ctrl));
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_PARSER_CLASSIFIER_DATAr(&egr_access, egr_ps_classifier_data));
    }
    /* Ingress */
    if (PHYMOD_TS_DIRECTION_RX_GET(direction) || (direction == 0) ) {
        P1588_ING_PARSER_CLASSIFIER_CTRLr_LOAD_INDEXf_SET(igr_ps_classifier_ctrl, 1);
        P1588_ING_PARSER_CLASSIFIER_CTRLr_INDEXf_SET(igr_ps_classifier_ctrl, field_index);
        P1588_ING_PARSER_CLASSIFIER_DATAr_SET(igr_ps_classifier_data, word);
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_PARSER_CLASSIFIER_CTRLr(&igr_access, igr_ps_classifier_ctrl));
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_PARSER_CLASSIFIER_DATAr(&igr_access, igr_ps_classifier_data));
    }

    return PHYMOD_E_NONE;
}

int _plp_aperta2_timesync_inband_filter_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t direction, uint32_t index, phymod_timesync_inband_filter_ctrl_t *config)
{
    P1588_EGR_PARSER_CLASSIFIER_CTRLr_t egr_ps_classifier_ctrl ;
    P1588_EGR_PARSER_CLASSIFIER_DATAr_t egr_ps_classifier_data ;
    P1588_ING_PARSER_CLASSIFIER_CTRLr_t igr_ps_classifier_ctrl ;
    P1588_ING_PARSER_CLASSIFIER_DATAr_t igr_ps_classifier_data ;

    uint32_t num_of_bytes = 0;
    uint32_t field_index  = 0;
    uint32_t idx  = 0;
    uint16_t word = 0;
    plp_aperta2_phymod_phy_access_t   igr_access, egr_access;

    PHYMOD_MEMCPY(&igr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&egr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMSET(&egr_ps_classifier_ctrl, 0, sizeof(P1588_EGR_PARSER_CLASSIFIER_CTRLr_t)) ;
    PHYMOD_MEMSET(&egr_ps_classifier_data, 0, sizeof(P1588_EGR_PARSER_CLASSIFIER_DATAr_t)) ;
    PHYMOD_MEMSET(&igr_ps_classifier_ctrl, 0, sizeof(P1588_ING_PARSER_CLASSIFIER_CTRLr_t)) ;
    PHYMOD_MEMSET(&igr_ps_classifier_data, 0, sizeof(P1588_ING_PARSER_CLASSIFIER_DATAr_t)) ;
#ifdef PHYMOD_APERTA2_SUPPORT
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_get_igr_egr_access(phy, &igr_access, &egr_access));
#endif
    PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_PARSER_CLASSIFIER_CTRLr(&egr_access, &egr_ps_classifier_ctrl));
    PHYMOD_IF_ERR_RETURN(READ_P1588_ING_PARSER_CLASSIFIER_CTRLr(&igr_access, &igr_ps_classifier_ctrl));

    field_index = index * 9 + 8;
    /* Egress */
    if (PHYMOD_TS_DIRECTION_TX_GET(direction) || (direction == 0)) {
        P1588_EGR_PARSER_CLASSIFIER_CTRLr_LOAD_INDEXf_SET(egr_ps_classifier_ctrl, 1);
        P1588_EGR_PARSER_CLASSIFIER_CTRLr_INDEXf_SET(egr_ps_classifier_ctrl, field_index);
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_PARSER_CLASSIFIER_CTRLr(&egr_access, egr_ps_classifier_ctrl));
        PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_PARSER_CLASSIFIER_DATAr(&egr_access, &egr_ps_classifier_data));
        word = P1588_EGR_PARSER_CLASSIFIER_DATAr_FILTER_DATAf_GET(egr_ps_classifier_data);
    }
    /* Ingress */
    if (PHYMOD_TS_DIRECTION_RX_GET(direction) || (direction == 0) ) {
        P1588_ING_PARSER_CLASSIFIER_CTRLr_LOAD_INDEXf_SET(igr_ps_classifier_ctrl, 1);
        P1588_ING_PARSER_CLASSIFIER_CTRLr_INDEXf_SET(igr_ps_classifier_ctrl, field_index);
        PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_PARSER_CLASSIFIER_CTRLr(&igr_access, igr_ps_classifier_ctrl));
        PHYMOD_IF_ERR_RETURN(READ_P1588_ING_PARSER_CLASSIFIER_DATAr(&igr_access, &igr_ps_classifier_data));
        word = P1588_ING_PARSER_CLASSIFIER_DATAr_FILTER_DATAf_GET(igr_ps_classifier_data);
    }

    config->valid  = (( word >> 15) & 0x1);
    config->action = (( word >> 14) & 0x1);
    config->flags  = (  word & 0x3);
    config->flags |= (((word >> 2 ) & 0x1) << 3);
    config->flags |= (((word >> 3 ) & 0x1) << 4);

    if(config->flags & TS_FILTER_FOR_MAC) {
        num_of_bytes = 6;
    } else if(config->flags & TS_FILTER_FOR_IPV6) {
        num_of_bytes = 16;
    } else {
        num_of_bytes = 4;
    }

    field_index = index * 9 ;
    for (idx = 0; idx < num_of_bytes; idx+=2, field_index++) {
        /* Egress */
        if (PHYMOD_TS_DIRECTION_TX_GET(direction) || (direction == 0)) {
            P1588_EGR_PARSER_CLASSIFIER_CTRLr_LOAD_INDEXf_SET(egr_ps_classifier_ctrl, 1);
            P1588_EGR_PARSER_CLASSIFIER_CTRLr_INDEXf_SET(egr_ps_classifier_ctrl, field_index);
            PHYMOD_IF_ERR_RETURN(WRITE_P1588_EGR_PARSER_CLASSIFIER_CTRLr(&egr_access, egr_ps_classifier_ctrl));
            PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_PARSER_CLASSIFIER_DATAr(&egr_access, &egr_ps_classifier_data));
            word = P1588_EGR_PARSER_CLASSIFIER_DATAr_FILTER_DATAf_GET(egr_ps_classifier_data);
        }
        /* Ingress */
        if (PHYMOD_TS_DIRECTION_RX_GET(direction) || (direction == 0) ) {
            P1588_ING_PARSER_CLASSIFIER_CTRLr_LOAD_INDEXf_SET(igr_ps_classifier_ctrl, 1);
            P1588_ING_PARSER_CLASSIFIER_CTRLr_INDEXf_SET(igr_ps_classifier_ctrl, field_index);
            PHYMOD_IF_ERR_RETURN(WRITE_P1588_ING_PARSER_CLASSIFIER_CTRLr(&igr_access, igr_ps_classifier_ctrl));
            PHYMOD_IF_ERR_RETURN(READ_P1588_ING_PARSER_CLASSIFIER_DATAr(&igr_access, &igr_ps_classifier_data));
            word = P1588_ING_PARSER_CLASSIFIER_DATAr_FILTER_DATAf_GET(igr_ps_classifier_data);
        }
        if(config->flags & TS_FILTER_FOR_MAC) {
            config->match_addr.mac_addr[idx]   = ( word & 0xFF);
            config->match_addr.mac_addr[idx+1] = ((word >> 8  ) & 0xFF);
        } else if(config->flags & TS_FILTER_FOR_IPV6) {
            config->match_addr.ip6_addr[idx]   = ( word & 0xFF);
            config->match_addr.ip6_addr[idx+1] = ((word >> 8  ) & 0xFF);
        } else {
            config->match_addr.ip_addr[idx]    = ( word & 0xFF);
            config->match_addr.ip_addr[idx+1]  = ((word >> 8  ) & 0xFF);
        }
    }
    return PHYMOD_E_NONE;
}

int _plp_aperta2_timesync_sopmem_get(const plp_aperta2_phymod_phy_access_t* phy, uint32_t flags, int entry_id, phymod_timesync_sopmem_t *keys)
{
    P1588_EGR_TS_STATUSr_t       egr_ts_status ;
    P1588_ING_TS_STATUSr_t       igr_ts_status ;
    P1588_EGR_TS_SOPMEM_SEQIDr_t egr_sopmem_seqid;
    P1588_ING_TS_SOPMEM_SEQIDr_t igr_sopmem_seqid;
    uint32_t port  = 0;
    uint32_t sopmem_seq_id          = 0;
    uint32_t sopmem_seq_id_saved    = 0;
    uint32_t sopmem_seq_id_encrypt_saved = 0;
    uint32_t sopmem_timestamp       = 0;
    uint32_t sopmem_src_prt_id      = 0;
    uint32_t sopmem_src_prt_id_4    = 0;
    uint8_t  sopmem_rd_done         = 1;
    uint8_t  sopmem_not_empty       = 0;
    uint8_t  sopmem_rd_ptr          = 0;
    uint8_t  sopmem_dir_saved       = 0;
    uint8_t  idx = 0;

    uint32_t encr_seqid    = 0xFFFFFFFF ;
    uint32_t sopmem_wr_ptr = 0;
    uint32_t encr_seqid_mem_egr_sts = 0;
    uint8_t  encr_seqid_mem_egr_not_empty = 0;
    plp_aperta2_phymod_phy_access_t   igr_access, egr_access;

    sopmem_seq_id_encrypt_saved = keys[0].seq_id_encrypt ;
    sopmem_seq_id_saved         = keys[0].seq_id ;
    sopmem_dir_saved            = keys[0].direction ;
    PHYMOD_MEMCPY(&igr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMCPY(&egr_access,     phy, sizeof(plp_aperta2_phymod_phy_access_t));
    PHYMOD_MEMSET(&egr_ts_status,    0, sizeof(P1588_EGR_TS_STATUSr_t))       ;
    PHYMOD_MEMSET(&igr_ts_status,    0, sizeof(P1588_ING_TS_STATUSr_t))       ;
    PHYMOD_MEMSET(&egr_sopmem_seqid, 0, sizeof(P1588_EGR_TS_SOPMEM_SEQIDr_t)) ;
    PHYMOD_MEMSET(&igr_sopmem_seqid, 0, sizeof(P1588_ING_TS_SOPMEM_SEQIDr_t)) ;
    for(idx=0; idx<16; idx++) {
        PHYMOD_MEMSET(&keys[idx],    0, sizeof(phymod_timesync_sopmem_t)) ;
    }
#ifdef PHYMOD_APERTA2_SUPPORT
    PHYMOD_IF_ERR_RETURN(
        plp_aperta2_get_igr_egr_access(phy, &igr_access, &egr_access));
#endif

    APERTA2_GET_PORT_FROM_LM(egr_access.access.lane_mask, port);

    do {
        PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_read(&egr_access,  (TS_ENCR_SEQID_MEM_EGR_STS_ADDR  + (((port >= 8) ? (port - 8) : port) << 4)) , &encr_seqid_mem_egr_sts));
        encr_seqid_mem_egr_not_empty = ((encr_seqid_mem_egr_sts >> 15) & 0x1);
        if(encr_seqid_mem_egr_not_empty && (PHYMOD_TS_DIRECTION_TX_GET(sopmem_dir_saved))) {

            PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_read(&egr_access, (TS_ENCR_SEQID_MEM_EGR_SEQID_ADDR       + (((port >= 8) ? (port - 8) : port) << 4)) , &encr_seqid ));
            PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_read(&egr_access, (TS_ENCR_SEQID_MEM_EGR_SOPMEM_WPTR_ADDR + (((port >= 8) ? (port - 8) : port) << 4)) , &sopmem_wr_ptr));
            encr_seqid    &= 0xFFFF ;
            sopmem_wr_ptr &= 0xF ;
        }
READ_SOPMEM_FIFO:
        if(PHYMOD_TS_DIRECTION_TX_GET(sopmem_dir_saved)) {
           PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_TS_STATUSr(&egr_access, &egr_ts_status));
           PHYMOD_IF_ERR_RETURN(READ_P1588_EGR_TS_SOPMEM_SEQIDr(&egr_access, &egr_sopmem_seqid));
           sopmem_rd_done   = P1588_EGR_TS_STATUSr_SOPMEM_RD_DONEf_GET(egr_ts_status);
           sopmem_not_empty = P1588_EGR_TS_STATUSr_SOPMEM_NOT_EMPTYf_GET(egr_ts_status);
           sopmem_rd_ptr    = P1588_EGR_TS_STATUSr_SOPMEM_RPTRf_GET(egr_ts_status);
           sopmem_seq_id    = P1588_EGR_TS_SOPMEM_SEQIDr_SOPMEM_SEQID_RDf_GET(egr_sopmem_seqid);
           if(sopmem_not_empty) {
               for(idx=0; idx<10; idx+=2) {
                   PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_read(&egr_access, (TS_SOPMEM_TS0_EGR_ADDR + (idx>>1)), &sopmem_timestamp));
                   keys[sopmem_rd_ptr].timestamp[idx  ] = ( sopmem_timestamp & 0xFF) ;
                   keys[sopmem_rd_ptr].timestamp[idx+1] = ((sopmem_timestamp >>  8 ) & 0xFF);
               }
           }
           PHYMOD_DEBUG_INFO(("EGR : RDPTR = 0x%x : SEQ_ID = %u : RD_DONE = %u : EGR_TS_STS = 0x%x\n", sopmem_rd_ptr, sopmem_seq_id, sopmem_rd_done, (egr_ts_status.v[0] & 0xFFFF)));
        }

        if((PHYMOD_TS_DIRECTION_RX_GET(sopmem_dir_saved))) {
           PHYMOD_IF_ERR_RETURN(READ_P1588_ING_TS_STATUSr(&igr_access, &igr_ts_status));
           PHYMOD_IF_ERR_RETURN(READ_P1588_ING_TS_SOPMEM_SEQIDr(&igr_access, &igr_sopmem_seqid));
           sopmem_rd_done   = P1588_ING_TS_STATUSr_SOPMEM_RD_DONEf_GET(igr_ts_status);
           sopmem_not_empty = P1588_ING_TS_STATUSr_SOPMEM_NOT_EMPTYf_GET(igr_ts_status);
           sopmem_rd_ptr    = P1588_ING_TS_STATUSr_SOPMEM_RPTRf_GET(igr_ts_status);
           sopmem_seq_id    = P1588_ING_TS_SOPMEM_SEQIDr_SOPMEM_SEQID_RDf_GET(igr_sopmem_seqid);
           if(sopmem_not_empty) {
               PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_read(&igr_access, P1588_ING_TS_SOPMEM_SRC_PORTID4r, &sopmem_src_prt_id_4));
               for(idx=0; idx<10; idx+=2) {
                   PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_read(&igr_access, (TS_SOPMEM_TS0_IGR_ADDR + (idx>>1)), &sopmem_timestamp));
                   PHYMOD_IF_ERR_RETURN(plp_aperta2_p1588_reg_read(&igr_access, (TS_SOPMEM_SRC_PRT0_IGR_ADDR + (idx>>1)), &sopmem_src_prt_id));
                   keys[sopmem_rd_ptr].timestamp[idx  ]   = ( sopmem_timestamp  & 0xFF) ;
                   keys[sopmem_rd_ptr].timestamp[idx+1]   = ((sopmem_timestamp  >> 8 ) & 0xFF);
                   if(idx >= 8) {
                       keys[sopmem_rd_ptr].src_port_id[idx]   = ( sopmem_src_prt_id_4 & 0xFF);
                       keys[sopmem_rd_ptr].src_port_id[idx+1] = ((sopmem_src_prt_id_4 >> 8 ) & 0xFF);
                   } else {
                       keys[sopmem_rd_ptr].src_port_id[idx]   = ( sopmem_src_prt_id & 0xFF);
                       keys[sopmem_rd_ptr].src_port_id[idx+1] = ((sopmem_src_prt_id >> 8 ) & 0xFF);
                   }
               }
           }
           PHYMOD_DEBUG_INFO(("ING : RDPTR = 0x%x : SEQ_ID = %u : RD_DONE = %u : IGR_TS_STS = 0x%x\n", sopmem_rd_ptr, sopmem_seq_id, sopmem_rd_done, (igr_ts_status.v[0] & 0xFFFF)));
        }

        if(sopmem_not_empty) {
            keys[sopmem_rd_ptr].seq_id    = sopmem_seq_id;
            keys[sopmem_rd_ptr].direction = sopmem_dir_saved;

            if(encr_seqid == sopmem_seq_id_encrypt_saved) {
                if(sopmem_wr_ptr != sopmem_rd_ptr) {
                    goto READ_SOPMEM_FIFO ;
                } else {
                    keys[sopmem_rd_ptr].seq_id_encrypt = encr_seqid;
                    keys[sopmem_rd_ptr].valid = 3 ;
                    break;
                }
            }

            if(sopmem_seq_id == sopmem_seq_id_saved) {
                keys[sopmem_rd_ptr].valid = 1 ;
                break;
            }
        }
    } while(sopmem_not_empty && !sopmem_rd_done);

    return PHYMOD_E_NONE;
}
