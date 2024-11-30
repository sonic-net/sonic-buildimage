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


#ifndef INCLUDE_COMMDEFS
#define INCLUDE_COMMDEFS

//#include "resources.h"
//#include "commdefs_pkg.h"


#define  USMDB_UNIT_ZERO 0
#define  LOGICAL_UNIT 0

/*--------------------------------------*/
/*  Commonly Used Constants             */
/*--------------------------------------*/
#define  NULLPTR  ((void *)0)

#define  EOS      ((unsigned char)'\0')   /* ASCIIZ end-of-string character */

#define  ONE_KB_UNIT      1024
#define  ONE_MB_UNIT      1048576         /* 1024 * 1024 */

#define  ENABLE           1
#define  DISABLE          0
#define  DIAG_DISABLE     2

#define  ON   1
#define  OFF  0

#define  YES  1
#define  NO   0

#define  OK           0
#define  NO_WAIT      0
#define  WAIT_FOREVER (-1)
#define  ALLONES      (-1)
#define  MSG_PRIORITY_NORM  0
#define  MSG_PRIORITY_HI    1

#define  MATCH_EXACT     1
#define  MATCH_GETNEXT   2
#define  MATCH_FREE      3


#ifndef min
  #define min(x, y) (((x) < (y)) ? (x) : (y))
#endif

#ifndef max
  #define max(x, y) (((x) < (y)) ? (y) : (x))
#endif

#define  COUNTOF(ary)       ((int) (sizeof (ary) / sizeof ((ary)[0])))

/* Admin mode */
typedef enum
{
   ADMIN_MODE_DISABLE =  DISABLE,
   ADMIN_MODE_ENABLE =  ENABLE
}  ADMIN_MODE_t;

/* Features or components supported */

typedef enum
{
   FIRST_COMPONENT_ID,
   DTL_COMPONENT_ID,
   CARDMGR_COMPONENT_ID,
   NIM_COMPONENT_ID,
   PORT_MIRROR_COMPONENT_ID,
   AUTHMGR_COMPONENT_ID,
   DOT1X_COMPONENT_ID,
   MAB_COMPONENT_ID,
   LAST_COMPONENT_ID
}  COMPONENT_IDS_t;

/* Maximum interface name length */
#define  INTF_NAME_MAX_LEN 32

typedef enum
{
   OSAPI_TIMER_TASK_SYNC,
   NIM_TASK_SYNC,
   DOT1X_TASK_SYNC,
   MAB_TASK_SYNC,
   MAB_SRVR_TASK_SYNC,
   MAB_DB_TASK_SYNC,
   MAB_ELOOP_TASK_SYNC,
   DOT1X_TIMER_TASK_SYNC,
   AUTHMGR_TASK_SYNC,
   AUTHMGR_TIMER_TASK_SYNC,
   AUTHMGR_SRVR_TASK_SYNC,
   AUTHMGR_DB_TASK_SYNC,
   TASK_SYNC_LAST
}  TASK_SYNCS_t;

/*********************************************************************
*
* PORT (OR INTERFACE) SECTION
*
*********************************************************************/

/*--------------------------------------*/
/*  Port (or Interface) Events          */
/*--------------------------------------*/

typedef enum
{
   PORT_DISABLE = 0,
   PORT_ENABLE,
   PORT_INSERT,
   DOWN,
   UP,
   INACTIVE,
   ACTIVE,
   FORWARDING,
   NOT_FORWARDING,
   CREATE,
   CREATE_COMPLETE,
   DELETE_PENDING,
   DELETE,
   DELETE_COMPLETE,
   LAG_ACQUIRE,
   LAG_RELEASE,
   SPEED_CHANGE,
   LAG_CFG_CREATE,
   LAG_CFG_MEMBER_CHANGE,
   LAG_CFG_REMOVE,
   LAG_CFG_END,
   PROBE_SETUP,
   PROBE_TEARDOWN,
   SET_INTF_SPEED,                                                      /* Affects Spanning Tree when path cost dynamically calculated */
   SET_MTU_SIZE,                                                        /* Affects IP layers */
   PORT_ROUTING_ENABLED,
   PORT_ROUTING_DISABLED,
   PORT_BRIDGING_ENABLED,
   PORT_BRIDGING_DISABLED,
   VRRP_TO_MASTER,
   VRRP_FROM_MASTER,
   AUTHMGR_PORT_AUTHORIZED,
   AUTHMGR_PORT_UNAUTHORIZED,
   ATTACH,
   ATTACH_COMPLETE,
   DETACH,
   DETACH_COMPLETE,
   AUTHMGR_ACQUIRE,
   AUTHMGR_RELEASE,
   PORT_STATS_RESET,
   PORT_AGG_UP,
   PORT_AGG_DOWN,
   PORT_PFC_ACTIVE,
   PORT_PFC_INACTIVE,
   TRILL_TRUNK_ENABLED,
   TRILL_TRUNK_DISABLED,
   ETHERNET_SWITCHPORT,
   NETWORK_INTF_TYPE_NNI,
   NETWORK_INTF_TYPE_UNI_C,
   NETWORK_INTF_TYPE_UNI_S,
   LAG_RELEASE_PENDING,
   LAG_DOWN_PENDING,
   LAG_ACQUIRE_PENDING,
   LAST_PORT_EVENT

}  PORT_EVENTS_t;


/*--------------------------------------*/
/*  Interface Acquire Actions           */
/*--------------------------------------*/

typedef enum
{
   INTF_ACQ_ACTION_NONE = 0,
   INTF_ACQ_ACTION_ACQUIRE,
   INTF_ACQ_ACTION_RELEASE

}  INTF_ACQUIRE_ACTIONS_t;


typedef enum
{
  CLI_PORT_MEDIUM_NONE              = 0,
  CLI_PORT_MEDIUM_COPPER            = 1,
  CLI_PORT_MEDIUM_FIBER             = 2,
  CLI_PORT_MEDIUM_COUNT             /* last, please */
} cli_port_medium_t;



/*--------------------------------------*/
/*  INTERFACE STATES                    */
/*--------------------------------------*/
typedef enum
{

   INTF_UNINITIALIZED = 0,  /* The interface doesn't exist */
   INTF_CREATING,           /* An  CREATE event is in process */
   INTF_CREATED,            /* The  CREATE event has been processed by all components */
   INTF_ATTACHING,          /* An  ATTACH event is in process */
   INTF_ATTACHED,           /* The  ATTACH event has been processed by all components */
   INTF_DETACHING,          /* An  DETACH event is in process, next state is  INTF_CREATED */
   INTF_DELETING,           /* An  DELETE event is in process, next state is  INTF_UNINITIALIZED */

}  INTF_STATES_t;

/****************************************
*
*  LVL7 Interfaces Types
*
*  If a value is removed from this enum,
*  replace it with  UNUSED<N>_INTF to
*  preserve the existing values which are
*  used in config.
*
*****************************************/
typedef enum
{
   INTF_TYPES_INVALID = 0,
/* Physical Interfaces */
   PHYSICAL_INTF = 3,
   STACK_INTF,
   CPU_INTF,
   MAX_PHYS_INTF_VALUE,
/* Logical Interfaces */
   LAG_INTF,
   UNUSED1_INTF,
   LOGICAL_VLAN_INTF,
   LOOPBACK_INTF,
   TUNNEL_INTF,
   UNUSED2_INTF,
   UNUSED3_INTF,
   SERVICE_PORT_INTF,
   MAX_INTF_TYPE_VALUE

} INTF_TYPES_t;

/****************************************
*
*  LVL7 Interfaces Parameter Types
*
*****************************************/
typedef enum
{
   INTF_PARM_ADMINSTATE       = 0x1,
   INTF_PARM_SPEED            = 0x2,
   INTF_PARM_AUTONEG          = 0x4,
   INTF_PARM_MTU              = 0x8,
   INTF_PARM_FRAMESIZE        = 0x10,
   INTF_PARM_MACADDR          = 0x20,
   INTF_PARM_LINKTRAP         = 0x40,
   INTF_PARM_LOOPBACKMODE     = 0x80,
   INTF_PARM_MACROPORT        = 0x100,
   INTF_PARM_ENCAPTYPE        = 0x200,
   INTF_PARM_CONNECTOR        = 0x400,
   INTF_PARM_IFDESC           = 0x800,
   INTF_PARM_CP_CAP           = 0x1000,
   INTF_PARM_DEBOUNCETIME     = 0x2000,
   INTF_PARM_PORTMODE         = 0x4000,
   INTF_PARM_FEC_MODE         = 0x8000,
   INTF_PARM_NW_INTF_TYPE     = 0x10000,
   INTF_PARM_LT_MODE          = 0x20000,
}  INTF_PARM_TYPES_t;

/*--------------------------------------*/
/*  IANAifTypes INTERFACE TYPES         */
/*--------------------------------------*/

/* From IANAifType-MIB */

/* Purposely unused in Stingray are:
        iso88023Csmacd
        ipSwitch
        ipForward
        fibreChannel
        l3ipVlan
        l3ipxVlan
        hyperchannel
*/
typedef enum
{
   IANA_INVALID          = 0,
   IANA_OTHER            = 1,       /* other (unknown)    */
#define IANA_OTHER_DESC             "Unknown Port Type"

   IANA_OTHER_CPU        = 1,       /* other CPU COUNTERS */
#define IANA_CPU_DESC               " "

   IANA_OTHER_SERV_PORT  = 2,       /* other SERVICE PORT value is IANA TBD */
#define IANA_OTHER_SERV_PORT_DESC   "Service Port"

   IANA_ETHERNET         = 6,       /* EthernetCsmacd     */
#define IANA_ETHERNET_DESC           "Ethernet Csmacd"

   IANA_SOFTWARE_LOOPBACK = 24,     /* software loopback  */
#define IANA_SOFTWARE_LOOPBACK_DESC  "software loopback"

   IANA_AAL5             = 49,      /* AAL5 over ATM      */
#define IANA_AAL5_DESC               "AAL5 over ATM"

   IANA_PROP_VIRTUAL     = 53,      /* proprietary virtual/internal */
#define IANA_PROP_VIRTUAL_DESC       "proprietary virtual/internal"

   IANA_FAST_ETHERNET    = 62,      /* 100BaseT           */
#define IANA_FAST_ETHERNET_DESC     "10/100 Copper - Level"

   IANA_FAST_ETHERNET_FX = 69,      /* 100BaseFX          */
#define IANA_FAST_ETHERNET_FX_DESC  "100 Fiber - Level"

   IANA_GIGABIT_ETHERNET = 117,     /* Gigabit Ethernet   */
#define IANA_GIGABIT_ETHERNET_DESC  "Gigabit - Level"

   IANA_TUNNEL = 131,               /* tunnel   */
#define IANA_TUNNEL_DESC            "tunnel"

   IANA_L2_VLAN          = 135,     /* 802.1Q VLAN        */
#define IANA_L2_VLAN_DESC            "802.1Q VLAN"

   IANA_LAG              = 161,     /* Link Aggregation   */
#define IANA_LAG_DESC            "Link Aggregate"

   IANA_LOGICAL          = 162,      /* need to find out this ??? */
#define IANA_LOGICAL_PORT_DESC       "Logical Interface"

   IANA_2P5G_ETHERNET     = 205,    /* 2.5G Ethernet       */
#define IANA_2P5G_ETHERNET_DESC      "2.5G - Level"

   IANA_5G_ETHERNET     = 207,    /* 5G Ethernet       */
#define IANA_5G_ETHERNET_DESC      "5G - Level"

   IANA_10G_ETHERNET     = 198,     /* 10G Ethernet       */
#define IANA_10G_ETHERNET_DESC      "10G - Level"

   IANA_20G_ETHERNET     = 200,    /* 20G Ethernet       */
#define IANA_20G_ETHERNET_DESC      "20G - Level"

   IANA_25G_ETHERNET     = 203,    /* 25G Ethernet       */
#define IANA_25G_ETHERNET_DESC      "25G - Level"

   IANA_40G_ETHERNET     = 202,    /* 40G Ethernet       */
#define IANA_40G_ETHERNET_DESC      "40G - Level"

   IANA_50G_ETHERNET     = 206,    /* 50G Ethernet       */
#define IANA_50G_ETHERNET_DESC      "50G - Level"

   IANA_100G_ETHERNET     = 204,    /* 100G Ethernet       */
#define IANA_100G_ETHERNET_DESC      "100G - Level"

   IANA_200G_ETHERNET     = 208,    /* 200G Ethernet       */
#define IANA_200G_ETHERNET_DESC      "200G - Level"

   IANA_400G_ETHERNET     = 209,    /* 400G Ethernet       */
#define IANA_400G_ETHERNET_DESC      "400G - Level"
} IANA_INTF_TYPE_t;

#define IANA_STACK_DESC
#define IANA_VLAN_DESC               "VLAN Routing Interface"

/* LLD_NOTE (trunking): Find IANA for trunking */

/*---------------------------*/
/* Physical Interface Types  */
/*---------------------------*/

typedef enum
{
   ETHERNET = 0,
   FRAME_RELAY,
   ATM
} PHY_INTF_TYPE_t;

/* This constant is to only be used when configuring a command to apply to all interfaces. */
#define  ALL_INTERFACES ((uint32)( MAX_INTERFACE_COUNT + 1))

/* The default port speed to be assumed when the speed cannot be successfully
   returned by NIM for reasons such as the link is down, in that case we assume
   a speed of 100 full duplex */
#define  PORTCTRL_PORTSPEED_DEFAULT 200

/*--------------------------------------*/
/*  Port characteristics                */
/*--------------------------------------*/

typedef enum
{
   PORT_NORMAL = 1,
   TRUNK_MEMBER,
   MIRRORED_PORT,
   PROBE_PORT,
   L2TUNNEL_PORT,
   SERVICE_PORT
}  SPECIAL_PORT_TYPE_t;

/* Common define for port control settings */

typedef enum
{
   PORTCTRL_PORTSPEED_AUTO_NEG   = 1,
   PORTCTRL_PORTSPEED_HALF_100TX,
   PORTCTRL_PORTSPEED_FULL_100TX,
   PORTCTRL_PORTSPEED_HALF_10T,
   PORTCTRL_PORTSPEED_FULL_10T,
   PORTCTRL_PORTSPEED_FULL_100FX,
   PORTCTRL_PORTSPEED_FULL_1000SX,
   PORTCTRL_PORTSPEED_FULL_10GSX,
   PORTCTRL_PORTSPEED_FULL_20GSX,
   PORTCTRL_PORTSPEED_FULL_25GSX,
   PORTCTRL_PORTSPEED_FULL_40GSX,
   PORTCTRL_PORTSPEED_FULL_50GSX,
   PORTCTRL_PORTSPEED_FULL_100GSX,
   PORTCTRL_PORTSPEED_AAL5_155,
   PORTCTRL_PORTSPEED_FULL_5FX, 
   PORTCTRL_PORTSPEED_FULL_200GSX,
   PORTCTRL_PORTSPEED_FULL_400GSX,
   PORTCTRL_PORTSPEED_FULL_2P5FX = 128, /* Speed 2.5G FD, Bit 7th set */
   PORTCTRL_PORTSPEED_LAG,
   PORTCTRL_PORTSPEED_UNKNOWN
}  PORT_SPEEDS_t;


typedef enum
{
   PHY_CAP_PORTSPEED_HALF_10    = 0x00000001,
   PHY_CAP_PORTSPEED_FULL_10    = 0x00000002,
   PHY_CAP_PORTSPEED_HALF_100   = 0x00000004,
   PHY_CAP_PORTSPEED_FULL_100   = 0x00000008,
   PHY_CAP_PORTSPEED_HALF_1000  = 0x00000010,
   PHY_CAP_PORTSPEED_FULL_1000  = 0x00000020,
   PHY_CAP_PORTSPEED_FULL_2P5G  = 0x00000040,
   PHY_CAP_PORTSPEED_FULL_5G    = 0x00000080,
   PHY_CAP_PORTSPEED_FULL_10G   = 0x00000100,
  /*                              = 0x00000200, */ 
   PHY_CAP_PORTSPEED_AUTO_NEG   = 0x00000400,
   PHY_CAP_PORTSPEED_SFP        = 0x00000800,
   PHY_CAP_PORTSPEED_SFP_DETECT = 0x00001000,
   PHY_CAP_POE_PSE              = 0x00002000,
   PHY_CAP_POE_PD               = 0x00004000,
   PHY_CAP_INTERNAL             = 0x00008000,  /* Indicates a blade switch internal port */
   PHY_CAP_NATIVE_EEE           = 0x00010000,
   PHY_CAP_AUTO_EEE             = 0x00020000,
   PHY_CAP_DUAL_MODE_SUPPORT    = 0x00040000,   /* Used represent PHYs where phy/port capabilities change
                                                dynamically(ex: SFP support in 10G fiber port, Combo port where two phys
                                                are used one for fiber, one for copper)*/
   PHY_CAP_POE_PSE_PLUS         = 0x00080000,
   PHY_CAP_POE_PD_PLUS          = 0x00100000,
   PHY_CAP_ENERGY_DETECT        = 0x00200000,
   PHY_CAP_COMBO_PREFERENCE     = 0x00400000,  /* If the port is a combo port *and* the preferred media can be configured. */
   PHY_CAP_EXPANDABLE_PORT      = 0x00800000,
   PHY_CAP_AUTO_CONFIG          = 0x01000000,
   PHY_CAP_SHORT_REACH          = 0x02000000,
   PHY_CAP_UPOE_PSE             = 0x04000000,  /* UPOE capability bits */
   PHY_CAP_UPOE_PD              = 0x08000000,
   PHY_CAP_DYNAMIC_PORT         = 0x10000000,  /* This should be used instead of DUAL_MODE */
   PHY_CAP_FIXED_STACK_PORT     = 0x20000000,
  
   PHY_CAP_FEC_SUPPORT          = 0x40000000,
   PHY_CAP_PORTSPEED_FULL_20G   = 0x80000000,
   PHY_CAP_PORTSPEED_FULL_25G   = 0x0100000000ULL,
   PHY_CAP_PORTSPEED_FULL_40G   = 0x0200000000ULL,
   PHY_CAP_PORTSPEED_FULL_50G   = 0x0400000000ULL,
   PHY_CAP_PORTSPEED_FULL_100G  = 0x0800000000ULL,
   PHY_CAP_PORTSPEED_FULL_200G  = 0x1000000000ULL,
   PHY_CAP_PORTSPEED_FULL_400G  = 0x2000000000ULL,
   PHY_CAP_LT_SUPPORT           = 0x4000000000ULL, /* Link-training capability */
   PHY_CAP_POE_BT               = 0x8000000000ULL,
   PHY_CAP_MACSEC               = 0x010000000000ULL,

}  PHY_CAPABILITIES_t;


#define  PHY_CAP_BIT_CHECK(phy_cap, __field) ((phy_cap) & ( PHY_CAP_PORTSPEED_##__field))
#define  PHY_CAP_FIELD_CHECK(phy_cap, __field)            \
  (((phy_cap) & ( PHY_CAP_##__field)) == ( PHY_CAP_##__field))
#define  PHY_CAP_CHECK(phy_cap, cap)                 (phy_cap & cap)

#define  PHY_CAP_PORTSPEED_AUTO_NEG_CHECK(phy_cap)   ( PHY_CAP_BIT_CHECK(phy_cap, AUTO_NEG))
#define  PHY_CAP_PORTSPEED_SFP_CHECK(phy_cap)        ( PHY_CAP_BIT_CHECK(phy_cap, SFP))
#define  PHY_CAP_PORTSPEED_SFP_DETECT_CHECK(phy_cap) ( PHY_CAP_BIT_CHECK(phy_cap, SFP_DETECT))
#define  PHY_CAP_PORTSPEED_HALF_10_CHECK(phy_cap)    ( PHY_CAP_BIT_CHECK(phy_cap, HALF_10))
#define  PHY_CAP_PORTSPEED_FULL_10_CHECK(phy_cap)    ( PHY_CAP_BIT_CHECK(phy_cap, FULL_10))
#define  PHY_CAP_PORTSPEED_HALF_100_CHECK(phy_cap)   ( PHY_CAP_BIT_CHECK(phy_cap, HALF_100))
#define  PHY_CAP_PORTSPEED_FULL_100_CHECK(phy_cap)   ( PHY_CAP_BIT_CHECK(phy_cap, FULL_100))
#define  PHY_CAP_PORTSPEED_HALF_1000_CHECK(phy_cap)  ( PHY_CAP_BIT_CHECK(phy_cap, HALF_1000))
#define  PHY_CAP_PORTSPEED_FULL_1000_CHECK(phy_cap)  ( PHY_CAP_BIT_CHECK(phy_cap, FULL_1000))
#define  PHY_CAP_PORTSPEED_FULL_2P5G_CHECK(phy_cap)  ( PHY_CAP_BIT_CHECK(phy_cap, FULL_2P5G))
#define  PHY_CAP_PORTSPEED_FULL_5G_CHECK(phy_cap)    ( PHY_CAP_BIT_CHECK(phy_cap, FULL_5G))
#define  PHY_CAP_PORTSPEED_FULL_10G_CHECK(phy_cap)   ( PHY_CAP_BIT_CHECK(phy_cap, FULL_10G))


#define  PHY_CAP_PORTSPEED_FULL_20G_CHECK(phy_cap)   ( PHY_CAP_BIT_CHECK(phy_cap, FULL_20G))
#define  PHY_CAP_PORTSPEED_FULL_25G_CHECK(phy_cap)   ( PHY_CAP_BIT_CHECK(phy_cap, FULL_25G))
#define  PHY_CAP_PORTSPEED_FULL_40G_CHECK(phy_cap)   ( PHY_CAP_BIT_CHECK(phy_cap, FULL_40G))
#define  PHY_CAP_PORTSPEED_FULL_50G_CHECK(phy_cap)   ( PHY_CAP_BIT_CHECK(phy_cap, FULL_50G))
#define  PHY_CAP_PORTSPEED_FULL_100G_CHECK(phy_cap)  ( PHY_CAP_BIT_CHECK(phy_cap, FULL_100G))
#define  PHY_CAP_PORTSPEED_FULL_200G_CHECK(phy_cap)  ( PHY_CAP_BIT_CHECK(phy_cap, FULL_200G))
#define  PHY_CAP_PORTSPEED_FULL_400G_CHECK(phy_cap)  ( PHY_CAP_BIT_CHECK(phy_cap, FULL_400G))
#define  PHY_CAP_LT_SUPPORT_CHECK(phy_cap)           ( PHY_CAP_FIELD_CHECK(phy_cap, LT_SUPPORT))
#define  PHY_CAP_FEC_SUPPORT_CHECK(phy_cap)          ( PHY_CAP_FIELD_CHECK(phy_cap, FEC_SUPPORT))
#define  PHY_CAP_DUAL_MODE_SUPPORT_CHECK(phy_cap)    ( PHY_CAP_FIELD_CHECK(phy_cap, DUAL_MODE_SUPPORT))
#define  PHY_CAP_COMBO_PREFERENCE_CHECK(phy_cap)     ( PHY_CAP_FIELD_CHECK(phy_cap, COMBO_PREFERENCE))
#define  PHY_CAP_EXPANDABLE_PORT_CHECK(phy_cap)      ( PHY_CAP_FIELD_CHECK(phy_cap, EXPANDABLE_PORT))
#define  PHY_CAP_NATIVE_EEE_CHECK(phy_cap)           ( PHY_CAP_FIELD_CHECK(phy_cap, NATIVE_EEE))
#define  PHY_CAP_AUTO_EEE_CHECK(phy_cap)             ( PHY_CAP_FIELD_CHECK(phy_cap, AUTO_EEE))
#define  PHY_CAP_DYNAMIC_PORT_CHECK(phy_cap)         ( PHY_CAP_FIELD_CHECK(phy_cap, DYNAMIC_PORT))
#define  PHY_CAP_ENERGY_DETECT_CHECK(phy_cap)        ( PHY_CAP_FIELD_CHECK(phy_cap, ENERGY_DETECT))
#define  PHY_CAP_SHORT_REACH_CHECK(phy_cap)          ( PHY_CAP_FIELD_CHECK(phy_cap, SHORT_REACH))
#define  PHY_CAP_AUTO_CONFIG_CHECK(phy_cap)          ( PHY_CAP_FIELD_CHECK(phy_cap, AUTO_CONFIG))
#define  PHY_CAP_INTERNAL_CHECK(phy_cap)             ( PHY_CAP_FIELD_CHECK(phy_cap, INTERNAL))
#define  PHY_CAP_POE_PSE_CHECK(phy_cap)              ( PHY_CAP_FIELD_CHECK(phy_cap, POE_PSE))
#define  PHY_CAP_POE_PD_CHECK(phy_cap)               ( PHY_CAP_FIELD_CHECK(phy_cap, POE_PD))
#define  PHY_CAP_POE_PSE_PLUS_CHECK(phy_cap)         ( PHY_CAP_FIELD_CHECK(phy_cap, POE_PSE_PLUS))
#define  PHY_CAP_POE_PD_PLUS_CHECK(phy_cap)          ( PHY_CAP_FIELD_CHECK(phy_cap, POE_PD_PLUS))
#define  PHY_CAP_UPOE_PSE_CHECK(phy_cap)             ( PHY_CAP_FIELD_CHECK(phy_cap, UPOE_PSE))
#define  PHY_CAP_UPOE_PD_CHECK(phy_cap)              ( PHY_CAP_FIELD_CHECK(phy_cap, UPOE_PD))
#define  PHY_CAP_POE_BT_CHECK(phy_cap)               ( PHY_CAP_FIELD_CHECK(phy_cap, POE_BT))
#define  PHY_CAP_MACSEC_CHECK(phy_cap)               ( PHY_CAP_FIELD_CHECK(phy_cap, MACSEC))

#define  PHY_CAP_FIXED_STACK_PORT_CHECK(phy_cap)     ( PHY_CAP_FIELD_CHECK(phy_cap, FIXED_STACK_PORT))

#define PHY_CAP_NARGS_IMPL(  \
                           _1, _2, _3, _4, _5, _6, _7, _8, \
                           _9,_10,_11,_12,_13,_14,_15,_16, \
                          _17,_18,_19,_20,_21,_22,_23,_24, \
                          _25,_26,_27,_28,_29,_30,_31,_32, \
                          _33,_34,_35,_36,_37,_38,_39,_40, \
                          _41,_42,_43,_44,_45,_46,_47,_48, \
                          _49,_50,_51,_52,_53,_54,_55,_56, \
                          _57,_58,_59,_60,_61,_62,_63,_64, N, ...) N



#define PHY_CAP_NARGS(...) PHY_CAP_NARGS_IMPL(__VA_ARGS__, \
                                              64,63,62,61,60,59,58,57, \
                                              56,55,54,53,52,51,50,49, \
                                              48,47,46,45,44,43,42,41, \
                                              40,39,38,37,36,35,34,33, \
                                              32,31,30,29,28,27,26,25, \
                                              24,23,22,21,20,19,18,17, \
                                              16,15,14,13,12,11,10, 9, \
                                               8, 7, 6, 5, 4, 3, 2, 1)

#define  PHY_CAP_OR_IMPL2(count, ...)  PHY_CAP_OR ## count (__VA_ARGS__)
#define  PHY_CAP_OR_IMPL(count, ...)  PHY_CAP_OR_IMPL2(count, __VA_ARGS__)
#define  PHY_CAP_OR(...) ( PHY_CAP_OR_IMPL(PHY_CAP_NARGS(__VA_ARGS__), __VA_ARGS__))


#define  PHY_CAP(A)                                               PHY_CAP_##A
#define  PHY_CAP_OR1(A)                                           PHY_CAP(A)
#define  PHY_CAP_OR2(A,B)                                         PHY_CAP_OR1(A)| PHY_CAP_OR1(B)
#define  PHY_CAP_OR3(A,B,C)                                       PHY_CAP_OR2(A,B)| PHY_CAP_OR1(C)
#define  PHY_CAP_OR4(A,B,C,D)                                     PHY_CAP_OR2(A,B)| PHY_CAP_OR2(C,D)
#define  PHY_CAP_OR5(A,B,C,D,E)                                   PHY_CAP_OR2(A,B)| PHY_CAP_OR3(C,D,E)
#define  PHY_CAP_OR6(A,B,C,D,E,F)                                 PHY_CAP_OR2(A,B)| PHY_CAP_OR4(C,D,E,F)
#define  PHY_CAP_OR7(A,B,C,D,E,F,G)                               PHY_CAP_OR2(A,B)| PHY_CAP_OR5(C,D,E,F,G)
#define  PHY_CAP_OR8(A,B,C,D,E,F,G,H)                             PHY_CAP_OR2(A,B)| PHY_CAP_OR6(C,D,E,F,G,H)
#define  PHY_CAP_OR9(A,B,C,D,E,F,G,H,I)                           PHY_CAP_OR2(A,B)| PHY_CAP_OR7(C,D,E,F,G,H,I)
#define  PHY_CAP_OR10(A,B,C,D,E,F,G,H,I,J)                        PHY_CAP_OR2(A,B)| PHY_CAP_OR8(C,D,E,F,G,H,I,J)
#define  PHY_CAP_OR11(A,B,C,D,E,F,G,H,I,J,K)                      PHY_CAP_OR2(A,B)| PHY_CAP_OR9(C,D,E,F,G,H,I,J,K)
#define  PHY_CAP_OR12(A,B,C,D,E,F,G,H,I,J,K,L)                    PHY_CAP_OR2(A,B)| PHY_CAP_OR10(C,D,E,F,G,H,I,J,K,L)
#define  PHY_CAP_OR13(A,B,C,D,E,F,G,H,I,J,K,L,M)                  PHY_CAP_OR2(A,B)| PHY_CAP_OR11(C,D,E,F,G,H,I,J,K,L,M)
#define  PHY_CAP_OR14(A,B,C,D,E,F,G,H,I,J,K,L,M,N)                PHY_CAP_OR2(A,B)| PHY_CAP_OR12(C,D,E,F,G,H,I,J,K,L,M,N)
#define  PHY_CAP_OR15(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O)              PHY_CAP_OR2(A,B)| PHY_CAP_OR13(C,D,E,F,G,H,I,J,K,L,M,N,O)
#define  PHY_CAP_OR16(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P)            PHY_CAP_OR2(A,B)| PHY_CAP_OR14(C,D,E,F,G,H,I,J,K,L,M,N,O,P)
#define  PHY_CAP_OR17(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q)          PHY_CAP_OR2(A,B)| PHY_CAP_OR15(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q)
#define  PHY_CAP_OR18(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R)        PHY_CAP_OR2(A,B)| PHY_CAP_OR16(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R)
#define  PHY_CAP_OR19(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S)      PHY_CAP_OR2(A,B)| PHY_CAP_OR17(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S)
#define  PHY_CAP_OR20(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T)    PHY_CAP_OR2(A,B)| PHY_CAP_OR18(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T)
#define  PHY_CAP_OR21(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U)  PHY_CAP_OR2(A,B)| PHY_CAP_OR19(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U)
#define  PHY_CAP_OR22(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V)                        \
                         PHY_CAP_OR2(A,B)| PHY_CAP_OR20(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V)

#define  PHY_CAP_OR23(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W)                        \
                         PHY_CAP_OR2(A,B)| PHY_CAP_OR21(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W)

#define  PHY_CAP_OR24(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X)                        \
                         PHY_CAP_OR2(A,B)| PHY_CAP_OR22(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X)

#define  PHY_CAP_OR25(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y)                        \
                         PHY_CAP_OR2(A,B)| PHY_CAP_OR23(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y)

#define  PHY_CAP_OR26(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z)                        \
                         PHY_CAP_OR2(A,B)| PHY_CAP_OR24(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z)

#define  PHY_CAP_OR27(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,A1)                        \
                         PHY_CAP_OR2(A,B)| PHY_CAP_OR25(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,A1)

#define  PHY_CAP_OR28(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,A1,A2)                        \
                         PHY_CAP_OR2(A,B)| PHY_CAP_OR26(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,A1,A2)

#define  PHY_CAP_OR29(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,A1,A2,A3)                        \
                         PHY_CAP_OR2(A,B)| PHY_CAP_OR27(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,A1,A2,A3)

#define  PHY_CAP_OR30(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,A1,A2,A3,A4)                        \
                         PHY_CAP_OR2(A,B)| PHY_CAP_OR28(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,A1,A2,A3,A4)

#define  PHY_CAP_OR31(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,A1,A2,A3,A4,A5)                        \
                         PHY_CAP_OR2(A,B)| PHY_CAP_OR29(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,A1,A2,A3,A4,A5)

#define  PHY_CAP_OR32(A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,A1,A2,A3,A4,A5,A6)                        \
                         PHY_CAP_OR2(A,B)| PHY_CAP_OR30(C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,A1,A2,A3,A4,A5,A6)

/* NOTE: Add  PHY_CAP_OR33 to  PHY_CAP_OR63 as per need */

#define  PHY_CAP_PORTSPEED_10G_AND_LOWER_1            PHY_CAP_OR(          \
                                                        PORTSPEED_HALF_10   ,  \
                                                        PORTSPEED_FULL_10   ,  \
                                                        PORTSPEED_HALF_100  ,  \
                                                        PORTSPEED_FULL_100  ,  \
                                                        PORTSPEED_FULL_1000 ,  \
                                                        PORTSPEED_FULL_10G)

#define  PHY_CAP_PORTSPEED_10G_AND_LOWER_2            PHY_CAP_OR(         \
                                                        PORTSPEED_HALF_10   ,  \
                                                        PORTSPEED_FULL_10   ,  \
                                                        PORTSPEED_HALF_100  ,  \
                                                        PORTSPEED_FULL_100  ,  \
                                                        PORTSPEED_HALF_1000 ,  \
                                                        PORTSPEED_FULL_1000 ,  \
                                                        PORTSPEED_FULL_10G)

#define  PHY_CAP_PORTSPEED_10G_AND_LOWER              PHY_CAP_OR(          \
                                                        PORTSPEED_HALF_10   ,  \
                                                        PORTSPEED_FULL_10   ,  \
                                                        PORTSPEED_HALF_100  ,  \
                                                        PORTSPEED_FULL_100  ,  \
                                                        PORTSPEED_HALF_1000 ,  \
                                                        PORTSPEED_FULL_1000 ,  \
                                                        PORTSPEED_FULL_2P5G ,  \
                                                        PORTSPEED_FULL_5G   ,  \
                                                        PORTSPEED_FULL_10G  ,  \
                                                        PORTSPEED_AUTO_NEG)
/* Autoneg supported port speeds */
#define  PHY_CAP_AN_PORT_SPEEDS                       PHY_CAP_OR(          \
                                                        PORTSPEED_HALF_10   ,  \
                                                        PORTSPEED_FULL_10   ,  \
                                                        PORTSPEED_HALF_100  ,  \
                                                        PORTSPEED_FULL_100  ,  \
                                                        PORTSPEED_HALF_1000 ,  \
                                                        PORTSPEED_FULL_1000 ,  \
                                                        PORTSPEED_FULL_2P5G ,  \
                                                        PORTSPEED_FULL_5G   ,  \
                                                        PORTSPEED_FULL_10G  ,  \
                                                        PORTSPEED_FULL_25G  ,  \
                                                        PORTSPEED_FULL_50G  ,  \
                                                        PORTSPEED_FULL_100G ,  \
                                                        PORTSPEED_AUTO_NEG)

/* Autoneg supported port speeds + EEE */
#define  PHY_CAP_AN_PORT_SPEEDS_PLUS_EEE              PHY_CAP_OR(          \
                                                        PORTSPEED_HALF_10   ,  \
                                                        PORTSPEED_FULL_10   ,  \
                                                        PORTSPEED_HALF_100  ,  \
                                                        PORTSPEED_FULL_100  ,  \
                                                        PORTSPEED_HALF_1000 ,  \
                                                        PORTSPEED_FULL_1000 ,  \
                                                        PORTSPEED_FULL_2P5G ,  \
                                                        PORTSPEED_FULL_5G   ,  \
                                                        PORTSPEED_FULL_10G  ,  \
                                                        PORTSPEED_FULL_25G  ,  \
                                                        PORTSPEED_FULL_50G  ,  \
                                                        PORTSPEED_FULL_100G ,  \
                                                        PORTSPEED_AUTO_NEG  ,  \
                                                        AUTO_EEE)

#define  PHY_CAP_PORTSPEED_40G_AND_LOWER              PHY_CAP_OR(          \
                                                        PORTSPEED_HALF_10   ,  \
                                                        PORTSPEED_FULL_10   ,  \
                                                        PORTSPEED_HALF_100  ,  \
                                                        PORTSPEED_FULL_100  ,  \
                                                        PORTSPEED_HALF_1000 ,  \
                                                        PORTSPEED_FULL_1000 ,  \
                                                        PORTSPEED_FULL_2P5G ,  \
                                                        PORTSPEED_FULL_5G   ,  \
                                                        PORTSPEED_FULL_10G  ,  \
                                                        PORTSPEED_FULL_40G)

#define  PHY_CAP_PORTSPEED_ALL                        PHY_CAP_OR(          \
                                                        PORTSPEED_HALF_10   ,  \
                                                        PORTSPEED_FULL_10   ,  \
                                                        PORTSPEED_HALF_100  ,  \
                                                        PORTSPEED_FULL_100  ,  \
                                                        PORTSPEED_HALF_1000 ,  \
                                                        PORTSPEED_FULL_1000 ,  \
                                                        PORTSPEED_FULL_2P5G ,  \
                                                        PORTSPEED_FULL_5G   ,  \
                                                        PORTSPEED_FULL_10G  ,  \
                                                        PORTSPEED_FULL_20G  ,  \
                                                        PORTSPEED_FULL_25G  ,  \
                                                        PORTSPEED_FULL_40G  ,  \
                                                        PORTSPEED_FULL_50G  ,  \
                                                        PORTSPEED_FULL_100G ,  \
                                                        PORTSPEED_FULL_200G ,  \
                                                        PORTSPEED_FULL_400G ,  \
                                                        PORTSPEED_AUTO_NEG)

#define  PHY_CAP_PORTSPEED_ALL_NO_AUTO_NEG            PHY_CAP_OR(          \
                                                        PORTSPEED_HALF_10   ,  \
                                                        PORTSPEED_FULL_10   ,  \
                                                        PORTSPEED_HALF_100  ,  \
                                                        PORTSPEED_FULL_100  ,  \
                                                        PORTSPEED_HALF_1000 ,  \
                                                        PORTSPEED_FULL_1000 ,  \
                                                        PORTSPEED_FULL_2P5G ,  \
                                                        PORTSPEED_FULL_5G   ,  \
                                                        PORTSPEED_FULL_10G  ,  \
                                                        PORTSPEED_FULL_20G  ,  \
                                                        PORTSPEED_FULL_25G  ,  \
                                                        PORTSPEED_FULL_40G  ,  \
                                                        PORTSPEED_FULL_50G  ,  \
                                                        PORTSPEED_FULL_100G ,  \
                                                        PORTSPEED_FULL_200G ,  \
                                                        PORTSPEED_FULL_400G)



/* bit masks for auto-negotiation capabilities. */
typedef enum
{
   PORT_NEGO_CAPABILITY_HALF_10   = (1 << 0),
   PORT_NEGO_CAPABILITY_FULL_10   = (1 << 1),
   PORT_NEGO_CAPABILITY_HALF_100  = (1 << 2),
   PORT_NEGO_CAPABILITY_FULL_100  = (1 << 3),
   PORT_NEGO_CAPABILITY_FULL_1000 = (1 << 4),
   PORT_NEGO_CAPABILITY_FULL_2P5G = (1 << 11),
   PORT_NEGO_CAPABILITY_FULL_5G   = (1 << 13),
   PORT_NEGO_CAPABILITY_FULL_10G  = (1 << 5),
   PORT_NEGO_CAPABILITY_FULL_20G  = (1 << 6),
   PORT_NEGO_CAPABILITY_FULL_40G  = (1 << 7),
   PORT_NEGO_CAPABILITY_FULL_25G  = (1 << 8),
   PORT_NEGO_CAPABILITY_FULL_100G = (1 << 9),
   PORT_NEGO_CAPABILITY_FULL_50G  = (1 << 12),
   PORT_NEGO_CAPABILITY_FULL_200G = (1 << 14),
   PORT_NEGO_CAPABILITY_FULL_400G = (1 << 15),
   PORT_NEGO_CAPABILITY_ALL       = (1 << 10)
}  PORT_NEGO_CAPABILITIES_t;

#if 0
typedef enum
{
   CONNECTOR_NONE = 0,
   RJ45,
   MTRJ,
   SCDUP,
   XAUI,
   CAUI
}  CONNECTOR_TYPES_t;
#endif
typedef enum
{
   PHYCLOCK_AUTO    = 0x1,
   PHYCLOCK_MASTER  = 0x2,
   PHYCLOCK_SLAVE   = 0x4,
}  PHY_PROPERTIES_t;


#define  PORT_ENET_ENCAP_MIN_MTU    576
#define  PORT_ENET_ENCAP_MAX_MTU   1500
#define  PORT_ENET_ENCAP_DEF_MTU   1500
#define  PORT_SNAP_ENCAP_MAX_MTU   1492

#if 0
typedef enum
{
   CABLE_UNTESTED,
   CABLE_TEST_FAIL,    /* Test failed for unknown reason */
   CABLE_NORMAL,      /* Cable is OK */
   CABLE_OPEN,         /* Cable is not connected on one end */
   CABLE_SHORT,        /* Cable is shorted */
   CABLE_OPENSHORT,
   CABLE_CROSSTALK,
   CABLE_NOCABLE

}  CABLE_STATUS_t;

typedef enum
{
  PORT_MEDIUM_COPPER = 1,
  PORT_MEDIUM_FIBER,
}  PORT_CABLE_MEDIUM_t;

typedef enum
{
  PORT_FAULT_NONE = 0,
  PORT_FAULT_LOCAL = 0x0001,
  PORT_FAULT_REMOTE = 0x0002,
}  PORT_FAULT_STATUS_t;
#endif

typedef enum
{
   STATUS_UP=1,
   STATUS_DOWN=2,
   STATUS_TESTING=3,
   STATUS_DETACH
}  OPER_STATUS_t;


/* Loopback Test */
typedef enum
{
   PORT_LOOPBACK_NONE = 0,
   PORT_LOOPBACK_MAC,
   PORT_LOOPBACK_PHY,
   PORT_LOOPBACK_COUNT
}  PORT_LOOPBACK_t;


typedef enum
{
   RATE_UNIT_NONE = 0,
   RATE_UNIT_PERCENT,                 /* rate is percentage (of link speed) */
   RATE_UNIT_KBPS,                    /* rate is specified in kbps */
   RATE_UNIT_PPS,                     /* rate is specified in pps */
   RATE_UNIT_TOTAL                    /* total number of unit types defined - this must be last */
}  RATE_UNIT_t;

#define  RPCAP_REMOTE_DEVICE 1
#define  RPCAP_LOCAL_DEVICE 0

typedef enum
{
   POLICY_PORT_ACTION_NONE = 0,
   POLICY_PORT_ACTION_SHUTDOWN,       /* shutdown port if storm */
   POLICY_PORT_ACTION_RATELIMIT,      /* rate limit port if storm */
   POLICY_PORT_ACTION_TRAP            /* trap if storm */
}  POLICY_PORT_ACTION_t;

typedef enum
{
   VRF_SET_ERROR_INVALID = 0,
   LOOPBACK_ERROR = 1,
   UNNUMBERED_ERROR,
   VRF_DHCP_ERROR,
   SYSLOG_ERROR,
   IPV6_ERROR
} VRF_SET_ERROR_t; 

#define  FDB_KEY_STR_LEN  (( FDB_KEY_SIZE*2)+( FDB_KEY_SIZE-1)) /* vlan id and the mac address string length */


/* 
===================================
   Port Mode for Expandable Ports. 
====================================

  A single root expandable port can be in different port modes where
  each mode (except single modes below) indicates a different
  speed. Any of the single modes below are the root port mode when no
  split is done.

  There can only be 1 port mode value programmed at a time. But we are
  still using enums 1, 2, 3 instead of bit shift since there might be
  > 8 splits supported in future.

*/
#define  EXP_MODE_SINGLE_10G      1   /* Single lane ports 10G */
#define  EXP_MODE_SINGLE_25G      2   /* Single lane ports 25G */
#define  EXP_MODE_DUAL_50G        3   /* Dual lane ports 50G */
#define  EXP_MODE_QUAD_40G        4   /* Quad lane 40g ports */
#define  EXP_MODE_QUAD_100G       5   /* Quad lane 100g ports */
#define  EXP_MODE_ROOT_DUAL_50G   6   /* Single 50GB port with dual lane*/
#define  EXP_MODE_DUAL_25G        7   /* Dual 25GB ports */
#define  EXP_MODE_8x10G           8   /* single lane 10G ports */
#define  EXP_MODE_8x25G           9   /* single lane 25G ports */
#define  EXP_MODE_8x50G          10   /* single lane 50G ports */
#define  EXP_MODE_4x100G         11   /* Quad lane 100g ports */
#define  EXP_MODE_2x100G         12   /* dual lane 100g ports */
#define  EXP_MODE_2x200G         13   /* Quad lane 200g ports */
#define  EXP_MODE_1x400G         14   /* OCTA lane 400g ports */
typedef enum
{
   PORT_PORTMODE_NONE           =    0,
   PORT_PORTMODE_SINGLE_10G     =     EXP_MODE_SINGLE_10G,
   PORT_PORTMODE_SINGLE_25G     =     EXP_MODE_SINGLE_25G,
   PORT_PORTMODE_DUAL_50G       =     EXP_MODE_DUAL_50G,
   PORT_PORTMODE_QUAD_40G       =     EXP_MODE_QUAD_40G,
   PORT_PORTMODE_QUAD_100G      =     EXP_MODE_QUAD_100G,
   PORT_PORTMODE_ROOT_DUAL_50G  =     EXP_MODE_ROOT_DUAL_50G,
   PORT_PORTMODE_DUAL_25G       =     EXP_MODE_DUAL_25G,
   PORT_PORTMODE_8x10G          =     EXP_MODE_8x10G,
   PORT_PORTMODE_8x25G          =     EXP_MODE_8x25G,
   PORT_PORTMODE_8x50G          =     EXP_MODE_8x50G,
   PORT_PORTMODE_4x100G         =     EXP_MODE_4x100G,
   PORT_PORTMODE_2x100G         =     EXP_MODE_2x100G,
   PORT_PORTMODE_2x200G         =     EXP_MODE_2x200G,
   PORT_PORTMODE_1x400G         =     EXP_MODE_1x400G,
  /*Add new modes here */
   PORT_PORTMODE_LAST,
   PORT_PORTMODE_INVALID = 0x80            /* No more than 1 byte */
}  portmode_t;

/* FEC types supported */
typedef enum
{
   PORT_FEC_DISABLE     =   DISABLE, /* FEC disabled */
   PORT_FEC_ENABLE      =   ENABLE, /* FEC enabled: Backward compatibility */
   PORT_FEC_AUTO        , /* for AN cases */
   PORT_FEC_BASE_R      , /* CL74/Base-R. 64/66b KR FEC for fabric */
   PORT_FEC_RS_FEC      , /* CL91/RS-FEC */
   PORT_FEC_RS_544      , /* Rs544, using 1xN RS FEC architecture */
   PORT_FEC_RS_272      , /* Rs272, using 1xN RS FEC architecture */
   PORT_FEC_RS_206      , /* Rs206. 64/66b 5T RS FEC for fabric   */
   PORT_FEC_RS_108      , /* Rs108. 64/66b 5T low latency RS FEC for fabric */
   PORT_FEC_RS_545      , /* Rs545. 64/66b 15T RS FEC for fabric */
   PORT_FEC_RS_304      , /* Rs304. 64/66b 15T low latency RS FEC for fabric */
   PORT_FEC_RS_544_2xN  , /* Rs544, using 2xN RS FEC architecture */
   PORT_FEC_RS_272_2xN  , /* Rs272, using 2xN RS FEC architecture */
   PORT_FEC_NOFEC_50G   , /* support FEC disable on 50G port */
  /* Add the new types here */
   PORT_FEC_LAST        ,
   PORT_FEC_SET_DEFAULT = 0x80      /* Always As Last, unlike others this is bitmaks */

}  fec_mode_t;

typedef enum
{
   CAP_FEC_NONE        =  0,
   CAP_FEC_DISABLE     = (1 << ( PORT_FEC_DISABLE)),
   CAP_FEC_ENABLE      = (1 << ( PORT_FEC_ENABLE)),
   CAP_FEC_AUTO        = (1 << ( PORT_FEC_AUTO  )),
   CAP_FEC_BASE_R      = (1 << ( PORT_FEC_BASE_R)),
   CAP_FEC_RS_FEC      = (1 << ( PORT_FEC_RS_FEC)),
   CAP_FEC_RS_544      = (1 << ( PORT_FEC_RS_544)),
   CAP_FEC_RS_272      = (1 << ( PORT_FEC_RS_272)),
   CAP_FEC_RS_206      = (1 << ( PORT_FEC_RS_206)),
   CAP_FEC_RS_108      = (1 << ( PORT_FEC_RS_108)),
   CAP_FEC_RS_545      = (1 << ( PORT_FEC_RS_545)),
   CAP_FEC_RS_304      = (1 << ( PORT_FEC_RS_304)),
   CAP_FEC_RS_544_2xN  = (1 << ( PORT_FEC_RS_544_2xN)),
   CAP_FEC_RS_272_2xN  = (1 << ( PORT_FEC_RS_272_2xN)),
   CAP_FEC_NOFEC_50G   = (1 << ( PORT_FEC_NOFEC_50G)),

}  FEC_CAPABILITY_t;

#define  FEC_CAP_TYPE_CHECK(fec_cap, __type)            \
  (((fec_cap) & ( CAP_FEC_##__type)) == ( CAP_FEC_##__type))

#define  FEC_CAP_DISABLE_CHECK(fec_cap)  FEC_CAP_TYPE_CHECK(fec_cap, DISABLE)
#define  FEC_CAP_ENABLE_CHECK(fec_cap)  FEC_CAP_TYPE_CHECK(fec_cap, ENABLE)
#define  FEC_CAP_CL74_CHECK(fec_cap)  FEC_CAP_TYPE_CHECK(fec_cap, BASE_R)
#define  FEC_CAP_CL91_CHECK(fec_cap)  FEC_CAP_TYPE_CHECK(fec_cap, RS_FEC)
#define  FEC_CAP_RS544_CHECK(fec_cap)  FEC_CAP_TYPE_CHECK(fec_cap, RS_544)
#define  FEC_CAP_RS272_CHECK(fec_cap)  FEC_CAP_TYPE_CHECK(fec_cap, RS_272)
#define  FEC_CAP_RS206_CHECK(fec_cap)  FEC_CAP_TYPE_CHECK(fec_cap, RS_206)
#define  FEC_CAP_RS108_CHECK(fec_cap)  FEC_CAP_TYPE_CHECK(fec_cap, RS_108)
#define  FEC_CAP_RS545_CHECK(fec_cap)  FEC_CAP_TYPE_CHECK(fec_cap, RS_545)
#define  FEC_CAP_RS304_CHECK(fec_cap)  FEC_CAP_TYPE_CHECK(fec_cap, RS_304)
#define  FEC_CAP_RS544_2xN_CHECK(fec_cap)  FEC_CAP_TYPE_CHECK(fec_cap, RS_544_2xN)
#define  FEC_CAP_RS272_2xN_CHECK(fec_cap)  FEC_CAP_TYPE_CHECK(fec_cap, RS_272_2xN)
#define  FEC_CAP_NOFEC_50G_CHECK(fec_cap)  FEC_CAP_TYPE_CHECK(fec_cap, NOFEC_50G)

typedef enum
{
   PORT_AUTO_NEG_HALF = 1,
   PORT_AUTO_NEG_FULL,
   PORT_AUTO_NEG_BOTH
}  portAutoNegModes_t;

#define  PORT_NEGO_FULL_CAPABILITIES     (  PORT_NEGO_CAPABILITY_FULL_10   | \
                                              PORT_NEGO_CAPABILITY_FULL_100  | \
                                              PORT_NEGO_CAPABILITY_FULL_1000 | \
                                              PORT_NEGO_CAPABILITY_FULL_2P5G | \
                                              PORT_NEGO_CAPABILITY_FULL_5G   | \
                                              PORT_NEGO_CAPABILITY_FULL_10G  | \
                                              PORT_NEGO_CAPABILITY_FULL_20G  | \
                                              PORT_NEGO_CAPABILITY_FULL_25G  | \
                                              PORT_NEGO_CAPABILITY_FULL_40G  | \
                                              PORT_NEGO_CAPABILITY_FULL_50G  | \
                                              PORT_NEGO_CAPABILITY_FULL_100G )
#endif /* INCLUDE_COMMDEFS */
