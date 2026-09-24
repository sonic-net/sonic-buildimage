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


#include <string.h>
#include "datatypes.h"
#include "commdefs.h"
#include "osapi.h"
#include "osapi_sem.h"
#include "buff_api.h"
#include "sll_api.h"
#include "apptimer_api.h"
#include "log.h"
#include "utils_api.h"
/*******************************************************************************
**                        Structure Definitions                               **
*******************************************************************************/

typedef struct appTmrCtrlBlk
{
  COMPONENT_IDS_t       compId;
  uint32                bufferPoolId;
  APP_TMR_GRAN_TYPE_t   type;
  void                     *semId;
  sll_t                 tmrList;
  sll_t                 wrapTmrList; /* SLL during expiry timer wrap */

  /* To store previous timetick when 
	 tick has come to process timers*/
  uint32                prevTime; 
  osapiTimerDescr_t        *pSysTimer;
  app_tmr_dispatcher_fn dispatchFn;
  void                     *pParam;
  struct appTmrCtrlBlk     *pSelf;   /* For sanity Sakes */
} appTmrCtrlBlk_t;


/*******************************************************************************
**                        Function Definitions                                **
*******************************************************************************/

/*********************************************************************
*
* @purpose  To compare the two keys given based on the timer expiry value.
*
* @param    data1  @b{(input)}The pointer to the first key.
* @param    data2  @b{(input)}The pointer to the second key.
* @param    keyLen @b{(input)}The length of the key to be compared.
*
* @returns  Less than 0, if node1 < node 2.
* @returns  Zero, if node1 == node2
* @returns  More than 0, if node1 > node2.
*
* @notes    Key Length is ignored for this comparison.
*
* @end
*
*********************************************************************/
static
 int32 appTimerCompare(void *data1,
                         void *data2,
                         uint32 keyLen)
{
  timerNode_t *pEntry1, *pEntry2;

  if(data1 ==  NULLPTR || data2 ==  NULLPTR)
    return 1;

  pEntry1 = (timerNode_t *)data1;
  pEntry2 = (timerNode_t *)data2;

  return (pEntry1->expiryTime - pEntry2->expiryTime);
}

/*********************************************************************
*
* @purpose  To destroy the node associated with the given node
*
* @param    node @b{(input)}The pointer to the node that is to be deleted.
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if failure
*
* @notes    There is currently no need for this.
*
* @end
*
*********************************************************************/
static
RC_t appTimerDestroy( sll_member_t *node)
{
  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  Function invoked by the system timer tick
*
* @param    param1 @b{(input)}First parameter from the system tick callback.
*                             This parameter is essentially the Application Timer instance's
*                             control block.
* @param    param2 @b{(input)}Second parameter from the system tick callback.
*                             This parameter is the user parameter registered with the library
*                    during instantiation of the timer module.
*
* @returns  None.
*
* @notes    I do not envisage that the control block would be freed very often.
*           So, acquiring the semaphore lock may not be needed while processing
*           this call. This would reduce the latency involved in processing the
*           system level timer.
*           In case the timer instance was actually deleted, then while really
*           processing the tick, we'll throw the call away.
*
* @end
*
*********************************************************************/
void appTimerTick( uint64 param1,  uint64 param2)
{
  
  appTmrCtrlBlk_t          *pCtrlBlk;
   app_tmr_dispatcher_fn pFunc;

  /* Some basic sanity checks */
  pCtrlBlk = (appTmrCtrlBlk_t *) ((unsigned long) param1);
  if(pCtrlBlk ==  NULLPTR)
    return;
  if(pCtrlBlk->pSelf!= pCtrlBlk)
    return;

  /* Retrieve the Dispatch callback function */
  pFunc = pCtrlBlk->dispatchFn;

  /* Make sure that the control block is still valid;
     indirectly verifying the authenticity of pFunc */
  if(pCtrlBlk->pSelf != pCtrlBlk)
    return;

  /* Invoke the dispatcher function */
  pFunc(( APP_TMR_CTRL_BLK_t)pCtrlBlk, (void *) ((unsigned long) param2));
}

/*********************************************************************
*
* @purpose  Initialize/Instantiate an Application Timer Module
*
* @param    compId       @b{(input)}Component ID of the owner
* @param    dispatchFn   @b{(input)}Function dispatcher that would off-load processing
*                                   to the application context
* @param    pParam       @b{(input)}Opaque user parameter that is to be returned in the tick
*                                   dispatcher for the application's use
* @param    timerType    @b{(input)}Granularity of the timer tick (applies to all timers
*                                   associated with this instance)
* @param    buffPoolID   @b{(input)}Buffer Pool ID to be associated with this instance
*
* @returns  appTmrCtrlBlk    Opaque Control Block for the timer module instance, if successful
* @returns   NULLPTR       If timer module instantiation failed
*
* @notes    Each application timer instance acts as a base timer tick. This is
*           equivalent to a system timer (eg. osapiTimer). So, it is highly
*           advisable that each application module/component instantiate
*           only one application timer using this routine to avoid depleting
*           (or overloading) the system timer. Timing within the application
*           is achieved by hooking the expiry function along with the timeout
*           to this basic timer instance by using the appTimerAdd() routine.
*
* @end
*
*********************************************************************/
 APP_TMR_CTRL_BLK_t appTimerInit( COMPONENT_IDS_t       compId,
                                    app_tmr_dispatcher_fn dispatchFn,
                                   void                     *pParam,
                                    APP_TMR_GRAN_TYPE_t   timerType,
                                   uint32                buffPoolID)
{
  appTmrCtrlBlk_t *pCtrlBlk =  NULLPTR;

  /* Some sanity checks */
  if(dispatchFn ==  NULLPTR)
    return ( APP_TMR_CTRL_BLK_t) NULLPTR;
  switch(timerType)
  {
    case  APP_TMR_1MSEC:
    case  APP_TMR_10MSEC:
    case  APP_TMR_100MSEC:
    case  APP_TMR_1SEC:
      break;
    default:
      return ( APP_TMR_CTRL_BLK_t) NULLPTR;
  }
  if(buffPoolID == 0)
    return ( APP_TMR_CTRL_BLK_t) NULLPTR;

  /* Allocate/Create the resources */
  pCtrlBlk = (appTmrCtrlBlk_t *)osapiMalloc(compId, sizeof(appTmrCtrlBlk_t));
  if(pCtrlBlk ==  NULLPTR)
    return ( APP_TMR_CTRL_BLK_t) NULLPTR;
  memset(pCtrlBlk, 0, sizeof(appTmrCtrlBlk_t));

	pCtrlBlk->bufferPoolId = buffPoolID;
  
  pCtrlBlk->semId = osapiSemaBCreate(OSAPI_SEM_Q_PRIORITY, OSAPI_SEM_FULL);
  if(pCtrlBlk->semId ==  NULLPTR)
  {
    osapiFree(compId, pCtrlBlk);
    return ( APP_TMR_CTRL_BLK_t) NULLPTR;
  }
  if(SLLCreate(compId,  SLL_ASCEND_ORDER, sizeof(uint32), appTimerCompare,
               appTimerDestroy, &(pCtrlBlk->tmrList)) !=  SUCCESS)
  {
    osapiSemaDelete(pCtrlBlk->semId);
    osapiFree(compId, pCtrlBlk);
    return ( APP_TMR_CTRL_BLK_t) NULLPTR;
  }
  if(SLLCreate(compId,  SLL_ASCEND_ORDER, sizeof(uint32), appTimerCompare,
               appTimerDestroy, &(pCtrlBlk->wrapTmrList)) !=  SUCCESS)
  {
    SLLDestroy(compId, &(pCtrlBlk->tmrList));
    osapiSemaDelete(pCtrlBlk->semId);
    osapiFree(compId, pCtrlBlk);
    return ( APP_TMR_CTRL_BLK_t) NULLPTR;
  }
  /* SLLCreate() creates an instance of the Singly Linked List that does not
     allow duplicates. Since for our application, we need support for multiple
     entries with the same expiry time, we need to explicitly enable nodes with
     duplicate key values in the list */
  SLLFlagsSet(&(pCtrlBlk->tmrList),  SLL_FLAG_ALLOW_DUPLICATES,  TRUE);
  SLLFlagsSet(&(pCtrlBlk->wrapTmrList),  SLL_FLAG_ALLOW_DUPLICATES,  TRUE);

  pCtrlBlk->compId     = compId;
  pCtrlBlk->type       = timerType;
  pCtrlBlk->dispatchFn = dispatchFn;
  pCtrlBlk->pParam     = pParam;
  pCtrlBlk->pSelf      = pCtrlBlk;
  pCtrlBlk->prevTime   = osapiTimeMillisecondsGet(); 

  /* Start the base system tick timer */
  osapiTimer64Add(appTimerTick,
                ( uint64)((unsigned long) pCtrlBlk),
                ( uint64)((unsigned long) pCtrlBlk->pParam),
                pCtrlBlk->type,
                &(pCtrlBlk->pSysTimer));

  return ( APP_TMR_CTRL_BLK_t)pCtrlBlk;
}

/*********************************************************************
*
* @purpose  De/Un-Initialize/Instantiate an Application Timer Module
*
* @param    timerCtrlBlk @b{(input)}Application Timer Control Block associated with the
*                                  instance that is to be destroyed
*
* @returns   SUCCESS  If operation was successful
* @returns   FAILURE  If operation failed
*
* @notes    The operation would clean-out any and all timers already in the
*           instance and also free-up any resources (memory,etc).
*
* @end
*
*********************************************************************/
RC_t appTimerDeInit( APP_TMR_CTRL_BLK_t timerCtrlBlk)
{
  appTmrCtrlBlk_t    *pCtrlBlk;
  void               *semId;
   COMPONENT_IDS_t compId;

  /* Basic sanity Checks */
  if(timerCtrlBlk ==  NULLPTR)
    return  FAILURE;
  pCtrlBlk = (appTmrCtrlBlk_t *)timerCtrlBlk;
  if(pCtrlBlk->pSelf != pCtrlBlk)
    return  FAILURE;

  /* Lock the timer module to prevent concurrent operations */
  semId = pCtrlBlk->semId;
  if(osapiSemaTake(semId,  WAIT_FOREVER) !=  SUCCESS)
  {
    return  FAILURE;
  }

  /* Stop the system timer ticks */
  osapiTimerFree(pCtrlBlk->pSysTimer);
  pCtrlBlk->pSysTimer =  NULLPTR;

  /* Release the resources */
  compId = pCtrlBlk->compId;
  SLLDestroy(compId, &(pCtrlBlk->tmrList));
  SLLDestroy(compId, &(pCtrlBlk->wrapTmrList));
  pCtrlBlk->type       = 0;
  pCtrlBlk->dispatchFn =  NULLPTR;
  pCtrlBlk->pSelf      =  NULLPTR;
  pCtrlBlk->prevTime   =  0;
  osapiFree(compId, pCtrlBlk);
  osapiSemaDelete(semId);

  /* All done */
  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  Add/Create a Timer Node
*
* @param    timerCtrlBlk  @b{(input)}Application Timer Control Block associated with the
*                                    instance on which the new timer is being added to
* @param    pFunc         @b{(input)}Pointer to the function that is to be invoked upon
*                                    the timer's expiry
* @param    pParam        @b{(input)}A parameter (opaque to the timer lib) that the application
*                                    prefers to be passed along to the expiry function
* @param    timeOut       @b{(input)}Timeout,  in Application Timer base ticks.
*
* @returns  tmrHandle     Opaque Application Timer handle, if successful.
*                         This handle, alongwith the Timer Module Control Block,
*                         uniquely identifies the specific timer within a timer
*                         module instantance and must be used to reference this
*                         timer for all other operations (eg.appTmrDelete,
*                         appTmrUpdate, etc).
* @return     NULLPTR    If operation failed
*
* @notes    The timeOut parameter MUST be in reference to the base application
*           timer instance's ticks. Please refer to the appTimerInit() call for
*           further reference.
*           As an example, if the appTimerInit() was invoked with  APP_TMR_100MSEC
*           for the timerType, then to get a timeout of 1 second, this function
*           must be called with a timeOut value of 10 (1 second = 10 * 10 msec).
*           This API is unprotected and it is the caller's (only this library) 
*           responsibility to acquire/release the timerCtrlBlk semaphore before 
*           invoking this.
*
* @end
*
*********************************************************************/
static  APP_TMR_HNDL_t
appTimerAddNode ( APP_TMR_CTRL_BLK_t timerCtrlBlk,
                  app_tmr_fn         pFunc,
                 void                  *pParam,
                 uint32             timeOut,
                  uchar8             *timerName,
                  uchar8             *fileName,
                 uint32             lineNum)
{
  appTmrCtrlBlk_t *pCtrlBlk;
  uint32       currTime;
  timerNode_t     *pTimerNode;

  pCtrlBlk = (appTmrCtrlBlk_t *)timerCtrlBlk;

  /* Retrieve the current timer tick */
  currTime = osapiTimeMillisecondsGet(); /* We must use the raw System Uptime for our
                                            time references to avoid problems due to
                                            user adjustments of the calender time */

  /* Get a new timer entry, populate it and put it into the timer list */
  if(bufferPoolAllocate(pCtrlBlk->bufferPoolId, ( uchar8 **)(&pTimerNode)) !=  SUCCESS)
  {
    return ( APP_TMR_HNDL_t) NULLPTR;
  }
  memset(pTimerNode, 0, sizeof(timerNode_t));
  pTimerNode->expiryFn   = pFunc;
  utilsFilenameStrip(( char8 **)&fileName);
#ifdef APPTIMER_DEBUG    
  osapiStrncpy(pTimerNode->name, timerName, APPTIMER_STR_LEN);
#endif  
  pTimerNode->expiryTime = currTime + (timeOut * pCtrlBlk->type);
  pTimerNode->pParam     = pParam;
  if ((pTimerNode->expiryTime < currTime) || (currTime < pCtrlBlk->prevTime))
  {
    /*  Add to wrap SLL */
    if(SLLAdd(&(pCtrlBlk->wrapTmrList), ( sll_member_t *)pTimerNode) !=  SUCCESS)
    {
      memset(pTimerNode, 0, sizeof(timerNode_t));
      bufferPoolFree(pCtrlBlk->bufferPoolId, ( uchar8 *)pTimerNode);
      return ( APP_TMR_HNDL_t) NULLPTR;
    }
  }
  else
  {
    if(SLLAdd(&(pCtrlBlk->tmrList), ( sll_member_t *)pTimerNode) !=  SUCCESS)
    {
      memset(pTimerNode, 0, sizeof(timerNode_t));
      bufferPoolFree(pCtrlBlk->bufferPoolId, ( uchar8 *)pTimerNode);
      return ( APP_TMR_HNDL_t) NULLPTR;
    }
  }
  return ( APP_TMR_HNDL_t)pTimerNode;
}

/*********************************************************************
*
* @purpose  Delete an Application Timer
*
* @param    timerCtrlBlk @b{(input)}Application Timer Control Block associated with the
*                                   instance from which the timer is being deleted from
*           timerHandle  @b{(input)}Timer Handle associated with the specific timer
*                                   being deleted
*
* @returns   SUCCESS    Timer deletion succesful
*            FAILURE    If timer deletion failed; either due to invalid parameters
*                         or that the timer had already popped
*
* @notes    None
*
* @end
*
*********************************************************************/
RC_t appTimerDelete( APP_TMR_CTRL_BLK_t timerCtrlBlk,
                        APP_TMR_HNDL_t     timerHandle)
{
  appTmrCtrlBlk_t *pCtrlBlk;
  timerNode_t     *pTimerNode;

  /* Basic sanity Checks */
  pCtrlBlk = (appTmrCtrlBlk_t *)timerCtrlBlk;
  pTimerNode = (timerNode_t *)timerHandle;
  if((pCtrlBlk ==  NULLPTR) || (pTimerNode ==  NULLPTR))
    return  FAILURE;
  if(pCtrlBlk->pSelf != pCtrlBlk)
    return  FAILURE;

  /* Lock the module */
  if(osapiSemaTake(pCtrlBlk->semId,  WAIT_FOREVER) !=  SUCCESS)
    return  FAILURE;

  /* Remove the entry from the timer list */
  if(SLLNodeRemove(&(pCtrlBlk->tmrList), ( sll_member_t *)pTimerNode) ==  NULL)
  {
    /*  Need to check for the same in wrap timer list as well*/
    if(SLLNodeRemove(&(pCtrlBlk->wrapTmrList), ( sll_member_t *)pTimerNode) ==  NULL)
    {
     osapiSemaGive(pCtrlBlk->semId);
     return  SUCCESS;
    }
  }

  /* Free-up the resources */
  memset(pTimerNode, 0, sizeof(timerNode_t));
  bufferPoolFree(pCtrlBlk->bufferPoolId, ( uchar8 *)pTimerNode);
  osapiSemaGive(pCtrlBlk->semId);
  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  Add/Create an Application Timer
*
* @param    timerCtrlBlk  @b{(input)}Application Timer Control Block associated with the
*                                    instance on which the new timer is being added to
* @param    pFunc         @b{(input)}Pointer to the function that is to be invoked upon
*                                    the timer's expiry
* @param    pParam        @b{(input)}A parameter (opaque to the timer lib) that the application
*                                    prefers to be passed along to the expiry function
* @param    timeOut       @b{(input)}Timeout,  in Application Timer base ticks.
*
* @returns  tmrHandle     Opaque Application Timer handle, if successful.
*                         This handle, alongwith the Timer Module Control Block,
*                         uniquely identifies the specific timer within a timer
*                         module instantance and must be used to reference this
*                         timer for all other operations (eg.appTmrDelete,
*                         appTmrUpdate, etc).
* @return     NULLPTR    If operation failed
*
* @notes    The timeOut parameter MUST be in reference to the base application
*           timer instance's ticks. Please refer to the appTimerInit() call for
*           further reference.
*           As an example, if the appTimerInit() was invoked with  APP_TMR_100MSEC
*           for the timerType, then to get a timeout of 1 second, this function
*           must be called with a timeOut value of 10 (1 second = 10 * 10 msec).
*
* @end
*
*********************************************************************/
 APP_TMR_HNDL_t appTimerAdd_track( APP_TMR_CTRL_BLK_t timerCtrlBlk,
                               app_tmr_fn         pFunc,
                              void                  *pParam,
                              uint32             timeOut,
                               char8              *timerName,
                               char8              *fileName,
                              uint32             lineNum)
{
  appTmrCtrlBlk_t *pCtrlBlk;
  timerNode_t     *pTimerNode;

  /* Basic sanity Checks */
  pCtrlBlk = (appTmrCtrlBlk_t *)timerCtrlBlk;
  if(pCtrlBlk ==  NULLPTR)
    return ( APP_TMR_HNDL_t) NULLPTR;
  if(pFunc ==  NULLPTR)
    return ( APP_TMR_HNDL_t) NULLPTR;
  if(pCtrlBlk->pSelf != pCtrlBlk)
    return ( APP_TMR_HNDL_t) NULLPTR;

  /* Lock the module */
  if(osapiSemaTake(pCtrlBlk->semId,  WAIT_FOREVER) !=  SUCCESS)
    return ( APP_TMR_HNDL_t) NULLPTR;

  pTimerNode = appTimerAddNode (timerCtrlBlk, pFunc, pParam, timeOut, timerName,
                    fileName, lineNum);

  osapiSemaGive(pCtrlBlk->semId);
  return ( APP_TMR_HNDL_t)pTimerNode;
}

/*********************************************************************
*
* @purpose  Update an Application Timer
*
* @param    timerCtrlBlk  @b{(input)}Application Timer Control Block associated with the
*                                    instance on which the timer is being updated
* @param    timerHandle  @b{(input)}Timer Handle associated with the specific timer
*                                   being updated
* @param    pFunc        @b{(input)}New Timer Expiry function. If  NULLPTR is given,
*                                   then the existing function pointer is left as is
* @param    pParam       @b{(input)}New parameter to be passed to the expiry function.
*                                  If  NULLPTR is given, then the existing parameter
*                                  value is left as is
* @param    timeOut      @b{(input)}New timeout value (in App Timer Ticks). The new timeOut
*                                  value is effective from the time this function is invoked
*                                  and not from the original addition time
*
* @returns   SUCCESS    Timer updation succesful
* @returns   FAILURE    If timer updation failed; either due to invalid parameters
*                         or that the timer had already popped
*
* @notes    Due to the nature of the definition of the pParam above, one
*           cannot update the parameter to a NULL pointer or to a 0 value (if
*           the pParam was actually a uint32 being typecasted). To achieve
*           this the caller is forced to do a appTimerDelete(), followed by a
*           appTimerAdd().
*           This has been enhanced to add a new node if the node to be
*           updated is not found.
*
* @end
*
*********************************************************************/
RC_t appTimerUpdate_track( APP_TMR_CTRL_BLK_t timerCtrlBlk,
                        APP_TMR_HNDL_t     *timerHandle,
                       void                  *pFunc,
                       void                  *pParam,
                       uint32             timeOut,
                        uchar8             *timerName,
                        uchar8             *fileName,
                       uint32             lineNum)
{
  appTmrCtrlBlk_t *pCtrlBlk;
  timerNode_t     *pTimerNode =  NULLPTR;
  uint32       currTime;

  /* Basic sanity Checks */
  pCtrlBlk = (appTmrCtrlBlk_t *)timerCtrlBlk;
  if(pCtrlBlk ==  NULLPTR)
    return  FAILURE;
  if(pCtrlBlk->pSelf != pCtrlBlk)
    return  FAILURE;
  if (timerHandle ==  NULLPTR)
    return  FAILURE;

  currTime = osapiTimeMillisecondsGet(); /* We must use the raw System Uptime for our
                                            time references to avoid problems due to
                                            user adjustments of the calender time */

  /* Lock the module */
  if(osapiSemaTake(pCtrlBlk->semId,  WAIT_FOREVER) !=  SUCCESS)
    return  FAILURE;


  if (*timerHandle ==  NULLPTR)
  {
    if ((*timerHandle = appTimerAddNode (timerCtrlBlk, pFunc, pParam, timeOut,timerName,
                        fileName, lineNum))
                      ==  NULLPTR)
    {
      osapiSemaGive(pCtrlBlk->semId);
      return  FAILURE;
    }
    osapiSemaGive(pCtrlBlk->semId);
    return  SUCCESS;
  }

  pTimerNode = (timerNode_t *)*timerHandle;
  /* Remove the entry from the timer list */
  if(SLLNodeRemove(&(pCtrlBlk->tmrList), ( sll_member_t *)pTimerNode) ==  NULL) 
  {
    if(SLLNodeRemove(&(pCtrlBlk->wrapTmrList), ( sll_member_t *)pTimerNode) ==  NULL)
    {
      if ((*timerHandle = appTimerAddNode (timerCtrlBlk, pFunc, pParam, timeOut,timerName,
                  fileName, lineNum))
                        ==  NULLPTR)
      {
        osapiSemaGive(pCtrlBlk->semId);
        return  FAILURE;
      }
      osapiSemaGive(pCtrlBlk->semId);
      return  SUCCESS;
    }
  }

  /* Update the timer entry */
  if(pFunc !=  NULLPTR)
    pTimerNode->expiryFn = pFunc;
  if(pParam !=  NULLPTR)
    pTimerNode->pParam = pParam;
  pTimerNode->expiryTime = currTime + (pCtrlBlk->type * timeOut);
  if ((pTimerNode->expiryTime < currTime)  || (currTime < pCtrlBlk->prevTime))
  {
    /*  Add to wrap SLL node*/
    if(SLLAdd(&(pCtrlBlk->wrapTmrList), ( sll_member_t *)pTimerNode) !=  SUCCESS)
    {
      memset(pTimerNode, 0, sizeof(timerNode_t));
      bufferPoolFree(pCtrlBlk->bufferPoolId, ( uchar8 *)pTimerNode);
      osapiSemaGive(pCtrlBlk->semId);
      return  FAILURE;
    }
  }
  else
  {
    /* Add the entry back to the timer list */
    if(SLLAdd(&(pCtrlBlk->tmrList), ( sll_member_t *)pTimerNode) !=  SUCCESS)
    {
      memset(pTimerNode, 0, sizeof(timerNode_t));
      bufferPoolFree(pCtrlBlk->bufferPoolId, ( uchar8 *)pTimerNode);
      osapiSemaGive(pCtrlBlk->semId);
      return  FAILURE;
    }
  }
  osapiSemaGive(pCtrlBlk->semId);
  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  Retrieve the time left for the given timer to expire
*
* @param    timerCtrlBlk @b{(input)}Application Timer Control Block associated with the
*                                   instance on which the timer is active
*           timerHandle  @b{(input)}Timer Handle associated with the specific timer
*           pTimeLeft    @b{(input)}Pointer to the output parameter that would be filled-in
*                                   with the time remaining (in multiples of the base
*                                   timer instance's granularity) for the timer to expire
*
* @returns   SUCCESS   Operation succesful
*            FAILURE   Operation failed; either due to invalid parameters
*                        or that the timer had already popped
*
* @end
*
*********************************************************************/
RC_t appTimerTimeLeftGet( APP_TMR_CTRL_BLK_t timerCtrlBlk,
                             APP_TMR_HNDL_t     timerHandle,
                            uint32             *pTimeLeft)
{
  appTmrCtrlBlk_t *pCtrlBlk;
  timerNode_t     *pTimerNode;
  timerNode_t     *pTmpNode =  NULLPTR;
  uint32       currTime;
   BOOL         isWrapTmrEntry =  FALSE;
  /* 
   * if timer does not exist or this api fails for whatever reason
   * just return 0 in timeleft
   */
  *pTimeLeft = 0;
  /* Basic sanity Checks */
  pCtrlBlk = (appTmrCtrlBlk_t *)timerCtrlBlk;
  pTimerNode = (timerNode_t *)timerHandle;
  if((pCtrlBlk ==  NULLPTR) || (pTimerNode ==  NULLPTR))
    return  FAILURE;
  if(pCtrlBlk->pSelf != pCtrlBlk)
    return  FAILURE;

  currTime = osapiTimeMillisecondsGet(); /* We must use the raw System Uptime for our
                                            time references to avoid problems due to
                                            user adjustments of the calender time */

  /* Lock the module */
  if(osapiSemaTake(pCtrlBlk->semId,  WAIT_FOREVER) !=  SUCCESS)
    return  FAILURE;

  /* Retrieve the entry from the timer list */
  pTmpNode = (timerNode_t *)SLLNodeFind(&(pCtrlBlk->tmrList), ( sll_member_t *)pTimerNode);
  if(pTmpNode ==  NULLPTR)
  {
    /*  find the same in wrap timer list as well*/  
    pTmpNode = (timerNode_t *)SLLNodeFind(&(pCtrlBlk->wrapTmrList), ( sll_member_t *)pTimerNode);
    if(pTmpNode ==  NULLPTR)
    {
      osapiSemaGive(pCtrlBlk->semId);
      return  FAILURE;
    }
    else
    {
      isWrapTmrEntry =  TRUE;
    }
  }


  /* Some times, if the timer tick events are still in the queue and not 
     processed yet means and during this instance if appTimerTimeLeftGet 
     is called from mgmt layer, there is a chance of pTmpNode->expiryTime being
     lesser than the current time and so may result in the negative value if 
     directly subtracted and so explicitly initialize it to 0 when expirytime is less than
     current time 
   */

  if(((pTmpNode->expiryTime < currTime) &&
      ( FALSE == isWrapTmrEntry)) ||
     ((pTmpNode->expiryTime > currTime) && ( TRUE == isWrapTmrEntry)))
  {
    *pTimeLeft =0; 
  }
  else
  {
    if (( TRUE == isWrapTmrEntry) &&
        (pTmpNode->expiryTime < currTime))
    {
      *pTimeLeft = (((0xFFFFFFFF) - currTime) + pTmpNode->expiryTime + 1) / pCtrlBlk->type;
    }
    else
    {
      /* Compute the time left based on the expiry time */
      *pTimeLeft = (pTmpNode->expiryTime - currTime) / pCtrlBlk->type;
    }
  }

  osapiSemaGive(pCtrlBlk->semId);
  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  Process an Application Timer tick
*
* @param    timerCtrlBlk @b{(input)}Application Timer Control Block associated with the
*                                   instance for which the timer tick is to be processed
*
* @returns  None
*
* @notes    This function MUST be invoked by the application (in its own context)
*           in response to a callback from the timer instance using dispatchFn
*           that was earlier registered with the library. The actual timer
*           expiry evaluations and the associated function invokations are
*           carried out in this function's caller's context.
*
* @end
*
*********************************************************************/
void appTimerProcess( APP_TMR_CTRL_BLK_t timerCtrlBlk)
{
  appTmrCtrlBlk_t *pCtrlBlk;
  timerNode_t     *pTimerNode =  NULLPTR;
  uint32       currTime;
   sll_t        tempSll;

  /* Basic sanity Checks */
  pCtrlBlk = (appTmrCtrlBlk_t *)timerCtrlBlk;
  if(pCtrlBlk ==  NULLPTR)
    return;
  if(pCtrlBlk->pSelf != pCtrlBlk)
    return;

  while( TRUE)
  {
    currTime = osapiTimeMillisecondsGet(); /* We must use the raw System Uptime for our
                                              time references to avoid problems due to
                                              user adjustments of the calender time */

    /* Lock the module */
    if(osapiSemaTake(pCtrlBlk->semId,  WAIT_FOREVER) !=  SUCCESS)
      break;

    /* Get the top of list entry */
    if(currTime >= pCtrlBlk->prevTime)
    {
      pTimerNode = (timerNode_t *)SLLAtStartPop(&(pCtrlBlk->tmrList));
      if(pTimerNode !=  NULLPTR)
      {
        if(pTimerNode->expiryTime > currTime)
        {
          SLLAtStartPush(&(pCtrlBlk->tmrList), ( sll_member_t *)pTimerNode);
          pCtrlBlk->prevTime = currTime;
          osapiSemaGive(pCtrlBlk->semId);
          break;
        }
        else
        {
           app_tmr_fn pFunc = pTimerNode->expiryFn;
          void          *pParam = pTimerNode->pParam;

          /* Free-up the timer entry as we are popping it */
          memset(pTimerNode, 0, sizeof(timerNode_t));
          bufferPoolFree(pCtrlBlk->bufferPoolId, ( uchar8 *)pTimerNode);
          osapiSemaGive(pCtrlBlk->semId);

          /* Invoke the expiry function */
          if(pFunc !=  NULLPTR)
          {
            pFunc(pParam);
          }
        }
      }
      else
      {
        /* No enties to process*/
        pCtrlBlk->prevTime = currTime;
        osapiSemaGive(pCtrlBlk->semId);
        break;
      }
    }
    else /* if(currTime < pCtrlBlk->prevTime) */
    {
      /* Time has wrapped; pop all the timers in the regular timer list */
      while ( TRUE)
      {
         app_tmr_fn      pFunc;
        void              *pParam;

        pTimerNode = (timerNode_t *)SLLAtStartPop(&(pCtrlBlk->tmrList));
        if(pTimerNode ==  NULLPTR)
        {
          break;
        }
        pFunc = pTimerNode->expiryFn;
        pParam = pTimerNode->pParam;

        /* Free-up the timer entry as we are popping it */
        memset(pTimerNode, 0, sizeof(timerNode_t));
        bufferPoolFree(pCtrlBlk->bufferPoolId, ( uchar8 *)pTimerNode);
        osapiSemaGive(pCtrlBlk->semId);

        /* Invoke the expiry function */
        if(pFunc !=  NULLPTR)
        {
          pFunc(pParam);
        }
        if(osapiSemaTake(pCtrlBlk->semId,  WAIT_FOREVER) !=  SUCCESS)
        {
          osapiTimer64Add(appTimerTick,
                        ( uint64) ((unsigned long) pCtrlBlk),
                        ( uint64) ((unsigned long) pCtrlBlk->pParam),
                        pCtrlBlk->type,
                        &(pCtrlBlk->pSysTimer));
          return;
        } 
      }

      /* Copy the wrap timer list to the regular timer list and reset the wrap timer list */
      memcpy(&tempSll, &(pCtrlBlk->tmrList), sizeof( sll_t));
      memcpy(&pCtrlBlk->tmrList, &pCtrlBlk->wrapTmrList, sizeof( sll_t));
      memcpy((&pCtrlBlk->wrapTmrList), &tempSll, sizeof( sll_t));
       LOGF( LOG_SEVERITY_DEBUG, "Regular timer list is cleared." 
              "Wrap timer list is copied to regular list. No. of elements copied %d", 
              SLLNumMembersGet(&pCtrlBlk->tmrList));
      /* Set the prevTime to the start of wrap around value */
      pCtrlBlk->prevTime = 0;

      /* Continue to process the regular timer list */
      osapiSemaGive(pCtrlBlk->semId);
    }
  };

  /* Restart the base system tick timer */
  osapiTimer64Add(appTimerTick,
                ( uint64) ((unsigned long) pCtrlBlk),
                ( uint64) ((unsigned long) pCtrlBlk->pParam), 
                pCtrlBlk->type,
                &(pCtrlBlk->pSysTimer));
}

/*********************************************************************
*
* @purpose  
*
* @param    timerCtrlBlk @b{(input)}Application Timer Control Block associated with the
*                                   instance for which the timer tick is to be processed
*
* @returns  None
*
* @notes    
*
* @end
*
*********************************************************************/
void appTimerDebugShow( APP_TMR_CTRL_BLK_t timerCtrlBlk)
{
#ifdef APPTIMER_DEBUG  

  appTmrCtrlBlk_t *pCtrlBlk;
  timerNode_t     *pTimerNode =  NULLPTR;
  uint32       currTime;
  
  /* Basic sanity Checks */
  pCtrlBlk = (appTmrCtrlBlk_t *)timerCtrlBlk;
  if(pCtrlBlk ==  NULLPTR)
    return;
  if(pCtrlBlk->pSelf != pCtrlBlk)
    return;
  sysapiPrintf("\n--------------------------------\n" );
  sysapiPrintf("TimerName  expiryTime expiryFn\n");
  sysapiPrintf("----------------------------------\n" );  

  /* Lock the module */
  if(osapiSemaTake(pCtrlBlk->semId,  WAIT_FOREVER) !=  SUCCESS)
    return;
  /* Retrieve the current timer tick */
  currTime = osapiTimeMillisecondsGet();
  for (pTimerNode = (timerNode_t *)SLLFirstGet(&(pCtrlBlk->tmrList));
       pTimerNode !=  NULLPTR;
       pTimerNode = (timerNode_t *)SLLNextGet(&(pCtrlBlk->tmrList), 
            ( sll_member_t *)pTimerNode))
  {
    sysapiPrintf("%-8s       %-4d      0x%x      \n",pTimerNode->name,
                ((pTimerNode->expiryTime - currTime)/pCtrlBlk->type), 
                pTimerNode->expiryFn);
  }
  /*  Check if etries in wrap timer list*/
  for (pTimerNode = (timerNode_t *)SLLFirstGet(&(pCtrlBlk->wrapTmrList));
       pTimerNode !=  NULLPTR;
       pTimerNode = (timerNode_t *)SLLNextGet(&(pCtrlBlk->wrapTmrList),
            ( sll_member_t *)pTimerNode))
  {
    sysapiPrintf("%-8s       %-4d      0x%x      \n",pTimerNode->name,
                ((pTimerNode->expiryTime - currTime)/pCtrlBlk->type),
                pTimerNode->expiryFn);
  }
  osapiSemaGive(pCtrlBlk->semId);
#endif
       
}
/******************************************************************************
* @purpose 
*
* @param   
*
* @returns   SUCCESS
* @returns   FAILURE
*
* @comments     
*     
* @end
******************************************************************************/
uint32 appTimerCbMemSizeGet(uint32   maxAppTimers)
{
 uint32 size;

  size = sizeof(appTmrCtrlBlk_t) + maxAppTimers * sizeof(timerNode_t);
  return size;
}
/*********************************************************************
*
* @purpose  Get pointer to the call back data
*
* @param    timerCtrlBlk @b{(input)}Application Timer Control Block associated with the
*                                   instance from which the timer is being deleted from
*           timerHandle  @b{(input)}Timer Handle associated with the specific timer
*                                   being deleted
*           cbDataPtr    @b{(input)}Pointer to return timer associated data

* @returns   SUCCESS    Call back data retrieve succesful
*            FAILURE    Due to invalid parameters
*
* @notes    None
*
* @end
*
*********************************************************************/
RC_t appTimerCbDataGet( APP_TMR_CTRL_BLK_t timerCtrlBlk,
                           APP_TMR_HNDL_t     timerHandle,
                          void                  **cbDataPtr)
{
  appTmrCtrlBlk_t *pCtrlBlk;
  timerNode_t     *pTimerNode;

  /* Basic sanity Checks */
  if((timerCtrlBlk ==  NULLPTR) || (timerHandle ==  NULLPTR) || 
     (cbDataPtr ==  NULLPTR))
    return  FAILURE;

  pCtrlBlk = (appTmrCtrlBlk_t *)timerCtrlBlk;
  pTimerNode = (timerNode_t *)timerHandle;

  if(pCtrlBlk->pSelf != pCtrlBlk)
    return  FAILURE;

  *cbDataPtr = pTimerNode->pParam;

  return  SUCCESS;
}

