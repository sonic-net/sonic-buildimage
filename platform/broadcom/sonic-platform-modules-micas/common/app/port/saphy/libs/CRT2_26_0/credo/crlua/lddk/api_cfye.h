/** @file api_cfye.h
 *
 * @brief Classification Engine (CfyE) API.
 *
 * This API can be used to create and configure packet classification
 * rules for the hardware Classification device, e.g. the functionality not
 * covered by the IEEE 802.1AE MACsec standard.
 *
 * This API is an extension of the SecY API and should be used
 * together with the latter. A data-plane packet I/O API should be used
 * in addition to these APIs for packet data input and output operations.
 *
 * This API implementation can be instantiated for one or several
 * Classification Engines.
 *
 * Refer to the API Implementation Notes for implementation-specific details,
 * see Security-IP_CfyE-API_Implementation-Notes.txt file.
 *
 * Typical CfyE API usage example:
 *
 * 1. Initialize the Classification Engine (Device) with CfyE_Device_Init() for
 *    either the egress or the ingress role, depending on the Device hardware
 *    configuration. The low-latency bypass mode can be configured with this
 *    function as well. The Device limits can be queried via CfyE_Device_Limits()
 *
 * 2. Update the Device default classification settings with CfyE_Device_Update()
 *    for a specific Channel. Multiple Channels can be supported by the Device.
 *    The setting for the Device header parser can be updated, control packet
 *    detection, low-latency bypass and statistics control.
 *
 *    CfyE_Device_Update() can also be used to configure the CfyE summary
 *    threshold global and channel-specific events. Upon occurrence these events
 *    can be cleared with CfyE_Statistics_Summary_Read() and
 *    CfyE_Statistics_Summary_Channel_Read() functions.
 *
 * 3. Use CfyE_vPort_Add() function to add a vPort Policy to the Device. This
 *    function will provide a unique handle for the added vPort which can be
 *    used for invoking the CfyE_vPort_Remove() and/or CfyE_vPort_Update()
 *    functions for this vPort.
 *
 * 4. Use CfyE_Rule_Add() or CfyE_Rule_Add_Index() function to add a Rule to
 *    the vPort Policy. Multiple packet matching rules can be added to the
 *    same vPort.
 *
 * 5. A Rule can be updated with the CfyE_Rule_Update() function. In order
 *    to enable the Rule and start packet matching process at the Device
 *    the CfyE_Rule_Enable() or CfyE_Rule_EnableDisable() function should
 *    be used. For devices with an external TCAM, rules are enabled
 *    when created, they can be disabled but they cannot be re-enabled later.
 *
 * 6. Use CfyE_Notify_Request() function to enable asynchronous CfyE
 *    notifications for specified global an/or channel-specific events.
 *
 * 7. The MACsec Engine SecY API should be used to initialize and configure
 *    the MACsec packet processing.
 *
 * 8. Now the Device is armed to match packets to the added vPorts using the
 *    added Rules and classification settings. A packet I/O API should be used
 *    to submit packets to the Classification Engine and read processed packet
 *    from it.
 *
 * 9. Use CfyE_Statistics_*() functions to read Device statistics about
 *    packet classified results.
 *
 * 10. Disable packet matching Rules with CfyE_Rule_Disable() or
 *     CfyE_Rule_EnableDisable() function using the Rule handle obtained from
 *     CfyE_Rule_Add().
 *
 * 11. Use CfyE_Rule_Remove() function to remove the disabled rules.
 *
 * 12. Use CfyE_vPort_Update() function to remove the vPort Policy for
 *     the removed Rules. Steps 10-12 must be repeated for all the added vPorts
 *     and Rules.
 *
 * 13. Un-initialize the Device with CfyE_Device_Uninit().
 *
 */


/* -------------------------------------------------------------------------- */
/*                                                                            */
/*   Module        : ddk164                                                   */
/*   Version       : 3.1.1                                                    */
/*   Configuration : DDK-164-IPSEC-CREDO-BSD                                  */
/*                                                                            */
/*   Date          : 2021-Aug-12                                              */
/*                                                                            */
/* Copyright (c) 2008-2021 by Rambus, Inc. and/or its subsidiaries.           */
/*                                                                            */
/* Redistribution and use in source and binary forms, with or without         */
/* modification, are permitted provided that the following conditions are     */
/* met:                                                                       */
/*                                                                            */
/* 1. Redistributions of source code must retain the above copyright          */
/* notice, this list of conditions and the following disclaimer.              */
/*                                                                            */
/* 2. Redistributions in binary form must reproduce the above copyright       */
/* notice, this list of conditions and the following disclaimer in the        */
/* documentation and/or other materials provided with the distribution.       */
/*                                                                            */
/* 3. Neither the name of the copyright holder nor the names of its           */
/* contributors may be used to endorse or promote products derived from       */
/* this software without specific prior written permission.                   */
/*                                                                            */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS        */
/* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT          */
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR      */
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT       */
/* HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,     */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT           */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,      */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY      */
/* THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT        */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE      */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.       */
/* -------------------------------------------------------------------------- */

#ifndef API_CFYE_H_
#define API_CFYE_H_

/*----------------------------------------------------------------------------
 * This module uses (requires) the following interface(s):
 */

/* Driver Framework Basic Definitions API */
#include "basic_defs.h"             /* uint32_t */



/*----------------------------------------------------------------------------
 * Definitions and macros
 */

/** Counter increment disable control values\n
    Each bit disables all the counters of one counter type.
    Counter types:
        - TCAM counters
        - Channel counters */
/** TCAM Counter increment disable bit for */
#define CFYE_TCAM_COUNT_INC_DIS                 BIT_0

/** Channel Counter increment disable bit for */
#define CFYE_CHAN_COUNT_INC_DIS                 BIT_1

/** Number of used non-control key/mask words */
#define CFYE_RULE_NON_CTRL_WORD_COUNT           5

/** Number of ethertype values to use for comparison for MPLS packets */
#define CFYE_MPLS_ETYPE_MAX_COUNT               4

/** Maximum number of VLAN user priority values */
#define CFYE_VLAN_UP_MAX_COUNT                  8

/** Maximum number of control packet matching rules
 *  using MAC destination address and EtherType */
#define CFYE_MAC_DA_ET_MATCH_RULES_COUNT        8

/** Maximum number of control packet matching rules
 *  using EtherType and range of MAC destination addresses */
#define CFYE_MAC_DA_ET_RANGE_MATCH_RULES_COUNT  2

/*----------------------------------------------------------------------------
 * Device global events to use with the notifications
 */

/** Statistics threshold event from TCAM Statistics Module */
#define CFYE_EVENT_STAT_TCAM_THR                1
/** Statistics threshold event from Channel Statistics Module */
#define CFYE_EVENT_STAT_CHAN_THR                2
/** ECC error event */
#define CFYE_EVENT_ECC_ERR                      256

/** Statistics threshold event from MTT Statistics Module */
#define CFYE_EVENT_STAT_MTT_THR                  512

/*----------------------------------------------------------------------------
 * Device channel-specific events to use with the notifications
 */
/* Input interface events: */
/** Start-Of-Packet (SOP) is received for a busy channel event */
#define CFYE_EVENT_CHAN_ERR_SOP_WO_EOP          1
/** End-Of-Packet (EOP) is received for an idle channel event */
#define CFYE_EVENT_CHAN_ERR_EOP_WO_SOP          2
/** Data is received for an idle channel event */
#define CFYE_EVENT_CHAN_ERR_XFER_WO_SOP         4
/** SOP is received in the 2nd cycle of the slot event */
#define CFYE_EVENT_CHAN_ERR_SLOT_SOP            8
/** Channel ID is changed in the 2nd cycle of the slot event */
#define CFYE_EVENT_CHAN_ERR_SLOT_CHID           16
/** First 2 packet words are not received back-to-back in the first slot event */
#define CFYE_EVENT_CHAN_ERR_NOT_B2B             32

/* Other channel-specific events: */
/** Packet classified as a control packet event */
#define CFYE_EVENT_CHAN_CTRL_PACKET             64
/** Packet classified as a data packet event */
#define CFYE_EVENT_CHAN_DATA_PACKET             128
/** Packet dropped as a result of classification event */
#define CFYE_EVENT_CHAN_DROP_PACKET             256
/** Multiple TCAM rule hit for a packet event */
#define CFYE_EVENT_CHAN_TCAM_HIT_MULT           512
/** TCAM rule not hit for a packet event */
#define CFYE_EVENT_CHAN_TCAM_MISS               1024
/** Data is received on the input interface in the 2nd cycle of a slot,
    while the first cycle had an EOP already */
#define CFYE_EVENT_CHAN_ERR_SLOT_2PKTS          2048

/** MTT rule not hit for IPSec lookup */
#define CFYE_EVENT_CHAN_MTT_MISS                4096

/** TCAM hit with non-IPSec packet */
#define CFYE_EVENT_CHAN_TCAM_HIT_NON_CS         8192

/** Status values returned by API functions  */
enum
{
    CFYE_STATUS_OK,             /**< \n */
    CFYE_ERROR_BAD_PARAMETER,   /**< \n */
    CFYE_ERROR_INTERNAL,        /**< \n */
    CFYE_ERROR_NOT_IMPLEMENTED,  /**< \n */
};

/** Return type for CfyE API functions. above enum values and any other
    integer values for device read/write errors */
typedef int CfyE_Status_t;

/** Classification device role: either Egress only, Ingress only */
typedef enum
{
    CFYE_ROLE_EGRESS,  /**< \n */
    CFYE_ROLE_INGRESS  /**< \n */
} CfyE_Role_t;

/** Classification device Operation mode */
typedef enum
{
    CFYE_MODE_MACSEC,  /**< \n */
    CFYE_MODE_IPSEC,   /**< \n */
    CFYE_MODE_INVALID  /**< \n */
} CfyE_Channel_Mode_t;

/** Drop type  */
typedef enum
{
    CFYE_DROP_CRC_ERROR,  /**< bypass with CRC error */
    CFYE_DROP_PKT_ERROR,  /**< bypass with packet error */
    CFYE_DROP_INTERNAL,   /**< internal drop by external logic */
    CFYE_DO_NOT_DROP      /**< do not drop, for debug purposes only */
} CfyE_DropType_t;

/** Statistics counter  */
typedef struct
{
    uint32_t Lo; /**< Low 32-bit counter word */
    uint32_t Hi; /**< High 32-bit counter word */
} CfyE_Stat_Counter_t;

/**----------------------------------------------------------------------------
 * CfyE_RuleHandle_t
 *
 * This handle is a reference to a vPort. The handle is NULL when
 * it is equal to CfyE_RuleHandle_NULL.
 */
typedef void * CfyE_RuleHandle_t;
/** external declared variable: CfyE_RuleHandle_t CfyE_RuleHandle_NULL */
extern const CfyE_RuleHandle_t CfyE_RuleHandle_NULL;

/**----------------------------------------------------------------------------
 * CfyE_vPortHandle_t
 *
 * This handle is a reference to a vPort. The handle is NULL when
 * it is equal to CfyE_vPortHandle_NULL.
 */
typedef void * CfyE_vPortHandle_t;

/**----------------------------------------------------------------------------
 * @var CfyE_vPortHandle_NULL
 * This handle can be assigned to a variable of type CfyE_vPortHandle_t.
 */
extern const CfyE_vPortHandle_t CfyE_vPortHandle_NULL;

/** Each bit in the Channel bit mask represents a Channel. Each word represents
 * a maximum of 32 Channels
 */
#define CFYE_CHANNEL_WORDS 2

/** Channel Mask */
typedef struct {
    /** Each bit in the Channel bit mask represents a channel supported by the
     *  CfyE device
     */
    uint32_t ch_bitmask[CFYE_CHANNEL_WORDS];
} CfyE_Ch_Mask_t;

/** Egress header control */
typedef struct
{
    /** Enable flag */
    bool fEnable;

    /** Egress header type */
    uint16_t EgressHeaderEtype;
} CfyE_EgressHeader_t;

/** MACsec SecTAG parser control */
typedef struct
{
    /** true - compare Ether-type in packet against MACsecTagValue value */
    bool fCompType;

    /** true - check V (Version) bit to be 0 */
    bool fCheckVersion;

    /** true - check if packet is meant to be handled by KaY (C & E bits) */
    bool fCheckKay;

    /** This flag controls key generation for packets with SecTAG after MAC SA. \n
     *  true - enable generation of rule key with packet type MACsec (TCI, SCI) \n
     *  false - always generate normal rule keys \n
     */
    bool fLookupUseSCI;

    /** Ether-type value used for MACsec tag type comparison */
    uint16_t MACsecTagValue;
} CfyE_SecTAG_Parser_t;

/** MPLS label select for packets with 3 labels */
typedef enum
{
    CFYE_MPLS3_SELECT_LABEL1 = 0,  /**< \n */
    CFYE_MPLS3_SELECT_LABEL2,      /**< \n */
    CFYE_MPLS3_SELECT_LABEL3       /**< \n */
} CfyE_MPLS3_Select_t;

/** MPLS label select for packets with 4 labels */
typedef enum
{
    CFYE_MPLS4_SELECT_LABEL1 = 0,  /**< \n */
    CFYE_MPLS4_SELECT_LABEL2,      /**< \n */
    CFYE_MPLS4_SELECT_LABEL3,      /**< \n */
    CFYE_MPLS4_SELECT_LABEL4       /**< \n */
} CfyE_MPLS4_Select_t;

/** MPLS label select for packets with >= 5 labels */
typedef enum
{
    CFYE_MPLS5_SELECT_LABEL1 = 0,  /**< \n */
    CFYE_MPLS5_SELECT_LABEL2,      /**< \n */
    CFYE_MPLS5_SELECT_LABEL3,      /**< \n */
    CFYE_MPLS5_SELECT_LABEL4,      /**< \n */
    CFYE_MPLS5_SELECT_LABEL5       /**< \n */
} CfyE_MPLS5_Select_t;


/** MPLS ethertype compare value */
typedef struct
{
    /** true - use MPLS_Etype value for comparison with ethertype filed
     *         in an MPLS packet
     */
    bool fCompare;

    /** Value of ethertype field to compare with ethertype in an MPLS packet */
    uint16_t MPLS_Etype;
} CfyE_MPLS_Etype_t;

/** MPLS parsing control */
typedef struct
{
    /** MPLS packet with 3 labels: select 2 labels to use in TCAM key\n */
    CfyE_MPLS3_Select_t MPLS3_Select1; /**< 1st label select */
    CfyE_MPLS3_Select_t MPLS3_Select2; /**< 2nd label select */

    /** MPLS packet with 4 labels: select 2 labels to use in TCAM key\n */
    CfyE_MPLS4_Select_t MPLS4_Select1; /**< 1st label select */
    CfyE_MPLS4_Select_t MPLS4_Select2; /**< 2nd label select */

    /** MPLS packet with 5 labels: select 2 labels to use in TCAM key\n */
    CfyE_MPLS5_Select_t MPLS5_Select1; /**< 1st label select */
    CfyE_MPLS5_Select_t MPLS5_Select2; /**< 2nd label select */

    /** Ethertype values (max CFYE_MPLS_ETYPE_MAX_COUNT) to use for
     * comparison with ethertype field in MPLS packets
     */
    CfyE_MPLS_Etype_t MPLS_Etype[CFYE_MPLS_ETYPE_MAX_COUNT];
} CfyE_MPLS_Parser_t;


/** VLAN tags parsing control */
typedef struct
{
    /** Enable detection of VLAN tags matching QTag value */
    bool fParseQTag;

    /** Enable detection of VLAN tags matching STag1 value */
    bool fParseStag1;

    /** Enable detection of VLAN tags matching STag2 value */
    bool fParseStag2;

    /** Enable detection of VLAN tags matching STag3 value */
    bool fParseStag3;

    /** Enable detection of multiple back-2-back VLAN tags (Q-in-Q and beyond) */
    bool fParseQinQ;

} CfyE_VLAN_ParseTag_t;

/** VLAN parsing control
 *  The classification engine is able to detect up to five VLAN tags following
 *  the outer MAC_SA and parse the first two for packet classification
 */
typedef struct
{
    /* 1. Per-channel VLAN parsing settings */

    /** Input header parser VLAN tags parsing settings */
    CfyE_VLAN_ParseTag_t CP;

    /* 2. Global VLAN parsing settings */

    /** Enable user priority processing for 802.1s packets. If set,
     *  when a 802.1s tag is found, take the user priority directly from
     *  the PCP field, otherwise take the default user priority.
     */
    bool fSTagUpEnable;

    /** Enable user priority processing for 802.1Q packets. If set,
     *  when a 802.1Q tag is found, translate the PCP field using UpTable1,
     *  otherwise take the default user priority.
     */
    bool fQTagUpEnable;

    /** Default user priority, assigned to non-VLAN packets and
     *  to VLAN packets for which the VLAN user priority processing is disabled
     */
    uint8_t DefaultUp; /**< Allowed values in range 0 .. 7! */

    /** Translation tables to derive the user priority from the PCP field
     *  in 802.1Q  tags. If the PCP field is 0 then take UpTable[0] as the user
     *  priority, if PCP=1 then take UpTable[1], etc.
     */

    /** Translation table for 1st 802.1Q  tag, allowed values in range 0 .. 7! */
    uint8_t UpTable1[CFYE_VLAN_UP_MAX_COUNT];

    /** Translation table for 2nd 802.1Q  tag, allowed values in range 0 .. 7! */
    uint8_t UpTable2[CFYE_VLAN_UP_MAX_COUNT];

    /** Ethertype value used for 802.1Q  tag type comparison */
    uint16_t QTag;

    /** Ethertype value used for 802.1s  tag 1 type comparison */
    uint16_t STag1;
    /** Ethertype value used for 802.1s  tag 2 type comparison */
    uint16_t STag2;
    /** Ethertype value used for 802.1s  tag 3 type comparison */
    uint16_t STag3;
} CfyE_VLAN_Parser_t;


/** IPsec specific parser configuration */
typedef struct
{
    /** Enable outer IP header parsing */
    bool fParseIP;

    /** Enable UDP parsing after outer IP header. */
    bool fParseUDP;

    /** Enable parsing of UDP encapsulated packets */
    bool fParseNAT;

    /** Enable detection of ESP header */
    bool fParseESP;

    /** Enable detection and bypassing of IKE v1/v2 header */
    bool fParseIKE;

    /** Enable detection and bypassing of NAT-T IKE. */
    bool fParseNATIKE;

    /** Enable detection and bypassing of NAT Keepalive. */
    bool fParseNATKeepAlive;

    /** Enable detection and bypassing of UDP headers that have non-zero checksum */
    bool fVerifyUDPChkSum; /** Ingress-only field */

    /** Ingress-only field: Enable MAC DA check for IPsec processing */
    bool fMACDACheck;

    /** Ingress-only field: allow fragments */
    bool fAllowFragments;

    /** Ingress-only field: ignore IPv5 Checksum */
    bool fIgnoreIPv4ChkSum;

    /** Ingress-only field: mark IKE packets as sub-control packets */
    bool fSubParseIKE;

    /** Ingress-only field: mark NAT-T IKE packets as sub-control packets */
    bool fSubParseNATIKE;

    /** Ingress-only field: mark NAT-T Keep Alive  packets as sub-control
        packets */
    bool fSubParseNATKeepAlive;

    /** Global Controls */

    /** Default IKE port */
    uint16_t IKE_Port;

    /** Default NAT port */
    uint16_t NAT_Port;

    /** MAC destination address, 6 bytes */
    uint8_t MAC_DA[6];
} CfyE_IPSEC_Parser_t;

/** Header Parser control */
typedef struct
{
    /** Egress header settings. Set to NULL if no update is required */
    CfyE_EgressHeader_t *EgressHeader_p;

    /** SecTAG parser settings. Set to NULL if no update is required */
    CfyE_SecTAG_Parser_t *SecTAG_Parser_p;

    /** MPLS parser settings. Set to NULL if no update is required */
    CfyE_MPLS_Parser_t *MPLS_Parser_p;

    /** VLAN parser settings. Set to NULL if no update is required */
    CfyE_VLAN_Parser_t *VLAN_Parser_p;

    /** IPSec parser settings, set to NULL of no update is required.*/
    CfyE_IPSEC_Parser_t *IPsec_Parser_p;
} CfyE_HeaderParser_t;

/** Packet match rule using MAC destination address and EtherType */
typedef struct
{
    /** MAC destination address, 6 bytes */
    uint8_t * MAC_DA_p; /**< Set to NULL if not used */

    /** EtherType */
    uint16_t EtherType;

} CfyE_MAC_DA_ET_MatchRule_t;

/** Packet match rule using range of MAC destination addresses */
typedef struct
{
    /** Start MAC destination address, 6 bytes\n */
    uint8_t * MAC_DA_Start_p; /**< Set to NULL if not used */

    /** End MAC destination address, 6 bytes\n */
    uint8_t * MAC_DA_End_p;   /**< Only applicable if MAC_DA_Start_p is not NULL */

} CfyE_MAC_DA_Range_MatchRule_t;

/** Packet match rule using range of MAC destination addresses and EtherType */
typedef struct
{
    /** Range of MAC destination addresses, start and end */
    CfyE_MAC_DA_Range_MatchRule_t Range;

    /** EtherType */
    uint16_t EtherType;

} CfyE_MAC_DA_ET_Range_MatchRule_t;

/** Control Packet Detector settings */
typedef struct
{
    /** MAC destination address and EtherType
     *  Set MAC_DA_ET_Rules[n].MAC_DA_p to NULL if no update is required,
     *  corresponding bits in CPMatchEnableMask and SCPMatchEnableMask should
     *  be set to 0
     */
    CfyE_MAC_DA_ET_MatchRule_t MAC_DA_ET_Rules[CFYE_MAC_DA_ET_MATCH_RULES_COUNT];

    /** Range of MAC destination address used with EtherType
     *  Set MAC_DA_ET_Range[n].Range.MAC_DA_Start_p and
     *  MAC_DA_ET_Range[n].Range.MAC_DA_End_p to NULL if no update is required,
     *  corresponding bits in CPMatchEnableMask and SCPMatchEnableMask should
     *  be set to 0
     */
    CfyE_MAC_DA_ET_Range_MatchRule_t MAC_DA_ET_Range[CFYE_MAC_DA_ET_RANGE_MATCH_RULES_COUNT];

    /** Range of MAC destination address (no EtherType)
     *  Set MAC_DA_Range[n].MAC_DA_Start_p and MAC_DA_Range[n].MAC_DA_End_p to NULL
     *  if no update is required, corresponding bits in CPMatchEnableMask and
     *  SCPMatchEnableMask should be set to 0
     */
    CfyE_MAC_DA_Range_MatchRule_t MAC_DA_Range;

    /** 44-bit MAC destination address constant, 6 bytes
     *  Set to NULL if no updated is required, corresponding bits in
     *  CPMatchEnableMask and SCPMatchEnableMask should be set to 0
     */
    uint8_t * MAC_DA_44Bit_Const_p;

    /** 48-bit MAC destination address constant, 6 bytes\n
     *  Set to NULL if no updated is required\n
     *  Set to NULL if no updated is required, corresponding bits in
     *  CPMatchEnableMask and SCPMatchEnableMask should be set to 0
     */
    uint8_t * MAC_DA_48Bit_Const_p;

    /** Match mask used to select which EtherType to compare with the EtherType
     *  value in the matching rule\n
     *  bit i (0-7): 1 enables packet MAC destination address comparison with
     *               the respective MAC_DA_ET_Rules[i].EtherType address\n
     *  bit j (8-9): 1 enables packet MAC destination address comparison with
     *               the respective MAC_DA_ET_Range[j-8].EtherType address\n */
    /** Primary Control Packet Detector */
    uint32_t CPMatchModeMask;

    /** Match mask which can be used to enable/disable control packet
     *  matching rules\n
     *  bit i (0-7): 1 enables packet MAC destination address comparison with
     *               the respective MAC_DA_ET_Rules[i].MAC_DA_p address\n
     *  bit j (8-15): 1 enables packet Ethernet Type value comparison with
     *                the respective MAC_DA_ET_Rules[j-8].EtherType value\n
     *  bit 16: 1 enables range of packet MAC destination address as well as
     *          Ethernet Type value comparison with the respective
     *          MAC_DA_ET_Range[0].Range.MAC_DA_Start_p and
     *          MAC_DA_ET_Range[0].Range.MAC_DA_End_p values as well as
     *          MAC_DA_ET_Range[0].EtherType value\n
     *  bit 17: 1 enables range of packet MAC destination address as well as
     *          Ethernet Type value comparison with the respective
     *          MAC_DA_ET_Range[1].Range.MAC_DA_Start_p and
     *          MAC_DA_ET_Range[1].Range.MAC_DA_End_p values as well as
     *          MAC_DA_ET_Range[1].EtherType value\n
     *  bit 18: 1 enables range of packet MAC destination addresses comparison
     *          with MAC_DA_Range.MAC_DA_Start_p and MAC_DA_Range.MAC_DA_End_p\n
     *  bit 19: 1 enables packet MAC destination addresses comparison with
     *          MAC_DA_44Bit_Const_p constant\n
     *  bit 20: 1 enables packet MAC destination addresses comparison with
     *          MAC_DA_48Bit_Const_p constant\n
     *  bit 31: 1 enables MACsec KaY packets as control packets\n */
    /** Primary Control Packet Detector */
    uint32_t CPMatchEnableMask;

    /* Match mask which can be used to mark control packets with the */
    /* Sub Control Packet signal, for each matching rule. */
    /* bit i (0-7): 1 enables packet MAC destination address comparison with */
    /*              the respective MAC_DA_ET_Rules[i].MAC_DA_p address */
    /* bit j (8-15): 1 enables packet Ethernet Type value comparison with */
    /*               the respective MAC_DA_ET_Rules[j-8].EtherType value */
    /* bit 16: 1 enables range of packet MAC destination address as well as */
    /*         Ethernet Type value comparison with the respective */
    /*         MAC_DA_ET_Range[0].Range.MAC_DA_Start_p and */
    /*         MAC_DA_ET_Range[0].Range.MAC_DA_End_p values as well as */
    /*         MAC_DA_ET_Range[0].EtherType value */
    /* bit 17: 1 enables range of packet MAC destination address as well as */
    /*         Ethernet Type value comparison with the respective */
    /*         MAC_DA_ET_Range[1].Range.MAC_DA_Start_p and */
    /*         MAC_DA_ET_Range[1].Range.MAC_DA_End_p values as well as */
    /*         MAC_DA_ET_Range[1].EtherType value */
    /* bit 18: 1 enables range of packet MAC destination addresses comparison */
    /*         with MAC_DA_Range.MAC_DA_Start_p and MAC_DA_Range.MAC_DA_End_p */
    /* bit 19: 1 enables packet MAC destination addresses comparison with */
    /*         MAC_DA_44Bit_Const_p constant */
    /* bit 20: 1 enables packet MAC destination addresses comparison with */
    /*         MAC_DA_48Bit_Const_p constant */
    /* bit 31: 1 enables MACsec KaY packets as control packets */
    /** Primary Control Packet Detector */
    uint32_t CPMatchSubMask;
} CfyE_ControlPacket_t;

/** Exception cases control */
typedef struct
{
    /** Defines the way the drop operation is performed if the vPort is missed */
    CfyE_DropType_t DropAction;

    /** Facilitates channel reset sequence and allows in single write drop all
     *  incoming packets for a channel\n
     *  true  - force dropping for all packets for a given channel\n
     *  false - do not force dropping */
    bool fForceDrop;

    /** Allows passing packets that missed the vPort through, for this DropAction
     *  must not be set to CFYE_DROP_INTERNAL\n
     *  true -  allow assigning a default vPort for packet that missed vPort
     *          matching\n
     *  false - do not change the results of the vPort matching */
    bool fDefaultVPortValid;

    /** Default vPort value, ignored if fDefaultVPortValid is set to false */
    uint16_t DefaultVPort;

    /** Defines the way the drop operation is performed if an uncorrectable
        ECC error occurs */
    CfyE_DropType_t ECCDropAction;

    /** Enable should-secure policy on this channel */
    bool fShouldSecure;
} CfyE_Device_Exceptions_t;

/** Device bypass control */
typedef struct
{
    /** Indication if the low latency bypass must be enabled (true)
     *  or the MACsec mode (false).
     */
    bool fLowLatencyBypass;

    /** true - if IPsec mode is supported.*/
    bool fIPsec;

    /** true - Spare register and interrupt controller are present */
    bool fSparseReg;

    /** Exception cases settings. Set to NULL if no update is required */
    CfyE_Device_Exceptions_t *Exceptions_p;
} CfyE_Device_Control_t;

/** Device statistics control */
typedef struct
{
    /** true - statistics counters are automatically reset on a read operation\n
     *  false - no reset on a read operation
     */
    bool fAutoStatCntrsReset;

    /** Threshold for the frame counters */
    CfyE_Stat_Counter_t CountFrameThr;

    /** Threshold for the per-channel frame counters */
    CfyE_Stat_Counter_t ChanCountFrameThr;

    /** Threshold for the per-MTT frame counters */
    CfyE_Stat_Counter_t MTTCountFrameThr;

    /** Counter increment enable control */
    uint8_t CountIncDisCtrl;
} CfyE_Statistics_Control_t;

/** Statistics counter per TCAM entry */
typedef struct
{
    /** Statistics counter */
    CfyE_Stat_Counter_t Counter;
} CfyE_Statistics_TCAM_t;

/** Per-channel statistics counters */
typedef struct
{
    /** Number of packets that hit multiple TCAM entries */
    CfyE_Stat_Counter_t TCAMHitMultiple;
    /** Number of packets that were dropped by header parser */
    CfyE_Stat_Counter_t HeaderParserDroppedPkts;
    /** Number of packets that missed TCAM lookup */
    CfyE_Stat_Counter_t TCAMMiss;
    /** Number of control packets */
    CfyE_Stat_Counter_t PktsCtrl;
    /** Number of data packets */
    CfyE_Stat_Counter_t PktsData;
    /** Number of packets that were dropped*/
    CfyE_Stat_Counter_t PktsDropped;
    /** Number of packets marked as error packets in input  */
    CfyE_Stat_Counter_t PktsErrIn;

    /** Number of MTT Misses - Ingress Only */
    CfyE_Stat_Counter_t MTTMiss;
} CfyE_Statistics_Channel_t ;

/** Statistics counter per MTT entry */
typedef struct
{
    /** Statistics counter */
    CfyE_Stat_Counter_t Counter;
} CfyE_Statistics_MTT_t;

/** Control of automatic EOP insertion. */
typedef struct
{
    /** Timeout value */
    unsigned int EOPTimeoutVal;

    /** bit mask to specify channels. */
    CfyE_Ch_Mask_t EOPTimeoutCtrl;
} CfyE_EOPConf_t;

/** ECC configuration: configuration parameters */
typedef struct
{
    /** Correctable error threshold. */
    unsigned int ECCCorrectableThr;

    /** Uncorrectable error threshold. */
    unsigned int ECCUncorrectableThr;
} CfyE_ECCConf_t;

/** Device control */
typedef struct
{
    /** Bypass settings. Set to NULL if no update is required */
    CfyE_Device_Control_t * Control_p;

    /** Control packet detection settings. Set to NULL if no update is required */
    CfyE_ControlPacket_t * CP_p;

    /** Header parser settings. Set to NULL if no update is required */
    CfyE_HeaderParser_t * HeaderParser_p;

    /** Statistics settings. Set to NULL if no update is required */
    CfyE_Statistics_Control_t * StatControl_p;

    /** automatic EOP insertion, Set to NULL if no update is required */
    CfyE_EOPConf_t * EOPConf_p;

    /** ECC threshold settings. Set to NULL of no update is required.*/
    CfyE_ECCConf_t * ECCConf_p;
} CfyE_Device_t;

/** Classification device limits */
typedef struct
{
    /** Major HW version */
    uint8_t major_version;
    /** Minor HW version */
    uint8_t minor_version;
    /** HW patch level */
    uint8_t patch_level;

    /** Number of channels */
    unsigned int channel_count;
    /** Number of vPorts */
    unsigned int vport_count;
    /** Number of rules */
    unsigned int rule_count;
    /** Number of MTT entriess */
    unsigned int mtt_count;
} CfyE_Device_Limits_t;

/** Classification device initialization settings */
typedef struct
{
    /** true - initialize all channels to pass packets in low-latency bypass mode\n
     *  false - initialize all channels for classification mode
     */
    bool fLowLatencyBypass;

    /** Threshold for the frame counters low-32bits */
    uint32_t CountFrameThrLo;
    /** Threshold for the frame counters high-32bits */
    uint32_t CountFrameThrHi;

    /** Threshold for the per-channel frame counters low-32bits */
    uint32_t ChanCountFrameThrLo;

    /** Threshold for the per-channel frame counters high-32bits */
    uint32_t ChanCountFrameThrHi;

    /** Threshold for the per-MTT frame counters low-32bits */
    uint32_t MTTCountFrameThrLo;

    /** Threshold for the per-MTT frame counters high-32bits */
    uint32_t MTTCountFrameThrHi;

    /** Control of automatic EOP insertion, timeout value */
    unsigned int EOPTimeoutVal;

    /** Control of automatic EOP insertion, bit mask to specify channels */
    CfyE_Ch_Mask_t EOPTimeoutCtrl;

    /** Threshold for the ECC correctable error counters */
    unsigned int ECCCorrectableThr; /* Correctable errors. */
    /** Threshold for the ECC uncorrectable error counters */
    unsigned int ECCUncorrectableThr; /* Uncorrectable errors */

    /** Counter increment enable control */
    uint8_t CountIncDisCtrl;

    /** Pointer to memory location where an array of 32-bit words
     *  is stored for the Input TCAM database of the CfyE device.
     *  This array will be copied to the Input TCAM memory of the CfyE device
     *  during its initialization.\n
     *  Set to NULL in order to retain to the default (or old) data
     *  in the Input TCAM memory. */
    uint32_t * InputTCAM_p;

    /** Size of the InputTCAM_p array in 32-bit words */
    unsigned int InputTCAM_WordCount;

    /** 32-bit word offset in the Input TCAM memory where
     *  the InputTCAM_p array must be written */
    unsigned int WordOffset;

    /** Drop control packets when they do not match any rule. */
    bool fDropControlPkts;

    /** Specify the maximum number of channels to be used by the driver.
     *  If set to zero, use the maximum supported by the hardware */
    unsigned int MaxChannelCount;

    /** Specify the maximum number of vPorts to be used by the driver.
     *  If set to zero, use the maximum supported by the hardware */
    unsigned int MaxvPortCount;

    /** Specify the maximum number of rules to be used by the driver.
     *  If set to zero, use the maximum supported by the hardware */
    unsigned int MaxRuleCount;
    /** Ingree-only Flag:
     * 1b : Packet Type in TCAM key is set to different values for MPLS IPsec, UDP encapsulated IPsec and IPsec
     * 0b : packet_type in TCAM key is set to same value for MPLS IPsec, UDP encapsulated IPsec and IPsec
     */
    bool fParsePktIPsec;
} CfyE_Init_t;


/** Debug registers for packet classification. These can be read to
    find out how the last packet was classified. For details see the
    corresponding register definitions in Operations and Programmer's
    Guide.*/
typedef struct
{
    /** Control packet matching status */
    uint32_t CPMatchDebug;
    /** TCAM matching status */
    uint32_t TCAMDebug;
    /** SecTAG parser status */
    uint32_t SecTAGDebug;
    /** Various VLAN parser results */
    uint32_t SAMPPDebug1;
    /** Various VLAN parser results */
    uint32_t SAMPPDebug2;
    /** Various VLAN parser results */
    uint32_t SAMPPDebug3;
    /** Parsed destination address low */
    uint32_t ParsedDALo;
    /** Parsed destination address high */
    uint32_t ParsedDAHi;
    /** Parsed source address low */
    uint32_t ParsedSALo;
    /** Parsed source address high + ether type */
    uint32_t ParsedSAHi;
    /** Parsed SecTAG low */
    uint32_t ParsedSecTAGLo;
    /** Parsed SecTAG high */
    uint32_t ParsedSecTAGHi;
    /** TCAM lookup result (vPort policy) */
    uint32_t DebugFlowLookup;
} CfyE_PktProcessDebug_t;


/** Number of ECC status counters. */
#define CFYE_ECC_NOF_STATUS_COUNTERS 13

/** ECC error status. */
typedef struct
{
    /** Array of status records per status counter */
    struct {
        /** Number of correctable errors. */
        unsigned int CorrectableCount;

        /** Number of uncorrectable errors.*/
        unsigned int UncorrectableCount;

        /** Threshold for correctable errors exceeded */
        bool fCorrectableThr;

        /** Threshold for uncorrectable errors exceeded */
        bool fUncorrectableThr;
    } Counters[CFYE_ECC_NOF_STATUS_COUNTERS];
} CfyE_ECCStatus_t;

/** Overall device status */
typedef struct
{
    /** may be NULL if this information is not desired.*/
    bool *fExternalTCAM_p;
    /** may be NULL if parser debug info is not desired.*/
    CfyE_PktProcessDebug_t *PktProcessDebug_p;
    /** May be NULL if ECC status is not desired. */
    CfyE_ECCStatus_t *ECCStatus_p;
} CfyE_DeviceStatus_t;

/** vPort policy data structure */
typedef struct
{
    /** SecTag offset\n
     * IPsec Mode: For ingress and egress write zeroes
     */
    uint8_t SecTagOffset;

    /** MPLS header options (ingress-only).*/
    /** 00b = Reserved */
    /** 01b = Retain full header */
    /** 10b = Remove full header */
    /** 11b = Remove all labels except of last label.*/
    uint8_t MPLS_Hdr;

    /** Packet expansion indication\n
     * 00b = 0 bytes (no expansion)
     * 01b = IPsec expansion
     * If the packet is classified as a 'control' packet, this field is ignored
     * and no expansion is indicated on the output interface.
     *
     * Note: For egress only, not used for ingress configurations.
     */
    uint8_t PktExtension;
} CfyE_vPort_t;

/** Packet types */
typedef enum
{
    CFYE_RULE_PKT_TYPE_OTHER  = 0,  /**< untagged, VLAN, etc */
    CFYE_RULE_PKT_TYPE_MPLS   = 9,  /**< \n */
    CFYE_RULE_PKT_TYPE_MACSEC = 3,  /**< Packet Type MACsec */
    CFYE_RULE_PKT_TYPE_IPSEC_MPLS  = 1,  /**< Packet Type IPsec wtih MPLS  */
    CFYE_RULE_PKT_TYPE_IPSEC_UDP  = 2,  /**< Packet Type IPsec wiht UDP  */
    CFYE_RULE_PKT_TYPE_IPSEC  = 3,  /**< Packet Type IPsec */
} CfyE_Rule_PacketType_t;


#define CFYE_RULE_PKT_TYPE_MASK 0x7 /**< Packet type mask in TCAM rule */
#define CFYE_RULE_NUMTAGS_MASK 0x7f /**< NumTags field mask in TCAM rule */
#define CFYE_RULE_CHANNEL_ID_MASK 0x3f /**< Channel ID field mask in TCAM rule */

/** vPort matching rule Key/Mask data structure */
typedef struct
{
    /** Packet type */
    CfyE_Rule_PacketType_t PacketType;

    /** If PacketType = CFYE_RULE_PKT_TYPE_MACSEC then\n
     *     Bits[5:0] = SecTAG TCI (if supported by configuration)
     *
     * If PacketType = CFYE_RULE_PKT_TYPE_MPLS or\n
     *    PacketType = CFYE_RULE_PKT_TYPE_OTHER then\n
     *     Bit 0 = 1 : No VLAN tags and no MPLS labels\n
     *     Bit 1 = 1 : 1 VLAN tag or 1 MPLS label\n
     *     Bit 2 = 1 : 2 VLAN tags or 2 MPLS labels\n
     *     Bit 3 = 1 : 3 VLAN tags or 3 MPLS labels\n
     *     Bit 4 = 1 : 4 VLAN tags or 4 MPLS labels\n
     *     Bit 5 = 1 : 5 MPLS labels\n
     *     Bit 6 = 1 : >4 VLAN tags or >5 MPLS labels
     */
    uint8_t NumTags; /**< \n bit mask, only 7 bits[6:0] are used, see above how */

    /** Channel ID */
    uint16_t ChannelID;
} CfyE_Rule_KeyMask_t;

/** vPort matching rule policy */
typedef struct
{
    /** vPort handle obtained via CfyE_vPort_Add() function */
    CfyE_vPortHandle_t vPortHandle;

    /** Priority value that is used to resolve multiple rule matches.
     *  When multiple rules are hit by a packet simultaneously, the rule with
     *  the higher priority value will be returned. If multiple rules with
     *  an identical priority value are hit, the rule with the lowest rule index
     *  is used.
     *
     *  @note  devices with an external TCAM will not honor the Priority field.
     */
    uint8_t Priority;

    /** vPort to SA association number */
    uint8_t AN;

    /** true : drop the packet */
    bool fDrop;

    /** true : Mark packets matching the rule as control packets. */
    bool fControlPacket;

    /** true: mark packets  matching the rule as subset control packets */
    bool fControlPacketSub;
} CfyE_Rule_Policy_t;

/** vPort matching rule data structure */
typedef struct
{
    /** Sets matching values as specified in CfyE_Rule_KeyMask_t */
    CfyE_Rule_KeyMask_t Key;

    /** Mask for matching values, can be used to mask out irrelevant Key bits */
    CfyE_Rule_KeyMask_t Mask;

    /**  Data[0] : MAC Destination Address least significant bytes (3..0)
      *  Data[1] : MAC Destination Address most significant bytes (5, 4)
      *  Data[2] : SPI in network byte order (6)
      *  Data[3] : [35:32] : MyTunnelId. [55:36] : Write zeroes and ignore on read
      *  See TCAM packet data fields description in the EIP-163 Programmer Manual
      */
    uint32_t Data[CFYE_RULE_NON_CTRL_WORD_COUNT];

    /** Mask for data values, can be used to mask out irrelevant Data bits */
    uint32_t DataMask[CFYE_RULE_NON_CTRL_WORD_COUNT];

    /** Rule policy */
    CfyE_Rule_Policy_t Policy;
} CfyE_Rule_t;

/** MTT matching rule key/mask data structure */
typedef struct
{
    /** One bit per channel number */
    CfyE_Ch_Mask_t ChannelMask;

    /** false : IPv4
     *  true  : IPv6
     */
    bool fIPv6;

    /** VLAN tag label */
    uint32_t TagLabel1;

    /** VLAN tag labe2 */
    uint32_t TagLabel2;

    /** true - speculatively parsed IP header */
    bool fIPHdrValid;

    /** false : Untagged/VLAN packet
     *  true  : MPLS packet
     */
    bool fPacketType;
} CfyE_MTT_KeyMask_t;

/** MTT matching rule data structure */
typedef struct
{
    /** Sets matching values as specified in CfyE_MTT_KeyMask_t */
    CfyE_MTT_KeyMask_t Key;

    /** Mask for matching values, can be used to mask out irrelevant Key bits */
    CfyE_MTT_KeyMask_t Mask;

    /** IP address (source for egress, destination for ingress) to match.*/
    uint32_t IPAddr[4];

    /** Mask for data values, can be used to mask out irrelevant Data bits */
    uint32_t IPAddrMask[4];
} CfyE_MTT_t;
/**----------------------------------------------------------------------------
 * @typedef CfyE_NotifyFunction_t
 *
 * This type specifies the callback function prototype for the function
 * CfyE_Notify_Request(). The notification will occur only once.
 *
 * @note The exact context in which the callback function is invoked and the
 *       allowed actions in that callback are implementation specific. The
 *       intention is that all API functions can be used, except
 *       CfyE_Device_Init(), CfyE_Device_UnInit(), CfyE API functions with
 *       fSync = true.
 *
 * @param [in] Events
 *     Mask containing the CFYE_EVENT_* flags indicating which Classification
 *     Device events cause the callback.
 *
 * @return value
 *     none
 */
typedef void (* CfyE_NotifyFunction_t)(unsigned int Events);

/** Asynchronous CfyE device notification */
typedef struct
{
    /** Pointer to the callback function */
    CfyE_NotifyFunction_t CBFunc_p;

    /** The requested events,
     *  bitwise OR of CFYE_EVENT_* global events or
     *  CFYE_EVENT_CHAN_* channel-specific events
     */
    unsigned int EventMask;

    /** True if notification is for the device Global AIC,
     *  otherwise the notification is for the device Channel AIC
     */
    bool fGlobal;

    /** If fGlobal is true then this parameter indicates the Channel Id,
     *  otherwise not used
     */
    unsigned int ChannelId;
} CfyE_Notify_t;


/**----------------------------------------------------------------------------
 * @fn CfyE_Notify_Request(
 *     const unsigned int DeviceId,
 *     const CfyE_Notify_t * const Notify_p)
 *
 * This routine can be used to request a one-time notification of available
 * Statistics or Classification device events. It is typically used when the
 * caller does not want to poll the device for classification events.
 *
 * Once the requested events become signaled, the implementation will invoke
 * the callback one-time to notify this fact.
 * The notification is then immediately disabled. It is possible that the
 * notification callback is invoked when not all the requested events (mask)
 * have been signaled (or even zero). In this case the application must
 * invoke CfyE_Notify_Request() again to be notified about the remaining
 * expected events.
 *
 * Once the notification callback is invoked, the application must handle
 * the signaled events and then call CfyE_Notify_Request() again, else the
 * notification callback may not be called again.
 *
 * @param [in] DeviceId
 *      Device identifier of the Classification device.
 *
 * @param [in] Notify_p
 *     Pointer to the notification data structure.
 *
 * This function is NOT re-entrant for the same DeviceId.
 * This function is re-entrant for different DeviceId's.
 *
 * @return  CFYE_STATUS_OK : success
 * @return  CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return  CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Notify_Request(
        const unsigned int DeviceId,
        const CfyE_Notify_t * const Notify_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_Device_Init(
 *     const unsigned int DeviceId,
 *     const CfyE_Role_t Role,
 *     const CfyE_Init_t * const Init_p)
 *
 * Initializes a CfyE device instance identified by IntefaceId parameter.
 *
 * @pre API use order:
 *      This function must be executed before any other of the CfyE_*()
 *      functions may be called for this DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device to be used.
 *
 * @param [in] Role
 *      Device role: Egress or Ingress.
 *
 * @param [in] Init_p
 *      Pointer to a memory location where the device initialization settings
 *      are stored.
 *
 * This function is NOT re-entrant for the same DeviceId.\n
 * This function is re-entrant for different DeviceIds.
 *
 * @note When this function returns an error, all driver resources
 *       are freed and no device entry will be created.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Device_Init(
        const unsigned int DeviceId,
        const CfyE_Role_t Role,
        const CfyE_Init_t * const Init_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_Device_Uninit(
 *     const unsigned int DeviceId)
 *
 * Uninitializes a CfyE device instance identified by DeviceId parameter.
 *
 * @pre API use order:
 *      This function must be called when the CfyE device for this DeviceId
 *      is no longer needed. After this function is called no other CfyE_*()
 *      functions may be called for this DeviceId except the CfyE_Device_Init()
 *      function.
 *
 * @pre Before this function is called all the added vPorts must be removed.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device to be used.
 *
 * This function is NOT re-entrant for the same DeviceId.\n
 * This function is re-entrant for different DeviceIds.
 *
 * This function cannot be called concurrently with any other CfyE API function
 * for the same DeviceId.\n
 * This function can be called concurrently with any other CfyE API function
 * for the different DeviceId.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Device_Uninit(
        const unsigned int DeviceId);


/**----------------------------------------------------------------------------
 * @fn CfyE_Device_Limits_Get(
 *     const unsigned int DeviceId,
 *     CfyE_Device_Limits_t* const device_limits_p)
 *
 * Returns the maximum number of channels, vPorts and/or rules of the
 * classification device instance identified by DeviceId parameter.
 *
 * @pre  API use order:
 *       This function can be called after the function CfyE_Device_Init() and
 *       before the function CfyE_Device_Uninit() is called.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device to be used.
 *
 * @param [out] device_limits_p
 *      Pointer to a data structure where the device limits are returned.
 *
 * This function is re-entrant and may be called always as long as
 * the API use order is respected.
 *
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_Device_Limits_Get(
        const unsigned int DeviceId,
        CfyE_Device_Limits_t* const device_limits_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_Device_Limits(
 *     const unsigned int DeviceId,
 *     unsigned int * const MaxChannelCount_p,
 *     unsigned int * const MaxvPortCount_p,
 *     unsigned int * const MaxRuleCount_p)
 *
 * Returns the maximum number of channels, vPorts and/or rules of the
 * classification device instance identified by DeviceId parameter.
 *
 * @pre  API use order:
 *       This function can be called after the function CfyE_Device_Init() and
 *       before the function CfyE_Device_Uninit() is called.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device to be used.
 *
 * @param [out] MaxChannelCount_p
 *      Pointer to a memory location where the maximum number of Channels must
 *      be stored. Skipped if the pointer is NULL.
 *
 * @param [out] MaxvPortCount_p
 *      Pointer to a memory location where the maximum number of vPorts must
 *      be stored. Skipped if the pointer is NULL.
 *
 * @param [out] MaxRuleCount_p
 *      Pointer to a memory location where the maximum number of TCAM rules
 *      must be stored. Skipped if the pointer is NULL.
 *
 * This function is re-entrant and may be called always as long as
 * the API use order is respected.
 *
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_Device_Limits(
        const unsigned int DeviceId,
        unsigned int * const MaxChannelCount_p,
        unsigned int * const MaxvPortCount_p,
        unsigned int * const MaxRuleCount_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_Device_Update(
 *     const unsigned int DeviceId,
 *     unsigned int ChannelId,
 *     const CfyE_Device_t * const Control_p)
 *
 * This function updates the classification device control settings
 * for the specified channel.
 *
 * The CFYE_EVENT_STAT_TCAM_THR and CFYE_EVENT_STAT_CHAN_THR events can
 * be configured via the CfyE_Device_t:StatControl_p data structure by
 * specifying the packet counter threshold to be reached in order for both
 * events to be generated by the Classification Engine.
 * The CfyE_Notify_Request() can be used to request the asynchronous
 * notifications for these events to be signaled via
 * the CfyE_NotifyFunction_tcallback.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device to be used.
 *
 * @param [in] ChannelId
 *      The channel index (number) to write the control setting from.
 *
 * @param [in] Control_p
 *      Pointer to the memory location where the control parameters
 *      are stored, see CfyE_Device_t.
 *
 * This function is NOT re-entrant for the same DeviceId.\n
 * This function is re-entrant for different DeviceIds.
 *
 * This function cannot be called concurrently with CfyE_Device_Config_Get()
 * or CfyE_Device_Status_Get() for the same DeviceId.
 * It can be called concurrently with any other function
 * for the same DeviceId or with any function for a different DeviceId.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Device_Update(
        const unsigned int DeviceId,
        unsigned int ChannelId,
        const CfyE_Device_t * const Control_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_Device_Config_Get(
 *       const unsigned int DeviceId,
 *       unsigned int ChannelId,
 *       CfyE_Device_t * const Control_p);
 *
 * This function reads the classification device control settings
 * as can be set by CfyE_Device_Update for the specified channel.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device to be used.
 *
 * @param [in] ChannelId
 *      The channel index (number) to write the control setting from.
 *
 * @param [out] Control_p
 *      Pointer to the memory location where the control parameters
 *      are stored, see CfyE_Device_t. The caller must initialize
 *      pointers within this structure to point to memory locations where the
 *      appropriate data structures can be stored.
 *      Alternatively the application can set some pointers within
 *      this structure to NULL and then this function will not read the
 *      corresponding configuration information.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function cannot be called concurrently with CfyE_Device_Update()
 * for the same DeviceId. It can be called concurrently with any other function
 * for the same DeviceId or with any function for a different DeviceId.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Device_Config_Get(
        const unsigned int DeviceId,
        unsigned int ChannelId,
        CfyE_Device_t * const Control_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_Device_Status_Get(
 *       const unsigned int DeviceId,
 *       CfyE_DeviceStatus_t * const DeviceStatus_p);
 *
 * This function reads the Device status and clears any error conditions.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device to be used.
 *
 * @param [out] DeviceStatus_p
 *      Data structure in which to return the device status.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function cannot be called concurrently with CfyE_Device_Update()
 * for the same DeviceId. It can be called concurrently with any other function
 * for the same DeviceId or with any function for a different DeviceId.
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Device_Status_Get(
        const unsigned int DeviceId,
        CfyE_DeviceStatus_t * const DeviceStatus_p);


/**---------------------------------------------------------------------------
 * @fn CfyE_Device_InsertSOP(
 *        const unsigned int DeviceId,
 *        const CfyE_Ch_Mask_t * const ChannelMask_p);
 *
 * This function inserts an SOP signal into the processing pipeline of the CfyE
 * device. This signal can be used with CfyE_Device_InsertEOP() to
 * clear the processing pipe in case of a fault condition.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device to be used.
 *
 * @param [in] ChannelMask_p
 *      Bit mask to specify on which channels the SOP signal must be inserted.
 *      A '1' bit in a bit position specifies the insertion of the signal on
 *      the corresponding channel.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Device_InsertSOP(
        const unsigned int DeviceId,
        const CfyE_Ch_Mask_t * const ChannelMask_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_Device_InsertEOP(
 *        const unsigned int DeviceId,
 *        const CfyE_Ch_Mask_t * const ChannelMask_p);
 *
 * This function inserts an EOP signal into the processing pipeline of the CfyE
 * device. This signal can be used with CfyE_Device_InsertSOP() to
 * clear the processing pipe in case of a fault condition.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device to be used.
 *
 * @param [in] ChannelMask_p
 *      Bit mask to specify on which channels the EOP signal must be inserted.
 *      A '1' bit in a bit position specifies the insertion of the signal on
 *      the corresponding channel.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Device_InsertEOP(
        const unsigned int DeviceId,
        const CfyE_Ch_Mask_t * const ChannelMask_p);


/**---------------------------------------------------------------------------
 * @fn CfyE_Channel_Bypass_Get(
 *       const unsigned int DeviceId,
 *       const unsigned int ChannelId,
 *       bool * const fBypass_p);
 *
 * Read the current channel low-latency bypass setting.
 *
 * API use order:
 *       This function can be called for a CfyE device only after this
 *       device has been initialized via the CfyE_Device_Init() function.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device
 *
 * @param [in] ChannelId
 *      The channel number/index for which the bypass setting is read.
 *
 * @param [out] fBypass_p
 *      Pointer to a bool indication in which the setting must be returned
 *      true if channel is set to low-latency bypass, false if channel is
 *      set for normal operation.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Channel_Bypass_Get(
        const unsigned int DeviceId,
        const unsigned int ChannelId,
        bool * const fBypass_p);


/**---------------------------------------------------------------------------
 * @fn CfyE_Channel_Bypass_Set(
 *       const unsigned int DeviceId,
 *       const unsigned int ChannelId,
 *       const bool fBypass);
 *
 * Configure channel for low-latency bypass.
 *
 * API use order:
 *       This function can be called for a CfyE device only after this
 *       device has been initialized via the CfyE_Device_Init() function.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device
 *
 * @param [in] ChannelId
 *      The channel number/index for which the bypass setting is set.
 *
 * @param [in] fBypass
 *      true if channel is set to low-latency bypass, false if channel is
 *      set for normal operation.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Channel_Bypass_Set(
        const unsigned int DeviceId,
        const unsigned int ChannelId,
        const bool fBypass);


/**----------------------------------------------------------------------------
 * @fn CfyE_Statistics_TCAM_Get(
 *     const unsigned int DeviceId,
 *     const unsigned int StatIndex,
 *     CfyE_Statistics_TCAM_t * const Stat_p,
 *     const bool fSync)
 *
 * This function reads the classification device statistics.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device to be used.
 *
 * @param [in] StatIndex
 *      The statistics counter index (number) to read the statistics for.
 *
 * @param [out] Stat_p
 *      Pointer to the memory location where the device statistics
 *      counter identified by StatIndex will be stored.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      before the statistics is read.
 *
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the CfyE API which also takes fSync parameter set
 * to true for the same DeviceId.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Statistics_TCAM_Get(
        const unsigned int DeviceId,
        const unsigned int StatIndex,
        CfyE_Statistics_TCAM_t * const Stat_p,
        const bool fSync);


/**----------------------------------------------------------------------------
 * @fn CfyE_Statistics_Summary_TCAM_Read(
 *     const unsigned int DeviceId,
 *     const unsigned int StartOffset,
 *     uint32_t * const Summary_p,
 *     const unsigned int Count)
 *
 * Reads the values for one of TCAM counter summary registers.
 * Each bit tells if one corresponding TCAM rule has its statistics counter
 * crossed the configured threshold. When all the summary bits are cleared
 * the CFYE_EVENT_STAT_TCAM_THR event will also be cleared.
 *
 * @param [in] DeviceId
 *      Device ID for the EIP-163 device to be used.
 *
 * @param [in] StartOffset
 *      Start offset of the TCAM counter summary register to start reading from.
 *
 * @param [out] Summary_p
 *      Pointer to a memory location where the content of the TCAM summary
 *      register s will be stored.
 *
 * @param [in] Count
 *      Number of summary registers to read starting from the offset specified
 *      by the StartOffset parameter.
 *
 * This function is NOT re-entrant for the same Device for overlapping set
 * of summary registers.\n
 * This function is re-entrant for the same Device for non-overlapping set
 * of summary registers.\n
 * This function is re-entrant for different Devices.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Statistics_Summary_TCAM_Read(
        const unsigned int DeviceId,
        const unsigned int StartOffset,
        uint32_t * const Summary_p,
        const unsigned int Count);


/**----------------------------------------------------------------------------
 * @fn CfyE_Statistics_MTT_Get(
 *     const unsigned int DeviceId,
 *     const unsigned int StatIndex,
 *     CfyE_Statistics_MTT_t * const Stat_p,
 *     const bool fSync)
 *
 * This function reads the classification device statistics.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device to be used.
 *
 * @param [in] StatIndex
 *      The statistics counter index (number) to read the statistics for.
 *
 * @param [out] Stat_p
 *      Pointer to the memory location where the device statistics
 *      counter identified by StatIndex will be stored.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      before the statistics is read.
 *
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the CfyE API which also takes fSync parameter set
 * to true for the same DeviceId.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Statistics_MTT_Get(
        const unsigned int DeviceId,
        const unsigned int StatIndex,
        CfyE_Statistics_MTT_t * const Stat_p,
        const bool fSync);


/**----------------------------------------------------------------------------
 * @fn CfyE_Statistics_Summary_MTT_Read(
 *     const unsigned int DeviceId,
 *     uint32_t * const Summary_p)
 *
 * Reads the values for one of TCAM counter summary registers.
 * Each bit tells if one corresponding TCAM rule has its statistics counter
 * crossed the configured threshold. When all the summary bits are cleared
 * the CFYE_EVENT_STAT_TCAM_THR event will also be cleared.
 *
 * @param [in] DeviceId
 *      Device ID for the EIP-163 device to be used.
 *
 * @param [out] Summary_p
 *      Pointer to a memory location where the content of the TCAM summary
 *      register s will be stored.
 *
 * This function is NOT re-entrant for the same Device for overlapping set
 * of summary registers.\n
 * This function is re-entrant for the same Device for non-overlapping set
 * of summary registers.\n
 * This function is re-entrant for different Devices.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Statistics_Summary_MTT_Read(
        const unsigned int DeviceId,
        uint32_t * const Summary_p);

/**----------------------------------------------------------------------------
 * @fn CfyE_Statistics_Channel_Get(
 *        const unsigned int DeviceId,
 *       const unsigned int StatIndex,
 *       CfyE_Statistics_Channel_t * const Stat_p,
 *       const bool fSync);
 *
 * This function reads the classification device per-channel statistics.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device to be used.
 *
 * @param [in] StatIndex
 *      The statistics counter index (number) to read the statistics for.
 *
 * @param [out] Stat_p
 *      Pointer to the memory location where the device statistics
 *      counter identified by StatIndex will be stored.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      before the statistics is read.
 *
 * This function is NOT re-entrant for the same Device when fSync is true.
 * This function is re-entrant for the same Device when fSync is false.
 * This function is re-entrant for different Devices.
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the CfyE API which also takes fSync parameter set
 * to true for the same DeviceId.
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Statistics_Channel_Get(
        const unsigned int DeviceId,
        const unsigned int StatIndex,
        CfyE_Statistics_Channel_t * const Stat_p,
        const bool fSync);


/**---------------------------------------------------------------------------
 * @fn CfyE_Statistics_Channel_Clear(
 *       const unsigned int DeviceId,
 *       const unsigned int StatIndex);
 *
 * Clear the CfyE statistics counters belonging to a given StatIndex.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device to be used.
 *
 * @param [in] StatIndex
 *      The statistics counter index (number) to read the statistics for.
 *
 * This function is NOT re-entrant for the same Device.
 * This function is re-entrant for different Devices.
 *
 * @return CFYE_STATUS_OK : success
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Statistics_Channel_Clear(
        const unsigned int DeviceId,
        const unsigned int StatIndex);


/**----------------------------------------------------------------------------
 * @fn CfyE_Statistics_Summary_Channel_Read(
 *     const unsigned int DeviceId,
 *     CfyE_Ch_Mask_t * const ChSummary_p);
 *
 * Reads the values for one of TCAM counter summary registers. When all
 * the summary bits are cleared the CFYE_EVENT_STAT_CHAN_THR event will also
 * be cleared.
 *
 * @param [in] DeviceId
 *      Device ID for the EIP-163 device to be used.
 *
 * @param [out] ChSummary_p
 *      Pointer to a memory location where the content of the channel counter
 *      summary will be stored (one bit per channel in the Channel Mask).
 *
 * This function is NOT re-entrant for the same Device.\n
 * This function is re-entrant for different Devices.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Statistics_Summary_Channel_Read(
        const unsigned int DeviceId,
        CfyE_Ch_Mask_t * const ChSummary_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_vPortHandle_IsSame(
 *     const CfyE_vPortHandle_t Handle1_p,
 *     const CfyE_vPortHandle_t Handle2_p)
 *
 * Check whether provided Handle1 is equal to provided Handle2.
 *
 * @param Handle1_p
 *      First handle
 *
 * @param Handle2_p
 *      Second handle
 *
 * This function is re-entrant for the same or different DeviceIds.
 *
 * This function can be called concurrently with
 * any other CfyE or SecY API function for the same or different DeviceId.
 *
 * @return     true:  provided handles are equal
 * @return     false: provided handles are not equal
 */
bool
CfyE_vPortHandle_IsSame(
        const CfyE_vPortHandle_t Handle1_p,
        const CfyE_vPortHandle_t Handle2_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_vPort_Add(
 *     const unsigned int DeviceId,
 *     CfyE_vPortHandle_t * const vPortHandle_p,
 *     const CfyE_vPort_t * const vPort_p,
 *     const CfyE_Channel_Mode_t ChannelMode)
 *
 * Adds a new vPort (vPort policy) for one classification device instance
 * identified by DeviceId parameter.
 *
 * @note If a vPort is added for an SA then it must be added to the same SecY
 *       device (DeviceId) where the SA was added via the SecY_SA_Add() function.
 *
 * @pre  API use order:
 *       A vPort can be added to a classification device instance only after this
 *       device has been initialized via the CfyE_Device_Init() function.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device
 *
 * @param [out] vPortHandle_p
 *      Pointer to a memory location where where the vPort handle will be stored.
 *
 * @param [in] vPort_p
 *      Pointer to a memory location where the data for the vPort is stored.
 *
 * @param [in] ChannelMode
 *      Operation Mode for which the vPort is being addded.
 *
 * This function is NOT re-entrant for the same vPort of the same DeviceId.\n
 * This function is re-entrant for different DeviceIds or different vPorts
 * of the same DeviceId.
 *
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @note When this function returns an error other than CFYE_ERROR_BAD_PARAMETER
 *       and the returned vPort handle is not a null handle. an entry for
 *       this vPort was allocated and it should be removed using
 *       CfyE_vPort_Remove().
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_vPort_Add(
        const unsigned int DeviceId,
        CfyE_vPortHandle_t * const vPortHandle_p,
        const CfyE_vPort_t * const vPort_p,
        const CfyE_Channel_Mode_t ChannelMode);

/**----------------------------------------------------------------------------
 * @fn CfyE_vPort_Remove(
 *     const unsigned int DeviceId,
 *     const CfyE_vPortHandle_t vPortHandle)
 *
 * Removes an already added vPort (vPort policy) from one classification device
 * instance identified by DeviceId parameter.
 *
 * @pre  API use order:
 *       A vPort can be removed from a classification device instance only after
 *       this vPort has been added to the device via the CfyE_vPort_Add()
 *       function.
 *
 *       Before this function can be called all the Rules associated with
 *       this vPort must be removed.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device
 *
 * @param [in] vPortHandle
 *      vPort handle for vPort to be removed.
 *
 * This function is NOT re-entrant for the same vPort of the same DeviceId.\n
 * This function is re-entrant for different DeviceIds or different vPorts
 * of the same DeviceId.
 *
 * This function cannot be called concurrently with CfyE_Rule_Add() and
 * CfyE_Rule_Remove() functions for the same DeviceId.\n
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_vPort_Remove(
        const unsigned int DeviceId,
        const CfyE_vPortHandle_t vPortHandle);


/**----------------------------------------------------------------------------
 * @fn CfyE_vPort_Update(
 *     const unsigned int DeviceId,
 *     const CfyE_vPortHandle_t vPortHandle,
 *     const CfyE_vPort_t * const vPort_p)
 *
 * Updates a vPort (vPort policy) for one classification device instance
 * identified by DeviceId parameter.
 *
 * @pre  API use order:
 *       A vPort can be updated for a classification device instance only after
 *       this vPort has been added to the device via the CfyE_vPort_Add()
 *       function.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device
 *
 * @param [in] vPortHandle
 *      vPort handle for vPort to be updated.
 *
 * @param [in] vPort_p
 *      Pointer to a memory location where the data for the vPort is stored.
 *
 * This function is NOT re-entrant for the same vPort of the same DeviceId.\n
 * This function is re-entrant for different DeviceIds or different vPorts
 * of the same DeviceId.
 *
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_vPort_Update(
        const unsigned int DeviceId,
        const CfyE_vPortHandle_t vPortHandle,
        const CfyE_vPort_t * const vPort_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_RuleHandle_IsSame(
 *     const CfyE_RuleHandle_t Handle1_p,
 *     const CfyE_RuleHandle_t Handle2_p)
 *
 * Check whether provided Handle1 is equal to provided Handle2.
 *
 * @param Handle1_p
 *      First handle
 *
 * @param Handle2_p
 *      Second handle
 *
 * This function is re-entrant for the same or different DeviceIds.
 *
 * This function can be called concurrently with any other CfyE API function
 * for the same or different DeviceId.
 *
 * @return     true:  provided handles are equal
 * @return     false: provided handles are not equal
 */
bool
CfyE_RuleHandle_IsSame(
        const CfyE_RuleHandle_t Handle1_p,
        const CfyE_RuleHandle_t Handle2_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_Rule_Add(
 *     const unsigned int DeviceId,
 *     const CfyE_vPortHandle_t vPortHandle,
 *     CfyE_RuleHandle_t * const RuleHandle_p,
 *     const CfyE_Rule_t * const Rule_p)
 *
 * Adds a new rule for matching a packet to a vPort identified by vPortHandle
 * for one classification device instance identified by DeviceId parameter.
 *
 * @note If a rule is added for a vPort then it must be added to the same
 *       device (DeviceId) where the vPort was added via the CfyE_vPort_Add()
 *       function.
 *
 * @pre  API use order:
 *       A rule can be added to a classification device instance only after
 *       the vPort identified by vPortHandle has been added to this device
 *       via the CfyE_vPort_Add() function.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device
 *
 * @param [in] vPortHandle
 *      vPort handle for the vPort where the packet matching rule must be added.
 *
 * @param [out] RuleHandle_p
 *      Pointer to a memory location where where the rule handle will be stored.
 *
 * @param [in] Rule_p
 *      Pointer to a memory location where the data for the rule is stored.
 *
 * This function is NOT re-entrant for the same rule of the same DeviceId.\n
 * This function is re-entrant for different DeviceIds or different rules
 * of the same DeviceId.
 *
 * This function cannot be called concurrently with CfyE_vPort_Remove() and
 * CfyE_Rule_Remove() functions for the same DeviceId.\n
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @note When this function returns an error other than CFYE_ERROR_BAD_PARAMETER
 *       and the returned Rule handle is not a null handle. an entry for
 *       this Rule was allocated and it should be removed using
 *       CfyE_Rule_Remove().
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Rule_Add(
        const unsigned int DeviceId,
        const CfyE_vPortHandle_t vPortHandle,
        CfyE_RuleHandle_t * const RuleHandle_p,
        const CfyE_Rule_t * const Rule_p);

/**----------------------------------------------------------------------------
 * @fn CfyE_Rule_Add_Index(
 *     const unsigned int DeviceId,
 *     const CfyE_vPortHandle_t vPortHandle,
 *     CfyE_RuleHandle_t * const RuleHandle_p,
 *     const CfyE_Rule_t * const Rule_p,
 *     const unsigned int RuleIndex)
 *
 * Adds a new rule for matching a packet to a vPort identified by vPortHandle
 * for one classification device instance identified by DeviceId parameter.
 * Using the given TCAM index.
 *
 * @note: If a rule is added for a vPort then it must be added to the same
 *       device (DeviceId) where the vPort was added via the CfyE_vPort_Add()
 *       function.
 * @note: The function CfyE_Rule_Add() allocates TCAM entries by itself and
 *       when this function is called, the given index may not be available.
 *       Either allocate all rules with CfyE_Rule_Add or allocate all rules
 *       with CfyE_Rule_Add_index()
 *
 * @pre API use order:
 *       A rule can be added to a classification device instance only after
 *       the vPort identified by vPortHandle has been added to this device
 *       via the CfyE_vPort_Add() function.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device
 *
 * @param [in] vPortHandle
 *      vPort handle for the vPort where the packet matching rule must be added.
 *
 * @param [out] RuleHandle_p
 *      Pointer to a memory location where where the rule handle will be stored.
 *
 * @param [in] Rule_p
 *      Pointer to a memory location where the data for the rule is stored.
 *
 * @param [in] RuleIndex
 *      TCAM index where the rule must be added. This entry must be free
 *      when the function is called.
 *
 * This function is NOT re-entrant for the same rule of the same DeviceId.
 * This function is re-entrant for different DeviceIds or different rules
 * of the same DeviceId.
 *
 * This function cannot be called concurrently with CfyE_vPort_Remove() and
 * CfyE_Rule_Remove() functions for the same DeviceId.
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @note When this function returns an error other than CFYE_ERROR_BAD_PARAMETER
 *       and the returned Rule handle is not a null handle. an entry for
 *       this Rule was allocated and it should be removed using
 *       CfyE_Rule_Remove().
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Rule_Add_Index(
        const unsigned int DeviceId,
        const CfyE_vPortHandle_t vPortHandle,
        CfyE_RuleHandle_t * const RuleHandle_p,
        const CfyE_Rule_t * const Rule_p,
        const unsigned int RuleIndex);


/**----------------------------------------------------------------------------
 * @fn CfyE_Rule_Remove(
 *     const unsigned int DeviceId,
 *     const CfyE_RuleHandle_t RuleHandle)
 *
 * Removes an already added rule from one classification device
 * instance identified by DeviceId parameter.
 *
 * @pre  API use order:
 *       A rule can be removed from a classification device instance only after
 *       this rule has been added to the device via the CfyE_Rule_Add()
 *       function.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device
 *
 * @param [in] RuleHandle
 *      Rule handle for the rule to be removed.
 *
 * This function is NOT re-entrant for the same rule of the same DeviceId.\n
 * This function is re-entrant for different DeviceIds or different rules
 * of the same DeviceId.
 *
 * This function cannot be called concurrently with CfyE_vPort_Remove() and
 * CfyE_Rule_Add() functions for the same DeviceId.\n
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Rule_Remove(
        const unsigned int DeviceId,
        const CfyE_RuleHandle_t RuleHandle);


/**----------------------------------------------------------------------------
 * @fn CfyE_Rule_Update(
 *     const unsigned int DeviceId,
 *     const CfyE_RuleHandle_t RuleHandle,
 *     const CfyE_Rule_t * const Rule_p)
 *
 * Updates a packet matching rule for one classification device instance
 * identified by DeviceId parameter.
 *
 * @note A rule must be updated to the same device (DeviceId) where
 *       the corresponding vPort was added via the CfyE_vPort_Add() function.
 *
 * @pre  API use order:
 *       A rule can be updated for a classification device instance only after
 *       the corresponding vPort has been added to this device
 *       via the CfyE_vPort_Add() function.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device
 *
 * @param [in] RuleHandle
 *      Rule handle for rule to be enabled.
 *
 * @param [in] Rule_p
 *      Pointer to a memory location where the data for the rule is stored.
 *
 * This function is NOT re-entrant for the same rule of the same DeviceId.\n
 * This function is re-entrant for different DeviceIds or different rules
 * of the same DeviceId.
 *
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Rule_Update(
        const unsigned int DeviceId,
        const CfyE_RuleHandle_t RuleHandle,
        const CfyE_Rule_t * const Rule_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_Rule_Enable(
 *     const unsigned int DeviceId,
 *     const CfyE_RuleHandle_t RuleHandle,
 *     const bool fSync)
 *
 * Enables an already added rule for one classification device instance
 * identified by DeviceId parameter.
 *
 * @note On devices with an external TCAM, rules cannot be enabled.
 *
 * @pre  API use order:
 *       A rule can be enabled after it has been added
 *       via the CfyE API CfyE_Rule_Add() function or updated via
 *       the CfyE_Rule_Update() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device
 *
 * @param [in] RuleHandle
 *      Rule handle for rule to be enabled.
 *
 * @param fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      before the rule is enabled.
 *
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.
 *
 * This function can be called concurrently with any other CfyE API function
 * for the same or different RuleHandle of the same Device or
 * different Devices provided the API use order is followed.
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the CfyE API which also takes fSync parameter set
 * to true for the same Device.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Rule_Enable(
        const unsigned int DeviceId,
        const CfyE_RuleHandle_t RuleHandle,
        const bool fSync);


/**----------------------------------------------------------------------------
 * @fn CfyE_Rule_Disable(
 *     const unsigned int DeviceId,
 *     const CfyE_RuleHandle_t RuleHandle,
 *     const bool fSync)
 *
 * Disables an already added rule for one classification device instance
 * identified by DeviceId parameter.
 *
 * @pre  API use order:
 *       A rule can be enabled after it has been added
 *       via the CfyE API CfyE_Rule_Add() function or updated via
 *       the CfyE_Rule_Update() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device
 *
 * @param [in] RuleHandle
 *      Rule handle for rule to be enabled.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      after the rule is disabled.
 *
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.
 *
 * This function can be called concurrently with any other CfyE API function
 * for the same or different RuleHandle of the same Device or
 * different Devices provided the API use order is followed.
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the CfyE API which also takes fSync parameter set
 * to true for the same Device.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Rule_Disable(
        const unsigned int DeviceId,
        const CfyE_RuleHandle_t RuleHandle,
        const bool fSync);


/**----------------------------------------------------------------------------
 * @fn CfyE_Rule_EnableDisable(
 *     const unsigned int DeviceId,
 *     const CfyE_RuleHandle_t RuleHandleDisable,
 *     const CfyE_RuleHandle_t RuleHandleEnable,
 *     const bool EnableAll,
 *     const bool DisableAll,
 *     const bool fSync)
 *
 * Enables and/or disables an already added rule from one Classification device
 * instance identified by DeviceId parameter. Can also enable or disable all
 * rules for this device instance at once.
 *
 * @note On devices with an external TCAM, rules cannot be enabled.
 *
 * @pre  API use order:
 *       An rule can be enabled or disabled after its transform record is
 *       installed via the SecY API SecY_SA_Add() function and the rule is
 *       added via the CfyE_Rule_Add() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the Classification device
 *
 * @param [in] RuleHandleEnable
 *      Rule handle for rule to be enabled. Will be ignored if equal to
 *      CfyE_RuleHandle_NULL.
 *
 * @param [in] RuleHandleDisable
 *      Rule handle for rule to be disabled. Will be ignored if equal to
 *      CfyE_RuleHandle_NULL.
 *
 * @param [in] EnableAll
 *      When set to true all rules will be enabled. Takes precedence over the
 *      other parameters.
 *
 * @param [in] DisableAll
 *      When set to true all rules will be disabled. Takes precedence over the
 *      other parameters, except EnableAll.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      after the rule is enabled or disabled or both.
 *
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.
 *
 * This function can be called concurrently with any other CfyE API function
 * for the same or different RuleHandles of the same Device or
 * different Devices provided the API use order is followed.
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the CfyE API which also takes fSync parameter set
 * to true for the same Device.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_Rule_EnableDisable(
        const unsigned int DeviceId,
        const CfyE_RuleHandle_t RuleHandleDisable,
        const CfyE_RuleHandle_t RuleHandleEnable,
        const bool EnableAll,
        const bool DisableAll,
        const bool fSync);


/**----------------------------------------------------------------------------
 * @fn CfyE_MTT_Update(
 *     const unsigned int DeviceId,
 *     const unsigned int MTTIndex,
 *     const CfyE_MTT_t * const MTT_p)
 *
 * Updates an MTT entry for one classification device instance
 * identified by DeviceId parameter.
 *
 * @note A rule must be updated to the same device (DeviceId) where
 *       the corresponding vPort was added via the CfyE_vPort_Add() function.
 *
 * @pre  API use order:
 *       A rule can be updated for a classification device instance only after
 *       the corresponding vPort has been added to this device
 *       via the CfyE_vPort_Add() function.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device
 *
 * @param [in] MTTIndex
 *      Index of the MTT entry to be updated.
 *
 * @param [in] MTT_p
 *      Pointer to a memory location where the data for the rule is stored.
 *
 * This function is NOT re-entrant for the same rule of the same DeviceId.\n
 * This function is re-entrant for different DeviceIds or different rules
 * of the same DeviceId.
 *
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_MTT_Update(
        const unsigned int DeviceId,
        const unsigned int MTTIndex,
        const CfyE_MTT_t * const MTT_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_MTT_Enable(
 *     const unsigned int DeviceId,
 *     const unsigned int MTTIndex,
 *     const bool fSync)
 *
 * Enables an MTT entry for one classification device instance
 * identified by DeviceId parameter.
 *
 * @note On devices with an external TCAM, rules cannot be enabled.
 *
 * @pre  API use order:
 *       A rule can be enabled after it has been added
 *       via the CfyE API CfyE_Rule_Add() function or updated via
 *       the CfyE_Rule_Update() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device
 *
 * @param [in] MTTIndex
 *      MTT index to be enabled.
 *
 * @param fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      before the rule is enabled.
 *
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.
 *
 * This function can be called concurrently with any other CfyE API function
 * for the same or different RuleHandle of the same Device or
 * different Devices provided the API use order is followed.
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the CfyE API which also takes fSync parameter set
 * to true for the same Device.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_MTT_Enable(
        const unsigned int DeviceId,
        const unsigned int MTTIndex,
        const bool fSync);


/**----------------------------------------------------------------------------
 * @fn CfyE_MTT_Disable(
 *     const unsigned int DeviceId,
 *     const unsigned int MTTIndex,
 *     const bool fSync)
 *
 * Disables an MTT entry for one classification device instance
 * identified by DeviceId parameter.
 *
 * @pre  API use order:
 *       A rule can be enabled after it has been added
 *       via the CfyE API CfyE_Rule_Add() function or updated via
 *       the CfyE_Rule_Update() function for the same DeviceId.
 *
 * @param [in] DeviceId
 *      Device identifier of the classification device
 *
 * @param [in] MTTIndex
 *      MTT index to be disabled.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      after the rule is disabled.
 *
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.
 *
 * This function can be called concurrently with any other CfyE API function
 * for the same or different RuleHandle of the same Device or
 * different Devices provided the API use order is followed.
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the CfyE API which also takes fSync parameter set
 * to true for the same Device.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_MTT_Disable(
        const unsigned int DeviceId,
        const unsigned int MTTIndex,
        const bool fSync);


/**----------------------------------------------------------------------------
 * @fn CfyE_MTT_EnableDisable(
 *     const unsigned int DeviceId,
 *     const unsigned int  MTTEnable,
 *     const unsigned int  MTTDisable,
 *     const bool EnableSingle,
 *     const bool DisableSingle,
 *     const bool EnableAll,
 *     const bool DisableAll,
 *     const bool fSync)
 *
 * Enables and/or disables an MTT entry from one Classification device
 * instance identified by DeviceId parameter. Can also enable or disable all
 * rules for this device instance at once.
 *
 * @note On devices with an external TCAM, rules cannot be enabled.
 *
 * @param [in] DeviceId
 *      Device identifier of the Classification device
 *
 * @param [in] MTTEnable
 *      MTT index of the entry to be enabled.
 *
 * @param [in] MTTDisable
 *      MTT index of the entry to be disabled.
 *
 * @param [in] EnableSingle
 *      Set if a single entry (indicated by MTTEnable) must be enabled
 *
 * @param [in] DisableSingle
 *      Set if a single entry (indicated by MTTDisable) must be disabled
 *
 * @param [in] EnableAll
 *      When set to true all entries will be enabled. Takes precedence over the
 *      other parameters.
 *
 * @param [in] DisableAll
 *      When set to true all entries will be disabled. Takes precedence over the
 *      other parameters, except EnableAll.
 *
 * @param [in] fSync
 *      If true then this function will ensure that all the packets
 *      available in the device at the time of the function call are processed
 *      after the rule is enabled or disabled or both.
 *
 * This function is NOT re-entrant for the same Device when fSync is true.\n
 * This function is re-entrant for the same Device when fSync is false.\n
 * This function is re-entrant for different Devices.
 *
 * This function can be called concurrently with any other CfyE API function
 * for the same or different RuleHandles of the same Device or
 * different Devices provided the API use order is followed.
 *
 * If fSync is set to true then this function cannot be called concurrently
 * with any other function of the CfyE API which also takes fSync parameter set
 * to true for the same Device.
 *
 * @return    CFYE_STATUS_OK : success
 * @return    CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 * @return    CFYE_INTERNAL_ERROR : failure
 * @return other values: device read/write errors.
 */
CfyE_Status_t
CfyE_MTT_EnableDisable(
        const unsigned int DeviceId,
        const unsigned int MTTEnable,
        const unsigned int MTTDisable,
        const bool EnableSingle,
        const bool DisableSingle,
        const bool EnableAll,
        const bool DisableAll,
        const bool fSync);

/**----------------------------------------------------------------------------
 * @fn CfyE_RuleIndex_Get(
 *     const CfyE_RuleHandle_t RuleHandle,
 *     unsigned int * const RuleIndex_p)
 *
 * Get the Rule index from a Rule, using the Rule handle.
 *
 * @pre API use order:
 *      This function may be called for the RuleHandle obtained via
 *      the CfyE_Rule_Add() function for the same DeviceId.
 *
 * @param [in] RuleHandle
 *      Rule handle of the Rule to get the index from.
 *
 * @param [out] RuleIndex_p
 *      Pointer to a memory location where the Rule index will be stored.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return CFYE_STATUS_OK : success\n
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 */
CfyE_Status_t
CfyE_RuleIndex_Get(
        const CfyE_RuleHandle_t RuleHandle,
        unsigned int * const RuleIndex_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_vPortIndex_Get(
 *     const CfyE_vPortHandle_t vPortHandle,
 *     unsigned int * const vPortIndex_p)
 *
 * Get the vPort index from an vPort, using the vPort handle.
 *
 * @pre API use order:
 *      This function may be called for the vPortHandle obtained via
 *      the CfyE_vPort_Add() function for the same DeviceId.
 *
 * @param [in] vPortHandle
 *      vPort handle of the vPort to get the index from.
 *
 * @param [out] vPortIndex_p
 *      Pointer to a memory location where the vPort index will be stored.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return CFYE_STATUS_OK : success\n
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter
 */
CfyE_Status_t
CfyE_vPortIndex_Get(
        const CfyE_vPortHandle_t vPortHandle,
        unsigned int * const vPortIndex_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_RuleHandle_Get(
 *     const unsigned int DeviceId,
 *     const unsigned int RuleIndex,
 *     CfyE_RuleHandle_t * const RuleHandle_p)
 *
 * Get the Rule handle from a Rule, using the Rule index
 *
 * @pre API use order:
 *      This function can be called after the function CfyE_Device_Init()
 *      and before the function CfyE_Device_Uninit() is called.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device.
 *
 * @param [in] RuleIndex
 *      Rule index for which the Rule handle must be returned.
 *
 * @param [out] RuleHandle_p
 *     Pointer to a memory location where the Rule handle will be stored.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return CFYE_STATUS_OK : success\n
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter\n
 * @return CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_RuleHandle_Get(
        const unsigned int DeviceId,
        const unsigned int RuleIndex,
        CfyE_RuleHandle_t * const RuleHandle_p);


/**----------------------------------------------------------------------------
 * @fn CfyE_vPortHandle_Get(
 *     const unsigned int DeviceId,
 *     const unsigned int vPortIndex,
 *     CfyE_vPortHandle_t * const vPortHandle_p)
 *
 * Get the vPort handle from an vPort, using the vPort index.
 *
 * @pre API use order:
 *      This function can be called after the function CfyE_Device_Init()
 *      and before the function CfyE_Device_Uninit() is called.
 *
 * @param [in] DeviceId
 *      Device identifier of the CfyE device.
 *
 * @param [in] vPortIndex
 *     vPort index for which the vPort handle must be returned.
 *
 * @param [out] vPortHandle_p
 *     Pointer to a memory location where the vPort handle will be stored.
 *
 * This function is re-entrant for the same or different Devices.
 *
 * This function can be called concurrently with any other CfyE API
 * function for the same or different DeviceId provided the API use order
 * is followed.
 *
 * @return CFYE_STATUS_OK : success\n
 * @return CFYE_ERROR_BAD_PARAMETER : incorrect input parameter\n
 * @return CFYE_INTERNAL_ERROR : failure
 */
CfyE_Status_t
CfyE_vPortHandle_Get(
        const unsigned int DeviceId,
        const unsigned int vPortIndex,
        CfyE_vPortHandle_t * const vPortHandle_p);


#endif /* API_CFYE_H_ */


/* end of file api_cfye.h */
