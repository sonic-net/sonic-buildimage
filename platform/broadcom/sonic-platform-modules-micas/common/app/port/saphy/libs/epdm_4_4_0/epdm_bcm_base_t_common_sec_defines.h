/*
 * $Copyright: (c) 2020 Broadcom.
 * Broadcom Proprietary and Confidential. All rights reserved.$
 $Id$
 */

#ifndef BCM_BASE_T_COMMON_SEC_DEFINES_H
#define BCM_BASE_T_COMMON_SEC_DEFINES_H
#define BCM_PLP_BASE_T_PM_IF_SUCCESS                0
#define BCM_PLP_BASE_T_PM_NOT_FOUND                -20
#define BCM_PLP_BASE_T_PM_IF_MEMORY                -21
#define SOC_CORE_NOT_FOUND                         -22
#define BCM_PLP_BASE_T_PM_IF_INTERNAL              -23
#define BCM_PLP_BASE_T_PM_IF_PHY_EXISTING          -24
#define BCM_PLP_BASE_T_PM_IF_PHY_NA                -25
#define BCM_PLP_BASE_T_PM_IF_INVALID_PHY           -26
#define BCM_PLP_BASE_T_PM_IF_UNAVAIL               -27
#define BCM_PLP_BASE_T_PM_IF_PARAM                 -28

#define BCM_PLP_BASE_T_PM_IF_CHIP_VER_NO            2
#define BCM_PLP_BASE_T_PM_IF_API_VER_NO             2
#define BCM_PLP_BASE_T_PM_IF_ENAHAN_VER_NO          6
#define BCM_PLP_BASE_T_PM_INTERFACE_SIDE_SHIFT      31
#define BCM_PLP_BASE_T_PM_STATIC_CONFIG_MAX_NUM_BYTES 16

#define BCM_PLP_BASE_T_SEC_MAX_PHY          1024
#ifndef PHYMOD_IF_CONFIG_MAX_PHYS
#define PHYMOD_IF_CONFIG_MAX_PHYS           BCM_PLP_BASE_T_SEC_MAX_PHY
#endif
/*
#define BCM_PLP_BASE_T_PM_IF_MAX_PHY        BCM_PLP_BASE_T_SEC_MAX_PHY
*/

/*! PHY INFO
 *
 * \arg void *platform_ctxt \n
 *             Represents user data, platform_ctxt is passed to\n
 *             register read/write API. It can be NULL if not used in read/write register
 *
 * \arg unsigned int phy_addr \n
 *             Represents Phy-id\n
 *
 * \arg unsigned int if_side \n
 *             Represents the interface side \n
 *                    0 - Line side of the PHY device\n
 *                    1 - system side of the PHY device\n
 *
 * \arg unsigned int lane_map \n
 *             Represents the Lane mapping of a port,\n
 *             LSB Bit 0 represents lane 0 of the specified PHY-ID.\n
 *             LSB Bit 1 represents lane 1 of the specified PHY-ID\n
 *             Similarly for lane 2 to lane N.\n
 *             where N is maximum number of lanes on a PHY.
 *             It also support multicast.\n
 *             Eg:
 *                   0x3 represent lane 0 and 1 \n
 *                   0xF represent lane 0 to lane 3
 */
typedef struct bcm_plp_base_t_access_s {
    void *platform_ctxt;
    unsigned int phy_addr;
    unsigned int if_side;
    unsigned int lane_map;
}bcm_plp_base_t_access_t;


/* bcm_plp_base_t_sec_mutex_take_f */
typedef int (*bcm_plp_base_t_sec_mutex_take_f)(
    int unit,
    void *user_data);

/* bcm_plp_base_t_sec_mutex_give_f */
typedef int (*bcm_plp_base_t_sec_mutex_give_f)(
    int unit,
    void *user_data);


/*! bcm_plp_base_t_sec_mutex_t
 * bcm_plp_base_t_sec mutex struct
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
 *
 *
 */

typedef struct bcm_plp_base_t_sec_mutex_s {
    void *user_data;
    bcm_plp_base_t_sec_mutex_take_f SecY0_mutex_take; /**< egress SecY mutex */
    bcm_plp_base_t_sec_mutex_take_f SecY1_mutex_take; /**< igress SecY mutex */
    bcm_plp_base_t_sec_mutex_give_f SecY0_mutex_give; /**< egress SecY mutex */
    bcm_plp_base_t_sec_mutex_give_f SecY1_mutex_give; /**< igress SecY mutex */
} bcm_plp_base_t_sec_mutex_t;

/* Forward declaration of UNIMAC context structure for future use */
#ifdef INCLUDE_PLP_UNIMAC
struct phy_mac_ctrl_s;
#endif
/*! bcm_plp_base_t_sec_access_t
 * bcm_plp_base_t_sec access struct
 *
 * \arg phy_info
 * bcm_plp_base_t_access_t structure
 *
 * \arg bcm_plp_base_t_sec_mutex_t mutex;
 * mutex lock for SecY
 *
 *  \arg macsec_side (input)
 *   Device identifier of the classification device to be used.\n
 *    macsec_side can be "0" or "1" \n
 *    macsec_side = 1 for Ingress only, \n
 *    macsec_side = 0 for Egress only
 */

typedef struct bcm_plp_base_t_sec_phy_access_s {
    bcm_plp_base_t_access_t phy_info;
    bcm_plp_base_t_sec_mutex_t mutex;  /**< mutex lock for SecY APIs */
    unsigned int macsec_side;
    unsigned int macsec_dev_addr;
    #ifdef INCLUDE_PLP_UNIMAC
    /* For future use */
    struct phy_mac_ctrl_s *unimac;
    #endif
} bcm_plp_base_t_sec_phy_access_t;
typedef bcm_plp_base_t_sec_phy_access_t bcm_plp_base_t_sec_access_t;

typedef struct bcm_plp_if_phymod_phy_id_s {
    void *user_acc;
    unsigned int phy_id;
    unsigned char valid;
    int      unit;
#ifdef SA_PLP_SUPPORT
    int (*read)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int* val);
    int (*write)(void* user_acc, unsigned int core_addr, unsigned int reg_addr, unsigned int val);
#else
    int      (*read)(int, unsigned int, unsigned int, unsigned short*);
    int      (*write)(int, unsigned int, unsigned int, unsigned short);
#endif
} bcm_plp_if_phymod_phy_id_t;

/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

/* Driver Framework Basic Definitions API */


/*----------------------------------------------------------------------------
 * Definitions and macros
 */

/** Number of used non-control key/mask words */
#define BCM_PLP_BASE_T_SECY_RULE_NON_CTRL_WORD_COUNT           5

/** Number of ethertype values to use for comparison for MPLS packets */
#define BCM_PLP_BASE_T_SECY_MPLS_ETYPE_MAX_COUNT               4

/** Maximum number of VLAN user priority values */
#define BCM_PLP_BASE_T_SECY_VLAN_UP_MAX_COUNT                  8

/** Maximum number of control packet matching rules
    using MAC destination address and ether_type */
#define BCM_PLP_BASE_T_SECY_MAC_DA_ET_MATCH_RULES_COUNT        8

/** Maximum number of control packet matching rules\n
    using ether_type and range of MAC destination addresses */
#define BCM_PLP_BASE_T_SECY_MAC_DA_ET_RANGE_MATCH_RULES_COUNT  2

/** Counter increment disable control values\n
    Each bit disables all the counters of one counter type.
    Counter types:
        - SA counters
        - Interface counters
        - SecY counters
        - RxCAM counters
        - TCAM counters
        - Global counters */
/** SA Counter increment disable control bit */
#define BCM_PLP_BASE_T_SECY_SA_COUNT_INC_DIS       (1<<0)
/** IFC Counter increment disable control bit */
#define BCM_PLP_BASE_T_SECY_IFC_COUNT_INC_DIS      (1<<1)
/** SECY Counter increment disable control bit */
#define BCM_PLP_BASE_T_SECY_SECY_COUNT_INC_DIS     (1<<2)
/** SECY Counter increment disable control bit */
#define BCM_PLP_BASE_T_SECY_RXCAM_COUNT_INC_DIS    (1<<3)
/** RXCAM Counter increment disable control bit */
#define BCM_PLP_BASE_T_SECY_TCAM_COUNT_INC_DIS     (1<<4)
/** TCAM Counter increment disable control bit */
#define BCM_PLP_BASE_T_SECY_GLOBAL_COUNT_INC_DIS   (1<<5)

/*----------------------------------------------------------------------------
 * Definitions and macros
 */

/*----------------------------------------------------------------------------
 * Adapter EIP-160 AIC events resulting in interrupts
 */
/** Packet drop event from classification logic */
#define BCM_PLP_BASE_T_SECY_EVENT_DROP_CLASS           (1<<0)
/** Packet drop event from post-processor logic */
#define BCM_PLP_BASE_T_SECY_EVENT_DROP_PP              (1<<1)
/** Packet drop event from MTU checking logic (egress only) */
#define BCM_PLP_BASE_T_SECY_EVENT_DROP_MTU             (1<<2)
/** Control packet event */
#define BCM_PLP_BASE_T_SECY_EVENT_CTRL_PKT             (1<<3)
/** Data packet event */
#define BCM_PLP_BASE_T_SECY_EVENT_DATA_PKT             (1<<4)
/** Interrupt event from MACsec crypto-engine (PE) core. */
#define BCM_PLP_BASE_T_SECY_EVENT_ENG_IRQ              (1<<5)
/** TCAM rule miss event from the TCAM module */
#define BCM_PLP_BASE_T_SECY_EVENT_TCAM_MISS            (1<<6)
/** Multiple TCAM rule hit event from the TCAM module */
#define BCM_PLP_BASE_T_SECY_EVENT_TCAM_MULT_HIT        (1<<7)

/** Statistics threshold event from SA Statistics Module */
#define BCM_PLP_BASE_T_SECY_EVENT_STAT_SA_THR          (1<<8)
/** Statistics threshold event from SecY Statistics Module */
#define BCM_PLP_BASE_T_SECY_EVENT_STAT_SECY_THR        (1<<9)
/** Statistics threshold event from IFC Statistics Module */
#define BCM_PLP_BASE_T_SECY_EVENT_STAT_IFC_THR         (1<<10)
/** Statistics threshold event from Global Statistics Module */
#define BCM_PLP_BASE_T_SECY_EVENT_STAT_GLOBAL_THR      (1<<11)
/** Statistics threshold event from TC Statistics Module */
#define BCM_PLP_BASE_T_SECY_EVENT_STAT_TC_THR          (1<<12)
/** Statistics threshold event from RC Statistics Module */
#define BCM_PLP_BASE_T_SECY_EVENT_STAT_RC_THR          (1<<13)
/** SC CAM miss event from the SC CAM module */
#define BCM_PLP_BASE_T_SECY_EVENT_SC_MISS              (1<<14)
/** Multiple SC CAM hit event from the SC CAM module */
#define BCM_PLP_BASE_T_SECY_EVENT_SC_MULT_HIT          (1<<15)

/** Packet number threshold event from Post Processor */
#define BCM_PLP_BASE_T_SECY_EVENT_SA_PN_THR            (1<<16)
/** SA expired event from Classifier */
#define BCM_PLP_BASE_T_SECY_EVENT_SA_EXPIRED           (1<<17)

/*! secy device interrupt
 *
 *  \arg event_mask
 *      The requested events, bitwise OR of BCM_PLP_BASE_T_SECY_EVENT_* events
 *
 */
typedef struct {
    unsigned int event_mask;
 } bcm_plp_base_t_secy_intr_t;

/** Status values returned by API functions */
typedef enum
{
    BCM_PLP_BASE_T_SECY_STATUS_OK,               /**< \n */
    BCM_PLP_BASE_T_SECY_ERROR_BAD_PARAMETER,     /**< \n */
    BCM_PLP_BASE_T_SECY_ERROR_INTERNAL,          /**< \n */
    BCM_PLP_BASE_T_SECY_ERROR_NOT_IMPLEMENTED,   /**< \n */
    /** Applicable for drivers compiled with MACSEC_BASET_V2_2 */
    BCM_PLP_BASE_T_SECY_ERROR_DEVICE_RW = 0x7fffffff /**< \n */
} bcm_plp_base_t_secy_status_t;

/** bcm_plp_base_t_secy device role: either egress only or ingress only */
typedef enum
{
    BCM_PLP_BASE_T_SECY_ROLE_EGRESS,             /**< \n */
    BCM_PLP_BASE_T_SECY_ROLE_INGRESS             /**< \n */
} bcm_plp_base_t_secy_role_t;

/** Statistics counter */
typedef struct
{
    /** Low 32-bit counter word */
    unsigned int lo;
    /** High 32-bit counter word */
    unsigned int hi;
} bcm_plp_base_t_secy_stat_counter_t;

/**----------------------------------------------------------------------------
 * bcm_plp_base_t_secy_sa_handle_t
 *
 * This handle is a reference to an SA. It is returned when an SA
 * is added and it remains valid until the SA is removed.
 *
 * The handle is set to NULL when bcm_plp_base_t_secy_sa_handle_t handle.p is equal to NULL.
 */
typedef struct
{
    /** Reference handle to an SA */
    void * p;
} bcm_plp_base_t_secy_sa_handle_t;

/**----------------------------------------------------------------------------
 * bcm_plp_base_t_secy_rule_handle_t
 *
 * This handle is a reference to a rule. It is returned when a rule
 * is added and it remains valid until the rule is removed.
 *
 * The handle is set to NULL when bcm_plp_base_t_secy_rule_handle_t handle.p is equal to NULL
 */
typedef struct
{
    /** Reference handle to an rule */
    void * p;
} bcm_plp_base_t_secy_rule_handle_t;

/**----------------------------------------------------------------------------
 * bcm_plp_base_t_secy_vport_handle_t
 *
 * This handle is a reference to a vPort. It is returned when a vPort
 * is added and it remains valid until the vPort is removed.
 *
 * The handle is set to NULL when bcm_plp_base_t_secy_vport_handle_t handle.p is equal
 * to NULL.
 */
typedef struct
{
    /** Reference handle to an vPort */
    void * p;
} bcm_plp_base_t_secy_vport_handle_t;

/**----------------------------------------------------------------------------
 * bcm_plp_base_t_secy_port_type_t
 *
 * Types of ports:\n
 *      Tx MAC\n
 *      Redirect FIFO\n
 *      Capture debug FIFO\n
 *      TX MAC and Capture debug FIFO
 */
typedef enum
{
    BCM_PLP_BASE_T_SECY_PORT_TXMAC,              /**< \n */
    BCM_PLP_BASE_T_SECY_PORT_REDIRECTFIFO,       /**< \n */
    BCM_PLP_BASE_T_SECY_PORT_CAPTUREFIFO,        /**< \n */
    BCM_PLP_BASE_T_SECY_PORT_TXMACCAPTURE        /**< \n */
} bcm_plp_base_t_secy_port_type_t;

/**----------------------------------------------------------------------------
 * bcm_plp_base_t_secy_sa_sction_type_t
 *
 * SA action type:\n
 *      bypass,\n
 *      drop,\n
 *      MACsec ingress (do not use for an egress only device),\n
 *      MACsec egress (do not use for an ingress only device),\n
 *      Crypt-Authenticate.
 */
typedef enum
{
    BCM_PLP_BASE_T_SECY_SA_ACTION_BYPASS,                /**< \n */
    BCM_PLP_BASE_T_SECY_SA_ACTION_DROP,                  /**< \n */
    BCM_PLP_BASE_T_SECY_SA_ACTION_INGRESS,               /**< \n */
    BCM_PLP_BASE_T_SECY_SA_ACTION_EGRESS,                /**< \n */
    BCM_PLP_BASE_T_SECY_SA_ACTION_CRYPT_AUTH             /**< \n */
} bcm_plp_base_t_secy_sa_sction_type_t;

/**----------------------------------------------------------------------------
 * bcm_plp_base_t_secy_drop_type_t
 *
 * SA drop type:\n
 *   0 = bypass with CRC corruption signaling,\n
 *   1 = bypass with bad packet indicator,\n
 *   2 = internal drop by crypto-core (packet is not seen outside),\n
 *   3 = do not drop (for debugging only).
 */
typedef enum
{
    BCM_PLP_BASE_T_SECY_SA_DROP_CRC_ERROR,               /**< \n */
    BCM_PLP_BASE_T_SECY_SA_DROP_PKT_ERROR,               /**< \n */
    BCM_PLP_BASE_T_SECY_SA_DROP_INTERNAL,                /**< \n */
    BCM_PLP_BASE_T_SECY_SA_DROP_NONE                     /**< \n */
} bcm_plp_base_t_secy_drop_type_t;

/**----------------------------------------------------------------------------
 * bcm_plp_base_t_secy_validate_frames_t
 *
 * Ingress tagged frame validation options
 */
typedef enum
{
    BCM_PLP_BASE_T_SECY_FRAME_VALIDATE_DISABLE,          /**< \n */
    BCM_PLP_BASE_T_SECY_FRAME_VALIDATE_CHECK,            /**< \n */
    BCM_PLP_BASE_T_SECY_FRAME_VALIDATE_STRICT,           /**< \n */
    /** Applicable for drivers compiled with MACSEC_BASET_V2_2 */
    BCM_PLP_BASE_T_SECY_FRAME_VALIDATE_NULL              /**< \n */
} bcm_plp_base_t_secy_validate_frames_t;

/** SA parameters for Egress action type */
typedef struct
{
    /** true - SA is in use, packets classified for it can be transformed\n
        false - SA not in use, packets classified for it can not be
                transformed */
    unsigned char fsa_inuse;

    /** The number of bytes (in the range of 0-127) that are authenticated\n
        but not encrypted following the SecTAG in the encrypted packet.\n
        Values 65-127 are reserved in HW < 4.0 and should not be used there.*/
    unsigned char confidentiality_offset;

    /** true - enable frame protection,\n
        false - bypass frame through device */
    unsigned char fprotect_frames;

    /** true - inserts explicit SCI in the packet,\n
        false - use implicit SCI (not transferred) */
    unsigned char finclude_sci;

    /** true - enable ES bit in the generated SecTAG\n
        false - disable ES bit in the generated SecTAG */
    unsigned char fuse_es;

    /** true - enable SCB bit in the generated SecTAG\n
        false - disable SCB bit in the generated SecTAG */
    unsigned char fuse_scb;

    /** true - enable confidentiality protection\n
        false - disable confidentiality protection */
    unsigned char fconf_protect;

    /** true - allow data (non-control) packets.\n
        false - drop data packets.*/
    unsigned char fallow_data_pkts;

     /** true - EoMPLS packet contains a 4-byte control word.\n
        false - EoMPLS packet does not contain a 4-byte control word. */
    unsigned char feompls_ctrl_word;

    /** true - (Eo)MPLS packets are assumed to be EoMPLS packets.\n
        false - (Eo)MPLS packets are assumed to be MPLS packets. */
    unsigned char feompls_subport;

    /** true - enable input sectag processing.\n
        false - disable input sectag processing. */
    unsigned char feg_sectag_enb;

    /** true - take e & c bits from input sectag.\n
        false - e & c bits are calculated. */
    unsigned char fec_from_sectag;

    /** true - take ES & SCB bits from input sectag.\n
        false - ES & SCB bits are calculated.*/
    unsigned char fes_scb_from_sectag;

    /** true - take SC & SCI bits from input sectag.\n
        false - SC & SCI bits are calculated. */
    unsigned char fsc_sci_from_sectag;

    /** true - Use value of SL field to strip Ethernet padding.\n
        false - Do not use value of SL field to strip Ethernet padding. */
    unsigned char fsl_pad_strip_enb;

    /** true - take e & c bits from ST-VLAN tag.\n
        false - e & c bits are calculated. */
    unsigned char fec_from_st_vlan;

    /** Specifies number of bytes from the start of the frame
        to be bypassed without MACsec protection */
    unsigned char pre_sectag_auth_start;

    /** Specifies number of bytes to be authenticated in the pre-SecTAG area.*/
    unsigned char pre_sectag_auth_length;

    /** Offset of the SecTag */
    unsigned char sectag_offset;
} bcm_plp_base_t_secy_sa_e_t;

/** SA parameters for Ingress action type */
typedef struct
{
    /** true - SA is in use, packets classified for it can be transformed\n
        false - SA not in use, packets classified for it can not be
                transformed */
    unsigned char fsa_inuse;

    /** The number of bytes (in the range of 0-127) that are authenticated\n
        but not encrypted following the SecTAG in the encrypted packet.\n
        Values 65-127 are reserved in hardware < 4.0 and should not be
        used there. */
    unsigned char confidentiality_offset;

    /** true - enable replay protection\n
        false - disable replay protection */
    unsigned char freplay_protect;

    /** MACsec frame validation level (tagged). */
    bcm_plp_base_t_secy_validate_frames_t validate_frames_tagged;

    /** SCI to which ingress SA applies (8 bytes). */
    unsigned char *sci_p;

    /** Association number to which ingress SA applies. */
    unsigned char an;

    /** true - allow tagged packets.\n
        false - drop tagged packets.*/
    unsigned char fallow_tagged;

    /** true - allow untagged packets.\n
        false - drop untagged packets. */
    unsigned char fallow_untagged;

    /** true - enable validate untagged packets.\n
        false - disable validate untagged packets.*/
    unsigned char fvalidate_untagged;

    /** true is EOMPLS */
    unsigned char feompls;

    /** true is Mac in Mac */
    unsigned char fmac_icmac;

    /** Specifies number of bytes from the start of the frame
        to be bypassed without MACsec protection */
    unsigned char pre_sectag_auth_start;

    /** Specifies number of bytes to be authenticated in the pre-SecTAG area.*/
    unsigned char pre_sectag_auth_length;

    /** Offset of the SecTag */
    unsigned char sectag_offset;

    /** true - EoMPLS packet contains a 4-byte control word.\n
        false - EoMPLS packet does not contain a 4-byte control word.*/
    unsigned char feompls_ctrl_word;

    /** true - (Eo)MPLS packets are assumed to be EoMPLS packets.\n
        false - (Eo)MPLS packets are assumed to be MPLS packets. */
    unsigned char feompls_subport;

    /** For situations when RxSC is not found or SAinUse=0 with validation
        level that allows packet to be sent to the Controlled port with the
        SecTAG/ICV removed, this flag represents a policy to allow SecTAG
        retaining.\n
        true - SecTAG is retained. */
    unsigned char fretain_sectag;

    /** true - ICV is retained (allowed only when fRetainSecTAG is true). */
    unsigned char fretain_icv;

    /**
     * true - Enable 802.1AE compliant processing for correctly tagged MACsec
     * frames for wich no valid MACsec secure channel is found
     * Applicable for drivers compiled with MACSEC_BASET_V2_2
     */
    unsigned char fnm_macsec_en;

} bcm_plp_base_t_secy_sa_i_t;

/** SA parameters for Bypass/Drop action type */
typedef struct
{
    /** true - enable statistics counting for the associated SA\n
       false - disable statistics counting for the associated SA */
    unsigned char fsa_inuse;

} bcm_plp_base_t_secy_sa_bd_t;

/** SA parameters for Crypt-Authenticate action type */
typedef struct
{
    /** true - message has length 0\n
        false - message has length > 0 */
    unsigned char fzero_length_message;

    /** The number of bytes (in the range of 0-255) that are authenticated but
        not encrypted (AAD length). */
    unsigned char confidentiality_offset;

    /** IV loading mode:\n
        0: The IV is fully loaded via the transform record.\n
        1: The full IV is loaded via the input frame. This IV is located in
           front of the frame and is considered to be part of the bypass data,
           however it is not part to the result frame.\n
        2: The full IV is loaded via the input frame. This IV is located at the
           end of the bypass data and is considered to be part of the bypass
           data, and it also part to the result frame.\n
        3: The first three IV words are loaded via the input frame, the counter
           value of the IV is set to one. The three IV words are located in
           front of the frame and are considered to be part of the bypass data,
           however it is not part to the result frame. */
    unsigned char iv_mode;

    /** true - append the calculated ICV\n
        false - don't append the calculated ICV */
    unsigned char ficv_append;

    /** true - enable ICV verification\n
        false - disable ICV verification */
    unsigned char ficv_verify;

    /** true - enable confidentiality protection (AES-GCM/CTR operation)\n
        false - disable confidentiality protection (AES-GMAC operation) */
    unsigned char fconf_protect;

} bcm_plp_base_t_secy_sa_ca_t;

/** SA parameters */
    typedef union bcm_plp_base_t_secy_sa_action_u
    {
        bcm_plp_base_t_secy_sa_e_t egress;
        bcm_plp_base_t_secy_sa_i_t ingress;
        bcm_plp_base_t_secy_sa_bd_t bypass_drop;
        bcm_plp_base_t_secy_sa_ca_t crypt_auth;
    } bcm_plp_base_t_secy_sa_action_t;

/*! bcm_plp_base_t_secy_sa_t
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
 *      <STRONG> Transform record format 24 * 32 bits </STRONG> \n
 *
 * @image html 64bit_Transform.jpg
 * @image html 32bit_Transform.jpg
 * The following fields must be written with zeroes when not used:
 * Key4 - Key7 fields, if cipher key is 128-bit long
 * Seq1 and IS0-S2, if 32-bit packet numbering is used
 *
 * <STRONG> Context ID word </STRONG>
 * The following fields of the transform record store the parameters needed for cryptographic transformations:
 *
 * @image html Control_Word.jpg
 * The context control word is the first 32-bit word in each transform record. It specifies the type of operation. Only those settings that are relevant for MACsec operations need to be defined.\n
 * The bit field diagram shows four rows of bit settings: two upper rows for egress operation and two lower rows for ingress operation: \n
 * 1.The 1st row is for egress operations with 32-bit frame numbering (hex base value 0x924BE066, 0x924DE066 or 0x924FE066 for 128, 192 and 256-bit AES key respectively) \n
 *   with as variable part the 2-bit association number (AN), which should be shifted left over 26 bits and added to that base value. \n
 * 2.  The 2nd row is for egress operations with 64-bit frame numbering (hex base value 0xA24BE066, 0xA24DE066 or 0xA24FE066 for 128, 192 and 256-bit AES key respectively)\n
 *    with as variable part the 2-bit association number (AN), which should be shifted left over 26 bits and added to that base value.
 * 3. The 3rd row is for ingress operations with 32-bit frame numbering (hex value 0xD24BE06F, 0xD24DE06F or 0xD24FE06F for 128, 192 and 256-bit AES key respectively) with no variable fields.\n
 * 4. The 4th row is for ingress operations with 64-bit frame numbering (hex value 0xE24BA0EF, 0xE24DA0EF or 0xE24FA0EF for 128, 192 and 256-bit AES key respectively) with no variable fields. \n
 *
 * <table>
 * <caption id="multi_row">Context control word 32 bit table</caption>
 * <tr><th>  Bits  <th>Name  <th>Description
 * </tr>
 * <tr> <td> [3:0] </td> <td> ToP </td> <td> Type of packet: the only valid values are 0110b for egress and 1111b for ingress </td> </tr>
 * <tr> <td> [4]  </td> <td> Reserved </td> <td> Write with zero and ignore on read </td> </tr>
 * <tr> <td> [5]  </td> <td> IV0 </td> <td> First word of IV present in context (is SCI for MACsec). Value depends on direction and presence of 64-bit packet numbering </td> </tr>
 * <tr> <td> [6]  </td> <td> IV1 </td> <td> Second word of IV present in context (is SCI for MACsec). Value depends on direction and presence of 64-bit packet numbering </td> </tr>
 * <tr> <td> [7]  </td> <td> IV2 </td> <td> Third word of IV present in context. Value depends on direction and presence of 64-bit packet numbering </td> </tr>
 * <tr> <td> [8]  </td> <td> keep_sectag </td> <td> 1b = SecTag is retained after macsec decryption, 0b = sectag is removed macsec decryption   </td> </tr>
 * <tr> <td> [9]  </td> <td> keep_icv </td> <td> 1b = ICV is retained after macsec decryption provided Sectag is also retained, 0b = ICV removed after decryption </td> </tr>
 * <tr> <td> [10] </td> <td> Rollover_mode </td> <td> Egress:1b = When nextPN reaching all 1's, it rolls over to value '1' without sequence number error generated.\n
 * Ingress: 1b = When packet is received with PN equal to all 1's, Sequence number field in transform record becomes '0'. This allows receiving and accepting packets with PN starting with value '1'.Key4 </td> </tr>
 * <tr> <td> [12-11] </td> <td> Reserved </td> <td> Write with zero and ignore on read </td> </tr>
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
 * <STRONG> Context ID word </STRONG>
 * This is a unique identifier for each context. In the EIP-160 it is sufficient to give all transform records a different context ID, possibly assigning a number from 0 to maximum index
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
 * It is derived from Key 0...Key 7 as follows: H_key=E(Key, 0^128). This means performing a 128-bit
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
 *  This makes the actual IV for the 32 MSB. For the 64 LSB, the EIP-62 only needs to perform an XOR of the lower 64-bits of the CtxSalt with the 64-bit packet number to get the IV.
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
 * <STRONG> SA update control word </STRONG>
 * For automatic SA expiry (all directions) and switching (egress-only MACsec), the transform record
 * contains an SA update control word that controls SA update when the packet number for the current SA is expired.
 *
  * <table>
 * <caption id="multi_row"> SA update control word </caption>
 * <tr><th>  Bits  <th>Name  <th>Description
 * </tr>
 * <tr> <td> [12-0] </td> <td> sa_index </td> <td> Egress: Index of the next SA. This index is valid only if sa_index_valid is set to 1b. \n
 *                                 Ingress: Index of the current SA. This value is used to set bit in the SA_EXP_SUMMARY register. </td> </tr>
 * <tr> <td> [13] </td> <td> update_time </td> <td> When set to 1b, during SA switch the current time stamp is written to the new transform record. </td> </tr>
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
 * <tr> <td>  [30:29]  </td> <td> an </td> <td> Egress: Must be to zero.
 * Ingress: AN number of the SA which of the 4 attached SA's invalidated  </td> </tr>
 * <tr> <td> [31] </td> <td> update_en </td> <td> Egress: SA index update enable. If set to 1b, enables the SA index update when the Packet Number for the current transform record has expired.
 * The current SA index in the corresponding flow control register is updated with the sa_index if sa_index_valid is 1b.
 * If sa_index_valid is 0b, the SA is invalidated by setting the value of the sa_in_use field to 0b in the flow control register.
 * Ingress: SA expiration enable. If set to 1b, enables the automatic inUse flag clearing when the Packet Number for the current transform record has expired </td> </tr>
 * </table>
 *
 * <STRONG> Time Stamps </STRONG>
 * Each transform record has space reserved for two time stamps: start time and stop time. When creating transform record, the host must initialize these values to 0.\n
 * After binding an SA to active secure channel, it must only read these values. the dedicated hardware module will update the time stamps.
 *
 * \arg params \n
 *     SA parameters \n
 *
 * \arg action_type \n
 *     SA action type, see bcm_plp_base_t_secy_SA_action_type_t \n
 *
 * \arg drop_type \n
 *     SA drop type, see bcm_plp_base_t_secy_drop_type_t \n
 *
 * \arg dest_port \n
 *     Destination port \n
 *
 */

typedef struct
{
    /** Size of the transform record (TransformRecord_p) associated with SA
       in 32-bit words */
    unsigned int sa_word_count;

    /** Pointer to the transform record data */
    unsigned int * transform_record_p;

    /** SA parameters */
    bcm_plp_base_t_secy_sa_action_t params;
    /** SA action type, see bcm_plp_base_t_secy_sa_sction_type_t */
    bcm_plp_base_t_secy_sa_sction_type_t action_type;

    /** SA drop type, see bcm_plp_base_t_secy_drop_type_t */
    bcm_plp_base_t_secy_drop_type_t drop_type;

    /** Destination port */
    bcm_plp_base_t_secy_port_type_t dest_port;

    /** 4-bit capture reason code */
    unsigned char capture_reason;

} bcm_plp_base_t_secy_sa_t;

/** MTU checking rule for packet post-processing of a Secure Channel
  (egress only) */
typedef struct
{
    /** Maximum allowed packet size (in bytes) */
    unsigned short int packet_max_byte_count;

    /** Action indication, if a packets is longer:\n
        true  - Drop packet (by corrupting the CRC)\n
        false - Pass packet on anyway */
    unsigned char fover_size_drop;

} bcm_plp_base_t_secy_sc_rule_mtu_check_t;

/** Actions per packet that did not match any SA */
typedef struct
{
    /** Action type\n
        false  - bypass,\n
        true   - perform drop action, see bcm_plp_base_t_secy_drop_type_t */
    unsigned char fbypass;

    /** Packet drop type, see bcm_plp_base_t_secy_drop_type_t */
    bcm_plp_base_t_secy_drop_type_t drop_type;

    /** Ingress only\n
        Perform drop action if packet is not from the reserved port */
    unsigned char fdrop_non_reserved;

    /** The destination port type */
    bcm_plp_base_t_secy_port_type_t dest_port;

    /** 4 bit Capture reason code */
    unsigned char capture_reason;

} bcm_plp_base_t_secy_rules_non_sa_t;

/** Actions per packet type for packets which did not match any SA,\n
   see bcm_plp_base_t_secy_SecTAG_Parser_t settings which is used to classify packets
   into these categories */
typedef struct
{
    /** rules for Untagged */
    bcm_plp_base_t_secy_rules_non_sa_t untagged;

    /** rules for Tagged */
    bcm_plp_base_t_secy_rules_non_sa_t tagged;

    /** rules for Badtag */
    bcm_plp_base_t_secy_rules_non_sa_t bad_tag;

    /** rules for KaY */
    bcm_plp_base_t_secy_rules_non_sa_t kaY;

} bcm_plp_base_t_secy_rules_sa_non_match_t;

/** Error drop rule settings */
typedef struct
{
    /** 1b = Drop frame on input packet error detect. */
    unsigned char fdrop_on_pkt_err;

    /** 1b = Drop frame on input CRC error detect. */
    unsigned char fdrop_On_crc_err;

    /** Defines the way the drop operation for header parser and (short)
        input packet/CRC error frames is performed. */
    bcm_plp_base_t_secy_drop_type_t err_drop_action;

    /** Dest port for header parser and (short) input packet/CRC error frames
        if not internal dropping. */
    bcm_plp_base_t_secy_port_type_t err_dest_port;

    /** Capture reason code for header parser and (short) input packet/CRC
        error frames if not internal dropping */
    unsigned char err_capt_reason;

} bcm_plp_base_t_secy_error_drop_settings_t;

/** The MACSEC_ID_CTRL register is intended for the ingress-only use-case,
    where the Packet Number in the frame can be replaced with a MACsec ID when
    the SC is not found. (in case the SC is found, the MACsec ID replacement
    can be controlled from the attached SA).\n
    @note This requires the vPort policy to retain the sectag. */
typedef struct
{
    /** Enable replacing PN with MACSec ID value in retained sectag. */
    unsigned char fmacsec_id_enb;

    /** MACSec ID value to replace PN with in retained sectag if SC is not
        found. */
    unsigned short int macsec_id;

} bcm_plp_base_t_secy_macsec_id_ctrl;

/** Store and Forward buffer control */
typedef struct
{
    /** 1b = Enable store and forward mode.\n
        0b = Disable store and forward mode. Engine operates in cut-through
        mode and frames cannot be dropped by post-processing
        (i.e. the other bits in this register are ignored). */
    unsigned char fenable;

    /** Enable dropping on a classification error. */
    unsigned char fdrop_class;

    /** Enable dropping on a post-processing error. */
    unsigned char fdrop_pp;

    /** Enable dropping on a security failure. */
    unsigned char fdrop_sec_fail;

    /** Enable dropping on a MTU overflow detection
        (egress-capable configuration only). */
    unsigned char fdrop_mtu;

    /** Enable dropping on an input MAC packet error detection. */
    unsigned char fdrop_mac_err;

    /** Enable dropping on an input MAC CRC error detection. */
    unsigned char fdrop_mac_crc;

    /** Low watermark value in 128-bit word (16 byte) units.\n
        When FIFO fill level becomes greater than or equal to this value,
        the saf_lwm output signal is set to 1b. Reset value is 50% of the
        SAF buffer depth. */
    unsigned short int low_watermark;

    /** High watermark value in 128-bit word (16 byte) units.\n
        When FIFO fill level becomes greater than or equal to this value,
        the saf_hwm output signal is set to 1b. Reset value is 75% of the
        SAF buffer depth. */
    unsigned short int high_watermark;
} bcm_plp_base_t_secy_saf_ctrl_t;

/** Context update control */
typedef struct
{
    /** 0b = strict comparison\n
        1b = greater or equal comparison */
    unsigned char fthreshold_mode;

    /** Threshold value used to trigger a context update through the context
        update interface */
    unsigned char update_threshold_value;
} bcm_plp_base_t_secy_ctx_update_ctrl_t;

/** bcm_plp_base_t_secy settings */
typedef struct
{
    /** Disable the MACsec crypto-core (EIP-62), \n
        send the packets around it to minimize latency*/
    unsigned char fstatic_bypass;

    /** Outbound sequence number threshold value (one global for all SA's)\n
        When non-0 the bcm_plp_base_t_secy device will generate a notification to indicate
        the threshold event which can be used to start the re-keying
        procedure.\n
        The notification will be generated only if it is requested
        by means of the bcm_plp_base_t_secy_Notify_Request(pa) function.\n
        If set to zero then only the sequence number roll-over notification
        will be generated.\n
        Note: This is a global parameter for all the SA's added to
              one bcm_plp_base_t_secy device. */
    unsigned int seqnr_threshold;

    /** Outbound sequence number threshold value for low-64-bit packet
        numbering */
    unsigned int seqnr_threshold_64_lo;
    /** Outbound sequence number threshold value for high-64-bit packet
        numbering */
    unsigned int seqnr_threshold_64_hi;

    /** Threshold for the SA frame counters low-32bits */
    unsigned int sa_count_frame_thr_lo;
    /** Threshold for the SA frame counters high-32bits */
    unsigned int sa_count_frame_thr_hi;
    /** Threshold for the SecY frame counters low-32bits  */
    unsigned int secy_count_frame_thr_lo;
    /** Threshold for the SecY frame counters high-32bits  */
    unsigned int secy_count_frame_thr_hi;
    /** Threshold for the IFC frame counters low-32bits */
    unsigned int ifc_count_frame_thr_lo;
    /** Threshold for the IFC frame counters high-32bits */
    unsigned int ifc_count_frame_thr_hi;
    /** Threshold for the RxCAM frame counters low-32bits */
    unsigned int rxcam_count_frame_thr_lo;
    /** Threshold for the RxCAM frame counters high-32bits */
    unsigned int rxcam_count_frame_thr_hi;
    /** Threshold for the TCAM frame counters - Low 32-bits*/
    unsigned int tcam_count_frame_thr_lo;
    /** Threshold for the TCAM frame counters - High 32-bits*/
    unsigned int tcam_count_frame_thr_hi;

    /** Threshold for the SA octet counters low-32bits */
    unsigned int sa_count_octet_thr_lo;
    /** Threshold for the SA octet counters high-32bits */
    unsigned int sa_count_octet_thr_hi;
    /** Threshold for the IFC octet counters low-32bits */
    unsigned int ifc_count_octet_thr_lo;
    /** Threshold for the IFC octet counters high-32bits */
    unsigned int ifc_count_octet_thr_hi;

    /** Prescale value for the time stamp tick counter */
    unsigned int gl_time_prescale;
    /** Time tick value used for timestamping the SAs */
    unsigned int gl_time_tick;

    /** MACsec ID Control */
    bcm_plp_base_t_secy_macsec_id_ctrl macsec_id_ctrl;

    /** Non-match rules (drop, bypass) Non-control (all vPorts) */
    bcm_plp_base_t_secy_rules_sa_non_match_t non_control_drop_bypass;
    /** Non-match rules (drop, bypass) control (all vPorts) */
    bcm_plp_base_t_secy_rules_sa_non_match_t control_drop_bypass;

    /** Error Drop rule settings */
    bcm_plp_base_t_secy_error_drop_settings_t error_drop_flow;

    /** Store and Forward buffer control */
    bcm_plp_base_t_secy_saf_ctrl_t saf_control;

    /** true - initialize device to pass packets in low-latency bypass mode\n
        false - initialize device for classification mode\n */
    unsigned char fbypass_no_class;

    /** Context update control */
    bcm_plp_base_t_secy_ctx_update_ctrl_t update_ctrl;

    /** Counter increment disable control values\n
        Each bit disables all the counters of one counter type. */
    unsigned char count_inc_dis_ctrl;

    /** Pointer to memory location where an array of 32-bit words
        is stored for the Input TCAM database of the bcm_plp_base_t_secy device.\n
        This array will be copied to the Input TCAM memory of the bcm_plp_base_t_secy device
        during its initialization.\n
        Set to NULL in order to retain to the default (or old) data
        in the Input TCAM memory. */
    unsigned int * input_tcam_p;

    /** Size of the InputTCAM_p array in 32-bit words */
    unsigned int input_tcam_word_count;

    /** 32-bit word offset in the Input TCAM memory where
        the InputTCAM_p array must be written */
    unsigned int word_offset;
    /** The fixed packet latency, if set to 0 then no fixed latency will be used
       Latency + 4 = engine clock cycles\n
       NOTE: do not set this value above 26 when setting the fStaticBypass.*/
    unsigned short int Latency;

    /** Number of words to keep in buffer for dynamic latency control */
    unsigned char DynamicLatencyWords;

    /** Enable dynamic latency control.*/
    unsigned char fDynamicLatency;

    /** Specify the maximum number of vPorts to be used by the driver.
        If set to zero, use the maximum supported by the hardware. */
    unsigned int MaxvPortCount;

    /** Specify the maximum number of Rules to be used by the driver.
        If set to zero, use the maximum supported by the hardware. */
    unsigned int MaxRuleCount;

    /** Specify the maximum number of SAs to be used by the driver.
        If set to zero, use the maximum supported by the hardware. */
    unsigned int MaxSACount;

    /** Specify the maximum number of SCs to be used by the driver.
        If set to zero, use the maximum supported by the hardware. */
    unsigned int MaxSCCount;

    /** Global Frame validation level:
     *  Applicable for drivers compiled with MACSEC_BASET_V2_2
     *   00b = disabled
     *   01b = check_command
     *   10b = strict
     *   11b = null
     */
    bcm_plp_base_t_secy_validate_frames_t ValidateFrames;

    /**
     * true - Enable 802.1AE compliant processing for correctly tagged MACsec
     * frames for wich no valid MACsec secure channel is found
     * Applicable for drivers compiled with MACSEC_BASET_V2_2
     */
    unsigned char fnm_macsec_en;

} bcm_plp_base_t_secy_settings_t;

/** SA statistics */
typedef struct
{
    /** Packet counters - Encrypted Protected */
    bcm_plp_base_t_secy_stat_counter_t out_pkts_encrypted_protected;
    /** Packet counters - TooLong */
    bcm_plp_base_t_secy_stat_counter_t out_pkts_too_long;
    /** Packet counters - SA Not In Use */
    bcm_plp_base_t_secy_stat_counter_t out_pkts_sa_not_inuse;

    /** Octet counters - Encrypted Protected */
    bcm_plp_base_t_secy_stat_counter_t out_octets_encrypted_protected;
} bcm_plp_base_t_secy_sa_stat_e_t;

/** SA counters */
typedef struct
{
    /** Packet counters - Unchecked */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_unchecked;
    /** Packet counters - Delayed */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_delayed;
    /** Packet counters - Late */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_late;
    /** Packet counters - OK */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_ok;
    /** Packet counters - Invalid */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_invalid;
    /** Packet counters - Not Valid */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_not_valid;
    /** Packet counters - Not Using SA */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_not_using_sa;
    /** Packet counters - Unused SA */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_unused_sa;

    /** Octet counters - Decrypted */
    bcm_plp_base_t_secy_stat_counter_t in_octets_decrypted;
    /** Octet counters - Validated */
    bcm_plp_base_t_secy_stat_counter_t in_octets_validated;
} bcm_plp_base_t_secy_sa_stat_i_t;

/** Union for SA Statistics counters for both Ingress & Egress */
typedef union
{
    /** SA Statistics for Egress */
    bcm_plp_base_t_secy_sa_stat_e_t egress;
    /** SA Statistics for Ingress */
    bcm_plp_base_t_secy_sa_stat_i_t ingress;
} bcm_plp_base_t_secy_sa_stat_t;


/** bcm_plp_base_t_secy statistics */
typedef struct
{
    /** Packet counters - Transform Error */
    bcm_plp_base_t_secy_stat_counter_t out_pkts_transform_error;
    /** Packet counters - Control */
    bcm_plp_base_t_secy_stat_counter_t out_pkts_control;
    /** Packet counters - Untagged */
    bcm_plp_base_t_secy_stat_counter_t out_pkts_untagged;
} bcm_plp_base_t_secy_secy_stat_e_t;

/** bcm_plp_base_t_secy counters */
typedef struct
{
    /** Packet counters - Transform Error */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_transform_error;
    /** Packet counters - Control */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_control;
    /** Packet counters - Untagged */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_untagged;
    /** Packet counters - No Tag */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_no_tag;
    /** Packet counters - Bad Tag */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_badtag;
    /** Packet counters - No SCI */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_no_sci;
    /** Packet counters - Unknown SCI */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_unknown_sci;
    /** Packet counters - Tagged Ctrl */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_tagged_ctrl;
} bcm_plp_base_t_secy_secy_stat_i_t;

/** Union for SecY Statistics counters for both Ingress & Egress */
typedef union
{
    /** Egress SecY Statistics */
    bcm_plp_base_t_secy_secy_stat_e_t egress;
    /** Ingress SecY Statistics */
    bcm_plp_base_t_secy_secy_stat_i_t ingress;
} bcm_plp_base_t_secy_secy_stat_t;

/** IFC (interface) statistics */
typedef struct
{
    /** Packet counters - Unicast Uncontrolled */
    bcm_plp_base_t_secy_stat_counter_t out_pkts_unicast_uncontrolled;
    /** Packet counters - Multicast Uncontrolled */
    bcm_plp_base_t_secy_stat_counter_t out_pkts_multicast_uncontrolled;
    /** Packet counters - Broadcast Uncontrolled */
    bcm_plp_base_t_secy_stat_counter_t out_pkts_broadcast_uncontrolled;

    /** Packet counters - Unicast Controlled */
    bcm_plp_base_t_secy_stat_counter_t out_pkts_unicast_controlled;
    /** Packet counters - Multicast Controlled */
    bcm_plp_base_t_secy_stat_counter_t out_pkts_multicast_controlled;
    /** Packet counters - Broadcast Controlled */
    bcm_plp_base_t_secy_stat_counter_t out_pkts_broadcast_controlled;

    /** Octet counters - Uncontrolled */
    bcm_plp_base_t_secy_stat_counter_t out_octets_uncontrolled;
    /** Octet counters - Controlled */
    bcm_plp_base_t_secy_stat_counter_t out_octets_controlled;
    /** Octet counters - Common */
    bcm_plp_base_t_secy_stat_counter_t out_octets_common;
} bcm_plp_base_t_secy_ifc_stat_e_t;

/** Statistics counters*/
typedef struct
{
    /** Packet counters - Unicast uncontrolled */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_unicast_uncontrolled;
    /** Packet counters - Multicast Uncontrolled */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_multicast_uncontrolled;
    /** Packet counters - Broadcast Uncontrolled */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_broadcast_uncontrolled;

    /** Packet counters - Unicast Controlled */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_unicast_controlled;
    /** Packet counters - Multicast Controlled */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_multicast_controlled;
    /** Packet counters - Broadcast Controlled */
    bcm_plp_base_t_secy_stat_counter_t in_pkts_broadcast_controlled;

    /** Octet counters - Uncontrolled */
    bcm_plp_base_t_secy_stat_counter_t in_octets_uncontrolled;
    /** Octet counters - Controlled */
    bcm_plp_base_t_secy_stat_counter_t in_octets_controlled;
} bcm_plp_base_t_secy_ifc_stat_i_t;

/** Union for Ifc Statistics counters for both Ingress & Egress */
typedef union
{
    /** Ifc Statistics counters for Egress */
    bcm_plp_base_t_secy_ifc_stat_e_t egress;
    /** Ifc Statistics counters for ingress */
    bcm_plp_base_t_secy_ifc_stat_i_t ingress;
} bcm_plp_base_t_secy_ifc_stat_t;

/** TCAM statistics */
typedef struct
{
    /** Statistics counter */
    bcm_plp_base_t_secy_stat_counter_t tcam_hit;

} bcm_plp_base_t_secy_tcam_stat_t;

/** RxCAM statistics (ingress only) */
typedef struct
{
    /** Packet counter */
    bcm_plp_base_t_secy_stat_counter_t cam_hit;
} bcm_plp_base_t_secy_rxcam_stat_t;

/** Global statistics */
typedef struct
{
    /** Number of frames that missed the TCAM */
    bcm_plp_base_t_secy_stat_counter_t tcam_miss;

    /** Number of frames that hit multiple TCAM entries simultaneously */
    bcm_plp_base_t_secy_stat_counter_t tcam_hit_multiple;

    /** Frames dropped by header parser as invalid */
    bcm_plp_base_t_secy_stat_counter_t header_parser_dropped_kts;

} bcm_plp_base_t_secy_global_stat_t;

/** Egress header control */
typedef struct
{
    /** true - enabled */
    unsigned char fenable;

    /** Egress header Etype */
    unsigned short int egress_header_etype;

} bcm_plp_base_t_secy_egress_header_t;


/** ST-VLAN parser control */
typedef struct
{
    /** true - enabled */
    unsigned char fenable;

    /** STVLAN Etype */
    unsigned short int st_vlan_etype;

} bcm_plp_base_t_secy_st_vlan_parser_t;

/** MACsec SecTAG parser control */
typedef struct
{
    /** true - compare ether_type in packet against ether_type value */
    unsigned char fcomp_type;

    /** true - check V (Version) bit to be 0 */
    unsigned char fcheck_version;

    /** true - check if packet is meant to be handled by KaY (C & E bits) */
    unsigned char fcheck_kay;

    /** true - check illegal C==1 & E==0 flags combinations */
    unsigned char fcheck_ce;

    /** true - check illegal SC/ES/SCB bit combinations */
    unsigned char fcheck_sc;

    /** true - check SL (Short Length) field contents out-of-range */
    unsigned char fcheck_sl;

    /** true - check PN (Frame number) to be non-zero for all cipher suites
        and regardless of SA hit */
    unsigned char fcheck_pn;

    /** true - extended SL (Short length) check enabled. */
    unsigned char fcheck_sl_ext;

    /** ether_type value used for MACsec tag type comparison */
    unsigned short int ether_type;

} bcm_plp_base_t_secy_sectag_parser_t;

/** MPLS label select for packets with 2 labels */
typedef enum
{
    BCM_PLP_BASE_T_SECY_MPLS3_SELECT_LABEL1 = 0,         /**< \n */
    BCM_PLP_BASE_T_SECY_MPLS3_SELECT_LABEL2,             /**< \n */
    BCM_PLP_BASE_T_SECY_MPLS3_SELECT_LABEL3              /**< \n */
} bcm_plp_base_t_secy_mpls3_select_t;

/** MPLS label select for packets with 4 labels */
typedef enum
{
    BCM_PLP_BASE_T_SECY_MPLS4_SELECT_LABEL1 = 0,         /**< \n */
    BCM_PLP_BASE_T_SECY_MPLS4_SELECT_LABEL2,             /**< \n */
    BCM_PLP_BASE_T_SECY_MPLS4_SELECT_LABEL3,             /**< \n */
    BCM_PLP_BASE_T_SECY_MPLS4_SELECT_LABEL4              /**< \n */
} bcm_plp_base_t_secy_mpls4_select_t;

/** MPLS label select for packets with >= 5 labels */
typedef enum
{
    BCM_PLP_BASE_T_SECY_MPLS5_SELECT_LABEL1 = 0,         /**< \n */
    BCM_PLP_BASE_T_SECY_MPLS5_SELECT_LABEL2,             /**< \n */
    BCM_PLP_BASE_T_SECY_MPLS5_SELECT_LABEL3,             /**< \n */
    BCM_PLP_BASE_T_SECY_MPLS5_SELECT_LABEL4,             /**< \n */
    BCM_PLP_BASE_T_SECY_MPLS5_SELECT_LABEL5              /**< \n */
} bcm_plp_base_t_secy_mpls5_select_t;

/** MPLS ethertype compare value */
typedef struct
{
    /** true - use MPLS_Etype value for comparison with ethertype filed
               in an MPLS packet */
    unsigned char fcompare;

    /** Value of ethertype field to compare with ethertype in an MPLS packet */
    unsigned short int mpls_etype;

} bcm_plp_base_t_secy_mpls_etype_t;

/** MPLS parsing control */
typedef struct
{
    /** MPLS packet with 3 labels: select 2 labels to use in TCAM key */
    bcm_plp_base_t_secy_mpls3_select_t mpls3_select1; /** 1st label select */
    bcm_plp_base_t_secy_mpls3_select_t mpls3_select2; /** 2nd label select */

    /** MPLS packet with 4 labels: select 2 labels to use in TCAM key */
    bcm_plp_base_t_secy_mpls4_select_t mpls4_select1; /** 1st label select */
    bcm_plp_base_t_secy_mpls4_select_t mpls4_select2; /** 2nd label select */

    /** MPLS packet with 5 labels: select 2 labels to use in TCAM key */
    bcm_plp_base_t_secy_mpls5_select_t mpls5_select1; /** 1st label select */
    bcm_plp_base_t_secy_mpls5_select_t mpls5_select2; /** 2nd label select */

    /** Allow SecTAG detection for more than 5 MPLS labels */
    unsigned char bmpls_parse_gt5_lbl;

    /** Ethertype values (max XTSECY_MPLS_ETYPE_MAX_COUNT) to use for
        comparison with ethertype field in MPLS packets */
    bcm_plp_base_t_secy_mpls_etype_t mpls_etype [BCM_PLP_BASE_T_SECY_MPLS_ETYPE_MAX_COUNT];

} bcm_plp_base_t_secy_mpls_parser_t;

/** PBB ethertype compare value */
typedef struct
{
    /** Value of ethertype field to compare with "BVID" ethertype in
        an PBB packet */
    unsigned short int btag_etype;

    /** Value of ethertype field to compare with "ISID" ethertype in
        an PBB packet */
    unsigned short int itag_etype;

} bcm_plp_base_t_secy_pbb_etype_t;

/** PBB parsing control */
typedef struct
{
    /** true - use PBB header parsing,
               BTagEtype and ITagEtype values for comparison with ethertype
               fields in an PBB packet */
    unsigned char fcompare;

    /** PBB Etype values. Set to NULL if no update is required */
    bcm_plp_base_t_secy_pbb_etype_t pbb_etype;

} bcm_plp_base_t_secy_pbb_parser_t;


/** VLAN tags parsing control */
typedef struct
{
    /** Enable detection of VLAN tags matching QTag value */
    unsigned char fparse_qtag;

    /** Enable detection of VLAN tags matching STag1 value */
    unsigned char fparse_stag1;

    /** Enable detection of VLAN tags matching STag2 value */
    unsigned char fparse_stag2;

    /** Enable detection of VLAN tags matching STag3 value */
    unsigned char fparse_stag3;

    /** Enable detection of multiple back-2-back VLAN tags
       (Q-in-Q and beyond) */
    unsigned char fparse_qinq;

} bcm_plp_base_t_secy_vlan_parse_tag_t;


/** VLAN parsing control\n
    The classification engine is able to detect up to five VLAN tags following
    the outer MAC_SA and parse the first two for packet classification */
typedef struct
{
    /** 1. VLAN parsing settings
        Input header parser VLAN tags parsing settings */
    bcm_plp_base_t_secy_vlan_parse_tag_t cp;
    /** true - update the secondary header parser VLAN tags parsing settings
                   with values from the SCP data structure member\n
            false - no update is required */
    unsigned char fscp;

    /** Secondary header parser VLAN tags parsing settings */
    bcm_plp_base_t_secy_vlan_parse_tag_t scp;

    /** 2. Global VLAN parsing settings
        Enable user priority processing for '802.1s' packets. If set,
        when a '802.1s' tag is found, take the user priority directly from
        the PCP field, otherwise take the default user priority. */
    unsigned char fstag_up_enable;

    /** Enable user priority processing for '802.1Q' packets. If set,
        when a '802.1Q' tag is found, translate the PCP field using UpTable1,
        otherwise take the default user priority. */
    unsigned char fqtag_up_enable;

    /** Default user priority, assigned to non-VLAN packets and
       to VLAN packets for which the VLAN user priority processing
       is disabled */
    unsigned char default_up; /** Allowed values in range 0 .. 7! */

    /** Translation tables to derive the user priority from the PCP field
        in '802.1Q' tags. If the PCP field is 0 then take UpTable[0] as
        the user priority, if PCP=1 then take UpTable[1], etc. */

    /** Translation table for 1st '802.1Q' tag,
        allowed values in range 0 .. 7! */
    unsigned char up_table1 [BCM_PLP_BASE_T_SECY_VLAN_UP_MAX_COUNT];

    /** Translation table for 2nd '802.1Q' tag,
        allowed values in range 0 .. 7! */
    unsigned char up_table2 [BCM_PLP_BASE_T_SECY_VLAN_UP_MAX_COUNT];

    /** Ethertype value used for '802.1Q' tag type comparison */
    unsigned short int qtag;

    /** Ethertype value used for first '802.1s' tag type comparison */
    unsigned short int stag1;
    /** Ethertype value used for second '802.1s' tag type comparison */
    unsigned short int stag2;
    /** Ethertype value used for third '802.1s' tag type comparison */
    unsigned short int stag3;

} bcm_plp_base_t_secy_vlan_parser_t;

/** SNAP/LLC Jumbo parsing control */
typedef struct
{
    /** true is enabled */
    unsigned char fenable;

    /** ether_type for a Jumbo frame */
    unsigned short int jumbo_etype;
} bcm_plp_base_t_secy_snap_llc_parse_jumbo_t;

/** LLC parsing control */
typedef struct
{
    /** true is enabled */
    unsigned char fenable;
    /** Max length of the LLC */
    unsigned short int llc_max_len;
} bcm_plp_base_t_secy_llc_parser_t;

/** SNAP parsing control */
typedef struct
{
    /** true is enabled */
    unsigned char fenable;

    /** true is parse SNAP enabled */
    unsigned char fparse_snap_eth;

    /** true is force SNAP enabled */
    unsigned char fforce_snap_eth;

    /** Value used to compare to for SNAP detection */
    unsigned int snap_value;

    /** Mask used to compare to for SNAP detection */
    unsigned int snap_mask;
} bcm_plp_base_t_secy_snap_parser_t;


/** LLC/SNAP Parsing control */
typedef struct
{
    /** SNAP LLC Jumbo Ethertype */
    bcm_plp_base_t_secy_snap_llc_parse_jumbo_t jumbo_etype;

    /** LLC Parsing */
    bcm_plp_base_t_secy_llc_parser_t llc_parser;

    /** SNAP Parser */
    bcm_plp_base_t_secy_snap_parser_t snap_parser;

} bcm_plp_base_t_secy_snap_llc_parser_t;

/** Header Parser control */
typedef struct
{
    /** Egress header settings. Set to NULL if no update is required */
    bcm_plp_base_t_secy_egress_header_t * egress_header_p;

    /** SecTAG parser settings. Set to NULL if no update is required */
    bcm_plp_base_t_secy_sectag_parser_t * sectag_parser_p;

    /** (Eo)MPLS parser settings. Set to NULL if no update is required */
    bcm_plp_base_t_secy_mpls_parser_t * mpls_parser_p;

    /** PBB parser settings. Set to NULL if no update is required */
    bcm_plp_base_t_secy_pbb_parser_t * pbb_parser_p;

    /** VLAN parser settings. Set to NULL if no update is required */
    bcm_plp_base_t_secy_vlan_parser_t * vlan_parser_p;

    /** ST-VLAN settings. Set to NULL if no update is required */
    bcm_plp_base_t_secy_st_vlan_parser_t * st_vlan_parser_p;

    /** SNAP/LLC settings. Set to NULL if no update is required */
    bcm_plp_base_t_secy_snap_llc_parser_t * snap_llc_parser_p;

} bcm_plp_base_t_secy_header_parser_t;

/** Packet match rule using MAC destination address and ether_type */
typedef struct
{
    /** MAC destination address, 6 bytes */
    unsigned char * mac_da_p; /** Set to NULL if not used */

    /** ether_type */
    unsigned short int ether_type;

} bcm_plp_base_t_secy_mac_da_et_match_rule_t;

/** Packet match rule using range of MAC destination addresses */
typedef struct
{
    /** Start MAC destination address, 6 bytes */
    unsigned char * mac_da_start_p; /** Set to NULL if not used */

    /** End MAC destination address, 6 bytes */
    unsigned char * mac_da_end_p;   /** Only applicable if mac_da_Start_p
                              is not NULL */

} bcm_plp_base_t_secy_mac_da_range_match_rule_t;

/** Packet match rule using range of MAC destination addresses and ether_type */
typedef struct
{
    /** Range of MAC destination addresses, start and end */
    bcm_plp_base_t_secy_mac_da_range_match_rule_t range;

    /** ether_type */
    unsigned short int ether_type;

} bcm_plp_base_t_secy_mac_da_et_range_match_rule_t;

/** Control Packet Detector settings */
typedef struct
{
    /** MAC destination address and ether_type
        Set mac_da_et_rules[n].mac_da_p to NULL if no update is required,
        corresponding bits in cp_match_enable_mask should
        be set to 0 */
    bcm_plp_base_t_secy_mac_da_et_match_rule_t
                    mac_da_et_rules[BCM_PLP_BASE_T_SECY_MAC_DA_ET_MATCH_RULES_COUNT];

    /** Range of MAC destination address used with ether_type
        Set mac_da_et_Range[n].Range.mac_da_Start_p and
        mac_da_et_Range[n].range.mac_da_End_p to NULL if no update is required,
        corresponding bits in cp_match_enable_mask should
        be set to 0 */
    bcm_plp_base_t_secy_mac_da_et_range_match_rule_t
                mac_da_et_range[BCM_PLP_BASE_T_SECY_MAC_DA_ET_RANGE_MATCH_RULES_COUNT];

    /** Range of MAC destination address (no ether_type)
        Set mac_da_Range[n].mac_da_Start_p and mac_da_Range[n].mac_da_end_p
        to NULL if no update is required, corresponding bits in
        cp_match_enable_mask should be set to 0 */
    bcm_plp_base_t_secy_mac_da_range_match_rule_t mac_da_range;

    /** 44-bit MAC destination address constant, 6 bytes
        Set to NULL if no updated is required, corresponding bits in
        cp_match_enable_mask should be set to 0 */
    unsigned char * mac_da_44bit_const_p;

    /** 48-bit MAC destination address constant, 6 bytes\n
        Set to NULL if no updated is required, corresponding bits in
        cp_match_enable_mask should be set to 0 */
    unsigned char * mac_da_48bit_const_p;

    /** Match mask used to select which ether_type to compare with the ether_type
        value in the matching rule\n
        bit i (0-7): 1 enables packet MAC destination address comparison with
                     the respective mac_da_et_rules[i].ether_type address\n
        bit j (8-9): 1 enables packet MAC destination address comparison with
                     the respective mac_da_et_range[j-8].ether_type address\n
        Primary Control Packet Detector */
    unsigned int cp_match_mode_mask;

    /** Secondary Control Packet Detector */
    unsigned int scp_match_mode_mask;

    /** Match mask which can be used to enable/disable control packet
        matching rules\n
        bit i (0-7): 1 enables packet MAC destination address comparison with
                     the respective mac_da_et_rules[i].mac_da_p address\n
        bit j (8-15): 1 enables packet Ethernet Type value comparison with
                      the respective mac_da_et_rules[j-8].ether_type value\n
        bit 16: 1 enables range of packet MAC destination address as well as
                Ethernet Type value comparison with the respective
                mac_da_et_Range[0].range.mac_da_start_p and
                mac_da_et_Range[0].range.mac_da_end_p values as well as
                mac_da_et_Range[0].ether_type value\n
        bit 17: 1 enables range of packet MAC destination address as well as
                Ethernet Type value comparison with the respective
                mac_da_et_Range[1].range.mac_da_start_p and
                mac_da_et_Range[1].range.mac_da_end_p values as well as
                mac_da_et_Range[1].ether_type value\n
        bit 18: 1 enables range of packet MAC destination addresses comparison
                with mac_da_Range.mac_da_Start_p and
                mac_da_Range.mac_da_end_p\n
        bit 19: 1 enables packet MAC destination addresses comparison with
                mac_da_44Bit_Const_p constant\n
        bit 20: 1 enables packet MAC destination addresses comparison with
                mac_da_48Bit_Const_p constant\n
        bit 31: 1 enables MACsec KaY packets as control packets\n
        Primary Control Packet Detector */
    unsigned int cp_match_enable_mask;

    /** Secondary Control Packet Detector */
    unsigned int scp_match_enable_mask;
} bcm_plp_base_t_secy_control_packet_t;

/** Device bypass control */
typedef struct
{
    /** Indication if the classifier bypass must be enabled (true)
        or the classifier mode (false).*/
    unsigned char fbypass_no_class;

    /** Disable the MACsec crypto-core (EIP-62), \n
        send the packets around it to minimize latency*/
    unsigned char fstatic_bypass;

} bcm_plp_base_t_secy_device_control_t;

/** Device statistics control */
typedef struct
{
    /** true - statistics counters are automatically reset on
        a read operation\n
        false - no reset on a read operation */
    unsigned char fauto_stat_cntrs_reset;

    /** Threshold for the SA frame counters */
    bcm_plp_base_t_secy_stat_counter_t  sa_count_frame_thr;

    /** Threshold for the SecY frame counters  */
    bcm_plp_base_t_secy_stat_counter_t  secy_count_frame_thr;

    /** Threshold for the IFC frame counters */
    bcm_plp_base_t_secy_stat_counter_t ifc_count_frame_thr;

    /** Threshold for the RxCAM frame counters */
    bcm_plp_base_t_secy_stat_counter_t rxcam_count_frame_thr;

    /** Threshold for the TCAM frame counters*/
    bcm_plp_base_t_secy_stat_counter_t  tcam_count_frame_thr;

    /** Threshold for the SA octet counters */
    bcm_plp_base_t_secy_stat_counter_t sa_count_octet_thr;

    /** Threshold for the IFC octet counters */
    bcm_plp_base_t_secy_stat_counter_t  ifc_count_octet_thr;

    /** Counter increment disable control values\n
        Each bit disables all the counters of one counter type. */
    unsigned char count_inc_dis_ctrl;

} bcm_plp_base_t_secy_statistics_control_t;

/** Non matching rule settings */
typedef struct
{
    /** Non-matching rules (drop, bypass) Non-control (all vPorts) */
    bcm_plp_base_t_secy_rules_sa_non_match_t non_control_drop_bypass;

    /** Non-matching rules (drop, bypass) control (all vPorts) */
    bcm_plp_base_t_secy_rules_sa_non_match_t control_drop_bypass;

} bcm_plp_base_t_secy_non_matching_control_t;

/** Device control */
typedef struct
{
    /** Bypass settings. Set to NULL if no update is required */
    bcm_plp_base_t_secy_device_control_t * control_p;

    /** Control packet detection settings.
        Set to NULL if no update is required */
    bcm_plp_base_t_secy_control_packet_t * cp_p;

    /** Header parser settings. Set to NULL if no update is required */
    bcm_plp_base_t_secy_header_parser_t * header_parser_p;

    /** Statistics settings. Set to NULL if no update is required */
    bcm_plp_base_t_secy_statistics_control_t * stat_control_p;

    /** Non-matching rule settings */
    bcm_plp_base_t_secy_non_matching_control_t * nm_control_p;

    /** Store and Forward buffer control */
    bcm_plp_base_t_secy_saf_ctrl_t * saf_control_p;

    /** Context update control */
    bcm_plp_base_t_secy_ctx_update_ctrl_t * update_ctrl_p;

} bcm_plp_base_t_secy_device_params_t;

/** Packet types */
typedef enum
{
    BCM_PLP_BASE_T_SECY_RULE_PKT_TYPE_OTHER  = 0,        /**< untagged, VLAN, etc\n */
    BCM_PLP_BASE_T_SECY_RULE_PKT_TYPE_MPLS   = 1,        /**< \n */
    BCM_PLP_BASE_T_SECY_RULE_PKT_TYPE_PBB    = 2         /**< \n */
} bcm_plp_base_t_secy_rule_packet_type_t;

/** Frame types */
typedef enum
{
    BCM_PLP_BASE_T_SECY_RULE_FRAME_TYPE_OTHER  = 0,      /**< Ethernet II\n */
    BCM_PLP_BASE_T_SECY_RULE_FRAME_TYPE_ETH    = 1,      /**< \n */
    BCM_PLP_BASE_T_SECY_RULE_FRAME_TYPE_LLC    = 2,      /**< \n */
    BCM_PLP_BASE_T_SECY_RULE_FRAME_TYPE_SNAP   = 3       /**< \n */
} bcm_plp_base_t_secy_rule_frame_type_t;

/** vPort matching rule Key/Mask data structure */
typedef struct
{
    /** rule Packet type */
    bcm_plp_base_t_secy_rule_packet_type_t packet_type;

    /** Bit 0 = 1 : No (ST)VLAN tags\n
        Bit 1 = 1 : 1 (ST)VLAN tag\n
        Bit 2 = 1 : 2 VLAN tags\n
        Bit 3 = 1 : 3 VLAN tags\n
        Bit 4 = 1 : 4 VLAN tags\n
        Bit 5 = 1 : 5 Reserved\n
        Bit 6 = 1 : >4 VLAN tags */
    unsigned char num_tags; /**< bit mask, only 7 bits [6:0] are used,
                     see above how */

    /** true is STVLAN enabled */
    unsigned char st_vlan;

    /** rule for frame type */
    bcm_plp_base_t_secy_rule_frame_type_t frame_type;

    /** true is MaCsec tagged */
    unsigned char macsec_tagged;
    /** true is coming from Redirect */
    unsigned char from_Redirect;

} bcm_plp_base_t_secy_rule_key_mask_t;

/** vPort matching rule policy */
typedef struct
{
    /** vPort handle obtained via bcm_plp_base_t_secy_vPort_Add(pa) function */
    bcm_plp_base_t_secy_vport_handle_t vport_handle;

    /** Priority value that is used to resolve multiple rule matches.
        When multiple rules are hit by a packet simultaneously, the rule with
        the higher priority value will be returned. If multiple rules with
        an identical priority value are hit, the rule with the lowest
        rule index is used. */
    unsigned char priority;

    /** true : drop the packet */
    unsigned char fdrop;

    /** true : process the packet as control packet */
    unsigned char fcontrol_packet;

} bcm_plp_base_t_secy_rule_policy_t;

/** vPort matching rule data structure */
typedef struct
{
    /** Sets matching values as specified in bcm_plp_base_t_secy_rule_key_mask_t */
    bcm_plp_base_t_secy_rule_key_mask_t key;

    /** Mask for matching values, can be used to mask out
        irrelevant Key bits */
    bcm_plp_base_t_secy_rule_key_mask_t mask;

    /** Data[0] : MAC Destination Address least significant bytes (3..0)\n
        Data[1] : MAC Destination Address most significant bytes (5, 4)\n
        Data[1] : MAC Source Address least significant bytes (1, 0)\n
        Data[2] : MAC Source Address most significant bytes (5..2)\n
        Data[3] : Frame data (ether_type, VLAN tag, MPLS label, etc..)\n
        Data[4] : Frame data (ether_type, VLAN tag, MPLS label, etc..)\n
        See TCAM packet data fields description in the
        EIP-160 Programmer Manual */
    unsigned int data [BCM_PLP_BASE_T_SECY_RULE_NON_CTRL_WORD_COUNT];

    /** Mask for data values, can be used to mask out irrelevant Data bits */
    unsigned int data_mask [BCM_PLP_BASE_T_SECY_RULE_NON_CTRL_WORD_COUNT];

    /** rule policy */
    bcm_plp_base_t_secy_rule_policy_t policy;

} bcm_plp_base_t_secy_rule_t;
typedef struct bcm_plp_base_t_version_s
{
    unsigned int major_no;
    unsigned int minor_no;
}bcm_plp_base_t_version_t;

/** Status returned by each of the functions in this API*/
typedef enum {
    BCM_PLP_BASE_T_SAB_STATUS_OK, /**< \n */
    BCM_PLP_BASE_T_SAB_INVALID_PARAMETER, /**< \n */
    BCM_PLP_BASE_T_SAB_UNSUPPORTED_FEATURE, /**< \n */
    BCM_PLP_BASE_T_SAB_ERROR /**< \n */
} bcm_plp_base_t_sa_builder_status_t;

/** Specify direction: egress (encrypt) or ingress (decrypt) */
typedef enum {
    BCM_PLP_BASE_T_SAB_DIRECTION_EGRESS, /**< \n */
    BCM_PLP_BASE_T_SAB_DIRECTION_INGRESS /**< \n */
} bcm_plp_base_t_sa_builder_direction_t;


/** Operation type */
typedef enum {
    BCM_PLP_BASE_T_SAB_OP_MACSEC,            /**< MACsec operation (default) */
    BCM_PLP_BASE_T_SAB_OP_ENCAUTH_AES_GCM,   /**< Test operation for authenticate-encrypt. */
    BCM_PLP_BASE_T_SAB_OP_ENC_AES_CTR       /**< Test operation for encryption. */
} bcm_plp_base_t_sa_builder_operation_t;


/** Flags for the flags field. Put a bitwise or (or zero) of any
 *  *  of the flags in this field.*/

/** Use 64-bit sequence number. */
#define BCM_PLP_BASE_T_SAB_MACSEC_FLAG_LONGSEQ 0x1
/** Retain SecTAG (debugging).*/
#define BCM_PLP_BASE_T_SAB_MACSEC_FLAG_RETAIN_SECTAG 0x2
/** Retain ICV (debugging). */
#define BCM_PLP_BASE_T_SAB_MACSEC_FLAG_RETAIN_ICV 0x4
/** Allow sequence number rollover (debugging).*/
#define BCM_PLP_BASE_T_SAB_MACSEC_FLAG_ROLLOVER 0x8
/** Enable SA auto update. */
#define BCM_PLP_BASE_T_SAB_MACSEC_FLAG_UPDATE_ENABLE 0x10
/** Generate IRQ when SA is expired. */
#define BCM_PLP_BASE_T_SAB_MACSEC_FLAG_SA_EXPIRED_IRQ 0x20
/** Update time stamps (only if hardware supports this feature).*/
#define BCM_PLP_BASE_T_SAB_MACSEC_FLAG_UPDATE_TIME   0x40

/*! Input parameters for the SA Builder
 * \arg flags
 * flags, either 0 or the bitwise or of one or more flag values described above SAB_MACSEC_FLAG
 * defined flag
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
 */
typedef struct {
    unsigned int flags;
    bcm_plp_base_t_sa_builder_direction_t direction;
    bcm_plp_base_t_sa_builder_operation_t operation;
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
} bcm_plp_base_t_sa_builder_params_t;

/** SecY device limits */
/** Applicable for drivers compiled with MACSEC_BASET_V2_2 */
typedef struct {
    /** Major HW version */
    unsigned char major_version;

    /** Minor HW version */
    unsigned char minor_version;

    /** HW patch level */
    unsigned char patch_level;

    /** EIP-number */
    unsigned char eip_number;

    /** true - MACsec 2018 supported
        false - MACsec 2018 not supported */
    unsigned char fmacsec2018_support;

    /** true - Store-and-forward buffer available.
        false - Store-and-forward buffer not available */
    unsigned char fsaf_buffer;
} bcm_plp_base_t_secy_device_capabilities_t;

/*! Status values returned by bcm_plp_macsec_warmboot API functions */
typedef enum {
#if !defined (BASET_HSIP_MACSEC_SUPPORT)
    BCM_PLP_WARMBOOT_STATUS_OK,
    BCM_PLP_WARMBOOT_ERROR_BAD_PARAMETER,
    BCM_PLP_WARMBOOT_INTERNAL_ERROR,
    BCM_PLP_WARMBOOT_ERROR_ALLOCATION
#else
    BCM_PLP_BASE_T_WARMBOOT_STATUS_OK,
    BCM_PLP_BASE_T_WARMBOOT_ERROR_BAD_PARAMETER,
    BCM_PLP_BASE_T_WARMBOOT_INTERNAL_ERROR,
    BCM_PLP_BASE_T_WARMBOOT_ERROR_ALLOCATION
#endif
} bcm_plp_base_t_warmboot_status_t;


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
typedef bcm_plp_base_t_warmboot_status_t (* bcm_plp_base_t_warmboot_alloc_callback_t)
                                     (const bcm_plp_base_t_sec_access_t *pa,
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
typedef bcm_plp_base_t_warmboot_status_t (* bcm_plp_base_t_warmboot_read_callback_t)
                                     (const bcm_plp_base_t_sec_access_t *pa,
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
typedef bcm_plp_base_t_warmboot_status_t (* bcm_plp_base_t_warmboot_write_callback_t)
                                     (const bcm_plp_base_t_sec_access_t *pa,
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
typedef bcm_plp_base_t_warmboot_status_t (* bcm_plp_base_t_warmboot_free_callback_t) (const bcm_plp_base_t_sec_access_t *pa,
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
    bcm_plp_base_t_warmboot_alloc_callback_t alloc_cb;
    bcm_plp_base_t_warmboot_free_callback_t free_cb;
    bcm_plp_base_t_warmboot_write_callback_t write_cb;
    bcm_plp_base_t_warmboot_read_callback_t read_cb;
} bcm_plp_base_t_macsec_warmboot_callbacks_t;

/*! Macsec Warmboot Device Types */
typedef enum
{
    BCM_PLP_WARMBOOT_DEVICE_TYPE_XTSECY_E,
    BCM_PLP_WARMBOOT_DEVICE_TYPE_XTSECY_I,
#if !defined (BASET_HSIP_MACSEC_SUPPORT)
    BCM_PLP_WARMBOOT_DEVICE_TYPE_DF,
#else
    BCM_PLP_WARMBOOT_DEVICE_TYPE_XTSECY_DF,
#endif
} bcm_plp_base_t_macsec_warmboot_device_type_t;

#endif /* BCM_BASE_T_COMMON_SEC_DEFINES_H */
