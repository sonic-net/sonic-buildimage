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

#ifndef NIM_TRACE_H
#define NIM_TRACE_H

#include "pacinfra_common.h"

/*********************************************************************
* @purpose  Trace a port event
*
* @param    component   {(input)}   The component issuing the event
* @param    event       {(input)}   The L7 event being issued
* @param    intIfNum    {(input)}   The internal interface for the event
* @param    start       {(input)}    TRUE if event is starting,  FALSE otherwise
*
* @returns  void
*
* @end
*********************************************************************/
extern void nimTracePortEvent( COMPONENT_IDS_t component,
                               PORT_EVENTS_t   event,
                              uint32          intIfNum,
                               BOOL            start,
                              NIM_HANDLE_t       handle);

/*********************************************************************
* @purpose  Trace a port event on a per component basis
*
* @param    component   {(input)} The component being traced
* @param    event       {(input)} The L7 event being issued
* @param    intIfNum    {(input)} The internal interface for the event
* @param    start       {(input)}   TRUE if event is starting,  FALSE otherwise
*
* @returns  void
*
* @end
*********************************************************************/
extern void nimTracePortEventComp( COMPONENT_IDS_t component,
                                   PORT_EVENTS_t   event,
                                  uint32          intIfNum,
                                   BOOL            start,
                                  NIM_HANDLE_t       handle);

/*********************************************************************
* @purpose  Trace a critical section entry/exit
*
* @param    write       {(input)}  1 == write, 0 == read
* @param    take        {(input)}  1 == take,  0 == give
*
* @returns  void
*
* @end
*********************************************************************/
void nimTraceCriticalSection( uchar8 write,
                              uchar8 take);

/*********************************************************************
* @purpose  Profile a port event
*
* @param    component   {(input)}   The component issuing the event
* @param    event       {(input)}   The L7 event being issued
* @param    intIfNum    {(input)}   The internal interface for the event
* @param    start       {(input)}    TRUE if event is starting,  FALSE otherwise
*
* @returns  void
*
* @end
*********************************************************************/
extern void nimProfilePortEvent( COMPONENT_IDS_t component,
                                 PORT_EVENTS_t   event,
                                uint32          intIfNum,
                                 BOOL            start
                               );

/*********************************************************************
* @purpose  Profile a port event on a per component basis
*
* @param    component   {(input)} The component being traced
* @param    event       {(input)} The L7 event being issued
* @param    intIfNum    {(input)} The internal interface for the event
* @param    start       {(input)}   TRUE if event is starting,  FALSE otherwise
*
* @returns  void
*
* @end
*********************************************************************/
extern void nimProfilePortEventComp( COMPONENT_IDS_t component,
                                     PORT_EVENTS_t   event,
                                    uint32          intIfNum,
                                     BOOL            start
                                   );

/*********************************************************************
*
* @purpose  Issue link down trap
*
* @param    intIfNum    uint32 internal interface number
*
* @returns   SUCCESS, if success
* @returns   NOT_EXIST, if interface does not exist
* @returns   FAILURE, if other failure
*
* @notes none
*
* @end
*********************************************************************/
RC_t trapMgrLinkDownLogTrap(uint32 intIfNum);

/*********************************************************************
*
* @purpose  Issue Link Failure trap
*
*
* @param    intIfNum    uint32 internal interface number
*
* @returns   SUCCESS, if success
* @returns   NOT_EXIST, if interface does not exist
* @returns   FAILURE, if other failure
*
* @notes none
*
* @end
*********************************************************************/
RC_t trapMgrLinkFailureLogTrap(uint32 ifIndex);


#endif // NIM_TRACE_H
