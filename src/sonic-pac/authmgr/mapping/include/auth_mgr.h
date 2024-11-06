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

#ifndef AUTHMGR_H
#define AUTHMGR_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

/* Authentication Manager Timers */
typedef enum
{
  AUTH_MGR_RESTART = 0,
} auth_mgr_timer_t;

/* Authentication Manager Event Message IDs */
typedef enum
{
  authMgrMethodSet = 1,
  authMgrPrioritySet,
  authMgrOpenAccess,
  authMgrRestartTimerSet,
  authMgrRestartTimerExpiry,
  authMgrNimStartup,
  authMgrCnfgr,
  authMgrActivateStartupDone,
  authMgrHandleNewBackupManager
}authMgrMessages_t;

typedef enum
{
  AUTH_MGR_UNAUTHENTICATED = 0,
  AUTH_MGR_AUTHENTICATED
} authMgrAuthStatus_t;


typedef struct authMgrIntfChangeParms_s
{
  uint32 event;
  NIM_CORRELATOR_t correlator;
} authMgrIntfChangeParms_t;

typedef struct authMgrNimStartup_s
{
  NIM_STARTUP_PHASE_t startupPhase;
} authMgrNimStartup_t;

typedef struct authMgrTimerParams_s
{
  uint32 timerCBHandle;
} authMgrTimerParams_t;
#define AUTHMGR_TIMER_MSG_SIZE         sizeof(authMgrTimerParams_t)

/* authentication manager Event Message format */
typedef struct authMgrMgmtMsg_s
{
  uint32        msgId;    /* Of type snoopMgmtMessages_t */
  uint32        intIfNum;
  union
  {
     CNFGR_CMD_DATA_t          CmdData;
    authMgrIntfChangeParms_t     authMgrIntfChangeParms;
    authMgrNimStartup_t          authMgrNimStartup;
    uint32                    mode;
    authMgrTimerParams_t         authMgrParams;
    uint32                    timerValue;
  } u;
} authMgrMgmtMsg_t;
#define AUTHMGR_MSG_SIZE         sizeof(authMgrMgmtMsg_t)


/* Start of Function Prototype */
void authMgrNotifyRegisteredUsers(uint32 intIfNum,
                               uint32 event);
/* End of function prototypes */

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* AUTHMGR_H */

