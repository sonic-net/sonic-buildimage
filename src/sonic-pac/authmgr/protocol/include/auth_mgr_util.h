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

#ifndef INCLUDE_AUTHMGR_UTIL_H
#define INCLUDE_AUTHMGR_UTIL_H
/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif
#include "comm_mask.h"
#include "apptimer_api.h"
#include "auth_mgr_exports.h"
#include "auth_mgr_sm.h"
#define AUTHMGR_PHYSICAL_PORT_GET(x)  (((x)/ AUTHMGR_MAX_USERS_PER_PORT)+1) 
#define AUTHMGR_LPORT_KEY_PACK(_x, _y, _z, _val) \
  do { \
    _val |= ((_x<<16) | (_y<<4) | (_z)); \
  } while (0);
#define AUTHMGR_LPORT_KEY_UNPACK(_x, _y, _z, _val) \
  do { \
    _x = (_val & 0XFFFF0000)>>16; \
    _y = (_val & 0X0000FFF0)>>4; \
    _z = (_val & 0X0000000F); \
  } while (0);
#define AUTHMGR_PORT_GET(_x, _val) \
  _x = (_val & 0XFFFF0000)>>16;
#define AUTHMGR_LPORT_GET(_y, _val) \
  _y = (_val & 0X0000FFF0)>>4; 
#define AUTHMGR_TYPE_GET(_z, _val) \
  _z = (_val & 0X0000000F);
#define AUTHMGR_MAX_PHY_PORT_USERS 2
#define AUTHMGR_IF_NULLPTR_RETURN_LOG(_p) \
  if ( NULLPTR == _p) \
  { \
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, 0, "%s is NULLPTR.", #_p); \
    return  FAILURE; \
  }

#define AUTHMGR_PRINT_MAC_ADDR(mac_addr)({                                   \
         uchar8 __maddr[32] = {'\0'};                                      \
        snprintf(__maddr, sizeof(__maddr), " %02X:%02X:%02X:%02X:%02X:%02X", \
                 (unsigned char)mac_addr[0], (unsigned char)mac_addr[1],     \
                 (unsigned char)mac_addr[2], (unsigned char)mac_addr[3],     \
                 (unsigned char)mac_addr[4], (unsigned char)mac_addr[5] );   \
        __maddr;  \
    })
 
/*********************************************************************
 * @purpose function to check policy validation based on host mode 
 *
 * @param    hostMode   @b{(input))  hostmode 
 * @param    *appyPolicy  @b{(input))  BOOL value 
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrHostIsDynamicNodeAllocCheck( AUTHMGR_HOST_CONTROL_t hostMode,  BOOL *valid);
/*********************************************************************
 * @purpose function to apply violation policy  
 *
 * @param    hostMode   @b{(input))  hostmode 
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrViolationPolicyApply(uint32 intIfNum);
/*********************************************************************
 * @purpose function to change the learning of an interface 
 *
 * @param    hostMode   @b{(input))  hostmode 
 * @param    *appyPolicy  @b{(input))  BOOL value 
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrPortLearningModify(uint32 intIfNum);
/*********************************************************************
 * @purpose function to check policy validation based on host mode 
 *
 * @param    hostMode   @b{(input))  hostmode 
 * @param    *appyPolicy  @b{(input))  BOOL value 
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrStaticFdbEntryValidCheck(uint32 intIfNum, BOOL *valid);
/*********************************************************************
  * @purpose function to check unlearn mac address violation policy validation
  *
  * @param    intIfNum   @b{(input))  internal interface number
  * @param    valid  @b{(input))  TRUE if allowed.
  *
  * @returns   SUCCESS
  * @returns   FAILURE
  *
  * @comments
  *
  * @end
  *********************************************************************/
RC_t authmgrViolationPolicyValidCheck (uint32 intIfNum,  BOOL * valid);
/*********************************************************************
* @purpose set the port settings when auth mgr is not enabled and blocked 
*
* @param intIfNum internal interface number
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPhysicalPortStatusBlockSet (uint32 intIfNum);
/*********************************************************************
* @purpose set the port settings when auth mgr is not enabled 
*
* @param intIfNum internal interface number
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPhysicalPortStatusOpenSet (uint32 intIfNum);
/*********************************************************************
* @purpose set the port settings when auth mgr is not enabled 
*
* @param intIfNum internal interface number
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPhysicalPortAccessSet (uint32 intIfNum);
/*********************************************************************
* @purpose function to change the learning of multi host mode
*
* @param intIfNum internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortAutoLearningModify (uint32 intIfNum);
/*********************************************************************
* @purpose  Get the physical port for the logical interface
*
* @param    intIfNum  @b{(input))  internal interface number
*
* @returns  end index for specified interface in authmgr logical
*           interface list
*
* @comments
*
* @end
*********************************************************************/
uint32 authmgrPhysPortGet(uint32 lIntIfNum);
/*********************************************************************
* @purpose  Get the interface name for the  given interface
*
* @param    intIfNum  @b{(input))  internal interface number
*
* @returns  Name of interface for Valid authmgr/nim interface,  
*           "UnKnown" for invalid interface
*
* @comments
*
* @end
*********************************************************************/
 char8 * authmgrIntfIfNameGet(uint32 intIfNum);
/*********************************************************************
* @purpose  validate the priority precedence of two methods 
*
* @param    intIfNum  @b{(input))  internal interface number
* @param    method1  @b{(input))   authentication method 
* @param    method2  @b{(input))   authentication method 
*
* @returns   SUCCESS if method 2 is of higher priority than method1 
* @returns    FAILURE  
*
* @comments
*
* @end
*********************************************************************/
uint32 authmgrPriorityPrecedenceValidate(uint32 intIfNum, 
                                             AUTHMGR_METHOD_t method1, 
                                             AUTHMGR_METHOD_t method2);
/* Set port learning config */
extern RC_t pacCfgIntfLearningModeSet(char *interface,  AUTHMGR_PORT_LEARNING_t learning);

/* Setup port learning config */
extern  BOOL pacCfgIntfViolationPolicySet(char *interface,  BOOL enable);

/* Add dot1x client */
extern  BOOL pacCfgIntfClientAdd(char *interface,  uchar8 *mac_addr, int vlan);

/* Delete dot1x client */
extern  BOOL pacCfgIntfClientRemove(char *interface,  uchar8 *mac_addr, int vlan);

/* Block a client's traffic while client is getting authorized */
extern  BOOL pacCfgIntfClientBlock(char *interface,  uchar8 *mac_addr, int vlan);

/* Unblock a client's traffic */
extern  BOOL pacCfgIntfClientUnblock(char *interface,  uchar8 *mac_addr, int vlan);

/* Set port PVID */
extern RC_t pacCfgPortPVIDSet(char *interface, int pvid);

/* Get port PVID */
extern RC_t pacCfgPortPVIDGet(char *interface, int *pvid);

/* Set port VLAN membership */
extern RC_t pacCfgVlanMemberAdd(int vlan, char *interface, dot1qTaggingMode_t mode);

/* Remove port VLAN membership */
extern RC_t pacCfgVlanMemberRemove(int vlan, char *interface);

/* Add a dynamic VLAN */
extern RC_t pacCfgVlanAdd(int vlan);

/* Remove a dynamic VLAN */
extern RC_t pacCfgVlanRemove(int vlan);

/* Send PVID set notification to VLAN mgr */
extern RC_t pacCfgVlanSendPVIDNotification(char *interface, int pvid);

/* USE C Declarations */
#ifdef __cplusplus
}
#endif
#endif /* INCLUDE_AUTHMGR_UTIL_H */
