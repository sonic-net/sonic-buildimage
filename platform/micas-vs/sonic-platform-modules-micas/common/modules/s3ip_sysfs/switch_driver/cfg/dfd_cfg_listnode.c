/*
 * An dfd_cfg_listnode driver for cfg of listnode devcie function
 *
 * Copyright (C) 2024 Micas Networks Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/list.h>
#include <linux/slab.h>

#include "dfd_cfg_listnode.h"

/**
 * Find node
 * @root: Root node pointer
 * @key: Node index value
 *
 * @return : Node data pointer, NULL failed
 */
void *lnode_find_node(lnode_root_t *root, uint64_t key)
{
    lnode_node_t *lnode;

    if (root == NULL) {
        return NULL;
    }

    /* Traversal query */
    list_for_each_entry(lnode, &(root->root), lst) {
        if (lnode->key == key) {
            return lnode->data;
        }
    }

    return NULL;
}

/**
 * Insert node
 * @root: Root node pointer
 * @key: Node index value
 * @data: data
 *
 * @return : 0-- success, other failures
 */
int lnode_insert_node(lnode_root_t *root, uint64_t key, void *data)
{
    lnode_node_t *lnode;
    void *data_tmp;

    if ((root == NULL) || (data == NULL)) {
        return LNODE_RV_INPUT_ERR;
    }

    /* Check whether the node exists */
    data_tmp = lnode_find_node(root, key);
    if (data_tmp != NULL) {
        return LNODE_RV_NODE_EXIST;
    }

    /* Node memory request */
    lnode = kmalloc(sizeof(lnode_node_t), GFP_KERNEL);
    if (lnode == NULL) {
        return LNODE_RV_NOMEM;
    }

    /* Add to list */
    lnode->key = key;
    lnode->data = data;
    list_add_tail(&(lnode->lst), &(root->root));

    return LNODE_RV_OK;
}

/**
 * Example Initialize the root node
 * @root: Root node pointer
 *
 * @return : 0 Succeeded, others failed
 */
int lnode_init_root(lnode_root_t *root)
{
    if (root == NULL) {
        return LNODE_RV_INPUT_ERR;
    }

    INIT_LIST_HEAD(&(root->root));

    return LNODE_RV_OK;
}

/**
 * Free linked list
 * @root: Root node pointer
 *
 * @return : void
 */
void lnode_free_list(lnode_root_t *root)
{
    lnode_node_t *lnode, *lnode_next;

    if (root == NULL) {
        return;
    }

    /* Iterate to delete the linked list */
    list_for_each_entry_safe(lnode, lnode_next, &(root->root), lst) {
        if (lnode->data) {
            kfree(lnode->data);
            lnode->data = NULL;
            lnode->key = 0;
        }
        list_del(&lnode->lst);
        kfree(lnode);
        lnode = NULL;
    }

    return;

}
