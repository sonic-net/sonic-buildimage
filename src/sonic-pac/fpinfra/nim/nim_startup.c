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

#include "datatypes.h"
#include "commdefs.h"
#include "cnfgr_api.h"
#include "log.h"
#include "osapi.h"
#include "osapi_sem.h"
#include "avl_api.h"
#include "tree_api.h"
#include "nim_util.h"
#include "nimapi.h"
#include "nim_startup.h"

static avlTree_t              nimStartUpTree  = { 0 };
static void                   *nimStartUpSema =  NULL;
static void                   *nimStartUpCbSema =  NULL;
static void                   *nimStartUpEvSema =  NULL;
static  COMPONENT_IDS_t     nimStartUpCompId =  FIRST_COMPONENT_ID;


/* Macros for protecting the AVL tree during operations */
#define NIM_STARTUP_CRIT_SEC_ENTER()  \
{   \
    osapiSemaTake(nimStartUpSema, WAIT_FOREVER);  \
}

#define NIM_STARTUP_CRIT_SEC_EXIT()  \
{   \
    osapiSemaGive(nimStartUpSema);  \
}

/* Maximum length of time that NIM waits for all applications to transition to Configurator-Execute Phase.
*/
#define MAX_NIM_DELAY_SEC  10

/*********************************************************************
*
* @purpose  Compare two Startup AVL keys and determine Greater, Less, or Equal
*
* @param    a    a void pointer to an AVL key comprised of a Startup structure
* @param    b    a void pointer to an AVL key comprised of a Startup structure
* @param    len  the length of the key in bytes (unused)
*
* @returns  1 if a > b
* @returns  -1 if a < b
* @returns  0 if a == b
*
* @notes    This function is used as a custom comparator in the
*           nimStartUpTreeData AVL tree. The 'len'
*           parameter is unused because the length of Startup data is
*           known, however, it is required for compatibility with
*           the comparator type definition.
*
* @end
*********************************************************************/
static int nimStartupCompare(const void *a, const void *b, size_t len)
{
  nimStartUpTreeData_t *sua = (nimStartUpTreeData_t *)a;
  nimStartUpTreeData_t *sub = (nimStartUpTreeData_t *)b;

  /* Sort priority highest -> lowest value */
  if (sua->priority > sub->priority)
  {
    return -1;
  }
  if (sua->priority < sub->priority)
  {
    return 1;
  }
  if (sua->componentId < sub->componentId)
  {
    return -1;
  }
  if (sua->componentId > sub->componentId)
  {
    return 1;
  }
  return 0;
}

/*********************************************************************
* @purpose  Allocate the memory for the ifIndex AVL tree
*
* @param    void
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimStartUpPhaseOneInit()
{
  RC_t   rc =  SUCCESS;

  do
  {

    if (avlAllocAndCreateAvlTree(&nimStartUpTree,
                                  NIM_COMPONENT_ID,
                                  LAST_COMPONENT_ID,
                                 sizeof(nimStartUpTreeData_t),
                                 0x10, nimStartupCompare,
                                 sizeof(uint32)*2) !=  SUCCESS)
    {
      NIM_LOG_ERROR("NIM: Unable to allocate resources\n");
      break; /* goto while */
    }

    nimStartUpSema = osapiSemaMCreate(OSAPI_SEM_Q_PRIORITY);
    nimStartUpCbSema = osapiSemaBCreate(OSAPI_SEM_Q_PRIORITY,
                                        OSAPI_SEM_EMPTY);
    nimStartUpEvSema = osapiSemaBCreate(OSAPI_SEM_Q_PRIORITY,
                                        OSAPI_SEM_EMPTY);

    if (nimStartUpSema ==  NULLPTR)
    {
      NIM_LOG_ERROR("NIM: unable to create the ifIndex Sema\n");
    }

  } while ( 0 );

  return rc;
}

/*********************************************************************
* @purpose  Create an StartUp
*
* @param    componentId   @b{(input)}   Component ID of startup function
* @param    priority      @b{(input)}   priority to execute startup function
* @param    startupFcn    @b{(input)}   Function pointer to startup routine
*
* @notes
*
* @end
*
*********************************************************************/
void nimStartUpCreate( COMPONENT_IDS_t componentId,
                      uint32 priority,
                      StartupNotifyFcn startupFcn)
{
  nimStartUpTreeData_t data;
  nimStartUpTreeData_t *pData;

  data.componentId = componentId;
  data.priority = priority;
  data.startupFunction = startupFcn;

  NIM_STARTUP_CRIT_SEC_ENTER();

  pData = avlInsertEntry(&nimStartUpTree, &data);

  NIM_STARTUP_CRIT_SEC_EXIT();

  if (pData !=  NULL)
  {
    NIM_LOG_MSG("NIM: startup function for %d not added to the AVL tree\n",
                componentId);
  }

  return;
}

/*********************************************************************
* @purpose  Find the first startup function
*
* @param    pRetData     @b{(input)}  pointer to return data
*
* @returns   SUCCESS if an entry exists
* @returns   FAILURE if no entry exists
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimStartUpFirstGet(nimStartUpTreeData_t *pRetData)
{
  RC_t rc =  FAILURE;
  nimStartUpTreeData_t searchData;
  nimStartUpTreeData_t *pData;

  searchData.priority = 0xffffffff;
  searchData.componentId = 0;

  NIM_STARTUP_CRIT_SEC_ENTER();

  pData = avlSearch (&nimStartUpTree, &searchData, AVL_NEXT);

  NIM_STARTUP_CRIT_SEC_EXIT();

  if (pData !=  NULL)
  {
    memcpy(pRetData, pData, sizeof(nimStartUpTreeData_t));
    rc =  SUCCESS;

  }
  return rc;
}

/*********************************************************************
* @purpose  Find the next startup function
*
* @param    pRetData     @b{(input)}  pointer to search/return data
*
* @returns   SUCCESS if a next entry exists
* @returns   FAILURE if no next entry exists
*
* @notes    none
*
* @end
*********************************************************************/
RC_t nimStartUpNextGet(nimStartUpTreeData_t *pRetData)
{
  RC_t rc =  FAILURE;
  nimStartUpTreeData_t *pData;

  NIM_STARTUP_CRIT_SEC_ENTER();

  pData = avlSearch (&nimStartUpTree, pRetData, AVL_NEXT);

  NIM_STARTUP_CRIT_SEC_EXIT();

  if (pData !=  NULL)
  {
    memcpy(pRetData, pData, sizeof(nimStartUpTreeData_t));
    rc =  SUCCESS;

  }
  return rc;
}


/*********************************************************************
*
* @purpose  Invoke startup callbacks for registered components
*
* @param    phase     @b{(input)}  startup phase - create or activate
*
* @returns  void
*
* @notes    Startup's are invoked serially, waiting for each one to
*           complete before invoking the next component's startup.
*
* @end
*********************************************************************/
void nimStartupCallbackInvoke(NIM_STARTUP_PHASE_t phase)
{
  RC_t              rc;
  nimStartUpTreeData_t startupData;
  nimPdu_t             startupMsg;
  static  BOOL       nimIssuRestoreInitiated =  FALSE;
   CNFGR_STATE_t     cnfgr_state;
  uint32            retry_count;

#ifdef  TRACE_ENABLED
  osapiTraceEvents     traceEventBegin = (phase == NIM_INTERFACE_CREATE_STARTUP) ? 
     TRACE_EVENT_NIM_CREATE_STARTUP_START :
     TRACE_EVENT_NIM_ACTIVATE_STARTUP_START;
  osapiTraceEvents     traceEventEnd = (phase == NIM_INTERFACE_CREATE_STARTUP) ? 
     TRACE_EVENT_NIM_CREATE_STARTUP_END :
     TRACE_EVENT_NIM_ACTIVATE_STARTUP_END;
#endif


  memset(&startupMsg, 0, sizeof(startupMsg));


#ifdef  TRACE_ENABLED
  OSAPI_TRACE_EVENT(traceEventBegin, ( uchar8 *) NULLPTR, 0);
#endif

  rc = nimStartUpFirstGet(&startupData);
  startupMsg.msgType = START_MSG;

  while (rc ==  SUCCESS)
  {
    if (nimCtlBlk_g->nimPhaseStatus !=  CNFGR_STATE_E)
    {
       LOGF( LOG_SEVERITY_INFO,
          "Terminating %s startup callbacks. No longer in EXECUTE state.",
          phase == NIM_INTERFACE_CREATE_STARTUP ? "CREATE" : "ACTIVATE");
      return;
    }
    nimStartUpCompId = startupData.componentId;

    /* Send the startup function to NIM to execute on NIM's task */
    startupMsg.data.nimStartMsg.componentId = nimStartUpCompId;
    startupMsg.data.nimStartMsg.startupFunction = startupData.startupFunction;
    startupMsg.data.nimStartMsg.phase = phase;

    /* send the message to NIM_QUEUE */
    if (osapiMessageSend(nimCtlBlk_g->nimMsgQueue, (void *)&startupMsg,
                         (uint32)sizeof(nimPdu_t),  WAIT_FOREVER,
                          MSG_PRIORITY_NORM ) ==  ERROR)
    {
      NIM_LOG_MSG("NIM: failed to send START message to NIM message Queue.\n");
      rc =  FAILURE;
    }
    else
    {
      /* Wait until startup has completed */
      osapiSemaTake(nimStartUpEvSema,  WAIT_FOREVER);
    }

    rc = nimStartUpNextGet(&startupData);
  }
#ifdef  TRACE_ENABLED
  OSAPI_TRACE_EVENT(traceEventEnd, ( uchar8 *) NULLPTR, 0 );
#endif
}

/*********************************************************************
*
* @purpose  Status callback from components to NIM for startup complete
*           Notifications
*
* @param    componentId  @b{(input)} component ID of the caller.
*
* @returns  void
*
* @notes    At the conclusion of processing a startup Event, each component
*           must call this function.
*
* @end
*********************************************************************/
extern void nimStartupEventDone( COMPONENT_IDS_t componentId)
{
    if (componentId == nimStartUpCompId)
    {
      nimStartUpCompId =  FIRST_COMPONENT_ID;
      osapiSemaGive(nimStartUpCbSema);
    }
    else
    {
       LOGF( LOG_SEVERITY_EMERGENCY,
              "Event Done received for component %d, expecting %d\n",
              componentId, nimStartUpCompId);
    }
}

/*********************************************************************
*
* @purpose  Waits for the component to complete its STARTUP processing,
*           then gives the nimStartUpEvSema signaling cardmgr to proceed
*           to the next component.
*
* @param    none
*
* @returns  void
*
* @notes    none
*
* @end
*********************************************************************/
extern void nimStartupEventWait(void)
{
  osapiSemaTake(nimStartUpCbSema,  WAIT_FOREVER);
  osapiSemaGive(nimStartUpEvSema);
}

