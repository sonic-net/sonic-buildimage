/*
 * $Copyright: (c) 2021 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

/*
 * This sample program is only a reference to demonstrate the usage of
 * bcm_pm_if APIs. This program might not work for all environments.
 */

#ifndef _APERTA_COMMON_C_
#define _APERTA_COMMON_C_

/* Includes */
#include <aperta_common.h>
#if defined(USE_FTDI)
/* Global variables */
FT_HANDLE ftHandle[1] = {NULL};
#endif
bcm_plp_cfye_vport_handle_t vport_handle[2][8][2];
bcm_plp_cfye_rule_handle_t rule_handle[2][8][2];
bcm_plp_secy_sa_handle_t secy_sahandle[2][8][2];
unsigned int vport_ids[2][8][2];
bcm_plp_secy_sa_t sa_params;

int use_replay_window = 1;
int replay_window_size = 0x80;
int ing_seq_num_incr_val;

bcm_plp_sa_builder_params_t egress_basic_transform_record[MAX_SA];
bcm_plp_sa_builder_params_t ingress_basic_transform_record[MAX_SA];

unsigned int ingress_transform_record[MAX_SA][24];
unsigned int egress_transform_record[MAX_SA][24];
unsigned char keys_128[MAX_SA][16];
unsigned char keys_256[MAX_SA][32];
unsigned char hkey[MAX_SA][16];
unsigned char sci[MAX_VPORT][8];
unsigned char ssci[MAX_VPORT][4];
unsigned char salt[MAX_VPORT][12];
unsigned int seq_num_lo[MAX_SA];
unsigned int seq_num_hi[MAX_SA];

/* Configure polarity settings */
int polarity_swap(void)
{
#ifdef BCM_PLP_81384_DEFAULT_POLARITY
    int sys_phy0_rx_invert  = 0x80;
    int sys_phy0_tx_invert  = 0x80;
    int sys_phy1_rx_invert  = 0x00;
    int sys_phy1_tx_invert  = 0x80;
    int line_phy0_rx_invert = 0x80;
    int line_phy0_tx_invert = 0x80;
    int line_phy1_rx_invert = 0x80;
    int line_phy1_tx_invert = 0x80;
#else
    int sys_phy0_rx_invert  = 0xFC;
    int sys_phy0_tx_invert  = 0xFF;
    int sys_phy1_rx_invert  = 0xFF;
    int sys_phy1_tx_invert  = 0x7F;
    int line_phy0_rx_invert = 0xFF;
    int line_phy0_tx_invert = 0xFC;
    int line_phy1_rx_invert = 0x1F;
    int line_phy1_tx_invert = 0x9F;
#endif
    unsigned int tx_pol = 0, rx_pol = 0, read_tx_pol = 0, read_rx_pol = 0;
    unsigned int phy_index = 0;
    int side = 0, retval = TEST_FAILURE;
    bcm_plp_access_t phy_info;

    phy_info.platform_ctxt = &p_ctxt;

    for (phy_index = 0; phy_index <  NUM_OF_PHY ; phy_index++) {
        phy_info.phy_addr = (phy_index == 0) ? PHY_ID0 : PHY_ID1;
        phy_info.lane_map = ALL_LANE_MAP;
        for (side = 1; side >=0 ; side--) {
            phy_info.if_side = side;
            if (phy_info.if_side == SYS_SIDE) {
                if (phy_index == 0) {
                    tx_pol = sys_phy0_tx_invert;
                    rx_pol = sys_phy0_rx_invert;
                } else {
                    tx_pol = sys_phy1_tx_invert;
                    rx_pol = sys_phy1_rx_invert;
                }
            } else { /* LINE_SIDE */
                if (phy_index == 0) {
                    tx_pol = line_phy0_tx_invert;
                    rx_pol = line_phy0_rx_invert;
                } else {
                    tx_pol = line_phy1_tx_invert;
                    rx_pol = line_phy1_rx_invert;
                }
            }
            /* Reading existing polarity settings */
            retval = bcm_plp_polarity_get(CHIP_NAME, phy_info, &read_tx_pol, &read_rx_pol);
            if (retval != TEST_SUCCESS) {
                printf("FAIL: Failed to get the polarity for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, (side == 1) ? "System" : "Line", retval);
                return retval;
            } else {
                printf("PASS: Successfully got the existing polarity for lane-map 0x%x of PHY-%d at %s side! (tx_pol_rd : 0x%x, rx_pol_rd : 0x%x)\n",
                phy_info.lane_map, phy_info.phy_addr, (side == 1) ? "System" : "Line", read_tx_pol, read_rx_pol);
            }
            /* Applying polarity swap configuration according to Board design */
            read_tx_pol ^= tx_pol;
            read_rx_pol ^= rx_pol;
            retval = bcm_plp_polarity_set(CHIP_NAME, phy_info, read_tx_pol, read_rx_pol);
            if (retval != TEST_SUCCESS) {
                printf("FAIL: Failed to set the polarity for lane-map 0x%x of PHY-%d at %s side (ret = %d)!\n",
                phy_info.lane_map, phy_info.phy_addr, (side == 1) ? "System" : "Line", retval);
                return retval;
            } else {
                printf("PASS: Successfully set the polarity (tx_pol : 0x%x, rx_pol : 0x%x) for lane-map 0x%x of PHY-%d at %s side!\n",
                read_tx_pol, read_rx_pol, phy_info.lane_map, phy_info.phy_addr, (side == 1) ? "System" : "Line");
            }
        }
    }
    return retval;
}

/* Initialize CfyE and SecY in MACsec enable or bypass mode */
int macsec_initialize(bcm_plp_sec_phy_access_t sec_info, int bypass_enable)
{
    int secy_rc = 0, cfye_rc = 0;
    bcm_plp_cfye_init_t init_settings;
    bcm_plp_secy_settings_t settings;

    memset(&settings, 0, sizeof(bcm_plp_secy_settings_t));
    memset(&init_settings, 0, sizeof(bcm_plp_cfye_init_t));

    if (bypass_enable) {
        init_settings.flow_latency_bypass = 1;
    }
    cfye_rc = bcm_plp_cfye_device_init(CHIP_NAME, &sec_info, &init_settings);
    if (cfye_rc != BCM_PLP_CFYE_STATUS_OK) {
        printf("FAIL: Failed to initialize cfye device for PHY-%d in %s path (ret = %d)!\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", cfye_rc);
        return cfye_rc;
    } else {
        printf("PASS: Successfully initialize cfye device for PHY-%d in %s path!\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }

    if (bypass_enable) {
        settings.drop_bypass.fbypass = 1;
    }
    else {
        /* Uncomment the next line to modify drop_type to PKT ERROR */
        /*settings.drop_bypass.drop_type = BCM_PLP_SECY_SA_DROP_PKT_ERROR;*/
        settings.drop_bypass.drop_type = BCM_PLP_SECY_SA_DROP_INTERNAL;
    }

   /*Initialize SecY device*/
    secy_rc = bcm_plp_secy_device_init(CHIP_NAME, &sec_info, &settings);
    if (secy_rc != BCM_PLP_SECY_STATUS_OK){
        printf("FAIL: Failed to initialize secy device for PHY-%d in %s path (ret = %d)!\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", secy_rc);
        return secy_rc;
    } else {
        printf("PASS: Successfully initialized secy device for PHY-%d in %s path!\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    return (secy_rc || cfye_rc);
}

/* Uninitialize CfyE and SecY configuration */
int macsec_uninitialize(bcm_plp_sec_phy_access_t sec_info)
{
    int cfye_rc = 0, secy_rc = 0; /* Return code */
    cfye_rc = bcm_plp_cfye_device_uninit(CHIP_NAME, &sec_info);
    if(cfye_rc != BCM_PLP_CFYE_STATUS_OK) {
        printf("FAIL: Failed to uninitialize cfye device for PHY-%d in %s path (ret = %d)!\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", cfye_rc);
        return cfye_rc;
    }

    secy_rc = bcm_plp_secy_device_uninit(CHIP_NAME, &sec_info);
    if(secy_rc != BCM_PLP_SECY_STATUS_OK) {
        printf("FAIL: Failed to uninitialize secy device for PHY-%d in %s path (ret = %d)!\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", cfye_rc);
        return secy_rc;
    }
    return (secy_rc || secy_rc);
}


/* Install vPort */
int macsec_install_vport(bcm_plp_sec_phy_access_t sec_info)
{
    bcm_plp_cfye_vport_t vportparams;
    int retval = 0;
    int phy_index = sec_info.phy_info.phy_addr;
    int macsec_side = sec_info.macsec_side;
    int port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

    memset(&vportparams, 0, sizeof(bcm_plp_cfye_vport_t));

    retval = bcm_plp_cfye_vport_add(CHIP_NAME, &sec_info, &vport_handle[phy_index][port][macsec_side], &vportparams);
    if (retval){
        printf("FAIL: bcm_plp_cfye_vport_add API failed for PHY-ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] \n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        return retval;
    } else {
        printf("PASS: bcm_plp_cfye_vport_add API successful for PHY-ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] \n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
    }

    vportparams.pkt_extension = sec_info.macsec_side ? 0 : 3;
    retval = bcm_plp_cfye_vport_update(CHIP_NAME, &sec_info, vport_handle[phy_index][port][macsec_side], &vportparams);
    if (retval){
        printf("FAIL: bcm_plp_cfye_vport_update API failed for PHY-ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] \n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        return retval;
    } else {
        printf("PASS: bcm_plp_cfye_vport_update API successful for PHY-ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d] \n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
    }

    vport_ids[phy_index][port][macsec_side] = bcm_plp_cfye_vport_id_get(CHIP_NAME, &sec_info, vport_handle[phy_index][port][macsec_side]);
    printf("PASS: Successfully got vPort ID = %d for lane-map 0x%x of PHY-%d in %s path!\n",
            vport_ids[phy_index][port][macsec_side], sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");

    return retval;
}

/* Install SA with transform record */
int macsec_install_sa_with_transform_record(bcm_plp_sec_phy_access_t sec_info, macsec_policy_bypass_option_t policy)
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
int macsec_add_rule_policy(bcm_plp_sec_phy_access_t sec_info, macsec_policy_bypass_option_t policy)
{
    bcm_plp_cfye_rule_t ruleparams;
    int retval = 0;
    int phy_index = sec_info.phy_info.phy_addr;
    int macsec_side = sec_info.macsec_side;
    int port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

    memset(&ruleparams, 0, sizeof(bcm_plp_cfye_rule_t));

    ruleparams.mask.packet_type = 0x3; /* exact match on all these fields. */
    ruleparams.key.packet_type  = BCM_PLP_CFYE_RULE_PKT_TYPE_OTHER;
    ruleparams.policy.vport_handle = vport_handle[phy_index][port][macsec_side];

    switch(policy)
    {
        case POLICY_BYPASS_CONTROL_PACKET :
            ruleparams.data_mask[0]     = 0x0; /*  do not match on destination address. */
            ruleparams.data_mask[1]     = 0x0;
            ruleparams.policy.fcontrol_packet = 1;
            break;
        case POLICY_BYPASS_DATA_PACKET :
            ruleparams.data_mask[0]     = 0x0; /* do not match on destination address. */
            ruleparams.data_mask[1]     = 0x0;
            break;
        case POLICY_BYPASS_NONE :
            ruleparams.data[0]          = 0x9000000;
            ruleparams.data[1]          = 0x101;
            ruleparams.data_mask[0]     = 0xffffffff; /* match on destination address. */
            ruleparams.data_mask[1]     = 0x0000ffff;
            break;
        default :
            break;
    }
    retval = bcm_plp_cfye_rule_add( CHIP_NAME, &sec_info,
                                    vport_handle[phy_index][port][macsec_side],
                                    &rule_handle[phy_index][port][macsec_side],
                                    &ruleparams
                                  );
    if(retval) {
        printf("FAIL: bcm_plp_cfye_rule_add API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        return retval ;
    } else {
        printf("PASS : bcm_plp_cfye_rule_add API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
    }
    return retval ;
}

/* Update rule */
int macsec_update_rule(bcm_plp_sec_phy_access_t sec_info)
{
    int retval = 0;
    bcm_plp_cfye_rule_t ruleparams;
    int phy_index = sec_info.phy_info.phy_addr;
    int macsec_side = sec_info.macsec_side;
    int port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

    memset(&ruleparams, 0, sizeof(bcm_plp_cfye_rule_t));

    ruleparams.mask.packet_type = 0x3; /* exact match on all these fields. */
    ruleparams.key.packet_type  = BCM_PLP_CFYE_RULE_PKT_TYPE_OTHER;
    ruleparams.policy.vport_handle = vport_handle[phy_index][port][macsec_side];
    ruleparams.data[0] = 0x8000000;
    ruleparams.data[1] = 0x101;
    ruleparams.data_mask[0] = 0xffffffff; /* match on destination address. */
    ruleparams.data_mask[1] = 0x0000ffff;

    retval = bcm_plp_cfye_rule_update(CHIP_NAME, &sec_info, rule_handle[phy_index][port][macsec_side], &ruleparams);

    if(retval) {
        printf("FAIL: bcm_plp_cfye_rule_update API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        return retval ;
    } else {
        printf("PASS : bcm_plp_cfye_rule_update API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
    }
    return retval ;
}

/* mtu update */
int macsec_mtu_update(bcm_plp_sec_phy_access_t sec_info, int mtusize)
{
    unsigned int saindex, scindex;
    int retval = 0;
    bcm_plp_secy_sc_rule_mtu_check_t mtu_update;
    int phy_index = sec_info.phy_info.phy_addr;
    int macsec_side = sec_info.macsec_side;
    int port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

    memset(&mtu_update, 0, sizeof(bcm_plp_secy_sc_rule_mtu_check_t));

    retval = bcm_plp_secy_sa_index_get(CHIP_NAME, &sec_info, secy_sahandle[phy_index][port][macsec_side], &saindex, &scindex);
    if(retval) {
        printf("FAIL: bcm_plp_secy_sa_index_get API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        return retval;
    }

    /* MTU Update */
    mtu_update.packet_max_byte_count = mtusize;
    mtu_update.fover_size_drop = 1;

    retval = bcm_plp_secy_rules_mtucheck_update(CHIP_NAME, &sec_info, scindex, &mtu_update);
    if(retval) {
        printf("FAIL: bcm_plp_secy_rules_mtucheck_update API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        return retval;
    } else {
        printf("PASS: bcm_plp_secy_rules_mtucheck_update API successfull for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
    }
    return retval ;
}

/* macsec SA update */
int macsec_sa_allowdrop_tagged_pkts(bcm_plp_sec_phy_access_t sec_info, int allow)
{
    int retval = 0;
    int phy_index = sec_info.phy_info.phy_addr;
    int macsec_side = sec_info.macsec_side;
    int port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

    memset(&sa_params, 0, sizeof(bcm_plp_secy_sa_t));
    sa_params.action_type = BCM_PLP_SECY_SA_ACTION_INGRESS;
    sa_params.drop_type   = BCM_PLP_SECY_SA_DROP_INTERNAL;
    sa_params.dest_port   = BCM_PLP_SECY_PORT_CONTROLLED;
    sa_params.params.ingress.validate_frames_tagged = BCM_PLP_SECY_FRAME_VALIDATE_STRICT;
    sa_params.params.ingress.fsa_inuse = 1;
    sa_params.params.ingress.freplay_protect = 1;
    sa_params.params.ingress.sci_p = (unsigned char *)discard_const(&sci[port]);
    sa_params.params.ingress.an = 0; /* This should match CCW in Transform record of Egress */
    /* size of the transform record in 32-bit words */
    sa_params.sa_word_count = TRANSREC_INGRESS_SIZE;
    sa_params.transform_record_p =   &ingress_transform_record[port][0];

    if (!allow) {
            /* Update SA to now allow untagged packet, drop tagged packets */
            sa_params.params.ingress.fallow_tagged = 0;
            sa_params.params.ingress.fallow_untagged = 1;
            sa_params.params.ingress.fvalidate_untagged = 0;
    } else  {
            sa_params.params.ingress.fallow_tagged = 1;
            sa_params.params.ingress.fallow_untagged = 0;
            sa_params.params.ingress.fvalidate_untagged = 0;
    }

    retval = bcm_plp_secy_sa_update(CHIP_NAME, &sec_info, secy_sahandle[phy_index][port][macsec_side], &sa_params);
    if(retval) {
        printf("FAIL: bcm_plp_secy_sa_update API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        return retval ;
    } else {
        printf("PASS : bcm_plp_secy_sa_update API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
    }
    return retval ;
}

/* macsec window size update */
int macsec_sa_windowsize_update(bcm_plp_sec_phy_access_t sec_info, unsigned int winsize)
{
    int retval = 0;
    int phy_index = sec_info.phy_info.phy_addr;
    int macsec_side = sec_info.macsec_side;
    int port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

    retval = bcm_plp_secy_sa_window_size_update(CHIP_NAME, &sec_info, secy_sahandle[phy_index][port][macsec_side], winsize);
    if(retval) {
        printf("FAIL: bcm_plp_secy_sa_window_size_update API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        return retval;
    } else {
        printf("PASS : bcm_plp_secy_sa_window_size_update API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
    }

    return retval;
}

/* macsec remove SA*/
int macsec_sa_remove(bcm_plp_sec_phy_access_t sec_info)
{
    int retval = 0;
    int phy_index = sec_info.phy_info.phy_addr;
    int macsec_side = sec_info.macsec_side;
    int port = macsec_lanemap_to_portindex(sec_info.phy_info.lane_map);

    retval = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info, secy_sahandle[phy_index][port][macsec_side]);
    if(retval) {
        printf("FAIL: bcm_plp_secy_sa_remove API failed for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
        return retval;
    } else {
        printf("PASS : bcm_plp_secy_sa_remove API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code[%d]\n",
        sec_info.phy_info.phy_addr, sec_info.macsec_side, sec_info.phy_info.lane_map, retval);
    }
    return retval;
}

/* macsec_print_statistics*/
int macsec_print_statistics(unsigned int lane_map)
{
    unsigned int retval, phy_index, macsec_side;
    bcm_plp_sec_phy_access_t sec_info;

    int port = macsec_lanemap_to_portindex(lane_map);
    memset(&sec_info, 0, sizeof(bcm_plp_sec_phy_access_t));

    /* PHY_ID = 0 will have Egress functionality */
    for(phy_index = 0; phy_index < 1; phy_index++)
    {
        printf("\n\n\n++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("PHY_ID = 0x%x\n", phy_index);
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
        /* Filling phy_info */
        sec_info.phy_info.phy_addr = phy_index;
        sec_info.phy_info.if_side  = LINE_SIDE;
        sec_info.phy_info.lane_map = lane_map;

        printf("\n\n++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("Check For Error Packet Statistics @Egress side \n");
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");

        for(macsec_side = 0 ; macsec_side < 1 ; macsec_side ++)
        {
            sec_info.macsec_side = macsec_side;

            /* SECY SECY Stats - Ingress */
            bcm_plp_secy_secy_stat_e_t stats;
            memset(&stats, 0, sizeof(bcm_plp_secy_secy_stat_e_t));


            retval = bcm_plp_secy_secy_statistics_e_get(CHIP_NAME, &sec_info, vport_ids[phy_index][port][macsec_side], &stats, 1);
            if(retval)
            {
                printf("bcm_plp_secy_secy_statistics_e_get API FAILED for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code =[%d] \n",
                phy_index, macsec_side, sec_info.phy_info.lane_map, retval);
                return retval;
            }
            printf("bcm_plp_secy_secy_statistics_e_get API successful for PHY_ID[%d], macsec_side[%d], LANE_MAP[0x%x], return code =[%d] \n",
            phy_index, macsec_side, sec_info.phy_info.lane_map, retval);

            /* Print Structure values */
            printf(" out_pkts_transform_error-lo[0x%4x], out_pkts_transform_error-hi[0x%4x] \n", stats.out_pkts_transform_error.lo , stats.out_pkts_transform_error.hi );
            printf(" out_pkts_control-lo        [0x%4x], out_pkts_control-hi        [0x%4x] \n", stats.out_pkts_control.lo         , stats.out_pkts_control.hi         );
            printf(" out_pkts_untagged-lo       [0x%4x], out_pkts_untagged-hi       [0x%4x] \n", stats.out_pkts_untagged.lo        , stats.out_pkts_untagged.hi        );
        }

        printf("\n\n++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("Check For Encrypted Bytes and Encrypted Packets @Egress side \n");
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");

        for(macsec_side = 0 ; macsec_side < 1 ; macsec_side ++)
        {
            sec_info.macsec_side = macsec_side;
            /* Stats - Secy - Egress */
            bcm_plp_secy_sa_stat_e_t stats_e;
            memset(&stats_e, 0, sizeof(bcm_plp_secy_sa_stat_e_t));

            retval = bcm_plp_secy_sa_statistics_e_get(CHIP_NAME, &sec_info, secy_sahandle[phy_index][port][macsec_side], &stats_e, 1 );
            if(retval)
            {
                printf("bcm_plp_secy_sa_statistics_e_get API FAILED for PHY_ID[%d], macsec_side[%d], return code =[%d] \n", phy_index, macsec_side,retval);
                return retval;
            }
            printf("\nbcm_plp_secy_sa_statistics_e_get API successful for PHY_ID[%d], macsec_side[%d], return code =[%d] \n", phy_index, macsec_side,retval);

            /* Print Structure values */
            printf(" out_pkts_encrypted_protected-lo  [0x%4x] , out_pkts_encrypted_protected-hi  [0x%4x] \n", stats_e.out_pkts_encrypted_protected.lo   , stats_e.out_pkts_encrypted_protected.hi   );
            printf(" out_pkts_too_long-lo             [0x%4x] , out_pkts_too_long-hi             [0x%4x] \n", stats_e.out_pkts_too_long.lo              , stats_e.out_pkts_too_long.hi              );
            printf(" out_pkts_sa_not_inuse-lo         [0x%4x] , out_pkts_sa_not_inuse-hi         [0x%4x] \n", stats_e.out_pkts_sa_not_inuse.lo          , stats_e.out_pkts_sa_not_inuse.hi          );
            printf(" out_octets_encrypted_protected-lo[0x%4x] , out_octets_encrypted_protected-hi[0x%4x] \n", stats_e.out_octets_encrypted_protected.lo , stats_e.out_octets_encrypted_protected.hi );
        }
    }

    /* PHY_ID = 1 will have Ingress functionality */
    for(phy_index = 1 ; phy_index < 2; phy_index++)
    {
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("PHY_ID = 0x%x\n", phy_index);
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");
        /* Filling phy_info */
        sec_info.phy_info.phy_addr = phy_index;
        sec_info.phy_info.if_side  = LINE_SIDE;
        sec_info.phy_info.lane_map = lane_map;

        printf("\n\n++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("Check For Error Packet Statistics @Ingress side \n");
        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");

        for(macsec_side = 1 ; macsec_side < 2 ; macsec_side ++)
        {
            /* SECY SECY Stats - Ingress */
            bcm_plp_secy_secy_stat_i_t stats;
            memset(&stats, 0, sizeof(bcm_plp_secy_secy_stat_i_t));
            sec_info.macsec_side = macsec_side;

            retval = bcm_plp_secy_secy_statistics_i_get(CHIP_NAME, &sec_info, vport_ids[phy_index][port][macsec_side], &stats, 1  );
            if(retval)
            {
                printf("bcm_plp_secy_secy_statistics_i_get API FAILED for PHY_ID[%d], macsec_side[%d], return code =[%d] \n", phy_index, macsec_side,retval);
                return retval;
            }
            else
            printf("bcm_plp_secy_secy_statistics_i_get API successful for PHY_ID[%d], macsec_side[%d], return code =[%d] \n",phy_index, macsec_side,retval);

            /* Print Structure values */
            printf(" in_pkts_transform_error-lo[0x%4x] , in_pkts_transform_error-hi[0x%4x] \n", stats.in_pkts_transform_error.lo , stats.in_pkts_transform_error.hi );
            printf(" in_pkts_control-lo        [0x%4x] , in_pkts_control-hi        [0x%4x] \n", stats.in_pkts_control.lo         , stats.in_pkts_control.hi         );
            printf(" in_pkts_untagged-lo       [0x%4x] , in_pkts_untagged-hi       [0x%4x] \n", stats.in_pkts_untagged.lo        , stats.in_pkts_untagged.hi        );
            printf(" in_pkts_no_tag-lo         [0x%4x] , in_pkts_no_tag-hi         [0x%4x] \n", stats.in_pkts_no_tag.lo          , stats.in_pkts_no_tag.hi          );
            printf(" in_pkts_badtag-lo         [0x%4x] , in_pkts_badtag-hi         [0x%4x] \n", stats.in_pkts_badtag.lo          , stats.in_pkts_badtag.hi          );
            printf(" in_pkts_no_sci-lo         [0x%4x] , in_pkts_no_sci-hi         [0x%4x] \n", stats.in_pkts_no_sci.lo          , stats.in_pkts_no_sci.hi          );
            printf(" in_pkts_unknown_sci-lo    [0x%4x] , in_pkts_unknown_sci-hi    [0x%4x] \n", stats.in_pkts_unknown_sci.lo     , stats.in_pkts_unknown_sci.hi     );
            printf(" in_pkts_tagged_ctrl-lo    [0x%4x] , in_pkts_tagged_ctrl-hi    [0x%4x] \n", stats.in_pkts_tagged_ctrl.lo     , stats.in_pkts_tagged_ctrl.hi     );
        }
        printf("\n\n++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");
        printf("Check For Decrypted Bytes and Decrypted Packets @Ingress side \n");
        printf("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n");

        for(macsec_side = 1 ; macsec_side < 2 ; macsec_side ++)
        {
            /* Stats - Secy - Ingress */
            bcm_plp_secy_sa_stat_i_t stats_i;
            sec_info.macsec_side = macsec_side;
            memset(&stats_i, 0, sizeof(bcm_plp_secy_sa_stat_i_t));

            retval = bcm_plp_secy_sa_statistics_i_get(CHIP_NAME, &sec_info,
                                                  secy_sahandle[phy_index][port][macsec_side],
                                                  &stats_i, 1);
            if(retval)
            {
                printf("bcm_plp_secy_sa_statistics_i_get API FAILED for PHY_ID[%d], macsec_side[%d], return code =[%d] \n",phy_index, macsec_side,retval);
                return retval;
            } else
            printf("\nbcm_plp_secy_sa_statistics_i_get API successful for PHY_ID[%d], macsec_side[%d], return code =[%d] \n",phy_index, macsec_side,retval);

            /* Print Structure values */
            printf(" in_pkts_unchecked-lo   [0x%4x] , in_pkts_unchecked-hi   [0x%4x] \n", stats_i.in_pkts_unchecked.lo    , stats_i.in_pkts_unchecked.hi    );
            printf(" in_pkts_delayed-lo     [0x%4x] , in_pkts_delayed-hi     [0x%4x] \n", stats_i.in_pkts_delayed.lo      , stats_i.in_pkts_delayed.hi      );
            printf(" in_pkts_late-lo        [0x%4x] , in_pkts_late-hi        [0x%4x] \n", stats_i.in_pkts_late.lo         , stats_i.in_pkts_late.hi         );
            printf(" in_pkts_ok-lo          [0x%4x] , in_pkts_ok-hi          [0x%4x] \n", stats_i.in_pkts_ok.lo           , stats_i.in_pkts_ok.hi           );
            printf(" in_pkts_invalid-lo     [0x%4x] , in_pkts_invalid-hi     [0x%4x] \n", stats_i.in_pkts_invalid.lo      , stats_i.in_pkts_invalid.hi      );
            printf(" in_pkts_not_valid-lo   [0x%4x] , in_pkts_not_valid-hi   [0x%4x] \n", stats_i.in_pkts_not_valid.lo    , stats_i.in_pkts_not_valid.hi    );
            printf(" in_pkts_not_using_sa-lo[0x%4x] , in_pkts_not_using_sa-hi[0x%4x] \n", stats_i.in_pkts_not_using_sa.lo , stats_i.in_pkts_not_using_sa.hi );
            printf(" in_pkts_unused_sa-lo   [0x%4x] , in_pkts_unused_sa-hi   [0x%4x] \n", stats_i.in_pkts_unused_sa.lo    , stats_i.in_pkts_unused_sa.hi    );
            printf(" in_octets_decrypted-lo [0x%4x] , in_octets_decrypted-hi [0x%4x] \n", stats_i.in_octets_decrypted.lo  , stats_i.in_octets_decrypted.hi  );
            printf(" in_octets_validated-lo [0x%4x] , in_octets_validated-hi [0x%4x] \n", stats_i.in_octets_validated.lo  , stats_i.in_octets_validated.hi  );
        }
    }
    return retval;
}

/* gen_macsec_configs */
void gen_macsec_configs()
{
    int ii;
    time_t t;
    int v_cnt,s_cnt;

    srand((unsigned) time(&t));

    for(s_cnt=0; s_cnt < MAX_SA; s_cnt++) {
        seq_num_lo[s_cnt] = 0;
        seq_num_hi[s_cnt] = 0;
        for(ii = 0; ii < 16; ii++){
            keys_128[s_cnt][ii] = rand();
            hkey[s_cnt][ii] = rand();
        }
        for(ii = 0; ii < 32; ii++){
            keys_256[s_cnt][ii] = rand();
        }
    }

    for(v_cnt = 0; v_cnt < MAX_VPORT; v_cnt++) {
        for(ii = 0; ii < 8; ii++){
            sci[v_cnt][ii] = rand();
            if (ii < 4) {
                ssci[v_cnt][ii] = v_cnt + ii;
            }
        }
    }
}

/* macsec_build_tranform_record */
int macsec_build_tranform_record(bcm_plp_sec_phy_access_t sec_info, int sa_index, int vp_index)
{
    int retval = TEST_FAILURE;
    int phy_index = sec_info.phy_info.phy_addr;
    int macsec_side = sec_info.macsec_side;

    ing_seq_num_incr_val = 1;

    memset(&egress_basic_transform_record[sa_index],0,sizeof(bcm_plp_sa_builder_params_t));
    memset(&ingress_basic_transform_record[sa_index],0,sizeof(bcm_plp_sa_builder_params_t));

    ingress_basic_transform_record[sa_index].flags = (BCM_PLP_SAB_MACSEC_FLAG_UPDATE_ENABLE |
                                             BCM_PLP_SAB_MACSEC_FLAG_SA_EXPIRED_IRQ |
                                             BCM_PLP_SAB_MACSEC_FLAG_UPDATE_ENABLE);
    ingress_basic_transform_record[sa_index].direction = BCM_PLP_SAB_DIRECTION_INGRESS;
    ingress_basic_transform_record[sa_index].operation = BCM_PLP_SAB_OP_MACSEC;
    ingress_basic_transform_record[sa_index].an = 0;
    ingress_basic_transform_record[sa_index].key_p = keys_128[sa_index];
    ingress_basic_transform_record[sa_index].key_byte_count = 16;
    ingress_basic_transform_record[sa_index].h_key_p = hkey[sa_index];
    ingress_basic_transform_record[sa_index].salt_p =  salt[sa_index];
    ingress_basic_transform_record[sa_index].ssci_p = ssci[vp_index];
    ingress_basic_transform_record[sa_index].sci_p = discard_const(sci[vp_index]);
    ingress_basic_transform_record[sa_index].seq_num_lo = use_replay_window ? seq_num_lo[sa_index] +
                                                ing_seq_num_incr_val : seq_num_lo[sa_index] + 0x1;
    ingress_basic_transform_record[sa_index].seq_num_hi = seq_num_hi[sa_index];
    ingress_basic_transform_record[sa_index].window_size = use_replay_window ? replay_window_size :  0x0;
    ingress_basic_transform_record[sa_index].icv_byte_count = 0x0;
    retval = bcm_plp_secy_build_transform_record(CHIP_NAME, &sec_info, &ingress_basic_transform_record[sa_index],
                                                 &ingress_transform_record[sa_index][0]);
    if(retval)
    {
        printf("bcm_plp_secy_build_transform_record API FAILED for PHY_ID[%d], macsec_side[%d], return code =[%d] \n", phy_index, macsec_side,retval);
        return retval;
    }

    egress_basic_transform_record[sa_index].flags = (BCM_PLP_SAB_MACSEC_FLAG_UPDATE_ENABLE |
                                           BCM_PLP_SAB_MACSEC_FLAG_SA_EXPIRED_IRQ |
                                           BCM_PLP_SAB_MACSEC_FLAG_UPDATE_ENABLE);
    egress_basic_transform_record[sa_index].direction = BCM_PLP_SAB_DIRECTION_EGRESS;
    egress_basic_transform_record[sa_index].operation = BCM_PLP_SAB_OP_MACSEC;
    egress_basic_transform_record[sa_index].an = 0;
    egress_basic_transform_record[sa_index].key_p = keys_128[sa_index];
    egress_basic_transform_record[sa_index].key_byte_count = 16;
    egress_basic_transform_record[sa_index].h_key_p = hkey[sa_index];
    egress_basic_transform_record[sa_index].salt_p = salt[sa_index];
    egress_basic_transform_record[sa_index].ssci_p = ssci[vp_index];
    egress_basic_transform_record[sa_index].sci_p = discard_const(sci[vp_index]);
    egress_basic_transform_record[sa_index].seq_num_lo = seq_num_lo[sa_index];
    egress_basic_transform_record[sa_index].seq_num_hi = seq_num_hi[sa_index];
    egress_basic_transform_record[sa_index].window_size = 0x00000080;
    egress_basic_transform_record[sa_index].icv_byte_count = 0x0;
    retval = bcm_plp_secy_build_transform_record(CHIP_NAME, &sec_info, &egress_basic_transform_record[sa_index], &egress_transform_record[sa_index][0]);
    if(retval)
    {
        printf("bcm_plp_secy_build_transform_record API FAILED for PHY_ID[%d], macsec_side[%d], return code =[%d] \n", phy_index, macsec_side,retval);
        return retval;
    }

    return retval;
}

/* macsec_traffic_verify */
int macsec_traffic_verify(char *dm, int lane_map, char *expect)
{
     int retval = -1;

     printf("\n Lane-map %0x is setup now and ready to test with traffic with dm=%s",lane_map, dm);
     printf("\n Packet %s expected", expect);
     printf("\n Press ENTER after sending traffic to see statistics");
     getchar();
     retval = macsec_print_statistics(lane_map);
     if (retval)
     {
         printf("Print_statistics() observed failure. Exiting... ");
         return retval;
     }
     return retval;
}

/* Determines Log2(n) */
int my_log2_n(unsigned int n)
{
    return (n > 1 ? 1 + my_log2_n(n>>1):0);
}

/* macsec_lanemap_to_portindex */
int macsec_lanemap_to_portindex(unsigned int lane_map)
{
    return(my_log2_n(lane_map & -lane_map));
}

/* Discard Constant */
void * discard_const(const void * Ptr_p)
{
    union {
        const void * c_p;
        void * n_p;
    } conversion;

    conversion.c_p = Ptr_p;
    return conversion.n_p;
}


int synce_test(bcm_plp_access_t phy_info, unsigned int clkGenSquelchCfg, unsigned int squelchMonitorLanemap,
    unsigned int recoveredClkLane, unsigned int divider, unsigned int rclk_side, unsigned int rclk_out_pin_sel)
{
    int retval = 0;
    bcm_plp_synce_cfg_t synce_cfg;

    memset(&synce_cfg, 0, sizeof(bcm_plp_synce_cfg_t));

    synce_cfg.clkGenSquelchCfg = clkGenSquelchCfg;
    synce_cfg.squelchMonitorLanemap = squelchMonitorLanemap;
    synce_cfg.recoveredClkLane = recoveredClkLane;
    synce_cfg.divider = divider;
    synce_cfg.rclk_if_side = rclk_side;
    synce_cfg.rclk_out_pin_sel = rclk_out_pin_sel;

    /* Set SyncE configuration */
    retval = bcm_plp_synce_config_set(CHIP_NAME, phy_info, &synce_cfg);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: bcm_plp_synce_config_set for lane-map 0x%x of PHY-%d, clkGenSquelchCfg 0x%x (ret = %d)!\n",
            squelchMonitorLanemap, phy_info.phy_addr, clkGenSquelchCfg, retval);
        return retval;
    } else {
        printf("PASS: bcm_plp_synce_config_set for lane-map 0x%x of PHY-%d, clkGenSquelchCfg 0x%x (ret = %d)!\n",
            squelchMonitorLanemap, phy_info.phy_addr, clkGenSquelchCfg, retval);
    }

    memset(&synce_cfg, 0, sizeof(bcm_plp_synce_cfg_t));
    synce_cfg.rclk_out_pin_sel = rclk_out_pin_sel;

    /* Get SyncE configuration */
    retval = bcm_plp_synce_config_get(CHIP_NAME, phy_info, &synce_cfg);
    if (retval != TEST_SUCCESS) {
        printf("FAIL: bcm_plp_synce_config_get for lane-map 0x%x of PHY-%d, clkGenSquelchCfg 0x%x (ret = %d)!\n",
            squelchMonitorLanemap, phy_info.phy_addr, clkGenSquelchCfg, retval);
        return retval;
    } else {
        printf("PASS: bcm_plp_synce_config_get for lane-map 0x%x of PHY-%d, clkGenSquelchCfg 0x%x (ret = %d)!\n",
            squelchMonitorLanemap, phy_info.phy_addr, clkGenSquelchCfg, retval);
    }

    if ((clkGenSquelchCfg == bcmplpClkGenSquelchDisable) && (synce_cfg.clkGenSquelchCfg == bcmplpClkGenSquelchDisable)) {
        printf("PASS: bcm_plp_synce_config_set and bcm_plp_synce_config_get configurations are correct\n");
    } else {
        if ((synce_cfg.clkGenSquelchCfg == clkGenSquelchCfg) &&
            (synce_cfg.squelchMonitorLanemap == squelchMonitorLanemap) &&
            (synce_cfg.recoveredClkLane == recoveredClkLane) &&
            (synce_cfg.rclk_if_side == rclk_side) &&
            (synce_cfg.rclk_out_pin_sel == rclk_out_pin_sel) &&
            (synce_cfg.divider == divider)) {
            printf("PASS: bcm_plp_synce_config_set and bcm_plp_synce_config_get configurations are correct\n");
        }
        else {
            printf("Test failed - Returned values: ClkGenSquelchCfgEn:0x%x, squelchMonitorLanemap:0x%x, recoveredClkLane:0x%x, Divider:0x%x, ifside:%d, outputpin:%d\n",
                synce_cfg.clkGenSquelchCfg, synce_cfg.squelchMonitorLanemap, synce_cfg.recoveredClkLane, synce_cfg.divider, synce_cfg.rclk_if_side, synce_cfg.rclk_out_pin_sel);
            retval = -1;
        }
    }
    return retval;
}

/* Function to read data from MDIO interface */
int mdio_read(void *user_acc, unsigned int mdio_addr, unsigned int reg_addr, unsigned int *data)
{
#if defined(USE_FTDI)
    int ftStatus;
    DWORD BytesRW;
    unsigned char port_addr;
    unsigned char sbuf[6] = { 0 };
    unsigned char rbuf[2] = { 0 };
    unsigned char dev_addr = (reg_addr >> 16) & 0x1f;
    FT_HANDLE read_ftHandle;

    if (!user_acc)
        return TEST_FAILURE;

    read_ftHandle = ftHandle[0];
    port_addr = (unsigned char) mdio_addr;

    sbuf[0] = port_addr >> 1;
    sbuf[1] = ((port_addr & 1) << 7) | ((dev_addr & 0x1F) << 2) | 2;
    sbuf[2] = (reg_addr >> 8);
    sbuf[3] = (reg_addr & 0xFF);
    sbuf[4] = sbuf[0] | 0x30;
    sbuf[5] = sbuf[1];

    ftStatus = FT_Write(read_ftHandle, sbuf, sizeof(sbuf), &BytesRW);
    if (ftStatus == FT_OK) {
        ftStatus = FT_Read(read_ftHandle, rbuf, sizeof(rbuf), &BytesRW);
        if (ftStatus == FT_OK) {
            *data = rbuf[0];
            *data <<= 8;
            *data |= rbuf[1];
        }
    }
#ifdef BCM_PLP_MDIO_TRACE_READ_ENABLE
    printf( "MDIO_TRACE: READ Register Address: [0x%x] Data:[0x%x] MDIO Address: [0x%x] \n", reg_addr, *data, mdio_addr);
#endif
    return ftStatus;
#else
    /* Implement application specific 'read' mechanism */
    return BCM_PM_IF_UNAVAIL;
#endif
}

/* Function to write data to MDIO interface */
int mdio_write(void *user_acc, unsigned int mdio_addr, unsigned int reg_addr, unsigned int data)
{
#if defined(USE_FTDI)
    int ftStatus;
    DWORD BytesRW;
    unsigned char sbuf[8] = { 0 };
    unsigned char dev_addr = (reg_addr >> 16) & 0x1f;
    unsigned char port_addr;
    FT_HANDLE write_ftHandle;

    if (!user_acc)
        return TEST_FAILURE;

    write_ftHandle = ftHandle[0];

#ifdef BCM_PLP_MDIO_TRACE_WRITE_ENABLE
    printf( "MDIO_TRACE: WRITE Register Address: [0x%x] Data:[0x%x] MDIO Address: [0x%x] \n", reg_addr, data, mdio_addr);
#endif

    port_addr = (unsigned char) mdio_addr;

    data = data & 0xFFFF;
    sbuf[0] = port_addr >> 1;
    sbuf[1] = ((port_addr & 1) << 7) | ((dev_addr & 0x1F) << 2) | 2;
    sbuf[2] = (reg_addr >> 8);
    sbuf[3] = (reg_addr & 0xFF);
    sbuf[4] = sbuf[0] | 0x10;
    sbuf[5] = sbuf[1];
    sbuf[6] = (data >> 8);
    sbuf[7] = (data & 0xFF);
    ftStatus = FT_Write(write_ftHandle, sbuf, sizeof(sbuf), &BytesRW);

    return ftStatus;
#else
    /* Implement application specific 'write' mechanism */
    return BCM_PM_IF_UNAVAIL;
#endif
}

/* Function to connect to the board through the given USB Serial Number */
int device_sn_open(char *board_sn)
{
#if defined(USE_FTDI)
    DWORD NumDevs = 0;
    int i;
    int j;
    int ftStatus = FT_OK;
    int retval;
    unsigned int chip_id = 0;
    unsigned int chip_rev = 0;
    bool chip_found = false;
    FT_DEVICE_LIST_INFO_NODE *devInfo = NULL;

    ftStatus = FT_CreateDeviceInfoList(&NumDevs);
    if (ftStatus != FT_OK) {
        printf("Failed to create the USB device info list!\n");
        return ftStatus;
    }

    printf("Number of devices: %d\n", NumDevs);
    if (NumDevs > 0) {
        devInfo = (FT_DEVICE_LIST_INFO_NODE *) malloc(sizeof(FT_DEVICE_LIST_INFO_NODE) * NumDevs);
        if (devInfo == NULL) {
            printf("Failed to allocate memory for the FT Device Info node!\n");
            return TEST_FAILURE;
        }

        ftStatus = FT_GetDeviceInfoList(devInfo, &NumDevs);
        if (ftStatus != FT_OK) {
            printf("Failed to get the USB device info list!\n");
            goto _device_open_error;
        }

        printf("Ready to try %d devices\n", NumDevs);
        for (i = 0; i < NumDevs; i++) {
            printf("Device %2d : Flags = %08x, Type = %08x, ID = %08x, LocId = %08x\n",
                    i, devInfo[i].Flags, devInfo[i].Type, devInfo[i].ID, devInfo[i].LocId);
            printf("SerialNumber = %s, Description = %s\n", devInfo[i].SerialNumber, devInfo[i].Description);

            if (devInfo[i].Type == 0x8) {
                if ((strcmp(devInfo[i].SerialNumber, board_sn)) == 0) {
                    ftStatus = FT_Open(i, &ftHandle[0]);
                    if (ftStatus != FT_OK) {
                        printf("Failed to open the USB device!\n");
                        goto _device_open_error;
                    }
                    printf("Using USB device serial number %s\n", devInfo[i].SerialNumber);

                    for (j = 0; j < 32; j++) {
                        retval = mdio_read(&p_ctxt, j, APRT_FTDI_CHIP_ID_REG, &chip_id);
                        if (retval == 0 && chip_id != 0xFFFF) {
                            printf("Chip ID (USB Dev: %d, MDIO: %d): 0x%x\n", i, j, chip_id);
                            mdio_read(&p_ctxt, j, APRT_FTDI_CHIP_REV_REG, &chip_rev);
                            chip_rev &= CHIP_REV_ID_BIT_MASK;
                            printf("Chip Rev (USB Dev: %d, MDIO: %d): 0x%x\n", i, j, chip_rev);

                            /* Check for the chip ID of first chip */
                            if (chip_id == CHIP_ID0 || chip_id == CHIP_ID1) {
                                chip_found = true;
                                printf("Chip found at USB device %d and MDIO address %d\n", i, j);
                                continue;
                            }
                        }
                        if (chip_found) {
                            goto _device_open_error;
                        }
                    }
                    /* Close the device if chip is not found */
                    if (!chip_found) {
                        printf("Chip is not responding to USB!\n");
                        FT_Close(ftHandle[0]);
                        ftStatus = TEST_FAILURE;
                        goto _device_open_error;
                    }
                }
            }
        }

        /* Return with error if chip is not found */
        if (!chip_found) {
            printf("USB serial %s (for chip) is not connected to this machine!\n", USB_DEV_SERIAL_CHIP0);
            ftStatus = TEST_FAILURE;
            goto _device_open_error;
        }
    }

_device_open_error:
    if (devInfo != NULL) free(devInfo);
    return ftStatus;
#else
    /* Implement application specific 'device_sn_open' mechanism */
    return BCM_PM_IF_UNAVAIL;
#endif
}

/* connect to the board through the default USB Serial Number */
int device_open(void) {
    return  device_sn_open(USB_DEV_SERIAL_CHIP0);
}

/* Function to close the USB handlers */
int device_close(void)
{
#if defined(USE_FTDI)
    if (ftHandle[0] != NULL) {
        FT_Close(ftHandle[0]);
    }
    return TEST_SUCCESS;
#else
    /* Implement application specific 'device_close' mechanism */
    return BCM_PM_IF_UNAVAIL;
#endif
}

void reverse(char str[], int len)
{
    int first = 0;
    int last = len -1;
    char tmp;
    while (first < last)
    {
        tmp = *(str + first);
        *(str+first) = *(str+last);
        *(str+last) = tmp;

        last--;
        first++;
    }
}

char* itoa(int nn, char* str, int usebase)
{
    int i = 0;
    bool isneg = false;

    if (nn== 0)
    {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }

    if (nn < 0 && usebase == 10)
    {
        isneg = true;
        nn = -nn;
    }

    while (nn != 0)
    {
        int rem = nn % usebase;
        str[i++] = (rem > 9)? (rem-10) + 'a' : rem + '0';
        nn = nn/usebase;
    }

    if (isneg)
        str[i++] = '-';
    str[i] = '\0';

    reverse(str, i);
    return str;
}


#endif /* _APERTA_COMMON_C_ */
