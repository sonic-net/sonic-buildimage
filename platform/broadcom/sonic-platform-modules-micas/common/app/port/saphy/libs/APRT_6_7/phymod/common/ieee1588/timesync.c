/*
 * $Id: $
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 */

#include "timesync.h"

#define  ACC(_p)        ((_p)->access)
#define  ACCP(_p)       &ACC(_p)
#define  EXP2(_n)       (0x1U << (_n))
#define  LOG2           __plp_aperta_simple_log2

#ifdef TS_IEEE1588_SOPMEM_ARCH_20   /*  XxxFINs & Whitetip  */
#define  CMP_IPMAC_SET                  __plp_aperta_sopmem_ipmac_sel_set
#define  CMP_IPMAC_GET                  __plp_aperta_sopmem_ipmac_sel_get
#else           /*  Aperta / Miura / Evora / Europa / Quadra28 / Orca  */
#define  CMP_IPMAC_SET(_e)         LOG2(__plp_aperta_sopmem_ipmac_sel_set(_e))
#define  CMP_IPMAC_GET(_r)         (__plp_aperta_sopmem_ipmac_sel_get(EXP2(_r)))
#endif

#define  WRT_IPMAC_SET                  __plp_aperta_sopmem_ipmac_sel_set
#define  WRT_IPMAC_GET                  __plp_aperta_sopmem_ipmac_sel_get

int8_t __plp_aperta_simple_log2(uint32 val) {
    int8_t  logv = -1;

    if ( val ) {
        for ( logv = 0; ! (val & 0x1); val >>= 1 ) {
            logv++;
        }
    }
    return  logv;
}

uint16_t __plp_aperta_sopmem_ipmac_sel_set(phymod_timesync_inband_parse_t in_val) {
    uint16_t  regval = TS_SOPMEM_CAP_UNKNOWN;

    switch ( in_val ) {
        case  phymodTimesyncInbandParseSrcPort   :
              regval = TS_SOPMEM_CAP_PTP_SRC_PORT;          break;
        case  phymodTimesyncInbandParseMacDa     :
              regval = TS_SOPMEM_CAP_MAC_DA      ;          break;
        case  phymodTimesyncInbandParseMacSa     :
              regval = TS_SOPMEM_CAP_MAC_SA      ;          break;
        case  phymodTimesyncInbandParseIpv4DstIp :
              regval = TS_SOPMEM_CAP_DST_IPV4    ;          break;
        case  phymodTimesyncInbandParseIpv4SrcIp :
              regval = TS_SOPMEM_CAP_SRC_IPV4    ;          break;
        case  phymodTimesyncInbandParseMplsLabel :
              regval = TS_SOPMEM_CAP_MPLS_LABEL  ;          break;
        case  phymodTimesyncInbandParseResv2     :
              regval = TS_SOPMEM_CAP_RESV_2      ;          break;
        case  phymodTimesyncInbandParseIpv6DstIp :
              regval = TS_SOPMEM_CAP_DST_IPV6    ;          break;
        default  :
              regval = TS_SOPMEM_CAP_UNKNOWN     ;
    }

    return  regval;
}

phymod_timesync_inband_parse_t __plp_aperta_sopmem_ipmac_sel_get(uint32 in_val) {
    phymod_timesync_inband_parse_t  rv = phymodTimesyncInbandParseNone;
    switch ( in_val ) {
        case  TS_SOPMEM_CAP_PTP_SRC_PORT :
              rv = phymodTimesyncInbandParseSrcPort  ;      break;
        case  TS_SOPMEM_CAP_MAC_DA       :
              rv = phymodTimesyncInbandParseMacDa    ;      break;
        case  TS_SOPMEM_CAP_MAC_SA       :
              rv = phymodTimesyncInbandParseMacSa    ;      break;
        case  TS_SOPMEM_CAP_DST_IPV4     :
              rv = phymodTimesyncInbandParseIpv4DstIp;      break;
        case  TS_SOPMEM_CAP_SRC_IPV4     :
              rv = phymodTimesyncInbandParseIpv4SrcIp;      break;
        case  TS_SOPMEM_CAP_MPLS_LABEL   :
              rv = phymodTimesyncInbandParseMplsLabel;      break;
        case  TS_SOPMEM_CAP_RESV_2       :
              rv = phymodTimesyncInbandParseResv2    ;      break;
        case  TS_SOPMEM_CAP_DST_IPV6     :
              rv = phymodTimesyncInbandParseIpv6DstIp;      break;
        default  :
              rv = phymodTimesyncInbandParseNone     ;
    }

    return  rv;
}

int _plp_aperta_timesync_config_set(const plp_aperta_phymod_phy_access_t* phy, const plp_aperta_phymod_timesync_config_t* config)
{
    uint32_t  uirx_mode2 = 0, uitx_mode2 = 0;
    uint16_t  rx_80bit_timer = 0, tx_80bit_timer = 0;
    uint16_t  use_ext_ref_clk = PHYMOD_TS_F_CLOCK_SRC_EXT_GET(config->flags);
    P1588_TX_CONTROLr_t tx_control;
    P1588_RX_CONTROLr_t rx_control;
    P1588_VLAN_TAGr_t   inner_tag;
    P1588_OUTER_VLAN_TAGr_t  outer1_tag;
    P1588_INNER_VLAN_TAGr_t  outer2_tag;
    P1588_INBAND_TX_CONTROL1r_t tx_ib_ctl1;
    P1588_INBAND_TX_CONTROL2r_t tx_ib_ctl2;
    P1588_INBAND_RX_CONTROL1r_t rx_ib_ctl1;
    P1588_INBAND_RX_CONTROL2r_t rx_ib_ctl2;
    P1588_TX_OPTION_SELr_t      tx_option_sel;
    P1588_RX_OPTION_SELr_t      rx_option_sel;
    P1588_PKT_COUNT_SELr_t      pkt_count_sel;
    P1588_TS_HB_SEL_14r_t       fifo_level;
    P1588_TX_MODE2_SELr_t       tx_mode2;
    P1588_RX_MODE2_SELr_t       rx_mode2;
    P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_t  ts_capture;
    P1588_RX_TX_CONTROLr_t   rx_tx_control;
    P1588_SOPMEM_CONTROL2r_t sopmem_ctrl2;
    P1588_HSR_CONTROLr_t hsr_ctrl;
    P1588_NSE_DPLL_7r_t  dpll_k1;
    P1588_NSE_DPLL_8r_t  dpll_k2;
    P1588_NSE_DPLL_9r_t  dpll_k3;
    P1588_NSE_DPLL_6r_t  nse_dpll6;
    P1588_NSE_DPLL_5r_t  nse_dpll5;
    P1588_NSE_DPLL_4r_t  nse_dpll4;
    P1588_NSE_DPLL_3r_t  nse_dpll3;
    P1588_NSE_DPLL_2r_t  nse_dpll2;
    P1588_NSE_DPLL_1r_t  nse_dpll1;
    P1588_PIM_DEBUG_TYPEr_t  nco_sync0_pulse;
    P1588_DPLL_DEBUG_SELr_t        dpll_dbg_sel;
    P1588_SLICE_ENABLE_CONTROLr_t  slice_en;
#ifdef PHYMOD_APERTA_SUPPORT
    uint32_t                             chip_rev = 0;
    BCMI_APERTA_D_CTRL_CHIP_REVISIONr_t  msb;
#endif

#ifdef  _MPLS_SUPPORT_
    P1588_MPLS_CONTROLr_t mpls_ctrl;
    P1588_MPLS_TX_SPECIAL_LABEL_1r_t tx_spl_lable1;
    P1588_MPLS_TX_SPECIAL_LABEL_2r_t tx_spl_lable2;
    P1588_MPLS_LABEL_DIR_1r_t label_dir1;
    P1588_MPLS_LABEL_DIR_2r_t label_dir2;

    PHYMOD_MEMSET(&mpls_ctrl, 0, sizeof(P1588_MPLS_CONTROLr_t));
    PHYMOD_MEMSET(&tx_spl_lable1, 0, sizeof(P1588_MPLS_TX_SPECIAL_LABEL_1r_t));
    PHYMOD_MEMSET(&tx_spl_lable2, 0, sizeof(P1588_MPLS_TX_SPECIAL_LABEL_2r_t));
    PHYMOD_MEMSET(&label_dir1, 0, sizeof(P1588_MPLS_LABEL_DIR_1r_t));
    PHYMOD_MEMSET(&label_dir2, 0, sizeof(P1588_MPLS_LABEL_DIR_2r_t));
#endif
    PHYMOD_MEMSET(&tx_control, 0, sizeof(P1588_TX_CONTROLr_t));
    PHYMOD_MEMSET(&rx_control, 0, sizeof(P1588_RX_CONTROLr_t));
    PHYMOD_MEMSET(&inner_tag,  0, sizeof(P1588_VLAN_TAGr_t));
    PHYMOD_MEMSET(&outer1_tag, 0, sizeof(P1588_OUTER_VLAN_TAGr_t));
    PHYMOD_MEMSET(&outer2_tag, 0, sizeof(P1588_INNER_VLAN_TAGr_t));
    PHYMOD_MEMSET(&tx_ib_ctl1, 0, sizeof(P1588_INBAND_TX_CONTROL1r_t));
    PHYMOD_MEMSET(&tx_ib_ctl2, 0, sizeof(P1588_INBAND_TX_CONTROL2r_t));
    PHYMOD_MEMSET(&rx_ib_ctl1, 0, sizeof(P1588_INBAND_RX_CONTROL1r_t));
    PHYMOD_MEMSET(&rx_ib_ctl2, 0, sizeof(P1588_INBAND_RX_CONTROL2r_t));
    PHYMOD_MEMSET(&tx_option_sel, 0, sizeof(P1588_TX_OPTION_SELr_t));
    PHYMOD_MEMSET(&rx_option_sel, 0, sizeof(P1588_RX_OPTION_SELr_t));
    PHYMOD_MEMSET(&pkt_count_sel, 0, sizeof(P1588_PKT_COUNT_SELr_t));
    PHYMOD_MEMSET(&fifo_level, 0, sizeof(P1588_TS_HB_SEL_14r_t));
    PHYMOD_MEMSET(&rx_mode2, 0, sizeof(P1588_RX_MODE2_SELr_t));
    PHYMOD_MEMSET(&tx_mode2, 0, sizeof(P1588_TX_MODE2_SELr_t));
    PHYMOD_MEMSET(&ts_capture, 0, sizeof(P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_t));
    PHYMOD_MEMSET(&rx_tx_control, 0, sizeof(P1588_RX_TX_CONTROLr_t));
    PHYMOD_MEMSET(&sopmem_ctrl2, 0, sizeof(P1588_SOPMEM_CONTROL2r_t));
    PHYMOD_MEMSET(&hsr_ctrl, 0, sizeof(P1588_HSR_CONTROLr_t));
    PHYMOD_MEMSET(&dpll_k1, 0, sizeof(P1588_NSE_DPLL_7r_t));
    PHYMOD_MEMSET(&dpll_k2, 0, sizeof(P1588_NSE_DPLL_8r_t));
    PHYMOD_MEMSET(&dpll_k3, 0, sizeof(P1588_NSE_DPLL_9r_t));
    PHYMOD_MEMSET(&nse_dpll1, 0, sizeof(P1588_NSE_DPLL_1r_t));
    PHYMOD_MEMSET(&nse_dpll2, 0, sizeof(P1588_NSE_DPLL_2r_t));
    PHYMOD_MEMSET(&nse_dpll3, 0, sizeof(P1588_NSE_DPLL_3r_t));
    PHYMOD_MEMSET(&nse_dpll4, 0, sizeof(P1588_NSE_DPLL_4r_t));
    PHYMOD_MEMSET(&nse_dpll5, 0, sizeof(P1588_NSE_DPLL_5r_t));
    PHYMOD_MEMSET(&nse_dpll6, 0, sizeof(P1588_NSE_DPLL_6r_t));
    PHYMOD_MEMSET(&nco_sync0_pulse, 0, sizeof(P1588_PIM_DEBUG_TYPEr_t));
#ifdef PHYMOD_APERTA_SUPPORT
    PHYMOD_MEMSET(&msb, 0, sizeof(BCMI_APERTA_D_CTRL_CHIP_REVISIONr_t));
    PHYMOD_IF_ERR_RETURN(
        BCMI_APERTA_D_READ_CTRL_CHIP_REVISIONr(&phy->access, &msb));

    chip_rev = BCMI_APERTA_D_CTRL_CHIP_REVISIONr_CHIP_REVf_GET(msb);
#endif


    if (PHYMOD_TS_F_CAPTURE_TS_ENABLE_GET(config->flags)) {
        P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_TS_CAPf_SET(ts_capture, 1);
        P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_CAPf_SET(ts_capture, 1);
    }
    if (PHYMOD_TS_F_TX_CRC_ENABLE_GET(config->flags)) {
        P1588_RX_TX_CONTROLr_TX_CRC_ENf_SET(rx_tx_control, 1);
    }
    if (PHYMOD_TS_F_RX_CRC_ENABLE_GET(config->flags)) {
        P1588_RX_TX_CONTROLr_RX_CRC_ENf_SET(rx_tx_control, 1);
    }
    if (PHYMOD_TS_F_8021AS_ENABLE_GET(config->flags)) {
        P1588_TX_CONTROLr_TX_AS_ENf_SET(tx_control, 1);
        P1588_RX_CONTROLr_RX_AS_ENf_SET(rx_control, 1);
    } else {
        P1588_TX_CONTROLr_TX_AS_ENf_SET(tx_control, 0);
        P1588_RX_CONTROLr_RX_AS_ENf_SET(rx_control, 0);
    }
    if (PHYMOD_TS_F_L2_ENABLE_GET(config->flags)) {
        P1588_TX_CONTROLr_TX_L2_ENf_SET(tx_control, 1);
        P1588_RX_CONTROLr_RX_L2_ENf_SET(rx_control, 1);
    } else {
        P1588_TX_CONTROLr_TX_L2_ENf_SET(tx_control, 0);
        P1588_RX_CONTROLr_RX_L2_ENf_SET(rx_control, 0);
    }
    if (PHYMOD_TS_F_IP4_ENABLE_GET(config->flags)) {
        P1588_TX_CONTROLr_TX_L4_IPV4_UDP_ENf_SET(tx_control, 1);
        P1588_RX_CONTROLr_RX_L4_IPV4_UDP_ENf_SET(rx_control, 1);
    } else {
        P1588_TX_CONTROLr_TX_L4_IPV4_UDP_ENf_SET(tx_control, 0);
        P1588_RX_CONTROLr_RX_L4_IPV4_UDP_ENf_SET(rx_control, 0);
    }
    if (PHYMOD_TS_F_IP6_ENABLE_GET(config->flags)) {
        P1588_RX_CONTROLr_RX_L4_IPV6_UDP_ENf_SET(rx_control, 1);
        P1588_TX_CONTROLr_TX_L4_IPV6_UDP_ENf_SET(tx_control, 1);
    } else {
        P1588_RX_CONTROLr_RX_L4_IPV6_UDP_ENf_SET(rx_control, 0);
        P1588_TX_CONTROLr_TX_L4_IPV6_UDP_ENf_SET(tx_control, 0);
    }

    if (PHYMOD_TS_F_1588_OVER_HSR_ENABLE_GET(config->flags)) {
        P1588_HSR_CONTROLr_HSR_LLC_DSAP_SSAP_CHECK_ENABLEf_SET(hsr_ctrl, 1);
        P1588_HSR_CONTROLr_HSR_PARSER_ENABLEf_SET(hsr_ctrl, 1);
    }
    if (PHYMOD_TS_F_CAPTURE_TIMESTAMP_TX_SYNC_GET(config->flags)) {
        uitx_mode2 |= (1 << 1);
    }
    if (PHYMOD_TS_F_CAPTURE_TIMESTAMP_TX_DELAY_REQ_GET(config->flags)) {
        uitx_mode2 |= (1 << 2);
    }
    if (PHYMOD_TS_F_CAPTURE_TIMESTAMP_TX_PDELAY_REQ_GET(config->flags)) {
        uitx_mode2 |= (1 << 3);
    }
    if (PHYMOD_TS_F_CAPTURE_TIMESTAMP_TX_PDELAY_RESP_GET(config->flags)) {
        uitx_mode2 |= (1 << 4);
    }
    if (PHYMOD_TS_F_CAPTURE_TIMESTAMP_RX_SYNC_GET(config->flags)) {
        uirx_mode2 |= (1 << 1);
    }
    if (PHYMOD_TS_F_CAPTURE_TIMESTAMP_RX_DELAY_REQ_GET(config->flags)) {
        uirx_mode2 |= (1 << 2);
    }
    if (PHYMOD_TS_F_CAPTURE_TIMESTAMP_RX_PDELAY_REQ_GET(config->flags)) {
        uirx_mode2 |= (1 << 3);
    }
    if (PHYMOD_TS_F_CAPTURE_TIMESTAMP_RX_PDELAY_RESP_GET(config->flags)) {
        uirx_mode2 |= (1 << 4);
    }

    /* inband reserved feild 0 */
    P1588_INBAND_TX_CONTROL1r_TX_RESV0_IDf_SET(tx_ib_ctl1, config->inband_ctrl.resv0_id);
    P1588_INBAND_RX_CONTROL1r_RX_RESV0_IDf_SET(rx_ib_ctl1, config->inband_ctrl.resv0_id);

    if (config->inband_ctrl.flags & 0x1) {
        P1588_INBAND_TX_CONTROL1r_TX_CHECK_RESV0_IDf_SET(tx_ib_ctl1, 1);
        P1588_INBAND_RX_CONTROL1r_RX_CHECK_RESV0_IDf_SET(rx_ib_ctl1, 1);
    }
    if (config->inband_ctrl.flags & 0x2) {
        P1588_INBAND_TX_CONTROL1r_TX_INBAND_ONf_SET(tx_ib_ctl1, 1);
        P1588_INBAND_RX_CONTROL1r_RX_INBAND_ONf_SET(rx_ib_ctl1, 1);
    }
    if (config->inband_ctrl.flags & 0x4) {
        P1588_INBAND_RX_CONTROL1r_RX_UPDATE_RESV0_IDf_SET(rx_ib_ctl1, 1);
    }

    /* 80-bit or 48-bit timers */
    tx_80bit_timer = ((config->inband_ctrl.timer_mode   == phymodTimesyncTimerMode48Bit) ||
                      (config->tx_inband_prop.ts_80bits == FALSE) ) ?  FALSE : TRUE;
    rx_80bit_timer = ((config->inband_ctrl.timer_mode   == phymodTimesyncTimerMode48Bit) ||
                      (config->rx_inband_prop.ts_80bits == FALSE) ) ?  FALSE : TRUE;
    PHYMOD_IF_ERR_RETURN( READ_P1588_SOPMEM_CONTROL2r( phy, &sopmem_ctrl2) );
    PLP_P1588_SOPMEM_CONTROL2r_NSE_48BIT_TIMERf_SET(sopmem_ctrl2, ((! rx_80bit_timer) && (! tx_80bit_timer)) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_SOPMEM_CONTROL2r(phy,  sopmem_ctrl2) );

    P1588_TX_OPTION_SELr_TX_OPTION_80BIT_TIMER_TCf_SET(tx_option_sel,  tx_80bit_timer);
    P1588_RX_OPTION_SELr_RX_OPTION_80BIT_TIMER_TCf_SET(rx_option_sel,  rx_80bit_timer);

  #ifdef PHYMOD_APERTA_SUPPORT
    if ( chip_rev == APERTA_REV_B0 ) {              /* Aperta_B0 always set USE_80BIT for */
        tx_80bit_timer = rx_80bit_timer =  TRUE;    /*    Rx/Tx Inband Control 1 register */
    }
  #endif
    P1588_INBAND_RX_CONTROL1r_RX_TS_USE_80BITSf_SET(rx_ib_ctl1,  rx_80bit_timer);
    P1588_INBAND_TX_CONTROL1r_TX_TS_USE_80BITSf_SET(tx_ib_ctl1,  tx_80bit_timer);

    /* Tx Inband Control 1 */
    P1588_INBAND_TX_CONTROL1r_TX_INBAND_ONf_SET(             tx_ib_ctl1, config->tx_inband_prop.inband_on);
    P1588_INBAND_TX_CONTROL1r_TX_CHECK_RESV0_IDf_SET(        tx_ib_ctl1, config->tx_inband_prop.check_resv0);
    P1588_INBAND_TX_CONTROL1r_TX_UPDATE_RESV0_IDf_SET(       tx_ib_ctl1, config->tx_inband_prop.update_resv0);
    P1588_INBAND_TX_CONTROL1r_TX_MODE_TCf_SET(               tx_ib_ctl1, config->tx_inband_prop.tc_mode);
    P1588_INBAND_TX_CONTROL1r_TX_MODE_PARTIAL_TCf_SET(       tx_ib_ctl1, config->tx_inband_prop.partial_tc_mode);
    P1588_INBAND_TX_CONTROL1r_TX_MDIO_SOPMEM_OPTIONf_SET(    tx_ib_ctl1, config->tx_inband_prop.mdio_sopmem);
    P1588_INBAND_TX_CONTROL1r_TX_INBAND_STRICT_OPTIONf_SET(  tx_ib_ctl1, config->tx_inband_prop.strict);
    P1588_INBAND_TX_CONTROL1r_TX_INBAND_TS32_FORMATf_SET(    tx_ib_ctl1, config->tx_inband_prop.ts32_format);
    P1588_INBAND_TX_CONTROL1r_TX_INBAND_CLEAR_RSV012_RGf_SET(tx_ib_ctl1, config->tx_inband_prop.clear_rsv012);
    /* Tx Inband Control 2 */
    P1588_INBAND_TX_CONTROL2r_CMP_VLAN_IDf_SET(   tx_ib_ctl2, config->tx_inband_prop.cmp_vlan_id);
    P1588_INBAND_TX_CONTROL2r_CMP_SOPMEMf_SET(    tx_ib_ctl2, CMP_IPMAC_SET(config->tx_inband_prop.compare_sopmem));
    P1588_INBAND_TX_CONTROL2r_CMP_FIELD_SELf_SET( tx_ib_ctl2, config->tx_inband_prop.cmp_field_sel);
    P1588_INBAND_TX_CONTROL2r_CMP_SRC_PORTf_SET(  tx_ib_ctl2, config->tx_inband_prop.cmp_src_port);
    P1588_INBAND_TX_CONTROL2r_CMP_SEQ_NUMf_GSET(  tx_ib_ctl2, config->tx_inband_prop.cmp_seq_num);
    P1588_INBAND_TX_CONTROL2r_CMP_DOMAIN_NUMf_SET(tx_ib_ctl2, config->tx_inband_prop.cmp_domain_num);
    P1588_INBAND_TX_CONTROL2r_WRITE_SOPMEMf_SET(  tx_ib_ctl2, WRT_IPMAC_SET(config->tx_inband_prop.write_sopmem));
    /* Tx P1588 Option Select */
    P1588_TX_OPTION_SELr_TX_OPTION_KEEP_CRCf_SET(     tx_option_sel, config->tx_option.keep_ori_crc);
    P1588_TX_OPTION_SELr_TX_OPTION_TC_TO_INSf_SET(    tx_option_sel, config->tx_option.timecode_to_insertion);
    P1588_TX_OPTION_SELr_TX_OPTION_PTPv2_CHK_DISf_SET(tx_option_sel, config->tx_option.ptpv2_chk_dis);
    P1588_TX_OPTION_SELr_TX_OPTION_TS_TO_SOPMEMf_SET( tx_option_sel, config->tx_option.force_ts_to_sopmem);

    PHYMOD_IF_ERR_RETURN( WRITE_P1588_INBAND_TX_CONTROL1r(phy, tx_ib_ctl1) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_INBAND_TX_CONTROL2r(phy, tx_ib_ctl2) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_TX_OPTION_SELr(phy, tx_option_sel) );

    /* Rx Inband Control 1 */
    P1588_INBAND_RX_CONTROL1r_RX_INBAND_ONf_SET(           rx_ib_ctl1, config->rx_inband_prop.inband_on);
    P1588_INBAND_RX_CONTROL1r_RX_CHECK_RESV0_IDf_SET(      rx_ib_ctl1, config->rx_inband_prop.check_resv0);
    P1588_INBAND_RX_CONTROL1r_RX_UPDATE_RESV0_IDf_SET(     rx_ib_ctl1, config->rx_inband_prop.update_resv0);
    P1588_INBAND_RX_CONTROL1r_RX_MODE_TCf_SET(             rx_ib_ctl1, config->rx_inband_prop.tc_mode);
    P1588_INBAND_RX_CONTROL1r_RX_MODE_PARTIAL_TCf_SET(     rx_ib_ctl1, config->rx_inband_prop.partial_tc_mode);
    P1588_INBAND_RX_CONTROL1r_RX_MDIO_SOPMEM_OPTIONf_SET(  rx_ib_ctl1, config->rx_inband_prop.mdio_sopmem);
    P1588_INBAND_RX_CONTROL1r_RX_INBAND_STRICT_OPTIONf_SET(rx_ib_ctl1, config->rx_inband_prop.strict);
    P1588_INBAND_RX_CONTROL1r_RX_INBAND_TS32_FORMATf_SET(  rx_ib_ctl1, config->rx_inband_prop.ts32_format);
    P1588_INBAND_RX_CONTROL1r_RX_INBAND_INSERT_4BYTESf_SET(rx_ib_ctl1, config->rx_inband_prop.clear_rsv012);
    /* Rx Inband Control 2 */
    P1588_INBAND_RX_CONTROL2r_CMP_VLAN_IDf_SET(   rx_ib_ctl2, config->rx_inband_prop.cmp_vlan_id);
    P1588_INBAND_RX_CONTROL2r_CMP_SOPMEMf_SET(    rx_ib_ctl2, CMP_IPMAC_SET(config->rx_inband_prop.compare_sopmem));
    P1588_INBAND_RX_CONTROL2r_CMP_FIELD_SELf_SET( rx_ib_ctl2, config->rx_inband_prop.cmp_field_sel);
    P1588_INBAND_RX_CONTROL2r_CMP_SRC_PORTf_SET(  rx_ib_ctl2, config->rx_inband_prop.cmp_src_port);
    P1588_INBAND_RX_CONTROL2r_CMP_SEQ_NUMf_GSET(  rx_ib_ctl2, config->rx_inband_prop.cmp_seq_num);
    P1588_INBAND_RX_CONTROL2r_CMP_DOMAIN_NUMf_SET(rx_ib_ctl2, config->rx_inband_prop.cmp_domain_num);
    P1588_INBAND_RX_CONTROL2r_WRITE_SOPMEMf_SET(  rx_ib_ctl2, WRT_IPMAC_SET(config->rx_inband_prop.write_sopmem));
    /* Rx P1588 Option Select */
    P1588_RX_OPTION_SELr_RX_OPTION_KEEP_CRCf_SET(     rx_option_sel, config->rx_option.keep_ori_crc);
    P1588_RX_OPTION_SELr_RX_OPTION_TC_TO_INSf_SET(    rx_option_sel, config->rx_option.timecode_to_insertion);
    P1588_RX_OPTION_SELr_RX_OPTION_PTPv2_CHK_DISf_SET(rx_option_sel, config->rx_option.ptpv2_chk_dis);
    P1588_RX_OPTION_SELr_RX_OPTION_TS_TO_SOPMEMf_SET( rx_option_sel, config->rx_option.force_ts_to_sopmem);

    PHYMOD_IF_ERR_RETURN( WRITE_P1588_INBAND_RX_CONTROL1r(phy, rx_ib_ctl1) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_INBAND_RX_CONTROL2r(phy, rx_ib_ctl2) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_RX_OPTION_SELr(phy, rx_option_sel) );

    /* PTP/CRC Packet Count Select  (TX) */
    P1588_PKT_COUNT_SELr_TX_CRC_COUNT_ENf_SET(pkt_count_sel,
                            PHYMOD_PKT_TYPE_CRC_GET(config->tx_pkt_count_sel));
    if ( ! PHYMOD_PKT_TYPE_ALL_GET(config->tx_pkt_count_sel) ) {
        /* count only PTP packets */
        uint32_t  val = 0;
        int       ptp_msg_sel_cnt = 0;
        P1588_PKT_COUNT_SELr_TX_P1588_PKT_SELf_SET(pkt_count_sel, TRUE);
        if ( PHYMOD_PKT_TYPE_PTP_PDELAYRESP_GET(config->tx_pkt_count_sel) ) {
            val = P1588_PKT_COUNT_SEL_MSG_TYPE_PDELAYRESP;
            ptp_msg_sel_cnt++;
        }
        if ( PHYMOD_PKT_TYPE_PTP_PDELAYREQ_GET(config->tx_pkt_count_sel) ) {
            val = P1588_PKT_COUNT_SEL_MSG_TYPE_PDELAYREQ;
            ptp_msg_sel_cnt++;
        }
        if ( PHYMOD_PKT_TYPE_PTP_DELAYREQ_GET(config->tx_pkt_count_sel) ) {
            val = P1588_PKT_COUNT_SEL_MSG_TYPE_DELAYREQ;
            ptp_msg_sel_cnt++;
        }
        if ( PHYMOD_PKT_TYPE_PTP_SYNC_GET(config->tx_pkt_count_sel) ) {
            val = P1588_PKT_COUNT_SEL_MSG_TYPE_SYNC;
            ptp_msg_sel_cnt++;
        }
        P1588_PKT_COUNT_SELr_TX_P1588_MSG_TYPE_SELf_SET(pkt_count_sel, val);
        /* ptp_msg_sel_cnt==4 means count all four types of PTP messages */
        val = (ptp_msg_sel_cnt < 4) ? P1588_PKT_COUNT_SEL_MSG_TYPE_SINGLE
                                    : P1588_PKT_COUNT_SEL_MSG_TYPE_ALLPTP;
        P1588_PKT_COUNT_SELr_TX_P1588_MSG_INDV_SELf_SET(pkt_count_sel, val);
    }
    /* PTP/CRC Packet Count Select  (RX) */
    P1588_PKT_COUNT_SELr_RX_CRC_COUNT_ENf_SET(pkt_count_sel,
                            PHYMOD_PKT_TYPE_CRC_GET(config->rx_pkt_count_sel));
    if ( ! PHYMOD_PKT_TYPE_ALL_GET(config->rx_pkt_count_sel) ) {
        /* count only PTP packets */
        uint32_t  val = 0;
        int       ptp_msg_sel_cnt = 0;
        P1588_PKT_COUNT_SELr_RX_P1588_PKT_SELf_SET(pkt_count_sel, TRUE);
        if ( PHYMOD_PKT_TYPE_PTP_PDELAYRESP_GET(config->rx_pkt_count_sel) ) {
            val = P1588_PKT_COUNT_SEL_MSG_TYPE_PDELAYRESP;
            ptp_msg_sel_cnt++;
        }
        if ( PHYMOD_PKT_TYPE_PTP_PDELAYREQ_GET(config->rx_pkt_count_sel) ) {
            val = P1588_PKT_COUNT_SEL_MSG_TYPE_PDELAYREQ;
            ptp_msg_sel_cnt++;
        }
        if ( PHYMOD_PKT_TYPE_PTP_DELAYREQ_GET(config->rx_pkt_count_sel) ) {
            val = P1588_PKT_COUNT_SEL_MSG_TYPE_DELAYREQ;
            ptp_msg_sel_cnt++;
        }
        if ( PHYMOD_PKT_TYPE_PTP_SYNC_GET(config->rx_pkt_count_sel) ) {
            val = P1588_PKT_COUNT_SEL_MSG_TYPE_SYNC;
            ptp_msg_sel_cnt++;
        }
        P1588_PKT_COUNT_SELr_RX_P1588_MSG_TYPE_SELf_SET(pkt_count_sel, val);
        /* ptp_msg_sel_cnt==4 means count all four types of PTP messages */
        val = (ptp_msg_sel_cnt < 4) ? P1588_PKT_COUNT_SEL_MSG_TYPE_SINGLE
                                    : P1588_PKT_COUNT_SEL_MSG_TYPE_ALLPTP;
        P1588_PKT_COUNT_SELr_RX_P1588_MSG_INDV_SELf_SET(pkt_count_sel, val);
    }
    /* write value to P1588 PKT COUNT SEL Register */
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_PKT_COUNT_SELr(phy, pkt_count_sel) );

    /* threshold value to trigger the packet timestamp interrupt */
    P1588_TS_HB_SEL_14r_FIFO_LEVELf_SET(fifo_level, config->fifo_level_intr_thold);
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_TS_HB_SEL_14r(phy, fifo_level) );

    /* read P1588M_slice_enable_control register  ( 1588reg.00[10] ) */
    PHYMOD_IF_ERR_RETURN( READ_P1588_SLICE_ENABLE_CONTROLr(phy, &slice_en) );
  #if defined(PHYMOD_XGBASET_SUPPORT) || defined(PHYMOD_MGAUTO_SUPPORT)
    PHYMOD_IF_ERR_RETURN( __timesync_xgbaset_clock_xtal_1588_enable(phy, PORTS_PER_PHY_CHIP_MAX,
                                        P1588_SLICE_ENABLE_CONTROLr_RXTX_SLICE_1588_ENf_GET(slice_en),
                                        config->flags) );
  #endif
    /* to use external PTP reference clock source,                              *\
    |* disable internal DPLL and set freq_mdio_sel to Use nco_freqcntrl_reg     *|
    \*         as adder input for 48-bit and 80-bit timers   ( 1588reg.13[5] )  */
    PHYMOD_IF_ERR_RETURN(  READ_P1588_DPLL_DEBUG_SELr(phy, &dpll_dbg_sel) );
    P1588_DPLL_DEBUG_SELr_FREQ_MDIO_SELf_SET(               dpll_dbg_sel, use_ext_ref_clk);
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_DPLL_DEBUG_SELr(phy,  dpll_dbg_sel) );

    if ( use_ext_ref_clk ) {    /* use external  PTP reference clock source */
      #if defined(PHYMOD_XGBASET_SUPPORT) || defined(PHYMOD_MGAUTO_SUPPORT)
        uint16_t  sp = 0;
        PHYMOD_IF_ERR_RETURN( _PHY_SPEED_LINE_GET(phy, &sp) );
        /* set nse_timer_new_clock_enable for 10G speed         ( 1588reg.00[10] ) */
        P1588_SLICE_ENABLE_CONTROLr_NSE_TIMER_NEW_CLOCK_ENABLEf_SET( slice_en,
                         (10000 == _PHY_SPEED_LINE_INTERPRET(sp)) ? TRUE : FALSE );
      #endif
        /* set IEEE 1588/PIM external timer clock select        ( 1588reg.00[3] )  */
        P1588_SLICE_ENABLE_CONTROLr_EXT_TIMER_CLOCK_ENABLEf_SET(     slice_en,
                 (PHYMOD_TS_F_CLOCK_SRC_EXT_MODE_GET(config->flags)) ? TRUE : FALSE );
        /* write P1588M_slice_enable_control register           ( 1588reg.00[10] ) */
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_SLICE_ENABLE_CONTROLr(phy, slice_en) );
    }
    else {                      /* use internal DPLL reference clock source */
        /* DPLL settings */
        P1588_NSE_DPLL_7r_NSE_REG_K1f_SET(dpll_k1, config->phy_1588_dpll_k1);
        P1588_NSE_DPLL_8r_NSE_REG_K2f_SET(dpll_k2, config->phy_1588_dpll_k2);
        P1588_NSE_DPLL_9r_NSE_REG_K3f_SET(dpll_k3, config->phy_1588_dpll_k3);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_NSE_DPLL_7r(phy, dpll_k1) );
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_NSE_DPLL_8r(phy, dpll_k2) );
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_NSE_DPLL_9r(phy, dpll_k3) );

        P1588_NSE_DPLL_4r_SET(nse_dpll4, ( COMPILER_64_LO(config->phy_1588_dpll_ref_phase)) & BIT_15_00_MASK);
        P1588_NSE_DPLL_3r_SET(nse_dpll3, ((COMPILER_64_LO(config->phy_1588_dpll_ref_phase)) >> 16) & BIT_15_00_MASK);
        P1588_NSE_DPLL_2r_SET(nse_dpll2, ( COMPILER_64_HI(config->phy_1588_dpll_ref_phase)) & BIT_15_00_MASK);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_NSE_DPLL_4r(phy, nse_dpll4) );
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_NSE_DPLL_3r(phy, nse_dpll3) );
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_NSE_DPLL_2r(phy, nse_dpll2) );

        P1588_NSE_DPLL_6r_SET(nse_dpll6, ( config->phy_1588_dpll_ref_phase_delta & BIT_15_00_MASK));
        P1588_NSE_DPLL_5r_SET(nse_dpll5, ((config->phy_1588_dpll_ref_phase_delta) >> 16) & BIT_15_00_MASK);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_NSE_DPLL_6r(phy, nse_dpll6) );
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_NSE_DPLL_5r(phy, nse_dpll5) );
    }  /* use_ext_ref_clk */

    /* HeartBeat capturing */
    P1588_NSE_DPLL_1r_NSE_HB_CAPTURE_MODEf_SET(nse_dpll1, config->hb_capture_mode);
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_NSE_DPLL_1r(phy, nse_dpll1) );

    /* TPID VLAN Tag */
    P1588_VLAN_TAGr_ITPIDf_SET(       inner_tag , config->itpid );
    P1588_OUTER_VLAN_TAGr_OTPIDf_SET( outer1_tag, config->otpid );
    P1588_INNER_VLAN_TAGr_OTPID2f_SET(outer2_tag, config->otpid2);

    P1588_RX_MODE2_SELr_SET(rx_mode2, uirx_mode2);
    P1588_TX_MODE2_SELr_SET(tx_mode2, uitx_mode2);
    P1588_PIM_DEBUG_TYPEr_PIM_SYNC0_PULSEf_SET(nco_sync0_pulse, config->nco_sync0_pulse);

    PHYMOD_IF_ERR_RETURN( WRITE_P1588_PIM_DEBUG_TYPEr(phy, nco_sync0_pulse) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_HSR_CONTROLr(phy, hsr_ctrl) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_RX_TX_CONTROLr(phy, rx_tx_control) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr(phy, ts_capture) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_TX_MODE2_SELr(phy, tx_mode2) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_RX_MODE2_SELr(phy, rx_mode2) );

    PHYMOD_IF_ERR_RETURN( WRITE_P1588_VLAN_TAGr(phy, inner_tag) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_OUTER_VLAN_TAGr(phy, outer1_tag) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_INNER_VLAN_TAGr(phy, outer2_tag) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_TX_CONTROLr(phy, tx_control) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_RX_CONTROLr(phy, rx_control) );

#ifdef  PHYMOD_APERTA_SUPPORT
  { /* P1588 TXRX SOP TS CAPTURE ENABLE Register */
    PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_t  sop_ts_cap;
    PHYMOD_MEMSET(&sop_ts_cap, 0, sizeof(PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_t));

    P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_UPDATE_DP_TSf_SET(sop_ts_cap,
                                          PHYMOD_TS_DIRECTION_RX_GET(  config->sop_ts_cap.dp_ts_wclk));
    P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_UPDATE_DP_TSf_SET(sop_ts_cap,
                                          PHYMOD_TS_DIRECTION_TX_GET(  config->sop_ts_cap.dp_ts_wclk));
    P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_PM_P1588_ENAf_SET(sop_ts_cap,
                                         PHYMOD_TS_STAMPING_PM_RX_GET( config->sop_ts_cap.stamping));
    P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_PM_P1588_ENAf_SET(sop_ts_cap,
                                         PHYMOD_TS_STAMPING_PM_TX_GET( config->sop_ts_cap.stamping));
    P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_P1588_BYPASSf_SET(   sop_ts_cap,
                                        PHYMOD_TS_STAMPING_BYPASS_GET( config->sop_ts_cap.stamping));
    P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_CAPf_SET(      sop_ts_cap,
                                           PHYMOD_TS_DIRECTION_RX_GET( config->sop_ts_cap.ts_cap));
    P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_TS_CAPf_SET(      sop_ts_cap,
                                           PHYMOD_TS_DIRECTION_TX_GET( config->sop_ts_cap.ts_cap));
    P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_ECCERR_ENAf_SET(     sop_ts_cap, config->sop_ts_cap.err_ecc2pkt);
    P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_LOV_ENAf_SET(  sop_ts_cap, config->sop_ts_cap.lov);

    PHYMOD_IF_ERR_RETURN( WRITE_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr(phy, sop_ts_cap) );
  }
#endif

    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_config_get(const plp_aperta_phymod_phy_access_t* phy, plp_aperta_phymod_timesync_config_t* config)
{
    uint32_t  temp = 0;
    P1588_SLICE_ENABLE_CONTROLr_t  slice_en;
    P1588_DPLL_DEBUG_SELr_t        dpll_dbg_sel;
    P1588_TX_CONTROLr_t         tx_control;
    P1588_RX_CONTROLr_t         rx_control;
    P1588_VLAN_TAGr_t           inner_tag;
    P1588_OUTER_VLAN_TAGr_t     outer1_tag;
    P1588_INNER_VLAN_TAGr_t     outer2_tag;
    P1588_INBAND_TX_CONTROL1r_t tx_ib_ctl1;
    P1588_INBAND_TX_CONTROL2r_t tx_ib_ctl2;
    P1588_INBAND_RX_CONTROL1r_t rx_ib_ctl1;
    P1588_INBAND_RX_CONTROL2r_t rx_ib_ctl2;
    P1588_TX_OPTION_SELr_t      tx_option_sel;
    P1588_RX_OPTION_SELr_t      rx_option_sel;
    P1588_PKT_COUNT_SELr_t      pkt_count_sel;
    P1588_TS_HB_SEL_14r_t       fifo_level;
    P1588_RX_MODE2_SELr_t       rx_mode2;
    P1588_TX_MODE2_SELr_t       tx_mode2;
    P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_t  ts_capture;
    P1588_RX_TX_CONTROLr_t   rx_tx_control;
    P1588_NSE_SC_8r_t        nse_sc_8;
    P1588_SOPMEM_CONTROL2r_t sopmem_ctrl2;
    P1588_HSR_CONTROLr_t hsr_ctrl;
    P1588_NSE_DPLL_7r_t  dpll_k1;
    P1588_NSE_DPLL_8r_t  dpll_k2;
    P1588_NSE_DPLL_9r_t  dpll_k3;
    P1588_NSE_DPLL_6r_t  nse_dpll6;
    P1588_NSE_DPLL_5r_t  nse_dpll5;
    P1588_NSE_DPLL_4r_t  nse_dpll4;
    P1588_NSE_DPLL_3r_t  nse_dpll3;
    P1588_NSE_DPLL_2r_t  nse_dpll2;
    P1588_NSE_DPLL_1r_t  nse_dpll1;
    P1588_PIM_DEBUG_TYPEr_t  nco_sync0_pulse;
#ifdef  _MPLS_SUPPORT_
    P1588_MPLS_CONTROLr_t mpls_ctrl;
    P1588_MPLS_TX_SPECIAL_LABEL_1r_t tx_spl_lable1;
    P1588_MPLS_TX_SPECIAL_LABEL_2r_t tx_spl_lable2;
    P1588_MPLS_LABEL_DIR_1r_t label_dir1;
    P1588_MPLS_LABEL_DIR_2r_t label_dir2;

    PHYMOD_MEMSET(&mpls_ctrl, 0, sizeof(P1588_MPLS_CONTROLr_t));
    PHYMOD_MEMSET(&tx_spl_lable1, 0, sizeof(P1588_MPLS_TX_SPECIAL_LABEL_1r_t));
    PHYMOD_MEMSET(&tx_spl_lable2, 0, sizeof(P1588_MPLS_TX_SPECIAL_LABEL_2r_t));
    PHYMOD_MEMSET(&label_dir1, 0, sizeof(P1588_MPLS_LABEL_DIR_1r_t));
    PHYMOD_MEMSET(&label_dir2, 0, sizeof(P1588_MPLS_LABEL_DIR_2r_t));
#endif
    P1588_SLICE_ENABLE_CONTROLr_CLR(slice_en);
    P1588_DPLL_DEBUG_SELr_CLR(dpll_dbg_sel);
    PHYMOD_MEMSET(&tx_control, 0, sizeof(P1588_TX_CONTROLr_t));
    PHYMOD_MEMSET(&rx_control, 0, sizeof(P1588_RX_CONTROLr_t));
    PHYMOD_MEMSET(&inner_tag,  0, sizeof(P1588_VLAN_TAGr_t));
    PHYMOD_MEMSET(&outer1_tag, 0, sizeof(P1588_OUTER_VLAN_TAGr_t));
    PHYMOD_MEMSET(&outer2_tag, 0, sizeof(P1588_INNER_VLAN_TAGr_t));
    PHYMOD_MEMSET(&tx_ib_ctl1, 0, sizeof(P1588_INBAND_TX_CONTROL1r_t));
    PHYMOD_MEMSET(&tx_ib_ctl2, 0, sizeof(P1588_INBAND_TX_CONTROL2r_t));
    PHYMOD_MEMSET(&rx_ib_ctl1, 0, sizeof(P1588_INBAND_RX_CONTROL1r_t));
    PHYMOD_MEMSET(&rx_ib_ctl2, 0, sizeof(P1588_INBAND_RX_CONTROL2r_t));
    PHYMOD_MEMSET(&tx_option_sel, 0, sizeof(P1588_TX_OPTION_SELr_t));
    PHYMOD_MEMSET(&rx_option_sel, 0, sizeof(P1588_RX_OPTION_SELr_t));
    PHYMOD_MEMSET(&pkt_count_sel, 0, sizeof(P1588_PKT_COUNT_SELr_t));
    PHYMOD_MEMSET(&fifo_level, 0, sizeof(P1588_TS_HB_SEL_14r_t));
    PHYMOD_MEMSET(&rx_mode2, 0, sizeof(P1588_RX_MODE2_SELr_t));
    PHYMOD_MEMSET(&tx_mode2, 0, sizeof(P1588_TX_MODE2_SELr_t));
    PHYMOD_MEMSET(&ts_capture, 0, sizeof(P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_t));
    PHYMOD_MEMSET(&rx_tx_control, 0, sizeof(P1588_RX_TX_CONTROLr_t));
    PHYMOD_MEMSET(&nse_sc_8, 0, sizeof(P1588_NSE_SC_8r_t));
    PHYMOD_MEMSET(&sopmem_ctrl2, 0, sizeof(P1588_SOPMEM_CONTROL2r_t));
    PHYMOD_MEMSET(&hsr_ctrl, 0, sizeof(P1588_HSR_CONTROLr_t));
    PHYMOD_MEMSET(&dpll_k1, 0, sizeof(P1588_NSE_DPLL_7r_t));
    PHYMOD_MEMSET(&dpll_k2, 0, sizeof(P1588_NSE_DPLL_8r_t));
    PHYMOD_MEMSET(&dpll_k3, 0, sizeof(P1588_NSE_DPLL_9r_t));
    PHYMOD_MEMSET(&nse_dpll2, 0, sizeof(P1588_NSE_DPLL_2r_t));
    PHYMOD_MEMSET(&nse_dpll3, 0, sizeof(P1588_NSE_DPLL_3r_t));
    PHYMOD_MEMSET(&nse_dpll4, 0, sizeof(P1588_NSE_DPLL_4r_t));
    PHYMOD_MEMSET(&nse_dpll5, 0, sizeof(P1588_NSE_DPLL_5r_t));
    PHYMOD_MEMSET(&nse_dpll6, 0, sizeof(P1588_NSE_DPLL_6r_t));
    PHYMOD_MEMSET(&nco_sync0_pulse, 0, sizeof(P1588_PIM_DEBUG_TYPEr_t));

    PHYMOD_IF_ERR_RETURN( READ_P1588_SLICE_ENABLE_CONTROLr(phy, &slice_en) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_DPLL_DEBUG_SELr( phy, &dpll_dbg_sel) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_PIM_DEBUG_TYPEr( phy, &nco_sync0_pulse) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_SOPMEM_CONTROL2r(phy, &sopmem_ctrl2) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_DPLL_5r(phy, &nse_dpll5) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_DPLL_6r(phy, &nse_dpll6) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_DPLL_4r(phy, &nse_dpll4) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_DPLL_3r(phy, &nse_dpll3) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_DPLL_2r(phy, &nse_dpll2) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_DPLL_1r(phy, &nse_dpll1) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_DPLL_7r(phy, &dpll_k1) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_DPLL_8r(phy, &dpll_k2) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_DPLL_9r(phy, &dpll_k3) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_HSR_CONTROLr(phy, &hsr_ctrl) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_SC_8r(phy, &nse_sc_8) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_RX_TX_CONTROLr(phy, &rx_tx_control) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr(phy, &ts_capture) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_TX_MODE2_SELr(phy, &tx_mode2) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_RX_MODE2_SELr(phy, &rx_mode2) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_INBAND_TX_CONTROL1r(phy, &tx_ib_ctl1) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_INBAND_RX_CONTROL1r(phy, &rx_ib_ctl1) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_INBAND_TX_CONTROL2r(phy, &tx_ib_ctl2) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_INBAND_RX_CONTROL2r(phy, &rx_ib_ctl2) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_TX_OPTION_SELr(phy, &tx_option_sel) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_RX_OPTION_SELr(phy, &rx_option_sel) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_TX_CONTROLr(phy, &tx_control) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_RX_CONTROLr(phy, &rx_control) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_VLAN_TAGr(phy, &inner_tag) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_OUTER_VLAN_TAGr(phy, &outer1_tag) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_INNER_VLAN_TAGr(phy, &outer2_tag) );

    if ( P1588_DPLL_DEBUG_SELr_FREQ_MDIO_SELf_GET(dpll_dbg_sel) ) {
        PHYMOD_TS_F_CLOCK_SRC_EXT_SET(config->flags);
    }
    if ( P1588_SLICE_ENABLE_CONTROLr_EXT_TIMER_CLOCK_ENABLEf_GET(slice_en) ) {
        PHYMOD_TS_F_CLOCK_SRC_EXT_MODE_SET(config->flags);
    }
    if (P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_TS_CAPf_GET(ts_capture)) {
        PHYMOD_TS_F_CAPTURE_TS_ENABLE_SET(config->flags);
    }
    if ( P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_13f_GET(nse_sc_8)) {
        PHYMOD_TS_F_HEARTBEAT_TS_ENABLE_SET(config->flags);
    }
    if (P1588_RX_TX_CONTROLr_TX_CRC_ENf_GET(rx_tx_control)) {
        PHYMOD_TS_F_TX_CRC_ENABLE_SET(config->flags);
    }
    if (P1588_RX_TX_CONTROLr_RX_CRC_ENf_GET(rx_tx_control)) {
        PHYMOD_TS_F_RX_CRC_ENABLE_SET(config->flags);
    }
    if (P1588_TX_CONTROLr_TX_AS_ENf_GET(tx_control)) {
        PHYMOD_TS_F_8021AS_ENABLE_SET(config->flags);
    }
    if (P1588_TX_CONTROLr_TX_L2_ENf_GET(tx_control)) {
        PHYMOD_TS_F_L2_ENABLE_SET(config->flags);
    }
    if (P1588_TX_CONTROLr_TX_L4_IPV4_UDP_ENf_GET(tx_control)) {
        PHYMOD_TS_F_IP4_ENABLE_SET(config->flags);
    }
    if (P1588_RX_CONTROLr_RX_L4_IPV6_UDP_ENf_GET(rx_control)) {
       PHYMOD_TS_F_IP6_ENABLE_SET(config->flags);
    }
    if (P1588_HSR_CONTROLr_HSR_PARSER_ENABLEf_GET(hsr_ctrl)) {
        PHYMOD_TS_F_1588_OVER_HSR_ENABLE_SET(config->flags);
    }

    if (tx_mode2._p1588_tx_mode2_sel & (1 << 1)) {
        PHYMOD_TS_F_CAPTURE_TIMESTAMP_TX_SYNC_SET(config->flags);
    }
    if (tx_mode2._p1588_tx_mode2_sel & (1 << 2)) {
        PHYMOD_TS_F_CAPTURE_TIMESTAMP_TX_DELAY_REQ_SET(config->flags);
    }
    if (tx_mode2._p1588_tx_mode2_sel & (1 << 3)) {
        PHYMOD_TS_F_CAPTURE_TIMESTAMP_TX_PDELAY_REQ_SET(config->flags);
    }
    if (tx_mode2._p1588_tx_mode2_sel & (1 << 4)) {
       PHYMOD_TS_F_CAPTURE_TIMESTAMP_TX_PDELAY_RESP_SET(config->flags);
    }
    if (rx_mode2._p1588_rx_mode2_sel & (1 << 1)) {
        PHYMOD_TS_F_CAPTURE_TIMESTAMP_RX_SYNC_SET(config->flags);
    }
    if (rx_mode2._p1588_rx_mode2_sel & (1 << 2)) {
        PHYMOD_TS_F_CAPTURE_TIMESTAMP_RX_DELAY_REQ_SET(config->flags);
    }
    if (rx_mode2._p1588_rx_mode2_sel & (1 << 3)) {
        PHYMOD_TS_F_CAPTURE_TIMESTAMP_RX_PDELAY_REQ_SET(config->flags);
    }
    if (rx_mode2._p1588_rx_mode2_sel & (1 << 4)) {
        PHYMOD_TS_F_CAPTURE_TIMESTAMP_RX_PDELAY_RESP_SET(config->flags);
    }

    config->inband_ctrl.resv0_id = P1588_INBAND_TX_CONTROL1r_TX_RESV0_IDf_GET(tx_ib_ctl1);
    if (P1588_INBAND_TX_CONTROL1r_TX_CHECK_RESV0_IDf_GET(tx_ib_ctl1)) {
        config->inband_ctrl.flags |= 0x1;
    }
    if (P1588_INBAND_TX_CONTROL1r_TX_INBAND_ONf_GET(tx_ib_ctl1)) {
        config->inband_ctrl.flags |= 0x2;
    }
    if (P1588_INBAND_RX_CONTROL1r_RX_UPDATE_RESV0_IDf_GET(rx_ib_ctl1)) {
        config->inband_ctrl.flags |= 0x4;
    }
    config->inband_ctrl.timer_mode = PLP_P1588_SOPMEM_CONTROL2r_NSE_48BIT_TIMERf_GET(sopmem_ctrl2)
                                   ? phymodTimesyncTimerMode48Bit : phymodTimesyncTimerMode80Bit ;
    /* global mode */
    config->gmode = P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_15_14f_GET(nse_sc_8) - 1;

    /* Tx Inband Control 1 */
    config->tx_inband_prop.inband_on       = P1588_INBAND_TX_CONTROL1r_TX_INBAND_ONf_GET(             tx_ib_ctl1);
    config->tx_inband_prop.check_resv0     = P1588_INBAND_TX_CONTROL1r_TX_CHECK_RESV0_IDf_GET(        tx_ib_ctl1);
    config->tx_inband_prop.update_resv0    = P1588_INBAND_TX_CONTROL1r_TX_UPDATE_RESV0_IDf_GET(       tx_ib_ctl1);
    config->tx_inband_prop.tc_mode         = P1588_INBAND_TX_CONTROL1r_TX_MODE_TCf_GET(               tx_ib_ctl1);
    config->tx_inband_prop.partial_tc_mode = P1588_INBAND_TX_CONTROL1r_TX_MODE_PARTIAL_TCf_GET(       tx_ib_ctl1);
    config->tx_inband_prop.ts_80bits       = P1588_INBAND_TX_CONTROL1r_TX_TS_USE_80BITSf_GET(         tx_ib_ctl1);
    config->tx_inband_prop.mdio_sopmem     = P1588_INBAND_TX_CONTROL1r_TX_MDIO_SOPMEM_OPTIONf_GET(    tx_ib_ctl1);
    config->tx_inband_prop.strict          = P1588_INBAND_TX_CONTROL1r_TX_INBAND_STRICT_OPTIONf_GET(  tx_ib_ctl1);
    config->tx_inband_prop.ts32_format     = P1588_INBAND_TX_CONTROL1r_TX_INBAND_TS32_FORMATf_GET(    tx_ib_ctl1);
    config->tx_inband_prop.clear_rsv012    = P1588_INBAND_TX_CONTROL1r_TX_INBAND_CLEAR_RSV012_RGf_GET(tx_ib_ctl1);
    /* Tx Inband Control 2 */
    config->tx_inband_prop.cmp_vlan_id     = P1588_INBAND_TX_CONTROL2r_CMP_VLAN_IDf_GET(   tx_ib_ctl2);
    config->tx_inband_prop.cmp_field_sel   = P1588_INBAND_TX_CONTROL2r_CMP_FIELD_SELf_GET( tx_ib_ctl2);
    config->tx_inband_prop.cmp_src_port    = P1588_INBAND_TX_CONTROL2r_CMP_SRC_PORTf_GET(  tx_ib_ctl2);
    config->tx_inband_prop.cmp_seq_num     = P1588_INBAND_TX_CONTROL2r_CMP_SEQ_NUMf_GET(   tx_ib_ctl2);
    config->tx_inband_prop.cmp_domain_num  = P1588_INBAND_TX_CONTROL2r_CMP_DOMAIN_NUMf_GET(tx_ib_ctl2);
    config->tx_inband_prop.compare_sopmem  = CMP_IPMAC_GET(P1588_INBAND_TX_CONTROL2r_CMP_SOPMEMf_GET(  tx_ib_ctl2));
    config->tx_inband_prop.write_sopmem    = WRT_IPMAC_GET(P1588_INBAND_TX_CONTROL2r_WRITE_SOPMEMf_GET(tx_ib_ctl2));
    /* Tx P1588 Option Select */
    config->tx_option.keep_ori_crc         = P1588_TX_OPTION_SELr_TX_OPTION_KEEP_CRCf_GET(     tx_option_sel);
    config->tx_option.timecode_to_insertion= P1588_TX_OPTION_SELr_TX_OPTION_TC_TO_INSf_GET(    tx_option_sel);
    config->tx_option.ptpv2_chk_dis        = P1588_TX_OPTION_SELr_TX_OPTION_PTPv2_CHK_DISf_GET(tx_option_sel);
    config->tx_option.force_ts_to_sopmem   = P1588_TX_OPTION_SELr_TX_OPTION_TS_TO_SOPMEMf_GET( tx_option_sel);
    /* Rx Inband Control 1 */
    config->rx_inband_prop.inband_on       = P1588_INBAND_RX_CONTROL1r_RX_INBAND_ONf_GET(             rx_ib_ctl1);
    config->rx_inband_prop.check_resv0     = P1588_INBAND_RX_CONTROL1r_RX_CHECK_RESV0_IDf_GET(        rx_ib_ctl1);
    config->rx_inband_prop.update_resv0    = P1588_INBAND_RX_CONTROL1r_RX_UPDATE_RESV0_IDf_GET(       rx_ib_ctl1);
    config->rx_inband_prop.tc_mode         = P1588_INBAND_RX_CONTROL1r_RX_MODE_TCf_GET(               rx_ib_ctl1);
    config->rx_inband_prop.partial_tc_mode = P1588_INBAND_RX_CONTROL1r_RX_MODE_PARTIAL_TCf_GET(       rx_ib_ctl1);
    config->rx_inband_prop.ts_80bits       = P1588_INBAND_RX_CONTROL1r_RX_TS_USE_80BITSf_GET(         rx_ib_ctl1);
    config->rx_inband_prop.mdio_sopmem     = P1588_INBAND_RX_CONTROL1r_RX_MDIO_SOPMEM_OPTIONf_GET(    rx_ib_ctl1);
    config->rx_inband_prop.strict          = P1588_INBAND_RX_CONTROL1r_RX_INBAND_STRICT_OPTIONf_GET(  rx_ib_ctl1);
    config->rx_inband_prop.ts32_format     = P1588_INBAND_RX_CONTROL1r_RX_INBAND_TS32_FORMATf_GET(    rx_ib_ctl1);
    config->rx_inband_prop.ins_4bytes      = P1588_INBAND_RX_CONTROL1r_RX_INBAND_INSERT_4BYTESf_GET(  rx_ib_ctl1);
    /* Rx Inband Control 2 */
    config->rx_inband_prop.cmp_vlan_id     = P1588_INBAND_RX_CONTROL2r_CMP_VLAN_IDf_GET(   rx_ib_ctl2);
    config->rx_inband_prop.cmp_field_sel   = P1588_INBAND_RX_CONTROL2r_CMP_FIELD_SELf_GET( rx_ib_ctl2);
    config->rx_inband_prop.cmp_src_port    = P1588_INBAND_RX_CONTROL2r_CMP_SRC_PORTf_GET(  rx_ib_ctl2);
    config->rx_inband_prop.cmp_seq_num     = P1588_INBAND_RX_CONTROL2r_CMP_SEQ_NUMf_GET(   rx_ib_ctl2);
    config->rx_inband_prop.cmp_domain_num  = P1588_INBAND_RX_CONTROL2r_CMP_DOMAIN_NUMf_GET(rx_ib_ctl2);
    config->rx_inband_prop.compare_sopmem  = CMP_IPMAC_GET(P1588_INBAND_RX_CONTROL2r_CMP_SOPMEMf_GET(  rx_ib_ctl2));
    config->rx_inband_prop.write_sopmem    = WRT_IPMAC_GET(P1588_INBAND_RX_CONTROL2r_WRITE_SOPMEMf_GET(rx_ib_ctl2));
    /* Rx P1588 Option Select */
    config->rx_option.keep_ori_crc         = P1588_RX_OPTION_SELr_RX_OPTION_KEEP_CRCf_GET(     rx_option_sel);
    config->rx_option.timecode_to_insertion= P1588_RX_OPTION_SELr_RX_OPTION_TC_TO_INSf_GET(    rx_option_sel);
    config->rx_option.ptpv2_chk_dis        = P1588_RX_OPTION_SELr_RX_OPTION_PTPv2_CHK_DISf_GET(rx_option_sel);
    config->rx_option.force_ts_to_sopmem   = P1588_RX_OPTION_SELr_RX_OPTION_TS_TO_SOPMEMf_GET( rx_option_sel);

    /* PTP/CRC Packet Count Select  (TX) */
    PHYMOD_IF_ERR_RETURN( READ_P1588_PKT_COUNT_SELr(phy, &pkt_count_sel) );
    if ( P1588_PKT_COUNT_SELr_TX_CRC_COUNT_ENf_GET(pkt_count_sel) ) {
        PHYMOD_PKT_TYPE_CRC_SET(config->tx_pkt_count_sel);
    }
    if ( P1588_PKT_COUNT_SELr_TX_P1588_MSG_INDV_SELf_GET(pkt_count_sel) ) {
        config->tx_pkt_count_sel |=  1U <<
                (P1588_PKT_COUNT_SELr_TX_P1588_MSG_TYPE_SELf_GET(pkt_count_sel) + 8);
    } else {
        config->tx_pkt_count_sel |= BIT_11_08_MASK;     /* count all PTP messages */
    }
    /* PTP/CRC Packet Count Select  (RX) */
    PHYMOD_IF_ERR_RETURN( READ_P1588_PKT_COUNT_SELr(phy, &pkt_count_sel) );
    if ( P1588_PKT_COUNT_SELr_RX_CRC_COUNT_ENf_GET(pkt_count_sel) ) {
        PHYMOD_PKT_TYPE_CRC_SET(config->rx_pkt_count_sel);
    }
    if ( P1588_PKT_COUNT_SELr_RX_P1588_MSG_INDV_SELf_GET(pkt_count_sel) ) {
        config->rx_pkt_count_sel |=  1U <<
                (P1588_PKT_COUNT_SELr_RX_P1588_MSG_TYPE_SELf_GET(pkt_count_sel) + 8);
    } else {
        config->rx_pkt_count_sel |= BIT_11_08_MASK;     /* count all PTP messages */
    }

    /* threshold value to trigger the packet timestamp interrupt */
    PHYMOD_IF_ERR_RETURN( READ_P1588_TS_HB_SEL_14r(phy, &fifo_level) );
    config->fifo_level_intr_thold = P1588_TS_HB_SEL_14r_FIFO_LEVELf_GET(fifo_level);

    config->phy_1588_dpll_k1 = P1588_NSE_DPLL_7r_NSE_REG_K1f_GET(dpll_k1);
    config->phy_1588_dpll_k2 = P1588_NSE_DPLL_8r_NSE_REG_K2f_GET(dpll_k2);
    config->phy_1588_dpll_k3 = P1588_NSE_DPLL_9r_NSE_REG_K3f_GET(dpll_k3);

    temp  = P1588_NSE_DPLL_4r_GET(nse_dpll4);
    temp |= P1588_NSE_DPLL_3r_GET(nse_dpll3) << 16;
    COMPILER_64_SET(config->phy_1588_dpll_ref_phase, P1588_NSE_DPLL_2r_GET(nse_dpll2), temp);

    config->hb_capture_mode = P1588_NSE_DPLL_1r_NSE_HB_CAPTURE_MODEf_GET(nse_dpll1);

    config->phy_1588_dpll_ref_phase_delta = P1588_NSE_DPLL_6r_GET(nse_dpll6);
    config->phy_1588_dpll_ref_phase_delta |= P1588_NSE_DPLL_5r_GET(nse_dpll5) << 16;

    config->itpid = P1588_VLAN_TAGr_ITPIDf_GET(inner_tag);
    config->otpid = P1588_OUTER_VLAN_TAGr_OTPIDf_GET(outer1_tag);
    config->otpid2 = P1588_INNER_VLAN_TAGr_OTPID2f_GET(outer2_tag);
    config->nco_sync0_pulse = PLP_P1588_PIM_DEBUG_TYPEr_PIM_SYNC0_PULSEf_GET(nco_sync0_pulse);

#ifdef  PHYMOD_APERTA_SUPPORT
  { /* P1588 TXRX SOP TS CAPTURE ENABLE Register */
    PLP_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_t  sop_ts_cap;
    PHYMOD_IF_ERR_RETURN( READ_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr(phy, &sop_ts_cap) );

    if ( P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_UPDATE_DP_TSf_GET(sop_ts_cap) ) {
        PHYMOD_TS_DIRECTION_RX_SET(   config->sop_ts_cap.dp_ts_wclk);
    }
    if ( P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_UPDATE_DP_TSf_GET(sop_ts_cap) ) {
        PHYMOD_TS_DIRECTION_TX_SET(   config->sop_ts_cap.dp_ts_wclk);
    }
    if ( P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_PM_P1588_ENAf_GET(sop_ts_cap) ) {
        PHYMOD_TS_STAMPING_PM_RX_SET( config->sop_ts_cap.stamping);
    }
    if ( P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_PM_P1588_ENAf_GET(sop_ts_cap) ) {
        PHYMOD_TS_STAMPING_PM_TX_SET( config->sop_ts_cap.stamping);
    }
    if ( P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_P1588_BYPASSf_GET(   sop_ts_cap) ) {
        PHYMOD_TS_STAMPING_BYPASS_SET(config->sop_ts_cap.stamping);
    }
    if ( P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_CAPf_GET(      sop_ts_cap) ) {
        PHYMOD_TS_DIRECTION_RX_SET(   config->sop_ts_cap.ts_cap);
    }
    if ( P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TX_TS_CAPf_GET(      sop_ts_cap) ) {
        PHYMOD_TS_DIRECTION_TX_SET(   config->sop_ts_cap.ts_cap);
    }
    config->sop_ts_cap.err_ecc2pkt = P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_ECCERR_ENAf_GET(sop_ts_cap);
    config->sop_ts_cap.lov    =   P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_RX_TS_LOV_ENAf_GET(sop_ts_cap);
  }
#endif

    return PHYMOD_E_NONE;
}

/* no Inband Filter for Whitetip and mGigAuto */
#if (! defined(PHYMOD_WHITETIP_SUPPORT)) && (! defined(PHYMOD_MGAUTO_SUPPORT))

int __plp_aperta_ts_hw_set_filter_flags_action_valid(const plp_aperta_phymod_phy_access_t* phy,
                                         PLP_P1588_PKGEN_CONTROLr_t *pkgen_ctrl,
                                         const phymod_timesync_inband_filter_ctrl_t *config,
                                         uint32_t filter_index, uint32_t direction)
{
    P1588_PKGEN_WDATAr_t pkgen_wdata;
    PLP_P1588_PKGEN_CONTROLr_t lpkgen_ctrl = *pkgen_ctrl;
    unsigned short bits16;
    uint32_t pkgen_index;

    /*
     * 0 = no action  1 = perform inband36  2 = perform inband32
     * 3 = perform MDIO, no-inband
     * 4 = inband32 + ptpver (Egress only)
     */
    if (config->action == FILTER_ACTION_INBAND32_PTPVER) {
        if (IS_TS_INBAND_FILTER_DIRECTION_RX(direction)) {
            return PHYMOD_E_PARAM;
        }
    }
    PHYMOD_MEMSET(&pkgen_wdata, 0, sizeof(P1588_PKGEN_WDATAr_t));
    pkgen_index = IS_TS_INBAND_FILTER_DIRECTION_TX(direction) ?
                  ((filter_index * 4) + 0x0D) : ((filter_index * 4) + 0x8D);
    P1588_PKGEN_CONTROLr_PKGEN_LOAD_INDEXf_SET(lpkgen_ctrl, PKGEN_EN_INDEX_LOAD);
    P1588_PKGEN_CONTROLr_PKGEN_INDEXf_SET(lpkgen_ctrl, pkgen_index);
    /* Reserved bits [10:09] must be zeroes */
    lpkgen_ctrl.p1588_pkgen_control[0] &= 0xf9ff;
    PHYMOD_IF_ERR_RETURN(PLP_WRITE_P1588_PKGEN_CONTROLr(phy, lpkgen_ctrl));

    bits16 = ((config->valid & 0x1) << 15) |
             ((config->action & 0x7)<< 12) |
             (config->flags & 0xF)  << 4;
    P1588_PKGEN_WDATAr_PKGEN_WDATAf_SET(pkgen_wdata, bits16);
    PHYMOD_IF_ERR_RETURN(PLP_WRITE_P1588_PKGEN_WDATAr(phy, pkgen_wdata));

    return PHYMOD_E_NONE;
}

int __plp_aperta_ts_hw_get_filter_flags_action_valid(const plp_aperta_phymod_phy_access_t* phy,
                                         PLP_P1588_PKGEN_CONTROLr_t *pkgen_ctrl,
                                         phymod_timesync_inband_filter_ctrl_t *config,
                                         uint32_t filter_index, uint32_t direction)
{
    int pkgen_index;
    PLP_P1588_PKGEN_RDATAr_t pkgen_rdata;
    unsigned short bits16;
    PLP_P1588_PKGEN_CONTROLr_t lpkgen_ctrl = *pkgen_ctrl;

    PHYMOD_MEMSET(&pkgen_rdata, 0, sizeof(P1588_PKGEN_RDATAr_t));

    pkgen_index = IS_TS_INBAND_FILTER_DIRECTION_TX(direction) ?
                  ((filter_index * 4) + 0x0D) : ((filter_index * 4) + 0x8D);
    /* 1588Reg.c7[15]  : load both index for indirector mode */
    P1588_PKGEN_CONTROLr_PKGEN_LOAD_INDEXf_SET(lpkgen_ctrl, PKGEN_EN_INDEX_LOAD);
    /* 1588Reg.c7[8:0] : index of filter entry */
    P1588_PKGEN_CONTROLr_PKGEN_INDEXf_SET(lpkgen_ctrl, pkgen_index);
    lpkgen_ctrl.p1588_pkgen_control[0] &= 0xf9ff;  /* 1588Reg.c7[10:9] reserved bits */
    /* write 1588Reg.c7 pkgen control type Register */
    PHYMOD_IF_ERR_RETURN(PLP_WRITE_P1588_PKGEN_CONTROLr(phy, lpkgen_ctrl));

    /* read  1588Reg.c9 pkgen data Register */
    PHYMOD_IF_ERR_RETURN(PLP_READ_P1588_PKGEN_RDATAr(phy, &pkgen_rdata));
    bits16 = P1588_PKGEN_RDATAr_PKGEN_RDATAf_GET(pkgen_rdata);

    config->flags  = (bits16 & 0xF0) >> 4;
    config->action = (bits16 >> 12) & 0x7;
    config->valid  = (bits16 >> 15) & 0x1;

    return PHYMOD_E_NONE;
}

int __plp_aperta_ts_hw_set_filter_addr(const plp_aperta_phymod_phy_access_t* phy,
                           PLP_P1588_PKGEN_CONTROLr_t *pkgen_ctrl,
                           const phymod_timesync_inband_filter_ctrl_t *config,
                           uint32_t filter_index, uint32_t direction)
{
    int ii, pkgen_index, nbytes;
    P1588_PKGEN_WDATAr_t pkgen_wdata;
    unsigned short bits16, lowbyte, highbyte;
    PLP_P1588_PKGEN_CONTROLr_t lpkgen_ctrl = *pkgen_ctrl;

    PHYMOD_MEMSET(&pkgen_wdata, 0, sizeof(P1588_PKGEN_WDATAr_t));
    nbytes = (config->flags & FILTER_FOR_MAC) ? 6 : 4;

    pkgen_index = IS_TS_INBAND_FILTER_DIRECTION_TX(direction) ?
                  ((filter_index * 4) + 0x0A) : ((filter_index * 4) + 0x8A);
    for (ii = 0; ii < nbytes; ii+=2, pkgen_index++) {
        P1588_PKGEN_CONTROLr_PKGEN_LOAD_INDEXf_SET(lpkgen_ctrl, PKGEN_EN_INDEX_LOAD);
        P1588_PKGEN_CONTROLr_PKGEN_INDEXf_SET(lpkgen_ctrl, pkgen_index);
        lpkgen_ctrl.p1588_pkgen_control[0] &= 0xf9ff;
        PHYMOD_IF_ERR_RETURN(PLP_WRITE_P1588_PKGEN_CONTROLr(phy, lpkgen_ctrl));
        if (config->flags & FILTER_FOR_MAC) {
            lowbyte = (config->match_addr.mac_addr[ii]);
            highbyte = (config->match_addr.mac_addr[ii+1]);
        } else {
            lowbyte = (config->match_addr.ip_addr[ii]);
            highbyte = (config->match_addr.ip_addr[ii+1]);
        }
        bits16  = ((highbyte & 0xFF) << 8) | (lowbyte & 0xFF);
        P1588_PKGEN_WDATAr_PKGEN_WDATAf_SET(pkgen_wdata, bits16);
        PHYMOD_IF_ERR_RETURN(PLP_WRITE_P1588_PKGEN_WDATAr(phy, pkgen_wdata));
    }

    return PHYMOD_E_NONE;
}

int __plp_aperta_ts_hw_get_filter_addr(const plp_aperta_phymod_phy_access_t* phy,
                           PLP_P1588_PKGEN_CONTROLr_t *pkgen_ctrl,
                           phymod_timesync_inband_filter_ctrl_t *config,
                           uint32_t filter_index, uint32_t direction)
{
    int ii, pkgen_index, nbytes;
    PLP_P1588_PKGEN_RDATAr_t pkgen_rdata;
    unsigned short bits16;
    PLP_P1588_PKGEN_CONTROLr_t lpkgen_ctrl = *pkgen_ctrl;

    PHYMOD_MEMSET(&pkgen_rdata, 0, sizeof(P1588_PKGEN_RDATAr_t));

    nbytes = (config->flags & FILTER_FOR_MAC) ? 6 : 4;

    pkgen_index = IS_TS_INBAND_FILTER_DIRECTION_TX(direction) ?
                  ((filter_index * 4) + 0x0A) : ((filter_index * 4) + 0x8A);
    for (ii = 0; ii < nbytes; ii+=2, pkgen_index++) {
        P1588_PKGEN_CONTROLr_PKGEN_LOAD_INDEXf_SET(lpkgen_ctrl, PKGEN_EN_INDEX_LOAD);
        P1588_PKGEN_CONTROLr_PKGEN_INDEXf_SET(lpkgen_ctrl, pkgen_index);
        lpkgen_ctrl.p1588_pkgen_control[0] &= 0xf9ff;
        PHYMOD_IF_ERR_RETURN(PLP_WRITE_P1588_PKGEN_CONTROLr(phy, lpkgen_ctrl));

        PHYMOD_IF_ERR_RETURN(PLP_READ_P1588_PKGEN_RDATAr(phy, &pkgen_rdata));
        bits16 = P1588_PKGEN_RDATAr_PKGEN_RDATAf_GET(pkgen_rdata);
        if (config->flags & FILTER_FOR_MAC) {
            config->match_addr.mac_addr[ii] = bits16 & 0xFF;
            config->match_addr.mac_addr[ii+1] = (bits16 & 0xFF00) >> 8;
        } else {
            config->match_addr.ip_addr[ii] = bits16 & 0xFF;
            config->match_addr.ip_addr[ii+1] = (bits16 & 0xFF00) >> 8;
        }
    }
    return PHYMOD_E_NONE;
}

int __plp_aperta_ts_hw_set_filter_direction(const plp_aperta_phymod_phy_access_t* phy,
                                PLP_P1588_PKGEN_CONTROLr_t *pkgen_ctrl,
                                uint32_t direction)
{
    int rxtx;
    unsigned short bits16;
    P1588_PKGEN_WDATAr_t pkgen_wdata;
    P1588_PKGEN_RDATAr_t pkgen_rdata;
    PLP_P1588_PKGEN_CONTROLr_t lpkgen_ctrl = *pkgen_ctrl;

    PHYMOD_MEMSET(&pkgen_rdata, 0, sizeof(P1588_PKGEN_RDATAr_t));
    PHYMOD_MEMSET(&pkgen_wdata, 0, sizeof(P1588_PKGEN_WDATAr_t));
    rxtx = IS_TS_INBAND_FILTER_DIRECTION_TX(direction) ?
               PKGEN_INDREG_TX_INDEX : PKGEN_INDREG_RX_INDEX;
    /* 1588Reg.c7[15] : load both index for indirector mode */
    P1588_PKGEN_CONTROLr_PKGEN_LOAD_INDEXf_SET(lpkgen_ctrl, PKGEN_EN_INDEX_LOAD);
    P1588_PKGEN_CONTROLr_PKGEN_INDEXf_SET(lpkgen_ctrl, rxtx);
    lpkgen_ctrl.p1588_pkgen_control[0] &= 0xf9ff;
    PHYMOD_IF_ERR_RETURN(PLP_WRITE_P1588_PKGEN_CONTROLr(phy, lpkgen_ctrl));

    PHYMOD_IF_ERR_RETURN(PLP_READ_P1588_PKGEN_RDATAr(phy, &pkgen_rdata));
    bits16 = P1588_PKGEN_RDATAr_PKGEN_RDATAf_GET(pkgen_rdata);
    bits16 = (bits16 & 0xFE) | DIS_PKGEN_ENA_FILTER;
    P1588_PKGEN_WDATAr_PKGEN_WDATAf_SET(pkgen_wdata, bits16);
    PHYMOD_IF_ERR_RETURN(PLP_WRITE_P1588_PKGEN_WDATAr(phy, pkgen_wdata));

    return PHYMOD_E_NONE;
}

int __plp_aperta_ts_hw_get_filter_direction(const plp_aperta_phymod_phy_access_t* phy,
                                PLP_P1588_PKGEN_CONTROLr_t *pkgen_ctrl,
                                uint32_t direction)
{
    int rxtx;
    unsigned short bits16;
    PLP_P1588_PKGEN_RDATAr_t pkgen_rdata;
    PLP_P1588_PKGEN_CONTROLr_t lpkgen_ctrl = *pkgen_ctrl;

    PHYMOD_MEMSET(&pkgen_rdata, 0, sizeof(P1588_PKGEN_RDATAr_t));
    rxtx = IS_TS_INBAND_FILTER_DIRECTION_TX(direction) ?
               PKGEN_INDREG_TX_INDEX : PKGEN_INDREG_RX_INDEX;
    /* 1588Reg.c7[15] : load both index for indirector mode */
    P1588_PKGEN_CONTROLr_PKGEN_LOAD_INDEXf_SET(lpkgen_ctrl, PKGEN_EN_INDEX_LOAD);
    /* 1588Reg.c7[8:0] 2=Tx_pkgen_enable , 3=Rx_pkgen_enable */
    P1588_PKGEN_CONTROLr_PKGEN_INDEXf_SET(lpkgen_ctrl, rxtx);
    lpkgen_ctrl.p1588_pkgen_control[0] &= 0xf9ff;
    /* write 1588Reg.c7 pkgen control type Register */
    PHYMOD_IF_ERR_RETURN(PLP_WRITE_P1588_PKGEN_CONTROLr(phy, lpkgen_ctrl));

    /* read  1588Reg.c9 pkgen data Register */
    PHYMOD_IF_ERR_RETURN(PLP_READ_P1588_PKGEN_RDATAr(phy, &pkgen_rdata));
    bits16 = P1588_PKGEN_RDATAr_PKGEN_RDATAf_GET(pkgen_rdata);
    if ((bits16 & 0x1) != DIS_PKGEN_ENA_FILTER)    /* check 1588Reg.c9[0] */
        return PHYMOD_E_UNAVAIL;

    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_inband_filter_set(const plp_aperta_phymod_phy_access_t* phy,
                                uint32_t direction, uint32_t index,
                                const phymod_timesync_inband_filter_ctrl_t *config)
{
    PLP_P1588_PKGEN_CONTROLr_t pkgen_ctrl;

    PHYMOD_MEMSET(&pkgen_ctrl, 0, sizeof(P1588_PKGEN_CONTROLr_t));
    PHYMOD_IF_ERR_RETURN(PLP_READ_P1588_PKGEN_CONTROLr(phy, &pkgen_ctrl));

    PHYMOD_IF_ERR_RETURN(__plp_aperta_ts_hw_set_filter_direction(phy, &pkgen_ctrl, direction));
    PHYMOD_IF_ERR_RETURN(__plp_aperta_ts_hw_set_filter_addr(phy, &pkgen_ctrl, config, index, direction));
    PHYMOD_IF_ERR_RETURN(__plp_aperta_ts_hw_set_filter_flags_action_valid(phy, &pkgen_ctrl, config, index, direction));

    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_inband_filter_get(const plp_aperta_phymod_phy_access_t* phy,
                                uint32_t direction, uint32_t index,
                                phymod_timesync_inband_filter_ctrl_t *config)
{
    int rv;
    PLP_P1588_PKGEN_CONTROLr_t pkgen_ctrl;

    PHYMOD_MEMSET(&pkgen_ctrl, 0, sizeof(P1588_PKGEN_CONTROLr_t));
    PHYMOD_IF_ERR_RETURN(PLP_READ_P1588_PKGEN_CONTROLr(phy, &pkgen_ctrl));

    rv = __plp_aperta_ts_hw_get_filter_direction(phy, &pkgen_ctrl, direction);
    if (rv == PHYMOD_E_UNAVAIL) {
        return PHYMOD_E_NONE;
    }
    PHYMOD_IF_ERR_RETURN(__plp_aperta_ts_hw_get_filter_flags_action_valid(phy, &pkgen_ctrl, config, index, direction));
    PHYMOD_IF_ERR_RETURN(__plp_aperta_ts_hw_get_filter_addr(phy, &pkgen_ctrl, config, index, direction));

    return PHYMOD_E_NONE;
}

#endif  /* no Inband Filter support fo Whitetip */

int _plp_aperta_timesync_mpls_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t direction,
                       const plp_aperta_phymod_timesync_mpls_ctrl_t *config)
{
    uint32_t  data = 0, mask = 0, reg= 0, shift = 0;
    int       lidx = 0, mpls_en = TRUE;
    P1588_MPLS_CONTROLr_t            mpls_ctrl;
    P1588_MPLS_TX_SPECIAL_LABEL_1r_t tx_spl_lable1;
    P1588_MPLS_TX_SPECIAL_LABEL_2r_t tx_spl_lable2;
    P1588_MPLS_LABEL_DIR_1r_t        label_dir1;
    P1588_MPLS_LABEL_DIR_2r_t        label_dir2;

    if ( (config->size < 0) || (TS_MPLS_LABEL_COUNT < config->size) ) {
        return PHYMOD_E_PARAM;
    }
    PHYMOD_MEMSET(&mpls_ctrl    , 0, sizeof(P1588_MPLS_CONTROLr_t));
    PHYMOD_MEMSET(&tx_spl_lable1, 0, sizeof(P1588_MPLS_TX_SPECIAL_LABEL_1r_t));
    PHYMOD_MEMSET(&tx_spl_lable2, 0, sizeof(P1588_MPLS_TX_SPECIAL_LABEL_2r_t));
    PHYMOD_MEMSET(&label_dir1   , 0, sizeof(P1588_MPLS_LABEL_DIR_1r_t));
    PHYMOD_MEMSET(&label_dir2   , 0, sizeof(P1588_MPLS_LABEL_DIR_2r_t));
    mpls_en = PHYMOD_TS_MPLS_F_ENABLE_GET(config->flags);

    /* MPLS control */
    if ( IS_TS_MPLS_DIRECTION_RX(direction) ) {
        P1588_MPLS_CONTROLr_MPLS_RX_ENf_SET(mpls_ctrl, mpls_en);
    }
    if ( IS_TS_MPLS_DIRECTION_TX(direction) ) {
        P1588_MPLS_CONTROLr_MPLS_TX_ENf_SET(mpls_ctrl, mpls_en);
        /* special label for Tx direction only */
        if ( PHYMOD_TS_MPLS_F_SPECIAL_LABEL_ENABLE_GET(config->flags) ) {
            P1588_MPLS_CONTROLr_MPLS_TX_SPECIAL_LABEL_ENf_SET(mpls_ctrl, mpls_en);
            P1588_MPLS_TX_SPECIAL_LABEL_1r_MPLS_TX_SPECIAL_LABEL_VALUE_15_0f_SET(
                         tx_spl_lable1, (config->special_label & BIT_15_00_MASK)      );
            P1588_MPLS_TX_SPECIAL_LABEL_2r_MPLS_TX_SPECIAL_LABEL_VALUE_19_16f_SET(
                         tx_spl_lable2, (config->special_label >> 16) & BIT_03_00_MASK);
            PHYMOD_IF_ERR_RETURN(    /* MPLS Special Label bit[15:00] */
                    PLP_WRITE_P1588_MPLS_TX_SPECIAL_LABEL_1r(phy, tx_spl_lable1) );
            PHYMOD_IF_ERR_RETURN(    /* MPLS Special Label bit[19:16] */
                    PLP_WRITE_P1588_MPLS_TX_SPECIAL_LABEL_2r(phy, tx_spl_lable2) );
        }
    }

    if ( PHYMOD_TS_MPLS_F_ENTROPY_ENABLE_GET(config->flags) ) {
        if ( IS_TS_MPLS_DIRECTION_RX(direction) ) {
            P1588_MPLS_CONTROLr_MPLS_RX_ENTROPY_ENf_SET(mpls_ctrl, mpls_en);
        }
        if ( IS_TS_MPLS_DIRECTION_TX(direction) ) {
            P1588_MPLS_CONTROLr_MPLS_TX_ENTROPY_ENf_SET(mpls_ctrl, mpls_en);
        }
    }
    if ( PHYMOD_TS_MPLS_F_CONTROL_WORD_ENABLE_GET(config->flags) ) {
        if ( IS_TS_MPLS_DIRECTION_RX(direction) ) {
            P1588_MPLS_CONTROLr_MPLS_RX_CONTROL_WORD_ENf_SET(mpls_ctrl, mpls_en);
        }
        if ( IS_TS_MPLS_DIRECTION_TX(direction) ) {
            P1588_MPLS_CONTROLr_MPLS_TX_CONTROL_WORD_ENf_SET(mpls_ctrl, mpls_en);
        }
    }
    PHYMOD_IF_ERR_RETURN(
            PLP_WRITE_P1588_MPLS_CONTROLr(phy, mpls_ctrl) );

    /* MPLS Label #0 - #9 */
    for ( lidx = 0; lidx < TS_MPLS_LABEL_COUNT; lidx++ ) {
        if ( lidx < config->size ) {  /* collect 4 nibbles to form the 16-bit data/mask */
            /* configure label value/mask bit[15:00]  (1588Reg.54-5D/61-6A) */
            PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_write(phy,
                                              P1588_MPLS_LABEL_VALUE_1r  + lidx,
                                              config->labels[lidx].value) );
            PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_write(phy,
                                              P1588_MPLS_LABEL_MASK_1r   + lidx,
                                              config->labels[lidx].mask ) );
        }
        /* configure label value/mask bit[19:16]  (1588Reg.5E-60/6B-6D) */
        reg   =  lidx >> 2;   /* reg=lidx/4 : label #0-3 in 1588Reg.5E/6B,          *\
                              \*              #4-7 in Reg.5F/6C, #8-10 in Reg.60/6D */
        shift = (lidx & 0x3) << 2;  /* shift 0,4,8,12 bits for corresponding labels */
        if (   (lidx & BIT_01_00_MASK) == 0x0 ) {  /* label #0,4,8 - (N&3) means (N%4) */
            /* read  1588Reg.5E/5F/60 when lidx == 0/4/8 */
            PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_read(phy,
                                              P1588_MPLS_LABEL_VALUE_11r + reg, &data) );
            /* read  1588Reg.6B/6C/6D when lidx == 0/4/8 */
            PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_read(phy,
                                              P1588_MPLS_LABEL_MASK_11r  + reg, &mask) );
        }
        if ( lidx < config->size ) {  /* collect 4 nibbles to form the 16-bit data/mask */
            data &= ~(BIT_03_00_MASK << shift);
            data |=  NIBBLE_SHIM(config->labels[lidx].value, shift);
            mask &= ~(BIT_03_00_MASK << shift);
            mask |=  NIBBLE_SHIM(config->labels[lidx].mask , shift);
        }
        if ( ((lidx &  BIT_01_00_MASK) == BIT_01_00_MASK) ||
              (lidx == TS_MPLS_LABEL_MAX) ) {
            /* write  1588Reg.5E/5F/60 when lidx == 3/7/10 */
            PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_write(phy,
                                              P1588_MPLS_LABEL_VALUE_11r + reg,  data) );
            /* write  1588Reg.6B/6C/6D when lidx == 3/7/10 */
            PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_write(phy,
                                              P1588_MPLS_LABEL_MASK_11r  + reg,  mask) );
        }
    }

    /* Configure Rx/Tx direction for label matching   (1588Reg.6E-6F) */
    data = 0x0;   /* reset data for MPLS_LABEL_DIR_1r register (1588Reg.6E) */
    for ( lidx = 0; lidx < TS_MPLS_LABEL_COUNT; lidx++ ) {
        reg   =  lidx >> 3;  /* reg=lidx/8 : #0-#7 in 1588Reg.6E, #8-#9 in 1588Reg.6F */
        shift = (lidx & 0x7) << 1;  /* shift 0,2,4,6,.. bits for corresponding labels */
        if ( lidx < config->size ) {
            if ( IS_TS_MPLS_DIRECTION_RX(direction) && mpls_en ) {
                data |= (TS_MPLS_DIRECTION_RX << shift);
            }
            if ( IS_TS_MPLS_DIRECTION_TX(direction) && mpls_en ) {
                data |= (TS_MPLS_DIRECTION_TX << shift);
            }
        }
        /* when idx is 7 or 9, write data to 1588Reg.6E for label #0 - #7 *\
        \*                                or 1588Reg.6F for label #8 - #9 */
        if ( ((lidx &  BIT_02_00_MASK) == BIT_02_00_MASK) ||
              (lidx == TS_MPLS_LABEL_MAX) ) {
            PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_write(phy,
                                              P1588_MPLS_LABEL_DIR_1r + reg,  data) );
            data = 0x0;   /* reset data for MPLS_LABEL_DIR_2r register (1588Reg.6F) */
        }
     }

    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_mpls_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t direction,
                             plp_aperta_phymod_timesync_mpls_ctrl_t *config)
{
    uint32_t  dataset = 0, maskset = 0, dirset  = 0, reg = 0, shift = 0;
    uint32_t  data_hi = 0, data_lo = 0, mask_hi = 0, mask_lo = 0;
    int       lidx = 0;
    P1588_MPLS_CONTROLr_t            mpls_ctrl;
    P1588_MPLS_TX_SPECIAL_LABEL_1r_t tx_spl_lable1;
    P1588_MPLS_TX_SPECIAL_LABEL_2r_t tx_spl_lable2;
    P1588_MPLS_LABEL_DIR_1r_t        label_dir1;
    P1588_MPLS_LABEL_DIR_2r_t        label_dir2;

    if ( (config->size < 0) || (TS_MPLS_LABEL_COUNT < config->size) ) {
        return PHYMOD_E_PARAM;
    }
    PHYMOD_MEMSET(&mpls_ctrl    , 0, sizeof(P1588_MPLS_CONTROLr_t));
    PHYMOD_MEMSET(&tx_spl_lable1, 0, sizeof(P1588_MPLS_TX_SPECIAL_LABEL_1r_t));
    PHYMOD_MEMSET(&tx_spl_lable2, 0, sizeof(P1588_MPLS_TX_SPECIAL_LABEL_2r_t));
    PHYMOD_MEMSET(&label_dir1   , 0, sizeof(P1588_MPLS_LABEL_DIR_1r_t));
    PHYMOD_MEMSET(&label_dir2   , 0, sizeof(P1588_MPLS_LABEL_DIR_2r_t));

    /* MPLS control */
    PHYMOD_IF_ERR_RETURN( PLP_READ_P1588_MPLS_CONTROLr(phy, &mpls_ctrl) );
    if ( ( IS_TS_MPLS_DIRECTION_RX(direction) &&
           P1588_MPLS_CONTROLr_MPLS_RX_ENf_GET(mpls_ctrl) ) ||
         ( IS_TS_MPLS_DIRECTION_TX(direction) &&
           P1588_MPLS_CONTROLr_MPLS_TX_ENf_GET(mpls_ctrl) ) ) {
        PHYMOD_TS_MPLS_F_ENABLE_SET(config->flags);
    }
    if ( ( IS_TS_MPLS_DIRECTION_RX(direction) &&
           P1588_MPLS_CONTROLr_MPLS_RX_ENTROPY_ENf_GET(mpls_ctrl) ) ||
         ( IS_TS_MPLS_DIRECTION_TX(direction) &&
           P1588_MPLS_CONTROLr_MPLS_TX_ENTROPY_ENf_GET(mpls_ctrl) ) ) {
        PHYMOD_TS_MPLS_F_ENTROPY_ENABLE_SET(config->flags);
    }
    if ( ( IS_TS_MPLS_DIRECTION_RX(direction) &&
           P1588_MPLS_CONTROLr_MPLS_RX_CONTROL_WORD_ENf_GET(mpls_ctrl) ) ||
         ( IS_TS_MPLS_DIRECTION_TX(direction) &&
           P1588_MPLS_CONTROLr_MPLS_TX_CONTROL_WORD_ENf_GET(mpls_ctrl) ) ) {
        PHYMOD_TS_MPLS_F_CONTROL_WORD_ENABLE_SET(config->flags);
    }

    /* MPLS Special Label */
    if ( P1588_MPLS_CONTROLr_MPLS_TX_SPECIAL_LABEL_ENf_GET(mpls_ctrl) ) {
        PHYMOD_TS_MPLS_F_SPECIAL_LABEL_ENABLE_SET(config->flags);
        PHYMOD_IF_ERR_RETURN(
                PLP_READ_P1588_MPLS_TX_SPECIAL_LABEL_2r(phy, &tx_spl_lable2) );
        PHYMOD_IF_ERR_RETURN(
                PLP_READ_P1588_MPLS_TX_SPECIAL_LABEL_1r(phy, &tx_spl_lable1) );
        config->special_label =
                 UINT20_SET( P1588_MPLS_TX_SPECIAL_LABEL_2r_VAL_HI_GET(tx_spl_lable2),
                             P1588_MPLS_TX_SPECIAL_LABEL_1r_VAL_LO_GET(tx_spl_lable1) );
    }

    /* MPLS Label #0 - #9 */
    for ( lidx = 0; lidx < TS_MPLS_LABEL_COUNT; lidx++ ) {
        /* read label value bit[15:00]  (1588Reg.54-5D) */
        PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_read(phy,
                                        P1588_MPLS_LABEL_VALUE_1r + lidx, &data_lo) );
        /* read label mask  bit[15:00]  (1588Reg.61-6A) */
        PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_read(phy,
                                        P1588_MPLS_LABEL_MASK_1r  + lidx, &mask_lo) );

        /* read label value/mask bit[19:16]  (1588Reg.5E-60/6B-6D) */
        reg   =  lidx >> 2;   /* reg=lidx/4 : label #0-3 in 1588Reg.5E/6B,          *\
                              \*              #4-7 in Reg.5F/6C, #8-10 in Reg.60/6D */
        shift = (lidx & 0x3) << 2;  /* shift 0,4,8,12 bits for corresponding labels */
        if (   (lidx & BIT_01_00_MASK) == 0x0 ) {  /* label #0,4,8 - (N&3) means (N%4) */
            /* read  1588Reg.5E/5F/60 when lidx == 0/4/8 */
            PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_read(phy,
                                        P1588_MPLS_LABEL_VALUE_11r + reg, &dataset) );
            /* read  1588Reg.6B/6C/6D when lidx == 0/4/8 */
            PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_read(phy,
                                        P1588_MPLS_LABEL_MASK_11r  + reg, &maskset) );
        }
        data_hi = (dataset & (BIT_03_00_MASK << shift)) >> shift;
        mask_hi = (maskset & (BIT_03_00_MASK << shift)) >> shift;
        config->labels[lidx].value =  UINT20_SET(data_hi, data_lo);
        config->labels[lidx].mask  =  UINT20_SET(mask_hi, mask_lo);

        reg   =  lidx >> 3;  /* reg=lidx/8 : #0-#7 in 1588Reg.6E, #8-#9 in 1588Reg.6F */
        shift = (lidx & 0x7) << 1;  /* shift 0,2,4,6,.. bits for corresponding labels */
        if (   (lidx & BIT_02_00_MASK) == 0x0 ) {   /* label #0,8 - (N&7) means (N%8) */
            /* read  1588Reg.6E/6F when lidx == 0 or 8 */
            PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_read(phy,
                                        P1588_MPLS_LABEL_DIR_1r + reg, &dirset) );
        }
        config->labels[lidx].flags |=  (dirset & (BIT_01_00_MASK << shift)) >> shift;
    }

    return PHYMOD_E_NONE;
}

#if defined(PHYMOD_XGBASET_SUPPORT) || defined(PHYMOD_MGAUTO_SUPPORT)
    /* utility to enable IEEE1588 clock & crystal */
int __timesync_xgbaset_clock_xtal_1588_enable(const plp_aperta_phymod_phy_access_t *phy,
                                              uint32_t port_offset, int en1588, int ptp_refclk ) {
    int                   rv = PHYMOD_E_NONE, is_internal_dpll_clk = FALSE;
    uint32_t              xtalctl, clken;
    P1588_PIM_ENABLEr_t   pim;
    P1588_PIM_ENABLEr_CLR(pim);

    /* Global Clock Enable Control Register  30.0x8002/8802[1] CLKEN_CLK_1G_1588   */
    PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_READ(ACCP(phy), CRG_CHIP_CLKEN_CTLr, &clken) );
    /* Chip Misc XTAL PTP Control 2 Register 30.0x8111/8911[0]: XTAL Power Down/UP */
    PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_READ(ACCP(phy), CHIP_MISC_CFG_XTAL_PTP_CTRL2r, &xtalctl) );
    is_internal_dpll_clk = (( clken  & CRG_CHIP_CLKEN_CTL_CLK_1G_MASK) == 0x0U)  ||
                           ((xtalctl & CHIP_MISC_CFG_XTAL_PTP_CTRL2_PWRDOWN_MODE) != 0x0U);
    /* if  attempting to enable 1588, but already in                                           *\
    |*       external PTP_XTAL reference clock source mode  ==>> DO NOTHING                    *|
    \* else (in internal DPLL clock mode or disabling 1588) ==>> Setting based on 'ptp_refclk' */
    if ( is_internal_dpll_clk || (en1588 == TIMESYNC_DISABLE) ) {
        if ( PHYMOD_TS_F_CLOCK_SRC_EXT_GET(ptp_refclk) ) {
            xtalctl |=  (CHIP_MISC_CFG_XTAL_PTP_CTRL2_PWRDOWN_MODE);
            clken   &= ~(CRG_CHIP_CLKEN_CTL_CLK_1G_MASK);
        } else {
            xtalctl &= ~(CHIP_MISC_CFG_XTAL_PTP_CTRL2_PWRDOWN_MODE);
            clken   |=  (CRG_CHIP_CLKEN_CTL_CLK_1G_MASK);
        }
        PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_WRITE(ACCP(phy), CHIP_MISC_CFG_XTAL_PTP_CTRL2r, xtalctl) );
        PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_WRITE(ACCP(phy), CRG_CHIP_CLKEN_CTLr,  clken) );
    }

    /* PIM mode slice enable register  1588Reg.0xE2[7:6] disable TX 1588/PCH bypass */
    if ( en1588 ) {
        PHYMOD_IF_ERR_RETURN( READ_P1588_PIM_ENABLEr( phy, &pim) );
        P1588_PIM_ENABLEr_TXRX_1588PCH_BYPASS_SET(pim, 0x0);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_PIM_ENABLEr(phy,  pim) );
    }

  #if defined(__CHIP_MISC_CFG_XTAL_PTP_CTRL1_REG_SETTING__)
    /* Chip Misc XTAL PTP Control 1 Register  30.0x8110                        */
    PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_READ( ACCP(phy), CHIP_MISC_CFG_XTAL_PTP_CTRL1r, &xtalctl) );
    if ( (en1588 == DISABLE) || (xtalctl != CHIP_MISC_CFG_XTAL_PTP_CTRL1_PTP_REFCLK) ) {
        xtalctl = PHYMOD_TS_F_CLOCK_SRC_EXT_GET(ptp_refclk) ? CHIP_MISC_CFG_XTAL_PTP_CTRL1_PTP_REFCLK
                                                            : CHIP_MISC_CFG_XTAL_PTP_CTRL1_RESET_VALUE;
        PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_WRITE( ACCP(phy) ,   CHIP_MISC_CFG_XTAL_PTP_CTRL1r, xtalctl) );
    }
  #endif
  #if defined(PHYMOD_SHORTFIN_SUPPORT) || defined(PHYMOD_BROADFIN_SUPPORT) || defined(PHYMOD_KAUAI_SUPPORT)
    /* Shortfin & Broadfin only :                                             *\
    \* above settings need to be done on P0 (the 1st port of this chip) also  */
    if ( port_offset != 0 ) {
        PHYMOD_IF_ERR_RETURN(
                PHYMOD_BUS_READ(ACCP(phy), CORE_CFG_HW_STRAPr, &port_offset) );
        /* calculate the offset between P0 and this port */
        port_offset  =      (port_offset & CORE_CFG_HW_STRAP_COREID_MASK)
                                        >> CORE_CFG_HW_STRAP_COREID_SHIFT;
        /* the current port is not P0, do the same settings on P0 */
        if ( port_offset > 0 ) {   /* the current port is not P0, do the same settings on P0 */
            plp_aperta_phymod_phy_access_t  phy_leader = { 0 };      /* PHY Access datastructure for P0 */
            PHYMOD_MEMCPY(&phy_leader, phy, sizeof(plp_aperta_phymod_phy_access_t));
            phy_leader.access.addr -=  port_offset;    /* assign P0's PHY address */
            /* recursively call this function for P0 */
            rv = __timesync_xgbaset_clock_xtal_1588_enable(&phy_leader, 0, en1588, ptp_refclk);
        }
    }
  #endif  /* Shortfin or Broadfin or Kauai */

    return  rv;
}
#endif  /* PHYMOD_XGBASET_SUPPORT || PHYMOD_MGAUTO_SUPPORT */


int _plp_aperta_timesync_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flag, uint32_t enable)
{
    P1588_SLICE_ENABLE_CONTROLr_t   slice_en;
    P1588_SLICE_ENABLE_CONTROLr_CLR(slice_en);

    if ( PHYMOD_TS_ENABLE_CTRL_FLAG_GET(flag) ) {
        /* set individual Enable Control items */
        P1588_SLICE_ENABLE_CONTROLr_SET(slice_en, enable);
    }
    else {
        int  en_dis = (enable) ? TRUE : FALSE;
        /* read P1588M_slice_enable_control register */
        PHYMOD_IF_ERR_RETURN( READ_P1588_SLICE_ENABLE_CONTROLr(phy, &slice_en) );

        /* check the flags: Tx / Rx / NSE_Timer */
        if ( (flag & (PHYMOD_TS_ENABLE_CTRL_RX_SLICE_1588_EN |
                      PHYMOD_TS_ENABLE_CTRL_TX_SLICE_1588_EN |
                      PHYMOD_TS_ENABLE_CTRL_NSE_TIMER_CLK_EN ))
                   == PHYMOD_TS_ENABLE_CTRL_NSE_TIMER_CLK_EN   ) {
            /* Tx/Rx flags are not set, only the NSE_Timer flag.  So               *\
            \* enable/disable NSE Timer Clock only, do no touch P1588 Rx/Tx slices */
        }
        else {   /* enable/disable TimeSync 1588 Tx/Rx slices */
            /* if flag is neither TX nor RX, defaults to both Tx/Rx */
            /* if flag==TX set Tx only,  otherwise set Rx, too     */
            if ( PHYMOD_TS_DIRECTION_RX_GET(flag) || (! PHYMOD_TS_DIRECTION_TX_GET(flag)) ) {
                P1588_SLICE_ENABLE_CONTROLr_RX_SLICE_1588_ENf_SET(slice_en, en_dis);
            }
            /* if flag==RX set Rx only,  otherwise set Tx, too     */
            if ( PHYMOD_TS_DIRECTION_TX_GET(flag) || (! PHYMOD_TS_DIRECTION_RX_GET(flag)) ) {
                P1588_SLICE_ENABLE_CONTROLr_TX_SLICE_1588_ENf_SET(slice_en, en_dis);
            }
        }
        /* enable/disable NSE Timer Clock */
        P1588_SLICE_ENABLE_CONTROLr_NSE_TIMER_CLOCK_ENABLEf_SET(slice_en, en_dis);
    }

  #if defined(PHYMOD_XGBASET_SUPPORT) || defined(PHYMOD_MGAUTO_SUPPORT)
        /* clock & crystal for P1588 engine */
        PHYMOD_IF_ERR_RETURN( __timesync_xgbaset_clock_xtal_1588_enable(phy, PORTS_PER_PHY_CHIP_MAX,
                                                     enable, (enable & PHYMOD_TS_F_CLOCK_SRC_EXT)) );
  #elif defined(PHYMOD_QUADRA28_SUPPORT)
  {            /* set Retimer mode to make IEEE1588 feature work */
    uint32_t  v16 = 0, wait_sts = 0;
    PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_READ( ACCP(phy), Q28_SERDES_RETIMER_9_REG, &v16) );
    PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_WRITE(ACCP(phy), Q28_SERDES_RETIMER_9_REG,
                                                      Q28_SERDES_RETIMER_9_VAL | v16) );
    /* check PMD STATUS */
    for ( wait_sts = 0; wait_sts <  QUADRA28_PMD_STATUS_WAIT_TIME_US;
                        wait_sts += QUADRA28_PMD_STATUS_WAIT_SLEEP_US ) {
        PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_READ( ACCP(phy), QUADRA28_PMD_STATUS_REG, &v16) );
        if ( QUADRA28_PMD_STATUS_VAL == (v16 & QUADRA28_PMD_STATUS_VAL) ) {
            break;   /* PMD is in READY status, Reg.0xC804 == 0xDD or 0xDF */
        }
        PHYMOD_USLEEP(QUADRA28_PMD_STATUS_WAIT_SLEEP_US);  /* sleep 10ms */
    }
    if ( wait_sts >= QUADRA28_PMD_STATUS_WAIT_TIME_US ) {
        return  PHYMOD_E_TIMEOUT;
    }
    v16 = 0;
    PHYMOD_IF_ERR_RETURN(PHYMOD_BUS_READ( ACCP(phy), Q28_SERDES_RETIMER_8_REG, &v16));
    /* Trigger Firmware to apply the configuration */
    PHYMOD_IF_ERR_RETURN(PHYMOD_BUS_WRITE(ACCP(phy), Q28_SERDES_RETIMER_8_REG,
                                               v16 | Q28_SERDES_RETIMER_8_APPLY_CFG));

    PHYMOD_IF_ERR_RETURN(PHYMOD_BUS_WRITE(ACCP(phy), Q28_SERDES_PLL_CTRL4_REG,
                                                     Q28_SERDES_PLL_CTRL4_VAL));
  }
  #endif

    /* write P1588M_slice_enable_control register */
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_SLICE_ENABLE_CONTROLr(phy, slice_en) );

    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags, uint32_t* enable)
{
    P1588_SLICE_ENABLE_CONTROLr_t   slice_en;
    P1588_SLICE_ENABLE_CONTROLr_CLR(slice_en);

    PHYMOD_IF_ERR_RETURN( READ_P1588_SLICE_ENABLE_CONTROLr(phy, &slice_en) );

    if ( PHYMOD_TS_ENABLE_CTRL_FLAG_GET(flags) ) {
        /* get individual Enable Control items */
        *enable = P1588_SLICE_ENABLE_CONTROLr_GET(slice_en);

    } else {        /* enable/disable status of TimeSync 1588 Tx/Rx slices */
        if ( PHYMOD_TS_DIRECTION_TX_GET(flags) ) {
            /* TX*/
            *enable = P1588_SLICE_ENABLE_CONTROLr_TX_SLICE_1588_ENf_GET(slice_en);
        } else {
            /* Defaults to RX*/
            *enable = P1588_SLICE_ENABLE_CONTROLr_RX_SLICE_1588_ENf_GET(slice_en);
        }
    }

    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_nco_addend_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t freq_step)
{
    P1588_NSE_NCO_1r_t  nco_1;
    P1588_NSE_NCO_2r_t  nco_2;

    PHYMOD_MEMSET(&nco_1, 0, sizeof(P1588_NSE_NCO_1r_t));
    PHYMOD_MEMSET(&nco_2, 0, sizeof(P1588_NSE_NCO_2r_t));
    P1588_NSE_NCO_2r_NSE_REG_NCO_FREQCNTRL_15_00f_SET(nco_2,  freq_step & BIT_15_00_MASK);
    P1588_NSE_NCO_1r_NSE_REG_NCO_FREQCNTRL_31_16f_SET(nco_1, (freq_step >> 16) & BIT_15_00_MASK);

    PHYMOD_IF_ERR_RETURN( WRITE_P1588_NSE_NCO_1r(phy, nco_1) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_NSE_NCO_2r(phy, nco_2) );
    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_nco_addend_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* freq_step)
{
    P1588_NSE_NCO_1r_t  nco_1;
    P1588_NSE_NCO_2r_t  nco_2;

    PHYMOD_MEMSET(&nco_1, 0, sizeof(P1588_NSE_NCO_1r_t));
    PHYMOD_MEMSET(&nco_2, 0, sizeof(P1588_NSE_NCO_2r_t));
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_NCO_1r(phy, &nco_1) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_NCO_2r(phy, &nco_2) );

    *freq_step  =  P1588_NSE_NCO_2r_NSE_REG_NCO_FREQCNTRL_15_00f_GET(nco_2) |
                  (P1588_NSE_NCO_1r_NSE_REG_NCO_FREQCNTRL_31_16f_GET(nco_1) << 16);
    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_framesync_mode_set(const plp_aperta_phymod_phy_access_t* phy,
                                 const plp_aperta_phymod_timesync_framesync_t* framesync)
{
    uint32_t  regval = 0x0U, use_sync0 = 0x0U, mode_sync0 = 0x0U, cpu_sync0 = 0x0U;
    P1588_NSE_SC_8r_t         nse_sc_8;
    P1588_NSE_SC_9r_t         nse_sc_9;
    P1588_NSE_SC_10r_t        nse_sc_10;
    P1588_PIM_NSE_TIME_5r_t   gen2_nes_ctrl;

    P1588_PIM_NSE_TIME_5r_CLR(gen2_nes_ctrl);
    P1588_NSE_SC_10r_CLR(     nse_sc_10    );
    P1588_NSE_SC_9r_CLR(      nse_sc_9     );
    P1588_NSE_SC_8r_CLR(      nse_sc_8     );

    P1588_NSE_SC_10r_NSE_REG_SC_SYNCIN_CNTRL_15_0f_SET(nse_sc_10, framesync->length_threshold);
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_NSE_SC_10r(phy , nse_sc_10) );
    P1588_NSE_SC_9r_NSE_REG_SC_SYNCIN_CNTRL_31_16f_SET(nse_sc_9 , framesync->event_offset);
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_NSE_SC_9r( phy , nse_sc_9)  );

    nse_sc_8._p1588_nse_sc_8 = framesync->flags & BIT_13_06_MASK;
    P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_1_0f_SET(  nse_sc_8, framesync->syncout_mode);
    regval = (phymodTimesyncGLobalModeFree   == framesync->gmode) ? 0x1U :
             (phymodTimesyncGLobalModeSyncIn == framesync->gmode) ? 0x2U :
             (phymodTimesyncGLobalModeCpu    == framesync->gmode) ? 0x3U : 0x0U;
    P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_15_14f_SET(nse_sc_8, regval );
    regval = (phymodTimesyncFramsyncModeSyncIn0 == framesync->mode ) ? 0x1U :
             (phymodTimesyncFramsyncModeSyncIn1 == framesync->mode ) ? 0x2U :
             (phymodTimesyncFramsyncModeSyncOut == framesync->mode ) ? 0x4U :
             (phymodTimesyncFramsyncModeCpu     == framesync->mode ) ? 0x8U : 0x0U;
    P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_5_2f_SET(  nse_sc_8, regval );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_NSE_SC_8r(phy ,   nse_sc_8) );

    switch ( framesync->mode ) {
    case  phymodTimesyncFramsyncModeSyncIn0 :
        /*  set  PIM_NSE_Time_5 (1588Reg.0xF0) if use Sync0 as the FrameSync */
        cpu_sync0  =  FALSE;
        use_sync0  =  TRUE ;
        mode_sync0 =  P1588_PIM_NSE_GEN2_MODE_3   ;
        break;
    case  phymodTimesyncFramsyncModeCpu :
        /*  set  PIM_NSE_Time_5 (1588Reg.0xF0) bit[6] to emulate FrameSync   */
        cpu_sync0  =  TRUE ;
        use_sync0  =  TRUE ;
        mode_sync0 =  P1588_PIM_NSE_GEN2_MODE_IDLE;
        break;
    default :   /* phymodTimesyncFramsyncModeSyncIn1 or phymodTimesyncFramsyncModeNone */
        /* clear PIM_NSE_Time_5 (1588Reg.0xF0) if use Sync1 as the FrameSync */
        cpu_sync0  =  FALSE;
        use_sync0  =  FALSE;
        mode_sync0 =  P1588_PIM_NSE_GEN2_MODE_IDLE;
    }
    P1588_PIM_NSE_TIME_5r_CPU_EMULATE_SYNC0f_SET(gen2_nes_ctrl, cpu_sync0 );
    P1588_PIM_NSE_TIME_5r_ENABLE_TS_CAPTUREf_SET(gen2_nes_ctrl, use_sync0 );
    P1588_PIM_NSE_TIME_5r_ENABLE_NSE_GEN2f_SET(  gen2_nes_ctrl, use_sync0 );
    P1588_PIM_NSE_TIME_5r_NSE_GEN2_MODEf_SET(    gen2_nes_ctrl, mode_sync0);
    /* write PIM_NSE_Time_5 register (1588Reg.0xF0) */
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_PIM_NSE_TIME_5r(phy, gen2_nes_ctrl) );

    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_framesync_mode_get(const plp_aperta_phymod_phy_access_t* phy,
                                 plp_aperta_phymod_timesync_framesync_t* framesync)
{
    uint32_t  regval = 0x0U;
    P1588_NSE_SC_8r_t         nse_sc_8;
    P1588_NSE_SC_9r_t         nse_sc_9;
    P1588_NSE_SC_10r_t        nse_sc_10;
    P1588_PIM_NSE_TIME_5r_t   gen2_nes_ctrl;

    P1588_PIM_NSE_TIME_5r_CLR(gen2_nes_ctrl);
    P1588_NSE_SC_10r_CLR(     nse_sc_10    );
    P1588_NSE_SC_9r_CLR(      nse_sc_9     );
    P1588_NSE_SC_8r_CLR(      nse_sc_8     );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_SC_8r(      phy, &nse_sc_8     ) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_SC_9r(      phy, &nse_sc_9     ) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_SC_10r(     phy, &nse_sc_10    ) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_PIM_NSE_TIME_5r(phy, &gen2_nes_ctrl) );

    framesync->length_threshold = P1588_NSE_SC_10r_NSE_REG_SC_SYNCIN_CNTRL_15_0f_GET(nse_sc_10);
    framesync->event_offset = P1588_NSE_SC_9r_NSE_REG_SC_SYNCIN_CNTRL_31_16f_GET( nse_sc_9);

    framesync->flags        = nse_sc_8._p1588_nse_sc_8 & BIT_13_06_MASK;
    framesync->syncout_mode = P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_1_0f_GET(  nse_sc_8);
    regval                  = P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_15_14f_GET(nse_sc_8);
    framesync->gmode        = (0x1U == regval) ? phymodTimesyncGLobalModeFree   :
                              (0x2U == regval) ? phymodTimesyncGLobalModeSyncIn :
                              (0x3U == regval) ? phymodTimesyncGLobalModeCpu    :
                                                 phymodTimesyncGLobalModeCount  ;

    /* check PIM_NSE_Time_5 (1588Reg.0xF0) to see if using Sync0 as FrameSync */
    if ( (P1588_PIM_NSE_TIME_5r_ENABLE_NSE_GEN2f_GET(gen2_nes_ctrl) != 0x0U) &&
         (P1588_PIM_NSE_TIME_5r_NSE_GEN2_MODEf_GET(  gen2_nes_ctrl) ==
                                                     P1588_PIM_NSE_GEN2_MODE_3) )  {
        framesync->mode =                   phymodTimesyncFramsyncModeSyncIn0 ;
    }
    else {  /* determine FrameSync mode by checking NSE_SC_8 (1588Reg.0x3A[5:2] */
        uint16_t  temp = P1588_NSE_SC_8r_NSE_REG_SC_SYNMODE_CNTRL_5_2f_GET(nse_sc_8);
        framesync->mode = (0x1u == temp ) ? phymodTimesyncFramsyncModeSyncIn0 :
                          (0x2U == temp ) ? phymodTimesyncFramsyncModeSyncIn1 :
                          (0x4U == temp ) ? phymodTimesyncFramsyncModeSyncOut :
                          (0x8U == temp ) ? phymodTimesyncFramsyncModeCpu     :
                                            phymodTimesyncFramsyncModeNone    ;
    }

    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_local_time_set(const plp_aperta_phymod_phy_access_t* phy, uint64_t local_time)
{
    P1588_NSE_NCO_3r_t nse_nco3;
    P1588_NSE_NCO_4r_t nse_nco4;
    P1588_NSE_NCO_5r_t nse_nco5;
    uint32_t local_time_temp = 0;

    PHYMOD_MEMSET(&nse_nco3, 0, sizeof(P1588_NSE_NCO_3r_t));
    PHYMOD_MEMSET(&nse_nco4, 0, sizeof(P1588_NSE_NCO_4r_t));
    PHYMOD_MEMSET(&nse_nco5, 0, sizeof(P1588_NSE_NCO_5r_t));

    local_time_temp = COMPILER_64_LO(local_time);
    P1588_NSE_NCO_5r_NSE_REG_NCO_LT_CNTRL_15_0f_SET(nse_nco5,   (local_time_temp & BIT_15_00_MASK));
    PHYMOD_IF_ERR_RETURN(
            WRITE_P1588_NSE_NCO_5r(phy, nse_nco5));

    P1588_NSE_NCO_4r_NSE_REG_NCO_LT_CNTRL_31_16f_SET(nse_nco4, ((local_time_temp & BIT_31_16_MASK) >> 16));
    PHYMOD_IF_ERR_RETURN(
            WRITE_P1588_NSE_NCO_4r(phy, nse_nco4));

    local_time_temp = COMPILER_64_HI(local_time);

    P1588_NSE_NCO_3r_NSE_REG_NCO_LT_CNTRL_47_32f_SET(nse_nco3,  (local_time_temp & BIT_15_00_MASK));
    PHYMOD_IF_ERR_RETURN(
            WRITE_P1588_NSE_NCO_3r(phy, nse_nco3));

    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_local_time_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* local_time)
{
    P1588_NSE_NCO_3r_t nse_nco3;
    P1588_NSE_NCO_4r_t nse_nco4;
    P1588_NSE_NCO_5r_t nse_nco5;
    uint32_t           local_time_low32 = 0;

    PHYMOD_MEMSET(&nse_nco3, 0, sizeof(P1588_NSE_NCO_3r_t));
    PHYMOD_MEMSET(&nse_nco4, 0, sizeof(P1588_NSE_NCO_4r_t));
    PHYMOD_MEMSET(&nse_nco5, 0, sizeof(P1588_NSE_NCO_5r_t));

    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_NCO_5r(phy, &nse_nco5) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_NCO_4r(phy, &nse_nco4) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_NSE_NCO_3r(phy, &nse_nco3) );
    local_time_low32  = P1588_NSE_NCO_5r_NSE_REG_NCO_LT_CNTRL_15_0f_GET(nse_nco5);
    local_time_low32 |= (P1588_NSE_NCO_4r_NSE_REG_NCO_LT_CNTRL_31_16f_GET(nse_nco4) << 16);
    COMPILER_64_SET(*local_time, P1588_NSE_NCO_3r_NSE_REG_NCO_LT_CNTRL_47_32f_GET(nse_nco3),
                                 local_time_low32);

    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_load_ctrl_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t load_once, uint32_t load_always)
{
    P1588_REGS_SHADOW_CONTROL_1r_t load;
    P1588_REGS_SHADOW_CONTROL_2r_t always_load;
    unsigned int load_reset = 0, load_set = 0;

    PHYMOD_MEMSET(&load, 0, sizeof(P1588_REGS_SHADOW_CONTROL_1r_t));
    PHYMOD_MEMSET(&always_load, 0, sizeof(P1588_REGS_SHADOW_CONTROL_2r_t));

    /* load once (reset) */
    if (PHYMOD_TS_LDCTL_TN_LOAD_GET(load_once)) {
        load_reset |= 1 << 11;
    }
    if (PHYMOD_TS_LDCTL_TIMECODE_LOAD_GET(load_once)) {
        load_reset |= 1 << 10;
    }
    if (PHYMOD_TS_LDCTL_SYNCOUT_LOAD_GET(load_once)) {
        load_reset |= 1 << 9;
    }
    if (PHYMOD_TS_LDCTL_NCO_DIVIDER_LOAD_GET(load_once)) {
        load_reset |= 1 << 8;
    }
    if (PHYMOD_TS_LDCTL_LOCAL_TIME_LOAD_GET(load_once)) {
        load_reset |= 1 << 7;
    }
    if (PHYMOD_TS_LDCTL_NCO_ADDEND_LOAD_GET(load_once)) {
        load_reset |= 1 << 6;
    }
    if (PHYMOD_TS_LDCTL_DPLL_LOOP_FILTER_LOAD_GET(load_once)) {
        load_reset |= 1 << 5;
    }
    if (PHYMOD_TS_LDCTL_DPLL_REF_PHASE_LOAD_GET(load_once)) {
        load_reset |= 1 << 4;
    }
    if (PHYMOD_TS_LDCTL_DPLL_REF_PHASE_DELTA_LOAD_GET(load_once)) {
        load_reset |= 1 << 3;
    }
    if (PHYMOD_TS_LDCTL_DPLL_K3_LOAD_GET(load_once)) {
        load_reset |= 1 << 2;
    }
    if (PHYMOD_TS_LDCTL_DPLL_K2_LOAD_GET(load_once)) {
        load_reset |= 1 << 1;
    }
    if (PHYMOD_TS_LDCTL_DPLL_K1_LOAD_GET(load_once)) {
        load_reset |= 1;
    }

    /* load always (set) */
    if (PHYMOD_TS_LDCTL_TN_LOAD_GET(load_always)) {
        load_set |= 1 << 11;
    }
    if (PHYMOD_TS_LDCTL_TIMECODE_LOAD_GET(load_always)) {
        load_set |= 1 << 10;
    }
    if (PHYMOD_TS_LDCTL_SYNCOUT_LOAD_GET(load_always)) {
        load_set |= 1 << 9;
    }
    if (PHYMOD_TS_LDCTL_NCO_DIVIDER_LOAD_GET(load_always)) {
        load_set |= 1 << 8;
    }
    if (PHYMOD_TS_LDCTL_LOCAL_TIME_LOAD_GET(load_always)) {
        load_set |= 1 << 7;
    }
    if (PHYMOD_TS_LDCTL_NCO_ADDEND_LOAD_GET(load_always)) {
        load_set |= 1 << 6;
    }
    if (PHYMOD_TS_LDCTL_DPLL_LOOP_FILTER_LOAD_GET(load_always)) {
        load_set |= 1 << 5;
    }
    if (PHYMOD_TS_LDCTL_DPLL_REF_PHASE_LOAD_GET(load_always)) {
        load_set |= 1 << 4;
    }
    if (PHYMOD_TS_LDCTL_DPLL_REF_PHASE_DELTA_LOAD_GET(load_always)) {
        load_set |= 1 << 3;
    }
    if (PHYMOD_TS_LDCTL_DPLL_K3_LOAD_GET(load_always)) {
        load_set |= 1 << 2;
    }
    if (PHYMOD_TS_LDCTL_DPLL_K2_LOAD_GET(load_always)) {
        load_set |= 1 << 1;
    }
    if (PHYMOD_TS_LDCTL_DPLL_K1_LOAD_GET(load_always)) {
        load_set |= 1;
    }
    P1588_REGS_SHADOW_CONTROL_1r_NSE_REG_CTL_31_16f_SET(load, load_reset);
    P1588_REGS_SHADOW_CONTROL_2r_NSE_REG_CTL_15_0f_SET(always_load, load_set);

    PHYMOD_IF_ERR_RETURN( WRITE_P1588_REGS_SHADOW_CONTROL_1r(phy, load) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_REGS_SHADOW_CONTROL_2r(phy, always_load) );

    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_load_ctrl_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t* load_once, uint32_t* load_always)
{
    P1588_REGS_SHADOW_CONTROL_1r_t load;
    P1588_REGS_SHADOW_CONTROL_2r_t always_load;

    PHYMOD_MEMSET(&load, 0, sizeof(P1588_REGS_SHADOW_CONTROL_1r_t));
    PHYMOD_MEMSET(&always_load, 0, sizeof(P1588_REGS_SHADOW_CONTROL_2r_t));
    PHYMOD_IF_ERR_RETURN( READ_P1588_REGS_SHADOW_CONTROL_1r(phy, &load) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_REGS_SHADOW_CONTROL_2r(phy, &always_load) );
    *load_once =  *load_always =  0;

    if (load._p1588_regs_shadow_control_1 & (1 << 11)) {
        PHYMOD_TS_LDCTL_TN_LOAD_SET(*load_once);
    }
    if (load._p1588_regs_shadow_control_1 & (1 << 10)) {
        PHYMOD_TS_LDCTL_TIMECODE_LOAD_SET(*load_once);
    }
    if (load._p1588_regs_shadow_control_1 & (1 << 9)) {
        PHYMOD_TS_LDCTL_SYNCOUT_LOAD_SET(*load_once);
    }
    if (load._p1588_regs_shadow_control_1 & (1 << 8)) {
        PHYMOD_TS_LDCTL_NCO_DIVIDER_LOAD_SET(*load_once);
    }
    if (load._p1588_regs_shadow_control_1 & (1 << 7)) {
        PHYMOD_TS_LDCTL_LOCAL_TIME_LOAD_SET(*load_once);
    }
    if (load._p1588_regs_shadow_control_1 & (1 << 6)) {
        PHYMOD_TS_LDCTL_NCO_ADDEND_LOAD_SET(*load_once);
    }
    if (load._p1588_regs_shadow_control_1 & (1 << 5)) {
        PHYMOD_TS_LDCTL_DPLL_LOOP_FILTER_LOAD_SET(*load_once);
    }
    if (load._p1588_regs_shadow_control_1 & (1 << 4)) {
        PHYMOD_TS_LDCTL_DPLL_REF_PHASE_LOAD_SET(*load_once);
    }
    if (load._p1588_regs_shadow_control_1 & (1 << 3)) {
        PHYMOD_TS_LDCTL_DPLL_REF_PHASE_DELTA_LOAD_SET(*load_once);
    }
    if (load._p1588_regs_shadow_control_1 & (1 << 2)) {
        PHYMOD_TS_LDCTL_DPLL_K3_LOAD_SET(*load_once);
    }
    if (load._p1588_regs_shadow_control_1 & (1 << 1)) {
        PHYMOD_TS_LDCTL_DPLL_K2_LOAD_SET(*load_once);
    }
    if (load._p1588_regs_shadow_control_1 & 1) {
        PHYMOD_TS_LDCTL_DPLL_K1_LOAD_SET(*load_once);
    }

    if (always_load._p1588_regs_shadow_control_2 & (1 << 11)) {
        PHYMOD_TS_LDCTL_TN_LOAD_SET(*load_always);
    }
    if (always_load._p1588_regs_shadow_control_2 & (1 << 10)) {
        PHYMOD_TS_LDCTL_TIMECODE_LOAD_SET(*load_always);
    }
    if (always_load._p1588_regs_shadow_control_2 & (1 << 9)) {
        PHYMOD_TS_LDCTL_SYNCOUT_LOAD_SET(*load_always);
    }
    if (always_load._p1588_regs_shadow_control_2 & (1 << 8)) {
        PHYMOD_TS_LDCTL_NCO_DIVIDER_LOAD_SET(*load_always);
    }
    if (always_load._p1588_regs_shadow_control_2 & (1 << 7)) {
        PHYMOD_TS_LDCTL_LOCAL_TIME_LOAD_SET(*load_always);
    }
    if (always_load._p1588_regs_shadow_control_2 & (1 << 6)) {
        PHYMOD_TS_LDCTL_NCO_ADDEND_LOAD_SET(*load_always);
    }
    if (always_load._p1588_regs_shadow_control_2 & (1 << 5)) {
        PHYMOD_TS_LDCTL_DPLL_LOOP_FILTER_LOAD_SET(*load_always);
    }
    if (always_load._p1588_regs_shadow_control_2 & (1 << 4)) {
        PHYMOD_TS_LDCTL_DPLL_REF_PHASE_LOAD_SET(*load_always);
    }
    if (always_load._p1588_regs_shadow_control_2 & (1 << 3)) {
        PHYMOD_TS_LDCTL_DPLL_REF_PHASE_DELTA_LOAD_SET(*load_always);
    }
    if (always_load._p1588_regs_shadow_control_2 & (1 << 2)) {
        PHYMOD_TS_LDCTL_DPLL_K3_LOAD_SET(*load_always);
    }
    if (always_load._p1588_regs_shadow_control_2 & (1 << 1)) {
        PHYMOD_TS_LDCTL_DPLL_K2_LOAD_SET(*load_always);
    }
    if (always_load._p1588_regs_shadow_control_2 & 1) {
        PHYMOD_TS_LDCTL_DPLL_K1_LOAD_SET(*load_always);
    }

    return PHYMOD_E_NONE;
}


/* Timing control regisers searching table */
#define  P1588_TIMING_REG_TAB    { \
           { PLP_P1588_PIM_NSE_TIME_5r     , PLP_P1588_PIM_NSE_TIME_4r     }, /* 1588Reg.0xF0-EF */ \
           { PLP_P1588_PIM_NSE_TIME_3r     , PLP_P1588_PIM_NSE_TIME_0r     }, /* 1588Reg.0xEE-EB */ \
           { P1588_NTP_Down_Counter_Ctrl_20, P1588_NTP_Down_Counter_Ctrl_0 }, /* 1588Reg.0xAF-AE */ \
           { P1588_NTP_Freq_Ctrl_Word_19   , P1588_NTP_Freq_Ctrl_Word_0    }, /* 1588Reg.0xA9-A8 */ \
           { PLP_P1588_NSE_NCO_3r          , PLP_P1588_NSE_NCO_5r          }, /* 1588Reg.0x30-32 */ \
           { P1588_NCO_Freq_Step_Ctrl_31   , P1588_NCO_Freq_Step_Ctrl_0    }, /* 1588Reg.0x2E-2F */ \
           { P1588_DPLL_Init_LoopFilter_63 , P1588_DPLL_Init_LoopFilter_0  }, /* 1588Reg.0x29-2C */ \
           { P1588_DPLL_K3                 , P1588_DPLL_K1 },  /* DPLL K1/K2/K3  1588Reg.0x28-26 */ \
           { P1588_DPLL_Ref_PhaseDelta_31  , P1588_DPLL_Ref_PhaseDelta_0   }, /* 1588Reg.0x24-25 */ \
           { P1588_DPLL_Ref_Phase_47       , P1588_DPLL_Ref_Phase_0        }, /* 1588Reg.0x21-23 */ \
                            { FALSE, FALSE }          }
#define  P1588_TIMING_VALUE_LEN_MAX     4

typedef struct {
    uint32_t reg4msb, reg4lsb;
} timing_reg_t;

/* distribute a 64-bit value into four 16-bit array elements */
void __plp_aperta_allot_u64_into_u16array(uint64_t u64, uint16_t u16[]) {
    /* u16[3]:bit_63_48  u16[2]:bit_47_32  u16[1]:bit_31_16  u16[0]:bit_15_0 */
    u16[3] = (uint16_t) ((COMPILER_64_HI(u64) >> 16) & BIT_15_00_MASK);
    u16[2] = (uint16_t) ((COMPILER_64_HI(u64) >>  0) & BIT_15_00_MASK);
    u16[1] = (uint16_t) ((COMPILER_64_LO(u64) >> 16) & BIT_15_00_MASK);
    u16[0] = (uint16_t) ((COMPILER_64_LO(u64) >>  0) & BIT_15_00_MASK);
}

#if defined(PHYMOD_EVORA_SUPPORT) || defined(PHYMOD_EUROPA_SUPPORT)  \
                                  || defined(PHYMOD_APERTA_SUPPORT)

/* send FW messages for HSIP PHYs */
int  __plp_aperta_timesync_hsip_fws_fwmsg_send(const plp_aperta_phymod_phy_access_t* phy,
                                    uint32_t fwmsg, uint64_t value64) {
    if ( bcmplpTimesyncTimerTypeTimeOfDay48 == fwmsg ) {   /* ToD update through FW */
        int       ii   = 0;
        uint32    vout = 0, wait_fwmsg = 0;
        uint16_t  data[P1588_TIMING_VALUE_LEN_MAX] = { 0, 0, 0, 0 };
        uint8     port_num = 0;
        HSIP_GET_PORT_FROM_LM(phy->access.lane_mask, port_num);

        /* distribute 48-bit ToD value and PortNumber as well to data array */
        __plp_aperta_allot_u64_into_u16array(((value64 << 8) | (port_num & 0xFF)), data);
        /* TOD update using firmware msg enable :  Reg.0xA000 = 0x0007 */
        PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_write(phy, HSIP_FWS_FWREG_000r,
                                                 (uint32_t) HSIP_FWS_FWMSG_TOD ) );
        /* write PortNumber and ToD value to FW message data registers 0xA001-0xA004 */
        for ( ii = 0; ii < 4; ii++ ) {
            PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_write(phy,
                                        (HSIP_FWS_FWREG_001r+ ii), (uint32_t) data[ii]) );
        }
        /* MSGIN - for PTP feature through fw msg interface to kick in  (Reg.0x8222=0x1801) */
        PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_write(phy, HSIP_MST_MSGINr    ,
                                                            HSIP_MST_MSGIN_TOD ) );
        /* wait for the execution of FW message to be done */
        for ( wait_fwmsg = 0; wait_fwmsg <  HSIP_MST_MSGOUT_WAIT_TIME_US;
                              wait_fwmsg += HSIP_MST_MSGOUT_WAIT_SLEEP_US ) {
            PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_read( phy, HSIP_MST_MSGOUTr, &vout) );
            if ( HSIP_MST_MSGOUT_TOD == vout ) {   /* FW message done, Reg.0x8221 == 0x1813 */
                break;    /* ToD update through FW done successfully  */
            }
            PHYMOD_USLEEP(HSIP_MST_MSGOUT_WAIT_SLEEP_US);  /* sleep 20us */
        }
        if ( wait_fwmsg >= HSIP_MST_MSGOUT_WAIT_TIME_US ) {
            return  PHYMOD_E_TIMEOUT;    /* ToD update through FW not done  */
        }
    }
    else {
        return PHYMOD_E_UNAVAIL;
    }

    return PHYMOD_E_NONE;
}

#endif

void __plp_aperta_lookup_timing_reg_tab(uint32_t  addr,
                             uint32_t *msbr, uint32_t *lsbr, int *pace) {
    timing_reg_t *ptr, rtab[] = P1588_TIMING_REG_TAB;

    /* linear search the rtab[] */
    for ( ptr = rtab; ptr->reg4msb; ptr++ ) {
        if ( (ptr->reg4msb >= addr) && (addr >= ptr->reg4lsb) ) {
            *pace =  1;  /* write LSB of value64 to the low address register  */
            break;   /* register series (ascending order)  found in the table */
        } else
        if ( (ptr->reg4msb <= addr) && (addr <= ptr->reg4lsb) ) {
            *pace = -1;  /* write MSB of value64 to the low address register  */
            break;   /* register series (descending order) found in the table */
        } else {
            continue;
        }
    }

    if ( FALSE == ptr->reg4msb ) {
        /* regaddr not found in rtab[], write value64[15:0] to regaddr        */
        ptr->reg4lsb = ptr->reg4msb = addr;
        *pace =  1;   /* write LSB[15:0] of value64 to the register regaddr   */
    }

    *msbr = ptr->reg4msb;
    *lsbr = ptr->reg4lsb;
}

int _plp_aperta_timesync_timing_control_set(const plp_aperta_phymod_phy_access_t* phy,
                                 uint32_t regaddr, uint64_t value64)
{
    uint32_t  reg4msb, reg4lsb;
    int       move, idx = 0;
    uint16_t  data[P1588_TIMING_VALUE_LEN_MAX] = { 0, 0, 0, 0 };

  #if defined(PHYMOD_EVORA_SUPPORT) || defined(PHYMOD_EUROPA_SUPPORT)  \
                                    || defined(PHYMOD_APERTA_SUPPORT)
    /* Time of Day (ToD) must be updated through FW message */
    if ( regaddr & TIMESYNC_HSIP_FWS_FWMSG_MASK ) {
        return  __plp_aperta_timesync_hsip_fws_fwmsg_send(phy, regaddr, value64);
    }
  #endif

    /* data[3]:bit_63_48 data[2]:bit_47_32 data[1]:bit_31_16 data[0]:bit_15_0 */
    __plp_aperta_allot_u64_into_u16array(value64, data);

    /* look up timing register table  */
    __plp_aperta_lookup_timing_reg_tab(regaddr, &reg4msb, &reg4lsb, &move);
    reg4msb +=  move;   /* set ending criterion for the for loop  */

    /* write data[] to the register series */
    for (  regaddr  = reg4lsb;
          (regaddr != reg4msb) && (idx < P1588_TIMING_VALUE_LEN_MAX);
           regaddr += move ) {
        PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_write(phy, regaddr, (uint32_t) data[idx++]) );
    }

    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_timing_control_get(const plp_aperta_phymod_phy_access_t* phy,
                                 uint32_t regaddr, uint64_t *value64)
{
    uint32_t  reg4msb, reg4lsb;
    int       move, idx = 0;
    uint32_t  data[P1588_TIMING_VALUE_LEN_MAX] =  { 0, 0, 0, 0 };

    /* look up timing register table  */
    __plp_aperta_lookup_timing_reg_tab(regaddr, &reg4msb, &reg4lsb, &move);
    reg4msb +=  move;   /* set ending criterion for the for loop  */

    /* write data[] to the register series */
    for (  regaddr  = reg4lsb;
          (regaddr != reg4msb) && (idx < P1588_TIMING_VALUE_LEN_MAX);
           regaddr += move ) {
        PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_read(phy, regaddr, &(data[idx++])) );
    }

    /* data[3]:bit_63_48 data[2]:bit_47_32 data[1]:bit_31_16 data[0]:bit_15_0 */
    COMPILER_64_SET( (*value64), (data[3] << 16) | data[2],
                                 (data[1] << 16) | data[0] );
    return PHYMOD_E_NONE;
}

#define  TIMESYNC_OPERATOR_ADD          1
#define  TIMESYNC_OPERATOR_SUB          2
#define  LINKDELAY_CTRL_SHIFT           4
#define  LINKDELAY_MSG_TYPE_MASK        0x00F0   /* bit[7:4] */
#define  LINKDELAY_OPERATOR_MASK        0x0200   /* bit[9] */

int _plp_aperta_timesync_link_delay_set(const plp_aperta_phymod_phy_access_t* phy,
                             uint32_t op, uint32_t msg_type, uint32_t linkdelay)
{
    P1588_PIM_NSE_OFFSET_5r_t       ctl;
    P1588_RX_LINK_DELAY_SELr_t      lsb;
    P1588_RX_LINK_DELAY_MSB_SELr_t  msb;
    PHYMOD_MEMSET(&ctl, 0, sizeof(P1588_PIM_NSE_OFFSET_5r_t));
    PHYMOD_MEMSET(&lsb, 0, sizeof(P1588_RX_LINK_DELAY_SELr_t));
    PHYMOD_MEMSET(&msb, 0, sizeof(P1588_RX_LINK_DELAY_MSB_SELr_t));

    msg_type >>= LINKDELAY_CTRL_SHIFT;  /* be aligned with link_delay bits in register */
    PHYMOD_IF_ERR_RETURN( READ_P1588_PIM_NSE_OFFSET_5r(phy, &ctl) );
    ctl._p1588_pim_nse_offset_5 &=  (~LINKDELAY_MSG_TYPE_MASK);
    ctl._p1588_pim_nse_offset_5 |=  ( LINKDELAY_MSG_TYPE_MASK & msg_type);
    ctl._p1588_pim_nse_offset_5 |=  ((TIMESYNC_OPERATOR_SUB == op)
                                     ? LINKDELAY_OPERATOR_MASK : 0);
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_PIM_NSE_OFFSET_5r(phy, ctl) );

    P1588_RX_LINK_DELAY_SELr_RX_LINK_DELAYf_SET(lsb, (linkdelay & BIT_15_00_MASK) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_RX_LINK_DELAY_SELr(phy, lsb) );

    PHYMOD_IF_ERR_RETURN( READ_P1588_RX_LINK_DELAY_MSB_SELr(phy, &msb) );
    P1588_RX_LINK_DELAY_MSB_SELr_RX_LINK_DELAY_MSBf_SET(msb, (linkdelay >> 16) );
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_RX_LINK_DELAY_MSB_SELr(phy, msb) );

    return PHYMOD_E_NONE;

}

int _plp_aperta_timesync_link_delay_get(const plp_aperta_phymod_phy_access_t* phy,
                             uint32_t *op, uint32_t *msg_type, uint32_t *linkdelay)
{
    P1588_PIM_NSE_OFFSET_5r_t       ctl;
    P1588_RX_LINK_DELAY_SELr_t      lsb;
    P1588_RX_LINK_DELAY_MSB_SELr_t  msb;
    PHYMOD_MEMSET(&ctl, 0, sizeof(P1588_PIM_NSE_OFFSET_5r_t));
    PHYMOD_MEMSET(&lsb, 0, sizeof(P1588_RX_LINK_DELAY_SELr_t));
    PHYMOD_MEMSET(&msb, 0, sizeof(P1588_RX_LINK_DELAY_MSB_SELr_t));

    PHYMOD_IF_ERR_RETURN( READ_P1588_PIM_NSE_OFFSET_5r(phy, &ctl) );
    *op       = (ctl._p1588_pim_nse_offset_5 & LINKDELAY_OPERATOR_MASK)
                ? TIMESYNC_OPERATOR_SUB : TIMESYNC_OPERATOR_ADD;
    *msg_type = (ctl._p1588_pim_nse_offset_5 & LINKDELAY_MSG_TYPE_MASK)
                << LINKDELAY_CTRL_SHIFT;

    PHYMOD_IF_ERR_RETURN( READ_P1588_RX_LINK_DELAY_SELr(phy, &lsb) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_RX_LINK_DELAY_MSB_SELr(phy, &msb) );
    *linkdelay = (P1588_RX_LINK_DELAY_SELr_RX_LINK_DELAYf_GET(lsb) & BIT_15_00_MASK) |
                 (P1588_RX_LINK_DELAY_MSB_SELr_RX_LINK_DELAY_MSBf_GET(msb) << 16);

    return PHYMOD_E_NONE;
}

#define  TS_OFFSET_CTRL_SHIFT           8
#define  TS_OFFSET_MSG_TYPE_MASK        0x000F   /* bit[3:0] */
#define  TS_OFFSET_OPERATOR_MASK        0x0100   /* bit[8] */

int _plp_aperta_timesync_timestamp_offset_set(const plp_aperta_phymod_phy_access_t* phy, int txrx,
                                   uint32_t op, uint32_t msg_type, uint32_t ts_offset)
{
    msg_type >>= TS_OFFSET_CTRL_SHIFT;   /* be aligned with TS_offset bits in register */
    /* TX */
    if ( (txrx & PHYMOD_TS_DIRECTION_TX) || (txrx == 0) ) {
        P1588_PIM_NSE_OFFSET_4r_t    ctl;
        P1588_TX_TS_OFFSETr_t        lsb;
        P1588_TXRX_TS_OFFSET_MSBr_t  msb;
        PHYMOD_MEMSET(&ctl, 0, sizeof(P1588_PIM_NSE_OFFSET_4r_t));
        PHYMOD_MEMSET(&lsb, 0, sizeof(P1588_TX_TS_OFFSETr_t));
        PHYMOD_MEMSET(&msb, 0, sizeof(P1588_TXRX_TS_OFFSET_MSBr_t));

        PHYMOD_IF_ERR_RETURN( READ_P1588_PIM_NSE_OFFSET_4r(phy, &ctl) );
        ctl._p1588_pim_nse_offset_4 &=  (~TS_OFFSET_MSG_TYPE_MASK);
        ctl._p1588_pim_nse_offset_4 |=  ( TS_OFFSET_MSG_TYPE_MASK & msg_type);
        ctl._p1588_pim_nse_offset_4 |=  ((TIMESYNC_OPERATOR_SUB == op)
                                         ? LINKDELAY_OPERATOR_MASK : 0);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_PIM_NSE_OFFSET_4r(phy, ctl) );

        P1588_TX_TS_OFFSETr_SET(lsb, (ts_offset & BIT_15_00_MASK));
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_TX_TS_OFFSETr(phy, lsb) );

        PHYMOD_IF_ERR_RETURN( READ_P1588_TXRX_TS_OFFSET_MSBr(phy, &msb) );
        P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_TX0_MSBf_SET(msb, (ts_offset >> 16) & BIT_03_00_MASK);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_TXRX_TS_OFFSET_MSBr(phy, msb) );
    }
    /* RX */
    if ( (txrx & PHYMOD_TS_DIRECTION_RX) || (txrx == 0) ) {
        P1588_PIM_NSE_OFFSET_5r_t    ctl;
        P1588_RX_TS_OFFSETr_t        lsb;
        P1588_TXRX_TS_OFFSET_MSBr_t  msb;
        PHYMOD_MEMSET(&ctl, 0, sizeof(P1588_PIM_NSE_OFFSET_5r_t));
        PHYMOD_MEMSET(&lsb, 0, sizeof(P1588_RX_TS_OFFSETr_t));
        PHYMOD_MEMSET(&msb, 0, sizeof(P1588_TXRX_TS_OFFSET_MSBr_t));

        PHYMOD_IF_ERR_RETURN( READ_P1588_PIM_NSE_OFFSET_5r(phy, &ctl) );
        ctl._p1588_pim_nse_offset_5 &=  (~TS_OFFSET_MSG_TYPE_MASK);
        ctl._p1588_pim_nse_offset_5 |=  ( TS_OFFSET_MSG_TYPE_MASK & msg_type);
        ctl._p1588_pim_nse_offset_5 |=  ((TIMESYNC_OPERATOR_SUB == op)
                                         ? LINKDELAY_OPERATOR_MASK : 0);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_PIM_NSE_OFFSET_5r(phy, ctl) );

        P1588_RX_TS_OFFSETr_TS_OFFSET_RX0f_SET(lsb, ts_offset & BIT_15_00_MASK);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_RX_TS_OFFSETr(phy, lsb) );

        PHYMOD_IF_ERR_RETURN( READ_P1588_TXRX_TS_OFFSET_MSBr(phy, &msb) );
        P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_RX0_MSBf_SET(msb, (ts_offset >> 16) & BIT_03_00_MASK);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_TXRX_TS_OFFSET_MSBr(phy, msb) );
    }

    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_timestamp_offset_get(const plp_aperta_phymod_phy_access_t* phy, int txrx,
                                   uint32_t *op, uint32_t *msg_type, uint32_t *ts_offset)
{
    /* TX */
    if ( (txrx & PHYMOD_TS_DIRECTION_TX) || (txrx == 0) ) {
        P1588_PIM_NSE_OFFSET_4r_t    ctl;
        P1588_TX_TS_OFFSETr_t        lsb;
        P1588_TXRX_TS_OFFSET_MSBr_t  msb;
        PHYMOD_MEMSET(&ctl, 0, sizeof(P1588_PIM_NSE_OFFSET_4r_t));
        PHYMOD_MEMSET(&lsb, 0, sizeof(P1588_TX_TS_OFFSETr_t));
        PHYMOD_MEMSET(&msb, 0, sizeof(P1588_TXRX_TS_OFFSET_MSBr_t));

        PHYMOD_IF_ERR_RETURN( READ_P1588_PIM_NSE_OFFSET_4r(phy, &ctl) );
        *op       = (ctl._p1588_pim_nse_offset_4 & TS_OFFSET_OPERATOR_MASK)
                    ? TIMESYNC_OPERATOR_SUB : TIMESYNC_OPERATOR_ADD;
        *msg_type = (ctl._p1588_pim_nse_offset_4 & TS_OFFSET_MSG_TYPE_MASK)
                    << TS_OFFSET_CTRL_SHIFT;

        PHYMOD_IF_ERR_RETURN( READ_P1588_TX_TS_OFFSETr(phy, &lsb) );
        PHYMOD_IF_ERR_RETURN( READ_P1588_TXRX_TS_OFFSET_MSBr(phy, &msb) );
        *ts_offset = (P1588_TX_TS_OFFSETr_GET(lsb) & BIT_15_00_MASK) |
                     (P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_TX0_MSBf_GET(msb) << 16);
    }
    /* RX */
    if ( (txrx & PHYMOD_TS_DIRECTION_RX) || (txrx == 0) ) {
        P1588_PIM_NSE_OFFSET_5r_t    ctl;
        P1588_RX_TS_OFFSETr_t        lsb;
        P1588_TXRX_TS_OFFSET_MSBr_t  msb;
        PHYMOD_MEMSET(&ctl, 0, sizeof(P1588_PIM_NSE_OFFSET_5r_t));
        PHYMOD_MEMSET(&lsb, 0, sizeof(P1588_RX_TS_OFFSETr_t));
        PHYMOD_MEMSET(&msb, 0, sizeof(P1588_TXRX_TS_OFFSET_MSBr_t));

        PHYMOD_IF_ERR_RETURN( READ_P1588_PIM_NSE_OFFSET_5r(phy, &ctl) );
        *op       = (ctl._p1588_pim_nse_offset_5 & TS_OFFSET_OPERATOR_MASK)
                    ? TIMESYNC_OPERATOR_SUB : TIMESYNC_OPERATOR_ADD;
        *msg_type = (ctl._p1588_pim_nse_offset_5 & TS_OFFSET_MSG_TYPE_MASK)
                    << TS_OFFSET_CTRL_SHIFT;

        PHYMOD_IF_ERR_RETURN( READ_P1588_RX_TS_OFFSETr(phy, &lsb) );
        PHYMOD_IF_ERR_RETURN( READ_P1588_TXRX_TS_OFFSET_MSBr(phy, &msb) );
        *ts_offset = (P1588_RX_TS_OFFSETr_GET(lsb) & BIT_15_00_MASK) |
                     (P1588_TXRX_TS_OFFSET_MSBr_TS_OFFSET_RX0_MSBf_GET(msb) << 16);
    }

    return PHYMOD_E_NONE;
}

#define  HB80_SZ  5   /* 80-bit heartbeat is distributed over 5 registers */

/* read the 80-bit heartbeat value from PIM_HEARTBEAT_0 - 4 registers */
int __plp_aperta_timesync_heartbeat_reg_get(const plp_aperta_phymod_phy_access_t* phy,
                                                  uint16_t  hb_val[]) {
    P1588_PIM_HEARTBEAT_4r_t   sec47_32;
    P1588_PIM_HEARTBEAT_3r_t   sec31_16;
    P1588_PIM_HEARTBEAT_2r_t   sec15_00;
    P1588_PIM_HEARTBEAT_1r_t   ns_31_16;
    P1588_PIM_HEARTBEAT_0r_t   ns_15_00;

    P1588_PIM_HEARTBEAT_4r_CLR(sec47_32);    /*     second[47:32] */
    P1588_PIM_HEARTBEAT_3r_CLR(sec31_16);    /*     second[31:16] */
    P1588_PIM_HEARTBEAT_2r_CLR(sec15_00);    /*     second[15:00] */
    P1588_PIM_HEARTBEAT_1r_CLR(ns_31_16);    /* nanosecond[29:16] */
    P1588_PIM_HEARTBEAT_0r_CLR(ns_15_00);    /* nanosecond[15:00] */

    /* read PIM_HEARTBEAT_0 - 4 registers, total of 80 bits */
    PHYMOD_IF_ERR_RETURN( READ_P1588_PIM_HEARTBEAT_0r(phy, &ns_15_00) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_PIM_HEARTBEAT_1r(phy, &ns_31_16) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_PIM_HEARTBEAT_2r(phy, &sec15_00) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_PIM_HEARTBEAT_3r(phy, &sec31_16) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_PIM_HEARTBEAT_4r(phy, &sec47_32) );
    /* copy heartbeat values to the returning array */
    hb_val[0] = P1588_PIM_HEARTBEAT_0r_PIM_HEARTBEAT_0f_GET(ns_15_00);
    hb_val[1] = P1588_PIM_HEARTBEAT_1r_PIM_HEARTBEAT_1f_GET(ns_31_16);
    hb_val[2] = P1588_PIM_HEARTBEAT_2r_PIM_HEARTBEAT_2f_GET(sec15_00);
    hb_val[3] = P1588_PIM_HEARTBEAT_3r_PIM_HEARTBEAT_3f_GET(sec31_16);
    hb_val[4] = P1588_PIM_HEARTBEAT_4r_PIM_HEARTBEAT_4f_GET(sec47_32);

    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_time_code_set(const plp_aperta_phymod_phy_access_t  *phy,
                            const plp_aperta_phymod_timesync_timespec_t *timecode)
{
    P1588_TIME_CODE_1r_t        t_79_64;
    P1588_TIME_CODE_2r_t        t_63_48;
    P1588_TIME_CODE_3r_t        t_47_32;
    P1588_TIME_CODE_4r_t        t_31_16;
    P1588_TIME_CODE_5r_t        t_15_00;
    PHYMOD_MEMSET(&t_79_64, 0, sizeof(P1588_TIME_CODE_1r_t));
    PHYMOD_MEMSET(&t_63_48, 0, sizeof(P1588_TIME_CODE_2r_t));
    PHYMOD_MEMSET(&t_47_32, 0, sizeof(P1588_TIME_CODE_3r_t));
    PHYMOD_MEMSET(&t_31_16, 0, sizeof(P1588_TIME_CODE_4r_t));
    PHYMOD_MEMSET(&t_15_00, 0, sizeof(P1588_TIME_CODE_5r_t));

    P1588_TIME_CODE_5r_NSE_REG_TIME_CODE_15_0f_SET(t_15_00,
                    (timecode->nanoseconds >>  0) & BIT_15_00_MASK);
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_TIME_CODE_5r(phy, t_15_00) );
    P1588_TIME_CODE_4r_NSE_REG_TIME_CODE_31_16f_SET(t_31_16,
                    (timecode->nanoseconds >> 16) & BIT_15_00_MASK);
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_TIME_CODE_4r(phy, t_31_16) );

    P1588_TIME_CODE_3r_NSE_REG_TIME_CODE_47_32f_SET(t_47_32,
                    (COMPILER_64_LO(timecode->seconds) >>  0) & BIT_15_00_MASK);
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_TIME_CODE_3r(phy, t_47_32) );
    P1588_TIME_CODE_2r_NSE_REG_TIME_CODE_63_48f_SET(t_63_48,
                    (COMPILER_64_LO(timecode->seconds) >> 16) & BIT_15_00_MASK);
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_TIME_CODE_2r(phy, t_63_48) );
    P1588_TIME_CODE_1r_NSE_REG_TIME_CODE_79_64f_SET( t_79_64,
                    (COMPILER_64_HI(timecode->seconds) >>  0) & BIT_15_00_MASK);
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_TIME_CODE_1r(phy, t_79_64) );

    return PHYMOD_E_NONE;
}

/* function to get 80-bit heartbeat or 80-bit time code in ToD format */
int _plp_aperta_timesync_time_code_get(const plp_aperta_phymod_phy_access_t  *phy, uint32_t flags,
                            plp_aperta_phymod_timesync_timespec_t *timecode)
{
    if ( (bcmplpTimesyncUnivHeartbeat80bit == flags)    ) {
        /* get the 80-bit heartbeat in ToD (time of day) format :             *\
        \*     48-bit second + 30-bit nanosecond (bit[31:30] are always zero) */
        uint16_t  hb_reg[HB80_SZ] = { 0U, 0U, 0U, 0U, 0U } ;
        uint32_t  hbw_63_32 = 0;

        /* get heartbeat value from PIM_HEARTBEAT_0 - 4 registers */
        PHYMOD_IF_ERR_RETURN( __plp_aperta_timesync_heartbeat_reg_get(phy, hb_reg) );

        /* copy bit[31:00] to 'nanosecond' */
        timecode->nanoseconds
                  = (((uint32_t) hb_reg[1]) << 16) | ((uint32_t) hb_reg[0]);
        /* copy bit[79:32] to 'second'     */
        hbw_63_32 = (((uint32_t) hb_reg[3]) << 16) | ((uint32_t) hb_reg[2]);
        COMPILER_64_SET(timecode->seconds, ((uint32_t) hb_reg[4]), hbw_63_32);
    }
    else {  /* get 80-bit time code value from P1588M_TIME_CODE_0 - 4 registers */
        P1588_TIME_CODE_1r_t  t_79_64;
        P1588_TIME_CODE_2r_t  t_63_48;
        P1588_TIME_CODE_3r_t  t_47_32;
        P1588_TIME_CODE_4r_t  t_31_16;
        P1588_TIME_CODE_5r_t  t_15_00;
        PHYMOD_MEMSET(&t_79_64, 0, sizeof(P1588_TIME_CODE_1r_t));
        PHYMOD_MEMSET(&t_63_48, 0, sizeof(P1588_TIME_CODE_2r_t));
        PHYMOD_MEMSET(&t_47_32, 0, sizeof(P1588_TIME_CODE_3r_t));
        PHYMOD_MEMSET(&t_31_16, 0, sizeof(P1588_TIME_CODE_4r_t));
        PHYMOD_MEMSET(&t_15_00, 0, sizeof(P1588_TIME_CODE_5r_t));

        PHYMOD_IF_ERR_RETURN( READ_P1588_TIME_CODE_5r(phy, &t_15_00) );
        PHYMOD_IF_ERR_RETURN( READ_P1588_TIME_CODE_4r(phy, &t_31_16) );
        timecode->nanoseconds =
                    (P1588_TIME_CODE_4r_NSE_REG_TIME_CODE_31_16f_GET(t_31_16) << 16) |
                     P1588_TIME_CODE_5r_NSE_REG_TIME_CODE_15_0f_GET( t_15_00);

        PHYMOD_IF_ERR_RETURN( READ_P1588_TIME_CODE_3r(phy, &t_47_32) );
        PHYMOD_IF_ERR_RETURN( READ_P1588_TIME_CODE_2r(phy, &t_63_48) );
        PHYMOD_IF_ERR_RETURN( READ_P1588_TIME_CODE_1r(phy, &t_79_64) );
        COMPILER_64_SET(timecode->seconds,
                    (P1588_TIME_CODE_1r_NSE_REG_TIME_CODE_79_64f_GET(t_79_64)),
                    (P1588_TIME_CODE_2r_NSE_REG_TIME_CODE_63_48f_GET(t_63_48) << 16) |
                     P1588_TIME_CODE_3r_NSE_REG_TIME_CODE_47_32f_GET(t_47_32));
    }

    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_capture_timestamp_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* cap_ts)
{
    P1588_TS_HB_SEL_1r_t   hb_sel;
    P1588_TS_HB_SEL_4r_t   bit15_00 ;
    P1588_TS_HB_SEL_3r_t   bit31_16;
    P1588_TS_HB_SEL_2r_t   bit47_32;
    P1588_TS_HB_SEL_13r_t  bit63_48;
    uint32_t lsb = 0, msb=0;

    PHYMOD_MEMSET(&hb_sel  , 0, sizeof(P1588_TS_HB_SEL_1r_t));
    PHYMOD_MEMSET(&bit15_00, 0, sizeof(P1588_TS_HB_SEL_4r_t));
    PHYMOD_MEMSET(&bit31_16, 0, sizeof(P1588_TS_HB_SEL_3r_t));
    PHYMOD_MEMSET(&bit47_32, 0, sizeof(P1588_TS_HB_SEL_2r_t));
    PHYMOD_MEMSET(&bit63_48, 0, sizeof(P1588_TS_HB_SEL_13r_t));

    P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_2f_SET(hb_sel, 1);
    PHYMOD_IF_ERR_RETURN(
            WRITE_P1588_TS_HB_SEL_1r(phy, hb_sel));

    PHYMOD_IF_ERR_RETURN(
            READ_P1588_TS_HB_SEL_13r(phy, &bit63_48));
    msb  = P1588_TS_HB_SEL_13r_SOP_TS_63_48f_GET(bit63_48) << 16;
    PHYMOD_IF_ERR_RETURN(
            READ_P1588_TS_HB_SEL_2r(phy, &bit47_32));
    msb |= P1588_TS_HB_SEL_2r_SOP_TS_47_32f_GET( bit47_32);

    PHYMOD_IF_ERR_RETURN(
            READ_P1588_TS_HB_SEL_3r(phy, &bit31_16));
    lsb  = P1588_TS_HB_SEL_3r_SOP_TS_31_16f_GET(bit31_16) << 16;
    PHYMOD_IF_ERR_RETURN(
            READ_P1588_TS_HB_SEL_4r(phy, &bit15_00));
    lsb |= P1588_TS_HB_SEL_4r_SOP_TS_15_0f_GET(bit15_00);

    COMPILER_64_SET(*cap_ts, msb, lsb);

    PHYMOD_MEMSET(&hb_sel, 0, sizeof(P1588_TS_HB_SEL_1r_t));
    P1588_TS_HB_SEL_1r_EXPANSION_REG_F3_1_3f_SET(hb_sel, 1);
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_TS_HB_SEL_1r(phy, hb_sel) );

    return PHYMOD_E_NONE;
}

/*
 * function to get the 48-bit heartbeat:
 *     16-bit second + 30-bit nanosecond  (bit[31:30] are always zero)
 */
int _plp_aperta_timesync_heartbeat_timestamp_get(const plp_aperta_phymod_phy_access_t* phy, uint64_t* hb_ts)
{
    PLP_P1588_PIM_HEARTBEAT_3r_t    hb_63_48;
    P1588_TS_HB_SEL_9r_t            hb_47_32;
    P1588_TS_HB_SEL_10r_t           hb_31_16;
    P1588_TS_HB_SEL_11r_t           hb_15_00;
    uint32_t                        hbw_63_32 = 0, hbw_31_00 = 0;

    PHYMOD_MEMSET(&hb_63_48, 0, sizeof(PLP_P1588_PIM_HEARTBEAT_3r_t));
    PHYMOD_MEMSET(&hb_47_32, 0, sizeof(P1588_TS_HB_SEL_9r_t ));
    PHYMOD_MEMSET(&hb_31_16, 0, sizeof(P1588_TS_HB_SEL_10r_t));
    PHYMOD_MEMSET(&hb_15_00, 0, sizeof(P1588_TS_HB_SEL_11r_t));

    /* Set nse_reg_sc_synmode_cntrl_13 via config set API to capture timestamp */
    PHYMOD_IF_ERR_RETURN( READ_P1588_TS_HB_SEL_11r(phy, &hb_15_00) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_TS_HB_SEL_10r(phy, &hb_31_16) );
    hbw_31_00  =  P1588_TS_HB_SEL_11r_HB_TS_15_0f_GET( hb_15_00)
               | (P1588_TS_HB_SEL_10r_HB_TS_31_16f_GET(hb_31_16) << 16);

    PHYMOD_IF_ERR_RETURN( READ_P1588_TS_HB_SEL_9r(    phy, &hb_47_32) );
    PHYMOD_IF_ERR_RETURN( READ_P1588_PIM_HEARTBEAT_3r(phy, &hb_63_48) );
    hbw_63_32  =  P1588_TS_HB_SEL_9r_HB_TS_47_32f_GET(        hb_47_32)
               | (P1588_PIM_HEARTBEAT_3r_PIM_HEARTBEAT_3f_GET(hb_63_48) << 16);
    COMPILER_64_SET(*hb_ts, hbw_63_32, hbw_31_00);

    return PHYMOD_E_NONE;
}

/* get the 80-bit heartbeat in plain format: 16 + 64 bit nanosecond */
int _plp_aperta_timesync_timestamp_univ_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags,
                                       uint32_t type, phymod_timestamp_univ_t *systime)
{
    if ( bcmplpTimesyncUnivHeartbeat80bit == type ) {  /* get 80-bit HeartBeat */
        uint16_t  hb_reg[HB80_SZ] = { 0U, 0U, 0U, 0U, 0U } ;
        uint32_t  hbw_63_32 = 0,  hbw_31_00 = 0;

        /* get heartbeat value from PIM_HEARTBEAT_0 - 4 registers */
        PHYMOD_IF_ERR_RETURN( __plp_aperta_timesync_heartbeat_reg_get(phy, hb_reg) );

        /* copy bit[63:00] to 'nanosecond' */
        hbw_63_32 = (((uint32_t) hb_reg[3]) << 16) | ((uint32_t) hb_reg[2]);
        hbw_31_00 = (((uint32_t) hb_reg[1]) << 16) | ((uint32_t) hb_reg[0]);
        COMPILER_64_SET(systime->nanosecond   ,  hbw_63_32, hbw_31_00);
        /* copy bit[79:64] to 'hi_nanosecond' */
        COMPILER_64_SET(systime->hi_nanosecond,  0U, ((uint32_t) hb_reg[0]));
    }
    else {
        return  PHYMOD_E_UNAVAIL;   /* unsupported timer type */
    }

    return  PHYMOD_E_NONE;
}

/*
 * SOPmem (start of packet memory) handling
 */

#if defined(HSIP_SOPMEM_ENTRY_FROM_FW_MESSAGE)
#define IF_LOOKUP( _key, _field, _tbd, _t, _w1, _w2 )           if ( (_key)->_field != (_tbd) )
#define SOPMEM_FETCH_IF_ERR_RETURN(_tab, _f, _acc, _e, _w, _sword)

#else
#define IF_LOOKUP( _key, _field, _tbd, _t, _w1, _w2 )  \
                                            if ( (_key)->_field == (_tbd) )  \
                                            { _t[(_w1)] |= PENDING; _t[(_w2)] |= PENDING; } else
#define IF_LOOKUP_ARRAY( _mask, _key, _field, _tbd )  \
                                            if ( ((_key)->lookup_key_mask & (_mask)) &&  \
                                                ((_key)->_field[0] != (_tbd)) )
#define SOPMEM_FETCH_IF_ERR_RETURN(_tab, _f, _acc, _e, _w, _sword)   do {  int _err;  \
            if ( _tab[(_w)] != FETCHED ) {  \
              if ((_err = _f((_acc), (_e), (_w), (_sword))) != PHYMOD_E_NONE) { return _err; }  \
              _tab[(_w)] |= FETCHED;  \
            }  \
        } while (0)
#endif

#define NEXT_IF_NOT_MATCH(_key, _hole, _field, _mask)       do {  \
            if (  (_key)->_field != (_hole)->_field )   return PHYMOD_E_FAIL;  \
            (_key)->lookup_key_mask &=  ~(_mask);  \
        } while (0)

#define NEXT_IF_NOT_MATCH_ARRAY(_key, _hole, _field, _n)    do {  int _i;  \
            for ( _i = 0; _i < _n; _i++ )  \
               if ((_key)->_field[_i] != (_hole)->_field[_i])   return PHYMOD_E_FAIL;  \
        } while (0)

#if defined(__SOPMEM_WEIGHTED_ENTRY_WORDS__)
int ___num_of_entry_words(uint32_t key_mask) {
    /* SrcIP & TimeStamp fields need multiple SOPMEM_RDATA reads */
    int  count = ((key_mask & bcmplpTimesyncSOPmemSrcIP    ) ? 6 : 0) +
                 ((key_mask & bcmplpTimesyncSOPmemTimestamp) ? 5 : 0) ;

    while ( key_mask ) {
        key_mask &= (key_mask - 1);  /* eliminate rightmost binary '1' */
        count++;                     /* count number of 1s in key_mask */
    }
    return  count;
}
#endif

#if ! defined(HSIP_SOPMEM_ENTRY_FROM_FW_MESSAGE)

int __sopmem_entry_words_get(const plp_aperta_phymod_phy_access_t* phy, int entry_id,
                             int word_start, int word_end, uint16_t entry_data[])
{
    uint32_t                 entry_data32 = 0;
    P1588_SOPMEM_CONTROLr_t  sopctrl;
    PHYMOD_MEMSET(&sopctrl, 0, sizeof(P1588_SOPMEM_CONTROLr_t));

    /* P1588 SOP Memory Control Type Register  [ load_index, sub_index, index ] */
    P1588_SOPMEM_CONTROLr_SOPMEM_LOAD_INDEXf_SET(        sopctrl, TRUE    );
    P1588_SOPMEM_CONTROLr_SOPMEM_SUB_INDEXf_SET(         sopctrl, word_start );
    P1588_SOPMEM_CONTROLr_SOPMEM_INDEXf_SET(             sopctrl, entry_id);
    PLP_P1588_SOPMEM_CONTROLr_SOPMEM_AUTO_INCREMENTf_SET(sopctrl, TRUE    );
  #if defined(XGBASET_IEEE1588_REG_FAST)
    PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_WRITE(ACCP(phy), (PLP_P1588_REG_BASE | PLP_P1588_SOPMEM_CONTROLr),
                                                      sopctrl._p1588_sopmem_control) );
  #else
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_SOPMEM_CONTROLr(phy, sopctrl) );
  #endif
    for ( word_end += 1; word_start < word_end; word_start++ ) {
        /* P1588 SOP Memory RDATA Register */
      #if defined(XGBASET_IEEE1588_REG_FAST)
        PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_READ(ACCP(phy), (PLP_P1588_REG_BASE | PLP_P1588_SOPMEM_RDATAr),
                                                         &entry_data32) );
      #else
        PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_read(phy, PLP_P1588_SOPMEM_RDATAr, &entry_data32) );
      #endif
        entry_data[word_start] = (uint16_t) entry_data32;
        PHYMOD_DEBUG_INFO(("SOPmem:: port(%d.%x) [entry=%02d,word=%02d] = 0x%04x\n",
                                     ACC(phy).addr, ACC(phy).lane_mask,
                                     entry_id, word_start, entry_data[word_start]));
    }
    return PHYMOD_E_NONE;
}

int __sopmem_entry_word_get(const plp_aperta_phymod_phy_access_t* phy,
                            int entry_id, int word_id, uint16_t entry_data[])
{
    uint32_t                 entry_data32 = 0;
    P1588_SOPMEM_CONTROLr_t  sopctrl;
    PHYMOD_MEMSET(&sopctrl, 0, sizeof(P1588_SOPMEM_CONTROLr_t));

    /* P1588 SOP Memory Control Type Register  [ load_index, sub_index, index ] */
    P1588_SOPMEM_CONTROLr_SOPMEM_LOAD_INDEXf_SET(sopctrl, TRUE    );
    P1588_SOPMEM_CONTROLr_SOPMEM_SUB_INDEXf_SET (sopctrl, word_id );
    P1588_SOPMEM_CONTROLr_SOPMEM_INDEXf_SET     (sopctrl, entry_id);
    /* P1588 SOP Memory Control / RDATA Registers */
  #if defined(XGBASET_IEEE1588_REG_FAST)
    PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_WRITE(ACCP(phy), (PLP_P1588_REG_BASE | PLP_P1588_SOPMEM_CONTROLr),
                                                      sopctrl._p1588_sopmem_control) );
    PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_READ( ACCP(phy), (PLP_P1588_REG_BASE | PLP_P1588_SOPMEM_RDATAr),
                                                      &entry_data32) );
  #else
    PHYMOD_IF_ERR_RETURN( WRITE_P1588_SOPMEM_CONTROLr(phy, sopctrl) );
    PHYMOD_IF_ERR_RETURN( plp_aperta_p1588_reg_read(phy, PLP_P1588_SOPMEM_RDATAr, &entry_data32) );
  #endif
    entry_data[word_id] = (uint16_t) entry_data32;
    PHYMOD_DEBUG_INFO(("SOPmem: port(%d.%x) [entry=%02d,word=%02d] = 0x%04x\n",
                                        ACC(phy).addr, ACC(phy).lane_mask,
                                        entry_id, word_id, entry_data[word_id]));

    return PHYMOD_E_NONE;
}

#endif  /* ! HSIP_SOPMEM_ENTRY_FROM_FW_MESSAGE */

#ifdef  TS_IEEE1588_SOPMEM_ARCH_20      /*  XxxFINs & Whitetip  */
  #define  ENTRY_VALID              13
  #define  ENTRY_VALID_BIT          15
  #define  ENTRY_RXTX               13
  #define  ENTRY_RXTX_BIT           6
  #define  VID_HI                   13
  #define  VID_HI_SHIFT             7
  #define  VID_HI_MASK              BIT_04_00_MASK
  #define  VID_LO_SHIFT             9
  #define  VID_LO_MASK              BIT_15_09_MASK
#else    /*  Aperta / Miura / Evora / Europa / Quadra28 / Orca  */
  #define  ENTRY_VALID              13
  #define  ENTRY_VALID_BIT          2
  #define  ENTRY_RXTX               12
  #define  ENTRY_RXTX_BIT           8
  #define  VID_HI                   12
  #define  VID_HI_SHIFT             4
  #define  VID_HI_MASK              BIT_07_00_MASK
  #define  VID_LO_SHIFT             12
  #define  VID_LO_MASK              BIT_15_12_MASK
#endif
  #define  VID_LO           (VID_HI-1)

int __plp_aperta_sopmem_entry_lookup(const plp_aperta_phymod_phy_access_t* phy, int entry_id,
                          phymod_timesync_sopmem_t *keys, phymod_timesync_sopmem_t *data)
{
    /* get one 28-byte entry from SOPmem, then parse it */

#if defined(HSIP_SOPMEM_ENTRY_FROM_FW_MESSAGE)
    /* Aperta has a Firmware Message to read SOPmem fast */
    aperta_ptp_sopmem_t  ptp_sopmem = { .addr = entry_id };
    uint16_t            *sopword;

    /* determine SOPmem port ID form the lane_mask */
    APERTA_GET_PORT_FROM_LM(phy->access.lane_mask, ptp_sopmem.port);
    /* Aperta uses firmware message to retrieve one whole 14-word entry from SOPmem */
    PHYMOD_IF_ERR_RETURN( plp_aperta_nse_ptp_sopmem_read(phy, &ptp_sopmem) );
    sopword =  ptp_sopmem.data; /* PHYMOD_MEMCPY(sopword,ptp_sopmem.data,P1588_SOPMEM_WORDS_MAX); */

#else  /* ! HSIP_SOPMEM_ENTRY_FROM_FW_MESSAGE */
    uint16_t  sopword[P1588_SOPMEM_WORDS_MAX] = { 0 }; /* one SOPmem entry has 14 entry words (16-bit word) */
    uint8_t   fetched[P1588_SOPMEM_WORDS_MAX];   /* entry_word's fetching status: INVALID/PENDING/FETCHED   */
    PHYMOD_MEMSET(fetched, INVALID, P1588_SOPMEM_WORDS_MAX);
#endif  /* HSIP_SOPMEM_ENTRY_FROM_FW_MESSAGE */

  /* STAGE 1: Fetch lookup-key fields and check whether this entry is a hit.                         *\
  |*          If any lookup-key gets missed, abandon this entry and move to next entry immediately.  *|
  |*          No need to waste time within this entry.   (SOPmem has totally 32 entries)             *|
  \*          The entry words of a non-lookup-key field will be marked as PENDING ( by IF_LOOKUP() ) */
    if ( keys->lookup_key_mask &  bcmplpTimesyncSOPmemSeqId     ) {
        IF_LOOKUP( keys, seq_id    , TBD32, fetched,  6,  5 )  {
            /* SOPmem entry_word_06 - { sourcePortID[03:00], sequenceID[15:04] }          */
            SOPMEM_FETCH_IF_ERR_RETURN( fetched, __sopmem_entry_word_get,
                                                            phy, entry_id,  6, sopword );
            /* SOPmem entry_word_05 - { sequenceID[3:0], domainNumber[7:0], msgType[3:0] } */
            SOPMEM_FETCH_IF_ERR_RETURN( fetched, __sopmem_entry_word_get,
                                                            phy, entry_id,  5, sopword );
            data->seq_id    = ((sopword[ 6] & BIT_11_00_MASK) <<  4) |
                              ((sopword[ 5] & BIT_15_12_MASK) >> 12);
            NEXT_IF_NOT_MATCH(keys, data, seq_id    , bcmplpTimesyncSOPmemSeqId    );
        }
    }
    if ( keys->lookup_key_mask &  bcmplpTimesyncSOPmemMsgType   ) {
        IF_LOOKUP( keys, msg_type  , TBD8 , fetched,  5,  0 )  {
            /* SOPmem entry_word_05 - { sequenceID[3:0], domainNumber[7:0], msgType[3:0] } */
            SOPMEM_FETCH_IF_ERR_RETURN( fetched, __sopmem_entry_word_get,
                                                            phy, entry_id,  5, sopword );
            data->msg_type  =  (sopword[ 5] & BIT_03_00_MASK) >>  0;
            NEXT_IF_NOT_MATCH(keys, data, msg_type  , bcmplpTimesyncSOPmemMsgType  );
        }
    }
    if ( keys->lookup_key_mask &  bcmplpTimesyncSOPmemDomainNum ) {
        IF_LOOKUP( keys, domain_num, TBD32, fetched,  5,  0 )  {
            /* SOPmem entry_word_05 - { sequenceID[3:0], domainNumber[7:0], msgType[3:0] } */
            SOPMEM_FETCH_IF_ERR_RETURN( fetched, __sopmem_entry_word_get,
                                                            phy, entry_id,  5, sopword );
            data->domain_num = (sopword[ 5] & BIT_11_04_MASK) >>  4;
            NEXT_IF_NOT_MATCH(keys, data, domain_num, bcmplpTimesyncSOPmemDomainNum);
        }
      #ifdef BCM_PLP_TIMESYNC_V2_1_SUPPORT
        fetched[11] = fetched[12] = PENDING;  /* need to fetch entry #11-12 for minor/major_SdoId */
      #endif
    }
    if ( keys->lookup_key_mask &  bcmplpTimesyncSOPmemDirection ) {
        IF_LOOKUP( keys, direction , TBD8 , fetched, ENTRY_RXTX,  0 )  {
            /* SOPmem entry_word_12 - { mask_enable[5:0], 1'b0, direction, vlanID[11:4] } */
            SOPMEM_FETCH_IF_ERR_RETURN( fetched, __sopmem_entry_word_get,
                                                            phy, entry_id,  ENTRY_RXTX,
                                                                               sopword );
            data->direction = (sopword[ENTRY_RXTX] & BITMAP32(ENTRY_RXTX_BIT)) >> ENTRY_RXTX_BIT;
            NEXT_IF_NOT_MATCH(keys, data, direction , bcmplpTimesyncSOPmemDirection);
        }
    }
  #ifdef BCM_PLP_TIMESYNC_V2_1_SUPPORT
    if ( keys->lookup_key_mask &  bcmplpTimesyncSOPmemSrcPort   ) {
        /* fetch multiple entry_words contiguously */
        PHYMOD_IF_ERR_RETURN( __sopmem_entry_words_get(phy, entry_id, 6, 11, sopword) ) ;
        fetched[6] = fetched[7] = fetched[11] =  FETCHED;

        /* SOPmem entry_word 6 - 11 : 80-bit sourcePortID */
        data->src_port_id[9] = ((sopword[11] & BIT_11_04_MASK) >>  4);   /* sourcePortID[79:72] */
        data->src_port_id[8] = ((sopword[11] & BIT_03_00_MASK) <<  4) |  /* sourcePortID[71:64] */
                               ((sopword[10] & BIT_15_12_MASK) >> 12);
        data->src_port_id[7] = ((sopword[10] & BIT_11_04_MASK) >>  4);   /* sourcePortID[63:56] */
        data->src_port_id[6] = ((sopword[10] & BIT_03_00_MASK) <<  4) |  /* sourcePortID[55:48] */
                               ((sopword[ 9] & BIT_15_12_MASK) >> 12);
        data->src_port_id[5] = ((sopword[ 9] & BIT_11_04_MASK) >>  4);   /* sourcePortID[47:40] */
        data->src_port_id[4] = ((sopword[ 9] & BIT_03_00_MASK) <<  4) |  /* sourcePortID[39:32] */
                               ((sopword[ 8] & BIT_15_12_MASK) >> 12);
        data->src_port_id[3] = ((sopword[ 8] & BIT_11_04_MASK) >>  4);   /* sourcePortID[31:24] */
        data->src_port_id[2] = ((sopword[ 8] & BIT_03_00_MASK) <<  4) |  /* sourcePortID[23:16] */
                               ((sopword[ 7] & BIT_15_12_MASK) >> 12);
        data->src_port_id[1] = ((sopword[ 7] & BIT_11_04_MASK) >>  4);   /* sourcePortID[15:08] */
        data->src_port_id[0] = ((sopword[ 7] & BIT_03_00_MASK) <<  4) |  /* sourcePortID[07:00] */
                               ((sopword[ 6] & BIT_15_12_MASK) >> 12);
        if ( keys->src_port_id[0] != TBD8 ) {
            NEXT_IF_NOT_MATCH_ARRAY(keys, data, src_port_id, 10 );
        }
    }

  #else  /* PTP v2.0 or older */
    if ( keys->lookup_key_mask &  bcmplpTimesyncSOPmemSrcPort   ) {
        IF_LOOKUP( keys, src_port  , TBD32, fetched,  7,  6 )  {
            /* SOPmem entry_word_07 - { sourceIP[03:00], sourcePortID[15:04]}             */
            SOPMEM_FETCH_IF_ERR_RETURN( fetched, __sopmem_entry_word_get,
                                                            phy, entry_id,  7, sopword );
            /* SOPmem entry_word_06 - { sourcePortID[03:00], sequenceID[15:04] }          */
            SOPMEM_FETCH_IF_ERR_RETURN( fetched, __sopmem_entry_word_get,
                                                            phy, entry_id,  6, sopword );
            data->src_port  = ((sopword[ 7] & BIT_07_00_MASK) <<  4) |
                              ((sopword[ 6] & BIT_15_12_MASK) >> 12) ;
            NEXT_IF_NOT_MATCH(keys, data, src_port  , bcmplpTimesyncSOPmemSrcPort  );
        }
    }
    if ( keys->lookup_key_mask &  bcmplpTimesyncSOPmemVlanId    ) {
        IF_LOOKUP( keys, vlan_id   , TBD32, fetched, VID_HI, VID_LO) {
            SOPMEM_FETCH_IF_ERR_RETURN( fetched, __sopmem_entry_word_get,
                                                            phy, entry_id, VID_HI, sopword );
            /* SOPmem entry_word_11 - { vlanID[3:0], sourceIP[63:52] }                    */
            SOPMEM_FETCH_IF_ERR_RETURN( fetched, __sopmem_entry_word_get,
                                                            phy, entry_id, VID_LO, sopword );
            /* SOPmem entry_word_12 - { mask_enable[5:0], 1'b0, direction, vlanID[11:4] } */
            data->vlan_id   = ((sopword[VID_HI] & VID_HI_MASK) << VID_HI_SHIFT) |
                              ((sopword[VID_LO] & VID_LO_MASK) >> VID_LO_SHIFT) ;
            NEXT_IF_NOT_MATCH(keys, data, vlan_id   , bcmplpTimesyncSOPmemVlanId   );
        }
    }
    if ( keys->lookup_key_mask &  bcmplpTimesyncSOPmemValid     ) {
        IF_LOOKUP( keys, valid     , TBD8 , fetched, ENTRY_VALID,  0 )  {
            /* SOPmem entry_word_13 - { 13'h0, valid, mask_enable[7:6] }                  */
            SOPMEM_FETCH_IF_ERR_RETURN( fetched, __sopmem_entry_word_get,
                                                            phy, entry_id, ENTRY_VALID, sopword );
            data->valid     = (sopword[ENTRY_VALID] & BITMAP32(ENTRY_VALID_BIT)) >> ENTRY_VALID_BIT;
            NEXT_IF_NOT_MATCH(keys, data, valid     , bcmplpTimesyncSOPmemValid    );
        }
    }

        /************************************************************************************\
         *   Sorce IP address (64 bits) layout inside an SOPmem entry :                     *
         *                                                                                  *
         *  SrcIP      6         5         4         3         2         1         0        *
         *    bit   3210987654321098765432109876543210987654321098765432109876543210        *
         *         +============################================################====        *
         * SOPmem  word_11     |    word_10    |    word_09    |    word_08    |  word_07   *
         *         +-------+-------+-------+-------+-------+-------+-------+-------+        *
         *  octet  | byte7 | byte6 | byte5 | byte4 | byte3 | byte2 | byte1 | byte0 |        *
         *         +-------+-------+-------+-------+-------+-------+-------+-------+        *
         *                                                                                  *
        \************************************************************************************/
    if ( keys->lookup_key_mask & bcmplpTimesyncSOPmemSrcIP     ) {
      #if ! defined(HSIP_SOPMEM_ENTRY_FROM_FW_MESSAGE)
        /* SOPmem entry_word 7 - 11  (Source IP, the least key, always fetch !)   */
        int  start = 7, end = 11;

        /* consider entry_word 5 & 6 as well to acclerate fetching since MDIO read is expensive! */
        if ( fetched[7] != FETCHED ) {
            if ( fetched[6] == PENDING ) {
                if (fetched[5] == PENDING ) {
                    start = 5;          /* involve entry_word 5 & 6 together for contiguous fetching */
                    fetched[5] = FETCHED;  /* mark entry_word 5 up (also 6 later) */
                } else {
                    start = 6;          /* involve entry_word 6 together for contiguous fetching */
                }
                fetched[6] = FETCHED;      /* mark entry_word 6 up */
            } else {  /* nothing */ }
        } else {
            start= 8;   /* entry_word 7 is in already, start from word 8 */
        }

        /* consider entry_word 12 & 13 as well to acclerate fetching since MDIO read is expensive! */
        if ( fetched[11] != FETCHED ) {
            if ( fetched[12] == PENDING ) {
                if (fetched[13] == PENDING ) {
                    end  = 13;         /* involve entry_word 12 & 13 together for contiguous fetching */
                    fetched[13] = FETCHED; /* mark entry_word 13 up (also 12 later) */
                } else {
                    end  = 12;         /* involve entry_word 12 together for contiguous fetching */
                }
                fetched[12] = FETCHED;     /* mark entry_word 12 up */
            } else {  /* nothing */ }
        } else {
            end = 10;   /* entry_word 11 is in already, end at word 10 */
        }

        /* fetch entry_words contiguously */
        PHYMOD_IF_ERR_RETURN( __sopmem_entry_words_get(phy, entry_id, start, end, sopword) ) ;
      #endif  /* ! HSIP_SOPMEM_ENTRY_FROM_FW_MESSAGE */

        /* SOPmem entry_word_11 - { vlanID[3:0], sourceIP[63:52] } */
        data->src_ip[7] = ((sopword[11] & BIT_11_04_MASK) >>  4);   /* srcIP[63:56] */
        data->src_ip[6] = ((sopword[11] & BIT_03_00_MASK) <<  4) |  /* srcIP[55:52] */
                          ((sopword[10] & BIT_15_12_MASK) >> 12);   /* srcIP[51:48] */
        /* SOPmem entry_word_10 - { sourceIP[51:36] } */
        data->src_ip[5] = ((sopword[10] & BIT_11_04_MASK) >>  4);   /* srcIP[47:40] */
        data->src_ip[4] = ((sopword[10] & BIT_03_00_MASK) <<  4) |  /* srcIP[39:36] */
                          ((sopword[ 9] & BIT_15_12_MASK) >> 12);   /* srcIP[35:32] */
        /* SOPmem entry_word_09 - { sourceIP[35:20] } */
        data->src_ip[3] = ((sopword[ 9] & BIT_11_04_MASK) >>  4);   /* srcIP[31:24] */
        data->src_ip[2] = ((sopword[ 9] & BIT_03_00_MASK) <<  4) |  /* srcIP[23:20] */
        /* SOPmem entry_word_08 - { sourceIP[19:04] } */
                          ((sopword[ 8] & BIT_15_12_MASK) >> 12);   /* srcIP[19:16] */
        data->src_ip[1] = ((sopword[ 8] & BIT_11_04_MASK) >>  4);   /* srcIP[15:08] */
        /* SOPmem entry_word_07 - { sourceIP[03:00], sourcePortID[15:04]} */
        data->src_ip[0] = ((sopword[ 8] & BIT_03_00_MASK) <<  4) |  /* srcIP[07:04] */
                          ((sopword[ 7] & BIT_15_12_MASK) >> 12);   /* srcIP[03:00] */
        if ( keys->src_ip[0] != TBD8 ) {
            NEXT_IF_NOT_MATCH_ARRAY(keys, data, src_ip, 8 );
        }
    }
  #endif /* BCM_PLP_TIMESYNC_V2_1_SUPPORT */

  /* STAGE 2: Now all lookup-keys are matched. This entry is a hit.  So go ahead to fetch        *\
  \*          the 5 entry words of the TimeStamp field.  ('TimeStamp' field is not a lookup-key) */
  #if defined(HSIP_SOPMEM_ENTRY_FROM_FW_MESSAGE)
    PHYMOD_MEMCPY(data->timestamp, sopword, P1588_TIMESTAMP_LEN);

  #else
    if ( keys->lookup_key_mask & bcmplpTimesyncSOPmemTimestamp ) {
        /* SOPmem entry_word 0 - 4  (TimeStamp) */
        PHYMOD_IF_ERR_RETURN(     __sopmem_entry_words_get(phy, entry_id,  0,  4,
                                                           (uint16_t *) (data->timestamp)) );
    }
  /* STAGE 3: resolve pending entry_word 5 or 6                                             *\
  |*          In the 1st Stage, only lookup-key words are fetched, and other required words *|
  \*          are marked as PENDING. So start fetching these pending words now.             */
    if ( fetched[6] == PENDING ) {
        if ( fetched[5] == PENDING ) {   /* __sopmem_entry_words_get() can save 1 MDIO write */
            PHYMOD_IF_ERR_RETURN( __sopmem_entry_words_get(phy, entry_id,  5,  6, sopword) );
        } else {
            PHYMOD_IF_ERR_RETURN( __sopmem_entry_word_get( phy, entry_id,  6, sopword) );
        }
    } else
    if ( fetched[5] == PENDING ) {
            PHYMOD_IF_ERR_RETURN( __sopmem_entry_word_get( phy, entry_id,  5, sopword) );
    } else {  /* nothing */ }

    /* resolve pending entry_word #11, 12 or 13 */
    if ( fetched[11] == PENDING ) {
            PHYMOD_IF_ERR_RETURN( __sopmem_entry_word_get( phy, entry_id, 11, sopword) );
    }
    if ( fetched[12] == PENDING ) {
        if ( fetched[13] == PENDING ) {   /* __sopmem_entry_words_get() can save 1 MDIO write */
            PHYMOD_IF_ERR_RETURN( __sopmem_entry_words_get(phy, entry_id, 12, 13, sopword) );
        } else {
            PHYMOD_IF_ERR_RETURN( __sopmem_entry_word_get( phy, entry_id, 12, sopword) );
        }
    } else
    if ( fetched[13] == PENDING ) {
            PHYMOD_IF_ERR_RETURN( __sopmem_entry_word_get( phy, entry_id, 13, sopword) );
    } else {  /* nothing */ }
  #endif  /* HSIP_SOPMEM_ENTRY_FROM_FW_MESSAGE */

    /* populate non-lookup-key member fields */
    if ( keys->lookup_key_mask &  bcmplpTimesyncSOPmemSeqId     ) {
        /* SOPmem entry_word_06 - { sourcePortID[03:00], sequenceID[15:04] }          */
        /* SOPmem entry_word_5 - { sequenceID[3:0], domainNumber[7:0], msgType[3:0] } */
        data->seq_id    = ((sopword[ 6] & BIT_11_00_MASK) <<  4) |
                          ((sopword[ 5] & BIT_15_12_MASK) >> 12) ;
    }
    if ( keys->lookup_key_mask &  bcmplpTimesyncSOPmemMsgType   ) {
        /* SOPmem entry_word_5 - { sequenceID[3:0], domainNumber[7:0], msgType[3:0] } */
        data->msg_type  =  (sopword[ 5] & BIT_03_00_MASK) >>  0;
    }
    if ( keys->lookup_key_mask &  bcmplpTimesyncSOPmemDomainNum ) {
        /* SOPmem entry_word_5 - { sequenceID[3:0], domainNumber[7:0], msgType[3:0] } */
        data->domain_num = (sopword[ 5] & BIT_11_04_MASK) >>  4;
      #ifdef BCM_PLP_TIMESYNC_V2_1_SUPPORT
        /*    domain_num[31:0] = reserved[31:20] + major_SdoId[19:16] + minor_SdoId[15:8] + domainNumber[7:0]   */
        data->domain_num    |= ((sopword[12] & BIT_07_00_MASK) << 12) |  /* major_SdoId[3:0] + minor_SdoId[7:4] */
                               ((sopword[11] & BIT_15_12_MASK) <<  8);   /*                  + minor_SdoId[3:0] */
      #endif
    }
    if ( keys->lookup_key_mask &  bcmplpTimesyncSOPmemDirection ) {
        /* SOPmem entry_word_12 - { mask_enable[5:0], 1'b0, direction, vlanID[11:4] } */
        data->direction  = (sopword[ENTRY_RXTX] & BITMAP32(ENTRY_RXTX_BIT))
                                                        >> ENTRY_RXTX_BIT;
    }
    if ( keys->lookup_key_mask &  bcmplpTimesyncSOPmemSrcPort   ) {
        /* SOPmem entry_word_07 - { sourceIP[03:00], sourcePortID[15:04]}             */
        /* SOPmem entry_word_06 - { sourcePortID[03:00], sequenceID[15:04] }          */
        data->src_port  = ((sopword[ 7] & BIT_07_00_MASK) <<  4) |
                          ((sopword[ 6] & BIT_15_12_MASK) >> 12) ;
    }
    if ( keys->lookup_key_mask &  bcmplpTimesyncSOPmemVlanId    ) {
        /* SOPmem entry_word_11 - { vlanID[3:0], sourceIP[63:52] }                    */
        /* SOPmem entry_word_12 - { mask_enable[5:0], 1'b0, direction, vlanID[11:4] } */
        data->vlan_id   = ((sopword[VID_HI] & VID_HI_MASK) << VID_HI_SHIFT) |
                          ((sopword[VID_LO] & VID_LO_MASK) >> VID_LO_SHIFT) ;
    }
    if ( keys->lookup_key_mask &  bcmplpTimesyncSOPmemValid     ) {
        /* SOPmem entry_word_13 - { 13'h0, valid, mask_enable[7:6] }                  */
        data->valid     =  (sopword[ENTRY_VALID] & BITMAP32(ENTRY_VALID_BIT))
                                                         >> ENTRY_VALID_BIT;
    }

    return PHYMOD_E_NONE;
}

/* clear SOPmem entries */
int __plp_aperta_sopmem_entry_clear(const plp_aperta_phymod_phy_access_t* phy, uint32_t entry_bitmap ) {
    if ( BIT_31_00_MASK == entry_bitmap ) {   /* clear all 32 entries at once */
        PLP_P1588_SOPMEM_CONTROL2r_t  sopmem_ctrl2;
        PHYMOD_IF_ERR_RETURN(  READ_P1588_SOPMEM_CONTROL2r(phy, &sopmem_ctrl2) );
        P1588_SOPMEM_CONTROL2r_SOPMEM_ALL_ENTRIES_CLEARf_SET(sopmem_ctrl2, TRUE);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_SOPMEM_CONTROL2r(phy,  sopmem_ctrl2) );
    } else {
        PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_t  sopmem_clr_lo;
        PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_t  sopmem_clr_hi;
        PHYMOD_MEMSET(&sopmem_clr_hi, 0, sizeof( PLP_P1588_SOPMEM_CLEAR_HI_ENTRYr_t));
        PHYMOD_MEMSET(&sopmem_clr_lo, 0, sizeof( PLP_P1588_SOPMEM_CLEAR_LO_ENTRYr_t));

        P1588_SOPMEM_CLEAR_LO_ENTRYr_SOPMEM_CLEAR_LO_ENTRYf_SET(sopmem_clr_lo,
                                                                entry_bitmap & BIT_15_00_MASK);
        P1588_SOPMEM_CLEAR_HI_ENTRYr_SOPMEM_CLEAR_HI_ENTRYf_SET(sopmem_clr_hi,
                                                                entry_bitmap >> 16);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_SOPMEM_CLEAR_HI_ENTRYr(phy, sopmem_clr_hi) );
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_SOPMEM_CLEAR_LO_ENTRYr(phy, sopmem_clr_lo) );
    }
    return PHYMOD_E_NONE;
}

/* retrieve the SOPmem entry matches the key */
int _plp_aperta_timesync_sopmem_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags,
                           int entry_id, phymod_timesync_sopmem_t *keys)
{
    PLP_P1588_SOPMEM_VALID_LO_ENTRYr_t  valid_lo;
    PLP_P1588_SOPMEM_VALID_HI_ENTRYr_t  valid_hi;
    phymod_timesync_sopmem_t            data = { 0x0U };
    uint32_t  valid_mask = 0;
    int       rv = PHYMOD_E_FAIL;
  #if defined(XGBASET_IEEE1588_REG_FAST)
    uint32_t  xgp = 0;
  #endif

    if ( flags == bcmplpTimesyncSOPmemOpClearAll ) {
        /* clear all SOPmem entries  (entry_id is don't care) */
        return  __plp_aperta_sopmem_entry_clear(phy, BIT_31_00_MASK);
    }
    if ( (entry_id < 0) || (P1588_SOPMEM_ENTRY_MAX <= entry_id) ) {  /* range check */
        return PHYMOD_E_PARAM;   /* entry_id is out of range */
    }

    /* perform SOPmem operations based on 'flags' */
    if ( flags == bcmplpTimesyncSOPmemOpClassify ) {
        /* get the classification: SrcIP / DstIP / SA / DA / PtpSrcPrt / MplsLabel */
        if ( keys->lookup_key_mask & bcmplpTimesyncSOPmemClassifiedData ) {
            P1588_INBAND_TX_CONTROL2r_t  tx_ib_ctl2;
            P1588_INBAND_RX_CONTROL2r_t  rx_ib_ctl2;

            /* Inband Tx Control 2  1588Reg.B8[14:12]/[2:0] select capture IP/MAC/SrcPort... */
            PHYMOD_IF_ERR_RETURN(  READ_P1588_INBAND_TX_CONTROL2r(phy, &tx_ib_ctl2) );
            keys->src_ip[bcmplpTimesyncTx] =
                  WRT_IPMAC_GET(P1588_INBAND_TX_CONTROL2r_WRITE_SOPMEMf_GET(tx_ib_ctl2));
            /* Inband RX Control 2  1588Reg.BA[14:12]/[2:0] select capture IP/MAC/SrcPort... */
            PHYMOD_IF_ERR_RETURN(  READ_P1588_INBAND_RX_CONTROL2r(phy, &rx_ib_ctl2) );
            keys->src_ip[bcmplpTimesyncRx] =
                  WRT_IPMAC_GET(P1588_INBAND_RX_CONTROL2r_WRITE_SOPMEMf_GET(rx_ib_ctl2));
        }
        return PHYMOD_E_NONE;
    } else
    if ( (flags & bcmplpTimesyncSOPmemOpNoSkipInvalid) == 0x0U ) {  /* NoSkipInvalid flag is not set */
        /* check the SOPmem's Number of Valid Entry register */
        PHYMOD_IF_ERR_RETURN( PLP_READ_P1588_SOPMEM_VALID_LO_ENTRYr(phy, &valid_lo) );
        PHYMOD_IF_ERR_RETURN( PLP_READ_P1588_SOPMEM_VALID_HI_ENTRYr(phy, &valid_hi) );
        valid_mask   =  PLP_P1588_SOPMEM_VALID_LO_ENTRYr_SOPMEM_VALID_LO_ENTRYf_GET(valid_lo) |
                       (PLP_P1588_SOPMEM_VALID_HI_ENTRYr_SOPMEM_VALID_HI_ENTRYf_GET(valid_hi) << 16);
        valid_mask >>= entry_id;    /* search SOPmem starting from the (entry_id)-th entry */
        PHYMOD_DEBUG_INFO(("SOPmem: valid mask = 0x%08x\n", valid_mask));
    }
    else {  /* bcmplpTimesyncSOPmemOpNoSkipInvalid flag is set */
        PHYMOD_MEMSET(&data, TBD8, sizeof(phymod_timesync_sopmem_t));    /* empty the entry buffer */
        valid_mask = BITMAP32(entry_id);  /* set the mask bit to get the entry indexed by entry_id */
    }

    if ( (INVALID == valid_mask) ) {
        keys->valid =  INVALID;     /* mark the returned entry line as invalid */
        return PHYMOD_E_MEMORY;     /* no valid entry found in SOP MEMORY      */
    }
  #if defined(XGBASET_IEEE1588_REG_FAST)
    /* set XGP table to XFI read/write for accessing P1588M registers */
    PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_READ(ACCP(phy), XGP_PF_RW_REG_00, &xgp) );
    if ( xgp != XGP_PF_RW_REG_XFI ) {
        PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_WRITE(ACCP(phy), XGP_PF_RW_REG_00, XGP_PF_RW_REG_XFI) );
    }
  #endif
    /* search the 32 SOPmem entries sequentially */
    while ( 0x0U != valid_mask ) {
        if ( (valid_mask & 0x1U) || (flags & bcmplpTimesyncSOPmemOpNoSkipInvalid) ) {
            rv = __plp_aperta_sopmem_entry_lookup(phy, entry_id, keys, &data);
            if ( PHYMOD_E_NONE == rv ) {
                /* an entry hit, copy SOPmem entry to data structure prepared by caller */
                PHYMOD_MEMCPY(keys, &data, sizeof(phymod_timesync_sopmem_t));
                break;
            }
        }
        entry_id++;    valid_mask >>= 1;
    }
  #if defined(XGBASET_IEEE1588_REG_FAST)
    /* resume XGP table */
    if ( xgp != XGP_PF_RW_REG_XFI ) {
        PHYMOD_IF_ERR_RETURN( PHYMOD_BUS_WRITE(ACCP(phy), XGP_PF_RW_REG_00, xgp) );
    }
  #endif

    /* clear the hit SOPmem entry if clear flag is set */
    if ( (PHYMOD_E_NONE == rv) && (flags & bcmplpTimesyncSOPmemOpClearOne) ) {
        if ( entry_id < P1588_SOPMEM_ENTRY_MAX ) {    /* range check */
            rv = __plp_aperta_sopmem_entry_clear(phy, BITMAP32(entry_id));
        }
    }
    return  rv;
}

/* retrieve the SOPmem entry matches the key */
int _plp_aperta_timesync_sopmem_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t flags,
                               phymod_timesync_sopmem_t *keys)
{
    if ( flags & bcmplpTimesyncSOPmemOpClassify ) {
        if ( keys->lookup_key_mask & bcmplpTimesyncSOPmemClassifiedData ) {
            P1588_INBAND_TX_CONTROL2r_t  tx_ib_ctl2;
            P1588_INBAND_RX_CONTROL2r_t  rx_ib_ctl2;

            /* Inband Tx Control 2  1588Reg.B8[14:12]/[2:0] select capture IP/MAC/SrcPort... */
            PHYMOD_IF_ERR_RETURN(  READ_P1588_INBAND_TX_CONTROL2r(phy, &tx_ib_ctl2) );
            P1588_INBAND_TX_CONTROL2r_CMP_SOPMEMf_SET(
                                    tx_ib_ctl2, CMP_IPMAC_SET(keys->src_ip[bcmplpTimesyncTx]));
            P1588_INBAND_TX_CONTROL2r_WRITE_SOPMEMf_SET(
                                    tx_ib_ctl2, WRT_IPMAC_SET(keys->src_ip[bcmplpTimesyncTx]));
            PHYMOD_IF_ERR_RETURN( WRITE_P1588_INBAND_TX_CONTROL2r(phy,  tx_ib_ctl2) );

            /* Inband RX Control 2  1588Reg.BA[14:12]/[2:0] select capture IP/MAC/SrcPort... */
            PHYMOD_IF_ERR_RETURN(  READ_P1588_INBAND_RX_CONTROL2r(phy, &rx_ib_ctl2) );
            P1588_INBAND_RX_CONTROL2r_CMP_SOPMEMf_SET(
                                    rx_ib_ctl2, CMP_IPMAC_SET(keys->src_ip[bcmplpTimesyncRx]));
            P1588_INBAND_RX_CONTROL2r_WRITE_SOPMEMf_SET(
                                    rx_ib_ctl2, WRT_IPMAC_SET(keys->src_ip[bcmplpTimesyncRx]));
            PHYMOD_IF_ERR_RETURN( WRITE_P1588_INBAND_RX_CONTROL2r(phy,  rx_ib_ctl2) );
        }
    }

    return PHYMOD_E_NONE;
}

/*
 * Interrupt handling
 */
#define  INTERRUPT_ENABLE_FLAG_MASK       (1U << 31)

int _plp_aperta_timesync_phy_intr_enable_set(const plp_aperta_phymod_phy_access_t* phy, uint32_t en_mask)
{
    P1588_INTERRUPT_MASKr_t  intr_mask;
    PHYMOD_MEMSET(&intr_mask, 0, sizeof( P1588_INTERRUPT_MASKr_t));
    int  en_dis = (en_mask & INTERRUPT_ENABLE_FLAG_MASK) ? TRUE : FALSE;

    if ( PHYMOD_BASET_INTR_PTP_IPG_GET(en_mask) ) {
        P1588_INTERRUPT_MASKr_INTC_IPG_MASKf_SET(intr_mask, en_dis);
    }
    if ( PHYMOD_BASET_INTR_PTP_SYNCIN_GET(en_mask) ) {
        P1588_INTERRUPT_MASKr_INTC_SYNCIN_MASKf_SET(intr_mask, en_dis);
    }
    if ( PHYMOD_BASET_INTR_PTP_SOP_GET(en_mask) ) {
        P1588_INTERRUPT_MASKr_INTC_SOP_MASKf_SET(intr_mask, en_dis);
    }
    if ( PHYMOD_BASET_INTR_PTP_FSYNC_GET(en_mask) ) {
        P1588_INTERRUPT_MASKr_INTC_FSYNC_MASKf_SET(intr_mask, en_dis);
    }

    PHYMOD_IF_ERR_RETURN( WRITE_P1588_INTERRUPT_MASKr(phy, intr_mask) );
    return PHYMOD_E_NONE;
}

int _plp_aperta_timesync_phy_intr_enable_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t *en_mask)
{
    P1588_INTERRUPT_MASKr_t  intr_mask;
    PHYMOD_MEMSET(&intr_mask, 0, sizeof( P1588_INTERRUPT_MASKr_t));

    *en_mask = 0x0;
    PHYMOD_IF_ERR_RETURN( READ_P1588_INTERRUPT_MASKr(phy, &intr_mask) );

    if ( P1588_INTERRUPT_MASKr_INTC_IPG_MASKf_GET(intr_mask) ) {
        PHYMOD_BASET_INTR_PTP_IPG_SET(*en_mask);
    }
    if ( P1588_INTERRUPT_MASKr_INTC_SYNCIN_MASKf_GET(intr_mask) ) {
        PHYMOD_BASET_INTR_PTP_SYNCIN_SET(*en_mask);
    }
    if ( P1588_INTERRUPT_MASKr_INTC_SOP_MASKf_GET(intr_mask) ) {
        PHYMOD_BASET_INTR_PTP_SOP_SET(*en_mask);
    }
    if ( P1588_INTERRUPT_MASKr_INTC_FSYNC_MASKf_GET(intr_mask) ) {
        PHYMOD_BASET_INTR_PTP_FSYNC_SET(*en_mask);
    }

    return PHYMOD_E_NONE;
}


int _plp_aperta_timesync_phy_intr_status_get(const plp_aperta_phymod_phy_access_t* phy, uint32_t *intr_status)
{
    P1588_INTERRUPT_STATUSr_t  intr_stat_reg;
    PHYMOD_MEMSET(&intr_stat_reg, 0, sizeof( P1588_INTERRUPT_STATUSr_t));

    *intr_status = 0x0;
    PHYMOD_IF_ERR_RETURN( READ_P1588_INTERRUPT_STATUSr(phy, &intr_stat_reg) );

    if ( P1588_INTERRUPT_STATUSr_INTC_IPG_STATf_GET(intr_stat_reg) ) {
        PHYMOD_BASET_INTR_PTP_IPG_SET(*intr_status);
    }
    if ( P1588_INTERRUPT_STATUSr_INTC_SYNCIN_STATf_GET(intr_stat_reg) ) {
        PHYMOD_BASET_INTR_PTP_SYNCIN_SET(*intr_status);
    }
    if ( P1588_INTERRUPT_STATUSr_INTC_SOP_STATf_GET(intr_stat_reg) ) {
        PHYMOD_BASET_INTR_PTP_SOP_SET(*intr_status);
    }
    if ( P1588_INTERRUPT_STATUSr_INTC_FSYNC_STATf_GET(intr_stat_reg) ) {
        PHYMOD_BASET_INTR_PTP_FSYNC_SET(*intr_status);
    }

    return PHYMOD_E_NONE;
}

#ifdef BCM_PLP_TIMESYNC_V2_1_SUPPORT

/******************************************************************************\
|*  IEEE1588 PTP_v2.1 Lookup-Actons support                                    |
\******************************************************************************/

/*--------------------------------------------------------------------------*\
   chip Pre-defined Lookup-Action modes :

                                Tx/Rx P1588_INBAND_CONTROL1 registers
                          0:Rx  SOPmemOp  1AS    PTC    TC  (0xB7/B9)
          mode            1:Tx   bit_10  bit_9  bit_8  bit_7
     ------------------   ----   ------  -----  -----  -----
      GM/SC normal          1       1      -      0      0
      GM/SC pure 2step      1       0      -      0      0
      GM/SC inband          0       0      -      0      0
      GM/SC Ooband          0       1      -      0      0
      E2E TC regular        -       0      0      0      1
      E2E TC partial        -       0      0      1      1
      E2E TC pure 2step     -       1      0      -      1
      P2P TC regular        -       0      1      0      1
      P2P TC partial        -       0      1      1      1
      P2P TC pure 2step     -       1      1      -      1
\*--------------------------------------------------------------------------*/

/* action mode mapping table entries: ------------ mode -----------------  --reg_val-- */
#define ACTION_MODE_MAP_ENTRY_F   bcmplpTimesyncPtpActionModeGmSc          /* 0x18U */
#define ACTION_MODE_MAP_ENTRY_E   bcmplpTimesyncPtpActionModeGmSc2Step     /* 0x10U */
#define ACTION_MODE_MAP_ENTRY_0   bcmplpTimesyncPtpActionModeGmScInband    /* 0x00U */
#define ACTION_MODE_MAP_ENTRY_8   bcmplpTimesyncPtpActionModeGmScOoband    /* 0x08U */
#define ACTION_MODE_MAP_ENTRY_1   bcmplpTimesyncPtpActionModeE2eTcRegular  /* 0x01U */
#define ACTION_MODE_MAP_ENTRY_3   bcmplpTimesyncPtpActionModeE2eTcPartial  /* 0x03U */
#define ACTION_MODE_MAP_ENTRY_9   bcmplpTimesyncPtpActionModeE2eTc2step    /* 0x09U */
#define ACTION_MODE_MAP_ENTRY_5   bcmplpTimesyncPtpActionModeP2pTcRegular  /* 0x05U */
#define ACTION_MODE_MAP_ENTRY_7   bcmplpTimesyncPtpActionModeP2pTcPartial  /* 0x07U */
#define ACTION_MODE_MAP_ENTRY_D   bcmplpTimesyncPtpActionModeP2pTc2step    /* 0x0dU */
#define ACTION_MODE_MAP_ENTRY__   bcmplpTimesyncPtpActionModeInvalid

/* map the regiser value to pre-defined lookup-action mode according to above */
bcm_plp_timesync_ptp_action_mode_t                           /* mapping table */
__map_reg_to_action(uint16_t act_reg, bcm_plp_timesync_txrx_t rxtx) {
    static const bcm_plp_timesync_ptp_action_mode_t    /* action mode mapping table */
           act_map[] =  { ACTION_MODE_MAP_ENTRY_0, ACTION_MODE_MAP_ENTRY_1,
                          ACTION_MODE_MAP_ENTRY__, ACTION_MODE_MAP_ENTRY_3,
                          ACTION_MODE_MAP_ENTRY__, ACTION_MODE_MAP_ENTRY_5,
                          ACTION_MODE_MAP_ENTRY__, ACTION_MODE_MAP_ENTRY_7,
                          ACTION_MODE_MAP_ENTRY_8, ACTION_MODE_MAP_ENTRY_9,
                          ACTION_MODE_MAP_ENTRY__, ACTION_MODE_MAP_ENTRY__,
                          ACTION_MODE_MAP_ENTRY__, ACTION_MODE_MAP_ENTRY_D,
                          ACTION_MODE_MAP_ENTRY_E, ACTION_MODE_MAP_ENTRY_F };
    uint16_t  idx = (act_reg & 0x0fU);  /* register value: 0x0 - 0xf */

    if ( (0x0U == (act_reg & 0x03U)) && (bcmplpTimesyncTx == rxtx) ) {
        /* register setting is in GM/SC mode for TX PTP messages */
        idx = 0x0eU | (idx >> 3);   /* if (act_reg==0x0) map to MAP_ENTRY_E */
    }                               /* if (act_reg==0x8) map to MAP_ENTRY_F */
    return (act_map[idx]);   /* return the corresponding action mode */
}

/* map the pre-defined lookup-action mode to corresponding regiser value */
uint16_t
__map_action_to_reg(bcm_plp_timesync_ptp_action_mode_t act_mode) {
    static const uint16_t
           reg_map[] = { 0x18U, 0x10U, 0x00U, 0x08U, 0x01U,    /* register values for */
                         0x03U, 0x09U, 0x05U, 0x07U, 0x0dU };  /* the 10 action modes */
    uint16_t  idx = (uint16_t) act_mode - 1;

    /* hash function to convert an mode value to the index of mapping table reg_map[]   */
    idx = (idx & 0x0fU) +                      /* mode value [0x101-104] to index [0-3] */
          ((0x10U == (idx & 0x10U)) ? 4 :      /* mode value [0x111-113] to index [4-7] */
           (0x20U == (idx & 0x20U)) ? 7 : 0);  /* mode value [0x121-123] to index [7-9] */
    return (reg_map[idx]);   /* return the corresponding register value */
}

/* set Pre-defined Lookup-Action mode */
int __set_p1588_action_mode(const plp_aperta_phymod_phy_access_t* phy, uint32_t addr,
                                  bcm_plp_timesync_ptp_action_mode_t mode) {
    P1588_PREDEFINE_ACTION_SELECTr_t   act_mode_sel;

    P1588_PREDEFINE_ACTION_SELECTr_CLR(act_mode_sel);
    /* Tx/Rx P1588_INBAND_CONTROL1 registers -- 1588Reg.0xb7/b9 */
    /*       bit[10:7] Action mode select */
    P1588_PREDEFINE_ACTION_SEL_MODEf_SET( act_mode_sel, __map_action_to_reg(mode) );
    /*       bit[4] must be set when PTPv2.1 is enabled         */
    P1588_PREDEFINE_ACTION_PTPv2p1f_SET(  act_mode_sel, 0x1U );
    return  WRITE_P1588_PREDEFINE_ACTION_SELECTr(phy, addr, act_mode_sel);
}


/* set PTP lookup actions */
int _timesync_ptp_action_set(const plp_aperta_phymod_phy_access_t* phy, unsigned int flags,
                                      bcm_plp_timesync_txrx_t             rxtx ,
                                      bcm_plp_timesync_ptp_action_mode_t  mode ,
                                      bcm_plp_timesync_user_action_t     *user_def) {
    /* pre-defined PTP lookup actions */
    if ( mode > bcmplpTimesyncPtpActionModeInvalid ) {
        /*   bcmplpTimesyncTx = 0x2 , bcmplpTimesyncRx = 0x1                     */
        if ( bcmplpTimesyncTx & rxtx ) {    /* set 1588Reg.0xb7 if (rxtx == 0x2) */
            PHYMOD_IF_ERR_RETURN(
                    __set_p1588_action_mode(phy, P1588_PREDEFINE_ACTION_TX_SELECT, mode) );
        }
        if ( bcmplpTimesyncRx & rxtx ) {    /* set 1588Reg.0xb9 if (rxtx == 0x1) */
            PHYMOD_IF_ERR_RETURN(
                    __set_p1588_action_mode(phy, P1588_PREDEFINE_ACTION_RX_SELECT, mode) );
        }
    }                                       /* set both if (rxtx == 0x3)         */

    /* user-defined PTP lookup actions */
    if ( user_def != NULL ) {
        int  ii, dir;
        P1588_USER_ACTION_SELECTr_t  user_act[NUM_USER_ACTION_SEL_REG];
        P1588_USER_ACTION_SELECTr_CLR(user_act);

        /* set user-defined PTP lookup actions for the 9 classified PTP packet types */
        P1588_USER_ACTION_SEL_SYNC_1STEPf_SET(user_act[0], user_def->sync_1step);
        P1588_USER_ACTION_SEL_SYNC_2STEPf_SET(user_act[0], user_def->sync_2step);
        P1588_USER_ACTION_SEL_DELAY_REQf_SET( user_act[0], user_def->delay_request);
        P1588_USER_ACTION_SEL_SYNC_FLUPf_SET( user_act[0], user_def->sync_followup);
        P1588_USER_ACTION_SEL_PDLY_REQf_SET(      user_act[1], user_def->pdelay_request);
        P1588_USER_ACTION_SEL_PDLY_RSP_1STEPf_SET(user_act[1], user_def->pdelay_response_1step);
        P1588_USER_ACTION_SEL_PDLY_RSP_2STEPf_SET(user_act[1], user_def->pdelay_response_2step);
        P1588_USER_ACTION_SEL_PDLY_RSP_FLUPf_SET( user_act[1], user_def->pdelay_response_followup);
        P1588_USER_ACTION_SEL_DELAY_RESPf_SET( user_act[2], user_def->delay_response);
        /* enable/disable User-defined Lookup Actions */
        P1588_USER_ACTION_SEL_USER_DEFINEf_SET(user_act[2],
                  (bcmplpTimesyncPtpActionModeUserDefine == mode) ? 0x1U : 0x0U);

        /* write values to indirect P1588_USER_ACTION_SELECT registers */
        dir = (bcmplpTimesyncRx == rxtx) ? 0 : NUM_USER_ACTION_SEL_REG;
        for ( ii = 0; ii < NUM_USER_ACTION_SEL_REG; ii++ ) {
            PHYMOD_IF_ERR_RETURN( WRITE_P1588_USER_ACTION_SELECTr(phy,
                                       (P1588_USER_ACTION_SELECTr + ii + dir), user_act[ii]) );
        }
    }

    /* enable PTP v2.1 support to active the Lookup-Action mechanism */
    if ( 0x00U == (bcmplpTimesyncPtpActionModeSetActionOnly & flags) ) {
        P1588_INTERRUPT_MASKr_t             intr_mask;
        P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_t ts_capture_en;
        P1588_TX_OPTION_SELr_t              tx_option_sel;
        P1588_RX_OPTION_SELr_t              rx_option_sel;
        P1588_TX_CONTROLr_t                 tx_ctrl;
        P1588_RX_CONTROLr_t                 rx_ctrl;
        P1588_SLICE_ENABLE_CONTROLr_t       p1588_en;
      #if ! defined(__PTP_HEADER_FLAG_TWO_STEP_PARSING_FIXED__)
        P1588_TX_DEBUGr_t                   tx_dbg;     /* workaround for the "Two-Step" flag */
        P1588_RX_DEBUGr_t                   rx_dbg;     /*   HW issue when parsing PTP header */
        /* P1588_TX_DEBUG register - 1588Reg.19[4]: correct the "Two-Step" flag in PTP header FLAGS field  */
        PHYMOD_IF_ERR_RETURN(  READ_P1588_TX_DEBUGr(phy, &tx_dbg) );
        P1588_TX_DEBUGr_TX_OFFSET_4f_SET(tx_dbg,
                            (bcmplpTimesyncPtpActionModeOff == mode) ? 0x0U : 0x1U);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_TX_DEBUGr(phy,  tx_dbg) );
        /* P1588_RX_DEBUG register - 1588Reg.1b[4]: correct the "Two-Step" flag in PTP header FLAGS field  */
        PHYMOD_IF_ERR_RETURN(  READ_P1588_RX_DEBUGr(phy, &rx_dbg) );
        P1588_RX_DEBUGr_RX_OFFSET_4f_SET(rx_dbg,
                            (bcmplpTimesyncPtpActionModeOff == mode) ? 0x0U : 0x1U);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_RX_DEBUGr(phy,  rx_dbg) );
      #endif

        /* P1588_SLICE_ENABLE_CONTROL register - 1588Reg.00[4]:ptp_version_2p1 */
        PHYMOD_IF_ERR_RETURN(  READ_P1588_SLICE_ENABLE_CONTROLr(phy, &p1588_en) );
        P1588_SLICE_ENABLE_CONTROLr_PTP_VER_2_1f_SET(p1588_en,
                            (bcmplpTimesyncPtpActionModeOff == mode) ? 0x0U : 0x1U);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_SLICE_ENABLE_CONTROLr(phy,  p1588_en) );

        /* P1588_TX_CONTROL register - 1588Reg.18[3:0]:enable 802.1AS & 1588 L2/L4 IPv4/v6 packets */
        PHYMOD_IF_ERR_RETURN(  READ_P1588_TX_CONTROLr(phy, &tx_ctrl) );
        P1588_TX_CONTROLr_TX_AS_L2_L4_ENf_SET(tx_ctrl,
                            (bcmplpTimesyncPtpActionModeOff == mode) ? 0x0U : 0xfU);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_TX_CONTROLr(phy,  tx_ctrl) );
        /* P1588_RX_CONTROL register - 1588Reg.1a[3:0]:enable 802.1AS & 1588 L2/L4 IPv4/v6 packets */
        PHYMOD_IF_ERR_RETURN(  READ_P1588_RX_CONTROLr(phy, &rx_ctrl) );
        P1588_RX_CONTROLr_RX_AS_L2_L4_ENf_SET(rx_ctrl,
                            (bcmplpTimesyncPtpActionModeOff == mode) ? 0x0U : 0xfU);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_RX_CONTROLr(phy,  rx_ctrl) );

        /* P1588_TxRx_SOP_TS_CAPTURE register - 1588Reg.07[1:0]:enable Tx/Rx timestamp capture */
        PHYMOD_IF_ERR_RETURN(  READ_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr(phy, &ts_capture_en) );
        P1588_TXRX_SOP_TS_CAPTURE_ENABLEr_TXRX_TS_CAPf_SET(ts_capture_en,
                            (bcmplpTimesyncPtpActionModeOff == mode) ? 0x0U : 0x3U);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_TXRX_SOP_TS_CAPTURE_ENABLEr(phy,  ts_capture_en) );
        /* P1588_TX_OPTION_SEL register - 1588Reg.08[6,2]:Disable the TX PTP v2.1/v2.0 version check */
        PHYMOD_IF_ERR_RETURN(  READ_P1588_TX_OPTION_SELr(phy, &tx_option_sel) );
        P1588_TX_OPTION_SELr_TX_OPTION_PTPv2p1_CHK_DISf_SET(tx_option_sel,
                            (bcmplpTimesyncPtpActionModeOff == mode) ? 0x0U : 0x1U);
        P1588_TX_OPTION_SELr_TX_OPTION_PTPv2_CHK_DISf_SET(tx_option_sel,
                            (bcmplpTimesyncPtpActionModeOff == mode) ? 0x0U : 0x1U);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_TX_OPTION_SELr(phy,  tx_option_sel) );
        /* P1588_RX_OPTION_SEL register - 1588Reg.09[6,2]:Disable the RX PTP v2.1/v2.0 version check */
        PHYMOD_IF_ERR_RETURN(  READ_P1588_RX_OPTION_SELr(phy, &rx_option_sel) );
        P1588_RX_OPTION_SELr_RX_OPTION_PTPv2p1_CHK_DISf_SET(rx_option_sel,
                            (bcmplpTimesyncPtpActionModeOff == mode) ? 0x0U : 0x1U);
        P1588_RX_OPTION_SELr_RX_OPTION_PTPv2_CHK_DISf_SET(rx_option_sel,
                            (bcmplpTimesyncPtpActionModeOff == mode) ? 0x0U : 0x1U);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_RX_OPTION_SELr(phy,  rx_option_sel) );

        /* P1588_INTERRUPT_MASK register - 1588Reg.16[3:0]:interrupt mask */
        PHYMOD_IF_ERR_RETURN(  READ_P1588_INTERRUPT_MASKr(phy, &intr_mask) );
        P1588_INTERRUPT_MASKr_INTC_IPG_PCH_SOP_FSYNC_SET(intr_mask,
                            (bcmplpTimesyncPtpActionModeOff == mode) ? 0x0U : 0xfU);
        PHYMOD_IF_ERR_RETURN( WRITE_P1588_INTERRUPT_MASKr(phy,  intr_mask) );
}

    return PHYMOD_E_NONE;
}

/* get PTP lookup actions */
int _timesync_ptp_action_get(const plp_aperta_phymod_phy_access_t *phy, unsigned int flags,
                                      bcm_plp_timesync_txrx_t             rxtx ,
                                      bcm_plp_timesync_ptp_action_mode_t *mode ,
                                      bcm_plp_timesync_user_action_t     *user_def) {
    P1588_SLICE_ENABLE_CONTROLr_t  en1588;
    PHYMOD_IF_ERR_RETURN( READ_P1588_SLICE_ENABLE_CONTROLr(phy, &en1588) );
    /* P1588_SLICE_ENABLE_CONTROL register - 1588Reg.00[4] */
    *mode = (0x0U == P1588_SLICE_ENABLE_CONTROLr_PTP_VER_2_1f_GET(en1588))
          ? bcmplpTimesyncPtpActionModeOff : bcmplpTimesyncPtpActionModeInvalid;

    /* user-defined PTP lookup actions */
    if ( user_def != NULL ) {
        P1588_USER_ACTION_SELECTr_t  user_act[NUM_USER_ACTION_SEL_REG];
        int   ii, dir = (bcmplpTimesyncRx == rxtx) ? 0 : NUM_USER_ACTION_SEL_REG;

        for ( ii = 0; ii < NUM_USER_ACTION_SEL_REG; ii++ ) {
            PHYMOD_IF_ERR_RETURN( READ_P1588_USER_ACTION_SELECTr(phy,
                                        (P1588_USER_ACTION_SELECTr + ii + dir), &user_act[ii]) );
        }
        user_def->sync_1step    = P1588_USER_ACTION_SEL_SYNC_1STEPf_GET(user_act[0]);
        user_def->sync_2step    = P1588_USER_ACTION_SEL_SYNC_2STEPf_GET(user_act[0]);
        user_def->delay_request = P1588_USER_ACTION_SEL_DELAY_REQf_GET( user_act[0]);
        user_def->sync_followup = P1588_USER_ACTION_SEL_SYNC_FLUPf_GET( user_act[0]);
        user_def->pdelay_request           = P1588_USER_ACTION_SEL_PDLY_REQf_GET(      user_act[1]);
        user_def->pdelay_response_1step    = P1588_USER_ACTION_SEL_PDLY_RSP_1STEPf_GET(user_act[1]);
        user_def->pdelay_response_2step    = P1588_USER_ACTION_SEL_PDLY_RSP_2STEPf_GET(user_act[1]);
        user_def->pdelay_response_followup = P1588_USER_ACTION_SEL_PDLY_RSP_FLUPf_GET( user_act[1]);
        user_def->delay_response  = P1588_USER_ACTION_SEL_DELAY_RESPf_GET(user_act[2]);

        if ( (bcmplpTimesyncPtpActionModeInvalid == *mode) &&
             (0x0U != P1588_USER_ACTION_SEL_USER_DEFINEf_GET(user_act[2])) ) {
            *mode = bcmplpTimesyncPtpActionModeUserDefine;
        }
    }

    /* pre-defined PTP lookup actions */
    if ( bcmplpTimesyncPtpActionModeInvalid == *mode ) {
        uint32_t  addr = (bcmplpTimesyncRx == rxtx) ? P1588_PREDEFINE_ACTION_RX_SELECT
                                                    : P1588_PREDEFINE_ACTION_TX_SELECT;
        P1588_PREDEFINE_ACTION_SELECTr_t   predef_act;
        P1588_PREDEFINE_ACTION_SELECTr_CLR(predef_act);
        /* Tx/Rx P1588_INBAND_CONTROL1 registers -- 1588Reg.B7/B9[10:7] */
        PHYMOD_IF_ERR_RETURN( READ_P1588_PREDEFINE_ACTION_SELECTr(phy, addr, &predef_act) );
        *mode = __map_reg_to_action(P1588_PREDEFINE_ACTION_SEL_MODEf_GET(predef_act), rxtx);
    }

    return PHYMOD_E_NONE;
}

#endif  /* BCM_PLP_TIMESYNC_V2_1_SUPPORT */

