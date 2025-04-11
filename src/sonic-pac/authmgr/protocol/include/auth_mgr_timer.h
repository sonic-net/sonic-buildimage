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

#ifndef INCLUDE_AUTHMGR_TIMER_H
#define INCLUDE_AUTHMGR_TIMER_H

/* USE C Declarations */
#ifdef __cplusplus
extern "C" {
#endif

#include "comm_mask.h"

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
void authmgrTimerExpiryHdlr( APP_TMR_CTRL_BLK_t timerCtrlBlk, void* ptrData);

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
RC_t authmgrTimerStart(authmgrLogicalPortInfo_t *logicalPortInfo, authmgrTimerType_t timerType);

/*************************************************************************
 * @purpose  Helper API to delete the specifed timer node
 *
 * @param    timer  @b{(input)}  Pointer to appTimer node
 * @param    handle @b{(input)}  Pointer to appTimer handle handle
 *
 * @returns   SUCCESS
 * @returns   FAILURE
 *
 * @comments none
 *
 * @end
 *************************************************************************/
RC_t  authmgrTimerDestroy( APP_TMR_CTRL_BLK_t timerCB,
    authmgrLogicalPortInfo_t *logicalPortInfo,
    authmgrTimerType_t timerType);

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
void authmgrTimerExpiryAction(void *param);


/*************************************************************************
  * @purpose get the map table entry for the timer type 
  *
  * @param    param    @b{(input)}  Pointer to added authmgr node identifier 
  *
  * @returns  void
  *
  * @comments none
  *
  * @end
  *************************************************************************/
RC_t authmgrTimerHandlerInfoGet(authmgrTimerType_t type, authmgrTimerMap_t *handler);

/*************************************************************************
 * @purpose  function to process on expiry of TX LP timer 
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
RC_t  authmgrTxLpTimerExpiryAction(authmgrLogicalPortInfo_t *logicalPortInfo);
/*************************************************************************
 * @purpose  function to process on expiry of reauth when timer
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
RC_t  authmgrReAuthWhenExpiryAction(authmgrLogicalPortInfo_t *logicalPortInfo);
/* USE C Declarations */
#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_AUTHMGR_TIMER_H */
