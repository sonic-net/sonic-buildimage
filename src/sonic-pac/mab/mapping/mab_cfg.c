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

#include "mab_include.h"
#include "mab_client.h"
#include "mab_timer.h"
#include "mab_struct.h"
#include "mab_debug.h"

extern mabBlock_t *mabBlock;

/*********************************************************************
* @purpose  Get the physical port for the logical interface
*
* @param    intIfNum  @b{(input))  internal interface number
*
* @returns  end index for specified interface in mab logical
*           interface list
*
* @comments
*
* @end
*********************************************************************/
uint32 mabPhysPortGet(uint32 lIntIfNum)
{
  uint32 physPort;
  MAB_PORT_GET(physPort, lIntIfNum);
  return physPort;
}

/*********************************************************************
* @purpose  Build default mab config data
*
* @param    ver @((input)) Software version of Config Data
*
* @returns  void
*
* @comments
*
* @end
*********************************************************************/
void mabBuildDefaultConfigData()
{
  uint32 cfgIndex;
  nimConfigID_t configId[ MAB_INTF_MAX_COUNT];

  /* Save the config IDs */
  memset((void *)&configId[0], 0, sizeof(nimConfigID_t) *  MAB_INTF_MAX_COUNT);

  for (cfgIndex = 1; cfgIndex <  MAB_INTF_MAX_COUNT; cfgIndex++)
    NIM_CONFIG_ID_COPY(&configId[cfgIndex], &mabBlock->mabCfg->mabPortCfg[cfgIndex].configId);

  memset((void *)mabBlock->mabCfg, 0, sizeof(mabCfg_t));

  for (cfgIndex = 1; cfgIndex <  MAB_INTF_MAX_COUNT; cfgIndex++)
  {
    mabBuildDefaultIntfConfigData(&configId[cfgIndex], &mabBlock->mabCfg->mabPortCfg[cfgIndex]);
  }
  return;
}

/*********************************************************************
* @purpose  Build default mab port config data
*
* @parms    config Id, the config Id to be placed into the intf config
* @parms    pCfg, a pointer to the interface structure
*
* @returns  none
*
*
* @end
*********************************************************************/
void mabBuildDefaultIntfConfigData(nimConfigID_t *configId, mabPortCfg_t *pCfg)
{
  pCfg->mabEnabled    = FD_MAB_PORT_MAB_ENABLED;
  pCfg->mabAuthType   = FD_MAB_PORT_MAB_AUTH_TYPE;
}

/*********************************************************************
* @purpose  Apply mab config data
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
RC_t mabApplyConfigData(void)
{
  return (mabIssueCmd(mabMgmtApplyConfigData,  NULL,  NULLPTR));
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
RC_t mabPortReset(uint32 intIfNum)
{
  mabPortInfoInitialize(intIfNum, FALSE);
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Initialize the Dot1x Port Structure with Default Values
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
RC_t mabPortInfoInitialize(uint32 intIfNum,  BOOL flag)
{
  mabPortCfg_t *pCfg;
  uint32 linkState = 0, adminState = 0;

  if (mabIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  SUCCESS;

  memset(&mabBlock->mabPortInfo[intIfNum], 0, sizeof(mabPortInfo_t));

  (void)osapiWriteLockGive(mabBlock->mabRWLock);


  (void)osapiWriteLockTake(mabBlock->mabRWLock,  WAIT_FOREVER);
 
  mabAppTimerDeInitCheck();

  mabBlock->mabPortInfo[intIfNum].portControlMode =  AUTHMGR_PORT_AUTO;
  mabBlock->mabPortInfo[intIfNum].hostMode =  AUTHMGR_MULTI_AUTH_MODE;

  mabBlock->mabPortInfo[intIfNum].maxUsers           =  MAB_MAX_USERS_PER_PORT;
  mabBlock->mabPortInfo[intIfNum].numUsers           = 0;

  mabBlock->mabPortInfo[intIfNum].currentId          = 0;
  mabBlock->mabPortInfo[intIfNum].initialize         =  FALSE;

  if ( (nimGetIntfLinkState(intIfNum, &linkState) ==  SUCCESS) && (linkState ==  UP) &&
       (nimGetIntfAdminState(intIfNum, &adminState) ==  SUCCESS) && (adminState == ENABLE) )
  {
    mabBlock->mabPortInfo[intIfNum].portEnabled =  TRUE;
    mabBlock->mabPortInfo[intIfNum].mabEnabled = pCfg->mabEnabled;
  }
  else
  {
    mabBlock->mabPortInfo[intIfNum].portEnabled =  FALSE;
    mabBlock->mabPortInfo[intIfNum].mabEnabled =  DISABLE;
  }

  if ( ENABLE == mabBlock->mabPortInfo[intIfNum].mabEnabled)
  {
    /* Register for time ticks with appTimer */
    if ( NULLPTR == mabBlock->mabTimerCB)
    {
      mabBlock->mabTimerCB =  appTimerInit( MAB_COMPONENT_ID, mabTimerExpiryHdlr,
           NULLPTR,  APP_TMR_1SEC, mabBlock->mabAppTimerBufferPoolId);
    }
  }

  mabBlock->mabPortInfo[intIfNum].authCount  = 0;

  /* Copy config data into operational data */
  mabBlock->mabPortInfo[intIfNum].maxUsers =  AUTHMGR_PORT_MAX_MAC_USERS;

  return  SUCCESS;
}
