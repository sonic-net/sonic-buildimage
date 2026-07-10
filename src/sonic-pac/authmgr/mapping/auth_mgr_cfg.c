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
#include "auth_mgr_client.h"
#include "auth_mgr_struct.h"

extern authmgrCB_t *authmgrCB;
extern void authmgrDebugCfgUpdate(void);

/*********************************************************************
* @purpose  Checks if authmgr user config data has changed
*
* @param    void
*
* @returns   TRUE
* @returns   FALSE
*
* @comments none
*
* @end
*********************************************************************/
 BOOL authmgrHasDataChanged(void)
{
  return authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged;
}

/*********************************************************************
* @purpose  Reset authmgr user config data flag
*
* @param    void
*
* @returns  none
*
* @comments none
*
* @end
*********************************************************************/
void authmgrResetDataChanged(void)
{
  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  FALSE;
  return;
}
/*********************************************************************
* @purpose  Print the current authmgr config values to serial port
*
* @param    none
*
* @returns   SUCCESS
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrCfgDump(void)
{
  char8 buf[32];
  uint32 i;
  uint32 intIfNum = 0;
  authmgrPortCfg_t *pCfg;
  nimConfigID_t configIdNull;

  memset(&configIdNull, 0, sizeof(nimConfigID_t));

  sysapiPrintf("\n");
  sysapiPrintf("AUTHMGR\n");
  sysapiPrintf("=====\n");

  if (authmgrCB->globalInfo->authmgrCfg->authmgrLogTraceMode ==  ENABLE)
    osapiSnprintf(buf, sizeof(buf),"Enable");
  else
    osapiSnprintf(buf,sizeof(buf),"Disable");
  sysapiPrintf("Log Trace Mode - %s\n\n", buf);

  if(authmgrCB->globalInfo->authmgrCfg->vlanAssignmentMode ==  ENABLE)
    osapiSnprintf(buf,sizeof(buf),"Enable");
  else
    osapiSnprintf(buf, sizeof(buf), "Disable");
  sysapiPrintf("Vlan Assignment Mode - %s\n\n", buf);

  sysapiPrintf("Interface configuration:\n");
  for (i = 1; i <  AUTHMGR_INTF_MAX_COUNT; i++)
  {
    if (authmgrIntfIsConfigurable(intIfNum, &pCfg) ==  TRUE)
    {
      switch (pCfg->portControlMode)
      {
      case  AUTHMGR_PORT_FORCE_UNAUTHORIZED:
        osapiSnprintf(buf,sizeof(buf), "forceUnauthorized");
        break;
      case  AUTHMGR_PORT_AUTO:
        osapiSnprintf(buf, sizeof(buf), "auto");
        break;
      case  AUTHMGR_PORT_FORCE_AUTHORIZED:
        osapiSnprintf(buf, sizeof(buf), "forceAuthorized");
        break;
      default:
        osapiSnprintf(buf,sizeof(buf), "N/A");
        break;
      }

      sysapiPrintf("  Port Control Mode:       %s(%d)\n", buf, pCfg->portControlMode);
      sysapiPrintf("  Quiet Period:            %d\n", pCfg->quietPeriod);
      sysapiPrintf("  ReAuth Period:           %d\n", pCfg->reAuthPeriod);
      sysapiPrintf("  Inactivity Period:       %d\n", pCfg->inActivityPeriod);

      switch (pCfg->reAuthEnabled)
      {
      case  TRUE:
        osapiSnprintf(buf, sizeof(buf),"True");
        break;
      case  FALSE:
        osapiSnprintf(buf, sizeof(buf), "False");
        break;
      default:
        osapiSnprintf(buf,sizeof(buf), "N/A");
        break;
      }
      sysapiPrintf("  ReAuth Enabled:          %s\n", buf);

      sysapiPrintf("\n");
    }
  }

  sysapiPrintf("=============\n");
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Build default authmgr config data
*
* @param    none
*
* @returns  void
*
* @comments
*
* @end
*********************************************************************/
void authmgrBuildDefaultConfigData(void)
{
  uint32 cfgIndex;
  nimConfigID_t configId[ AUTHMGR_INTF_MAX_COUNT];

  /* Save the config IDs */
  memset((void *)&configId[0], 0, sizeof(nimConfigID_t) *  AUTHMGR_INTF_MAX_COUNT);
  for (cfgIndex = 1; cfgIndex <  AUTHMGR_INTF_MAX_COUNT; cfgIndex++)
    NIM_CONFIG_ID_COPY(&configId[cfgIndex], &authmgrCB->globalInfo->authmgrCfg->authmgrPortCfg[cfgIndex].configId);

  memset((void *)authmgrCB->globalInfo->authmgrCfg, 0, sizeof(authmgrCfg_t));

  for (cfgIndex = 1; cfgIndex <  AUTHMGR_INTF_MAX_COUNT; cfgIndex++)
  {
    authmgrBuildDefaultIntfConfigData(&configId[cfgIndex], &authmgrCB->globalInfo->authmgrCfg->authmgrPortCfg[cfgIndex]);
  }
  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  FALSE;

  authmgrCB->globalInfo->authmgrCfg->adminMode = FD_AUTHMGR_ADMIN_MODE;
  authmgrCB->globalInfo->authmgrCfg->authmgrLogTraceMode = FD_AUTHMGR_LOG_TRACE_MODE;
  authmgrCB->globalInfo->authmgrCfg->vlanAssignmentMode = FD_AUTHMGR_VLAN_ASSIGN_MODE;
  authmgrCB->globalInfo->authmgrCfg->portControlMode = FD_AUTHMGR_PORT_MODE;
  authmgrCB->globalInfo->authmgrCfg->hostMode = FD_AUTHMGR_HOST_MODE;


  return;
}

/*********************************************************************
* @purpose  Build default authmgr port config data
*
* @parms    config Id, the config Id to be placed into the intf config
* @parms    pCfg, a pointer to the interface structure
*
* @returns  none
*
*
* @end
*********************************************************************/
void authmgrBuildDefaultIntfConfigData(nimConfigID_t *configId, authmgrPortCfg_t *pCfg)
{
  uint32 i = 0;
   AUTHMGR_METHOD_t buildFdList[] = FD_AUTHMGR_LIST;

  pCfg->portControlMode = FD_AUTHMGR_PORT_MODE;
  pCfg->hostMode = FD_AUTHMGR_HOST_MODE;
  pCfg->intfConfigMask = 0;
  pCfg->quietPeriod = FD_AUTHMGR_RESTART_TIMER_VAL;
  pCfg->reAuthPeriod = FD_AUTHMGR_PORT_REAUTH_PERIOD;
  pCfg->reAuthEnabled = FD_AUTHMGR_PORT_REAUTH_ENABLED;
  pCfg->reAuthPeriodServer = FD_AUTHMGR_PORT_REAUTH_PERIOD_FROM_SERVER;
  pCfg->maxUsers = FD_AUTHMGR_PORT_MAX_USERS;
  pCfg->maxAuthAttempts = FD_AUTHMGR_RADIUS_MAX_AUTH_ATTEMPTS;
  pCfg->inActivityPeriod = FD_AUTHMGR_PORT_INACTIVITY_PERIOD;
  pCfg->paeCapabilities = FD_AUTHMGR_PORT_PAE_CAPABILITIES;

  for (i = 0; i <  AUTHMGR_METHOD_MAX; i++)
  {
    pCfg->methodList[i] = buildFdList[i];
    pCfg->priorityList[i] = buildFdList[i];
  }

  authmgrCB->globalInfo->authmgrCfg->cfgHdr.dataChanged =  FALSE;
}

/*********************************************************************
* @purpose  Apply authmgr config data
*
* @param    void
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrApplyConfigData(void)
{
  return (authmgrIssueCmd(authmgrMgmtApplyConfigData,  NULL,  NULLPTR));
}

/*********************************************************************
* @purpose  Apply authmgr config data to specified interface
*
* @param    intIfNum     @b{(input)) internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrApplyPortConfigData(uint32 intIfNum)
{
  return (authmgrIssueCmd(authmgrMgmtApplyPortConfigData,  NULL,  NULLPTR));
}

/*********************************************************************
* @purpose  Fill in default values and set the port state
*
* @param    intIfNum  @b{(input))  internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortReset(uint32 intIfNum)
{
  authmgrPortInfoInitialize(intIfNum, FALSE);
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Checks if authmgr debug config data has changed
*
* @param    void
*
* @returns   TRUE or  FALSE
*
* @notes    none
*
* @end
*********************************************************************/
 BOOL authmgrDebugHasDataChanged(void)
{
  return authmgrCB->globalInfo->authmgrDebugCfg.hdr.dataChanged;
}

/*********************************************************************
* @purpose  Build default authmgr config data
*
* @param    ver   Software version of Config Data
*
* @returns  void
*
* @notes
*
* @end
*********************************************************************/
void authmgrDebugBuildDefaultConfigData(uint32 ver)
{
  authmgrCB->globalInfo->authmgrDebugCfg.hdr.dataChanged =  FALSE;

  /* set all flags to  FALSE */
  memset(&authmgrCB->globalInfo->authmgrDebugCfg.cfg, 0, sizeof(authmgrCB->globalInfo->authmgrDebugCfg.cfg));
}

