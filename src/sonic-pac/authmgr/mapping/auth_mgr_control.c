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

#define  MAC_EAPOL_PDU

#include "auth_mgr_include.h"
#include "osapi_sem.h"
#include "auth_mgr_exports.h"
#include "auth_mgr_client.h"
#include "auth_mgr_timer.h"
#include "auth_mgr_struct.h"
#include "auth_mgr_auth_method.h"
#include "pac_cfg_authmgr.h"
#include "auth_mgr_vlan_db.h"
#include "pacoper_common.h"

extern authmgrCB_t *authmgrCB;

extern authmgrCnfgrState_t authmgrCnfgrState;

extern authmgrLogicalPortInfo_t processInfo;

RC_t authmgrClientHwInfoCleanupAndReadd(authmgrLogicalPortInfo_t *logicalPortInfo,
                                           authmgrClientInfo_t *processInfo);

RC_t authmgrAddMac (uint32 lIntIfNum);

extern int osapi_proc_set (const char *path, const char *value);

#define attributeCmp(a, b) \
  if (strlen(a) !=  0) \
  { \
    if (memcmp(b, a, sizeof(b)) ==  0) \
    { \
      entryFound =  TRUE; \
    } \
    else \
    { \
      entryFound =  FALSE; \
      break; \
    } \
  }

#define attributeIntCmp(a, b) \
  if (a) \
  { \
    if (a == b) \
    { \
      entryFound =  TRUE; \
    } \
    else \
    { \
      entryFound =  FALSE; \
      break; \
    } \
  }

/*********************************************************************
* @purpose function to compare auth mgr lists
*
* @param list1 auth mgr list1
* @param list2 auth mgr list2
* @return  TRUE or  FALSE
*
* @comments
*
* @end
*********************************************************************/
 BOOL authmgrListArrayCompare ( AUTHMGR_METHOD_t * list1,
                                  AUTHMGR_METHOD_t * list2,
                                 uint32 len)
{
  if (0 != memcmp (list1, list2, len))
  {
    return  FALSE;
  }

  return  TRUE;
}

/*********************************************************************
* @purpose  Initialize authmgr tasks and data
*
* @param   none 
*
* @returns   SUCCESS or  FAILURE
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrStartTasks ()
{
  printf("%s:%d\r\n", __FUNCTION__, __LINE__);

  if (osapiRWLockCreate (&authmgrCB->authmgrRWLock,
                         OSAPI_RWLOCK_Q_PRIORITY) ==  FAILURE)
  {
     LOGF ( LOG_SEVERITY_INFO,
             "Error creating authmgrRWlock semaphore \n");
    return  FAILURE;
  }

  if (osapiRWLockCreate (&authmgrCB->authmgrCfgRWLock,
                         OSAPI_RWLOCK_Q_PRIORITY) ==  FAILURE)
  {
     LOGF ( LOG_SEVERITY_INFO,
             "Error creating authmgrCfgRWlock semaphore \n");
    return  FAILURE;
  }

  /* create authmgrTask - to service authmgr message queue */
  authmgrCB->authmgrTaskId =
    osapiTaskCreate ("authmgrTask", (void *) authmgrTask, 0, 0,
                     2 * authmgrSidDefaultStackSize (),
                     authmgrSidDefaultTaskPriority (),
                     authmgrSidDefaultTaskSlice ());

  if (authmgrCB->authmgrTaskId == 0)
  {
     LOGF ( LOG_SEVERITY_INFO,
             "Failed to create authmgr task.\n");
    return  FAILURE;
  }

  if (osapiWaitForTaskInit ( AUTHMGR_TASK_SYNC,  WAIT_FOREVER) !=
       SUCCESS)
  {
     LOGF ( LOG_SEVERITY_INFO,
             "Unable to initialize authmgr task.\n");
    return  FAILURE;
  }

  /* create authmgrSrvrTask - to service authmgr message queue */
  authmgrCB->authmgrSrvrTaskId =
    osapiTaskCreate ("authmgrSrvrTask", (void *) authmgrSrvrTask, 0, 0,
                     2 * authmgrSidDefaultStackSize (),
                     authmgrSidDefaultTaskPriority (),
                     authmgrSidDefaultTaskSlice ());

  if (authmgrCB->authmgrSrvrTaskId == 0)
  {
     LOGF ( LOG_SEVERITY_INFO,
             "Failed to create authmgr task.\n");
    return  FAILURE;
  }

  if (osapiWaitForTaskInit ( AUTHMGR_SRVR_TASK_SYNC,  WAIT_FOREVER) !=
       SUCCESS)
  {
     LOGF ( LOG_SEVERITY_INFO,
             "Unable to initialize authmgr srvr task.\n");
    return  FAILURE;
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose  authmgr task which serves the request queue
*
* @param    none
*
* @returns  void
*
* @comments User-interface writes and  all are serviced off
*           of the authmgrQueue
*
* @end
*********************************************************************/
void authmgrTask ()
{
  authmgrMsg_t msg;
  authmgrBulkMsg_t bulkMsg;
  authmgrVlanMsg_t vlanMsg;

  printf("%s:%d\r\n", __FUNCTION__, __LINE__);

  (void) osapiTaskInitDone ( AUTHMGR_TASK_SYNC);

  /* allocate the required data structures */
  authmgrCnfgrInitPhase1Process();
  
  /* do inter component registration */
  authmgrCnfgrInitPhase2Process();

  authmgrCnfgrInitPhase3Process( FALSE);
  
  for (;;)
  {
    /* Since we are reading from multiple queues, we cannot wait forever
     * on the message receive from each queue. Rather than sleep between
     * queue reads, use a semaphore to indicate whether any queue has
     * data. Wait until data is available. */

    if (osapiSemaTake(authmgrCB->authmgrTaskSyncSema,  WAIT_FOREVER) !=  SUCCESS)
    {
       LOGF( LOG_SEVERITY_ERROR,
          "Unable to acquire AUTHMGR message queue semaphore.");
      continue;
    }

    memset(&authmgrCB->processInfo, 0, sizeof(authmgrClientInfo_t));
    memset(&authmgrCB->oldInfo, 0, sizeof(authmgrClientInfo_t));


    if (osapiMessageReceive
        (authmgrCB->authmgrVlanEventQueue, (void *) &vlanMsg,
         (uint32) sizeof (authmgrVlanMsg_t),  NO_WAIT) ==  SUCCESS)
    {
      (void) authmgrVlanDispatchCmd (&vlanMsg);
    }
    else if (osapiMessageReceive
        (authmgrCB->authmgrQueue, (void *) &msg,
         (uint32) sizeof (authmgrMsg_t),  NO_WAIT) ==  SUCCESS)
    {
      (void) authmgrDispatchCmd (&msg);
    }
    else if (osapiMessageReceive
        (authmgrCB->authmgrBulkQueue, (void *) &bulkMsg,
         (uint32) sizeof (authmgrBulkMsg_t),  NO_WAIT) ==  SUCCESS)
    {
      (void) authmgrBulkDispatchCmd (&bulkMsg);
    }
  }
}

/*********************************************************************
* @purpose  authmgr srvr task which serves the request queue
*
* @param    none
*
* @returns  void
*
* @comments external applications are serviced off
*           of the authmgrQueue
*
* @end
*********************************************************************/
void authmgrSrvrTask ()
{
  printf("%s:%d\r\n", __FUNCTION__, __LINE__);

  (void) osapiTaskInitDone ( AUTHMGR_SRVR_TASK_SYNC);

  handle_async_resp_data (&authmgrCB->listen_sock);
  return;
}

/*********************************************************************
* @purpose  Save the data in a message to a shared memory
*
* @param    *data   @b{(input)} pointer to data
* @param    *msg   @b{(input)} authmgr message 
*
* @returns   SUCCESS or  FAILURE
*
* @comments Once the message is serviced, this variable size data will
*           be retrieved
*
* @end
*********************************************************************/
RC_t authmgrFillMsg (void *data, authmgrMsg_t * msg)
{
  switch (msg->event)
  {

  case authmgrMgmtAdminModeEnable:
  case authmgrMgmtAdminModeDisable:
  case authmgrMethodOrderModify:
  case authmgrMethodPriorityModify:
    break; 

    /* events originating from UI */
  case authmgrMgmtPortInitializeSet:
  case authmgrMgmtLogicalPortInitializeSet:
  case authmgrMgmtPortReauthenticateSet:
  case authmgrMgmtLogicalPortReauthenticateSet:
  case authmgrMgmtPortControlModeSet:
  case authmgrMgmtHostControlModeSet:
  case authmgrMgmtPortQuietPeriodSet:
  case authmgrMgmtPortInactivePeriodSet:
  case authmgrMgmtPortReAuthEnabledSet:
  case authmgrMgmtPortMaxUsersSet:
  case authmgrPaeCapabilitiesEvent:
  case authmgrViolationModeSet:
  case authmgrCtlPortInfoReset:
    /* add to queue uint32 size */
    memcpy (&msg->data.msgParm, data, sizeof (uint32));
    break;

  case authmgrMgmtPortReAuthPeriodSet:
    memcpy (&msg->data.timePeriod, data, sizeof (authmgrMgmtTimePeriod_t));
    break;

  case authmgrIntfChange:
    /* add to queue a NIM correlator */
    memcpy (&msg->data.authmgrIntfChangeParms, data,
            sizeof (authmgrIntfChangeParms_t));
    break;

  case authmgrIntfStartup:
    /* add to queue a NIM correlator */
    memcpy (&msg->data.startupPhase, data, sizeof (NIM_STARTUP_PHASE_t));
    break;

  case authmgrClientTimeout:
  case authmgrDelDuplicateEntry:
  case authmgrAddMacInMacDB:
  case authmgrTimeTick:
  case authmgrAuthenticationStart:
  case authmgrClientCleanup:
    break;                      /* NULL data, proceed */

  case authmgrAuthMethodCallbackEvent:
    memcpy (&msg->data.authParams, data, sizeof (authmgrAuthRespParams_t));
    break;

  case authmgrAaaInfoReceived:
    /* add to queue a char pointer */
    memcpy (&msg->data.authmgrAaaMsg, data, sizeof (authmgrAaaMsg_t));
    break;

  default:
    /* unmatched event */
    return  FAILURE;
  }                             /* switch */

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Save the bulk data in a message to a shared memory
*
* @param    *data   @b{(input)} pointer to data
* @param    *msg   @b{(input)} authmgr message 
*
* @returns   SUCCESS or  FAILURE
*
* @comments Once the message is serviced, this variable size data will
*           be retrieved
*
* @end
*********************************************************************/
RC_t authmgrBulkFillMsg (void *data, authmgrBulkMsg_t * msg)
{
  switch (msg->event)
  {
    case authmgrUnauthAddrCallBackEvent:
      memcpy (&msg->data.unauthParms, data,
              sizeof (authmgrUnauthCallbackParms_t));
      break;

    default:
      /* unmatched event */
      return  FAILURE;
  }                             /* switch */

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Save the VLAN data in a message to a shared memory
*
* @param    *data   @b{(input)} pointer to data
* @param    *msg   @b{(input)} authmgr message 
*
* @returns   SUCCESS or  FAILURE
*
* @comments Once the message is serviced, this variable size data will
*           be retrieved
*
* @end
*********************************************************************/
RC_t authmgrVlanFillMsg (void *data, authmgrVlanMsg_t * msg)
{
  switch (msg->event)
  {
    case authmgrVlanDeleteEvent:
    case authmgrVlanAddEvent:
    case authmgrVlanAddPortEvent:
    case authmgrVlanDeletePortEvent:
    case authmgrVlanPvidChangeEvent:
    case authmgrVlanConfDeleteEvent:
    case authmgrVlanConfPortDeleteEvent:
      memcpy (&msg->data.vlanData, data, sizeof (dot1qNotifyData_t));
      break;

    default:
      /* unmatched event */
      return  FAILURE;
  }                             /* switch */

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Send a command to authmgr queue
*
* @param    event      @b{(input)} event type
* @param    intIfNum   @b{(input)} internal interface number
* @param    *data      @b{(input)} pointer to data
*
* @returns   SUCCESS or  FAILURE
*
* @comments Command is queued for service
*
* @end
*********************************************************************/
RC_t authmgrIssueCmd (uint32 event, uint32 intIfNum, void *data)
{
  authmgrMsg_t msg;
  authmgrBulkMsg_t bulkMsg;
  authmgrVlanMsg_t vlanMsg;

  RC_t rc;

  memset(&msg, 0, sizeof(authmgrMsg_t));
  memset(&bulkMsg, 0, sizeof(authmgrBulkMsg_t));
  memset(&vlanMsg, 0, sizeof(authmgrVlanMsg_t));


  /* send message */
  if (event == authmgrUnauthAddrCallBackEvent)
  {
    if (data !=  NULLPTR)
    {
      bulkMsg.event = event;
      bulkMsg.intf = intIfNum;

      (void) authmgrBulkFillMsg (data, &bulkMsg);
    }

    rc =
      osapiMessageSend (authmgrCB->authmgrBulkQueue, &bulkMsg,
          (uint32) sizeof (authmgrBulkMsg_t),  NO_WAIT,
           MSG_PRIORITY_NORM);
  }
  else if ((event == authmgrVlanDeleteEvent) ||
           (event == authmgrVlanAddEvent) ||
           (event == authmgrVlanAddPortEvent) ||
           (event == authmgrVlanDeletePortEvent) ||
           (event == authmgrVlanPvidChangeEvent) ||
           (event == authmgrVlanConfDeleteEvent) ||
           (event == authmgrVlanConfPortDeleteEvent))
  {
    if (data !=  NULLPTR)
    {
      vlanMsg.event = event;
      vlanMsg.intf = intIfNum;

      (void) authmgrVlanFillMsg (data, &vlanMsg);
    }

    rc =
      osapiMessageSend (authmgrCB->authmgrVlanEventQueue, &vlanMsg,
          (uint32) sizeof (authmgrVlanMsg_t),  NO_WAIT,
           MSG_PRIORITY_NORM);
  }
  else
  {  
    msg.event = event;
    msg.intf = intIfNum;
    if (data !=  NULLPTR)
    {
      (void) authmgrFillMsg (data, &msg);
    }

    rc =
      osapiMessageSend (authmgrCB->authmgrQueue, &msg,
          (uint32) sizeof (authmgrMsg_t),  NO_WAIT,
           MSG_PRIORITY_NORM);
  }

  rc = osapiSemaGive(authmgrCB->authmgrTaskSyncSema);
  if(rc !=  SUCCESS)
  {
     LOGF( LOG_SEVERITY_NOTICE,
        "Failed to give msgQueue to Authmgr task sync semaphore.\n");
  }


  if (rc !=  SUCCESS)
  {
    AUTHMGR_ERROR_SEVERE
      ("Failed to send to authmgrQueue! Event: %u, interface: %s\n", event,
       authmgrIntfIfNameGet(intIfNum));
  }

  return rc;
}

/*********************************************************************
* @purpose  Route the event to a handling function and grab the parms
*
* @param    msg   @b{(input)} authmgr message 
*
* @returns   SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrDispatchCmd (authmgrMsg_t * msg)
{
  RC_t rc =  FAILURE;

  (void) osapiWriteLockTake (authmgrCB->authmgrRWLock,  WAIT_FOREVER);

  memset(&authmgrCB->oldInfo, 0, sizeof(authmgrClientInfo_t));

  switch (msg->event)
  {
  case authmgrIntfChange:
    rc = authmgrIhProcessIntfChange (msg->intf,
                                     msg->data.authmgrIntfChangeParms.intfEvent,
                                     msg->data.authmgrIntfChangeParms.
                                     nimCorrelator);
    break;

  case authmgrIntfStartup:
    rc = authmgrIhProcessIntfStartup (msg->data.startupPhase);
    break;

  case authmgrTimeTick:
    rc = authmgrTimerAction ();
    break;

  case authmgrMgmtAdminModeEnable:
    rc = authmgrCtlAdminModeEnable();
    break;

  case authmgrMgmtAdminModeDisable:
    rc = authmgrCtlAdminModeDisable();
    break;

  case authmgrMgmtPortInitializeSet:
    rc = authmgrCtlPortInitializeSet (msg->intf, msg->data.msgParm);
    break;

  case authmgrMgmtLogicalPortInitializeSet:
    rc = authmgrCtlLogicalPortInitializeSet (msg->intf);
    break;

  case authmgrMgmtPortReauthenticateSet:
    rc = authmgrCtlPortReauthenticateSet (msg->intf, msg->data.msgParm);
    break;

  case authmgrMgmtLogicalPortReauthenticateSet:
    rc = authmgrCtlLogicalPortReauthenticateSet (msg->intf, msg->data.msgParm);
    break;

  case authmgrMgmtPortControlModeSet:
    rc = authmgrCtlPortControlModeSet (msg->intf, msg->data.msgParm);
    break;

  case authmgrPaeCapabilitiesEvent:
    rc = authmgrPaeCapabilitiesEventProcess (msg->intf, msg->data.msgParm);
    break;

  case authmgrViolationModeSet:
    rc = authmgrViolationModeSetAction (msg->intf, msg->data.msgParm);
    break;

  case authmgrMgmtHostControlModeSet:
    rc = authmgrPortCtrlHostModeSet (msg->intf, msg->data.msgParm);
    break;

  case authmgrMgmtPortQuietPeriodSet:
    rc = authmgrCtlPortQuietPeriodSet (msg->intf, msg->data.msgParm);
    break;

  case authmgrMgmtPortReAuthPeriodSet:
    rc = authmgrCtlPortReAuthPeriodSet (msg->intf, &msg->data.timePeriod);
    break;

  case authmgrMgmtPortReAuthEnabledSet:
    rc = authmgrCtlPortReAuthEnabledSet (msg->intf, msg->data.msgParm);
    break;

  case authmgrMgmtPortStatsClear:
    rc = authmgrCtlPortStatsClear (msg->intf);
    break;

  case authmgrMgmtApplyConfigData:
    rc = authmgrCtlApplyConfigData ();
    break;

  case authmgrMgmtApplyPortConfigData:
    rc = authmgrCtlApplyPortConfigData (msg->intf);
    break;

  case authmgrMgmtPortMaxUsersSet:
    rc = authmgrCtlPortMaxUsersSet (msg->intf, msg->data.msgParm);
    break;

  case authmgrAuthMethodCallbackEvent:
    rc = authmgrClientCallbackEventProcess (msg->intf, &msg->data.authParams);
    break;

  case authmgrClientTimeout:
  case authmgrDelDuplicateEntry:
    rc = authmgrCtlLogicalPortClientTimeout (msg->intf);
    break;

  case authmgrClientCleanup:
    rc = authmgrCtlClientCleanup (msg->intf);
    break;

  case authmgrAddMacInMacDB:
    rc = authmgrAddMac (msg->intf);
    break;

  case authmgrAaaInfoReceived:
    rc = authmgrRadiusResponseProcess (msg->intf,
                                       msg->data.authmgrAaaMsg.status,
                                       msg->data.authmgrAaaMsg.pResponse,
                                       msg->data.authmgrAaaMsg.respLen);
    break;

  case authmgrAuthenticationStart:
    authmgrAuthenticationInitiate (msg->intf);
    break;

  case authmgrMethodOrderModify:
  case authmgrMethodPriorityModify:
    rc = authmgrMethodModifyAction (msg->intf);
    break;

  case authmgrCtlPortInfoReset:
    rc = authmgrCtlPortReset(msg->intf, msg->data.msgParm);
    break;

  default:
    rc =  FAILURE;
  }

  (void) osapiWriteLockGive (authmgrCB->authmgrRWLock);
  return rc;
}

/*********************************************************************
* @purpose  Route the event to a handling function and grab the parms
*
* @param    msg   @b{(input)} authmgr message 
*
* @returns   SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrBulkDispatchCmd (authmgrBulkMsg_t * msg)
{
  RC_t rc =  FAILURE;

  (void) osapiWriteLockTake (authmgrCB->authmgrRWLock,  WAIT_FOREVER);

  switch (msg->event)
  {
    case authmgrUnauthAddrCallBackEvent:
      rc =
        authmgrCtlPortUnauthAddrCallbackProcess (msg->intf,
                                                 msg->data.unauthParms.macAddr,
                                                 msg->data.unauthParms.vlanId);
      break;
    default:
      rc =  FAILURE;
  }

  (void) osapiWriteLockGive (authmgrCB->authmgrRWLock);
  return rc;
}

/*********************************************************************
* @purpose  Route the event to a handling function and grab the parms
*
* @param    msg   @b{(input)} authmgr message 
*
* @returns   SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrVlanDispatchCmd (authmgrVlanMsg_t * msg)
{
  RC_t rc =  FAILURE;

  (void) osapiWriteLockTake (authmgrCB->authmgrRWLock,  WAIT_FOREVER);

  switch (msg->event)
  {
    case authmgrVlanDeleteEvent:
    case authmgrVlanAddEvent:
    case authmgrVlanAddPortEvent:
    case authmgrVlanDeletePortEvent:
    case authmgrVlanPvidChangeEvent:
    case authmgrVlanConfDeleteEvent:
    case authmgrVlanConfPortDeleteEvent:
      authmgrVlanChangeProcess (msg->event, msg->intf, &(msg->data.vlanData));
      rc =  SUCCESS;
    break;

    default:
      rc =  FAILURE;
  }

  (void) osapiWriteLockGive (authmgrCB->authmgrRWLock);
  return rc;
}

/*********************************************************************
* @purpose  Add supplicant MAC in MAC database
*
* @param    lIntIfNum   @b{(input)} logical interface number 
*                                   on which client is received
*
* @returns   SUCCESS or  FAILURE
*
* @end
*********************************************************************/

RC_t authmgrAddMac (uint32 lIntIfNum)
{
  authmgrLogicalPortInfo_t *entry =  NULLPTR;
  RC_t rc =  FAILURE;

  entry = authmgrLogicalPortInfoGet (lIntIfNum);
  if (entry !=  NULLPTR)
  {
    rc = authmgrMacAddrInfoAdd (&(entry->client.suppMacAddr), lIntIfNum);
  }
  return rc;
}

/*********************************************************************
* @purpose  Check if client is to be processed considering logical port
*           use and availability using dynamic allocation
*
* @param    *srcMac      @b{(input)} supplicant mac address
* @param    intIfNum     @b{(input)} physical interface
* @param    existing_node  @b{(output)} is already existing node
*
* @returns   SUCCESS or  FAILURE
*
* @comments Command is queued for service
*
* @end
*********************************************************************/
RC_t authmgrDynamicUserPduMapCheck (uint32 intIfNum,  char8 * srcMac,
                                       uint32 * lIntIfNum,
                                        BOOL * existing_node)
{
  uint32 lIndex;
  authmgrPortCfg_t *pCfg;
  authmgrLogicalPortInfo_t *logicalPortInfo;
  uint32 physPort = 0, lPort = 0, type = 0;

#ifdef AUTHMGR_MAC_MOVE_ON
   enetMacAddr_t macAddr;
#endif

  *existing_node =  FALSE;

  /* Get the port mode */
  if ( TRUE != authmgrIntfIsConfigurable (intIfNum, &pCfg))
  {
    return  FAILURE;
  }

  if ( AUTHMGR_PORT_AUTO == pCfg->portControlMode)
  {
    /* loop based on the intIfNum */
    lIndex = AUTHMGR_LOGICAL_PORT_ITERATE;
    while ( NULLPTR !=
           (logicalPortInfo =
            authmgrLogicalPortInfoGetNextNode (intIfNum, &lIndex)))
    {
      if (0 ==
          memcmp (srcMac, logicalPortInfo->client.suppMacAddr.addr,
                   MAC_ADDR_LEN))
      {
        *lIntIfNum = lIndex;
        *existing_node =  TRUE;
        return  SUCCESS;
      }
    }

#ifdef AUTHMGR_MAC_MOVE_ON
    memcpy (macAddr.addr, srcMac,  MAC_ADDR_LEN);
    if ( SUCCESS == authmgrMacAddrInfoFind (&macAddr, lIntIfNum))
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, intIfNum,
                           "\n%s:%d: Found the device : %s  on interface:%s \n",
                           __FUNCTION__, __LINE__, AUTHMGR_PRINT_MAC_ADDR(srcMac),
                           authmgrIntfIfNameGet(intIfNum));

      if ( NULLPTR != authmgrLogicalPortInfoGet (*lIntIfNum))
      {
        /* get the key and unpack */
        AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, *lIntIfNum);

        if (physPort != intIfNum)
        {
           uchar8 ifNamel[ NIM_IF_ALIAS_SIZE + 1];

          nimGetIntfName (physPort,  ALIASNAME, ifNamel);

          /* Remove client from previous interface */
          if ( SUCCESS != authmgrCtlLogicalPortClientTimeout(*lIntIfNum))
          {
             LOGF ( LOG_SEVERITY_DEBUG,
                     "Duplicate client %s  detected on interface %s (intIfNum %d). Unable to remove.",
                     AUTHMGR_PRINT_MAC_ADDR(srcMac), ifNamel, physPort);
            return  FAILURE;
          }
          else
          {
             LOGF ( LOG_SEVERITY_DEBUG,
                     "Duplicate client %s detected on interface %s (intIfNum %d) and removed.",
                      AUTHMGR_PRINT_MAC_ADDR(srcMac), ifNamel, physPort);
          }
        }
      }
    }
#endif
    if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].numUsers >=
        authmgrCB->globalInfo->authmgrPortInfo[intIfNum].maxUsers)
    {
      return  FAILURE;
    }

    /* allocate a new logical port for this supplicant */
    logicalPortInfo = authmgrLogicalPortInfoAlloc (intIfNum);

    if (logicalPortInfo !=  NULLPTR)
    {
      authmgrLogicalPortInfoInit (logicalPortInfo->key.keyNum);

      /* logicalPortInfo->inUse =  TRUE; */
      *existing_node =  FALSE;
      *lIntIfNum = logicalPortInfo->key.keyNum;

      memcpy (logicalPortInfo->client.suppMacAddr.addr, srcMac,
               MAC_ADDR_LEN);

      /* add mac address to Mac Addr Database*/
      if (authmgrAddMac(logicalPortInfo->key.keyNum) !=  SUCCESS)
      {
         LOGF ( LOG_SEVERITY_ERROR,
            "Failed to add MAC entry %s"
            " in MAC database for interface %s (intIfNum %d, logical port %d). Reason: Failed to send event authmgrAddMacInMacDB\n",
            AUTHMGR_PRINT_MAC_ADDR(srcMac), authmgrIntfIfNameGet(intIfNum), intIfNum, logicalPortInfo->key.keyNum);
        authmgrLogicalPortInfoDeAlloc (logicalPortInfo);
        return  FAILURE;
      }

      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].numUsers++;
      /*  authmgrCtlApplyLogicalPortConfigData(logicalPortInfo->key.keyNum); */
      return  SUCCESS;
    }
  }
  return  FAILURE;
}

/*********************************************************************
* @purpose  Check if client is to be processed considering logical port
*           use and availability
*
* @param    intIfNum     @b{(input)} physical interface
* @param    *srcMac      @b{(input)} supplicant mac address
* @param    lIntIfNum     @b{(input)} logical interface
* @param    existing_node  @b{(output)} is already existing node
*
* @returns   SUCCESS or  FAILURE
*
* @comments 
*
* @end
*********************************************************************/
RC_t authmgrCheckMapPdu (uint32 intIfNum,  char8 * srcMac,
                            uint32 * lIntIfNum,  BOOL * existingNode)
{
  authmgrPortCfg_t *pCfg;
   BOOL valid =  FALSE;

  *existingNode =  FALSE;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }

  /* Get the port mode */
  if ( TRUE != authmgrIntfIsConfigurable (intIfNum, &pCfg))
  {
    return  FAILURE;
  }

  /* check the host mode validity */
  if ( SUCCESS != authmgrHostIsDynamicNodeAllocCheck
      (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode, &valid))
  {
    /* some thing is wrong. */
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, intIfNum,
                         "%s:%d:Unable to get the host mode %s", __FUNCTION__,
                         __LINE__,
                         authmgrHostModeStringGet (authmgrCB->globalInfo->
                                                   authmgrPortInfo[intIfNum].
                                                   hostMode));
    return  FAILURE;
  }

  if ( FALSE == valid)
  {
  }
  else
  {
    /* logical nodes are dynamically allocated */
    return authmgrDynamicUserPduMapCheck (intIfNum, srcMac, lIntIfNum,
                                          existingNode);
  }
  return  FAILURE;
}

/*********************************************************************
* @purpose  This routine starts the application timer 
*
* @param    none
*
* @returns   SUCCESS or  FAILURE
*
* @comments 
*
* @end
*********************************************************************/
RC_t authmgrTimerAction ()
{
  if (!AUTHMGR_IS_READY)
  {
    return  SUCCESS;
  }

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }

  appTimerProcess (authmgrCB->globalInfo->authmgrTimerCB);

  return  SUCCESS;
}

void authmgrIntfOperBuildDefault(uint32 intIfNum)
{
  authmgrPortInfo_t  *pOper;

  pOper = &authmgrCB->globalInfo->authmgrPortInfo[intIfNum];
  memset(pOper, 0, sizeof(authmgrPortInfo_t));

  pOper->portControlMode = FD_AUTHMGR_PORT_MODE;
  pOper->hostMode = FD_AUTHMGR_HOST_MODE;
  pOper->quietPeriod = FD_AUTHMGR_RESTART_TIMER_VAL;
  pOper->reAuthPeriod = FD_AUTHMGR_PORT_REAUTH_PERIOD;
  pOper->reAuthEnabled = FD_AUTHMGR_PORT_REAUTH_ENABLED;
  pOper->reAuthPeriodServer = FD_AUTHMGR_PORT_REAUTH_PERIOD_FROM_SERVER;
  pOper->maxUsers = FD_AUTHMGR_PORT_MAX_USERS;
  pOper->authFailRetryMaxCount = FD_AUTHMGR_RADIUS_MAX_AUTH_ATTEMPTS;

  pOper->paeCapabilities = FD_AUTHMGR_PORT_PAE_CAPABILITIES;

}

/*********************************************************************
* @purpose  Set initialize value for a port
*
* @param    intIfNum   @b{(input)) internal interface number
* @param    initialize @b{(input)) initialize value
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
RC_t authmgrCtlPortInitializeSet (uint32 intIfNum,  BOOL initialize)
{
   uchar8  paeCapabilities = 0;
  RC_t rc =  SUCCESS;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }
  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, intIfNum,
                       "%s:Intialize physical port-%d \n", __FUNCTION__,
                       intIfNum);

  authmgrPortInfoCleanup (intIfNum);

  (void)authmgrDot1xPortPaeCapabilitiesGet(intIfNum, &paeCapabilities);

  if (paeCapabilities ==  DOT1X_PAE_PORT_AUTH_CAPABLE)
  {
    rc = authmgrCtlApplyPortConfigData(intIfNum);
  }

  return rc;
}

/*********************************************************************
* @purpose  Set initialize logical port
*
* @param    lIntIfNum   @b{(input)) Logical internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments 
*
* @end
*********************************************************************/
RC_t authmgrCtlLogicalPortInitializeSet (uint32 lIntIfNum)
{
  authmgrLogicalPortInfo_t *logicalPortInfo;
  uint32 physPort = 0, lPort = 0, type = 0;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }
  logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
  if (logicalPortInfo)
  {
    AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type,
                              logicalPortInfo->key.keyNum);
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, physPort,
                         "%s:Intialize Logical port-%d type %s\n", __FUNCTION__,
                         lIntIfNum, authmgrNodeTypeStringGet (type));

    return authmgrCtlApplyLogicalPortConfigData (lIntIfNum);
  }
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Set reauthentication value for a port
*
* @param    intIfNum       @b{(input)) internal interface number
* @param    reauthenticate @b{(input)) reauthentication value
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
RC_t authmgrCtlPortReauthenticateSet (uint32 intIfNum,
                                          BOOL reauthenticate)
{
  RC_t rc =  SUCCESS;
  uint32 lIntIfNum;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }

  lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;
  while (authmgrLogicalPortInfoGetNextNode (intIfNum, &lIntIfNum) !=  NULLPTR)
  {
    rc = authmgrCtlLogicalPortReauthenticateSet (lIntIfNum, reauthenticate);
  }
  return rc;
}

/*********************************************************************
* @purpose  Set reauthentication value for a port
*
* @param    intIfNum       @b{(input)) internal interface number
* @param    reauthenticate @b{(input)) reauthentication value
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
RC_t authmgrCtlLogicalPortReauthenticateSet (uint32 lIntIfNum,
                                                 BOOL reauthenticate)
{
  RC_t rc =  SUCCESS;
  uint32 physPort = 0, lPort = 0, type = 0;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }
  logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);

  if (logicalPortInfo !=  NULLPTR)
  {
    AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, lIntIfNum);
    if (0 != logicalPortInfo->key.keyNum)
    {
      if (AUTHMGR_AUTHENTICATED != logicalPortInfo->protocol.authState)
      {
         LOGF ( LOG_SEVERITY_DEBUG,
                 "Skipping reauthentication request for clients which are not already authenticated.");
        return  SUCCESS;
      }

      logicalPortInfo->client.reAuthenticate =  TRUE;
      logicalPortInfo->protocol.reauth =  TRUE;

       LOGF ( LOG_SEVERITY_INFO,
             "re-authentication triggered for client with mac address %s on port %s.",
             AUTHMGR_PRINT_MAC_ADDR(logicalPortInfo->client.suppMacAddr.addr), 
             authmgrIntfIfNameGet(physPort));  

      rc = authmgrStateMachineClassifier (authmgrReauthenticate, lIntIfNum);
    }
  }
  else
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                         "%s:Reauth set fail for client\n", __FUNCTION__);

    rc =  FAILURE;
  }

  return rc;
}

/*********************************************************************
* @purpose  Set port control mode based on the control mode
*
* @param    intIfNum    @b{(input)) internal interface number
* @param    portControl @b{(input)) port control mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortCtrlModeSet (uint32 intIfNum,
                                 AUTHMGR_PORT_CONTROL_t portControl)
{
  RC_t rc =  SUCCESS;
  authmgrPortCfg_t *pCfg;
  uint32 i = 0;
   char8 ifName[ NIM_IF_ALIAS_SIZE + 1];

  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  authmgrHostModeHwPolicyApply ( AUTHMGR_INVALID_HOST_MODE, intIfNum, FALSE);

  switch (portControl)
  {
  case  AUTHMGR_PORT_FORCE_UNAUTHORIZED:
    authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode =
      portControl;
 /* remove port from member of all vlans */
  authmgrVlanAcquirePort(intIfNum);
    rc = authmgrPortControlForceUnAuthActionSet (intIfNum);
    break;

  case  AUTHMGR_PORT_FORCE_AUTHORIZED:

    if (nimGetIntfName (intIfNum,  ALIASNAME, ifName) !=  SUCCESS)
    {
       LOGF ( LOG_SEVERITY_ERROR,
               "Unable to get alias for intf %s", authmgrIntfIfNameGet(intIfNum));
      return  FAILURE;
    }

    if ( TRUE != authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portEnabled)
    {
      authmgrVlanReleasePort(intIfNum);
      return  SUCCESS;
    }
    authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode =
      portControl;

    rc = authmgrPortPvidSet(intIfNum, 0);

    if (rc !=  SUCCESS)
    {
       LOGF ( LOG_SEVERITY_ERROR,
               "Unable to set PVID back to 0 for %s", ifName);
    }

    authmgrVlanReleasePort(intIfNum);
    rc = authmgrPortControlForceAuthActionSet (intIfNum);
    break;

  case  AUTHMGR_PORT_AUTO:
    authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode =
      portControl;
 /* remove port from member of all vlans */
  authmgrVlanAcquirePort(intIfNum);
  authmgrIhPhysicalPortStatusSet(intIfNum, AUTHMGR_PORT_STATUS_UNAUTHORIZED);
    rc = authmgrPortControlAutoActionSet (intIfNum);
    break;

  default:
    rc =  FAILURE;
  }

  for (i = 0; i <  AUTHMGR_METHOD_MAX; i++)
  {
    if ( NULLPTR != authmgrCB->globalInfo->authmgrCallbacks[i].portCtrlFn)
    {
      authmgrCB->globalInfo->authmgrCallbacks[i].portCtrlFn (intIfNum,
                                                             portControl);
    }
  }

  return rc;
}

/*********************************************************************
* @purpose  control function to set port control mode
*
* @param    intIfNum    @b{(input)) internal interface number
* @param    portControl @b{(input)) port control mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrCtlPortControlModeSet (uint32 intIfNum,
                                       AUTHMGR_PORT_CONTROL_t portControl)
{
  RC_t rc =  SUCCESS;
  authmgrPortCfg_t *pCfg;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  if ( DOT1X_PAE_PORT_AUTH_CAPABLE !=
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].paeCapabilities)
  {
    return rc;
  }
  if (portControl ==
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode)
  {
    return rc;
  }

  /* clean up previous info */
  authmgrPortInfoCleanup (intIfNum);
  authmgrPortInfoInitialize (intIfNum,  TRUE);
  authmgrMethodOrderChangeProcess (intIfNum);

  return authmgrPortCtrlModeSet (intIfNum, portControl);
}

/*********************************************************************
* @purpose  Set authentication restart period value
*
* @param    intIfNum    @b{(input)) internal interface number
* @param    quietPeriod @b{(input)) authentication restart period 
*
* @returns   SUCCESS
*
* @comments The quietPeriod is the initialization value for quietWhile,
*           which is a timer used by the Authenticator state machine
*           to define periods of time in which it will not attempt to
*           acquire a Supplicant.
*
* @end
*********************************************************************/
RC_t authmgrCtlPortQuietPeriodSet (uint32 intIfNum, uint32 quietPeriod)
{
  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].quietPeriod = quietPeriod;
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Set the Reauthentication period
*
* @param    intIfNum      @b{(input)) internal interface number
* @param    reAuthPeriod  @b{(input)) reauthentication period
*
* @returns   SUCCESS
*
* @comments The reAuthPeriod is the initialization value for reAuthWhen,
*           which is a timer used by the Authenticator state machine to
*           determine when reauthentication of the Supplicant takes place.
*
* @end
*********************************************************************/
RC_t authmgrCtlPortReAuthPeriodSet (uint32 intIfNum,
                                       authmgrMgmtTimePeriod_t * params)
{
  authmgrLogicalPortInfo_t *logicalPortInfo;
  uint32 lIntIfNum = 0;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].reAuthPeriodServer =
    params->reAuthPeriodServer;
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].reAuthPeriod = params->val;

  lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;
  while ((logicalPortInfo =
          authmgrLogicalPortInfoGetNextNode(intIfNum,
                                            &lIntIfNum)) !=  NULLPTR)
  {
    if ( TRUE == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].reAuthEnabled)
    {
      authmgrTimerStart(logicalPortInfo, AUTHMGR_REAUTH_WHEN);
    }
  }
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Set the Reauthentication mode
*
* @param    intIfNum       @b{(input)) internal interface number
* @param    reAuthEnabled  @b{(input)) reauthentication mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments The reAuthEnabled mode determines whether reauthentication
*           of the Supplicant takes place.
*
* @end
*********************************************************************/
RC_t authmgrCtlPortReAuthEnabledSet (uint32 intIfNum,
                                         BOOL reAuthEnabled)
{
  RC_t rc =  SUCCESS;
  uint32 lIntIfNum = 0;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }
  if (reAuthEnabled ==
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].reAuthEnabled)
  {
    return  SUCCESS;
  }

  /* Whenever the reAuthEnabled setting is changed, reset the reAuthWhen timer
   */

  lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;
  while ((logicalPortInfo =
          authmgrLogicalPortInfoGetNextNode (intIfNum,
                                             &lIntIfNum)) !=  NULLPTR)
  {
    logicalPortInfo->client.reAuthenticate =
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].reAuthEnabled;

    if ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
        logicalPortInfo->client.logicalPortStatus)
    {
      if (reAuthEnabled ==  FALSE)
      {
        if (AUTHMGR_REAUTH_WHEN == logicalPortInfo->authmgrTimer.cxt.type)
        {
          /* stop the timer */
          authmgrTimerDestroy (authmgrCB->globalInfo->authmgrTimerCB,
                               logicalPortInfo, AUTHMGR_REAUTH_WHEN);
        }
      }
      else
      {
        authmgrTimerStart (logicalPortInfo, AUTHMGR_REAUTH_WHEN);
      }
    }
  }

  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].reAuthEnabled =
    reAuthEnabled;
  return rc;
}

/*********************************************************************
* @purpose  Clear authmgr stats for specified port
*
* @param    intIfNum     @b{(input)) internal interface number
*
* @returns   SUCCESS
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrCtlPortStatsClear (uint32 intIfNum)
{
  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }
  memset (&authmgrCB->globalInfo->authmgrPortStats[intIfNum], 0,
          sizeof (authmgrPortStats_t));
  return  SUCCESS;
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
RC_t authmgrCtlApplyConfigData (void)
{

  /* Apply the global admin mode for authmgr */
  if (authmgrCB->globalInfo->authmgrCfg->adminMode ==  ENABLE)
  {
    authmgrCtlAdminModeEnable ();
  }
  else
  {
    authmgrCtlAdminModeDisable ();
  }
  return  SUCCESS;
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
RC_t authmgrCtlApplyPortConfigData (uint32 intIfNum)
{
  authmgrPortCfg_t *pCfg;
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  authmgrPortInfoInitialize (intIfNum,  TRUE);

  if ( DOT1X_PAE_PORT_AUTH_CAPABLE !=
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].paeCapabilities)
  {
    return  SUCCESS;
  }
  authmgrMethodOrderChangeProcess (intIfNum);

  authmgrPortCtrlModeSet (intIfNum, pCfg->portControlMode);
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Notifies  authmgr has released the interface.
*
* @param    intIfNum   @b{(input)} internal interface number
* @param    VlanId       @b{(input)} VlanId
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrCtlPortAuthmgrRelease (uint32 intIfNum, uint32 vlanId)
{
    return  SUCCESS;
}

/*********************************************************************
* @purpose function to handle vlan ents
*
* @param event dot1q event
* @param intIfNum interface number
* @param vlanData vlan event data
*
* @comments
*
* @end
*********************************************************************/
void authmgrVlanChangeProcess (uint32 event, uint32 intIfNum,
                               dot1qNotifyData_t * vlanData)
{
  uint32 i = 0, vlanId = 0, numVlans = 0;
  RC_t rc;
  dot1qTaggingMode_t tagging =  DOT1Q_MEMBER_UNTAGGED;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, 0,
                         "Authmgr not Enabled.\r\n");
    return;
  }

  for (i = 1; i <=  VLAN_MAX_MASK_BIT; i++)
  {
    {
      vlanId = vlanData->data.vlanId;
      /* For any continue, we will break out */
      i =  VLAN_MAX_MASK_BIT + 1;

      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, 0,
                           "authmgrEvent %d port %s vlan %d", event, authmgrIntfIfNameGet(intIfNum), vlanId);
    }
    switch (event)
    {
    case authmgrVlanDeleteEvent:
      authmgrVlanDeleteProcess (vlanId);
      break;

    case authmgrVlanDeletePortEvent:
      if ( TRUE == vlanData->tagged)
      {
        tagging =  DOT1Q_MEMBER_TAGGED;
      }
      authmgrVlanPortDeleteProcess (intIfNum, vlanId, tagging);

      break;

    case authmgrVlanAddEvent:
      rc = authmgrVlanAddProcess (vlanId);
      break;

    case authmgrVlanAddPortEvent:
      if ( TRUE == vlanData->tagged)
      {
        tagging =  DOT1Q_MEMBER_TAGGED;
      }
      rc = authmgrVlanPortAddProcess (intIfNum, vlanId, tagging);
      break;

    case authmgrVlanPvidChangeEvent:
      rc = authmgrVlanPVIDChangeEventProcess (intIfNum, vlanId);
      break;

    case authmgrVlanConfDeleteEvent:
      authmgrVlanConfDeleteProcess (vlanId);
      break;

    case authmgrVlanConfPortDeleteEvent:
      authmgrVlanConfPortDeleteProcess (intIfNum, vlanId);
      break;

    }
    numVlans++;
  }
}

/*********************************************************************
* @purpose  Set max users value
*
* @param    intIfNum @b{(input)) internal interface number
* @param    maxUsers @b{(input)) max users
*
* @returns   SUCCESS
*
* @comments The maxUsers is the maximum number of hosts that can be
*           authenticated on a port using mac based authentication
*
* @end
*********************************************************************/
RC_t authmgrCtlPortMaxUsersSet (uint32 intIfNum, uint32 maxUsers)
{
  RC_t rc =  SUCCESS;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }

  /* Check the operating host mode.
     Max users are applicable for multi-auth mode only.
     Ignore if mode is different. */

  if ( AUTHMGR_MULTI_AUTH_MODE == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode)
  {
    authmgrCB->globalInfo->authmgrPortInfo[intIfNum].maxUsers = maxUsers;

    if ( AUTHMGR_PORT_AUTO == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode)
    {
      if (maxUsers < authmgrCB->globalInfo->authmgrPortInfo[intIfNum].numUsers)
      {
         LOGF ( LOG_SEVERITY_INFO,
                 "Cleaning all clients on port as new max user cfg [%d] < current no. of users [%d].",
                 maxUsers, authmgrCB->globalInfo->authmgrPortInfo[intIfNum].numUsers);
        authmgrPortInfoCleanup(intIfNum);
        rc = authmgrCtlApplyPortConfigData (intIfNum);
      }
    }
  }

  return rc;
}

/*********************************************************************
* @purpose  Apply authmgr logical config data to specified interface
*
* @param    lIntIfNum     @b{(input)) Logical internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrCtlApplyLogicalPortConfigData (uint32 lIntIfNum)
{
  uint32 physPort;

  AUTHMGR_PORT_GET (physPort, lIntIfNum);

  /* Initialize state machines */
  if (authmgrCB->globalInfo->authmgrPortInfo[physPort].portControlMode ==
       AUTHMGR_PORT_AUTO
      || authmgrCB->globalInfo->authmgrPortInfo[physPort].portEnabled ==
       FALSE)
  {
    authmgrLogicalPortInfoInit (lIntIfNum);
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Reset authmgr session data to specified interface
*
* @param    logicalPortInfo  @b{(input))  Logical Port Info node
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrCtlResetLogicalPortSessionData (authmgrLogicalPortInfo_t *
                                               logicalPortInfo)
{
 /* logicalPortInfo->client.sessionTime = simSystemUpTimeGet ();*/
  if (logicalPortInfo->client.sessionTimeout != 0)
     logicalPortInfo->client.lastAuthTime = logicalPortInfo->client.sessionTime;
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Reset authmgr session data to specified interface
*
* @param    logicalPortInfo  @b{(input))  Logical Port Info node
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrCtlStopLogicalPortSessionData (authmgrLogicalPortInfo_t *
                                              logicalPortInfo)
{
  uint32 tempU32 = 0; 
  if (tempU32 > logicalPortInfo->client.sessionTime)
  {
    logicalPortInfo->client.sessionTime =
      tempU32 - logicalPortInfo->client.sessionTime;
  }
  else
  {
    logicalPortInfo->client.sessionTime = 0xffffffff -
      logicalPortInfo->client.sessionTime + tempU32;
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Disable  radius assigned vlan on a specified interface
*
* @param    intIfNum   @b{(input)) internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrCtlLogicalPortVlanAssignedReset (uint32 lIntIfNum)
{
  RC_t rc =  FAILURE;
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;

  logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
  rc = authmgrClientInfoCleanup (logicalPortInfo);

  return rc;
}


/*********************************************************************
* @purpose  Process the unauthenticated Users on the port
*
* @param    intIfNum   @b{(input)) internal interface number
* @param    macAddr   @b{(input)) mac address 
* @param    vlanId @b{(input)} vlan id
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrCtlPortUnauthAddrCallbackProcess (uint32 intIfNum,
                                                  enetMacAddr_t macAddr,
                                                  ushort16 vlanId)
{
  uint32 lIntIfNum = 0;
   BOOL exists;
  authmgrPortCfg_t *pCfg;
  authmgrLogicalPortInfo_t *logicalPortInfo;
  authmgrLogicalPortInfo_t *lPortInfo;
   char8 ifName[ NIM_IF_ALIAS_SIZE + 1];

  if (nimGetIntfName (intIfNum,  ALIASNAME, ifName) !=  SUCCESS)
  {
     LOGF ( LOG_SEVERITY_ERROR,
             "Unable to get alias for intf %d", intIfNum);
    return  FAILURE;
  }

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, intIfNum,
                         "%s, %d, PAC not enabled on interface %s \n",
                         __func__, __LINE__, ifName);
    return  SUCCESS;
  }

  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, intIfNum,
                         "%s, %d, PAC not configurable on interface %s \n",
                         __func__, __LINE__, ifName);
    return  SUCCESS;
  }

  if ( TRUE != authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portEnabled)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, intIfNum,
                         "%s, %d, PAC not operational on interface %s \n",
                         __func__, __LINE__, ifName);
    return  SUCCESS;
  }

  if ( TRUE!= authmgrCB->globalInfo->authmgrPortInfo[intIfNum].unLearnMacPolicy)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, intIfNum,
                         "%s, %d, PAC unlearnt policy not enabled on interface %s \n",
                         __func__, __LINE__, ifName);
    return  SUCCESS;
  }

  if (0 == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethodCount)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, intIfNum,
                         "%s, %d, PAC enabled method count is Zero on interface %s \n",
                         __func__, __LINE__, ifName);
    return  SUCCESS;
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, intIfNum,
                       "\n%s:%d: Check Source Mac: %s Interface: %s Vlan: %d \n",
                       __FUNCTION__, __LINE__, AUTHMGR_PRINT_MAC_ADDR(macAddr.addr),
                       ifName, vlanId);                       

  /* search the Mac address in the list of clients on the port */
  if (authmgrCheckMapPdu (intIfNum, macAddr.addr, &lIntIfNum, &exists) !=
       SUCCESS)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, intIfNum,
                         "Failed to Process the unauth Addr Callback \n");
    return  SUCCESS;
  }

  if (exists ==  FALSE)
  {
    if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].numUsers >
        authmgrCB->globalInfo->authmgrPortInfo[intIfNum].maxUsers)
    {
      return  FAILURE;
    }

    logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
    if (logicalPortInfo ==  NULLPTR)
    {
      /*Coverity 88470 fix: Should not come here as logical port would have been
         created and assigned in authmgrCheckMapPdu function */
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, intIfNum,
                           "Failed to create logiucal port %d \n", lIntIfNum);
      return  SUCCESS;
    }

    logicalPortInfo->client.vlanId = vlanId;
    logicalPortInfo->client.vlanType = AUTHMGR_VLAN_DEFAULT;
    logicalPortInfo->client.blockVlanId = vlanId;

    if (!logicalPortInfo->protocol.authenticate)
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, intIfNum,
          "%s, %d, triggering event  authmgrAuthenticationStart for logicalInterface %d \n",
          __func__, __LINE__, lIntIfNum);
      authmgrIssueCmd (authmgrAuthenticationStart, lIntIfNum,  NULLPTR);
    }
  }

  lPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
  AUTHMGR_IF_NULLPTR_RETURN_LOG(lPortInfo);

  /* nothing to do if the client is already authenticated */
  if ( AUTHMGR_PORT_STATUS_AUTHORIZED == 
      lPortInfo->client.logicalPortStatus)
  {
    /* client already authenticated. */
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, intIfNum,
          "%s, %d, client with logicalInterface %d to is already authenticated.\n",
          __func__, __LINE__, lIntIfNum);
      return  SUCCESS;
  }
   /* Block further traffic from this client. */
  if ( FALSE == lPortInfo->client.dataBlocked)
  {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, intIfNum,
          "%s, %d, disabling the settings for logicalInterface %d to receive further packets to CPU\n",
          __func__, __LINE__, lIntIfNum);

    if (pacCfgIntfClientBlock(ifName, macAddr.addr, vlanId) !=  TRUE)
    {
       LOGF ( LOG_SEVERITY_ERROR,
               "Unable to block port %s", ifName); 
      return  FAILURE;
    }

    lPortInfo->client.dataBlocked =  TRUE;
    lPortInfo->client.blockVlanId = vlanId;
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Disconnect the client interface on expiry of client timer
*
* @param    intIfNum       @b{(input)) internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrCtlLogicalPortClientTimeout (uint32 lIntIfNum)
{
  RC_t rc =  FAILURE;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }
  logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);

  if ((logicalPortInfo !=  NULLPTR)
      && (logicalPortInfo->key.keyNum !=  0))
  {
    if (logicalPortInfo->protocol.authState == AUTHMGR_AUTHENTICATED)
    {
      rc = authmgrClientInfoCleanup (logicalPortInfo);
    }
  }

  return rc;
}

/*********************************************************************
* @purpose  Disconnect the client
*
* @param    intIfNum       @b{(input)) internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrCtlClientCleanup (uint32 lIntIfNum)
{
  RC_t rc =  FAILURE;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }

  logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);

  if ((logicalPortInfo !=  NULLPTR)
      && (logicalPortInfo->key.keyNum !=  0))
  {
    rc = authmgrClientInfoCleanup (logicalPortInfo);
  }

  return rc;
}


/*********************************************************************
* @purpose  Used to get method no response timeout 
*
* @param    val     @b{(input)) periodic timeout in seconds
*
* @returns   SUCCESS
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrCtlPortMethodNoRespPeriodGet (uint32 intIfNum,
                                        uint32 * val)
{
  if ( NULLPTR == val)
  {
    return  FAILURE;
  }

  *val = FD_AUTHMGR_PORT_METHOD_NO_RESP_PERIOD;

  return  SUCCESS;
}

/*********************************************************************
* @purpose control mode function to set the port control mode to auto
*
* @param    intIfNum   @b{(input)) internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortControlAutoActionSet (uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  authmgrPortCfg_t *pCfg;
  authmgrHostModeMap_t entry;
  uint32 i = 0;
   AUTHMGR_HOST_CONTROL_t hostMode =  AUTHMGR_INVALID_HOST_MODE;

  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  /* check the configured host mode
     and set the port accordingly */

  memset (&entry, 0, sizeof (authmgrHostModeMap_t));

  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode =  AUTHMGR_PORT_AUTO;
  if ( SUCCESS != authmgrHostModeMapInfoGet (pCfg->hostMode, &entry))
  {
    /* failed to get the handler for the host mode */
    return  FAILURE;
  }


  rc = entry.hostModeFn (intIfNum);

  for (i = 0; i <  AUTHMGR_METHOD_MAX; i++)
  {
    if ( NULLPTR != authmgrCB->globalInfo->authmgrCallbacks[i].hostCtrlFn)
    {
      authmgrCB->globalInfo->authmgrCallbacks[i].hostCtrlFn (intIfNum,
                                                             hostMode);
    }
  }
  return rc;
}

/*********************************************************************
* @purpose control mode function to set the port host mode 
*
* @param    intIfNum   @b{(input)) internal interface number
* @param    hostMode   @b{(input))  host mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortCtrlHostModeSet (uint32 intIfNum,
                                     AUTHMGR_HOST_CONTROL_t hostMode)
{
  RC_t rc =  SUCCESS;
  authmgrPortCfg_t *pCfg;
  authmgrHostModeMap_t entry;
  uint32 i = 0;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }
  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  if ( AUTHMGR_PORT_AUTO !=
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode)
  {
    return  FAILURE;
  }

  if (hostMode == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode)
  {
    if (hostMode ==  AUTHMGR_MULTI_AUTH_MODE)
    {
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].maxUsers = pCfg->maxUsers;
    }
    return  SUCCESS;
  }

  authmgrHostModeHwPolicyApply ( AUTHMGR_INVALID_HOST_MODE, intIfNum,
                                 FALSE);
  authmgrPortInfoCleanup (intIfNum);
  authmgrPortInfoInitialize (intIfNum,  TRUE);
  authmgrMethodOrderChangeProcess (intIfNum);
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode =
     AUTHMGR_PORT_AUTO;

  /* check the configured host mode
     and set the port accordingly */
  memset (&entry, 0, sizeof (authmgrHostModeMap_t));
  if ( SUCCESS != authmgrHostModeMapInfoGet (hostMode, &entry))
  {
    /* failed to get the handler for the host mode */
    return  FAILURE;
  }

  rc = entry.hostModeFn (intIfNum);

  for (i = 0; i <  AUTHMGR_METHOD_MAX; i++)
  {
    if ( NULLPTR != authmgrCB->globalInfo->authmgrCallbacks[i].hostCtrlFn)
    {
      authmgrCB->globalInfo->authmgrCallbacks[i].hostCtrlFn (intIfNum,
                                                             hostMode);
    }
  }

  return rc;
}

/*********************************************************************
* @purpose control function to set the host mode to multi host
*
* @param    intIfNum   @b{(input)) internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrControlMultiHostActionSet (uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  authmgrPortCfg_t *pCfg;

  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  /* Set the operating host mode */
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode =
     AUTHMGR_MULTI_HOST_MODE;

  rc = authmgrIhPhysicalPortStatusSet(intIfNum,  AUTHMGR_PORT_STATUS_UNAUTHORIZED);

  if ( SUCCESS != rc)
  {
     LOGF ( LOG_SEVERITY_WARNING,
             "Could not set status of Interface %u", intIfNum);
  }

  return rc;
}

/*********************************************************************
* @purpose control function to set the host mode to single host mode
*
* @param    intIfNum   @b{(input)) internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrControlSingleAuthActionSet (uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  authmgrPortCfg_t *pCfg;

  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  /* Set the operating host mode */
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode =
     AUTHMGR_SINGLE_AUTH_MODE;

  rc = authmgrIhPhysicalPortStatusSet(intIfNum,  AUTHMGR_PORT_STATUS_UNAUTHORIZED);

  if ( SUCCESS != rc) 
  {
     LOGF ( LOG_SEVERITY_WARNING,
             "Could not set status of Interface %u", intIfNum);
  }
  return rc;
}

/*********************************************************************
* @purpose control function to set the host mode to multi auth
*
* @param    intIfNum   @b{(input)) internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrControlMultAuthActionSet (uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  authmgrPortCfg_t *pCfg;

  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  /* Set the operating host mode */
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode =
     AUTHMGR_MULTI_AUTH_MODE;

  rc = authmgrIhPhysicalPortStatusSet(intIfNum,  AUTHMGR_PORT_STATUS_UNAUTHORIZED);

  if ( SUCCESS != rc) 
  {
     LOGF ( LOG_SEVERITY_WARNING,
             "Could not set status of Interface %u", intIfNum);
  }

  return rc;
}

/*********************************************************************
* @purpose control function to set the to force authorized
*
* @param    intIfNum   @b{(input)) internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortControlForceAuthActionSet (uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  authmgrPortCfg_t *pCfg;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  /* Set the operating host mode */
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode =
     AUTHMGR_INVALID_HOST_MODE;

  if ( NULLPTR == (logicalPortInfo = authmgrLogicalPortInfoAlloc (intIfNum)))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, 0,
                         "%s:Logical port alloc failure\n", __FUNCTION__);
    return  FAILURE;
  }

  /* Call the api to set the port to authorized */
  authmgrClientStatusSet (logicalPortInfo,  AUTHMGR_PORT_STATUS_AUTHORIZED);

  /* call the api to send EAP success */
  authmgrTxCannedSuccess (logicalPortInfo->key.keyNum, AUTHMGR_LOGICAL_PORT);
  return rc;
}

/*********************************************************************
* @purpose control function to set the to force un-authorized
*
* @param    intIfNum   @b{(input)) internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortControlForceUnAuthActionSet (uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  authmgrPortCfg_t *pCfg;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  /* Set the operating host mode */
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode =
     AUTHMGR_INVALID_HOST_MODE;

  if ( NULLPTR == (logicalPortInfo = authmgrLogicalPortInfoAlloc (intIfNum)))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, 0,
                         "%s:Logical port alloc failure\n", __FUNCTION__);
    return  FAILURE;
  }

  /* Call the api to set the port to unauthorized */
  authmgrClientStatusSet (logicalPortInfo,  AUTHMGR_PORT_STATUS_UNAUTHORIZED);

  /* call the api to send EAP failure */
  authmgrTxCannedFail (logicalPortInfo->key.keyNum, AUTHMGR_LOGICAL_PORT);

  return rc;
}

/*********************************************************************
* @purpose function to clean up authmgr port oper info
*
* @param    intIfNum   @b{(input)) internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortInfoCleanup (uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  uint32 lIntIfNum;
  authmgrPortCfg_t *pCfg;
  authmgrLogicalPortInfo_t *logicalPortInfo;
/*   BOOL valid =  FALSE; */

  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    return  SUCCESS;
  }

  /* reset all the clients associated with the port */
  lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;
  while ( NULLPTR !=
         (logicalPortInfo =
          authmgrLogicalPortInfoGetNextNode (intIfNum, &lIntIfNum)))
  {
    if (0 != logicalPortInfo->key.keyNum)
    {
      authmgrClientInfoCleanup (logicalPortInfo);
    }
  }
  return rc;
}

/*********************************************************************
* @purpose function to check policy validation based on host mode
*
* @param    hostMode   @b{(input))  hostmode
* @param    intIfNum   @b{(input))  interface number
* @param    *appyPolicy  @b{(input)) bool value
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrHostModeHwPolicyApply ( AUTHMGR_HOST_CONTROL_t hostMode,
                                      uint32 intIfNum,  BOOL install)
{
  RC_t rc =  SUCCESS;
   BOOL valid =  FALSE;

  switch (hostMode)
  {
  case  AUTHMGR_INVALID_HOST_MODE:
    valid =  FALSE;
    break;

  case  AUTHMGR_MULTI_HOST_MODE:
  case  AUTHMGR_SINGLE_AUTH_MODE:
  case  AUTHMGR_MULTI_AUTH_MODE:
    valid =  TRUE;
    break;

  default:
    rc =  FAILURE;
  }

  if ( SUCCESS == rc)
  {
    if (( TRUE == install) && ( TRUE == valid))
    {
      /* apply the policy */
      authmgrIhPhyPortViolationCallbackSet (intIfNum,
                                             AUTHMGR_PORT_VIOLATION_CALLBACK_ENABLE);
    }
    else
    {
      /* remove policy */
      authmgrIhPhyPortViolationCallbackSet (intIfNum,
                                             AUTHMGR_PORT_VIOLATION_CALLBACK_DISABLE);
    }
  }

  return rc;
}

/*********************************************************************
* @purpose function to get the auth restart timer value
*
* @param intIfNum internal interface number
* @param val timeout value
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrQuietPeriodGet (uint32 intIfNum, uint32 * val)
{
  if ( NULLPTR == val)
  {
    return  FAILURE;
  }

  *val = authmgrCB->globalInfo->authmgrPortInfo[intIfNum].quietPeriod;

  return  SUCCESS;
}

/*********************************************************************
* @purpose function to get the reauth period on interface
*
* @param intIfNum interface number
* @param val value of the reauth period
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrReAuthPeriodGet (uint32 intIfNum, uint32 * val)
{
  if ( NULLPTR == val)
  {
    return  FAILURE;
  }

  *val = authmgrCB->globalInfo->authmgrPortInfo[intIfNum].reAuthPeriod;

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Processes Authmgr-related event initiated by Dot1Q
*
* @param (in)    vlanId    Virtual LAN Id
* @param (in)    intIfNum  Interface Number
* @param (in)    event
*
* @returns   SUCCESS  or  FAILURE
*
* @end
*********************************************************************/
RC_t authmgrVlanChangeCallback (dot1qNotifyData_t *vlanData,
                                   uint32 intIfNum, uint32 event)
{
  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, 0,
                       "Received Vlan event %d for interface %s, vlan %d\n",
                       event, authmgrIntfIfNameGet(intIfNum), vlanData->data.vlanId);

  /* Vlan Change callbacks can be called during unconfig phase when dot1q is
     trying
     to restore the vlan config. */

   INTF_TYPES_t intfType;

  if (!((AUTHMGR_IS_READY) || authmgrCnfgrState == AUTHMGR_PHASE_UNCONFIG_2))
  {
     LOGF ( LOG_SEVERITY_INFO,
             "Received an VLAN change callback while DOT1Q is not ready to receive it during unconfig state.");
    return  FAILURE;
  }

  /* before performing any operations with interfaces,
     check if NIM is ready to handle requests */
  if ((nimPhaseStatusCheck () ==  TRUE)
      && (nimGetIntfType (intIfNum, &intfType) ==  SUCCESS))
  {
    if (authmgrIsValidIntfType (intfType) !=  TRUE)
    {
      /* if AUTHMGR is not interested in this interface,
       * inform event issuer that we have completed processing.
       */
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
                           "Interface %s is not AUTHMGR configurable\n",
                           authmgrIntfIfNameGet(intIfNum));
      return  SUCCESS;
    }
  }

  if ((intIfNum != 0)
      && (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portEnabled !=
           TRUE) && (event != VLAN_DELETE_PORT_NOTIFY))
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
                         "Interface %s is not enabled \n",
                         authmgrIntfIfNameGet(intIfNum));
    return  SUCCESS;
  }

  switch (event)
  {
  case VLAN_DELETE_PENDING_NOTIFY:
    authmgrIssueCmd (authmgrVlanDeleteEvent, intIfNum, vlanData);
    break;

  case VLAN_ADD_NOTIFY:
    if ( NOT_EXIST == authmgrVlanCheckValid(vlanData->data.vlanId))
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, 0,
                           "Adding vlan %d\n", vlanData->data.vlanId);
      authmgrIssueCmd (authmgrVlanAddEvent, intIfNum, vlanData);
    }
    break;

  case VLAN_ADD_PORT_NOTIFY:
    if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode ==
         AUTHMGR_PORT_AUTO)
    {
      authmgrIssueCmd (authmgrVlanAddPortEvent, intIfNum, vlanData);
    }
    break;

  case VLAN_DELETE_PORT_NOTIFY:
    authmgrIssueCmd (authmgrVlanDeletePortEvent, intIfNum, vlanData);
    break;

  case VLAN_PVID_CHANGE_NOTIFY:
    authmgrIssueCmd (authmgrVlanPvidChangeEvent, intIfNum, vlanData);

  default:
    break;
  }
  return  SUCCESS;
}

/*********************************************************************
* @purpose function to get hostmode map entry function entry
*
* @param type  host control mode
* @param elem associated entry for the host mode
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrHostModeMapInfoGet ( AUTHMGR_HOST_CONTROL_t type,
                                   authmgrHostModeMap_t * elem)
{
  uint32 i = 0;
  static authmgrHostModeMap_t authmgrHostModeHandlerTable[] = {
    { AUTHMGR_SINGLE_AUTH_MODE, authmgrControlSingleAuthActionSet},
    { AUTHMGR_MULTI_HOST_MODE, authmgrControlMultiHostActionSet},
    { AUTHMGR_MULTI_AUTH_MODE, authmgrControlMultAuthActionSet},
  };

  for (i = 0;
       i <
       (sizeof (authmgrHostModeHandlerTable) / sizeof (authmgrHostModeMap_t));
       i++)
  {
    if (type == authmgrHostModeHandlerTable[i].hostMode)
    {
      *elem = authmgrHostModeHandlerTable[i];
      return  SUCCESS;
    }
  }
  return  FAILURE;
}

/*********************************************************************
* @purpose function to check whether attributes are changed,
*          if changed, clean up the hardware info and readd the new info.
*
* @param logicalPortInfo  @b{(inout)}  Pointer to the Logical Port Info
* @param processInfo      @b{(inout)}  Pointer to the process Info
*
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end 
*********************************************************************/
RC_t authmgrClientHwInfoCleanupAndReadd(authmgrLogicalPortInfo_t *logicalPortInfo,
                                           authmgrClientInfo_t *processInfo)
{
  uint32 physPort = 0;

  AUTHMGR_PORT_GET (physPort, logicalPortInfo->key.keyNum);
  

  /* check if the client params have been changed */

  if ( SUCCESS != authmgrClientInfoCleanupCheck
      (&logicalPortInfo->client, &authmgrCB->processInfo))
  {
    if ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
        logicalPortInfo->client.logicalPortStatus)
    {
      /* clean up previous data */
      if ( SUCCESS != authmgrClientHwInfoCleanup (logicalPortInfo))
      {
        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                             "%s:Unable to cleanup client hw info logicalPort num-%d\n",
                             __FUNCTION__, logicalPortInfo->key.keyNum);
        return  FAILURE;
      }
    }

    /* push the client info to hw */
    if ( SUCCESS != authmgrClientHwInfoAdd (logicalPortInfo,
                                              logicalPortInfo->client.suppMacAddr,
                                              authmgrCB->processInfo.vlanId,
                                              logicalPortInfo->client.blockVlanId))
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                           "%s:Unable to add client hw info logicalPort num-%d\n",
                           __FUNCTION__, logicalPortInfo->key.keyNum);	
      return  FAILURE;
    }
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose function to handle update of new client
*
* @param lIntIfNum client logical interface number
* @param callbackParams authentication callback params
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrNewClientAction (uint32 lIntIfNum,
                                authmgrAuthRespParams_t * callbackParams)
{
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 physPort = 0;

  logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
  /* Coverity Fix to handle the possible NULLL ptr from the
     authmgrLogicalPortInfoGet */
  if (logicalPortInfo ==  NULLPTR)
  {
    /* Coverity defect fix. should never come here as authmgrCheckMapPdu would
       have assigned a new node if a new client is detected
       or returned existing logical interface number */
    return  FAILURE;
  }

  AUTHMGR_PORT_GET (physPort, lIntIfNum);

  if (AUTHMGR_UNAUTHENTICATED == logicalPortInfo->protocol.authState)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                         "%s, %d, triggering event  authmgrAuthenticationStart for logicalInterface %d \n",
                         __func__, __LINE__, lIntIfNum);

    authmgrIssueCmd (authmgrAuthenticationStart, lIntIfNum,  NULLPTR);
    authmgrStatsUpdate (physPort, callbackParams->method, authmgrStatsAuthEnter);
  }
  else
  {
    /* check if the client received is already authenticated */

    if ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
          logicalPortInfo->client.logicalPortStatus)
    {
      if (logicalPortInfo->client.authenticatedMethod !=
          callbackParams->method)
      {
        /* check if the received method is of higher priority than the existing one */

        if (( AUTHMGR_METHOD_NONE != logicalPortInfo->client.authenticatedMethod) &&
            ( SUCCESS == authmgrPriorityPrecedenceValidate(physPort, 
                                                             logicalPortInfo->client.authenticatedMethod,
                                                             callbackParams->method)))
        {

          if (AUTHMGR_AUTHENTICATED == logicalPortInfo->protocol.authState)
          {
          AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
              "%s, %d, Trigger from method %s received. "
              "Client already authenticated with method %s for logicalInterface %d \n"
              "Try to authenticate again as higher priority method is received\n",
              __func__, __LINE__, authmgrMethodStringGet(callbackParams->method), 
              authmgrMethodStringGet(logicalPortInfo->client.authenticatedMethod),lIntIfNum);
             logicalPortInfo->protocol.authenticatedRcvdStartAuth =  TRUE;
            logicalPortInfo->client.currentMethod = callbackParams->method;
            authmgrGenerateEvents (lIntIfNum);
            return  SUCCESS;
          }
          else
          {
           /* ignore the trigger */
          AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
              "%s, %d, Trigger from method %s received. "
              "Client already authenticated with method %s for logicalInterface %d \n"
              "ignoring trigger as the client auth state is in %s\n",
              __func__, __LINE__, authmgrMethodStringGet(callbackParams->method), 
              authmgrMethodStringGet(logicalPortInfo->client.authenticatedMethod),lIntIfNum,
              authmgrAuthStateStringGet(logicalPortInfo->protocol.authState));
            return  SUCCESS;
          }     
        }
        else if ( AUTHMGR_METHOD_NONE == logicalPortInfo->client.authenticatedMethod)
        {
          AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
              "%s, %d, Trigger from method %s received. "
              "Client already authenticated with method %s for logicalInterface %d \n"
              "Initiate authentication\n",
              __func__, __LINE__, authmgrMethodStringGet(callbackParams->method), 
              authmgrMethodStringGet(logicalPortInfo->client.authenticatedMethod),lIntIfNum);

          /* mimic authentication restart, if client state is not authenticating.
          Using existing event to generate the same */
          if(AUTHMGR_AUTHENTICATING != logicalPortInfo->protocol.authState)
          {
            logicalPortInfo->protocol.authenticatedRcvdStartAuth =  TRUE;

            AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, physPort,
                "Current auth method is none. Marking the current method with first method %s " 
                "for logicalInterface %d for new authentication on interface %s \n",
                authmgrMethodStringGet(authmgrCB->globalInfo->authmgrPortInfo[physPort].enabledMethods[0]),
                logicalPortInfo->key.keyNum, authmgrIntfIfNameGet(physPort));

            /* Restart with the first enabled method */
            logicalPortInfo->client.currentMethod = authmgrCB->globalInfo->authmgrPortInfo[physPort].enabledMethods[0];
            authmgrGenerateEvents (lIntIfNum);
          }
        }
      }
      else
      {
        if ( AUTHMGR_METHOD_8021X == callbackParams->method)
        {
          AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
              "%s, %d, Trigger from method %s received. "
              "Client already authenticated with method %s for logicalInterface %d \n"
              "since client is authenticated and received start generating further events\n",
              __func__, __LINE__, authmgrMethodStringGet(callbackParams->method), 
              authmgrMethodStringGet(logicalPortInfo->client.authenticatedMethod),lIntIfNum);

          if(AUTHMGR_AUTHENTICATING != logicalPortInfo->protocol.authState)
          {
            logicalPortInfo->protocol.authenticatedRcvdStartAuth =  TRUE;
            authmgrGenerateEvents (lIntIfNum);
          }
        }
        else
        {

        /* just ignore */
        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
            "%s, %d, Trigger from method %s received. "
            "Client already authenticated with method %s for logicalInterface %d \n"
            "Ignoring the request as the client already authenticated\n",
            __func__, __LINE__, authmgrMethodStringGet(callbackParams->method), 
            authmgrMethodStringGet(logicalPortInfo->client.authenticatedMethod),lIntIfNum);
        }
      } 
    }
    else
    {

      if ((AUTHMGR_AUTHENTICATING == logicalPortInfo->protocol.authState) &&
          ( AUTHMGR_METHOD_8021X == callbackParams->method)&&
          (logicalPortInfo->client.currentMethod == callbackParams->method))
      {
        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
            "%s, %d, Triggering authetication for method %s. "
            "for logicalInterface %d , current auth state of client is %s\n",
            __func__, __LINE__, authmgrMethodStringGet(callbackParams->method), lIntIfNum,
            authmgrAuthStateStringGet(logicalPortInfo->protocol.authState));

        authmgrAuthenticationTrigger(logicalPortInfo);
      }
      else
      {
        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
            "%s, %d, Cannot trigger event  authmgrAuthenticationStart "
            "for logicalInterface %d authState %d\n",
            __func__, __LINE__, lIntIfNum, logicalPortInfo->protocol.authState);

        if (AUTHMGR_AUTHENTICATING != logicalPortInfo->protocol.authState)
        {
           LOGF ( LOG_SEVERITY_INFO,
              "Client not ready for authentication.");
        }
      }
    }
  }


  return  SUCCESS;
}

/*********************************************************************
* @purpose function to handle RADIUS comm failure response from client
*
* @param lIntIfNum client logical interface number
* @param callbackParams authentication callback params
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrStatusServerCommFailAction (uint32 lIntIfNum,
                                           authmgrAuthRespParams_t * callbackParams)
{
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 physPort = 0;
  authmgrStatsUpdate_t status = authmgrStatsAuthFail;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (callbackParams);

  logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_PORT_GET (physPort, lIntIfNum);

  /* update statistics */
  status = authmgrStatsAuthFail;
  authmgrStatsUpdate (physPort, callbackParams->method, status);

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, physPort,
      "%s:RADIUS server comm failure for logicalPort num-%d\n",
      __FUNCTION__, logicalPortInfo->key.keyNum);

  if (0 !=
      strlen (callbackParams->clientParams.info.authInfo.authmgrUserName))
  {
    memcpy (logicalPortInfo->client.authmgrUserName,
        callbackParams->clientParams.info.authInfo.authmgrUserName,
        AUTHMGR_USER_NAME_LEN);
    logicalPortInfo->client.authmgrUserNameLength = 
      callbackParams->clientParams.info.authInfo.authmgrUserNameLength;
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, physPort,
      "%s:logicalPort num %d currentIdL %d\n",
      __FUNCTION__, logicalPortInfo->key.keyNum,
      callbackParams->clientParams.info.authInfo.attrInfo.idFromServer);

  logicalPortInfo->client.currentIdL = callbackParams->clientParams.info.authInfo.attrInfo.idFromServer;

  authmgrClientInfoCleanup (logicalPortInfo);
  return  SUCCESS;
}

/*********************************************************************
* @purpose function to handle failure or timeout response from client
*
* @param lIntIfNum client logical interface number
* @param callbackParams authentication callback params
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrStatusNotSuccessAction (uint32 lIntIfNum,
                                       authmgrAuthRespParams_t * callbackParams)
{
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 physPort = 0;
  authmgrStatsUpdate_t status = authmgrStatsAuthFail; /* Need to check - TBD */
  authmgrClientType_t clientType = AUTHMGR_CLIENT_UNAWARE;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (callbackParams);
  /* update statistics */

  logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_PORT_GET (physPort, lIntIfNum);

  authmgrTimerDestroy (authmgrCB->globalInfo->authmgrTimerCB,
                               logicalPortInfo, AUTHMGR_METHOD_NO_RESP_TMR);

  authmgrCB->oldInfo = logicalPortInfo->client;

  if (0 !=
      strlen (callbackParams->clientParams.info.authInfo.authmgrUserName))
  {
    memcpy (logicalPortInfo->client.authmgrUserName,
            callbackParams->clientParams.info.authInfo.authmgrUserName,
            AUTHMGR_USER_NAME_LEN);
    logicalPortInfo->client.authmgrUserNameLength = 
          callbackParams->clientParams.info.authInfo.authmgrUserNameLength;
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, physPort,
                       "%s:logicalPort num %d currentIdL %d\n",
                       __FUNCTION__, logicalPortInfo->key.keyNum,
                       callbackParams->clientParams.info.authInfo.attrInfo.idFromServer);

  logicalPortInfo->client.currentIdL = callbackParams->clientParams.info.authInfo.attrInfo.idFromServer;

  if (callbackParams->status ==  AUTHMGR_AUTH_FAIL)
  {
    logicalPortInfo->protocol.authFail =  TRUE;
    logicalPortInfo->client.reAuthCount++;
    status = authmgrStatsAuthFail;
    clientType =
      (callbackParams->method ==
        AUTHMGR_METHOD_8021X ? AUTHMGR_CLIENT_AWARE : AUTHMGR_CLIENT_UNAWARE);
  }
  else if (callbackParams->status ==  AUTHMGR_AUTH_TIMEOUT)
  {
    logicalPortInfo->protocol.authTimeout =  TRUE;
    status = authmgrStatsAuthTimeout;
  }

  if ((AUTHMGR_CLIENT_AWARE != logicalPortInfo->client.clientType) ||
      (AUTHMGR_CLIENT_UNASSIGNED == logicalPortInfo->client.clientType))
  {
    logicalPortInfo->client.clientType = clientType;
  }

  authmgrStatsUpdate (physPort, callbackParams->method, status);

  logicalPortInfo->protocol.authSuccess =  FALSE;

  return authmgrGenerateEvents (lIntIfNum);
}

/*********************************************************************
* @purpose function to handle auth disconnect of the client
*
* @param lIntIfNum client logical interface number
* @param callbackParams authentication callback params
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrStatusDisconnectAction (uint32 lIntIfNum,
                                       authmgrAuthRespParams_t * callbackParams)
{
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 physPort = 0;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (callbackParams);
  /* update statistics */

  logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_PORT_GET (physPort, lIntIfNum);

  logicalPortInfo->client.currentIdL = callbackParams->clientParams.info.authInfo.attrInfo.idFromServer;

  /* check if the client is already authenticated using the same method.
     If yes, remove the client details */
  if (( AUTHMGR_PORT_STATUS_AUTHORIZED ==
       logicalPortInfo->client.logicalPortStatus)
      && (logicalPortInfo->client.authenticatedMethod ==
          logicalPortInfo->client.currentMethod))
  {
    authmgrClientInfoCleanup (logicalPortInfo);
  }
  return  SUCCESS;
}

/*********************************************************************
* @purpose function to handle auth success of the client
*
* @param lIntIfNum client logical interface number
* @param callbackParams authentication callback params
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrStatusSuccessAction (uint32 lIntIfNum,
                                    authmgrAuthRespParams_t * callbackParams)
{
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 physPort = 0;
  authmgrClientType_t clientType = 0;

  logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);
  AUTHMGR_IF_NULLPTR_RETURN_LOG (callbackParams);

  AUTHMGR_PORT_GET (physPort, logicalPortInfo->key.keyNum);

  authmgrTimerDestroy (authmgrCB->globalInfo->authmgrTimerCB,
                               logicalPortInfo, AUTHMGR_METHOD_NO_RESP_TMR);

  /* Received auth success.
     Parse the received attributes and program
     accordingly */

  memset(&authmgrCB->processInfo, 0, sizeof(authmgrClientInfo_t));
  memset(&authmgrCB->oldInfo, 0, sizeof(authmgrClientInfo_t));
  memset(&authmgrCB->attrInfo, 0, sizeof(authmgrAuthAttributeInfo_t));

  authmgrCB->oldInfo = logicalPortInfo->client;
  memcpy(&authmgrCB->attrInfo, 
         &callbackParams->clientParams.info.authInfo.attrInfo,
         sizeof(authmgrAuthAttributeInfo_t));

  if (0 != strlen (callbackParams->clientParams.info.authInfo.authmgrUserName))
  {
    memcpy (logicalPortInfo->client.authmgrUserName,
            callbackParams->clientParams.info.authInfo.authmgrUserName,
            AUTHMGR_USER_NAME_LEN);
    logicalPortInfo->client.authmgrUserNameLength = 
          callbackParams->clientParams.info.authInfo.authmgrUserNameLength;
  }

  if ( SUCCESS != authmgrRadiusAcceptPostProcess (logicalPortInfo, &authmgrCB->processInfo,  AUTHMGR_ATTR_RADIUS))
  {
    /* update failure stats and move further */
    authmgrStatsUpdate (physPort, callbackParams->method, authmgrStatsAuthFail);

    if ( AUTHMGR_METHOD_NONE != callbackParams->method)
    {
      /* clean up the info at the caller */
      if ( NULLPTR != authmgrCB->globalInfo->authmgrCallbacks[callbackParams->method].eventNotifyFn)
      {
        authmgrCB->globalInfo->authmgrCallbacks[callbackParams->method].eventNotifyFn
          (physPort, authmgrClientDisconnect, &logicalPortInfo->client.suppMacAddr);
      }
    }

    logicalPortInfo->protocol.authSuccess =  FALSE;
    logicalPortInfo->protocol.authFail =  TRUE;

    return authmgrGenerateEvents (logicalPortInfo->key.keyNum);
  }
  /* Update Session timeout and terminate action */ 
  authmgrCB->processInfo.sessionTimeout    =  authmgrCB->attrInfo.sessionTimeout; 
  authmgrCB->processInfo.terminationAction =  RADIUS_TERMINATION_ACTION_DEFAULT; 
  if ((RADIUS_TERMINATION_ACTION_DEFAULT == authmgrCB->attrInfo.terminationAction) ||
      (RADIUS_TERMINATION_ACTION_RADIUS == authmgrCB->attrInfo.terminationAction))
  {
     authmgrCB->processInfo.terminationAction =  authmgrCB->attrInfo.terminationAction ; 
  }

  memcpy(&(logicalPortInfo->client.serverClass),
         &(authmgrCB->attrInfo.serverClass), authmgrCB->attrInfo.serverClassLen);
  logicalPortInfo->client.serverClassLen = authmgrCB->attrInfo.serverClassLen;

  if ( SUCCESS != authmgrClientHwInfoCleanupAndReadd(logicalPortInfo,
                                                       &authmgrCB->processInfo))
  {
    if ( AUTHMGR_METHOD_NONE != callbackParams->method)
    {
      /* clean up the info at the caller */
      if ( NULLPTR != authmgrCB->globalInfo->authmgrCallbacks[callbackParams->method].eventNotifyFn)
      {
        authmgrCB->globalInfo->authmgrCallbacks[callbackParams->method].eventNotifyFn
          (physPort, authmgrClientDisconnect, &logicalPortInfo->client.suppMacAddr);
      }
    }

    logicalPortInfo->protocol.authSuccess =  FALSE;
    logicalPortInfo->protocol.authFail =  TRUE;
    return authmgrGenerateEvents (lIntIfNum);
  }

  logicalPortInfo->protocol.authSuccess =  TRUE;
  logicalPortInfo->protocol.authFail =  FALSE;

    clientType =
      (callbackParams->method ==
        AUTHMGR_METHOD_8021X ? AUTHMGR_CLIENT_AWARE : AUTHMGR_CLIENT_UNAWARE);
    logicalPortInfo->client.clientType = clientType;

    if ( AUTH_METHOD_RADIUS ==
        callbackParams->clientParams.info.authInfo.authMethod)
    {
      logicalPortInfo->client.vlanType = AUTHMGR_VLAN_RADIUS;
    }
    else
    {
      logicalPortInfo->client.vlanType = AUTHMGR_VLAN_DEFAULT;
    }

  logicalPortInfo->client.authenticatedMethod = callbackParams->method;
  logicalPortInfo->client.authMethod =
    callbackParams->clientParams.info.authInfo.authMethod;

  logicalPortInfo->client.vlanId = authmgrCB->processInfo.vlanId;

  logicalPortInfo->client.sessionTimeout =
    authmgrCB->processInfo.sessionTimeout;
  logicalPortInfo->client.terminationAction =
    authmgrCB->processInfo.terminationAction;

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, physPort,
                       "%s:logicalPort num %d currentIdL %d\n",
                       __FUNCTION__, logicalPortInfo->key.keyNum,
                       authmgrCB->attrInfo.idFromServer);

  logicalPortInfo->client.currentIdL = authmgrCB->attrInfo.idFromServer;

  /* update statistics */
  authmgrStatsUpdate (physPort, callbackParams->method,
                      authmgrStatsAuthSuccess);
  return authmgrGenerateEvents (lIntIfNum);
}

/*********************************************************************
* @purpose deletes all the authenticated clients using the method
*
* @param    intIfNum @b{(input)) internal interface number
* @param    method @b{(input)) method for which entris are to be deleted
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrClientsByMethodDelete (uint32 intIfNum,
                                       AUTHMGR_METHOD_t method)
{
  /* This function purges all the clients who are
     authenticated using this method */
  uint32 lIndex;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, intIfNum,
                       "%s:Deleting clients authenticated with method %d on Physical port-%d \n",
                       __FUNCTION__, method, intIfNum);

  lIndex = AUTHMGR_LOGICAL_PORT_ITERATE;
  while ((logicalPortInfo =
          authmgrLogicalPortInfoGetNextNode (intIfNum, &lIndex)) !=  NULLPTR)
  {
    if (logicalPortInfo->client.authenticatedMethod == method)
    {
      /* cleanup the client */
      authmgrClientInfoCleanup (logicalPortInfo); 
    }
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose interface function to hanlde auth method change event
*
* @param intIfNum internal interface number
* @param callbackParams authentication callback params
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrStatusAuthModifyAction (uint32 intIfNum,
                                       authmgrAuthRespParams_t * callbackParams)
{
   AUTHMGR_METHOD_t orderList[ AUTHMGR_METHOD_LAST];
  AUTHMGR_IF_NULLPTR_RETURN_LOG (callbackParams);

  /* see if the method is disabled */

  if ( FALSE == callbackParams->clientParams.info.enableStatus)
  {
    /* method is disabled. If enabled in the order,
       delete all the authenticated clients and
       remove the method from the order */
    authmgrClientsByMethodDelete (intIfNum, callbackParams->method);
  }

  memcpy (orderList, authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority,
      sizeof (orderList));
  authmgrMethodModifyAction (intIfNum);

  if (( AUTHMGR_METHOD_MAB == callbackParams->method) && 
      ( AUTHMGR_PORT_AUTO == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode) &&
      ( AUTHMGR_MULTI_HOST_MODE == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].hostMode) &&
      (0 == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authCount))
  {
    if ( TRUE !=
        authmgrListArrayCompare (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].
          enabledMethods, orderList, sizeof(orderList)))
    {
      authmgrPortLearningModify(intIfNum);
    }
  }
  return  SUCCESS;
}

/*********************************************************************
* @purpose function to handle auth status of all clients
*
* @param callbackParams  authentication callback param
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrStatusAuthModifyAllAction (authmgrAuthRespParams_t *
                                          callbackParams)
{
  RC_t nimRc;
  uint32 phyIntf;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (callbackParams);

  /* loop through all the interfaces and initiate the changes */
  nimRc = authmgrFirstValidIntfNumber (&phyIntf);
  while (nimRc ==  SUCCESS)
  {
    /* clean up the hw info */
    authmgrMethodModifyAction (phyIntf);

    if ( DISABLE == callbackParams->clientParams.info.enableStatus)
    {
      authmgrClientsByMethodDelete (phyIntf, callbackParams->method);
    }

    nimRc = authmgrNextValidIntf (phyIntf, &phyIntf);
  }
  return  SUCCESS;
}

/*************************************************************************
* @purpose  function to get function map entry for the given method
*
* @param    type  @b{(input)} method type
* @param    elem @b{(input)}  Pointer to map entry
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments none
*
* @end
*************************************************************************/
RC_t authmgrStatusCalbackMapEntryGet ( AUTHMGR_STATUS_t type,
                                         authmgrStatusMap_t * elem)
{
  uint32 i = 0;
  static authmgrStatusMap_t authmgrStatusMap[] = {
    { AUTHMGR_NEW_CLIENT, authmgrNewClientAction},
    { AUTHMGR_AUTH_FAIL, authmgrStatusNotSuccessAction},
    { AUTHMGR_AUTH_SUCCESS, authmgrStatusSuccessAction},
    { AUTHMGR_AUTH_TIMEOUT, authmgrStatusNotSuccessAction},
    { AUTHMGR_AUTH_SERVER_COMM_FAILURE, authmgrStatusServerCommFailAction},
    { AUTHMGR_CLIENT_DISCONNECTED, authmgrStatusDisconnectAction},
    { AUTHMGR_METHOD_CHANGE, authmgrStatusAuthModifyAction}
  };

  for (i = 0; i < (sizeof (authmgrStatusMap) / sizeof (authmgrStatusMap_t));
       i++)
  {
    if (type == authmgrStatusMap[i].type)
    {
      *elem = authmgrStatusMap[i];
      return  SUCCESS;
    }
  }

  return  FAILURE;
}

/*********************************************************************
* @purpose  Control function to handle the events received from methods
*
* @param    intIfNum       @b{(input)) internal interface number
* @param    callbackParams       @b{(input)) status from the calling applications like
   802.1X/MAB/CP
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrClientCallbackEventProcess (uint32 intIfNum,
                                           authmgrAuthRespParams_t *
                                           callbackParams)
{
  uint32 lIntIfNum = 0;
   BOOL exists =  FALSE;
  authmgrStatusMap_t entry;
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 physPort = 0, lPort = 0, type = 0;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }
  if ( NULLPTR == callbackParams)
  {
    return  FAILURE;
  }

  if ( AUTHMGR_METHOD_CHANGE == callbackParams->status)
  {
    if ( FALSE == callbackParams->clientParams.info.enableStatus)
    {
      /* Allow some time for methods to disable. Otherwise Ping-Pong will return "enabled" for 
         disabled methods. However Ping-Pong should be optimized going forward and then this
         delay should be taken out. */
      osapiSleepMSec(100);
    }

    if ( ALL_INTERFACES == intIfNum)
    {
      authmgrStatusAuthModifyAllAction (callbackParams);
    }
    else
    {
      authmgrStatusAuthModifyAction (intIfNum, callbackParams);
    }
    return  SUCCESS;
  }

  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  if ( DOT1X_PAE_PORT_AUTH_CAPABLE != authmgrCB->globalInfo->authmgrPortInfo[intIfNum].paeCapabilities)
  {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
                       "Interface %d is not PAE capable port. Hence No action \n", intIfNum);
      return  SUCCESS;
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
                       "%s Received Event - %s  and mac address(%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x)for method- %s \n",
                       __FUNCTION__,
                       authmgrMethodStatusStringGet (callbackParams->status),
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[0],
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[1],
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[2],
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[3],
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[4],
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[5], authmgrMethodStringGet (callbackParams->method));

  lIntIfNum = 0;
  if (( AUTHMGR_AUTH_FAIL == callbackParams->status) ||
      ( AUTHMGR_AUTH_TIMEOUT == callbackParams->status) ||
      ( AUTHMGR_CLIENT_DISCONNECTED == callbackParams->status))
  {
    if (( SUCCESS != authmgrMacAddrInfoFind (&callbackParams->clientParams.info.authInfo.macAddr, 
                                               &lIntIfNum)) ||
        (0 == lIntIfNum))
    {
         AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
                       "%s client is not present. Ignoring the result \n",
                       __FUNCTION__);

      /* client doesn't exist, ignoring the result 
         for non existing client */
       return  SUCCESS;
    }
    else
    {
      if ( NULLPTR != authmgrLogicalPortInfoGet (lIntIfNum))
      {
        /* get the key and unpack */
        AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, lIntIfNum);

        if (physPort != intIfNum)
        {
          /* Event is received for a client on differnt interface. Hence return  FAILURE. */
           uchar8 ifNameEvent[ NIM_IF_ALIAS_SIZE + 1];
           uchar8 ifNameClient[ NIM_IF_ALIAS_SIZE + 1];

          nimGetIntfName (physPort,  ALIASNAME, ifNameClient);
          nimGetIntfName (intIfNum,  ALIASNAME, ifNameEvent);

          AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
                   "Event %s for the client %s is received on interface %s (intIfNum %d) but this client exists on the interface %s (intIfNum %d). "
                   "Hence ignoring.", authmgrMethodStatusStringGet (callbackParams->status),
                   AUTHMGR_PRINT_MAC_ADDR(callbackParams->clientParams.info.authInfo.macAddr.addr),
                   ifNameEvent, intIfNum, ifNameClient, physPort);
           LOGF ( LOG_SEVERITY_NOTICE,
                   "Update for the client %s is received on interface %s (intIfNum %d) but this client exists on the interface %s (intIfNum %d).",
                   AUTHMGR_PRINT_MAC_ADDR(callbackParams->clientParams.info.authInfo.macAddr.addr),
                   ifNameEvent, intIfNum, ifNameClient, physPort);
          return  FAILURE;
        }
        else
        {
          AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
                               "Received client is present on %d port. \n", intIfNum);
        }
      }
    }
  }
 
  lIntIfNum = 0;

  if ( AUTHMGR_CLIENT_DISCONNECTED == callbackParams->status)
  {
    if ( SUCCESS == authmgrMacAddrInfoFind (&callbackParams->clientParams.info.authInfo.macAddr, 
                                              &lIntIfNum))
    {
      logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
      AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);



    /* check if the client received is already authenticated */
    if ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
          logicalPortInfo->client.logicalPortStatus)
    {
      {

        if (logicalPortInfo->client.authenticatedMethod !=
            callbackParams->method)
        {
          /* check if the received method is of higher priority than the existing one */

          if (( AUTHMGR_METHOD_NONE != logicalPortInfo->client.authenticatedMethod) &&
              ( SUCCESS != authmgrPriorityPrecedenceValidate(intIfNum, 
                                                               logicalPortInfo->client.authenticatedMethod,
                                                               callbackParams->method)))
          {
            /* ignore the message for authentication 
               priority over rules */
            AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, intIfNum,
                "%s, %d, Trigger from method %s received. "
                "Client already authenticated with method %s for logicalInterface %d \n"
                "Ignoring the request as client current auth method has higher priority than received\n",
                __func__, __LINE__, authmgrMethodStringGet(callbackParams->method), 
                authmgrMethodStringGet(logicalPortInfo->client.authenticatedMethod),lIntIfNum);
            return  SUCCESS;
          }
        }
      }
    }
      authmgrClientInfoCleanup (logicalPortInfo);
    }
    else
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, intIfNum,
                           "%s:Client not found in AuthMgr. Calling Disconnect "
                           "for client with method %d on Physical port-%d \n",
                           __FUNCTION__, callbackParams->method, intIfNum);
      if ( NULLPTR !=
          authmgrCB->globalInfo->authmgrCallbacks[callbackParams->method].eventNotifyFn)
      {
          (void)authmgrCB->globalInfo->authmgrCallbacks[callbackParams->method].eventNotifyFn
          (intIfNum, authmgrClientDisconnect,
           &callbackParams->clientParams.info.authInfo.macAddr);
      }
    }
    return  SUCCESS;
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_CLIENT, intIfNum,
                       "\n%s:%d: Check Source Mac: %s Interface: %s \n",
                       __FUNCTION__, __LINE__, 
                       AUTHMGR_PRINT_MAC_ADDR(callbackParams->clientParams.info.authInfo.macAddr.addr),
                       authmgrIntfIfNameGet(intIfNum));                       

  /* check for the associated node */
  /* search the Mac address in the list of clients on the port */
  if (authmgrCheckMapPdu
      (intIfNum, callbackParams->clientParams.info.authInfo.macAddr.addr,
       &lIntIfNum, &exists) !=  SUCCESS)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, intIfNum,
                         "Failed to Process the authmgrClientCallbackEvent \n");
    if ( NULLPTR !=
        authmgrCB->globalInfo->authmgrCallbacks[callbackParams->method].eventNotifyFn)
    {
        (void)authmgrCB->globalInfo->authmgrCallbacks[callbackParams->method].eventNotifyFn
        (intIfNum, authmgrClientDisconnect,
         &callbackParams->clientParams.info.authInfo.macAddr);
    }
    return  SUCCESS;
  }

  if ( TRUE == exists)
  {
    logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);
    AUTHMGR_IF_NULLPTR_RETURN_LOG(logicalPortInfo);
  }
  else
  {
    logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);
    AUTHMGR_IF_NULLPTR_RETURN_LOG(logicalPortInfo);
    authmgrGenerateEvents (logicalPortInfo->key.keyNum);
  }

  if (0 != callbackParams->clientParams.info.authInfo.eapolVersion)
  {
    logicalPortInfo =  NULLPTR;

    logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);
    AUTHMGR_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
                       "%s Status %s Received EAPoL version for mac address(%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x)using method- %s EAPoL Version - %d \n",
                       __FUNCTION__,
                       authmgrMethodStatusStringGet (callbackParams->status),
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[0],
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[1],
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[2],
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[3],
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[4],
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[5], authmgrMethodStringGet (callbackParams->method),
                       callbackParams->clientParams.info.authInfo.eapolVersion);

    if (logicalPortInfo->client.rcvdEapolVersion != callbackParams->clientParams.info.authInfo.eapolVersion)
    {

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
                       "%s  Updating  EAPoL version for mac address(%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x) from version %d to version - %d \n",
                       __FUNCTION__,
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[0],
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[1],
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[2],
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[3],
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[4],
                       callbackParams->clientParams.info.authInfo.macAddr.
                       addr[5],
                       logicalPortInfo->client.rcvdEapolVersion,
                       callbackParams->clientParams.info.authInfo.eapolVersion);

      logicalPortInfo->client.rcvdEapolVersion = callbackParams->clientParams.info.authInfo.eapolVersion;
    }
  }

  if (( AUTHMGR_AUTH_SUCCESS == callbackParams->status) ||
      ( AUTHMGR_AUTH_FAIL == callbackParams->status) || 
      ( AUTHMGR_AUTH_TIMEOUT == callbackParams->status))
  {
     uchar8 ifNameEvent[ NIM_IFNAME_SIZE + 1];
    nimGetIntfName (intIfNum,  ALIASNAME, ifNameEvent);

    if (AUTHMGR_HELD == logicalPortInfo->protocol.authState)
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
               "Event %s for the client %s is received on interface %s (intIfNum %d) but this client is in HELD state. "
               "Hence ignoring.", authmgrMethodStatusStringGet (callbackParams->status),
               AUTHMGR_PRINT_MAC_ADDR(callbackParams->clientParams.info.authInfo.macAddr.addr),
               ifNameEvent, intIfNum);

      return  SUCCESS;
    }

    if ((AUTHMGR_AUTHENTICATED == logicalPortInfo->protocol.authState) && 
       (logicalPortInfo->client.authenticatedMethod != callbackParams->method))
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
               "Event %s for the client %s is received on interface %s (intIfNum %d) "
               "but this client's authenticated method (%s) is different from callback method (%s). "
               "Hence ignoring.", authmgrMethodStatusStringGet (callbackParams->status),
               AUTHMGR_PRINT_MAC_ADDR(callbackParams->clientParams.info.authInfo.macAddr.addr),
               ifNameEvent, intIfNum, authmgrMethodStringGet (logicalPortInfo->client.authenticatedMethod),
               authmgrMethodStringGet (callbackParams->method));

      return  SUCCESS;
    }
  }

  memset (&entry, 0, sizeof (authmgrStatusMap_t));
  if ( SUCCESS !=
      authmgrStatusCalbackMapEntryGet (callbackParams->status, &entry))
  {
    return  FAILURE;
  }

  return entry.statusFn (lIntIfNum, callbackParams);
}

/*********************************************************************
* @purpose  Get the first operationally enabled method on a interface
*
* @param    intIfNum  @b{(input)) internal interface number
* @param    nextMethod  @b{(output)) pointer to the next method
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrFirstMethodGet (uint32 intIfNum,
                                AUTHMGR_METHOD_t * nextMethod)
{
  uint32 j;

  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  if ( AUTHMGR_METHOD_NONE ==
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[0])
  {
    return  FAILURE;
  }

  j = 0;
  for (j =  AUTHMGR_METHOD_MIN; j <  AUTHMGR_METHOD_MAX; j++)
  {
    if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[j] !=
         AUTHMGR_METHOD_NONE)
    {
      *nextMethod =
        authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[j];
      return  SUCCESS;
    }
  }
  return  FAILURE;
}

/*********************************************************************
* @purpose  Get the next operationally enabled method on a interface
*
* @param    intIfNum  @b{(input)) internal interface number
* @param    nextMethod  @b{(output)) pointer to the next method
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrNextMethodGet (uint32 intIfNum,
                               AUTHMGR_METHOD_t * nextMethod)
{
  uint32 j;

  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  if ( AUTHMGR_METHOD_NONE ==
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[0])
  {
    return  FAILURE;
  }

  for (j =  AUTHMGR_METHOD_MIN; j <  AUTHMGR_METHOD_MAX; j++)
  {
    if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[j] ==
         AUTHMGR_METHOD_NONE)
    {
      return  FAILURE;
    }
    if ((*nextMethod ==
         authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[j])
        && ((j + 1) <  AUTHMGR_METHOD_MAX))
    {
      if ( AUTHMGR_METHOD_NONE !=
          authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[j +
                                                                          1])
      {
        *nextMethod =
          authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[j +
                                                                          1];
        return  SUCCESS;
      }
      else
      {
        return  FAILURE;
      }
    }
  }
  return  FAILURE;
}

/*********************************************************************
* @purpose  Get the next operationally enabled method on a interface
*
* @param    intIfNum  @b{(input)) internal interface number
* @param    nextMethod  @b{(output)) pointer to the next method
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/

RC_t authmgrEnabledMethodNextGet (uint32 intIfNum,
                                      AUTHMGR_METHOD_t * nextMethod)
{
  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  if ( AUTHMGR_METHOD_NONE ==
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[0])
  {
    return  FAILURE;
  }

  if ( AUTHMGR_METHOD_NONE == *nextMethod)
  {
    /* get the first method */
    return authmgrFirstMethodGet (intIfNum, nextMethod);
  }
  else
  {
    return authmgrNextMethodGet (intIfNum, nextMethod);
  }
}

/*********************************************************************
* @purpose  Get the first operationally enabled method on a interface
*
* @param    intIfNum  @b{(input)) internal interface number
* @param    nextPriority  @b{(output)) pointer to the next method
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/

RC_t authmgrFirstPriorityGet (uint32 intIfNum,
                                  AUTHMGR_METHOD_t * nextPriority)
{
  uint32 j;

  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  if ( AUTHMGR_METHOD_NONE ==
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority[0])
  {
    return  FAILURE;
  }

  j = 0;
  for (j =  AUTHMGR_METHOD_MIN; j <  AUTHMGR_METHOD_MAX; j++)
  {
    if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority[j] !=
         AUTHMGR_METHOD_NONE)
    {
      *nextPriority =
        authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority[j];
      return  SUCCESS;
    }
  }
  return  FAILURE;
}

/*********************************************************************
* @purpose  Get the next operationally enabled method on a interface
*
* @param    intIfNum  @b{(input)) internal interface number
* @param    method  @b{(input))  input method for which next method is needed.
* @param    nextPriority  @b{(output)) pointer to the next method
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/

RC_t authmgrNextPriorityGet (uint32 intIfNum,
                                 AUTHMGR_METHOD_t * nextPriority)
{
  uint32 j;

  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  if ( AUTHMGR_METHOD_NONE ==
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority[0])
  {
    return  FAILURE;
  }

  j = 0;
  for (j =  AUTHMGR_METHOD_MIN; j <  AUTHMGR_METHOD_MAX; j++)
  {
    if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority[j] ==
         AUTHMGR_METHOD_NONE)
    {
      return  FAILURE;
    }
    if ((*nextPriority ==
         authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority[j])
        && ((j + 1) <  AUTHMGR_METHOD_MAX))
    {
      if ( AUTHMGR_METHOD_NONE !=
          authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority[j +
                                                                           1])
      {
        *nextPriority =
          authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority[j +
                                                                           1];
        return  SUCCESS;
      }
      else
      {
        return  FAILURE;
      }
    }
  }
  return  FAILURE;
}

/*********************************************************************
* @purpose  Get the next operationally enabled method on a interface
*
* @param    intIfNum  @b{(input)) internal interface number
* @param    method  @b{(input))  input method for which next method is needed.
* @param    nextPriority  @b{(output)) pointer to the next method
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/

RC_t authmgrEnabledPriorityNextGet (uint32 intIfNum,
                                        AUTHMGR_METHOD_t * nextPriority)
{
  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  if ( AUTHMGR_METHOD_NONE ==
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority[0])
  {
    return  FAILURE;
  }

  if ( AUTHMGR_METHOD_NONE == *nextPriority)
  {
    /* get the first method */
    return authmgrFirstPriorityGet (intIfNum, nextPriority);
  }
  else
  {
    return authmgrNextPriorityGet (intIfNum, nextPriority);
  }
}

/*********************************************************************
* @purpose  status function to update dot1x stats
*
* @param intIfNum interface number
* @param status 
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrDot1xStatsUpdate (uint32 intIfNum,
                                 authmgrStatsUpdate_t status)
{
  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  switch (status)
  {
  case authmgrStatsAuthEnter:
    authmgrCB->globalInfo->authmgrPortStats[intIfNum].dot1x.
      authEntersAuthenticating++;
    break;

  case authmgrStatsAuthSuccess:
    authmgrCB->globalInfo->authmgrPortStats[intIfNum].dot1x.authSuccess++;
    break;

  case authmgrStatsAuthFail:
    authmgrCB->globalInfo->authmgrPortStats[intIfNum].dot1x.authFailure++;
    break;

  case authmgrStatsAuthTimeout:
    authmgrCB->globalInfo->authmgrPortStats[intIfNum].dot1x.authTimeout++;
    break;

  default:
    break;
  }
  return  SUCCESS;
}

/*********************************************************************
* @purpose status function to update mab stats
*
* @param intIfNum interface number
* @param status 
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrMabStatsUpdate (uint32 intIfNum, authmgrStatsUpdate_t status)
{
  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }
  switch (status)
  {
  case authmgrStatsAuthEnter:
    authmgrCB->globalInfo->authmgrPortStats[intIfNum].mab.
      authEntersAuthenticating++;
    break;

  case authmgrStatsAuthSuccess:
    authmgrCB->globalInfo->authmgrPortStats[intIfNum].mab.authSuccess++;
    break;

  case authmgrStatsAuthFail:
    authmgrCB->globalInfo->authmgrPortStats[intIfNum].mab.authFailure++;
    break;

  case authmgrStatsAuthTimeout:
    authmgrCB->globalInfo->authmgrPortStats[intIfNum].mab.authTimeout++;
    break;

  default:
    break;
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose status function to update captive portal stats
*
* @param intIfNum interface number
* @param status 
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrCpStatsUpdate (uint32 intIfNum, authmgrStatsUpdate_t status)
{
  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }
  switch (status)
  {
  case authmgrStatsAuthEnter:
    authmgrCB->globalInfo->authmgrPortStats[intIfNum].cp.
      authEntersAuthenticating++;
    break;

  case authmgrStatsAuthSuccess:
    authmgrCB->globalInfo->authmgrPortStats[intIfNum].cp.authSuccess++;
    break;

  case authmgrStatsAuthFail:
    authmgrCB->globalInfo->authmgrPortStats[intIfNum].cp.authFailure++;
    break;

  case authmgrStatsAuthTimeout:
    authmgrCB->globalInfo->authmgrPortStats[intIfNum].cp.authTimeout++;
    break;

  default:
    break;
  }
  return  SUCCESS;
}

/*************************************************************************
* @purpose  function to get function map entry for the given method
*
* @param    type  @b{(input)} method type
* @param    elem @b{(input)}  Pointer to map entry
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments none
*
* @end
*************************************************************************/
RC_t authmgrAuthStatsMapEntryGet ( AUTHMGR_METHOD_t type,
                                     authmgrStatsMap_t * elem)
{
  uint32 i = 0;
  static authmgrStatsMap_t authmgrStatsMap[] = {
    { AUTHMGR_METHOD_8021X, authmgrDot1xStatsUpdate},
    { AUTHMGR_METHOD_MAB, authmgrMabStatsUpdate}
  };

  for (i = 0; i < (sizeof (authmgrStatsMap) / sizeof (authmgrStatsMap_t)); i++)
  {
    if (type == authmgrStatsMap[i].type)
    {
      *elem = authmgrStatsMap[i];
      return  SUCCESS;
    }
  }

  return  FAILURE;
}

/*********************************************************************
* @purpose  Function to Update the statistics
*
* @param    intIfNum       @b{(input)) internal interface number
* @param    method         @b{(input)) 802.1x/mab/cp
* @param    status           @b{(input)) status 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments 
*
* @end
*********************************************************************/
RC_t authmgrStatsUpdate (uint32 intIfNum,
                             AUTHMGR_METHOD_t method,
                            authmgrStatsUpdate_t status)
{
  authmgrStatsMap_t entry;

  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  if ( SUCCESS != authmgrAuthStatsMapEntryGet (method, &entry))
  {
    return  FAILURE;
  }

  if ( NULLPTR != entry.statsFn)
  {
    return entry.statsFn (intIfNum, status);
  }

  return  FAILURE;
}

/*********************************************************************
* @purpose function to map callbacks for the given method
*
* @param method authentication method
* @param entry  auth mgr callback function map entry
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrRegisteredEntryFnMapGet ( AUTHMGR_METHOD_t method,
                                        authmgrMethodCallbackNotifyMap_t *
                                        entry)
{
  uint32 i = 0;

  if ( AUTHMGR_METHOD_NONE == method)
  {
    return  FAILURE;
  }

  for (i = 0; i <  AUTHMGR_METHOD_LAST; i++)
  {
    if (method == authmgrCB->globalInfo->authmgrCallbacks[i].method)
    {
      *entry = authmgrCB->globalInfo->authmgrCallbacks[i];
      return  SUCCESS;
    }
  }
  return  FAILURE;
}

/*********************************************************************
* @purpose function to check the entry and populate in the list
*
* @param intIfNum interface number
* @param method authentication method
* @param methodEnabled  TRUE if method is enabled 
* @param out mehod value, if method is emabled 
* @return   SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrOperListEntryPopulate (uint32 intIfNum,
                                       AUTHMGR_METHOD_t method,
                                       BOOL * methodEnabled,
                                       AUTHMGR_METHOD_t * out)
{
  authmgrMethodCallbackNotifyMap_t entry;
  uint32 enabled =  DISABLE;

  memset (&entry, 0, sizeof (authmgrMethodCallbackNotifyMap_t));

  AUTHMGR_IF_NULLPTR_RETURN_LOG (methodEnabled);
  AUTHMGR_IF_NULLPTR_RETURN_LOG (out);

  if ( SUCCESS != authmgrRegisteredEntryFnMapGet (method, &entry))
  {
    return  FAILURE;
  }

  if ( NULLPTR == entry.enableGetFn)
  {
    return  FAILURE;
  }

  /* Explicitly releasing the locks temporarily
     since , query is done for another component API
     which contains readlocks. */

  (void) osapiReadLockGive (authmgrCB->authmgrCfgRWLock);
  (void) osapiWriteLockGive (authmgrCB->authmgrRWLock);
  if (( SUCCESS == entry.enableGetFn (intIfNum, &enabled)) &&
      ( ENABLE != enabled))
  {
    (void) osapiReadLockTake (authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
    (void) osapiWriteLockTake (authmgrCB->authmgrRWLock,  WAIT_FOREVER);
    *methodEnabled =  FALSE;
    return  SUCCESS;
  }

  (void) osapiReadLockTake (authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  (void) osapiWriteLockTake (authmgrCB->authmgrRWLock,  WAIT_FOREVER);
  *methodEnabled =  TRUE;

  *out = method;
  return  SUCCESS;
}

/*********************************************************************
* @purpose  function to populate the oper enabled methods
*
* @param    intIfNum       @b{(input)) internal interface number
* @param    inArray       @b{(input)) input list 
* @param    outArray       @b{(output)) output list
* @param    count       @b{(output)) enabled count 
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments The outArray should be memset before passing to the function
*
* @end
*********************************************************************/
RC_t authmgrEnabledListPopulate (uint32 intIfNum,
                                     AUTHMGR_METHOD_t * inArray,
                                     AUTHMGR_METHOD_t * outArray,
                                    uint32 * count)
{
   BOOL flag =  FALSE;
  uint32 i = 0, cnt = 0;
    AUTHMGR_METHOD_t *pIn, *pOut;

  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  AUTHMGR_IF_NULLPTR_RETURN_LOG (inArray);
  AUTHMGR_IF_NULLPTR_RETURN_LOG (outArray);

  pIn = inArray;
  pOut = outArray;

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
                       "%s repopulating the enabled methods list\n",
                       __FUNCTION__);

  /* Loop through the configured methods
     and maintain an oper list */

  for (i = 0; i <  AUTHMGR_METHOD_MAX; i++)
  {
    if ( AUTHMGR_METHOD_NONE == *inArray)
    {
      break;
    }

    flag =  FALSE;

    if ( SUCCESS !=
        authmgrOperListEntryPopulate (intIfNum, *inArray, &flag, outArray))
    {
      break;
    }

    if (flag)
    {
      outArray++;
      cnt++;
    }
    inArray++;
  }

  *count = cnt;
  outArray = pOut;
  inArray = pIn;

  return  SUCCESS;
}

/*********************************************************************
* @purpose function to handle changes in enabled auth method list
*
* @param intIfNum internal interface number
* @param old old list
* @param new updated list
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrEnableListChangeAction (uint32 intIfNum,
                                        AUTHMGR_METHOD_t * old,
                                        AUTHMGR_METHOD_t * new)
{
  uint32 i = 0, j = 0;
   BOOL exists =  FALSE;
   AUTHMGR_METHOD_t *temp =  NULLPTR;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (old);
  AUTHMGR_IF_NULLPTR_RETURN_LOG (new);

  /* no lists in the order */
  while ((i <  AUTHMGR_METHOD_MAX) && ( AUTHMGR_METHOD_NONE != *old))
  {
    temp = old;
    exists =  FALSE;
    for (j = 0; j <  AUTHMGR_METHOD_MAX; j++)
    {
      if (*new == *old)
      {
        /* method is still present */
        exists =  TRUE;
        break;
      }
      temp++;
    }
    if (!(exists))
    {
      /* clean up all the clients in the list by method */
      authmgrClientsByMethodDelete (intIfNum,
                                    authmgrCB->globalInfo->
                                    authmgrPortInfo[intIfNum].
                                    enabledMethods[i]);
    }
    old++;
    i++;
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Control function to populate the oper enabled methods
*
* @param    intIfNum       @b{(input)) internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortEnabledListPopulate (uint32 intIfNum)
{
  authmgrPortCfg_t *pCfg =  NULLPTR;
   BOOL orderChanged =  FALSE, priorityChanged =  FALSE;
  uint32 count = 0, count1 = 0;
   uchar8   ifName[  NIM_IF_ALIAS_SIZE + 1];

   AUTHMGR_METHOD_t orderList[ AUTHMGR_METHOD_LAST];
   AUTHMGR_METHOD_t priorityList[ AUTHMGR_METHOD_LAST];
   AUTHMGR_METHOD_t zeroList[ AUTHMGR_METHOD_LAST];

  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  if (authmgrIntfIsConfigurable (intIfNum, &pCfg) !=  TRUE)
  {
    return  FAILURE;
  }

  AUTHMGR_IF_NULLPTR_RETURN_LOG (pCfg);

  if (nimGetIntfName (intIfNum,  ALIASNAME, ifName) !=  SUCCESS)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
        "Unable to get alias for intf %d", intIfNum);
    return  FAILURE;
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
                       "%s repopulating the oper methods list for intf %s\n",
                       __FUNCTION__, ifName);

  /* take the read lock */
  (void) osapiReadLockTake (authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  /* Loop through the configured methods
     and maintain an oper list */

  memset (orderList, 0, sizeof (orderList));
  memset (priorityList, 0, sizeof (priorityList));
  memset (zeroList, 0, sizeof (zeroList));

  /* get the enabled order list */

  if ( SUCCESS !=
      authmgrEnabledListPopulate (intIfNum, &pCfg->methodList[0], &orderList[0],
                                  &count))
  {
    (void) osapiReadLockGive (authmgrCB->authmgrCfgRWLock);
    return  FAILURE;
  }

  if ( SUCCESS !=
      authmgrEnabledListPopulate (intIfNum, &pCfg->priorityList[0],
                                  &priorityList[0], &count1))
  {
    (void) osapiReadLockGive (authmgrCB->authmgrCfgRWLock);
    return  FAILURE;
  }

  /* check if there is any change in the oper values */

  if ( TRUE !=
      authmgrListArrayCompare (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].
                               enabledMethods, orderList, sizeof(orderList)))
  {
    orderChanged =  TRUE;
  }

  if ( TRUE !=
      authmgrListArrayCompare (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].
                               enabledPriority, priorityList, sizeof(priorityList)))
  {
    priorityChanged =  TRUE;
  }

  (void) osapiReadLockGive (authmgrCB->authmgrCfgRWLock);

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
      "%s intf %s, orderChanged %d, priorityChanged %d\n",
      __FUNCTION__, ifName, orderChanged, priorityChanged);

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
      "%s %s -- cfgMethods[0] %d, cfgMethods[1] %d\n",
      __FUNCTION__, ifName, pCfg->methodList[0],
      pCfg->methodList[1]);

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
      "%s %s -- generated list- methods[0] %d, methods[1] %d\n",
      __FUNCTION__, ifName, orderList[0],
      orderList[1]);

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
      "%s %s current list- methods[0] %d, methods[1] %d\n",
      __FUNCTION__, ifName, authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[0],
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[1]);

  if (orderChanged)
  {
    if ( SUCCESS !=
        authmgrEnableListChangeAction (intIfNum,
                                       authmgrCB->globalInfo->
                                       authmgrPortInfo[intIfNum].enabledMethods,
                                       orderList))
    {
      return  FAILURE;
    }

    memcpy (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods,
            orderList, sizeof (orderList));

    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
        "%s enabledMethods[0] %d, enabledMethods[1] %d\n",
        __FUNCTION__, authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[0],
        authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[1]);

      /* we may need to alter the violation policy
         based on the new list  */
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
       "%s Updating learn mode of %s\n",  __FUNCTION__, ifName);

    authmgrViolationPolicyApply(intIfNum); 
    authmgrPortLearningModify(intIfNum);

    if ( TRUE == authmgrListArrayCompare (priorityList, zeroList, sizeof(priorityList)))
    {
      memcpy (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority,
              orderList, sizeof (orderList));
    }
    else if (priorityChanged)
    {
      memcpy (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority,
              priorityList, sizeof (priorityList));
    }

    authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethodCount = count;
    authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriorityCount =
      count1;
  }
  else if (priorityChanged)
  {
    memcpy (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority,
            priorityList, sizeof (priorityList));
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
      "%s Updating DB: enabledMethods[0] %d, enabledMethods[1] %d\n",
      __FUNCTION__, authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[0],
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods[1]);

  /* Update Oper DB */

  PacPortOperTblSet(intIfNum, 
                    authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods,
                    authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority);

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Control function to handle the authentication method order changes
*
* @param    intIfNum       @b{(input)) internal interface number
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrMethodOrderChangeProcess (uint32 intIfNum)
{
  authmgrPortCfg_t *pCfg;

  if ( TRUE != authmgrIntfIsConfigurable (intIfNum, &pCfg))
  {
    return  FAILURE;
  }

  /* populate the methods if and only if
     the control mode is valid */

  if ( AUTHMGR_PORT_AUTO == pCfg->portControlMode)
  {
    /* Just re-populate the interface enabled list */
    authmgrPortEnabledListPopulate (intIfNum);
  }
  else
  {
			AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, intIfNum,
													 "%s intf %d, Zeroing out methods for portControlMode %d\n",
													 __FUNCTION__, intIfNum, pCfg->portControlMode);
    /* just memset the methods */
    memset (&authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethods, 0,
            sizeof (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].
                    enabledMethods));
    memset (&authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriority,
            0,
            sizeof (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].
                    enabledPriority));
    authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledMethodCount = 0;
    authmgrCB->globalInfo->authmgrPortInfo[intIfNum].enabledPriorityCount = 0;
  }
  return  SUCCESS;
}

/*********************************************************************
* @purpose function to trigger authentication for a client.
*
* @param logicalPortInfo logical port structure
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrAuthenticationTrigger (authmgrLogicalPortInfo_t *
                                      logicalPortInfo)
{
  RC_t rc =  SUCCESS;
  uint32 physPort = 0;
   enetMacAddr_t zeroMac;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);
  AUTHMGR_PORT_GET (physPort, logicalPortInfo->key.keyNum);

  if ( AUTHMGR_METHOD_NONE == logicalPortInfo->client.currentMethod)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                         "logicalInterface %d failed to update %s to start authentication\n",
                         logicalPortInfo->key.keyNum,
                         authmgrMethodStringGet (logicalPortInfo->client.
                                                 currentMethod));
    rc =  FAILURE;
  }

  if (logicalPortInfo->client.currentMethod !=
      authmgrCB->globalInfo->authmgrCallbacks[logicalPortInfo->client.
                                              currentMethod].method)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                         "logicalInterface %d failed to update %s to start authentication\n"
                         "since the registered method is %s\n",
                         logicalPortInfo->key.keyNum,
                         authmgrMethodStringGet (logicalPortInfo->client.
                                                 currentMethod),
                         authmgrMethodStringGet (authmgrCB->globalInfo->
                                                 authmgrCallbacks
                                                 [logicalPortInfo->client.
                                                  currentMethod].method));
    rc =  FAILURE;
  }

  if ( NULLPTR ==
      authmgrCB->globalInfo->authmgrCallbacks[logicalPortInfo->client.
                                              currentMethod].eventNotifyFn)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                         "logicalInterface %d failed to update %s to start authentication\n"
                         "since the callback function is not registered method\n",
                         logicalPortInfo->key.keyNum,
                         authmgrMethodStringGet (logicalPortInfo->client.
                                                 currentMethod));
    rc =  FAILURE;
  }

  /* check if the client mac is all 0s.
     If yes, no need to validate against MAB */

  memset (&zeroMac, 0, sizeof ( enetMacAddr_t));
  if ((0 ==
      memcmp (zeroMac.addr,
        logicalPortInfo->client.suppMacAddr.
        addr,  ENET_MAC_ADDR_LEN)) && 
        ( AUTHMGR_METHOD_MAB == 
        logicalPortInfo->client.currentMethod))
  {
    rc =  FAILURE;
  }

  if (AUTHMGR_AUTHENTICATING != logicalPortInfo->protocol.authState)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, physPort,
                        "client  with logicalInterface %d is in state %s.\n",
          logicalPortInfo->key.keyNum, authmgrAuthStateStringGet(logicalPortInfo->protocol.authState));
    return  SUCCESS;
  }

  if (( SUCCESS == rc) && ( NULLPTR != 
      authmgrCB->globalInfo->authmgrCallbacks[logicalPortInfo->client.
                                              currentMethod].eventNotifyFn))
  {
    rc =
      authmgrCB->globalInfo->authmgrCallbacks[logicalPortInfo->client.
                                              currentMethod].eventNotifyFn
      (physPort, authmgrClientAuthStart,
       &logicalPortInfo->client.suppMacAddr);
    authmgrStatsUpdate (physPort, logicalPortInfo->client.currentMethod,
                        authmgrStatsAuthEnter);

     if ( SUCCESS == rc)
     {
       /* start the method_no_response timer. If this method doesn't 
          report back the result, we will move to next method */
          authmgrTimerStart (logicalPortInfo, AUTHMGR_METHOD_NO_RESP_TMR);
     }
  }
  return rc;
}

/*********************************************************************
* @purpose function to get the operationally enabled method count on an interface
*
* @param physPort internal interface number
* @param count count
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPortEnabledMethodCountGet (uint32 physPort, uint32 * count)
{
  *count = authmgrCB->globalInfo->authmgrPortInfo[physPort].enabledMethodCount;
  return  SUCCESS;
}

/*********************************************************************
* @purpose updates the port pae capabilities
*
* @param   intIfNum
* @param   mode  pae capabilities
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrPaeCapabilitiesEventProcess (uint32 intIfNum, uint32 mode)
{
   uchar8                ifName[ NIM_IF_ALIAS_SIZE + 1];
  RC_t rc =  FAILURE;

  if (!(AUTHMGR_IS_READY))
  {
    return  SUCCESS;
  }

  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  if (mode == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].paeCapabilities)
  {
    return  SUCCESS;
  }

  if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].paeCapabilities ==  DOT1X_PAE_PORT_AUTH_CAPABLE)
  {
    /* cleanup the clients on this port */
    authmgrPortInfoCleanup (intIfNum);
  }

  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].paeCapabilities = mode;

  if ( DOT1X_PAE_PORT_AUTH_CAPABLE == mode)
  {
      /* enable authentication on this interface */
      (void)authmgrCtlApplyPortConfigData(intIfNum);
  }
  else
  {
    authmgrIhPhysicalPortStatusSet(intIfNum,  AUTHMGR_PORT_STATUS_AUTHORIZED);
    if (nimGetIntfName (intIfNum,  ALIASNAME, ifName) !=  SUCCESS)
    {
       LOGF ( LOG_SEVERITY_ERROR,
               "Unable to get alias for intf %d", intIfNum);
      return  FAILURE;
    }

    rc = authmgrPortPvidSet(intIfNum, 0);
    if (rc !=  SUCCESS)
    {
       LOGF ( LOG_SEVERITY_ERROR,
               "Unable to set PVID back to 0 for %d", intIfNum);
    }

    authmgrVlanReleasePort(intIfNum);

    /* disable authentication on this interface */
    if ( TRUE == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portEnabled)
    {
      authmgrPhysicalPortStatusOpenSet(intIfNum);
    }
    else
    {
      authmgrPhysicalPortStatusBlockSet(intIfNum);
    }
    
    pacCfgVlanMemberRemove(1, ifName);
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose updates the port violation mode
*
* @param   intIfNum
* @param   mode  violation mode
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrViolationModeSetAction (uint32 intIfNum,
                                    AUTHMGR_PORT_AUTH_VIOLATION_MODE_t mode)
{
  if (!(AUTHMGR_IS_READY))
  {
    return  SUCCESS;
  }

  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  if (mode == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].violationMode)
  {
    return  SUCCESS;
  }

  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].violationMode = mode;

  return  SUCCESS;
}

/*********************************************************************
* @purpose set max auth retry attempts on port
*
* @param   intIfNum
* @param   intIfNum
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrAuthFailMaxRetryCountSetAction (uint32 intIfNum,
                                               uint32 count)
{
  if (!(AUTHMGR_IS_READY))
  {
    return  SUCCESS;
  }

  if (authmgrIsValidIntf (intIfNum) !=  TRUE)
  {
    return  FAILURE;
  }

  if (count ==
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authFailRetryMaxCount)
  {
    return  SUCCESS;
  }

  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authFailRetryMaxCount =
    count;

  return  SUCCESS;
}

/*********************************************************************
* @purpose interface function to clear all timers of specified type
*
* @param physIntf internal interface number
* @param type timer type
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrInterfaceTimerReset (uint32 physIntf, authmgrTimerType_t type)
{
  uint32 lIndex = 0;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  lIndex = AUTHMGR_LOGICAL_PORT_ITERATE;
  while ((logicalPortInfo =
          authmgrLogicalPortInfoGetNextNode (physIntf, &lIndex)) !=  NULLPTR)
  {
    authmgrTimerDestroy (authmgrCB->globalInfo->authmgrTimerCB, logicalPortInfo,
                         type);
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose function to clear all timers of specified type
*
* @param type timer type
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrTimerReset (authmgrTimerType_t type)
{
  RC_t nimRc;
  uint32 phyIntf = 0;

  /* loop through all the interfaces and initiate the changes */
  nimRc = authmgrFirstValidIntfNumber (&phyIntf);
  while (nimRc ==  SUCCESS)
  {
    /* clean up the timers on this interface info */
    authmgrInterfaceTimerReset (phyIntf, type);
    nimRc = authmgrNextValidIntf (phyIntf, &phyIntf);
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose interface funtion to handle the auth method changes
*
* @param intIfNum  internal interface number
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrMethodModifyAction (uint32 intIfNum)
{
  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }

  authmgrMethodOrderChangeProcess (intIfNum);
  return  SUCCESS;
}

/*********************************************************************
* @purpose function to get the reauthentication period of client
*
* @param lIntfNum client interface number
* @param val  value
* @return  SUCCESS or  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortReAuthPeriodGet (uint32 lIntfNum, uint32 * val)
{
  uint32 physPort = 0;
  authmgrLogicalPortInfo_t *logicalPortInfo;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (val);

  logicalPortInfo = authmgrLogicalPortInfoGet (lIntfNum);
  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_PORT_GET (physPort, lIntfNum);

  if ( FALSE == authmgrCB->globalInfo->authmgrPortInfo[physPort].reAuthEnabled)
  {
    *val = 0;
  }

  if (authmgrCB->globalInfo->authmgrPortInfo[physPort].reAuthPeriodServer)
  {
    *val = logicalPortInfo->client.sessionTimeout;
  }
  else
  {
    authmgrReAuthPeriodGet (physPort, val);
  }

  return  SUCCESS;
}

/*********************************************************************
* @purpose interface level function to restart timers
*
* @param phyIntf interface number
*
* @comments
*
* @end
*********************************************************************/
void authmgrIntfAuthClientsTimersRestart (uint32 phyIntf)
{
  uint32 lIntIfNum;
  authmgrLogicalPortInfo_t *logicalPortInfo;
  /* Authenticator timer actions */
  if (authmgrCB->globalInfo->authmgrPortInfo[phyIntf].paeCapabilities ==
       DOT1X_PAE_PORT_AUTH_CAPABLE)
  {
    lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;
    while ((logicalPortInfo =
            authmgrLogicalPortInfoGetNextNode (phyIntf,
                                               &lIntIfNum)) !=  NULLPTR)
    {
      if ((logicalPortInfo->key.keyNum !=  0) &&
          ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
           logicalPortInfo->client.logicalPortStatus))
      {
        if (authmgrCB->globalInfo->authmgrPortInfo[phyIntf].portEnabled ==
             TRUE
            && (authmgrCB->globalInfo->authmgrPortInfo[phyIntf].
                portControlMode ==  AUTHMGR_PORT_AUTO))
        {
          if ( TRUE == authmgrCB->globalInfo->authmgrPortInfo[phyIntf].reAuthEnabled)
          {
            /* Start the reauthentication timer */
            authmgrTimerStart (logicalPortInfo, AUTHMGR_REAUTH_WHEN);
          }
        }
      }
    }
  }
}

/*********************************************************************
* @purpose function to restart all the timers
*
* @param  none
* @returns  none
*
*
* @comments
*
* @end
*********************************************************************/
void authmgrAuthClientsTimersRestart ()
{
  RC_t nimRc =  SUCCESS;
  uint32 phyIntf = 0;

  nimRc = authmgrFirstValidIntfNumber (&phyIntf);
  while (nimRc ==  SUCCESS)
  {
    /* Authenticator timer actions */
    if (authmgrCB->globalInfo->authmgrPortInfo[phyIntf].paeCapabilities ==
         DOT1X_PAE_PORT_AUTH_CAPABLE)
    {
      authmgrIntfAuthClientsTimersRestart (phyIntf);
    }
    nimRc = authmgrNextValidIntf (phyIntf, &phyIntf);
  }
}

/*********************************************************************
* @purpose function to start timers on an interface
*
* @param phyIntf interface number
* @param type timer type
* @param flag start or stop
*
* @comments
*
* @end
*********************************************************************/
void authmgrIntfClientsTimerStart (uint32 phyIntf, authmgrTimerType_t type,
                                    BOOL flag)
{
  uint32 lIntIfNum;
  authmgrLogicalPortInfo_t *logicalPortInfo;
  /* Authenticator timer actions */
  if (authmgrCB->globalInfo->authmgrPortInfo[phyIntf].paeCapabilities ==
       DOT1X_PAE_PORT_AUTH_CAPABLE)
  {
    lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;
    while ((logicalPortInfo =
            authmgrLogicalPortInfoGetNextNode (phyIntf,
                                               &lIntIfNum)) !=  NULLPTR)
    {
      if (logicalPortInfo->key.keyNum !=  0)
      {
        if (authmgrCB->globalInfo->authmgrPortInfo[phyIntf].portEnabled ==
             TRUE
            && (authmgrCB->globalInfo->authmgrPortInfo[phyIntf].
                portControlMode ==  AUTHMGR_PORT_AUTO))
        {
          if ( TRUE == flag)
          {
            authmgrTimerStart (logicalPortInfo, type);
          }
          else
          {
            authmgrTimerDestroy (authmgrCB->globalInfo->authmgrTimerCB,
                                 logicalPortInfo, type);
          }
        }
      }
    }
  }
}

/*********************************************************************
* @purpose Function to start all timers of the given type
*
* @param type timer type
* @param flag start or stop
*
* @comments
*
* @end
*********************************************************************/
void authmgrAllTimersStart (authmgrTimerType_t type,  BOOL flag)
{
  RC_t nimRc =  SUCCESS;
  uint32 phyIntf = 0;

  nimRc = authmgrFirstValidIntfNumber (&phyIntf);
  while (nimRc ==  SUCCESS)
  {
    /* Authenticator timer actions */

    if (authmgrCB->globalInfo->authmgrPortInfo[phyIntf].paeCapabilities ==
         DOT1X_PAE_PORT_AUTH_CAPABLE) 
    {
      authmgrIntfClientsTimerStart (phyIntf, type, flag);
    }
    nimRc = authmgrNextValidIntf (phyIntf, &phyIntf);
  }
}

/*********************************************************************
 * @purpose  Enable administrative mode setting for authmgr
 *
 * @param    none
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrCtlAdminModeEnable()
{
  uint32 intIfNum;
/*   uchar8  paeCapabilities = 0; */
  RC_t nimRc;
  authmgrPortCfg_t *pCfg;

  if (!(AUTHMGR_IS_READY))
    return  SUCCESS;

  /* Initialize the auth mgr global data */
  authmgrGlobalInfoPopulate();

  /* Register for time ticks with appTimer */
  authmgrCB->globalInfo->authmgrTimerCB =
    appTimerInit ( AUTHMGR_COMPONENT_ID, authmgrTimerExpiryHdlr,  NULLPTR,
         APP_TMR_1SEC,
        authmgrCB->globalInfo->authmgrAppTimerBufferPoolId);

  auth_mgr_eap_socket_create(&authmgrCB->globalInfo->eap_socket);

  nimRc = authmgrFirstValidIntfNumber(&intIfNum);
  while (nimRc ==  SUCCESS)
  {
    if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
      return  FAILURE;


    (void)authmgrPortInfoInitialize(intIfNum, TRUE);
    if (authmgrCB->globalInfo->authmgrPortInfo[intIfNum].paeCapabilities ==  DOT1X_PAE_PORT_AUTH_CAPABLE)
    {
      (void)authmgrCtlApplyPortConfigData(intIfNum);
    }
    else
    {
      if ( TRUE == authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portEnabled)
      {
        authmgrPhysicalPortStatusOpenSet(intIfNum);
      }
      else
      {
        authmgrPhysicalPortStatusBlockSet(intIfNum);
      }
    }
    nimRc = authmgrNextValidIntf( intIfNum, &intIfNum);
  }
  return  SUCCESS;
}

/*********************************************************************
 * @purpose  Disable administrative mode setting for authmgr
 *
 * @param    none
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments
 *
 * @end
 *********************************************************************/
RC_t authmgrCtlAdminModeDisable()
{
  uint32 phyIntf;
 RC_t nimRc;

  if (!(AUTHMGR_IS_READY))
  {
    return  SUCCESS;
  }

  nimRc = authmgrFirstValidIntfNumber(&phyIntf);

  while (nimRc ==  SUCCESS)
  {
    authmgrPortInfoCleanup(phyIntf);
    authmgrPhysicalPortAccessSet(phyIntf);
    memset(&authmgrCB->globalInfo->authmgrPortInfo[phyIntf].enabledMethods[0],
        0, sizeof(authmgrCB->globalInfo->authmgrPortInfo[phyIntf].enabledMethods));

    nimRc = authmgrNextValidIntf(phyIntf, &phyIntf);
  }

  /* stop the timer */
  if ( NULLPTR != authmgrCB->globalInfo->authmgrTimerCB)
  {
    (void) appTimerDeInit (authmgrCB->globalInfo->authmgrTimerCB);
    authmgrCB->globalInfo->authmgrTimerCB =  NULLPTR;
  }

  memset(&authmgrCB->globalInfo->authmgrVlanMask, 0, sizeof( VLAN_MASK_t));

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Initialize the Authmgr Port Structure with Default Values
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
RC_t authmgrPortInfoInitialize(uint32 intIfNum,  BOOL flag)
{
  authmgrPortCfg_t *pCfg;
  uint32 linkState, adminState, maxUsers = 0;
   ushort16 oldPvid = 0;
   char8 ifName[ NIM_IF_ALIAS_SIZE + 1];
  RC_t rc;
   AUTHMGR_PORT_CONTROL_t cfgPortControlMode =  AUTHMGR_PORT_CONTROL_INVALID;
   AUTHMGR_HOST_CONTROL_t cfgHostMode =  AUTHMGR_INVALID_HOST_MODE;
   uchar8 capabilities =  DOT1X_PAE_PORT_NONE_CAPABLE;

  if (authmgrIntfIsConfigurable(intIfNum, &pCfg) !=  TRUE)
    return  SUCCESS;

  /* clean up previous info */
  oldPvid = authmgrCB->globalInfo->authmgrPortInfo[intIfNum].pvid;
  if (oldPvid)
  {
    rc = authmgrPortPvidSet(intIfNum, 0);
    if (rc !=  SUCCESS)
    {
      if (nimGetIntfName (intIfNum,  ALIASNAME, ifName) !=  SUCCESS)
      {
         LOGF ( LOG_SEVERITY_ERROR,
                 "Unable to get alias for intf %s", authmgrIntfIfNameGet(intIfNum));
        return  FAILURE;
      }
       LOGF ( LOG_SEVERITY_INFO,
               "Unable to set PVID back to 0 for %s", ifName);
    }
  }

  memset(&authmgrCB->globalInfo->authmgrPortInfo[intIfNum], 0, sizeof(authmgrPortInfo_t));

  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].maxUsers           =  AUTHMGR_MAX_USERS_PER_PORT;
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].numUsers           = 0;

  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].initialize         =  FALSE;
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authVlan         =  0;

  if ( (nimGetIntfLinkState(intIfNum, &linkState) ==  SUCCESS) && (linkState ==  UP) &&
       (nimGetIntfAdminState(intIfNum, &adminState) ==  SUCCESS) && (adminState ==  ENABLE) )
    authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portEnabled =  TRUE;
  else
    authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portEnabled =  FALSE;

  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authCount                = 0;
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portStatus               =  AUTHMGR_PORT_STATUS_UNAUTHORIZED;

  (void)osapiReadLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
   cfgPortControlMode = pCfg->portControlMode;
   cfgHostMode = pCfg->hostMode;
   capabilities = pCfg->paeCapabilities;
  (void)osapiReadLockGive(authmgrCB->authmgrCfgRWLock);

  /* Copy config data into operational data */
  if (cfgPortControlMode !=  AUTHMGR_PORT_AUTO)
  {
      AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_EVENTS,intIfNum,
          "Setting the max users to %d on interface %d. Current port control mode is not Auto \n",
           1, intIfNum);
    authmgrCB->globalInfo->authmgrPortInfo[intIfNum].maxUsers =  1;
  }
  else
  {
    if ( SUCCESS != authmgrMaxUsersGet(intIfNum, &maxUsers))
    {
      AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_FAILURE,intIfNum,
          "Unable to retrieve the max users. Current host mode is %s \n",
          authmgrHostModeStringGet(cfgHostMode));
    }
    else
    {
      AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_EVENTS,intIfNum,
          "Setting the max users to %d on interface %d. Current host mode is %s \n",
          maxUsers, intIfNum, authmgrHostModeStringGet(cfgHostMode));
      authmgrCB->globalInfo->authmgrPortInfo[intIfNum].maxUsers = maxUsers;
    }
  }

  (void)osapiReadLockTake(authmgrCB->authmgrCfgRWLock,  WAIT_FOREVER);
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].quietPeriod = pCfg->quietPeriod;
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].reAuthPeriod = pCfg->reAuthPeriod;
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].reAuthPeriodServer = pCfg->reAuthPeriodServer;
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].reAuthEnabled = pCfg->reAuthEnabled;
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authFailRetryMaxCount = pCfg->maxAuthAttempts;

  (void)osapiReadLockGive(authmgrCB->authmgrCfgRWLock);

  /* Get the PAE capabilities */ 
  authmgrCB->globalInfo->authmgrPortInfo[intIfNum].paeCapabilities = capabilities;
    
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Set values of the Logical Authmgr Port Structure
*           with Default Values of port it belongs to
*
* @param    logicalPortInfo  @b{(input))  Logical port Info
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortInfoInit(uint32 lIntIfNum)
{
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 physPort = 0, lPort = 0, type = 0, vlanId = 0;

  logicalPortInfo = authmgrLogicalPortInfoGet(lIntIfNum);

  AUTHMGR_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  AUTHMGR_LPORT_KEY_UNPACK(physPort, lPort, type, logicalPortInfo->key.keyNum);
  AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_CLIENT, physPort,
                        "%s:Resetting information for linterface = %d . \n",
                        __FUNCTION__, lPort);

  if ( AUTHMGR_PORT_STATUS_AUTHORIZED == logicalPortInfo->client.logicalPortStatus)
  {
    /* Clean up the client hw info */
    AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_CLIENT,physPort,
        "\n%s:Cleaning up hw info for linterface = %d\n",
        __FUNCTION__,lIntIfNum);

    if ( SUCCESS != authmgrClientHwInfoCleanup(logicalPortInfo))
    {
      AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_FAILURE,physPort,
          "\n%s:Resetting client hw settings for linterface = %d failed. \n",
          __FUNCTION__, lIntIfNum);
    }
  }

  memset(&logicalPortInfo->protocol, 0, sizeof(authmgrProtocolInfo_t));
  memset(&logicalPortInfo->client, 0, sizeof(authmgrClientInfo_t));

  uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1] = {'\0'};

  if (nimGetIntfName(physPort,  ALIASNAME, ifName) ==  SUCCESS)
  {
    pacCfgPortPVIDGet(ifName, &vlanId);
  }

 
  logicalPortInfo->client.vlanId         = vlanId;
  logicalPortInfo->client.vlanType       = AUTHMGR_VLAN_DEFAULT;
  logicalPortInfo->client.rcvdEapolVersion =  DOT1X_PAE_PORT_PROTOCOL_VERSION_2;
  authmgrStateMachineClassifier(authmgrInitialize, logicalPortInfo->key.keyNum); 

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Populate Auth Manager Global Info structure
*
* @param    none
*
* @returns  none
*
* @comments
*
* @end
*********************************************************************/
void authmgrGlobalInfoPopulate()
{
  memset(&authmgrCB->globalInfo->authmgrVlanMask, 0, sizeof( VLAN_MASK_t));
}


RC_t authmgrVlanClientsCleanup(uint32 vlanId)
{
  uint32 intIfNum = 0, lIntIfNum = 0;
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  RC_t nimRc =  SUCCESS;

  nimRc = authmgrFirstValidIntfNumber (&intIfNum);
  while (nimRc ==  SUCCESS)
  {
    if ((0 != authmgrCB->globalInfo->authmgrPortInfo[intIfNum].authCount)
        && ( AUTHMGR_PORT_AUTO ==
          authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portControlMode))
    {
      lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;
      while ((logicalPortInfo =
            authmgrLogicalPortInfoGetNextNode (intIfNum,
              &lIntIfNum)) !=  NULLPTR)
      {
        if (vlanId == logicalPortInfo->client.vlanId)
        {
          authmgrClientInfoCleanup(logicalPortInfo);
        }
      }
    }
    nimRc = authmgrNextValidIntf (intIfNum, &intIfNum);
  }
  return  SUCCESS;
}

/*********************************************************************
* @purpose  To close the authenticated sessions gracefully.
*
*
* @returns    SUCCESS
* @returns    FAILURE
*
* @comments
*       
* @end
*********************************************************************/
RC_t authmgrTerminateAuthSessions()
{
  uint32                intIfNum = 0, lIntIfNum = 0, mgmtUnit = 0;
  RC_t                  rc =  SUCCESS;
  authmgrLogicalPortInfo_t *logicalPortInfo;
  nimUSP_t                 usp;

  /* Get the first Valid Interface and cleanup the authentication sessions 
   * belongs to this unit.
   */
  rc = authmgrFirstValidIntfNumber(&intIfNum);
  while (rc ==  SUCCESS)
  {
    memset((void *)&usp, 0, sizeof(nimUSP_t));
    if(nimGetUnitSlotPort(intIfNum, &usp) ==  SUCCESS)
    {
      if (usp.unit != ( uchar8)mgmtUnit)
      {
        rc = authmgrNextValidIntf(intIfNum, &intIfNum);
        continue;  
      }
    }
    else
    {
       rc = authmgrNextValidIntf(intIfNum, &intIfNum);
       continue;
    }

    lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;
    authmgrCB->globalInfo->authmgrPortInfo[intIfNum].portEnabled =  FALSE;
    while ( NULLPTR != 
           (logicalPortInfo = authmgrLogicalPortInfoGetNextNode (intIfNum, &lIntIfNum)))
    {
      if (0 != logicalPortInfo->key.keyNum)
      {
        if ( AUTHMGR_PORT_STATUS_AUTHORIZED == logicalPortInfo->client.logicalPortStatus)
        {
          AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_API_CALLS,intIfNum,
               "%s Auth Manager Cleanup the client session %d on port %s\n",
               __FUNCTION__, logicalPortInfo->key.keyNum, authmgrIntfIfNameGet(intIfNum));

          (void)authmgrClientInfoCleanup(logicalPortInfo);
        }
      }
    }
    rc = authmgrNextValidIntf(intIfNum, &intIfNum);
  }
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Reset port information
*
* @param    intIfNum   @b{(input)) internal interface number
* @param    initialize @b{(input)) initialize value
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
RC_t authmgrCtlPortReset(uint32 intIfNum,  BOOL initialize)
{
  RC_t rc =  SUCCESS;

  if (authmgrCB->globalInfo->authmgrCfg->adminMode !=  ENABLE)
  {
    return  SUCCESS;
  }
  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, intIfNum,
                       "%s:Intialize physical port's oper values with default -%d \n", __FUNCTION__,
                       intIfNum);

  authmgrPortInfoCleanup (intIfNum);
  authmgrPortCtrlModeSet (intIfNum, FD_AUTHMGR_PORT_MODE);
  authmgrIntfOperBuildDefault(intIfNum);

  return rc;
}
