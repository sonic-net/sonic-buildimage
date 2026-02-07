/*
 * Copyright 2022 Clounix
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
 */

/* FILE NAME:   netif_knl.h
 * NOTES:
 */
#ifndef NETIF_KNL_H
#define NETIF_KNL_H

#include <clx_error.h>
#include <clx_types.h>

#define CLX_NETIF_NAME_LEN              (16)
#define CLX_NETIF_PROFILE_NUM_MAX       (256)
#define CLX_NETIF_PROFILE_PATTERN_NUM   (4)
#define CLX_NETIF_PROFILE_PATTERN_LEN   (8)

#define NETIF_EN_NETLINK

/* Port Speed */
typedef enum
{
    CLX_PORT_SPEED_1G   = 1000,
    CLX_PORT_SPEED_10G  = 10000,
    CLX_PORT_SPEED_25G  = 25000,
    CLX_PORT_SPEED_40G  = 40000,
    CLX_PORT_SPEED_50G  = 50000,
    CLX_PORT_SPEED_100G = 100000,
    CLX_PORT_SPEED_200G = 200000,
    CLX_PORT_SPEED_400G = 400000,
    CLX_PORT_SPEED_LAST
} CLX_PORT_SPEED_T;

#define HAL_INVALID_GROUP_LABEL           (0)
#define HAL_INVALID_NVO3_ENCAP_IDX        (0x3FFF)
#define HAL_INVALID_NVO3_ADJ_IDX          (0xFF)
#define HAL_INVALID_LCL_INTF_GRP          (0x1FFF)
#define HAL_INVALID_CNT_MTR_IDX           (0x7FFF)
#define HAL_INVALID_FDID                  (0)       /* means fdid is not created                         */
#define HAL_INVALID_L3_INTF               (0)       /* means L3 interface is disabled                    */
#define HAL_INVALID_FRR_STATE_IDX         (0)       /* means ECMP FRR is disabled                        */
#define HAL_INVALID_SEG_VMID              (0xFFFFFF)
#define HAL_INVALID_CPU_ID                (0x1F)
#define HAL_INVALID_DOS_IDX               (0xF)
#define HAL_INVALID_HW_BUM_OFFSET         (0x3)
#define HAL_INVALID_IEV_RSLT_IDX          (0x3FFFF)
#define HAL_INVALID_PHB_HW_IDX            (0x1F)    /* SRV INTF uses invalid index                       */
#define HAL_DEFAULT_PHB_HW_IDX            (0x1F)    /* LCL INTF uses default index                       */
#define HAL_RSV_FDID                      (0x3FFF)  /* HW reserved; all 1 means use outer vid as FDID    */
#define HAL_RSV_L3_INTF                   (0x3FFF)  /* HW reserved; all 1 means use outer vid as L3 Intf */
                                                    /*              all 1 means invalid at egress side   */
#define HAL_INVALID_ADJ_IDX               (0x3FFFF) /* For 18bit adj_idx                                 */
#define HAL_INVALID_MTR_HW_IDX            (0x1FFFF)
#define HAL_RSV_MEL_IDX                   (0x1FFF)

#define HAL_LAG_PORT_NUM                  (512)
#define HAL_L2_MGID_NUM                   (16384)   /* non-replicate mgid */
#define HAL_L3_MGID_NUM                   (8192)    /* replicate mgid */
#define HAL_MGID_NUM                      (HAL_L2_MGID_NUM + HAL_L3_MGID_NUM)
#define HAL_EXCPT_CPU_NUM                 (256)
#define HAL_EXCPT_DROP_NUM                (256)
#define HAL_DROP_NUM                      (512)
#define HAL_MIRROR_NUM                    (256)
#define HAL_REDIRECT_CPU_NUM              (2048)
#define HAL_TUNNEL_NUM                    (8192)
#define HAL_NSH_NUM                       (8192)
#define HAL_FRR_NUM                       (4096)
#define HAL_ECMP_NUM                      (2048)
#define HAL_INVALID_NUM                   (10240)

#define HAL_EXCPT_CPU_BASE_ID             (28 * 1024)
#define HAL_EXCPT_CPU_NON_L3_MIN          (0)
#define HAL_EXCPT_CPU_NON_L3_MAX          (HAL_EXCPT_CPU_NON_L3_MIN + HAL_EXCPT_CPU_NUM - 1)
#define HAL_EXCPT_CPU_L3_MIN              (HAL_EXCPT_CPU_NON_L3_MIN + HAL_EXCPT_CPU_NUM)
#define HAL_EXCPT_CPU_L3_MAX              (HAL_EXCPT_CPU_L3_MIN     + HAL_EXCPT_CPU_NUM - 1)

#if defined(NETIF_EN_NETLINK)

#define CLX_NETIF_NETLINK_NUM_MAX                   (256)
#define CLX_NETIF_NETLINK_MC_GROUP_NUM_MAX          (32)

typedef enum
{
    CLX_NETIF_INTF_PROPERTY_IGR_SAMPLING_RATE,
    CLX_NETIF_INTF_PROPERTY_EGR_SAMPLING_RATE,
    CLX_NETIF_INTF_PROPERTY_LAST
} CLX_NETIF_INTF_PROPERTY_T;

typedef struct
{
    C8_T                                name[CLX_NETIF_NAME_LEN];

} CLX_NETIF_NETLINK_MC_GROUP_T;

typedef struct
{
    UI32_T                              id;
    C8_T                                name[CLX_NETIF_NAME_LEN];
    CLX_NETIF_NETLINK_MC_GROUP_T        mc_group[CLX_NETIF_NETLINK_MC_GROUP_NUM_MAX];
    UI32_T                              mc_group_num;

} CLX_NETIF_NETLINK_T;

#endif

/* The reason defiend for the packets which are copied/redirected to CPU. */

/* RX Packet Reasons */
typedef enum
{
    CLX_PKT_RX_REASON_ECIA_SFLOW = 0,               /* Egress flow-based sflow hit.                                       */
    CLX_PKT_RX_REASON_VLAN_MISS_VLAN_CHK,           /* VLAN lookup miss or not accepted.                                  */
    CLX_PKT_RX_REASON_PAR_ERR,                      /* Parser error to CPU.                                               */
    CLX_PKT_RX_REASON_PAR_WARN,                     /* Parser warning to CPU.                                             */
    CLX_PKT_RX_REASON_DOS_CHK,                      /* DoS checking fail to CPU.                                          */
    CLX_PKT_RX_REASON_BFD_CTRL_PKT,                 /* BFD control packet to CPU.                                         */
    CLX_PKT_RX_REASON_INVALID_BFD_PKT,              /* Invalid BFD control packet to CPU.                                 */
    CLX_PKT_RX_REASON_SA_LEARN_FAIL,                /* Add to l2fdb fail to CPU.                                          */
    CLX_PKT_RX_REASON_TUNNEL_ECN_CU,                /* The current unused ECN combination defined in RFC6040.             */
    CLX_PKT_RX_REASON_FCOE_CLASS_2_F,               /* Copy FCoE Class 2 packet to cpu when packet is dropped.            */
    CLX_PKT_RX_REASON_URPF_CHECK_FAIL,              /* L3 URPF check fail to CPU.                                         */
    CLX_PKT_RX_REASON_L3_LKP_MISS,                  /* L3 lookup miss to CPU.                                             */
    CLX_PKT_RX_REASON_ICMP_REDIRECT,                /* The same L3 interface for ingress and egress to CPU.               */
    CLX_PKT_RX_REASON_IPV4_HDR_OPTION,              /* IPv4 header with option to CPU.                                    */
    CLX_PKT_RX_REASON_VCLAG_INVALID,                /* VM tag invalid to CPU.                                             */
    CLX_PKT_RX_REASON_VM_RPF_FAIL,                  /* VM RPF check fail to CPU.                                          */
    /* 16 */
    CLX_PKT_RX_REASON_VM_L2_FDB_MISS,               /* VM l2fdb lookup miss to CPU.                                       */
    CLX_PKT_RX_REASON_IGR_PORT_SFLOW,               /* Ingress port-based sflow hit.                                      */
    CLX_PKT_RX_REASON_EGR_PORT_SFLOW,               /* Egress port-based sflow hit.                                       */
    CLX_PKT_RX_REASON_IGR_FD_SFLOW,                 /* Ingress fd-based sflow hit.                                        */
    CLX_PKT_RX_REASON_ICIA_SFLOW,                   /* Ingress flow-based sflow hit.                                      */
    CLX_PKT_RX_REASON_IPSG_CHK,                     /* IPSG check fail to CPU.                                            */
    CLX_PKT_RX_REASON_FCOE_IFR_TO_CPU,              /* FCoE extend header include IFR header.                             */
    CLX_PKT_RX_REASON_TTL_EXPIRE,                   /* L3 TTL check fail to CPU.                                          */
    CLX_PKT_RX_REASON_L3MC_RPF_CHECK,               /* L3MC RPF check fail to CPU                                         */
    CLX_PKT_RX_REASON_IPV6_HOP_BY_HOP_EXT_HDR,      /* IPv6 with hop by hop extension header to CPU.                      */
    CLX_PKT_RX_REASON_VM_PDU,                       /* Received frames whose E-CID matches the IBR upstream Port's PCID.  */
    CLX_PKT_RX_REASON_PIM_REGISTER,                 /* For source hit and group miss, send to CPU for PIM-registration.   */
    CLX_PKT_RX_REASON_TUNNEL_TERM_LKP_MISS,         /* Tunnel term lookup failed with bank error or entry miss.           */
    CLX_PKT_RX_REASON_PP,                           /* Protocol/Management packet for SDN.                                */
    CLX_PKT_RX_REASON_EGR_FD_SFLOW,                 /* Egress fd-based sflow hit.                                         */
    CLX_PKT_RX_REASON_VXLAN_ROUTER_ALERT,           /* VXLAN packet with router alert to CPU.                             */
    /* 32 */
    CLX_PKT_RX_REASON_NVGRE_ROUTER_ALERT,           /* NVGRE packet with router alert to CPU.                             */
    CLX_PKT_RX_REASON_EX_PMOD_LIMIT,                /* Packet modification is excess PMOD's limitation.                   */
    CLX_PKT_RX_REASON_L3MC_SPT_READY_UNSET,         /* L3MC to CPU to trigger setting Shortest Path Tree ready bit.       */
    CLX_PKT_RX_REASON_IGR_MTU_FAIL,                 /* Ingress MTU check fail to CPU.                                     */
    CLX_PKT_RX_REASON_EGR_MTU_FAIL,                 /* Egress MTU check fail to CPU.                                      */
    CLX_PKT_RX_REASON_VXLAN_PING,                   /* VXLAN packet to CPU.                                               */
    CLX_PKT_RX_REASON_REDIRECT_TO_CPU_L2MC,         /* L2MC Redirect to CPU.                                              */
    CLX_PKT_RX_REASON_REDIRECT_TO_CPU_L3MC,         /* L3MC Redirect to CPU.                                              */
    CLX_PKT_RX_REASON_REDIRECT_TO_CPU_L2UC,         /* L2UC Redirect to CPU.                                              */
    CLX_PKT_RX_REASON_REDIRECT_TO_CPU_L3UC,         /* L3UC Redirect to CPU.                                              */
    CLX_PKT_RX_REASON_SDN_IGR_FLOW_SFLOW,           /* SDN ingress flow-based sflow hit.                                  */
    CLX_PKT_RX_REASON_SDN_EGR_FLOW_SFLOW,           /* SDN egress flow-based sflow hit.                                   */
    CLX_PKT_RX_REASON_SDN_HW_LIMITATION,            /* SDN hardware limitation to CPU.                                    */
    CLX_PKT_RX_REASON_SDN_OTHERS,                   /* SDN other exceptions to CPU.                                       */
    CLX_PKT_RX_REASON_SDN_PKT_IN,                   /* SDN output controller port (packet-in) to CPU.                     */
    CLX_PKT_RX_REASON_SDN_TO_LOCAL,                 /* SDN output local port to CPU.                                      */
    /* 48 */
    CLX_PKT_RX_REASON_CIA_0,                        /* CIA To CPU. */
    CLX_PKT_RX_REASON_CIA_1,                        /* CIA To CPU. */
    CLX_PKT_RX_REASON_CIA_2,                        /* CIA To CPU. */
    CLX_PKT_RX_REASON_CIA_3,                        /* CIA To CPU. */
    CLX_PKT_RX_REASON_CIA_4,                        /* CIA To CPU. */
    CLX_PKT_RX_REASON_CIA_5,                        /* CIA To CPU. */
    CLX_PKT_RX_REASON_CIA_6,                        /* CIA To CPU. */
    CLX_PKT_RX_REASON_CIA_7,                        /* CIA To CPU. */
    CLX_PKT_RX_REASON_USR_DEFINE_0,
    CLX_PKT_RX_REASON_USR_DEFINE_1,
    CLX_PKT_RX_REASON_USR_DEFINE_2,
    CLX_PKT_RX_REASON_USR_DEFINE_3,
    CLX_PKT_RX_REASON_USR_DEFINE_4,
    CLX_PKT_RX_REASON_USR_DEFINE_5,
    CLX_PKT_RX_REASON_USR_DEFINE_6,
    CLX_PKT_RX_REASON_USR_DEFINE_7,
    /* 64 */
    CLX_PKT_RX_REASON_USR_DEFINE_8,
    CLX_PKT_RX_REASON_USR_DEFINE_9,
    CLX_PKT_RX_REASON_USR_DEFINE_10,
    CLX_PKT_RX_REASON_USR_DEFINE_11,
    CLX_PKT_RX_REASON_USR_DEFINE_12,
    CLX_PKT_RX_REASON_USR_DEFINE_13,
    CLX_PKT_RX_REASON_L2_LKP_MISS,                  /* L2 lookup failed with the bank error or entry miss.                */
    CLX_PKT_RX_REASON_L2_HDR_MISS,                  /* l2 bridge copy without ingress Ethernet header.                    */
    CLX_PKT_RX_REASON_L2_SA_MISS,                   /* L2 lookup SA miss.                                                 */
    CLX_PKT_RX_REASON_L2_SA_MOVE,                   /* L2 lookup SA hit but interface move.                               */
    CLX_PKT_RX_REASON_IPV4_VER_ERR,                 /* IPv4 packet has bad version.                                       */
    CLX_PKT_RX_REASON_IPV4_OPT,                     /* IPv4 packet has options.                                           */
    CLX_PKT_RX_REASON_IPV4_LEN_ERR,                 /* IPv4 packet Internet Header Length is unmatched to payload length. */
    CLX_PKT_RX_REASON_IPV4_CHKSM_ERR,               /* IPv4 packet checksum error.                                        */
    CLX_PKT_RX_REASON_IPV4_MC_MALFORMED,            /* IPv4 multicast packet is malformed from RFC.                       */
    CLX_PKT_RX_REASON_IPV4_MC_LKP_MISS,             /* IPv4 multicast packet lookup miss.                                 */
    /* 80 */
    CLX_PKT_RX_REASON_IPV6_VER_ERR,                 /* IPv6 packet has bad version.                                       */
    CLX_PKT_RX_REASON_IPV6_LEN_ERR,                 /* IPv6 packet total length does not cover 40-Bytes IPv6 base header. */
    CLX_PKT_RX_REASON_IPV6_MC_MALFORMED,            /* IPv6 multicast packet is malformed from RFC.                       */
    CLX_PKT_RX_REASON_IPV6_MC_LKP_MISS,             /* IPv6 multicast packet lookup miss.                                 */
    CLX_PKT_RX_REASON_FCOE_VER_ERR,                 /* FCoE packet has bad version.                                       */
    CLX_PKT_RX_REASON_FCOE_LKP_MISS,                /* FCoE DID lookup failed, bank error or entry miss.                  */
    CLX_PKT_RX_REASON_FCOE_ZONING_FAIL,             /* FCoE zoning check failed to CPU.                                   */
    CLX_PKT_RX_REASON_MPLS_CTRL_PKT,                /* MPLS control packets to CPU.                                       */
    CLX_PKT_RX_REASON_MPLS_INVALID_PKT,             /* MPLS packet is illegal from RFC.                                   */
    CLX_PKT_RX_REASON_MPLS_LKP_MISS,                /* MPLS lookup failed with the bank error or entry miss.              */
    CLX_PKT_RX_REASON_MPLS_UHP_P2P_MISS,            /* */
    CLX_PKT_RX_REASON_MPLS_UHP_TTL_0,               /* MPLS UHP packet TTL is 0.                                          */
    CLX_PKT_RX_REASON_MPLS_UHP_TTL_1,               /* MPLS UHP packet TTL is 1.                                          */
    CLX_PKT_RX_REASON_MPLS_TRANSIT_TTL_0,           /* MPLS transit packet TTL is 0.                                      */
    CLX_PKT_RX_REASON_MPLS_TRANSIT_TTL_1,           /* MPLS transit packet TTL is 1.                                      */
    CLX_PKT_RX_REASON_MPLS_TERM_TTL_0,              /* MPLS term packet TTL is 0.                                         */
    /* 96 */
    CLX_PKT_RX_REASON_MPLS_TERM_TTL_1,              /* MPLS term packet TTL is 1.                                         */
    CLX_PKT_RX_REASON_IP_TUNNEL_CTRL_PKT,           /* IP tunnel control packets to CPU.                                  */
    CLX_PKT_RX_REASON_IP_TUNNEL_INNER_IPV4_UC_LCL,  /* IP tunnel packet has inner IPv4 unicast link local address.        */
    CLX_PKT_RX_REASON_IP_TUNNEL_INNER_IPV6_UC_LCL,  /* IP tunnel packet has inner IPv6 unicast link local address.        */
    CLX_PKT_RX_REASON_IP_TUNNEL_INNER_MC_LCL,       /* IP tunnel packet has inner multicast link local address.           */
    CLX_PKT_RX_REASON_IP_TUNNEL_INNER_VER_ERR,      /* IP tunnel packet inner is not IPv4 and IPv6.                       */
    CLX_PKT_RX_REASON_IP_TUNNEL_INNER_L3_ROUTE,     /* Not allow GRE, IPoMPLS or MPLS VPN routed by inner header.         */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV4_FRAG,    /* IP tunnel packet outer IPv4 fragment offset is not 0.              */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV4_OPT,     /* IP tunnel packet outer IPv4 has options.                           */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV4_AH,      /* IP tunnel packet outer IPv4 has authentication header.             */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV4_TTL_0,   /* IP tunnel packet outer IPv4 TTL is 0.                              */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV4_TTL_1,   /* IP tunnel packet outer IPv4 TTL is 1.                              */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV6_FRAG,    /* IP tunnel packet outer IPv6 fragment offset is not 0.              */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV6_OPT,     /* IP tunnel packet outer IPv6 has extension header.                  */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV6_AH,      /* IP tunnel packet outer IPv6 has authentication header.             */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV6_TTL_0,   /* IP tunnel packet outer IPv6 TTL is 0.                              */
    /* 112 */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV6_TTL_1,   /* IP tunnel packet outer IPv6 TTL is 1.                              */
    CLX_PKT_RX_REASON_IPUC_TUNNEL_INNER_VLAN_MISS,  /* IP unicast tunnel packet inner VLAN miss.                          */
    CLX_PKT_RX_REASON_IPMC_TUNNEL_INNER_VLAN_MISS,  /* IP multicast tunnel packet inner VLAN miss.                        */
    CLX_PKT_RX_REASON_AUTO_TUNNEL_DIP_MISS,         /* Auto tunnel outer IPv4 and inner IPv6 DIP are unmatched.           */
    CLX_PKT_RX_REASON_AUTO_TUNNEL_SIP_MISS,         /* Auto tunnel outer IPv4 and inner IPv6 SIP are unmatched.           */
    CLX_PKT_RX_REASON_ETHER_IP_VER_ERR,             /* Ethernet-within-IP tunnel version is illegal from RFC 3378.        */
    CLX_PKT_RX_REASON_GRE_VER_ERR,                  /* GRE tunnel version is illegal from RFC 2784.                       */
    CLX_PKT_RX_REASON_GRE_RSVD_NON_ZERO,            /* GRE tunnel reserved fields is illegal from RFC 2784.               */
    CLX_PKT_RX_REASON_GRE_CTRL_FLAG_ERR,            /* GRE tunnel control flag is illegal from RFC 2784.                  */
    CLX_PKT_RX_REASON_GRE_ERSPAN_TYP2_VER_ERR,      /* GRE ERSPAN type II tunnel version is illegal from ERSPAN draft.    */
    CLX_PKT_RX_REASON_GRE_ERSPAN_TYP3_VER_ERR,      /* GRE ERSPAN type III tunnel version is illegal from ERSPAN draft.   */
    CLX_PKT_RX_REASON_GRE_ERSPAN_TYP3_FT_ERR,       /* GRE ERSPAN type III tunnel frame type is illegal from ERSPAN draft.*/
    CLX_PKT_RX_REASON_GRE_ERSPAN_TERM_LKP_MISS,     /* GRE ERSPAN tunnel term lookup miss.                                */
    CLX_PKT_RX_REASON_VXLAN_BAS_RSVD_NON_ZERO,      /* VXLAN Basic header reserved fields is illegal from RFC.            */
    CLX_PKT_RX_REASON_VXLAN_BAS_VNI_FLAG_ERR,       /* VXLAN Basic header VNI flag is illegal from RFC.                   */
    CLX_PKT_RX_REASON_VXLAN_BAS_CTRL_FLAG_ERR,      /* VXLAN Basic header control flag is illegal from RFC.               */
    /* 128 */
    CLX_PKT_RX_REASON_VXLAN_BAS_UDP_CHKSM_ERR,      /* VXLAN Basic header UDP checksum is abnormal.                       */
    CLX_PKT_RX_REASON_VXLAN_GPE_VNI_FLAG_ERR,       /* VXLAN GRE header VNI flag is illegal from RFC.                     */
    CLX_PKT_RX_REASON_VXLAN_GPE_CTRL_FLAG_ERR,      /* VXLAN GPE header control flag is illegal from RFC.                 */
    CLX_PKT_RX_REASON_VXLAN_GPE_UDP_CHKSM_ERR,      /* VXLAN GPE header UDP checksum is abnormal.                         */
    CLX_PKT_RX_REASON_TRILL_VER_ERR,                /* TRILL version is illegal from RFC.                                 */
    CLX_PKT_RX_REASON_TRILL_MC_FLAG_ERR,            /* TRILL multicast flag is unmatched DMAC.                            */
    CLX_PKT_RX_REASON_TRILL_OPT,                    /* TRILL packet has option or option header length is illegal.        */
    CLX_PKT_RX_REASON_TRILL_LKP_MISS,               /* TRILL lookup failed with the bank error or entry miss.             */
    CLX_PKT_RX_REASON_TRILL_TRANSIT_TTL_0,          /* TRILL transit packet TTL is 0.                                     */
    CLX_PKT_RX_REASON_TRILL_TRANSIT_TTL_1,          /* TRILL transit packet TTL is 1.                                     */
    CLX_PKT_RX_REASON_TRILL_TERM_TTL_0,             /* TRILL term packet TTL is 0.                                        */
    CLX_PKT_RX_REASON_TRILL_TERM_TTL_1,             /* TRILL term packet TTL is 1.                                        */
    CLX_PKT_RX_REASON_TRILL_MRPF_CHECK_FAIL,        /* TRILL multicast RPF check failed.                                  */
    CLX_PKT_RX_REASON_NSH_CTRL_PKT,                 /* NSH control packets to CPU.                                        */
    CLX_PKT_RX_REASON_NSH_INVALID_PKT,              /* NSH packet is illegal from RFC.                                    */
    CLX_PKT_RX_REASON_NSH_LKP_MISS,                 /* NSH lookup failed with the bank error or entry miss.               */
    /* 144 */
    CLX_PKT_RX_REASON_ECMP_LKP_MISS,                /* ECMP lookup failed with the bank error or entry miss.              */
    CLX_PKT_RX_REASON_ACL_LKP_MISS,                 /* ACL lookup failed with the bank error or entry miss.               */
    CLX_PKT_RX_REASON_FLOW_LKP_MISS,                /* Flow lookup failed with the bank error or entry miss.              */
    CLX_PKT_RX_REASON_IGR_FLOW_SFLOW,               /* Ingress flow-based sflow hit.                                      */
    CLX_PKT_RX_REASON_EGR_FLOW_SFLOW,               /* Egress flow-based sflow hit.                                       */
    CLX_PKT_RX_REASON_HW_ERROR,                     /* HW error. e.g. ECC.                                                */
    CLX_PKT_RX_REASON_1588_RX_PKT,                  /* User spcified the 1588 packets.                                    */
    CLX_PKT_RX_REASON_STP_BLOCK,                    /* STP block packets to CPU.                                          */
    CLX_PKT_RX_REASON_STACKING_NEIGHBOR,            /* The stacking packets from neighbor.                                */
    CLX_PKT_RX_REASON_STACKING_BROADCAST,           /* The stacking broadcast packets.                                    */
    CLX_PKT_RX_REASON_STACKING_LOOP,                /* The stacking path is loop.                                         */
    CLX_PKT_RX_REASON_STORM_CONTROL,                /* Storm Control packets to CPU.                                      */
    CLX_PKT_RX_REASON_TUNNEL_INIT_LKP_MISS,         /* Tunnel init lookup failed with bank error or entry miss.           */
    CLX_PKT_RX_REASON_TUNNEL_INNER_VLAN_MISS,       /* IP Tunnel or TRILL packets miss inner VLAN.                        */
    CLX_PKT_RX_REASON_TUNNEL_INNER_VLAN_UNEXP,      /* IP Tunnel or TRILL packets not allowed inner VLAN.                 */
    CLX_PKT_RX_REASON_IGR_L3_MTU_FAIL,              /* Ingress L3 MTU check fail to CPU.                                  */
    /* 160 */
    CLX_PKT_RX_REASON_EGR_L3_MTU_FAIL,              /* Egress L3 MTU check fail to CPU.                                   */
    CLX_PKT_RX_REASON_IGR_TUNNEL_MTU_FAIL,          /* Ingress Tunnel MTU check fail to CPU.                              */
    CLX_PKT_RX_REASON_EGR_TUNNEL_MTU_FAIL,          /* Egress Tunnel MTU check fail to CPU.                               */
    CLX_PKT_RX_REASON_EGR_IPV4_TTL_1,               /* Egress IPv4 interface configure TTL 1.                             */
    CLX_PKT_RX_REASON_EGR_IPV6_TTL_1,               /* Egress IPv6 interface configure TTL 1.                             */
    CLX_PKT_RX_REASON_COPY_TO_CPU_L2MC,             /* L2MC Copy to CPU.                                                  */
    CLX_PKT_RX_REASON_COPY_TO_CPU_L3MC,             /* L3MC Copy to CPU.                                                  */
    CLX_PKT_RX_REASON_COPY_TO_CPU_L2UC,             /* L2UC Copy to CPU.                                                  */
    CLX_PKT_RX_REASON_COPY_TO_CPU_L3UC,             /* L3UC Copy to CPU.                                                  */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_UDP_CHKSM_ERR,    /* Flex tunnel packet UDP checksum is abnormal.                       */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_0_CHK,            /* Flex tunnel 0 with sanity, type, or length check fail.             */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_1_CHK,            /* Flex tunnel 1 with sanity, type, or length check fail.             */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_2_CHK,            /* Flex tunnel 2 with sanity, type, or length check fail.             */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_3_CHK,            /* Flex tunnel 3 with sanity, type, or length check fail.             */
    CLX_PKT_RX_REASON_EGR_SFLOW_HIGH_LATENCY,       /* Sample high latency packet. */
    CLX_PKT_RX_REASON_DPP_LOOPBACK,                 /* DPP loopback packet */
    /* 176 */
    CLX_PKT_RX_REASON_PPPOE_SRV_UNKNOWN,            /* PPPOE service unknown */
    CLX_PKT_RX_REASON_PPPOE_HDR_ERR,                /* PPPOE header error */
    CLX_PKT_RX_REASON_PPPOE_ENCAP_ERR,              /* PPPOE encap error */
    CLX_PKT_RX_REASON_IP_TUNNEL_IPV4_ERR,           /* IP tunnel IPv4 header error */
    CLX_PKT_RX_REASON_IP_TUNNEL_IPV6_ERR,           /* IP tunnel IPv6 header error */
    CLX_PKT_RX_REASON_DECAP_NSH_TTL_1,              /* NSH TLL 1 error */
    CLX_PKT_RX_REASON_TRANSIT_NSH_TTL_1,            /* NSH TLL 1 error */
    CLX_PKT_RX_REASON_PORT_MTR_DROP,                /* Meter over rate drop */
    CLX_PKT_RX_REASON_WECMP,                        /* WECMP config error */
    CLX_PKT_RX_REASON_IOAM_NODE_LEN_INVALID_IPV6,   /* IOAM over IPv6 node len not align */
    CLX_PKT_RX_REASON_IOAM_NODE_LEN_INVALID_GRE,    /* IOAM over GRE node len not align */
    CLX_PKT_RX_REASON_IOAM_NODE_LEN_INVALID_GPE,    /* IOAM over GPE node len not align */
    CLX_PKT_RX_REASON_OTHERS,                       /* None of the above reasons */

    CLX_PKT_RX_REASON_LAST
} CLX_PKT_RX_REASON_T;

#define CLX_PKT_RX_REASON_BITMAP_SIZE   (CLX_BITMAP_SIZE(CLX_PKT_RX_REASON_LAST))

/* The data structure is the reason bitmap for "to CPU" reason codes. */
typedef UI32_T CLX_PKT_RX_REASON_BITMAP_T[CLX_PKT_RX_REASON_BITMAP_SIZE];


/* ----------------------------------------------------------------------------------- struct */
typedef struct
{
    UI32_T          tx_pkt;
    UI32_T          tx_queue_full;
    UI32_T          tx_error;
    UI32_T          rx_pkt;

} CLX_NETIF_INTF_CNT_T;

typedef struct
{
    /* unique key */
    UI32_T                      id;
    C8_T                        name[CLX_NETIF_NAME_LEN];
    CLX_PORT_T                  port;    /* only support unit port and local port */

    /* metadata */
    CLX_MAC_T                   mac;

#define CLX_NETIF_INTF_FLAGS_MAC        (1UL << 0)
    UI32_T                      flags;

} CLX_NETIF_INTF_T;


#if defined(NETIF_EN_NETLINK)
typedef struct
{
    C8_T                        name[CLX_NETIF_NAME_LEN];
    C8_T                        mc_group_name[CLX_NETIF_NAME_LEN];
} CLX_NETIF_RX_DST_NETLINK_T;
#endif

typedef enum
{
    CLX_NETIF_RX_DST_SDK = 0,
#if defined(NETIF_EN_NETLINK)
    CLX_NETIF_RX_DST_NETLINK,
#endif
    CLX_NETIF_RX_DST_LAST
} CLX_NETIF_RX_DST_TYPE_T;

typedef struct
{
    /* unique key */
    UI32_T                      id;
    C8_T                        name[CLX_NETIF_NAME_LEN];
    UI32_T                      priority;

    /* match fields */
    CLX_PORT_T                  port;     /* only support unit port and local port */
    CLX_PKT_RX_REASON_BITMAP_T  reason_bitmap;
    UI8_T                       pattern[CLX_NETIF_PROFILE_PATTERN_NUM][CLX_NETIF_PROFILE_PATTERN_LEN];
    UI8_T                       mask[CLX_NETIF_PROFILE_PATTERN_NUM][CLX_NETIF_PROFILE_PATTERN_LEN];
    UI32_T                      offset[CLX_NETIF_PROFILE_PATTERN_NUM];

    /* for each flag 1:must hit, 0:don't care */
#define CLX_NETIF_PROFILE_FLAGS_PORT      (1UL << 0)
#define CLX_NETIF_PROFILE_FLAGS_REASON    (1UL << 1)
#define CLX_NETIF_PROFILE_FLAGS_PATTERN_0 (1UL << 2)
#define CLX_NETIF_PROFILE_FLAGS_PATTERN_1 (1UL << 3)
#define CLX_NETIF_PROFILE_FLAGS_PATTERN_2 (1UL << 4)
#define CLX_NETIF_PROFILE_FLAGS_PATTERN_3 (1UL << 5)
    UI32_T                      flags;

    CLX_NETIF_RX_DST_TYPE_T     dst_type;
#if defined(NETIF_EN_NETLINK)
    CLX_NETIF_RX_DST_NETLINK_T  netlink;
#endif

} CLX_NETIF_PROFILE_T;

/* NAMING CONSTANT DECLARATIONS
 */
/* Keep these values applied to different modules. */
#define HAL_PKT_IPP_EXCPT_NUM           (256)
#define HAL_PKT_EPP_EXCPT_NUM           (64)
#define HAL_PKT_IPP_L3_EXCPT_NUM        (6)
#define HAL_PKT_IPP_RSN_NUM             (16)
#define HAL_PKT_IPP_COPY2CPU_NUM        (16)
#define HAL_PKT_EPP_COPY2CPU_NUM        (8)

/* IEV_CFG_EXCPT_EN_W1_SDK_REDIRECT_TO_CPU_L2UC_FIELD_ID */
#define HAL_PKT_IPP_EXCPT_IEV_SDK_REDIRECT_TO_CPU_L2UC      (192 + 23)

/* IEV_CFG_EXCPT_EN_W1_SDK_REDIRECT_TO_CPU_L3UC_FIELD_ID */
#define HAL_PKT_IPP_EXCPT_IEV_SDK_REDIRECT_TO_CPU_L3UC      (192 + 24)

/* IEV_CFG_EXCPT_EN_W1_SDK_L3UC_DA_MISS_FIELD_ID */
#define HAL_PKT_IPP_EXCPT_IEV_SDK_L3UC_DA_MISS              (192 + 30)

/* IEV_CFG_EXCPT_EN_W1_SDK_L3MC_PIM_REGISTER_FIELD_ID */
#define HAL_PKT_IPP_EXCPT_IEV_SDK_L3MC_PIM_REGISTER         (192 + 31)

/* IEV_CFG_EXCPT_EN_W0_SDK_FLEX_DECAP_0_REASON_0_FIELD_ID */
#define HAL_PKT_IPP_EXCPT_IEV_SDK_FLEX_DECAP_0_REASON_0     (224 + 8)

/* HW define offset
 * Module write IEV.cp_to_cpu_idx=1-3
 * Module write IEV_CFG_CP_TO_CPU_BIT_POS[offset + cp_to_cpu_idx - 1]
 */
#define HAL_PKT_IEV_CP_TO_CPU_BIT_POS_L2UC_DA   (0)
#define HAL_PKT_IEV_CP_TO_CPU_BIT_POS_L2MC_DA   (3)
#define HAL_PKT_IEV_CP_TO_CPU_BIT_POS_L2SA      (6)
#define HAL_PKT_IEV_CP_TO_CPU_BIT_POS_MPLS      (9)
#define HAL_PKT_IEV_CP_TO_CPU_BIT_POS_TRILL     (12)
#define HAL_PKT_IEV_CP_TO_CPU_BIT_POS_FCOE      (15)
#define HAL_PKT_IEV_CP_TO_CPU_BIT_POS_L3UC_DA   (18)
#define HAL_PKT_IEV_CP_TO_CPU_BIT_POS_L3UC_SA   (21)
#define HAL_PKT_IEV_CP_TO_CPU_BIT_POS_L3MC      (24)
#define HAL_PKT_IEV_CP_TO_CPU_BIT_POS_FLW_UC    (27)
#define HAL_PKT_IEV_CP_TO_CPU_BIT_POS_FLW_MC    (30)

/* capacities are the same between CL8360 and CL8570 */
#define HAL_PKT_IPP_EXCPT_BITMAP_SIZE    (CLX_BITMAP_SIZE(HAL_PKT_IPP_EXCPT_NUM))
#define HAL_PKT_IPP_L3_EXCPT_BITMAP_SIZE (CLX_BITMAP_SIZE(HAL_PKT_IPP_L3_EXCPT_NUM))
#define HAL_PKT_EPP_EXCPT_BITMAP_SIZE    (CLX_BITMAP_SIZE(HAL_PKT_EPP_EXCPT_NUM))
#define HAL_PKT_IPP_RSN_BITMAP_SIZE      (CLX_BITMAP_SIZE(HAL_PKT_IPP_RSN_NUM))
#define HAL_PKT_IPP_COPY2CPU_BITMAP_SIZE (CLX_BITMAP_SIZE(HAL_PKT_IPP_COPY2CPU_NUM))
#define HAL_PKT_EPP_COPY2CPU_BITMAP_SIZE (CLX_BITMAP_SIZE(HAL_PKT_EPP_COPY2CPU_NUM))

typedef UI32_T HAL_PKT_IPP_EXCPT_BITMAP_T[HAL_PKT_IPP_EXCPT_BITMAP_SIZE];
typedef UI32_T HAL_PKT_IPP_L3_EXCPT_BITMAP_T[HAL_PKT_IPP_L3_EXCPT_BITMAP_SIZE];
typedef UI32_T HAL_PKT_EPP_EXCPT_BITMAP_T[HAL_PKT_EPP_EXCPT_BITMAP_SIZE];
typedef UI32_T HAL_PKT_IPP_RSN_BITMAP_T[HAL_PKT_IPP_RSN_BITMAP_SIZE];
typedef UI32_T HAL_PKT_IPP_COPY2CPU_BITMAP_T[HAL_PKT_IPP_COPY2CPU_BITMAP_SIZE];
typedef UI32_T HAL_PKT_EPP_COPY2CPU_BITMAP_T[HAL_PKT_EPP_COPY2CPU_BITMAP_SIZE];

typedef struct
{
    /* excpt */
    HAL_PKT_IPP_EXCPT_BITMAP_T        ipp_excpt_bitmap;
    HAL_PKT_IPP_L3_EXCPT_BITMAP_T     ipp_l3_excpt_bitmap;
    HAL_PKT_EPP_EXCPT_BITMAP_T        epp_excpt_bitmap;

    /* cp */
    HAL_PKT_IPP_RSN_BITMAP_T          ipp_rsn_bitmap;
    HAL_PKT_IPP_COPY2CPU_BITMAP_T     ipp_copy2cpu_bitmap;
    HAL_PKT_EPP_COPY2CPU_BITMAP_T     epp_copy2cpu_bitmap;

} HAL_PKT_RX_REASON_BITMAP_T;

#endif  /* End of NETIF_KNL_H */
