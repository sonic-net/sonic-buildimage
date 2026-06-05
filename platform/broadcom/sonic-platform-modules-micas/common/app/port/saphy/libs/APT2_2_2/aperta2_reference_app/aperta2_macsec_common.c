/*
 * $Copyright: (c) 2023 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

/*
 * This sample program is only a reference to demonstrate the usage of
 * macsec/ipsec APIs. This program might not work for all environments.
 */

#ifndef _APERTA2_MACSEC_COMMON_C_
#define _APERTA2_MACSEC_COMMON_C_

/* Includes */
#include <aperta2_macsec_common.h>

/* Global variables */
bcm_plp_cfye_vport_handle_t vport_handle[MAX_NUM_OF_LANES][BCM_PLP_SECY_ROLE_EGRESS_INGRESS];
bcm_plp_cfye_rule_handle_t rule_handle[MAX_NUM_OF_LANES][BCM_PLP_SECY_ROLE_EGRESS_INGRESS];
bcm_plp_secy_sa_handle_t secy_sahandle[MAX_NUM_OF_LANES][BCM_PLP_SECY_ROLE_EGRESS_INGRESS];
unsigned int vport_ids[MAX_NUM_OF_LANES][BCM_PLP_SECY_ROLE_EGRESS_INGRESS];
unsigned int mtt_idx[MAX_NUM_OF_LANES][BCM_PLP_SECY_ROLE_EGRESS_INGRESS];
bcm_plp_secy_sa_t sa_params;

static unsigned int *ingress_transform_record[MAX_NUM_OF_LANES]={NULL};
static unsigned int *egress_transform_record[MAX_NUM_OF_LANES]={NULL};

/* MACsec key */
static unsigned char macsec_key[] = {
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
unsigned char * sci_basic_transform_ingress_p = &SCI1[0];

static bcm_plp_sa_builder_params_t macsec_transform_ingress=
{
    BCM_PLP_SAB_MACSEC_FLAG_UPDATE_ENABLE | BCM_PLP_SAB_MACSEC_FLAG_SA_EXPIRED_IRQ, /* flag specifying control operation */
    BCM_PLP_SAB_DIRECTION_INGRESS,  /* Direction Ingress or egress*/
    BCM_PLP_SAB_OP_MACSEC, /* MACSEC operation type  */
    0,   /* association number  */
    macsec_key,  /* Macsec Key */
    sizeof(macsec_key), /* size of the key */
    HKEY_P,     /* h_key_p */
    NULL,     /* salt_p  */
    NULL,     /* ssci_p */
    SCI1,     /* 8 byte SCI */
    0x00000001, /* Sequence Number */
    0,          /* 64 bit sequence number high bit */
    0x80,        /* Window size or Mask */
    0,          /* ICV byte count */
};

static bcm_plp_sa_builder_params_t macsec_transform_egress=
{
    BCM_PLP_SAB_MACSEC_FLAG_UPDATE_ENABLE | BCM_PLP_SAB_MACSEC_FLAG_SA_EXPIRED_IRQ, /* flag specifying control operation */
    BCM_PLP_SAB_DIRECTION_EGRESS,  /* Direction Ingress or egress*/
    BCM_PLP_SAB_OP_MACSEC, /* MACSEC operation type  */
    0,   /* association number  */
    macsec_key,  /* Macsec Key */
    sizeof(macsec_key), /* size of the key */
    HKEY_P,     /* h_key_p */
    NULL,     /* salt_p  */
    NULL,     /* ssci_p */
    SCI1,     /* 8 byte SCI */
    0x00000000, /* Sequence Number */
    0,          /* 64 bit sequence number high bit */
    0,        /* Window size or Mask */
    0,          /* ICV byte count */
};


/*IPSec*/
unsigned short ether_type = 0x9999;
int use_replay_window = 1;
int ing_seq_num_incr_val;
int replay_window_size = 0x80;

/* Key for AES256 */
unsigned char K1[] = {
    0xc9, 0xdf, 0xdf, 0x40, 0x9f, 0x58, 0xf9, 0x2c,
    0x28, 0x92, 0xd6, 0xa5, 0x21, 0x9a, 0x22, 0x08,
    0xc1, 0x84, 0x27, 0xee, 0xc4, 0xd7, 0x1f, 0x6e,
    0x98, 0x9c, 0x06, 0xbd, 0xf5, 0x9e, 0xc9, 0x00,
};

unsigned char Host_K1[] = {
    0x6b, 0x17, 0x10, 0x0e, 0x46, 0x43, 0x07, 0x0e,
    0xa5, 0x92, 0x34, 0xab, 0x7c, 0x1f, 0xde, 0x21,
};

unsigned char EgressSalt[] = {0xdc, 0x53, 0x54, 0x69};
unsigned char IngressSalt[] = {0x9f, 0x9c, 0xa4, 0x0f};
static unsigned char SCI2[] = {
    0xfa, 0xd4, 0xd3, 0x40, 0x00, 0x00, 0x00, 0x0,
};
unsigned int ipv6_addr[4] = {0x870c0667, 0xfe5f7a08, 0x6ab85439, 0x23ef7fdd};

static bcm_plp_sa_builder_params_t ipsec_transform_ingress=
{
    BCM_PLP_SAB_IPSEC_FLAG_PAD_CHECK, /* flag specifying control operation */
    BCM_PLP_SAB_DIRECTION_INGRESS,  /* Direction Ingress or egress*/
    BCM_PLP_SAB_OP_IPSEC, /* MACSEC operation type  */
    0,   /* association number  */
    K1,  /* Macsec Key */
    32, /* size of the key */
    Host_K1,     /* h_key_p */
    IngressSalt,     /* salt_p  */
    NULL,     /* ssci_p */
    NULL,     /* 8 byte SCI */
    0, /* Sequence Number */
    0,          /* 64 bit sequence number high bit */
    REPLAY_WINDOW_SIZE,        /* Window size or Mask */
    0,          /* ICV byte count */
    0          /* SPI */
};

static bcm_plp_sa_builder_params_t ipsec_transform_egress=
{
    BCM_PLP_SAB_MACSEC_FLAG_UPDATE_ENABLE | BCM_PLP_SAB_MACSEC_FLAG_SA_EXPIRED_IRQ, /* flag specifying control operation */
    BCM_PLP_SAB_DIRECTION_EGRESS,  /* Direction Ingress or egress*/
    BCM_PLP_SAB_OP_IPSEC, /* MACSEC operation type  */
    0,   /* association number  */
    K1,  /* Macsec Key */
    32, /* size of the key */
    Host_K1,     /* h_key_p */
    IngressSalt,     /* salt_p  */
    NULL,     /* ssci_p */
    SCI2,     /* 8 byte SCI */
    0x0, /* Sequence Number */
    0,          /* 64 bit sequence number high bit */
    REPLAY_WINDOW_SIZE,        /* Window size or Mask */
    0,          /* ICV byte count */
    SPI          /* SPI */
};

/* Determines Log2(n) */
int my_log2_n(unsigned int n)
{
    return (n > 1 ? 1 + my_log2_n(n>>1):0);
}

/* lanemap_to_portindex */
int lanemap_to_portindex(unsigned int lane_map)
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

/*! \brief Initialize MACSec/IPSec 
 *
 *  Initialize CfyE and SecY in MACsec/IPSEC enable or bypass mode.
 *
 *  @param sec_info represents bcm_plp_sec phy access\n
 *
 *  @param bypass_enable represents 1: MACsec/IPSEC bypass 0: MACsec/IPSEC enabled\n
 *
 *  @return Success or Failure\n 
 */
int macsec_ipsec_initialize(bcm_plp_sec_phy_access_t sec_info, int bypass_enable)
{
    int secy_rc = 0, cfye_rc = 0;
    bcm_plp_cfye_init_t init_settings;
    bcm_plp_secy_settings_t settings;
    bcm_plp_cfye_device_limits_t device_limits;

    memset(&settings, 0, sizeof(bcm_plp_secy_settings_t));
    memset(&init_settings, 0, sizeof(bcm_plp_cfye_init_t));
    memset(&device_limits, 0, sizeof(bcm_plp_cfye_device_limits_t));

    if (bypass_enable) {
        init_settings.flow_latency_bypass = 1;
    }
    /* If any channel will be configure as IPSEC then mtt_count_frame_thr_lo = 1*/
    init_settings.mtt_count_frame_thr_lo = 1;
    cfye_rc = bcm_plp_cfye_device_init(CHIP_NAME, &sec_info, &init_settings);
    if (cfye_rc != BCM_PLP_CFYE_STATUS_OK) {
        printf("FAIL: Failed to initialize cfye device for PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", cfye_rc);
        return cfye_rc;
    } else {
        printf("PASS: Successfully initialize cfye device for PHY-%d in %s path!\n",
                sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    if(!(sec_info.phy_info.flags & BCM_PLP_WARM_BOOT)){ /* Skip if Warmboot enabled*/
        /* To check IPSEC support on given HW*/
        cfye_rc = bcm_plp_cfye_device_limits_get(CHIP_NAME, &sec_info, &device_limits);
        if (cfye_rc != BCM_PLP_CFYE_STATUS_OK) {
            printf("FAIL: Failed to get cfye device limit for PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", cfye_rc);
            return cfye_rc;
        }
        if (device_limits.fipsec) {
            printf("IPsec is supported on this HW\n MTT counts %d\n", device_limits.mtt_count);
        } else {
            printf("IPsec is not supported on this HW version.\n"); 
        }
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

/*! \brief Un-Initialize MACSec/IPSec 
 *
 *  Uninitialize CfyE and SecY in MACsec/IPSEC.
 *
 *  @param sec_info represents bcm_plp_sec phy access\n
 *
 *  @return Success or Failure\n 
 */
int macsec_ipsec_uninitialize(bcm_plp_sec_phy_access_t sec_info)
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

/*! \brief Channel configuration for IPSec 
 *
 *  Update SecY channel configuration in IPSEC.
 *
 *  @param sec_info represents bcm_plp_sec phy access\n
 *
 *  @return Success or Failure\n 
 */
int ipsec_channel_mode_config_set(bcm_plp_sec_phy_access_t sec_info)
{
    int retval = 0;
    bcm_plp_secy_channel_t channel;
    bcm_plp_secy_device_params_t sec_device_params;

    memset(&channel, 0, sizeof(bcm_plp_secy_channel_t));
    memset(&sec_device_params, 0, sizeof(bcm_plp_secy_device_params_t));

    retval = bcm_plp_secy_channel_config_get(CHIP_NAME, &sec_info, &channel);
    if (retval) {
        printf("FAIL: Failed to get channel config for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully get channel config for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    channel.fipsec = 1;
    channel.ether_type = ether_type;
    sec_device_params.params.channel_p = &channel;
    sec_device_params.params.fchannel_config = 1;

    retval = bcm_plp_secy_device_update(CHIP_NAME, &sec_info, &sec_device_params);
    if (retval) {
        printf("FAIL: Failed to update device config for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully update device config for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    return retval;
}

/*! \brief parser settings 
 *
 *  Install parsers settings for SecTAG and IPsec ports.
 *
 *  @param sec_info represents bcm_plp_sec phy access\n
 *
 *  @return Success or Failure\n 
 */
int ipsec_install_parser_settings(bcm_plp_sec_phy_access_t sec_info)
{
    int retval = 0;
    bcm_plp_cfye_device_t device_params;
    bcm_plp_cfye_header_parser_t header_params;
    bcm_plp_cfye_device_control_t device_ctrl;
    bcm_plp_cfye_ipsec_parser_t ipsec_parser_params;
    bcm_plp_cfye_egress_header_t egress_hdr_params;
    bcm_plp_cfye_device_exceptions_t dev_exceptions;

    memset(&device_params, 0, sizeof(bcm_plp_cfye_device_t));
    memset(&header_params, 0, sizeof(bcm_plp_cfye_header_parser_t));
    memset(&device_ctrl, 0, sizeof(bcm_plp_cfye_device_control_t));
    memset(&ipsec_parser_params, 0, sizeof(bcm_plp_cfye_ipsec_parser_t));
    memset(&egress_hdr_params, 0, sizeof(bcm_plp_cfye_egress_header_t));
    memset(&dev_exceptions, 0, sizeof(bcm_plp_cfye_device_exceptions_t));

    dev_exceptions.drop_action = BCM_PLP_CFYE_DROP_CRC_ERROR;
    dev_exceptions.ecc_drop_action = BCM_PLP_CFYE_DROP_INTERNAL;
    device_ctrl.fipsec = true; /* Enable IPsec Mode */
    device_ctrl.exceptions_p = &dev_exceptions;

    ipsec_parser_params.fparse_ip     = true; /* Mandatory for both the directions */
    ipsec_parser_params.fparse_udp    = true; 
    ipsec_parser_params.fparse_nat    = true;
    ipsec_parser_params.fparse_esp    = true; /* Mandatory for Ingress */
    ipsec_parser_params.fallow_fragments = false;
    ipsec_parser_params.fignore_ipv4_chksum = false;
    ipsec_parser_params.fsub_parse_ike = true;
    ipsec_parser_params.fsub_parse_nat_keepalive = false;
    ipsec_parser_params.fsub_parse_natike = true;
    ipsec_parser_params.fparse_ike    = true;
    ipsec_parser_params.fparse_natike = true;
    ipsec_parser_params.fparse_nat_keepalive = true;
    ipsec_parser_params.fverify_udp_chksum = false;
    ipsec_parser_params.fmac_da_check = false;
    ipsec_parser_params.ike_port = 0x500;
    ipsec_parser_params.nat_port = 0x4500;
    ipsec_parser_params.mac_da[0] = 0; /* Any MAC address is accepted */
    ipsec_parser_params.mac_da[1] = 0;
    ipsec_parser_params.mac_da[2] = 0;
    ipsec_parser_params.mac_da[3] = 0;
    ipsec_parser_params.mac_da[4] = 0;
    ipsec_parser_params.mac_da[5] = 0;

    switch(sec_info.macsec_side) {
        case INGRESS :
            break;
        case EGRESS:
            egress_hdr_params.egress_header_etype = ether_type;
            egress_hdr_params.fenable = true;
            header_params.egress_header_p = &egress_hdr_params;
            break;
        default:
            return -1;
    };
    header_params.ipsec_parser_p = &ipsec_parser_params;
    device_params.header_parser_p = &header_params;
    device_params.control_p = &device_ctrl;

    retval = bcm_plp_cfye_device_update(CHIP_NAME, &sec_info, &device_params);
    if(retval) {
        printf("FAIL: bcm_plp_cfye_device_update for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
        return -1;
    } else {
        printf("PASS: bcm_plp_cfye_device_update for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    }
    return retval;
}

/*! \brief Add vport  MACSec
 *
 *  Add vport in MACSec.
 *
 *  @param sec_info represents bcm_plp_sec phy access\n
 *
 *  @return Success or Failure\n 
 */
int macsec_add_vport(bcm_plp_sec_phy_access_t sec_info)
{
    int retval = 0;
    int macsec_side = sec_info.macsec_side;
    bcm_plp_cfye_vport_t vportparams;
    int port = lanemap_to_portindex(sec_info.phy_info.lane_map);

    memset(&vportparams, 0, sizeof(bcm_plp_cfye_vport_t));

    vportparams.pkt_extension = sec_info.macsec_side ? 0 : 3;
    /* Add Vport For MACSEC*/
    retval = bcm_plp_cfye_vport_add(CHIP_NAME, &sec_info, &vport_handle[port][macsec_side], &vportparams);
    if (retval) {
        printf("FAIL: Failed to add vport for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully add vport for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    vport_ids[port][macsec_side] = bcm_plp_cfye_vport_id_get(CHIP_NAME, &sec_info, vport_handle[port][macsec_side]);
    printf("PASS: Successfully got vPort ID = %d for lane-map 0x%x of PHY-%d in %s path!\n",
            vport_ids[port][macsec_side], sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    return retval;
}

/*! \brief Add vport IPSec 
 *
 *  Add vport in IPSec.
 *
 *  @param sec_info represents bcm_plp_sec phy access\n
 *
 *  @return Success or Failure\n 
 */
int ipsec_add_vport(bcm_plp_sec_phy_access_t sec_info)
{
    int retval = 0;
    int macsec_side = sec_info.macsec_side;
    bcm_plp_cfye_vport_t vportparams;
    int port = lanemap_to_portindex(sec_info.phy_info.lane_map);

    memset(&vportparams, 0, sizeof(bcm_plp_cfye_vport_t));

    vportparams.pkt_extension = sec_info.macsec_side ? 0 : 1;
    /* Add Vport For IPSEC*/
    retval = bcm_plp_cfye_channel_vport_add(CHIP_NAME, &sec_info, &vport_handle[port][macsec_side], &vportparams, BCM_PLP_CYFE_MODE_IPSEC);
    if (retval) {
        printf("FAIL: Failed to add channel vport for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully add channel vport for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    vport_ids[port][macsec_side] = bcm_plp_cfye_vport_id_get(CHIP_NAME, &sec_info, vport_handle[port][macsec_side]);
    printf("PASS: Successfully got vPort ID = %d for lane-map 0x%x of PHY-%d in %s path!\n",
            vport_ids[port][macsec_side], sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    return retval;
}

/*! \brief Add SA in MACSec 
 *
 *  Add SA in MACSec based upon policy.
 *
 *  @param sec_info represents bcm_plp_sec phy access\n
 *  @param policy represents type of policy bypass option\n
 *  @return Success or Failure\n 
 */
int macsec_add_sa_with_transform_record(bcm_plp_sec_phy_access_t sec_info, macsec_policy_bypass_option_t policy)
{
    int retval = 0;
    int macsec_side = sec_info.macsec_side;
    int port = lanemap_to_portindex(sec_info.phy_info.lane_map);
    unsigned int sa_index = 0, sc_index = 0;
    memset(&sa_params, 0, sizeof(bcm_plp_secy_sa_t));
    switch(sec_info.macsec_side) {
        case INGRESS :
            sa_params.action_type = BCM_PLP_SECY_SA_ACTION_INGRESS;
            sa_params.drop_type   = BCM_PLP_SECY_SA_DROP_INTERNAL;
            sa_params.dest_port   = BCM_PLP_SECY_PORT_CONTROLLED;
            sa_params.params.ingress.validate_frames_tagged = BCM_PLP_SECY_FRAME_VALIDATE_STRICT;
            sa_params.params.ingress.fsa_inuse = 1;
            sa_params.params.ingress.freplay_protect = 1;
            sa_params.params.ingress.sci_p = sci_basic_transform_ingress_p;
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
            if(ingress_transform_record[port] == NULL)
            {
                ingress_transform_record[port] = (unsigned int *)malloc(TRANSREC_INGRESS_SIZE * sizeof(unsigned int));
            }
            printf("seq no. for first tx_ingress is [0x%x]\n", macsec_transform_ingress.seq_num_lo);
            retval = bcm_plp_secy_build_transform_record(CHIP_NAME, &sec_info, &macsec_transform_ingress,ingress_transform_record[port]);
            if (retval) {
                printf("FAIL: Failed to build transform record for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            } else {
                printf("PASS: Successfully build transform record for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
            sa_params.transform_record_p = ingress_transform_record[port];
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
            if(egress_transform_record[port] == NULL)
            {
                egress_transform_record[port] = (unsigned int *)malloc(TRANSREC_EGRESS_SIZE * sizeof(unsigned int));
            }
            printf("seq no. for first tx_ingress is [0x%x]\n",macsec_transform_egress.seq_num_lo);
            retval = bcm_plp_secy_build_transform_record(CHIP_NAME, &sec_info, &macsec_transform_egress, egress_transform_record[port]);
            if (retval) {
                printf("FAIL: Failed to build transform record for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            } else {
                printf("PASS: Successfully build transform record for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
            sa_params.transform_record_p = egress_transform_record[port];
            break;

        default :
            break ;
    }
    retval = bcm_plp_secy_sa_add(CHIP_NAME, &sec_info,
            vport_ids[port][macsec_side],
            &secy_sahandle[port][macsec_side],
            &sa_params
            );
    if (retval) {
        printf("FAIL: Failed to add SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully add SA for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    retval = bcm_plp_secy_sa_index_get(CHIP_NAME, &sec_info, secy_sahandle[port][macsec_side], &sa_index, &sc_index);
    if (retval) {
        printf("FAIL: Failed to get SA index for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully get SA index for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        printf("SA Index %d SC Index %d\n", sa_index, sc_index);
    }
    return retval;
}

/*! \brief Add SA in IPSec 
 *
 *  Add SA in IPSec based upon policy.
 *
 *  @param sec_info represents bcm_plp_sec phy access\n
 *  @return Success or Failure\n 
 */
int ipsec_add_sa_with_transform_record(bcm_plp_sec_phy_access_t sec_info)
{
    int retval = 0;
    int macsec_side = sec_info.macsec_side;
    int port = lanemap_to_portindex(sec_info.phy_info.lane_map);
    unsigned int sa_index = 0, sc_index = 0;
    memset(&sa_params, 0, sizeof(bcm_plp_secy_sa_t));
    switch(sec_info.macsec_side) {
        case INGRESS :
            sa_params.action_type = BCM_PLP_SECY_SA_ACTION_IPSEC_INGRESS;
            sa_params.drop_type   = BCM_PLP_SECY_SA_DROP_CRC_ERROR;
            sa_params.dest_port   = BCM_PLP_SECY_PORT_CONTROLLED;
            sa_params.params.ipsec_ingress.fsa_in_use = true;
            sa_params.params.ipsec_ingress.fretain_pad = false;
            sa_params.params.ipsec_ingress.fpad_check = true;
            sa_params.params.ipsec_ingress.freplay_check = true;
            sa_params.params.ipsec_ingress.fig_hdr_insert = true;
            sa_params.params.ipsec_ingress.freplay_protect   = true;
            sa_params.params.ipsec_ingress.fconf_protect     = true;
            sa_params.params.ipsec_ingress.fpad_not_valid_drop = true;
            sa_params.params.ipsec_ingress.fpad_len_fail_drop  = true;
            sa_params.params.ipsec_ingress.fupdate_ip  = true;
            sa_params.params.ipsec_ingress.fupdate_ttl = false;
            sa_params.sa_word_count = TRANSREC_INGRESS_SIZE;
            if(ingress_transform_record[port] == NULL)
            {
                ingress_transform_record[port] = (unsigned int *)malloc(TRANSREC_INGRESS_SIZE * sizeof(unsigned int));
            }
            printf("seq no. for first tx_ingress is [0x%x]\n",ipsec_transform_ingress.seq_num_lo);
            retval = bcm_plp_secy_build_transform_record(CHIP_NAME, &sec_info, &ipsec_transform_ingress,ingress_transform_record[port]);
            if (retval) {
                printf("FAIL: Failed to build transform record for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            } else {
                printf("PASS: Successfully build transform record for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
            sa_params.transform_record_p = ingress_transform_record[port];
            break;
        case EGRESS :
            sa_params.params.ipsec_egress.frollover_mode = true;
            sa_params.params.ipsec_egress.fencr_auth = true;
            sa_params.params.ipsec_egress.crypto_alg = 7; /*5 - AES-CTR-128 7 - AES-CTR-256 */
            sa_params.params.ipsec_egress.freplay_check = false;
            sa_params.params.ipsec_egress.fig_hdr_insert = false;
            sa_params.params.ipsec_egress.fsa_in_use = true;
            sa_params.params.ipsec_egress.fupdate_udp = true;
            sa_params.params.ipsec_egress.fupdate_ip = true;
            sa_params.params.ipsec_egress.fnat_udp = false;
            sa_params.params.ipsec_egress.fouter_iphdr = true;
            sa_params.params.ipsec_egress.fconf_protect = true;
            sa_params.params.ipsec_egress.fprotect_frames = true;
            sa_params.action_type = BCM_PLP_SECY_SA_ACTION_IPSEC_EGRESS;
            sa_params.drop_type   = BCM_PLP_SECY_SA_DROP_INTERNAL;
            sa_params.dest_port   = BCM_PLP_SECY_PORT_COMMON;
            sa_params.sa_word_count = TRANSREC_EGRESS_SIZE;
            if(egress_transform_record[port] == NULL)
            {
                egress_transform_record[port] = (unsigned int *)malloc(TRANSREC_EGRESS_SIZE * sizeof(unsigned int));
            }
            printf("seq no. for first tx_ingress is [0x%x]\n",ipsec_transform_egress.seq_num_lo);
            retval = bcm_plp_secy_build_transform_record(CHIP_NAME, &sec_info, &ipsec_transform_egress, egress_transform_record[port]);
            if (retval) {
                printf("FAIL: Failed to build transform record for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
            } else {
                printf("PASS: Successfully build transform record for lane-map 0x%x of PHY-%d in %s path\n",
                        sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            }
            sa_params.transform_record_p = egress_transform_record[port];
            break;
        default :
            break ;
    }
    retval = bcm_plp_secy_sa_add(CHIP_NAME, &sec_info,
            vport_ids[port][macsec_side],
            &secy_sahandle[port][macsec_side],
            &sa_params
            );
    if (retval) {
        printf("FAIL: Failed to add SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully add SA for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    retval = bcm_plp_secy_sa_index_get(CHIP_NAME, &sec_info, secy_sahandle[port][macsec_side], &sa_index, &sc_index);
    if (retval) {
        printf("FAIL: Failed to get SA index for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully get SA index for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        printf("SA Index %d SC Index %d\n", sa_index, sc_index);
    }
    return retval;
}

/*! \brief Add rule in MACSec 
 *
 *  Add rule in MACSec based upon policy.
 *
 *  @param sec_info represents bcm_plp_sec phy access\n
 *  @param policy represents type of policy bypass option\n
 *  @return Success or Failure\n 
 */
int macsec_add_rule_policy(bcm_plp_sec_phy_access_t sec_info, macsec_policy_bypass_option_t policy)
{
    int retval = 0;
    bcm_plp_cfye_rule_t ruleparams;
    int macsec_side = sec_info.macsec_side;
    int port = lanemap_to_portindex(sec_info.phy_info.lane_map);

    memset(&ruleparams, 0, sizeof(bcm_plp_cfye_rule_t));

    ruleparams.mask.packet_type = BCM_PLP_CFYE_RULE_PKT_TYPE_MASK; /* exact match on all these fields. */
    ruleparams.key.packet_type  = BCM_PLP_CFYE_RULE_PKT_TYPE_OTHER;
    ruleparams.policy.vport_handle = vport_handle[port][macsec_side];

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
                                    vport_handle[port][macsec_side],
                                    &rule_handle[port][macsec_side],
                                    &ruleparams
                                  );
    if (retval) {
        printf("FAIL: Failed to add rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully add rule for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    retval = bcm_plp_cfye_rule_enable(CHIP_NAME, &sec_info, rule_handle[port][macsec_side], 1);
    if (retval) {
        printf("FAIL: Failed to enable rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully enable rule for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    return retval ;
}
/*! \brief Add rule in IPSec 
 *
 *  Add rule in IPSec based upon policy.
 *
 *  @param sec_info represents bcm_plp_sec phy access\n
 *  @param policy represents type of policy bypass option\n
 *  @return Success or Failure\n 
 */
int ipsec_add_rule_policy(bcm_plp_sec_phy_access_t sec_info, macsec_policy_bypass_option_t policy)
{
    int retval = 0;
    bcm_plp_cfye_rule_t ruleparams;
    bcm_plp_cfye_mtt_t  mtt_params, mtt_params_get;
    int macsec_side = sec_info.macsec_side;
    unsigned char fenabled_p;
    int port = lanemap_to_portindex(sec_info.phy_info.lane_map);

    memset(&ruleparams, 0, sizeof(bcm_plp_cfye_rule_t));
    memset(&mtt_params, 0, sizeof(bcm_plp_cfye_mtt_t));
    memset(&mtt_params_get, 0, sizeof(bcm_plp_cfye_mtt_t));

    mtt_idx[port][macsec_side] = vport_ids[port][macsec_side];
    ruleparams.policy.vport_handle = vport_handle[port][macsec_side];
    ruleparams.key.packet_type = BCM_PLP_CFYE_RULE_PKT_TYPE_IPSEC;
    ruleparams.key.num_tags = 0x00;
    ruleparams.mask.packet_type = BCM_PLP_CFYE_RULE_PKT_TYPE_MASK;
    ruleparams.mask.num_tags = BCM_PLP_CFYE_RULE_NUMTAGS_MASK;
    switch(policy) {
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
            if (sec_info.macsec_side == INGRESS) {
                /* DMAC 0x3a, 0x2a, 0x26, 0x19, 0x7f, 0x0f */
                ruleparams.data[0] = 0x19262a3a;
                ruleparams.data[1] = 0xf7f;
                /* SPI should match with Egress transform record configuration */
                ruleparams.data[2] = SPI;
                ruleparams.data[3] = mtt_idx[port][macsec_side];
                ruleparams.data_mask[0] = 0xffffffff; /* match on destination IP address. */
                ruleparams.data_mask[1] = 0x0000ffff;
                ruleparams.data_mask[2] = 0xffffffff;
                ruleparams.data_mask[3] = 0xf;
            }
            break;
        default :
            break;
    }
    retval = bcm_plp_cfye_rule_add( CHIP_NAME, &sec_info,
            vport_handle[port][macsec_side],
            &rule_handle[port][macsec_side],
            &ruleparams
            );
    if (retval) {
        printf("FAIL: Failed to add rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully add rule for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    retval = bcm_plp_cfye_rule_enable(CHIP_NAME, &sec_info, rule_handle[port][macsec_side], 1);
    if (retval) {
        printf("FAIL: Failed to enable rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully enable rule for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    if (sec_info.macsec_side == INGRESS) {
        mtt_params.key.fipv6 = true;
        mtt_params.key.tag_label1 = 0;
        mtt_params.key.tag_label2 = 0;
        mtt_params.key.fpacket_type = 0;
        mtt_params.key.fip_hdr_valid = true;
        mtt_params.ip_addr[0] = ipv6_addr[0];
        mtt_params.ip_addr[1] = ipv6_addr[1];
        mtt_params.ip_addr[2] = ipv6_addr[2];
        mtt_params.ip_addr[3] = ipv6_addr[3];
        mtt_params.mask.fipv6 = true;
        mtt_params.mask.tag_label1 = 0;
        mtt_params.mask.tag_label2 = 0;
        mtt_params.mask.fpacket_type = true;
        mtt_params.mask.fip_hdr_valid = true;
        mtt_params.ip_addr_mask[0] = 0xffffffff;
        mtt_params.ip_addr_mask[1] = 0xffffffff;
        mtt_params.ip_addr_mask[2] = 0xffffffff;
        mtt_params.ip_addr_mask[3] = 0xffffffff;

        retval = bcm_plp_cfye_mtt_update(CHIP_NAME, &sec_info, mtt_idx[port][macsec_side], &mtt_params);
        if (retval) {
            printf("FAIL: Failed to update MTT for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
        } else {
            printf("PASS: Successfully update MTT for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
        /* Read back MTT configuration */
        retval = bcm_plp_cfye_mtt_read(CHIP_NAME, &sec_info, mtt_idx[port][macsec_side], &mtt_params_get, &fenabled_p);
        if (retval) {
            printf("FAIL: Failed to MTT read for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
        } else {
            printf("PASS: Successfully MTT read for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
            printf("fipv6:%d, fenabled:%d, mtt_idx:%d\n", mtt_params_get.key.fipv6, fenabled_p, mtt_idx[port][macsec_side]);
        }

        retval = bcm_plp_cfye_mtt_entry_enable_disable(CHIP_NAME, &sec_info, mtt_idx[port][macsec_side], 1, 1);
        if (retval) {
            printf("FAIL: Failed to enable/disable MTT for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
        } else {
            printf("PASS: Successfully enable/disable MTT for lane-map 0x%x of PHY-%d in %s path\n",
                    sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
        }
    }
    return retval ;
}

/*! \brief remove rule 
 *
 *  Disable rule and remove them.
 *
 *  @param sec_info represents bcm_plp_sec phy access\n
 *  @return Success or Failure\n 
 */
int macsec_remove_rule_policy(bcm_plp_sec_phy_access_t sec_info)
{
    int retval = 0;
    int macsec_side = sec_info.macsec_side;
    int port = lanemap_to_portindex(sec_info.phy_info.lane_map);
    /*++++++++++++++++++++++++++++++++++++++++++++
      Disable all rules
      +++++++++++++++++++++++++++++++++++++++++++++*/
    retval = bcm_plp_cfye_rule_enable(CHIP_NAME, &sec_info, rule_handle[port][macsec_side], 0);
    if (retval) {
        printf("FAIL: Failed to disable rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully disable rule for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    /*++++++++++++++++++++++++++++++++++++++++++++
      remove rule
      +++++++++++++++++++++++++++++++++++++++++++++*/
    retval = bcm_plp_cfye_rule_remove(CHIP_NAME, &sec_info, rule_handle[port][macsec_side]);
    if (retval) {
        printf("FAIL: Failed to remove rule for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully remove rule for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    return retval ;
}

/*! \brief remove vport 
 *
 *  Remove vport.
 *
 *  @param sec_info represents bcm_plp_sec phy access\n
 *  @return Success or Failure\n 
 */
int macsec_remove_vport(bcm_plp_sec_phy_access_t sec_info)
{
    int retval = 0;
    int macsec_side = sec_info.macsec_side;
    int port = lanemap_to_portindex(sec_info.phy_info.lane_map);
    /*++++++++++++++++++++++++++++++++++++++++++++
      remove vPort
      +++++++++++++++++++++++++++++++++++++++++++++*/
    retval = bcm_plp_cfye_vport_remove(CHIP_NAME, &sec_info, vport_handle[port][macsec_side]);
    if (retval) {
        printf("FAIL: Failed to remove vport for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully remove vport for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    return retval ;
}

/*! \brief remove SA 
 *
 *  Remove SA.
 *
 *  @param sec_info represents bcm_plp_sec phy access\n
 *  @return Success or Failure\n 
 */

int macsec_remove_sa_with_transform_record(bcm_plp_sec_phy_access_t sec_info)
{
    int retval = 0;
    int macsec_side = sec_info.macsec_side;
    int port = lanemap_to_portindex(sec_info.phy_info.lane_map);
    /*++++++++++++++++++++++++++++++++++++++++++++
      remove SA
      +++++++++++++++++++++++++++++++++++++++++++++*/
    retval = bcm_plp_secy_sa_remove(CHIP_NAME, &sec_info, secy_sahandle[port][macsec_side]);
    if (retval) {
        printf("FAIL: Failed to remove SA for lane-map 0x%x of PHY-%d in %s path (ret = %d)!\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS", retval);
    } else {
        printf("PASS: Successfully remove SA for lane-map 0x%x of PHY-%d in %s path\n",
                sec_info.phy_info.lane_map, sec_info.phy_info.phy_addr, sec_info.macsec_side ? "INGRESS" : "EGRESS");
    }
    return retval;
}
#endif /* _APERTA2_MACSEC_COMMON_C_*/
