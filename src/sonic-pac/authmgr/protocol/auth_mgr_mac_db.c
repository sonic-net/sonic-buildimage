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

extern authmgrCB_t *authmgrCB;


/* Global parameters */
typedef struct authmgrMacAddrInfo_s{
     sll_member_t          *next;
     enetMacAddr_t         suppMacAddr;
    uint32                lIntIfNum;
}authmgrMacAddrInfo_t;

/*************************************************************************
* @purpose  API to destroy the Mac Addr Info data node
*
* @param    ll_member  @b{(input)}  Linked list node containing the
*                                   Mac Addr Info to be destroyed
*
* @returns   SUCCESS
*
* @comments This is called by SLL library when a node is being deleted
*
* @end
*************************************************************************/
RC_t authmgrMacAddrDataDestroy ( sll_member_t *ll_member)
{
  authmgrMacAddrInfo_t *pMacAddrInfo ;

  pMacAddrInfo = ( authmgrMacAddrInfo_t *)ll_member;

  pMacAddrInfo->lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;

  bufferPoolFree(authmgrCB->globalInfo->authmgrMacAddrBufferPoolId,( uchar8 *)pMacAddrInfo);

  return  SUCCESS;
}
/*************************************************************************
* @purpose  Helper API to compare two Mac Addr Info nodes  and
*           returns the result
*
* @param     p  @b{(input)}  Pointer to Candidate 1 for comparison
* @param     q  @b{(input)}  Pointer to Candidate 2 for comparison
*
* @returns   0   p = q
* @returns  -1   p < q
* @returns  +1   p > q
*
* @comments This is called by SLL library when a nodes are compared
*
* @end
*************************************************************************/
 int32 authmgrMacAddrDataCmp(void *p, void *q, uint32 key)
{
     int32 cmp;
    cmp = memcmp(((authmgrMacAddrInfo_t *)p)->suppMacAddr.addr,((authmgrMacAddrInfo_t *)q)->suppMacAddr.addr, ENET_MAC_ADDR_LEN);
    if (cmp == 0)
      return 0;
  else if (cmp < 0)
      return -1;
  else
      return 1;

}


/*********************************************************************
* @purpose  Initialize Mac Address Info Database
*
* @param    nodeCount    @b{(input)} The number of nodes to be created.
*
* @returns   SUCCESS or  FAILURE
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrMacAddrInfoDBInit(uint32 nodeCount)
{
  /* Allocate the buffer pool */
  if (bufferPoolInit( AUTHMGR_COMPONENT_ID, nodeCount, sizeof(authmgrMacAddrInfo_t), 
                     "Authmgr Mac Addr Bufs",
                     &(authmgrCB->globalInfo->authmgrMacAddrBufferPoolId)) !=  SUCCESS)
  {
     LOGF(  LOG_SEVERITY_NOTICE,
        "\n%s: Error allocating buffers for supplicant mac address database."
        " Could not allocate buffer pool for Mac address link list. Insufficient memory."
        ,__FUNCTION__);
    AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_FAILURE,0,
                "%s: Error allocating buffers for supplicant mac address database\n",__FUNCTION__);
    return  FAILURE;
  }

  /* Create the SLL*/
  if (SLLCreate( AUTHMGR_COMPONENT_ID,  SLL_ASCEND_ORDER,
               sizeof( enetMacAddr_t),authmgrMacAddrDataCmp ,authmgrMacAddrDataDestroy ,
               &authmgrCB->globalInfo->authmgrMacAddrSLL) !=  SUCCESS)
  {
     LOGF( LOG_SEVERITY_INFO,
            "\n%s: Failed to create supplicant mac address linked list \n",__FUNCTION__);
    AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_FAILURE,0,
                "%s: Failed to create supplicant mac address linked list \n",__FUNCTION__);
    return  FAILURE;
  }

  /* Create Mac Address DB Semaphore*/
  /* Read write lock for controlling Mac Addr Info additions and Deletions */
  if (osapiRWLockCreate(&authmgrCB->globalInfo->authmgrMacAddrDBRWLock,
                        OSAPI_RWLOCK_Q_PRIORITY) ==  FAILURE)
  {
       LOGF( LOG_SEVERITY_INFO,
              "Error creating dot1qCfgRWlock semaphore \n");
      return  FAILURE;
  }

  return  SUCCESS;
}


/*********************************************************************
* @purpose  DeInitialize Mac Address Info Database
*
* @param    none
*
* @returns   SUCCESS or  FAILURE
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrMacAddrInfoDBDeInit(void)
{



  /* Destroy the SLL */
  if (SLLDestroy( AUTHMGR_COMPONENT_ID, &authmgrCB->globalInfo->authmgrMacAddrSLL)!=  SUCCESS)
  {
     LOGF( LOG_SEVERITY_INFO,
            "\n%s: Failed to destroy the supplicant mac address linked list \n",__FUNCTION__);
    AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_MAC_ADDR_DB,0,
                "\n%s: Failed to destroy the supplicant mac address linked list \n",__FUNCTION__);
  }

  /* Deallocate the buffer pool */

  if (authmgrCB->globalInfo->authmgrMacAddrBufferPoolId  != 0)
  {
    bufferPoolDelete(authmgrCB->globalInfo->authmgrMacAddrBufferPoolId );
    authmgrCB->globalInfo->authmgrMacAddrBufferPoolId  = 0;
  }

  /* Delete the Mac Address DB Semaphore */
  (void)osapiRWLockDelete(authmgrCB->globalInfo->authmgrMacAddrDBRWLock);


  return  SUCCESS;
}

/*********************************************************************
* @purpose  Add a node Mac Address Info Database
*
* @param    mac_addr  supplicant mac address
* @param    lIntIfNum corresponding logical interface
*
* @returns   SUCCESS or  FAILURE
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrMacAddrInfoAdd( enetMacAddr_t *mac_addr,uint32 lIntIfNum)
{
   authmgrMacAddrInfo_t *pMacAddrInfo,*pMacAddrFind,macAddrInfo;
    enetMacAddr_t    nullMacAddr;
   uint32 physPort = 0;

   memset(&(nullMacAddr.addr),0, ENET_MAC_ADDR_LEN);

   AUTHMGR_PORT_GET(physPort, lIntIfNum);

   if (mac_addr== NULL)
   {
       AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_FAILURE, physPort,"\n%s: Could not add supplicant mac address. Mac address is NULL. Input error. \n",
               __FUNCTION__);
       return  FAILURE;
   }
   if((lIntIfNum == AUTHMGR_LOGICAL_PORT_ITERATE)||
        (memcmp(mac_addr->addr,nullMacAddr.addr, ENET_MAC_ADDR_LEN)==0))
   {
       AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_FAILURE, physPort,"\n%s: Could not add supplicant mac address(%s) logical Interface: %d . Input error. \n",
               __FUNCTION__,AUTHMGR_PRINT_MAC_ADDR(mac_addr->addr),lIntIfNum);
       return  FAILURE;
   }

   /* In order to handle client roaming , check if the mac address already exists*/
   /* copy info to node*/
    memcpy(macAddrInfo.suppMacAddr.addr,mac_addr->addr, ENET_MAC_ADDR_LEN);
   /* take Mac address DB semaphore*/
   (void)osapiWriteLockTake(authmgrCB->globalInfo->authmgrMacAddrDBRWLock,  WAIT_FOREVER);

   if ((pMacAddrFind=(authmgrMacAddrInfo_t *)SLLFind(&authmgrCB->globalInfo->authmgrMacAddrSLL,( sll_member_t *)&macAddrInfo)) !=  NULLPTR)
   {
       pMacAddrFind->lIntIfNum = lIntIfNum;
      (void) osapiWriteLockGive(authmgrCB->globalInfo->authmgrMacAddrDBRWLock);
       AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_MAC_ADDR_DB,lIntIfNum,"\n%s: Found supplicant mac address(%s) Changed logical Interface to: %d .\n",
               __FUNCTION__, AUTHMGR_PRINT_MAC_ADDR(mac_addr->addr),lIntIfNum);
       return  SUCCESS;
   }

   /* Allocate memory from the buffer pool*/
   if (bufferPoolAllocate(authmgrCB->globalInfo->authmgrMacAddrBufferPoolId,
                            ( uchar8 **)&pMacAddrInfo) !=  SUCCESS)
     {
       /* release semaphore*/
       (void)osapiWriteLockGive(authmgrCB->globalInfo->authmgrMacAddrDBRWLock);

        LOGF(  LOG_SEVERITY_NOTICE,
           "\n%s: Could not add supplicant mac address(%s) logical Interface: %d . Insufficient memory. \n",
               __FUNCTION__, AUTHMGR_PRINT_MAC_ADDR(mac_addr->addr), lIntIfNum);

       AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_FAILURE, physPort,"\n%s: Could not add supplicant mac address(%s) logical Interface: %d . Insufficient memory. \n",
               __FUNCTION__, AUTHMGR_PRINT_MAC_ADDR(mac_addr->addr),lIntIfNum );
       return  FAILURE;
     }

   /* copy details into allocated node*/
   memset(pMacAddrInfo,0,sizeof(authmgrMacAddrInfo_t));
   memcpy(pMacAddrInfo->suppMacAddr.addr,mac_addr->addr, ENET_MAC_ADDR_LEN);
   pMacAddrInfo->lIntIfNum = lIntIfNum;


   /* Add node to the SLL*/
   if (SLLAdd(&authmgrCB->globalInfo->authmgrMacAddrSLL, ( sll_member_t *)pMacAddrInfo)
         !=  SUCCESS)
   {
       AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_FAILURE,authmgrPhysPortGet(lIntIfNum),"\n%s: Could not add supplicant mac address(%s) logical Interface: %d . \n",
               __FUNCTION__, AUTHMGR_PRINT_MAC_ADDR(mac_addr->addr),lIntIfNum);
       /* Free the previously allocated bufferpool */
       bufferPoolFree(authmgrCB->globalInfo->authmgrMacAddrBufferPoolId, ( uchar8 *)pMacAddrInfo);
       /* release semaphore*/
       (void)osapiWriteLockGive(authmgrCB->globalInfo->authmgrMacAddrDBRWLock);
       return  FAILURE;
   }

    /* release semaphore*/
    (void)osapiWriteLockGive(authmgrCB->globalInfo->authmgrMacAddrDBRWLock);
    return  SUCCESS;
}

/*********************************************************************
* @purpose  Remove a node from the Mac Address Info Database
*
* @param    mac_addr  supplicant mac address
*
* @returns   SUCCESS or  FAILURE
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrMacAddrInfoRemove( enetMacAddr_t *mac_addr)
{
   authmgrMacAddrInfo_t macAddrInfo;
    enetMacAddr_t    nullMacAddr;

   memset(&nullMacAddr.addr,0, ENET_MAC_ADDR_LEN);
   /*input check*/
   if ((mac_addr ==  NULLPTR) ||
        (memcmp(mac_addr->addr,nullMacAddr.addr, ENET_MAC_ADDR_LEN)==0))
   {
       return  FAILURE;
   }
   /* copy info to node*/
   memcpy(macAddrInfo.suppMacAddr.addr,mac_addr->addr, ENET_MAC_ADDR_LEN);
   macAddrInfo.lIntIfNum = 0;

   /* take Mac address DB semaphore*/
   (void)osapiWriteLockTake(authmgrCB->globalInfo->authmgrMacAddrDBRWLock,  WAIT_FOREVER);

    /* delete from SLL*/
    if (SLLDelete(&authmgrCB->globalInfo->authmgrMacAddrSLL, ( sll_member_t *)&macAddrInfo)
                  !=  SUCCESS)
    {
        /* release semaphore*/
       (void)osapiWriteLockGive(authmgrCB->globalInfo->authmgrMacAddrDBRWLock);

        AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_FAILURE,0,"\n%s: Could not delete supplicant mac address(%s) from the SLL . \n",
               __FUNCTION__,AUTHMGR_PRINT_MAC_ADDR(mac_addr->addr));
       return  FAILURE;
    }

  /* release semaphore*/
  (void)osapiWriteLockGive(authmgrCB->globalInfo->authmgrMacAddrDBRWLock);
  return  SUCCESS;
}


/*********************************************************************
* @purpose  Find a node in the Mac Address Info Database
*
* @param    mac_addr            supplicant mac address
* @param   lIntIfnum{(output)} logical Port Number
*
* @returns   SUCCESS or  FAILURE
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrMacAddrInfoFind( enetMacAddr_t *mac_addr,uint32 *lIntIfNum)
{
  authmgrMacAddrInfo_t macAddrInfo,*pMacAddrInfo;
   enetMacAddr_t    nullMacAddr;

  memset(&nullMacAddr.addr,0, ENET_MAC_ADDR_LEN);
  /*input check*/
  if ((mac_addr ==  NULLPTR) ||
      (memcmp(mac_addr->addr,nullMacAddr.addr, ENET_MAC_ADDR_LEN)==0))
  {
      return  FAILURE;
  }
  /* copy info to node*/
  memcpy(macAddrInfo.suppMacAddr.addr,mac_addr->addr, ENET_MAC_ADDR_LEN);

  /* take Mac address DB semaphore*/
   (void)osapiWriteLockTake(authmgrCB->globalInfo->authmgrMacAddrDBRWLock,  WAIT_FOREVER);

  if ((pMacAddrInfo=(authmgrMacAddrInfo_t *)SLLFind(&authmgrCB->globalInfo->authmgrMacAddrSLL,( sll_member_t *)&macAddrInfo)) ==  NULLPTR)
  {
      /* release semaphore*/
     (void)osapiWriteLockGive(authmgrCB->globalInfo->authmgrMacAddrDBRWLock);
      AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_FAILURE,0,"\n%s: Could not find supplicant mac address(%s). \n",
               __FUNCTION__, AUTHMGR_PRINT_MAC_ADDR(mac_addr->addr));
      *lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;
      return  FAILURE;
  }
  *lIntIfNum = pMacAddrInfo->lIntIfNum;
  /* release semaphore*/
  (void)osapiWriteLockGive(authmgrCB->globalInfo->authmgrMacAddrDBRWLock);
  return  SUCCESS;
}

/*********************************************************************
* @purpose  Find the next node in the Mac Address Info Database
*
* @param    mac_addr            supplicant mac address
* @param   lIntIfnum{(output)}  logical Port Number
*
* @returns   SUCCESS or  FAILURE
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrMacAddrInfoFindNext( enetMacAddr_t *mac_addr,uint32 *lIntIfNum)
{
  authmgrMacAddrInfo_t macAddrInfo,*pMacAddrInfo;

  /*input check*/
  if (mac_addr ==  NULLPTR)
  {
      return  FAILURE;
  }

  /* copy info to node*/
  memcpy(macAddrInfo.suppMacAddr.addr,mac_addr->addr, ENET_MAC_ADDR_LEN);

   /* take Mac address DB semaphore*/
   (void)osapiWriteLockTake(authmgrCB->globalInfo->authmgrMacAddrDBRWLock,  WAIT_FOREVER);

  if ((pMacAddrInfo=(authmgrMacAddrInfo_t *)SLLFindNext(&authmgrCB->globalInfo->authmgrMacAddrSLL,( sll_member_t *)&macAddrInfo)) ==  NULLPTR)
  {
      /* release semaphore*/
      (void)osapiWriteLockGive(authmgrCB->globalInfo->authmgrMacAddrDBRWLock);

      AUTHMGR_EVENT_TRACE(AUTHMGR_TRACE_FAILURE,0,"\n%s: Could not find next node for supplicant mac address(%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x). \n",
               __FUNCTION__, mac_addr->addr[0],mac_addr->addr[1],mac_addr->addr[2],mac_addr->addr[3],mac_addr->addr[4],mac_addr->addr[5]);
      *lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;
      return  FAILURE;
  }
  memcpy(mac_addr->addr,pMacAddrInfo->suppMacAddr.addr, ENET_MAC_ADDR_LEN);
  *lIntIfNum = pMacAddrInfo->lIntIfNum;

  /* release semaphore*/
  (void)osapiWriteLockGive(authmgrCB->globalInfo->authmgrMacAddrDBRWLock);

  return  SUCCESS;
}

/*********************************************************************
* @purpose  Print the information in the Mac Address Database
*
* @param
*
* @returns   SUCCESS or  FAILURE
*
* @comments none
*
* @end
*********************************************************************/
RC_t authmgrDebugMacAddrDBList()
{
     enetMacAddr_t macAddr,nullMacAddr;
    uint32 lIntIfNum;

    memset(&macAddr,0,sizeof( enetMacAddr_t));

    while(authmgrMacAddrInfoFindNext(&macAddr,&lIntIfNum)==  SUCCESS)
    {
        SYSAPI_PRINTF("\n Mac Address: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x",
                      macAddr.addr[0],macAddr.addr[1],macAddr.addr[2],macAddr.addr[3],macAddr.addr[4],macAddr.addr[5]);

        SYSAPI_PRINTF("\n Logical Port :%u",lIntIfNum);

    }

     memset(&nullMacAddr,0,sizeof( enetMacAddr_t));
     if (memcmp(macAddr.addr,nullMacAddr.addr, ENET_MAC_ADDR_LEN)!=0)
     {
         if (authmgrMacAddrInfoFind(&macAddr,&lIntIfNum)== SUCCESS)
         {
             SYSAPI_PRINTF("\n Testing authmgrMacAddrInfoFind.Found \n");
             SYSAPI_PRINTF("\n Mac Address: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x",
                      macAddr.addr[0],macAddr.addr[1],macAddr.addr[2],macAddr.addr[3],macAddr.addr[4],macAddr.addr[5]);

            SYSAPI_PRINTF("\n Logical Port :%u",lIntIfNum);
         }
     }
    return  SUCCESS;
}
