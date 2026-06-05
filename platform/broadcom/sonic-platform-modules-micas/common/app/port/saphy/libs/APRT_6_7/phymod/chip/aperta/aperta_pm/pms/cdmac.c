/*
 *
 * $Id:$
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 * File:    cdmac.c
 * Purpose:
 *
 *
 *
 */

#include <include/portmod.h>
#include <include/cdmac.h>
#include <phymod/phymod.h>
#include <include/bcmi_aperta_cdmac_defs.h>

#if defined(PHYMOD_APERTA_SUPPORT)
#include <phymod/phymod_acc.h>
#include <tier1/aperta_reg_access.h>

/* Minimun and maximum average IPG value in bits */
#define CDMAC_AVE_IPG_MIN               64
#define CDMAC_AVE_IPG_MAX               480

#define APERTA_RD_CDMAC(REGNAME, PHY, REG_INS)                          \
    PHYMOD_MEMSET(&REG_INS, 0, sizeof(BCMI_APERTA_CDMAC_##REGNAME##_t));   \
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_CDMAC_READ_##REGNAME((&PHY->access, &REG_INS));

#define aperta_soc_reg_field_get(PHY, REGNAME, REG_INS, FIELD)          BCMI_APERTA_CDMAC_##REGNAME##_##FIELD##_GET(REG_INS)
#define aperta_soc_reg_field_set(PHY, REGNAME, REG_INS, FIELD, VALUE)   BCMI_APERTA_CDMAC_##REGNAME##_##FIELD##_SET(REG_INS, VALUE)

#define APERTA_WR_CDMAC(REGNAME, PHY, REG_INS)                          \
    PHYMOD_IF_ERR_RETURN(BCMI_APERTA_CDMAC_WRITE_##REGNAME((&PHY->access, REG_INS));

int plp_aperta_cdmac_init(const plp_aperta_phymod_phy_access_t *phy, uint32_t init_flags)
{
#if 1
    portmod_pause_control_t pause_control;
    portmod_pfc_control_t pfc_control;
    portmod_remote_fault_control_t remote_fault_control;
    portmod_local_fault_control_t local_fault_control;
    int crc_mode;
    int is_append_crc;
    int ctr_enable = 0, ctr_clear = 0;
    BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t rx_ctrl;
    BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_t tx_ctrl;
    BCMI_APERTA_CDMAC_CDMAC_ECC_CTRLr_t ecc_ctrl;
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t mac_ctrl;

    PHYMOD_MEMSET(&mac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));
    PHYMOD_MEMSET(&rx_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t));
    PHYMOD_MEMSET(&tx_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_t));
    PHYMOD_MEMSET(&ecc_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_ECC_CTRLr_t));

    is_append_crc = 1;

    /* RX Max size */
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_rx_max_size_set(phy, CDMAC_JUMBO_MAXSZ));
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_cntmaxsize_set(phy, CDMAC_JUMBO_MAXSZ));
    /*
     * Enable and Clear the CDMAC MIB counters
     * No need to explicitly set the clear flag to 0 since the
     * function internally unsets the clear flag
     */
    ctr_enable = TRUE;
    ctr_clear = TRUE;
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_mib_counter_control_set(phy, ctr_enable,
                                                   ctr_clear));

    /* RX Control */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_CTRLr(phy, &rx_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_RX_CTRLr, rx_ctrl, STRIP_CRCf, 1);
    aperta_soc_reg_field_set(phy, CDMAC_RX_CTRLr, rx_ctrl, RX_PASS_PAUSEf, 0);
    aperta_soc_reg_field_set(phy, CDMAC_RX_CTRLr, rx_ctrl, RX_PASS_PFCf, 0);
    aperta_soc_reg_field_set(phy, CDMAC_RX_CTRLr, rx_ctrl, RX_PASS_CTRLf, 1);
    aperta_soc_reg_field_set(phy, CDMAC_RX_CTRLr, rx_ctrl, RUNT_THRESHOLDf,
                      CDMAC_RUNT_THRESHOLD_DEFAULT);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_RX_CTRLr(phy, rx_ctrl));

    /* TX Control */
    if(is_append_crc) {
        /* CRC is computed on incoming packet data and appended. */
        crc_mode = 0;
    }

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_TX_CTRLr(phy, &tx_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_TX_CTRLr, tx_ctrl, CRC_MODEf, crc_mode);
    aperta_soc_reg_field_set(phy, CDMAC_TX_CTRLr, tx_ctrl, DISCARDf, FALSE);
    aperta_soc_reg_field_set(phy, CDMAC_TX_CTRLr, tx_ctrl, PAD_ENf, FALSE);
    aperta_soc_reg_field_set(phy, CDMAC_TX_CTRLr, tx_ctrl, PAD_THRESHOLDf,
                      CDMAC_PAD_THRESHOLD_SIZE_DEFAULT);
    aperta_soc_reg_field_set(phy, CDMAC_TX_CTRLr, tx_ctrl, AVERAGE_IPGf,
                      CDMAC_AVERAGE_IPG_DEFAULT);
    aperta_soc_reg_field_set(phy, CDMAC_TX_CTRLr, tx_ctrl, TX_THRESHOLDf, 1);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_TX_CTRLr(phy,tx_ctrl));

    /* ECC control */
    /* TX CDC ecc control */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_ECC_CTRLr(phy, &ecc_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_ECC_CTRLr, ecc_ctrl,
                      TX_CDC_ECC_CTRL_ENf, 1);
    aperta_soc_reg_field_set(phy, CDMAC_ECC_CTRLr, ecc_ctrl, TX_CDC_ECC_CTRL_ENf, TRUE);
    aperta_soc_reg_field_set(phy, CDMAC_ECC_CTRLr, ecc_ctrl, MIB_COUNTER_ECC_CTRL_ENf,
                      TRUE);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_ECC_CTRLr(phy, ecc_ctrl));

    /* LSS */
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_portmod_remote_fault_control_t_init(0,&remote_fault_control));
    remote_fault_control.enable = TRUE;
    remote_fault_control.drop_tx_on_fault = TRUE;
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_remote_fault_control_set(phy,
                                                    &remote_fault_control));

    PHYMOD_IF_ERR_RETURN(
            plp_aperta_portmod_local_fault_control_t_init(0, &local_fault_control));
    local_fault_control.enable = TRUE;
    local_fault_control.drop_tx_on_fault = TRUE;
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_local_fault_control_set(phy,
                                                   &local_fault_control));

    /* Pause */
    PHYMOD_IF_ERR_RETURN(plp_aperta_portmod_pause_control_t_init(0, &pause_control));
    pause_control.tx_enable = TRUE;
    pause_control.rx_enable = TRUE;
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_pause_control_set(phy, &pause_control));

    /* PFC */
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_pfc_control_get(phy, &pfc_control));
    pfc_control.rx_enable = FALSE;
    pfc_control.tx_enable = FALSE;
    pfc_control.stats_en = TRUE;
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_pfc_control_set(phy, &pfc_control));
#endif
    /* MAC control
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &mac_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, mac_ctrl, LOCAL_LPBKf, FALSE);
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, mac_ctrl, TX_ENf, TRUE);
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, mac_ctrl, RX_ENf, TRUE);
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, mac_ctrl, MAC_LINK_DOWN_SEQ_ENf, FALSE);
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, mac_ctrl, SOFT_RESETf, FALSE);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_CTRLr(phy, mac_ctrl));*/

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_speed_set(const plp_aperta_phymod_phy_access_t *phy, int flags, int speed)
{
    /*
     * There is nothing to be programmed in CDMAC for Speed.
     * Speed setting is handled in the serdes.
     */
    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_speed_get(const plp_aperta_phymod_phy_access_t *phy, int *speed, int speed_to_configure)
{
    BCMI_APERTA_CDMAC_CDPORT_MODEr_t reg_val;
    int single_port_mode = 0, mac0_mode = 0, mac1_mode =0;
    BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_t tx_ctrl;

    *speed = 0;
    PHYMOD_IF_ERR_RETURN(READ_CDPORT_MODEr(phy, &reg_val));
    single_port_mode = BCMI_APERTA_CDMAC_CDPORT_MODEr_SINGLE_PORT_MODE_SPEED_400Gf_GET(reg_val);
    mac1_mode = BCMI_APERTA_CDMAC_CDPORT_MODEr_MAC1_PORT_MODEf_GET(reg_val);
    mac0_mode = BCMI_APERTA_CDMAC_CDPORT_MODEr_MAC0_PORT_MODEf_GET(reg_val);
    PHYMOD_CRIT_INFO(("Speed get single:%d mac1:%d mac0:%d lm:%x\n", single_port_mode, mac1_mode, mac0_mode,
            phy->access.lane_mask));
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_TX_CTRLr(phy, &tx_ctrl));
    if (BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_CRC_MODEf_GET(tx_ctrl) == 0x2) {
        *speed = 0;
        return PHYMOD_E_NONE;
    }

    if ((single_port_mode == 1) && (mac1_mode == CDMAC_4_LANES_TOGETHER) &&
        (mac0_mode == CDMAC_4_LANES_TOGETHER)) {
        *speed = 400000;
        return PHYMOD_E_NONE;
    } 
    if ((phy->access.lane_mask == 0xF) && mac0_mode == CDMAC_4_LANES_TOGETHER) {
         /* Can be 100G or 200G or 40G*/
        *speed = speed_to_configure;
    }
    if ((phy->access.lane_mask == 0xF0) && mac1_mode == CDMAC_4_LANES_TOGETHER) {
        /* Can be 100G or 200G or 40G*/
        *speed = speed_to_configure;
    }
    if (speed_to_configure == 10000 || speed_to_configure == 25000 || speed_to_configure == 50000) {
        /* Added above condition to avoid conflict with 100G/200G/40G*/
        if ((phy->access.lane_mask & 0xF) && (mac0_mode == CDMAC_4_LANES_SEPARATE)) {
            if ((phy->access.lane_mask != 0x3) && (phy->access.lane_mask != 0xC)) { /* Avoid 2 lane 50G*/
                /* Can be 10g or 25G or 50G*/
                *speed = speed_to_configure;
            }
        } else if ((phy->access.lane_mask & 0xF0) && (mac1_mode == CDMAC_4_LANES_SEPARATE)) {
            if ((phy->access.lane_mask != 0x30) && (phy->access.lane_mask != 0xC0)) { /* Avoid 2 lane 50G*/
                /* Can be 10g or 25G 0r 50G*/
                *speed = speed_to_configure;
            }
        }
    }
    if (((phy->access.lane_mask == 0x3) || (phy->access.lane_mask == 0xC)) && 
            (mac0_mode == CDMAC_2_LANES_DUAL)) {
        *speed = speed_to_configure;
    }
    if (((phy->access.lane_mask == 0x30) || (phy->access.lane_mask == 0xC0)) && 
            (mac1_mode == CDMAC_2_LANES_DUAL)) {
        *speed = speed_to_configure;
    }

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_encap_set(const plp_aperta_phymod_phy_access_t *phy, int flags,
                    portmod_encap_t encap)
{
    int val = 0;

    BCMI_APERTA_CDMAC_CDMAC_MODEr_t mac_mode;
    PHYMOD_MEMSET(&mac_mode, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_MODEr_t));

    /*
     * Only IEEE encap supHIGIG not supported.
     */
    switch(encap){
        case _SHR_PORT_ENCAP_HIGIG:
            val = 0;
            break;
        case _SHR_PORT_ENCAP_PREAMBLE_SOP_ONLY:
        /* no preamble or sfd, 0xFB is followed by mac da */
            val = 5;
            break;
        default:
            APERTA_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                               (APERTA_SOC_MSG("illegal encap mode %d"), encap));
            break;
    }

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_MODEr(phy, &mac_mode));
    aperta_soc_reg_field_set(phy, CDMAC_MODEr, mac_mode, HDR_MODEf, val);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_MODEr(phy, mac_mode));

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_encap_get(const plp_aperta_phymod_phy_access_t *phy, int *flags,
                    portmod_encap_t *encap)
{
    uint32_t fld_val;
    BCMI_APERTA_CDMAC_CDMAC_MODEr_t mac_mode;
    PHYMOD_MEMSET(&mac_mode, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_MODEr_t));


    (*flags) = 0;

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_MODEr(phy, &mac_mode));
    fld_val = aperta_soc_reg_field_get(phy, CDMAC_MODEr, mac_mode, HDR_MODEf);

    switch(fld_val){
        case 0:
            *encap = _SHR_PORT_ENCAP_HIGIG;
            break;
        case 5:
            *encap = _SHR_PORT_ENCAP_PREAMBLE_SOP_ONLY;
            break;
        default:
            APERTA_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (APERTA_SOC_MSG("unknown encap mode %d"), fld_val));
            break;
    }


    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_enable_set(const plp_aperta_phymod_phy_access_t *phy, int flags, int enable)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t mac_ctrl;
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &mac_ctrl));

    /* Don't disable TX since it stops egress and hangs if CPU sends */
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, mac_ctrl, TX_ENf, 1);
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, mac_ctrl, RX_ENf, enable? 1: 0);
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, mac_ctrl, SOFT_RESETf, enable? 0: 1);

    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_CTRLr(phy, mac_ctrl));

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_enable_get(const plp_aperta_phymod_phy_access_t *phy, int flags, int *enable)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t mac_ctrl;

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &mac_ctrl));

    if (flags & CDMAC_ENABLE_SET_FLAGS_TX_EN) {
        *enable = aperta_soc_reg_field_get(phy, CDMAC_CTRLr, mac_ctrl, TX_ENf);
    } else {
        *enable = aperta_soc_reg_field_get(phy, CDMAC_CTRLr, mac_ctrl, RX_ENf);
    }

    return (PHYMOD_E_NONE);
}

/*
 * This function can be called to either set/reset
 * the TX/RX or put the MAC in reset/bring the MAC
 * out of reset.
 * Only one operation allowed in a single call.
 */
int plp_aperta_cdmac_enable_selective_set(const plp_aperta_phymod_phy_access_t *phy,
                               int flags, int enable)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t mac_ctrl, orig_rval;

    PHYMOD_MEMSET(&mac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));
    PHYMOD_MEMSET(&orig_rval, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));

    if (!(flags == CDMAC_ENABLE_SET_FLAGS_TX_EN) &&
        !(flags == CDMAC_ENABLE_SET_FLAGS_RX_EN) &&
        !(flags == CDMAC_ENABLE_SET_FLAGS_SOFT_RESET)) {
        APERTA_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (APERTA_SOC_MSG("unknown control flag - %x"), flags));
    }

    /*
     * based on the flags passed the required fields are set/reset
     */

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &mac_ctrl));
    orig_rval.v[0] = mac_ctrl.v[0];

    if (flags == CDMAC_ENABLE_SET_FLAGS_TX_EN) {
        aperta_soc_reg_field_set(phy, CDMAC_CTRLr, mac_ctrl, TX_ENf, enable? 1: 0);
    }

    if (flags == CDMAC_ENABLE_SET_FLAGS_RX_EN) {
        aperta_soc_reg_field_set(phy, CDMAC_CTRLr, mac_ctrl, RX_ENf, enable? 1: 0);
    }

    /*
     * if enable = 0, bring mac out of reset, else mac in reset.
     */
    if (flags == CDMAC_ENABLE_SET_FLAGS_SOFT_RESET) {
        aperta_soc_reg_field_set(phy, CDMAC_CTRLr, mac_ctrl, SOFT_RESETf, enable? 1: 0);
    }

    /* write only if value changed */
    if (orig_rval.v[0] != mac_ctrl.v[0]) {
       PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_CTRLr(phy, mac_ctrl));
    }


    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_duplex_set(const plp_aperta_phymod_phy_access_t *phy, int duplex)
{

    if (!duplex) {
        APERTA_SOC_EXIT_WITH_ERR(PHYMOD_E_UNAVAIL,
                          (APERTA_SOC_MSG("half-duplex unsupported")));
    }

    /* Only duplex supnothing to be done here */

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_duplex_get(const plp_aperta_phymod_phy_access_t *phy, int *duplex)
{


    /* Only duplex support */
    *duplex = TRUE;

    return (PHYMOD_E_NONE);
}


int plp_aperta_cdmac_loopback_set(const plp_aperta_phymod_phy_access_t *phy, portmod_loopback_mode_t lb,
                       int enable)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t mac_ctrl;

    /* only line side loopback supported */
    if (lb == portmodLoopbackMacOuter) {
        PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &mac_ctrl));
        aperta_soc_reg_field_set(phy, CDMAC_CTRLr, mac_ctrl, LOCAL_LPBKf,
                          enable? 1: 0);
        PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_CTRLr(phy, mac_ctrl));
    } else {
        APERTA_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (APERTA_SOC_MSG("unsupported loopback type %d"), lb));
    }
    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_loopback_get(const plp_aperta_phymod_phy_access_t *phy, portmod_loopback_mode_t lb,
                       int *enable)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t mac_ctrl;

    if (lb == portmodLoopbackMacOuter) {
        PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &mac_ctrl));
        *enable = aperta_soc_reg_field_get(phy, CDMAC_CTRLr, mac_ctrl, LOCAL_LPBKf);
    } else {
        APERTA_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (APERTA_SOC_MSG("unsupported loopback type %d"), lb));
    }


    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_tx_mac_enable_set is the caller function
 */
int plp_aperta_cdmac_tx_enable_set (const plp_aperta_phymod_phy_access_t *phy, int enable)
{
    return(plp_aperta_cdmac_enable_selective_set(phy,
                        CDMAC_ENABLE_SET_FLAGS_TX_EN, enable));
}

/*
 * plp_aperta_portmod_port_tx_mac_enable_get is the caller function
 */
int plp_aperta_cdmac_tx_enable_get (const plp_aperta_phymod_phy_access_t *phy, int *enable)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t mac_ctrl;

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &mac_ctrl));
    *enable = aperta_soc_reg_field_get(phy, CDMAC_CTRLr, mac_ctrl, TX_ENf);

    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_rx_mac_enable_set is the caller function
 */
int plp_aperta_cdmac_rx_enable_set (const plp_aperta_phymod_phy_access_t *phy, int enable)
{

    return(plp_aperta_cdmac_enable_selective_set(phy,
                        CDMAC_ENABLE_SET_FLAGS_RX_EN, enable));
}

/*
 * plp_aperta_portmod_port_rx_mac_enable_get is the caller function
 */
int plp_aperta_cdmac_rx_enable_get (const plp_aperta_phymod_phy_access_t *phy, int *enable)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t mac_ctrl;

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &mac_ctrl));
    *enable = aperta_soc_reg_field_get(phy, CDMAC_CTRLr, mac_ctrl, RX_ENf);

    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_mac_reset_set is the caller function
 */
int plp_aperta_cdmac_soft_reset_set(const plp_aperta_phymod_phy_access_t *phy, int enable)
{
    return(plp_aperta_cdmac_enable_selective_set(phy,
                        CDMAC_ENABLE_SET_FLAGS_SOFT_RESET, enable));
}

/*
 * plp_aperta_portmod_port_mac_reset_set is the caller function
 */
int plp_aperta_cdmac_soft_reset_get(const plp_aperta_phymod_phy_access_t *phy, int *enable)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t mac_ctrl;

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &mac_ctrl));
    *enable = aperta_soc_reg_field_get(phy, CDMAC_CTRLr, mac_ctrl, SOFT_RESETf);

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_tx_mac_sa_set(const plp_aperta_phymod_phy_access_t *phy, uint8_t mac[6])
{
    BCMI_APERTA_CDMAC_CDMAC_TX_MAC_SAr_t mac_sa;
    uint32_t mac_addr=0;

    BCMI_APERTA_CDMAC_CDMAC_TX_MAC_SAr_CLR(mac_sa);

    /* high-order 8-bits of field value corresponds to mac[0],
       low-order 8-bits of field value corresponds to mac[5] */
    mac_addr = (mac[0] << 8) | (mac[1]) ;
    BCMI_APERTA_CDMAC_CDMAC_TX_MAC_SAr_SET(mac_sa,1,mac_addr);

    mac_addr = (mac[2] << 24) | (mac[3] << 16)| (mac[4] << 8) | (mac[5]) ;
    BCMI_APERTA_CDMAC_CDMAC_TX_MAC_SAr_SET(mac_sa,0,mac_addr);

    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_TX_MAC_SAr(phy, mac_sa));

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_tx_mac_sa_get(const plp_aperta_phymod_phy_access_t *phy, uint8_t mac[6])
{
    BCMI_APERTA_CDMAC_CDMAC_TX_MAC_SAr_t mac_sa;
    uint32_t umac = 0;

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_TX_MAC_SAr(phy, mac_sa));
    umac = BCMI_APERTA_CDMAC_CDMAC_TX_MAC_SAr_GET(mac_sa, 1);
    mac[0] = (umac & 0xFF00) >> 8;
    mac[1] = (umac & 0xFF);

    umac = BCMI_APERTA_CDMAC_CDMAC_TX_MAC_SAr_GET(mac_sa, 0);
    mac[2] = (umac & 0xFF000000) >> 24;
    mac[3] = (umac & 0x00FF0000) >> 16;
    mac[4] = (umac & 0x0000FF00) >> 8;
    mac[5] = (umac & 0x000000FF);

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_rx_mac_sa_set(const plp_aperta_phymod_phy_access_t *phy, pm_mac_addr_t mac)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_MAC_SAr_t mac_sa;
    uint32_t mac_addr=0;

    BCMI_APERTA_CDMAC_CDMAC_RX_MAC_SAr_CLR(mac_sa);

    /* high-order 8-bits of field value corresponds to mac[0],
       low-order 8-bits of field value corresponds to mac[5] */
    mac_addr = (mac[0] << 8) | (mac[1]) ;
    BCMI_APERTA_CDMAC_CDMAC_RX_MAC_SAr_SET(mac_sa,1,mac_addr);

    mac_addr = (mac[2] << 24) | (mac[3] << 16)| (mac[4] << 8) | (mac[5]) ;
    BCMI_APERTA_CDMAC_CDMAC_RX_MAC_SAr_SET(mac_sa,0,mac_addr);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_RX_MAC_SAr(phy, mac_sa));

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_rx_mac_sa_get(const plp_aperta_phymod_phy_access_t *phy, pm_mac_addr_t mac)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_MAC_SAr_t mac_sa;
    uint32_t u_mac = 0;

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_MAC_SAr(phy, mac_sa));
    u_mac = BCMI_APERTA_CDMAC_CDMAC_RX_MAC_SAr_GET(mac_sa, 1);
    mac[0] = (u_mac & 0xFF00) >> 8;
    mac[1] = (u_mac & 0xFF);

    u_mac = BCMI_APERTA_CDMAC_CDMAC_RX_MAC_SAr_GET(mac_sa, 0);
    mac[2] = (u_mac & 0xFF000000) >> 24;
    mac[3] = (u_mac & 0x00FF0000) >> 16;
    mac[4] = (u_mac & 0x0000FF00) >> 8;
    mac[5] = (u_mac & 0x000000FF);

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_rx_vlan_tag_set(const plp_aperta_phymod_phy_access_t *phy, int outer_vlan_tag,
                          int inner_vlan_tag)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_VLAN_TAGr_t rx_vlan;

    PHYMOD_MEMSET(&rx_vlan, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_VLAN_TAGr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_VLAN_TAGr(phy, &rx_vlan));

    if(inner_vlan_tag == -1) {
       aperta_soc_reg_field_set(phy, CDMAC_RX_VLAN_TAGr, rx_vlan,
                             INNER_VLAN_TAG_ENABLEf, 0);
    } else {
       aperta_soc_reg_field_set(phy, CDMAC_RX_VLAN_TAGr, rx_vlan,
                             INNER_VLAN_TAGf, inner_vlan_tag);
       aperta_soc_reg_field_set(phy, CDMAC_RX_VLAN_TAGr, rx_vlan,
                             INNER_VLAN_TAG_ENABLEf, 1);
    }

    if(outer_vlan_tag == -1) {
       aperta_soc_reg_field_set(phy, CDMAC_RX_VLAN_TAGr, rx_vlan,
                             OUTER_VLAN_TAG_ENABLEf, 0);
    } else {
       aperta_soc_reg_field_set(phy, CDMAC_RX_VLAN_TAGr, rx_vlan,
                             OUTER_VLAN_TAGf, outer_vlan_tag);
       aperta_soc_reg_field_set(phy, CDMAC_RX_VLAN_TAGr, rx_vlan,
                             OUTER_VLAN_TAG_ENABLEf, 1);
    }
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_RX_VLAN_TAGr(phy, rx_vlan));


    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_rx_vlan_tag_get(const plp_aperta_phymod_phy_access_t *phy, int *outer_vlan_tag,
                          int *inner_vlan_tag)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_VLAN_TAGr_t rx_vlan;
    uint32_t is_enabled = 0;

    PHYMOD_MEMSET(&rx_vlan, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_VLAN_TAGr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_VLAN_TAGr(phy, &rx_vlan));

    is_enabled = aperta_soc_reg_field_get(phy, CDMAC_RX_VLAN_TAGr, rx_vlan,
                                       INNER_VLAN_TAG_ENABLEf);
    if(is_enabled == 0) {
       *inner_vlan_tag = -1;
    } else {
       *inner_vlan_tag = aperta_soc_reg_field_get(phy, CDMAC_RX_VLAN_TAGr,
                                               rx_vlan, INNER_VLAN_TAGf);
    }

    is_enabled = aperta_soc_reg_field_get(phy, CDMAC_RX_VLAN_TAGr, rx_vlan,
                                       OUTER_VLAN_TAG_ENABLEf);
    if(is_enabled == 0) {
       *outer_vlan_tag = -1;
    } else {
       *outer_vlan_tag = aperta_soc_reg_field_get(phy, CDMAC_RX_VLAN_TAGr,
                                               rx_vlan, OUTER_VLAN_TAGf);
    }
    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_rx_max_size_set(const plp_aperta_phymod_phy_access_t *phy, int value)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_MAX_SIZEr_t rval;

    PHYMOD_MEMSET(&rval, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_MAX_SIZEr_t));
    /*
     * Maximum packet size in receive direction, exclusive of preamble
     * and  CRC in strip mode. Packets greater than this size are
     * truncated to this value.
     */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_MAX_SIZEr(phy, &rval));
    aperta_soc_reg_field_set(phy, CDMAC_RX_MAX_SIZEr, rval, RX_MAX_SIZEf, value);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_RX_MAX_SIZEr(phy, rval));


    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_rx_max_size_get(const plp_aperta_phymod_phy_access_t *phy, int *value)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_MAX_SIZEr_t rval;

    PHYMOD_MEMSET(&rval, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_MAX_SIZEr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_MAX_SIZEr(phy, &rval));
    *value = aperta_soc_reg_field_get(phy, CDMAC_RX_MAX_SIZEr, rval, RX_MAX_SIZEf);


    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_pad_size_set(const plp_aperta_phymod_phy_access_t *phy, int value)
{
    BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_t rval;

    PHYMOD_MEMSET(&rval, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_t));
    if ((value < CDMAC_PAD_THRESHOLD_SIZE_MIN) ||
        (value > CDMAC_PAD_THRESHOLD_SIZE_MAX)) {
        if (value != 0)  {
            APERTA_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                   (APERTA_SOC_MSG("unsupported pad threshold size %d"), value));
        }
    }

    /*
     * If PAD_EN is set, packets smaller than PAD_THRESHOLD are padded
     * to this size. PAD_THRESHOLD must be >=64 and <= 96.
     */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_TX_CTRLr(phy, &rval));
    aperta_soc_reg_field_set(phy, CDMAC_TX_CTRLr, rval, PAD_ENf, value? 1: 0);
    aperta_soc_reg_field_set(phy, CDMAC_TX_CTRLr, rval, PAD_THRESHOLDf, value);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_TX_CTRLr(phy, rval));


    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_pad_size_get(const plp_aperta_phymod_phy_access_t *phy, int *value)
{
    BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_t rval;

    PHYMOD_MEMSET(&rval, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_t));
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_TX_CTRLr(phy, &rval));
    *value = aperta_soc_reg_field_get(phy, CDMAC_TX_CTRLr, rval, PAD_THRESHOLDf);

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_tx_average_ipg_set(const plp_aperta_phymod_phy_access_t *phy, int val)
{
    BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_t rval;

    PHYMOD_MEMSET(&rval, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_t));
    /*
     * Average inter packet gap can be in the range 8 to 56 or 60.
     * should be 56 for XLGMII, 60 for XGMII,
     * default is 12.
     * Granularity is in bytes.
     * Input 'val' is in bits.
     */
    if ((val < CDMAC_AVE_IPG_MIN) || (val > CDMAC_AVE_IPG_MAX)) {
       APERTA_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (APERTA_SOC_MSG("Average IPG is out of range.")));
    }

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_TX_CTRLr(phy, &rval));
    aperta_soc_reg_field_set(phy, CDMAC_TX_CTRLr, rval, AVERAGE_IPGf, (val/8));
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_TX_CTRLr(phy, rval));

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_tx_average_ipg_get(const plp_aperta_phymod_phy_access_t *phy, int *val)
{
    BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_t rval;

    PHYMOD_MEMSET(&rval, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_t));

    /*
     * Register value is in bytes.
     * Output 'val' is in bits.
     */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_TX_CTRLr(phy, &rval));
    *val = aperta_soc_reg_field_get(phy, CDMAC_TX_CTRLr, rval, AVERAGE_IPGf) * 8;
    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_runt_threshold_set(const plp_aperta_phymod_phy_access_t *phy, int value)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t rx_ctrl;

    PHYMOD_MEMSET(&rx_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t));
    /*
     * The threshold, below which the packets are dropped or
     * marked as runt. Should be programmed >=64 and <= 96 bytes.
     */
    if ((value < CDMAC_RUNT_THRESHOLD_MIN) ||
        (value > CDMAC_RUNT_THRESHOLD_MAX)) {
        APERTA_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
            (APERTA_SOC_MSG("runt size should be greater than %d and "
            "smaller than %d. got %d"), CDMAC_RUNT_THRESHOLD_MIN,
            CDMAC_RUNT_THRESHOLD_MAX,  value));
    }
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_CTRLr(phy, &rx_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_RX_CTRLr, rx_ctrl, RUNT_THRESHOLDf, value);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_RX_CTRLr(phy, rx_ctrl));

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_runt_threshold_get(const plp_aperta_phymod_phy_access_t *phy, int *value)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t rx_ctrl;

    PHYMOD_MEMSET(&rx_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_CTRLr(phy, &rx_ctrl));
    *value = aperta_soc_reg_field_get(phy, CDMAC_RX_CTRLr, rx_ctrl, RUNT_THRESHOLDf);


    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_remote_fault_control_set is the caller of this function.
 */
int plp_aperta_cdmac_remote_fault_control_set(const plp_aperta_phymod_phy_access_t *phy,
                          const portmod_remote_fault_control_t *control)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_LSS_CTRLr_t rx_lss;

    PHYMOD_MEMSET(&rx_lss, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_LSS_CTRLr_t));
    /*
     * REMOTE_FAULT_DISABLE determines the transmit response during remote
     * fault state. The REMOTE_FAULT_STATUS bit is always updated irrespective
     * of this configuration.
     * If set, MAC will continue to transmit data irrespective of
     *  REMOTE_FAULT_STATUS.
     * If reset, MAC transmit behavior is governed by
     * DROP_TX_DATA_ON_REMOTE_FAULT configuration.
     */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_LSS_CTRLr(phy, &rx_lss));

    aperta_soc_reg_field_set(phy, CDMAC_RX_LSS_CTRLr, rx_lss, REMOTE_FAULT_DISABLEf,
                      control->enable? 0: 1); /* flip */
    aperta_soc_reg_field_set(phy, CDMAC_RX_LSS_CTRLr, rx_lss,
                      DROP_TX_DATA_ON_REMOTE_FAULTf,
                      control->drop_tx_on_fault? 1: 0);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_RX_LSS_CTRLr(phy, rx_lss));


    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_remote_fault_control_get is the caller of this function.
 */
int plp_aperta_cdmac_remote_fault_control_get(const plp_aperta_phymod_phy_access_t *phy,
                                   portmod_remote_fault_control_t *control)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_LSS_CTRLr_t rx_lss;
    uint32_t fval = 0;

    PHYMOD_MEMSET(&rx_lss, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_LSS_CTRLr_t));
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_LSS_CTRLr(phy, &rx_lss));

    fval = aperta_soc_reg_field_get(phy, CDMAC_RX_LSS_CTRLr, rx_lss,
                             REMOTE_FAULT_DISABLEf);
    /* if fval is reset, indicates enable */
    control->enable = (fval? 0: 1);

    fval = aperta_soc_reg_field_get(phy, CDMAC_RX_LSS_CTRLr, rx_lss,
                             DROP_TX_DATA_ON_REMOTE_FAULTf);
    control->drop_tx_on_fault = (fval? 1: 0);

    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_remote_fault_status_get is the caller of this function.
 * The status bit is clear on read, no seperate api for status_clear
 * required.
 */
int plp_aperta_cdmac_remote_fault_status_get(const plp_aperta_phymod_phy_access_t *phy, int *status)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_LSS_STATUSr_t rx_lss;
    uint32_t fval;

    PHYMOD_MEMSET(&rx_lss, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_LSS_STATUSr_t));
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_LSS_STATUSr(phy, &rx_lss));

    /* fault status bits clear on read */
    fval = aperta_soc_reg_field_get(phy, CDMAC_RX_LSS_STATUSr, rx_lss,
                             REMOTE_FAULT_STATUSf);
    *status = (fval? 1: 0);

    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_local_fault_control_set is the caller of this function.
 */
int plp_aperta_cdmac_local_fault_control_set(const plp_aperta_phymod_phy_access_t *phy,
                                  const portmod_local_fault_control_t *control)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_LSS_CTRLr_t rx_lss;

    PHYMOD_MEMSET(&rx_lss, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_LSS_CTRLr_t));
    /*
     * LOCALFAULT_DISABLE determines the transmit response during remote
     * fault state. The LOCAL_FAULT_STATUS bit is always updated irrespective
     * of this configuration.
     * If set, MAC will continue to transmit data irrespective of
     *  REMOTE_FAULT_STATUS.
     * If reset, MAC transmit behavior is governed by
     * DROP_TX_DATA_ON_LOCAL_FAULT configuration.
     */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_LSS_CTRLr(phy, &rx_lss));

    aperta_soc_reg_field_set(phy, CDMAC_RX_LSS_CTRLr, rx_lss, LOCAL_FAULT_DISABLEf,
                      control->enable? 0: 1); /* flip */
    aperta_soc_reg_field_set(phy, CDMAC_RX_LSS_CTRLr, rx_lss,
                      DROP_TX_DATA_ON_LOCAL_FAULTf,
                      control->drop_tx_on_fault? 1: 0);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_RX_LSS_CTRLr(phy, rx_lss));


    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_local_fault_control_get is the caller of this function.
 */
int plp_aperta_cdmac_local_fault_control_get(const plp_aperta_phymod_phy_access_t *phy,
                                  portmod_local_fault_control_t *control)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_LSS_CTRLr_t rx_lss;
    uint32_t fval = 0;

    PHYMOD_MEMSET(&rx_lss, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_LSS_CTRLr_t));


    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_LSS_CTRLr(phy, &rx_lss));

    fval = aperta_soc_reg_field_get(phy, CDMAC_RX_LSS_CTRLr, rx_lss,
                             LOCAL_FAULT_DISABLEf);
    control->enable = (fval? 0: 1);

    fval = aperta_soc_reg_field_get(phy, CDMAC_RX_LSS_CTRLr, rx_lss,
                             DROP_TX_DATA_ON_LOCAL_FAULTf);
    control->drop_tx_on_fault = (fval? 1: 0);


    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_local_fault_status_get is the caller of this function.
 * The status bit is clear on read, no seperate api for status_clear
 * required.
 */
int plp_aperta_cdmac_local_fault_status_get(const plp_aperta_phymod_phy_access_t *phy, int *status)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_LSS_STATUSr_t rx_lss;
    uint32_t fval = 0;

    PHYMOD_MEMSET(&rx_lss, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_LSS_STATUSr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_LSS_STATUSr(phy, &rx_lss));
    /* fault status bits clear on read */
    fval = aperta_soc_reg_field_get(phy, CDMAC_RX_LSS_STATUSr, rx_lss,
                             LOCAL_FAULT_STATUSf);
    *status = (fval? 1: 0);


    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_pfc_control_set is the caller of this function.
 */
int plp_aperta_cdmac_pfc_control_set(const plp_aperta_phymod_phy_access_t *phy,
                         const portmod_pfc_control_t *control)
{
    BCMI_APERTA_CDMAC_CDMAC_PAUSE_CTRLr_t pause_ctrl;
    BCMI_APERTA_CDMAC_CDMAC_PFC_CTRLr_t pfc_ctrl;

    PHYMOD_MEMSET(&pause_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PAUSE_CTRLr_t));
    PHYMOD_MEMSET(&pfc_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PFC_CTRLr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PAUSE_CTRLr(phy, &pause_ctrl));

    if(control->refresh_timer > 0 ) {
       aperta_soc_reg_field_set(phy, CDMAC_PAUSE_CTRLr, pause_ctrl,
                         PFC_REFRESH_TIMERf, control->refresh_timer);
       aperta_soc_reg_field_set(phy, CDMAC_PAUSE_CTRLr, pause_ctrl,
                             PFC_REFRESH_ENf, 1);
    } else {
       aperta_soc_reg_field_set(phy, CDMAC_PAUSE_CTRLr, pause_ctrl,
                             PFC_REFRESH_ENf, 0);
    }
    aperta_soc_reg_field_set(phy, CDMAC_PAUSE_CTRLr, pause_ctrl,
                      PFC_XOFF_TIMERf, control->xoff_timer);

    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_PAUSE_CTRLr(phy, pause_ctrl));


    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PFC_CTRLr(phy, &pfc_ctrl));

    aperta_soc_reg_field_set(phy, CDMAC_PFC_CTRLr, pfc_ctrl, PFC_STATS_ENf,
                      control->stats_en);
    aperta_soc_reg_field_set(phy, CDMAC_PFC_CTRLr, pfc_ctrl, FORCE_PFC_XONf,
                      control->force_xon);
    aperta_soc_reg_field_set(phy, CDMAC_PFC_CTRLr, pfc_ctrl, TX_PFC_ENf,
                      control->tx_enable);
    aperta_soc_reg_field_set(phy, CDMAC_PFC_CTRLr, pfc_ctrl, RX_PFC_ENf,
                      control->rx_enable);

    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_PFC_CTRLr(phy, pfc_ctrl));
    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_pfc_control_get(const plp_aperta_phymod_phy_access_t *phy,
                          portmod_pfc_control_t *control)
{
    BCMI_APERTA_CDMAC_CDMAC_PAUSE_CTRLr_t pause_ctrl;
    BCMI_APERTA_CDMAC_CDMAC_PFC_CTRLr_t pfc_ctrl;
    uint32_t refresh_enable = 0;
    uint32_t refresh_timer = 0;

    PHYMOD_MEMSET(&pause_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PAUSE_CTRLr_t));
    PHYMOD_MEMSET(&pfc_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PFC_CTRLr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PAUSE_CTRLr(phy, &pause_ctrl));

    refresh_enable = aperta_soc_reg_field_get(phy, CDMAC_PAUSE_CTRLr, pause_ctrl,
                                       PFC_REFRESH_ENf);
    refresh_timer = aperta_soc_reg_field_get(phy, CDMAC_PAUSE_CTRLr, pause_ctrl,
                                      PFC_REFRESH_TIMERf);

    control->refresh_timer = (refresh_enable? refresh_timer: 0);
    control->xoff_timer = aperta_soc_reg_field_get(phy, CDMAC_PAUSE_CTRLr,
                                                pause_ctrl, PFC_XOFF_TIMERf);

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PFC_CTRLr(phy, &pfc_ctrl));

    control->stats_en = aperta_soc_reg_field_get(phy, CDMAC_PFC_CTRLr,  pfc_ctrl,
                                           PFC_STATS_ENf);
    control->force_xon = aperta_soc_reg_field_get(phy, CDMAC_PFC_CTRLr, pfc_ctrl,
                                           FORCE_PFC_XONf);
    control->tx_enable = aperta_soc_reg_field_get(phy, CDMAC_PFC_CTRLr, pfc_ctrl,
                                           TX_PFC_ENf);
    control->rx_enable = aperta_soc_reg_field_get(phy, CDMAC_PFC_CTRLr, pfc_ctrl,
                                           RX_PFC_ENf);
    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_pause_control_set(const plp_aperta_phymod_phy_access_t *phy,
                            const portmod_pause_control_t *control)
{
    BCMI_APERTA_CDMAC_CDMAC_PAUSE_CTRLr_t pause_ctrl;
    PHYMOD_MEMSET(&pause_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PAUSE_CTRLr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PAUSE_CTRLr(phy, &pause_ctrl));

    if(control->rx_enable || control->tx_enable) {
       if(control->refresh_timer > 0 ) {
          aperta_soc_reg_field_set(phy, CDMAC_PAUSE_CTRLr, pause_ctrl,
                            PAUSE_REFRESH_TIMERf, control->refresh_timer);
          aperta_soc_reg_field_set(phy, CDMAC_PAUSE_CTRLr, pause_ctrl,
                            PAUSE_REFRESH_ENf, 1);
       } else {
          aperta_soc_reg_field_set(phy, CDMAC_PAUSE_CTRLr, pause_ctrl,
                            PAUSE_REFRESH_ENf, 0);
       }
       aperta_soc_reg_field_set(phy, CDMAC_PAUSE_CTRLr, pause_ctrl,
                         PAUSE_XOFF_TIMERf, control->xoff_timer);
    }

    aperta_soc_reg_field_set(phy, CDMAC_PAUSE_CTRLr, pause_ctrl , TX_PAUSE_ENf,
                      control->tx_enable);
    aperta_soc_reg_field_set(phy, CDMAC_PAUSE_CTRLr, pause_ctrl , RX_PAUSE_ENf,
                      control->rx_enable);

    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_PAUSE_CTRLr(phy, pause_ctrl));
    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_pause_control_get(const plp_aperta_phymod_phy_access_t *phy,
                            portmod_pause_control_t *control)
{
    BCMI_APERTA_CDMAC_CDMAC_PAUSE_CTRLr_t pause_ctrl;
    uint32_t refresh_enable;
    uint32_t refresh_timer;

    PHYMOD_MEMSET(&pause_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PAUSE_CTRLr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PAUSE_CTRLr(phy, &pause_ctrl));

    refresh_enable = aperta_soc_reg_field_get(phy, CDMAC_PAUSE_CTRLr, pause_ctrl,
                                       PAUSE_REFRESH_ENf);
    refresh_timer = aperta_soc_reg_field_get(phy, CDMAC_PAUSE_CTRLr, pause_ctrl,
                                      PAUSE_REFRESH_TIMERf);

    control->refresh_timer = (refresh_enable? refresh_timer: -1);
    control->xoff_timer = aperta_soc_reg_field_get(phy, CDMAC_PAUSE_CTRLr,
                                                pause_ctrl, PAUSE_XOFF_TIMERf);

    control->tx_enable = aperta_soc_reg_field_get(phy, CDMAC_PAUSE_CTRLr, pause_ctrl,
                                           TX_PAUSE_ENf);
    control->rx_enable = aperta_soc_reg_field_get(phy, CDMAC_PAUSE_CTRLr, pause_ctrl,
                                           RX_PAUSE_ENf);
    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_pfc_config_set (const plp_aperta_phymod_phy_access_t *phy,
                          const portmod_pfc_config_t* pfc_cfg)
{
    BCMI_APERTA_CDMAC_CDMAC_PFC_TYPEr_t pfc_type;
    BCMI_APERTA_CDMAC_CDMAC_PFC_OPCODEr_t pfc_opcode;
    BCMI_APERTA_CDMAC_CDMAC_PFC_DAr_t pfc_da;
    uint32_t fval[2] = {0};

    PHYMOD_MEMSET(&pfc_type, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PFC_TYPEr_t));
    PHYMOD_MEMSET(&pfc_opcode, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PFC_OPCODEr_t));
    PHYMOD_MEMSET(&pfc_da, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PFC_DAr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PFC_TYPEr(phy, &pfc_type));
    aperta_soc_reg_field_set(phy, CDMAC_PFC_TYPEr, pfc_type, PFC_ETH_TYPEf,
                      pfc_cfg->type);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_PFC_TYPEr(phy, pfc_type));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PFC_OPCODEr(phy, &pfc_opcode));
    aperta_soc_reg_field_set(phy, CDMAC_PFC_OPCODEr, pfc_opcode, PFC_OPCODEf,
                      pfc_cfg->opcode);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_PFC_OPCODEr(phy, pfc_opcode));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PFC_DAr(phy, &pfc_da));
    fval[0] |= pfc_cfg->da_nonoui;
    fval[0] |= (pfc_cfg->da_oui & 0xff) << 24;
    fval[1] |= (pfc_cfg->da_oui & 0xffff00) >> 8;

    BCMI_APERTA_CDMAC_CDMAC_PFC_DAr_PFC_MACDAf_SET(pfc_da, fval);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_PFC_DAr(phy, pfc_da));

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_pfc_config_get (const plp_aperta_phymod_phy_access_t *phy, portmod_pfc_config_t* pfc_cfg)
{
    BCMI_APERTA_CDMAC_CDMAC_PFC_TYPEr_t pfc_type;
    BCMI_APERTA_CDMAC_CDMAC_PFC_OPCODEr_t pfc_code;
    BCMI_APERTA_CDMAC_CDMAC_PFC_DAr_t pfc_da;
    uint32_t fval[2] = {0};

    PHYMOD_MEMSET(&pfc_type, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PFC_TYPEr_t));
    PHYMOD_MEMSET(&pfc_code, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PFC_OPCODEr_t));
    PHYMOD_MEMSET(&pfc_da, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PFC_DAr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PFC_TYPEr(phy, &pfc_type));
    pfc_cfg->type = aperta_soc_reg_field_get(phy, CDMAC_PFC_TYPEr, pfc_type,
                                      PFC_ETH_TYPEf);

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PFC_OPCODEr(phy, &pfc_code));
    pfc_cfg->opcode = aperta_soc_reg_field_get(phy, CDMAC_PFC_OPCODEr, pfc_code,
                                        PFC_OPCODEf);

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PFC_DAr(phy, &pfc_da));
    fval[0] = pfc_da.v[0];
    fval[1] = pfc_da.v[1];

    pfc_cfg->da_nonoui = fval[0] & 0xffffff;
    pfc_cfg->da_oui = (fval[1] << 8) | (fval[0] >> 24);

    return (PHYMOD_E_NONE);
}


int plp_aperta_cdmac_pass_control_frame_set(const plp_aperta_phymod_phy_access_t *phy, int value)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t rx_ctrl;

    PHYMOD_MEMSET(&rx_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t));
    /*
     * This configuration is used to drop or pass all control frames
     * (with ether type 0x8808) except pause packets.
     * If set, all control frames are passed to system side.
     * if reset, control frames (including pfc frames wih ether type 0x8808)i
     * are dropped in CDMAC.
     */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_CTRLr(phy, &rx_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_RX_CTRLr, rx_ctrl,
                      RX_PASS_CTRLf, (value? 1: 0));
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_RX_CTRLr(phy, rx_ctrl));

    return (PHYMOD_E_NONE);
}


int plp_aperta_cdmac_pass_control_frame_get(const plp_aperta_phymod_phy_access_t *phy, int *value)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t rx_ctrl;

    PHYMOD_MEMSET(&rx_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_CTRLr(phy, &rx_ctrl));
    *value = aperta_soc_reg_field_get(phy, CDMAC_RX_CTRLr, rx_ctrl, RX_PASS_CTRLf);

    return (PHYMOD_E_NONE);
}


int plp_aperta_cdmac_pass_pfc_frame_set(const plp_aperta_phymod_phy_access_t *phy, int value)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t rx_ctrl;

    PHYMOD_MEMSET(&rx_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t));

    /*
     * This configuration is used to pass or drop PFC packets when
     * PFC_ETH_TYPE is not equal to 0x8808.
     * If set, PFC frames are passed to system side.
     * If reset, PFC frames are dropped in CDMAC.
     */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_CTRLr(phy, &rx_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_RX_CTRLr, rx_ctrl,
                      RX_PASS_PFCf, (value? 1: 0));
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_RX_CTRLr(phy, rx_ctrl));

    return (PHYMOD_E_NONE);
}


int plp_aperta_cdmac_pass_pfc_frame_get(const plp_aperta_phymod_phy_access_t *phy, int *value)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t rx_ctrl;

    PHYMOD_MEMSET(&rx_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_CTRLr(phy, &rx_ctrl));
    *value = aperta_soc_reg_field_get(phy, CDMAC_RX_CTRLr, rx_ctrl, RX_PASS_PFCf);

    return (PHYMOD_E_NONE);
}


int plp_aperta_cdmac_pass_pause_frame_set(const plp_aperta_phymod_phy_access_t *phy, int value)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t rx_ctrl;

    PHYMOD_MEMSET(&rx_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t));

    /*
     * If set, PAUSE frames are passed to sytem side.
     * If reset, PAUSE frames are dropped in CDMAC
     */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_CTRLr(phy, &rx_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_RX_CTRLr, rx_ctrl,
                      RX_PASS_PAUSEf, (value? 1: 0));
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_RX_CTRLr(phy, rx_ctrl));

    return (PHYMOD_E_NONE);
}


int plp_aperta_cdmac_pass_pause_frame_get(const plp_aperta_phymod_phy_access_t *phy, int *value)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t rx_ctrl;

    PHYMOD_MEMSET(&rx_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_CTRLr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_CTRLr(phy, &rx_ctrl));
    *value = aperta_soc_reg_field_get(phy, CDMAC_RX_CTRLr, rx_ctrl, RX_PASS_PAUSEf);

    return (PHYMOD_E_NONE);
}

/*
 * LAG FAILOVER
 * Following routines are used and invoked for the LAG
 * failover feature.
 * When a LAG(Trunk) member port goes down, if LAG failover
 * is enabled all the tx packets are loopback to RX path.
 * This is done by enabling loopback on the port which went
 * down, when the port comes back up, the software detects
 * the LINK UP event and removes the loopback.
 * Programming Sequence:
 * CDMAC_CTRL.LAG_FAILOVER_ENf 1/0, enable/disable feature.
 *                             If set, enable LAG Failover.
 * This bit has priority over LOCAL_LPBK. The lag failover
 * kicks in when the link status selected by LINK_STATUS_SELECT
 * transitions from 1 to 0. TSC clock and TSC credits must
 * be active for lag failover.
 *
 * plp_aperta_portmod_port_trunk_hwfailover_config_set is the caller
 * of this function, which in turn is called by the bcm
 * application layer function calls handling the lag  hardware
 * failover feature.
 */
int plp_aperta_cdmac_lag_failover_en_set(const plp_aperta_phymod_phy_access_t *phy, int val)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;
    PHYMOD_MEMSET(&cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, LAG_FAILOVER_ENf, val);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_CTRLr(phy, cdmac_ctrl));

    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_trunk_hwfailover_config_get is the caller
 * of this function, returns if the hw lag failover feature
 * is enabled on a port.
 */
int plp_aperta_cdmac_lag_failover_en_get(const plp_aperta_phymod_phy_access_t *phy, int *val)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;
    PHYMOD_MEMSET(&cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));
    *val = aperta_soc_reg_field_get(phy, CDMAC_CTRLr, cdmac_ctrl, LAG_FAILOVER_ENf);

    return (PHYMOD_E_NONE);
}

/*
 * This function is used to disable the LAG failover feature
 * on a port.
 * plp_aperta_portmod_port_lag_failover_disable is the caller
 * of this function, which in turn is called by the bcm
 * application layer function calls handling the lag hardware
 * failover feature.
 */
int plp_aperta_cdmac_lag_failover_disable(const plp_aperta_phymod_phy_access_t *phy)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;
    PHYMOD_MEMSET(&cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));

    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, LAG_FAILOVER_ENf, 0);
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, LINK_STATUS_SELECTf, 0);
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, REMOVE_FAILOVER_LPBKf, 0);

    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_CTRLr(phy, cdmac_ctrl));

    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_lag_failover_loopback_set is the caller
 * of this function.
 */
int plp_aperta_cdmac_lag_failover_loopback_set(const plp_aperta_phymod_phy_access_t *phy, int val)
{
    BCMI_APERTA_CDMAC_CDMAC_LAG_FAILOVER_STATUSr_t failover_sts;

    PHYMOD_MEMSET(&failover_sts, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_LAG_FAILOVER_STATUSr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_LAG_FAILOVER_STATUSr(phy, &failover_sts));
    /* If set, indicates that the port is in lag failover state */
    aperta_soc_reg_field_set(phy, CDMAC_LAG_FAILOVER_STATUSr, failover_sts,
                      LAG_FAILOVER_LOOPBACKf, val);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_LAG_FAILOVER_STATUSr(phy, failover_sts));

    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_lag_failover_loopback_get is the caller
 * of this function. This function is invoked as part of
 * LINK events from LINKSCAN thread
 */
int plp_aperta_cdmac_lag_failover_loopback_get(const plp_aperta_phymod_phy_access_t *phy, int *val)
{
    BCMI_APERTA_CDMAC_CDMAC_LAG_FAILOVER_STATUSr_t failover_sts;

    PHYMOD_MEMSET(&failover_sts, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_LAG_FAILOVER_STATUSr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_LAG_FAILOVER_STATUSr(phy, &failover_sts));
    /*
     * The LAG_FAILOVER_LOOPBACK is SET by the hardware if
     * the LINK goes down and LAG_FAILOVER_ENf is set on the port.
     */
    *val = aperta_soc_reg_field_get(phy, CDMAC_LAG_FAILOVER_STATUSr, failover_sts,
                             LAG_FAILOVER_LOOPBACKf);

    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_lag_remove_failover_lpbk_set is the caller
 * of this function. This function is invoked as part of
 * LINK events from LINKSCAN thread
 */
int plp_aperta_cdmac_lag_remove_failover_lpbk_set(const plp_aperta_phymod_phy_access_t *phy, int val)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;

    PHYMOD_MEMSET(&cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));
    /*
     * If set, CDMAC port will move from lag failover state to
     * normal operation. This bit should be set after link is up.
     * A transition from 0 to 1 is required, so first a write with
     * 0 should be followed by a write with 1.
     * The BCM layer should take care of the calling sequnce as
     * part of LINK events.
     */
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, REMOVE_FAILOVER_LPBKf, val);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_CTRLr(phy, cdmac_ctrl));

    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_lag_remove_failover_lpbk_get is the caller
 * of this function.
 */
int plp_aperta_cdmac_lag_remove_failover_lpbk_get(const plp_aperta_phymod_phy_access_t *phy, int *val)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;

    PHYMOD_MEMSET(&cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));
    *val = aperta_soc_reg_field_get(phy, CDMAC_CTRLr, cdmac_ctrl, REMOVE_FAILOVER_LPBKf);

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_reset_fc_timers_on_link_dn_get(const plp_aperta_phymod_phy_access_t *phy, int *val)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_LSS_CTRLr_t lss_ctrl;

    PHYMOD_MEMSET(&lss_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_LSS_CTRLr_t));
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_LSS_CTRLr(phy, &lss_ctrl));
    *val = aperta_soc_reg_field_get(phy, CDMAC_RX_LSS_CTRLr, lss_ctrl,
                             RESET_FLOW_CONTROL_TIMERS_ON_LINK_DOWNf);

    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_trunk_hwfailover_config_set is the caller of this
 * function. This function is invoked during lag failover config
 * on a port.
 */
int plp_aperta_cdmac_reset_fc_timers_on_link_dn_set(const plp_aperta_phymod_phy_access_t *phy, int val)
{
    BCMI_APERTA_CDMAC_CDMAC_RX_LSS_CTRLr_t lss_ctrl;

    PHYMOD_MEMSET(&lss_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_RX_LSS_CTRLr_t));

    /*
     * If set, the receive pause and PFC timers are reset whenever the link
     * status is down, or local or remote faults are received.
     */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RX_LSS_CTRLr(phy, &lss_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_RX_LSS_CTRLr, lss_ctrl,
                      RESET_FLOW_CONTROL_TIMERS_ON_LINK_DOWNf, val);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_RX_LSS_CTRLr(phy, lss_ctrl));

    return (PHYMOD_E_NONE);
}


int plp_aperta_cdmac_mac_ctrl_set(const plp_aperta_phymod_phy_access_t *phy, uint64_t ctrl)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;

    PHYMOD_MEMSET(&cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));

    BCMI_APERTA_CDMAC_CDMAC_CTRLr_SET(cdmac_ctrl, ctrl);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_CTRLr(phy, cdmac_ctrl));
    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_drain_cell_get(const plp_aperta_phymod_phy_access_t *phy,
                         portmod_drain_cells_t *drain_cells)
{
    BCMI_APERTA_CDMAC_CDMAC_PFC_CTRLr_t pfc_ctrl;
    BCMI_APERTA_CDMAC_CDMAC_PAUSE_CTRLr_t pause_ctrl;

    PHYMOD_MEMSET(&pfc_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PFC_CTRLr_t));
    PHYMOD_MEMSET(&pause_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PAUSE_CTRLr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PFC_CTRLr(phy, &pfc_ctrl));
    drain_cells->rx_pfc_en = aperta_soc_reg_field_get(phy, CDMAC_PFC_CTRLr, pfc_ctrl,
                                               RX_PFC_ENf);

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PAUSE_CTRLr(phy, &pause_ctrl));
    drain_cells->rx_pause = aperta_soc_reg_field_get(phy, CDMAC_PAUSE_CTRLr,
                                                  pause_ctrl, RX_PAUSE_ENf);
    drain_cells->tx_pause = aperta_soc_reg_field_get(phy, CDMAC_PAUSE_CTRLr,
                                                  pause_ctrl, TX_PAUSE_ENf);

    return (PHYMOD_E_NONE);
}


int plp_aperta_cdmac_discard_set(const plp_aperta_phymod_phy_access_t *phy, int discard)
{
    BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_t tx_ctrl;

    PHYMOD_MEMSET(&tx_ctrl, 0 , sizeof(BCMI_APERTA_CDMAC_CDMAC_TX_CTRLr_t));
    /* Clear Discard fields */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_TX_CTRLr(phy, &tx_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_TX_CTRLr, tx_ctrl, DISCARDf, discard);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_TX_CTRLr(phy, tx_ctrl));

    return (PHYMOD_E_NONE);
}


int plp_aperta_cdmac_drain_cell_stop(const plp_aperta_phymod_phy_access_t *phy,
                           const portmod_drain_cells_t *drain_cells)
{
    BCMI_APERTA_CDMAC_CDMAC_PAUSE_CTRLr_t pause_ctrl;
    BCMI_APERTA_CDMAC_CDMAC_PFC_CTRLr_t pfc_ctrl;

    PHYMOD_MEMSET(&pause_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PAUSE_CTRLr_t));
    PHYMOD_MEMSET(&pfc_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PFC_CTRLr_t));
    /* Clear Discard fields */
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_discard_set(phy, 0));

    /* set pause fields */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PAUSE_CTRLr(phy, &pause_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_PAUSE_CTRLr, pause_ctrl, RX_PAUSE_ENf,
                      drain_cells->rx_pause);
    aperta_soc_reg_field_set(phy, CDMAC_PAUSE_CTRLr, pause_ctrl, TX_PAUSE_ENf,
                      drain_cells->tx_pause);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_PAUSE_CTRLr(phy, pause_ctrl));

    /* set pfc rx_en fields */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PFC_CTRLr(phy, &pfc_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_PFC_CTRLr, pfc_ctrl, RX_PFC_ENf,
                      drain_cells->rx_pfc_en);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_PFC_CTRLr(phy, pfc_ctrl));

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_drain_cell_start(const plp_aperta_phymod_phy_access_t *phy)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;
    BCMI_APERTA_CDMAC_CDMAC_PAUSE_CTRLr_t pause_ctrl;
    BCMI_APERTA_CDMAC_CDMAC_PFC_CTRLr_t pfc_ctrl;

    PHYMOD_MEMSET(&cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));
    PHYMOD_MEMSET(&pause_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PAUSE_CTRLr_t));
    PHYMOD_MEMSET(&pfc_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_PFC_CTRLr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));

    /* Don't disable TX since it stops egress and hangs if CPU sends */
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, TX_ENf, 1);
    /* Disable RX */
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, RX_ENf, 0);

    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_CTRLr(phy, cdmac_ctrl));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PAUSE_CTRLr(phy, &pause_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_PAUSE_CTRLr, pause_ctrl, RX_PAUSE_ENf,0);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_PAUSE_CTRLr(phy, pause_ctrl));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_PFC_CTRLr(phy, &pfc_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_PFC_CTRLr, pfc_ctrl, RX_PFC_ENf, 0);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_PFC_CTRLr(phy, pfc_ctrl));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, SOFT_RESETf, 1);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_CTRLr(phy, cdmac_ctrl));

    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_discard_set(phy, 1));

    return (PHYMOD_E_NONE);
}


int plp_aperta_cdmac_txfifo_cell_cnt_get(const plp_aperta_phymod_phy_access_t *phy, uint32_t* val)
{
    BCMI_APERTA_CDMAC_CDMAC_TXFIFO_CELL_CNTr_t txfifo;

    PHYMOD_MEMSET(&txfifo, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_TXFIFO_CELL_CNTr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_TXFIFO_CELL_CNTr(phy, &txfifo));
    *val = aperta_soc_reg_field_get(phy, CDMAC_TXFIFO_CELL_CNTr, txfifo, CELL_CNTf);

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_egress_queue_drain_get(const plp_aperta_phymod_phy_access_t *phy, uint64_t *mac_ctrl,
                                 int *rx_en)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;

    PHYMOD_MEMSET(&cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));
    *mac_ctrl = cdmac_ctrl.v[0];

    *rx_en = aperta_soc_reg_field_get(phy, CDMAC_CTRLr, cdmac_ctrl, RX_ENf);

    return (PHYMOD_E_NONE);
}


int plp_aperta_cdmac_drain_cells_rx_enable(const plp_aperta_phymod_phy_access_t *phy, int rx_en)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl, orig_rval;
    uint32_t soft_reset = 0;

    PHYMOD_MEMSET(&cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));
    PHYMOD_MEMSET(&orig_rval, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));
    /* Enable both TX and RX, de-assert SOFT_RESET */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));
    orig_rval.v[0] = cdmac_ctrl.v[0];

    /* Don't disable TX since it stops egress and hangs if CPU sends */
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, TX_ENf, 1);
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, RX_ENf, rx_en ? 1: 0);

    if (orig_rval.v[0] == cdmac_ctrl.v[0]) {
        /*
         *  To avoid the unexpected early return to prevent this problem.
         *  1. Problem occurred for disabling process only.
         *  2. To comply original designing scenario, CDMAC_CTRLr.SOFT_RESETf
         *     is used to early check to see if this port is at disabled state
         *     already.
         */
        soft_reset = aperta_soc_reg_field_get(phy, CDMAC_CTRLr, cdmac_ctrl, SOFT_RESETf);
        if ((rx_en) || (!rx_en && soft_reset)){
            return PHYMOD_E_NONE;
        }
    }

    if (rx_en) {
        aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, SOFT_RESETf, 0);
    }

    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_CTRLr(phy, cdmac_ctrl));

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_egress_queue_drain_rx_en(const plp_aperta_phymod_phy_access_t *phy, int rx_en)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;
    PHYMOD_MEMSET(&cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));

    /* Enable RX, de-assert SOFT_RESET */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, RX_ENf, rx_en? 1: 0);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_CTRLr(phy, cdmac_ctrl));
    /*Bring mac out of reset */
    PHYMOD_IF_ERR_RETURN(plp_aperta_cdmac_soft_reset_set(phy, 0));

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_reset_check(const plp_aperta_phymod_phy_access_t *phy, int enable, int *reset)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl, orig_cdmac_ctrl;

    *reset = 1;
    PHYMOD_MEMSET(&cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));
    PHYMOD_MEMSET(&orig_cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));

    /* Enable both TX and RX, de-assert SOFT_RESET */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));
    orig_cdmac_ctrl.v[0] = cdmac_ctrl.v[0];

    /* Don't disable TX since it stops egress and hangs if CPU sends */
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, TX_ENf, 1);
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, RX_ENf, enable? 1: 0);

    if (cdmac_ctrl.v[0] == orig_cdmac_ctrl.v[0]) {
        if (enable) {
            *reset = 0;
        } else {
            if (aperta_soc_reg_field_get(phy, CDMAC_CTRLr, cdmac_ctrl, SOFT_RESETf)) {
                *reset = 0;
            }
        }
    }

    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_trunk_hwfailover_config_set is the caller of this
 * function. This function is invoked during lag failover config
 * on a port.
 */
int plp_aperta_cdmac_sw_link_status_select_set(const plp_aperta_phymod_phy_access_t *phy, int enable)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;

    PHYMOD_MEMSET(&cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));
    /*
     * This configuration chooses between link status indication from software
     * (SW_LINK_STATUSf) or the hardware link status indication from the PCS i
     * to MAC. If set, hardware link status is used.
     */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, LINK_STATUS_SELECTf, enable);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_CTRLr(phy, cdmac_ctrl));

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_sw_link_status_select_get(const plp_aperta_phymod_phy_access_t *phy, int *enable)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;

    PHYMOD_MEMSET(&cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));
    *enable = aperta_soc_reg_field_get(phy, CDMAC_CTRLr, cdmac_ctrl, LINK_STATUS_SELECTf);

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_sw_link_status_set (const plp_aperta_phymod_phy_access_t *phy, int link)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;

    PHYMOD_MEMSET(&cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));

    /*
     * This is valid only if LINK_STATUS_SELECT is 0, which means the
     * software drives the link status. If SW_LINK_STATUS is set,
     * it indicates that the link is active. The use case is if there is
     * some other mechanism used for the link status determination other
     * than hardware.
     */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, SW_LINK_STATUSf, link);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_CTRLr(phy, cdmac_ctrl));


    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_sw_link_status_get (const plp_aperta_phymod_phy_access_t *phy, int *link)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;
    int link_status_sel = 0;

    PHYMOD_MEMSET(&cdmac_ctrl, 0, sizeof(BCMI_APERTA_CDMAC_CDMAC_CTRLr_t));

    /*
     * check if link status selection is software based, if yes return
     * sw_link_status.
     */
    PHYMOD_IF_ERR_RETURN(
        plp_aperta_cdmac_sw_link_status_select_get(phy, &link_status_sel));

    /* link status selection is software based */
    if (!link_status_sel) {
        PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));
        *link = aperta_soc_reg_field_get(phy, CDMAC_CTRLr, cdmac_ctrl, SW_LINK_STATUSf);
    }
    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_mib_counter_control_set(const plp_aperta_phymod_phy_access_t *phy,
                                  int enable, int clear)
{
    BCMI_APERTA_CDMAC_CDMAC_MIB_COUNTER_CTRLr_t mib_ctrl;

    /*
     * For enabling the MIB statistics counters for a the ENABLE field
     * must be set.
     * To reset/clear MIB statistics counters, CLEAR field should be set 1.
     * A low-to-high(0->1) transition on this bit(CLEAR field) will trigger
     * the counter-clear operation.
     * Please Note:
     * If a subsequent counter-clear operation is required, this bit has to
     * be first written to 0 and then written to 1. Instead of doing this
     * setting CLEAR field to 0 after setting to 1.
     * SW name for CLEAR field is CNT_CLEARf.
     */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_MIB_COUNTER_CTRLr(phy, &mib_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_MIB_COUNTER_CTRLr, mib_ctrl,
                      ENABLEf, enable);
    aperta_soc_reg_field_set(phy, CDMAC_MIB_COUNTER_CTRLr, mib_ctrl, CNT_CLEARf, clear);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_MIB_COUNTER_CTRLr(phy, mib_ctrl));

    /*
     * set CLEARf to 0, this operation is not mandatory, adding to
     * remove ambiguity in interpretation if a read on this register
     */
    if (clear) {
        PHYMOD_IF_ERR_RETURN(READ_CDMAC_MIB_COUNTER_CTRLr(phy, &mib_ctrl));
        aperta_soc_reg_field_set(phy, CDMAC_MIB_COUNTER_CTRLr, mib_ctrl, CNT_CLEARf, 0);
        PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_MIB_COUNTER_CTRLr(phy, mib_ctrl));
    }

    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_cntmaxsize_set is caller of this function
 */
int plp_aperta_cdmac_cntmaxsize_set(const plp_aperta_phymod_phy_access_t *phy, int val)
{
    BCMI_APERTA_CDMAC_CDMAC_MIB_COUNTER_CTRLr_t mib_ctrl;
    /*
     * The maximum packet size that is used in statistics counter updates.
     * default size is 1518. Note if RX_MAX_SIZE(max frame size received)
     * is greater than CNTMAXSIZEf, a good packet that is valid CRC and
     * contains no other errors, will increment the OVR(oversize) counters
     * if the length of the packet > CNTMAXSIZE < RX_MAX_SIZE values.
     * Having CNTMAXSIZE > RX_MAX_SIZE is not recommended.
     * This is generally taken care in the statistics module while
     * accumulating the counts.
     */
    PHYMOD_IF_ERR_RETURN(READ_CDMAC_MIB_COUNTER_CTRLr(phy, &mib_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_MIB_COUNTER_CTRLr, mib_ctrl, CNTMAXSIZEf, val);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_MIB_COUNTER_CTRLr(phy, mib_ctrl));

    return (PHYMOD_E_NONE);
}

/*
 * plp_aperta_portmod_port_cntmaxsize_get is caller of this function
 */
int plp_aperta_cdmac_cntmaxsize_get(const plp_aperta_phymod_phy_access_t *phy, int *val)
{
    BCMI_APERTA_CDMAC_CDMAC_MIB_COUNTER_CTRLr_t mib_ctrl;

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_MIB_COUNTER_CTRLr(phy, &mib_ctrl));
    *val = aperta_soc_reg_field_get(phy, CDMAC_MIB_COUNTER_CTRLr, mib_ctrl, CNTMAXSIZEf);

    return (PHYMOD_E_NONE);
}

/*
 * This function controls which RSV(Receive statistics vector) event
 * causes a purge event that triggers RXERR to be set for the packet
 * sent by the MAC to the IP. These bits are used to mask RSV[34:16]
 * for CDMAC; bit[18] of MASK maps to bit[34] of RSV, bit[0] of MASK
 * maps to bit[16] of RSV.
 * Enable : Set 0. Go through
 * Disable: Set 1. Purged.
 * bit[18] --> PFC frame detected
 * bit[17] --> Reserved
 * bit[16] --> Reserved
 * bit[15] --> Unicast detected
 * bit[14] --> VLAN tag detected
 * bit[13] --> Unsupported opcode detected
 * bit[12] --> Pause frame received
 * bit[11] --> Control frame received
 * bit[10] --> Promiscuous packet detected
 * bit[ 9] --> Broadcast detected
 * bit[ 8] --> Multicast detected
 * bit[ 7] --> Receive OK
 * bit[ 6] --> Truncated/Frame out of Range
 * bit[ 5] --> Frame length not out of range, but incorrect -
 *             IEEE length check failed
 * bit[ 4] --> CRC error
 * bit[ 3] --> Receive terminate/code error
 * bit[ 2] --> Unsupported DA for pause/PFC packets detected
 * bit[ 1] --> Stack VLAN detected
 * bit[ 0] --> Wrong SA
 */
int plp_aperta_cdmac_rsv_mask_control_set(const plp_aperta_phymod_phy_access_t *phy, uint32_t flags, uint32_t value)
{

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_rsv_mask_control_get(const plp_aperta_phymod_phy_access_t *phy, uint32_t flags, uint32_t *value)
{
    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_rsv_mask_set(const plp_aperta_phymod_phy_access_t *phy, uint32_t p_rsv_mask)
{
    BCMI_APERTA_CDMAC_CDMAC_RSV_MASKr_t rsv_mask;

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RSV_MASKr(phy, &rsv_mask));
    aperta_soc_reg_field_set(phy, CDMAC_RSV_MASKr, rsv_mask, MASKf, p_rsv_mask);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_RSV_MASKr(phy, rsv_mask));

   return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_rsv_mask_get(const plp_aperta_phymod_phy_access_t *phy, uint32_t *p_rsv_mask)
{
    BCMI_APERTA_CDMAC_CDMAC_RSV_MASKr_t rsv_mask;


    PHYMOD_IF_ERR_RETURN(READ_CDMAC_RSV_MASKr(phy, &rsv_mask));
    *p_rsv_mask = aperta_soc_reg_field_get(phy, CDMAC_RSV_MASKr, rsv_mask, MASKf);


   return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_link_down_sequence_enable_set(const plp_aperta_phymod_phy_access_t *phy, uint32_t value)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));
    aperta_soc_reg_field_set(phy, CDMAC_CTRLr, cdmac_ctrl, MAC_LINK_DOWN_SEQ_ENf,
                      (value)? 1: 0);
    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_CTRLr(phy, cdmac_ctrl));


    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_link_down_sequence_enable_get(const plp_aperta_phymod_phy_access_t *phy, uint32_t *value)
{
    BCMI_APERTA_CDMAC_CDMAC_CTRLr_t cdmac_ctrl;

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_CTRLr(phy, &cdmac_ctrl));
    *value = aperta_soc_reg_field_get(phy, CDMAC_CTRLr, cdmac_ctrl,
                               MAC_LINK_DOWN_SEQ_ENf);

    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_interrupt_enable_get(const plp_aperta_phymod_phy_access_t *phy, int intr_type, uint32_t *value)
{
    BCMI_APERTA_CDMAC_CDMAC_INTR_ENABLEr_t intr_enable;

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_INTR_ENABLEr(phy, &intr_enable));

    switch(intr_type) {
        case portmodIntrTypeTxPktUnderflow :
            *value = aperta_soc_reg_field_get(phy, CDMAC_INTR_ENABLEr, intr_enable,
                                       TX_PKT_UNDERFLOWf);
            break;
        case portmodIntrTypeTxPktOverflow :
            *value = aperta_soc_reg_field_get(phy, CDMAC_INTR_ENABLEr, intr_enable,
                                       TX_PKT_OVERFLOWf);
            break;
        case portmodIntrTypeTxCdcSingleBitErr :
            *value = aperta_soc_reg_field_get(phy, CDMAC_INTR_ENABLEr, intr_enable,
                                       TX_CDC_SINGLE_BIT_ERRf);
            break;
        case portmodIntrTypeTxCdcDoubleBitErr :
            *value = aperta_soc_reg_field_get(phy, CDMAC_INTR_ENABLEr, intr_enable,
                                       TX_CDC_DOUBLE_BIT_ERRf);
            break;
        case portmodIntrTypeLocalFaultStatus :
            *value = aperta_soc_reg_field_get(phy, CDMAC_INTR_ENABLEr, intr_enable,
                                       LOCAL_FAULT_STATUSf);
            break;
        case portmodIntrTypeRemoteFaultStatus :
            *value = aperta_soc_reg_field_get(phy, CDMAC_INTR_ENABLEr, intr_enable,
                                       REMOTE_FAULT_STATUSf);
            break;
        case portmodIntrTypeLinkInterruptionStatus :
            *value = aperta_soc_reg_field_get(phy, CDMAC_INTR_ENABLEr, intr_enable,
                                       LINK_INTERRUPTION_STATUSf);
            break;
        case portmodIntrTypeMibMemSingleBitErr :
            *value = aperta_soc_reg_field_get(phy, CDMAC_INTR_ENABLEr, intr_enable,
                                       MIB_COUNTER_SINGLE_BIT_ERRf);
            break;
        case portmodIntrTypeMibMemDoubleBitErr :
            *value = aperta_soc_reg_field_get(phy, CDMAC_INTR_ENABLEr, intr_enable,
                                       MIB_COUNTER_DOUBLE_BIT_ERRf);
            break;
        case portmodIntrTypeMibMemMultipleBitErr :
            *value = aperta_soc_reg_field_get(phy, CDMAC_INTR_ENABLEr, intr_enable,
                                       MIB_COUNTER_MULTIPLE_ERRf);
            break;
        default:
                APERTA_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                                  (APERTA_SOC_MSG("Invalid interrupt type")));
                break;
     }


    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_interrupt_enable_set(const plp_aperta_phymod_phy_access_t *phy, int intr_type, uint32_t value)
{
    BCMI_APERTA_CDMAC_CDMAC_INTR_ENABLEr_t intr_enable;

    PHYMOD_IF_ERR_RETURN(READ_CDMAC_INTR_ENABLEr(phy, &intr_enable));

    switch(intr_type) {
        case portmodIntrTypeTxPktUnderflow :
            aperta_soc_reg_field_set(phy, CDMAC_INTR_ENABLEr, intr_enable,
                              TX_PKT_UNDERFLOWf, value);
            break;
        case portmodIntrTypeTxPktOverflow :
            aperta_soc_reg_field_set(phy, CDMAC_INTR_ENABLEr, intr_enable,
                              TX_PKT_OVERFLOWf, value);
            break;
        case portmodIntrTypeTxCdcSingleBitErr :
            aperta_soc_reg_field_set(phy, CDMAC_INTR_ENABLEr, intr_enable,
                              TX_CDC_SINGLE_BIT_ERRf, value);
            break;
        case portmodIntrTypeTxCdcDoubleBitErr :
            aperta_soc_reg_field_set(phy, CDMAC_INTR_ENABLEr, intr_enable,
                              TX_CDC_DOUBLE_BIT_ERRf, value);
            break;
        case portmodIntrTypeLocalFaultStatus :
            aperta_soc_reg_field_set(phy, CDMAC_INTR_ENABLEr, intr_enable,
                              LOCAL_FAULT_STATUSf, value);
            break;
        case portmodIntrTypeRemoteFaultStatus :
            aperta_soc_reg_field_set(phy, CDMAC_INTR_ENABLEr, intr_enable,
                              REMOTE_FAULT_STATUSf, value);
            break;
        case portmodIntrTypeLinkInterruptionStatus :
            aperta_soc_reg_field_set(phy, CDMAC_INTR_ENABLEr, intr_enable,
                              LINK_INTERRUPTION_STATUSf, value);
            break;
        case portmodIntrTypeMibMemSingleBitErr :
            aperta_soc_reg_field_set(phy, CDMAC_INTR_ENABLEr, intr_enable,
                              MIB_COUNTER_SINGLE_BIT_ERRf, value);
            break;
        case portmodIntrTypeMibMemDoubleBitErr :
            aperta_soc_reg_field_set(phy, CDMAC_INTR_ENABLEr, intr_enable,
                              MIB_COUNTER_DOUBLE_BIT_ERRf, value);
            break;
        case portmodIntrTypeMibMemMultipleBitErr :
            aperta_soc_reg_field_set(phy, CDMAC_INTR_ENABLEr, intr_enable,
                              MIB_COUNTER_MULTIPLE_ERRf, value);
            break;
        default:
                APERTA_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                                  (APERTA_SOC_MSG("Invalid interrupt type")));
                break;
     }

    PHYMOD_IF_ERR_RETURN(WRITE_CDMAC_INTR_ENABLEr(phy, intr_enable));


    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_interrupt_status_get(const plp_aperta_phymod_phy_access_t *phy, int intr_type, uint32_t *value)
{
    /* Not needed, Aperta use top level interrupt*/
    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_interrupts_status_get(const plp_aperta_phymod_phy_access_t *phy, int arr_max_size,
                                uint32_t* intr_arr, uint32_t* size)
{
    /* Not needed, Aperta use top level interrupt*/
    return (PHYMOD_E_NONE);
}

int plp_aperta_cdmac_reg64_read(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t *data)
{
    unsigned long long temp;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta_reg64_read(phy, reg_addr, &temp));
    *data = COMPILER_64_LO(temp);
    data++;
    *data = COMPILER_64_HI(temp);
    return 0;
}

int plp_aperta_cdmac_reg64_write(const plp_aperta_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t *data)
{
    uint64_t reg_data;

    COMPILER_64_SET(reg_data, data[1], data[0]);
    return plp_aperta_reg64_write(phy, reg_addr, reg_data);
}

#endif
