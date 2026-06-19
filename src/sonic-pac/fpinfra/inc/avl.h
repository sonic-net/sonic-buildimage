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

 
#ifndef _AVL_H_
#define _AVL_H_

 
#define AVL_LESS_THAN       -1
#define AVL_EQUAL            0
#define AVL_GREATER_THAN     1


#if defined(FUTURE_RELEASE) && FUTURE_RELEASE
/* defines for type of avl tree*/ 
 #define    MAC_ADDR            1
 #define    QVLAN_ADDR          2
 #define    MAC_FILTER          3
/* -----------------------------*/
#endif

#ifndef  AVL_MAX_DEPTH
#define  AVL_MAX_DEPTH	32          
#endif

#define  LEFT 0
#define  RIGHT 1
#define  LEFT_WEIGHT -1
#define  RIGHT_WEIGHT 1

void* avlNewNewDataNode(avlTree_t *avl_tree );
void avlNewFreeDataNode(avlTree_t * avl_tree, void *ptr);
void * avlRemoveEntry( avlTree_t *avl_tree, void *item);
void ** avlFindEntry(avlTree_t *avl_tree, void *item);
void * avlAddEntry( avlTree_t *avl_tree, void *item);
int avlCompareKey(void *item,
                  void *DataItem,
                  unsigned int lengthSearchKey,
                  avlComparator_t func);
void swapNodes (int direction, avlTreeTables_t *oldUpper, 
                avlTreeTables_t *oldLower);
avlTreeTables_t *rotateNodes (int otherDirection, avlTreeTables_t *node1, 
                  avlTreeTables_t *node2);
void balance (avlTreeTables_t *q, avlTreeTables_t *s, 
              avlTreeTables_t *t, avlTree_t *avl_tree, int direction);
avlTreeTables_t *removeAndBalance (avlTreeTables_t *s, int direction, 
                                   unsigned int *done);

#endif
