#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/types.h>

struct clounix_node_info {
    struct list_head node;
    char name[64];
    char info[256];
    struct kobj_attribute kobj_attr;
};
