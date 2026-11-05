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

#ifndef INCLUDE_MAB_AUTH_H
#define INCLUDE_MAB_AUTH_H

/* USE C Declarations */
#ifdef __cplusplus
  extern "C" {
#endif
#include "comm_mask.h"

/*********************************************************************
 * @purpose  Actions to be performed when sending request to a client
 *
 * @param   logicalPortInfo  @b{(input))  Logical Port Info node
 * @param   bufHandle  @b{(input))  buff handle
 *
 * @returns  SUCCESS
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabClientRequestAction(mabLogicalPortInfo_t *logicalPortInfo,  netBufHandle bufHandle);

/*********************************************************************
 * @purpose  Actions to be performed when sending response to AAA
 *
 * @param   logicalPortInfo  @b{(input))  Logical Port Info node
 * @param   bufHandle  @b{(input))  buff handle
 *
 * @returns  SUCCESS
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabClientResponseAction(mabLogicalPortInfo_t *logicalPortInfo,  netBufHandle bufHandle);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_MAB_AUTH_H */
