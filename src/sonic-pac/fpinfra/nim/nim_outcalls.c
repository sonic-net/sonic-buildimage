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


#include <string.h>
#include "datatypes.h"
#include "commdefs.h"
#include "cnfgr_api.h"
#include "nim_data.h"
#include "nim_util.h"
#include "nimapi.h"

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
void nimNotifyUserOfIntfChangeOutcall(uint32 intIfNum, uint32 event)
{
#ifdef  PFC_PACKAGE
  uint32 lagIntfNum;
   BOOL   lagIsPfcActive;
#endif
  NIM_EVENT_SPECIFIC_DATA_t eventData;

  memset (&eventData, 0, sizeof(NIM_EVENT_SPECIFIC_DATA_t));

  switch (event)
  {
    case  UP:
    case  PORT_ENABLE:
      /* Ensure that caller of this function calls this when both the events have happened */
      {
        /* set the interface to Authorize since authmgr is not managing this port */
        NIM_CRIT_SEC_WRITE_ENTER();
        NIM_INTF_SETMASKBIT(nimCtlBlk_g->authorizedStateMask, intIfNum);
        NIM_CRIT_SEC_WRITE_EXIT();
        nimNotifyIntfChange(intIfNum,  AUTHMGR_PORT_AUTHORIZED, eventData);
      }
      break;

    case  PORT_DISABLE:
    case  DOWN:
      {
        /* set the interface to Unauthorize since authmgr is not managing this port */
        NIM_CRIT_SEC_WRITE_ENTER();
        NIM_INTF_CLRMASKBIT(nimCtlBlk_g->authorizedStateMask, intIfNum);
        NIM_CRIT_SEC_WRITE_EXIT();
        nimNotifyIntfChange(intIfNum,  AUTHMGR_PORT_UNAUTHORIZED, eventData);
      }

      break;

    case  FORWARDING:
      /*
      ** Port needs to be 802.1x authorized before it can become active.
      */
      if(NIM_INTF_ISMASKBITSET(nimCtlBlk_g->authorizedStateMask, intIfNum) !=  NIM_UNUSED_PARAMETER)
      {
        nimNotifyIntfChange(intIfNum,  ACTIVE, eventData);
      }
      break;

    case  NOT_FORWARDING:
      /*
      ** Port is no longer forwarding and is now inactive
      */
      nimNotifyIntfChange(intIfNum,  INACTIVE, eventData);
      break;

    case  AUTHMGR_PORT_AUTHORIZED:
      {
        /*
         * always set to forwarding if the intf is not dot1s managed
         */
        nimNotifyIntfChange(intIfNum,  FORWARDING, eventData);
      }
      break;

    case  AUTHMGR_PORT_UNAUTHORIZED:
      {
        /*
         * always set to not forwarding if the intf is not dot1s managed
         */
        nimNotifyIntfChange(intIfNum,  NOT_FORWARDING, eventData);
      }
      break;

    case  PROBE_SETUP:
      break;

#ifdef  PORT_AGGREGATOR_PACKAGE
    case  PORT_AGG_UP:

      /* IEEE 802.1s Support for Multiple Spanning Tree */

      stpMode = dot1sModeGet();
      if (stpMode ==  DISABLE)
      {
        /* MSTP disabled, set the released lag member State to
         * manual Fowarding for the CIST.
         */

        dot1sIhSetPortState( DOT1S_CIST_INSTANCE, intIfNum,  DOT1S_MANUAL_FWD);
      }
      break;

    case  PORT_AGG_DOWN:

      /* IEEE 802.1s Support for Multiple Spanning Tree */

      stpMode = dot1sModeGet();
      if (stpMode ==  DISABLE)
      {
        /* MSTP disabled, set the released lag member State to
         * manual Fowarding for the CIST.
         */

        dot1sIhSetPortState( DOT1S_CIST_INSTANCE, intIfNum,  DOT1S_DISCARDING);
      }
      break;

#endif

#ifdef  PFC_PACKAGE
    case  LAG_ACQUIRE:
      /* Check if a PFC event should be issued based on the new composition
         of the LAG. */
      if (pfcIntfTypeIsValid(intIfNum))
      {
        do
        {
          if (dot3adWhoisOwnerLag(intIfNum, &lagIntfNum) !=  SUCCESS)
            break;

          if (NIM_INTF_ISMASKBITSET(nimCtlBlk_g->pfcActiveMask, intIfNum))
          {
            lagIsPfcActive = nimIsPfcActiveOnLag(lagIntfNum);
          }
          else
          {
            /* Only a single LAG member needs to have PFC inactive in order
               for the LAG to be PFC inactive. */
            lagIsPfcActive =  FALSE;
          }

          if(!NIM_INTF_ISMASKBITSET(nimCtlBlk_g->pfcActiveMask, lagIntfNum) && lagIsPfcActive)
          {
            nimNotifyIntfChange(lagIntfNum,  PORT_PFC_ACTIVE, eventData);
          }
          else if (NIM_INTF_ISMASKBITSET(nimCtlBlk_g->pfcActiveMask, lagIntfNum) && !lagIsPfcActive)
          {
            nimNotifyIntfChange(lagIntfNum,  PORT_PFC_INACTIVE, eventData);
          }

        } while (0);
      }
      break;

    case  LAG_RELEASE:
      /* Check if a PFC event should be issued based on the new composition
         of the LAG. */
      if (pfcIntfTypeIsValid(intIfNum))
      {
        lagIntfNum = 0;
        while (dot3adAggEntryGetNext(lagIntfNum, &lagIntfNum) ==  SUCCESS)
        {
          if (dot3adIsLagConfigured(lagIntfNum))
          {
            lagIsPfcActive = nimIsPfcActiveOnLag(lagIntfNum);

            if(!NIM_INTF_ISMASKBITSET(nimCtlBlk_g->pfcActiveMask, lagIntfNum) && lagIsPfcActive)
            {
              nimNotifyIntfChange(lagIntfNum,  PORT_PFC_ACTIVE, eventData);
            }
            else if (NIM_INTF_ISMASKBITSET(nimCtlBlk_g->pfcActiveMask, lagIntfNum) && !lagIsPfcActive)
            {
              nimNotifyIntfChange(lagIntfNum,  PORT_PFC_INACTIVE, eventData);
            }
          }
        }
      }
      break;

    case  PORT_PFC_ACTIVE:
      /* If this event occured on a LAG member, check to see if the event should also
         be issued for the LAG itself. */
      if (pfcIntfTypeIsValid(intIfNum) && dot3adIsLagMember(intIfNum))
      {
        do
        {
          if (dot3adWhoisOwnerLag(intIfNum, &lagIntfNum) !=  SUCCESS)
            break;

          /* If the LAG is not PFC active, then check to see that all
             active LAG members are PFC active before issuing the event
             for the LAG. */
          if(!NIM_INTF_ISMASKBITSET(nimCtlBlk_g->pfcActiveMask, lagIntfNum))
          {
            if (nimIsPfcActiveOnLag(lagIntfNum))
            {
              nimNotifyIntfChange(lagIntfNum,  PORT_PFC_ACTIVE, eventData);
            }
          }

        } while (0);
      }
      break;

    case  PORT_PFC_INACTIVE:
      /* If this event occured on a LAG member, check to see if the event should also
         be issued for the LAG itself. */
      if (pfcIntfTypeIsValid(intIfNum) && dot3adIsLagMember(intIfNum))
      {
        do
        {
          if (dot3adWhoisOwnerLag(intIfNum, &lagIntfNum) !=  SUCCESS)
            break;

          /* If the LAG was previously PFC active, notify that the LAG
             is PFC inactive. */
          if(!nimIsPfcActiveOnLag(lagIntfNum))
          {
            nimNotifyIntfChange(lagIntfNum,  PORT_PFC_INACTIVE, eventData);
          }

        } while (0);
      }
      break;
#endif
    default:
      break;
  }

  return;
}


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
RC_t nimPortInstanceNumGet(nimConfigID_t configId, uint32 *instance)
{
  uint32 index;
  RC_t rc;

  rc =  SUCCESS;
  *instance = 0;

  switch (configId.type)
  {
    case  LOGICAL_VLAN_INTF:
      *instance = index;
      break;
    default:
      NIM_LOG_MSG(" Only vlan interfaces supported at this time ");
      rc =  FAILURE;
      break;
  }

  return rc;
}

