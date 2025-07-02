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
#include "auth_mgr_util.h"
#include "auth_mgr_struct.h"
#include "osapi_sem.h"
#include "pacoper_common.h"

extern authmgrCB_t *authmgrCB;

//static RC_t authmgrAuthHistoryLogCreateEntryIndex (uint32 * entryIndex);

/****************************************?*****************************
*
* @purpose   Compare function for the authmgr history db Entry Tree
*
* @param     pData1 @b{ (input) } Pointer to authmgr Entry Key1
*            pData2 @b{ (input) } Pointer to authmgr Entry Key2
*            size   @b{ (input) } Size for the comparision
*
* @returns   > 0 if pData1 > pData2
*            = 0 if pData1 == pData2
*            < 0 if pData1 < pData2
*
* @comments  None
*
* @end
*****************************************?****************************/
 int32
authmgrLogicalPortDbEntryCompare (const void *pData1,
                                  const void *pData2, size_t size)
{
  const authmgrLogicalNodeKey_t *pKey1 = (const authmgrLogicalNodeKey_t *) pData1;
  const authmgrLogicalNodeKey_t *pKey2 = (const authmgrLogicalNodeKey_t *) pData2;
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
RC_t authmgrLogicalPortInfoDBInit (uint32 nodeCount)
{
  /* Allocate the Heap structures */
  authmgrCB->globalInfo->authmgrLogicalPortTreeHeap =
    (avlTreeTables_t *) osapiMalloc ( AUTHMGR_COMPONENT_ID,
                                     nodeCount * sizeof (avlTreeTables_t));

  authmgrCB->globalInfo->authmgrLogicalPortDataHeap =
    (authmgrLogicalPortInfo_t *) osapiMalloc ( AUTHMGR_COMPONENT_ID,
                                              nodeCount *
                                              sizeof
                                              (authmgrLogicalPortInfo_t));

  /* validate the pointers */
  if ((authmgrCB->globalInfo->authmgrLogicalPortTreeHeap ==  NULLPTR)
      || (authmgrCB->globalInfo->authmgrLogicalPortDataHeap ==  NULLPTR))
  {
     LOGF ( LOG_SEVERITY_NOTICE,
             " Error in allocating memory for the AUTHMGR database. Possible causes are insufficient memory.");
    return  FAILURE;
  }

  /* AVL Tree creations - authmgrLogicalPortTreeDb */
  avlCreateAvlTree (&(authmgrCB->globalInfo->authmgrLogicalPortTreeDb),
                    authmgrCB->globalInfo->authmgrLogicalPortTreeHeap,
                    authmgrCB->globalInfo->authmgrLogicalPortDataHeap,
                    nodeCount, sizeof (authmgrLogicalPortInfo_t), 0x10,
                    sizeof (authmgrLogicalNodeKey_t));

  avlSetAvlTreeComparator (&(authmgrCB->globalInfo->authmgrLogicalPortTreeDb),
                           authmgrLogicalPortDbEntryCompare);
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
RC_t authmgrLogicalPortInfoDBDeInit (void)
{
  /* Destroy the AVL Tree */
  if (authmgrCB->globalInfo->authmgrLogicalPortTreeDb.semId !=  NULLPTR)
  {
    avlDeleteAvlTree (&authmgrCB->globalInfo->authmgrLogicalPortTreeDb);
  }

  /* Give up the memory */
  if (authmgrCB->globalInfo->authmgrLogicalPortTreeHeap !=  NULLPTR)
  {
    osapiFree ( AUTHMGR_COMPONENT_ID,
               authmgrCB->globalInfo->authmgrLogicalPortTreeHeap);
    authmgrCB->globalInfo->authmgrLogicalPortTreeHeap =  NULLPTR;
  }

  if (authmgrCB->globalInfo->authmgrLogicalPortDataHeap !=  NULLPTR)
  {
    osapiFree ( AUTHMGR_COMPONENT_ID,
               authmgrCB->globalInfo->authmgrLogicalPortDataHeap);
    authmgrCB->globalInfo->authmgrLogicalPortDataHeap =  NULLPTR;
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
*           the authmgr threads context.
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortInfoTakeLock (void)
{
  return osapiSemaTake (authmgrCB->globalInfo->authmgrLogicalPortTreeDb.semId,
                         WAIT_FOREVER);
}

/*********************************************************************
* @purpose  To Giveup lock for the Logical Port Info Node
*
* @param    None
*
* @returns   SUCCESS or  FAILURE
*
* @comments This lock needs to be taken only the API functions not running in
*           the authmgr threads context.
*
* @end
*********************************************************************/
RC_t authmgrLogicalPortInfoGiveLock (void)
{
  return osapiSemaGive (authmgrCB->globalInfo->authmgrLogicalPortTreeDb.semId);
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
authmgrLogicalPortInfo_t *authmgrDynamicLogicalPortInfoAlloc (uint32
                                                              intIfNum)
{
  uint32 lIntIfNum;
  authmgrLogicalPortInfo_t newNode, *retNode, *tmpNode;
  authmgrLogicalNodeKey_t key;

  /* iterate through the Logical interfaces to assign a empty slot to the new
     node. */
  for (lIntIfNum = AUTHMGR_LOGICAL_PORT_START;
       lIntIfNum < AUTHMGR_LOGICAL_PORT_END; lIntIfNum++)
  {
    memset (&key, 0, sizeof (authmgrLogicalNodeKey_t));
    AUTHMGR_LPORT_KEY_PACK (intIfNum, lIntIfNum, AUTHMGR_LOGICAL, key.keyNum);

    tmpNode = authmgrLogicalPortInfoGet (key.keyNum);
    if (tmpNode ==  NULLPTR)
    {
      /* found one - use it */
      memset (&newNode, 0, sizeof (authmgrLogicalPortInfo_t));
      newNode.key = key;

      /* add the node to the tree */
      osapiSemaTake (authmgrCB->globalInfo->authmgrLogicalPortTreeDb.semId,
                      WAIT_FOREVER);
      retNode =
        avlInsertEntry (&authmgrCB->globalInfo->authmgrLogicalPortTreeDb,
                        &newNode);
      osapiSemaGive (authmgrCB->globalInfo->authmgrLogicalPortTreeDb.semId);
      if (retNode == &newNode)
      {
         LOGF ( LOG_SEVERITY_INFO,
                 "Error in adding the node to the AUTHMGR tree for interface %s.\n",
                 authmgrIntfIfNameGet(intIfNum));
        return  NULLPTR;
      }
      return authmgrLogicalPortInfoGet (key.keyNum);
    }
    else
    {
#if 0
      /* flush stale data. */
      memset (tmpNode, 0, sizeof (authmgrLogicalPortInfo_t));
      tmpNode->key = key;

      return tmpNode;
#endif
    }
  }
   LOGF ( LOG_SEVERITY_NOTICE,
           "Error in allocating node for interface %s,as it reached maximum limit per port."
           " Could not allocate memory for client as maximum number of clients allowed per port"
           " has been reached.", authmgrIntfIfNameGet(intIfNum));
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
authmgrLogicalPortInfo_t *authmgrLogicalPortInfoAlloc (uint32 intIfNum)
{
   BOOL valid =  FALSE;
  authmgrHostIsDynamicNodeAllocCheck (authmgrCB->globalInfo->
                                      authmgrPortInfo[intIfNum].hostMode,
                                      &valid);

  if ( TRUE == valid)
  {
    return authmgrDynamicLogicalPortInfoAlloc (intIfNum);
  }
  return  NULLPTR;
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
RC_t authmgrLogicalPortInfoDeAlloc (authmgrLogicalPortInfo_t * node)
{
  uint32 physPort = 0, lPort = 0, type = 0;
  if (node !=  NULLPTR)
  {
    AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, node->key.keyNum);
  
    if (AUTHMGR_LOGICAL == type)
    {
      osapiSemaTake (authmgrCB->globalInfo->authmgrLogicalPortTreeDb.semId,
                      WAIT_FOREVER);
      avlDeleteEntry (&authmgrCB->globalInfo->authmgrLogicalPortTreeDb, node);
      osapiSemaGive (authmgrCB->globalInfo->authmgrLogicalPortTreeDb.semId);
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
authmgrLogicalPortInfo_t *authmgrLogicalPortInfoGet (uint32 lIntIfNum)
{
  authmgrLogicalNodeKey_t key;
  uint32 physPort = 0, lPort = 0, type = 0;
  authmgrLogicalPortInfo_t *entry =  NULLPTR;

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, lIntIfNum);

  key.keyNum = lIntIfNum;

  if (AUTHMGR_LOGICAL == type)
  {
    entry =
      (authmgrLogicalPortInfo_t *) avlSearch (&authmgrCB->globalInfo->
                                                  authmgrLogicalPortTreeDb,
                                                  &key, AVL_EXACT);
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
authmgrLogicalPortInfo_t *authmgrLogicalPortInfoGetNext (uint32 lIntIfNum)
{
  uint32 physPort = 0, lPort = 0, type = 0;
  authmgrLogicalPortInfo_t *entry =  NULLPTR;
  RC_t nimRc =  SUCCESS;

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, lIntIfNum);

  while (( SUCCESS == nimRc) &&
         ( NULLPTR ==
          (entry = authmgrLogicalPortInfoGetNextNode (physPort, &lIntIfNum))))
  {
    /* Entry not available on this port.
       search on the next port */
    lIntIfNum = AUTHMGR_LOGICAL_PORT_ITERATE;
    nimRc = authmgrNextValidIntf (physPort, &physPort);
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
authmgrLogicalPortInfo_t *authmgrDynamicLogicalPortInfoFirstGet (uint32
                                                                 intIfNum,
                                                                 uint32 *
                                                                 lIntIfNum)
{
  authmgrLogicalPortInfo_t *node;
  uint32 maxintf = AUTHMGR_LOGICAL_PORT_END;
  uint32 lPort = 0;
  uint32 temp = 0;

  lPort = AUTHMGR_LOGICAL_PORT_START;
  AUTHMGR_LPORT_KEY_PACK (intIfNum, lPort, AUTHMGR_LOGICAL, temp);
  while ((node = authmgrLogicalPortInfoGet (temp)) ==  NULLPTR &&
         lPort < maxintf)
  {
    lPort = lPort + 1;
    temp = 0;
    AUTHMGR_LPORT_KEY_PACK (intIfNum, lPort, AUTHMGR_LOGICAL, temp);
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
authmgrLogicalPortInfo_t *authmgrDynamicLogicalPortInfoGetNextNode (uint32
                                                                    intIfNum,
                                                                    uint32 *
                                                                    lIntIfNum)
{
  authmgrLogicalPortInfo_t *node =  NULLPTR;
  uint32 physPort = 0, lPort = 0, type = 0, temp = 0;

  if (*lIntIfNum == AUTHMGR_LOGICAL_PORT_ITERATE)
  {
    return authmgrLogicalPortInfoFirstGet (intIfNum, lIntIfNum);
  }

  AUTHMGR_LPORT_KEY_UNPACK (physPort, lPort, type, *lIntIfNum);
  
  if ((intIfNum != physPort) || (lPort >= AUTHMGR_LOGICAL_PORT_END))
  {
    return  NULLPTR;
  }

  lPort = lPort + 1;
  AUTHMGR_LPORT_KEY_PACK (intIfNum, lPort, AUTHMGR_LOGICAL, temp);
  while ((node = authmgrLogicalPortInfoGet (temp)) ==  NULLPTR &&
         lPort < AUTHMGR_LOGICAL_PORT_END)
  {
    lPort = lPort + 1;
    temp = 0;
    AUTHMGR_LPORT_KEY_PACK (intIfNum, lPort, AUTHMGR_LOGICAL, temp);
  }

  *lIntIfNum = temp;
  return node;
}

/*********************************************************************
* @purpose  Debug Info of the Logical Port DB
*
* @param    None
*
* @returns  None
*
* @comments none
*
* @end
*********************************************************************/
void authmgrDebugLogicalPortInfo (void)
{
  if (authmgrCB->globalInfo->authmgrLogicalPortTreeHeap !=  NULLPTR)
  {
    SYSAPI_PRINTF ("The Authmgr Logical Port Info has %d Nodes\n",
                   authmgrCB->globalInfo->authmgrLogicalPortTreeDb.count);
  }
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
authmgrLogicalPortInfo_t *authmgrLogicalPortInfoFirstGet (uint32 intIfNum,
                                                          uint32 * lIntIfNum)
{
  authmgrLogicalPortInfo_t *node;
   BOOL valid =  FALSE;

  authmgrHostIsDynamicNodeAllocCheck (authmgrCB->globalInfo->
                                      authmgrPortInfo[intIfNum].hostMode,
                                      &valid);

  if ( TRUE == valid)
  {
    /* dynamically allocated node */
    node = authmgrDynamicLogicalPortInfoFirstGet (intIfNum, lIntIfNum);
  }
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
authmgrLogicalPortInfo_t *authmgrLogicalPortInfoGetNextNode (uint32 intIfNum,
                                                             uint32 *
                                                             lIntIfNum)
{
  authmgrLogicalPortInfo_t *node;
   BOOL valid =  FALSE;

  authmgrHostIsDynamicNodeAllocCheck (authmgrCB->globalInfo->
                                      authmgrPortInfo[intIfNum].hostMode,
                                      &valid);

  if ( TRUE == valid)
  {
    /* dynamically allocated node */
    node = authmgrDynamicLogicalPortInfoGetNextNode (intIfNum, lIntIfNum);
  }
  return node;
}

