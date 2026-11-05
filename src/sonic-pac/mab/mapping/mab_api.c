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

#include "auth_mgr_api.h"
#include "mab_include.h"
#include "osapi.h"
#include "mab_vlan.h"
#include "mab_struct.h"

extern mabBlock_t *mabBlock;

/*********************************************************************
* @purpose  Set the MAB value on the port.
*
* @param    intIfNum      @b{(input)} internal interface number
* @param    mabEnable   @b{(output)} boolean value determining if MAB 
*                                    has been configured on the port 
*
* @returns   SUCCESS
* @returns   FAILURE
* @results   REQUEST_DENIED     if port control mode of the port is 
*                                 not mac-based  
*
* @comments
*       
* @end
*********************************************************************/
RC_t mabPortMABEnableSet(uint32 intIfNum, uint32 mabEnable)
{
  mabPortCfg_t *pCfg;
  uint32 event = 0;

  if (mabIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  if (mabEnable !=  ENABLE && mabEnable !=  DISABLE)
    return  FAILURE;

  if (mabEnable ==  ENABLE)
  {
    pCfg->mabEnabled =  ENABLE;
    event =  mabMgmtPortMABEnableSet;
  }
  else 
  {
    pCfg->mabEnabled =  DISABLE;
    event = mabMgmtPortMABDisableSet;  
  }
  pCfg->mabAuthType = FD_MAB_PORT_MAB_AUTH_TYPE;

  return mabIssueCmd(event, intIfNum, &mabEnable);;
}

/*********************************************************************
* @purpose  Get the operational MAB value on the port.
*
* @param    intIfNum      @b{(input)} internal interface number
* @param    mabEnable   @b{(output)} value determining if MAB 
*                                     has been operationally 
*                                     enabled on the port 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
RC_t mabPortOperMABEnabledGet(uint32 intIfNum, uint32 *mabEnable)
{
  if (mabIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  *mabEnable = mabBlock->mabPortInfo[intIfNum].mabEnabled;

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Set the authentication type on the port to be used by MAB.
*
* @param    intIfNum      @b{(input)} internal interface number
* @param    auth_type     @b{(input)} Authentication type {EAP-MD5 or PAP} 
*
* @returns   SUCCESS            if able to set the auth_type successfully
* @results   REQUEST_DENIED     if MAB is not enabled on that port
* @returns   FAILURE            otherwise
*
* @comments
*       
* @end 
*********************************************************************/
RC_t mabPortMABAuthTypeSet(uint32 intIfNum,
                               AUTHMGR_PORT_MAB_AUTH_TYPE_t auth_type)
{
  mabPortCfg_t *pCfg =  NULLPTR;

  if (( AUTHMGR_PORT_MAB_AUTH_TYPE_INVALID >= auth_type) ||
      ( AUTHMGR_PORT_MAB_AUTH_TYPE_LAST <= auth_type))
  {
    /* Invalid input arguments */
    return  FAILURE;
  }

  if ( TRUE != mabIsValidIntf(intIfNum))
  {
    return  FAILURE;
  }

  if (( TRUE != mabIntfIsConfigurable(intIfNum, &pCfg)) ||
      ( NULLPTR == pCfg))
  {
    return  FAILURE;
  }

  /* check if MAB has been configured for the port */
  if ( ENABLE != pCfg->mabEnabled)
  {
    return  REQUEST_DENIED;
  }

  pCfg->mabAuthType = auth_type;
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Determine if the interface is valid to participate in mab
*
* @param    intIfNum              @b{(input)} internal interface number
*
* @returns   TRUE
* @returns   FALSE
*
* @comments
*       
* @end
*********************************************************************/
 BOOL mabIsValidIntf(uint32 intIfNum)
{
  INTF_TYPES_t sysIntfType;

  if ((nimGetIntfType(intIfNum, &sysIntfType) !=  SUCCESS) || 
      (mabIsValidIntfType(sysIntfType) !=  TRUE))
  {
    return  FALSE;
  }

  return  TRUE;
}

/*********************************************************************
* @purpose  Determine if the interface type is valid to participate in mab
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
 BOOL mabIsValidIntfType(uint32 sysIntfType)
{
  if (sysIntfType !=  PHYSICAL_INTF)
    return  FALSE;

  return  TRUE;
}

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
RC_t mabPortControlModeSet(uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t portControl)
{
  mabPortCfg_t *pCfg;

  if (mabIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  return mabIssueCmd(mabMgmtPortControlModeSet, intIfNum, &portControl);
}

/*********************************************************************
* @purpose  Set host control mode
*
* @param    intIfNum    @b{(input)} internal interface number
* @param    hostControl @b{(input)} host control mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*       
* @end
*********************************************************************/
RC_t mabPortControlHostModeSet(uint32 intIfNum,  AUTHMGR_HOST_CONTROL_t hostMode)
{
  mabPortCfg_t *pCfg;

  if (mabIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  return mabIssueCmd(mabMgmtPortHostModeSet, intIfNum, &hostMode);
}

/*********************************************************************
* @purpose  Return Internal Interface Number of the first valid interface for
*           mab.
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
RC_t mabFirstValidIntfNumber(uint32 *pFirstIntIfNum)
{
  RC_t rc =  FAILURE;
  nimUSP_t usp;
  uint32 intIfNum =  NULL;
  
  if ( SUCCESS == nimFirstValidIntfNumberByType( PHYSICAL_INTF, &intIfNum))
  {
    memset(&usp, 0, sizeof(usp));
    if (( SUCCESS == nimGetUnitSlotPort(intIfNum, &usp)))
    {    
      rc = mabNextValidIntf(intIfNum, &intIfNum);
    }
    else
    {
      rc =  SUCCESS;
    }
    
    if ( SUCCESS == rc)
    {
      /* Copy the valid interface in the output pointer */
      *pFirstIntIfNum = intIfNum;
    }
  } 
  return rc;
}

/*********************************************************************
* @purpose  Return Internal Interface Number of next valid interface for
*           mab.
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
RC_t mabNextValidIntf(uint32 intIfNum, uint32 *pNextIntIfNum)
{
  RC_t rc =  FAILURE;
  nimUSP_t usp;
  uint32 nextIntIfNum = intIfNum;

  while ( SUCCESS == nimNextValidIntfNumberByType( PHYSICAL_INTF, nextIntIfNum, &nextIntIfNum))
  {
    memset(&usp, 0, sizeof(usp));
    if ( SUCCESS == nimGetUnitSlotPort(nextIntIfNum, &usp))
    {
      /* Next mab valid interface found. */
      *pNextIntIfNum = nextIntIfNum;
      rc =  SUCCESS;
      break;
    }
  }

  return rc;

}

/*********************************************************************
* @purpose  Update the RADIUS server configuration
*
* @param    add           @b{(input)} whether server should be added or deleted
* @param    radius_type   @b{(input)} radius server type
* @param    serv_addr     @b{(input)} radius server address
* @param    serv_priority @b{(input)} radius server priority
* @param    radius_key    @b{(input)} radius server key
* @param    serv_port     @b{(input)} radius server port
*
* @returns   SUCCESS    values are valid and are updated successfully
* @returns   FAILURE    otherwise
*
* @comments
*
* @end
*********************************************************************/
RC_t mabRadiusServerUpdate(uint32 cmd, const char *radius_type,
                              const char *serv_addr, const char *serv_priority,
                              const char *radius_key, const char *serv_port)
{
  mabRadiusServer_t msg;
  RC_t rc =  SUCCESS;

  memset(&msg, 0, sizeof(msg));
  
  msg.cmd = cmd;

  if (radius_type)
    osapiStrncpySafe(msg.cmd_data.server.radius_type, radius_type, sizeof(msg.cmd_data.server.radius_type)-1);

  if (serv_addr)
    osapiStrncpySafe(msg.cmd_data.server.serv_addr, serv_addr, sizeof(msg.cmd_data.server.serv_addr)-1);

  if (serv_port)
  {
    osapiStrncpySafe(msg.cmd_data.server.serv_port, serv_port, sizeof(msg.cmd_data.server.serv_port)-1);
    MAB_EVENT_TRACE("%s: cfg update for server %s port %s", __FUNCTION__, serv_addr, serv_port);
  }

  if (serv_priority)
  {
    osapiStrncpySafe(msg.cmd_data.server.serv_priority, serv_priority, sizeof(msg.cmd_data.server.serv_priority)-1);
    MAB_EVENT_TRACE("%s: cfg update for server %s priority %s", __FUNCTION__, serv_addr, serv_priority);
  }

  if (radius_key)
  {
    osapiStrncpySafe(msg.cmd_data.server.key.key, radius_key, sizeof(msg.cmd_data.server.key.key)-1);
    msg.cmd_data.server.key.key_len = strlen(radius_key);
    MAB_EVENT_TRACE("%s: cfg update for server %s key len %d", __FUNCTION__, serv_addr, (unsigned int)strlen(radius_key));
  }

  rc = mabIssueCmd(mabRadiusConfigUpdate, 0, &msg);

  MAB_EVENT_TRACE("%s:Sent cfg update for server %s rc = %d", __FUNCTION__, serv_addr, rc);
  return rc;
}
