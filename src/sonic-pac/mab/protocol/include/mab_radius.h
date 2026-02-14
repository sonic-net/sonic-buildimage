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

#ifndef __INCLUDE_MAB_RADIUS_H__
#define __INCLUDE_MAB_RADIUS_H__

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "radius_attr_parse.h"

extern RC_t mabRadiusResponseCallback(void *msg, uint32 correlator);
extern RC_t mabRadiusResponseProcess(unsigned int lIntIfNum, void *resp);
extern RC_t mabRadiusAcceptProcess(uint32 intIfNum, void *payload);
extern RC_t mabRadiusResponseHandle(void *resp, int len);
extern RC_t mabSendRespToServer(uint32 lIntIfNum,  netBufHandle bufHandle);

extern RC_t mabRadiusChallengeProcess(uint32 intIfNum, void *radiusPayload);
extern RC_t mabRadiusAccessRequestSend(uint32 intIfNum,  uchar8 *suppEapData);
extern RC_t mabRadiusSuppResponseProcess(uint32 intIfNum,  netBufHandle bufHandle);
extern  void mabRadiusClearRadiusMsgsSend( enetMacAddr_t suppMacAddr);

extern int radius_mab_client_register(void *data);

extern RC_t mabRadiusServerTaskLockTake(void);
extern RC_t mabRadiusServerTaskLockGive(void);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_MAB_RADIUS_H__*/
