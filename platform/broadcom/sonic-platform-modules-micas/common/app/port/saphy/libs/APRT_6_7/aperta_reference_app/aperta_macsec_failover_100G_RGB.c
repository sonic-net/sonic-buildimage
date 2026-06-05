/*
 * $Copyright: (c) 2021 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

/*
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
 * This sample application configures the Aperta chip for 100G (4x25G)              *
 * macsec transform mode as follows:                                                *
 *   1. Connect to the board.                                                       *
 *   2. Initialize the PHY and download the firmware                                *
 *   3. Initialize MACsec in bypass disable mode                                    *
 *   4. Configure Tx/Rx polarity                                                    *
 *   5. Configure operating mode parameters for 100G RGB mode Failover              *
 *   6. Setup SA/vport for macsec transformation & macsec validation                *
 *   7. Read Registers with respect to primary and secondary ports                  *
 *   8. Switchover the port                                                         *
 *   9. Read Registers with respect to primary and secondary ports                  *
 * -------------------------------------------------------------------------------- *
 * Please Note : This sample application is only for 81343 parts.                   *
 * -------------------------------------------------------------------------------- */

/* 100G RGB PAM4-NRZ MACsec mode */
#define OP_MODE_STR    "100G RGB PAM4-NRZ MACsec mode"

/* Enable MACsec un-initialization */
#define MACSEC_UNINIT_EN

/* System side parameters */
#define SYS_PORT_SPEED          100000
#define SYS_LANE_RATE           bcmplpLaneDataRate_53P125G 
#define SYS_MODULATION          bcmplpModulationPAM4
#define SYS_FEC_MODE            bcmplpapertaRS544
#define SYS_IF_TYPE             bcm_pm_InterfaceKR
#define SYS_IF_MODE             bcm_pm_Interface_mode_IEEE
#define SYS_PORT_TYPE           bcmplpPortTypeReverseGearBox 

unsigned int sys_lane_map_list[] =  {
    0x03, 0x30
};

unsigned int sys_fo_lane_map_list[] =  {
    0xC, 0xC0
};


/* Line side parameters */
#define LINE_PORT_SPEED         100000
#define LINE_LANE_RATE          bcmpLplaneDataRate_25P78125G
#define LINE_MODULATION         bcmplpModulationNRZ
#define LINE_FEC_MODE           bcmplpapertaNoFEC
#define LINE_IF_TYPE            bcm_pm_InterfaceKR
#define LINE_IF_MODE            bcm_pm_Interface_mode_IEEE
#define LINE_PORT_TYPE          bcmplpPortTypeReverseGearBox 

unsigned int line_lane_map_list[] = {
    0x0F, 0xF0
};

static unsigned int *ingress_transform_record1=NULL;
static unsigned int *egress_transform_record1=NULL;
extern bcm_plp_cfye_vport_handle_t vport_handle[2][8][2];
extern bcm_plp_cfye_rule_handle_t  rule_handle[2][8][2];
extern bcm_plp_secy_sa_handle_t    secy_sahandle[2][8][2];
extern unsigned int vport_ids[2][8][2];


/* MACsec key */
static unsigned char K1[] = {
    0xad, 0x7a, 0x2b, 0xd0, 0x3e, 0xac, 0x83, 0x5a,
    0x6f, 0x62, 0x0f, 0xdc, 0xb5, 0x06, 0xb3, 0x45,
};

/* MACsec SCI */
static unsigned char SCI1[] = {
    0xde, 0xad, 0xbe, 0xef, 0xa5, 0x5a, 0x00, 0x00};

static unsigned char HKEY_P[]={
    0x73, 0xa2, 0x3d, 0x80, 0x12, 0x1d, 0xe2, 0xd5,
    0xa8, 0x50, 0x25, 0x3f, 0xcf, 0x43, 0x12, 0x0e,
};

static bcm_plp_sa_builder_params_t transform_ingress1=
{
    BCM_PLP_SAB_MACSEC_FLAG_UPDATE_ENABLE | BCM_PLP_SAB_MACSEC_FLAG_SA_EXPIRED_IRQ, /* flag specifying control operation */
    BCM_PLP_SAB_DIRECTION_INGRESS,  /* Direction Ingress or egress*/
    BCM_PLP_SAB_OP_MACSEC, /* MACSEC operation type  */
    0,   /* association number  */
    K1,  /* Macsec Key */
    sizeof(K1), /* size of the key */
    HKEY_P,     /* h_key_p */
    NULL,     /* salt_p  */
    NULL,     /* ssci_p */
    SCI1,     /* 8 byte SCI */
    0x00000001, /* Sequence Number */
    0,          /* 64 bit sequence number high bit */
    0x80,        /* Window size or Mask */
    0,          /* ICV byte count */
};


static bcm_plp_sa_builder_params_t transform_egress1=
{
    BCM_PLP_SAB_MACSEC_FLAG_UPDATE_ENABLE | BCM_PLP_SAB_MACSEC_FLAG_SA_EXPIRED_IRQ, /* flag specifying control operation */
    BCM_PLP_SAB_DIRECTION_EGRESS,  /* Direction Ingress or egress*/
    BCM_PLP_SAB_OP_MACSEC, /* MACSEC operation type  */
    0,   /* association number  */
    K1,  /* Macsec Key */
    sizeof(K1), /* size of the key */
    HKEY_P,     /* h_key_p */
    NULL,     /* salt_p  */
    NULL,     /* ssci_p */
    SCI1,     /* 8 byte SCI */
    0x00000000, /* Sequence Number */
    0,          /* 64 bit sequence number high bit */
    0,        /* Window size or Mask */
    0,          /* ICV byte count */
};
int macsec_flow_test(unsigned int sys_lane_map, unsigned int line_lane_map)
{
    int rv = 0, macsec_side = 0;
    bcm_plp_access_t phy_info;
    bcm_plp_sec_phy_access_t sec_info;
    bcm_plp_secy_sa_t sa_params;
    bcm_plp_cfye_rule_t ruleparams;
    bcm_plp_cfye_vport_t vportparams;
    bcm_plp_cfye_device_exceptions_t exception;
    bcm_plp_cfye_device_control_t dev_control;
    bcm_plp_cfye_device_t dev_init;
    memset(&ruleparams, 0, sizeof(ruleparams));
    memset(&sa_params, 0, sizeof(bcm_plp_secy_sa_t));
    memset(&phy_info, 0, sizeof(bcm_plp_access_t));
    memset(&sec_info,0,sizeof(bcm_plp_sec_phy_access_t));
    memset(&exception, 0, sizeof(bcm_plp_cfye_device_exceptions_t));
    memset(&dev_control, 0, sizeof(bcm_plp_cfye_device_control_t));
    memset(&dev_init, 0, sizeof(bcm_plp_cfye_device_t));
    int phy_index, port;
    /* Configure all Ports of macsec device */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
        for (macsec_side = 0 ; macsec_side < 2 ; macsec_side ++) {
            sec_info.macsec_side = macsec_side; /* macsec_side = 0 -> Egress ; macsec_side = 1 -> Ingress */
            phy_info.lane_map = macsec_side ? line_lane_map : sys_lane_map;
            port = macsec_lanemap_to_portindex(phy_info.lane_map);
            sec_info.phy_info.lane_map = phy_info.lane_map;
            memset(&vportparams, 0, sizeof(bcm_plp_cfye_vport_t));
            rv = bcm_plp_cfye_vport_add(CHIP_NAME, &sec_info, &vport_handle[phy_index][port][macsec_side],
                    &vportparams);
            if (rv) {
                printf("FAIL: Failed to add vport for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            } else {
                printf("PASS: Successfully add vport for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
            if (macsec_side)
            { /* Ingress */
                memset(&sa_params, 0, sizeof(sa_params));
                sa_params.action_type = BCM_PLP_SECY_SA_ACTION_INGRESS;
                sa_params.dest_port = BCM_PLP_SECY_SA_DROP_INTERNAL;
                sa_params.drop_type = BCM_PLP_SECY_SA_DROP_CRC_ERROR;

                sa_params.params.ingress.validate_frames_tagged = BCM_PLP_SECY_FRAME_VALIDATE_STRICT;
                sa_params.params.ingress.freplay_protect = true;
                sa_params.params.ingress.sci_p = discard_const(SCI1);
                sa_params.params.ingress.an = 0;
                sa_params.params.ingress.fallow_tagged = true;
                /* Size of the transform record in 32-bit words */
                sa_params.sa_word_count = 24;
                if(ingress_transform_record1 == NULL)
                {
                    ingress_transform_record1 = (unsigned int *)malloc(24 * sizeof(unsigned int));
                }

                printf("seq no. for first tx_ingress is [0x%x]\n",transform_ingress1.seq_num_lo);
                rv = bcm_plp_secy_build_transform_record(CHIP_NAME, &sec_info, &transform_ingress1,ingress_transform_record1);
                if (rv) {
                    printf("FAIL: Failed to build transform record for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                            sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
                } else {
                    printf("PASS: Successfully build transform record for lane-map 0x%x of PHY-%d in %s path\n",
                            sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
                }
                sa_params.transform_record_p = ingress_transform_record1;
                /* Get the vPort id from Handle */
                vport_ids[sec_info.phy_info.phy_addr][port][macsec_side] = bcm_plp_cfye_vport_id_get(CHIP_NAME, &sec_info,
                        vport_handle[phy_index][port][macsec_side]);
                printf("PASS: Successfully got vPort ID = %d for lane-map 0x%x of PHY-%d in %s path\n",
                        vport_ids[phy_index][port][macsec_side], sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            } else { /* Egress */
                memset(&sa_params, 0, sizeof(sa_params));
                sa_params.action_type = BCM_PLP_SECY_SA_ACTION_EGRESS;
                sa_params.dest_port = BCM_PLP_SECY_SA_DROP_INTERNAL;
                sa_params.drop_type = BCM_PLP_SECY_SA_DROP_CRC_ERROR;
                sa_params.params.egress.fprotect_frames = true;
                sa_params.params.egress.finclude_sci = true;
                sa_params.params.egress.fconf_protect = true;
                sa_params.params.egress.fallow_data_pkts = true;

                /* Size of the transform record in 32-bit words */
                sa_params.sa_word_count = 24;
                if(egress_transform_record1 == NULL)
                {
                    egress_transform_record1 = (unsigned int *)malloc(24 * sizeof(unsigned int));
                }
                printf("seq no. for first tx_egress is [0x%x]\n",transform_egress1.seq_num_lo);
                rv = bcm_plp_secy_build_transform_record(CHIP_NAME, &sec_info, &transform_egress1,egress_transform_record1);
                if (rv) {
                    printf("FAIL: Failed to build transform record for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                            sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
                } else {
                    printf("PASS: Successfully build transform record for lane-map 0x%x of PHY-%d in %s path\n",
                            sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
                }
                sa_params.transform_record_p = egress_transform_record1;
                /* Get the vPort id from Handle */
                vport_ids[sec_info.phy_info.phy_addr][port][macsec_side] = bcm_plp_cfye_vport_id_get(CHIP_NAME, &sec_info, vport_handle[phy_index][port][macsec_side]);
                printf("PASS: Successfully got vPort ID = %d for lane-map 0x%x of PHY-%d in %s path\n",
                        vport_ids[phy_index][port][macsec_side], sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }

            rv = bcm_plp_secy_sa_add(CHIP_NAME, &sec_info,
                    vport_ids[sec_info.phy_info.phy_addr][port][macsec_side],
                    &secy_sahandle[phy_index][port][macsec_side],
                    &sa_params);
            if (rv) {
                printf("FAIL: Failed to add SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            } else {
                printf("PASS: Successfully add SA for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
            /*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
              Rule Set
              ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
            memset(&ruleparams, 0, sizeof(bcm_plp_cfye_rule_t));
            /* Rule Add */
            ruleparams.policy.vport_handle =  vport_handle[phy_index][port][macsec_side];

            ruleparams.mask.packet_type = BCM_PLP_CFYE_RULE_PKT_TYPE_OTHER;
            ruleparams.mask.num_tags = 0x1;
            ruleparams.key.packet_type = BCM_PLP_CFYE_RULE_PKT_TYPE_OTHER;
            ruleparams.key.num_tags = 0x1;
            ruleparams.data_mask[0] = 0xffffffff; /* match on dest addr */
            ruleparams.data_mask[1] = 0x0000ffff;

            /* Set the DA */
            ruleparams.data[0] = 0x9000000;
            ruleparams.data[1] = 0x101;
            rv = bcm_plp_cfye_rule_add(CHIP_NAME, &sec_info,
                    vport_handle[phy_index][port][macsec_side],
                    &rule_handle[phy_index][port][macsec_side],
                    &ruleparams);
            if (rv) {
                printf("FAIL: Failed to add rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            } else {
                printf("PASS: Successfully add rule for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
            rv = bcm_plp_cfye_rule_enable(CHIP_NAME, &sec_info,
                    rule_handle[phy_index][port][macsec_side], true);
            if (rv) {
                printf("FAIL: Failed to enable rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", rv);
            } else {
                printf("PASS: Successfully enable rule for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
        }
    }
    return rv;
}
/* Main entry to application */
int main(int argc, char *argv[])
{
    int retval = TEST_SUCCESS;
    int p_ctxt = 5;
    int phy_index = 0;
    int macsec_side = 0;
    int lane_map_index;
    int total_lane_maps;
    int device_bypass_enable = 0;
    unsigned int sys_lane_map;
    unsigned int sys_fo_lane_map;
    unsigned int line_lane_map;
    unsigned int fw_ver;
    bcm_plp_mac_access_t mac_info;
    bcm_plp_sec_phy_access_t sec_info;
    bcm_plp_access_t phy_info;
    unsigned int fw_crc;
    bcm_plp_firmware_load_type_t firmware_load_type;
    bcm_plp_aperta_fw_init_params_t aperta_init_params;
    bcm_laneswap_map_t sys_laneswap_map, line_laneswap_map;
    bcm_plp_aperta_device_aux_modes_t aux_mode_sys_set, aux_mode_line_set;

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

    phy_info.platform_ctxt = &p_ctxt;

    /* -------------------------------------------------------------------------------- *
     * Connect to the board                                                             *
     * -------------------------------------------------------------------------------- */
    retval = device_open();
    if (retval != TEST_SUCCESS) {
        printf("FAIL: Failed to connect to the board (ret = %d)!\n", retval);
        return retval;
    }

    /* -------------------------------------------------------------------------------- *
     * Section 1: PHY, MACsec Initialization and MACsec Configuration                   *
     * -------------------------------------------------------------------------------- */

    /* -------------------------------------------------------------------------------- *
     * Initialize the PHYs and download firmware based on the test setup                *
     * -------------------------------------------------------------------------------- */
    aperta_init_params.macsec_static_bypass = 0;
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
        if (retval != TEST_SUCCESS) {
            printf("FAIL: bcm_plp_init_fw_bcast failed to initialize PHY-%d (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _aperta_init_error;
        }

        /* Read firmware info */
        retval = bcm_plp_firmware_info_get(CHIP_NAME, phy_info, &fw_ver, &fw_crc);
        if (retval != TEST_SUCCESS) {
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
            if (retval != TEST_SUCCESS) {
                printf("FAIL: MACSec Initialize for PHY-ID[%d], macsec_side[%d], return code [%d]\n",
                        phy_info.phy_addr, sec_info.macsec_side, retval);
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
    if (retval != TEST_SUCCESS) {
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
        if (retval != TEST_SUCCESS) {
            printf("FAIL: Failed to set lane swap for PHY-%d at System side (ret = %d)!\n", phy_info.phy_addr, retval);
            goto _aperta_config_error;
        } else {
            printf("PASS: Successfully configured the lane swap for PHY-%d at System side!\n", phy_info.phy_addr);
        }

        phy_info.if_side = LINE_SIDE;
        retval = bcm_plp_rxtx_laneswap_set(CHIP_NAME, phy_info, &line_laneswap_map);
        if (retval != TEST_SUCCESS) {
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
                if (retval != TEST_SUCCESS) {
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
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        phy_info.if_side  = LINE_SIDE;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            phy_info.lane_map = sys_fo_lane_map_list[lane_map_index];
            memcpy(&sec_info.phy_info, &phy_info, sizeof(bcm_plp_access_t));
            for (macsec_side = 0; macsec_side < 2; macsec_side++) {
                sec_info.macsec_side = macsec_side; /* macsec_side = 0 -> Egress ; macsec_side = 1 -> Ingress */
                retval = bcm_plp_secy_config_set(CHIP_NAME, &sec_info);
                if (retval != TEST_SUCCESS) {
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


    /* -------------------------------------------------------------------------------- *
     * It is required to configure SYSTEM side first followed by LINE side              *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            sys_lane_map  = sys_lane_map_list[lane_map_index];
            sys_fo_lane_map  = sys_fo_lane_map_list[lane_map_index];
            line_lane_map = line_lane_map_list[lane_map_index];

            /* Set mode configuration at System side */
            phy_info.lane_map = sys_lane_map;
            phy_info.if_side  = SYS_SIDE;
            aux_mode_sys_set.lane_data_rate  = SYS_LANE_RATE;
            aux_mode_sys_set.modulation_mode = SYS_MODULATION;
            aux_mode_sys_set.fec_mode_sel    = SYS_FEC_MODE;
            aux_mode_sys_set.port_type       = SYS_PORT_TYPE;
            aux_mode_sys_set.failover_config.lane_map =  sys_fo_lane_map;
            aux_mode_sys_set.failover_config.mux_location =  0; /* After Macsec*/
            retval = bcm_plp_mode_config_set(CHIP_NAME, phy_info, SYS_PORT_SPEED, SYS_IF_TYPE, REF_CLOCK, SYS_IF_MODE, (void *)&aux_mode_sys_set);
            if (retval != TEST_SUCCESS) {
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
            if (retval != TEST_SUCCESS) {
                printf("FAIL: Failed to set mode configuration parameters for lane-map 0x%x of PHY-%d at Line side (ret = %d)!\n",
                        phy_info.lane_map, phy_info.phy_addr, retval);
                goto _aperta_config_error;
            } else {
                printf("PASS: Successfully configured the mode for lane-map 0x%x of PHY-%d at Line side!\n",
                        phy_info.lane_map, phy_info.phy_addr);
            }
        }
    }

    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        sys_lane_map  = sys_lane_map_list[lane_map_index];
        line_lane_map = line_lane_map_list[lane_map_index];
        macsec_flow_test(sys_lane_map, line_lane_map);
    }
    for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
        sys_lane_map  = sys_fo_lane_map_list[lane_map_index];
        line_lane_map = line_lane_map_list[lane_map_index];
        macsec_flow_test(sys_lane_map, line_lane_map);
    }
    /* -------------------------------------------------------------------------------- *
     * Ports are setup now and ready to test with traffic with dm=00:00:00:09:01:01     *
     * -------------------------------------------------------------------------------- */
    printf("***** Send Traffic with dm=00:00:00:09:01:01\n");
    getchar();
    /* -------------------------------------------------------------------------------- *
     *                  Switchover                                                      *
     * -------------------------------------------------------------------------------- */
    for (phy_index = 0; phy_index < NUM_OF_PHY; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        for (lane_map_index = 0; lane_map_index < total_lane_maps; lane_map_index++) {
            phy_info.lane_map = sys_lane_map_list[lane_map_index];
            /* Filling phy_info */
            phy_info.if_side = SYS_SIDE;
            retval = bcm_plp_failover_mode_set(CHIP_NAME, phy_info, 1);
            if(retval) {
                printf("FAIL: bcm_plp_failover_mode_se API failed for PHY_ID[%d], LANE_MAP[0x%x], return code[%d]\n",
                        phy_info.phy_addr, phy_info.lane_map, retval);
                goto _aperta_config_error;
            }
        } 
    } 
    /* -------------------------------------------------------------------------------- *
     * Ports are setup now and ready to test with traffic with dm=00:00:00:09:01:01     *
     * -------------------------------------------------------------------------------- */
    printf("***** Send Traffic with dm=00:00:00:09:01:01\n");
    getchar();

_aperta_config_error:
_aperta_init_error:
    /* Check for test result */
    if (retval == TEST_SUCCESS) {
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
