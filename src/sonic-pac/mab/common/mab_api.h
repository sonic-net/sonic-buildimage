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


#ifndef INCLUDE_MAB_API_H
#define INCLUDE_MAB_API_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "pacinfra_common.h"
#include "auth_mgr_exports.h"
#include "mab_exports.h"

/*********************************************************************
* @purpose  Set the MAB value on the port.
*
* @param    intIfNum      @b{(input)} internal interface number
* @param    mabEnable   @b{(output)} boolean value determining if MAB 
*                                    has been configured on the port 
*
* @returns   SUCCESS
* @returns   FAILURE
* @results   REQUEST_DENIED     if port control mode of the port is 
*                                 not mac-based  
*
* @comments
*       
* @end
*********************************************************************/
RC_t mabPortMABEnableSet(uint32 intIfNum, uint32 mabEnable);

/*********************************************************************
* @purpose  Get the operational MAB value on the port.
*
* @param    intIfNum      @b{(input)} internal interface number
* @param    mabEnabled    @b{(output)} value determining if MAB 
*                                     has been operationally 
*                                     enabled on the port 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
RC_t mabPortOperMABEnabledGet(uint32 intIfNum, uint32 *mabEnabled);

/*********************************************************************
* @purpose  Set the authentication type on the port to be used by MAB.
*
* @param    intIfNum      @b{(input)} internal interface number
* @param    auth_type     @b{(input)} Authentication type {EAP-MD5 or PAP} 
*
* @returns   SUCCESS            if able to set the auth_type successfully
* @results   REQUEST_DENIED     if MAB is not enabled on that port
* @returns   FAILURE            otherwise
*
* @comments
*       
* @end 
*********************************************************************/
RC_t mabPortMABAuthTypeSet(uint32 intIfNum,
                               AUTHMGR_PORT_MAB_AUTH_TYPE_t auth_type);

/*********************************************************************
* @purpose  Determine if the interface is valid to participate in mab
*
* @param    intIfNum              @b{(input)} internal interface number
*
* @returns   TRUE
* @returns   FALSE
*
* @comments
*       
* @end
*********************************************************************/
 BOOL mabIsValidIntf(uint32 intIfNum);

/*********************************************************************
* @purpose  Determine if the interface type is valid to participate in mab
*
* @param    sysIntfType              @b{(input)} interface type
*
* @returns   TRUE
* @returns   FALSE
*
* @comments
*
* @end
*********************************************************************/
 BOOL mabIsValidIntfType(uint32 sysIntfType);

/*********************************************************************
* @purpose  Set port control mode
*
* @param    intIfNum    @b{(input)} internal interface number
* @param    portControl @b{(input)} port control mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
RC_t mabPortControlModeSet(uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t portControl);

/*********************************************************************
* @purpose  Set host control mode
*
* @param    intIfNum    @b{(input)} internal interface number
* @param    hostControl @b{(input)} host control mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
RC_t mabPortControlHostModeSet(uint32 intIfNum,  AUTHMGR_HOST_CONTROL_t hostMode);

/*********************************************************************
  * @purpose  Handle Auth Manager event
  *
  * @param    intIfNum    @b{(input)} internal interface number
  * @param    event       @b{(input)} event
  * @param    macAddr     @b{(input)} client mac address
  *
  * @returns   SUCCESS
  * @returns   FAILURE
  *
  * @comments
  *
  * @end
  *********************************************************************/
RC_t mabClientEventUpdate(uint32 intIfNum, uint32 event,  enetMacAddr_t *macAddr);

/*********************************************************************
* @purpose  Return Internal Interface Number of the first valid interface for
*           mab.
*
* @param    pFirstIntIfNum @b{(output)}  pointer to first internal interface number
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t mabFirstValidIntfNumber(uint32 *pFirstIntIfNum);

/*********************************************************************
* @purpose  Return Internal Interface Number of next valid interface for
*           mab.
*
* @param    intIfNum  @b{(input)}   Internal Interface Number
* @param    pNextintIfNum @b{(output)}  pointer to Next Internal Interface Number,
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t mabNextValidIntf(uint32 intIfNum, uint32 *pNextIntIfNum);

/*********************************************************************
* @purpose  Update the RADIUS server configuration
*
* @param    add           @b{(input)} whether server should be added or deleted
* @param    radius_type   @b{(input)} radius server type
* @param    serv_addr     @b{(input)} radius server address
* @param    serv_priority @b{(input)} radius server priority
* @param    radius_key    @b{(input)} radius server key
* @param    serv_port     @b{(input)} radius server port
*
* @returns   SUCCESS    values are valid and are updated successfully
* @returns   FAILURE    otherwise
*
* @comments
*
* @end
*********************************************************************/
RC_t mabRadiusServerUpdate(uint32 cmd, const char *radius_type,
                              const char *serv_addr, const char *serv_priority,
                              const char *radius_key, const char *serv_port);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_MAB_API_H */
