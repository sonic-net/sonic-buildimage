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


#ifndef _SLL_H
#define _SLL_H

/*******************************************************************************
**                             Includes                                       **
*******************************************************************************/

//#include "pacinfra_common.h"


/*******************************************************************************
**                             Definitions                                    **
*******************************************************************************/

typedef enum
{
   SLL_NO_ORDER = 0,
   SLL_ASCEND_ORDER,
   SLL_DESCEND_ORDER
}  SLL_SORT_TYPE;

typedef enum
{
   SLL_FLAG_ALLOW_DUPLICATES = 1
}  SLL_FLAG_t;

/*  sll_member_t structure definition */
typedef struct  sll_member_s
{
    struct  sll_member_s *next;
    void                   *data;
}  sll_member_t;


/* fucntion pointer definitions */
typedef  int32    (* sllCompareFunc)(void      *data1,
                                         void      *data2,
                                         uint32 keyLen);

typedef RC_t     (* sllDestroyFunc)( sll_member_t *node);


/*  sll_t structure definition */
typedef struct  sll_s
{
  uint32               sllNumElements;
   SLL_SORT_TYPE        sllSortType;
   BOOL                 sllDupEnable;
  uint32               sllKeySize;
   COMPONENT_IDS_t      sllCompId;

  /* data holders */
  struct  sll_member_s  *sllStart;
  struct  sll_member_s  *sllEnd;

  /* function pointers for comparision and deletion */
   sllCompareFunc       sllCompareFunc;
   sllDestroyFunc       sllDestroyFunc;

   BOOL            inUse;     /* TRUE if initialized; FALSE if un-initialized*/
  void              *semId;     /* semaphore id for this sll*/
}  sll_t;


/*******************************************************************************
**                 Function Prototype Declarations                            **
*******************************************************************************/

/*********************************************************************
*
* @purpose  Creation of a Single Linked List
*
* @param    sortType @b{(input)}Specifies the sort order for the list.
* @param    keySize  @b{(input)}It is the size of the index(key) for each node in the list
* @param    compFunc @b{(input)}A function pointer that would be used for comparision
*                               between the nodes.
* @param    desFunc  @b{(input)}A function pointer that would be used for the deletion of
*                               node.
* @param     list    @b{(input)}The output pointer to the linked list that is created.
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
extern
RC_t SLLCreate( COMPONENT_IDS_t compId,
                   SLL_SORT_TYPE   sortType,
                  uint32          keySize,
                   sllCompareFunc  compFunc,
                   sllDestroyFunc  desFunc,
                   sll_t           *list);

/*********************************************************************
*
* @purpose  Set/Reset flags on the Single Linked List
*
* @param    list     @b{(input)}The list to modify the flags on.
* @param    flagType @b{(input)}Flag Identifier.
* @param    flagVal  @b{(input)}Value associated with the flag.
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
extern
RC_t SLLFlagsSet( sll_t *list,  SLL_FLAG_t flagType, uint32 flagVal);

/*********************************************************************
*
* @purpose  Deletion of the Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list that is to be destroyed.
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if failure
*
* @notes    This API is provided to destroy a linked list, All the nodes are
*           iteratively deleted using the provided destroy function.
*
* @end
*
*********************************************************************/
extern
RC_t SLLDestroy( COMPONENT_IDS_t compId,  sll_t *list);

/*********************************************************************
*
* @purpose  Deletion of ALL the elements of the Single Linked List
*
* @param    compId  @b{(input)}ID of the component deleting the list
* @param     list    @b{(input)}The pointer to the linked list that is to be destroyed.
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if failure
*
* @notes    This API is provided to destroy ALL the elements of the linked list, 
*           All the nodes are iteratively deleted using the provided destroy function.
*           The list is maintained as is.
*
* @end
*
*********************************************************************/
extern
RC_t SLLPurge( COMPONENT_IDS_t compId,  sll_t *list);

/*********************************************************************
*
* @purpose  Add a node to the Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list to which the node is to
*                              be added.
* @param    node    @b{(input)}The pointer to the node to be added.
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
extern
RC_t SLLAdd( sll_t *list,  sll_member_t *node);

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
extern
 sll_member_t *SLLFind( sll_t *list,  sll_member_t *node);

/*********************************************************************
*
* @purpose  Find a node next in the order in the given Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list.
* @param    node    @b{(input)}The pointer to a node containing the key to be used for searching.
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
extern
 sll_member_t *SLLFindNext( sll_t *list,  sll_member_t *node);

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
extern
 sll_member_t *SLLRemove( sll_t *list,  sll_member_t *node);

/*********************************************************************
*
* @purpose  Deletes a node from the Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list.
* @param    node    @b{(input)}The pointer to a node containing the key to be used for searching.
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if failure
*
* @notes    The Node is removed from the list and memory deallocation is also done.
*
* @end
*
*********************************************************************/
extern
RC_t SLLDelete( sll_t *list,  sll_member_t *node);

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
extern
 sll_member_t *SLLFirstGet( sll_t *list);

/*********************************************************************
*
* @purpose  Return the next node after the given node in the Single Linked List
*
* @param    list  - The pointer to the linked list.
*           pNode - The pointer to the current node in the linked list.
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
extern
 sll_member_t *SLLNextGet( sll_t *list,  sll_member_t *pNode);

/*********************************************************************
*
* @purpose  Find the given exact node in the Single Linked List
*
* @param    list      @b{(input)} The pointer to the linked list.
* @param    searchKey @b{(input)}The pointer to the actual mode to be found.
*
* @returns  The pointer to the node if found, else NULL is returned.
*
* @end
*
*********************************************************************/
extern
 sll_member_t *SLLNodeFind( sll_t *list,  sll_member_t *pNode);

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
extern
 sll_member_t *SLLNodeRemove( sll_t *list,  sll_member_t *pNode);

/*********************************************************************
*
* @purpose  Deletes the given exact node from the Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list.
* @param    pNode   @b{(input)}The pointer to the actual node to be deleted.
*
* @returns   SUCCESS, if success
* @returns   FAILURE, if failure
*
* @notes      The Node is removed from the list and memory deallocation
*             is also done.
*
* @end
*
*********************************************************************/
extern
RC_t SLLNodeDelete( sll_t *list,  sll_member_t *pNode);

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
*********************************************************************/
extern
RC_t SLLAtEndPush( sll_t *list,  sll_member_t *node);

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
*********************************************************************/
extern
RC_t SLLAtStartPush( sll_t *list,  sll_member_t *node);

/*********************************************************************
*
* @purpose  Extract a node from the End of the Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list.
*
* @returns  The pointer to the node, if there are nodes in the list.
* @returns   NULL, if there are no nodes in the list.
*
* @notes
*
* @end
*
*********************************************************************/
extern
 sll_member_t *SLLAtEndPop( sll_t *list);

/*********************************************************************
*
* @purpose  Extract a node from the start of the Single Linked List
*
* @param    list    @b{(input)}The pointer to the linked list.
*
* @returns  The pointer to the node, if there are nodes in the list.
* @returns   NULL, if there are no nodes in the list.
*
* @notes
*
* @end
*
*********************************************************************/
extern
 sll_member_t *SLLAtStartPop( sll_t *list);

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
extern
uint32 SLLNumMembersGet( sll_t *list);

#endif /*  _SLL_H */
