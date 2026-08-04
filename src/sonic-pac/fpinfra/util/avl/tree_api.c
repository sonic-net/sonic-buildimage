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
#include "tree_api.h"

/*********************************************************************
* @purpose  Creates the AVL tree and our wrapper
*           
* @param     COMPONENT_IDS_t @b{(input)} component ID for memory 
*           allocation
* @param    uint32 max_entries @b{(input)} max data entries in tree
* @param    uint32 data_length @b{(input)} length in bytes of an entry
* @param    uint32 key_length @b{(input)} length in bytes of key
*
* @returns  pointer to tree_t if successful
* @returns   NULLPTR otherwise
*
* @notes    allocates ALL memory required.  Failure of any one step
*           will cause all allocated memory to be freed.
* @end
*********************************************************************/
tree_t * 
treeCreate( COMPONENT_IDS_t component_ID,
           uint32          max_entries,
           uint32          data_length,
           uint32          key_length)
{
  avlTreeTables_t *pTHeap   =  NULLPTR;
  void            *pDHeap   =  NULLPTR;
  void            *pZeroKey =  NULLPTR;
  tree_t          *pTree    =  NULLPTR;

  do 
  {
    /* allocate tree heap */
    pTHeap = (avlTreeTables_t *)
             osapiMallocTry(component_ID, 
                         max_entries * sizeof(avlTreeTables_t));
    if ( NULLPTR == pTHeap)
    {
      break;
    }
    
    /* allocate data heap */
    pDHeap = (void *) osapiMallocTry(component_ID, 
                                  max_entries * data_length);
    if ( NULLPTR == pDHeap)
    {
      break;
    }

    /* allocate "zero key" */
    pZeroKey = (void *) osapiMallocTry(component_ID, 
                                    key_length);
    if ( NULLPTR == pZeroKey)
    {
      break;
    }

    /* allocate tree structure */
    pTree = (tree_t *) osapiMallocTry(component_ID, 
                                   sizeof(tree_t));
    if ( NULLPTR == pTree)
    {
      break;
    }

    /* init our tree wrapper if we get here */
    avlCreateAvlTree(&(pTree->avlTree), pTHeap, pDHeap, 
                     max_entries, data_length, 0, key_length);
    pTree->tree_heap    = pTHeap;
    pTree->data_heap    = pDHeap;
    pTree->component_ID = component_ID;
    pTree->max_entries  = max_entries;
    pTree->zero_key     = pZeroKey;
    memset(pZeroKey, 0, key_length);
  } while (0);

  /* deallocate everything if we fail */
  if ( NULLPTR == pTree)
  {
    if ( NULLPTR != pTHeap)
    {
      osapiFree(component_ID, (void *) pTHeap);
    }
    if ( NULLPTR != pDHeap)
    {
      osapiFree(component_ID, (void *) pDHeap);
    }
    if ( NULLPTR != pZeroKey)
    {
      osapiFree(component_ID, (void *) pZeroKey);
    }
    /* pTree will be NULL if we get here */
  }
  return pTree;
}


/*********************************************************************
* @purpose  Sets comparator for tree
*           
* @param    tree_t          *pTree @b{(output)} tree to manipulate
* @param    avlComparator_t comp   @b{(input)} new comparator
*
* @returns  pointer to former comparator if successful
* @returns   NULLPTR otherwise
*
* @end
*********************************************************************/
avlComparator_t 
treeComparatorSet(tree_t * pTree, avlComparator_t comp)
{
  if (!pTree)
  {
    return  NULLPTR;
  }
  return avlSetAvlTreeComparator(&(pTree->avlTree), comp);
}



/*********************************************************************
* @purpose  Deletes tree and all of its previously allocated memory
*           
* @param    tree_t   *pTree @b{(output)} tree to wipe
*
* @returns  void
*
* @end
*********************************************************************/
void
treeDelete(tree_t * pTree)
{
  if (!pTree)
    return;

  /* Delete tree */  
  (void) avlDeleteAvlTree(&(pTree->avlTree));
  /* Delete heaps */
  osapiFree(pTree->component_ID, (void *) pTree->tree_heap);
  osapiFree(pTree->component_ID, (void *) pTree->data_heap);
  osapiFree(pTree->component_ID, (void *) pTree->zero_key);
  /* Delete wrapper */
  osapiFree(pTree->component_ID, pTree);
}


/*********************************************************************
* @purpose  Removes all elements from a tree
*           
* @param    tree_t   *pTree @b{(output)} tree to prune
*
* @returns  void
*
* @end
*********************************************************************/
void
treePurge(tree_t * pTree)
{
  if ( NULLPTR != pTree)
  {
    avlPurgeAvlTree(&(pTree->avlTree), pTree->max_entries);
  }
}

/*********************************************************************
* @purpose  Removes all elements from a tree, deletes tree and
*           all of its previously allocate
*
* @param    tree_t   *pTree @b{(output)} tree to prune
*
* @returns  void
*
* @end
*********************************************************************/
void
treeDestroy(tree_t *pTree)
{
  if ( NULLPTR != pTree)
  {
    treePurge (pTree);
    treeDelete (pTree);
  }
}

/*********************************************************************
* @purpose  Gets current element count
*           
* @param    tree_t          *pTree @b{(output)} tree to count
*
* @returns  uint32 
*
* @notes    returns 0 if pTree is a null pointer
* @end
*********************************************************************/
uint32 
treeCount(tree_t * pTree)
{
  if (!pTree)
  {
    return 0;
  }
  return avlTreeCount(&(pTree->avlTree));
}


/*********************************************************************
* @purpose  Inserts an element into a tree
*           
* @param    tree_t   *pTree @b{(output)} tree to augment
* @param    tree_t   *pItem @b{(input)}  element to insert
*
* @returns   TRUE  if insertion worked
* @returns   FALSE otherwise
*
* @end
*********************************************************************/
 BOOL
treeEntryInsert(tree_t * pTree, void * pItem)
{
   BOOL rc =  FALSE;

  do
  {  
    if (!pTree || !pItem)
    {
      break;
    }
    if ( NULLPTR == avlInsertEntry(&(pTree->avlTree), pItem))
    {
      rc =  TRUE;
    }
  } while (0);
  return rc;
}

/*********************************************************************
* @purpose  Delete an element from a tree
*           
* @param    tree_t   *pTree @b{(output)} tree to augment
* @param    tree_t   *pItem @b{(input)}  element to delete
*
* @returns   TRUE  if deletion worked (i.e. element found)
* @returns   FALSE otherwise
*
* @end
*********************************************************************/
 BOOL
treeEntryDelete(tree_t * pTree, void * pItem)
{
   BOOL rc =  FALSE;

  do
  {  
    if (!pTree || !pItem)
    {
      break;
    }
    if ( NULLPTR != avlDeleteEntry(&(pTree->avlTree), pItem))
    {
      rc =  TRUE;
    }
  } while (0);
  return rc;
}


/*********************************************************************
* @purpose  Find an element within a tree
*           
* @param    tree_t *pTree @b{(input)} tree to search
* @param    void   *key   @b{(input)} key to search for
*
* @returns   NULLPTR if matching element not found
* @returns  pointer to element otherwise
*
* @end
*********************************************************************/
void *
treeEntryFind(tree_t * pTree, void * key)
{
  void * pItem =  NULLPTR;
  do
  {  
    if (!pTree || !key)
    {
      break;
    }
    pItem = avlSearch(&(pTree->avlTree), key, AVL_EXACT);
  } while (0);

  return pItem;
}

/*********************************************************************
* @purpose  Find least element within a tree with key greater than
*           a given value
*           
* @param    tree_t *pTree @b{(input)} tree to search
* @param    void   *key   @b{(input)} key to be used in search
*
* @returns   NULLPTR if matching element not found
* @returns  pointer to element otherwise
*
* @end
*********************************************************************/
void *
treeEntryNextFind(tree_t * pTree, void * key)
{
  void * pItem =  NULLPTR;
  do
  {  
    if (!pTree || !key)
    {
      break;
    }
    pItem = avlSearch(&(pTree->avlTree), key, AVL_NEXT);
  } while (0);

  return pItem;
}


/*********************************************************************
* @purpose  Perform an operation on every element in a tree, from
*           "smallest" to "largest" key value
*           
* @param    tree_t *pTree @b{(output)} tree to manipulate
* @param    treeEntryManipulatorFn @b{(input)} manipulation function
* @param    void   *args  @b{(input/output)} arguments to function
*
* @returns  void
* 
* @notes    Used to iterate over every element in a tree.  The
*           args parameter may point to a structure that holds 
*           all of the necessary data for the maninpulator function.
* @end
*********************************************************************/

void
treeForEachEntryDo(tree_t                  *pTree, 
                    treeEntryManipulatorFn manip, 
                    void                   *args)
{
  void * pIter =  NULLPTR; 
  if (!pTree || !manip)
  {
    return;
  }
  pIter = avlSearch(&(pTree->avlTree), 
                        pTree->zero_key, AVL_NEXT);
  while ( NULLPTR != pIter)
  {
    (*manip)(pIter, args);
    pIter = avlSearch(&(pTree->avlTree), 
                          pIter, AVL_NEXT);
  }
}
