/*
 * Copyright 2022 Clounix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 */

/* FILE NAME:  cmlib_list.h
 * PURPOSE:
 *  this file is used to provide linked list operations to other users.
 *  linked list include: singly linked list, doubly linked list
 *
 * NOTES:
 *  it contains operations as below:
 *      1. create a linked list
 *      2. destroy a linked list
 *      3. insert user data to a linked list
 *      4. delete a user data from a linked list
 *      5. locate a user data from a linked list
 *      6. get next node from a linked list node
 *      7. get previous node from a linked list node
 *      7. get a linked list nodes count
 *
 */
#ifndef CMLIB_LIST_H
#define CMLIB_LIST_H
/* INCLUDE FILE DECLARATIONS
 */
#include <cmlib/cmlib.h>
/* NAMING CONSTANT DECLARATIONS
 */



/* linked list type */
typedef enum
{
    CMLIB_LIST_TYPE_SINGLY = 0,           /* singly linked list type   */
    CMLIB_LIST_TYPE_DOUBLY,               /* doubly linked list type   */
    CMLIB_LIST_TYPE_LAST,
} CMLIB_LIST_TYPE_T;

/* MACRO FUNCTION DECLARATIONS
 */
/* DATA TYPE DECLARATIONS
 */

/* linked list node */
typedef struct CMLIB_LIST_NODE_S
{
    void                     *ptr_data;      /* node data                   */
    struct CMLIB_LIST_NODE_S *ptr_next;      /* point to next link node     */
    struct CMLIB_LIST_NODE_S *ptr_prev;      /* point to previous link node */
} CMLIB_LIST_NODE_T;

struct CMLIB_LIST_S;

/* FUNCTION TYPE NAME: CMLIB_LIST_CMP_FUNC_T
 * PURPOSE:
 *      it is used to find which position the node should be inserted.
 * INPUT:
 *      ptr_node_data   -- the checked linked list node data.
 *      ptr_insert_data -- the data will be inserted.
 * OUTPUT:
 *      None.
 * RETURN:
 *      0     -- the insert node should be inserted before the node.
 *      non 0 -- it is not the right position.
 * NOTES:
 *
 */
typedef I32_T (*CMLIB_LIST_CMP_FUNC_T)(
    void *ptr_node_data,
    void *ptr_insert_data );

/* FUNCTION TYPE NAME: CMLIB_LIST_LOCATE_FUNC_T
 * PURPOSE:
 *      it is used to locate a linked list node.
 * INPUT:
 *      ptr_node_data  -- the checked linked list node data.
 *      ptr_cookie     -- the data used to check the linked list node. it is
 *                          one of locate function parameters.
 * OUTPUT:
 *      None.
 * RETURN:
 *      0     -- locate success
 *      non 0 -- locate failed, should continue to check.
 * NOTES:
 *
 */
typedef I32_T (*CMLIB_LIST_LOCATE_FUNC_T)(
    void *ptr_node_data,
    void *ptr_cookie );


/* FUNCTION TYPE NAME: CMLIB_LIST_DESTROY_FUNC_T
 * PURPOSE:
 *      it is used to release linked list node when destroy a linked list.
 * INPUT:
 *      ptr_node_data  -- the linked list node data will be destroyed.
 * OUTPUT:
 *      None.
 * RETURN:
 *      None.
 * NOTES:
 *
 */
typedef void (*CMLIB_LIST_DESTROY_FUNC_T)(
    void *ptr_node_data );

typedef CMLIB_LIST_DESTROY_FUNC_T CMLIB_LIST_DELETE_FUNC_T;

/* linked list operations */
typedef struct
{
    CLX_ERROR_NO_T (*getNodeData) (
    struct CMLIB_LIST_S      *ptr_list,         /* the node owner                      */
    CMLIB_LIST_NODE_T        *ptr_node,         /* the node will be get the data       */
    void                     **pptr_node_data );/* the data pointer saved in the node  */

    CLX_ERROR_NO_T (*insertByFunc)(                /* it used to insert a node by a function        */
    struct CMLIB_LIST_S      *ptr_list,        /* the linked list in which the node is inserted */
    void                     *ptr_data,        /* the inserted data                             */
    CMLIB_LIST_CMP_FUNC_T    insert_callback );/* the insert function decide where the data is
                                                    * inserted.
                                                    */

    CLX_ERROR_NO_T (*insertToHead)(         /* it used to insert a data to the head          */
    struct CMLIB_LIST_S    *ptr_list,   /* the linked list in which the node is inserted */
    void                   *ptr_data ); /* the inserted data                             */

    CLX_ERROR_NO_T (*insertToTail)(         /* it used to insert a data to the tail          */
    struct CMLIB_LIST_S    *ptr_list,   /* the linked list in which the node is inserted */
    void                   *ptr_data ); /* the inserted data                             */

    CLX_ERROR_NO_T (*insertBefore)(                /* insert a data before a specified node         */
    struct CMLIB_LIST_S    *ptr_list,          /* the linked list in which the node is inserted */
    CMLIB_LIST_NODE_T      *ptr_node,          /* the specified node                            */
    void                   *ptr_insert_data ); /* the inserted data                             */

    CLX_ERROR_NO_T (*insertAfter)(                 /* insert a data after a specified node          */
    struct CMLIB_LIST_S    *ptr_list,          /* the linked list in which the node is inserted */
    CMLIB_LIST_NODE_T      *ptr_node,          /* the specified node                            */
    void                   *ptr_insert_data ); /* the inserted data                             */

    CLX_ERROR_NO_T (*deleteNode)(                  /* delete a node from a linked list     */
    struct CMLIB_LIST_S    *ptr_list,          /* the linked list from which delete    */
    CMLIB_LIST_NODE_T      *ptr_delete_node ); /* the node will be deleted             */

    CLX_ERROR_NO_T (*deleteByData)(
    struct CMLIB_LIST_S *ptr_list,
    void                *ptr_data );

    CLX_ERROR_NO_T (*locateByFunc)(                  /* locate a node from a linked list    */
    struct CMLIB_LIST_S      *ptr_list,          /* the linked list from which locate   */
    void                     *ptr_cookie,        /* the cookie data for locate callback */
    CMLIB_LIST_LOCATE_FUNC_T locate_callback,    /* locate calback function             */
    CMLIB_LIST_NODE_T        **pptr_node );      /* the located node                    */

    CLX_ERROR_NO_T (*locateHead)(                  /* get the head node from a linked list */
    struct CMLIB_LIST_S      *ptr_list,         /* the linked list from which get       */
    CMLIB_LIST_NODE_T        **pptr_node );     /* the head node                        */

    CLX_ERROR_NO_T (*locateTail)(                   /* get the tail node from a linked list */
    struct CMLIB_LIST_S      *ptr_list,         /* the linked list from which get       */
    CMLIB_LIST_NODE_T        **pptr_node );     /* the tail node                        */

    CLX_ERROR_NO_T (*next)(                         /* get next node of the specified node */
    struct CMLIB_LIST_S      *ptr_list,         /* the node of this linked list        */
    CMLIB_LIST_NODE_T        *ptr_node,         /* the speicified node                 */
    CMLIB_LIST_NODE_T        **pptr_next_node); /* the next node                       */

    CLX_ERROR_NO_T (*prev)(                         /* get previous node of the specified node */
    struct CMLIB_LIST_S      *ptr_list,         /* the node of this linked list            */
    CMLIB_LIST_NODE_T        *ptr_node,         /* the speicified node                     */
    CMLIB_LIST_NODE_T        **pptr_prev_node );/* the previous node                       */

    CLX_ERROR_NO_T (*getLength)(                    /* get the node count of a linked list     */
    struct CMLIB_LIST_S      *ptr_list,         /* the linked list                         */
    UI32_T                   *ptr_length);      /* the linked list, output parameter       */

    CLX_ERROR_NO_T (*destroy)(                        /* destroy a linked list     */
    struct CMLIB_LIST_S       *ptr_list,          /* the destroyed linked list */
    CMLIB_LIST_DESTROY_FUNC_T destroy_callback ); /* the destroy function for
                                                       * releasing linked list node data
                                                       */
    CLX_ERROR_NO_T (*deleteAll)(                  /* destroy all nodes from a linked list*/
    struct CMLIB_LIST_S       *ptr_list,           /*  the linked list*/
    CMLIB_LIST_DELETE_FUNC_T delete_callback);   /*  the delete function for releasing node data*/

} CMLIB_LIST_OPS_T;


/* linked list head */
typedef struct CMLIB_LIST_S
{
    CMLIB_LIST_NODE_T *ptr_head_node;       /* linked list head node   */
    CMLIB_LIST_NODE_T *ptr_tail_node;       /* linked list tail node   */
    CMLIB_LIST_TYPE_T type;                 /* list type */
    UI32_T            capacity;             /* max count of nodes in list
                                             * size=0: the capacity is unlimited.
                                             * size>0: the capacity is limited.
                                             */
    void              *ptr_node_pool;       /* node pool */
    UI32_T            node_count;           /* the count of nodes in the list */
    CMLIB_LIST_OPS_T  *ptr_ops;             /* linked list operations         */
} CMLIB_LIST_T;

#endif /* End of CMLIB_LIST_H */

