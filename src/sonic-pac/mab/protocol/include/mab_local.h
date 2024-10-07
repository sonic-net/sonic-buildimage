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

#ifndef INCLUDE_MAB_LOCAL_H
#define INCLUDE_MAB_LOCAL_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

extern RC_t mabLocalAuthResponseProcess(mabLogicalPortInfo_t *logicalPortInfo,  netBufHandle bufHandle);
extern RC_t mabLocalAuthMd5ResponseValidate(mabLogicalPortInfo_t *logicalPortInfo,  uchar8 *response);
extern void mabLocalAuthChallengeGenerate( uchar8 *challenge, uint32 challengeLen);
extern void mabLocalMd5Calc( uchar8 *inBuf, uint32 inLen,  uchar8 *outBuf);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_MAB_LOCAL_H */
