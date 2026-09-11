/*
 * Copyright 2024 Broadcom Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef L3_COMMDEFS_H
#define L3_COMMDEFS_H

#include "datatypes.h"
#include "packet.h"

/****************************************
*
*  IP ROUTER DEFINES
*
*****************************************/

/* IP MTU defines */
#define  L3_MIN_IP_MTU             68
/* Interfaces now allow the IPv4 MTU to be > 1500 bytes. The IPv4 MTU is limited
 * by the link MTU. */
#define  L3_MAX_IP_MTU             1500

#define  L3_MAX_POSSIBLE_IP_MTU    (platMaxFrameSizeGet() - ( ENET_HDR_SIZE +  ENET_ENCAPS_HDR_SIZE +  ETHERNET_CHECKSUM_LEN))

/* Invalid Interface number. */
#define  INVALID_INTF             0

#define  L3_31_BIT_NETMASK        0xFFFFFFFEUL
#define  IP4_MASK_LEN_TO_MASK(maskLen) ((maskLen == 0) ? 0 : (0xffffffff<<(32-maskLen)))

#define  DEFAULT_VRF_ID           0
#define  DEFAULT_VRF_NAME         ""
#define  RTO_DEF_INST             0

#define  ALL_VRF_ID               0xFFFF          /*  all VRFs  */

#define  RTO_PREFERENCE_MIN                       1
#define  RTO_PREFERENCE_MAX                       255
#define  RTO_RIP_PREFERENCE_DEF                   FD_RTR_RTO_PREF_RIP


#define  RTO_STATIC_PREFERENCE_DEF                FD_RTR_RTO_PREF_STATIC

#define  RTO_PREFERENCE_INTRA                     110
#define  RTO_PREFERENCE_INTER                     110
#define  RTO_PREFERENCE_EXTERNAL                  110

#define MCAST_UNUSED_PARAM(arg) ((void) arg)

#define  IPV6_STRLEN 4

/* Special Routing interface numbers */
#define RFC5549INTFNUM         5549
#define VRF_LEAK_ROUTE_INTFNUM 6549
#define MPLSINTFNUM            3031   /*  just choose MPLS RFC no. 3031 for the interface  */

/* Special Inteface name */
#define RFC5549INTF "rfc5549if"

#define RFC5549INTF_PREFIX_STRLEN (strlen (RFC5549INTF))

/*--------------------------------------*/
/*  IP Metrics                          */
/*--------------------------------------*/
typedef enum TosTypes
{
   TOS_NORMAL_SERVICE        = 0,
   TOS_MINIMIZE_COST         = 2,
   TOS_MAXIMIZE_RELIABILITY  = 4,
   TOS_MAXIMIZE_THROUGHPUT   = 8,
   TOS_MINIMIZE_DELAY        = 16
}   TOSTYPES;


typedef enum TosTypeIndex
{
   TOS_NORMAL_SERVICE_INDEX  = 0,
   TOS_MINIMIZE_COST_INDEX,
   TOS_MAXIMIZE_RELIABILITY_INDEX,
   TOS_MAXIMIZE_THROUGHPUT_INDEX,
   TOS_MINIMIZE_DELAY_INDEX
}   TOS_TYPES_INDICES;

#define      RTR_MAX_TOS_INDICES         5
#define      RTR_MAX_TOS                32

/*---------------------------------------*/
/*   Bandwidth                           */
/*---------------------------------------*/
#define      OSPF_MIN_REF_BW                1
#define      OSPF_MAX_REF_BW                4294967

#define      L3_MIN_BW                 1
#define      L3_MAX_BW                 10000000

/*--------------------------------------*/
/*   ICMP Rate Limit                    */
/*---------------------------------------*/
#define  L3_ICMP_RATE_LIMIT_MAX_BURST_SIZE 200
#define  L3_ICMP_RATE_LIMIT_MIN_BURST_SIZE 1
#define  L3_ICMP_RATE_LIMIT_MAX_INTERVAL 2147483647 /* Milli seconds */
#define  L3_ICMP_RATE_LIMIT_MIN_INTERVAL 0

/*--------------------------------------*/
/*  IP Routing Protocols                */
/*--------------------------------------*/

/*This is used for IP Mapping Registrants */

/* NOTE: Make sure to also update ipMapEventClientNames array with
 * appropriate protocol names in the same order.
 */
typedef enum
{
   RESERVED_PROTOCOL_ID = 0,
   IPRT_RIP,
   IPRT_VRRP,
   IPRT_OSPF,
   IPRT_MGMD,  /*This sould be before multicast routing protocols */
   IPRT_PIMDM,
   IPRT_DVMRP,
   IPRT_STATIC,
   IPRT_LOCAL,
   IPRT_BGP,
   IPRT_DEFAULT,
   IPRT_RTR_DISC,
   IPRT_MPLS,
   IPRT_MCAST,
   IPRT_PIMSM,
   IPRT_OSPFV3,
   IPRT_RLIM,
   IPRT_DHCP6S,
   IPRT_RADIUS,
   IPRT_SNOOPING,
   IPRT_SNTP,
   IPRT_DHCP_CLIENT,
   IPRT_LOG,
   IPRT_NETLINK,
   IPRT_RPPI,
   IPRT_PBR,
   IPRT_DCVPN,
   IPRT_IP_HELPER,
   IPRT_FPTI,
   IPRT_BONJOUR,
   IPRT_IPSLA,
   IPRT_DAS,
   IPRT_SNMP,
   IPRT_DHCP6_CLIENT,
   IPRT_NTP,
   LAST_ROUTER_PROTOCOL

}  IPMAP_REGISTRANTS_t;

RC_t ipMapEventClientNameGet (uint32 comp,  char8 *buf, uint32 size);

/* The following enum will be used for the ipMapIntfStateChangeCallback
   function used to alert protocols on configuration/ state changes
   on routing interfaces. If you change this enum, also change
   ipMapRouterEventNames[]. */
typedef enum
{
   RESERVED_STATE_CHANGE = 0,
   RTR_ENABLE,                            /* Routing  enabled */
   RTR_DISABLE_PENDING,                   /* Routing to be disabled */
   RTR_DISABLE,                           /* Routing  disable */
   RTR_INTF_CREATE,                       /* routing interface create */
   RTR_INTF_ENABLE_PENDING,               /* Routing interface enable pending */
   RTR_INTF_ENABLE,                       /* Routing interface enable complete */
   RTR_INTF_DISABLE_PENDING,              /* Routing interface disable pendng */
   RTR_INTF_DISABLE,                      /* Routing interface disable complete */
   RTR_INTF_DELETE,                       /* Delete routing interface */
   RTR_INTF_IPADDR_ADD,                   /* IP MAP no longer sends */
   RTR_INTF_IPADDR_DELETE_PENDING,        /* IP MAP no longer sends */
   RTR_INTF_IPADDR_DELETE,                /* IP MAP no longer sends */
   RTR_INTF_SECONDARY_IP_ADDR_ADD,        /* Routing interface secondary ip address add   */
   RTR_INTF_SECONDARY_IP_ADDR_DELETE,     /* Routing interface secondary ip address delete */
   RTR_INTF_MTU,                          /* IP MTU has changed on this interface */
   RTR_INTF_BW_CHANGE,                    /* Interface bandwidth changed */
   RTR_INTF_SPEED_CHANGE,                 /* Actual interface speed has changed */
   RTR_STARTUP_DONE,                      /* All router events have been sent at startup */
   RTR_INTF_HOST_MODE,                    /* Host Interface is Enabled */
   RTR_INTF_CFG_CHANGE,                   /* Router interface config changed */
   RTR_VRF_CREATE,                        /* New VRF created */
   RTR_VRF_DELETE,                        /* VRF deleted */
   RTR_INTF_VRF_CHANGE_PENDING,           /* Event to synchronize VRF change handling between IPMAP and IP6MAP */
   LAST_RTR_STATE_CHANGE

}  RTR_EVENT_CHANGE_t;

/*--------------------------------------*/
/*  Routing Protocol Types              */
/*  (lower number means higher priority)*/
/*--------------------------------------*/
/* If you add or remove an enum, you must also update protocolMapping[]
 * and rtoProtoNames[] in rto.c. */
typedef enum
{
  RTO_RESERVED = 0,      /* Not a valid protocol */
  RTO_LOCAL,
  RTO_KERNEL,
  RTO_STATIC,
  RTO_MPLS,
  RTO_OSPF_INTRA_AREA,
  RTO_OSPF_INTER_AREA,
  RTO_OSPF_TYPE1_EXT,
  RTO_OSPF_TYPE2_EXT,
  RTO_OSPF_NSSA_TYPE1_EXT,
  RTO_OSPF_NSSA_TYPE2_EXT,
  RTO_OSPFV3_INTRA_AREA,
  RTO_OSPFV3_INTER_AREA,
  RTO_OSPFV3_TYPE1_EXT,
  RTO_OSPFV3_TYPE2_EXT,
  RTO_OSPFV3_NSSA_TYPE1_EXT,
  RTO_OSPFV3_NSSA_TYPE2_EXT,
  RTO_RIP,
  RTO_DEFAULT,
  RTO_IBGP,
  RTO_EBGP,
  RTO_LBGP,        /* local BGP routes, e.g., reject routes for aggregates */
  RTO_EXTERNAL,    /* Learnt from external third party entity via netlink*/
  RTO_UNNUMBERED,  /* interface routes to unnumbered peers.
                    * this route type should not be used for routes in RTO */
  RTO_NET_PROTOTYPE,
  RTO_LAST_USED,
  RTO_RSVD3,       /* Reserved to minimize config file upgrades */
  RTO_LAST_PROTOCOL  /* Always keep this last */

}  RTO_PROTOCOL_INDICES_t;

/*----------------------------------------------------------*/
/*  Route Preference Types                                  */
/*  Route types for which preferences can be set            */
/*  We are making independent enums for the preferences     */
/*  and decoupling from the  RTO_PROTOCOL_INDICES_t enum  */
/*----------------------------------------------------------*/
typedef enum
{
  ROUTE_PREF_RESERVED = 0,      /* Not a valid protocol */
  ROUTE_PREF_LOCAL,
  ROUTE_PREF_STATIC,
  ROUTE_PREF_MPLS,
  ROUTE_PREF_OSPF_INTRA_AREA,
  ROUTE_PREF_OSPF_INTER_AREA,
  ROUTE_PREF_OSPF_EXTERNAL,
  ROUTE_PREF_RIP,
  ROUTE_PREF_IBGP,
  ROUTE_PREF_EBGP,
  ROUTE_PREF_6TO4,
  ROUTE_PREF_GLOBAL_DEFAULT_GATEWAY,
  ROUTE_PREF_DEFAULT_ROUTE_DHCP,
  ROUTE_PREF_LOCAL_BGP,
  ROUTE_PREF_NET_PROTOTYPE,
  ROUTE_PREF_LAST               /* Always keep this last */

}  ROUTE_PREF_INDICES_t;

#define      RTO_MAX_PROTOCOLS        RTO_LAST_PROTOCOL
#define      ROUTE_MAX_PREFERENCES    ROUTE_PREF_LAST

/* Redistribution route types*/
typedef enum
{
  REDIST_RT_FIRST = 0,
  REDIST_RT_LOCAL,
  REDIST_RT_STATIC,
  REDIST_RT_RIP,
  REDIST_RT_OSPF,
  REDIST_RT_BGP,
  REDIST_RT_OSPFV3,
  REDIST_RT_KERNEL,
  REDIST_RT_NET_PROTOTYPE,
  REDIST_RT_LAST
}  REDIST_RT_INDICES_t;

/* NSF applications that may send IPv4 routes to RTO. Values are powers
 * of two used to index nsfRoutesPending. RIP and BGP don't support NSF
 * now but adding values for them anyway. */
typedef enum
{
  NSF_PROTOCOL_OSPF = 0x1,
  NSF_PROTOCOL_RIP = 0x2,
  NSF_PROTOCOL_BGP = 0x4,
  NSF_PROTOCOL_LAST = 0x8

}  NSF_PROTOCOL_t;

/* These values should match those in aclActionIndex_t, except
 * we do not need the CPU values here.
 */
typedef enum
{
    FILTER_ACTION_PERMIT = 0,
    FILTER_ACTION_DENY = 1
} FILTER_ACTION_t;

/*------------------------------*/
/* CIDR Routing Protocol Types  */
/*------------------------------*/
/* The following Enum is used by the ipMapIpCidrRouteProtoGet
   Routing protocols have been defined as per rfc 2096 */

typedef enum
{
   IP_CIDR_ROUTE_PROTO_OTHER = 1,                           /* 1: Not specified                 */
   IP_CIDR_ROUTE_PROTO_LOCAL,                               /* 2: Local interface               */
   IP_CIDR_ROUTE_PROTO_NETMGMT,                             /* 3: Static Route                  */
   IP_CIDR_ROUTE_PROTO_ICMP,                                /* 4: Result of ICMP Redirect       */

  /* The following are dynamic routing protocols */
   IP_CIDR_ROUTE_PROTO_EGP,                                 /* 5:   Exterior Gateway Protocol   */
   IP_CIDR_ROUTE_PROTO_GGP,                                 /* 6:   Gateway-Gateway Protocol    */
   IP_CIDR_ROUTE_PROTO_HELLO,                               /* 7:   FuzzBall HelloSpeak         */
   IP_CIDR_ROUTE_PROTO_RIP,                                 /* 8:   Berkeley RIP or RIP-II      */
   IP_CIDR_ROUTE_PROTO_ISIS,                                /* 9:   Dual IS-IS                  */
   IP_CIDR_ROUTE_PROTO_ESIS,                                /* 10:  ISO 9542                    */
   IP_CIDR_ROUTE_PROTO_CISCOIGRP,                           /* 11:  Cisco IGRP                  */
   IP_CIDR_ROUTE_PROTO_BBNSPFIGP,                           /* 12:  BBN SPF IGP                 */
   IP_CIDR_ROUTE_PROTO_OSPF,                                /* 13:  Open Shortest Path First    */
   IP_CIDR_ROUTE_PROTO_BGP,                                 /* 14:  Border Gateway Protocol     */
   IP_CIDR_ROUTE_PROTO_IDPR,                                /* 15:  Interdomain Policy Routing  */
   IP_CIDR_ROUTE_PROTO_CISCOEIGRP,                          /* 16:  Cisco EIGRP                 */
   IP_CIDR_ROUTE_PROTO_LAST

}  IP_CIDR_ROUTE_PROTO_t;

/*---------------------------------*/
/* IP Address Configuration Method */
/*---------------------------------*/
/* The following enum indicates the method used to
 * configure an IP Address on a Router Interface.
 */
typedef enum
{
   RTR_INTF_IP_ADDR_METHOD_NONE = 0,
   RTR_INTF_IP_ADDR_METHOD_CONFIG,     /* Manual Configuration */
   RTR_INTF_IP_ADDR_METHOD_DHCP        /* Leased through DHCP */

}  RTR_INTF_IP_ADDR_METHOD_t;

/****************************************
*
*  ARP DEFINES
*
*****************************************/

/*--------------------------------------*/
/*  ARP Constants                       */
/*--------------------------------------*/

/* config parameter min/max/default values */
#define  IP_ARP_AGE_TIME_MIN          15            /* seconds */
#define  IP_ARP_AGE_TIME_MAX          21600         /* seconds (=6 hr) */
#define  IP_ARP_AGE_TIME_DEF          1200          /* seconds (=20 min)*/

#define  IP_ARP_RESP_TIME_MIN         1             /* seconds */
#define  IP_ARP_RESP_TIME_MAX         10            /* seconds */
#define  IP_ARP_RESP_TIME_DEF         1             /* seconds */

#define  IP_ARP_RETRIES_MIN           0
#define  IP_ARP_RETRIES_MAX           10
#define  IP_ARP_RETRIES_DEF           4

#define  IP_ARP_AGE_TIME_CHECK        360          /* 6 minutes */

#define  IP_ARP_DYNAMIC_RENEW_DEF      DISABLE


/**************************************
*
*     OSPF DEFINES
*
***************************************/

#define  OSPF_NOT_CONFIGURED  0

typedef enum
{
   OSPF_METRIC_EXT_TYPE_INVALID = 0,
   OSPF_METRIC_EXT_TYPE1 = 1,
   OSPF_METRIC_EXT_TYPE2

}  OSPF_EXT_METRIC_TYPES_t;

typedef enum
{
   OSPF_METRIC_TYPE_INTERNAL = 1,
   OSPF_METRIC_TYPE_EXT1 = 2,
   OSPF_METRIC_TYPE_EXT2 = 4,
   OSPF_METRIC_TYPE_NSSA_EXT1 = 8,
   OSPF_METRIC_TYPE_NSSA_EXT2 = 16

}  OSPF_METRIC_TYPES_t;

typedef enum
{
   OSPF_AREA_IMPORT_EXT = 1,            /* Import and propagate T5 LSAs */
   OSPF_AREA_IMPORT_NO_EXT,             /* Do not import and propagate ext LSAs- make stub area*/
   OSPF_AREA_IMPORT_NSSA                /* Import and propagate T7 LSAs */

}  OSPF_AREA_EXT_ROUTING_t;

typedef enum
{
   OSPF_AREA_NO_SUMMARY = 1,            /* Do not originate and propagate summary LSAs */
   OSPF_AREA_SEND_SUMMARY               /* Originate and propagate summary LSAs */

}  OSPF_AREA_SUMMARY_t;

typedef enum
{
   OSPF_ROW_ACTIVE = 1,       /* - in call:   activate the object  */
                                /*   in return: the object is active */
   OSPF_ROW_NOT_IN_SERVICE,   /* - in call:   deactivate the object */
                                /* - in return: the object is not active */
   OSPF_ROW_NOT_READY,        /* - in return only: the object not ready */
   OSPF_ROW_CREATE_AND_GO,    /* - in call only: init and run the object */
   OSPF_ROW_CREATE_AND_WAIT,  /* - in call only: init the object */
   OSPF_ROW_DESTROY,          /* - in call only: delete the object */
   OSPF_ROW_CHANGE

}  OSPF_ROW_STATUS_t;

typedef enum
{
   OSPF_STUB_METRIC_TYPE_INVALID = 0,
   OSPF_AREA_STUB_METRIC = 1,
   OSPF_AREA_STUB_COMPARABLE_COST,
   OSPF_AREA_STUB_NON_COMPARABLE_COST

}  OSPF_STUB_METRIC_TYPE_t;

typedef enum
{
   OSPF_AREA_RANGE_EFFECT_ADVERTISE_MATCHING = 1,
   OSPF_AREA_RANGE_EFFECT_DONOT_ADVERTISE_MATCHING

} OSPF_AREA_RANGE_EFFECT_t;

typedef enum
{
   OSPF_AREA_AGGREGATE_EFFECT_ADVERTISEMATCHING = 1,
   OSPF_AREA_AGGREGATE_EFFECT_DONOT_ADVERTISEMATCHING

} OSPF_AREA_AGGREGATE_STATUS_t;

typedef enum
{
   OSPF_AREA_AGGREGATE_LSDBTYPE_SUMMARYLINK = 3,
   OSPF_AREA_AGGREGATE_LSDBTYPE_NSSAEXTERNALLINK = 7

} OSPF_AREA_AGGREGATE_LSDBTYPE_t;

typedef enum
{
   OSPF_MULTICAST_FORWARDING_BLOCKED = 0,
   OSPF_MULTICAST_FORWARDING_MULTICAST,
   OSPF_MULTICAST_FORWARDING_UNICAST

} OSPF_MULTICAST_FORWARDING_t;

typedef enum
{
   OSPF_NBMA_NBR_PERMANENCE_DOWN = 1,
   OSPF_NBMA_NBR_PERMANENCE_PERMANENT

} OSPF_NBMA_NBR_PERMANENCE__t;

typedef enum
{
   OSPF_VIRT_NBR_STATE_DOWN = 1,
   OSPF_VIRT_NBR_STATE_ATTEMPT,
   OSPF_VIRT_NBR_STATE_INIT,
   OSPF_VIRT_NBR_STATE_TWOWAY,
   OSPF_VIRT_NBR_STATE_EXCHANGE_START,
   OSPF_VIRT_NBR_STATE_EXCHANGE,
   OSPF_VIRT_NBR_STATE_LOADING,
   OSPF_VIRT_NBR_STATE_FULL

} OSPF_VIRT_NBR_STATE_t;

typedef enum
{
   OSPF_VIRT_IFSTATE_DOWN = 1,
   OSPF_VIRT_IFSTATE_POINTTOPOINT = 4

} OSPF_VIRT_IFSTATE__t;


/* NSSATranslatorRole
** Specifies whether or not an NSSA border router will
** unconditionally translate Type-7 LSAs into Type-5 LSAs.  When
** it is set to Always, an NSSA border router always translates
** Type-7 LSAs into Type-5 LSAs regardless of the translator state
** of other NSSA border routers.  When it is set to Candidate, an
** NSSA border router participates in the translator election
** process described in Section 3.1.  The default setting is
** Candidate.
*/
typedef enum
{
    OSPF_NSSA_TRANSLATOR_ALWAYS    = 1,
    OSPF_NSSA_TRANSLATOR_CANDIDATE = 2

}   OSPF_NSSA_TRANSLATOR_ROLE_t;

typedef enum
{
    OSPF_NSSA_TRANS_STATE_ENABLED  = 1,
    OSPF_NSSA_TRANS_STATE_ELECTED  = 2,
    OSPF_NSSA_TRANS_STATE_DISABLED = 3

}   OSPF_NSSA_TRANSLATOR_STATE_t;

/* TranslatorStabilityInterval
** Defines the length of time an elected Type-7 translator will
** continue to perform its translator duties once it has
** determined that its translator status has been deposed by
** another NSSA border router translator as described in Section
** 3.1 and 3.3.  The default setting is 40 seconds.
*/
#define  OSPF_NSSA_TRANS_STABILITY_INT_MAX   3600
#define  OSPF_NSSA_TRANS_STABILITY_INT_MIN   0
#define  OSPF_NSSA_TRANS_STABILITY_INT_DEF   40

typedef enum
{
   OSPF_OPT_Q_BIT = 0x01,
   OSPF_OPT_E_BIT = 0x02,
   OSPF_OPT_MC_BIT = 0x04,
   OSPF_OPT_NP_BIT = 0x08,
   OSPF_OPT_O_BIT = 0x40,
   VPN_OPTION_BIT = 0x80

}  OSPF_OPTIONS_t;

/* Bits defined for the OSPF router LSA flags field */
typedef enum
{
     OSPFRTRLSAFLAGS_B = 0x1,
     OSPFRTRLSAFLAGS_E = 0x2,
     OSPFRTRLSAFLAGS_V = 0x4,
     OSPFRTRLSAFLAGS_W = 0x8,
     OSPFRTRLSAFLAGS_Nt = 0x10
}  OSPFRTRLSAFLAGS_t;

/* OSPF stub router configuration options */
typedef enum
{
  /* OSPF is not configured as a stub router. */
  OSPF_STUB_RTR_CFG_DISABLED = 0,

  /* OSPF is configured as a stub router. */
  OSPF_STUB_RTR_CFG_ALWAYS = 1,

  /* OSPF is a stub router for a limited time after reboot. */
  OSPF_STUB_RTR_CFG_STARTUP = 2

} OSPF_STUB_ROUTER_CFG_t;

/* Reason why OSPF is in stub router advertisement mode */
typedef enum
{
  /* OSPF is not in stub router mode */
  OSPF_STUB_RTR_NO_REASON = 0,

  /* OSPF is configured as a stub router */
  OSPF_STUB_RTR_CONFIGURED,

  /* OSPF configured as a stub router at startup and OSPF is
   * in the startup period. */
  OSPF_STUB_RTR_STARTUP,

  /* OSPF automaically entered stub router mode because of a resource limitation. */
  OSPF_STUB_ROUTER_RESOURCE

} OSPF_STUB_ROUTER_REASON_t;

typedef enum
{
  /* Does not act as a helpful neighbor */
  OSPF_HELPER_NONE,

  /* Acts as a helpful neighbor for planned restarts only */
  OSPF_HELPER_PLANNED_ONLY,

  /* Acts as a helpful neighbor for planned and unplanned restarts */
  OSPF_HELPER_ALWAYS

} OSPF_HELPER_SUPPORT_t;

typedef enum
{
  /* Never do graceful restart */
  OSPF_NSF_NONE,

  /* Do graceful restart only for planned restarts */
  OSPF_NSF_PLANNED_ONLY,

  /* Do graceful restart for both planned and unplanned restarts */
  OSPF_NSF_ALWAYS

} OSPF_NSF_SUPPORT_t;

/* reasons why OSPF could exit helpful neighbor mode for a given neighbor. Compare to
 * ospfNbrRestartHelperExitReason in RFC 4750. */
typedef enum
{
  OSPF_HELPER_EXIT_NONE = 1,    /* never acted as helpful neighbor for this neighbor */
  OSPF_HELPER_IN_PROGRESS = 2,  /* restart is in progress */
  OSPF_HELPER_COMPLETED = 3,    /* restart successfully completed */
  OSPF_HELPER_TIMED_OUT = 4,    /* grace period timed out */
  OSPF_HELPER_TOPO_CHANGE = 5,  /* topology change interrupted the graceful restart */
  OSPF_HELPER_MANUAL_CLEAR = 6, /* clear ip ospf neighbor */
  OSPF_HELPER_LINK_DOWN = 7     /* link to neighbor went down */

} OSPF_HELPER_EXIT_REASON_t;

/* reasons why OSPF could exit graceful restart. Compare to ospfRestartExitReason
 * in RFC 4750. */
typedef enum
{
  OSPF_GR_EXIT_NONE = 1,    /* never attempted graceful restart */
  OSPF_GR_IN_PROGRESS = 2,  /* restart is in progress */
  OSPF_GR_COMPLETED = 3,    /* restart successfully completed */
  OSPF_GR_TIMED_OUT = 4,    /* grace period timed out */
  OSPF_GR_TOPO_CHANGE = 5,  /* topology change interrupted the graceful restart */
  OSPF_GR_MANUAL_CLEAR = 6  /* clear ip ospf */

} OSPF_GR_EXIT_REASON_t;

/* Current status of OSPF graceful restart. Compare to RFC 4750 ospfRestartStatus. */
typedef enum
{
  OSPF_GR_NOT_RESTARTING = 1,
  OSPF_GR_PLANNED_RESTART = 2,
  OSPF_GR_UNPLANNED_RESTART = 3

} OSPF_GR_RESTART_STATUS_t;


#define  OSPF_INTF_DOWN                             1
#define  OSPF_INTF_LOOPBACK                         2
#define  OSPF_INTF_WAITING                          3
#define  OSPF_INTF_POINTTOPOINT                     4
#define  OSPF_INTF_DESIGNATEDROUTER                 5
#define  OSPF_INTF_BACKUPDESIGNATEDROUTER           6
#define  OSPF_INTF_OTHERDESIGNATEDROUTER            7

#define  OSPF_INTF_DEFAULT                          0
#define  OSPF_INTF_BROADCAST                        1
#define  OSPF_INTF_NBMA                             2
#define  OSPF_INTF_PTP                              3
#define  OSPF_INTF_VIRTUAL                          4
#define  OSPF_INTF_POINTTOMULTIPOINT                5
#define  OSPF_INTF_TYPE_LOOPBACK                    6

#define  OSPF_INTF_PASSIVE                           TRUE
#define  OSPF_INTF_NOT_PASSIVE                       FALSE

#define  OSPF_INTF_PREFIX_SUPPRESSED                       2 
#define  OSPF_INTF_PREFIX_NOT_SUPPRESSED                   1 
#define  OSPF_INTF_PREFIX_SUPPRESSION_UNSPECIFIED          0 


#define  OSPF_INTF_SECONDARIES_NONE                 0
#define  OSPF_INTF_SECONDARIES_ALL                  0xffffffff

/* if  OSPF_INTF_METRIC_NOT_CONFIGURED is zero, the metric will be
 *  computed from port speed otherwise only the value configured by user will be used
 */
#define  OSPF_INTF_METRIC_NOT_CONFIGURED            0

typedef enum
{
   S_ILLEGAL_LSA          = 0,
   S_ROUTER_LSA           = 1,
   S_NETWORK_LSA          = 2,
   S_IPNET_SUMMARY_LSA    = 3,
   S_ASBR_SUMMARY_LSA     = 4,
   S_AS_EXTERNAL_LSA      = 5,
   S_GROUP_MEMBER_LSA     = 6,
   S_NSSA_LSA             = 7,
   S_TMP2_LSA             = 8,
   S_LINK_OPAQUE_LSA      = 9,
   S_AREA_OPAQUE_LSA      = 10,
   S_AS_OPAQUE_LSA        = 11,
   S_LAST_LSA_TYPE        = 12
}  OSPF_LSDB_TYPE_t;

#define  OSPF_DYNAMIC            1
#define  OSPF_PERMANENT          2

#define  OSPF_DOWN               1
#define  OSPF_ATTEMPT            2
#define  OSPF_INIT               3
#define  OSPF_TWOWAY             4
#define  OSPF_EXCHANGESTART      5
#define  OSPF_EXCHANGE           6
#define  OSPF_LOADING            7
#define  OSPF_FULL               8

typedef enum
{
   OSPF_AREA_ID_INTEGER_FORMAT,
   OSPF_AREA_ID_DOTTED_DECIMAL_FORMAT
} OSPF_AREA_ID_FORMAT_t;

/*------------------*/
/* OSPF RANGE SIZES */
/*------------------*/

#define  OSPF_INTF_PRIORITY_MAX     255
#define  OSPF_INTF_PRIORITY_MIN     0
#define  OSPF_INTF_PRIORITY_DEF     1

#define  VRRP_INTF_PRIORITY_MAX     255
#define  VRRP_INTF_PRIORITY_MIN     0
#define  VRRP_INTF_PRIORITY_DEF     1

#define  OSPF_INTF_SECONDS_MAX      65535
#define  OSPF_INTF_SECONDS_MIN      1
#define  OSPF_INTF_SECONDS_DEF      10

#define  OSPF_INTF_DEAD_MAX         65535
#define  OSPF_INTF_DEAD_MIN         1
#define  OSPF_INTF_DEAD_DEF         40

#define  OSPF_INTF_RETRANSMIT_MAX   3600
#define  OSPF_INTF_RETRANSMIT_MIN   0
#define  OSPF_INTF_RETRANSMIT_DEF   5

#define  OSPF_INTF_NBMAPOLL_MAX     0x7FFFFFFF   /* 2147483647    */
#define  OSPF_INTF_NBMAPOLL_MIN     0

#define  OSPF_INTF_TRANSMITDELAY_MAX 3600
#define  OSPF_INTF_TRANSMITDELAY_MIN 1
#define  OSPF_INTF_TRANSMITDELAY_DEF 5

/* Cisco allows the user to set this to 16777215. Presumably, this allows
 * an operator to make destinations outside the OSPF domain or even the
 * stub area unreachable. */
#define  OSPF_AREA_STUBMETRIC_MAX   0xFFFFFF /*16777215*/
#define  OSPF_AREA_STUBMETRIC_MIN   1

#define  OSPF_AREA_NSSA_DEFAULT_METRIC_MAX   0xFFFFFE /*16777214*/
#define  OSPF_AREA_NSSA_DEFAULT_METRIC_MIN   1

#define  OSPF_INTF_COST_MAX   65535
#define  OSPF_INTF_COST_MIN   1
#define  OSPF_INTF_COST_DEF   10

#define  OSPF_VIRTIF_RETRANSMIT_MAX  3600
#define  OSPF_VIRTIF_RETRANSMIT_MIN  0
#define  OSPF_VIRTIF_RETRANSMIT_DEF  5

#define  OSPF_VIRTIF_DELAY_MAX   3600
#define  OSPF_VIRTIF_DELAY_MIN   0
#define  OSPF_VIRTIF_DELAY_DEF   1

#define  OSPF_EXT_LSDB_LIMIT_MAX   0x7FFFFFFF   /* 2147483647 */
#define  OSPF_EXT_LSDB_LIMIT_MIN   -1
#define  OSPF_EXT_LSDB_LIMIT_DEF   -1

#define  OSPF_EXIT_OVERFLOW_INT_MAX   0x7FFFFFFF   /* 2147483647  */
#define  OSPF_EXIT_OVERFLOW_INT_MIN   0
#define  OSPF_EXIT_OVERFLOW_INT_DEF   0

#define  OSPF_SPF_DELAY_TIME_MAX      65535
#define  OSPF_SPF_DELAY_TIME_MIN      0
#define  OSPF_SPF_DELAY_TIME_DEF      5

#define  OSPF_SPF_HOLD_TIME_MAX      65535
#define  OSPF_SPF_HOLD_TIME_MIN      0
#define  OSPF_SPF_HOLD_TIME_DEF      10

#define  OSPFV3_SPF_DELAY_TIME_MAX     600000
#define  OSPFV3_SPF_DELAY_TIME_MIN     1
#define  OSPFV3_SPF_DELAY_TIME_DEF     2000

#define  OSPFV3_SPF_HOLD_TIME_MAX      600000
#define  OSPFV3_SPF_HOLD_TIME_MIN      1
#define  OSPFV3_SPF_HOLD_TIME_DEF      5000

#define  OSPFV3_SPF_MAX_HOLD_TIME_MAX  600000
#define  OSPFV3_SPF_MAX_HOLD_TIME_MIN  1
#define  OSPFV3_SPF_MAX_HOLD_TIME_DEF  5000


#define  OSPF_AUTH_KEY_ID_MIN         0
#define  OSPF_AUTH_KEY_ID_MAX         255

#define  VRID_RANGE_MAX     255
#define  VRID_RANGE_MIN     1
#define  OSPF_REDIST_METRIC_MIN       0
#define  OSPF_REDIST_METRIC_MAX       16777214

#define  OSPF_DEFAULT_METRIC_MIN      1
#define  OSPF_DEFAULT_METRIC_MAX      16777214

/* How long to stay in stub router mode at startup. 5 seconds to 1 day */
#define  OSPF_STUB_ROUTER_DURATION_MIN   5
#define  OSPF_STUB_ROUTER_DURATION_MAX   86400

/* Maximum link metric for router LSAs */
#define  OSPF_STUB_ROUTER_MAXLINKMETRIC  0xFFFF

/* Override metric for external LSAs (type 5) when in stub router mode. */
#define  OSPF_STUB_ROUTER_EXTLSA_MET_MIN  1
#define  OSPF_STUB_ROUTER_EXTLSA_MET_MAX  16777215

/* Override metric for inter-area router LSAs (type 4) when in stub router mode. */
#define  OSPF_STUB_ROUTER_IARLSA_MET_MIN  1
#define  OSPF_STUB_ROUTER_IARLSA_MET_MAX  16777215

/* Override metric for summary LSAs when in stub router mode. */
#define  OSPF_STUB_ROUTER_SUMLSA_MET_MIN  1
#define  OSPF_STUB_ROUTER_SUMLSA_MET_MAX  16777215

#define  OSPF_RESTART_INTERVAL_MIN    1
#define  OSPF_RESTART_INTERVAL_MAX    1800

/* Range for avg time between LS Update tx, in milliseconds */
#define  OSPF_TIMERS_PACING_FLOOD_MIN 5
#define  OSPF_TIMERS_PACING_FLOOD_MAX 100

/* Range for LSA group window, in seconds */
#define  OSPF_TIMERS_PACING_LSA_GROUP_MIN 10
#define  OSPF_TIMERS_PACING_LSA_GROUP_MAX 1800

/* Range for cost defined with static area range */
#define  OSPF_AREA_RANGE_COST_MIN  0
#define  OSPF_AREA_RANGE_COST_MAX  16777215

#define  OSPF_LSA_PACKET_LENGTH_MAX   65535

/* NOTE: Must match OSPF_MAP_EXTEN_AUTH_MAX_KEY_SIMPLE definition */
#define  AUTH_MAX_KEY_OSPF_SIMPLE   8
/* NOTE: Must match OSPF_MAP_EXTEN_AUTH_MAX_KEY_MD5 definition */
#define  AUTH_MAX_KEY_OSPF_MD5      16
/* define this as the larger of the two */
#define  AUTH_MAX_KEY_OSPF           AUTH_MAX_KEY_OSPF_MD5

#define  OSPF_AREA_BACKBONE         0


/*------------------*/
/* OSPF TRAP TYPES  */
/*------------------*/

/* The following flags are to be used by user-interface routines to
   enable/disable or get the mode of the specified ospf trap */
#define  OSPF_TRAP_NONE                             0x0
#define  OSPF_TRAP_ALL                  ( OSPF_TRAP_ERRORS_ALL | \
                                           OSPF_TRAP_LSA_ALL  | \
                                           OSPF_TRAP_RETRANSMIT_ALL | \
                                           OSPF_TRAP_STATE_CHANGE_ALL | \
                                           OSPF_TRAP_OVERFLOW_ALL)

#define  OSPF_TRAP_ERRORS_ALL          ( OSPF_TRAP_VIRT_IF_CONFIG_ERROR | \
                                           OSPF_TRAP_IF_CONFIG_ERROR | \
                                           OSPF_TRAP_IF_AUTH_FAILURE | \
                                           OSPF_TRAP_VIRT_IF_AUTH_FAILURE | \
                                           OSPF_TRAP_RX_BAD_PACKET | \
                                           OSPF_TRAP_VIRT_IF_RX_BAD_PACKET)
#define  OSPF_TRAP_LSA_ALL             ( OSPF_TRAP_MAX_AGE_LSA | \
                                           OSPF_TRAP_ORIGINATE_LSA)
#define  OSPF_TRAP_RETRANSMIT_ALL      ( OSPF_TRAP_TX_RETRANSMIT | \
                                           OSPF_TRAP_VIRT_IF_TX_RETRANSMIT)
#define  OSPF_TRAP_STATE_CHANGE_ALL    ( OSPF_TRAP_VIRT_IF_STATE_CHANGE | \
                                           OSPF_TRAP_NBR_STATE_CHANGE | \
                                           OSPF_TRAP_VIRT_NBR_STATE_CHANGE | \
                                           OSPF_TRAP_IF_STATE_CHANGE)
#define  OSPF_TRAP_OVERFLOW_ALL        ( OSPF_TRAP_LS_DB_OVERFLOW | \
                                           OSPF_TRAP_LS_DB_APPROACHING_OVERFLOW)
#define  OSPF_TRAP_RTB_ALL               OSPF_TRAP_RTB_ENTRY_INFO
#define  OSPF_TRAP_IF_RX_ALL             OSPF_TRAP_IF_RX_PACKET

#define  OSPF_TRAP_VIRT_IF_STATE_CHANGE             0x1
#define  OSPF_TRAP_NBR_STATE_CHANGE                 0x2
#define  OSPF_TRAP_VIRT_NBR_STATE_CHANGE            0x4
#define  OSPF_TRAP_IF_CONFIG_ERROR                  0x8
#define  OSPF_TRAP_VIRT_IF_CONFIG_ERROR             0x10
#define  OSPF_TRAP_IF_AUTH_FAILURE                  0x20
#define  OSPF_TRAP_VIRT_IF_AUTH_FAILURE             0x40
#define  OSPF_TRAP_RX_BAD_PACKET                    0x80
#define  OSPF_TRAP_VIRT_IF_RX_BAD_PACKET            0x100
#define  OSPF_TRAP_TX_RETRANSMIT                    0x200
#define  OSPF_TRAP_VIRT_IF_TX_RETRANSMIT            0x400
#define  OSPF_TRAP_ORIGINATE_LSA                    0x800
#define  OSPF_TRAP_MAX_AGE_LSA                      0x1000
#define  OSPF_TRAP_LS_DB_OVERFLOW                   0x2000
#define  OSPF_TRAP_LS_DB_APPROACHING_OVERFLOW       0x4000
#define  OSPF_TRAP_IF_STATE_CHANGE                  0x8000
#define  OSPF_TRAP_IF_RX_PACKET                     0x10000
#define  OSPF_TRAP_RTB_ENTRY_INFO                   0x20000

/*enumerated values for ospfConfigErrorType */
typedef enum
{
   CONFIG_OK = 0,
   BAD_VERSION,
   AREA_MISMATCH,
   UNKNOWN_NBMA_NBR,
   UNKNOWN_VIRTUAL_NBR,
   AUTH_TYPE_MISMATCH,
   AUTH_FAILURE,
   NET_MASK_MISMATCH,
   HELLO_INTERVAL_MISMATCH,
   DEAD_INTERVAL_MISMATCH,
   OPTION_MISMATCH,
   BAD_PACKET,
   UKNOWN_ERROR

} OSPF_TRAP_CONFIG_ERROR_TYPE_t;

/* OSPF Packet Types */
typedef enum
{
     S_ILLEGAL = 0,
     S_HELLO,
     S_DB_DESCR,
     S_LS_REQUEST,
     S_LS_UPDATE,
     S_LS_ACK
} OSPF_PCKTYPE_t;

/**************************************
*
*     OSPFV3 DEFINES
*
*  ospfv3 uses the ospf defines.
*  This section contains definitions that
*  Must be different for ospfv3.
*
***************************************/

#define  OSPFV3_NOT_CONFIGURED  0

/* draft OSPFv3 MIB says dead interval max is 65535. That's different from
 * the dead max for OSPFv2. */
#define  OSPFV3_INTF_DEAD_MAX         0xFFFF   /* 65535 */
#define  OSPFV3_INTF_DEAD_MIN         1
#define  OSPFV3_INTF_DEAD_DEF         40

#define  OSPFV3_VER                   3

typedef enum
{
   V3_ILLEGAL_LSA          = 0,
   V3_ROUTER_LSA           = 1,
   V3_NETWORK_LSA          = 2,
   V3_INTER_PREFIX_LSA     = 3,
   V3_INTER_ROUTER_LSA     = 4,
   V3_AS_EXTERNAL_LSA      = 5,
   V3_GROUP_MEMBER_LSA     = 6,
   V3_NSSA_LSA             = 7,
   V3_LINK_LSA             = 8,
   V3_INTRA_PREFIX_LSA     = 9,
   V3_TMP3_LSA             = 10,
   V3_GRACE_LSA            = 11,
   V3_LINK_UNKNOWN_LSA     = 12,
   V3_AREA_UNKNOWN_LSA     = 13,
   V3_AS_UNKNOWN_LSA       = 14,
   V3_LAST_LSA_TYPE        = 15
}  OSPFV3_LSDB_TYPE_t;

typedef enum
{
   OSPFV3_OPT_V6_BIT = 0x01,
   OSPFV3_OPT_E_BIT =  0x02,
   OSPFV3_OPT_MC_BIT = 0x04,
   OSPFV3_OPT_NP_BIT = 0x08,
   OSPFV3_OPT_R_BIT =  0x10,
   OSPFV3_OPT_DC_BIT = 0x20

}  OSPFV3_OPTIONS_t;
/* The Following are the Exceptions in Ospfv3 Traps because of the current release doesn't support IPSec */
#define  OSPFV3_TRAP_ALL              ( OSPFV3_TRAP_ERRORS_ALL | \
                                          OSPF_TRAP_LSA_ALL | \
                                          OSPF_TRAP_RETRANSMIT_ALL | \
                                          OSPF_TRAP_STATE_CHANGE_ALL | \
                                          OSPF_TRAP_OVERFLOW_ALL)

#define  OSPFV3_TRAP_ERRORS_ALL       ( OSPF_TRAP_VIRT_IF_CONFIG_ERROR | \
                                          OSPF_TRAP_IF_CONFIG_ERROR | \
                                          OSPF_TRAP_RX_BAD_PACKET | \
                                          OSPF_TRAP_VIRT_IF_RX_BAD_PACKET)

/**************************************
*
*     ROUTERDISCOVER DEFINES
*
***************************************/

#define  RTR_DISC_MAXINTERVAL_MIN                    4
#define  RTR_DISC_MAXINTERVAL_MAX                    1800
#define  RTR_DISC_MAXINTERVAL_DEF                    600

#define  RTR_DISC_MININTERVAL_MIN                    3
#define  RTR_DISC_MININTERVAL_MAX                    1800
#define  RTR_DISC_MININTERVAL_DEF(time)              (0.75*(time))

#define  RTR_DISC_LIFETIME_MIN                        RTR_DISC_MAXINTERVAL_MIN
#define  RTR_DISC_LIFETIME_MAX                       9000
#define  RTR_DISC_LIFETIME_DEF(time)                 (3*(time))

#define  RTR_DISC_PREFERENCE_MIN                     0x80000000 /* -2147483648 */
#define  RTR_DISC_PREFERENCE_MAX                     0x7FFFFFFF /* 2147483647  */
#define  RTR_DISC_PREFERENCE_DEF                     0

/******************************************
*
*     ROUTER VRRP DEFINES
*
*******************************************/

#define  ROUTING_VRRP_MIN_VRID                       PLATFORM_ROUTING_VRRP_MIN_VRID

#define  ROUTING_VRRP_MAX_VRID                       PLATFORM_ROUTING_VRRP_MAX_VRID


/******************************************
*
*     IPSLA DEFINES
*
*******************************************/

#define  ROUTING_IPSLA_MIN_IPSLA_ID                  PLATFORM_ROUTING_IPSLA_MIN_IPSLA_ID
#define  ROUTING_IPSLA_MAX_IPSLA_ID                  PLATFORM_ROUTING_IPSLA_MAX_IPSLA_ID
#define  ROUTING_IPSLA_MIN_TRACK_ID                  PLATFORM_ROUTING_IPSLA_MIN_TRACK_ID
#define  ROUTING_IPSLA_MAX_TRACK_ID                  PLATFORM_ROUTING_IPSLA_MAX_TRACK_ID

/**************************************
*
*     RIP DEFINES
*
***************************************/

/*--------------------------------------*/
/*  Authentication Constants            */
/*--------------------------------------*/

#define  AUTH_MAX_KEY_RIP        16

#define  RIP_CONF_AUTH_KEY_ID_MIN     0
#define  RIP_CONF_AUTH_KEY_ID_MAX     255

/*--------------------------------------*/
/*  RIP Constants                       */
/*--------------------------------------*/

typedef enum
{
   RIP_CONF_SEND_DO_NOT_SEND = 1,
   RIP_CONF_SEND_RIPV1,
   RIP_CONF_SEND_RIPV1_COMPAT,
   RIP_CONF_SEND_RIPV2,
   RIP_CONF_SEND_RIPV1_DEMAND,                    /* not used */
   RIP_CONF_SEND_RIPV2_DEMAND                     /* not used */

}  RIP_CONF_SEND_t;

typedef enum
{
   RIP_CONF_RECV_RIPV1 = 1,
   RIP_CONF_RECV_RIPV2,
   RIP_CONF_RECV_RIPV1_RIPV2,
   RIP_CONF_RECV_DO_NOT_RECEIVE

}  RIP_CONF_RECV_t;

typedef enum
{
   RIP_SPLITHORIZON_NONE = 1,
   RIP_SPLITHORIZON_SIMPLE,
   RIP_SPLITHORIZON_POISONREVERSE,
   RIP_SPLITHORIZON_LAST                /* Should be the last entry */
}  RIP_SPLITHORIZON_t;

/* config parameter min/max/default values */
#define  RIP_CONF_METRIC_MIN                  1
#define  RIP_CONF_METRIC_MAX                  15


 /*--------------------------------------*/
/* OSPF Constants                       */
/*--------------------------------------*/

#define  OSPF_REDIST_TAG_MIN      0
#define  OSPF_REDIST_TAG_MAX      4294967295UL

#define  OSPF_DEFAULT_RT_ADVERTISE_METRIC_MIN      0
#define  OSPF_DEFAULT_RT_ADVERTISE_METRIC_MAX      16777214

/*--------------------------------------*/
/* BGP Constants                        */
/*--------------------------------------*/

#define  BGP4_DEFAULT_METRIC_MIN  1
#define  BGP4_DEFAULT_METRIC_MAX  4294967295UL

#define  BGP4_REDIST_METRIC_MIN   0
#define  BGP4_REDIST_METRIC_MAX   4294967295UL

/* Max length of the as-path string used in the command
 * 'set as-path prepend <as-path string>'
 * We accept upto 10 ASNs */
#define  MAX_ASN_IN_AS_PATH_PREPEND_STR          10
#define  MAX_STR_LEN_ASN                         10  /* Max length of the ASN in string
                                                        * format., eg. 4294967295 */

/* Assuming single space between each ASN and including terminating '\0'
 * character, add 10 more to the size of the string */
#define  MAX_AS_PATH_PREPEND_STR_LEN \
        (( MAX_ASN_IN_AS_PATH_PREPEND_STR *  MAX_STR_LEN_ASN) + \
                                MAX_ASN_IN_AS_PATH_PREPEND_STR)

/* Maximum number of characters in an AS Path string. If each
 * is a 10-digit AS with space or comma, then with 1024 characters, we can
 * represent ~85 ASs. That seems sufficient.
 * But this define is generally used for defining the size of character
 * array used to display the as path string (need not display the complete
 * as path string if it exceeds 1024 bytes)  */
#define BGP_AS_PATH_STR_MAX 1024

/* Maximum number of ASNs in a single AS Path segment */
#define BGP_MAX_ASN_PER_AS_SEG 255

#define  BGP_MAX_AS_LIMIT_MIN  1
#define  BGP_MAX_AS_LIMIT_MAX  300

/* Maximum number of characters in the max allowed AS path length.
 * Can be used internally for forming the complete AS path string
 * used to match against AS path regular expressions.
 * Length comes to around (100 * 11) = 1100 */
#define BGP_MAX_AS_PATH_STR_LEN ( BGP_MAX_AS_LIMIT_MAX * ( MAX_STR_LEN_ASN + 1))

/**************************************
*
*     VRRP DEFINES
*
***************************************/

/*--------------------------------------*/
/*  Authentication Constants            */
/*--------------------------------------*/

#define  VRRP_MAX_AUTH_DATA        8

/*--------------------------------------*/
/*  VRRP Constants                       */
/*--------------------------------------*/

/*------------------*/
/* VRRP TRAP TYPES  */
/*------------------*/

/* The following flags are to be used by user-interface routines to
   enable/disable or get the mode of the specified VRRP trap */
#define  VRRP_TRAP_ALL                              0x7
#define  VRRP_TRAP_NEW_MASTER                       0x1
#define  VRRP_TRAP_AUTH_FAILURE                     0x2
#define  VRRP_TRAP_PROTO_ERR                        0x4

/**************************************
*
*     BootP/DHCP Relay DEFINES
*
***************************************/

/*------------------------------------*/
/* BootP/DHCP Relay Parameter Limits  */
/*------------------------------------*/

#define  DHCP_MAX_HOP_COUNT       16
#define  DHCP_MIN_HOP_COUNT       1
#define  DHCP_MAX_WAIT_TIME       100
#define  DHCP_MIN_WAIT_TIME       0

#define  RTR_DEFAULT_BOOTP_DHCP_RELAY_SERVER "0.0.0.0"


#define  IP_HELPER_MIN_PORT       1
#define  IP_HELPER_MAX_PORT   65535


/* wildcard for IP helper ingress interface */
#define IH_INTF_ANY 0

/* wildcard for IP helper destination UDP port number */
#define IH_UDP_PORT_DEFAULT 0

/* Max length of a UDP port name */
#define IH_UDP_PORT_NAME_LEN 64

/* UDP Relay Constants */

#define  UDP_PORT_DHCP              67
#define  UDP_PORT_DOMAIN            53
#define  UDP_PORT_ISAKMP           500
#define  UDP_PORT_MOBILEIP         434
#define  UDP_PORT_NAMESERVER        42
#define  UDP_PORT_NETBIOS_DGM      138
#define  UDP_PORT_NETBIOS_NS       137
#define  UDP_PORT_NTP              123
#define  UDP_PORT_PIM_AUTO_RP      496
#define  UDP_PORT_RIP              520
#define  UDP_PORT_TACACS            49
#define  UDP_PORT_TFTP              69
#define  UDP_PORT_TIME              37



/*------------------------------*/
/* BootP/DHCP Relay TRAP TYPES  */
/*------------------------------*/

/* The following flags are to be used by user-interface routines to
   enable/disable or get the mode of the specified BootP/DHCP Relay trap */
#define  RELAY_TRAP_ALL                               0x07
#define  RELAY_TRAP_FORWARD_MODE                      0x01
#define  RELAY_TRAP_CIRCUITID_OPTION_MODE             0x02
#define  RELAY_TRAP_REMOTEID_OPTION_MODE              0x04


/* Virtual router. This version of FASTPATH has no virtual router support. These
 * defines are retained because they are referenced by ping and traceroute code. */
#define  VR_MAX_VIRTUAL_ROUTERS 1
#define  VALID_VRID 0

/* A way to indicate vrid is out-of-range */
#define  VR_NO_VRID 0xFFFFFFFF

/**************************************
*
*     IANA standard port DEFINES
*     http://www.iana.org/assignments/port-numbers
*
***************************************/
#define  IANA_IP_PORT_MIN                0
#define  IANA_IP_PORT_MAX                65535
#define  IANA_IP_PORT_WELL_KNOWN_MIN     0
#define  IANA_IP_PORT_WELL_KNOWN_MAX     1023
#define  IANA_IP_PORT_REGISTERED_MIN     1024
#define  IANA_IP_PORT_REGISTERED_MAX     49151
#define  IANA_IP_PORT_PRIVATE_MIN        49152
#define  IANA_IP_PORT_PRIVATE_MAX        65535

#ifdef INCLUDE_L3_COMMDEFS_OVERRIDES
#include "l3_commdefs_overrides.h"
#endif

#endif /* L3_COMMDEFS_H */
