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

#ifndef _AVL_API_H_
#define _AVL_API_H_

#include <string.h>

/*********************************************************************
*
* @structures  avlTreeTables_s
* 
* @purpose     this is the generic tree table structure. 
*              all data structures need to have this defined
*
* @notes       this stores all the information needed for the 
*              traversal of the tree with poiters to 
*              the data item and links to its subtrees
* 
*********************************************************************/
typedef struct avlTreeTables_s
{          
  signed char               balance;        /* Balance factor. */
  char                      balanceNeeded;  /* Used during insertion. */
  struct avlTreeTables_s    *link[2];       /* Subtrees. */   
  void                      *data;          /* Pointer to data. */
} avlTreeTables_t; 

/*
 * @purpose   Function pointer type for AVL tree comparators
 *
 * @notes     This is the prototype that a custom AVL tree comparator
 *            must match.  The arguments 'a' and 'b' are pointers to
 *            space containing a key of the type used by the particular
 *            tree in question.  The 'l' argument is a length specifier
 *            which may or may not be used by the comparator (most
 *            custom comparators will know the length of the keys to
 *            be compared, but some may not).
 *
 *            The comparator function must accept these arguments, and
 *            return a determination of 'a' greater than 'b', 'a'
 *            less than 'b', or 'a' equal to 'b', using locally defined
 *            rules for greater than, less than, and equal to.  The only
 *            constraints on those rules are that greater than and less
 *            than must return a consistent result when given the same
 *            pair of keys in the same order, and equal to must uniquely
 *            identify a key in the domain of all possible keys in the
 *            tree.
 *
 *            NOTE: The use of 'size_t' in this context is specifically
 *                  sanctioned by the CRC since this prototype must
 *                  match the memcmp(3) prototype, and the size_t
 *                  usage here is not problematic.  Because the size
 *                  of a size_t varies widely, there are cases
 *                  in which the use of size_t can cause portability
 *                  concerns.  In particular, those concerns arize over
 *                  bitwise operations or operations that are aware of
 *                  the maximum value that a size_t can represent.
 *                  Neither of these concerns apply here.
 *                 
 *                  This declaration should not be seen as precedent
 *                  for general use of size_t parameters.
 */
typedef int (*avlComparator_t)(const void *a, const void *b, size_t l);

/*********************************************************************
*
* @structures avl_tree_s
* 
* @purpose    this is the generic avl tree structure
*
* @notes      contains all the information needed for the working
*             of the avl tree
* 
*********************************************************************/
typedef struct avl_tree_s
{
  unsigned int         type;         /* type of data to be stored */
  unsigned int         lengthData;       /* length of data */             
  void                 *semId;            /* semaphore id for this avl tree */
  unsigned int         offset_next;      /* offset to the 'next' member of the data structure*/
  unsigned int         lengthSearchKey;  /* length of the serch key of the data structure*/
  avlTreeTables_t   *currentTableHeap;   /* pointer to the current position of the table heap*/
  avlTreeTables_t   *initialTableHeap;   /* pointer to the first position of the table heap*/
  void              *currentDataHeap;  /* pointer to the current position of the data heap*/
  void              *initialDataHeap;  /* pointer to the first position of the data heap*/
  avlTreeTables_t   root;              /* Tree root node. */
  unsigned int         count;      /* Number of nodes in the tree. */
  void              *value;        /* Arbitary user data. */
  avlComparator_t   compare;    /* Comparator func ptr (default: memcmp()) */

  /* The following pointers are only set if the memory was allocated   */
  /* by avlAllocAndCreateAvlTree and must be freed by avlDeleteAvlTree */
  unsigned int      componentId;
  avlTreeTables_t   *tableHeap;   /* pointer to the table heap */
  void              *dataHeap;    /* pointer to the data heap*/
} avlTree_t;



#define AVL_EXACT 1
#define AVL_NEXT  2

/*********************************************************************
* @purpose  Creates the generic avl tree structure and initialize  
*           
* @param    tree              pointer to the tree structure
* @param    treeHeap          pointer to the tree heap 
* @param    dataHeap          pointer to the data heap 
* @param    max_entries       maximum number of entries in the tree 
* @param    dataLength        length of a data entry 
* @param    type              type of data
* @param    lengthSearchKey   length of the search key
*
* @returns  AvlTree_t   pointer to the generic avl tree structure if successful
* @returns  0           if not successful   
*
* @notes    RESTRICTIONS:1. the first field in the data structure (of the
*                           data heap) must be the key 
*                        2. the last field in the data structure (of the data
*                           heap) must be a void pointer type
*                        
*       
* @end
*********************************************************************/
void avlCreateAvlTree(avlTree_t *tree, avlTreeTables_t *treeHeap, void *dataHeap,
                      unsigned int max_entries, unsigned int dataLength, unsigned int type,
                      unsigned int lengthSearchKey);

/*********************************************************************
* @purpose  Creates the generic avl tree structure and initialize
*
* @param    tree              pointer to the tree structure
* @param    treeHeap          pointer to the tree heap
* @param    dataHeap          pointer to the data heap
* @param    max_entries       maximum number of entries in the tree
* @param    dataLength        length of a data entry
* @param    type              type of data
* @param    lengthSearchKey   length of the search key
*
* @returns  AvlTree_t   pointer to the generic avl tree structure if successful
* @returns  0           if not successful
*
* @notes    RESTRICTIONS:1. the first field in the data structure (of the
*                           data heap) must be the key
*                        2. the last field in the data structure (of the data
*                           heap) must be a void pointer type
*
*
* @end
*********************************************************************/
void avlCreateAvlTreeProcLib(avlTree_t *tree, avlTreeTables_t *treeHeap, void *dataHeap,
                             unsigned int max_entries, unsigned int dataLength, unsigned int type,
                             unsigned int lengthSearchKey);

/*********************************************************************
* @purpose  Set the comparator function for an AVL tree
*           
* @param    tree              pointer to the tree structure
* @param    compare           pointer to the new comparator function
*                             if NULL is used, the default comparator
*                             (memcmp()) will be used.
*
* @returns  pointer to the previous comparator (this function does not fail)
*
* @notes    The default comparator in a generic avl tree is memcmp()
*           There are also some canned comparators supplied (these are
*           declared below).  If the user wants to provide a new type
*           specific comparator it should have the same signature as
*           memcmp():
*
*               int bar(void *a, void *b, size_t key_len);
*
*           and it should have the following integer return values:
*
*               >0 (I generally use 1) if a > b
*               0  if a == b
*               <0 if a < b
*
*           The algorithm for the comparison and the definitions
*           of <, >, and == belong entirely to the comparator. The
*           only requirements are that < and > are consistent, and
*           == ensures uniqueness within the tree.
* @end
*********************************************************************/
avlComparator_t avlSetAvlTreeComparator(avlTree_t *t, avlComparator_t c);

/*********************************************************************
* @purpose  Deletes an avl tree structure
*
* @param    tree              pointer to an avl tree structure
*
* @returns  0
* @returns  -1
* @returns  -2
*
* @notes
*
* @end
*********************************************************************/
int avlDeleteAvlTreeProcLib(avlTree_t *tree);

/*********************************************************************
* @purpose  Resets all entries but the root node 
*
* @param    avl_tree    pointer to the avl tree structure
* @param    max_entries max number of entries in the structure
*
* @returns  void
*
* @notes   
*       
* @end
*********************************************************************/
void avlPurgeAvlTree (avlTree_t *avl_tree, unsigned int max_entries);
/*********************************************************************
* @purpose  Inserts ITEM into TREE.  Returns NULL if the item was inserted,
*           otherwise a pointer to the duplicate item 
*
* @param    avl_tree    pointer to the avl tree structure
* @param    item        pointer to item to be inserted 
*
* @returns  0        if item was inserted
* @returns  void  pointer to a duplicate item, if duplicate exists
* @returns  void  pointer to item, if error
*
* @notes   
*
* @end
*********************************************************************/
void * avlInsertEntry( avlTree_t *avl_tree, void *item);
/*********************************************************************
* @purpose  Searches AVL tree for an item matching ITEM.  If found, the
*           item is removed from the tree and the actual item found is returned
*           to the caller.  If no item matching ITEM exists in the tree,
*           returns NULL. 
*
* @param    avl_tree pointer to the avl tree structure
* @param    item     pointer to item to be deleted
*
* @returns  void *   pointer to the item if deleted
* @returns  0     if item does not exist in the tree
*
* @notes   
*       
* @end
*********************************************************************/
void * avlDeleteEntry (avlTree_t *avl_tree, void *item);
/*****************************************************************
* @purpose  Searches AVL tree for exact or get next match
*
* @param    avl_tree    pointer to the avl tree structure
* @param    item        pointer to item to be found
* @param    matchType   AVL_EXACT or AVL_NEXT
*  
* @returns  void *      pointer to the item if matched
* @returns  0     if item does not exist in the tree
*
* @notes   
*
* @end
*****************************************************************/
void * avlSearch (avlTree_t *avl_tree, void *key, unsigned int flags);

/*****************************************************************
* @purpose  Obtains count of nodes in the tree            
*
* @param    avl_tree    pointer to the avl tree structure
*  
* @returns  count       count of items in the tree
*
* @notes   
*
* @end
*****************************************************************/
unsigned int avlTreeCount (avlTree_t *avl_tree);

/*****************************************************************
* @purpose  Compare short (16 bit signed keys) and indicate relationship
*
* @param    a           Pointer to the first key
* @param    b           Pointer to the second key
* @param    len         Length of the key (unused)
*  
* @returns   0 if the keys are equal
* @returns   1 if key 'a' is greater than key 'b'
* @returns  -1 if key 'a' is less than key 'b'
*
* @notes   
*
* @end
*****************************************************************/
int avlCompareShort16(const void *a, const void *b, size_t len);

/*****************************************************************
* @purpose  Compare long (32 bit signed keys) and indicate relationship
*
* @param    a           Pointer to the first key
* @param    b           Pointer to the second key
* @param    len         Length of the key (unused)
*  
* @returns   0 if the keys are equal
* @returns   1 if key 'a' is greater than key 'b'
* @returns  -1 if key 'a' is less than key 'b'
*
* @notes   
*
* @end
*****************************************************************/
int avlCompareLong32(const void *a, const void *b, size_t len);

/*****************************************************************
* @purpose  Compare ushort (16 bit unsigned keys) and indicate relationship
*
* @param    a           Pointer to the first key
* @param    b           Pointer to the second key
* @param    len         Length of the key (unused)
*  
* @returns   0 if the keys are equal
* @returns   1 if key 'a' is greater than key 'b'
* @returns  -1 if key 'a' is less than key 'b'
*
* @notes   
*
* @end
*****************************************************************/
int avlCompareUShort16(const void *a, const void *b, size_t len);

/*****************************************************************
* @purpose  Compare ulong (32 bit unsigned keys) and indicate relationship
*
* @param    a           Pointer to the first key
* @param    b           Pointer to the second key
* @param    len         Length of the key (unused)
*  
* @returns   0 if the keys are equal
* @returns   1 if key 'a' is greater than key 'b'
* @returns  -1 if key 'a' is less than key 'b'
*
* @notes   
*
* @end
*****************************************************************/
int avlCompareULong32(const void *a, const void *b, size_t len);

/*****************************************************************
* @purpose  Compare IP Address keys and indicate relationship
*
* @param    a           Pointer to the first key
* @param    b           Pointer to the second key
* @param    len         Length of the key (unused)
*  
* @returns   0 if the keys are equal
* @returns   1 if key 'a' is greater than key 'b'
* @returns  -1 if key 'a' is less than key 'b'
*
* @notes   
*
* @end
*****************************************************************/
int avlCompareIPAddr(const void *a, const void *b, size_t len);

/*****************************************************************
* @purpose  Compare IP Network Address (Address and Mask) keys and
*           indicate relationship
*
* @param    a           Pointer to the first key
* @param    b           Pointer to the second key
* @param    len         Length of the key (unused)
*  
* @returns   0 if the keys are equal
* @returns   1 if key 'a' is greater than key 'b'
* @returns  -1 if key 'a' is less than key 'b'
*
* @notes   
*
* @end
*****************************************************************/
int avlCompareIPNetAddr(const void *a, const void *b, size_t len);

/*********************************************************************
* @purpose  Creates the generic avl tree structure and initialize  
*           
* @param    avlTree           pointer to the tree structure
* @param    max_entries       maximum number of entries in the tree 
* @param    dataLength        length of a data entry 
* @param    type              type of data
* @param    compareFcn        pointer to the comparator function
*                             if NULL is used, the default comparator
*                             (memcmp()) will be used.
* @param    lengthSearchKey   length of the search key
*
* @returns  0  if avl tree initialization was successful
* @returns  -1  if not successful   
*
* @notes    RESTRICTIONS:1. First field in the data structure 
*                           (of data heap) must be the key 
*                        2. Last field in the data structure 
*                           (of data heap) must be a void pointer type
*       
* @end
*********************************************************************/
int avlAllocAndCreateAvlTreeProcLib(avlTree_t *avlTree,
                                    unsigned int max_entries,
                                    unsigned int dataLength,
                                    unsigned int type,
                                    avlComparator_t compareFcn,
                                    unsigned int lengthSearchKey);

#endif
