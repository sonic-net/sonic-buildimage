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

#ifndef INCLUDE_AUTHMGR_STRUCT_H
#define INCLUDE_AUTHMGR_STRUCT_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "pacinfra_common.h"
#include "osapi.h"
#include "portevent_mask.h"
#include "avl_api.h"
#include "tree_api.h"
#include "apptimer_api.h"
#include "auth_mgr_db.h"
#include "auth_mgr_cfg.h"
#include "auth_mgr_api.h"
#include "auth_mgr_debug.h"
#include "auth_mgr_include.h"

typedef struct authmgrGlobalInfo_s
{
  authmgrCfg_t *authmgrCfg;
  authmgrInfo_t      authmgrInfo;
  authmgrPortInfo_t  *authmgrPortInfo;
  authmgrPortStats_t *authmgrPortStats;
  uint32        *authmgrMapTbl;
  authmgrPortSessionStats_t *authmgrPortSessionStats;
  authmgrDebugCfg_t authmgrDebugCfg;

  authmgrMethodCallbackNotifyMap_t authmgrCallbacks[ AUTHMGR_METHOD_LAST];
  /* App timer related data */
  APP_TMR_CTRL_BLK_t   authmgrTimerCB;
  uint32              authmgrAppTimerBufferPoolId;

  /* avl tree parameters */
  avlTree_t                authmgrLogicalPortTreeDb;
  avlTreeTables_t          *authmgrLogicalPortTreeHeap;
  authmgrLogicalPortInfo_t   *authmgrLogicalPortDataHeap;

  uint32     authmgrMacAddrBufferPoolId;
  sll_t      authmgrMacAddrSLL;
  osapiRWLock_t authmgrMacAddrDBRWLock;

  VLAN_MASK_t authmgrVlanMask;
  int32 eap_socket;
  uint32 reservedVlan;
}authmgrGlobalInfo_t;

typedef struct authmgrCB_s
{
  void  *authmgrTaskSyncSema;
  void * authmgrTaskId;
  void  *authmgrSrvrTaskSyncSema;
  void * authmgrSrvrTaskId;
  int   listen_sock;
  osapiRWLock_t       authmgrRWLock;
  osapiRWLock_t       authmgrCfgRWLock;
  void *authmgrQueue;      /* reference to the authmgr message queue */
  void *authmgrBulkQueue;      /* reference to the authmgr bulk message queue */
  void *authmgrVlanEventQueue;      /* reference to the authmgr vlan message queue */
  authmgrGlobalInfo_t *globalInfo;
  authmgrClientInfo_t processInfo;
  authmgrClientInfo_t oldInfo;
  authmgrAuthAttributeInfo_t attrInfo;
}authmgrCB_t;

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_AUTHMGR_STRUCT_H */
