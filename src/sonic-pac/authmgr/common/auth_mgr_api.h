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


#ifndef __AUTH_MGR_API_H_
#define __AUTH_MGR_API_H_

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "auth_mgr_exports.h"
#include "datatypes.h" 


typedef struct authmgrAuthRespParams_s
{
   AUTHMGR_METHOD_t method;
   AUTHMGR_STATUS_t status;
  authmgrClientStatusInfo_t clientParams;
}authmgrAuthRespParams_t;


/*********************************************************************
* @purpose  Get initialize value for a port
*
* @param    intIfNum    @b{(input)} internal interface number
* @param    *initialize @b{(output)} initialize value
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments This value indicates whether a port is being initialized
*           due to a management request
*       
* @end
*********************************************************************/
RC_t authmgrPortInitializeGet(uint32 intIfNum,  BOOL *initialize);

/*********************************************************************
* @purpose  Set initialize value for a port
*
* @param    intIfNum   @b{(input)} internal interface number
* @param    initialize @b{(input)} initialize value
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments This value is set to  TRUE by management in order to force
*           initialization of a port.  It is re-set to  FALSE after
*           initialization has completed.
*       
* @end
*********************************************************************/
RC_t authmgrPortInitializeSet(uint32 intIfNum,  BOOL initialize);

/*********************************************************************
* @purpose  Get auth mgr reauthenticate timer value
*
* @param    intIfNum     @b{(input)} internal interface number
* @param    *timerVal@b{(output)}reauthenticate timer value 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments 
*       
* @end
*********************************************************************/
RC_t authmgrPortAuthRestartTimerGet(uint32 intIfNum, uint32 *timerVal);

/*********************************************************************
* @purpose  Set auth mgr reauthenticate timer value
*
* @param    intIfNum    @b{(input)} internal interface number
* @param    timerval @b{(input)} reauthenticate timer value 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments Authentication restart timer value , for which the port will wait before,
            retstarting authentication when all the authentication methods fail.
*       
* @end
*********************************************************************/
RC_t authmgrPortAuthRestartTimerSet(uint32 intIfNum, uint32 timerVal);

/*********************************************************************
* @purpose  Set auth mgr method or priority 
*
* @param    mode   @b{(input)} order or priority 
* @param    index @b{(input)} position of the method or order 
* @param    method @b{(input)} authentication manager methods,
                               i.e.dot1x/mab/cp
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments Authentication restart timer value , for which the port will wait before,
            retstarting authentication when all the authentication methods fail.
*       
* @end
*********************************************************************/
RC_t authmgrPortAuthMethodSet( AUTHMGR_METHOD_TYPE_t mode, uint32 intIfNum,
        uint32 index,  AUTHMGR_METHOD_t method);
/*********************************************************************
* @purpose  Get auth mgr method or priority 
*
* @param    mode   @b{(input)} order or priority 
* @param    index @b{(input)} position of the method or order 
* @param    *method @b{(output)} authentication manager methods,
                               i.e.dot1x/mab/cp
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments Authentication restart timer value , for which the port will wait before,
            retstarting authentication when all the authentication methods fail.
*       
* @end
*********************************************************************/
RC_t authmgrPortAuthMethodGet( AUTHMGR_METHOD_TYPE_t mode, uint32 intIfNum,
        uint32 index,  AUTHMGR_METHOD_t *method);

/*********************************************************************
 * @purpose  Clear authmgr stats for specified port
 *
 * @param    intIfNum     @b{(input)} internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrPortStatsClear(uint32 intIfNum);
/*********************************************************************
 * @purpose  Get authmgr stats for specified port
 *
 * @param    intIfNum     @b{(input)} internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrPortStatsGet(uint32 intIfNum,  AUTHMGR_STATS_TYPE_t method, uint32 *stats);

/*********************************************************************
* @purpose  Return Internal Interface Number of next valid interface for
*           authmgr.
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
RC_t authmgrNextValidIntf(uint32 intIfNum, uint32 *pNextIntIfNum);
/*********************************************************************
* @purpose  Return Internal Interface Number of the first valid interface for
*           authmgr.
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
RC_t authmgrFirstValidIntfNumber(uint32 *pFirstIntIfNum);

/*********************************************************************
* @purpose  Returns the first logical port for the physcial interface
*
* @param    intIfNum    @b{(input)} internal interface number
* @param    lIntIfNum    @b((output)) the logical interface
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortFirstGet(uint32 intIfNum,uint32 *lIntIfNum);

/*********************************************************************
* @purpose  Returns the first logical port for the physcial interface
*
* @param    lIntIfNum    @b((input)) the logical interface
* @param    nextIntf    @b{(ouput)}  the next interface
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortNextGet(uint32 lIntIfNum,uint32 *nextIntf);

/*********************************************************************
* @purpose  Returns the User Name for the  logical interface
*
* @param    lIntIfNum    @b((input)) the specified interface
* @param    userName     @b((output)) user name for the logical interface
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortUserNameGet (uint32 lIntIfNum,
                                        uchar8 *userName);

/*********************************************************************
* @purpose  Returns the accouting session Id for the logical interface
*
* @param    lIntIfNum            @b((input)) the specified interface
* @param    acctSessionIdStr     @b((output)) Accouting Session Id
*                                  for the logical interface
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortAcctSessionIdGet (uint32 lIntIfNum,
                                             uchar8 *acctSessionIdStr);

/*********************************************************************
* @purpose  Returns the client Mac address for the logical interface
*
* @param    lIntIfNum    @b((input)) the specified interface
* @param    macAddr      @b((output)) Mac Address of the supplicant
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortClientMacAddrGet(uint32 lIntIfNum,
                                              uchar8 *macAddr);


/*********************************************************************
* @purpose  Returns the physical port corresponding to the logical interface
*
* @param    lIntIfNum    @b((input)) the logical interface
* @param    physport    @b{(ouput)}  the physical interface
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrPhysicalPortGet(uint32 lIntIfNum,uint32 *physPort);

/*********************************************************************
*
* @purpose  Update the status and other information of the client 
            from the authentication  method to Auth Mgr.
*
* @param    uint32        intIfNum  @b((input)) Internal interface number
* @param    method @b{(input)} authentication manager methods,
                               i.e.dot1x/mab/cp
* @param    status @b{(input)} authentication status,
                               i.e start/success/fail/timeout.
* @param    clientParams @b{(input)} client status event related information
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes   This API is called from DOT1X and CP when they are starting the authentication 
           and also when the method is success/failure/timedout
*
* @end
*********************************************************************/
RC_t authmgrPortClientAuthStatusUpdate(uint32 intIfNum, 
                                     AUTHMGR_METHOD_t method,
                                     AUTHMGR_STATUS_t  status,
                                    void *clientParams);


/******************************************************************************
 * @purpose  Gets the next History Log interface index
 *
 * @param    intIfNum   @b{(input/output)} Physical Interface Number
 *
 * @returns   SUCCESS
 * @returns   FAILURE  If there are no entries
 *
 * @notes
 *
 * @end
 *******************************************************************************/
RC_t authmgrAuthHistoryLogIfIndexNextGet(uint32 *intIfNum);


/******************************************************************************
 * @purpose  Gets the next History Log entry indexs
 *
 * @param    intIfNum   @b{(input/output)} Physical Interface Number
 * @param    entryIndex @b{(input/output)} EntryIndex
 *
 * @returns   SUCCESS
 * @returns   FAILURE  If there are no entries
 *
 * @notes
 *
 * @end
 *******************************************************************************/
RC_t authmgrAuthHistoryLogIndexNextGet(uint32 *intIfNum,
                                        uint32 *entryIndex);
/******************************************************************************
 * @purpose  Gets the next History Log entry indexs in reverse order
 *
 * @param    intIfNum   @b{(input/output)} Physical Interface Number
 * @param    entryIndex @b{(input/output)} EntryIndex
 *
 * @returns   SUCCESS
 * @returns   FAILURE  If there are no entries
 *
 * @notes
 *
 * @end
 *******************************************************************************/
RC_t authmgrAuthHistoryLogReverseIndexNextGet(uint32 *intIfNum,
                                               uint32 *entryIndex);

/*********************************************************************
 * @purpose  Purge all authmgr auth history log entries for the given
 *           interface
 *
 * @param    intIfNum   @b{(input)} Physical Interface Number
 *
 * @returns  SUCCESS
 * @returns  FAILURE
 *
 * @comments
 *
 * @notes
 * @end
 *
 **********************************************************************/
RC_t authmgrAuthHistoryLogInterfacePurge(uint32 intIfNum);

/*********************************************************************
 * @purpose  Purge all authmgr auth history log entries
 *
 * @param    void
 *
 * @returns  SUCCESS
 * @returns  FAILURE
 *
 * @comments
 *
 * @notes
 *
 * @end
 *
 **********************************************************************/
RC_t authmgrAuthHistoryLogPurgeAll();


/*********************************************************************
 * @purpose  Get the timestamp from the authmgr Auth History table
 *          
 * @param    intIfNum   @b{(input)} Physical Interface Number
 * @param    entryIndex @b{(input)} EntryIndex
 * @param    pTimeStamp @b{(output)} reference to the Reason Code
 *
 * @returns  SUCCESS  
 * @returns  FAILURE
 *
 * @comments
 *
 * @notes 
 *
 * @end
 *
 *********************************************************************/
RC_t authmgrAuthHistoryLogTimestampGet(uint32 intIfNum,
                                        uint32 entryIndex,
                                        uint32 *pTimeStamp);

/*********************************************************************
 * @purpose  Get the VlanId from the authmgr Auth History table
 *
 * @param    intIfNum   @b{(input)} Physical Interface Number
 * @param    entryIndex @b{(input)} EntryIndex
 * @param    pVlanId    @b{(output)} reference to the VLANID
 *
 * @returns  SUCCESS
 * @returns  FAILURE
 *
 * @comments
 *
 * @notes
 *
 * @end
 *
 *********************************************************************/
RC_t authmgrAuthHistoryLogVlanIdGet(uint32 intIfNum,
                                     uint32 entryIndex,
                                      ushort16 *pVlanId);

/*********************************************************************
 * @purpose  Get the VlanId from the authmgr Auth History table
 *          
 * @param    intIfNum   @b{(input)} Physical Interface Number
 * @param    entryIndex @b{(input)} EntryIndex
 * @param    pMethod    @b{(output)} reference to the auth Method 
 *
 * @returns  SUCCESS  
 * @returns  FAILURE
 *
 * @comments
 *
 * @notes 
 *
 * @end
 *
 *********************************************************************/
RC_t authmgrAuthHistoryLogMethodGet(uint32 intIfNum,
                                     uint32 entryIndex,
                                      AUTHMGR_METHOD_t *pMethod); 

/*********************************************************************
 * @purpose  Get the reasonCode from the authmgr Auth History table
 *          
 * @param    intIfNum   @b{(input)} Physical Interface Number
 * @param    entryIndex @b{(input)} EntryIndex
 * @param    pReasonCode @b{(output)} Reference to the Reason Code
 *
 * @returns  SUCCESS  
 * @returns  FAILURE
 *
 * @comments
 *
 * @notes 
 *
 * @end
 *
 *********************************************************************/
RC_t authmgrAuthHistoryLogReasonCodeGet(uint32 intIfNum,
                                         uint32 entryIndex,
                                         uint32 *pReasonCode); 

/*********************************************************************
 * @purpose  Get the accessStatus from the authmgr Auth History table
 *          
 * @param    intIfNum   @b{(input)} Physical Interface Number
 * @param    entryIndex @b{(input)} EntryIndex
 * @param    pStatus    @b{(output)} Reference to the Access Status
 *
 * @returns  SUCCESS  
 * @returns  FAILURE
 *
 * @comments
 *
 * @notes 
 *
 * @end
 *
 *********************************************************************/
RC_t authmgrAuthHistoryLogAuthStatusGet(uint32 intIfNum,
                                           uint32 entryIndex,
                                           uint32 *pStatus); 


/*********************************************************************
 * @purpose  Get the authmgr authentication Status from the authmgr Auth 
 *           History table
 *          
 * @param    intIfNum   @b{(input)} Physical Interface Number
 * @param    entryIndex @b{(input)} EntryIndex
 * @param    pStatus    @b{(output)} Reference to the Auth Status
 *
 * @returns  SUCCESS  
 * @returns  FAILURE
 *
 * @comments
 *
 * @notes 
 *
 * @end
 *
 *********************************************************************/
RC_t authmgrAuthHistoryLogAuthStatusGet(uint32 intIfNum,
                                         uint32 entryIndex,
                                         uint32 *pStatus); 

/*********************************************************************
 * @purpose  Get the supplicant Mac Address from the authmgr Auth History table
 *          
 * @param    intIfNum   @b{(input)} Physical Interface Number
 * @param    entryIndex @b{(input)} EntryIndex
 * @param    pMacAddress @b{(output)} Reference to the Mac Address
 *
 * @returns  SUCCESS  
 * @returns  FAILURE
 *
 * @comments
 *
 * @notes 
 *
 * @end
 *
 *********************************************************************/
RC_t authmgrAuthHistoryLogClientMacAddressGet(uint32        intIfNum,
                                                 uint32        entryIndex,
                                                  enetMacAddr_t *pMacAddress); 

/*********************************************************************
 * @purpose  Get the FilterID for the Authmgr Radius Accept Packet
 *           located in the authmgr Auth History table
 *
 * @param    intIfNum   @b{(input)} Physical Interface Number
 * @param    entryIndex @b{(input)} EntryIndex
 * @param    pFilterId  @b{(output)} Reference to the filter Id
 * @param    pFilterLen @b{(input/output)} Reference to filter Length
 *
 * @returns  SUCCESS
 * @returns  FAILURE
 *
 * @comments
 *
 * @notes
 *
 * @end
 *
 *********************************************************************/
RC_t authmgrAuthHistoryLogFilterIdGet (uint32 intIfNum,
                                          uint32 entryIndex,
                                           uchar8 *pFilterIdName,
                                          uint32 *pFilterLen);

/*********************************************************************
 * @purpose  Check if the authmgr auth history log entry exists in History
 *           table
 *          
 * @param    intIfNum   @b{(input)} Physical Interface Number
 * @param    entryIndex @b{(input)} EntryIndex
 *
 * @returns  SUCCESS  
 * @returns  FAILURE
 *
 * @comments
 *
 * @notes 
 *
 * @end
 *
 *********************************************************************/
RC_t authmgrAuthHistoryLogEntryIsValid(uint32 intIfNum,
                                        uint32 entryIndex);
 /*********************************************************************
 * @purpose  Return Reason in String format for the given entry
 *          
 * @param    intIfNum   (input)  - Interface
 *           entryIndex (input)  - Entry Index
 *           reasonCode (input)  - Reason Code
 *           strReason  (output) - Reason in String format
 *           strLen     (input/output) - Length of Reason String
 *
 * @returns  SUCCESS/ FAILURE
 *
 * @comments
 *
 * @notes 
 *
 * @end
 *
 *********************************************************************/
RC_t authmgrAuthHistoryLogReasonStringGet(uint32  intIfNum,
                                           uint32  entryIndex,
                                           uint32  reasonCode,
                                            char8  *strReason,
                                           uint32 *strLen);

/*********************************************************************
 * @purpose  Verify specified config interface index is valid
 *
 * @param    intIfNum    @b{(input)}  Internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrIntfIndexGet(uint32 intIfNum);

/*********************************************************************
 * @purpose  Determine next sequential auth Mgr config interface index
 *
 * @param    intIfNum    @b{(input)}  Internal interface number to begin search
 * @param    *pNext      @b{(output)} Ptr to next internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrIntfIndexGetNext(uint32 intIfNum, uint32 *pNext);

/*********************************************************************
 * @purpose  Verify specified  index exists
 *
 * @param    index     @b{(input)} index of the config array 
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments Auth Mgr expects the index to come to the api with incremented by 1.
             In the API we reduce the  index by 1 as the data structure is array.
 *
 * @end
 *********************************************************************/
RC_t authmgrMethodIndexGet(uint32 index);

/*********************************************************************
 * @purpose  Determine next sequential  index
 *
 * @param    index     @b{(input)} index of the config array 
 * @param    *pNext      @b{(output)} Ptr to next priority
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrIndexGetNext(uint32 index, uint32 *pNext);

/*********************************************************************
 * @purpose  Determine next sequential auth Mgr config interface index
 *
 * @param    intIfNum    @b{(input)}  Internal interface number to begin search
 * @param    *pNext      @b{(output)} Ptr to next internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrEntryIndexGet(uint32 intIfNum, uint32 index);
/*********************************************************************
 * @purpose  Determine next sequential auth Mgr config interface index
 *
 * @param    intIfNum    @b{(input)}  Internal interface number to begin search
 * @param    *pNext      @b{(output)} Ptr to next internal interface number
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrEntryIndexGetNext(uint32 intIfNum, uint32 *pNextNum,
                                  uint32 index, uint32 *pNextIndex);

/*********************************************************************
 * @purpose  chechks if method order config is valid.
 *
 * @param    intIfNum    @b{(input)} internal interface number
 * @param    method @b{(input)} authentication manager methods,
 i.e.dot1x/mab/cp
 *
 * @returns   TRUE
 * @returns   FALSE
 *
 * @comments  This API should only be called from the methods DOT1X and captive portal
              applications only. When 8021x or CP  receives a PDU or packet then they 
              query Auth mgr if the can start authentication process.This API returns if the
              same can start the authentication process.
 *
 * @end
 *********************************************************************/
RC_t authmgrPortAuthMethodStartAuthAllowed(uint32 intIfNum, AUTHMGR_METHOD_t method); 
/*********************************************************************
 * @purpose  gets the authenticated method or currently running authenticated method for the client 
 *
 * @param    intIfNum    @b{(input)} internal interface number
 * @param    mac_addr    @b{(input)}client's mac address 
 * @param    *method    @b{(input)}reference to the method 
 *
 * @returns   FALSE_
 * @returns   TRUE
 *
 * @comments  This API should only be called from the methods DOT1X and captive portal
 *
 * @end
 *********************************************************************/
RC_t authmgrClientAuthenticatedMethodGet(uint32 intIfNum,  enetMacAddr_t mac_addr,  AUTHMGR_METHOD_t *method );

/*********************************************************************
 * @purpose  chechs if method is Enabled.
 *
 * @param    intIfNum    @b{(input)} internal interface number
 * @param    method @b{(input)} authentication method which is being checked,
 i.e.dot1x/mab/cp
 * @param    *entryIndex    @b{(outout)}reference to the entry index
 *
 * @returns   FALSE
 * @returns   TRUE
 *
 * @comments  This API should only be called from the methods DOT1X and captive portal
              applications only. 
 *
 * @end
 *********************************************************************/

 BOOL authmgrIsMethodEnabled(uint32 intIfNum,  AUTHMGR_METHOD_t method, uint32 *entryIndex);
/*********************************************************************
 * @purpose  Determine if the interface is valid for auth mgr
 *
 * @param    intIfNum  @b{(input)} internal interface number
 *
 * @returns   TRUE
 * @returns   FALSE
 *
 * @comments none
 *
 * @end
 *********************************************************************/
 BOOL authmgrIsValidIntf(uint32 intIfNum);

/*********************************************************************
* @purpose  Get number of attempts for the method 
*
* @param    intIfNum     @b{(input)} internal interface number
* @param    method       @b{(input)} method for which the attempts are requested 
* @param    *numAttempts @b{(output)} number of attempts 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
RC_t authmgrPortAttemptsGet(uint32 intIfNum,  AUTHMGR_METHOD_t method, uint32 *numAttempts);

/*********************************************************************
* @purpose  Get number of failed attempts for the method 
*
* @param    intIfNum     @b{(input)} internal interface number
* @param    method       @b{(input)} method for which the attempts are requested 
* @param    *numAttempts @b{(output)} number of attempts 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
RC_t authmgrPortFailedAttemptsGet(uint32 intIfNum,  AUTHMGR_METHOD_t method, uint32 *numAttempts);


/*********************************************************************
 * @purpose  Determine next sequential  index
 *
 * @param    index     @b{(input)} index of the config array 
 * @param    *pNext      @b{(output)} Ptr to next priority
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrMethodIndexGetNext(uint32 index, uint32 *pNext);

/*********************************************************************
 * @purpose  Get the number of clients authenticated.
 *          
 * @param    intIfNum @b((input)) interface number  
 * @param    pCount @b((output)) ptr to the number of clients  
 *
 * @returns  SUCCESS  
 * @returns  FAILURE
 *
 * @comments
 *
 * @notes 
 * @notes 
 *
 * @end
 *
 *********************************************************************/
RC_t authmgrNumClientsGet(uint32 intIfNum, uint32 *pCount);

/*********************************************************************
 * @purpose  Get the number of clients authenticated.
 *          
 * @param    intIfNum @b((input)) interface number  
* @param    mode   @b{(input)} order or priority 
* @param    index @b{(input)} position of the method or order 
* @param    *method @b{(output)} authentication manager methods,
                               i.e.dot1x/mab/cp
 *
 * @returns  SUCCESS  
 * @returns  FAILURE
 *
 * @comments
 *
 * @notes 
 * @notes 
 *
 * @end
 *
 *********************************************************************/
RC_t authmgrEnabledMethodGet( AUTHMGR_METHOD_TYPE_t mode, uint32 intIfNum,
        uint32 index,  AUTHMGR_METHOD_t *method);

/*********************************************************************
* @purpose  Returns the client authenticated Method for the logical interface
*
* @param    lIntIfNum    @b((input)) the specified interface
* @param    method      @b((output)) authenticating method
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortClientAuthenticatedMethodGet(uint32 lIntIfNum,
                                                        AUTHMGR_METHOD_t *method);


/*********************************************************************
* @purpose  Returns the client auth state for the logical interface
*
* @param    lIntIfNum    @b((input))  the specified interface
* @param    state        @b((output)) authenticating state 
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortClientAuthStateGet(uint32 lIntIfNum,
                                             AUTHMGR_STATES_t *state);

/*********************************************************************
* @purpose  Returns the client reauth state for the logical interface
*
* @param    lIntIfNum    @b((input))  the specified interface
* @param    state        @b((output)) reauthenticating state 
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortClientReauthStateGet(uint32 lIntIfNum, 
                                                BOOL *state);

/*********************************************************************
* @purpose  Returns the client auth status for the logical interface
*
* @param    lIntIfNum    @b((input)) the specified interface
* @param    status       @b((output)) authenticated status 
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortClientAuthStatusGet(uint32 lIntIfNum,
                                              AUTHMGR_PORT_STATUS_t *status);

/*********************************************************************
*
* @purpose  Update the status and other information of the client 
            from the authentication  method to Auth Mgr.
*
* @param    uint32        intIfNum  @b((input)) Internal interface number
* @param    method @b{(input)} authentication manager methods,
                               i.e.dot1x/mab/cp
* @param    status @b{(input)}  TRUE/ FALSE 
                               i.e start/success/fail/timeout.
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes   This API is called from DOT1X and CP when the feature is enabled or disabled.
           In case of Global mode changes, the interface will come as  ALL_INTERFACES
           and also when the method is success/failure/timedout
*
* @end
*********************************************************************/
RC_t authmgrMethodAdminModeCallback( uint32 intIfNum, 
                                     AUTHMGR_METHOD_t method,
                                     BOOL  status);
/*********************************************************************
* @purpose  Returns the logical port for the next client Mac Address
*           in the mac address database
*
* @param    mac_addr    @b{(input)} supplicant mac address to be searched
* @param    lIntIfNum    @b((output)) the logical interface
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments for SNMP
*
* @end
*********************************************************************/
RC_t authmgrClientMacAddressNextGet( enetMacAddr_t *macAddr,uint32 *lIntIfNum);
/*********************************************************************
* @purpose  Returns the logical port for the corresponding supplicant Mac Address
*
* @param    mac_addr    @b{(input)} supplicant mac address to be searched
* @param    lIntIfNum    @b((output)) the logical interface
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments for SNMP
*
* @end
*********************************************************************/
RC_t authmgrClientMacAddressGet( enetMacAddr_t *macAddr,uint32 *lIntIfNum);
/*********************************************************************
* @purpose  function to validate of Mab is enabled before 8021x 
* intIfNum : interface number
*
* @returns   SUCCESS  if MAB is enabled before 802.1X
* @returns   FAILURE  
*
* @comments 
*
* @end
*********************************************************************/
 BOOL authmgrIsMabEnabledPriorToDot1x(uint32 intIfNum);
/**************************************************************************
* @purpose   Wrapper function to authentication manager API 
*
* @returns    SUCCESS
* @returns    FAILURE
*
* @comments  
*         
* @end
*************************************************************************/
RC_t authmgrEnabledMethodGetNext(uint32 intIfNum, uint32 method, uint32 *nextMethod);

/*********************************************************************
 * @purpose  Get the Authentication Method string for given method type
 *
 * @param    method  @b{(input)} Authentication Method type
 *
 * @returns  Method name String for given input method
 *
 * @comments none
 *
 * @end
 *********************************************************************/
 uchar8 *authmgrMethodTypeToName( AUTHMGR_METHOD_t method);

/*********************************************************************
* @purpose  Get port control mode
*
* @param    intIfNum     @b{(input)} internal interface number
* @param    *portControl @b{(output)} port control mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
extern RC_t authmgrPortControlModeGet(uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t *portControl);

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
extern RC_t authmgrPortControlModeSet(uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t portControl);

/*********************************************************************
* @purpose  Get Authentiation Server timeout value
*
* @param    intIfNum       @b{(input)} internal interface number
* @param    *serverTimeout @b{(output)} Authentication Server timeout
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments The serverTimeout is the initialization value for aWhile,
*           which is a timer used by the Authenticator state machine
*           to time out the Authentiation Server.
*       
* @end
*********************************************************************/
extern RC_t authmgrPortServerTimeoutGet(uint32 intIfNum, uint32 *serverTimeout);

/*********************************************************************
* @purpose  Set Authentiation Server timeout value
*
* @param    intIfNum      @b{(input)} internal interface number
* @param    serverTimeout @b{(input)} Authentication Server timeout
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments The serverTimeout is the initialization value for aWhile,
*           which is a timer used by the Authenticator state machine
*           to time out the Authentiation Server.
*       
* @end
*********************************************************************/
extern RC_t authmgrPortServerTimeoutSet(uint32 intIfNum, uint32 serverTimeout);
/*********************************************************************
*
* @purpose  Callback from DTL informing about an unauthorized address
*
* @param    uint32        intIfNum  @b((input)) Internal interface number
* @param     enetMacAddr_t macAddr   @b((output)) MAC address
* @param     ushort16      vlanId    @b((output)) VLAN ID
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes    none
*
* @end
*********************************************************************/
extern RC_t authmgrUnauthAddrCallBack( uint32 intIfNum,  enetMacAddr_t macAddr,  ushort16 vlanId );


/*********************************************************************
* @purpose  Get host control mode
*
* @param    intIfNum     @b{(input)} internal interface number
* @param    *hostControl @b{(output)} host control mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
extern RC_t authmgrHostControlModeGet(uint32 intIfNum,  AUTHMGR_HOST_CONTROL_t *hostControl);

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
RC_t authmgrHostControlModeSet(uint32 intIfNum,  AUTHMGR_HOST_CONTROL_t hostMode);

/*********************************************************************
  * @purpose  Determine if the interface type is valid to participate in authmgr
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
 BOOL authmgrIsValidIntfType(uint32 sysIntfType);


/*********************************************************************
* @purpose  Determine if a client is authenticated on an interface
*
* @param    intIfNum      @b{(input)} internal interface number
* @param    macAddr       @b{(input)} client's MAC address
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
 BOOL authmgrPortClientAuthenticationGet(uint32 intIfNum,  uchar8 *macAddr);

/*********************************************************************
* @purpose  Get operational value of controlled directions
*
* @param    intIfNum      @b{(input)} internal interface number
* @param    *portStatus   @b{(output)} port authentication status
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
extern RC_t authmgrPortAuthControlledPortStatusGet(uint32 intIfNum,
                                                     AUTHMGR_PORT_STATUS_t *portStatus);

/*********************************************************************
* @purpose  Get the port autherization status.
*
* @param    intIfNum               @b{(input)} internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
extern RC_t authmgrPortIsAuthorized(uint32 intIfNum);

/*********************************************************************
 * @purpose  Get the Authentication Method string for given method type
 *
 * @param    method  @b{(input)} Authentication Method type 
 *
 * @returns  Method name String for given input method
 *
 * @comments none
 *
 * @end
 *********************************************************************/
extern  uchar8 *authmgrMethodTypeToName( AUTHMGR_METHOD_t method);

/*********************************************************************
 * @purpose function to get max users 
 *
 * @param    hostMode   @b{(input))  hostmode 
 * @param    *appyPolicy  @b{(input)) bool value 
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrMaxUsersGet(uint32 intIfNum, uint32 *maxUsers);

/*********************************************************************
* @purpose  Set administrative mode setting for authmgr Vlan Assignment
*
* @param    mode @b{(input)} radius vlan assignment mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments    
*       
* @end
*********************************************************************/
RC_t authmgrVlanAssignmentModeSet (uint32 mode);

/*********************************************************************
* @purpose  Get administrative mode setting for authmgr Vlan Assignment
*
* @param    mode @b{(input)} radius vlan assignment mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments    
*       
* @end
*********************************************************************/
RC_t authmgrVlanAssignmentModeGet(uint32 *mode);

/*********************************************************************
* @purpose  Set the Guest Vlan Id for the port.
*
* @param    intIfNum      @b{(input)} internal interface number
* @param    *guestVlanId       @b{(output)} guest vlan Id
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrAdvancedGuestPortsCfgSet(uint32 intIfNum,uint32 guestVlanId);

/*********************************************************************
* @purpose  Set the Guest Vlan Id for the port.
*
* @param    intIfNum      @b{(input)} internal interface number
* @param    *guestVlanId  @b{(output)} guest vlan Id
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrAdvancedGuestPortCfgGet(uint32 intIfNum,uint32 *guestVlanId);

/*********************************************************************
* @purpose  Set the Guest Vlan Period for the port.
*
* @param    intIfNum      @b{(input)} internal interface number
* @param    guestVlanPeriod  @b{(output)} guest vlan Period
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrAdvancedGuestVlanPeriodSet(uint32 intIfNum,uint32 guestVlanPeriod);


/*********************************************************************
* @purpose  Set max users value
*
* @param    intIfNum    @b{(input)} internal interface number
* @param    maxUsers @b{(input)} max users
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments The maxUsers is the maximum number of hosts that can be 
*           authenticated on a port using mac based authentication
*       
* @end
*********************************************************************/
RC_t authmgrPortMaxUsersSet(uint32 intIfNum, uint32 maxUsers);

/*********************************************************************
* @purpose  Get max users value
*
* @param    intIfNum  @b{(input)} internal interface number
* @param    *maxUsers @b{(output)} max users per port
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments The maxUsers is the maximum number of hosts that can be 
*           authenticated on a port using mac based authentication
*       
* @end
*********************************************************************/
RC_t authmgrPortMaxUsersGet(uint32 intIfNum, uint32 *maxUsers);
/*********************************************************************
* @purpose  Returns the session timeout value for the  logical interface
*
* @param    lIntIfNum  @b((input)) the specified interface
* @param    sessiontimeout  @b((output)) session timeout for the logical interface
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortSessionTimeoutGet(uint32 lIntIfNum,
                                            uint32 *session_timeout);

/*********************************************************************
* @purpose  To get the time left for the session termination action
*           to occur for the logical interface
*
* @param    lIntIfNum  @b((input))  Logical interface number
* @param    timeLeft   @b((output)) Pointer to store the left out time
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortTimeLeftForTerminateActionGet(uint32 lIntIfNum,
                                                        uint32 *timeLeft);

/*********************************************************************
* @purpose  Returns the termination Action for the  logical interface
*
* @param    lIntIfNum  @b((input)) the specified interface
* @param    terminationAction  @b((output)) termination Action for the logical interface
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortTerminationActionGet(uint32 lIntIfNum,
                                    uint32 *terminationAction);

/*********************************************************************
* @purpose  Check if the vlan is assigned to any client or port
*
* @param    phyPort       @b{(input)} physical port
* @param    vlanId        @b{(input)} vlanId
*
* @returns   TRUE
* @returns   FALSE
*
* @comments
*
* @end
*********************************************************************/
 BOOL authmgrVlanAssignedCheck (uint32 phyPort, uint32 vlanId);


/*********************************************************************
* @purpose  Get reauthentication value for a port
*
* @param    intIfNum        @b{(input)} internal interface number
* @param    *reauthenticate @b{(output)} reauthentication value
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments This value indicates whether a port is being reauthenticated
*           due to a management request
*       
* @end
*********************************************************************/
extern RC_t authmgrPortReauthenticateGet(uint32 intIfNum,  BOOL *reauthenticate);

/*********************************************************************
* @purpose  Set reauthentication value for a port
*
* @param    intIfNum       @b{(input)} internal interface number
* @param    reauthenticate @b{(input)} reauthentication value
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments This value is set to  TRUE by management in order to force
*           reauthentication of a port.  It is re-set to  FALSE after
*           reauthentication has completed.
*       
* @end
*********************************************************************/
extern RC_t authmgrPortReauthenticateSet(uint32 intIfNum,  BOOL reauthenticate);


/*********************************************************************
* @purpose  Get the Reauthentication period
*
* @param    intIfNum       @b{(input)}  internal interface number
* @param    *reAuthPeriod  @b{(output)} reauthentication period
* @param    serverConfig   @b{(output)} get reauthentication period
*                                       from server option
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments The reAuthPeriod is the initialization value for reAuthWhen,
*           which is a timer used by the Authenticator state machine to
*           determine when reauthentication of the Supplicant takes place.
*
* @end
*********************************************************************/
extern RC_t authmgrPortReAuthPeriodGet(uint32 intIfNum,
                                          uint32 *reAuthPeriod,
                                           BOOL *serverConfig);

/*********************************************************************
* @purpose  Set the Reauthentication period
*
* @param    intIfNum      @b{(input)} internal interface number
* @param    reAuthPeriod  @b{(input)} reauthentication period
* @param    serverConfig  @b{(input)} set option to get reauthentication
*                                     period from server option
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments The reAuthPeriod is the initialization value for reAuthWhen,
*           which is a timer used by the Authenticator state machine to
*           determine when reauthentication of the Supplicant takes place.
*       
* @end
*********************************************************************/
extern RC_t authmgrPortReAuthPeriodSet(uint32 intIfNum, 
                                          uint32 reAuthPeriod, 
                                           BOOL serverConfig);

/*********************************************************************
* @purpose  Get the Reauthentication mode
*
* @param    intIfNum        @b{(input)} internal interface number
* @param    *reAuthEnabled  @b{(output)} reauthentication mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments The reAuthEnabled mode determines whether reauthentication
*           of the Supplicant takes place.
*       
* @end
*********************************************************************/
extern RC_t authmgrPortReAuthEnabledGet(uint32 intIfNum,  BOOL *reAuthEnabled);

/*********************************************************************
* @purpose  Set the Reauthentication mode
*
* @param    intIfNum       @b{(input)} internal interface number
* @param    reAuthEnabled  @b{(input)} reauthentication mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments The reAuthEnabled mode determines whether reauthentication
*           of the Supplicant takes place.
*       
* @end
*********************************************************************/
extern RC_t authmgrPortReAuthEnabledSet(uint32 intIfNum,  BOOL reAuthEnabled);
/*********************************************************************
* @purpose  Get port operational mode
*
* @param    intIfNum     @b{(input)} internal interface number
* @param    *portMode    @b{(output)} port operational mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
extern RC_t authmgrPortOperControlModeGet(uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t *portMode);


/*********************************************************************
* @purpose  Set maximum number of times authentication
*           may be reattempted by the user on the port.
*
* @param    intIfNum @b{(input)} internal interface number
* @param    maxReq   @b{(input)} maximum request value
*
* @returns   SUCCESS
* @returns   FAILURE
* @returns   NOT_SUPPORTED
*
* @end
*********************************************************************/
RC_t authmgrPortMaxAuthAttemptsSet(uint32 intIfNum, uint32 maxAuthAttempts);

/*********************************************************************
* @purpose  Get maximum number of times authentication 
*           may be reattempted by the user on the port.
*
* @param    intIfNum @b{(input)} internal interface number
* @param    *maxReq  @b{(output)} maximum request value
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*********************************************************************/
RC_t authmgrPortMaxAuthAttemptsGet(uint32 intIfNum, uint32 *maxAuthAttempts);

/*********************************************************************
* @purpose  Returns the Supplicant Mac address for the logical interface
*
* @param    lIntIfNum    @b((input)) the specified interface
* @param    macAddr      @b((output)) Mac Address of the supplicant
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortSupplicantMacAddrGet(uint32 lIntIfNum,
                                              uchar8 *macAddr);
/*********************************************************************
* @purpose  Get the port acquire status.
*
* @param    intIfNum               @b{(input)} internal interface number
*
* @returns   TRUE if yes
* @returns   FALSE otherwise
*
* @comments
*
* @end
*********************************************************************/
 BOOL authmgrPortIsAcquired(uint32 intIfNum);

/*********************************************************************
 *
 * @purpose Register routines to be called by Auth Manager for various events.
 *
 * @param method @b((input)) authentication protocol
 * @param *notify @b((input)) pointer to a routine to be invoked upon a respones.
 *                       portCtrlFn: routine to set port control mode     
 *                       hostCtrlFn: routine to set port host mode     
 *                       eventNotifyFn: routine to handle Auth Mgr events     
 *                       enableGetFn: routine to get admin mode of the authentication protocol     
 *                       radiusEnableGetFn: routine to get whether RADIUS is configured as
 *                       an authentication method
 * 
 * 
 * @returns  SUCCESS
 * @returns  FAILURE
 *
 * @comments
 *
 * @end
 *
 *********************************************************************/
RC_t authmgrEventCallbackRegister( AUTHMGR_METHOD_t method,
    RC_t(*portCtrlFn) (uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t portControl),
    RC_t(*hostCtrlFn) (uint32 intIfNum,  AUTHMGR_HOST_CONTROL_t hostMode),
    RC_t(*eventNotifyFn) (uint32 intIfNum, uint32 event,  enetMacAddr_t *macAddr),
    RC_t(*enableGetFn) (uint32 intIfNum, uint32 *enabled),
    RC_t(*radiusEnableGetFn) (uint32 intIfNum, uint32 *enabled));


/*********************************************************************
*
* @purpose Deregister all routines to be called when a RADIUS response is
*          received from a server for a previously submitted request.
*
* @param   componentId  @b{(input)}  one of  COMPONENT_IDS_t
*
* @returns  SUCCESS
*
* @comments
*
* @end
*
*********************************************************************/
RC_t authmgrEventCallbackDeregister( AUTHMGR_METHOD_t method);

/*********************************************************************
*
* @purpose Set the port capabilities
*
* @param   intIfNum  @b{(input)}  interface number
* @param   paeCapabilities  @b{(input)}  capabiities (authenticator or supplicant)
*
* @returns  SUCCESS
*
* @comments
*
* @end
*
*********************************************************************/
RC_t authmgrDot1xCapabilitiesUpdate(uint32 intIfNum, uint32 paeCapabilities);
/*********************************************************************
* @purpose  Returns the Vlan assigned for the  logical interface
*
* @param    lIntIfNum    @b((input)) the specified interface
* @param    vlan        @b((output)) vlan assigned to the logical interface
* @param    mode        @b((output)) mode of assignment Radius/Default
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortVlanAssignmentGet(uint32 lIntIfNum,
                                          uint32 *vlanId,
                                          uint32 *mode);

/*********************************************************************
* @purpose  Returns the authentication status of the client
*
* @param    lIntIfNum    @b((input)) the specified interface
* @param    authStat @b((output)) auth status
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrClientVlanGet(uint32 lIntIfNum,
                             uint32 *vlanType,
                             uint32 *vlanId);

/*********************************************************************
* @purpose  Returns the authentication status of the client 
*
* @param    lIntIfNum    @b((input)) the specified interface
* @param    authStat @b((output)) auth status 
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrClientAuthStatusGet(uint32 lIntIfNum,
                                    uint32 *authStatus);

/*********************************************************************
* @purpose  Get global port control mode
*
* @param    *portControl @b{(output)} port control mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
RC_t authmgrGlobalPortControlModeGet( AUTHMGR_PORT_CONTROL_t *portControl);

/*********************************************************************
* @purpose  Set port control mode
*
* @param    portControl @b{(input)} port control mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
RC_t authmgrGlobalPortControlModeSet( AUTHMGR_PORT_CONTROL_t portControl);

/*********************************************************************
* @purpose  Set port control mode to default
*
* @param    intIfNum    @b{(input)} internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
RC_t authmgrPortControlModeReset(uint32 intIfNum);

/*********************************************************************
* @purpose  Set global host control mode
*
* @param    intIfNum    @b{(input)} internal interface number
* @param    host @b{(input)} port control mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
RC_t authmgrGlobalHostModeSet( AUTHMGR_HOST_CONTROL_t hostMode);

/*********************************************************************
* @purpose  Get global host control mode
*
* @param    intIfNum    @b{(input)} internal interface number
* @param    host @b{(input)} port control mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
RC_t authmgrGlobalHostModeGet( AUTHMGR_HOST_CONTROL_t *hostMode);

/*********************************************************************
* @purpose  Set host control mode to default
*
* @param    intIfNum    @b{(input)} internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
RC_t authmgrHostModeReset(uint32 intIfNum);

/*********************************************************************
* @purpose  Get number of authenticated clients on a port
*
* @param    intIfNum    @b{(input)} internal interface number
* @param    *count @b{(output)} number of authenticated clients
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments none
*       
* @end
*********************************************************************/
RC_t authmgrPortAuthCountGet(uint32 intIfNum, uint32 *count);

/*********************************************************************
* @purpose  Get number of authenticated clients on a port
*
* @param    intIfNum    @b{(input)} internal interface number
* @param    *count @b{(output)} number of authenticated clients
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments none
*       
* @end
*********************************************************************/
RC_t authmgrPortAuthCountGet(uint32 intIfNum, uint32 *count);
/*********************************************************************
* @purpose  Processes Authmgr-related event initiated by PACmgr.
*
* @param (in)    vlanData  VLAN data
* @param (in)    intIfNum  Interface Number
* @param (in)    event
*
* @returns   SUCCESS  or  FAILURE
*
* @end
*********************************************************************/
RC_t authmgrVlanChangeCallback (dot1qNotifyData_t * vlanData,
                                   uint32 intIfNum, uint32 event);

/*********************************************************************
* @purpose  Processes Authmgr-related event initiated by PACmgr. 
*
* @param (in)    vlanData  VLAN data
* @param (in)    intIfNum  Interface Number
* @param (in)    event
*
* @returns   SUCCESS  or  FAILURE
*
* @end
*********************************************************************/
RC_t authmgrVlanConfChangeCallback (dot1qNotifyData_t * vlanData,
                                       uint32 intIfNum, uint32 event);

/*********************************************************************
* @purpose  Reset port information
*
* @param    intIfNum   @b{(input)} internal interface number
* @param    initialize @b{(input)} initialize value
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments This value is set to  TRUE by management in order to force
*           initialization of a port.  It is re-set to  FALSE after
*           initialization has completed.
* 
* @end
*********************************************************************/
RC_t authmgrPortInfoReset(uint32 intIfNum,  BOOL initialize);

/*********************************************************************
 * @purpose  Cleans up a client session
 *
 * @param    mac_addr    @b{(input)}client's mac address 
 *
 * @returns   FALSE_
 * @returns   TRUE
 *
 * @comments 
 *
 * @end
 *********************************************************************/
RC_t authmgrClientDelete( enetMacAddr_t macAddr);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif
