/** @file api_secy_types.h
 *
 * @brief MACsec SecY API, type definitions
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

#ifndef API_SECY_TYPES_H_
#define API_SECY_TYPES_H_

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
        - SA counters
        - Interface counters
        - Interface1 counters
        - SecY counters
        - Channel counters
        - RxCAM counters */
/** SA counters Increment disable bit */
#define SECY_SA_COUNT_INC_DIS               BIT_0
/** Interface counters Increment disable bit */
#define SECY_IFC_COUNT_INC_DIS              BIT_1
/** Interface1 counters Increment disable bit */
#define SECY_IFC1_COUNT_INC_DIS             BIT_2
/** SecY counters Increment disable bit */
#define SECY_SECY_COUNT_INC_DIS             BIT_3
/** Channel counters Increment disable bit */
#define SECY_CHAN_COUNT_INC_DIS             BIT_4
/** RxCAM counters Increment disable bit */
#define SECY_RXCAM_COUNT_INC_DIS            BIT_5

/** Maximum number of device channels the register interface supports
 *  This number should be used for operations with the channel map,
 *  see SecY_ChannelConf_t:ChannelMap.
 *  The number of actually used channels can be less than this number.
 */
#define SECY_MAX_NOF_CHANNELS               64

/** Status values returned by API functions */
enum
{
    SECY_STATUS_OK,            /**< \n */
    SECY_ERROR_BAD_PARAMETER,  /**< \n */
    SECY_ERROR_INTERNAL,       /**< \n */
    SECY_ERROR_NOT_IMPLEMENTED, /**< \n */
};
/** Return type for SecY API functions. above enum values and any other
    integer values for device read/write errors */
typedef int SecY_Status_t;

/** SecY device role: either egress only, ingress only or both */
typedef enum
{
    SECY_ROLE_EGRESS,         /**< \n */
    SECY_ROLE_INGRESS,        /**< \n */
    SECY_ROLE_EGRESS_INGRESS, /**< \n */
    SECY_ROLE_IPSEC_EGRESS,   /**< \n */
    SECY_ROLE_IPSEC_INGRESS   /**< \n */
} SecY_Role_t;

/** Statistics counter */
typedef struct
{
    uint32_t Lo; /**< Low 32-bit counter word */
    uint32_t Hi; /**< High 32-bit counter word */
} SecY_Stat_Counter_t;

/**----------------------------------------------------------------------------
 * This handle is a reference to an SA. It is returned when an SA
 * is added and it remains valid until the SA is removed.
 *
 * The handle is set to NULL when SecY_SAHandle_t handle.p is equal to NULL.
 */
typedef struct
{
    /** Reference handle to an SA */
    void * p;
} SecY_SAHandle_t;

/**----------------------------------------------------------------------------
 * @var SecY_SAHandle_NULL
 *
 * This handle can be assigned to a variable of type SecY_SAHandle_t.
 */
extern const SecY_SAHandle_t SecY_SAHandle_NULL;

/**----------------------------------------------------------------------------
 * SecY_PortType_t
 *
 * Types of ports:\n
 *      Tx MAC\n
 *      Redirect FIFO\n
 *      Capture debug FIFO\n
 *      TX MAC and Capture debug FIFO
 */
typedef enum
{
    SECY_PORT_COMMON,      /**< \n */
    SECY_PORT_RESERVED,    /**< \n */
    SECY_PORT_CONTROLLED,  /**< \n */
    SECY_PORT_UNCONTROLLED /**< \n */
} SecY_PortType_t;

/**----------------------------------------------------------------------------
 * @typedef SecY_SA_ActionType_t
 *
 * SA action type:\n
 *      bypass,\n
 *      drop,\n
 *      MACsec ingress (do not use for an egress only device),\n
 *      MACsec egress (do not use for an ingress only device).
 */
typedef enum
{
    SECY_SA_ACTION_BYPASS,        /**< \n */
    SECY_SA_ACTION_DROP,          /**< \n */
    SECY_SA_ACTION_INGRESS,       /**< \n */
    SECY_SA_ACTION_EGRESS,        /**< \n */
    SECY_SA_ACTION_CRYPT_AUTH,    /**< \n */
    SECY_SA_ACTION_IPSEC_INGRESS, /**< \n */
    SECY_SA_ACTION_IPSEC_EGRESS,  /**< \n */
} SecY_SA_ActionType_t;

/**----------------------------------------------------------------------------
 * @typedef SecY_DropType_t
 *
 * SA drop type:\n
 *   0 = bypass with CRC corruption signaling,\n
 *   1 = bypass with bad packet indicator,\n
 *   2 = internal drop by crypto-core (packet is not seen outside),\n
 *   3 = do not drop (for debugging only).\n
 */
typedef enum
{
    SECY_SA_DROP_CRC_ERROR, /**< \n */
    SECY_SA_DROP_PKT_ERROR, /**< \n */
    SECY_SA_DROP_INTERNAL,  /**< \n */
    SECY_SA_DROP_NONE       /**< \n */
} SecY_DropType_t;

/**----------------------------------------------------------------------------
 * @typedef SecY_ValidateFrames_t
 *
 * Ingress tagged frame validation options
 */
typedef enum
{
    SECY_FRAME_VALIDATE_DISABLE, /**< \n */
    SECY_FRAME_VALIDATE_CHECK,   /**< \n */
    SECY_FRAME_VALIDATE_STRICT   /**< \n */
} SecY_ValidateFrames_t;

/** Each bit in the Channel bit mask represents a Channel. Each word represents
 * a maximum of 32 Channels
 */
#define SECY_CHANNEL_WORDS 2

/** Channel Mask */
typedef struct {
    /** Each bit in the Channel bit mask represents a channel supported by the
     *  SecY device
     */
    uint32_t ch_bitmask[SECY_CHANNEL_WORDS];
} SecY_Ch_Mask_t;

/** SA parameters for Egress action type */
typedef struct
{
    /** true - SA is in use, packets classified for it can be transformed\n
     *  false - SA not in use, packets classified for it can not be transformed
     */
    bool fSAInUse;

    /** The number of bytes (in the range of 0-127) that are authenticated but
     *  not encrypted following the SecTAG in the encrypted packet. Values
     *  65-127 are reserved in HW < 4.0 and should not be used there. */
    uint8_t ConfidentialityOffset;

    /** true - enable frame protection,\n
        false - bypass frame through device */
    bool fProtectFrames;

    /** true - inserts explicit SCI in the packet,\n
        false - use implicit SCI (not transferred) */
    bool fIncludeSCI;

    /** true - enable ES bit in the generated SecTAG\n
        false - disable ES bit in the generated SecTAG */
    bool fUseES;

    /** true - enable SCB bit in the generated SecTAG\n
        false - disable SCB bit in the generated SecTAG */
    bool fUseSCB;

    /** true - enable confidentiality protection\n
        false - disable confidentiality protection */
    bool fConfProtect;

    /** true - allow data (non-control) packets.\n
        false - drop data packets.*/
    bool fAllowDataPkts;

    /** Specifies number of bytes from the start of the frame
        to be bypassed without MACsec protection */
    uint8_t PreSecTagAuthStart;

    /** Specifies number of bytes to be authenticated in the pre-SecTAG area.*/
    uint8_t PreSecTagAuthLength;
} SecY_SA_E_t;

/** SA parameters for Ingress action type */
typedef struct
{
    /** true - SA is in use, packets classified for it can be transformed\n
     *  false - SA not in use, packets classified for it can not be transformed
     */
    bool fSAInUse;

    /** The number of bytes (in the range of 0-127) that are authenticated but
     *  not encrypted following the SecTAG in the encrypted packet. Values
     *  65-127 are reserved in hardware < 4.0 and should not be used there.
     */
    uint8_t ConfidentialityOffset;

    /** true - enable replay protection\n
        false - disable replay protection */
    bool fReplayProtect;

    /** MACsec frame validation level (tagged). */
    SecY_ValidateFrames_t ValidateFramesTagged;

    /** SCI to which ingress SA applies (8 bytes). */
    uint8_t *SCI_p;

    /** Association number to which ingress SA applies. */
    uint8_t AN;

    /** true - allow tagged packets.\n
       false - drop tagged packets. */
    bool fAllowTagged;

    /** true - allow untagged packets.\n
        false - drop untagged packets. */
    bool fAllowUntagged;

    /** true - enable validate untagged packets.\n
        false - disable validate untagged packets. */
    bool fValidateUntagged;

    /** pre-Sectag Auth start  */
    uint8_t PreSecTagAuthStart;

    /** pre-Sectag Auth length */
    uint8_t PreSecTagAuthLength;

    /** For situations when RxSC is not found or fSAInUse=false with validation
     *  level that allows packet to be sent to the Controlled port with
     *  the SecTAG/ICV removed, this flag represents a policy to allow SecTAG
     *  retaining.\n
     *  true - SecTAG is retained.
     */
    bool fRetainSecTAG;

    /** true - ICV is retained (allowed only when fRetainSecTAG is true). */
    bool fRetainICV;

    /** true - Ritire the SA after a period of inactivity. */
    bool fRetire;

} SecY_SA_I_t;

/** SA parameters for Bypass/Drop action type */
typedef struct
{
    /** true - enable statistics counting for the associated SA\n
        false - disable statistics counting for the associated SA */
    bool fSAInUse;
} SecY_SA_BD_t;

/** SA parameters for Crypt-Authenticate action type */
typedef struct
{
    /** true - message has length 0\n
        false - message has length > 0 */
    bool fZeroLengthMessage;

    /** The number of bytes (in the range of 0-255) that are authenticated but
        not encrypted (AAD length). */
    uint8_t ConfidentialityOffset;

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
    uint8_t IVMode;

    /** true - append the calculated ICV\n
        false - don't append the calculated ICV */
    bool fICVAppend;

    /** true - enable ICV verification\n
        false - disable ICV verification */
    bool fICVVerify;

    /** true - enable confidentiality protection (AES-GCM/CTR operation)\n
        false - disable confidentiality protection (AES-GMAC operation) */
    bool fConfProtect;
} SecY_SA_CA_t;

/** SA parameters for IPsec Egress action type */
typedef struct
{
    /** true - SA is in use, packets classified for it can be transformed\n
     *  false - SA not in use, packets classified for it can not be transformed
     */
    bool fSAInUse;

    /** true - When nextPN reaching all 1s, it rolls over to value 1 without
      * a sequence number error generated.
      */
    bool fRollOverMode;

    /** true - Encrypt the GHASH output.*/
    bool fEncrAuth;

    /** Algorithm for data encryption.\n
      * 101b - AES-CTR-128\n
      * 111b - AES-CTR-256\n
      */
    uint8_t CryptoAlg;

    /** Enabling IPsec replay check with the mask */
    bool fReplayCheck;

    /** true - enable inserting 4-byte SA-Tag.\n
      * false - disable inserting 4-byte SA-Tag
      */
    bool fIgHdrInsert;

    /** true - If UDP header is present, update length */
    bool fUpdateUDP;

    /** true - Update outer IP header */
    bool fUpdateIP;

    /** true - The NAT-T UDP is present after outer IP header */
    bool fNAT_UDP;

    /** true  - Outer IP header is IPv6
     * false - Outer IP header is IPV4
     */
    bool fOuterIPHdr;

    /** true - enable confidentiality protection\n
      * false - disable confidentiality protection
      */
    bool fConfProtect;

    /** true - enable frame protection,\n
     * false - bypass frame through device
     */
    bool fProtectFrames;
} SecY_SA_IPsec_E_t;

/** SA parameters for IPsec Ingress action type */
typedef struct
{
    /** true - SA is in use, packets classified for it can be transformed\n
     *  false - SA not in use, packets classified for it can not be transformed
     */
    bool fSAInUse;

    /** IPsec padding is not removed */
    bool fRetainPad;

    /** IPsec pad check is enabled */
    bool fPadCheck;

    /** Enabling IPsec replay check with the mask */
    bool fReplayCheck;

    /** true - enable inserting 4-byte SA-Tag.
      * false - disable inserting 4-byte SA-Tag
      */
    bool fIgHdrInsert;

    /** true - enable replay protection\n
      * false - disable replay protection
      */
    bool fReplayProtect;

    /** true - enable confidentiality protection */
    bool fConfProtect;

    /** true - Drop policy for packets with not valid padding */
    bool fPadNotValidDrop;

    /** true - Drop policy for packets with padding that failed length check */
    bool fPadLenFailDrop;

    /** true - update outer IP header, based on information from the decrypted
     * inner IP header.
     */
    bool fUpdateIP;

    /** true - for MPLS-IPsec: copy TTL from the outer MPLS header into the outer
     * IP header TTL, if MPLS TTL value is smaller.
     */
    bool fUpdateTTL;
} SecY_SA_IPsec_I_t;


typedef union {
    /** SA parameters for Egress action type */
    SecY_SA_E_t        Egress;

    /** SA parameters for Ingress action type */
    SecY_SA_I_t        Ingress;

    /** SA parameters for Bypass/Drop action type */
    SecY_SA_BD_t       BypassDrop;

    /** SA parameters for Crypt-Authenticate action type */
    SecY_SA_CA_t       CryptAuth;

    /** SA parameters for IPsec Egress action type */
    SecY_SA_IPsec_E_t  IPsecEgress;

    /** SA parameters for IPsec Ingress action type */
    SecY_SA_IPsec_I_t  IPsecIngress;
} SecY_SA_Params_t;

 /** SecY SA data structure that contains data required to add a new SA. */
typedef struct
{
    /** Size of the transform record (TransformRecord_p) associated with SA
       in 32-bit words */
    unsigned int SA_WordCount;

    /** Pointer to the transform record data */
    uint32_t * TransformRecord_p;

    /** SA parameters */
    SecY_SA_Params_t Params;

    /** SA action type, see SecY_SA_ActionType_t */
    SecY_SA_ActionType_t ActionType;

    /** SA drop type, see SecY_DropType_t */
    SecY_DropType_t DropType;

    /** Destination port */
    SecY_PortType_t DestPort;
} SecY_SA_t;

/** MACsec SecTAG parsing rules,
 *  these rules classify each packet into one of four categories,
 *  see also the SecY_Rules_SA_NonMatch_t data structure:\n
 *  1) Untagged, the packet has no MACsec tag,
 *     i.e. the Ether-type differs from 0x88E5.\n
 *  2) Bad tag, the packet has an invalid MACsec tag\n
 *  3) KaY tag, the packet has a KaY tag. These packets are generated and/or
 *     handled by application software and no MACsec processing is performed
 *     for them by the Classification device except for straight bypass.\n
 *  4) Tagged, the packet has a valid MACsec tag that is not KaY.
 */
typedef struct
{
    /** Compare Ether-type in packet against EtherType value.\n
     *  false - all packets are treated as MACsec\n
     *          (no packets are classified as untagged).\n
     * default is true */
    bool fCompEType;

    /** true - check V bit to be 0\n
        default is true */
    bool fCheckV;

    /** true - check if this is a KaY packet (C and E bits)\n
        default is true */
    bool fCheckKay;

    /** true - check illegal C and E bits combination (C=1 and E=0)\n
        default is true */
    bool fCheckCE;

    /** true - check illegal SC/ES/SCB bits combinations\n
        default is true */
    bool fCheckSC;

    /** true - check if SL (Short Length) field value is out of range\n
        default is true */
    bool fCheckSL;

    /** true - check PN (Packet Number) is non-zero\n
        default is true */
    bool fCheckPN;

    /** true- check if SL (Short Length) field value is consistent with
     * packet length
     */
    bool fCheckSLExt;

    /** Ether-type value used for MACsec tag type comparison\n
      * default is 0x88E5
      */
    uint16_t EtherType;
} SecY_Channel_Rule_SecTAG_t;

/** MTU checking rule for packet post-processing of a Secure Channel
 *  (egress only)
 */
typedef struct
{
    /** Maximum allowed packet size (in bytes) */
    uint16_t PacketMaxByteCount;

    /** Action indication, if a packets is longer:\n
     *  true  - Drop packet (by corrupting the CRC)\n
     *  false - Pass packet on anyway
     */
    bool fOverSizeDrop;
} SecY_SC_Rule_MTUCheck_t;

/** Actions per packet that did not match any SA */
typedef struct
{
    /** Flow action type\n
     *  true  - bypass,\n
     *  false - perform drop action, see SecY_DropType_t
     */
    bool fBypass;

    /** Packet drop type, see SecY_DropType_t */
    SecY_DropType_t DropType;

    /** Ingress only\n
     *  Perform drop action if packet is not from the reserved port
     */
    bool fDropNonReserved;

    /** The destination port type */
    SecY_PortType_t DestPort;
} SecY_Rules_NonSA_t;

/** Device statistics control */
typedef struct
{
    /** true - statistics counters are automatically reset on a read operation\n
        false - no reset on a read operation */
    bool fAutoStatCntrsReset;

    /** Outbound sequence number threshold value (one global for all SA's)\n
        When non-0 the SecY device will generate a notification to indicate
        the threshold event which can be used to start the re-keying
        procedure.\n
        The notification will be generated only if it is requested
        by means of the SecY_Notify_Request() function.\n
        If set to zero then only the sequence number roll-over notification
        will be generated.\n
        @note: This is a global parameter for all the SA's added to
                one SecY device. */
    uint32_t SeqNrThreshold;

    /** Outbound sequence number threshold value for 64-bit packet
        numbering */
    SecY_Stat_Counter_t SeqNrThreshold64;

    /** Threshold for the SA frame counters */
    SecY_Stat_Counter_t SACountFrameThr;
    /** Threshold for the SecY frame counters */
    SecY_Stat_Counter_t SecYCountFrameThr;
    /** Threshold for the IFC frame counters */
    SecY_Stat_Counter_t IFCCountFrameThr;
    /** Threshold for the IDC1 frame counters */
    SecY_Stat_Counter_t IFC1CountFrameThr;
    /** Threshold for the RxCAM frame counters */
    SecY_Stat_Counter_t RxCAMCountFrameThr;

    /** Threshold for the SA octet counters */
    SecY_Stat_Counter_t SACountOctetThr;
    /** Threshold for the IFC octet counters */
    SecY_Stat_Counter_t IFCCountOctetThr;
    /** Threshold for the IFC1 octet counters */
    SecY_Stat_Counter_t IFC1CountOctetThr;

    /** Counter increment enable control */
    uint8_t CountIncDisCtrl;
} SecY_Statistics_Control_t;

/** SecY settings */
typedef struct
{
    /** Outbound sequence number threshold value (one global for all SA's)
     *  When non-0 the SecY device will generate a notification to indicate
     *  the threshold event which can be used to start the re-keying procedure.
     *  The notification will be generated only if it is requested
     *  by means of the SecY_Notify_Request() function.\n
     *  If set to zero then only the sequence number roll-over notification
     *  will be generated.\n
     *  @note This is a global parameter for all the SA's added to
     *        one SecY device.
     */
    uint32_t SeqNrThreshold;

    /** Outbound sequence number threshold value for low-64-bit packet
        numbering */
    uint32_t SeqNrThreshold64Lo;
    /** Outbound sequence number threshold value for high-64-bit packet
        numbering */
    uint32_t SeqNrThreshold64Hi;

    /** Threshold for the SA frame counters low-32bits */
    uint32_t SACountFrameThrLo;
    /** Threshold for the SA frame counters high-32bits */
    uint32_t SACountFrameThrHi;
    /** Threshold for the SecY frame counters low-32bits  */
    uint32_t SecYCountFrameThrLo;
    /** Threshold for the SecY frame counters high-32bits  */
    uint32_t SecYCountFrameThrHi;
    /** Threshold for the IFC frame counters low-32bits */
    uint32_t IFCCountFrameThrLo;
    /** Threshold for the IFC frame counters high-32bits */
    uint32_t IFCCountFrameThrHi;
    /** Threshold for the IFC1 frame counters low-32bits */
    uint32_t IFC1CountFrameThrLo;
    /** Threshold for the IFC1 frame counters high-32bits */
    uint32_t IFC1CountFrameThrHi;
    /** Threshold for the RxCAM frame counters low-32bits */
    uint32_t RxCAMCountFrameThrLo;
    /** Threshold for the RxCAM frame counters high-32bits */
    uint32_t RxCAMCountFrameThrHi;

    /** Threshold for the SA octet counters low-32bits */
    uint32_t SACountOctetThrLo;
    /** Threshold for the SA octet counters high-32bits */
    uint32_t SACountOctetThrHi;
    /** Threshold for the IFC octet counters low-32bits */
    uint32_t IFCCountOctetThrLo;
    /** Threshold for the IFC octet counters high-32bits */
    uint32_t IFCCountOctetThrHi;
    /** Threshold for the IFC1 octet counters low-32bits */
    uint32_t IFC1CountOctetThrLo;
    /** Threshold for the IFC1 octet counters high-32bits */
    uint32_t IFC1CountOctetThrHi;

    /** Control of automatic EOP insertion, timeout value */
    unsigned int EOPTimeoutVal;
    /** Control of automatic EOP insertion, bit mask to specify channels  */
    SecY_Ch_Mask_t EOPTimeoutCtrl;

    /** Threshold for the correctable ECC error counters.*/
    unsigned int ECCCorrectableThr;
    /** Threshold for the uncorrectable ECC error counters.*/
    unsigned int ECCUncorrectableThr;

    /** Counter increment enable control */
    uint8_t CountIncDisCtrl;

    /** Enable the Crypt-authenticate operation (all vPorts) */
    bool fFlowCyptAuth;

    /** Non-match flows (drop, bypass) control (all vPorts) */
    SecY_Rules_NonSA_t  DropBypass;

    /** Drop control packets when no valid vPort exists */
    bool fDropControlPkts;

    /** Presacle for SA retire timer, set to 0 to disable the feature */
    unsigned int SARetireTimerPrescale;

    /** Timeout value for SA retire timer */
    unsigned int SARetireTimeout;

    /** Specify the maximum number of channels to be used by the driver.
     * If set to zero, use the maximum supported by the hardware */
    unsigned int MaxChannelCount;

    /** Specify the maximum number of vPorts to be used by the driver.
     * If set to zero, use the maximum supported by the hardware */
    unsigned int MaxvPortCount;

    /** Specify the maximum number of SAs to be used by the driver.
     *  If set to zero, use the maximum supported by the hardware */
    unsigned int MaxSACount;

    /** Specify the maximum number of SCs to be used by the driver.
     *  If set to zero, use the maximum supported by the hardware */
    unsigned int MaxSCCount;

    /** Ingress Header type - Global for device - Applicable only for IPsec */
    uint16_t IngressHdrEtype;
} SecY_Settings_t;

/** SA statistics */
typedef struct
{
    /** Packet counters - Encrypted Protected */
    SecY_Stat_Counter_t OutPktsEncryptedProtected;
    /** Packet counters - TooLong */
    SecY_Stat_Counter_t OutPktsTooLong;
    /** Packet counters - SA Not In Use */
    SecY_Stat_Counter_t OutPktsSANotInUse;

    /** Octet counters - Encrypted Protected */
    SecY_Stat_Counter_t OutOctetsEncryptedProtected;
} SecY_SA_Stat_E_t;

/** SA counters */
typedef struct
{
    /** Packet counters - Unchecked */
    SecY_Stat_Counter_t InPktsUnchecked;
    /** Packet counters - Delayed */
    SecY_Stat_Counter_t InPktsDelayed;
    /** Packet counters - Late */
    SecY_Stat_Counter_t InPktsLate;
    /** Packet counters - OK */
    SecY_Stat_Counter_t InPktsOK;
    /** Packet counters - Invalid */
    SecY_Stat_Counter_t InPktsInvalid;
    /** Packet counters - Not Valid */
    SecY_Stat_Counter_t InPktsNotValid;
    /** Packet counters - Not Using SA */
    SecY_Stat_Counter_t InPktsNotUsingSA;
    /** Packet counters - Unused SA */
    SecY_Stat_Counter_t InPktsUnusedSA;

    /** Octet counters - Decrypted */
    SecY_Stat_Counter_t InOctetsDecrypted;
    /** Octet counters - Validated */
    SecY_Stat_Counter_t InOctetsValidated;
} SecY_SA_Stat_I_t;


/** SA counters for IPSec Ingress */
typedef struct
{
    /** Packet counters - Pad check for NH byte and pad pattern failed */
    SecY_Stat_Counter_t InPktsPadNotValid;

    /** Packet counters - Replayed */
    SecY_Stat_Counter_t InPktsReplayed;

    /** Packet counters - Late */
    SecY_Stat_Counter_t InPktsLate;

    /** Packet counters - OK */
    SecY_Stat_Counter_t InPktsOK;

    /** Packet counters - Pad length consistency check failed */
    SecY_Stat_Counter_t InPktsPadLenFail;

    /** Packet counters - Not Valid */
    SecY_Stat_Counter_t InPktsNotValid;

    /** Packet counters - Not Using SA */
    SecY_Stat_Counter_t InPktsNotUsingSA;

    /** Packet counters - Packet passed authentication check but has
      * dummy in the next header in its padding
      */
    SecY_Stat_Counter_t InPktsPadDummy;

    /** Octet counters - Decrypted */
    SecY_Stat_Counter_t InOctetsDecrypted;

    /** Octet counters - Validated */
    SecY_Stat_Counter_t InOctetsValidated;
} SecY_SA_Stat_IPsec_I_t;


/** Union for SA Statistics counters for both Ingress & Egress */
typedef union
{
    /** SA Statistics for Egress */
    SecY_SA_Stat_E_t Egress;

    /** SA Statistics for Ingress */
    SecY_SA_Stat_I_t Ingress;

    /** SA Statistics for IPsec Ingress */
    SecY_SA_Stat_IPsec_I_t IPsecIngress;
} SecY_SA_Stat_t;

/** SecY MACsec Egress statistics */
typedef struct
{
    /** Packet counters - Transform Error */
    SecY_Stat_Counter_t OutPktsTransformError;

    /** Packet counters - Control */
    SecY_Stat_Counter_t OutPktsControl;

    /** Packet counters - Untagged */
    SecY_Stat_Counter_t OutPktsUntagged;

    /** Packet counters - SA not in use */
    SecY_Stat_Counter_t OutPktsSANotInUse;
} SecY_SecY_Stat_E_t;


/** SecY IPsec Egress statistics */
typedef struct
{
    /** Packet counters - Transform Error */
    SecY_Stat_Counter_t OutPktsTransformError;

    /** Packet counters - Untagged */
    SecY_Stat_Counter_t OutPktsUntagged;

    /** Packet counters - Not Using SA */
    SecY_Stat_Counter_t OutPktsSANotInUse;
} SecY_SecY_Stat_IPsec_E_t;


/** SecY MACsec Ingress counters */
typedef struct
{
    /** Packet counters - Transform Error */
    SecY_Stat_Counter_t InPktsTransformError;
    /** Packet counters - Control */
    SecY_Stat_Counter_t InPktsControl;
    /** Packet counters - Untagged */
    SecY_Stat_Counter_t InPktsUntagged;
    /** Packet counters - No Tag */
    SecY_Stat_Counter_t InPktsNoTag;
    /** Packet counters - Bad Tag */
    SecY_Stat_Counter_t InPktsBadTag;
    /** Packet counters - No SCI */
    SecY_Stat_Counter_t InPktsNoSCI;
    /** Packet counters - Unknown SCI */
    SecY_Stat_Counter_t InPktsUnknownSCI;
    /** Packet counters - Tagged Ctrl */
    SecY_Stat_Counter_t InPktsTaggedCtrl;
    /** Packet counters - SA not in use */
    SecY_Stat_Counter_t InPktsSANotInUse;
} SecY_SecY_Stat_I_t;


/** SecY IPsec Ingress counters */
typedef struct
{
    /** Packet counters - Transform Error */
    SecY_Stat_Counter_t InPktsTransformError;

    /** Packet counters - Counts packets that are late at SOP */
    SecY_Stat_Counter_t InPktsLateHdr;

    /** Packet counters - Counts packet that have mismatch of outer IP
     *  header and inner IP header type.
     */
    SecY_Stat_Counter_t InPktsIPMismatch;

    /** Packet counters - Not Using SA */
    SecY_Stat_Counter_t InPktsSANotInUse;
} SecY_SecY_Stat_IPsec_I_t;

/** Union for SecY Statistics counters for both Ingress & Egress */
typedef union
{
    /** Egress SecY Statistics */
    SecY_SecY_Stat_E_t Egress;

    /** Ingress SecY Statistics */
    SecY_SecY_Stat_I_t Ingress;

    /** IPsec Egress SecY Statistics */
    SecY_SecY_Stat_IPsec_E_t IPsecEgress;

    /** IPsec Ingress SecY Statistics */
    SecY_SecY_Stat_IPsec_I_t IPsecIngress;
} SecY_SecY_Stat_t;

/** IFC (interface) statistics */
typedef struct
{
    /** Packet counters - Unicast Uncontrolled */
    SecY_Stat_Counter_t OutPktsUnicastUncontrolled;
    /** Packet counters - Multicast Uncontrolled */
    SecY_Stat_Counter_t OutPktsMulticastUncontrolled;
    /** Packet counters - Broadcast Uncontrolled */
    SecY_Stat_Counter_t OutPktsBroadcastUncontrolled;

    /** Packet counters - Unicast Controlled */
    SecY_Stat_Counter_t OutPktsUnicastControlled;
    /** Packet counters - Multicast Controlled */
    SecY_Stat_Counter_t OutPktsMulticastControlled;
    /** Packet counters - Broadcast Controlled */
    SecY_Stat_Counter_t OutPktsBroadcastControlled;

    /** Octet counters - Uncontrolled */
    SecY_Stat_Counter_t OutOctetsUncontrolled;
    /** Octet counters - Controlled */
    SecY_Stat_Counter_t OutOctetsControlled;
    /** Octet counters - Common */
    SecY_Stat_Counter_t OutOctetsCommon;
} SecY_Ifc_Stat_E_t;

/** Statistics counters*/
typedef struct
{
    /** Packet counters - Unicast uncontrolled */
    SecY_Stat_Counter_t InPktsUnicastUncontrolled;
    /** Packet counters - Multicast Uncontrolled */
    SecY_Stat_Counter_t InPktsMulticastUncontrolled;
    /** Packet counters - Broadcast Uncontrolled */
    SecY_Stat_Counter_t InPktsBroadcastUncontrolled;

    /** Packet counters - Unicast Controlled */
    SecY_Stat_Counter_t InPktsUnicastControlled;
    /** Packet counters - Multicast Controlled */
    SecY_Stat_Counter_t InPktsMulticastControlled;
    /** Packet counters - Broadcast Controlled */
    SecY_Stat_Counter_t InPktsBroadcastControlled;

    /** Octet counters - Uncontrolled */
    SecY_Stat_Counter_t InOctetsUncontrolled;
    /** Octet counters - Controlled */
    SecY_Stat_Counter_t InOctetsControlled;
} SecY_Ifc_Stat_I_t;

/** Union for Ifc Statistics counters for both Ingress & Egress */
typedef union
{
    /** Ifc Statistics counters for Egress */
    SecY_Ifc_Stat_E_t Egress;
    /** Ifc Statistics counters for Ingress */
    SecY_Ifc_Stat_I_t Ingress;
} SecY_Ifc_Stat_t;

/** RxCAM statistics (ingress only) */
typedef struct
{
    /** Packet counter */
    SecY_Stat_Counter_t CAMHit;
} SecY_RxCAM_Stat_t;


/** Packet Number Threshold configuration */
typedef struct
{
    /** Outbound sequence number threshold value
     * When non-0 the SecY device will generate a notification to indicate
     * the threshold event which can be used to start the re-keying procedure.
     * The notification will be generated only if it is requested
     * by means of the SecY_Notify_Request() function.
     * If set to zero then only the sequence number roll-over notification
     * will be generated.\n
     * @note This is a global parameter for all the SA's added to
     *       one SecY device.
     */
    uint32_t SeqNrThreshold;

    /** Outbound sequence number threshold value for 64-bit packet numbering */
    SecY_Stat_Counter_t SeqNrThreshold64;
} SecY_Channel_StatControl_t;


/** Channel configuration parameters */
typedef struct
{
    /** Channel identifier */
    unsigned int ChannelId;

    /** Indication if the low latency bypass must be enabled (true) for channel
     *  or the MACsec mode (false)
     */
    bool fLowLatencyBypass;

    /** set channel to should-secure */
    bool fShouldSecure;

    /** Indication if the channel-specific latency must be enabled (true),
     *  this can be enabled for MACsec channels only with
     *  fLowLatencyBypass set to false, otherwise this setting will be ignored
     */
    bool fLatencyEnable;

    /** Channel-specific latency
     *  this can be set for MACsec channels only with
     *  fLowLatencyBypass set to false, otherwise this setting will be ignored
     */
    uint16_t Latency;

    /** Burst limit. Set to 0 to disable burst limiting, set to value between
     *  4 and 15 to insert a dummy cycle after BurstLimit transfers */
    uint8_t BurstLimit;

    /** Packet Number Threshold configuration */
    SecY_Channel_StatControl_t StatCtrl;

    /** Mode for packet sequence number threshold comparison:\n
     *     false - comparison is greater or equal,\n
     *     true  - comparison is strictly equal.
     */
    bool fPktNumThrMode;

    /** EtherType field for MACsec to be inserted in the SecTAG for outbound
     *  MACsec packet processing. Default (reset) value is 0x88E5.
     */
    uint16_t EtherType;

    /** MACsec SecTAG parsing rules, see above for details */
    SecY_Channel_Rule_SecTAG_t RuleSecTAG;

    /** true - for IPsec mode */
    bool fIPsec;

    /** Read-only, Per-channel bypass status */
    bool fBypassStatus;

    /** bypass mode */
    bool fBypassMode;

    /** Enable IPsec pad Next Header field check */
    bool  fPad_NH_Check;

    /** Enable IPsec pad length field check */
    bool  fPadLenCheck;

    /** Enable IPsec pad pattern field check */
    bool  fPadPatternCheck;

    /** Enable IPsec dummy packet detection */
    bool  fPadDummyCheck;
} SecY_Channel_t;

/** Channels */
typedef struct
{
    /** Number of channels to configure.
     *  Set to 0 if no channels configuration is required.
     */
    unsigned int ChannelCount;

    /** Array of channel configuration parameters for channels to re-configure */
    SecY_Channel_t * Channel_p;
} SecY_ChannelParams_t;

/** SecY Channel configuration parameters */
typedef struct
{
    /** Channel configuration parameters */
    SecY_ChannelParams_t Params;
} SecY_ChannelConf_t;


/** Control of automatic EOP insertion. */
typedef struct
{
    /** Timeout value */
    unsigned int EOPTimeoutVal;

    /** bit mask to specify channels */
    SecY_Ch_Mask_t EOPTimeoutCtrl;
} SecY_EOPConf_t;


/** ECC configuration parameters */
typedef struct
{
    /** Correctable error threshold */
    unsigned int ECCCorrectableThr;
    /** Uncorrectable error threshold. */
    unsigned int ECCUncorrectableThr;
} SecY_ECCConf_t;

/** SA retire configuration parameters */
typedef struct
{
    /** Timer prescale value, set to 0 to disable the feature  */
    unsigned int TimerPrescale;
    /** SA Timeout value */
    unsigned int SATimeout;
} SecY_SARetireConf_t;

/** Device control */
typedef struct
{
    /** Statistics settings. Set to NULL if no update is required */
    SecY_Statistics_Control_t * StatControl_p;

    /** Channel settings, specified per channel for a number of channels.
     *  Set to NULL if no update is required.
     */
    SecY_ChannelConf_t * ChConf_p;

    /** automatic EOP insertion, Set to NULL if no update is required */
    SecY_EOPConf_t * EOPConf_p;

    /** ECC settings. Set to NULL if no update is required */
    SecY_ECCConf_t * ECCConf_p;

    /** SA retire settings, Set to NULL if no update is required */
    SecY_SARetireConf_t * SARetireConf_p;
} SecY_Device_Params_t;


/** Debug registers for packet classification. These can be read to
    find out how the last packet was classified. For details see the
    corresponding register definitions in Operations and Programmer's
    Guide.*/
typedef struct
{
    /** Parsed destination address low*/
    uint32_t ParsedDALo;
    /** Parsed destination address high*/
    uint32_t ParsedDAHi;
    /** Parsed source address low*/
    uint32_t ParsedSALo;
    /** Parsed source address high + ether type*/
    uint32_t ParsedSAHi;
    /** Parsed source SecTAG low*/
    uint32_t ParsedSecTAGLo;
    /** Parsed source SecTAG high*/
    uint32_t ParsedSecTAGHi;
    /** Parsed source SCI low*/
    uint32_t ParsedSCILo;
    /** Parsed source SCI high*/
    uint32_t ParsedSCIHi;
    /** various fields related to SecTAG */
    uint32_t SecTAGDebug;
    /** SCI for RxCAM lookup low*/
    uint32_t RxCAMSCILo;
    /** SCI for RxCAM lookup high*/
    uint32_t RxCAMSCIHi;
    /** various fields related to RxCAM lookup*/
    uint32_t ParserInDebug;

    /** VLAN detector debug registers */
    uint32_t SAMPP_Dbg;
} SecY_PktProcessDebug_t;

/** Number of ECC status counters. */
#define SECY_ECC_NOF_STATUS_COUNTERS 19

/** ECC error status. */
typedef struct
{
    /** Array of status records per status counter */
    struct  {
        /** Number of correctable errors. */
        unsigned int CorrectableCount;

        /** Number of uncorrectable errors.*/
        unsigned int UncorrectableCount;

        /** Threshold for correctable errors exceeded */
        bool fCorrectableThr;

        /** Threshold for uncorrectable errors exceeded */
        bool fUncorrectableThr;
    } Counters[SECY_ECC_NOF_STATUS_COUNTERS];
} SecY_ECCStatus_t;


/** Overall device status */
typedef struct
{
    /** may be NULL if parser debug info is not desired. */
    SecY_PktProcessDebug_t *PktProcessDebug_p;

    /** may be NULL if ECC status is not desired. */
    SecY_ECCStatus_t *ECCStatus_p;
} SecY_DeviceStatus_t;

#endif /* API_SECY_TYPES_H_ */


/* end of file api_secy_types.h */
