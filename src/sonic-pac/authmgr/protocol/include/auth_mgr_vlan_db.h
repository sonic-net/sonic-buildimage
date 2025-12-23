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

#ifndef INCLUDE_AUTHMGR_VLAN_DB_H
#define INCLUDE_AUTHMGR_VLAN_DB_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "comm_mask.h"
#include "auth_mgr_exports.h"
#include "pacinfra_common.h"

typedef struct authMgrVlanData_s
{
   VLAN_MASK_t vlanMask;
} authMgrVlanData_t;

typedef struct authMgrVlanPortData_s
{
   VLAN_MASK_t vlanMask; /* VLANs this port is a member part of. */
   VLAN_MASK_t tagging; /* Tagging mode for each VLAN */
   BOOL isTrunkPort;   /* Is port a trunk port. */
  uint32 pvid;          /* Port PVID */
} authMgrVlanPortData_t;

typedef struct authMgrVlanDbData_s
{
  authMgrVlanData_t vlanDb;
  authMgrVlanPortData_t portVlanDb[ AUTHMGR_INTF_MAX_COUNT]; 
} authMgrVlanDbData_t;

typedef enum authMgrVlanPortCfgType_s
{
   AUTHMGR_INTF_CFG_REMOVE = 0,
   AUTHMGR_INTF_CFG_REVERT
} authMgrVlanPortCfgType_t;

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
RC_t authmgrVlanAddLocalUpdate(uint32 vlan);

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
RC_t authmgrVlanDeleteLocalUpdate(uint32 vlan);

/*********************************************************************
* @purpose  Update local VLAN DB with VLAN port add event.
*
* @param    vlan       @b{(input)) VLAN
* @param    intIfNum   @b{(input)) internal interface number
* @param    tagging    @b{(input)) Port VLAN tag mode
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
                                      dot1qTaggingMode_t tagging);

/*********************************************************************
* @purpose  Update local VLAN DB with VLAN port delete  event.
*
* @param    vlan       @b{(input)) VLAN
* @param    intIfNum   @b{(input)) internal interface number
* @param    tagging    @b{(input)) Port VLAN tag mode
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
                                         dot1qTaggingMode_t tagging);

/*********************************************************************
 * @purpose  check if VLAN is valid
 *
 * @param    vlan      @b{(input)) vlan Id
 *
 * @returns   SUCCESS  If VLAN exists
 * @returns   ERROR    If VLAN does not exist
 * @returns   FAILURE  Otherwise
 *
 * @comments On SONiC, authmgr maintains an internal VLAN data cache
 *           that is a mirror of the STATE_DB.
 *
 * @end
 *********************************************************************/
RC_t authmgrVlanCheckValid(uint32 vlan);

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
RC_t authmgrVlanCheckStatic(uint32 vlan);

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
RC_t authmgrVlanEgressPortsGet(uint32 vlan, uint32 *numPorts);

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
RC_t authmgrPortDefaultVlanGet(uint32 intIfNum, uint32 *vlan);

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
RC_t authmgrVlanAcquirePort(uint32 intIfNum);

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
RC_t authmgrVlanReleasePort(uint32 intIfNum);


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
RC_t authmgrVlanConfChangeCallback(dot1qNotifyData_t *vlanData, uint32 intIfNum,
                                      uint32 event);

/* Send VLAN cfg notifications. */
extern RC_t pacCfgVlanSendCfgNotification(authMgrVlanPortCfgType_t type,
                                          char *interface, authMgrVlanPortData_t *cfg);

/* Send PVID notifications. */
extern RC_t pacCfgVlanSendPVIDNotification(char *interface, int pvid);

/* Set acquire/release port info. */
extern RC_t pacCfgIntfAcquireSet(char *interface,  BOOL acquire);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_AUTHMGR_VLAN_DB_H */
