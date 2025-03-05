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

#ifndef NIM_OUTCALLS_H
#define NIM_OUTCALLS_H

/*********************************************************************
* @purpose Callout from NIM interface notifications
*
* @param   intIfNum    internal interface number
* @param   event       one of  PORT_EVENTS_t
*
* @return  none
*
* @notes   Hooks for interactions with other components or for platform-specific
*          extensions to the standard dot1q offering.  This outcall occurs for
*          NIM notifications of interface changes. The NIM callout may cause the
*          event to be propagated.
*
*          The LVL7 hooks to interact are specific hooks for forwarding/not
*          forwarding and link active/link inactive  notifications to be issued
*          when spanning tree is not enabled.
*
*          These hooks also ensure that the underlying platform is appropriately
*          configured with respect to spanning tree configuration.
*
*          These hooks also ensure that the filtering database is appropriately
*          configured with respect to a port's MAC address.
*
* @end
*********************************************************************/
void nimNotifyUserOfIntfChangeOutcall(uint32 intIfNum, uint32 event);

/*********************************************************************
* @purpose  Get the instance number associated with an interface
*
* @param    configId  @b{(input)} an instance of a nimConfigID_t structure
* @param    instance  @b{(output)} instance number
*
* @returns   SUCCESS  if success
* @returns   ERROR    if interface does not exist
* @returns   FAILURE  if other failure
*
* @notes    Only supported for VLAN interfaces at this point
*
* @notes   Returns a relative instance number for the interface,
*          from 1 to the maximum number of ports for the interface type
*
*
* @end
*********************************************************************/
RC_t nimPortInstanceNumGet(nimConfigID_t configId, uint32 *instance);

#endif
