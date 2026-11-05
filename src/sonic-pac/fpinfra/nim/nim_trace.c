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


#include "datatypes.h"
#include "commdefs.h"
#include "cnfgr_api.h"
#include "nim_data.h"
#include "nim_util.h"

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
void nimTracePortEvent( COMPONENT_IDS_t component,
                        PORT_EVENTS_t   event,
                       uint32          intIfNum,
                        BOOL            start,
                       NIM_HANDLE_t       handle)
{
  return;
}


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
void nimTracePortEventComp( COMPONENT_IDS_t component,
                            PORT_EVENTS_t   event,
                           uint32          intIfNum,
                            BOOL            start,
                           NIM_HANDLE_t       handle)
{

  return;
}

/*********************************************************************
* @purpose  Trace a critical section entry/exit
*
* @param    write       {(input)}  1 == write, 0 == read
* @param    take        {(input)}  1 == take,  0 == give
* @param    take        {(input)}  1 == enter, 0 == exit
*
* @returns  void
*
* @end
*********************************************************************/
void nimTraceCriticalSection( uchar8 write,
                              uchar8 take)
{
  return;
}


/*********************************************************************
* @purpose  Profile a port event on a per component basis
*
* @param    component   {(input)}   The component being traced
* @param    event       {(input)}   The L7 event being issued
* @param    intIfNum    {(input)}   The internal interface for the event
* @param    start       {(input)}    TRUE if event is starting,  FALSE otherwise
*
* @returns  void
*
* @end
*********************************************************************/
void nimProfilePortEventComp( COMPONENT_IDS_t component,
                              PORT_EVENTS_t   event,
                             uint32          intIfNum,
                              BOOL            start
                            )
{
  return;
}

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
void nimProfilePortEvent( COMPONENT_IDS_t component,
                          PORT_EVENTS_t   event,
                         uint32          intIfNum,
                          BOOL            start
                        )
{
  return;
}

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


 char8 *nimDebugCompStringGet( COMPONENT_IDS_t cid)
{
  return("NA");
}

