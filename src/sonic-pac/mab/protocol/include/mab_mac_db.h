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

#ifndef INCLUDE_MAB_MAC_DB_H
#define INCLUDE_MAB_MAC_DB_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif


#include "sll_api.h"
#include "buff_api.h"

extern RC_t mabMacAddrDataDestroy ( sll_member_t *ll_member);
extern  int32 mabMacAddrDataCmp(void *p, void *q, uint32 key);
extern RC_t mabMacAddrInfoDBInit(uint32 nodeCount);
extern RC_t mabMacAddrInfoDBDeInit(void);
extern RC_t mabMacAddrInfoAdd( enetMacAddr_t *mac_addr,uint32 lIntIfNum);
extern RC_t mabMacAddrInfoRemove( enetMacAddr_t *mac_addr);
extern RC_t mabMacAddrInfoFind( enetMacAddr_t *mac_addr,uint32 *lIntIfNum);
extern RC_t mabMacAddrInfoFindNext( enetMacAddr_t *mac_addr,uint32 *lIntIfNum);


/* USE C Declarations */
#ifdef __cplusplus
}
#endif
#endif /* INCLUDE_MAB_MAC_DB_H */
