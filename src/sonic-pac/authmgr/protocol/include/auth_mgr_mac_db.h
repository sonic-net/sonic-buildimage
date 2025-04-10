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


#ifndef INCLUDE_AUTHMGR_MAC_DB_H
#define INCLUDE_AUTHMGR_MAC_DB_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "sll_api.h"
#include "buff_api.h"

typedef enum authMgrFdbCfgType_s
{
   AUTHMGR_FDB_CFG_ADD = 0,
   AUTHMGR_FDB_CFG_REMOVE
} authMgrFdbCfgType_t;

extern RC_t authmgrMacAddrDataDestroy ( sll_member_t *ll_member);
extern  int32 authmgrMacAddrDataCmp(void *p, void *q, uint32 key);
extern RC_t authmgrMacAddrInfoDBInit(uint32 nodeCount);
extern RC_t authmgrMacAddrInfoDBDeInit(void);
extern RC_t authmgrMacAddrInfoAdd( enetMacAddr_t *mac_addr,uint32 lIntIfNum);
extern RC_t authmgrMacAddrInfoRemove( enetMacAddr_t *mac_addr);
extern RC_t authmgrMacAddrInfoFind( enetMacAddr_t *mac_addr,uint32 *lIntIfNum);
extern RC_t authmgrMacAddrInfoFindNext( enetMacAddr_t *mac_addr,uint32 *lIntIfNum);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_AUTHMGR_MAC_DB_H */
