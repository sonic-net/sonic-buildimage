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

#include "auth_mgr_include.h"
#include "auth_mgr_exports.h"
#include "auth_mgr_vlan_db.h"

extern authMgrVlanDbData_t *authmgrVlanStateDb; /* Vlan operational state cache. */
extern authMgrVlanDbData_t *authmgrVlanCfgDb; /* Vlan configured state cache. */ 
 VLAN_MASK_t dynamicVlanList;   /* List of dynamic VLANs created by PAC. */

/*********************************************************************
* @purpose  Add newly created VLAN to database.
*
* @param    vlan @b{(input)) vlan to be added.
*
* @returns   SUCCESS If VLAN was added to DB successfully.
* @returns   FAILURE Otherwise
*
* @comments Authmgr maintains an internal VLAN cache that mimics the
*           current SONiC VLAN state. This is done to avoid reading the
*           redis DB too many times.
*
* @end
*********************************************************************/
RC_t authmgrVlanAddLocalUpdate(uint32 vlan)
{
  RC_t rc =  FAILURE;

  /* Validate the VLAN being added */
  if ((vlan <   DOT1Q_MIN_VLAN_ID) || (vlan >  DOT1Q_MAX_VLAN_ID))
  {
     LOGF( LOG_SEVERITY_ERROR,
                              "Invalid VLAN %d received.", vlan);
    return rc;
  }

  /* Set bit corresponding to VLAN in VLAN DB */
   VLAN_SETMASKBIT(authmgrVlanStateDb->vlanDb.vlanMask, vlan);

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Update local VLAN DB with VLAN delete notification.
*
* @param    vlan @b{(input)) vlan to be removed.
*
* @returns   SUCCESS If VLAN was removed from DB successfully.
* @returns   FAILURE Otherwise
*
* @comments Authmgr maintains an internal VLAN cache that mimics the
*           current SONiC VLAN state. This is done to avoid reading the
*           redis DB too many times.
*
* @end
*********************************************************************/
RC_t authmgrVlanDeleteLocalUpdate(uint32 vlan)
{
  RC_t rc =  FAILURE;

  /* Validate the VLAN being deleted. */
  if ((vlan <   DOT1Q_MIN_VLAN_ID) || (vlan >  DOT1Q_MAX_VLAN_ID))
  {
     LOGF( LOG_SEVERITY_ERROR,
                              "Invalid VLAN %d received.", vlan);
    return rc;
  }

  /* Clear bit corresponding to VLAN in VLAN DB. */
   VLAN_CLRMASKBIT(authmgrVlanStateDb->vlanDb.vlanMask, vlan);

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Update local VLAN DB with VLAN port add event.
*
* @param    vlan       @b{(input)) VLAN
* @param    intIfNum   @b{(input)) internal interface number
* @param    tagging    @b{(input)) Port VLAN tag mode.
*
* @returns   SUCCESS If VLAN port add was handled successfully.
* @returns   FAILURE Otherwise
*
* @comments Authmgr maintains an internal VLAN cache that mimics the
*           current SONiC VLAN state. This is done to avoid reading the
*           redis DB too many times.
*
* @end
*********************************************************************/
RC_t authmgrVlanPortAddLocalUpdate(uint32 vlan, uint32 intIfNum,
                                      dot1qTaggingMode_t tagging)
{
  RC_t rc =  FAILURE;

  /* Validate VLAN. */
  if ((vlan <   DOT1Q_MIN_VLAN_ID) || (vlan >  DOT1Q_MAX_VLAN_ID))
  {
     LOGF( LOG_SEVERITY_ERROR,
                              "Invalid VLAN %d received.", vlan);
    return rc;
  }

  /* Check if VLAN exists. */
  if (! VLAN_ISMASKBITSET(authmgrVlanStateDb->vlanDb.vlanMask, vlan))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, 0,
                         "VLAN %d does not exist in cache. Continuing to update cache.", vlan);
  }

  /* Validate interface number. */
  if ( SUCCESS != nimCheckIfNumber(intIfNum))
  {
     LOGF( LOG_SEVERITY_ERROR,
                              "Interface number %s does not exist.",
                              authmgrIntfIfNameGet(intIfNum));
    return rc;
  }

  /* Set VLAN bitmask for port. */
   VLAN_SETMASKBIT(authmgrVlanStateDb->portVlanDb[intIfNum].vlanMask, vlan);

  /* Save tagging information for port */
  if (tagging ==  DOT1Q_MEMBER_TAGGED)
  {
    /* Bit set = tagged, bit clear = untagged */
     VLAN_SETMASKBIT(authmgrVlanStateDb->portVlanDb[intIfNum].tagging, vlan);
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Update local VLAN DB with VLAN port delete  event.
*
* @param    vlan       @b{(input)) VLAN
* @param    intIfNum   @b{(input)) internal interface number
* @param    tagging    @b{(input)) Port VLAN tag mode.
*
* @returns   SUCCESS If VLAN port delete was handled successfully.
* @returns   FAILURE Otherwise
*
* @comments Authmgr maintains an internal VLAN cache that mimics the
*           current SONiC VLAN state. This is done to avoid reading the
*           redis DB too many times.
*
*********************************************************************/
RC_t authmgrVlanPortDeleteLocalUpdate(uint32 vlan, uint32 intIfNum,
                                         dot1qTaggingMode_t tagging)
{
  RC_t rc =  FAILURE;

  /* Validate VLAN. */
  if ((vlan <   DOT1Q_MIN_VLAN_ID) || (vlan >  DOT1Q_MAX_VLAN_ID))
  {
     LOGF( LOG_SEVERITY_ERROR,
                              "Invalid VLAN %d received.", vlan);
    return rc;
  }

  /* Check if VLAN exists. */
  if (! VLAN_ISMASKBITSET(authmgrVlanStateDb->vlanDb.vlanMask, vlan))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, 0,
                         "VLAN %d does not exist in cache. Continuing to update cache.", vlan);
  }

  /* Validate interface number. */
  if ( SUCCESS != nimCheckIfNumber(intIfNum))
  {
     LOGF( LOG_SEVERITY_ERROR,
                              "Interface number %s does not exist.",
                              authmgrIntfIfNameGet(intIfNum));
    return rc;
  }

  /* Clear VLAN bitmask for port. */
   VLAN_CLRMASKBIT(authmgrVlanStateDb->portVlanDb[intIfNum].vlanMask, vlan);

  /* Clear tagging info for port. */
   VLAN_CLRMASKBIT(authmgrVlanStateDb->portVlanDb[intIfNum].tagging, vlan);

  return  SUCCESS;
}

/*********************************************************************
 * @purpose  check if VLAN is valid
 *
 * @param    vlan      @b{(input)) vlan Id
 *
 * @returns   SUCCESS  If VLAN exists
 * @returns   NOT_EXIST    If VLAN does not exist
 * @returns   FAILURE  Otherwise
 *
 * @comments On SONiC, authmgr maintains an internal VLAN data cache
 *           that is a mirror of the STATE_DB.
 *
 * @end
 *********************************************************************/
RC_t authmgrVlanCheckValid(uint32 vlan)
{
  RC_t rc =  FAILURE;

  /* Validate VLAN. */
  if ((vlan <   DOT1Q_MIN_VLAN_ID) || (vlan >  DOT1Q_MAX_VLAN_ID))
  {
     LOGF( LOG_SEVERITY_ERROR,
                              "Invalid VLAN %d received.", vlan);
    return rc;
  }

  /* Check if VLAN is configured. */
  if (! VLAN_ISMASKBITSET(authmgrVlanStateDb->vlanDb.vlanMask, vlan))
  {
    return  NOT_EXIST;
  }
  return  SUCCESS;
}

/*********************************************************************
 * @purpose  check if VLAN is static
 *
 * @param    vlan      @b{(input)) vlan Id
 *
 * @returns   SUCCESS  If VLAN exists
 * @returns   NOT_EXIST    If VLAN does not exist
 * @returns   FAILURE  Otherwise
 *
 * @comments On SONiC, authmgr maintains an internal VLAN data cache
 *           that is a mirror of the CONFIG_DB.
 *
 * @end
 *********************************************************************/
RC_t authmgrVlanCheckStatic(uint32 vlan)
{
  RC_t rc =  FAILURE;

  /* Validate VLAN. */
  if ((vlan <   DOT1Q_MIN_VLAN_ID) || (vlan >  DOT1Q_MAX_VLAN_ID))
  {
     LOGF( LOG_SEVERITY_ERROR,
                              "Invalid VLAN %d received.", vlan);
    return rc;
  }

  /* Check if VLAN is configured. */
  if (! VLAN_ISMASKBITSET(authmgrVlanCfgDb->vlanDb.vlanMask, vlan))
  {
    return  NOT_EXIST;
  }

  return  SUCCESS;
}

/*********************************************************************
 * @purpose  Get number of ports that are members of VLAN.
 *
 * @param    vlan      @b{(input)) vlan Id
 * @param    numPorts  @b{(output)) Number of ports in VLAN.
 *
 * @returns   SUCCESS  If number of ports are returned successfully.
 * @returns   FAILURE  Otherwise
 *
 * @comments On SONiC, authmgr maintains an internal VLAN data cache
 *           that is a mirror of the STATE_DB.
 *
 * @end
 *********************************************************************/
RC_t authmgrVlanEgressPortsGet(uint32 vlan, uint32 *numPorts)
{
  RC_t rc =  FAILURE;
  int i = 0, ports = 0;

  if (numPorts ==  NULLPTR)
  {
     LOGF( LOG_SEVERITY_ERROR,
                              "Invalid input parameter.");
    return rc;
  }

  /* Validate VLAN. */
  if ((vlan <   DOT1Q_MIN_VLAN_ID) || (vlan >  DOT1Q_MAX_VLAN_ID))
  {
     LOGF( LOG_SEVERITY_ERROR,
                              "Invalid VLAN %d received.", vlan);
    return rc;
  }

  for (i = 1; i <  AUTHMGR_INTF_MAX_COUNT; i++)
  {
    /* Check if port is member of VLAN */
    if ( VLAN_ISMASKBITSET(authmgrVlanStateDb->portVlanDb[i].vlanMask, vlan))
    {
      ports++;
    }
  }

  *numPorts = ports;
  return  SUCCESS;
}

/*********************************************************************
 * @purpose  Get the default VLAN of the port.
 *
 * @param    intIfNum      @b{(input)) Interface number
 * @param    vlan  @b{(output)) default VLAN.
 *
 * @returns   SUCCESS  If default VLAN is returned successfully.
 * @returns   FAILURE  Otherwise
 *
 * @comments The default VLAN is the configured Access VLAN of the port.
 *
 * @end
 *********************************************************************/
RC_t authmgrPortDefaultVlanGet(uint32 intIfNum, uint32 *vlan)
{
  RC_t rc =  FAILURE;
  int i = 0;
  authMgrVlanPortData_t *pCfg;

  if (vlan ==  NULLPTR)
  {
     LOGF( LOG_SEVERITY_ERROR,
                              "Invalid input parameter.");
    return rc;
  }

  *vlan = 0;

  pCfg = &authmgrVlanCfgDb->portVlanDb[intIfNum];
  if (pCfg ==  NULLPTR)
  {
     LOGF( LOG_SEVERITY_ERROR,
                    "Null data for port config data %s.",
                    authmgrIntfIfNameGet(intIfNum));
    return rc;
  }

  for (i = 1; i <  DOT1Q_MAX_VLAN_ID; i++)
  {
    if ( VLAN_ISMASKBITSET(pCfg->vlanMask, i))
    {
      if (! VLAN_ISMASKBITSET(pCfg->tagging, i))
      {
        break;;  
      }
    }
  }  

  if (i !=  DOT1Q_MAX_VLAN_ID)
  {
    if ( SUCCESS == authmgrVlanCheckStatic(i))
    {
      *vlan = i;
      rc =  SUCCESS;
    }
  }
  return rc;
}

/*********************************************************************
 * @purpose  Acquire port for PAC by removing all VLAN config on port.
 *
 * @param    intIfNum    @b{(input)) Internal interface number of port.
 *
 * @returns   SUCCESS  If port is acquired successfully.
 * @returns   FAILURE  Otherwise
 *
 * @comments On SONiC, authmgr maintains a snapshot of CONFIG_DB data 
 *           on PAC enabled ports.
 *
 * @end
 *********************************************************************/
RC_t authmgrVlanAcquirePort(uint32 intIfNum)
{
  RC_t rc =  FAILURE;
  authMgrVlanPortData_t *pCfg;
   char8 aliasName[ NIM_IF_ALIAS_SIZE + 1];

  /* Validate interface number. */
  if ( SUCCESS != nimCheckIfNumber(intIfNum))
  {
     LOGF( LOG_SEVERITY_ERROR,
                              "Interface number %s does not exist.", 
                              authmgrIntfIfNameGet(intIfNum));
    return rc;
  }

  /* Vlan config from CONFIG_DB is cached locally and used.
   * Get port config.
   */
  pCfg = &authmgrVlanCfgDb->portVlanDb[intIfNum];
  if (pCfg ==  NULLPTR)
  {
     LOGF( LOG_SEVERITY_ERROR,
                    "Null data for port config data %s.",
                    authmgrIntfIfNameGet(intIfNum));
    return rc;
  }

  if (nimGetIntfName (intIfNum,  ALIASNAME, aliasName) !=  SUCCESS)
  {
     LOGF ( LOG_SEVERITY_ERROR,
             "Unable to get alias for intf %s",
             authmgrIntfIfNameGet(intIfNum));
    return  FAILURE;
  }

 /* User config is removed from port by 
   * sending a notification to VLAN mgr.
   */
  if (pacCfgVlanSendCfgNotification( AUTHMGR_INTF_CFG_REMOVE,
                                       aliasName, pCfg) !=  SUCCESS)
  {
     LOGF( LOG_SEVERITY_ERROR,
                    "Unable to remove user config on port %s",
                    authmgrIntfIfNameGet(intIfNum));
    return  FAILURE;
  }

  /* Set port as acquired in STATE_DB. */
  if (pacCfgIntfAcquireSet(aliasName,  TRUE) !=  SUCCESS)
  {
     LOGF( LOG_SEVERITY_ERROR,
                    "Unable to set acquire flag on port %s",
                    authmgrIntfIfNameGet(intIfNum));
  }

  return  SUCCESS;  
}

/*********************************************************************
 * @purpose  Release port for PAC by reverting all VLAN config on port.
 *
 * @param    intIfNum    @b{(input)) Internal interface number of port.
 *
 * @returns   SUCCESS  If port is released successfully.
 * @returns   FAILURE  Otherwise
 *
 * @comments On SONiC, authmgr maintains a snapshot of CONFIG_DB data
 *           on PAC enabled ports.
 *
 * @end
 *********************************************************************/
RC_t authmgrVlanReleasePort(uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  authMgrVlanPortData_t *pCfg;
   char8 aliasName[ NIM_IF_ALIAS_SIZE + 1];

  /* Validate interface number. */
  if ( SUCCESS != nimCheckIfNumber(intIfNum))
  {
     LOGF( LOG_SEVERITY_ERROR,
                              "Interface number %s does not exist.",
                              authmgrIntfIfNameGet(intIfNum));
    return rc;
  }

  /* Vlan config from CONFIG_DB is cached locally and used.
   * Get port config.
   */
  pCfg = &authmgrVlanCfgDb->portVlanDb[intIfNum];
  if (pCfg ==  NULLPTR)
  {
     LOGF( LOG_SEVERITY_ERROR,
                    "Null data for port config data %s.",
                    authmgrIntfIfNameGet(intIfNum));
    return rc;
  }

  if (nimGetIntfName (intIfNum,  ALIASNAME, aliasName) !=  SUCCESS)
  {
     LOGF ( LOG_SEVERITY_ERROR,
             "Unable to get alias for intf %s",
             authmgrIntfIfNameGet(intIfNum));
    return  FAILURE;
  }

  /* Go over the config and revert all user config on port. */
  /* User config is reverted on port by 
   * sending a notification to VLAN mgr.
   */
  if (pacCfgVlanSendCfgNotification( AUTHMGR_INTF_CFG_REVERT,
                                       aliasName, pCfg) !=  SUCCESS)
  {
     LOGF( LOG_SEVERITY_ERROR,
             "Failed to revert user config on port %s",
             authmgrIntfIfNameGet(intIfNum));
    return  FAILURE;
  }

  /* Set port as released in STATE_DB. */
  if (pacCfgIntfAcquireSet(aliasName,  FALSE) !=  SUCCESS)
  {
     LOGF( LOG_SEVERITY_ERROR,
                    "Unable to set release flag on port %s",
                    authmgrIntfIfNameGet(intIfNum));
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Processes Authmgr-related event initiated by PACmgr.
*
* @param (in)    vlanData  VLAN event data
* @param (in)    intIfNum  Interface Number
* @param (in)    event
*
* @returns   SUCCESS  or  FAILURE
*
* @end
*********************************************************************/
RC_t authmgrVlanConfChangeCallback(dot1qNotifyData_t *vlanData, uint32 intIfNum, 
                                      uint32 event)
{
   uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1];
  authMgrVlanPortData_t *pCfg;

  memset (ifName, 0x00, sizeof (ifName));

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, 0,
                       "Received Vlan event %d for interface %s, vlan %d\n",
                       event, authmgrIntfIfNameGet(intIfNum), vlanData->data.vlanId);

  switch (event)
  {
    case VLAN_DELETE_PENDING_NOTIFY:
      authmgrIssueCmd (authmgrVlanConfDeleteEvent, intIfNum, vlanData);

       VLAN_CLRMASKBIT(authmgrVlanCfgDb->vlanDb.vlanMask, vlanData->data.vlanId);
      break;

    case VLAN_ADD_NOTIFY:
       VLAN_SETMASKBIT(authmgrVlanCfgDb->vlanDb.vlanMask, vlanData->data.vlanId);
      break;

    case VLAN_ADD_PORT_NOTIFY:
      pCfg = &authmgrVlanCfgDb->portVlanDb[intIfNum];
       VLAN_SETMASKBIT(pCfg->vlanMask, vlanData->data.vlanId);
      break;

    case VLAN_DELETE_PORT_NOTIFY:
      pCfg = &authmgrVlanCfgDb->portVlanDb[intIfNum];

      if (! VLAN_ISMASKBITSET (pCfg->tagging, vlanData->data.vlanId))
      {
        /* Untagged port membership removed. Clean up affected clients. */
        authmgrIssueCmd (authmgrVlanConfPortDeleteEvent, intIfNum, vlanData);
      }

       VLAN_CLRMASKBIT(pCfg->vlanMask, vlanData->data.vlanId);
      break;

    case VLAN_PVID_CHANGE_NOTIFY:
      pCfg = &authmgrVlanCfgDb->portVlanDb[intIfNum];
      pCfg->pvid = vlanData->data.vlanId;

  default:
    break;
  }

  return  SUCCESS;
}



