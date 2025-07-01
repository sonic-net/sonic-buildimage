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


#ifndef __AUTHMGR_EXPORTS_H_
#define __AUTHMGR_EXPORTS_H_

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "auth_mgr_common.h"
#include "pacinfra_common.h"
#include "defaultconfig.h"

/* AUTHMGR Component Feature List */
typedef enum
{
   AUTHMGR_FEATURE_ID = 0,                   /* general support statement */
   AUTHMGR_VLANASSIGN_FEATURE_ID,           /* RFC 3580 VLAN Assignments via dot1x */
   AUTHMGR_DYNAMIC_VLANASSIGN_FEATURE_ID,           
   AUTHMGR_MAB_FEATURE_ID,
   AUTHMGR_DACL_FEATURE_ID,
   AUTHMGR_FEATURE_ID_TOTAL,                /* total number of enum values */
}  AUTHMGR_FEATURE_IDS_t;



#define  AUTHMGR_USER_NAME_LEN             65
#define  AUTHMGR_CHALLENGE_LEN             32

#define AUTHMGR_MAC_ADDR_STR_LEN (( MAC_ADDR_LEN * 2) + ( MAC_ADDR_LEN - 1))
#define AUTHMGR_SESSION_ID_LEN   AUTHMGR_USER_NAME_LEN * 2


/******************************************************************/
/*************       Start AUTHMGR types and defines        *********/
/******************************************************************/

#define AUTHMGR_USER_INDEX_INVALID    -1

#define AUTHMGR_SERVER_STATE_LEN 253
#define AUTHMGR_SERVER_CLASS_LEN 253


#define AUTHMGR_RADIUS_VLAN_ASSIGNED_LEN  32 /* Radius Assigned vlan length */

typedef enum
{
   AUTHMGR_TYPE_ORDER = 1,
   AUTHMGR_TYPE_PRIORITY,
}  AUTHMGR_METHOD_TYPE_t;

typedef enum
{
   AUTHMGR_METHOD_NONE = 0,
   AUTHMGR_METHOD_8021X,
   AUTHMGR_METHOD_MAB,
   AUTHMGR_METHOD_LAST
}  AUTHMGR_METHOD_t;

typedef enum
{
   AUTHMGR_NEW_CLIENT = 1, /* fdb entry is received */
   AUTHMGR_AUTH_FAIL,
   AUTHMGR_AUTH_SUCCESS,
   AUTHMGR_AUTH_TIMEOUT,
   AUTHMGR_AUTH_SERVER_COMM_FAILURE,
   AUTHMGR_CLIENT_DISCONNECTED,
   AUTHMGR_METHOD_CHANGE,
}  AUTHMGR_STATUS_t;

typedef enum
{
  AUTHMGR_INITIALIZE = 0,
  AUTHMGR_AUTHENTICATING,
  AUTHMGR_AUTHENTICATED,
  AUTHMGR_HELD,
  AUTHMGR_UNAUTHENTICATED,
  AUTHMGR_STATES
} AUTHMGR_STATES_t;

typedef enum
{
   AUTHMGR_ATTR_NA = 0,
   AUTHMGR_ATTR_RADIUS
}  AUTHMGR_ATTR_PROCESS_t;

/* Port authorization status */
typedef enum
{
   AUTHMGR_PORT_STATUS_NA = 0,
   AUTHMGR_PORT_STATUS_AUTHORIZED,
   AUTHMGR_PORT_STATUS_UNAUTHORIZED
}  AUTHMGR_PORT_STATUS_t;

typedef enum
{
   AUTHMGR_METHOD_8021X_ATTEMPTS = 1,
   AUTHMGR_METHOD_8021X_FAILED_ATTEMPTS,
   AUTHMGR_METHOD_MAB_ATTEMPTS,
   AUTHMGR_METHOD_MAB_FAILED_ATTEMPTS,
}  AUTHMGR_STATS_TYPE_t;


/* Port authorization mode */
typedef enum
{
   AUTHMGR_PORT_CONTROL_INVALID = 0,
   AUTHMGR_PORT_FORCE_UNAUTHORIZED = 1,
   AUTHMGR_PORT_FORCE_AUTHORIZED,
   AUTHMGR_PORT_AUTO,
   AUTHMGR_PORT_NA
}  AUTHMGR_PORT_CONTROL_t;


/* Port host mode */
typedef enum
{
   AUTHMGR_INVALID_HOST_MODE = 0,
   AUTHMGR_SINGLE_AUTH_MODE,
   AUTHMGR_MULTI_HOST_MODE,
   AUTHMGR_MULTI_AUTH_MODE
}  AUTHMGR_HOST_CONTROL_t;


/* L2 learning */
typedef enum
{
   AUTHMGR_PORT_LEARNING_NA = 0,
   AUTHMGR_PORT_LEARNING_DISABLE,
   AUTHMGR_PORT_LEARNING_ENABLE,
   AUTHMGR_PORT_LEARNING_CPU
}  AUTHMGR_PORT_LEARNING_t;

/* violation callback */
typedef enum
{
   AUTHMGR_PORT_VIOLATION_CALLBACK_NA = 0,
   AUTHMGR_PORT_VIOLATION_CALLBACK_DISABLE,
   AUTHMGR_PORT_VIOLATION_CALLBACK_ENABLE
}  AUTHMGR_PORT_VIOLATION_CALLBACK_t;

typedef enum authmgrClientType_s
{
  AUTHMGR_CLIENT_UNASSIGNED = 0,
  AUTHMGR_CLIENT_AWARE,
  AUTHMGR_CLIENT_UNAWARE,
  AUTHMGR_CLIENT_MAB
}authmgrClientType_t;


typedef enum authmgrVlanType_s
{
  AUTHMGR_VLAN_UNASSIGNED = 0,
  AUTHMGR_VLAN_RADIUS,
  AUTHMGR_VLAN_UNAUTH,
  AUTHMGR_VLAN_GUEST,
  AUTHMGR_VLAN_DEFAULT,
  AUTHMGR_VLAN_BLOCKED
}authmgrVlanType_t;

/* Vlan Assignment Mode */
typedef enum
{
   AUTHMGR_NOT_ASSIGNED = 0,
   AUTHMGR_DEFAULT_ASSIGNED_VLAN,
   AUTHMGR_RADIUS_ASSIGNED_VLAN,
   AUTHMGR_UNAUTHENTICATED_VLAN,
   AUTHMGR_GUEST_VLAN,
}  AUTHMGR_VLAN_ASSIGNED_MODE_t;


/* Authentication violation types */
typedef enum
{
   AUTHMGR_PORT_AUTH_VIOLATION_INVALID = 0,
   AUTHMGR_PORT_AUTH_VIOLATION_PROTECT,
   AUTHMGR_PORT_AUTH_VIOLATION_RESTRICT,
   AUTHMGR_PORT_AUTH_VIOLATION_SHUTDOWN,
   AUTHMGR_PORT_AUTH_VIOLATION_MODE_LAST
}  AUTHMGR_PORT_AUTH_VIOLATION_MODE_t;


/* Radius Termination Action (needed by UI)*/
typedef enum 
{
   AUTHMGR_TERMINATION_ACTION_DEFAULT = 1,
   AUTHMGR_TERMINATION_ACTION_RADIUS
}  AUTHMGR_TERMINATION_ACTION_t;


/* Authentication types used for Mac-Authentication Bypass */
typedef enum
{
   AUTHMGR_PORT_MAB_AUTH_TYPE_INVALID = 0,
   AUTHMGR_PORT_MAB_AUTH_TYPE_EAP_MD5,
   AUTHMGR_PORT_MAB_AUTH_TYPE_PAP,
   AUTHMGR_PORT_MAB_AUTH_TYPE_CHAP,
   AUTHMGR_PORT_MAB_AUTH_TYPE_LAST
}  AUTHMGR_PORT_MAB_AUTH_TYPE_t;


typedef enum authmgrFailureReason_s
{
  AUTHMGR_FAIL_REASON_INVALID  = 0,
  AUTHMGR_FAIL_REASON_AUTH_FAILED  = 1,
  AUTHMGR_FAIL_REASON_WRONG_AUTH     = 2,
  AUTHMGR_FAIL_REASON_INVALID_USER   = 3
  
}authmgrFailureReason_t;

typedef enum authmgrRadiusAttrFlags_s
{
  AUTHMGR_RADIUS_ATTR_TYPE_STATE = (1 << 0),
  AUTHMGR_RADIUS_ATTR_TYPE_SERVICE_TYPE = (1 << 1),
  AUTHMGR_RADIUS_ATTR_TYPE_CLASS = (1 << 2),
  AUTHMGR_RADIUS_ATTR_TYPE_SESSION_TIMEOUT = (1 << 3),
  AUTHMGR_RADIUS_ATTR_TYPE_TERMINATION_ACTION = (1 << 4),
  AUTHMGR_RADIUS_ATTR_TYPE_EAP_MESSAGE = (1 << 5),
  AUTHMGR_RADIUS_ATTR_TYPE_TUNNEL_TYPE = (1 << 6),
  AUTHMGR_RADIUS_ATTR_TYPE_TUNNEL_MEDIUM_TYPE = (1 << 7),
  AUTHMGR_RADIUS_ATTR_TYPE_TUNNEL_PRIVATE_GROUP_ID = (1 << 8),
  AUTHMGR_RADIUS_ATTR_USER_NAME = (1 << 9)
}authmgrRadiusAttrFlags_t;


typedef enum authmgrClientEventCode_s
{
  AUTHMGR_8021X_FIRST = (1 << 0),
  AUTHMGR_8021X_HIGHER_PRIO = (1 << 1)
}authmgrClientEventCode_t;



typedef struct authmgrPortSessionStats_s
{
  /* Authenticator Stats (9.4.4) */
  uint32        sessionOctetsRx;
  uint32        sessionOctetsTx;
  uint32        sessionOctetsGbRx;
  uint32        sessionOctetsGbTx;
  uint32        sessionPacketsRx;
  uint32        sessionPacketsGbRx;
  uint32        sessionPacketsTx;
  uint32        sessionPacketsGbTx;
  uint32        sessionTime;
   uchar8        userName[AUTHMGR_USER_NAME_LEN];
   char8         sessionId[AUTHMGR_SESSION_ID_LEN];
} authmgrPortSessionStats_t;

typedef enum
{
  AUTHMGR_LOGICAL_PORT = 0,
  AUTHMGR_PHYSICAL_PORT
}authmgrPortType_t;

typedef enum authmgrNodeType_s
{
  AUTHMGR_NODE_UNASSIGNED = 0,
  AUTHMGR_PHYSICAL,
  AUTHMGR_LOGICAL
}authmgrNodeType_t;


typedef enum authmgrFilterAssignedType_s
{
  AUTHMGR_FILTER_ASSIGNED_NONE = 0,
  AUTHMGR_FILTER_ASSIGNED_RADIUS = 1,
  AUTHMGR_FILTER_ASSIGNED_FAILED = 2
}authmgrFilterAssignedType_t;



typedef struct authmgrAuthAttributeInfo_s
{
   uchar8   userName[AUTHMGR_USER_NAME_LEN];
  uint32   userNameLen;

   uchar8   serverState[AUTHMGR_SERVER_STATE_LEN];
  uint32   serverStateLen;

   uchar8   serverClass[AUTHMGR_SERVER_CLASS_LEN];
  uint32   serverClassLen;

  uint32   sessionTimeout;
  uint32   terminationAction;

  uint32   accessLevel;  
   uchar8   idFromServer;   /* Most recent ID in EAP pkt received from Auth Server (0-255) */
   uchar8   vlanString[AUTHMGR_RADIUS_VLAN_ASSIGNED_LEN+1];
  uint32   vlanId; /* parsed VLAN id from vlan string */ 
  uint32   attrFlags;
  uint32   vlanAttrFlags;
   BOOL     rcvdEapAttr;
}authmgrAuthAttributeInfo_t;


typedef struct authmgrClientAuthInfo_s
{
   enetMacAddr_t          macAddr;
  uint32                 eapolVersion;
  uint32                 authMethod;
  authmgrAuthAttributeInfo_t attrInfo;
  uint32                 sessionId;
   char8 authmgrUserName[AUTHMGR_USER_NAME_LEN];
  uint32 authmgrUserNameLength;
}authmgrClientAuthInfo_t;

typedef struct authmgrClientStatusInfo_s
{
  union 
  {
    authmgrClientAuthInfo_t authInfo;
    uint32               enableStatus;
  }info;
}authmgrClientStatusInfo_t;

typedef struct authmgrClientStatusReply_s
{
  char intf[16];
  char addr[6];
  unsigned int method;
  unsigned int status;
  union 
  {
    unsigned int vlanId;
    unsigned int enableStatus;
  }info;
}authmgrClientStatusReply_t;

typedef enum authmgrNotifyEvent_s
{
  authmgrClientReAuthenticate = 1,
  authmgrClientAuthStart,
//  authmgrClientReqIdTx,
  authmgrClientDisconnect
}authmgrNotifyEvent_t;


#define  AUTHMGR_MULTI_HOST_MODE_MAX_USERS 1
#define  AUTHMGR_SINGLE_AUTH_MODE_MAX_USERS 1


#define  AUTHMGR_RADIUS_MAX_AUTH_ATTEMPTS	1
#define  AUTHMGR_RADIUS_MAX_AUTH_ATTEMPTS_RANGE_MIN 1
#define  AUTHMGR_RADIUS_MAX_AUTH_ATTEMPTS_RANGE_MAX 5

#define  AUTHMGR_AUTHENTICATION_MAX_EVENTS        1024

#define  AUTHMGR_AUTHENTICATION_MAX_INTF_EVENTS   20


#define  AUTHMGR_RESTART_TIMER_MIN      10 
#define  AUTHMGR_RESTART_TIMER_MAX      65535 

#define  AUTHMGR_PORT_MIN_MAC_USERS        1
#define  AUTHMGR_PORT_MAX_MAC_USERS        FD_AUTHMGR_PORT_MAX_USERS 


#define  AUTHMGR_PORT_MIN_QUIET_PERIOD     0
#define  AUTHMGR_PORT_MAX_QUIET_PERIOD     65535

#define  AUTHMGR_PORT_MIN_TX_PERIOD        1
#define  AUTHMGR_PORT_MAX_TX_PERIOD        65535

#define  AUTHMGR_PORT_MIN_SUPP_TIMEOUT     1
#define  AUTHMGR_PORT_MAX_SUPP_TIMEOUT     65535

#define  AUTHMGR_PORT_MIN_SERVER_TIMEOUT   1
#define  AUTHMGR_PORT_MAX_SERVER_TIMEOUT   65535

#define  AUTHMGR_PORT_MIN_MAX_REQ          1
#define  AUTHMGR_PORT_MAX_MAX_REQ          20

#define  AUTHMGR_PORT_MIN_MAX_REQ_IDENTITY     1
#define  AUTHMGR_PORT_MAX_MAX_REQ_IDENTITY     20

#define  AUTHMGR_PORT_MIN_REAUTH_PERIOD    1
#define  AUTHMGR_PORT_MAX_REAUTH_PERIOD    65535

#define  AUTHMGR_MAX_USERS_PER_PORT  FD_AUTHMGR_PORT_MAX_USERS

/* Get the re-authentication timeout value from the server */
#define  AUTHMGR_PORT_REAUTH_PERIOD_FROM_SERVER   TRUE 

/******************** conditional Override *****************************/

#ifdef INCLUDE_AUTH_MGR_EXPORTS_OVERRIDES
#include "auth_mgr_exports_overrides.h"
#endif

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* __AUTHMGR_EXPORTS_H_*/
