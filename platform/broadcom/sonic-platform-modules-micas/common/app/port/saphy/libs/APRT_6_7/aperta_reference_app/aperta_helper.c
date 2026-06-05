/*
 *  $Id: aperta_helper.c $
 * $Copyright: (c) 2021 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

#include <stdio.h>
#include <unistd.h>
#include "aperta_helper.h"
/*
* Build and use of function defined in aperta_helper.c
* 1) Copy aperta_helper.c and aperta_helper.h in aperta_reference_app directory
* 2) Update Makefile in aperta_reference_app with flag as INCLUDE_C_FILE= phy_common.c aperta_helper.c
* 3) #include "aperta_helper.h" in application to use API defined in aperta_helper.c
*/

/*! \brief Link recovery for 100G with RS FEC
 *
 *  This API toggle RX when the PCS link is down.
 *
 *  @param chip_name      Represents chip name\n
 *  @param plp_info       Represents PHY access\n
 *
 *  @return rv            SUCCESS (0)
 *                        OR Error code
 */

int plp_helper_100G_FEC_link_recovery(char* chip_name, bcm_plp_access_t plp_info)
{
    int speed = 0, if_type = 0, ref_clk = 0;
    int if_mode = 0, rv = 0;
    unsigned int pmd_lock = 0, index = 0, link_status = 0;
    unsigned int power_rx_org = 0, power_tx_org = 0;
    bcm_plp_aperta_device_aux_modes_t device_aux_mode;

    if_error((bcm_plp_rx_pmd_lock_get(chip_name, plp_info, &pmd_lock)) , "bcm_plp_rx_pmd_lock_get");
    printf("PMD_LOCK :%d\n", pmd_lock);
    if_error((bcm_plp_mode_config_get(chip_name, plp_info, &speed, &if_type, &ref_clk, &if_mode, &device_aux_mode)), "config_get");

    (void)if_type;
    (void)ref_clk;
    (void)if_mode;
    printf("Speed:%d fec_mode:%d\n", speed, device_aux_mode.fec_mode_sel);
    if(pmd_lock && (speed == 100000) && (device_aux_mode.fec_mode_sel == bcmplpapertaRSFEC)) {
        for(index = 0; index < 5; index++) {
            if_error(bcm_plp_link_status_get(chip_name, plp_info, &link_status), "bcm_plp_link_status_get");
            if (link_status == 1) {
                printf("Link UP\n");
                return 0;
            }
        }
        if_error((bcm_plp_power_get(chip_name, plp_info, &power_rx_org, &power_tx_org)), "bcm_plp_power_get");
        if_error((bcm_plp_power_set(chip_name, plp_info,  3 /*PowerOffOn*/, power_tx_org)), "bcm_plp_power_set");
        if_error((bcm_plp_power_set(chip_name, plp_info, power_rx_org , power_tx_org)) , "bcm_plp_power_set");
    }

    return 0;
}

/*! \brief Update PLL0 port list
 *
 *  This API updates the pass through port that are configured with PLL0 to use PLL1.
 *
 *  @param chip_name        Represents chip name\n
 *  @param plp_info         Represents PHY access\n
 *  @param pll1_rate        Represents PLL1 rate\n
 *  @param pll0_port_list   Represents list of PT port lanemap that configured with PLL0\n
 *
 *  @return rv            SUCCESS (0)
 *                        OR Error code
 */
int plp_helper_update_pll0_port(char* chip_name,  bcm_plp_access_t plp_info,
                bcm_plp_aperta_pll1_vco_t pll1_rate, unsigned int pll0_port_list[8])
{
    int speed = 0, port = 0;
    int rv = 0, side = 0;
    bcm_plp_aperta_device_aux_modes_t aperta_aux_mode;

    memset(&aperta_aux_mode, 0, sizeof(bcm_plp_aperta_device_aux_modes_t));
    
    if (pll1_rate == bcmplpVco25p781G) {
        speed = 25000;
        aperta_aux_mode.lane_data_rate = bcmpLplaneDataRate_25P78125G;
        aperta_aux_mode.modulation_mode = bcmplpModulationNRZ;
        aperta_aux_mode.fec_mode_sel = bcmplpapertaNoFEC;
        aperta_aux_mode.port_type = bcmplpPortTypePassthrough;
    } else if (pll1_rate == bcmplpVco26p562G) {
        speed = 50000;
        aperta_aux_mode.lane_data_rate = bcmpLplaneDataRate_53P125G;
        aperta_aux_mode.modulation_mode = bcmplpModulationPAM4;
        aperta_aux_mode.fec_mode_sel = bcmplpapertaRS544;
        aperta_aux_mode.port_type = bcmplpPortTypePassthrough;
    } else {
        speed = 10000;
        aperta_aux_mode.lane_data_rate = bcmpLplaneDataRate_10P3125G;
        aperta_aux_mode.modulation_mode = bcmplpModulationNRZ;
        aperta_aux_mode.fec_mode_sel = bcmplpapertaNoFEC;
        aperta_aux_mode.port_type = bcmplpPortTypePassthrough;
    }
    for (port = 0; port < 8; port++) {
        if (pll0_port_list[port] != 0) {
            plp_info.lane_map = pll0_port_list[port];
            for (side = 1; side >=0; side --) { 
                plp_info.if_side = side;
                rv = bcm_plp_mode_config_set(chip_name, plp_info, speed, bcm_pm_InterfaceKR, bcm_pm_RefClk156Mhz, bcm_pm_Interface_mode_IEEE, &aperta_aux_mode);
                if (rv != 0) {
                    printf("Error in setting Config:%d\n",rv);
                    return rv;
                }
            }
        }
    }

    return 0;
}

/*! \brief Config SPIROM
 *
 *  This API allows the user to configure and enable SPIROM interface.
 *  This API needs to called only for DIE 0
 *
 *  @param chip_name        Represents chip name\n
 *  @param plp_info         Represents PHY access\n
 *  @param config_spi_rom   Represents the SPIROM configuration\n
 *
 *  @return rv            SUCCESS (0)
 *                        OR Error code
 */
int plp_helper_config_spirom(char* chip_name,  bcm_plp_access_t plp_info,
                              aperta_config_spi_rom_t config_spi_rom)
{

    unsigned int data = 5 /*Length*/, rv = 0;
    int addr = 0x101a000;
    unsigned int retry_cnt = 1000;

    if_error((bcm_plp_reg_value_set(chip_name, plp_info, 1, addr, data)), "bcm_plp_reg_value_set");
    data = (config_spi_rom.addr_mode << 8) | (config_spi_rom.spirom_en);
    addr ++;
    if_error((bcm_plp_reg_value_set(chip_name, plp_info, 1, addr, data)), "bcm_plp_reg_value_set");
    
    data = (config_spi_rom.write_delay << 8) | (config_spi_rom.read_sr_delay);
    addr ++;
    if_error((bcm_plp_reg_value_set(chip_name, plp_info, 1, addr, data)), "bcm_plp_reg_value_set");
    
    data = config_spi_rom.erase_delay;
    addr ++;
    if_error((bcm_plp_reg_value_set(chip_name, plp_info, 1, addr, data)), "bcm_plp_reg_value_set");
    
    data = (0x30 << 8) | (0 << 4) | (1<< 0);  /*Function - write - sts sent*/
    addr = 0x18222;
    if_error((bcm_plp_reg_value_set(chip_name, plp_info, 1, addr, data)), "bcm_plp_reg_value_set");

    /* Wait for the SUCCESS response from firmware*/
    do {
        if_error((bcm_plp_reg_value_get(chip_name, plp_info, 1, 0x18221, &data)), "bcm_plp_reg_value_get");
        if ((data & 0xF0) == 0xF0) {
            printf("Config SPIROM FW message failed:%x\n",data);
            return -1;
        }
        usleep(100);
    } while (((data & 0xFFF0) != ((0x30 << 8) | (0xE<< 4))) && (--retry_cnt));
    if (retry_cnt == 0) {
        printf("Config SPIROM FW message failed:%x\n", data);
        return -1;
    }
    
    printf("Config spirom success - 0x%x\n", data);
    return 0;
}

/*! \brief Erase SPIROM
 *
 *  This API allows the user to erase SPIROM.
 *
 *  @param chip_name        Represents chip name\n
 *  @param plp_info         Represents PHY access\n
 *
 *  @return rv            SUCCESS (0)
 *                        OR Error code
 */
int plp_helper_erase_spirom(char* chip_name,  bcm_plp_access_t plp_info)
{
    int rv = 0;
    unsigned int data = 0;
    unsigned int retry_cnt = 1000;

    /*Update length as 0*/
    if_error((bcm_plp_reg_value_set(chip_name, plp_info, 1, 0x1a000, 0)), "bcm_plp_reg_value_set");

    data = (0x31 << 8) | (0 << 4) | (1<< 0);  /*Function - write - sts sent*/
    if_error((bcm_plp_reg_value_set(chip_name, plp_info, 1, 0x18222, data)), "bcm_plp_reg_value_set");

    /* Wait for the SUCCESS response from firmware*/
    do {
        if_error((bcm_plp_reg_value_get(chip_name, plp_info, 1, 0x18221, &data)), "bcm_plp_reg_value_get");
        if ((data & 0xF0) == 0xF0) {
            printf("Erase SPIROM FW message failed:%x\n",data);
            return -1;
        }
        usleep(100);
    } while (((data & 0xFFF0) != ((0x31 << 8) | (0xE << 4))) && (--retry_cnt));
    
    if (retry_cnt == 0) {
        printf("Erase SPIROM FW message failed:%d\n",data);
        return -1;
    }
    printf("Erase SPIROM success - 0x%x\n", data);
    return 0;
}

/*! \brief Link Down event
 *
 *  User has to call this API when PCS link goes down.
 *
 *  @param chip_name        Represents chip name\n
 *  @param plp_info         Represents PHY access\n
 *  @param link_event       Represents the port config\n
 *
 *  @return rv            SUCCESS (0)
 *                        OR Error code
 */
int plp_helper_link_down_event(char* chip_name,  bcm_plp_access_t plp_info, aperta_link_event_t link_event)
{
    int rv = 0;
    unsigned int data = 0;
    bcm_plp_mac_access_t mac_access;
    memcpy(&mac_access.phy_info, &plp_info, sizeof(bcm_plp_access_t));
    
    if (link_event.pm_timesync_enabled) {
        if_error((bcm_plp_reg_value_get(chip_name, plp_info, 1, 0x1802c134, &data)), "bcm_plp_reg_value_get");
        /*
         * RX flag is for SFD and Deskew Enable in tscbh_gen1.
         */
        data |= (1 << 2);
        data &= ~(1);
        if_error((bcm_plp_reg_value_set(chip_name, plp_info, 1, 0x1802c134, data)), "bcm_plp_reg_value_set");
        /*
         * Set RX_PCS_ENABLE only for 400G 8 lanes case
         * to clear TS_UPDATE_EN.
         */
        if (link_event.port_data_rate == 400000) { /* Only for 400G*/
            if_error((bcm_plp_reg_value_get(chip_name, plp_info, 1, 0x1802c134, &data)), "bcm_plp_reg_value_get");
            data &= ~(2);
            if_error((bcm_plp_reg_value_set(chip_name, plp_info, 1, 0x1802c134, data)), "bcm_plp_reg_value_set");
        }
        if_error((bcm_plp_pm_timesync_enable_set(chip_name, mac_access, 0, 0)), "bcm_plp_pm_timesync_enable_set");
    }
    return 0;
}

/*! \brief Link up event
 *
 *  User has to call this API when PCS link comes up.
 *
 *  @param chip_name        Represents chip name\n
 *  @param plp_info         Represents PHY access\n
 *  @param link_event       Represents the port config\n
 *
 *  @return rv            SUCCESS (0)
 *                        OR Error code
 */
int plp_helper_link_up_event(char* chip_name,  bcm_plp_access_t plp_info, aperta_link_event_t link_event)
{
    int rv = 0;
    bcm_plp_mac_access_t mac_access;
    memcpy(&mac_access.phy_info, &plp_info, sizeof(bcm_plp_access_t));
    
    if (link_event.pm_timesync_enabled) {
        if_error((bcm_plp_pm_timesync_enable_set(chip_name, mac_access, link_event.flags, 1)), "bcm_plp_pm_timesync_enable_set");
    }

    return 0;
}

/*! \brief Disable incoming reset
 *
 *  This API disables the incoming reset from upstream die.
 *  This API need to be called before performing softreset to disable incoming reset
 *  from upstream die.
 *
 *  @param chip_name        Represents chip name\n
 *  @param plp_info         Represents PHY access\n
 *
 *  @return rv            SUCCESS (0)
 *                        OR Error code
 */
int plp_helper_disable_incoming_reset(char *chip_name, bcm_plp_access_t plp_info)
{
    int rv = 0;
    unsigned int data = 0;
    if_error((bcm_plp_reg_value_get(chip_name, plp_info, 1, 0x1018b21, &data)), "bcm_plp_reg_value_get");
    data &= ~0xc0;
    data |= 0xc0;
    if_error((bcm_plp_reg_value_set(chip_name, plp_info, 1, 0x1018b21, data)), "bcm_plp_reg_value_set");

    return 0;
}

/*! \brief Drop Tx data on fault
 *
 *  This API configures MAC to drop TX data on faults.
 *  This API needs to be called post configuring the port mode.
 *
 *  @param chip_name        Represents chip name\n
 *  @param plp_info         Represents PHY access\n
 *
 *  @return rv            SUCCESS (0)
 *                        OR Error code
 */
int plp_helper_drop_tx_data_on_fault(char *chip_name, bcm_plp_access_t plp_info)
{
    int rv = 0;
    unsigned int data = 0;
    if_error((bcm_plp_reg_value_get(chip_name, plp_info, 1, 0x14000113, &data)), "bcm_plp_reg_value_get");
    data &= ~0x30;
    data |= 0x30;
    if_error((bcm_plp_reg_value_set(chip_name, plp_info, 1, 0x14000113, data)), "bcm_plp_reg_value_set");
    return 0;
}

unsigned char plp_helper_aperta_log2n(unsigned int n)
{
    return ((n > 1) ? (1 + plp_helper_aperta_log2n(n / 2)) : 0);
}


/*! \brief SW based Port flush
 *
 *  This API allows the user to flush the port based on SW register sequence.
 *
 *  @param chip_name        Represents chip name\n
 *  @param sys_plp_info     Represents SYSTEM PHY access\n
 *  @param line_plp_info    Represents LINE PHY access\n
 *
 *  @return rv            SUCCESS (0)
 *                        OR Error code
 */
#define HELPER_FLUSH_DEBUG_PRINT 1
int plp_helper_sw_flush_seq(char* chip_name,  bcm_plp_access_t sys_plp_info, bcm_plp_access_t line_plp_info)
{
    int rv = 0;
    int port_sel = 0;
    unsigned int data = 0;
    printf("Line lane:0x%x sys lane:0x%x\n", line_plp_info.lane_map, sys_plp_info.lane_map); 
    port_sel = plp_helper_aperta_log2n(line_plp_info.lane_map & (-line_plp_info.lane_map));
    if (port_sel > 7) {
        printf("Invalid port_sel : %d\n", port_sel);
        return -1;
    }
    line_plp_info.if_side = APERTA_HELPER_LINE_SIDE; 
    sys_plp_info.if_side = APERTA_HELPER_SYSTEM_SIDE; 
#ifdef HELPER_FLUSH_DEBUG_PRINT
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x1400010b, &data)), "bcm_plp_reg_value_get");
    printf("%s_0x%x CDMAC_CTRL:0x%x\n", line_plp_info.if_side ? "SYS":"LINE", line_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x1500010d, &data)), "bcm_plp_reg_value_get");
    printf("%s_0x%x CDMAC_TX_CTRL:0x%x\n", line_plp_info.if_side ? "SYS":"LINE", line_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x14000113, &data)), "bcm_plp_reg_value_get");
    printf("%s_0x%x CDMAC__RX_LSS_CTRL:0x%x\n", line_plp_info.if_side ? "SYS":"LINE", line_plp_info.lane_map, data);

    if_error((bcm_plp_reg_value_get(chip_name, sys_plp_info, 1, 0x1400010b, &data)), "bcm_plp_reg_value_get");
    printf("%s_0x%x CDMAC_CTRL:0x%x\n", sys_plp_info.if_side ? "SYS":"LINE", sys_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, sys_plp_info, 1, 0x1500010d, &data)), "bcm_plp_reg_value_get");
    printf("%s_0x%x CDMAC_TX_CTRL:0x%x\n", sys_plp_info.if_side ? "SYS":"LINE", sys_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, sys_plp_info, 1, 0x14000113, &data)), "bcm_plp_reg_value_get");
    printf("%s_0x%x CDMAC__RX_LSS_CTRL:0x%x\n", sys_plp_info.if_side ? "SYS":"LINE", sys_plp_info.lane_map, data);
#endif

    /*1st part - Pause Port */
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x1400010b, &data)), "bcm_plp_reg_value_get");
    data &= ~2; 
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x1400010b, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, sys_plp_info, 1, 0x1400010b, &data)), "bcm_plp_reg_value_get");
    data &= ~2; 
    if_error((bcm_plp_reg_value_set(chip_name, sys_plp_info, 1, 0x1400010b, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x1500010d, &data)), "bcm_plp_reg_value_get");
    data &= ~4; 
    data |= 4; 
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x1500010d, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, sys_plp_info, 1, 0x1500010d, &data)), "bcm_plp_reg_value_get");
    data &= ~4; 
    data |= 4; 
    if_error((bcm_plp_reg_value_set(chip_name, sys_plp_info, 1, 0x1500010d, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x14000113, &data)), "bcm_plp_reg_value_get");
    data &= ~0x30;
    data |= 0x30;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x14000113, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get( chip_name,sys_plp_info, 1, 0x14000113, &data)), "bcm_plp_reg_value_get");
    data &= ~0x30;
    data |= 0x30;
    if_error((bcm_plp_reg_value_set( chip_name,sys_plp_info, 1, 0x14000113, data)), "bcm_plp_reg_value_set");

    usleep(20);
    /* 2nd part - Flush port : p2m*/
#ifdef HELPER_FLUSH_DEBUG_PRINT
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x4900d008 | (port_sel << 8) , &data)), "bcm_plp_reg_value_get");
    printf("0x%x EGR_PM2MACSEC_P2M_STATUS_0 :0x%x\n", line_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x4900C008 | (port_sel << 8) , &data)), "bcm_plp_reg_value_get");
    printf("0x%x ING_PM2MACSEC_P2M_STATUS_0 :0x%x\n", line_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x4900D009 | (port_sel << 8) , &data)), "bcm_plp_reg_value_get");
    printf("0x%x EGR_PM2MACSEC_P2M_STATUS2_0 :0x%x\n", line_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x4900C009 | (port_sel << 8) , &data)), "bcm_plp_reg_value_get");
    printf("0x%x ING_PM2MACSEC_P2M_STATUS2_0 :0x%x\n", line_plp_info.lane_map, data);
#endif

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49004001, &data)), "bcm_plp_reg_value_get");
    data &= ~(1 << port_sel);
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x49004001, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49003001, &data)), "bcm_plp_reg_value_get");
    data &= ~(1 << port_sel);
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x49003001, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49004001, &data)), "bcm_plp_reg_value_get");
    data |= (1 << port_sel);
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x49004001, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49003001, &data)), "bcm_plp_reg_value_get");
    data |= (1 << port_sel);
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x49003001, data)), "bcm_plp_reg_value_set");

#ifdef HELPER_FLUSH_DEBUG_PRINT
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x4900b003 | (port_sel << 8) , &data)), "bcm_plp_reg_value_get");
    printf("0x%x EGR_FC_FC_STATUS_CHX :0x%x\n", line_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x4900a003 | (port_sel << 8) , &data)), "bcm_plp_reg_value_get");
    printf("0x%x ING_FC_FC_STATUS_CHX :0x%x\n", line_plp_info.lane_map, data);
#endif

    usleep(20);
     /*3rd part  fc*/
    /*EIP164IF_EGR_PKTINFLGT_STATUS*/
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49004045, &data)), "bcm_plp_reg_value_get");
    if (data & (1 << port_sel)) {
        if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49004041, &data)), "bcm_plp_reg_value_get");
        data &= ~(1 << port_sel);
        if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x49004041, data)), "bcm_plp_reg_value_set");

        if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49004041, &data)), "bcm_plp_reg_value_get");
        data |= (1 << port_sel);
        if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x49004041, data)), "bcm_plp_reg_value_set");

        usleep(10);
        if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49004045, &data)), "bcm_plp_reg_value_get");
        if (data & (1 << port_sel)) {
            printf("EGR PKT INFLIGHT STATUS NOT CLEARED\n");
            rv |= 1;
        }
    }
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49003045, &data)), "bcm_plp_reg_value_get");
    if (data & (1 << port_sel)) {
        if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49003041, &data)), "bcm_plp_reg_value_get");
        data &= ~(1 << port_sel);
        if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x49003041, data)), "bcm_plp_reg_value_set");

        if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49003041, &data)), "bcm_plp_reg_value_get");
        data |= (1 << port_sel);
        if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x49003041, data)), "bcm_plp_reg_value_set");
        
	    usleep(10);
        if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49003045, &data)), "bcm_plp_reg_value_get");

        if (data & (1 << port_sel)) {
            printf("ING PKT INFLIGHT STATUS NOT CLEARED\n");
            rv |= 2;
        }
    }

    usleep(20);
    /*Part:4 FLUSH*/
#ifdef HELPER_FLUSH_DEBUG_PRINT
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49004045  , &data)), "bcm_plp_reg_value_get");
    printf("0x%x EIP164IF_EGR_PKTINFLGT_STATUS :0x%x\n", line_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49003045 , &data)), "bcm_plp_reg_value_get");
    printf("0x%x EIP164IF_ING_PKTINFLGT_STATUS :0x%x\n", line_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49009002 | (port_sel << 8) , &data)), "bcm_plp_reg_value_get");
    printf("0x%x EGR_SF_SF_ERR_INTR_STATUS_CHX :0x%x\n", line_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x4900a003 | (port_sel << 8) , &data)), "bcm_plp_reg_value_get");
    printf("0x%x ING_FC_FC_STATUS_CHX :0x%x\n", line_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x4900d027 | (port_sel << 8) , &data)), "bcm_plp_reg_value_get");
    printf("0x%x EGR_MACSEC2PM_M2P_STATUS_CHX :0x%x\n", line_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x4900C027 | (port_sel << 8) , &data)), "bcm_plp_reg_value_get");
    printf("0x%x ING_MACSEC2PM_M2P_STATUS_CHX :0x%x\n", line_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x4900d027 | (port_sel << 8) , &data)), "bcm_plp_reg_value_get");
    printf("0x%x EGR_MACSEC2PM_M2P_STATUS_CHX :0x%x\n", line_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x4900C027 | (port_sel << 8) , &data)), "bcm_plp_reg_value_get");
    printf("0x%x ING_MACSEC2PM_M2P_STATUS_CHX :0x%x\n", line_plp_info.lane_map, data);

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x1400011C, &data)), "bcm_plp_reg_value_get");
    printf("%s_0x%x CDMAC_TXFIFO_CELL_CNT:0x%x\n", line_plp_info.if_side ? "SYS":"LINE", line_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x1400011d, &data)), "bcm_plp_reg_value_get");
    printf("%s_0x%x CDMAC_TXFIFO_CELL_REQ_CNT:0x%x\n", line_plp_info.if_side ? "SYS":"LINE", line_plp_info.lane_map, data);

    if_error((bcm_plp_reg_value_get(chip_name, sys_plp_info, 1, 0x1400011C, &data)), "bcm_plp_reg_value_get");
    printf("%s_0x%x CDMAC_TXFIFO_CELL_CNT:0x%x\n", sys_plp_info.if_side ? "SYS":"LINE", sys_plp_info.lane_map, data);
    if_error((bcm_plp_reg_value_get(chip_name, sys_plp_info, 1, 0x1400011d, &data)), "bcm_plp_reg_value_get");
    printf("%s_0x%x CDMAC_TXFIFO_CELL_REQ_CNT:0x%x\n", sys_plp_info.if_side ? "SYS":"LINE", sys_plp_info.lane_map, data);

#endif

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, (0x4900C021 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    data &= ~1;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, (0x4900C021 | (port_sel << 8)), data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, (0x4900d021 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    data &= ~1;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, (0x4900d021 | (port_sel << 8)), data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x1400010b, &data)), "bcm_plp_reg_value_get");
    data &= ~1;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x1400010b, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, sys_plp_info, 1, 0x1400010b, &data)), "bcm_plp_reg_value_get");
    data &= ~1;
    if_error((bcm_plp_reg_value_set(chip_name, sys_plp_info, 1, 0x1400010b, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x1400010b, &data)), "bcm_plp_reg_value_get");
    data &= ~0x40;
    data |= 0x40;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x1400010b, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, sys_plp_info, 1, 0x1400010b, &data)), "bcm_plp_reg_value_set");
    data &= ~0x40;
    data |= 0x40;
    if_error((bcm_plp_reg_value_set(chip_name, sys_plp_info, 1, 0x1400010b, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x1400010b, &data)), "bcm_plp_reg_value_get");
    data |= 1;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x1400010b, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, sys_plp_info, 1, 0x1400010b, &data)), "bcm_plp_reg_value_get");
    data |= 1;
    if_error((bcm_plp_reg_value_set(chip_name, sys_plp_info, 1, 0x1400010b, data)), "bcm_plp_reg_value_set");

    /*ING M2P reset*/
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, (0x4900C021 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    data |= 1;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, (0x4900C021 | (port_sel << 8)), data)), "bcm_plp_reg_value_set");

    /* EGR M2P reset*/
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, (0x4900d021 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    data |= 1;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, (0x4900d021 | (port_sel << 8)), data)), "bcm_plp_reg_value_set");

#if 0
    /*ING P2M reset*/
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, (0x4900C001 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    data &= ~1;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, (0x4900C001 | (port_sel << 8)), data)), "bcm_plp_reg_value_set");

    /* EGR P2M reset*/
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, (0x4900d001 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    data &= ~1;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, (0x4900d001 | (port_sel << 8)), data)), "bcm_plp_reg_value_set");

    /* ING P2M out of reset*/
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, (0x4900C001 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    data |= 1;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, (0x4900C001 | (port_sel << 8)), data)), "bcm_plp_reg_value_set");

    /* EGR P2M out of reset*/
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, (0x4900d001 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    data |= 1;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, (0x4900d001 | (port_sel << 8)), data)), "bcm_plp_reg_value_set");
#endif

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, (0x4900A002 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    data &= ~2;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, (0x4900A002 | (port_sel << 8)), data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, (0x4900B002 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    data &= ~2;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, (0x4900B002 | (port_sel << 8)), data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, (0x49009004 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    data &= ~2;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, (0x49009004 | (port_sel << 8)), data)), "bcm_plp_reg_value_set");

    if (port_sel < 4) { /*0-3 Ingress SF enable */
        if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x4900a800, &data)), "bcm_plp_reg_value_get");
        data &= ~(4 << (port_sel*4));
        data |= (4 << (port_sel*4));
        if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x4900a800, data)), "bcm_plp_reg_value_set");
    } else { /*4-7 SF Ingress enable*/
        if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x4900a801, &data)), "bcm_plp_reg_value_get");
        data &= ~(4 << ((port_sel-4)*4));
        data |= (4 << ((port_sel-4)*4));
        if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x4900a801, data)), "bcm_plp_reg_value_set");
    }
    /*EGress SF enable*/
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x49009800, &data)), "bcm_plp_reg_value_get");
    data &= ~(2 << ((port_sel)*2));
    data |= (2 << ((port_sel)*2));
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x49009800, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, (0x4900A002 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    data |= 2;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, (0x4900A002 | (port_sel << 8)), data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, (0x4900B002 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    data |= 2;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, (0x4900B002 | (port_sel << 8)), data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, (0x49009004 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    data |= 2;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, (0x49009004 | (port_sel << 8)), data)), "bcm_plp_reg_value_set");

    
    /* Resume Port*/
    usleep(20);
#if 0
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x14000113, &data)), "bcm_plp_reg_value_get");
    data &= ~0x30;
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x14000113, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get( chip_name,sys_plp_info, 1, 0x14000113, &data)), "bcm_plp_reg_value_get");
    data &= ~0x30;
    if_error((bcm_plp_reg_value_set( chip_name,sys_plp_info, 1, 0x14000113, data)), "bcm_plp_reg_value_set");
#endif
    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x1500010d, &data)), "bcm_plp_reg_value_get");
    data &= ~4; 
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x1500010d, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, sys_plp_info, 1, 0x1500010d, &data)), "bcm_plp_reg_value_get");
    data &= ~4; 
    if_error((bcm_plp_reg_value_set(chip_name, sys_plp_info, 1, 0x1500010d, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x1400010b, &data)), "bcm_plp_reg_value_get");
    data &= ~0x40; 
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x1400010b, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, sys_plp_info, 1, 0x1400010b, &data)), "bcm_plp_reg_value_get");
    data &= ~0x40; 
    if_error((bcm_plp_reg_value_set(chip_name, sys_plp_info, 1, 0x1400010b, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, line_plp_info, 1, 0x1400010b, &data)), "bcm_plp_reg_value_get");
    data &= ~2; 
    data |= 2; 
    if_error((bcm_plp_reg_value_set(chip_name, line_plp_info, 1, 0x1400010b, data)), "bcm_plp_reg_value_set");

    if_error((bcm_plp_reg_value_get(chip_name, sys_plp_info, 1, 0x1400010b, &data)), "bcm_plp_reg_value_get");
    data &= ~2; 
    data |= 2; 
    if_error((bcm_plp_reg_value_set(chip_name, sys_plp_info, 1, 0x1400010b, data)), "bcm_plp_reg_value_set");

    
    return rv;
}

/*! \brief SW based Port flush
 *
 *  This API allows the user to flush the port based on SW register sequence.
 *
 *  @param chip_name        Represents chip name\n
 *  @param plp_info         Represents PHY access\n
 *
 *  @return rv            SUCCESS (0)
 *                        OR Error code
 */
int plp_helper_check_datapath(char* chip_name,  bcm_plp_access_t plp_info)
{
    int rv = 0;
    unsigned int data = 0;
    int port_sel = 0;

    port_sel = plp_helper_aperta_log2n(plp_info.lane_map & (-plp_info.lane_map));
    if (port_sel > 7) {
        printf("Invalid port_sel : %d\n", port_sel);
        return -1;
    }
    /* Ingress:*/
    plp_info.if_side = APERTA_HELPER_LINE_SIDE; 
    if_error_no_return((bcm_plp_reg_value_get(chip_name, plp_info, 1, (0x4900a003 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    if (data & 0x80) {
        printf("Ingress packet dropped for LM:0x%x\n", plp_info.lane_map);
        rv = 1;
    }
    if_error_no_return((bcm_plp_reg_value_get(chip_name, plp_info, 1, (0x4900c027 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    if (data & 0x4) {
        printf("Ingress infifo_empty_during_mop Set for LM:0x%x\n", plp_info.lane_map);
        rv |=2;
    }
    if_error_no_return((bcm_plp_reg_value_get(chip_name, plp_info, 1, (0x1400011A), &data)), "bcm_plp_reg_value_get");
    if (data & 0x3) {
        printf("Line side TX pkt %s set\n", (data & 1) ? "Underflow" : "Overflow");
        rv |=4;
    }
    if_error_no_return((bcm_plp_reg_value_get(chip_name, plp_info, 1, (0x4900b003 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    if (data & 0x80) {
        printf("Egress packet dropped for LM:0x%x\n", plp_info.lane_map);
        rv |=8;
    }
    if_error_no_return((bcm_plp_reg_value_get(chip_name, plp_info, 1, (0x4900d027 | (port_sel << 8)), &data)), "bcm_plp_reg_value_get");
    if (data & 0x4) {
        printf("Egress infifo_empty_during_mop Set for LM:0x%x\n", plp_info.lane_map);
        rv |=0x10;
    }
    plp_info.if_side = APERTA_HELPER_SYSTEM_SIDE; 
    if_error_no_return((bcm_plp_reg_value_get(chip_name, plp_info, 1, (0x1400011A), &data)), "bcm_plp_reg_value_get");
    if (data & 0x3) {
        printf("System side TX pkt %s set\n", (data & 1) ? "Underflow" : "Overflow");
        rv |=0x20;
    }

    return rv;
}


