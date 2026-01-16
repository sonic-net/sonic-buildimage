/*******************************************************************************
 *  Copyright Statement:
 *  --------------------
 *  This software and the information contained therein are protected by
 *  copyright and other intellectual property laws and terms herein is
 *  confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Clounix (Shanghai) Technology Limited. (C) 2020-2023
 *
 *  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 *  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("CLOUNIX SOFTWARE")
 *  RECEIVED FROM CLOUNIX AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
 *  AN "AS-IS" BASIS ONLY. CLOUNIX EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 *  NEITHER DOES CLOUNIX PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 *  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 *  SUPPLIED WITH THE CLOUNIX SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
 *  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. CLOUNIX SHALL ALSO
 *  NOT BE RESPONSIBLE FOR ANY CLOUNIX SOFTWARE RELEASES MADE TO BUYER'S
 *  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
 *
 *  BUYER'S SOLE AND EXCLUSIVE REMEDY AND CLOUNIX'S ENTIRE AND CUMULATIVE
 *  LIABILITY WITH RESPECT TO THE CLOUNIX SOFTWARE RELEASED HEREUNDER WILL BE,
 *  AT CLOUNIX'S OPTION, TO REVISE OR REPLACE THE CLOUNIX SOFTWARE AT ISSUE,
 *  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
 *  CLOUNIX FOR SUCH CLOUNIX SOFTWARE AT ISSUE.
 *
 *  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
 *  WITH THE LAWS OF THE PEOPLE'S REPUBLIC OF CHINA, EXCLUDING ITS CONFLICT OF
 *  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
 *  RELATED THERETO SHALL BE SETTLED BY LAWSUIT IN SHANGHAI,CHINA UNDER.
 *
 *******************************************************************************/

/* FILE NAME:  clx_pkt.h
 * PURPOSE:
 *      Provide the interface for packet TX/RX configuration.
 *
 * NOTES:
 *
 */

#ifndef CLX_PKT_H
#define CLX_PKT_H

/* INCLUDE FILE DECLARATIONS
 */
#include <clx_types.h>
#include <clx_error.h>

/* NAMING DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */

/* ----------------------------------------------------------------------------------- SDN */
/* SDN-Specific Port Type */
typedef enum {
    CLX_PKT_SDN_PHY_PORT = 0,    /* SDN port type physical port */
    CLX_PKT_SDN_LAG_PORT = 1,    /* SDN port type link aggregation group */
    CLX_PKT_SDN_TUNNEL_PORT = 2, /* SDN port type tunnel port */
    CLX_PKT_SDN_RSV_PORT = 255,  /* reserved port */
} CLX_PKT_SDN_PORT_TYPE_T;

/* SDN-Specific Port Structure */
typedef struct CLX_PKT_SDN_PORT_S {
    CLX_PKT_SDN_PORT_TYPE_T port_type; /* SDN specific port type */
    UI32_T port_no;                    /* Port number, including physical port, lag port,
                                        * tunnel port, and reserved port
                                        */

} CLX_PKT_SDN_PORT_T;

/* SDN-Specific Tunnel Structure */
typedef struct CLX_PKT_SDN_TUNNEL_S {
    CLX_TUNNEL_TYPE_T tunnel_type; /* Tunnel types which are defined in clx_types.h */
    UI32_T tunnel_no;              /* Tunnel number.
                                    * For IP tunnel, don't care.
                                    * For GRE tunnel and ERSPAN, it is GRE_key.
                                    * For NVGRE tunnel, the higher 24 bits is VSID and the lower 8 bits is
                                    * FlowID.              For VxLAN tunnel, it is VNI.
                                    */
} CLX_PKT_SDN_TUNNEL_T;

/* SDN-Specific Packet-In Reason Type */
typedef enum {
    CLX_PKT_SDN_PKT_IN_REASON_TABLE_MISS = 0,   /* No matching flow (table-miss flow entry). */
    CLX_PKT_SDN_PKT_IN_REASON_APPLY_ACTION = 1, /* Output to controller in apply-actions.    */
    CLX_PKT_SDN_PKT_IN_REASON_INVALID_TTL = 2,  /* Packet has invalid TTL.                   */
    CLX_PKT_SDN_PKT_IN_REASON_ACTION_SET = 3,   /* Output to controller in action set.       */
    CLX_PKT_SDN_PKT_IN_REASON_GROUP = 4,        /* Output to controller in group bucket.     */
    CLX_PKT_SDN_PKT_IN_REASON_PACKET_OUT = 5,   /* Output to controller in packet-out.       */
    CLX_PKT_SDN_PKT_IN_REASON_LAST
} CLX_PKT_SDN_PKT_IN_REASON_T;

/* -----------------------------------------------------------------------------------
 * Control-to-CPU */
/* Control-to-CPU Key Types */
typedef enum {
    CLX_PKT_KEY_TYPE_IPV6 = 0, /* Key type IPv6    */
    CLX_PKT_KEY_TYPE_IPV4,     /* Key type IPv4    */
    CLX_PKT_KEY_TYPE_ARP,      /* Key type ARP     */
    CLX_PKT_KEY_TYPE_L2,       /* Key type L2      */
    CLX_PKT_KEY_TYPE_TUNNEL,   /* [CL8600 not support] Key type TUNNEL  */
    CLX_PKT_KEY_TYPE_LAST
} CLX_PKT_KEY_TYPE_T;

/* Tunnel Types */
typedef enum {
    CLX_PKT_TUNNEL_TYPE_NONE = 0,        /* tunnel type is none                                  */
    CLX_PKT_TUNNEL_TYPE_TRILL,           /* [CL8600 not support] tunnel type is trill            */
    CLX_PKT_TUNNEL_TYPE_MPLS_GAL,        /* [CL8600 not support] tunnel type is mpls gal         */
    CLX_PKT_TUNNEL_TYPE_MPLS_PW,         /* tunnel type is mpls pw                               */
    CLX_PKT_TUNNEL_TYPE_GRE_L2,          /* [CL8600 not support] tunnel type is gre l2           */
    CLX_PKT_TUNNEL_TYPE_GRE,             /* [CL8600 not support] tunnel type is gre              */
    CLX_PKT_TUNNEL_TYPE_VXLAN_GPE_L2,    /* [CL8600 not support] tunnel type is vxlan_gpe_l2     */
    CLX_PKT_TUNNEL_TYPE_VXLAN_GPE,       /* [CL8600 not support] tunnel type is vxlan_gpe        */
    CLX_PKT_TUNNEL_TYPE_VXLAN_BAS_L2,    /* [CL8600 not support] tunnel type is vxlan_bas_l2     */
    CLX_PKT_TUNNEL_TYPE_VXLAN_BAS,       /* [CL8600 not support] tunnel type is type_vxlan_bas   */
    CLX_PKT_TUNNEL_TYPE_IP,              /* tunnel type is ip                                    */
    CLX_PKT_TUNNEL_TYPE_GENEVE_L2,       /* [CL8600 not support] tunnel type is geneve_l2        */
    CLX_PKT_TUNNEL_TYPE_GENEVE,          /* [CL8600 not support] tunnel type is geneve           */
    CLX_PKT_TUNNEL_TYPE_IP_L2,           /* [CL8600 only] tunnel type is ip l2                   */
    CLX_PKT_TUNNEL_TYPE_ERSPAN,          /* [CL8600 only] tunnel type is erspan                  */
    CLX_PKT_TUNNEL_TYPE_SRV6_POP_SRH,    /* [CL8600 only] tunnel type is srv6 pop srh            */
    CLX_PKT_TUNNEL_TYPE_SRV6_POP_IP_SRH, /* [CL8600 only] tunnel type is srv6 pop ip srh         */
    CLX_PKT_TUNNEL_TYPE_LAST
} CLX_PKT_TUNNEL_TYPE_T;

/* The reason defiend for the packets which are copied/redirected to CPU. */

/* RX Packet Reasons */
typedef enum {
    CLX_PKT_RX_REASON_ECIA_SFLOW = 0,     /* Egress flow-based sflow hit. */
    CLX_PKT_RX_REASON_VLAN_MISS_VLAN_CHK, /* VLAN lookup miss or not accepted. */
    CLX_PKT_RX_REASON_PAR_ERR,            /* Parser error to CPU. */
    CLX_PKT_RX_REASON_PAR_WARN,           /* Parser warning to CPU. */
    CLX_PKT_RX_REASON_DOS_CHK,            /* DoS checking fail to CPU. */
    CLX_PKT_RX_REASON_BFD_CTRL_PKT,       /* BFD control packet to CPU. */
    CLX_PKT_RX_REASON_INVALID_BFD_PKT,    /* Invalid BFD control packet to CPU. */
    CLX_PKT_RX_REASON_SA_LEARN_FAIL,      /* Add to l2fdb fail to CPU. */
    CLX_PKT_RX_REASON_TUNNEL_ECN_CU,   /* The current unused ECN combination defined in RFC6040. */
    CLX_PKT_RX_REASON_FCOE_CLASS_2_F,  /* Copy FCoE Class 2 packet to cpu when packet is dropped. */
    CLX_PKT_RX_REASON_URPF_CHECK_FAIL, /* L3 URPF check fail to CPU. */
    CLX_PKT_RX_REASON_L3_LKP_MISS,     /* L3 lookup miss to CPU. */
    CLX_PKT_RX_REASON_ICMP_REDIRECT,   /* The same L3 interface for ingress and egress to CPU. */
    CLX_PKT_RX_REASON_IPV4_HDR_OPTION, /* IPv4 header with option to CPU. */
    CLX_PKT_RX_REASON_VCLAG_INVALID,   /* VM tag invalid to CPU. */
    CLX_PKT_RX_REASON_VM_RPF_FAIL,     /* VM RPF check fail to CPU. */
    /* 16 */
    CLX_PKT_RX_REASON_VM_L2_FDB_MISS,          /* VM l2fdb lookup miss to CPU. */
    CLX_PKT_RX_REASON_IGR_PORT_SFLOW,          /* Ingress port-based sflow hit. */
    CLX_PKT_RX_REASON_EGR_PORT_SFLOW,          /* Egress port-based sflow hit. */
    CLX_PKT_RX_REASON_IGR_FD_SFLOW,            /* Ingress fd-based sflow hit. */
    CLX_PKT_RX_REASON_ICIA_SFLOW,              /* Ingress flow-based sflow hit. */
    CLX_PKT_RX_REASON_IPSG_CHK,                /* IPSG check fail to CPU. */
    CLX_PKT_RX_REASON_FCOE_IFR_TO_CPU,         /* FCoE extend header include IFR header. */
    CLX_PKT_RX_REASON_TTL_EXPIRE,              /* L3 TTL check fail to CPU. */
    CLX_PKT_RX_REASON_L3MC_RPF_CHECK,          /* L3MC RPF check fail to CPU */
    CLX_PKT_RX_REASON_IPV6_HOP_BY_HOP_EXT_HDR, /* IPv6 with hop by hop extension header to CPU. */
    CLX_PKT_RX_REASON_VM_PDU, /* Received frames whose E-CID matches the IBR upstream Port's PCID.
                               */
    CLX_PKT_RX_REASON_PIM_REGISTER,         /* For source hit and group miss, send to CPU for
                                               PIM-registration. */
    CLX_PKT_RX_REASON_TUNNEL_TERM_LKP_MISS, /* Tunnel term lookup failed with bank error or entry
                                               miss. */
    CLX_PKT_RX_REASON_EGR_FD_SFLOW,         /* Egress fd-based sflow hit. */
    CLX_PKT_RX_REASON_VXLAN_ROUTER_ALERT,   /* VXLAN packet with router alert to CPU. */
    CLX_PKT_RX_REASON_NVGRE_ROUTER_ALERT,   /* NVGRE packet with router alert to CPU. */
    /* 32 */
    CLX_PKT_RX_REASON_EX_PMOD_LIMIT,        /* Packet modification is excess PMOD's limitation. */
    CLX_PKT_RX_REASON_L3MC_SPT_READY_UNSET, /* L3MC to CPU to trigger setting Shortest Path Tree
                                               ready bit. */
    CLX_PKT_RX_REASON_IGR_MTU_FAIL,         /* Ingress MTU check fail to CPU. */
    CLX_PKT_RX_REASON_EGR_MTU_FAIL,         /* Egress MTU check fail to CPU. */
    CLX_PKT_RX_REASON_VXLAN_PING,           /* VXLAN packet to CPU. */
    CLX_PKT_RX_REASON_REDIRECT_TO_CPU_L2MC, /* L2MC Redirect to CPU. */
    CLX_PKT_RX_REASON_REDIRECT_TO_CPU_L3MC, /* L3MC Redirect to CPU. */
    CLX_PKT_RX_REASON_REDIRECT_TO_CPU_L2UC, /* L2UC Redirect to CPU. */
    CLX_PKT_RX_REASON_REDIRECT_TO_CPU_L3UC, /* L3UC Redirect to CPU. */
    CLX_PKT_RX_REASON_CIA_0,                /* CIA To CPU. CL8500, CL8300 */
    CLX_PKT_RX_REASON_CIA_1,                /* CIA To CPU. CL8500, CL8300 */
    CLX_PKT_RX_REASON_CIA_2,                /* CIA To CPU. CL8500, CL8300 */
    CLX_PKT_RX_REASON_CIA_3,                /* CIA To CPU. CL8500, CL8300 */
    CLX_PKT_RX_REASON_CIA_4,                /* CIA To CPU. CL8500, CL8300 */
    CLX_PKT_RX_REASON_CIA_5,                /* CIA To CPU. CL8500, CL8300 */
    CLX_PKT_RX_REASON_CIA_6,                /* CIA To CPU. CL8500, CL8300 */
    /* 48 */
    CLX_PKT_RX_REASON_CIA_7,         /* CIA To CPU. CL8500, CL8300 */
    CLX_PKT_RX_REASON_USR_DEFINE_0,  /* user define CL8500, CL8300 */
    CLX_PKT_RX_REASON_USR_DEFINE_1,  /* user define CL8500, CL8300 */
    CLX_PKT_RX_REASON_USR_DEFINE_2,  /* user define CL8500, CL8300 */
    CLX_PKT_RX_REASON_USR_DEFINE_3,  /* user define CL8500, CL8300 */
    CLX_PKT_RX_REASON_USR_DEFINE_4,  /* user define CL8500, CL8300 */
    CLX_PKT_RX_REASON_USR_DEFINE_5,  /* user define CL8500, CL8300 */
    CLX_PKT_RX_REASON_USR_DEFINE_6,  /* user define CL8500, CL8300 */
    CLX_PKT_RX_REASON_USR_DEFINE_7,  /* user define CL8500, CL8300 */
    CLX_PKT_RX_REASON_USR_DEFINE_8,  /* user define CL8500, CL8300 */
    CLX_PKT_RX_REASON_USR_DEFINE_9,  /* user define CL8500, CL8300 */
    CLX_PKT_RX_REASON_USR_DEFINE_10, /* user define CL8500, CL8300 */
    CLX_PKT_RX_REASON_USR_DEFINE_11, /* user define CL8500, CL8300 */
    CLX_PKT_RX_REASON_USR_DEFINE_12, /* user define CL8500, CL8300 */
    CLX_PKT_RX_REASON_USR_DEFINE_13, /* user define CL8500, CL8300 */
    CLX_PKT_RX_REASON_L2_LKP_MISS,   /* L2 lookup failed with the bank error or entry miss. */
    /* 64 */
    CLX_PKT_RX_REASON_L2_HDR_MISS,    /* l2 bridge copy without ingress Ethernet header. */
    CLX_PKT_RX_REASON_L2_SA_MISS,     /* L2 lookup SA miss. */
    CLX_PKT_RX_REASON_L2_SA_MOVE,     /* L2 lookup SA hit but interface move. */
    CLX_PKT_RX_REASON_IPV4_VER_ERR,   /* IPv4 packet has bad version. */
    CLX_PKT_RX_REASON_IPV4_OPT,       /* IPv4 packet has options. */
    CLX_PKT_RX_REASON_IPV4_LEN_ERR,   /* IPv4 packet Internet Header Length is unmatched to payload
                                           length. */
    CLX_PKT_RX_REASON_IPV4_CHKSM_ERR, /* IPv4 packet checksum error. */
    CLX_PKT_RX_REASON_IPV4_MC_MALFORMED, /* IPv4 multicast packet is malformed from RFC. */
    CLX_PKT_RX_REASON_IPV4_MC_LKP_MISS,  /* IPv4 multicast packet lookup miss. */
    CLX_PKT_RX_REASON_IPV6_VER_ERR,      /* IPv6 packet has bad version. */
    CLX_PKT_RX_REASON_IPV6_LEN_ERR, /* IPv6 packet total length does not cover 40-Bytes IPv6 base
                                       header. */
    CLX_PKT_RX_REASON_IPV6_MC_MALFORMED, /* IPv6 multicast packet is malformed from RFC. */
    CLX_PKT_RX_REASON_IPV6_MC_LKP_MISS,  /* IPv6 multicast packet lookup miss. */
    CLX_PKT_RX_REASON_FCOE_VER_ERR,      /* FCoE packet has bad version. */
    CLX_PKT_RX_REASON_FCOE_LKP_MISS,     /* FCoE DID lookup failed, bank error or entry miss. */
    CLX_PKT_RX_REASON_FCOE_ZONING_FAIL,  /* FCoE zoning check failed to CPU. */
    /* 80 */
    CLX_PKT_RX_REASON_MPLS_CTRL_PKT,     /* MPLS control packets to CPU. */
    CLX_PKT_RX_REASON_MPLS_INVALID_PKT,  /* MPLS packet is illegal from RFC. */
    CLX_PKT_RX_REASON_MPLS_LKP_MISS,     /* MPLS lookup failed with the bank error or entry miss. */
    CLX_PKT_RX_REASON_MPLS_UHP_P2P_MISS, /* MPLS UHP packet p2p miss */
    CLX_PKT_RX_REASON_MPLS_UHP_TTL_0,    /* MPLS UHP packet TTL is 0. */
    CLX_PKT_RX_REASON_MPLS_UHP_TTL_1,    /* MPLS UHP packet TTL is 1. */
    CLX_PKT_RX_REASON_MPLS_TRANSIT_TTL_0,          /* MPLS transit packet TTL is 0. */
    CLX_PKT_RX_REASON_MPLS_TRANSIT_TTL_1,          /* MPLS transit packet TTL is 1. */
    CLX_PKT_RX_REASON_MPLS_TERM_TTL_0,             /* MPLS term packet TTL is 0. */
    CLX_PKT_RX_REASON_MPLS_TERM_TTL_1,             /* MPLS term packet TTL is 1. */
    CLX_PKT_RX_REASON_IP_TUNNEL_CTRL_PKT,          /* IP tunnel control packets to CPU. */
    CLX_PKT_RX_REASON_IP_TUNNEL_INNER_IPV4_UC_LCL, /* IP tunnel packet has inner IPv4 unicast link
                                                      local address. */
    CLX_PKT_RX_REASON_IP_TUNNEL_INNER_IPV6_UC_LCL, /* IP tunnel packet has inner IPv6 unicast link
                                                      local address. */
    CLX_PKT_RX_REASON_IP_TUNNEL_INNER_MC_LCL,   /* IP tunnel packet has inner multicast link local
                                                   address. */
    CLX_PKT_RX_REASON_IP_TUNNEL_INNER_VER_ERR,  /* IP tunnel packet inner is not IPv4 and IPv6. */
    CLX_PKT_RX_REASON_IP_TUNNEL_INNER_L3_ROUTE, /* Not allow GRE, IPoMPLS or MPLS VPN routed by
                                                   inner header. */
    /* 96 */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV4_FRAG,  /* IP tunnel packet outer IPv4 fragment offset is
                                                     not 0. */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV4_OPT,   /* IP tunnel packet outer IPv4 has options. */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV4_AH,    /* IP tunnel packet outer IPv4 has authentication
                                                        header. */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV4_TTL_0, /* IP tunnel packet outer IPv4 TTL is 0. */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV4_TTL_1, /* IP tunnel packet outer IPv4 TTL is 1. */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV6_FRAG,  /* IP tunnel packet outer IPv6 fragment offset is
                                                     not 0. */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV6_OPT, /* IP tunnel packet outer IPv6 has extension header.
                                                 */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV6_AH,  /* IP tunnel packet outer IPv6 has authentication
                                                   header. */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV6_TTL_0,  /* IP tunnel packet outer IPv6 TTL is 0. */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_IPV6_TTL_1,  /* IP tunnel packet outer IPv6 TTL is 1. */
    CLX_PKT_RX_REASON_IPUC_TUNNEL_INNER_VLAN_MISS, /* IP unicast tunnel packet inner VLAN miss. */
    CLX_PKT_RX_REASON_IPMC_TUNNEL_INNER_VLAN_MISS, /* IP multicast tunnel packet inner VLAN miss. */
    CLX_PKT_RX_REASON_AUTO_TUNNEL_DIP_MISS,        /* Auto tunnel outer IPv4 and inner IPv6 DIP are
                                                      unmatched. */
    CLX_PKT_RX_REASON_AUTO_TUNNEL_SIP_MISS,        /* Auto tunnel outer IPv4 and inner IPv6 SIP are
                                                      unmatched. */
    CLX_PKT_RX_REASON_ETHER_IP_VER_ERR, /* Ethernet-within-IP tunnel version is illegal from RFC
                                           3378. */
    CLX_PKT_RX_REASON_GRE_VER_ERR,      /* GRE tunnel version is illegal from RFC 2784. */
    /* 112 */
    CLX_PKT_RX_REASON_GRE_RSVD_NON_ZERO, /* GRE tunnel reserved fields is illegal from RFC 2784. */
    CLX_PKT_RX_REASON_GRE_CTRL_FLAG_ERR, /* GRE tunnel control flag is illegal from RFC 2784. */
    CLX_PKT_RX_REASON_GRE_ERSPAN_TYP2_VER_ERR, /* GRE ERSPAN type II tunnel version is illegal from
                                                  ERSPAN draft. */
    CLX_PKT_RX_REASON_GRE_ERSPAN_TYP3_VER_ERR, /* GRE ERSPAN type III tunnel version is illegal from
                                                  ERSPAN draft. */
    CLX_PKT_RX_REASON_GRE_ERSPAN_TYP3_FT_ERR,  /* GRE ERSPAN type III tunnel frame type is illegal
                                                  from ERSPAN draft.*/
    CLX_PKT_RX_REASON_GRE_ERSPAN_TERM_LKP_MISS, /* GRE ERSPAN tunnel term lookup miss. */
    CLX_PKT_RX_REASON_VXLAN_BAS_RSVD_NON_ZERO, /* VXLAN Basic header reserved fields is illegal from
                                                  RFC. */
    CLX_PKT_RX_REASON_VXLAN_BAS_VNI_FLAG_ERR, /* VXLAN Basic header VNI flag is illegal from RFC. */
    CLX_PKT_RX_REASON_VXLAN_BAS_CTRL_FLAG_ERR, /* VXLAN Basic header control flag is illegal from
                                                  RFC. */
    CLX_PKT_RX_REASON_VXLAN_BAS_UDP_CHKSM_ERR, /* VXLAN Basic header UDP checksum is abnormal. */
    CLX_PKT_RX_REASON_VXLAN_GPE_VNI_FLAG_ERR,  /* VXLAN GRE header VNI flag is illegal from RFC. */
    CLX_PKT_RX_REASON_VXLAN_GPE_CTRL_FLAG_ERR, /* VXLAN GPE header control flag is illegal from RFC.
                                                */
    CLX_PKT_RX_REASON_VXLAN_GPE_UDP_CHKSM_ERR, /* VXLAN GPE header UDP checksum is abnormal. */
    CLX_PKT_RX_REASON_TRILL_VER_ERR,           /* TRILL version is illegal from RFC. */
    CLX_PKT_RX_REASON_TRILL_MC_FLAG_ERR,       /* TRILL multicast flag is unmatched DMAC. */
    CLX_PKT_RX_REASON_TRILL_OPT, /* TRILL packet has option or option header length is illegal. */
    /* 128 */
    CLX_PKT_RX_REASON_TRILL_LKP_MISS, /* TRILL lookup failed with the bank error or entry miss. */
    CLX_PKT_RX_REASON_TRILL_TRANSIT_TTL_0,   /* TRILL transit packet TTL is 0. */
    CLX_PKT_RX_REASON_TRILL_TRANSIT_TTL_1,   /* TRILL transit packet TTL is 1. */
    CLX_PKT_RX_REASON_TRILL_TERM_TTL_0,      /* TRILL term packet TTL is 0. */
    CLX_PKT_RX_REASON_TRILL_TERM_TTL_1,      /* TRILL term packet TTL is 1. */
    CLX_PKT_RX_REASON_TRILL_MRPF_CHECK_FAIL, /* TRILL multicast RPF check failed. */
    CLX_PKT_RX_REASON_NSH_CTRL_PKT,          /* NSH control packets to CPU. */
    CLX_PKT_RX_REASON_NSH_INVALID_PKT,       /* NSH packet is illegal from RFC. */
    CLX_PKT_RX_REASON_NSH_LKP_MISS,   /* NSH lookup failed with the bank error or entry miss. */
    CLX_PKT_RX_REASON_ECMP_LKP_MISS,  /* ECMP lookup failed with the bank error or entry miss. */
    CLX_PKT_RX_REASON_ACL_LKP_MISS,   /* ACL lookup failed with the bank error or entry miss. */
    CLX_PKT_RX_REASON_FLOW_LKP_MISS,  /* Flow lookup failed with the bank error or entry miss. */
    CLX_PKT_RX_REASON_IGR_FLOW_SFLOW, /* Ingress flow-based sflow hit. */
    CLX_PKT_RX_REASON_EGR_FLOW_SFLOW, /* Egress flow-based sflow hit. */
    CLX_PKT_RX_REASON_HW_ERROR,       /* HW error. e.g. ECC. */
    CLX_PKT_RX_REASON_1588_RX_PKT,    /* User spcified the 1588 packets. */
    /* 144 */
    CLX_PKT_RX_REASON_STP_BLOCK,               /* STP block packets to CPU. */
    CLX_PKT_RX_REASON_STACKING_NEIGHBOR,       /* The stacking packets from neighbor. */
    CLX_PKT_RX_REASON_STACKING_BROADCAST,      /* The stacking broadcast packets. */
    CLX_PKT_RX_REASON_STACKING_LOOP,           /* The stacking path is loop. */
    CLX_PKT_RX_REASON_STORM_CONTROL,           /* Storm Control packets to CPU. */
    CLX_PKT_RX_REASON_TUNNEL_INIT_LKP_MISS,    /* Tunnel init lookup failed with bank error or entry
                                                  miss. */
    CLX_PKT_RX_REASON_TUNNEL_INNER_VLAN_MISS,  /* IP Tunnel or TRILL packets miss inner VLAN. */
    CLX_PKT_RX_REASON_TUNNEL_INNER_VLAN_UNEXP, /* IP Tunnel or TRILL packets not allowed inner VLAN.
                                                */
    CLX_PKT_RX_REASON_IGR_L3_MTU_FAIL,         /* Ingress L3 MTU check fail to CPU. */
    CLX_PKT_RX_REASON_EGR_L3_MTU_FAIL,         /* Egress L3 MTU check fail to CPU. */
    CLX_PKT_RX_REASON_IGR_TUNNEL_MTU_FAIL,     /* Ingress Tunnel MTU check fail to CPU. */
    CLX_PKT_RX_REASON_EGR_TUNNEL_MTU_FAIL,     /* Egress Tunnel MTU check fail to CPU. */
    CLX_PKT_RX_REASON_EGR_IPV4_TTL_1,          /* Egress IPv4 interface configure TTL 1. */
    CLX_PKT_RX_REASON_EGR_IPV6_TTL_1,          /* Egress IPv6 interface configure TTL 1. */
    CLX_PKT_RX_REASON_COPY_TO_CPU_L2MC,        /* L2MC Copy to CPU. */
    CLX_PKT_RX_REASON_COPY_TO_CPU_L3MC,        /* L3MC Copy to CPU. */
    /* 160 */
    CLX_PKT_RX_REASON_COPY_TO_CPU_L2UC,          /* L2UC Copy to CPU. */
    CLX_PKT_RX_REASON_COPY_TO_CPU_L3UC,          /* L3UC Copy to CPU. */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_UDP_CHKSM_ERR, /* Flex tunnel packet UDP checksum is abnormal. */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_0_CHK,  /* Flex tunnel 0 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_1_CHK,  /* Flex tunnel 1 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_2_CHK,  /* Flex tunnel 2 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_3_CHK,  /* Flex tunnel 3 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_4_CHK,  /* Flex tunnel 4 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_5_CHK,  /* Flex tunnel 5 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_6_CHK,  /* Flex tunnel 6 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_7_CHK,  /* Flex tunnel 7 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_8_CHK,  /* Flex tunnel 8 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_9_CHK,  /* Flex tunnel 9 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_10_CHK, /* Flex tunnel 10 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_11_CHK, /* Flex tunnel 11 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_12_CHK, /* Flex tunnel 12 with sanity, type, or length check fail.
                                           */
    /* 176 */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_13_CHK, /* Flex tunnel 13 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_14_CHK, /* Flex tunnel 14 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_15_CHK, /* Flex tunnel 15 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_16_CHK, /* Flex tunnel 16 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_17_CHK, /* Flex tunnel 17 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_18_CHK, /* Flex tunnel 18 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_19_CHK, /* Flex tunnel 19 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_20_CHK, /* Flex tunnel 20 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_21_CHK, /* Flex tunnel 21 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_22_CHK, /* Flex tunnel 22 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_FLEX_TUNNEL_23_CHK, /* Flex tunnel 23 with sanity, type, or length check fail.
                                           */
    CLX_PKT_RX_REASON_EGR_SFLOW_HIGH_LATENCY, /* Sample high latency packet, CL8600 only. */
    CLX_PKT_RX_REASON_DPP_LOOPBACK,           /* DPP loopback packet, CL8600 only. */
    CLX_PKT_RX_REASON_PPPOE_SRV_UNKNOWN,      /* PPPOE service unknown, CL8600 only. */
    CLX_PKT_RX_REASON_PPPOE_HDR_ERR,          /* PPPOE header error, CL8600 only. */
    CLX_PKT_RX_REASON_PPPOE_ENCAP_ERR,        /* PPPOE encap error, CL8600 only. */
    /* 192 */
    CLX_PKT_RX_REASON_IP_TUNNEL_IPV4_ERR,         /* IP tunnel IPv4 header error, CL8600 only. */
    CLX_PKT_RX_REASON_IP_TUNNEL_IPV6_ERR,         /* IP tunnel IPv6 header error, CL8600 only. */
    CLX_PKT_RX_REASON_DECAP_NSH_TTL_1,            /* NSH TLL 1 error, CL8600 only. */
    CLX_PKT_RX_REASON_TRANSIT_NSH_TTL_1,          /* NSH TLL 1 error, CL8600 only. */
    CLX_PKT_RX_REASON_PORT_MTR_DROP,              /* Meter over rate drop, CL8600 only. */
    CLX_PKT_RX_REASON_WECMP,                      /* WECMP config error, CL8600 only. */
    CLX_PKT_RX_REASON_IOAM_NODE_LEN_INVALID_IPV6, /* IOAM over IPv6 node len not align, CL8600 only.
                                                   */
    CLX_PKT_RX_REASON_IOAM_NODE_LEN_INVALID_GRE,  /* IOAM over GRE node len not align, CL8600 only.
                                                   */
    CLX_PKT_RX_REASON_IOAM_NODE_LEN_INVALID_GPE,  /* IOAM over GPE node len not align, CL8600 only.
                                                   */
    CLX_PKT_RX_REASON_TUNNEL_MGO_HIT,  /* IP tunnel (*, G) lookup hit copy to CPU, CL8600 only. */
    CLX_PKT_RX_REASON_TUNNEL_MSGO_HIT, /* IP tunnel (S, G) lookup hit copy to CPU, CL8600 only. */
    CLX_PKT_RX_REASON_TUNNEL_SPT_RDY_UNSET,  /* IP tunnel (S, G) ready, notify to update spt ready
                                                bit, CL8600 only. */
    CLX_PKT_RX_REASON_TUNNEL_INVALID_SA,     /* Tunnel header SMAC invalid to CPU, CL8600 only. */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_TTL_0, /* IP tunnel packet outer TTL is 0 to CPU, CL8600 only.
                                              */
    CLX_PKT_RX_REASON_IP_TUNNEL_OUTER_TTL_1, /* IP tunnel packet outer TTL is 1 to CPU, CL8600 only.
                                              */
    CLX_PKT_RX_REASON_IP_TUNNEL_IP_HDR_ERR,  /* IP tunnel pacet IP invalid to CPU, CL8600 only. */
    /* 208 */
    CLX_PKT_RX_REASON_TUNNEL_UNK_PLD,    /* IP tunnel unknown payload to CPU, CL8600 only. */
    CLX_PKT_RX_REASON_TUNNEL_SPTO_BLOCK, /* Underlay STP block packets to CPU, CL8600 only. */
    CLX_PKT_RX_REASON_MPLS_RMAC_MISS,    /* MPLS packtes DMAC not match to CPU, CL8600 only. */
    CLX_PKT_RX_REASON_TUNNEL_MGO_MISS,   /* IP tunnel (*, G) lookup miss to CPU, CL8600 only. */
    CLX_PKT_RX_REASON_TUNNEL_MSGO_MISS,  /* IP tunnel (S, G) lookup miss to CPU, CL8600 only. */
    CLX_PKT_RX_REASON_MPLS_LSP_MISS, /* MPLS packets LSP label lookup miss to CPU, CL8600 only. */
    CLX_PKT_RX_REASON_TUNNEL_SA_DA_BIND,  /* IP tunnel SIP and DIP bind check fail to CPU, CL8600
                                             only. */
    CLX_PKT_RX_REASON_TUNNEL_RPF_CHECK,   /* IP tunnel multicast RPF check fail to CPU, CL8600 only.
                                           */
    CLX_PKT_RX_REASON_GENEVE_VER_ERR,     /* GENEVE packets version is illegal from RFC8926, CL8600
                                             only. */
    CLX_PKT_RX_REASON_GENEVE_CTRL_FLAG_O, /* GENEVE packets O-bit is illegal form RFC8926, CL8600
                                             only. */
    CLX_PKT_RX_REASON_GENEVE_CTRL_FLAG_C, /* GENEVE packets C-bit is illegal form RFC8926, CL8600
                                             only. */
    CLX_PKT_RX_REASON_MPLS_LSP_RSVD,      /* Select RSVD or RSVD1 via the register MPLS_rsvd_excpt,
                                             CL8600 only.*/
    CLX_PKT_RX_REASON_MPLS_LSP_RSVD1,     /* Select RSVD or RSVD1 via the register MPLS_rsvd_excpt,
                                             CL8600 only.*/
    CLX_PKT_RX_REASON_MPLS_ELI_BOS,       /* ELI label is at the bottom, CL8600 only. */
    CLX_PKT_RX_REASON_MPLS_NULL_TOP,      /* NULL label not at the top, CL8600 only. */
    CLX_PKT_RX_REASON_MPLS_EXPOSE_TTL0,   /* VPN or Transit label ttl id 0, CL8600 only. */
    /* 224 */
    CLX_PKT_RX_REASON_MPLS_EXPOSE_TTL1, /* VPN or Transit label ttl id 1, CL8600 only. */
    CLX_PKT_RX_REASON_MPLS_NULL_IP, /* No VPN labe and have NULL label, the inner layer is not the
                                       IP but the Ether, CL8600 only. */
    CLX_PKT_RX_REASON_MPLS_EL_IP, /* Only have LSP and have entroy label, the inner layer is not the
                                     IP but the Ether, CL8600 only. */
    CLX_PKT_RX_REASON_MPLS_LSP_IP,    /* Only have LSP, the inner is not Ether, CL8600 only. */
    CLX_PKT_RX_REASON_SRV6_FUNC_MISS, /* SRv6 packets function lookup miss to CPU, CL8600 only. */
    CLX_PKT_RX_REASON_SRH_ERR,        /* SRH is illegal from RFC, CL8600 only. */
    CLX_PKT_RX_REASON_SRV6_FUNC_REDIRECT_TO_CPU, /* SRv6 packets function match to CPU, CL8600 only.
                                                  */
    CLX_PKT_RX_REASON_SRV6_FLAVOR_ERR,    /* SRv6 packets flavor invalid to CPU, CL8600 only. */
    CLX_PKT_RX_REASON_SRH_SL0_USP,        /* SRv6 USP flavor and SL is 0 to CPU, CL8600 only. */
    CLX_PKT_RX_REASON_SRH_SL0_USP_NO_SRH, /* SRv6 USP flavor and packets has no SRH, CL8600 only. */
    CLX_PKT_RX_REASON_SRH_SL0_USP_TWO_SRH, /* SRv6 USP flavor and packets has two SRH, CL8600 only.
                                            */
    CLX_PKT_RX_REASON_SRH_SL0_NO_POP_SRH, /* SRv6 packet SL is 0 and no USP/USD flavor, CL8600 only.
                                           */
    CLX_PKT_RX_REASON_SRH_SL_NO_ZERO_DECAP, /* SRv6 decapsulatin when SL is not 0, CL8600 only. */
    CLX_PKT_RX_REASON_SRV6_UNK_PLD_DECAP,   /* SRv6 decapsulation when payload invalid, CL8600 only.
                                             */
    CLX_PKT_RX_REASON_B6_INSERT_RCV_REDUCED,     /* SRv6 packets B6.Insert when DIP is not in SRH,
                                                    CL8600 only. */
    CLX_PKT_RX_REASON_B6_INSERT_RCV_DIP_UNMATCH, /* SRv6 packets B6.Insert when DIP not match SRH
                                                    current SID, CL8600 only. */
    /* 240 */
    CLX_PKT_RX_REASON_SRV6_UNK_BEHAVIOR,   /* SRv6 endpoint behavior invalid to CPU, CL8600 only. */
    CLX_PKT_RX_REASON_AUTO_TUNNEL_IP_MISS, /* 6to4 tunnel outer IPv4 and inner IPv6 are unmatched,
                                              CL8600 only. */
    CLX_PKT_RX_REASON_ISATAP_TUNNEL_IP_MISS, /* ISATAP tunnel outer IPv4 and inner IPv6 are
                                                unmatched, CL8600 only.*/
    CLX_PKT_RX_REASON_MPLS_DECAP_BOS,        /* No bottom of the labels stack, CL8600 only. */
    CLX_PKT_RX_REASON_MPLS_PW_CW,            /* Common PWCW packets, CL8600 only. */
    CLX_PKT_RX_REASON_MPLS_PW_ACH,           /* ACH packets, CL8600 only. */
    CLX_PKT_RX_REASON_MPLS_PW_UNK,           /* PWCW parsing error, CL8600 only. */
    CLX_PKT_RX_REASON_MPLS_L3VPN_IP,         /* L3VPN scenario, the inner is ether, CL8600 only. */
    CLX_PKT_RX_REASON_BDI_HSH_MISS,  /* Hash search for inner bridge domain failed, CL8600 only. */
    CLX_PKT_RX_REASON_IGMP_SNOOPING, /* IGMP snooping packets, CL8600 only. */
    CLX_PKT_RX_REASON_MLD_SNOOPING,  /* MLD snooping packets, CL8600 only. */
    CLX_PKT_RX_REASON_TUNNEL_ECN,    /* Used Ecn map of tunnel decap: ECT(1) (!), CL8600 only. */
    CLX_PKT_RX_REASON_TUNNEL_ECN_2,  /* Used Ecn map of tunnel decap: Not-ECT (!!!), CL8600 only. */
    CLX_PKT_RX_REASON_TUNNEL_ECN_3,  /* Used Ecn map of tunnel decap: drop (!!!), CL8600 only. */
    CLX_PKT_RX_REASON_MPLS_TRANSIT_LBL_MISS, /* MPLS transit label not hit, CL8600 only. */
    CLX_PKT_RX_REASON_L2_SA_MISS_1,          /* L2 source address miss, CL8600 only. */
    /* 256 */
    CLX_PKT_RX_REASON_INT_SRC_RCV_INT_ENCAP, /* INT sourrc receive igr with INT encap, CL8600 only.
                                              */
    CLX_PKT_RX_REASON_UC_SOURCE_PRUNING,     /* Unicast source prune, CL8600 only. */
    CLX_PKT_RX_REASON_UC_SPLITHORIZON_CHECK, /* Tunnel farward to tunnel by L2UC, CL8600 only. */
    CLX_PKT_RX_REASON_IP_TTL_0,              /* Ip header TTL is 0, CL8600 only. */
    CLX_PKT_RX_REASON_IP_TTL_1,              /* Ip header TTL is 0, CL8600 only. */
    CLX_PKT_RX_REASON_TELM_OVER_UNSUPPORTED_TYPE, /* Source role ingress is not valid L4
                                                     type(TCP/UDP/GRE), and tnl_idx is 0, CL8600
                                                     only. */
    CLX_PKT_RX_REASON_EVPN_ESI_FILTER,            /* EVPN esi filter to CPU, CL8600 only. */
    CLX_PKT_RX_REASON_EVPN_DF_FILTER,             /* EVPN DF filter to CPU, CL8600 only. */
    CLX_PKT_RX_REASON_VLAN_HSH_MISS, /* Hash search for VLAN editing failed, CL8600 only. */
    CLX_PKT_RX_REASON_IGR_NOT_PVLAN_PORT_PORT, /* Inbound port is a non PVLAN port, CL8600 only. */
    CLX_PKT_RX_REASON_EGR_PVLAN_PORT_TYPE_CHECK, /* Check PVLAN role on outbound port, CL8600 only.
                                                  */
    CLX_PKT_RX_REASON_INT_OVER_UNSUPPORTED_TYPE, /* INT source ingress is not valid L4
                                                    type(TCP/UDP/GRE), and tnl_idx is 0, CL8600
                                                    only. */
    CLX_PKT_RX_REASON_MC_SOURCE_PRUNING,         /* Multicast source prune, CL8600 only. */
    CLX_PKT_RX_REASON_MC_SPLITHORIZON_CHECK, /* Tunnel farward to tunnel by L2UC, CL8600 only. */
    CLX_PKT_RX_REASON_MC_L3VPN_PRUNING,      /* L3 VPN source pruning with same l3 interface, CL8600
                                                only. */
    CLX_PKT_RX_REASON_IOAM_OVER_UNSUPPORTED_TYPE, /* IOAM source ingress is not IPv4/IPv6 and
                                                     tnl_idx is 0, CL8600 only.*/
    /* 272 */
    CLX_PKT_RX_REASON_INT_MTU_EXCEED,  /* INT MTU exceed, CL8600 only. */
    CLX_PKT_RX_REASON_INT_META_INCOMP, /* Ingress packet header is oversize, making it impossible to
                                          fully decap INT, CL8600 only. */
    CLX_PKT_RX_REASON_TELM_NONEXIST_DECAP, /* CUD type is decap action, but ingress packet has no
                                              TELM header, CL8600 only. */
    CLX_PKT_RX_REASON_INT_OVER_UNSUPPORTED_TNL,  /* INT source role cfg encap TNL type is
                                                     Non-VxlanGpe/Geneve/GRE, CL8600 only. */
    CLX_PKT_RX_REASON_IOAM_OVER_UNSUPPORTED_TNL, /* IOAM source role cfg encap tnl type is
                                                    Non-Vxlan, CL8600 only. */
    CLX_PKT_RX_REASON_MPLS_PHP_BOS_ETH, /* PHP have no mpls label stack, but inner is not IP, CL8600
                                           only. */
    CLX_PKT_RX_REASON_EGR_INC_OVERFLOW, /* Egress packet size overflow, CL8600 only. */
    CLX_PKT_RX_REASON_URPF_CHECK_FAIL_DROP, /* L3 URPF check fail drop, CL8600 only. */
    CLX_PKT_RX_REASON_L3MC_RPF_CHECK_DROP,  /* L3 RPF check fail drop, CL8600 only. */
    CLX_PKT_RX_REASON_PORT_HIGH_LATENCY,    /* Port high latency packet, CL8600 only. */
    CLX_PKT_RX_REASON_ACL_DROP,             /* ACL drop, CL8600 only. */
    CLX_PKT_RX_REASON_IGR_BDI_SPT,          /* Ingress bridge domain spanning tree, CL8600 only. */
    CLX_PKT_RX_REASON_EGR_BDI_SPT,          /* Egress bridge domain spanning tree, CL8600 only. */
    CLX_PKT_RX_REASON_PSR_PKT_INC,          /* Parse packet incompatible. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_0,  /* ctrl2cpu reason for entry 0, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_1,  /* ctrl2cpu reason for entry 1, CL8600 only. */
    /* 288 */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_2,  /* ctrl2cpu reason for entry 2, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_3,  /* ctrl2cpu reason for entry 3, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_4,  /* ctrl2cpu reason for entry 4, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_5,  /* ctrl2cpu reason for entry 5, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_6,  /* ctrl2cpu reason for entry 6, CL8600 only */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_7,  /* ctrl2cpu reason for entry 7, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_8,  /* ctrl2cpu reason for entry 8, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_9,  /* ctrl2cpu reason for entry 9, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_10, /* ctrl2cpu reason for entry 10, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_11, /* ctrl2cpu reason for entry 11, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_12, /* ctrl2cpu reason for entry 12, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_13, /* ctrl2cpu reason for entry 13, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_14, /* ctrl2cpu reason for entry 14, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_15, /* ctrl2cpu reason for entry 15, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_16, /* ctrl2cpu reason for entry 16, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_17, /* ctrl2cpu reason for entry 17, CL8600 only. */
    /* 304 */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_18, /* ctrl2cpu reason for entry 18, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_19, /* ctrl2cpu reason for entry 19, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_20, /* ctrl2cpu reason for entry 20, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_21, /* ctrl2cpu reason for entry 21, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_22, /* ctrl2cpu reason for entry 22, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_23, /* ctrl2cpu reason for entry 23, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_24, /* ctrl2cpu reason for entry 24, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_25, /* ctrl2cpu reason for entry 25, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_26, /* ctrl2cpu reason for entry 26, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_27, /* ctrl2cpu reason for entry 27, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_28, /* ctrl2cpu reason for entry 28, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_29, /* ctrl2cpu reason for entry 29, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_30, /* ctrl2cpu reason for entry 30, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_31, /* ctrl2cpu reason for entry 31, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_32, /* ctrl2cpu reason for entry 32, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_33, /* ctrl2cpu reason for entry 33, CL8600 only. */
    /* 320 */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_34, /* ctrl2cpu reason for entry 34, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_35, /* ctrl2cpu reason for entry 35, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_36, /* ctrl2cpu reason for entry 36, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_37, /* ctrl2cpu reason for entry 37, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_38, /* ctrl2cpu reason for entry 38, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_39, /* ctrl2cpu reason for entry 39, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_40, /* ctrl2cpu reason for entry 40, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_41, /* ctrl2cpu reason for entry 41, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_42, /* ctrl2cpu reason for entry 42, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_43, /* ctrl2cpu reason for entry 43, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_44, /* ctrl2cpu reason for entry 44, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_45, /* ctrl2cpu reason for entry 45, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_46, /* ctrl2cpu reason for entry 46, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_47, /* ctrl2cpu reason for entry 47, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_48, /* ctrl2cpu reason for entry 48, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_49, /* ctrl2cpu reason for entry 49, CL8600 only. */
    /* 336 */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_50, /* ctrl2cpu reason for entry 50, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_51, /* ctrl2cpu reason for entry 51, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_52, /* ctrl2cpu reason for entry 52, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_53, /* ctrl2cpu reason for entry 53, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_54, /* ctrl2cpu reason for entry 54, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_55, /* ctrl2cpu reason for entry 55, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_56, /* ctrl2cpu reason for entry 56, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_57, /* ctrl2cpu reason for entry 57, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_58, /* ctrl2cpu reason for entry 58, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_59, /* ctrl2cpu reason for entry 59, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_60, /* ctrl2cpu reason for entry 60, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_61, /* ctrl2cpu reason for entry 61, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_62, /* ctrl2cpu reason for entry 62, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_63, /* ctrl2cpu reason for entry 63, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_64, /* ctrl2cpu reason for entry 64, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_65, /* ctrl2cpu reason for entry 65, CL8600 only. */
    /* 352 */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_66, /* ctrl2cpu reason for entry 66, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_67, /* ctrl2cpu reason for entry 67, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_68, /* ctrl2cpu reason for entry 68, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_69, /* ctrl2cpu reason for entry 69, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_70, /* ctrl2cpu reason for entry 70, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_71, /* ctrl2cpu reason for entry 71, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_72, /* ctrl2cpu reason for entry 72, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_73, /* ctrl2cpu reason for entry 73, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_74, /* ctrl2cpu reason for entry 74, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_75, /* ctrl2cpu reason for entry 75, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_76, /* ctrl2cpu reason for entry 76, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_77, /* ctrl2cpu reason for entry 77, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_78, /* ctrl2cpu reason for entry 78, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_79, /* ctrl2cpu reason for entry 79, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_80, /* ctrl2cpu reason for entry 80, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_81, /* ctrl2cpu reason for entry 81, CL8600 only. */
    /* 368 */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_82, /* ctrl2cpu reason for entry 82, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_83, /* ctrl2cpu reason for entry 83, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_84, /* ctrl2cpu reason for entry 84, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_85, /* ctrl2cpu reason for entry 85, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_86, /* ctrl2cpu reason for entry 86, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_87, /* ctrl2cpu reason for entry 87, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_88, /* ctrl2cpu reason for entry 88, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_89, /* ctrl2cpu reason for entry 89, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_90, /* ctrl2cpu reason for entry 90, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_91, /* ctrl2cpu reason for entry 91, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_92, /* ctrl2cpu reason for entry 92, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_93, /* ctrl2cpu reason for entry 93, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_94, /* ctrl2cpu reason for entry 94, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_95, /* ctrl2cpu reason for entry 95, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_96, /* ctrl2cpu reason for entry 96, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_97, /* ctrl2cpu reason for entry 97, CL8600 only. */
    /* 384 */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_98,  /* ctrl2cpu reason for entry 98, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_99,  /* ctrl2cpu reason for entry 99, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_100, /* ctrl2cpu reason for entry 100, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_101, /* ctrl2cpu reason for entry 101, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_102, /* ctrl2cpu reason for entry 102, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_103, /* ctrl2cpu reason for entry 103, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_104, /* ctrl2cpu reason for entry 104, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_105, /* ctrl2cpu reason for entry 105, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_106, /* ctrl2cpu reason for entry 106, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_107, /* ctrl2cpu reason for entry 107, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_108, /* ctrl2cpu reason for entry 108, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_109, /* ctrl2cpu reason for entry 109, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_110, /* ctrl2cpu reason for entry 110, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_111, /* ctrl2cpu reason for entry 111, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_112, /* ctrl2cpu reason for entry 112, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_113, /* ctrl2cpu reason for entry 113, CL8600 only. */
    /* 400 */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_114, /* ctrl2cpu reason for entry 114, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_115, /* ctrl2cpu reason for entry 115, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_116, /* ctrl2cpu reason for entry 116, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_117, /* ctrl2cpu reason for entry 117, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_118, /* ctrl2cpu reason for entry 118, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_119, /* ctrl2cpu reason for entry 119, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_120, /* ctrl2cpu reason for entry 120, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_121, /* ctrl2cpu reason for entry 121, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_122, /* ctrl2cpu reason for entry 122, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_123, /* ctrl2cpu reason for entry 123, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_124, /* ctrl2cpu reason for entry 124, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_125, /* ctrl2cpu reason for entry 125, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_126, /* ctrl2cpu reason for entry 126, CL8600 only. */
    CLX_PKT_RX_REASON_CTRL_TO_CPU_ENTRY_127, /* ctrl2cpu reason for entry 127, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_0,            /* cia copy reason 0, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_1,            /* cia copy reason 1, CL8600 only. */
    /* 416 */
    CLX_PKT_RX_REASON_CIA_COPY_2,  /* cia copy reason 2, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_3,  /* cia copy reason 3, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_4,  /* cia copy reason 4, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_5,  /* cia copy reason 5, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_6,  /* cia copy reason 6, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_7,  /* cia copy reason 7, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_8,  /* cia copy reason 8, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_9,  /* cia copy reason 9, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_10, /* cia copy reason 10, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_11, /* cia copy reason 11, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_12, /* cia copy reason 12, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_13, /* cia copy reason 13, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_14, /* cia copy reason 14, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_15, /* cia copy reason 15, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_16, /* cia copy reason 16, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_17, /* cia copy reason 17, CL8600 only. */
    /* 432 */
    CLX_PKT_RX_REASON_CIA_COPY_18, /* cia copy reason 18, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_19, /* cia copy reason 19, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_20, /* cia copy reason 20, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_21, /* cia copy reason 21, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_22, /* cia copy reason 22, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_23, /* cia copy reason 23, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_24, /* cia copy reason 24, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_25, /* cia copy reason 25, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_26, /* cia copy reason 26, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_27, /* cia copy reason 27, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_28, /* cia copy reason 28, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_29, /* cia copy reason 29, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_30, /* cia copy reason 30, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_31, /* cia copy reason 31, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_32, /* cia copy reason 32, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_33, /* cia copy reason 33, CL8600 only. */
    /* 448 */
    CLX_PKT_RX_REASON_CIA_COPY_34, /* cia copy reason 34, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_35, /* cia copy reason 35, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_36, /* cia copy reason 36, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_37, /* cia copy reason 37, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_38, /* cia copy reason 38, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_39, /* cia copy reason 39, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_40, /* cia copy reason 40, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_41, /* cia copy reason 41, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_42, /* cia copy reason 42, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_43, /* cia copy reason 43, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_44, /* cia copy reason 44, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_45, /* cia copy reason 45, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_46, /* cia copy reason 46, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_COPY_47, /* cia copy reason 47, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_0,  /* cia trap reason 0. */
    CLX_PKT_RX_REASON_CIA_TRAP_1,  /* cia trap reason 1. */
    /* 464 */
    CLX_PKT_RX_REASON_CIA_TRAP_2,  /* cia trap reason 2. */
    CLX_PKT_RX_REASON_CIA_TRAP_3,  /* cia trap reason 3. */
    CLX_PKT_RX_REASON_CIA_TRAP_4,  /* cia trap reason 4. */
    CLX_PKT_RX_REASON_CIA_TRAP_5,  /* cia trap reason 5. */
    CLX_PKT_RX_REASON_CIA_TRAP_6,  /* cia trap reason 6. */
    CLX_PKT_RX_REASON_CIA_TRAP_7,  /* cia trap reason 7. */
    CLX_PKT_RX_REASON_CIA_TRAP_8,  /* cia trap reason 8. */
    CLX_PKT_RX_REASON_CIA_TRAP_9,  /* cia trap reason 9. */
    CLX_PKT_RX_REASON_CIA_TRAP_10, /* cia trap reason 10. */
    CLX_PKT_RX_REASON_CIA_TRAP_11, /* cia trap reason 11. */
    CLX_PKT_RX_REASON_CIA_TRAP_12, /* cia trap reason 12. */
    CLX_PKT_RX_REASON_CIA_TRAP_13, /* cia trap reason 13. */
    CLX_PKT_RX_REASON_CIA_TRAP_14, /* cia trap reason 14. */
    CLX_PKT_RX_REASON_CIA_TRAP_15, /* cia trap reason 15. */
    CLX_PKT_RX_REASON_CIA_TRAP_16, /* cia trap reason 16. */
    CLX_PKT_RX_REASON_CIA_TRAP_17, /* cia trap reason 17. */
    /* 480 */
    CLX_PKT_RX_REASON_CIA_TRAP_18, /* cia trap reason 18. */
    CLX_PKT_RX_REASON_CIA_TRAP_19, /* cia trap reason 19. */
    CLX_PKT_RX_REASON_CIA_TRAP_20, /* cia trap reason 20. */
    CLX_PKT_RX_REASON_CIA_TRAP_21, /* cia trap reason 21. */
    CLX_PKT_RX_REASON_CIA_TRAP_22, /* cia trap reason 22. */
    CLX_PKT_RX_REASON_CIA_TRAP_23, /* cia trap reason 23. */
    CLX_PKT_RX_REASON_CIA_TRAP_24, /* cia trap reason 24, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_25, /* cia trap reason 25, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_26, /* cia trap reason 26, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_27, /* cia trap reason 27, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_28, /* cia trap reason 28, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_29, /* cia trap reason 29, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_30, /* cia trap reason 30, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_31, /* cia trap reason 31, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_32, /* cia trap reason 32, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_33, /* cia trap reason 33, CL8600 only. */
    /* 496 */
    CLX_PKT_RX_REASON_CIA_TRAP_34, /* cia trap reason 34, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_35, /* cia trap reason 35, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_36, /* cia trap reason 36, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_37, /* cia trap reason 37, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_38, /* cia trap reason 38, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_39, /* cia trap reason 39, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_40, /* cia trap reason 40, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_41, /* cia trap reason 41, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_42, /* cia trap reason 42, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_43, /* cia trap reason 43, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_44, /* cia trap reason 44, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_45, /* cia trap reason 45, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_46, /* cia trap reason 46, CL8600 only. */
    CLX_PKT_RX_REASON_CIA_TRAP_47, /* cia trap reason 47, CL8600 only. */
    CLX_PKT_RX_REASON_MOD,         /* mod reason */
    CLX_PKT_RX_REASON_OTHERS,      /* None of the above reasons */
    CLX_PKT_RX_REASON_LAST
} CLX_PKT_RX_REASON_T;

#define CLX_PKT_RX_REASON_BITMAP_SIZE (CLX_BITMAP_SIZE(CLX_PKT_RX_REASON_LAST))

/* The data structure is the reason bitmap for "to CPU" reason codes. */
typedef UI32_T CLX_PKT_RX_REASON_BITMAP_T[CLX_PKT_RX_REASON_BITMAP_SIZE];

#define CLX_PKT_SET_REASONTBIT(reason_bmp, reason) \
    (((UI32_T *)(reason_bmp))[(reason) / 32] |= (1UL << ((reason) % 32)))
#define CLX_PKT_CLR_REASONTBIT(reason_bmp, reason) \
    (((UI32_T *)(reason_bmp))[(reason) / 32] &= ~(1UL << ((reason) % 32)))
#define CLX_PKT_CHK_REASONTBIT(reason_bmp, reason) \
    (((UI32_T *)(reason_bmp))[(reason) / 32] & (1UL << ((reason) % 32)))

#define CLX_PKT_FOREACH_REASONBIT(clx_reason_bitmap, reason)    \
    for (reason = 0; reason < CLX_PKT_RX_REASON_LAST; reason++) \
        if (CLX_PKT_CHK_REASONTBIT(clx_reason_bitmap, reason))

/* Packet Drop Reasons */
typedef enum {
    CLX_PKT_DROP_REASON_NONE = 0,
    CLX_PKT_DROP_REASON_ECC_ERR,
    CLX_PKT_DROP_REASON_IPL_ERR_FRM,
    CLX_PKT_DROP_REASON_IPL_CRC_ERR,
    CLX_PKT_DROP_REASON_IPL_MAX_LEN_VLT,
    CLX_PKT_DROP_REASON_IPL_JABBER_FRM,
    CLX_PKT_DROP_REASON_IPL_LEN_ERR,
    CLX_PKT_DROP_REASON_IPL_CONST_SOP,
    CLX_PKT_DROP_REASON_IPL_OVER_MTU,
    CLX_PKT_DROP_REASON_IPL_EOP_TIME_OUT,
    CLX_PKT_DROP_REASON_IPL_BUF_OVF,
    CLX_PKT_DROP_REASON_LBM_FRG_DROP,
    CLX_PKT_DROP_REASON_LBM_TRUN_ERR,
    CLX_PKT_DROP_REASON_LBM_RECVD_ERR,
    CLX_PKT_DROP_REASON_L2_IGR_MTU_ERR = 16,
    CLX_PKT_DROP_REASON_L2_EGR_MTU_ERR,
    CLX_PKT_DROP_REASON_L2_MC_SOURCE_PRUNING,
    CLX_PKT_DROP_REASON_L2_UC_SOURCE_PRUNING,
    CLX_PKT_DROP_REASON_L2_BDI_SPT_DROP,
    CLX_PKT_DROP_REASON_L3_LKUP_TYPE_ERR_DROP,
    CLX_PKT_DROP_REASON_L3_MTU_ERR,
    CLX_PKT_DROP_REASON_IP_SG_DROP,
    CLX_PKT_DROP_REASON_MC_L3VPN_PRUNING_DROP,
    CLX_PKT_DROP_REASON_L3_DIP_MISS_DROP,
    CLX_PKT_DROP_REASON_L3_RPF_CHECK_FAIL_DROP,
    CLX_PKT_DROP_REASON_L3UC_FWD_PTR_DROP,
    CLX_PKT_DROP_REASON_ECMP_ERR_DROP,
    CLX_PKT_DROP_REASON_IP_TTL0_DROP,
    CLX_PKT_DROP_REASON_VLAN_PKT_TAG_FMT_ERR,
    CLX_PKT_DROP_REASON_VLAN_HSH_MISS_DROP,
    CLX_PKT_DROP_REASON_TUNNEL_OUTER_HDR_ERR,
    CLX_PKT_DROP_REASON_EVPN_ESI_FILTER,
    CLX_PKT_DROP_REASON_EVPN_DF_FILTER,
    CLX_PKT_DROP_REASON_SRV6_HDR_ERR,
    CLX_PKT_DROP_REASON_MPLS_TTL_DROP,
    CLX_PKT_DROP_REASON_MPLS_RSVD1_DROP,
    CLX_PKT_DROP_REASON_MPLS_LKP_MISS_DROP,
    CLX_PKT_DROP_REASON_METER_IGR_DROP,
    CLX_PKT_DROP_REASON_METER_EGR_DROP,
    CLX_PKT_DROP_REASON_STORM_DROP,
    CLX_PKT_DROP_REASON_DOS_CHK_ERR,
    CLX_PKT_DROP_REASON_SEC_DROP,
    CLX_PKT_DROP_REASON_ACL_IGR_DROP,
    CLX_PKT_DROP_REASON_ACL_EGR_DROP,
    CLX_PKT_DROP_REASON_PSR_PKT_ERR,
    CLX_PKT_DROP_REASON_PDB_WR_ERR,
    CLX_PKT_DROP_REASON_ASM_PKTSZ_ERR,
    CLX_PKT_DROP_REASON_REP_MC_CONGEST,
    CLX_PKT_DROP_REASON_REP_MC_MBR_DROP,
    CLX_PKT_DROP_REASON_REP_DELETE_PORT,
    CLX_PKT_DROP_REASON_REP_CHASSIS_PRUN,
    CLX_PKT_DROP_REASON_REP_INT_PRUN,
    CLX_PKT_DROP_REASON_REP_BLACKHOLE_DI,
    CLX_PKT_DROP_REASON_REP_OUTRANGE_DI,
    CLX_PKT_DROP_REASON_REP_LAG_SIZE_ERR,
    CLX_PKT_DROP_REASON_REP_FL_SIZE_ERR,
    CLX_PKT_DROP_REASON_REP_BYPASS_DI,
    CLX_PKT_DROP_REASON_REP_CODE_ERR,
    CLX_PKT_DROP_REASON_REP_OQ_OUT_RANGE_ERR,
    CLX_PKT_DROP_REASON_BAC_SRC_CONGEST,
    CLX_PKT_DROP_REASON_BAC_DST_CONGEST,
    CLX_PKT_DROP_REASON_OQ_ENQ_FAIL,
    CLX_PKT_DROP_REASON_LAST = 64,
} CLX_PKT_DROP_REASON_T;

/* Key Value, IPv6 */
typedef struct {
    CLX_IPV6_T dip;         /* 128-bit IPv6 destination IP Address                  */
    CLX_IPV6_T dip_mask;    /* Hit if packet.dip equals to (dip & dip_mask).        */
    CLX_IPV6_T dip_max;     /* Hit if packet.dip within [dip, dip_max].             */

    UI8_T next_header;      /* IPv6 next header                                     */
    UI8_T next_header_mask; /* It is used to mask IPv6 next header.                 */

    UI16_T dst_port;        /* ipv6 l4 dst port                                     */
    UI16_T dst_port_mask;   /* used to mask ipv6 l4 dst port                        */
    UI16_T src_port;        /* ipv6 l4 src port                                     */
    UI16_T src_port_mask;   /* used to mask ipv6 l4 src port                        */
    UI8_T icmp_type;        /* ipv6 icmp type                                       */
    UI8_T icmp_type_mask;   /* used to mask ipv6 icmp type                          */
    UI8_T icmp_code;        /* ipv6 icmp code                                       */
    UI8_T icmp_code_mask;   /* used to mask ipv6 icmp code                          */

    /* CLX_PKT_CTRL_TO_CPU_IPV6_FLAGS_XXX_CHECK works as the mask of
     * CLX_PKT_CTRL_TO_CPU_IPV6_FLAGS_XXX_EN */

/* The Mac of IPv6 Packet is My Router Mac.                 */
#define CLX_PKT_CTRL_TO_CPU_IPV6_FLAGS_MY_ROUTER_MAC_EN (1 << 0)
/* Check the Mac of IPv6 Packet is My Router Mac or not.    */
#define CLX_PKT_CTRL_TO_CPU_IPV6_FLAGS_MY_ROUTER_MAC_CHECK (1 << 1)
/* The IPv6 route is enable.                                */
#define CLX_PKT_CTRL_TO_CPU_IPV6_FLAGS_ROUTE_EN (1 << 2)
/* Check the IPv6 route is enable or not.                   */
#define CLX_PKT_CTRL_TO_CPU_IPV6_FLAGS_ROUTE_CHECK (1 << 3)
/* If true: icmp type/code, Else: TCP/UDP/SCTP dport.       */
#define CLX_PKT_CTRL_TO_CPU_IPV6_FLAGS_ICMP_EN (1 << 4)
/* If true: key is dip-to-dip_max, Else: dip & dip_mask     */
#define CLX_PKT_CTRL_TO_CPU_IPV6_FLAGS_DIP_RANGE_EN (1 << 5)
/* [CL8600 only] IPv6 Packet with Header Option is allowed.   */
#define CLX_PKT_CTRL_TO_CPU_IPV6_FLAGS_HEADER_OPTION_EN (1 << 6)
/* [CL8600 only] Check IPv6 Packet with Header Option or not. */
#define CLX_PKT_CTRL_TO_CPU_IPV6_FLAGS_HEADER_OPTION_CHECK (1 << 7)

    UI32_T flags;                    /* used to set ipv6 flag                                */
    CLX_PKT_TUNNEL_TYPE_T term_type; /* ipv6 tunnel type                                */
    CLX_BRIDGE_DOMAIN_T bdid;        /* [CL8600 only] bridge domain id                       */
    CLX_BRIDGE_DOMAIN_T bdid_mask;   /* [CL8600 only] used to mask bridge domain id          */
} CLX_PKT_CTRL_TO_CPU_IPV6_KEY_T;

/* Key Value, IPv4 */
typedef struct {
    CLX_IPV4_T dip;         /* 32-bit IPv4 destination IP Address                   */
    CLX_IPV4_T dip_mask;    /* Hit if packet.dip equals to (dip & dip_mask).        */
    CLX_IPV4_T dip_max;     /* Hit if packet.dip within [dip, dip_max].             */

    UI8_T protocol_id;      /* Protocol ID                                          */
    UI8_T protocol_id_mask; /* It is used to mask the protocol ID.                  */

    UI16_T dst_port;        /* ipv4 l4 dst port                                     */
    UI16_T dst_port_mask;   /* used to mask ipv4 l4 dst port                        */
    UI16_T src_port;        /* ipv4 l4 src port                                     */
    UI16_T src_port_mask;   /* used to mask ipv4 l4 src port                        */
    UI8_T icmp_type;        /* ipv4 icmp type                                       */
    UI8_T icmp_type_mask;   /* used to mask ipv4 icmp type                          */
    UI8_T icmp_code;        /* ipv4 icmp code                                       */
    UI8_T icmp_code_mask;   /* used to mask ipv4 icmp code                          */

    /* CLX_PKT_CTRL_TO_CPU_IPV4_FLAGS_XXX_CHECK works as the mask of
     * CLX_PKT_CTRL_TO_CPU_IPV4_FLAGS_XXX_EN */

/* The Mac of IPv4 Packet is My Router Mac.               */
#define CLX_PKT_CTRL_TO_CPU_IPV4_FLAGS_MY_ROUTER_MAC_EN (1 << 0)
/* Check the Mac of IPv4 Packet is My Router Mac or not.  */
#define CLX_PKT_CTRL_TO_CPU_IPV4_FLAGS_MY_ROUTER_MAC_CHECK (1 << 1)
/* The IPv4 route is enable.                              */
#define CLX_PKT_CTRL_TO_CPU_IPV4_FLAGS_ROUTE_EN (1 << 2)
/* Check the IPv4 route is enable or not.                 */
#define CLX_PKT_CTRL_TO_CPU_IPV4_FLAGS_ROUTE_CHECK (1 << 3)
/* IPv4 Packet with Header Option is allowed.             */
#define CLX_PKT_CTRL_TO_CPU_IPV4_FLAGS_HEADER_OPTION_EN (1 << 4)
/* Check IPv4 Packet with Header Option or not.           */
#define CLX_PKT_CTRL_TO_CPU_IPV4_FLAGS_HEADER_OPTION_CHECK (1 << 5)
/* IPv4 Packet with Header Fragment is allowed.           */
#define CLX_PKT_CTRL_TO_CPU_IPV4_FLAGS_HEADER_FRAGMENT_EN (1 << 6)
/* Check IPv4 Packet with Header Fragment or not.         */
#define CLX_PKT_CTRL_TO_CPU_IPV4_FLAGS_HEADER_FRAGMENT_CHECK (1 << 7)
/* If true: icmp type/code, Else: TCP/UDP/SCTP dport.     */
#define CLX_PKT_CTRL_TO_CPU_IPV4_FLAGS_ICMP_EN (1 << 8)
/* If true: key is dip-to-dip_max, Else: dip & dip_mask   */
#define CLX_PKT_CTRL_TO_CPU_IPV4_FLAGS_DIP_RANGE_EN (1 << 9)

    UI32_T flags;                    /* used to set flag                                     */
    UI8_T header_length;             /* ipv4 header length                                   */
    UI8_T header_length_mask;        /* used to mask ipv4 header length                      */
    CLX_PKT_TUNNEL_TYPE_T term_type; /* ipv4 tunnel type                                     */
    CLX_BRIDGE_DOMAIN_T bdid;        /* [CL8600 only] bridge domain id                       */
    CLX_BRIDGE_DOMAIN_T bdid_mask;   /* [CL8600 only] used to mask bridge domain id          */
} CLX_PKT_CTRL_TO_CPU_IPV4_KEY_T;

/* Key Value, ARP */
typedef struct {
    UI16_T oper;               /* ARP/RARP operation                                   */
    UI16_T oper_mask;          /* ARP/RARP operation                                   */
    CLX_IPV4_T target_ip;      /* Target IP address                                    */
    CLX_IPV4_T target_ip_mask; /* It is used to mask the target IP address.            */

    /* CLX_PKT_CTRL_TO_CPU_ARP_FLAGS_XXX_CHECK works as the mask of
     * CLX_PKT_CTRL_TO_CPU_ARP_FLAGS_XXX_EN */

/* The Mac of ARP Packet is My Router Mac.                  */
#define CLX_PKT_CTRL_TO_CPU_ARP_FLAGS_MY_ROUTER_MAC_EN (1 << 0)
/* Ignore the Mac of ARP Packet is My Router Mac or not.    */
#define CLX_PKT_CTRL_TO_CPU_ARP_FLAGS_MY_ROUTER_MAC_CHECK (1 << 1)
/* [CL8600 only] The L2 mac da is broadcast                 */
#define CLX_PKT_CTRL_TO_CPU_ARP_FLAGS_L2_DA_BC_EN (1 << 2)
/* [CL8600 only] Ignore the l2 mac da is broadcast or not.  */
#define CLX_PKT_CTRL_TO_CPU_ARP_FLAGS_L2_DA_BC_CHECK (1 << 3)
/* [CL8600 only] The l2 mac da is equal to target hard address.               */
#define CLX_PKT_CTRL_TO_CPU_ARP_FLAGS_L2_DA_EQ_THA_EN (1 << 4)
/* [CL8600 only] Ignore the l2 mac da is equal to target hard address or not. */
#define CLX_PKT_CTRL_TO_CPU_ARP_FLAGS_L2_DA_EQ_THA_CHECK (1 << 5)
/* [CL8600 only] The l2 mac sa is equal to sender hard address.               */
#define CLX_PKT_CTRL_TO_CPU_ARP_FLAGS_L2_SA_EQ_SHA_EN (1 << 6)
/* [CL8600 only] Ignore l2 mac sa is equal to sender hard address not.        */
#define CLX_PKT_CTRL_TO_CPU_ARP_FLAGS_L2_SA_EQ_SHA_CHECK (1 << 7)

    UI32_T flags;                    /* used to set flag                                     */
    CLX_PKT_TUNNEL_TYPE_T term_type; /* tunnel type                                          */
    CLX_BRIDGE_DOMAIN_T bdid;        /* [CL8600 only] bridge domain id                       */
    CLX_BRIDGE_DOMAIN_T bdid_mask;   /* [CL8600 only] used to mask bridge domain id          */
    UI16_T eth_type;                 /* [CL8600 only] Ethernet type                          */
    UI16_T eth_type_mask;            /* [CL8600 only] It is used to make the Ethernet type.  */
    CLX_MAC_T da;                    /* [CL8600 only] 48-bit DA                              */
    CLX_MAC_T da_mask;               /* [CL8600 only] It is used to mask the DA.             */
    CLX_MAC_T sa;                    /* [CL8600 only] 48-bit SA                              */
    CLX_MAC_T sa_mask;               /* [CL8600 only] It is used to mask the SA.             */
    CLX_IPV4_T sender_ip;            /* [CL8600 only] Sender IP address                      */
    CLX_IPV4_T sender_ip_mask;       /* [CL8600 only] It is used to mask the sender IP address.*/
} CLX_PKT_CTRL_TO_CPU_ARP_KEY_T;

/* Key Value, L2 */
typedef struct {
    CLX_MAC_T da;         /* 48-bit DA                                            */
    CLX_MAC_T da_mask;    /* It is used to mask the DA.                           */
    UI16_T eth_type;      /* Ethernet type                                        */
    UI16_T eth_type_mask; /* It is used to make the Ethernet type.                */
    UI16_T sub_type;      /* Sub type                                             */
    UI16_T sub_type_mask; /* It is used to mask the sub type.                     */

    /* CLX_PKT_CTRL_TO_CPU_L2_FLAGS_XXX_CHECK works as the mask of
     * CLX_PKT_CTRL_TO_CPU_L2_FLAGS_XXX_EN */

/* The Mac of L2 Packet is My Router Mac.                   */
#define CLX_PKT_CTRL_TO_CPU_L2_FLAGS_MY_ROUTER_MAC_EN (1 << 0)
/* Ignore the Mac of L2 Packet is My Router Mac or not.     */
#define CLX_PKT_CTRL_TO_CPU_L2_FLAGS_MY_ROUTER_MAC_CHECK (1 << 1)
/* The eth-type is < 1500 or Jumbo LLC.                     */
#define CLX_PKT_CTRL_TO_CPU_L2_FLAGS_LLC_EN (1 << 2)
/* Ignore the eth-type check.                               */
#define CLX_PKT_CTRL_TO_CPU_L2_FLAGS_LLC_CHECK (1 << 3)

    UI32_T flags;                    /* used to set flag                                     */
    CLX_PKT_TUNNEL_TYPE_T term_type; /* tunnel type                                          */
    CLX_BRIDGE_DOMAIN_T bdid;        /* [CL8600 only] bridge domain id                       */
    CLX_BRIDGE_DOMAIN_T bdid_mask;   /* [CL8600 only] used to mask bridge domain id          */
} CLX_PKT_CTRL_TO_CPU_L2_KEY_T;

/* Key Value, TUNNEL */
typedef struct {
    CLX_PKT_TUNNEL_TYPE_T term_type; /* Tunnel type                                          */

#define CLX_PKT_CTRL_TO_CPU_MATCH_SIZE (25)

    UI32_T data[CLX_PKT_CTRL_TO_CPU_MATCH_SIZE];
    UI32_T data_mask[CLX_PKT_CTRL_TO_CPU_MATCH_SIZE];
} CLX_PKT_CTRL_TO_CPU_TUNNEL_KEY_T;

/* Key Value */
typedef union {
    CLX_PKT_CTRL_TO_CPU_IPV6_KEY_T ipv6; /* when the key_type is CLX_PKT_KEY_TYPE_IPV6.   */
    CLX_PKT_CTRL_TO_CPU_IPV4_KEY_T ipv4; /* when the key_type is CLX_PKT_KEY_TYPE_IPV4.   */
    CLX_PKT_CTRL_TO_CPU_ARP_KEY_T arp;   /* when the key_type is CLX_PKT_KEY_TYPE_ARP.    */
    CLX_PKT_CTRL_TO_CPU_L2_KEY_T l2;     /* when the key_type is CLX_PKT_KEY_TYPE_L2.     */
    CLX_PKT_CTRL_TO_CPU_TUNNEL_KEY_T
    tunnel; /*[CL8600 not support] when the key_type is CLX_PKT_KEY_TYPE_TUNNEL. */
} CLX_PKT_CTRL_TO_CPU_INKEY_T;

/* Entry of Control-to-CPU Table */
typedef struct {
    /* Key */
    CLX_PKT_KEY_TYPE_T key_type; /* Key for IPv6, IPv4, ARP and L2          */
    CLX_PKT_CTRL_TO_CPU_INKEY_T
    in_key_value; /* CLX_PKT_KEY_TYPE_IPV6   needs to use CLX_PKT_CTRL_TO_CPU_IPV6_KEY_T.
                   * CLX_PKT_KEY_TYPE_IPV4   needs to use CLX_PKT_CTRL_TO_CPU_IPV4_KEY_T.
                   * CLX_PKT_KEY_TYPE_ARP    needs to use CLX_PKT_CTRL_TO_CPU_ARP_KEY_T.
                   * CLX_PKT_KEY_TYPE_L2     needs to use CLX_PKT_CTRL_TO_CPU_L2_KEY_T.
                   * CLX_PKT_KEY_TYPE_TUNNEL needs to use CLX_PKT_CTRL_TO_CPU_TUNNEL_KEY_T.
                   */

    /* Action */
    CLX_FWD_ACTION_T fwd_action; /* It can be forwarded, flooded, dropped, It will be obsoleted by
                                    clx_swc_setRxReasonAction  */
    CLX_PKT_RX_REASON_T
    cpu_reason_code;             /* [CL8600 not support] Self-defined ctrl2cpu reason code       */

    UI32_T ctrl_traffic; /* [CL8600 not support] TRUE: The packet to be forwarded to TM via the head
                            room */
    UI32_T bpdu;         /* TRUE: The packet can be received under MSTP_DISCARD.            */
    UI32_T learn; /* TRUE: Enable the SA learning in L2 forwarding table, It will be obsoleted by
                     clx_swc_setRxReasonAction */
    BOOL_T valid; /* TRUE: It indicates the entry is valid.                          */
} CLX_PKT_CTRL_TO_CPU_ENTRY_T;

/* ----------------------------------------------------------------------------------- Queuing */
/* "To CPU" Priority */
typedef enum {
    CLX_PKT_TO_CPU_PRI_COPY = 0, /* RX copy to CPU priority */
    CLX_PKT_TO_CPU_PRI_REDIRECT, /* RX redirect to CPU priority */
    CLX_PKT_TO_CPU_PRI_LAST
} CLX_PKT_TO_CPU_PRI_T;

/* ----------------------------------------------------------------------------------- CLX PKT */
/* Packet Type */
typedef enum {
    CLX_PKT_TYPE_GENERIC = 0, /* Generic packet */
    CLX_PKT_TYPE_SDN,         /* SDN packet */
    CLX_PKT_TYPE_LAST
} CLX_PKT_TYPE_T;

/* Outer and Inner VLAN IDs */
typedef struct {
    CLX_VLAN_T outer; /* The VLAN of outer tag */
    CLX_VLAN_T inner; /* The VLAN of inner tag */
} CLX_PKT_VLAN_T;

/* Segment ID Type */
typedef enum {
    CLX_PKT_SEG_TYPE_VLAN_DOUBLE = 0, /* The egress segment tag type double vlan              */
    CLX_PKT_SEG_TYPE_VLAN_SINGLE = 1, /* The egress segment tag type single vlan              */
    CLX_PKT_SEG_TYPE_VXLAN = 2,       /* The egress segment tag type vxlan                    */
    CLX_PKT_SEG_TYPE_NVGRE = 3,       /* The egress segment tag type nvgre                    */
    CLX_PKT_SEG_TYPE_LAST
} CLX_PKT_SEG_TYPE_T;

/* Packet Buffer Block */
typedef struct CLX_PKT_BLK_S {
    UI8_T *ptr_buf;                 /* The data buffer pointer                              */
    UI8_T *ptr_dma_addr;            /* used for free dma mem when pph in dma mem */
    UI32_T len;                     /* The length of this block                             */
    struct CLX_PKT_BLK_S *ptr_next; /* The pointer for the next buffer block                */
} CLX_PKT_BLK_T;

/* RX Packet Structure */
typedef struct {
    CLX_PKT_TYPE_T pkt_type; /* Generic or SDN packet                                */
    CLX_COLOR_T color;       /* Color                                                */
    UI32_T tc;               /* Traffic class                                        */

    /* Port */
    CLX_PORT_T igr_phy_port; /* The ingress physical port                            */
    CLX_PORT_T egr_phy_port; /* The egress physical port                             */

    /* Port, LAG, VM or Tunnels */
    CLX_PORT_T igr_port; /* The ingress port                                     */
    CLX_PORT_T egr_port; /* The egress port                                      */

    /* VLAN and Bridge Domain */
    CLX_VLAN_TAG_T igr_tag_type; /* The ingress VLAN tag type                            */
    CLX_BRIDGE_DOMAIN_T bdid;    /* The bridge domain ID                                 */

    /* L3 interface */
    UI32_T igr_intf_id; /* The ingress logical interface                        */
    UI32_T egr_intf_id; /* The egress logical interface                         */

    /* Segment */
    CLX_PKT_SEG_TYPE_T egr_seg_type; /* The egress segment tag type                          */
    UI32_T egr_seg_id;               /* The egress segment ID                                */

    /* VM */
    CLX_VM_TAG_TYPE_T vmtag_type; /* The VM tag type                                      */
    UI32_T vm_id;                 /* The extended port at PE, ingress; otherwise, egress  */

    /* Metas */
    CLX_PKT_RX_REASON_BITMAP_T reason_bitmap; /* The reason bits of the packet */
    UI32_T queue;       /* The queue of the packet                              */
    UI32_T group_label; /* ACL ingress group label                              */
    UI32_T ts_sec;      /* The timestamp in seconds                             */
    UI32_T ts_nsec;     /* The timestamp in nano seconds                        */
    UI32_T hdc_latency; /* high latency */

    /* Stacking */
    UI32_T path; /* The ingress path ID for packet Rx from remote chip   */

    /* SDN */
    CLX_PKT_SDN_PORT_T in_port;     /* Switch input port                                    */
    CLX_PKT_SDN_PORT_T in_phy_port; /* Switch physical input port                           */
    UI8_T table_id;                 /* ID of the table that has been looked up              */
    UI16_T tunnel_header_length;    /* The tunnel header length of the packet               */
    UI64_T flow_id;                 /* ID of the flow that has been looked up               */
    CLX_PKT_SDN_TUNNEL_T tunnel_id; /* Logical port metadata                                */
    UI64_T metadata;                /* Metadata passed between flow tables                  */
    CLX_PKT_SDN_PKT_IN_REASON_T reason_type; /* The reason for the packet sent to the controller */
    UI16_T igr_sflow_sample_point; /* The ingress flow based sflow sample point            */
    UI16_T egr_sflow_sample_point; /* The egress flow based sflow sample point             */

    /* Payload */
    CLX_PKT_BLK_T *ptr_data; /* The packet payload saved block                       */
    UI32_T total_len;        /* Total length of the payload                          */

    /* Flags */
/* The packet is an L3 routed packet.                   */
#define CLX_PKT_RX_PKT_FLAGS_ROUTE (1 << 0)
/* The packet is a bpdu packet.                         */
#define CLX_PKT_RX_PKT_FLAGS_BPDU (1 << 1)
/* The tunnel header should be removed.                 */
#define CLX_PKT_RX_PKT_FLAGS_TUNNEL_TERM (1 << 2)
/* The packet is either ERSPAN or RSPAN mirror packet.  */
#define CLX_PKT_RX_PKT_FLAGS_SPAN_TERM (1 << 3)
/* The packet has been truncated.                       */
#define CLX_PKT_RX_PKT_FLAGS_TRUNCATE (1 << 4)
/* The packet is dropped.                               */
#define CLX_PKT_RX_PKT_FLAGS_DROP (1 << 5)
    UI32_T flags;
} CLX_PKT_RX_PKT_T;

/* ----------------------------------------------------------------------------------- RX */
/* Callback Function for Application to Receive Packets  */
typedef CLX_ERROR_NO_T (*CLX_PKT_RX_FUNC_T)(const UI32_T unit,
                                            const CLX_PKT_RX_PKT_T *ptr_pkt, /* Packet to be
                                                                                processed   */
                                            void *ptr_cookie); /* Private data of user     */

/* RX packet Buffer Allocate Function  */
typedef void *(*CLX_PKT_RX_ALLOC_FUNC_T)(void);

/* RX Packet Buffer Free Function  */
typedef CLX_ERROR_NO_T (*CLX_PKT_RX_FREE_FUNC_T)(const void *ptr_pkt_buf);

/* User Configurable RX Parameter */
typedef struct {
    UI32_T buf_len; /* The size of packet buffer attached to each descriptor     */
    CLX_PKT_RX_ALLOC_FUNC_T rx_alloc; /* RX packet allocation function  */
    CLX_PKT_RX_FREE_FUNC_T rx_free;   /* RX packet free function        */
    CLX_PKT_RX_FUNC_T callback; /* The callback for processing the RX packet                 */
    void *ptr_cookie;           /* The user data for packet callback                         */

#define CLX_PKT_RX_CFG_FLAGS_DEINIT (1 << 0)
    UI32_T flags;
} CLX_PKT_RX_CFG_T;

/* ----------------------------------------------------------------------------------- TX */
/* Callback function type for applications to transmit a packet */
typedef void (*CLX_PKT_TX_FUNC_T)(const UI32_T unit,
                                  void *ptr_pkt,     /* Packet to be processed  */
                                  void *ptr_cookie); /* Private data            */

/*  Tx mode */
typedef enum {
    CLX_PKT_TX_MODE_ETH = 1, /* TABLE_FIRST, normal process   */
    CLX_PKT_TX_MODE_RAW = 3, /* Bypass the IPP and EPP logic  */
    CLX_PKT_TX_MODE_LAST
} CLX_PKT_TX_MODE_T;

/* Tx raw mode */
typedef enum {
    CLX_PKT_TX_RAW_MODE_PORT = 0, /* To local port or remote port  */
    CLX_PKT_TX_RAW_MODE_CPU,      /* To Remote CPU  */
    CLX_PKT_TX_RAW_MODE_MCAST,    /* To multiple local ports in the mcast group  */
    CLX_PKT_TX_RAW_MODE_LAST
} CLX_PKT_TX_RAW_MODE_T;

/* Legacy Packet Structure for Raw Mode. */
typedef struct {
    CLX_PKT_TX_RAW_MODE_T type;                   /* Tx raw mode */
    CLX_PORT_T egr_phy_port;                      /* The destination port */

#define CLX_PKT_TX_RAW_CPU_NEIGHBOR  (0xfffffffe) /* Neighbor Discovery   */
#define CLX_PKT_TX_RAW_CPU_BROADCAST (0xffffffff) /* Broadcast            */
    UI32_T cpu_id;                                /* Remote CPU ID        */
    UI32_T rmt_cpu_queue;                         /* Remote CPU queue     */

    UI32_T mcast_id;                              /* Multicast ID         */

    CLX_COLOR_T color;                            /* Color                */
    UI32_T tc;                                    /* Traffic class        */
    UI8_T pcp;                                    /* Priority Code Point  */

} CLX_PKT_TX_RAW_T;

/* Legacy Packet Structure for Ethernet Mode. */
typedef struct {
    CLX_PORT_T igr_phy_port; /* The source port      */
} CLX_PKT_TX_ETH_T;

/* TX Packet Structure */
typedef struct {
    CLX_PKT_TYPE_T pkt_type;   /* The packet type of the packet sent */

    CLX_PKT_TX_MODE_T tx_mode; /* The Tx mode of the packet sent     */
    UI32_T pkj_journal;        /* The pkj journal: 0-disable 1-enable */
    union {
        CLX_PKT_TX_RAW_T raw;  /* RAW mode: metadata of CLX_PKT_TX_MODE_RAW. */
        CLX_PKT_TX_ETH_T eth;  /* ETH mode: metadata of CLX_PKT_TX_MODE_ETH. */
    };

    CLX_PKT_BLK_T *ptr_data;    /* The packet payload saved block             */
    CLX_PKT_TX_FUNC_T callback; /* Callback function for this packet buffer   */
    void *ptr_cookie;           /* User data for packet callback              */

    UI16_T seq_num;

#define CLX_PKT_TX_PKT_FLAGS_PTP_EN (1 << 0) /* The Precision Time Protocol is enable.     */
/* The 2-step Master Sync message.            */
#define CLX_PKT_TX_PKT_FLAGS_PTP_2STEP_SYNC_MESSAGE (1 << 1)
/* The 1-step Master Sync message.            */
#define CLX_PKT_TX_PKT_FLAGS_PTP_1STEP_SYNC_MESSAGE (1 << 2)
/* The Slave Delay Request message.           */
#define CLX_PKT_TX_PKT_FLAGS_PTP_DELAY_REQUEST (1 << 3)
#define CLX_PKT_TX_PKT_FLAGS_PTP_TC_EN         (1 << 4) /* The Transparent clock is enable.           */
/* The Slave Peer Delay Request message.      */
#define CLX_PKT_TX_PKT_FLAGS_PTP_PEER_DELAY_REQUEST (1 << 5)
/* The Slave Peer Delay Response message.     */
#define CLX_PKT_TX_PKT_FLAGS_PTP_PEER_DELAY_RESPONSE (1 << 6)

    UI32_T flags;
} CLX_PKT_TX_PKT_T;

/* ----------------------------------------------------------------------------------- cnt */
/* TX Cnt */
typedef struct {
    UI32_T packet; /* normal packet */
    UI32_T under_size_err;
    UI32_T over_size_err;

} CLX_PKT_TX_CNT_T;

/* RX Cnt */
typedef struct {
    UI32_T packet; /* normal packet */

} CLX_PKT_RX_CNT_T;

typedef struct {
    /* queue */
    UI32_T enque_ok;    /*enqueue count*/
    UI32_T enque_retry; /*enqueue retry*/
    UI32_T deque_ok;    /*dequeue count*/
    UI32_T deque_fail;  /*dequeue fail count*/

    /* event */
    UI32_T trig_event; /*trigger event*/

    /* normal interrupt */
    UI32_T rx_done; /*rx done interrupt*/

    /* abnormal interrupt */
    UI32_T avbl_gpd_low;   /* Rx GPD.avbl_gpd_num < threshold */
    UI32_T avbl_gpd_empty; /* Rx GPD.avbl_gpd_num = 0 */
    UI32_T avbl_gpd_err;   /* Rx GPD.hwo = 0 */
    UI32_T gpd_chksm_err;  /* Rx GPD.chksm is error */
    UI32_T dma_read_err;   /* DMAR error occurs in PCIE */
    UI32_T dma_write_err;  /* DMAW error occurs in PCIE */
    UI32_T sw_issue_stop;  /* Stop Completion Acknowledge */
    UI32_T gpd_gt255_err;  /* Multi-GPD packet's GPD# > 255 */
    UI32_T tod_uninit;     /* Pdma tod uninit */
    UI32_T pkt_err_drop;   /* Pdma pkt err drop */
    UI32_T udsz_drop;      /* Pdma under size drop */
    UI32_T ovsz_drop;      /* Pdma over size drop */
    UI32_T cmdq_ovf_drop;  /* Pdma cmdq full drop */
    UI32_T fifo_ovf_drop;  /* Pdma fifo full drop */

    /* others */
    UI32_T err_recover; /* Error recover */
    UI32_T ecc_err;     /* Ecc error occur */

#if defined(CLX_EN_NETIF)
    /* it means that user doesn't create intf on that port */
    UI32_T netdev_miss;
#endif

} CLX_PKT_RX_CHANNEL_CNT_T;

typedef struct {
    UI32_T rch_avbl_gpd_no;  /* Rch avbl gpd no */
    UI32_T rch_pfc;          /* Rch pfc */
    UI32_T rch_cmdq_pfc;     /* Rch cmdq pfc */
    UI32_T rch_fifo_pfc;     /* Rch fifo pfc */
    UI32_T rch_avbl_gpd_pfc; /* Rch avbl gpd pfc */
    UI32_T rch_active;       /* Rch active */

} CLX_PKT_RX_RCH_STATUS_T;

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */

/**
 * @brief To init or deinit the RX subsystem.
 *
 * This API can only be invoked once after the device is booted up.
 *
 * support_chip all
 *
 * @param [in]     unit          - Device unit number
 * @param [in]     ptr_rx_cfg    - The user configuration
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_setRxConfig(const UI32_T unit, const CLX_PKT_RX_CFG_T *ptr_rx_cfg);

/**
 * @brief To get the RX subsystem configuration.
 *
 * This API can only be invoked once after the device is booted up.
 *
 * support_chip all
 *
 * @param [in]     unit          - Device unit number
 * @param [out]    ptr_rx_cfg    - The user configuration
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_getRxConfig(const UI32_T unit, CLX_PKT_RX_CFG_T *ptr_rx_cfg);

/**
 * @brief To transmit the specified packet from CPU port.
 *
 * support_chip all
 *
 * @param [in]     unit       - Device unit number
 * @param [in]     channel    - The channel used for packet transmission
 * @param [in]     ptr_pkt    - The packet structure of the TX packet
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_sendPacket(const UI32_T unit, const UI32_T channel, const CLX_PKT_TX_PKT_T *ptr_pkt);

/**
 * @brief To prepare tx packet.
 *
 * support_chip all
 *
 * @param [in]     unit            - Device unit number
 * @param [in]     ptr_pkt         - The packet structure of the TX packet
 * @param [in]     ptr_data_buf    - The buffer payload
 * @param [in]     len             - The packet length
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_prepareTxPkt(const UI32_T unit,
                     CLX_PKT_TX_PKT_T *ptr_pkt,
                     const UI8_T *ptr_data_buf,
                     UI32_T len);

/**
 * @brief To set the mapping of the RX queue to the RX DMA channel.
 *
 * support_chip all
 *
 * @param [in]     unit       - Device unit number
 * @param [in]     queue      - The specified queue
 * @param [in]     channel    - The specified RX channel
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_setQueueToRxChannel(const UI32_T unit, const UI32_T queue, const UI32_T channel);

/**
 * @brief To get the RX DMA channel which is mapped to the specified queue.
 *
 * support_chip all
 *
 * @param [in]     unit           - Device unit number
 * @param [in]     queue          - The specified queue
 * @param [out]    ptr_channel    - The channel to which the specified queue is mapped
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_getQueueToRxChannel(const UI32_T unit, const UI32_T queue, UI32_T *ptr_channel);

/**
 * @brief To set the packet truncated size of the target queue.
 *
 * The packet will be truncated to the size of a multiple of 64B.
 *
 * support_chip all
 *
 * @param [in]     unit             - Device unit number
 * @param [in]     queue            - The specified queue ID
 * @param [in]     truncate_size    - Packet size of the queue
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_setRxQueueTruncateSize(const UI32_T unit, const UI32_T queue, const UI32_T truncate_size);

/**
 * @brief To get the packet truncated size of the target queue.
 *
 * support_chip all
 *
 * @param [in]     unit                 - Device unit number
 * @param [in]     queue                - The target queue ID
 * @param [out]    ptr_truncate_size    - Packet size of the queue
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_getRxQueueTruncateSize(const UI32_T unit, const UI32_T queue, UI32_T *ptr_truncate_size);

/**
 * @brief To set the expected COS value to the target TX DMA channel.
 *
 * support_chip CL8300 CL8500
 *
 * @param [in]     unit          - Device unit number
 * @param [in]     channel       - The specified TX channel
 * @param [in]     cos_bitmap    - The specified CoS values
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_setTxChannelCosBitmap(const UI32_T unit, const UI32_T channel, const UI8_T cos_bitmap);

/**
 * @brief To get the COS value mapped to the target TX DMA channel.
 *
 * support_chip CL8300 CL8500
 *
 * @param [in]     unit              - Device unit number
 * @param [in]     channel           - The specified TX channel
 * @param [out]    ptr_cos_bitmap    - Pointer for the CoS bitmap
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_getTxChannelCosBitmap(const UI32_T unit, const UI32_T channel, UI8_T *ptr_cos_bitmap);

/**
 * @brief To set a specific rule applied to the packet.
 *
 * support_chip all
 *
 * @param [in]     unit         - Device unit number
 * @param [in]     index        - The specified entry index for the rule
 * @param [in]     ptr_entry    - The value which will be set to the target entry
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_setCtrlToCpuEntry(const UI32_T unit,
                          const UI32_T index,
                          const CLX_PKT_CTRL_TO_CPU_ENTRY_T *ptr_entry);

/**
 * @brief To get a specified ctrl-to-CPU entry.
 *
 * support_chip all
 *
 * @param [in]     unit         - Device unit number
 * @param [in]     index        - The specified entry index
 * @param [out]    ptr_entry    - The value obtained from the entry
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_getCtrlToCpuEntry(const UI32_T unit,
                          const UI32_T index,
                          CLX_PKT_CTRL_TO_CPU_ENTRY_T *ptr_entry);

/**
 * @brief To delete all ctrl-to-CPU entries configured.
 *
 * support_chip all
 *
 * @param [in]     unit    - Device unit number
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_delCtrlToCpuEntryAll(const UI32_T unit);

/**
 * @brief To set the CPU reason code to RX queue mapping.
 *
 * support_chip all
 *
 * @param [in]     unit             - Device unit number
 * @param [in]     queue            - The specified queue
 * @param [in]     reason_bitmap    - The reason bitmap
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_setRxQueueMapping(const UI32_T unit,
                          const UI32_T queue,
                          const CLX_PKT_RX_REASON_BITMAP_T reason_bitmap);

/**
 * @brief To get the CPU reason code to RX queue mapping.
 *
 * support_chip all
 *
 * @param [in]     unit                 - Device unit number
 * @param [in]     queue                - The specified queue
 * @param [out]    ptr_reason_bitmap    - Pointer of the reason bitmap
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_getRxQueueMapping(const UI32_T unit,
                          const UI32_T queue,
                          CLX_PKT_RX_REASON_BITMAP_T *ptr_reason_bitmap);

/**
 * @brief To set the CPU reason code to RX port and queue mapping.
 *
 * support_chip all
 *
 * @param [in]     unit             - Device unit number
 * @param [in]     port             - The port ID indicating CPU or CPI
 * @param [in]     queue            - The specified queue
 * @param [in]     reason_bitmap    - The reason bitmap
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_setRxMapping(const UI32_T unit,
                     const UI32_T port,
                     const UI32_T queue,
                     const CLX_PKT_RX_REASON_BITMAP_T reason_bitmap);

/**
 * @brief To get the CPU reason code to RX port and queue mapping.
 *
 * support_chip all
 *
 * @param [in]     unit                 - Device unit number
 * @param [in]     port                 - The port ID indicating CPU or CPI
 * @param [in]     queue                - The specified queue
 * @param [out]    ptr_reason_bitmap    - Pointer of the reason bitmap
 * @return         CLX_E_OK        - Operation is successful.
 * @return         CLX_E_OTHERS    - Fail
 */
CLX_ERROR_NO_T
clx_pkt_getRxMapping(const UI32_T unit,
                     const UI32_T port,
                     const UI32_T queue,
                     CLX_PKT_RX_REASON_BITMAP_T *ptr_reason_bitmap);

/**
 * @brief To get the PDMA RX counters of the target channel.
 *
 * support_chip all
 *
 * @param [in]     unit          - Device unit number
 * @param [in]     channel       - The target channel
 * @param [out]    ptr_rx_cnt    - Pointer for the Rx counter
 * @return         CLX_E_OK                 - Operation is successful.
 * @return         CLX_E_ENTRY_NOT_FOUND    - The channel is invalid.
 */
CLX_ERROR_NO_T
clx_pkt_getRxCnt(const UI32_T unit, const UI32_T channel, CLX_PKT_RX_CNT_T *ptr_rx_cnt);

/**
 * @brief To get the PDMA TX counters of the target channel.
 *
 * support_chip all
 *
 * @param [in]     unit          - Device unit number
 * @param [in]     channel       - The target channel
 * @param [out]    ptr_tx_cnt    - Pointer for the Tx counter
 * @return         CLX_E_OK                 - Operation is successful.
 * @return         CLX_E_ENTRY_NOT_FOUND    - The channel is invalid.
 */
CLX_ERROR_NO_T
clx_pkt_getTxCnt(const UI32_T unit, const UI32_T channel, CLX_PKT_TX_CNT_T *ptr_tx_cnt);

/**
 * @brief To clear the PDMA RX counters of the target channel.
 *
 * support_chip all
 *
 * @param [in]     unit       - Device unit number
 * @param [in]     channel    - The target channel
 * @return         CLX_E_OK    - Successfully clear the counters.
 */
CLX_ERROR_NO_T
clx_pkt_clearRxCnt(const UI32_T unit, const UI32_T channel);

/**
 * @brief To clear the PDMA TX counters of the target channel.
 *
 * support_chip all
 *
 * @param [in]     unit       - Device unit number
 * @param [in]     channel    - The target channel
 * @return         CLX_E_OK    - Successfully clear the counters.
 */
CLX_ERROR_NO_T
clx_pkt_clearTxCnt(const UI32_T unit, const UI32_T channel);

/**
 * @brief To show the PDMA TX counters of the target channel.
 *
 * support_chip all
 *
 * @param [in]     unit       - Device unit number
 * @param [in]     channel    - The target channel
 * @return         CLX_E_OK    - Successfully clear the counters.
 */
CLX_ERROR_NO_T
clx_pkt_showTxDbgCnt(const UI32_T unit, const UI32_T channel);

/**
 * @brief To show the PDMA RX counters of the target channel.
 *
 * support_chip all
 *
 * @param [in]     unit       - Device unit number
 * @param [in]     channel    - The target channel
 * @return         CLX_E_OK    - Successfully clear the counters.
 */
CLX_ERROR_NO_T
clx_pkt_showRxDbgCnt(const UI32_T unit, const UI32_T channel);

#endif /* End of CLX_PKT_H */
