/*
 * Copyright(C) 2001-2015 whitebox Network. All rights reserved.
 */
/*
 * dfd_cfg_listnode.c
 * Original Author: sonic_rd@whitebox 2020-02-17
 *
 * Linked list structure module, nodes are organized according to the order of insertion, 
 * linked list creation only query operation, no lock protection
 * Kernel module use, user space <linux/list.h> and kmalloc need to be revised
 *
 * History
 *  [Version]        [Author]                   [Date]            [Description]
 *     v1.0    sonic_rd@whitebox         2020-02-17        Initial version
 *
 */

#include <linux/list.h>
#include <linux/slab.h>
#include "dfd_cfg.h"
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
            if (lnode->debug_on == NODE_DEBUG_ON) {
                return lnode->debug_data;
            }
            return lnode->data;
        }
    }

    return NULL;
}

/**
 * Find node debug mode
 * @root: Root node pointer
 *
 * @return : Node data pointer, NULL failed
 */
int lnode_find_node_debug_mode(lnode_root_t *root)
{
    lnode_node_t *lnode;

    if (root == NULL) {
        return LNODE_RV_NODE_EXIST;
    }

    /* Traversal query */
    list_for_each_entry(lnode, &(root->root), lst) {
        return lnode->debug_on;
    }

    return LNODE_RV_INPUT_ERR;
}

/**
 * reset debug node
 * @root: Root node pointer
 * @debug_on: 0:disable all debug, 1 enable all debug_on
 */
int lnode_set_debug_mode_and_reset(lnode_root_t *root, size_t debug_on)
{
    lnode_node_t *lnode;
    void *swap_tmp;

    if (root == NULL) {
        return LNODE_RV_INPUT_ERR;
    }

    list_for_each_entry(lnode, &(root->root), lst) {

        if (lnode->tmp_debug_data == NULL || lnode->data == NULL || lnode->debug_data == NULL) {
            return LNODE_RV_INPUT_ERR;
        }

        memset(lnode->tmp_debug_data, 0, lnode->data_size);
        memcpy(lnode->tmp_debug_data, lnode->data, lnode->data_size);

        lnode->debug_on = debug_on;

        swap_tmp = lnode->debug_data;
        lnode->debug_data = lnode->tmp_debug_data;
        lnode->tmp_debug_data = swap_tmp;
    }
    return LNODE_RV_OK;
}


/**
 * Insert node
 * @root: Root node pointer
 * @key: Node index value
 * @data: data
 * @data_size: data size
 *
 * @return : 0-- success, other failures
 */
int lnode_insert_node(lnode_root_t *root, uint64_t key, void *data, size_t data_size)
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

    lnode->debug_data = NULL;
    lnode->tmp_debug_data = NULL;

    lnode->debug_on = 0;
    lnode->key = key;
    lnode->data_size = data_size;
    lnode->data = data;

    lnode->debug_data = kmalloc(data_size, GFP_KERNEL);
    if (lnode->debug_data == NULL) {
        kfree(lnode);
        return LNODE_RV_NOMEM;
    }

    lnode->tmp_debug_data = kmalloc(data_size, GFP_KERNEL);
    if (lnode->tmp_debug_data == NULL) {
        kfree(lnode->debug_data);
        kfree(lnode);
        return LNODE_RV_NOMEM;
    }
    
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
        if (lnode->debug_data) {
            kfree(lnode->debug_data);
            lnode->debug_data = NULL;
        }
        if (lnode->tmp_debug_data) {
            kfree(lnode->tmp_debug_data);
            lnode->tmp_debug_data = NULL;
        }
        list_del(&lnode->lst);
        kfree(lnode);
        lnode = NULL;
    }

    return;

}
