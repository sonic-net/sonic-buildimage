#include <linux/list.h>
#include <linux/slab.h>

#include "../include/dfd_cfg_listnode.h"
#include "../../dev_sysfs/include/sysfs_common.h"

void *lnode_find_node(lnode_root_t *root, int key)
{
    lnode_node_t *lnode;

    if (root == NULL){
        return NULL;
    }

    list_for_each_entry(lnode, &(root->root), lst) {
        if (lnode->key == key) {
            return lnode->data;
        }
    }

    return NULL;
}

int lnode_insert_node(lnode_root_t *root, int key, void *data)
{
    lnode_node_t *lnode;
    void *data_tmp;

    if ((root == NULL) || (data == NULL)) {
        return LNODE_RV_INPUT_ERR;
    }

    data_tmp = lnode_find_node(root, key);
    if (data_tmp != NULL) {
        return LNODE_RV_NODE_EXIST;
    }

    lnode = kmalloc(sizeof(lnode_node_t), GFP_KERNEL);
    if (lnode == NULL) {
        return LNODE_RV_NOMEM;
    }

    lnode->key = key;
    lnode->data = data;
    list_add_tail(&(lnode->lst), &(root->root));

    return LNODE_RV_OK;
}

int lnode_init_root(lnode_root_t *root)
{
    if (root == NULL) {
        return LNODE_RV_INPUT_ERR;
    }

    INIT_LIST_HEAD(&(root->root));

    return LNODE_RV_OK;
}

void lnode_free_list(lnode_root_t *root)
{
    lnode_node_t *lnode, *lnode_next;

    if (root == NULL){
        return ;
    }

    list_for_each_entry_safe(lnode, lnode_next, &(root->root), lst) {
        if ( lnode->data ) {
            kfree(lnode->data);
            lnode->data = NULL;
            lnode->key = 0;
        }
        list_del(&lnode->lst);
        kfree(lnode);
        lnode = NULL;
    }

    return ;

}
