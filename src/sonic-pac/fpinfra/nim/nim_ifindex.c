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
#include "nim_ifindex.h"
#include "avl_api.h"
#include "tree_api.h"
#include "nim_util.h" /* needed for NIM_LOG_MSG */
#include "osapi_sem.h"
#include <string.h>

static avlTree_t              nimIfIndexTreeData  = { 0 };
static void                   *nimIfIndexSema     =  NULL;

/* Macros for protecting the AVL tree during operations */
#define NIM_IFINDEX_CRIT_SEC_ENTER()  \
{   \
    osapiSemaTake(nimIfIndexSema, WAIT_FOREVER);  \
}

#define NIM_IFINDEX_CRIT_SEC_EXIT()  \
{   \
    osapiSemaGive(nimIfIndexSema);  \
}

/*********************************************************************
* @purpose  Create an IfIndex
*
* @param    usp         @b{(input)}   The usp to encode in the ifIndex
* @param    ifIndex     @b{(output)}  The ifIndex created
*
* @returns   SUCCESS or  FAILURE
*
* @notes   
*
*       
* @end
*
*********************************************************************/
void nimIfIndexCreate(nimUSP_t usp, INTF_TYPES_t type,uint32 *ifIndex, uint32 intIfNum)
{
  RC_t rc =  SUCCESS;
  nimIfIndexTreeData_t data;
  nimIfIndexTreeData_t *pData;

  if (ifIndex ==  NULLPTR)
  {
    NIM_LOG_MSG("NIM: Attempted dereferencing of NULL\n");
  }
  else
  {
    switch (type)
    {
      case  PHYSICAL_INTF:
      case  STACK_INTF:
      case  CPU_INTF:
        break;
      case  LAG_INTF:
      case  LOGICAL_VLAN_INTF:
      case  LOOPBACK_INTF:
      case  TUNNEL_INTF:
      case  SERVICE_PORT_INTF:
        usp.unit =  LOGICAL_UNIT;
        break;
      default:
        rc =  FAILURE;
        break;
    }

    if (rc ==  SUCCESS)
    {
      /* *ifIndex = (usp.unit << NIM_UNIT_SHIFT) | (usp.slot << NIM_SLOT_SHIFT) | (usp.port << NIM_PORT_SHIFT); */
      *ifIndex = intIfNum;

      data.ifIndex = *ifIndex;
      data.intIfNum = intIfNum;

      NIM_IFINDEX_CRIT_SEC_ENTER();

      pData = avlInsertEntry(&nimIfIndexTreeData, &data);

      NIM_IFINDEX_CRIT_SEC_EXIT();

      if (pData !=  NULL)
      {
        NIM_LOG_MSG("NIM: ifIndex not added to the AVL tree\n");
      }
    }
  }

  return;
}

/*********************************************************************
* @purpose  Delete an IfIndex
*
* @param    ifIndex     @b{(output)}  The ifIndex to delete
*
* @returns   SUCCESS or  FAILURE
*
* @notes   
*
*       
* @end
*
*********************************************************************/
void nimIfIndexDelete(uint32 ifIndex)
{
  nimIfIndexTreeData_t *pData;
  nimIfIndexTreeData_t data;

  data.ifIndex = ifIndex;

  NIM_IFINDEX_CRIT_SEC_ENTER();

  pData = avlDeleteEntry(&nimIfIndexTreeData, &data);

  NIM_IFINDEX_CRIT_SEC_EXIT();

  if (pData ==  NULL)
  {
    NIM_LOG_MSG("NIM: ifIndex %d not found, cannot delete it\n",ifIndex);
  }

  return;
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
RC_t nimIfIndexPhaseOneInit()
{
  RC_t   rc =  SUCCESS;

  do
  {

    if (avlAllocAndCreateAvlTree(&nimIfIndexTreeData,
                                  NIM_COMPONENT_ID,
                                 platIntfTotalMaxCountGet(),
                                 sizeof(nimIfIndexTreeData_t),
                                 0x10, avlCompareULong32,
                                 sizeof(uint32)) !=  SUCCESS)
    {
      NIM_LOG_ERROR("NIM: Unable to allocate resources\n");
      break; /* goto while */
    }

    nimIfIndexSema = osapiSemaMCreate(OSAPI_SEM_Q_PRIORITY); 

    if (nimIfIndexSema ==  NULLPTR)
    {
      NIM_LOG_ERROR("NIM: unable to create the ifIndex Sema\n");
    }

  } while ( 0 );

  return rc;
}

/*********************************************************************
* @purpose  Find the next external inferface number
*
* @param    extIfNum      @b{(input)}  The present extIfNum
* @param    pNextExtIfNum @b{(output)} The next extIfNum
* @param    pIntIfNum     @b{(output)} The internal interface number
*
* @returns   ERROR if the extIfNum doesn't exist
*
* @notes    none 
*
* @end
*********************************************************************/
RC_t nimIfIndexNextGet(uint32 extIfNum, uint32 *pNextExtIfNum,uint32 *pIntIfNum)
{
  RC_t rc =  SUCCESS;
  nimIfIndexTreeData_t *pData;

  NIM_IFINDEX_CRIT_SEC_ENTER();

  pData = avlSearch (&nimIfIndexTreeData, &extIfNum, AVL_NEXT);

  NIM_IFINDEX_CRIT_SEC_EXIT();

  if (pData !=  NULL)
  {
    *pNextExtIfNum = pData->ifIndex;
    *pIntIfNum = pData->intIfNum;
  }
  else
  {
    rc =  ERROR;
  }
  return rc;
}


/*********************************************************************
* @purpose  Find the first external inferface number
*
* @param    pExtIfNum      @b{(input)}  The present extIfNum
* @param    pIntIfNum      @b{(output)} The internal interface number
*
* @returns   ERROR if the extIfNum doesn't exist
*
* @notes    none 
*
* @end
*********************************************************************/
RC_t nimIfIndexFirstGet(uint32 *pExtIfNum,uint32 *pIntIfNum)
{
  RC_t rc =  SUCCESS;
  nimIfIndexTreeData_t *pData;
  uint32 ifIndex = 0;

  NIM_IFINDEX_CRIT_SEC_ENTER();

  pData = avlSearch (&nimIfIndexTreeData, &ifIndex, AVL_NEXT);

  NIM_IFINDEX_CRIT_SEC_EXIT();

  if (pData !=  NULL)
  {
    *pExtIfNum = pData->ifIndex;
    *pIntIfNum = pData->intIfNum;

  }
  else
  {
    rc =  ERROR;
  }
  return rc;
}

/*********************************************************************
* @purpose  Find the intIfNum associated with the extIfNum 
*
* @param    extIfNum      @b{(input)}  The present extIfNum
* @param    pNextExtIfNum  @b{(output)} The next extIfNum
*
* @returns   ERROR if the extIfNum doesn't exist
*
* @notes    none 
*
* @end
*********************************************************************/
RC_t nimIfIndexIntIfNumGet(uint32 extIfNum, uint32 *pIntIfNum)
{
  RC_t rc =  SUCCESS;
  nimIfIndexTreeData_t *pData;

  NIM_IFINDEX_CRIT_SEC_ENTER();

  pData = avlSearch (&nimIfIndexTreeData, &extIfNum, AVL_EXACT);

  NIM_IFINDEX_CRIT_SEC_EXIT();

  if (pData !=  NULL)
  {
    *pIntIfNum= pData->intIfNum;
  }
  else
  {
    rc =  ERROR;
  }
  return rc;
}




/*********************************************************************
* @purpose  Find the intIfNum associated with the extIfNum 
*
* @param    extIfNum      @b{(input)}  The present extIfNum
* @param    pNextExtIfNum  @b{(output)} The next extIfNum
*
* @returns   ERROR if the extIfNum doesn't exist
*
* @notes    none 
*
* @end
*********************************************************************/
void nimIfIndexDataClear(void)
{

  NIM_IFINDEX_CRIT_SEC_ENTER();

  avlPurgeAvlTree(&nimIfIndexTreeData, platIntfTotalMaxCountGet());

  NIM_IFINDEX_CRIT_SEC_EXIT();
}


