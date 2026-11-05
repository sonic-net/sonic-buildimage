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
#include "mab_struct.h"

extern mabBlock_t *mabBlock;


/* Global parameters */
typedef struct mabMacAddrInfo_s{
     sll_member_t          *next;
     enetMacAddr_t         suppMacAddr;
    uint32                lIntIfNum;
}mabMacAddrInfo_t;

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
RC_t mabMacAddrDataDestroy ( sll_member_t *ll_member)
{
  mabMacAddrInfo_t *pMacAddrInfo ;

  pMacAddrInfo = ( mabMacAddrInfo_t *)ll_member;

  pMacAddrInfo->lIntIfNum = MAB_LOGICAL_PORT_ITERATE;

  bufferPoolFree(mabBlock->mabMacAddrBufferPoolId,( uchar8 *)pMacAddrInfo);

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
 int32 mabMacAddrDataCmp(void *p, void *q, uint32 key)
{
     int32 cmp;
    cmp = memcmp(((mabMacAddrInfo_t *)p)->suppMacAddr.addr,((mabMacAddrInfo_t *)q)->suppMacAddr.addr, ENET_MAC_ADDR_LEN);
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
RC_t mabMacAddrInfoDBInit(uint32 nodeCount)
{
  /* Allocate the buffer pool */
  if (bufferPoolInit( MAB_COMPONENT_ID, nodeCount, sizeof(mabMacAddrInfo_t), 
                     "MAB Mac Addr Bufs",
                     &(mabBlock->mabMacAddrBufferPoolId)) !=  SUCCESS)
  {
     LOGF(  LOG_SEVERITY_NOTICE,
        "\n%s: Error allocating buffers for supplicant mac address database."
        " Could not allocate buffer pool for Mac address link list. Insufficient memory."
        ,__FUNCTION__);
    MAB_EVENT_TRACE("%s: Error allocating buffers for supplicant mac address database\n",__FUNCTION__);
    return  FAILURE;
  }

  /* Create the SLL*/
  if (SLLCreate( MAB_COMPONENT_ID,  SLL_ASCEND_ORDER,
               sizeof( enetMacAddr_t),mabMacAddrDataCmp ,mabMacAddrDataDestroy ,
               &mabBlock->mabMacAddrSLL) !=  SUCCESS)
  {
     LOGF( LOG_SEVERITY_INFO,
            "\n%s: Failed to create supplicant mac address linked list \n",__FUNCTION__);
    MAB_EVENT_TRACE("%s: Failed to create supplicant mac address linked list \n",__FUNCTION__);
    return  FAILURE;
  }

  /* Create Mac Address DB Semaphore*/
  /* Read write lock for controlling Mac Addr Info additions and Deletions */
  if (osapiRWLockCreate(&mabBlock->mabMacAddrDBRWLock,
                        OSAPI_RWLOCK_Q_PRIORITY) ==  FAILURE)
  {
       LOGF( LOG_SEVERITY_INFO,
              "Error creating mabMacAddrDBRWLock semaphore \n");
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
RC_t mabMacAddrInfoDBDeInit(void)
{
  /* Destroy the SLL */
  if (SLLDestroy( MAB_COMPONENT_ID, &mabBlock->mabMacAddrSLL)!=  SUCCESS)
  {
     LOGF( LOG_SEVERITY_INFO,
            "\n%s: Failed to destroy the supplicant mac address linked list \n",__FUNCTION__);
    MAB_EVENT_TRACE("\n%s: Failed to destroy the supplicant mac address linked list \n",__FUNCTION__);
  }

  /* Deallocate the buffer pool */

  if (mabBlock->mabMacAddrBufferPoolId  != 0)
  {
    bufferPoolDelete(mabBlock->mabMacAddrBufferPoolId );
    mabBlock->mabMacAddrBufferPoolId  = 0;
  }

  /* Delete the Mac Address DB Semaphore */
  (void)osapiRWLockDelete(mabBlock->mabMacAddrDBRWLock);


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
RC_t mabMacAddrInfoAdd( enetMacAddr_t *mac_addr,uint32 lIntIfNum)
{
   mabMacAddrInfo_t *pMacAddrInfo,*pMacAddrFind,macAddrInfo;
    enetMacAddr_t    nullMacAddr;
   uint32 physPort = 0;

   memset(&(nullMacAddr.addr),0, ENET_MAC_ADDR_LEN);

   MAB_PORT_GET(physPort, lIntIfNum);

   if (mac_addr== NULL)
   {
       MAB_EVENT_TRACE("\n%s: Could not add supplicant mac address. Mac address is NULL. Input error. \n",
               __FUNCTION__);
       return  FAILURE;
   }
   if((lIntIfNum == MAB_LOGICAL_PORT_ITERATE)||
        (memcmp(mac_addr->addr,nullMacAddr.addr, ENET_MAC_ADDR_LEN)==0))
   {
       MAB_EVENT_TRACE("\n%s: Could not add supplicant mac address(%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x) logical Interface: %d . Input error. \n",
               __FUNCTION__, mac_addr->addr[0],mac_addr->addr[1],mac_addr->addr[2],mac_addr->addr[3],mac_addr->addr[4],mac_addr->addr[5],lIntIfNum);
       return  FAILURE;
   }

   /* In order to handle client roaming , check if the mac address already exists*/
   /* copy info to node*/
    memcpy(macAddrInfo.suppMacAddr.addr,mac_addr->addr, ENET_MAC_ADDR_LEN);
   /* take Mac address DB semaphore*/
   (void)osapiWriteLockTake(mabBlock->mabMacAddrDBRWLock,  WAIT_FOREVER);

   if ((pMacAddrFind=(mabMacAddrInfo_t *)SLLFind(&mabBlock->mabMacAddrSLL,( sll_member_t *)&macAddrInfo)) !=  NULLPTR)
   {
       pMacAddrFind->lIntIfNum = lIntIfNum;
      (void) osapiWriteLockGive(mabBlock->mabMacAddrDBRWLock);
       MAB_EVENT_TRACE("\n%s: Found supplicant mac address(%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x) Changed logical Interface to: %d .\n",
               __FUNCTION__, mac_addr->addr[0],mac_addr->addr[1],mac_addr->addr[2],mac_addr->addr[3],mac_addr->addr[4],mac_addr->addr[5],lIntIfNum);
       return  SUCCESS;
   }

   /* Allocate memory from the buffer pool*/
   if (bufferPoolAllocate(mabBlock->mabMacAddrBufferPoolId,
                            ( uchar8 **)&pMacAddrInfo) !=  SUCCESS)
     {
       /* release semaphore*/
       (void)osapiWriteLockGive(mabBlock->mabMacAddrDBRWLock);

        LOGF(  LOG_SEVERITY_NOTICE,
           "\n%s: Could not add supplicant mac address(%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x) logical Interface: %d . Insufficient memory. \n",
               __FUNCTION__, mac_addr->addr[0],mac_addr->addr[1],mac_addr->addr[2],mac_addr->addr[3],mac_addr->addr[4],mac_addr->addr[5],lIntIfNum);

       MAB_EVENT_TRACE("\n%s: Could not add supplicant mac address(%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x) logical Interface: %d . Insufficient memory. \n",
               __FUNCTION__, mac_addr->addr[0],mac_addr->addr[1],mac_addr->addr[2],mac_addr->addr[3],mac_addr->addr[4],mac_addr->addr[5],lIntIfNum);

       return  FAILURE;
     }

   /* copy details into allocated node*/
   memset(pMacAddrInfo,0,sizeof(mabMacAddrInfo_t));
   memcpy(pMacAddrInfo->suppMacAddr.addr,mac_addr->addr, ENET_MAC_ADDR_LEN);
   pMacAddrInfo->lIntIfNum = lIntIfNum;


   /* Add node to the SLL*/
   if (SLLAdd(&mabBlock->mabMacAddrSLL, ( sll_member_t *)pMacAddrInfo)
         !=  SUCCESS)
   {
       MAB_EVENT_TRACE("\n%s: Could not add supplicant mac address(%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x) logical Interface: %d . \n",
               __FUNCTION__, mac_addr->addr[0],mac_addr->addr[1],mac_addr->addr[2],mac_addr->addr[3],mac_addr->addr[4],mac_addr->addr[5],lIntIfNum);
       /* Free the previously allocated bufferpool */
       bufferPoolFree(mabBlock->mabMacAddrBufferPoolId, ( uchar8 *)pMacAddrInfo);
       /* release semaphore*/
       (void)osapiWriteLockGive(mabBlock->mabMacAddrDBRWLock);
       return  FAILURE;
   }

    /* release semaphore*/
    (void)osapiWriteLockGive(mabBlock->mabMacAddrDBRWLock);
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
RC_t mabMacAddrInfoRemove( enetMacAddr_t *mac_addr)
{
   mabMacAddrInfo_t macAddrInfo;
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
   (void)osapiWriteLockTake(mabBlock->mabMacAddrDBRWLock,  WAIT_FOREVER);

    /* delete from SLL*/
    if (SLLDelete(&mabBlock->mabMacAddrSLL, ( sll_member_t *)&macAddrInfo)
                  !=  SUCCESS)
    {
        /* release semaphore*/
       (void)osapiWriteLockGive(mabBlock->mabMacAddrDBRWLock);

        MAB_EVENT_TRACE("\n%s: Could not delete supplicant mac address(%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x) from the SLL . \n",
               __FUNCTION__, mac_addr->addr[0],mac_addr->addr[1],mac_addr->addr[2],mac_addr->addr[3],mac_addr->addr[4],mac_addr->addr[5]);
       return  FAILURE;
    }

  /* release semaphore*/
  (void)osapiWriteLockGive(mabBlock->mabMacAddrDBRWLock);
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
RC_t mabMacAddrInfoFind( enetMacAddr_t *mac_addr,uint32 *lIntIfNum)
{
  mabMacAddrInfo_t macAddrInfo,*pMacAddrInfo;
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
   (void)osapiWriteLockTake(mabBlock->mabMacAddrDBRWLock,  WAIT_FOREVER);

  if ((pMacAddrInfo=(mabMacAddrInfo_t *)SLLFind(&mabBlock->mabMacAddrSLL,( sll_member_t *)&macAddrInfo)) ==  NULLPTR)
  {
      /* release semaphore*/
     (void)osapiWriteLockGive(mabBlock->mabMacAddrDBRWLock);
      MAB_EVENT_TRACE("\n%s: Could not find supplicant mac address(%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x). \n",
               __FUNCTION__, mac_addr->addr[0],mac_addr->addr[1],mac_addr->addr[2],mac_addr->addr[3],mac_addr->addr[4],mac_addr->addr[5]);
      *lIntIfNum = MAB_LOGICAL_PORT_ITERATE;
      return  FAILURE;
  }
  *lIntIfNum = pMacAddrInfo->lIntIfNum;
  /* release semaphore*/
  (void)osapiWriteLockGive(mabBlock->mabMacAddrDBRWLock);
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
RC_t mabMacAddrInfoFindNext( enetMacAddr_t *mac_addr,uint32 *lIntIfNum)
{
  mabMacAddrInfo_t macAddrInfo,*pMacAddrInfo;

  /*input check*/
  if (mac_addr ==  NULLPTR)
  {
      return  FAILURE;
  }

  /* copy info to node*/
  memcpy(macAddrInfo.suppMacAddr.addr,mac_addr->addr, ENET_MAC_ADDR_LEN);

   /* take Mac address DB semaphore*/
   (void)osapiWriteLockTake(mabBlock->mabMacAddrDBRWLock,  WAIT_FOREVER);

  if ((pMacAddrInfo=(mabMacAddrInfo_t *)SLLFindNext(&mabBlock->mabMacAddrSLL,( sll_member_t *)&macAddrInfo)) ==  NULLPTR)
  {
      /* release semaphore*/
      (void)osapiWriteLockGive(mabBlock->mabMacAddrDBRWLock);

      MAB_EVENT_TRACE("\n%s: Could not find next node for supplicant mac address(%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x). \n",
               __FUNCTION__, mac_addr->addr[0],mac_addr->addr[1],mac_addr->addr[2],mac_addr->addr[3],mac_addr->addr[4],mac_addr->addr[5]);
      *lIntIfNum = MAB_LOGICAL_PORT_ITERATE;
      return  FAILURE;
  }
  memcpy(mac_addr->addr,pMacAddrInfo->suppMacAddr.addr, ENET_MAC_ADDR_LEN);
  *lIntIfNum = pMacAddrInfo->lIntIfNum;

  /* release semaphore*/
  (void)osapiWriteLockGive(mabBlock->mabMacAddrDBRWLock);

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
RC_t mabDebugMacAddrDBList()
{
     enetMacAddr_t macAddr,nullMacAddr;
    uint32 lIntIfNum;

    memset(&macAddr,0,sizeof( enetMacAddr_t));

    while(mabMacAddrInfoFindNext(&macAddr,&lIntIfNum)==  SUCCESS)
    {
        SYSAPI_PRINTF("\n Mac Address: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x",
                      macAddr.addr[0],macAddr.addr[1],macAddr.addr[2],macAddr.addr[3],macAddr.addr[4],macAddr.addr[5]);

        SYSAPI_PRINTF("\n Logical Port :%u",lIntIfNum);

    }

     memset(&nullMacAddr,0,sizeof( enetMacAddr_t));
     if (memcmp(macAddr.addr,nullMacAddr.addr, ENET_MAC_ADDR_LEN)!=0)
     {
         if (mabMacAddrInfoFind(&macAddr,&lIntIfNum)== SUCCESS)
         {
             SYSAPI_PRINTF("\n Testing mabMacAddrInfoFind.Found \n");
             SYSAPI_PRINTF("\n Mac Address: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x",
                      macAddr.addr[0],macAddr.addr[1],macAddr.addr[2],macAddr.addr[3],macAddr.addr[4],macAddr.addr[5]);

            SYSAPI_PRINTF("\n Logical Port :%u",lIntIfNum);
         }
     }
    return  SUCCESS;
}
