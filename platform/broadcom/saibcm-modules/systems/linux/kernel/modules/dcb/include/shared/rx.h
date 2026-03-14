/*
 * $Id: rx.h,v 1.35 Broadcom SDK $
 *
 * $Copyright: 2017-2025 Broadcom Inc. All rights reserved.
 * 
 * Permission is granted to use, copy, modify and/or distribute this
 * software under either one of the licenses below.
 * 
 * License Option 1: GPL
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 * 
 * 
 * License Option 2: Broadcom Open Network Switch APIs (OpenNSA) license
 * 
 * This software is governed by the Broadcom Open Network Switch APIs license:
 * https://www.broadcom.com/products/ethernet-connectivity/software/opennsa $
 * 
 * 
 *
 * File:        rx.h
 * Purpose:     Packet Receive Reason Types shared between BCM and SOC layers
 */

#ifndef   _SHR_RX_H_
#define   _SHR_RX_H_

/*
 * PKT RX Packet Reasons; reason CPU received the packet.
 * Notes: It is possible no reasons are set (directed to CPU from ARL
 *        for example), or multiple reasons may be set.
 */

typedef enum _shr_rx_reason_e {
    _SHR_RX_INVALID                     = 0,
    _SHR_RX_ARP                         = 1,   /* ARP Packet                          */
    _SHR_RX_BPDU                        = 2,   /* BPDU Packet                         */
    _SHR_RX_BROADCAST                   = 3,   /* Broadcast packet                    */
    _SHR_RX_CLASS_BASED_MOVE            = 4,   /* Class-based move prevented          */
    _SHR_RX_CLASS_TAG_PACKETS           = 5,   /* Higig Header with PPD=1             */
    _SHR_RX_CONTROL                     = 6,   /* Control frame or reserved addr      */
    _SHR_RX_CPU_LEARN                   = 7,   /* CPU Learned (or VLAN not found      */
                                               /* on Strata devices)                  */
    _SHR_RX_DEST_LOOKUP_FAIL            = 8,   /* Destination lookup fail (or L3      */
                                               /* station move on Strata devices)     */
    _SHR_RX_DHCP                        = 9,   /* DHCP packets                        */
    _SHR_RX_DOS_ATTACK                  = 10,  /* DOS Attack Packet                   */
    _SHR_RX_E2E_HOL_IBP                 = 11,  /* E2E HOL or IBP Packet               */
    _SHR_RX_ENCAP_HIGIG_ERROR           = 12,  /* Non-HG packets received on an       */
                                               /* E-HG port                           */
    _SHR_RX_FILTER_MATCH                = 13,  /* Filter Match                        */
    _SHR_RX_GRE_CHECKSUM                = 14,  /* GRE checksum                        */
    _SHR_RX_GRE_SOURCE_ROUTE            = 15,  /* GRE source routing                  */
    _SHR_RX_HIGIG_CONTROL               = 16,  /* Higig Packet with Control Opcode    */
    _SHR_RX_HIGIG_HDR_ERROR             = 17,  /* Higig+ header errors                */
    _SHR_RX_ICMP_REDIRECT               = 18,  /* ICMP Recirect                       */
    _SHR_RX_IGMP                        = 19,  /* IGMP packet                         */
    _SHR_RX_INGRESS_FILTER              = 20,  /* Ingress Filter (VLAN membership)    */
    _SHR_RX_IP                          = 21,  /* IP packet                           */
    _SHR_RX_IPFIX_RATE_VIOLATION        = 22,  /* IPFIX flows exceed metering         */
    _SHR_RX_IP_MCAST_MISS               = 23,  /* IPMC miss                           */
    _SHR_RX_IP_OPTION_VERSION           = 24,  /* IP options present or IP ver != 4   */
    _SHR_RX_IPMC                        = 25,  /* Class D IP multicast packet         */
    _SHR_RX_IPMC_RSVD                   = 26,  /* IPMC Reserved Packet                */
    _SHR_RX_L2_CPU                      = 27,  /* L2_TABLE - copy to CPU (on 5690)    */
    _SHR_RX_L2_DEST_MISS                = 28,  /* L2 destination lookup failure       */
    _SHR_RX_L2_LEARN_LIMIT              = 29,  /* L2 Learn Limit                      */
    _SHR_RX_L2_MOVE                     = 30,  /* L2 Station Movement                 */
    _SHR_RX_L2_MTU_FAIL                 = 31,  /* L2 MTU check fail                   */
    _SHR_RX_L2_NON_UNICAST_MISS         = 32,  /* L2 Non-Unicast lookup miss          */
    _SHR_RX_L2_SOURCE_MISS              = 33,  /* L2 source lookup failure            */
    _SHR_RX_L3_ADDR_BIND_FAIL           = 34,  /* MAC to IP bind check failure        */
    _SHR_RX_L3_DEST_MISS                = 35,  /* L3 DIP Miss                         */
    _SHR_RX_L3_HEADER_ERROR             = 36,  /* L3 header - IP options,             */
    _SHR_RX_L3_MTU_FAIL                 = 37,  /* L3 MTU check fail                   */
    _SHR_RX_L3_SLOW_PATH                = 38,  /* L3 slow path processed pkt.         */
    _SHR_RX_L3_SOURCE_MISS              = 39,  /* L3 SIP Miss                         */
    _SHR_RX_L3_SOUCE_MOVE               = 40,  /* L3 Station Movement                 */
    _SHR_RX_MARTIAN_ADDR                = 41,  /* Pkt. with Martian address           */
    _SHR_RX_MCAST_IDX_ERROR             = 42,  /* Multicast index error               */
    _SHR_RX_MCAST_MISS                  = 43,  /* MC miss                             */
    _SHR_RX_MIM_SERVICE_ERROR           = 44,  /* MiM terminated unicast packets      */
                                               /* that do not have a valid I-SID      */
    _SHR_RX_MPLS_CTRL_WORD_ERROR        = 45,  /* MPLS Control Word type is not 0     */
    _SHR_RX_MPLS_ERROR                  = 46,  /* MPLS error                          */
    _SHR_RX_MPLS_INVALID_ACTION         = 47,  /* MPLS Invalid Action                 */
    _SHR_RX_MPLS_INVALID_PAYLOAD        = 48,  /* MPLS Invalid Payload                */
    _SHR_RX_MPLS_LABEL_MISS             = 49,  /* MPLS table miss                     */
    _SHR_RX_MPLS_SEQUENCE_NUMBER        = 50,  /* MPLS Sequence number                */
    _SHR_RX_MPLS_TTL                    = 51,  /* MPLS TTL                            */
    _SHR_RX_MULTICAST                   = 52,  /* Multicast packet                    */
    _SHR_RX_NHOP                        = 53,  /* Copy to CPU from NH Idx Tbl         */
    _SHR_RX_OAM_ERROR                   = 54,  /* OAM packets to CPU for error cases  */
    _SHR_RX_OAM_SLOW_PATH               = 55,  /* OAM packets to CPU - slowpath       */
                                               /* process                             */
    _SHR_RX_OAM_LMDM                    = 56,  /* OAM LMM/LMR, DMM/DMR packets to CPU */
    _SHR_RX_PARITY_ERROR                = 57,  /* Parity error on IP tables           */
    _SHR_RX_PROTOCOL                    = 58,  /* Protocol Packet                     */
    _SHR_RX_SAMPLE_DEST                 = 59,  /* Egress  sFlow sampled               */
    _SHR_RX_SAMPLE_SOURCE               = 60,  /* Ingress sFlow sampled               */
    _SHR_RX_SHARED_VLAN_MISMATCH        = 61,  /* Private VLAN Mismatch               */
    _SHR_RX_SOURCE_ROUTE                = 62,  /* Source routing bit set              */
    _SHR_RX_TIME_STAMP                  = 63,  /* Network time sync packet            */
    _SHR_RX_TTL                         = 64,  /* TTL <= 0 or TTL < IPMC threshold    */
    _SHR_RX_TTL1                        = 65,  /* L3UC or IPMC packet with TTL        */
                                               /* equal to 1                          */
    _SHR_RX_TUNNEL_ERROR                = 66,  /* Tunnel error trap                   */
    _SHR_RX_UDP_CHECKSUM                = 67,  /* UDP checksum                        */
    _SHR_RX_UNKNOWN_VLAN                = 68,  /* Unknown VLAN; VID = 0xfff;          */
                                               /* CPU Learn bit (on 5690 devices)     */
    _SHR_RX_URPF_FAIL                   = 69,  /* URPF Check Failed                   */
    _SHR_RX_VC_LABEL_MISS               = 70,  /* VPLS table miss                     */
    _SHR_RX_VLAN_FILTER_MATCH           = 71,  /* VLAN Filter Match                   */
    _SHR_RX_WLAN_CLIENT_ERROR           = 72,  /* ROC error packets to the CPU        */
    _SHR_RX_WLAN_SLOW_PATH              = 73,  /* WLAN packets slowpath to the CPU    */
    _SHR_RX_WLAN_DOT1X_DROP             = 74,  /* WLAN client is unauthenticated      */
    _SHR_RX_EXCEPTION_FLOOD             = 75,  /* Exception processing or flooding    */
    _SHR_RX_TIMESYNC                    = 76,  /* Time Sync protocol packet           */
    _SHR_RX_EAV_DATA                    = 77,  /* Ethernet AV data packet             */
    _SHR_RX_SAME_PORT_BRIDGE            = 78,  /* Hairpin or Same port                */
                                               /* switching/bridging                  */
    _SHR_RX_SPLIT_HORIZON               = 79,  /* Basic bridging or VPLS Split        */
                                               /* horizon                             */
    _SHR_RX_L4_ERROR                    = 80,  /* TCP/UDP header or port number       */
                                               /* errors                              */
    _SHR_RX_STP                         = 81,  /* STP Ingress or Egress checks        */
    _SHR_RX_EGRESS_FILTER_REDIRECT      = 82,  /* Vlan egress filter redirect         */
    _SHR_RX_FILTER_REDIRECT             = 83,  /* Field processor redirect            */
    _SHR_RX_LOOPBACK                    = 84,  /* Loopbacked                          */
    _SHR_RX_VLAN_TRANSLATE              = 85,  /* VLAN translation table missed when  */
                                               /* it is expected to hit               */
    _SHR_RX_MMRP                        = 86,  /* Packet of type MMRP                 */
    _SHR_RX_SRP                         = 87,  /* Packet of type SRP                  */
    _SHR_RX_TUNNEL_CONTROL              = 88,  /* Tunnel control packet               */
    _SHR_RX_L2_MARKED                   = 89,  /* L2 table marked                     */
    _SHR_RX_WLAN_SLOWPATH_KEEPALIVE     = 90,  /* WLAN slowpath to the CPU,           */
                                               /* otherwise dropped                   */
    _SHR_RX_STATION                     = 91,  /* My Station packet to CPU            */
    _SHR_RX_NIV                         = 92,  /* NIV packet                          */
    _SHR_RX_NIV_PRIO_DROP               = 93,  /* NIV packet, priority drop           */
    _SHR_RX_NIV_INTERFACE_MISS          = 94,  /* NIV packet, interface miss          */
    _SHR_RX_NIV_RPF_FAIL                = 95,  /* NIV packet, RPF failed              */
    _SHR_RX_NIV_TAG_INVALID             = 96,  /* NIV packet, invalid tag             */
    _SHR_RX_NIV_TAG_DROP                = 97,  /* NIV packet, tag drop                */
    _SHR_RX_NIV_UNTAG_DROP              = 98,  /* NIV packet, untagged drop           */
    _SHR_RX_TRILL                       = 99,  /* TRILL packet                        */
    _SHR_RX_TRILL_INVALID               = 100, /* TRILL packet, header error          */
    _SHR_RX_TRILL_MISS                  = 101, /* TRILL packet, lookup miss           */
    _SHR_RX_TRILL_RPF_FAIL              = 102, /* TRILL packet, RPF check failed      */
    _SHR_RX_TRILL_SLOWPATH              = 103, /* TRILL packet, slowpath to CPU       */
    _SHR_RX_TRILL_CORE_IS_IS            = 104, /* TRILL packet, Core IS-IS            */
    _SHR_RX_TRILL_TTL                   = 105, /* TRILL packet, TTL check failed      */
    _SHR_RX_BFD_SLOWPATH                = 106, /* The BFD packet is being fwd to the  */
                                               /* local uC for processing             */
    _SHR_RX_BFD                         = 107, /* BFD Error                           */
    _SHR_RX_MIRROR                      = 108, /* Mirror packet                       */
    _SHR_RX_REGEX_ACTION                = 109, /* Flow tracker                        */
    _SHR_RX_REGEX_MATCH                 = 110, /* Signature Match                     */
    _SHR_RX_FAILOVER_DROP               = 111, /* Protection drop data                */
    _SHR_RX_WLAN_TUNNEL_ERROR           = 112, /* WLAN shim header error to CPU       */
    _SHR_RX_CONGESTION_CNM_PROXY        = 113, /* Congestion CNM Proxy                */
    _SHR_RX_CONGESTION_CNM_PROXY_ERROR  = 114, /* Congestion CNM Proxy Error          */
    _SHR_RX_CONGESTION_CNM              = 115, /* Congestion CNM Internal Packet      */
    _SHR_RX_MPLS_UNKNOWN_ACH            = 116, /* MPLS Unknown ACH                    */
    _SHR_RX_MPLS_LOOKUPS_EXCEEDED       = 117, /* MPLS out of lookups                 */
    _SHR_RX_MPLS_RESERVED_ENTROPY_LABEL = 118, /* MPLS Entropy label in unallowed     */
                                               /* range                               */
    _SHR_RX_MPLS_ILLEGAL_RESERVED_LABEL = 119, /* MPLS illegal reserved label         */
    _SHR_RX_MPLS_ROUTER_ALERT_LABEL     = 120, /* MPLS alert label                    */
    _SHR_RX_NIV_PRUNE                   = 121, /* NIV access port pruning (dst = src) */
    _SHR_RX_VIRTUAL_PORT_PRUNE          = 122, /* SVP == DVP                          */
    _SHR_RX_NON_UNICAST_DROP            = 123, /* Explicit multicast packet drop      */
    _SHR_RX_TRILL_PACKET_PORT_MISMATCH  = 124, /* TRILL packet vs Rbridge port        */
                                               /* conflict                            */
    _SHR_RX_WLAN_CLIENT_MOVE            = 125, /* WLAN client moved                   */
    _SHR_RX_WLAN_SOURCE_PORT_MISS       = 126, /* WLAN SVP miss                       */
    _SHR_RX_WLAN_CLIENT_SOURCE_MISS     = 127, /* WLAN client database SA miss        */
    _SHR_RX_WLAN_CLIENT_DEST_MISS       = 128, /* WLAN client database DA miss        */
    _SHR_RX_WLAN_MTU                    = 129, /* WLAN MTU error                      */
    _SHR_RX_TRILL_NAME                  = 130, /* TRILL packet, Name check failed     */
    _SHR_RX_L2GRE_SIP_MISS              = 131, /* L2 GRE SIP miss                     */
    _SHR_RX_L2GRE_VPN_ID_MISS           = 132, /* L2 GRE VPN id miss                  */
    _SHR_RX_TIMESYNC_UNKNOWN_VERSION    = 133, /* Unknown version of IEEE1588         */
    _SHR_RX_BFD_ERROR                   = 134, /* BFD ERROR                           */
    _SHR_RX_BFD_UNKNOWN_VERSION         = 135, /* BFD UNKNOWN VERSION                 */
    _SHR_RX_BFD_INVALID_VERSION         = 136, /* BFD INVALID VERSION                 */
    _SHR_RX_BFD_LOOKUP_FAILURE          = 137, /* BFD LOOKUP FAILURE                  */
    _SHR_RX_BFD_INVALID_PACKET          = 138, /* BFD INVALID PACKET                  */
    _SHR_RX_VXLAN_SIP_MISS              = 139, /* Vxlan SIP miss                      */
    _SHR_RX_VXLAN_VPN_ID_MISS           = 140, /* Vxlan VPN id miss                   */
    _SHR_RX_FCOE_ZONE_CHECK_FAIL        = 141, /* Fcoe zone check failed              */
    _SHR_RX_IPMC_INTERFACE_MISMATCH     = 142, /* IPMC input interface check failed   */
    _SHR_RX_NAT                         = 143, /* NAT                                 */
    _SHR_RX_TCP_UDP_NAT_MISS            = 144, /* TCP/UDP packet NAT lookup miss      */
    _SHR_RX_ICMP_NAT_MISS               = 145, /* ICMP packet NAT lookup miss         */
    _SHR_RX_NAT_FRAGMENT                = 146, /* NAT lookup on fragmented packet     */
    _SHR_RX_NAT_MISS                    = 147, /* Non TCP/UDP/ICMP packet NAT lookup  */
                                               /* miss                                */
    _SHR_RX_UNKNOWN_SUBTENTING_PORT     = 148, /* UNKNOWN_SUBTENTING_PORT             */
    _SHR_RX_LLTAG_ABSENT_DROP           = 149, /* LLTAG_ABSENT                        */
    _SHR_RX_LLTAG_PRESENT_DROP          = 150, /* LLTAG_PRESENT                       */
    _SHR_RX_OAM_CCM_SLOWPATH            = 151, /* OAM CCM packet copied to CPU        */
    _SHR_RX_OAM_INCOMPLETE_OPCODE       = 152, /* OAM INCOMPLETE_OPCODE               */
    _SHR_RX_BHH_OAM_PACKET              = 153, /* BHH OAM Packet                      */
    _SHR_RX_RESERVED_0                  = 154, /* Broadcom Reserved                   */
    _SHR_RX_OAM_MPLS_LMDM               = 155, /* MPLS LM/DM (RFC 6374) packet        */
    _SHR_RX_SAT                         = 156, /* OAM SAT pkt                         */
    _SHR_RX_SAMPLE_SOURCE_FLEX          = 157, /* Flexible sampled packets to CPU     */
    _SHR_RX_FLEX_SFLOW                  = 158, /* Flex Sflow?                         */
    _SHR_RX_VXLT_MISS                   = 159, /* VLAN Translation miss packet        */
    _SHR_RX_TUNNEL_DECAP_ECN_ERROR      = 160, /* Tunnel decap ECN error              */
    _SHR_RX_TUNNEL_OBJECT_VALIDATION_FAIL = 161, /* Tunnel Object Validation Fail     */
    _SHR_RX_L3_CPU                      = 162, /* L3 Copy to CPU                      */
    _SHR_RX_TUNNEL_ADAPT_LOOKUP_MISS    = 163, /* Tunnel Adapt Lookup Miss Drop       */
    _SHR_RX_PACKET_FLOW_SELECT_MISS     = 164, /* Packet Flow Select Miss             */
    _SHR_RX_PROTECTION_DATA_DROP        = 165, /* Protection Data Drop                */
    _SHR_RX_PACKET_FLOW_SELECT          = 166, /* Packet Flow Select                  */
    _SHR_RX_OTHER_LOOKUP_MISS           = 167, /* Neither Source or Dest type of      */
                                               /* Lookup Miss                         */
    _SHR_RX_INVALID_TPID                = 168, /* Invalid TPID                        */
    _SHR_RX_MPLS_CONTROL_PACKET         = 169, /* MPLS Control Packet                 */
    _SHR_RX_TUNNEL_TTL_ERROR            = 170, /* Tunnel TTL Error                    */
    _SHR_RX_L2_HEADER_ERROR             = 171, /* L2 header                           */
    _SHR_RX_OTHER_LOOKUP_HIT            = 172, /* Neither Source or Dest type of      */
                                               /* Lookup Hit                          */
    _SHR_RX_L2_SRC_LOOKUP_MISS          = 173, /* L2 Source Lookup Miss               */
    _SHR_RX_L2_SRC_LOOKUP_HIT           = 174, /* L2 Source Lookup Hit                */
    _SHR_RX_L2_DST_LOOKUP_MISS          = 175, /* L2 Dest Lookup Miss                 */
    _SHR_RX_L2_DST_LOOKUP_HIT           = 176, /* L2 Dest Lookup Hit                  */
    _SHR_RX_L3_SRC_ROUTE_LOOKUP_MISS    = 177, /* L3 Source Route Lookup Miss         */
    _SHR_RX_L3_SRC_HOST_LOOKUP_MISS     = 178, /* L3 Source Host Lookup Miss          */
    _SHR_RX_L3_SRC_ROUTE_LOOKUP_HIT     = 179, /* L3 Source Route Lookup Hit          */
    _SHR_RX_L3_SRC_HOST_LOOKUP_HIT      = 180, /* L3 Source Host Lookup Hit           */
    _SHR_RX_L3_DST_ROUTE_LOOKUP_MISS    = 181, /* L3 Dest Route Lookup Miss           */
    _SHR_RX_L3_DST_HOST_LOOKUP_MISS     = 182, /* L3 Dest Host Lookup Miss            */
    _SHR_RX_L3_DST_ROUTE_LOOKUP_HIT     = 183, /* L3 Dest Route Lookup Hit            */
    _SHR_RX_L3_DST_HOST_LOOKUP_HIT      = 184, /* L3 Dest Host Lookup Hit             */
    _SHR_RX_VLAN_TRANSLATE1_LOOKUP1_MISS = 185, /* VLAN Translate1 Lookup1 Miss       */
    _SHR_RX_VLAN_TRANSLATE1_LOOKUP2_MISS = 186, /* VLAN Translate1 Lookup2 Miss       */
    _SHR_RX_MPLS_LOOKUP1_MISS            = 187, /* MPLS Lookup1 Miss                  */
    _SHR_RX_MPLS_LOOKUP2_MISS            = 188, /* MPLS Lookup2 Miss                  */
    _SHR_RX_L3_TUNNEL_LOOKUP_MISS        = 189, /* L3 Tunnel Lookup Miss              */
    _SHR_RX_VLAN_TRANSLATE2_LOOKUP1_MISS = 190, /* VLAN Translate2 Lookup1 Miss       */
    _SHR_RX_VLAN_TRANSLATE2_LOOKUP2_MISS = 191, /* VLAN Translate2 Lookup2 Miss       */
    _SHR_RX_L2_STU_FAIL                 = 192, /* L2 STU check fail                   */
    _SHR_RX_SR_COUNTER_EXCEEDED         = 193, /* Seamless Redundancy(SR) -           */
                                               /* Counter Threshold Exceeded          */
    _SHR_RX_SR_COPY_TO_CPU_BIT0         = 194, /* Seamless Redundancy(SR) copy to CPU */
                                               /* SR custom reason code bit 0         */
    _SHR_RX_SR_COPY_TO_CPU_BIT1         = 195, /* SR custom reason code bit 1         */
    _SHR_RX_SR_COPY_TO_CPU_BIT2         = 196, /* SR custom reason code bit 2         */
    _SHR_RX_SR_COPY_TO_CPU_BIT3         = 197, /* SR custom reason code bit 3         */
    _SHR_RX_SR_COPY_TO_CPU_BIT4         = 198, /* SR custom reason code bit 4         */
    _SHR_RX_SR_COPY_TO_CPU_BIT5         = 199, /* SR custom reason code bit 5         */
    _SHR_RX_L3_HEADER_MISMATCH          = 200, /* L3 Header Mismatch                  */
    _SHR_RX_DLB_MONITOR                 = 201, /* DLB Monitor                         */
    _SHR_RX_ETRAP_MONITOR               = 202, /* ETRAP Monitor                       */
    _SHR_RX_INT_TURNAROUND              = 203, /* INT TurnAround                      */
    _SHR_RX_INT_HOP_LIMIT               = 204, /* INT Hop Limit                       */
    _SHR_RX_PROTECTION_STATUS_DOWN      = 205, /* Protection Status Down              */
    _SHR_RX_SVTAG                       = 206, /* SVtag                               */
    _SHR_RX_SVTAG_LOOKUP_MISS           = 207, /* SVtag Lookup Miss                   */
    _SHR_RX_PKT_DROP_ON_PORT_HIGIG3_CHECK = 208, /* Port HIGIG3 check fail            */
    _SHR_RX_SRV6_PROCESS_ERROR          = 209, /* SRV6 process error                  */
    _SHR_RX_SRV6_VALIDATION_ERROR       = 210, /* SRV6 validation error               */
    _SHR_RX_SRV6_UNKNOWN_RT_WITH_NONZERO_SL = 211, /* unknown routing type in SRV6    */
                                                   /* packets with nonzero segment    */
                                                   /* left field                      */
    _SHR_RX_RCPU_TO_CPU                 = 212, /* RCPU copied to CPU                  */
    _SHR_RX_REASON_COUNT                = 213  /* MUST BE LAST                        */
} _shr_rx_reason_t;

#define _SHR_RX_REASON_NAMES_INITIALIZER { \
    "Invalid",                  \
    "Arp",                      \
    "Bpdu",                     \
    "Broadcast",                \
    "ClassBasedMove",           \
    "ClassTagPackets",          \
    "Control",                  \
    "CpuLearn",                 \
    "DestLookupFail",           \
    "Dhcp",                     \
    "DosAttack",                \
    "E2eHolIbp",                \
    "EncapHiGigError",          \
    "FilterMatch",              \
    "GreChecksum",              \
    "GreSourceRoute",           \
    "HigigControl",             \
    "HigigHdrError",            \
    "IcmpRedirect",             \
    "Igmp",                     \
    "IngressFilter",            \
    "Ip",                       \
    "IpfixRateViolation",       \
    "IpMcastMiss",              \
    "IpOptionVersion",          \
    "Ipmc",                     \
    "IpmcRsvd",                 \
    "L2Cpu",                    \
    "L2DestMiss",               \
    "L2LearnLimit",             \
    "L2Move",                   \
    "L2MtuFail",                \
    "L2NonUnicastMiss",         \
    "L2SourceMiss",             \
    "L3AddrBindFail",           \
    "L3DestMiss",               \
    "L3HeaderError",            \
    "L3MtuFail",                \
    "L3Slowpath",               \
    "L3SourceMiss",             \
    "L3SourceMove",             \
    "MartianAddr",              \
    "McastIdxError",            \
    "McastMiss",                \
    "MimServiceError",          \
    "MplsCtrlWordError",        \
    "MplsError",                \
    "MplsInvalidAction",        \
    "MplsInvalidPayload",       \
    "MplsLabelMiss",            \
    "MplsSequenceNumber",       \
    "MplsTtl",                  \
    "Multicast",                \
    "Nhop",                     \
    "OamError",                 \
    "OamSlowPath",              \
    "OamLMDM",                  \
    "ParityError",              \
    "Protocol",                 \
    "SampleDest",               \
    "SampleSource",             \
    "SharedVlanMismatch",       \
    "SourceRoute",              \
    "TimeStamp",                \
    "Ttl",                      \
    "Ttl1",                     \
    "TunnelError",              \
    "UdpChecksum",              \
    "UnknownVlan",              \
    "UrpfFail",                 \
    "VcLabelMiss",              \
    "VlanFilterMatch",          \
    "WlanClientError",          \
    "WlanSlowPath",             \
    "WlanDot1xDrop",            \
    "ExceptionFlood",           \
    "Timesync",                 \
    "EavData",                  \
    "SamePortBridge",           \
    "SplitHorizon",             \
    "L4Error",                  \
    "Stp",                      \
    "EgressFilterRedirect",     \
    "FilterRedirect",           \
    "Loopback",                 \
    "VlanTranslate",            \
    "Mmrp",                     \
    "Srp",                      \
    "TunnelControl",            \
    "L2Marked",                 \
    "WlanSlowpathKeepalive",    \
    "Station",                  \
    "Niv",                      \
    "NivPrioDrop",              \
    "NivInterfaceMiss",         \
    "NivRpfFail",               \
    "NivTagInvalid",            \
    "NivTagDrop",               \
    "NivUntagDrop",             \
    "Trill",                    \
    "TrillInvalid",             \
    "TrillMiss",                \
    "TrillRpfFail",             \
    "TrillSlowpath",            \
    "TrillCoreIsIs",            \
    "TrillTtl",                 \
    "BfdSlowpath",              \
    "Bfd",                      \
    "Mirror",                   \
    "RegexAction",              \
    "RegexMatch",               \
    "FailoverDrop",             \
    "WlanTunnelError",          \
    "CongestionCnmProxy",       \
    "CongestionCnmProxyError",  \
    "CongestionCnm",            \
    "MplsUnknownAch",           \
    "MplsLookupsExceeded",      \
    "MplsReservedEntropyLabel", \
    "MplsIllegalReservedLabel", \
    "MplsRouterAlertLabel",     \
    "NivPrune",                 \
    "VirtualPortPrune",         \
    "NonUnicastDrop",           \
    "TrillPacketPortMismatch",  \
    "WlanClientMove",           \
    "WlanSourcePortMiss",       \
    "WlanClientSourceMiss",     \
    "WlanClientDestMiss",       \
    "WlanMtu",                  \
    "TrillName",                \
    "L2GreSipMiss",             \
    "L2GreVpnIdMiss",           \
    "TimesyncUnknownVersion",   \
    "BfdError",                 \
    "BfdUnknownVersion",        \
    "BfdInvalidVersion",        \
    "BfdLookupFailure",         \
    "BfdInvalidPacket",         \
    "VxlanSipMiss",             \
    "VxlanVpnIdMiss",           \
    "FcoeZoneCheckFail",        \
    "IpmcInterfaceMismatch",    \
    "Nat",                      \
    "TcpUdpNatMiss",            \
    "IcmpNatMiss",              \
    "NatFragment",              \
    "NatMiss",                  \
    "UnknownSubtentingPort",    \
    "LLTagAbsentDrop",          \
    "LLTagpresenDrop",          \
    "OAMCCMslowpath",           \
    "OAMIncompleteOpcode",      \
    "OAMCCMpacket",             \
    "Reserved0",                \
    "OAMMplsLmDM",              \
    "SAT",                      \
    "SampleSourceFlex",         \
    "FlexSflow",                \
    "VxltMiss",                 \
    "TunnelDecapEcnError",      \
    "TunnelObjectValidationFail",   \
    "L3Cpu",                    \
    "TunnelAdaptLookupMiss",    \
    "PacketFlowSelectMiss",     \
    "ProtectionDataDrop",       \
    "PacketFlowSelect",         \
    "OtherLookupMiss",          \
    "InvalidTpid",              \
    "MplsControlPacket",        \
    "TunnelTtlError",           \
    "L2HeaderError",            \
    "OtherLookupHit",           \
    "L2SrcLookupMiss",          \
    "L2SrcLookupHit",           \
    "L2DstLookupMiss",          \
    "L2DstLookupHit",           \
    "L3SrcRouteLookupMiss",     \
    "L3SrcHostLookupMiss",      \
    "L3SrcRouteLookupHit",      \
    "L3SrcHostLookupHit",       \
    "L3DstRouteLookupMiss",     \
    "L3DstHostLookupMiss",      \
    "L3DstRouteLookupHit",      \
    "L3DstHostLookupHit",       \
    "MplsLookup1Miss",          \
    "MplsLookup2Miss",          \
    "L3TunnelLookupMiss",       \
    "VlanTranslate1Lookup1Miss",\
    "VlanTranslate1Lookup2Miss",\
    "VlanTranslate2Lookup1Miss",\
    "VlanTranslate2Lookup2Miss",\
    "L2StuFail",                \
    "SrCounterExceeded",        \
    "SrCopyToCpuBit0",          \
    "SrCopyToCpuBit1",          \
    "SrCopyToCpuBit2",          \
    "SrCopyToCpuBit3",          \
    "SrCopyToCpuBit4",          \
    "SrCopyToCpuBit5",          \
    "L3HeaderMismatch",         \
    "DlbMonitor",               \
    "EtrapMonitor",             \
    "IntTurnAround",            \
    "IntHopLimit",              \
    "ProtectionStatusDown",     \
    "Svtag",                    \
    "SvtagLookupMiss",          \
    "PktDropOnPortHigig3Check", \
    "Srv6ProcessError",         \
    "Srv6ValidationError",      \
    "Srv6UnknownRoutingTypeWithNonZeroSl",   \
    "RcpuToCpu",                \
}

/* Base type for declarations */
#define    SHR_BITDCL        uint32
#define    SHR_BITWID        32

/* (internal) Number of SHR_BITDCLs needed to contain _max bits */
#define    _SHR_BITDCLSIZE(_max)    (((_max) + SHR_BITWID - 1) / SHR_BITWID)

/*
 * Set of "reasons" (see _SHR_RX_*) why a packet came to the CPU.
 */
typedef struct _shr_rx_reasons_s {
    SHR_BITDCL  pbits[_SHR_BITDCLSIZE(_SHR_RX_REASON_COUNT)];
} _shr_rx_reasons_t;

/*
 * Macro to add a reason (_SHR_RX_*) to a set of
 * reasons (_shr_rx_reasons_t)
 */
#define _SHR_RX_REASON_SET(_reasons, _reason) \
        ((((_reasons).pbits)[(_reason) / SHR_BITWID]) |= (1U << ((_reason) % SHR_BITWID)))

/*
 * Macro to clear a set of reasons (_shr_rx_reasons_t).
 */
#define _SHR_RX_REASON_CLEAR_ALL(_reasons)      \
{                                               \
    SHR_BITDCL *_ptr;                           \
    int _c = _SHR_RX_REASON_COUNT;              \
                                                \
    if (_c <= 0) {                              \
        return;                                 \
    }                                           \
                                                \
    _ptr = (((_reasons).pbits));                \
                                                \
    while (_c >= SHR_BITWID) {                  \
        *(_ptr++) = 0;                          \
        _c -= SHR_BITWID;                       \
    }                                           \
                                                \
    if(_c > 0) {                                \
        SHR_BITDCL _mask = ~0;                  \
        _mask >>= (SHR_BITWID - _c);            \
        *(_ptr) &= ~_mask;                      \
    }                                           \
}

/*
 * Macro to check if a reason (_SHR_RX_*) is included in a
 * set of reasons (_shr_rx_reasons_t). Returns:
 *   zero     => reason is not included in the set
 *   non-zero => reason is included in the set
 */
#define _SHR_RX_REASON_GET(_reasons, _reason) \
        ((((_reasons).pbits)[(_reason) / SHR_BITWID]) & (1U << ((_reason) % SHR_BITWID)))

#define _SHR_RX_REASON_ITER(_reasons, reason) \
    for(reason = _SHR_RX_INVALID; reason < (int)_SHR_RX_REASON_COUNT; reason++) \
        if(_SHR_RX_REASON_GET(_reasons, reason))

typedef enum _shr_rx_redirect_e { 
    _SHR_RX_REDIRECT_NORMAL = 0, 
    _SHR_RX_REDIRECT_HIGIG  = 1, 
    _SHR_RX_REDIRECT_TRUNCATED = 2,
    _SHR_RX_REDIRECT_MAX = _SHR_RX_REDIRECT_TRUNCATED
} _shr_rx_redirect_e; 


typedef enum _shr_rx_decap_tunnel_e {
    _SHR_RX_DECAP_NONE            = 0,  /* No tunnel Decap */
    _SHR_RX_DECAP_ACCESS_SVP      = 1,  /* Packet ingress on Access SVP (No decap) */
    _SHR_RX_DECAP_MIM             = 2,  /* Decap Mac-in-Mac tunnel */
    _SHR_RX_DECAP_L2GRE           = 3,  /* Decap L2GRE tunnel */
    _SHR_RX_DECAP_VXLAN           = 4,  /* Decap VXLAN tunnel */
    _SHR_RX_DECAP_AMT             = 5,  /* Decap AMT tunnel */
    _SHR_RX_DECAP_IP              = 6,  /* Decap IP tunnel */
    _SHR_RX_DECAP_TRILL           = 7,  /* Decap TRILL tunnel */
    _SHR_RX_DECAP_L2MPLS_1LABEL   = 8,  /* Decap MPLS 1 Label, L2 payload, no
                                           Control Word */
    _SHR_RX_DECAP_L2MPLS_2LABEL   = 9,  /* Decap MPLS 2 Label, L2 payload, no
                                           Control Word */
    _SHR_RX_DECAP_L2MPLS_1LABELCW = 10, /* Decap MPLS 1 Label, L2 payload, Control
                                           Word present */
    _SHR_RX_DECAP_L2MPLS_2LABELCW = 11, /* Decap MPLS 2 Label, L2 payload, Control
                                           Word present */
    _SHR_RX_DECAP_L3MPLS_1LABEL   = 12, /* Decap MPLS 1 Label, L3 payload, no
                                           Control Word present */
    _SHR_RX_DECAP_L3MPLS_2LABEL   = 13, /* Decap MPLS 2 Label, L3 payload, no
                                           Control Word present */
    _SHR_RX_DECAP_L3MPLS_1LABELCW = 14, /* Decap MPLS 1 Label, L3 payload, Control
                                           Word present */
    _SHR_RX_DECAP_L3MPLS_2LABELCW = 15, /* Decap MPLS 2 Label, L3 payload, Control
                                           Word present */
    _SHR_RX_DECAP_WTP2AC          = 16, /* Decap WTP2AC Tunnel */
    _SHR_RX_DECAP_AC2AC           = 17, /* Decap AC2AC Tunnel */
    _SHR_RX_DECAP_MPLS_1LABELCW   = 18, /* Decap MPLS 1 Label, Control
                                           Word present */
    _SHR_RX_DECAP_MPLS_2LABELCW   = 19, /* Decap MPLS 2 Label, Control
                                           Word present */
    _SHR_RX_DECAP_MPLS_1LABEL     = 20,  /* Decap MPLS 1 Label, no
                                           Control Word */
    _SHR_RX_DECAP_MPLS_2LABEL     = 21,  /* Decap MPLS 2 Label, no
                                           Control Word */
    _SHR_RX_DECAP_MPLS_3LABEL     = 22,  /* Decap MPLS 3 Label, no Control
                                            Word */
    _SHR_RX_DECAP_MPLS_3LABELCW   = 23,  /* Decap MPLS 3 Label, Control
                                            Word present */
    _SHR_RX_DECAP_MPLS_3LABEL_ENTROPY   = 25,  /* Decap MPLS 3 Label + entropy,
                                                 no Control Word */
    _SHR_RX_DECAP_MPLS_3LABEL_ENTROPYCW = 26,  /* Decap MPLS 3 Label + entropy,
                                                 Control Word present */
    _SHR_RX_DECAP_COUNT                = 27   /* Decap Tunnel Max */
} _shr_rx_decap_tunnel_t;

typedef enum _shr_rx_decap_hdr_e {
    _SHR_RX_DECAP_HDR_NONE         = 0,  /* No headers Decapped */
    _SHR_RX_DECAP_SYSTEM_HDR       = 1,  /* Decap System Header */
    _SHR_RX_DECAP_OUTER_L2_HDR     = 2,  /* Decap Outer L2 Header */
    _SHR_RX_DECAP_L3_L4_TUNNEL_HDR = 4,  /* Decap Outer L3/L4/TUNNEL Header */
    _SHR_RX_DECAP_INNER_L2_HDR     = 8,  /* Decap Inner L2 Header */
    _SHR_RX_DECAP_INNER_L3_L4_HDR  = 16, /* Decap Inner L3 and L4 Header */
    _SHR_RX_DECAP_HDR_MAX          = _SHR_RX_DECAP_INNER_L3_L4_HDR /* Max Value */
} _shr_rx_decap_hdr_t;

#endif /* _SHR_RX_H_ */
