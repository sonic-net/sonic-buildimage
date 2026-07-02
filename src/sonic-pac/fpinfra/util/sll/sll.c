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


/*******************************************************************************
**                             Includes                                       **
*******************************************************************************/
#include "datatypes.h"
#include "commdefs.h"
#include "sll_api.h"
#include "osapi.h"
#include "osapi_sem.h"
#include <string.h>
#include <stdio.h>
/*******************************************************************************
**                        Static Function Declaration                         **
*******************************************************************************/

static RC_t SLLSeek( sll_t *list,
                        sll_member_t *node,
                        sll_member_t **prev);

static void SLLNodeInsert( sll_t *list,
                           sll_member_t *node,
                           sll_member_t *prev);

static  sll_member_t* SLLNodeExtract( sll_t *list,
                                        sll_member_t *prev);

static  int32 SLLDefaultFuncCompare(void *data1,
                                      void *data2,
                                      uint32 keyLen);

static RC_t SLLDefaultFuncDestroy( COMPONENT_IDS_t compId,  sll_member_t *node);


/*******************************************************************************
**                        Function Definitions                                **
*******************************************************************************/

/*********************************************************************
*
* @purpose  Creation of a Single Linked List
*
* @param    compId   @b{(input)}ID of the component creating the list
* @param    sortType @b{(input)}Specifies the sort order for the list.
* @param    keySize  @b{(input)}It is the size of the index(key) for each node in the list
* @param    compFunc @b{(input)}A function pointer that would be used for comparision
*                               between the nodes.
* @param    desFunc  @b{(input)}A function pointer that would be used for the deletion of
*                               node.
* @param    list     @b{(input)}The output pointer to the linked list that is created.
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if failure
*
* @notes    This API is provided to create a linked list, if the comparision
*           function and destroy function are not passed then the default
*           implementations are used. Make sure to give correct key length.
*           Make Sure that the return value of compare functions is similar to
*           the memcmp() function.
*
* @end
*
*********************************************************************/
RC_t SLLCreate( COMPONENT_IDS_t compId,
                   SLL_SORT_TYPE   sortType,
                  uint32          keySize,
                   sllCompareFunc  compFunc,
                   sllDestroyFunc  desFunc,
                   sll_t           *list)
{
  /* Some basic validations */
  if(list ==  NULLPTR)
    return  FAILURE;

  switch (sortType)
  {
    case  SLL_NO_ORDER:
    case  SLL_ASCEND_ORDER:
    case  SLL_DESCEND_ORDER:
      break;
    default:
      return  FAILURE;
  }

  memset(list,0,sizeof( sll_t));
  /* assign the parameters */
  list->sllSortType    = sortType;
  list->sllDupEnable   =  FALSE;
  list->sllKeySize     = keySize;
  list->sllCompId      = compId;
  list->sllCompareFunc = (compFunc)?compFunc:SLLDefaultFuncCompare;
  list->sllDestroyFunc = desFunc;
  list->semId = osapiSemaBCreate (OSAPI_SEM_Q_PRIORITY, OSAPI_SEM_FULL);  
  list->inUse =  TRUE;
  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  Set/Reset flags on the Single Linked List
*
* @param    list      @b{(input)}The list to modify the flags on.
* @param    flagType  @b{(input)}Flag Identifier.
* @param    flagVal   @b{(input)}Value associated with the flag.
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if failure
*
* @notes    This API is provided to change the default behavior of a linked list.
*           Any behaviorial changes do not alter the already present nodes, but
*           apply only to future list operations. Due to this, it is highly adviced
*           that this call follows the creation call immediately and before
*           any other operation is performed on the list.
*
* @end
*
*********************************************************************/
RC_t SLLFlagsSet( sll_t *list,  SLL_FLAG_t flagType, uint32 flagVal)
{
  /* validate the sll */
  if (list ==  NULLPTR)
  {
    return  FAILURE;
  }

  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  FAILURE;
  }

  /* Set the flag on the list */
  switch(flagType)
  {
    case  SLL_FLAG_ALLOW_DUPLICATES:
      if(flagVal ==  TRUE)
        list->sllDupEnable =  TRUE;
      else if(flagVal ==  FALSE)
        list->sllDupEnable =  FALSE;
      else
        return  FAILURE;
      break;
    default:
      return  FAILURE;
  }
  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  Deletion of the Single Linked List
*
* @param    compId  @b{(input)}ID of the component deleting the list
*           list    @b{(input)}The pointer to the linked list that is to be destroyed.
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if failure
*
* @notes    This API is provided to destroy a linked list, All the nodes are
*           iteratively deleted using the provided destroy function. The
*           given list object is destroyed as well.
*
* @end
*
*********************************************************************/
RC_t SLLDestroy( COMPONENT_IDS_t compId,  sll_t *list)
{
  /* validate the sll */
  if (list ==  NULLPTR)
  {
    return  FAILURE;
  }
  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  FAILURE;
  }

  /*  Destroy all the nodes */
  if(SLLPurge(compId, list) !=  SUCCESS)
  {
    return  FAILURE;
  }

  osapiSemaDelete(list->semId);

  list->inUse =  FALSE;
  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  Deletion of ALL the elements of the Single Linked List
*
* @param    compId  @b{(input)}ID of the component deleting the list
*           list    @b{(input)}The pointer to the linked list that is to be destroyed.
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if failure
*
* @notes    This API is provided to destroy ALL the elements of the linked list, 
*           All the nodes are iteratively deleted using the provided destroy function.
*           The list object is maintained as is.
*
* @end
*
*********************************************************************/
RC_t SLLPurge( COMPONENT_IDS_t compId,  sll_t *list)
{
   sll_member_t   *node,*delNode;

  /* validate the sll */
  if (list ==  NULLPTR)
  {
    return  FAILURE;
  }


  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  FAILURE;
  }

  /* Iterate through the list and delete */
  for (node = list->sllStart; node !=  NULLPTR;)
  {
    delNode = node;
    node = node->next;
    if(list->sllDestroyFunc !=  NULLPTR)
      list->sllDestroyFunc(delNode);
    else
      SLLDefaultFuncDestroy(compId, delNode);
  }

  /* Reset the list variables */
  list->sllNumElements = 0;
  list->sllStart =  NULLPTR;
  list->sllEnd =  NULLPTR;

  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  Add a node to the Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list to which the node is to
*                              be added.
*           node    @b{(input)}The pointer to the node to be added.
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if failure
*
* @notes    The Memory allocation of the node is upto the caller not the
*           responsibility of this function.
*
* @end
*
*********************************************************************/
RC_t SLLAdd( sll_t *list, sll_member_t *node)
{
   sll_member_t *prev;

  /* Validate the input parameters */
  if (list ==  NULLPTR || node ==  NULLPTR)
    return  FAILURE;


  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  FAILURE;
  }
  /* find the place where to insert */
  if (SLLSeek(list,node,&prev) ==  SUCCESS)
  {
    if(list->sllDupEnable ==  TRUE)
    {
      if(prev ==  NULLPTR)
        prev = list->sllStart;
      else
        prev = prev->next;
      for(; prev->next !=  NULLPTR; prev = prev->next)
      {
        if(list->sllCompareFunc((void *)(prev->next),(void *)node,list->sllKeySize) != 0)
        {
          break;
        }
      }
    }
    else
    {
      return  FAILURE;
    }
  }

  /* add it at the appropriate position */
  SLLNodeInsert(list,node,prev);
  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  Find a node in the Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list.
* @param    node    @b{(input)}The pointer to a node containing the key to be used for searching.
*
* @returns  The pointer to the actual node if found, else NULL is returned.
*
* @notes
*
* @end
*
*********************************************************************/
 sll_member_t *SLLFind( sll_t *list,  sll_member_t *node)
{
   sll_member_t *prev =  NULLPTR;

  if (list ==  NULLPTR || node ==  NULLPTR)
    return  NULLPTR;

  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  NULLPTR;
  }
  
  if(SLLSeek(list, node, &prev) !=  SUCCESS)
    return  NULLPTR;

  if (prev ==  NULLPTR)
    return list->sllStart;
  return prev->next;
}

/*********************************************************************
*
* @purpose  Find a node next in the order in the given Single Linked List
*
* @param    list   @b{(input)} The pointer to the linked list.
* @param    node   @b{(input)}The pointer to a node containing the key to be used for searching.
*
* @returns    The pointer to the node whose key is next to the key that is provided if found,
*             else NULL is returned.
*
* @notes      This API is only applicable for ordered lists that do not have Duplicates
*             enabled. The main purpose of this API is to be able to support SNMP kind
*             of NextGet operations.
*
* @end
*
*********************************************************************/
 sll_member_t *SLLFindNext( sll_t *list,  sll_member_t *node)
{
   sll_member_t *prev;

  if (list ==  NULLPTR || node ==  NULLPTR)
    return  NULLPTR;

  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  NULLPTR;
  }
  
  if((list->sllDupEnable ==  TRUE) || (list->sllSortType ==  SLL_NO_ORDER))
    return  NULLPTR;

  if(SLLSeek(list, node, &prev) ==  SUCCESS)
  {
    if(prev ==  NULLPTR)
      prev = list->sllStart;
    else
      prev = prev->next;
  }

  if (prev ==  NULLPTR)
    return list->sllStart;
  return prev->next;
}

/*********************************************************************
*
* @purpose  Removes a node from the Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list.
* @param    node    @b{(input)}The pointer to a node containing the key to be used for searching.
*
* @returns  The pointer to the actual node if found, else NULL is returned.
*
* @notes    The Node is removed from the list and given to the caller.
*           No memory deallocation is done.
*
* @end
*
*********************************************************************/
 sll_member_t *SLLRemove( sll_t *list,  sll_member_t *node)
{
   sll_member_t *prev;

  if (list ==  NULLPTR || node ==  NULLPTR)
    return  NULLPTR;

  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  NULLPTR;
  }
  
  if(SLLSeek(list, node, &prev) ==  SUCCESS)
    return SLLNodeExtract(list, prev);
  return  NULLPTR;
}

/*********************************************************************
*
* @purpose  Deletes a node from the Single Linked List
*
* @param    list    - The pointer to the linked list.
*           node    - The pointer to a node containing the key to be used for searching.
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if failure
*
* @notes    The Node is removed from the list and memory deallocation is also done.
*
* @end
*********************************************************************/
RC_t SLLDelete( sll_t *list,  sll_member_t *node)
{
   sll_member_t *pNode =  NULLPTR;

  if (list ==  NULLPTR || node ==  NULLPTR)
    return  FAILURE;

  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  FAILURE;
  }

  /* Extract the node first from the list */
  pNode = SLLRemove(list,node);
  if (pNode ==  NULLPTR)
    return  FAILURE;
  if(list->sllDestroyFunc !=  NULLPTR)
    list->sllDestroyFunc(pNode);
  else
    SLLDefaultFuncDestroy(list->sllCompId,pNode);
  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  Return the first node in the Single Linked List
*
* @param    list  @b{(input)}The pointer to the linked list.
*
* @returns  The pointer to the first node, if there is one in the list.
* @returns   NULLPTR, if there are no nodes in the list.
*
* @notes    This API does not remove the node from the list, but simply
*           returns the first node's reference. This could be used in
*           iterating through the list.
*
* @end
*
*********************************************************************/
 sll_member_t *SLLFirstGet( sll_t *list)
{
  if (list ==  NULLPTR)
  {
    return  NULLPTR;
  }
  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  NULLPTR;
  }  
  return (list->sllStart);
}

/*********************************************************************
*
* @purpose  Return the next node after the given node in the Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list.
*           pNode   @b{(input)}The pointer to the current node in the linked list.
*
* @returns  The pointer to the next node, if there is one in the list.
* @returns   NULLPTR, if there are no nodes in the list.
*
* @notes    This API does not remove the node from the list, but simply
*           returns the node's reference. This could be used in
*           iterating through the list.
* @notes    If  NULLPTR is given for the pNode; then the first entry in the
*           list is returned.
*
* @end
*
*********************************************************************/
 sll_member_t *SLLNextGet( sll_t *list,  sll_member_t *pNode)
{
  /* Validate the input parameters */
  if(list ==  NULLPTR)
    return  NULLPTR;

  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  NULLPTR;
  }  
  
  /* If the given node pointer is NULL, return the first node on the list */
  if(pNode ==  NULLPTR)
  {
    return SLLFirstGet(list);
  }

  /* Ensure that the given node is actually in the list */
  if(SLLNodeFind(list, pNode) != pNode)
    return  NULLPTR;

  /* Return the pointer to the next node */
  return pNode->next;
}

/*********************************************************************
*
* @purpose  Find the given exact node in the Single Linked List
*
* @param    list      @b{(input)}The pointer to the linked list.
* @param    searchKey @b{(input)}The pointer to the actual mode to be found.
*
* @returns  The pointer to the node if found, else NULL is returned.
*
* @end
*
*********************************************************************/
 sll_member_t *SLLNodeFind( sll_t *list,  sll_member_t *pNode)
{
   sll_member_t *member =  NULLPTR;

  if(list ==  NULLPTR || pNode ==  NULLPTR)
    return  NULLPTR;

  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  NULLPTR;
  }  
  
  for(member = list->sllStart; member !=  NULLPTR; member=member->next)
  {
    if(member == pNode)
    {
      return member;
    }
  }
  return  NULLPTR;
}

/*********************************************************************
*
* @purpose  Removes the given exact node from the Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list.
* @param    pNode   @b{(input)}The pointer to the actual node to be removed.
*
* @returns  The pointer to the node if found, else NULL is returned.
*
* @notes    The Node is removed from the list and given to the caller.
*           No memory deallocation is done.
*
* @end
*
*********************************************************************/
 sll_member_t *SLLNodeRemove( sll_t *list,  sll_member_t *pNode)
{
   sll_member_t *prev =  NULLPTR;
   sll_member_t *member =  NULLPTR;

  if(list ==  NULLPTR || pNode ==  NULLPTR)
    return  NULLPTR;

  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  NULLPTR;
  }  
  
  for (member = list->sllStart; member !=  NULLPTR; member=member->next)
  {
    if(member == pNode)
    {
      return SLLNodeExtract(list, prev);
    }
    prev = member;
  }
  return  NULLPTR;
}

/*********************************************************************
*
* @purpose  Deletes the given exact node from the Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list.
*           pNode   @b{(input)}The pointer to the actual node to be deleted.
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if failure
*
* @notes    The Node is removed from the list and memory deallocation
*           is also done.
*
* @end
*
*********************************************************************/
RC_t SLLNodeDelete( sll_t *list,  sll_member_t *pNode)
{
   sll_member_t *prev =  NULLPTR;
   sll_member_t *member =  NULLPTR;

  if(list ==  NULLPTR || pNode ==  NULLPTR)
    return  FAILURE;


  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  FAILURE;
  }

  /* Extract the node first from the list */
  for (member = list->sllStart; member !=  NULLPTR; member=member->next)
  {
    if(member == pNode)
    {
      SLLNodeExtract(list, prev);
      if(list->sllDestroyFunc !=  NULLPTR)
        list->sllDestroyFunc(member);
      else
        SLLDefaultFuncDestroy(list->sllCompId, member);
      return  SUCCESS;
    }
    prev = member;
  }
  return  FAILURE;
}

/*********************************************************************
*
* @purpose  Add a node to the End of the Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list to which the node is to
*                              be added.
* @param    node    @b{(input)}The pointer to the node to be added.
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if failure
*
* @notes
*
* @end
*
*********************************************************************/
RC_t SLLAtEndPush( sll_t *list, sll_member_t *node)
{
   sll_member_t *prev;

  /* Validate the input parameters */
  if (list ==  NULLPTR || node ==  NULLPTR)
    return  FAILURE;


  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  FAILURE;
  }

  /* Insert the node at the end after validations */
  if(list->sllSortType ==  SLL_NO_ORDER)
  {
     /* Non-ordered list */
    if(list->sllDupEnable !=  TRUE)
    {
      /* If duplicates are not enabled, then check for duplicate entries
         in the list */
      if (SLLSeek(list,node,&prev) ==  SUCCESS)
        return  FAILURE;
    }
    /* Insert the node at the end */
    SLLNodeInsert(list,node,list->sllEnd);
  }
  else
  {
    /* Ordered list */
    if(SLLSeek(list,node,&prev) ==  SUCCESS)
    {
      /* Entry already in the list */
      if(list->sllDupEnable !=  TRUE)
        return  FAILURE;

      /* Find the last duplicate entry */
      if(prev ==  NULLPTR)
        prev = list->sllStart;
      else
        prev = prev->next;
      for(; prev->next !=  NULLPTR; prev = prev->next)
      {
        if(list->sllCompareFunc((void *)(prev->next),(void *)node,list->sllKeySize) != 0)
        {
          break;
        }
      }
    }
    /* Insert at the appropriate location */
    SLLNodeInsert(list,node,prev);
  }
  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  Add a node to the start of the Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list to which the node is to
*                              be added.
* @param    node    @b{(input)}The pointer to the node to be added.
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if failure
*
* @notes
*
* @end
*
*********************************************************************/
RC_t SLLAtStartPush( sll_t *list, sll_member_t *node)
{
   sll_member_t *prev;

  /* Validate the input parameters */
  if (list ==  NULLPTR || node ==  NULLPTR)
    return  FAILURE;


  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  FAILURE;
  }

  /* Insert the node at the start after validations */
  if(list->sllSortType ==  SLL_NO_ORDER)
  {
     /* Non-ordered list */
    if(list->sllDupEnable !=  TRUE)
    {
      /* If duplicates are not enabled, then check for duplicate entries
         in the list */
      if (SLLSeek(list,node,&prev) ==  SUCCESS)
        return  FAILURE;
    }
    /* Insert the node at the start */
    SLLNodeInsert(list,node, NULLPTR);
  }
  else
  {
    /* Ordered list */
    if(SLLSeek(list,node,&prev) ==  SUCCESS)
    {
      /* Entry already in the list? */
      if(list->sllDupEnable !=  TRUE)
        return  FAILURE;
    }
    /* Insert at the appropriate location */
    SLLNodeInsert(list,node,prev);
  }
  return  SUCCESS;
}

/*********************************************************************
*
* @purpose  Extract a node from the End of the Single Linked List
*
* @param    list    @b{(input)}The pointer to the unsorted linked list.
*
* @returns  The pointer to the node, if there are nodes in the list.
* @returns   NULL, if there are no nodes in the list.
*
* @notes
*
* @end
*
*********************************************************************/
 sll_member_t *SLLAtEndPop( sll_t *list)
{
   sll_member_t  *member,*temp;

  /* Validate the input parameters */
  if (list ==  NULLPTR)
    return  NULLPTR;


  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  NULLPTR;
  }

  if (list->sllStart ==  NULLPTR)
    return  NULLPTR;
  
  member = list->sllStart; 
   
  if (member->next ==  NULLPTR)
  {
    list->sllStart =  NULLPTR;
    return member;
  }
  
  /* Traverse to the end but one */
  for (; member->next->next !=  NULLPTR; member=member->next);

  /* Extract it from the end of the list */
  temp = member->next;
  member->next =  NULLPTR;
  return temp;
}

/*********************************************************************
*
* @purpose  Extract a node from the start of the Single Linked List
*
* @param    list    @b{(input)}The pointer to the unsorted linked list.
*
* @returns  The pointer to the node, if there are nodes in the list.
* @returns   NULL, if there are no nodes in the list.
*
* @notes
*
* @end
*
*********************************************************************/
 sll_member_t *SLLAtStartPop( sll_t *list)
{
  /* Validate the input parameters */
  if (list ==  NULLPTR)
    return  NULLPTR;

  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return  NULLPTR;
  }

  if (list->sllStart ==  NULLPTR)
    return  NULLPTR;


  /* Extract it from the start of the list */
  return (SLLNodeExtract(list, NULLPTR));
}

/*********************************************************************
*
* @purpose  Retrieve the number of members/entries in the Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list.
*
* @returns  Number of members/entries/nodes in the list.
*
* @notes
*
* @end
*
*********************************************************************/
uint32 SLLNumMembersGet( sll_t *list)
{
  if(list == NULLPTR)
    return  0;

  /* validate the sll inUse */
  if (list->inUse ==  FALSE)
  {
    return 0;
  }  
  return list->sllNumElements;
}

/*********************************************************************
*
* @purpose  Finds the position of a node in the Single Linked List
*
* @param    list @b{(input)}The pointer to the linked list.
* @param    node @b{(input)}The pointer to the key/node to be used for searching.
* @param    prev @b{(input)}the output pointer contains :
*             (i) the node previous to he node that matches the given key/node,
*                 if the return value is  SUCCESS.
*             (ii)the node after which the given node could be inserted,
*                 if the return value is  FAILURE.
*
* @returns   SUCCESS, if node is found
* @returns   FAILURE, if node is not found.
* @returns   ERROR, if list is invalid.
*
* @notes    -  None.
*
* @end
*
*********************************************************************/
static
RC_t SLLSeek( sll_t *list,
                 sll_member_t *node,
                 sll_member_t **prev)
{
   sll_member_t *member;
   int32 cmp;

  *prev =  NULLPTR;
  for (member = list->sllStart; member !=  NULLPTR; member=member->next)
  {
    /* use the compare function */
    cmp = list->sllCompareFunc((void *)member,(void *)node,list->sllKeySize);
    if(cmp == 0)
      return  SUCCESS;
    switch (list->sllSortType)
      {
        case  SLL_NO_ORDER:
          break;
        case  SLL_ASCEND_ORDER:
          if(cmp > 0)
            return  FAILURE;
          break;
        case  SLL_DESCEND_ORDER:
          if(cmp < 0)
            return  FAILURE;
          break;
        default:
          return  ERROR;
      }
      *prev = member;
  }
  return  FAILURE;
}

/*********************************************************************
*
* @purpose  Inserts the node into the linked list.
*
* @param    list  @b{(input)}The pointer to the linked list.
* @param    node  @b{(input)}The pointer to the node to be inserted
* @param    prev  @b{(input)}The previous pointer after which the node is inserted.
*
* @returns   None.
*
* @notes     None
*
* @end
*
*********************************************************************/
static
void SLLNodeInsert( sll_t *list,
                    sll_member_t *node,
                    sll_member_t *prev)
{
  if (prev ==  NULLPTR)
  {
    node->next = list->sllStart;
    list->sllStart = node;
  }
  else
  {
    node->next = prev->next;
    prev->next = node;
  }
  if (node->next ==  NULLPTR)
    list->sllEnd = node;
  list->sllNumElements++;
}

/*********************************************************************
*
* @purpose  Extracts the node into the linked list.
*
* @param    list @b{(input)}The pointer to the linked list.
*           prev @b{(input)}The previous pointer after which the node is to be extracted.
*
* @returns  The pointer to the node which has been extracted.
*
* @notes    None
*
* @end
*
*********************************************************************/
static
 sll_member_t *SLLNodeExtract( sll_t *list,
                                 sll_member_t *prev)
{
   sll_member_t *node;

  if (list->sllStart ==  NULLPTR)
    return  NULLPTR;

  if (prev ==  NULLPTR)
  {
    node = list->sllStart;
    list->sllStart = node->next;
  }
  else
  {
    node = prev->next;
    prev->next = node->next;
  }
  if (node->next ==  NULLPTR)
    list->sllEnd = prev;
  list->sllNumElements--;
  return node;
}

/*********************************************************************
*
* @purpose  The Defunct compare function to be used when compare function
*           is not provided during the creation of the linked list.
*
* @param    data1  @b{(input)}The pointer to the first key.
* @param    data2  @b{(input)}The pointer to the second key.
* @param    keyLen @b{(input)}The length of the key to be compared.
*
* @returns  Less than 0, if node1 < node 2.
* @returns  Zero, if node1 == node2
* @returns  More than 0, if node1 > node2.
*
* @notes    This function is valid only when key length is more than zero.
*
* @end
*
*********************************************************************/
static
 int32 SLLDefaultFuncCompare(void *data1,
                               void *data2,
                               uint32  keyLen)
{
   sll_member_t *pNode1, *pNode2;

  if(data1 ==  NULLPTR || data2 ==  NULLPTR)
   return 1;

  pNode1 = ( sll_member_t *)data1;
  pNode2 = ( sll_member_t *)data2;

  /* when key length is zero, comparision makes no sense */
  if (keyLen > 0)
    return memcmp(pNode1->data,pNode2->data,keyLen);
  else
    return 1;
}

/*********************************************************************
*
* @purpose  The Defunct destroy function to be used when destroy function
*           is not provided during the creation of the linked list.
*
* @param    node @b{(input)}The pointer to the node that is to be deleted.
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if failure
*
* @notes    -   None.
*
* @end
*
*********************************************************************/
static
RC_t SLLDefaultFuncDestroy( COMPONENT_IDS_t compId,  sll_member_t *node)
{
  osapiFree(compId, node);
  return  SUCCESS;
}
