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
#include "pacinfra_common.h"
#include "product.h"
#include "resources.h"
#include "osapi.h"
#include "sysapi.h"
#include "defaultconfig.h"

static void * sysapiTimerTaskID = 0;

/*************************************
 * Mbuf Queue declarations
 *************************************/
void **pMbufQTop;      /* top of queue */
void **pMbufQBot;      /* bottom of queue */
void **MbufQHead;
void **MbufQTail;
uint32 MbufsFree;
uint32 MbufsRxUsed;
uint32 MbufsMaxFree;
void      *pMbufPool;


/**************************************************************************
 *
 * @purpose  Task that creates the application timer function
 *
 * @param    none
 *
 * @returns  none.
 *
 * @notes If the task is already created, just return.
 *
 * @end
 *
 *************************************************************************/
void sysapiTimerTaskStart(void)
{
  if ( sysapiTimerTaskID != 0 )
  {
    return;
  }
  sysapiTimerTaskID = osapiTaskCreate(   "osapiTimer",
      (void *)osapiTimerHandler,
      0,
       NULLPTR,
       DEFAULT_STACK_SIZE,
       MEDIUM_TASK_PRIORITY,
       DEFAULT_TASK_SLICE);

  /* Wait for osapiTimer task to finish initialization.*/
  if (osapiWaitForTaskInit( OSAPI_TIMER_TASK_SYNC,  WAIT_FOREVER) !=  SUCCESS)
  {
    //SYSAPI_PRINTF(SYSAPI_LOGGING_ALWAYS, "In routine %s line %d, osapiWaitForTaskInit failed\n",
    //    __FUNCTION__, __LINE__);
    //LOG_ERROR(0);
  }
  return;
}

/**************************************************************************
 * @purpose  Initialize the sysapi component
 *
 * @param    none
 *
 * @returns   SUCCESS
 * @returns   ERROR
 *
 * @notes
 *
 * @end
 *
 *************************************************************************/
RC_t sysapiSystemInit(void)
{
   int32 i;
  uint32 temp32;
  uint32 phy_size = 0;
  uint32 mtu_size = FD_NIM_DEFAULT_MTU_SIZE;

  /* initialize system timers */
  sysapiTimerTaskStart();

  /* Reserve extra space for control overhead. */
  phy_size += (sizeof(SYSAPI_NET_MBUF_HEADER_t) + NET_MBUF_START_OFFSET + 64);

  temp32 = phy_size + mtu_size + SYSAPI_PKT_BUF_ALIGN_LEN;

  pMbufPool = osapiMalloc (  SIM_COMPONENT_ID,  MAX_NETWORK_BUFF_PER_BOX * ( temp32 ) );
  if ( pMbufPool ==  NULLPTR )
    return( ERROR);


  /********************************************************
   * Allocate the "mbuf" Queue. Each entry is a 32 bit ptr
   *********************************************************/
  pMbufQTop = ( void ** )osapiMalloc (  SIM_COMPONENT_ID,  MAX_NETWORK_BUFF_PER_BOX * sizeof (void *));
  if ( pMbufQTop ==  NULLPTR )
    return( ERROR);

  /***************************************************
   * Initialize the "mbuf" Queue and counter.
   ****************************************************/
  MbufQHead = pMbufQTop;
  MbufQTail = pMbufQTop;
  MbufsMaxFree =  MAX_NETWORK_BUFF_PER_BOX;
  MbufsFree = MbufsMaxFree;
  MbufsRxUsed = 0;
  memset(&mbuf_stats, 0, sizeof(mbuf_stats_t));

  for ( i=0;i<( int32)MbufsFree;i++ )
  {
    *MbufQHead = ( void * ) ( ( uchar8 *)pMbufPool + i * ( temp32 ));
    MbufQHead++;
  }
  pMbufQBot = --MbufQHead;            /* set bottom of queue ptr */
  MbufQHead = pMbufQTop;              /* reset head ptr to top */


  return( SUCCESS);
}
