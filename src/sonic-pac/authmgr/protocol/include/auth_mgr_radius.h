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


#ifndef __INCLUDE_AUTHMGR_RADIUS_H__
#define __INCLUDE_AUTHMGR_RADIUS_H__

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#define AUTHMGR_MAC_ADDR_STR_LEN (( MAC_ADDR_LEN * 2) + ( MAC_ADDR_LEN - 1))

typedef enum
{
   RADIUS_FORMAT_UNKNOWN = 0,
   RADIUS_FORMAT_LEGACY_LOWERCASE,
   RADIUS_FORMAT_LEGACY_UPPERCASE,
   RADIUS_FORMAT_IETF_LOWERCASE,
   RADIUS_FORMAT_IETF_UPPERCASE,
   RADIUS_FORMAT_UNFORMATTED_LOWERCASE,
   RADIUS_FORMAT_UNFORMATTED_UPPERCASE
}  RADIUS_MAC_ATTRIBUTE_FORMAT_t;

extern RC_t authmgrRadiusResponseCallback(uint32 status, uint32 correlator,
                                              uchar8 *attribues, uint32 attributesLen);
extern RC_t authmgrRadiusResponseProcess(uint32 intIfNum, uint32 status,  uchar8 *attributes,
                                          uint32 attributesLen);
extern RC_t authmgrRadiusAcceptProcess(uint32 intIfNum,  uchar8 *radiusPayload, uint32 payloadLen);


extern RC_t authmgrRadiusChallengeProcess(uint32 intIfNum,  uchar8 *radiusPayload, uint32 payloadLen);
extern RC_t authmgrRadiusAccessRequestSend(uint32 intIfNum,  uchar8 *suppEapData);
extern RC_t authmgrRadiusSuppResponseProcess(uint32 intIfNum,  netBufHandle bufHandle);
extern RC_t authmgrRadiusAcceptPostProcess(authmgrLogicalPortInfo_t *logicalPortInfo,
                                              authmgrClientInfo_t *clientInfo,  AUTHMGR_ATTR_PROCESS_t attrProcess);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_AUTHMGR_RADIUS_H__*/
