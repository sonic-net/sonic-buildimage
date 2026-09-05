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
#include "auth_mgr_exports.h"
#include "auth_mgr_sm.h"
#include "auth_mgr_client.h"
#include "auth_mgr_struct.h"
#include "auth_mgr_timer.h"
#include "auth_mgr_db.h"

extern authmgrCB_t *authmgrCB;

RC_t authmgrStateMachine(uint32 authmgrEvent, authmgrLogicalPortInfo_t *logicalPortInfo);

static AUTHMGR_STATES_t authmgrStateTable[authmgrSmEvents][AUTHMGR_STATES] =
{
  /*Ev/St AUTHMGR_INITIALIZE        AUTHMGR_AUTHENTICATING     AUTHMGR_AUTHENTICATED    AUTHMGR_HELD             AUTHMGR_UNAUTHENTICATED */
  /*E1*/ {AUTHMGR_INITIALIZE,       AUTHMGR_INITIALIZE,        AUTHMGR_INITIALIZE,      AUTHMGR_INITIALIZE,      AUTHMGR_INITIALIZE},
  /*E2*/ {AUTHMGR_UNAUTHENTICATED,  AUTHMGR_STATES,            AUTHMGR_STATES,          AUTHMGR_STATES,          AUTHMGR_STATES},
  /*E3*/ {AUTHMGR_STATES,           AUTHMGR_AUTHENTICATED,     AUTHMGR_AUTHENTICATED,   AUTHMGR_STATES,          AUTHMGR_AUTHENTICATED},
  /*E4*/ {AUTHMGR_STATES,           AUTHMGR_HELD,              AUTHMGR_STATES,          AUTHMGR_STATES,          AUTHMGR_STATES},
  /*E5*/ {AUTHMGR_STATES,           AUTHMGR_AUTHENTICATING,    AUTHMGR_STATES,          AUTHMGR_STATES,          AUTHMGR_AUTHENTICATING},
  /*E6*/ {AUTHMGR_STATES,           AUTHMGR_STATES,            AUTHMGR_STATES,          AUTHMGR_UNAUTHENTICATED, AUTHMGR_STATES},
  /*E7*/ {AUTHMGR_STATES,           AUTHMGR_UNAUTHENTICATED,   AUTHMGR_UNAUTHENTICATED, AUTHMGR_STATES,          AUTHMGR_STATES},
  /*E8*/ {AUTHMGR_STATES,           AUTHMGR_STATES,            AUTHMGR_AUTHENTICATING,  AUTHMGR_STATES,          AUTHMGR_STATES},
  /*E9*/ {AUTHMGR_STATES,           AUTHMGR_STATES,            AUTHMGR_AUTHENTICATING,  AUTHMGR_STATES,          AUTHMGR_STATES},
  /*E10*/{AUTHMGR_STATES,           AUTHMGR_STATES,            AUTHMGR_AUTHENTICATING,  AUTHMGR_STATES,          AUTHMGR_STATES},
  /*E11*/{AUTHMGR_STATES,           AUTHMGR_STATES,            AUTHMGR_AUTHENTICATING,  AUTHMGR_STATES,          AUTHMGR_STATES},
  /*E12*/{AUTHMGR_STATES,           AUTHMGR_STATES,            AUTHMGR_AUTHENTICATING,  AUTHMGR_STATES,          AUTHMGR_STATES},
};



/*********************************************************************
* @purpose  This is the classifier which dispatches the received authmgr event
*            to a particular state machine
*
* @param   authmgrEvent  @b{(input)) event
* @param   intIfNum    @b{(input)) internal interface number
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrStateMachineClassifier (authmgrSmEvents_t authmgrEvent,
                                       uint32 lIntIfNum)
{
  RC_t rc =  SUCCESS;
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;

  logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);

  if ( NULLPTR == logicalPortInfo)
  {
    return  FAILURE;
  }

  if (authmgrEvent < authmgrSmEvents)
  {
    rc = authmgrStateMachine (authmgrEvent, logicalPortInfo);
  }
  else
  {
    rc =  FAILURE;
  }

  return rc;
}

/***************************************************************************/
/*******************************APM State Machine Routines******************/
/***************************************************************************/
/*********************************************************************
* @purpose  Actions to be performed in the AUTHMGR state INITIALIZE
*
* @param   pNode  @b{(input))  Logical Port Info node
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrInitializeAction (authmgrLogicalPortInfo_t * pNode)
{
  uint32 physPort = 0;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (pNode);
  AUTHMGR_PORT_GET (physPort, pNode->key.keyNum);

  /* Clear all the data */
  memset (&pNode->client.executedMethod[0], 0,
          sizeof (pNode->client.executedMethod));

  pNode->protocol.authState = AUTHMGR_INITIALIZE;

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                       "logicalInterface %d moved to state %s\n",
                       pNode->key.keyNum,
                       authmgrAuthStateStringGet (pNode->protocol.authState));
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Actions to be performed in the APM state HELD
*
* @param   logicalPortInfo  @b{(input))  Logical Port Info node
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrHeldAction (authmgrLogicalPortInfo_t * logicalPortInfo)
{
  uint32 physPort = 0;
  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_PORT_GET (physPort, logicalPortInfo->key.keyNum);

  authmgrAuthenticatedClientCleanupAction(logicalPortInfo);

  if (( NULL != logicalPortInfo->authmgrTimer.handle.timer))
  {
    if (AUTHMGR_REAUTH_WHEN == logicalPortInfo->authmgrTimer.cxt.type)
    {
      authmgrTimerDestroy (authmgrCB->globalInfo->authmgrTimerCB,
                           logicalPortInfo, AUTHMGR_REAUTH_WHEN);
    }
  }

  /* start timer with held period */
  authmgrTimerStart (logicalPortInfo, AUTHMGR_QWHILE);
  logicalPortInfo->protocol.authState = AUTHMGR_HELD;

    if ((logicalPortInfo->protocol.authFail) ||
        (logicalPortInfo->protocol.authTimeout))
    {
      authmgrTxCannedFail (logicalPortInfo->key.keyNum, AUTHMGR_LOGICAL_PORT);

       LOGF ( LOG_SEVERITY_NOTICE,
               "Client %s authentication failed on port (%s).", 
               AUTHMGR_PRINT_MAC_ADDR(logicalPortInfo->client.suppMacAddr.addr),
               authmgrIntfIfNameGet(physPort));
    }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                       "logicalInterface %d moved to state %s\n",
                       logicalPortInfo->key.keyNum,
                       authmgrAuthStateStringGet (logicalPortInfo->protocol.
                                                  authState));
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Actions to be performed in the APM state Unauthenticated
*
* @param   logicalPortInfo  @b{(input))  Logical Port Info node
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrUnauthenticatedAction (authmgrLogicalPortInfo_t *
                                      logicalPortInfo)
{
  uint32 physPort = 0;

  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_PORT_GET (physPort, logicalPortInfo->key.keyNum);

  if ((logicalPortInfo->protocol.reauth) &&
      (logicalPortInfo->protocol.authFail))
  {
    logicalPortInfo->protocol.reauth =  FALSE;
  }

  if ((!(( TRUE == logicalPortInfo->protocol.reauth) &&
        ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
         logicalPortInfo->client.logicalPortStatus))) ||
      ( TRUE == logicalPortInfo->protocol.heldTimerExpired))
  {
    if ( AUTHMGR_PORT_STATUS_AUTHORIZED ==
        logicalPortInfo->client.logicalPortStatus)
    {
        /* delete the authenticated client */
        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_PORT_STATUS, physPort,
                            "%s:Initiating Client logicalPort num-%d cleanup. \n",

                            __FUNCTION__, logicalPortInfo->key.keyNum);
        if ( SUCCESS != authmgrClientHwInfoCleanup (logicalPortInfo))
        {
           AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                               "%s:Unable to cleanup client hw info logicalPort num-%d\n",
                               __FUNCTION__, logicalPortInfo->key.keyNum);
        }
    }

      authmgrClientStatusSet (logicalPortInfo,
                               AUTHMGR_PORT_STATUS_UNAUTHORIZED);
  }


  logicalPortInfo->protocol.authState = AUTHMGR_UNAUTHENTICATED;

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                       "logicalInterface %d moved to state %s\n",
                       logicalPortInfo->key.keyNum,
                       authmgrAuthStateStringGet (logicalPortInfo->protocol.
                                                  authState));

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Actions to be performed in the APM state AUTHENTICATING
*
* @param   logicalPortInfo  @b{(input))  Logical Port Info node
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrAuthenticatingAction (authmgrLogicalPortInfo_t * logicalPortInfo)
{
  RC_t rc;
  uint32 physPort = 0;
  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_PORT_GET (physPort, logicalPortInfo->key.keyNum);

  logicalPortInfo->protocol.authState = AUTHMGR_AUTHENTICATING;
  /* Send the notification to start authentication
     to the enabled methods  */

  logicalPortInfo->protocol.authFail =  FALSE;
  logicalPortInfo->protocol.authTimeout =  FALSE;
  logicalPortInfo->protocol.authSuccess =  FALSE;

  if (logicalPortInfo->protocol.authenticatedRcvdStartAuth)
  {
    logicalPortInfo->protocol.authenticatedRcvdStartAuth  =  FALSE;
    logicalPortInfo->client.currentMethod = logicalPortInfo->client.authenticatedMethod;
    return  SUCCESS;
  }

  rc = authmgrAuthenticationTrigger (logicalPortInfo);

  if ( SUCCESS != rc)
  {
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FAILURE, physPort,
                         "logicalInterface %d failed to update %s to start authentication\n",
                         logicalPortInfo->key.keyNum,
                         authmgrMethodStringGet (logicalPortInfo->client.
                                                 currentMethod));

    logicalPortInfo->protocol.authFail =  TRUE;
    rc = authmgrGenerateEvents (logicalPortInfo->key.keyNum);
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                       "logicalInterface %d moved to state %s\n",
                       logicalPortInfo->key.keyNum,
                       authmgrAuthStateStringGet (logicalPortInfo->protocol.
                                                  authState));

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Actions to be performed in the APM state AUTHENTICATED
*
* @param   logicalPortInfo  @b{(input))  Logical Port Info node
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrAuthenticatedAction (authmgrLogicalPortInfo_t * logicalPortInfo)
{
  uint32 physPort = 0;
  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_PORT_GET (physPort, logicalPortInfo->key.keyNum);

  authmgrClientStatusSet (logicalPortInfo,  AUTHMGR_PORT_STATUS_AUTHORIZED);
  authmgrTxCannedSuccess (logicalPortInfo->key.keyNum, AUTHMGR_LOGICAL_PORT);

  logicalPortInfo->protocol.authState = AUTHMGR_AUTHENTICATED;
  logicalPortInfo->protocol.reauth =  FALSE;
  logicalPortInfo->protocol.authFail =  FALSE;
  logicalPortInfo->protocol.authTimeout =  FALSE;
  logicalPortInfo->protocol.authSuccess =  FALSE;

  /* check if re-auth is enabled. If yes
     start the reauth timer */
  if (authmgrCB->globalInfo->authmgrPortInfo[physPort].reAuthEnabled) 
  {
    authmgrTimerStart (logicalPortInfo, AUTHMGR_REAUTH_WHEN);
  }

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                       "logicalInterface %d moved to state %s\n",
                       logicalPortInfo->key.keyNum,
                       authmgrAuthStateStringGet (logicalPortInfo->protocol.
                                                  authState));

   LOGF ( LOG_SEVERITY_NOTICE,
           "Client %s authorized on port (%s) with VLAN type %s.", 
           AUTHMGR_PRINT_MAC_ADDR(logicalPortInfo->client.suppMacAddr.addr),
           authmgrIntfIfNameGet(physPort), authmgrVlanTypeStringGet (logicalPortInfo->client.vlanType));



  if (AUTHMGR_VLAN_RADIUS == logicalPortInfo->client.vlanType)
  {
    logicalPortInfo->client.vlanTypePortCfg = (RADIUS_REQUIRED_TUNNEL_ATTRIBUTES_SPECIFIED == authmgrCB->attrInfo.vlanAttrFlags) ?  FALSE :  TRUE;
  }
    
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Events that auth mgr state machine needs to generate and propagate
*
* @param   lIntIfNum  @b{(input))  Logical Port number 
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrGenerateEvents (uint32 lIntIfNum)
{
  uint32 physPort = 0;
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  RC_t rc =  SUCCESS;
   uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1];

  logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);
  AUTHMGR_IF_NULLPTR_RETURN_LOG (logicalPortInfo);

  AUTHMGR_PORT_GET (physPort, logicalPortInfo->key.keyNum);
  if ((AUTHMGR_INITIALIZE == logicalPortInfo->protocol.authState) &&
      (authmgrCB->globalInfo->authmgrPortInfo[physPort].portEnabled))
  {
    /* E2 */
    AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                         "generating event %s for logicalInterface %d \n",
                         authmgrSmEventStringGet (authmgrStartAuthenticate),
                         logicalPortInfo->key.keyNum);
    (void) authmgrStateMachineClassifier (authmgrStartAuthenticate, lIntIfNum);
  }

  if (AUTHMGR_UNAUTHENTICATED == logicalPortInfo->protocol.authState)
  {
    if (logicalPortInfo->protocol.authSuccess)
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                           "Auth success due to special cases such as guest/unauth access"
                           "for logicalInterface %d. generating event %s to Allowing access on the port %s \n",
                           logicalPortInfo->key.keyNum,
                           authmgrSmEventStringGet (authmgrAuthSuccess),
                            authmgrIntfIfNameGet(physPort));
      /* open the access for this port */
      (void) authmgrStateMachineClassifier (authmgrAuthSuccess, lIntIfNum);
      return  SUCCESS;
    }
    if ( TRUE == logicalPortInfo->protocol.heldTimerExpired)
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                           "Held timer expired for logicalInterface %d \n",
                           logicalPortInfo->key.keyNum);

      if ( SUCCESS == authmgrClientDisconnectAction (logicalPortInfo))
      {
        return  SUCCESS;
      }

      /* reset the variables for next cycle
         of authentication */
      logicalPortInfo->client.currentMethod =  AUTHMGR_METHOD_NONE;
    }

    if (logicalPortInfo->protocol.authenticate)
    {
      /* get the first method */
      rc =
        authmgrEnabledMethodNextGet (physPort,
                                     &logicalPortInfo->client.currentMethod);
      if ( SUCCESS != rc)
      {
        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                             "Next method %s not available for logicalInterface %d \n",
                             authmgrMethodStringGet (logicalPortInfo->client.
                                                     currentMethod),
                             logicalPortInfo->key.keyNum);
        return  SUCCESS;
      }

      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                           "Next method %s available for logicalInterface %d \n",
                           authmgrMethodStringGet (logicalPortInfo->client.
                                                   currentMethod),
                           logicalPortInfo->key.keyNum);
      (void) authmgrStateMachineClassifier (authmgrNotAuthSuccessNextMethod,
                                            lIntIfNum);
      return  SUCCESS;
    }
  }

  if (AUTHMGR_AUTHENTICATING == logicalPortInfo->protocol.authState)
  {
    /* if the client has failed during re-auth,
       purge the existing client */

    if ( TRUE == logicalPortInfo->protocol.reauth)
    {
      if (( FALSE == logicalPortInfo->protocol.authFail) &&
          ( FALSE == logicalPortInfo->protocol.authTimeout) &&
          ( FALSE == logicalPortInfo->protocol.authSuccess))
      {
        /* Re-auth is in progress
           nothing to be done */
        return  SUCCESS;
      }

      if (( TRUE == logicalPortInfo->protocol.authFail) &&
          ( AUTHMGR_METHOD_NONE != logicalPortInfo->client.authenticatedMethod))
      {
        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
            "%s: %d: generating event authmgrNotAuthSuccessNoNextMethod forlogicalInterface %d \n",
            __func__, __LINE__, logicalPortInfo->key.keyNum);
        (void)
          authmgrStateMachineClassifier (authmgrNotAuthSuccessNoNextMethod,
              lIntIfNum);
        return  SUCCESS;
      }
    }

    if ((logicalPortInfo->protocol.unAuthenticate))
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                           "un authenticating client on logicalInterface %d \n",
                           logicalPortInfo->key.keyNum);
      (void) authmgrStateMachineClassifier (authmgrStopAuthenticate, lIntIfNum);
      return  SUCCESS;
    }

    if (logicalPortInfo->protocol.authSuccess)
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                           "Auth Success for method %s for logicalInterface %d \n",
                           authmgrMethodStringGet (logicalPortInfo->client.
                                                   currentMethod),
                           logicalPortInfo->key.keyNum);
      (void) authmgrStateMachineClassifier (authmgrAuthSuccess, lIntIfNum);
      return  SUCCESS;
    }
    else
    {
      rc =  SUCCESS;
      if ((logicalPortInfo->protocol.authFail) ||
          (logicalPortInfo->protocol.authTimeout))
      {
        rc =
          authmgrEnabledMethodNextGet (physPort,
              &logicalPortInfo->client.
              currentMethod);
      }

      if ( SUCCESS == rc)
      {
        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                             "Next method %s available for logicalInterface %d \n",
                             authmgrMethodStringGet (logicalPortInfo->client.
                                                     currentMethod),
                             logicalPortInfo->key.keyNum);
        (void) authmgrStateMachineClassifier (authmgrNotAuthSuccessNextMethod,
                                              lIntIfNum);
        return  SUCCESS;
      }
      else
      {
        AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                             "No Next method available for logicalInterface %d \n",
                             logicalPortInfo->key.keyNum);
        (void) authmgrStateMachineClassifier (authmgrNotAuthSuccessNoNextMethod,
                                              lIntIfNum);
        return  SUCCESS;
      }
    }
  }


  if (AUTHMGR_HELD == logicalPortInfo->protocol.authState)
  {
    if ( TRUE == logicalPortInfo->protocol.heldTimerExpired)
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
          "generating event %s for logicalInterface %d \n",
          authmgrSmEventStringGet (authmgrHeldTimerEqualsZero),
          logicalPortInfo->key.keyNum);
      (void) authmgrStateMachineClassifier (authmgrHeldTimerEqualsZero,
          lIntIfNum);
      return  SUCCESS;
    }
  }

  if (AUTHMGR_AUTHENTICATED == logicalPortInfo->protocol.authState)
  {
    if (logicalPortInfo->protocol.authSuccess)
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                           "Reauth and received success. Moving to authenticated state for client on logicalInterface %d \n",
                           logicalPortInfo->key.keyNum);
      (void) authmgrStateMachineClassifier (authmgrAuthSuccess, lIntIfNum);
      return  SUCCESS;
    }
    if (logicalPortInfo->protocol.unAuthenticate)
    {
      nimGetIntfName(physPort,  ALIASNAME, ifName); 
       LOGF ( LOG_SEVERITY_DEBUG,
               "Unauthenticating client on logicalInterface (%d) port (%s).", 
                logicalPortInfo->key.keyNum, ifName);
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                           "Unauthenticating client on logicalInterface %d \n",
                           logicalPortInfo->key.keyNum);
      (void) authmgrStateMachineClassifier (authmgrStopAuthenticate, lIntIfNum);
      return  SUCCESS;
    }

    if ( TRUE == logicalPortInfo->protocol.authenticatedRcvdStartAuth)
    {
      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                           "Received authenticate action on logicalInterface %d."
                             "Setting reauthenticate true for client \n",
                           logicalPortInfo->key.keyNum);
      logicalPortInfo->protocol.authenticatedRcvdStartAuth =  FALSE;
      logicalPortInfo->protocol.reauth =  TRUE;
    }

    if ( TRUE == logicalPortInfo->protocol.reauth)
    {
      nimGetIntfName(physPort,  ALIASNAME, ifName);

       LOGF ( LOG_SEVERITY_NOTICE,
             "Reauthentication triggered for client %s on port %s.",
             AUTHMGR_PRINT_MAC_ADDR(logicalPortInfo->client.suppMacAddr.addr),
             ifName);  


      AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                           "generating event %s for logicalInterface %d \n",
                           authmgrSmEventStringGet (authmgrReauthenticate),
                           logicalPortInfo->key.keyNum);
      (void) authmgrStateMachineClassifier (authmgrReauthenticate, lIntIfNum);
      return  SUCCESS;
    }

  }
  return  SUCCESS;
}

/*********************************************************************
* @purpose state machine function to trigger authentication
*
* @param lIntIfNum logical interface number
* @return  SUCCESS
* @return  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrAuthenticationInitiate (uint32 lIntIfNum)
{
  authmgrLogicalPortInfo_t *logicalPortInfo =  NULLPTR;
  uint32 physPort = 0;

  logicalPortInfo = authmgrLogicalPortInfoGet (lIntIfNum);

  if ( NULLPTR == logicalPortInfo)
  {
    return  FAILURE;
  }

  AUTHMGR_PORT_GET (physPort, logicalPortInfo->key.keyNum);

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                       "%s : received event to start authentication for logicalInterface %d \n",
                       __func__, logicalPortInfo->key.keyNum);

  logicalPortInfo->protocol.authenticate =  TRUE;

  authmgrGenerateEvents (lIntIfNum);
  return  SUCCESS;
}

/*********************************************************************
* @purpose  This routine calculates the next state for the APM state machine
*           and executes the action for that next state
*
* @param   authmgrEvent  @b{(input)) event
* @param   logicalPortInfo    @b{(input)) Logical port structure
*
* @returns  SUCCESS
* @returns  FAILURE
*
* @comments
*
* @end
*********************************************************************/
RC_t authmgrStateMachine (uint32 authmgrEvent,
                             authmgrLogicalPortInfo_t * logicalPortInfo)
{
  AUTHMGR_STATES_t nextState;
  uint32 normalizedEvent;
  RC_t rc =  FAILURE;
  uint32 physPort = 0;

  if (logicalPortInfo ==  NULLPTR)
  {
    return  FAILURE;
  }

  AUTHMGR_PORT_GET (physPort, logicalPortInfo->key.keyNum);
  normalizedEvent = authmgrEvent;

  nextState =
    authmgrStateTable[normalizedEvent][logicalPortInfo->protocol.authState];

  AUTHMGR_EVENT_TRACE (AUTHMGR_TRACE_FSM_EVENTS, physPort,
                       "AuthMgr Machine for logical port - %d moving from %s to %s for event %s \n",
                       logicalPortInfo->key.keyNum,
                       authmgrAuthStateStringGet (logicalPortInfo->protocol.
                                                  authState),
                       authmgrAuthStateStringGet (nextState),
                       authmgrSmEventStringGet (normalizedEvent));
  switch (nextState)
  {
  case AUTHMGR_INITIALIZE:
    rc = authmgrInitializeAction (logicalPortInfo);
    rc = authmgrGenerateEvents (logicalPortInfo->key.keyNum);
    break;
  case AUTHMGR_AUTHENTICATING:
    rc = authmgrAuthenticatingAction (logicalPortInfo);
    /*   rc = authmgrGenerateEvents(logicalPortInfo->key.keyNum); */
    break;
  case AUTHMGR_AUTHENTICATED:
    rc = authmgrAuthenticatedAction (logicalPortInfo);
    /* No need to generate events here */
    break;
  case AUTHMGR_HELD:
    rc = authmgrHeldAction (logicalPortInfo);
    /* No need to generate events here */
    break;
  case AUTHMGR_UNAUTHENTICATED:
    rc = authmgrUnauthenticatedAction (logicalPortInfo);
    rc = authmgrGenerateEvents (logicalPortInfo->key.keyNum);
    break;
  default:
    break;
  }

  return rc;
}
