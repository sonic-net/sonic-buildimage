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


#ifndef NIM_EVENTS_H
#define NIM_EVENTS_H

#include "nim_data.h"
#include "sysapi_hpc.h"

/*********************************************************************
*
* @purpose  Delete a correlator for the event
*
* @param    correlator        @b{(input)}  The correlator to delete
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*********************************************************************/
RC_t nimEventCorrelatorDelete(NIM_CORRELATOR_t correlator);

/*********************************************************************
*
* @purpose  Create a correlator for the event
*
* @param    correlator        @b{(output)}  A returned correlator
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @end
*********************************************************************/
RC_t nimEventCorrelatorCreate(NIM_CORRELATOR_t *correlator);

/*********************************************************************
* @purpose  Remove the Event Handler Resources
*
* @param    file_name   @b{(input)}   File name from where this function is called
* @param    line_number @b{(input)}   Line number from where this function is called
* @param    status      @b{(input)}   The returned value from the caller
* @param    complete    @b{(output)}  Boolean value indicating whether event is complete
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes    none
*
* @end
*
*********************************************************************/
RC_t nimEventTally( char8 *file_name, uint32 line_number,
                      NIM_EVENT_COMPLETE_INFO_t status,
                       BOOL *complete);

/*********************************************************************
* @purpose  Initialize the Event Handler Resources
*
* @param    none
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes    none
*
* @end
*
*********************************************************************/
RC_t nimEventHdlrInit();

/*********************************************************************
* @purpose  Post Processor for Events
*
* @param    eventInfo    @b{(input)}   The event information as assigned by the generator
* @param    status       @b{(output)}  The status of the event by either Tally or Timeout
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes    none
*
* @end
*
*********************************************************************/
void nimEventPostProcessor(NIM_EVENT_NOTIFY_INFO_t eventInfo, NIM_NOTIFY_CB_INFO_t status);

/*********************************************************************
* @purpose  Card Manager Task
*
* @param    none
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
void nimTask();

/*********************************************************************
* @purpose  Notify all recepients of nim notifications of the link up event
*
* @param    none
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @notes    This routine is called only once when the system initialization is complete.
*           This event was suppressed when the system comes up and is only
*           propagated when all the components have initialized.
*
* @end
*
*********************************************************************/
EXT_API RC_t nimNotifyLinkUp();

/*********************************************************************
* @purpose  Notify the system application layer of a change in link status.
*
*
* @notes    This is the OLD function, once CM is in place, it SHALL be removed
*
* @end
*********************************************************************/
EXT_API void nimDtlIntfChangeCallback(nimUSP_t *usp, uint32 event, void * dapiIntmgmt);

EXT_API RC_t nimCmgrNewIntfChangeCallback(uint32 unit, uint32 slot, uint32 port,
                                              uint32 cardType, PORT_EVENTS_t event,
                                              SYSAPI_HPC_PORT_DESCRIPTOR_t *portData,
                                                            enetMacAddr_t *macAddr);
#endif /* NIM_EVENTS_H */
