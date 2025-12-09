/*
 * $Id: rx.h,v 1.16 Broadcom SDK $
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
 */

#ifndef _SOC_RX_H
#define _SOC_RX_H

#include <sal/types.h>

#include <shared/rx.h>

/*
 * PKT RX Packet Reasons; reason CPU received the packet.
 * Notes:
 *      It is possible no reasons are set (directed to CPU from ARL
 *      for example), or multiple reasons may be set.
 *
 * See "include/shared/rx.h" for full description.
 */

typedef enum soc_rx_reason_e {
    socRxReasonInvalid = _SHR_RX_INVALID, 
    socRxReasonArp = _SHR_RX_ARP, 
    socRxReasonBpdu = _SHR_RX_BPDU, 
    socRxReasonBroadcast = _SHR_RX_BROADCAST, 
    socRxReasonClassBasedMove = _SHR_RX_CLASS_BASED_MOVE, 
    socRxReasonClassTagPackets = _SHR_RX_CLASS_TAG_PACKETS, 
    socRxReasonControl = _SHR_RX_CONTROL, 
    socRxReasonCpuLearn = _SHR_RX_CPU_LEARN, 
    socRxReasonDestLookupFail = _SHR_RX_DEST_LOOKUP_FAIL, 
    socRxReasonDhcp = _SHR_RX_DHCP, 
    socRxReasonDosAttack = _SHR_RX_DOS_ATTACK, 
    socRxReasonE2eHolIbp = _SHR_RX_E2E_HOL_IBP, 
    socRxReasonEncapHigigError = _SHR_RX_ENCAP_HIGIG_ERROR, 
    socRxReasonFilterMatch = _SHR_RX_FILTER_MATCH, 
    socRxReasonGreChecksum = _SHR_RX_GRE_CHECKSUM, 
    socRxReasonGreSourceRoute = _SHR_RX_GRE_SOURCE_ROUTE, 
    socRxReasonHigigControl = _SHR_RX_HIGIG_CONTROL, 
    socRxReasonHigigHdrError = _SHR_RX_HIGIG_HDR_ERROR, 
    socRxReasonIcmpRedirect = _SHR_RX_ICMP_REDIRECT, 
    socRxReasonIgmp = _SHR_RX_IGMP, 
    socRxReasonIngressFilter = _SHR_RX_INGRESS_FILTER, 
    socRxReasonIp = _SHR_RX_IP, 
    socRxReasonIpfixRateViolation = _SHR_RX_IPFIX_RATE_VIOLATION, 
    socRxReasonIpMcastMiss = _SHR_RX_IP_MCAST_MISS, 
    socRxReasonIpmcReserved = _SHR_RX_IPMC_RSVD, 
    socRxReasonIpOptionVersion = _SHR_RX_IP_OPTION_VERSION, 
    socRxReasonIpmc = _SHR_RX_IPMC, 
    socRxReasonL2Cpu = _SHR_RX_L2_CPU, 
    socRxReasonL2DestMiss = _SHR_RX_L2_DEST_MISS, 
    socRxReasonL2LearnLimit = _SHR_RX_L2_LEARN_LIMIT, 
    socRxReasonL2Move = _SHR_RX_L2_MOVE, 
    socRxReasonL2MtuFail = _SHR_RX_L2_MTU_FAIL, 
    socRxReasonL2NonUnicastMiss = _SHR_RX_L2_NON_UNICAST_MISS, 
    socRxReasonL2SourceMiss = _SHR_RX_L2_SOURCE_MISS, 
    socRxReasonL3AddrBindFail = _SHR_RX_L3_ADDR_BIND_FAIL, 
    socRxReasonL3DestMiss = _SHR_RX_L3_DEST_MISS, 
    socRxReasonL3HeaderError = _SHR_RX_L3_HEADER_ERROR, 
    socRxReasonL3MtuFail = _SHR_RX_L3_MTU_FAIL, 
    socRxReasonL3Slowpath = _SHR_RX_L3_SLOW_PATH, 
    socRxReasonL3SourceMiss = _SHR_RX_L3_SOURCE_MISS, 
    socRxReasonL3SourceMove = _SHR_RX_L3_SOUCE_MOVE, 
    socRxReasonMartianAddr = _SHR_RX_MARTIAN_ADDR, 
    socRxReasonMcastIdxError = _SHR_RX_MCAST_IDX_ERROR, 
    socRxReasonMcastMiss = _SHR_RX_MCAST_MISS, 
    socRxReasonMimServiceError = _SHR_RX_MIM_SERVICE_ERROR, 
    socRxReasonMplsCtrlWordError = _SHR_RX_MPLS_CTRL_WORD_ERROR, 
    socRxReasonMplsError = _SHR_RX_MPLS_ERROR, 
    socRxReasonMplsInvalidAction = _SHR_RX_MPLS_INVALID_ACTION, 
    socRxReasonMplsInvalidPayload = _SHR_RX_MPLS_INVALID_PAYLOAD, 
    socRxReasonMplsLabelMiss = _SHR_RX_MPLS_LABEL_MISS, 
    socRxReasonMplsSequenceNumber = _SHR_RX_MPLS_SEQUENCE_NUMBER, 
    socRxReasonMplsTtl = _SHR_RX_MPLS_TTL, 
    socRxReasonMulticast = _SHR_RX_MULTICAST, 
    socRxReasonNhop = _SHR_RX_NHOP, 
    socRxReasonOAMError = _SHR_RX_OAM_ERROR, 
    socRxReasonOAMSlowpath = _SHR_RX_OAM_SLOW_PATH, 
    socRxReasonOAMLMDM = _SHR_RX_OAM_LMDM, 
    socRxReasonParityError = _SHR_RX_PARITY_ERROR, 
    socRxReasonProtocol = _SHR_RX_PROTOCOL, 
    socRxReasonSampleDest = _SHR_RX_SAMPLE_DEST, 
    socRxReasonSampleSource = _SHR_RX_SAMPLE_SOURCE, 
    socRxReasonSharedVlanMismatch = _SHR_RX_SHARED_VLAN_MISMATCH, 
    socRxReasonSourceRoute = _SHR_RX_SOURCE_ROUTE, 
    socRxReasonTimeStamp = _SHR_RX_TIME_STAMP, 
    socRxReasonTtl = _SHR_RX_TTL, 
    socRxReasonTtl1 = _SHR_RX_TTL1, 
    socRxReasonTunnelError = _SHR_RX_TUNNEL_ERROR, 
    socRxReasonUdpChecksum = _SHR_RX_UDP_CHECKSUM, 
    socRxReasonUnknownVlan = _SHR_RX_UNKNOWN_VLAN, 
    socRxReasonUrpfFail = _SHR_RX_URPF_FAIL, 
    socRxReasonVcLabelMiss = _SHR_RX_VC_LABEL_MISS, 
    socRxReasonVlanFilterMatch = _SHR_RX_VLAN_FILTER_MATCH, 
    socRxReasonWlanClientError = _SHR_RX_WLAN_CLIENT_ERROR, 
    socRxReasonWlanSlowpath = _SHR_RX_WLAN_SLOW_PATH, 
    socRxReasonWlanDot1xDrop = _SHR_RX_WLAN_DOT1X_DROP, 
    socRxReasonExceptionFlood = _SHR_RX_EXCEPTION_FLOOD, 
    socRxReasonTimeSync = _SHR_RX_TIMESYNC, 
    socRxReasonEAVData = _SHR_RX_EAV_DATA, 
    socRxReasonSamePortBridge = _SHR_RX_SAME_PORT_BRIDGE, 
    socRxReasonSplitHorizon = _SHR_RX_SPLIT_HORIZON, 
    socRxReasonL4Error = _SHR_RX_L4_ERROR, 
    socRxReasonStp = _SHR_RX_STP, 
    socRxReasonEgressFilterRedirect = _SHR_RX_EGRESS_FILTER_REDIRECT, 
    socRxReasonFilterRedirect = _SHR_RX_FILTER_REDIRECT, 
    socRxReasonLoopback = _SHR_RX_LOOPBACK,
    socRxReasonVlanTranslate = _SHR_RX_VLAN_TRANSLATE, 
    socRxReasonMmrp = _SHR_RX_MMRP, 
    socRxReasonSrp = _SHR_RX_SRP, 
    socRxReasonTunnelControl = _SHR_RX_TUNNEL_CONTROL, 
    socRxReasonL2Marked = _SHR_RX_L2_MARKED, 
    socRxReasonWlanSlowpathKeepalive = _SHR_RX_WLAN_SLOWPATH_KEEPALIVE, 
    socRxReasonStation = _SHR_RX_STATION, 
    socRxReasonNiv = _SHR_RX_NIV, 
    socRxReasonNivPrioDrop = _SHR_RX_NIV_PRIO_DROP, 
    socRxReasonNivInterfaceMiss = _SHR_RX_NIV_INTERFACE_MISS, 
    socRxReasonNivRpfFail = _SHR_RX_NIV_RPF_FAIL, 
    socRxReasonNivTagInvalid = _SHR_RX_NIV_TAG_INVALID, 
    socRxReasonNivTagDrop = _SHR_RX_NIV_TAG_DROP, 
    socRxReasonNivUntagDrop = _SHR_RX_NIV_UNTAG_DROP, 
    socRxReasonTrill = _SHR_RX_TRILL, 
    socRxReasonTrillInvalid = _SHR_RX_TRILL_INVALID, 
    socRxReasonTrillMiss = _SHR_RX_TRILL_MISS, 
    socRxReasonTrillRpfFail = _SHR_RX_TRILL_RPF_FAIL, 
    socRxReasonTrillSlowpath = _SHR_RX_TRILL_SLOWPATH, 
    socRxReasonTrillCoreIsIs = _SHR_RX_TRILL_CORE_IS_IS, 
    socRxReasonTrillTtl = _SHR_RX_TRILL_TTL, 
    socRxReasonTrillName = _SHR_RX_TRILL_NAME, 
    socRxReasonBfdSlowpath = _SHR_RX_BFD_SLOWPATH, 
    socRxReasonBfd = _SHR_RX_BFD, 
    socRxReasonMirror = _SHR_RX_MIRROR, 
    socRxReasonRegexAction = _SHR_RX_REGEX_ACTION, 
    socRxReasonFailoverDrop = _SHR_RX_FAILOVER_DROP, 
    socRxReasonWlanTunnelError = _SHR_RX_WLAN_TUNNEL_ERROR, 
    socRxReasonMplsReservedEntropyLabel = \
                                      _SHR_RX_MPLS_RESERVED_ENTROPY_LABEL, 
    socRxReasonCongestionCnmProxy = _SHR_RX_CONGESTION_CNM_PROXY,
    socRxReasonCongestionCnmProxyError = _SHR_RX_CONGESTION_CNM_PROXY_ERROR,
    socRxReasonCongestionCnm = _SHR_RX_CONGESTION_CNM,
    socRxReasonMplsUnknownAch = _SHR_RX_MPLS_UNKNOWN_ACH, 
    socRxReasonMplsLookupsExceeded = _SHR_RX_MPLS_LOOKUPS_EXCEEDED, 
    socRxReasonMplsIllegalReservedLabel = \
                                      _SHR_RX_MPLS_ILLEGAL_RESERVED_LABEL, 
    socRxReasonMplsRouterAlertLabel = _SHR_RX_MPLS_ROUTER_ALERT_LABEL, 
    socRxReasonNivPrune = _SHR_RX_NIV_PRUNE, 
    socRxReasonVirtualPortPrune = _SHR_RX_VIRTUAL_PORT_PRUNE, 
    socRxReasonNonUnicastDrop = _SHR_RX_NON_UNICAST_DROP, 
    socRxReasonTrillPacketPortMismatch = _SHR_RX_TRILL_PACKET_PORT_MISMATCH, 
    socRxReasonRegexMatch = _SHR_RX_REGEX_MATCH, 
    socRxReasonWlanClientMove = _SHR_RX_WLAN_CLIENT_MOVE, 
    socRxReasonWlanSourcePortMiss = _SHR_RX_WLAN_SOURCE_PORT_MISS, 
    socRxReasonWlanClientSourceMiss = _SHR_RX_WLAN_CLIENT_SOURCE_MISS, 
    socRxReasonWlanClientDestMiss = _SHR_RX_WLAN_CLIENT_DEST_MISS, 
    socRxReasonWlanMtu = _SHR_RX_WLAN_MTU, 
    socRxReasonL2GreSipMiss = _SHR_RX_L2GRE_SIP_MISS, 
    socRxReasonL2GreVpnIdMiss = _SHR_RX_L2GRE_VPN_ID_MISS, 
    socRxReasonTimesyncUnknownVersion = _SHR_RX_TIMESYNC_UNKNOWN_VERSION,
    socRxReasonVxlanSipMiss = _SHR_RX_VXLAN_SIP_MISS, 
    socRxReasonVxlanVpnIdMiss = _SHR_RX_VXLAN_VPN_ID_MISS, 
    socRxReasonFcoeZoneCheckFail = _SHR_RX_FCOE_ZONE_CHECK_FAIL, 
    socRxReasonIpmcInterfaceMismatch = _SHR_RX_IPMC_INTERFACE_MISMATCH, 
    socRxReasonNat = _SHR_RX_NAT, 
    socRxReasonTcpUdpNatMiss = _SHR_RX_TCP_UDP_NAT_MISS, 
    socRxReasonIcmpNatMiss = _SHR_RX_ICMP_NAT_MISS, 
    socRxReasonNatFragment = _SHR_RX_NAT_FRAGMENT, 
    socRxReasonNatMiss = _SHR_RX_NAT_MISS,
    socRxReasonUnknownSubtendingPort = _SHR_RX_UNKNOWN_SUBTENTING_PORT,
    socRxReasonLLTagAbsentDrop = _SHR_RX_LLTAG_ABSENT_DROP,
    socRxReasonLLTagPresentDrop = _SHR_RX_LLTAG_PRESENT_DROP,  
    socRxReasonOAMCCMSlowPath = _SHR_RX_OAM_CCM_SLOWPATH,  
    socRxReasonOAMIncompleteOpcode = _SHR_RX_OAM_INCOMPLETE_OPCODE,
    socRxReasonReserved0 = _SHR_RX_RESERVED_0,
    socRxReasonOAMMplsLmDm = _SHR_RX_OAM_MPLS_LMDM,
    socRxReasonSat = _SHR_RX_SAT,
    socRxReasonSampleSourceFlex = _SHR_RX_SAMPLE_SOURCE_FLEX,
    socRxReasonFlexSflow = _SHR_RX_FLEX_SFLOW,
    socRxReasonVxltMiss = _SHR_RX_VXLT_MISS,
    socRxReasonTunnelDecapEcnError = _SHR_RX_TUNNEL_DECAP_ECN_ERROR,
    socRxReasonTunnelObjectValidationFail = _SHR_RX_TUNNEL_OBJECT_VALIDATION_FAIL,
    socRxReasonL3Cpu = _SHR_RX_L3_CPU,
    socRxReasonTunnelAdaptLookupMiss = _SHR_RX_TUNNEL_ADAPT_LOOKUP_MISS,
    socRxReasonPacketFlowSelectMiss = _SHR_RX_PACKET_FLOW_SELECT_MISS,
    socRxReasonProtectionDataDrop = _SHR_RX_PROTECTION_DATA_DROP,
    socRxReasonPacketFlowSelect = _SHR_RX_PACKET_FLOW_SELECT,
    socRxReasonOtherLookupMiss = _SHR_RX_OTHER_LOOKUP_MISS,
    socRxReasonInvalidTpid = _SHR_RX_INVALID_TPID,
    socRxReasonMplsControlPacket = _SHR_RX_MPLS_CONTROL_PACKET,
    socRxReasonTunnelTtlError = _SHR_RX_TUNNEL_TTL_ERROR,
    socRxReasonL2HeaderError = _SHR_RX_L2_HEADER_ERROR,
    socRxReasonOtherLookupHit = _SHR_RX_OTHER_LOOKUP_HIT,
    socRxReasonL2SrcLookupMiss = _SHR_RX_L2_SRC_LOOKUP_MISS,
    socRxReasonL2SrcLookupHit = _SHR_RX_L2_SRC_LOOKUP_HIT,
    socRxReasonL2DstLookupMiss = _SHR_RX_L2_DST_LOOKUP_MISS,
    socRxReasonL2DstLookupHit = _SHR_RX_L2_DST_LOOKUP_HIT,
    socRxReasonL3SrcRouteLookupMiss = _SHR_RX_L3_SRC_ROUTE_LOOKUP_MISS,
    socRxReasonL3SrcHostLookupMiss = _SHR_RX_L3_SRC_HOST_LOOKUP_MISS,
    socRxReasonL3SrcRouteLookupHit = _SHR_RX_L3_SRC_ROUTE_LOOKUP_HIT,
    socRxReasonL3SrcHostLookupHit = _SHR_RX_L3_SRC_HOST_LOOKUP_HIT,
    socRxReasonL3DstRouteLookupMiss = _SHR_RX_L3_DST_ROUTE_LOOKUP_MISS,
    socRxReasonL3DstHostLookupMiss = _SHR_RX_L3_DST_HOST_LOOKUP_MISS,
    socRxReasonL3DstRouteLookupHit = _SHR_RX_L3_DST_ROUTE_LOOKUP_HIT,
    socRxReasonL3DstHostLookupHit = _SHR_RX_L3_DST_HOST_LOOKUP_HIT,
    socRxReasonVlanTranslate1Lookup1Miss = _SHR_RX_VLAN_TRANSLATE1_LOOKUP1_MISS,
    socRxReasonVlanTranslate1Lookup2Miss = _SHR_RX_VLAN_TRANSLATE1_LOOKUP2_MISS,
    socRxReasonMplsLookup1Miss = _SHR_RX_MPLS_LOOKUP1_MISS,
    socRxReasonMplsLookup2Miss = _SHR_RX_MPLS_LOOKUP2_MISS,
    socRxReasonL3TunnelLookupMiss = _SHR_RX_L3_TUNNEL_LOOKUP_MISS,
    socRxReasonVlanTranslate2Lookup1Miss = _SHR_RX_VLAN_TRANSLATE2_LOOKUP1_MISS,
    socRxReasonVlanTranslate2Lookup2Miss = _SHR_RX_VLAN_TRANSLATE2_LOOKUP2_MISS,
    socRxReasonL2StuFail = _SHR_RX_L2_STU_FAIL,
    socReasonSrCounterExceeded = _SHR_RX_SR_COUNTER_EXCEEDED,
    socRxReasonSrCopyToCpuBit0 = _SHR_RX_SR_COPY_TO_CPU_BIT0,
    socRxReasonSrCopyToCpuBit1 = _SHR_RX_SR_COPY_TO_CPU_BIT1,
    socRxReasonSrCopyToCpuBit2 = _SHR_RX_SR_COPY_TO_CPU_BIT2,
    socRxReasonSrCopyToCpuBit3 = _SHR_RX_SR_COPY_TO_CPU_BIT3,
    socRxReasonSrCopyToCpuBit4 = _SHR_RX_SR_COPY_TO_CPU_BIT4,
    socRxReasonSrCopyToCpuBit5 = _SHR_RX_SR_COPY_TO_CPU_BIT5,
    socRxReasonL3HeaderMismatch = _SHR_RX_L3_HEADER_MISMATCH,
    socRxReasonEtrapMonitor = _SHR_RX_ETRAP_MONITOR,
    socRxReasonDlbMonitor = _SHR_RX_DLB_MONITOR,
    socRxReasonIntTurnAround = _SHR_RX_INT_TURNAROUND,
    socRxReasonCount = _SHR_RX_REASON_COUNT /* MUST BE LAST */
} soc_rx_reason_t;


/*
 * Set of "reasons" (see socRxReason*) why a packet came to the CPU.
 */
typedef _shr_rx_reasons_t soc_rx_reasons_t;

/*
 * Macro to add a reason (socRxReason*) to a set of
 * reasons (soc_rx_reasons_t)
 */
#define SOC_RX_REASON_SET(_reasons, _reason) \
       _SHR_RX_REASON_SET(_reasons, _reason)

/*
 * Macro to clear a set of reasons (soc_rx_reasons_t).
 */
#define SOC_RX_REASON_CLEAR_ALL(_reasons) \
       _SHR_RX_REASON_CLEAR_ALL(_reasons)

#endif  /* !_SOC_RX_H */
