/*
 * $Id: epdm_bcm_common_sec_defines.h $
 *
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

#ifndef EPDM_SEC_COMMON_H
#define EPDM_SEC_COMMON_H 

/*! Macro to select a particular bit */
#define BIT_LOCATION(_n_)    (1U << (_n_))

/*! Number of used non-control key/mask words */
#define BCM_PLP_CFYE_RULE_NON_CTRL_WORD_COUNT           5

/*! Number of ethertype values to use for comparison for MPLS packets*/
#define BCM_PLP_CFYE_MPLS_ETYPE_MAX_COUNT               4

/*! Maximum number of VLAN user priority values */
#define BCM_PLP_CFYE_VLAN_UP_MAX_COUNT                  8

/*! Maximum number of control packet matching rules using MAC
 * destination address and ether_type \n
 */
#define BCM_PLP_CFYE_MAC_DA_ET_MATCH_RULES_COUNT        8

/*! Maximum number of control packet matching rules
 * using ether_type and range of MAC destination addresses \n
 */
#define BCM_PLP_CFYE_MAC_DA_ET_RANGE_MATCH_RULES_COUNT  2

/*!
 * Device global events to use with the notifications \n
 */
/*! Statistics threshold event from TCAM Statistics Module */
#define BCM_PLP_CFYE_EVENT_STAT_TCAM_THR                1

/*! Statistics threshold event from Channel Statistics Module */
#define BCM_PLP_CFYE_EVENT_STAT_CHAN_THR                2

/*! ECC error event */
#define BCM_PLP_CFYE_EVENT_ECC_ERR                      256

/*! Statistics threshold event from MTT Statistics Module */
#define BCM_PLP_CFYE_EVENT_STAT_MTT_THR                 512

/*!
 * Device channel-specific events to use with the notifications \n
 */
/*! Input interface events: */
/*! Start-Of-Packet (SOP) is received for a busy channel event */
#define BCM_PLP_CFYE_EVENT_CHAN_ERR_SOP_WO_EOP          1

/*! End-Of-Packet (EOP) is received for an idle channel event */
#define BCM_PLP_CFYE_EVENT_CHAN_ERR_EOP_WO_SOP          2

/*! Data is received for an idle channel event */
#define BCM_PLP_CFYE_EVENT_CHAN_ERR_XFER_WO_SOP         4

/*!SOP is received in the 2nd cycle of the slot event */
#define BCM_PLP_CFYE_EVENT_CHAN_ERR_SLOT_SOP            8

/*! Channel ID is changed in the 2nd cycle of the slot event */
#define BCM_PLP_CFYE_EVENT_CHAN_ERR_SLOT_CHID           16

/*! First 2 packet words are not received back-to-back in the first slot event */
#define BCM_PLP_CFYE_EVENT_CHAN_ERR_NOT_B2B             32

/*! Other channel-specific events: */
/*! Packet classified as a control packet event */
#define BCM_PLP_CFYE_EVENT_CHAN_CTRL_PACKET             64

/*! Packet classified as a data packet event */
#define BCM_PLP_CFYE_EVENT_CHAN_DATA_PACKET             128

/*! Packet dropped as a result of classification event */
#define BCM_PLP_CFYE_EVENT_CHAN_DROP_PACKET             256

/*! Multiple TCAM rule hit for a packet event */
#define BCM_PLP_CFYE_EVENT_CHAN_TCAM_HIT_MULT           512

/*! TCAM rule not hit for a packet event */
#define BCM_PLP_CFYE_EVENT_CHAN_TCAM_MISS               1024

/*! Data is received on the input interface in the 2nd cycle of a slot,
    while the first cycle had an EOP already */
#define CFYE_EVENT_CHAN_ERR_SLOT_2PKTS                  2048

/*! MTT rule not hit for IPSec lookup */
#define CFYE_EVENT_CHAN_MTT_MISS                        4096

/*! TCAM hit with non-IPSec packet */
#define CFYE_EVENT_CHAN_TCAM_HIT_NON_CS                 8192

#define BCM_PLP_CFYE_STAT_CHAN_SUMMARY_MAX_COUNTERS     7

/*! CFYE Global AIC interrupt resources  */
#define BCM_PLP_CFYE_PHY_STAT_TCAM_THR_IRQ         0
#define BCM_PLP_CFYE_PHY_STAT_CHAN_THR_IRQ         1

/*! CFYE Channel AIC interrupt resources */
#define BCM_PLP_CFYE_CHAN_ERR_SOP_WO_EOP_IRQ       0
#define BCM_PLP_CFYE_CHAN_ERR_EOP_WO_SOP_IRQ       1
#define BCM_PLP_CFYE_CHAN_ERR_XFER_WO_SOP_IRQ      2
#define BCM_PLP_CFYE_CHAN_ERR_SLOT_SOP_IRQ         3
#define BCM_PLP_CFYE_CHAN_ERR_SLOT_CHID_IRQ        4
#define BCM_PLP_CFYE_CHAN_ERR_NOT_B2B_IRQ          5
#define BCM_PLP_CFYE_CHAN_CTRL_PKT_IRQ             6
#define BCM_PLP_CFYE_CHAN_DATA_PKT_IRQ             7
#define BCM_PLP_CFYE_CHAN_DROP_PKT_IRQ             8
#define BCM_PLP_CFYE_CHAN_TCAM_HIT_MULT_IRQ        9
#define BCM_PLP_CFYE_CHAN_TCAM_MISS_IRQ            10

/*! Status values returned by API functions */
typedef enum {
    BCM_PLP_CFYE_STATUS_OK,
    BCM_PLP_CFYE_ERROR_BAD_PARAMETER,
    BCM_PLP_CFYE_ERROR_INTERNAL,
    BCM_PLP_CFYE_ERROR_NOT_IMPLEMENTED,
    BCM_PLP_CFYE_ERROR_DEVICE_RW = 0x7fffffff
} bcm_plp_cfye_status_t;


/*! Classification device role: either Egress only, Ingress only */
typedef enum {
    BCM_PLP_CFYE_ROLE_EGRESS,
    BCM_PLP_CFYE_ROLE_INGRESS
} bcm_plp_cfye_role_t;


/*! Drop type
 *
 * bypass with CRC error \n
 * bypass with packet error \n
 * internal drop by external logic \n
 * do not drop, for debug purposes only \n
 */
typedef enum {
    BCM_PLP_CFYE_DROP_CRC_ERROR,
    BCM_PLP_CFYE_DROP_PKT_ERROR,
    BCM_PLP_CFYE_DROP_INTERNAL,
    BCM_PLP_CFYE_DO_NOT_DROP
} bcm_plp_cfye_drop_type_t;

#define BCM_PLP_CFYE_RULE_PKT_TYPE_MASK 0x7 /**< Packet type mask in TCAM rule */
#define BCM_PLP_CFYE_RULE_NUMTAGS_MASK 0x7f /**< NumTags field mask in TCAM rule */
#define BCM_PLP_CFYE_RULE_CHANNEL_ID_MASK 0x3f /**< Channel ID field mask in TCAM rule */

/*! Packet types \n
 * 0 - Any data packet \n
 * 1 - MPLS packet \n
 * 2 - PBB packet (Applicable to non-IPSec supported PHYs)\n
 * 3 - MACSEC packet (Applicable to non-IPSec supported PHYs)\n
 * 4 - Reserved \n
 * 5 - Packet Type IPsec with MPLS \n
 * 6 - Packet Type IPsec with UDP \n
 * 7 - Packet Type IPsec \n
 */
typedef enum {
    BCM_PLP_CFYE_RULE_PKT_TYPE_OTHER  = 0,
    BCM_PLP_CFYE_RULE_PKT_TYPE_MPLS   = 1,
    BCM_PLP_CFYE_RULE_PKT_TYPE_PBB    = 2,
    BCM_PLP_CFYE_RULE_PKT_TYPE_MACSEC = 3,
    BCM_PLP_CFYE_RULE_PKT_TYPE_RSVD =   4,
    BCM_PLP_CFYE_RULE_PKT_TYPE_IPSEC_MPLS = 5,
    BCM_PLP_CFYE_RULE_PKT_TYPE_IPSEC_UDP  = 6,
    BCM_PLP_CFYE_RULE_PKT_TYPE_IPSEC  = 7
} bcm_plp_cfye_rule_packet_type_t;


/*! Statistics counter */
typedef struct {
    unsigned int lo;
    unsigned int hi;
} bcm_plp_cfye_stat_counter_t;

/*!----------------------------------------------------------------------------
 * bcm_plp_cfye_rule_handle_t \n
 *
 * This handle is a reference to a rule.\n
 */
typedef void * bcm_plp_cfye_rule_handle_t;

/*!----------------------------------------------------------------------------
 * bcm_plp_cfye_vport_handle_t \n
 *
 * This handle is a reference to a vPort.\n
 */
typedef void * bcm_plp_cfye_vport_handle_t;

/*! MACsec SecTAG parser control
 *
 * \arg fcomp_type \n
 *     true - compare Ether-type in packet against mac_sec_tag_value value \n
 *
 * \arg fcheck_version \n
 *     true - check V (Version) bit to be 0 \n
 *
 * \arg fcheck_kay \n
 *     true - check if packet is meant to be handled by KaY (C & E bits) \n
 *
 * \arg flookup_use_sci \n
 *     This flag controls key generation for packets with SecTAG after MAC SA. \n
 *     true - enable generation of rule key with packet type MACsec (TCI, SCI) \n
 *     false - always generate normal rule keys \n
 *
 * \arg mac_sec_tag_value \n
 *     Ether-type value used for MACsec tag type comparison \n
 */
typedef struct {
    unsigned char fcomp_type;
    unsigned char fcheck_version;
    unsigned char fcheck_kay;
    unsigned char flookup_use_sci;
    unsigned short int mac_sec_tag_value;
} bcm_plp_cfye_sectag_parser_t;

/*! Egress header control */
typedef struct {
    unsigned char fenable;
    unsigned short egress_header_etype;
} bcm_plp_cfye_egress_header_t;

/*! MPLS label select for packets with 2 labels */
typedef enum {
    BCM_PLP_CFYE_MPLS3_SELECT_LABEL1 = 0,
    BCM_PLP_CFYE_MPLS3_SELECT_LABEL2,
    BCM_PLP_CFYE_MPLS3_SELECT_LABEL3
} bcm_plp_cfye_mpls3_select_t;

/*! MPLS label select for packets with 4 labels */
typedef enum {
    BCM_PLP_CFYE_MPLS4_SELECT_LABEL1 = 0,
    BCM_PLP_CFYE_MPLS4_SELECT_LABEL2,
    BCM_PLP_CFYE_MPLS4_SELECT_LABEL3,
    BCM_PLP_CFYE_MPLS4_SELECT_LABEL4
} bcm_plp_cfye_mpls4_select_t;

/*! MPLS label select for packets with >= 5 labels */
typedef enum {
    BCM_PLP_CFYE_MPLS5_SELECT_LABEL1 = 0,
    BCM_PLP_CFYE_MPLS5_SELECT_LABEL2,
    BCM_PLP_CFYE_MPLS5_SELECT_LABEL3,
    BCM_PLP_CFYE_MPLS5_SELECT_LABEL4,
    BCM_PLP_CFYE_MPLS5_SELECT_LABEL5
} bcm_plp_cfye_mpls5_select_t;

/*! MPLS ethertype compare value
 *
 * \arg fcompare \n
 *     true - use mpls_etype value for comparison with ethertype filed in an MPLS packet \n
 *
 * \arg mpls_etype \n
 *     Value of ethertype field to compare with ethertype in an MPLS packet \n
 */

typedef struct {
    unsigned char fcompare;
    unsigned short mpls_etype;
} bcm_plp_cfye_mpls_etype_t;

/*! MPLS parsing control
 *
 * \arg mpls3_select1 \n
 * \arg mpls3_select2 \n
 *      MPLS packet with 3 labels: select 2 labels to use in TCAM key \n
 *      1st label select and 2nd label select \n
 * \arg mpls4_select1 \n
 * \arg mpls4_select2 \n
 *      MPLS packet with 4 labels: select 2 labels to use in TCAM key \n
 *      1st label select and 2nd label select \n
 *
 * \arg mpls5_select1 \n
 * \arg mpls5_select2 \n
 *      MPLS packet with 5 labels: select 2 labels to use in TCAM key \n
 *      1st label select and 2nd label select \n
 * \arg mpls_etype \n
 *      Ethertype values (max BCM_PLP_ CFYE_MPLS_ETYPE_MAX_COUNT) to use for \n
 *      comparison with ethertype field in MPLS packets \n
 */
typedef struct {
    bcm_plp_cfye_mpls3_select_t mpls3_select1;
    bcm_plp_cfye_mpls3_select_t mpls3_select2;
    bcm_plp_cfye_mpls4_select_t mpls4_select1;
    bcm_plp_cfye_mpls4_select_t mpls4_select2;
    bcm_plp_cfye_mpls5_select_t mpls5_select1;
    bcm_plp_cfye_mpls5_select_t mpls5_select2;
    bcm_plp_cfye_mpls_etype_t mpls_etype[BCM_PLP_CFYE_MPLS_ETYPE_MAX_COUNT];
} bcm_plp_cfye_mpls_parser_t;

/*! PBB ethertype compare value
 *
 * \arg btag_etype \n
 *      Value of ethertype field to compare with "BVID" ethertype in an PBB packet, \n
 *      global for all channels \n
 *
 * \arg itag_etype \n
 *      Value of ethertype field to compare with "ISID" ethertype in an PBB packet \n
 *      global for all channels \n
 */
typedef struct {
    unsigned short btag_etype;
    unsigned short itag_etype;
} bcm_plp_cfye_pbb_etype_t;

/*! PBB parsing control
 *
 * \arg fcompare \n
 *      true - use PBB header parsing for a specific channel, \n
 *             btag_etype and itag_etype values for comparison with ethertype \n
 *             fields in an PBB packet \n
 *
 * \arg pbb_etype \n
 *      PBB Etype values for all channels. Set to NULL if no update is required \n
 */
typedef struct {
    unsigned char fcompare;
    bcm_plp_cfye_pbb_etype_t pbb_etype;
} bcm_plp_cfye_pbb_parser_t;

/*! VLAN tags parsing control
 *
 * \arg fparse_qtag \n
 *      Enable detection of VLAN tags matching qtag value  \n
 *
 * \arg fparse_stag1 \n
 *      Enable detection of VLAN tags matching stag1 value \n
 *
 * \arg fparse_stag2 \n
 *      Enable detection of VLAN tags matching stag2 value \n
 *
 * \arg fparse_stag3 \n
 *      Enable detection of VLAN tags matching stag3 value \n
 *
 * \arg fparse_qinq \n
 *      Enable detection of multiple back-2-back VLAN tags (Q-in-Q and beyond) \n
 */
typedef struct {
    unsigned char fparse_qtag;
    unsigned char fparse_stag1;
    unsigned char fparse_stag2;
    unsigned char fparse_stag3;
    unsigned char fparse_qinq;
} bcm_plp_cfye_vlan_parse_tag_t;

/*! VLAN parsing control
 * The classification engine is able to detect up to five VLAN tags following \n
 * the outer MAC_SA and parse the first two for packet classification \n
 *
 * \arg cp \n
 *     1. Per-channel VLAN parsing settings \n
 *     Input header parser VLAN tags parsing settings \n
 *
 * \arg fscp \n
 *     true - update the secondary header parser VLAN tags parsing settings \n
 *            with values from the SCP data structure member \n
 *     false - no update is required \n
 *
 * \arg scp \n
 *     Secondary header parser VLAN tags parsing settings \n
 *
 * \arg fstag_up_enable \n
 *     Enable user priority processing for 802.1s packets. If set, \n
 *     when a 802.1s tag is found, take the user priority directly from \n
 *     the PCP field, otherwise take the default user priority.  \n
 *
 * \arg fqtag_up_enable \n
 *     Enable user priority processing for 802.1Q packets. If set, \n
 *     when a 802.1Q tag is found, translate the PCP field using uptable1, \n
 *     otherwise take the default user priority.  \n
 *
 * \arg default_up \n
 *     Default user priority, assigned to non-VLAN packets and \n
 *     to VLAN packets for which the VLAN user priority processing is disabled \n
 *     Allowed values in range 0 .. 7! \n
 *
 * \arg uptable1 \n
 *     Translation tables to derive the user priority from the PCP field \n
 *     in ~S802.1Q~T tags. If the PCP field is 0 then take UpTable[0] as the user \n
 *     priority, if PCP=1 then take UpTable[1], etc. \n
 *     Translation table for 1st 802.1Q tag, allowed values in range 0 .. 7! \n
 *
 * \arg uptable2 \n
 *     Translation table for 2nd 802.1Q tag, allowed values in range 0 .. 7! \n
 *  \n
 * \arg qtag \n
 *     Ethertype value used for 802.1Q tag type comparison \n
 *
 * \arg stag1 \n
 * \arg stag2 \n
 * \arg stag3 \n
 *     Ethertype value used for 802.1s tag type comparison \n
 */

typedef struct {
    bcm_plp_cfye_vlan_parse_tag_t cp;
    unsigned char fscp;
    bcm_plp_cfye_vlan_parse_tag_t scp;
    unsigned char fstag_up_enable;
    unsigned char fqtag_up_enable;
    unsigned char default_up;
    unsigned char uptable1[BCM_PLP_CFYE_VLAN_UP_MAX_COUNT];
    unsigned char uptable2[BCM_PLP_CFYE_VLAN_UP_MAX_COUNT];
    unsigned short qtag;
    unsigned short stag1;
    unsigned short stag2;
    unsigned short stag3;
} bcm_plp_cfye_vlan_parser_t;

/*! L4  parser configuration
 *
 * \arg vxlan_port1 \n
 *     VxLAN UDP port 1 \n
 *
 * \arg vxlan_port2 \n
 *     VxLAN UDP port 2 \n
 */
typedef struct
{
    unsigned short vxlan_port1;
    unsigned short vxlan_port2;
} bcm_plp_cfye_l4_ports_t;

/** Various flags to set in bcm_plp_cfye_channel_l3l4_t */
/** Parse IP datagrams */
#define BCM_PLP_CFYE_VXL_FLAG_PARSE_IP        BIT_LOCATION(0)
/** Parse UDP datagrams */
#define BCM_PLP_CFYE_VXL_FLAG_PARSE_UDP       BIT_LOCATION(1)
/** Parse VXLAN Port 1 */
#define BCM_PLP_CFYE_VXL_FLAG_PARSE_VXLAN1    BIT_LOCATION(20)
/** Parse VXLAN Port 2 */
#define BCM_PLP_CFYE_VXL_FLAG_PARSE_VXLAN2    BIT_LOCATION(21)

/*! Per-channel L3/L4 header parser configuration
 *
 * \arg flags \n
 *     Logical or of CFYE_*_FLAG_* values, specify per-channel header parsing \n
 *
 */
typedef struct
{
    unsigned int flags;
} bcm_plp_cfye_channel_l3l4_t;

/*! IPsec specific parser configuration
 *
 * \arg fparse_ip \n
 *     Enable outer IP header parsing \n
 *
 * \arg fparse_udp \n
 *     Enable UDP parsing after outer IP header \n
 *
 * \arg fparse_nat \n
 *     Enable parsing of UDP encapsulated packets \n
 *
 * \arg fparse_esp \n
 *     Enable detection of ESP header \n
 *
 * \arg fparse_ike \n
 *     Enable detection and bypassing of IKE v1/v2 header \n
 *
 * \arg fparse_natike \n
 *     Enable detection and bypassing of NAT-T IKE \n
 *
 * \arg fparse_nat_keepalive \n
 *     Enable detection and bypassing of NAT Keepalive \n
 *
 * \arg fverify_udp_chksum \n
 *     Ingress-only field: Enable detection and bypassing of UDP headers that have non-zero checksum \n
 *
 * \arg fmac_da_check \n
 *     Ingress-only field: Enable MAC DA check for IPsec processing \n
 *
 * \arg fallow_fragments \n
 *     Ingress-only field: allow fragments \n
 *
 * \arg fignore_ipv4_chksum \n
 *     Ingress-only field: ignore IPv5 Checksum \n
 *
 * \arg fsub_parse_ike \n
 *     Ingress-only field: mark IKE packets as sub-control packets \n
 *
 * \arg fsub_parse_natike \n
 *     Ingress-only field: mark NAT-T IKE packets as sub-control packets \n
 *
 * \arg fsub_parse_nat_keepalive \n
 *     Ingress-only field: mark NAT-T Keep Alive packets as sub-control packets \n
 *
 * \arg ike_port \n
 *     Default IKE port \n
 *
 * \arg nat_port \n
 *     Default NAT port \n
 *
 * \arg mac_da \n
 *     MAC destination address, 6 bytes \n
 */
typedef struct
{
    unsigned char fparse_ip;
    unsigned char fparse_udp;
    unsigned char fparse_nat;
    unsigned char fparse_esp;
    unsigned char fparse_ike;
    unsigned char fparse_natike;
    unsigned char fparse_nat_keepalive;
    unsigned char fverify_udp_chksum;
    unsigned char fmac_da_check;
    unsigned char fallow_fragments;
    unsigned char fignore_ipv4_chksum;
    unsigned char fsub_parse_ike;
    unsigned char fsub_parse_natike;
    unsigned char fsub_parse_nat_keepalive;
    /** Global Controls */
    unsigned short ike_port;
    unsigned short nat_port;
    unsigned char mac_da[6];
} bcm_plp_cfye_ipsec_parser_t;

/*! Header Parser control
 *
 * \arg egress_header_p \n
 *     Egress header settings. Set to NULL if no update is required \n
 *
 * \arg sectag_parser_p \n
 *     SecTAG parser settings. Set to NULL if no update is required \n
 *   \n
 * \arg mpls_parser_p \n
 *     MPSL parser settings. Set to NULL if no update is required \n
 *
 * \arg pbb_parser_p \n
 *     PBB parser settings. Set to NULL if no update is required \n
 *
 * \arg vlan_parser_p \n
 *     VLAN parser settings. Set to NULL if no update is required \n
 *
 * \arg l4_ports_p \n
 *     L4 port settings, set to NULL if no update is required. \n
 *
 * \arg ipsec_parser_p \n
 *     IPSec parser settings, set to NULL if no update is required. \n
 *
 */
typedef struct {
    bcm_plp_cfye_egress_header_t * egress_header_p;
    bcm_plp_cfye_sectag_parser_t * sectag_parser_p;
    bcm_plp_cfye_mpls_parser_t * mpls_parser_p;
    bcm_plp_cfye_pbb_parser_t * pbb_parser_p;
    bcm_plp_cfye_vlan_parser_t * vlan_parser_p;
    bcm_plp_cfye_l4_ports_t *l4_ports_p;
    bcm_plp_cfye_ipsec_parser_t *ipsec_parser_p;
} bcm_plp_cfye_header_parser_t;

/*! Packet match rule using MAC destination address and ether_type
 *
 * \arg mac_da_p \n
 *     MAC destination address, 6 bytes, Set to NULL if not used \n
 *
 * \arg ether_type \n
 *     ether_type \n
 */
typedef struct {
    unsigned char * mac_da_p;
    unsigned short int ether_type;
} bcm_plp_cfye_mac_da_et_match_rule_t;

/*! Packet match rule using range of MAC destination addresses
 *
 * \arg mac_da_start_p \n
 *     Start MAC destination address, 6 bytes, Set to NULL if not used \n
 *
 * \arg mac_da_end_p \n
 *     End MAC destination address, 6 bytes, Only applicable if mac_da_start_p is not NULL \n
 */
typedef struct {
    unsigned char * mac_da_start_p;
    unsigned char * mac_da_end_p;
} bcm_plp_cfye_mac_da_range_match_rule_t;

/*! Packet match rule using range of MAC destination addresses and ether_type
 *
 * \arg range \n
 *      Range of MAC destination addresses, start and end \n
 *
 * \arg ether_type \n
 *      ether_type \n
 */
typedef struct {
    bcm_plp_cfye_mac_da_range_match_rule_t range;
    unsigned short int ether_type;
} bcm_plp_cfye_mac_da_et_range_match_rule_t;

/*! Control Packet Detector settings
 *
 * \arg mac_da_et_rules \n
 *     MAC destination address and ether_type \n
 *     Set mac_da_et_rules[n].mac_da_p to NULL if no update is required, \n
 *     corresponding bits in cp_match_enable_mask and Scp_match_enable_mask should be set to 0 \n
 *
 * \arg mac_da_et_range \n
 *     Range of MAC destination address used with ether_type \n
 *     Set mac_da_et_range[n].Range.mac_da_start_p and \n
 *     mac_da_et_range[n].Range.mac_da_end_p to NULL if no update is required, \n
 *     corresponding bits in cp_match_enable_mask and Scp_match_enable_mask should be set to 0 \n
 *
 * \arg mac_da_range \n
 *     Range of MAC destination address (no ether_type) \n
 *     Set mac_da_range[n].mac_da_start_p and mac_da_range[n].mac_da_end_p to NULL \n
 *     if no update is required, corresponding bits in cp_match_enable_mask and \n
 *     Scp_match_enable_mask should be set to 0 \n
 *
 * \arg mac_da_44bit_const_p \n
 *     44-bit MAC destination address constant, 6 bytes \n
 *     Set to NULL if no updated is required, corresponding bits in \n
 *     cp_match_enable_mask and Scp_match_enable_mask should be set to 0 \n
 *
 * \arg mac_da_48bit_const_p \n
 *     48-bit MAC destination address constant, 6 bytes \n
 *     Set to NULL if no updated is required \n
 *     Set to NULL if no updated is required, corresponding bits in \n
 *     cp_match_enable_mask and Scp_match_enable_mask should be set to 0 \n
 * \arg cp_match_mode_mask \n
 * \arg scp_match_mode_mask \n
 *     Match mask used to select which ether_type to compare with the ether_type \n
 *     value in the matching rule \n
 *     bit i (0-7): 1 enables packet MAC destination address comparison with \n
 *                  the respective mac_da_et_rules[i].ether_type address \n
 *     bit j (8-9): 1 enables packet MAC destination address comparison with \n
 *                  the respective mac_da_et_range[j-8].ether_type address \n
 *     Primary Control Packet Detector \n
 *     Secondary Control Packet Detector \n
 *
 * \arg cp_match_enable_mask \n
 * \arg scp_match_enable_mask \n
 *     Match mask which can be used to enable/disable control packet matching rules \n
 *     bit i (0-7): 1 enables packet MAC destination address comparison with \n
 *                 the respective mac_da_et_rules[i].mac_da_p address \n
 *     bit j (8-15): 1 enables packet Ethernet Type value comparison with \n
 *                  the respective mac_da_et_rules[j-8].ether_type value \n
 *     bit 16: 1 enables range of packet MAC destination address as well as \n
 *             Ethernet Type value comparison with the respective \n
 *             mac_da_et_range[0].Range.mac_da_start_p and \n
 *             mac_da_et_range[0].Range.mac_da_end_p values as well as \n
 *             mac_da_et_range[0].ether_type value \n
 *     bit 17: 1 enables range of packet MAC destination address as well as \n
 *             Ethernet Type value comparison with the respective \n
 *             mac_da_et_range[1].Range.mac_da_start_p and \n
 *             mac_da_et_range[1].Range.mac_da_end_p values as well as \n
 *             mac_da_et_range[1].ether_type value \n
 *     bit 18: 1 enables range of packet MAC destination addresses comparison \n
 *             with mac_da_range.mac_da_start_p and mac_da_range.mac_da_end_p \n
 *     bit 19: 1 enables packet MAC destination addresses comparison with \n
 *             mac_da_44bit_const_p constant \n
 *     bit 20: 1 enables packet MAC destination addresses comparison with \n
 *             mac_da_48bit_const_p constant \n
 *     bit 31: 1 enables MACsec KaY packets as control packets \n
 *
 * \arg cp_match_sub_mask \n
 *     Match mask which can be used to mark control packets with the \n
 *     Sub Control Packet signal, for each matching rule. \n
 *     bit i (0-7): 1 enables packet MAC destination address comparison with \n
 *                 the respective mac_da_et_rules[i].mac_da_p address \n
 *     bit j (8-15): 1 enables packet Ethernet Type value comparison with \n
 *                  the respective mac_da_et_rules[j-8].ether_type value \n
 *     bit 16: 1 enables range of packet MAC destination address as well as \n
 *             Ethernet Type value comparison with the respective \n
 *             mac_da_et_range[0].Range.mac_da_start_p and \n
 *             mac_da_et_range[0].Range.mac_da_end_p values as well as \n
 *             mac_da_et_range[0].ether_type value \n
 *     bit 17: 1 enables range of packet MAC destination address as well as \n
 *             Ethernet Type value comparison with the respective \n
 *             mac_da_et_range[1].Range.mac_da_start_p and \n
 *             mac_da_et_range[1].Range.mac_da_end_p values as well as \n
 *             mac_da_et_range[1].ether_type value \n
 *     bit 18: 1 enables range of packet MAC destination addresses comparison \n
 *             with mac_da_range.mac_da_start_p and mac_da_range.mac_da_end_p \n
 *     bit 19: 1 enables packet MAC destination addresses comparison with \n
 *             mac_da_44bit_const_p constant \n
 *     bit 20: 1 enables packet MAC destination addresses comparison with \n
 *             mac_da_48bit_const_p constant \n
 *     bit 31: 1 enables MACsec KaY packets as control packets \n
 *
 */
typedef struct {
    bcm_plp_cfye_mac_da_et_match_rule_t mac_da_et_rules[BCM_PLP_CFYE_MAC_DA_ET_MATCH_RULES_COUNT];
    bcm_plp_cfye_mac_da_et_range_match_rule_t mac_da_et_range[BCM_PLP_CFYE_MAC_DA_ET_RANGE_MATCH_RULES_COUNT];
    bcm_plp_cfye_mac_da_range_match_rule_t mac_da_range;
    unsigned char * mac_da_44bit_const_p;
    unsigned char * mac_da_48bit_const_p;
    unsigned int cp_match_mode_mask;
    unsigned int scp_match_mode_mask;
    unsigned int cp_match_enable_mask;
    unsigned int scp_match_enable_mask;
    unsigned int cp_match_sub_mask;
} bcm_plp_cfye_control_packet_t;


/*! Exception cases control
 *
 * \arg drop_action \n
 *     Defines the way the drop operation is performed if the vPort is missed \n
 *
 * \arg fforce_drop \n
 *     Facilitates channel reset sequence and allows in single write drop all \n
 *     incoming packets for a channel \n
 *     true  - force dropping for all packets for a given channel \n
 *     false - do not force dropping \n
 *
 * \arg fdefault_vport_valid \n
 *     Allows passing packets that missed the vPort through, for this drop_action \n
 *     must not be set to BCM_PLP_ CFYE_DROP_INTERNAL \n
 *     true -  allow assigning a default vPort for packet that missed vPort matching \n
 *     false - do not change the results of the vPort matching \n
 *
 * \arg default_vport \n
 *     Default vPort value, ignored if fdefault_vportValid is set to false \n
 *
 * \arg ecc_drop_action \n
 *     Defines the way the drop operation is performed if an uncorrectable \n
 *       ECC error occurs \n
 *
 * \arg fshould_secure \n
 *     Enable should-secure policy on this channel \n
 *
 */
typedef struct {
    bcm_plp_cfye_drop_type_t drop_action;
    unsigned char fforce_drop;
    unsigned char fdefault_vport_valid;
    unsigned short int default_vport;
    bcm_plp_cfye_drop_type_t ecc_drop_action;
    unsigned char fshould_secure;
} bcm_plp_cfye_device_exceptions_t;

/*! Device bypass control
 *
 * \arg flow_latency_bypass \n
 *     Indication if the low latency bypass must be enabled (true) or the MACsec mode (false). \n
 *
 * \arg exceptions_p \n
 *     Exception cases settings. Set to NULL if no update is required \n
 *
 * \arg l3l4_p \n
 *     Per-channel L3/L4 header parsing, set to NULL if no update is required \n
 *
 * \arg fipsec \n
 *     true - if IPsec mode is supported. \n
 *
 * \arg fsparse_reg \n
 *     true - Spare register and interrupt controller are present \n
 */

typedef struct {
    unsigned char flow_latency_bypass;
    bcm_plp_cfye_device_exceptions_t * exceptions_p;
    bcm_plp_cfye_channel_l3l4_t * l3l4_p;
    unsigned char fipsec;
    unsigned char fsparse_reg;
} bcm_plp_cfye_device_control_t;

/*! Device statistics control
 *
 * \arg fauto_stat_cntrs_reset \n
 *     true - statistics counters are automatically reset on a read operation \n
 *     false - no reset on a read operation \n
 *
 * \arg count_frame_thr \n
 *     Threshold for the frame counters \n
 *
 * \arg count_inc_dis_ctrl \n
 *     Counter to increment enable control \n
 *
 * \arg chan_count_frame_thr \n
 *     Threshold for the per-channel frame counters \n
 *
 * \arg mtt_count_frame_thr \n
 *     Threshold for the per-MTT frame counters \n
 */
typedef struct {
    unsigned char fauto_stat_cntrs_reset;
    bcm_plp_cfye_stat_counter_t count_frame_thr;
    unsigned char count_inc_dis_ctrl;
    bcm_plp_cfye_stat_counter_t chan_count_frame_thr;
    bcm_plp_cfye_stat_counter_t mtt_count_frame_thr;
} bcm_plp_cfye_statistics_control_t;

/*! Control of automatic EOP insertion.
 *
 * \arg eop_timeout_val \n
 *     Timeout value. \n
 *
 * \arg eop_timeout_ctrl \n
 *     bit mask to specify channels 0 to 31 \n
 *
 * \arg eop_timeout_ctrl_ext \n
 *     bit mask to specify channels 32 to 63 \n
 *
 */
typedef struct
{
    unsigned int eop_timeout_val;
    unsigned int eop_timeout_ctrl;
    unsigned int eop_timeout_ctrl_ext;
} bcm_plp_cfye_eop_conf_t;

/*! ECC configuration parameters
 *
 * \arg ecc_correctable_thr \n
 *     Correctable error threshold. \n
 *
 * \arg ecc_uncorrectable_thr \n
 *     Uncorrectable error threshold. \n
 */
typedef struct
{
    unsigned int ecc_correctable_thr;
    unsigned int ecc_uncorrectable_thr;
} bcm_plp_cfye_ecc_conf_t;

/*! Device control
 *
 * \arg control_p \n
 *     Bypass settings. Set to NULL if no update is required \n
 * \arg cp_p \n
 *     Control packet detection settings. Set to NULL if no update is required \n
 * \arg header_parser_p \n
 *     Header parser settings. Set to NULL if no update is required \n
 * \arg stat_control_p \n
 *     Statistics settings. Set to NULL if no update is required \n
 * \arg eop_conf_p \n
 *     Control of automatic EOP insertion. \n
 * \arg ecc_conf_p \n
 *     ECC configuration settings \n
 */
typedef struct {
    bcm_plp_cfye_device_control_t * control_p;
    bcm_plp_cfye_control_packet_t * cp_p;
    bcm_plp_cfye_header_parser_t * header_parser_p;
    bcm_plp_cfye_statistics_control_t * stat_control_p;
    bcm_plp_cfye_eop_conf_t * eop_conf_p;
    bcm_plp_cfye_ecc_conf_t * ecc_conf_p;
} bcm_plp_cfye_device_t;

/*! Classification device initialization settings
 *
 * \arg flow_latency_bypass \n
 *      true - initialize all channels to pass packets in low-latency bypass mode \n
 *      false - initialize all channels for classification mode \n
 *
 * \arg count_frame_thr_lo \n
 * \arg count_frame_thr_hi \n
 *      Threshold for the frame counters \n
 *
 *  \arg count_inc_dis_ctrl \n
 *    Counter to increment enable control
 *
 *
 * \arg input_tcam_p \n
 *     Pointer to memory location where an array of 32-bit words \n
 *     is stored for the Input TCAM database of the cfye device. \n
 *     This array will be copied to the Input TCAM memory of the cfye device \n
 *     during its initialization. \n
 *     Set to NULL in order to retain to the default (or old) data in the Input TCAM memory \n
 *
 * \arg input_tcam_word_count \n
 *     Size of the input_tcam_p array in 32-bit words \n
 *
 * \arg word_offset \n
 *     32-bit word offset in the Input TCAM memory where the input_tcam_p array must be written \n
 *
 * \arg max_channel_count \n
 *     Specify the maximum number of channels to be used by the driver.
 *     If set to zero, use the maximum supported by the hardware
 *
 * \arg max_vport_count \n
 *    Specify the maximum number of vPorts to be used by the driver.
 *    If set to zero, use the maximum supported by the hardware
 *
 * \arg max_rule_count \n
 *    Specify the maximum number of rules to be used by the driver.
 *    If set to zero, use the maximum supported by the hardware.
 *
 * \arg chan_count_frame_thr_lo \n
 *    Threshold for the per-channel frame counters low-32bits.\n
 *
 * \arg chan_count_frame_thr_hi \n
 *    Threshold for the per-channel frame counters high-32bits.\n
 *
 * \arg eop_timeout_val \n
 *     Automatic EOP insertion timeout value. This field is only valid for applicable PHYs.\n
 *
 * \arg eop_timeout_ctrl \n
 *     Bit mask to specify channels (0 to 31) for automatic EOP insertion. This field is only valid for applicable PHYs.\n
 *
 * \arg ecc_correctable_thr \n
 *     Threshold for the ECC correctable error counters. This field is only valid for applicable PHYs.\n
 *
 * \arg ecc_uncorrectable_thr \n
 *     Threshold for the ECC uncorrectable error counters. This field is only valid for applicable PHYs.\n
 *
 * \arg eop_timeout_ctrl_ext \n
 *     Bit mask to specify channels (32 to 63) for automatic EOP insertion. This field is only valid for applicable PHYs.\n
 *
 * \arg mtt_count_frame_thr_lo \n
 *     Threshold for the per-MTT frame counters low-32bits \n
 *
 * \arg mtt_count_frame_thr_hi \n
 *     Threshold for the per-MTT frame counters high-32bits \n
 *
 * \arg fdrop_control_pkts \n
 *     Drop control packets when they do not match any rule \n
 *
 * \arg fparse_pkt_ipsec \n
 *     Ingress-only Flag: \n
 *     - 1b : Packet Type in TCAM key is set to different values for MPLS IPsec, UDP encapsulated IPsec and IPsec \n
 *     - 0b : packet_type in TCAM key is set to same value for MPLS IPsec, UDP encapsulated IPsec and IPsec \n
 *
 */
typedef struct {
    unsigned char flow_latency_bypass;
    unsigned int count_frame_thr_lo;
    unsigned int count_frame_thr_hi;
    unsigned char count_inc_dis_ctrl;
    unsigned int * input_tcam_p;
    unsigned int input_tcam_word_count;
    unsigned int word_offset;
    unsigned int max_channel_count;
    unsigned int max_vport_count;
    unsigned int max_rule_count;
    unsigned int chan_count_frame_thr_lo;
    unsigned int chan_count_frame_thr_hi;
    unsigned int eop_timeout_val;
    unsigned int eop_timeout_ctrl;
    unsigned int ecc_correctable_thr;
    unsigned int ecc_uncorrectable_thr;
    unsigned int eop_timeout_ctrl_ext;
    unsigned int mtt_count_frame_thr_lo;
    unsigned int mtt_count_frame_thr_hi;
    unsigned char fdrop_control_pkts;
    unsigned char fparse_pkt_ipsec;
} bcm_plp_cfye_init_t;

/*! vPort (vPort policy) data structure
 *
 * \arg sectag_offset \n
 *     Width and function of this field depends on direction: \n
 *       - Egress, 6-bit wide: SecTAG location relative to the start of the frame. \n
 *       - Ingress, 5-bit wide: SecTAG location relative to the last or \n
 *                            5th MPLS label. This field is applicable only for \n
 *                            Bulk MPLS. For other protocols, the SecTAG \n
 *                            location is determined by the parser. \n
 *     IPsec Mode: For ingress and egress write zeroes
 *
 * \arg feompls_support \n
 *     true - vPort processes EoMPLS protocol. \n
 *     false - vPort processes Bulk MPLS or a non-MPLS protocol. \n
 *
 * \arg fmpls_ctrlword \n
 *     true - MPLS control word is present in all packets for this vPort. \n
 *     Combination of this field and eompls_subport field provides a pointer \n
 *     to the start of the inner Ethernet frame for the EoMPLS protocol \n
 *
 * \arg pkt_extension \n
 *     Packet expansion indication \n
 *     00b - 0 bytes (no expansion) \n
 *     01b - IPsec expansion \n
 *     10b - 24 bytes \n
 *     11b - 32 bytes \n
 *     If the packet is classified as a control packet, this field is ignored \n
 *     and no expansion is indicated on the output interface. \n
 *     Note: For egress only, not used for ingress configurations. \n
 *
 * \arg mpls_hdr \n
 *     MPLS header options (ingress-only). \n
 *     00b = Reserved \n
 *     01b = Retain full header \n
 *     10b = Remove full header \n
 *     11b = Remove all labels except of last label. \n
 */

typedef struct {
    unsigned char sectag_offset;
    unsigned char feompls_support;
    unsigned char fmpls_ctrlword;
    unsigned char pkt_extension;
    unsigned char mpls_hdr;
} bcm_plp_cfye_vport_t;

/*! vPort matching rule key/Mask data structure
 *
 * \arg packet_type \n
 *    If packet_type = BCM_PLP_CFYE_RULE_PKT_TYPE_MACSEC then \n
 *         Bits [5:0] = SecTAG TCI (if supported by configuration) \n
 *
 *    If packet_type = BCM_PLP_CFYE_RULE_PKT_TYPE_MPLS or \n
 *         packet_type = BCM_PLP_CFYE_RULE_PKT_TYPE_OTHER then \n
 *         Bit 0 = 1 : No VLAN tags and no MPLS labels \n
 *         Bit 1 = 1 : 1 VLAN tag or 1 MPLS label \n
 *         Bit 2 = 1 : 2 VLAN tags or 2 MPLS labels \n
 *         Bit 3 = 1 : 3 VLAN tags or 3 MPLS labels \n
 *         Bit 4 = 1 : 4 VLAN tags or 4 MPLS labels \n
 *         Bit 5 = 1 : VxLAN packet or 5 MPLS labels \n
 *         Bit 6 = 1 : >4 VLAN tags or >5 MPLS labels \n
 *
 * \arg num_tags \n
 *     bit mask, only 7 bits [6:0] are used, see above how \n
 */
typedef struct {
    bcm_plp_cfye_rule_packet_type_t packet_type;
    unsigned char num_tags;
} bcm_plp_cfye_rule_key_mask_t;

/*! vPort matching rule policy
 *
 * \arg vport_handle \n
 *     vPort handle obtained via bcm_plp_cfye_vPort_Add() function \n
 *
 * \arg priority \n
 *     Priority value that is used to resolve multiple rule matches. \n
 *     When multiple rules are hit by a packet simultaneously, the rule with \n
 *     the higher priority value will be returned. If multiple rules with \n
 *     an identical priority value are hit, the rule with the lowest rule index is used. \n
 *
 * \arg fdrop \n
 *     true : drop the packet \n
 *
 * \arg fcontrol_packet \n
 *     true : process the packet as controlled \n
 *
 * \arg an \n
 *     vPort to SA association number \n
 *
 * \arg fcontrol_packet_sub \n
 *     true: mark packets  matching the rule as subset control packets  \n
 */
typedef struct {
    bcm_plp_cfye_vport_handle_t vport_handle;
    unsigned char priority;
    unsigned char fdrop;
    unsigned char fcontrol_packet;
    unsigned char an;
    unsigned char fcontrol_packet_sub;
} bcm_plp_cfye_rule_policy_t;


/*! vPort matching rule data structure
 *
 * \arg key \n
 *    Sets matching values as specified in bcm_plp_cfye_rule_key_mask_t \n
 *
 * \arg mask \n
 *    Mask for matching values, can be used to mask out irrelevant key bits \n
 *
 * \arg data \n
 *    Data[0] : MAC Destination Address least significant bytes (3..0) \n
 *    Data[1] : MAC Destination Address most significant bytes (5, 4) \n
 *    For IPsec:\n
 *    Data[2] : SPI in network byte order (6)\n
 *    Data[3] : [35:32] : MyTunnelId. [55:36] : Write zeroes and ignore on read\n
 *
 *    For any other protocols:\n
 *    Data[2] : Packet data (ether_type, VLAN tag, MPLS label) \n
 *    Data[3] : Packet data (ether_type, VLAN tag, MPLS label) \n
 *
 * \arg data_mask \n
 *    Mask for data values, can be used to mask out irrelevant Data bits \n
 *
 * \arg policy \n
 *    Rule policy \n
 */
typedef struct {
    bcm_plp_cfye_rule_key_mask_t key;
    bcm_plp_cfye_rule_key_mask_t mask;
    unsigned int data[BCM_PLP_CFYE_RULE_NON_CTRL_WORD_COUNT];
    unsigned int data_mask[BCM_PLP_CFYE_RULE_NON_CTRL_WORD_COUNT];
    bcm_plp_cfye_rule_policy_t policy;
} bcm_plp_cfye_rule_t;

/*! Statistics counter */
typedef struct {
    bcm_plp_cfye_stat_counter_t counter;
} bcm_plp_cfye_statistics_t;

/*! cfye statistics channel summary
 *
 *  \arg Counter
 *      Pointer to a memory location where the content of the channel counter summary
 *      will be stored. This field is supported only on applicable PHYs.
 *
 *  \arg ChSummary
 *      Pointer to a memory location where the content of the channel counter summary
 *      will be stored (one bit per channel), e.g. bit 0 for channel 0, bit 1 for channel 1.
 *      This field is supported only on applicable PHYs.
 *
 *  \arg ChSummary_ext
 *      Pointer to a memory location where the content of the channel counter summary
 *      will be stored (one bit per channel), e.g. bit 0 for channel 32, bit 1 for channel 33.
 *      This field is supported only on applicable PHYs.
 *
 */
typedef struct {
    unsigned char Counter[BCM_PLP_CFYE_STAT_CHAN_SUMMARY_MAX_COUNTERS];
    unsigned int ChSummary;     /* Channels 31-0  */
    unsigned int ChSummary_ext; /* Channels 63-32 */  /* This field is supported only on applicable PHYs. Used in DDK_V3.2 */
} bcm_plp_cfye_statistics_channel_summary_t;

/*! cfye device interrupt
 *
 *  \arg event_mask
 *      The requested events, bitwise OR of BCM_PLP_SECY_EVENT_* global events or
 *      BCM_PLP_SECY_EVENT_CHAN_* channel-specific events
 *
 *  \arg [IN] f_global
 *      True if notification is for the device Global AIC,
 *      otherwise the notification is for the device Channel AIC
 */
typedef struct {
    unsigned int event_mask;
    unsigned char f_global;
} bcm_plp_cfye_intr_t;



/*! Packet drop event from classification logic */
#define BCM_PLP_SECY_EVENT_DROP_CLASS           1

/*! Packet drop event from post-processor logic */
#define BCM_PLP_SECY_EVENT_DROP_PP              2

/*! Packet drop event from MTU checking logic (egress only) */
#define BCM_PLP_SECY_EVENT_DROP_MTU             4

/*! Interrupt event from EIP-64 MACsec crypto-engine (PE) core. */
#define BCM_PLP_SECY_EVENT_ENG_IRQ              8

/*! Statistics threshold event from SA Statistics Module */
#define BCM_PLP_SECY_EVENT_STAT_SA_THR          16

/*! Statistics threshold event from SecY Statistics Module */
#define BCM_PLP_SECY_EVENT_STAT_SECY_THR        32

/*! Statistics threshold event from IFC Statistics Module */
#define BCM_PLP_SECY_EVENT_STAT_IFC_THR         64

/*! Statistics threshold event from IFC1 Statistics Module */
#define BCM_PLP_SECY_EVENT_STAT_IFC1_THR        128

/*! Statistics threshold event from RXCAM Statistics Module */
#define BCM_PLP_SECY_EVENT_STAT_RXCAM_THR       256

/*! Statistics threshold event from Channel Statistics Module */
#define BCM_PLP_SECY_EVENT_STAT_CHAN_THR        512

/*! Packet number threshold event from Post Processor */
#define BCM_PLP_SECY_EVENT_SA_PN_THR            1024

/*! SA expired event from Classifier */
#define BCM_PLP_SECY_EVENT_SA_EXPIRED           2048

/*! Input error event from the Packet Engine */
#define BCM_PLP_SECY_EVENT_PE_IN_ERR            0x00010000U

/*! Output error event from the Packet Engine */
#define BCM_PLP_SECY_EVENT_PE_OUT_ERR           0x00020000U

/*! Packet processing error event from the Packet Engine */
#define BCM_PLP_SECY_EVENT_PE_PKT_PROC_ERR      0x00040000U

/*! Context error event from the Packet Engine */
#define BCM_PLP_SECY_EVENT_PE_CTX_ERR           0x00080000U

/*! Sequence number threshold event from the Packet Engine */
#define BCM_PLP_SECY_EVENT_PE_OB_SEQNR_THR      0x00100000U

/*! Sequence number roll-over event from the Packet Engine */
#define BCM_PLP_SECY_EVENT_PE_OB_SEQNR_RO       0x00200000U

/*! Device channel-specific events to use with the notifications
 */
/*! Input interface events */
/*! Start-Of-Packet (SOP) is received for a busy channel event */
#define BCM_PLP_SECY_EVENT_CHAN_ERR_SOP_WO_EOP          1

/*! End-Of-Packet (EOP) is received for an idle channel event */
#define BCM_PLP_SECY_EVENT_CHAN_ERR_EOP_WO_SOP          2

/*! Data is received for an idle channel event */
#define BCM_PLP_SECY_EVENT_CHAN_ERR_XFER_WO_SOP         4

/*! SOP is received in the 2nd cycle of the slot event */
#define BCM_PLP_SECY_EVENT_CHAN_ERR_SLOT_SOP            8

/*! Channel ID is changed in the 2nd cycle of the slot event */
#define BCM_PLP_SECY_EVENT_CHAN_ERR_SLOT_CHID           16

/*! First 2 packet words are not received back-to-back in the first slot event */
#define BCM_PLP_SECY_EVENT_CHAN_ERR_NOT_B2B             32

/*! Other channel-specific events */
/*! Multiple RxCAM rule hit for a packet event (ingress only) */
#define BCM_PLP_SECY_EVENT_CHAN_RXCAM_HIT_MULT          64

/*! RxCAM rule not hit for a packet event (ingress only) */
#define BCM_PLP_SECY_EVENT_CHAN_RXCAM_MISS              128

/*! Packet is dropped due to overrun in data pipeline event */
#define BCM_PLP_SECY_EVENT_CHAN_PKT_DATA_OVERRUN        256

/*! Packet is dropped due to overrun in classification pipeline event */
#define BCM_PLP_SECY_EVENT_CHAN_PKT_CFY_OVERRUN         512

/*! SECY Global AIC interrupt resources */
#define BCM_PLP_SECY_PHY_DROP_CLASS_IRQ       0
#define BCM_PLP_SECY_PHY_DROP_PP_IRQ          1
#define BCM_PLP_SECY_PHY_DROP_MTU_IRQ         2
#define BCM_PLP_SECY_PHY_IRQ                  3
#define BCM_PLP_SECY_PHY_STAT_SA_THR_IRQ      4
#define BCM_PLP_SECY_PHY_STAT_SECY_THR_IRQ    5
#define BCM_PLP_SECY_PHY_STAT_IFC_THR_IRQ     6
#define BCM_PLP_SECY_PHY_STAT_IFC1_THR_IRQ    7
#define BCM_PLP_SECY_PHY_STAT_RXCAM_THR_IRQ   8
#define BCM_PLP_SECY_PHY_STAT_CHAN_THR_IRQ    9
#define BCM_PLP_SECY_PHY_SA_PN_THR_IRQ        10
#define BCM_PLP_SECY_PHY_SA_EXPIRED_IRQ       11

/*! SECY Channel AIC interrupt resources */
#define BCM_PLP_SECY_CHAN_ERR_SOP_WO_EOP_IRQ       0
#define BCM_PLP_SECY_CHAN_ERR_EOP_WO_SOP_IRQ       1
#define BCM_PLP_SECY_CHAN_ERR_XFER_WO_SOP_IRQ      2
#define BCM_PLP_SECY_CHAN_ERR_SLOT_SOP_IRQ         3
#define BCM_PLP_SECY_CHAN_ERR_SLOT_CHID_IRQ        4
#define BCM_PLP_SECY_CHAN_ERR_NOT_B2B_IRQ          5
#define BCM_PLP_SECY_CHAN_RXCAM_HIT_MULT_IRQ       6
#define BCM_PLP_SECY_CHAN_RXCAM_MISS_IRQ           7
#define BCM_PLP_SECY_CHAN_PKT_DATA_OVERRUN_IRQ     8
#define BCM_PLP_SECY_CHAN_PKT_CFY_OVERRUN_IRQ      9

/*! Maximum Crypt-Authenticate bypass length */
#define BCM_PLP_CRYPT_AUTH_BYPASS_LEN_MAX          63

/*! Status values returned by API functions */
typedef enum {
    BCM_PLP_SECY_STATUS_OK,
    BCM_PLP_SECY_ERROR_BAD_PARAMETER,
    BCM_PLP_SECY_ERROR_INTERNAL,
    BCM_PLP_SECY_ERROR_NOT_IMPLEMENTED,
    BCM_PLP_SECY_ERROR_DEVICE_RW = 0x7fffffff
} bcm_plp_secy_status_t;

/*! SecY device role: either MACsec egress only, ingress only, both or IPsec ingress, IPSec egress */
typedef enum {
    BCM_PLP_SECY_ROLE_EGRESS,
    BCM_PLP_SECY_ROLE_INGRESS,
    BCM_PLP_SECY_ROLE_EGRESS_INGRESS,
    BCM_PLP_SECY_ROLE_IPSEC_EGRESS,
    BCM_PLP_SECY_ROLE_IPSEC_INGRESS
} bcm_plp_secy_role_t;

/*! Types of ports
 *      Tx MAC, \n
 *      Redirect FIFO, \n
 *      Capture debug FIFO, \n
 *      TX MAC and Capture debug FIFO. \n
 */
typedef enum {
    BCM_PLP_SECY_PORT_COMMON,
    BCM_PLP_SECY_PORT_RESERVED,
    BCM_PLP_SECY_PORT_CONTROLLED,
    BCM_PLP_SECY_PORT_UNCONTROLLED
} bcm_plp_secy_port_type_t;

/*! bcm_plp_secy_sa_action_type_t
 *
 * SA action type: \n
 *      bypass, \n
 *      drop, \n
 *      MACsec ingress (do not use for an egress only device), \n
 *      MACsec egress (do not use for an ingress only device), \n
 *      Crypt Authenticate, \n
 *      IPsec Ingress, \n
 *      IPsec Egress. \n
 */

typedef enum {
    BCM_PLP_SECY_SA_ACTION_BYPASS,
    BCM_PLP_SECY_SA_ACTION_DROP,
    BCM_PLP_SECY_SA_ACTION_INGRESS,
    BCM_PLP_SECY_SA_ACTION_EGRESS,
    BCM_PLP_SECY_SA_ACTION_CRYPT_AUTH,
    BCM_PLP_SECY_SA_ACTION_IPSEC_INGRESS,
    BCM_PLP_SECY_SA_ACTION_IPSEC_EGRESS
} bcm_plp_secy_sa_action_type_t;

/*! bcm_plp_secy_drop_type_t
 *
 * SA drop type: \n
 *   0 = bypass with CRC corruption signaling, \n
 *   1 = bypass with bad packet indicator, \n
 *   2 = marked for internal drop (by external logic), \n
 *   3 = do not drop (for debugging only). \n
 */

typedef enum {
    BCM_PLP_SECY_SA_DROP_CRC_ERROR,
    BCM_PLP_SECY_SA_DROP_PKT_ERROR,
    BCM_PLP_SECY_SA_DROP_INTERNAL,
    BCM_PLP_SECY_SA_DROP_NONE
} bcm_plp_secy_drop_type_t;

/*! bcm_plp_secy_validate_frames_t
 *
 * ingress tagged frame validation options \n
 */

typedef enum {
    BCM_PLP_SECY_FRAME_VALIDATE_DISABLE,
    BCM_PLP_SECY_FRAME_VALIDATE_CHECK,
    BCM_PLP_SECY_FRAME_VALIDATE_STRICT
} bcm_plp_secy_validate_frames_t;

/*! Statistics counter */
typedef struct {
    unsigned int lo;
    unsigned int hi;
} bcm_plp_secy_stat_counter_t;

/*! bcm_plp_secy_sa_handle_t
 *
 * This handle is a reference to an SA. It is returned when an SA \n
 * is added and it remains valid until the SA is removed. \n
 *
 * The handle is set to NULL when bcm_plp_secy_sa_handle_t handle.p is equal to NULL. \n
 */

typedef struct {
    void * p;
} bcm_plp_secy_sa_handle_t;

/*! SA parameters for egress action type
 * \n
 * \arg fsa_inuse \n
 *    true - SA is in use, packets classified for it can be transformed \n
 *    false - SA not in use, packets classified for it can not be transformed \n
 *
 * \arg confidentiality_offset \n
 *    The number of bytes (in the range of 0-127) that are authenticated but \n
 *    not encrypted following the SecTAG in the encrypted packet. Values \n
 *    65-127 are reserved in HW < 4.0 and should not be used there. \n
 *
 * \arg fprotect_frames \n
 *    true - enable frame protection, \n
 *    false - bypass frame through device \n
 *
 * \arg finclude_sci \n
 *    true - inserts explicit SCI in the packet, \n
 *    false - use implicit SCI (not transferred) \n
 *
 * \arg fuse_es \n
 *    true - enable ES bit in the generated SecTAG \n
 *    false - disable ES bit in the generated SecTAG \n
 *
 * \arg fuse_scb \n
 *    true - enable SCB bit in the generated SecTAG \n
 *    false - disable SCB bit in the generated SecTAG \n
 *
 * \arg fconf_protect \n
 *    true - enable confidentiality protection \n
 *    false - disable confidentiality protection \n
 *
 * \arg fallow_data_pkts \n
 *    true - allow data (non-control) packets. \n
 *    false - drop data packets. \n
 *
 * \arg pre_sectag_auth_start \n
 *  Specifies number of bytes from the start of the packet to be bypassed without MACsec protection
 *
 * \arg pre_sectag_auth_length \n
 *  Specifies number of bytes to be authenticated in pre-SecTAG area
*/

typedef struct {
    unsigned char fsa_inuse;
    unsigned char confidentiality_offset;
    unsigned char fprotect_frames;
    unsigned char finclude_sci;
    unsigned char fuse_es;
    unsigned char fuse_scb;
    unsigned char fconf_protect;
    unsigned char fallow_data_pkts;
    unsigned char pre_sectag_auth_start;
    unsigned char pre_sectag_auth_length;
} bcm_plp_secy_sa_e_t;


/*! SA parameters for ingress action type
 * \n
 * \arg fsa_inuse \n
 *     true - SA is in use, packets classified for it can be transformed \n
 *     false - SA not in use, packets classified for it can not be transformed \n
 *
 * \arg confidentiality_offset \n
 *     The number of bytes (in the range of 0-127) that are authenticated but \n
 *     not encrypted following the SecTAG in the encrypted packet. Values \n
 *     65-127 are reserved in hardware < 4.0 and should not be used there. \n
 *
 * \arg freplay_protect \n
 *     true - enable replay protection \n
 *     false - disable replay protection \n
 *
 * \arg validate_frames_tagged; \n
 *     MACsec frame validation level (tagged). \n
 *
 * \arg sci_p \n
 *     SCI to which ingress SA applies (8 bytes). \n
 *
 * \arg an \n
 *     Association number to which ingress SA applies. \n
 * \n
 * \arg fallow_tagged \n
 *     true - allow tagged packets. \n
 *     false - drop tagged packets. \n
 *
 * \arg fallow_untagged \n
 *     true - allow untagged packets. \n
 *     false - drop untagged packets. \n
 * \n
 * \arg fvalidate_untagged \n
 *     true - enable validate untagged packets. \n
 *     false - disable validate untagged packets. \n
 *
 * \arg feompls \n
 *  True is EOMPLS
 *
 * \arg fmac_inmac \n
 *  True is Mac in Mac
 *
 * \arg pre_sectag_auth_start \n
 *  Specifies number of bytes from the start of the packet to be bypassed without MACsec protection
 *
 * \arg pre_sectag_auth_length \n
 *  Specifies number of bytes to be authenticated in pre-SecTAG area
 *
 * \arg fretain_sectag
 * For situations when RxSC is not found or fSAInUse=false with validation
 *  level that allows packet to be sent to the Controlled port with
 *  the SecTAG/ICV removed, this flag represents a policy to allow SecTAG
 *  retaining.\n
 *  true - SecTAG is retained.
 * 
 * \arg fretain_icv
 *  true - ICV is retained (allowed only when fretain_sectag is true)
 *
 *  \arg fretire_sa
 *  true - Retire the SA after a period of inactivity.
*/

typedef struct {
    unsigned char fsa_inuse;
    unsigned char confidentiality_offset;
    unsigned char freplay_protect;
    bcm_plp_secy_validate_frames_t validate_frames_tagged;
    unsigned char *sci_p;
    unsigned char an;
    unsigned char fallow_tagged;
    unsigned char fallow_untagged;
    unsigned char fvalidate_untagged;
    unsigned char feompls;
    unsigned char fmac_inmac;
    unsigned char pre_sectag_auth_start;
    unsigned char pre_sectag_auth_length;
    unsigned char fretain_sectag;
    unsigned char fretain_icv;
    unsigned char fretire_sa;
} bcm_plp_secy_sa_i_t;


/*! SA parameters for Bypass/Drop action type
 *
 * \arg fsa_inuse \n
 *     true - enable statistics counting for the associated SA \n
 *     false - disable statistics counting for the associated SA \n
 */
typedef struct {
    unsigned char fsa_inuse;
} bcm_plp_secy_sa_bd_t;

/*! SA parameters for Crypt-Authenticate action type
 *
 * \arg fzero_length_message \n
 *     true - message has length 0 \n
 *     false - message has length > 0 \n
 *
 * \arg confidentiality_offset \n
 *     The number of bytes (in the range of 0-255) that are authenticated but \n
 *     not encrypted (AAD length). \n
 * \arg iv_mode \n
 *    IV loading mode: \n
 *       0: The IV is fully loaded via the transform record. \n
 *       1: The full IV is loaded via the input frame. This IV is located in \n
 *          front of the frame and is considered to be part of the bypass data, \n
 *          however it is not part to the result frame. \n
 *       2: The full IV is loaded via the input frame. This IV is located at the \n
 *          end of the bypass data and is considered to be part of the bypass \n
 *          data, and it also part to the result frame. \n
 *       3: The first three IV words are loaded via the input frame, the counter \n
 *          value of the IV is set to one. The three IV words are located in \n
 *          front of the frame and are considered to be part of the bypass data, \n
 *          however it is not part to the result frame. \n
 *
 * \arg ficv_append \n
 *    true - append the calculated ICV \n
 *    false - don't append the calculated ICV \n
 *
 * \arg ficv_verify \n
 *    true - enable ICV verification \n
 *    false - disable ICV verification \n
 *
 * \arg fconf_protect \n
 *    true - enable confidentiality protection (AES-GCM/CTR operation) \n
 *    false - disable confidentiality protection (AES-GMAC operation) \n
 */
typedef struct {
    unsigned char fzero_length_message;
    unsigned char confidentiality_offset;
    unsigned char iv_mode;
    unsigned char ficv_append;
    unsigned char ficv_verify;
    unsigned char fconf_protect;
} bcm_plp_secy_sa_ca_t;

/*! SA parameters for IPsec Egress action type
 *
 * \arg fsa_in_use \n
 *     true - SA is in use, packets classified for it can be transformed \n
 *     false - SA not in use, packets classified for it can not be transformed \n
 *
 * \arg frollover_mode \n
 *     true - When nextPN reaching all 1s, it rolls over to value 1 without \n
 *     a sequence number error generated. \n
 *
 * \arg fencr_auth \n
 *     true - Encrypt the GHASH output \n
 *
 * \arg crypto_alg \n
 *     Algorithm for data encryption.\n
 *     101b - AES-CTR-128\n
 *     111b - AES-CTR-256\n
 *
 * \arg freplay_check \n
 *     Enabling IPsec replay check with the mask \n
 *
 * \arg fig_hdr_insert \n
 *     true - enable inserting 4-byte SA-Tag.\n
 *     false - disable inserting 4-byte SA-Tag \n
 *
 * \arg fupdate_udp \n
 *     true - If UDP header is present, update length  \n
 *
 * \arg fupdate_ip \n
 *     true - Update outer IP header \n
 *
 * \arg fnat_udp \n
 *     true - The NAT-T UDP is present after outer IP header \n
 *
 * \arg fouter_iphdr \n
 *     true  - Outer IP header is IPv6 \n
 *     false - Outer IP header is IPV4 \n
 *
 * \arg fconf_protect \n
 *     true - enable confidentiality protection \n
 *     false - disable confidentiality protection \n
 *
 * \arg fprotect_frames \n
 *     true - enable frame protection \n
 *     false - bypass frame through device \n
 *
 * \arg ftransport \n
 *     true - transport mode is used \n
 *     false - tunnel mode is used \n
 *
 * \arg next_hdr \n
 *    Next Header byte for Transport mode \n
 */
typedef struct
{
    unsigned char fsa_in_use;
    unsigned char frollover_mode;
    unsigned char fencr_auth;
    unsigned char crypto_alg;
    unsigned char freplay_check;
    unsigned char fig_hdr_insert;
    unsigned char fupdate_udp;
    unsigned char fupdate_ip;
    unsigned char fnat_udp;
    unsigned char fouter_iphdr;
    unsigned char fconf_protect;
    unsigned char fprotect_frames;
    unsigned char ftransport;
    unsigned char next_hdr;
} bcm_plp_secy_sa_ipsec_e_t;

/*! SA parameters for IPsec Ingress action type
 *
 * \arg fsa_in_use \n
 *     true - SA is in use, packets classified for it can be transformed \n
 *     false - SA not in use, packets classified for it can not be transformed \n
 *
 * \arg fretain_pad \n
 *     IPsec padding is not removed \n
 *
 * \arg fpad_check \n
 *     IPsec pad check is enabled \n
 *
 * \arg freplay_check \n
 *     Enabling IPsec replay check with the mask \n
 *
 * \arg fig_hdr_insert \n
 *     true - enable inserting 4-byte SA-Tag.\n
 *     false - disable inserting 4-byte SA-Tag \n
 *
 * \arg freplay_protect \n
 *     true - enable replay protection \n
 *     false - disable replay protection \n
 *
 * \arg fconf_protect \n
 *     true - enable confidentiality protection \n
 *
 * \arg fpad_not_valid_drop \n
 *     true - Drop policy for packets with not valid padding \n
 *
 * \arg fpad_len_fail_drop \n
 *     true - Drop policy for packets with padding that failed length check  \n
 *
 * \arg fupdate_ip \n
 *     true - update outer IP header, based on information from the decrypted \n
 *     inner IP header \n
 *
 * \arg fupdate_ttl \n
 *     true - for MPLS-IPsec: copy TTL from the outer MPLS header into the outer \n
 *     IP header TTL, if MPLS TTL value is smaller.\n
 *
 * \arg ftransport \n
 *     true - transport mode is used \n
 *     false - tunnel mode is used \n
 *
 * \arg next_hdr \n
 *    Next Header byte for Transport mode \n
 *
 * \arg an \n
 *    AN (0 or 1) for which this SA is used. Selects one of 2 SAs on an ingress vPort \n
 */
typedef struct
{
    unsigned char fsa_in_use;
    unsigned char fretain_pad;
    unsigned char fpad_check;
    unsigned char freplay_check;
    unsigned char fig_hdr_insert;
    unsigned char freplay_protect;
    unsigned char fconf_protect;
    unsigned char fpad_not_valid_drop;
    unsigned char fpad_len_fail_drop;
    unsigned char fupdate_ip;
    unsigned char fupdate_ttl;
    unsigned char ftransport;
    unsigned char next_hdr;
    unsigned char an;
} bcm_plp_secy_sa_ipsec_i_t;

/*! bcm_plp_secy_sa_t
 *
 * SecY SA data structure that contains data required to add a new SA. \n
 *
 * \arg sa_word_count \n
 *     Size of the transform record (transform_record_p) associated with SA in 32-bit words \n
 * \arg transform_record_p \n
 *     Pointer to the transform record data \n
 *     All fields of the transform record must be populated by the Host software before the
 *     corresponding SA flow can be enabled. For bypass and drop flows, the transform record is not used.
 *      For MACsec transformations, the hardware only updates the sequence number field;
 *      it will not modify the other fields during MACsec egress and ingress processing.
 *      Transform record has fixed layout for all supported MACsec use cases.
 *      <STRONG>Note: The unused bytes at the end of the record must be written with zeroes.</STRONG>\n
 *
 *      <STRONG> Transform record format 24 * 32 bits for Miura, Evora, and Europa</STRONG> \n
 * <table>
 * <caption id="multi_row">Transform record format table</caption>
 * <tr><th>    <th>Egress Fields   <th>Ingress Fields
 * </tr>
 * <tr> <td>  0  </td> <td> Ctrl </td> <td> Ctrl </td> </tr>
 * <tr> <td>  1  </td> <td> ID </td> <td> ID </td> </tr>
 * <tr> <td>  2  </td> <td> Key0 </td> <td> Key0 </td> </tr>
 * <tr> <td>  3  </td> <td> Key1 </td> <td> Key1 </td> </tr>
 * <tr> <td>  4  </td> <td> Key2 </td> <td> Key2 </td> </tr>
 * <tr> <td>  5  </td> <td> Key3 </td> <td> Key3 </td> </tr>
 * <tr> <td>  6  </td> <td> Key4 </td> <td> Key4 </td> </tr>
 * <tr> <td>  7  </td> <td> Key5 </td> <td> Key5 </td> </tr>
 * <tr> <td>  8  </td> <td> Key6 </td> <td> Key6 </td> </tr>
 * <tr> <td>  9  </td> <td> Key7 </td> <td> Key7 </td> </tr>
 * <tr> <td>  10  </td> <td> Hkey0 </td> <td> Hkey0 </td> </tr>
 * <tr> <td>  11  </td> <td> Hkey1 </td> <td> Hkey1 </td> </tr>
 * <tr> <td>  12  </td> <td> Hkey2 </td> <td> Hkey2 </td> </tr>
 * <tr> <td>  13  </td> <td> Hkey3 </td> <td> Hkey3 </td> </tr>
 * <tr> <td>  14  </td> <td> Seq0 </td> <td> Seq0 </td> </tr>
 * <tr> <td>  15  </td> <td> Seq1 </td> <td> Seq1 </td> </tr>
 * <tr> <td>  16  </td> <td> (Zero) </td> <td> Mask </td> </tr>
 * <tr> <td>  17  </td> <td> ISO(CtxSalt) </td> <td> ISO(CtxSalt) </td> </tr>
 * <tr> <td>  18  </td> <td> IS1(CtxSalt) </td> <td> IS1(CtxSalt) </td> </tr>
 * <tr> <td>  19  </td> <td> IS2(CtxSalt) </td> <td> IS2(CtxSalt) </td> </tr>
 * <tr> <td>  20  </td> <td> IV0(SCI0) </td> <td> IVO(SCI0) </td> </tr>
 * <tr> <td>  21  </td> <td> IV1(SCI1) </td> <td> IV1(SCI1) </td> </tr>
 * <tr> <td>  22  </td> <td> (Zero) </td> <td> (Zero) </td> </tr>
 * <tr> <td>  23  </td> <td> SA upd ctrl </td> <td> SA upd ctrl </td> </tr>
 * </table>
 *
 *
 * <STRONG> MACsec Transform record format 22 * 32 bits for Aperta and Aperta2</STRONG> \n
 * <table>
 * <caption id="multi_row">Transform record format table</caption>
 * <tr><th>    <th>Egress Fields   <th>Ingress Fields
 * </tr>
 * <tr> <td>  0  </td> <td> Ctrl </td> <td> Ctrl </td> </tr>
 * <tr> <td>  1  </td> <td> SA upd ctrl </td> <td> SA upd ctrl </td> </tr>
 * <tr> <td>  2  </td> <td> Key0 </td> <td> Key0 </td> </tr>
 * <tr> <td>  3  </td> <td> Key1 </td> <td> Key1 </td> </tr>
 * <tr> <td>  4  </td> <td> Key2 </td> <td> Key2 </td> </tr>
 * <tr> <td>  5  </td> <td> Key3 </td> <td> Key3 </td> </tr>
 * <tr> <td>  6  </td> <td> Key4 </td> <td> Key4 </td> </tr>
 * <tr> <td>  7  </td> <td> Key5 </td> <td> Key5 </td> </tr>
 * <tr> <td>  8  </td> <td> Key6 </td> <td> Key6 </td> </tr>
 * <tr> <td>  9  </td> <td> Key7 </td> <td> Key7 </td> </tr>
 * <tr> <td>  10  </td> <td> Hkey0 </td> <td> Hkey0 </td> </tr>
 * <tr> <td>  11  </td> <td> Hkey1 </td> <td> Hkey1 </td> </tr>
 * <tr> <td>  12  </td> <td> Hkey2 </td> <td> Hkey2 </td> </tr>
 * <tr> <td>  13  </td> <td> Hkey3 </td> <td> Hkey3 </td> </tr>
 * <tr> <td>  14  </td> <td> Seq0 </td> <td> Seq0 </td> </tr>
 * <tr> <td>  15  </td> <td> Seq1 </td> <td> Seq1 </td> </tr>
 * <tr> <td>  16  </td> <td> MTU </td> <td> Mask </td> </tr>
 * <tr> <td>  17  </td> <td> ISO(CtxSalt) </td> <td> ISO(CtxSalt)/ IV0(SCI0) </td> </tr>
 * <tr> <td>  18  </td> <td> IS1(CtxSalt) </td> <td> IS1(CtxSalt)/ IV1(SCI1) </td> </tr>
 * <tr> <td>  19  </td> <td> IS2(CtxSalt) </td> <td> IS2(CtxSalt)/ Zero </td> </tr>
 * <tr> <td>  20  </td> <td> IV0(SCI0) </td> </tr>
 * <tr> <td>  21  </td> <td> IV1(SCI1) </td> </tr>
 * </table>
 *
 *
 * <STRONG> IPsec transform record format 22 * 32 bits for Aperta2</STRONG> \n
 * <table>
 * <caption id="multi_row">Transform record format table</caption>
 * <tr><th>    <th>Egress Fields   <th>Ingress Fields
 * </tr>
 * <tr> <td>  0  </td> <td> Ctrl </td> <td> Ctrl </td> </tr>
 * <tr> <td>  1  </td> <td> SA upd ctrl </td> <td> SA upd ctrl </td> </tr>
 * <tr> <td>  2  </td> <td> Key0 </td> <td> Key0 </td> </tr>
 * <tr> <td>  3  </td> <td> Key1 </td> <td> Key1 </td> </tr>
 * <tr> <td>  4  </td> <td> Key2 </td> <td> Key2 </td> </tr>
 * <tr> <td>  5  </td> <td> Key3 </td> <td> Key3 </td> </tr>
 * <tr> <td>  6  </td> <td> Key4 </td> <td> Key4 </td> </tr>
 * <tr> <td>  7  </td> <td> Key5 </td> <td> Key5 </td> </tr>
 * <tr> <td>  8  </td> <td> Key6 </td> <td> Key6 </td> </tr>
 * <tr> <td>  9  </td> <td> Key7 </td> <td> Key7 </td> </tr>
 * <tr> <td>  10  </td> <td> Hkey0 </td> <td> Hkey0 </td> </tr>
 * <tr> <td>  11  </td> <td> Hkey1 </td> <td> Hkey1 </td> </tr>
 * <tr> <td>  12  </td> <td> Hkey2 </td> <td> Hkey2 </td> </tr>
 * <tr> <td>  13  </td> <td> Hkey3 </td> <td> Hkey3 </td> </tr>
 * <tr> <td>  14  </td> <td> Seq0 </td> <td> Seq0 </td> </tr>
 * <tr> <td>  15  </td> <td> Seq1 </td> <td> Seq1 </td> </tr>
 * <tr> <td>  16  </td> <td> MTU </td> <td> Mask </td> </tr>
 * <tr> <td>  17  </td> <td> Salt </td> <td> Salt </td> </tr>
 * <tr> <td>  18  </td> <td> Zero </td> <td> Zero </td> </tr>
 * <tr> <td>  19  </td> <td> Zero </td> <td> Zero </td> </tr>
 * <tr> <td>  20  </td> <td> SPI </td> </tr>
 * <tr> <td>  21  </td> <td> Zero </td> </tr>
 * </table>
 *
 *
 * The following fields must be written with zeroes when not used:
 * Key4 - Key7 fields, if cipher key is 128-bit long
 * Seq1 and IS0-S2, if 32-bit packet numbering is used
 *
 * <STRONG> Context Control word </STRONG>
 * The following fields of the transform record store the parameters needed for cryptographic transformations:\n
 * <STRONG> Following Context Control word structure format is only applicable to Miura, Evora, and Europa. </STRONG>
 * <table>
 * <caption id="multi_row">Context control word 32 bit table </caption>
 * <tr><th>  Bits  <th>Name  <th>Description
 * </tr>
 * <tr> <td> [3:0] </td> <td> ToP </td> <td> Type of packet: the only valid values are 0110b for egress and 1111b for ingress </td> </tr>
 * <tr> <td> [4]  </td> <td> Reserved </td> <td> Write with zero and ignore on read </td> </tr>
 * <tr> <td> [5]  </td> <td> IV0 </td> <td> First word of IV present in context (is SCI for MACsec). Value depends on direction and presence of 64-bit packet numbering </td> </tr>
 * <tr> <td> [6]  </td> <td> IV1 </td> <td> Second word of IV present in context (is SCI for MACsec). Value depends on direction and presence of 64-bit packet numbering </td> </tr>
 * <tr> <td> [7]  </td> <td> IV2 </td> <td> Third word of IV present in context. Value depends on direction and presence of 64-bit packet numbering </td> </tr>
 * <tr> <td> [8]  </td> <td> Reserved </td> <td> Write with zero and ignore on read </td> </tr>
 * <tr> <td> [9]  </td> <td> Reserved </td> <td> Write with zero and ignore on read </td> </tr>
 * <tr> <td> [10] </td> <td> Rollover_mode </td> <td> Egress:1b = When nextPN reaching all 1's, it rolls over to value '1' without sequence number error generated.\n
 * Ingress: 1b = When packet is received with PN equal to all 1's, Sequence number field in transform record becomes '0'. This allows receiving and accepting packets with PN starting with value '1'.Key4 </td> </tr>
 * <tr> <td> [18-11] </td> <td> Reserved </td> <td> Write with zero and ignore on read </td> </tr>
 * <tr> <td> [13] </td> <td> Update seq </td> <td> Update sequence number. Must be set to 1b for MACsec.</td> </tr>
 * <tr> <td> [14] </td> <td> IV Format </td> <td> If set, use sequence number as part of IV. Value depends on direction and presence of 64-bit packet numbering.</td> </tr>
 * <tr> <td> [15] </td> <td> Encrypt auth </td> <td> If set, encrypt ICV. Must be set to 1b for MACsec.</td> </tr>
 * <tr> <td> [16] </td> <td> Key </td> <td> Load crypto key from context. Must be set to 1b for MACsec.</td> </tr>
 * <tr> <td> [19-17] </td> <td> Crypto algorithm </td> <td> Algorithm for data encryption. 101b - AES-CTR-128, 111b - AES-CTR-256. The other values are reserved </td> </tr>
 * <tr> <td> [20] </td> <td> Reserved </td> <td> Write with zero and ignore on read </td> </tr>
 * <tr> <td> [22-21] </td> <td> Digest type </td> <td> Type of digest key. Only single digest key is supported, setting 10b. </td> </tr>
 * <tr> <td> [25-23] </td> <td> Auth algorithm </td> <td> Algorithm for authentication. Only AES-GHASH is supported, setting 100b.</td> </tr>
 * <tr> <td> [27-26] </td> <td> AN </td> <td> The two-bit Association Number, which will be inserted into the SecTag for egress operations. Must be kept 00b for ingress </td> </tr>
 * <tr> <td> [29-28] </td> <td> Seq type </td> <td> Type of sequence number: 01b - for 32-bit sequence number, 10b - for 64-bit sequence numbeKey5 </td> </tr>
 * <tr> <td> [30] </td> <td> Seq mask </td> <td> Sequence mask is present in context: set to 1b for ingress, set to 0b for egress. </td> </tr>
 * <tr> <td> [31] </td> <td> Context ID </td> <td> Context ID present: must be set to 1b.</td> </tr>
 * </table>
 *
 * <STRONG> Context Control word structure format for Aperta is as follows: </STRONG>
 *
 * <table>
 * <caption id="multi_row">Context control word 32 bit table</caption>
 * <tr><th>  Bits  <th>Name  <th>Description
 * </tr>
 * <tr> <td> [3:0] </td> <td> ToP </td> <td> Type of packet:\n
 * 0110b: egress MACsec and basic AES-GCM.
 * 1111b: ingress MACsec and basic AES-GCM.
 * 0100b: AES-CTR encrypt/decrypt.
 * All other values are reserved.
 * <tr> <td> [7:4]  </td> <td> Reserved </td> <td> Write with zero and ignore on read </td> </tr>
 * <tr> <td> [8]  </td> <td> retain_icv </td> <td> Ingress-only.\n
 * If set to 1b, ICV is retained after MACsec decryption. ICV can be retained only if SecTAG is retained. If SecTAG is not retained, a context error is generated.\n
 * If set to 0b, ICV is removed after MACsec decryption. This value must be also programmed into the flow control register.
 * This value must be also programmed into the flow control register. </td> </tr>
 * <tr> <td> [9]  </td> <td> retain_sectag </td> <td> Ingress-only.\n
 * If set to 1b, SecTAG is retained after MACsec decryption.\n
 * If set to 0b, SecTAG is removed after MACsec decryption.\n
 * This value must be also programmed into the flow control register. </td> </tr>
 * <tr> <td> [10] </td> <td> rollover_mode </td> <td> Egress:1b = When nextPN reaching all 1's, it rolls over to value '1' without sequence number error generated.\n
 * Ingress: 1b = When packet is received with PN equal to all 1's, Sequence number field in transform record becomes '0'. This allows receiving and accepting packets with PN starting with value '1'. </td> </tr>
 * <tr> <td> [12:11] </td> <td> Reserved </td> <td> Write with zero and ignore on read </td> </tr>
 * <tr> <td> [13] </td> <td> update_seq </td> <td> Update sequence number control, for ingress only.\n
 * Must be set to 1b for MACsec SC with all validation settings enabled.
 * Must be set to 0b for MACsec SC that have validateFrames disabled, such that replay check still performed but context is never updated. </td> </tr>
 * <tr> <td> [14] </td> <td> Reserved </td> <td> Write with zero and ignore on read. </td> </tr>
 * <tr> <td> [15] </td> <td> Encrypt auth </td> <td> If set, Encrypt the GHASH output. Must be set to 1b. </td> </tr>
 * <tr> <td> [16] </td> <td> Reserved </td> <td> Write with zero and ignore on read. </td> </tr>
 * <tr> <td> [19:17] </td> <td> Crypto algorithm </td> <td> Algorithm for data encryption. 101b - AES-CTR-128, 111b - AES-CTR-256. The other values are reserved </td> </tr>
 * <tr> <td> [25:20] </td> <td> Reserved </td> <td> Write with zero and ignore on read </td> </tr>
 * <tr> <td> [27:26] </td> <td> AN </td> <td> The two-bit Association Number, which will be inserted into the SecTag for egress operations. Must be kept 00b for ingress </td> </tr>
 * <tr> <td> [29:28] </td> <td> Seq type </td> <td> Type of sequence number: 01b - for 32-bit sequence number, 10b - for 64-bit sequence number. The other values are reserved.  </td> </tr>
 * <tr> <td> [31:30] </td> <td> Reserved </td> <td> Write with zero and ignore on read. </td> </tr>
 * </table>
 *
 *
 * <STRONG> Context Control word structure format for Aperta2 is as follows: </STRONG>
 *
 * <table>
 * <caption id="multi_row">Context control word 32 bit table</caption>
 * <tr><th>  Bits  <th>Name  <th>Description
 * </tr>
 * <tr> <td> [3:0] </td> <td> ToP </td> <td> Type of packet:\n
 * For MACsec:\n
 * 0110b: egress MACsec and basic AES-GCM.\n 1111b: ingress MACsec and basic AES-GCM.\n 0100b: AES-CTR encrypt/decrypt.\n All other values are reserved.\n
 * For IPsec:\n
 * 0110b: egress IPsec.\n 1111b: ingress IPsec.\n
 * <tr> <td> [4]  </td> <td> late_hdr_check </td> <td> Only for ingress (MACsec and IPsec).\n
 * 1b - Enable late packet detection at SOP.\n
 * 0b - Feature is disabled.\n
 * <tr> <td> [5]  </td> <td> late_hdr_drop </td> <td> Drop action for late packet detection at SOP.\n
 * 1b - Internal drop.\n
 * 0b - Do not drop. The post-processing logic can still implement bad FCS based on late indication.
 * <tr> <td> [7:6]  </td> <td> Reserved </td> <td> Write with zero and ignore on read </td> </tr>
 * <tr> <td> [8]  </td> <td> retain_icv </td> <td> Ingress-only.\n
 * For MACsec:\n
 * If set to 1b, ICV is retained after MACsec decryption. ICV can be retained only if SecTAG is retained. If SecTAG is not retained, a context error is generated.\n
 * If set to 0b, ICV is removed after MACsec decryption. This value must be also programmed into the flow control register.\n
 * This value must be also programmed into the flow control register.\n
 * For IPsec:
 * Must be 0b.</td> </tr>
 * <tr> <td> [9]  </td> <td> retain_sectag </td> <td> Ingress-only.\n
 * For MACsec:\n
 * If set to 1b, SecTAG is retained after MACsec decryption.\n
 * If set to 0b, SecTAG is removed after MACsec decryption.\n
 * This value must be also programmed into the flow control register.\n
 * For IPsec:
 * Must be 0b.</td> </tr>
 * <tr> <td> [10] </td> <td> rollover_mode </td> <td> Egress:1b = When nextPN reaching all 1's, it rolls over to value '1' without sequence number error generated.\n
 * Ingress: 1b = When packet is received with PN equal to all 1's, Sequence number field in transform record becomes '0'. This allows receiving and accepting packets with PN starting with value '1'. </td> </tr>
 * <tr> <td> [11]  </td> <td> retain_pad </td> <td> Ingress IPsec only.\n
 * 1b - IPsec padding is not removed.\n
 * 0b - IPsec padding removal is enabled. Result depends on length of the padding.\n
 * <tr> <td> [12]  </td> <td> pad_check </td> <td> Ingress IPsec only.\n
 * 1b - IPsec pad check is enabled.\n
 * 0b - IPsec pad check is disabled.\n
 * <tr> <td> [13] </td> <td> update_seq </td> <td> Update sequence number control, for ingress only.\n
 * For MACsec:\n
 * Must be set to 1b for MACsec SC with all validation settings enabled.\n
 * Must be set to 0b for MACsec SC that have validateFrames disabled, such that replay check still performed but context is never updated.\n
 * For IPsec:
 * Must be set to 1b.</td> </tr>
 * <tr> <td> [14] </td> <td> Reserved </td> <td> Write with zero and ignore on read. </td> </tr>
 * <tr> <td> [15] </td> <td> Encrypt auth </td> <td> If set, Encrypt the GHASH output. Must be set to 1b. </td> </tr>
 * <tr> <td> [16] </td> <td> Reserved </td> <td> Write with zero and ignore on read. </td> </tr>
 * <tr> <td> [19:17] </td> <td> Crypto algorithm </td> <td> Algorithm for data encryption. 101b - AES-CTR-128,\n 111b - AES-CTR-256.\n The other values are reserved </td> </tr>
 * <tr> <td> [25:20] </td> <td> Reserved </td> <td> Write with zero and ignore on read </td> </tr>
 * <tr> <td> [27:26] </td> <td> AN </td> <td> For MACSsec:
 * The two-bit Association Number, which will be inserted into the SecTag for egress operations. Must be kept 00b for ingress\n
 * For IPsec:
 * Must be set to 0b</td> </tr>
 * <tr> <td> [29:28] </td> <td> Seq type </td> <td> Type of sequence number: 01b - for 32-bit sequence number,\n 10b - for 64-bit sequence number.\n The other values are reserved.  </td> </tr>
 * <tr> <td> [30]  </td> <td> replay_check </td> <td> Enabling IPsec replay check with the mask.\n
 * 1b - enabled (must be set for IPsec).\n
 * 0b - disabled (IPsec debugging only).\n
 * Note: Late check is always performed.
 * <tr> <td> [31]  </td> <td> ig_hdr_insert </td> <td> IPsec only.\n
 * 1b - enable inserting 4-byte SA-Tag.\n
 * 0b - disable.\n
 * </table>
 *
 *
 * <STRONG> Context ID word </STRONG>
 * This is a unique identifier for each context. In the EIP-164 it is sufficient to give all transform records a different context ID, possibly be assigning them a number from 0 to maximum index.
 * <STRONG> This field is only applicable to Miura, Evora, and Europa. </STRONG>
 *
 * <STRONG> Cryptographic parameters </STRONG>
 * The following fields of the transform record store the parameters needed for cryptographic transformations:
 *
 * <STRONG> Key 0 - Key 7 </STRONG>
 *
 * This is the AES encryption key for the MACsec SA.
 *  Each word of the key is a 32-bit integer representing four bytes of the key in little-endian order.
 * The number of words is fixed regardless of operation. The unused 32-bit words must be filled with zeroes.
 * Example: if the AES key is 128-bit long: 00_11_22_33_44_55_66_77_88_99_AA_BB_CC_DD_EE_FF, \n
 * Key 0 = 0x33221100 \n
 * Key 1 = 0x77665544 \n
 * Key 2 = 0xBBAA9988 \n
 * Key 3 = 0xFFEEDDCC \n
 *
 * <STRONG> HKey 0 - HKey 3  </STRONG>
 * This is a 128-bit key for the authentication operation. It is represented in the same byte order as Key 0...Key 7.
 * It is derived from Key 0...Key 7 as follows: H_key=E(Key, 0128). This means performing a 128-bit
 * AES-ECB block encryption operation with Key 0...Key 7 as the key and a block of 128 zero bits as the plaintext input.
 * The cipher-text result of the AES block encryption is the 128-bit H_Key.
 * Usage of the following fields depends on the direction and selection of extended packet numbering.
 * All egress transform records and ingress transform records, contain an SCI, placed at the IV0 and IV1 fields.
 * For ingress with 64 bit packet numbering the SCI is not needed in the transform record because it is not part of any operation
 *
 * <STRONG> IV0 and IV1 (SCI 0 and SCI 1) </STRONG>
 * This is the SCI that belongs to the specific MACsec SA.
 * Even in modes that do not explicitly transmit or receive the SCI with each packet, an SCI is defined,
 * which depends on the source MAC address and the ES and SCB bits. It is a 64-bit block,
 * represented by two 32-bit integers in little-endian order. This is the same byte order in which SAM_SCI_MATCH_HI/SAM_SCI_MATCH_LO represent an SCI.
 *
 * Example: the SCI is 00_01_02_03_04_05_06_07 (hexadecimal). Then: \n
 * SCI 0 = 0x03020100 \n
 * SCI 1 = 0x07060504 \n
 *
 * <STRONG> IS0, IS1 and IS2 (CtxSalt) </STRONG>
 * The egress and ingress transform records with 64-bit packet numbering also contain a 96-bit CtxSalt.
 *  This is a 96-bit Salt as described in IEEE 802.1AEbw but with the most significant 32-bits are XOR-ed with the SSCI field.
 *  This makes the actual IV for the 32 MSB. For the 64 LSB, the EIP-164 only needs to perform an XOR of the lower 64-bits of the CtxSalt with the 64-bit packet number to get the IV.
 * Example: \n
 * The Salt is E6_30_E8_1A_48_DE_86_A2_1C_66_FA_6D. \n
 * The SSCI is 7A_30_C1_18 \n
 * Then the CtxSalt is: 9C_00_29_02_48_DE_86_A2_1C_66_FA_6D \n
 *
 * The CtxSalt is placed at IS0-S2 as following: \n
 * IS0: 0x0229009C (Salt XOR-ed with SSCI) \n
 * IS1: 0xA286DE48 (Salt) \n
 * IS2: 0x6DFA661C (Salt) \n
 *
 * <STRONG> Packet numbering and replay check </STRONG>
 * The following fields are used to control MACsec packet number processing:
 * Sequence Number (Seq0 for 32-bit packet number, Seq0/1 for 64-bit packet number):\n
 *
 * For egress MACsec this is one less than the sequence number (PN) that is to be inserted into the MACsec frame.
 * For a new SA to generate the first MACsec packet with PN=1 this must be initialized to 0.
 * After each egress packet, this field is incremented by 1. If it rolls over from 0xFFFFFFFF to '0' (0xFFFFFFFF_FFFFFFFF to 0 for 64-bit packet numbering),
 * a sequence number error will occur and the context will not be updated.
 * This event is a trigger to update the flow control word pointing to the current SA. For ingress MACsec the Sequence Number should be initialized to 1.
 *
 * Mask (replay window size): \n
 * This specifies the window size for ingress sequence number checking. Value 0 is enforced for strict ordering For inbound frames,
 * the EIP-164 supports both 32-bit and 64-bit packet numbering. Replay checking for these modes is different
 * When performing replay checking for 32-bit packet numbers, the PN is compared against the
 * sequence number (PN) from the context, resulting in one of the following three cases.
 * 1. If the received number is greater or equal to the number in the context: \n
 *          received_PN >= next_PN \n
 * In this case, the context sequence number (PN) is updated (if the Update seq bit is set to 1b). The updated value is the received number plus one.
 *
 * 2. If the received number is below the number from the context, but within the greplayWindow: \n
 *          received_PN < next_PN \n
 * received_PN >= (next_PN - replayWindow) \n
 * In this case, no context update is required.
 *
 * 3. If the received number is below the number from the context, and outside the greplayWindow:\n
 * received_PN < (next_PN - replayWindow) \n
 * In this case, the sequence number check fails and error bit e10 is set in the result token. No context update is done
 *
 * When performing replay checking for 64-bit packet numbers, the lower 32-bits of the packet number are retrieved
 * from the packet and the upper 32-bits are recovered (estimated) based on replay check window size and full 64-bit value
 * (the highest packet number that was correctly recovered for the correctly processed packet, incremented with 1).
 * This value is stored in the transform record (sequence number field).
 *
 * Attention: \n
 * According to IEEE 802.1AEbw, if extended packet numbering is used, the value of the replay window should not exceed 230,
 *  even if network management software sets it to a higher value. Software must ensure that such a higher value is not programmed in the transform record.
 *
 * Attention: \n
 * For 32-bit packet numbering, this value can be up to 23^32-1, in which case any nonzero sequence number is accepted.
 *
 * Note: When the sequence number of an egress SA is about to roll over, it must be replaced by a new SA with different keys.
 * It is not allowed to reset the Sequence Number of an egress SA to a lower value - doing so will (in general) lead to Sequence Number checking failures at the receiving end of the connection.
 *
 * For IPsec, replay window size must be configured to 128-bits
 *
 * <STRONG> SA update control word </STRONG>
 * For automatic SA expiry (all directions) and switching (egress-only MACsec), the transform record
 * contains an SA update control word that controls SA update when the packet number for the current SA is expired.
 *
 * <table>
 * <caption id="multi_row"> SA update control word </caption>
 * <tr><th>  Bits  <th>Name  <th>Description
 * </tr>
 * <tr> <td> [13-0] </td> <td> sa_index </td> <td> Egress: Index of the next SA. This index is valid only if sa_index_valid is set to 1b. \n
 *                                 Ingress: Index of the current SA. This value is used to set bit in the SA_EXP_SUMMARY register. </td> </tr>
 * <tr> <td> [14] </td> <td> sa_expired_irq </td> <td> When set to 1b, allows annotation into the SA expired summary register and generates a subsequent interrupt when the SA expires.
 * For the egress processing, this is performed regardless whether the next SA is available.
 * When set to 0b, no annotation in the SA expired summary register is done.\n
 * Note: This field is valid only if update_en set to 1bWrite with zero and ignore on read </td> </tr>
 * <tr> <td> [15] </td> <td> sa_index_valid </td> <td> Egress: Next SA index valid. When set to 1b, indicates that the sa_index field contains a valid index pointing to the installed transform record.
 * If the next SA is not installed yet, this field must be set to 0b. If the next SA was not installed in time and the current SA expired,
 * the SA is invalidated and an SA expired interrupt is generated along with a flag set in the SA expired summary register.
 * Ingress: Must be to zero becase on ingress processing only expiration can be performed </td> </tr>
 * <tr> <td>  [28:16] </td> <td> sc_index </td> <td> Egress: Index of the TxSC for the current SA. If SA switching is successful, it gets the next SA index.
 * Ingress: Index of the RxSC for the current SA </td> </tr>
 * <tr> <td>  [30:29]  </td> <td> sa_an </td> <td> Egress: Must be to zero.
 * Ingress: AN number of the SA </td> </tr>
 * <tr> <td> [31] </td> <td> update_en </td> <td> Egress: SA index update enable. If set to 1b, enables the SA index update when the Packet Number for the current transform record has expired.
 * The current SA index in the corresponding flow control register is updated with the sa_index if sa_index_valid is 1b.
 * If sa_index_valid is 0b, the SA is invalidated by setting the value of the sa_in_use field to 0b in the flow control register.
 * Ingress: SA expiration enable. If set to 1b, enables the automatic inUse flag clearing when the Packet Number for the current transform record has expired </td> </tr>
 * </table>
 *
 * <STRONG> MTU (This field is only applicable to Aperta and Aperta2) </STRONG>
 * The context record contains a 16-bit MTU value and the related flags to control usage of maximum size control.
 * This functionality is required for egress processing to check the MTU versus the expanded packet as required by IEEE 802.1AE
 *
 * <table>
 * <caption id="multi_row"> MTU </caption>
 * <tr><th>  Bits  <th>Name  <th>Description
 * </tr>
 * <tr> <td> [15:0] </td> <td> mtu </td> <td> Maximum size of the packet. This value is output with start of packet. </td> </tr>
 * <tr> <td> [16] </td> <td> enable </td> <td> When set to 1b, it enables usage of mtu value for comparison with output length.
 * If output length is greater than mtu value, the sa.OutPktsTooLong counter is incremented. </td> </tr>
 * <tr> <td> [17] </td> <td> drop </td> <td> Specifies action if encrypted packet is larger than MTU field:\n
 * If set to 0b, packet is not dropped and forwarded as good packet if all other operations are succeeded.\n
 * If set to 1b, packet is dropped via CRC error signaling. </td> </tr>
 * <tr> <td>  [31:18] </td> <td> reserved </td> <td> Write with zero and ignore on read </td> </tr>
 * </table>
 *
 * <STRONG> Egress SPI (This field is only applicable to Aperta2) </STRONG>
 * An SPI value that is going to be inserted into the packet must be placed in network byte order format.
 *
 * \arg params \n
 *     SA parameters based on MACsec/IPsec/Crypt operation\n
 *
 * \arg action_type \n
 *     SA action type, see bcm_plp_secy_SA_action_type_t \n
 *
 * \arg drop_type \n
 *     SA drop type, see bcm_plp_secy_drop_type_t \n
 *
 * \arg dest_port \n
 *     Destination port \n
 *
 *
 */
typedef struct {
    unsigned int sa_word_count;
    unsigned int * transform_record_p;
    union {
        bcm_plp_secy_sa_e_t egress;
        bcm_plp_secy_sa_i_t ingress;
        bcm_plp_secy_sa_bd_t bypass_drop;
        bcm_plp_secy_sa_ca_t crypt_auth;
        bcm_plp_secy_sa_ipsec_e_t  ipsec_egress;
        bcm_plp_secy_sa_ipsec_i_t  ipsec_ingress;
    } params;
    bcm_plp_secy_sa_action_type_t action_type;
    bcm_plp_secy_drop_type_t drop_type;
    bcm_plp_secy_port_type_t dest_port;
} bcm_plp_secy_sa_t;


/*! MACsec SecTAG parsing rules,
 * these rules classify each packet into one of four categories, \n
 * see also the bcm_plp_secy_rules_sa_nonmatch_t data structure: \n
 *  1) untagged, the packet has no MACsec tag, \n
 *    i.e. the Ether-type differs from 0x88E5. \n
 *  2) Bad tag, the packet has an invalid MACsec tag \n
 *  3) kay tag, the packet has a kay tag. These packets are generated and/or \n
 *     handled by application software and no MACsec processing is performed \n
 *     for them by the Classification device except for straight bypass. \n
 *  4) tagged, the packet has a valid MACsec tag that is not kay. \n
 *
 * \arg fcomp_etype \n
 *    Compare Ether-type in packet against ether_type value. \n
 *    false - all packets are treated as MACsec(no packets are classified as untagged). \n
 *    default is true \n
 *
 * \arg fcheck_v \n
 *    true - check V bit to be 0 \n
 *    default is true \n
 *
 * \arg fcheck_kay \n
 *    true - check if this is a kay packet (C and E bits) \n
 *    default is true \n
 *
 * \arg fcheck_ce \n
 *    true - check illegal C and E bits combination (C=1 and E=0) \n
 *    default is true \n
 *
 * \arg fcheck_sc \n
 *    true - check illegal SC/ES/SCB bits combinations \n
 *    default is true \n
 *
 * \arg fcheck_sl \n
 *    true - check if SL (Short Length) field value is out of range \n
 *    default is true \n
 *
 * \arg fcheck_pn \n
 *    true - check PN (Packet Number) is non-zero \n
 *    default is true \n
 *
 * \arg fouter_sa_pbb \n
 *     false - Use inner MAC SA location for PBB packets. Inner MAC header \n
 *             location is provided by the external classifier. If inner MAC \n
 *             header is not found, the outer MAC header is used. \n
 *     true - Use outer MAC SA location for PBB packets (byte offset 6) \n
 *     default is false \n
 *
 * \arg fouter_sa_eompls \n
 *     false - Use inner MAC SA location for EoMPLS packets. Inner MAC header \n
 *             location is provided by the external classifier. If inner MAC \n
 *             header is not found, the outer MAC header is used. \n
 *     true - Use outer MAC SA location (byte offset 6) \n
 *     default is false \n
 *
 * \arg ether_type \n
 *     Ether-type value used for MACsec tag type comparison default is 0x88E5 \n
 *
 * \arg fcheck_sl_ext \n
 *     true- check if SL (Short Length) field value is consistent with packet length. \n
 *
 */
typedef struct {
    unsigned char fcomp_etype;
    unsigned char fcheck_v;
    unsigned char fcheck_kay;
    unsigned char fcheck_ce;
    unsigned char fcheck_sc;
    unsigned char fcheck_sl;
    unsigned char fcheck_pn;
    unsigned char fouter_sa_pbb;
    unsigned char fouter_sa_eompls;
    unsigned short int ether_type;
    unsigned char fcheck_sl_ext;
} bcm_plp_secy_rule_sectag_t;

/*! MTU checking rule for packet post-processing of a Secure Channel (egress only)
 *
 * \arg packet_max_byte_count \n
 *    Maximum allowed packet size (in bytes) \n
 *
 * \arg fover_size_drop \n
 *    Action indication, if a packets is longer: \n
 *     true  - Drop packet (by corrupting the CRC) \n
 *     false - Pass packet on anyway \n
 */
typedef struct {
    unsigned short packet_max_byte_count;
    unsigned char fover_size_drop;
} bcm_plp_secy_sc_rule_mtu_check_t;

/*! Actions per packet that did not match any SA
 *
 * \arg fbypass \n
 *    Flow action type \n
 *    true  - bypass, \n
 *    false - perform drop action, see bcm_plp_secy_drop_type_t \n
 *
 * \arg drop_type \n
 *     Packet drop type, see bcm_plp_secy_drop_type_t \n
 *
 * \arg fdrop_non_reserved \n
 *     ingress only, Perform drop action if packet is not from the reserved port \n
 *
 * \arg dest_port \n
 *     The destination port type \n
 */
typedef struct {
    unsigned char fbypass;
    bcm_plp_secy_drop_type_t drop_type;
    unsigned char fdrop_non_reserved;
    bcm_plp_secy_port_type_t dest_port;
} bcm_plp_secy_rules_non_sa_t;

/*! SecY settings
 *
 * \arg seqnr_threshold \n
 *      Outbound sequence number threshold value (one global for all SA's) \n
 *      When non-0 the SecY device will generate a notification to indicate \n
 *      the threshold event which can be used to start the re-keying procedure. \n
 *      The notification will be generated only if it is requested \n
 *      by means of the bcm_plp_secy_Notify_Request() function. \n
 *      If set to zero then only the sequence number roll-over notification \n
 *      will be generated. \n
 *      Note: This is a global parameter for all the SA's added to one SecY device. \n
 *
 * \arg seqnr_threshold_64_lo \n
 * \arg seqnr_threshold_64_hi \n
 *      Outbound sequence number threshold value for 64-bit packet numbering \n
 *
 * \arg sa_count_frame_thr_lo \n
 * \arg sa_count_frame_thr_hi \n
 * \arg secy_count_frame_thr_lo \n
 * \arg secy_count_frame_thr_hi \n
 * \arg ifc_count_frame_thr_lo \n
 * \arg ifc_count_frame_thr_hi \n
 * \arg ifc1_count_frame_thr_lo \n
 * \arg ifc1_count_frame_thr_hi \n
 * \arg rxcam_count_frame_thr_lo \n
 * \arg rxcam_count_frame_thr_hi \n
 *     Threshold for the frame counters \n
 *
 * \arg sa_count_octet_thr_lo \n
 * \arg sa_count_octet_thr_hi \n
 * \arg ifc_count_octet_thr_lo \n
 * \arg ifc_count_octet_thr_hi \n
 * \arg ifc1_count_octet_thr_lo \n
 * \arg ifc1_count_octet_thr_hi \n
 *     Threshold for the octet counters \n
 *
 * \arg count_inc_dis_ctrl \n
 *     Counter to increment enable control
 *
 * \arg fflow_cypt_auth \n
 *     Enable the Crypt-authenticate operation (all vports) \n
 *
 * \arg drop_bypass \n
 *     Non-match flows (drop, bypass) control (all vports) \n
 *
 * \arg max_channel_count \n
 *     Specify the maximum number of channels to be used by the driver.
 *     If set to zero, use the maximum supported by the hardware
 *
 * \arg max_vport_count \n
 *     Specify the maximum number of vPorts to be used by the driver.
 *     If set to zero, use the maximum supported by the hardware
 *
 * \arg max_sa_count \n
 *     Specify the maximum number of SAs to be used by the driver.
 *     If set to zero, use the maximum supported by the hardware *
 * 
 * \arg max_sc_count
 *     Specify the maximum number of SCs to be used by the driver.
 *     If set to zero, use the maximum supported by the hardware
 *
 * \arg eop_timeout_val \n
 *     Control of automatic EOP insertion, timeout value. This field is only valid for applicable PHYs. \n
 *
 * \arg eop_timeout_ctrl \n
 *     Control of automatic EOP insertion, bit mask to specify channels (0 to 31). This field is only valid for applicable PHYs. \n
 *
 * \arg ecc_correctable_thr \n
 *     Threshold for the correctable ECC error counters. This field is only valid for applicable PHYs. \n
 *
 * \arg ecc_uncorrectable_thr \n
 *     Threshold for the uncorrectable ECC error counters. This field is only valid for applicable PHYs. \n
 *
 * \arg eop_timeout_ctrl_ext \n
 *     Control of automatic EOP insertion, bit mask to specify channels (32 to 63). This field is only valid for applicable PHYs. \n
 *
 * * \arg fdrop_control_pkts \n
 *     Drop control packets when no valid vPort exists.\n
 *
 * \arg sa_retire_timer_prescale \n
 *     Prescale for SA retire timer, set to 0 to disable the feature \n
 *
 * \arg sa_retire_timeout \n
 *     Timeout value for SA retire timer \n
 *
 * \arg ingress_hdr_etype \n
 *     Ingress Header type - Global for device - Applicable only for IPsec \n
 *
 */
typedef struct {
    unsigned int seqnr_threshold;
    unsigned int seqnr_threshold_64_lo;
    unsigned int seqnr_threshold_64_hi;
    unsigned int sa_count_frame_thr_lo;
    unsigned int sa_count_frame_thr_hi;
    unsigned int secy_count_frame_thr_lo;
    unsigned int secy_count_frame_thr_hi;
    unsigned int ifc_count_frame_thr_lo;
    unsigned int ifc_count_frame_thr_hi;
    unsigned int ifc1_count_frame_thr_lo;
    unsigned int ifc1_count_frame_thr_hi;
    unsigned int rxcam_count_frame_thr_lo;
    unsigned int rxcam_count_frame_thr_hi;
    unsigned int sa_count_octet_thr_lo;
    unsigned int sa_count_octet_thr_hi;
    unsigned int ifc_count_octet_thr_lo;
    unsigned int ifc_count_octet_thr_hi;
    unsigned int ifc1_count_octet_thr_lo;
    unsigned int ifc1_count_octet_thr_hi;
    unsigned char count_inc_dis_ctrl;
    unsigned char fflow_cypt_auth;
    bcm_plp_secy_rules_non_sa_t  drop_bypass;
    unsigned int max_channel_count;
    unsigned int max_vport_count;
    unsigned int max_sa_count;
    unsigned int max_sc_count;
    unsigned int eop_timeout_val;
    unsigned int eop_timeout_ctrl;
    unsigned int ecc_correctable_thr;
    unsigned int ecc_uncorrectable_thr;
    unsigned int eop_timeout_ctrl_ext;
    unsigned char fdrop_control_pkts;
    unsigned int sa_retire_timer_prescale;
    unsigned int sa_retire_timeout;
    unsigned short ingress_hdr_etype;
} bcm_plp_secy_settings_t;

/*! Egress SA statistics
 *
 * \arg out_pkts_encrypted_protected \n
 *     Packet counters - Encrypted Protected \n
 *
 * \arg out_pkts_encrypted_protected \n
 *     Packet counters - TooLong \n
 *
 * \arg out_pkts_encrypted_protected \n
 *     Packet counters - SA Not In Use \n
 *
 * \arg out_octets_encrypted_protected \n
 *     Octet counters - Encrypted Protected \n
*/
typedef struct {
    bcm_plp_secy_stat_counter_t out_pkts_encrypted_protected;
    bcm_plp_secy_stat_counter_t out_pkts_too_long;
    bcm_plp_secy_stat_counter_t out_pkts_sa_not_inuse;
    bcm_plp_secy_stat_counter_t out_octets_encrypted_protected;
} bcm_plp_secy_sa_stat_e_t;

/*! Ingress SA statistics
 *
 * \arg in_pkts_unchecked \n
 *     Packet counters - Unchecked \n
 *
 * \arg in_pkts_delayed \n
 *     Packet counters - Delayed \n
 *
 * \arg in_pkts_late \n
 *     Packet counters - Late \n
 *
 * \arg in_pkts_ok \n
 *     Packet counters - OK \n
 *
 * \arg in_pkts_invalid \n
 *     Packet counters - Invalid \n
 *
 * \arg in_pkts_not_valid \n
 *     Packet counters - Not Valid \n
 *
 * \arg in_pkts_not_using_sa \n
 *     Packet counters - Not Using SA \n
 *
 * \arg in_pkts_unused_sa \n
 *     Packet counters - Unused SA \n
 *
 * \arg in_octets_decrypted \n
 *     Octet counters - Decrypted \n
 *
 * \arg in_octets_validated \n
 *     Octet counters - Validated \n
*/
typedef struct {
    bcm_plp_secy_stat_counter_t in_pkts_unchecked;
    bcm_plp_secy_stat_counter_t in_pkts_delayed;
    bcm_plp_secy_stat_counter_t in_pkts_late;
    bcm_plp_secy_stat_counter_t in_pkts_ok;
    bcm_plp_secy_stat_counter_t in_pkts_invalid;
    bcm_plp_secy_stat_counter_t in_pkts_not_valid;
    bcm_plp_secy_stat_counter_t in_pkts_not_using_sa;
    bcm_plp_secy_stat_counter_t in_pkts_unused_sa;
    bcm_plp_secy_stat_counter_t in_octets_decrypted;
    bcm_plp_secy_stat_counter_t in_octets_validated;
} bcm_plp_secy_sa_stat_i_t;

/*! SA counters for IPSec Ingress
 *
 * \arg in_pkts_pad_not_valid \n
 *     Packet counters - Pad check for NH byte and pad pattern failed \n
 *
 * \arg in_pkts_replayed \n
 *     Packet counters - Replayed \n
 *
 * \arg in_pkts_late \n
 *     Packet counters - Late \n
 *
 * \arg in_pkts_ok \n
 *     Packet counters - OK \n
 *
 * \arg in_pkts_pad_len_fail \n
 *     Packet counters - Pad length consistency check failed \n
 *
 * \arg in_pkts_not_valid \n
 *     Packet counters - Not Valid \n
 *
 * \arg in_pkts_not_using_sa \n
 *     Packet counters - Not Using SA \n
 *
 * \arg in_pkts_pad_dummy \n
 *     Packet counters - Packet passed authentication check but has \n
 *     dummy in the next header in its padding \n
 *
 * \arg in_octets_decrypted \n
 *     Octet counters - Decrypted \n
 *
 * \arg in_octets_validated \n
 *     Octet counters - Validated \n
 */
typedef struct
{
    bcm_plp_secy_stat_counter_t in_pkts_pad_not_valid;
    bcm_plp_secy_stat_counter_t in_pkts_replayed;
    bcm_plp_secy_stat_counter_t in_pkts_late;
    bcm_plp_secy_stat_counter_t in_pkts_ok;
    bcm_plp_secy_stat_counter_t in_pkts_pad_len_fail;
    bcm_plp_secy_stat_counter_t in_pkts_not_valid;
    bcm_plp_secy_stat_counter_t in_pkts_not_using_sa;
    bcm_plp_secy_stat_counter_t in_pkts_pad_dummy;
    bcm_plp_secy_stat_counter_t in_octets_decrypted;
    bcm_plp_secy_stat_counter_t in_octets_validated;
} bcm_plp_secy_sa_stat_ipsec_i_t;

/*! Union for SA Statistics counters for MACsec Ingress, Egress and IPsec Ingress*/
typedef union {
    bcm_plp_secy_sa_stat_e_t egress;
    bcm_plp_secy_sa_stat_i_t ingress;
    bcm_plp_secy_sa_stat_ipsec_i_t ipsec_ingress;
} bcm_plp_secy_sa_stat_t;

/*! Egress SecY statistics
 *
 * \arg out_pkts_transform_error \n
 *     Packet counters - Transform Error \n
 *
 * \arg out_pkts_control \n
 *     Packet counters - Control \n
 *
 * \arg out_pkts_untagged \n
 *     Packet counters - Untagged \n
 *
 * \arg out_pkts_sa_not_in_use \n
 *     Packet counters - SA not in use \n
*/
typedef struct {
    bcm_plp_secy_stat_counter_t out_pkts_transform_error;
    bcm_plp_secy_stat_counter_t out_pkts_control;
    bcm_plp_secy_stat_counter_t out_pkts_untagged;
    bcm_plp_secy_stat_counter_t out_pkts_sa_not_in_use;
} bcm_plp_secy_secy_stat_e_t;

/*! Ingress SecY Counters
 *
 * \arg in_pkts_transform_error \n
 *     Packet counters - Transform Error \n
 *
 * \arg in_pkts_control \n
 *     Packet counters - Control \n
 *
 * \arg in_pkts_untagged \n
 *     Packet counters - Untagged \n
 *
 * \arg in_pkts_no_tag \n
 *     Packet counters - No Tag \n
 *
 * \arg in_pkts_badtag \n
 *     Packet counters - Bad Tag \n
 *
 * \arg in_pkts_no_sci \n
 *     Packet counters - No SCI \n
 *
 * \arg in_pkts_unknown_sci \n
 *     Packet counters - Unknown SCI \n
 *
 * \arg in_pkts_tagged_ctrl \n
 *     Packet counters - Tagged Ctrl \n
 *
 * \arg in_pkts_sa_not_in_use \n
 *     Packet counters - SA not in use \n
 *
*/
typedef struct {
    bcm_plp_secy_stat_counter_t in_pkts_transform_error;
    bcm_plp_secy_stat_counter_t in_pkts_control;
    bcm_plp_secy_stat_counter_t in_pkts_untagged;
    bcm_plp_secy_stat_counter_t in_pkts_no_tag;
    bcm_plp_secy_stat_counter_t in_pkts_badtag;
    bcm_plp_secy_stat_counter_t in_pkts_no_sci;
    bcm_plp_secy_stat_counter_t in_pkts_unknown_sci;
    bcm_plp_secy_stat_counter_t in_pkts_tagged_ctrl;
    bcm_plp_secy_stat_counter_t in_pkts_sa_not_in_use;
} bcm_plp_secy_secy_stat_i_t;

/*! SecY IPsec Egress statistics
 *
 * \arg out_pkts_transform_error \n
 *     Packet counters - Transform Error \n
 *
 * \arg out_pkts_untagged \n
 *     Packet counters - Untagged \n
 *
 * \arg out_pkts_sa_not_in_use \n
 *     Packet counters - Not Using SA \n
 */
typedef struct
{
    bcm_plp_secy_stat_counter_t out_pkts_transform_error;
    bcm_plp_secy_stat_counter_t out_pkts_untagged;
    bcm_plp_secy_stat_counter_t out_pkts_sa_not_in_use;
} bcm_plp_secy_secy_stat_ipsec_e_t;

/*! SecY IPsec Ingress counters
 *
 * \arg in_pkts_transform_error \n
 *     Packet counters - Transform Error \n
 *
 * \arg in_pkts_late_hdr \n
 *     Packet counters - Counts packets that are late at SOP \n
 *
 * \arg in_pkts_ip_mismatch \n
 *     Packet counters - Counts packet that have mismatch of outer IP \n
 *     header and inner IP header type. \n
 *
 * \arg in_pkts_sa_not_in_use \n
 *     Packet counters - Not Using SA \n
 */
typedef struct
{
    bcm_plp_secy_stat_counter_t in_pkts_transform_error;
    bcm_plp_secy_stat_counter_t in_pkts_late_hdr;
    bcm_plp_secy_stat_counter_t in_pkts_ip_mismatch;
    bcm_plp_secy_stat_counter_t in_pkts_sa_not_in_use;
} bcm_plp_secy_secy_stat_ipsec_i_t;
/*! Union for SecY Statistics counters for MACsec, IPsec Ingress & Egress */
typedef union {
    bcm_plp_secy_secy_stat_e_t egress;
    bcm_plp_secy_secy_stat_i_t ingress;
    bcm_plp_secy_secy_stat_ipsec_e_t ipsec_egress;
    bcm_plp_secy_secy_stat_ipsec_i_t ipsec_ingress;
} bcm_plp_secy_secy_stat_t;

/*! IFC (interface) statistics */
typedef struct {
    bcm_plp_secy_stat_counter_t out_pkts_unicast_uncontrolled;
    bcm_plp_secy_stat_counter_t out_pkts_multicast_uncontrolled;
    bcm_plp_secy_stat_counter_t out_pkts_broadcast_uncontrolled;
    bcm_plp_secy_stat_counter_t out_rx_drop_pkts_uncontrolled;
    bcm_plp_secy_stat_counter_t out_rx_err_pkts_uncontrolled;
    bcm_plp_secy_stat_counter_t out_pkts_unicast_controlled;
    bcm_plp_secy_stat_counter_t out_pkts_multicast_controlled;
    bcm_plp_secy_stat_counter_t out_pkts_broadcast_controlled;
    bcm_plp_secy_stat_counter_t out_rx_drop_pkts_controlled;
    bcm_plp_secy_stat_counter_t out_rx_err_pkts_controlled;
    bcm_plp_secy_stat_counter_t out_octets_uncontrolled;
    bcm_plp_secy_stat_counter_t out_octets_controlled;
    bcm_plp_secy_stat_counter_t out_octets_common;
} bcm_plp_secy_ifc_stat_e_t;


/*! Ingress IFC (interface) statistics
 *
 * \arg in_pkts_unicast_uncontrolled \n
 *     Packet counters - Unicast Uncontrolled \n
 *
 * \arg in_pkts_multicast_uncontrolled \n
 *     Packet counters - Multicast Uncontrolled \n
 *
 * \arg in_pkts_broadcast_uncontrolled \n
 *     Packet counters - Broadcast Uncontrolled \n
 *
 * \arg in_rx_drop_pkts_uncontrolled \n
 *     Packet counters - In Rx Dropped UnControlled \n
 *
 * \arg in_rx_err_pkts_uncontrolled \n
 *     Packet counters - In Rx Error UnControlled \n
 *
 * \arg in_pkts_unicast_controlled \n
 *     Packet counters - Unicast Controlled \n
 *
 * \arg in_pkts_multicast_controlled \n
 *     Packet counters - Multicast Controlled \n
 *
 * \arg in_pkts_broadcast_controlled \n
 *     Packet counters - Broadcast Controlled \n
 *
 * \arg in_rx_drop_pkts_controlled \n
 *     Packet counters - In Rx Dropped Controlled \n
 *
 * \arg in_rx_err_pkts_controlled \n
 *     Packet counters - In Rx Error Controlled \n
 *
 * \arg in_octets_uncontrolled \n
 *     Octet counters - Uncontrolled \n
 *
 * \arg in_octets_controlled \n
 *     Octet counters - Controlled \n
*/
typedef struct {
    bcm_plp_secy_stat_counter_t in_pkts_unicast_uncontrolled;
    bcm_plp_secy_stat_counter_t in_pkts_multicast_uncontrolled;
    bcm_plp_secy_stat_counter_t in_pkts_broadcast_uncontrolled;
    bcm_plp_secy_stat_counter_t in_rx_drop_pkts_uncontrolled;
    bcm_plp_secy_stat_counter_t in_rx_err_pkts_uncontrolled;
    bcm_plp_secy_stat_counter_t in_pkts_unicast_controlled;
    bcm_plp_secy_stat_counter_t in_pkts_multicast_controlled;
    bcm_plp_secy_stat_counter_t in_pkts_broadcast_controlled;
    bcm_plp_secy_stat_counter_t in_rx_drop_pkts_controlled;
    bcm_plp_secy_stat_counter_t in_rx_err_pkts_controlled;
    bcm_plp_secy_stat_counter_t in_octets_uncontrolled;
    bcm_plp_secy_stat_counter_t in_octets_controlled;
} bcm_plp_secy_ifc_stat_i_t;

/*! Union for Ifc Statistics counters for both Ingress & Egress */
typedef union {
    bcm_plp_secy_ifc_stat_e_t egress;
    bcm_plp_secy_ifc_stat_i_t ingress;
} bcm_plp_secy_ifc_stat_t;

/*! RxCAM statistics (ingress only) */
typedef struct {
    bcm_plp_secy_stat_counter_t cam_hit;
} bcm_plp_secy_rxcam_stat_t;

/*! secy device interrupt
 *
 *  \arg event_mask
 *      The requested events, bitwise OR of BCM_PLP_SECY_EVENT_* global events or
 *      BCM_PLP_SECY_EVENT_CHAN_* channel-specific events
 *
 *  \arg [IN] f_global
 *      True if notification is for the device Global AIC,
 *      otherwise the notification is for the device Channel AIC
 */
typedef struct {
    unsigned int event_mask;
    unsigned char f_global;
} bcm_plp_secy_intr_t;


typedef int (*bcm_plp_sec_mutex_take_f)(void* user_acc);
typedef int (*bcm_plp_sec_mutex_give_f)(void* user_acc);

/*! bcm_plp_sec_mutex_t
 * bcm_plp_sec mutex struct
 *  \arg SecY0_mutex_take;
 *   Mutex lock function for egress SecY apis
 *
 *  \arg SecY1_mutex_take;
 *   Mutex lock function for ingress SecY apis
 *
 *  \arg SecY0_mutex_give;
 *  Mutex unlock function for egress SecY apis
 *
 *  \arg SecY1_mutex_give;
 *  Mutex unlock function for igress SecY apis
 *
 *  \arg CfyE0_mutex_take;
 *  Mutex lock function for egress CfyE apis
 *
 *  \arg CfyE1_mutex_take;
 *  Mutex lock function for igress CfyE apis
 *
 *  \arg CfyE0_mutex_give;
 *  Mutex unlock function for egress CfyE apis
 *
 *  \arg CfyE1_mutex_give;
 *  Mutex unlock function for igress CfyE apis
 *
 */


typedef struct bcm_plp_sec_mutex_s {
    bcm_plp_sec_mutex_take_f SecY0_mutex_take; /**< egress SecY mutex */
    bcm_plp_sec_mutex_take_f SecY1_mutex_take; /**< igress SecY mutex */
    bcm_plp_sec_mutex_give_f SecY0_mutex_give; /**< egress SecY mutex */
    bcm_plp_sec_mutex_give_f SecY1_mutex_give; /**< igress SecY mutex */
    bcm_plp_sec_mutex_take_f CfyE0_mutex_take; /**< egress CfyE mutex */
    bcm_plp_sec_mutex_take_f CfyE1_mutex_take; /**< igress CfyE mutex */
    bcm_plp_sec_mutex_give_f CfyE0_mutex_give; /**< egress CfyE mutex */
    bcm_plp_sec_mutex_give_f CfyE1_mutex_give; /**< igress CfyE mutex */
} bcm_plp_sec_mutex_t;

/*! bcm_plp_sec_phy_access_t
 *  bcm_plp_sec access struct
 *
 * \arg phy_info
 *  bcm_plp_access_t structure.
 *
 * <STRONG> Note: phy_info mapping is for per port </STRONG>
 *
 * \arg bcm_plp_sec_mutex_t mutex;
 *  mutex lock for CfyE and SecY
 *  Applications can use bcm_plp_sec_mutex_take_f to init their callback functions and
 *  and assign to mutex during bcm_cfye_device_init and bcm_secy_device_init.
 *  This assignment only required during bcm_cfye_device_init and bcm_secy_device_init.
 *  Same callback functions will be used for all other cfye and secy apis for lock and unlock.
 *
 *  \arg macsec_side (input)
 *   MAcSec side identifier, either Ingress or Egress.\n
 *    macsec_side can be "0" or "1" \n
 * <STRONG> macsec_side = 1 for Ingress only,</STRONG> \n
 * <STRONG> macsec_side = 0 for Egress only </STRONG>
 */
typedef struct bcm_plp_sec_phy_access_s {
    bcm_plp_access_t phy_info;
    bcm_plp_sec_mutex_t mutex;  /**< mutex lock for CfyE and SecY APIs */
    unsigned int macsec_side;
} bcm_plp_sec_phy_access_t;



/*! Device register access
 *
 *  \arg start_addr
 *      Starting address of the registers
 *
 *  \arg [IN] count
 *      Number of entries/registers to read/write
 *      Supports maximum of 30 entries/registers to read/write 
 *
 *  \arg [IN/OUT] data
 *      Pointer to the memory where the values to be written are located based on count[Write]
 *      Pointer to the memory where the retrieved words will be stored based on count[Read]
 */
typedef struct {
    unsigned int start_addr;
    unsigned char count;
    plp_uint64_t *data;
} bcm_plp_sec_reg_access_t;


/*! Device statistics control
 *
 * \arg fauto_stat_cntrs_reset
 *      true - statistics counters are automatically reset on a read operation\n
 *      false - no reset on a read operation
 * \arg seq_nr_threshold
 *      Outbound sequence number threshold value (one global for all SA's)\n
 *      When non-0 the SecY device will generate a notification to indicate
 *      the threshold event which can be used to start the re-keying
 *      procedure.\n
 *      Note: This is a global parameter for all the SA's added to
 *              one SecY device.
 * \arg seq_nr_threshold_64
 *      Outbound sequence number threshold value for 64-bit packet numbering
 * \arg sa_count_frame_thr
 *      Threshold for the SA frame counters
 * \arg secy_count_frame_thr
 *      Threshold for the SecY frame counters
 * \arg ifc_count_frame_thr
 *      Threshold for the IFC frame counters
 * \arg ifc1_count_frame_thr
 *      Threshold for the IFC1 frame counters
 * \arg rx_cam_count_frame_thr
 *      Threshold for the RxCAM frame counters
 * \arg sa_count_octet_thr
 *      Threshold for the SA octet counters
 * \arg ifc_count_octet_thr
 *      Threshold for the IFC octet counters
 * \arg ifc1_count_octet_thr
 *      Threshold for the IFC1 octet counters
 * \arg count_inc_dis_ctrl
 *      Counter increment enable control
 */
typedef struct {
    unsigned char fauto_stat_cntrs_reset;
    unsigned int seq_nr_threshold;
    bcm_plp_secy_stat_counter_t seq_nr_threshold_64;
    bcm_plp_secy_stat_counter_t sa_count_frame_thr;
    bcm_plp_secy_stat_counter_t secy_count_frame_thr;
    bcm_plp_secy_stat_counter_t ifc_count_frame_thr;
    bcm_plp_secy_stat_counter_t ifc1_count_frame_thr;
    bcm_plp_secy_stat_counter_t rx_cam_count_frame_thr;
    bcm_plp_secy_stat_counter_t sa_count_octet_thr;
    bcm_plp_secy_stat_counter_t ifc_count_octet_thr;
    bcm_plp_secy_stat_counter_t ifc1_count_octet_thr;
    unsigned char count_inc_dis_ctrl;
} bcm_plp_secy_statistics_control_t;


/*! Channel statistics control (thresholds)
 *
 * \arg seq_nr_threshold
 *      Outbound sequence number threshold value
 *      When non-0 the SecY device will generate a notification to indicate
 *      the threshold event which can be used to start the re-keying procedure.
 *      The notification will be generated only if it is requested
 *      NONE: This is a global parameter for all the SA's added to one SecY device.
 *
 * \arg seq_nr_threshold_64
 *      Outbound sequence number threshold value for 64-bit packet numbering.
 */
typedef struct {
    unsigned int seq_nr_threshold;
    bcm_plp_secy_stat_counter_t seq_nr_threshold_64;
} bcm_plp_secy_channel_stat_control_t;


/*! Channel configuration parameters
 *
 * \arg flow_latency_bypass
 *     Indicates if the low latency bypass must be enabled (true) for channel
 *     or the MACsec mode (false)
 *
 * \arg flatency_enable
 *     Indicates if the channel-specific latency must be enabled (true),
 *     this can be enabled for MACsec channels only with
 *     flow_latency_bypass set to false, otherwise this setting will be ignored
 *
 * \arg latency
 *     Channel-specific latency this can be set for MACsec channels only with
 *     flow_latency_bypass set to false, otherwise this setting will be ignored
 *
 * \arg burst_limit
 *     Burst limit. Set to 0 to disable burst limiting, set to value between
 *     4 and 15 to insert a dummy cycle after BurstLimit transfers
 *
 * \arg stat_ctrl
 *    Channel statistics control settings (threshold values)
 *
 * \arg fpkt_num_thr_mode
 *    Mode for packet sequence number threshold comparison:\n
 *         false - comparison is greater or equal,\n
 *         true  - comparison is strictly equal.
 *
 * \arg ether_type
 *    EtherType field for MACsec to be inserted in the SecTAG for outbound
 *    MACsec packet processing. Default (reset) value is 0x88E5.
 *
 * \arg rule_sectag
 *    MACsec SecTAG parsing rules, see above for details
 *
 * \arg fshould_secure
 *    set channel to should-secure
 *
 * \arg fipsec
 *    1 - for IPsec mode
 *
 * \arg fbypass_status
 *    Read-only, Per-channel bypass status
 *
 * \arg fbypass_mode
 *    bypass mode
 *
 * \arg fpad_nh_check
 *    Enable IPsec pad Next Header field check
 *
 * \arg fpad_len_check
 *    Enable IPsec pad length field check
 *
 * \arg fpad_pattern_check
 *    Enable IPsec pad pattern field check
 *
 * \arg fpad_dummy_check
 *    Enable IPsec dummy packet detection
 */

typedef struct {
    unsigned char flow_latency_bypass;
    unsigned char flatency_enable;
    unsigned short latency;
    unsigned char burst_limit;
    bcm_plp_secy_channel_stat_control_t stat_ctrl;
    unsigned char fpkt_num_thr_mode;
    unsigned short ether_type;
    bcm_plp_secy_rule_sectag_t rule_sectag;
    unsigned char fshould_secure;
    unsigned char fipsec;
    unsigned char fbypass_status;
    unsigned char fbypass_mode;
    unsigned char fpad_nh_check;
    unsigned char fpad_len_check;
    unsigned char fpad_pattern_check;
    unsigned char fpad_dummy_check;
} bcm_plp_secy_channel_t;

/*! Channels params
 *
 * \arg fchannel_config
 *  Set to 1 if channels configuration is required using bcm_plp_secy_channel_t * channel_p
 *
 * \arg channel_p
 *  Channel configuration parameters for channels to re-configure
 */
typedef struct {
    unsigned char fchannel_config;
    bcm_plp_secy_channel_t * channel_p;
} bcm_plp_secy_channel_params_t;

/*! Channel statistics
 * \arg pkts_data_overrun
 *  Data Overrun Packet counters
 *
 * \arg pkts_class_overrun
 *  Class Overrun Packet counters
 */
typedef struct {
    bcm_plp_secy_stat_counter_t pkts_data_overrun;
    bcm_plp_secy_stat_counter_t pkts_class_overrun;
} bcm_plp_secy_channel_stat_t;


/*! Control of automatic EOP insertion.
 *
 * \arg eop_timeout_val \n
 *     Timeout value. \n
 *
 * \arg eop_timeout_ctrl \n
 *     bit mask to specify channels 0 to 31 \n
 *
 * \arg eop_timeout_ctrl_ext \n
 *     bit mask to specify channels 32 to 63 \n
 *
 */
typedef struct
{
    unsigned int eop_timeout_val;
    unsigned int eop_timeout_ctrl;
    unsigned int eop_timeout_ctrl_ext;
} bcm_plp_secy_eop_conf_t;

/*! ECC configuration parameters
 *
 * \arg ecc_correctable_thr \n
 *     Correctable error threshold. \n
 *
 * \arg ecc_uncorrectable_thr \n
 *     Uncorrectable error threshold. \n
 */
typedef struct
{
    unsigned int ecc_correctable_thr;
    unsigned int ecc_uncorrectable_thr;
} bcm_plp_secy_ecc_conf_t;

/*! SA retire configuration parameters
 *
 * \arg timer_prescale \n
 *     Timer prescale value, set to 0 to disable the feature \n
 *
 * \arg sa_timeout \n
 *     SA Timeout value \n
 */
typedef struct
{
    unsigned int timer_prescale;
    unsigned int sa_timeout;
} bcm_plp_secy_sa_retire_conf_t;

/*! Device control
 * \arg stat_control_p
 *  Statistics settings. Set to NULL if no update is required
 *
 * \arg params
 *  Channel configuration parameters
 *
 * \arg eop_conf_p
 *  Control of automatic EOP insertion.
 *
 * \arg ecc_conf_p
 *  ECC configuration parameters
 *
 * \arg sa_retire_conf_p
 *  SA retire settings, Set to NULL if no update is required
 *
 */
typedef struct {
    bcm_plp_secy_statistics_control_t * stat_control_p;
    bcm_plp_secy_channel_params_t params;
    bcm_plp_secy_eop_conf_t * eop_conf_p;
    bcm_plp_secy_ecc_conf_t * ecc_conf_p;
    bcm_plp_secy_sa_retire_conf_t * sa_retire_conf_p;
} bcm_plp_secy_device_params_t;

/** Status returned by each of the functions in this API*/
typedef enum {
    BCM_PLP_SAB_STATUS_OK, /**< Success \n */
    BCM_PLP_SAB_INVALID_PARAMETER, /**< Invalid or NULL pointer\n */
    BCM_PLP_SAB_UNSUPPORTED_FEATURE, /**< Not supported\n */
    BCM_PLP_SAB_ERROR /**< Internal Error \n */
} bcm_plp_sa_builder_status_t;

/** Specify direction: egress (encrypt) or ingress (decrypt) */
typedef enum {
    BCM_PLP_SAB_DIRECTION_EGRESS, /**< Egress Direction\n */
    BCM_PLP_SAB_DIRECTION_INGRESS /**< Ingress Direction\n */
} bcm_plp_sa_builder_direction_t;


/** Operation type */
typedef enum {
    BCM_PLP_SAB_OP_MACSEC,            /**< MACsec operation (default) */
    BCM_PLP_SAB_OP_ENCAUTH_AES_GCM,   /**< Test operation for authenticate-encrypt. */
    BCM_PLP_SAB_OP_ENC_AES_CTR,       /**< Test operation for encryption. */
    BCM_PLP_SAB_OP_IPSEC,             /**< IPsec operation */
} bcm_plp_sa_builder_operation_t;


/** Flags for the flags field. Put a bitwise or (or zero) of any
 *  *  of the flags in this field.*/

/** Use 64-bit sequence number. */
#define BCM_PLP_SAB_MACSEC_FLAG_LONGSEQ 0x1
/** Retain SecTAG (debugging).*/
#define BCM_PLP_SAB_MACSEC_FLAG_RETAIN_SECTAG 0x2
/** Retain ICV (debugging). */
#define BCM_PLP_SAB_MACSEC_FLAG_RETAIN_ICV 0x4
/** Allow sequence number rollover (debugging).*/
#define BCM_PLP_SAB_MACSEC_FLAG_ROLLOVER 0x8
/** Enable SA auto update. */
#define BCM_PLP_SAB_MACSEC_FLAG_UPDATE_ENABLE 0x10
/** Generate IRQ when SA is expired. */
#define BCM_PLP_SAB_MACSEC_FLAG_SA_EXPIRED_IRQ 0x20
/** Update time stamps (only if hardware supports this feature).*/
#define BCM_PLP_SAB_MACSEC_FLAG_UPDATE_TIME   0x40
/** Replay/Late Check Disabling at Post-Decryption State */
#define BCM_PLP_SAB_MACSEC_FLAG_NO_LATE_CHECK 0x80
/** Drop Action for Late packet detection at SOP */
#define BCM_PLP_SAB_MACSEC_FLAG_LATE_HDR_DROP 0x0100
/** Enable/Disable Late Packet detection at SOP */
#define BCM_PLP_SAB_MACSEC_FLAG_LATE_HDR_CHECK 0x0200
/** Update sequence number control, for ingress-only */
#define BCM_PLP_SAB_MACSEC_DISABLE_UPDATE_SEQ_NUM 0x0400
/** IPsec padding control, for ingress-only */
#define BCM_PLP_SAB_IPSEC_FLAG_RETAIN_PADDING 0x0800
/** IPsec padding check , for ingress-only */
#define BCM_PLP_SAB_IPSEC_FLAG_PAD_CHECK 0x1000
/** Enabling IPsec replay check with the mask. (ingress IPsec only) */
#define BCM_PLP_SAB_IPSEC_DISABLE_REPLAY_CHECK 0x2000
/** Enabling IPsec 4-byte SA-Tag */
#define BCM_PLP_SAB_IPSEC_DISABLE_SA_TAG_INSERT 0x4000
/** Enabling ICV check (ingress  only) */
#define BCM_PLP_SAB_MACSEC_DISABLE_ICV_CHECK 0x8000
/** Use SM4 cipher, (default is AES). Not all hardware configurations
    support SM4. */
#define BCM_PLP_SAB_MACSEC_FLAG_SM4 0x10000

/*! Input parameters for the SA Builder
 * \arg flags
 * flags, either 0 or the bitwise or of one or more flag values described above
 *
 * \arg direction
 *  Direction, egress or ingress 
 *
 * \arg operation
 *   Operation type 
 *
 * \arg an
 * AN inserted in SecTAG (egress).
 *
 * \arg key_p
 * MACsec Key.
 *
 * \arg key_byte_count
 * Size of the MACsec key in bytes.
 *
 * \arg h_key_p
 * authentication key, derived from MACsec key.
 *
 * \arg salt_p
 * 12-byte salt (64-bit sequence numbers).
 *
 * \arg ssci_p
 * 4-byte SSCI value (64-bit sequence numbers).
 *
 * \arg sci_p
 * 8-byte SCI.
 *
 * \arg seq_num_lo
 * sequence number.
 *
 * \arg seq_num_hi
 * High part of sequence number (64-bit sequence numbers)
 *
 * \arg window_size
 * Size of the replay window (ingress).
 *
 * \arg icv_byte_count
 * digest length for ENCAUTH operation only.
 *
 * \arg spi
 * 32-bit SPI value
 *
 */
typedef struct {
    unsigned int flags;
    bcm_plp_sa_builder_direction_t direction;
    bcm_plp_sa_builder_operation_t operation;
    unsigned char an;
    unsigned char *key_p;
    unsigned int key_byte_count;
    unsigned char *h_key_p;
    unsigned char *salt_p;
    unsigned char *ssci_p;
    unsigned char *sci_p;
    unsigned int seq_num_lo;
    unsigned int seq_num_hi;
    unsigned int window_size;
    unsigned int icv_byte_count;
    unsigned int spi;
} bcm_plp_sa_builder_params_t;

/*! Debug registers for packet classification. These can be read to
 *  find out how the last packet was classified. For details see the
 *  corresponding register definitions in Operations and Programmer's
 *  Guide.
 *
 * \arg cp_match_debug \n
 *     Control packet matching status \n
 *
 * \arg tcam_debug \n
 *     TCAM matching status \n
 *
 * \arg sec_tag_debug \n
 *     SecTAG parser status \n
 *
 * \arg sampp_debug1 \n
 *     Various VLAN parser results \n
 *
 * \arg sampp_debug2 \n
 *     Various VLAN parser results \n
 *
 * \arg sampp_debug3 \n
 *     Various VLAN parser results \n
 *
 * \arg parsed_da_lo \n
 *     Parsed destination address low \n
 *
 * \arg parsed_da_hi \n
 *     Parsed destination address high \n
 *
 * \arg parsed_sa_lo \n
 *     Parsed source address low \n
 *
 * \arg parsed_sa_hi \n
 *     Parsed source address high + ether type \n
 *
 * \arg parsed_sec_tag_lo \n
 *     Parsed SecTAG low \n
 *
 * \arg parsed_sec_tag_hi \n
 *     Parsed SecTAG high \n
 *
 * \arg debug_flow_lookup \n
 *     TCAM lookup result (vPort policy)
 */
typedef struct {
    unsigned int cp_match_debug;
    unsigned int tcam_debug;
    unsigned int sec_tag_debug;
    unsigned int sampp_debug1;
    unsigned int sampp_debug2;
    unsigned int sampp_debug3;
    unsigned int parsed_da_lo;
    unsigned int parsed_da_hi;
    unsigned int parsed_sa_lo;
    unsigned int parsed_sa_hi;
    unsigned int parsed_sec_tag_lo;
    unsigned int parsed_sec_tag_hi;
    unsigned int debug_flow_lookup;
} bcm_plp_cfye_pkt_process_debug_t;

/*! Number of ECC status counters. */
#define BCM_PLP_CFYE_ECC_NOF_STATUS_COUNTERS 18

/*! CfyE ECC error status.
 * Array of status records per status counter
 *
 * \arg correctable_count \n
 *     Number of correctable errors. \n
 *
 * \arg uncorrectable_count \n
 *     Number of uncorrectable errors. \n
 *
 * \arg fcorrectable_thr \n
 *     Threshold for correctable errors exceeded. \n
 *
 * \arg funcorrectable_thr \n
 *     Threshold for uncorrectable errors exceeded. \n
 */
typedef struct {
    struct {
        unsigned int correctable_count;
        unsigned int uncorrectable_count;
        unsigned char fcorrectable_thr;
        unsigned char funcorrectable_thr;
    } Counters[BCM_PLP_CFYE_ECC_NOF_STATUS_COUNTERS];
} bcm_plp_cfye_ecc_status_t;

/*! Overall CfyE device status.
 *
 * \arg pkg_procses_debug_p \n
 *     May be NULL if parser debug info is not desired. \n
 *
 * \arg ecc_status_p \n
 *     May be NULL if ECC status is not desired. \n
 *
 * \arg fexternal_tcam_p \n
 *     May be NULL if this information is not desired. \n
 */
typedef struct {
    bcm_plp_cfye_pkt_process_debug_t *pkg_procses_debug_p;
    bcm_plp_cfye_ecc_status_t *ecc_status_p;
    unsigned char *fexternal_tcam_p;
} bcm_plp_cfye_device_status_t;

/*! Statistics counter per TCAM entry
 *
 * \arg counter \n
 *     Statistics counter. \n
 */
typedef struct {
    bcm_plp_cfye_stat_counter_t counter;
}bcm_plp_cfye_statistics_tcam_t;

/*! Per-channel statistics counters
 *
 * \arg tcam_hit_multiple \n
 *     Number of packets that hit multiple TCAM entries. \n
 *
 * \arg header_parser_dropped_pkts \n
 *     Number of packets that were dropped by header parser. \n
 *
 * \arg tcam_miss \n
 *     Number of packets that missed TCAM lookup. \n
 *
 * \arg pkts_ctrl \n
 *     Number of control packets. \n
 *
 * \arg pkts_data \n
 *     Number of data packets. \n
 *
 * \arg pkts_dropped \n
 *     Number of packets that were dropped. \n
 *
 * \arg pkts_err_in \n
 *     Number of packets marked as error packets in input. \n
 *
 * \arg pkts_err_in \n
 *     Number of MTT Misses - Ingress Only \n
 *
 */
typedef struct
{
    /** Number of packets that hit multiple TCAM entries */
    bcm_plp_cfye_stat_counter_t tcam_hit_multiple;
    /** Number of packets that were dropped by header parser */
    bcm_plp_cfye_stat_counter_t header_parser_dropped_pkts;
    /** Number of packets that missed TCAM lookup */
    bcm_plp_cfye_stat_counter_t tcam_miss;
    /** Number of control packets */
    bcm_plp_cfye_stat_counter_t pkts_ctrl;
    /** Number of data packets */
    bcm_plp_cfye_stat_counter_t pkts_data;
    /** Number of packets that were dropped*/
    bcm_plp_cfye_stat_counter_t pkts_dropped;
    /** Number of packets marked as error packets in input  */
    bcm_plp_cfye_stat_counter_t pkts_err_in;
    /** Number of MTT Misses - Ingress Only */
    bcm_plp_cfye_stat_counter_t mtt_miss;
} bcm_plp_cfye_statistics_channel_t;

/*! Debug registers for packet classification. These can be read to
 *  find out how the last packet was classified. For details see the
 *  corresponding register definitions in Operations and Programmer's
 *  Guide.
 *
 * \arg parsed_da_lo \n
 *     Parsed destination address low \n
 *
 * \arg parsed_da_hi \n
 *     Parsed destination address high \n
 *
 * \arg parsed_sa_lo \n
 *     Parsed source address low \n
 *
 * \arg parsed_sa_hi \n
 *     Parsed source address high + ether type \n
 *
 * \arg parsed_sectag_lo \n
 *     Parsed source SecTAG low \n
 *
 * \arg parsed_sectag_hi \n
 *     Parsed source SecTAG high \n
 *
 * \arg parsed_sci_lo \n
 *     Parsed source SCI low \n
 *
 * \arg parsed_sci_hi \n
 *     Parsed source SCI high \n
 *
 * \arg sectag_debug \n
 *     various fields related to SecTAG \n
 *
 * \arg rxcam_sci_lo \n
 *     SCI for RxCAM lookup low \n
 *
 * \arg rxcam_sci_hi \n
 *     SCI for RxCAM lookup high \n
 *
 * \arg parser_in_debug \n
 *     various fields related to RxCAM lookup \n
 *
 * \arg sampp_dbg \n
 *     VLAN detector debug registers \n
 */

typedef struct
{
    /** Parsed destination address low*/
    unsigned int parsed_da_lo;
    /** Parsed destination address high*/
    unsigned int parsed_da_hi;
    /** Parsed source address low*/
    unsigned int parsed_sa_lo;
    /** Parsed source address high + ether type*/
    unsigned int parsed_sa_hi;
    /** Parsed source SecTAG low*/
    unsigned int parsed_sectag_lo;
    /** Parsed source SecTAG high*/
    unsigned int parsed_sectag_hi;
    /** Parsed source SCI low*/
    unsigned int parsed_sci_lo;
    /** Parsed source SCI high*/
    unsigned int parsed_sci_hi;
    /** various fields related to SecTAG */
    unsigned int sectag_debug;
    /** SCI for RxCAM lookup low*/
    unsigned int rxcam_sci_lo;
    /** SCI for RxCAM lookup high*/
    unsigned int rxcam_sci_hi;
    /** various fields related to RxCAM lookup*/
    unsigned int parser_in_debug;
    /** VLAN detector debug registers */
    unsigned int sampp_dbg;
} bcm_plp_secy_pkt_process_debug_t;


/** Number of ECC status counters. */
#define BCM_PLP_SECY_ECC_NOF_STATUS_COUNTERS 19

/*! SecY ECC error status.
 * Array of status records per status counter
 *
 * \arg correctable_count \n
 *     Number of correctable errors. \n
 *
 * \arg uncorrectable_count \n
 *     Number of uncorrectable errors. \n
 *
 * \arg fcorrectable_thr \n
 *     Threshold for correctable errors exceeded. \n
 *
 * \arg funcorrectable_thr \n
 *     Threshold for uncorrectable errors exceeded. \n
 */
typedef struct
{
    /** Array of status records per status counter */
    struct  {
        /** Number of correctable errors. */
        unsigned int correctable_count;

        /** Number of uncorrectable errors.*/
        unsigned int uncorrectable_count;

        /** Threshold for correctable errors exceeded */
        unsigned char fcorrectable_thr;

        /** Threshold for uncorrectable errors exceeded */
        unsigned char funcorrectable_thr;
    } Counters[BCM_PLP_SECY_ECC_NOF_STATUS_COUNTERS];
} bcm_plp_secy_ecc_status_t;

/*! Overall SecY device status.
 *
 * \arg pkg_procses_debug_p \n
 *     May be NULL if parser debug info is not desired. \n
 *
 * \arg ecc_status_p \n
 *     May be NULL if ECC status is not desired. \n
 */
typedef struct
{
    /** may be NULL if parser debug info is not desired. */
    bcm_plp_secy_pkt_process_debug_t *pkt_process_debug_p;

    /** may be NULL if ECC status is not desired. */
    bcm_plp_secy_ecc_status_t *ecc_status_p;
} bcm_plp_secy_device_status_t;

/*! Status values returned by bcm_plp_macsec_warmboot API functions */
typedef enum {
    BCM_PLP_WARMBOOT_STATUS_OK,
    BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER,
    BCM_PLP_WARMBOOT_INTERNAL_ERROR,
    BCM_PLP_WARMBOOT_ERROR_ALLOCATION
} bcm_plp_warmboot_status_t;


/*! Alloc Callback API for MACSEC Warmboot
 *
 * This callback function allocates a storage area of the desired size in
 * a memory area that will be preserved across CPU reboots. The allocated
 * area is filled entirely with bytes of value zero. The returned area_id
 * represents a valid storage area. Each allocated (and not yet freed) storage
 * area has a unique area_id.
 *
 * @param pa (input)
 *     bcm_plp_sec_phy_access_t structure pointer of the PHY device
 * @param storage_byte_count (input)
 *     Size of the storage area in bytes.
 * @param area_id_p (output)
 *     ID of the allocated storage area.
 *
 * @return WARMBOOT_STATUS_OK : success
 * @return WARMBOOT_ERROR_BAD_PARAMETER : incorrect input parameter. area_id_p is
 *         NULL or storage_byte_count exceeds area size limit imposed by system.
 * @return WARMBOOT_ERROR_ALLOCATION : no space available for the requested
 *         storage area.
 * @return WARMBOOT_INTERNAL_ERROR : other failure
 */
typedef bcm_plp_warmboot_status_t (* bcm_plp_warmboot_alloc_callback_t)
                                     (const bcm_plp_sec_phy_access_t *pa, 
                                      const unsigned int storage_byte_count,
                                      unsigned int *const area_id_p);

/*! Read Callback API for MACSEC Warmboot
 *
 * This callback function reads data from the storage area described by
 * the area_id. It reads the data starting at byte_offset and with size
 * byte_count
 *
 * @param pa (input)
 *     bcm_plp_sec_phy_access_t structure pointer of the PHY device
 * @param area_id (input)
 *     ID of the storage area to read from.
 * @param data_p (output)
 *     Pointer an area where the read data will be stored.
 * @param byte_offset (input)
 *     Offset from the start of the storage area where the first data
 *     byte will be read (byte_offset == 0 corresponds to start of area).
 * @param byte_count (input)
 *     Number of bytes to read.
 *
 * Return value:
 * @return WARMBOOT_STATUS_OK : success
 * @return WARMBOOT_ERROR_BAD_PARAMETER : incorrect input parameter. area_id does
 *         not represent a valid storage area, data_p is NULL,
 *         byte_offset + byte_count greater than area size.
 * @return WARMBOOT_INTERNAL_ERROR : other failure
 */
typedef bcm_plp_warmboot_status_t (* bcm_plp_warmboot_read_callback_t)
                                     (const bcm_plp_sec_phy_access_t *pa, 
                                      const unsigned int area_id,
                                      unsigned char *const data_p,
                                      const unsigned int byte_offset,
                                      const unsigned int byte_count);

/*! Write Callback API for MACSEC Warmboot
 *
 * This callback function writes data to the storage area described by
 * the area_id. It overwrites the data starting at byte_offset and with size
 * byte_count. Other bytes stored in this area are unchanged.
 *
 * @param pa (input)
 *     bcm_plp_sec_phy_access_t structure pointer of the PHY device
 * @param area_id (input)
 *     ID of the storage area to write to.
 * @param data_p (input)
 *     Pointer to the data to be written.
 * @param byte_offset (input)
 *     Offset from the start of the storage area where the first data
 *     byte will be written (byte_offset == 0 corresponds to start of area).
 * @param byte_count (input)
 *     Number of bytes to write.
 *
 * Note: data may be partially written in case of an unexpected reboot.
 *       The last byte will be written last, so it can be used as a flag to
 *       indicate validity.
 *
 * Return value:
 * @return BCM_PLP_WARMBOOT_STATUS_OK : success
 * @return BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER : incorrect input parameter. area_id does
 *         not represent a valid storage area, data_p is NULL,
 *         byte_offset + byte_count greater than area size.
 * @return BCM_PLP_WARMBOOT_INTERNAL_ERROR : other failure
 */
typedef bcm_plp_warmboot_status_t (* bcm_plp_warmboot_write_callback_t)
                                     (const bcm_plp_sec_phy_access_t *pa, 
                                      const unsigned int area_id,
                                      const unsigned char *const data_p,
                                      const unsigned int byte_offset,
                                      const unsigned int byte_count);

/*! Free Callback API for MACSEC Warmboot
 *
 * This callback function frees a storage area previously allocated by
 * the WarnBoot_Alloc callback.  After this call, area_id no longer corresponds
 * to a valid storage area and it may no longer be passed to callbacks of this
 * API. Future calls to the WarmBoot_Alloc callback may re-use this area_id.
 *
 * @param pa (input)
 *     bcm_plp_sec_phy_access_t structure pointer of the PHY device
 * @param area_id (input)
 *     ID of the storage area to be freed.
 *
 * @return BCM_PLP_WARMBOOT_STATUS_OK : success
 * @return BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER : incorrect input parameter. area_id does
 *         not represent a valid storage area.
 * @return BCM_PLP_WARMBOOT_INTERNAL_ERROR : other failure
 */
typedef bcm_plp_warmboot_status_t (* bcm_plp_warmboot_free_callback_t) (const bcm_plp_sec_phy_access_t *pa, 
                                                                        const unsigned int area_id);

/*! Macsec Warmboot Callbacks.
 *
 * \arg alloc_cb \n
 *     Persistent Memory Allocation Callback. \n
 *
 * \arg free_cb \n
 *     Persistent Memory Free Callback. \n
 *
 * \arg write_cb \n
 *     Callback used to save macsec config in Persistent Memory. \n
 *
 * \arg read_cb \n
 *     Callback used to restore macsec config from Persistent Memory. \n
 */
typedef struct
{
    bcm_plp_warmboot_alloc_callback_t alloc_cb;
    bcm_plp_warmboot_free_callback_t free_cb;
    bcm_plp_warmboot_write_callback_t write_cb;
    bcm_plp_warmboot_read_callback_t read_cb;
} bcm_plp_macsec_warmboot_callbacks_t;

/*! Macsec Warmboot Device Types */
typedef enum
{
    BCM_PLP_WARMBOOT_DEVICE_TYPE_SECY_E,
    BCM_PLP_WARMBOOT_DEVICE_TYPE_SECY_I,
    BCM_PLP_WARMBOOT_DEVICE_TYPE_CFYE_E,
    BCM_PLP_WARMBOOT_DEVICE_TYPE_CFYE_I,
    BCM_PLP_WARMBOOT_DEVICE_TYPE_DF,
} bcm_plp_macsec_warmboot_device_type_t;

/** Classification device Operation mode */
typedef enum
{
    BCM_PLP_CYFE_MODE_MACSEC,  /**< MACsec mode  */
    BCM_PLP_CYFE_MODE_IPSEC,   /**< IPsec mode   */
    BCM_PLP_CYFE_MODE_INVALID  /**< Invalid mode */
} bcm_plp_cfye_channel_mode_t;

/*! Classification device limits
 *
 * \arg eip_number \n
 *     EIP Number (can be 363 or 163)
 * \arg major_version \n
 *     Major HW version \n
 *
 * \arg minor_version \n
 *     Minor HW version \n
 *
 * \arg patch_level \n
 *     HW patch level \n
 *
 * \arg channel_count \n
 *     Number of channels \n
 *
 * \arg vport_count \n
 *     Number of vPorts \n
 *
 * \arg rule_count \n
 *     Number of rules \n
 *
 * \arg mtt_count \n
 *     Number of MTT entries \n
 *
 * \arg fipsec \n
 *     Does the device have IPsec support \n
 *
 * \arg fvxlan \n
 *     Does the device have VxLAN support \n
 */
typedef struct
{
    unsigned short eip_number;
    unsigned char major_version;
    unsigned char minor_version;
    unsigned char patch_level;
    unsigned int channel_count;
    unsigned int vport_count;
    unsigned int rule_count;
    unsigned int mtt_count;
    unsigned char fipsec;
    unsigned char fvxlan;
} bcm_plp_cfye_device_limits_t;

/*! Statistics counter per MTT entry
 *
 * \arg counter \n
 *     MTT entry statistics counter \n
 */
typedef struct
{
    bcm_plp_cfye_stat_counter_t counter;
} bcm_plp_cfye_mtt_stat_t;


/*! MTT matching rule key/mask data structure
 *
 * \arg fipv6 \n
 *     false : IPv4 \n
 *     true  : IPv6 \n
 *
 * \arg tag_label1 \n
 *     VLAN tag label \n
 *
 * \arg tag_label2 \n
 *     VLAN tag label2 \n
 *
 * \arg fip_hdr_valid \n
 *     true - speculatively parsed IP header \n
 *
 * \arg fpacket_type \n
 *     false : Untagged/VLAN packet \n
 *     true  : MPLS packet \n
 */
typedef struct
{
    unsigned char fipv6;
    unsigned int tag_label1;
    unsigned int tag_label2;
    unsigned char fip_hdr_valid;
    unsigned char fpacket_type;
} bcm_plp_cfye_mtt_keymask_t;

/*! MTT matching rule data structure
 *
 * \arg key \n
 *     Sets matching values as specified in bcm_plp_cfye_mtt_keymask_t \n
 *
 * \arg mask \n
 *     Mask for matching values, can be used to mask out irrelevant Key bits \n
 *
 * \arg ip_addr \n
 *     IP address (source for egress, destination for ingress) to match. \n
 *
 * \arg ip_addr_mask \n
 *     Mask for data values, can be used to mask out irrelevant Data bits \n
 */
typedef struct
{
    bcm_plp_cfye_mtt_keymask_t key;
    bcm_plp_cfye_mtt_keymask_t mask;
    unsigned int ip_addr[4];
    unsigned int ip_addr_mask[4];
} bcm_plp_cfye_mtt_t;

#endif /* EPDM_SEC_COMMON_H */
