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


#ifndef INCLUDE_AUTH_MGR_COMMON
#define INCLUDE_AUTH_MGR_COMMON

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "commdefs.h"
#include "datatypes.h"
#include "resources.h"
#include "component_mask.h"
#include "log.h"
#include "packet.h"
#include "cpustats_api.h"

#define SYSAPI_PRINTF printf
#define __FP_FILE__ __FILE__

#define  AUTHMGR_INTF_MAX_COUNT ( MAX_PORT_COUNT + 1)

#define  IP6_LEN           40
#define  IP4_STR_LEN       20
#define  IP6_ADDR_LEN      16

#define  DOT1Q_NULL_VLAN_ID     0
#define  DOT1Q_MIN_VLAN_ID      1
#define  DOT1Q_MAX_VLAN_ID      4094 // - RESERVED VLANs if any
#define  VLAN_MAX_MASK_BIT       DOT1Q_MAX_VLAN_ID

#define  CLI_MAX_STRING_LENGTH  256

#define  ETYPE_EAPOL    0x888E

/* EAPOL Packet types */
#define EAPOL_EAPPKT      0  /* 0000 0000B */
#define EAPOL_START       1  /* 0000 0001B */
#define EAPOL_LOGOFF      2  /* 0000 0010B */
#define EAPOL_KEY         3  /* 0000 0011B */
#define EAPOL_ENCASFALERT 4  /* 0000 0100B */

/* EAPOL packet header */
typedef struct  eapolPacket_s
{
   uchar8   protocolVersion;
   uchar8   packetType;
   ushort16 packetBodyLength;
}  eapolPacket_t;

/* Length defines for EAPOL-Key frame */
#define EAPOL_KEY_REPLAY_COUNTER_LEN    8
#define EAPOL_KEY_IV_LEN                16
#define EAPOL_KEY_SIGNATURE_LEN         16

/* Bitmask defines for keyIndex field in EAPOL-Key frame */
#define EAPOL_KEY_UNICAST  0x80  /* first bit */
#define EAPOL_KEY_INDEX    0x7F  /* last 7 bits */

/* EAPOL-Key packet format */
typedef struct  eapolKeyPacket_s
{
   uchar8   descriptorType;
   ushort16 keyLength;
   uchar8   replayCounter[EAPOL_KEY_REPLAY_COUNTER_LEN];
   uchar8   keyIV[EAPOL_KEY_IV_LEN];
   uchar8   keyIndex;
   uchar8   keySignature[EAPOL_KEY_SIGNATURE_LEN];
}  eapolKeyPacket_t;

/* EAP Packet code types */
#define EAP_REQUEST   1
#define EAP_RESPONSE  2
#define EAP_SUCCESS   3
#define EAP_FAILURE   4

/* EAP packet header */
typedef struct  authmgrEapPacket_s
{
   uchar8   code;
   uchar8   id;
   ushort16 length;
}  authmgrEapPacket_t;

/* Defines for 'type' field of EAP Request and Response frames */
#define EAP_RRIDENTITY  1  /* Request/Identity or Response/Identity */
#define EAP_RRNOTIF     2  /* Notification */
#define EAP_RRNAK       3  /* NAK (Response only) */
#define EAP_RRMD5       4  /* MD5-Challenge */
#define EAP_RROTP       5  /* One-Time Password */
#define EAP_RRGTK       6  /* Generic Token Card */
#define EAP_TLS         13 /* EAP/TLS */
#define EAP_TTLS        21 /* EAP/TTLS */
#define EAP_PEAP        25 /* EAP/PEAP */

/* EAP Request/Response packet header */
typedef struct  eapRrPacket_s
{
   uchar8   type;
}  eapRrPacket_t;

/* Ethernet Encapsulation Overlay */
typedef struct  enet_encaps_s
{
   ushort16       type;   /* ether type */
} enet_encaps_t;

#define  ENET_ENCAPS_HDR_SIZE    (uint32)sizeof( enet_encaps_t)

/* Ethernet MAC Address */

#define  ENET_HDR_SIZE    (uint32)sizeof( enetHeader_t)
#define  ETH_HDR_SIZE    (uint32)sizeof( ethHeader_t)

typedef enum
{
   AUTH_METHOD_UNDEFINED = 0,
   AUTH_METHOD_LOCAL,
   AUTH_METHOD_NONE,
   AUTH_METHOD_RADIUS,
   AUTH_METHOD_REJECT
}  USER_MGR_AUTH_METHOD_t;

#define  MAX_AUTH_METHODS 4

#define  MAX_FRAME_SIZE   3000

#define  VLAN_INDICES   ((4095) / (sizeof( uchar8) * 8) + 1)

typedef enum dot1qTaggingMode_s
{
   DOT1Q_MEMBER_UNTAGGED = 0,
   DOT1Q_MEMBER_TAGGED = 1
} dot1qTaggingMode_t;

typedef struct
{
   uchar8   value[ VLAN_INDICES];
}  VLAN_MASK_t;

#define  VLAN_STRING_SIZE   (32 + 1)

/* VLAN Notification Structure */
typedef struct dot1qNotifyData_s
{
//    uint32 numVlans; /* If num Vlan is 1 use vlanId member of the union, else use vlanMask of the union*/
//     VLAN_MASK_t vlanTagModeMask; /* If set, VLAN member is tagged else untagged. */
     BOOL tagged; /* VLAN member is tagged else untagged. */
    union
    {
        uint32 vlanId;
         char8 vlanString[ VLAN_STRING_SIZE];
//         VLAN_MASK_t vlanMask;
    }data;
}dot1qNotifyData_t;

typedef struct sysnet_pdu_info_s
{
  /* Originally, this structure contained only intIfNum and vlanId on the
   * receive side. Unfortunately, the meaning of intIfNum depends on context. 
   * intIfNum is originally set to the ingress physical port. 
   * Once IP MAP processes an incoming packet, it resets intIfNum to 
   * the logical (e.g., VLAN routing interface) ingress interface. See ipMapPduRcv().
   * All the software forwarding code and sysnet interceptors depend on 
   * this behavior. DHCP relay needs to know the ingress physical port to
   * set option 82 correctly. So we add a new member, rxPort, to this
   * structure as a place to put this. If we were starting over, intIfNum
   * would always have the same meaning and we'd add a field for the 
   * logical ingress interface, but I'm afraid we're stuck with 
   * a bunch of code that depends on the current behavior. NOTE:  rxPort is 
   * only set in IP MAP. So if packet hasn't been handled by IP MAP, you can't
   * use rxPort. */
  uint32 intIfNum;          /* Receiving intIfNum */
  uint32 rxPort;            /* Physical port where packet was received */
  uint32 vlanId;            /* Receiving VLAN */
  uint32 innerVlanId;        /* Receiving inner VLAN if the packet is double tagged.*/

  uint32 destIntIfNum;      /* Destination intIfNum */
  uint32 destVlanId;        /* Destination VLAN */

  uint32 vrfId;             /* NOTE: Identifier of the VRF that this PDU is received on
                                * and this field makes sense only when 'intIfNum' is not set.
                                * If 'intIfNum' is set, this field can be derived from that
                                * receiving interface number and 'vrfId' would be redundant. */
} sysnet_pdu_info_t;

typedef enum
{
  SYSNET_PDU_RC_DISCARD = 0,  /* Discard this frame */
  SYSNET_PDU_RC_CONSUMED,     /* Frame has been consumed by interceptor */
  SYSNET_PDU_RC_COPIED,       /* Frame has been copied by interceptor */
  SYSNET_PDU_RC_IGNORED,      /* Frame ignored by interceptor */
  SYSNET_PDU_RC_PROCESSED,    /* Frame has been processed by interceptor */
  SYSNET_PDU_RC_MODIFIED,     /* Frame has been modified by interceptor */
  SYSNET_PDU_RC_LAST
} SYSNET_PDU_RC_t;

/* TLV handle type */
typedef uint32    tlvHandle_t;

typedef struct
{
  uint32     type;                           /* TLV type identifier        */
  uint32     length;                         /* TLV length of value field  */
   uchar8     valueStart[1];                  /* start of TLV value field   */
}  tlv_t;


/* VLAN outcall notification events */
typedef enum
{
  VLAN_ADD_NOTIFY = 0x00000001,     /* Create a new VLAN */
  VLAN_DELETE_PENDING_NOTIFY = 0x00000002, /* Vlan is about to be deleted */
  VLAN_DELETE_NOTIFY = 0x00000004,      /* Delete a VLAN */
  VLAN_ADD_PORT_NOTIFY = 0x00000008,    /* Add a port to a VLAN */
  VLAN_DELETE_PORT_NOTIFY = 0x00000010,  /* Delete a port from a VLAN */
  VLAN_START_TAGGING_PORT_NOTIFY = 0x00000020,  /* Start tagging on a port */
  VLAN_STOP_TAGGING_PORT_NOTIFY = 0x00000040,   /* Stop tagging on a port */
  VLAN_INITIALIZED_NOTIFY = 0x00000080,
  VLAN_RESTORE_NOTIFY = 0x00000100,
  VLAN_PVID_CHANGE_NOTIFY = 0x00000200,        /* PVID change on a port*/
  VLAN_DOT1P_PRIORITY_CHANGE_NOTIFY = 0x00000400,        /* dot1p priority change on port*/
  VLAN_DYNAMIC_TO_STATIC_NOTIFY     = 0x00000800, /* Dynamic to static convert notification */
  VLAN_INGRESS_FILTER_PORT_NOTIFY   = 0x00001000, /* Ingress filter notification on port */
  VLAN_SWITCHPORT_MODE_CHANGE_NOTIFY = 0x00002000,  /* Switchport mode change on port notification */
  VLAN_AUTO_TRUNK_CHANGE_NOTIFY   = 0x00004000, /* Change in auto-trunk configuration os a port */
  VLAN_LAST_NOTIFY                = 0x00004000  /* Any time we add an event adjust this be the last in the series */
} vlanNotifyEvent_t;


/*
** The Termination Action value codes
*/
#define RADIUS_TERMINATION_ACTION_DEFAULT  0
#define RADIUS_TERMINATION_ACTION_RADIUS   1
#define RADIUS_ACCT_TERM_CAUSE_REAUTHENTICATION_FAILURE       20


#define RADIUS_VENDOR_9_VOICE 1<<0
#define RADIUS_VENDOR_9_DACL 1<<1
#define RADIUS_VENDOR_9_SWITCH 1<<2
#define RADIUS_VENDOR_9_REDIRECT_URL 1<<3
#define RADIUS_VENDOR_9_REDIRECT_ACL 1<<4
#define RADIUS_VENDOR_9_ACS_SEC_DACL 1<<5
#define RADIUS_VENDOR_9_LINKSEC_POLICY 1<<6

#define RADIUS_VENDOR_311_MS_MPPE_SEND_KEY 1<<0
#define RADIUS_VENDOR_311_MS_MPPE_RECV_KEY 1<<1

/* The type of attribute values for Tunnel Medium type attribute
*/
#define RADIUS_TUNNEL_MEDIUM_TYPE_802    6

#define RADIUS_ATTR_TYPE_TUNNEL_TYPE_SPECIFIED              0x1
#define RADIUS_ATTR_TYPE_TUNNEL_MEDIUM_TYPE_SPECIFIED       0x2
#define RADIUS_ATTR_TYPE_TUNNEL_PRIVATE_GROUP_ID_SPECIFIED  0x4
#define RADIUS_REQUIRED_TUNNEL_ATTRIBUTES_SPECIFIED         0x7

/* Downloadable ACL Fields */
#define RADIUS_TLV_HEADER_LENGTH             2
#define RADIUS_VALUE_LENGTH             253

#define RADIUS_ATTR_SIZE_SERVICE_TYPE             6
/*
** The Service-Type value codes
*/
#define RADIUS_SERVICE_TYPE_LOGIN                 1
#define RADIUS_SERVICE_TYPE_FRAMED                2
#define RADIUS_SERVICE_TYPE_CALLBACK_LOGIN        3
#define RADIUS_SERVICE_TYPE_CALLBACK_FRAMED       4
#define RADIUS_SERVICE_TYPE_OUTBOUND              5
#define RADIUS_SERVICE_TYPE_ADMIN                 6
#define RADIUS_SERVICE_TYPE_NAS_PROMPT            7
#define RADIUS_SERVICE_TYPE_AUTHEN_ONLY           8
#define RADIUS_SERVICE_TYPE_CALLBACK_NAS_PROMPT   9
#define RADIUS_SERVICE_TYPE_CALL_CHECK            10


typedef struct radiusValuePair_s
{
   struct radiusValuePair_s *nextPair;
   uint32                attrId;
   uint32                attrType;
   uint32                vendorCode;
   uint32                vsAttrId;
   uint32                intValue;
    char8                 strValue[RADIUS_VALUE_LENGTH + 1];

} radiusValuePair_t;

typedef enum
{
   ACCT_METHOD_UNDEFINED = 0,
   ACCT_METHOD_TACACS,
   ACCT_METHOD_RADIUS,
   ACCT_METHOD_MAX
}  USER_MGR_ACCT_METHOD_t;

#define  MAX_AML_NAME_LEN         15 /*Maximum length of the Accounting Method List Name*/

/* Port PAE capabilities bitmask values */
#define  DOT1X_PAE_PORT_NONE_CAPABLE 0x00
#define  DOT1X_PAE_PORT_AUTH_CAPABLE 0x01
#define  DOT1X_PAE_PORT_SUPP_CAPABLE 0x02

#define  DOT1X_PAE_PORT_PROTOCOL_VERSION_2 2
#define  DOT1X_PAE_PORT_PROTOCOL_VERSION_3 3

#define RADIUS_ACCT_TERM_CAUSE_ADMIN_RESET 6

#define DS_ADMIN_MODE_NOTIFY     0x00000001
#define DSV6_ADMIN_MODE_NOTIFY   0x00000002

typedef enum
{
   FDB_ADDR_FLAG_STATIC = 0,
   FDB_ADDR_FLAG_LEARNED,
   FDB_ADDR_FLAG_MANAGEMENT,
   FDB_ADDR_FLAG_GMRP_LEARNED,
   FDB_ADDR_FLAG_SELF,
   FDB_ADDR_FLAG_AUTHMGR_STATIC,
   FDB_ADDR_FLAG_DOT1X_STATIC,
   FDB_ADDR_FLAG_DOT1AG_STATIC,
   FDB_ADDR_FLAG_ETH_CFM_STATIC,
   FDB_ADDR_FLAG_L3_MANAGEMENT, /* Routing Intf address */
   FDB_ADDR_FLAG_LEARNED_UNCONFIRMED, /* Address is learned, but not guaranteed
                                         * to be in HW (relevant for SW learning). */
   FDB_ADDR_FLAG_FIP_SNOOP_LEARNED, /* MAC added by FIP snooping */
   FDB_ADDR_FLAG_CAPTIVE_PORTAL_STATIC, /* CP client MAC Addr */
   FDB_ADDR_FLAG_Y1731_STATIC,
}  FDB_ADDR_FLAG_t;

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif
