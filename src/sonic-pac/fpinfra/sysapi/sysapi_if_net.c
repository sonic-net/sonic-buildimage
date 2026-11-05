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
#include "system_exports.h"
#include "log.h"

/*************************************
* Mbuf Queue declarations
*************************************/
extern void **pMbufQTop;     /* top of queue */
extern void **pMbufQBot;     /* bottom of queue */
extern void **MbufQHead;
extern void **MbufQTail;
extern uint32 MbufsFree;
extern uint32 MbufsRxUsed;
extern uint32 MbufsMaxFree;
extern void      *pMbufPool;

mbuf_stats_t mbuf_stats;

#define __USE_GNU
#include <pthread.h>
#include <time.h>
static pthread_mutex_t sysapiMbufMutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#undef __USE_GNU
#define SYSAPI_MBUF_LOCK() pthread_mutex_lock(&sysapiMbufMutex)
#define SYSAPI_MBUF_UNLOCK() pthread_mutex_unlock(&sysapiMbufMutex)

/**************************************************************************
* @purpose  Function used to track the Mbuf with the file name and the line num.
*           This can be used by the individual components for further tracking of Mbuf.
*
* @param    netMbufHandle - Ptr to network mbuf handle
* @param    file - file name
* @param    line - line number
*
* @returns  void
*
* @comments
*
* @end
*************************************************************************/
void sysapiNetMbufTrack( netBufHandle netMbufHandle,  uchar8 *file, uint32 line)
{
  SYSAPI_NET_MBUF_HEADER_t *bufHandle = (SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle;

  osapiStrncpySafe(bufHandle->last_file, file, sizeof(bufHandle->last_file));
  bufHandle->last_line = line;
}


/**************************************************************************
*
* @purpose  Retrieve a network mbuf to the caller (and track the caller)
*
* @param    none.
*
* @returns  A ptr to a network mbuf handle
* @returns  0 if none are available
*
* @notes    Delegates to sysapiNetMbufGet
*
* @end
*
*************************************************************************/
 netBufHandle sysapiNetMbufGetTrack(const  char8 *file, uint32 line)
{
  SYSAPI_NET_MBUF_HEADER_t *netMbufHandle = 0;

  /* get the MBUF */
  netMbufHandle = (SYSAPI_NET_MBUF_HEADER_t *)sysapiNetMbufGet();

  /* store tracking information */
  if(netMbufHandle)
  {
    osapiStrncpySafe(netMbufHandle->last_file, file, sizeof(netMbufHandle->last_file));
    netMbufHandle->last_line = line;
    netMbufHandle->mbufLoc = MBUF_LOC_ALLOC;

  }

  return(( netBufHandle)netMbufHandle);
}


/**************************************************************************
* @purpose  Retrieve an mbuf to the caller
*
* @param    none.
*
* @returns  A ptr to an mbuf
* @returns   NULL if none are available
*
* @comments    Use the Mutex semaphore to inhibit global variable corruption
*
* @end
*************************************************************************/
static uint32 *sysapiMbufGet( void )
{
  void * buffer;
  uint32 mbufUsed = 0;

  if ( MbufsFree != 0 )
  {
    buffer = *MbufQHead;
    if ( MbufQHead >= pMbufQBot )
      MbufQHead = pMbufQTop;    /* wrap the Q head ptr */
    else
      MbufQHead++;        /* move the Q head ptr */
    MbufsFree--;         /* keep track...       */

    ((SYSAPI_NET_MBUF_HEADER_t *)buffer)->in_use =  TRUE;
  }
  else
  {
    buffer =  NULL;
  }

  mbufUsed = MbufsMaxFree - MbufsFree;
  /* (void) sysapiMbufUsedNotify(mbufUsed); */

  return( ( uint32 * )buffer );
}

/**************************************************************************
* @purpose  Retrieve a network mbuf to the caller
*
* @param    none.
*
* @returns  A ptr to a network mbuf handle
* @returns  0 if none are available
*
* @comments
*
* @end
*************************************************************************/
 netBufHandle sysapiNetMbufGet( void )
{
  SYSAPI_NET_MBUF_HEADER_t *netMbufHandle;

  SYSAPI_MBUF_LOCK();
  mbuf_stats.alloc_tx_alloc_attempts++;

  netMbufHandle = (SYSAPI_NET_MBUF_HEADER_t *)sysapiMbufGet();
  if (netMbufHandle !=  NULL)
  {
    netMbufHandle->bufStart  = ( uchar8 *)netMbufHandle + sizeof(SYSAPI_NET_MBUF_HEADER_t) +
                                NET_MBUF_START_OFFSET;

    netMbufHandle->bufStart = ( char8 *)SYSAPI_BUF_ALIGN(netMbufHandle->bufStart, MBUF_ALIGN_BOUND);

    netMbufHandle->bufLength = 0;
    netMbufHandle->taskId    = osapiTaskIdSelf();
    netMbufHandle->timeStamp = osapiUpTimeRaw();
    netMbufHandle->rxBuffer  =  FALSE;

    /* wipe out tracking information */
    netMbufHandle->last_file[0] = 0;
    netMbufHandle->last_line = 0;
    netMbufHandle->mbufLoc = MBUF_LOC_ALLOC;
  }
  else
  {
    mbuf_stats.alloc_tx_failures++;
  }

  SYSAPI_MBUF_UNLOCK();

  return(( netBufHandle)netMbufHandle);
}


/**************************************************************************
* @purpose  Retrieve an aligned network mbuf to the caller
*
* @param    align   @b{(input)}  Alignment indicator, for IP or frame
*
* @returns  A ptr to a network mbuf handle
* @returns  0 if none are available
*
* @note     All mbufs are 4 byte aligned
*
* @end
*************************************************************************/
 netBufHandle sysapiNetMbufAlignGet( uchar8 *file, uint32 line,
                                       MBUF_ALIGNMENT alignType)
{
  SYSAPI_NET_MBUF_HEADER_t *netMbufHandle = 0;

  /* get the MBUF */
  netMbufHandle = (SYSAPI_NET_MBUF_HEADER_t *)sysapiNetMbufGet();

  /* store tracking information */
  if(netMbufHandle)
  {
    if ( MBUF_IP_ALIGNED == alignType)
    {
      /* Compensate for ipheader offset */
      netMbufHandle->bufStart +=  MBUF_IP_CORRECTION;
    }
    osapiStrncpySafe(netMbufHandle->last_file, file, sizeof(netMbufHandle->last_file));
    netMbufHandle->last_line = line;
    netMbufHandle->mbufLoc = MBUF_LOC_ALLOC;
  }

  return(( netBufHandle)netMbufHandle);
}

/**************************************************************************
* @purpose  Free a network mbuf with debug information.
*
* @param    Ptr to network mbuf handle
*
* @returns  void
*
* @comments
*
* @end
*************************************************************************/
void sysapiNetMbufFreeTrack(  netBufHandle netMbufHandle, const  char8 *file, uint32 line)
{
  SYSAPI_NET_MBUF_HEADER_t  *header;
#if  FEAT_CPUSTATS
  uint32 headNext =  NULL;
  uint32 traceIndex =  NULL;
  uint32 currentStep =  NULL;
#endif /*  FEAT_CPUSTATS */

  header = (SYSAPI_NET_MBUF_HEADER_t *) netMbufHandle;

  if (header->in_use ==  FALSE)
  {
    LOG_ERROR ((uint32) ((unsigned long) netMbufHandle));
  }

  osapiStrncpySafe(header->last_file, file, sizeof(header->last_file));
  header->last_line = line;
  header->mbufLoc = MBUF_LOC_FREE;

  sysapiNetMbufFree (netMbufHandle);

}


/**************************************************************************
* @purpose  Return an mbuf to the mbuf pool
*
* @param    *mbuf ptr to the mbuf to return
*
* @returns  none.
*
* @comments    Use the Mutex semaphore to inhibit global variable corruption
*
* @end
*************************************************************************/
static void sysapiMbufFree(  uint32 *mbuf )
{
  uint32 mbufUsed = 0;


  *MbufQTail = ( void * )mbuf;
  if ( MbufQTail >= pMbufQBot )
    MbufQTail = pMbufQTop;    /* wrap the Q tail ptr */
  else
    MbufQTail++;        /* move the Q tail ptr */

  MbufsFree++;         /* keep track...       */

  /* If we have extra buffers then somebody must have freed a buffer twice.
  ** This is a fatal error.
  */
  if (MbufsFree > MbufsMaxFree)
  {
    LOG_ERROR ((unsigned long) mbuf);
  }

  mbufUsed = MbufsMaxFree - MbufsFree;
  /*(void) sysapiMbufUsedNotify(mbufUsed);*/


  return;
}

/**************************************************************************
* @purpose  Free a network mbuf
*
* @param    Ptr to network mbuf handle
*
* @returns  void
*
* @comments
*
* @end
*************************************************************************/
void sysapiNetMbufFree(  netBufHandle netMbufHandle )
{
  SYSAPI_MBUF_LOCK();
  if (netMbufHandle !=  NULL)
  {
    if (((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->in_use ==  TRUE)
    {
      sysapiMbufRxusedStatsUpdate(((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->priorityPool,
                                     FALSE);
    }
    ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->bufStart  =  NULL;
    ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->bufLength = 0;
    ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->taskId    = 0;
    ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->timeStamp = 0;
    ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->in_use    =  FALSE;
    ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->mbufLoc   = MBUF_LOC_FREE;
    ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->priorityPool =  MBUF_RX_PRIORITY_NULL;
    ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->rxCode    =  MBUF_RX_REASON_NONE;

    if (((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->rxBuffer ==  TRUE)
    {
      ((SYSAPI_NET_MBUF_HEADER_t *)netMbufHandle)->rxBuffer =  FALSE;

      MbufsRxUsed--;
    }

    sysapiMbufFree ((uint32 *)netMbufHandle);
  }
  SYSAPI_MBUF_UNLOCK();
}


/**********************************************************************
* @pupose   To increment or decrement Mbuf stats per RX priority as in
*            MBUF_RX_PRIORITY
*
* @param    priority @b{(input)}  Mbuf priority to increment/decrement
*                                 corresponding stats
*           operation @b{(input)} Bool value to increment/decrement
*                                  TRUE - to increment
*                                  FALSE - to decrement
*
* @returns  SUCCESS,
*
* @notes
*
* @end
*
*********************************************************************/
RC_t sysapiMbufRxusedStatsUpdate( MBUF_RX_PRIORITY priority,
                                     BOOL operation)
{
  switch(priority)
  {
    case  MBUF_RX_PRIORITY_HIGH:
      operation ? mbuf_stats.alloc_rx_high++ : mbuf_stats.alloc_rx_high--;
      break;
    case  MBUF_RX_PRIORITY_MID0:
      operation ? mbuf_stats.alloc_rx_mid0++ : mbuf_stats.alloc_rx_mid0--;
      break;
    case  MBUF_RX_PRIORITY_MID1:
      operation ? mbuf_stats.alloc_rx_mid1++ : mbuf_stats.alloc_rx_mid1--;
      break;
    case  MBUF_RX_PRIORITY_MID2:
      operation ? mbuf_stats.alloc_rx_mid2++ : mbuf_stats.alloc_rx_mid2--;
      break;
    case  MBUF_RX_PRIORITY_NORMAL:
      operation ? mbuf_stats.alloc_rx_norm++ : mbuf_stats.alloc_rx_norm--;
      break;
    default :
      break;
  }
  return  SUCCESS;
}

