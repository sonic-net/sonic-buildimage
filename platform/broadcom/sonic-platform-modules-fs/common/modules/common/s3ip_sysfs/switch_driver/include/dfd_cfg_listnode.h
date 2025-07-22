#ifndef __DFD_CFG_LISTNODE_H__
#define __DFD_CFG_LISTNODE_H__

#include <linux/list.h>

/* Returned value */
#define LNODE_RV_OK             (0)
#define LNODE_RV_INPUT_ERR      (-1)    /* Entry error */
#define LNODE_RV_NODE_EXIST     (-2)    /* Node already exists */
#define LNODE_RV_NOMEM          (-3)    /* Node already exists */

/* Root node public structure */
typedef struct lnode_root_s {
    struct list_head root;
} lnode_root_t;

/* Node structure */
typedef struct lnode_node_s {
    struct list_head lst;

    uint64_t key;               /* Node search index value */
    void *data;                 /* The actual data pointer */
} lnode_node_t;

/**
 * Find node
 * @root: Root node pointer
 * @key: Node index value
 *
 * @return : Node data pointer,NULL failed
 */
void *lnode_find_node(lnode_root_t *root, uint64_t key);

/**
 * Insert node
 * @root: Root node pointer
 * @key: Node index value
 * @data: data
 *
 * @return : 0-- success, other failures
 */
int lnode_insert_node(lnode_root_t *root, uint64_t key, void *data);

/**
 * Example Initialize the root node
 * @root: Root node pointer
 *
 * @return : 0-- success, other failures
 */
int lnode_init_root(lnode_root_t *root);

/**
 * Free linked list
 * @root: Root node pointer
 *
 * @return : void
 */
void lnode_free_list(lnode_root_t *root);

#endif /* __DFD_CFG_LISTNODE_H__ */
