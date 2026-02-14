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


#ifndef INCLUDE_AUTHMGR_VLAN_H
#define INCLUDE_AUTHMGR_VLAN_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "comm_mask.h"
#include "auth_mgr_exports.h"
#include "pacinfra_common.h"

typedef struct authmgrVlanInfoEntry_s
{
  authmgrVlanType_t type;
   AUTHMGR_VLAN_ASSIGNED_MODE_t assignmentReason;
}authmgrVlanInfoEntry_t;

typedef struct authmgrVlanStringParams_s
{
  authmgrVlanType_t vlanType;
  authmgrClientType_t clientType;
  uint32 vlanId;
}authmgrVlanStringParams_t;

#define AUTH_MGR_DEFAULT_VLANID   1

/*********************************************************************
 * @purpose  check if the port participation can be removed for a vlan
 *
 * @param    physPort    @b{(input))  Port
 * @param    vlanId      @b{(input)) vlan Id
 *
 * @returns   SUCCESS/ FAILRE
 *
 * @comments
 *
 * @end
 *********************************************************************/

RC_t authmgrVlanPortDeletionValidate(uint32 physPort, uint32 vlanId);


/*********************************************************************
 * @purpose  check if the port can be aquired by authmgr 
 *
 * @param    physPort    @b{(input))  Port
 * @param    vlanId      @b{(input)) vlan Id
 *
 * @returns   SUCCESS/ FAILRE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrVlanPortAcquireCheck(uint32 physPort);

RC_t authmgrVlanTypeInfoGet(authmgrVlanType_t type, authmgrVlanInfoEntry_t *entry);

/*********************************************
 * @purpose  Enable  authmgr vlan to a specified interface
 *
 * @param    intIfNum   @b{(input)) internal interface number
 * @param    guestVlanId @b{(input)} guest vlan id
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrVlanDeleteProcess(uint32 vlanId);


RC_t authmgrVlanPortDeleteProcess(uint32 intIfNum,uint32 vlanId, 
                                     dot1qTaggingMode_t tagging);
RC_t authmgrVlanPortAddProcess(uint32 intIfNum,uint32 vlanId,
                                  dot1qTaggingMode_t tagging);
RC_t authmgrVlanAddProcess(uint32 vlanId);

/********************************************************************
 * @purpose  function to process vlan configuration delete event 
 *
 * @param    vlanId @b{(input)} vlan id
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
 RC_t authmgrVlanConfDeleteProcess (uint32 vlanId);

/********************************************************************
 * @purpose  function to process vlan port configuration delete event 
 *
 * @param    vlanId @b{(input)} vlan id
 * @param    intIfNum @b{(input)} interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrVlanConfPortDeleteProcess (uint32 intIfNum, uint32 vlanId);

/*********************************************************************
* @purpose Set Port PVID
*
* @param    intIfNum  @b{(input)) interface
* @param    pvid   @b{(input)) port default vlan id
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortPvidSet(uint32 intIfNum,  ushort16 pvid);

RC_t authmgrVlanPortParticipationValidate (uint32 physPort, uint32 vlanId);
/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_AUTHMGR_VLAN_H */
