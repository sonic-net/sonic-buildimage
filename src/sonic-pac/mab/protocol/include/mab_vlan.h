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

#ifndef INCLUDE_MAB_VLAN_H
#define INCLUDE_MAB_VLAN_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "comm_mask.h"
#include "mab_exports.h"

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
RC_t mabVlanPortDeletionValidate(uint32 physPort, uint32 vlanId);


/*********************************************************************
 * @purpose  check if the port can be aquired by mab 
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
RC_t mabVlanPortAcquireCheck(uint32 physPort);

/*********************************************
 * @purpose  Enable  mab vlan to a specified interface
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
RC_t mabVlanDeleteProcess(uint32 vlanId);


RC_t mabVlanPortDeleteProcess(uint32 intIfNum,uint32 vlanId);
RC_t mabVlanPortAddProcess(uint32 intIfNum,uint32 vlanId);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_MAB_VLAN_H */
