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
#include "auth_mgr_vlan.h"
#include "auth_mgr_vlan_db.h"
#include "auth_mgr_struct.h"
#include "pacoper_common.h"

extern authmgrCB_t *authmgrCB;

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
RC_t authmgrPortInitializeGet(uint32 intIfNum,  BOOL *initialize)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  /* Setting this value to TRUE causes an action.  It is not actually
   * stored in the config structure.  So, just return what's stored
   * in the operational structure.
   */
  (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
  *initialize = authmgrCB->globalInfo->authmgrPortInfo[intIfNum].initialize;
  (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
  return  SUCCESS;
}

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
RC_t authmgrPortInitializeSet(uint32 intIfNum,  BOOL initialize)
{
  authmgrPortCfg_t *pCfg;
  RC_t rc =  SUCCESS;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  /* Port Mode must be Auto (i.e. not ForceAuthorized
   * or ForceUnauthorized)
   */
    if (pCfg->portControlMode ==  AUTHMGR_PORT_FORCE_AUTHORIZED ||
      pCfg->portControlMode ==  AUTHMGR_PORT_FORCE_UNAUTHORIZED )
    return  SUCCESS;
 
  if (initialize ==  TRUE)
    rc = authmgrIssueCmd(authmgrMgmtPortInitializeSet, intIfNum, &initialize);

  return rc;
}

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
*           due to a management request.  Per the MIB, this object always
*           returns FALSE when read.
*       
* @end
*********************************************************************/
RC_t authmgrPortReauthenticateGet(uint32 intIfNum,  BOOL *reauthenticate)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  /* Per the MIB, this object always returns FALSE when read */
  *reauthenticate =  FALSE;
  return  SUCCESS;
}

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
RC_t authmgrPortReauthenticateSet(uint32 intIfNum,  BOOL reauthenticate)
{
  authmgrPortCfg_t *pCfg;
  RC_t rc =  SUCCESS;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  /* Port Mode must be Auto (i.e. not ForceAuthorized
   * or ForceUnauthorized)
   */
    if (pCfg->portControlMode ==  AUTHMGR_PORT_FORCE_AUTHORIZED ||
      pCfg->portControlMode ==  AUTHMGR_PORT_FORCE_UNAUTHORIZED )
    return  FAILURE;

  if (reauthenticate ==  TRUE)
    rc = authmgrIssueCmd(authmgrMgmtPortReauthenticateSet, intIfNum, &reauthenticate);

  return rc;
}

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
RC_t authmgrPortControlModeGet(uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t *portControl)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  *portControl = pCfg->portControlMode;
  (void)osapiReadLockGive(authmgrCB->authmgrCfgRWLock);
  return  SUCCESS;
}

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
RC_t authmgrGlobalPortControlModeGet( AUTHMGR_PORT_CONTROL_t *portControl)
{
  (void)osapiReadLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  *portControl = authmgrCB->globalInfo->authmgrCfg->portControlMode;
  (void)osapiReadLockGive(authmgrCB->authmgrCfgRWLock);
  return  SUCCESS;
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
RC_t authmgrPortControlModeSet(uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t portControl)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiWriteLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  pCfg->portControlMode = portControl;
  (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);
  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  TRUE;

  return authmgrIssueCmd(authmgrMgmtPortControlModeSet, intIfNum, &portControl);
}

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
RC_t authmgrPortControlModeReset(uint32 intIfNum)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;


  (void)osapiWriteLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  pCfg->portControlMode = FD_AUTHMGR_PORT_MODE;
  (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);
  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  TRUE;

  return authmgrIssueCmd(authmgrMgmtPortControlModeSet, intIfNum, (void *)(&pCfg->portControlMode));
}

/*********************************************************************
* @purpose  Get quiet period value
*
* @param    intIfNum     @b{(input)} internal interface number
* @param    *quietPeriod @b{(output)} quiet period
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments The quietTime is the initialization value for quietWhile,
*           which is a timer used by the Authenticator state machine
*           to define periods of time in which it will not attempt to
*           acquire a Supplicant.
*       
* @end
*********************************************************************/
RC_t authmgrPortAuthRestartTimerGet(uint32 intIfNum, uint32 *quietPeriod)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  *quietPeriod = pCfg->quietPeriod;
  (void)osapiReadLockGive(authmgrCB->authmgrCfgRWLock);
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Set quiet period value
*
* @param    intIfNum    @b{(input)} internal interface number
* @param    quietPeriod @b{(input)} quiet period
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments The quietPeriod is the initialization value for quietWhile,
*           which is a timer used by the Authenticator state machine
*           to define periods of time in which it will not attempt to
*           acquire a Supplicant.
*       
* @end
*********************************************************************/
RC_t authmgrPortAuthRestartTimerSet(uint32 intIfNum, uint32 quietPeriod)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);
  pCfg->quietPeriod = quietPeriod;
  (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);
  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  TRUE;

  return authmgrIssueCmd(authmgrMgmtPortQuietPeriodSet, intIfNum, &quietPeriod);
}
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
RC_t authmgrPortReAuthPeriodGet(uint32 intIfNum, 
                                   uint32 *reAuthPeriod, 
                                    BOOL *serverConfig)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  *reAuthPeriod = pCfg->reAuthPeriod;
  *serverConfig = pCfg->reAuthPeriodServer;
  (void)osapiReadLockGive(authmgrCB->authmgrCfgRWLock);
  return  SUCCESS;
}

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
RC_t authmgrPortReAuthPeriodSet(uint32 intIfNum, uint32 reAuthPeriod,  BOOL serverConfig)
{
  authmgrPortCfg_t *pCfg;
  authmgrMgmtTimePeriod_t params;

  memset(&params, 0, sizeof(authmgrMgmtTimePeriod_t));

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiWriteLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  if (serverConfig)
  {
    pCfg->reAuthPeriodServer =  TRUE;
    pCfg->reAuthPeriod = FD_AUTHMGR_PORT_REAUTH_PERIOD;
  }
  else
  {
    pCfg->reAuthPeriodServer =  FALSE;
    pCfg->reAuthPeriod = reAuthPeriod;
  }
  (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);
  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  TRUE;

  params.reAuthPeriodServer = pCfg->reAuthPeriodServer;
  params.val = pCfg->reAuthPeriod;

  return authmgrIssueCmd(authmgrMgmtPortReAuthPeriodSet, intIfNum, (void *)&params);
}

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
RC_t authmgrPortReAuthEnabledGet(uint32 intIfNum,  BOOL *reAuthEnabled)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  *reAuthEnabled = pCfg->reAuthEnabled;
  (void)osapiReadLockGive(authmgrCB->authmgrCfgRWLock);
  return  SUCCESS;
}

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
RC_t authmgrPortReAuthEnabledSet(uint32 intIfNum,  BOOL reAuthEnabled)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;


  (void)osapiWriteLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  pCfg->reAuthEnabled = reAuthEnabled;
  (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);
  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  TRUE;

  return authmgrIssueCmd(authmgrMgmtPortReAuthEnabledSet, intIfNum, &reAuthEnabled);
}


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
RC_t authmgrPortAuthControlledPortStatusGet(uint32 intIfNum,  AUTHMGR_PORT_STATUS_t *portStatus)
{
  if (authmgrCB->globalInfo->authmgrPortInfo ==  NULLPTR)
    return  FAILURE;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);

  if ( AUTHMGR_PORT_STATUS_AUTHORIZED == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portStatus)
  {
    *portStatus =  AUTHMGR_PORT_STATUS_AUTHORIZED;
  }
  else
  {
    *portStatus =  AUTHMGR_PORT_STATUS_UNAUTHORIZED;
  }

  (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
  return  SUCCESS;
}

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
RC_t authmgrPortOperControlModeGet(uint32 intIfNum,  AUTHMGR_PORT_CONTROL_t *portMode)
{
  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
  if(authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portEnabled ==  TRUE)
    *portMode = authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode;
  else
    *portMode =  AUTHMGR_PORT_NA; 
  (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
  return  SUCCESS;
}

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
RC_t authmgrPortStatsClear(uint32 intIfNum)
{
  RC_t rc =  SUCCESS;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  rc = authmgrIssueCmd(authmgrMgmtPortStatsClear, intIfNum,  NULLPTR);

  return rc;
}

/*********************************************************************
* @purpose  Determine if the interface is valid to participate in authmgr
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
 BOOL authmgrIsValidIntfType(uint32 sysIntfType)
{
  if (sysIntfType !=  PHYSICAL_INTF)
    return  FALSE;

  return  TRUE;
}

/*********************************************************************
* @purpose  Determine if the interface is valid to participate in authmgr
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
BOOL authmgrIsValidIntf(uint32 intIfNum)
{
  INTF_TYPES_t sysIntfType;

  if ((nimGetIntfType(intIfNum, &sysIntfType) !=  SUCCESS) || 
      (authmgrIsValidIntfType(sysIntfType) !=  TRUE))
  {
    return  FALSE;
  }

  return  TRUE;
}


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
RC_t authmgrNextValidIntf(uint32 intIfNum, uint32 *pNextIntIfNum)
{
  RC_t rc =  FAILURE;
  uint32 nextIntIfNum = intIfNum;

  while ( SUCCESS == nimNextValidIntfNumberByType( PHYSICAL_INTF, nextIntIfNum, &nextIntIfNum))
  {
    if ( TRUE == authmgrIsValidIntf(nextIntIfNum))
    {
      /* Next authmgr valid interface found. */
      *pNextIntIfNum = nextIntIfNum;
      rc =  SUCCESS; 
      break;
    }
  }

  return rc;
}

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
RC_t authmgrFirstValidIntfNumber(uint32 *pFirstIntIfNum)
{
  RC_t rc =  FAILURE;
  uint32 intIfNum =  NULL;
  
  if ( SUCCESS == nimFirstValidIntfNumberByType( PHYSICAL_INTF, &intIfNum))
  {
    if ( FALSE == authmgrIsValidIntf(intIfNum))
    {    
      /* Front panel  and fixed stack ports are not a valid authmgr interfaces.
       * Hence get the next authmgr valid interface.
       */
      rc = authmgrNextValidIntf(intIfNum, &intIfNum);
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
RC_t authmgrVlanAssignmentModeSet (uint32 mode)
{
  if (authmgrCB->globalInfo->authmgrCfg->vlanAssignmentMode == mode)
    return  SUCCESS;

  (void)osapiWriteLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  if(mode ==  ENABLE)
  {
    authmgrCB->globalInfo->authmgrCfg->vlanAssignmentMode =  ENABLE;
  }
  else
  {
    authmgrCB->globalInfo->authmgrCfg->vlanAssignmentMode =  DISABLE;
  }
  (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);
  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  TRUE;

  return  SUCCESS;
}

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
RC_t authmgrVlanAssignmentModeGet(uint32 *mode)
{
  (void)osapiReadLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  *mode = authmgrCB->globalInfo->authmgrCfg->vlanAssignmentMode;
  (void)osapiReadLockGive(authmgrCB->authmgrCfgRWLock);
  return  SUCCESS;
}

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
RC_t authmgrPortMaxUsersSet(uint32 intIfNum, uint32 maxUsers)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  if ((maxUsers <  AUTHMGR_PORT_MIN_MAC_USERS) ||
      (maxUsers >  AUTHMGR_PORT_MAX_MAC_USERS))
  {
    return  FAILURE;
  }

  (void)osapiWriteLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  pCfg->maxUsers = maxUsers;
  (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);
  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  TRUE;

  return authmgrIssueCmd(authmgrMgmtPortMaxUsersSet, intIfNum, &maxUsers);
}

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
RC_t authmgrPortMaxUsersGet(uint32 intIfNum, uint32 *maxUsers)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  *maxUsers = pCfg->maxUsers;
  (void)osapiReadLockGive(authmgrCB->authmgrCfgRWLock);
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Returns the first logical port in use for the physcial interface
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
RC_t authmgrLogicalPortFirstGet(uint32 intIfNum,uint32 *lIntIfNum)
{
  authmgrPortCfg_t *pCfg;
  authmgrLogicalPortInfo_t *logicalPortInfo;
  RC_t  rc =  FAILURE;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE)
    return rc;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return rc;

  if ( AUTHMGR_PORT_AUTO == pCfg->portControlMode)
  {
    (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
     authmgrLogicalPortInfoTakeLock();
     logicalPortInfo = authmgrLogicalPortInfoFirstGet(intIfNum,lIntIfNum);

     if ( NULLPTR != logicalPortInfo)
     {
       if (0 != logicalPortInfo->key.keyNum)
       {
         rc =  SUCCESS;
       }
     }
     authmgrLogicalPortInfoGiveLock();
    (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
  }
  return rc;
}

/*********************************************************************
* @purpose  Returns the next logical port for the physcial interface
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
RC_t authmgrLogicalPortNextGet(uint32 lIntIfNum,uint32 *nextIntf)
{
  authmgrPortCfg_t *pCfg;
  uint32 next;
  RC_t  rc =  FAILURE;
  uint32 physPort = 0;

  AUTHMGR_PORT_GET(physPort, lIntIfNum);

  if (authmgrIsValidIntf(physPort) !=  TRUE)
    return rc;

  if (authmgrIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return rc;

  if ( AUTHMGR_PORT_AUTO == pCfg->portControlMode)
  {
    next = lIntIfNum;
    authmgrLogicalPortInfoTakeLock();
    if(authmgrLogicalPortInfoGetNextNode(physPort,&next)!= NULLPTR)
    {
      *nextIntf = next;
      rc =  SUCCESS;
    }
    authmgrLogicalPortInfoGiveLock();
  }
  return rc;
}

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
                                              uchar8 *macAddr)
{
  uint32 physPort = 0;
  authmgrPortCfg_t *pCfg;
  RC_t  rc =  SUCCESS;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  AUTHMGR_PORT_GET (physPort, lIntIfNum);

  if (authmgrIsValidIntf(physPort) !=  TRUE)
    return  FAILURE;

  if (authmgrIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return  FAILURE;


  (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
  authmgrLogicalPortInfoTakeLock();
  logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);
  if(logicalPortInfo !=  NULLPTR)
  {
    memcpy(macAddr,&logicalPortInfo->client.suppMacAddr, ENET_MAC_ADDR_LEN);
  }
  else
  {
    rc =  FAILURE;
  }
  authmgrLogicalPortInfoGiveLock();
  (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
  return rc;
}


/*********************************************************************
* @purpose  Returns the PAE state for the logical interface
*
* @param    lIntIfNum    @b((input)) the specified interface
* @param    authState @b((output)) PAE state
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortStateGet(uint32 lIntIfNum,
                                    uint32 *authState)
{
  uint32 physPort;
  authmgrPortCfg_t *pCfg;
  RC_t  rc =  SUCCESS;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  AUTHMGR_PORT_GET(physPort, lIntIfNum);

  if (authmgrIsValidIntf(physPort) !=  TRUE)
    return  FAILURE;

  if (authmgrIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
  authmgrLogicalPortInfoTakeLock();
  logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);
  if(logicalPortInfo !=  NULLPTR)
  {
    *authState = logicalPortInfo->protocol.authState + 1;
  }
  else
  {
    rc =  FAILURE;
  }
  authmgrLogicalPortInfoGiveLock();
  (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
  return rc;
}

/*********************************************************************
* @purpose  Returns the Vlan assigned for the  logical interface
*
* @param    lIntIfNum    @b((input)) the specified interface
* @param    vlan        @b((output)) vlan assigned to the logical interface
* @param    mode        @b((output)) mode of assignment Radius/unauthenticated/Default
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
                                          uint32 *mode)
{
  uint32 physPort;
  authmgrPortCfg_t *pCfg;
  authmgrLogicalPortInfo_t *logicalPortInfo;
  authmgrVlanInfoEntry_t entry;

  AUTHMGR_PORT_GET(physPort, lIntIfNum);

  if (authmgrIsValidIntf(physPort) !=  TRUE)
    return  FAILURE;

  if (authmgrIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
  authmgrLogicalPortInfoTakeLock();
  logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);
  if(logicalPortInfo !=  NULLPTR)
  {
    memset(&entry, 0, sizeof(authmgrVlanInfoEntry_t));
    if ( SUCCESS != authmgrVlanTypeInfoGet(logicalPortInfo->client.vlanType, &entry))
    {
      AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_FAILURE,physPort,
          "%s %s Unable to find the vlan Info entry for the vlan type %d\n",
          __FUNCTION__, authmgrIntfIfNameGet(physPort), logicalPortInfo->client.vlanType);
      authmgrLogicalPortInfoGiveLock();
     (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
      return  FAILURE;
    }

    *vlanId =logicalPortInfo->client.vlanId;
    *mode = entry.assignmentReason;
  }

    authmgrLogicalPortInfoGiveLock();
    (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
    return  SUCCESS;
  }

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
RC_t authmgrLogicalPortUserNameGet(uint32 lIntIfNum,
                                     uchar8 *userName)
{
  uint32 physPort;
  authmgrPortCfg_t *pCfg;
  RC_t  rc =  SUCCESS;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  AUTHMGR_PORT_GET(physPort, lIntIfNum);

  if (authmgrIsValidIntf(physPort) !=  TRUE)
    return  FAILURE;

  if (authmgrIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
  authmgrLogicalPortInfoTakeLock();
  logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);
  if(logicalPortInfo !=  NULLPTR)
  {
    memcpy(userName,logicalPortInfo->client.authmgrUserName,
           logicalPortInfo->client.authmgrUserNameLength);
  }
  else
  {
    rc =  FAILURE;
  }
  authmgrLogicalPortInfoGiveLock();
  (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
  return rc;
}


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
                                    uint32 *session_timeout)
{
  uint32 physPort;
  authmgrPortCfg_t *pCfg;
  RC_t  rc =  SUCCESS;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  AUTHMGR_PORT_GET(physPort, lIntIfNum);

  if (authmgrIsValidIntf(physPort) !=  TRUE)
    return  FAILURE;

  if (authmgrIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
  authmgrLogicalPortInfoTakeLock();
  logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);
  if(logicalPortInfo !=  NULLPTR)
  {
    *session_timeout = logicalPortInfo->client.sessionTimeout;
  }
  else
  {
    rc =  FAILURE;
  }
  authmgrLogicalPortInfoGiveLock();
  (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
  return rc;
}

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
                                                        uint32 *timeLeft)
{
  uint32 physPort;
  authmgrPortCfg_t *pCfg;
  RC_t  rc =  SUCCESS;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  AUTHMGR_PORT_GET(physPort, lIntIfNum);

  if (authmgrIsValidIntf(physPort) !=  TRUE)
    return  FAILURE;

  if (authmgrIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
  authmgrLogicalPortInfoTakeLock();
  logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);
  if(logicalPortInfo !=  NULLPTR)
  {
    rc = appTimerTimeLeftGet(authmgrCB->globalInfo->authmgrTimerCB,
                             logicalPortInfo->authmgrTimer.handle.timer,
                             timeLeft);
  }
  else
  {
    rc =  FAILURE;
  }
  authmgrLogicalPortInfoGiveLock();
  (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
  return rc;
}


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
                                    uint32 *terminationAction)
{
  uint32 physPort;
  authmgrPortCfg_t *pCfg;
  RC_t  rc =  SUCCESS;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  AUTHMGR_PORT_GET(physPort, lIntIfNum);

  if (authmgrIsValidIntf(physPort) !=  TRUE)
    return  FAILURE;

  if (authmgrIntfIsConfigurable(physPort, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
  authmgrLogicalPortInfoTakeLock();
  logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);
  if(logicalPortInfo !=  NULLPTR)
  {
    *terminationAction = logicalPortInfo->client.terminationAction + 1;
  }
  else
  {
    rc =  FAILURE;
  }
  authmgrLogicalPortInfoGiveLock();
  (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
  return rc;
}

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
RC_t authmgrClientMacAddressGet( enetMacAddr_t *macAddr,uint32 *lIntIfNum)
{
    return authmgrMacAddrInfoFind(macAddr,lIntIfNum);
}

/*********************************************************************
* @purpose  Returns the logical port for the next supplicant Mac Address
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
RC_t authmgrClientMacAddressNextGet( enetMacAddr_t *macAddr,uint32 *lIntIfNum)
{
    return authmgrMacAddrInfoFindNext(macAddr,lIntIfNum);
}

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
RC_t authmgrPhysicalPortGet(uint32 lIntIfNum,uint32 *physPort)
{
  AUTHMGR_PORT_GET(*physPort, lIntIfNum)
  return  SUCCESS;
}
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
RC_t authmgrUnauthAddrCallBack( uint32 intIfNum,  enetMacAddr_t macAddr,  ushort16 vlanId )
{
  authmgrPortCfg_t *pCfg;
  authmgrUnauthCallbackParms_t      parms;

  /* If it is not a valid interface drop the request */
  if ( authmgrIsValidIntf( intIfNum ) !=  TRUE )
  {
    return  FAILURE;
  }

  /* check if Guest vlan is enabled */
  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
  {
    return  FAILURE;
  }

  memcpy(&parms.macAddr,&macAddr.addr, ENET_MAC_ADDR_LEN);
  parms.vlanId = vlanId;

  /* send the message to the authmgr component */
  return authmgrIssueCmd(authmgrUnauthAddrCallBackEvent,intIfNum,(void *)&parms);
}

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
                                    uint32 *vlanId)
{
  RC_t  rc =  SUCCESS;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
  authmgrLogicalPortInfoTakeLock();
  logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);
  if(logicalPortInfo !=  NULLPTR)
  {
    *vlanType = logicalPortInfo->client.vlanType;
    *vlanId = logicalPortInfo->client.vlanId;
  }
  else
  {
    rc =  FAILURE;
  }
  authmgrLogicalPortInfoGiveLock();
  (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
  return rc;
}


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
                                    uint32 *authStatus)
{
  RC_t  rc =  SUCCESS;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
  authmgrLogicalPortInfoTakeLock();
  logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);
  if(logicalPortInfo !=  NULLPTR)
  {
    *authStatus = logicalPortInfo->client.logicalPortStatus;
  }
  else
  {
    rc =  FAILURE;
  }
  authmgrLogicalPortInfoGiveLock();
  (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
  return rc;
}

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
 BOOL authmgrPortClientAuthenticationGet(uint32 intIfNum,  uchar8 *macAddr)
{
   AUTHMGR_PORT_CONTROL_t portControl;
   AUTHMGR_PORT_STATUS_t    status;
  uint32               lIntIfNum;
  uint32               physPort=0;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  TRUE;
  }
  if ( SUCCESS != authmgrPortControlModeGet(intIfNum, &portControl))
  {
    return  TRUE;
  }

  if ( AUTHMGR_PORT_FORCE_UNAUTHORIZED == portControl)
  {
    /* If port is force unauthorized, client is not authenticated */
    return  FALSE;
  }

  if ( AUTHMGR_PORT_FORCE_AUTHORIZED == portControl)
  {
    /* If port is force authorized, client is assumed to be authenticated */
    return  TRUE;
  }

  if ( SUCCESS != authmgrClientMacAddressGet(( enetMacAddr_t *)macAddr, &lIntIfNum))
  {
    return  FALSE;
  }

  if (( SUCCESS != authmgrPhysicalPortGet(lIntIfNum, &physPort)) ||
      (physPort != intIfNum))
  {
    return  FALSE;
  }

  if (( SUCCESS != authmgrClientAuthStatusGet(lIntIfNum, &status)) ||
      ( AUTHMGR_PORT_STATUS_AUTHORIZED != status))
  {
    return  FALSE;
  }

  return  TRUE;

}

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
 BOOL authmgrPortIsAcquired(uint32 intIfNum)
{
  authmgrPortCfg_t *pCfg;
   BOOL val =  FALSE;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
    return  FALSE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FALSE;

  (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
   val = authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authmgrAcquire;
  (void)osapiReadLockGive(authmgrCB->authmgrRWLock);

  return val;
}

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
RC_t authmgrPortMaxAuthAttemptsGet(uint32 intIfNum, uint32 *maxAuthAttempts)
{
  authmgrPortCfg_t *pCfg =  NULLPTR;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
  {
    return  FAILURE;
  }

  (void)osapiReadLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  *maxAuthAttempts = pCfg->maxAuthAttempts;
  (void)osapiReadLockGive(authmgrCB->authmgrCfgRWLock);
  return  SUCCESS;
}

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
RC_t authmgrPortMaxAuthAttemptsSet(uint32 intIfNum, uint32 maxAuthAttempts)
{
  authmgrPortCfg_t *pCfg =  NULLPTR;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
  {
    return  FAILURE;
  }

  if ((maxAuthAttempts <  AUTHMGR_RADIUS_MAX_AUTH_ATTEMPTS_RANGE_MIN) ||
      (maxAuthAttempts >  AUTHMGR_RADIUS_MAX_AUTH_ATTEMPTS_RANGE_MAX))
  {
    return  FAILURE;
  }

  (void)osapiWriteLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  pCfg->maxAuthAttempts = maxAuthAttempts;
  (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);
  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  TRUE;
  return authmgrIssueCmd(authmgrMaxAuthAttemptsSet, intIfNum, (void *)&maxAuthAttempts);

}

/*********************************************************************
* @purpose  Check if the vlan is assigned to any client or port
*
* @param    intIfNum        @b{(input)} internal interface number
* @param    vlanId        @b{(input)} vlanId
*
* @returns   TRUE
* @returns   FALSE
*
* @comments
*
* @end
*********************************************************************/
 BOOL authmgrVlanAssignedCheck (uint32 intIfNum, uint32 vlanId)
{
  uint32 lIntIfNum;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  FALSE;
  }
  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
  {
    return  FALSE;
  }

   (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
    /* iterate through all the logical interfaces of the physical interface */
    lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;
    while((logicalPortInfo=authmgrLogicalPortInfoGetNextNode(intIfNum, &lIntIfNum))!=  NULLPTR)
    {
      if (0 != logicalPortInfo->key.keyNum)
      {
        if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portEnabled ==  TRUE &&
            (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode ==  AUTHMGR_PORT_AUTO))
        {
          if ((AUTHMGR_VLAN_RADIUS == logicalPortInfo->client.vlanType) && 
              (logicalPortInfo->client.vlanId == vlanId)) 
          {
            /* Vlan is assigned. Return TRUE. */
            (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
            return  TRUE;
          }
        }
      }
    }

    (void)osapiReadLockGive(authmgrCB->authmgrRWLock);

  /* If we reach here, Vlan is not assigned. Return FALSE. */
  return  FALSE;
}


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
RC_t authmgrHostControlModeGet(uint32 intIfNum,  AUTHMGR_HOST_CONTROL_t *hostMode)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  *hostMode = pCfg->hostMode;
  return  SUCCESS;
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
RC_t authmgrHostControlModeSet(uint32 intIfNum,  AUTHMGR_HOST_CONTROL_t hostMode)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  if (hostMode == pCfg->hostMode)
  {
    return  SUCCESS;
  }

  (void)osapiWriteLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  pCfg->hostMode = hostMode;
  (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);
  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  TRUE;

  return authmgrIssueCmd(authmgrMgmtHostControlModeSet, intIfNum, &hostMode);
}

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
RC_t authmgrHostModeReset(uint32 intIfNum)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;


  (void)osapiWriteLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  pCfg->hostMode = FD_AUTHMGR_HOST_MODE;
  (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);
  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  TRUE;

  return authmgrIssueCmd(authmgrMgmtHostControlModeSet, intIfNum, (void *)&pCfg->hostMode);
}


void debugUnLearnAddrCallback(uint32 intIfNum)
{
   enetMacAddr_t macAddr;
   ushort16 vlanId;

  memset(&macAddr, 0, sizeof( enetMacAddr_t));

  macAddr.addr[5] = 1;
  vlanId  = 1;
  authmgrUnauthAddrCallBack(intIfNum, macAddr, vlanId);
}
/*********************************************************************
 * @purpose  chechks if method order config is valid.
 *
 * @param    intIfNum    @b{(input)} internal interface number
 * @param    index @b{(input)} position of the method or order
 * @param    method @b{(input)} authentication manager methods,
 i.e.dot1x/mab/cp
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments Captive portal is always the last method. If any other method is tried
 to configure after captive portal it should not allow.
 *
 * @end
 *********************************************************************/
RC_t authmgrPortAuthMethodOrderValidate(uint32 intIfNum, uint32 index,
     AUTHMGR_METHOD_t method)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE)
    return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  /* either no command or 
     first method in the list .
     Allow */
  if(( AUTHMGR_METHOD_NONE == method) || 
      (0 == index))
  {
    return  SUCCESS;
  }

  else if ( AUTHMGR_METHOD_NONE == pCfg->methodList[index-1])
  {
    return  FAILURE;
  }
  return  SUCCESS;
}

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
        uint32 index,  AUTHMGR_METHOD_t method)
{
  authmgrPortCfg_t *pCfg;
  uint32 i = 0, j = 0;
  RC_t rc =  FAILURE;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
    return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  if (( AUTHMGR_TYPE_ORDER != mode) && ( AUTHMGR_TYPE_PRIORITY != mode))
  {
    return  FAILURE;
  }

  if ((index <  AUTHMGR_METHOD_START ) || (index >  AUTHMGR_METHOD_MAX))
  {
    return  FAILURE;
  }

  if (( AUTHMGR_METHOD_8021X != method) &&
      ( AUTHMGR_METHOD_MAB != method) &&
      ( AUTHMGR_METHOD_NONE != method))
  {
    return  FAILURE;
  }
  /* Now input always comes with order+1, reduce the order by 1 */
  j = index - 1;

  if ( AUTHMGR_TYPE_ORDER == mode)
  {
    /* Validate if the config of the order is correct */
    if ( FAILURE == authmgrPortAuthMethodOrderValidate(intIfNum, j, method))
    {
      return  ERROR;
    }

    if (pCfg->methodList[j] == method)
    {
      return  SUCCESS;
    }
    (void)osapiWriteLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);

    pCfg->methodList[j] = method;
    for (i = index; i <  AUTHMGR_METHOD_MAX; i++)
    {
      /* reset the configuration after the passed order
         since we don't know if the configuration stops here or not */
      pCfg->methodList[i] =  AUTHMGR_METHOD_NONE;
    }
    (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);
  }
  else if ( AUTHMGR_TYPE_PRIORITY == mode)
  {
    if (pCfg->priorityList[j] == method)
    {
      return  SUCCESS;
    }
   (void)osapiWriteLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
    pCfg->priorityList[j] = method;
    for (i = index; i <  AUTHMGR_METHOD_MAX; i++)
    {
      /* reset the configuration after the passed order
         since we don't know if the configuration stops here or not */
      pCfg->priorityList[i] =  AUTHMGR_METHOD_NONE;
    }
  }
  (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);

  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  TRUE;

  AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_API_CALLS,intIfNum,"%s:exiting-%s \n",
                      __FUNCTION__,authmgrIntfIfNameGet(intIfNum));
  if ( AUTHMGR_TYPE_ORDER == mode)
  {
    rc = authmgrIssueCmd(authmgrMethodOrderModify, intIfNum,  NULL);
  }
  else if ( AUTHMGR_TYPE_PRIORITY == mode)
  {
    rc = authmgrIssueCmd(authmgrMethodPriorityModify, intIfNum,  NULL);
  }
  return rc;
}


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
        uint32 index,  AUTHMGR_METHOD_t *method)
{
  authmgrPortCfg_t *pCfg;
  uint32 j = 0;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
    return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  if (( AUTHMGR_TYPE_ORDER != mode) && ( AUTHMGR_TYPE_PRIORITY != mode))
  {
    return  FAILURE;
  }

  if ((index <  AUTHMGR_METHOD_START ) || (index >  AUTHMGR_METHOD_MAX))
  {
    return  FAILURE;
  }

  /* Now input always comes with order+1, reduce the order by 1 */
  j = index - 1;

  (void)osapiReadLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  if ( AUTHMGR_TYPE_ORDER == mode)
  {
    *method = pCfg->methodList[j];
  }
  else if ( AUTHMGR_TYPE_PRIORITY == mode)
  {
    *method = pCfg->priorityList[j];
  }

  (void)osapiReadLockGive(authmgrCB->authmgrCfgRWLock);

  return  SUCCESS;
}


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
        uint32 index,  AUTHMGR_METHOD_t *method)
{
  uint32 j = 0;

  if ( TRUE != authmgrIsValidIntf(intIfNum))
  {
    return  FAILURE;
  }
  if (( AUTHMGR_TYPE_ORDER != mode) && ( AUTHMGR_TYPE_PRIORITY != mode))
  {
    return  FAILURE;
  }

  if ((index <  AUTHMGR_METHOD_START ) || (index >  AUTHMGR_METHOD_MAX))
  {
    return  FAILURE;
  }

  /* Now input always comes with order+1, reduce the order by 1 */
  j = index - 1;

  (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
  if ( AUTHMGR_TYPE_ORDER == mode)
  {
    *method = authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[j];
  }
  else if ( AUTHMGR_TYPE_PRIORITY == mode)
  {
    *method = authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority[j];
  }
  (void)osapiReadLockGive(authmgrCB->authmgrRWLock);

  return  SUCCESS;
}


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
    RC_t(*radiusEnabledGetFn) (uint32 intIfNum, uint32 *enabled))
{

  AUTHMGR_IF_INVALID_METHOD_RETURN_LOG(method);

  if ((authmgrCB->globalInfo->authmgrCallbacks[method].method !=  AUTHMGR_METHOD_NONE) ||
      (method ==  AUTHMGR_METHOD_NONE))
  {
     LOGF( LOG_SEVERITY_INFO,
            "AUTHMGR: method ID %s already registered", authmgrMethodStringGet(authmgrCB->globalInfo->authmgrCallbacks[method].method));
    return  FAILURE;
  }

  authmgrCB->globalInfo->authmgrCallbacks[method].method = method;
  authmgrCB->globalInfo->authmgrCallbacks[method].portCtrlFn = portCtrlFn;
  authmgrCB->globalInfo->authmgrCallbacks[method].hostCtrlFn = hostCtrlFn;
  authmgrCB->globalInfo->authmgrCallbacks[method].eventNotifyFn = eventNotifyFn;
  authmgrCB->globalInfo->authmgrCallbacks[method].enableGetFn = enableGetFn;
  authmgrCB->globalInfo->authmgrCallbacks[method].radiusEnabledGetFn = radiusEnabledGetFn;

  return  SUCCESS;
}

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
RC_t authmgrEventCallbackDeregister( AUTHMGR_METHOD_t method)
{
  AUTHMGR_IF_INVALID_METHOD_RETURN_LOG(method);

  if ((authmgrCB->globalInfo->authmgrCallbacks[method].method ==  AUTHMGR_METHOD_NONE) ||
      (method != authmgrCB->globalInfo->authmgrCallbacks[method].method))
  {
     LOGF( LOG_SEVERITY_INFO,
            "AUTHMGR: fail to de reigister method ID %s, existing method is %s",
            authmgrMethodStringGet(method), authmgrMethodStringGet(authmgrCB->globalInfo->authmgrCallbacks[method].method));
    return  FAILURE;
  }

  memset(&authmgrCB->globalInfo->authmgrCallbacks[method], 0, sizeof(authmgrMethodCallbackNotifyMap_t));

  return  SUCCESS;
}

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
RC_t authmgrPortIsAuthorized(uint32 intIfNum)
{
  authmgrPortCfg_t *pCfg;
   AUTHMGR_PORT_STATUS_t portStatus;

  if (authmgrIsValidIntf(intIfNum) ==  TRUE)
  {
    if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    {
      return  SUCCESS;
    }
    if ((authmgrPortAuthControlledPortStatusGet(intIfNum, &portStatus) !=  SUCCESS) ||
        (portStatus !=  AUTHMGR_PORT_STATUS_AUTHORIZED))
    {
      return  FAILURE;
    }
  }
  return  SUCCESS;
}


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
 uchar8 *authmgrMethodTypeToName( AUTHMGR_METHOD_t method)
{
  switch(method)
  {
    case  AUTHMGR_METHOD_NONE:
      return ( uchar8 *)"Auth Method Undefined"; //shiva check with amit

    case  AUTHMGR_METHOD_8021X:
      return ( uchar8 *)"Dot1X"; 

    case  AUTHMGR_METHOD_MAB:
      return ( uchar8 *)"MAB"; 

    default:
      break;
  }
  return ( uchar8 *)"Invalid Auth Method"; 
}


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
RC_t authmgrMaxUsersGet(uint32 intIfNum, uint32 *maxUsers)
{
  RC_t rc =  SUCCESS;
  authmgrPortCfg_t *pCfg;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  switch (pCfg->hostMode)
  {
    case  AUTHMGR_MULTI_HOST_MODE: 
          *maxUsers =  AUTHMGR_MULTI_HOST_MODE_MAX_USERS;
          break;
          
    case  AUTHMGR_SINGLE_AUTH_MODE: 
          *maxUsers =  AUTHMGR_SINGLE_AUTH_MODE_MAX_USERS;
          break;
          
    case  AUTHMGR_MULTI_AUTH_MODE: 
          *maxUsers = pCfg->maxUsers;
          break;

    default:
          rc =  FAILURE;
  }
  (void)osapiReadLockGive(authmgrCB->authmgrCfgRWLock);

  return rc;
}

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
RC_t authmgrIntfIndexGet(uint32 intIfNum)
{
  /* the global  ALL_INTERFACES value is not valid in context of this API */
  if (intIfNum ==  ALL_INTERFACES)
    return  FAILURE;

  if ( TRUE != authmgrIsValidIntf(intIfNum))
    return  FAILURE;

  return  SUCCESS;
}


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
RC_t authmgrIntfIndexGetNext(uint32 intIfNum, uint32 *pNext)
{   
  RC_t rc =  FAILURE;
  uint32 nextIntIfNum = intIfNum;
    
  while ( SUCCESS == nimNextValidIntfNumberByType( PHYSICAL_INTF, nextIntIfNum, &nextIntIfNum))
  { 
    if ( TRUE == authmgrIsValidIntf(nextIntIfNum))
    {
      /* Next authmgr valid interface found. */
      *pNext = nextIntIfNum;
      rc =  SUCCESS;
      break;
    }
  } 
    
  return rc;
}   

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
RC_t authmgrMethodIndexGet(uint32 index)
{
  if ((index >=  AUTHMGR_METHOD_START) &&
      (index <=  AUTHMGR_METHOD_MAX))
    return  SUCCESS;

  return  FAILURE;
}

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
RC_t authmgrMethodIndexGetNext(uint32 index, uint32 *pNext)
{
  if (index >  AUTHMGR_METHOD_MAX)
    return  FAILURE;

  if (pNext ==  NULLPTR)
    return  FAILURE;

  if (index <  AUTHMGR_METHOD_START)
    *pNext =  AUTHMGR_METHOD_START;
  else
    *pNext = index + 1;

  return  SUCCESS;
}

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
RC_t authmgrEntryIndexGet(uint32 intIfNum, uint32 index)
{

  if (( SUCCESS == authmgrIntfIndexGet(intIfNum))&&
      ( SUCCESS == authmgrMethodIndexGet(index)))
  {
    return   SUCCESS;
  }

  return  FAILURE;
}
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
                                  uint32 index, uint32 *pNextIndex)
{
  RC_t       rc =  FAILURE;

  if ( SUCCESS == authmgrIntfIndexGet(intIfNum))
  {
    rc = authmgrMethodIndexGetNext(index,pNextIndex);
  }
  if ( SUCCESS != rc)
  {
    if ( SUCCESS == authmgrIntfIndexGetNext(intIfNum, pNextNum))
    {
      rc = authmgrMethodIndexGetNext(0,pNextIndex);
    }
  }
  return rc;

}
/*********************************************************************
 * @purpose  chechs if method is configured in the order.
 *
 * @param    intIfNum    @b{(input)} internal interface number
 * @param    method @b{(input)} authentication method which is being checked,
 i.e.dot1x/mab/cp
 *
 * @returns   FAILURE_
 * @returns   SUCCESS
 *
 * @comments  This API should only be called from the methods DOT1X and captive portal
              applications only. 
 *
 * @end
 *********************************************************************/

RC_t authmgrIsMethodConfigured(uint32 intIfNum,  AUTHMGR_METHOD_t method, uint32 *entryIndex)
{
  authmgrPortCfg_t *pCfg;
  uint32 methodIndex = 0xFFFF;
  uint32 i;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE)
    return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

 for (i =  AUTHMGR_METHOD_MIN; i <  AUTHMGR_METHOD_MAX; i++)
 {
   if(method == pCfg->methodList[i])
   {
     methodIndex = i;
     break;
   }
 }

 if (methodIndex >=  AUTHMGR_METHOD_MAX)
 {
    return  FAILURE;
 }
  *entryIndex = methodIndex;
 return  SUCCESS;
}

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

 BOOL authmgrIsMethodEnabled(uint32 intIfNum,  AUTHMGR_METHOD_t method, uint32 *entryIndex)
{
  uint32 methodIndex = 0xFF;
  uint32 i;

  if ( TRUE != authmgrIsValidIntf(intIfNum))
      return  FALSE;


 for (i =  AUTHMGR_METHOD_MIN; i <  AUTHMGR_METHOD_MAX; i++)
 {
   if(method == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[i])
   {
     methodIndex = i;
     break;
   }
 }

 if (methodIndex >=  AUTHMGR_METHOD_MAX)
 {
    return  FALSE;
 }
  *entryIndex = methodIndex;
 return  TRUE;
}

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
RC_t authmgrLogicalPortClientAuthStatusGet(uint32 lIntIfNum,  AUTHMGR_PORT_STATUS_t *status)
{
  authmgrLogicalPortInfo_t *logicalPortInfo;
  logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);

  authmgrLogicalPortInfoTakeLock();
  if ( NULLPTR == logicalPortInfo)
  {
    authmgrLogicalPortInfoGiveLock();
    return  FAILURE;
  }

  *status = logicalPortInfo->client.logicalPortStatus;
  authmgrLogicalPortInfoGiveLock();

  return  SUCCESS;
}

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
RC_t authmgrLogicalPortClientAuthStateGet(uint32 lIntIfNum, AUTHMGR_STATES_t *state)
{
  authmgrLogicalPortInfo_t *logicalPortInfo;
  logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);

  authmgrLogicalPortInfoTakeLock();
  if ( NULLPTR == logicalPortInfo)
  {
    authmgrLogicalPortInfoGiveLock();
    return  FAILURE;
  }

  *state = logicalPortInfo->protocol.authState;
  authmgrLogicalPortInfoGiveLock();

  return  SUCCESS;
}

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
RC_t authmgrLogicalPortClientReauthStateGet(uint32 lIntIfNum,  BOOL *state)
{
  authmgrLogicalPortInfo_t *logicalPortInfo;
  logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);

  authmgrLogicalPortInfoTakeLock();
  if ( NULLPTR == logicalPortInfo)
  {
    authmgrLogicalPortInfoGiveLock();
    return  FAILURE;
  }

  *state = logicalPortInfo->protocol.reauth;
  authmgrLogicalPortInfoGiveLock();

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Returns the client authenticated Method for the logical interface
*
* @param    lIntIfNum    @b((input)) the specified interface
* @param    method       @b((output)) authenticating method
*
* @returns   SUCCESS  if success
* @returns   FAILURE  if other failure
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortClientAuthenticatedMethodGet(uint32 lIntIfNum, 
                                                        AUTHMGR_METHOD_t *method)
{
  authmgrLogicalPortInfo_t *logicalPortInfo;
  logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);

  authmgrLogicalPortInfoTakeLock();
  if ( NULLPTR == logicalPortInfo)
  {
    authmgrLogicalPortInfoGiveLock();
    return  FAILURE;
  }

  *method = logicalPortInfo->client.authenticatedMethod;
  authmgrLogicalPortInfoGiveLock();

  return  SUCCESS;
}
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
 * @comments 
 *
 * @end
 *********************************************************************/
RC_t authmgrClientAuthenticatedMethodGet(uint32 intIfNum,  enetMacAddr_t macAddr,  AUTHMGR_METHOD_t *method )
{
  uint32 lIntIfNum;
   enetMacAddr_t tempMac;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE)
    return  FAILURE;

  memset (tempMac.addr, 0,  ENET_MAC_ADDR_LEN);

  if (0 == memcmp(tempMac.addr, macAddr.addr,  ENET_MAC_ADDR_LEN))
  {
    return  FAILURE;
  } 
  /* first find the logical node for this mac address */
  if ( SUCCESS != authmgrMacAddrInfoFind(&macAddr, &lIntIfNum))
  {
    return  FAILURE;
  }

  if ( SUCCESS != authmgrLogicalPortClientAuthenticatedMethodGet(lIntIfNum, method))
  {
    return  FAILURE;
  }
  return  SUCCESS;
}


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
RC_t authmgrPortAttemptsGet(uint32 intIfNum,  AUTHMGR_METHOD_t method, uint32 *numAttempts)
{
  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  switch (method)
  {
    case  AUTHMGR_METHOD_8021X:
         *numAttempts = authmgrCB->globalInfo->authmgrPortStats[intIfNum].dot1x.authEntersAuthenticating;
         break;
    case  AUTHMGR_METHOD_MAB:
         *numAttempts = authmgrCB->globalInfo->authmgrPortStats[intIfNum].mab.authEntersAuthenticating;
         break;
    default:
         break;
  }
  return  SUCCESS;
}

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
RC_t authmgrPortFailedAttemptsGet(uint32 intIfNum,  AUTHMGR_METHOD_t method, uint32 *numAttempts)
{
  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  switch (method)
  {
    case  AUTHMGR_METHOD_8021X:
         *numAttempts = authmgrCB->globalInfo->authmgrPortStats[intIfNum].dot1x.authFailure;
         *numAttempts = *numAttempts + authmgrCB->globalInfo->authmgrPortStats[intIfNum].dot1x.authTimeout;
         break;
    case  AUTHMGR_METHOD_MAB:
         *numAttempts = authmgrCB->globalInfo->authmgrPortStats[intIfNum].mab.authFailure;
         *numAttempts = *numAttempts + authmgrCB->globalInfo->authmgrPortStats[intIfNum].mab.authTimeout;
         break;
    default:
         break;
  }
  return  SUCCESS;
}


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
RC_t authmgrNumClientsGet(uint32 intIfNum, uint32 *pCount) 
{

  if ( TRUE != authmgrIsValidIntf(intIfNum))
  {
      return  FAILURE;
  }

  if(pCount ==  NULLPTR)
  {
    return  FAILURE;
  }

  *pCount = authmgrCB->globalInfo->authmgrPortInfo[intIfNum].numUsers;
  return  SUCCESS;
}

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
                                     AUTHMGR_STATUS_t status,
                                    void *clientParams)
{
  authmgrPortCfg_t *pCfg;
  authmgrAuthRespParams_t      parms;
  authmgrClientStatusInfo_t *temp =  NULLPTR;
  RC_t rc =  SUCCESS;
 

  if ( ALL_INTERFACES != intIfNum)
  {
    /* If it is not a valid interface drop the request */
    if ( authmgrIsValidIntf( intIfNum ) !=  TRUE )
    {
      return  FAILURE;
    }
    if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    {
      return  FAILURE;
    }
  }

  AUTHMGR_IF_NULLPTR_RETURN_LOG(clientParams);

  temp = (authmgrClientStatusInfo_t *)clientParams;

  parms.status = status;
  parms.method = method;

  parms.clientParams = *temp;
  rc = authmgrIssueCmd(authmgrAuthMethodCallbackEvent, intIfNum, (void *)&parms);

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, 0,
      "Updated PAC on user %02X:%02X:%02X:%02X:%02X:%02X with Status %s  method %s, rc %d",
      (unsigned int)parms.clientParams.info.authInfo.macAddr.addr[0], 
      (unsigned int)parms.clientParams.info.authInfo.macAddr.addr[1], 
      (unsigned int)parms.clientParams.info.authInfo.macAddr.addr[2],
      (unsigned int)parms.clientParams.info.authInfo.macAddr.addr[3], 
      (unsigned int)parms.clientParams.info.authInfo.macAddr.addr[4], 
      (unsigned int)parms.clientParams.info.authInfo.macAddr.addr[5],
      authmgrMethodStatusStringGet(parms.status), authmgrMethodStringGet(parms.method), rc);
  /* send the message to the authmgr component */
  AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_API_CALLS,intIfNum,"%s:exiting-%s \n",
                      __FUNCTION__,authmgrIntfIfNameGet(intIfNum));

  return rc;
}

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
RC_t authmgrDot1xCapabilitiesUpdate(uint32 intIfNum, uint32 paeCapabilities)
{
  authmgrPortCfg_t *pCfg;

  /* If it is not a valid interface drop the request */
  if ( authmgrIsValidIntf( intIfNum ) !=  TRUE )
  {
    return  FAILURE;
  }
  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
  {
    return  FAILURE;
  }

  if (paeCapabilities == pCfg->paeCapabilities)
{
  return  SUCCESS;
}

  (void)osapiWriteLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
   pCfg->paeCapabilities = paeCapabilities;
  (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);

  /* send the message to the authmgr component */
  AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_API_CALLS,intIfNum,"%s:exiting-%s \n",
                      __FUNCTION__,authmgrIntfIfNameGet(intIfNum));
  return authmgrIssueCmd(authmgrPaeCapabilitiesEvent, intIfNum, &paeCapabilities);


}

/*********************************************************************
* @purpose  Set administrative mode setting for authmgr
*
* @param    adminMode @b{(input)} authmgr admin mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments    
*       
* @end
*********************************************************************/
RC_t authmgrAdminModeSet(uint32 adminMode)
{

  if (authmgrCB->globalInfo->authmgrCfg->adminMode == adminMode)
    return  SUCCESS;

  authmgrCB->globalInfo->authmgrCfg->adminMode =  ENABLE;

  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  TRUE;

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Get administrative mode setting for authmgr
*
* @param    *adminMode @b{(output)} authmgr admin mode
*
* @returns   SUCCESS
*
* @comments    
*       
* @end
*********************************************************************/
RC_t authmgrAdminModeGet(uint32 *adminMode)
{
  *adminMode = authmgrCB->globalInfo->authmgrCfg->adminMode;

  return  SUCCESS;
}


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
RC_t authmgrPortAuthCountGet(uint32 intIfNum, uint32 *count)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  (void)osapiReadLockTake(authmgrCB->authmgrRWLock,  WAIT_FOREVER);
  *count = authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authCount;
  (void)osapiReadLockGive(authmgrCB->authmgrRWLock);
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Set inactivity period value
*
* @param    intIfNum    @b{(input)} internal interface number
* @param    inActivityPeriod @b{(input)} inActivity period
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments The inactivity period is the time period for which the system will
*,          wait. At the expiry of this timer, the authenticated client is removed, if inactive.
*       
* @end
*********************************************************************/
RC_t authmgrPortAuthInactiveTimerSet(uint32 intIfNum, uint32 inActivityPeriod)
{
  authmgrPortCfg_t *pCfg;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE) 
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;


  if (inActivityPeriod == pCfg->inActivityPeriod)
    return  SUCCESS;

  (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);
  pCfg->inActivityPeriod = inActivityPeriod;
  (void)osapiWriteLockGive(authmgrCB->authmgrCfgRWLock);
  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  TRUE;

  return authmgrIssueCmd(authmgrMgmtPortInactivePeriodSet, intIfNum, &inActivityPeriod);
}

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
RC_t authmgrPortInfoReset(uint32 intIfNum,  BOOL initialize)
{
  authmgrPortCfg_t *pCfg;
  RC_t rc =  SUCCESS;

  if (authmgrIsValidIntf(intIfNum) !=  TRUE)
      return  FAILURE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  FAILURE;

  /* Set the config values to default */
   authmgrBuildDefaultIntfConfigData( NULLPTR, pCfg);

  if (initialize ==  TRUE)
    rc = authmgrIssueCmd(authmgrCtlPortInfoReset, intIfNum, &initialize);

  return rc;
}

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
RC_t authmgrClientDelete( enetMacAddr_t macAddr)
{
  uint32 lIntIfNum =  NULL;

  if ( SUCCESS == authmgrClientMacAddressGet(( enetMacAddr_t *)&macAddr, &lIntIfNum))
  {
    return authmgrIssueCmd(authmgrClientCleanup, lIntIfNum,  NULLPTR);

  }
  else
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_API_CALLS, 0,
                         "%s: Device not found in Auth Mgr db \n",
                         __FUNCTION__);
  }
  return  SUCCESS;
}

