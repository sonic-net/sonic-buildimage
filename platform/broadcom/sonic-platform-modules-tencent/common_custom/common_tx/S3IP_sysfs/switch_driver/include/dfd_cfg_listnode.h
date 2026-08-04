#ifndef __DFD_CFG_LISTNODE_H__
#define __DFD_CFG_LISTNODE_H__

#include <linux/list.h>

#define LNODE_RV_OK             (0)
#define LNODE_RV_INPUT_ERR      (-1)
#define LNODE_RV_NODE_EXIST     (-2)
#define LNODE_RV_NOMEM          (-3)

typedef struct lnode_root_s {
    struct list_head root;
} lnode_root_t;

typedef struct lnode_node_s {
    struct list_head lst;

    int key;
    void *data;
} lnode_node_t;

void *lnode_find_node(lnode_root_t *root, int key);

int lnode_insert_node(lnode_root_t *root, int key, void *data);

int lnode_init_root(lnode_root_t *root);

void lnode_free_list(lnode_root_t *root);

#endif /* __DFD_CFG_LISTNODE_H__ */
