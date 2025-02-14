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
#include "mab_util.h"
#include "mab_struct.h"
#include "osapi_sem.h"

extern mabBlock_t *mabBlock;

  int32
 mabLogicalPortDbEntryCompare (const void* pData1,
                             const void* pData2,
                             size_t size)
{
  mabLogicalNodeKey_t *pKey1 = (mabLogicalNodeKey_t *) pData1;
  mabLogicalNodeKey_t *pKey2 = (mabLogicalNodeKey_t *) pData2;
  register  int32 retVal = 0;

  if ((pKey1 ==  NULLPTR) || (pKey2 ==  NULLPTR))
  {
    return 1;
  }

  retVal = pKey1->keyNum - pKey2->keyNum;

  return retVal;
}

/*********************************************************************
 * @purpose  Initialize Logical Port Info Database
 *
 * @param    nodeCount    @b{(input)} The number of nodes to be created. 
 *
 * @returns   SUCCESS or  FAILURE
 *
 * @comments none
 *       
 * @end
 *********************************************************************/
RC_t mabLogicalPortInfoDBInit(uint32 nodeCount)
{
  /* Allocate the Heap structures */
  mabBlock->mabLogicalPortTreeHeap = (avlTreeTables_t *)osapiMalloc( MAB_COMPONENT_ID,
      nodeCount * sizeof(avlTreeTables_t));

  mabBlock->mabLogicalPortDataHeap  = (mabLogicalPortInfo_t *)osapiMalloc( MAB_COMPONENT_ID,
      nodeCount * sizeof(mabLogicalPortInfo_t));

  /* validate the pointers*/
  if ((mabBlock->mabLogicalPortTreeHeap ==  NULLPTR) ||(mabBlock->mabLogicalPortDataHeap ==  NULLPTR))
  {
     LOGF( LOG_SEVERITY_NOTICE,
        " Error in allocating memory for the MAB database. Possible causes are insufficient memory.");
    return  FAILURE;
  }

  /* AVL Tree creations - mabLogicalPortTreeDb*/
  avlCreateAvlTree(&(mabBlock->mabLogicalPortTreeDb),  mabBlock->mabLogicalPortTreeHeap,
      mabBlock->mabLogicalPortDataHeap, nodeCount,
      sizeof(mabLogicalPortInfo_t), 0x10,
      sizeof(mabLogicalNodeKey_t));

  avlSetAvlTreeComparator(&(mabBlock->mabLogicalPortTreeDb), mabLogicalPortDbEntryCompare);
  return  SUCCESS;
}


/*********************************************************************
 * @purpose  DeInitialize Logical Port Info Database
 *
 * @param    none
 *
 * @returns   SUCCESS or  FAILURE
 *
 * @comments none
 *
 * @end
 *********************************************************************/
RC_t mabLogicalPortInfoDBDeInit(void)
{
  /* Destroy the AVL Tree */
  if(mabBlock->mabLogicalPortTreeDb.semId !=  NULLPTR)
  {
    avlDeleteAvlTree(&mabBlock->mabLogicalPortTreeDb);
  }

  /* Give up the memory */
  if (mabBlock->mabLogicalPortTreeHeap !=  NULLPTR)
  {
    osapiFree( MAB_COMPONENT_ID, mabBlock->mabLogicalPortTreeHeap);
    mabBlock->mabLogicalPortTreeHeap =  NULLPTR;
  }

  if (mabBlock->mabLogicalPortDataHeap !=  NULLPTR)
  {
    osapiFree( MAB_COMPONENT_ID, mabBlock->mabLogicalPortDataHeap);
    mabBlock->mabLogicalPortDataHeap =  NULLPTR;
  }
  return  SUCCESS;
}

/*********************************************************************
 * @purpose  To Take lock for the Logical Port Info Node
 *
 * @param    None
 *
 * @returns   SUCCESS or  FAILURE
 *
 * @comments This lock needs to be taken only the API functions not running in 
 *           the mab threads context.
 *       
 * @end
 *********************************************************************/
RC_t mabLogicalPortInfoTakeLock(void)
{
  return osapiSemaTake(mabBlock->mabLogicalPortTreeDb.semId,  WAIT_FOREVER);
}

/*********************************************************************
 * @purpose  To Giveup lock for the Logical Port Info Node
 *
 * @param    None
 *
 * @returns   SUCCESS or  FAILURE
 *
 * @comments This lock needs to be taken only the API functions not running in 
 *           the mab threads context.
 *       
 * @end
 *********************************************************************/
RC_t mabLogicalPortInfoGiveLock(void)
{
  return osapiSemaGive(mabBlock->mabLogicalPortTreeDb.semId);
}


/*********************************************************************
 * @purpose  To allocate a Logical Port Info Node
 *
 * @param    intIfNum  @b{(input)} The internal interface for which this 
 *                                 logical interface is being created
 *
 * @returns  Logical Internal Interface Number
 *
 * @comments none
 *       
 * @end
 *********************************************************************/
mabLogicalPortInfo_t *mabDynamicLogicalPortInfoAlloc(uint32 intIfNum)
{
  uint32               lIntIfNum;
  mabLogicalPortInfo_t  newNode,*retNode,*tmpNode;
   uchar8 ifName[ NIM_IF_ALIAS_SIZE + 1];
  nimGetIntfName(intIfNum,  ALIASNAME, ifName);
  mabLogicalNodeKey_t key;

  /* iterate through the Logical interfaces to assign a empty slot to the new node.*/
  for (lIntIfNum = MAB_LOGICAL_PORT_START;
      lIntIfNum < MAB_LOGICAL_PORT_END; lIntIfNum++)
  {
    memset(&key, 0, sizeof(mabLogicalNodeKey_t));
    MAB_LPORT_KEY_PACK(intIfNum, lIntIfNum, AUTHMGR_LOGICAL, key.keyNum);
    
    tmpNode = mabLogicalPortInfoGet(key.keyNum);
    if (tmpNode ==  NULLPTR)
    {
      /* found one - use it */
      memset(&newNode,0,sizeof(mabLogicalPortInfo_t));
      newNode.key = key;

      /* add the node to the tree */
      osapiSemaTake(mabBlock->mabLogicalPortTreeDb.semId,  WAIT_FOREVER);
      retNode  = avlInsertEntry(&mabBlock->mabLogicalPortTreeDb,&newNode);
      osapiSemaGive(mabBlock->mabLogicalPortTreeDb.semId);
      if (retNode == &newNode)
      {
         LOGF( LOG_SEVERITY_INFO,
            "Error in adding the node to the MAB tree for interface %s.\n",
            ifName);
        return  NULLPTR;
      }
      return mabLogicalPortInfoGet(key.keyNum);
    }
  }
   LOGF( LOG_SEVERITY_NOTICE,
      "Error in allocating node for interface %s,as it reached maximum limit per port."
      " Could not allocate memory for client as maximum number of clients allowed per port"
      " has been reached.", ifName);
  return  NULLPTR;
}

/*********************************************************************
 * @purpose  To allocate a Logical Port Info Node
 *
 * @param    intIfNum  @b{(input)} The internal interface for which this 
 *                                 logical interface is being created
 *
 * @returns  Logical Internal Interface Number
 *
 * @comments none
 *       
 * @end
 *********************************************************************/
mabLogicalPortInfo_t *mabLogicalPortInfoAlloc(uint32 intIfNum)
{
  return mabDynamicLogicalPortInfoAlloc(intIfNum);
}

/*********************************************************************
 * @purpose  To Deallocate a Logical Port Info Node
 *
 * @param    intIfNum  @b{(input)} The internal interface for which this 
 *                                 logical interface is being destroyed
 *
 * @returns   SUCCESS or  FAILURE
 *
 * @comments none
 *       
 * @end
 *********************************************************************/
RC_t mabLogicalPortInfoDeAlloc(mabLogicalPortInfo_t *node)
{
  uint32 physPort = 0, lPort = 0, type = 0;
  if(node !=  NULLPTR)
  {
    MAB_LPORT_KEY_UNPACK(physPort, lPort, type, node->key.keyNum);

    if (AUTHMGR_LOGICAL == type)
    {
    osapiSemaTake(mabBlock->mabLogicalPortTreeDb.semId,  WAIT_FOREVER);
    avlDeleteEntry(&mabBlock->mabLogicalPortTreeDb,node);
    osapiSemaGive(mabBlock->mabLogicalPortTreeDb.semId);
    }
    return  SUCCESS;
  }
  return  FAILURE;
}

/*********************************************************************
 * @purpose  To Get a Logical Port Info Node
 *
 * @param    lIntIfNum  @b{(input)} The logical internal interface number
 *
 * @returns  Logical Internal Interface node
 *
 * @comments none
 *       
 * @end
 *********************************************************************/
mabLogicalPortInfo_t *mabLogicalPortInfoGet(uint32  lIntIfNum)
{
  mabLogicalNodeKey_t key;
  uint32 physPort = 0, lPort = 0, type = 0;
  mabLogicalPortInfo_t  *entry =  NULLPTR;

  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, lIntIfNum);

  key.keyNum = lIntIfNum;

  if (AUTHMGR_LOGICAL == type)
  {
    entry = (mabLogicalPortInfo_t *)avlSearch(&mabBlock->mabLogicalPortTreeDb, &key, AVL_EXACT);
  }
  return entry;
}

/*********************************************************************
 * @purpose  To Get Next Logical Port Info Node
 *
 * @param    lIntIfNum  @b{(input)} The logical internal interface number
 *
 * @returns  Logical Internal Interface node
 *
 * @comments none
 *       
 * @end
 *********************************************************************/
mabLogicalPortInfo_t *mabLogicalPortInfoGetNext(uint32 lIntIfNum)
{
  uint32 physPort = 0, lPort = 0, type = 0;
  mabLogicalPortInfo_t  *entry =  NULLPTR;
  RC_t nimRc =  SUCCESS;

  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, lIntIfNum);

  while (( SUCCESS == nimRc) && 
    ( NULLPTR == (entry = mabLogicalPortInfoGetNextNode(physPort,&lIntIfNum))))
  {
    /* Entry not available on this port.
       search on the next port */
    lIntIfNum = MAB_LOGICAL_PORT_ITERATE;
    nimRc = mabNextValidIntf(physPort, &physPort);
  }

  return entry;
}

/*********************************************************************
 * @purpose  To get First logical interfaces for dynamically allocated nodes
 *
 * @param    intIfNum  @b{(input)} The internal interface 
 * @param    lIntIfNum  @b{(input/output)} The logical internal interface number
 *
 * @returns  Logical Internal Interface node
 *
 * @comments For the first iteration start with zero.
 *       
 * @end
 *********************************************************************/
mabLogicalPortInfo_t *mabDynamicLogicalPortInfoFirstGet(uint32 intIfNum,
    uint32 *lIntIfNum)
{
  mabLogicalPortInfo_t *node;
  uint32 maxintf = MAB_LOGICAL_PORT_END;
  uint32 lPort = 0;
  uint32 temp = 0;


  lPort = MAB_LOGICAL_PORT_START;
  MAB_LPORT_KEY_PACK(intIfNum, lPort, AUTHMGR_LOGICAL, temp);
  while((node=mabLogicalPortInfoGet(temp))== NULLPTR &&
      lPort < maxintf)
  {
    lPort = lPort + 1;
    temp = 0;
    MAB_LPORT_KEY_PACK(intIfNum, lPort, AUTHMGR_LOGICAL, temp);
  }

  *lIntIfNum = temp;
  return node;
}

/*********************************************************************
 * @purpose  To iterate all the logical interfaces of a physical interface
 *
 * @param    intIfNum  @b{(input)} The internal interface 
 * @param    lIntIfNum  @b{(input/output)} The logical internal interface number
 *
 * @returns  Logical Internal Interface node
 *
 * @comments For the first iteration start with zero.
 *       
 * @end
 *********************************************************************/
mabLogicalPortInfo_t *mabDynamicLogicalPortInfoGetNextNode(uint32 intIfNum,
    uint32 *lIntIfNum)
{
  mabLogicalPortInfo_t *node =  NULLPTR;
  uint32 physPort = 0, lPort = 0, type = 0, temp = 0;

  if(*lIntIfNum == MAB_LOGICAL_PORT_ITERATE)
  {
    return mabLogicalPortInfoFirstGet(intIfNum,lIntIfNum);
  }
  
  MAB_LPORT_KEY_UNPACK(physPort, lPort, type, *lIntIfNum);

  if ((intIfNum != physPort) || (lPort >= MAB_LOGICAL_PORT_END))
  {
    return  NULLPTR;
  }

  lPort = lPort + 1;
  MAB_LPORT_KEY_PACK(intIfNum, lPort, AUTHMGR_LOGICAL, temp);
  while((node=mabLogicalPortInfoGet(temp))== NULLPTR &&
      lPort < MAB_LOGICAL_PORT_END)
  {
    lPort = lPort + 1;
    temp = 0;
    MAB_LPORT_KEY_PACK(intIfNum, lPort, AUTHMGR_LOGICAL, temp);
  }

  *lIntIfNum = temp;
  return node;
}

/*********************************************************************
 * @purpose  To get First logical interfaces for a physical interface
 *
 * @param    intIfNum  @b{(input)} The internal interface 
 * @param    lIntIfNum  @b{(input/output)} The logical internal interface number
 *
 * @returns  Logical Internal Interface node
 *
 * @comments For the first iteration start with zero.
 *       
 * @end
 *********************************************************************/
mabLogicalPortInfo_t *mabLogicalPortInfoFirstGet(uint32 intIfNum,
    uint32 *lIntIfNum)
{
  mabLogicalPortInfo_t *node;

  /* dynamically allocated node */
  node = mabDynamicLogicalPortInfoFirstGet(intIfNum, lIntIfNum);

  return node;
}

/*********************************************************************
 * @purpose  To iterate all the logical interfaces of a physical interface
 *
 * @param    intIfNum  @b{(input)} The internal interface 
 * @param    lIntIfNum  @b{(input/output)} The logical internal interface number
 *
 * @returns  Logical Internal Interface node
 *
 * @comments For the first iteration start with zero.
 *       
 * @end
 *********************************************************************/
mabLogicalPortInfo_t *mabLogicalPortInfoGetNextNode(uint32 intIfNum,
    uint32 *lIntIfNum)
{
  mabLogicalPortInfo_t *node;

  /* dynamically allocated node */
  node = mabDynamicLogicalPortInfoGetNextNode(intIfNum, lIntIfNum);

  return node;
}


