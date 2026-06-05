/*
 * $Copyright: (c) 2021 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

/*
 * The app is tested with System Side connected to a Broadcom Switch,
 * that generates Egress traffic on BCM81343 PHY-0 and with Line Side
 * of BCM81343 PHY-0 connected to Line Side of BCM81343 PHY-1
 * BCM81343 PHY-1 is configured for Ingress and its System Side is
 * connected to the same Broadcom Switch.
 *
 * This sample program is only a reference to demonstrate the usage of
 * bcm_pm_if APIs. This program might not work for all environments.
 *
 * Note: INITIALIZE/MEMSET EVERY FUNCTION PARAMETERS TO "0",
 *       TO AVOID FUTURE COMPATIBILITY ISSUES WITH THE DRIVER.
 */

/* Includes */
#include <aperta_common.h>

/* -------------------------------------------------------------------------------- *
 *                         Aperta reference application                             *
 * -------------------------------------------------------------------------------- *
 * This sample application configures the Aperta chip for 400G PAM4 (8x53G)         *
 * macsec transform mode as follows:                                                *
 *   1. Connect to the board                                                        *
 *   2. Initialize the PHY and downloads the firmware                               *
 *   3. Initialize MACsec in bypass disable mode                                    *
 *   4. Configure Tx/Rx polarity                                                    *
 *   5. Configure operating mode parameters for 400 PAM4 (8x53G) mode               *
 *   6. Setup link training or configure TX FIR settings                            *
 *   7. Configure MACsec cleartag functionality using Secy/Cfye.                    *
 *      - This application can be used to validate VLAN QinQ packet type.           *
 *      - Configure correct rules for Destination MAC, VLAN in function             *
 *        populate_tcam_rules().                                                    *
 *        The Destination MAC, VLAN used in the reference application is based on   *
 *        an example packet and these values need to be changed with the actual     *
 *        packet values.                                                            *
 *   8. Cleanup and close the connection to the board if uninitialization path is   *
 *       enabled.                                                                   *
 * -------------------------------------------------------------------------------- *
 * Note 1 : This sample application is only for 81343 parts.                        *
 * Note 2 : By default control packet policy bypass is enabled in Makefile.         *
 *          To enable data packet policy bypass user needs to disable the flag      *
 *          BCM_PLP_CONTROL_PACKET_BYPASS in Makefile.                              *
 * -------------------------------------------------------------------------------- */

/* 400G PAM4 (8x53G) MACsec macsec mode*/
#define OP_MODE_STR    "400G PAM4 (8x53G) MACsec mode"

/* Enable Tx forced training */
#define LINK_TRAINING_EN

/* Enable MACsec un-initialization */
#define MACSEC_UNINIT_EN

/* Disable any bypass set in the Makefile */
#undef BCM_PLP_CONTROL_PACKET_BYPASS
#undef BCM_PLP_DATA_PACKET_BYPASS

#define TRAINED                 1
#define UNTRAINED               0
#define TRAINING_COMPLETE       0
#define TRAINING_FAILURE        1

extern bcm_plp_cfye_vport_handle_t vport_handle[2][8][2];
extern bcm_plp_cfye_rule_handle_t  rule_handle[2][8][2];
extern bcm_plp_secy_sa_handle_t    secy_sahandle[2][8][2];
extern unsigned int vport_ids[2][8][2];
extern unsigned char sci[MAX_VPORT][8];

extern unsigned int ingress_transform_record[MAX_SA][24];
extern unsigned int egress_transform_record[MAX_SA][24];
extern bcm_plp_secy_sa_t sa_params;

/* System side parameters */
#define SYS_PORT_SPEED          400000
#define SYS_LANE_RATE           bcmpLplaneDataRate_53P125G
#define SYS_MODULATION          bcmplpModulationPAM4
#define SYS_FEC_MODE            bcmplpapertaRS544_2XN
#define SYS_IF_TYPE             bcm_pm_InterfaceAUI_C2C
#define SYS_IF_MODE             bcm_pm_Interface_mode_IEEE
#define SYS_PORT_TYPE           bcmplpPortTypePassthrough

unsigned int sys_lane_map_list[] =  {
                                      0xFF
                                    };

/* Line side parameters */
#define LINE_PORT_SPEED         400000
#define LINE_LANE_RATE          bcmpLplaneDataRate_53P125G
#define LINE_MODULATION         bcmplpModulationPAM4
#define LINE_FEC_MODE           bcmplpapertaRS544_2XN
#define LINE_IF_TYPE            bcm_pm_InterfaceAUI_C2M
#define LINE_IF_MODE            bcm_pm_Interface_mode_IEEE
#define LINE_PORT_TYPE          bcmplpPortTypePassthrough

unsigned int line_lane_map_list[] = {
                                      0xFF
                                    };

#define NUM_OF_PKTS               1
#define PKT_TYPE_VLAN_QINQ        0x1
#define MAX_NUM_OF_PHY            1024

const char *pkt_type_name[] = {"INVALID", "VLAN QinQ"};

/*
 * This function configures the TCAM rules for egress or ingress packets.
 * macsec_side: 0 for Egress and 1 for Ingress.
 * ruleparams: bcm_plp_cfye_rule_t structure to configure TCAM rules.
 * pkt_type: Selects the types of packet to configure the rule.
 */
int populate_tcam_rules(int macsec_side, bcm_plp_cfye_rule_t *ruleparams, int pkt_type)
{
    int rv = 0;

    if (!ruleparams)
        return -1;

    memset(ruleparams, 0, sizeof(bcm_plp_cfye_rule_t));

    /* User can configure different rules for Egress or Ingress side. */
    switch(pkt_type) {
        case PKT_TYPE_VLAN_QINQ:
            ruleparams->mask.packet_type = 0x3; /* exact match on all these fields. */
            ruleparams->mask.num_tags = 0x7F;
            ruleparams->key.packet_type = BCM_PLP_CFYE_RULE_PKT_TYPE_OTHER;
            ruleparams->data_mask[0] = 0xffffffff; /* match on destination address. */
            ruleparams->data_mask[1] = 0x0000ffff;
            ruleparams->data_mask[2] = 0xffffffff;
            if (macsec_side == EGRESS) {
                ruleparams->key.num_tags = 4;
                /* Set the Dest MAC: 00:00:00:09:01:01*/
                ruleparams->data[0] = 0x9000000;
                ruleparams->data[1] = 0x101;
                /* Set VLAN Tags (0x01A1 and 0x02B2) and Ether Type 0x0800 */
                ruleparams->data[2] = (0xA1 << 24) | (01 << 16) | 0x0008;
                ruleparams->data[3] = (0xB2 << 16) | (0x02 << 8);
                ruleparams->data_mask[3] = 0x00ffff00;
            } else {
                ruleparams->key.num_tags = 2;
                /* Set the Dest MAC: 00:00:00:09:01:01 */
                ruleparams->data[0] = 0x9000000;
                ruleparams->data[1] = 0x101;
                /* Set VLAN Tags (0x01A1) and MACsec tag value (Ether Type: 0x88E5 */
                ruleparams->data[2] = (0xA1 << 24) | (01 << 16) | 0xE588;
                ruleparams->data_mask[3] = 0x00000000;
            }
            break;
        default:
            printf("%s(): Invalid pkt_type, macsec_side[%d], pkt_type[0x%x] \n",
                  __func__, macsec_side, pkt_type);
            rv = -1;
            break;
    }
    return rv;
}

/* Install parsers settings for SecTAG, VLANs */
int macsec_install_parser_settings(bcm_plp_sec_phy_access_t sec_info, int pkt_type)
{
    int retval = 0;
    bcm_plp_cfye_device_t DeviceParams;
    bcm_plp_cfye_header_parser_t HeaderParams;
    bcm_plp_cfye_sectag_parser_t SecTAGParams;
    bcm_plp_cfye_vlan_parser_t VLANTagParams;

    memset(&DeviceParams, 0, sizeof(bcm_plp_cfye_device_t));
    memset(&HeaderParams, 0, sizeof(bcm_plp_cfye_header_parser_t));
    memset(&SecTAGParams, 0, sizeof(bcm_plp_cfye_sectag_parser_t));
    memset(&VLANTagParams, 0, sizeof(bcm_plp_cfye_vlan_parser_t));

    SecTAGParams.fcheck_version = 1;
    SecTAGParams.fcheck_kay = 1;
    SecTAGParams.fcomp_type = 1;
    SecTAGParams.mac_sec_tag_value = 0x88E5;

    switch(pkt_type) {
        case PKT_TYPE_VLAN_QINQ:
            VLANTagParams.cp.fparse_qinq = 1;
            VLANTagParams.cp.fparse_qtag = 1;
            VLANTagParams.cp.fparse_stag1 = 1;
            VLANTagParams.fstag_up_enable = 1;
            VLANTagParams.fqtag_up_enable = 1;
            VLANTagParams.default_up = 0;
            VLANTagParams.uptable1[0] = VLANTagParams.uptable2[0] = 0;
            VLANTagParams.uptable1[1] = VLANTagParams.uptable2[1] = 1;
            VLANTagParams.uptable1[2] = VLANTagParams.uptable2[2] = 2;
            VLANTagParams.uptable1[3] = VLANTagParams.uptable2[3] = 3;
            VLANTagParams.uptable1[4] = VLANTagParams.uptable2[4] = 4;
            VLANTagParams.uptable1[5] = VLANTagParams.uptable2[5] = 5;
            VLANTagParams.uptable1[6] = VLANTagParams.uptable2[6] = 6;
            VLANTagParams.uptable1[7] = VLANTagParams.uptable2[7] = 7;
            VLANTagParams.qtag = 0x8100;
            VLANTagParams.stag1 = 0x9200;
            HeaderParams.vlan_parser_p = &VLANTagParams;
            break;
        default:
            printf("Invalid pkt_type for PHY-ID[%d],  macsec_side[%d], LANE_MAP[0x%x], pkt_type[0x%x] \n",
                sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, pkt_type);
            return retval;
    }
    HeaderParams.sectag_parser_p = &SecTAGParams;
    DeviceParams.header_parser_p = &HeaderParams;

    retval = bcm_plp_cfye_device_update(CHIP_NAME, &sec_info, &DeviceParams);
    if(retval) {
        printf("bcm_plp_cfye_device_update API FAILED for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code =[%d] \n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        return retval;
    }
    printf("bcm_plp_cfye_device_update API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code =[%d] \n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
    return retval;
}

/* Install SA with transform record with */
int macsec_install_sa_with_ct_params(bcm_plp_sec_phy_access_t sec_info, macsec_policy_bypass_option_t policy, int pkt_type)
{
    int retval = 0;
    int phy_index = sec_info.phy_info.phy_addr;
    int macsec_side = sec_info.macsec_side;
    int port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

    retval = macsec_build_tranform_record(sec_info, port, port);
    if(retval) {
        printf("FAIL: build_tranform_record failed for phy_index[%d], macsec_side[%d], return code =[%d] \n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, retval);
        return retval;
    }

    memset(&sa_params, 0, sizeof(bcm_plp_secy_sa_t));
    switch(sec_info.macsec_side) {
        case INGRESS :
            sa_params.action_type = BCM_PLP_SECY_SA_ACTION_INGRESS;
            sa_params.drop_type   = BCM_PLP_SECY_SA_DROP_INTERNAL;
            sa_params.dest_port   = BCM_PLP_SECY_PORT_CONTROLLED;
            sa_params.params.ingress.validate_frames_tagged = BCM_PLP_SECY_FRAME_VALIDATE_STRICT;
            sa_params.params.ingress.fsa_inuse = 1;
            sa_params.params.ingress.freplay_protect = 1;
            sa_params.params.ingress.sci_p = (unsigned char *)discard_const(&sci[port]);
            sa_params.params.ingress.an = 0; /* This should match CCW in Transform record of Egress */
            sa_params.params.ingress.fallow_tagged = 1;
            sa_params.params.ingress.fvalidate_untagged = 0;

            if((policy == POLICY_BYPASS_CONTROL_PACKET) || (policy == POLICY_BYPASS_NONE)) {
                sa_params.params.ingress.fallow_untagged = 0;
            } else /* (policy == POLICY_BYPASS_DATA_PACKET) */ {
                sa_params.params.ingress.fallow_untagged = 1;
            }
            /* size of the transform record in 32-bit words */
            sa_params.sa_word_count = TRANSREC_INGRESS_SIZE;
            sa_params.transform_record_p =   &ingress_transform_record[port][0];
        break;

        case EGRESS :
            sa_params.action_type = BCM_PLP_SECY_SA_ACTION_EGRESS;
            sa_params.drop_type   = BCM_PLP_SECY_SA_DROP_INTERNAL;
            sa_params.dest_port   = BCM_PLP_SECY_PORT_CONTROLLED;
            sa_params.params.egress.fsa_inuse        = 1;
            sa_params.params.egress.finclude_sci     = 1;
            sa_params.params.egress.fallow_data_pkts = 1;

            if((policy == POLICY_BYPASS_CONTROL_PACKET) || (policy == POLICY_BYPASS_NONE)) {
                sa_params.params.egress.fprotect_frames  = 1;
                sa_params.params.egress.fconf_protect    = 1;
            } else { /* POLICY_BYPASS_DATA_PACKET */
                sa_params.params.egress.fprotect_frames  = 0;
                sa_params.params.egress.fconf_protect    = 0;
            }
            /* size of the transform record in 32-bit words */
            sa_params.sa_word_count = TRANSREC_EGRESS_SIZE;
            sa_params.transform_record_p =   &egress_transform_record[port][0];
        break;

        default :
            break ;
    }
    retval = bcm_plp_secy_sa_add(CHIP_NAME, &sec_info,
                                  vport_ids[phy_index][port][macsec_side],
                                  &secy_sahandle[phy_index][port][macsec_side],
                                  &sa_params
                                );
    if(retval) {
        printf("FAIL: bcm_plp_secy_sa_add API failed for phy_index[%d], macsec_side[%d], return code =[%d] \n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, retval);
        return retval;
    } else {
        printf("PASS: bcm_plp_secy_sa_add API successful for phy_index[%d], macsec_side[%d], return code =[%d] \n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, retval);
    }
    return retval;
}

/* Add rule policy */
int macsec_add_ct_rule_policy(bcm_plp_sec_phy_access_t sec_info, int pkt_type)
{
    bcm_plp_cfye_rule_t ruleparams;
    int retval = 0;
    int phy_index = sec_info.phy_info.phy_addr;
    int macsec_side = sec_info.macsec_side;
    int port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

    memset(&ruleparams, 0, sizeof(bcm_plp_cfye_rule_t));

    if (populate_tcam_rules(macsec_side, &ruleparams, pkt_type) < 0) {
        printf("FAIL: populate_tcam_rules failed for phy_index[%d], macsec_side[%d], return code =[%d] \n",
            sec_info.phy_info.phy_addr, sec_info.macsec_side, retval);
        return retval;
    }
    ruleparams.policy.vport_handle = vport_handle[phy_index][port][macsec_side];

    retval = bcm_plp_cfye_rule_add( CHIP_NAME, &sec_info,
                                    vport_handle[phy_index][port][macsec_side],
                                    &rule_handle[phy_index][port][macsec_side],
                                    &ruleparams
                                  );
    if(retval) {
        printf("FAIL: bcm_plp_cfye_rule_add API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        return retval;
    } else {
        printf("PASS : bcm_plp_cfye_rule_add API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
    }
    return retval ;
}

/* Main entry to application */
int main(int argc, char *argv[])
{
    int retval = TEST_SUCCESS;
    int p_ctxt = 5;
    int phy_index = 0;
    int side = 0;
    int macsec_side = 0;
    int lane_map_index;
    int total_lane_maps;
    int device_bypass_enable = 0;
    int port = 0;
    int mtusize_bytes = 0;
    bcm_plp_cfye_vport_t vportparams;
    int sf_enable_get = 0;
    int sf_enable_set;

    unsigned int link_status_get;
    unsigned int sys_lane_map;
    unsigned int line_lane_map;
    unsigned int fw_ver;
    unsigned int fw_crc;

    int pkt_type = PKT_TYPE_VLAN_QINQ;

    bcm_plp_sec_phy_access_t sec_info;
    bcm_plp_mac_access_t mac_info;
    bcm_plp_access_t phy_info;
    bcm_plp_firmware_load_type_t firmware_load_type;
    bcm_plp_aperta_fw_init_params_t aperta_init_params;
    bcm_plp_mac_flow_control_t flow_option, flow_option_get;
    bcm_laneswap_map_t sys_laneswap_map, line_laneswap_map;
    bcm_plp_aperta_device_aux_modes_t aux_mode_sys_set, aux_mode_line_set;
    bcm_plp_cfye_device_exceptions_t exception;
    bcm_plp_cfye_device_control_t dev_control;
    bcm_plp_cfye_device_t dev_init;


#ifdef LINK_TRAINING_EN
    unsigned int training_ena_dis = DISABLE;
    unsigned int training_failure = TRAINING_FAILURE;
    unsigned int training_status  = UNTRAINED;
#endif

    /* Initialize structure instances to zero */
    memset(&sec_info, 0, sizeof(bcm_plp_sec_phy_access_t));
    memset(&mac_info, 0, sizeof(bcm_plp_mac_access_t));
    memset(&phy_info, 0, sizeof(bcm_plp_access_t));
    memset(&firmware_load_type, 0, sizeof(bcm_plp_firmware_load_type_t));
    memset(&aperta_init_params,0, sizeof(bcm_plp_aperta_fw_init_params_t));
    memset(&sys_laneswap_map,  0, sizeof(bcm_laneswap_map_t));
    memset(&line_laneswap_map, 0, sizeof(bcm_laneswap_map_t));
    memset(&aux_mode_sys_set,  0, sizeof(bcm_plp_aperta_device_aux_modes_t));
    memset(&aux_mode_line_set, 0, sizeof(bcm_plp_aperta_device_aux_modes_t));
    memset(&exception, 0, sizeof(bcm_plp_cfye_device_exceptions_t));
    memset(&dev_control, 0, sizeof(bcm_plp_cfye_device_control_t));
    memset(&dev_init, 0, sizeof(bcm_plp_cfye_device_t));

    phy_info.platform_ctxt = &p_ctxt;

    /* -------------------------------------------------------------------------------- *
     * Connect to the board                                                             *
     * -------------------------------------------------------------------------------- */
    retval = device_open();
    if (retval) {
        printf("FAIL: Failed to connect to the board (ret = %d)!\n", retval);
        return retval;
    }

    /* -------------------------------------------------------------------------------- *
     * Section 1: PHY, MACsec Initialization and MACsec Configuration                   *
     * -------------------------------------------------------------------------------- */

    /* -------------------------------------------------------------------------------- *
     * Initialize the PHYs and download firmware based on the test setup                *
     * -------------------------------------------------------------------------------- */
    aperta_init_params.pll1_vco_rate = bcmplpVco26p562G;
    firmware_load_type.firmware_load_method = bcmpmFirmwareLoadMethodInternal;
    firmware_load_type.force_load_method = bcmpmFirmwareLoadForce;
    firmware_load_type.fw_init_params = &aperta_init_params;

    phy_info.if_side = 0;
    phy_info.lane_map = ALL_LANE_MAP;
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;

        printf("Initializing PHY-%d with loading internal firmware...\n", phy_info.phy_addr);
        retval = bcm_plp_init_fw_bcast(CHIP_NAME, phy_info, mdio_read, mdio_write, &firmware_load_type, bcmpmFirmwareBroadcastNone);
        if (retval) {
            printf("FAIL: bcm_plp_init_fw_bcast failed to initialize PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _aperta_init_error;
        }

        /* Read firmware info */
        retval = bcm_plp_firmware_info_get(CHIP_NAME, phy_info, &fw_ver, &fw_crc);
        if (retval) {
            printf("FAIL: Failed to get the firmware info for PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _aperta_init_error;
        } else {
            printf("Firmware info for PHY-%d: FW version 0x%x, FW CRC 0x%x\n", phy_info.phy_addr, fw_ver, fw_crc);
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Initialize MACsec CfyE and SecY                                                  *
     * -------------------------------------------------------------------------------- */
    phy_info.lane_map = ALL_LANE_MAP;
    device_bypass_enable = 0;
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
        for (macsec_side = 0; macsec_side < 2; macsec_side++) {
            sec_info.macsec_side = macsec_side; /* macsec_side = 0 -> Egress ; macsec_side = 1 -> Ingress */
            /* device_bypass_enable is set to 0, to trigger macsec transformation & validation */
            retval = macsec_initialize(sec_info, device_bypass_enable);
            if (retval) {
                printf("FAIL: MACSec Initialize for PHY-ID[%d], macsec_side[%d], return code [%d]\n",
                phy_info.phy_addr, sec_info.macsec_side, retval);
                goto _aperta_macsec_init_error;
            } else {
                printf("PASS: MACSec Initialize for PHY-ID[%d], macsec_side [%d] \n",
                phy_info.phy_addr, sec_info.macsec_side);
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Section 2: HW Configuration for Polarity and Laneswap if applicable              *
     * -------------------------------------------------------------------------------- */

    /* -------------------------------------------------------------------------------- *
     * Applying polarity configurations                                                 *
     * -------------------------------------------------------------------------------- */
    retval = polarity_swap();
    if (retval) {
        printf("FAIL: Failed to get and set initial Tx/Rx polarity with ret = %d\n", retval);
        goto _aperta_config_error;
    }

    /* -------------------------------------------------------------------------------- *
     * Applying LANE TX/RX Mapping                                                      *
     * LANE Mapping can be skipped if there is no lane swap                             *
     * -------------------------------------------------------------------------------- */
    sys_laneswap_map.num_of_lanes = 8;
    /* System side lane swap can be set differently if there are swaps */
    sys_laneswap_map.lane_map_rx[0] = sys_laneswap_map.lane_map_tx[0] = 0;
    sys_laneswap_map.lane_map_rx[1] = sys_laneswap_map.lane_map_tx[1] = 1;
    sys_laneswap_map.lane_map_rx[2] = sys_laneswap_map.lane_map_tx[2] = 2;
    sys_laneswap_map.lane_map_rx[3] = sys_laneswap_map.lane_map_tx[3] = 3;
    sys_laneswap_map.lane_map_rx[4] = sys_laneswap_map.lane_map_tx[4] = 4;
    sys_laneswap_map.lane_map_rx[5] = sys_laneswap_map.lane_map_tx[5] = 5;
    sys_laneswap_map.lane_map_rx[6] = sys_laneswap_map.lane_map_tx[6] = 6;
    sys_laneswap_map.lane_map_rx[7] = sys_laneswap_map.lane_map_tx[7] = 7;

    line_laneswap_map.num_of_lanes = 8;
    /* Line side lane swap can be set differently if there are swaps */
    line_laneswap_map.lane_map_rx[0] = line_laneswap_map.lane_map_tx[0] = 0;
    line_laneswap_map.lane_map_rx[1] = line_laneswap_map.lane_map_tx[1] = 1;
    line_laneswap_map.lane_map_rx[2] = line_laneswap_map.lane_map_tx[2] = 2;
    line_laneswap_map.lane_map_rx[3] = line_laneswap_map.lane_map_tx[3] = 3;
    line_laneswap_map.lane_map_rx[4] = line_laneswap_map.lane_map_tx[4] = 4;
    line_laneswap_map.lane_map_rx[5] = line_laneswap_map.lane_map_tx[5] = 5;
    line_laneswap_map.lane_map_rx[6] = line_laneswap_map.lane_map_tx[6] = 6;
    line_laneswap_map.lane_map_rx[7] = line_laneswap_map.lane_map_tx[7] = 7;

    phy_info.lane_map = ALL_LANE_MAP;
    for (phy_index = 0; phy_index < NUM_OF_PHY ; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;

        phy_info.if_side = SYS_SIDE;
        retval = bcm_plp_rxtx_laneswap_set(CHIP_NAME, phy_info, &sys_laneswap_map);
        if (retval) {
            printf("FAIL: Failed to set lane swap for PHY-%d at System side (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully configured the lane swap for PHY-%d at System side!\n", phy_info.phy_addr);
        }

        phy_info.if_side = LINE_SIDE;
        retval = bcm_plp_rxtx_laneswap_set(CHIP_NAME, phy_info, &line_laneswap_map);
        if (retval) {
            printf("FAIL: Failed to set lane swap for PHY-%d at Line side (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully configured the lane swap for PHY-%d at Line side!\n", phy_info.phy_addr);
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Section 3: Mode configuration                                                    *
     * -------------------------------------------------------------------------------- */
   total_lane_maps = NUM_ARR_ELEMENTS(sys_lane_map_list);

    /* Config SecY set for device*/
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        phy_info.if_side  = LINE_SIDE;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            phy_info.lane_map = line_lane_map ;
            memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side; /* macsec_side = 0 -> Egress ; macsec_side = 1 -> Ingress */
                retval = bcm_plp_secy_config_set(CHIP_NAME, &sec_info);
                if (retval) {
                    printf("FAIL: Failed to perform secy config set for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                    goto _aperta_config_error;
                } else {
                    printf("PASS: Successfully performed secy config set for lane-map 0x%x of PHY-%d in %s path!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
                }
            }
        }
    }

    /* Enable flow control and configure 400G (8x53G) mode in PAM4 */
    flow_option = bcmplpFlowcontrolTerminateGenerate;
    /* flow_option = bcmplpFlowcontrolPassthrough; */
    memcpy(&mac_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        mac_info.phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        mac_info.phy_info.if_side = LINE_SIDE;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            mac_info.phy_info.lane_map = line_lane_map;

            retval = bcm_plp_mac_flow_control_set(CHIP_NAME, mac_info, flow_option);
            if (retval) {
                printf("FAIL: Failed to set flow control option for lane-map 0x%x of PHY-%d(ret = %d)!\n",
                mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully set flow control option %d for lane-map 0x%x of PHY-%d!\n",
                flow_option, mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr);
            }
            retval = bcm_plp_mac_flow_control_get(CHIP_NAME, mac_info, &flow_option_get);
            if (retval) {
                printf("FAIL: Failed to get flow control option for lane-map 0x%x of PHY-%d(ret = %d)!\n",
                mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully got flow control option %d for lane-map 0x%x of PHY-%d!\n",
                flow_option_get, mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr);
            }
            if (flow_option != flow_option_get) {
                printf("FAIL: flow options does not match for lane-map 0x%x of PHY-%d!\n",
                mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr);
                goto _aperta_config_error;
            }
        }
    }

    sf_enable_set = 1;
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map  = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];

            /* Set MAC store and forward mode at Line side */
            phy_info.lane_map = line_lane_map;
            phy_info.if_side = LINE_SIDE;

            printf("Setting MAC store and forward mode (%d) for lane-map 0x%x of PHY-%d at Line side...\n", sf_enable_set, phy_info.lane_map, phy_info.phy_addr);
            retval = bcm_plp_mac_store_and_forward_mode_set(CHIP_NAME, mac_info, sf_enable_set);
            if (retval) {
                printf("Failed to set MAC store and forward mode (%d) for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n", sf_enable_set, phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_init_error;
            }
            printf("Setting MAC store and forward mode (%d) for lane-map 0x%x of PHY-%d at Line side is successful\n", sf_enable_set, phy_info.lane_map, phy_info.phy_addr);

            /* Get MAC store and forward mode at Line side */
            printf("Getting MAC store and forward mode for lane-map 0x%x of PHY-%d at Line side...\n", phy_info.lane_map, phy_info.phy_addr);
            retval = bcm_plp_mac_store_and_forward_mode_get(CHIP_NAME, mac_info, &sf_enable_get);

            if (retval) {
                printf("Failed to get MAC store and forward mode for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n", phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_init_error;
            }

            /* Validate MAC store and forward mode at Line side */
            if (sf_enable_set != sf_enable_get) {
                printf("FAIL: MAC store and forward mode is not as expected for PHY-%d (expected: 0x%x, actual: 0x%x)!\n", phy_info.phy_addr, sf_enable_set, sf_enable_get);
                goto _aperta_init_error;
            }
            printf("Getting MAC store and forward mode for lane-map 0x%x of PHY-%d at Line side is successful\n", phy_info.lane_map, phy_info.phy_addr);
            printf("--------------------------------------------------------------------------------\n");
         }
     }

    /* -------------------------------------------------------------------------------- *
     * It is required to configure SYSTEM side first followed by LINE side              *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map  = sys_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];

            /* Set mode configuration at System side */
            phy_info.lane_map = sys_lane_map;
            phy_info.if_side  = SYS_SIDE;
            aux_mode_sys_set.lane_data_rate  = SYS_LANE_RATE;
            aux_mode_sys_set.modulation_mode = SYS_MODULATION;
            aux_mode_sys_set.fec_mode_sel    = SYS_FEC_MODE;
            aux_mode_sys_set.port_type       = SYS_PORT_TYPE;
            retval = bcm_plp_mode_config_set(CHIP_NAME, phy_info, SYS_PORT_SPEED, SYS_IF_TYPE, REF_CLOCK, SYS_IF_MODE, (void *)&aux_mode_sys_set);
            if (retval) {
                printf("FAIL: Failed to set mode configuration parameters for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully configured the mode for lane-map 0x%x of PHY-%d at System side!\n",
                phy_info.lane_map, phy_info.phy_addr);
            }

            /* Set mode configuration at Line side */
            phy_info.lane_map = line_lane_map;
            phy_info.if_side  = LINE_SIDE;
            aux_mode_line_set.lane_data_rate  = LINE_LANE_RATE;
            aux_mode_line_set.modulation_mode = LINE_MODULATION;
            aux_mode_line_set.fec_mode_sel    = LINE_FEC_MODE;
            aux_mode_line_set.port_type       = LINE_PORT_TYPE;
            retval = bcm_plp_mode_config_set(CHIP_NAME, phy_info, LINE_PORT_SPEED, LINE_IF_TYPE, REF_CLOCK, LINE_IF_MODE, (void *)&aux_mode_line_set);
            if (retval) {
                printf("FAIL: Failed to set mode configuration parameters for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully configured the mode for lane-map 0x%x of PHY-%d at Line side!\n",
                phy_info.lane_map, phy_info.phy_addr);
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Section 4: Link training                                                         *
     * -------------------------------------------------------------------------------- */
#ifdef LINK_TRAINING_EN
    /* -------------------------------------------------------------------------------- *
     * Start Force Tx training at system side                                           *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map = sys_lane_map_list[lane_map_index];
            phy_info.if_side = SYS_SIDE;
            phy_info.lane_map = sys_lane_map ;
            retval = bcm_plp_force_tx_training_set(CHIP_NAME, phy_info, ENABLE);
            if (retval) {
                printf("FAIL: Failed to enable training feature for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("Force Tx training is in progress for lane map 0x%x of PHY-%d at System side...\n",
                phy_info.lane_map, phy_info.phy_addr);
            }
        }
    }
    sleep(5);
    /* -------------------------------------------------------------------------------- *
     * Get Force Tx training status at System side                                      *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map = sys_lane_map_list[lane_map_index];
            phy_info.if_side  = SYS_SIDE;
            phy_info.lane_map = sys_lane_map;
            training_ena_dis  = DISABLE ;
            retval = bcm_plp_force_tx_training_get(CHIP_NAME, phy_info, &training_ena_dis);
            if (retval) {
                printf("FAIL: Failed to get training feature for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("Training enable/disable status for lane-map 0x%x of PHY-%d at System side as below :\n",
                phy_info.lane_map, phy_info.phy_addr);
                printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
            }

            training_ena_dis = DISABLE;
            training_failure = TRAINING_FAILURE;
            training_status  = UNTRAINED;

            retval = bcm_plp_force_tx_training_status_get(CHIP_NAME, phy_info, &training_ena_dis, &training_failure, &training_status);
            if (retval) {
              printf("FAIL: Failed to get training status for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
              phy_info.lane_map, phy_info.phy_addr, retval);
              goto _aperta_config_error;
            } else {
                printf("Training status for lane-map 0x%x of PHY-%d at System side as below :\n", phy_info.lane_map, phy_info.phy_addr);
                printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
                printf("training_failure        = %s\n", training_failure ? "TRAINING_FAILURE" : "TRAINING_COMPLETE");
                printf("training_status         = %s\n", training_status ? "TRAINED" : "UNTRAINED");
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Start Force Tx training at Line side                                             *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            phy_info.if_side = LINE_SIDE;
            phy_info.lane_map = line_lane_map ;
            retval = bcm_plp_force_tx_training_set(CHIP_NAME, phy_info, ENABLE);
            if (retval) {
                printf("FAIL: Failed to enable training feature for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("Force Tx training is in progress for lane map 0x%x of PHY-%d at Line side...\n",
                phy_info.lane_map, phy_info.phy_addr);
            }
        }
    }
    sleep(5);
    /* -------------------------------------------------------------------------------- *
     * Get Force Tx training status at Line side                                        *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            phy_info.if_side  = LINE_SIDE;
            phy_info.lane_map = line_lane_map;
            training_ena_dis  = DISABLE ;
            retval = bcm_plp_force_tx_training_get(CHIP_NAME, phy_info, &training_ena_dis);
            if (retval) {
                printf("FAIL: Failed to get training feature for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("Training enable/disable status for lane-map 0x%x of PHY-%d at Line side as below :\n",
                phy_info.lane_map, phy_info.phy_addr);
                printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
            }

            training_ena_dis = DISABLE;
            training_failure = TRAINING_FAILURE;
            training_status  = UNTRAINED;

            retval = bcm_plp_force_tx_training_status_get(CHIP_NAME, phy_info, &training_ena_dis, &training_failure, &training_status);
            if (retval) {
              printf("FAIL: Failed to get training status for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
              phy_info.lane_map, phy_info.phy_addr, retval);
              goto _aperta_config_error;
            } else {
                printf("Training status for lane-map 0x%x of PHY-%d at Line side as below :\n", phy_info.lane_map, phy_info.phy_addr);
                printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
                printf("training_failure        = %s\n", training_failure ? "TRAINING_FAILURE" : "TRAINING_COMPLETE");
                printf("training_status         = %s\n", training_status ? "TRAINED" : "UNTRAINED");
            }
        }
    }
#else
    /* -------------------------------------------------------------------------------- *
     * Configure TX FIR settings                                                        *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        bcm_plp_pam4_tx_t tx_set;
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (side=1; side>=0; side-=1) {
            phy_info.if_side = side;
            phy_info.lane_map = ALL_LANE_MAP;
            tx_set.pre  = TX_SET_PAM4_DEF_PRE;
            tx_set.main = TX_SET_PAM4_DEF_MAIN;
            tx_set.post = TX_SET_PAM4_DEF_POST;
            tx_set.serdes_tx_tap_mode = TX_SET_PAM4_TAP_MODE;
            retval = bcm_plp_pam4_tx_set(CHIP_NAME, phy_info, &tx_set);
            if (retval) {
                printf("FAIL: Failed to set default Tx TAP values in all lanes of PHY-%d at %s side (ret = %d)!\n",
                phy_info.phy_addr, (side==1) ? "System" : "Line", retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully set default Tx TAP values in all lanes of PHY-%d at %s side!\n",
                phy_info.phy_addr, (side==1) ? "System" : "Line");
            }
        }
    }
    sleep(5);
#endif

    /* Getting link status on SYS and LINE side */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            sys_lane_map  = sys_lane_map_list[lane_map_index];
            for (side = SYS_SIDE; side >= LINE_SIDE; side--) {
                phy_info.if_side = side;
                phy_info.lane_map = (side == SYS_SIDE) ? sys_lane_map : line_lane_map ;
                retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL: Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                }
                retval = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
                if (retval != TEST_SUCCESS) {
                    printf("FAIL: Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", retval);
                    goto _aperta_config_error;
                }
                /* Validate the link status */
                if (LINK_ON != link_status_get) {
                    retval = TEST_FAILURE;
                    printf("Link status for lane-map 0x%x of PHY-%d at %s side is not as expected (expected: %d, actual: %d)!\n",
                    phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", LINK_ON, link_status_get);
                    goto _aperta_config_error;
                }
            }
        }
    }

    /* -------------------------------------------------------------------------------- *
     * Section 5 : Macsec Transformations and sample cfye & secy operations             *
     * -------------------------------------------------------------------------------- */
    gen_macsec_configs();

    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            phy_info.lane_map = line_lane_map_list[lane_map_index];

            /* Filling phy_info */
            phy_info.if_side = LINE_SIDE;
            memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
            port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

            /*++++++++++++++++++++++++++++++++++++++++++++
                              install vPort
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                memset(&vportparams, 0, sizeof(bcm_plp_cfye_vport_t));

                vportparams.pkt_extension = macsec_side ? 0 : 3 ;
                vportparams.sectag_offset = 16;
                retval = bcm_plp_cfye_vport_add(CHIP_NAME, &sec_info, &vport_handle[phy_index][port][macsec_side], &vportparams);
                if (retval) {
                    printf("FAIL: bcm_plp_cfye_vport_add API failed for PHY-ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] \n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
                } else {
                    printf("PASS: bcm_plp_cfye_vport_add API successful for PHY-ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] \n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                }
                vport_ids[phy_index][port][macsec_side] = bcm_plp_cfye_vport_id_get(CHIP_NAME, &sec_info, vport_handle[phy_index][port][macsec_side]);
                printf("PASS: Successfully got vPort ID = %d for lane-map 0x%x of PHY-%d in %s path!\n",
                    vport_ids[phy_index][port][macsec_side], sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
             Perform cfye drop action if a vPort is missed
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for(macsec_side = 0 ; macsec_side < 2 ; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                dev_init.control_p = &dev_control;
                dev_init.control_p->exceptions_p = &exception;
                dev_init.control_p->exceptions_p->drop_action = BCM_PLP_CFYE_DROP_INTERNAL;
                retval = bcm_plp_cfye_device_update(CHIP_NAME, &sec_info, &dev_init);
                if(retval) {
                    printf("FAIL: bcm_plp_cfye_device_update for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                            sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                    goto _aperta_config_error;
                }
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
                   install sa with transform record
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                retval = macsec_install_sa_with_ct_params(sec_info, POLICY_BYPASS_NONE, pkt_type);
                if(retval) {
                    printf("FAIL: Failed to install SA with transform record for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                    goto _aperta_config_error;
                }
                if (macsec_install_parser_settings(sec_info, pkt_type) < 0) {
                    printf("FAIL: Failed to macsec_install_parser_settings for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                    goto _aperta_config_error;
                }
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
                           Add rule policy
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                retval = macsec_add_ct_rule_policy(sec_info, pkt_type);
                if(retval) {
                    printf("FAIL: Failed to add rule policy for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                    goto _aperta_config_error;
                }
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
                              Enable rule
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                retval = bcm_plp_cfye_rule_enable(CHIP_NAME, &sec_info, rule_handle[phy_index][port][macsec_side], 1);
                if(retval) {
                    printf("FAIL: bcm_plp_cfye_rule_enable API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
                } else {
                    printf("PASS : bcm_plp_cfye_rule_enable API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                }
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
                               MTU Update
            +++++++++++++++++++++++++++++++++++++++++++++*/
            sec_info.macsec_side = EGRESS;
            mtusize_bytes = 1500;
            retval = macsec_mtu_update(sec_info, mtusize_bytes);
            if(retval) {
                printf("FAIL: Failed to update mtu for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                       sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
                goto _aperta_config_error;
            }

	    /*++++++++++++++++++++++++++++++++++++++++++++
                       Dump all rules
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                retval = bcm_plp_cfye_diag_rule_dump(CHIP_NAME, &sec_info, NULL, 1);

                if(retval) {
                    printf("FAIL: bcm_plp_cfye_diag_rule_dump API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
                } else {
                    printf("PASS : bcm_plp_cfye_diag_rule_dump API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                }
            }
         }
     }

    printf("Please send %d %s type packet(s), Waiting for 10 seconds for packets to pass\n",
        NUM_OF_PKTS, pkt_type_name[pkt_type]);
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    sleep(10);
    printf("\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("Check Packet Statistics (Encrypted/Decrypted bytes, RxCAM hit, Number of pkts transformed etc)\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            phy_info.lane_map = line_lane_map_list[lane_map_index];
            /* Filling phy_info */
            phy_info.if_side = LINE_SIDE;
            memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
            port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
		retval = bcm_plp_cfye_diag_channel_dump(CHIP_NAME, &sec_info, 0);
                if(retval) {
                    printf("FAILED: bcm_plp_cfye_diag_channel_dump for phy_index[%d], macsec_side[%d], return code =[%d] \n", phy_index, macsec_side,retval);
                    goto _aperta_config_error;
                }
                printf("PASSED: bcm_plp_cfye_diag_channel_dump for phy_index[%d], macsec_side[%d], return code =[%d] \n", phy_index, macsec_side, retval);
                retval = bcm_plp_secy_diag_channel_dump (CHIP_NAME, &sec_info, 1);
                if(retval) {
                    printf("bcm_plp_secy_diag_channel_dump API FAILED for PHY_ID[%d], macsec_side[%d], ALL_LANE_MAP[0x%x], return code =[%d] \n", phy_index, macsec_side, phy_info.lane_map, retval);
                    goto _aperta_config_error;
                }
                printf("bcm_plp_secy_diag_channel_dump API successful for PHY_ID[%d], macsec_side[%d], ALL_LANE_MAP[0x%x], return code =[%d] \n", phy_index, macsec_side, phy_info.lane_map, retval);
            }
        }
    }

#ifdef MACSEC_UNINIT_EN
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            phy_info.lane_map = line_lane_map_list[lane_map_index];

            /* Filling phy_info */
            phy_info.if_side = LINE_SIDE;
            memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
            port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

            /*++++++++++++++++++++++++++++++++++++++++++++
                       Disable all rules
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                retval = bcm_plp_cfye_rule_enable_disable(CHIP_NAME, &sec_info, NULL, NULL, 0, 1, 1);

                if(retval) {
                    printf("FAIL: bcm_plp_cfye_rule_enable_disable API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
                } else {
                    printf("PASS : bcm_plp_cfye_rule_enable_disable API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                }
            }
            /*++++++++++++++++++++++++++++++++++++++++++++
                       remove rule
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                retval = bcm_plp_cfye_rule_remove(CHIP_NAME, &sec_info, rule_handle[phy_index][port][macsec_side]);

                if(retval) {
                    printf("FAIL: bcm_plp_cfye_rule_remove API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
                } else {
                    printf("PASS : bcm_plp_cfye_rule_remove API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                }
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
                       remove vPort
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                retval = bcm_plp_cfye_vport_remove(CHIP_NAME, &sec_info, vport_handle[phy_index][port][macsec_side]);
                if (retval){
                    printf("FAIL: bcm_plp_cfye_vport_remove API failed for PHY-ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] \n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
                } else {
                    printf("PASS: bcm_plp_cfye_vport_remove API successful for PHY-ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] \n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                }
            }

            /*++++++++++++++++++++++++++++++++++++++++++++
                       remove SA
            +++++++++++++++++++++++++++++++++++++++++++++*/
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side;
                retval = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info, secy_sahandle[phy_index][port][macsec_side]);
                if(retval) {
                    printf("FAIL: bcm_plp_secy_sa_remove API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                           sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
                    goto _aperta_config_error;
                } else {
                    printf("PASS : bcm_plp_secy_sa_remove API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
               }
            }
        }
    }
#endif
_aperta_config_error:
#ifdef MACSEC_UNINIT_EN
    /* -------------------------------------------------------------------------------- *
     * Uninitialize MACsec CfyE and SecY                                                *
     * -------------------------------------------------------------------------------- */
    phy_info.lane_map = ALL_LANE_MAP;
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
        for(macsec_side = 0; macsec_side < 2; macsec_side++) {
            sec_info.macsec_side = macsec_side; /* macsec_side = 0 -> Egress ; macsec_side = 1 -> Ingress */
            retval = macsec_uninitialize(sec_info);
            if (retval) {
                printf("FAIL: Failed to uninitialize MACSec device for PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            } else {
                printf("PASS: Successfully uninitialized MACSec device for PHY-%d in %s path!\n",
                sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
        }
    }
#endif
_aperta_macsec_init_error:
#ifdef MACSEC_UNINIT_EN
    /* -------------------------------------------------------------------------------- *
     * PHY and MAC Cleanup                                                              *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        phy_info.if_side = LINE_SIDE;

        memcpy(&mac_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
        retval = bcm_plp_mac_cleanup(CHIP_NAME, mac_info);
        if (retval) {
            printf("FAIL: Failed to cleanup mac for PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
        }
        retval = bcm_plp_cleanup(CHIP_NAME, phy_info);
        if (retval) {
            printf("FAIL: Failed to cleanup PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
        }
    }
#endif /* MACSEC_UNINIT_EN */

_aperta_init_error:
    /* Check for test result */
    if (!retval) {
        printf("Test for %s mode completed successfully\n", argv[0]);
    } else {
        printf("Test for %s mode failed!\n", argv[0]);
    }

    /* -------------------------------------------------------------------------------- *
     * Close the connections to the board                                               *
     * -------------------------------------------------------------------------------- */
    device_close();

    /* Return with test status */
    return retval;
}

