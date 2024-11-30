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

#ifndef __IP_EXPORTS_H_
#define __IP_EXPORTS_H_

#include "datatypes.h"
#include "commdefs.h"

#define  NULL_IP_ADDR 0
#define  NULL_IP_MASK 0

/* Controls whether we track file and line numbers for memory allocations 
 * from the routing heap. Enabled in FASTPATH by default. 
 * Turning this flag off decreases the size of the routing heap by 30% or so. */
#define DO_MEMCHK  1


/* Maximum number of unique next hops in RTO. Most routed network designs today use
 * point-to-point links, with a max of one neighbor per routing interface. Allow four
 * neighbors per routing interface. */
#ifdef  MPLS_PACKAGE
#define RTO_MAX_UNIQUE_NEXT_HOPS   (16384)
#define RTO6_MAX_UNIQUE_NEXT_HOPS  (16384)
#else
#define RTO_MAX_UNIQUE_NEXT_HOPS   (4 *  MAX_NUM_ROUTER_INTF)
#define RTO6_MAX_UNIQUE_NEXT_HOPS  (4 *  MAX_NUM_ROUTER_INTF)
#endif

/* Maximum number of subscribers for event of creation of the routing interface for management VLAN */
#define  IP_VR_CREATE_REGISTRATIONS_MAX 9

typedef RC_t (* IP_VR_CREATE_CB)( COMPONENT_IDS_t registrar_ID, 
                                      uint32 vlanId, uint32 rtrIntfNum);

typedef struct ipMapMVlanRIntfNotifyList_s
{
   COMPONENT_IDS_t  registrar_ID;
   IP_VR_CREATE_CB  intfCreateCB;

} ipMapMVlanRIntfNotifyList_t;


/* IP Address type - from RFC 2851 */
typedef enum
{
   IP_ADDRESS_TYPE_UNKNOWN = 0,
   IP_ADDRESS_TYPE_IPV4 = 1,
   IP_ADDRESS_TYPE_IPV6 = 2,
   IP_ADDRESS_TYPE_DNS = 16
}  IP_ADDRESS_TYPE_t;


/*--------------------------------------*/
/*  RTR DISC Constants                  */
/*--------------------------------------*/
/* Router Constants */
#define MAX_INITIAL_ADVERT_INTERVAL   16 /* seconds */
#define MAX_INITIAL_ADVERTISEMENTS    3  /* transmissions */
#define MAX_RESPONSE_DELAY        2  /* seconds */


#define  DEFAULT_ADV_ADDR     0xE0000001          /* 224.0.0.1 */
#define MAX_ADV_INTERVAL        600
#define ADVERTISEMENT            FALSE
#define PREFERENCE_LEVEL        0


/*--------------------------------------*/
/*  RLIM Constants                      */
/*--------------------------------------*/
/*
 * Tunnel Mode values
 */
typedef enum
{
   TUNNEL_MODE_INVALID = 0,
   TUNNEL_MODE_UNDEFINED = 1,
   TUNNEL_MODE_6OVER4,
   TUNNEL_MODE_6TO4,
   TUNNEL_MODE_ISATAP,
   TUNNEL_MODE_IP6,
   TUNNEL_MODE_4OVER4,
}  TUNNEL_MODE_t;

/*--------------------------------------*/
/*  Authentication Constants            */
/*--------------------------------------*/

#define  AUTH_MAX_KEY 255

typedef enum
{
   AUTH_TYPE_NONE = 0,
   AUTH_TYPE_SIMPLE_PASSWORD,
   AUTH_TYPE_MD5

}  AUTH_TYPES_t;


/* IP MAP Component Feature List */
typedef enum
{
   IPMAP_FEATURE_LAG_RTING_SUPPORT_FEATURE_ID = 0,
   IPMAP_FEATURE_PROXY_ARP,
   IPMAP_FEATURE_MULTINETTING,
   IPMAP_FEATURE_REJECTROUTE,
   IPMAP_FEATURE_PORT_BASED_ROUTING_SUPPORT_FEATURE_ID,
   IPMAP_FEATURE_PER_PORT_ICMP_REDIRECTS,
   IPMAP_FEATURE_RESILIENT_HASHING,
   IPMAP_FEATURE_RFC5549,
   IPMAP_FEATURE_ECMP_LB,                  /* general ECMP LB feature */
   IPMAP_FEATURE_ECMP_LB_HASH,
   IPMAP_FEATURE_URPF,  
   IPMAP_FEATURE_ECMP_IPSEC_SPI_HASH,      /* Support ECMP hashing on IPSEC SPI field. */
   IPMAP_FEATURE_ECMP_DYNAMIC_LOAD_BALANCE_HASH,
   IPMAP_FEATURE_IP_MTU_INDEPENDENT_L2_MTU,
   IPMAP_FEATURE_ID_TOTAL                  /* total number of enum values */
}  IPMAP_FEATURE_IDS_t;

/*--------------------------------------*/
/*  IP AT Table Public Constants           */
/*--------------------------------------*/

typedef enum
{
   IP_AT_TYPE_OTHER = 1,
   IP_AT_TYPE_INVALID,
   IP_AT_TYPE_DYNAMIC,
   IP_AT_TYPE_STATIC,
   IP_AT_TYPE_LOCAL
}  IP_AT_TYPE_t;

/*--------------------------------------*/
/*  ARP Constants                       */
/*--------------------------------------*/
#define  IP_ARP_MAX_QUERY             5             /* hard limit needed for DAPI */
/*********************************************************************
*
*  RT_BRIDGING_ROUTING_COEXISTANCE
*
* If  RT_BRIDGE_FIRST_THEN_ROUTE, all frames received on an interface
* which is enabled for bridging are first bridged, and then routed.
*
* If  RT_BRIDGE_OR_ROUTE, all routable frames are routed. All others 
* are bridged.
*
* The aforementioned choices are mutually exclusive.
* VLAN routing is available only when  RT_BRIDGE_FIRST_THEN_ROUTE is
* selected.
*
*
*********************************************************************/

typedef enum
{
   RT_BRIDGE_FIRST_THEN_ROUTE = 1,
   RT_BRIDGE_OR_ROUTE

}  RT_BRIDGING_ROUTING_COEXISTANCE_t;


#define  RT_BRIDGING_ROUTING_COEXISTANCE   RT_BRIDGE_FIRST_THEN_ROUTE

#define  CIDR_MAX_SIZE                    4  /* size of uint32 */

/*--------------------------*/
/* DAI specific constants */
/*--------------------------*/
typedef enum
{
   DAI_REMARKS_FEATURE_ID = 0,
   DAI_TOTAL_FEATURE_ID
}  DAI_FEATURE_IDS_t;

#define DAI_TRUSTED_PORT_NOTIFY            0x00000001
#define DAI_UNTRUSTED_PORT_NOTIFY          0x00000002

typedef struct daiNotifyData_s
{
  uint32         intIfNum;

} daiNotifyData_t;

/* Notify Request function pointer */
typedef  RC_t  (*daiNotifyRequest_t)(daiNotifyData_t *daiData, uint32 event);

typedef enum
{
   ARP_ACL_ACTION_PERMIT = 1,
   ARP_ACL_ACTION_DENY
}  ARP_ACL_ACTION_t;

#define  ARP_ACL_REMARK_MAX_PER_RULE 10
#define  ARP_ACL_REMARK_LEN_MAX      100  /* doesn't include trailing '\0' */
#define  ARP_ACL_REMARK_LEN_MIN      1

#define  ARP_ACL_NAME_LEN_MIN              1
#define  ARP_ACL_NAME_LEN_MAX             31
#define  ARP_ACL_CMDS_MAX                100
#define  ARP_ACL_RULES_MAX                20
#define  DAI_RATE_LIMIT_NONE              -1
#define  DAI_RATE_LIMIT_MIN                0
#define  DAI_RATE_LIMIT_MAX              300
#define  DAI_BURST_INTERVAL_MIN            1
#define  DAI_BURST_INTERVAL_MAX           15

/*--------------------------*/
/* Virtual Router constants */
/*--------------------------*/
#ifdef  VRF_PACKAGE
#define  MAX_VR_INSTANCES       64
#else
#define  MAX_VR_INSTANCES        0
#endif

#define  VRF_MAX_NAME_LEN       15
#define  VRF_DESCR_MAX_LEN     512

/*---------------------------------------*/
/*  IP event dampening configuration return values */
/*---------------------------------------*/

typedef enum {
   IP_EVT_DAMP_INVALID,
   IP_EVT_DAMP_INTERFACE_IS_NOT_CONFIGURABLE,
   IP_EVT_DAMP_MAX_PENALTY_GREATER_THAN_RESTRICTED,
   IP_EVT_DAMP_SUPPRESS_THRESHOLD_GREATER_THAN_MAX_PENALTY,
   IP_EVT_DAMP_RESTART_PENALTY_GREATER_THAN_MAX_PENALTY,
   IP_EVT_DAMP_CFG_SUCCESS,
   IP_EVT_DAMP_CFG_FAILURE
} ipevtdampcfg_RC_t;

/*------------------------------------*/
/*  IP EVENT DAMPENING DEFAULT VALUES        */
/*------------------------------------*/

#define  IP_EVT_DAMP_DEFAULT_HALF_LIFE_PERIOD   5
#define  IP_EVT_DAMP_DEFAULT_REUSE_THRESHOLD    1000
#define  IP_EVT_DAMP_DEFAULT_SUPPRESS_THRESHOLD 2000
#define  IP_EVT_DAMP_DEFAULT_MAX_SUPPRESS_TIME  (4 *  IP_EVT_DAMP_DEFAULT_HALF_LIFE_PERIOD)
#define  IP_EVT_DAMP_DEFAULT_RESTART_PENALTY    2000

#define  IP_EVT_DAMP_MIN_PENALTY                1
#define  IP_EVT_DAMP_MAX_PENALTY                20000

#define  IP_EVT_DAMP_MIN_HALF_LIFE_PERIOD       1
#define  IP_EVT_DAMP_MAX_HALF_LIFE_PERIOD       30

#define  IP_EVT_DAMP_MIN_REUSE_TRESHOLD         1
#define  IP_EVT_DAMP_MAX_REUSE_TRESHOLD         20000

#define  IP_EVT_DAMP_MIN_SUPPRESS_TRESHOLD      1
#define  IP_EVT_DAMP_MAX_SUPPRESS_TRESHOLD      20000

#define  IP_EVT_DAMP_MIN_MAX_SUPPRESS_TIME      1
#define  IP_EVT_DAMP_MAX_MAX_SUPPRESS_TIME      255

#define  IP_EVT_DAMP_MIN_RESTART_PENALTY        1
#define  IP_EVT_DAMP_MAX_RESTART_PENALTY        20000

/* Defines for IP ECMP LB Modes and defaults */
typedef enum {
   IP_ECMP_LB_MIN = 0,                          /* general mode */
   IP_ECMP_LB_HASH_IN_SIP,
   IP_ECMP_LB_HASH_IN_DIP,
   IP_ECMP_LB_HASH_IN_SIP_DIP,
   IP_ECMP_LB_HASH_IN_SIP_SPORT,
   IP_ECMP_LB_HASH_IN_DIP_DPORT,
   IP_ECMP_LB_HASH_IN_SIP_DIP_SPORT_DPORT,
   IP_ECMP_LB_HASH_OUT_SIP,
   IP_ECMP_LB_HASH_OUT_DIP,
   IP_ECMP_LB_HASH_OUT_SIP_DIP,
   IP_ECMP_LB_HASH_OUT_SIP_SPORT,
   IP_ECMP_LB_HASH_OUT_DIP_DPORT,
   IP_ECMP_LB_HASH_OUT_SIP_DIP_SPORT_DPORT,
   IP_ECMP_LB_MAX                               /* total number of enum values */
}  IP_ECMP_LB_MODE_t;

#define FD_IP_ECMP_LB_MODE       IP_ECMP_LB_HASH_IN_SIP_DIP_SPORT_DPORT
#define FD_IP_ECMP_INOUT_OFFSET ( IP_ECMP_LB_HASH_OUT_SIP -  IP_ECMP_LB_HASH_IN_SIP)

/* Flag indicating whether the IPSEC load balancing is enabled on the
** Security Parameter Index (SPI) field. 
*/ 
#define FD_IP_ECMP_IPSEC_SPI_MODE    ENABLE

/*ECMP Dynamic Load Balance Modes */
#define  IP_ECMP_HASH_DLB_DISABLE          0
#define  IP_ECMP_HASH_DLB_NORMAL_MODE      1
#define  IP_ECMP_HASH_DLB_ASSIGNED_MODE    2
#define  IP_ECMP_HASH_DLB_OPTIMAL_MODE     3

/******************** conditional Override *****************************/

#ifdef INCLUDE_IP_EXPORTS_OVERRIDES
#include "ip_exports_overrides.h"
#endif

#endif /* __IP_EXPORTS_H_*/
