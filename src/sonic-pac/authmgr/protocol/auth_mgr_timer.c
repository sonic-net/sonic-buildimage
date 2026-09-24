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
#include "auth_mgr_debug.h"
#include "auth_mgr_struct.h"

extern authmgrCB_t *authmgrCB;

/*************************************************************************
* @purpose  function to process on expiry of reauth when timer
*
* @param    logicalPortInfo @b{(input)}  Pointer to logicalPort Info
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments none
*
* @end
*************************************************************************/
RC_t authmgrReAuthWhenExpiryAction (authmgrLogicalPortInfo_t *
                                       logicalPortInfo)
{
  uint32 physPort = 0;
  RC_t rc = SUCCESS;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_PORT_GET (physPort, logicalPortInfo->key.keyNum);

  if (AUTHMGR_AUTHENTICATING == logicalPortInfo->protocol.authState) 
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_TIMER, physPort,
                         "\n%s:%d Client already authenticating for port %s\n",
                         __FUNCTION__, __LINE__, 
                         authmgrIntfIfNameGet(logicalPortInfo->key.keyNum));
    return  SUCCESS;
  }

  /* get the current authenticated method and
     notify to re-start authentication */

  if (NULLPTR ==
      authmgrCB->globalInfo->authmgrCallbacks[logicalPortInfo->client.
      authenticatedMethod].
      eventNotifyFn)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
        "Port %s logicalInterface %d failed to update %s to start authentication\n"
        "since the callback function is not registered method\n",
        authmgrIntfIfNameGet(logicalPortInfo->key.keyNum), logicalPortInfo->key.keyNum,
        authmgrMethodStringGet(logicalPortInfo->client.authenticatedMethod));
    rc = FAILURE;
  }

  if (SUCCESS == rc)
  {
    if ((authmgrCB->globalInfo->authmgrPortInfo[physPort].reAuthPeriodServer) &&
        (RADIUS_TERMINATION_ACTION_DEFAULT ==
         logicalPortInfo->client.terminationAction)) 
    {
      /* session timeout is derived from radius server and
         and received termination action as default */
      /* purge the client */
      authmgrClientInfoCleanup (logicalPortInfo);
      return SUCCESS;
    }

    /* Invoke Reauth with Authentication Method only if RADIUS
       dependency is not there. */
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_EVENTS, physPort,
        "Invoking Reauth for logicalInterface 0x%x with method %s.\n",
        logicalPortInfo->key.keyNum,
        authmgrMethodStringGet (logicalPortInfo->client.authenticatedMethod));
    rc =
      authmgrCB->globalInfo->authmgrCallbacks[logicalPortInfo->client.
      authenticatedMethod].eventNotifyFn
      (physPort, authmgrClientReAuthenticate,
       &logicalPortInfo->client.suppMacAddr);
  }

  logicalPortInfo->protocol.reauth =  TRUE;
  logicalPortInfo->protocol.authFail =  FALSE;
  logicalPortInfo->protocol.authTimeout =  FALSE;
  logicalPortInfo->protocol.authSuccess =  FALSE;
  authmgrGenerateEvents (logicalPortInfo->key.keyNum);
  return  SUCCESS;
}

/*************************************************************************
* @purpose  function to process on expiry of reauth when timer
*
* @param    logicalPortInfo @b{(input)}  Pointer to logicalPort Info
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments none
*
* @end
*************************************************************************/
RC_t authmgrQwhileExpiryAction (authmgrLogicalPortInfo_t * logicalPortInfo)
{
  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  logicalPortInfo->protocol.heldTimerExpired =  TRUE;

  /* QWhile Timer has expired. */
  authmgrGenerateEvents (logicalPortInfo->key.keyNum);

  return  SUCCESS;
}




/*************************************************************************
* @purpose  function to get function map entry for the given timer type
*
* @param    type  @b{(input)} timer type
* @param    elem @b{(input)}  Pointer to map entry
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments none
*
* @end
*************************************************************************/
RC_t authmgrTimerHandlerInfoGet (authmgrTimerType_t type,
                                    authmgrTimerMap_t * elem)
{
  uint32 i = 0;
  static authmgrTimerMap_t authmgrTimerHandlerTable[] = {
    {AUTHMGR_QWHILE, authmgrQwhileExpiryAction,  FALSE, authmgrQuietPeriodGet,
      NULLPTR},
    {AUTHMGR_REAUTH_WHEN, authmgrReAuthWhenExpiryAction,  TRUE,
     authmgrReAuthPeriodGet, authmgrLogicalPortReAuthPeriodGet},
  };

  for (i = 0;
       i < (sizeof (authmgrTimerHandlerTable) / sizeof (authmgrTimerMap_t));
       i++)
  {
    if (type == authmgrTimerHandlerTable[i].type)
    {
      *elem = authmgrTimerHandlerTable[i];
      return  SUCCESS;
    }
  }

  return  FAILURE;
}


/*********************************************************************
* @purpose   This function is used to return appropriate timer
*
* @param     logicalPortInfo    @b{(input)} client logical port
* @param     timerType         @b{(input)}  timer type
* @param     timerType         @b{(output)} timer
*
* @returns   None
*
* @notes     None
* @end
*********************************************************************/
static void authmgrGetTimer (authmgrLogicalPortInfo_t *logicalPortInfo,
                             authmgrTimerType_t timerType, 
                             authmgrTimer_t **pTmr)
{
  *pTmr = &logicalPortInfo->authmgrTimer;
  return;
}

/*********************************************************************
* @purpose   This function is used to send timer events
*
* @param     timerCtrlBlk    @b{(input)}   Timer Control Block
* @param     ptrData         @b{(input)}   Ptr to passed data
*
* @returns   None
*
* @notes     None
* @end
*********************************************************************/
void authmgrTimerExpiryHdlr ( APP_TMR_CTRL_BLK_t timerCtrlBlk, void *ptrData)
{
  authmgrIssueCmd (authmgrTimeTick,  NULL,  NULLPTR);
}

/*************************************************************************
* @purpose  Process the authmgr timer expiry event
*
* @param    param    @b{(input)}  Pointer to added authmgr node identifier
*
* @returns  void
*
* @comments none
*
* @end
*************************************************************************/
void authmgrTimerExpiryAction (void *param)
{
  authmgrTimerContext_t *pNode;
  authmgrTimerMap_t entry;
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  authmgrTimer_t *pTmr =  NULLPTR;
  uint32 physPort = 0, lPort = 0, type = 0;

  pNode = (authmgrTimerContext_t *) param;
  if (pNode ==  NULLPTR)
  {
     LOGF ( LOG_SEVERITY_INFO,
             "authmgrTimerExpiryAction: Failed to retrieve handle \n");
    return;
  }

  logicalPortInfo = authmgrLogicalPortInfoGet (pNode->keyNum);

  if ( NULLPTR == logicalPortInfo)
  {
    return;
  }

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, logicalPortInfo->key.keyNum);

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_TIMER, physPort, 
                       "timer %s expired for logical port %d \r\n",
                       authmgrTimerTypeStringGet (pNode->type),
                       logicalPortInfo->key.keyNum);

  memset(&authmgrCB->oldInfo, 0, sizeof(authmgrClientInfo_t));
  authmgrCB->oldInfo = logicalPortInfo->client;

  authmgrGetTimer(logicalPortInfo, pNode->type, &pTmr);

  /* Delete the apptimer node */
  (void) appTimerDelete (authmgrCB->globalInfo->authmgrTimerCB,
                         pTmr->handle.timer);
  /* Null out the timer Details */
  pTmr->handle.timer =  NULL;  
  
  /* pass the event accoring to the timer type */

  memset (&entry, 0, sizeof (authmgrTimerMap_t));
  if ( SUCCESS != authmgrTimerHandlerInfoGet (pNode->type, &entry))
  {
     LOGF ( LOG_SEVERITY_WARNING,
             "authmgrTimerExpiryAction: Failed to retrieve information"
             "for timer type %s", authmgrTimerTypeStringGet (pNode->type));
    return;
  }

  if ( NULLPTR != entry.expiryFn)
  {
    entry.expiryFn (logicalPortInfo);
  }
}

/*************************************************************************
* @purpose  Helper API to delete the specifed timer node
*
* @param    timer  @b{(input)}  Pointer to appTimer node
* @param    logicalPortInfo @b{(input)}  Pointer to logicalPort Info
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments none
*
* @end
*************************************************************************/
RC_t authmgrTimerDestroy ( APP_TMR_CTRL_BLK_t timerCB,
                             authmgrLogicalPortInfo_t * logicalPortInfo,
                             authmgrTimerType_t timerType)
{
  uint32 physPort = 0, lPort = 0, type = 0;
  authmgrTimer_t *pTmr =  NULLPTR; 
  
  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);
  
  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, logicalPortInfo->key.keyNum);
  
  authmgrGetTimer(logicalPortInfo, timerType, &pTmr);
  
  /* Delete the apptimer node */
  if (( NULL != pTmr->handle.timer) && (timerType == pTmr->cxt.type))
  {
    (void) appTimerDelete (timerCB, pTmr->handle.timer);
    pTmr->handle.timer =  NULLPTR;
    
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_TIMER, physPort,
                         "deleted the timer type %s"
                         "port %s type %s lport %d \r\n",
                         authmgrTimerTypeStringGet (pTmr->cxt.type),
                         authmgrIntfIfNameGet(physPort),
                         authmgrNodeTypeStringGet (type), lPort);
    /* clear the context */
    memset (pTmr, 0, sizeof (authmgrTimer_t));
  } 
  
  return  SUCCESS;
} 

/*************************************************************************
* @purpose  Starts the specified timer
*
* @param    logicalPortInfo @b{(input)}  Pointer to logicalPort Info
* @param    timerType     @b{(input)}  Interface/Timer type
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments none
*
* @end
*************************************************************************/
RC_t authmgrTimerStart (authmgrLogicalPortInfo_t * logicalPortInfo,
                           authmgrTimerType_t timerType)
{
  uint32 physPort = 0, lPort = 0, type = 0, val = 0;
  authmgrTimerMap_t entry;
  authmgrTimer_t *pTmr =  NULLPTR;

  memset (&entry, 0, sizeof (authmgrTimerMap_t));

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  authmgrGetTimer(logicalPortInfo, timerType, &pTmr);

  if (pTmr->handle.timer !=  NULLPTR)
  {
	AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_TIMER, physPort, 
		"timer %s already running for port %s logical port %d \r\n",
		authmgrTimerTypeStringGet (pTmr->cxt.type),
		authmgrIntfIfNameGet(logicalPortInfo->key.keyNum), logicalPortInfo->key.keyNum);

	AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_TIMER, physPort, 
		"not starting timer %s for port %s logical port %d \r\n",
		authmgrTimerTypeStringGet (timerType),
		authmgrIntfIfNameGet(logicalPortInfo->key.keyNum), logicalPortInfo->key.keyNum);
	return  SUCCESS;
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_TIMER, physPort, 
                       "starting timer %s for port %s logical port %d \r\n",
                       authmgrTimerTypeStringGet (timerType),
                       authmgrIntfIfNameGet(logicalPortInfo->key.keyNum), logicalPortInfo->key.keyNum);

  if ( SUCCESS != authmgrTimerHandlerInfoGet (timerType, &entry))
  {
     LOGF ( LOG_SEVERITY_WARNING,
             "Failed to retrieve information"
             "for timer type %s", authmgrTimerTypeStringGet (timerType));
    return  FAILURE;
  }

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, logicalPortInfo->key.keyNum);

  if (entry.serverConfigSupport)
  {
    if ( NULLPTR != entry.lportGetFn)
    {
      entry.lportGetFn (logicalPortInfo->key.keyNum, &val);
    }
  }
  else
  {
    if ( NULLPTR != entry.getFn)
    {
      /* get the timer value */
      entry.getFn (physPort, &val);
    }
  }

  /* fill the timer context */
  pTmr->cxt.type = timerType;
  pTmr->cxt.keyNum = logicalPortInfo->key.keyNum;
  
  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_TIMER, physPort, 
                       "timer %s for logical port %d val %d\r\n",
                       authmgrTimerTypeStringGet (timerType),
                       logicalPortInfo->key.keyNum, val);
  if (0 == val)
  {
    return  SUCCESS;
  }

  /* Start the timer */
  pTmr->handle.timer =
    appTimerAdd (authmgrCB->globalInfo->authmgrTimerCB,
                 authmgrTimerExpiryAction, (void *) &pTmr->cxt, val,
                 authmgrTimerTypeStringGet (timerType));

  if (pTmr->handle.timer ==  NULLPTR)
  {
     LOGF ( LOG_SEVERITY_WARNING,
             "authmgrTimerStart: Could not Start the %s timer."
             "intIf %s, clientType %s, logical IntIfNum %d.",
             authmgrTimerTypeStringGet (timerType), authmgrIntfIfNameGet(physPort),
             authmgrNodeTypeStringGet (type), lPort);
    return  FAILURE;
  }

  return  SUCCESS;
}
