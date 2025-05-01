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

#ifndef INCLUDE_AUTHMGR_DB_H
#define INCLUDE_AUTHMGR_DB_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "comm_mask.h"
#include "apptimer_api.h"
#include "auth_mgr_exports.h"
//#include "acl_exports.h"
#include "auth_mgr_sm.h"
#include "auth_mgr_vlan.h"
#include "auth_mgr_util.h"
#include "avl_api.h"


#define AUTHMGR_CLIENT_TIMEOUT    300 /* seconds. Checks for client timeout every 5 minutes*/

#define AUTHMGR_LOGICAL_PORT_START    0 
#define AUTHMGR_LOGICAL_PORT_END       AUTHMGR_MAX_USERS_PER_PORT
#define AUTHMGR_LOGICAL_PORT_ITERATE  0xFFFFFFFF

/* Max Unsigned Integer Value i.e.; (2 ^ 16)-1 or (2 ^ 32)-1 */
#define AUTHMGR_UNSIGNED_INTERGER_MAX_LIMIT 0xFFFFFFFF

#define AUTHMGR_IF_INVALID_METHOD_RETURN_LOG(_method_) \
  if (( AUTHMGR_METHOD_8021X != _method_)&& \
      ( AUTHMGR_METHOD_MAB != _method_)) \
  { \
     LOGF( LOG_SEVERITY_ERROR, "%s %d is invalid.", #_method_, _method_); \
    return  FAILURE; \
  } 

#define AUTHMGR_MAX_ACLS_CLIENT 2

typedef enum authmgrParamOperation_s
{
  AUTHMGR_PARAM_DELETE = 1,
  AUTHMGR_PARAM_ADD
}authmgrParamOperation_t;


/* switch info */
typedef struct authmgrInfo_s
{
  uint32 traceId;
} authmgrInfo_t;

typedef enum authmgrTimerType_s
{
  AUTHMGR_TIMER_UNASSIGNED = 0,
  AUTHMGR_QWHILE,
  AUTHMGR_REAUTH_WHEN,
  AUTHMGR_METHOD_NO_RESP_TMR
}authmgrTimerType_t;

typedef enum authmgrHwAttr_s
{
  AUTHMGR_HW_ATTR_STATIC_FDB = 1,
  AUTHMGR_HW_ATTR_BLOCK_FDB,
  AUTHMGR_HW_ATTR_PVID,
  AUTHMGR_HW_ATTR_LAST
}authmgrHwAttr_t;







typedef struct authmgrClientInfo_s
{
  /* authmgr Client category */
  authmgrClientType_t clientType;

  /* Re-auth and session related info */
 /* Set to TRUE when reAuthWhen timer expires.
  This one is from the user and not when the timer expires */
   BOOL reAuthenticate;
   int32                retryCount;      /* Number of retres with wrong auth credentials */
  uint32               reAuthCount;  /* Number of times AUTHENTICATING state is entered */

   AUTHMGR_METHOD_t     currentMethod;
   AUTHMGR_METHOD_t     authenticatedMethod;
   AUTHMGR_METHOD_t     executedMethod[ AUTHMGR_METHOD_LAST];

  /* client authentication status */
   AUTHMGR_PORT_STATUS_t  logicalPortStatus;      /* Current authorization state of the port */

  /* user Details */
   uchar8 authmgrUserName[AUTHMGR_USER_NAME_LEN];
  uint32 authmgrUserNameLength;

  uint32 serverStateLen;
   uchar8 serverState[AUTHMGR_SERVER_STATE_LEN];
   uchar8 serverClass[AUTHMGR_SERVER_CLASS_LEN];
  uint32 serverClassLen;

  uint32 currentIdL;

  uint32 rcvdEapolVersion; 

   enetMacAddr_t suppMacAddr;   /* MAC address of Supplicant */

  /* vlan related info */
  authmgrVlanType_t  vlanType;     /* assigned vlan category */
   BOOL          vlanTypePortCfg;  /* vlan picked from port cfg */
  uint32        vlanId;       /* Vlan Id of Supplicant */
  uint32        blockVlanId;  /* blocked Vlan Id of Supplicant */
   BOOL          dataBlocked; 

 
   BOOL       reAuthenticating;
   BOOL       suppRestarting;

   USER_MGR_AUTH_METHOD_t authMethod; /* Auth method for the user of this port */
  uint32                 sessionTime;  
  
  uint32    clientTimeout;
  
  uint32   sessionTimeout;
  uint32   terminationAction;
  uint32   lastAuthTime;

   AUTHMGR_PORT_MAB_AUTH_TYPE_t mabAuthType; /* Authentication type used by MAB. To be filled in only if isMABClient is  TRUE */

  uint32 attrCreateMask;
}authmgrClientInfo_t;

typedef struct authmgrLogicalNodeKey_s
{
  /* first 16 bits represent physical port
     next 12 bits represent logical port
     and remaining 3 bits represent client type.
     last bit is always 0 */
  uint32 keyNum;
}authmgrLogicalNodeKey_t;

typedef struct authmgrTimerContext_s
{
  authmgrTimerType_t   type;
  uint32          keyNum;
}authmgrTimerContext_t;


typedef struct authmgrTimerHandle_s
{
   APP_TMR_HNDL_t         timer;
 /* void                     *timerHandle; */
}authmgrTimerHandle_t;


typedef struct authmgrTimer_s
{
  authmgrTimerContext_t      cxt;
  authmgrTimerHandle_t       handle;
}authmgrTimer_t;


typedef struct authmgrProtocolInfo_s
{
  BOOL  heldTimerExpired;
 AUTHMGR_STATES_t authState;
  BOOL authSuccess;
  BOOL authFail;
  BOOL authTimeout;
  BOOL unAuthenticate;
  BOOL authenticate;
 /* Set to TRUE when reAuthWhen timer expires */
  BOOL reauth;
  BOOL newMethodInsert;
  BOOL eapSuccess;
  BOOL authenticatedRcvdStartAuth;
}authmgrProtocolInfo_t;

/* logical port info */
typedef struct authmgrLogicalPortInfo_s
{
 /* unique node identifier*/ 
  authmgrLogicalNodeKey_t key;

  authmgrTimer_t          authmgrTimer;
  authmgrTimer_t          authmgrMethodNoRespTimer;

  /* protocol related info */
  authmgrProtocolInfo_t protocol;

  /* client specific non protocol data */
  authmgrClientInfo_t client;
  
  void         *next; /* This field must be the last one in this structure */
} authmgrLogicalPortInfo_t;

typedef struct authmgrPortInfo_s
{
  uint32            maxUsers;
  uint32            numUsers;
  uint32               authCount;       /* number of authorized clients */ 
  uint32           currentId;

   uchar8            paeCapabilities;  

  /* Authentication methods */
   AUTHMGR_METHOD_t           enabledMethods[ AUTHMGR_METHOD_LAST];
  /* Authentication priority */
   AUTHMGR_METHOD_t           enabledPriority[ AUTHMGR_METHOD_LAST];

  uint32     enabledMethodCount;
  uint32     enabledPriorityCount;
  
  /* Inter-state machine communication and initialization */
   BOOL                 initialize;      /* Set to TRUE by mgmt to cause port initialization */
   AUTHMGR_PORT_CONTROL_t portControlMode; /* Current control mode setting by mgmt */
   AUTHMGR_HOST_CONTROL_t hostMode;        /* host mode setting by mgmt */
   BOOL                 portEnabled;     /* TRUE if port is active */
   AUTHMGR_PORT_STATUS_t  portStatus;      /* Current authorization state of the port */
   AUTHMGR_PORT_STATUS_t  nimStatus;       /* Status set to nim so duplicate events are not sent to nim */
   BOOL  unLearnMacPolicy;       

  /* Authenticator PAE state machine parameters */
  uint32               quietPeriod;  /* Initialization value for quietWhile timer */

  /* Reauthentication Timer state machine parameters */
  uint32 reAuthPeriod;   /* Number of seconds between periodic reauthentication */
   BOOL   reAuthEnabled;  /* TRUE if reauthentication is enabled */
   BOOL   reAuthPeriodServer;

  uint32 authFailRetryMaxCount;    /* Max reauthentication attempts */

   BOOL        authmgrAcquire;  /* flag to keep track of authmgr acquire for a port */
   AUTHMGR_PORT_AUTH_VIOLATION_MODE_t violationMode;
   uint32 pvid;
   uint32 authVlan;
} authmgrPortInfo_t;



typedef RC_t(*authmgrCtrlTimerExpiryFn_t) (authmgrLogicalPortInfo_t *logicalPortInfo);
typedef RC_t(*authmgrCtrlTimerNodeSetFn_t) (uint32 intIfNum, uint32 val);
typedef RC_t(*authmgrCtrlTimerNodeGetFn_t) (uint32 intIfNum, uint32 *val);


typedef struct authmgrTimerMap_s
{
  authmgrTimerType_t  type;
  authmgrCtrlTimerExpiryFn_t  expiryFn;
   BOOL                     serverConfigSupport;
  authmgrCtrlTimerNodeGetFn_t getFn;
  authmgrCtrlTimerNodeGetFn_t lportGetFn;
}authmgrTimerMap_t;


typedef RC_t(*authmgrCtrlHostModeSetFn_t) (uint32 intIfNum);

typedef struct authmgrHostModeMap_s
{
   AUTHMGR_HOST_CONTROL_t  hostMode;
  authmgrCtrlHostModeSetFn_t  hostModeFn;
}authmgrHostModeMap_t;

typedef RC_t(*authmgrPortCtrlLearnFn_t) (uint32 intIfNum);

typedef struct authmgrPortCtrlLearnMap_s
{
   AUTHMGR_PORT_CONTROL_t  portControlMode;
  authmgrPortCtrlLearnFn_t       learnFn;
}authmgrPortCtrlLearnMap_t;


typedef RC_t(*authmgrHostCtrlLearnFn_t) (uint32 intIfNum);

typedef struct authmgrHostCtrlLearnMap_s
{
   AUTHMGR_HOST_CONTROL_t  hostMode;
  authmgrHostCtrlLearnFn_t   learnFn;
}authmgrHostCtrlLearnMap_t;

typedef enum authmgr_stats_update_enum_s
{
  authmgrStatsAuthEnter = 1,
  authmgrStatsAuthSuccess,
  authmgrStatsAuthFail,
  authmgrStatsAuthTimeout
}authmgrStatsUpdate_t;


typedef RC_t(*authmgrStatsMapFn_t) (uint32 intIfNum, authmgrStatsUpdate_t status);

typedef struct authmgrStatsMap_s
{
   AUTHMGR_METHOD_t  type;
  authmgrStatsMapFn_t  statsFn;
}authmgrStatsMap_t;

typedef RC_t(*authmgrHwCleanupEventFn_t) (authmgrLogicalPortInfo_t *logicalPortInfo);

typedef struct authmgrHwCleanupEventMap_s
{
  uint32  event;
  authmgrHwCleanupEventFn_t  cleanupFn;
}authmgrHwCleanupEventMap_t;

typedef struct authmgrMethodCommonStats_s
{
  uint32 authEntersAuthenticating;
  uint32 authSuccess;
  uint32 authFailure;
  uint32 authTimeout;
  uint32 authAbort;
}authmgrMethodCommonStats_s;


typedef struct authmgrEapStats_s
{
  uint32 eapTxSuccess;
  uint32 eapTxFailure;
}authmgrEapStats_t;

typedef struct authmgrPortStats_s
{
  authmgrEapStats_t    eap;
  authmgrMethodCommonStats_s dot1x;
  authmgrMethodCommonStats_s mab;
  authmgrMethodCommonStats_s cp;
} authmgrPortStats_t;

extern authmgrInfo_t authmgrInfo;
extern authmgrPortInfo_t *authmgrPortInfo;
extern authmgrPortStats_t *authmgrPortStats;
extern uint32 *authmgrMapTbl;
extern authmgrPortSessionStats_t  *authmgrPortSessionStats;



/*********************************************************************************************/


/* Prototypes for the auth_mgr_db.c file */
RC_t authmgrLogicalPortInfoDBInit(uint32 nodeCount);
RC_t authmgrLogicalPortInfoDBDeInit(void);

RC_t authmgrLogicalPortInfoTakeLock(void);
RC_t authmgrLogicalPortInfoGiveLock(void);

authmgrLogicalPortInfo_t *authmgrLogicalPortInfoAlloc(uint32 intIfNum);
RC_t authmgrLogicalPortInfoDeAlloc(authmgrLogicalPortInfo_t *node);

authmgrLogicalPortInfo_t *authmgrLogicalPortInfoGet(uint32 lIntIfNum);
authmgrLogicalPortInfo_t *authmgrLogicalPortInfoGetNext(uint32 lIntIfNum);

authmgrLogicalPortInfo_t *authmgrLogicalPortInfoFirstGet(uint32 intIfNum,
                                                    uint32 *lIntIfNum);
authmgrLogicalPortInfo_t *authmgrLogicalPortInfoGetNextNode(uint32 intIfNum,
                                                        uint32 *lIntIfNum);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_AUTHMGR_DB_H */
