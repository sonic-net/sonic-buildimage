/*
 *         
 * $Id:$
 * 
 * $Copyright: (c) 2016 Broadcom Corp.
 * All Rights Reserved.$
 *
 * File:    dcccport.c
 * Purpose: 
 *         
 *         
 *
 */

/*! \file dcccport.c
 *
 * DC3MAC driver.
 *
 * A DC3PORT contains 1 DC3PORT and 1 DC3MACs, and supports 8 ports at most.
 *
 * In this driver, we always use the port number 0~7 to access the DC3PORT
 * and DC3MAC per-port registers
 *
 */

#include <include/portmod.h>
#include <include/dcccport.h>
#include <phymod/phymod_util.h>
#include <include/bcm_aperta2_dc3mac_defs.h>

#if defined(PHYMOD_APERTA2_SUPPORT)

/*******************************************************************************
 * Local definitions
 ******************************************************************************/

/*! Drain cell waiting time. */
#define DRAIN_WAIT_MSEC 500

/*! Number of ports per DC3MAC. */
#define PORTS_PER_DC3MAC 8

/*! Minimum RUNT threshold. */
#define DC3MAC_RUNT_THRESHOLD_MIN      64

/*! Maximum RUNT threshold. */
#define DC3MAC_RUNT_THRESHOLD_MAX      96

/*! Minimun Average IPG */
#define DC3MAC_AVE_IPG_MIN             8

/*! Maximum Average IPG */
#define DC3MAC_AVE_IPG_MAX             60

/* Jumbo Packet Size*/
#define DC3MAC_JUMBO_PACKET_SIZE       0x3FFF  /*16K Packet Size*/

/*! DC3MAC header mode value per encap mode. */
typedef enum aperta2_dc3mac_hdr_mode_e {

    /*! IEEE Ethernet format (K.SOP + 6Byte preamble + SFD). */
    DC3MAC_HDR_MODE_IEEE = 0,

    /*! /S/ (K.SOP) character in header. */
    DC3MAC_HDR_MODE_KSOP = 5,

    /*! Reduced Preamble mode (K.SOP + 2Byte preamble + SFD). */
    DC3MAC_HDR_MODE_REDUCED_PREAMBLE = 6,

} aperta2_dc3mac_hdr_mode_t;

/*! DC3PORT port mode value. */
typedef enum aperta2_dc3port_port_mode_e {

    /*! Quad Port Mode. All four ports are enabled. */
    DC3PORT_PORT_MODE_QUAD = 0,

    /*! Tri Port Mode. Lanes 0, 1, and 2 active. lane 2 is dual. */
    DC3PORT_PORT_MODE_TRI_012 = 1,

    /*! Tri Port Mode. Lanes 0, 2, and 3 active. lane 0 is dual. */
    DC3PORT_PORT_MODE_TRI_023 = 2,

    /*! Dual Port Mode. Each of lanes 0 and 2 are dual. */
    DC3PORT_PORT_MODE_DUAL = 3,

    /*! Single Port Mode. Lanes 0 through 3 are single XLGMII. */
    DC3PORT_PORT_MODE_SINGLE = 4,

    /*! Single 8 lane Port Mode. Lanes 0 through 7 are single XLGMII. */
    DC3PORT_PORT_MODE_SINGLE_8_LANE = 8

} aperta2_dc3port_port_mode_t;

/*! DC3MAC CRC MODE. */
typedef enum aperta2_dc3mac_crc_mode_e {

    /*! CRC is computed on incoming packet data and appended. */
    DC3MAC_CRC_MODE_APPEND = 0,

    /*! Incoming pkt CRC is passed through without modifications. */
    DC3MAC_CRC_MODE_KEEP = 1,

    /*! Incoming pkt CRC is replaced with CRC value computed by the MAC. */
    DC3MAC_CRC_MODE_REPLACE = 2,

    /*! The CRC mode is determined by the HW. */
    DC3MAC_CRC_MODE_AUTO = 3,

} aperta2_dc3mac_crc_mode_t;


#define aperta2_soc_reg_field_get(PHY, REGNAME, REG_INS, FIELD)                  BCM_APERTA2_DC3MAC_##REGNAME##_##FIELD##_GET(REG_INS)
#define aperta2_soc_reg_field_set(PHY, REGNAME, REG_INS, FIELD, VALUE)   BCM_APERTA2_DC3MAC_##REGNAME##_##FIELD##_SET(REG_INS, VALUE)

/******************************************************************************
 * Private functions
 ******************************************************************************/

int
plp_aperta2_dc3port_port_reset_set(const plp_aperta2_phymod_phy_access_t *phy, uint32_t reset)
{
    /* There is no per-port RESET control in DC3PORT. */
    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3port_port_reset_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t *reset)
{
    /* There is no per-port RESET control in DC3PORT. */
    *reset = 0;
    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3port_port_enable_set(const plp_aperta2_phymod_phy_access_t *phy, uint32_t enable)
{
    /* There is no per-port ENABLE control in DC3PORT. */
    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3port_port_enable_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t *enable)
{
    /* There is no per-port ENABLE control in DC3PORT. */
    *enable = 1;
    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_reset_set(const plp_aperta2_phymod_phy_access_t *phy, uint32_t reset)
{
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t rval;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(phy, &rval));
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, rval, SOFT_RESETf, reset);

    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_CTRLr(phy, rval));


    return (PHYMOD_E_NONE);
}

int
plp_aperta2_dc3mac_reset_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t *reset)
{
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t rval;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(phy, &rval));
    *reset = aperta2_soc_reg_field_get(phy, DC3MAC_CTRLr, rval, SOFT_RESETf);

    return (PHYMOD_E_NONE);
}

int
plp_aperta2_dc3mac_rx_enable_set(const plp_aperta2_phymod_phy_access_t *phy, uint32_t enable)
{
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t rval;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(phy, &rval));
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, rval, RX_ENf, enable);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_CTRLr(phy, rval));

    return (PHYMOD_E_NONE);
}

int
plp_aperta2_dc3mac_rx_enable_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t *enable)
{
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t rval;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(phy, &rval));

    *enable = aperta2_soc_reg_field_get(phy, DC3MAC_CTRLr, rval, RX_ENf);

    return (PHYMOD_E_NONE);
}

int
plp_aperta2_dc3mac_tx_enable_set(const plp_aperta2_phymod_phy_access_t *phy, uint32_t enable)
{
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t rval;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(phy, &rval));
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, rval, TX_ENf, enable);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_CTRLr(phy, rval));

    return (PHYMOD_E_NONE);
}

int
plp_aperta2_dc3mac_tx_enable_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t *enable)
{
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t rval;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(phy, &rval));

    *enable = aperta2_soc_reg_field_get(phy, DC3MAC_CTRLr, rval, TX_ENf);

    return (PHYMOD_E_NONE);
}

int
plp_aperta2_dc3mac_encap_set(const plp_aperta2_phymod_phy_access_t *phy, portmod_encap_t encap)
{
    BCM_APERTA2_DC3MAC_DC3MAC_MODEr_t aperta2_dc3mac_mode;
    uint32_t hdr_mode = DC3MAC_HDR_MODE_IEEE;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_MODEr(phy, &aperta2_dc3mac_mode));
    aperta2_soc_reg_field_set(phy, DC3MAC_MODEr, aperta2_dc3mac_mode, HDR_MODEf, hdr_mode);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_MODEr(phy, aperta2_dc3mac_mode));


    return PHYMOD_E_NONE;

}

int
plp_aperta2_dc3mac_encap_get(const plp_aperta2_phymod_phy_access_t *phy, portmod_encap_t *encap)
{
    *encap = DC3MAC_HDR_MODE_IEEE;

    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3port_lpbk_set(const plp_aperta2_phymod_phy_access_t *phy, uint32_t en)
{
    /* FIXME later */
    return PHYMOD_E_UNAVAIL;
}

int
plp_aperta2_dc3port_lpbk_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t *en)
{
    /* FIXME later */
    return PHYMOD_E_UNAVAIL;
}

int
plp_aperta2_dc3mac_pause_set(const plp_aperta2_phymod_phy_access_t *phy,
                 const portmod_pause_control_t *ctrl)
{
    BCM_APERTA2_DC3MAC_DC3MAC_PAUSE_CTRLr_t pause_ctrl;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PAUSE_CTRLr(phy, &pause_ctrl));

    if(ctrl->rx_enable || ctrl->tx_enable) {
       if(ctrl->refresh_timer > 0 ) {
          aperta2_soc_reg_field_set(phy, DC3MAC_PAUSE_CTRLr, pause_ctrl,
                            PAUSE_REFRESH_TIMERf, ctrl->refresh_timer);
          aperta2_soc_reg_field_set(phy, DC3MAC_PAUSE_CTRLr, pause_ctrl,
                            PAUSE_REFRESH_ENf, 1);
       } else {
          aperta2_soc_reg_field_set(phy, DC3MAC_PAUSE_CTRLr, pause_ctrl,
                            PAUSE_REFRESH_ENf, 0);
       }
       pause_ctrl.v[0] = (ctrl->xoff_timer & 0xFFF) << 20;
       pause_ctrl.v[1] = (ctrl->xoff_timer & 0xF000) >> 12 ;
    }

    aperta2_soc_reg_field_set(phy, DC3MAC_PAUSE_CTRLr, pause_ctrl, TX_PAUSE_ENf,
                      ctrl->tx_enable);
    aperta2_soc_reg_field_set(phy, DC3MAC_PAUSE_CTRLr, pause_ctrl, RX_PAUSE_ENf,
                      ctrl->rx_enable);

    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PAUSE_CTRLr(phy, pause_ctrl));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_pause_get(const plp_aperta2_phymod_phy_access_t *phy,
                 portmod_pause_control_t *ctrl)
{
    BCM_APERTA2_DC3MAC_DC3MAC_PAUSE_CTRLr_t pause_ctrl;
    uint32_t refresh_enable;
    uint32_t refresh_timer;

    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PAUSE_CTRLr(phy, &pause_ctrl));

    ctrl->tx_enable = aperta2_soc_reg_field_get(phy, DC3MAC_PAUSE_CTRLr, pause_ctrl,
                                           TX_PAUSE_ENf);
    ctrl->rx_enable = aperta2_soc_reg_field_get(phy, DC3MAC_PAUSE_CTRLr, pause_ctrl,
                                           RX_PAUSE_ENf);
    refresh_enable = aperta2_soc_reg_field_get(phy, DC3MAC_PAUSE_CTRLr, pause_ctrl,
                                       PAUSE_REFRESH_ENf);
    refresh_timer = aperta2_soc_reg_field_get(phy, DC3MAC_PAUSE_CTRLr, pause_ctrl,
                                      PAUSE_REFRESH_TIMERf);

    ctrl->refresh_timer = (refresh_enable? refresh_timer: -1);
    ctrl->xoff_timer = (((pause_ctrl.v[0] >> 20) & 0xFFF) | ((pause_ctrl.v[1] & 0xF) << 12)) ;


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_tx_mac_sa_set(const plp_aperta2_phymod_phy_access_t *phy,
                      uint8_t mac[6])
{
    BCM_APERTA2_DC3MAC_DC3MAC_TX_MAC_SAr_t mac_sa;
    uint32_t mac_addr=0;

    BCM_APERTA2_DC3MAC_DC3MAC_TX_MAC_SAr_CLR(mac_sa);

    /* high-order 8-bits of field value corresponds to mac[0],
       low-order 8-bits of field value corresponds to mac[5] */
    mac_addr = (mac[0] << 8) | (mac[1]) ;
    BCM_APERTA2_DC3MAC_DC3MAC_TX_MAC_SAr_SET(mac_sa,1,mac_addr);

    mac_addr = (mac[2] << 24) | (mac[3] << 16)| (mac[4] << 8) | (mac[5]) ;
    BCM_APERTA2_DC3MAC_DC3MAC_TX_MAC_SAr_SET(mac_sa,0,mac_addr);

    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_TX_MAC_SAr(phy, mac_sa));

    return (PHYMOD_E_NONE);

}
int plp_aperta2_dc3mac_tx_mac_sa_get(const plp_aperta2_phymod_phy_access_t *phy, uint8_t mac[6])
{
    BCM_APERTA2_DC3MAC_DC3MAC_TX_MAC_SAr_t mac_sa;
    uint32_t u_mac = 0;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_TX_MAC_SAr(phy, mac_sa));
    u_mac = BCM_APERTA2_DC3MAC_DC3MAC_TX_MAC_SAr_GET(mac_sa,1);
    mac[0] = (u_mac & 0xFF00) >> 8;
    mac[1] = (u_mac & 0xFF);

    u_mac = BCM_APERTA2_DC3MAC_DC3MAC_TX_MAC_SAr_GET(mac_sa,0);
    mac[2] = (u_mac & 0xFF000000) >> 24;
    mac[3] = (u_mac & 0x00FF0000) >> 16;
    mac[4] = (u_mac & 0x0000FF00) >> 8;
    mac[5] = (u_mac & 0x000000FF);

    return (PHYMOD_E_NONE);
}


int
plp_aperta2_dc3mac_rx_mac_sa_set(const plp_aperta2_phymod_phy_access_t *phy,
                      uint8_t mac[6])
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_MAC_SAr_t mac_sa;
    uint32_t mac_addr=0;

    BCM_APERTA2_DC3MAC_DC3MAC_RX_MAC_SAr_CLR(mac_sa);

    /* high-order 8-bits of field value corresponds to mac[0],
       low-order 8-bits of field value corresponds to mac[5] */
    mac_addr = (mac[0] << 8) | (mac[1]) ;
    BCM_APERTA2_DC3MAC_DC3MAC_RX_MAC_SAr_SET(mac_sa,1,mac_addr);

    mac_addr = (mac[2] << 24) | (mac[3] << 16)| (mac[4] << 8) | (mac[5]) ;
    BCM_APERTA2_DC3MAC_DC3MAC_RX_MAC_SAr_SET(mac_sa,0,mac_addr);

    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_RX_MAC_SAr(phy, mac_sa));

    return (PHYMOD_E_NONE);

}

int plp_aperta2_dc3mac_rx_mac_sa_get(const plp_aperta2_phymod_phy_access_t *phy, uint8_t mac[6])
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_MAC_SAr_t mac_sa;
    uint32_t u_mac = 0;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_MAC_SAr(phy, mac_sa));
    u_mac = BCM_APERTA2_DC3MAC_DC3MAC_RX_MAC_SAr_GET(mac_sa,1);
    mac[0] = (u_mac & 0xFF00) >> 8;
    mac[1] = (u_mac & 0xFF);

    u_mac = BCM_APERTA2_DC3MAC_DC3MAC_RX_MAC_SAr_GET(mac_sa,0);
    mac[2] = (u_mac & 0xFF000000) >> 24;
    mac[3] = (u_mac & 0x00FF0000) >> 16;
    mac[4] = (u_mac & 0x0000FF00) >> 8;
    mac[5] = (u_mac & 0x000000FF);
    (void)mac_sa;
    return (PHYMOD_E_NONE);
}


int
plp_aperta2_dc3mac_frame_max_set(const plp_aperta2_phymod_phy_access_t *phy,
                     uint32_t size)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_MAX_SIZEr_t rx_max_size; 

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_MAX_SIZEr(phy, &rx_max_size));
    aperta2_soc_reg_field_set(phy, DC3MAC_RX_MAX_SIZEr, rx_max_size, RX_MAX_SIZEf, size);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_RX_MAX_SIZEr(phy, rx_max_size));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_frame_max_get(const plp_aperta2_phymod_phy_access_t *phy,
                     uint32_t *size)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_MAX_SIZEr_t rx_max_size; 

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_MAX_SIZEr(phy, &rx_max_size));
     *size = aperta2_soc_reg_field_get(phy, DC3MAC_RX_MAX_SIZEr, rx_max_size, RX_MAX_SIZEf);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_remote_fault_status_get(const plp_aperta2_phymod_phy_access_t *phy,
                               int *status)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_CTRLr_t lss_ctrl;
    BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_STATUSr_t lss_st;
    /*
     *  The fault status bit in DC3MAC_RX_LSS_STATUSr is cleared-on-read.
     *  Read it twice to make sure the fault status is set.
     */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_LSS_STATUSr(phy, &lss_st));
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_LSS_CTRLr(phy, &lss_ctrl));
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_LSS_STATUSr(phy, &lss_st));

    /* The fault status is vaild when the fault control is enabled. */
    if (aperta2_soc_reg_field_get(phy, DC3MAC_RX_LSS_CTRLr, lss_ctrl,
                          REMOTE_FAULT_DISABLEf) == 0) {
        *status =
            aperta2_soc_reg_field_get(phy, DC3MAC_RX_LSS_STATUSr, lss_st,
                              REMOTE_FAULT_STATUSf);
    }

    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_local_fault_status_get(const plp_aperta2_phymod_phy_access_t *phy,
                              int *status)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_CTRLr_t lss_ctrl;
    BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_STATUSr_t lss_st;

    

    /*
     *  The fault status bit in DC3MAC_RX_LSS_STATUSr is cleared-on-read.
     *  Read it twice to make sure the fault status is set.
     */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_LSS_STATUSr(phy, &lss_st));
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_LSS_CTRLr(phy, &lss_ctrl));
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_LSS_STATUSr(phy, &lss_st));

    /* The fault status is vaild when the fault control is enabled. */
    if (aperta2_soc_reg_field_get(phy, DC3MAC_RX_LSS_CTRLr, lss_ctrl,
                          LOCAL_FAULT_DISABLEf) == 0) {
        *status =
            aperta2_soc_reg_field_get(phy, DC3MAC_RX_LSS_STATUSr, lss_st,
                              LOCAL_FAULT_STATUSf);
    }


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_pfc_control_set(const plp_aperta2_phymod_phy_access_t *phy,
                       const portmod_pfc_control_t  *cfg)
{
    BCM_APERTA2_DC3MAC_DC3MAC_PAUSE_CTRLr_t rval64;
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_CTRLr_t rval;


   PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PAUSE_CTRLr(phy, &rval64));
    aperta2_soc_reg_field_set(phy, DC3MAC_PAUSE_CTRLr, rval64,
                          PFC_REFRESH_ENf, (cfg->refresh_timer > 0));
    aperta2_soc_reg_field_set(phy, DC3MAC_PAUSE_CTRLr, rval64,
                          PFC_REFRESH_TIMERf, cfg->refresh_timer);
    rval64.v[0] = (cfg->xoff_timer & 0xFFF) << 20;
    rval64.v[1] = (cfg->xoff_timer & 0xF000) >> 12 ;
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PAUSE_CTRLr(phy, rval64));

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PFC_CTRLr(phy, &rval));
    aperta2_soc_reg_field_set(phy, DC3MAC_PFC_CTRLr, rval, PFC_STATS_ENf,
                      cfg->stats_en);
    aperta2_soc_reg_field_set(phy, DC3MAC_PFC_CTRLr, rval, FORCE_PFC_XONf,
                      cfg->force_xon);
    aperta2_soc_reg_field_set(phy, DC3MAC_PFC_CTRLr, rval, TX_PFC_ENf,
                      cfg->tx_enable);
    aperta2_soc_reg_field_set(phy, DC3MAC_PFC_CTRLr, rval, RX_PFC_ENf,
                      cfg->rx_enable);

    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PFC_CTRLr(phy, rval));


    /*
     * If pfc is disabled on RX, toggle FORCE_XON.
     * This forces the MAC to generate an XON indication to
     * the MMU for all classes of service in the receive direction.
     * Do not check the previous pfx rx enable state.
     * This is not needed when pfc is enabled on RX.
     */
    if (!cfg->rx_enable) {
        PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PFC_CTRLr(phy, &rval));
        aperta2_soc_reg_field_set(phy, DC3MAC_PFC_CTRLr, rval, FORCE_PFC_XONf, 1);
        PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PFC_CTRLr(phy, rval));

        PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PFC_CTRLr(phy, &rval));
        aperta2_soc_reg_field_set(phy, DC3MAC_PFC_CTRLr, rval, FORCE_PFC_XONf, 0);
        PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PFC_CTRLr(phy, rval));
    }



    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_pfc_control_get(const plp_aperta2_phymod_phy_access_t *phy,
                       portmod_pfc_control_t  *cfg)
{
    BCM_APERTA2_DC3MAC_DC3MAC_PAUSE_CTRLr_t pause_ctrl;
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_CTRLr_t pfc_ctrl;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PAUSE_CTRLr(phy, &pause_ctrl));
    cfg->refresh_timer =
        aperta2_soc_reg_field_get(phy, DC3MAC_PAUSE_CTRLr, pause_ctrl, PFC_REFRESH_TIMERf);
    cfg->xoff_timer = ((pause_ctrl.v[0] & 0xfff00000) >> 20) | ((pause_ctrl.v[1] & 0xF) << 12);

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PFC_CTRLr(phy, &pfc_ctrl));
    cfg->force_xon =
        aperta2_soc_reg_field_get(phy, DC3MAC_PFC_CTRLr, pfc_ctrl, FORCE_PFC_XONf);
    cfg->stats_en =
        aperta2_soc_reg_field_get(phy, DC3MAC_PFC_CTRLr, pfc_ctrl, PFC_STATS_ENf);
    cfg->tx_enable =
        aperta2_soc_reg_field_get(phy, DC3MAC_PFC_CTRLr, pfc_ctrl, TX_PFC_ENf);
    cfg->rx_enable =
        aperta2_soc_reg_field_get(phy, DC3MAC_PFC_CTRLr, pfc_ctrl, RX_PFC_ENf);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_pfc_config_set(const plp_aperta2_phymod_phy_access_t *phy,
                      const portmod_pfc_config_t *cfg)
{
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_TYPE_OPCODEr_t rval;
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_DAr_t rval64;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PFC_TYPE_OPCODEr(phy, &rval));
    aperta2_soc_reg_field_set(phy, DC3MAC_PFC_TYPE_OPCODEr, rval, PFC_OPCODEf, cfg->opcode);
    aperta2_soc_reg_field_set(phy, DC3MAC_PFC_TYPE_OPCODEr, rval, PFC_ETH_TYPEf, cfg->type);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PFC_TYPE_OPCODEr(phy, rval));

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PFC_DAr(phy, &rval64));
    rval64.v[0] |= cfg->da_nonoui;
    rval64.v[0] |= (cfg->da_oui & 0xff) << 24;
    rval64.v[1] |= (cfg->da_oui & 0xffff00) >> 8;

    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PFC_DAr(phy, rval64));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_pfc_config_get(const plp_aperta2_phymod_phy_access_t *phy,
                      portmod_pfc_config_t *cfg)
{
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_TYPE_OPCODEr_t pfc_opcode;
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_DAr_t pfc_da;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PFC_TYPE_OPCODEr(phy, &pfc_opcode));
    cfg->type =
        aperta2_soc_reg_field_get(phy, DC3MAC_PFC_TYPE_OPCODEr, pfc_opcode, PFC_ETH_TYPEf);
    cfg->opcode =
        aperta2_soc_reg_field_get(phy, DC3MAC_PFC_TYPE_OPCODEr, pfc_opcode, PFC_OPCODEf);

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PFC_DAr(phy, &pfc_da));
    cfg->da_nonoui = pfc_da.v[0] & 0xffffff;
    cfg->da_oui = (pfc_da.v[1] << 8) | (pfc_da.v[0] >> 24);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_local_fault_disable_set(const plp_aperta2_phymod_phy_access_t *phy,
                               const portmod_local_fault_control_t *st)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_CTRLr_t rval;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_LSS_CTRLr(phy, &rval));

    aperta2_soc_reg_field_set(phy, DC3MAC_RX_LSS_CTRLr, rval, LOCAL_FAULT_DISABLEf,
                      st->enable? 0: 1); /* flip */
    aperta2_soc_reg_field_set(phy, DC3MAC_RX_LSS_CTRLr, rval,
                      DROP_TX_DATA_ON_LOCAL_FAULTf,
                      st->drop_tx_on_fault? 1: 0);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_RX_LSS_CTRLr(phy, rval));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_local_fault_disable_get(const plp_aperta2_phymod_phy_access_t *phy,
                               portmod_local_fault_control_t *st)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_CTRLr_t rval;
    uint32_t fval;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_LSS_CTRLr(phy, &rval));

    fval = aperta2_soc_reg_field_get(phy, DC3MAC_RX_LSS_CTRLr, rval,
                             LOCAL_FAULT_DISABLEf);
    st->enable = (fval? 0: 1);

    fval = aperta2_soc_reg_field_get(phy, DC3MAC_RX_LSS_CTRLr, rval,
                             DROP_TX_DATA_ON_LOCAL_FAULTf);
    st->drop_tx_on_fault = (fval? 1: 0);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_remote_fault_disable_set(const plp_aperta2_phymod_phy_access_t *phy,
                                const portmod_remote_fault_control_t *st)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_CTRLr_t rval;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_LSS_CTRLr(phy, &rval));

    aperta2_soc_reg_field_set(phy, DC3MAC_RX_LSS_CTRLr, rval, REMOTE_FAULT_DISABLEf,
                      st->enable? 0: 1); /* flip */
    aperta2_soc_reg_field_set(phy, DC3MAC_RX_LSS_CTRLr, rval,
                      DROP_TX_DATA_ON_REMOTE_FAULTf,
                      st->drop_tx_on_fault? 1: 0);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_RX_LSS_CTRLr(phy, rval));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_remote_fault_disable_get(const plp_aperta2_phymod_phy_access_t *phy,
                                portmod_remote_fault_control_t *st)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_CTRLr_t rval;
    uint32_t fval;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_LSS_CTRLr(phy, &rval));

    fval = aperta2_soc_reg_field_get(phy, DC3MAC_RX_LSS_CTRLr, rval,
                             REMOTE_FAULT_DISABLEf);
    /* if fval is reset, indicates enable */
    st->enable = (fval? 0: 1); 

    fval = aperta2_soc_reg_field_get(phy, DC3MAC_RX_LSS_CTRLr, rval,
                             DROP_TX_DATA_ON_REMOTE_FAULTf);
    st->drop_tx_on_fault = (fval? 1: 0);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_avg_ipg_set(const plp_aperta2_phymod_phy_access_t *phy,
                   uint8_t ipg_size)
{
    BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr_t rval;
    

    /*
     * Average inter packet gap can be in the range 8 to 56 or 60.
     * should be 56 for XLGMII, 60 for XGMII,
     * default is 12.
     */
    if ((ipg_size < DC3MAC_AVE_IPG_MIN) ||
        (ipg_size > DC3MAC_AVE_IPG_MAX)) {
       APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (_SOC_MSG("Average IPG is out of range.")));
    }

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_TX_CTRLr(phy, &rval));
    aperta2_soc_reg_field_set(phy, DC3MAC_TX_CTRLr, rval, AVERAGE_IPGf, ipg_size);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_TX_CTRLr(phy, rval));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_avg_ipg_get(const plp_aperta2_phymod_phy_access_t *phy,
                   uint8_t *ipg_size)
{
    BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr_t rval;
    if (ipg_size == NULL) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                          (_SOC_MSG("Invalid Input.")));
        return PHYMOD_E_PARAM;
    }
    *ipg_size = 0;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_TX_CTRLr(phy, &rval));
    *ipg_size = aperta2_soc_reg_field_get(phy, DC3MAC_TX_CTRLr, rval, AVERAGE_IPGf);


    return PHYMOD_E_NONE;
}

int plp_aperta2_dc3mac_interrupt_enable_get(const plp_aperta2_phymod_phy_access_t *phy, int intr_type, uint32_t *value)
{
    BCM_APERTA2_DC3MAC_DC3MAC_INTR_ENABLEr_t rval;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_INTR_ENABLEr(phy, &rval));

    switch(intr_type) {
        case portmodIntrTypeTxPktUnderflow :
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_INTR_ENABLEr, rval, 
                                       TX_PKT_UNDERFLOWf);
            break;
        case portmodIntrTypeTxPktOverflow :
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_INTR_ENABLEr, rval, 
                                       TX_PKT_OVERFLOWf);
            break;
        case portmodIntrTypeTxCdcSingleBitErr :
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_INTR_ENABLEr, rval,
                                       TX_CDC_SINGLE_BIT_ERRf);
            break;
        case portmodIntrTypeTxCdcDoubleBitErr :
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_INTR_ENABLEr, rval,
                                       TX_CDC_DOUBLE_BIT_ERRf);
            break;
        case portmodIntrTypeLocalFaultStatus :
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_INTR_ENABLEr, rval,
                                       LOCAL_FAULT_STATUSf);
            break;
        case portmodIntrTypeRemoteFaultStatus :
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_INTR_ENABLEr, rval,
                                       REMOTE_FAULT_STATUSf);
            break;
        case portmodIntrTypeMibMemSingleBitErr :
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_INTR_ENABLEr, rval,
                                       MIB_COUNTER_SINGLE_BIT_ERRf);
            break;
        case portmodIntrTypeMibMemDoubleBitErr :
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_INTR_ENABLEr, rval,
                                       MIB_COUNTER_DOUBLE_BIT_ERRf);
            break;
        case portmodIntrTypeMibMemMultipleBitErr :
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_INTR_ENABLEr, rval,
                                       MIB_COUNTER_MULTIPLE_ERRf);
            break;
        case portmodIntrTypeRxPfcFifoOverflow :
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_INTR_ENABLEr, rval,
                                       RX_PFC_FIFO_OVERFLOWf);
            break;
        case portmodIntrTypeTxPfcFifoOverflow :
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_INTR_ENABLEr, rval,
                                       TX_PFC_FIFO_OVERFLOWf);
            break;
        case portmodIntrTypeFdrInterrupt :
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_INTR_ENABLEr, rval,
                                       FDR_INTERRUPTf);
            break;
        default:
                APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                                  (_SOC_MSG("Invalid interrupt type")));
                break;
     }


    return PHYMOD_E_NONE;
}

int plp_aperta2_dc3mac_interrupt_enable_set(const plp_aperta2_phymod_phy_access_t *phy, int intr_type, uint32_t value)
{
    BCM_APERTA2_DC3MAC_DC3MAC_INTR_ENABLEr_t rval;
 

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_INTR_ENABLEr(phy, &rval));

    switch(intr_type) {
        case portmodIntrTypeTxPktUnderflow :
            aperta2_soc_reg_field_set(phy, DC3MAC_INTR_ENABLEr, rval, 
                              TX_PKT_UNDERFLOWf, value);
            break;
        case portmodIntrTypeTxPktOverflow :
            aperta2_soc_reg_field_set(phy, DC3MAC_INTR_ENABLEr, rval, 
                              TX_PKT_OVERFLOWf, value);
            break;
        case portmodIntrTypeTxCdcSingleBitErr :
            aperta2_soc_reg_field_set(phy, DC3MAC_INTR_ENABLEr, rval,
                              TX_CDC_SINGLE_BIT_ERRf, value);
            break;
        case portmodIntrTypeTxCdcDoubleBitErr :
            aperta2_soc_reg_field_set(phy, DC3MAC_INTR_ENABLEr, rval,
                              TX_CDC_DOUBLE_BIT_ERRf, value);
            break;
        case portmodIntrTypeLocalFaultStatus :
            aperta2_soc_reg_field_set(phy, DC3MAC_INTR_ENABLEr, rval,
                              LOCAL_FAULT_STATUSf, value);
            break;
        case portmodIntrTypeRemoteFaultStatus :
            aperta2_soc_reg_field_set(phy, DC3MAC_INTR_ENABLEr, rval,
                              REMOTE_FAULT_STATUSf, value);
            break;
        case portmodIntrTypeMibMemSingleBitErr :
            aperta2_soc_reg_field_set(phy, DC3MAC_INTR_ENABLEr, rval,
                              MIB_COUNTER_SINGLE_BIT_ERRf, value);
            break;
        case portmodIntrTypeMibMemDoubleBitErr :
            aperta2_soc_reg_field_set(phy, DC3MAC_INTR_ENABLEr, rval,
                              MIB_COUNTER_DOUBLE_BIT_ERRf, value);
            break;
        case portmodIntrTypeMibMemMultipleBitErr :
            aperta2_soc_reg_field_set(phy, DC3MAC_INTR_ENABLEr, rval,
                              MIB_COUNTER_MULTIPLE_ERRf, value);
            break;
        case portmodIntrTypeRxPfcFifoOverflow :
            aperta2_soc_reg_field_set(phy, DC3MAC_INTR_ENABLEr, rval,
                              RX_PFC_FIFO_OVERFLOWf, value);
            break;
        case portmodIntrTypeTxPfcFifoOverflow :
            aperta2_soc_reg_field_set(phy, DC3MAC_INTR_ENABLEr, rval,
                              TX_PFC_FIFO_OVERFLOWf, value);
            break;
        case portmodIntrTypeFdrInterrupt :
            aperta2_soc_reg_field_set(phy, DC3MAC_INTR_ENABLEr, rval,
                              FDR_INTERRUPTf, value);
            break;
        default:
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (_SOC_MSG("Invalid interrupt type")));
            break;
     }

    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_INTR_ENABLEr(phy, rval));


    return PHYMOD_E_NONE;
}

int plp_aperta2_dc3mac_interrupt_status_get(const plp_aperta2_phymod_phy_access_t *phy, int intr_type, uint32_t *value)
{
    BCM_APERTA2_DC3MAC_DC3MAC_FIFO_STATUSr_t rval;
    BCM_APERTA2_DC3MAC_DC3MAC_ECC_STATUSr_t ecc;
    BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_STATUSr_t rx_lss;
    

    switch(intr_type) {
        case portmodIntrTypeTxPktUnderflow :
            /* Clear on Read Operation */
            PHYMOD_IF_ERR_RETURN(READ_DC3MAC_FIFO_STATUSr(phy, &rval));
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_FIFO_STATUSr, rval,
                                       TX_PKT_UNDERFLOWf);
            break;
        case portmodIntrTypeTxPktOverflow :
            /* Clear on Read Operation */
            PHYMOD_IF_ERR_RETURN(READ_DC3MAC_FIFO_STATUSr(phy, &rval));
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_FIFO_STATUSr, rval,
                                       TX_PKT_OVERFLOWf);
            break;
        case portmodIntrTypeRxPfcFifoOverflow :
            /* Clear on Read Operation */
            PHYMOD_IF_ERR_RETURN(READ_DC3MAC_FIFO_STATUSr(phy, &rval));
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_FIFO_STATUSr, rval,
                                       RX_PFC_FIFO_OVERFLOWf);
            break;
        case portmodIntrTypeTxPfcFifoOverflow :
            /* Clear on Read Operation */
            PHYMOD_IF_ERR_RETURN(READ_DC3MAC_FIFO_STATUSr(phy, &rval));
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_FIFO_STATUSr, rval,
                                       TX_PFC_FIFO_OVERFLOWf);
            break;
        case portmodIntrTypeFdrInterrupt :
            /* Clear on Read Operation */
            PHYMOD_IF_ERR_RETURN(READ_DC3MAC_FIFO_STATUSr(phy, &rval));
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_FIFO_STATUSr, rval,
                                       FDR_INTERRUPTf);
            break;
        case portmodIntrTypeLocalFaultStatus :
            /* Clear on Read Operation */
            PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_LSS_STATUSr(phy, &rx_lss));
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_RX_LSS_STATUSr, rx_lss,
                                       LOCAL_FAULT_STATUSf);
            break;
        case portmodIntrTypeRemoteFaultStatus :
            /* Clear on Read Operation */
            PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_LSS_STATUSr(phy, &rx_lss));
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_RX_LSS_STATUSr, rx_lss,
                                       REMOTE_FAULT_STATUSf);
            break;
        case portmodIntrTypeTxCdcSingleBitErr :
            PHYMOD_IF_ERR_RETURN(READ_DC3MAC_ECC_STATUSr(phy, &ecc));
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_ECC_STATUSr, ecc, TX_CDC_SINGLE_BIT_ERRf);
            break;
        case portmodIntrTypeTxCdcDoubleBitErr :
            PHYMOD_IF_ERR_RETURN(READ_DC3MAC_ECC_STATUSr(phy, &ecc));
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_ECC_STATUSr, ecc, TX_CDC_DOUBLE_BIT_ERRf);
            break;
        case portmodIntrTypeMibMemSingleBitErr :
            PHYMOD_IF_ERR_RETURN(READ_DC3MAC_ECC_STATUSr(phy, &ecc));
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_ECC_STATUSr, ecc, MIB_COUNTER_SINGLE_BIT_ERRf);
            break;
        case portmodIntrTypeMibMemDoubleBitErr :
            PHYMOD_IF_ERR_RETURN(READ_DC3MAC_ECC_STATUSr(phy, &ecc));
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_ECC_STATUSr, ecc, MIB_COUNTER_DOUBLE_BIT_ERRf);
            break;
        case portmodIntrTypeMibMemMultipleBitErr :
            PHYMOD_IF_ERR_RETURN(READ_DC3MAC_ECC_STATUSr(phy, &ecc));
            *value = aperta2_soc_reg_field_get(phy, DC3MAC_ECC_STATUSr, ecc, MIB_COUNTER_MULTIPLE_ERRf);
            break;
        default:
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (_SOC_MSG("Invalid interrupt type")));
            break;
    }


    return PHYMOD_E_NONE;
}

int plp_aperta2_dc3mac_interrupts_status_get(const plp_aperta2_phymod_phy_access_t *phy, int arr_max_size,
                                 uint32_t* intr_arr, uint32_t* size)
{
    uint32_t cnt = 0;
    BCM_APERTA2_DC3MAC_DC3MAC_INTR_STATUSr_t rval;
    BCM_APERTA2_DC3MAC_DC3MAC_FIFO_STATUSr_t fifo_sts;
    BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_STATUSr_t lss_status;
    BCM_APERTA2_DC3MAC_DC3MAC_ECC_STATUSr_t ecc_status;
    

    /* Clear on Read */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_INTR_STATUSr(phy, &rval));

    /* Read FIFO STATUS - Clear on Read Operation */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_FIFO_STATUSr(phy, &fifo_sts));

    if (aperta2_soc_reg_field_get(phy, DC3MAC_FIFO_STATUSr, fifo_sts, TX_PFC_FIFO_OVERFLOWf)) {
        if (cnt >= arr_max_size) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (_SOC_MSG("Insufficient Array size")));
        }
        intr_arr[cnt++] = portmodIntrTypeTxPfcFifoOverflow;
    }

    if (aperta2_soc_reg_field_get(phy, DC3MAC_FIFO_STATUSr, fifo_sts, RX_PFC_FIFO_OVERFLOWf)) {
        if (cnt >= arr_max_size) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (_SOC_MSG("Insufficient Array size")));
        }
        intr_arr[cnt++] = portmodIntrTypeRxPfcFifoOverflow;
    }

    if (aperta2_soc_reg_field_get(phy, DC3MAC_FIFO_STATUSr, fifo_sts, FDR_INTERRUPTf)) {
        if (cnt >= arr_max_size) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (_SOC_MSG("Insufficient Array size")));
        }
        intr_arr[cnt++] = portmodIntrTypeFdrInterrupt;
    }

    if (aperta2_soc_reg_field_get(phy, DC3MAC_FIFO_STATUSr, fifo_sts, TX_PKT_UNDERFLOWf)) {
        if (cnt >= arr_max_size) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (_SOC_MSG("Insufficient Array size")));
        }
        intr_arr[cnt++] = portmodIntrTypeTxPktUnderflow;
    }

    if (aperta2_soc_reg_field_get(phy, DC3MAC_FIFO_STATUSr, fifo_sts, TX_PKT_OVERFLOWf)) {
        if (cnt >= arr_max_size) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (_SOC_MSG("Insufficient Array size")));
        }
        intr_arr[cnt++] = portmodIntrTypeTxPktOverflow;
    }

    /* Read RX LSS STATUS register - Clear on Read Operation */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_LSS_STATUSr(phy, &lss_status));

    if (aperta2_soc_reg_field_get(phy, DC3MAC_RX_LSS_STATUSr, lss_status,
                          LOCAL_FAULT_STATUSf)) {
        if (cnt >= arr_max_size) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (_SOC_MSG("Insufficient Array size")));
        }
        intr_arr[cnt++] = portmodIntrTypeLocalFaultStatus;
    }

    if (aperta2_soc_reg_field_get(phy, DC3MAC_RX_LSS_STATUSr, lss_status,
                          REMOTE_FAULT_STATUSf)) {
        if (cnt >= arr_max_size) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (_SOC_MSG("Insufficient Array size")));
        }
        intr_arr[cnt++] = portmodIntrTypeRemoteFaultStatus;
    }

    /* Read ECC STATUS register - Clear on Read Operation */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_ECC_STATUSr(phy, &ecc_status));

    if (aperta2_soc_reg_field_get(phy, DC3MAC_ECC_STATUSr, ecc_status,
                          TX_CDC_SINGLE_BIT_ERRf)) {
        if (cnt >= arr_max_size) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (_SOC_MSG("Insufficient Array size")));
        }
        intr_arr[cnt++] = portmodIntrTypeTxCdcSingleBitErr;
    }

    if (aperta2_soc_reg_field_get(phy, DC3MAC_ECC_STATUSr, ecc_status,
                          TX_CDC_DOUBLE_BIT_ERRf)) {
        if (cnt >= arr_max_size) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (_SOC_MSG("Insufficient Array size")));
        }
        intr_arr[cnt++] = portmodIntrTypeTxCdcDoubleBitErr;
    }

    if (aperta2_soc_reg_field_get(phy, DC3MAC_ECC_STATUSr, ecc_status,
                          MIB_COUNTER_SINGLE_BIT_ERRf)) {
        if (cnt >= arr_max_size) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (_SOC_MSG("Insufficient Array size")));
        }
        intr_arr[cnt++] = portmodIntrTypeMibMemSingleBitErr;
    }

    if (aperta2_soc_reg_field_get(phy, DC3MAC_ECC_STATUSr, ecc_status,
                          MIB_COUNTER_DOUBLE_BIT_ERRf)) {
        if (cnt >= arr_max_size) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (_SOC_MSG("Insufficient Array size")));
        }
        intr_arr[cnt++] = portmodIntrTypeMibMemDoubleBitErr;
    }

    if (aperta2_soc_reg_field_get(phy, DC3MAC_ECC_STATUSr, ecc_status,
                          MIB_COUNTER_MULTIPLE_ERRf)) {
        if (cnt >= arr_max_size) {
            APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                              (_SOC_MSG("Insufficient Array size")));
        }
        intr_arr[cnt++] = portmodIntrTypeMibMemMultipleBitErr;
    }

    *size = cnt;


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_mib_counter_control_set(const plp_aperta2_phymod_phy_access_t *phy,
                               int enable, int clear)
{
    BCM_APERTA2_DC3MAC_DC3MAC_MIB_COUNTER_CTRLr_t rval;
    

    /*
     * For enabling the MIB statistics counters for a port, the ENABLE field
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
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_MIB_COUNTER_CTRLr(phy, &rval));
    aperta2_soc_reg_field_set(phy, DC3MAC_MIB_COUNTER_CTRLr, rval,
                      ENABLEf, enable);
    aperta2_soc_reg_field_set(phy, DC3MAC_MIB_COUNTER_CTRLr, rval, CNT_CLEARf, clear);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_MIB_COUNTER_CTRLr(phy, rval));

    /*
     * set CLEARf to 0, this operation is not mandatory, adding to
     * remove ambiguity in interpretation if a read on this register
     */
    if (clear) {
        PHYMOD_IF_ERR_RETURN(READ_DC3MAC_MIB_COUNTER_CTRLr(phy, &rval));
        aperta2_soc_reg_field_set(phy, DC3MAC_MIB_COUNTER_CTRLr, rval, CNT_CLEARf, 0);
        PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_MIB_COUNTER_CTRLr(phy, rval));
    }


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_mib_oversize_set(const plp_aperta2_phymod_phy_access_t *phy,
                        uint32_t size)
{
    BCM_APERTA2_DC3MAC_DC3MAC_MIB_COUNTER_CTRLr_t rval;

    

    /*
     * The maximum packet size that is used in statistics counter updates.
     * default size is 1518. Note if RX_MAX_SIZE(max frame size received)
     * is greater than CNTMAXSIZE_Nf, a good packet that is valid CRC and
     * contains no other errors, will increment the OVR(oversize) counters
     * if the length of the packet > CNTMAXSIZE < RX_MAX_SIZE values.
     * Having CNTMAXSIZE > RX_MAX_SIZE is not recommended.
     * This is generally taken care in the statistics module while
     * accumulating the counts.
     */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_MIB_COUNTER_CTRLr(phy, &rval));
    aperta2_soc_reg_field_set(phy, DC3MAC_MIB_COUNTER_CTRLr, rval, CNTMAXSIZEf, size);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_MIB_COUNTER_CTRLr(phy, rval));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_mib_oversize_get(const plp_aperta2_phymod_phy_access_t *phy,
                        uint32_t *size)
{
    BCM_APERTA2_DC3MAC_DC3MAC_MIB_COUNTER_CTRLr_t rval;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_MIB_COUNTER_CTRLr(phy, &rval));
    *size = aperta2_soc_reg_field_get(phy, DC3MAC_MIB_COUNTER_CTRLr, rval, CNTMAXSIZEf);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_pass_control_frame_set(const plp_aperta2_phymod_phy_access_t *phy,
                              uint32_t enable)
{
    /* FIXME later */
    return  PHYMOD_E_NONE;

   /* int ioerr = 0;
    * DC3MAC_RX_CTRLr_t rx_ctrl;
    */

    /*
     * This configuration is used to drop or pass all control frames
     * (with ether type 0x8808) except pause packets.
     * If set, all control frames are passed to system side.
     * if reset, control frames (including pfc frames wih ether type 0x8808)i
     * are dropped in DC3MAC.
     */
   /*
    ioerr += READ_DC3MAC_RX_CTRLr(phy, &rx_ctrl);
    DC3MAC_RX_CTRLr_RX_PASS_CTRLf_SET(rx_ctrl, (enable? 1: 0));
    ioerr += WRITE_DC3MAC_RX_CTRLr(phy, rx_ctrl);

    return ioerr ? SHR_E_ACCESS : PHYMOD_E_NONE;
    */
}


int
plp_aperta2_dc3mac_pass_control_frame_get(const plp_aperta2_phymod_phy_access_t *phy,
                              uint32_t *enable)
{
    /* FIXME later */
    return  PHYMOD_E_NONE;
    /*
    int ioerr = 0;
    DC3MAC_RX_CTRLr_t rx_ctrl;

    ioerr += READ_DC3MAC_RX_CTRLr(phy, &rx_ctrl);
    *enable = DC3MAC_RX_CTRLr_RX_PASS_CTRLf_GET(rx_ctrl);

    return ioerr ? SHR_E_ACCESS : PHYMOD_E_NONE;
    */
}


int
plp_aperta2_dc3mac_pass_pfc_frame_set(const plp_aperta2_phymod_phy_access_t *phy,
                          uint32_t enable)
{
    /* FIXME later */
    return  PHYMOD_E_NONE;

    /*
    int ioerr = 0;
    uint32_t status = 0;
    DC3MAC_RX_CTRLr_t rx_ctrl;
    */

    /*
     * This configuration is used to pass or drop PFC packets when
     * PFC_ETH_TYPE is not equal to 0x8808.
     * If set, PFC frames are passed to system side.
     * If reset, PFC frames are dropped in DC3MAC.
     */
    /*
    ioerr += READ_DC3MAC_RX_CTRLr(phy, &rx_ctrl);
    status = DC3MAC_RX_CTRLr_RX_PASS_PFCf_GET(rx_ctrl);

    if (status != enable) {
        DC3MAC_RX_CTRLr_RX_PASS_PFCf_SET(rx_ctrl, (enable? 1: 0));
    }
    ioerr += WRITE_DC3MAC_RX_CTRLr(phy, rx_ctrl);

    return ioerr ? SHR_E_ACCESS : PHYMOD_E_NONE;
    */
}


int
plp_aperta2_dc3mac_pass_pfc_frame_get(const plp_aperta2_phymod_phy_access_t *phy,
                          uint32_t *enable)
{
    /* FIXME later */
    return  PHYMOD_E_NONE;

    /*
    int ioerr = 0;
    DC3MAC_RX_CTRLr_t rx_ctrl;

    ioerr += READ_DC3MAC_RX_CTRLr(phy, &rx_ctrl);
    *enable = DC3MAC_RX_CTRLr_RX_PASS_PFCf_GET(rx_ctrl);

    return ioerr ? SHR_E_ACCESS : PHYMOD_E_NONE;
    */
}


int
plp_aperta2_dc3mac_pass_pause_frame_set(const plp_aperta2_phymod_phy_access_t *phy,
                            uint32_t enable)
{
    /* FIXME later */
    return  PHYMOD_E_NONE;
    /*
    int ioerr = 0;
    DC3MAC_RX_CTRLr_t rx_ctrl;
    */

    /*
     * If set, PAUSE frames are passed to sytem side.
     * If reset, PAUSE frames are dropped in DC3MAC
     */
    /*
    ioerr += READ_DC3MAC_RX_CTRLr(phy, &rx_ctrl);
    DC3MAC_RX_CTRLr_RX_PASS_PAUSEf_SET(rx_ctrl, (enable? 1: 0));
    ioerr += WRITE_DC3MAC_RX_CTRLr(phy, rx_ctrl);

    return ioerr ? SHR_E_ACCESS : PHYMOD_E_NONE;
    */
}


int
plp_aperta2_dc3mac_pass_pause_frame_get(const plp_aperta2_phymod_phy_access_t *phy,
                            uint32_t *enable)
{
    /* FIXME later */
    return  PHYMOD_E_NONE;

    /*
    int ioerr = 0;
    DC3MAC_RX_CTRLr_t rx_ctrl;

    ioerr += READ_DC3MAC_RX_CTRLr(phy, &rx_ctrl);
    *enable = DC3MAC_RX_CTRLr_RX_PASS_PAUSEf_GET(rx_ctrl);

    return ioerr ? SHR_E_ACCESS : PHYMOD_E_NONE;
    */
}

int
plp_aperta2_dc3mac_discard_set(const plp_aperta2_phymod_phy_access_t *phy,
                   uint32_t discard)
{
    BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr_t rval;
    

    /* Clear Discard fields */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_TX_CTRLr(phy, &rval));
    aperta2_soc_reg_field_set(phy, DC3MAC_TX_CTRLr, rval, DISCARDf, discard);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_TX_CTRLr(phy, rval));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_discard_get(const plp_aperta2_phymod_phy_access_t *phy,
                   uint32_t *discard)
{
    BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr_t rval;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_TX_CTRLr(phy, &rval));
    *discard = aperta2_soc_reg_field_get(phy, DC3MAC_TX_CTRLr, rval, DISCARDf);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_stall_tx_enable_get(const plp_aperta2_phymod_phy_access_t *phy,
                           int *enable)
{
    BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr_t rval;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_TX_CTRLr(phy, &rval));
    *enable = aperta2_soc_reg_field_get(phy, DC3MAC_TX_CTRLr, rval, STALL_TXf);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_stall_tx_enable_set(const plp_aperta2_phymod_phy_access_t *phy,
                           int enable)
{
    BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr_t rval;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_TX_CTRLr(phy, &rval));
    aperta2_soc_reg_field_set(phy, DC3MAC_TX_CTRLr, rval, STALL_TXf, enable);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_TX_CTRLr(phy, rval));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_runt_threshold_get(const plp_aperta2_phymod_phy_access_t *phy,
                          uint32_t *value)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_CTRLr_t rval;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_CTRLr(phy, &rval));
    *value = aperta2_soc_reg_field_get(phy, DC3MAC_RX_CTRLr, rval, RUNT_THRESHOLDf);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_port_init(const plp_aperta2_phymod_phy_access_t *phy,
                  uint32_t init)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_CTRLr_t rx_ctrl;
    BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr_t tx_ctrl;
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t mac_ctrl;
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_CTRLr_t pfc_ctrl; 
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_TYPE_OPCODEr_t pfc_opcode;
    BCM_APERTA2_DC3MAC_DC3MAC_PAUSE_CTRLr_t pause_ctrl;
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_DAr_t pfc_da;
    uint32_t da[2];
    portmod_remote_fault_control_t remote_fault_control;
    portmod_local_fault_control_t local_fault_control;

    

    if (!init) {
        return PHYMOD_E_NONE;
    }

    /* Set recieve maximum frame size 16K. */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_dc3mac_frame_max_set(phy, DC3MAC_JUMBO_PACKET_SIZE));

    /*
     * Enable and Clear the DC3MAC MIB counters
     * No need to explicitly set the clear flag to 0 since the
     * function internally unsets the clear flag
     */
    PHYMOD_IF_ERR_RETURN
        (plp_aperta2_dc3mac_mib_counter_control_set(phy, 1, 1));

    /* RX Control */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_CTRLr(phy, &rx_ctrl));
    aperta2_soc_reg_field_set(phy, DC3MAC_RX_CTRLr, rx_ctrl, STRIP_CRCf, 1);
    /* FIXME later */
    /*
    DC3MAC_RX_CTRLr_RX_PASS_PAUSEf_SET(rx_ctrl, 0);
    DC3MAC_RX_CTRLr_RX_PASS_PFCf_SET(rx_ctrl, 0);
    */
    aperta2_soc_reg_field_set(phy, DC3MAC_RX_CTRLr, rx_ctrl, RUNT_THRESHOLDf, 0x40);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_RX_CTRLr(phy, rx_ctrl));

    /* Configure MAC Tx. */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_TX_CTRLr(phy, &tx_ctrl));
    aperta2_soc_reg_field_set(phy, DC3MAC_TX_CTRLr, tx_ctrl, DISCARDf, 0);
    /* FIXME later */
    /*
    DC3MAC_TX_CTRLr_PAD_ENf_SET(tx_ctrl, 0);
    DC3MAC_TX_CTRLr_PAD_THRESHOLDf_SET(tx_ctrl, 64);
    */
    aperta2_soc_reg_field_set(phy, DC3MAC_TX_CTRLr, tx_ctrl, CRC_MODEf, 0);
    aperta2_soc_reg_field_set(phy, DC3MAC_TX_CTRLr, tx_ctrl, AVERAGE_IPGf, 12);
    aperta2_soc_reg_field_set(phy, DC3MAC_TX_CTRLr, tx_ctrl, TX_THRESHOLDf, 2);
   PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_TX_CTRLr(phy, tx_ctrl));

    /* Disable local and remote faults. */
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_portmod_remote_fault_control_t_init(0,&remote_fault_control));
    remote_fault_control.enable = TRUE;
    remote_fault_control.drop_tx_on_fault = TRUE;
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_remote_fault_disable_set(phy,
                                                    &remote_fault_control));

    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_portmod_local_fault_control_t_init(0, &local_fault_control));
    local_fault_control.enable = TRUE;
    local_fault_control.drop_tx_on_fault = TRUE;
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_local_fault_disable_set(phy,
                                                   &local_fault_control));

    /* Disable pause settings. */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PAUSE_CTRLr(phy, pause_ctrl));
    aperta2_soc_reg_field_set(phy, DC3MAC_PAUSE_CTRLr, pause_ctrl, TX_PAUSE_ENf, 1);
    aperta2_soc_reg_field_set(phy, DC3MAC_PAUSE_CTRLr, pause_ctrl, RX_PAUSE_ENf, 1);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PAUSE_CTRLr(phy, pause_ctrl));

    /* Disable pfc settings. */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PFC_CTRLr(phy, &pfc_ctrl));
    aperta2_soc_reg_field_set(phy, DC3MAC_PFC_CTRLr, pfc_ctrl, PFC_STATS_ENf, 1);
    aperta2_soc_reg_field_set(phy, DC3MAC_PFC_CTRLr, pfc_ctrl, TX_PFC_ENf, 0);
    aperta2_soc_reg_field_set(phy, DC3MAC_PFC_CTRLr, pfc_ctrl, RX_PFC_ENf, 0);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PFC_CTRLr(phy, pfc_ctrl));

    /* Reset PFC MAC DA. */
    da[1] = 0x0180;
    da[0] = 0xc2000001;
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_DAr_SET(pfc_da,0,da[0]);
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_DAr_SET(pfc_da,1,da[1]);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PFC_DAr(phy, pfc_da));
    (void)da;
    /* Reset PFC OPCODE. */
    /* Reset PFC ETH TYPE. */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PFC_TYPE_OPCODEr(phy, &pfc_opcode));
    aperta2_soc_reg_field_set(phy, DC3MAC_PFC_TYPE_OPCODEr, pfc_opcode, PFC_OPCODEf, 0x101);
    aperta2_soc_reg_field_set(phy, DC3MAC_PFC_TYPE_OPCODEr, pfc_opcode, PFC_ETH_TYPEf, 0x8808);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PFC_TYPE_OPCODEr(phy, pfc_opcode));

    /* MAC control settings. */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(phy, &mac_ctrl));
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, mac_ctrl, TX_ENf, 0);
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, mac_ctrl, RX_ENf, 0);
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, mac_ctrl, MAC_LINK_DOWN_SEQ_ENf, 0);
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, mac_ctrl, SOFT_RESETf, 1);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_CTRLr(phy, mac_ctrl));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_force_pfc_xon_set(const plp_aperta2_phymod_phy_access_t *phy,
                         uint32_t value)
{
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_CTRLr_t pfc_ctrl;
    
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PFC_CTRLr(phy, &pfc_ctrl));
    aperta2_soc_reg_field_set(phy, DC3MAC_PFC_CTRLr, pfc_ctrl, FORCE_PFC_XONf, value);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PFC_CTRLr(phy, pfc_ctrl));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_force_pfc_xon_get(const plp_aperta2_phymod_phy_access_t *phy,
                         uint32_t *value)
{
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_CTRLr_t pfc_ctrl;

    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PFC_CTRLr(phy, &pfc_ctrl));
    *value = aperta2_soc_reg_field_get(phy, DC3MAC_PFC_CTRLr, pfc_ctrl, FORCE_PFC_XONf);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_rsv_mask_set(const plp_aperta2_phymod_phy_access_t *phy,
                    uint32_t rsv_mask)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RSV_MASKr_t reg_mask;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RSV_MASKr(phy, &reg_mask));
    aperta2_soc_reg_field_set(phy, DC3MAC_RSV_MASKr, reg_mask, MASKf, rsv_mask);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_RSV_MASKr(phy, reg_mask));


   return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_rsv_mask_get(const plp_aperta2_phymod_phy_access_t *phy,
                    uint32_t *rsv_mask)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RSV_MASKr_t reg_mask;
    
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RSV_MASKr(phy, &reg_mask));
    *rsv_mask = aperta2_soc_reg_field_get(phy, DC3MAC_RSV_MASKr, reg_mask, MASKf);


   return PHYMOD_E_NONE;
}

/*
 * This function controls which RSV(Receive statistics vector) event
 * causes a purge event that triggers RXERR to be set for the packet
 * sent by the MAC to the IP. These bits are used to mask RSV[34:16]
 * for DC3MAC; bit[18] of MASK maps to bit[34] of RSV, bit[0] of MASK
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
int
plp_aperta2_dc3mac_rsv_selective_mask_set(const plp_aperta2_phymod_phy_access_t *phy,
                              uint32_t flags, uint32_t value)
{
    int i = 0;
    uint32_t tmp_mask = APERTA2_DC3MAC_RSV_MASK_MIN;
    uint32_t rsv_mask_bmap[1] = {0};

    

    if (flags > APERTA2_DC3MAC_RSV_MASK_ALL) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                           (_SOC_MSG("invalid mask %x"), flags));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rsv_mask_get(phy, rsv_mask_bmap));

    while(tmp_mask <= APERTA2_DC3MAC_RSV_MASK_MAX) {
        if (flags & tmp_mask) {
            /*
             * if value = 1 means Enable, set the mask to 0.
             * if value = 0 means Purge, set the mask to 1.
             */
            if (value) {
                rsv_mask_bmap[0] &= ~i;
            } else {
                rsv_mask_bmap[0] |= i;
            }
        }
        tmp_mask = (1 << ++i);
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rsv_mask_set(phy, rsv_mask_bmap[0]));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_rsv_selective_mask_get(const plp_aperta2_phymod_phy_access_t *phy,
                              uint32_t flags, uint32_t *value)
{
    uint32_t rsv_mask;

    

    /* Check if only 1 bit is set in flags */
    if ((flags) & (flags - 1)) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                           (_SOC_MSG("invalid mask %x"), flags));
    }

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_rsv_mask_get(phy, &rsv_mask));

    /*
     * if bit in rsv_mask = 0 means Enable, return 1.
     * if bit in rsv_mask = 1 means Purge, return 0.
     */
    *value = (rsv_mask & flags)? 0: 1;


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_strip_crc_get(const plp_aperta2_phymod_phy_access_t *phy,
                     uint32_t *enable)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_CTRLr_t rval;

    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_CTRLr(phy, &rval));
    *enable = aperta2_soc_reg_field_get(phy, DC3MAC_RX_CTRLr, rval, STRIP_CRCf);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_strip_crc_set(const plp_aperta2_phymod_phy_access_t *phy,
                     uint32_t enable)
{
    uint32_t status = 0;
    BCM_APERTA2_DC3MAC_DC3MAC_RX_CTRLr_t rx_ctrl;

    

    /* If set, CRC is stripped from the received packet */
     PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_CTRLr(phy, &rx_ctrl));
    status = aperta2_soc_reg_field_get(phy, DC3MAC_RX_CTRLr, rx_ctrl, STRIP_CRCf);
    if (status != enable) {
        aperta2_soc_reg_field_set(phy, DC3MAC_RX_CTRLr, rx_ctrl, STRIP_CRCf, (enable? 1: 0));
    }
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_RX_CTRLr(phy, rx_ctrl));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_tx_crc_mode_get(const plp_aperta2_phymod_phy_access_t *phy,
                       uint32_t *crc_mode)
{
    BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr_t tx_ctrl;

    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_TX_CTRLr(phy, &tx_ctrl));
    *crc_mode = aperta2_soc_reg_field_get(phy, DC3MAC_TX_CTRLr, tx_ctrl, CRC_MODEf);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_tx_crc_mode_set(const plp_aperta2_phymod_phy_access_t *phy,
                       uint32_t crc_mode)
{
    BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr_t tx_ctrl;

    

    /* Validate crc_mode.  */
    if (crc_mode > DC3MAC_CRC_MODE_AUTO) {
        APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                           (_SOC_MSG("illegal CRC mode %d"), crc_mode));
    }

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_TX_CTRLr(phy, &tx_ctrl));
    aperta2_soc_reg_field_set(phy, DC3MAC_TX_CTRLr, tx_ctrl, CRC_MODEf, crc_mode);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_TX_CTRLr(phy, tx_ctrl));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_tx_threshold_get(const plp_aperta2_phymod_phy_access_t *phy,
                        uint32_t *threshold)
{
    BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr_t tx_ctrl;

    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_TX_CTRLr(phy, &tx_ctrl));
    *threshold = aperta2_soc_reg_field_get(phy, DC3MAC_TX_CTRLr, tx_ctrl, TX_THRESHOLDf);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_tx_threshold_set(const plp_aperta2_phymod_phy_access_t *phy,
                        uint32_t threshold)
{
    BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr_t tx_ctrl;

    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_TX_CTRLr(phy, &tx_ctrl));
    aperta2_soc_reg_field_set(phy, DC3MAC_TX_CTRLr, tx_ctrl, TX_THRESHOLDf, threshold);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_TX_CTRLr(phy, tx_ctrl));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3port_port_mode_set(const plp_aperta2_phymod_phy_access_t *phy,
                      uint32_t flags, uint32_t lane_mask)
{
    int num_lanes = 0, port_is_upper_half = 0;
    uint32_t port_mode_upper_lower;
    uint32_t port_mode = DC3PORT_PORT_MODE_QUAD;
    uint32_t new_port_mode = DC3PORT_PORT_MODE_QUAD;
    uint32_t new_port_mode_mask = 0xf0;
    BCM_APERTA2_DC3MAC_DC3PORT_MODEr_t aperta2_dc3port_mode;

   

    port_mode_upper_lower = DC3PORT_PORT_MODE_QUAD << 4 |
                            DC3PORT_PORT_MODE_QUAD;

    num_lanes = plp_aperta2_phymod_count_set_bits(lane_mask);
    PHYMOD_IF_ERR_RETURN(READ_DC3PORT_MODEr(phy, &aperta2_dc3port_mode));

    /* next need to check if port is lower half or not */
    if ((lane_mask & 0xf0) && (lane_mask != 0xff)) {
        port_is_upper_half = 1;
        new_port_mode_mask = 0xf;
    } else if (((lane_mask >> 8) & 0xF0) && (lane_mask!= 0xFF)) {
        port_is_upper_half = 1;
        new_port_mode_mask = 0xf;
    }

    port_mode_upper_lower =
        aperta2_soc_reg_field_get(phy, DC3PORT_MODEr, aperta2_dc3port_mode, MAC_PORT_MODEf);
    /* need to get the correct port mode based on port location */
    port_mode = (port_mode_upper_lower >> (port_is_upper_half * 4)) & 0xf;

    /* need to clear 8 lane port mode */
    port_mode &= ~DC3PORT_PORT_MODE_SINGLE_8_LANE;

    if (num_lanes == 8) {
        new_port_mode = DC3PORT_PORT_MODE_SINGLE_8_LANE;
    } else if (num_lanes == 4) {
            new_port_mode = DC3PORT_PORT_MODE_SINGLE;
    } else if (num_lanes == 2) {
        switch (port_mode) {
            case DC3PORT_PORT_MODE_QUAD:
                if ((lane_mask == 0x0c) || (lane_mask == 0xc00) || (lane_mask == 0xc0) || (lane_mask == 0xc000)) {
                    new_port_mode = DC3PORT_PORT_MODE_TRI_012;
                } else if ((lane_mask == 0x03) || (lane_mask == 0x30) || (lane_mask == 0x300) || (lane_mask == 0x3000)) {
                    new_port_mode = DC3PORT_PORT_MODE_TRI_023;
                }
                break;
            case DC3PORT_PORT_MODE_TRI_012:
                if ((lane_mask == 0x3) || (lane_mask == 0x30) || (lane_mask == 0x300) || (lane_mask == 0x3000)) {
                    new_port_mode = DC3PORT_PORT_MODE_DUAL;
                } else if ((lane_mask == 0x0c) || (lane_mask == 0xc0) || (lane_mask == 0xc00) || (lane_mask == 0xc000)  ) {
                    new_port_mode = DC3PORT_PORT_MODE_TRI_012;
                }
                break;
            case DC3PORT_PORT_MODE_TRI_023:
                if ((lane_mask == 0x3) || (lane_mask == 0x30) || (lane_mask == 0x300) || (lane_mask == 0x3000)) {
                    new_port_mode = DC3PORT_PORT_MODE_TRI_023;
                } else if ((lane_mask == 0x0c) || (lane_mask == 0xc0) || (lane_mask == 0xc00) || (lane_mask == 0xc000)) {
                    new_port_mode = DC3PORT_PORT_MODE_DUAL;
                }
                break;
            case DC3PORT_PORT_MODE_DUAL:
                new_port_mode = DC3PORT_PORT_MODE_DUAL;
                break;
            case DC3PORT_PORT_MODE_SINGLE:
                if ((lane_mask == 0x3) || (lane_mask == 0x30) || (lane_mask == 0x300) || (lane_mask == 0x3000)) {
                    new_port_mode = DC3PORT_PORT_MODE_TRI_023;
                } else if ((lane_mask == 0x0c) || (lane_mask == 0xc0) || (lane_mask == 0xc00) || (lane_mask == 0xc000)) {
                    new_port_mode = DC3PORT_PORT_MODE_TRI_012;
                }
                break;
            default:
                return PHYMOD_E_PARAM;
        }
    } else {
        /* num_lanes == 1 */
        switch (port_mode) {
            case DC3PORT_PORT_MODE_QUAD:
                new_port_mode = DC3PORT_PORT_MODE_QUAD;
                break;
            case DC3PORT_PORT_MODE_TRI_012:
                if ((lane_mask == 0x1) || (lane_mask == 0x2) ||
                    (lane_mask == 0x10) || (lane_mask == 0x20) || 
                    (lane_mask == 0x100) || (lane_mask == 0x200) ||
                    (lane_mask == 0x1000) || (lane_mask == 0x2000)) {
                    new_port_mode = DC3PORT_PORT_MODE_TRI_012;
                } else {
                    new_port_mode = DC3PORT_PORT_MODE_QUAD;
                }
                break;
            case DC3PORT_PORT_MODE_TRI_023:
                if ((lane_mask == 0x1) || (lane_mask == 0x2) ||
                    (lane_mask == 0x10) || (lane_mask == 0x20) || 
                    (lane_mask == 0x100) || (lane_mask == 0x200) ||
                    (lane_mask == 0x1000) || (lane_mask == 0x2000)) {
                    new_port_mode = DC3PORT_PORT_MODE_QUAD;
                } else {
                    new_port_mode = DC3PORT_PORT_MODE_TRI_023;
                }
                break;
            case DC3PORT_PORT_MODE_DUAL:
                if ((lane_mask == 0x1) || (lane_mask == 0x2) ||
                    (lane_mask == 0x10) || (lane_mask == 0x20) || 
                    (lane_mask == 0x100) || (lane_mask == 0x200) ||
                    (lane_mask == 0x1000) || (lane_mask == 0x2000)) {
                    new_port_mode = DC3PORT_PORT_MODE_TRI_012;
                } else {
                    new_port_mode = DC3PORT_PORT_MODE_TRI_023;
                }
                break;
            case DC3PORT_PORT_MODE_SINGLE:
                new_port_mode = DC3PORT_PORT_MODE_QUAD;
                break;
           default:
                return PHYMOD_E_PARAM;
        }
    }

    /* if not 8 lane port, need to keep the other half port mode config */
    if (num_lanes != 8) {
        /* first clear 800G mode */
        port_mode_upper_lower &= ~DC3PORT_PORT_MODE_SINGLE_8_LANE;
        new_port_mode <<= port_is_upper_half * 4;
        new_port_mode |= port_mode_upper_lower & new_port_mode_mask;
    }

    aperta2_soc_reg_field_set(phy, DC3PORT_MODEr, aperta2_dc3port_mode, MAC_PORT_MODEf, new_port_mode);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3PORT_MODEr(phy, aperta2_dc3port_mode));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3port_dc3mac_control_set(const plp_aperta2_phymod_phy_access_t *phy,
                           uint32_t reset)
{
    BCM_APERTA2_DC3MAC_DC3PORT_MAC_CONTROLr_t pmac_ctrl;

    

    PHYMOD_IF_ERR_RETURN(READ_DC3PORT_MAC_CONTROLr(phy, &pmac_ctrl));

    if (reset) {
        aperta2_soc_reg_field_set(phy, DC3PORT_MAC_CONTROLr, pmac_ctrl, MAC_RESETf, 1);
    } else {
        aperta2_soc_reg_field_set(phy, DC3PORT_MAC_CONTROLr, pmac_ctrl, MAC_RESETf, 0);
    }
    PHYMOD_IF_ERR_RETURN(WRITE_DC3PORT_MAC_CONTROLr(phy, pmac_ctrl));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3port_dc3mac_control_get(const plp_aperta2_phymod_phy_access_t *phy,
                           uint32_t *reset)
{
    BCM_APERTA2_DC3MAC_DC3PORT_MAC_CONTROLr_t pmac_ctrl;

    

    PHYMOD_IF_ERR_RETURN(READ_DC3PORT_MAC_CONTROLr(phy, &pmac_ctrl));
    *reset = aperta2_soc_reg_field_get(phy, DC3PORT_MAC_CONTROLr, pmac_ctrl, MAC_RESETf);


    return PHYMOD_E_NONE;

}

int
plp_aperta2_dc3port_tsc_ctrl_get(const plp_aperta2_phymod_phy_access_t *phy,
                     uint32_t *tsc_rstb, uint32_t *tsc_pwrdwn)
{
    BCM_APERTA2_DC3MAC_DC3PORT_XGXS0_CTRLr_t tsc_ctrl;

    PHYMOD_IF_ERR_RETURN(READ_DC3PORT_XGXS0_CTRLr(phy, &tsc_ctrl));
    *tsc_rstb =
        aperta2_soc_reg_field_get(phy, DC3PORT_XGXS0_CTRLr, tsc_ctrl, TSC_RSTBf);
    *tsc_pwrdwn = 
        aperta2_soc_reg_field_get(phy, DC3PORT_XGXS0_CTRLr, tsc_ctrl, TSC_PWRDWNf);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3port_tsc_ctrl_set(const plp_aperta2_phymod_phy_access_t *phy,
                     int tsc_pwr_on)
{
    BCM_APERTA2_DC3MAC_DC3PORT_XGXS0_CTRLr_t tsc_ctrl;

    PHYMOD_IF_ERR_RETURN(READ_DC3PORT_XGXS0_CTRLr(phy, &tsc_ctrl));
    if (tsc_pwr_on) {
        aperta2_soc_reg_field_set(phy, DC3PORT_XGXS0_CTRLr, tsc_ctrl, TSC_PWRDWNf, 1);
/*        aperta2_soc_reg_field_set(phy, DC3PORT_XGXS0_CTRLr, tsc_ctrl, TSC_RSTBf, 0);*/
        PHYMOD_IF_ERR_RETURN(WRITE_DC3PORT_XGXS0_CTRLr(phy, tsc_ctrl));
        PHYMOD_USLEEP(1000);
        aperta2_soc_reg_field_set(phy, DC3PORT_XGXS0_CTRLr, tsc_ctrl, TSC_PWRDWNf, 0);
       /* aperta2_soc_reg_field_set(phy, DC3PORT_XGXS0_CTRLr, tsc_ctrl, TSC_RSTBf, 1);*/
        PHYMOD_IF_ERR_RETURN(WRITE_DC3PORT_XGXS0_CTRLr(phy, tsc_ctrl));
    } else {
        aperta2_soc_reg_field_set(phy, DC3PORT_XGXS0_CTRLr, tsc_ctrl, TSC_PWRDWNf, 1);
        aperta2_soc_reg_field_set(phy, DC3PORT_XGXS0_CTRLr, tsc_ctrl, TSC_RSTBf, 0);
        PHYMOD_IF_ERR_RETURN(WRITE_DC3PORT_XGXS0_CTRLr(phy, tsc_ctrl));
    }


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3port_link_status_get(const plp_aperta2_phymod_phy_access_t *phy,
                        uint32_t start_lane, int* link)
{
    

    switch(start_lane) {
        case 0:{
            BCM_APERTA2_DC3MAC_DC3PORT_XGXS0_LN0_STATUSr_t ln_status;
            PHYMOD_IF_ERR_RETURN(READ_DC3PORT_XGXS0_LN0_STATUSr(phy, &ln_status));
            *link = aperta2_soc_reg_field_get(phy, DC3PORT_XGXS0_LN0_STATUSr, ln_status, LINK_STATUSf);
            break;
               }
        case 1:{
            BCM_APERTA2_DC3MAC_DC3PORT_XGXS0_LN1_STATUSr_t ln_status;
            PHYMOD_IF_ERR_RETURN(READ_DC3PORT_XGXS0_LN1_STATUSr(phy, &ln_status));
            *link = aperta2_soc_reg_field_get(phy, DC3PORT_XGXS0_LN1_STATUSr, ln_status, LINK_STATUSf);
            break;
               }
        case 2:{
            BCM_APERTA2_DC3MAC_DC3PORT_XGXS0_LN2_STATUSr_t ln_status;
            PHYMOD_IF_ERR_RETURN(READ_DC3PORT_XGXS0_LN2_STATUSr(phy, &ln_status));
            *link = aperta2_soc_reg_field_get(phy, DC3PORT_XGXS0_LN2_STATUSr, ln_status, LINK_STATUSf);
            break;
               }
        case 3:{
            BCM_APERTA2_DC3MAC_DC3PORT_XGXS0_LN3_STATUSr_t ln_status;
            PHYMOD_IF_ERR_RETURN(READ_DC3PORT_XGXS0_LN3_STATUSr(phy, &ln_status));
            *link = aperta2_soc_reg_field_get(phy, DC3PORT_XGXS0_LN3_STATUSr, ln_status, LINK_STATUSf);
            break;
               }
        case 4:{
            BCM_APERTA2_DC3MAC_DC3PORT_XGXS0_LN4_STATUSr_t ln_status;
            PHYMOD_IF_ERR_RETURN(READ_DC3PORT_XGXS0_LN4_STATUSr(phy, &ln_status));
            *link = aperta2_soc_reg_field_get(phy, DC3PORT_XGXS0_LN4_STATUSr, ln_status, LINK_STATUSf);
            break;
               }
        case 5:{
            BCM_APERTA2_DC3MAC_DC3PORT_XGXS0_LN5_STATUSr_t ln_status;
            PHYMOD_IF_ERR_RETURN(READ_DC3PORT_XGXS0_LN5_STATUSr(phy, &ln_status));
            *link = aperta2_soc_reg_field_get(phy, DC3PORT_XGXS0_LN5_STATUSr, ln_status, LINK_STATUSf);
            break;
               }
        case 6:{
            BCM_APERTA2_DC3MAC_DC3PORT_XGXS0_LN6_STATUSr_t ln_status;
            PHYMOD_IF_ERR_RETURN(READ_DC3PORT_XGXS0_LN6_STATUSr(phy, &ln_status));
            *link = aperta2_soc_reg_field_get(phy, DC3PORT_XGXS0_LN6_STATUSr, ln_status, LINK_STATUSf);
            break;
               }
        case 7:{
            BCM_APERTA2_DC3MAC_DC3PORT_XGXS0_LN7_STATUSr_t ln_status;
            PHYMOD_IF_ERR_RETURN(READ_DC3PORT_XGXS0_LN7_STATUSr(phy, &ln_status));
            *link = aperta2_soc_reg_field_get(phy, DC3PORT_XGXS0_LN7_STATUSr, ln_status, LINK_STATUSf);
            break;
               }
        default:
                APERTA2_SOC_EXIT_WITH_ERR(PHYMOD_E_PARAM,
                                  (_SOC_MSG("Invalid starting lane %d"), start_lane));
                break;
    }


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_port_enable_set(const plp_aperta2_phymod_phy_access_t *phy,
                       uint32_t enable)
{
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t mac_ctrl;

    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(phy, &mac_ctrl));
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, mac_ctrl, TX_ENf, enable? 1: 0);
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, mac_ctrl, RX_ENf, enable? 1: 0);
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, mac_ctrl, SOFT_RESETf, enable? 0: 1);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_CTRLr(phy, mac_ctrl));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_port_fdr_symbol_error_window_size_set(const plp_aperta2_phymod_phy_access_t *phy,
                                             uint32_t window_size)
{
    BCM_APERTA2_DC3MAC_DC3MAC_FDR_CTRLr_t fdr_control;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_FDR_CTRLr(phy, &fdr_control));
    aperta2_soc_reg_field_set(phy, DC3MAC_FDR_CTRLr, fdr_control, SYMBOL_ERROR_WINDOW_MODEf,
                      window_size);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_FDR_CTRLr(phy, fdr_control));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_port_fdr_symbol_error_window_size_get(const plp_aperta2_phymod_phy_access_t *phy,
                                             uint32_t* window_size)
{
    BCM_APERTA2_DC3MAC_DC3MAC_FDR_CTRLr_t fdr_control;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_FDR_CTRLr(phy, &fdr_control));
    *window_size = aperta2_soc_reg_field_get(phy, DC3MAC_FDR_CTRLr, fdr_control, SYMBOL_ERROR_WINDOW_MODEf);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_port_fdr_symbol_error_count_threshold_set(const plp_aperta2_phymod_phy_access_t *phy,
                       uint32_t threshold)
{
    BCM_APERTA2_DC3MAC_DC3MAC_FDR_CTRLr_t fdr_control;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_FDR_CTRLr(phy, &fdr_control));
    aperta2_soc_reg_field_set(phy, DC3MAC_FDR_CTRLr, fdr_control, SYMBOL_ERROR_COUNT_THRESHOLDf,
                      threshold);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_FDR_CTRLr(phy, fdr_control));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_port_fdr_symbol_error_count_threshold_get(const plp_aperta2_phymod_phy_access_t *phy,
                                                 uint32_t* threshold)
{
    BCM_APERTA2_DC3MAC_DC3MAC_FDR_CTRLr_t fdr_control;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_FDR_CTRLr(phy, &fdr_control));
    *threshold = aperta2_soc_reg_field_get(phy, DC3MAC_FDR_CTRLr, fdr_control, SYMBOL_ERROR_COUNT_THRESHOLDf);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_port_mac_link_down_seq_enable_set(const plp_aperta2_phymod_phy_access_t *phy,
                                         uint32_t enable)
{
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t mac_ctrl;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(phy, &mac_ctrl));
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, mac_ctrl, MAC_LINK_DOWN_SEQ_ENf,
                      (enable)? 1: 0);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_CTRLr(phy, mac_ctrl));


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_rx_da_timestmap_enable_get(const plp_aperta2_phymod_phy_access_t *phy,
                                  uint32_t *enable)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_CTRLr_t rx_ctrl;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_CTRLr(phy, &rx_ctrl));
    *enable = aperta2_soc_reg_field_get(phy, DC3MAC_RX_CTRLr, rx_ctrl, DA_TIMESTAMP_ENf);


    return PHYMOD_E_NONE;
}

int
plp_aperta2_dc3mac_rx_da_timestmap_enable_set(const plp_aperta2_phymod_phy_access_t *phy,
                                  uint32_t enable)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_CTRLr_t rx_ctrl;

    

    /*
     * This configuration is used to enabel mac da timestamp
     * If set, dc3mac will use the mac da for timestamping.
     * If reset, sfd will be used.
     */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_CTRLr(phy, &rx_ctrl));
    aperta2_soc_reg_field_set(phy, DC3MAC_RX_CTRLr, rx_ctrl, DA_TIMESTAMP_ENf, (enable? 1: 0));
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_RX_CTRLr(phy, rx_ctrl));


    return PHYMOD_E_NONE;

}

int
plp_aperta2_dc3mac_reset_check(const plp_aperta2_phymod_phy_access_t *phy, int enable, int *reset)
{
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t rval, orig_rval;

    *reset = 1;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(phy, &rval));
    orig_rval = rval;

    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, rval, TX_ENf, enable? 1: 0);
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, rval, RX_ENf, enable? 1: 0);

    if (rval.v[0] == orig_rval.v[0]) {
        if (enable) {
            *reset = 0;
        } else {
            if (aperta2_soc_reg_field_get(phy, DC3MAC_CTRLr, rval, SOFT_RESETf)) {
                *reset = 0;
            }
        }
    }

    return (PHYMOD_E_NONE);
}

int
plp_aperta2_dc3mac_txfifo_cell_cnt_get(const plp_aperta2_phymod_phy_access_t *phy, uint32_t* val)
{
    BCM_APERTA2_DC3MAC_DC3MAC_TXFIFO_STATUSr_t rval;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_TXFIFO_STATUSr(phy, &rval));
    *val = aperta2_soc_reg_field_get(phy, DC3MAC_TXFIFO_STATUSr, rval, CELL_CNTf);

    return (PHYMOD_E_NONE);
}

int
plp_aperta2_dc3mac_mac_ctrl_set(const plp_aperta2_phymod_phy_access_t *phy, uint64_t ctrl)
{
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t rval;

    COMPILER_64_TO_32_LO(rval.v[0], ctrl);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_CTRLr(phy, rval));
    return (PHYMOD_E_NONE);
}

int
plp_aperta2_dc3port_port_fault_link_status_set(const plp_aperta2_phymod_phy_access_t *phy, int enable) {
    BCM_APERTA2_DC3MAC_DC3PORT_FAULT_LINK_STATUSr_t reg_val;

    

    PHYMOD_IF_ERR_RETURN(READ_DC3PORT_FAULT_LINK_STATUSr(phy, &reg_val));
    aperta2_soc_reg_field_set(phy, DC3PORT_FAULT_LINK_STATUSr, reg_val, REMOTE_FAULTf, enable);
    aperta2_soc_reg_field_set(phy, DC3PORT_FAULT_LINK_STATUSr, reg_val, LOCAL_FAULTf, enable);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3PORT_FAULT_LINK_STATUSr(phy, reg_val));


    return PHYMOD_E_NONE;
}

int plp_aperta2_dc3mac_vlan_tag_set(const plp_aperta2_phymod_phy_access_t *phy, int outer_vlan_tag,
                        int inner_vlan_tag)
{
    BCM_APERTA2_DC3MAC_DC3MAC_VLAN_TAGr_t rval64;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_VLAN_TAGr(phy, &rval64));

    if(inner_vlan_tag == -1) {
       aperta2_soc_reg_field_set(phy, DC3MAC_VLAN_TAGr, rval64,
                             INNER_VLAN_TAG_ENABLEf, 0);
    } else {
       aperta2_soc_reg_field_set(phy, DC3MAC_VLAN_TAGr, rval64,
                             INNER_VLAN_TAGf, inner_vlan_tag);
       aperta2_soc_reg_field_set(phy, DC3MAC_VLAN_TAGr, rval64,
                             INNER_VLAN_TAG_ENABLEf, 1);
    }

    if(outer_vlan_tag == -1) {
       aperta2_soc_reg_field_set(phy, DC3MAC_VLAN_TAGr, rval64,
                             OUTER_VLAN_TAG_ENABLEf, 0);
    } else {
       aperta2_soc_reg_field_set(phy, DC3MAC_VLAN_TAGr, rval64,
                             OUTER_VLAN_TAGf, outer_vlan_tag);
       aperta2_soc_reg_field_set(phy, DC3MAC_VLAN_TAGr, rval64,
                             OUTER_VLAN_TAG_ENABLEf, 1);
    }

    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_VLAN_TAGr(phy, rval64));


    return PHYMOD_E_NONE;
}

int plp_aperta2_dc3mac_vlan_tag_get(const plp_aperta2_phymod_phy_access_t *phy, int *outer_vlan_tag,
                        int *inner_vlan_tag)
{
    BCM_APERTA2_DC3MAC_DC3MAC_VLAN_TAGr_t rval64;
    uint32_t is_enabled = 0;
    

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_VLAN_TAGr(phy, &rval64));

    is_enabled = aperta2_soc_reg_field_get(phy, DC3MAC_VLAN_TAGr, rval64,
                                       INNER_VLAN_TAG_ENABLEf);
    if(is_enabled == 0) {
       *inner_vlan_tag = -1;
    } else {
       *inner_vlan_tag = aperta2_soc_reg_field_get(phy, DC3MAC_VLAN_TAGr,
                                               rval64, INNER_VLAN_TAGf);
    }

    is_enabled = aperta2_soc_reg_field_get(phy, DC3MAC_VLAN_TAGr, rval64,
                                       OUTER_VLAN_TAG_ENABLEf);
    if(is_enabled == 0) {
       *outer_vlan_tag = -1;
    } else {
       *outer_vlan_tag = aperta2_soc_reg_field_get(phy, DC3MAC_VLAN_TAGr,
                                               rval64, OUTER_VLAN_TAGf);
    }


    return PHYMOD_E_NONE;
}

int plp_aperta2_dc3mac_drain_cell_get(const plp_aperta2_phymod_phy_access_t *phy,
                          portmod_drain_cells_t *drain_cells)
{
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_CTRLr_t rval;
    BCM_APERTA2_DC3MAC_DC3MAC_PAUSE_CTRLr_t rval64;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PFC_CTRLr(phy, &rval));
    drain_cells->rx_pfc_en = aperta2_soc_reg_field_get(phy, DC3MAC_PFC_CTRLr, rval,
                                               RX_PFC_ENf);

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PAUSE_CTRLr(phy, &rval64));
    drain_cells->rx_pause = aperta2_soc_reg_field_get(phy, DC3MAC_PAUSE_CTRLr,
                                                  rval64, RX_PAUSE_ENf);
    drain_cells->tx_pause = aperta2_soc_reg_field_get(phy, DC3MAC_PAUSE_CTRLr,
                                                  rval64, TX_PAUSE_ENf);

    return (PHYMOD_E_NONE);
}

int plp_aperta2_dc3mac_drain_cell_stop(const plp_aperta2_phymod_phy_access_t *phy,
                           const portmod_drain_cells_t *drain_cells)
{
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_CTRLr_t rval;
    BCM_APERTA2_DC3MAC_DC3MAC_PAUSE_CTRLr_t rval64;

    /* Clear Discard fields */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_discard_set(phy, 0));

    /* set pause fields */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PAUSE_CTRLr(phy, &rval64));
    aperta2_soc_reg_field_set(phy, DC3MAC_PAUSE_CTRLr, rval64, RX_PAUSE_ENf,
                      drain_cells->rx_pause);
    aperta2_soc_reg_field_set(phy, DC3MAC_PAUSE_CTRLr, rval64, TX_PAUSE_ENf,
                      drain_cells->tx_pause);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PAUSE_CTRLr(phy, rval64));

    /* set pfc rx_en fields */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PFC_CTRLr(phy, &rval));
    aperta2_soc_reg_field_set(phy, DC3MAC_PFC_CTRLr, rval, RX_PFC_ENf,
                      drain_cells->rx_pfc_en);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PFC_CTRLr(phy, rval));

    return (PHYMOD_E_NONE);
}

int plp_aperta2_dc3mac_drain_cell_start(const plp_aperta2_phymod_phy_access_t *phy)
{
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t rval;
    BCM_APERTA2_DC3MAC_DC3MAC_PAUSE_CTRLr_t rval64;
    BCM_APERTA2_DC3MAC_DC3MAC_PFC_CTRLr_t pfc_ctrl;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(phy, &rval));

    /* Don't disable TX since it stops egress and hangs if CPU sends */
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, rval, TX_ENf, 1);
    /* Disable RX */
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, rval, RX_ENf, 0);

    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_CTRLr(phy, rval));

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PAUSE_CTRLr(phy, &rval64));
    aperta2_soc_reg_field_set(phy, DC3MAC_PAUSE_CTRLr, rval64, RX_PAUSE_ENf,0);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PAUSE_CTRLr(phy, rval64));

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_PFC_CTRLr(phy, &pfc_ctrl));
    aperta2_soc_reg_field_set(phy, DC3MAC_PFC_CTRLr, pfc_ctrl, RX_PFC_ENf, 0);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_PFC_CTRLr(phy, pfc_ctrl));

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(phy, &rval));
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, rval, SOFT_RESETf, 1);
    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_CTRLr(phy, rval));

    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_discard_set(phy, 1));

    return (PHYMOD_E_NONE);
}

int plp_aperta2_dc3mac_drain_cells_rx_enable(const plp_aperta2_phymod_phy_access_t *phy, int rx_en)
{
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t rval, orig_rval;
    uint32_t soft_reset = 0;

    /* Enable both TX and RX, de-assert SOFT_RESET */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(phy, &rval));
    orig_rval = rval;

    /* Don't disable TX since it stops egress and hangs if CPU sends */
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, rval, TX_ENf, 1);
    aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, rval, RX_ENf, rx_en ? 1: 0);

    if (rval.v[0] == orig_rval.v[0]) {
        /*
         *  To avoid the unexpected early return to prevent this problem.
         *  1. Problem occurred for disabling process only.
         *  2. To comply original designing scenario, DC3MAC_DC_3_MAC_CTRLr.SOFT_RESET_Nf
         *     is used to early check to see if this port is at disabled state
         *     already.
         */
        soft_reset = aperta2_soc_reg_field_get(phy, DC3MAC_CTRLr, rval, SOFT_RESETf);
        if ((rx_en) || (!rx_en && soft_reset)){
            return PHYMOD_E_NONE;
        }
    }

    if (rx_en) {
        aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, rval, SOFT_RESETf, 0);
    }

    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_CTRLr(phy, rval));

    return (PHYMOD_E_NONE);
}

int plp_aperta2_dc3mac_egress_queue_drain_rx_en(const plp_aperta2_phymod_phy_access_t *phy, int rx_en)
{
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t rval;
    uint32_t txfifo_cell_cnt = 0;
    int retry_cnt = 1000;

    /* Enable RX, de-assert SOFT_RESET */
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(phy, &rval));
    if (rx_en) {
        BCM_APERTA2_DC3MAC_DC3MAC_TX_CTRLr_t aperta2_dc3mac_tx_ctrl_rval;

        PHYMOD_IF_ERR_RETURN(READ_DC3MAC_TX_CTRLr(phy, &aperta2_dc3mac_tx_ctrl_rval));

        aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, rval, RX_ENf, 0);
        PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_CTRLr(phy, rval));

        aperta2_soc_reg_field_set(phy, DC3MAC_TX_CTRLr, aperta2_dc3mac_tx_ctrl_rval, DISCARDf, 1);
        PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_TX_CTRLr(phy, aperta2_dc3mac_tx_ctrl_rval));

        /* Wait until TX fifo cell count is 0 */
            for (;;) {
                PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_txfifo_cell_cnt_get(phy, &txfifo_cell_cnt));
                if (txfifo_cell_cnt == 0) {
                    break;
                }
                retry_cnt--;
                if (retry_cnt == 0) {
                    return (PHYMOD_E_INTERNAL);
                }
            }

        aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, rval, RX_ENf, 1);
        PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_CTRLr(phy, rval));

        aperta2_soc_reg_field_set(phy, DC3MAC_TX_CTRLr, aperta2_dc3mac_tx_ctrl_rval, DISCARDf, 0);
        PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_TX_CTRLr(phy, aperta2_dc3mac_tx_ctrl_rval));
        PHYMOD_USLEEP(10000);
    } else {
        aperta2_soc_reg_field_set(phy, DC3MAC_CTRLr, rval, RX_ENf, 0);
        PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_CTRLr(phy, rval));
    }

    /*Bring mac out of reset */
    PHYMOD_IF_ERR_RETURN(plp_aperta2_dc3mac_reset_set(phy, 0));

    return (PHYMOD_E_NONE);
}

int plp_aperta2_dc3mac_egress_queue_drain_get(const plp_aperta2_phymod_phy_access_t *phy, uint64_t *mac_ctrl,
                                  int *rx_en)
{
    BCM_APERTA2_DC3MAC_DC3MAC_CTRLr_t rval;

    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_CTRLr(phy, &rval));
    COMPILER_64_SET(*mac_ctrl, 0, rval.v[0]);

    *rx_en = aperta2_soc_reg_field_get(phy, DC3MAC_CTRLr, rval, RX_ENf);

    return (PHYMOD_E_NONE);
}

int plp_aperta2_dc3mac_link_fault_os_set(const plp_aperta2_phymod_phy_access_t *phy, int is_remote, uint32_t enable)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_CTRLr_t rval;
    
    enable = enable ? 1 : 0;
    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_LSS_CTRLr(phy, &rval));
    if (enable) {
        if (is_remote) {
            BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_CTRLr_FORCE_REMOTE_FAULT_OSf_SET(rval,1);
            BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_CTRLr_FORCE_LOCAL_FAULT_OSf_SET(rval,0);
        } else {
            BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_CTRLr_FORCE_REMOTE_FAULT_OSf_SET(rval,0);
            BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_CTRLr_FORCE_LOCAL_FAULT_OSf_SET(rval,1);

        }
        aperta2_soc_reg_field_set(phy, DC3MAC_RX_LSS_CTRLr, rval, FAULT_SOURCE_FOR_TXf, 2);
    } else {
        aperta2_soc_reg_field_set(phy, DC3MAC_RX_LSS_CTRLr, rval, FORCE_REMOTE_FAULT_OSf, 0);
        aperta2_soc_reg_field_set(phy, DC3MAC_RX_LSS_CTRLr, rval, FORCE_LOCAL_FAULT_OSf, 0);
        aperta2_soc_reg_field_set(phy, DC3MAC_RX_LSS_CTRLr, rval, FAULT_SOURCE_FOR_TXf, 0);
    }

    PHYMOD_IF_ERR_RETURN(WRITE_DC3MAC_RX_LSS_CTRLr(phy, rval));


    return PHYMOD_E_NONE;
}

int plp_aperta2_dc3mac_link_fault_os_get(const plp_aperta2_phymod_phy_access_t *phy, int is_remote, uint32_t * enable)
{
    BCM_APERTA2_DC3MAC_DC3MAC_RX_LSS_CTRLr_t rval;


    PHYMOD_IF_ERR_RETURN(READ_DC3MAC_RX_LSS_CTRLr(phy, &rval));

    if (is_remote) {
        *enable = aperta2_soc_reg_field_get(phy, DC3MAC_RX_LSS_CTRLr, rval,
                                    FORCE_REMOTE_FAULT_OSf);
    } else {
        *enable = aperta2_soc_reg_field_get(phy, DC3MAC_RX_LSS_CTRLr, rval,
                                    FORCE_LOCAL_FAULT_OSf);
    }


    return PHYMOD_E_NONE;
}

int plp_aperta2_dc3mac_reg64_read(const plp_aperta2_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t *data)
{
    uint64_t temp;
    PHYMOD_IF_ERR_RETURN(
            plp_aperta2_reg64_read(phy, reg_addr, &temp));
    *data = COMPILER_64_LO(temp);
    data++;
    *data = COMPILER_64_HI(temp);
    return 0;
}

int plp_aperta2_dc3mac_reg64_write(const plp_aperta2_phymod_phy_access_t *phy, uint32_t reg_addr, uint32_t *data)
{
    uint64_t reg_data;

    COMPILER_64_SET(reg_data, data[1], data[0]);
    return plp_aperta2_reg64_write(phy, reg_addr, reg_data);
}


#endif
