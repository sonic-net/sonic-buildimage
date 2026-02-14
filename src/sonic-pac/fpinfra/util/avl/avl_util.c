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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "avl_api.h"
#include "avl.h"
     
     
/*********************************************************************
* @purpose  Get space for a new node from the tree heap and updates the current  
*           table heap pointer in the avl tree structure
*         
* @param    avl_tree             pointer to the avl tree structure 
*
* @returns  avlTreeTables_t   pointer to new node 
*
* @notes   
*
* @end
*********************************************************************/
avlTreeTables_t* avlNewNewNode(avlTree_t *avl_tree)
{
  avlTreeTables_t* root;

  root=avl_tree->currentTableHeap;
  if (root==NULL)
    return NULL;
  avl_tree->currentTableHeap=root->link[1];

  return  root;
}

/*********************************************************************
* @purpose  get space for a new node from the data heap and updates the current  
*           data heap pointer in the avl tree structure          
*
* @param  avlTree_t  *avl_tree--- pointer to the avl tree structure 
* @param 
*
* @returns void pointer to new node 
*
* @notes   
*       
* @end
*********************************************************************/

void* avlNewNewDataNode(avlTree_t *avl_tree )
{
  void * ptr;
  unsigned int offset_next;

  offset_next=avl_tree->offset_next;
  ptr=avl_tree->currentDataHeap;
  if (ptr==NULL)
    return NULL;

  avl_tree->currentDataHeap=(void *)(*((unsigned long*)((char*)ptr+offset_next)));
  return  ptr;
}

/*********************************************************************
* @purpose  gives memory data node back to data heap and updates the current  
*           data heap pointer in the avl tree structure  
*           
* @param    avl_tree    pointer to the avl tree structure
* @param    ptr         pointer to the data item
*
* @returns  void
*
* @notes   
*       
* @end
*********************************************************************/
void avlNewFreeDataNode(avlTree_t * avl_tree, void *ptr)
{
  unsigned int offset_next;

  offset_next=avl_tree->offset_next;
  bzero((unsigned char *)ptr,avl_tree->lengthData);
  *((unsigned long*)((char*)ptr+offset_next))=(unsigned long)(avl_tree->currentDataHeap);
  avl_tree->currentDataHeap=ptr;

}

/*********************************************************************
* @purpose   gives memory of node back to table heap and updates the current  
*            table heap pointer in the avl tree structure      
*           
* @param     avl_tree      pointer to the avl tree structure
* @param     rootp         pointer to the item stored in the table heap
*
* @returns   void
*
* @notes   
*       
* @end
*********************************************************************/
void avlNewFreeNode(avlTree_t * avl_tree, avlTreeTables_t *rootp)
{
  bzero((unsigned char *)rootp,sizeof(avlTreeTables_t));
  rootp->link[1]=avl_tree->currentTableHeap;
  avl_tree->currentTableHeap=rootp;

}


/*********************************************************************
* @purpose  this is the comparision function which is used to compare two keys 
*
* @param item              pointer to item being searched for 
* @param DataItem          pointer to data item being searched with
* @param lengthSearchKey   length of the search key
*
* @returns AVL_LESS_THAN, if item's key is less than DataItem's key 
* @returns AVL_GREATER_THAN, 1 if item's key is greater than DataItem's key 
* @returns AVL_EQUAL if item's key is equal to DataItem's key
*
* @notes    
*       
* @end
*********************************************************************/
int avlCompareKey(void *item,
                  void *DataItem,
                  unsigned int lengthSearchKey,
                  avlComparator_t func)

{
  int result;

  result = func(item, DataItem, lengthSearchKey);
  if (result <0)
    return AVL_LESS_THAN;
  else if (result> 0)
    return AVL_GREATER_THAN;
  else
    return AVL_EQUAL; 
}


/*********************************************************************
* @purpose  Searches AVL tree TREE for an item matching ITEM.  If found, the
*           item is removed from the tree and the actual item found is returned
*           to the caller.  If no item matching ITEM exists in the tree,
*           returns NULL. 
*
* @param    avl_tree    pointer to the avl tree structure
* @param    item        pointer to item to be deleted
*
* @returns  pointer to the item if deleted
* @returns  NULL if item does not exist in the tree
*
* @notes   
*
* @end
*********************************************************************/
void * avlRemoveEntry( avlTree_t *avl_tree, void *item)
{
   unsigned int done = 0;
   int depth;
   char pathMap[ AVL_MAX_DEPTH+3];
   avlTreeTables_t *path[ AVL_MAX_DEPTH+3];               
   avlTreeTables_t **nodePtr;
   avlTreeTables_t *node = avl_tree->root.link[ LEFT];
  
   if (node == NULL)
     return NULL;

   pathMap[ LEFT] =  LEFT;
   path[ LEFT] = &avl_tree->root;

   /* Find item to remove */
   for ( depth = 1; depth <  AVL_MAX_DEPTH; depth++)
   {
     int diff = avlCompareKey(item, node->data,
                              avl_tree->lengthSearchKey, avl_tree->compare); 

     if (diff == AVL_EQUAL)
       break;

     path[depth] = node;
     if (diff == AVL_GREATER_THAN)
     {
       node = node->link[ RIGHT];
       pathMap[depth] =  RIGHT;
     }
     else if (diff == AVL_LESS_THAN)
     {
       node = node->link[ LEFT];
       pathMap[depth] =  LEFT;
     }
     if (node == NULL)
       return NULL;
   }

   item = node->data;
   nodePtr = &path[depth - 1]->link[(int) pathMap[depth - 1]];
   if (node->link[ RIGHT] == NULL)
   {
     *nodePtr = node->link[ LEFT];
     if (*nodePtr)
       (*nodePtr)->balance = 0;
   }
   else
   { 
     avlTreeTables_t *tempNode = node->link[ RIGHT];
     if (tempNode->link[ LEFT] == NULL)
     {
       tempNode->link[ LEFT] = node->link[ LEFT];
       tempNode->balance = node->balance;
       pathMap[depth] = 1;
       path[depth++] = tempNode;
       *nodePtr = tempNode;
     }
     else
     {
       avlTreeTables_t *node2 = tempNode->link[ LEFT];
       int baseDepth = depth++;
               
       pathMap[depth] = 0;
       path[depth++] = tempNode;

       while (node2->link[ LEFT] != NULL)
       {
         tempNode = node2;
         node2 = tempNode->link[ LEFT];
         pathMap[depth] = 0;
         path[depth++] = tempNode;
       }

       pathMap[baseDepth] = 1;
       path[baseDepth] = node2;
       node2->link[ LEFT] = node->link[ LEFT];
       tempNode->link[ LEFT] = node2->link[ RIGHT];
       node2->link[ RIGHT] = node->link[ RIGHT];
       node2->balance = node->balance;
       *nodePtr = node2;
     }
   }

   avlNewFreeNode (avl_tree,node);

   while ((--depth) && (done == 0))
   {
     avlTreeTables_t *tempNode;
     tempNode = removeAndBalance (path[depth], pathMap[depth], &done);
     if (tempNode != NULL) 
     {
       path[depth-1]->link[(int) pathMap[depth-1]] = tempNode;
     }
   }
   
   avl_tree->count--;
   avlNewFreeDataNode(avl_tree,item);    
   return(void *) item;
}

/*********************************************************************
* @purpose  Balances the avl tree after a node has been removed. 
*
* @param    node        pointer to the node being acted upon
* @param    direction   direction to balance the tree
* @param    done        indication of whether balancing is complete
*
* @returns  pointer to a node
*
* @notes   
*
* @end
*********************************************************************/
avlTreeTables_t *removeAndBalance (avlTreeTables_t *node, int direction, 
                                   unsigned int *done)
{
  int otherDirection, weight, otherWeight;
  avlTreeTables_t *tempNode = NULL;
   
  otherDirection = (direction ==  LEFT) ?  RIGHT :  LEFT;
  weight = (direction ==  LEFT) ?  LEFT_WEIGHT :  RIGHT_WEIGHT;
  otherWeight = (direction ==  LEFT) ?  RIGHT_WEIGHT :  LEFT_WEIGHT;
  direction = (direction ==  LEFT) ?  LEFT :  RIGHT;
        
  if (node->balance == weight)
    node->balance = 0;
  else if (node->balance == 0)
  {
    node->balance = otherWeight;
    *done = 1;   
  }
  else
  {
    tempNode = node->link[otherDirection];

    if (tempNode == 0)
    {
      /* This should never happen.
      */
      *done = 1;   
      return tempNode;
    }

    if ((tempNode->balance == 0) || 
        ((direction ==  RIGHT) && (tempNode == NULL))) 
    {
      node->link[otherDirection] = tempNode->link[direction];
      tempNode->link[direction] = node;
      tempNode->balance = weight;
      *done = 1;
    }
    else if (tempNode->balance == otherWeight)
    {                           
      swapNodes (otherDirection, node, tempNode);
    }
    else
    {
      tempNode = rotateNodes (otherDirection, node, tempNode);
    }
  }
  return tempNode; 
}



/*********************************************************************
* @purpose  Search TREE for an item matching ITEM.  If found, returns a pointer
*           to the address of the item.  If none is found, ITEM is inserted
*           into the tree, and a pointer to the address of ITEM is returned.
*           In either case, the pointer returned can be changed by the caller,
*           or the returned data item can be directly edited, but the key data
*           in the item must not be changed   
*
* @param    avl_tree    pointer to the avl tree structure
* @param    item        pointer to item to be found
*
* @returns  pointer to the item if found or inserted
*
* @notes   
*
* @end
*********************************************************************/
void ** avlFindEntry(avlTree_t *avl_tree, void *item)
{
   int depth;
   avlTreeTables_t *newNode = 0, *node2;
   avlTreeTables_t *baseNode = &avl_tree->root;
   avlTreeTables_t *node;
  
   node = node2 = baseNode->link[ LEFT];

   /* empty tree case */
   if (node == NULL)
   {
     avl_tree->count++;
     newNode = baseNode->link[ LEFT] = avlNewNewNode(avl_tree);
     newNode->data = item;
     newNode->link[ LEFT] = NULL;
     newNode->link[ RIGHT] = NULL;
     newNode->balance =  LEFT;
     return((void *)(&newNode->data));
   }

   /* find match and return or create new node and break */
   for (depth = 0; depth <  AVL_MAX_DEPTH; depth++)  
   {
     int diff = avlCompareKey (item, node2->data,
                               avl_tree->lengthSearchKey, avl_tree->compare);

     /* Traverse down left side of tree. */
     if (diff == AVL_LESS_THAN)
     {
       node2->balanceNeeded = 0;
       newNode = node2->link[ LEFT];
       if (newNode == NULL)
       {
         newNode = avlNewNewNode (avl_tree);
         node2->link[ LEFT] = newNode;
         break;
       }
     }
     /* Traverse down right side of tree */
     else if (diff == AVL_GREATER_THAN)
     {
       node2->balanceNeeded = 1;
       newNode = node2->link[ RIGHT];
       if (newNode == NULL)
       {
         newNode = avlNewNewNode (avl_tree);
         node2->link[ RIGHT] = newNode;
         break;
       }
     }
     /* Found it */
     else
       return((void *)(&node2->data));

     if (newNode->balance !=  LEFT)
     {
       baseNode = node2; /* shift nodes */
       node = newNode;
     }
     node2 = newNode;
   }

   avl_tree->count++;
   newNode->data = item;
   newNode->link[ LEFT] = NULL;
   newNode->link[ RIGHT] = NULL;
   newNode->balance =  LEFT;

   balance (newNode, node, baseNode, avl_tree, (int)node->balanceNeeded);

   return((void *)(&newNode->data));
}

/* This function will balance the added nodes.  */
void balance (avlTreeTables_t *newNode, avlTreeTables_t *node, avlTreeTables_t *baseNode, 
              avlTree_t *avl_tree, int direction)
{
  avlTreeTables_t *shiftNode, *tempNode;
  int weight =  (direction ==  LEFT) ?  LEFT_WEIGHT:  RIGHT_WEIGHT;
  
  shiftNode = node->link[direction];
  while (shiftNode != newNode) 
  {
    shiftNode->balance = shiftNode->balanceNeeded * 2 - 1;
    shiftNode = shiftNode->link[(int) shiftNode->balanceNeeded];
  }
  
  if (((weight ==  LEFT_WEIGHT) && (node->balance >= 0)) ||
      ((weight ==  RIGHT_WEIGHT) && (node->balance <= 0)))
  {
    node->balance += weight;
    return;
  }
  
  tempNode= node->link[direction];
  if (tempNode->balance == weight) 
  {
    shiftNode = tempNode;
    swapNodes(direction, node, tempNode);
  }
  else
    shiftNode = rotateNodes (direction, node, tempNode);

  if (baseNode != &avl_tree->root && node == baseNode->link[ RIGHT]) 
    baseNode->link[ RIGHT] = shiftNode;
  else
    baseNode->link[ LEFT] = shiftNode;

  return;

}

/*********************************************************************
* @purpose  This function will swap the depth of the two imput nodes.
*           The node that is input as oldUpper will become the new lower
*           node, and oldLower will become the new Upper node.  This
*           function will cause a shift in the tree balance.  The 
*           direction parameter indicates which direction to shift. 
*
* @param    oldUpper    upper node in table
* @param    oldLower    lower node in table
*
* @returns  VOID
*
* @notes   
*
* @end
*********************************************************************/

void swapNodes (int direction, avlTreeTables_t *oldUpper, 
                avlTreeTables_t *oldLower)
{
  int otherDirection = (direction ==  LEFT) ?  RIGHT:  LEFT;

  oldUpper->link[direction] = oldLower->link[otherDirection];
  oldLower->link[otherDirection] = oldUpper;
  oldUpper->balance = 0;
  oldLower->balance = 0;
}

/*********************************************************************
* @purpose  This function will rotate the nodes to the left or right
*           depending on the value of diretion.  This
*           function will cause a shift in the tree balance.  The 
*           direction parameter indicates which direction to shift. 
*
* @param    node1     node in table
* @param    node2     node in table
* @param    direction direction to rotate 
*
* @returns  pointer to rotated node.
*
* @notes   
*
* @end
*********************************************************************/

avlTreeTables_t *rotateNodes (int otherDirection, avlTreeTables_t *node1, 
                  avlTreeTables_t *node2)
{
  avlTreeTables_t * node3;
  int direction = (otherDirection ==  LEFT) ?  RIGHT:  LEFT;

  node3 = node2->link[direction];
  node2->link[direction] = node3->link[otherDirection];
  node3->link[otherDirection] = node2;
  node1->link[otherDirection] = node3->link[direction];
  node3->link[direction] = node1;

  node1->balance = node2->balance = 0;
  if (direction== RIGHT)       
  {
    if (node3->balance < 0) 
      node1->balance = 1;    
    else if (node3->balance > 0) 
      node2->balance = -1;
  }
  else
  {
    if (node3->balance > 0) 
      node1 ->balance = -1;
    else if (node3->balance < 0) 
      node2->balance = 1;
  }

  node3->balance = 0;
  return node3;

}
/*********************************************************************
* @purpose  Inserts ITEM into TREE.  Returns NULL if the item was inserted,
*           otherwise a pointer to the duplicate item 
*
* @param    avl_tree    pointer to the avl tree structure
* @param    item        pointer to item to be inserted 
*
* @returns  NULL if item was inserted
* @returns  void  pointer to duplicate item if duplicate exists
* @returns  void  pointer to item, if error
*
* @notes   
*       
* @end
*********************************************************************/

void * avlAddEntry( avlTree_t *avl_tree, void *item)
{
  void **p;
  void * new_entry;
  void *exactEntry;

  new_entry=avlNewNewDataNode(avl_tree);
  if (new_entry==NULL)
  {
    /*check for the condition that the data heap is full 
    but the entry is already present in the tree*/
    exactEntry = avlSearch(avl_tree,item,AVL_EXACT);
    if (exactEntry!=0)
    {
      return exactEntry;
    }
    else
      return item;
  }

  memcpy(new_entry,item,avl_tree->lengthData);

  p = avlFindEntry (avl_tree, new_entry);

  if (*p != new_entry) /*entry not added*/
  {
    avlNewFreeDataNode(avl_tree,new_entry);
    return(*p);
  }
  else
    return(NULL);
}

