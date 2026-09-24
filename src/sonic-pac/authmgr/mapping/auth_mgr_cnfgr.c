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
#include "auth_mgr_struct.h"
#include "auth_mgr_debug.h"
#include "auth_mgr_auth_method.h"
#include "auth_mgr_ih.h"
#include "auth_mgr_vlan_db.h"
#include "osapi_sem.h"

authmgrCB_t *authmgrCB =  NULLPTR;

authmgrCnfgrState_t authmgrCnfgrState;
authMgrVlanDbData_t *authmgrVlanStateDb; /* Vlan operational state cache. */
authMgrVlanDbData_t *authmgrVlanCfgDb; /* Vlan configured state cache. */

#define platAuthMgrMaxUsersGet()         (512)

static uint32 platIntfMaxCountGet (void)
{
  return  MAX_INTERFACE_COUNT;
}

/*********************************************************************
*
* @purpose  System Initialization for authmgr component
*
* @param    none

* @returns   SUCCESS, if success
* @returns   FAILURE, if other failure
*
* @comments  none
*
* @end
*********************************************************************/
RC_t authmgrInit (void)
{
  /* invoke dependent libraries */

  authmgrCB = osapiMalloc ( AUTHMGR_COMPONENT_ID, sizeof (authmgrCB_t));

  if ( NULLPTR == authmgrCB)
  {
    return  FAILURE;
  }

  memset (authmgrCB, 0, sizeof (authmgrCB_t));

  /*semaphore creation for task protection over the common data*/
  authmgrCB->authmgrTaskSyncSema = osapiSemaCCreate( OSAPI_SEM_Q_FIFO, OSAPI_SEM_EMPTY);
  if (authmgrCB->authmgrTaskSyncSema ==  NULL)
  {
     LOGF( LOG_SEVERITY_NOTICE,
        "Unable to create authmgr task semaphore");
    return  FAILURE;
  }

  authmgrCB->authmgrBulkQueue =
    (void *) osapiMsgQueueCreate ("authmgrBulkQueue", AUTHMGR_MSG_COUNT,
                                  (uint32) sizeof (authmgrBulkMsg_t));
  if (authmgrCB->authmgrBulkQueue ==  NULLPTR)
  {
     LOGF ( LOG_SEVERITY_INFO,
             "authmgrInit: Bulk msgQueue creation error.\n");
    return  FAILURE;
  }

  authmgrCB->authmgrVlanEventQueue =
    (void *) osapiMsgQueueCreate ("authmgrVlanEventQueue", AUTHMGR_VLAN_MSG_COUNT,
                                  (uint32) sizeof (authmgrVlanMsg_t));
  if (authmgrCB->authmgrVlanEventQueue ==  NULLPTR)
  {
     LOGF ( LOG_SEVERITY_INFO,
             "authmgrInit: VLAN event msgQueue creation error.\n");
    return  FAILURE;
  }

  authmgrCB->authmgrQueue =
    (void *) osapiMsgQueueCreate ("authmgrQueue", AUTHMGR_MSG_COUNT,
                                  (uint32) sizeof (authmgrMsg_t));
  if (authmgrCB->authmgrQueue ==  NULLPTR)
  {
     LOGF ( LOG_SEVERITY_INFO,
             "authmgrInit: msgQueue creation error.\n");
    return  FAILURE;
  }

  if (authmgrStartTasks () !=  SUCCESS)
  {
    return  FAILURE;
  }

  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  System Init Undo for authmgr component
*
* @param    none
*
* @comments  none
*
* @end
*********************************************************************/
void authmgrInitUndo ()
{
  if (authmgrCB->authmgrQueue !=  NULLPTR)
  {
    osapiMsgQueueDelete (authmgrCB->authmgrQueue);
  }

  if (authmgrCB->authmgrBulkQueue !=  NULLPTR)
  {
    osapiMsgQueueDelete (authmgrCB->authmgrBulkQueue);
  }

  if (authmgrCB->authmgrVlanEventQueue !=  NULLPTR)
  {
    osapiMsgQueueDelete (authmgrCB->authmgrVlanEventQueue);
  }

  if ( NULL != authmgrCB->authmgrTaskSyncSema)
  {
    (void)osapiSemaDelete(authmgrCB->authmgrTaskSyncSema);
  }

  if ( NULL != authmgrCB->authmgrSrvrTaskSyncSema)
  {
    (void)osapiSemaDelete(authmgrCB->authmgrSrvrTaskSyncSema);
  }

  (void) osapiRWLockDelete (authmgrCB->authmgrRWLock);
  (void) osapiRWLockDelete (authmgrCB->authmgrCfgRWLock);


  if (authmgrCB->authmgrTaskId != 0)
  {
    osapiTaskDelete (authmgrCB->authmgrTaskId);
  }

  if (authmgrCB->authmgrSrvrTaskId != 0)
  {
    osapiTaskDelete (authmgrCB->authmgrSrvrTaskId);
  }

  if ( NULLPTR != authmgrCB)
  {
    osapiFree ( AUTHMGR_COMPONENT_ID, authmgrCB);
  }

  authmgrCnfgrState = AUTHMGR_PHASE_INIT_0;
}

/*********************************************************************
* @purpose  This function process the configurator control commands/request
*           pair Init Phase 1.
*
* @param    none

* @param    pReason   - @b{(output)}  Reason if  ERROR.
*
* @returns   SUCCESS - There were no errors. Response is available.
*
* @returns   ERROR   - There were errors. Reason code is available.
*
* @comments  The following are valid response:
*            CNFGR_CMD_COMPLETE
*
* @commets  The following are valid error reason code:
*            CNFGR_ERR_RC_FATAL
*            CNFGR_ERR_RC_LACK_OF_RESOURCES
*
* @end
*********************************************************************/
RC_t authmgrCnfgrInitPhase1Process (void)
{
  RC_t authmgrRC, rc;
  RC_t rc1 =  SUCCESS;
  RC_t rc2 =  SUCCESS;
  uint32 authmgrMaxTimerNodes;

  authmgrRC =  SUCCESS;

  printf("%s:%d\r\n", __FUNCTION__, __LINE__);

  authmgrCB->globalInfo =
    osapiMalloc ( AUTHMGR_COMPONENT_ID, sizeof (authmgrGlobalInfo_t));

  if ( NULLPTR == authmgrCB->globalInfo)
  {
    authmgrRC =  ERROR;
    return authmgrRC;
  }

  authmgrCB->globalInfo->authmgrCfg =
    osapiMalloc ( AUTHMGR_COMPONENT_ID, sizeof (authmgrCfg_t));

  authmgrCB->globalInfo->authmgrPortInfo =
    osapiMalloc ( AUTHMGR_COMPONENT_ID,
                 sizeof (authmgrPortInfo_t) *  AUTHMGR_INTF_MAX_COUNT);

  rc = authmgrLogicalPortInfoDBInit (platAuthMgrMaxUsersGet ());

  authmgrCB->globalInfo->authmgrPortSessionStats =
    osapiMalloc ( AUTHMGR_COMPONENT_ID,
                 (sizeof (authmgrPortSessionStats_t) *
                  (platIntfMaxCountGet () + 1)));

  authmgrCB->globalInfo->authmgrPortStats =
    osapiMalloc ( AUTHMGR_COMPONENT_ID,
                 sizeof (authmgrPortStats_t) * platIntfMaxCountGet ());

  authmgrCB->globalInfo->authmgrMapTbl =
    osapiMalloc ( AUTHMGR_COMPONENT_ID,
                 sizeof (uint32) * platIntfMaxCountGet ());
 
  authmgrVlanStateDb = osapiMalloc( AUTHMGR_COMPONENT_ID,
                                   sizeof (authMgrVlanDbData_t));

  authmgrVlanCfgDb = osapiMalloc( AUTHMGR_COMPONENT_ID,
                                   sizeof (authMgrVlanDbData_t));

  if (( NULLPTR == authmgrVlanStateDb) || ( NULLPTR == authmgrVlanCfgDb))
  {
    authmgrRC =  ERROR;
    return authmgrRC;
  }

  /* initialize Mac address database */

  rc = authmgrMacAddrInfoDBInit (platAuthMgrMaxUsersGet ());

  /* Two timers per client; one for protocol and one for interim accounting.
     30 seconds timer needed for voice clients are not accounted for presently. */
  authmgrMaxTimerNodes = (2 * platAuthMgrMaxUsersGet());

  /* Allocate buffer pool for App Timer */
  if (bufferPoolInit
      ( AUTHMGR_COMPONENT_ID, authmgrMaxTimerNodes,  APP_TMR_NODE_SIZE,
       "AUTHMGR Timer Bufs",
       &authmgrCB->globalInfo->authmgrAppTimerBufferPoolId) !=  SUCCESS)
  {
    authmgrRC =  ERROR;

    return authmgrRC;
  }

  if ((authmgrCB->globalInfo->authmgrCfg ==  NULLPTR) ||
      (authmgrCB->globalInfo->authmgrPortInfo ==  NULLPTR) ||
      (rc ==  FAILURE) ||
      (rc1 ==  FAILURE) ||
      (rc2 ==  FAILURE) ||
      (authmgrCB->globalInfo->authmgrPortStats ==  NULLPTR) ||
      (authmgrCB->globalInfo->authmgrMapTbl ==  NULLPTR) ||
      (authmgrCB->globalInfo->authmgrPortSessionStats ==  NULLPTR))
  {
    authmgrRC =  ERROR;

    return authmgrRC;
  }

  /* Zero bitmasks to indicate no interfaces are enabled */
  memset ((void *) authmgrCB->globalInfo->authmgrCfg, 0, sizeof (authmgrCfg_t));
  memset ((void *) authmgrCB->globalInfo->authmgrPortInfo, 0,
          sizeof (authmgrPortInfo_t) *  AUTHMGR_INTF_MAX_COUNT);
  memset ((void *) authmgrCB->globalInfo->authmgrPortStats, 0,
          sizeof (authmgrPortStats_t) * platIntfMaxCountGet ());
  memset ((void *) authmgrCB->globalInfo->authmgrMapTbl, 0,
          sizeof (uint32) * platIntfMaxCountGet ());
  memset ((void *) authmgrCB->globalInfo->authmgrPortSessionStats, 0,
          sizeof (authmgrPortSessionStats_t) * ( MAX_INTERFACE_COUNT + 1));

  authmgrCnfgrState = AUTHMGR_PHASE_INIT_1;

  printf("%s:%d\r\n", __FUNCTION__, __LINE__);

  return authmgrRC;
}

/*********************************************************************
* @purpose  This function process the configurator control commands/request
*           pair Init Phase 2.
*
* @param    pResponse - @b{(output)}  Response if  SUCCESS.
*
* @param    pReason   - @b{(output)}  Reason if  ERROR.
*
* @returns   SUCCESS - There were no errors. Response is available.
*
* @returns   ERROR   - There were errors. Reason code is available.
*
* @comments  The following are valid response:
*            CNFGR_CMD_COMPLETE
*
* @comments  The following are valid error reason code:
*            CNFGR_ERR_RC_FATAL
*            CNFGR_ERR_RC_LACK_OF_RESOURCES
*
* @end
*********************************************************************/
RC_t authmgrCnfgrInitPhase2Process (void)
{
  RC_t authmgrRC;
  printf("%s:%d\r\n", __FUNCTION__, __LINE__);

  authmgrRC =  SUCCESS;

  authmgrCnfgrState = AUTHMGR_PHASE_INIT_2;
  
  if (nimRegisterIntfChange ( AUTHMGR_COMPONENT_ID, authmgrIntfChangeCallback,
                             authmgrIntfStartupCallback, NIM_STARTUP_PRIO_AUTHMGR) 
                                 !=  SUCCESS)
  {
    return  FAILURE;
  }

  if ( SUCCESS != authmgrEventCallbackRegister( AUTHMGR_METHOD_8021X,
         NULL,  NULL,
        authmgrDot1xEventSend, authmgrDot1xIntfAdminModeGet,
         NULL))
	 {
    return  FAILURE;
	 }


  if ( SUCCESS != authmgrEventCallbackRegister( AUTHMGR_METHOD_MAB,
         NULL,  NULL,
        authmgrMabEventSend, authmgrMabIntfAdminModeGet,
         NULL))
	 {
    return  FAILURE;
	 }

  return authmgrRC;
}

/*********************************************************************
* @purpose  This function process the configurator control commands/request
*           pair Init Phase 3.
*
* @param    pResponse - @b{(output)}  Response if  SUCCESS.
*
* @param    pReason   - @b{(output)}  Reason if  ERROR.
*
* @returns   SUCCESS - There were no errors. Response is available.
*
* @returns   ERROR   - There were errors. Reason code is available.
*
* @comments  The following are valid response:
*            CNFGR_CMD_COMPLETE
*
* @comments  The following are valid error reason code:
*            CNFGR_ERR_RC_FATAL
*            CNFGR_ERR_RC_LACK_OF_RESOURCES
*
* @end
*********************************************************************/
RC_t authmgrCnfgrInitPhase3Process ( BOOL warmRestart)
{
  RC_t authmgrRC;

  authmgrRC =  SUCCESS;
  printf("%s:%d\r\n", __FUNCTION__, __LINE__);

  authmgrBuildDefaultConfigData();

  authmgrCnfgrState = AUTHMGR_PHASE_INIT_3;
  if (authmgrCtlApplyConfigData () !=  SUCCESS)
  {
    authmgrRC =  ERROR;

    return authmgrRC;
  }
  printf("%s:%d\r\n", __FUNCTION__, __LINE__);

  return authmgrRC;
}

/*********************************************************************
* @purpose  This function undoes authmgrCnfgrInitPhase1Process
*
* @param    none
*
* @returns  none
*
* @comments  none
*
* @end
*********************************************************************/
void authmgrCnfgrFiniPhase1Process ()
{
  if (authmgrCB->globalInfo->authmgrCfg !=  NULLPTR)
  {
    osapiFree ( AUTHMGR_COMPONENT_ID, authmgrCB->globalInfo->authmgrCfg);
    authmgrCB->globalInfo->authmgrCfg =  NULLPTR;
  }

  if (authmgrCB->globalInfo->authmgrPortInfo !=  NULLPTR)
  {
    osapiFree ( AUTHMGR_COMPONENT_ID, authmgrCB->globalInfo->authmgrPortInfo);
    authmgrCB->globalInfo->authmgrPortInfo =  NULLPTR;
  }
  authmgrLogicalPortInfoDBDeInit ();

  if (authmgrCB->globalInfo->authmgrPortStats !=  NULLPTR)
  {
    osapiFree ( AUTHMGR_COMPONENT_ID,
               authmgrCB->globalInfo->authmgrPortStats);
    authmgrCB->globalInfo->authmgrPortStats =  NULLPTR;
  }

  if (authmgrCB->globalInfo->authmgrMapTbl !=  NULLPTR)
  {
    osapiFree ( AUTHMGR_COMPONENT_ID, authmgrCB->globalInfo->authmgrMapTbl);
    authmgrCB->globalInfo->authmgrMapTbl =  NULLPTR;
  }

  authmgrMacAddrInfoDBDeInit ();

  authmgrInitUndo ();

  if (authmgrCB->globalInfo !=  NULLPTR)
  {
    osapiFree ( AUTHMGR_COMPONENT_ID, authmgrCB->globalInfo);
    authmgrCB->globalInfo =  NULLPTR;
  }

  authmgrCnfgrState = AUTHMGR_PHASE_INIT_0;
}

/*********************************************************************
* @purpose  This function undoes authmgrCnfgrInitPhase2Process
*
* @param    none
*
* @returns  none
*
* @comments  none
*
* @end
*********************************************************************/
void authmgrCnfgrFiniPhase3Process ()
{
   CNFGR_RESPONSE_t response;
   CNFGR_ERR_RC_t reason;

  /* this func will place authmgrCnfgrState to WMU */
  authmgrCnfgrUconfigPhase2 (&response, &reason);
}

/*********************************************************************
* @purpose  This function process the configurator control commands/request
*           pair as a NOOP.
*
* @param    pResponse - @b{(output)}  Response always command complete.
*
* @param    pReason   - @b{(output)}  Always 0
*
* @returns   SUCCESS - Always return this value. onse is available.
*
*
* @comments  The following are valid response:
*            CNFGR_CMD_COMPLETE
*
* @comments  The following are valid error reason code:
*           None.
*
* @end
*********************************************************************/
RC_t authmgrCnfgrNoopProccess ( CNFGR_RESPONSE_t * pResponse,
                                   CNFGR_ERR_RC_t * pReason)
{
  RC_t authmgrRC =  SUCCESS;

  /* Return Value to caller */
  *pResponse =  CNFGR_CMD_COMPLETE;
  *pReason = 0;
  return authmgrRC;
}

/*********************************************************************
* @purpose  This function process the configurator control commands/request
*           pair Unconfigure Phase 2.
*
* @param    pResponse - @b{(output)}  Response if  SUCCESS.
*
* @param    pReason   - @b{(output)}  Reason if  ERROR.
*
* @returns   SUCCESS - There were no errors. Response is available.
*
* @returns   ERROR   - There were errors. Reason code is available.
*
* @comments  The following are valid response:
*            CNFGR_CMD_COMPLETE
*
* @comments  The following are valid error reason code:
*            CNFGR_ERR_RC_FATAL
*
* @end
*********************************************************************/

RC_t authmgrCnfgrUconfigPhase2 ( CNFGR_RESPONSE_t * pResponse,
                                    CNFGR_ERR_RC_t * pReason)
{
  RC_t authmgrRC;

  *pResponse =  CNFGR_CMD_COMPLETE;
  *pReason = 0;
  authmgrRC =  SUCCESS;

  memset (authmgrCB->globalInfo->authmgrCfg, 0, sizeof (authmgrCfg_t));

  authmgrCnfgrState = AUTHMGR_PHASE_WMU;

  return authmgrRC;
}

/***************************************************************************
* @purpose  This function process the configurator control commands/request
*           pair TERMINATE.
*
* @param    pCmdData - @b{(input)} command data.
*
* @returns  none.
*
*
* @notes
*
* @end
***************************************************************************/
void authmgrCnfgrTerminateProcess ( CNFGR_CMD_DATA_t * pCmdData)
{

}


