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


#ifndef INCLUDE_MAB_DB_H
#define INCLUDE_MAB_DB_H

/* USE C Declarations */
#ifdef __cplusplus
  extern "C" {
#endif

#include "comm_mask.h"
#include "apptimer_api.h"
#include "mab_vlan.h"
#include "auth_mgr_exports.h"
#include "mab_radius.h"
#include "mab_exports.h"
#include "mab_util.h"
#include "avl_api.h"
#include "radius_attr_parse.h"

#define MAB_USER_INDEX_INVALID  -1

#define MAB_MD5_LEN   16

#define MAB_SERVER_STATE_LEN 253
#define MAB_SERVER_CLASS_LEN 253

#define MAB_FILTER_NAME_LEN 256

#define MAB_LOGICAL_PORT_START    0 
#define MAB_LOGICAL_PORT_END       MAB_MAX_USERS_PER_PORT

#define MAB_LOGICAL_PORT_ITERATE  0xFFFFFFFF
#define MAB_RADIUS_VLAN_ASSIGNED_LEN  32 /* Radius Assigned vlan length */

/* switch info */
typedef struct mabInfo_s
{
  uint32 traceId;
} mabInfo_t;

typedef enum mabTimerType_s
{
  MAB_TIMER_UNASSIGNED = 0,
  MAB_SERVER_AWHILE,
}mabTimerType_t;

typedef enum
{
  MAB_UNAUTHENTICATED = 0,
  MAB_AUTHENTICATING,
  MAB_AUTHENTICATED
} MAB_AUTH_STATES_t;

typedef struct mabProtocolInfo_s
{
  MAB_AUTH_STATES_t mabAuthState;
   BOOL authSuccess;
   BOOL authFail;
}mabProtocolInfo_t;

typedef struct mabClientInfo_s
{
  /* mab Client category */
  authmgrClientType_t clientType;

  /* Re-auth and session related info */
   uchar8               currentIdL;       /* ID of current auth session (0-255) */
   BOOL                 reAuthenticate;  /* Set to TRUE when reAuthWhen timer expires */

  /* client authentication status */
   AUTHMGR_PORT_STATUS_t  logicalPortStatus;      /* Current authorization state of the port */

  /* user Details */
   uchar8 mabUserName[MAB_USER_NAME_LEN];
   uint32 mabUserNameLength;
   int32 mabUserIndex;

   uchar8 mabChallenge[MAB_CHALLENGE_LEN];
   uint32 mabChallengelen;

   netBufHandle suppBufHandle;  /* Hold onto buf handle for re-transmit */
   enetMacAddr_t suppMacAddr;   /* MAC address of Supplicant */

   /* vlan related info */
   authmgrVlanType_t  vlanType;     /* assigned vlan category */
   uint32        vlanId;       /* Vlan Id of Supplicant */

   attrInfo_t attrInfo;
   uchar8   filterName[MAB_FILTER_NAME_LEN];
 
   USER_MGR_AUTH_METHOD_t authMethod; /* Auth method for the user of this port */

   AUTHMGR_PORT_MAB_AUTH_TYPE_t mabAuthType; /* Authentication type used by MAB. To be filled in only if isMABClient is  TRUE */

}mabClientInfo_t;

typedef struct mabLogicalNodeKey_s
{
  /* first 16 bits represent physical port
     next 12 bits represent logical port
     and remaining 3 bits represent client type.
     last bit is always 0 */
  uint32 keyNum;
}mabLogicalNodeKey_t;


typedef struct mabTimerContext_s
{
  mabTimerType_t   type;
  uint32          keyNum;
}mabTimerContext_t;


typedef struct mabTimerHandle_s
{
   APP_TMR_HNDL_t         timer;
 /* void                     *timerHandle; */
}mabTimerHandle_t;


typedef struct mabTimer_s
{
  mabTimerContext_t      cxt;
  mabTimerHandle_t       handle;
}mabTimer_t;


/* logical port info */
typedef struct mabLogicalPortInfo_s
{
 /* unique node identifier*/ 
  mabLogicalNodeKey_t key;

  mabTimer_t          mabTimer;

  /* protocol related info */
  mabProtocolInfo_t protocol;

  /* client specific non protocol data */
  mabClientInfo_t client;
  
  void         *next; /* This field must be the last one in this structure */
} mabLogicalPortInfo_t;


/* Per-port info */
typedef struct mabPortInfo_s
{
  uint32            maxUsers;
  uint32            numUsers;

  /* Inter-state machine communication and initialization */
   uchar8               currentId;       /* ID of current auth session (0-255) */
   BOOL                 initialize;      /* Set to TRUE by mgmt to cause port initialization */
   AUTHMGR_PORT_CONTROL_t portControlMode; /* Current control mode setting by mgmt */
   AUTHMGR_HOST_CONTROL_t hostMode;      /* host mode setting by mgmt */
   BOOL                 portEnabled;     /* TRUE if port is active */
   uint32               authCount;       /* number of authorized clients */ 
   uint32               serverTimeout;   /* Initialization value for aWhile timer when timing out Auth. Server */
   USER_MGR_AUTH_METHOD_t authMethod;    /* Authentication method for the user of this port */
   AcquiredMask            acquiredList;    /* Mask of components "acquiring" an interface */
   uint32               mabEnabled;      /*  ENABLE if MAB has been enabled on the port and port control mode is mac-based*/
    
} mabPortInfo_t;

typedef RC_t(*mabCtrlTimerExpiryFn_t) (mabLogicalPortInfo_t *logicalPortInfo);
typedef RC_t(*mabCtrlTimerNodeSetFn_t) (uint32 intIfNum, uint32 val);
typedef RC_t(*mabCtrlTimerNodeGetFn_t) (uint32 intIfNum, uint32 *val);

typedef struct mabTimerMap_s
{
  mabTimerType_t  type;
  mabCtrlTimerExpiryFn_t  expiryFn;
}mabTimerMap_t;


typedef RC_t(*mabCtrlHostModeSetFn_t) (uint32 intIfNum);

typedef struct mabHostModeMap_s
{
   AUTHMGR_HOST_CONTROL_t  hostMode;
  mabCtrlHostModeSetFn_t  hostModeFn;
}mabHostModeMap_t;


typedef RC_t(*mabPortCtrlLearnFn_t) (uint32 intIfNum);

typedef struct mabPortCtrlLearnMap_s
{
   AUTHMGR_PORT_CONTROL_t  portControlMode;
  mabPortCtrlLearnFn_t       learnFn;
}mabPortCtrlLearnMap_t;


typedef RC_t(*mabHostCtrlLearnFn_t) (uint32 intIfNum);

typedef struct mabHostCtrlLearnMap_s
{
   AUTHMGR_HOST_CONTROL_t  hostMode;
  mabHostCtrlLearnFn_t   learnFn;
}mabHostCtrlLearnMap_t;

typedef RC_t(*mabAuthmgrEventMapFn_t) (uint32 intIfNum,  enetMacAddr_t suppMacAddr);

typedef struct mabAuthmgrEventFnMap_s
{
  uint32  event;
  mabAuthmgrEventMapFn_t   eventMapFn;
}mabAuthmgrEventFnMap_t;

/* This structure is used to keep track of vlan addport/delport evetnts */
typedef struct mabMacBasedVlanParticipation_s
{
   INTF_MASK_t    intfBitMask;
   INTF_MASK_t    adminBitMask; /* Dot1q admin mode */
}mabMacBasedVlanParticipation_t;


typedef struct mabPortStats_s
{
  /* Authenticator Diagnostics */
  uint32 authEntersAuthenticating;
  uint32 authAuthSuccessWhileAuthenticating;
} mabPortStats_t;

extern mabInfo_t mabInfo;
extern mabPortInfo_t *mabPortInfo;
extern mabPortStats_t *mabPortStats;
extern uint32 *mabMapTbl;

/*********************************************************************************************/


/* Prototypes for the mab_db.c file */
RC_t mabLogicalPortInfoDBInit(uint32 nodeCount);
RC_t mabLogicalPortInfoDBDeInit(void);

RC_t mabLogicalPortInfoTakeLock(void);
RC_t mabLogicalPortInfoGiveLock(void);

mabLogicalPortInfo_t *mabLogicalPortInfoAlloc(uint32 intIfNum);
RC_t mabLogicalPortInfoDeAlloc(mabLogicalPortInfo_t *node);

mabLogicalPortInfo_t *mabLogicalPortInfoGet(uint32 lIntIfNum);
mabLogicalPortInfo_t *mabLogicalPortInfoGetNext(uint32 lIntIfNum);

mabLogicalPortInfo_t *mabLogicalPortInfoFirstGet(uint32 intIfNum,
                                                    uint32 *lIntIfNum);
mabLogicalPortInfo_t *mabLogicalPortInfoGetNextNode(uint32 intIfNum,
                                                        uint32 *lIntIfNum);
/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_MAB_DB_H */
