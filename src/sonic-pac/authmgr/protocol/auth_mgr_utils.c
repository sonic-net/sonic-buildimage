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
#include "auth_mgr_db.h"
#include "auth_mgr_client.h"
#include "auth_mgr_timer.h"
#include "auth_mgr_control.h"
#include "auth_mgr_struct.h"
#include "pac_cfg_authmgr.h"
#include "fpSonicUtils.h"
#include "osapi.h"

extern void pacCreateDeleteSocket(char *if_name, bool isCreate);
extern authmgrCB_t *authmgrCB;
/*********************************************************************
* @purpose function to check policy validation based on host mode
*
* @param    hostMode   @b{(input))  hostmode
* @param    *appyPolicy  @b{(input)) bool value
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrHostIsDynamicNodeAllocCheck (AUTHMGR_HOST_CONTROL_t hostMode,
                                            BOOL * valid)
{
  *valid = TRUE;
  return SUCCESS;
}
/*********************************************************************
* @purpose check the nim interface status before invoking any call
*
* @param intIfNum internal interface number
* @return SUCCESS or FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrNimIntfStateCheck (uint32 intIfNum)
{
  INTF_STATES_t state;
  authmgrPortCfg_t *pCfg;
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  state = nimGetIntfState (intIfNum);
  if ((state != INTF_ATTACHED)
      && (state != INTF_ATTACHING) && (state != INTF_DETACHING))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, intIfNum,
                         "%s:authmgrNimIntfStateCheck failed  Port = %s, nim state = %d\n",
                         __FUNCTION__, authmgrIntfIfNameGet(intIfNum), state);
    return FAILURE;
  }

  return SUCCESS;
}
/*********************************************************************
* @purpose function to check policy validation based on host mode
*
* @param intIfNum internal interface number
* @param    *appyPolicy  @b{(input)) bool value
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrStaticFdbEntryValidCheck (uint32 intIfNum,BOOL * valid)
{
  authmgrPortCfg_t *pCfg;
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  switch (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode)
  {
  case AUTHMGR_SINGLE_AUTH_MODE:
  case AUTHMGR_MULTI_AUTH_MODE:
    *valid = TRUE;
    break;
  default:
    *valid = FALSE;
  }
  return SUCCESS;
}
/*********************************************************************
* @purpose function to change the learning of multi host mode
*
* @param intIfNum internal interface number
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrMultiHostHwLearningModify (uint32 intIfNum)
{
  authmgrPortCfg_t *pCfg;
  AUTHMGR_PORT_LEARNING_t learning = AUTHMGR_PORT_LEARNING_NA;
  char8 aliasName[NIM_IF_ALIAS_SIZE + 1];
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  if (SUCCESS != authmgrNimIntfStateCheck (intIfNum))
  {
    return FAILURE;
  }
  if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authCount)
  {
    /*  learning  enable */
    learning = AUTHMGR_PORT_LEARNING_ENABLE;
  }
  else
  {
    /* disable learning */
    learning = AUTHMGR_PORT_LEARNING_DISABLE;
  }
  if (nimGetIntfName (intIfNum, ALIASNAME, aliasName) != SUCCESS)
  {
    LOGF (LOG_SEVERITY_ERROR,
             "Unable to get alias for intf %s", authmgrIntfIfNameGet(intIfNum));
    return FAILURE;
  }

  if (AUTHMGR_PORT_LEARNING_DISABLE == learning)
  {
    learning = (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].unLearnMacPolicy == TRUE) ?
                AUTHMGR_PORT_LEARNING_CPU : learning;
  }

  if (pacCfgIntfLearningModeSet(aliasName, learning) != SUCCESS)
  {
    LOGF (LOG_SEVERITY_ERROR,
             "Unable to set Authmgr learning for port %s", aliasName);
    return FAILURE;
  }
  return SUCCESS;
}

/*********************************************************************
* @purpose function to change the learning of multi host mode
*
* @param intIfNum internal interface number
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrMultAuthHwLearningModify (uint32 intIfNum)
{
  authmgrPortCfg_t *pCfg;
  uchar8 aliasName[NIM_IF_ALIAS_SIZE + 1];
  AUTHMGR_PORT_LEARNING_t learning = AUTHMGR_PORT_LEARNING_DISABLE;

  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  if (SUCCESS != authmgrNimIntfStateCheck (intIfNum))
  {
    return FAILURE;
  }
  /* disable learning */
  if (nimGetIntfName (intIfNum, ALIASNAME, aliasName) != SUCCESS)
  {   
    LOGF (LOG_SEVERITY_ERROR,
             "Unable to get alias for intf %s", authmgrIntfIfNameGet(intIfNum));
    return FAILURE;
  }

  if (AUTHMGR_PORT_LEARNING_DISABLE == learning)
  {
    learning = (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].unLearnMacPolicy == TRUE) ?
                AUTHMGR_PORT_LEARNING_CPU : learning;
  }

  if (pacCfgIntfLearningModeSet(aliasName, learning) != SUCCESS)
  {  
    LOGF (LOG_SEVERITY_ERROR,
             "Unable to set Authmgr learning for port %s", aliasName);
    return FAILURE;
  }
  return SUCCESS;
}
/*********************************************************************
* @purpose function to change the learning of multi host mode
*
* @param intIfNum internal interface number
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrSingleAuthHwLearningModify (uint32 intIfNum)
{
  authmgrPortCfg_t *pCfg;
  uchar8 aliasName[NIM_IF_ALIAS_SIZE + 1];
  AUTHMGR_PORT_LEARNING_t learning = AUTHMGR_PORT_LEARNING_DISABLE;

  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  if (SUCCESS != authmgrNimIntfStateCheck (intIfNum))
  {
    return FAILURE;
  }
  if (nimGetIntfName (intIfNum, ALIASNAME, aliasName) != SUCCESS)
  {   
    LOGF (LOG_SEVERITY_ERROR,
             "Unable to get alias for intf %s", authmgrIntfIfNameGet(intIfNum));
    return FAILURE;
  }

  if (AUTHMGR_PORT_LEARNING_DISABLE == learning)
  {
    learning = (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].unLearnMacPolicy == TRUE) ?
                AUTHMGR_PORT_LEARNING_CPU : learning;
  }

  if (pacCfgIntfLearningModeSet(aliasName, learning) != SUCCESS)
  {  
    LOGF (LOG_SEVERITY_ERROR,
             "Unable to set Authmgr learning for port %s", aliasName);
    return FAILURE;
  }
  return SUCCESS;
}
/*********************************************************************
* @purpose function to change the learning of multi host mode
*
* @param intIfNum internal interface number
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortForceAuthLearningModify (uint32 intIfNum)
{
  authmgrPortCfg_t *pCfg;
  uchar8 aliasName[NIM_IF_ALIAS_SIZE + 1];
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  if (SUCCESS != authmgrNimIntfStateCheck (intIfNum))
  {
    return FAILURE;
  }
  if (nimGetIntfName (intIfNum, ALIASNAME, aliasName) != SUCCESS)
  {   
    LOGF (LOG_SEVERITY_ERROR,
             "Unable to get alias for intf %s", authmgrIntfIfNameGet(intIfNum));
    return FAILURE;
  }
  if (pacCfgIntfLearningModeSet(aliasName, AUTHMGR_PORT_LEARNING_ENABLE) != SUCCESS)
  {  
    LOGF (LOG_SEVERITY_ERROR,
             "Unable to set Authmgr learning for port %s", aliasName);
    return FAILURE;
  }
  return SUCCESS;
}
/*********************************************************************
* @purpose function to check policy validation based on host mode
*
* @param intIfNum internal interface number
* @param entry function pointer entry map 
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrHostCtrlLearninMapEntryGet (uint32 intIfNum,
                                           authmgrHostCtrlLearnMap_t * entry)
{
  static authmgrHostCtrlLearnMap_t authmgrHostCtrlLearnMap[] = {
    {AUTHMGR_MULTI_HOST_MODE, authmgrMultiHostHwLearningModify},
    {AUTHMGR_MULTI_AUTH_MODE, authmgrMultAuthHwLearningModify},
    {AUTHMGR_SINGLE_AUTH_MODE, authmgrSingleAuthHwLearningModify},
  };
  uint32 i = 0;
  authmgrPortCfg_t *pCfg;
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  for (i = 0;
       i <
       (sizeof (authmgrHostCtrlLearnMap) / sizeof (authmgrHostCtrlLearnMap_t));
       i++)
  {
    if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode ==
        authmgrHostCtrlLearnMap[i].hostMode)
    {
      *entry = authmgrHostCtrlLearnMap[i];
      return SUCCESS;
    }
  }
  return FAILURE;
}
/*********************************************************************
* @purpose function to change the learning of multi host mode
*
* @param intIfNum internal interface number
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortAutoLearningModify (uint32 intIfNum)
{
  authmgrPortCfg_t *pCfg;
 RC_t rc = SUCCESS;
  authmgrHostCtrlLearnMap_t entry;
  memset (&entry, 0, sizeof (authmgrHostCtrlLearnMap_t));
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  if (SUCCESS != authmgrNimIntfStateCheck (intIfNum))
  {
    return FAILURE;
  }
  /* get the host mode map entry */
  if (SUCCESS != authmgrHostCtrlLearninMapEntryGet (intIfNum, &entry))
  {
    return FAILURE;
  }
  if (NULLPTR != entry.learnFn)
  {
    rc = entry.learnFn (intIfNum);
  }
  return rc;
}
/*********************************************************************
* @purpose function to change the learning of multi host mode
*
* @param intIfNum internal interface number
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortForceUnauthLearningModify (uint32 intIfNum)
{
  authmgrPortCfg_t *pCfg;
  uchar8 aliasName[NIM_IF_ALIAS_SIZE + 1];
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  if (SUCCESS != authmgrNimIntfStateCheck (intIfNum))
  {
    return FAILURE;
  }
  if (nimGetIntfName (intIfNum, ALIASNAME, aliasName) != SUCCESS)
  {   
    LOGF (LOG_SEVERITY_ERROR,
             "Unable to get alias for intf %d", intIfNum);
    return FAILURE;
  }
  if (pacCfgIntfLearningModeSet(aliasName, AUTHMGR_PORT_LEARNING_DISABLE) != SUCCESS)
  {  
    LOGF (LOG_SEVERITY_ERROR,
             "Unable to set Authmgr learning for port %s", aliasName);
    return FAILURE;
  }
  return SUCCESS;
}
/*********************************************************************
* @purpose function to check policy validation based on host mode
*
* @param intIfNum internal interface number
* @param entry function pointer entry map 
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortCtrlLearninMapEntryGet (uint32 intIfNum,
                                           authmgrPortCtrlLearnMap_t * entry)
{
  static authmgrPortCtrlLearnMap_t authmgrPortCtrlLearnMap[] = {
    {AUTHMGR_PORT_AUTO, authmgrPortAutoLearningModify},
    {AUTHMGR_PORT_FORCE_UNAUTHORIZED, authmgrPortForceUnauthLearningModify},
    {AUTHMGR_PORT_FORCE_AUTHORIZED, authmgrPortForceAuthLearningModify}
  };
  uint32 i = 0;
  authmgrPortCfg_t *pCfg;
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  
  for (i = 0;
       i <
       (sizeof (authmgrPortCtrlLearnMap) / sizeof (authmgrPortCtrlLearnMap_t));
       i++)
  {
    if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode ==
        authmgrPortCtrlLearnMap[i].portControlMode)
    {
      *entry = authmgrPortCtrlLearnMap[i];
      return SUCCESS;
    }
  }
  return FAILURE;
}
/*********************************************************************
* @purpose function to change the learning of an interface
*
* @param intIfNum internal interface number
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortLearningModify (uint32 intIfNum)
{
  authmgrPortCfg_t *pCfg;
 RC_t rc = SUCCESS;
  authmgrPortCtrlLearnMap_t entry;
  memset (&entry, 0, sizeof (authmgrPortCtrlLearnMap_t));
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  if (SUCCESS != authmgrNimIntfStateCheck (intIfNum))
  {
    return FAILURE;
  }
  /* get the host mode map entry */
  if (SUCCESS !=
      authmgrPortCtrlLearninMapEntryGet (intIfNum, &entry))
  {
    return FAILURE;
  }
  if (NULLPTR != entry.learnFn)
  {
    rc = entry.learnFn (intIfNum);
  }
  return rc;
}
/*********************************************************************
* @purpose function to check policy can be removed on based on host mode
*
* @param    intIfNum   @b{(input))  internal interface number
* @param    valid  @b{(input)) TRUE if allowed. 
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrViolationPolicyUnInstallAfterAuthValidCheck (uint32 intIfNum, BOOL * valid)
{
  authmgrPortCfg_t *pCfg;
 RC_t rc = SUCCESS;
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  switch (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode)
  {
  case AUTHMGR_MULTI_HOST_MODE:
    *valid = TRUE;
    break;
  default:
    *valid = FALSE;
  }
  return rc;
}
/*********************************************************************
* @purpose function to check unlearn mac address violation policy validation
*
* @param    intIfNum   @b{(input))  internal interface number
* @param    valid  @b{(input)) TRUE if allowed. 
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrHostViolationPolicyValidCheck (uint32 intIfNum, BOOL * valid)
{
  authmgrPortCfg_t *pCfg;
  uint32 i = 0;
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  if (AUTHMGR_MULTI_HOST_MODE != authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode)
  {
    return FAILURE;
  }
  for (i = 0; i < AUTHMGR_METHOD_MAX; i++)
  {
    if (AUTHMGR_METHOD_MAB == 
        authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[i])
    {
      *valid = TRUE;
      return SUCCESS;
    }
  }
  *valid = FALSE;
  return SUCCESS;
}
/*********************************************************************
* @purpose function to check unlearn mac address violation policy validation
*
* @param    intIfNum   @b{(input))  internal interface number
* @param    valid  @b{(input)) TRUE if allowed. 
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrViolationPolicyValidCheck (uint32 intIfNum, BOOL * valid)
{
  authmgrPortCfg_t *pCfg;
 RC_t rc = SUCCESS;
  BOOL allowed = FALSE;
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  switch (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode)
  {
  case AUTHMGR_SINGLE_AUTH_MODE:
  case AUTHMGR_MULTI_AUTH_MODE:
    *valid = TRUE;
    break;
  case AUTHMGR_MULTI_HOST_MODE:
    if (0 == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authCount)
	{
	  if (SUCCESS == authmgrHostViolationPolicyValidCheck(intIfNum, &allowed))
	  {
		if (allowed)
		{
		  *valid = TRUE;
		}
		else
		{
		  *valid = FALSE;
		}
	  }
	  else
	  {
		return FAILURE;
	  }
	}
    else
    {
      *valid = FALSE;
    }
    break;
  default:
    rc = FAILURE;
    *valid = FALSE;
  }
  return rc;
}
/*********************************************************************
* @purpose function to apply policy validation based on host mode
*
* @param    intIfNum   @b{(input))  intIfNum
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrViolationPolicyApply (uint32 intIfNum)
{
  BOOL apply = FALSE;
  authmgrPortCfg_t *pCfg;
 RC_t rc = SUCCESS;
  AUTHMGR_PORT_VIOLATION_CALLBACK_t mode = AUTHMGR_PORT_VIOLATION_CALLBACK_DISABLE;
  char8 aliasName[NIM_IF_ALIAS_SIZE + 1];

  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  if (SUCCESS != authmgrNimIntfStateCheck (intIfNum))
  {
    return FAILURE;
  }
  if (AUTHMGR_PORT_AUTO ==
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode)
  {
    if (SUCCESS != authmgrViolationPolicyValidCheck (intIfNum, &apply))
    {
      return FAILURE;
    }
  }
  else
  {
    apply = FALSE;
  }
  if (apply)
  {
    mode = AUTHMGR_PORT_VIOLATION_CALLBACK_ENABLE;
  }

  fpGetHostIntfName(intIfNum, aliasName);

  pacCreateDeleteSocket(aliasName, (mode == AUTHMGR_PORT_VIOLATION_CALLBACK_ENABLE) ? TRUE : FALSE);

  /* apply the policy */
  rc = authmgrIhPhyPortViolationCallbackSet (intIfNum, mode);
  return rc;
}

/*********************************************************************
* @purpose set the port settings when auth mgr is not enabled 
*
* @param intIfNum internal interface number
* @return SUCCESS or FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPhysicalPortStatusOpenSet (uint32 intIfNum)
{
  INTF_STATES_t state;
  authmgrPortCfg_t *pCfg;
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  state = nimGetIntfState (intIfNum);
  if ((state != INTF_ATTACHED)
      && (state != INTF_ATTACHING) && (state != INTF_DETACHING))
  {
    return FAILURE;
  }
  /* disable unauth  mac violation policy */
  authmgrIhPhyPortViolationCallbackSet(intIfNum, AUTHMGR_PORT_VIOLATION_CALLBACK_DISABLE); 
  /* Enable learning */
  authmgrPortForceAuthLearningModify(intIfNum);
  authmgrTxCannedSuccess(intIfNum, AUTHMGR_PHYSICAL_PORT);
  return SUCCESS;
}
/*********************************************************************
* @purpose set the port settings when auth mgr is not enabled and blocked 
*
* @param intIfNum internal interface number
* @return SUCCESS or FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPhysicalPortStatusBlockSet (uint32 intIfNum)
{
  INTF_STATES_t state;
  authmgrPortCfg_t *pCfg;
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  state = nimGetIntfState (intIfNum);
  if ((state != INTF_ATTACHED)
      && (state != INTF_ATTACHING) && (state != INTF_DETACHING))
  {
    return FAILURE;
  }
  /* disable unauth  mac violation policy */
  authmgrIhPhyPortViolationCallbackSet(intIfNum, AUTHMGR_PORT_VIOLATION_CALLBACK_DISABLE); 
  /* Disable learning */
  authmgrPortForceUnauthLearningModify(intIfNum);
  authmgrTxCannedFail(intIfNum, AUTHMGR_PHYSICAL_PORT);
  return SUCCESS;
}
/*********************************************************************
* @purpose set the port settings when auth mgr is not enabled 
*
* @param intIfNum internal interface number
* @return SUCCESS or FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPhysicalPortAccessSet (uint32 intIfNum)
{
  INTF_STATES_t state;
  authmgrPortCfg_t *pCfg;
  BOOL status = TRUE;
 RC_t rc = SUCCESS;
  uint32 portLinkState = 0, adminState = 0;
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) != TRUE)
  {
    return FAILURE;
  }
  state = nimGetIntfState (intIfNum);
  if ((state != INTF_ATTACHED)
      && (state != INTF_ATTACHING) && (state != INTF_DETACHING))
  {
    return FAILURE;
  }
  /* Link state */
  rc = nimGetIntfLinkState (intIfNum, &portLinkState);
  if (rc != SUCCESS)
  {
    status = FALSE;
  }
  rc = nimGetIntfAdminState (intIfNum, &adminState);
  if (rc != SUCCESS)
  {
    status = FALSE;
  }
  if ((UP == portLinkState) &&
      (ENABLE == adminState) &&
      (status))
  {
    authmgrPhysicalPortStatusOpenSet(intIfNum);
  }
  else
  {
    authmgrPhysicalPortStatusBlockSet(intIfNum);
  }
  return SUCCESS;
}

RC_t authmgrIsMethodPresentInList(AUTHMGR_METHOD_t method, 
                                     AUTHMGR_METHOD_t *inPut,
                                     uint32 length)
{
  uint32 i = 0;
  AUTHMGR_IF_NULLPTR_RETURN_LOG(inPut);
  
  if (length < AUTHMGR_METHOD_MAX)
  {
   /* check not to read/compare some one else's memory */
    return FAILURE;
  }
  for (i = 0; i < AUTHMGR_METHOD_MAX; i++)
  {
    if (method == *inPut)
    {
      return SUCCESS;
    }
    inPut++;
  }
  return FAILURE;
}
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
uint32 authmgrPhysPortGet(uint32 lIntIfNum)
{
  uint32 physPort;
  AUTHMGR_PORT_GET(physPort, lIntIfNum);
  return physPort;
}
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
char8 * authmgrIntfIfNameGet(uint32 intIfNum)
{
  static  char8 ifName[NIM_IF_ALIAS_SIZE + 1]; 

  memset (&ifName, '\0', sizeof (ifName));
  if ((TRUE != (authmgrIsValidIntf(intIfNum))) ||
      (SUCCESS != nimGetIntfName (intIfNum, ALIASNAME, ifName)))
  {
    LOGF (LOG_SEVERITY_INFO,
                   "Unable to get if name for intf %d", intIfNum);
    osapiStrncpySafe(ifName, "Unknown", strlen("Unknown")+1);
  }
  return ifName; 
}

/*********************************************************************
* @purpose  validate the priority precedence of two methods 
*
* @param    intIfNum  @b{(input))  internal interface number
* @param    method1  @b{(input))   authentication method 
* @param    method2  @b{(input))   authentication method 
*
* @returns  SUCCESS if method 2 is of higher priority than method1 
* @returns  FAILURE  
*
* @comments
*
* @end
*********************************************************************/
uint32 authmgrPriorityPrecedenceValidate(uint32 intIfNum, 
                                            AUTHMGR_METHOD_t method1, 
                                            AUTHMGR_METHOD_t method2)
{
  uint32 j, i;
  BOOL exists = FALSE;
  if (authmgrIsValidIntf (intIfNum) != TRUE)
  {
    return FAILURE;
  }
  if ((AUTHMGR_METHOD_NONE == method1) &&
      (AUTHMGR_METHOD_NONE == method2))
  {
    return FAILURE;
  }
  /* check first if the new method is in the enabled order */
  j = 0;
  for (j = AUTHMGR_METHOD_MIN; j < AUTHMGR_METHOD_MAX; j++)
  {
    if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[j] ==
        method2)
    {
      exists = TRUE;
    }
  }
  if (FALSE == exists)
  {
    return FAILURE;
  }
  j = 0;
  for (j = AUTHMGR_METHOD_MIN; j < AUTHMGR_METHOD_MAX; j++)
  {
    if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority[j] ==
        method1)
    {
      for (i = 0; i < j; i++)
      {
        if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority[i] ==
            method2)
        {
          return SUCCESS;
        }
      }
    }
  }
  return FAILURE;
}



