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

#ifndef TREE_API_H
#define TREE_API_H

#include "osapi.h"
#include "avl_api.h"

/*********************************************************************
 * 
 * Note that with this wrapper, element restrictions with the existing
 * AVL tree still apply:
 * 1. The search key MUST appear FIRST in the data structure.
 * 2. The last element in the data structure MUST be a void pointer.
 * (It would be nice to get rid of these, but that would involve
 *  re-writing the AVL tree code itself.  No.  Thank.  You.
 *
 *
 *
 *********************************************************************/
 
/* typedef int (*avlComparator_t)(const void *a, const void *b, size_t l); */

typedef struct tree_s {
   COMPONENT_IDS_t component_ID; /* for memory alloc/free */
  avlTree_t          avlTree;     
  uint32          max_entries;  
  void               *zero_key;    /* for finding "first" element */
  avlTreeTables_t    *tree_heap;   
  void               *data_heap;
} tree_t;


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
	   uint32          key_length);


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
treeComparatorSet(tree_t *pTree,  avlComparator_t comp);


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
uint32  treeCount(tree_t *pTree);

/*********************************************************************
* @purpose  Deletes tree and all of its previously allocated memory
*           
* @param    tree_t   *pTree @b{(output)} tree to wipe
*
* @returns  void
*
* @end
*********************************************************************/
void       treeDelete(tree_t *pTree);

/*********************************************************************
* @purpose  Removes all elements from a tree
*           
* @param    tree_t   *pTree @b{(output)} tree to prune
*
* @returns  void
*
* @end
*********************************************************************/
void        treePurge(tree_t *pTree);

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
void        treeDestroy(tree_t *pTree);

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
 BOOL     treeEntryInsert(tree_t *pTree, void *pItem);

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
 BOOL     treeEntryDelete(tree_t *pTree,  void *pItem);


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
void *      treeEntryFind(tree_t *pTree, void   *key);


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
void *      treeEntryNextFind(tree_t *pTree,  void   *key);


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

typedef void (*treeEntryManipulatorFn)(void * element, void * args);

void
treeForEachEntryDo(tree_t                 *pTree, 
                   treeEntryManipulatorFn manip, 
                   void                   *args);
                    
/* Internal FastPath avlAllocAndCreateAvlTree and avlDeleteAvlTree Functions declaration */
/*********************************************************************
  * @purpose  Creates the generic avl tree structure and initialize
  *
  * @param    avlTree           pointer to the tree structure
  * @param    componentId       caller's component ID.
  * @param    max_entries       maximum number of entries in the tree
  * @param    dataLength        length of a data entry
  * @param    type              type of data
  * @param    compareFcn        pointer to the comparator function
  *                             if NULL is used, the default comparator
  *                             (memcmp()) will be used.
  * @param    lengthSearchKey   length of the search key
  *
  * @returns   SUCCESS  if avl tree initialization was successful
  * @returns   FAILURE  if not successful
  *
  * @notes    RESTRICTIONS:1. First field in the data structure
  *                           (of data heap) must be the key
  *                        2. Last field in the data structure
  *                           (of data heap) must be a void pointer type
  *
  * @end
  *********************************************************************/
  RC_t avlAllocAndCreateAvlTree(avlTree_t *avlTree,
                                    COMPONENT_IDS_t componentId,
                                   uint32 max_entries,
                                   uint32 dataLength,
                                   uint32 type,
                                   avlComparator_t compareFcn,
                                   uint32 lengthSearchKey);

/*********************************************************************
* @purpose  Deletes an avl tree structure
*
* @param    tree              pointer to an avl tree structure
*
* @returns   SUCCESS
* @returns   FAILURE
* @returns   ERROR
*
* @notes
*
* @end
*********************************************************************/
RC_t avlDeleteAvlTree(avlTree_t *tree);

#endif /* TREE_API_H */
