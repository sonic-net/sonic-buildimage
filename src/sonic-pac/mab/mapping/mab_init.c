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
#include "mab_debug.h"
#include "mab_struct.h"
#include "mab_radius.h"
#include "osapi_sem.h"

/*dummy declaration to resolve linking errors */
#define platAuthMgrMaxUsersGet()         (512)
static uint32 platIntfMaxCountGet (void)
{
  return  MAX_INTERFACE_COUNT;
}

mabBlock_t *mabBlock =  NULLPTR;

 BOOL mabIsRestartTypeWarm()
{
  return mabBlock->warmRestart;
}

void mabWarmRestartTypeSet(  BOOL warmType)
{
  mabBlock->warmRestart = warmType;
}

/*********************************************************************
*
* @purpose  System Initialization for mab 
*
* @param    none
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if other failure
*
* @comments  none
*
* @end
*********************************************************************/
RC_t mabInit(void)
{
  mabBlock = osapiMalloc( MAB_COMPONENT_ID, sizeof(mabBlock_t));

  if ( NULLPTR == mabBlock)
  {
    return  FAILURE;
  }

  memset(mabBlock, 0, sizeof(mabBlock_t));
  /*semaphore creation for task protection over the common data*/
  mabBlock->mabTaskSyncSema = osapiSemaCCreate( OSAPI_SEM_Q_FIFO, OSAPI_SEM_EMPTY);
  if (mabBlock->mabTaskSyncSema ==  NULL)
  {
     LOGF( LOG_SEVERITY_NOTICE,
        "Unable to create mab task semaphore");
    return  FAILURE;
  }

  mabBlock->mabRadiusSrvrTaskSyncSema = osapiSemaCCreate( OSAPI_SEM_Q_FIFO, OSAPI_SEM_EMPTY);
  if (mabBlock->mabRadiusSrvrTaskSyncSema ==  NULL)
  {
     LOGF( LOG_SEVERITY_NOTICE,
        "Unable to create mab radius server task semaphore");
    return  FAILURE;
  }

  mabBlock->mabQueue = (void *)osapiMsgQueueCreate("mabQueue", MAB_MSG_COUNT, (uint32)sizeof(mabMsg_t));
  if (mabBlock->mabQueue ==  NULLPTR)
  {
     LOGF( LOG_SEVERITY_INFO,
            "mabInit: msgQueue creation error.\n");
    return  FAILURE;
  }

  if (mabStartTasks() !=  SUCCESS)
  {
    return  FAILURE;
  }

  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  System Init Undo for mab 
*
* @param    none
*
* @comments  none
*
* @end
*********************************************************************/
void mabInitUndo()
{
  if (mabBlock->mabQueue !=  NULLPTR)
    osapiMsgQueueDelete(mabBlock->mabQueue);

  (void)osapiRWLockDelete(mabBlock->mabRWLock);

  if ( NULL != mabBlock->mabTaskSyncSema)
  {
    (void)osapiSemaDelete(mabBlock->mabTaskSyncSema);
  }

  if ( NULL != mabBlock->mabRadiusSrvrTaskSyncSema)
  {
    (void)osapiSemaDelete(mabBlock->mabRadiusSrvrTaskSyncSema);
  }

  if (mabBlock->mabTaskId != 0)
    osapiTaskDelete(mabBlock->mabTaskId);

  if ( NULLPTR != mabBlock)
  {
    osapiFree(MAB_COMPONENT_ID, mabBlock);
  }
}

/*********************************************************************
* @purpose  This function performs the Phase 1 initialization.
*
* @param    none
*
* @returns   SUCCESS - There were no errors.
*
* @returns   ERROR   - There were errors.
*
* @comments  none
*
* @end
*********************************************************************/
RC_t mabInitPhase1Process(void)
{
  RC_t mabRC,rc;
  uint32 mabMaxNodes;

  mabRC     =  SUCCESS;

  mabBlock->mabCfg        = osapiMalloc( MAB_COMPONENT_ID, sizeof(mabCfg_t));
  mabBlock->mabPortInfo   = osapiMalloc( MAB_COMPONENT_ID, sizeof(mabPortInfo_t) *  MAB_INTF_MAX_COUNT);

  rc = mabLogicalPortInfoDBInit(platAuthMgrMaxUsersGet());

  mabBlock->mabPortStats  = osapiMalloc( MAB_COMPONENT_ID, sizeof(mabPortStats_t) * platIntfMaxCountGet());
  mabBlock->mabMapTbl     = osapiMalloc( MAB_COMPONENT_ID, sizeof(uint32) * platIntfMaxCountGet());

  /* initialize Mac address database*/
  rc = mabMacAddrInfoDBInit(platAuthMgrMaxUsersGet());

  mabMaxNodes = ((2*platIntfMaxCountGet()) + platAuthMgrMaxUsersGet() +1);

  /* Allocate buffer pool for App Timer */
  if (bufferPoolInit( MAB_COMPONENT_ID, 2 * mabMaxNodes,  APP_TMR_NODE_SIZE, 
                     "mab Timer Bufs", &mabBlock->mabAppTimerBufferPoolId) !=  SUCCESS)
  {
    mabRC   =  ERROR;
    return mabRC;
  }

  if ((mabBlock->mabCfg       ==  NULLPTR) ||
      (mabBlock->mabPortInfo  ==  NULLPTR) ||
      (rc ==  FAILURE)                     ||
      (mabBlock->mabPortStats ==  NULLPTR) ||
      (mabBlock->mabMapTbl    ==  NULLPTR))
  {
    mabRC      =  ERROR;
    return mabRC;
  }

  /* Zero bitmasks to indicate no interfaces are enabled */
  memset((void *)mabBlock->mabCfg, 0, sizeof(mabCfg_t));
  memset((void *)mabBlock->mabPortInfo, 0, sizeof(mabPortInfo_t) *  MAB_INTF_MAX_COUNT);
  memset((void *)mabBlock->mabPortStats, 0, sizeof(mabPortStats_t) * platIntfMaxCountGet());
  memset((void *)mabBlock->mabMapTbl, 0, sizeof(uint32) * platIntfMaxCountGet());

  return mabRC;
}

/*********************************************************************
* @purpose  This function performs the Phase 2 initialization.
*
* @param    none
*
* @returns   SUCCESS - There were no errors.
*
* @returns   ERROR   - There were errors.
*
* @comments  none
*
* @end
*********************************************************************/
RC_t mabInitPhase2Process(void)
{
  RC_t mabRC =  SUCCESS;

  if (nimRegisterIntfChange( MAB_COMPONENT_ID, mabIntfChangeCallback,
                            mabIntfStartupCallback, NIM_STARTUP_PRIO_MAB) !=  SUCCESS)
  {
     LOGF( LOG_SEVERITY_INFO,
            "mab: Unable to register with NIM\n");
    mabRC       =  ERROR;
    return mabRC;
  }

  return mabRC;
}

/*********************************************************************
* @purpose  This function performs the Phase 3 initialization.
*
* @param    none
*
* @returns   SUCCESS - There were no errors.
*
* @returns   ERROR   - There were errors.
*
* @comments  none
*
* @end
*********************************************************************/
RC_t mabInitPhase3Process( BOOL warmRestart)
{
  RC_t mabRC;
  mabRC     =  SUCCESS;

  if (warmRestart ==  TRUE)
  {
    mabBlock->mabSwitchoverInProgress =  TRUE;
  }

  mabBuildDefaultConfigData();

  if (mabCtlApplyConfigData() !=  SUCCESS)
  {
    mabRC     =  ERROR;
  }

  return  mabRC;
}
