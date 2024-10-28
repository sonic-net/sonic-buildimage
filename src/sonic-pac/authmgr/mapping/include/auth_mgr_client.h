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

#ifndef INCLUDE_AUTHMGR_CLIENT_H
#define INCLUDE_AUTHMGR_CLIENT_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "comm_mask.h"

/*********************************************************************
 * @purpose  Set the authmgr physical port authorization status
 *
 * @param    intIfNum   @b{(input)) internal interface number
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
RC_t authmgrIhPhysicalPortStatusSet(uint32 intIfNum, 
                                 AUTHMGR_PORT_STATUS_t portStatus);

/*********************************************************************
 * @purpose  Set the authmgr physical port authorization status
 *
* @param    intIfNum   @b{(input)}  internal interface number
* @param    macAddr    @b{(input)}  MAC address of authorized client
* @param    vlanId     @b{(input)}  set to non-zero value to assign this client to a VLAN
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 * @returns   ERROR
 *
 * @comments none
 *
 * @end
 *********************************************************************/
RC_t authmgrAuthenticatedClientAdd(uint32 physPort, 
                                     enetMacAddr_t macAddr,
                                     ushort16 vlanId,
                                     ushort16 blockVlanId);

/*********************************************************************
 * @purpose  Set the authmgr physical port authorization status
 *
* @param    intIfNum   @b{(input)}  internal interface number
* @param    macAddr    @b{(input)}  MAC address of authorized client
* @param    vlanId     @b{(input)}  set to non-zero value to assign this client to a VLAN
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 * @returns   ERROR
 *
 * @comments none
 *
 * @end
 *********************************************************************/
RC_t authmgrAuthenticatedClientDelete(uint32 physPort, 
                                     enetMacAddr_t macAddr,
                                     ushort16 vlanId);

/*********************************************************************
* @purpose  Cleanup the client Secure Downloadable ACL info from DB
*
* @param    logicalPortInfo   @b{(input)) client logical port info structure
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrRadiusSecDAclCleanupFromDb (authmgrLogicalPortInfo_t * logicalPortInfo);

/*********************************************************************
 * @purpose function to  cleanup the vlan and other settings 
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
RC_t authmgrClientHwInfoCleanup(authmgrLogicalPortInfo_t *logicalPortInfo);

/*********************************************************************
 * @purpose function to  cleanup the vlan and other settings 
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
RC_t authmgrClientHwInfoAdd(authmgrLogicalPortInfo_t *logicalPortInfo, 
                              enetMacAddr_t macAddr,
                              ushort16 vlanId,
                              ushort16 blockVlanId); 

/*********************************************************************
 * @purpose  Set the authmgr client authorization status
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
RC_t authmgrClientStatusSet(authmgrLogicalPortInfo_t *logicalPortInfo,  AUTHMGR_PORT_STATUS_t portStatus);

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
RC_t authmgrClientInfoCleanup(authmgrLogicalPortInfo_t *logicalPortInfo);

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
RC_t authmgrClientSwInfoCleanup(authmgrLogicalPortInfo_t *logicalPortInfo);

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
RC_t authmgrClientDisconnectAction(authmgrLogicalPortInfo_t *logicalPortInfo);


RC_t authmgrClientInfoCleanupCheck (authmgrClientInfo_t *src,
                                       authmgrClientInfo_t *dst);
RC_t authmgrClientFailTimeoutAction(authmgrLogicalPortInfo_t *logicalPortInfo);

/*********************************************************************
* @purpose utility function to check if the client ckpt params modified 
* @param src
* @param dst
* @return  SUCCESS/ FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrClientHwAddFailPostHwCleanup (authmgrLogicalPortInfo_t *logicalPortInfo,
                                             uint32 mask);


/*********************************************************************
* @purpose function to  check and cleanup authenticated client's params
*
* @param    logicalPortInfo   @b{(input)) logical interface structure 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrAuthenticatedClientCleanupAction (authmgrLogicalPortInfo_t *
                                       logicalPortInfo);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_AUTHMGR_CLIENT_H */
