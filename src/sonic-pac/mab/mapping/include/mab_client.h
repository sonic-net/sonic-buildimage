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

#ifndef INCLUDE_MAB_CLIENT_H
#define INCLUDE_MAB_CLIENT_H

/* USE C Declarations */
#ifdef __cplusplus
  extern "C" {
#endif

#define MAC_STR_LEN                     17

#include "comm_mask.h"

/*********************************************************************
 * @purpose  Set the mab client authorization status
 *
 * @param    lIntIfNum   @b{(input)) internal interface number
 * @param    portStatus @b{(input)) port authorization status setting
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 * @returns   ERROR
 *
 * @comments none
 *
 * @end
 *********************************************************************/
RC_t mabClientStatusSet(mabLogicalPortInfo_t *logicalPortInfo,  AUTHMGR_PORT_STATUS_t portStatus);

/*********************************************************************
 * @purpose function to  cleanup the client 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabClientInfoCleanup(mabLogicalPortInfo_t *logicalPortInfo);

/*********************************************************************
 * @purpose function to  cleanup the client sw info 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabClientSwInfoCleanup(mabLogicalPortInfo_t *logicalPortInfo);

/*********************************************************************
 * @purpose function to  check and deAllocate the client 
 *
 * @param    intIfNum   @b{(input)) internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t mabClientDisconnectAction(mabLogicalPortInfo_t *logicalPortInfo);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_MAB_CLIENT_H */
