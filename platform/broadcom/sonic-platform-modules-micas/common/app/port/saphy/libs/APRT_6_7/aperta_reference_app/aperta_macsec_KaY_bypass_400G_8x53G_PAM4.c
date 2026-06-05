/*
 *
 *
 * $Copyright: (c) 2021 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 *
 *
 */

/*
 * The app is tested with System Side connected to a Broadcom Switch,
 * that generates Egress traffic on  BCM81343  PHY-0 and with Line Side
 * of  BCM81343  PHY-0 connected to Line Side of BCM81343  PHY-1
 * BCM81343  PHY-1 is configured for Ingress and its System Side is
 * connected to the same Broadcom Switch.
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
 *   8. Create/Add vPort(SC) on each phy-id for both Egress and Ingress             *
 *   9. Create/add SA for Egress and Ingress path                                   *
 *   10.Update cfye to process for KaY                                              *
 *   11.Send Traffic with SecTAG 0x88E5, KaY packets should Rx as Data packets)     *
 *   12.Read Statistics. Observe packets as tagged Data packets at INGRESS          *                                                         *
 *   13.Create/Add another SA (Egress) (using Chaining) and allow Data pkts.        *
 *   14.Switch to newly created SA (Egress).                                        *
 *   15.Update newly created SA (Egress).                                           *
 *   16.Remove Old SA.                                                              *
 *   17.Create/Add another SA (Ingress)and allow tagged control pkts.               *
 *   18.Update newly created SA (Egress).                                           *
 *   19.Remove Old SA.                                                              *
 *   20.Send Traffic. KaY pkts send should reach destination as Control Pkts)       *
 *   21.Read Statistics.                                                            *
 * -------------------------------------------------------------------------------- *
 * Note 1 : This sample application is only for 81343 parts.                        *
 * -------------------------------------------------------------------------------- */


/* Enable MACsec un-initialization */
#define MACSEC_UNINIT_EN

/* Disable any bypass set in the Makefile */
#undef BCM_PLP_CONTROL_PACKET_BYPASS
#undef BCM_PLP_DATA_PACKET_BYPASS

#define TRAINED                 1
#define UNTRAINED               0
#define TRAINING_COMPLETE       0
#define TRAINING_FAILURE        1


/* Enable Tx forced training */
#define LINK_TRAINING_EN

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

int print_statistics_local(int saindex);

/* Transform Records - Ingress */
static uint32_t transform_record_basic_transform_ingress[] =
{
    0xd24be06f, /* CCW [3:0] = 0xf -> Ingress , 32-bit packet numbering, encrypt ICV */
    0x0000c000, /* SA Update CW */
    0xd02b7aad, /* Key0 */
    0x5a83ac3e, /* Key1 */
    0xdc0f626f, /* Key2 */
    0x45b306b5, /* Key3 */
    0x00000000, /* Key4 */
    0x00000000, /* Key5 */
    0x00000000, /* Key6 */
    0x00000000, /* Key7 */
    0x803da273, /* H_Key0 */
    0xd5e21d12, /* H_Key1 */
    0x3f2550a8, /* H_Key2 */
    0x0e1243cf, /* H_Key3 */
    0x00000005, /* Seq0 */
    0x00000000, /* Seq1*/
    0x00000080, /* Mask  - Replay Window */
    0x00000000, /* (zero) */
    0x00000000, /* (zero) */
    0x00000000, /* (zero) */
    0xefbeadde, /* IV0 (SCI/CtxSalt) */
    0x01005aa5, /* IV1 (SCI/CtxSalt) */
    0x00000000, /* IV2 (CtxSalt) */
};

static const unsigned char srcpacket_basic_transform_ingress_sci[] =
{
    0xde, 0xad, 0xbe, 0xef, 0xa5, 0x5a, 0x00, 0x01
};
static const unsigned char * sci_basic_transform_ingress_p = &srcpacket_basic_transform_ingress_sci[0];

/* Transform Record - Egress */
/* key and hash key match with ingress to ensure encryption and decryption work */
static uint32_t transform_record_basic_transform_egress[] =
{
    0x924be066, /* CCW */
    0x8000c001, /* SA Update CW */
    0xd02b7aad, /* Key0*/
    0x5a83ac3e, /* Key1*/
    0xdc0f626f, /* Key2*/
    0x45b306b5, /* Key3*/
    0x00000000, /* Key4*/
    0x00000000, /* Key5*/
    0x00000000, /* Key6*/
    0x00000000, /* Key7*/
    0x803da273, /* H_Key0*/
    0xd5e21d12, /* H_Key1*/
    0x3f2550a8, /* H_Key2*/
    0x0e1243cf, /* H_Key3*/
    0x00000004, /* Seq0*/
    0x00000000, /* Seq1*/
    0x000005DC, /* (zero)*/
    0x00000000, /* IS0 (CtxSalt)*/
    0x00000000, /* IS1 (CtxSalt)*/
    0x00000000, /* IS2 (CtxSalt)*/
    0xefbeadde, /* IV0*/
    0x01005aa5, /* IV1*/
    0x00000000, /* (zero)*/
};

static uint32_t transform_record_basic_transform_ingress1[] =
{
    0xd24be46f, /* CCW [3:0] = 0xf -> Ingress , 32-bit packet numbering, encrypt ICV*/
    0x0000c000, /* SA Update CW */
    0xd02b7aad, /* Key0 */
    0x5a83ac3e, /* Key1*/
    0xdc0f626f, /* Key2*/
    0x45b306b5, /* Key3*/
    0x00000000, /* Key4*/
    0x00000000, /* Key5*/
    0x00000000, /* Key6*/
    0x00000000, /* Key7*/
    0x803da273, /* H_Key0*/
    0xd5e21d12, /* H_Key1*/
    0x3f2550a8, /* H_Key2*/
    0x0e1243cf, /* H_Key3*/
    0x00000005, /* Seq0*/
    0x00000000, /* Seq1*/
    0x00000080, /* Mask  - Replay Window*/
    0x00000000, /* (zero)*/
    0x00000000, /* (zero)*/
    0x00000000, /* (zero)*/
    0xefbeadde, /* IV0 (SCI/CtxSalt)*/
    0x01005aa5, /* IV1 (SCI/CtxSalt)*/
    0x00000000, /* IV2 (CtxSalt)*/
};

/*SCI */
static const unsigned char srcpacket_basic_transform_ingress_sci1[] = {
    0xde, 0xad, 0xbe, 0xef, 0xa5, 0x5a, 0x00, 0x01
};
static const unsigned char * sci_basic_transform_ingress1_p = &srcpacket_basic_transform_ingress_sci1[0];

static uint32_t transform_record_basic_transform_egress1[] =
{
    0x964be466, /* CCW*/
    0x8000c001, /* SA Update CW */
    0xd02b7aad, /* Key0*/
    0x5a83ac3e, /* Key1*/
    0xdc0f626f, /* Key2*/
    0x45b306b5, /* Key3*/
    0x00000000, /* Key4*/
    0x00000000, /* Key5*/
    0x00000000, /* Key6*/
    0x00000000, /* Key7*/
    0x803da273, /* H_Key0*/
    0xd5e21d12, /* H_Key1*/
    0x3f2550a8, /* H_Key2*/
    0x0e1243cf, /* H_Key3*/
    0x00000004, /* Seq0*/
    0x00000000, /* Seq1*/
    0x00000000, /* (zero)*/
    0x00000000, /* IS0 (CtxSalt)*/
    0x00000000, /* IS1 (CtxSalt)*/
    0x00000000, /* IS2 (CtxSalt)*/
    0xefbeadde, /* IV0*/
    0x01005aa5, /* IV1*/
    0x00000000, /* (zero)*/
};

extern bcm_plp_cfye_vport_handle_t vport_handle[2][8][2];
extern bcm_plp_cfye_rule_handle_t  rule_handle[2][8][2];
extern unsigned int vport_ids[2][8][2];
static bcm_plp_secy_sa_handle_t secy_sahandle[4][2];

/* Main Start here */
int main(int argc, char *argv[])
{
    unsigned int phy_index = 0;
    int rv = 0, macsec_side = 0;
    int sf_enable_get = 0;
    int sf_enable_set;
    int p_ctxt = 5;
    int side = 0;
    int port = 0;
    int sa_id = 0;
    int lane_map_index;
    int total_lane_maps;
    int device_bypass_enable = 0;
    unsigned int link_status_get;
    unsigned int sys_lane_map;
    unsigned int line_lane_map;
    unsigned int fw_ver;
    unsigned int fw_crc;
    bcm_plp_secy_sa_t sa_params;
    bcm_plp_secy_sa_t sa_params1;
    bcm_plp_cfye_vport_t vportparams;
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
    bcm_plp_cfye_rule_t ruleparams;
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
    memset(&vportparams, 0, sizeof(bcm_plp_cfye_vport_t));
    memset(&ruleparams, 0, sizeof(bcm_plp_cfye_rule_t));
    memset(&sa_params, 0, sizeof(bcm_plp_secy_sa_t));
    memset(&sa_params1, 0, sizeof(bcm_plp_secy_sa_t));
    phy_info.platform_ctxt = &p_ctxt;

    /* -------------------------------------------------------------------------------- *
     * Connect to the board                                                             *
     * -------------------------------------------------------------------------------- */
    rv = device_open();
    if (rv) {
        printf("FAIL: Failed to connect to the board (ret = %d)!\n", rv);
        return rv;
    }
    /* -------------------------------------------------------------------------------- *
     * Section 1: PHY, MACsec Initialization and MACsec Configuration                   *
     * -------------------------------------------------------------------------------- */

    /* -------------------------------------------------------------------------------- *
     * Initialize the PHYs and download firmware based on the setup                     *
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
        rv = bcm_plp_init_fw_bcast(CHIP_NAME, phy_info, mdio_read, mdio_write, &firmware_load_type, bcmpmFirmwareBroadcastNone);
        if (rv) {
            printf("FAIL: bcm_plp_init_fw_bcast failed to initialize PHY-%d (ret = %d)!\n", phy_info.phy_addr, rv);
            goto _aperta_init_error;
        }

        /* Read firmware info */
        rv = bcm_plp_firmware_info_get(CHIP_NAME, phy_info, &fw_ver, &fw_crc);
        if (rv) {
            printf("FAIL: Failed to get the firmware info for PHY-%d (ret = %d)!\n", phy_info.phy_addr, rv);
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
            rv = macsec_initialize(sec_info, device_bypass_enable);
            if (rv != BCM_PLP_SECY_STATUS_OK) {
                printf("FAIL: MACSec Initialize for PHY-ID[%d], macsec_side[%d], return code [%d]\n",
                        phy_info.phy_addr, sec_info.macsec_side, rv);
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
    rv = polarity_swap();
    if (rv) {
        printf("FAIL: Failed to get and set initial Tx/Rx polarity with ret = %d\n", rv);
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
        rv = bcm_plp_rxtx_laneswap_set(CHIP_NAME, phy_info, &sys_laneswap_map);
        if (rv) {
            printf("FAIL: Failed to set lane swap for PHY-%d at System side (ret = %d)!\n", phy_info.phy_addr, rv);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully configured the lane swap for PHY-%d at System side!\n", phy_info.phy_addr);
        }

        phy_info.if_side = LINE_SIDE;
        rv = bcm_plp_rxtx_laneswap_set(CHIP_NAME, phy_info, &line_laneswap_map);
        if (rv) {
            printf("FAIL: Failed to set lane swap for PHY-%d at Line side (ret = %d)!\n", phy_info.phy_addr, rv);
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
                rv = bcm_plp_secy_config_set(CHIP_NAME, &sec_info);
                if (rv != BCM_PLP_SECY_STATUS_OK) {
                    printf("FAIL: Failed to perform secy config set for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                            sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
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
    memcpy(&mac_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        mac_info.phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        mac_info.phy_info.if_side = LINE_SIDE;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            line_lane_map = line_lane_map_list[lane_map_index];
            mac_info.phy_info.lane_map = line_lane_map;

            rv = bcm_plp_mac_flow_control_set(CHIP_NAME, mac_info, flow_option);
            if (rv) {
                printf("FAIL: Failed to set flow control option for lane-map 0x%x of PHY-%d(ret = %d)!\n",
                        mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr, rv);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully set flow control option %d for lane-map 0x%x of PHY-%d!\n",
                        flow_option, mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr);
            }
            rv = bcm_plp_mac_flow_control_get(CHIP_NAME, mac_info, &flow_option_get);
            if (rv) {
                printf("FAIL: Failed to get flow control option for lane-map 0x%x of PHY-%d(ret = %d)!\n",
                        mac_info.phy_info.lane_map, mac_info.phy_info.phy_addr, rv);
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
            rv = bcm_plp_mac_store_and_forward_mode_set(CHIP_NAME, mac_info, sf_enable_set);
            if (rv) {
                printf("Failed to set MAC store and forward mode (%d) for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n", sf_enable_set, phy_info.lane_map, phy_info.phy_addr, rv);
                goto _aperta_init_error;
            }
            printf("Setting MAC store and forward mode (%d) for lane-map 0x%x of PHY-%d at Line side is successful\n", sf_enable_set, phy_info.lane_map, phy_info.phy_addr);

            /* Get MAC store and forward mode at Line side */
            printf("Getting MAC store and forward mode for lane-map 0x%x of PHY-%d at Line side...\n", phy_info.lane_map, phy_info.phy_addr);
            rv = bcm_plp_mac_store_and_forward_mode_get(CHIP_NAME, mac_info, &sf_enable_get);

            if (rv) {
                printf("Failed to get MAC store and forward mode for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n", phy_info.lane_map, phy_info.phy_addr, rv);
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
            rv = bcm_plp_mode_config_set(CHIP_NAME, phy_info, SYS_PORT_SPEED, SYS_IF_TYPE, REF_CLOCK, SYS_IF_MODE, (void *)&aux_mode_sys_set);
            if (rv) {
                printf("FAIL: Failed to set mode configuration parameters for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                        phy_info.lane_map, phy_info.phy_addr, rv);
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
            rv = bcm_plp_mode_config_set(CHIP_NAME, phy_info, LINE_PORT_SPEED, LINE_IF_TYPE, REF_CLOCK, LINE_IF_MODE, (void *)&aux_mode_line_set);
            if (rv) {
                printf("FAIL: Failed to set mode configuration parameters for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                        phy_info.lane_map, phy_info.phy_addr, rv);
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
            rv = bcm_plp_force_tx_training_set(CHIP_NAME, phy_info, ENABLE);
            if (rv) {
                printf("FAIL: Failed to enable training feature for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                        phy_info.lane_map, phy_info.phy_addr, rv);
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
            rv = bcm_plp_force_tx_training_get(CHIP_NAME, phy_info, &training_ena_dis);
            if (rv) {
                printf("FAIL: Failed to get training feature for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                        phy_info.lane_map, phy_info.phy_addr, rv);
                goto _aperta_config_error;
            } else {
                printf("Training enable/disable status for lane-map 0x%x of PHY-%d at System side as below :\n",
                        phy_info.lane_map, phy_info.phy_addr);
                printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
            }

            training_ena_dis = DISABLE;
            training_failure = TRAINING_FAILURE;
            training_status  = UNTRAINED;

            rv = bcm_plp_force_tx_training_status_get(CHIP_NAME, phy_info, &training_ena_dis, &training_failure, &training_status);
            if (rv) {
                printf("FAIL: Failed to get training status for lane-map 0x%x of PHY-%d at System side (ret = %d)!\n",
                        phy_info.lane_map, phy_info.phy_addr, rv);
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
            rv = bcm_plp_force_tx_training_set(CHIP_NAME, phy_info, ENABLE);
            if (rv) {
                printf("FAIL: Failed to enable training feature for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                        phy_info.lane_map, phy_info.phy_addr, rv);
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
            rv = bcm_plp_force_tx_training_get(CHIP_NAME, phy_info, &training_ena_dis);
            if (rv) {
                printf("FAIL: Failed to get training feature for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                        phy_info.lane_map, phy_info.phy_addr, rv);
                goto _aperta_config_error;
            } else {
                printf("Training enable/disable status for lane-map 0x%x of PHY-%d at Line side as below :\n",
                        phy_info.lane_map, phy_info.phy_addr);
                printf("training_enable_disable = %s\n", training_ena_dis ? "TRAINING_ENABLE" : "TRAINING_DISABLE");
            }

            training_ena_dis = DISABLE;
            training_failure = TRAINING_FAILURE;
            training_status  = UNTRAINED;

            rv = bcm_plp_force_tx_training_status_get(CHIP_NAME, phy_info, &training_ena_dis, &training_failure, &training_status);
            if (rv) {
                printf("FAIL: Failed to get training status for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                        phy_info.lane_map, phy_info.phy_addr, rv);
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
            rv = bcm_plp_pam4_tx_set(CHIP_NAME, phy_info, &tx_set);
            if (rv) {
                printf("FAIL: Failed to set default Tx TAP values in all lanes of PHY-%d at %s side (ret = %d)!\n",
                        phy_info.phy_addr, (side==1) ? "System" : "Line", rv);
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
                rv = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
                if (rv) {
                    printf("FAIL: Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                            phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", rv);
                    goto _aperta_config_error;
                }
                rv = bcm_plp_link_status_get(CHIP_NAME, phy_info,  &link_status_get);
                if (rv) {
                    printf("FAIL: Failed to get link status for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                            phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", rv);
                    goto _aperta_config_error;
                }
                /* Validate the link status */
                if (LINK_ON != link_status_get) {
                    rv = TEST_FAILURE;
                    printf("Link status for lane-map 0x%x of PHY-%d at %s side is not as expected (expected: %d, actual: %d)!\n",
                            phy_info.lane_map, phy_info.phy_addr, (side == SYS_SIDE) ? "System" : "Line", LINK_ON, link_status_get);
                    goto _aperta_config_error;
                }
            }
        }
    }

    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        phy_info.if_side  = LINE_SIDE;
        phy_info.lane_map = ALL_LANE_MAP;
        memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
        port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);
        /* Install vPort */
        for(macsec_side = 0 ; macsec_side < 2 ; macsec_side ++)
        {
            sec_info.macsec_side = macsec_side;
            vportparams.pkt_extension = macsec_side ? 0 : 3;
            rv = bcm_plp_cfye_vport_add(CHIP_NAME, &sec_info,
                    &vport_handle[phy_index][port][macsec_side],
                    &vportparams);
            if (rv) {
                printf("FAIL: Failed to add vport for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully add vport for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
        }

        for(macsec_side = 0 ; macsec_side < 2 ; macsec_side ++)
        {
            sec_info.macsec_side = macsec_side;
            /* Get the vPort id from Handle */
            vport_ids[phy_index][port][macsec_side] = bcm_plp_cfye_vport_id_get(CHIP_NAME, &sec_info,
                    vport_handle[phy_index][port][macsec_side]);
            printf("PASS: Successfully got vPort ID = %d for lane-map 0x%x of PHY-%d in %s path\n",
                    vport_ids[phy_index][port][macsec_side], sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* install sa with transform record */
        /* INGRESS Param settings */
        sa_params.action_type = BCM_PLP_SECY_SA_ACTION_INGRESS;
        sa_params.drop_type   = BCM_PLP_SECY_SA_DROP_CRC_ERROR;
        sa_params.dest_port   = BCM_PLP_SECY_PORT_CONTROLLED;

        sa_params.params.ingress.validate_frames_tagged = BCM_PLP_SECY_FRAME_VALIDATE_CHECK;
        sa_params.params.ingress.fsa_inuse = 1;
        sa_params.params.ingress.freplay_protect = 1;
        sa_params.params.ingress.sci_p = discard_const(sci_basic_transform_ingress_p);

        sa_params.params.ingress.an = 0;
        sa_params.params.ingress.fallow_tagged = 1;
        sa_params.params.ingress.fallow_untagged = 1;
        sa_params.params.ingress.fvalidate_untagged = 0;

        /* size of the transform record in 32-bit words*/
        sa_params.sa_word_count =
            sizeof(transform_record_basic_transform_ingress) /
            sizeof(uint32_t);
        sa_params.transform_record_p =
            transform_record_basic_transform_ingress;
        /*Egress Param settings */
        sa_params1.action_type = BCM_PLP_SECY_SA_ACTION_EGRESS;
        sa_params1.drop_type = BCM_PLP_SECY_SA_DROP_INTERNAL;
        sa_params1.dest_port = BCM_PLP_SECY_PORT_CONTROLLED;

        sa_params1.params.egress.fsa_inuse = 1;
        sa_params1.params.egress.fprotect_frames = 1;
        sa_params1.params.egress.finclude_sci = 1;
        sa_params1.params.egress.fconf_protect = 1;
        sa_params1.params.egress.fallow_data_pkts = 1;

        /* size of the transform record in 32-bit words*/
        sa_params1.sa_word_count =
            sizeof(transform_record_basic_transform_egress) /
            sizeof(uint32_t);
        sa_params1.transform_record_p =
            transform_record_basic_transform_egress;

        /*Egress */
        macsec_side = EGRESS;
        sec_info.macsec_side = macsec_side;
        rv = bcm_plp_secy_sa_add(CHIP_NAME, &sec_info,
                vport_ids[phy_index][port][macsec_side],
                &secy_sahandle[phy_index][macsec_side],
                &sa_params1
                );
        if (rv) {
            printf("FAIL: Failed to add SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully add SA for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        /* Ingress */
        macsec_side = INGRESS;
        sec_info.macsec_side = macsec_side;
        rv = bcm_plp_secy_sa_add(CHIP_NAME, &sec_info,
                vport_ids[phy_index][port][macsec_side],
                &secy_sahandle[phy_index][macsec_side],
                &sa_params
                );
        if (rv) {
            printf("FAIL: Failed to add SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully add SA for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        for(macsec_side = 0 ; macsec_side < 2 ; macsec_side ++)
        {
            sec_info.macsec_side = macsec_side;
            ruleparams.data_mask[0]=0x00000000; /* match on any destination address.*/
            ruleparams.data_mask[1]=0x00000000;
            ruleparams.policy.vport_handle = vport_handle[phy_index][port][macsec_side];
            /* Rules Add */
            rv = bcm_plp_cfye_rule_add( CHIP_NAME, &sec_info,
                    vport_handle[phy_index][port][macsec_side],
                    &rule_handle[phy_index][port][macsec_side],
                    &ruleparams
                    );
            if (rv) {
                printf("FAIL: Failed to add rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully add rule for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
            /* Enable Rule */
            rv = bcm_plp_cfye_rule_enable(CHIP_NAME, &sec_info,
                    rule_handle[phy_index][port][macsec_side],
                    1
                    );
            if (rv) {
                printf("FAIL: Failed to enable rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully enable rule for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
        }
    }

    /* Call Cfye Device Update */
    /* This block of code is to demonstrate KaY Processing */
    {
        bcm_plp_secy_rule_sectag_t sec_tag_rules;

        memset(&sec_tag_rules, 0, sizeof(bcm_plp_secy_rule_sectag_t));

        sec_tag_rules.fcomp_etype = 1;
        sec_tag_rules.fcheck_v = 1;
        sec_tag_rules.fcheck_kay = 1;
        sec_tag_rules.fcheck_ce = 1;
        sec_tag_rules.fcheck_sc = 0;
        sec_tag_rules.fcheck_sl = 0;
        sec_tag_rules.ether_type = 0x88E5;

        for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
            sec_info.phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1; 
            sec_info.phy_info.if_side  = LINE_SIDE;
            sec_info.phy_info.lane_map = ALL_LANE_MAP;

            /* macsec_side */
            for(macsec_side = 0 ; macsec_side < 2 ; macsec_side ++)
            {
                sec_info.macsec_side = macsec_side;
                rv = bcm_plp_secy_rules_sectag_update(CHIP_NAME, &sec_info, &sec_tag_rules);
                if (rv) {
                    printf("FAIL: Failed to update sectag rules for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                            sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
                    goto _aperta_config_error;
                } else {
                    printf("PASS: Successfully update sectag rules for lane-map 0x%x of PHY-%d in %s path\n",
                            sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
                }
            }
        }
    }

    /*
Note: At this point KaY Packets will be routed as Tagged Data packets (by Cfye Device) to destination.
*/

#ifdef TRAFFIC_DEBUG
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("\n Please send packets now: and press ENTER after sending packets\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    getchar();
    /* Print Statistics */
    print_statistics_local(0);
#endif

    /* Add SecY Handle to allow KaY packets as control packets now */

    for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++, sa_id++) {
        sec_info.phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        sec_info.phy_info.if_side  = LINE_SIDE;
        sec_info.phy_info.lane_map = ALL_LANE_MAP;
        port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

        sa_params1.action_type = BCM_PLP_SECY_SA_ACTION_EGRESS;
        sa_params1.drop_type = BCM_PLP_SECY_SA_DROP_INTERNAL;
        sa_params1.dest_port = BCM_PLP_SECY_PORT_CONTROLLED;

        sa_params1.params.egress.fsa_inuse = 1;
        sa_params1.params.egress.fprotect_frames = 1;
        sa_params1.params.egress.finclude_sci = 1;
        sa_params1.params.egress.fconf_protect = 1;
        sa_params1.params.egress.fallow_data_pkts = 0;/* Do not allow Data Packets */

        sa_params1.sa_word_count =
            sizeof(transform_record_basic_transform_egress1) /
            sizeof(uint32_t);
        sa_params1.transform_record_p =
            transform_record_basic_transform_egress1;

        macsec_side = EGRESS;
        sec_info.macsec_side = macsec_side;
        /* SA Chain */
        rv = bcm_plp_secy_sa_chain(CHIP_NAME, &sec_info,
                secy_sahandle[phy_index][macsec_side],
                &secy_sahandle[phy_index + 2][macsec_side],
                &sa_params1
                );
        if (rv) {
            printf("FAIL: Failed to SA chain for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA chain for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        /* Switch to New SA */
        rv = bcm_plp_secy_sa_switch(CHIP_NAME, &sec_info,
                secy_sahandle[phy_index][macsec_side],
                secy_sahandle[phy_index + 2][macsec_side],
                &sa_params1);
        if (rv) {
            printf("FAIL: Failed to SA switch for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA switch for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        /* Update New SA to not allow Data packets */
        rv = bcm_plp_secy_sa_update(CHIP_NAME, &sec_info,
                secy_sahandle[sa_id + 2][macsec_side],&sa_params1);
        if (rv) {
            printf("FAIL: Failed to SA update for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA update for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* Remove Old SA */
        rv = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info,
                secy_sahandle[phy_index][macsec_side]);
        if (rv) {
            printf("FAIL: Failed to SA remove for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA remove for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /*++++++++++++++++++++++++++++++++++++ */

        /* Ingress */
        sa_params.params.ingress.fallow_tagged = 1;
        sa_params.params.ingress.sci_p = discard_const(sci_basic_transform_ingress1_p);
        sa_params.params.ingress.an  = 1;

        /* size of the transform record in 32-bit words*/
        sa_params.sa_word_count =
            sizeof(transform_record_basic_transform_ingress1) /
            sizeof(uint32_t);
        sa_params.transform_record_p =
            transform_record_basic_transform_ingress1;

        /* Filling phy_info */
        macsec_side = INGRESS;
        sec_info.macsec_side = macsec_side;

        /* Attach the transform records to the vport */
        rv = bcm_plp_secy_sa_add(CHIP_NAME, &sec_info,
                vport_ids[phy_index][port][macsec_side],
                &secy_sahandle[sa_id + 2][macsec_side],
                &sa_params
                );
        if (rv) {
            printf("FAIL: Failed to SA add for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA add for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* Update New SA to not allow Data packets */
        rv = bcm_plp_secy_sa_update(CHIP_NAME, &sec_info,
                secy_sahandle[sa_id + 2][macsec_side],&sa_params);
        if (rv) {
            printf("FAIL: Failed to SA update for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA update for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        /* Remove Old SA */
        rv = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info,
                secy_sahandle[phy_index][macsec_side]);
        if (rv) {
            printf("FAIL: Failed to SA remove for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully SA remove for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
    }
    /* Call Cfye Device Update */
    /* This block of code is to demonstrate KaY Processing */
    {
        bcm_plp_cfye_device_t primary_parser_control;
        bcm_plp_cfye_control_packet_t cp_p;
        bcm_plp_secy_rule_sectag_t sec_tag_rules;

        memset(&primary_parser_control,0,sizeof(bcm_plp_cfye_device_t));
        memset(&cp_p,0,sizeof(bcm_plp_cfye_control_packet_t));
        memset(&sec_tag_rules, 0, sizeof(bcm_plp_secy_rule_sectag_t));

        /* Classify KaY packets as control packets */
        cp_p.cp_match_enable_mask = 0x80000000;
        primary_parser_control.cp_p = &cp_p;

        sec_tag_rules.fcomp_etype = 1;
        sec_tag_rules.fcheck_v = 1;
        sec_tag_rules.fcheck_kay = 1;
        sec_tag_rules.fcheck_ce = 1;
        sec_tag_rules.fcheck_sc = 0;
        sec_tag_rules.fcheck_sl = 0;
        sec_tag_rules.ether_type = 0x88E5;

        for (phy_index = 0; phy_index <  NUM_OF_PHY; phy_index++) {
            sec_info.phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
            sec_info.phy_info.if_side  = LINE_SIDE;
            sec_info.phy_info.lane_map = ALL_LANE_MAP;
            /* macsec_side */
            for(macsec_side = 0 ; macsec_side < 2 ; macsec_side ++)
            {
                sec_info.macsec_side = macsec_side;
                rv = bcm_plp_cfye_device_update(CHIP_NAME, &sec_info,&primary_parser_control);
                if (rv) {
                    printf("FAIL: Failed to Cfye device update for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                            sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
                    goto _aperta_config_error;
                } else {
                    printf("PASS: Successfully Cfye device update for lane-map 0x%x of PHY-%d in %s path\n",
                            sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
                }
                rv = bcm_plp_secy_rules_sectag_update(CHIP_NAME, &sec_info, &sec_tag_rules);
                if (rv) {
                    printf("FAIL: Failed to update sectag rules for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                            sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
                    goto _aperta_config_error;
                } else {
                    printf("PASS: Successfully update sectag rules for lane-map 0x%x of PHY-%d in %s path\n",
                            sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
                }
            }
        }
    }

    /*
Note: At this point Packets will be routed as Tagged Data packets (by Cfye Device) to destination.
*/

#ifdef TRAFFIC_DEBUG
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    printf("\n Please send packets now: and press ENTER after sending packets\n");
    printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
    getchar();
    /* Print Statistics */
    print_statistics_local(2);
#endif



#ifdef MACSEC_UNINIT_EN
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        phy_info.lane_map = ALL_LANE_MAP;
        phy_info.if_side = LINE_SIDE;
        memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
        port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

        /*++++++++++++++++++++++++++++++++++++++++++++
          Disable all rules
          +++++++++++++++++++++++++++++++++++++++++++++*/
        for (macsec_side = 0; macsec_side < 2; macsec_side++) {
            sec_info.macsec_side = macsec_side;
            rv = bcm_plp_cfye_rule_enable_disable(CHIP_NAME, &sec_info, NULL, NULL, 0, 1, 1);
            if (rv) {
                printf("FAIL: Failed to disable rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully disable rule for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
        }

        /*++++++++++++++++++++++++++++++++++++++++++++
          remove rule
          +++++++++++++++++++++++++++++++++++++++++++++*/
        for (macsec_side = 0; macsec_side < 2; macsec_side++) {
            sec_info.macsec_side = macsec_side;
            rv = bcm_plp_cfye_rule_remove(CHIP_NAME, &sec_info, rule_handle[phy_index][port][macsec_side]);
            if (rv) {
                printf("FAIL: Failed to remove rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully remove rule for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
        }

        /*++++++++++++++++++++++++++++++++++++++++++++
          remove vPort
          +++++++++++++++++++++++++++++++++++++++++++++*/
        for (macsec_side = 0; macsec_side < 2; macsec_side++) {
            sec_info.macsec_side = macsec_side;
            rv = bcm_plp_cfye_vport_remove(CHIP_NAME, &sec_info, vport_handle[phy_index][port][macsec_side]);
            if (rv) {
                printf("FAIL: Failed to remove vport for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully remove vport for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
        }

        /*++++++++++++++++++++++++++++++++++++++++++++
          remove SA
          +++++++++++++++++++++++++++++++++++++++++++++*/
        sec_info.macsec_side = EGRESS;
        rv = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info, secy_sahandle[phy_index + 2][EGRESS]);
        if (rv) {
            printf("FAIL: Failed to remove SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully remove SA for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        sec_info.macsec_side = INGRESS;
        rv = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info, secy_sahandle[phy_index + 2][INGRESS]);
        if (rv) {
            printf("FAIL: Failed to remove SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully remove SA for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
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
            rv = macsec_uninitialize(sec_info);
            if (rv) {
                printf("FAIL: Failed to uninitialize MACSec device for PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
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
        rv = bcm_plp_mac_cleanup(CHIP_NAME, mac_info);
        if (rv) {
            printf("FAIL: Failed to cleanup mac for PHY-%d (ret = %d)!\n", phy_info.phy_addr, rv);
        }
        rv = bcm_plp_cleanup(CHIP_NAME, phy_info);
        if (rv) {
            printf("FAIL: Failed to cleanup PHY-%d (ret = %d)!\n", phy_info.phy_addr, rv);
        }
    }
#endif /* MACSEC_UNINIT_EN */

_aperta_init_error:
    /* Check for application result */
    if (!rv) {
        printf("Application for %s mode completed successfully\n", argv[0]);
    } else {
        printf("Application for %s mode failed!\n", argv[0]);
    }

    /* -------------------------------------------------------------------------------- *
     * Close the connections to the board                                               *
     * -------------------------------------------------------------------------------- */
    device_close();

    /* Return with application status */
    return rv;
}

int print_statistics_local(int saindex)
{
    int rv = 0;
    int port = 0;
    int phy_index = 0;
    int macsec_side = 0, sa_id = 0;
    bcm_plp_sec_phy_access_t sec_info;
    bcm_plp_cfye_statistics_channel_t chanstat;
    bcm_plp_secy_secy_stat_e_t secy_stat_e;
    bcm_plp_secy_sa_stat_e_t sa_stat_e;
    bcm_plp_secy_secy_stat_i_t secy_stat_i;
    bcm_plp_secy_sa_stat_i_t sa_stat_i;
    memset(&sec_info, 0, sizeof(bcm_plp_sec_phy_access_t));

    /* Egress */
    for (phy_index = 0; phy_index < NUM_OF_PHY - 1; phy_index++, sa_id++) 
    {
        sec_info.phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        sec_info.phy_info.if_side  = LINE_SIDE;
        sec_info.phy_info.lane_map = ALL_LANE_MAP;
        port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("PHY_ID = 0x%x\n", sec_info.phy_info.phy_addr);
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");

        /* SECY SECY Stats - Egress */
        sec_info.macsec_side = EGRESS;
        memset(&secy_stat_e, 0, sizeof(bcm_plp_secy_secy_stat_e_t));

        rv = bcm_plp_secy_secy_statistics_e_get(CHIP_NAME, &sec_info, vport_ids[phy_index][port][macsec_side], &secy_stat_e, 1);
        if (rv) {
            printf("FAIL: Failed to SecY statistics for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
        } else {
            printf("PASS: Successfully SecY statistics for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }

        /* Print Structure values */
        printf(" out_pkts_transform_error-lo[0x%x] , out_pkts_transform_error-hi[0x%x] \n", secy_stat_e.out_pkts_transform_error.lo ,
                secy_stat_e.out_pkts_transform_error.hi );
        printf(" out_pkts_control-lo        [0x%x] , out_pkts_control-hi        [0x%x] \n", secy_stat_e.out_pkts_control.lo , secy_stat_e.out_pkts_control.hi );
        printf(" out_pkts_untagged-lo       [0x%x] , out_pkts_untagged-hi       [0x%x] \n", secy_stat_e.out_pkts_untagged.lo , secy_stat_e.out_pkts_untagged.hi );

        /* Stats - Secy - Egress */
        sec_info.macsec_side = EGRESS;
        memset(&sa_stat_e, 0, sizeof(bcm_plp_secy_sa_stat_e_t));

        rv = bcm_plp_secy_sa_statistics_e_get(CHIP_NAME, &sec_info, secy_sahandle[sa_id + saindex][macsec_side], &sa_stat_e, 1);
        if (rv) {
            printf("FAIL: Failed to SA statistics for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
        } else {
            printf("PASS: Successfully SA statistics for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* Print Structure values */
        printf(" out_pkts_encrypted_protected-lo  [0x%x] , out_pkts_encrypted_protected-hi  [0x%x] \n", sa_stat_e.out_pkts_encrypted_protected.lo ,
                sa_stat_e.out_pkts_encrypted_protected.hi );
        printf(" out_pkts_too_long-lo             [0x%x] , out_pkts_too_long-hi             [0x%x] \n", sa_stat_e.out_pkts_too_long.lo ,
                sa_stat_e.out_pkts_too_long.hi );
        printf(" out_pkts_sa_not_inuse-lo         [0x%x] , out_pkts_sa_not_inuse-hi         [0x%x] \n", sa_stat_e.out_pkts_sa_not_inuse.lo ,
                sa_stat_e.out_pkts_sa_not_inuse.hi );
        printf(" out_octets_encrypted_protected-lo[0x%x] , out_octets_encrypted_protected-hi[0x%x] \n", sa_stat_e.out_octets_encrypted_protected.lo ,
                sa_stat_e.out_octets_encrypted_protected.hi );
    }

    /* Ingress */
    sa_id = 1;
    for(phy_index = 1; phy_index < NUM_OF_PHY; phy_index++, sa_id++)
    {
        sec_info.phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        sec_info.phy_info.if_side  = LINE_SIDE;
        sec_info.phy_info.lane_map = ALL_LANE_MAP;
        port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("PHY_ID = 0x%x\n", sec_info.phy_info.phy_addr);
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");

        /* SECY SECY Stats - Ingress */
        sec_info.macsec_side = INGRESS;
        memset(&secy_stat_i, 0, sizeof(bcm_plp_secy_secy_stat_i_t));

        rv = bcm_plp_secy_secy_statistics_i_get(CHIP_NAME, &sec_info, vport_ids[phy_index][port][macsec_side], &secy_stat_i, 1);
        if (rv) {
            printf("FAIL: Failed to SecY statistics for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
        } else {
            printf("PASS: Successfully SecY statistics for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* Print Structure values */
        printf(" in_pkts_transform_error-lo[0x%x] , in_pkts_transform_error-hi[0x%x] \n", secy_stat_i.in_pkts_transform_error.lo , 
                secy_stat_i.in_pkts_transform_error.hi );
        printf(" in_pkts_control-lo        [0x%x] , in_pkts_control-hi        [0x%x] \n", secy_stat_i.in_pkts_control.lo , secy_stat_i.in_pkts_control.hi );
        printf(" in_pkts_untagged-lo       [0x%x] , in_pkts_untagged-hi       [0x%x] \n", secy_stat_i.in_pkts_untagged.lo , secy_stat_i.in_pkts_untagged.hi );
        printf(" in_pkts_no_tag-lo         [0x%x] , in_pkts_no_tag-hi         [0x%x] \n", secy_stat_i.in_pkts_no_tag.lo , secy_stat_i.in_pkts_no_tag.hi );
        printf(" in_pkts_badtag-lo         [0x%x] , in_pkts_badtag-hi         [0x%x] \n", secy_stat_i.in_pkts_badtag.lo , secy_stat_i.in_pkts_badtag.hi );
        printf(" in_pkts_no_sci-lo         [0x%x] , in_pkts_no_sci-hi         [0x%x] \n", secy_stat_i.in_pkts_no_sci.lo , secy_stat_i.in_pkts_no_sci.hi );
        printf(" in_pkts_unknown_sci-lo    [0x%x] , in_pkts_unknown_sci-hi    [0x%x] \n", secy_stat_i.in_pkts_unknown_sci.lo , secy_stat_i.in_pkts_unknown_sci.hi );
        printf(" in_pkts_tagged_ctrl-lo    [0x%x] , in_pkts_tagged_ctrl-hi    [0x%x] \n", secy_stat_i.in_pkts_tagged_ctrl.lo , secy_stat_i.in_pkts_tagged_ctrl.hi );

        /* Stats - Secy - Ingress */
        sec_info.macsec_side = INGRESS;
        memset(&sa_stat_i, 0, sizeof(bcm_plp_secy_sa_stat_i_t));

        rv = bcm_plp_secy_sa_statistics_i_get(CHIP_NAME, &sec_info,
                secy_sahandle[sa_id + saindex][macsec_side],
                &sa_stat_i, 1);
        if (rv) {
            printf("FAIL: Failed to SA statistics for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
        } else {
            printf("PASS: Successfully SA statistics for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* Print Structure values */
        printf(" in_pkts_unchecked-lo   [0x%x] , in_pkts_unchecked-hi   [0x%x] \n", sa_stat_i.in_pkts_unchecked.lo , sa_stat_i.in_pkts_unchecked.hi );
        printf(" in_pkts_delayed-lo     [0x%x] , in_pkts_delayed-hi     [0x%x] \n", sa_stat_i.in_pkts_delayed.lo , sa_stat_i.in_pkts_delayed.hi );
        printf(" in_pkts_late-lo        [0x%x] , in_pkts_late-hi        [0x%x] \n", sa_stat_i.in_pkts_late.lo , sa_stat_i.in_pkts_late.hi );
        printf(" in_pkts_ok-lo          [0x%x] , in_pkts_ok-hi          [0x%x] \n", sa_stat_i.in_pkts_ok.lo , sa_stat_i.in_pkts_ok.hi );
        printf(" in_pkts_invalid-lo     [0x%x] , in_pkts_invalid-hi     [0x%x] \n", sa_stat_i.in_pkts_invalid.lo , sa_stat_i.in_pkts_invalid.hi );
        printf(" in_pkts_not_valid-lo   [0x%x] , in_pkts_not_valid-hi   [0x%x] \n", sa_stat_i.in_pkts_not_valid.lo , sa_stat_i.in_pkts_not_valid.hi );
        printf(" in_pkts_not_using_sa-lo[0x%x] , in_pkts_not_using_sa-hi[0x%x] \n", sa_stat_i.in_pkts_not_using_sa.lo , sa_stat_i.in_pkts_not_using_sa.hi );
        printf(" in_pkts_unused_sa-lo   [0x%x] , in_pkts_unused_sa-hi   [0x%x] \n", sa_stat_i.in_pkts_unused_sa.lo , sa_stat_i.in_pkts_unused_sa.hi );
        printf(" in_octets_decrypted-lo [0x%x] , in_octets_decrypted-hi [0x%x] \n", sa_stat_i.in_octets_decrypted.lo , sa_stat_i.in_octets_decrypted.hi );
        printf(" in_octets_validated-lo [0x%x] , in_octets_validated-hi [0x%x] \n", sa_stat_i.in_octets_validated.lo , sa_stat_i.in_octets_validated.hi );
    }
    for(phy_index = 0; phy_index < NUM_OF_PHY; phy_index++, sa_id++)
    {
        sec_info.phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        sec_info.phy_info.if_side  = LINE_SIDE;
        sec_info.phy_info.lane_map = ALL_LANE_MAP;
        memset(&chanstat, 0, sizeof(bcm_plp_cfye_statistics_channel_t));
        for(macsec_side = 0 ; macsec_side < 2 ; macsec_side ++)
        {
            sec_info.macsec_side = macsec_side;
            rv = bcm_plp_cfye_statistics_channel_get(CHIP_NAME, &sec_info, &chanstat, 1);
            if (rv) {
                printf("FAIL: Failed to CfyE statistics channel for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            } else {
                printf("PASS: Successfully  CfyE statistics channel for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
            printf(" pkts data[%d] \n", chanstat.pkts_data.lo);
            printf(" pkts ctrl[%d] \n", chanstat.pkts_ctrl.lo);
            printf(" pkts dropped[%d] \n", chanstat.pkts_dropped.lo);
        }
    }
    return rv;
}
