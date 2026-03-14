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
#include "mab_db.h"
#include "mab_debug.h"
#include "mab_struct.h"

extern mabBlock_t *mabBlock;

/*************************************************************************
 * @purpose  function to process on expiry of server awhile timer 
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
RC_t  mabServerAwhileExpiryAction(mabLogicalPortInfo_t *logicalPortInfo)
{

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  /* Supp AWhile Timer has expired. */
  logicalPortInfo->protocol.authFail =  TRUE;
  mabUnAuthenticatedAction(logicalPortInfo);
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
RC_t mabTimerHandlerInfoGet(mabTimerType_t type, mabTimerMap_t *elem)
{
  uint32 i = 0;
  static mabTimerMap_t mabTimerHandlerTable[] =
  { 
    {MAB_SERVER_AWHILE, mabServerAwhileExpiryAction},
  };


  for (i = 0; i < (sizeof (mabTimerHandlerTable)/sizeof(mabTimerMap_t)); i++)
  {
    if (type == mabTimerHandlerTable[i].type)
    {
      *elem = mabTimerHandlerTable[i];
      return  SUCCESS;
    }
  }

  return  FAILURE;
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
void mabTimerExpiryHdlr( APP_TMR_CTRL_BLK_t timerCtrlBlk, void* ptrData)
{
  mabIssueCmd(mabTimeTick,  NULL,  NULLPTR);
}

/*************************************************************************
  * @purpose  Process the mab timer expiry event
  *
  * @param    param    @b{(input)}  Pointer to added mab node identifier 
  *
  * @returns  void
  *
  * @comments none
  *
  * @end
  *************************************************************************/
void mabTimerExpiryAction(void *param)
{
  mabTimerContext_t *pNode;
  mabTimerMap_t entry;
  mabLogicalPortInfo_t *logicalPortInfo =  NULLPTR;

  pNode = (mabTimerContext_t *)param;
  if (pNode ==  NULLPTR)
  {
     LOGF( LOG_SEVERITY_INFO,
        "mabTimerExpiryAction: Failed to retrieve handle \n");
    return;
  }

  logicalPortInfo = mabLogicalPortInfoGet(pNode->keyNum);

  if ( NULLPTR == logicalPortInfo)
  {
    return;
  }

  MAB_EVENT_TRACE(
      "MAB timer %s expired on logical port %d \r\n",
      mabTimerTypeStringGet(pNode->type), logicalPortInfo->key.keyNum);

  /* Delete the apptimer node */
  (void)appTimerDelete(mabBlock->mabTimerCB, logicalPortInfo->mabTimer.handle.timer);
  /* Null out the timer Details */
  logicalPortInfo->mabTimer.handle.timer =  NULL;

  /* pass the event accoring to the timer type */

  memset(&entry, 0, sizeof(mabTimerMap_t));
  if ( SUCCESS != mabTimerHandlerInfoGet(pNode->type, &entry))
  {
     LOGF( LOG_SEVERITY_WARNING,
        "mabTimerExpiryAction: Failed to retrieve information"
        "for timer type %s", mabTimerTypeStringGet(pNode->type));
    return;
  }

  if ( NULLPTR != entry.expiryFn)
  {
    entry.expiryFn(logicalPortInfo);
  }

  return;
}

/*************************************************************************
 * @purpose  Starts the specified timer
 *
 * @param    intIfNum      @b{(input)}  Interface for starting the timer
 * @param    timerType     @b{(input)}  Interface/Timer type
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments none
 *
 * @end
 *************************************************************************/
RC_t mabTimerStart(mabLogicalPortInfo_t *logicalPortInfo, mabTimerType_t timerType)
{
  uint32 physPort = 0, lPort = 0, type = 0, val = 0;
  mabTimerMap_t entry;

  memset(&entry, 0, sizeof(mabTimerMap_t));

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);
  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, logicalPortInfo->key.keyNum);

  if (logicalPortInfo->mabTimer.handle.timer !=  NULLPTR)
  {
    MAB_EVENT_TRACE( 
                      "timer %s already running for logical port %d \r\n",
                      mabTimerTypeStringGet (logicalPortInfo->mabTimer.cxt.type),
                      logicalPortInfo->key.keyNum);

    MAB_EVENT_TRACE( 
                      "not starting timer %s for logical port %d \r\n",
                      mabTimerTypeStringGet (timerType),
                      logicalPortInfo->key.keyNum);
    return  SUCCESS;
  }

  if ( SUCCESS != mabTimerHandlerInfoGet(timerType, &entry))
  {
     LOGF( LOG_SEVERITY_WARNING,
        "mabTimerExpiryAction: Failed to retrieve information"
        "for timer type %s", mabTimerTypeStringGet(timerType));
    return  FAILURE;
  }

  MAB_EVENT_TRACE( "mab Timer start:"
      "starting timer %s for logical port %d \r\n",
      mabTimerTypeStringGet(timerType), 
      logicalPortInfo->key.keyNum);

  /* Timer value should be multipled by 2 to match the retries in hostapd. */
  val = (2 * FD_MAB_PORT_SERVER_TIMEOUT);

  /* fill the timer context */
  logicalPortInfo->mabTimer.cxt.type = timerType;
  logicalPortInfo->mabTimer.cxt.keyNum = logicalPortInfo->key.keyNum;

 /* Start the timer */
  logicalPortInfo->mabTimer.handle.timer = appTimerAdd(mabBlock->mabTimerCB, mabTimerExpiryAction,
      (void *)&logicalPortInfo->mabTimer.cxt, val, mabTimerTypeStringGet(timerType));

  if(logicalPortInfo->mabTimer.handle.timer ==  NULLPTR)
  {
     LOGF( LOG_SEVERITY_WARNING,
        "mabTimerStart: Could not Start the %s timer."
        "intIf %d, clientType %s, logical IntIfNum %d.",
         mabTimerTypeStringGet(timerType), physPort, 
         mabNodeTypeStringGet(type), lPort);
    return  FAILURE;
  }

  return  SUCCESS;
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
RC_t  mabTimerDestroy( APP_TMR_CTRL_BLK_t timerCB,
    mabLogicalPortInfo_t *logicalPortInfo)
{
  uint32 physPort = 0, lPort = 0, type = 0;

  MAB_IF_NULLPTR_RETURN_LOG(logicalPortInfo);

  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, logicalPortInfo->key.keyNum);

  /* Delete the apptimer node */
  if ( NULL != logicalPortInfo->mabTimer.handle.timer)
  {
    (void)appTimerDelete(timerCB, logicalPortInfo->mabTimer.handle.timer);
    logicalPortInfo->mabTimer.handle.timer =  NULL;

    MAB_EVENT_TRACE( "deleted the timer type %s"
        "port %d type %s lport %d \r\n",
        mabTimerTypeStringGet(logicalPortInfo->mabTimer.cxt.type), 
        physPort, mabNodeTypeStringGet(type), lPort);
  }


  /* clear the context */
  memset(&logicalPortInfo->mabTimer, 0, sizeof(mabTimer_t));
  return  SUCCESS;
}

